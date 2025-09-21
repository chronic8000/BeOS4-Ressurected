/************************************************************************/
/*                                                                      */
/*                              ymf724.c                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

#include <KernelExport.h>
#include <ISA.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>

#include "audio_drv.h"

#include "yamaha.h"
#include "hwmcode.h"
#include "hwio.h"
#include "misc.h"

#include "debug.h"
#include <ac97_module.h>
#include "audio_drv.h"

typedef struct _ymf724_playback_control_t {
	area_id	 areaId;
	vuint32* pNumOfSlots;
	vuint32* Bank[4];
} ymf724_playback_control_t;

typedef struct _ymf724_capture_control_t {
	area_id	 areaId;
	vuint32* pCtrlData;
	vuint32* RecBank[2];
	vuint32* ADCBank[2];
} ymf724_capture_control_t;

typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;

typedef struct _ymf724_hw {
	timers_data_t timers_data;	
	mem_mapped_regs_t native_regs;
	ac97_t codec;
	ymf724_playback_control_t PlaybackCtrl;
	ymf724_capture_control_t CaptureCtrl;
	area_id PcmCaptureAreaId; 
	area_id PcmPlaybackAreaId; 
	size_t PcmPlaybackBufSize;
	int PcmCaptureHalfNo;//  ((ymf724_hw*)card->hw)->PcmCaptureHalfNo  ((ymf724_hw*)card->hw)->PcmCaptureHalfNo
	int PcmPlaybackHalfNo;// ((ymf724_hw*)card->hw)->PcmPlaybackHalfNo  ((ymf724_hw*)card->hw)->PcmPlaybackHalfNo
} ymf724_hw; 




static status_t 	ymf724_Init(sound_card_t * card);
static status_t 	ymf724_Uninit(sound_card_t * card);
static void 		ymf724_SetPlaybackSR(sound_card_t *card, uint32 sample_rate);
static status_t 	ymf724_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 size, void** addr);
static status_t 	ymf724_SetPlaybackDmaBuf(sound_card_t *card,  uint32* pSize, void** addr);
static void 		ymf724_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		ymf724_SetCaptureSR(sound_card_t *card, uint32 sample_rate);
static status_t		ymf724_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** addr);
static status_t		ymf724_SetCaptureDmaBuf(sound_card_t *card, uint32* pSize, void** addr);
static void 		ymf724_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch);
static void 		ymf724_StartPcm(sound_card_t *card);
static void 		ymf724_StopPcm(sound_card_t *card);
static status_t		ymf724_SoundSetup(sound_card_t *card, sound_setup *ss);
static status_t		ymf724_GetSoundSetup(sound_card_t *card, sound_setup *ss);
static void 		ymf724_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);

static status_t		ymf724_InitJoystick(sound_card_t * card);
static void 		ymf724_enable_gameport(sound_card_t * card);
static void 		ymf724_disable_gameport(sound_card_t * card);

static status_t 	ymf724_InitMPU401(sound_card_t * card);
static void 		ymf724_enable_mpu401(sound_card_t *card);
static void 		ymf724_disable_mpu401(sound_card_t *card);
static void 		ymf724_enable_mpu401_interrupts(sound_card_t *card);
static void 		ymf724_disable_mpu401_interrupts(sound_card_t *card);

sound_card_functions_t ymf724_functions = 
{
/*status_t	(*Init) (sound_card_t * card);*/
&ymf724_Init,
/*	status_t	(*Uninit) (sound_card_t *card);*/
&ymf724_Uninit,
/*	void		(*StartPcm) (sound_card_t *card);*/
&ymf724_StartPcm,
/*	void		(*StopPcm) (sound_card_t *card);*/
&ymf724_StopPcm,
/*	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);*/
&ymf724_GetClocks,
/*	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);*/
&ymf724_SoundSetup,
/*	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);*/
&ymf724_GetSoundSetup,
/*	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);*/
&ymf724_SetPlaybackSR,
/*	void		(*SetPlaybackDmaBufOld) (sound_card_t *card, void* phys_addr, uint32 size);*/
&ymf724_SetPlaybackDmaBufOld,
/*	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&ymf724_SetPlaybackFormat,
/*	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);*/
&ymf724_SetCaptureSR,
/*	void		(*SetCaptureDmaBufOld) (sound_card_t *card, void* phys_addr, uint32 size);*/
&ymf724_SetCaptureDmaBufOld,
/*	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);*/
&ymf724_SetCaptureFormat,
/*	status_t	(*InitJoystick)(sound_card_t *);*/
&ymf724_InitJoystick,
/*	void	 	(*enable_gameport) (sound_card_t *);*/
&ymf724_enable_gameport,
/*	void 		(*disable_gameport) (sound_card_t *);*/
&ymf724_disable_gameport,
/*	status_t	(*InitMidi)(sound_card_t *);*/
&ymf724_InitMPU401,
/*	void	 	(*enable_midi) (sound_card_t *);*/
&ymf724_enable_mpu401,
/*	void 		(*disable_midi) (sound_card_t *);*/
&ymf724_disable_mpu401,
/*	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);*/
&ymf724_enable_mpu401_interrupts,
/*	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);*/
&ymf724_disable_mpu401_interrupts,

