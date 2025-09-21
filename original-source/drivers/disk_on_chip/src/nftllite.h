/*
 * $Log:   V:/Flite/archives/FLite/src/NFTLLITE.H_V  $
   
      Rev 1.11   02 Mar 2000 15:48:08   dimitrys
   Move scacheTable to NFTLLITE.C file
   
      Rev 1.10   23 Feb 2000 17:49:04   dimitrys
   Define MAX_UNIT_NUM == 12 Kbytes
   
      Rev 1.9   27 Jan 2000 17:01:44   dimitrys
   Define IS_BAD(b) for DFORMAT.C utility
   
      Rev 1.8   Jan 19 2000 15:15:22   vadimk
   add ANAND_SPARE_SIZE definition

      Rev 1.7   Jan 13 2000 18:31:56   vadimk
   TrueFFS OSAK 4.1

      Rev 1.6   Aug 03 1999 12:02:14   marinak
   Wear Leveling data structure is added

      Rev 1.5   Jul 12 1999 18:06:00   marinak
   Unite U_CACHE and S_CACHE definitions

      Rev 1.4   16 Mar 1999 14:35:52   marina
   Compilation problem fix

      Rev 1.3   01 Mar 1999 16:08:14   Dmitry
   Bad Block processing in new format fixed (UNIT_BAD_MOUNT)

      Rev 1.2   25 Feb 1999 12:04:20   Dmitry
   Add EXTRA_LARGE define for special unit size calculation

      Rev 1.1   24 Feb 1999 14:27:12   marina
   put TLrec back

      Rev 1.0   23 Feb 1999 21:10:06   marina
   Initial revision.
 *
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


#ifndef NFTLLITE_H
#define NFTLLITE_H

#include "flbuffer.h"
#include "flflash.h"
#include "fltl.h"

typedef long int ANANDVirtualAddress;
typedef unsigned char ANANDPhysUnit;
typedef unsigned short ANANDUnitNo;

#define ANAND_UNASSIGNED_ADDRESS 0xffffffffl
#define ANAND_SPARE_SIZE         16

#define UNIT_DATA_OFFSET   8
#define SECTOR_DATA_OFFSET 6
#define UNIT_TAILER_OFFSET (SECTOR_SIZE + 8)
#define FOLD_MARK_OFFSET   (2 * SECTOR_SIZE + 8)

#define ERASE_MARK      0x3c69

#define MAX_UNIT_CHAIN  20

#define ANAND_UNIT_FREE	0xff
#define UNIT_REPLACED	0x80
#define UNIT_COUNT	0x7f
#define UNIT_ORPHAN     0x10

#define	UNIT_UNAVAIL	0x6a	/* Illegal count denoting unit not available */
#define UNIT_BAD_MOUNT  0x6b    /* Bad unit sign after mount */

#define	UNIT_MAX_COUNT	0x40	/* Largest valid count */

#define IS_BAD(u)       ( u == UNIT_BAD_MOUNT )

#define	UNIT_BAD_ORIGINAL 0
/*#define UNIT_BAD_MARKED   7*/

#define countOf(unitNo)     (vol.physicalUnits[unitNo] & UNIT_COUNT)
#define isAvailable(unitNo) (vol.physicalUnits[unitNo] == ANAND_UNIT_FREE || countOf(unitNo) <= UNIT_MAX_COUNT)
#define setUnavail(unitNo)  {vol.physicalUnits[unitNo] &= ~UNIT_COUNT; vol.physicalUnits[unitNo] |= UNIT_UNAVAIL; }
#define isReplaced(unitNo)  (vol.physicalUnits[unitNo] & UNIT_REPLACED)

#define MAX_UNIT_SIZE_BITS      15
#define MORE_UNIT_BITS_MASK     3

#define	ANAND_NO_UNIT           0xffff
#define ANAND_REPLACING_UNIT    0x8000

#define MAX_UNIT_NUM            (12 * 1024)


/* Block flags */

#define SECTOR_FREE             0xff
#define	SECTOR_USED             0x55
#define SECTOR_IGNORE           0x11
#define	SECTOR_DELETED          0x00


