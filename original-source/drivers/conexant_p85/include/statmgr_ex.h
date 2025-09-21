/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/StatMgr_ex.h_v   1.4   29 Jun 2000 11:50:08   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			StatMgrE.h	

File Description:		

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

#ifndef __STAT_MGR_E_H__
#define __STAT_MGR_E_H__

//#include "typedefs.h"
#include "comtypes.h"
#include "statcode.h"
#include "intfctrl_ex.h"

#define	STAT_MGR_MAJOR_VERSION	0
#define	STAT_MGR_MINOR_VERSION	2

#define STAT_MGR_VERSION (((STAT_MGR_MAJOR_VERSION) << 16) | (STAT_MGR_MINOR_VERSION))

typedef enum 
{
	STATMGR_CONFIG_LAST /* Dummy */
}STATMGR_CONFIG_CODE;

typedef enum 
{
	STATMGR_MONITOR_LAST /* Dummy */
}STATMGR_MONITOR_CODE;

typedef enum 
{
	STATMGR_CONTROL_LAST /* Dummy */
}STATMGR_CONTROL_CODE;

typedef VOID (*PEVENT_CB_FUNC)(	HANDLE			hEventInitiator, 
								STATUS_CODE		StatusCode,		
								PVOID			pStatusData,
								PVOID			pRefData);	

typedef struct STATUS_CODE_INFO_MAP_TAG{
	STATUS_CODE	StatusCode;
	UINT32		StatusDataSize;
	BOOL		bByValue;
}STATUS_CODE_INFO_MAP_T, *PSTATUS_CODE_INFO_MAP_T;

typedef enum 
{
	SNT_ALWAYS,
	SNT_ON_CHANGE,
	SNT_ON_VALUE,
	SNT_ON_CHANGE_VALUE,
} NOTIFY_TYPE;

typedef struct REQUEST_STATUS_NOTIFY_PARAMS_TAG{
	STATUS_CODE		StatusCode;
	HANDLE			hClient;		/* a handle of the client we want to be notify on its StatusCode change. */
	PEVENT_CB_FUNC	pCallBack;
	NOTIFY_TYPE		NotifyType;
	PVOID			pNotifyStatusData;
} REQUEST_STATUS_NOTIFY_PARAMS_T, *PREQUEST_STATUS_NOTIFY_PARAMS_T;

	/********************************************************************************
	
	Function Name: StatMgrGetInterface.

	Function Description:The function returns a pointer to the status manager interface 
						 structure that contains pointer to all the exported status manager
						 functions.
	Arguments: None.
	
	Return Value: A pointer to the status manager interface functions structure.

	********************************************************************************/
	GLOBAL	PVOID	StatMgrGetInterface(void);

