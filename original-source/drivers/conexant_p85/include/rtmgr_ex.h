/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/RtMgr_ex.h_v   1.5   29 Jun 2000 11:50:06   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			RTMgrE.h	

File Description:	Runtime Manager enumerated types and prototypes

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


#ifndef __RT_MGR_E_H__
#define __RT_MGR_E_H__

#include "comtypes.h"
#include "intfctrl_ex.h"

#define	RT_MGR_MAJOR_VERSION	((UINT32)0)
#define	RT_MGR_MINOR_VERSION	((UINT32)2)

#define RT_MGR_VERSION ((UINT32)(((RT_MGR_MAJOR_VERSION) << 16) | (RT_MGR_MINOR_VERSION)))

typedef enum 
{
	RTMGR_CONFIG_LAST /* Dummy */
} RTMGR_CONFIG_CODE;

typedef enum 
{
	RTMGR_MONITOR_TIME_STAMP,						/* UINT32 * - time stamp in milliseconds */
	RTMGR_MONITOR_LAST /* Dummy */
} RTMGR_MONITOR_CODE;
	
typedef enum 
{
	RTMGR_CONTROL_LAST /* Dummy */
} RTMGR_CONTROL_CODE;

/*********************** Run time Manager Exported types :********************/
typedef enum {
	PASSIVE_IRQL,
	DPC_IRQL = PASSIVE_IRQL,
	TIME_CRITICAL_IRQL = PASSIVE_IRQL,
	LAST_IRQL
} TIMER_IRQL_TYPE;


typedef enum ASYNC_CALL_TYPE_TAG{
	ASYNC_CALL_TYPE_ONE_SHOT, 
	ASYNC_CALL_TYPE_PERIODIC
} ASYNC_CALL_TYPE_T;
typedef struct ASYNC_CALL_CONFIG_TAG{
	ASYNC_CALL_TYPE_T	Type;
	int 				TimeOut;
	PUINT32				pStack;				
	UINT32				StackSize;		
	PCBFUNC     		pFuncAsyncCall;
	TIMER_IRQL_TYPE		TimerIRQL;
}ASYNC_CALL_CONFIG_T, *PASYNC_CALL_CONFIG_T;

typedef struct RUN_TIME_CLIENT_CONFIG_TAG
{
	int					TimerResolution; /* Defines the requred system timer resolution*/
}RUN_TIME_CLIENT_CONFIG_T, *PRUN_TIME_CLIENT_CONFIG_T;

typedef struct CALL_CONFIG_TAG
{
	PUINT32		pStack;
	UINT32		StackSize;
	PCBFUNC		pFunc;
	PVOID		FuncParams;
}CALL_CONFIG_T, *PCALL_CONFIG_T;
/********************************************************************************
Function Name: RTMgrGetInterface.

Function Description:Return pointer to RunTime Manager interface.

rguments: None.

Return Value: Pointer to RunTime interface function table.

********************************************************************************/
GLOBAL	PVOID	RTMgrGetInterface(void);


