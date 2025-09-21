/*osTime.c

This file includes os specific code for timing related functionalities.

*/
#include	<Drivers.h>
#include	<ISA.h>
#include	<PCI.h>
#include	<KernelExport.h>
#include	<driver_settings.h>
#include	<termios.h>
#include	<tty/ttylayer.h>
#include    "typedefs.h"
#include	"ostypedefs.h"
#include	"ostime_ex.h"
#include	"ostime_in.h"

//#define DEBUGGING
#ifdef DEBUGGING
#define dbgkprint dprintf
#else
#define dbgkprint
#endif

/*
This file includes os specific code for timing related functionalities.
*/
/********************************************************************/
/* Construct a periodic time out instance.RunTime manager does not	*/
/* simply use global timers , since these are called with the		*/
/* same thread handle as the ring3 application. This prevent		*/
/* synchronization between passive and async calls with semaphores	*/
/* or mutexs.
/* Parameters :                                                     */
/* Irql - run time IQRL in which the timeout call back              */  
/*        should be called. In Wi95 this parameter in ignored,      */
/*        and all timeout are called at dispatch level.             */
/* InitialTimeout - Initial timeout interval in ms pRefData         */
/*                  should be passed as a parameter to this function*/
/* PTimeOutCallBack - CallBack to be call every timeout duration.   */
/* pFuncAlloc - alloc function,pRefData should be passed as         */ 
/*              parameter for thids function. function should       */
/*              allocate memory in non-paged pool                   */
/* pFuncFree - free function,pRefData should be passed as           */
/*             parameter for thids function                         */
/* pRefData -  reference data to be passed to pTimeOutCallBack      */
/*             pFuncAllc and pFuncFree                              */
/********************************************************************/
GLOBAL	HANDLE	OSCreatePeriodicTimeOut	(	IN	TIMER_IRQL_TYPE	Irql,
											IN	UINT32			InitialTimeOut,
											IN	PCBFUNC			pTimeOutCallBack,
											IN	PALLOC_FUNC		pFuncAlloc,
											IN	PFREE_FUNC		pFuncFree,
											IN	PVOID			pRefData)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance;
dbgkprint("OSCreatePeriodicTimeOut()\n");
	pTimeOutInstance= pFuncAlloc(sizeof(TIME_OUT_INSTANCE_T), pRefData);
	if (NULL == pTimeOutInstance )
		return(NULL);

	pTimeOutInstance->pFuncFree			= pFuncFree;
	pTimeOutInstance->pRefData			= pRefData;
	pTimeOutInstance->pTimeOutCallBack	= pTimeOutCallBack;
	pTimeOutInstance->mSec				= InitialTimeOut;
	pTimeOutInstance->bLocked			= FALSE;
	pTimeOutInstance->TimerActive		= FALSE;
	pTimeOutInstance->hTimeOutHandle	= 0;
	pTimeOutInstance->hSemaphore		= create_sem(0,"Conexant Modem Sem");
	if (0 == pTimeOutInstance->hSemaphore)
	{
		pFuncFree(pTimeOutInstance, pRefData);
		return (NULL);
	}

    {
    	thread_id tid;
dbgkprint("spawning thread\n");    	
    	set_sem_owner(pTimeOutInstance->hSemaphore, B_SYSTEM_TEAM);
		tid = spawn_kernel_thread(TimerThreadFunction, "Conexant timer",  B_NORMAL_PRIORITY,  pTimeOutInstance);
		if (tid < B_NO_ERROR) {
dbgkprint("thread id: %d invalid\n", tid);
			// somthing went wrong.  Return NULL
			return (NULL);
		}
dbgkprint("starting thread\n");
		resume_thread(tid);
        
        pTimeOutInstance->hThread = tid;
		pTimeOutInstance->hTimeOutHandle	= tid;
    }


	if (0 == pTimeOutInstance -> hThread )
	{
		delete_sem(pTimeOutInstance->hSemaphore);
		pFuncFree(pTimeOutInstance, pRefData);
		return (NULL);
	}


	return( pTimeOutInstance);
	
}

