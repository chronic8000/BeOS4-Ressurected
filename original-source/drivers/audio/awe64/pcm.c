
#include "awe_private.h"

static status_t pcm_open(const char *name, uint32 flags, void **cookie);
static status_t pcm_close(void *cookie);
static status_t pcm_free(void *cookie);
static status_t pcm_control(void *cookie, uint32 op, void *data, size_t len);
static status_t pcm_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t pcm_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks pcm_hooks = {
    &pcm_open,
    &pcm_close,
    &pcm_free,
    &pcm_control,
    &pcm_read,
    &pcm_write
};

static pcm_cfg default_pcm = {
	44100.0,				/* sample rate */
	2,						/* channels */
	0x2,					/* format */		
	!!B_HOST_IS_BENDIAN,	/* bendian? */
	0,						/* header size */
	MAX_DMA_BUF_SIZE/2,
	MAX_DMA_BUF_SIZE/2
};


static int printed = 0;

static status_t
start_dma (pcm_dma* dma)
{
  pcm_dev* port = dma->port;
  sb_card* sb = port->card;
  int samples = (dma->wide_p ? dma->buf_size / 2 : dma->buf_size);
  int xfers = (dma->channel < 4 ? dma->buf_size : dma->buf_size / 2);

	if (printed < 4) {
		dprintf("awe64: DMA half-page is %d\n", dma->buf_size/2);
		printed++;
	}
  dma->mode = DMA_RUNNING;
  dma->inth_count = 0;
  dma->rw_count = 0;

  START_ISA_DMA(dma->channel,
				dma->buf_phys_addr,
				xfers,
				(dma->capture_p ? 0x14 : 0x18),
				0x34);

  dsp_write_n(sb,
			  dma->start,
			  3,
			  ((samples / 2 - 1) << 8) + 0x30);

  dma->last_int = system_time();
  return B_NO_ERROR;
}


static void
stop_dma (pcm_dma* dma)
{
  pcm_dev* port = dma->port;
  sb_card* sb = port->card;

  /* disable capture & playback */
  dsp_write_n(sb, dma->exit, 0, 0);

  delete_sem(dma->sem);
  dma->mode = DMA_STOPPED;
  dma->index = 0;
}


static status_t 
configure_pcm(pcm_dev* port, pcm_cfg* config, bool force)
{
  uint16 rate;

  ddprintf(CHIP_NAME ": configure_pcm()\n");

  /* check args */
  if (config->sample_rate < 4000.0)
	config->sample_rate = 4000.0;
  if (config->sample_rate > 48000.0)
	config->sample_rate = 48000.0;

  config->channels = 2;
  config->format = 0x02;
  config->big_endian = !!config->big_endian;

  if (config->buf_header < 0)
	config->buf_header = 0;

  config->play_buf_size = port->p.buf_size;
  config->rec_buf_size = port->c.buf_size;

  if (force || config->sample_rate != port->config.sample_rate) {
	rate = config->sample_rate;
	dsp_write_n(port->card,
				SB_DSP_SetInputRate,
				2,
				(rate << 8) + (rate >> 8));
	dsp_write_n(port->card,
				SB_DSP_SetOutputRate,
				2,
				(rate << 8) + (rate >> 8));
	port->config.sample_rate = rate;
	port->card->sample_rate = rate;
  }

  port->config.buf_header = config->buf_header;

  if (force)
	port->config = *config;

  return B_NO_ERROR;
}


static void
init_channel (pcm_dma* dma)
{
  sb_card* sb = dma->port->card;
  int offset = (dma->capture_p ? MAX_DMA_BUF_SIZE : 0);

  dma->channel = (dma->capture_p ? sb->DMA_C : sb->DMA_P);
  dma->wide_p = (dma->channel >= 4);
  dma->mode = DMA_STOPPED;
  dma->index = 0;

  if (!dma->capture_p && sb->version >= 0x400)
	dma->wide_p = true;

  if (dma->wide_p) {
	dma->start = (dma->capture_p ? SB_DSP_Capture_16 : SB_DSP_Playback_16);
	dma->pause = SB_DSP_Pause_16;
	dma->cont = SB_DSP_Continue_16;
	dma->exit = SB_DSP_Exit_16;
	dma->buf_size = MAX_DMA_BUF_SIZE/2;
  }
  else {
	dma->start = (dma->capture_p ? SB_DSP_Capture_8 : SB_DSP_Playback_8);
	dma->pause = SB_DSP_Pause_8;
	dma->cont = SB_DSP_Continue_8;
	dma->exit = SB_DSP_Exit_8;
	/* use only half as much buffer when capturing in 8-bit */
	dma->buf_size = MAX_DMA_BUF_SIZE/4;
  }

  dma->buf_addr = dma->port->card->dma_addr + offset;
  dma->buf_phys_addr = (void*)((char*) dma->port->card->dma_phys_addr + offset);
}

