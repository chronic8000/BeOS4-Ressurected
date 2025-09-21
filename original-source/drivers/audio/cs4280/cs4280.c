/************************************************************************/
/*                                                                      */
/*                              cs4280.c                              	*/
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
#include "cs4280.h"
#include "cwimage.h"

#define AC97_BAD_VALUE		(0xffff)


extern pci_module_info *pci;

typedef struct _timers_data_t {
	vuint64 IntCount;
	bigtime_t SystemClock;
	int32 Lock;
} timers_data_t;

typedef struct _cs4280_hw {

	timers_data_t timers_data;	
	ac97_t codec;

	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	void * PcmPlaybackBuf;

	area_id PcmCaptureAreaId;
	size_t PcmCaptureBufSize; 
	void * PcmCaptureBuf;

	volatile uchar * BA0;
	volatile uchar * BA1;
	area_id BA0_map;
	area_id BA1_map;
	
	uint32 StartCapture;
	uint32 StartPlay;
	
	sem_id ac97sem;
	int32 ac97ben;

	int32 SRHz;
	uint32 format;

} cs4280_hw; 

static uint32 READ_REG( volatile uchar * base, int32 reg)
{
	return *(volatile uint32*)(base+reg);
}

static void WRITE_REG( volatile uchar * base, uint32 reg, uint32 value, uint32 mask)
{
	if (mask != 0xffffffff) {
		value = (value & mask) | (READ_REG(base, reg)& ~mask);
	}
	*(volatile uint32*)(base+reg) = value;
}


static status_t 	cs4280_Init(sound_card_t * card);
static status_t 	cs4280_Uninit(sound_card_t * card);
static void 		cs4280_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	cs4280_SetPlaybackDmaBuf(sound_card_t *card,  uint32 *size, void** addr);
static status_t 	cs4280_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 size, void** addr);
static void 		cs4280_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		cs4280_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		cs4280_SetCaptureDmaBuf(sound_card_t *card, uint32 *size, void** addr);
static status_t		cs4280_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** addr);
static void 		cs4280_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		cs4280_StartPcm(sound_card_t *card);
static void 		cs4280_StopPcm(sound_card_t *card);
static status_t		cs4280_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		cs4280_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		cs4280_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		cs4280_InitJoystick(sound_card_t * card);
static void 		cs4280_enable_gameport(sound_card_t * card);
static void 		cs4280_disable_gameport(sound_card_t * card);

static status_t 	cs4280_InitMPU401(sound_card_t * card);
static void 		cs4280_enable_mpu401(sound_card_t *card);
static void 		cs4280_disable_mpu401(sound_card_t *card);
static void 		cs4280_enable_mpu401_interrupts(sound_card_t *card);
static void 		cs4280_disable_mpu401_interrupts(sound_card_t *card);

static sound_card_functions_t cs4280_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&cs4280_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&cs4280_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&cs4280_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&cs4280_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&cs4280_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&cs4280_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&cs4280_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&cs4280_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&cs4280_SetPlaybackDmaBufOld,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&cs4280_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&cs4280_SetCaptureSR,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32 size);*/
&cs4280_SetCaptureDmaBufOld,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&cs4280_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&cs4280_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&cs4280_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&cs4280_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&cs4280_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&cs4280_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&cs4280_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&cs4280_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&cs4280_disable_mpu401_interrupts,

NULL,     // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports
NULL, // nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 

&cs4280_SetPlaybackDmaBuf,
&cs4280_SetCaptureDmaBuf
} ;


card_descrtiptor_t cs4280_audio_descr = {
	"cs4280",
	PCI, 
	&cs4280_functions,
	0x1013, // Cirrus Logic 
	0x6003L // CS4280,
};
 
static status_t 	cs4280_InitCodec(sound_card_t *card);
static void 		cs4280_UninitCodec(sound_card_t *card);
static uint16 		cs4280_CodecRead(void* host, uchar offset);
static status_t 	cs4280_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);

/* --- 
	Interrupt service routine
	---- */
