#ifndef ESS_H
#define ESS_H


#include <OS.h>
#include <KernelExport.h>

#include <PCI.h>
#include <Drivers.h>
#include <Errors.h>
#include "sound.h"

#include <ISA.h>
#include <module.h>
#include <isapnp.h>

#include <stdio.h>
#include <stdlib.h>


#include "audio_drv.h"
//#include "audio_card.h"
//#include "device.h"
#include "debug.h"

#define IS_ISA_BUS		1
#define INTEL_VER		1			// to out/inp direct from the port 

/* DSP part */
#define DSP_RESET       0x6 
#define DSP_READ        0xA 
#define DSP_WRITE       0xC 
#define DSP_COMMAND     0xC 	// for write
#define DSP_STATUS      0xC 	// for read
#define DSP_DATAD_AVAIL  0xE 
#define DSP_DATA_AVL16  0xF 
#define DSP_DATA_MIDI   SB_MIDI_DATA  


#define DSP_TIME_CT		0x40
#define DSP_BLOCK_SIZE 	0x48

#define SBPRO_VERSION				0xE1

#define DMA_TRANSFER_SIZE			16*B_PAGE_SIZE
#define BUFFER_SIZE					2*DMA_TRANSFER_SIZE
#define SBPRO_DELAY					10000
#define IRQ_STAT        			0x82 		/* on SBPRO */

/*mixer commands*/
#define SBPRO_VOICE					0x04
#define SBPRO_MIC					0x0a
#define SBPRO_INPUT_MUX				0x0c
#define SBPRO_OUTPUT_MUX			0x0e
#define SBPRO_MASTER				0x22
#define SBPRO_MIDI					0x26
#define SBPRO_CD					0x28
#define SBPRO_LINE					0x2e

/* midi ports */ 
#define SB_MIDI_DATA				0x330						/* R/W */
#define SB_MIDI_CMD					(SB_MIDI_DATA + 1)		/* W only */
#define SB_MIDI_STATUS				(SB_MIDI_DATA + 1)		/* R only */




///////////////// added by Gg   //////////////////////
#define	SB_INPUT				1
#define	SB_OUTPUT				2

/* data formats for SB */
#define DATA_8_MONO				0x10
#define DATA_8_STEREO 			0x20
#define DATA_16_MONO			0x40
#define DATA_16_STEREO			0x80

/* DSP part */
#define DSP_RESET       0x6 
#define DSP_READ        0xA 
#define DSP_WRITE       0xC 
#define DSP_COMMAND     0xC 	// for write
#define DSP_STATUS      0xC 	// for read
#define DSP_DATA_AVAIL  0xE 
#define DSP_DATA_AVL16  0xF 

/* DSP Commands */ 
#define DSP_CMD_SPKON           0xD1 
#define DSP_CMD_SPKOFF          0xD3 
#define DSP_CMD_DMAON           0xD0 
#define DSP_CMD_DMAOFF          0xD4 

#define	ON		TRUE
#define OFF		FALSE


///////////////////////// 		part for ESS1868 / 1869 / 1879 chips  //////////////


typedef struct _timers_data_t {
	vuint64 IntCount;	
	bigtime_t SystemClock;
	int32 Lock;	
} timers_data_t;


typedef struct _dma_buffer {
	char *data;
	int size;
	area_id area;

	struct { bool IsStereo; bool Is16bits;} PcmFormat;
	bool SetFormat;				// dma8 / dma16

	uint32 SampleRate;
	bool SetSampleRate;

	int LockFlag	;			// for benaphore
	sem_id	LockSem ;			// access to buffer
	int  whichHalf ;			// in which half I am 

	} dma_buffer;


typedef struct _Sb_hw {		// for SB / SB Pro
	char			name[B_OS_NAME_LENGTH];	/* used for resources */
	int16   		sb_caps ;					/* capabilities for ess 18XX cards */

	int				sb_port; 			/* audio_base address */
	int				sb_mpu_port;
	int 			sb_fm_port;			/* FM port */
	bool 			sb_init ;
	int 			sb_irq;
	int 			sb_dma8;			/* 8 bit channel */	
	int 			sb_dma16;			/* 16 bit channel in full duplex mode  */
	
	int				sb_irq_mode ; 		/* from where comes an interrupt */ 
	int				sb_irq_ok ; 
	uint16			sb_config_base ;	/* configuration base for ESS  */
	uint16			sb_dsp_ver ;		/* type of the DSP for ESS */
	uint16			sb_version ;
	
	unsigned char 	irq_bits ;			/*for ESS Sb*/
	unsigned char	dma_bits ;			/* for ESS Sb  */
	int				mixer_type ;
	int				sb_dsp_ok ;					// initialise OK

	int 			sb_data_format ;	/* only DATA_X_STEREO  */ 	
	int 			sb_direction ;		/* SB_INPUT/OUTPUT  */
	
	int 			sb_major, 			// exist 4 major versions of SB
					sb_minor;			// you set a lot with these values
	int 			sb_stereo_output,
					sb_stereo_input ;
	int				sb_dma ; 			// extended mode DMA	

	timers_data_t 	timers_data;	
	sem_id			SetupLockSem ;		/* to check/get/set something from/to card */

	int32			spinlock;		

	dma_buffer		write_buf, read_buf ;	// I think that is a single buffer

	sound_card_functions_t *func;

} sb_hw;





