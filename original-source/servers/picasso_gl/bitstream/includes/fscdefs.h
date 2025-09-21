/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/true_type/fscdefs.h,v 14.0 97/03/17 17:53:45 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	fscdefs.h,v $
 * Revision 14.0  97/03/17  17:53:45  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.2  97/01/14  17:37:09  shawn
 * Changed int LoopCount to int16 LoopCount.
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 10.1  96/05/31  16:10:44  shawn
 * Cleaned up casting (ANSI)
 * 
 * Revision 10.0  95/02/15  16:23:23  roberte
 * Release
 * 
 * Revision 9.2  95/01/11  13:35:23  shawn
 * Changed the typedef for voidFunc to void (*voidFunc)(void)
 * 
 * Revision 9.1  95/01/04  16:33:43  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  13:18:42  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/07/19  10:45:03  roberte
 * Removed stray '#' and silly nested comment around DEBUG.
 * Added default #define of INCL_VERTICAL to 0.
 * 
 * Revision 8.0  94/05/04  09:29:40  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:57:34  roberte
 * Release
 * 
 * Revision 6.94  94/03/30  14:46:39  roberte
 * Reinserted extern for memset() and removed arguments from prototype.
 * 
 * Revision 6.93  94/03/30  14:43:20  roberte
 * Removed extern of memset prototype
 * 
 * Revision 6.92  94/03/18  14:00:57  roberte
 * Added extern statement for UNIX memset() function as in MEMSET macro.
 * 
 * Revision 6.91  93/08/30  14:51:06  roberte
 * Release
 * 
 * Revision 6.46  93/06/24  17:42:41  roberte
 * Removed some #ifdef OLDWAY conditionals that were dead.
 * Added macro MEMSET and macro FLOOR which add speed on
 * systems supporting memset() type functionality and have 
 * floats rounding towards zero when assigned to integers.
 * 
 * Revision 6.45  93/05/18  09:41:17  roberte
 * Tightened up some macros to get better performance.
 * 
 * Revision 6.44  93/03/15  13:11:32  roberte
 * Release
 * 
 * Revision 6.12  93/03/11  15:51:50  roberte
 * Changed #if __MSDOS to #ifdef MSDOS.
 * 
 * Revision 6.11  93/03/09  12:19:54  roberte
 * Changed test for signed defines to use || __MSDOS rather than || INTEL
 * 
 * Revision 6.10  93/03/04  11:49:30  roberte
 * Used #if _STDC_ and etc test for adding "signed" modifier in some declarations.
 * These were int8, int16 and int32
 * 
 * Revision 6.9  93/03/03  16:55:53  mark
 * change SWAPWINC macro to return (short) instead of (unsigned short) since it
 * is used to unpack the CVT which contains positive and negative values.
 * 
 * Revision 6.8  93/03/03  11:16:48  mark
 * make ArrayIndex 32 bits on Intel platforms as well since
 * ArrayIndexes are pushed and popped of the (32bit) stack
 * 
 * Revision 6.7  93/02/24  17:37:34  weili
 * Changed incorrect prototype of GetSFNTFunc implying params as long, long, long.
 * 
 * This was causing a problem with prototyping and declaring the tt_get_font_fragment()
 * function in general, when prototyping was used.
 * 
 * Revision 6.6  93/01/08  10:25:27  roberte
 * Put #ifndef true check arounf define of true and false.
 * 
 * Revision 6.5  93/01/06  13:20:57  roberte
 * Got rid of extraneous define of boolean.
 * 
 * Revision 6.4  92/12/29  12:45:10  roberte
 * Changed #ifdef INTEL to #if INTEL so can be compatible
 * with 4-in-1 and speedo.h
 * 
 * Revision 6.3  92/11/13  16:48:26  roberte
 * Moved SHORTMUL and USHORTMUL macros outside of any block,
 * using 32 bit multiply.  These did bad things ifdef INTEL (as 16 bit
 * multiplies) when on some compilers (Crosscode 68000).
 * 
 * Revision 6.2  92/09/28  15:55:36  roberte
 * Changed "#define boolean int" to "#define boolean unsigned char"
 * 
 * Revision 6.1  91/08/14  16:45:38  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:48  mark
 * Release
 * 
 * Revision 4.3  91/08/07  11:52:07  mark
 * remove rcsstatus string
 * 
 * Revision 4.2  91/08/07  11:43:22  mark
 * added RCS control strings
 * 
*************************************************************************************/

