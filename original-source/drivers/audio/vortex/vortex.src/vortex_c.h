/************************************************************************/
/*                                                                      */
/*                              vortex.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov										*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#define TEST (1)

#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>

#include "audio_drv.h"
#include "aureal.h"
#include "hwio.h"
#include "misc.h"
#include "debug.h"
#include <ac97_module.h>
#include "audio_drv.h"
#if defined(AU8820)
	#include "au8820regs.h"
	typedef FIFOVDBPointersAndControlReg FIFOPointersAndControlReg;
	typedef DmaVDBChannelModeReg DmaChannelModeReg;
	typedef DmaVDBChannelSizeAndGotoReg DmaChannelSizeAndGotoReg;
	typedef DmaVDBChannelAddressReg DmaChannelAddressReg;
#elif defined(AU8830)
	#include "au8830regs.h"
#endif



#define AC97_BAD_VALUE		(0xffff)

#define PLAY_VDB_CHANNEL (0)
#define REC_VDB_CHANNEL		(1)

#define PLAY_SRC_CHANNEL	(0) //0 - left, 1 - right
#define REC_SRC_CHANNEL		(2) //2 - left, 3 - right

enum {LEFT_CH = 0, RIGHT_CH};
enum {DISABLE = 0, ENABLE};
enum {NO = 0, YES};

#define CODEC_FREQ			(48000)


typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;

typedef struct _vortex_hw {
	timers_data_t timers_data;	
	mem_mapped_regs_t regs;
	ac97_t codec;
	DmaStatusReg LastVDBStat;

	area_id PcmPlaybackAreaId; 
	struct{bool IsStereo; bool Is16bits;} PcmPlaybackFormat;
	bool PcmSetPlaybackFormat;
	uint32 PcmPlaybackSR;
	bool PcmSetPlaybackSR;

	area_id PcmCaptureAreaId; 
	struct{bool IsStereo; bool Is16bits;} PcmCaptureFormat;
	bool PcmSetCaptureFormat;
	uint32 PcmCaptureSR;
	bool PcmSetCaptureSR;
} vortex_hw; 


static status_t 	vortex_Init(sound_card_t * card);
static status_t 	vortex_Uninit(sound_card_t * card);
static void 		vortex_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	vortex_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		vortex_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		vortex_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		vortex_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		vortex_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		vortex_StartPcm(sound_card_t *card);
static void 		vortex_StopPcm(sound_card_t *card);
static status_t		vortex_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		vortex_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		vortex_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		vortex_InitJoystick(sound_card_t * card);
static void 		vortex_enable_gameport(sound_card_t * card);
static void 		vortex_disable_gameport(sound_card_t * card);

static status_t 	vortex_InitMPU401(sound_card_t * card);
static void 		vortex_enable_mpu401(sound_card_t *card);
static void 		vortex_disable_mpu401(sound_card_t *card);
static void 		vortex_enable_mpu401_interrupts(sound_card_t *card);
static void 		vortex_disable_mpu401_interrupts(sound_card_t *card);

static uchar mpu401_read_port(void*, int);
static void mpu401_write_port(void*, int, uchar);
static mpu401_io_hooks_t mpu401_io_hooks = {&mpu401_read_port, &mpu401_write_port}; 

static uchar joy_read_port(void*);
static void joy_write_port(void*, uchar);

static joy_io_hooks_t joy_io_hooks = {&joy_read_port,&joy_write_port};




static sound_card_functions_t vortex_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&vortex_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&vortex_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&vortex_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&vortex_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&vortex_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&vortex_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&vortex_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&vortex_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&vortex_SetPlaybackDmaBuf,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&vortex_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&vortex_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&vortex_SetCaptureDmaBuf,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&vortex_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&vortex_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&vortex_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&vortex_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&vortex_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&vortex_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&vortex_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&vortex_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&vortex_disable_mpu401_interrupts,

&joy_io_hooks,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
&mpu401_io_hooks // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
} ;



#if defined(AU8820)
	card_descrtiptor_t au8820_descriptor = {
		"au8820",
		PCI, 
		&vortex_functions,
		AUREAL_VENDOR_ID, 
		AU8820_DEVICE_ID
	};
#elif defined(AU8830)
	card_descrtiptor_t au8830_descriptor = {
		"au8830",
		PCI, 
		&vortex_functions,
		AUREAL_VENDOR_ID, 
		AU8830_DEVICE_ID
	};
#endif

 
static status_t 	vortex_InitCodec(sound_card_t *card);
static void 		vortex_UninitCodec(sound_card_t *card);
static uint16 		vortex_CodecRead(void* host, uchar offset);
static status_t 	vortex_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);

//uchar *global_dma_buf_log_addr;
//void *global_dma_buf_phys_addr;
static size_t global_dma_buf_size;


/* --- 
	Interrupt service routine
	---- */
static int32 vortex_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;
	PciGlobalStatusReg GSReg;
	PciInterruptSCReg ISReg;

	/* Check if the interrupt is generated by this card */
	GSReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gsr));
	if (!GSReg.Bits.InterruptAsserted) 
		goto exit; // not us. Leave
		
	/* Read interrupt status reg */
	ISReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.isr));

	/* Check if this is "DMA end-of-buffer" interrupt */ 
	if (ISReg.Bits.DMAEndOfBuffer) 
	{
		{// Acknowledge it
			PciInterruptSCReg mask;
			
			mask.dwValue = 0x0;
			mask.Bits.DMAEndOfBuffer = 1;
			mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.isr), 
							mask.Bits.DMAEndOfBuffer, mask.Bits.DMAEndOfBuffer);
		}
		
		ret = B_HANDLED_INTERRUPT;


		{// Determine DMA VDB channel 
			DmaStatusReg reg;

			reg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBStat[0]));
			if((reg.dwValue ^ ((vortex_hw*)card->hw)->LastVDBStat.dwValue) & (0x1<<(31 - PLAY_VDB_CHANNEL*2)))
			{
				{// Store internal time for synchronization 
					bigtime_t time = system_time();
					
					acquire_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);	
					((vortex_hw*)card->hw)->timers_data.SystemClock = time;
					((vortex_hw*)card->hw)->timers_data.IntCount++;
					release_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);
				}			
				// Proceed playback interrupt
				if ((*card->pcm_playback_interrupt)(card, ((reg.dwValue & (0x1<<(31 - PLAY_VDB_CHANNEL*2))) ? 0 : 1))) 
					ret = B_INVOKE_SCHEDULER;
				
			}				
		if((reg.dwValue ^ ((vortex_hw*)card->hw)->LastVDBStat.dwValue) & (0x1<<(31 - REC_VDB_CHANNEL*2)))
			{
				/*{// Store internal time for synchronization 
					bigtime_t time = system_time();
					
					acquire_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);	
					((vortex_hw*)card->hw)->timers_data.SystemClock = time;
					((vortex_hw*)card->hw)->timers_data.IntCount++;
					release_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);
				}*/			
				// Proceed playback interrupt
				if ((*card->pcm_capture_interrupt)(card, ((reg.dwValue & (0x1<<(31 - REC_VDB_CHANNEL*2))) ? 0 : 1))) 
					ret = B_INVOKE_SCHEDULER;
				
			}				
			//Save dma.VDBStat[0] in ((vortex_hw*)card->hw)->LastVDBStat
			((vortex_hw*)card->hw)->LastVDBStat.dwValue = reg.dwValue;
		}

		goto exit;
	}

	/* Check if MIDI request is on */
	if (ISReg.Bits.MIDIRequest)
	{
		if ((*card->midi_interrupt)(card))
		{  
			ret = B_INVOKE_SCHEDULER;
		}
		else 
		{
			ret = B_HANDLED_INTERRUPT;
		}
	}

