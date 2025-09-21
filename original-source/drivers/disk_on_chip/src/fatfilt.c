/*
 * $Log:   V:/Flite/archives/Flite/src/fatfilt.c_V  $
 * 
 *    Rev 1.7   May 14 2000 17:47:08   vadimk
 * Fix check for abs_mounted volume
 * 
 *    Rev 1.6   Apr 06 2000 19:38:54   vadimk
 * Remove warnings and unnecessary array allocation
 * 
 *    Rev 1.5   Apr 05 2000 16:12:42   vadimk
 * DOS mounted volume support was added
 * Heap was removed
 *
 *    Rev 1.4   Feb 03 2000 17:03:38   danig
 * Everything under: #ifdef ABS_READ_WRITE
 *
 *    Rev 1.3   Feb 03 2000 14:27:58   vadimk
 * Enclose the whole file under ifndef FL_READ_ONLY
 *
 *    Rev 1.2   Jan 24 2000 11:06:00   vadimk
 * in ffcall function  .
 * physDrv variable was eliminated.
 * logVol became pointer
 *
 *    Rev 1.1   Jan 19 2000 15:00:30   vadimk
 * Debug_print bug fixed
 *
 *    Rev 1.0   Jul 07 1999 21:11:16   marinak
 * Initial revision
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			                        */
/*		Copyright (C) M-Systems Ltd. 1995-1996			                      */
/*									                                                    */
/************************************************************************/

#include "blockdev.h"
#include "dosformt.h"
#include "fatfilt.h"
#include "flflash.h"
#include <stdio.h>
#include "bddefs.h"

#ifdef ABS_READ_WRITE
#ifndef FL_READ_ONLY
static FLBoolean ffInitDone = FALSE;

typedef struct {
  unsigned  driveNo;
  unsigned  logVolID;
  unsigned  flags;
  unsigned  sectorsPerCluster;  /* Cluster size in sectors */
  unsigned  maxCluster;         /* highest cluster no. */
  SectorNo  bootSectorNo;       /* Sector no. of DOS boot sector */
  SectorNo  firstDataSectorNo;
  SectorNo  firstFATSectorNo;   /* Sector no. of 1st FAT */
  unsigned  numberOfFATS;       /* number of FAT copies */
  unsigned  sectorsPerFAT;      /* Sectors per FAT copy */
  unsigned  sectorsInPartition;
} LogicalVolume;

#define MAX_LOG_VOL_NUM MAX_PARTITION_DEPTH*4
typedef struct {
  unsigned partitionsNum;
  unsigned long sizeInSectors;
  LogicalVolume partition[MAX_PARTITION_DEPTH*4];
  unsigned allocationRover;
} PhysicalDrive;

