
#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdlib.h>
#include <string.h>

#include "audio_drv.h"
#include "misc.h"
#include "debug.h"
#include "ac97.h"
#include "ali1535.h"

#define AC97_BAD_VALUE		(0xffff)
#define MAX_PLAYBACK_BUF_SIZE	0x4000UL

//	define to 1 to actually start record channel
#define DO_RECORD 0



extern pci_module_info *pci;
#define g_pci pci



typedef struct _timers_data_t {
	vuint64 IntCount;
	bigtime_t SystemClock;
	int32 Lock;
} timers_data_t;

typedef struct _ali1535_hw {

	timers_data_t timers_data;	
	ac97_t codec;

	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	void * PcmPlaybackBuf;

	area_id PcmCaptureAreaId;
	size_t PcmCaptureBufSize; 
	void * PcmCaptureBuf;

	ushort BA0;
	ushort half_frame_count;

	sem_id ac97sem;
	int32 ac97ben;

	int32 SRHz;
	uint32 format;

} ali1535_hw; 

static status_t 	ali1535_Init(sound_card_t * card);
static status_t 	ali1535_Uninit(sound_card_t * card);
static void 		ali1535_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	ali1535_SetPlaybackDmaBuf(sound_card_t *card,  uint32 *size, void** addr);
static status_t 	ali1535_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 size, void** addr);
static void 		ali1535_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		ali1535_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		ali1535_SetCaptureDmaBuf(sound_card_t *card, uint32 *size, void** addr);
static status_t		ali1535_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** addr);
static void 		ali1535_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		ali1535_StartPcm(sound_card_t *card);
static void 		ali1535_StopPcm(sound_card_t *card);
static status_t		ali1535_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		ali1535_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		ali1535_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		ali1535_InitJoystick(sound_card_t * card);
static void 		ali1535_enable_gameport(sound_card_t * card);
static void 		ali1535_disable_gameport(sound_card_t * card);

static status_t 	ali1535_InitMPU401(sound_card_t * card);
static void 		ali1535_enable_mpu401(sound_card_t *card);
static void 		ali1535_disable_mpu401(sound_card_t *card);
static void 		ali1535_enable_mpu401_interrupts(sound_card_t *card);
static void 		ali1535_disable_mpu401_interrupts(sound_card_t *card);

static sound_card_functions_t ali1535_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&ali1535_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&ali1535_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&ali1535_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&ali1535_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&ali1535_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&ali1535_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&ali1535_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&ali1535_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&ali1535_SetPlaybackDmaBufOld,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&ali1535_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&ali1535_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&ali1535_SetCaptureDmaBufOld,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&ali1535_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&ali1535_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&ali1535_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&ali1535_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&ali1535_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&ali1535_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&ali1535_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&ali1535_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&ali1535_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL, // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 

&ali1535_SetPlaybackDmaBuf,
&ali1535_SetCaptureDmaBuf
} ;


static status_t 	ali1535_InitCodec(sound_card_t *card);
static void 		ali1535_UninitCodec(sound_card_t *card);
static uint16 		ali1535_CodecRead(void* host, uchar offset);
static status_t 	ali1535_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);

/* --- 
	Interrupt service routine
	---- */

#define rprintf(x) (void)0
//#define rprintf(x) dprintf x

static inline uchar REGREAD8(ushort base, ushort reg)
{
	uchar ret = g_pci->read_io_8(base+reg);
	rprintf(("0x%02x <= 0x%02x       (0x%04x)\n", reg, ret, base));
	return ret;
}

static inline ushort REGREAD16(ushort base, ushort reg)
{
	ushort ret = g_pci->read_io_16(base+reg);
	rprintf(("0x%02x <= 0x%04x     (0x%04x)\n", reg, ret, base));
	return ret;
}

static inline ulong REGREAD32(ushort base, ushort reg)
{
	ulong ret = g_pci->read_io_32(base+reg);
	rprintf(("0x%02x <= 0x%08lx (0x%04x)\n", reg, ret, base));
	return ret;
}

static inline void REGWRITE8(ushort base, ushort reg, uchar val)
{
	rprintf(("0x%02x => 0x%02x       (0x%04x)\n", reg, val, base));
	return g_pci->write_io_8(base+reg, val);
}

static inline void REGWRITE16(ushort base, ushort reg, ushort val)
{
	rprintf(("0x%02x => 0x%04x     (0x%04x)\n", reg, val, base));
	return g_pci->write_io_16(base+reg, val);
}

