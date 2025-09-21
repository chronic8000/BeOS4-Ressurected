/************************************************************************/
/*                                                                      */
/*                              yamaha.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

/* Important YMF724 values */
#define YAMAHA_VENDOR_ID		0x1073	/* Yamaha Corp. */

#define YMF724_DEVICE_ID		0x0004	
#define YMF724F_DEVICE_ID		0x000d
#define YMF734_DEVICE_ID		0x0005	
#define YMF737_DEVICE_ID		0x0008
#define YMF738_DEVICE_ID		0x0020	
#define YMF738_TEG_DEVICE_ID	0x0006
#define YMF740_DEVICE_ID		0x000a	
#define YMF740C_DEVICE_ID		0x000c
#define YMF744_DEVICE_ID		0x0010	
#define YMF754_DEVICE_ID		0x0012
	
#define LEGACY_AUDIO_ENABLE 	0x107f	/* Enables legacy audio mode when written into LEGACY_CONTROL reg */
#define STOP_DSP				0x0
#define DS1_MUTE				0x80000000
#define DEFAULT_VOLUME			0x3fff3fff
#define SOFT_RESET				0x10000
#define PCI_ENABLE_BUS_MASTERING	0x0004

/* AC97 codec */
#define AC97_CMD_DATA			0x0060
#define AC97_CMD_ADDRESS		0x0062
#define AC97_STATUS_DATA		0x0064
#define AC97_STATUS_ADDRESS		0x0066
#define AC97_BAD_VALUE			0xffff

/* PCI configuration register */
#define PCI_COMMAND				0x04
#define	PCI_AUDIO_BASE			0x10	/* Physical address of the PCI Audio registers */
#define LEGACY_CONTROL			0x40	/* Legacy mode control reg */
#define EXT_LEGACY_CONTROL		0x42	/* Includes base ports for FM, SB, MPU, JOY */
#define DS_1_CONTROL			0x48	/* YMF724 Control register */

/* Native PCI Audio mode registers */
#define DS_GLOBAL_CONT			0x0008
#define DS_SPDIF_CTRL			0x0018
#define DS_LEG_OUT_L			0x0080
#define DS_DAC_OUT_L			0x0084
#define	DS_ADC_INP_L			0x00a8
#define DS_ADC_SAMPLING_RATE	0x00c0
#define DS_ADC_FORMAT			0x00c8
#define DS_STATUS				0x0100
#define DS_CSEL					0x0104
#define	DS_MODE					0x0108
#define DS_SAMP_CNT				0x010c
#define	DS_NUM_SAMP				0x0110
#define DS_CFG					0x0114 		
#define DS_PLAY_CSIZE	 		0x0140
#define DS_REC_CSIZE	 		0x0144
#define DS_EFF_CSIZE	 		0x0148
#define DS_WORK_SIZE			0x014c
#define DS_MAP_REC				0x0150
#define DS_MAP_EFF				0x0154
#define DS_PLAY_CBASE			0x0158
#define DS_REC_CBASE			0x015c
#define DS_EFF_CBASE			0x0160
#define DS_WORK_BASE			0x0164
#define	DS_DSP_BASE				0x1000
#define	DS_CONT_BASE			0x4000

/* Bank strucuture for playback */
enum play_bank_regs {
	 	Format = 0,
	 	LoopDefault,
	 	PgBase,
	 	PgLoop,
	 	PgLoopEnd,
	 	PgLoopFrac,
	 	PgDeltaEnd,
	 	LpfKEnd,
	 	EgGainEnd,
	 	LchGainEnd,
	 	RchGainEnd,
	 	Effect1GainEnd,
	 	Effect2GainEnd,
	 	Effect3GainEnd,
	 	LpfQ,
	 	Status,
	 	NumOfFrames,
	 	LoopCount,
	 	PgStart,
	 	PgStartFrac,
	 	PgDelta,
	 	LpfK,
	 	EgGain,
	 	LchGain,
	 	RchGain,
	 	Effect1Gain,
	 	Effect2Gain,
	 	Effect3Gain
}; 

/* Bank structure for capture */ 
enum cap_bank_regs {
	CapPgBase = 0,
	CapPgLoopEnd,
	CapPgStart,
	CapNumOfLoops,
};

/* end yamaha.h */
