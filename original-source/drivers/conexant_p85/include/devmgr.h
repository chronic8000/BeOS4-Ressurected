/****************************************************************************************
*                     Version Control Information										*
*                                                                                       *
* $Header:   P:/d942/octopus/archives/Include/DevMgr.h_v   1.10   29 Jun 2000 12:19:08   beliloi  $
* 
*****************************************************************************************/


/****************************************************************************************

File Name:			DevMgr.h

File Description:	Device Manager enumerated types, data structures, and prototypes.

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


#ifndef _DEVMGR_H
#define _DEVMGR_H

#include "intfctrl_ex.h"
#include "pim.h"
#include "dm_stream.h"
#include "dpfax.h"

#define DEVMGR_VERSION		1

/*******************************************************************

DEVICE MANAGER DATA TYPES

*******************************************************************/

// See DevMgr_Configure function
typedef enum {
	DEVMGR_ASSIGN_HW_RES
}  eConfigureCode;

// See DevMgr_Monitor function
typedef enum {
	DEVMGR_TRAINING_EQM_SUM,				// AT#UD, AT#UG
	DEVMGR_DIGITAL_LOSS_ESTIMATE,			// go to connected state, V90, read digital loss
	DEVMGR_GET_TRANSMIT_OUTPUT_LEVEL,		// AT#UD
	DEVMGR_GET_TRAIN_FLEX_SKIP_V8,			// ???
	DEVMGR_HIGH_POWER_SERVER,				// LMON
	DEVMGR_RATEMASK,						// LMON
	DEVMGR_TX_BITRATE,
	DEVMGR_RX_BITRATE,
	DEVMGR_RX_BITRATE_EFF,					// K56
	DEVMGR_EQM,								// LMON, get EQM
	DEVMGR_RXAGC,							// LMON, get AGC gain word (to calculate receive level)
	DEVMGR_RX_CARRIER,						// AT#UD
	DEVMGR_SNR,								// signal to noise ratio
	DEVMGR_MODULATION,
	DEVMGR_TX_SYMBOLRATE,
	DEVMGR_RX_SYMBOLRATE,
	DEVMGR_RBSMASK,							// AT#UD, AT#UG
	DEVMGR_PADDETECT,						// AT#UD, AT#UG
	DEVMGR_RECEIVE_AGC_AUTO_GAIN,			// LMON, get auto gain
	DEVMGR_GET_DAA_STATE,
	DEVMGR_STATUSMGR_CLIENT,				// Device Manager as a client of Status Manager
	DEVMGR_FLEX_CTLDOWNLOADVERSION,
	DEVMGR_DUAL_PCM_DETECTED,				// AT#UD, AT#UG
	DEVMGR_HIGH_PASS_FILTER_ENABLED,		// AT#UD, AT#UG
	DEVMGR_TRAINING_COUNT,					// AT#UD, AT#UG
	DEVMGR_V34FBCAUSE,						// AT#UD, AT#UG
	DEVMGR_QUERY_DEBUG,
} eMonitorCode;

// See DevMgr_Control function
typedef enum {
	DEVMGR_ENABLE_FIFO,
	DEVMGR_DISABLE_FIFO,
	DEVMGR_ENABLE_COMPR_EQUALIZER,			// V8bis
	DEVMGR_DISABLE_COMPR_EQUALIZER,			// connect, if V23
	DEVMGR_SET_TRANSMIT_LEVEL,				// DPISetTransmitLevel
	DEVMGR_SET_CALL_MODE,					// TRUE-Originate, FALSE-Answer
	DEVMGR_CONTROLLER_MODE,					// fax, voice, EC, EC_NONE, etc.
	DEVMGR_CTRL_UPDATE_EC_ERR_CNT,			// parameter requested by LMON
	DEVMGR_CTRL_ATH_IN_PROGRESS,			// parameter requested by LMON
	DEVMGR_CTRL_CLOSE_CONNECTION,			// for ATH
	DEVMGR_SET_DAA_STATE,					// see SetDAARelay function
	DEVMGR_ACTIVATE_LMON,
	DEVMGR_DEACTIVATE_LMON,
	DEVMGR_WAIT_DP_TX_BUFF_EMPTY,
	DEVMGR_SET_DEBUG,
	DEVMGR_SET_V21_CONFIGURATION,			// HCF only, Param != 0 --> V.21 Channel 2
											// Param == 0 --> V.21
	DEVMGR_SET_RLSD_DROP_OUT_TIMER,			// HCF only, Param - RLSD drop out timer
	DEVMGR_UPDATE_COUNTRY
} eControlCode;

