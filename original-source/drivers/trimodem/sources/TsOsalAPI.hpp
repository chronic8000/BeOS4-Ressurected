#ifndef _TS_OSAL_API_H
#define _TS_OSAL_API_H

#include "TsType.h"

/* DebugPrint() as been seen as a macro. in such case, the compiler generates
   an error when it replaces the method name DebugPrint with the macro. */
#ifdef  DebugPrint
#undef  DebugPrint
#endif

/**************************/
/* callback timeout type */
/**************************/
#define ONE_SHOT            100 /* the registered callback will be called only once and unregistered */
#define PERIODIC            101 /* the registered callback will be periodically called */

/**************************/
/* timeout callback state */
/**************************/
#define UNREG_SUCCESS       200 /* The provided callback ID is valid and successfully unregistered: 
                                    the OS will not invoked this callback again. */
#define CALLBACK_PENDING    201 /* The callback is currently running.  
                                    The OS will complete the unregister operation on the 
                                    callback when the callback completes its task.*/
#define INVALID_CALLBACK_ID 202 /* The provided callback ID has not been recognized 
                                    as a valid callback ID. */

/***************************************************************
  General storageID used by GMC and OS dependant
  implementation of volatile/non-volatile storage functionality
 ****************************************************************/
enum STORAGEID
{
    MODEM_CONFIG_STORAGEID,
    FAX_CONFIG_STORAGEID,
    EEPROM_STORAGEID,
    BLACK_LISTING_STORAGEID,
    MAX_STORAGEID,    /* Total number of storage ID */

//    TEMP_SREGISTERS_STORAGEID,    //this storage area can be destroyed when OS session ends.
};

/**************************/
/* Virtual class of OSAL  */
/**************************/
class TsOsalAPI
{
public:

	// For some odd reason the BeOS compiler can't link if the
	// constructor and destructor are present.  Lee
#ifndef __BEOS__
    TsOsalAPI() {/* GNU Compiler needs me! */}
    virtual ~TsOsalAPI() {/* GNU Compiler needs me! */}
#endif
    
    /*****************************************************/
    /* The following services must be provided by the    */
    /* HOST driver to the generic modem core module      */
    /*****************************************************/
    /* Memory allocation */
    virtual void*       AllocLockedMem( UINT32 NBytes )       = 0;
    virtual void        FreeLockedMem( void* LockedMemPtr )   = 0;
    virtual void*       AllocUnLockedMem( UINT32 NBytes )     = 0;
    virtual void        FreeUnLockedMem( void* LockedMemPtr ) = 0;
    /* Timer Tick */
    virtual UINT32      GetTickCount( void )          = 0;
    virtual UINT16      GetTickCountPrecision( void ) = 0;
    virtual UINT32      GetRealTimeClock( void )      = 0;
    /* Timeout callbacks */
    virtual UINT32      RegisterTimeoutCallback( void  (*CallbackFnct)(void*,UINT16) , 
                                                      void*   pContext , 
                                                      UINT16  TimeoutPeriod , 
                                                      UINT16  TType,
                                                      UINT16  Priority ) = 0;
    virtual UINT16      UnRegisterTimeoutCallback( UINT32 CallbackID ) = 0;
    virtual UINT16      SupportTaskPrioritization( void )       = 0;

    /* Critical section */
    virtual UINT32      CreateCriticalSection( void )         = 0;
    virtual void        DestroyCriticalSection( UINT32 CSID ) = 0;
    virtual void        EnterCriticalSection( UINT32 CSID )   = 0;
    virtual void        LeaveCriticalSection( UINT32 CSID )   = 0;

    /* Atomic operation */
    virtual UINT32      EnterAtomicSection (void)              = 0;
    virtual void        LeaveAtomicSection (UINT32 EnterState) = 0;

    /* Non-volatile storage */
    virtual BOOL        OpenNonVStorage( UINT16 StorageID, UINT16 NVSize ) = 0;
    virtual void        ReadNonVStorage( UINT16 StorageID, 
                                         UINT16 usOffset, 
                                         UINT16 OutBufSize, 
                                         BYTE*  OutBufPtr, 
                                         UINT16* BytesReadPtr) = 0;
    //BytesReadPtr contains zero before first WriteNonVStorage or after DeleteNonVStorage
    virtual void        WriteNonVStorage(UINT16 StorageID, 
                                         UINT16 usOffset, 
                                         UINT16 InBufSize, 
                                         BYTE*  InBufPtr, 
                                         UINT16* BytesWrittenPtr) = 0;
    virtual void        DeleteNonVStorage(UINT16 StorageID)       = 0;

    // force to transfer data to non-volatile storage device
    // when data is buffered by WriteNonVStorage()
    // FlushNonVStorage() is an optional OSAL service so a default implementation
    // is provided.
    virtual void        CloseNonVStorage(UINT16 StorageID) = 0;
    virtual void        FlushNonVStorage(UINT16 StorageID) = 0;
    /* Volatile storage */
    virtual BOOL        OpenVStorage(UINT16 StorageID, UINT16 NVSize ) = 0;
    virtual void        ReadVStorage(UINT16 StorageID,
                                     UINT16 usOffset, 
                                     UINT16 OutBufSize, 
                                     BYTE*  OutBufPtr, 
                                     UINT16* BytesReadPtr) = 0;
    //BytesReadPtr contains zero before first WriteVStorage or after DeleteVStorage
    virtual void        WriteVStorage( UINT16 StorageID,
                                       UINT16 usOffset, 
                                       UINT16 InBufSize, 
                                       BYTE*  InBufPtr, 
                                       UINT16* BytesWrittenPtr) = 0;
    virtual void        DeleteVStorage(UINT16 StorageID) = 0;
    virtual void        CloseVStorage(UINT16 StorageID)  = 0;

    /* optional DEBUG services */
    virtual void        DebugPrintf(CHAR *fmt, ...)       = 0; // sprintf type formated
    virtual void        DebugPrint(CHAR *strDebugMessage) = 0; // simple string message
    virtual void        DebugBreakPoint(void)             = 0;

};

extern TsOsalAPI*  OsalPtr;

#endif // _TS_OSAL_API_H
