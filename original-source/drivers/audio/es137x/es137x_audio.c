/************************************************************************/
/*   es137x_audio.c --     Main Module                                             */
/*  Copyright (c)  1998-1995  alt.software.com  All Rights Reserved     */
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Evgeny Pronin  					    */
/*			  															*/
/************************************************************************/

//#define  DEBUG

#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>

#include <ac97_module.h>
#include "es137xHW.h"

#include "audio_drv.h"
#include "misc.h"
#include "debug.h"


#define PCI_VENDOR_ID_ENSONIQ           0x1274
#define PCI_DEVICE_ID_ENSONIQ_ES1370    0x5000
#define PCI_DEVICE_ID_ENSONIQ_ES1371    0x1371
#define PCI_DEVICE_ID_ENSONIQ_ES5880    0x5880

#define ES1370_CODEC_WRITE_DELAY    	20
#define AC97_BAD_VALUE					0xffff
#define ES1371_CODEC_POLL_COUNT			0x10000
#define SRC_POLL_COUNT 					0x10000
#define SRC_OK_STATE					1
#define SRC_OK_POLL_COUNT 				0x1000


extern pci_module_info *pci;



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


typedef struct _es137x_hw {
	timers_data_t	timers_data;	
	sound_setup 	ss;
	ac97_t			ac97codec;
	void* 			codec;
	uint32 			DevId;
	int32 			Lock;	
	uint32 			iobase;
	int 			joy_base;
	int 			mpu401_base;

	area_id 		PcmPlaybackAreaId; 
	size_t 			PcmPlaybackBufSize;
	struct{bool IsStereo; bool Is16bits;} PcmPlaybackFormat;
	uint32 			PcmPlaybackSR;
	my_timer 		PlaybackTimer;
	int 			PlaybackHalf;

	area_id 		PcmCaptureAreaId;
	size_t 			PcmCaptureBufSize;
	void* 			PcmCaptureBufLogAddr; 
	struct{bool IsStereo; bool Is16bits;} PcmCaptureFormat;
	uint32			PcmCaptureSR;
	my_timer 		CaptureTimer;
	int 			CaptureHalf;
	uint32 			SamplesCaptured;
	
	int				mpu401_emul_ack; // count of ack bytes (0xfe) to emulate in mpu401 data port 
	bool			mpu401_uart_mode; //true to emulate mpu401 UART mode  
	
	sem_id  		codec_read_sem;   // semaphor  id  
	sem_id  		codec_write_sem;  // semaphor  id  
	
} es137x_hw; 


static status_t 	es137x_Init(sound_card_t * card);
static status_t 	es137x_Uninit(sound_card_t * card);
static void 		es137x_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	es137x_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** addr);
static void 		es137x_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		es137x_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		es137x_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** addr);
static void 		es137x_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		es137x_StartPcm(sound_card_t *card);
static void 		es137x_StopPcm(sound_card_t *card);
static status_t		es137x_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		es137x_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		es137x_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		es137x_InitJoystick(sound_card_t * card);
static void 		es137x_enable_gameport(sound_card_t * card);
static void 		es137x_disable_gameport(sound_card_t * card);

static status_t 	es137x_InitMPU401(sound_card_t * card);
static void 		es137x_enable_mpu401(sound_card_t *card);
static void 		es137x_disable_mpu401(sound_card_t *card);
static void 		es137x_enable_mpu401_interrupts(sound_card_t *card);
static void 		es137x_disable_mpu401_interrupts(sound_card_t *card);

static uchar 		mpu401_read_port(void*, int);
static void 		mpu401_write_port(void*, int, uchar);


static void 		es1370_codec_reg_write(void* the_card, uint8 reg, uint8 value);

static void 		hw_set_sample_rate(sound_card_t*, uint32 sample_rate);

static status_t 	es1371_InitCodec(sound_card_t *card);
static void 		es1371_UninitCodec(sound_card_t *card);
static uint16 		es1371_CodecRead(void* host, uchar offset);
static status_t 	es1371_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);
static uint16 		es1371_CodecReadUnsecure(sound_card_t *card, uchar offset);

static void 		SRCInit(sound_card_t *card);
static void 		SRCRegWrite(sound_card_t *card, int reg, uint16 val);
static void 		SRCSetDACRateWithAdjustment(sound_card_t *card, uint16 rate, int adjustment);
static void 		SRCSetDACRate(sound_card_t *card, uint16 rate);
static void 		SRCSetADCRate(sound_card_t *card, uint16 rate);
static uint16 		SRCRegRead(sound_card_t *card, int reg);

static mpu401_io_hooks_t mpu401_io_hooks = {&mpu401_read_port, &mpu401_write_port}; 



static sound_card_functions_t es1370_audio_functions = 
{
&es137x_Init,
&es137x_Uninit,
&es137x_StartPcm,
&es137x_StopPcm,
&es137x_GetClocks,
&es137x_SoundSetup,
&es137x_GetSoundSetup,
&es137x_SetPlaybackSR,
&es137x_SetPlaybackDmaBuf,
&es137x_SetPlaybackFormat,
&es137x_SetCaptureSR,
&es137x_SetCaptureDmaBuf,
&es137x_SetCaptureFormat,
&es137x_InitJoystick,
&es137x_enable_gameport,
&es137x_disable_gameport,
&es137x_InitMPU401,
&es137x_enable_mpu401,
&es137x_disable_mpu401,
&es137x_enable_mpu401_interrupts,
&es137x_disable_mpu401_interrupts,

NULL, // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
&mpu401_io_hooks, // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
NULL,
NULL
};


// Card descriptor for es1370
card_descrtiptor_t es1370_audio_descr = {
	"es1370",
	PCI, 
	&es1370_audio_functions,
	PCI_VENDOR_ID_ENSONIQ,            
	PCI_DEVICE_ID_ENSONIQ_ES1370,       
	0x0
};

