/* ++++++++++
	FILE:	awacs.c
	REVS:	$Revision: 1.10 $
	NAME:	marc
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <ByteOrder.h>
#include <sound.h>
#include <gcentral.h>
#include <dbdma.h>
#include <R3MediaDefs.h>

#undef ANALYZE
#include <analyser.h>

#define ddprintf if (debug_output) dprintf
//#define ddprintf if (TRUE) dprintf

#define BUFTIME_SIZE sizeof(audio_buffer_header)

/* -----
	GC defines
----- */

#define MAX_DBDMA_COUNT			0xf000

/* GC_AUDIO registers */

#define AUDIO_CONTROL			0x00
#define AUDIO_CODEC_CONTROL		0x10
#define AUDIO_CODEC_STATUS		0x20
#define AUDIO_CLIP_COUNT		0x30
#define AUDIO_BYTE_SWAP			0x40

/* CODEC_CONTROL bits */

#define AUDIO_NEW_ECMD			0x01000000

/* CODEC_STATUS bits */

#define AUDIO_VALID_DATA		0x00400000
#define AUDIO_PHONES_CONNECTED	0x00000008
#define AUDIO_MIC_CONNECTED		0x00000004

/* -----
	private defines
----- */

#define MAX_QUEUE		33				/* dma command buffer size */
#define INT_FREQ		(MAX_QUEUE/4)	/* minimum interrupt frequency */
#define SUBFRAME_SELECT 0x11

/* -----
	private types
----- */

typedef dbdma_command qentry;

/* -----
	private globals
----- */

static vchar	*mac_io;			/* -> mac i/o area */
static int		debug_output = 0;	/* flag: spewage on/off */
static long		initialized = 0;	/* driver initialized bit */

static spinlock	p_lock = 0;			/* spinlock for playback queue */
static sem_id	p_sem = -1;			/* playback queue access */
static int		p_write = 0;		/* next playback queue elem # to set up */
static int		p_read = 0;			/* next playback queue elem # to use */
static sem_id	p_completion_sem;	/* sem to release when buffer complete */
static qentry*	p_queue;			/* aligned array of MAX_QUEUE+1 qentries */
static int64	p_sample_count;
static bigtime_t p_last_int;

static spinlock	c_lock = 0;			/* spinlock for capture queue */
static sem_id	c_sem = -1;			/* capture queue access */
static int		c_write = 0;		/* next capture queue elem # to set up */
static int		c_read = 0;			/* next capture queue elem # to use */
static sem_id	c_completion_sem;	/* sem to release when buffer complete */
static qentry*	c_queue;			/* aligned array of MAX_QUEUE+1 qentries */
static int64	c_sample_count;
static bigtime_t c_last_int;

static spinlock	reg_lock = 0;		/* access lock for codec regs */

/* locked area for p_queue, c_queue, p_headers, and c_headers */
static area_id	queue_area;

static audio_buffer_header** p_headers; /* array of MAX_QUEUE pointers */
static audio_buffer_header** c_headers; /* array of MAX_QUEUE pointers */

static sound_setup codec = {		/* current codec setup */
	{
	/* left channel setup */
	mic,		/* adc input source */
	0,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	0,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes line mix */
	0,			/* 0..64 dac attenuation, in -1.5 dB steps */
	0},			/* non-zero mutes dac output */

	{
	/* right channel setup */
	mic,		/* adc input source */
	0,			/* 0..15 adc gain, in 1.5 dB steps */
	0,			/* non-zero enables 20 dB MIC input gain */
	0,			/* 0..32 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux1 mix */
	8,			/* 0..32 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes aux2 mix */
	8,			/* 0..32 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	1,			/* non-zero mutes line mix */
	0,			/* 0..64 dac attenuation, in -1.5 dB steps */
	0},			/* non-zero mutes dac output */

	/* general setup */
	kHz_44_1,	/* 44.1KHz */
	linear_16bit_big_endian_stereo,	/* playback format */
	linear_16bit_big_endian_stereo,	/* capture format */
	0,			/* non-zero enables dither on 16 => 8 bit */
	0,			/* 0..64 dac to adc loopback attenuation, in -1.5 dB steps */
	0,			/* non-zero enables loopback */
	0,			/* zero (2.0 Vpp) non-zero (2.8 Vpp) output level boost */
	0,			/* non-zero enables highpass filter in adc */
	32,			/* set mono speaker volume to 1/2 scale */
	0			/* non-zero mutes speaker */
};


