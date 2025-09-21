/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/OsTime_ex.h_v   1.6   29 Jun 2000 11:50:06   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			OsTime_Ex.h

File Description:	Operating Specific Timer Services prototypes.

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


#include "comtypes.h"
#include "rtmgr_ex.h"


/**********************************************************************************
Function name :	OSCreatePeriodicTimeOut
Parameters :	Irql				- IRQ level at wich the timer should be called
				InitialTimeOut		- initial interval between 2 timeout callbacks
				pTimeOutCallBack	- Callback to be periodically called back
				pFuncAlloc			- wrapper function for allocations
				pFuncFree			- wrapper function for deallocations
				pRefData			- Reference Data to be passed to wrapper functions
Return value :	Returns a handle to the periodic timeout. This handle is to be
				passed to OS functions acting on periodic timeout. Returns NULL
				on failure.
functionality :	Creates a periodic timeout of InitialTimeOut interval. pTimeOutCallBack
				function will be called on each timer interval. 
**********************************************************************************/
GLOBAL	HANDLE	OSCreatePeriodicTimeOut	(	IN	TIMER_IRQL_TYPE	Irql,
											IN	UINT32			InitialTimeOut,
											IN	PCBFUNC			pTimeOutCallBack,
											IN	PALLOC_FUNC		pFuncAllc,
											IN	PFREE_FUNC		pFuncFree,
											IN	PVOID			pRefData);

/**********************************************************************************
Function name :	OSDesrtoyPeriodicTimeOut
Parameters :	hTimeOut -	Handle to the periodic time out returned by 
							OSCreatePeriodicTimeOut.
Return value :	none
functionality : destroys a periodic timeout resource.
**********************************************************************************/
GLOBAL	VOID	OSDestroyPeriodicTimeOut	(	IN	HANDLE			hTimeOut);

/**********************************************************************************
Function name :	OSSetPeriodicTimeOut
Parameters :	hTimeOut	-	Handle to the periodic timeout
				NewTimeOut	-	New timeout interval
Return value :	returns TRUE when the timeout interval was successfully changed to 
				NewTimeOut
functionality : resets the timeout inteval to a new value.
**********************************************************************************/
GLOBAL	BOOL	OSSetPeriodicTimeOut	(	IN	HANDLE			hTimeOut,
											IN	UINT32			NewTimeOut);

GLOBAL  VOID    OSImmediateTimeOut      (	IN	HANDLE			hTimeOut);

GLOBAL VOID OSCancelPeriodicTimeOut		(	IN HANDLE			hTimeOut);

/**********************************************************************************
Function name :	OSCallOnMyStack
Parameters :	pFunc			-	function to be called
				lParam			-	parameter to pass for pFunc
				pTopOfStack		-	Pointer to stack
				StackSizeBytes	-	stack size in bytes.
Return value :	None
functionality :	Temporarily switches a stack to a different locked block of memory.
**********************************************************************************/
GLOBAL	VOID	OSCallOnMyStack(			IN	PCBFUNC			pFunc, 
											IN	UINT32			lParam, 
											IN	PUINT32			pTopOfStack, 
											IN	UINT32			StackSizeBytes);

/**********************************************************************************
Function name :	OSGetSystemTime
Parameters :	None
Return value :	returns timestamp in milliseconds
functionality	The function returnes a milliseconds timestamp.
**********************************************************************************/
/* returns time stamp in milli-seconds					*/
GLOBAL	UINT32	OSGetSystemTime(			VOID	);

/**********************************************************************************
Function name :	OSSemaphoreCreate
Parameters :	InitCount - specifies the initial token count for the semaphore
Return value :	Handle to the created semaphore to be passed as parameter for OS 
				functions acting on semaphores. Returns NULL on failure.
functionality	The tokenCount should correspond to the number of clients that can 
				simultaneously acquire the resource the semaphore controls
**********************************************************************************/
GLOBAL	HANDLE	OSSemaphoreCreate(			IN	int				InitCount);

/**********************************************************************************
Function name :	OSSemaphoreDestroy
Parameters :	hSemaphore	-	Handle identifing the semaphore.
Return value :	None
functionality :	Destroys a semaphore. Any tasks currently locked by the semaphore 
				become reschedules.
**********************************************************************************/
GLOBAL	VOID	OSSemaphoreDestroy(			IN	HANDLE			hSemaphore);

/**********************************************************************************
Function name :	OSSemaphoreWait
Parameters :	hSemaphore	-	Handle identifing the semaphore.
Return value :	None
functionality	Blocks the calling run time  if all the tokens of the semaphore are 
				currently claimed.  If the number of clients that currently hold 
				the controlled resource is less than the initial token count, 
				then wait returns immediately and the caller holds the resource.  
				Otherwise, the client is blocked until another client releases 
				the resource.
**********************************************************************************/
GLOBAL	VOID	OSSemaphoreWait(			IN	HANDLE			hSemaphore);

