/************************************************************************/
/*                                                                      */
/*                              pcm.c	                              	*/
/*                                                                      */
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "speaker.h"

#define ddprintf //dprintf

static int qs_out = QS_OUT;

void cb_notify_out(void *cookie, status_t status, void *data, uint32 actual_len);

static void
queue_packet_out(iso_channel* ch)
{
  iso_packet* packet = ch->next_to_queue;
  status_t err;

  packet->buffer_size = qs_out;
  usb->queue_isochronous(ch->pipe, packet->buffer, packet->buffer_size,
							packet->packet_descriptors, TS,
							NULL, USB_ISO_ASAP,
							cb_notify_out, packet);
  ch->next_to_queue = packet->next;
} 

void
init_iso_channel(iso_channel* ch, size_t buf_size, bool is_in)
{
  int i,j;
  uint16	normal_packet_size, diff_packet_size;


  ch->quit = FALSE;
  ch->packet_size = buf_size;
  ch->buffer_area =	/* FIXME change to malloc() or use client's buffers */
	create_area(CHIP_NAME "_buffer", (void**) &ch->buffers,
				B_ANY_KERNEL_ADDRESS,
				((NB * buf_size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1),
				B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
  ddprintf(ID "buffer_area %d @ 0x%08x\n", ch->buffer_area, ch->buffers);

	/* FIXME  better packetizing/synchronization algorithm is needed. The current one produces
	44000 Hz sampling frequency insted of 44100 Hz then TS<10. */
  normal_packet_size = buf_size/SAMPLE_SIZE;
  normal_packet_size = (normal_packet_size + TS/2)/TS;
  normal_packet_size *= SAMPLE_SIZE;
  diff_packet_size = buf_size - normal_packet_size*(TS-1);

  for(i = 0; i < NB; i++) {
	ch->iso_packets[i].channel = ch;
	ch->iso_packets[i].buffer = ch->buffers + i * buf_size;
	ch->iso_packets[i].next = &ch->iso_packets[(i + 1) % NB];  
	ch->iso_packets[i].rw_count = 0;
	 	
	ch->iso_packets[i].packet_descriptors[0].req_len =  diff_packet_size;
	for(j=1; j<TS; j++)
		ch->iso_packets[i].packet_descriptors[j].req_len =  normal_packet_size;
  }

  ch->rw_count = 0;
  ch->next_time = 0;
  ch->next_clock = 0;
  init_ben(&ch->lock, "iso channel lock");
}

void
uninit_iso_channel(iso_channel* ch)
{
  acquire_ben(&ch->lock);
  delete_ben(&ch->lock);
  delete_area(ch->buffer_area);
}

void
start_iso_channel(iso_channel* ch)
{
  int i;
  ddprintf(ID "start_iso_channel()\n");
  memset(ch->buffers, 0, NB * ch->packet_size);
  ch->quit = FALSE;

  acquire_ben(&ch->lock);
  ch->available_packets = create_sem(0, "iso channel packets");
  ch->next_to_queue = &ch->iso_packets[0];
  ch->current_rw = &ch->iso_packets[NB-1];
  ch->remain = 0;
  ch->buf_ptr = NULL;
  qs_out = QS_OUT;
  ch->rw_count = ch->next_clock * 441 / 2500;

  release_ben(&ch->lock);	/* BUGBUG ? */

  for(i = 0; i < NB; i++)
	queue_packet_out(ch);

  //release_ben(&ch->lock);
}

void
stop_iso_channel(iso_channel* channel)
{
  channel->quit = TRUE;
  usb->cancel_queued_transfers(channel->pipe);
  acquire_ben(&channel->lock);
  delete_sem(channel->available_packets);
  channel->available_packets = -1;
  release_ben(&channel->lock);
}

void
start_audio(spkr_dev* p)
{
  if (p->audio_open) {
	start_iso_channel(&p->audio_out);
	if (p->lego_api)
	  init_params(p->lego_api);
  }
}

void
stop_audio(spkr_dev* p)
{
  if (p->audio_open)
	stop_iso_channel(&p->audio_out);
}

status_t
pcm_open(const char* dname, uint32 flags, void** cookie)
{	
  int i = dname[strlen(dname) - 1] - '1';
  status_t err;

  ddprintf(ID "pcm_open()\n");
  lock_dev_table();
  err = spkr_open(dname, i);
  if (err == B_OK) {
	spkr_dev* p = pdev[i];
	if (p->audio_open) {
	  spkr_close(p);
	  err = ENODEV;
	}
	else {
	  p->audio_open = true;
	  p->lego_api = NULL;
	  init_iso_channel(&p->audio_out, QS_OUT, FALSE);
	  start_audio(p);
	  *cookie = p;
	}
  }
  unlock_dev_table();

  return err;
}

status_t
pcm_close(void *cookie)
{
  spkr_dev* p = (spkr_dev*) cookie;
  status_t err;

  ddprintf(ID "pcm_close(%p)\n", cookie);
  lock_dev_table();
  err = spkr_close(cookie);
  if (err == B_OK) {
	stop_audio(p);
	p->audio_open = false;
  }
  unlock_dev_table();
  ddprintf(ID "pcm_close(%p): done.\n", cookie);
  snooze(2000); // wait for callbacks... this is not optimal XXX

  return err;
}

status_t
pcm_free(void *cookie)
{
  spkr_dev* p = (spkr_dev*) cookie;
  status_t err;

  lock_dev_table();
  err = spkr_free(cookie);
  if (err == B_OK)
	uninit_iso_channel(&p->audio_out);
  unlock_dev_table();

  return err;
}

void
cb_notify_out(void *cookie, status_t status, void *data, uint32 actual_len)
{
  iso_packet* packet = (iso_packet*) cookie;
  iso_channel* channel = packet->channel;
  cpu_status ps;
  status_t err;

  if(channel->quit)
  	return;
  	 
  if (status) {
	if(status == B_CANCELED)
	  return;
	//ddprintf(ID "cb_notify_out(): status: 0x%04x\n", status);
  }

  LOCK(channel->time_lock, ps);
  packet->time = system_time();
  packet->status = status;
  //queue_packet_out(channel);
  channel->rw_count += packet->buffer_size;
  packet->rw_count = channel->rw_count;
  UNLOCK(channel->time_lock, ps);

  queue_packet_out(channel);	// BUGBUG locking?

  err = acquire_sem_etc(channel->available_packets, NB, B_TIMEOUT, 0);
  //  if (err == B_OK)
  //	kprintf("e");
  release_sem(channel->available_packets);
}

status_t
pcm_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
  audio_buffer_header* h = (audio_buffer_header*) buf - 1;
  bigtime_t		buffer_duration = *count * 2500LL / 441;
  spkr_dev*		device = (spkr_dev*) cookie; 
  iso_channel*	ch = &device->audio_out;
  bool			first = true;
  size_t		sent = 0;
  size_t		unsent = *count;
  size_t		data_size;
  cpu_status	ps;
  status_t		err;

  //dprintf(ID "write %d at %Ld from %p\n", *count, pos, buf);
  //dprintf("w%d\n", unsent);
  //static bigtime_t t1, t2;
  //t1 = system_time();
  //kprintf("%d\n", (int)(t1-t2)/1000);
  //t2 = t1;

  while (unsent > 0) {
	if(ch->remain == 0) {
	  err = acquire_sem_etc(ch->available_packets, 1, B_TIMEOUT, 4000*1000);
	  if (err == B_TIMED_OUT) {
		dprintf(ID "pcm_write(): TIMED_OUT\n");
		*count = sent;
		return err;
	  }
	  
	  acquire_ben(&ch->lock);
	  if (ch->available_packets >= 0) {
		LOCK(ch->time_lock, ps);
		ch->current_rw = ch->current_rw->next;
		ch->current_time = ch->current_rw->time;
		ch->current_count = ch->current_rw->rw_count;
		ch->remain = ch->current_rw->buffer_size;
		ch->buf_ptr = ch->current_rw->buffer;
		UNLOCK(ch->time_lock, ps);
	  }
	}
	else
	  acquire_ben(&ch->lock);

	if (device->lego_api && first) {
	  first = false;
	  if (ch->available_packets >= 0) {
		h->time = (ch->current_time - ch->remain * 2500LL / 441
				   + NB * TS * 1000);
		h->sample_clock = (ch->current_count - ch->remain) * 2500LL / 441;
	  }
	  else {
		h->time = ch->next_time;
		h->sample_clock = ch->next_clock;
	  }
	  ch->next_time = h->time + buffer_duration;
	  ch->next_clock = h->sample_clock + buffer_duration;
	}

	if (ch->available_packets < 0 || ch->buf_ptr == NULL)
	  unsent = 0;
	else {
	  data_size = min(unsent, ch->remain);
	  memcpy(ch->buf_ptr, (char*) buf + sent, data_size);
	  sent += data_size;
	  unsent -= data_size;
	  ch->remain -= data_size;
	  if (ch->remain)
		ch->buf_ptr += data_size;
	  else
		ch->buf_ptr = NULL;
	}
	release_ben(&ch->lock);
  }

  return B_OK;
}
