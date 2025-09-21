
#include "private.h"

extern device_hooks pcm_hooks;

static status_t old_open(const char *name, uint32 flags, void **cookie);
static status_t old_close(void *cookie);
static status_t old_free(void *cookie);
static status_t old_control(void *cookie, uint32 op, void *data, size_t len);
static status_t old_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t old_write(void *cookie, off_t pos, const void *data, size_t *len);

static void set_params (old_dev*, sound_setup*);


device_hooks old_hooks = {
    &old_open,
    &old_close,
    &old_free,
    &old_control,
    &old_read,
    &old_write
};


static const sound_setup default_setup = {		/* default codec setup */
	{
	/* left channel setup */
	line,		/* adc input source */
	15,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	8,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	0,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	0,			/* non-zero mutes line mix */
	7,			/* 0..64 dac attenuation, in -1.5 dB steps */
	0},			/* non-zero mutes dac output */

	{
	/* right channel setup */
	line,		/* adc input source */
	15,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	8,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	0,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	0,			/* non-zero mutes line mix */
	7,			/* 0..64 dac attenuation, in -1.5 dB steps */
	0},			/* non-zero mutes dac output */

	/* general setup */
	kHz_44_1,							/* 44.1KHz */
	linear_16bit_little_endian_stereo,	/* playback format */
	linear_16bit_little_endian_stereo,	/* capture format */
	0,			/* non-zero enables dither on 16 => 8 bit */
	0,			/* 0..64 dac to adc loopback attenuation, in -1.5 dB steps */
	0,			/* non-zero enables loopback */
	1,			/* zero (2.0 Vpp) non-zero (2.8 Vpp) output level boost */
	1,			/* non-zero enables highpass filter in adc */
	32,			/* set mono speaker volume to 1/2 scale */
	0			/* non-zero mutes speaker */
};


/* ----------
	init_params sets up the codec parameters and makes the codec data
	structure current.
---- */

static void
init_params (old_dev* port)
{
	sound_setup codec_copy = port->setup;
	char *c;

	/* force initial setup by copying initial parameters, then changing
	   every byte of the saved structure, then set back to the copy */

	for (c = (char*) &port->setup; c < (char*) (&port->setup+1); c++)
	  *c = ~*c;
	set_params (port, &codec_copy);
}

/* ----------
	get_params fills in the client record with the current codec parameters
----- */

static void
get_params (old_dev* port, sound_setup *s)
{
	memcpy (s, &port->setup, sizeof (sound_setup));
}


/* ----------
	set_params updates the codec parameters that have changed from the
	current ones.
----- */