NULL,// nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
NULL, // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports

/*	void		(*SetPlaybackDmaBuf) (sound_card_t *card, void* phys_addr, uint32* size);*/
&ymf724_SetPlaybackDmaBuf,
/*	void		(*SetCaptureDmaBuf) (sound_card_t *card, void* phys_addr, uint32* size);*/
&ymf724_SetCaptureDmaBuf,

} ;

card_descrtiptor_t 
ymf724_descriptor = {
	"ymf724",
	PCI, 
	&ymf724_functions,
	YAMAHA_VENDOR_ID, 
	YMF724_DEVICE_ID
};

 
const char* ymf724_joystick_base_disable_param[4] = {
	"ymf724_disable_0x201",
	"ymf724_disable_0x202",
	"ymf724_disable_0x204",
	"ymf724_disable_0x205"
	};
const char* ymf724_mpu_base_disable_param[4] = {
	"ymf724_disable_0x330",
	"ymf724_disable_0x300",
	"ymf724_disable_0x332",
	"ymf724_disable_0x334"
	};

uint ymf724_joystick_base[4] = {0x201,0x202,0x204,0x205};
uint ymf724_mpu_base[4] = {0x330,0x300,0x332,0x334};

int ymf724_allowed_joystick_cnf[4] = {-1, -1, -1, -1};
int ymf724_allowed_mpu_cnf[4] = {-1, -1, -1, -1};

static void 		ymf724_LoadFirmware(sound_card_t *card);

static status_t 	ymf724_InitCodec(sound_card_t *card);
static void 		ymf724_UninitCodec(sound_card_t *card);
static uint16 		ymf724_CodecRead(void* host, uchar offset);
static status_t 	ymf724_CodecWrite(void* host, uchar offset, uint16 value , uint16 mask);

static void* 		ymf724_PreparePlaybackControl(ymf724_playback_control_t* ctrl, uint32 play_bank_size);
static void* 		ymf724_PrepareCaptureControl(ymf724_capture_control_t* ctrl, uint32 cap_bank_size);

static uint32		calc_PgDelta(uint32);
static uint32		calc_LpfK(uint32);
static uint32		calc_LpfQ(uint32);



void ymf724_GetDriverSettings(void* hSettings)
{
	int i,j;
	ddprintf1("Geting joystick settings for ymf724\n");
	for (i = 0, j = 0; i < 4; i++)
	{
		if(get_driver_boolean_parameter(hSettings,ymf724_joystick_base_disable_param[i],false,true)) continue;
		ymf724_allowed_joystick_cnf[j] = i;
		ddprintf1("ymf724_allowed_joystick_cnf[j] = %d (j = %d)\n",ymf724_allowed_joystick_cnf[j],j);
		j++;			
	}
	ddprintf1("Geting mpu settings for ymf724\n");
	for (i = 0, j = 0; i < 4; i++)
	{
		if(get_driver_boolean_parameter(hSettings,ymf724_mpu_base_disable_param[i],false,true)) continue;
		ymf724_allowed_mpu_cnf[j] = i;
		ddprintf1("ymf724_allowed_mpu_cnf[j] = %d (j = %d)\n",ymf724_allowed_mpu_cnf[j],j);
		j++;			
	}
}



/* --- 
	Interrupt service routine
	---- */
