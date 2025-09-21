/*========================================================================
   File			:	OsalCallback.cpp 
 
   Purpose		:	Implementation file for the BeOS OSAL class.  This file 
   					contains all the methods used to add, remove and
   					maniplate callback functions inside the OSAL entity.
   					
   					The rest of the OsalBeOS class's methods are split into 
   					two other files :  OSALstorage.cpp and OSAL_beos.cpp
   					The class itself is defined in Osal_BeOS.hpp
         
   NOTE			:   This version uses high priority threads as well as
   					the periodic interrupt priority timer to execute the 
   					callback functions.  The timer is declared in 
   					Osal_BeOS.cpp 
   
   Author		:	Bryan Pong, 2000-05

   Modifications:	Added callback threads to improve system responsiveness.
   					Added extended and exagerated comments. ^_^
      				Lee Patekar, 2000-11

             		Copyright (C) 1999 by Trisignal communications Inc.
                            	     All Rights Reserved
  =========================================================================*/

#include "HostCommon.h"
#include "TsOsalAPI.hpp"
#include "Osal_BeOS.hpp"
#include "target.h"
#include <KernelExport.h> 		// for Kernel threads 

//#define USELOCK

#ifdef USELOCK
#include <lock.h>				// for locks
#include <string.h>				// for memset
#endif

/*========================================================================
			Constant definitions
  ========================================================================*/
#define MAX_NBR_CALLBACK    5 	// Max number of callbacks we can accept
#define INVALID_ID          0	// Value of an invalid callback ID

	// Thread priorities in BeOS.  Must be > 105 for PC BeOS R5.0.1
	// or the callback threads will never have processor time during
	// bootup.  (The port will thus force-close which adds 20 seconds 
	// to the bootup time.  Not fun. ;o)

#define LOWPRIORITY		60
#define MEDPRIORITY		70
#define HIGHPRIORITY	80
								
/*========================================================================
			Macro definitions
  ========================================================================*/
#define START_CALLBACKID(priority)      (10000 * priority)     

/*========================================================================
			Structure definitions
  ========================================================================*/
typedef struct _CallbackInfo
{
public:
    void    	(*CallbackFnct)(void*,UINT16);	// function to be called
    void*		pContext;		// address of the CTimoutCallback class
    							// that is .. TODO
    UINT16  	TimeoutPeriod;	// interval at which it should be called
    UINT16  	TType;			// Not used
    UINT16  	Priority;		// It's priority (non-BeOS priority)

    UINT16  	Index;			// Index inside the Callback array
    UINT32  	CallbackID;		// ID to manipulate the callback function
    	// Indicates that the Callback is being removed
    UINT8   	UnRegisterRequest;
		// Time at which previous call to CallbackFnct was made 
    UINT32  	LastCallTime;   
		// Indicates that the callback function is a thread
	bool		IsThread;
		// Handle of the thread (the callback function itself)
	thread_id	ThreadId;
		// Handle of the Semaphore used to activate the thread				
	sem_id		SemId;
	
} CallbackInfo , *PCallbackInfo;


/*========================================================================
				Public variables definitions
   ========================================================================*/

/*========================================================================
				Private variables definitions
   ========================================================================*/
#ifdef USELOCK
static lock callback_lock;
#endif

		// All structures content is set at zero by default
static UINT16 NbrRegisteredCallback = 0;
		// Array containing all the callback structures
static CallbackInfo RegisteredCallback[MAX_NBR_CALLBACK];

/*========================================================================
			Private routines declarations
   ========================================================================*/
static void          RemoveFromList(PCallbackInfo CallbackPtr);
static PCallbackInfo Find(UINT32 ID);
static UINT32        GetNextAvailableID(UINT16 Priority);
static void          GenericCallbackCall(PCallbackInfo CallbackPtr);
//static void          EnterCritSect(PCallbackInfo CallbackPtr);
//static void          LeaveCritSect(PCallbackInfo CallbackPtr);