static uint32 b0count=0,b1count=0,count=0;
static int32 cs4280_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	sound_card_t *card = (sound_card_t *) data;
	uint32 istat;
	static int half = 0;
	static int chalf= 0;

	acquire_spinlock(&card->bus.pci.lock);
	istat = READ_REG(((cs4280_hw*)card->hw)->BA0, BA0_HISR);

	if (!(istat & (0x1|0x2|0x100000))) {	/*	not one of ours	*/
		release_spinlock(&card->bus.pci.lock);
		return ret;
	}
	//	ask another interrupt
	ret = B_HANDLED_INTERRUPT;
	WRITE_REG(((cs4280_hw*)card->hw)->BA0, BA0_HICR, 0x3, -1);
	release_spinlock(&card->bus.pci.lock);

	if (istat & 0x1) {
		//fixme:	figure out which half of the playback buffer

		{// Store internal time for synchronization 
			bigtime_t time = system_time();
			
			acquire_spinlock(&((cs4280_hw*)card->hw)->timers_data.Lock);	
			((cs4280_hw*)card->hw)->timers_data.SystemClock = time;
			((cs4280_hw*)card->hw)->timers_data.IntCount++;
			release_spinlock(&((cs4280_hw*)card->hw)->timers_data.Lock);
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

}


static void cs4280_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
{

	cpu_status cp;
	uint32 num_samples_per_half_buf;
 	double half_buf_time;
	uint64 int_count;

	cp = disable_interrupts();	// critical section
 	acquire_spinlock(&((cs4280_hw*)card->hw)->timers_data.Lock);

	*pSystemClock = ((cs4280_hw*)card->hw)->timers_data.SystemClock;	// system time
	int_count = ((cs4280_hw*)card->hw)->timers_data.IntCount;			// interrupt count

	release_spinlock(&((cs4280_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp); 	// end of the critical section
	
	// number of samples in half-buffer
	num_samples_per_half_buf = ((cs4280_hw*)card->hw)->PcmPlaybackBufSize 
								/2		//half of the buffer
								/2 		//16-bit format of samples
								/2;		//stereo

	// compute the time elapsed using the number of played-back samples in microsec
	half_buf_time = (double)num_samples_per_half_buf / (double)(((cs4280_hw*)card->hw)->SRHz) * 1000000.0; 	
 	*pSampleClock = (bigtime_t)((double)int_count * half_buf_time + 0.5);

}

static void cs4280_enable_mpu401(sound_card_t *card)
{
}

static void cs4280_disable_mpu401(sound_card_t *card)
{
}

static void cs4280_enable_mpu401_interrupts(sound_card_t *card)
{
}

static void cs4280_disable_mpu401_interrupts(sound_card_t *card)
{
}


static void cs4280_enable_gameport(sound_card_t * card)
{
}

static void cs4280_disable_gameport(sound_card_t * card)
{
}

static status_t cs4280_InitJoystick(sound_card_t * card)
{
	return ENODEV;
}

static status_t cs4280_InitMPU401(sound_card_t * card)
{
	return ENODEV;
}


static status_t cs4280_PcmStartPlayback(sound_card_t * card)
{
	DB_PRINTF(("cs4280_PcmStartPlayback\n"));
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PLAYBACK_VOLUME, 0xc000c000, -1);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PFIE, 0, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_PLAY, ((cs4280_hw*)(card->hw))->StartPlay, 0xffff0000);
	return B_OK;
}

static status_t cs4280_PcmStartCapture(sound_card_t * card)
{
	DB_PRINTF(("cs4280_PcmStartCapture\n"));

   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_CAPTURE_VOLUME_REG, 0x80008000, -1);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_CAPTURE_INTERRUPT_ENABLE_REG, 1, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_CAPTURE, ((cs4280_hw*)(card->hw))->StartCapture, 0x0000ffff);
	return B_OK;
}


static void lock_ac97(sound_card_t * card)
{
	if (atomic_add(&((cs4280_hw*)card->hw)->ac97ben, 1) > 0)
		acquire_sem(((cs4280_hw*)card->hw)->ac97sem);
}
static void unlock_ac97(sound_card_t * card)
{
	if (atomic_add(&((cs4280_hw*)card->hw)->ac97ben, -1) > 1)
		release_sem_etc(((cs4280_hw*)card->hw)->ac97sem, 1, B_DO_NOT_RESCHEDULE);
}

static void duh(short *buf, int frames)
{
	while (frames-- > 0) {
		*buf++ = ((frames & 0x3f)-0x20)*1000;
		*buf++ = ((frames & 0x3f)-0x20)*1000;
	}
}