// See DevMgr_Control function + DEVMGR_SET_DAA_STATE control code
typedef enum {
	DEVMGR_GPIO_RELAY_MASK,
	DEVMGR_GPIO_DEFAULT_STATE,
	DEVMGR_OFFHOOK_PHONETOLINE,
	DEVMGR_OFFHOOK_PHONEOFFLINE,
	DEVMGR_ONHOOK_PHONETOLINE_CALLID,
	DEVMGR_ONHOOK_PHONETOLINE_NOCALLID,
	DEVMGR_ONHOOK_PHONEOFFLINE_CALLID,
	DEVMGR_ONHOOK_PHONEOFFLINE_NOCALLID,
	DEVMGR_OFFHOOK_PULSE_MAKE,
	DEVMGR_OFFHOOK_PULSE_BREAK,
	DEVMGR_OFFHOOK_PULSESETUP,
	DEVMGR_OFFHOOK_PULSECLEAR,
	DEVMGR_LAST_RELAY_ENTRY
} DAA_RELAY_CODE;

// See DevMgr_Control function + DEVMGR_CONTROLLER_MODE control code
typedef enum {
	Data_Mode,
	Fax_Mode,
	Voice_Mode,
	Test_Mode
} eControllerMode;

// See DevMgr_ConfigureStreamObject function
typedef enum {
	DEVMGR_INIT_STREAM,			// for HDLC stream a CRC (dwParam = 16) must be applied
	DEVMGR_CONNECT_STREAM
} eStreamConfigCode;

// See DevMgr_PMControl function
typedef enum {
	DEVMGR_SetACPIState,
	DEVMGR_EnableRingWake,
	DEVMGR_DisablePMESTS,
	DEVMGR_PMESupportMask,
	DEVMGR_EnableD3CID
} ePmeFunction;

// See DevMgr_PMControl function
typedef enum {
	DEVMGR_D0,
	DEVMGR_D1,
	DEVMGR_D2,
	DEVMGR_D3
} ePmeState;

// See DevMgr_SpeakerControl function
typedef enum {
	DEVMGR_SET_SPEAKER_INPUT,
	DEVMGR_DISABLE_SPEAKER,
	DEVMGR_ENABLE_SPEAKER,
	DEVMGR_DISCONNECT_SPEAKER_FROM_DP,
	DEVMGR_CONNECT_SPEAKER_TO_DP
} eSpeakerControlType;

// See DevMgr_MaskDataTransfer & DevMgr_PurgeData functions
typedef enum {
	DEVMGR_DIR_RX = 0x1,
	DEVMGR_DIR_TX = 0x2,
	DEVMGR_DIR_ALL = 0x3
} eDirection;

// See DevMgr_BeginCtrlNotification function
typedef enum {
	DEVMGR_POLARITY_OFF,
	DEVMGR_POLARITY_ON,
	DEVMGR_POLARITY_ANY
} ePolarity;

// See DevMgr_CreateToneDetector
typedef enum {
	TONE_PRIMARY,				// 0x0
	TONE_ALTERNATE,
	TONE_ATBEL,
	TONE_ATV25,
	TONE_1100,
	TONE_2130,
	TONE_2312,
	TONE_2750,
	TONE_2912,
	VOICETONEACALLPROG,
	VOICETONEB_1100,
	VOICETONEC_1300,
	VOICETONEC_2100,
	VOICETONEB_2225,
	V8BTONE_2100,				// 0xE
	V8BTONE_2225,				// 0xF
	CI_CENTER_TONEB,
	CI_SIDE_TONEC,
	VOICEVIEW_TONEC_2912,
	VOICEVIEW_TONEB_2312,
	VOICEVIEW_TONEB_2750,
	VOICEVIEW_TONEA_2130,
	VOICEVIEW_TONEA_NOT_ST,
	TONEC_1650,
	CReSEG1_2002
} eTONEDETECT_TYPE;

// See DevMgr_SetupCallingTone function
typedef enum {
	DEVMGR_TONE_SILENCE,
	DEVMGR_TONE_CNG,
	DEVMGR_TONE_CI,
	DEVMGR_TONE_V25CALL
} eCALLING_TONE;

// See DevMgr_SetupCallingTone function
// TONEGEN - contains parameters for tone generator.
typedef struct tagTONEGEN {
	UINT32		OnTime;
	UINT32		OffTime;
} TONEGEN, *PTONEGEN;

