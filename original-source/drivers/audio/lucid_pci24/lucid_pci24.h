/*
 * pci24.h
 */

/* NOTE: The DSP on the PCI24 is a Motorola 56301.
Hence, all references to the PCI24 "dsp" or "56301' are synonymous. */

#if !defined(_LUCID_PCI24_H)
#define _LUCID_PCI24_H

#if !defined(_DRIVERS_H)
	#include <Drivers.h>
#endif /* _DRIVERS_H */
#if !defined(_SUPPORT_DEFS_H)
	#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */
#if !defined(_OS_H)
	#include <OS.h>
#endif /* _OS_H */
#if !defined(_PCI_H)
	#include <PCI.h>
#endif/* _PCI_H */
#if !defined(_AUDIO_DRIVER_H)
	#include <audio_driver.h>
#endif /* _AUDIO_DRIVER_H */

#if !defined(DEBUG)
	#define DEBUG 1
#endif

#if defined(DEBUG)
	#define ddprintf dprintf
#else
	#define ddprintf 0
#endif

#define EXPORT __declspec(export)

#define NUM_CARDS 3
#define DEVNAME 32

#define FIFO_DEPTH	0x200
#define RD_BUF_SIZE 2048
#define WR_BUF_SIZE 2048

#define LUCID_VENDOR_ID		0x1057	/* Lucid Inc */
#define PCI24_DEVICE_ID		0x1801	/* PCI24 */

typedef audio_format pcm_cfg;
typedef audio_buf_header pcm_buf_hdr;

typedef struct pcm_dev {
	struct pci24_dev * card;
	char		name[DEVNAME];
	int32		open_count;
	int32		open_mode;

/* playback */
	int32 		write_waiting;	//semaphore flag
	uint32 		left_to_write;
	size_t 		wr_fifo_size;
	int16		*wr_base;
	int16		*wr_int_ptr;
	int16		*wr_thr_ptr;
	sem_id 		wr_entry_sem;
	sem_id 		wr_int_sem;

/* recording */
	int32 		read_waiting;
	int32		first_read;		//since open
	uint32 		left_to_read;
	size_t 		rd_fifo_size;
	bigtime_t	rd_time;
	bigtime_t	next_rd_time;
	int16		*rd_base;
	int16		*rd_int_ptr;
	int16		*rd_thr_ptr;
	sem_id 		rd_entry_sem;
	sem_id 		rd_int_sem;

/* buffers are owned by the device record (because of allocation) */

	pcm_cfg		config;
} pcm_dev;

typedef struct pci24_dev {
	char			name[DEVNAME];	/* used for resources */
	int32			hardware;		/* spinlock */
	uint32			*dsp;			/* base address */
	area_id			area;			/* mapped reg_area */
	char			*buf;
	area_id	 		buf_area;	
	int32			inth_count;
	pci_info		info;
	pcm_dev			pcm;
} pci24_dev;

extern int32 num_cards;
extern pci24_dev cards[NUM_CARDS];

extern uint32 send_host_cmd(uint32 command, uint32 *dsp);
extern uint32 wait_on_status(uint32 *dsp);
extern void increment_interrupt_handler(pci24_dev *);
extern void decrement_interrupt_handler(pci24_dev *);

enum {
	kPlayback = 1,
	kRecord = 2
};

/**********************************************************/
/* Address bases */
/**********************************************************/

//Host register offsets
enum
{
	CTR = 4,
	STR = 5,
	CVR = 6,
	RXS = 7,
	TXR = 8
};

/*
// 56301 Host registers - PCI memory address space
typedef struct
{
	volatile unsigned long		reserved[4];		// don't access these registers
	volatile unsigned long		HostCTR;			// DSP control register, host side
	volatile unsigned long		HostSTR;			// DSP status register, host side
	volatile unsigned long		HostCVR;			// DSP command vector register, host side
	volatile unsigned long		HostRXS;			// DSP to Host FIFO (PCI target mode)
	volatile unsigned long		HostTXR;			// Host to DSP FIFO (PCI target mode)
} DSPMemRecord, *DSPMemPtr, **DSPMemHdl;

// 56301 Host registers - PCI configuration address space
typedef struct
{
	unsigned long		CDID_CVID;
	unsigned long		CSTR_CCMR;
	unsigned long		CCCR_CRID;
	unsigned long		CLAT;
	unsigned long		CBMA;
	unsigned long		reserved[58];
	unsigned long		CILP;
} dspConfigRecord, *dspConfigPtr, **dspConfigHdl;
*/