// Card descriptor for es1371
card_descrtiptor_t es1371_audio_descr = {
	"es1371",
	PCI, 
	&es1370_audio_functions,
	PCI_VENDOR_ID_ENSONIQ,            
	PCI_DEVICE_ID_ENSONIQ_ES1371,       
	0x0
};

// Card descriptor for newer es1371 with 5880 chip
card_descrtiptor_t es5880_audio_descr = {
	"es1371",
	PCI, 
	&es1370_audio_functions,
	PCI_VENDOR_ID_ENSONIQ,            
	PCI_DEVICE_ID_ENSONIQ_ES5880,       
	0x0
};

const char* es137x_joystick_base_disable_param[4] = {
	"disable_joyBase_0x200",
	"disable_joyBase_0x208",
	"disable_joyBase_0x210",
	"disable_joyBase_0x218"
	};

enum {JoyBase200 = 0,JoyBase208,JoyBase210,JoyBase218};
static uint es137x_joystick_base[4] 	   = {0x200,0x208,0x210,0x218};
static bool es137x_allowed_joystick_cnf[4] = {true, true, true, true};

void es137x_GetDriverSettings(void* hSettings)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if(get_driver_boolean_parameter(hSettings,es137x_joystick_base_disable_param[i],true,false))
			es137x_allowed_joystick_cnf[i] = false;
	}
}

/***************************************************/
static int32 es137x_isr(void *data)
{
	status_t ret 	   = B_HANDLED_INTERRUPT;
	sound_card_t * card= (sound_card_t *) data;
	
	uint32 status	   = 0;
	uint8  serctl	   = 0;
	bigtime_t  time    = 0; 
	
	DB_PRINTF(("es1370_isr\n"));
	
	acquire_spinlock(&card->bus.pci.lock);
	// read status register
	status = pci->read_io_32(((es137x_hw*)card->hw)->iobase + ICS_STATUS_REGISTER); // read status register
	
	if (status & (0x1<<1)) // DAC2
	{
	    int Half;
	   		    
    	// clear interrupt by toggling interrupt enable
		serctl = pci->read_io_8(((es137x_hw*)card->hw)->iobase + 0x21);
		pci->write_io_8(((es137x_hw*)card->hw)->iobase +0x21,serctl & ~0x02);
		pci->write_io_8(((es137x_hw*)card->hw)->iobase +0x21,serctl | 0x02);
		
		// Store internal time for synchronization 
		time = system_time();
				
		acquire_spinlock(&((es137x_hw*)card->hw)->timers_data.Lock);	
		((es137x_hw*)card->hw)->timers_data.SystemClock = time;
		((es137x_hw*)card->hw)->timers_data.IntCount++;
	    release_spinlock(&((es137x_hw*)card->hw)->timers_data.Lock);
	
		// read current frame count (counts down)
		{
			uint16 position;
			pci->write_io_8(((es137x_hw*)card->hw)->iobase+0x0c,0x0c);
			position = pci->read_io_16(((es137x_hw*)card->hw)->iobase+0x3E);//current hw position in dma buffer (in longwords)
			position <<= 2;   // transform into bytes! 	
			Half = (position/(((es137x_hw*)card->hw)->PcmPlaybackBufSize/2) + 1)%2;
		}
			
		// Proceed playback interrupt
		if ((*card->pcm_playback_interrupt)(card, Half)) 
			ret = B_INVOKE_SCHEDULER;
			
	}
	else
	if (status & 0x1) // ADC
	{
	    int Half;
	   	    
		// clear interrupt by toggling interrupt enable
		serctl = pci->read_io_8(((es137x_hw*)card->hw)->iobase + 0x21);
		pci->write_io_8(((es137x_hw*)card->hw)->iobase +0x21,serctl & ~0x04);
		pci->write_io_8(((es137x_hw*)card->hw)->iobase +0x21,serctl | 0x04);
		
					
		// read current frame count (counts down)
		{
			uint16 position;
			pci->write_io_8(((es137x_hw*)card->hw)->iobase+0x0c,0x0d);
			position = pci->read_io_16(((es137x_hw*)card->hw)->iobase+0x36);//current hw position in dma buffer (in longwords)
			position <<= 2; 	// transform into bytes! 	
			Half = (position/(((es137x_hw*)card->hw)->PcmCaptureBufSize/2) + 1)%2;		
		}
							
		// Proceed capture interrupt
		if ((*card->pcm_capture_interrupt)(card, Half ))
				ret = B_INVOKE_SCHEDULER;
	}
	else  /* Check if MIDI request is on */
	if (status & (0x1<<3)) //UART
	{
		if ((*card->midi_interrupt)(card))
			ret = B_INVOKE_SCHEDULER;
	}
	
	else
	{
	  	ret = B_UNHANDLED_INTERRUPT;
	}
		
	release_spinlock(&card->bus.pci.lock);
	return ret;

}