static void
set_params (old_dev* port, sound_setup* s)
{
  int i;
  struct channel *sc;
  struct channel *cc;
  sb_card* sb = port->card;

  /* to save code space, loop for left & right channel */

  for (i = 1, sc = &s->right, cc = &port->setup.right;
	   i >= 0;
	   i--, sc = &s->left, cc = &port->setup.left)
	{
	  if (s->loop_attn != port->setup.loop_attn ||
		  !s->loop_enable != !port->setup.loop_enable ||
		  !sc->mic_gain_enable != !cc->mic_gain_enable)
		x_change(sb, 2 + i, 0x00,				/* mic volume (boost disabled) */
				 ((s->loop_attn >> 1) & 0x1f) +
				 (s->loop_enable ? 0 : 0xc0));

	  if (sc->adc_gain != cc->adc_gain)
		x_change(sb, 4 + i, (i ? 0 : 0x90),
				 0x60 & ((15 - sc->adc_gain) << 3));

	  if (sc->aux1_mix_gain != cc->aux1_mix_gain ||
		  !sc->aux1_mix_mute != !cc->aux1_mix_mute)
		ichange (sb, CS_left_aux2_in_control + i, 0x00, 
				 (sc->aux1_mix_gain & CS_iaux_attn) +
				 (sc->aux1_mix_mute ? CS_iaux_mute : 0));

	  if (sc->aux2_mix_gain != cc->aux2_mix_gain ||
		  !sc->aux2_mix_mute != !cc->aux2_mix_mute)
		ichange (sb, CS_left_line_in_control + i, 0x00, 
				 (sc->aux2_mix_gain & CS_iaux_attn) +
				 (sc->aux2_mix_mute ? CS_iaux_mute : 0));

	  if (sc->line_mix_gain != cc->line_mix_gain ||
		  !sc->line_mix_mute != !cc->line_mix_mute)
		ichange (sb, CS_left_aux1_in_control + i, 0x00, 
				 (sc->line_mix_gain & CS_line_in_mix_gain) +
				 (sc->line_mix_mute ? CS_line_in_mix_mute : 0));

	  if (sc->dac_attn != cc->dac_attn ||
		  !sc->dac_mute != !cc->dac_mute)
		ichange (sb, CS_left_dac_control + i, 0x00, 
				 (sc->dac_attn & CS_dac_attn) +
				 (sc->dac_mute ? CS_dac_mute : 0));
	}

	/* now check remaining, non left-or-right paramters */

	if (s->sample_rate != port->setup.sample_rate
		|| s->playback_format != port->setup.playback_format
		|| s->capture_format != port->setup.capture_format) {

	  /* !!! TO DO: ENSURE NO SOUND CURRENTLY PLAYING !!! */

#if B_HOST_IS_BIG_ENDIAN	// force format for now
	  s->capture_format = s->playback_format = linear_16bit_big_endian_stereo;
#else
	  s->capture_format = s->playback_format = linear_16bit_little_endian_stereo;
#endif

	  /* change capture first, as it does not cause resync */
	  ichange(sb, CS_capture_data_format | CS_index_mce, 0x0F,
			  s->capture_format<<4);

	  /* change playback, sample rate. Causes resync (+ autocal if enabled) */
	  ichange(sb, CS_clock_play_data_format | CS_index_mce, 0, 
			  s->sample_rate + (s->playback_format<<4));

	  complete_mode_change (sb);	/* complete mode change, wait for autocal */
	}

	/* finally, update our saved copy of the parameters */

	memcpy (&port->setup, s, sizeof (sound_setup));
}


static status_t
old_open (const char* name, uint32 flags, void** cookie)
{
  int i;
  status_t err;
  old_dev* port;

  ddprintf(CHIP_NAME ": old_open()\n");

  *cookie = NULL;
  for (i = 0; i < num_cards; i++)
	if (!strcmp(name, cards[i].old.name))
	  break;
  if (i >= num_cards)
	return ENODEV;

  port = &cards[i].old;

  /* note: not completely thread safe but as safe as most other drivers */
  if (atomic_add (&port->open_count, 1) == 0) {

	err = pcm_hooks.open(cards[i].pcm.name, flags, cookie);
	if (err != B_NO_ERROR)
	  goto fail;

	port->card = &cards[i];
	port->pcm = (pcm_dev*) *cookie;
	port->p_completion_sem = B_BAD_SEM_ID;
	port->c_completion_sem = B_BAD_SEM_ID;
	memcpy (&port->setup, &default_setup, sizeof(default_setup));

	init_params(port);
  }

  *cookie = port;
  return B_NO_ERROR;

fail:
  atomic_add (&port->open_count, -1);
  *cookie = NULL;
  return err;
}


static status_t
old_close (void* cookie)
{
  old_dev* port = (old_dev*) cookie;

  if (atomic_add(&port->open_count, -1) == 1)
	pcm_hooks.close(port->pcm);

  return B_NO_ERROR;
}

static status_t
old_free(void * cookie)
{
  return B_NO_ERROR;
}


