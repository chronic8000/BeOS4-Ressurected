/************************************************************************/
/* copyright 2000 Be, Incorporated. All rights reserved.                */
/************************************************************************/

#include "speaker.h"

#define ddprintf //dprintf

#define BUFTIME_SIZE sizeof(audio_buffer_header)

status_t old_read(void *cookie, off_t pos, void *buf, size_t *count);
status_t old_write(void *cookie, off_t pos, const void *buf, size_t *count);

static void set_params (old_dev*, sound_setup*);

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

void
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
  spkr_dev* p = port->pdev;

  if (s->left.dac_attn != port->setup.left.dac_attn
	  ||s->right.dac_attn != port->setup.right.dac_attn)
	vc_set(p, (63-s->left.dac_attn)/63.0, (63-s->right.dac_attn)/63.0);

  if (s->left.dac_mute != port->setup.left.dac_mute)
	vc_mute(p, s->left.dac_mute);

  memcpy (&port->setup, s, sizeof (sound_setup));
  /* force 44.1k for now */
  port->setup.sample_rate = kHz_44_1;
}


status_t
old_open (const char* name, uint32 flags, void** cookie)
{
  status_t err;
  old_dev* port = (old_dev*) malloc(sizeof(old_dev));

  ddprintf(ID "old_open()\n");
  if (port == NULL)
	return ENOMEM;

  err = pcm_open(name, flags, cookie);
  if (err != B_OK) {
	free(port);
	return err;
  }

  port->pdev = *cookie;
  port->p.completion_sem = B_BAD_SEM_ID;
  port->p.count = 0;
  memcpy (&port->setup, &default_setup, sizeof(default_setup));
  init_params(port);

  *cookie = port;
  return B_NO_ERROR;
}


status_t
old_close (void* cookie)
{
  ddprintf(ID "old_close()\n");
  return pcm_close(((old_dev*) cookie)->pdev);
}

status_t
old_free(void* cookie)
{
  ddprintf(ID "old_free()\n");
  pcm_free(((old_dev*) cookie)->pdev);
  free(cookie);
  return B_OK;
}


status_t
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
	port->pdev->lego_api = port;
	port->p.completion_sem = *(sem_id *)buf;
	if (port->p.completion_sem < 0) err = B_ERROR;
	ddprintf("SOUND_SET_PLAYBACK_COMPLETION_SEM\n");
	break;

  case SOUND_SET_CAPTURE_COMPLETION_SEM:
	/*
	port->c.completion_sem = *(sem_id *)buf;
	if (port->c.completion_sem < 0) err = B_ERROR;
	ddprintf("SOUND_SET_CAPTURE_COMPLETION_SEM\n");
	*/
	break;

  case SOUND_UNSAFE_WRITE:
	size = ((audio_buffer_header*)buf)->reserved_1;
	err = old_write(cookie, 0, buf, &size);
	break;

  case SOUND_UNSAFE_READ:
	err = B_ERROR;
	break;

  case SOUND_LOCK_FOR_DMA:
	break;

  case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
	size = 2 * (size_t) buf;
	/*
	if (size >= 32 && size <= MAX_DMA_BUF_SIZE)
	  port->card->pcm.c.buf_size = size;
	else
	  err = B_BAD_VALUE;
	*/
	ddprintf("SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE %d\n", size/2);
	break;

  case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
	size = 2 * (size_t) buf;
	/*
	if (size >= 32 && size <= MAX_DMA_BUF_SIZE)
	  port->card->pcm.p.buf_size = size;
	else
	  err = B_BAD_VALUE;
	*/
	ddprintf("SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE %d\n", size/2);
	break;

  case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
	//	*(int32*) buf = port->card->pcm.c.buf_size / 2;
	break;

  case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
	//	*(int32*) buf = port->card->pcm.p.buf_size / 2;
	break;

  default:
	err = B_ERROR;
	ddprintf("SOUND unknown ioctl\n");
  }
  if (err)
	ddprintf(" => %x (=%d)\n", err, err);

  return err;
}


status_t
old_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
  return ENODEV;
}


status_t 
old_write (void* cookie, off_t pos, const void* buf, size_t* numBytes)
{
  old_dev* port = (old_dev*) cookie;
  spkr_dev* dev = port->pdev;
  char* where = (char*) buf + BUFTIME_SIZE;
  status_t err;

  if (!dev->lego_api)
	return pcm_write(dev, pos, buf, numBytes);

  *numBytes -= BUFTIME_SIZE;
  err = pcm_write(dev, pos, where, numBytes);
  if (err == B_OK)
	release_sem(port->p.completion_sem);
  *numBytes += BUFTIME_SIZE;

  //dprintf("<%Ld>\n", (system_time()-t1)/1000%1000);
  return err;
}