static void es137x_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{
	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;
	
		
	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((es137x_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((es137x_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((es137x_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((es137x_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = ((es137x_hw*)card->hw)->PcmPlaybackBufSize 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((es137x_hw*)card->hw)->PcmPlaybackSR) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);
	if(0)
 	{
 		static int i = 0;
 		static bigtime_t LastSystemClock = 0, LastSampleClock = 0;
 		i++;
 		i%=100;
 		if(i==0)
 		{
 			dprintf("SystemTime = %ld,SampleTime = %ld \n",
 				(uint32)(*pSystemClock-LastSystemClock),(uint32)(*pSampleClock-LastSampleClock));
 		}
		LastSystemClock = *pSystemClock;
		LastSampleClock = *pSampleClock;
 	}
}

static void es137x_enable_mpu401(sound_card_t *card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0,pci->read_io_32(((es137x_hw*)card->hw)->iobase+0) | (0x1<<3));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void es137x_disable_mpu401(sound_card_t *card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0, pci->read_io_32(((es137x_hw*)card->hw)->iobase+0) & ~(0x1<<3)); /*disable UART*/
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void es137x_enable_mpu401_interrupts(sound_card_t *card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
  	pci->write_io_8(((es137x_hw*)card->hw)->iobase+9,(0x1<<7));/*RXINTEN*/
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
 
}

static void es137x_disable_mpu401_interrupts(sound_card_t *card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_8(((es137x_hw*)card->hw)->iobase+9,0);
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void es137x_enable_gameport(sound_card_t * card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_32(((es137x_hw*)card->hw)->iobase,
						pci->read_io_32(((es137x_hw*)card->hw)->iobase) |
						(0x1 << 2 ));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void es137x_disable_gameport(sound_card_t * card)
{
    cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	pci->write_io_32(((es137x_hw*)card->hw)->iobase,
						pci->read_io_32(((es137x_hw*)card->hw)->iobase) &
						~(0x1 << 2 ));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static status_t es137x_InitJoystick(sound_card_t * card)
{

   if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
   {
   		if(es137x_allowed_joystick_cnf[JoyBase200])	
		{
			card->joy_base = 0x200;
			es137x_allowed_joystick_cnf[JoyBase200] = false;
			return B_OK;
		}
	}
	else if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1371)
	{
		int i;
		for(i=3;i>=0;i--) //do not change loop direction
		{
	   		if(es137x_allowed_joystick_cnf[i])	
			{
			    cpu_status cp;
				card->joy_base = es137x_joystick_base[i];
				es137x_allowed_joystick_cnf[i] = false;

				cp = disable_interrupts();
				acquire_spinlock(&card->bus.pci.lock);
				pci->write_io_32(((es137x_hw*)card->hw)->iobase,
									pci->read_io_32(((es137x_hw*)card->hw)->iobase) |
									((i & 0x3) << 24));
				release_spinlock(&card->bus.pci.lock);
				restore_interrupts(cp);
				return B_OK;
			}
		}
	}
	return ENODEV;
}

static status_t es137x_InitMPU401(sound_card_t * card)
{
	cpu_status cp;
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
    pci->write_io_32(((es137x_hw*)card->hw)->iobase+0,pci->read_io_32(((es137x_hw*)card->hw)->iobase+0) | (0x1<<3));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
	return B_OK;
}


static status_t es137x_PcmStartPlayback(sound_card_t * card)
{
	cpu_status cp;
	uint32 control;
		
	DB_PRINTF(("es1370_PcmStartPlayback\n"));
	
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
		
	// P2_END_INC . where is next buffer 8bit : +1; 16bit : +2;
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~B_SERIAL_INTF_CONTROL_P2_END_INC;
    // need frame alignment ???
    control |= (control & 0x08) ? (2<<19) : (1<<19);
    pci->write_io_32(((es137x_hw*)card->hw)->iobase +SERIAL_INTF_CONTROL_REGISTER,control);
    // need frame alignment ???
    // P2_ST_INC ?? zero lah...
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~B_SERIAL_INTF_CONTROL_P2_ST_INC;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);
    // DAC2 in loop mode
    // DAC2 in play mode
    // DAC2 playback zeros when disabled
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~B_SERIAL_INTF_CONTROL_P2_LOOP_SEL; // loop mode
    control &= ~B_SERIAL_INTF_CONTROL_P2_PAUSE;
    control &= ~B_SERIAL_INTF_CONTROL_P2_DAC_SEN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);
    // enable DAC2 interrupt
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~B_SERIAL_INTF_CONTROL_P2_INTR_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);
    control |= B_SERIAL_INTF_CONTROL_P2_INTR_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);
    // enable DAC2
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER);
    control &= ~B_ICS_CONTROL_DAC2_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER,control);
    control |= B_ICS_CONTROL_DAC2_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER,control);
 
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	return B_OK;
}

static status_t es137x_PcmStartCapture(sound_card_t * card)
{
    cpu_status cp;
    uint32 control;
    
    DB_PRINTF(("es1370_PcmStartCapture\n"));

    cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	
	// ADC in loop mode
	control = pci->read_io_32(((es137x_hw*)card->hw)->iobase+SERIAL_INTF_CONTROL_REGISTER);
	control &= ~B_SERIAL_INTF_CONTROL_R1_LOOP_SEL;
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+SERIAL_INTF_CONTROL_REGISTER,control);
	// enable ADC interrupt
	control = pci->read_io_32(((es137x_hw*)card->hw)->iobase+SERIAL_INTF_CONTROL_REGISTER);
	control &= ~B_SERIAL_INTF_CONTROL_R1_INT_EN;
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+SERIAL_INTF_CONTROL_REGISTER,control);
	control |= B_SERIAL_INTF_CONTROL_R1_INT_EN;
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+SERIAL_INTF_CONTROL_REGISTER,control);
	// enable ADC
	control = pci->read_io_32(((es137x_hw*)card->hw)->iobase+ICS_CONTROL_REGISTER);
	control &= ~B_ICS_CONTROL_ADC_EN;
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+ICS_CONTROL_REGISTER,control);
    control &= ~B_ICS_CONTROL_ADC_STOP;  // record buffer transfers enabled
	control |= B_ICS_CONTROL_ADC_EN;
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+ICS_CONTROL_REGISTER,control);
	
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	return B_OK;
}