static status_t cs4280_Init(sound_card_t * card)
{
	uint32 temp1, temp2, i;
	status_t err = B_OK;
	uint16 value = 0;

	DB_PRINTF(("cs4280_Init(): here! \n"));

	
	/*	Enable PCI config space	*/
	value = (*pci->read_pci_config)(
			card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function,
			0x04, 0x02);

	(*pci->write_pci_config)(
			card->bus.pci.info.bus, card->bus.pci.info.device, card->bus.pci.info.function,
			0x04, 0x02, value | 0x0006);

	/* Allocate memory for the card data structure */
	card->hw = malloc(sizeof(cs4280_hw));
	if(!card->hw) {
		derror("cs4280_Init(): malloc error!\n");
        cs4280_Uninit(card);
		return ENOMEM;
	}
	memset(card->hw, 0, sizeof(cs4280_hw));	// zero it out

	/* Some initialization */
	((cs4280_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((cs4280_hw*)card->hw)->PcmPlaybackAreaId = -1; 
	((cs4280_hw*)card->hw)->BA0_map = -1; 
	((cs4280_hw*)card->hw)->BA1_map = -1; 


	((cs4280_hw*)card->hw)->ac97sem = create_sem(1, "cs4280 ac97sem");
	if (((cs4280_hw*)card->hw)->ac97sem < 0) {
		derror("cs4280: cannot create ac97sem\n");
		return ((cs4280_hw*)card->hw)->ac97sem;
	}
	set_sem_owner(((cs4280_hw*)card->hw)->ac97sem, B_SYSTEM_TEAM);

	/* memory-map the IO */
	((cs4280_hw*)(card->hw))->BA0_map = map_physical_memory ( "cs4280 BA0 memory",
									(void *)card->bus.pci.info.u.h0.base_registers[0],
									card->bus.pci.info.u.h0.base_register_sizes[0],
									B_ANY_KERNEL_ADDRESS,
									B_READ_AREA | B_WRITE_AREA,
									(void **)&((cs4280_hw*)(card->hw))->BA0);
	if (((cs4280_hw*)(card->hw))->BA0_map < B_OK) {
		derror("cs4280_Init: Map memory (BA0) error!\n");
		err = ((cs4280_hw*)(card->hw))->BA0_map;
        cs4280_Uninit(card);
		return err;
	}								
	((cs4280_hw*)(card->hw))->BA1_map = map_physical_memory ( "cs4280 BA1 memory",
									(void *)card->bus.pci.info.u.h0.base_registers[1],
									card->bus.pci.info.u.h0.base_register_sizes[1],
									B_ANY_KERNEL_ADDRESS,
									B_READ_AREA | B_WRITE_AREA,
									(void **)&((cs4280_hw*)(card->hw))->BA1);
	if (((cs4280_hw*)(card->hw))->BA1_map < B_OK) {
		derror("cs4280_Init: Map memory (BA1) error!\n");
        err = ((cs4280_hw*)(card->hw))->BA1_map;
		cs4280_Uninit(card);
		return err;
	}								
 									
	ddprintf("cs4280: BA0 @ 0x%x,  BA1 @ 0x%x\n", ((cs4280_hw*)(card->hw))->BA0, ((cs4280_hw*)(card->hw))->BA1);

	((cs4280_hw*)card->hw)->PcmPlaybackAreaId = create_area("cs4280_PlaybackDmaBuf",
												&((cs4280_hw*)card->hw)->PcmPlaybackBuf, 
												B_ANY_KERNEL_ADDRESS, 
												4096,
												B_CONTIGUOUS,
												B_READ_AREA | B_WRITE_AREA);
	if (((cs4280_hw*)card->hw)->PcmPlaybackAreaId < 0) {
		derror("cs4280_Init: Memory allocation for cs4280__PlaybackDmaBuf failed!\n");
		err = ((cs4280_hw*)card->hw)->PcmPlaybackAreaId;
        cs4280_Uninit(card);
		return err;
	}
	memset(((cs4280_hw*)card->hw)->PcmPlaybackBuf, 0x00, 4096);
//	duh((short *)((cs4280_hw*)card->hw)->PcmPlaybackBuf, 1024);
	((cs4280_hw*)card->hw)->PcmPlaybackBufSize = 4096;

	((cs4280_hw*)card->hw)->PcmCaptureAreaId = create_area("cs4280_CaptureDmaBuf",
												&((cs4280_hw*)card->hw)->PcmCaptureBuf, 
												B_ANY_KERNEL_ADDRESS, 
												4096,
												B_CONTIGUOUS,
												B_READ_AREA | B_WRITE_AREA);
	if (((cs4280_hw*)card->hw)->PcmCaptureAreaId < 0) {
		derror("cs4280_Init: Memory allocation for cs4280__CaptureDmaBuf failed!\n");
		err = ((cs4280_hw*)card->hw)->PcmCaptureAreaId;
        cs4280_Uninit(card);
		return err;
	}
	memset(((cs4280_hw*)card->hw)->PcmCaptureBuf, 0x00, 4096);
	((cs4280_hw*)card->hw)->PcmCaptureBufSize = 4096;

	/* Install cs4280 isr */
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, cs4280_isr, card, 0);
	ddprintf("cs4280: int_line %d\n", card->bus.pci.info.u.h0.interrupt_line);

	/* enable AC-link */
	
	/* Clear clock */
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_CLKCR1, 0, -1);

   	/* Clear serial ports */
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERMC1, 0, -1);

    /* Use 'host-controlled AC-Link' */
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERACC, (SERACC_HSP | SERACC_CODEC_TYPE_1_03), -1); 

   	/* Drive the ARST# pinlow for a min of 1 microsecond.  This	*/
   	/* is done for non AC97 modes since there might be external	*/
   	/* logic that uses the ARST# line for a reset.				*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACCTL, 0, -1);
   	spin(100);            
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACCTL, ACCTL_RSTN, -1);
	spin(10);
    /* Enable sync generation. */
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACCTL, ACCTL_ESYN | ACCTL_RSTN, -1);

    /* Now wait 'for a short while' to allow the  AC97	*/
    /* part to start generating bit clock. (so we don't	*/
    /* Try to start the PLL without an input clock.)	*/
   	snooze(50000);

    /* Set the serial port timing configuration, so that the	  */
    /* clock control circuit gets its clock from the right place. */
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERMC1, SERMC1_PTC_AC97, -1);

   	/* Write the selected clock control setup to the hardware.  Don't	*/
   	/* turn on SWCE yet, so thqt the devices clocked by the output of	*/
   	/* PLL are not clocked until the PLL is stable.						*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_PLLCC, PLLCC_LPF_1050_2780_KHZ | PLLCC_CDR_73_104_MHZ , -1);
	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_PLLM,  0x3A, -1);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_CLKCR2, CLKCR2_PDIVS_8, -1);

   	/* Power up the PLL. */
	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_CLKCR1, CLKCR1_PLLP, -1);

   	/* Wait for the PLL to stabilize. */
   	snooze(50000);

    /* Turn on clocking of the core so we can set up serial ports. */
	WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_CLKCR1, CLKCR1_PLLP | CLKCR1_SWCE, -1);

	/*	this part is not documented in the databook; neither in	*/
	/*	the procedure overview nor in the registers section. Let's	*/
	/*	keep it anyway.		*/

   /*  Clear the serial fifo's to silence(0). */
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERBWP, 0, -1);

   /* Fill all 256 sample FIFO locations. */
   for (temp1=0; temp1<256; temp1++)
   {
        /* Make sure the previous FIFO write is complete. */
       for (temp2=0; temp2<5; temp2++) {
           snooze(1000);              // Wait a mil.
           if ( !(READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERBST) & SERBST_WBSY) )  // If write not busy,
               break;    // break from 'for' loop.
       }

       /* Write the serial port FIFO index. */
       WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERBAD, temp1, -1);
       /* Load the new value into the FIFO location. */
	   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERBCM, SERBCM_WRC, -1);
   }

   /*  Write the serial port configuration to the part.  The Master   */
   /*  Enable bit isn't set until all other values have been written. */
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERC1, SERC1_SO1F_AC97 | SERC1_SO1EN, -1);
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERC2, SERC2_SI1F_AC97 | SERC1_SO1EN, -1);
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_SERMC1, SERMC1_PTC_AC97 | SERMC1_MSPE, -1);

        // Wait for the codec ready signal from the AC97 codec.
   for(temp1=0; temp1<1000; temp1++)
   {
            // Delay a mil to let things settle out and
            // to prevent retrying the read too quickly.
       snooze(1*1000);
       if( READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACSTS) & ACSTS_CRDY )    // If ready,
           break;                                   //   exit the 'for' loop.
   }

   if( !(READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACSTS) & ACSTS_CRDY) ) {    // If never came ready,
	   derror("cs4280_Init(): AC link not ready (temp1 is %d) 0x%x!\n", temp1,READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACSTS));
       cs4280_Uninit(card);
       return B_ERROR;                                //   exit initialization.
   }
        // Assert the 'valid frame' signal so we can
        // begin sending commands to the AC97 codec.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACCTL, ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN, -1);
        // Wait until we've sampled input slots 3 & 4 as valid, meaning
        // that the codec is pumping ADC data across the AC link.
   for(temp1=0; temp1<1000; temp1++)
   {
            // Delay a mil to let things settle out and
            // to prevent retrying the read too quickly.
       snooze(1*1000);

            // Read the input slot valid register;  See
            // if input slots 3 and 4 are valid yet.
       if( (READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACISV) & (ACISV_ISV3 | ACISV_ISV4) ) ==
                  (ACISV_ISV3 | ACISV_ISV4)  )
            break;    // Exit the 'for' if slots are valid.
   }

            // If we never got valid data, exit initialization.
   if( (READ_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACISV) & (ACISV_ISV3 | ACISV_ISV4) ) !=
                  (ACISV_ISV3 | ACISV_ISV4)  ) {
           derror("cs4280_Init(): input slots error!\n");
           cs4280_Uninit(card);
           return B_ERROR;     // If no valid data, exit initialization.
   }
        // Start digital data transfer of audio data to the codec.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA0, BA0_ACOSV, ACOSV_SLV3 | ACOSV_SLV4, -1);

        //********************************************
        // Return from Initialize()-back in main()
        // Reset the processor(ProcReset()-from main()
        //********************************************
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_SPCR, SPCR_RSTSP, -1);    // Write the SP cntrl reg reset bit.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_SPCR, SPCR_DRQEN, -1);    // Write the SP cntrl reg DMAreqenbl bit.

         // Clear the trap registers.
   for(temp1=0; temp1<8; temp1++)
   {
       WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_DREG, DREG_REGID_TRAP_SELECT+temp1, -1);
       WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_TWPR, 0xFFFF, -1);
   }
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_DREG, 0, -1);

         // Set the frame timer to reflect the number of cycles per frame.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_FRMT, 0xADF, -1);

        //*********************************************************
        // Download the Processor Image(DownLoadImage()-from main()
        //**********************************************************
   temp2 = 0;           // offset into BA1Array(CWCIMAGE.H)
   for(i=0; i<INKY_MEMORY_COUNT; i++)
   {
	   uint32 offset,count;
       offset =  BA1Struct.MemoryStat[i].ulDestByteOffset;
       count  =  BA1Struct.MemoryStat[i].ulSourceByteSize;
       for(temp1=(offset); temp1<(offset+count); temp1+=4)
           WRITE_REG(((cs4280_hw*)(card->hw))->BA1, temp1, BA1Struct.BA1Array[temp2++], -1);
   }

        //*****************************************
        //  Save STARTSTOP_PLAY contents, replace
        //  high word with zero to stop PLAY.
        //*****************************************
   temp1 = READ_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_PLAY);
   ((cs4280_hw*)(card->hw))->StartPlay = temp1;
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_PLAY, 0, 0xffff0000);     // Stop play.

        //*******************************************
        //  Save STARTSTOP_CAPTURE contents, replace
        //  low word with zero to stop CAPTURE.
        //*******************************************
   temp1 = READ_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_CAPTURE);
   ((cs4280_hw*)(card->hw))->StartCapture = temp1 & 0x0000ffff;
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_CAPTURE, 0, 0x0000ffff);  // Stop capture.

	/* Init codec */
	if ((err = cs4280_InitCodec(card))) 
	{
		derror("cs4280_Init: cs4280_InitCodec error!\n");
        cs4280_Uninit(card);
		return err;
	}

	// set up chip to point to buffer
	{
		physical_entry ent;
		if ((err = get_memory_map(((cs4280_hw*)(card->hw))->PcmPlaybackBuf, 4096, &ent, 1)) < 0) {
			derror("cs4280: cannot get_memory_map() ?? (0x%x)\n", err);
			cs4280_Uninit(card);
			return err;
		}
		ddprintf("Physical play addr: 0x%x\n", ent.address);
		WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PLAY_BUFFER_ADDRESS, (uint32)ent.address, -1);

		if ((err = get_memory_map(((cs4280_hw*)(card->hw))->PcmCaptureBuf, 4096, &ent, 1)) < 0) {
			derror("cs4280: cannot get_memory_map() ?? (0x%x)\n", err);
			cs4280_Uninit(card);
			return err;
		}
		ddprintf("Physical capture addr: 0x%x\n", ent.address);
		WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_CAPTURE_BUFFER_ADDRESS, (uint32)ent.address, -1);

	}



		//	enable CPU here
        //**********************************************
        // Start the processor(ProcStart()--from main())
        //**********************************************
        // Set the frame timer to reflect
        // the number of cycles per frame.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_FRMT, 0xADF, -1);

        // Turn on the 'run', 'run at frame' and
        // 'DMA enable' bits in the  SP Control Reg.
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_SPCR, SPCR_RUN | SPCR_RUNFR | SPCR_DRQEN, -1);

        // Wait until the 'run at frame' bit
        // is reset in the SP control reg.
   for(temp1=0; temp1 < 25; temp1++)
   {
      snooze(1000);
      temp2 = READ_REG(((cs4280_hw*)(card->hw))->BA1, BA1_SPCR);
           // If 'run at frame' bit
           // is zero, quit waiting.
      if(!(temp2 & SPCR_RUNFR))
         break;
   }

        // If 'run at frame' never went
        // to zero, return error.
   if(temp2 & SPCR_RUNFR) {
      derror("cs4280_Init(): run at frame error!\n");
      cs4280_Uninit(card);
      return B_ERROR;
  }
        //**************************************
        // Unmute the Master and Alternate
        // (headphone) volumes.  Set to max.
        //**************************************
   WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PLAYBACK_VOLUME, 0xFFFFFFFF, -1);   // Max atten of playback vol.

	return B_OK;
}