static inline void REGWRITE32(ushort base, ushort reg, ulong val)
{
	rprintf(("0x%02x => 0x%08lx (0x%04x)\n", reg, val, base));
	return g_pci->write_io_32(base+reg, val);
}



static int32 ali1535_isr(void *data)
{
	bigtime_t time = system_time();
	sound_card_t * card = (sound_card_t *)data;
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	uint32 BA0 = hw->BA0;
	uint16 frame;
	uint32 status = REGREAD32(BA0, ALI_AINT);

	acquire_spinlock(&card->bus.pci.lock);

	//	ack interrupts
	REGWRITE32(BA0, ALI_AINT, status);

kprintf("%c", (status & (1<<ALI_PLAYBACK_CHANNEL)) ? ((status & (1<<ALI_RECORD_CHANNEL)) ? 'P' : 'p') : ((status & (1<<ALI_RECORD_CHANNEL)) ? 'r' : '?'));

	if (status & (1<<ALI_PLAYBACK_CHANNEL))
	{
		acquire_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);	
		hw->timers_data.SystemClock = time;
		hw->timers_data.IntCount++;
		release_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);

		REGWRITE8(BA0, ALI_GC_CIR, ALI_PLAYBACK_CHANNEL);
		frame = REGREAD32(BA0, ALI_CH_CSO_ALPHA_FMS)>>16;
		(*card->pcm_playback_interrupt)(card, (frame >= hw->half_frame_count));
	}
	if (status & (1<<ALI_RECORD_CHANNEL))
	{
		REGWRITE8(BA0, ALI_GC_CIR, ALI_RECORD_CHANNEL);
		frame = REGREAD32(BA0, ALI_CH_CSO_ALPHA_FMS)>>16;
		(*card->pcm_capture_interrupt)(card, (frame >= hw->half_frame_count));
	}

	release_spinlock(&card->bus.pci.lock);

	return (status ? B_INVOKE_SCHEDULER : B_UNHANDLED_INTERRUPT);
#if 0
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;
	uint32 istat;
	static int half = 0;
	static int chalf= 0;

	acquire_spinlock(&card->bus.pci.lock);
	istat = READ_REG(((ali1535_hw*)card->hw)->BA0, BA0_HISR);

	if (!(istat & (0x1|0x2|0x100000))) {	/*	not one of ours	*/
		release_spinlock(&card->bus.pci.lock);
		return ret;
	}
	//	ask another interrupt
	ret = B_HANDLED_INTERRUPT;
	WRITE_REG(((ali1535_hw*)card->hw)->BA0, BA0_HICR, 0x3, -1);
	release_spinlock(&card->bus.pci.lock);

	if (istat & 0x1) {
		//fixme:	figure out which half of the playback buffer

		{// Store internal time for synchronization 
			bigtime_t time = system_time();
			
			acquire_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);	
			((ali1535_hw*)card->hw)->timers_data.SystemClock = time;
			((ali1535_hw*)card->hw)->timers_data.IntCount++;
			release_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);
		}			

		//fixme:	call the card->pcm_playback_interrupt function

		if ((*card->pcm_playback_interrupt)(card, half))
			ret = B_INVOKE_SCHEDULER;
		half = !half;
	}
	if (istat & 0x2) {
		//fixme:	figure out which half of the capture buffer
		if ((*card->pcm_capture_interrupt)(card, chalf))
			ret = B_INVOKE_SCHEDULER;
		chalf = !chalf;
	}
	if (istat & 0x100000) {
		//fixme:	MIDI interrupt
		if (ret == B_UNHANDLED_INTERRUPT)
			ret = B_HANDLED_INTERRUPT;
	}
//kprintf("%c", '@'+(istat&3)+ret*4);
	return ret;
#endif
}


