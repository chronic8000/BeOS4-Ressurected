/****************************************************************************************
*                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/ConfigTypes.h_v   1.13   29 Jun 2000 11:50:02   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			ConfigTypes.h

File Description:	Configuration parameter data structures.

*****************************************************************************************/


/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2000                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                             Personal Computing Division                           ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
*****************************************************************************************/


#ifndef __CONFIGTYPES_H__
#define __CONFIGTYPES_H__

#include "configcodes.h"
#include "osstring_ex.h"

typedef enum tagCFGMGR_CODE CFGMGR_CODE;

typedef struct tagDTE_USER_CONFIG
{
    UINT8       bQuiet;     // DTE result codes disabled
    UINT8       bVerbose;   // Long result form
    UINT8       bLevel;     // Result codes level 
    UINT8       bConnect;   // Connect Message Form

} DTE_USER_CONFIG, *PDTE_USER_CONFIG;

typedef struct tagMS_USER_CONFIG
{
   MODULATION_TYPE	eModulation;
   BOOL				bAutomode;
   UINT32			dwMinTxSpeed;
   UINT32			dwMaxTxSpeed;
   UINT32			dwMinRxSpeed;
   UINT32			dwMaxRxSpeed;

} MS_USER_CONFIG, *PMS_USER_CONFIG;

typedef struct tagDS_USER_CONFIG 
{
   UINT32	dwCompressDict;
   UINT32	dwCompressStr;
   UINT32	dwCompressDir;
   UINT32	dwCompressNeg;

} DS_USER_CONFIG, *PDS_USER_CONFIG;

typedef struct tagEC_USER_CONFIG
{
   UINT32   dwOrgReq;
   UINT32   dwOrgFbk;
   UINT32   dwAnsFbk;
   BOOL     bV42SkipToXID;
   UINT32   dwSkipV42DetPhase;

} EC_USER_CONFIG, *PEC_USER_CONFIG;

typedef enum
{
	TONE,
	PULSE
} DIAL_MODE;

typedef struct
{
	UINT8	RingMaxFrequency,
			RingMinFrequency;
	UINT16	RingStable,
			MinTimeBeforeAnswering,
			MinDelayBtwnRings,
			MaxDelayBtwnRings,
			MinRingOnTime,
			MinImmediateRingOn,
			MaxImmediateRingOn;
} RING_PARAMS, *PRING_PARAMS;

#define NUM_LIMITED_SREGS	10		//Number of S-registers limited by entries in the INF file

typedef struct
{
	UINT16	InterCallDelay1,		//Delay when same number dialed after outgoing call
			InterCallDelay2,		//Delay when different number dialed after outgoing call
			InterCallDelay3,		//Delay after incoming call
			BillingDelay;			//Delay between ATA and start of v8bis
	UINT16	uiDummy1,
			uiDummy2;
} CALL_PROGRESS_TIMING_CONFIG, *PCALL_PROGRESS_TIMING_CONFIG;

typedef struct
{
	UINT16	DialToneStable,
			WDialToneStable,
			AnswerToneStable,
			CallProgressToneStable;
} TONE_DEBOUNCE_CONFIG, *PTONE_DEBOUNCE_CONFIG;

typedef struct 
{
	UINT8 v8o,		// 1 = enable +A8x indicators, 6 = disable +A8x (originate)
		  v8a,		// 1 = enable +A8x indicators, 5 = disable +A8x (answer)
		  v8c,		// value of V.8 CI signal call function
		  v8b,		// 0 = disable V.8bis, 1 = DCE-controlled V.8bis, 2 = DTE-controlled V.8bis
          v8pf;     // ???, but we need it
//	PCHAR cfrange,	// not supported
//		  protrange;// not supported
} V8BIS_OPERATION_CONTROL, *PV8BIS_OPERATION_CONTROL;

typedef enum 
{ 
	V8BIS_FAILED, 
	V8BIS_SUCCESSFUL, 
	V8BIS_MODE_NOT_SUPPORTED
} V8BIS_STATE;