static status_t es137x_Init(sound_card_t * card)
{
	cpu_status cp;
	status_t err = B_OK;
	config_manager_for_driver_module_info *config_info;
	uint32 control = 0;
	void* pCodec =NULL;
	
	DB_PRINTF(("es1370_Init(): here! \n"));
				
	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(es137x_hw));
	if(!card->hw) 
	{
		DB_PRINTF(("es1370_Init(): malloc error!\n"));
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(es137x_hw));	// zero it out
	
	
    ((es137x_hw*)card->hw)->DevId = card->bus.pci.info.device_id;
	DB_PRINTF((" DevID = %d\n",((es137x_hw*)card->hw)->DevId));
	
	/*	init config manager*/
	
	err=get_module(	B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&config_info);
	if ( err != B_OK) 
	{
		DB_PRINTF(("Init SBPRO for config_info...\n"));
		return err;
	}
	
	err=B_ERROR;
	
	
	((es137x_hw*)card->hw)->codec_write_sem = create_sem(1, "write");
	if(((es137x_hw*)card->hw)->codec_write_sem  < B_NO_ERROR)
	{
		DB_PRINTF(("Error: codec_write_sem creation error!\n"));
		return ((es137x_hw*)card->hw)->codec_write_sem;
	}
	
	((es137x_hw*)card->hw)->codec_read_sem = create_sem(1, "read");
	if(((es137x_hw*)card->hw)->codec_read_sem  < B_NO_ERROR)
	{
		DB_PRINTF(("Error: codec_read_sem creation error!\n"));
		return ((es137x_hw*)card->hw)->codec_read_sem;
	}
		
	/* Initialize addresses of the hw functions in the card structure*/	
	/* Some initialization */
	((es137x_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((es137x_hw*)card->hw)->PcmPlaybackAreaId = -1; 

    ((es137x_hw*)card->hw)->iobase = card->bus.pci.info.u.h0.base_registers[0];
    
	card->mpu401_base = ((es137x_hw*)card->hw)->mpu401_base = 0;
	card->joy_base = ((es137x_hw*)card->hw)->joy_base = 0;
	
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370) 
	{
		control = 0;	
		// Enable GP bit ??
		control = 1<<30;              //B_ICS_CONTROL_XCTL1;
		// Enable the CODEC here, since both the topology & wave minicards
	    // need to access it anyway.
	    control |= 1<<1;             //B_ICS_CONTROL_CDC_EN;
	    
	    
	    cp = disable_interrupts();
		acquire_spinlock(&card->bus.pci.lock);
	    // Write it !
	    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER , control);

	 	spin(10);
	 	
	 	release_spinlock(&card->bus.pci.lock);
		restore_interrupts(cp);
	       
	    // reset the codec
	
		DB_PRINTF(("es1370 init: ak4531_init........ \n"));
				
	    err = ak4531_init(&pCodec, card, es1370_codec_reg_write);
	    
	   	if(err!=B_OK) 
	   	{
			DB_PRINTF(("es1370 init: ak4531_init error\n"));
	   		return err;
	   	}
	   	
	   	((es137x_hw*)card->hw)->codec = pCodec;
	   		 
	    {   // Poll the CSTAT bit, which tells if the CODEC is ready.
		    int counter = 0xffff;
		    uint32 status;
   			for(counter = 1000; counter>0;counter--)
				{	
					status = pci->read_io_32 (((es137x_hw*)card->hw)->iobase + ICS_STATUS_REGISTER);
		    		if( (status & B_ICS_STATUS_CSTAT) !=0 ) break;
					snooze(100);
		    	} 
    //		if(counter == 0) return B_ERROR;
	
			ak4531_Setup(pCodec);
	
		}
		
			
	}
	else
	if((((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1371) ||
		(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES5880))
	{
	    DB_PRINTF(("es1370 init: ac97_init........ \n"));
	    
	    cp = disable_interrupts();
		acquire_spinlock(&card->bus.pci.lock);
		
		//disabling UART interrupts
		pci->write_io_8(((es137x_hw*)card->hw)->iobase + 9,0x00);		

		//disabling serial interrupts
		pci->write_io_32(((es137x_hw*)card->hw)->iobase + 0x21,pci->read_io_8(((es137x_hw*)card->hw)->iobase +0x21) & ~7);		
	
		//disabling card interrupts and dev's enabling ??
		pci->write_io_32(((es137x_hw*)card->hw)->iobase,0x0cff0000UL);	
					    
	 	 	
	 	release_spinlock(&card->bus.pci.lock);
		restore_interrupts(cp);
	
	    SRCInit(card);
		/* Init codec */
		if ((err = es1371_InitCodec(card))) 
		{
			derror("es1370_Init: es1370_InitCodec error!\n");
			 return err;
		}

	}
		
    /* Install isr */
	DB_PRINTF(("*** es1370_Init(): INT line = %d\n", card->bus.pci.info.u.h0.interrupt_line));
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, es137x_isr, card, 0);
	
	// set sample rate
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
	{
		hw_set_sample_rate(card, 41000);
	}
	    
	return B_OK;
}

/***************************************************************************/
static status_t es137x_Uninit(sound_card_t * card)
{
	DB_PRINTF(("es1370_Uninit(): here! \n"));
	
	delete_sem(((es137x_hw*)card->hw)->codec_write_sem);
	delete_sem(((es137x_hw*)card->hw)->codec_read_sem);
	
	/* Remove  isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, es137x_isr, card);
			
	if(((es137x_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((es137x_hw*)card->hw)->PcmCaptureAreaId);
	if(((es137x_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((es137x_hw*)card->hw)->PcmPlaybackAreaId);
    if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1371 ||
       ((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES5880)
	    ac97uninit(&((es137x_hw*)card->hw)->ac97codec); 
    else
    	ak4531_uninit(((es137x_hw*)card->hw)->codec); 
       
	/* Clean up */
	free(card->hw);

	return B_OK;
}
/**************************************************************************/

static void es137x_StartPcm(sound_card_t *card)
{
	es137x_PcmStartPlayback(card);
	es137x_PcmStartCapture(card);
}