//#pragma mark ali1535_GetClocks
static void ali1535_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
 	double half_buf_time;
	uint64 int_count;
	ali1535_hw * hw = (ali1535_hw *)card->hw;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((ali1535_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((ali1535_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((ali1535_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)hw->half_frame_count / (double)(((ali1535_hw*)card->hw)->SRHz) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
}

static void ali1535_enable_mpu401(sound_card_t *card)
{
}

static void ali1535_disable_mpu401(sound_card_t *card)
{
}

static void ali1535_enable_mpu401_interrupts(sound_card_t *card)
{
}

static void ali1535_disable_mpu401_interrupts(sound_card_t *card)
{
}


static void ali1535_enable_gameport(sound_card_t * card)
{
}

static void ali1535_disable_gameport(sound_card_t * card)
{
}

static status_t ali1535_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static status_t ali1535_InitMPU401(sound_card_t * card)
{
	return ENODEV;
}


static cpu_status ac97_cp;

static void lock_ac97(sound_card_t * card)
{
	if (atomic_add(&((ali1535_hw*)card->hw)->ac97ben, 1) > 0)
	{
		acquire_sem(((ali1535_hw*)card->hw)->ac97sem);
	}
	ac97_cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
}

static void unlock_ac97(sound_card_t * card)
{
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(ac97_cp);

	if (atomic_add(&((ali1535_hw*)card->hw)->ac97ben, -1) > 1)
	{
		release_sem_etc(((ali1535_hw*)card->hw)->ac97sem, 1, B_DO_NOT_RESCHEDULE);
	}
}

#if 0
static void duh(short *buf, int frames)
{
	while (frames-- > 0) {
		*buf++ = ((frames & 0x3f)-0x20)*1000;
		*buf++ = ((frames & 0x3f)-0x20)*1000;
	}
}
#endif

static uint32
ali1535_set_rate(ali1535_hw * hw, uint32 rate, int channel)
{
	ushort BA0 = hw->BA0;

	ASSERT(channel >= 0 && channel <= 31);
	if (rate > 48000)
		rate = 48000;
	if (rate < 8000)
		rate = 8000;

	REGWRITE8(BA0, ALI_GC_CIR, channel);
	REGWRITE32(BA0, ALI_CH_ESO_DELTA,
		(REGREAD32(BA0, ALI_CH_ESO_DELTA) & 0xffff0000UL) | 
		((rate << 12) / 48000));

	return rate;
}

static void
ali1535_set_format(ali1535_hw * hw, uint32 format, uint32 mask, int channel)
{
	ushort BA0 = hw->BA0;
	uint32 old = 0;

	ASSERT(channel >= 0 && channel <= 31);

	REGWRITE8(BA0, ALI_GC_CIR, channel);
	REGWRITE32(BA0, ALI_EBUF1, ALI_STILL_MODE);
	REGWRITE32(BA0, ALI_EBUF2, ALI_STILL_MODE);
	if (mask != 0xffffffff)
	{
		old = REGREAD32(BA0, ALI_CH_GVSEL_PAN_VOL_CTRL_EC);
	}
	old = (old & ~mask) | (format & mask);
	REGWRITE32(BA0, ALI_CH_GVSEL_PAN_VOL_CTRL_EC, old);
	spin(20);	//	empirically determined...
}

static uchar fmt_shift(uint32 format)
{
	uchar ret = 1;
	if (format & ALI_FMT_16BITU)
		ret++;
	if (format & ALI_FMT_STEREO)
		ret++;
	return ret;
}

static void
ali1535_setup_dma(ali1535_hw * hw, void * address, size_t size, uint32 format, int channel)
{
	uchar shift = fmt_shift(format);
	uint32 BA0 = hw->BA0;
	physical_entry ent;

	ASSERT(channel >= 0 && channel <= 31);

	get_memory_map(address, size, &ent, 1);
	ddprintf(("ali1535: physical address 0x%lx size 0x%lx\n", (uint32)ent.address, ent.size));

	//	select channel
	REGWRITE32(BA0, ALI_GC_CIR, channel);

	//	ping-pong interrupts
	REGWRITE8(BA0, ALI_GC_CIR+1, ALI_MIDLP_ENDLP_IE |
		REGREAD8(BA0, ALI_GC_CIR+1));

	//	enable channel interrupts
	REGWRITE32(BA0, ALI_AINTEN, (1<<channel) |
		REGREAD8(BA0, ALI_AINTEN));

	REGWRITE32(BA0, ALI_CH_CSO_ALPHA_FMS, 0x0);

	REGWRITE32(BA0, ALI_CH_LBA, (uint32)ent.address);

	REGWRITE32(BA0, ALI_CH_ESO_DELTA, (((size>>shift)-1) << 16) |
		(REGREAD32(BA0, ALI_CH_ESO_DELTA) & 0xffffUL));

	REGWRITE32(BA0, ALI_CH_GVSEL_PAN_VOL_CTRL_EC,
		format | 0x80000000UL);
}

static void
ali1535_start_dma(ali1535_hw * hw, uint32 channels)
{
	ushort BA0 = hw->BA0;

	REGWRITE32(BA0, ALI_START, channels);
}

static void
ali1535_stop_dma(ali1535_hw * hw, uint32 channels)
{
	ushort BA0 = hw->BA0;

	REGWRITE32(BA0, ALI_STOP, channels);
}

static status_t ali1535_Init(sound_card_t * card)
{
	status_t err = B_OK;
	ali1535_hw * hw;
	ushort BA0;
	cpu_status cp;

#if DEBUG
	uint32 cc;

//kernel_debugger("Yoo-hoo!");

ddprintf(("ali1535: Init()\n"));

	cc = g_pci->read_pci_config(card->bus.pci.info.bus, card->bus.pci.info.device,
		card->bus.pci.info.function, 0x10, 0x4);
ddprintf(("ali1535: config 0x10 == 0x%08lx\n", cc));

	cc = g_pci->read_pci_config(card->bus.pci.info.bus, card->bus.pci.info.device,
		card->bus.pci.info.function, 0x4, 0x4);
ddprintf(("ali1535: config 0x04 == 0x%08lx\n", cc));
#endif

	hw = (ali1535_hw *)malloc(sizeof(*hw));
	if (!hw)
	{
		err = ENOMEM;
		goto error_0;
	}
	
	memset(hw, 0, sizeof(*hw));

	hw->ac97sem = create_sem(0, "ali1535 ac97sem");
	set_sem_owner(hw->ac97sem, B_SYSTEM_TEAM);
	hw->ac97ben = 0;
	if (hw->ac97sem < 0)
	{
		goto error_1;
	}

	hw->PcmPlaybackBufSize = MAX_PLAYBACK_BUF_SIZE;
	hw->PcmPlaybackAreaId = create_area("ali1535 playback DMA",
		&hw->PcmPlaybackBuf, B_ANY_KERNEL_ADDRESS, MAX_PLAYBACK_BUF_SIZE,
		B_CONTIGUOUS, B_WRITE_AREA);
	if ((err = hw->PcmPlaybackAreaId) < 0)
	{
		goto error_2;
	}

	hw->PcmCaptureBufSize = MAX_PLAYBACK_BUF_SIZE;
	hw->PcmCaptureAreaId = create_area("ali1535 playback DMA",
		&hw->PcmCaptureBuf, B_ANY_KERNEL_ADDRESS, MAX_PLAYBACK_BUF_SIZE,
		B_CONTIGUOUS, B_WRITE_AREA);
	if ((err = hw->PcmPlaybackAreaId) < 0)
	{
		goto error_3;
	}

	hw->SRHz = 44100;
	hw->format = ALI_FMT_STEREO|ALI_FMT_16BIT;
	BA0 = card->bus.pci.info.u.h0.base_registers[0];
	hw->BA0 = BA0;
	ddprintf(("ali1535: found at 0x%04x\n", hw->BA0));

	card->hw = hw;

	REGWRITE32(BA0, ALI_GLOBAL_CONTROL, ALI_PCM_IN_ENABLE);
	REGWRITE32(BA0, ALI_AINTEN, ALI_AINTEN_DISABLE);
	REGWRITE32(BA0, ALI_AINT, ALI_AINT_RESET);

	err = ali1535_InitCodec(card);
	if (err < 0)
	{
		goto error_4;
	}

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	hw->SRHz = ali1535_set_rate(hw, hw->SRHz, ALI_PLAYBACK_CHANNEL);
	ali1535_set_format(hw, hw->format, ALI_FMT_MASK, ALI_PLAYBACK_CHANNEL);
	hw->SRHz = ali1535_set_rate(hw, hw->SRHz, ALI_RECORD_CHANNEL);
	ali1535_set_format(hw, hw->format, ALI_FMT_MASK, ALI_RECORD_CHANNEL);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, ali1535_isr, card, 0);

ddprintf(("ali1535: Init OK\n"));

	return B_OK;

error_4:
		delete_area(hw->PcmCaptureAreaId);
error_3:
		delete_area(hw->PcmPlaybackAreaId);
error_2:
		delete_sem(hw->ac97sem);
error_1:
		free(hw);
		card->hw = NULL;
error_0:
		eprintf(("ali1535: %s\n", strerror(err)));
		return err;
}

static status_t ali1535_Uninit(sound_card_t * card)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	cpu_status cp;

ddprintf(("ali1535: Uninit()\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	ali1535_stop_dma(hw, (1<<ALI_PLAYBACK_CHANNEL)|(1<<ALI_RECORD_CHANNEL));

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, ali1535_isr, card);
	ali1535_UninitCodec(card);

	/* Clean up */
	delete_area(hw->PcmCaptureAreaId);
	delete_area(hw->PcmPlaybackAreaId);
	delete_sem(hw->ac97sem);
	free(card->hw);
	card->hw = NULL;

	return B_OK;
}


#if TIMER_INTERRUPT
struct timer_int
{
	timer t;
	sound_card_t * card;
};
static struct timer_int t_int;

static int32
timer_func(
	timer * t)
{
	return ali1535_isr(((struct timer_int *)t)->card);
}

static void
start_tint(
	sound_card_t * card)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;

	memset(&t_int, 0, sizeof(t_int));
	t_int.card = card;
	add_timer(&t_int.t, timer_func, 1000000LL / hw->SRHz * hw->half_frame_count, B_PERIODIC_TIMER);
}

static void
stop_tint()
{
	cancel_timer(&t_int.t);
}
#endif

static void ali1535_StartPcm(sound_card_t *card)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	cpu_status cp;

ddprintf(("ali1535: StartPcm()\n"));

	memset(&hw->timers_data, 0, sizeof(hw->timers_data));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	ali1535_setup_dma(hw, hw->PcmPlaybackBuf, hw->PcmPlaybackBufSize, hw->format,
		ALI_PLAYBACK_CHANNEL);

	hw->half_frame_count = hw->PcmPlaybackBufSize >> (fmt_shift(hw->format)+1);

#if TIMER_INTERRUPT
	start_tint(card);
#endif

#if DO_RECORD
	ali1535_setup_dma(hw, hw->PcmCaptureBuf, hw->PcmCaptureBufSize, hw->format,
		ALI_RECORD_CHANNEL);

	ali1535_start_dma(hw, (1<<ALI_PLAYBACK_CHANNEL)|(1<<ALI_RECORD_CHANNEL));
#else
	ali1535_start_dma(hw, (1<<ALI_PLAYBACK_CHANNEL));
#endif

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void ali1535_StopPcm(sound_card_t *card)
{
	cpu_status cp;

ddprintf(("ali1535: StopPcm()\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

#if TIMER_INTERRUPT
	stop_tint();
#endif

	ali1535_stop_dma((ali1535_hw *)card->hw, (1<<ALI_PLAYBACK_CHANNEL)|(1<<ALI_RECORD_CHANNEL));

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void ali1535_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	cpu_status cp;

ddprintf(("ali1535: SetPlaybackSR(%ld)\n", sample_rate));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	hw->SRHz = ali1535_set_rate(hw, sample_rate, ALI_PLAYBACK_CHANNEL);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static status_t ali1535_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 size, void** log_addr)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;

ddprintf(("ali1535: SetPlaybackDmaBufOld(0x%lx)\n", size));

	if (size > MAX_PLAYBACK_BUF_SIZE)
	{
		eprintf(("ali1535: max playback buf size is 0x%lx, requested 0x%lx\n", MAX_PLAYBACK_BUF_SIZE, size));
		return ENOMEM;
	}
	hw->PcmPlaybackBufSize = size;
	*log_addr = hw->PcmPlaybackBuf;

	return B_OK;
}

static status_t ali1535_SetPlaybackDmaBuf(sound_card_t *card,  uint32 *psize, void** log_addr)
{
	if (*psize < 1024)
	{
		*psize = 1024;
	}
	if (*psize > MAX_PLAYBACK_BUF_SIZE)
	{
		*psize = MAX_PLAYBACK_BUF_SIZE;
	}
	return ali1535_SetPlaybackDmaBufOld(card, *psize, log_addr);
}

static void ali1535_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	uint32 format = 0;
	cpu_status cp;

ddprintf(("ali1535: SetPlaybackFormat(%d, %d)\n", num_of_bits, num_of_ch));

	if (num_of_bits == 16)
		format |= ALI_FMT_16BIT;
	if (num_of_ch == 2)
		format |= ALI_FMT_STEREO;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	ali1535_set_format(hw, format, ALI_FMT_MASK, ALI_PLAYBACK_CHANNEL);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	hw->format = format;
}

static void ali1535_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	cpu_status cp;

ddprintf(("ali1535: SetCaptureSR(%ld)\n", sample_rate));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	hw->SRHz = ali1535_set_rate(hw, sample_rate, ALI_RECORD_CHANNEL);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static status_t ali1535_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** log_addr)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;

ddprintf(("ali1535: SetCaptureDmaBufOld(0x%lx)\n", size));

	if (size > MAX_PLAYBACK_BUF_SIZE)
	{
		eprintf(("ali1535: max capture buf size is 0x%lx, requested 0x%lx\n", MAX_PLAYBACK_BUF_SIZE, size));
		return ENOMEM;
	}
	hw->PcmCaptureBufSize = size;
	*log_addr = hw->PcmCaptureBuf;

	return B_OK;
}