exit:	
	return ret;
}


static void vortex_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((vortex_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((vortex_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((vortex_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = global_dma_buf_size 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((vortex_hw*)card->hw)->PcmPlaybackSR) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
}

static void vortex_enable_mpu401(sound_card_t *card)
{
/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.
	PciGlobalControlReg GCReg;
	
	GCReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));
	GCReg.Bits.MPU401Enable = ENABLE; 
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	(void)mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));			
*/
}

static void vortex_disable_mpu401(sound_card_t *card)
{
/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.
	PciGlobalControlReg GCReg;
	
	GCReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));
	GCReg.Bits.MPU401Enable = DISABLE; 
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	(void)mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));			
*/
}

static void vortex_enable_mpu401_interrupts(sound_card_t *card)
{
	PciInterruptSCReg ICReg;
	
	ICReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr));
	ICReg.Bits.MIDIRequest = ENABLE;
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr), ICReg.dwValue, 0xffffffff);
}

static void vortex_disable_mpu401_interrupts(sound_card_t *card)
{
	PciInterruptSCReg ICReg;
	
	ICReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr));
	ICReg.Bits.MIDIRequest = DISABLE;
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr), ICReg.dwValue, 0xffffffff);
}


static void vortex_enable_gameport(sound_card_t * card)
{
/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.
	PciGlobalControlReg GCReg;
	
	GCReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));
	GCReg.Bits.JoystickEnable = ENABLE;
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
*/
}

static void vortex_disable_gameport(sound_card_t * card)
{
/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.
	PciGlobalControlReg GCReg;
	
	GCReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));
	GCReg.Bits.JoystickEnable = DISABLE;
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
*/
}

static status_t vortex_InitJoystick(sound_card_t * card)
{
	card->joy_base = 0;

/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.
	int base_port = 0x201; // hardcoded for now
	
	{// Set game base port/ 
		PciGlobalControlReg GCReg;
		 
		GCReg.dwValue = mapped_read_32(&((vortex_hw *)card->hw)->regs, OFFSET(pci.gcr));
		GCReg.Bits.JoystickSelect = PCI_JOYSTICK_SELECT_201;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	}
	card->joy_base = base_port;
*/

	return B_OK;
}

static status_t vortex_InitMPU401(sound_card_t * card)
{
	card->mpu401_base = 0; 
/* MX
// Starting from maui version the pci configurable io used for vortex mpu401 and joy
// Hard-Decode Enabling not requred.

	int base_port = 0x330; // hardcoded for now
	
	{// Set mpu401 base port  
		PciGlobalControlReg GCReg;
		 
		GCReg.dwValue = mapped_read_32(&((vortex_hw *)card->hw)->regs, OFFSET(pci.gcr));
		GCReg.Bits.MPU401Select = PCI_MPU401_SELECT_330;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	}
	card->mpu401_base = base_port; 
*/	
	{/* Set the gameport clock divider */
		GAMEControlReg Control;
		enum {MASTER_CLOCK_49_DOT_152 = 0x61, MASTER_CLOCK_50 = 0x63};
	
		Control.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(game.Control));
		Control.Bits.ClockDivider = MASTER_CLOCK_49_DOT_152;  
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(game.Control), Control.dwValue, 0xffffffff);
	}

	return B_OK;
}


static status_t vortex_UninitPCM(sound_card_t * card)
{
	{/* Disable DMA end-of-buffer interrupts */
		PciInterruptSCReg reg;
		reg.dwValue = 0x0;
		reg.Bits.DMAEndOfBuffer = ENABLE; //clear this bit
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr), 0, reg.dwValue);
	}
	return B_OK;
}

