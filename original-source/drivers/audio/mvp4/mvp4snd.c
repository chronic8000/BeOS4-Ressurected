/************************************************************************/
/*                                                                      */
/*                              mvp4snd.c                              	*/
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



#define AC97_BAD_VALUE		(0xffff)




extern pci_module_info *pci;

typedef struct _SGD_Table_t{
	uint32 BaseAddr;
	unsigned count : 24;
	unsigned reserved : 5;
	unsigned stop : 1;
	unsigned flag : 1;
	unsigned eol : 1;
}SGD_Table_t;


typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;

typedef struct _mvp4_hw {
	timers_data_t timers_data;	
	ac97_t codec;

	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	struct{bool IsStereo; bool Is16bits;} PcmPlaybackFormat;
	uint32 PcmPlaybackSR;

	area_id PcmCaptureAreaId;
	size_t PcmCaptureBufSize; 
	struct{bool IsStereo; bool Is16bits;} PcmCaptureFormat;
	uint32 PcmCaptureSR;

	SGD_Table_t* SGDTable; 
 	void* SGDTablePhysAddr;
	area_id SGDTableAreaId;

} mvp4_hw; 


static status_t 	mvp4_Init(sound_card_t * card);
static status_t 	mvp4_Uninit(sound_card_t * card);
static void 		mvp4_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	mvp4_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		mvp4_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		mvp4_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		mvp4_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		mvp4_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		mvp4_StartPcm(sound_card_t *card);
static void 		mvp4_StopPcm(sound_card_t *card);
static status_t		mvp4_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		mvp4_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		mvp4_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		mvp4_InitJoystick(sound_card_t * card);
static void 		mvp4_enable_gameport(sound_card_t * card);
static void 		mvp4_disable_gameport(sound_card_t * card);

static status_t 	mvp4_InitMPU401(sound_card_t * card);
static void 		mvp4_enable_mpu401(sound_card_t *card);
static void 		mvp4_disable_mpu401(sound_card_t *card);
static void 		mvp4_enable_mpu401_interrupts(sound_card_t *card);
static void 		mvp4_disable_mpu401_interrupts(sound_card_t *card);






static sound_card_functions_t mvp4_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&mvp4_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&mvp4_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&mvp4_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&mvp4_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&mvp4_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&mvp4_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&mvp4_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&mvp4_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&mvp4_SetPlaybackDmaBuf,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&mvp4_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&mvp4_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&mvp4_SetCaptureDmaBuf,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&mvp4_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&mvp4_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&mvp4_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&mvp4_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&mvp4_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&mvp4_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&mvp4_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&mvp4_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&mvp4_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
} ;



card_descrtiptor_t mvp4_audio_descr = {
	"MVP4",
	PCI, 
	&mvp4_functions,
	0x1106, //MVP4_VENDOR_ID, 
	0x3058 // MVP4 AUDIO CODEC DEVICE_ID,
};




 
static status_t 	mvp4_InitCodec(sound_card_t *card);
static void 		mvp4_UninitCodec(sound_card_t *card);
static uint16 		mvp4_CodecRead(void* host, uchar offset);
static status_t 	mvp4_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);



/* --- 
	Interrupt service routine
	---- */
static uint32 b00count=0,b01count=0,b10count=0,b11count=0;
static int32 mvp4_isr(void *data)
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
				
				acquire_spinlock(&((mvp4_hw*)card->hw)->timers_data.Lock);	
				((mvp4_hw*)card->hw)->timers_data.SystemClock = time;
				((mvp4_hw*)card->hw)->timers_data.IntCount++;
				release_spinlock(&((mvp4_hw*)card->hw)->timers_data.Lock);
			}			
			{
				//Determinate current block (half_buffer)
				int half;
				uint32 CurrentSGDTableAddr = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x4);
				uint32 SGDCurrentCount = pci->read_io_32(card->bus.pci.info.u.h0.base_registers[0]+0xc);
//				if(CurrentSGDTableAddr == (uint32)((mvp4_hw*)card->hw)->SGDTablePhysAddr)
//					half = 1;
				if(CurrentSGDTableAddr == (uint32)(((mvp4_hw*)card->hw)->SGDTablePhysAddr + sizeof(SGD_Table_t)))
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
				if(CurrentSGDTableAddr == (uint32)(((mvp4_hw*)card->hw)->SGDTablePhysAddr + 3*sizeof(SGD_Table_t)))
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