/*=========================================================================

  Function	:	ThreadFunc

  Purpose	:	Generic function representing a callback function thread.
  				A copy of this function is loaded as a thread for each
  				callback function when the port is opened.
  				
  				The periodic timer intterupt releases the latch semaphore
  				of every callabck function thread every 10 ms.  The threads
  				then checks if the callback function should be called and
  				if so the threads then calculates the lagtime between the
  				time the sallback function should be called and the actual
  				time which it is being called.  This lagtime is then sent
  				to the callback function.
  				
  Input		:   param :	
				
				Contains a UINT16 number representing the index where the
				callback's variables and status flags are contained in
				the callback function array (RegisteredCallback).

				The variables are copied into local memory, the status 
				flags remain inside the structure 

  Output	:   Modifies the Status flag in the callback function 
  				array (RegisteredCallback).

  Returns	:   Always returns B_OK (return value is not used by GMC)
  
  NOTE		:	The Timer Interrupt releases the semaphore that allows the
  				the thread to operate once every 10 ms.  
  				
  				If the threads BeOS priority is lower then 105 it never gets 
  				processor time during bootup, thus it is unable to reach its
  				return instruction to terminate.

  =========================================================================*/
static int32 ThreadFunc(void *param)
{
	UINT16 Index = (UINT16)param;

    UINT16 LagTimeMsec, ActualPeriodMsec;
	UINT32 ActualTime;
	status_t SemReturn;

	// We make a local copy to avoid crashing the kernel
	// if ever the thread has to timeout.

	sem_id LatchSem = RegisteredCallback[Index].SemId;
	
	// Loop around periodically until told to quit or a timeout occurs
	while(true)
	{
		// Wait for the trig signal from the periodic interrupt to continue
		SemReturn = acquire_sem_etc(LatchSem, 1, B_RELATIVE_TIMEOUT, 500000);	

		// If the semaphore timed out, that means the port was forced to 
		// close and this thread must terminate.  We shouldn't touch the 
		// OSAL memory space to avoid causing a kernel crash.
		if(SemReturn != B_NO_ERROR)
		{
			break;
		}
		
		// If its time to quit then we should terminate the thread
		if(TRUE == RegisteredCallback[Index].UnRegisterRequest)
		{
			break;
		}

		// Calculate real timeout period	
	    ActualTime = OsalPtr->GetTickCount();			

	    ActualPeriodMsec = (UINT16)(ActualTime - RegisteredCallback[Index].LastCallTime);

		// if timeout period is sufficient, execute the callback
		if (ActualPeriodMsec >= RegisteredCallback[Index].TimeoutPeriod)		
		{
			// calculate the lagtime
		    LagTimeMsec = ActualPeriodMsec - RegisteredCallback[Index].TimeoutPeriod;	// calculate lag time
	        RegisteredCallback[Index].LastCallTime = ActualTime;

			// call the process (callback function)		
			RegisteredCallback[Index].CallbackFnct(RegisteredCallback[Index].pContext, LagTimeMsec);			
		}
	}
	
	return B_OK;		// We don't process the thread's return
}

/*=========================================================================

  Function	:	RegisterTimeoutCallback

  Purpose	:   Adds a callback function to the callback function 
  				array (RegisteredCallback).

  Input		:   CallbackFnct :
  				
  				Pointer to the callback function's code (or a static 
  				function that will call the callback function).  This 
  				callback function receives a void pointer and an UINT16 
  				parameter and it returns a void pointer.
  				
  				pContext :
  				
  				Pointer to the CTimeoutCallback class that is handling
  				the callback function.
  	
  				TimeoutPeriod :
  				
  				Contains the period, in ms, that this periodic callback
  				function should have.  In other words, contains the time
  				needed between each sucessive call to the callback function.
  
  				TType :
  				
  				Can be PERIODIC or ONE_SHOT.  It is not used in this file,
  				class or method.
  				
  				Priority :
  				
  				Contains the GMC priority that the callback function should
  				posses.  The maxium priority is defined by the 
  				SupportTaskPrioritization method in this file.  This is not
  				the actual BeOS thread priority!!
  
  Output	:   Adds a callaback function in the callback function 
  				array (RegisteredCallback).

  Returns	:   This method returns an UINT32 value representing the succes
  				or failure of the callback's registration.  The return value
  				is either INVALID_ID if the callback function was not added
  				or a valid ID number if the callabcak was added successfully.

  NOTE		:	The actual priorities given to the callback threads are
  				determined by the LOWPRIORITY and MEDPRIORITY definitions.

  				The third definition, HIGHPRIORITY, is not used since
  				our higest priority thread will be called inside the
  				periodic timer itself which will grant it interrupt priority.

				Since the HIGHPRIORITY thread (or intterup function) is
				small we should see no system degradation.  However, if need
				be, the HIGHPRIORITY callback function can also be placed in
				a thread.

  =========================================================================*/