static void es137x_StopPcm(sound_card_t *card)
{
	cpu_status cp;
	uint32 control;
	
	DB_PRINTF(("es1370_PcmStopPcm\n"));

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	
    // Pause...
    control = pci->read_io_32 (((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control |= B_SERIAL_INTF_CONTROL_P2_PAUSE;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);

    // Stop by disabling DAC2
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER);
    control &= ~B_ICS_CONTROL_DAC2_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase +ICS_CONTROL_REGISTER,control);
    
    // disable ADC 
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase +ICS_CONTROL_REGISTER);
    control &= ~B_ICS_CONTROL_ADC_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase+ICS_CONTROL_REGISTER,control);

    // disable DAC/ADC interrupts
    control = pci->read_io_32 (((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~B_SERIAL_INTF_CONTROL_R1_INT_EN;
    control &= ~B_SERIAL_INTF_CONTROL_P2_INTR_EN;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER,control);
    
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void es137x_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	uint32 control;
	uint32 ratio;
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
		((es137x_hw*)card->hw)->PcmPlaybackSR = 44100; 
	else	
		((es137x_hw*)card->hw)->PcmPlaybackSR = 48000;
}

static status_t es137x_SetPlaybackDmaBuf(sound_card_t *card,  uint32 size, void** log_addr)
{
    cpu_status cp;
	void* phys_addr;
	area_id AreaId;
	size_t tmp_size = size;
	
	
	DB_PRINTF(("In es1370_SetPlaybackDmaBuf\n"));

	AreaId = allocate_memory("es1370_PlaybackDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		DB_PRINTF(("Error: Memory allocation for es1370__PlaybackDmaBuf failed!!\n"));
		return AreaId;
	}
	if(((es137x_hw*)card->hw)->PcmPlaybackAreaId >= 0)
		 delete_area(((es137x_hw*)card->hw)->PcmPlaybackAreaId);
	((es137x_hw*)card->hw)->PcmPlaybackAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

	((es137x_hw*)card->hw)->PcmPlaybackBufSize = size;
	
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// Memory page register = 1100b.
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + MEMORY_PAGE_REGISTER,0x0000000C);
    // Program the DAC2 frame register 1
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + DAC2_FRAME_REGISTER_1,(uint32)phys_addr);
    // Program the DAC2 frame register 2
    // longwords:  tmp_size in bytes => /4
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + DAC2_FRAME_REGISTER_2, size/4 -1);
     
    // Program DAC2 channel sample count register
    // 16-bit samples,stereo,half of buffer size
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + DAC2_CHANNEL_SAMPLE_COUNT_REGISTER, size/8 -1);
    
    release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp); 
	
	DB_PRINTF(("\t\tes1370_SetPlaybackDmaBuf OK\n"));
	return B_OK;
}

static void es137x_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	cpu_status cp;
	uint32 mask;
	uint32 control;
		
	DB_PRINTF(("es1370_SetPlaybackFormat\n"));

    if ((num_of_bits == 16) && (num_of_ch == 2)) mask= 3;
	if ((num_of_bits == 16) && (num_of_ch != 2)) mask= 2;	
	if ((num_of_bits == 8) &&  (num_of_ch == 2)) mask= 1;
	if ((num_of_bits == 8) &&  (num_of_ch != 2)) mask= 0;
	
		
	mask <<= 2;
	
    cp = disable_interrupts();
    acquire_spinlock(&card->bus.pci.lock);
	
    control = pci->read_io_32 (((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER);
    control &= ~(3<<2);
    control |= mask;
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER, control);
    
 	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);  
	
	DB_PRINTF(("es1370_SetPlaybackFormat OK\n"));
}

//  set capture sample rate
static void es137x_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	DB_PRINTF(("es1370_SetCaptureSR\n"));
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
		((es137x_hw*)card->hw)->PcmPlaybackSR = 44100;
	else	
		((es137x_hw*)card->hw)->PcmCaptureSR = 48000;
}

static status_t es137x_SetCaptureDmaBuf(sound_card_t *card, uint32 size, void** log_addr)
{
	void* phys_addr;
	cpu_status cp;
	area_id AreaId;
	size_t tmp_size = size;
	
	DB_PRINTF(("In es1370_SetCaptureDmaBuf\n"));

	AreaId = allocate_memory("es1370_CaptureDmaBuf",
				log_addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		DB_PRINTF(("Error: Memory allocation for es1370__PlaybackDmaBuf failed!!\n"));
		DB_PRINTF(("es1370_SetPlaybackDmaBuf: return %ld;\n", AreaId));
		return AreaId;
	}
	
	if(((es137x_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((es137x_hw*)card->hw)->PcmCaptureAreaId);
	((es137x_hw*)card->hw)->PcmCaptureAreaId = AreaId; 
	memset(*log_addr, 0x0, tmp_size);

	((es137x_hw*)card->hw)->PcmCaptureBufSize = size;
	((es137x_hw*)card->hw)->PcmCaptureBufLogAddr = *log_addr;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);	
	// Memory page register = 1101b.
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + MEMORY_PAGE_REGISTER,0x0000000D);
    // Program the ADC frame register 1
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ADC_FRAME_REGISTER_1,(uint32)phys_addr);
    // Program the ADC frame register 2
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ADC_FRAME_REGISTER_2,size/4-1);
    
    // Program ADC channel sample count register
    // 16-bit samples,stereo,half of buffer size
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ADC_CHANNEL_SAMPLE_COUNT_REGISTER,size/8 -1);
    
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	DB_PRINTF(("es1370_SetCaptureDmaBuf OK\n"));
	return B_OK;
}

static void es137x_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uchar stereo;
	uchar bits16;
	uint32 mask;
	uint8 bits;
	uint32 control;
		
	cpu_status cp;
	
	DB_PRINTF(("es1370_SetCaptureFormat \n"));
	
	// prepare mask
	if ((num_of_bits == 16) && (num_of_ch == 2)) mask= 3;
	if ((num_of_bits == 16) && (num_of_ch != 2)) mask= 2;	
	if ((num_of_bits == 8) &&  (num_of_ch == 2)) mask= 1;
	if ((num_of_bits == 8) &&  (num_of_ch != 2)) mask= 0;
	mask <<= 4;
    
    control = pci->read_io_32(((es137x_hw*)card->hw)->iobase +SERIAL_INTF_CONTROL_REGISTER);
    control &= ~(3<<4);
    control |= mask;
    
    cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
    // write it
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + SERIAL_INTF_CONTROL_REGISTER, control);
	
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	DB_PRINTF(("es1370_SetCaptureFormat OK\n"));
}