static status_t vortex_PcmStartPlayback(sound_card_t * card)
{
	if(((vortex_hw*)card->hw)->PcmSetPlaybackFormat)
	{	
		bool isStereo = ((vortex_hw*)card->hw)->PcmPlaybackFormat.IsStereo; 
	
		/********************************
		 * 11.2.4 Programm SRC's		*
		 ********************************/
		 
		// Set throttle source of SRC0 (left ch) and SRC1 (right ch) to DMA connect
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ThrottleSource), 
							(0x1 << (PLAY_SRC_CHANNEL + LEFT_CH)) || ((isStereo ? 1 : 0) << (PLAY_SRC_CHANNEL + RIGHT_CH)), 
							(0x1 << (PLAY_SRC_CHANNEL + LEFT_CH)) || (0x1 << (PLAY_SRC_CHANNEL + RIGHT_CH)));
			
		{// Set SRC0 and SRC1 for the left and right channels correspondingly
			// Set param reg 0
			SRCChannelParams ParamReg;
			enum {THROTTLE = 0, DRIFT}; 
			enum {SLOW = 0, FAST}; 
	
			ParamReg.dwValue = 0x0;						
			ParamReg.cp0.CorrelatedChannel = 0;		//src0 (left channel is master) 
			ParamReg.cp0.SamplesPerWing = (13 - 1);	// samples per wing minus 1
			ParamReg.cp0.DriftThrottle = THROTTLE; 
			ParamReg.cp0.DriftLockSpeed = SLOW;
			mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[0][PLAY_SRC_CHANNEL + LEFT_CH]), 
							ParamReg.dwValue, 0xffffffff);
			mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[0][PLAY_SRC_CHANNEL + RIGHT_CH]), 
							(isStereo ? ParamReg.dwValue : 0), 0xffffffff);
	
	
			// Set param reg 6				
			ParamReg.dwValue = 0x0;						
			ParamReg.cp6.ThrottleDirectionFlag = THROTTLE_IN;		
			ParamReg.cp6.FIFOTargetDepth = 1;	// FIFO depth = 15			
	 		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[6][PLAY_SRC_CHANNEL + LEFT_CH]), 
							ParamReg.dwValue, 0xffffffff);	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[6][PLAY_SRC_CHANNEL + RIGHT_CH]), 
							(isStereo ? ParamReg.dwValue : 0), 0xffffffff);	
		} 
	
		{//Program SRC Work-To-Do list
			SRCSrHeader HeaderChReg;
			SRCNextChannel NextChReg;
			
			HeaderChReg.dwValue	= 0x0;
			HeaderChReg.Bits.Header = PLAY_SRC_CHANNEL + (isStereo ? RIGHT_CH : LEFT_CH); 
			HeaderChReg.Bits.ChannelValid = (isStereo ? 1 : 0);
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.SrHeader[VDB_SR_CODEC]), 
							HeaderChReg.dwValue, 0xffffffff);	
			
			if(isStereo)
			{
				NextChReg.dwValue = 0x0;
				NextChReg.Bits.NextChannel = PLAY_SRC_CHANNEL + LEFT_CH; //src channel 1 is tail
				NextChReg.Bits.ChannelValid = 0;
				mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.NextChannel[PLAY_SRC_CHANNEL + RIGHT_CH]), 
								NextChReg.dwValue, 0xffffffff);	
			}
		}
	
	
		{/* Set Pointer & Control reg */
			FIFOPointersAndControlReg reg;
			
			reg.dwValue = 0x0;
#if defined(AU8830)
			reg.Bits.FlushFIFO 		= NO;
			reg.Bits.FreezePtrs		= NO;
			reg.Bits.WTVDBPointer	= 0;
#endif
			reg.Bits.HadUnderrun	= NO;
			reg.Bits.Wait4DMA 		= YES;
			reg.Bits.DMAPointer		= 0;
			reg.Bits.Empty 			= YES;
			reg.Bits.ChannelValid 	= NO;
			reg.Bits.Priority 		= FIFO_LOW_PRIORITY;
			reg.Bits.Stereo 		= (isStereo ? FIFO_STEREO_DATA : FIFO_MONO_DATA);
			reg.Bits.SentDMAReq		= 0;	//software never sets this bit to 1
			mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[PLAY_VDB_CHANNEL]), 
							reg.dwValue, 0xffffffff);
		}
	
	}

	if(((vortex_hw*)card->hw)->PcmSetPlaybackSR)
	{
		/* Store sampling rate in the "global" variable to be used in the GetClocks() ... */
		
		// Set throttle counter size
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ThrottleCountSize), 
							(uint32)(50000000 / (((vortex_hw*)card->hw)->PcmPlaybackSR)), 	//50 Mhz / sampling rate
							0xffffffff);
		
		/* Set Conversion_ratio (param reg 1)*/
			{
				SRCChannelParams ParamReg;
				uint32 dwSourceFreq = ((vortex_hw*)card->hw)->PcmPlaybackSR;
				uint32 dwTargetFreq = CODEC_FREQ;
				uint32 dwRatio;
		
				ParamReg.dwValue = 0x0;						
				
				if (dwSourceFreq > 96050) dwSourceFreq = 96050;
				dwRatio = (uint32)(((dwSourceFreq << 15) / dwTargetFreq) + 1) >> 1;
				if (dwRatio < 0x800) dwRatio = 0x800;
				if (dwRatio > 0x1FFFF) dwRatio = 0x1FFFF;
				ParamReg.cp1.ConversionRatio = (uint16)dwRatio;	//1/Conversion_ratio
		 		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[1][PLAY_SRC_CHANNEL + LEFT_CH]), 
								ParamReg.dwValue, 0xffffffff);	
				mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.ChannelParams[1][PLAY_SRC_CHANNEL + RIGHT_CH]), 
								ParamReg.dwValue, 0xffffffff);	
			}
	}

	return B_OK;
}