static int32 ymf724_isr(void *data)
{
	status_t ret = B_UNHANDLED_INTERRUPT;
	bigtime_t time = system_time();
	sound_card_t *card = (sound_card_t *) data;
	if ((mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_STATUS) & 0x80000000) != 0) { 
		/* That's us! Acknowledge it */
		mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_STATUS, 0x80000000, 0xffffffff); 
	
		/* Reset hardware interrupts */
		mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, (0x1<<1), (0x1<<1)); 

		ret = B_HANDLED_INTERRUPT;

		{//Playback section
			bool is_anoter_half;
			int CurrentSampleNo;
			int32 bank_no,SamplesPerHalfBuf;
			bank_no = mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_CSEL);
			CurrentSampleNo = ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[bank_no][PgStart];
			SamplesPerHalfBuf = ((ymf724_hw*)card->hw)->PcmPlaybackBufSize/2/2/2;
			is_anoter_half = (((ymf724_hw*)card->hw)->PcmPlaybackHalfNo == 1 && (CurrentSampleNo >= SamplesPerHalfBuf))
				|| (((ymf724_hw*)card->hw)->PcmPlaybackHalfNo == 0 && (CurrentSampleNo < SamplesPerHalfBuf)); 

			if(is_anoter_half)
			{
				((ymf724_hw*)card->hw)->PcmPlaybackHalfNo++;
				((ymf724_hw*)card->hw)->PcmPlaybackHalfNo %=2;

				if ((*card->pcm_playback_interrupt)(card,((ymf724_hw*)card->hw)->PcmPlaybackHalfNo)) ret = B_INVOKE_SCHEDULER;

				CurrentSampleNo %= SamplesPerHalfBuf;
				time-=(CurrentSampleNo*(1000.0/44.1));
				acquire_spinlock(&((ymf724_hw*)card->hw)->timers_data.Lock);	
				((ymf724_hw*)card->hw)->timers_data.SystemClock = time;
				((ymf724_hw*)card->hw)->timers_data.IntCount++;
				release_spinlock(&((ymf724_hw*)card->hw)->timers_data.Lock);
			}
		}//end of Playback section

		{//Capture section
			bool is_anoter_half;
			int32 bank_no, PgStartVal, HalfSize;
		
			bank_no = mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_CSEL);
			PgStartVal = ((ymf724_hw*)card->hw)->CaptureCtrl.ADCBank[bank_no][CapPgStart];
			HalfSize = ((ymf724_hw*)card->hw)->CaptureCtrl.ADCBank[bank_no][CapPgLoopEnd] / 2;
			is_anoter_half = (((ymf724_hw*)card->hw)->PcmCaptureHalfNo && (PgStartVal >= HalfSize))
				|| (!((ymf724_hw*)card->hw)->PcmCaptureHalfNo && (PgStartVal < HalfSize)); 

			if(is_anoter_half)
			{
				((ymf724_hw*)card->hw)->PcmCaptureHalfNo++;
				((ymf724_hw*)card->hw)->PcmCaptureHalfNo %= 2;
				if ((*card->pcm_capture_interrupt)(card,((ymf724_hw*)card->hw)->PcmCaptureHalfNo)) ret = B_INVOKE_SCHEDULER;
			}
		}//endof Capture section 
		goto exit;
	}

	/* Not PCM interrupt. MIDI, maybe? */
	if ((*card->midi_interrupt)(card)) 
		ret = B_INVOKE_SCHEDULER;

exit:
	return ret;
}


static void ymf724_GetClocks(sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock)
 {
 	cpu_status cp;
 	uint64 IntCount;
	uint64 PlayedSamples;	

	cp = disable_interrupts();
 	acquire_spinlock(&((ymf724_hw*)card->hw)->timers_data.Lock);	
	*pSystemClock = ((ymf724_hw*)card->hw)->timers_data.SystemClock;
	IntCount = ((ymf724_hw*)card->hw)->timers_data.IntCount;
	release_spinlock(&((ymf724_hw*)card->hw)->timers_data.Lock);	
	restore_interrupts(cp);

	PlayedSamples =IntCount*((ymf724_hw*)card->hw)->PcmPlaybackBufSize/2/2/2;
 	*pSampleClock = (bigtime_t)( PlayedSamples*(1000.0/44.1));
 }


static void ymf724_enable_mpu401(sound_card_t *card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, (0x1<<3), (0x1<<3));
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, (0x1<<15));  // enable legacy audio
}

static void ymf724_disable_mpu401(sound_card_t *card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, (0x1<<3));
}

static void ymf724_enable_mpu401_interrupts(sound_card_t *card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, (0x1<<4), (0x1<<4));
}

static void ymf724_disable_mpu401_interrupts(sound_card_t *card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, (0x1<<4));
}


static void ymf724_enable_gameport(sound_card_t * card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, (0x1<<2), (0x1<<2));
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, (0x1<<15)); // enable legacy audio

}

static void ymf724_disable_gameport(sound_card_t * card)
{
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, (0x1<<2));
}

static status_t ymf724_InitJoystick(
	sound_card_t * card)
{
	if(ymf724_allowed_joystick_cnf[card->idx] < 0) return ENODEV;
	pci_config_write(card, EXT_LEGACY_CONTROL, 2, (ymf724_allowed_joystick_cnf[card->idx] << 6), (0x3<<6));
	/* Create Joystick device */
	card->joy_base = ymf724_joystick_base[ymf724_allowed_joystick_cnf[card->idx]];
	ddprintf("*** ymf724_InitJoystick(): joy_base = %x\n", card->joy_base);
	return B_OK;
}

static status_t ymf724_InitMPU401(
	sound_card_t * card)
{
	if(ymf724_allowed_mpu_cnf[card->idx] < 0) return ENODEV;
	pci_config_write(card, EXT_LEGACY_CONTROL, 2, (ymf724_allowed_mpu_cnf[card->idx] << 4), (0x3<<4));
	/* Create MPU device */
	card->mpu401_base = ymf724_mpu_base[ymf724_allowed_mpu_cnf[card->idx]];
	ddprintf("*** ymf724_InitMPU401(): mpu401_base = %x\n", card->mpu401_base);
	return B_OK;
}


