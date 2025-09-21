// monkey.h - header file used by all project files

#include <ByteOrder.h>

/*------------------------------------------------------------------------

	Debug defines
	
------------------------------------------------------------------------*/

// Comment out this line for a release build
//#define DEBUG 1

#ifdef DEBUG

#define DEBUG_DETAIL_LEVEL	2

#if DEBUG_DETAIL_LEVEL>=1
#define DPF1(strings) 	dprintf##strings;
#else 
#define DPF1(strings)
#endif

#if DEBUG_DETAIL_LEVEL>=2
#define DPF2(strings) 	dprintf##strings;
#else
#define DPF2(strings)
#endif

#if DEBUG_DETAIL_LEVEL>=3
#define DPF3(strings) 	dprintf##strings;
#else
#define DPF3(strings)
#endif

#define DPF_ON			set_dprintf_enabled(true);
#define DPF_OFF			set_dprintf_enabled(false);

#else

#define DPF1(strings)
#define DPF2(strings)
#define DPF3(strings)
#define DPF_ON
#define DPF_OFF

#endif

// At some point this should do something Be-like; for now, let Windows code
// build
#define DEBUGBREAK


/*------------------------------------------------------------------------

	Defines for the audio buffers

------------------------------------------------------------------------*/

#define DEFAULT_SINGLE_BUFFER_SIZE_SAMPLES 	128
#define BYTES_PER_SAMPLE					4

/*------------------------------------------------------------------------

	Defines for MIDI buffers

------------------------------------------------------------------------*/

#define MIDI_INPUT_BUFFER_SIZE		1024


/*------------------------------------------------------------------------

	Defines for different card types

------------------------------------------------------------------------*/

#define DARLA									0x0010
#define GINA									0x0020
#define LAYLA									0x0030
#define DARLA24									0x0040


/*------------------------------------------------------------------------

	Defines for device names
	
------------------------------------------------------------------------*/

#define MAX_DEVICES		20
#define MAX_DEVICE_NAME	79


/*------------------------------------------------------------------------

	C type defines and macros
	
------------------------------------------------------------------------*/

typedef uint32 DWORD;
typedef uint16 WORD;
typedef uint8 BYTE;
typedef DWORD * PDWORD;
typedef WORD * PWORD;
typedef BYTE * PBYTE;
typedef bool BOOL;

#define PAGE_SIZE_ROUND_UP(x)	((x+B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1))


/*------------------------------------------------------------------------

	Error code mapping
	
------------------------------------------------------------------------*/

#define MMSYSERR_NOERROR	B_OK
#define FAIL				B_ERROR


/*------------------------------------------------------------------------

	DSP commands and defines
	
------------------------------------------------------------------------*/

#define PLAY_FILTER22_TAPS				112
#define PLAY_FILTER11_TAPS				128
#define PLAY_FILTER32_TAPS				153
#define RECORD_FILTER11_TAPS			96
#define RECORD_FILTER22_TAPS			96
#define RECORD_FILTER32_TAPS			192

#define LAYLA_ASIC_ALREADY_LOADED		1
#define LAYLA_ASIC_NOT_LOADED			0

// DSP slave mode commands
#define SET_COMMPAGE_ADDR				0x02
#define SET_CACHELINE_SIZE				0x03
#define LOAD_LAYLA_ASIC					0xd8


// DSP vector commands
#define ACK_INT								0x0073
#define START_TRANSFER					0x0075
#define CONFIRM_XFER_STATE				0x0077 
#define METERS_ON							0x0079
#define METERS_OFF							0x007b
#define UPDATE_OUTGAIN					0x007d
#define UPDATE_INGAIN						0x007f
#define ADDBUFF_WAVEOUT0					0x0081
#define UNBLOCK_MIDI						0x00ed	// Unblock MIDI interrupt
#define UPDATE_CLOCKS						0x00ef
#define SET_LAYLA_SAMPLE_RATE			0x00f1
#define SET_GD_AUDIO_STATE				0x00f1
#define MIDI_IN_START						0x00f3
#define MIDI_WRITE							0x00f5
#define STOP_TRANSFER						0x00f7
#define RESET_PEAK_METERS				0x00f9
#define UNBLOCK_IRQ						0x00fb	// Unblock audio interrupt
#define UPDATE_FLAGS						0x00fd

#define VECTOR_TIMEOUT					40		// Units of microseconds



/*------------------------------------------------------------------------

	Data structure that describes a duck entry
	
------------------------------------------------------------------------*/

typedef struct
{
	DWORD	dwPhysicalAddress;
	DWORD	dwLength;
} PLE, *PPLE; 

#define DUCK_SIZE_ALLOC	64


/*------------------------------------------------------------------------

	Data structure that describes a single card
	
------------------------------------------------------------------------*/

#define MAX_CHANNELS	32