static status_t vortex_PcmStartCapture(sound_card_t * card)
{
	if(((vortex_hw*)card->hw)->PcmSetCaptureFormat)
	{	
		bool isStereo = ((vortex_hw*)card->hw)->PcmCaptureFormat.IsStereo; 

		{// Set param regs for the rec SRC channels
			SRCChannelParams ParamReg;
			enum {THROTTLE = 0, DRIFT}; 
			enum {SLOW = 0, FAST}; 
			
			// Set param reg 0
			ParamReg.dwValue = 0x0;						
			ParamReg.cp0.CorrelatedChannel = REC_SRC_CHANNEL + LEFT_CH;	// left rec ch is master
			ParamReg.cp0.SamplesPerWing = (13 - 1);	// samples per wing minus 1
			ParamReg.cp0.DriftThrottle = THROTTLE; 
			ParamReg.cp0.DriftLockSpeed = SLOW;
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.ChannelParams[0][REC_SRC_CHANNEL + LEFT_CH]), 
							ParamReg.dwValue, 0xffffffff);
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.ChannelParams[0][REC_SRC_CHANNEL + RIGHT_CH]), 
							(isStereo ? ParamReg.dwValue : 0), 0xffffffff);
	
			// Set param reg 6				
			ParamReg.dwValue = 0x0;						
			ParamReg.cp6.FIFOWritePointer = 15;	// = FIFO depth	
			ParamReg.cp6.FIFOReadPointer = 0;	// = FIFOWritePointer - FIFO depth		
			ParamReg.cp6.ThrottleDirectionFlag = THROTTLE_OUT;		
			ParamReg.cp6.FIFOTargetDepth = 1;	// correspond to FIFO depth = 15 (table 11-1)			
	 		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
	 						OFFSET(src.ChannelParams[6][REC_SRC_CHANNEL + LEFT_CH]), 
							ParamReg.dwValue, 0xffffffff);	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.ChannelParams[6][REC_SRC_CHANNEL + RIGHT_CH]), 
							(isStereo ? ParamReg.dwValue : 0), 0xffffffff);	
		}
		{//Program SRC Work-To-Do list
			SRCSrHeader HeaderChReg;
			SRCNextChannel NextChReg;
	
			//left ch is master, should be last in the list
			HeaderChReg.dwValue	= 0x0;
			HeaderChReg.Bits.Header = REC_SRC_CHANNEL + (isStereo ? RIGHT_CH : LEFT_CH); 
			HeaderChReg.Bits.ChannelValid = (isStereo ? 1 : 0);
	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.SrHeader[VDB_SR_SRC2]), 
										HeaderChReg.dwValue, 0xffffffff);	
			if(isStereo)
			{
				NextChReg.dwValue = 0x0;
				NextChReg.Bits.NextChannel = REC_SRC_CHANNEL + LEFT_CH;
				NextChReg.Bits.ChannelValid = 0;
				mapped_write_32(&((vortex_hw*)card->hw)->regs, 
								OFFSET(src.NextChannel[REC_SRC_CHANNEL + RIGHT_CH]), 
											NextChReg.dwValue, 0xffffffff);	
			}
		}

		{/* Set Pointer & Ctrl reg */
			FIFOPointersAndControlReg reg;
			
			reg.dwValue = 0x0;
#if defined(AU8830)
			reg.Bits.FlushFIFO 		= NO;
			reg.Bits.FreezePtrs		= NO;
			reg.Bits.WTVDBPointer	= 0;
#endif
			reg.Bits.HadUnderrun	= NO;
			reg.Bits.Wait4DMA 		= YES;
			reg.Bits.DMAPointer		= 0;
			reg.Bits.Empty 			= YES;
			reg.Bits.ChannelValid 	= NO;
			reg.Bits.Priority 		= FIFO_LOW_PRIORITY;
			reg.Bits.Stereo 		= (isStereo ? FIFO_STEREO_DATA : FIFO_MONO_DATA);
			reg.Bits.SentDMAReq		= 0;	//software never sets this bit to 1
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(fifo.VDBPtrCtl[REC_VDB_CHANNEL]), 	
							reg.dwValue, 0xffffffff);	// capture FIFO
		}
	}

	if(((vortex_hw*)card->hw)->PcmSetCaptureSR)
	{
		{// Set param regs for the rec SRC channels
			SRCChannelParams ParamReg;
	
			// Set param reg 1
			ParamReg.dwValue = 0x0;						
			{
				uint32 dwSourceFreq = (uint32) CODEC_FREQ;
				uint32 dwTargetFreq = (uint32) ((vortex_hw*)card->hw)->PcmCaptureSR;
				uint32 dwRatio;
				
				dwRatio = (uint32)(((dwSourceFreq << 15) / dwTargetFreq) + 1) >> 1;
				if (dwRatio < 0x800) 
					dwRatio = 0x800;
				if (dwRatio > 0x1FFFF) 
					dwRatio = 0x1FFFF;
				
				DB_PRINTF(("*** dwRatio = 0x%lx\n", dwRatio));
				
				ParamReg.cp1.ConversionRatio = (uint16)dwRatio;	// 1/Conversion_ratio
			}
	 		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
	 						OFFSET(src.ChannelParams[1][REC_SRC_CHANNEL + LEFT_CH]), 
							ParamReg.dwValue, 0xffffffff);	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(src.ChannelParams[1][REC_SRC_CHANNEL + RIGHT_CH]), 
							ParamReg.dwValue, 0xffffffff);	
		}
	}

/*
		// Set param reg 4
		ParamReg.dwValue = 0x0;						
		ParamReg.cp4.CurrentTimeFraction = 0;		
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(src.ChannelParams[4][REC_SRC_CHANNEL + LEFT_CH]), 
						ParamReg.dwValue, 0xffffffff);	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(src.ChannelParams[4][REC_SRC_CHANNEL + RIGHT_CH]), 
						ParamReg.dwValue, 0xffffffff);	


*/

 
	return B_OK;
}


static status_t vortex_Init(sound_card_t * card)
{
	status_t err = B_OK;

	//DB_PRINTF(("vortex_Init(): here! \n"));
	
	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(vortex_hw));
	if(!card->hw) 
	{
		derror("vortex_Init(): malloc error!\n");
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(vortex_hw));	// zero it out

	/* Some initialization */
	((vortex_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((vortex_hw*)card->hw)->PcmPlaybackAreaId = -1; 

	/* Enable Bus Mastering, Memory Space and I/O Space responses */
	pci_config_write(card, PCI_COMMAND, 2, 0x7, 0x7);

	/* Map vortex registers into pci memory */
	err = init_mem_mapped_regs(	&((vortex_hw*)card->hw)->regs,
								"vortex registers", 
								(void *)(card->bus.pci.info.u.h0.base_registers[0]),
								card->bus.pci.info.u.h0.base_register_sizes[0]); 
	
	if (err != B_OK) 
	{
		derror("vortex_Init: init_mem_mapped_regs error!\n");
		return err;
	}
	
	{/* Vortex soft reset */
		PciGlobalControlReg GCReg;
		 
		GCReg.dwValue = mapped_read_32(&((vortex_hw *)card->hw)->regs, OFFSET(pci.gcr));
		GCReg.Bits.VortexSoftReset = ENABLE;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
		spin(20000);	// wait some time
	}

	/* Init codec */
	if ((err = vortex_InitCodec(card))) 
	{
		derror("vortex_Init: vortex_InitCodec error!\n");
		 return err;
	}

	/* Install vortex isr */
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, vortex_isr, card, 0);

	
	{/* Enable global INTA# */
		PciGlobalControlReg GCReg;
		 
		GCReg.dwValue = mapped_read_32(&((vortex_hw *)card->hw)->regs, OFFSET(pci.gcr));
		GCReg.Bits.GlobalInterruptEnable = ENABLE; //set this bit
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	}


	return B_OK;
}


