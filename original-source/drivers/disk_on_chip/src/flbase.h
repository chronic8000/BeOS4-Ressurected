/*
 * $Log:   V:/Flite/archives/FLite/src/FLBASE.H_V  $
   
      Rev 1.22   Mar 14 2000 16:14:36   vadimk
   stdmemcpy/set/cmp were removed
   NAMING_CONVENTION was added to the declaration of
   flmemcpy/set/cmp and buffercpy,bufferset,buffercmp

      Rev 1.21   12 Mar 2000 16:40:40   dimitrys
   Get rid of warnings (unsigned -> unsigned short)

      Rev 1.20   Mar 07 2000 11:11:08   vadimk
   ycpu definition was removed
   flsleep declaration was taken out of ENVIRONMENT_VARS

      Rev 1.19   Feb 03 2000 17:47:54   vadimk
   Remove flyieldcpu

      Rev 1.18   Jan 19 2000 14:43:32   vadimk
   "fl" prefix added to the environment variables

      Rev 1.17   Jan 13 2000 18:16:44   vadimk
   TrueFFS OSAK 4.1
 *
 *    Rev 1.3   Jan 13 2000 18:12:44   vadimk
 * TrueFFS OSAK 4.1

      Rev 1.16   23 Feb 1998 17:08:00   Yair
   Spinned-off flstatus.h

      Rev 1.15   06 Oct 1997 19:39:44   danig
   Changed LEushort\long & Unaligned\4 to arrays

      Rev 1.14   10 Sep 1997 16:11:40   danig
   Got rid of generic names

      Rev 1.13   31 Aug 1997 14:27:32   danig
   flTooManyComponents

      Rev 1.12   28 Aug 1997 16:46:00   danig
   Moved SectorNo definition from fltl.h

      Rev 1.11   21 Aug 1997 14:06:42   unknown
   Unaligned4

      Rev 1.10   31 Jul 1997 14:28:22   danig
   UNALIGNED -> Unaligned

      Rev 1.9   28 Jul 1997 15:15:58   danig
   Standard typedefs

      Rev 1.8   24 Jul 1997 18:05:40   amirban
   New include file org

      Rev 1.7   07 Jul 1997 15:23:26   amirban
   Ver 2.0

      Rev 1.5   18 May 1997 17:34:30   amirban
   Defined dataError

      Rev 1.4   03 Oct 1996 11:56:16   amirban
   New Big-Endian

      Rev 1.3   18 Aug 1996 13:47:34   amirban
   Comments

      Rev 1.2   12 Aug 1996 15:46:58   amirban
   Defined incomplete and timedOut codes

      Rev 1.1   16 Jun 1996 14:03:00   amirban
   Added iFLite compatibility mode

      Rev 1.0   20 Mar 1996 13:33:20   amirban
   Initial revision.
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-99              */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/


#ifndef FLBASE_H
#define FLBASE_H

#include "flsystem.h"
#include "flcustom.h"
#include "flstatus.h"


/* standard type definitions */
typedef int 		FLBoolean;

/* Boolean constants */

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define	TRUE	1
#endif

#ifndef ON
#define	ON	1
#endif
#ifndef OFF
#define	OFF	0
#endif

#define SECTOR_SIZE		(1 << SECTOR_SIZE_BITS)

/* define SectorNo range according to media maximum size */
#if (MAX_VOLUME_MBYTES * 0x100000l) / SECTOR_SIZE > 0x10000l
typedef unsigned long SectorNo;
#define	UNASSIGNED_SECTOR 0xffffffffl
#else
typedef unsigned short SectorNo;
#define UNASSIGNED_SECTOR 0xffff
#endif

#if FAR_LEVEL > 0
  #define FAR0	far
#else
  #define FAR0
#endif

#if FAR_LEVEL > 1
  #define FAR1	far
#else
  #define FAR1
#endif

#if FAR_LEVEL > 2
  #define FAR2	far
#else
  #define FAR2
#endif


#define vol (*pVol)


#if !BIG_ENDIAN

typedef unsigned short LEushort;
typedef unsigned long LEulong;

#define LE2(arg)	arg
#define	toLE2(to,arg)	(to) = (arg)
#define LE4(arg)	arg
#define	toLE4(to,arg)	(to) = (arg)
#define COPY2(to,arg)	(to) = (arg)
#define COPY4(to,arg)	(to) = (arg)

typedef unsigned char Unaligned[2];
typedef Unaligned Unaligned4[2];

#define UNAL2(arg)	fromUNAL(arg)
#define toUNAL2(to,arg)	toUNAL(to,arg)

#define UNAL4(arg)	fromUNALLONG(arg)
#define toUNAL4(to,arg)	toUNALLONG(to,arg)

extern void toUNAL(unsigned char FAR0 *unal, unsigned short n);

extern unsigned short fromUNAL(unsigned char const FAR0 *unal);

extern void toUNALLONG(Unaligned FAR0 *unal, unsigned long n);

extern unsigned long fromUNALLONG(Unaligned const FAR0 *unal);

#else

typedef unsigned char LEushort[2];
typedef unsigned char LEulong[4];

#define LE2(arg)	fromLEushort(arg)
#define	toLE2(to,arg)	toLEushort(to,arg)
#define LE4(arg)	fromLEulong(arg)
#define	toLE4(to,arg)	toLEulong(to,arg)
#define COPY2(to,arg)	copyShort(to,arg)
#define COPY4(to,arg)	copyLong(to,arg)

#define	Unaligned	LEushort
#define	Unaligned4	LEulong

extern void toLEushort(unsigned char FAR0 *le, unsigned short n);

extern unsigned short fromLEushort(unsigned char const FAR0 *le);

extern void toLEulong(unsigned char FAR0 *le, unsigned long n);

extern unsigned long fromLEulong(unsigned char const FAR0 *le);

extern void copyShort(unsigned char FAR0 *to,
		      unsigned char const FAR0 *from);

extern void copyLong(unsigned char FAR0 *to,
		     unsigned char const FAR0 *from);

#define	UNAL2		LE2
#define	toUNAL2		toLE2

#define	UNAL4		LE4
#define	toUNAL4		toLE4

#endif /* BIG_ENDIAN */

/* Call a procedure returning status and fail if it fails. This works only in */
/* routines that return Status: */
#define checkStatus(exp)      {	FLStatus status = (exp); \
				if (status != flOK)	 \
				  return status; }

#include "flsysfun.h"

#ifdef ENVIRONMENT_VARS

typedef void FAR0 * NAMING_CONVENTION (*cpyBuffer)(void FAR0 * ,const void FAR0 * ,size_t);
typedef void FAR0 * NAMING_CONVENTION (*setBuffer)(void FAR0 * ,int ,size_t);
typedef int  NAMING_CONVENTION (*cmpBuffer)(const void FAR0 * ,const void FAR0 * ,size_t);
extern cpyBuffer tffscpy;
extern cmpBuffer tffscmp;
extern setBuffer tffsset;
extern void FAR0* NAMING_CONVENTION flmemcpy(void FAR0* dest,const void FAR0 *src,size_t count);
extern void FAR0*  NAMING_CONVENTION flmemset(void FAR0* dest,int c,size_t count);
extern int  NAMING_CONVENTION flmemcmp(const void FAR0* dest,const void FAR0 *src,size_t count);

extern unsigned char flUse8Bit;
extern unsigned char flUseNFTLCache;
extern unsigned char flUseisRAM;
extern void flSetEnvVar(void);

#endif /* ENVIRONMENT_VARS */

extern void flsleep(unsigned long msec);

#endif