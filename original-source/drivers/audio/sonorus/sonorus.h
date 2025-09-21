/*	sonorus.h	*/

#if !defined(sonorus_h)
#define sonorus_h
#include "multi_audio.h"
/* basic data structs and enums -- */
enum {
	SONORUS_TEST_WRITE = 25252,
	SONORUS_TEST_START,
	SONORUS_TEST_STOP
};

/* modes that could be supported */
enum {
	SONORUS_MODE_0202,
	SONORUS_MODE_1111,
	SONORUS_MODE_2020
};

/*
	Buffer types supported.
*/

typedef enum
{
	STUD_BUF_INT16 = 0			// transfer to/from 16-bit signed buffer.
  , STUD_BUF_INT16_GAIN			// transfer to/from 16-bit signed buffer with gain coeff
  , STUD_BUF_INT32				// transfer to/from 32-bit signed buffer with gain coeff
  , STUD_BUF_FLOAT_GAIN
}
studBufType_t;

#define sampleSizeFromBufType( t )	(((int)(t) >= STUD_BUF_INT32) ? (4) : (2))



// values for _dspCaps
enum { kSupportsNoDSP = 0 , kSupportsMixing = 1 , kSupportsEQ = 2 };

// buffer transfer types for WriteHostBuf() and ReadHostBuf().
typedef enum
{
	kStudXfer32 = 0			// buffer is 32-bit values, DSP xfer as 24-bit words
  , kStudXfer16				// buffer is 16-bit values, DSP xfer in upper 16 bits of 24-bit word
  , kStudXfer16Gain			// buffer is 16-bit values, DSP xfer with float gain scaling
  , kStudXfer16Packed		// buffer is 16-bit values, DSP xfer in packed mode
  , kStudXferFltGain		// buffer is float values, DSP xfer with gain to 24-bits
}
studXferType_t;

/*
	Definitions for SetSyncParms().
*/

typedef enum
{
	STUD_SYNCSRC_INT = 0		// internal sync source
  , STUD_SYNCSRC_EXTA			// external sync source from optical input A
  , STUD_SYNCSRC_EXTB			// external sync source from optical input B
  , STUD_SYNCSRC_EXTWC			// external sync source from backplate WC input
  , STUD_SYNCSRC_DSP			// internal sync source from DSP timer 1
}
studSyncSrc_t;

/*
	Parameter IDs passed to SetParam/GetParam.
*/

typedef enum
{
	STUD_PARM_SPEED = 0			// 0x080000 == unity, 0x7FFFFF = 15.99, etc
  , STUD_PARM_POSL				// lower 24 bits of current position
  , STUD_PARM_POSH				// upper 8 bits of current position
  , STUD_PARM_INITOFST			// initial sample offset into first buffer
  , STUD_PARM_METER				// current maximum 16-bit sample (write 0 to clear)
  , STUD_PARM_INPMON			// pos value ramps to monitor input, neg ramps to repro
  , STUD_PARM_OUTENB			// pos value ramps output OFF, neg ramps output ON
  , STUD_PARM_RECENB			// set to TRUE to enable play/record, FALSE for scrub
  , STUD_PARM_SRATE				// current measured sample rate.
  , STUD_PARM_SPARECYCS			// number of spare DSP cycles / 5
  , STUD_PARM_PORTMODE			// set at initialization time to set serial port modes
  , STUD_PARM_PACKMODE			// set to 1 for fast packed xfers to/from DSP
  , STUD_PARM_SYNCSELECT		// set to 1 to sync DSP sample loop to sync source B
  , STUD_PARM_DSPSRADJ			// used internally.
  , STUD_PARM_BOARDREV			// hardware revision of the primary board (board zero).
  , STUD_PARM_TIMECODEL			// lower 24 bits of extracted 32-bit timecode
  , STUD_PARM_TIMECODEH			// upper 8 bits of extracted 32-bit timecode
  , STUD_PARM_TCPOSL			// lower 24 bits of sample position when timcode arrived
  , STUD_PARM_TCPOSH			// upper 8 bits of sample position when timcode arrived
}
studParmID_t;

