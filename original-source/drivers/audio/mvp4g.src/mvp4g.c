/************************************************************************/
/*                                                                      */
/*                              mvp4g.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov										*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

//#define DEBUG
#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>

#include "audio_drv.h"
#include "mvp4_debug.h"
#include "ac97.h"

#include "buffer.h"
#include "stream.h"
#include "SGD.h"
#include "queue.h"


#define AC97_BAD_VALUE		(0xffff)


extern pci_module_info *pci;

typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;

typedef struct _mvp4g_hw {
	timers_data_t timers_data;	
	ac97_t codec;

	size_t PcmPlaybackBufSize;
	struct{bool IsStereo; bool Is16bits;} PcmPlaybackFormat;
	uint32 PcmPlaybackSR;

	struct{bool IsStereo; bool Is16bits;} PcmCaptureFormat;
	uint32 PcmCaptureSR;

} mvp4g_hw; 


static status_t 	mvp4g_Init(sound_card_t * card);
static status_t 	mvp4g_Uninit(sound_card_t * card);
static void 		mvp4g_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	mvp4g_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void* addr);
static void 		mvp4g_SetPlaybackFormat(sound_card_t *card, uint32 format, uint16 channel_count);
static void 		mvp4g_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		mvp4g_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		mvp4g_SetCaptureFormat(sound_card_t *card, uint32 format, uint16 channel_count);


static status_t		mvp4g_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		mvp4g_GetSoundSetup(sound_card_t *card, sound_setup *ss);

static void 		mvp4g_GetClocks(sound_card_t * card, float*	cur_frame_rate,	uint32*	frames_played, bigtime_t* pSystemClock);

static status_t		mvp4g_InitJoystick(sound_card_t * card);
static void 		mvp4g_enable_gameport(sound_card_t * card);
static void 		mvp4g_disable_gameport(sound_card_t * card);

static status_t 	mvp4g_InitMPU401(sound_card_t * card);
static void 		mvp4g_enable_mpu401(sound_card_t *card);
static void 		mvp4g_disable_mpu401(sound_card_t *card);
static void 		mvp4g_enable_mpu401_interrupts(sound_card_t *card);
static void 		mvp4g_disable_mpu401_interrupts(sound_card_t *card);

static  void  		mvp4g_PrepareStart(sound_card_t *card);
static 	void 		mvp4g_PcmStartPlayback(sound_card_t * card);
static 	void 		mvp4g_PcmStartCapture(sound_card_t * card);
static 	void 		mvp4g_StopCapturePcm(sound_card_t *card);
static 	void 		mvp4g_StopPlaybackPcm(sound_card_t *card);
static  void        mvp4g_Re_StartPlayback(sound_card_t *card);
static  void        mvp4g_Re_StartCapture(sound_card_t *card);
static void         mvp4g_PausePlayback(sound_card_t *card);
static void         mvp4g_UnpauseCapture(sound_card_t *card);
static void         mvp4g_UnpausePlayback(sound_card_t *card);
static void         mvp4g_PauseCapture(sound_card_t *card);
static void         mvp4g_GDT_playback_setup(sound_card_t *card, uint32 address);
static void         mvp4g_GDT_capture_setup(sound_card_t *card, uint32 address);
static status_t     mvp4g_SetSound(sound_card_t *card, int16 types, int16 value);
static status_t     mvp4g_SetSampleRate(sound_card_t *card, uint16 rate);

static sound_card_functions_t mvp4g_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&mvp4g_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&mvp4g_Uninit,

NULL,
NULL,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&mvp4g_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&mvp4g_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&mvp4g_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&mvp4g_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&mvp4g_SetPlaybackDmaBuf,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&mvp4g_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&mvp4g_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&mvp4g_SetCaptureDmaBuf,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&mvp4g_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&mvp4g_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&mvp4g_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&mvp4g_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&mvp4g_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&mvp4g_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&mvp4g_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&mvp4g_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&mvp4g_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL, // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
NULL,
NULL,

&mvp4g_PcmStartPlayback,
&mvp4g_PcmStartCapture,
&mvp4g_StopPlaybackPcm,
&mvp4g_StopCapturePcm,
&mvp4g_Re_StartPlayback,
&mvp4g_Re_StartCapture,
&mvp4g_PausePlayback,
&mvp4g_PauseCapture,
&mvp4g_UnpausePlayback,
&mvp4g_UnpauseCapture,
&mvp4g_GDT_playback_setup,
&mvp4g_GDT_capture_setup,
&mvp4g_SetSound,
&mvp4g_SetSampleRate
} ;



card_descrtiptor_t mvp4g_audio_descr = {
	"mvp4g",
	PCI, 
	&mvp4g_functions,
	0x1106, //mvp4g_VENDOR_ID, 
	0x3058 // mvp4g AUDIO CODEC DEVICE_ID,
};

 
static status_t 	mvp4g_InitCodec(sound_card_t *card);
static void 		mvp4g_UninitCodec(sound_card_t *card);
static uint16 		mvp4g_CodecRead(void* host, uchar offset);
static status_t 	mvp4g_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);

/* --- 
	Interrupt service routine
	---- */