static status_t cs4280_Uninit(sound_card_t * card)
{
	DB_PRINTF(("cs4280_Uninit(): here! \n"));

	/* turn off interrupts from the chip */
	WRITE_REG(((cs4280_hw*)card->hw)->BA0, BA0_HICR, 0x1, -1);

	/* Uninit codec */
	cs4280_UninitCodec(card);

	/* Remove CS4280 isr */
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, cs4280_isr, card);

	if (((cs4280_hw*)card->hw)->ac97sem > 0) delete_sem(((cs4280_hw*)card->hw)->ac97sem);

	if(((cs4280_hw*)card->hw)->BA0_map >= 0) delete_area(((cs4280_hw*)card->hw)->BA0_map);
	if(((cs4280_hw*)card->hw)->BA1_map >= 0) delete_area(((cs4280_hw*)card->hw)->BA1_map);

	if(((cs4280_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((cs4280_hw*)card->hw)->PcmCaptureAreaId);
	if(((cs4280_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((cs4280_hw*)card->hw)->PcmPlaybackAreaId);

	/* Clean up */
	free(card->hw);
	card->hw = NULL;

	return B_OK;
}


static void cs4280_StartPcm(sound_card_t *card)
{
	cpu_status cp;

	b0count=0;
	b1count=0;

	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	cs4280_PcmStartPlayback(card);
	cs4280_PcmStartCapture(card);
	//	request "another" interrupt
	WRITE_REG(((cs4280_hw*)card->hw)->BA0, BA0_HICR, 0x3, -1);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void cs4280_StopPcm(sound_card_t *card)
{
	cpu_status cp;
	DB_PRINTF(("cs4280_StopPcm\n"));


	cp = disable_interrupts();
	acquire_spinlock(&card->bus.pci.lock);

	/*terminate playback*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PFIE, 0x10, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_PLAY, 0, 0xffff0000);
	/*terminate capture*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_CAPTURE_INTERRUPT_ENABLE_REG, 0x11, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_CAPTURE, 0, 0x0000ffff);
	/* turn off interrupts */
	WRITE_REG(((cs4280_hw*)card->hw)->BA0, BA0_HICR, 0x2, -1);

	/*terminate playback*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_PFIE, 0x10, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_PLAY, 0, 0xffff0000);
	/*terminate capture*/
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_CAPTURE_INTERRUPT_ENABLE_REG, 0x11, 0x3F);
   	WRITE_REG(((cs4280_hw*)(card->hw))->BA1, BA1_STARTSTOP_CAPTURE, 0, 0x0000ffff);

	release_spinlock(&card->bus.pci.lock);
	restore_interrupts(cp);
}


static void cs4280_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	int32 play_phase_increment;
	int64 px;
	int32 py;
	int16 play_sample_rate_correction;

	ddprintf("cs4280: sample_rate is %d\n", sample_rate);

	play_phase_increment = floor( (int64)sample_rate * 0x4000000LL / 48000LL);
	px = (((int64)sample_rate * 65536LL * 1024LL - (int64)play_phase_increment * 48000LL) + 0.5);
	py = floor( px/200);
	play_sample_rate_correction = px - 200 * py;
	
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_PLAY_SAMPLE_RATE_CORRECTION_REG,
				(((int32)(play_sample_rate_correction) << 16) & 0xFFFF0000) |
				(py & 0x0000FFFF), -1);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_PLAY_PHASE_INCREMENT_REG,
				play_phase_increment, -1);

        /* "Make SRC the child of the master mixer." */
		/* -- I have no idea what that means. -- hplus & solson in unison */
    WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_MASTER_MIX_CHILD_REG, (PROC_SRC_SCB <<16) & 0xFFFF0000, -1);

	((cs4280_hw*)card->hw)->SRHz = sample_rate;
}