/* 56301 enums */
enum
{
/* Bits in the HostCTR (Host Control) register */
kTREQ		=	0x02,
kRREQ		=	0x04,
kHF0		=	0x08,
kHF1		=	0x10,
kHF2		=	0x20,
kDMAE		=	0x40,
kSFT		=	0x80,

/* Bits in the HostSTR (Host Status) register */
kTRDY		=	0x01,
kHTRQ		=	0x02,
kHRRQ		=	0x04,

// Record & Playback bits in HF bits in HostSTR register
// When an interrupt comes from the card, check the DSP status register. Mask out the HF bits with kHFMask,
// and read the remainder - it will be either 0x20 (Playback request for data to host)
// or 0x10 (Record request for data from DSP). We have more bits to play with if need be.
kHFMask		= 	0x38,
kPlayReq	=	0x20,		// Request for host to supply more "playback" data
kRecReq		= 	0x10,		// Request for host to read "record" data
kHostCmdAck	=	0x08,		// Currently unused	

kHINT		=	0x40,		// Signals the status of the DSP's HINT bit (which controls the HINTA PCIbus signal)
							// If kHINT is set(1), we can assume the DSP has interrupted the host
							// This is used in our interrupt service routine.
/*
   Host Command Vectors - write these values to the Host Command Vector
   (HCVR) to execute a Host Command on the DSP
   Each value includes bit 7=1, which sets the HC bit
*/

kHstCmdReset = 0x73,				//Resets DSP - to make this non-maskable, make it 0x8073
kHstCmdXmitEnable = 0x75,			//Enables playback from Host to DSP - initiates interrupts
kHstCmdXmitDisable = 0x77,			//Disables playback
kHstCmdRecvEnable = 0x79,			//Enables recording from DSP to Host - initiates ints
kHstCmdRecvDisable = 0x7b,
kHstCmdXmt32 = 0x7d,				//Xmt at 32k
kHstCmdXmt44 = 0x7f,
kHstCmdXmt48 = 0x81,
kHstCmdRcv32 = 0x83,				//Rcv at 32k
kHstCmdRcv44 = 0x85,
kHstCmdRcv48 = 0x87,
kHstCmdXmtAES = 0x89,				//AES output
kHstCmdXmtSPDIF = 0x8b,				//SPDIF output
kHstCmdPlaythruEnable = 0x8d,		//Enables h/w level monitoring
kHstCmdPlaythruDisable = 0x8f,
kHstIntAckPlay = 0x91,			   	//Used to acknowledge play interrupts - this turns off the int from the DSP
kHstIntAckRec = 0x93,				// Used to acknowledge record ints - or synchonrous r/w ints
kHstHFClear = 0x95,					//Clears HF bits manually
kHstCmdRcvStatus = 0x97,			//Unused
kHstCmdGetInputMeterLeftLSB = 0x99,	
kHstCmdGetInputMeterLeftMSB = 0x9b,
kHstCmdGetInputMeterRightLSB = 0x9d,
kHstCmdGetInputMeterRightMSB = 0x9f,
kHstCmdSynch = 0xa1,				// Enables synchronous I/O (read/write synchronous with DSP)
kHstCmdAsynch = 0xa3,
kHstCmdBufferSoundMan = 0xa5,
kHstCmdBufferC00 = 0xa7,			 			
kHstCmdBuffer1000 = 0xa9,
kHstCmdBufferDeck = 0xab,
kHstCmdBuffer100 = 0xad,
kHstCmdBuffer200 = 0xaf,
kHstCmdBuffer400 = 0xb1,
kHstCmdBuffer800 = 0xb3,
kHstCmdAutoFormat = 0xb5,
kHstCmdXmtPause = 0xb7,
kHstCmdRcvPause = 0xb9,
kHstCmdPlayVolume = 0xbb,
kHstCmdRecordVolume = 0xbd
};

// Allow for different sized input & output buffers...at this stage, we use double buffering only
// Currently the DSP is set up for these buffer sizes - this is optimized for the MacOS Sound Manager.
// Originally the buffer sizes were both 0x200, but due to problems with asynchronous recording with the
// Mac Sound & Device Managers, the record buffer was increased to utilize all available external RAM on
// the PCI24. For the Win95 driver, we can change these buffer sizes if necessary (a small buffer size is
// preferred to minimize latency through the system).

// Note : the driver needs to call a host command to set the DSP buffer to this length with kHstCmdBuffer1000

#define kInputBufferLen					0x00000200			// Input buffer size in 16-bit words
#define kOutputBufferLen				0x00000200			// Output buffer size in 16-bit words

#endif /* _LUCID_PCI24_H */