static status_t
init_dma (pcm_dev* port)
{
  sb_card* sb = port->card;
  status_t err = B_NO_ERROR;;

  port->c.port = port;
  port->p.port = port;
  port->c.capture_p = true;
  port->p.capture_p = false;
  port->c.sem = B_BAD_SEM_ID;
  port->p.sem = B_BAD_SEM_ID;
  port->c.mode_lock = 0;
  port->p.mode_lock = 0;

  init_channel(&port->c);
  init_channel(&port->p);

  if (sb->DMA_C >= 0) {
	err = LOCK_ISA_DMA_CHANNEL(sb->DMA_C);
	if (err != B_NO_ERROR)
	  return err;
  }
  if (sb->DMA_P != sb->DMA_C && sb->DMA_P >= 0) {
	err = LOCK_ISA_DMA_CHANNEL(sb->DMA_P);
	if (err != B_NO_ERROR)
	  UNLOCK_ISA_DMA_CHANNEL(sb->DMA_C);
  }

  return err;
}

static void
uninit_dma (pcm_dev* port)
{
  sb_card* sb = port->card;
  stop_dma(&port->c);
  stop_dma(&port->p);

  if (sb->DMA_C >= 0)
	UNLOCK_ISA_DMA_CHANNEL(sb->DMA_C);
  if (sb->DMA_P != sb->DMA_C && sb->DMA_P >= 0)
	UNLOCK_ISA_DMA_CHANNEL(sb->DMA_P);
}


static status_t
pcm_open (const char* name, uint32 flags, void** cookie)
{
  int i;
  status_t err;
  pcm_dev* port;

  dprintf(CHIP_NAME ": pcm_open()\n");

  *cookie = NULL;
  for (i = 0; i < num_cards; i++)
	if (!strcmp(name, cards[i].pcm.name))
	  break;
  if (i >= num_cards)
	return ENODEV;

  port = &cards[i].pcm;

  /* note: not completely thread safe but as safe as most other drivers */
  if (atomic_add (&port->open_count, 1) == 0) {
	port->card = &cards[i];

	err = find_low_memory(port->card);
	if (err != B_NO_ERROR)
	  goto fail;

	/* configuration */
	configure_pcm(port, &default_pcm, true);

	err = init_dma(port);
	if (err != B_NO_ERROR)
	  goto fail;

	port->read_lock = create_sem(1, "blaster read_lock");
	if (port->read_lock < 0) {
	  uninit_dma(port);
	  err = port->read_lock;
	  goto fail;
	}

	port->write_lock = create_sem(1, "blaster write_lock");
	if (port->write_lock < 0) {
	  uninit_dma(port);
	  delete_sem(port->read_lock);
	  err = port->write_lock;
	  goto fail;
	}

	set_sem_owner(port->read_lock, B_SYSTEM_TEAM);
	set_sem_owner(port->write_lock, B_SYSTEM_TEAM);

	/* interrupts */
	increment_interrupt_handler(port->card);
  }

  *cookie = port;
  return B_NO_ERROR;

fail:
  atomic_add (&port->open_count, -1);
  return err;
}


static status_t
pcm_close (void* cookie)
{
  pcm_dev* port = (pcm_dev*) cookie;
  sb_card* sb = port->card;

  if (atomic_add(&port->open_count, -1) == 1) {
	delete_sem(port->read_lock);
	delete_sem(port->write_lock);

	uninit_dma(port);
	decrement_interrupt_handler(port->card);
  }

  return B_NO_ERROR;
}

static status_t
pcm_free(void * cookie)
{
  return B_NO_ERROR;
}


static status_t
pcm_control(void* cookie, uint32 iop, void* data, size_t len)
{
  pcm_dev * port = (pcm_dev *)cookie;
  status_t err = B_NO_ERROR;
  static float rates[4] = { 48000.0, 24000.0, 16000.0, 8000.0 };

  ddprintf(CHIP_NAME ": pcm_control()\n");

  switch (iop) {
  case B_AUDIO_GET_AUDIO_FORMAT:
	memcpy(data, &port->config, sizeof(port->config));
	break;
  case B_AUDIO_GET_PREFERRED_SAMPLE_RATES:
	memcpy(data, rates, sizeof(rates));
	break;
  case B_AUDIO_SET_AUDIO_FORMAT:
	err = configure_pcm(port, (pcm_cfg*) data, false);
	break;
  default:
	dprintf(CHIP_NAME ": unknown code %d\n", iop);
	err = B_BAD_VALUE;
	break;
  }

  return err;
}


static void
copy_lendian (pcm_dev* port, void* dst, void* src, int bytes)
{
  if (port->config.big_endian) {
	int i;
	int shorts = bytes / 2;
	for (i = 0; i < shorts; i++)
	  ((short*) dst)[i] = B_LENDIAN_TO_HOST_INT16 (((short*) src)[i]);
  }
  else
	memcpy(dst, src, bytes);
}