UINT32 OsalBeOS::RegisterTimeoutCallback(void (*CallbackFnct)(void*,UINT16), 
                                         void*   pContext, 
                                         UINT16  TimeoutPeriod, 
                                         UINT16  TType,
                                         UINT16  Priority)
{
    UINT32  RetID = INVALID_ID;  // Assume invalid ID

#ifdef USELOCK
	LOCK(callback_lock);
#endif

	do
	{
	    if(NbrRegisteredCallback < MAX_NBR_CALLBACK)
	    {
	        int          index = 0;
	        CallbackInfo TmpInfo;
	        int PriorityTable[3] = {LOWPRIORITY, MEDPRIORITY, HIGHPRIORITY};
	 		
	        // Find first available CallbackInfo in list
	        // ID = 0 means it is not used
	        while(RegisteredCallback[index].CallbackID != 0)
	        {
	            index++;
	        }
	
	        // Fill temp struct.
	        TmpInfo.CallbackFnct      = CallbackFnct;
	        TmpInfo.pContext          = pContext;
	        TmpInfo.TimeoutPeriod     = TimeoutPeriod; 
	        TmpInfo.TType             = TType;
	        TmpInfo.Priority          = Priority;
	        TmpInfo.Index             = index;
				// Give a unique ID to the callback function
	        TmpInfo.CallbackID        = GetNextAvailableID(Priority);
	        TmpInfo.UnRegisterRequest = FALSE;
	        TmpInfo.LastCallTime      = OsalPtr->GetTickCount();
	
	
			// If callback function will be a thread.  Priority 3 will be an 
			// interrupt callback routine.  Changing the "2" to "3" will place
			// all callback functions inside of threads.
			if(Priority <= 2)
			{
				TmpInfo.IsThread = true;	// Indicate callback is a thread
	
				// Create callback thread and trigger semaphore		
				TmpInfo.SemId = create_sem(0, "modemSem");  
				if(TmpInfo.SemId <= 0)					
				{
					// In this case the thread never began to execute so
					// killing the thread this way should be safe.
					RetID= INVALID_ID;
					break;
				}

				TmpInfo.ThreadId = spawn_kernel_thread(ThreadFunc, "modemThread", PriorityTable[Priority-1], (void *)index);
				if(TmpInfo.ThreadId <= 0)
				{
					delete_sem(TmpInfo.SemId);
					RetID= INVALID_ID;
					break;
				}
	
	
				// Assign semaphore to kernel
				set_sem_owner(TmpInfo.SemId, B_SYSTEM_TEAM);
			}
			
			// if callback will be in interrupt timer routine
			else
			{
				// say its not a thread
				TmpInfo.IsThread 		  = false;	
				TmpInfo.ThreadId 		  = B_BAD_THREAD_ID;
				TmpInfo.SemId			  = 0;
			}
	
	
			// register the new callback
	        RegisteredCallback[index] = TmpInfo;
	        NbrRegisteredCallback++;
	     	if(TmpInfo.IsThread) {
				resume_thread(TmpInfo.ThreadId);	// launch the thread
			}
   
			// Return the ID number of the callback function so that the
			// GMC modules can remove or manipulate the callback function.
			// NOTE : If two callbacks have identical IDs the system will not
			// crash, however the order in which the callbacks will be removed
			// will be uncertain.
			
	        RetID = TmpInfo.CallbackID;
		}
   } while(0);

#ifdef USELOCK
   UNLOCK(callback_lock);
#endif
   
    return RetID;
}

