#ifndef __OSTIME_IN_H__
#define __OSTIME_IN_H__

/*osTimeI.h*/
/* Internal osTimeOut types :*/
typedef struct TIME_OUT_INSTANCE_TYPE
{
//	TIMEOUT_THUNK	TimeOuthunk;
	UINT32			mSec;
	UINT32          hTimeOutHandle;
	BOOL			bLocked;
	BOOL			TimerActive;

	PFREE_FUNC		pFuncFree;
	PVOID			pRefData;

	PCBFUNC			pTimeOutCallBack;

	thread_id       hThread;
	sem_id          hSemaphore;

}TIME_OUT_INSTANCE_T, *PTIME_OUT_INSTANCE_T;

#if 0
typedef struct CRITICAL_SECTION_TYPE
{
	thread_id	cstid; // thread_id of CS owner
	UINT32		count; // count of NESTED Aquires (will be zero if no nested Aquires)
	sem_id		sem;   // sem
} CRIT_SEC_T, *PCRIT_SEC_T;
#endif

typedef struct CRITICAL_SECTION_TYPE 
{ 
	thread_id		cstid; // thread_id of CS owner 
	UINT32			count; // count of NESTED Acquires (will be zero if no nested Aquires) 
	spinlock		atom; 
	cpu_status		status;
	uint64			alldeltas, maxtime, avetime, lastacquire, lastinterval, numinsample; 
} CRIT_SEC_T, *PCRIT_SEC_T; 

//typedef struct STACK_CHANGE_INFO_TAG
//{
//	PCBFUNC		pFunc;
//	UINT32		lParam;
//}STACK_CHANGE_INFO_T, *PSTACK_CHANGE_INFO_T;

/* private osTimeOut functions :*/
//STATIC VOID __stdcall TimeOutHandler(HANDLE hVM, PCLIENT_STRUCT pcrs, PVOID RefData, UINT32 Extra);
STATIC int32 TimerThreadFunction(void * hTimeOut);
//STATIC VOID  _cdecl FuncOnNewStack(UINT32       lParam);   
#endif