/********************************************************************/
GLOBAL	VOID	OSDestroyPeriodicTimeOut	(IN	HANDLE hTimeOut)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance = (PTIME_OUT_INSTANCE_T)hTimeOut;
	status_t status;
//dbgkprint("OSDestroyPeriodicTimeOut()\n");
	
//	pTimeOutInstance->bLocked = TRUE;
	pTimeOutInstance->TimerActive= FALSE;

	if ( 0  != pTimeOutInstance->hTimeOutHandle )
	{
dprintf("*1* asking timer thread to exit\n");		
		delete_sem(pTimeOutInstance->hSemaphore);			   // this will unblock the thread if it was waiting
		wait_for_thread(pTimeOutInstance->hThread, &status);   // bad things happen when the driver unloads
									                           // while a thread is still executing
	}
	
	pTimeOutInstance->pFuncFree(pTimeOutInstance, pTimeOutInstance->pRefData);
}

/********************************************************************/
GLOBAL	BOOL	OSSetPeriodicTimeOut		(	IN	HANDLE			hTimeOut,
												IN	UINT32			NewTimeOut)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance = (PTIME_OUT_INSTANCE_T)hTimeOut;
dbgkprint("OSSetPeriodicTimeOut() to %dms\n", NewTimeOut);
	pTimeOutInstance->mSec = NewTimeOut;
	pTimeOutInstance->TimerActive	= TRUE;
	release_sem(pTimeOutInstance->hSemaphore);
	
	return ((UINT32)NULL != pTimeOutInstance->hTimeOutHandle);
}

GLOBAL VOID OSCancelPeriodicTimeOut			(	IN HANDLE			hTimeOut)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance = (PTIME_OUT_INSTANCE_T)hTimeOut;
dbgkprint("OSCancelPeriodicTimeOut()\n");
	if ( (UINT32)NULL != pTimeOutInstance -> hTimeOutHandle )
	{
		pTimeOutInstance->TimerActive = FALSE;
	}
}

/********************************************************************/
#ifdef windows
STATIC void _cdecl FuncOnNewStack(			IN	UINT32	lParam)
{
	PSTACK_CHANGE_INFO_T	pStackChangeInfo = (PSTACK_CHANGE_INFO_T)lParam;
	(pStackChangeInfo -> pFunc)((PVOID)pStackChangeInfo -> lParam);

}
#endif
/********************************************************************/
GLOBAL	VOID	OSCallOnMyStack(			IN	PCBFUNC			pFunc, 
											IN	UINT32			lParam, 
											IN	PUINT32			pTopOfStack, 
											IN	UINT32			StackSizeBytes)
{
//	STACK_CHANGE_INFO_T	StackChangeInfo;

//	StackChangeInfo.pFunc = pFunc;
//	StackChangeInfo.lParam = lParam;

	/* Cannot call pFunc dirctly, since _Call_On_My_Stack requires
	   that the function will be declared using the C calling convention.
	   so we are calling pFunc through FuncOnNewStack function*/
//	_Call_On_My_Stack((PVOID)FuncOnNewStack, (UINT32)&StackChangeInfo, pTopOfStack, StackSizeBytes);


    if(pFunc) pFunc((PVOID)lParam);
}

/********************************************************************/
GLOBAL  VOID    OSImmediateTimeOut      (	IN	HANDLE			hTimeOut)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance;
	pTimeOutInstance = (PTIME_OUT_INSTANCE_T)hTimeOut;

//	if (pTimeOutInstance->hSemaphore)
//		release_sem(pTimeOutInstance->hSemaphore);
}

extern bool freecalled, timeractive;