static status_t ymf724_Init(sound_card_t * card)
{
	status_t err = ENODEV;

	ddprintf("ymf724_Init(): here! \n");
	card->hw = malloc(sizeof(ymf724_hw));
	if(!card->hw) return ENOMEM;
	
	/* Disable all legacy devices */
	pci_config_write(card, LEGACY_CONTROL, 2, 0x0, 0x001f);

	/* Enable PCI INTA# mode for legacy devices */
	pci_config_write(card, EXT_LEGACY_CONTROL, 2, (0x1<<15), (0x1<<15));

	/* Initialize addresses of the hw functions in the card structure*/


	((ymf724_hw*)card->hw)->timers_data.IntCount = 0;
	((ymf724_hw*)card->hw)->timers_data.SystemClock = 0;
	((ymf724_hw*)card->hw)->timers_data.Lock = 0;	
	((ymf724_hw*)card->hw)->PcmCaptureAreaId = -1; 
	((ymf724_hw*)card->hw)->PcmPlaybackAreaId = -1; 


	/* Enable Bus Mastering, Memory Space and I/O Space responses  */
	pci_config_write(card, PCI_COMMAND, 2, 0x7, 0x7);

	err = init_mem_mapped_regs(	&((ymf724_hw*)card->hw)->native_regs,
								"native rergs", 
								(void *)(card->bus.pci.info.u.h0.base_registers[0]),
								card->bus.pci.info.u.h0.base_register_sizes[0]); 
	if (err != B_OK) 
		return err;


	/* Mute DAC OUT */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_DAC_OUT_L, 0x0, 0xffffffff);	

	/* Reset controller */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x10000, 0xffffffff); 
	spin(100);
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x0, 0xffffffff);
	spin(100);


	/* Install isr */
	ddprintf("*** ymf724_Init(): INT line = %d\n", card->bus.pci.info.u.h0.interrupt_line);
	install_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, ymf724_isr, card, 0);


	/* Initialize native audio control regs */  
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MAP_REC, 0x0, 0xffffffff);
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MAP_EFF, 0x0, 0xffffffff);
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_PLAY_CBASE, 0x0, 0xffffffff);
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_REC_CBASE, 0x0, 0xffffffff);
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_EFF_CBASE, 0x0, 0xffffffff);

	/* Prepare for DSP test */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_CSEL, 0x1, 0xffffffff);
	if(mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_CSEL)!=0x1) {
		derror("ymf724_Init: unsuccessful mapped_write_32 to DS_CSEL register.\n");
		return B_ERROR;
	}

	/* Load DSP and Controller instructions */
	ymf724_LoadFirmware(card);

	/* Start DSP */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_CFG, 0x1, 0xffffffff);
	
	/* Test DSP*/
	{
		int i;
		int wait_tick = 10;
		for(i=500000/wait_tick; i>0 ; i--) /* loop 0.5 sec. */
		{
			if(mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_CSEL) == 0x0) break; 
			snooze(wait_tick);
		}
		if(i<=0)
		{
			derror("ymf724_Init: DSP test failed.\n");
			return B_ERROR;
		}
	}

	/* Init Codec */
	if ((err = ymf724_InitCodec(card))) {
	 return err;
	}


	{	/* Allocate and prepare playback control memory */
		void* phys_addr; 
		uint32 play_bank_size = mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_PLAY_CSIZE);		
		dprintf("Prepare playback control memory play_bank_size = %lx\n",play_bank_size);
		phys_addr = ymf724_PreparePlaybackControl(&((ymf724_hw*)card->hw)->PlaybackCtrl, play_bank_size);
		if(phys_addr == NULL) return ((ymf724_hw*)card->hw)->PlaybackCtrl.areaId;
		mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_PLAY_CBASE, (uint32)(phys_addr), 0xffffffff);
	}

	{	/* Allocate and prepare capture control memory */
		void* phys_addr; 
		uint32 cap_bank_size = mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_REC_CSIZE);		
		phys_addr = ymf724_PrepareCaptureControl(&((ymf724_hw*)card->hw)->CaptureCtrl, cap_bank_size);
		if(phys_addr == NULL) return ((ymf724_hw*)card->hw)->CaptureCtrl.areaId;
		mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_REC_CBASE, (uint32)(phys_addr), 0xffffffff);
		ddprintf("ymf724_Init(): DS_REC_CBASE = %lX\n", 
			mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_REC_CBASE));

		mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MAP_REC, 
			0x02, 0xffffffff); // use ADC slot for capture
		ddprintf("ymf724_Init(): DS_MAP_REC = %lX\n", 
			mapped_read_32(&((ymf724_hw*)card->hw)->native_regs, DS_MAP_REC));
	}

	return B_OK;
}