// The following struct include the parameters for DTMF generator.
typedef struct tagDTMFParamsOut {
	UINT32			Duration;			// millisec
	UINT32			InterDigitDelay;	// millisec
	long			LowBandPower;		// dBm
	long			HighBandPower;		// dBm
} DTMF_PARAMS_OUT, *PDTMF_PARAMS_OUT;

// See DevMgr_SetDialMode function
typedef enum {
	DEVMGR_DIAL_TONE,
	DEVMGR_DIAL_PULSE
} eDialMode;

// See DevMgr_DialDigitAsync function
typedef enum { 
	DIGIT_0,
	DIGIT_1,
	DIGIT_2,
	DIGIT_3,
	DIGIT_4,
	DIGIT_5,
	DIGIT_6,
	DIGIT_7,
	DIGIT_8,
	DIGIT_9,
	DIGIT_STAR,
	DIGIT_POUND,
	DIGIT_A,
	DIGIT_B,
	DIGIT_C,
	DIGIT_D
} eDialDigit;

// see DevMgr_V8bisStartTone
typedef enum {
	MRd_TONE_INIT =		0x21,	// Mode Request-d (Initiating)
	CRe_TONE = 			0x22,	// Initiating 1375+2002+400, Capabilities Request-e (Initiating)
	CRd_TONE_INIT =		0x23,	// Capabilities Request (Initiating)
	ESi_TONE =			0x24,
	CRd_TONE = 			0x33,	// Responding 1529+2225+1900, Capabilities Request-d (Responding)
	ESr_TONE = 			0x34,
	VALID_SEG1_TONE =	0xE0
} eV8BIS_TONE_ID;

typedef struct tagMODULATIOM_PARAMS {
	MODULATION_TYPE	ModType;
	BOOL			bAutoRate;
	UINT32			dwMinTxSpeed;
	UINT32			dwMaxTxSpeed;
	UINT32			dwMinRxSpeed;
	UINT32			dwMaxRxSpeed;
	BOOL			buLaw;
	BOOL			bV8bisK56Neg;
	BOOL			bAllowV8;
} MODULATIOM_PARAMS, *PMODULATIOM_PARAMS;

// See V.8 API
typedef struct tagV8Config {
	// bit 0 - 
	// bit 1 - No Tx of ANSam until CI detected
	// bit 2 - Transmit CI
	// bit 3 - 
	// bit 4 - Send GSTN Octet
	// bit 5, 6, 7 - Call Function to be Transmitted
	UINT8	cV8CtrlMask0;
	// bit 0 - Send Protocol Octet
	// bit 1 - Send Minimum JM Sequence
	// bit 2 - Cellular Access
	// bit 3 - No RLSD in V.8
	// bit 4 - 
	// bit 5, 6, 7 - Protocol Bits
	UINT8	cV8CtrlMask1;
} V8Config;

typedef struct tagV8Status {
	// bit 0 - Min CI Sent
	// bit 1 - CI Detected
	// bit 2 - CM Detected
	// bit 3 - CJ Detected
	// bit 4 - JM Detected
	// bit 5 - ANS Detected
	// bit 6 - FSK Byte Ready
	// bit 7 - 
	UINT8	cV8StatMask0;
	// bit 0 - No Modes Common
	// bit 1 - 
	// bit 2 - 
	// bit 3 - ANSam Detected
	// bit 4 - Protocol Octet Received
	// bit 5, 6, 7 - Received Call Function
	UINT8	cV8StatMask1;
	// bit 0 - Received GSTN Octet
	// bit 1 - Remote is using Cellular
	// bit 2 - 
	// bit 3 - 
	// bit 4 - 
	// bit 5, 6, 7 - Received Protocol Bits
	UINT8	cV8StatMask2;
} V8Status;

// See DevMgr_BeginRetrain function
typedef enum {
	DEVMGR_LMON_RETRAIN,
	DEVMGR_LMON_FAST_RETRAIN,
	DEVMGR_LMON_RATE_RENEG,
	DEVMGR_LMON_CLEARDOWN
} eRetrainType;

typedef enum {
	LMON_INACTIVE, 
	LMON_ACTIVE, 
	LMON_RETRAINING, 
	LMON_RRUPSTATE, 
	LMON_RRDOWNSTATE,
	LMON_REMOTERETRAINING, 
	LMON_REMOTERENEGOTIATING, 
	LMON_CARRIERLOST
} LMON_STATE;

/*******************************************************************

DEVICE MANAGER API

*******************************************************************/