static status_t
request_copy(pcm_dma* dma, int32 request, int32* to_copy)
{
  cpu_status ps;
  status_t err;
  pcm_dev* port = dma->port;
  sb_card* sb = port->card;
  int32 half_size = dma->buf_size / 2;

  *to_copy = 0;
  dma->index %= dma->buf_size;
  if ((dma->index % half_size) == 0)
	do {
	  err = acquire_sem_etc(dma->sem, 1, B_CAN_INTERRUPT | B_TIMEOUT,
							1000000);
	  if (err != B_NO_ERROR && err != B_TIMED_OUT)
		return err;

	  ps = lock_spinner(&dma->mode_lock);
	  if (dma->mode == DMA_PAUSED) {
		bigtime_t paused_at = dma->last_int;
		int64 skipped_samples;
		dsp_write_n(sb, dma->cont, 0, 0);
		dma->last_int = system_time();
		skipped_samples = ((dma->last_int - paused_at)
						   * sb->sample_rate / 1000000);
		dma->inth_count += skipped_samples;
		dma->rw_count += skipped_samples;
		dma->mode = DMA_RUNNING;
	  }
	  unlock_spinner(&dma->mode_lock, ps);

	} while (err == B_TIMED_OUT);

  *to_copy = half_size - (dma->index % half_size);
  if (request < *to_copy)
	*to_copy = request;

  return B_NO_ERROR;
}


static status_t
pcm_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
  pcm_dev* port = (pcm_dev*) cookie;
  pcm_dma* dma = &port->c;
  int hdrsize = port->config.buf_header;
  char* buf_start = (char*) buf + hdrsize;
  int32 count = *numBytes - hdrsize;
  int32 to_copy;
  status_t err;
  cpu_status ps;
  uint8* src;
  int i;

  //ddprintf (CHIP_NAME ": pcm_read: buf=%.8x count=%.8x\n", buf, count);

  *numBytes = 0;
  if (port->card->DMA_C < 0)
	return B_ERROR;

  err = acquire_sem_etc(port->read_lock, 1, B_CAN_INTERRUPT, 0);
  if (err != B_NO_ERROR)
	return err;

  if (dma->mode == DMA_STOPPED) {
	dma->sem = create_sem(0, "Blaster capture DMA");
	set_sem_owner(dma->sem, B_SYSTEM_TEAM);
	start_dma(dma);
  }

  while (count > 0) {
	err = request_copy(dma, (dma->wide_p ? count : count / 2), &to_copy);
	if (err != B_NO_ERROR)
	  break;

	src = dma->buf_addr + dma->index;
	if (dma->wide_p)
	  copy_lendian (port, buf_start, src, to_copy);
	else
	  for (i = 0; i < to_copy; i++)
		/* strangely, SB16 doc says data is unsigned */
		((short*) buf_start)[i] = ((int32) src[i]) << 8;

	dma->index += to_copy;
	if (!dma->wide_p)
	  to_copy = 2 * to_copy;
	buf_start += to_copy;
	count -= to_copy;
	*numBytes += to_copy;
  }

  release_sem(port->read_lock);
  return err;
}


static status_t 
pcm_write (void* cookie, off_t pos, const void* buf, size_t* numBytes)
{
  pcm_dev* port = (pcm_dev*) cookie;
  pcm_dma* dma = &port->p;
  int hdrsize = port->config.buf_header;
  char* buf_start = (char*) buf + hdrsize;
  int32 count = *numBytes - hdrsize;
  int32 to_copy;
  status_t err;
  cpu_status ps;

  //triangle(buf_start, count);
  //ddprintf (CHIP_NAME ": pcm_write: buf=%.8x count=%.8x\n", buf, count);

  *numBytes = 0;
  if (port->card->DMA_P < 0)
	return B_ERROR;

  err = acquire_sem_etc(port->write_lock, 1, B_CAN_INTERRUPT, 0);
  if (err != B_NO_ERROR)
	return err;

  if (dma->mode == DMA_STOPPED) {
	dma->sem = create_sem(2, "Blaster playback DMA");
	set_sem_owner(dma->sem, B_SYSTEM_TEAM);
  }

  while (count > 0) {
	err = request_copy(dma, count, &to_copy);
	if (err != B_NO_ERROR)
	  break;

	copy_lendian (port, dma->buf_addr + dma->index, buf_start, to_copy);
	dma->index += to_copy;
	buf_start += to_copy;
	count -= to_copy;
	*numBytes += to_copy;

	if (dma->mode == DMA_STOPPED)
	  start_dma(dma);
  }

  release_sem(port->write_lock);
  return err;
}


void
pcm_interrupt (sb_card* sb, pcm_dma* dma)
{
  int32 sem_count = 0;

  acquire_spinlock(&dma->mode_lock);
  dma->last_int = system_time();
  dma->inth_count += (dma->wide_p ? dma->buf_size / 8 : dma->buf_size / 4);

  get_sem_count(dma->sem, &sem_count);
  if (sem_count > (dma->capture_p ? 1 : 0)) {
	dsp_write_n(sb, dma->pause, 0, 0);
	if (dma->mode == DMA_RUNNING)
	  dma->mode = DMA_PAUSED;
  }

  release_spinlock(&dma->mode_lock);
  release_sem_etc(dma->sem, 1, B_DO_NOT_RESCHEDULE);
}