// some general defs
#define DEFAULT_SAMPLE_RATE	44100
#define DEFAULT_LOCK_SOURCE	STUD_SYNCSRC_INT
#define DEFAULT_MULTI_LOCK_SOURCE B_MULTI_LOCK_INTERNAL
#define DEFAULT_MULTI_LOCK_DATA 0
#define DEFAULT_MULTI_TIMECODE_SOURCE 0

#define MAX_CHNLS_PER_BD	16		// number of channels per board.
#define MAX_MONS_PER_BD		2		// number of monitor channels per board.
#define MAX_STUDIO_BD		4		// number of StudI/O boards supported.
#define MAX_CHNLS			(MAX_CHNLS_PER_BD*MAX_STUDIO_BD)
#define MAX_MONS			(MAX_MONS_PER_BD*MAX_STUDIO_BD)

#define OUTPUT_MASK_1111_ADAT	0x000000ff
#define OUTPUT_MASK_1111_SPDIF	0x00000300
#define OUTPUT_MASK_1111	(OUTPUT_MASK_1111_SPDIF | OUTPUT_MASK_1111_ADAT)

#define INPUT_MASK_1111_ADAT	0x0003fc00
#define INPUT_MASK_1111_SPDIF	0x000c0000
#define INPUT_MASK_1111		(INPUT_MASK_1111_SPDIF | INPUT_MASK_1111_ADAT)

#define STUD_SPEED_UNITY	0x80000	// unity play speed for STUD_PARM_SPEED parameter
#define STUD_SPEED_MAX		(4*STUD_SPEED_UNITY)
#define STUD_SPEED_MIN		(-4*STUD_SPEED_UNITY)

#define STUD_UNITY_GAIN		0x200000	// unity gain for each of the gain/pans above
#define STUD_MAX_GAIN		0x7FFFFF	// max gain is about +12 dB

#define MIN_RAMP_SIZE 	64

#define SUPPORTED_EVENT_MASK   (B_MULTI_EVENT_STARTED | \
								B_MULTI_EVENT_STOPPED   | \
								B_MULTI_EVENT_CHANNEL_FORMAT_CHANGED  | \
								B_MULTI_EVENT_CONTROL_CHANGED ) 
#define	MAX_EVENTS		16

// channel info structs
typedef struct
{
	uint32		reserved;
	int32		begGain[ MAX_CHNLS ];
	int32		begPanL[ MAX_CHNLS ];
	int32		begPanR[ MAX_CHNLS ];
}
chnlInfoX_t;

typedef struct
{
	uint32		nSamps;
	int32		endGain[ MAX_CHNLS ];
	int32		endPanL[ MAX_CHNLS ];
	int32		endPanR[ MAX_CHNLS ];
}
chnlInfoY_t;

typedef struct HostIfc_t HostIfc_t;
struct HostIfc_t {
	uint32		reserved[4];
	uint32		hctr;
	uint32		hstr;
	uint32		hcvr;
	uint32		hdat[16377];	/* up to 64k limit */
};

typedef enum memSpace_t { xMem = 0 , yMem = 2 , pMem = 4 } memSpace_t;
typedef struct sonorus_channel sonorus_channel;
struct sonorus_channel {
	char *				buf[2];
	uint32				current_buffer;	// 0 or 1
	uint32				index;
	int32				monitor_ramp;
	bigtime_t			monitor_ts;
	int32				output_ramp;
	bigtime_t			output_ts;

};
	 
typedef struct timer_plus timer_plus;
struct timer_plus {
	timer		timerx;
	void *		cookie;
};	