static status_t 
ymf724_Uninit(
	sound_card_t * card)
{
	remove_io_interrupt_handler(card->bus.pci.info.u.h0.interrupt_line, ymf724_isr, card);


	ymf724_UninitCodec(card);
	/* Stop DSP */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_CFG, 0x0, 0xffffffff);

	/* Reset controller */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x10000, 0xffffffff); 
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x0, 0xffffffff);

	uninit_mem_mapped_regs(&((ymf724_hw*)card->hw)->native_regs);
	delete_area(((ymf724_hw*)card->hw)->PlaybackCtrl.areaId);
	delete_area(((ymf724_hw*)card->hw)->CaptureCtrl.areaId);
	
	/* Disable Bus Mastering, Memory Space and I/O Space responses  */
	pci_config_write(card, PCI_COMMAND, 2, 0x0, 0x7);

	/* Disable legacy audio */
	pci_config_write(card, LEGACY_CONTROL, 2, (0x1<<15), (0x1<<15));

	if(((ymf724_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((ymf724_hw*)card->hw)->PcmPlaybackAreaId);
	if(((ymf724_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((ymf724_hw*)card->hw)->PcmCaptureAreaId);

	free(card->hw);
/*
	dprintf("Max interrupt time = %ld\n", (uint32)max_delta);
*/	
	
	return B_OK;
}

static void ymf724_StartPcm(sound_card_t *card)
{
	snooze(1000000);
	((ymf724_hw*)card->hw)->PcmPlaybackHalfNo = 1;
	((ymf724_hw*)card->hw)->PcmCaptureHalfNo = 1;
	((ymf724_hw*)card->hw)->timers_data.IntCount = 0;
	((ymf724_hw*)card->hw)->timers_data.SystemClock = system_time();

	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x03, 0xffffffff);
	snooze(30);

	/* Unmute DAC OUT */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_DAC_OUT_L, 0xffffffff, 0xffffffff);	/* unmute native DAC out */
	ddprintf("End of ymf724_SetPlaybackDmaBuf\n");
	/* Unmute ADC INP */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_INP_L, 0xffffffff, 0xffffffff);
}

static void ymf724_StopPcm(sound_card_t *card)
{
	/* Mute DAC OUT */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_DAC_OUT_L, 0x0, 0xffffffff);	
	/* Mute ADC INP */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_INP_L, 0x0, 0xffffffff);
	/* Stop */
	mapped_write_32(&((ymf724_hw*)card->hw)->native_regs, DS_MODE, 0x00, 0x1);
	snooze(30);
}


static void ymf724_SetPlaybackSR(sound_card_t *card, uint32 sample_rate)
{
	int i;
	
	for(i=0; i<4; i++){
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgDelta] = ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgDeltaEnd] = calc_PgDelta(sample_rate);
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LpfK] = ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LpfKEnd] = calc_LpfK(sample_rate);
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LpfQ] = calc_LpfQ(sample_rate);
	}

}

static status_t ymf724_SetPlaybackDmaBufOld(sound_card_t *card,  uint32 Size, void** addr)
{
	uint32 s = Size;
	if(s < 4096) return B_ERROR;// 2K half buffer is lower limit caused by ymf724 interrupt accurency(256 48Kh clocks)
	return ymf724_SetPlaybackDmaBuf(card,  &s, addr);
	
}


static status_t ymf724_SetPlaybackDmaBuf(sound_card_t *card,  uint32* pSize, void** addr)
{
	int i;
	void* phys_addr;
	area_id AreaId;
	uint32 size = *pSize;
	size_t tmp_size;
	
	if(size < 4096) size = 4096; // 2K half buffer is lower limit caused by ymf724 interrupt accurency(256 48Kh clocks)
	tmp_size = size + 0x400;
	ddprintf("ymf724_SetPlaybackDmaBuf:  size = %ld\n", size);

	AreaId = allocate_memory("ymf724_PlaybackDmaBuf",
				addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for ymf724_PlaybackDmaBuf failed!!\n");
		ddprintf("ymf724_SetPlaybackDmaBuf: return %ld;\n",AreaId);
		return AreaId;
	}
	if(((ymf724_hw*)card->hw)->PcmPlaybackAreaId >= 0) delete_area(((ymf724_hw*)card->hw)->PcmPlaybackAreaId);
	((ymf724_hw*)card->hw)->PcmPlaybackAreaId = AreaId;
	((ymf724_hw*)card->hw)->PcmPlaybackBufSize = size;
	 
	memset(*addr,0,tmp_size);
	*addr += 0x400;
	phys_addr += 0x400;

	for(i=0; i<4; i++)
	{
		int num_of_ch = (( ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] & (0x1<<16) ) >> 16) + 1;
		int num_of_bits = 16 - 8 * ( (((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] & (0x1<<31)) >> 31 );
		ddprintf("ymf724_SetPlaybackDmaBuf: Bank[%d]: num_of_ch = %d, num_of_bits = %d\n", i, num_of_ch, num_of_bits);
		ddprintf("Was: PgBase = %lX, PgLoopEnd = %lX\n",
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgBase],
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgLoopEnd]
		);
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgBase] = (uint32)(phys_addr);
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgLoopEnd] = (uint32)(size / (num_of_ch * num_of_bits / 8));
		ddprintf("New: PgBase = %lX, PgLoopEnd = %lX\n",
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgBase],
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgLoopEnd]
		);
	}
	*pSize = size;// return actual value 
	return B_OK;
}