typedef struct I_STAT_MGR_TAG{

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
	HANDLE		(*Create)(IN	PSYSTEM_INSTANCES_T	pSystemInstances);			

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
	COM_STATUS	(*Configure)(IN HANDLE				hBlock,
							 IN STATMGR_CONFIG_CODE	ConfigCode,
							 IN PVOID				pConfig);

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
	COM_STATUS	(*Monitor)(IN	HANDLE					hBlock,
						   IN	STATMGR_MONITOR_CODE	MonitorCode,
						   OUT	PVOID					pMonitor);

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
	COM_STATUS	(*Control)(IN	HANDLE					hBlock,
						   IN	STATMGR_CONTROL_CODE	ControlCode,
						   IN	PVOID					pControl);



	/************ Specific Interface functions. ************/

	/********************************************************************************
	
	Function Name:RegisterClient

	Function Description: The function registers a client to the Status Manager.
						  It allocates memory for storing the status codes and values.
						  The Status Manager will support only the StatusCodes which were
						  reported in the pStatusCodeSizeMap table.

	Arguments: hBlock - block handle (identify of the block).
			   hClient - client handle to be registered.
			   pStatusCodeSizeMap - is a pointer to a table of structures. Each entry
			   defines a pair of StatusCode and its related pStatusData structure 
			   (see NotifyEvent function).
			   CodeSizeMapNum - the number of entries in the pStatusCodeSizeMap table.

	Return Value: HANDLE to the registered client.

	********************************************************************************/
	HANDLE	(*RegisterClient)(IN	HANDLE					hBlock,
							  IN	PSTATUS_CODE_INFO_MAP_T	pStatusCodeSizeMap,
							  IN	UINT32					CodeSizeMapNum);

	/********************************************************************************
	
	Function Name:UnregisterClient

	Function Description: The function unregisters a client from the Status Manager.
						  It deallocated all the memory belongs to this client. (the
						  same memory space that was allocated in the RegisterClient
						  function).

	Arguments: hBlock - block handle (identify of the block).
			   hClient - client handle to be unregistered.
			   
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the client was successfully unregistered).

	********************************************************************************/
	COM_STATUS	(*UnregisterClient)(IN	HANDLE		hBlock,
									IN	HANDLE		hClient);

	/********************************************************************************
	
	Function Name:StartWaitForEvent

	Function Description: Start event notification request.

	Arguments: hBlock - block handle (identify of the block).
			   pRefData - a reference data to be passed in the callback function.
			   pReportParams - structure containing event params.

	Return Value: HANDLE - a handle to the notification request.

	********************************************************************************/
	HANDLE	(*StartWaitForEvent)(IN	HANDLE							hBlock,
								 IN PVOID							pRefData,
								 IN	PREQUEST_STATUS_NOTIFY_PARAMS_T	pReportParams);

	/********************************************************************************
	
	Function Name:StopWaitForEvent

	Function Description: stop event notification request.

	Arguments: hBlock - block handle (identify of the block).
			   hNotify - the notification handle that was returned by the 
			   StatMgrStartWaitForEvent function.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the status code was successfully rempved
				  from the notification list)

	********************************************************************************/
	COM_STATUS	(*StopWaitForEvent)	(IN	HANDLE		hBlock,
									 IN  HANDLE		hNotify);	

	/********************************************************************************
	
	Function Name:QueryStatus

	Function Description: Starts 

	Arguments: hBlock - block handle (identify of the block).
			   hClient - queried Client handle.
			   StatusCode - queried status code
			   pStatusData- pointer to be filled with status data

	Return Value: COM_STATUS (COM_STATUS_SUCCESS status can be queried.

	********************************************************************************/
	COM_STATUS	(*QueryStatus)( IN	HANDLE				hBlock,
								IN	HANDLE				hClient,
								IN	STATUS_CODE			StatusCode,
								OUT	PVOID				pStatusData);

	/********************************************************************************
	
	Function Name:DirectQueryStatus

	Function Description: Queries the state of a specified status 

	Arguments: hBlock - block handle (identify of the block).
			   hClient - specifies Client handle.
			   StatusCode - specifies status code
			   ppStatusData- address of pointer to status data

	Return Value: COM_STATUS (COM_STATUS_SUCCESS status can be queried.)

	********************************************************************************/
	COM_STATUS	(*DirectQueryStatus)(	IN	HANDLE				hBlock,
										IN	HANDLE				hClient,
										IN	STATUS_CODE			StatusCode,
										IN	PPVOID				ppStatusData);

	/********************************************************************************
	
	Function Name:UpdateStatus

	Function Description: Updates status state

	Arguments: hBlock - block handle (identify of the block).
			   hClient - Client handle.
			   StatusCode - status code
			   pStatusData- pointer to structure with updated status data
			   bDefferedNotofication - boolean indicating if to callback immedeatly 
			                           or in a deffered call

	Return Value: COM_STATUS (COM_STATUS_SUCCESS status can be queried.

	********************************************************************************/
	COM_STATUS	(*UpdateStatus)(	IN	HANDLE				hBlock,
									IN	HANDLE				hClient,
									IN	STATUS_CODE			StatusCode,
									IN	PVOID				pStatusData,
									IN	BOOL				bDefferedNotification);


} I_STAT_MGR_T, *PI_STAT_MGR_T;

#endif /* __STAT_MGR_E_H__ */