typedef struct sonorus_dev sonorus_dev;
struct sonorus_dev {
	pci_info				card_info;
	area_id					dsp_area;
	HostIfc_t *				dsp;
	uint32*					hdata;
	area_id					hdata_area;					
	int32					spinlock;
	sem_id					open_sem;
	sem_id					irq_sem;
	sem_id					control_sem;
	sem_id					queue_sem;
	int32					open_mode;
	int						ix;				/* make life easy */
	char					revision;
	uint32					sample_rate;	/* move this! */
	studSyncSrc_t			lock_source;    
	uint32					timecode;
	uint64					tc_position;
	uint64					position;
	bigtime_t				timestamp;
	multi_channel_enable	mce;
	uint32					channels_enabled;
	area_id					channel_area;
	sonorus_channel			channels[MAX_CHNLS];
	uint32					channel_buff_size;	/* in samples */
	/* controls */
	chnlInfoX_t				chix[2];
	chnlInfoY_t				chiy[2];
	bool					record_enable;
	uint32					preroll;
	bigtime_t				start_time;
	timer_plus				timer_stuff;
	/* multi block configuration */
	/* errrr, make that events   */
	uint32					current_mask;
	uint32					queue_size;
	uint32					event_count;
	multi_get_event			event_queue[MAX_EVENTS];
	sem_id					event_sem;
};

typedef struct open_sonorus open_sonorus;
struct open_sonorus {
	sonorus_dev *		device;
	uint32				mode;
};

/* MIX MIX MIX it up     */
#define MAX_CONTROLS 10
#define MAX_CONTROLS_1111 10
#define MAX_MIX_CHANNELS 42
typedef struct mix_channel_controls mix_channel_controls;
struct mix_channel_controls {
	int32 channel;
	int32 control_count;
	int32 *control_id;
};	




/* specific to the 1111 configuration!!!!!!! */
/*	Definitions for FPGA bits for 1111.ttf:  */
#define	SYNC_SRATE_44K	0x03	// set bits for 44.1 KHz
#define SYNC_SRATE_48K	0x13	// set bits for 48 KHz
#define SYNC_SRATE_PLL	0x23	// set bits for PLL sample clock
#define SYNC_SRATE_AES	0x33	// set bits for AES sample clock

#define SYNC_EXT_OPTA	0x03	// set bits for optical 1 source
#define SYNC_EXT_OPTB	0x07	// set bits for optical 2 source
#define SYNC_EXT_DSP	0x0B	// set bits for DSP timer source
#define SYNC_EXT_EXT	0x0F	// set bits for external source


/* hardware specific defines */

#define MOTOROLA_VENDOR_ID 0x1057
#define DSP56300_DEVICE_ID 0x1801
#define SONORUS_VENDOR_ID 0x5353
#define STUDIO_DEVICE_ID 0x0001

#define M_HCVR_HC		0x000001
#define M_HCVR_HVMSK	0x0000FE
#define M_HCVR_HNMI		0x008000

#define M_AAR1		0xFFFFF8
#define M_AAR2		0xFFFFF7
#define M_AAR3		0xFFFFF6

#define EEPROM_PRESENT_FLAG		0x15		// location to test for EEPROM presence
#define UNMAPPED_Y_LOCATION		0x100000	// location to write in Y: for rev B altera load
#define ALTERA_REPROGRAM_ON		0x1C05		// write this to AAR2 to assert reprogram pin
#define ALTERA_REPROGRAM_OFF	0x1C01		// write this to AAR2 to deassert reprogram pin

