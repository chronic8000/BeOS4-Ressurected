
#include "ymf_private.h"

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
    &pcm_write,
	NULL,
	NULL
};

#define HALF_DUPLEX(SB) (!WSS_MODE(SB))

static pcm_cfg default_pcm = {
	44100.0,	/* sample rate */
	2,			/* channels */
	0x2,		/* format */		
#if B_HOST_IS_BENDIAN
	1,			/* endian (big) */
#else
	0,			/* endian (little) */
#endif
	0,			/* header size */
	MAX_DMA_BUF_SIZE/2,
	MAX_DMA_BUF_SIZE/2
};


static status_t
start_dma (pcm_dma* dma)
{
  sb_card* sb = dma->port->card;
  int count = dma->buf_size / 8 - 1;
  ddprintf ("start_dma: phys addr = %.8x count=%.8x\n",
			dma->buf_phys_addr, count);

  dma->mode = DMA_RUNNING;
  dma->inth_count = 0;
  dma->rw_count = 0;

  if (dma->capture_p) {
	START_ISA_DMA (sb->DMA_C,
				   dma->buf_phys_addr,
				   dma->buf_size,
				   0x14,
				   0x30
				   );
	ichange (sb, CS_capture_lower_base, 0x00, count & 0xFF);
	ichange (sb, CS_capture_upper_base, 0x00, count >> 8);
	ichange (sb, CS_interface_config, ~CS_config_cen, CS_config_cen);
  }
  else {
	START_ISA_DMA (sb->DMA_P,
				   dma->buf_phys_addr,
				   dma->buf_size,
				   0x18,
				   0x30
				   );
	ichange (sb, CS_play_lower_base, 0x00, count & 0xFF);
	ichange (sb, CS_play_upper_base, 0x00, count >> 8);
	ichange (sb, CS_interface_config, ~CS_config_pen, CS_config_pen);
  }

  dma->last_int = system_time();

  return B_NO_ERROR;
}
	

static void
stop_dma (pcm_dma* dma)
{
  sb_card* sb = dma->port->card;

  /* disable capture & playback */
  ichange (sb, CS_interface_config, ~(CS_config_pen + CS_config_cen), 0);

  dma->mode = DMA_STOPPED;
  dma->index = 0;
}
	

static status_t 
configure_pcm(pcm_dev* port, pcm_cfg* new_config, bool force)
{
  pcm_cfg config;
  uint16 rate;

  ddprintf("sblaster: configure_pcm()\n");

#if 0
  memcpy(&config, new_config, sizeof(config));

  /* check args */
  if (config.sample_rate < 4000.0)
	config.sample_rate = 4000.0;
  if (config.sample_rate > 48000.0)
	config.sample_rate = 48000.0;

  if (force || config.sample_rate != port->config.sample_rate) {
	rate = config.sample_rate;
	dsp_write_n(port->card,
				SB_DSP_SetInputRate,
				2,
				(rate << 8) + (rate >> 8));
	dsp_write_n(port->card,
				SB_DSP_SetOutputRate,
				2,
				(rate << 8) + (rate >> 8));
	port->config.sample_rate = config.sample_rate;
  }

  // until we support other changes
  if (force)
	port->config = config;
#endif

  return B_NO_ERROR;
}


static status_t
init_channel (pcm_dma* dma)
{
  sb_card* sb = dma->port->card;
  int offset = (dma->capture_p ? MAX_DMA_BUF_SIZE : 0);

  dma->mode = DMA_STOPPED;
  dma->index = 0;

  dma->buf_size = MAX_DMA_BUF_SIZE/2;
  dma->buf_addr = dma->port->card->dma_addr + offset;
  dma->buf_phys_addr = (void*)((char*) dma->port->card->dma_phys_addr + offset);

  dma->sem = create_sem(dma->capture_p ? 0 : 2, CHIP_NAME " dma_sem");
  if (dma->sem < 0)
	return dma->sem;
  set_sem_owner(dma->sem, B_SYSTEM_TEAM);

  return B_NO_ERROR;
}

static status_t
init_dma (pcm_dev* port)
{
  sb_card* sb = port->card;
  status_t err;

  port->c.port = port;
  port->p.port = port;
  port->c.capture_p = true;
  port->p.capture_p = false;
  port->c.sem = B_BAD_SEM_ID;
  port->p.sem = B_BAD_SEM_ID;
  port->mode_lock = 0;

  err = init_channel(&port->c);
  if (err != B_NO_ERROR)
	return err;

  err = init_channel(&port->p);
  if (err != B_NO_ERROR) {
	delete_sem(port->c.sem);
	return err;
  }

  if (sb->DMA_C >= 0) {
	err = LOCK_ISA_DMA_CHANNEL(sb->DMA_C);
	if (err == B_NO_ERROR) {
	  if (sb->DMA_P != sb->DMA_C && sb->DMA_P >= 0)
		err = LOCK_ISA_DMA_CHANNEL(sb->DMA_P);
	  if (err != B_NO_ERROR)
		UNLOCK_ISA_DMA_CHANNEL(sb->DMA_C);
	}
  }

  if (err != B_NO_ERROR) {
	delete_sem(port->c.sem);
	delete_sem(port->p.sem);
  }

  return err;
}

