/****************************************************************************************
*                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/TestDebug.h_v   1.14   29 Jun 2000 11:50:08   lisitse  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			TestDebug.h

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


#ifndef __TESTDEBUG_H__
#define __TESTDEBUG_H__

UINT32	OSGetSystemTime(void);

//
// !!!!     This header is for development/testing/debug purposes only          !!!!!
// !!!!     It shouldn't be included in final version of any OCTOPUS files      !!!!!
//

// RMY - release tracing    
#if defined(_DEBUG) || defined(RELEASE_TRACE)

#define TRACE_FILE_NAME "USBModemLog"

#ifndef ASSERT
#define ASSERT(x)   if(!(x)) {                                                                               \
                        TRACE (T__ASSERT, ("Assertion failed, File: %s, Line: %d\n", __FILE__, __LINE__));   \
                        /*__asm int 3*/				                                                             \
                    }
#endif	// ASSERT

extern unsigned long ulTraceMask;

#define	T__FATAL_ERROR	0x80000000
#define T__ERROR		0x40000000
#define T__ASSERT		0x20000000

#define T__DEVMGR		0x00000001
#define T__CALLPROG		0x00000002
#define T__SESSIONMGR	0x00000004
#define T__STREAMMGR	0x00000008
#define T__DATASESSION	0x00000010
//#define T__ECPIM		0x00000020
#define T__MNP			0x00000020
#define T__V42			0x00000040
#define T__V42BIS		0x00000080
#define T__FAXSESSION	0x00000100
#define T__FAXPIM		0x00000200
#define T__ATPARSER		0x00000400
#define T__COMMCTRL		0x00000800
#define T__UTILS		0x00001000
#define T__RTMGR		0x00002000
#define T__CONFIGMGR    0x00004000
#define T__SERIALSHIM   0x00008000
#define T__V8BIS        0x00010000
#define T__STATMGR      0x00020000
#define T__HAL          0x00040000
#define T__HALOS		0x00080000
#define T__LMON         0x00100000
#define T__PORT         0x00200000
#define T__V8           0x00400000
#define T__DOWNLOAD     0x00800000
#define T__HDLC			0x01000000
#define T__PROFILE		0x02000000
#define T__NVMMGR		0x04000000
#define T__TESTSESSION	0x08000000	  //PQ

//extern unsigned long DbgPrint(char* Format, ... );
#define DbgPrint dprintf

#define TRACE(mask,printf) if ((mask) & ulTraceMask) DbgPrint printf

#define DUMP(ptr, size) DbgDump(ptr, size);

void TraceInit(void);
void TraceClose(void);

#ifdef BREAKONERROR
	#ifdef MACTOPUS
		#include <mactypes.h>	// debugstr() prototype
		
		#define BREAKPOINT(BreakMsg)	DebugStr("\p" BreakMsg );
	#else
		#define BREAKPOINT(BreakMsg)	{ DbgPrint(BreakMsg); __asm int 3 }
	#endif
#else
	#define BREAKPOINT(BreakMsg) 		TRACE(T__ERROR, (BreakMsg));
#endif	



#else	// _DEBUG

	#define TraceInit()
	#define TraceClose()
	#define TRACE(x,y)
	#define DUMP(ptr, size)
    #define DPRINTF(x) 
    #define ASSERT(x)
	#define BREAKPOINT(BreakMsg)    
#endif




#endif  // #ifndef __DEBUG_H__