static status_t ali1535_SetCaptureDmaBuf(sound_card_t *card, uint32 *psize, void** log_addr)
{
	if (*psize < 1024)
		*psize = 1024;
	else if (*psize > MAX_PLAYBACK_BUF_SIZE)
		*psize = MAX_PLAYBACK_BUF_SIZE;
	return ali1535_SetCaptureDmaBufOld(card, *psize, log_addr);
}

static void ali1535_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	uint32 format = 0;
	cpu_status cp;

ddprintf(("ali1535: SetCaptureFormat(%d, %d)\n", num_of_bits, num_of_ch));

	if (num_of_bits == 16)
		format |= ALI_FMT_16BIT;
	if (num_of_ch == 2)
		format |= ALI_FMT_STEREO;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	ali1535_set_format(hw, format, ALI_FMT_MASK, ALI_RECORD_CHANNEL);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	hw->format = format;
}

static uint16 ali1535_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	ushort BA0 = hw->BA0;
	int guard_spins = 200;
	uint32 time1, time2;

	while ((--guard_spins > 0) && ((REGREAD16(BA0, ALI_ACR0_AC97_R) & ALI_AC97_BUSY) != 0))
	{
		spin(1);
	}
	if (!guard_spins)
	{
		eprintf(("ali1535: timeout waiting for ac97 1\n"));
		return AC97_BAD_VALUE;
	}

	REGWRITE8(BA0, ALI_ACR0_AC97_R, offset);
	guard_spins = 1000;
	time1 = REGREAD32(BA0, ALI_STIMER);
	do {
		spin(1);
		time2 = REGREAD32(BA0, ALI_STIMER);
	}
	while ((time1 == time2) && (--guard_spins > 0));