static status_t es137x_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	DB_PRINTF(("In es1370_SoundSetup\n"));
	((es137x_hw*)card->hw)->ss = *ss;	
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
		((es137x_hw*)card->hw)->ss.sample_rate = kHz_44_1;
	else
		((es137x_hw*)card->hw)->ss.sample_rate = kHz_48_0;
	
    if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1371 ||
       ((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES5880)
	{
		ret = AC97_SoundSetup(&((es137x_hw*)card->hw)->ac97codec, ss);
		if(ret!=B_OK)return ret;
		AC97_GetSoundSetup(&((es137x_hw*)card->hw)->ac97codec,ss);
    } 
    else
    if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1370)
    {
		ak4531_SoundSetup(((es137x_hw*)card->hw)->codec, &((es137x_hw*)card->hw)->ss);
	}	
	*ss=((es137x_hw*)card->hw)->ss; //set back current values
	return B_OK;
}


// it fils sound setup data struct
static status_t  es137x_GetSoundSetup(sound_card_t *card, sound_setup *ss) 
{
	if(((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES1371 ||
	   ((es137x_hw*)card->hw)->DevId == PCI_DEVICE_ID_ENSONIQ_ES5880)
	{
		status_t ret;
		ret = AC97_GetSoundSetup(&((es137x_hw*)card->hw)->ac97codec,ss);
		DB_PRINTF(("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate));
		return ret;
	}
	
	*ss = ((es137x_hw*)card->hw)->ss;	
	return B_OK;
}

/************************************************************/
static void hw_set_sample_rate(sound_card_t* card, uint32 sample_rate)
{
    uint32 control;
    uint32 ratio;
    cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
         
    control = pci->read_io_32 (((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER); 
    // Programmable clock freq = Master clock freq/ratio
	// ratio = ((MASTER_CLOCK_FREQUENCY/16)/sample_rate)-2;
	// linux uses the above formula (plus rounding)
	// but this one sounds better.  --marc
	ratio = ((MASTER_CLOCK_FREQUENCY/16)/sample_rate)-4;

    // Set the programmable clock divide ratio.
    control &= ~B_ICS_CONTROL_PCLKDIV;   //	((0x1fff)<<16)
    control |= (ratio<<16);
    // Write it.
    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ICS_CONTROL_REGISTER, control);
    
    release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
 }


static void es1370_codec_reg_write(void* the_card, uint8 reg, uint8 value)
{
	sound_card_t* card = (sound_card_t*)the_card;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
    
    // write it out
    pci->write_io_16(((es137x_hw*)card->hw)->iobase + CODEC_WRITE_REGISTER,  
                     ((reg<<8)|value) );
    // wait for a bit while...
    spin(ES1370_CODEC_WRITE_DELAY);
    
    release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

}


static status_t es1371_InitCodec(sound_card_t *card)
{
	cpu_status cp;
	status_t err = B_OK;
	pci_info *pi;
	DB_PRINTF(("In es1371_InitCodec\n"));
	
	pi = &card->bus.pci.info;
	// 1371 revisions 4 and 7 need their reset-bit twiddled before they'll work
	// I've only seen a rev. 2 5880, but I'll assume all revisions need to be reset in a similar fashion
	if((pi->device_id == 0x1371 && (pi->revision == 4 || pi->revision == 7))||
	   (pi->device_id == 0x5880 /*&& pi->revision == 2*/))
	{
		dprintf("es137x device %04x revision %d, resetting\n",pi->device_id,pi->revision);
	    cp = disable_interrupts();
		acquire_spinlock(&card->bus.pci.lock);
	    pci->write_io_32(((es137x_hw*)card->hw)->iobase + ICS_STATUS_REGISTER , 0x20000000); // 5880 reset bit
	 	spin(10);
		release_spinlock(&card->bus.pci.lock);
		restore_interrupts(cp);
	}

	err = ac97init(&((es137x_hw*)card->hw)->ac97codec, 
					(void*)card,
					es1371_CodecWrite,
					es1371_CodecRead);
					
	if(err!=B_OK) dprintf("es1371_InitCodec(): ac97init err = %ld!\n",err);
	es1371_CodecWrite(card,0x1e,0xf,0xffff);
	return err;				
}


static uint16 es1371_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	if(acquire_sem(((es137x_hw*)card->hw)->codec_read_sem) != B_NO_ERROR)
	{
		DB_PRINTF(("Error: can't get Codec_read_sem !\n"));
		return B_ERROR;
	}

	ret = es1371_CodecReadUnsecure(card, offset);
	
	release_sem(((es137x_hw*)card->hw)->codec_read_sem);

	return ret;
}


static uint16 es1371_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
    int i;
	uint32 dtemp;
	uint32 dinit;
	uint32 dDataRdy = 1UL << 31;   
	
	// wait for WIP to go away
	for (i=ES1371_CODEC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x14) & (1UL << 30)); i--)
		;
	if (!i)
	{
		DB_PRINTF(("es1371CodecRead: WIP timeout!"));
	}
	
	// synchronize with SRC, interrupts should aready be disabled
	
	// save the current SRC state for later
	dinit = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10);
	
	// wait for SRC to be ready
	for (i=SRC_POLL_COUNT; i; i--)
	{
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10);
		if (!(dtemp & 0x00800000UL))
			break; 
	}
	if (!i)
	{
		DB_PRINTF(("es1371CodecRead: SRC poll timeout!"));
	}
	
	// enable SRC state data
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,(dtemp & 0x00780000UL) | (1UL << 16));
	
	// wait for OK state
	for (i=SRC_OK_POLL_COUNT; i; i--)
	{
		if ((pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00870000UL) == (SRC_OK_STATE << 16))
			break; 
	}
	if (!i)
	{
		DB_PRINTF(("es1371CodecRead: SRC OK poll timeout!"));
	}
	
	// send read request to es1371Codec
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x14,((uint32)offset << 16) | (1UL << 23));
	
	// wait for the es1371Codec answer
	for (i=ES1371_CODEC_POLL_COUNT; i; i--)
	{
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x14);
		if (dtemp & dDataRdy)
			break;
	}
	if (!i)
	{
		DB_PRINTF(("es1371CodecRead: READY timeout!"));
	}

    // restore SRC state
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,dinit);
	
	//DB_PRINTF(("... es1371CodecRead(...0x%x) = 0x%04x\n",offset,(uint16)dtemp));
	return (uint16)dtemp;
}




