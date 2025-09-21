/************************************************************************/
/*                                                                      */
/*                              pcm.c	                              	*/
/*                                                                      */
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "ua100.h"

#define ddprintf //dprintf

static int qs_out = QS_OUT;

void cb_notify_in(void *cookie, status_t status, void *data, uint32 actual_len);
void cb_notify_out(void *cookie, status_t status, void *data, uint32 actual_len);


static void
remove_holes_from_in_buffer(void* buf, const usb_iso_packet_descriptor* packet_descriptors, uint32 packet_count )
{
	uint32	i;
	char* 	dst = buf;
	char* 	src = buf;
	
	for(i=0; i<packet_count; i++)
	{
		const size_t pl = packet_descriptors[i].act_len;
		if(dst != src)
			memmove(dst, src, pl);	/* can't use memcpy() here, dst and src overlap */
		dst += pl;
		src += packet_descriptors[i].req_len;
	}
}

static void
queue_packet_out(iso_channel* ch)
{
  iso_packet* packet = ch->next_to_queue;
  status_t err;	
  int i;	
  uint16	normal_packet_size, diff_packet_size;
  const int qso = qs_out;
 	
/*
	FIXME this primitive packetizing algorithm works with UA100 only for TS <= 4,
	A better algorithm (Bresenham) is need for longer buffers.
	BUGBUG each UA100 device has to have its own qs_out & qs_in. 
*/
  normal_packet_size = qso/8;
  normal_packet_size = (normal_packet_size + TS/2)/TS;
  normal_packet_size *= 8;
  diff_packet_size = qso - normal_packet_size*(TS-1);
  packet->packet_descriptors[0].req_len =  diff_packet_size;
  for(i=1; i<TS; i++)
  {
  	packet->packet_descriptors[i].req_len =  normal_packet_size;
  }


  //dprintf("queue_isochronous_out(%d, %p, %p)\n", qs_out, packet->buffer, packet);
  //dprintf("out %d\n", qso);
  packet->buffer_size = qso;
  err = usb->queue_isochronous(ch->pipe, packet->buffer, qso,
							   packet->packet_descriptors, TS,
							   NULL, USB_ISO_ASAP,
							   cb_notify_out, packet);
  if(err)
	dprintf(ID "packet out %p status %x\n", packet, err);

  ch->next_to_queue = packet->next;
} 

static void
queue_packet_in(iso_channel* ch)
{
  iso_packet* packet = ch->next_to_queue;
  status_t err;	

  //dprintf("queue_isochronous_in(%d, %p, %p)\n", QS_IN, packet->buffer, &packet->rle_array);
  //dprintf("in(%d\n", QS_IN);
  err = usb->queue_isochronous(ch->pipe, packet->buffer, QS_IN,
  							   packet->packet_descriptors, TS,
							   NULL, USB_ISO_ASAP,
							   cb_notify_in, packet);
  if (err)
	dprintf(ID "packet in %p status %x\n", packet, err);

  ch->next_to_queue = packet->next;
} 