typedef enum 
{
    V8BIS_OK,
    V8BIS_NO_CRe,
    V8BIS_CRe_DETECTOR_FAILED,
    V8BIS_CRd_GENERATOR_FAILED,
    V8BIS_NO_CRd,
    V8BIS_NO_CL,
    V8BIS_NO_MS,
    V8BIS_NO_ACK_NAK,
    V8BIS_NAK,
    V8BIS_NO_CTS,
    V8BIS_NOT_SUPPORTED,
    V8BIS_UNDEFINED
} V8BIS_FAILURE_REASON;

typedef struct
{
    UINT8	    NS_FlexVersion;                     // NS - Flex Version Number
    UINT8	    NS_DSPVersion;                      // NS - DSP Version Number
	UINT8	    bRemoteIsConexant;                  // remote modem is Conexant
    UINT8	    bK56UseUlaw;                        // use u-law in K56 negotiation
    UINT8	    bK56flexNegotiated;                 // remote server supports K56
    UINT8	    bV90InNonStdFld;                    // v.90 support is detected in NSF
    UINT8	    bV90InStdFld;                       // v.90 support is detected in SF
    UINT8	    bV70_Enabled;
    V8BIS_STATE eState;
    V8BIS_FAILURE_REASON    eReason;
} V8BIS_RESULT, *PV8BIS_RESULT;

/* Structure passed with CFGMGR_RING_BURST */
typedef struct tagRingBurstParams {
    UINT16  m_nMinFreq;				// in Hz
    UINT16  m_nMaxFreq;				// in Hz
} RING_BURST_PARAMS, *PRING_BURST_PARAMS;

/* Structure passed with CFGMGR_ANSWER_TONE_DETECTOR/CFGMGR_BELL_TONE_DETECTOR */
typedef struct tagAnswerToneParams {
    UINT16  m_wCenterFrequency;
    UINT16  m_wBandwidth;
    short   m_nAmplitude;			// normed to 32768
} ANSWER_TONE_PARAMS, *PANSWER_TONE_PARAMS;

/* Structure passed with CFGMGR_DTMF_GENERATOR */
typedef struct tagDTMFGenParams {
	UINT16  m_wDigitDuration;		// in ms
	UINT16  m_wPauseDuration;		// in ms
	short	m_nHighBandAmplitude;	// normed to 32768
	short	m_nLowBandAmplitude;	// normed to 32768
} DTMF_GEN_PARAMS, *PDTMF_GEN_PARAMS;

// Unimodem Diagnostic defines
typedef enum
{
	UD_DISPLAY_FALSE,
	UD_DISPLAY_TRUE,
	UD_DISPLAY_REBOOT
} UD_DISPLAY;

typedef enum
{
	CauseUndefined			= 0x0,
	NoPreviousCall			= 0x1,
	CallInProgress			= 0x2,
	CallWaiting				= 0x3,
	Delayed					= 0x4,
	InacTimerExpired		= 0x19,
	DTRDrop					= 0x20,
	BlackListed				= 0x29,
	ExtOffhook				= 0x2B,
	S7Expired				= 0x2C,
	LoopCurrentInterrupted	= 0x2E,
	NoDialTone				= 0x2F,
	Voice					= 0x30,
	ReorderTone				= 0x31,
	CarrierLost				= 0x3C,
	TrainingFailed			= 0x3D,
	NoModinCommon			= 0x3E,
	RetrainFailed			= 0x3F,
	GSTNCleardown			= 0x41,
	FaxDetected				= 0x42,
	AnykeyAbort				= 0x50,
	ATH						= 0x51,
	ATZ						= 0x52,
	FrameReject				= 0x5A,
	NoECEstablished			= 0x5B,
	ProtViolation			= 0x5C,
	n400Exceeded			= 0x5D,
	NegotiatFailed			= 0x5E,
	DiscontFrame			= 0x5F,
	SabmeFrame				= 0x60,
	LostSync				= 0x64,
	DLG						= 0x65,    	// added for dlg support
    OverVoltage             = 0x7F
} CALL_TERMINATION_CAUSE;