static status_t vortex_Uninit(sound_card_t * card)
{
	//DB_PRINTF(("vortex_Uninit(): here! \n"));

	/* Remove vortex isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, vortex_isr, card);

	/* Uninit codec */
	vortex_UninitCodec(card);

	/* Reset controller */
/*.......................................*/
/*.......................................*/
/*.......................................*/

	{/* Disable "DMA end-of-buffer" interrupts */
		PciInterruptSCReg reg;
		
		reg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr));
		reg.Bits.DMAEndOfBuffer = DISABLE;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr), reg.dwValue, 0xffffffff);
	}
	
	{/* Disable global INTA# */
		PciGlobalControlReg GCReg;
		
		GCReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr));
		GCReg.Bits.GlobalInterruptEnable = DISABLE; //clear this bit
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.gcr), GCReg.dwValue, 0xffffffff);
	}

	/* Disable Bus Mastering, Memory Space and I/O Space responses */
	pci_config_write(card, PCI_COMMAND, 2, 0x0, 0x7);

	uninit_mem_mapped_regs(&((vortex_hw*)card->hw)->regs);

	/* Clean up */
	free(card->hw);

	return B_OK;
}


static void vortex_StartPcm(sound_card_t *card)
{
	{/* Enable DMA end-of-buffer interrupts */
		PciInterruptSCReg reg;
		reg.dwValue = 0x0;
		reg.Bits.DMAEndOfBuffer = ENABLE; //set this bit
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(pci.icr), reg.dwValue, reg.dwValue);
	}
	/*****************************************
	* Set up the VDB Arbiter Work-To-Do List *
	******************************************/

	/* Connect: FIFO -> SRC -> CODEC for playback*/

	{// Stereo route FIFO -> SRC (see 5.3.6) for playback
		VDBRamLink reg;
		
		// Set list terminator 
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_NULL;
		reg.Bits.DestAddr = VDB_DEST_NULL; 	// terminate the list 
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_SRC + RIGHT_CH]), 
						reg.dwValue, 0xffffffff);
						
		/* Write the rigth route to ram index = LDST_ADDR. It is the tail of the list */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_FIFO + PLAY_VDB_CHANNEL;
		reg.Bits.DestAddr = VDB_DEST_SRC + RIGHT_CH; 	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_SRC + LEFT_CH]), 
						reg.dwValue, 0xffffffff); // terminate the list 
		
		/* Write the left route to ram index = header_ram_offset + SR. It is now the header of thr list */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_FIFO + PLAY_VDB_CHANNEL;
		reg.Bits.DestAddr = VDB_DEST_SRC + LEFT_CH; 	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(vdb.HeadLink[VDB_SR_SRC0]), 
						reg.dwValue, 0xffffffff); 
	}

	{//Stereo route SRC -> CODEC for playback and CODEC -> SRC for capture
		VDBRamLink reg;


		/* Terminator */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_NULL;
		reg.Bits.DestAddr = VDB_DEST_NULL; 
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_SRC + REC_SRC_CHANNEL + RIGHT_CH]), 
						reg.dwValue, 0xffffffff);

		/* CODEC OUT 1 -> SRC 3 */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_CODEC + RIGHT_CH;
		reg.Bits.DestAddr = VDB_DEST_SRC + REC_SRC_CHANNEL + RIGHT_CH; 	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_SRC + REC_SRC_CHANNEL + LEFT_CH]), 
						reg.dwValue, 0xffffffff); 

		/* CODEC OUT 0 -> SRC 2 */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_CODEC + LEFT_CH;
		reg.Bits.DestAddr = VDB_DEST_SRC + REC_SRC_CHANNEL + LEFT_CH; 
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_CODEC + RIGHT_CH]), 
						reg.dwValue, 0xffffffff);


		/* Write the rigth route to ram index = LDST_ADDR. It is the tail of the list */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_SRC + RIGHT_CH;
		reg.Bits.DestAddr = VDB_DEST_CODEC + RIGHT_CH; 	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(vdb.DestLink[VDB_DEST_CODEC + LEFT_CH]), 
						reg.dwValue, 0xffffffff); // terminate the list 
		
		/* Write the left route to ram index = header_ram_offset + SR. It is now the header of thr list */
		reg.dwValue = 0x0;
		reg.Bits.SrcAddr = VDB_SRC_SRC + LEFT_CH;
		reg.Bits.DestAddr = VDB_DEST_CODEC + LEFT_CH; 	
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(vdb.HeadLink[VDB_SR_CODEC]), 
						reg.dwValue, 0xffffffff); 
	}

		/*****************************************
		* Set up the VDB Arbiter Work-To-Do List *
		******************************************/

		// Stereo connect: CODEC -> SRC -> FIFO (see 5.3.6)
#if 0
		{// SR_SOURCE = CODEC (48000)
			VDBRamLink reg;
	
			/* Terminator */
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_NULL;
			reg.Bits.DestAddr = VDB_DEST_NULL; 
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.DestLink[VDB_DEST_SRC + REC_SRC_CHANNEL + RIGHT_CH]), 
							reg.dwValue, 0xffffffff);
	
			/* CODEC OUT 1 -> SRC 3 */
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_CODEC + RIGHT_CH;
			reg.Bits.DestAddr = VDB_DEST_SRC + REC_SRC_CHANNEL + RIGHT_CH; 	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.DestLink[VDB_DEST_SRC + REC_SRC_CHANNEL + LEFT_CH]), 
							reg.dwValue, 0xffffffff); 
	
			/* CODEC OUT 0 -> SRC 2 */
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_CODEC + LEFT_CH; 
			reg.Bits.DestAddr = VDB_DEST_SRC + REC_SRC_CHANNEL + LEFT_CH;	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.HeadLink[VDB_SR_CODEC]), 
							reg.dwValue, 0xffffffff);
		}