/* ----------
	find_mac_io - finds the area mapping the mac i/o chip
----- */
static bool
find_mac_io (void)
{
	area_info	a;
	area_id		id;

	id = find_area ("mac_io");
	if (id < 0)
		return FALSE;
	if (get_area_info (id, &a) < 0)
		return FALSE;

	mac_io = (vchar *) a.address;
	
	return TRUE;
}
	

/* ----------
	init_hardware - ensure mac i/o area is present
----- */
status_t
init_hardware (void)
{
	return find_mac_io() ? B_NO_ERROR : B_ERROR;
}



/* ----------
	physical_address
----- */

static long
physical_address (void* vaddr)
{
  physical_entry pe;
  get_memory_map (vaddr, 1, &pe, 1);
  return (long) pe.address;
}


/* ----------
	read_gc_reg
----- */

static ulong
read_gc_reg (long addr)
{
  ulong x = B_LENDIAN_TO_HOST_INT32 (*(vlong*) (mac_io + addr));
  __eieio();
  //dprintf (" read_gc_reg:  0x%04x  0x%08x\n", addr, x);
  return x;
}


/* ----------
	write_gc_reg
----- */

static void
write_gc_reg (long addr, ulong data)
{
  *(vlong*) (mac_io + addr) = B_HOST_TO_LENDIAN_INT32 (data);
  __eieio();
  //dprintf (" write_gc_reg:  0x%04x  0x%08x\n", addr, data);
}


/* ----------
	lock_codec_regs
----- */

static cpu_status
lock_codec_regs (void)
{
  int        count = 0;
  cpu_status ps;

  while (count++ < 5000) {
	ps = disable_interrupts ();
	acquire_spinlock (&reg_lock);
	if (0 == (read_gc_reg (GC_AUDIO + AUDIO_CODEC_CONTROL)
			  & AUDIO_NEW_ECMD))
	  break;
	release_spinlock (&reg_lock);
	restore_interrupts (ps);
	snooze (100);
  }

  if (count >= 5000) {
	ps = disable_interrupts ();
	acquire_spinlock (&reg_lock);
  }

  return ps;
}


/* ----------
	unlock_codec_regs
----- */

static void
unlock_codec_regs (cpu_status ps)
{
  release_spinlock (&reg_lock);
  restore_interrupts (ps);
}


/* ----------
	set_awacs_reg sets an exapnded data mode control register.
----- */

static void
set_awacs_reg (short address, short data)
{
  cpu_status ps;
  address &= 0x0fff;
  data &= 0x0fff;

  ps = lock_codec_regs();
  write_gc_reg (GC_AUDIO + AUDIO_CODEC_CONTROL, (address << 12) | data);
  unlock_codec_regs (ps);
  //dprintf (" set_awacs_reg:  %d  0x%03x\n", (int)address, (int)data);
}


/* ----------
	lock_queue locks access to a queue's data structures.
----- */

static cpu_status
lock_queue (spinlock *lock)
{
	cpu_status ps;

	ps = disable_interrupts ();
	acquire_spinlock (lock);
	return ps;
}


/* ----------
	unlock_queue unlock access to a queue's data structures.
----- */

static void
unlock_queue (spinlock *lock, cpu_status ps)
{
	release_spinlock (lock);
	restore_interrupts (ps);
}


/* ----------
	p_inth
----- */

