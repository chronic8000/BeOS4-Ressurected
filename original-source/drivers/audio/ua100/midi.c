/************************************************************************/
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include <sound.h>
#include "ua100.h"

#define ddprintf //dprintf

#if MIDI_INTERRUPT
#define QUEUE_INPUT usb->queue_interrupt
#else
#define QUEUE_INPUT usb->queue_bulk
#endif

static void queue_send(ua100dev* dev, xfer_buf* xb);
static void queue_recv(ua100dev* dev, xfer_buf* xb);

static const uint8 chunk_bytes[] = {
  0, 0, 2, 3, 3, 1, 2, 3,
  3, 3, 3, 3, 2, 2, 3, 1
};

#define CHUNK_BYTES(CH) chunk_bytes[(CH)->head & 15]
#define CHUNK_CHAN(CH) ((CH)->head >> 4)

void
mixer_change(ua100dev* p, int32 channel, int32 control, int32 value)
{
  size_t size = 3;
  uint8 msg[3];
  msg[0] = 0xb0 + ((channel - 1) & 15);
  msg[1] = control & 0x7f;
  msg[2] = value & 0x7f;
  midi_write(&p->chan[2], 0, msg, &size);
  //dprintf("(%d %d) => %d\n", channel, control, value);
}

#if 0

typedef struct {
  uint8 data[16];
} ua100_sysex;

void
sysex_DT1(ua100dev* p, uint32 address, uint32 size, uint32 data)
{
  static const ua100_sysex template = {
	0xf0, 0x41, 0x10, 0x00, 0x11, 0x12, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xf7, 0xf7, 0xf7, 0xf7};
  ua100_sysex sysex = template;
  size_t total_size = size + 12;
  uint32 cs;

  address &= 0x7f7f7f7f;
  data &= 0x7f7f7f7f;
  //dprintf(ID "setting %p to %04x\n",address,data);

  cs = address;
  sysex.data[9] = address;
  address >>= 8;
  cs += address;
  sysex.data[8] = address;
  address >>= 8;
  cs += address;
  sysex.data[7] = address;
  address >>= 8;
  cs += address;
  sysex.data[6] = address;
  while (size--) {
	cs += data;
	sysex.data[10 + size] = data;
	data >>= 8;
  }
  sysex.data[total_size - 2] = 127 & -cs;

  midi_write(&p->chan[2], 0, &sysex, &total_size);
}

#endif

static void
recv_enqueue(midi_channel* chan, int count, uchar* buf)
{
  chan->recvq_count += count;
  while (count--) {
	chan->recvq_bufr[chan->recvq_head] = *buf++;
	chan->recvq_head = (chan->recvq_head + 1) & SEND_MASK;
  }
}

static void
recv_dequeue(midi_channel* chan, int count, uchar* buf)
{
  chan->recvq_count -= count;
  while (count--) {
	*buf++ = chan->recvq_bufr[chan->recvq_tail];
	chan->recvq_tail = (chan->recvq_tail + 1) & SEND_MASK;
  }
}

static void
send_enqueue(midi_channel* chan, int count, uchar* buf)
{
  chan->sendq_count += count;
  while (count--) {
	chan->sendq_bufr[chan->sendq_head] = *buf++;
	chan->sendq_head = (chan->sendq_head + 1) & SEND_MASK;
  }
}

#define LOCK_RECVQ(DEV,PS) LOCK((DEV)->recvq_lock,PS)
#define LOCK_SENDQ(DEV,PS) LOCK((DEV)->sendq_lock,PS)
#define UNLOCK_RECVQ(DEV,PS) UNLOCK((DEV)->recvq_lock,PS)
#define UNLOCK_SENDQ(DEV,PS) UNLOCK((DEV)->sendq_lock,PS)

void
init_midi(ua100dev* p)
{
  int i;
  midi_chunk* ch;
  char name[32];

  sprintf(name, ID "send_lock");
  p->send_lock = create_sem(1, name);
  sprintf(name, ID "recv_lock");
  p->recv_lock = create_sem(1, name);

  for (i = 0; i < CHAN_COUNT; i++) {
	p->chan[i].dev = p;
	sprintf(name, ID "send_count:%d", i);
	p->chan[i].send_count = create_sem(SEND_DEPTH, name);
	sprintf(name, ID "recv_count:%d", i);
	p->chan[i].recv_count = create_sem(0, name);
  }

  /* ensure that we're 32-byte aligned so we don't cross
	 a physical page boundary. XXX assumption of chunk*4 */
  ch = (midi_chunk*) (((int32) p->raw & 0xFFFFFFE0) + 0x20 );

  for (i = 0; i < XFER_BUFS; i++) {
	if (i < XFER_BUFS / 2) {
	  p->xfer_bufs[i].next = p->send_bufs;
	  p->send_bufs = p->xfer_bufs + i;
	}
	else {
	  p->xfer_bufs[i].next = p->recv_bufs;
	  p->recv_bufs = p->xfer_bufs + i;
	}
	p->xfer_bufs[i].dev = p;
	p->xfer_bufs[i].ch = ch;
	ch += CHUNKS_PER_BUF;
  }
}

