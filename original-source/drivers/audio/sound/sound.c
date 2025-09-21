/* ++++++++++
	FILE:	sound.c
	REVS:	$Revision: 1.39 $
	NAME:	herold
	DATE:	Mon Feb 06 18:46:32 PST 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <OS.h>
#include <KernelExport.h>
#include <ISA.h>
#include <ByteOrder.h>
#include <Drivers.h>
#include <R3MediaDefs.h>
#include <interrupts.h>
#include <sound.h>
#include <cs4231.h>

#if defined(__INTEL__)
# include <mapio.h>
# define WSS_MODE 1
# define DMA_COUNT(entry)			((entry)->transfer_count)
# define DMA_ADDRESS(entry)			((entry)->address)
#else
# include <eagle.h>
# define WSS_MODE 0
# define DMA_COUNT(e)				(B_LENDIAN_TO_HOST_INT16((e)->transfer_count))
# define DMA_ADDRESS(e)				(B_LENDIAN_TO_HOST_INT32((e)->address) \
									 - PCI_MEM_HOST_MEMORY)
#endif

#define ISA_READ_CHAR(addr)			(*isa->read_io_8)((int) addr)
#define ISA_WRITE_CHAR(addr, val)	(*isa->write_io_8)((int) addr, val)

#undef ANALYZE
#include <analyser.h>

#define ddprintf if (debug_output) dprintf
//#define ddprintf if (TRUE) dprintf
#define X(N) ddprintf("*%d*\n",(N));

#define BUFTIME_SIZE sizeof(audio_buffer_header)

#if WSS_MODE
#define IO_PORT				0x534
#define INTERRUPT			B_INT_IRQ7
#define PLAYBACK_CHANNEL	1
#define CAPTURE_CHANNEL		0
#else
#define IO_PORT				CS_base
#define INTERRUPT			B_INT_SND
#define PLAYBACK_CHANNEL	6
#define CAPTURE_CHANNEL		7
#endif

#define INSTALL_INTERRUPT	install_io_interrupt_handler
#define REMOVE_INTERRUPT	remove_io_interrupt_handler

int32	api_version = B_CUR_DRIVER_API_VERSION;

static char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info		*isa;

static void print_regs();

/* -----
	forward declarations
----- */

static void set_params(sound_setup *);

/* -----
	private defines
----- */

#define MAX_SAMPLE		0x10000
#define MAX_DMALIST 	(((MAX_SAMPLE*2)/B_PAGE_SIZE)+2)
#define MAX_QUEUE		4

/* -----
	private types
----- */

typedef struct {
	int				is_last;				/* flag: last q entry for buffer */
	int				sample_count;			/* # samples in this entry */
	audio_buffer_header* timestamp;			/* NULL if not first q entry */
	int				index;					/* index into scat/gath list */
	isa_dma_entry	dlist [MAX_DMALIST];	/* scatter/gather list */
} qentry;


/* -----
	private globals
----- */


static vchar 	*index_reg;			/* -> index register */
static vchar 	*data_reg;			/* -> data register */
static vchar 	*wss_config;		/* -> WSS register */
static int		debug_output = 0;	/* flag: spewage on/off */
static long		initialized = 0;	/* driver initialized bit */

static spinlock	p_lock = 0;			/* spinlock for playback queue */
static sem_id	p_sem = -1;			/* playback queue access */
static int		p_write = 0;		/* next playback queue elem # to set up */
static int		p_read = 0;			/* next playback queue elem # to use */
static uchar	p_bytes_sample = 2;	/* playback shift for sample-to-bytes */
static sem_id	p_completion_sem;	/* sem to release when buffer complete */
static qentry	p_queue[MAX_QUEUE];	/* playback queue */
static int64	p_sample_count;
static bigtime_t p_last_int;

static int pad[500] = {0, 0, 0, 0, 0, 0, 0, 0};

