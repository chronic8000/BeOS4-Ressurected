

static const CtryPrmsStruct g_Country_Parms[] =
{
#ifdef 0 //only USA for now
//Japan

{
/*	UINT16							T35Code;							*/
/*	char							cInter[MAX_OEM_STR_LEN];			*/
/*	char							cIntCode[MAX_OEM_STR_LEN];			*/
/*	CntryTxlevelStructure			Txlevel;							*/
/*	UINT16							Relays[LAST_RELAY_ENTRY];			*/
/*	CntryPulseStructure				Pulse;								*/
/*	RING_PARAMS						Ring;								*/
/*	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	*/
/*	CntryDTMFStructure				DTMF;								*/
/*	CntryFilterStructure			Filter;								*/
/*	CntryThresholdStructure			Threshold;							*/
/*	CntryRLSDStructure				RLSD;								*/
/*	CntryToneStructure				Tone;								*/
/*	CALL_PROGRESS_TIMING_CONFIG		Timing;								*/
/*	CntryCadenceStructure			Cadence;							*/
/*	CntryBlacklistingStructure		Blacklisting;						*/
/*	CntryCallerIDStructure			CallerID;							*/
/*	CALL_PROGRESS_FLAGS				Flags;								*/
/*	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					*/
#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x00, "Country Parameters - 00", preload | purgeable)
{
    0x00,
    "JAPAN",
    "001"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x00, "Country Tx level - 00", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x4D,
    0x38,
    0x0A
};

//	Relays
resource 'CRLS' (0x00, "Country Relays - 00", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x00, "Country Pulse Dial Parameters - 00", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x00, "Country Ring Parameters - 00", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x00, "Country S Register Limiting - 00", preload | purgeable)
{
      6,   0,  15,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  60, 255,  85,
     27,   0, 255,  73,
     29,   5, 100,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x00, "Country DTMF Parameters - 00", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x00, "Country Filter Parameters - 00", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x00, "Country Threshold Structure - 00", preload | purgeable)
{
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x00, "Country RLSD - 00", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x00, "Country Tone - 00", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x00, "Country Timing - 00", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x00, "Country Cadence - 00", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x00, "Country Blacklisting - 00", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x00, "Country Caller ID - 00", preload | purgeable)
{
    0x02
};

resource 'CCPF' (0x00, "Country Call Progress Flags - 00", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x00, "CntryAgressSpeedIndexStructure - 00", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};


},

// Canada

{
/*	UINT16							T35Code;							*/
/*	char							cInter[MAX_OEM_STR_LEN];			*/
/*	char							cIntCode[MAX_OEM_STR_LEN];			*/
/*	CntryTxlevelStructure			Txlevel;							*/
/*	UINT16							Relays[LAST_RELAY_ENTRY];			*/
/*	CntryPulseStructure				Pulse;								*/
/*	RING_PARAMS						Ring;								*/
/*	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	*/
/*	CntryDTMFStructure				DTMF;								*/
/*	CntryFilterStructure			Filter;								*/
/*	CntryThresholdStructure			Threshold;							*/
/*	CntryRLSDStructure				RLSD;								*/
/*	CntryToneStructure				Tone;								*/
/*	CALL_PROGRESS_TIMING_CONFIG		Timing;								*/
/*	CntryCadenceStructure			Cadence;							*/
/*	CntryBlacklistingStructure		Blacklisting;						*/
/*	CntryCallerIDStructure			CallerID;							*/
/*	CALL_PROGRESS_FLAGS				Flags;								*/
/*	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					*/
#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x20, "Country Parameters - 20", preload | purgeable)
{
    0x20,
    "CANADA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x20, "Country Tx level - 20", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x28,
    0x14,
    0x04
};

//	Relays
resource 'CRLS' (0x20, "Country Relays - 20", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x20, "Country Pulse Dial Parameters - 20", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x20, "Country Ring Parameters - 20", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x20, "Country S Register Limiting - 20", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,  70,  70,  70,
    255,   0,   0,   0 
};

resource 'CDTM' (0x20, "Country DTMF Parameters - 20", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x20, "Country Filter Parameters - 20", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x20, "Country Threshold Structure - 20", preload | purgeable)
{
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x20, "Country RLSD - 20", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x20, "Country Tone - 20", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x20, "Country Timing - 20", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x20, "Country Cadence - 20", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x20, "Country Blacklisting - 20", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x20, "Country Caller ID - 20", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x20, "Country Call Progress Flags - 20", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x20, "CntryAgressSpeedIndexStructure - 20", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};


},


// Mexico

{

/*	UINT16							T35Code;							*/
/*	char							cInter[MAX_OEM_STR_LEN];			*/
/*	char							cIntCode[MAX_OEM_STR_LEN];			*/
/*	CntryTxlevelStructure			Txlevel;							*/
/*	UINT16							Relays[LAST_RELAY_ENTRY];			*/
/*	CntryPulseStructure				Pulse;								*/
/*	RING_PARAMS						Ring;								*/
/*	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	*/
/*	CntryDTMFStructure				DTMF;								*/
/*	CntryFilterStructure			Filter;								*/
/*	CntryThresholdStructure			Threshold;							*/
/*	CntryRLSDStructure				RLSD;								*/
/*	CntryToneStructure				Tone;								*/
/*	CALL_PROGRESS_TIMING_CONFIG		Timing;								*/
/*	CntryCadenceStructure			Cadence;							*/
/*	CntryBlacklistingStructure		Blacklisting;						*/
/*	CntryCallerIDStructure			CallerID;							*/
/*	CALL_PROGRESS_FLAGS				Flags;								*/
/*	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					*/
#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x73, "Country Parameters - 73", preload | purgeable)
{
    0x73,
    "MEXICO",
    "98"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x73, "Country Tx level - 73", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x73, "Country Relays - 73", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x73, "Country Pulse Dial Parameters - 73", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0xFFFFFFFE,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x73, "Country Ring Parameters - 73", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x73, "Country S Register Limiting - 73", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  10 
};

resource 'CDTM' (0x73, "Country DTMF Parameters - 73", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x73, "Country Filter Parameters - 73", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x73, "Country Threshold Structure - 73", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x73, "Country RLSD - 73", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x73, "Country Tone - 73", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x73, "Country Timing - 73", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x73, "Country Cadence - 73", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       150,    300,    150,    300,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x73, "Country Blacklisting - 73", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x73, "Country Caller ID - 73", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x73, "Country Call Progress Flags - 73", preload | purgeable)
{
    0x00,
    0x40,
    0x32,
    0x00 
};

resource 'CASI' (0x73, "CntryAgressSpeedIndexStructure - 73", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};


},

// Taiwan

{
/*	UINT16							T35Code;							*/
/*	char							cInter[MAX_OEM_STR_LEN];			*/
/*	char							cIntCode[MAX_OEM_STR_LEN];			*/
/*	CntryTxlevelStructure			Txlevel;							*/
/*	UINT16							Relays[LAST_RELAY_ENTRY];			*/
/*	CntryPulseStructure				Pulse;								*/
/*	RING_PARAMS						Ring;								*/
/*	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	*/
/*	CntryDTMFStructure				DTMF;								*/
/*	CntryFilterStructure			Filter;								*/
/*	CntryThresholdStructure			Threshold;							*/
/*	CntryRLSDStructure				RLSD;								*/
/*	CntryToneStructure				Tone;								*/
/*	CALL_PROGRESS_TIMING_CONFIG		Timing;								*/
/*	CntryCadenceStructure			Cadence;							*/
/*	CntryBlacklistingStructure		Blacklisting;						*/
/*	CntryCallerIDStructure			CallerID;							*/
/*	CALL_PROGRESS_FLAGS				Flags;								*/
/*	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					*/
#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xFE, "Country Parameters - fe", preload | purgeable)
{
    0xFE,
    "TAIWAN",
    "886"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xFE, "Country Tx level - fe", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x28,
    0x14,
    0x04
};

//	Relays
resource 'CRLS' (0xFE, "Country Relays - fe", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xFE, "Country Pulse Dial Parameters - fe", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xFE, "Country Ring Parameters - fe", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xFE, "Country S Register Limiting - fe", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  10 
};

resource 'CDTM' (0xFE, "Country DTMF Parameters - fe", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xFE, "Country Filter Parameters - fe", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xFE, "Country Threshold Structure - fe", preload | purgeable)
{
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xFE, "Country RLSD - fe", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xFE, "Country Tone - fe", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xFE, "Country Timing - fe", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xFE, "Country Cadence - fe", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xFE, "Country Blacklisting - fe", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0xFE, "Country Caller ID - fe", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xFE, "Country Call Progress Flags - fe", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0xFE, "CntryAgressSpeedIndexStructure - fe", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};


},
#endif
// United Stated

{
/*	UINT16							T35Code;							*/
    0xB5,
/*	char							cInter[MAX_OEM_STR_LEN];			*/
    "USA",
/*	char							cIntCode[MAX_OEM_STR_LEN];			*/
    "011",
/*	CntryTxlevelStructure			Txlevel;							*/
    0x0F, 0x0A, 0x0A, 0x0F, 0x0A, 0x0A, 0x00, 0x0F, 0x08, 0x00, 0x28, 0x14, 0x04,
/*	UINT16							Relays[LAST_RELAY_ENTRY];			*/
//    0x0800, 0x0000, 0x0800, 0x0800, 0x0000, 0x0000,
//    0x0000, 0x0000, 0x0800, 0x0000, 0x0800, 0x0800,
// The other above Relay values are for USB
	0x8007,	0x8007,	0x8004,	0x8000,	0x8005,	0x8007,
	0x8001,	0x8003,	0x8000,	0x8003,	0x8000,	0x8000,
/*	CntryPulseStructure				Pulse;								*/
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000,
/*	RING_PARAMS						Ring;								*/
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000, 
/*	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	*/
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 ,

/*	CntryDTMFStructure				DTMF;								*/
    0x005C,
    0x0048,
/*	CntryFilterStructure			Filter;								*/
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0100, 
    0x0200, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0100, 
    0x0200, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
/*	CntryThresholdStructure			Threshold;							*/
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000,
/*	CntryRLSDStructure				RLSD;								*/
    0x00000000,
/*	CntryToneStructure				Tone;								*/
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000,
/*	CALL_PROGRESS_TIMING_CONFIG		Timing;								*/
    0x0000,
    0x0000,
    0x0000,
    0x0000,
/*	CntryCadenceStructure			Cadence;							*/
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    650,    420,    650,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       160,    280,    160,    280,      0,      0,      0,      0,      6,
/*	CntryBlacklistingStructure		Blacklisting;						*/
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00,
/*	CntryCallerIDStructure			CallerID;							*/
    0x01,
/*	CALL_PROGRESS_FLAGS				Flags;								*/
    0x00,
    0x00,
    0x32,
    0x00,
/*	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					*/
    0x64,
    0x64,
    0x64
}
};