typedef enum tagUD_CallSetupInfo {
    UD_CallSetupInfo_None = 0,
    UD_CallSetupInfo_NoDialTone,
    UD_CallSetupInfo_FastBusy,
    UD_CallSetupInfo_Busy,
    UD_CallSetupInfo_UnknownSignal,
    UD_CallSetupInfo_Voice,
    UD_CallSetupInfo_DataAns = 7,
    UD_CallSetupInfo_DataCall,
    UD_CallSetupInfo_FaxAns,
    UD_CallSetupInfo_FaxCall,
    UD_CallSetupInfo_V8bis 
    } UD_CALL_SETUP_INFO;


typedef enum tagUD_MultimediaMode {
    UD_MultimediaMode_DataOnly = 0x0,
    UD_MultimediaMode_FaxOnly,
    UD_MultimediaMode_VoiceOnly,
    UD_MultimediaMode_VoiceView,
    UD_MultimediaMode_DSVD = 0x8,
    UD_MultimediaMode_H324,
    UD_MultimediaMode_OtherV80
    } UD_MM_MODE;

typedef enum tagUD_DteDceModes {
    UD_DteDceModes_AsyncData = 0x0,
    UD_DteDceModes_V80TransSync,
    UD_DteDceModes_V80FramedSync,
    } UD_DTE_DCE_MODE;

typedef enum tagUD_Compression {
    UD_Compression_NONE = 0,
    UD_Compression_V42BIS = 1,
    UD_Compression_ALT = 80,
    } UD_COMPRESSION;

typedef enum tagUD_Protocol {
    UD_Protocol_NONE = 0,
    UD_Protocol_LAPM,
    UD_Protocol_ALT
    } UD_PROTOCOL;

/*****************************************************************
OEM & COUNTRY specific parameters, should be loaded from NVRAM
BEGIN
*****************************************************************/
// COUNTRY PARAMETRERS



#define MAX_INTERVALS				4
#define MAX_OEM_STR_LEN				80
// Number of S-registers limited by entries in the INF file
#define NUM_LIMITED_SREGS			10
// !!!NOTE: LAST_RELAY_ENTRY == DEVMGR_LAST_RELAY_ENTRY
#define LAST_RELAY_ENTRY			12



// Transmit Level
#pragma pack(1)
typedef struct tagTxlevel {
	UINT8	TxDataLevelMin;
	UINT8	TxDataLevelMax;
	UINT8	TxDataLevelDefault;
	UINT8	TxFaxLevelMin;
	UINT8	TxFaxLevelMax;
	UINT8	TxFaxLevelDefault;
	UINT8	TxVoiceLevelMin;
	UINT8	TxVoiceLevelMax;
	UINT8	TxVoiceLevelDefault;
	char	TxLvlAdj;							// can be + or -
	UINT8	LowDialLevel;
	UINT8	HighDialLevel;
	UINT8	MaxVTSLineSingleToneLevel;
} CntryTxlevelStructure;
#pragma pack()

// Pulse Dial Parameters
#pragma pack(1)
typedef struct tagPulse {
	UINT8	PulseDialMode;
	UINT8	PulseMapAmperP0;					//Maps the &P values
	UINT8	PulseMapAmperP1;
	UINT8	PulseMapAmperP2;
	UINT8	PulseMapAmperP3;
	char	PulseMakeOffset;					// can be +/-
	char	PulseBreakOffset;					// can be +/-
	UINT8	PulseSetupTime;
	UINT8	PulseClearTime;
	UINT16	PulseInterd;
	UINT16	LineMuteDelay;						//VRO: Line mute delay
} CntryPulseStructure;
#pragma pack()