static void ymf724_SetPlaybackFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	int i;
	uint32	format = 0x0;
	if(num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format
	if (num_of_bits == 8) format |= (0x1<<31);
	if (num_of_ch == 2) format |= (0x1<<16);
	ddprintf("Set num of slots\n");

	*(((ymf724_hw*)card->hw)->PlaybackCtrl.pNumOfSlots) = num_of_ch;

	for(i=0; i<4; i++)
	{
		/* Recalculate control data depending on 
		the pcm format and the dma buffer size */
		int old_num_of_ch = (( ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] & (0x1<<16) ) >> 16) + 1;
		int old_num_of_bits = 16 - 8 * ( (((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] & (0x1<<31)) >> 31 );
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgLoopEnd] *= (old_num_of_ch*(old_num_of_bits/8));
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][PgLoopEnd] /= (num_of_ch * (num_of_bits/8));
	
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][EgGain] = 0x40000000;
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][EgGainEnd] = 0x40000000;
	}
	for(i=0; i<2; i++) {
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] = format;
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LchGain]  = 0x40000000;
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LchGainEnd] = 0x40000000;
		if (num_of_ch == 1) {//mono	
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[0][RchGain] = 0x40000000;
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[0][RchGainEnd] = 0x40000000;
		}
		else {//stereo, right channel controlled by the another slot
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[0][RchGain] = 0x0;
			((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[0][RchGainEnd] = 0x0;
		}
	}
	for(i=2; i<4; i++) {//this slot (bank 3 and 4) is used only in the stereo mode 
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][Format] = format | (0x1<<16) | 0x1; // allways sets stereo and right ch
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LchGain] = ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][LchGainEnd] = 0x0;
		((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][RchGain] = ((ymf724_hw*)card->hw)->PlaybackCtrl.Bank[i][RchGainEnd] = 0x40000000;
	}
}


static void* ymf724_PreparePlaybackControl(ymf724_playback_control_t* ctrl, uint32 play_bank_size)
{
	int i;
	size_t buf_size;
	uint32 *Addr;
	void *PhysAddr;
	uint32 play_table_size = 3;	//in dwords: num_of_slots + 2 pointers reserved for slots (stereo)	

	ddprintf("ymf724_PreparePlaybackControl:play_bank_size = %ld\n",play_bank_size);

	buf_size =	(sizeof(uint32))*(
		play_table_size + 
		play_bank_size*2*2 //2 slots each consist of 2 banks
			);

	dprintf("ymf724_PreparePlaybackControl: play_bank_size = %ld,buf_size = %ld\n", play_bank_size,buf_size);

	ctrl->areaId = allocate_memory("PlaybackControlArea",(void**)&Addr, &PhysAddr, &buf_size);
	if (ctrl->areaId < 0)
	{
		derror("ymf724_Init(): error allocaing memory!\n");
		return NULL;
	}
	memset(Addr,0,buf_size);
	ctrl->pNumOfSlots = Addr;
	Addr[0] = 2; // 2 slots for stereo
	Addr[1] = (uint32)(PhysAddr + play_table_size*sizeof(uint32));
	Addr[2] = Addr[1]+play_bank_size*2*sizeof(uint32);

	for(i=0;i<4;i++) ctrl->Bank[i] =&Addr[play_table_size+i*play_bank_size]; 
	return PhysAddr;
}



static void ymf724_LoadFirmware(sound_card_t *card)
{
	vuint32 *dst;
	uint32 size;
	uint32 idx;
	
	ddprintf("ymf724_LoadFirmware(): here!\n");


	/* Load DSP instructions (hwmcode.h) */
	dst = (vuint32 *)(((ymf724_hw*)card->hw)->native_regs.base + DS_DSP_BASE);
 	size = sizeof(CntrlInst) / sizeof(uint32);
	for (idx = 0; idx < size; idx++) 
		dst[idx] = DspInst[idx];
	
	/* Load Controller instructions (hwmcode.h) */	
	dst = (vuint32 *)(((ymf724_hw*)card->hw)->native_regs.base + DS_CONT_BASE);
 	size = sizeof(CntrlInst) / sizeof(uint32);
	for (idx = 0; idx < size; idx++) 
		dst[idx] = CntrlInst[idx];
}


/* ----------
	Functions used to calculate some of the playback control data
------- */

//    Calculate PgDeltaEnd & PgDelta
static uint32 calc_PgDelta(uint32 sr)
{
	uint64 tmp = (uint32)sr;

	tmp = (tmp<<28) / 48000;
	return (uint32)tmp & 0x7fffff00;
}