static int32 mvp4g_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;
	
    //DB_PRINTF(("mvp4g_isr\n"));
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
				
				acquire_spinlock(&((mvp4g_hw*)card->hw)->timers_data.Lock);	
				((mvp4g_hw*)card->hw)->timers_data.SystemClock = time;
				((mvp4g_hw*)card->hw)->timers_data.IntCount++;
				release_spinlock(&((mvp4g_hw*)card->hw)->timers_data.Lock);
			}
						
			{
				// Proceed playback interrupt
				if ((*card->pcm_playback_interrupt)(card)) 
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
				
				// Proceed playback interrupt
				if ((*card->pcm_capture_interrupt)(card)) 
					ret = B_INVOKE_SCHEDULER;

			}
		}
	}
	return ret;
}

static void mvp4g_GetClocks(sound_card_t * card,
							float*	cur_frame_rate,
							uint32*	frames_played,
							bigtime_t* pSystemClock)
{
    cpu_status cp;
    static bigtime_t  LastSysClock = 0;
	uint32 num_samples_per_buf;
 	double buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((mvp4g_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((mvp4g_hw*)card->hw)->timers_data.SystemClock;	// system time
		
	int_count = ((mvp4g_hw*)card->hw)->timers_data.IntCount;			// interrupt count
    
    *frames_played = (uint32) int_count;
    
	release_spinlock(&((mvp4g_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in buffer (in bytes)
	num_samples_per_buf = ((mvp4g_hw*)card->hw)->PcmPlaybackBufSize 
								/2 		//16-bit format of samples
								/2;		//stereo
								
	if(LastSysClock == 0 )
	{//first time: init
		LastSysClock = *pSystemClock;
	}
	else
	{
		buf_time = *pSystemClock - LastSysClock;
		LastSysClock = *pSystemClock;
	}							
								
	*cur_frame_rate = (float)(num_samples_per_buf/(buf_time * 1000000.0));						
}


/*
 	{//debug
		static bigtime_t LastSampleClock = 0, LastSysClock = 0;
		if(LastSysClock == 0 && LastSampleClock == 0)
		{//first time: init
			LastSysClock = *pSystemClock;
			LastSampleClock = *pSampleClock;
		}
		else if(*pSystemClock-LastSysClock>=10000000)
		{
				dprintf("CPU clocks = %ld, SampleClock = %ld\n",
					(uint32)(*pSystemClock-LastSysClock),(uint32)(*pSampleClock-LastSampleClock));
				LastSysClock = *pSystemClock;
				LastSampleClock = *pSampleClock;
		}
 	}
*/



static void mvp4g_enable_mpu401(sound_card_t *card)
{
}

static void mvp4g_disable_mpu401(sound_card_t *card)
{
}

static void mvp4g_enable_mpu401_interrupts(sound_card_t *card)
{
}

static void mvp4g_disable_mpu401_interrupts(sound_card_t *card)
{
}


static void mvp4g_enable_gameport(sound_card_t * card)
{
}

static void mvp4g_disable_gameport(sound_card_t * card)
{
}

static status_t mvp4g_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static status_t mvp4g_InitMPU401(sound_card_t * card)
{
	return ENODEV;
}


static void mvp4g_PcmStartPlayback(sound_card_t * card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4g_PcmStartPlayback\n"));
	((mvp4g_hw*)card->hw)->timers_data.IntCount = 0;
 	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x01,	(0x1<<7));
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x00, (0x1<<2)); //write 1 to resume transfer
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void mvp4g_PcmStartCapture(sound_card_t * card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4g_PcmStartCapture\n"));
	((mvp4g_hw*)card->hw)->timers_data.IntCount = 0;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11,	(0x1<<7));
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x10,(0x1<<2)); //write 1 to resume transfer
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}



static void mvp4g_StopPlaybackPcm(sound_card_t *card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4g_PcmStopPlaybackPcm\n"));
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	/*terminate playback*/
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x01,	(0x1<<6));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void mvp4g_StopCapturePcm(sound_card_t *card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4g_PcmStopCapturePcm\n"));
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	/*terminate capture*/
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11,	(0x1<<6));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static status_t mvp4g_Init(sound_card_t * card)
{
	status_t err = B_OK;
	uint16 playback_entry;
	
	DB_PRINTF(("mvp4g_Init(): here! \n"));
	
	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(mvp4g_hw));
	if(!card->hw) 
	{
		DB_PRINTF(("mvp4g_Init(): malloc error!\n"));
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(mvp4g_hw));	// zero it out

	buffer_init();
	DB_PRINTF(("buffer_init : done!\n"));
	stream_init();
	DB_PRINTF(("stream_init : done!\n"));
	if(init_SGD(card) < 0)
	{
		DB_PRINTF(("init_SGD : did not finish properly\n"));
	}
	else
	{	 
		DB_PRINTF(("init_SGD : done!\n"));
	}
		   
	/* Init codec */
	if((err = mvp4g_InitCodec(card))) 
	{
		 DB_PRINTF(("mvp4g_Init: mvp4g_InitCodec error!\n"));
		 return err;
	}

	/* Install mvp4g isr */
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, mvp4g_isr, card, 0);

	return B_OK;
}