/*=========================================================================

  Function	:	UnRegisterTimeoutCallback

  Purpose	:   Removes a Callback function from the callback function 
  				array (RegisteredCallback).  Ordinairily this method is 
  				called repeatedly every 20 ms untill the callback is 
  				eventually removed.  However, for system stability, our 
  				threaded implementation will block untill the thread 
  				is removed sucessfully.

  Input		:   CallbackID : 
  
  				An UINT32 containing the unique ID of the callback function.

  Output	:   Removes the callback from the callback function 
				array (RegisteredCallback).

  Returns	:   An UINT16 containing the success of the unregister request.
  				Possible values are INVALID_CALLBACK_ID if the callback ID
  				does not exist, UNREG_SUCCESS if the callback was removed or
  				CALLBACK_PENDING if the callback is still in the process to
  				be removed.

  NOTE		:	As mentioned above, if the callback is a thread this method
  				will block untill it is removed.
  				
  				The threads require processor time to allow them to terminate
  				correctly.  Thats why the threads require a higher priority
  				then the thread that call's this method.
            
  =========================================================================*/
UINT16 OsalBeOS::UnRegisterTimeoutCallback(UINT32 CallbackID)
{
    UINT16        RetVal;
    PCallbackInfo CallbackPtr;  
	status_t waste;
//	status_t error;

#ifdef USELOCK
	LOCK(callback_lock);
#endif

	do
	{
	    if((CallbackPtr = Find(CallbackID)))
	    {
				// Tell the thread or callback to exit
			CallbackPtr->UnRegisterRequest = TRUE;
			
			//	When removing the threads it's important to wait untill they are 
			//	removed completely.  This goes against the traditional callback
			//  function philosophy used in the GMC but is necessary to prevent
			//  kernel crashes if ever the port is forced to close.
	
			// If callback is a valid thread
			if(true == CallbackPtr->IsThread)
			{	
				// Keep a backup of the semaphore handle so we can delete it 
				// after we invalidate its value.  Make the semaphore handle 
				// invalid so that the periodic timer interrupt doesent trig
				// a terminated thread.
				
	        	sem_id tmp;				
		       	tmp = CallbackPtr->SemId;
				CallbackPtr->SemId = 0;
	
				// Release sem and launch the thread so it can terminate
				release_sem(tmp);
	
				// Wait for the thread to terminate
				wait_for_thread(CallbackPtr->ThreadId, &waste);
	
				// Remove the semaphore from the system
				delete_sem(tmp);
	
				// Invalidate thread handle just to be safe
				CallbackPtr->ThreadId 	= 0;
	
				// Remove the thread from the callback list and tell the calling
				// method or function that we unregistered the thread.
				RemoveFromList(CallbackPtr);		
				RetVal = UNREG_SUCCESS;
			}
			
			// If the callback is not a thread, use the traditional method
			else
			{
				// Since the callback function is executed inside the periodic
				// interrupt routine, we know that we'll never be able to
				// unregister the callback while its running.  So we will never
				// have the CALLBACK_PENDING return value.
				
	          	// Remove the callback function from the array
	            RemoveFromList(CallbackPtr);
	
	            // Tell the calling method or function that the callback
	            // was removed successfully.
	            RetVal = UNREG_SUCCESS;
			}
	    }
	    else
	    {   // the callback was not found in list
	        RetVal = INVALID_CALLBACK_ID;
	    }
	}while(0);

#ifdef USELOCK
	UNLOCK(callback_lock);
#endif

    return RetVal;
}


