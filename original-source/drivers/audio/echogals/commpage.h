#define	MONITOR_ARRAY_SIZE	0x180
#define MAX_PLAY_TAPS		168
#define MAX_REC_TAPS		192

#ifndef NUMWDEVICES
#define NUMWDEVICES			32
#endif

//
//	Hi32 Extended register flags
//
#define	HI32_EXT_AUDIOIRQ		1					// Interrupt from audio channel
#define	HI32_EXT_MIDIIRQ		2					// Interrupt from MIDI channel

typedef struct
{
	DWORD		dwCommSize;						// 0x000		4
	DWORD		dwFlags;						// 0x004		4
	DWORD		dwPosition;						// 0x008		4
	DWORD		dwSampleRate;					// 0x00c		4
	DWORD		dwStart;						// 0x010		4
	DWORD		dwStop;							// 0x014		4
	DWORD		dwReset;						// 0x018		4
	DWORD		dwDone;							// 0x01c  		4
	DWORD		dwDuckListPhys[NUMWDEVICES];	// 0x020		4*32 0x80
	DWORD		dwHi32Extended;					// 0x0A0		4*1 Hi32 extended flags
	WORD		wBuffersConsumed[NUMWDEVICES];	// 0x0A4		2*32 0x40
	WORD		wDummy1[NUMWDEVICES-2];			// 0x0E4		2*30 0x3c
	BYTE		VULevel[NUMWDEVICES*2];			// 0x120		1*64 0x40
	BYTE		Dummy2[NUMWDEVICES*2];			// 0x160		1*64 0x40
	BYTE		PeakMeter[NUMWDEVICES*2];		// 0x1a0		1*64 0x40
	BYTE		Dummy3[NUMWDEVICES*2];			// 0x1e0		1*64 0x40
	BYTE		LineLevel[NUMWDEVICES*2];		// 0x220		1*64 0x40
	BYTE		Dummy4[NUMWDEVICES*2];			// 0x260		1*64 0x40
	BYTE		Monitors[MONITOR_ARRAY_SIZE];	// 0x2a0		0x180
	BYTE		Dummy5[MONITOR_ARRAY_SIZE];  	// 0x420		0x180
	DWORD		dwAudioFormat[NUMWDEVICES];		// 0x5a0		4*32 0x80
	DWORD		dwDSPError[NUMWDEVICES];		// 0x620		4*32 0x80
	DWORD		dwMixFlags[NUMWDEVICES];		// 0x6a0		4*32 0x80
	DWORD		dwFillCount[NUMWDEVICES];		// 0x720		4*32 0x80
	BYTE		bGDClockState;					// 0x7a0		1
	BYTE		bGDSpdifStatus;					// 0x7a1		1
	BYTE		bGDResamplerState;				// 0x7a2		1
	BYTE		bDummy6;						// 0x7a3		1
	DWORD		dwOverMeter;					// 0x7a4		4
	DWORD		dwTimeoutCount;					// 0x7a8		4
	DWORD		dwLastFail;						// 0x7ac		4
	DWORD		dwNominalLevels; 				// 0x7b0    4*1	0x04
	WORD		wInputClock; 					// 0x7b4    2*1	0x02
	WORD		wOutputClock; 					// 0x7b6    2*1	0x02
	DWORD		dwStatusClocks; 				// 0x7b8    2*1	0x08
	DWORD		dwHandshake; 					// 0x7bc    4*1	0x04

	DWORD		dwPlayCoeff[MAX_PLAY_TAPS]; 	// 0x7c0		4*168 0x2a0
	DWORD		dwRecCoeff[MAX_REC_TAPS];		// 0xa60		4*192	0x300
	DWORD		dwMIDIInTimestamp;				// 0xd60		4
	DWORD		dwMIDIInData;					// 0xd64		4
	DWORD		dwMIDIOutData;					// 0xd68		4
	DWORD 		dwMIDIInStatus; 				// 0xd6c    	4		make this one go away!!!!
	DWORD		dwSerialNo[5];					// 0xd70		4*5 	0x14 
	
	// End of DSP caring
	BYTE			MonitorCopy[MONITOR_ARRAY_SIZE];
} COMMPAGE, *PCOMMPAGE ;

#define CP_SIZE_IN_PAGES	PAGE_SIZE_ROUND_UP(sizeof(COMMPAGE))

// dwFlags flags
#define CPFLAG_MIDI_INPUT	0x0001
#define	CPFLAG_SPDIFPRO		0x0008 	// 1 Professional, 0 Consumer

// Codes for input clock
#define	CLK_CLOCKININTERNAL		0
#define	CLK_CLOCKINSPDIF		1
#define	CLK_CLOCKINWORD			2
#define	CLK_CLOCKINSUPER		3
#define	CLK_CLOCKINMIDI			4

// Bits for input clock detection
#define	CLKFLAG_CLOCKWORDAVAIL	0x0002
#define	CLKFLAG_CLOCKSUPERAVAIL	0x0004
#define	CLKFLAG_CLOCKSPDIFAVAIL	0x0008

// Codes for output clock
#define	CLK_CLOCKOUTSUPER		0
#define	CLK_CLOCKOUTWORD		1

// Nominal levels
#define MINUS10	1
#define	PLUS4	0

//
//	Audio formats used by DSP in Low Latency Mode
//
#define AUDIOFORM_8				0		// 8 bit
#define AUDIOFORM_16LE			1		// 16 bit little endian
#define AUDIOFORM_16BE			2		// 16 bit big endian
#define AUDIOFORM_24LE			3		// 24 bit little endian
#define AUDIOFORM_24BE			4		// 24 bit big endian
#define AUDIOFORM_32LE			5		// 32 bit little endian
#define AUDIOFORM_32BE			6		// 32 bit big endian
#define AUDIOFORM_32FLE			7		// 32 bit float little endian
#define AUDIOFORM_32FBE			8		// 32 bit float big endian

#define GD_CLOCK_NOCHANGE		0
#define GD_CLOCK_44				1
#define GD_CLOCK_48				2
#define GD_CLOCK_SPDIFIN		3
#define GD_CLOCK_UNDEF			0xff

#define GD_RESAMPLE_11			0
#define GD_RESAMPLE_22			1
#define GD_RESAMPLE_32			2
#define GD_RESAMPLE_SAME		3
#define GD_RESAMPLE_UNDEF		0xff

#define GD_SPDIF_STATUS_NOCHANGE	0
#define GD_SPDIF_STATUS_44			1
#define GD_SPDIF_STATUS_48			2
#define GD_SPDIF_STATUS_32			3
#define GD_SPDIF_STATUS_UNDEF		0xff

//
//	Return values from the DSP when Layla is loaded
//

#define LAYLA_ASIC_ALREADY_LOADED	1
#define LAYLA_ASIC_NOT_LOADED		0

/*

Note that Monitors is a two dimensional array whose dimesions depend on the
card.  For Layla:

In1->Out1
In2->Out1
In3->Out1
In4->Out1
In5->Out1
...
In10->Out1
In1->Out2
etc.

So, to get the correct index for an input output pair, do

(dwNumInputs * output) + input

where both input and output start at zero.
 
*/

// **** End Of CommPage.h ****