/*
	File:		FSCdefs.h

	Contains:	xxx put contents here (or delete the whole line) xxx

	Written by:	xxx put name of writer here (or delete the whole line) xxx

	Copyright:	) 1988-1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		 <3>	11/27/90	MR		Add #define for PASCAL. [ph]
		 <2>	 11/5/90	MR		Move USHORTMUL from FontMath.h, add Debug definition [rb]
		 <7>	 7/18/90	MR		Add byte swapping macros for INTEL, moved rounding macros from
									fnt.h to here
		 <6>	 7/14/90	MR		changed defines to typedefs for int[8,16,32] and others
		 <5>	 7/13/90	MR		Declared ReleaseSFNTFunc and GetSFNTFunc
		 <4>	  5/3/90	RB		cant remember any changes
		 <3>	 3/20/90	CL		type changes for Microsoft
		 <2>	 2/27/90	CL		getting bbs headers
	   <3.0>	 8/28/89	sjk		Cleanup and one transformation bugfix
	   <2.2>	 8/14/89	sjk		1 point contours now OK
	   <2.1>	  8/8/89	sjk		Improved encryption handling
	   <2.0>	  8/2/89	sjk		Just fixed EASE comment
	   <1.5>	  8/1/89	sjk		Added composites and encryption. Plus some enhancementsI
	   <1.4>	 6/13/89	SJK		Comment
	   <1.3>	  6/2/89	CEL		16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.2>	 5/26/89	CEL		EASE messed up on RcS comments
	  <%1.1>	 5/26/89	CEL		Integrated the new Font Scaler 1.0 into Spline Fonts
	   <1.0>	 5/25/89	CEL		Integrated 1.0 Font scaler into Bass code for the first timeI

	To Do:
*/

#ifndef fscdefs_h
#define fscdefs_h
/* #define DEBUG			*/

/* default vertical character substitution code not included: */
#ifndef INCL_VERTICAL
#define INCL_VERTICAL	0
#endif


#define RELEASE_MEM_FRAG 1

#ifdef NOT_ON_THE_MAC
#define PASCAL
#else
#define PASCAL		pascal
#endif

#ifndef true
#define true 1
#define false 0
#endif
#define ONEFIX 		( 1L << 16 )
#define ONEFRAC 	( 1L << 30 )
#define ONEHALFFIX	0x8000L
#define ONEVECSHIFT	16
#define HALFVECDIV	(1L << (ONEVECSHIFT-1))

/* To prevent annoying warnings */
#ifdef __BEOS__

#include <SupportDefs.h>

#else

#if __STDC__ || defined(sgi) || defined(AIXV3) || defined(_IBMR2) || defined(MSDOS)
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
#else
typedef char int8;
typedef short int16;
typedef long int32;
#endif
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

#endif

typedef short FWord;
typedef unsigned short uFWord;
typedef long F26Dot6;

#ifndef Fixed
#define Fixed long
#endif

#ifndef Fract
#define Fract long
#endif

typedef void (*voidFunc)(void);

typedef void* VoidPtr;

typedef void	(*ReleaseSFNTFunc)(void*);
typedef void*	(*GetSFNTFunc)(int32, int32, int32);

typedef int16	LoopCount;
typedef int32	ArrayIndex;

#if INTEL

#define SWAPW(a)		(short)(((unsigned char)((a) >> 8)) | ((unsigned char)(a) << 8))
#define SWAPWINC(a)		SWAPW(*(a)); a++
#define SWAPL(a)		((((a)&0xff) << 24) | (((a)&0xff00) << 8) | (((a)&0xff0000) >> 8) | ((a) >> 24))

#define SHORTDIV(a,b)	(int32)((int32)(a) / (int32)(b))

#else

#define SWAPW(a)		(a)
#define SWAPWINC(a)		(*(a)++)
#define SWAPL(a)		(a)

#define SHORTDIV(a,b)	(int32)((int16)(a) / (int16)(b))

#endif	/* INTEL */

#define USHORTMUL(a, b)	((uint32)((uint32)(a)*(uint32)(b)))
#define SHORTMUL(a,b)	(int32)((int32)(a) * (int32)(b))

/* d is half of the denumerator */
#define FROUND( x, n, d, s ) \
	    x = (SHORTMUL(x, n) + d) >> s;

/* <3> */
#define SROUND( x, n, d, halfd ) \
    if ( x > -1 ) { \
	    x = (SHORTMUL(x, n) + halfd)/d; \
	} else { \
	    x = -((SHORTMUL(-x, n) + halfd) / d); \
	}

#ifdef DEBUG
#ifndef NOT_ON_THE_MAC
#ifndef __TYPES__
	pascal void Debugger(void) = 0xA9FF; 
	pascal void DebugStr(char* aStr) = 0xABFF; 
#endif
/* PRR Modif begin */
#else
	#define Debugger() *((char*)0L) = 0
#endif
/* PRR Modif end */
#endif

#define MEMSET(dst, value, size)    memset(dst, value, (size_t)(size))

/* if your compiler won't round doubles towards zero, don't do the following: */
#define FLOOR(x)	x
/* use this instead: */
/* #define FLOOR(x)	floor(x) */

#endif /* ifndef fscdefs_h */