/*=========================================================================

  Function	:	SupportTaskPrioritization

  Purpose	:   Returns the amount of priority levels we can support.  If we
  				don't support any priority levels we should return 1.

  Input		:   none  

  Output	:   none  

  Returns	:   An UINT16 value containing the amount of priority levels
  				our methods can support.

  NOTE		:	We support a grand total of 3 levels.  Level "1" are 
  				LOWPRIORITY threads, level "2" are MEDPRIORITY threads and 
  				level "3" are interrupt priority callback functions.
  				
  				As mentioned in RegisterTimeoutCallback's notes, level "3"
  				can be simply modified to support HIGHPRIORITY threads 
  				instead of the interrupt priority callback.
            
  =========================================================================*/
UINT16 OsalBeOS::SupportTaskPrioritization( void )
{
	return 3;
}


/*=========================================================================

  Function	:	RemoveFromList

  Purpose	:   Invalidates a callback function and decrements the global
  				callback counter.  This sucessfully removes the callback
  				from the array of registered callbacks.

  Input		:  	CallbackPtr:
  				
  				A PCallbackInfo (CallbackInfo structure pointer) that points
  				to the callback function's information struction inside the 
  				registered callback	function array.

  Output	:   Modifies the CallbackID of the Callback pointed to by
  				CallbackPtr and modifies the static global counter 
  				NbrRegisteredCallback.

  Returns	:   nothing   
  =========================================================================*/
static void RemoveFromList(PCallbackInfo CallbackPtr)
{
    // Invalidtated the CallbackID so that another Callback function can
    // be stored in this one's place (eventually).
    CallbackPtr->CallbackID = INVALID_ID;

	// Since the CallbackID is invalid, the Callback is unusable, so we
	// decrement the callback count.
    NbrRegisteredCallback--;
}


/*=========================================================================

  Function	:	Find

  Purpose	:   Finds the specified callback inside the registed callback 
  				array and returns a pointer to its pointer and information.

  Input		:	ID :
  
  				A UINT32 variable containing the ID number of the callback
  				function.  

  Output	:   none

  Returns	:   A PCallbackInfo (CallbackInfo structure pointer) that points
  				to the desired callback function structure or NULL if the
  				specified ID is invalid.

  NOTE		:	If two or more Callback function have the same ID this
  				function will return a pointer to the first one it finds.
            
  =========================================================================*/
static PCallbackInfo Find(UINT32 ID)
{
    int           index;
    PCallbackInfo CallbackPtr = RegisteredCallback;

	// For every possible index inside the registered callback array
    for(index = 0; index < MAX_NBR_CALLBACK; index++, CallbackPtr++)
    {
        // if struct is filled with valid information we found the callback
        // we were looking for
        if(CallbackPtr->CallbackID == ID)  
        {   
            break;
        }
    }

	// If we went through all the possible indexs ...
    if(index == MAX_NBR_CALLBACK)
    {
        // .. that means we didn't find our callback.
        CallbackPtr = NULL;
    }
    
    return CallbackPtr;
}


/*=========================================================================

  Function	:	GetNextAvailableID

  Purpose	:   This function is responsible for giving each callback a 
  				unique ID.  This function does not check to see if a valid
  				ID is available.. it doesent even check if we've reached the
  				maximum amount of callbacks.  So theses checks should be
  				performed before calling this function.

  Input		:   Priority :
  
  				A UINT16 variable containing the callbacks priority level.
  				Note that this is not the thread's BeOS priority.

  Output	:   none

  Returns	:   The ID the callback should take.

  =========================================================================*/
static UINT32 GetNextAvailableID(UINT16 Priority)
{
    int           index;
    PCallbackInfo CallbackPtr = RegisteredCallback;
    UINT32        MaxIDInUse  = START_CALLBACKID(Priority);

    // Scan ALL registered callbacks
    for(index = 0; index < MAX_NBR_CALLBACK; index++, CallbackPtr++)
    {
        // if struct is filled with valid information
        // looking for SAME priority and valid ID
        if((CallbackPtr->CallbackID != INVALID_ID) && (CallbackPtr->Priority == Priority))
        {   
            MaxIDInUse = MaxIDInUse < CallbackPtr->CallbackID ? CallbackPtr->CallbackID : MaxIDInUse;
        }
    }

    // this way we never use an ID already in use.
    // not checking if MAX ID is reached - assuming range is large enough
    return MaxIDInUse + 1;
}