// This function returns device manager version
UINT32		DevMgr_GetInterfaceVersion(void);
// This function creates device manager instance (hDevMgr)
HANDLE		DevMgr_Create(PSYSTEM_INSTANCES_T pAccess);
// This function deletes device manager instance (hDevMgr)
COM_STATUS	DevMgr_Destroy(HANDLE hDevMgr);
COM_STATUS	DevMgr_Open(HANDLE hDevMgr);
COM_STATUS	DevMgr_Close(HANDLE hDevMgr);
// dwParam - dwHwID (for Windows 95/98 this parameter is device node)
COM_STATUS	DevMgr_Configure(HANDLE hDevMgr, eConfigureCode ConfigureCode, void* pParam);
COM_STATUS	DevMgr_Monitor(HANDLE hDevMgr, eMonitorCode MonitorCode, void* pMonitor, UINT32 dwSize);
COM_STATUS	DevMgr_Control(HANDLE hDevMgr, eControlCode ControlCode, void* pParam);

/*******************************************************************

STREAM OBJECT API

*******************************************************************/

HANDLE		DevMgr_CreateStreamObject(HANDLE hDevMgr, eStreamType StreamType);
COM_STATUS	DevMgr_DestroyStreamObject(HANDLE hDevMgr, HANDLE hStream);
COM_STATUS	DevMgr_ConfigureStreamObject(HANDLE hDevMgr, HANDLE hStream, 
										 eStreamConfigCode StreamConfigCode, void* pParam);

COM_STATUS	DevMgr_PMControl(HANDLE hDevMgr, ePmeState PmeState);
COM_STATUS	DevMgr_SpeakerControl(HANDLE hDevMgr, eSpeakerControlType SpeakerControlType, UINT8 cValue);

COM_STATUS	DevMgr_MaskDataTransfer(HANDLE hDevMgr, eDirection Direction, BOOL bEnable);
COM_STATUS	DevMgr_PurgeData(HANDLE hDevMgr, eDirection Direction);

/**************************************************************************************

RING DETECTOR API

**************************************************************************************/

COM_STATUS	DevMgr_SetRingDetect(HANDLE hDevMgr);
COM_STATUS	DevMgr_CancelRingDetect(HANDLE hDevMgr);

/**************************************************************************************

ASYNC NOTIFICATION API

**************************************************************************************/

// dwStatCode - status to monitor, ... + DevMgr_WaitDataTransferFinish (buffers + TDBE)
// Polarity - ON, OFF, ANY
COM_STATUS	DevMgr_BeginCtrlNotification(HANDLE hDevMgr, HANDLE StatCode, ePolarity Polarity);
COM_STATUS	DevMgr_EndCtrlNotification(HANDLE hDevMgr, HANDLE StatCode);

/**************************************************************************************

TONE DETECTOR API

**************************************************************************************/

HANDLE		DevMgr_CreateToneDetector(HANDLE hDevMgr, eTONEDETECT_TYPE ToneDetectId);
COM_STATUS	DevMgr_StartToneDetector(HANDLE hDevMgr, HANDLE hTone);
COM_STATUS	DevMgr_StopToneDetector(HANDLE hDevMgr, HANDLE hTone);
COM_STATUS	DevMgr_DeleteToneDetector(HANDLE hDevMgr, HANDLE hTone);

/**************************************************************************************

TONE GENERATOR API
It's a part of V.8 bis API, except for DevMgr_SetupDTMFGenerator

**************************************************************************************/

COM_STATUS	DevMgr_SetupCallingTone(HANDLE hDevMgr, eCALLING_TONE CallingToneID, PTONEGEN pParams);
COM_STATUS	DevMgr_StartCallingTone(HANDLE hDevMgr);
COM_STATUS	DevMgr_StopCallingTone(HANDLE hDevMgr);
COM_STATUS	DevMgr_SetupDTMFGenerator(HANDLE hDevMgr, PDTMF_PARAMS_OUT pDTMFParmsOut);

/**************************************************************************************

DIALER API
also see DevMgr_SetupDTMFGenerator function

**************************************************************************************/

COM_STATUS	DevMgr_SetDialMode(HANDLE hDevMgr, eDialMode DialMode);
COM_STATUS	DevMgr_DialDigitAsync(HANDLE hDevMgr, eDialDigit digit);
COM_STATUS	DevMgr_CancelDialMode(HANDLE hDevMgr);

/*******************************************************************

V.8 BIS

*******************************************************************/