void
uninit_midi(ua100dev* p)
{
  int i;

  delete_sem(p->send_lock);
  p->send_lock = -1;
  delete_sem(p->recv_lock);
  p->recv_lock = -1;
  for (i = 0; i < CHAN_COUNT; i++) {
	delete_sem(p->chan[i].send_count);
	p->chan[i].send_count = -1;
	delete_sem(p->chan[i].recv_count);
	p->chan[i].recv_count = -1;
  }
}

void
start_midi(ua100dev* p)
{
  ddprintf(ID "Firing up MIDI listener\n");
  queue_recv(p, NULL);
}

void
stop_midi(ua100dev* p)
{
  status_t err = usb->cancel_queued_transfers(p->midi_in);
  if (err != B_OK)
	ddprintf(ID "stop_midi(): cancel_queued_transfers(in): %x\n", err);
  err = usb->cancel_queued_transfers(p->midi_out);
  if (err != B_OK)
	ddprintf(ID "stop_midi(): cancel_queued_transfers(out): %x\n", err);
}

status_t
midi_open (const char* name, uint32 flags, void** cookie)
{
  int i = name[strlen(name) - 3] - '1';
  int n = name[strlen(name) - 1] - '1';
  status_t err;

  ddprintf(ID "midi_open()\n");

  if (n < 0 || n >= CHAN_COUNT)
	return ENODEV;

  lock_dev_table();
  err = ua100_open(name, i);
  if (err == B_OK) {
	midi_channel* chan = &pdev[i]->chan[n];
	++chan->open_count;
	*cookie = chan;
  }
  unlock_dev_table();

  return err;
}

status_t
midi_close (void* cookie)
{
  midi_channel* chan = (midi_channel*) cookie;
  status_t err;

  ddprintf(ID "midi_close(%p)\n", cookie);
  lock_dev_table();
  err = ua100_close(chan->dev);
  if (err == B_OK)
	--chan->open_count;
  unlock_dev_table();
  snooze(2000); // wait for callbacks... this is not optimal XXX

  return err;
}

status_t
midi_free(void * cookie)
{
  status_t err;
  lock_dev_table();
  err = ua100_free(((midi_channel*) cookie)->dev);
  unlock_dev_table();
  return err;
}


status_t
midi_control(void* cookie, uint32 msg, void* buf, size_t len)
{
  return B_ERROR;
}


status_t
midi_read (void* cookie, off_t pos, void *buf, size_t* count)
{
  midi_channel* chan = (midi_channel*) cookie;
  int sz = *count;
  cpu_status ps;

//	dprintf(ID "read %x %d %x %d\n", cookie, (int) pos, buf, (int) *count);

  if (acquire_sem_etc(chan->recv_count, sz, B_CAN_INTERRUPT, 0)) {
	*count = 0;
	if(!chan->dev->connected)
	  return ENODEV;
	return B_ERROR;
  }

  LOCK_RECVQ(chan->dev, ps);
  recv_dequeue(chan, sz, buf);
  UNLOCK_RECVQ(chan->dev, ps);
  return B_OK;
}