#ifdef 0 // more countires to be added

// CtryPrmsStruct
resource 'CNTR' (0x04, "Country Parameters - 04", preload | purgeable)
{
    0x04,
    "GERMANY",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x04, "Country Tx level - 04", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x31,
    0x1C,
    0x04
};

//	Relays
resource 'CRLS' (0x04, "Country Relays - 04", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x04, "Country Pulse Dial Parameters - 04", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0xFFFFFFFE,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x04, "Country Ring Parameters - 04", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x04, "Country S Register Limiting - 04", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x04, "Country DTMF Parameters - 04", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x04, "Country Filter Parameters - 04", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x04, "Country Threshold Structure - 04", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x04, "Country RLSD - 04", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x04, "Country Tone - 04", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x04, "Country Timing - 04", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x04, "Country Cadence - 04", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
        80,    600,    300,    650,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
        80,    300,    150,    650,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x04, "Country Blacklisting - 04", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x04, "Country Caller ID - 04", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x04, "Country Call Progress Flags - 04", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x40 
};

resource 'CASI' (0x04, "CntryAgressSpeedIndexStructure - 04", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x07, "Country Parameters - 07", preload | purgeable)
{
    0x07,
    "ARGENTINA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x07, "Country Tx level - 07", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x07, "Country Relays - 07", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x07, "Country Pulse Dial Parameters - 07", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x07, "Country Ring Parameters - 07", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x07, "Country S Register Limiting - 07", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x07, "Country DTMF Parameters - 07", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x07, "Country Filter Parameters - 07", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x07, "Country Threshold Structure - 07", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x07, "Country RLSD - 07", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x07, "Country Tone - 07", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x07, "Country Timing - 07", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x07, "Country Cadence - 07", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x07, "Country Blacklisting - 07", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x07, "Country Caller ID - 07", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x07, "Country Call Progress Flags - 07", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x07, "CntryAgressSpeedIndexStructure - 07", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x09, "Country Parameters - 09", preload | purgeable)
{
    0x09,
    "AUSTRALIA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x09, "Country Tx level - 09", preload | purgeable)
{
    0x0F,
    0x00,
    0x0B,
    0x0F,
    0x00,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x45,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x09, "Country Relays - 09", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x09, "Country Pulse Dial Parameters - 09", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x09, "Country Ring Parameters - 09", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x09, "Country S Register Limiting - 09", preload | purgeable)
{
      0,   2,   5,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  25,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x09, "Country DTMF Parameters - 09", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x09, "Country Filter Parameters - 09", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x09, "Country Threshold Structure - 09", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x09, "Country RLSD - 09", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x09, "Country Tone - 09", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x09, "Country Timing - 09", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x09, "Country Cadence - 09", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       350,    450,    150,    250,    350,    450,   1800,   2200,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x09, "Country Blacklisting - 09", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x0005,
    0x0005,
    0x0708,
    0x0000,
    0x000A,
    0x0000,
    0x80 
};

resource 'CCID' (0x09, "Country Caller ID - 09", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x09, "Country Call Progress Flags - 09", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x40 
};

resource 'CASI' (0x09, "CntryAgressSpeedIndexStructure - 09", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x0A, "Country Parameters - 0a", preload | purgeable)
{
    0x0A,
    "AUSTRIA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x0A, "Country Tx level - 0a", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x0A, "Country Relays - 0a", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x0A, "Country Pulse Dial Parameters - 0a", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x0A, "Country Ring Parameters - 0a", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x0A, "Country S Register Limiting - 0a", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x0A, "Country DTMF Parameters - 0a", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x0A, "Country Filter Parameters - 0a", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x0A, "Country Threshold Structure - 0a", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x0A, "Country RLSD - 0a", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x0A, "Country Tone - 0a", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x0A, "Country Timing - 0a", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x0A, "Country Cadence - 0a", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       250,    625,    250,    625,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       125,    300,    125,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x0A, "Country Blacklisting - 0a", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x0A, "Country Caller ID - 0a", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x0A, "Country Call Progress Flags - 0a", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x0A, "CntryAgressSpeedIndexStructure - 0a", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x0F, "Country Parameters - 0f", preload | purgeable)
{
    0x0F,
    "BELGIUM",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x0F, "Country Tx level - 0f", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x0F, "Country Relays - 0f", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x0F, "Country Pulse Dial Parameters - 0f", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x0F, "Country Ring Parameters - 0f", preload | purgeable)
{
    0x3C,
    0x13,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x0F, "Country S Register Limiting - 0f", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  12 
};

resource 'CDTM' (0x0F, "Country DTMF Parameters - 0f", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x0F, "Country Filter Parameters - 0f", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x0F, "Country Threshold Structure - 0f", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x0F, "Country RLSD - 0f", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x0F, "Country Tone - 0f", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x0F, "Country Timing - 0f", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x0F, "Country Cadence - 0f", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       149,    185,    149,    185,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x0F, "Country Blacklisting - 0f", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x0F, "Country Caller ID - 0f", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x0F, "Country Call Progress Flags - 0f", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x0F, "CntryAgressSpeedIndexStructure - 0f", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x14, "Country Parameters - 14", preload | purgeable)
{
    0x14,
    "BOLIVIA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x14, "Country Tx level - 14", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x14, "Country Relays - 14", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x14, "Country Pulse Dial Parameters - 14", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x14, "Country Ring Parameters - 14", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x14, "Country S Register Limiting - 14", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x14, "Country DTMF Parameters - 14", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x14, "Country Filter Parameters - 14", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x14, "Country Threshold Structure - 14", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x14, "Country RLSD - 14", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x14, "Country Tone - 14", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x14, "Country Timing - 14", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x14, "Country Cadence - 14", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x14, "Country Blacklisting - 14", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x14, "Country Caller ID - 14", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x14, "Country Call Progress Flags - 14", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x14, "CntryAgressSpeedIndexStructure - 14", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x16, "Country Parameters - 16", preload | purgeable)
{
    0x16,
    "BRAZIL",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x16, "Country Tx level - 16", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x16, "Country Relays - 16", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x16, "Country Pulse Dial Parameters - 16", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x16, "Country Ring Parameters - 16", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x16, "Country S Register Limiting - 16", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x16, "Country DTMF Parameters - 16", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x16, "Country Filter Parameters - 16", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x16, "Country Threshold Structure - 16", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x16, "Country RLSD - 16", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x16, "Country Tone - 16", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x16, "Country Timing - 16", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x16, "Country Cadence - 16", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x16, "Country Blacklisting - 16", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x16, "Country Caller ID - 16", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x16, "Country Call Progress Flags - 16", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x16, "CntryAgressSpeedIndexStructure - 16", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x1B, "Country Parameters - 1b", preload | purgeable)
{
    0x1B,
    "BULGARIA",
    "359"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x1B, "Country Tx level - 1b", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x5F,
    0x44,
    0x04
};

//	Relays
resource 'CRLS' (0x1B, "Country Relays - 1b", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x1B, "Country Pulse Dial Parameters - 1b", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0320,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x1B, "Country Ring Parameters - 1b", preload | purgeable)
{
    0x46,
    0x0F,
    0x00BE,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x1B, "Country S Register Limiting - 1b", preload | purgeable)
{
      6,   4, 255,   4,
      7,   1, 255, 180,
     10,  20, 254,  20,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x1B, "Country DTMF Parameters - 1b", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x1B, "Country Filter Parameters - 1b", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x1B, "Country Threshold Structure - 1b", preload | purgeable)
{
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x1B, "Country RLSD - 1b", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x1B, "Country Tone - 1b", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x1B, "Country Timing - 1b", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x1B, "Country Cadence - 1b", preload | purgeable)
{
         2,
       170,    300,    170,    300,    520,    700,    820,   1180,      2,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       160,    650,    160,    720,      0,      0,      0,      0,      6,
       800,   2000,   2900,   5000,      0,      0,      0,      0,      2,
       240,    360,    240,    360,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x1B, "Country Blacklisting - 1b", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x0005,
    0x0005,
    0xFFFF,
    0x0000,
    0x000F,
    0x0000,
    0xC0 
};

resource 'CCID' (0x1B, "Country Caller ID - 1b", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x1B, "Country Call Progress Flags - 1b", preload | purgeable)
{
    0x00,
    0x00,
    0x3A,
    0x00 
};

resource 'CASI' (0x1B, "CntryAgressSpeedIndexStructure - 1b", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x25, "Country Parameters - 25", preload | purgeable)
{
    0x25,
    "CHILE",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x25, "Country Tx level - 25", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x25, "Country Relays - 25", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x25, "Country Pulse Dial Parameters - 25", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x25, "Country Ring Parameters - 25", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x25, "Country S Register Limiting - 25", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x25, "Country DTMF Parameters - 25", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x25, "Country Filter Parameters - 25", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x25, "Country Threshold Structure - 25", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x25, "Country RLSD - 25", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x25, "Country Tone - 25", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x25, "Country Timing - 25", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x25, "Country Cadence - 25", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x25, "Country Blacklisting - 25", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x25, "Country Caller ID - 25", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x25, "Country Call Progress Flags - 25", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x25, "CntryAgressSpeedIndexStructure - 25", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x26, "Country Parameters - 26", preload | purgeable)
{
    0x26,
    "CHINA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x26, "Country Tx level - 26", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x26, "Country Relays - 26", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x26, "Country Pulse Dial Parameters - 26", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x26, "Country Ring Parameters - 26", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x26, "Country S Register Limiting - 26", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x26, "Country DTMF Parameters - 26", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x26, "Country Filter Parameters - 26", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x26, "Country Threshold Structure - 26", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x26, "Country RLSD - 26", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x26, "Country Tone - 26", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x26, "Country Timing - 26", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x26, "Country Cadence - 26", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    800,    300,    800,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x26, "Country Blacklisting - 26", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x26, "Country Caller ID - 26", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x26, "Country Call Progress Flags - 26", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x26, "CntryAgressSpeedIndexStructure - 26", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x27, "Country Parameters - 27", preload | purgeable)
{
    0x27,
    "COLOMBIA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x27, "Country Tx level - 27", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x27, "Country Relays - 27", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x27, "Country Pulse Dial Parameters - 27", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x27, "Country Ring Parameters - 27", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x27, "Country S Register Limiting - 27", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x27, "Country DTMF Parameters - 27", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x27, "Country Filter Parameters - 27", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x27, "Country Threshold Structure - 27", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x27, "Country RLSD - 27", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x27, "Country Tone - 27", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x27, "Country Timing - 27", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x27, "Country Cadence - 27", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x27, "Country Blacklisting - 27", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x27, "Country Caller ID - 27", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x27, "Country Call Progress Flags - 27", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x27, "CntryAgressSpeedIndexStructure - 27", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x2D, "Country Parameters - 2d", preload | purgeable)
{
    0x2D,
    "CYPRUS",
    "357"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x2D, "Country Tx level - 2d", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x5F,
    0x44,
    0x04
};

//	Relays
resource 'CRLS' (0x2D, "Country Relays - 2d", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x2D, "Country Pulse Dial Parameters - 2d", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0320,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x2D, "Country Ring Parameters - 2d", preload | purgeable)
{
    0x46,
    0x0F,
    0x00BE,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x2D, "Country S Register Limiting - 2d", preload | purgeable)
{
      6,   4, 255,   4,
      7,   1, 255, 180,
     10,  20, 254,  20,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x2D, "Country DTMF Parameters - 2d", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x2D, "Country Filter Parameters - 2d", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x2D, "Country Threshold Structure - 2d", preload | purgeable)
{
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x2D, "Country RLSD - 2d", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x2D, "Country Tone - 2d", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x2D, "Country Timing - 2d", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x2D, "Country Cadence - 2d", preload | purgeable)
{
         2,
       170,    300,    170,    300,    520,    700,    820,   1180,      2,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       160,    650,    160,    720,      0,      0,      0,      0,      6,
       800,   2000,   2900,   5000,      0,      0,      0,      0,      2,
       240,    360,    240,    360,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x2D, "Country Blacklisting - 2d", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x0005,
    0x0005,
    0xFFFF,
    0x0000,
    0x000F,
    0x0000,
    0xC0 
};

resource 'CCID' (0x2D, "Country Caller ID - 2d", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x2D, "Country Call Progress Flags - 2d", preload | purgeable)
{
    0x00,
    0x00,
    0x3A,
    0x00 
};

resource 'CASI' (0x2D, "CntryAgressSpeedIndexStructure - 2d", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x2E, "Country Parameters - 2e", preload | purgeable)
{
    0x2E,
    "CZECH REPUBLIC",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x2E, "Country Tx level - 2e", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x2E, "Country Relays - 2e", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x2E, "Country Pulse Dial Parameters - 2e", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x2E, "Country Ring Parameters - 2e", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x2E, "Country S Register Limiting - 2e", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x2E, "Country DTMF Parameters - 2e", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x2E, "Country Filter Parameters - 2e", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x2E, "Country Threshold Structure - 2e", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x2E, "Country RLSD - 2e", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x2E, "Country Tone - 2e", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x2E, "Country Timing - 2e", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x2E, "Country Cadence - 2e", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x2E, "Country Blacklisting - 2e", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x2E, "Country Caller ID - 2e", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x2E, "Country Call Progress Flags - 2e", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x2E, "CntryAgressSpeedIndexStructure - 2e", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x31, "Country Parameters - 31", preload | purgeable)
{
    0x31,
    "DENMARK",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x31, "Country Tx level - 31", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x31, "Country Relays - 31", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x31, "Country Pulse Dial Parameters - 31", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x31, "Country Ring Parameters - 31", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x31, "Country S Register Limiting - 31", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x31, "Country DTMF Parameters - 31", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x31, "Country Filter Parameters - 31", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x31, "Country Threshold Structure - 31", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x31, "Country RLSD - 31", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x31, "Country Tone - 31", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x31, "Country Timing - 31", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x31, "Country Cadence - 31", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x31, "Country Blacklisting - 31", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x31, "Country Caller ID - 31", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x31, "Country Call Progress Flags - 31", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x31, "CntryAgressSpeedIndexStructure - 31", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x33, "Country Parameters - 33", preload | purgeable)
{
    0x33,
    "DOMINICAN REPUBLIC",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x33, "Country Tx level - 33", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x33, "Country Relays - 33", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x33, "Country Pulse Dial Parameters - 33", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x33, "Country Ring Parameters - 33", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x33, "Country S Register Limiting - 33", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x33, "Country DTMF Parameters - 33", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x33, "Country Filter Parameters - 33", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x33, "Country Threshold Structure - 33", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x33, "Country RLSD - 33", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x33, "Country Tone - 33", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x33, "Country Timing - 33", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x33, "Country Cadence - 33", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x33, "Country Blacklisting - 33", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x33, "Country Caller ID - 33", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x33, "Country Call Progress Flags - 33", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x33, "CntryAgressSpeedIndexStructure - 33", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x35, "Country Parameters - 35", preload | purgeable)
{
    0x35,
    "ECUADOR",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x35, "Country Tx level - 35", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x35, "Country Relays - 35", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x35, "Country Pulse Dial Parameters - 35", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x35, "Country Ring Parameters - 35", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x35, "Country S Register Limiting - 35", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x35, "Country DTMF Parameters - 35", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x35, "Country Filter Parameters - 35", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x35, "Country Threshold Structure - 35", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x35, "Country RLSD - 35", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x35, "Country Tone - 35", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x35, "Country Timing - 35", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x35, "Country Cadence - 35", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x35, "Country Blacklisting - 35", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x35, "Country Caller ID - 35", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x35, "Country Call Progress Flags - 35", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x35, "CntryAgressSpeedIndexStructure - 35", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x37, "Country Parameters - 37", preload | purgeable)
{
    0x37,
    "EL SALVADOR",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x37, "Country Tx level - 37", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x37, "Country Relays - 37", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x37, "Country Pulse Dial Parameters - 37", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x37, "Country Ring Parameters - 37", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x37, "Country S Register Limiting - 37", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x37, "Country DTMF Parameters - 37", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x37, "Country Filter Parameters - 37", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x37, "Country Threshold Structure - 37", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x37, "Country RLSD - 37", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x37, "Country Tone - 37", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x37, "Country Timing - 37", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x37, "Country Cadence - 37", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x37, "Country Blacklisting - 37", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x37, "Country Caller ID - 37", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x37, "Country Call Progress Flags - 37", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x37, "CntryAgressSpeedIndexStructure - 37", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x3C, "Country Parameters - 3c", preload | purgeable)
{
    0x3C,
    "FINLAND",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x3C, "Country Tx level - 3c", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x3C, "Country Relays - 3c", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x3C, "Country Pulse Dial Parameters - 3c", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x3C, "Country Ring Parameters - 3c", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x3C, "Country S Register Limiting - 3c", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x3C, "Country DTMF Parameters - 3c", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x3C, "Country Filter Parameters - 3c", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x3C, "Country Threshold Structure - 3c", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x3C, "Country RLSD - 3c", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x3C, "Country Tone - 3c", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x3C, "Country Timing - 3c", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x3C, "Country Cadence - 3c", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       250,    600,    250,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x3C, "Country Blacklisting - 3c", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x3C, "Country Caller ID - 3c", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x3C, "Country Call Progress Flags - 3c", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x3C, "CntryAgressSpeedIndexStructure - 3c", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x3D, "Country Parameters - 3d", preload | purgeable)
{
    0x3D,
    "FRANCE",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x3D, "Country Tx level - 3d", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x3D, "Country Relays - 3d", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x3D, "Country Pulse Dial Parameters - 3d", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0xC8,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x3D, "Country Ring Parameters - 3d", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1D4C,
    0x01F4,
    0x00C8,
    0x0190 
};

//	S Register Limiting
resource 'CSRG' (0x3D, "Country S Register Limiting - 3d", preload | purgeable)
{
      0,   2,   5,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     29,   5, 100,  27,
     28,   0, 255,   8 
};

resource 'CDTM' (0x3D, "Country DTMF Parameters - 3d", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x3D, "Country Filter Parameters - 3d", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x3D, "Country Threshold Structure - 3d", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x3D, "Country RLSD - 3d", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x3D, "Country Tone - 3d", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x3D, "Country Timing - 3d", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x3D, "Country Cadence - 3d", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x3D, "Country Blacklisting - 3d", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x3D, "Country Caller ID - 3d", preload | purgeable)
{
    0x04
};

resource 'CCPF' (0x3D, "Country Call Progress Flags - 3d", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x3D, "CntryAgressSpeedIndexStructure - 3d", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x42, "Country Parameters - 42", preload | purgeable)
{
    0x42,
    "GERMANY",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x42, "Country Tx level - 42", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x31,
    0x1C,
    0x04
};

//	Relays
resource 'CRLS' (0x42, "Country Relays - 42", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x42, "Country Pulse Dial Parameters - 42", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0xFFFFFFFE,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x42, "Country Ring Parameters - 42", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x42, "Country S Register Limiting - 42", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x42, "Country DTMF Parameters - 42", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x42, "Country Filter Parameters - 42", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x42, "Country Threshold Structure - 42", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x42, "Country RLSD - 42", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x42, "Country Tone - 42", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x42, "Country Timing - 42", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x42, "Country Cadence - 42", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
        80,    600,    300,    650,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
        80,    300,    150,    650,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x42, "Country Blacklisting - 42", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x42, "Country Caller ID - 42", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x42, "Country Call Progress Flags - 42", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x40 
};

resource 'CASI' (0x42, "CntryAgressSpeedIndexStructure - 42", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x46, "Country Parameters - 46", preload | purgeable)
{
    0x46,
    "GREECE",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x46, "Country Tx level - 46", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x46, "Country Relays - 46", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x46, "Country Pulse Dial Parameters - 46", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x46, "Country Ring Parameters - 46", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x46, "Country S Register Limiting - 46", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x46, "Country DTMF Parameters - 46", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x46, "Country Filter Parameters - 46", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x46, "Country Threshold Structure - 46", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x46, "Country RLSD - 46", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x46, "Country Tone - 46", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x46, "Country Timing - 46", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x46, "Country Cadence - 46", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       150,    350,    150,    350,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    350,    150,    350,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x46, "Country Blacklisting - 46", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x46, "Country Caller ID - 46", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x46, "Country Call Progress Flags - 46", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x46, "CntryAgressSpeedIndexStructure - 46", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x49, "Country Parameters - 49", preload | purgeable)
{
    0x49,
    "GUATEMALA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x49, "Country Tx level - 49", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x49, "Country Relays - 49", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x49, "Country Pulse Dial Parameters - 49", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x49, "Country Ring Parameters - 49", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x49, "Country S Register Limiting - 49", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x49, "Country DTMF Parameters - 49", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x49, "Country Filter Parameters - 49", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x49, "Country Threshold Structure - 49", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x49, "Country RLSD - 49", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x49, "Country Tone - 49", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x49, "Country Timing - 49", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x49, "Country Cadence - 49", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x49, "Country Blacklisting - 49", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x49, "Country Caller ID - 49", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x49, "Country Call Progress Flags - 49", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x49, "CntryAgressSpeedIndexStructure - 49", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x4E, "Country Parameters - 4e", preload | purgeable)
{
    0x4E,
    "HAITI",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x4E, "Country Tx level - 4e", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x4E, "Country Relays - 4e", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x4E, "Country Pulse Dial Parameters - 4e", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x4E, "Country Ring Parameters - 4e", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x4E, "Country S Register Limiting - 4e", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x4E, "Country DTMF Parameters - 4e", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x4E, "Country Filter Parameters - 4e", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x4E, "Country Threshold Structure - 4e", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x4E, "Country RLSD - 4e", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x4E, "Country Tone - 4e", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x4E, "Country Timing - 4e", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x4E, "Country Cadence - 4e", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x4E, "Country Blacklisting - 4e", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x4E, "Country Caller ID - 4e", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x4E, "Country Call Progress Flags - 4e", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x4E, "CntryAgressSpeedIndexStructure - 4e", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x4F, "Country Parameters - 4f", preload | purgeable)
{
    0x4F,
    "HONDURAS",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x4F, "Country Tx level - 4f", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x4F, "Country Relays - 4f", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x4F, "Country Pulse Dial Parameters - 4f", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x4F, "Country Ring Parameters - 4f", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x4F, "Country S Register Limiting - 4f", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x4F, "Country DTMF Parameters - 4f", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x4F, "Country Filter Parameters - 4f", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x4F, "Country Threshold Structure - 4f", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x4F, "Country RLSD - 4f", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x4F, "Country Tone - 4f", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x4F, "Country Timing - 4f", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x4F, "Country Cadence - 4f", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x4F, "Country Blacklisting - 4f", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x4F, "Country Caller ID - 4f", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x4F, "Country Call Progress Flags - 4f", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x4F, "CntryAgressSpeedIndexStructure - 4f", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x50, "Country Parameters - 50", preload | purgeable)
{
    0x50,
    "HONG KONG",
    "001"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x50, "Country Tx level - 50", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x50, "Country Relays - 50", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x50, "Country Pulse Dial Parameters - 50", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0xFFFFFFFE,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x50, "Country Ring Parameters - 50", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x50, "Country S Register Limiting - 50", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 200,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x50, "Country DTMF Parameters - 50", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x50, "Country Filter Parameters - 50", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x50, "Country Threshold Structure - 50", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x50, "Country RLSD - 50", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x50, "Country Tone - 50", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x50, "Country Timing - 50", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x50, "Country Cadence - 50", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x50, "Country Blacklisting - 50", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x50, "Country Caller ID - 50", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x50, "Country Call Progress Flags - 50", preload | purgeable)
{
    0x00,
    0x40,
    0x32,
    0x00 
};

resource 'CASI' (0x50, "CntryAgressSpeedIndexStructure - 50", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x51, "Country Parameters - 51", preload | purgeable)
{
    0x51,
    "HUNGARY",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x51, "Country Tx level - 51", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x4D,
    0x38,
    0x04
};

//	Relays
resource 'CRLS' (0x51, "Country Relays - 51", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x51, "Country Pulse Dial Parameters - 51", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0xFFFFFFFE,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x51, "Country Ring Parameters - 51", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x51, "Country S Register Limiting - 51", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,   9,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x51, "Country DTMF Parameters - 51", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x51, "Country Filter Parameters - 51", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x51, "Country Threshold Structure - 51", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x51, "Country RLSD - 51", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x51, "Country Tone - 51", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x51, "Country Timing - 51", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x51, "Country Cadence - 51", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x51, "Country Blacklisting - 51", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x51, "Country Caller ID - 51", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x51, "Country Call Progress Flags - 51", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x51, "CntryAgressSpeedIndexStructure - 51", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x52, "Country Parameters - 52", preload | purgeable)
{
    0x52,
    "ICELAND",
    "90"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x52, "Country Tx level - 52", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x52, "Country Relays - 52", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x52, "Country Pulse Dial Parameters - 52", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x52, "Country Ring Parameters - 52", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x52, "Country S Register Limiting - 52", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     29,   5, 100,  10,
    255,   0,   0,   0 
};

resource 'CDTM' (0x52, "Country DTMF Parameters - 52", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x52, "Country Filter Parameters - 52", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x52, "Country Threshold Structure - 52", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x52, "Country RLSD - 52", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x52, "Country Tone - 52", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x52, "Country Timing - 52", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x52, "Country Cadence - 52", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    350,    150,    350,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x52, "Country Blacklisting - 52", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x52, "Country Caller ID - 52", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x52, "Country Call Progress Flags - 52", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x52, "CntryAgressSpeedIndexStructure - 52", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x53, "Country Parameters - 53", preload | purgeable)
{
    0x53,
    "INDIA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x53, "Country Tx level - 53", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x53, "Country Relays - 53", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x53, "Country Pulse Dial Parameters - 53", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0320,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x53, "Country Ring Parameters - 53", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x53, "Country S Register Limiting - 53", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x53, "Country DTMF Parameters - 53", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x53, "Country Filter Parameters - 53", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x53, "Country Threshold Structure - 53", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x53, "Country RLSD - 53", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x53, "Country Tone - 53", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x53, "Country Timing - 53", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x53, "Country Cadence - 53", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    850,    300,    850,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x53, "Country Blacklisting - 53", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x53, "Country Caller ID - 53", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x53, "Country Call Progress Flags - 53", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x53, "CntryAgressSpeedIndexStructure - 53", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x54, "Country Parameters - 54", preload | purgeable)
{
    0x54,
    "INDONESIA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x54, "Country Tx level - 54", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x54, "Country Relays - 54", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x54, "Country Pulse Dial Parameters - 54", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x54, "Country Ring Parameters - 54", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x54, "Country S Register Limiting - 54", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  40, 255,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x54, "Country DTMF Parameters - 54", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x54, "Country Filter Parameters - 54", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x54, "Country Threshold Structure - 54", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x54, "Country RLSD - 54", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x54, "Country Tone - 54", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x54, "Country Timing - 54", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x54, "Country Cadence - 54", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x54, "Country Blacklisting - 54", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x54, "Country Caller ID - 54", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x54, "Country Call Progress Flags - 54", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x54, "CntryAgressSpeedIndexStructure - 54", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x57, "Country Parameters - 57", preload | purgeable)
{
    0x57,
    "IRELAND",
    "16"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x57, "Country Tx level - 57", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x57, "Country Relays - 57", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x57, "Country Pulse Dial Parameters - 57", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x57, "Country Ring Parameters - 57", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x57, "Country S Register Limiting - 57", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  10 
};

resource 'CDTM' (0x57, "Country DTMF Parameters - 57", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x57, "Country Filter Parameters - 57", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x57, "Country Threshold Structure - 57", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x57, "Country RLSD - 57", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x57, "Country Tone - 57", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x57, "Country Timing - 57", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x57, "Country Cadence - 57", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    800,    300,    800,      0,      0,      0,      0,      6,
       350,    450,    150,    250,    350,    450,   1800,   2200,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x57, "Country Blacklisting - 57", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x57, "Country Caller ID - 57", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x57, "Country Call Progress Flags - 57", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x57, "CntryAgressSpeedIndexStructure - 57", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x58, "Country Parameters - 58", preload | purgeable)
{
    0x58,
    "ISRAEL",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x58, "Country Tx level - 58", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x58, "Country Relays - 58", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x58, "Country Pulse Dial Parameters - 58", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x58, "Country Ring Parameters - 58", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x58, "Country S Register Limiting - 58", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x58, "Country DTMF Parameters - 58", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x58, "Country Filter Parameters - 58", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x58, "Country Threshold Structure - 58", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x58, "Country RLSD - 58", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x58, "Country Tone - 58", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x58, "Country Timing - 58", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x58, "Country Cadence - 58", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       180,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       180,    600,    300,    600,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x58, "Country Blacklisting - 58", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x58, "Country Caller ID - 58", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x58, "Country Call Progress Flags - 58", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x58, "CntryAgressSpeedIndexStructure - 58", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x59, "Country Parameters - 59", preload | purgeable)
{
    0x59,
    "ITALY",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x59, "Country Tx level - 59", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x59, "Country Relays - 59", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x59, "Country Pulse Dial Parameters - 59", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x59, "Country Ring Parameters - 59", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x59, "Country S Register Limiting - 59", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,   9 
};

resource 'CDTM' (0x59, "Country DTMF Parameters - 59", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x59, "Country Filter Parameters - 59", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x59, "Country Threshold Structure - 59", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x59, "Country RLSD - 59", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x59, "Country Tone - 59", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x59, "Country Timing - 59", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x59, "Country Cadence - 59", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       160,    240,    160,    240,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x59, "Country Blacklisting - 59", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x59, "Country Caller ID - 59", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x59, "Country Call Progress Flags - 59", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x59, "CntryAgressSpeedIndexStructure - 59", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x61, "Country Parameters - 61", preload | purgeable)
{
    0x61,
    "KOREA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x61, "Country Tx level - 61", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x61, "Country Relays - 61", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x61, "Country Pulse Dial Parameters - 61", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x61, "Country Ring Parameters - 61", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x61, "Country S Register Limiting - 61", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  60, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x61, "Country DTMF Parameters - 61", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x61, "Country Filter Parameters - 61", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x61, "Country Threshold Structure - 61", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x61, "Country RLSD - 61", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x61, "Country Tone - 61", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x61, "Country Timing - 61", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x61, "Country Cadence - 61", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    350,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x61, "Country Blacklisting - 61", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x61, "Country Caller ID - 61", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x61, "Country Call Progress Flags - 61", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x61, "CntryAgressSpeedIndexStructure - 61", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x68, "Country Parameters - 68", preload | purgeable)
{
    0x68,
    "LIECHTENSTEIN",
    "41"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x68, "Country Tx level - 68", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x5F,
    0x44,
    0x04
};

//	Relays
resource 'CRLS' (0x68, "Country Relays - 68", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x68, "Country Pulse Dial Parameters - 68", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0320,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x68, "Country Ring Parameters - 68", preload | purgeable)
{
    0x46,
    0x0F,
    0x00BE,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x68, "Country S Register Limiting - 68", preload | purgeable)
{
      6,   4, 255,   4,
      7,   1, 255, 180,
     10,  20, 254,  20,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x68, "Country DTMF Parameters - 68", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x68, "Country Filter Parameters - 68", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x68, "Country Threshold Structure - 68", preload | purgeable)
{
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x68, "Country RLSD - 68", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x68, "Country Tone - 68", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x68, "Country Timing - 68", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x68, "Country Cadence - 68", preload | purgeable)
{
         2,
       170,    300,    170,    300,    520,    700,    820,   1180,      2,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       160,    650,    160,    720,      0,      0,      0,      0,      6,
       800,   2000,   2900,   5000,      0,      0,      0,      0,      2,
       240,    360,    240,    360,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x68, "Country Blacklisting - 68", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x0005,
    0x0005,
    0xFFFF,
    0x0000,
    0x000F,
    0x0000,
    0xC0 
};

resource 'CCID' (0x68, "Country Caller ID - 68", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x68, "Country Call Progress Flags - 68", preload | purgeable)
{
    0x00,
    0x00,
    0x3A,
    0x00 
};

resource 'CASI' (0x68, "CntryAgressSpeedIndexStructure - 68", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x69, "Country Parameters - 69", preload | purgeable)
{
    0x69,
    "LUXEMBOURG",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x69, "Country Tx level - 69", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x4D,
    0x38,
    0x04
};

//	Relays
resource 'CRLS' (0x69, "Country Relays - 69", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x69, "Country Pulse Dial Parameters - 69", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x69, "Country Ring Parameters - 69", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x69, "Country S Register Limiting - 69", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,  10 
};

resource 'CDTM' (0x69, "Country DTMF Parameters - 69", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x69, "Country Filter Parameters - 69", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x69, "Country Threshold Structure - 69", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x69, "Country RLSD - 69", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x69, "Country Tone - 69", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x69, "Country Timing - 69", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x69, "Country Cadence - 69", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x69, "Country Blacklisting - 69", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x69, "Country Caller ID - 69", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x69, "Country Call Progress Flags - 69", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x69, "CntryAgressSpeedIndexStructure - 69", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x6C, "Country Parameters - 6c", preload | purgeable)
{
    0x6C,
    "MALAYSIA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x6C, "Country Tx level - 6c", preload | purgeable)
{
    0x0F,
    0x00,
    0x0A,
    0x0F,
    0x00,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x6C, "Country Relays - 6c", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x6C, "Country Pulse Dial Parameters - 6c", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x6C, "Country Ring Parameters - 6c", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x6C, "Country S Register Limiting - 6c", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x6C, "Country DTMF Parameters - 6c", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x6C, "Country Filter Parameters - 6c", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x6C, "Country Threshold Structure - 6c", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x6C, "Country RLSD - 6c", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x6C, "Country Tone - 6c", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x6C, "Country Timing - 6c", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x6C, "Country Cadence - 6c", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x6C, "Country Blacklisting - 6c", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x6C, "Country Caller ID - 6c", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x6C, "Country Call Progress Flags - 6c", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x6C, "CntryAgressSpeedIndexStructure - 6c", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x70, "Country Parameters - 70", preload | purgeable)
{
    0x70,
    "MALTA",
    "356"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x70, "Country Tx level - 70", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x5F,
    0x44,
    0x04
};

//	Relays
resource 'CRLS' (0x70, "Country Relays - 70", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x70, "Country Pulse Dial Parameters - 70", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0320,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x70, "Country Ring Parameters - 70", preload | purgeable)
{
    0x46,
    0x0F,
    0x00BE,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x70, "Country S Register Limiting - 70", preload | purgeable)
{
      6,   4, 255,   4,
      7,   1, 255, 180,
     10,  20, 254,  20,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x70, "Country DTMF Parameters - 70", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x70, "Country Filter Parameters - 70", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x70, "Country Threshold Structure - 70", preload | purgeable)
{
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x0400, 0x0400,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x70, "Country RLSD - 70", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x70, "Country Tone - 70", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x70, "Country Timing - 70", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x70, "Country Cadence - 70", preload | purgeable)
{
         2,
       170,    300,    170,    300,    520,    700,    820,   1180,      2,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       160,    650,    160,    720,      0,      0,      0,      0,      6,
       800,   2000,   2900,   5000,      0,      0,      0,      0,      2,
       240,    360,    240,    360,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x70, "Country Blacklisting - 70", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x0005,
    0x0005,
    0xFFFF,
    0x0000,
    0x000F,
    0x0000,
    0xC0 
};

resource 'CCID' (0x70, "Country Caller ID - 70", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x70, "Country Call Progress Flags - 70", preload | purgeable)
{
    0x00,
    0x00,
    0x3A,
    0x00 
};

resource 'CASI' (0x70, "CntryAgressSpeedIndexStructure - 70", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x7B, "Country Parameters - 7b", preload | purgeable)
{
    0x7B,
    "NETHERLANDS",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x7B, "Country Tx level - 7b", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x7B, "Country Relays - 7b", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x7B, "Country Pulse Dial Parameters - 7b", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x7B, "Country Ring Parameters - 7b", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x03E8,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x7B, "Country S Register Limiting - 7b", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x7B, "Country DTMF Parameters - 7b", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x7B, "Country Filter Parameters - 7b", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x7B, "Country Threshold Structure - 7b", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x7B, "Country RLSD - 7b", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x7B, "Country Tone - 7b", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x7B, "Country Timing - 7b", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x7B, "Country Cadence - 7b", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       350,    650,    350,    650,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       130,    330,    130,    330,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x7B, "Country Blacklisting - 7b", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x7B, "Country Caller ID - 7b", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x7B, "Country Call Progress Flags - 7b", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x7B, "CntryAgressSpeedIndexStructure - 7b", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x7E, "Country Parameters - 7e", preload | purgeable)
{
    0x7E,
    "NEW ZEALAND",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x7E, "Country Tx level - 7e", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0x7E, "Country Relays - 7e", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x7E, "Country Pulse Dial Parameters - 7e", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x7E, "Country Ring Parameters - 7e", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x7E, "Country S Register Limiting - 7e", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1,  80,  72,
     10,   1, 150,  14,
     11,  65, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  60,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x7E, "Country DTMF Parameters - 7e", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x7E, "Country Filter Parameters - 7e", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x7E, "Country Threshold Structure - 7e", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x7E, "Country RLSD - 7e", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x7E, "Country Tone - 7e", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x7E, "Country Timing - 7e", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x7E, "Country Cadence - 7e", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       350,    450,    150,    250,    350,    450,   1800,   2200,      2,
        60,    300,     60,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x7E, "Country Blacklisting - 7e", preload | purgeable)
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x001E,
    0x0005,
    0x0708,
    0x0000,
    0x000A,
    0x0000,
    0x80 
};

resource 'CCID' (0x7E, "Country Caller ID - 7e", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x7E, "Country Call Progress Flags - 7e", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x7E, "CntryAgressSpeedIndexStructure - 7e", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x82, "Country Parameters - 82", preload | purgeable)
{
    0x82,
    "NORWAY",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x82, "Country Tx level - 82", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x82, "Country Relays - 82", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x82, "Country Pulse Dial Parameters - 82", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x82, "Country Ring Parameters - 82", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x82, "Country S Register Limiting - 82", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x82, "Country DTMF Parameters - 82", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x82, "Country Filter Parameters - 82", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x82, "Country Threshold Structure - 82", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x82, "Country RLSD - 82", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x82, "Country Tone - 82", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x82, "Country Timing - 82", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x82, "Country Cadence - 82", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x82, "Country Blacklisting - 82", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x82, "Country Caller ID - 82", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x82, "Country Call Progress Flags - 82", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x82, "CntryAgressSpeedIndexStructure - 82", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x85, "Country Parameters - 85", preload | purgeable)
{
    0x85,
    "PANAMA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x85, "Country Tx level - 85", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x85, "Country Relays - 85", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x85, "Country Pulse Dial Parameters - 85", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x85, "Country Ring Parameters - 85", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x85, "Country S Register Limiting - 85", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x85, "Country DTMF Parameters - 85", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x85, "Country Filter Parameters - 85", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x85, "Country Threshold Structure - 85", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x85, "Country RLSD - 85", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x85, "Country Tone - 85", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x85, "Country Timing - 85", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x85, "Country Cadence - 85", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x85, "Country Blacklisting - 85", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x85, "Country Caller ID - 85", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x85, "Country Call Progress Flags - 85", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x85, "CntryAgressSpeedIndexStructure - 85", preload | purgeable)
{
    0x71,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x87, "Country Parameters - 87", preload | purgeable)
{
    0x87,
    "PARAGUAY",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x87, "Country Tx level - 87", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x87, "Country Relays - 87", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x87, "Country Pulse Dial Parameters - 87", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x87, "Country Ring Parameters - 87", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x87, "Country S Register Limiting - 87", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x87, "Country DTMF Parameters - 87", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x87, "Country Filter Parameters - 87", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x87, "Country Threshold Structure - 87", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x87, "Country RLSD - 87", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x87, "Country Tone - 87", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x87, "Country Timing - 87", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x87, "Country Cadence - 87", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x87, "Country Blacklisting - 87", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x87, "Country Caller ID - 87", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x87, "Country Call Progress Flags - 87", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x87, "CntryAgressSpeedIndexStructure - 87", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x88, "Country Parameters - 88", preload | purgeable)
{
    0x88,
    "PERU",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x88, "Country Tx level - 88", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0x88, "Country Relays - 88", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x88, "Country Pulse Dial Parameters - 88", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x88, "Country Ring Parameters - 88", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x88, "Country S Register Limiting - 88", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x88, "Country DTMF Parameters - 88", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0x88, "Country Filter Parameters - 88", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x88, "Country Threshold Structure - 88", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x88, "Country RLSD - 88", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x88, "Country Tone - 88", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x88, "Country Timing - 88", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x88, "Country Cadence - 88", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x88, "Country Blacklisting - 88", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x88, "Country Caller ID - 88", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0x88, "Country Call Progress Flags - 88", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0x88, "CntryAgressSpeedIndexStructure - 88", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x89, "Country Parameters - 89", preload | purgeable)
{
    0x89,
    "PHILIPPINES",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x89, "Country Tx level - 89", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x4D,
    0x38,
    0x04
};

//	Relays
resource 'CRLS' (0x89, "Country Relays - 89", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x89, "Country Pulse Dial Parameters - 89", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x89, "Country Ring Parameters - 89", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x89, "Country S Register Limiting - 89", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
     10,   1, 254,  14,
     11,  40, 255,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x89, "Country DTMF Parameters - 89", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x89, "Country Filter Parameters - 89", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x89, "Country Threshold Structure - 89", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x89, "Country RLSD - 89", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x89, "Country Tone - 89", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x89, "Country Timing - 89", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x89, "Country Cadence - 89", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    580,    180,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       180,    280,    180,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x89, "Country Blacklisting - 89", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x89, "Country Caller ID - 89", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x89, "Country Call Progress Flags - 89", preload | purgeable)
{
    0x00,
    0x40,
    0x32,
    0x00 
};

resource 'CASI' (0x89, "CntryAgressSpeedIndexStructure - 89", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x8B, "Country Parameters - 8b", preload | purgeable)
{
    0x8B,
    "PORTUGAL",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x8B, "Country Tx level - 8b", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0x8B, "Country Relays - 8b", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x8B, "Country Pulse Dial Parameters - 8b", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x8B, "Country Ring Parameters - 8b", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x8B, "Country S Register Limiting - 8b", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  23 
};

resource 'CDTM' (0x8B, "Country DTMF Parameters - 8b", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x8B, "Country Filter Parameters - 8b", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x8B, "Country Threshold Structure - 8b", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x8B, "Country RLSD - 8b", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x8B, "Country Tone - 8b", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0046,
    0x46,
    0x0000 
};

resource 'CTIM' (0x8B, "Country Timing - 8b", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x8B, "Country Cadence - 8b", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       350,    650,    350,    650,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       130,    300,    130,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x8B, "Country Blacklisting - 8b", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0x8B, "Country Caller ID - 8b", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x8B, "Country Call Progress Flags - 8b", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0x8B, "CntryAgressSpeedIndexStructure - 8b", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x9C, "Country Parameters - 9c", preload | purgeable)
{
    0x9C,
    "SINGAPORE",
    "005"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x9C, "Country Tx level - 9c", preload | purgeable)
{
    0x0F,
    0x00,
    0x0A,
    0x0F,
    0x00,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x32,
    0x17,
    0x04
};

//	Relays
resource 'CRLS' (0x9C, "Country Relays - 9c", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x9C, "Country Pulse Dial Parameters - 9c", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x9C, "Country Ring Parameters - 9c", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x9C, "Country S Register Limiting - 9c", preload | purgeable)
{
      0,   2,   8,   0,
      6,   2,  10,   2,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     29,   1, 255,  40,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x9C, "Country DTMF Parameters - 9c", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x9C, "Country Filter Parameters - 9c", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x9C, "Country Threshold Structure - 9c", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x9C, "Country RLSD - 9c", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x9C, "Country Tone - 9c", preload | purgeable)
{
    0x0384,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x9C, "Country Timing - 9c", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x9C, "Country Cadence - 9c", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    850,    300,    850,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x9C, "Country Blacklisting - 9c", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x9C, "Country Caller ID - 9c", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x9C, "Country Call Progress Flags - 9c", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x9C, "CntryAgressSpeedIndexStructure - 9c", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0x9F, "Country Parameters - 9f", preload | purgeable)
{
    0x9F,
    "SOUTH AFRICA",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0x9F, "Country Tx level - 9f", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x4D,
    0x38,
    0x04
};

//	Relays
resource 'CRLS' (0x9F, "Country Relays - 9f", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0x9F, "Country Pulse Dial Parameters - 9f", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0x9F, "Country Ring Parameters - 9f", preload | purgeable)
{
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0x9F, "Country S Register Limiting - 9f", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   8,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0x9F, "Country DTMF Parameters - 9f", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0x9F, "Country Filter Parameters - 9f", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0x9F, "Country Threshold Structure - 9f", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0x9F, "Country RLSD - 9f", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0x9F, "Country Tone - 9f", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0x9F, "Country Timing - 9f", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0x9F, "Country Cadence - 9f", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0x9F, "Country Blacklisting - 9f", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0x9F, "Country Caller ID - 9f", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0x9F, "Country Call Progress Flags - 9f", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0x9F, "CntryAgressSpeedIndexStructure - 9f", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xA0, "Country Parameters - a0", preload | purgeable)
{
    0xA0,
    "SPAIN",
    "07"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xA0, "Country Tx level - a0", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0xA0, "Country Relays - a0", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xA0, "Country Pulse Dial Parameters - a0", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xA0, "Country Ring Parameters - a0", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xA0, "Country S Register Limiting - a0", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  10 
};

resource 'CDTM' (0xA0, "Country DTMF Parameters - a0", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xA0, "Country Filter Parameters - a0", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xA0, "Country Threshold Structure - a0", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xA0, "Country RLSD - a0", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xA0, "Country Tone - a0", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xA0, "Country Timing - a0", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xA0, "Country Cadence - a0", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    650,    300,    650,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       130,    300,    130,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xA0, "Country Blacklisting - a0", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x00 
};

resource 'CCID' (0xA0, "Country Caller ID - a0", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xA0, "Country Call Progress Flags - a0", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0xA0, "CntryAgressSpeedIndexStructure - a0", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xA5, "Country Parameters - a5", preload | purgeable)
{
    0xA5,
    "SWEDEN",
    "009"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xA5, "Country Tx level - a5", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0xA5, "Country Relays - a5", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xA5, "Country Pulse Dial Parameters - a5", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xA5, "Country Ring Parameters - a5", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xA5, "Country S Register Limiting - a5", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0xA5, "Country DTMF Parameters - a5", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xA5, "Country Filter Parameters - a5", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xA5, "Country Threshold Structure - a5", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xA5, "Country RLSD - a5", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xA5, "Country Tone - a5", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xA5, "Country Timing - a5", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xA5, "Country Cadence - a5", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       150,    300,    150,    300,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       180,    300,    600,    850,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xA5, "Country Blacklisting - a5", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0xA5, "Country Caller ID - a5", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xA5, "Country Call Progress Flags - a5", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0xA5, "CntryAgressSpeedIndexStructure - a5", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xA6, "Country Parameters - a6", preload | purgeable)
{
    0xA6,
    "SWITZERLAND",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xA6, "Country Tx level - a6", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x31,
    0x1C,
    0x04
};

//	Relays
resource 'CRLS' (0xA6, "Country Relays - a6", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xA6, "Country Pulse Dial Parameters - a6", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xA6, "Country Ring Parameters - a6", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xA6, "Country S Register Limiting - a6", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   0,
     29,   5, 100,   9 
};

resource 'CDTM' (0xA6, "Country DTMF Parameters - a6", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xA6, "Country Filter Parameters - a6", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xA6, "Country Threshold Structure - a6", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xA6, "Country RLSD - a6", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xA6, "Country Tone - a6", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xA6, "Country Timing - a6", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xA6, "Country Cadence - a6", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    700,    300,    700,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       100,    300,    100,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xA6, "Country Blacklisting - a6", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0xA6, "Country Caller ID - a6", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xA6, "Country Call Progress Flags - a6", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x40 
};

resource 'CASI' (0xA6, "CntryAgressSpeedIndexStructure - a6", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xA9, "Country Parameters - a9", preload | purgeable)
{
    0xA9,
    "THAILAND",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xA9, "Country Tx level - a9", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x39,
    0x24,
    0x04
};

//	Relays
resource 'CRLS' (0xA9, "Country Relays - a9", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xA9, "Country Pulse Dial Parameters - a9", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xA9, "Country Ring Parameters - a9", preload | purgeable)
{
    0x46,
    0x0F,
    0x012C,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xA9, "Country S Register Limiting - a9", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  90,
     10,   1, 254,  14,
     11,  50, 255,  85,
     28,   0, 255,   0,
     29,   5, 100,  10,
    255,   0,   0,   0,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0xA9, "Country DTMF Parameters - a9", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xA9, "Country Filter Parameters - a9", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xA9, "Country Threshold Structure - a9", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xA9, "Country RLSD - a9", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xA9, "Country Tone - a9", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xA9, "Country Timing - a9", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xA9, "Country Cadence - a9", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       300,    600,    300,    600,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       150,    300,    150,    300,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xA9, "Country Blacklisting - a9", preload | purgeable)
{
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x01,
    0x0005,
    0x0005,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0xA9, "Country Caller ID - a9", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xA9, "Country Call Progress Flags - a9", preload | purgeable)
{
    0x00,
    0x40,
    0x30,
    0x00 
};

resource 'CASI' (0xA9, "CntryAgressSpeedIndexStructure - a9", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xB4, "Country Parameters - b4", preload | purgeable)
{
    0xB4,
    "UK",
    "00"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xB4, "Country Tx level - b4", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0B,
    0x0F,
    0x0A,
    0x0B,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x48,
    0x30,
    0x04
};

//	Relays
resource 'CRLS' (0xB4, "Country Relays - b4", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xB4, "Country Pulse Dial Parameters - b4", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x5A,
    0x5A,
    0x0384,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xB4, "Country Ring Parameters - b4", preload | purgeable)
{
    0x46,
    0x0E,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xB4, "Country S Register Limiting - b4", preload | purgeable)
{
      0,   2,   8,   0,
      6,   3,   8,   3,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 254,  14,
     11,  50, 255,  85,
     27,   0, 255,  73,
     28,   0, 255,   8,
     29,   5, 100,  10 
};

resource 'CDTM' (0xB4, "Country DTMF Parameters - b4", preload | purgeable)
{
    0x0000,
    0x0000
};

resource 'CFLT' (0xB4, "Country Filter Parameters - b4", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x1923, 0xCD3C, 0x1A2C, 0xC467, 0x774F, 
    0x1923, 0xD7DB, 0x1A2C, 0xC774, 0x69BC, 
    0x7B30, 
    0x06CF, 
    0x0180, 
    0x0180, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xB4, "Country Threshold Structure - b4", preload | purgeable)
{
    0x0180, 0x0180,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x0200, 0x0200,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xB4, "Country RLSD - b4", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xB4, "Country Tone - b4", preload | purgeable)
{
    0x0226,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xB4, "Country Timing - b4", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xB4, "Country Cadence - b4", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       200,    550,    200,    550,      0,      0,      0,      0,      6,
       650,   1600,   2900,   6000,      0,      0,      0,      0,      2,
       130,    500,    250,    750,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xB4, "Country Blacklisting - b4", preload | purgeable)
{
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x03,
    0x0005,
    0x0005,
    0x0000,
    0x0005,
    0x0000,
    0x0005,
    0x80 
};

resource 'CCID' (0xB4, "Country Caller ID - b4", preload | purgeable)
{
    0x05
};

resource 'CCPF' (0xB4, "Country Call Progress Flags - b4", preload | purgeable)
{
    0x00,
    0x40,
    0x10,
    0x00 
};

resource 'CASI' (0xB4, "CntryAgressSpeedIndexStructure - b4", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xB7, "Country Parameters - b7", preload | purgeable)
{
    0xB7,
    "URUGUAY",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xB7, "Country Tx level - b7", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0xB7, "Country Relays - b7", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xB7, "Country Pulse Dial Parameters - b7", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xB7, "Country Ring Parameters - b7", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xB7, "Country S Register Limiting - b7", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0xB7, "Country DTMF Parameters - b7", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0xB7, "Country Filter Parameters - b7", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xB7, "Country Threshold Structure - b7", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xB7, "Country RLSD - b7", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xB7, "Country Tone - b7", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xB7, "Country Timing - b7", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xB7, "Country Cadence - b7", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xB7, "Country Blacklisting - b7", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0xB7, "Country Caller ID - b7", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0xB7, "Country Call Progress Flags - b7", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0xB7, "CntryAgressSpeedIndexStructure - b7", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xBB, "Country Parameters - bb", preload | purgeable)
{
    0xBB,
    "VENEZUELA",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xBB, "Country Tx level - bb", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0xBB, "Country Relays - bb", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xBB, "Country Pulse Dial Parameters - bb", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xBB, "Country Ring Parameters - bb", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xBB, "Country S Register Limiting - bb", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0xBB, "Country DTMF Parameters - bb", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0xBB, "Country Filter Parameters - bb", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xBB, "Country Threshold Structure - bb", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xBB, "Country RLSD - bb", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xBB, "Country Tone - bb", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xBB, "Country Timing - bb", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xBB, "Country Cadence - bb", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      4,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xBB, "Country Blacklisting - bb", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0xBB, "Country Caller ID - bb", preload | purgeable)
{
    0x01
};

resource 'CCPF' (0xBB, "Country Call Progress Flags - bb", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0xBB, "CntryAgressSpeedIndexStructure - bb", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#include "TypeDefs.r"

// CtryPrmsStruct
resource 'CNTR' (0xBC, "Country Parameters - bc", preload | purgeable)
{
    0xBC,
    "VIETNAM",
    "011"
};

//	CntryTxlevelStructure
resource 'CTXL' (0xBC, "Country Tx level - bc", preload | purgeable)
{
    0x0F,
    0x0A,
    0x0A,
    0x0F,
    0x0A,
    0x0A,
    0x00,
    0x0F,
    0x08,
    0x00,
    0x36,
    0x22,
    0x04
};

//	Relays
resource 'CRLS' (0xBC, "Country Relays - bc", preload | purgeable)
{
    0x0800,
    0x0000,
    0x0800,
    0x0800,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0800,
    0x0000,
    0x0800,
    0x0800
};

//	CntryPulseStructure
resource 'CPLS' (0xBC, "Country Pulse Dial Parameters - bc", preload | purgeable)
{
    0x00,
    0x00,
    0x01,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02EE,
    0x0000 
};

//	RING_PARAMS
resource 'CRNG' (0xBC, "Country Ring Parameters - bc", preload | purgeable)
{
    0x46,
    0x0F,
    0x0096,
    0x0000,
    0x012C,
    0x1F40,
    0x0000,
    0x0000,
    0x0000 
};

//	S Register Limiting
resource 'CSRG' (0xBC, "Country S Register Limiting - bc", preload | purgeable)
{
      6,   0, 255,   2,
      7,   1, 255,  50,
      8,   2, 255,   2,
      9,   1, 255,   6,
     10,   1, 255,  14,
     11,  50, 255,  95,
     27,   0, 255,  73,
     29,  70,  70,  70,
    255,   0,   0,   0,
    255,   0,   0,   0 
};

resource 'CDTM' (0xBC, "Country DTMF Parameters - bc", preload | purgeable)
{
    0x005C,
    0x0048
};

resource 'CFLT' (0xBC, "Country Filter Parameters - bc", preload | purgeable)
{
    // Primary ToneA
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Alternate ToneA
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 

    // Voice Mode Filter VToneACallProgress
    0x0000, 
    0x08C2, 0xEE7C, 0x08C2, 0xC774, 0x74FE, 
    0x08C2, 0x1184, 0x08C2, 0xCD4F, 0x6495, 
    0x7B30, 
    0x04CF, 
    0x0000, 
    0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000 
};

resource 'CTHR' (0xBC, "Country Threshold Structure - bc", preload | purgeable)
{
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x0200, 0x0200,
    0x0000, 0x0000,
    0x01A0, 0x01A0,
    0x2750,
    0x0000,
    0x0000
};

resource 'CCDR' (0xBC, "Country RLSD - bc", preload | purgeable)
{
    0x00000000
};

resource 'CTON' (0xBC, "Country Tone - bc", preload | purgeable)
{
    0x02EE,
    0x02EE,
    0x0064,
    0x0005,
    0x46,
    0x0000 
};

resource 'CTIM' (0xBC, "Country Timing - bc", preload | purgeable)
{
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

resource 'CCAD' (0xBC, "Country Cadence - bc", preload | purgeable)
{
         1,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,      0,
       420,    580,    420,    580,      0,      0,      0,      0,      6,
       900,   3000,   2900,   4600,      0,      0,      0,      0,      2,
       220,    280,    220,    280,      0,      0,      0,      0,      6 
};

resource 'CBLL' (0xBC, "Country Blacklisting - bc", preload | purgeable)
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x00 
};

resource 'CCID' (0xBC, "Country Caller ID - bc", preload | purgeable)
{
    0x00
};

resource 'CCPF' (0xBC, "Country Call Progress Flags - bc", preload | purgeable)
{
    0x00,
    0x00,
    0x32,
    0x00 
};

resource 'CASI' (0xBC, "CntryAgressSpeedIndexStructure - bc", preload | purgeable)
{
    0x64,
    0x64,
    0x64,
};

#endif