//    Calculate LpfK
static uint32 calc_LpfK(uint32 sr)
{
	uint8	i;
	uint32	val = 0x40000000;
	uint16	freq[8] = {100,    2000,   8000,   11025,  16000,  22050,  32000,  48000};
	uint16	lpfk[8] = {0x0057, 0x06aa, 0x18B2, 0x2093, 0x2b9a, 0x35a1, 0x3eaa, 0x4000};


	if (sr == 44100) {
		val = (uint32)0x4646 << 16;
	}
	else {
		for (i = 0; i < 8; i++) {
			if (sr <= freq[i] ) {
				val = (uint32)(lpfk[i]) << 16;
				break;
			}
		}
	}

	return val;
 }

//    Calculate LpfQ
static uint32 calc_LpfQ(uint32 sr)
{
	uint8	i;
	uint32	val = 0x40000000;
	uint16	freq[8] = {100,    2000,   8000,   11025,  16000,  22050,  32000,  48000};
	uint16  lpfq[8] = {0x3528, 0x34a7, 0x3202, 0x3177, 0x3139, 0x31c9, 0x33d0, 0x4000};


	if (sr == 44100) {
		val = (uint32)0x370A << 16;
	}
	else {
		for (i = 0; i < 8; i++) {
			if (sr <= freq[i]) {
				val = (uint32)(lpfq[i]) << 16;
				break;
			}
		}
	}

	return val;
 }



static uint16 ymf724_CodecReadUnsecure(sound_card_t *card, uchar offset)
{
	int count;
	uint16 ret = AC97_BAD_VALUE;
	

	offset &= 0x7f;
	mapped_write_16(&((ymf724_hw*)card->hw)->native_regs, AC97_CMD_ADDRESS, 0x8000 | offset, 0xffff);
	/* Poll until not BUSY */
	for (count = 0; count < 400; count++) {
		if (!(mapped_read_16(&((ymf724_hw*)card->hw)->native_regs, AC97_STATUS_ADDRESS) & 0x8000)) 
			{
			ret = mapped_read_16(&((ymf724_hw*)card->hw)->native_regs, AC97_STATUS_DATA);
			break;
			}
		spin(100);
	}
	return ret;
}