static void
uninit_dma (pcm_dev* port)
{
  sb_card* sb = port->card;
  stop_dma(&port->c);
  stop_dma(&port->p);

  delete_sem(port->c.sem);
  delete_sem(port->p.sem);

  if (sb->DMA_C >= 0) {
	UNLOCK_ISA_DMA_CHANNEL(sb->DMA_C);
	if (sb->DMA_P != sb->DMA_C && sb->DMA_P >= 0)
	  UNLOCK_ISA_DMA_CHANNEL(sb->DMA_P);
  }
}


static status_t
pcm_open (const char* name, uint32 flags, void** cookie)
{
  int i;
  status_t err;
  pcm_dev* port;

  ddprintf("sblaster: pcm_open()\n");

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

	/* enable interrupts */
	ichange (port->card, CS_pin_control, ~CS_pin_ien, CS_pin_ien);
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

	/* disable interrupts */
	ichange (sb, CS_pin_control, ~CS_pin_ien, 0);

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

  ddprintf("sonic_vibes: pcm_control()\n");

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
	dprintf("sonic_vibes: unknown code %d\n", iop);
	err = B_BAD_VALUE;
	break;
  }

  return err;
}


static void
copy_lendian (void* dst, void* src, int bytes)
{
#if B_HOST_IS_BENDIAN

  int shorts = bytes / 2;
  int i;

  for (i = 0; i < shorts; i++)
	((short*) dst)[i] = read_16_swap((short*) src + i);

#else

  memcpy(dst, src, bytes);

#endif
}


static status_t
request_copy(pcm_dma* dma, int32 request, int32* to_copy)
{
  cpu_status ps;
  status_t err;
  pcm_dev* port = dma->port;
  sb_card* sb = port->card;
  int32 semibuf_size = dma->buf_size / 2;

  *to_copy = 0;
  dma->index %= dma->buf_size;
  if ((dma->index % semibuf_size) == 0)
	do {
	  err = acquire_sem_etc(dma->sem, 1, B_CAN_INTERRUPT | B_TIMEOUT,
							1000000);
	  if (err != B_NO_ERROR && err != B_TIMED_OUT)
		return err;

	  ps = lock_spinner(&port->mode_lock);
	  if (dma->mode == DMA_PAUSED) {
		bigtime_t paused_at = dma->last_int;
		int64 skipped_samples;
		if (dma->capture_p)
		  ichange (sb, CS_interface_config, ~CS_config_cen, CS_config_cen);
		else
		  ichange (sb, CS_interface_config, ~CS_config_pen, CS_config_pen);
		dma->last_int = system_time();
		skipped_samples = (dma->last_int - paused_at) * 441 / 10000;
		dma->inth_count += skipped_samples;
		dma->rw_count += skipped_samples;
		dma->mode = DMA_RUNNING;
	  }
	  unlock_spinner(&port->mode_lock, ps);

	} while (err == B_TIMED_OUT);

  *to_copy = semibuf_size - (dma->index % semibuf_size);
  if (request < *to_copy)
	*to_copy = request;

  return B_NO_ERROR;
}


static status_t
pcm_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
  char*		buf_start = (char*) buf;
  int32		count = *numBytes;
  int32		to_copy;
  status_t	err;
  pcm_dev*	port = (pcm_dev*) cookie;
  pcm_dma*	dma = &port->c;

  //ddprintf ("sblaster: pcm_read: buf=%.8x count=%.8x\n", buf, count);

  *numBytes = 0;
  err = acquire_sem_etc(port->read_lock, 1, B_CAN_INTERRUPT, 0);
  if (err != B_NO_ERROR)
	return err;

  if (dma->mode == DMA_STOPPED)
	start_dma(dma);

  while (count > 0) {
	err = request_copy(dma, count, &to_copy);
	if (err != B_NO_ERROR)
	  break;

	copy_lendian (buf_start, dma->buf_addr + dma->index, to_copy);
	dma->index += to_copy;
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
  char*		buf_start = (char*) buf;
  int32		count = *numBytes;
  int32		to_copy;
  status_t	err;
  pcm_dev*	port = (pcm_dev*) cookie;
  pcm_dma*	dma = &port->p;
  cpu_status ps;

  //triangle(buf_start, count);

  //ddprintf ("sblaster: pcm_write: buf=%.8x count=%.8x\n", buf, count);

  *numBytes = 0;
  err = acquire_sem_etc(port->write_lock, 1, B_CAN_INTERRUPT, 0);
  if (err != B_NO_ERROR)
	return err;

  while (count > 0) {
	err = request_copy(dma, count, &to_copy);
	if (err != B_NO_ERROR)
	  break;

	copy_lendian (dma->buf_addr + dma->index, buf_start, to_copy);
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


int32
pcm_interrupt (pcm_dma* dma)
{
  pcm_dev* port = dma->port;
  int32 sem_count = 0;
  cpu_status ps = lock_spinner(&port->mode_lock);

  dma->last_int = system_time();
  dma->inth_count += dma->buf_size / 8;

  get_sem_count(dma->sem, &sem_count);
  if (sem_count > (dma->capture_p ? 1 : 0)) {
	ichange (port->card, CS_interface_config,
			 (dma->capture_p ? ~CS_config_cen : ~CS_config_pen),
			 0);
	if (dma->mode == DMA_RUNNING)
	  dma->mode = DMA_PAUSED;
  }

  unlock_spinner(&port->mode_lock, ps);
  release_sem_etc(dma->sem, 1, B_DO_NOT_RESCHEDULE);
  return B_INVOKE_SCHEDULER;
}