#define HC_RESET	0x7C		// cause DSP to re-enter boot mode
#define HC_INTACK	0x7E		// host acknowledges interrupt (turns off HINT)
#define HC_SETR7	0x80		// set monitor pointer register r7
#define HC_SETN7	0x82		// set monitor index register r7
#define HC_READX	0x84		// cause DSP to send x:(r7)+n7 to host
#define HC_READY	0x86		// cause DSP to send y:(r7)+n7 to host
#define HC_READP	0x88		// cause DSP to send p:(r7)+n7 to host
#define HC_WRITEX	0x8A		// cause DSP to store host word to x:(r7)+n7
#define HC_WRITEY	0x8C		// cause DSP to store host word to y:(r7)+n7
#define HC_WRITEP	0x8E		// cause DSP to store host word to p:(r7)+n7
#define HC_DMA_RD_X	0x90		// cause DSP to transfer data to X: using DMA channel 0
#define HC_DMA_RD_Y	0x92		// cause DSP to transfer data to Y: using DMA channel 0 (not implemented!)
#define HC_DMA_WR_X	0x94		// allow host to transfer data to X: using DMA channel 0
#define HC_DMA_WR_Y	0x96		// allow host to transfer data to Y: using DMA channel 0 (not implemented!)
#define HC_FXW_ON	0x98		// turn on fast interrupt driven writes to X:
#define HC_FXW_OFF	0x9A		// turn of fast interrupt driven writes to X:
#define HC_FXR_ON	0x9C		// turn on fast interrupt driven reads from X:
#define HC_FXR_OFF	0x9E		// turn off fast interrupt driven reads from X:
#define HC_INTDIS	0xA0		// disable DSP from interrupting host
#define HC_INTENB	0xA2		// enable DSP interrupts to host
#define HC_STARTIO	0xB0		// start serial DMA for all audio channels
#define HC_STOPIO	0xB2		// stop serial DMA for all audio channels
#define HC_WRITE_FPGA_B	0xC0	// write byte to Altera to program (Rev B boards only).
#define HC_WRITE_EEPROM	0xC2	// write byte to EEProm.

// DSP memory map (more defs in StudioDev.h)
#define FPGA_BASE		0xE00000		// base in Y: memory
#define EEPROM_BASE		0xD00000		// base in P: memory

#define CHNL_BUF_ORG	0x400000		// interleaved play record buffers in X: memory
#define CHNL_BUF_STRIDE	18				// interleave factor.
#define CHNL_BUF_SIZE   3640    		// number of samples per channel in DSP sample buffer 

#define CHNL_INFO_ORG	0x200			// channel info in internal X/Y memory
#define CHNL_INFO_SIZE	0x31			// size of each.



#define M_HCTR_HF0		0x000008		// host flag 0 (host->DSP)
#define M_HCTR_HF1		0x000010		// host flag 1 (host->DSP)
#define M_HCTR_HF2		0x000020		// host flag 2 (host->DSP)

#define M_HCTR_SFT		0x000080		// slave fetch type

#define M_HCTR_HTF0		0x000000		// host transmit mode 0 (16-bit packed)
#define M_HCTR_HTF1		0x000100		// right-alligned 24 bit zero extended
#define M_HCTR_HTF2		0x000200		// right-alligned 24 bit sign extended
#define M_HCTR_HTF3		0x000300		// left-alligned 24 bit
#define M_HCTR_HTF_MASK	0x000300		// mask of all HTF bits

#define M_HCTR_HRF0		0x000000		// host receive mode 0 (16-bit packed)
#define M_HCTR_HRF1		0x000800		// right-alligned 24 bit zero extended
#define M_HCTR_HRF2		0x001000		// right-alligned 24 bit sign extended
#define M_HCTR_HRF3		0x001800		// left-alligned 24 bit
#define M_HCTR_HRF_MASK	0x001800		// mask of all HRF bits

#define M_HCTR_TWSD		0x080000		// target wait state disable

// useful defines for host status register.
#define M_HSTR_TRDY		0x000001		// host->DSP FIFO is empty
#define M_HSTR_HTRQ		0x000002		// host->DSP FIFO is not full
#define M_HSTR_HRRQ		0x000004		// DSP->host FIFO not empty
#define M_HSTR_HF3		0x000008		// host flag 3 (DSP->host)
#define M_HSTR_HF4		0x000010		// host flag 3 (DSP->host)
#define M_HSTR_HF5		0x000020		// host flag 3 (DSP->host)
#define M_HSTR_HINT		0x000040		// status of INTA line


#endif /* sonorus_h */




