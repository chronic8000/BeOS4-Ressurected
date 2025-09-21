/*========================================================================
   File:	Osal_BeOS.hpp
   Purpose:	Defintions of basic structures/classes 

 
   Author:	Bryan Pong, April 2000
   Modifications:	

             		Copyright (C) 2000 by Trisignal communications Inc.
                            	     All Rights Reserved
  =========================================================================*/
#ifndef OSALBEOS_HPP
#define OSALBEOS_HPP

#include <SupportDefs.h>
#include "HostCommon.h"
#include "TsOsalAPI.hpp"
#include <scheduler.h>   // Needed for timer operations


/******************************************************************************

Public class definition

******************************************************************************/

class OsalBeOS : public TsOsalAPI
{
public:
    OsalBeOS();
    /*****************************************************/
    /* The following services must be provided by the    */
    /* HOST driver to the generic modem core module      */
    /*****************************************************/
    /* Memory allocation */
    void*  AllocLockedMem( UINT32 NBytes );
    void   FreeLockedMem( void* LockedMemPtr );
    void*  AllocUnLockedMem( UINT32 NBytes );
    void   FreeUnLockedMem( void* LockedMemPtr );
    /* Timer Tick */
    UINT32 GetTickCount( void );
    UINT16 GetTickCountPrecision( void );
    UINT32 GetRealTimeClock( void );
    /* Timeout callbacks */
    UINT32 RegisterTimeoutCallback( void  (*CallbackFnct)(void*,UINT16), 
                                    void*   pContext , 
                                    UINT16  TimeoutPeriod , 
                                    UINT16  TType,
                                    UINT16  Priority);
    UINT16 UnRegisterTimeoutCallback( UINT32 CallbackID );
    UINT16 SupportTaskPrioritization( void );

    /* Critical section */
    UINT32 CreateCriticalSection( void );
    void   DestroyCriticalSection( UINT32 CSID );
    void   EnterCriticalSection( UINT32 CSID );
    void   LeaveCriticalSection( UINT32 CSID );

    /* Atomic operation */
    UINT32 EnterAtomicSection (void);
    void   LeaveAtomicSection (UINT32 EnterState);

    /* Non-volatile storage */
    BOOL   OpenNonVStorage( UINT16 StorageID, UINT16 NVSize );
    void   ReadNonVStorage( UINT16 StorageID, 
                            UINT16 usOffset, 
                            UINT16 OutBufSize, 
                            BYTE*  OutBufPtr, 
                            UINT16* BytesReadPtr);
    //BytesReadPtr contains zero before first WriteNonVStorage or after DeleteNonVStorage
    void   WriteNonVStorage(UINT16 StorageID, 
                            UINT16 usOffset, 
                            UINT16 InBufSize, 
                            BYTE*  InBufPtr, 
                            UINT16* BytesWrittenPtr);
    void   DeleteNonVStorage(UINT16 StorageID);
    void   CloseNonVStorage(UINT16 StorageID);
    void   FlushNonVStorage(UINT16 /*StorageID*/) {return;};

    /* Volatile storage */
    BOOL   OpenVStorage(UINT16 StorageID, UINT16 NVSize );
    void   ReadVStorage(UINT16 StorageID,
                        UINT16 usOffset, 
                        UINT16 OutBufSize, 
                        BYTE*  OutBufPtr, 
                        UINT16* BytesReadPtr);
    //BytesReadPtr contains zero before first WriteVStorage or after DeleteVStorage
    void   WriteVStorage( UINT16 StorageID,
                          UINT16 usOffset, 
                          UINT16 InBufSize, 
                          BYTE*  InBufPtr, 
                          UINT16* BytesWrittenPtr);
    void   DeleteVStorage(UINT16 StorageID);
    void   CloseVStorage(UINT16 StorageID);

    /* optional DEBUG services */
	void   DebugPrintf(CHAR *fmt, ...);
    void   DebugPrint(CHAR *strDebugMessage);
    void   DebugBreakPoint(void);

public:
    /* Start/Stop timer */
    status_t  Start( void );
    bool      Stop( void );

    int       *fileDescriptor;
    UINT16    *totalStorageSize;
    char      **fileName;
};

#endif /*OSALBEOS_HPP*/