status_t 
midi_write (void* cookie, off_t pos, const void* buf, size_t* count)
{
  midi_channel* chan = (midi_channel*) cookie;
  ua100dev* dev = chan->dev;
  int sz = *count;
  int send_count;
  cpu_status ps;
  midi_chunk data;

  //dprintf(ID "write %p\n", *(int*)buf);

  while (sz) {
	/* packetize the data */
	send_count = (sz > 16 ? 16 : sz);

	if (acquire_sem_etc(chan->send_count, send_count, B_CAN_INTERRUPT, 0)) {
	  *count = 0;
	  if (!dev->connected)
		return ENODEV;
	  return B_ERROR;
	}

	LOCK_SENDQ(dev, ps);
	send_enqueue(chan, send_count, (uchar*) buf);
	UNLOCK_SENDQ(dev, ps);

	acquire_sem(dev->send_lock);	
	chan->dev->send_count += send_count;
	queue_send(dev, NULL);
	release_sem(dev->send_lock);

	buf += send_count;
	sz -= send_count;
  }	

  return B_OK;	
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void 
send_cb (void* cookie, status_t status, void* data, uint32 actual_len)
{
  xfer_buf* xb = (xfer_buf*) cookie;
  ua100dev* dev = xb->dev;

  //dprintf(ID "send cb %d\n",status);
  if (status) {
	if(status == B_CANCELED)
	  return;
	if(dev->connected)
	  dprintf(ID "send_cb(): status: 0x%04x\n", status);
  }

  acquire_sem(dev->send_lock);	
  queue_send(dev, xb);
  release_sem(dev->send_lock);
}

static void 
recv_cb(void* cookie, status_t status, void* data, uint32 actual_len)
{
  xfer_buf* xb = (xfer_buf*) cookie;
  ua100dev* dev = xb->dev;
  midi_chunk* ch = xb->ch;
  midi_chunk* end = &ch[actual_len / sizeof(*ch)];
  cpu_status ps;

  //dprintf(ID "recv cb %d\n",status);
  if (status) {
	if(status == B_CANCELED)
	  return;
	if(dev->connected)
	  dprintf(ID "recv_cb(): status: 0x%04x\n", status);
  }
  else while (ch < end) {
	if (CHUNK_BYTES(ch) && CHUNK_CHAN(ch) < CHAN_COUNT) {
	  midi_channel* chan = &dev->chan[CHUNK_CHAN(ch)];
	  int recv_count = CHUNK_BYTES(ch);

	  LOCK_RECVQ(dev, ps);
	  if (recv_count > RECV_DEPTH - chan->recvq_count) {
		chan->stat_drop += recv_count;
		//dprintf("VDSP dropped %d bytes\n", chan->stat_drop);
	  }
	  else {
		chan->stat_recv += recv_count;
		recv_enqueue(chan, recv_count, ch->data);
		release_sem_etc(chan->recv_count, recv_count, B_DO_NOT_RESCHEDULE);
	  }
	  UNLOCK_RECVQ(dev, ps);
	}
	++ch;
  }
  queue_recv(dev, xb);
}

/* dev->send_lock must be held while this is called */
static void
queue_send(ua100dev* dev, xfer_buf* xb)
{
  if (xb) {
	xb->next = dev->send_bufs;
	dev->send_bufs = xb;
  }
  else
	xb = dev->send_bufs;

  if (!dev->connected)
	return;

  while (dev->send_count && xb) {
	midi_chunk* ch = xb->ch;
	midi_chunk* end = &ch[CHUNKS_PER_BUF];

	while (dev->send_count && ch < end) {
	  midi_channel* chan = &dev->chan[dev->send_channel];
	  cpu_status ps;

	  ch->head = 0;
	  LOCK_SENDQ(dev, ps);
	  if (chan->sendq_count) {
		int send_count = parse_midi(chan, ch);
		chan->stat_sent += send_count;
		dev->send_count -= send_count;
		release_sem_etc(chan->send_count, send_count, B_DO_NOT_RESCHEDULE);
	  }
	  UNLOCK_SENDQ(dev, ps);

	  if (ch->head) {
		ch->head |= dev->send_channel << 4;
		++ch;
	  }
	  dev->send_channel = (dev->send_channel + 1) % CHAN_COUNT;
	}

	if (ch > xb->ch) {
	  size_t size = (char*) ch - (char*) xb->ch;
	  if (usb->queue_bulk(dev->midi_out, xb->ch, size, send_cb, xb)) {
		dprintf(ID "cannot queue send\n");
		break;
	  }
	  xb = dev->send_bufs = xb->next;
	}
  }
}

static void
queue_recv(ua100dev* dev, xfer_buf* xb)
{
//	dprintf(ID "queue recv %x %x\n",dev,xb);

  acquire_sem(dev->recv_lock);
  if (xb) {
	xb->next = dev->recv_bufs;
	dev->recv_bufs = xb;
  }
  else
	xb = dev->recv_bufs;

  if (dev->connected)
	while (xb) {
	  status_t err = QUEUE_INPUT(dev->midi_in, xb->ch, 32, recv_cb, xb);
	  if (err != B_OK) {
		dprintf(ID "cannot queue receive\n");
		break;
	  }
	  dev->recv_bufs = xb->next;
	  xb = xb->next;
	}
  release_sem(dev->recv_lock);
}