#endif
		{// SR_SOURCE = SRC2 (44100)
			VDBRamLink reg;
			
			// Terminator 
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_NULL;
			reg.Bits.DestAddr = VDB_DEST_NULL; 	 
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.DestLink[VDB_DEST_FIFOA + REC_VDB_CHANNEL]), 
							reg.dwValue, 0xffffffff);
	
			/* SRC 3 -> FIFO_ALIAS 1 */
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_SRC + REC_SRC_CHANNEL + RIGHT_CH;
			reg.Bits.DestAddr = VDB_DEST_FIFOA + REC_VDB_CHANNEL; 	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.DestLink[VDB_DEST_FIFO + REC_VDB_CHANNEL]), 
							reg.dwValue, 0xffffffff); 
	
			/* SRC 2 -> FIFO 1 */
			reg.dwValue = 0x0;
			reg.Bits.SrcAddr = VDB_SRC_SRC + REC_SRC_CHANNEL + LEFT_CH;
			reg.Bits.DestAddr = VDB_DEST_FIFO + REC_VDB_CHANNEL; 	
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(vdb.HeadLink[VDB_SR_SRC2]), 
							reg.dwValue, 0xffffffff); 
		}
	


	vortex_PcmStartPlayback(card);
	vortex_PcmStartCapture(card);
	
	{// Start FIFO
		FIFOPointersAndControlReg reg;
		
		reg.dwValue = 0x0;
		reg.Bits.ChannelValid 	= YES;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[PLAY_VDB_CHANNEL]), 
						reg.dwValue, reg.dwValue);
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[REC_VDB_CHANNEL]), 
						reg.dwValue, reg.dwValue);
	}
	{// Set SRC Sample Rate Active reg 
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.SRActive), 
						(0x1 << VDB_SR_CODEC) | (0x1 << VDB_SR_SRC2), 
						(0x1 << VDB_SR_CODEC) | (0x1 << VDB_SR_SRC2));	
	}
	{// Set VDB Arbiter Sample Rate Active reg
		VDBSampleRateActiveReg reg;
		
		reg.dwValue = 0x0;
		reg.Bits.SRC0 = ENABLE;
		reg.Bits.SRC2 = ENABLE;			
		reg.Bits.CODEC = ENABLE;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(vdb.SRActive), 
						reg.dwValue, 0xffffffff);		
	}

}


static void vortex_StopPcm(sound_card_t *card)
{
	{// Stop FIFO
		FIFOPointersAndControlReg reg; 
		
		reg.dwValue = 0x0;
		reg.Bits.ChannelValid = NO;	// disable this channel
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[PLAY_VDB_CHANNEL]), 
						reg.dwValue, reg.dwValue);
	}
	{// Clear SRC Sample Rate Active reg 
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(src.SRActive), 
						0x0, 
						(0x1 << VDB_SR_CODEC));	
	}
	{// Clear VDB ARbiter Sample Rate Active reg
		VDBSampleRateActiveReg mask;
		
		mask.dwValue = 0x0;
		mask.Bits.SRC0 = 1;
		mask.Bits.CODEC = 1;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(vdb.SRActive), 
						0x0, mask.dwValue);		
	}

	spin(10000); // wait 10 msec for all DMA requests to complete
// NOTE: See 3.2.4.3 for more info on how to stop a DMA channel properly
}


static void vortex_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	((vortex_hw*)card->hw)->PcmPlaybackSR = sample_rate;
	((vortex_hw*)card->hw)->PcmSetPlaybackSR = true;
}

static status_t vortex_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** log_addr)
{

	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	
	//DB_PRINTF(("*** vortex_SetPlaybackDmaBuf:  size = %ld\n", size));

	AreaId = allocate_memory("vortex_PlaybackDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for vortex_PlaybackDmaBuf failed!!\n");
		ddprintf("vortex_SetPlaybackDmaBuf: return %ld;\n", AreaId);
		return AreaId;
	}
	if(((vortex_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((vortex_hw*)card->hw)->PcmPlaybackAreaId);
	((vortex_hw*)card->hw)->PcmPlaybackAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

// ?? - we have to find more appropriate solution to pass dma buf_size to GetClocks()
	global_dma_buf_size = size;

	/***************** Program DMA for VDB channel 0 *****************************/	
	{// Set Address regs
		DmaChannelAddressReg reg;
		int i;

		reg.subbuf[0].dwValue = 0x0;
		reg.subbuf[1].dwValue = 0x0;
		reg.subbuf[2].dwValue = 0x0;
		reg.subbuf[3].dwValue = 0x0;
		reg.subbuf[0].Bits.address = (uint32)phys_addr;
		reg.subbuf[1].Bits.address = reg.subbuf[0].Bits.address + size / 4;
		reg.subbuf[2].Bits.address = reg.subbuf[1].Bits.address + size / 4;
		reg.subbuf[3].Bits.address = reg.subbuf[2].Bits.address + size / 4;
		for (i = 0; i < 4; i++) 
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(dma.VDBAddr[PLAY_VDB_CHANNEL].subbuf[i]), 
							reg.subbuf[i].dwValue, 
							0xffffffff
							);
	}
	{/* Set GotoSize Regs */
		DmaChannelSizeAndGotoReg reg;

		reg.dwValue = 0x0;
		/* Sub-buffers 0 and 1 */
		reg.Bits.Subbuf1Size = size / 4 - 1;
		reg.Bits.Subbuf0Size = size / 4 - 1;
		reg.Bits.Subbuf1Next = 2;
		reg.Bits.Subbuf1GotoEna = ENABLE;
		reg.Bits.Subbuf1IntEna = ENABLE;
		reg.Bits.Subbuf0Next = 1;
		reg.Bits.Subbuf0GotoEna = ENABLE;
		reg.Bits.Subbuf0IntEna = DISABLE;	// generate interrupts of half-buffer border only
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBSizeGoto[PLAY_VDB_CHANNEL].sb0_1),
						 reg.dwValue, 0xffffffff);

		/* Sub-buffers 2 and 3 */
		reg.Bits.Subbuf1Next = 0;
		reg.Bits.Subbuf0Next = 3;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBSizeGoto[PLAY_VDB_CHANNEL].sb2_3),
						 reg.dwValue , 0xffffffff);
	}
	{/* Set DMA Mode reg */
		DmaChannelModeReg reg;

		reg.dwValue = 0x0;
		reg.Bits.CurrentPosition = 0;
		reg.Bits.InterruptEnable = DMA_INTERRUPT_ON;
		reg.Bits.ReadWrite = DMA_HOST_TO_VORTEX;
		reg.Bits.DataFormat = DMA_16_BIT_LINEAR;
		reg.Bits.SubbufError = 0;
		reg.Bits.SubbufErrorChk = DISABLE;	//ENABLE;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBMode[PLAY_VDB_CHANNEL]), 
							reg.dwValue, 0xffffffff);
	}
	{/* Set Status Reg */
		DmaStatusReg reg;

		reg.dwValue = 0x0;
		reg.Bits.Channel0SubBuffer = 0;	// set cur sub-buffer 0

#if defined(AU8820)
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.WriteVDBStat), 
						reg.dwValue , 0xc0000000);	// bits [31:30] = dma vdb ch 0
#elif defined(AU8830)		
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBStat[PLAY_VDB_CHANNEL/16]), 
						reg.dwValue , 0xc0000000);	// bits [31:30] = dma vdb ch 0