/**********************************************************************************
Function name :	OSSemaphoreSignal
Parameters :	hSemaphore	-	Handle identifing the semaphore.
Return value :	None
functionality :	increments the token count of the semaphore.  If run times are waiting 
				on the semaphore, one of them will be unblocked.  
				The current run time may be rescheduled this function is called.
**********************************************************************************/
GLOBAL	VOID	OSSemaphoreSignal(			IN	HANDLE			hSemaphore);

/**********************************************************************************
Function name :	OSCriticalSectionCreate
Parameters :	None
Return value :	Handle to the created critical section to be passed as parameter for OS 
				functions acting on critical section. Returns NULL on failure.
functionality :	Creates an instance of a critical section. Returns NULL on failure
**********************************************************************************/
GLOBAL	HANDLE	OSCriticalSectionCreate(	VOID);

/**********************************************************************************
Function name :	OSCriticalSectionDestroy
Parameters :	hMutex	-	Handle identifing the mutex.
Return value :	None
functionality : Destroys a mutex instance. Do not attemt to destroy a mutex while 
				it is acquired.
**********************************************************************************/
GLOBAL	VOID	OSCriticalSectionDestroy(	IN	HANDLE			hMutex);

/**********************************************************************************
Function name :	OSCriticalSectionAcquire
Parameters :	hMutex	-	Handle identifing the mutex.
Return value :	None
functionality:	If the mutex is unowned, the calling run time becomes the owner
				If a run time  other than the calling run time owns the mutex, 
				the calling run time is blocked until the mutex  becomes unowned.  
				If the calling run time already owns the mutex, the run time continues 
				execution.

**********************************************************************************/
GLOBAL	VOID	OSCriticalSectionAcquire(	IN	HANDLE			hMutex);

/**********************************************************************************
Function name :	OSCriticalSectionTry
Parameters :	hMutex	-	Handle identifing the mutex.
Return value :	COM_STATUS_SUCCESS if entered critical section, COM_STATUS_FAIL 
				otherwise

functionality:	If hMutex is unowned - enters critical section. If someone
				owns hMutex, doesn't block or wait - immediately returns COM_STATUS_FAIL.

**********************************************************************************/

GLOBAL	COM_STATUS	OSCriticalSectionTry(	IN	HANDLE			hMutex);




/**********************************************************************************
Function name :	OSCriticalSectionRelease
Parameters :	hMutex	-	Handle identifing the mutex.
Return value :	None
functionality:	A mutex is initially unowned, and the ownership count is zero.  For 
				each call to acquire, there must be a corresponding call to release.  
				Each time acquire is called, the ownership count  increments, and each 
				time release is called, the ownership count decrements.  Only the run time
				that owns a mutex may leave it.
				When the ownership count falls to zero, the mutex becomes unowned.  At 
				this point, a run time blocked by the mutex, with the highest priority 
				becomes schedulable.
**********************************************************************************/
GLOBAL	VOID	OSCriticalSectionRelease(	IN	HANDLE			hMutex);

/**********************************************************************************
Function name : OSSetTimeSensitivity
Parameters :	Interval - timer interrupt interval
Return value :	None
functionality:	Determines the operatin system timeout interval, to enable timer 
				resolution control. For each call to OSSetTimeSensitivity there
				should be a matching call to OSRestoreTimeSensitivity with same Interval 
				value
**********************************************************************************/
GLOBAL	VOID	OSSetTimeSensitivity(	IN	UINT32				Interval);				

/**********************************************************************************
Function name :	OSRestoreTimeSensitivity
Parameters :	Interval - timer interrupt interval
Return value :	None
functionality :	Restores the timer interrupt to the value before OSSetTimeSensitivity
				was called
**********************************************************************************/
GLOBAL	VOID	OSRestoreTimeSensitivity(		IN	UINT32				Interval);				

/**********************************************************************************
Function name :	OSDisableInterrupt
Parameters :	None
Return value :	handle to be passed to OSRestoreInterrupt
functionality :	Disables interrupt.
**********************************************************************************/
GLOBAL	HANDLE	OSDisableInterrupts();

/**********************************************************************************
Function name :	OSRestoreInterrupts
Parameters :	hStatus - handle returned by OSDisableInterrupt
Return value :	None
functionality :	Restores the interrupt status as before the OSDisableInterrupt was called
**********************************************************************************/
GLOBAL	VOID	OSRestoreInterrupts(		IN	HANDLE			hStatus);