// Ring Detection Parameters
#pragma pack(1)
typedef struct tagRing {
	UINT8	RingMaxFrequency;
	UINT8	RingMinFrequency;
	UINT16	RingStable;
	UINT16	MinTimeBeforeAnswering;
	UINT16	MinDelayBtwnRings;
	UINT16	MaxDelayBtwnRings;
    UINT16	MinRingOnTime;
    UINT16	MinImmediateRingOn;
    UINT16	MaxImmediateRingOn;
} CntryRingStructure;
#pragma pack()

// DTMF Dial Parameters
#pragma pack(1)
typedef struct tagDTMF {
	UINT16	DTMFOnTime;
	UINT16	DTMFInterdigit;   
} CntryDTMFStructure;
#pragma pack()

#pragma pack(1)
typedef struct FilterParamTAG {
	UINT16	FilterType;
	UINT16	Biquad1[5];
	UINT16	Biquad2[5];    
	UINT16	LpFBK;
	UINT16	LpGain;
	UINT16	ThreshU;
	UINT16	ThreshL;
	UINT16	Biquad1_PreF[5];
	UINT16	Biquad2_PreF[5];
	BOOL	SqDisState;
} FilterParam, *ptrFilterParam;
#pragma pack()

// Filter Parameters
#pragma pack(1)
typedef struct tagFilter {
	FilterParam		Primary;
	FilterParam		Alternate;
	FilterParam		VoiceToneACallProgress;
} CntryFilterStructure;
#pragma pack()

#pragma pack(1)
typedef struct FilterThreshold_Tag {
	UINT16	ThreshU;
	UINT16	ThreshL;
} FilterThresholdStructure;
#pragma pack()

#pragma pack(1)
typedef struct tagTHRESHOLD 
{
	FilterThresholdStructure DialThresh;
	FilterThresholdStructure AltDialThresh;
	FilterThresholdStructure WDialThresh;
	FilterThresholdStructure AltWDialThresh;
	FilterThresholdStructure ProgThresh;
	FilterThresholdStructure AltProgThresh;
	FilterThresholdStructure VoiceToneACallProgressThresh;
	UINT16 DTMFRxThresh;
	UINT16 RingThreshold;	//[Modif0021]LKS03jun98: Add soft ring threshold set to 0 if not use
	short int RingBalance;	//[Modif0021]LKS28jul98: Add soft ring threshold compensation slope, set to 0 if not use
} CntryThresholdStructure;
#pragma pack()

// RLSD Threshold Adjustment
typedef struct tagRLSD {
	UINT32		XrthOffset;				// can be + or -
} CntryRLSDStructure;

// Tone Detection Parameters
#pragma pack(1)
typedef struct tagTone {
	UINT16		DialStable;
	UINT16		WDialStable; 
	UINT16		AnswerStable;
	UINT16		ProgressStable;
	UINT8		DialtoneWaitTime;
    UINT16		PolarityReversalIgnoreTime;
} CntryToneStructure;
#pragma pack()

#pragma pack(1)
typedef struct tagCadPrm {
	struct {
		UINT32	lMin;
		UINT32	lMax;
	} Interval[MAX_INTERVALS];
	UINT32		lNumIntervalsNeeded;
} CadPrm;
#pragma pack()

// TONE CADENCE
#pragma pack(1)
typedef struct tagCadence {
	UINT8		ToneCadence_NumberOfPhases;
	CadPrm		DialtoneParams;
    CadPrm		WDialtoneParams;
	CadPrm		BusyParams;
	CadPrm		RingbackParams;
	CadPrm		CongestionParams;
} CntryCadenceStructure;
#pragma pack()

// Blacklisting FLAGS
#pragma pack(1)
typedef struct tagBL_Flags {
	char		fPermBlst: 1;
	char		fResetOnSuccess: 1;
	char		fDistictFailures: 1;
	char		fFlag0Spare3: 1;
	char		fFlag0Spare4: 1;
	char		fFlag0Spare5: 1;
	char		fFlag0Spare6: 1;
	char		fFlag0Spare7: 1;
} BLFlagStructure;
#pragma pack()