COM_STATUS	DevMgr_V8bisSet(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisStartToneDetector(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisStopToneDetector(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisStartTone(HANDLE hDevMgr, eV8BIS_TONE_ID ToneId);
// Stop calling tone after receiving CRe
COM_STATUS	DevMgr_V8bisStopTone(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisSetupV21(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisSetupMode(HANDLE hDevMgr, BOOL bInitiator);
COM_STATUS	DevMgr_V8bisSendMsg(HANDLE hDevMgr);
COM_STATUS	DevMgr_V8bisTxOff(HANDLE hDevMgr);
// V8bis finished
COM_STATUS	DevMgr_V8bisCancel(HANDLE hDevMgr, BOOL bV8bisSuccessful);

/*******************************************************************

MS & V.8 configuration API

*******************************************************************/

COM_STATUS	DevMgr_ConfigureMsParams(HANDLE hDevMgr, MODULATIOM_PARAMS* pModParam);
COM_STATUS	DevMgr_V8SetConfig(HANDLE hDevMgr, V8Config* pV8ConfParams, BOOL bV90Enabled);
COM_STATUS	DevMgr_V8GetConfig(HANDLE hDevMgr, V8Config* pV8ConfParams);
COM_STATUS	DevMgr_V8GetStatus(HANDLE hDevMgr, V8Status* pV8StatParams);
COM_STATUS	DevMgr_ActivateMsParams(HANDLE hDevMgr);

/*******************************************************************

LMON "STUB"

*******************************************************************/

COM_STATUS	DevMgr_UpdateRateMask(HANDLE hDevMgr);
COM_STATUS	DevMgr_SetAutoRateControl(HANDLE hDevMgr, UINT8 cFlag);
COM_STATUS	DevMgr_BeginRetrain(HANDLE hDevMgr, UINT32 dwRate, eRetrainType RetrainType);
COM_STATUS	DevMgr_ClearRemoteRetrainDetect(HANDLE hDevMgr);

/*******************************************************************

STREAM OBJECT API

*******************************************************************/

HANDLE		DevMgr_CreateFax(HANDLE hDevMgr);
COM_STATUS	DevMgr_DestroyFax(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	DevMgr_StartFaxAnswerTone(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	DevMgr_StopFaxAnswerTone(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	DevMgr_ConfigureFaxParams(HANDLE hDevMgr, HANDLE hDpFaxObject, FAX_MOD_PARAMS* pFaxModParam);
COM_STATUS	DevMgr_ActivateFaxParams(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	DevMgr_FaxSilence(HANDLE hDevMgr, HANDLE hDpFaxObject);

/*******************************************************************

DEVICE MANAGER INTERFACE (FUNCTION TABLE)

*******************************************************************/

typedef UINT32		(*DEVMGRGetInterfaceVersion)(void);
typedef HANDLE		(*DEVMGRCreate)(PSYSTEM_INSTANCES_T);
typedef COM_STATUS	(*DEVMGRDestroy)(HANDLE);
typedef COM_STATUS	(*DEVMGROpen)(HANDLE);
typedef COM_STATUS	(*DEVMGRClose)(HANDLE);
typedef COM_STATUS	(*DEVMGRConfigure)(HANDLE, eConfigureCode, void*);
typedef COM_STATUS	(*DEVMGRMonitor)(HANDLE, eMonitorCode, void*, UINT32);
typedef COM_STATUS	(*DEVMGRControl)(HANDLE, eControlCode, void*);

typedef HANDLE		(*DEVMGRCreateStreamObject)(HANDLE, eStreamType);
typedef COM_STATUS	(*DEVMGRDestroyStreamObject)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRConfigureStreamObject)(HANDLE, HANDLE, eStreamConfigCode, void*);

typedef COM_STATUS	(*DEVMGRPMControl)(HANDLE, ePmeState);
typedef COM_STATUS	(*DEVMGRSpeakerControl)(HANDLE, eSpeakerControlType, UINT8);
typedef COM_STATUS	(*DEVMGRMaskDataTransfer)(HANDLE, eDirection, BOOL);
typedef COM_STATUS	(*DEVMGRPurgeData)(HANDLE, eDirection);
typedef COM_STATUS	(*DEVMGRSetRingDetect)(HANDLE);
typedef COM_STATUS	(*DEVMGRCancelRingDetect)(HANDLE);
typedef COM_STATUS	(*DEVMGRBeginCtrlNotification)(HANDLE, HANDLE, ePolarity);
typedef COM_STATUS	(*DEVMGREndCtrlNotification)(HANDLE, HANDLE);
typedef HANDLE		(*DEVMGRCreateToneDetector)(HANDLE, eTONEDETECT_TYPE);
typedef COM_STATUS	(*DEVMGRStartToneDetector)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRStopToneDetector)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRDeleteToneDetector)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRSetupCallingTone)(HANDLE, eCALLING_TONE, PTONEGEN);
typedef COM_STATUS	(*DEVMGRStartCallingTone)(HANDLE);
typedef COM_STATUS	(*DEVMGRStopCallingTone)(HANDLE);
typedef COM_STATUS	(*DEVMGRSetupDTMFGenerator)(HANDLE, PDTMF_PARAMS_OUT);
typedef COM_STATUS	(*DEVMGRSetDialMode)(HANDLE, eDialMode);
typedef COM_STATUS	(*DEVMGRDialDigitAsync)(HANDLE, eDialDigit);
typedef COM_STATUS	(*DEVMGRCancelDialMode)(HANDLE);

typedef COM_STATUS	(*DEVMGRV8bisSet)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisStartToneDetector)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisStopToneDetector)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisStartTone)(HANDLE, eV8BIS_TONE_ID);
typedef COM_STATUS	(*DEVMGRV8bisStopTone)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisSetupV21)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisSetupMode)(HANDLE, BOOL);
typedef COM_STATUS	(*DEVMGRV8bisSendMsg)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisTxOff)(HANDLE);
typedef COM_STATUS	(*DEVMGRV8bisCancel)(HANDLE, BOOL);

