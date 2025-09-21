/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/SessionMgr.h_v   1.10   29 Jun 2000 11:50:06   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			SessionMgr.h	

File Description:	Session Manager object enumerated types and ptototypes.	

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


#ifndef __SESSION_MGR_H__
#define __SESSION_MGR_H__

#include "comtypes.h"
#include "intfctrl_ex.h"
#include "configtypes.h"

#define	SESSION_MGR_MAJOR_VERSION	0
#define	SESSION_MGR_MINOR_VERSION	1

#define SESSION_MGR_VERSION (((SESSION_MGR_MAJOR_VERSION) << 16) | (SESSION_MGR_MINOR_VERSION))

#define	UNSUPPORTED_SESSION	0

typedef enum {
    SESMGR_HR_ATH_DTRDROP,
    SESMGR_HR_KEYABORT,
    SESMGR_HR_CARRIERLOSS,
    SESMGR_HR_DETECTEDBUSY,
    SESMGR_HR_DETECTEDVOICE,
    SESMGR_HR_INACTIVETIMEOUT,
    SESMGR_HR_NODIALTONE,
    SESMGR_HR_S7HANGUP,
    } HANGUP_REASON;

typedef enum {
    SESMGR_CP_COMPLETE_V8BIS_DETECTED,
    SESMGR_CP_COMPLETE_V8BIS_COMPLETE,
    SESMGR_CP_COMPLETE_ANSWER_DETECTED,
    SESMGR_CP_COMPLETE_FAILURE,
    SESMGR_CP_COMPLETE_SUCCESS,
    SESMGR_CP_TRAINING_STARTED
    } CP_COMPLETE_REASON;

typedef enum {
    SESMGR_STATUSMGR_CLIENT,
    SESMGR_MONITOR_ORIGINATOR,                  /* BOOL - TRUE when originator */
    // FCLASS mode monitor code
    SESMGR_MONITOR_FCLASS,                      /* FCLASS - current Fax Class mode */
    SESMGR_MONITOR_MODULATION,                  /* int - current fax modulation AT+FRM,FTM,FRH,FTH */
    SESMGR_MONITOR_AT_FAR,                      /* AT+FAR processing routine (Fax session) */
    SESMGR_MONITOR_AT_FCL,                      /* AT+FCL processing routine (Fax session) */
    SESMGR_MONITOR_AT_FDD,                      /* AT+FDD processing routine (Fax session) */
    SESMGR_MONITOR_AT_FIT,                      /* AT+FIT processing routine (Fax session) */
   
    SESMGR_MONITOR_BLKLST_DELAY,

    SESMGR_MONITOR_BLACKLISTED,
    SESMGR_MONITOR_DELAYED,

    SESMGR_MONITOR_LAST
    
    } SESMGR_MONITOR_CODE;