static status_t
old_control(void* cookie, uint32 msg, void* buf, size_t len)
{
  old_dev* port = (old_dev*) cookie;
  status_t err = B_NO_ERROR;
  size_t size;
  int i;

  switch (msg) {

  case SOUND_GET_PARAMS:
	get_params (port, (sound_setup*) buf);
	ddprintf("SOUND_GET_PARAMS\n");
	break;

  case SOUND_SET_PARAMS:
	set_params (port, (sound_setup*) buf);
	ddprintf("SOUND_SET_PARAMS\n");
/*
for (i = port->card->wss_base - 4; i < port->card->wss_base + 4; i++)
  dprintf("%x = %02x\n", i, READ_IO_8(i));
for (i = 0; i < 32; i++)
  dprintf("R%02d  %02x\n", i, iread(port->card, i));
*/
	break;

  case SOUND_DEBUG_ON:
	break;

  case SOUND_DEBUG_OFF:
	break;

  case SOUND_GET_PLAYBACK_TIMESTAMP:
	break;

  case SOUND_GET_CAPTURE_TIMESTAMP:
	break;

  case SOUND_SET_PLAYBACK_COMPLETION_SEM:
	port->p_completion_sem = *(sem_id *)buf;
	if (port->p_completion_sem < 0) err = B_ERROR;
	ddprintf("SOUND_SET_PLAYBACK_COMPLETION_SEM\n");
	break;

  case SOUND_SET_CAPTURE_COMPLETION_SEM:
	port->c_completion_sem = *(sem_id *)buf;
	if (port->c_completion_sem < 0) err = B_ERROR;
	ddprintf("SOUND_SET_CAPTURE_COMPLETION_SEM\n");
	break;

  case SOUND_UNSAFE_WRITE:
	size = ((audio_buffer_header*)buf)->reserved_1;
	err = old_write(cookie, 0, buf, &size);
	break;

  case SOUND_UNSAFE_READ:
	size = ((audio_buffer_header*)buf)->reserved_1;
	err = old_read(cookie, 0, buf, &size);
	break;

  case SOUND_LOCK_FOR_DMA:
	break;

  case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
	size = 2 * (size_t) buf;
	if (size > MAX_DMA_BUF_SIZE)
	  size = MAX_DMA_BUF_SIZE;
	if (size < MIN_DMA_BUF_SIZE)
	  size = MIN_DMA_BUF_SIZE;
	port->card->pcm.c.buf_size = size;
	ddprintf("SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE %d\n", size/2);
	break;

  case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
	size = 2 * (size_t) buf;
	if (size > MAX_DMA_BUF_SIZE)
	  size = MAX_DMA_BUF_SIZE;
	if (size < MIN_DMA_BUF_SIZE)
	  size = MIN_DMA_BUF_SIZE;
	port->card->pcm.p.buf_size = size;
	ddprintf("SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE %d\n", size/2);
	break;

  case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
	*(int32*) buf = port->card->pcm.c.buf_size / 2;
	break;

  case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
	*(int32*) buf = port->card->pcm.p.buf_size / 2;
	break;

  default:
	err = B_ERROR;
	ddprintf("SOUND unknown ioctl\n");
  }
  if (err)
	ddprintf(" => %x (=%d)\n", err, err);

  return err;
}


static void
guess_buffer_time(pcm_dma* dma, audio_buffer_header* h, int32 bytes)
{
  cpu_status ps = lock_spinner(&dma->port->mode_lock);
  h->time = dma->last_int + (dma->rw_count - dma->inth_count) * 10000 / 441;
  h->sample_clock = dma->rw_count * 10000 / 441;
  dma->rw_count += bytes / 4;
  unlock_spinner(&dma->port->mode_lock, ps);
}


static status_t
old_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
  old_dev* port = (old_dev*) cookie;
  status_t err;

  *numBytes -= BUFTIME_SIZE;
  err = pcm_hooks.read(port->pcm, pos, (char*) buf + BUFTIME_SIZE, numBytes);
  if (err == B_NO_ERROR) {
	guess_buffer_time(&port->pcm->c, (audio_buffer_header*) buf, *numBytes);
	release_sem(port->c_completion_sem);
  }
  *numBytes += BUFTIME_SIZE;

  return err;
}


static status_t 
old_write (void* cookie, off_t pos, const void* buf, size_t* numBytes)
{
  old_dev* port = (old_dev*) cookie;
  status_t err;

  *numBytes -= BUFTIME_SIZE;
  err = pcm_hooks.write(port->pcm, pos, (char*) buf + BUFTIME_SIZE, numBytes);
  if (err == B_NO_ERROR) {
	guess_buffer_time(&port->pcm->p, (audio_buffer_header*) buf, *numBytes);
	release_sem(port->p_completion_sem);
  }
  *numBytes += BUFTIME_SIZE;

  return err;
}