// Blacklisting Parameters
#pragma pack(1)
typedef struct tagBlacklisting {
	UINT8			BlstAction_NoDialTone;
	UINT8			BlstAction_KeyAbort;
	UINT8			BlstAction_Busy;
	UINT8			BlstAction_NoConnection;
	UINT8			BlstAction_NoAnswer;
	UINT8			BlstAction_VoiceAnswer;
	UINT16			BlackListingDelay1;			// InterCall Delay upon Failure when redialing same number
	UINT16			BlackListingDelay2;			// InterCall Delay upon Failure when redialing different number
	UINT16			BlackListingDelay3;			// Method 2 Delay
	UINT16			BlackListingDelay4;			// Method 3 Delay 
	UINT16			BlackListingCount1;			// Method 2 # of Failures 
	UINT16			BlackListingCount2;			// Method 3 # of Failures 
	BLFlagStructure	BlstFlags;
} CntryBlacklistingStructure;
#pragma pack()

// CallerID Parameter
#pragma pack(1)
typedef struct tagCallerID {
	UINT8		Cid_TYPE;
} CntryCallerIDStructure;
#pragma pack()

#pragma pack(1)
typedef struct 
{
  // Flag0
  int fAltDialToneFilter: 1;
  int fAltCallProgressFilter: 1;
  int fAltWToneFilter: 1;
  int fAltWAfterIntCodeFilter: 1;
  int fFlag0Spare4: 1;
  int fFlag0Spare5: 1;
  int fFlag0Spare6: 1;
  int fFlag0Spare7: 1;

  // Flag1
  int fEnforceDialTone: 1;
  int fEnforceCallingToneorCI: 1;
  int fEnforceGuardTone: 1;
  int fEnforceBusyTone: 1;
  int fUseS7whenW: 1;
  int fHangupOnCallWaiting: 1;
  int fFlag1Spare6: 1;
  int fFlag1Spare7: 1;

  // Flag2
  int fDisableATH1: 1;
  int fDisablePulseAfterTone: 1;
  int fAllowPulseDialing: 1;
  int fAllowDTMFabcd: 1;
  int fEnableBlacklisting: 1;
  int fLongToneIsBusy: 1;
  int fSelectUlaw: 1;
  int fNoBellMode: 1;

  // Flag3
  int fSwissComma: 1;		// Swiss approval do not want first comma in dial string
  int fDtmfCompensation: 1; // Use the DAA compesation table for DTMF Level.
  int fFlag3Spare2: 1;
  int fFlag3Spare3: 1;
  int fFlag3Spare4: 1;
  int fFlag3Spare5: 1;
  int fFlag3Spare6: 1;
  int fFlag3Spare7: 1;
} CALL_PROGRESS_FLAGS, *PCALL_PROGRESS_FLAGS;
#pragma pack()

// Agress speed Parameter
#pragma pack(1)
typedef struct tagAgressSpeedIndex {
	UINT8	cV90AgressSpeedIndex;
	UINT8	cK56AgressSpeedIndex;
	UINT8	cV34AgressSpeedIndex;
} CntryAgressSpeedIndexStructure;
#pragma pack()

#pragma pack(1)
typedef struct tagCtryPrmsStruct {
	UINT16							T35Code;							// T.35 Country Identification
	char							cInter[MAX_OEM_STR_LEN];			// Country Name
	char							cIntCode[MAX_OEM_STR_LEN];			// Country International Access code
	CntryTxlevelStructure			Txlevel;							// + Transmit Level
	UINT16							Relays[LAST_RELAY_ENTRY];			// + Relay Control Parameters
	CntryPulseStructure				Pulse;								// Pulse Dial Parameters
	RING_PARAMS						Ring;								// + Ring Detection Parameters
	UINT8							SRegLimits[NUM_LIMITED_SREGS][4];	// S Register Limiting
	CntryDTMFStructure				DTMF;								// + DTMF Dial Parameters
	CntryFilterStructure			Filter;								// + Filter Parameters
	CntryThresholdStructure			Threshold;							// + Thresholds Parameters
	CntryRLSDStructure				RLSD;								// + RLSD Threshold Adjustment
	CntryToneStructure				Tone;								// Tone Detection Parameters
	CALL_PROGRESS_TIMING_CONFIG		Timing;								// Timing Parameter
	CntryCadenceStructure			Cadence;							// TONE CADENCE
	CntryBlacklistingStructure		Blacklisting;						// Blacklisting Parameters
	CntryCallerIDStructure			CallerID;							// CallerId Control
	CALL_PROGRESS_FLAGS				Flags;								// + Flag Control
	CntryAgressSpeedIndexStructure	AgressSpeedIndex;					// + Agress Speed Parameter
} CtryPrmsStruct;
#pragma pack()