typedef enum {
	SESMGR_CONTROL_START,
    SESMGR_CONTROL_STOP,
	SESMGR_CONTROL_DIAL,
	SESMGR_CONTROL_ANSWER,
	SESMGR_CONTROL_ABORT,					    /* Key Abort */
    SESMGR_CONTROL_HANGUP,                      /* DTE-initiated hangup */
	SESMGR_CONTROL_OFFHOOK,					    /* ATH1 - go offhook */
    SESMGR_CONTROL_RETRAIN,
    SESMGR_CONTROL_START_SESSION,
    SESMGR_CONTROL_STOP_SESSION,
    SESMGR_CONTROL_CP_COMPLETE,
	SESMGR_CONTROL_START_TRAINING,
    SESMGR_CONTROL_STOP_TRAINING,               /* Carrier timeout */

    /* Digital call progress */
    SESMGR_CONTROL_DCP_ACTIVATE,
    SESMGR_CONTROL_DCP_DEACTIVATE,
    SESMGR_CONTROL_DCP_CONTROL,                 /* Volume control */

	SESMGR_CONTROL_DTR,
	SESMGR_CONTROL_RTS,

    SESMGR_CONTROL_TX_DATA_READY,               /* required for V.21 operation */
    SESMGR_CONTROL_DTEFLOW,                     /* DTE Flow control event DTEFLOW_EVENT_TYPE */ 
    SESMGR_CONTROL_UPDATE_POUNDUD,              /* */
    SESMGR_CONTROL_UPDATE_EC_POUNDUD,
    SESMGR_CONTROL_INIT_POUNDUD,
    SESMGR_CONTROL_UPDATE_TERMINATION_CAUSE,
// FCLASS switch control code
    SESMGR_CONTROL_AT_FCLASS,                   /* AT+FCLASS processing routine (Fax session) */
    SESMGR_CONTROL_QUERY_FCLASS,
// Fax session control codes
    SESMGR_CONTROL_AT_FTS,                      /* AT+FTS processing routine (Fax session) */
    SESMGR_CONTROL_AT_FRS,                      /* AT+FRS processing routine (Fax session) */
    SESMGR_CONTROL_AT_FTH,                      /* AT+FTH processing routine (Fax session) */
    SESMGR_CONTROL_AT_FRH,                      /* AT+FRH processing routine (Fax session) */
    SESMGR_CONTROL_AT_FTM,                      /* AT+FTM processing routine (Fax session) */
    SESMGR_CONTROL_AT_FRM,                      /* AT+FRM processing routine (Fax session) */
    SESMGR_CONTROL_AT_FAR,                      /* AT+FAR processing routine (Fax session) */
    SESMGR_CONTROL_AT_FCL,                      /* AT+FCL processing routine (Fax session) */
    SESMGR_CONTROL_AT_FDD,                      /* AT+FDD processing routine (Fax session) */
    SESMGR_CONTROL_AT_FIT,                      /* AT+FIT processing routine (Fax session) */
//  Black Listing control codes
    SESMGR_CONTROL_CHANGE_COUNTRY,
// Test session control codes
	SESMGR_CONTROL_ACTIVATE_TEST,   //pq
	SESMGR_CONTROL_DEACTIVATE_TEST,   //pq
    } SESMGR_CONTROL_CODE;

typedef enum {
    SESMGR_CONFIG_DUMMY
    } SESMGR_CONFIG_CODE;

typedef struct
{
	MODULATION_TYPE Modulation;
	UINT32 TxBitRate, RxBitRate;
} CARRIER_REPORT;

/* used for +a8i: indications */
typedef enum {
    ANSWER_TIMEOUT = 0,                         /* DCE timeout waiting for answer signal    */
    ANSAM_TYPE,                                 /* ANSam (2100Hz AM modulated signal)       */
    V25ANS_TYPE,                                /* ANS (2100Hz)                             */
    V25ANSPR_TYPE,                              /* ANS (2100Hz) with phase reversal         */
    T30PREAMBLE_TYPE,                           /* T.30 preamble (v21ch2)                   */
    V22B_USB1_TYPE,                             /* V.22bis USB1                             */
    V32B_AC_TYPE,                               /* V.32bis AC                               */
    V34_TONEA_TYPE,                             /* V.34 ToneA                               */
    } ANSWER_TYPE;

typedef enum
{
   FCLASS0,
   FCLASS1,
   FCLASS2,
   FCLASS8,
   FCLASS80,
   FCLASS_PTTSESSION,  //pq
   FCLASS_TESTSESSION, //pq
}  FCLASS;


typedef struct tagAT_FIT_PARAMS
{
    int     time;
    int     action;
}   AT_FIT_PARAMS, *PAT_FIT_PARAMS;


typedef enum tagDTEFLOW_EVENT_TYPE
{
    DTEFLOW_TX_HALTED,
    DTEFLOW_TX_RESUMED,
    DTEFLOW_RX_HALTED,
    DTEFLOW_RX_RESUMED,
    DTEFLOW_RX_OVERRUN

} DTEFLOW_EVENT_TYPE;