STATIC	int32 TimerThreadFunction(void * hTimeOut)
{
	PTIME_OUT_INSTANCE_T	pTimeOutInstance;
	timeractive=TRUE;
dbgkprint("TimerThreadFunction()\n");
dprintf("TimerThreadFunction()\n");
	pTimeOutInstance = (PTIME_OUT_INSTANCE_T)hTimeOut;
	while(TRUE)
	{
		status_t err;
//dbgkprint("TT - %d ms\n", pTimeOutInstance->mSec);
		err = acquire_sem_etc
		    (pTimeOutInstance->hSemaphore, 1, B_TIMEOUT, pTimeOutInstance->mSec*1000);
		if ((err == B_BAD_SEM_ID) || (freecalled))
		{
dprintf("*2* timer really ending now\n");
			timeractive=FALSE;
			return ;
		}
		
		// if we changed the delta timeout in OSSetPeriodicTimeOut, the release_sem()
		// call will cause an immediate continuation of this loop which is not desired.
//		if (err == B_TIMED_OUT)
		{
			if (pTimeOutInstance->TimerActive)
			// Call the periodic function
				pTimeOutInstance->pTimeOutCallBack(pTimeOutInstance->pRefData);
		}
	}
}

/********************************************************************/
GLOBAL	UINT32	OSGetSystemTime(			VOID	)
{

	int64 Time = system_time()/1000;
 
	return Time;
}

int32 semcount =0;

/* Semaphores */
/********************************************************************/
GLOBAL HANDLE	OSSemaphoreCreate(	IN	int InitCount)
{
	sem_id sem = create_sem (InitCount,"Conexant Modem SEM");
	if (sem >= B_NO_ERROR)
	{
dbgkprint("OSSemaphoreCreate() - could not create\n");
		sem = (sem_id)NULL;  /* there was an error */
	}
	else
	{
dbgkprint("OSSemaphoreCreate() - %d\n",++semcount);
		set_sem_owner(sem, B_SYSTEM_TEAM);
	}
	return ( (HANDLE) sem);
}

/********************************************************************/
GLOBAL VOID	OSSemaphoreDestroy(		IN	HANDLE hSemaphore)
{
	if (hSemaphore)
	{
dbgkprint("OSSemaphoreDestroy() -%d\n", semcount--);
		delete_sem((sem_id)hSemaphore);
	}
	else
dbgkprint("OSSemaphoreDestroy() - invalid\n");

}

/********************************************************************/
GLOBAL VOID	OSSemaphoreWait(		IN	HANDLE hSemaphore)
{
dbgkprint("OSSemaphoreWait()\n");
	acquire_sem((sem_id) hSemaphore);
}

/********************************************************************/
GLOBAL VOID	OSSemaphoreSignal(		IN	HANDLE hSemaphore)
{
dbgkprint("OSSemaphoreSignal()\n");
	release_sem((sem_id)hSemaphore);
}

int32 critsec=0;


 
typedef struct tagCS 
{ 
    int nCounter; 
    cpu_status cpu; 
 
} CS, *PCS; 
 
HANDLE OSCriticalSectionCreate() 
{ 
     PCS pCS = (PCS)calloc(sizeof(CS), 1); 
 
     if(!pCS) 
          return NULL; 
 
     pCS->nCounter = 0; 
 
     return pCS; 
} 
 
 
VOID OSCriticalSectionDestroy(HANDLE hCS) 
{ 
     OsMemFree(hCS); 
} 
 
VOID OSCriticalSectionAcquire(HANDLE hCS) 
{ 
    PCS  pCS = hCS; 
	cpu_status cpu;
 
	cpu = disable_interrupts();
	 
     if( !pCS->nCounter++  ) 
          pCS->cpu = cpu;  // Store flags only on first entrance 
} 
 
COM_STATUS OSCriticalSectionTry(HANDLE hCS) 
{ 
     OSCriticalSectionAcquire(hCS); 
     return COM_STATUS_SUCCESS; 
} 
 
VOID OSCriticalSectionRelease(HANDLE hCS) 
{ 
     PCS pCS = hCS; 
 
     if(!pCS->nCounter)  // Just sanity check 
     { 
          kernel_debugger("OSCriticalSectionRelease - CS isn't locked"); 
          return; 
     } 
 
     if(!--pCS->nCounter) 
     { 
	 	restore_interrupts( pCS->cpu );
     } 
} 



#if 0

/* Critical Section (Mutex) */ 
/********************************************************************/ 
GLOBAL HANDLE	OSCriticalSectionCreate() 
{ 
	PCRIT_SEC_T crit_sec = (PCRIT_SEC_T)calloc(sizeof(CRIT_SEC_T), 1); 
	crit_sec->cstid = B_BAD_THREAD_ID; 
	return ( (HANDLE) crit_sec); 
} 
 
