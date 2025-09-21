/*========================================================================
   File:	Osal_BeOS.cpp
   Purpose:	TsOsalAPI implementation for BeOS.
 
   Author:	Bryan Pong , May 2000
   Modifications:

             		Copyright (C) 2000 by Trisignal communications Inc.
                            	     All Rights Reserved
  =========================================================================*/

#include <KernelExport.h> // To get a new timer interrupt
#include <malloc.h>       // For malloc functions

#include "HostCommon.h"
#include "target.h"
#include "Osal_BeOS.hpp"

static spinlock timeout_lock= 0;
static bool timeout_installed= false;
static timer timeout_timer;

extern void RoundRobinModemCallbacks(void);
/********************************************************************************************/
/*                              Memory allocation                                           */
/********************************************************************************************/
void* OsalBeOS::AllocLockedMem(UINT32 NBytes)
{
	return malloc(NBytes);
}

void OsalBeOS::FreeLockedMem(void* LockedMemPtr)
{
    if(LockedMemPtr) 
    {
        free(LockedMemPtr);
    }
}

void* OsalBeOS::AllocUnLockedMem(UINT32 NBytes)
{
	return malloc(NBytes);
}

void OsalBeOS::FreeUnLockedMem(void* UnLockedMemPtr)
{
    if(UnLockedMemPtr) 
    {
        free(UnLockedMemPtr);
    }
}

/********************************************************************************************/
/*                                  Timer Tick                                              */
/********************************************************************************************/

// Return system time in ms
UINT32 OsalBeOS::GetTickCount(void)
{
    bigtime_t time;
    
    //return system time since PC was booted
    time = system_time();
    //change to ms
    return (time / 1000);
}

UINT16 OsalBeOS::GetTickCountPrecision(void)
{
    return 1; // ms
}

//
// Returns s since an arbitrary time which should not change.
// Returns seconds since 1/1/1970.
// Used for blacklisting...
//
UINT32 OsalBeOS::GetRealTimeClock(void)
{
    return real_time_clock();
}

/* Call this function every 10ms */
static int32 TimerCallback(timer* /* tick_signal */)
{
//    target_test_pin(3, TRUE);
    RoundRobinModemCallbacks();
//    dprintf("Run Round Robin\n");
//    target_test_pin(3, FALSE);

	return B_OK;
}

/* To start the timer */
status_t OsalBeOS::Start() 
{
    /* Set every shot period to 10ms */ 
	status_t ps;
	status_t retVal;

	(void)(timeout_lock); /* prevent warnings for non SMP builds */

	ps= disable_interrupts();
	acquire_spinlock(&timeout_lock);
	if(timeout_installed) {
		retVal= B_ERROR;
	} else {
    	add_timer(&timeout_timer, TimerCallback, 10000, B_PERIODIC_TIMER);
		timeout_installed= true;
		retVal= B_OK;
	}
	release_spinlock(&timeout_lock);
	restore_interrupts(ps);

	return retVal;
} 

/* To stop the timer */ 
bool OsalBeOS::Stop() 
{ 
	status_t ps;
	status_t retVal;

	ps= disable_interrupts();
	acquire_spinlock(&timeout_lock);
	if(timeout_installed) {
    	cancel_timer(&timeout_timer); 
		timeout_installed= false;
		retVal= B_OK;
	} else {
		retVal= B_ERROR;
	}
	release_spinlock(&timeout_lock);
	restore_interrupts(ps);

	return retVal;
} 

/********************************************************************************************/
/*                  Critical section / Atomic operation                                     */
/********************************************************************************************/
UINT32 OsalBeOS::CreateCriticalSection(void)
{
    return 0;
}

void OsalBeOS::DestroyCriticalSection(UINT32 /* CSID */)
{
}

void OsalBeOS::EnterCriticalSection(UINT32 /* CSID */)
{
}

void OsalBeOS::LeaveCriticalSection(UINT32 /* CSID */)
{
}

UINT32 OsalBeOS::EnterAtomicSection (void)
{
	return 0;
}

void OsalBeOS::LeaveAtomicSection (UINT32 /* EnterState */)
{
}

/********************************************************************************************/
/*                              optional DEBUG services                                     */ 
/********************************************************************************************/
void OsalBeOS::DebugPrintf(CHAR *, ...)
{
//    *fmt;
}

void OsalBeOS::DebugPrint(CHAR *  /* strDebugMessage */)
{
#ifdef BE_DEBUG
    dprintf(strDebugMessage);
#endif
}

void OsalBeOS::DebugBreakPoint(void)
{
}

void dbgprint(char *  /* str */)
{
#ifdef BE_DEBUG
    dprintf(str); 
#endif
}