typedef COM_STATUS	(*DEVMGRConfigureMsParams)(HANDLE, MODULATIOM_PARAMS*);
typedef COM_STATUS	(*DEVMGRV8SetConfig)(HANDLE, V8Config*, BOOL);
typedef COM_STATUS	(*DEVMGRV8GetConfig)(HANDLE, V8Config*);
typedef COM_STATUS	(*DEVMGRV8GetStatus)(HANDLE, V8Status*);
typedef COM_STATUS	(*DEVMGRActivateMsParams)(HANDLE);

typedef COM_STATUS	(*DEVMGRUpdateRateMask)(HANDLE);
typedef COM_STATUS	(*DEVMGRSetAutoRateControl)(HANDLE, UINT8);
typedef COM_STATUS	(*DEVMGRBeginRetrain)(HANDLE, UINT32, eRetrainType);
typedef COM_STATUS	(*DEVMGRClearRemoteRetrainDetect)(HANDLE);

typedef HANDLE		(*DEVMGRCreateFax)(HANDLE);
typedef COM_STATUS	(*DEVMGRDestroyFax)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRStartFaxAnswerTone)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRStopFaxAnswerTone)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRConfigureFaxParams)(HANDLE, HANDLE, FAX_MOD_PARAMS*);
typedef COM_STATUS	(*DEVMGRActivateFaxParams)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRFaxSilence)(HANDLE, HANDLE);
typedef COM_STATUS	(*DEVMGRStartLoopbackTest)(HANDLE);  //pq
typedef COM_STATUS	(*DEVMGRStopLoopbackTest)(HANDLE);  //pq