typedef struct I_RT_MGR_TAG{
	
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
	UINT32		(*GetInterfaceVersion)(	void);

	/********************************************************************************

	Function Name: Create.

	Function Description: The function creates a block instance.
	
	Arguments: None.
	
	Return Value: An handle to the created instance. (this handle will passed as an 
				  argument to all other functions to identify the block instance).
				  (in case of error it returns NULL).

	********************************************************************************/
	HANDLE		(*Create)(				IN PSYSTEM_INSTANCES_T	pSystemInstances);			

	/********************************************************************************

	Function Name: Destroy.

	Function Description: The function destroy the block instance identify by the hBlock
						  handle. After the destroying this handle becomes invalid. 
	
	Arguments: hblock - a handle to the block to destroy. (this handle was returned by 
						the create function when we created this block instance).
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully 
				  destroyed).

	********************************************************************************/
	COM_STATUS	(*Destroy)(				IN HANDLE			hBlock);

	/********************************************************************************

	Function Name: Init.

	Function Description: The function initialize the block identified by the hBlock 
						  parameter.
	
	Arguments: a handle to the block to initialize.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  initialized). Close function should be called even when init failed.

	********************************************************************************/
	COM_STATUS	(*Init)(				IN	HANDLE			hBlock);

	/********************************************************************************

	Function Name: Close.

	Function Description: The function closes the block identified by the hBlock handle.
						  After the instance was closed, its handle can be destroyed by 
						  the destroy function.
	
	Arguments: a handle to the block to close.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Close)(				IN	HANDLE			hBlock);		

	/********************************************************************************

	Function Name: Configure.

	Function Description: The function is used to update the block instance
						  configurations. Each ConfigCode associates with a specific
						  structure containing the configuration value/data.
						  The function can be called only after create and before init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ConfigCode - enumerated code identifying the configuration type.
			   pConfig - pointer to a structure containing the configuration data.
			   ConfigSize - size of the configuration structure.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  closed).

	********************************************************************************/
	COM_STATUS	(*Configure)(			IN	HANDLE				hBlock,
										IN	RTMGR_CONFIG_CODE	ConfigCode,
										IN	PVOID				pConfig);

	/********************************************************************************

	Function Name: Monitor.

	Function Description: The function is used to monitor a block instance.
						  Each MonitorCode is associates with a specific structure
						  containing the Monitoring information.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   MonitorCode - enumerated code identifying the monitor type.
			   pMonitor - pointer to a structure that will be filled with the monitored data.
			   MonitorSize - size of the monitor structure.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  monitored).

	********************************************************************************/
	COM_STATUS	(*Monitor)(				IN	HANDLE				hBlock,
										IN	RTMGR_MONITOR_CODE	MonitorCode,
										OUT	PVOID				pMonitor);

	/********************************************************************************

	Function Name: Control.

	Function Description: The function is used to change the value of the block 
						  instance parameters. Each ControlCode is associates with 
						  a specific structure containing the control data/value.
						  The function can be called only after init.
	
	Arguments: hBlock - block handle (identify of the block).
			   ControlCode - enumerated code identifying the control type.
			   pControl - pointer to a structure containing the control data.
			   ControlSize - size of the control structure.
	
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the block instance was successfully
				  controlled).

	********************************************************************************/
	COM_STATUS	(*Control)(				IN	HANDLE				hBlock,
										IN	RTMGR_CONTROL_CODE	ControlCode,
										IN	PVOID				pControl);



	/************ Specific Interface functions. ************/

	
	/********************************************************************************
	
	Function Name:RegisterClient
	  
	Function Description: The function registers a client to the Run Time Manager identified
						  by hRunTime. 

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   pClientConfig - a pointer to a structure containing all the client 
			   registration parameters.

	Return Value: a handle (identifier) for the new client. This handle should be used as
				  a parameter for the functions: UnregisterClient, AsyncCall, BeginBlockRT,
				  CancelAsyncCall and EndBlockRT.(in case of error it returns NULL).

	********************************************************************************/
	HANDLE	(*RegisterClient)(	IN	HANDLE						hRunTime,
								IN	PRUN_TIME_CLIENT_CONFIG_T		pClientConfig);
	
	/********************************************************************************
	
	Function Name:SetClientTimerResolution
	  
	Function Description: Updates the timer resolution required by the 
						  Client. 

	Arguments:	hRunTime - a handle for the Run Time Manager instance.
				hClient	- handle to the client
			   TimerResolution - New client reqired timer resolution

	Return Value: COM_STAUTS_SUCCESS when client resolution is successfuly 
					updated, else failure code.

	********************************************************************************/
	COM_STATUS	(*SetClientTimerResolution)(	
								IN	HANDLE						hRunTime,
								IN	HANDLE						hClient,
								IN	int							TimerResolution);
	/********************************************************************************
	
	Function Name: UnregisterClient
	  
	Function Description: The function unregisters a client from the RunTime manager.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the client was successfully unregistered)

	********************************************************************************/
	COM_STATUS	(*UnregisterClient)(		IN	HANDLE		hRunTime,
											IN	HANDLE		hClient);

	/********************************************************************************
	Function Name: CreateAsyncCall
	  
	Function Description: Creates a non schedules async call.
	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).

	Return Value: a handle to the AsyncCall. (in case of error it returns NULL).

	********************************************************************************/
	HANDLE	(*CreateAsyncCall)(	IN	HANDLE					hRunTime,
								IN	HANDLE					hClient);

	/********************************************************************************
	Function Name: SetAsyncCall
	  
	Function Description: This function is a request to the run time manager to schedule 
						  an async call as discribed in the hAsyncCall structure.
						  After the timeout is over the function that was sent as a parameter 
						  in the pConfig will get a run-time. (the timer an be one-shot 
						  or periodic).

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).
			   hAsyncCall - a handle to the AsyncCall that was returned from the CreateAsyncCall
			   pRefData - Reference data to be passed to the async callback function
			   pConfig - a pointer to a structure containing the data about the request
			   for the async call .

	Return Value: COM_STATUS_SUCCES on success, else error code.

	********************************************************************************/
	COM_STATUS	(*SetAsyncCall)(IN	HANDLE					hRunTime,
								IN	HANDLE					hClient,
								IN	HANDLE					hAsyncCall,
								IN	PVOID					pRefData,
								IN	PASYNC_CALL_CONFIG_T	pConfig);

	/********************************************************************************
	Function Name: RestartAsyncCall
	  
	Function Description:	Sets an async call with configuration passed in
							a previous SetAsyncCall of the specified hAsyncCall.
							If the async call was not set previously the 
							behaviour is not defined.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).
			   hAsyncCall - a handle to the AsyncCall that was returned from the CreateAsyncCall
			   pRefData - Reference data to be passed to the async callback function

	Return Value: COM_STATUS_SUCCES on success, else error code.

	********************************************************************************/
	COM_STATUS	(*RestartAsyncCall)
								(IN	HANDLE					hRunTime,
								IN	HANDLE					hClient,
								IN	HANDLE					hAsyncCall,
								IN	PVOID					pRefData);

	/********************************************************************************

	Function Name: CancelAsyncCall
	  
	Function Description: The function cancels the AsyncCall identified by the hAsyncCall
						  handle from scheduler. The async call can be rescheduled through 
						  SetAsyncCall function, or destroyed with DestroyAsyncCall function.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).
			   hAsyncCall - a handle to the AsyncCall that was returned from the CreateAsyncCall
			   function.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the AsyncCall was successfully canceled)

	********************************************************************************/
	COM_STATUS	(*CancelAsyncCall)(			IN	HANDLE		hRunTime,
											IN	HANDLE		hClient,
											IN	HANDLE		hAsyncCall);
	/********************************************************************************

	Function Name: DestroyAsyncCall
	  
	Function Description: The function destroys the AsyncCall identified by the hAsyncCall
						  handle.The async call handle becomes unusable.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client (which returns from the 
			   RegisterClient function).
			   hAsyncCall - a handle to the AsyncCall that was returned from the CreateAsyncCall
			   function.

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the AsyncCall was successfully canceled)

	********************************************************************************/
	COM_STATUS	(*DestroyAsyncCall)(			IN	HANDLE		hRunTime,
											IN	HANDLE		hClient,
											IN	HANDLE		hAsyncCall);

	/********************************************************************************
	
	Function Name: BeginBlockRT
	  
	Function Description: The function blocks asynchronous call of the client hClient
						  is refered to (if hClient == ANY_CLIENT, it blocks all the 
						  clients)
						  To stop the blocking use the EndBlockRT function.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client that we want to block its 
			   async calls. (ANY_CLIENT will block all the clients async calls).
			   
	Return Value: COM_STATUS (COM_STATUS_SUCCESS if it successes to block all the async call
				  associated with hClient).

	********************************************************************************/
	COM_STATUS	(*BeginBlockRT)(			IN	HANDLE		hRunTime,
											IN	HANDLE		hClient);

	/********************************************************************************
	
	Function Name: EndBlockRT
	  
	Function Description:

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hClient - a handle for the specific RunTIme client that we want to end its 
			   blocking (ANY_CLIENT will end the block for all the clients async calls).

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if it successes to end the blocking of the 
				  async call associated with hClient).

	********************************************************************************/
	COM_STATUS	(*EndBlockRT)(				IN	HANDLE		hRunTime,
											IN	HANDLE		hClient);

	/********************************************************************************
	
	Function Name: CallOnMyStack
	  
	Function Description: The function temporarily switches the stack to a different 
						  block of memory and then calls the given callback function.
						  When the callback function returns, it switches back to the
						  original stack.
						  This service can handle nested calls.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   pConfig - a pointer to a structure contains the information about
			   the request (a pointer to the function we should call, a pointer for
			   the stack (optional) etc.).

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if it successes to call the function
				  (from pConfig).

	********************************************************************************/
	COM_STATUS	(*CallOnMyStack)(			IN	HANDLE		hRunTime,
											IN	PCALL_CONFIG_T		pConfig);


	/********************************************************************************
	
	Function Name: DisableAllInterrupts
	  
	Function Description: The function disables all the interrupts in the system.

	Arguments: hRunTime - a handle for the Run Time Manager instance.

	Return Value: a handle containing the previous status of the flags.

	********************************************************************************/
	HANDLE	(*DisableAllInterrupts)(		IN	HANDLE		hRunTime);

	/********************************************************************************
	
	Function Name: RestoreAllInterrupts
	  
	Function Description: The function restore the previous status of the system flags
						  and enable interrupts in the system.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hPrevStatus - a handle containing the previous status of the flags.
			   (this handle was returned by the DisableAllInterrupts function).

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the previous status was successfully
				  restored).

	********************************************************************************/
	COM_STATUS	(*RestoreAllInterrupts)(	IN	HANDLE		hRunTime,
											IN	HANDLE		hPrevStatus);

	/********************************************************************************
	
	Function Name: DisableOurInterrupts
	  
	Function Description: The function disables the interrupts belongs to our system only.

	Arguments: hRunTime - a handle for the Run Time Manager instance.

	Return Value: a handle containing the previous status of the flags.

	********************************************************************************/
	HANDLE	(*DisableOurInterrupts)(		IN	HANDLE		hRunTime);

	/********************************************************************************
	
	Function Name: RestoreOurInterrupts
	  
	Function Description: The function restore the previous status of the our flags
						  and enable interrupts in the our system.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hPrevStatus - a handle containing the previous status of the flags.
			   (this handle was returned by the DisableOurInterrupts function).

	Return Value: COM_STATUS (COM_STATUS_SUCCESS if the previous status was successfully
				  restored).

	********************************************************************************/
	COM_STATUS	(*RestoreOurInterrupts)(	IN	HANDLE		hRunTime,
											IN	HANDLE		hPrevStatus);

	/********************************************************************************
	
	Function Name: SemaphoreCreate
	  
	Function Description: The function creates a semaphore with InitCount resources.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   InitCount - the number of resources the semaphore holds (the number of clients
			   that can simultaneously acquire the resource in question).

	Return Value: a handle to the new semaphore.(in case of error it returns NULL).

	********************************************************************************/
	HANDLE	(*SemaphoreCreate)(				IN	HANDLE		hRunTime,
											IN	UINT32		InitCount);

	/********************************************************************************
	
	Function Name: SemaphoreDestroy
	  
	Function Description: The function destroys the semaphore that hSemaphore is referred to.
						  Any threads that were blocked by this semaphore become schedulable.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hSemaphore - a handle to the semaphore (that was returned by SemaphoreCreate).

	Return Value: Handle identifying the created semaphore. NULL when creation failed

	********************************************************************************/
	COM_STATUS	(*SemaphoreDestroy)(		IN	HANDLE		hRunTime,
											IN	HANDLE		hSemaphore);


	/********************************************************************************
	
	Function Name: SemaphoreWait
	  
	Function Description: A client uses this function to ask for a resource from the
						  specific semaphore. If the number of clients that currently 
						  hold the resource is less than the InitCount (see SemaphoreCreate) 
						  it gets the resource and the function immediately return, else it 
						  blocked and wait untill other client releases the resource.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hSemaphore - a handle to the semaphore (that was returned by SemaphoreCreate).

	Return Value: COM_STATUS_SUCCESS when the semaphore was destored successfully.

	********************************************************************************/
	VOID	(*SemaphoreWait)(				IN	HANDLE		hRunTime,
											IN	HANDLE		hSemaphore);

	/********************************************************************************
	
	Function Name: SemaphoreSignal
	  
	Function Description: A client uses this function to release a semaphore.

	Arguments: hRunTime - a handle for the Run Time Manager instance.
			   hSemaphore - a handle to the semaphore (that was returned by SemaphoreCreate).

	Return Value: None.

	********************************************************************************/
	VOID	(*SemaphoreSignal)(				IN	HANDLE		hRunTime,
											IN	HANDLE		hSemaphore);

	/********************************************************************************
	
	Function Name: CriticalSectionCreate
	  
	Function Description: Creates A Critical Section.

	Arguments: hRunTime - a handle for the Run Time Manager instance.

	Return Value: Handle identifying the created Critical Section. NULL when creation failed

	********************************************************************************/
	HANDLE	(*CriticalSectionCreate)(		IN	HANDLE		hRunTime);

	/********************************************************************************
	
	Function Name: CriticalSectionDestroy
	  
	Function Description: Destroys A Critical Section.

	Arguments:	hRunTime - a handle for the Run Time Manager instance.
				hCSection - a handle to the critical section instance

	Return Value: COM_STATUS_SUCCESS when the Crtical Section was destored successfully.

	********************************************************************************/
	COM_STATUS	(*CriticalSectionDestroy)(	IN	HANDLE		hRunTime, 
											IN	HANDLE		hCSection);

	/********************************************************************************
	
	Function Name: CriticalSectionAcquire
	  
	Function Description:Acquires a CriticalSection. The current run time is
						 Blocked if the Critical Section is already acquired.

	Arguments:	hRunTime - a handle for the Run Time Manager instance.
				hCSection - a handle to the critical section instance

	Return Value: None.

	********************************************************************************/
	VOID	(*CriticalSectionAcquire)(		IN	HANDLE		hRunTime, 
											IN	HANDLE		hCSection);

	/********************************************************************************
	
	Function Name: CriticalSectionTry
	  
	Function Description:Tries to acquires a CriticalSection. If hCSection is 
						 already owned by someone else - doesn't wait or block, it
						 returns immediately with COM_STATUS_FAIL. Otherwise it enters 
						 critical section (takes ownership of hCSection) and returns COM_STATUS_SUCCESS

	Arguments:	hRunTime - a handle for the Run Time Manager instance.
				hCSection - a handle to the critical section instance

	Return Value: None.

	********************************************************************************/
	COM_STATUS	(*CriticalSectionTry)(	IN	HANDLE		hRunTime, 
										IN	HANDLE		hCSection);


	/********************************************************************************
	
	Function Name: CriticalSectionRelease
	  
	Function Description:Acquires a CriticalSection. The current run time is
						 Blocked if the Critical Section is already acquired.

	Arguments:	hRunTime - a handle for the Run Time Manager instance.
				hCSection - a handle to the critical section instance

	Return Value: None.

	********************************************************************************/
	VOID	(*CriticalSectionRelease)(		IN	HANDLE		hRunTime, 
											IN	HANDLE		hCSection);
	/********************************************************************************/
} I_RT_MGR_T, *PI_RT_MGR_T;

#endif /* __RT_MGR_E_H__ */