static int32
p_inth(void *data)
{
  bigtime_t now = system_time();
  cpu_status ps;
  int q_entries_freed = 0;
  int buffers_completed = 0;

  analyser (0x124, 0);
  /* clear interrupt */
  write_gc_reg (GC_INT_CLEAR, INT_TO_MASK (GC_INT_DMA_AUDIO_OUT));

  ps = lock_queue (&p_lock);
  if (p_read != p_write)
	while (B_LENDIAN_TO_HOST_INT32 (p_queue[p_read].xfer_status_and_res_count)
		   & (DBDMA_CS_ACTIVE << 16)) {
	  ++q_entries_freed;
	  if ((B_LENDIAN_TO_HOST_INT32 (p_queue[p_read].op_code_and_req_count) & 0xf0000000)
		  == DBDMA_OUTPUT_LAST)
		++buffers_completed;
	  p_queue[p_read].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (DBDMA_STOP);
	  p_read = (p_read + 1) % MAX_QUEUE;
	  __eieio();
	  if (p_read == p_write) {
		p_last_int = now;
		break;
	  }
	  if (p_headers[p_read])
		p_headers[p_read]->time = now;
	}
  unlock_queue (&p_lock, ps);
  
  if (q_entries_freed)
	release_sem_etc (p_sem, q_entries_freed, B_DO_NOT_RESCHEDULE);
  if (buffers_completed)
	release_sem_etc (p_completion_sem, buffers_completed, B_DO_NOT_RESCHEDULE);

  //dprintf(" p_inth:  bc = %d, ef = %d\n", buffers_completed, q_entries_freed);
  return (buffers_completed ? B_INVOKE_SCHEDULER : B_HANDLED_INTERRUPT);
}


/* ----------
	c_inth
----- */

static int32
c_inth(void *data)
{
  bigtime_t now = system_time();
  cpu_status ps;
  int q_entries_freed = 0;
  int buffers_completed = 0;

  analyser (0x128, 0);
  /* clear interrupt */
  write_gc_reg (GC_INT_CLEAR, INT_TO_MASK (GC_INT_DMA_AUDIO_IN));

  ps = lock_queue (&c_lock);
  if (c_read != c_write)
	while (B_LENDIAN_TO_HOST_INT32 (c_queue[c_read].xfer_status_and_res_count)
		   & (DBDMA_CS_ACTIVE << 16)) {
	  ++q_entries_freed;
	  if ((B_LENDIAN_TO_HOST_INT32 (c_queue[c_read].op_code_and_req_count) & 0xf0000000)
		  == DBDMA_INPUT_LAST)
		++buffers_completed;
	  c_queue[c_read].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (DBDMA_STOP);
	  c_read = (c_read + 1) % MAX_QUEUE;
	  __eieio();
	  if (c_read == c_write) {
		c_last_int = now;
		break;
	  }
	  if (c_headers[c_read])
		c_headers[c_read]->time = now;
	}
  unlock_queue (&c_lock, ps);

  if (q_entries_freed)
	release_sem_etc (c_sem, q_entries_freed, B_DO_NOT_RESCHEDULE);
  if (buffers_completed)
	release_sem_etc (c_completion_sem, buffers_completed, B_DO_NOT_RESCHEDULE);

  return (buffers_completed ? B_INVOKE_SCHEDULER : B_HANDLED_INTERRUPT);
}


/* ----------
	print_params prints out the current state of the codec structure
---- */

static void
print_params ()
{
  int i;
  struct channel *cc;

  for (i = 0, cc = &codec.left; i < 2; i++, cc = &codec.right) {
	if (cc == &codec.left) {
	  dprintf("==== Left Channel:");
	} else {
	  dprintf("==== Right Channel:");
	}
	dprintf("\n\
		adc_source = %x\n\
		adc_gain = %x\n\
		mic_gain_enable = %x\n\
		aux1_mix_gain = %x\n\
		aux1_mix_mute = %x\n\
		aux2_mix_gain = %x\n\
		aux2_mix_mute = %x\n\
		line_mix_gain = %x\n\
		line_mix_mute = %x\n\
		dac_attn = %x\n\
		dac_mute = %x\n",
		   cc->adc_source, cc->adc_gain,
		   cc->mic_gain_enable,
		   cc->aux1_mix_gain, cc->aux1_mix_mute,
		   cc->aux2_mix_gain, cc->aux2_mix_mute,
		   cc->line_mix_gain, cc->line_mix_mute,
		   cc->dac_attn, cc->dac_mute);
  }

  /* and print the common parameters */
  
  dprintf("==== Common Parameters:\n\
		sample_rate = %x\n\
		playback_format = %x\n\
		capture_format = %x\n\
		dither_enable = %x\n\
		loop_attn = %x\n\
		loop_enable = %x\n\
		output_boost = %x\n\
		highpass_enable = %x\n\
		mono_gain = %x\n\
		mono_mute = %x\n",
		  codec.sample_rate,
		  codec.playback_format,
		  codec.capture_format,
		  codec.dither_enable,
		  codec.loop_attn,
		  codec.loop_enable,
		  codec.output_boost,
		  codec.highpass_enable,
		  codec.mono_gain,
		  codec.mono_mute);
}