COM_STATUS UD_Init(HANDLE hSessionMgr);
COM_STATUS UD_Update_MM(HANDLE hSessionMgr);
COM_STATUS UD_CollectInfo(HANDLE hSessionMgr);
void UD_UpdateTerminationCause(HANDLE hSessionMgr, CALL_TERMINATION_CAUSE eTerminationCause);
void UD_CollectECInfo(HANDLE hSessionMgr);

	/********************************************************************************
	
	Function Name: SessionMgrGetInterface.

	Function Description:The function returns a pointer to the status manager interface 
						 structure that contains pointer to all the exported status manager
						 functions.
	Arguments: None.
	
	Return Value: A pointer to the status manager interface functions structure.

	********************************************************************************/
	GLOBAL	PVOID	SessionMgrGetInterface(void);

typedef struct I_SESSION_MGR_TAG{

	/************ Common Interface functions. ************/
	
	/********************************************************************************
	
	Function Name: GetInterfaceVersion.

	Function Description:The function returns the interface compilation version.
						 The function should be called before any other function 
						 in the function table in order to verify that the calling 
						 code and the block code are compatible.
	
	Arguments: None.
	
	Return Value: The interface compilation version.

	********************************************************************************/
	UINT32		(*GetInterfaceVersion)();


	/********************************************************************************

	Function Name: Create.

	Function Description: The function creates a block instance.
	
	Arguments: MemMgrInterface - Access (function table & instance handle ) 
	                             to memory manager.

	Return Value: An handle to the created instance. (this handle will passed as an 
				  argument to all other functions to identify the block instance).
				  (in case of error it returns NULL).

	********************************************************************************/
	HANDLE		(*Create)(PSYSTEM_INSTANCES_T pInstances);			

	/********************************************************************************

	Function Name: Destroy.

	Function Description: The function destroy the block instance identify by the hBlock
						  handle. After the destroying this handle becomes invalid. 
	
	Arguments: hblock - a handle to the block to destroy. (this handle was returned by 
						the create function when we created this block instance).
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully 
				  destroyed).

	********************************************************************************/
	COM_STATUS	(*Destroy)(IN HANDLE	hBlock);

	/********************************************************************************

	Function Name: Init.

	Function Description: The function initialize the block identified by the hBlock 
						  parameter.
	
	Arguments: a handle to the block to initialize.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  initialized).

	********************************************************************************/
	COM_STATUS	(*Init)(IN	HANDLE	hBlock);

	/********************************************************************************

	Function Name: Close.

	Function Description: The function closes the block identified by the hBlock handle.
						  After the instance was closed, its handle can be destroyed by 
						  the destroy function.
	
	Arguments: a handle to the block to close.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Close)(IN HANDLE	hBlock);		

	/********************************************************************************

	Function Name: Configure.

	Function Description: The function is used to update the block instance
						  configurations. Each ConfigCode associates with a specific
						  structure containing the configuration value/data.
						  The function can be called only after create and before init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ConfigCode - enumerated code identifying the configuration type.
			   pConfig - pointer to a structure containing the configuration data.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Configure)(IN HANDLE			hBlock,
							 IN SESMGR_CONFIG_CODE	ConfigCode,
							 IN PVOID			pConfig);

	/********************************************************************************

	Function Name: Monitor.

	Function Description: The function is used to monitor a block instance.
						  Each MonitorCode is associates with a specific structure
						  containing the Monitoring information.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   MonitorCode - enumerated code identifying the monitor type.
			   pMonitor - pointer to a structure that will be filled with the monitored data.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  monitored).

	********************************************************************************/
	COM_STATUS	(*Monitor)(IN	HANDLE			hBlock,
						   IN	SESMGR_MONITOR_CODE MonitorCode,
						   OUT	PVOID			pMonitor);

	/********************************************************************************

	Function Name: Control.

	Function Description: The function is used to change the value of the block 
						  instance parameters. Each ControlCode is associates with 
						  a specific structure containing the control data/value.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ControlCode - enumerated code identifying the control type.
			   pControl - pointer to a structure containing the control data.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  controlled).

	********************************************************************************/
	COM_STATUS	(*Control)(IN	HANDLE			hBlock,
						   IN	SESMGR_CONTROL_CODE	ControlCode,
						   IN	PVOID			pControl);



	/************ Specific Interface functions. ************/

} I_SESSION_MGR_T, *PI_SESSION_MGR_T;

#endif /* __SESSION_MGR_H__ */
