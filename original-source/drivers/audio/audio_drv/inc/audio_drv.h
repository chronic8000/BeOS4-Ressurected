/************************************************************************/
/*                                                                      */
/*                              audio_drv.h                           	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#ifndef _AUDIO_DRV_H
#define _AUDIO_DRV_H

#include <PCI.h> 
#include <ISA.h>
#include <isapnp.h>
#include <config_manager.h>

#include "sound.h" 

/************************************************************************************/
#ifdef GENERAL_INFO_MODULE
/*
This code fixes the linker problem. Without it in case then hw depended 
driver do not use some of audio_drv.a library function linker does not link
the audio_drv.a library to the binary. So driver interface functions are not 
linked.
*/ 
void uninit_driver (void);
void FixLinker()
{
	 init_driver();
}
#endif
/**************************************************************************************/

typedef struct _sound_card_t sound_card_t;


typedef struct _mpu401_io_hooks_t {
	uchar (*read_port_func)(void *, int);
	void (*write_port_func)(void *, int, uchar);
} mpu401_io_hooks_t;

typedef struct _joystick_io_hooks_t {
	uchar (*read_port_func)(void *);
	void (*write_port_func)(void *, uchar);
} joy_io_hooks_t;


typedef struct _sound_card_functions_t
{
	status_t	(*Init) (sound_card_t * card);
	status_t	(*Uninit) (sound_card_t *card);
	void		(*StartPcm) (sound_card_t *card);
	void		(*StopPcm) (sound_card_t *card);
	void		(*GetClocks) (sound_card_t * card, bigtime_t* pSampleClock, bigtime_t* pSystemClock);
	status_t	(*SoundSetup) (sound_card_t *card, sound_setup *ss);
	status_t	(*GetSoundSetup) (sound_card_t *card, sound_setup *ss);
	void		(*SetPlaybackSR) (sound_card_t *card, uint32 sample_rate);
	status_t	(*SetPlaybackDmaBufOld) (sound_card_t *card, uint32 size, void** addr);
	void		(*SetPlaybackFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);
	void		(*SetCaptureSR) (sound_card_t *card, uint32 sample_rate);
	status_t	(*SetCaptureDmaBufOld) (sound_card_t *card, uint32 size, void** addr);
	void		(*SetCaptureFormat) (sound_card_t *card, int num_of_bits, int num_of_ch);

	status_t	(*InitJoystick)(sound_card_t *);
	void	 	(*enable_gameport) (sound_card_t *);
	void 		(*disable_gameport) (sound_card_t *);

	status_t	(*InitMidi)(sound_card_t *);
	void	 	(*enable_midi) (sound_card_t *);
	void 		(*disable_midi) (sound_card_t *);
	void 		(*enable_midi_receiver_interrupts) (sound_card_t *);
	void 		(*disable_midi_receiver_interrupts) (sound_card_t *);

	joy_io_hooks_t *joy_io_hooks;// nonzero value of this pointer means that mpu401 driver will use alternative access method for mpu401 IO ports 
	mpu401_io_hooks_t *mpu401_io_hooks; // nonzero value of this pointer means that gameport driver will use alternative access method for joystick IO ports

	status_t	(*SetPlaybackDmaBuf) (sound_card_t *card, uint32* size, void** addr);
	status_t	(*SetCaptureDmaBuf) (sound_card_t *card, uint32* size, void** addr);

} sound_card_functions_t;



struct _sound_card_t {
	void*	host;
	char	name[B_OS_NAME_LENGTH];
	int		idx; // card index (separate for each card type)
	bool	(*midi_interrupt)(sound_card_t *);
	bool	(*pcm_playback_interrupt)(sound_card_t *, int half_no);
	bool	(*pcm_capture_interrupt)(sound_card_t *, int half_no);
	
	sound_card_functions_t* func;
	void* hw;
	union{
		struct {
			int32 lock;
			pci_info info;
		}pci;
		struct {
			int32 lock;
			struct isa_info info;
		}isa;
	} bus;
	int joy_base;
	int mpu401_base;
};

typedef enum {NO_HW,PCI,ISA} bus_t;

typedef struct card_desctriptor_t{
	const char *name;
	bus_t bus;
	sound_card_functions_t *func;
	uint32	vendor_id;		
	uint32	device_id;		
	uint32	logical_id;		// or use for function id in pci case
} card_descrtiptor_t;

extern card_descrtiptor_t* CardDescriptors[]; /*pointer for null_terminated array*/
void GetDriverSettings();


#endif