#endif
	}

	return B_OK;
}

static void vortex_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	if(num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format

	{/* Change DMA Mode Reg */
		DmaChannelModeReg reg;

		reg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs,
										OFFSET(dma.VDBMode[PLAY_VDB_CHANNEL]));
		if (num_of_bits == 8) 
			reg.Bits.DataFormat = DMA_8_BIT_LINEAR;
		else
			reg.Bits.DataFormat = DMA_16_BIT_LINEAR;
		mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(dma.VDBMode[PLAY_VDB_CHANNEL]), 
										reg.dwValue , 0xffffffff);
	}

	{/* Change FIFO Control reg */
		FIFOPointersAndControlReg reg;
		
		reg.dwValue = 0;
		if (num_of_ch == 1) 
			reg.Bits.Stereo	= FIFO_MONO_DATA;
		else
			reg.Bits.Stereo	= FIFO_STEREO_DATA;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[PLAY_VDB_CHANNEL]), 
						reg.dwValue, reg.dwValue);
	}
	
	((vortex_hw*)card->hw)->PcmPlaybackFormat.IsStereo = (num_of_ch == 2);
	((vortex_hw*)card->hw)->PcmSetPlaybackFormat = true;
}


static void vortex_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	((vortex_hw*)card->hw)->PcmCaptureSR = sample_rate;
	((vortex_hw*)card->hw)->PcmSetCaptureSR = true;
}

static status_t vortex_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr)
{
	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;

	AreaId = allocate_memory("vortex_CaptureDmaBuf",
				addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for vortex_CaptureDmaBuf failed!!\n");
		ddprintf("vortex_SetCaptureDmaBuf: return %ld;\n",AreaId);
		return AreaId;
	}
	if(((vortex_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((vortex_hw*)card->hw)->PcmCaptureAreaId);
	((vortex_hw*)card->hw)->PcmCaptureAreaId = AreaId; 
	memset(*addr,0,tmp_size);



//RECORD

// ?? - we have to find more appropriate solution to pass dma buf_size to GetClocks()
//	global_dma_buf_size = size;

	/***************** Program DMA for VDB channel 0 *****************************/	
	{// Set Address regs
		DmaChannelAddressReg reg;
		int i;

		reg.subbuf[0].dwValue = 0x0;
		reg.subbuf[1].dwValue = 0x0;
		reg.subbuf[2].dwValue = 0x0;
		reg.subbuf[3].dwValue = 0x0;
		reg.subbuf[0].Bits.address = (uint32)phys_addr;
		reg.subbuf[1].Bits.address = reg.subbuf[0].Bits.address + size / 4;
		reg.subbuf[2].Bits.address = reg.subbuf[1].Bits.address + size / 4;
		reg.subbuf[3].Bits.address = reg.subbuf[2].Bits.address + size / 4;
		for (i = 0; i < 4; i++) 
			mapped_write_32(&((vortex_hw*)card->hw)->regs, 
							OFFSET(dma.VDBAddr[REC_VDB_CHANNEL].subbuf[i]), 
							reg.subbuf[i].dwValue, 
							0xffffffff
							);
	}
	{/* Set GotoSize Regs */
		DmaChannelSizeAndGotoReg reg;

		reg.dwValue = 0x0;
		/* Sub-buffers 0 and 1 */
		reg.Bits.Subbuf1Size = size / 4 - 1;
		reg.Bits.Subbuf0Size = size / 4 - 1;
		reg.Bits.Subbuf1Next = 2;
		reg.Bits.Subbuf1GotoEna = ENABLE;
		reg.Bits.Subbuf1IntEna = ENABLE;
		reg.Bits.Subbuf0Next = 1;
		reg.Bits.Subbuf0GotoEna = ENABLE;
		reg.Bits.Subbuf0IntEna = DISABLE;	// generate interrupts of half-buffer border only
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(dma.VDBSizeGoto[REC_VDB_CHANNEL].sb0_1),
						reg.dwValue, 0xffffffff);

		/* Sub-buffers 2 and 3 */
		reg.Bits.Subbuf1Next = 0;
		reg.Bits.Subbuf0Next = 3;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, 
						OFFSET(dma.VDBSizeGoto[REC_VDB_CHANNEL].sb2_3),
						reg.dwValue , 0xffffffff);
	}
	{/* Set DMA Mode reg */
		DmaChannelModeReg reg;

		reg.dwValue = 0x0;
		reg.Bits.CurrentPosition = 0;
		reg.Bits.InterruptEnable = DMA_INTERRUPT_ON;
		reg.Bits.ReadWrite = DMA_VORTEX_TO_HOST; // write to memory
		reg.Bits.DataFormat = DMA_16_BIT_LINEAR;
		reg.Bits.SubbufError = 0;
		reg.Bits.SubbufErrorChk = ENABLE;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBMode[REC_VDB_CHANNEL]), 
							reg.dwValue, 0xffffffff);
	}
	{/* Set Status Reg */
		DmaStatusReg reg;

		reg.dwValue = 0x0;
		reg.Bits.Channel1SubBuffer = 0;	// set to start from sub-buffer 0
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(dma.VDBStat[REC_VDB_CHANNEL/16]), 
						reg.dwValue, reg.dwValue);	
	}
	
//end RECORD	


	return B_OK;
}

static void vortex_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	if (num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format

	if(num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format

	{/* Change DMA Mode Reg */
		DmaChannelModeReg reg;

		reg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs,
										OFFSET(dma.VDBMode[REC_VDB_CHANNEL]));
		if (num_of_bits == 8) 
			reg.Bits.DataFormat = DMA_8_BIT_LINEAR;
		else
			reg.Bits.DataFormat = DMA_16_BIT_LINEAR;
		mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(dma.VDBMode[REC_VDB_CHANNEL]), 
										reg.dwValue , 0xffffffff);
	}

	{/* Change FIFO Control reg */
		FIFOPointersAndControlReg reg;
		
		reg.dwValue = 0;
		if (num_of_ch == 1) 
			reg.Bits.Stereo	= FIFO_MONO_DATA;
		else
			reg.Bits.Stereo	= FIFO_STEREO_DATA;
		mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(fifo.VDBPtrCtl[REC_VDB_CHANNEL]), 
						reg.dwValue, reg.dwValue);
	}
	
	((vortex_hw*)card->hw)->PcmCaptureFormat.IsStereo = (num_of_ch == 2);
	((vortex_hw*)card->hw)->PcmSetCaptureFormat = true;

}




