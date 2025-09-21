
/************************************************************************/
/*                                                                      */
/*                              null_audio.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov										*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>

#include "audio_drv.h"
#include "misc.h"
#include "debug.h"
#include <ac97_module.h>
#include "audio_drv.h"


static uint GenerateRate = 1000;


typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;


typedef struct my_timer
{
	timer t;
	void* host;
}my_timer;



typedef struct _null_hw {
	timers_data_t timers_data;	
	sound_setup ss;
	int32 Lock;	

	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	struct{bool IsStereo; bool Is16bits;} PcmPlaybackFormat;
	uint32 PcmPlaybackSR;
	my_timer PlaybackTimer;
	int PlaybackHalf;

	area_id PcmCaptureAreaId;
	size_t PcmCaptureBufSize;
	void* PcmCaptureBufLogAddr; 
	struct{bool IsStereo; bool Is16bits;} PcmCaptureFormat;
	uint32 PcmCaptureSR;
	my_timer CaptureTimer;
	int CaptureHalf;
	uint32 SamplesCaptured;
} null_hw; 


static status_t 	null_Init(sound_card_t * card);
static status_t 	null_Uninit(sound_card_t * card);
static void 		null_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	null_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		null_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		null_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		null_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		null_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		null_StartPcm(sound_card_t *card);
static void 		null_StopPcm(sound_card_t *card);
static status_t		null_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		null_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		null_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		null_InitJoystick(sound_card_t * card);
static void 		null_enable_gameport(sound_card_t * card);
static void 		null_disable_gameport(sound_card_t * card);

static status_t 	null_InitMPU401(sound_card_t * card);
static void 		null_enable_mpu401(sound_card_t *card);
static void 		null_disable_mpu401(sound_card_t *card);
static void 		null_enable_mpu401_interrupts(sound_card_t *card);
static void 		null_disable_mpu401_interrupts(sound_card_t *card);






static sound_card_functions_t null_audio_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&null_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&null_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&null_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&null_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&null_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&null_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&null_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&null_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&null_SetPlaybackDmaBuf,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&null_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&null_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&null_SetCaptureDmaBuf,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&null_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&null_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&null_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&null_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&null_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&null_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&null_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&null_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&null_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
} ;



card_descrtiptor_t null_audio_descr = {
	"null",
	NO_HW, 
	&null_audio_functions,
	0x0, 
	0x0,
	0x0
};




 



static int32 PlaybackIsr(timer* timer)
{
	status_t ret = B_HANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *)(((my_timer*)timer)->host);
	{// Store internal time for synchronization 
		bigtime_t time = system_time();
		
		acquire_spinlock(&((null_hw*)card->hw)->timers_data.Lock);	
		((null_hw*)card->hw)->timers_data.SystemClock = time;
		((null_hw*)card->hw)->timers_data.IntCount++;
		release_spinlock(&((null_hw*)card->hw)->timers_data.Lock);
	}			
	((null_hw*)card->hw)->PlaybackHalf++;
	((null_hw*)card->hw)->PlaybackHalf %=2;
	// Proceed playback interrupt
	if ((*card->pcm_playback_interrupt)(card, ((null_hw*)card->hw)->PlaybackHalf)) 
		ret = B_INVOKE_SCHEDULER;

	return ret;
}




static int32 CaptureIsr(timer* timer)
{
	status_t ret = B_HANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *)(((my_timer*)timer)->host);
	static int32 CaptureIsrCount = 0; 

	((null_hw*)card->hw)->CaptureHalf++;
	((null_hw*)card->hw)->CaptureHalf %=2;
	{
		int i;
		//static uint GenerateRate = 1000;
		uint period = ((null_hw*)card->hw)->PcmCaptureSR/GenerateRate;
		uchar *p = (uchar*)(((null_hw*)card->hw)->PcmCaptureBufLogAddr)+((null_hw*)card->hw)->CaptureHalf*((null_hw*)card->hw)->PcmCaptureBufSize/2;
		uchar SampleSize = 1;
 		if(((null_hw*)card->hw)->PcmCaptureFormat.IsStereo) SampleSize *=2;
 		if(((null_hw*)card->hw)->PcmCaptureFormat.Is16bits) SampleSize *=2;
 		for(i=0;i<((null_hw*)card->hw)->PcmCaptureBufSize/2;i++)
 		{
 			*p = 0x7f*( ( ((null_hw*)card->hw)->SamplesCaptured % period)/(period/2));
 			p++;
 			if(i%SampleSize == 0)((null_hw*)card->hw)->SamplesCaptured++;
 		}
	}
	// Proceed playback interrupt
	if ((*card->pcm_capture_interrupt)(card, ((null_hw*)card->hw)->CaptureHalf)) 
		ret = B_INVOKE_SCHEDULER;
	return ret;
}


#if 0
static int32 null_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;

	{//Playback
		uint8 status;
		acquire_spinlock(&card->bus.pci.lock);
		status = pci->read_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x00);
		if((status & 0x1) == 0) release_spinlock(&card->bus.pci.lock);
		else
		{
			pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x00,status | ~0x1);
			release_spinlock(&card->bus.pci.lock);
			ret = B_HANDLED_INTERRUPT;
			{// Store internal time for synchronization 
				bigtime_t time = system_time();
				
				acquire_spinlock(&((null_hw*)card->hw)->timers_data.Lock);	
				((null_hw*)card->hw)->timers_data.SystemClock = time;
				((null_hw*)card->hw)->timers_data.IntCount++;
				release_spinlock(&((null_hw*)card->hw)->timers_data.Lock);
			}			
			{
				//Determinate current block (half_buffer)
				int half;
				uint32 CurrentSGDTableAddr = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x4);
				uint32 SGDCurrentCount = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0xc);
//				if(CurrentSGDTableAddr == (uint32)((null_hw*)card->hw)->SGDTablePhysAddr)
//					half = 1;
				if(CurrentSGDTableAddr == (uint32)(((null_hw*)card->hw)->SGDTablePhysAddr + sizeof(SGD_Table_t)))
					if(SGDCurrentCount<0x400)
					{
					 	half = 0;
					 	atomic_add(&b00count,1);
					}	
					else
					{
						half =1;
					 	atomic_add(&b01count,1);
					}
				else if(SGDCurrentCount<0x400)
				{
					half = 1;
				 	atomic_add(&b10count,1);
				}
				else
				{
					half =0;
				 	atomic_add(&b11count,1);
				}

				// Proceed playback interrupt
				if ((*card->pcm_playback_interrupt)(card, half)) 
					ret = B_INVOKE_SCHEDULER;

			}
		}
	}
	{//Capture
		uint8 status;
		acquire_spinlock(&card->bus.pci.lock);
		status = pci->read_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x10);
		if((status & 0x1) == 0) release_spinlock(&card->bus.pci.lock);
		else
		{
			pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x10,status | ~0x1);
			release_spinlock(&card->bus.pci.lock);
			ret = B_HANDLED_INTERRUPT;
			{
				//Determinate current block (half_buffer)
				int half;
				uint32 CurrentSGDTableAddr = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x14);
				uint32 SGDCurrentCount = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x1c);
				if(CurrentSGDTableAddr == (uint32)(((null_hw*)card->hw)->SGDTablePhysAddr + 3*sizeof(SGD_Table_t)))
					if(SGDCurrentCount<0x400)
					{
					 	half = 0;
					 	atomic_add(&b00count,1);
					}	
					else
					{
						half =1;
					 	atomic_add(&b01count,1);
					}
				else if(SGDCurrentCount<0x400)
				{
					half = 1;
				 	atomic_add(&b10count,1);
				}
				else
				{
					half =0;
				 	atomic_add(&b11count,1);
				}

				// Proceed playback interrupt
				if ((*card->pcm_capture_interrupt)(card, half)) 
					ret = B_INVOKE_SCHEDULER;

			}
		}
	}
	return ret;
}
#endif


static void null_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((null_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((null_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((null_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((null_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = ((null_hw*)card->hw)->PcmPlaybackBufSize 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((null_hw*)card->hw)->PcmPlaybackSR) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
}

static void null_enable_mpu401(sound_card_t *card)
{
}

static void null_disable_mpu401(sound_card_t *card)
{
}

static void null_enable_mpu401_interrupts(sound_card_t *card)
{
}

static void null_disable_mpu401_interrupts(sound_card_t *card)
{
}


static void null_enable_gameport(sound_card_t * card)
{
}

static void null_disable_gameport(sound_card_t * card)
{
}

static status_t null_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static status_t null_InitMPU401(sound_card_t * card)
{
	return ENODEV;
}


static status_t null_PcmStartPlayback(sound_card_t * card)
{
	cpu_status cp;
	bigtime_t period;

	DB_PRINTF(("null_PcmStartPlayback\n"));

	period = ((null_hw*)card->hw)->PcmPlaybackBufSize/2;
	if(((null_hw*)card->hw)->PcmPlaybackFormat.IsStereo) period /= 2;
	if(((null_hw*)card->hw)->PcmPlaybackFormat.Is16bits) period /= 2;
	period = (bigtime_t)(((double)period)*1000000.0/((null_hw*)card->hw)->PcmPlaybackSR); 

	DB_PRINTF(("Playback interrupt period = %ld\n",(uint32)period));
	
	cp = disable_interrupts();
	acquire_spinlock(&((null_hw*)card->hw)->Lock);
	
    add_timer((timer*)(&((null_hw*)card->hw)->PlaybackTimer), PlaybackIsr, period, B_PERIODIC_TIMER);
	release_spinlock(&((null_hw*)card->hw)->Lock);
	restore_interrupts(cp);

	return B_OK;
}

static status_t null_PcmStartCapture(sound_card_t * card)
{
	cpu_status cp;
	bigtime_t period;

	DB_PRINTF(("null_PcmStartCapture\n"));

	period = ((null_hw*)card->hw)->PcmCaptureBufSize/2;
	if(((null_hw*)card->hw)->PcmCaptureFormat.IsStereo) period /= 2;
	if(((null_hw*)card->hw)->PcmCaptureFormat.Is16bits) period /= 2;
	period = (bigtime_t)(((double)period)*1000000.0/((null_hw*)card->hw)->PcmCaptureSR); 

	DB_PRINTF(("Capture interrupt period = %ld\n",(uint32)period));

	cp = disable_interrupts();
	acquire_spinlock(&((null_hw*)card->hw)->Lock);

    add_timer((timer*)(&((null_hw*)card->hw)->CaptureTimer), CaptureIsr, period, B_PERIODIC_TIMER);

	release_spinlock(&((null_hw*)card->hw)->Lock);
	restore_interrupts(cp);

	return B_OK;
}



static status_t null_Init(sound_card_t * card)
{
	status_t err = B_OK;

	//DB_PRINTF(("null_Init(): here! \n"));
	
	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(null_hw));
	if(!card->hw) 
	{
		derror("null_Init(): malloc error!\n");
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(null_hw));	// zero it out

	/* Some initialization */
	((null_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((null_hw*)card->hw)->PcmPlaybackAreaId = -1; 

	((null_hw*)card->hw)->PlaybackTimer.host = card;
	((null_hw*)card->hw)->CaptureTimer.host	=card;

	return B_OK;
}


static status_t null_Uninit(sound_card_t * card)
{
	//DB_PRINTF(("null_Uninit(): here! \n"));

	if(((null_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((null_hw*)card->hw)->PcmCaptureAreaId);
	if(((null_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((null_hw*)card->hw)->PcmPlaybackAreaId);

	/* Clean up */
	free(card->hw);

	return B_OK;
}


static void null_StartPcm(sound_card_t *card)
{
	null_PcmStartPlayback(card);
	null_PcmStartCapture(card);
}


static void null_StopPcm(sound_card_t *card)
{
	cpu_status cp;
	DB_PRINTF(("null_PcmStopPcm\n"));

	cp = disable_interrupts();
	acquire_spinlock(&((null_hw*)card->hw)->Lock);

	cancel_timer((timer*)(&((null_hw*)card->hw)->PlaybackTimer));
	cancel_timer((timer*)(&((null_hw*)card->hw)->CaptureTimer));

	release_spinlock(&((null_hw*)card->hw)->Lock);
	restore_interrupts(cp);
}


static void null_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	((null_hw*)card->hw)->PcmPlaybackSR = sample_rate;
}

static status_t null_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** log_addr)
{

	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	DB_PRINTF(("In null_SetPlaybackDmaBuf\n"));

	AreaId = allocate_memory("null_PlaybackDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for null__PlaybackDmaBuf failed!!\n");
		ddprintf("vortex_SetPlaybackDmaBuf: return %ld;\n", AreaId);
		return AreaId;
	}
	if(((null_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((null_hw*)card->hw)->PcmPlaybackAreaId);
	((null_hw*)card->hw)->PcmPlaybackAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

	((null_hw*)card->hw)->PcmPlaybackBufSize = size;


	DB_PRINTF(("null_SetPlaybackDmaBuf OK\n"));
	return B_OK;
}

static void null_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uchar stereo;
	uchar bits16;
	if(num_of_bits==8) bits16 = 0x0; else bits16 = 0x1; 
	if (num_of_ch != 2)stereo = 0x0;  else stereo =0x1;

	((null_hw*)card->hw)->PcmPlaybackFormat.IsStereo = stereo; 
	((null_hw*)card->hw)->PcmPlaybackFormat.Is16bits = bits16; 


	DB_PRINTF(("null_SetPlaybackFormat\n"));

}


static void null_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	((null_hw*)card->hw)->PcmCaptureSR = sample_rate;
}

static status_t null_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** log_addr)
{
	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	DB_PRINTF(("In null_SetCaptureDmaBuf\n"));

	AreaId = allocate_memory("null_CaptureDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for null__PlaybackDmaBuf failed!!\n");
		ddprintf("vortex_SetPlaybackDmaBuf: return %ld;\n", AreaId);
		return AreaId;
	}
	if(((null_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((null_hw*)card->hw)->PcmCaptureAreaId);
	((null_hw*)card->hw)->PcmCaptureAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);


	((null_hw*)card->hw)->PcmCaptureBufSize = size;
	((null_hw*)card->hw)->PcmCaptureBufLogAddr = *log_addr;

	DB_PRINTF(("null_SetCaptureDmaBuf OK\n"));
	return B_OK;
}

static void null_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uchar stereo;
	uchar bits16;
	
	if(num_of_bits==8) bits16 = 0x0; else bits16 = 0x1; 
	if (num_of_ch != 2)stereo = 0x0;  else stereo =0x1;

	((null_hw*)card->hw)->PcmCaptureFormat.IsStereo = stereo; 
	((null_hw*)card->hw)->PcmCaptureFormat.Is16bits = bits16; 

	DB_PRINTF(("null_SetCaptureFormat\n"));

}




static status_t null_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	((null_hw*)card->hw)->ss = *ss;	
	return B_OK;
}

static status_t  null_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	*ss = ((null_hw*)card->hw)->ss;	
	return B_OK;
}