/********************************************************************/ 
GLOBAL VOID	OSCriticalSectionDestroy(	IN	HANDLE hMutex) 
{
	PCRIT_SEC_T crit_sec = (PCRIT_SEC_T) hMutex; 
//	kprintf("Maximum: %dus\n",crit_sec->maxtime);
	free(hMutex); 
} 

extern bool inittime;
 
/********************************************************************/ 
GLOBAL VOID	OSCriticalSectionAcquire(	IN	HANDLE hMutex) 
{ 
#if 0
	PCRIT_SEC_T crit_sec = (PCRIT_SEC_T) hMutex; 
	thread_id tid = find_thread(0); 

	if (inittime)
		return; 
	if(crit_sec->cstid != tid) 
	{ 
		cpu_status cpu; 
		cpu = disable_interrupts(); 
		acquire_spinlock(&crit_sec->atom);
		crit_sec->lastacquire = system_time();
		crit_sec->status = cpu; 
		crit_sec->cstid = tid; 
	} 
	crit_sec->count++;
#endif	
}

GLOBAL	COM_STATUS	OSCriticalSectionTry(	IN	HANDLE			hMutex)
{
	return COM_STATUS_SUCCESS;
}

 
/********************************************************************/ 
GLOBAL VOID	OSCriticalSectionRelease(	IN	HANDLE hMutex) 
{
#if 0
	PCRIT_SEC_T crit_sec = (PCRIT_SEC_T) hMutex; 
	thread_id tid = find_thread(0); 

	if (inittime)
		return; 
	if(crit_sec->cstid == tid) 
	{ 
		 
		crit_sec->count--; 
		if(crit_sec->count <= 0) 
		{
			uint64 curr_time = system_time();
			uint64 delta_time = curr_time - crit_sec->lastacquire;
			crit_sec->alldeltas+=delta_time;
			crit_sec->numinsample++;
			crit_sec->avetime=crit_sec->alldeltas/crit_sec->numinsample;
//kprintf("delta time is %d\n",delta_time);
//kprintf("maxtime is %d\n",crit_sec->maxtime);			
			if (delta_time > crit_sec->maxtime)
				crit_sec->maxtime= delta_time;
//kprintf("new maxtime is %d\n",crit_sec->maxtime);
			if (crit_sec->lastinterval +50000 > curr_time)
			{
//				kprintf("system time: %d\n", curr_time);
//				kprintf("system time+50000000: %d\n", curr_time+50000000);
				kprintf("lastinterval: %d\n",crit_sec->lastinterval);
				kprintf("CS: Average: %dus\n",crit_sec->avetime);
				kprintf("Maximum: %dus\n",crit_sec->maxtime);
				kprintf("numintervals: %d\n\n",crit_sec->numinsample);
				crit_sec->lastinterval = curr_time;

			}	
			crit_sec->count = 0; 
			crit_sec->cstid = B_BAD_THREAD_ID; 
			release_spinlock(&crit_sec->atom);  
			restore_interrupts(crit_sec->status); 
		} 
	} 
#endif
}
#endif
 

/********************************************************************/
GLOBAL	VOID	OSSetTimeSensitivity(	IN	UINT32				Interval)			
{
	// The assumption is that since the timer resolution of the Be timer is
	// in the us range, there is no need for this call.  (much less, a way
	// to implement it.) 
	
}
/********************************************************************/
GLOBAL	VOID	OSRestoreTimeSensitivity(		IN	UINT32				Interval)
{
}

/********************************************************************/
GLOBAL	HANDLE	OSDisableInterrupts()
{
	cpu_status	ps;
//dbgkprint("Disable interupts\n");
	ps = disable_interrupts();
	
	
	return( (HANDLE)ps);
}

/********************************************************************/
GLOBAL	VOID	OSRestoreInterrupts(	IN	HANDLE hStatus)
{
//dbgkprint("Enable interrupts\n");
 	restore_interrupts( (cpu_status)hStatus );
}