static status_t cs4280_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 size, void** log_addr)
{

	DB_PRINTF(("In cs4280_SetPlaybackDmaBuf\n"));

	// Must be 4096 !!!
	if (size != 4096 ) {
		dprintf("cs4280: Playback buffer must be exactly 4096 bytes! request was %ld\n",size);
		return B_ERROR;
	}
	
	*log_addr = ((cs4280_hw*)card->hw)->PcmPlaybackBuf;
	
	DB_PRINTF(("cs4280_SetPlaybackDmaBuf OK\n"));
	return B_OK;
}
static status_t cs4280_SetPlaybackDmaBuf(sound_card_t *card,  uint32 *psize, void** log_addr)
{
	*psize = 4096;
	return cs4280_SetPlaybackDmaBufOld(card, *psize, log_addr);
}

static void cs4280_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uint32 format_val = 0;
	uint32 count_val = 0xf;

	DB_PRINTF(("cs4280_SetPlaybackFormat\n"));

	if (num_of_bits == 8) {
		format_val |= PFIE_8BIT;
		count_val >>= 1;
	}
	if (num_of_ch == 1) {
		format_val |= PFIE_MONO;
		count_val >>= 1;
	}

	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_PFIE, format_val, PFIE_8BIT | PFIE_MONO);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_PLAY_DCW, count_val, 0x1ff);

	((cs4280_hw*)card->hw)->format = format_val;
}

