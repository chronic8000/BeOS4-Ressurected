/*
 * $Log:   V:/Flite/archives/FLite/src/BDDEFS.H_V  $
   
      Rev 1.5   Mar 12 2000 17:49:00   veredg
   Add file definitions for blockdev.c
   
      Rev 1.4   Feb 24 2000 11:29:36   vadimk
   BDK_COMPLETE_IMAGE_UPDATE value is changed from 0 to 16

      Rev 1.3   Jan 13 2000 17:59:12   vadimk
   TrueFFS-OSAK 4.1

      Rev 1.2   04 Apr 1999 17:38:00   marina
   initFS does not get parameters any more

      Rev 1.1   07 Jan 1999 12:58:48   marina
   include backgrnd.h was removed

      Rev 1.0   22 Dec 1998 13:41:14   marina
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

#ifndef BDDEFS_H
#define BDDEFS_H

#include "fltl.h"
#include "flsocket.h"
#include "flbuffer.h"
#include "stdcomp.h"

#ifdef BDK_ACCESS

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
#define SIGNATURE_LEN  8
#define SIGNATURE_NAME 4
#define SIGNATURE_NUM  4
#define ANAND_LEN      5
#define SYNDROM_BYTES  6
#define BDK_COMPLETE_IMAGE_UPDATE  16
#define BDK_MIN(a,b)   ((a) < (b) ? (a) : (b))



typedef struct {
   CardAddress bdkDocWindow;      /* DiskOnChip Window Address  */
   word  bdkGlobalStatus;         /* BDK global status variable */
   word  bdkEDC;                  /* ECC mode flag */
   word  bdkSignOffset;           /* BDK image sign offset (can be 0 or 8) */
   byte  signBuffer[SIGNATURE_LEN];
   byte  syndromEDC[SYNDROM_BYTES];
   byte  noOfChips, firstChipId;
   word  pageSize, erasableBlockSize, flags, pagesPerBlock, erasableBlockBits;
   word  startImageBlock, endImageBlock, curReadImageBlock;
   dword chipSize, noOfBlocks, floorSize, bootImageSize, realBootImageSize;
   dword curReadImageAddress, actualReadLen;
   word  curUpdateImageBlock;
   dword curUpdateImageAddress, actualUpdateLen;
   byte  updateImageFlag;

} BDKVol;
#endif

typedef struct {
  char		flags;			/* See description above */

  unsigned	sectorsPerCluster;	/* Cluster size in sectors */
  unsigned	maxCluster;		/* highest cluster no. */
  unsigned	bytesPerCluster;	/* Bytes per cluster */
  unsigned	bootSectorNo;		/* Sector no. of DOS boot sector */
  unsigned	firstFATSectorNo;	/* Sector no. of 1st FAT */
  unsigned	secondFATSectorNo;	/* Sector no. of 2nd FAT */
  unsigned	numberOfFATS;		/* number of FAT copies */
  unsigned	sectorsPerFAT;		/* Sectors per FAT copy */
  unsigned	rootDirectorySectorNo;	/* Sector no. of root directory */
  unsigned	sectorsInRootDirectory;	/* No. of sectors in root directory */
  unsigned	firstDataSectorNo;	/* 1st cluster sector no. */
  unsigned	allocationRover;	/* rover pointer for allocation */

#ifndef SINGLE_BUFFER
  #if FILES > 0
  FLBuffer	volBuffer;		/* Define a sector buffer */
  #endif
  FLMutex	volExecInProgress;
#endif

#ifdef LOW_LEVEL
  FLFlash 	flash;			/* flash structure for low level operations */
#endif

  TL		tl;			/* Translation layer methods */
  FLSocket	*socket;		/* Pointer to socket */
#ifdef WRITE_PROTECTION
  unsigned long password[2];
#endif
#ifdef BDK_ACCESS
  BDKVol bdk;
#endif
} Volume;

#if defined(FILES) && FILES > 0
typedef struct {
  long		currentPosition;	/* current byte offset in file */
#define		ownerDirCluster currentPosition	/* 1st cluster of owner directory */
  long 		fileSize;		/* file size in bytes */
  SectorNo	directorySector;	/* sector of directory containing file */
  unsigned      currentCluster;		/* cluster of current position */
  unsigned char	directoryIndex;		/* entry no. in directory sector */
  unsigned char	flags;			/* See description below */
  Volume *	fileVol;		/* Drive of file */
} File;

/* File flag definitions */
#define	FILE_MODIFIED		4	/* File was modified */
#define FILE_IS_OPEN		8	/* File entry is used */
#define	FILE_IS_DIRECTORY    0x10	/* File is a directory */
#define	FILE_IS_ROOT_DIR     0x20	/* File is root directory */
#define	FILE_READ_ONLY	     0x40	/* Writes not allowed */
#define	FILE_MUST_OPEN       0x80	/* Create file if not found */
#endif /* FILES > 0 */

#ifdef SINGLE_BUFFER

extern FLBuffer buffer;
extern FLMutex execInProgress;

#else

#define buffer (vol.volBuffer)
#define execInProgress (vol.volExecInProgress)

#endif

extern FLStatus dismountVolume(Volume vol);
extern void dismountLowLevel(Volume vol);
extern FLBoolean initDone;	/* Initialization already done */
extern Volume 	vols[DRIVES];
extern FLStatus setBusy(Volume vol, FLBoolean state);
const void FAR0 *findSector(Volume vol, SectorNo sectorNo);
FLStatus dismountFS(Volume vol,FLStatus status);
#if FILES>0
void initFS(void);
#endif
#endif