// OEM PARAMETERS

typedef struct tagOEM_Flags {
	// OEMFlag0
	int		fUseTIES: 1;
	int		fAnalogSpeaker: 1;
	int		fDataFaxRemoteTAM: 1;
	int		fDataFaxVoiceView: 1;
	int		fLCSPresent: 1;
	int		f3VoltIA: 1;
	int		fRemHangupPresent: 1;
	int		fSpkMuteHWPresent: 1;
	// OEMFlag1
	int		fReadCountryFromSwitch: 1;
	int		fLegacy: 1;						// Add fLegacy flags (ACF compatibility cmd)
	int		fPME_Enable: 1;					// enable D3 sleep and arming PME
	int		fAlternateConnectMSG: 1;
	int		fCountryNoneSelect: 1;			// Option whether to allow NONE in country select tab
	int		fSpkMuteHWPolarity: 1;
	int		fLocalSpkBoostTAM: 1;			// ON = Set IA Spk Output to maximum level, OFF = pickup default value 
	int		fMicSpkControl: 1;				// ON = allow +VGM, +VGS; OFF = not allow +VGM, +VGS & load DSVD_Default values
	// OEMFlag2
	int		fNEC_ACPI: 1;					// NEC special handling of APM/ACPI
	int		fDisForceLAPMWhenNoV8bis: 1;	// Disable forcing LAPM when V.8bis fails in V.90
	int		fDosSupportDisabled: 1;			// indicates if DOS support checkbox is toggled on/off (1=OFF)
	int		fDosCheckBoxSupport: 1;			// indicates if DOS support checkbox is shaded/unshaded (0=Shaded)
	int		fCIDHandsetMode: 1;				// ON = special local handset handling based on +VCID setting.
	int		fExtOffHookHWPresent: 1;		// ON = hardware is able to detect extension off hook
	int		fLAN_Modem_Support: 1;			// ON = customize for lan/modem
	int		fPortCloseConnected: 1;			// ON = maintain modem connection on PortClose
	// OEMFlag3
	int		fFullGPIO: 1;					// ON=full control to GPIO with AT-GPIO command
	int		fD3CIDCapable: 1;				// does DAA support d3 caller ID circuitry
	int		fLineInUsed: 1;					// When 1 enable customized line in used detection before dialing
	int		fV42ThrottleDisable: 1;			// When one, disable V.42 Rx throttle
	int		fxDSLCombo: 1;					// xDSL modem only, 1 = combo board
	int		fV23AnswerDis: 1;
	int		fDialSkipS6: 1;
	int		fFxCallCadFlag: 1;
} OEMFlagStructure;

typedef struct tagOEM_Filter {
	FilterParam		Tone1100;
	FilterParam		V8bToneC2100;
	FilterParam		V8bToneB2225;
	FilterParam		VoiceToneB1100;
	FilterParam		VoiceToneC1300;
	FilterParam		VoiceToneC2100;
	FilterParam		VoiceToneB2225;
    FilterParam     ToneB_2002;
} OEMFilterStructure;

typedef struct tagModulation {
	UINT16		Org;
	UINT16		Ans;
} Modulation;