static void mvp4_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((mvp4_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((mvp4_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((mvp4_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((mvp4_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = ((mvp4_hw*)card->hw)->PcmPlaybackBufSize 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((mvp4_hw*)card->hw)->PcmPlaybackSR) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);

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
}

static void mvp4_enable_mpu401(sound_card_t *card)
{
}

static void mvp4_disable_mpu401(sound_card_t *card)
{
}

static void mvp4_enable_mpu401_interrupts(sound_card_t *card)
{
}

static void mvp4_disable_mpu401_interrupts(sound_card_t *card)
{
}


static void mvp4_enable_gameport(sound_card_t * card)
{
}

static void mvp4_disable_gameport(sound_card_t * card)
{
}

static status_t mvp4_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static status_t mvp4_InitMPU401(sound_card_t * card)
{
	return ENODEV;
}


static status_t mvp4_PcmStartPlayback(sound_card_t * card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4_PcmStartPlayback\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x01,	(0x1<<7));

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	return B_OK;
}

static status_t mvp4_PcmStartCapture(sound_card_t * card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4_PcmStartCapture\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11,	(0x1<<7));

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	return B_OK;
}



static status_t mvp4_Init(sound_card_t * card)
{
	status_t err = B_OK;

	//DB_PRINTF(("mvp4_Init(): here! \n"));
	
	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(mvp4_hw));
	if(!card->hw) 
	{
		derror("mvp4_Init(): malloc error!\n");
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(mvp4_hw));	// zero it out

	/* Some initialization */
	((mvp4_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((mvp4_hw*)card->hw)->PcmPlaybackAreaId = -1; 
	((mvp4_hw*)card->hw)->SGDTableAreaId = -1;

	{
		size_t size = sizeof(SGD_Table_t)*4;//2 for playback and 2 for capture
		((mvp4_hw*)card->hw)->SGDTableAreaId = allocate_memory("mvp4_SGDTableAreaId",
					(void**)(&((mvp4_hw*)card->hw)->SGDTable),
					&((mvp4_hw*)card->hw)->SGDTablePhysAddr,
					&size);
	}
	if (((mvp4_hw*)card->hw)->SGDTableAreaId < 0)
	{
		ddprintf("Error: Memory allocation for ((mvp4_hw*)card->hw)->SGDTableAreaId failed!!\n");
		return ((mvp4_hw*)card->hw)->SGDTableAreaId;
	}


	/* Init codec */
	if ((err = mvp4_InitCodec(card))) 
	{
		derror("mvp4_Init: mvp4_InitCodec error!\n");
		 return err;
	}

	/* Install mvp4 isr */
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, mvp4_isr, card, 0);

	return B_OK;
}


static status_t mvp4_Uninit(sound_card_t * card)
{
	//DB_PRINTF(("mvp4_Uninit(): here! \n"));

	/* Remove mvp4 isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, mvp4_isr, card);

	/* Uninit codec */
	mvp4_UninitCodec(card);

	if(((mvp4_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((mvp4_hw*)card->hw)->PcmCaptureAreaId);
	if(((mvp4_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((mvp4_hw*)card->hw)->PcmPlaybackAreaId);
	if(((mvp4_hw*)card->hw)->SGDTableAreaId >= 0) delete_area(((mvp4_hw*)card->hw)->SGDTableAreaId);

	/* Clean up */
	free(card->hw);

	return B_OK;
}


static void mvp4_StartPcm(sound_card_t *card)
{
	b00count=0;
	b01count=0;
	b10count=0;
	b11count=0;
	((mvp4_hw*)card->hw)->timers_data.IntCount = 0;
	mvp4_PcmStartPlayback(card);
	mvp4_PcmStartCapture(card);
//	spin(10000000);
//	dprintf("b00count = %ld, b01count = %ld, b10count = %ld, b11count = %ld\n",
//		b00count,b01count,b10count,b11count);
}


static void mvp4_StopPcm(sound_card_t *card)
{
	cpu_status cp;
	DB_PRINTF(("mvp4_PcmStopPcm\n"));


	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	/*terminate playback*/
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x01,	(0x1<<6));
	/*terminate capture*/
	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x11,	(0x1<<6));

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

}


static void mvp4_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	((mvp4_hw*)card->hw)->PcmPlaybackSR = 44100;//patch
}

static status_t mvp4_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** log_addr)
{

	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	DB_PRINTF(("In mvp4_SetPlaybackDmaBuf\n"));

	AreaId = allocate_memory("mvp4_PlaybackDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for mvp4__PlaybackDmaBuf failed!!\n");
		ddprintf("mvp4_SetPlaybackDmaBuf: return %ld;\n", AreaId);
		return AreaId;
	}
	if(((mvp4_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((mvp4_hw*)card->hw)->PcmPlaybackAreaId);
	((mvp4_hw*)card->hw)->PcmPlaybackAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

	((mvp4_hw*)card->hw)->SGDTable[0].BaseAddr = (uint32)phys_addr;
	((mvp4_hw*)card->hw)->SGDTable[0].count = size/2/*/2/2*/;
	((mvp4_hw*)card->hw)->SGDTable[0].stop = 0;
	((mvp4_hw*)card->hw)->SGDTable[0].flag = 1;
	((mvp4_hw*)card->hw)->SGDTable[0].eol = 0;

	((mvp4_hw*)card->hw)->SGDTable[1].BaseAddr =(uint32)phys_addr + size/2;
	((mvp4_hw*)card->hw)->SGDTable[1].count = size/2/*/2/2*/;
	((mvp4_hw*)card->hw)->SGDTable[1].stop = 0;
	((mvp4_hw*)card->hw)->SGDTable[1].flag = 1;
	((mvp4_hw*)card->hw)->SGDTable[1].eol = 1;

	((mvp4_hw*)card->hw)->PcmPlaybackBufSize = size;

	{
		cpu_status cp;
		cp = disable_interrupts();
		acquire_spinlock(&card->bus.pci.lock);
	
		pci->write_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x04,
							(uint32)(((mvp4_hw*)card->hw)->SGDTablePhysAddr)
							);
	
		release_spinlock(&card->bus.pci.lock);
		restore_interrupts(cp);
	}

	DB_PRINTF(("mvp4_SetPlaybackDmaBuf OK\n"));
	return B_OK;
}

static void mvp4_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uchar stereo;
	uchar bits16;
	
	if(num_of_bits==8) bits16 = 0x0; else bits16 = 0x1; 
	if (num_of_ch != 2)stereo = 0x0;  else stereo =0x1;

	DB_PRINTF(("mvp4_SetPlaybackFormat\n"));

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x02,
							(0x1<<7) //loop playback
						|	(bits16 << 5)
						|	(stereo << 4)
						|	0x1 //interrupt at end of block
					);

}


static void mvp4_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
}