/* ----------
	get_params fills in the client record with the current codec parameters
----- */

static void
get_params (sound_setup *s)
{
	memcpy (s, &codec, sizeof (sound_setup));
}


/* ----------
	set_params updates the codec parameters that have changed from the
	current ones.
----- */

static void
set_params (sound_setup *s)
{
  short x;

  static const char sample_rate_conv [] = {
	7, 7, 4, 5, 1, 3, 1, 2, 2, 1, 2, 0, 0, 1, 6, 7
  };
  static const short source_conv [] = {
	0x800, 0x200, 0x400, 0x000
  };

  if (s->left.dac_attn != codec.left.dac_attn
	  || s->right.dac_attn != codec.right.dac_attn
	  || s->mono_gain != codec.mono_gain)
  {
	/* line out */
	x = ((short) s->left.dac_attn & 0x3c) << 4;
	x |= (s->right.dac_attn & 0x3c) >> 2;
	set_awacs_reg (2, x);

	/* speaker */
	x = (~(short) s->mono_gain & 0x3c) << 4;
	x |= (~(short) s->mono_gain & 0x3c) >> 2;
	set_awacs_reg (4, x);
  }

  if (s->left.adc_gain != codec.left.adc_gain
	  || s->right.adc_gain != codec.right.adc_gain
	  || s->left.adc_source != codec.left.adc_source
	  || !s->left.mic_gain_enable != !codec.left.mic_gain_enable)
	set_awacs_reg (0, ((s->left.adc_gain & 15) << 4)
				   + (s->right.adc_gain & 15)
				   + (s->left.mic_gain_enable ? 0x100 : 0)
				   + source_conv[s->left.adc_source & 3]);

  if (!s->left.dac_mute != !codec.left.dac_mute
	  || !s->mono_mute != !codec.mono_mute
	  || !s->loop_enable != !codec.loop_enable
	  || s->sample_rate != codec.sample_rate)
	set_awacs_reg (1, (s->left.dac_mute ? 0x200 : 0)
				   + (s->mono_mute ? 0x080 : 0)
				   + (s->loop_enable ? 0x040 : 0)
				   + (sample_rate_conv[s->sample_rate & 15] << 3));

  if (s->sample_rate != codec.sample_rate)
	write_gc_reg (GC_AUDIO + AUDIO_CONTROL,
				  (sample_rate_conv[s->sample_rate & 15] << 8)
				  + SUBFRAME_SELECT);

  /* finally, update our saved copy of the parameters */
  memcpy (&codec, s, sizeof (sound_setup));
}


/* ----------
	init_params sets up the codec parameters and makes the codec data
	structure current.
---- */

static void
init_params ()
{
	char			*c;
	sound_setup		codec_copy;

	/* force initial setup by copying initial parameters, then changing
	   every byte of the saved structure, then set back to the copy */

	memcpy (&codec_copy, &codec, sizeof (sound_setup));
	for (c = (char*) &codec; c < (char*) (&codec+1); c++)
		*c = ~*c;
	set_params (&codec_copy);
}


/* ----------
	sound_uninit does the actual driver shutdown.
----- */

static void
sound_uninit (void)
{
  /* disable capture & playback */

  write_gc_reg (GC_DMA_AUDIO_IN + DBDMA_CONTROL, DBDMA_CS_RUN << 16);
  write_gc_reg (GC_DMA_AUDIO_OUT + DBDMA_CONTROL, DBDMA_CS_RUN << 16);
 
  remove_io_interrupt_handler(GC_INT_DMA_AUDIO_IN, c_inth, NULL);
  remove_io_interrupt_handler(GC_INT_DMA_AUDIO_OUT, p_inth, NULL);

  set_awacs_reg (1, 0x240);	/* mute outputs */

  if (p_sem >= 0)
	delete_sem (p_sem);
  p_sem = -1;

  if (c_sem >= 0)
	delete_sem (c_sem);
  c_sem = -1;

  initialized = 0;
}
	

/* ----------
	sound_close shuts down the driver.
----- */

static status_t
sound_close (void *cookie)
{
  sound_uninit ();			/* do the real shutdown */
  return B_NO_ERROR;
}


/* ----------
	sound_free
----- */