static uint32 GCD(uint32 in1, uint32 in2)
{
	uint32 r;
	if (in2 > in1) {
		r = in2;
		in2 = in1;
		in1 = r;
	}
	r = 1;
	while (r > 0) {
		r = in1 % in2;
		in1 = in2;
		in2 = r;
	}
	return in1;		
}

static void cs4280_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	int32 capture_coeff_increment;
	int32 capture_phase_increment;
	int64 cx;
	int32 cy;
	int16 capture_sample_rate_correction;
	uint32 capture_delay;
	int32 capture_num_triplets;
	int16 capture_group_length;

	ddprintf("cs4280: capture sample_rate is %d\n", sample_rate);

	capture_coeff_increment = -(((int64)sample_rate * 128LL * 65536LL / 48000) + 0.5);
	capture_phase_increment = floor( 48000LL * 0x4000000LL / (int64)sample_rate);
	cx = ((48000LL * 65536LL * 1024LL - (int64)capture_phase_increment * (int64)sample_rate) + 0.5);
	cy = floor( cx/200);
	capture_sample_rate_correction = cx - 200 * cy;
	capture_delay = (24*48000/sample_rate);
	capture_num_triplets = floor( 65536LL * (int64)sample_rate / 24000LL);
	capture_group_length = 24000 / GCD(sample_rate, 24000);

	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_SAMPLE_RATE_CORRECTION_REG,
				((((int32)(capture_sample_rate_correction) << 16) & 0xFFFF0000) |
				(cy & 0x0000FFFF)) , -1);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_COEFFICIENT_INCREMENT_REG,
				capture_coeff_increment /* << 16 */, -1);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_DELAY_REG,
				((capture_delay<<18) | 0x80), -1);
    WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_PHASE_INCREMENT_REG,
				capture_phase_increment, -1);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_FRAME_GROUP_1_REG,
				capture_group_length, -1);
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_FRAME_GROUP_2_REG,
				(capture_group_length | 0x00800000), -1);
    WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_CONSTANT_REG, 0x0000FFFF, -1);

	WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_SPB_ADDRESS,
				capture_num_triplets, -1);

    WRITE_REG(((cs4280_hw*)card->hw)->BA1, BA1_CAPTURE_SPB_ADDRESS + 4 , 0x0000FFFF, -1);