void
init_iso_channel(iso_channel* ch, size_t buf_size, bool is_in)
{
  int i;

  ch->is_in = is_in;
  ch->quit = FALSE;
  ch->packet_size = buf_size;
  ch->buffer_area =
	create_area(CHIP_NAME "_buffer", (void**) &ch->buffers,
				B_ANY_KERNEL_ADDRESS,
				((NB * buf_size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1),
				B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
  ddprintf(ID "buffer_area %d @ 0x%08x\n", ch->buffer_area, ch->buffers);

  for(i = 0; i < NB; i++) {
	ch->iso_packets[i].channel = ch;
	ch->iso_packets[i].buffer = ch->buffers + i * buf_size;
	ch->iso_packets[i].next = &ch->iso_packets[(i + 1) % NB];  
	ch->iso_packets[i].rw_count = 0;
	if(is_in)
	{
		int	j;
		usb_iso_packet_descriptor* pd = ch->iso_packets[i].packet_descriptors;
		for(j=0; j<TS; j++)
			pd[j].req_len = QS_IN/TS;
	}
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
  if (ch->is_in)
	ch->rw_count = ch->next_clock * 441 / 2500;
  else {
	qs_out = QS_OUT;
	ch->rw_count = ch->next_clock * 441 / 1250;
  }

  release_ben(&ch->lock);	// BUGBUG ??

  for(i = 0; i < NB; i++)
	if (ch->is_in)
	  queue_packet_in(ch);
	else
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
start_audio(ua100dev* p)
{
  if (p->audio_open) {
	start_iso_channel(&p->audio_in);
	start_iso_channel(&p->audio_out);
  }
}

void
stop_audio(ua100dev* p)
{
  if (p->audio_open) {
	stop_iso_channel(&p->audio_in);
	stop_iso_channel(&p->audio_out);
  }
}

status_t
pcm_open(const char* dname, uint32 flags, void** cookie)
{	
  int i = dname[strlen(dname) - 1] - '1';
  status_t err;

  ddprintf(ID "pcm_open()\n");
  lock_dev_table();
  err = ua100_open(dname, i);
  if (err == B_OK) {
	ua100dev* p = pdev[i];
	if (p->audio_open) {
	  ua100_close(p);
	  err = ENODEV;
	}
	else {
	  p->audio_open = true;
	  init_iso_channel(&p->audio_in, QS_IN, TRUE);
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
  ua100dev* p = (ua100dev*) cookie;
  status_t err;

  ddprintf(ID "pcm_close(%p)\n", cookie);
  lock_dev_table();
  err = ua100_close(cookie);
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
  ua100dev* p = (ua100dev*) cookie;
  status_t err;

  lock_dev_table();
  err = ua100_free(cookie);
  if (err == B_OK) {
	uninit_iso_channel(&p->audio_out);
	uninit_iso_channel(&p->audio_in);
  }
  unlock_dev_table();

  return err;
}

void
cb_notify_in(void *cookie, status_t status, void *data, uint32 actual_len)
{
  iso_packet* packet = (iso_packet*) cookie;
  iso_channel* channel = packet->channel;
  cpu_status ps;
  status_t err;

  if (status)
	if((status == B_CANCELED) || channel->quit)
	  return;
	else
	  ddprintf(ID "cb_notify_in(): status: 0x%04x\n", status);

  LOCK(channel->time_lock, ps);
  packet->time = system_time();
  packet->status = status;
  packet->buffer_size = actual_len;
  qs_out = min(2 * packet->buffer_size, QS_OUT);
  //queue_packet_in(channel);
  channel->rw_count += packet->buffer_size;
  packet->rw_count = channel->rw_count;
  UNLOCK(channel->time_lock, ps);
  /* ATTTENTION: we can touch the buffer here (in callback) only because it was malloc()/create_area()ed in the kernel
  	and it belongs to the kernel address space, so a kernel thread can touch it.
  */
  remove_holes_from_in_buffer(packet->buffer, packet->packet_descriptors, TS);	
  queue_packet_in(channel);	// BUGBUG locking?


  err = acquire_sem_etc(channel->available_packets, NB, B_TIMEOUT, 0);
  //	if (err == B_OK)
  //	  kprintf("e");
  release_sem(channel->available_packets);
 }

void
cb_notify_out(void *cookie, status_t status, void *data, uint32 actual_len)
{
  iso_packet* packet = (iso_packet*) cookie;
  iso_channel* channel = packet->channel;
  cpu_status ps;
  status_t err;

  if (status) {
	//	memset(packet->buffer, 0, QS_OUT);
	if((status == B_CANCELED) || channel->quit) {
	  return;
	}
	ddprintf(ID "cb_notify_out(): status: 0x%04x\n", status);
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
pcm_read(void *cookie, off_t pos, void *buf, size_t *count)
{
  audio_buffer_header* h = (audio_buffer_header*) buf - 1;
  bigtime_t		buffer_duration = *count * 2500LL / 441;
  ua100dev*		device = (ua100dev*) cookie; 
  iso_channel*	ch = &device->audio_in;
  bool			first = true;
  size_t		recd = 0;
  size_t		unrecd = *count;
  size_t		data_size;
  cpu_status	ps;
  status_t		err;

  //dprintf(ID "read %d\n", *count);

  while (unrecd > 0) {
	if (ch->remain == 0) {
	  err = acquire_sem_etc(ch->available_packets, 1, B_TIMEOUT, 4000*1000);
	  if (err == B_TIMED_OUT) {
		dprintf(ID "pcm_read(): TIMED_OUT\n");
		*count = recd;
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
		h->time = (ch->current_time - ch->remain * 2500LL / 441);
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
	  unrecd = 0;
	else {
	  data_size = min(unrecd, ch->remain);
	  memcpy((char*) buf + recd, ch->buf_ptr, data_size);
	  recd += data_size;
	  unrecd -= data_size;
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

status_t
pcm_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
  audio_buffer_header* h = (audio_buffer_header*) buf - 1;
  bigtime_t		buffer_duration = *count * 2500LL / 441;
  ua100dev*		device = (ua100dev*) cookie; 
  iso_channel*	ch = &device->audio_out;
  bool			first = true;
  size_t		sent = 0;
  size_t		unsent = *count;
  size_t		data_size;
  int32			frames;
  int32*		src;
  int32*		dst;
  cpu_status	ps;
  status_t		err;
  int			i;

  //dprintf(ID "write %d at %Ld from %p\n", *count, pos, buf);
  //kprintf("w%d\n", *count);
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
		h->time = (ch->current_time - ch->remain * 1250LL / 441
				   + NB * TS * 1000);
		h->sample_clock = (ch->current_count - ch->remain) * 1250LL / 441;
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
	  data_size = min(unsent, ch->remain / 2);
	  frames = data_size / 4;
	  src = (int32*) ((char*) buf + sent);
	  dst = (int32*) ch->buf_ptr;
	  for (i = 0; i < frames; i++) {
		dst[2*i] = src[i];
		dst[2*i+1] = 0;
	  }
	  sent += data_size;
	  unsent -= data_size;
	  ch->remain -= 2 * data_size;
	  if (ch->remain)
		ch->buf_ptr += 2 * data_size;
	  else
		ch->buf_ptr = NULL;
	}
	release_ben(&ch->lock);
  }
  return B_OK;
}