typedef struct tagI_DEVICE_MGR_T {
	DEVMGRGetInterfaceVersion			GetInterfaceVersion;		// +
	DEVMGRCreate						Create;						// +
	DEVMGRDestroy						Destroy;					// +
	DEVMGROpen							Open;						// +
	DEVMGRClose							Close;						// +
	DEVMGRConfigure						Configure;					// +
	DEVMGRMonitor						Monitor;					// +
	DEVMGRControl						Control;					// +
	DEVMGRCreateStreamObject			CreateStreamObject;			// +
	DEVMGRDestroyStreamObject			DestroyStreamObject;		// +
	DEVMGRConfigureStreamObject			ConfigureStreamObject;		// +
	DEVMGRPMControl						PMControl;					// -
	DEVMGRSpeakerControl				SpeakerControl;				// +
	DEVMGRMaskDataTransfer				MaskDataTransfer;			// +
	DEVMGRPurgeData						PurgeData;					// +
	DEVMGRSetRingDetect					SetRingDetect;				// +
	DEVMGRCancelRingDetect				CancelRingDetect;			// +
	DEVMGRBeginCtrlNotification			BeginCtrlNotification;		// +
	DEVMGREndCtrlNotification			EndCtrlNotification;		// +
	DEVMGRCreateToneDetector			CreateToneDetector;			// +
	DEVMGRStartToneDetector				StartToneDetector;			// +
	DEVMGRStopToneDetector				StopToneDetector;			// +
	DEVMGRDeleteToneDetector			DeleteToneDetector;			// +
	DEVMGRSetupCallingTone				SetupCallingTone;			// +
	DEVMGRStartCallingTone				StartCallingTone;			// +
	DEVMGRStopCallingTone				StopCallingTone;			// +
	DEVMGRSetupDTMFGenerator			SetupDTMFGenerator;			// +
	DEVMGRSetDialMode					SetDialMode;				// +
	DEVMGRDialDigitAsync				DialDigitAsync;				// +
	DEVMGRCancelDialMode				CancelDialMode;				// +
	DEVMGRV8bisSet						V8bisSet;					// +
	DEVMGRV8bisStartToneDetector		V8bisStartToneDetector;		// +
	DEVMGRV8bisStopToneDetector			V8bisStopToneDetector;		// +
	DEVMGRV8bisStartTone				V8bisStartTone;				// +
	DEVMGRV8bisStopTone					V8bisStopTone;				// +
	DEVMGRV8bisSetupV21					V8bisSetupV21;				// +
	DEVMGRV8bisSetupMode				V8bisSetupMode;				// +
	DEVMGRV8bisSendMsg					V8bisSendMsg;				// +
	DEVMGRV8bisTxOff					V8bisTxOff;					// +
	DEVMGRV8bisCancel					V8bisCancel;				// +
	DEVMGRConfigureMsParams				ConfigureMsParams;			// +
	DEVMGRV8SetConfig					V8SetConfig;				// +
	DEVMGRV8GetConfig					V8GetConfig;				// +
	DEVMGRV8GetStatus					V8GetStatus;				// +
	DEVMGRActivateMsParams				ActivateMsParams;			// +
	DEVMGRUpdateRateMask				UpdateRateMask;				// +
	DEVMGRSetAutoRateControl			SetAutoRateControl;			// +
	DEVMGRBeginRetrain					BeginRetrain;				// +
	DEVMGRClearRemoteRetrainDetect		ClearRemoteRetrainDetect;	// +
	DEVMGRCreateFax						CreateFax;					// +
	DEVMGRDestroyFax					DestroyFax;					// +
	DEVMGRStartFaxAnswerTone			StartFaxAnswerTone;			// +
	DEVMGRStopFaxAnswerTone				StopFaxAnswerTone;			// +
	DEVMGRConfigureFaxParams			ConfigureFaxParams;			// +
	DEVMGRActivateFaxParams				ActivateFaxParams;			// +
	DEVMGRFaxSilence					FaxSilence;					// +
    DEVMGRStartLoopbackTest				StartLoopbackTest;			// +  //pq
	DEVMGRStopLoopbackTest				StopLoopbackTest;			// +  //pq
    HANDLE                              (*PttObjectCreate)(HANDLE hDevMgr, PVOID pParams);
    void                                (*PttObjectDestroy)(HANDLE handle);
    COM_STATUS                          (*PttObjectStart)(HANDLE hDevMgr, PVOID pParams);
    COM_STATUS                          (*PttObjectStop)(HANDLE hDevMgr);
} I_DEVICE_MGR_T, *PI_DEVICE_MGR_T;

/**************************************************************************************

PUBLIC API

**************************************************************************************/
void* DevMgrGetInterface(void);