typedef struct tagJackCount 
{
	int32	NumAnalogOut;
	int32	NumDigitalOut;
	int32	NumAnalogIn;
	int32	NumDigitalIn;
	int32	NumOut;
	int32	NumIn;
	int32	NumJacks;
	int32	AnalogConnector;
} JACKCOUNT;

typedef struct tagMA_CLIENT
{
	struct tagMONKEYINFO 	*pMI;
	bool						fFullAccess;
	struct tagMA_CLIENT	*pNext;
} MA_CLIENT, *PMA_CLIENT;


typedef struct tagMIDI_CLIENT
{
	struct tagMONKEYINFO 	*pMI;
	uint8						*pBufferBase;
	uint8						*pBufferEnd;
	uint8						*pBufferRead;
	uint8						*pBufferWrite;
	sem_id					DataSem;
	struct tagMIDI_CLIENT	*pNext;
} MIDI_CLIENT, *PMIDI_CLIENT;


typedef struct tagMONKEYINFO
{
	area_id 	CommPageAreaID;
	area_id 	HardwareRegsAreaID;
	area_id	OutAudioBufferAreaID;
	area_id	InAudioBufferAreaID;	
	area_id	DuckAreaID;
	
	sem_id	ControlSem;
	sem_id	IrqSem;

	char		szCardBaseName[MAX_DEVICE_NAME+1];
	char		szCardDeviceName[MAX_DEVICE_NAME+1];
	char		szMidiDeviceName[MAX_DEVICE_NAME+1];
	char		szFriendlyName[32];
	WORD		wCardType;
	WORD		wCardRev;
	DWORD		dwMonkeyPhysAddr;
	DWORD*	pMonkeyBase;
	void *	 	pCommPage;
	DWORD		dwCommPagePhys;

	BYTE		bASICLoaded;
	BYTE		bIRQ;
	DWORD		dwFlags;
	
	int32		*pAudio[MAX_CHANNELS][2];
	PPLE		pDuck[MAX_CHANNELS];
	DWORD		dwSingleBufferSizeSamples;
	
	JACKCOUNT	*jc;
	int32		*node_type_counts;
	int32		*channel_map;
	
	DWORD		dwSampleRate;
	BYTE		bGDCurrentClockState;
	BYTE		bGDCurrentSpdifStatus;
	BYTE		bGDCurrentResamplerState;
	BYTE		bDummy;
	WORD		wInputClock;
	WORD		wOutputClock;
	
	DWORD		dwEnabledChannels;
	DWORD		dwActualBufferCount;
	DWORD		dwActiveChannels;
	
	DWORD		dwSampleTimestamp;
	DWORD		dwLastSampleTimestamp;
	bigtime_t	qwSampleTimestamp;
	bigtime_t	qwSystemTimestamp;
	DWORD		dwMbxFlags;
	DWORD		dwShiftFactor;
	
	vlong		dwHardwareSpinlock;

	PMA_CLIENT		ma_clients;
	PMIDI_CLIENT 	midi_clients;
	DWORD		dwNumMidiClients;	
	
	struct tagCardType	*ct;

	struct tagMONKEYINFO* 	pNext;	
} MONKEYINFO;

typedef MONKEYINFO * PMONKEYINFO;

#define PMI_FLAG_BADBOARD		0x0002
#define PMI_FLAG_ASIC_WAIT		0x0004

#define CHANNEL_SPDIF				0x0001
#define CHANNEL_HASGAINCTRL		0x0002

#define JC						pMI->jc


typedef struct tagCardType
{
	bool				(*CheckSampleRate)(uint32 dwSampleRate);
	status_t			(*SetSampleRate)(PMONKEYINFO pMI,uint32 dwSampleRate);
	status_t			(*SetInputClock)(PMONKEYINFO pMI,uint16 wInputClock);
	status_t			(*SetOutputClock)(PMONKEYINFO pMI,uint16 wOutputClock);	
	status_t			(*SetNominalOutLevel)(PMONKEYINFO pMI,int32 iChannel,int32 iLevel);
	status_t			(*SetNominalInLevel)(PMONKEYINFO pMI,int32 iChannel,int32 iLevel);	
	status_t			(*GetNominalOutLevel)(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);
	status_t			(*GetNominalInLevel)(PMONKEYINFO pMI,int32 iChannel,int32 *piLevel);	
	interrupt_handler	InterruptHandler;
	JACKCOUNT		*jc;
	int32				*node_type_counts;
	int32				*channel_map;
	char				*szName;
	uint16			*pwDspCode;	
	uint32			dwNumMidiPorts;
	uint32			dwSampleRates;
	float				flMinCvsrRate;
	float 				flMaxCvsrRate;
	uint32			dwLockSources;
	uint32			dwTimecodeSources;
} CARDTYPE, *PCARDTYPE;

#define SAMPLE_RATE_NO_CHANGE	0xffffffff

#define CT							pMI->ct

