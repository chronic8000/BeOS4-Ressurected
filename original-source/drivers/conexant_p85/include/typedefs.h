/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/TypeDefs.h_v   1.3   20 Apr 2000 14:29:44   woodrw  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			Typedefs.h	

File Description:	Basic Type Definitions

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


#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

#ifndef __OSTYPEDEFS_H__

#ifndef MACTOPUS
#pragma warning(disable : 4001)
#pragma warning(disable : 4100)
#endif

// typedef enum {FALSE, TRUE} BOOL;
typedef int BOOL;
#define FALSE	0
#define TRUE	1


#define	NULL	((void *)0)
typedef	void			VOID,	*PVOID,	**PPVOID;


#define LPVOID void* 
#define	PCHAR  char*

#define UINT32 unsigned long
#define PUINT32 UINT32*		

#define INT32   long
#define PINT32  long*

#define UINT16 unsigned short
#define PUINT16 UINT16*		

#define UINT8 unsigned char
#define BYTE unsigned char
#define PUINT8 UINT8*	

#define INT8 signed char
#define PINT8 INT8*

#define INT16 signed short
#define PINT16 INT16*

#define HANDLE PVOID	

#define STRING char*	
typedef char* LPSTR;
typedef const char* LPCSTR;



/* The following definitions should be used for each parameter in a function's parameters list. */
#define IN
#define OUT
#define IO


/* The following definitions should be used with each function's definition. */
#define GLOBAL
#define STATIC		static

#ifndef min
#define min(x,y) ((x) < (y)  ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y)  ? (x) : (y))
#endif

#endif

#endif /* __TYPEDEFS_H__ */


 