static status_t mvp4g_Uninit(sound_card_t * card)
{
	DB_PRINTF(("mvp4g_Uninit(): here! \n"));

	mvp4g_StopCapturePcm(card);
    mvp4g_StopPlaybackPcm(card);
    
	/* Remove mvp4g isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, mvp4g_isr, card);

	/* Uninit codec */
	mvp4g_UninitCodec(card);
	
	uninit_SGD();
	DB_PRINTF(("completed uninit_SGD\n"));
	stream_uninit();
	DB_PRINTF(("completed stream_uninit\n"));
	buffer_uninit();
	DB_PRINTF(("completed buffer_uninit\n"));
	mem_uninit();
	DB_PRINTF(("completed mem_uninit\n"));
	
	/* Clean up */
	free(card->hw);

	return B_OK;
}

			

static void mvp4g_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	((mvp4g_hw*)card->hw)->PcmPlaybackSR = 44100;
}


/*   re-starts DMA if transfer was stopped */
static void mvp4g_Re_StartPlayback(sound_card_t *card)
{
	uint8 status;
	cpu_status cp;
	
//  re-start DMA  if transfer was stopped
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x00,(0x1<<2)); //write 1 to resume transfer
     
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}

static void mvp4g_Re_StartCapture(sound_card_t *card)
{
	uint8 status;
	cpu_status cp;
	
//  re-start DMA  if transfer was stopped
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x10,(0x1<<2)); //write 1 to resume transfer
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}

static void mvp4g_PauseCapture(sound_card_t *card)
{
	cpu_status cp;
	
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11,0x1<<3); //write 1 to pause
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}

static void mvp4g_PausePlayback(sound_card_t *card)
{
	cpu_status cp;
	
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x1,0x1<<3); //write 1 to pause
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}

static void mvp4g_UnpauseCapture(sound_card_t *card)
{
	cpu_status cp;
	
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11, 0); //write 0 to unpause
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}

static void mvp4g_UnpausePlayback(sound_card_t *card)
{
	cpu_status cp;
	
   cp = disable_interrupts();
   acquire_spinlock(&card->bus.pci.lock);
   
   pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x1, 0); //write 0 to unpause
   release_spinlock(&card->bus.pci.lock);
   restore_interrupts(cp);
}


static void mvp4g_GDT_playback_setup(sound_card_t *card, uint32 address)
{
	cpu_status cp;
	
    cp = disable_interrupts();
    acquire_spinlock(&card->bus.pci.lock);
    pci->write_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x04, address);
    release_spinlock(&card->bus.pci.lock);
    restore_interrupts(cp);
}