typedef struct tagOEM_Threshold {
	Modulation		V21;
	Modulation		V21fax;
	Modulation		V23;
	Modulation		V22;
	Modulation		V22b;
	Modulation		V32;
	Modulation		V32b;
	Modulation		V34;
	Modulation		K56;
	Modulation		V90;
	Modulation		V27;
	Modulation		V29;
	Modulation		V17;
} OEMThresholdStruct;

typedef struct tagPROFILE_DATA
{
    UINT8       Echo;          // ATE 	CFGMGR_DTE_ECHO
    UINT8       Volume;        // ATL 	CFGMGR_SPEAKER_VOLUME
    UINT8       Speaker;       // ATM 	CFGMGR_SPEAKER_CONTROL
    UINT8   	Pulse;         // ATT/ATD 1-PULSE, 0-TONE 	CFGMGR_DIAL_MODE

    UINT8       Quiet;         // ATQ	CFGMGR_DTE_CONFIG.bQuiet
    UINT8       Verbose;       // ATV	DTE_USER_CONFIG.bVerbose
    UINT8       Level;         // ATX	DTE_USER_CONFIG.bLevel
    UINT8       Connect;

    UINT8       AmperC;     // CFGMGR_RLSD_BEHAVIOR
    UINT8       AmperD;     // CFGMGR_DTR_BEHAVIOR
    UINT8       AmperG;     // ??? not stored yet
    UINT8       AmperT;     // ??? not implemented yet

    UINT8       S0;         // Sregisters CFGMGR_SREG + <number>
    UINT8       S1;
    UINT8       S2;
    UINT8       S3;
    UINT8       S4;
    UINT8       S5;
    UINT8       S6;
    UINT8       S7;
    UINT8       S8;
    UINT8       S10;
    UINT8       S12;
    UINT8       S16;
    UINT8       S18;
    UINT8       S29;

} PROFILE_DATA, *PPROFILE_DATA;