static status_t
sound_free (void* cookie)
{
  return B_NO_ERROR;
}


/* ----------
	init_driver does the one-time-only codec hardware (and associated codec
	state data structure) initialization.  Subsequent opens of the driver
	do NOT call this.
----- */

status_t
init_driver(void)
{
  int i;
  status_t err;

  if (!find_mac_io())
		return B_ERROR;

  /* wait for chip to wake up */
  /* for as long as 1 second */
  for (i = 0; ; i++)
	if (read_gc_reg (GC_AUDIO + AUDIO_CODEC_STATUS) & AUDIO_VALID_DATA)
	  break;
	else if (i < 10000)
	  spin (100);
	else
	  return B_ERROR;

  write_gc_reg (GC_AUDIO + AUDIO_CONTROL, SUBFRAME_SELECT);
  //set_awacs_reg (1, 0x240);	/* mute outputs */

  /* 16-byte aligned arrays of MAX_QUEUE+1 qentries on a single page */
  queue_area = create_area("awacs driver",
						   &p_queue,
						   B_ANY_KERNEL_ADDRESS,
						   B_PAGE_SIZE,
						   B_FULL_LOCK,
						   B_READ_AREA + B_WRITE_AREA);
  if (queue_area < B_NO_ERROR)
	return queue_area;
  err = lock_memory(p_queue, B_PAGE_SIZE, B_DMA_IO);
  if (err < B_NO_ERROR)
	return err;
  c_queue = p_queue + MAX_QUEUE + 1;
  p_headers = (audio_buffer_header**) (c_queue + MAX_QUEUE + 1);
  c_headers = p_headers + MAX_QUEUE;

  ddprintf ("one_time_init complete\n");
  return B_NO_ERROR;
}


void
uninit_driver()
{
  delete_area(queue_area);
}


/* ----------
	sound_open sets up the driver.
----- */

static status_t
sound_open (const char *name, uint32 flags, void **cookie)
{
  int i;

  if (atomic_or (&initialized, 1) != 0) 	/* only 1 open at a time */
	return B_ERROR;

  /* stop */
  write_gc_reg (GC_DMA_AUDIO_IN + DBDMA_CONTROL, DBDMA_CS_RUN << 16);
  write_gc_reg (GC_DMA_AUDIO_OUT + DBDMA_CONTROL, DBDMA_CS_RUN << 16);

  for (i = 0; i < MAX_QUEUE; i++) {
	p_queue[i].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (DBDMA_STOP);
	c_queue[i].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (DBDMA_STOP);
  }
  p_queue[MAX_QUEUE].cmd_dep = B_HOST_TO_LENDIAN_INT32 (physical_address (p_queue));
  p_queue[MAX_QUEUE].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (
				 DBDMA_NOP + (3 << DBDMA_B_SHIFT));
  c_queue[MAX_QUEUE].cmd_dep = B_HOST_TO_LENDIAN_INT32 (physical_address (c_queue));
  c_queue[MAX_QUEUE].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (
				 DBDMA_NOP + (3 << DBDMA_B_SHIFT));

  p_write = 0;
  p_read = 0;
  p_sample_count = 0;
  p_last_int = 0;
  c_write = 0;
  c_read = 0;
  c_sample_count = 0;
  c_last_int = 0;

  if ((p_sem = create_sem (MAX_QUEUE-1, "sound_play_queue")) < 0)
	goto err;
  set_sem_owner(p_sem, B_SYSTEM_TEAM);
  if ((c_sem = create_sem (MAX_QUEUE-1, "sound_capture_queue")) < 0)
	goto err;
  set_sem_owner(c_sem, B_SYSTEM_TEAM);

  init_params ();							/* setup codec */

  install_io_interrupt_handler (GC_INT_DMA_AUDIO_IN, c_inth, NULL, 0);
  install_io_interrupt_handler (GC_INT_DMA_AUDIO_OUT, p_inth, NULL, 0);
  
  write_gc_reg (GC_DMA_AUDIO_IN + DBDMA_COMMAND_PTR,
				physical_address (c_queue));
  write_gc_reg (GC_DMA_AUDIO_OUT + DBDMA_COMMAND_PTR,
				physical_address (p_queue));
  /* start */
  write_gc_reg (GC_DMA_AUDIO_IN + DBDMA_CONTROL,
				(DBDMA_CS_RUN << 16) + DBDMA_CS_RUN);
  write_gc_reg (GC_DMA_AUDIO_OUT + DBDMA_CONTROL,
				(DBDMA_CS_RUN << 16) + DBDMA_CS_RUN);

  //dprintf (" sound_open: success\n");
  return B_NO_ERROR;

err:
  dprintf (" sound_open: failed\n");
  sound_uninit ();
  return B_ERROR;
}