/* Audio device */
#define 	ESS_READ_DATA			0x0A
#define 	ESS_READ_BUF_STATUS		0x0E
#define		ESS_COMMAND				0x0C
#define		ESS_INTR_CONFIG			0xB1
#define		ESS_DRQ_CONFIG			0xB2
#define		ESS_DMA_START			0xB8

/* ESS commands + SB PRO commans included in "sbpro.h" */
#define 	ESS_CMD_VERSION				0xE1
#define		ESS_EXTENDED				0xC6
#define 	ESS_READ_EXTENDED			0xC0
#define 	ESS_READ_WRITE_DATA			0xC6


/* audio 1 extended mode */ 
#define		ESS_1_SAMPLE_RATE 		0xA1
#define		ESS_1_FILTER_CLOCK		0xA2
#define		ESS_1_COUNT_LOW			0xA4
#define		ESS_1_COUNT_HIGH		0xA5
#define		ESS_1_IN_VOL			0xB4
#define		ESS_1_DIRECT_ACCESS		0xB6
#define		ESS_1_CTRL1				0xB7
#define		ESS_1_CTRL2				0xB8
#define		ESS_1_TRANSF_TYPE		0xB9


/* audio 2 extended mode */ 
#define		ESS_2_SAMPLE_RATE		0x70
#define		ESS_2_MODE				0x71
#define		ESS_2_FILTER_CLOCK		0x72
#define		ESS_2_COUNT_LOW			0x74
#define		ESS_2_COUNT_HIGH		0x76
#define		ESS_2_CTRL1				0x78
#define		ESS_2_CTRL2				0x7A
#define		ESS_2_VOL_CTRL			0x7C
#define	 	ESS_2_CONFIG			0x7D

/* Configuration device */ 
#define		ESS_STATUS				0x05 
#define		ESS_STATUS_INTR			0x06
#define		ESS_MASK_INTR			0x07
#define 	ESS_MIXER_ADDR			0x04
#define 	ESS_MIXER_DATA			0x05



/* mixer extensions 		*/ 
#define		ESS_MIXER_EXT				0x40
#define		ESS_MIXER_DMA				0x0E
#define		ESS_MIXER_MAST_VOL			0x64
#define		ESS_MIXER_COUNT1			0x74
#define		ESS_MIXER_COUNT2			0x76



/*	ESS mixer registers 	*/
#define 	ESS_AUDIO1_PLAYBACK_VOL		0x14
#define		ESS_MASTER_VOL				0x32
#define 	ESS_DAC_VOL					0x36
#define 	ESS_AUX1_VOL				0x38
#define 	ESS_AUX2_VOL				0x3A
#define 	ESS_SPEAKER_VOL				0x3C
#define 	ESS_LINE_VOL				0x3E
#define 	ESS_SOURCE_SELECT			0x1C	// how did you know ???



#define		ESS_DELAY					20000
#define 	ESS_PAUSE					100





/*capabilities for ESS_18xx card */
#define ESS_CAP_PCM2		0x0001	/* Has two useable PCM */
#define ESS_CAP_IS_3D		0x0002	/* Has 3D Spatializer */
#define ESS_CAP_RECMIX		0x0004	/* Has record mixer */
#define ESS_CAP_DUPLEX_MONO 0x0008	/* Has mono duplex only */
#define ESS_CAP_DUPLEX_SAME 0x0010	/* Playback and record must share the same rate */
#define ESS_CAP_NEW_RATE	0x0020	/* More precise rate setting */
#define ESS_CAP_AUXB		0x0040	/* AuxB mixer control */
#define ESS_CAP_SPEAKER		0x0080	/* Speaker mixer control */
#define ESS_CAP_MONO		0x0100	/* Mono_in mixer control */
#define ESS_CAP_I2S			0x0200	/* I2S mixer control */
#define ESS_CAP_MUTEREC		0x0400	/* Record source can be muted */
#define ESS_CAP_CONTROL		0x0800	/* Has control ports */
#define ESS_CAP_HWV			0x1000	/* Has hardware volume */





#endif