typedef struct tagPOUND_UD_DATA
{
    UD_DISPLAY          cUDDisplay;         //Flag used to detemine if to display #UD/#UG   Done
	UD_CALL_SETUP_INFO  cUDCallSetup;		//0x1, Table 2                                  Done
	UD_MM_MODE          cUDMultMediaMode;   //0x2, Table 3                                  Done
	UD_DTE_DCE_MODE     cUDDTEDCEmode;      //0x3, Table 4

	char    sUDV8CM[20];					// 0x4 V8_CM                                    Done
	char    sUDV8JM[20];					// 0x5 V8_JM                                    Done

	UINT8   cUDRXdb;						//0x10, 2 Digits in Hex                         Done
	UINT8   cUDTXdb;						//0x11, 2 Digits in Hex                         Done
	UINT8   cUDSNratio;						//0x12, 2 Digits in Hex                         Done
	UINT8   cUDMSE;							//0x13 Normalized Mean Squared error
	UINT8   cUDNearEchoLoss;				//0x14 near echo loss
	UINT8   cUDFarEchoLoss;					//0x15 far echo loss
	UINT16  wUDFarEchoDelay;				//0x16 far echo delay
	UINT8   cUDRoundTripDelay;				//0x17 round trip delay
    UINT32  cV34InfoBits;                   //0x18 status 
   
	UINT8   cUDTXCarrierNegotiation;		//0x20, Table 6                                 Done
	UINT8   cUDRXCarrierNegotiation;		//0x21, Table 6                                 Done
	UINT16  wUDTXSymbol;					//0x22, 4 Digits in Hex                         Done
	UINT16  wUDRXSymbol;					//0x23, 4 Digits in Hex                         Done
	UINT16  wUDTXCarrierFreq;				//0x24, 4 Digits in Hex                         Done
	UINT16  wUDRXCarrierFreq;				//0x25, 4 Digits in Hex                         Done
	UINT16  wUDFirstTXrate;					//0x26, 4 Digits in Hex                         Done
	UINT16  wUDFirstRXrate;					//0x27, 4 Digits in Hex                         Done

	UINT8   cUDCarrierLossCount;			//0x30, 2 Digits in Hex                         Done
    UINT8   cUDRateRenegReqCount;           //0x31, 2 Digits in Hex, (half)                 Done
    UINT8   cUDRateRenegDetCount;           //0x31, 2 Digits in Hex, (half)                 Done
    UINT8   cUDRetrainReqCount;             //0x32, 2 digits in Hex                         Done

    UINT32  dwEqmAboveRtrnThreshold;
    UINT32  dwEqmAboveRenegThreshold;
    UINT32  dwEqmBelowRenegThreshold;
    UINT32  dwBlockErrors;
    UINT32  dwRtrnCounterExceeded;
    UINT32  dwRtrnAsThirdReneg;
	UINT32	dwInvalidSymbolRate;
	UINT32	dwInvalidMsIndex;
    UINT8   cUDRetrainDetCount;             //0x33  2 digits, Retrains granted              Done
    UINT16  wUDLastTXrate;                  //0x34, 4 digits in Hex                         Done
    UINT16  wUDLastRXrate;                  //0x35, 4 digits in Hex                         Done

    UD_PROTOCOL     cUDProtocolNegotiation;     //0x40, table 4                             Done
    UINT16  wUDMaxFrameSize;                    //0x41, 3 digits
    UINT8   cUDECLinkTimeout;                   //0x42, 1 digits
    UINT8   cUDECLinkNAK;                       //0x43, 1 digits
    UD_COMPRESSION  cUDCompressionNegotiation;  //0x44, table 5                             Done
    UINT16  cUDCompressionDictSize;             //0x45, 4 digits                            Done

    UINT8   cUDTXFlowControl;               //0x50, 1 digits                                Done
    UINT8   cUDRXFlowControl;               //0x51, 1 digits                                Done
    UINT32  lUDTXCharFromDTE;               //0x52, 8 digits                                Done
    UINT32  lUDRXCharToDTE;                 //0x53, 8 digits                                Done
    UINT16  wUDTXCharLost;                  //0x54, 4 digits                                ---
    UINT16  wUDRXCharLost;                  //0x55, 4 digits                                ---
    UINT32  lUDTXFrameCount;                //0x56, 8 digits                                Done ?
    UINT32  lUDRXFrameCount;                //0x57, 8 digits                                Done ?
    UINT32  lUDTXFrameErrorCount;           //0x58, 8 digits                                Done ?
    UINT32  lUDRXFrameErrorCount;           //0x59, 8 digits                                Done ?

    CALL_TERMINATION_CAUSE cUDTermination;  //0x60 Tables 8-9                               In process

    UINT8   bHighPassDpFilterOn;            //                                              Done
    UINT8   bRBSpattern;                    //                                              Done
    UINT8   bPadDetected;                   //                                              Done
    UINT16  wDigitalLossEstimate;           //                                              Done
    UINT8   bDualPCMDetected;               //                                              Done
    UINT32  wTrainingCount;                 //                                              Done
    UINT32  dwTrainingEqmSum;               //                                              Done
    short   EqmValue;                       //                                              Done
    UINT8   cUDV34FBCause;                  //                                              Done

    } POUND_UD_DATA, *PPOUND_UD_DATA;


#define SET_POUNDUD_FIELD(pObj, FieldName, FieldValue) (pObj->FieldName = FieldValue)
#define SET_POUNDUD_STRING(pObj, FieldName, FieldValue) OsStrCpy(pObj->FieldName, FieldValue)

typedef enum {
    PERCTT_INFO,
    PERCTT_DTMF,
    PERCTT_FSK,
    PERCTT_QAM,
    PERCTT_MISC,
    PERCTT_FAX,
    PERCTT_V34,
    } PERCTT_TYPE;

typedef struct tagPercTT_Parms
{
    PERCTT_TYPE eType;
    UINT32      x;
    UINT32      y;
    UINT32      z;
} PERCTT_PARAMS, *PPERCTT_PARAMS;


/*****************************************************************
OEM & COUNTRY specific parameters, should be loaded from NVRAM
END
*****************************************************************/

#endif      /* #ifndef __CONFIGCODES_H__ */