static uint16 ymf724_CodecRead(void* host, uchar offset)
{
	sound_card_t *card = (sound_card_t *)host;
	uint16 ret = AC97_BAD_VALUE;
	
	cpu_status cp = disable_interrupts();
	acquire_spinlock(&(((ymf724_hw*)card->hw)->codec.lock));

	ret = ymf724_CodecReadUnsecure(card, offset);
	
	release_spinlock(&(((ymf724_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);

	return ret;
}

static status_t ymf724_CodecWrite(void* host, uchar offset, uint16 value, uint16 mask )
{
	sound_card_t *card = (sound_card_t *)host;
	int count;
	status_t ret = B_ERROR;
	cpu_status cp;
	
	if (mask == 0) return B_OK;
	
	ddprintf("ymf724_CodecWrite()here! codec.lock = %lx\n",((ymf724_hw*)card->hw)->codec.lock);	
	cp = disable_interrupts();
	acquire_spinlock(&(((ymf724_hw*)card->hw)->codec.lock));

	if (mask != 0xffff) {
		uint16 old  = ymf724_CodecReadUnsecure(card, offset);
		value = (value&mask)|(old&~mask);
	}

	offset &= 0x7f;
	mapped_write_16(&((ymf724_hw*)card->hw)->native_regs, AC97_CMD_ADDRESS, offset, 0xffff);
	mapped_write_16(&((ymf724_hw*)card->hw)->native_regs, AC97_CMD_DATA, value, 0xffff);
	/* Poll until not BUSY */
	for (count = 0; count < 400; count++) {
		if (!(mapped_read_16(&((ymf724_hw*)card->hw)->native_regs, AC97_STATUS_ADDRESS) & 0x8000)) 
		{	
			ret = B_OK;
			break;
		}
		spin(100);
	}
	release_spinlock(&(((ymf724_hw*)card->hw)->codec.lock));
	restore_interrupts(cp);
	ddprintf("ymf724_CodecWrite()return! codec.lock = %lx\n",((ymf724_hw*)card->hw)->codec.lock);	
	return ret;
}


static status_t ymf724_InitCodec(sound_card_t *card)
{
	status_t err = B_OK;
	ddprintf("ymf724_InitCodec(): here!\n");
	
	if (pci_config_read(card, DS_1_CONTROL, 1) & 0x03) {
		pci_config_write(card, DS_1_CONTROL, 1, 0x03, 0xff);
		pci_config_write(card, DS_1_CONTROL, 1, 0x0, 0xff);
		snooze(700e3); /* wait 700 millisec */
	}
	/* Check if AC97 link with YMF724 is OK */
	if (mapped_read_16(&(((ymf724_hw*)card->hw)->native_regs), AC97_STATUS_ADDRESS) & (1<<14)) 
	{
		dprintf("ymf724_InitCodec: error - AC link is disabled!\n");
		return B_ERROR;
	}
	err = ac97init(&((ymf724_hw*)card->hw)->codec, 
					(void *)card,
					ymf724_CodecWrite,
					ymf724_CodecRead);

	ddprintf("ymf724_InitCodec(): ac97init err = %ld!\n",err);
	return err;				
}

static void ymf724_UninitCodec(sound_card_t *card)
{
	ac97uninit(&((ymf724_hw*)card->hw)->codec); 
}

static status_t ymf724_SoundSetup(sound_card_t *card, sound_setup *ss)
{
	return AC97_SoundSetup(&((ymf724_hw*)card->hw)->codec, ss);
}

static status_t  ymf724_GetSoundSetup(sound_card_t *card, sound_setup *ss)
{
	status_t ret;
	ddprintf("ymf724_GetSoundSetup: ss = 0x%lx\n",(uint32)ss);
	ret = AC97_GetSoundSetup(&((ymf724_hw*)card->hw)->codec,ss);
	ss->sample_rate = kHz_44_1;
	return ret;
}

static void ymf724_SetCaptureSR(sound_card_t *card, uint32 sample_rate)
{
	uint16 sr = (uint16)(48000 * 4096 / sample_rate) - 1;
	
	mapped_write_16(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_SAMPLING_RATE, sr, 0xffff);

	ddprintf("ymf724_SetCaptureSR(): sampling rate = %X\n", 
				mapped_read_16(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_SAMPLING_RATE));
}

static status_t ymf724_SetCaptureDmaBufOld(sound_card_t *card, uint32 size, void** addr)
{
	uint32 s = size;
	if(s < 4096) return B_ERROR; // 2K half buffer is lower limit caused by ymf724 interrupt accurency(256 48Kh clocks)
	return ymf724_SetCaptureDmaBuf(card, &s, addr);	
}
static status_t ymf724_SetCaptureDmaBuf(sound_card_t *card, uint32* pSize, void** addr)
{
	void* phys_addr;
	area_id AreaId;
	int bank_num;
	uint32 size = *pSize;
	size_t tmp_size;
	
	if(size < 4096) size = 4096; // 2K half buffer is lower limit caused by ymf724 interrupt accurency(256 48Kh clocks)
	tmp_size = size + 0x400;

	AreaId = allocate_memory("ymf724_CaptureDmaBuf",
				addr, &phys_addr, &tmp_size);
	if (AreaId < 0)
	{
		ddprintf("Error: Memory allocation for ymf724_CaptureDmaBuf failed!!\n");
		ddprintf("ymf724_SetCaptureDmaBuf: return %ld;\n",AreaId);
		return AreaId;
	}
	if(((ymf724_hw*)card->hw)->PcmCaptureAreaId >= 0) delete_area(((ymf724_hw*)card->hw)->PcmCaptureAreaId);
	((ymf724_hw*)card->hw)->PcmCaptureAreaId = AreaId; 
	memset(*addr,0,tmp_size);
	*addr += 0x400;
	phys_addr += 0x400;

	for(bank_num = 0; bank_num < 2; bank_num++) {
		((ymf724_hw*)card->hw)->CaptureCtrl.ADCBank[bank_num][CapPgBase] = (uint32)(phys_addr);
		((ymf724_hw*)card->hw)->CaptureCtrl.ADCBank[bank_num][CapPgLoopEnd] = (uint32)(size);
	}
	*pSize = size;
	return B_OK;
}

static void ymf724_SetCaptureFormat(sound_card_t *card, int num_of_bits, int num_of_ch)
{
	uint8 format;

	if(num_of_bits != 8 && num_of_bits != 16) return; //bad format	
	if (num_of_ch != 2 && num_of_ch != 1) return; //bad format

	format = (num_of_ch & 0x02) | ((num_of_bits & 0x08)>>3);
	mapped_write_8(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_FORMAT, format, 0xff);

	ddprintf("ymf724_SetCaptureFormat(): format = %X\n", mapped_read_8(&((ymf724_hw*)card->hw)->native_regs, DS_ADC_FORMAT));
}

static void* ymf724_PrepareCaptureControl(ymf724_capture_control_t* ctrl, uint32 cap_bank_size)
{
	size_t buf_size;
	uint32 *Addr;
	void *PhysAddr;

	buf_size =	(sizeof(uint32)) * (
									cap_bank_size * 2 * 2 // 2 slots (Rec and ADC), each consists of 2 banks
									);

	ddprintf("ymf724_PrepareCaptureControl: cap_bank_size = %ld, buf_size = %ld\n", cap_bank_size, buf_size);

	ctrl->areaId = allocate_memory("CaptureControlArea",(void**)&Addr, &PhysAddr, &buf_size);
	if (ctrl->areaId < 0)
	{
		derror("ymf724_Init(): error allocaing memory!\n");
		return NULL;
	}
	memset((void *)Addr, 0, buf_size);
	ctrl->pCtrlData = Addr;

	/* Skip Rec slot (2 banks) and initialize ADC slot */
	ctrl->ADCBank[0] = (vuint32 *)(Addr + 2 * cap_bank_size);
	ctrl->ADCBank[1] = (vuint32 *)(ctrl->ADCBank[0] + cap_bank_size);

	return PhysAddr;
}