{
    DWORD i;
    for(i=0; i<sizeof(InitArray)/sizeof(struct InitStruct); i++) {
           WRITE_REG(((cs4280_hw*)card->hw)->BA1, InitArray[i].off << 2, InitArray[i].val, -1);
	}
}

}

static status_t cs4280_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** log_addr)
{
	DB_PRINTF(("In cs4280_SetCaptureDmaBuf\n"));
	DB_PRINTF(("In cs4280_SetPlaybackDmaBuf\n"));

	// Must be 4096 !!!
	if (size != 4096 ) {
		ddprintf("cs4280: Capture buffer must be exactly 4096 bytes! req was %ld\n",size);
		return B_ERROR;
	}
	
	*log_addr = ((cs4280_hw*)card->hw)->PcmCaptureBuf;
	
	DB_PRINTF(("cs4280_SetCaptureDmaBuf OK\n"));
	return B_OK;
}
static status_t cs4280_SetCaptureDmaBuf(sound_card_t *card, uint32 *psize, void** log_addr)
{
	*psize = 4096;
	return cs4280_SetCaptureDmaBufOld(card, *psize, log_addr);
}

static void cs4280_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uint32 count_val = 0xf;

	DB_PRINTF(("cs4280_SetCaptureFormat\n"));

	if (num_of_bits == 8) {
		count_val >>= 1;
	}
	if (num_of_ch == 1) {
		count_val >>= 1;
	}

	// clear the done bit
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, 0x0100, 0, 0x00004000);
	// set transaction count
	WRITE_REG(((cs4280_hw*)card->hw)->BA1, 0x0100, count_val, 0x3ff);

}

//
//DO NOT USE THE SHADOW REGS!!! USE THE OFFSETS DIRECTLY!!!!!
//
static uint16 cs4280_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	uint16 ret = AC97_BAD_VALUE;
	uint32 count;
	volatile uchar * BA0 = ((cs4280_hw*)(card->hw))->BA0;
	uint32 ac97status;

	// Make sure that there is not data sitting
    // around from a previous uncompleted access.
    // ACSDA = Status Data Register = 47Ch
    ac97status = READ_REG(BA0, BA0_ACSDA);

    // Setup the AC97 control registers on the CS461x to send the
    // appropriate command to the AC97 to perform the read.
    // ACCAD = Command Address Register = 46Ch
    // ACCDA = Command Data Register = 470h
    // ACCTL = Control Register = 460h
    // set DCV - will clear when process completed
    // set CRW - Read command
    // set VFRM - valid frame enabled
    // set ESYN - ASYNC generation enabled
    // set RSTN - ARST# inactive, AC97 codec not reset

    //
    // Get the actual AC97 register from the offset
    //
    // (shadow regs!) WRITE_REG(BA0, BA0_ACCAD, ulOffset - BA0_AC97_RESET, -1);
    WRITE_REG(BA0, BA0_ACCAD, offset, -1);
    WRITE_REG(BA0, BA0_ACCDA, 0, -1);
    WRITE_REG(BA0, BA0_ACCTL,  ACCTL_DCV | ACCTL_CRW | ACCTL_VFRM |
                            ACCTL_ESYN | ACCTL_RSTN, -1);
    //
    // Wait for the read to occur.
    //
    for (count = 0; count < 10; count++)
    {
        // First, we want to wait for a short time.
        snooze(1000);

        // Now, check to see if the read has completed.
        // ACCTL = 460h, DCV should be reset by now and 460h = 17h
        if( !(READ_REG(BA0, BA0_ACCTL) & ACCTL_DCV))
            break;
    }
    // Make sure the read completed.
    if (READ_REG(BA0, BA0_ACCTL) & ACCTL_DCV)
        return ret;

         // Wait for the valid status bit to go active.
    for (count = 0; count < 10; count++)
    {
         // Read the AC97 status register.
         // ACSTS = Status Register = 464h
         ac97status = READ_REG(BA0, BA0_ACSTS);

         // See if we have valid status.
         // VSTS - Valid Status
         if(ac97status & ACSTS_VSTS)
             break;

              // Wait for a short while.
         snooze(1000);
    }

    // Make sure we got valid status.
    if(!(ac97status & ACSTS_VSTS))
        return ret;

     // Read the data returned from the AC97 register.
     // ACSDA = Status Data Register = 474h
     return READ_REG(BA0, BA0_ACSDA);

}