static void mvp4g_GDT_capture_setup(sound_card_t *card, uint32 address)
{
	cpu_status cp;
	
    cp = disable_interrupts();
    acquire_spinlock(&card->bus.pci.lock);
    pci->write_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x14, address);
    release_spinlock(&card->bus.pci.lock);
    restore_interrupts(cp);
}

/* allocate memory for buffer  */
static status_t mvp4g_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void* phys_addr)
{
	return B_OK;
}

static void mvp4g_SetPlaybackFormat(sound_card_t *card, uint32 format, uint16 channel_count)
{
	uchar stereo;
	uchar bits16;
	
	if( format == B_FMT_16BIT) bits16 = 0x1;
	else if( format == B_FMT_8BIT_S || format == B_FMT_8BIT_U) bits16 = 0x0;
	else bits16 = 0x1;
	
	if (channel_count != 2)stereo = 0x0;  else stereo =0x1;

	DB_PRINTF(("mvp4g_SetPlaybackFormat\n"));

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x02,
							(0x1<<7) //loop playback
						|	(bits16 << 5)
						|	(stereo << 4)
						|	0x1  //interrupt at end of block
					);

}


static void mvp4g_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
}

static status_t mvp4g_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** log_addr)
{
	return B_OK;
}

static void mvp4g_SetCaptureFormat(sound_card_t *card, uint32 format, uint16 channel_count)
{
	uchar stereo;
	uchar bits16;
	
	if( format == B_FMT_16BIT) bits16 = 0x1;
	else if( format == B_FMT_8BIT_S || format == B_FMT_8BIT_U) bits16 = 0x0;
	else bits16 = 0x1;
	
	if (channel_count != 2)stereo = 0x0;  else stereo =0x1;
	
	
	DB_PRINTF(("mvp4g_SetCaptureFormat\n"));

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x12,
							(0x1<<7) //loop playback
						|	(bits16 << 5)
						|	(stereo << 4)
						|	0x1 //interrupt at end of block
					);

}


static uint16 mvp4g_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	uint16 ret = AC97_BAD_VALUE;
	int count;
	int base = card->bus.pci.info.u.h0.base_registers[0];
	uint32 ac97status;
	int attempt = 10;
try_again:	 
	// wait while busy
	for (count = 0x1000; count > 0; count--)
	{
		// check BUSY bit
		ac97status = pci->read_io_32(base+0x80);
		if((ac97status & (0x1<<24)) == 0) break;
		spin(50);
	}
	if(count == 0)
	{
		dprintf("mvp4g_Codec busy!!!\n");
		return AC97_BAD_VALUE;
	}

	pci->write_io_32(base+0x80,((offset & 0x7f)<<16) | (0x1<<23));
	
	for (count = 0x1000; count > 0; count--)
	{
		// read specified AC97 register
		ac97status = pci->read_io_32(base+0x80);
		if ((ac97status & (0x3<<24)) == (0x1<<25))
		{
			if (  (ac97status & (0x7f<<16)) == ((offset & 0x7f)<<16)  ) 
			{
				ret = ac97status & 0xffff;
//				dprintf("mvp4g_Codec Read is ok by %d attempts, count = %d \n", 10 - attempt, count);
				break;
			}
			else
			{
				if((--attempt)>0) goto try_again;
				else 
				{
					dprintf("mvp4g_Codec Read: attemts expired!!! ac97status = 0x%lx, offset = 0x%x\n",ac97status,offset);
					return AC97_BAD_VALUE;
				}
			}
		}
		spin(50);
	}

	if(count == 0)
	{
		dprintf("mvp4g_Codec Read: Data not valid!!! ac97status = 0x%lx\n",ac97status);
		return AC97_BAD_VALUE;
	}

	return ret;
}