static status_t es1371_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	int i;
	uint32 dtemp, dinit;
	sound_card_t* card = (sound_card_t*) host;
	status_t ret = B_ERROR;
	int attempt = 10;

	if (mask == 0) return B_OK;
	
	if(acquire_sem(((es137x_hw*)card->hw)->codec_write_sem) != B_NO_ERROR)
	{
		DB_PRINTF(("Error: can't get Codec_write_sem !\n"));
		return B_ERROR;
	}
	

	if (mask != 0xffff) {
		uint16 old  = es1371_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}
	
    // wait for WIP to go away
	for (i=ES1371_CODEC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase + 0x14) & (1UL << 30)); i--)
		;
	if (!i)
	{
		dprintf("es1371CodecWrite: WIP timeout!");
	}
	
	// synchronize with SRC, interrupts should aready be disabled
	
	// save the current SRC state for later
	dinit = pci->read_io_32(((es137x_hw*)card->hw)->iobase + 0x10);
  
	// wait while busy
	for (i=SRC_POLL_COUNT; i; i--)
	{
		// check BUSY bit
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase + 0x10);
		if(!(dtemp & (0x1<<23)))  /*SRC_RAM_BUSY*/ /*0x00800000UL*/
		   break;
	}
	if (!i)
	{
		dprintf("es1371CodecWrite: SRC poll timeout!");
	}
	
	// enable SRC state data
	pci->write_io_32(((es137x_hw*)card->hw)->iobase +0x10,(dtemp & 0x00780000UL) | (1UL << 16));
	
	// wait for OK state
	for (i=SRC_OK_POLL_COUNT; i; i--)
	{
		if ((pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00870000UL) == (SRC_OK_STATE << 16))
			break; 
	}
	if (!i)
	{
		dprintf("es1371CodecWrite: SRC OK poll timeout!");
	}

	// write to es1371Codec
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x14,((uint32)offset << 16) | value);
	
	// restore SRC state
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,dinit);

	release_sem(((es137x_hw*)card->hw)->codec_write_sem);

	return ret;
}

static void es1371_UninitCodec(sound_card_t *card)
{
	ac97uninit(&((es137x_hw*)card->hw)->ac97codec); 
}

static void SRCInit(sound_card_t *card)
{
	int i;
	cpu_status cp;
  	dprintf("SRCInit");
		
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00800000UL); i--)
		;
	if (!i)
	{
		dprintf("SRCInit: poll timeout!");
	}
	
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// disable SRC	
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,0x00400000UL);
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
	
	// clear RAM
	for (i=0; i<0x80; i++)
		SRCRegWrite(card,i,0);
		
	// setup defaults
	SRCRegWrite(card,0x70+0, 16 << 4);	// synth TRUNC_N
	SRCRegWrite(card,0x70+1, 16 << 10);	// synth INT_REGS
	SRCRegWrite(card,0x74+0, 16 << 4);	// DAC TRUNC_N
	SRCRegWrite(card,0x74+1, 16 << 10); // DAC INT_REGS
	
	// setup volumes
	SRCRegWrite(card,0x7c, 1 << 12);	// synth L
	SRCRegWrite(card,0x7d, 1 << 12);	// synth R
	SRCRegWrite(card,0x7e, 1 << 12);	// DAC L
	SRCRegWrite(card,0x7f, 1 << 12);	// DAC R
	SRCRegWrite(card,0x6c, 1 << 12);	// ADC L
	SRCRegWrite(card,0x6d, 1 << 12);	// ADC R
	
		
	// setup rates
	SRCSetADCRate(card, 48000); // Recording 
	SRCSetDACRate(card, 48000); // Playback

    cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00800000UL); i--)
		;
	if (!i)
	{
		dprintf("SRCInit: poll timeout!");
	}
	
	// enable SRC	
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,0);
	
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void SRCRegWrite(sound_card_t *card, int reg, uint16 val)
{
	int i;
	uint32 dtemp;
	cpu_status cp;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);	
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i; i--)
	{
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10);
		if (!(dtemp & 1UL << 23))  //0x00800000UL))
			break;
	}
	if (!i)
	{
		dprintf("SRCRegWrite: poll timeout!");
	}

	// do write request	
	dtemp &= 0x00780000UL; // save control bits, clear everything else
	dtemp |= reg << 25;	   // OR in register address
	dtemp |= 1UL << 24;    //0x01000000UL; // OR in write enable
	dtemp |= val;		   // OR in register value
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10,dtemp);
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

}