/*
UINT32		GetInterfaceVersion(void);
HANDLE		Create(PINSTANCE_ACCESS_INFO pAccess);
COM_STATUS	Destroy(HANDLE hDevMgr);
COM_STATUS	Open(HANDLE hDevMgr);
COM_STATUS	Close(HANDLE hDevMgr);
COM_STATUS	Configure(HANDLE hDevMgr, eConfigureCode ConfigureCode, void* pParam);
COM_STATUS	Monitor(HANDLE hDevMgr, eMonitorCode MonitorCode, void* pMonitor, UINT32 dwSize);
COM_STATUS	Control(HANDLE hDevMgr, eControlCode ControlCode, void* pParam);
HANDLE		CreateStreamObject(HANDLE hDevMgr, eStreamType StreamType);
COM_STATUS	DestroyStreamObject(HANDLE hDevMgr, HANDLE hStream);
COM_STATUS	ConfigureStreamObject(HANDLE hDevMgr, HANDLE hStream, 
										 eStreamConfigCode StreamConfigCode, void* pParam);
COM_STATUS	PMControl(HANDLE hDevMgr, ePmeFunction PmeFunction, ePmeState PmeState);
COM_STATUS	SpeakerControl(HANDLE hDevMgr, eSpeakerControlType SpeakerControlType, UINT8 cValue);
COM_STATUS	MaskDataTransfer(HANDLE hDevMgr, eDirection Mask, BOOL bEnable);
COM_STATUS	PurgeData(HANDLE hDevMgr, eDirection Direction);
COM_STATUS	SetRingDetect(HANDLE hDevMgr);
COM_STATUS	CancelRingDetect(HANDLE hDevMgr);
COM_STATUS	BeginCtrlNotification(HANDLE hDevMgr, HANDLE StatCode, ePolarity Polarity);
COM_STATUS	EndCtrlNotification(HANDLE hDevMgr, HANDLE StatCode);
HANDLE		CreateToneDetector(HANDLE hDevMgr, eTONEDETECT_TYPE ToneDetectId);
COM_STATUS	StartToneDetector(HANDLE hDevMgr, HANDLE hTone);
COM_STATUS	StopToneDetector(HANDLE hDevMgr, HANDLE hTone);
COM_STATUS	DeleteToneDetector(HANDLE hDevMgr, HANDLE hTone);
COM_STATUS	SetupCallingTone(HANDLE hDevMgr, eCALLING_TONE CallingToneID, PTONEGEN pParams);
COM_STATUS	StartCallingTone(HANDLE hDevMgr);
COM_STATUS	StopCallingTone(HANDLE hDevMgr);
COM_STATUS	SetupDTMFGenerator(HANDLE hDevMgr, PDTMF_PARAMS_OUT pDTMFParmsOut);
COM_STATUS	SetDialMode(HANDLE hDevMgr, eDialMode DialMode);
COM_STATUS	DialDigitAsync(HANDLE hDevMgr, eDialDigit digit);
COM_STATUS	CancelDialMode(HANDLE hDevMgr);

COM_STATUS	V8bisSet(HANDLE hDevMgr);
COM_STATUS	V8bisStartToneDetector(HANDLE hDevMgr);
COM_STATUS	V8bisStopToneDetector(HANDLE hDevMgr);
COM_STATUS	V8bisStartTone(HANDLE hDevMgr, eV8BIS_TONE_ID ToneId);
COM_STATUS	V8bisStopTone(HANDLE hDevMgr);
COM_STATUS	V8bisSetupV21(HANDLE hDevMgr);
COM_STATUS	V8bisSetupMode(HANDLE hDevMgr, BOOL bInitiator);
COM_STATUS	V8bisSendMsg(HANDLE hDevMgr);
COM_STATUS	V8bisTxOff(HANDLE hDevMgr);
COM_STATUS	V8bisCancel(HANDLE hDevMgr, BOOL bV8bisSuccessful);

COM_STATUS	ConfigureMsParams(HANDLE hDevMgr, MODULATIOM_PARAMS* pModParam);
COM_STATUS	ActivateMsParams(HANDLE hDevMgr);
COM_STATUS	V8SetConfig(HANDLE hDevMgr, V8Config* pV8ConfParams, BOOL bV90Enabled);
COM_STATUS	V8GetConfig(HANDLE hDevMgr, V8Config* pV8ConfParams);
COM_STATUS	V8GetStatus(HANDLE hDevMgr, V8Status* pV8StatParams);

COM_STATUS	UpdateRateMask(HANDLE hDevMgr);
COM_STATUS	SetAutoRateControl(HANDLE hDevMgr, UINT8 cFlag);
COM_STATUS	BeginRetrain(HANDLE hDevMgr, UINT32 dwRate, eRetrainType RetrainType);
COM_STATUS	ClearRemoteRetrainDetect(HANDLE hDevMgr);

HANDLE		CreateFax(HANDLE hDevMgr);
COM_STATUS	DestroyFax(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	StartFaxAnswerTone(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	StopFaxAnswerTone(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	ConfigureFaxParams(HANDLE hDevMgr, HANDLE hDpFaxObject, FAX_MOD_PARAMS* pFaxModParam);
COM_STATUS	ActivateFaxParams(HANDLE hDevMgr, HANDLE hDpFaxObject);
COM_STATUS	FaxSilence(HANDLE hDevMgr, HANDLE hDpFaxObject);
*/

/**************************************************************************************

Test Object API

**************************************************************************************/

COM_STATUS DevMgr_StartLoopbackTest( HANDLE hDevMgr); //pq
COM_STATUS DevMgr_StopLoopbackTest( HANDLE hDevMgr);  //pq
HANDLE      DevMgr_PttObjectCreate(HANDLE hDevMgr, PVOID pParams);
void        DevMgr_PttObjectDestroy(HANDLE handle);
COM_STATUS DevMgr_PttObjectStart(HANDLE hDevMgr, PVOID pParams);
COM_STATUS DevMgr_PttObjectStop(HANDLE hDevMgr);

#endif // _DEVMGR_H