static spinlock	c_lock = 0;			/* spinlock for capture queue */
static sem_id	c_sem = -1;			/* capture queue access */
static int		c_write = 0;		/* next capture queue elem # to set up */
static int		c_read = 0;			/* next capture queue elem # to use */
static uchar	c_bytes_sample = 2;	/* capture shift for sample-to-bytes */
static sem_id	c_completion_sem;	/* sem to release when buffer complete */
static qentry	c_queue[MAX_QUEUE];	/* capture queue */
static int64	c_sample_count;
static bigtime_t c_last_int;

static spinlock	reg_lock = 0;		/* access lock for codec regs */

static sem_id	read_lock = -1;
static sem_id	write_lock = -1;

static uchar	sample_sizes[16] = {
	1, 2, 1, 2, 2, 4, 1, 2, 0, 0, 1, 2, 2, 4, 0, 0 };

static sound_setup		codec = {				/* current codec setup */
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
	1 },		/* non-zero mutes dac output */

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
	1},			/* non-zero mutes dac output */

	/* general setup */
	kHz_44_1,	/* 44.1KHz */
	linear_16bit_big_endian_stereo,	/* playback format */
	linear_16bit_big_endian_stereo,	/* capture format */
	0,			/* non-zero enables dither on 16 => 8 bit */
	0,			/* 0..64 dac to adc loopback attenuation, in -1.5 dB steps */
	0,			/* non-zero enables loopback */
	1,			/* zero (2.0 Vpp) non-zero (2.8 Vpp) output level boost */
	1,			/* non-zero enables highpass filter in adc */
	32,			/* set mono speaker volume to 1/2 scale */
	0			/* non-zero mutes speaker */
};