static uint16 cs4280_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	lock_ac97(card);
		ret = cs4280_CodecReadUnsecure(card, offset);
	unlock_ac97(card);	
	return ret;
}

static status_t cs4280_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{

	uint32 count;
	sound_card_t* card = (sound_card_t*) host;
	volatile uchar * BA0 = ((cs4280_hw*)(card->hw))->BA0;
	uint32 ac97status;
	status_t ret = B_ERROR;

	if (mask == 0)
		return B_OK;
	
	lock_ac97(card);
		if (mask != 0xffff) {
			uint16 old  = cs4280_CodecReadUnsecure(card, offset);
			value = (value&mask)|(old&~mask);
		}

	    // Setup the AC97 control registers on the CS461x to send the
	    // appropriate command to the AC97 to perform the read.
	    // ACCAD = Command Address Register = 46Ch
	    // ACCDA = Command Data Register = 470h
	    // ACCTL = Control Register = 460h
	    // set DCV - will clear when process completed
	    // reset CRW - Write command
	    // set VFRM - valid frame enabled
	    // set ESYN - ASYNC generation enabled
	    // set RSTN - ARST# inactive, AC97 codec not reset
	
	    // Get the actual AC97 register from the offset
	    // NO shadow regs! WRITE_REG(BA0 ,BA0_ACCAD, ulOffset - BA0_AC97_RESET, -1);
	    WRITE_REG(BA0, BA0_ACCAD, offset, -1);
	    WRITE_REG(BA0, BA0_ACCDA, value, -1);
	    WRITE_REG(BA0, BA0_ACCTL, ACCTL_DCV | ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN, -1);

	    // Wait for the write to occur.
	    for(count = 0; count < 10; count++)
	    {
	        // First, we want to wait for a short time.
	        snooze(1000);
	
	        // Now, check to see if the write has completed.
	        // ACCTL = 460h, DCV should be reset by now and 460h = 07h
	        ac97status = READ_REG(BA0, BA0_ACCTL);
	        if(!(ac97status & ACCTL_DCV))
	            break;
	    }

	    // Make sure the write completed.
	    if(ac97status & ACCTL_DCV) {
	        ret = B_ERROR;
		}
		else {
	    	// Success.
	    	ret = B_OK;
		}
	unlock_ac97(card);
	return ret;
}


static status_t cs4280_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;

	DB_PRINTF(("In cs4280_InitCodec\n"));

	err = ac97init(&((cs4280_hw*)card->hw)->codec, 
					(void*)card,
					cs4280_CodecWrite,
					cs4280_CodecRead);

	if(err!=B_OK) {
		dprintf("cs4280_InitCodec(): ac97init err = %ld!\n",err);
	}	
	return err;				
}

static void cs4280_UninitCodec(sound_card_t *card)
{
	ddprintf("cs4280_UninitCodec()\n");
	ac97uninit(&((cs4280_hw*)card->hw)->codec); 
}

static status_t cs4280_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t err;
	err = AC97_SoundSetup(&((cs4280_hw*)card->hw)->codec, ss);
	if (err == B_OK) {
		cs4280_SetPlaybackSR(card, sr_from_constant(ss->sample_rate));
		switch (ss->playback_format) {
		case linear_8bit_unsigned_mono:
			cs4280_SetPlaybackFormat(card, 8, 1);
			break;
		case linear_8bit_unsigned_stereo:
			cs4280_SetPlaybackFormat(card, 8, 2);
			break;
		case linear_16bit_big_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_mono:	/* REALLY MEANS HOST ENDIAN! */
			cs4280_SetPlaybackFormat(card, 16, 1);
			break;
		case linear_16bit_big_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
		case linear_16bit_little_endian_stereo:	/* REALLY MEANS HOST ENDIAN! */
			cs4280_SetPlaybackFormat(card, 16, 2);
			break;
		default:
			derror("cs4280: unknown sample format %d\n", ss->playback_format);
			cs4280_SetPlaybackFormat(card, 16, 2);
		}
	}
	return err;
}

static status_t  cs4280_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ret = AC97_GetSoundSetup(&((cs4280_hw*)card->hw)->codec,ss);
	ss->sample_rate = constant_from_sr(((cs4280_hw*)card->hw)->SRHz);;
	switch (((cs4280_hw *)card->hw)->format) {
	default:
		derror("cs4280: unknown existing format 0x%x\n", ((cs4280_hw *)card->hw)->format);
	case 0:
		ss->playback_format = linear_16bit_big_endian_stereo;
		break;
	case PFIE_8BIT:
		ss->playback_format = linear_8bit_unsigned_stereo;
		break;
	case PFIE_MONO:
		ss->playback_format = linear_16bit_big_endian_mono;
		break;
	case PFIE_8BIT|PFIE_MONO:
		ss->playback_format = linear_8bit_unsigned_mono;
		break;
	}
	ss->capture_format = ss->playback_format;
	ddprintf("GetSoundSetup: ss->sample_rate = %d\n",ss->sample_rate);
	return ret;
}