static PhysicalDrive physicalDrives[DRIVES];
extern Volume  vols[DRIVES];
/*----------------------------------------------------------------------*/
/*               r e a d P a r t i t i o n D a t a                      */
/*                                                                      */
/* Reads data of the partition: FAT placement,size, root directory size */
/*                                                                      */
/* Parameters:                                                          */
/*	logVol		: Pointer identifying logical drive             */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus readPartitionData(LogicalVolume* logVol)
{
  DOSBootSector bootSector;
  SectorNo rootDirectorySectorNo;
  unsigned sectorsInRootDirectory;
  IOreq ioreq;
  SectorNo noOfSectors;

  logVol->sectorsPerCluster = 0;
  logVol->numberOfFATS = 0;
  logVol->sectorsPerFAT = 0;
  logVol->firstFATSectorNo = 0;


  ioreq.irHandle = logVol->driveNo;
  ioreq.irSectorNo = logVol->bootSectorNo-vols[ioreq.irHandle].bootSectorNo; /* Vadim : shift on hidden sectors */
  ioreq.irSectorCount = 1;
  ioreq.irData = &bootSector;
  checkStatus(flAbsRead(&ioreq));
  /* Do the customary sanity checks */
  if (!(bootSector.bpb.jumpInstruction[0] == 0xe9 ||
	(bootSector.bpb.jumpInstruction[0] == 0xeb &&
	 bootSector.bpb.jumpInstruction[2] == 0x90))) {
    DEBUG_PRINT(("Debug: did not recognize format.\n\r"));
    return flNonFATformat;
  }

  /* See if we handle this sector size */
  if (UNAL2(bootSector.bpb.bytesPerSector) != SECTOR_SIZE) {
    DEBUG_PRINT(("flFormatNotSupported\n\r"));
    return flFormatNotSupported;
  }

  logVol->sectorsPerCluster = bootSector.bpb.sectorsPerCluster;

  logVol->numberOfFATS = bootSector.bpb.noOfFATS;
  logVol->sectorsPerFAT = LE2(bootSector.bpb.sectorsPerFAT);
  logVol->firstFATSectorNo = logVol->bootSectorNo +
			    LE2(bootSector.bpb.reservedSectors);

  rootDirectorySectorNo = logVol->firstFATSectorNo +
		   bootSector.bpb.noOfFATS * LE2(bootSector.bpb.sectorsPerFAT);
  sectorsInRootDirectory =
	(UNAL2(bootSector.bpb.rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE - 1) /
		SECTOR_SIZE + 1;

  logVol->firstDataSectorNo = rootDirectorySectorNo +
			     sectorsInRootDirectory;

  noOfSectors = UNAL2(bootSector.bpb.totalSectorsInVolumeDOS3);
  if (noOfSectors == 0)
    noOfSectors = (SectorNo) LE4(bootSector.bpb.totalSectorsInVolume);


  logVol->maxCluster = (unsigned) ((noOfSectors + logVol->bootSectorNo - logVol->firstDataSectorNo) /
				logVol->sectorsPerCluster) + 1;

  rootDirectorySectorNo = logVol->firstFATSectorNo +
		   bootSector.bpb.noOfFATS * LE2(bootSector.bpb.sectorsPerFAT);
  sectorsInRootDirectory =
	(UNAL2(bootSector.bpb.rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE - 1) /
		SECTOR_SIZE + 1;
  logVol->firstDataSectorNo = rootDirectorySectorNo +
			     sectorsInRootDirectory;

  if (logVol->maxCluster < 4085) {
#ifdef FAT_12BIT
    logVol->flags |= VOLUME_12BIT_FAT;	/* 12-bit FAT */
#else
    DEBUG_PRINT(("Debug: FAT_12BIT must be defined.\n\r"));
    return flFormatNotSupported;
#endif
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*               F A T f i l t I n i t                                  */
/*                                                                      */
/* Initializes the FAT filter data structures, checks partitions        */
/*                                                                      */
/* Parameters:                                                          */
/*	driveNo		: Number of the DiskOnChip in the system        */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus FATfiltInit(unsigned driveNo)
{
  PartitionTable  partitionTable;
  Partition ptEntry;
  SectorNo ptSector;
  unsigned ptCount,extended_depth;
  FLBoolean extendedPtFound = TRUE;
  LogicalVolume* logVol;
  PhysicalDrive* physDrv;
  unsigned lastLogVolID = 0;
  IOreq ioreq;
  

  physDrv = &physicalDrives[driveNo];
  physDrv->allocationRover = 0;

  for(extended_depth = 0,ptSector = 0;
      (extended_depth<MAX_PARTITION_DEPTH) &&
        (extendedPtFound==TRUE) && (lastLogVolID < MAX_LOG_VOL_NUM);
      extended_depth++) {

    extendedPtFound=FALSE;

    /* Read in paritition table */
    ioreq.irHandle = driveNo;
    ioreq.irSectorNo = ptSector-vols[ioreq.irHandle].bootSectorNo; /* Vadim: shift on hidden sectors  */
    ioreq.irSectorCount = 1;
    ioreq.irData = &partitionTable;
    
    checkStatus(flAbsRead(&ioreq));
    

    if (LE2(partitionTable.signature) != PARTITION_SIGNATURE)
      break;

    for(ptCount=0;
        (ptCount<4) && (extendedPtFound==FALSE);
        ptCount++) {

      ptEntry = partitionTable.ptEntry[ptCount];

      switch (ptEntry.type) {
        case FAT12_PARTIT:
        case FAT16_PARTIT:
        case DOS4_PARTIT:
          logVol = (&(physDrv->partition[lastLogVolID]));
          logVol->logVolID = lastLogVolID;
          logVol->driveNo = driveNo;
          physDrv->partitionsNum++;
          lastLogVolID++;
          logVol->bootSectorNo =
              (unsigned) UNAL4(ptEntry.startingSectorOfPartition)+ptSector;

          break;
        case EX_PARTIT:
          extendedPtFound = TRUE;
          ptSector = (SectorNo)UNAL4(ptEntry.startingSectorOfPartition);
          break;
        default:
          break;
      }
    }
  }


  for(ptCount = 0;ptCount<physDrv->partitionsNum;ptCount++)
    readPartitionData(&(physDrv->partition[ptCount]));


  return flOK;
}


/*----------------------------------------------------------------------*/
/*               f r e e D e l e t e d C l u s t e r s                  */
/*                                                                      */
/* When a FAT block is about to be written by an absolute write, this   */
/* routine will first scan whether any sectors are being logically	*/
/* deleted by this FAT update, and if so, it will delete-sector them	*/
/* before the actual FAT update takes place.				*/
/*									*/
/* Parameters:                                                          */
/*	logVol		: Pointer identifying drive			*/
/*	sectorNo	: FAT Sector no. about to be written		*/
/*      newFATsector	: Address of FAT sector about to be written	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus freeDeletedClusters(LogicalVolume* logVol,
                                    SectorNo sectorNo,
                                    char FAR1 *newFATsector)
{
  IOreq ioreq;
  unsigned char oldFATsector[SECTOR_SIZE];
  SectorNo firstSector;
  unsigned firstCluster;
  PhysicalDrive* physDrv = &physicalDrives[logVol->driveNo];
  FLStatus status;

#ifdef FAT_12BIT
  unsigned FAThalfBytes;
  unsigned int halfByteOffset;
#else
  unsigned int byteOffset;
#endif

    ioreq.irHandle = logVol->driveNo;
    ioreq.irSectorNo = sectorNo-vols[ioreq.irHandle].bootSectorNo;  /* Vadim shift on hidden sectors */ 
    ioreq.irSectorCount = 1;
    ioreq.irData = oldFATsector;
    status = flAbsRead(&ioreq);
    if(status!=flOK) {
      DEBUG_PRINT(("Read previous FAT data failed\n\r"));
      return status;
    }

#ifdef FAT_12BIT
  FAThalfBytes = logVol->flags & VOLUME_12BIT_FAT ? 3 : 4;

  firstCluster =
	(FAThalfBytes == 3) ?
	(((unsigned) (sectorNo - logVol->firstFATSectorNo) * (2 * SECTOR_SIZE) + 2) / 3) :
	((unsigned) (sectorNo - logVol->firstFATSectorNo) * (SECTOR_SIZE / 2));
  firstSector =
	((SectorNo) firstCluster - 2) * logVol->sectorsPerCluster + logVol->firstDataSectorNo;
  halfByteOffset =
	(firstCluster * FAThalfBytes) & (2 * SECTOR_SIZE - 1);

  /* Find if any clusters were logically deleted, and if so, delete them */
  /* NOTE: We are skipping over 12-bit FAT entries which span more than  */
  /*       one sector. Nobody's perfect anyway.                          */
  for (; halfByteOffset < (2 * SECTOR_SIZE - 2);
       firstSector += logVol->sectorsPerCluster, halfByteOffset += FAThalfBytes) {
    unsigned short oldFATentry, newFATentry;

#if BIG_ENDIAN
    oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + (halfByteOffset / 2)));
    newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + (halfByteOffset / 2)));
#else
    oldFATentry = UNAL2(*(Unaligned FAR0 *)(oldFATsector + (halfByteOffset / 2)));
    newFATentry = UNAL2(*(Unaligned FAR1 *)(newFATsector + (halfByteOffset / 2)));
#endif
    if (halfByteOffset & 1) {
      oldFATentry >>= 4;
      newFATentry >>= 4;
    }
    else if (FAThalfBytes == 3) {
      oldFATentry &= 0xfff;
      newFATentry &= 0xfff;
    }
#else
  firstCluster =
	((unsigned) (sectorNo - logVol->firstFATSectorNo) * (SECTOR_SIZE / 2));
  firstSector =
	((SectorNo) firstCluster - 2) * logVol->sectorsPerCluster + logVol->firstDataSectorNo;

  /* Find if any clusters were logically deleted, and if so, delete them */
  for (byteOffset = 0; byteOffset < SECTOR_SIZE;
       firstSector += logVol->sectorsPerCluster, byteOffset += 2) {
    unsigned short oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + byteOffset));
    unsigned short newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + byteOffset));