//ddprintf(("time1 = 0x%lx, time2 = 0x%lx\n", time1, time2));
	if (!guard_spins)
	{
		eprintf(("ali1535: timeout waiting for ac97 2\n"));
		return AC97_BAD_VALUE;
	}

	REGWRITE8(BA0, ALI_ACR0_AC97_R+1, 0x80);
	guard_spins = 200;
	while (--guard_spins > 0)
	{
		if (!(REGREAD16(BA0, ALI_ACR0_AC97_R) & ALI_AC97_BUSY))
		{
			return REGREAD16(BA0, ALI_ACR0_AC97_RD);
		}
		spin(1);
	}
	eprintf(("ali1535: timeout waiting for ac97 3\n"));
	return AC97_BAD_VALUE;
}


static uint16 ali1535_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	lock_ac97(card);
	ret = ali1535_CodecReadUnsecure(card, offset);
	unlock_ac97(card);	

//ddprintf(("ali1535: CodecRead(0x%02x) = 0x%04x\n", offset, ret));

	return ret;
}

static status_t ali1535_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	sound_card_t* card = (sound_card_t*) host;
	ali1535_hw * hw = (ali1535_hw *)card->hw;
	ushort BA0 = hw->BA0;
	int guard_spins = 200;
	ushort data = value;
	uint32 time1, time2;

