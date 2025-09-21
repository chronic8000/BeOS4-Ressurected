
#include "awe_private.h"

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
	0,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	0,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes line mix */
	0,			/* 0..64 dac attenuation, in -1.5 dB steps */
	1 },		/* non-zero mutes dac output */

	{
	/* right channel setup */
	line,		/* adc input source */
	0,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	0,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes line mix */
	0,			/* 0..64 dac attenuation, in -1.5 dB steps */
	1},			/* non-zero mutes dac output */

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
	bool mmute;
	unsigned short rate;
	struct channel *sc;
	struct channel *cc;
	sb_card* sb = port->card;

	static const unsigned short sample_rate_conv [] = {
	  8000, 5510, 16000, 11025, 27420, 18900, 32000, 22050,
	  0, 37800, 0, 44100, 48000, 33075, 9600, 6620	/* 48k may not work */
	};

	static const uchar source_conv[] = {
	  0x18, 0x06, 0x01, 0x00
	};

	/* to save code space, loop for left & right channel */

	for (i = 1, sc = &s->right, cc = &port->setup.right;
	     i >= 0;
	     i--, sc = &s->left, cc = &port->setup.left)
	{
	  if (sc->dac_attn != cc->dac_attn
		  || !sc->dac_mute != !cc->dac_mute)
		mixer_change(sb, 0x32 + i, 7, sc->dac_mute
					 ? 0 : ((~sc->dac_attn) & 0xfe) << 2);
	  if (sc->aux2_mix_gain != cc->aux2_mix_gain)
		mixer_change(sb, 0x34 + i, 7, (~sc->aux2_mix_gain) << 3);
	  if (sc->aux1_mix_gain != cc->aux1_mix_gain)
		mixer_change(sb, 0x36 + i, 7, (~sc->aux1_mix_gain) << 3);
	  if (sc->line_mix_gain != cc->line_mix_gain)
		mixer_change(sb, 0x38 + i, 7, (~sc->line_mix_gain) << 3);
	  if (sc->adc_source != cc->adc_source)
		mixer_change(sb, 0x3d + i, 0x80, source_conv[sc->adc_source]);
	  if (sc->adc_gain != cc->adc_gain)
		mixer_change(sb, 0x3f + i, 0x3f, (sc->adc_gain & 0xfc) << 4);
	}

	/* now check remaining, non left-or-right paramters */

	if (s->loop_attn != port->setup.loop_attn)		/* mic volume */
	  mixer_change(sb, 0x3a, 7, ((~s->loop_attn) & 0xfe) << 2);
	if (!sc->mic_gain_enable != !cc->mic_gain_enable)
	  mixer_change(sb, 0x43, 0xfe, !!sc->mic_gain_enable);

	if (!sc->aux1_mix_mute != !cc->aux1_mix_mute
		|| !sc->line_mix_mute != !cc->line_mix_mute
		|| !s->loop_enable != !port->setup.loop_enable)
	  mixer_change(sb, 0x3c, 0xe0,
				   (sc->aux1_mix_mute ? 0x19 : 0x1f)
				   & (sc->line_mix_mute ? 0x07 : 0x1f)
				   & (s->loop_enable ? 0x1f : 0x1e));

	if (s->mono_gain != port->setup.mono_gain
		|| !s->mono_mute != !port->setup.mono_mute)
	  mixer_change(sb, 0x3b, 0x3f, s->mono_mute
				   ? 0 : (s->mono_gain & 0xf0) << 2);

	if (s->sample_rate != port->setup.sample_rate) {
	  sb->sample_rate = sample_rate_conv[s->sample_rate];
	  if (sb->sample_rate == 0)
		sb->sample_rate = 44100;
	  dsp_write_n(sb,
				  SB_DSP_SetInputRate,
				  2,
				  (sb->sample_rate << 8) + (sb->sample_rate >> 8));
	  dsp_write_n(sb,
				  SB_DSP_SetOutputRate,
				  2,
				  (sb->sample_rate << 8) + (sb->sample_rate >> 8));
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
  sb_card* sb;

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

	sb = &cards[i];
	port->card = sb;
	port->pcm = (pcm_dev*) *cookie;
	port->p_completion_sem = B_BAD_SEM_ID;
	port->c_completion_sem = B_BAD_SEM_ID;
	memcpy (&port->setup, &default_setup, sizeof(default_setup));

	/* max output level by default */
	mixer_change(sb, 0x30, 0x07, 0xf8);
	mixer_change(sb, 0x31, 0x07, 0xf8);
	mixer_change(sb, 0x41, 0x3f, 0x40);  // output gain = +6db
	mixer_change(sb, 0x42, 0x3f, 0x40);

	init_params(port);
  }

  *cookie = port;
  return B_NO_ERROR;

fail:
  atomic_add (&port->open_count, -1);
  dprintf("old_open(): %x\n", err);
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
  size_t		size;
  pcm_dma*		dma;

  switch (msg) {

  case SOUND_GET_PARAMS:
	get_params (port, (sound_setup*) buf);
	ddprintf("SOUND_GET_PARAMS\n");
	break;

  case SOUND_SET_PARAMS:
	set_params (port, (sound_setup*) buf);
	ddprintf("SOUND_SET_PARAMS\n");
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
	size = (size_t) buf;
	dma = &port->card->pcm.c;
	if (dma->wide_p)
	  size *= 2;
	if (size > MAX_DMA_BUF_SIZE)
	  size = MAX_DMA_BUF_SIZE;
	if (size < MIN_DMA_BUF_SIZE)
	  size = MIN_DMA_BUF_SIZE;
	dma->buf_size = size;
	break;

  case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
	size = (size_t) buf;
	dma = &port->card->pcm.p;
	if (dma->wide_p)
	  size *= 2;
	if (size > MAX_DMA_BUF_SIZE)
	  size = MAX_DMA_BUF_SIZE;
	if (size < MIN_DMA_BUF_SIZE)
	  size = MIN_DMA_BUF_SIZE;
	dma->buf_size = size;
	break;

  case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
	dma = &port->card->pcm.c;
	size = dma->buf_size;
	*(int32*) buf = (dma->wide_p ? size / 2 : size);
	break;

  case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
	dma = &port->card->pcm.p;
	size = dma->buf_size;
	*(int32*) buf = (dma->wide_p ? size / 2 : size);
	break;

  default:
	err = B_ERROR;
	ddprintf("SOUND unknown ioctl\n");
  }
  if (err)
	ddprintf(" => %x\n", err);

  return err;
}


static void
guess_buffer_time(old_dev* port, pcm_dma* dma,
				  audio_buffer_header* h, int32 bytes)
{
  cpu_status ps = lock_spinner(&dma->mode_lock);
  h->time = (dma->last_int + (dma->rw_count - dma->inth_count)
			 * 1000000 / port->card->sample_rate);
  h->sample_clock = dma->rw_count * 1000000 / port->card->sample_rate;
  dma->rw_count += bytes / 4;
  unlock_spinner(&dma->mode_lock, ps);
}


static status_t
old_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
  old_dev* port = (old_dev*) cookie;
  status_t err;

  *numBytes -= BUFTIME_SIZE;
  err = pcm_hooks.read(port->pcm, pos, (char*) buf + BUFTIME_SIZE, numBytes);
  if (err == B_NO_ERROR) {
	guess_buffer_time(port, &port->pcm->c,
					  (audio_buffer_header*) buf, *numBytes);
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
	guess_buffer_time(port, &port->pcm->p,
					  (audio_buffer_header*) buf, *numBytes);
	release_sem(port->p_completion_sem);
  }
  *numBytes += BUFTIME_SIZE;

  return err;
}