static uint16 mvp4g_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(((mvp4g_hw*)card->hw)->codec.lock));

	ret = mvp4g_CodecReadUnsecure(card, offset);
	
	release_spinlock(&(((mvp4g_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);

	return ret;
}

static status_t mvp4g_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	int count;
	sound_card_t* card = (sound_card_t*) host;
	int base = card->bus.pci.info.u.h0.base_registers[0];
	uint32 ac97status;
	status_t ret = B_ERROR;
	cpu_status cp;
	int attempt = 10;

	if (mask == 0) return B_OK;
	
	cp = disable_interrupts();
	acquire_spinlock(&(((mvp4g_hw*)card->hw)->codec.lock));

	if (mask != 0xffff) {
		uint16 old  = mvp4g_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}


try_again:	 

	// wait while busy
	for (count = 0x1000; count > 0; count--)
	{
		// check BUSY bit
		ac97status = pci->read_io_32(base+0x80);
		if((ac97status & (0x1<<24)) == 0) break;
		spin(50);
	}

	if(count)
	{
		pci->write_io_32(base+0x80,((offset & 0x7f)<<16) | value);
#if 0
		for (count = 0x400; count > 0; count--)
		{
			// read specified AC97 register
			ac97status = g_io_32(base+0x80);

			if (ac97status & (0x3<<24) == (0x1<<25))
			{
//				if (  (ac97status & ((0x7f<<16) | 0xffff)) == ((offset & 0x7f)<<16) | value) 
				{
					ret = B_OK;
//					dprintf("mvp4g_Codec Write is ok by %d attempts, count = %d \n", 10 - attempt, count);
					break;
				}
/*				else 
				{
					if((--attempt)>0) goto try_again;
					else 
					{
						dprintf("mvp4g_Codec write: attemts expired!!!\n");
						dprintf("ac97status = 0x%lx, offset = 0x%x, value = 0x%x\n",ac97status,offset,value);
						ret = B_ERROR;
						break;
					}
				}*/
			}
			spin(50);
		}
#endif
	ret = B_OK;
	}
	release_spinlock(&(((mvp4g_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);
	return ret;
}


static status_t mvp4g_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;
	DB_PRINTF(("In mvp4g_InitCodec\n"));
	{
		uchar  bus    = card->bus.pci.info.bus;
		uchar  device = card->bus.pci.info.device;
		uchar  ctrl,stat;

		ctrl = pci->read_pci_config( bus, device, card->bus.pci.info.function, 0x41, 1);
		dprintf("AC Link Interface Control = 0x%x\n",(uint16)ctrl);

//		dprintf("Enable AC Link SGD Read ChannelPCM Data Output and  Variable SR on DEMAND\n");
		pci->write_pci_config(  bus, device, card->bus.pci.info.function, 0x41, 1,  ctrl | (0x1<<2)| (0x1<<3));
		spin(1000000);

		stat = pci->read_pci_config( bus, device, card->bus.pci.info.function, 0x40, 1);
		if((stat & 0x1) == 0)
		{
			dprintf("mvp4g_InitCodec error: AC Link Interface Status = %x\n",stat);
			return B_ERROR;
		}
	}

	err = ac97init(&((mvp4g_hw*)card->hw)->codec, 
					(void*)card,
					mvp4g_CodecWrite,
					mvp4g_CodecRead);
					
	if(err!=B_OK) dprintf("mvp4g_InitCodec(): ac97init err = %ld!\n",err);
	mvp4g_CodecWrite(card,0x1e,0xf,0xffff);
	return err;				
}

static void mvp4g_UninitCodec(sound_card_t *card)
{
	ac97uninit(&((mvp4g_hw*)card->hw)->codec); 
}

static status_t mvp4g_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	DB_PRINTF(("SoundSetup: ss->sample_rate = %d\n",ss->sample_rate));
	ss->sample_rate = kHz_44_1;
	ret = AC97_SoundSetup(&((mvp4g_hw*)card->hw)->codec, ss);
	if(ret!=B_OK)return ret;
//	AC97_GetSoundSetup(&((mvp4g_hw*)card->hw)->codec,ss);
	return B_OK;
}

static status_t  mvp4g_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ret = AC97_GetSoundSetup(&((mvp4g_hw*)card->hw)->codec,ss);
	DB_PRINTF(("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate));
	return ret;
}


static status_t  mvp4g_SetSound(sound_card_t *card, int16 types, int16 value)
{
	status_t ret;
	ret = AC97_SetSound(&((mvp4g_hw*)card->hw)->codec, types, value);
	DB_PRINTF(("SetSound: value = %d\n", value));
	return ret;
}

static status_t   mvp4g_SetSampleRate(sound_card_t *card, uint16 rate)
{
	status_t ret;
	ret = AC97_SetSampleRate(&((mvp4g_hw*)card->hw)->codec, rate);
	
	return ret;

}