/*=========================================================================

  Function	:	GenericCallbackCall

  Purpose	:   If the callback is a thread, it will release the trigger 
  				semaphore so that the thread can execute and run the 
  				callback function if its timeout period has expired.
  				
  				If the callback is an interrupt priority function, 
  				GenericCallbackCall will determine if the callback function
  				should be called.  If its timeout period has expired, it 
  				will calculate the lag time between the time the callback
  				function will execute and the time it should have been 
  				executed and it will launch the callback function.

  Input		:   CallbackPtr :
				
				A PCallbackInfo (CallbackInfo structure pointer) pointing
				to the callback thread's (or function's) callback info 
				structure containing its callback function pointer and
				timeout values.

  Output	:   If the callback is an interrupt priority function
				GenericCallbackCall will add the time the callback was 
				executed inside the LastCallTime variable inside the 
				callback's info structure for later reference.
				
				If the callback is a thread GenericCallbackCall does not
				modify the callback's info structure.

  Returns	:   nothing

  =========================================================================*/
static void GenericCallbackCall(PCallbackInfo CallbackPtr)
{
    UINT32 ActualTime;
    UINT16 LagTimeMsec, ActualPeriodMsec;
	
	// if the callback is a thread ...
	if(true == CallbackPtr->IsThread)
	{
		// .. and if the trigger semaphore is valid ...
		if(CallbackPtr->SemId > 0)
		{
			// .. tell the thread to run, but do not launch it right away
			// because we are inside an interrupt routine!
			release_sem_etc(CallbackPtr->SemId, 1, B_DO_NOT_RESCHEDULE);
		}
	}
	else
	{
	    // Compute lagtime : diff between desired period and 'real' period 
	    // of the call.
		ActualTime = OsalPtr->GetTickCount();
	    ActualPeriodMsec = (UINT16)(ActualTime - CallbackPtr->LastCallTime) ;

		// if timeout period is sufficient, execute the callback
	    if (ActualPeriodMsec >= CallbackPtr->TimeoutPeriod)
	    {
	        LagTimeMsec = ActualPeriodMsec - CallbackPtr->TimeoutPeriod;
	
	        // update time for next call
	        CallbackPtr->LastCallTime = ActualTime;
	        // call the callback function that was registered
	        CallbackPtr->CallbackFnct(CallbackPtr->pContext, LagTimeMsec);
		}
	}
}


/*=========================================================================

  Function	:	RoundRobinModemCallbacks

  Purpose	:   This function is called by the periodic timer interrupt 
  				every 10 ms.  It will call each and every callback function
  				that is registered inside the registered callback array via
  				the GenericCallbackCall function.

  Input		:   none

  Output	:   none 

  Returns	:   none 

  NOTE		:	It's the GenericCallbackCall function that will determine 
  				if the callback function should be executed or not.  If the
  				callback function is a thread, the GenericCallbackCall 
  				function will release the callback thread's trigger 
  				semaphore so that the thread may execute and determine if
  				the callback function should be called.
            
  =========================================================================*/
void RoundRobinModemCallbacks(void)
{
    UINT16        index;
    PCallbackInfo CallbackPtr = RegisteredCallback;

	// For every callback function inside the array 
    for(index = 0; index < MAX_NBR_CALLBACK; index++, CallbackPtr++)
    {
		// If the Callback function or thread exists
        if(CallbackPtr->CallbackID != INVALID_ID)
        {   
            // if it is call it! (Or as Picard would say: Engage)
            GenericCallbackCall(CallbackPtr);
        }
    }
}

void InitCallbacks(void)
{
#ifdef USELOCK
	new_lock(&callback_lock, "trimodem_callback_lock");
	NbrRegisteredCallback = 0;
	memset(RegisteredCallback, 0, sizeof(RegisteredCallback));
#endif
}

void UninitCallbacks(void)
{
#ifdef USELOCK
	free_lock(&callback_lock);
#endif
}