static uint16 vortex_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	int count;
	uint16 ret = AC97_BAD_VALUE;

	SERCodecIntCtlReg	rCodecIntfCtrlReg;
	SERCmdStatReg		rCodecCmdStatReg;
	


	// wait for Command Write OK bit
	for (count = 400; count > 0; count--) {
		// check Command Write OK bit
		rCodecIntfCtrlReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl));
		if (rCodecIntfCtrlReg.Bits.CmdWrtOK == 1)
			break;
		spin(100);
	}
	if(!count) return ret;
	
	// set Aspen Codec Command Status Register
    rCodecCmdStatReg.dwValue = 0;
    rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatAddr	= offset & 0x7F;
    rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatRWStat = 0;
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCmdStat),rCodecCmdStatReg.dwValue,0xffffffff);

	for (count = 400; count > 0; count--) {
		// read specified AC97 register
		rCodecCmdStatReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCmdStat));	
		if ((rCodecCmdStatReg.dwValue & 0x0FFF0000) == (unsigned)((1<<23)|((offset & 0x7F) << 16)))
		{
			ret = rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatData;
			break;
		}
		spin(100);
	}
	return ret;
}


static uint16 vortex_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(((vortex_hw*)card->hw)->codec.lock));

	ret = vortex_CodecReadUnsecure(card, offset);
	
	release_spinlock(&(((vortex_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);

	return ret;
}

static status_t vortex_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	sound_card_t *card = (sound_card_t *)host;
	int count;
	status_t ret = B_ERROR;
	cpu_status cp;
	SERCodecIntCtlReg	rCodecIntfCtrlReg;
	SERCmdStatReg		rCodecCmdStatReg;

	if (mask == 0) return B_OK;
	
	cp = disable_interrupts();
	acquire_spinlock(&(((vortex_hw*)card->hw)->codec.lock));

	if (mask != 0xffff) {
		uint16 old  = vortex_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}

	for (count = 400; count > 0; count--) {
		// check Command Write OK bit
		rCodecIntfCtrlReg.dwValue = mapped_read_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl));
		if (rCodecIntfCtrlReg.Bits.CmdWrtOK == 1) break;
		spin(100);
	}
	if(count)
	{
		ret = B_OK;
		// set Codec Command Status Register
		rCodecCmdStatReg.dwValue = 0;
		rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatAddr	= offset & 0x7F;
		rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatRWStat = 1;
		rCodecCmdStatReg.CtlStat.AC97.Bits.AC97CmdStatData	= value;
		mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCmdStat),rCodecCmdStatReg.dwValue,0xffffffff);
	}
	release_spinlock(&(((vortex_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);
	return ret;
}


static status_t vortex_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;
	ddprintf("vortex_InitCodec(): here!\n");

#if defined(AU8820)
	// disable clock first
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x00a8, 0xffffffff);	// 32'b0000_0000_1010_1000
	spin(1000);	

	// place codec into reset
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x80a8, 0xffffffff);	// 32'b1000_0000_1010_1000
	spin(1000);	

	// give codec some clocks with reset asserted
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x80e8, 0xffffffff);	// 32'b0000_0000_1110_1000
	spin(1000);	

	// turn off clocks
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x80a8, 0xffffffff);	// 32'b1000_0000_1010_1000
	spin(1000);	

	// release reset
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x00a8, 0xffffffff);	// 32'b0000_0000_1010_1000
	spin(1000);	

	// turn on clocks
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x00e8, 0xffffffff);	// 32'b0000_0000_1110_1000
	spin(1000);

#elif defined(AU8830) || defined(AU8810)

	//do cold reset
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x8068, 0xffffffff);	// 32'b1000_0000_0110_1000
	spin(1000);	

	// and turn on CCLK
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x00e8, 0xffffffff);	//32'b0000_0000_1110_1000
	spin(1000);	
#endif
//	{// init Codec Channel data
//		uchar i;
//		for (i = 0; i < 32; i++) 
//			mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecChannelData[i]), (uint32) -i, 0xffffffff);	//32'b0000_0000_1110_1000
//		for (i = 0; i < 32; i++) 
//			DB_PRINTF(("CodecChannelData[%d] = 0x%lx, 0x%lx\n",i, mapped_read_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecChannelData[i])),(uint32) -i ));
//	}
	// turn on CCLK
	mapped_write_32(&((vortex_hw*)card->hw)->regs,OFFSET(ser.CodecCtl), 0x00e8, 0xffffffff);	//32'b0000_0000_1110_1000
	spin(1000);

	// enable the codec channel 0 & 1
	mapped_write_32(&((vortex_hw*)card->hw)->regs, OFFSET(ser.ChannelEnable),0x0300,0x0300);

	err = ac97init(&((vortex_hw*)card->hw)->codec, 
					(void*)card,
					vortex_CodecWrite,
					vortex_CodecRead);

	ddprintf("vortex_InitCodec(): ac97init err = %ld!\n",err);
	return err;				
}

static void vortex_UninitCodec(sound_card_t *card)
{
	ac97uninit(&((vortex_hw*)card->hw)->codec); 
}

static status_t vortex_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	return AC97_SoundSetup(&((vortex_hw*)card->hw)->codec, ss);
}

static status_t  vortex_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ddprintf("vortex_GetSoundSetup: ss = 0x%lx\n",(uint32)ss);
	ret = AC97_GetSoundSetup(&((vortex_hw*)card->hw)->codec,ss);
	ss->sample_rate = kHz_44_1;
	return ret;
}





static uchar mpu401_read_port(void* hw, int port)
{
	return mapped_read_8( &(((vortex_hw*)hw)->regs), (port == 0)?(OFFSET(game.MidiData)):(OFFSET(game.CmdStat)));
}

static void mpu401_write_port(void* hw, int port, uchar value)
{
	mapped_write_8( &(((vortex_hw*)hw)->regs), (port == 0)?(OFFSET(game.MidiData)):(OFFSET(game.CmdStat)), value, 0xff);
}

static uchar joy_read_port(void* hw)
{
	return mapped_read_8( &(((vortex_hw*)hw)->regs), OFFSET(game.Joystick));
}

static void joy_write_port(void* hw, uchar value)
{
	mapped_write_8( &(((vortex_hw*)hw)->regs), OFFSET(game.Joystick),value,0xff);
}






