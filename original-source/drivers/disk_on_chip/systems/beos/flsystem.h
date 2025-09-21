/*
 * $Log:   V:/FLSYSTEM.H_V  $
   
      Rev 1.7   03 Sep 1998 13:58:58   ANDRY
   better DEBUG_PRINT

      Rev 1.6   26 Feb 1998 15:55:10   amirban
   Reinstate 386 code

      Rev 1.5   03 Nov 1997 16:27:14   danig
   pointerToPhysical

      Rev 1.4   11 Sep 1997 14:14:22   danig
   physicalToPointer receives drive no. when FAR == 0

      Rev 1.3   04 Sep 1997 13:58:30   danig
   DEBUG_PRINT

      Rev 1.2   28 Aug 1997 16:39:32   danig
   include stdlib.h instead of malloc.h

      Rev 1.1   19 Aug 1997 20:05:06   danig
   Andray's changes

      Rev 1.0   24 Jul 1997 18:13:06   amirban
   Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

/*	Customization for BeOS. Copyright (c) Be Inc. 2000	*/

#ifndef FLSYSTEM_H
#define FLSYSTEM_H

#include <OS.h>
#include <KernelExport.h>
#include <lock.h> 

/*
 * 			signed/unsigned char
 *
 * It is assumed that 'char' is signed. If this is not your compiler
 * default, use compiler switches, or insert a #pragma here to define this.
 *
 */



/* 			CPU target
 *
 * Use compiler switches or insert a #pragma here to select the CPU type
 * you are targeting.
 *
 */



/* 			NULL constant
 *
 * Some compilers require a different definition for the NULL pointer
 */



/* 			Little-endian/big-endian
 *
 * FAT and translation layers structures use the little-endian (Intel)
 * format for integers.
 * If your machine uses the big-endian (Motorola) format, uncomment the
 * following line.
 * Note that even on big-endian machines you may omit the BIG_ENDIAN
 * definition for smaller code size and better performance, but your media
 * will not be compatible with standard FAT and FTL.
 */

/* #define BIG_ENDIAN */


#define	FAR_LEVEL	0

/* 			Memory routines
 *
 * You need to supply library routines to copy, set and compare blocks of
 * memory, internally and to/from callers. The code uses the names 'tffscpy',
 * 'tffsset' and 'tffscmp' with parameters as in the standard 'memcpy',
 * 'memset' and 'memcmp' C library routines.
 */


#include <string.h>

#define tffscpy		memcpy
#define tffsset		memset
#define tffscmp		memcmp


extern void* my_memcpy(void* dest, const void* src, size_t n);
extern void* my_memset(void* dest, int c,  size_t n);
extern int my_memcmp(const void* src1, const void* src2, size_t n);


/* 			Pointer arithmetic
 *
 * The following macros define machine- and compiler-dependent macros for
 * handling pointers to physical window addresses. 
 *
 * 'physicalToPointer' translates a physical flat address to a (far) pointer.
 * Note that if when your processor uses virtual memory, the code should
 * map the physical address to virtual memory, and return a pointer to that
 * memory (the size parameter tells how much memory should be mapped).
 *
 * 'addToFarPointer' adds an increment to a pointer and returns a new
 * pointer. The increment may be as large as your window size. The code
 * below assumes that the increment may be larger than 64 KB and so performs
 * huge pointer arithmetic.
 */

/*
#define physicalToPointer(physical,size,drive)	        \
	((void *) (physical))	
*/

extern void* physicalToPointer(unsigned long physical, size_t size, unsigned drive);

#define addToFarPointer(base,increment)		\
	((void *) ((unsigned char *) (base) + (increment)))

#define flAddLongToFarPointer	addToFarPointer

/* 			Default calling convention
 *
 * C compilers usually use the C calling convention to routines (cdecl), but
 * often can also use the pascal calling convention, which is somewhat more
 * economical in code size. Some compilers also have specialized calling
 * conventions which may be suitable. Use compiler switches or insert a
 * #pragma here to select your favorite calling convention.
 */




/* 			Mutex type
 *
 * If you intend to access the FLite API in a multi-tasking environment,
 * you may need to implement some resource management and mutual-exclusion
 * of FLite with mutex & semaphore services that are available to you. In
 * this case, define here the Mutex type you will use, and provide your own
 * implementation of the Mutex functions incustom.c
 *
 * By default, a Mutex is defined as a simple counter, and the Mutex
 * functions in custom.c implement locking and unlocking by incrementing
 * and decrementing the counter. This will work well on all single-tasking
 * environment, as well as on many multi-tasking environments.
 */

typedef lock FLMutex;


/* 			Memory allocation
 *
 * The translation layers (e.g. FTL) need to allocate memory to handle
 * Flash media. The size needed depends on the media being handled.
 *
 * You may choose to use the standard 'malloc' and 'free' to handle such
 * memory allocations, provide your own equivalent routines, or you may
 * choose not to define any memory allocation routine. In this case, the
 * memory will be allocated statically at compile-time on the assumption of
 * the largest media configuration you need to support. This is the simplest
 * choice, but may cause your RAM requirements to be larger than you
 * actually need.
 *
 * If you define routines other than malloc & free, they should have the
 * same parameters and return types as malloc & free. You should either code
 * these routines in flcustom.c or include them when you link your application.
 */

#include <stdlib.h>

#define MALLOC malloc
#define FREE free


/*			Debug mode
 *
 * Uncomment the following lines if you want debug messages to be printed
 * out. Messages will be printed at initialization key points, and when
 * low-level errors occure.
 * You may choose to use 'printf' or provide your own routine.
 */
#include "debugoutput.h"
#define DEBUG_PRINT(str)  DE(1) dprintf str 


#endif