#ifdef NFTL_CACHE
/* values for 2-bit entries in Sector Flags cache */
#define	S_CACHE_SECTOR_DELETED	0x00
#define S_CACHE_SECTOR_IGNORE	0x01
#define	S_CACHE_SECTOR_USED	0x02
#define S_CACHE_SECTOR_FREE	0x03
#endif /* NFTL_CACHE */


#define FOLDING_IN_PROGRESS     0x5555
#define FOLDING_COMPLETE        0x1111

#define ERASE_NOT_IN_PROGRESS   -1

#ifdef NFTL_CACHE
/* Unit Header cache entry, close relative of struct UnitHeader */
typedef struct {
  unsigned short virtualUnitNo;
  unsigned short replacementUnitNo;
} ucacheEntry;

#endif /* NFTL_CACHE */

/* erase record */
typedef struct {
  LEulong  eraseCount;
  LEushort eraseMark;
  LEushort eraseMark1;
} UnitTailer;

/* unit header  */
typedef struct {
  LEushort virtualUnitNo;
  LEushort replacementUnitNo;
  LEushort spareVirtualUnitNo;
  LEushort spareReplacementUnitNo;
} ANANDUnitHeader;

/* Medium Boot Record */

typedef struct {
  char      bootRecordId[6];          /* = "ANAND" */
  LEushort  noOfUnits;
  LEushort  bootUnits;
  Unaligned4 virtualMediumSize;
#ifdef EXTRA_LARGE
  unsigned char anandFlags;
#endif /* EXTRA_LARGE */
} ANANDBootRecord;

#ifndef MALLOC

#define ANAND_HEAP_SIZE	(0x100000l / ASSUMED_NFTL_UNIT_SIZE) *       \
			(sizeof(ANANDUnitNo) + sizeof(ANANDPhysUnit)) *  	\
			MAX_VOLUME_MBYTES

#ifdef NFTL_CACHE
#define U_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / ASSUMED_NFTL_UNIT_SIZE)
#define S_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / (SECTOR_SIZE * 4))
#endif

#endif /* MALLOC */

#define WLnow           0xff

typedef struct {
  unsigned short alarm;
  ANANDUnitNo currUnit;
} WLdata;

struct tTLrec{
  FLBoolean	    badFormat;		/* true if TFFS format is bad*/

  ANANDUnitNo	    orgUnit,           	/* Unit no. of boot record */
		    spareOrgUnit;		/* ... and spare copy of it*/
  ANANDUnitNo	    freeUnits;		/* Free units on media*/
  unsigned int	    erasableBlockSizeBits;	/* log2 of erasable block size*/
  ANANDUnitNo	    noOfVirtualUnits;
  ANANDUnitNo	    noOfTransferUnits;
  unsigned long     unitOffsetMask; 	/* = 1 << unitSizeBits - 1 */
  unsigned int	    sectorsPerUnit;

  ANANDUnitNo	    noOfUnits,
		    bootUnits;
  unsigned int	    unitSizeBits;
  SectorNo	    virtualSectors;

  ANANDUnitNo       roverUnit,          /* Starting point for allocation search */
		    countsValid;	/* Number of units for which unit
					   count was set */
  ANANDPhysUnit	    *physicalUnits; 	/* unit table by physical no. */
  ANANDUnitNo	    *virtualUnits; 	/* unit table by logical no. */

#ifdef NFTL_CACHE
  ucacheEntry       *ucache;            /* Unit Header cache */
  unsigned char     *scache;            /* Sector Flags cache */
#endif

  SectorNo 	    mappedSectorNo;
  const void FAR0   *mappedSector;
  CardAddress	    mappedSectorAddress;

  /* Accumulated statistics. */
  long int	    sectorsRead,
		    sectorsWritten,
		    sectorsDeleted,
		    parasiteWrites,
		    unitsFolded;

  FLFlash	    flash;
  FLBuffer          *buffer;

#ifndef MALLOC
  char		    heap[ANAND_HEAP_SIZE];
#ifdef NFTL_CACHE
  ucacheEntry       ucacheBuf[U_CACHE_SIZE];
  unsigned char     scacheBuf[S_CACHE_SIZE];
#endif
#endif /* MALLOC */

  WLdata wearLevel;
  unsigned long eraseSum;
};

typedef TLrec Anand;

#define nftlBuffer  vol.buffer->data

#endif