//ddprintf(("ali1535: CodecWrite(0x%02x = 0x%04x & 0x%04x) (BA0 = 0x%04x)\n", offset, value, mask, BA0));

	lock_ac97(card);
	if (mask != 0xffff)
	{
		ushort current = ali1535_CodecReadUnsecure(host, offset);
//ddprintf(("ali1535: mask = 0x%04x, called read = 0x%04x\n", mask, current));
		data = (value & mask) | (current & ~mask);
	}
	while (--guard_spins > 0)
	{
		ushort v = REGREAD16(BA0, ALI_ACR0_AC97_W);
		if (v & ALI_AC97_BUSY)
		{
			spin(1);
			continue;
		}
		REGWRITE16(BA0, ALI_ACR0_AC97_WD, data);
		REGWRITE8(BA0, ALI_ACR0_AC97_W, offset);
		guard_spins = 200;
		time1 = REGREAD32(BA0, ALI_STIMER);
		do {
			spin(1);
			time2 = REGREAD32(BA0, ALI_STIMER);
		}
		while ((time1 == time2) && (--guard_spins > 0));
		if (guard_spins > 0)
		{
			REGWRITE8(BA0, ALI_ACR0_AC97_W+1, 0x80);
		}
		unlock_ac97(card);	
		if (guard_spins == 0)
		{
			eprintf(("ali1535: end guard_spins = %d\n", guard_spins));
		}
		return (guard_spins > 0) ? B_OK : B_TIMED_OUT;
	}
	unlock_ac97(card);	
	eprintf(("ali1535: codec busy guard_spins = %d\n", guard_spins));
	return B_TIMED_OUT;
}