/* ----------
	lock_codec_regs
----- */
static cpu_status
lock_codec_regs (void)
{
	cpu_status	ps;

	ps = disable_interrupts ();
	acquire_spinlock (&reg_lock);
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
	iread reads one of the indirectly accessed registers
----- */
static uchar
iread (int offset)
{
	cpu_status	ps;
	char		reg;

	ps = lock_codec_regs();

	ISA_WRITE_CHAR(index_reg, offset);
	reg = ISA_READ_CHAR(data_reg);
	
	unlock_codec_regs (ps);
	return reg;
}


/* ----------
	ichange changes select bits in one of the indirectly accessed registers
----- */
static void
ichange (int offset, uchar mask, uchar merge)
{
	cpu_status	ps;
	uchar		data;

	ps = lock_codec_regs();

	ISA_WRITE_CHAR(index_reg, offset);
	data = ISA_READ_CHAR(data_reg);

	data &= mask;
	data |= merge;

	ISA_WRITE_CHAR(data_reg, data);

	unlock_codec_regs (ps);
	return;
}



/* ----------
	set_mono_gain sets the volume of the mono speaker.  The volume control
	is external to the codec, and is set using the codec's two external control
	pins as follows:
		xctl0:	0 - clear volume level 1 - enable increment of volume level
		xctl1:	1->0 transition increments volume level (when xctl0 is 1)
----- */
static void
set_mono_gain (char level)
{
	level &= 0x3F;

	ichange (CS_pin_control, ~CS_pin_xctl0, CS_pin_xctl0);	/* volume = 0 */
	ichange (CS_pin_control, ~CS_pin_xctl0, 0);				/* enable incr */
	while (level--) {
		ichange (CS_pin_control, ~CS_pin_xctl1, CS_pin_xctl1);
		ichange (CS_pin_control, ~CS_pin_xctl1, 0);			/* volume++ */
	}
}


/* ----------
	complete_mode_change completes a pending mode change, then
	waits until the autocalibration sequence has completed
----- */
static status_t
complete_mode_change (void)
{
	cpu_status	ps;
	int			i;
	uchar val;

	ps = lock_codec_regs();

	for (i = 0; ISA_READ_CHAR(index_reg) & 0x80; i++)	/* wait till chip readable */
		if (i > 10000)					/* for as long as 1 second */
			goto bail;
		else
			spin (100);

	val = ISA_READ_CHAR(index_reg); 	/* complete mode change */
	val &= ~CS_index_mce;
	ISA_WRITE_CHAR(index_reg, val);

	for (i = 0; ISA_READ_CHAR(index_reg) & 0x80; i++)	/* wait till chip readable */
		if (i > 10000)					/* for as long as 1 second */
			goto bail;
		else
			spin (100);

	unlock_codec_regs (ps);

	/* !!! should prevent others from touching h/w till this completes !!! */

	for (i = 0; iread (CS_error_init) & CS_estat_aci; i++)
		if (i > 10000)					/* wait till autocal done */
			return B_ERROR;				/* for as long as 1 second */
		else
			spin (100);

	return B_NO_ERROR;

bail:
	unlock_codec_regs (ps);
	return B_ERROR;
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
	start_playback
----- */
static void
start_playback (int elem)
{
	qentry		*p;
	int			count;

	isa_dma_entry *entry;
	int dma_count;

	p = p_queue + elem;
	entry = p->dlist + p->index++;				/* -> scat/gath list entry */
	//ddprintf ("playback: phys addr = %.8x count=%.8x\n", entry->address, entry->transfer_count);
	count = dma_count = DMA_COUNT(entry) + 1;
	
	/* write count reg, then enable playback */

	count /= p_bytes_sample;
	count -= 1;
	ichange (CS_play_lower_base, 0x00, count & 0xFF);
	ichange (CS_play_upper_base, 0x00, count >> 8);
	ichange (CS_interface_config + CS_index_trd, ~CS_config_pen, CS_config_pen);
	//	ISA_WRITE_LONG((vlong *) (index_reg - 16), DMA_ADDRESS(entry));
	//	analyser (0x120, DMA_ADDRESS(entry));
	start_isa_dma (
		PLAYBACK_CHANNEL,
		(void *) DMA_ADDRESS(entry),
		dma_count,
		8,
		0x30
	);

	if (p->timestamp)
	  p->timestamp->time = system_time();
}
	

/* ----------
	start_capture
----- */
static void
start_capture (int elem)
{
	qentry				*c;
	int					count;

	isa_dma_entry *entry;
	int dma_count;

	c = c_queue + elem;
	//ddprintf ("elem=%d index=%d sample_count=%.8x\n", elem, c->index, c->sample_count);
	entry = c->dlist + c->index++;			/* -> scat/gath list entry */
    count = dma_count = DMA_COUNT(entry) + 1;

	/* write count reg, then enable capture */

	count /= c_bytes_sample;
	count -= 1;
	analyser (0x128, 0);
	ichange (CS_capture_lower_base, 0x00, count & 0xFF);
	ichange (CS_capture_upper_base, 0x00, count >> 8);
	ichange (CS_interface_config + CS_index_trd, ~CS_config_cen, CS_config_cen);
	start_isa_dma (
		CAPTURE_CHANNEL,
		(void *) DMA_ADDRESS(entry),
		dma_count,
		4,
		0x30
	);

	if (c->timestamp)
	  c->timestamp->time = system_time();
}
	

/* ----------
	sound_inth
----- */

static int32
sound_inth(void *data)
{
	uchar		status;
	cpu_status	ps;
	qentry		*q;
	int			is_last = false;

	analyser (0x124, 0);
	status = iread (CS_alt_feature_status);

	if (status & CS_alt_status_pi) {			/* playback interrupt? */
		ichange (CS_alt_feature_status,			/* clear interrupt */
			~CS_alt_status_pi, 0x00);

		acquire_spinlock(&p_lock);
		q = p_queue + p_read;

		if (!(q->dlist[q->index-1].flag & B_LAST_ISA_DMA_ENTRY)) { /* more this list? */
			start_playback (p_read);			/* play it */
			release_spinlock (&p_lock);
			return B_HANDLED_INTERRUPT;
		}
		is_last = q->is_last;

		p_read = (p_read + 1) % MAX_QUEUE;		/* -> next q elem */
		if (p_read != p_write)					/* anything to play? */
			start_playback (p_read);			/* play it */
		else {									/* otherwise, stop playback */
			ichange (CS_interface_config, ~CS_config_pen, 0x00);
			p_last_int = system_time();
		}

		release_spinlock (&p_lock);
		release_sem_etc (p_sem, 1, B_DO_NOT_RESCHEDULE); /* queue entry available */
		if (is_last) {								/* buffer completed? */
		  release_sem_etc (p_completion_sem, 1,	B_DO_NOT_RESCHEDULE);
		  return B_INVOKE_SCHEDULER;
		}
		return B_HANDLED_INTERRUPT;
	}

	if (status & CS_alt_status_ci) {		/* capture interrupt? */
		ichange (CS_alt_feature_status,			/* clear interrupt */
			~CS_alt_status_ci, 0x00);

		acquire_spinlock(&c_lock);
		q = c_queue + c_read;

		if (!(q->dlist[q->index-1].flag & B_LAST_ISA_DMA_ENTRY)) { /* more this list? */
			start_capture (c_read);				/* capture it */
			release_spinlock (&c_lock);
			return B_HANDLED_INTERRUPT;
		}
		is_last = q->is_last;

		c_read = (c_read + 1) % MAX_QUEUE;		/* -> next q elem */
		if (c_read != c_write)					/* anything to capture? */
			start_capture (c_read);				/* capture it */
		else {									/* otherwise, stop capturing */
			ichange (CS_interface_config, ~CS_config_cen, 0x00);
			c_last_int = system_time();
		}

		release_spinlock (&c_lock);
		release_sem_etc (c_sem, 1, B_DO_NOT_RESCHEDULE); /* queue entry available */
		if (is_last) {									/* buffer completed? */
		  release_sem_etc (c_completion_sem, 1,	B_DO_NOT_RESCHEDULE);
		  return B_INVOKE_SCHEDULER;
		}
		return B_HANDLED_INTERRUPT;
	}

	if (status & CS_alt_status_ti) {		/* timer interrupt? */
		ichange (CS_alt_feature_status,			/* clear interrupt */
			~CS_alt_status_ti, 0x00);
		ddprintf ("sound: timer interrupt\n");
		return B_HANDLED_INTERRUPT;
	}

	return B_UNHANDLED_INTERRUPT;
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
	int				i;
	struct channel 	*sc;
	struct channel 	*cc;

	/* to save code space, loop for left & right channel */

	for (i = 0, sc = &s->left, cc = &codec.left;
	     i < 2;
	     i++, sc = &s->right, cc = &codec.right)
	{
		if ( (sc->adc_gain != cc->adc_gain) ||
		     (sc->adc_source != cc->adc_source) ||
		     (!sc->mic_gain_enable != !cc->mic_gain_enable) )
			ichange (CS_left_adc_control + i, 0x00, 
				(sc->adc_gain & CS_adc_gain) +
				(sc->adc_source << 6) +
				(sc->mic_gain_enable ? CS_adc_mge : 0) );

		if ( (sc->aux1_mix_gain != cc->aux1_mix_gain) ||
		     (!sc->aux1_mix_mute != !cc->aux1_mix_mute) )
			ichange (CS_left_aux1_in_control + i, 0x00, 
				(sc->aux1_mix_gain & CS_iaux_attn) +
				(sc->aux1_mix_mute ? CS_iaux_mute : 0) );

		if ( (sc->aux2_mix_gain != cc->aux2_mix_gain) ||
		     (!sc->aux2_mix_mute != !cc->aux2_mix_mute) )
			ichange (CS_left_aux2_in_control + i, 0x00, 
				(sc->aux2_mix_gain & CS_iaux_attn) +
				(sc->aux2_mix_mute ? CS_iaux_mute : 0) );

		if ( (sc->line_mix_gain != cc->line_mix_gain) ||
		     (!sc->line_mix_mute != !cc->line_mix_mute) )
			ichange (CS_left_line_in_control + i, 0x00, 
				(sc->line_mix_gain & CS_line_in_mix_gain) +
				(sc->line_mix_mute ? CS_line_in_mix_mute : 0) );

		if ( (sc->dac_attn != cc->dac_attn) ||
		     (!sc->dac_mute != !cc->dac_mute) )
			ichange (CS_left_dac_control + i, 0x00, 
				(sc->dac_attn & CS_dac_attn) +
				(sc->dac_mute ? CS_dac_mute : 0) );

	}

	/* now check remaining, non left-or-right paramters */

	if (!s->dither_enable != codec.dither_enable)
		ichange (CS_pin_control, ~CS_pin_den,
			s->dither_enable ? CS_pin_den : 0);

	if ( (s->loop_attn != codec.loop_attn) ||
	     (!s->loop_enable != !codec.loop_enable) )
		ichange (CS_loopback_control, 0x00, 
			((s->loop_attn << 2) & CS_loop_attn) +
			(s->loop_enable ? CS_loop_dme : 0) );

	if (!s->output_boost != !codec.output_boost)
		ichange (CS_alt_feature_enable_1, ~CS_alt1_olb, 
			(s->output_boost ? CS_alt1_olb : 0) );
	
	if (!s->highpass_enable != !codec.highpass_enable)
		ichange (CS_alt_feature_enable_2, ~CS_alt2_hpf, 
			(s->highpass_enable ? CS_alt2_hpf : 0) );
	
	if (s->mono_gain != codec.mono_gain
		|| !s->mono_mute != !codec.mono_mute)
		set_mono_gain (s->mono_mute ? 0 : s->mono_gain);

	if ( (s->sample_rate != codec.sample_rate) ||
		 (s->playback_format != codec.playback_format) ||
		 (s->capture_format != codec.capture_format) ) {

		/* !!! TO DO: ENSURE NO SOUND CURRENTLY PLAYING !!! */

		/* change capture first, as it does not cause resync */
		ichange(CS_capture_data_format | CS_index_mce, 0x0F,
			s->capture_format<<4);

		/* change playback, sample rate. Causes resync (+ autocal if enabled) */
		ichange(CS_clock_play_data_format | CS_index_mce, 0, 
				s->sample_rate + (s->playback_format<<4) );

		p_bytes_sample = sample_sizes[s->playback_format];
		c_bytes_sample = sample_sizes[s->capture_format];

		complete_mode_change ();	/* complete mode change, wait for autocal */
	}

	/* finally, update our saved copy of the parameters */

	memcpy (&codec, s, sizeof (sound_setup));
}


/* ----------
	sound_uninit does the actual driver shutdown.
----- */
static void
sound_uninit (void)
{

	/* disable capture & playback */

	ichange (CS_interface_config, ~(CS_config_pen + CS_config_cen), 0);

	ichange (CS_pin_control, ~CS_pin_ien, 0);		/* disable interrupts */
	REMOVE_INTERRUPT (INTERRUPT, sound_inth, NULL);	/* remove handler */

	unlock_isa_dma_channel (PLAYBACK_CHANNEL);
	unlock_isa_dma_channel (CAPTURE_CHANNEL);

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
sound_close (void* cookie)
{
	sound_uninit ();			/* do the real shutdown */
	return B_NO_ERROR;
}


/* ----------
	find_codec - checks if codec is present, sets up pointers
----- */
static bool
find_codec (void)
{

#if !defined(__INTEL__)
	if (io_card_version() < 0)
		return FALSE;
#endif

	index_reg = (vchar*) IO_PORT + CS_index;
	data_reg = (vchar*) IO_PORT + CS_data;
	ISA_WRITE_CHAR(0x333, 0xf1);
	ddprintf("HSR  %02x\n", ISA_READ_CHAR(0x333) & 0xff);

	wss_config = index_reg - 4;

	ddprintf ("index reg=%.8x data reg=%.8x\n", index_reg, data_reg);
	return TRUE;
}
	

/* ----------
	init_wss
----- */

void
init_wss()
{
#if WSS_MODE

  static const char interrupt_bits[12] = {
    -1, -1, -1, -1, -1, -1, -1, 0x08, -1, 0x10, 0x18, 0x20
  };
  static const char dma_bits[4] = {
    1, 2, 0, 3
  };
  static const char int_config_enable = 0x40;
  static const char dma_config_enable = 0x08;
  static char val;

  val = (int_config_enable | interrupt_bits[INTERRUPT]);
  ddprintf ("WSS interrupt setup: write %.2x to %.8x\n", val, wss_config);
  ISA_WRITE_CHAR(wss_config, val);
  val = ISA_READ_CHAR(wss_config + 3);
  ddprintf ("WSS interrupt setup: read %.2x from %.8x\n", val, wss_config+3);
  if ((val | 0x40) == 0)
	ddprintf("[IRQ Conflict?]\n");
  val = (dma_config_enable | dma_bits[PLAYBACK_CHANNEL]);
  ddprintf ("WSS dma setup: write %.2x to %.8x\n", val, wss_config);
  ISA_WRITE_CHAR(wss_config, val);
  val = ISA_READ_CHAR(wss_config + 3);
  ddprintf ("WSS dma setup: read %.2x from %.8x\n", val, wss_config+3);
#endif
}

/* ----------
	init_hardware does the one-time-only codec hardware
----- */

status_t
init_hardware(void)
{
	int				i;

	static struct {
		uchar	offset;			/* offset to indirect register */
		uchar	mask;			/* mask to apply to current value */
		uchar	merge;			/* new data to merge w/masked value */
	} inittab[] = {
		CS_mode_id,					0xBF, 0x40,	/* mode 2 features enabled */
		CS_interface_config,		0x30, 0x08,	/* all dma, autocal enabled, all disabled */
		CS_pin_control,				0x3B, 0x00,	/* ints disabled */
		CS_alt_feature_enable_1,	0x3E, 0x00,	/* no timer, use last on overrun, 2V output max */
		CS_alt_feature_status,		0x80, 0x00,	/* clear all interrupts */
		CS_capture_data_format,		0x0F, 0xD0,	/* 44.1 kHz, 16 bit stereo */
		CS_clock_play_data_format,	0x00, 0xDB	/* xtal2, 44.1 Khz, 16bit stereo */
	};

	if (get_module(isa_name, &(module_info *)isa) != B_OK)
		return B_ERROR;

	if (!find_codec())
		goto error;

	/* wait for chip to wake up for as long as 1 second */
	for (i = 0; ISA_READ_CHAR(index_reg) & 0x80; i++)
		if (i > 10000)
			goto error;
		else
			spin (100);

	init_wss();

	/* initialize some indirect registers, w/mode change enabled */

	for (i = 0; i < sizeof (inittab)/sizeof(inittab[0]); i++) {
		ichange (inittab[i].offset | CS_index_mce, inittab[i].mask,
			inittab[i].merge);
	}

	/* complete mode change, wait for autocal */

	if (complete_mode_change () != B_OK)
		goto error;

	put_module(isa_name);
	return B_NO_ERROR;


error:
	put_module(isa_name);
	return B_ERROR;
}


/* ----------
	init_driver is called when the driver is loaded.
----- */

status_t
init_driver(void)
{
	char			sound_area_name[]="SOUND_DMA";
	area_info		sound_area_info;
	physical_entry	sound_physical_entry;
	int				table_size;

	
	if (get_module(isa_name, &(module_info *)isa) != B_OK)
		return B_ERROR;

	if (!find_codec()) {
		put_module(isa_name);
		return B_ERROR;
	}

	read_lock = create_sem(1, "sound read lock");
	if (read_lock < 0)
	  return read_lock;

	write_lock = create_sem(1, "sound write lock");
	if (write_lock < 0)
	  return write_lock;

	ddprintf ("SOUND: one_time_init complete\n");
	return B_NO_ERROR;
}


/* ----------
	uninit_driver - clean up before driver is unloaded
----- */
void
uninit_driver (void)
{
	delete_sem(read_lock);
	delete_sem(write_lock);
	put_module(isa_name);
}


	
/* ----------
	sound_open sets up the driver.
----- */

static status_t
sound_open (const char* name, uint32 flags, void** cookie)
{
	if (atomic_or (&initialized, 1) != 0)	/* only 1 open at a time */
	  return B_ERROR;

	if ((p_sem = create_sem (MAX_QUEUE-1, "sound_play_queue")) < 0)
		goto err;
	set_sem_owner(p_sem, B_SYSTEM_TEAM);
	if ((c_sem = create_sem (MAX_QUEUE-1, "sound_capture_queue")) < 0)
		goto err;
	set_sem_owner(c_sem, B_SYSTEM_TEAM);

	if (lock_isa_dma_channel (PLAYBACK_CHANNEL) != B_NO_ERROR)
		goto err;
	if (lock_isa_dma_channel (CAPTURE_CHANNEL) != B_NO_ERROR)
		goto err;

	init_params ();										/* setup codec */

	p_write = 0;
	p_read = 0;
	p_sample_count = 0;
	p_last_int = 0;
	c_write = 0;
	c_read = 0;
	c_sample_count = 0;
	c_last_int = 0;

	INSTALL_INTERRUPT (INTERRUPT, sound_inth, NULL, 0);		/* install handler */
	ichange (CS_pin_control, ~CS_pin_ien, CS_pin_ien);	/* enable interrupts */
	
	return B_NO_ERROR;

err:
	sound_uninit ();
	return B_ERROR;
}


/* ----------
	sound_read
----- */

static status_t
sound_read (void* cookie, off_t pos, void *buf, size_t* numBytes)
{
    long		count = *numBytes - BUFTIME_SIZE;
	int			size;					/* # bytes this pass */
	long		nread;					/* # bytes read this dma */
	int			nread_total = 0;		/* total # bytes read */
	audio_buffer_header* header = (audio_buffer_header*) buf;
	char*		dma_start = (char*) buf + BUFTIME_SIZE;
	cpu_status	ps;
	status_t	err;

	//ddprintf ("sound_read: buf=%.8x count=%.8x\n", buf, count);
	//ddprintf ("sound_read: c_bytes_sample=%.8x\n", c_bytes_sample);

	if (!initialized)
		return B_ERROR;

	err = acquire_sem_etc(read_lock, 1, B_CAN_INTERRUPT, 0);
	if (err != B_NO_ERROR)
	  return err;

	while (count > 0) {
		size = count / c_bytes_sample;
		if (size > MAX_SAMPLE)
			size = MAX_SAMPLE;
		size *= c_bytes_sample;
		count -= size;

		//ddprintf ("sound_read: this pass, size=%.8x\n", size);
		while (size > 0) {
			acquire_sem(c_sem);

			nread = make_isa_dma_table (dma_start, size, B_8_BIT_TRANSFER,
										c_queue [c_write].dlist, MAX_DMALIST);
			c_queue [c_write].sample_count = nread / c_bytes_sample;
			c_queue [c_write].index = 0;

			if (BUFTIME_SIZE && !nread_total) {
			  if (c_read == c_write && c_last_int)
				c_sample_count += (system_time() - c_last_int) * 441 / 10000;
			  header->sample_clock = c_sample_count * 10000 / 441;
			  c_sample_count += size / 4;
			  c_queue[c_write].timestamp = header;
			}
			else
			  c_queue[c_write].timestamp = NULL;

			size -= nread;
			nread_total += nread;
			dma_start += nread;

			/* if last part of buffer, notify client when done */
			if (count <= 0 && size <= 0)
				c_queue[c_write].is_last = 1;
			else
				c_queue[c_write].is_last = 0;

			ps = lock_queue (&c_lock);
			if (c_read == c_write)
				start_capture (c_write);
			c_write = (c_write + 1) % MAX_QUEUE;
			unlock_queue (&c_lock, ps);
		}
	}

	release_sem(read_lock);
	*numBytes = nread_total + BUFTIME_SIZE;
	return 0;
}

/* ----------
	sound_write
----- */

static status_t 
sound_write (void* cookie, off_t pos, const void* buf, size_t* numBytes)
{
    long		count = *numBytes - BUFTIME_SIZE;
	int			size;					/* # bytes this pass */
	int			written;				/* # bytes written this dma */
	int			written_total = 0;		/* total # bytes written */
	audio_buffer_header* header = (audio_buffer_header*) buf;
	char*		dma_start = (char*) buf + BUFTIME_SIZE;
	cpu_status	ps;
	status_t	err;

	if (!initialized)
		return B_ERROR;

	//ddprintf ("sound_write: buf=%.8x count=%.8x\n", buf, count);
	//ddprintf ("sound_write: p_bytes_sample=%.8x\n", p_bytes_sample);

	err = acquire_sem_etc(write_lock, 1, B_CAN_INTERRUPT, 0);
	if (err != B_NO_ERROR)
	  return err;

	while (count > 0) {
		size = count / p_bytes_sample;
		if (size > MAX_SAMPLE)
			size = MAX_SAMPLE;
		size *= p_bytes_sample;
		count -= size;

		//ddprintf ("sound_write: this pass, size=%.8x\n", size);
		while (size > 0) {
			acquire_sem(p_sem);

			written = make_isa_dma_table ((void*) dma_start, size, B_8_BIT_TRANSFER,
										  p_queue [p_write].dlist, MAX_DMALIST);
			/*display_memory (p_queue [p_write].dlist, sizeof (p_queue [p_write].dlist));*/
			p_queue [p_write].sample_count = written / p_bytes_sample;
			p_queue [p_write].index = 0;

			if (BUFTIME_SIZE && !written_total) {
			  if (p_read == p_write && p_last_int)
				p_sample_count += (system_time() - p_last_int) * 441 / 10000;
			  header->sample_clock = p_sample_count * 10000 / 441;
			  p_sample_count += size / 4;
			  p_queue[p_write].timestamp = header;
			}
			else
			  p_queue[p_write].timestamp = NULL;

			size -= written;
			written_total += written;
			dma_start += written;

			/* if last part of buffer, notify client when done */
			if (count <= 0 && size <= 0)
				p_queue[p_write].is_last = 1;
			else
				p_queue[p_write].is_last = 0;

			ps = lock_queue (&p_lock);
			if (p_read == p_write)
				start_playback (p_write);
			p_write = (p_write + 1) % MAX_QUEUE;
			unlock_queue (&p_lock, ps);

		}
	}

	release_sem(write_lock);
	*numBytes = written_total + BUFTIME_SIZE;
	return 0;
}


/* ----------
	sound_control
----- */

static status_t
sound_control(void* cookie, uint32 msg, void *buf, size_t len)
{
	status_t	err = B_NO_ERROR;
	cpu_status	ps;
	size_t		size;

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

	case SOUND_GET_PLAYBACK_TIMESTAMP:
		ps = lock_queue (&p_lock);
		*(audio_buffer_header*) buf = *p_queue[p_read].timestamp;
		err = p_write - p_read;
		unlock_queue (&p_lock, ps);
		if (err < 0)
			err = err + MAX_QUEUE;
		break;

	case SOUND_GET_CAPTURE_TIMESTAMP:
		ps = lock_queue (&c_lock);
		*(audio_buffer_header*) buf = *c_queue[c_read].timestamp;
		err = c_write - c_read;
		unlock_queue (&c_lock, ps);
		if (err < 0)
			err = err + MAX_QUEUE;
		break;

	case SOUND_SET_PLAYBACK_COMPLETION_SEM:
		p_completion_sem = *(sem_id *)buf;
		if (p_completion_sem < 0) err = B_ERROR;
		ddprintf("SOUND_SET_PLAYBACK_COMPLETION_SEM");
		break;

	case SOUND_SET_CAPTURE_COMPLETION_SEM:
		c_completion_sem = *(sem_id *)buf;
		if (c_completion_sem < 0) err = B_ERROR;
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

static status_t
sound_free (void* cookie)
{
  return 0;
}

static char* sound_name[] = {"audio/old/sound", NULL};

static device_hooks sound_device = {
		sound_open, 		/* -> open entry point */
		sound_close, 		/* -> close entry point */
		sound_free,			/* -> free entry point */
		sound_control, 		/* -> control entry point */
		sound_read,			/* -> read entry point */
		sound_write,		/* -> write entry point */
		NULL,				/* -> select entry point */
		NULL				/* -> deselect entry point */
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


static void
print_regs ()
{
  int i;

  ddprintf("SR   %02x\n", ISA_READ_CHAR(index_reg + CS_status) & 0xff);

  for (i=0; i<32; i++)
	ddprintf("I%-2d  %02x\n", i, iread(i));
}