static void SRCSetDACRateWithAdjustment(sound_card_t *card, uint16 rate, int adjustment)
{
	int i;
	uint32 dtemp, freq;
	int offset = 0x74;
	double adc_real_rate;
	cpu_status cp;	
	
    dtemp = 1UL << 20;	// DAC freeze
    
	if (rate > 48000)
		rate = 48000;
	else
	if (rate < 4000)
		rate = 4000;


	{	// calculate real value of the corresponding adc rate 
		uint32 N = rate / 3000;
		if (N == 15 || N == 13 || N == 11 || N == 9)
			N--;
		freq = (uint32)(((48000UL << 15) / rate) * N + 0.5);
		adc_real_rate = (48000UL <<15)/((double)freq/N);
	}


	// compute and set rate registers
	freq = (uint32)((adc_real_rate * (1 << 15 )) / 3000);	
	freq += adjustment;

 
    cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00800000UL); i--)
		;
	if (!i)
	{
		dprintf("SRCSetRate: poll timeout!");
	}
	
	// freeze channel
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10, (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00780000UL) | dtemp);
    release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);

	SRCRegWrite(card,offset+1, // INT_REGS
		(SRCRegRead(card,offset+1) & 0xff) | ((freq >> 5) & 0xfc00));
	SRCRegWrite(card,offset+3, // VFREQ_FRAC
		freq & 0x7fff);
		
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i && (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00800000UL); i--)
		;
	if (!i)
	{
		dprintf("SRCSetRate: poll timeout!");
	}
	
	// unfreeze channel
    pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10, (pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10) & 0x00780000UL) & ~dtemp);
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}

static void SRCSetDACRate(sound_card_t *card, uint16 rate)
{
	SRCSetDACRateWithAdjustment(card, rate, 0);
}

static void SRCSetADCRate(sound_card_t *card, uint16 rate)
{
	double real_rate;
	uint32 freq, N, truncM, truncStart;
	int offset = 0x78;
	
	if (rate > 48000)
		rate = 48000;
	else
	if (rate < 4000)
		rate = 4000;

	// derive oversample ratio
	N = rate / 3000;
	if (N == 15 || N == 13 || N == 11 || N == 9)
		N--;
	// calculate new frequency and real rate 
	freq = (uint32)(((48000UL << 15) / rate) * N + 0.5);
	real_rate = (48000UL <<15)/((double)freq/N);
			
	// truncate filter and write n/trunc start
	truncM = (21 * N - 1) | 1;
	if (rate >= 24000)
	{
		if (truncM > 239)
			truncM = 239;
		truncStart = (239 - truncM) >> 1;
		SRCRegWrite(card,offset+0, // TRUNC_N
			(truncStart << 9) | (N << 4));
	}
	else
	{
		if (truncM > 119)
			truncM = 119;
		truncStart = (119 - truncM) >> 1;
		SRCRegWrite(card,offset+0, // TRUNC_N
			0x8000 | (truncStart << 9) | (N << 4));
	}
	
	dprintf("adc calculated real_rate = %ld\n",(uint32)(real_rate*1000));
	SRCRegWrite(card,offset+1, // INT_REGS
		(SRCRegRead(card,offset+1) & 0xff) | ((freq >> 5) & 0xfc00));
	SRCRegWrite(card,offset+3, // VFREQ_FRAC_OFF
		freq & 0x7fff);
		
	SRCRegWrite(card,0x6c, // ADC volume L
		N << 8);
	SRCRegWrite(card,0x6d, // ADC volume R
		N << 8);
}

static uint16 SRCRegRead(sound_card_t *card, int reg)
{
	int i;
	uint32 dtemp, orig;
	cpu_status cp;	
	
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);			
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i; i--)
	{
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10);
		if (!(dtemp & 0x00800000UL))
			break;
	}
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
	if (!i)
	{
		dprintf("SRCRegRead: poll timeout!");
	}

	// synchronize SRC reads; interrupts should already be disabled
	
	orig = dtemp;
	
	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);
	// do read request, but expose state bits!
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10, (dtemp & 0x00780000UL) | (1 << 16) | (reg << 25));
	
	// wait until SRC isn't busy
	for (i=SRC_POLL_COUNT; i; i--)
	{
		dtemp = pci->read_io_32(((es137x_hw*)card->hw)->iobase+0x10);
		if (!(dtemp & 0x00800000UL))
			break;
	}
	if (!i)
	{
		dprintf("SRCRegRead: poll timeout!");
	}

	// hide state bits by doing a normal read request
	pci->write_io_32(((es137x_hw*)card->hw)->iobase+0x10, (orig & 0x00780000UL) | (reg << 25));
	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
	// return result we got in the OK state

	dprintf("... SRCRegRead(...,0x%x) = 0x%04x\n",
			reg,(uint16)dtemp);

	return (uint16)dtemp; 	
}



static uchar mpu401_read_port(void* p_hw, int port)
{
	es137x_hw* hw =  (es137x_hw*)p_hw;
  
    uint8 stat;
   
	if((port - hw->mpu401_base)%2) //status port
	{
		stat = pci->read_io_8(hw->iobase+9);
		return (((~stat & (0x1<<1)) >> 1) << 6) | ((~stat & 0x1) << 7);
	}
	else //data port
	{
		if(hw->mpu401_emul_ack>0) //emulate mpu401 ack
		{
			hw->mpu401_emul_ack--;
			return 0xfe; //return ack
		}
		else
		{
			return pci->read_io_8(hw->iobase+8);
		}
	}
	return 0xff;
	
}

static void mpu401_write_port(void* p_hw, int port, uchar value)
{
    es137x_hw* hw =  (es137x_hw*)p_hw;
	if((port - hw->mpu401_base)%2) //command port
	{
		if(value == 0xff) //reset
		{
			hw->mpu401_uart_mode = false;
			hw->mpu401_emul_ack = 1;
			//reset
			pci->write_io_8(hw->iobase+9,0x3);
			pci->write_io_8(hw->iobase+9,0x00);
		}
		else if(!(hw->mpu401_uart_mode))
		{
			hw->mpu401_emul_ack++;
			if(value == 0x3f) hw->mpu401_uart_mode = true; // switch to uart mode
		}
		else //it is in uart mode
		{
		 //do nothing
		}		
	}
	else //data port
	{
		pci->write_io_8(hw->iobase+8, value);	/* push the byte out */
	}
	
}