static status_t mvp4_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** log_addr)
{
	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	DB_PRINTF(("In mvp4_SetCaptureDmaBuf\n"));

	AreaId = allocate_memory("mvp4_CaptureDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for mvp4__PlaybackDmaBuf failed!!\n");
		ddprintf("mvp4_SetPlaybackDmaBuf: return %ld;\n", AreaId);
		return AreaId;
	}
	if(((mvp4_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((mvp4_hw*)card->hw)->PcmCaptureAreaId);
	((mvp4_hw*)card->hw)->PcmCaptureAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

	((mvp4_hw*)card->hw)->SGDTable[2].BaseAddr = (uint32)phys_addr;
	((mvp4_hw*)card->hw)->SGDTable[2].count = size/2/*/2/2*/;
	((mvp4_hw*)card->hw)->SGDTable[2].stop = 0;
	((mvp4_hw*)card->hw)->SGDTable[2].flag = 1;
	((mvp4_hw*)card->hw)->SGDTable[2].eol = 0;

	((mvp4_hw*)card->hw)->SGDTable[3].BaseAddr =(uint32)phys_addr + size/2;
	((mvp4_hw*)card->hw)->SGDTable[3].count = size/2/*/2/2*/;
	((mvp4_hw*)card->hw)->SGDTable[3].stop = 0;
	((mvp4_hw*)card->hw)->SGDTable[3].flag = 1;
	((mvp4_hw*)card->hw)->SGDTable[3].eol = 1;

	((mvp4_hw*)card->hw)->PcmCaptureBufSize = size;

	{
		cpu_status cp;
		cp = disable_interrupts();
		acquire_spinlock(&card->bus.pci.lock);
	
		pci->write_io_32(card->bus.pci.info.u.h0.base_registers[0]+0x14,
							(uint32)(((mvp4_hw*)card->hw)->SGDTablePhysAddr)+2*sizeof(SGD_Table_t)
							);
	
		release_spinlock(&card->bus.pci.lock);
		restore_interrupts(cp);
	}

	DB_PRINTF(("mvp4_SetCaptureDmaBuf OK\n"));
	return B_OK;
}

static void mvp4_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uchar stereo;
	uchar bits16;
	
	if(num_of_bits==8) bits16 = 0x0; else bits16 = 0x1; 
	if (num_of_ch != 2)stereo = 0x0;  else stereo =0x1;

	DB_PRINTF(("mvp4_SetCaptureFormat\n"));

	pci->write_io_8(card->bus.pci.info.u.h0.base_registers[0]+0x12,
							(0x1<<7) //loop playback
						|	(bits16 << 5)
						|	(stereo << 4)
						|	0x1 //interrupt at end of block
					);

}