/* ----------
	sound_read
----- */

static status_t
sound_read (void *cookie, off_t pos, void* buf, size_t *nbytes)
{
  bool done = false;
  bool first = true;
  long count = *nbytes - BUFTIME_SIZE;	/* # bytes to read */
  long size;							/* # bytes this pass */
  audio_buffer_header* header = (audio_buffer_header*) buf;
  char* dma_start = (char*) buf + BUFTIME_SIZE;
  status_t err;
  cpu_status ps;
  int i;
  physical_entry map[INT_FREQ];

  //dprintf (" sound_read:  0x%08x  %d\n", buf, count);
  if (!initialized)
	return B_ERROR;

  while (!done) {
	get_memory_map (dma_start, count, map, INT_FREQ);
	for (i = 0; !done && i < INT_FREQ; ++i) {
	  size = min (map[i].size, MAX_DBDMA_COUNT);
	  done = (count <= size);

	  err = acquire_sem(c_sem);
	  if (err < 0)
		return err;
	  ps = lock_queue (&c_lock);
	  c_queue[c_write].xfer_status_and_res_count = 0;
	  c_queue[c_write].address = B_HOST_TO_LENDIAN_INT32 ((long) map[i].address);
	  __eieio();
	  c_queue[c_write].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (
					 (done ? DBDMA_INPUT_LAST : DBDMA_INPUT_MORE)
					 | ((done || i == INT_FREQ - 1) ? (3 << DBDMA_I_SHIFT) : 0)
					 | size);
	  __eieio();
	  write_gc_reg (GC_DMA_AUDIO_IN + DBDMA_CONTROL,
					(DBDMA_CS_WAKE << 16) + DBDMA_CS_WAKE);
	  if (BUFTIME_SIZE && first) {
	  	header->time = system_time();
		if (c_read == c_write && c_last_int)
		  c_sample_count += (header->time - c_last_int) * 441 / 10000;
		header->sample_clock = c_sample_count * 10000 / 441;
		c_sample_count += count / 4;
		c_headers[c_write] = header;
	  }
	  else
	  	c_headers[c_write] = NULL;
	  c_write = (c_write + 1) % MAX_QUEUE;
	  unlock_queue (&c_lock, ps);

	  count -= size;
	  dma_start += size;
	  first = false;

	  if (size < map[i].size) {
		map[i].size -= size;
		map[i].address = (void*) ((long) map[i].address + size);
		--i;
	  }
	}
  }
  return B_NO_ERROR;
}


/* ----------
	sound_write
----- */

static status_t 
sound_write (void *cookie, off_t pos, const void* buf, size_t *nbytes)
{
  bool done = false;
  bool first = true;
  ulong count = *nbytes - BUFTIME_SIZE;	/* # bytes to write */
  ulong size;							/* # bytes this pass */
  audio_buffer_header* header = (audio_buffer_header*) buf;
  char* dma_start = (char*) buf + BUFTIME_SIZE;
  status_t err;
  cpu_status ps;
  int i;
  physical_entry map[INT_FREQ];

  //dprintf (" sound_write:  0x%08x  %d\n", buf, count);
  if (!initialized)
	return B_ERROR;

  while (!done) {
	get_memory_map (dma_start, count, map, INT_FREQ);
	for (i = 0; !done && i < INT_FREQ; ++i) {
	  size = min (map[i].size, MAX_DBDMA_COUNT);
	  done = (count <= size);

	  err = acquire_sem(p_sem);
	  if (err < 0)
		return err;
	  ps = lock_queue (&p_lock);
	  p_queue[p_write].xfer_status_and_res_count = 0;
	  p_queue[p_write].address = B_HOST_TO_LENDIAN_INT32 ((long) map[i].address);
	  __eieio();
	  p_queue[p_write].op_code_and_req_count = B_HOST_TO_LENDIAN_INT32 (
					 (done ? DBDMA_OUTPUT_LAST : DBDMA_OUTPUT_MORE)
					 | ((done || i == INT_FREQ - 1) ? (3 << DBDMA_I_SHIFT) : 0)
					 | size);
	  __eieio();
	  write_gc_reg (GC_DMA_AUDIO_OUT + DBDMA_CONTROL,
					(DBDMA_CS_WAKE << 16) + DBDMA_CS_WAKE);
	  if (BUFTIME_SIZE && first) {
	  	header->time = system_time();
		if (p_read == p_write && p_last_int)
		  p_sample_count += (header->time - p_last_int) * 441 / 10000;
		header->sample_clock = p_sample_count * 10000 / 441;
		p_sample_count += count / 4;
		p_headers[p_write] = header;
	  }
	  else
	  	p_headers[p_write] = NULL;
	  p_write = (p_write + 1) % MAX_QUEUE;
	  unlock_queue (&p_lock, ps);

	  count -= size;
	  dma_start += size;
	  first = false;

	  if (size < map[i].size) {
		map[i].size -= size;
		map[i].address = (void*) ((long) map[i].address + size);
		--i;
	  }
	}
  }
  return B_NO_ERROR;
}