#endif

    if (oldFATentry != FAT_FREE && newFATentry == FAT_FREE) {
      ioreq.irHandle = logVol->driveNo;
      ioreq.irSectorNo = firstSector-vols[ioreq.irHandle].bootSectorNo;  /* Vadim: shift on hidden sectors */   
      ioreq.irSectorCount = logVol->sectorsPerCluster;
      checkStatus(flAbsDelete(&ioreq));
    }
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*               f f C a l l                                            */
/*                                                                      */
/* Common entry-point to all FAT filter functions. Macros are           */
/* to call individual function, which are separately described below.	*/
/*                                                                      */
/* Parameters:                                                          */
/*	functionNo      : file-system function code (listed below)	*/
/*      ioreq           : IOreq structure                               */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/



FLStatus ffCall(ffFunctionNo functionNo, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer;
  SectorNo currSector,iSector;
  SectorNo sectorCount = ioreq->irSectorCount;
  LogicalVolume *logVol;
  unsigned ptNum;


  if (!ffInitDone) {
    checkStatus(flInit());
   if(!(vols[ioreq->irHandle].flags & VOLUME_ABS_MOUNTED))
      return flNotMounted;/*  Can't work with unmounted volume */
   
    checkStatus(FATfiltInit((unsigned)ioreq->irHandle));
    ffInitDone = TRUE;
  }


  for(ptNum = 0;ptNum<physicalDrives[ioreq->irHandle].partitionsNum;ptNum++) {
  	if((vols[ioreq->irHandle].flags & VOLUME_MOUNTED)&&(ptNum==0))  /*  Vadim : use built in fat filter for the first partition */ 
		continue;
    logVol = &physicalDrives[ioreq->irHandle].partition[ptNum];
    currSector = ioreq->irSectorNo;
    userBuffer = (char FAR1 *) ioreq->irData;
    for (iSector = 0; iSector < sectorCount;
      iSector++, currSector++, userBuffer += SECTOR_SIZE) {
      if ((currSector+vols[ioreq->irHandle].bootSectorNo) < logVol->sectorsPerFAT+logVol->firstFATSectorNo &&
          (currSector+vols[ioreq->irHandle].bootSectorNo) >= logVol->firstFATSectorNo) {
        checkStatus(freeDeletedClusters(logVol,currSector+vols[ioreq->irHandle].bootSectorNo,userBuffer));  /*  ??? */
		
      }
      else if(currSector >= logVol->firstFATSectorNo)
        break;

      #ifdef SCATTER_GATHER
      userBuffer = *((char FAR1 **)(ioreq->irData) +(int)(ioreq->irSectorCount));
      #endif
    }
  }
  return flOK;
}

#endif  /* FL_READ_ONLY */
#endif  /* ABS_READ_WRITE */