static uint16 mvp4_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	uint16 ret = AC97_BAD_VALUE;
	int count;
	int base = card->bus.pci.info.u.h0.base_registers[0];
	uint32 ac97status;
	int attempt = 10;
try_again:	 
	// wait while busy
	for (count = 400; count > 0; count--)
	{
		// check BUSY bit
		ac97status = pci->read_io_32(base+0x80);
		if((ac97status & (0x1<<24)) == 0) break;
		spin(50);
	}
	if(count == 0)
	{
		dprintf("mvp4_Codec busy!!!\n");
		return AC97_BAD_VALUE;
	}

	pci->write_io_32(base+0x80,((offset & 0x7f)<<16) | (0x1<<23));
	
	for (count = 400; count > 0; count--)
	{
		// read specified AC97 register
		ac97status = pci->read_io_32(base+0x80);
		if ((ac97status & (0x3<<24)) == (0x1<<25))
		{
			if (  (ac97status & (0x7f<<16)) == ((offset & 0x7f)<<16)  ) 
			{
				ret = ac97status & 0xffff;
//				dprintf("mvp4_Codec Read is ok by %d attempts, count = %d \n", 10 - attempt, count);
				break;
			}
			else
			{
				if((--attempt)>0) goto try_again;
				else 
				{
					dprintf("mvp4_Codec Read: attemts expired!!! ac97status = 0x%lx, offset = 0x%x\n",ac97status,offset);
					return AC97_BAD_VALUE;
				}
			}
		}
		spin(50);
	}

	if(count == 0)
	{
		dprintf("mvp4_Codec Read: Data not valid!!! ac97status = 0x%lx\n",ac97status);
		return AC97_BAD_VALUE;
	}

	return ret;
}


static uint16 mvp4_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(((mvp4_hw*)card->hw)->codec.lock));

	ret = mvp4_CodecReadUnsecure(card, offset);
	
	release_spinlock(&(((mvp4_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);

	return ret;
}

static status_t mvp4_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
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
	acquire_spinlock(&(((mvp4_hw*)card->hw)->codec.lock));

	if (mask != 0xffff) {
		uint16 old  = mvp4_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}


try_again:	 

	// wait while busy
	for (count = 400; count > 0; count--)
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
		for (count = 400; count > 0; count--)
		{
			// read specified AC97 register
			ac97status = pci->read_io_32(base+0x80);

			if (ac97status & (0x3<<24) == (0x1<<25))
			{
//				if (  (ac97status & ((0x7f<<16) | 0xffff)) == ((offset & 0x7f)<<16) | value) 
				{
					ret = B_OK;
//					dprintf("mvp4_Codec Write is ok by %d attempts, count = %d \n", 10 - attempt, count);
					break;
				}
/*				else 
				{
					if((--attempt)>0) goto try_again;
					else 
					{
						dprintf("mvp4_Codec write: attemts expired!!!\n");
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
	release_spinlock(&(((mvp4_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);
	return ret;
}


static status_t mvp4_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;
	DB_PRINTF(("In mvp4_InitCodec\n"));
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
			dprintf("mvp4_InitCodec error: AC Link Interface Status = %x\n",stat);
			return B_ERROR;
		}
	}

	err = ac97init(&((mvp4_hw*)card->hw)->codec, 
					(void*)card,
					mvp4_CodecWrite,
					mvp4_CodecRead);
					
	if(err!=B_OK) dprintf("mvp4_InitCodec(): ac97init err = %ld!\n",err);
	mvp4_CodecWrite(card,0x1e,0xf,0xffff);
	return err;				
}

static void mvp4_UninitCodec(sound_card_t *card)
{
	ac97uninit(&((mvp4_hw*)card->hw)->codec); 
}

static status_t mvp4_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	dprintf("SoundSetup: ss->sample_rate = %d\n",ss->sample_rate);
	ret = AC97_SoundSetup(&((mvp4_hw*)card->hw)->codec, ss);
	if(ret!=B_OK)return ret;
	AC97_GetSoundSetup(&((mvp4_hw*)card->hw)->codec,ss);
	return B_OK;
}

static status_t  mvp4_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ret = AC97_GetSoundSetup(&((mvp4_hw*)card->hw)->codec,ss);
	dprintf("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate);
	return ret;
}