/* ----------
	sound_control
----- */

static status_t
sound_control(void *cookie, uint32 msg, void *buf, size_t len)
{
	status_t	err = B_NO_ERROR;
	cpu_status	ps;
	size_t		size;
		
	err = B_NO_ERROR;

	switch (msg) {

	case SOUND_GET_PARAMS:
		get_params ( (sound_setup*) buf);
		ddprintf("SOUND_GET_PARAMS");
		break;

	case SOUND_SET_PARAMS:
		set_params ( (sound_setup*) buf);
		ddprintf("SOUND_SET_PARAMS");
		break;

	case SOUND_DEBUG_ON:
		debug_output = 1;
		ddprintf("SOUND_DEBUG_ON");
		break;

	case SOUND_DEBUG_OFF:
		ddprintf("SOUND_DEBUG_OFF");
		debug_output = 0;
		break;

#if 0
	case SOUND_PLAYING:
		ps = lock_queue (&p_lock);
		err = p_write - p_read;
		unlock_queue (&p_lock, ps);
		if (err < 0)
		  err = err + MAX_QUEUE;
		break;

	case SOUND_CAPTURING:
		ps = lock_queue (&c_lock);
		err = c_write - c_read;
		unlock_queue (&c_lock, ps);
		if (err < 0)
		  err = err + MAX_QUEUE;
		break;
#endif

	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		p_completion_sem = *(sem_id *)buf;
		if (p_completion_sem < 0)
		  err = B_ERROR;
		ddprintf("SOUND_SET_PLAYBACK_COMPLETION_SEM");
		break;

	case SOUND_SET_CAPTURE_COMPLETION_SEM:
		c_completion_sem = *(sem_id *)buf;
		if (c_completion_sem < 0)
		  err = B_ERROR;
		ddprintf("SOUND_SET_CAPTURE_COMPLETION_SEM");
		break;

	case SOUND_UNSAFE_WRITE:
		size = ((audio_buffer_header*)buf)->reserved_1;
		err = sound_write(cookie, 0, buf, &size);
		break;

	case SOUND_UNSAFE_READ:
		size = ((audio_buffer_header*)buf)->reserved_1;
	    err = sound_read(cookie, 0, buf, &size);
		break;

	case SOUND_LOCK_FOR_DMA:
	    err = lock_memory(buf, ((audio_buffer_header*)buf)->reserved_1,
						  B_DMA_IO);
		break;

	default:
		err = B_ERROR;
		ddprintf("SOUND unknown ioctl");
	}

	ddprintf(" => %x (=%d)\n", err, err);
	return err;
}


/* -----
	driver-related structures
----- */

static char* sound_name[] = {"audio/old/sound", NULL};

static device_hooks sound_device = {
		sound_open, 		/* -> open entry point */
		sound_close, 		/* -> close entry point */
		sound_free,			/* -> free entry point */
		sound_control, 		/* -> control entry point */
		sound_read,			/* -> read entry point */
		sound_write			/* -> write entry point */
};

const char**
publish_devices()
{
	return sound_name;
}

device_hooks*
find_device(const char *name)
{
	return &sound_device;
}