static status_t ali1535_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;

ddprintf(("ali1535: InitCodec\n"));

	err = ac97init(&((ali1535_hw*)card->hw)->codec, 
					(void*)card,
					ali1535_CodecWrite,
					ali1535_CodecRead);

ddprintf(("ali1535: ac97init() returns %s\n", strerror(err)));

	if (err!=B_OK)
	{
		eprintf(("ali1535_InitCodec(): ac97init err = %ld!\n",err));
	}	
	return err;				
}

static void ali1535_UninitCodec(sound_card_t *card)
{
ddprintf(("ali1535: UninitCodec()\n"));
	ac97uninit(&((ali1535_hw*)card->hw)->codec); 
}

static status_t ali1535_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t err;

ddprintf(("ali1535: SoundSetup()\n"));

	err = AC97_SoundSetup(&((ali1535_hw*)card->hw)->codec, ss);
	if (err == B_OK) {
		ali1535_SetPlaybackSR(card, sr_from_constant(ss->sample_rate));
		switch (ss->playback_format) {
		case linear_8bit_unsigned_mono:
			ali1535_SetPlaybackFormat(card, 8, 1);
			break;
		case linear_8bit_unsigned_stereo:
			ali1535_SetPlaybackFormat(card, 8, 2);
			break;
		case linear_16bit_big_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
			ali1535_SetPlaybackFormat(card, 16, 1);
			break;
		case linear_16bit_big_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
			ali1535_SetPlaybackFormat(card, 16, 2);
			break;
		default:
			eprintf(("ali1535: unknown sample format %d\n", ss->playback_format));
			ali1535_SetPlaybackFormat(card, 16, 2);
		}
	}
	return err;
}

static status_t  ali1535_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;

ddprintf(("ali1535: GetSoundSetup()\n"));

	ret = AC97_GetSoundSetup(&((ali1535_hw*)card->hw)->codec,ss);
	ss->sample_rate = constant_from_sr(((ali1535_hw*)card->hw)->SRHz);;
	switch (((ali1535_hw *)card->hw)->format) {
	default:
		eprintf(("ali1535: unknown existing format 0x%lx\n", ((ali1535_hw *)card->hw)->format));
	case ALI_FMT_STEREO|ALI_FMT_16BIT:
		ss->playback_format = linear_16bit_big_endian_stereo;
		break;
	case ALI_FMT_STEREO:
		ss->playback_format = linear_8bit_unsigned_stereo;
		break;
	case ALI_FMT_16BIT:
		ss->playback_format = linear_16bit_big_endian_mono;
		break;
	case 0:
		ss->playback_format = linear_8bit_unsigned_mono;
		break;
	}
	ss->capture_format = ss->playback_format;
	ddprintf(("GetSoundSetup: ss->sample_rate = %d\n", ss->sample_rate));
	return ret;
}




card_descrtiptor_t ali1535_audio_descr = {
	"ali1535",
	PCI, 
	&ali1535_functions,
	PCI_VENDOR_ID_ALI,  
	PCI_DEVICE_ID_ALI_M1535_AUDIO
};

card_descrtiptor_t* CardDescriptors[] = {
	&ali1535_audio_descr, 
	NULL
};
 
void GetDriverSettings()
{
}






