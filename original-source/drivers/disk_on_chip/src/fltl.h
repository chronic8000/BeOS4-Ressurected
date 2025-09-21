
/*
 * $Log:   V:/Flite/archives/FLite/src/FLTL.H_V  $
   
      Rev 1.18   Jan 13 2000 18:27:34   vadimk
   TrueFFS OSAK 4.1

      Rev 1.17   31 Jan 1999 20:08:16   marina
   read/write multiple sectors functions

      Rev 1.16   13 Jan 1999 18:30:56   marina
   In function deleteSector parametr noOfSectors is of type SectorNo

      Rev 1.15   30 Dec 1998 10:58:32   amirban
   always define sectorsInVolume

      Rev 1.14   26 Oct 1998 18:09:34   marina
   function noFormat is added and the field formatRoutine in the
      structure tlEntry is defined in spite of FORMAT_VOLUME definition

      Rev 1.13   16 Aug 1998 20:29:10   amirban
   TL definition changes for ATA & ZIP

      Rev 1.12   01 Mar 1998 12:59:14   amirban
   Add parameter to mapSector

      Rev 1.11   10 Sep 1997 16:31:40   danig
   Got rid of generic names

      Rev 1.10   28 Aug 1997 16:46:50   danig
   Moved SectorNo definition to flbase.h

      Rev 1.9   28 Jul 1997 14:49:38   danig
   volForCallback

      Rev 1.8   24 Jul 1997 17:58:16   amirban
   FAR to FAR0

      Rev 1.7   07 Jul 1997 15:23:52   amirban
   Ver 2.0

      Rev 1.6   03 Jun 1997 17:08:02   amirban
   setBusy change

      Rev 1.5   21 Oct 1996 18:02:34   amirban
   Defragment i/f change

      Rev 1.4   10 Sep 1996 17:32:26   amirban
   Unsigned int --> unsigned short

      Rev 1.3   18 Aug 1996 13:47:32   amirban
   Comments

      Rev 1.2   12 Aug 1996 15:48:12   amirban
   Defined setBusy

      Rev 1.1   14 Jul 1996 16:48:52   amirban
   Format params

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

#ifndef FLTL_H
#define FLTL_H

#include "flflash.h"

typedef struct {
  SectorNo sectorsInVolume;
  unsigned long bootAreaSize;
  unsigned long eraseCycles;
} TLInfo;

/* See interface documentation of functions in ftllite.c    */

typedef struct tTL TL;		/* Forward definition */
typedef struct tTLrec TLrec; 	/* Defined by translation layer */

struct tTL {
  TLrec		*rec;

  const void FAR0 *(*mapSector)(TLrec *, SectorNo sectorNo, CardAddress *physAddr);
  FLStatus	(*writeSector)(TLrec *, SectorNo sectorNo, void FAR1 *fromAddress);

  FLStatus	(*writeMultiSector)(TLrec *, SectorNo sectorNo, void FAR1 *fromAddress,SectorNo sectorCount);
  FLStatus	(*readSectors)(TLrec *, SectorNo sectorNo, unsigned char FAR1 *dest,SectorNo sectorCount);

  FLStatus	(*deleteSector)(TLrec *, SectorNo sectorNo, SectorNo noOfSectors);
  FLStatus	(*tlSetBusy)(TLrec *, FLBoolean);
  void		(*dismount)(TLrec *);

  #if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
  FLStatus	(*defragment)(TLrec *, long FAR2 *bytesNeeded);
  #endif

  SectorNo 	(*sectorsInVolume)(TLrec *);
  FLStatus  (*getTLInfo)(TLrec *, TLInfo *tlInfo);
  void		(*recommendedClusterInfo)(TLrec *, int *sectorsPerCluster, SectorNo *clusterAlignment);
};


#include "dosformt.h"

/* Translation layer registration information */

extern int noOfTLs;	/* No. of translation layers actually registered */

typedef struct {
  FLStatus (*mountRoutine) (unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback);
  FLStatus (*formatRoutine) (unsigned volNo, FormatParams FAR1 *formatParams, FLFlash *flash);
} TLentry;

extern TLentry tlTable[TLS];
extern FLStatus noFormat (unsigned volNo, FormatParams FAR1 *formatParams, FLFlash *flash);

extern FLStatus	flMount(unsigned volNo, TL *, FLBoolean useFilters);

#ifdef FORMAT_VOLUME
extern FLStatus	flFormat(unsigned, FormatParams FAR1 *formatParams);
#endif

#endif
