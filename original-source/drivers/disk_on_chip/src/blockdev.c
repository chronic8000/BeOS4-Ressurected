/*
 * $Log:   V:/Flite/archives/FLite/src/BLOCKDEV.C_V  $
 * 
 *    Rev 1.41   May 25 2000 15:24:06   vadimk
 * Use flcpy,set ,cmp defines instead of memcpy,set,cmp under 
 * ENVIRONMENT_VARS in flInit function
 *
 *    Rev 1.40   May 22 2000 11:48:34   vadimk
 * Add SCATTER_GATHER option in changePassword function . add FAR 2 to the second parameter dclaration in socketInfo function
 *
 *    Rev 1.39   Apr 09 2000 12:00:58   vadimk
 * Add tl_format_only option in bdformatvolume function
 *
 *    Rev 1.38   05 Apr 2000 16:13:44   dimitrys
 * Fix C/H/S values in volumeInfo() by calculation in
 *   flBuildGeometry() call
 *
 *    Rev 1.37   Mar 21 2000 11:11:42   vadimk
 * get rid of warnings
 *
 *    Rev 1.36   Mar 14 2000 16:10:32   vadimk
 * stdmemcpy/set/cmp functions were removed
 *
 *    Rev 1.35   Mar 12 2000 19:46:48   veredg
 * Unite flCall with bdCall
 *
 *    Rev 1.34   12 Mar 2000 13:55:36   dimitrys
 * Change #define FL_BACKGROUND,  get rid of
 *   warnings
 *
 *    Rev 1.33   Feb 21 2000 16:02:28   vadimk
 * add unlock feature to the writeprotect function
 *
 *    Rev 1.32   Feb 21 2000 13:47:28   vadimk
 * add  write protection ability to the absmounted volume
 *
 *    Rev 1.31   Feb 20 2000 18:29:06   veredg
 * The status returned by the flash read operation in PhysicalRead was ignored.
 *
 *    Rev 1.30   Feb 10 2000 15:33:00   veredg
 * Fix Big/Little Endian bug in volumeInfo
 *
 *    Rev 1.29   Jan 25 2000 15:37:08   vadimk
 * Remove include of version.h
 *
 *    Rev 1.28   Jan 25 2000 14:18:38   vadimk
 * add UNAL4 treatment to passwordInfo field of partitionTable structure
 *
 *    Rev 1.27   Jan 23 2000 12:03:08   vadimk
 * Put  partitionTable variable under  #ifdef WRITE_PROTECTION in absmountvolume
 *
 *    Rev 1.26   Jan 20 2000 16:33:30   danig
 * In volumeInfo: getBPB is called only if ABS_READ_WRITE is defined.
 * In bdCall: volumeInfo is not under #ifdef LOW_LEVEL
 *
 *    Rev 1.25   Jan 20 2000 11:43:54   vadimk
 * mountlowlevel added to volumeInfo
 * vol added to the writeProtect parameters
 *
 *    Rev 1.24   Jan 17 2000 13:56:54   vadimk
 * Put dosformat into formatVolume
 *
 *    Rev 1.23   Jan 16 2000 17:45:58   vadimk
 * Write_Protect was added in flInit function.
 *
 *    Rev 1.22   Jan 16 2000 14:28:28   vadimk
 * "fl" previx was added to the environment variables
 *
 *    Rev 1.21   Jan 13 2000 18:00:52   vadimk
 * TrueFFS OSAK 4.1
 *
 *    Rev 1.20   Jan 06 2000 14:36:02   vadimk
 * If sector is out of range of virtual sectors In absRead return flSectorNotFound status .
 *
 *    Rev 1.19   Aug 03 1999 11:53:42   marinak
 * SectorsInVolume - shift by boot sector no
 *
 *    Rev 1.18   Jul 26 1999 12:36:48   marinak
 * Memory leak in FAT only format
 *
 *    Rev 1.17   Jul 12 1999 16:53:04   marinak
 * dosFormat call is passed from blockdev.c to fatlite.c
 *
 *    Rev 1.16   Jul 11 1999 18:09:50   marinak
 * In function mountVolume find the first primary DOS partition
 * in spite of its placement in the partition table
 *
 *    Rev 1.15   18 Apr 1999 15:39:56   marina
 * Formula of finding max cluster is updated in function mountVolume
 *
 *    Rev 1.14   14 Apr 1999 13:42:22   marina
 * checkStatus(flRegisterComponents(... in function flInit
 *
 *    Rev 1.13   14 Apr 1999 12:38:34   marina
 * Fix low level mount bug
 *
 *    Rev 1.12   08 Apr 1999 18:06:02   marina
 * change QNX to SCATTER_GATHER and fix mappedSector declaration in flAbsRead
 *
 *    Rev 1.11   08 Apr 1999 10:30:48   marina
 * Fix createMutex only for detected drives in flInit
 *
 *    Rev 1.10   25 Feb 1999 15:40:08   marina
 * fix memory leaks problem in formatVolume
 *
 *    Rev 1.9   23 Feb 1999 20:19:44   marina
 * new user call mountLowLevel; new format flag TL_FORMAT_ONLY
 *
 *    Rev 1.8   10 Feb 1999 11:59:38   marina
 * return flDataError in place of dataErrorObject
 *
 *    Rev 1.7   03 Feb 1999 18:19:22   marina
 * doc2map edc check bug fix
 *
 *    Rev 1.6   31 Jan 1999 16:50:18   marina
 * Scatter-gather fix
 *
 *    Rev 1.5   13 Jan 1999 18:28:12   marina
 * sectorsInVolume call demands only that (vol.flags & VOL_ABS_MOUNTED)==TRUE
 * and works in spite of (vol.flags & VOL_MOUNTED)==FALSE
 *
 *    Rev 1.4   07 Jan 1999 13:23:04   marina
 * Include backgrnd.h was added; get rid of warnings
 *
 *    Rev 1.3   31 Dec 1998 16:35:54   marina
 * Exit call was passed from fatlite.c; mountZip bug fix, scatter - gather write
 *
 *    Rev 1.2   23 Dec 1998 18:31:20   amirban
 * formatVolume and sectorsInVolume changes
 *
 *    Rev 1.1   23 Dec 1998 17:42:22   marina
 * flMsecCounter was carried from fatlite.c
 *
 *    Rev 1.0   22 Dec 1998 12:24:08   marina
 * Initial revision.
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

#include "dosformt.h"
#include "blockdev.h"
#include "bddefs.h"

#if defined(FILES) && FILES > 0
extern File 	fileTable[FILES];	/* the file table */

extern FLStatus flushBuffer(Volume vol);
extern FLStatus openFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus closeFile(File *file);
extern FLStatus joinFile (File *file, IOreq FAR2 *ioreq);
extern FLStatus splitFile (File *file, IOreq FAR2 *ioreq);
extern FLStatus readFile(File *file,IOreq FAR2 *ioreq);
extern FLStatus writeFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus seekFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus findFile(Volume vol, File *file, IOreq FAR2 *ioreq);
extern FLStatus findFirstFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus findNextFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus getDiskInfo(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus deleteFile(Volume vol, IOreq FAR2 *ioreq, FLBoolean isDirectory);
extern FLStatus renameFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus makeDir(Volume vol, IOreq FAR2 *ioreq);

#endif

#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif

unsigned long flMsecCounter = 0;

FLBoolean initDone = FALSE;	/* Initialization already done */
/* Volume flag definitions */

Volume 	vols[DRIVES];

#ifdef ENVIRONMENT_VARS
cpyBuffer tffscpy;
cmpBuffer tffscmp;
setBuffer tffsset;
#endif

/*----------------------------------------------------------------------*/
/*		      d i s m o u n t V o l u m e			*/
/*									*/
/* Dismounts the volume, closing all files.				*/
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus dismountVolume(Volume vol)
{
  if (vol.flags & VOLUME_ABS_MOUNTED) {
    FLStatus status = flOK;
#ifndef FIXED_MEDIA
    status = flMediaCheck(vol.socket);
#endif
    if (status != flOK)
      vol.flags = 0;
#if FILES>0
    status = dismountFS(&vol,status);
#endif
    vol.tl.dismount(vol.tl.rec);
  }

  vol.flags = 0;	/* mark volume unmounted */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	           s e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: ON (1) = operation entry			*/
/*			  OFF(0) = operation exit			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus setBusy(Volume vol, FLBoolean state)
{
  FLStatus status = flOK;

  if (state == ON) {
    if (!flTakeMutex(&execInProgress))
      return flDriveNotAvailable;
    flSocketSetBusy(vol.socket,ON);
    flNeedVcc(vol.socket);
    if (vol.flags & VOLUME_ABS_MOUNTED)
      status = vol.tl.tlSetBusy(vol.tl.rec,ON);
  }
  else {
    if (vol.flags & VOLUME_ABS_MOUNTED)
      status = vol.tl.tlSetBusy(vol.tl.rec,OFF);
    flDontNeedVcc(vol.socket);
    flSocketSetBusy(vol.socket,OFF);
    flFreeMutex(&execInProgress);
  }

  return status;
}



/*----------------------------------------------------------------------*/
/*		          f i n d S e c t o r				*/
/*									*/
/* Locates a sector in the buffer or maps it				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to locate				*/
/*                                                                      */
/*----------------------------------------------------------------------*/


const void FAR0 *findSector(Volume vol, SectorNo sectorNo)
{
  return
#if FILES > 0
  (sectorNo == buffer.sectorNo && &vol == buffer.owner) ?
    buffer.data :
#endif
    vol.tl.mapSector(vol.tl.rec,sectorNo,NULL);
}



/*----------------------------------------------------------------------*/
/*		  a b s M o u n t V o l u m e				*/
/*									*/
/* Mounts the Flash volume and assume that volume has no FAT            */
/*									*/
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.								*/
/* Mounting a volume has the effect of discarding all open files (the   */
/* files cannot be properly closed since the original volume is gone),  */
/* and turning off the media-change indication to allow file processing */
/* calls.								*/
/*									*/
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus absMountVolume(Volume vol)
{
 #ifdef WRITE_PROTECTION
  PartitionTable FAR0 *partitionTable;
 #endif
  checkStatus(dismountVolume(&vol));

  checkStatus(flMount((unsigned)(&vol - vols),&vol.tl,TRUE)); /* Try to mount translation layer */

  vol.bootSectorNo = 0;	/*  assume sector 0 is DOS boot block */
#ifdef WRITE_PROTECTION
 partitionTable = (PartitionTable FAR0 *) findSector(&vol,0);
 if((partitionTable == NULL)||
   (partitionTable==dataErrorToken)||
   (LE2(partitionTable->signature) != PARTITION_SIGNATURE))
    vol.password[0] = vol.password[1] = 0;
 else
 {
    vol.password[0] = vol.password[1] = 0;
    if (UNAL4(partitionTable->passwordInfo[0]) == 0 &&
       (UNAL4(partitionTable->passwordInfo[1]) != 0 ||
       UNAL4(partitionTable->passwordInfo[2]) != 0)) {
       vol.password[0] = UNAL4(partitionTable->passwordInfo[1]);
       vol.password[1] = UNAL4(partitionTable->passwordInfo[2]);
       vol.flags |= VOLUME_WRITE_PROTECTED;
    }
 }
#endif   /* WRITE_PROTECTION */
  vol.firstFATSectorNo = vol.secondFATSectorNo = 0; /* Disable FAT monitoring */
  vol.flags |= VOLUME_ABS_MOUNTED;  /* Enough to do abs operations */

  return flOK;
}


#if FILES >0
/*----------------------------------------------------------------------*/
/*                     m o u n t V o l u m e                            */
/*                                                                      */
/* Mounts the Flash volume.                                             */
/*                                                                      */
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.                                                          */
/* Mounting a volume has the effect of discarding all open files (the   */
/* files cannot be properly closed since the original volume is gone),  */
/* and turning off the media-change indication to allow file processing */
/* calls.                                                               */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus mountVolume(Volume vol)
{
  SectorNo noOfSectors;
  PartitionTable FAR0 *partitionTable;
  Partition ptEntry;
  DOSBootSector FAR0 *bootSector;
  unsigned ptCount,extended_depth,ptSector;
  FLBoolean primaryPtFound = FALSE, extendedPtFound = TRUE;

  checkStatus(absMountVolume(&vol));

  for(extended_depth = 0,ptSector = 0;
      (extended_depth<MAX_PARTITION_DEPTH) &&
          (primaryPtFound==FALSE) &&
          (extendedPtFound==TRUE);
      extended_depth++) {

    extendedPtFound=FALSE;
    /* Read in paritition table */
    partitionTable = (PartitionTable FAR0 *) findSector(&vol,ptSector);
    if(partitionTable == NULL) {
      vol.tl.dismount(vol.tl.rec);
      return flSectorNotFound;
    }
    if(partitionTable==dataErrorToken) {
      vol.tl.dismount(vol.tl.rec);
      return flDataError;
    }

    if (LE2(partitionTable->signature) != PARTITION_SIGNATURE)
      break;
    for(ptCount=0;
        (ptCount<4) && (primaryPtFound==FALSE) && (extendedPtFound==FALSE);
        ptCount++) {

      ptEntry = partitionTable->ptEntry[ptCount];

      switch (ptEntry.type) {
        case FAT12_PARTIT:
        case FAT16_PARTIT:
        case DOS4_PARTIT:
          primaryPtFound = TRUE;
          vol.bootSectorNo =
              (unsigned) UNAL4(ptEntry.startingSectorOfPartition);
          break;
        case EX_PARTIT:
          extendedPtFound = TRUE;
          ptSector = (unsigned)UNAL4(ptEntry.startingSectorOfPartition);
          break;
        default:
          break;
      }
    }
  }

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;

  if(bootSector==dataErrorToken)
    return flDataError;

  /* Do the customary sanity checks */
  if (!(bootSector->bpb.jumpInstruction[0] == 0xe9 ||
	(bootSector->bpb.jumpInstruction[0] == 0xeb &&
	 bootSector->bpb.jumpInstruction[2] == 0x90))) {
    DEBUG_PRINT(("Debug: did not recognize format.\n"));
    return flNonFATformat;
  }

  /* See if we handle this sector size */
  if (UNAL2(bootSector->bpb.bytesPerSector) != SECTOR_SIZE)
    return flFormatNotSupported;

  vol.sectorsPerCluster = bootSector->bpb.sectorsPerCluster;
  vol.numberOfFATS = bootSector->bpb.noOfFATS;
  vol.sectorsPerFAT = LE2(bootSector->bpb.sectorsPerFAT);
  vol.firstFATSectorNo = vol.bootSectorNo +
			    LE2(bootSector->bpb.reservedSectors);
  vol.secondFATSectorNo = vol.firstFATSectorNo +
			     LE2(bootSector->bpb.sectorsPerFAT);
  vol.rootDirectorySectorNo = vol.firstFATSectorNo +
		   bootSector->bpb.noOfFATS * LE2(bootSector->bpb.sectorsPerFAT);
  vol.sectorsInRootDirectory =
	(UNAL2(bootSector->bpb.rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE - 1) /
		SECTOR_SIZE + 1;
  vol.firstDataSectorNo = vol.rootDirectorySectorNo +
			     vol.sectorsInRootDirectory;

  noOfSectors = UNAL2(bootSector->bpb.totalSectorsInVolumeDOS3);
  if (noOfSectors == 0)
    noOfSectors = (SectorNo) LE4(bootSector->bpb.totalSectorsInVolume);


  vol.maxCluster = (unsigned) ((noOfSectors + vol.bootSectorNo - vol.firstDataSectorNo) /
				vol.sectorsPerCluster) + 1;

  if (vol.maxCluster < 4085) {
#ifdef FAT_12BIT
    vol.flags |= VOLUME_12BIT_FAT;	/* 12-bit FAT */
#else
    DEBUG_PRINT(("Debug: FAT_12BIT must be defined.\n"));
    return flFormatNotSupported;
#endif
  }
  vol.bytesPerCluster = vol.sectorsPerCluster * SECTOR_SIZE;
  vol.allocationRover = 2;	/* Set rover at first cluster */
  vol.flags |= VOLUME_MOUNTED;	/* That's it */

  return flOK;
}
#endif /* #if FILES >0 */


#ifndef FL_READ_ONLY
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)

/*----------------------------------------------------------------------*/
/*		      	 d e f r a g m e n t V o l u m e		*/
/*									*/
/* Performs a general defragmentation and recycling of non-writable	*/
/* Flash areas, to achieve optimal write speed.				*/
/*                                                                      */
/* NOTE: The required number of sectors (in irLength) may be changed    */
/* (from another execution thread) while defragmentation is active. In  */
/* particular, the defragmentation may be cut short after it began by	*/
/* modifying the irLength field to 0.					*/
/*									*/
/* Parameters:                                                          */
/*	ioreq->irLength	: Minimum number of sectors to make available   */
/*			  for writes.					*/
/*                                                                      */
/* Returns:                                                             */
/*	ioreq->irLength	: Actual number of sectors available for writes	*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus defragmentVolume(Volume vol, IOreq FAR2 *ioreq)
{
  return vol.tl.defragment(vol.tl.rec,&ioreq->irLength);
}

#endif /* DEFRAGMENT_VOLUME */


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*		      f l F o r m a t V o l u m e			*/
/*									*/
/* Formats a volume, writing a new and empty file-system. All existing  */
/* data is destroyed. Optionally, a low-level FTL formatting is also    */
/* done.								*/
/* Formatting leaves the volume in the dismounted state, so that a	*/
/* flMountVolume call is necessary after it.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irFlags		: FAT_ONLY_FORMAT: Do FAT formatting only	*/
/*			  TL_FORMAT: Translation layer formatting	*/
/*			  TL_FORMAT_IF_NEEDED: Do FTL formatting only	*/
/*				      if current format is invalid	*/
/*			  ZIP_FORMAT: Create compressed drive		*/
/*	irData		: Address of FormatParams structure to use	*/
/*			  (defined in dosformt.h)			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

extern FLStatus flFormatZIP(unsigned volNo, TL *baseTL);

FLStatus bdFormatVolume(Volume vol, IOreq FAR2 *ioreq)
{
  FormatParams FAR1 *formatParams = (FormatParams FAR1 *) ioreq->irData;
  FLBoolean mountOK = FALSE;
  unsigned volNo = (unsigned)(&vol - vols);
  FLStatus status;

  checkStatus(dismountVolume(&vol));
  if ((ioreq->irFlags & TL_FORMAT)||(ioreq->irFlags & TL_FORMAT_ONLY)) {
    checkStatus(flFormat(volNo,formatParams));
  }
  else {
    status = flMount(volNo,&vol.tl,FALSE); /* Try to mount translation layer */
    mountOK = TRUE;
    if ((status == flUnknownMedia || status == flBadFormat) &&
	(ioreq->irFlags & TL_FORMAT_IF_NEEDED)) {
      status = flFormat(volNo,formatParams);
      mountOK = FALSE;
    }
    else {
      vol.bootSectorNo = 0;	/*  assume sector 0 is DOS boot block */
      vol.firstFATSectorNo = vol.secondFATSectorNo = 0; /* Disable FAT monitoring */
      vol.flags |= VOLUME_ABS_MOUNTED;  /* Enough to do abs operations */
    }
    if (status != flOK)
      return status;
  }

#ifdef COMPRESSION
  if (ioreq->irFlags & ZIP_FORMAT) {
    if (!mountOK) {
      checkStatus(flMount(volNo,&vol.tl,FALSE));
    }
    status = flFormatZIP(volNo,&vol.tl);
    vol.tl.dismount(vol.tl.rec);
    if(status!=flOK)
      return status;
    mountOK = FALSE;
  }
#endif
  if (!mountOK)
    checkStatus(absMountVolume(&vol));

#if (FILES > 0)
  if(!(ioreq->irFlags & TL_FORMAT_ONLY))
    checkStatus(flDosFormat(&vol.tl,formatParams));
#endif

  return flOK;
}

#endif /* FORMAT_VOLUME */

#endif  /* FL_READ_ONLY */



/*----------------------------------------------------------------------*/
/*		    s e c t o r s I n V o l u m e                       */
/*									*/
/* Defines actual number of virtual sectors according to the low-level	*/
/* format of the media.							*/
/*									*/
/* Returns:                                                             */
/*	ioreq->irLength	: Actual number of virtual sectors in volume	*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/
static FLStatus sectorsInVolume(Volume vol, IOreq FAR2 *ioreq)
{
  unsigned long sectorsInVol = vol.tl.sectorsInVolume(vol.tl.rec);
  if(sectorsInVol<=vol.bootSectorNo) {
    ioreq->irLength = 0;
    return flGeneralFailure;
  }

  ioreq->irLength = sectorsInVol-vol.bootSectorNo;
  return flOK;
}


#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*		             a b s R e a d 				*/
/*									*/
/* Reads absolute sectors by sector no.					*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to read into		*/
/*	irSectorNo	: First sector no. to read (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to read	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irSectorCount	: Number of sectors actually read		*/
/*----------------------------------------------------------------------*/

static FLStatus absRead(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo currSector = vol.bootSectorNo + ioreq->irSectorNo;
  SectorNo sectorCount = (SectorNo)ioreq->irSectorCount;
  void FAR0 *mappedSector;

  for (ioreq->irSectorCount = 0;
       (SectorNo)(ioreq->irSectorCount) < sectorCount;
       ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE) {

  #ifdef SCATTER_GATHER
    userBuffer = *((char FAR1 **)(ioreq->irData) +(int)(ioreq->irSectorCount));
  #endif

    mappedSector = (void FAR0 *)findSector(&vol,currSector);
    if (mappedSector) {
      if(mappedSector==dataErrorToken)
        return flDataError;

      tffscpy(userBuffer,mappedSector,SECTOR_SIZE);
    }
    else  {
      if(vol.tl.sectorsInVolume(vol.tl.rec)<=(currSector))
          return flSectorNotFound;
      tffsset(userBuffer,0,SECTOR_SIZE);
    }
  }

  return flOK;
}


#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		      r e p l a c e F A T s e c t o r 			*/
/*									*/
/* Monitors sector deletions in the FAT.				*/
/*									*/
/* When a FAT block is about to be written by an absolute write, this   */
/* routine will first scan whether any sectors are being logically	*/
/* deleted by this FAT update, and if so, it will delete-sector them	*/
/* before the actual FAT update takes place.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: FAT Sector no. about to be written		*/
/*      newFATsector	: Address of FAT sector about to be written	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus replaceFATsector(Volume vol,
			SectorNo sectorNo,
			const char FAR1 *newFATsector)
{
  const char FAR0 *oldFATsector = (const char FAR0 *) findSector(&vol,sectorNo);
  SectorNo firstSector;
  unsigned firstCluster;
#ifdef FAT_12BIT
  unsigned FAThalfBytes;
  unsigned int halfByteOffset;
#else
  unsigned int byteOffset;
#endif

  if((oldFATsector==NULL) || oldFATsector==dataErrorToken)
    return flOK;

#ifdef FAT_12BIT
  FAThalfBytes = vol.flags & VOLUME_12BIT_FAT ? 3 : 4;

  firstCluster =
	(FAThalfBytes == 3) ?
	(((unsigned) (sectorNo - vol.firstFATSectorNo) * (2 * SECTOR_SIZE) + 2) / 3) :
	((unsigned) (sectorNo - vol.firstFATSectorNo) * (SECTOR_SIZE / 2));
  firstSector =
	((SectorNo) firstCluster - 2) * vol.sectorsPerCluster + vol.firstDataSectorNo;
  halfByteOffset =
	(firstCluster * FAThalfBytes) & (2 * SECTOR_SIZE - 1);

  /* Find if any clusters were logically deleted, and if so, delete them */
  /* NOTE: We are skipping over 12-bit FAT entries which span more than  */
  /*       one sector. Nobody's perfect anyway.                          */
  for (; halfByteOffset < (2 * SECTOR_SIZE - 2);
       firstSector += vol.sectorsPerCluster, halfByteOffset += FAThalfBytes) {
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
	((unsigned) (sectorNo - vol.firstFATSectorNo) * (SECTOR_SIZE / 2));
  firstSector =
	((SectorNo) firstCluster - 2) * vol.sectorsPerCluster + vol.firstDataSectorNo;

  /* Find if any clusters were logically deleted, and if so, delete them */
  for (byteOffset = 0; byteOffset < SECTOR_SIZE;
       firstSector += vol.sectorsPerCluster, byteOffset += 2) {
    unsigned short oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + byteOffset));
    unsigned short newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + byteOffset));
#endif

    if (oldFATentry != FAT_FREE && newFATentry == FAT_FREE)
      checkStatus(vol.tl.deleteSector(vol.tl.rec,firstSector,vol.sectorsPerCluster));

    /* make sure sector is still there */
    oldFATsector = (const char FAR0 *) findSector(&vol,sectorNo);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           a b s W r i t e 				*/
/*									*/
/* Writes absolute sectors by sector no.				*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to write from		*/
/*	irSectorNo	: First sector no. to write (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to write	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irSectorCount	: Number of sectors actually written		*/
/*----------------------------------------------------------------------*/

static FLStatus absWrite(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo currSector = vol.bootSectorNo + ioreq->irSectorNo;
  SectorNo sectorCount = (SectorNo)ioreq->irSectorCount;

  if (currSector < vol.secondFATSectorNo &&
      currSector + sectorCount > vol.firstFATSectorNo) {
    SectorNo iSector;

    for (iSector = 0; iSector < sectorCount;
        iSector++, currSector++, userBuffer += SECTOR_SIZE) {

      if (currSector >= vol.firstFATSectorNo &&
          currSector < vol.secondFATSectorNo)
	replaceFATsector(&vol,currSector,userBuffer);
    }

    userBuffer = (char FAR1 *) ioreq->irData;
    currSector = (SectorNo)vol.bootSectorNo + (SectorNo)ioreq->irSectorNo;
  }

  for (ioreq->irSectorCount = 0;
       (SectorNo)(ioreq->irSectorCount) < sectorCount;
       ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE) {

    #if FILES>0
    if (currSector == buffer.sectorNo && &vol == buffer.owner) {
      buffer.sectorNo = UNASSIGNED_SECTOR;		/* no longer valid */
      buffer.dirty = buffer.checkPoint = FALSE;
    }
    #endif

    #ifdef SCATTER_GATHER
    userBuffer = *((char FAR1 **)(ioreq->irData) +(int)(ioreq->irSectorCount));
    #endif

    checkStatus(vol.tl.writeSector(vol.tl.rec,currSector,userBuffer));
  }

  return flOK;
}

#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		         f l A b s A d d r e s s			*/
/*									*/
/* Returns the current physical media offset of an absolute sector by	*/
/* sector no.								*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irSectorNo	: Sector no. to address (sector 0 is the DOS 	*/
/*			  boot sector)					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irCount		: Offset of the sector on the physical media	*/
/*----------------------------------------------------------------------*/

static FLStatus absAddress(Volume vol, IOreq FAR2 *ioreq)
{
  CardAddress cardOffset;
  const void FAR0 * sectorData =
	vol.tl.mapSector(vol.tl.rec,ioreq->irSectorNo,&cardOffset);

  if (sectorData) {
    if(sectorData==dataErrorToken)
    return flDataError;

    ioreq->irCount = cardOffset;
    return flOK;
  }
  else
    return flSectorNotFound;
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		          a b s D e l e t e 				*/
/*									*/
/* Marks absolute sectors by sector no. as deleted.			*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irSectorNo	: First sector no. to write (sector 0 is the	*/
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to delete	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus absDelete(Volume vol, IOreq FAR2 *ioreq)
{
  SectorNo first;
  first = (SectorNo)(vol.bootSectorNo + ioreq->irSectorNo);
  return vol.tl.deleteSector(vol.tl.rec,first,(SectorNo)ioreq->irSectorCount);
}

#endif /* FL_READ_ONLY   */
/*----------------------------------------------------------------------*/
/*		             g e t B P B 				*/
/*									*/
/* Reads the BIOS Parameter Block from the boot sector			*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*      irData		: Address of user buffer to read BPB into	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getBPB(Volume vol, IOreq FAR2 *ioreq)
{
  BPB FAR1 *userBPB = (BPB FAR1 *) ioreq->irData;
  DOSBootSector FAR0 *bootSector;

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;
  if(bootSector==dataErrorToken)
    return flDataError;

  *userBPB = bootSector->bpb;
  return flOK;
}

#ifndef FL_READ_ONLY
#ifdef WRITE_PROTECTION
/*----------------------------------------------------------------------*/
/*              c h a n g e P a s s w o r d                             */
/*									*/
/* Change password for write protectipon.                               */
/*									*/
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive                             */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus changePassword(Volume vol)
  {
  PartitionTable partitionTable;
  IOreq ioreq;
  FLStatus status;
 #ifdef SCATTER_GATHER
  void FAR1 *iovec[1];
 #endif

  ioreq.irHandle=(unsigned)(&vol-vols);
  ioreq.irSectorNo=0-(int)vol.bootSectorNo;
  ioreq.irSectorCount=1;
#ifdef SCATTER_GATHER
  iovec[0]     = (void FAR1 *) &partitionTable;
  ioreq.irData = (void FAR1 *) iovec;
#else
  ioreq.irData=&partitionTable;
#endif
    if((status=absRead(&vol,&ioreq))!=flOK)
     return status;
  toUNAL4(partitionTable.passwordInfo[0], 0);
  toUNAL4(partitionTable.passwordInfo[1],vol.password[0]);
  toUNAL4(partitionTable.passwordInfo[2],vol.password[1]);

  vol.flags &= ~VOLUME_WRITE_PROTECTED;

  return absWrite(&vol,&ioreq);
}

/*----------------------------------------------------------------------*/
/*              w r i t e P r o t e c t                                 */
/*									*/
/* Put and remove write protection from the volume                      */
/*									*/
/* Parameters:                                                          */
/*  irHandle         : Drive number ( 0,1,2...  )                       */
/*  irFlags          : 0=remove, 1=put                                  */
/*  irData           : password (8 bytes)                               */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus writeProtect(Volume vol,IOreq FAR2*ioreq)
{
  FLStatus status=flWriteProtect;
  unsigned long *data=(unsigned long *)ioreq->irData;
  unsigned long passCode1 = data[0] ^ SCRAMBLE_KEY_1;
  unsigned long passCode2 = data[1] ^ SCRAMBLE_KEY_2;

    switch (ioreq->irFlags) {

      case 2:     /* unlock volume */
         if((vol.password[0] == passCode1 && vol.password[1] == passCode2)||
            (vol.password[0] == 0 && vol.password[1] == 0))
            {
            vol.flags &= ~VOLUME_WRITE_PROTECTED;
            status=flOK;
            }
         else
	    status=flWriteProtect;
         break;

      case 1:     /* remove password */
         if(vol.password[0] == passCode1 && vol.password[1] == passCode2)
            {
            vol.password[0] = vol.password[1] = 0;
            status = changePassword(&vol);
            }
         else
            status=flWriteProtect;
         break;

      case 0: /* set password */
        if(vol.password[0] == 0 && vol.password[1] == 0)
           {
           vol.password[0] = passCode1;
           vol.password[1] = passCode2;
           status = changePassword(&vol);
           vol.flags|=VOLUME_WRITE_PROTECTED;
           }
        else
           status=flWriteProtect;
	      break;


	    default:
        status = flGeneralFailure;
	  }
return status;
}
#endif  /* WRITE_PROTECTION */
#endif /* FL_READ_ONLY   */

#endif /* ABS_READ_WRITE */

#ifdef LOW_LEVEL

/*----------------------------------------------------------------------*/
/*		             m o u n t L o w L e v e l 			*/
/*									*/
/* Mount a volume for low level operations. If a low level routine is   */
/* called and the volume is not mounted for low level operations, this	*/
/* routine is called atomatically. 					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus mountLowLevel(Volume vol)
{
  checkStatus(flIdentifyFlash(vol.socket,&vol.flash));
  vol.flash.setPowerOnCallback(&vol.flash);
  vol.flags |= VOLUME_LOW_LVL_MOUNTED;

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             d i s m o u n t L o w L e v e l 		*/
/*									*/
/* Dismount the volume for low level operations.			*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void dismountLowLevel(Volume vol)
{
  /* mark the volume as unmounted for low level operations, untouch other flags */
  vol.flags &= ~VOLUME_LOW_LVL_MOUNTED;
}

/*----------------------------------------------------------------------*/
/*		             g e t P h y s i c a l I n f o 		*/
/*									*/
/* Get physical information of the media. The information includes	*/
/* JEDEC ID, unit size and media size.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*      irData		: Address of user buffer to read physical	*/
/*			  information into.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getPhysicalInfo(Volume vol, IOreq FAR2 *ioreq)
{
  PhysicalInfo FAR2 *physicalInfo = (PhysicalInfo FAR2 *)ioreq->irData;

  physicalInfo->type = vol.flash.type;
  physicalInfo->unitSize = vol.flash.erasableBlockSize;
  physicalInfo->mediaSize = vol.flash.chipSize * vol.flash.noOfChips;
  physicalInfo->chipSize = vol.flash.chipSize;
  physicalInfo->interleaving = vol.flash.interleaving;
  switch(vol.flash.mediaType) {
    case NOT_DOC_TYPE:
        physicalInfo->mediaType = FL_NOT_DOC;
        break;
    case DOC_TYPE :
        physicalInfo->mediaType = FL_DOC;
        break;
    case MDOC_TYPE :
        physicalInfo->mediaType = FL_MDOC;
        break;
  }

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l R e a d	 		*/
/*									*/
/* Read from a physical address.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	irAddress	: Physical address to read from.		*/
/*	irByteCount	: Number of bytes to read.			*/
/*      irData		: Address of user buffer to read into.		*/
/*      irFlags         : Mode of the operation.                        */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalRead(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are reading whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > vol.flash.chipSize *
                                                    vol.flash.noOfChips)
    return flBadParameter;

  /* We don't read accross a unit boundary */
  if ((long)ioreq->irByteCount > vol.flash.erasableBlockSize -
                           (ioreq->irAddress % vol.flash.erasableBlockSize))
    return flBadParameter;

  checkStatus(vol.flash.read(&vol.flash, ioreq->irAddress, ioreq->irData,
		 (int)ioreq->irByteCount, ioreq->irFlags));
  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l W r i t e	 		*/
/*									*/
/* Write to a physical address.						*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	irAddress	: Physical address to write to.			*/
/*	irByteCount	: Number of bytes to write.			*/
/*      irData		: Address of user buffer to write from.		*/
/*      irFlags         : Mode of the operation.                        */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalWrite(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are writing whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > vol.flash.chipSize *
      vol.flash.noOfChips)
    return flBadParameter;

  /* We don't write accross a unit boundary */
  if ((long)ioreq->irByteCount > vol.flash.erasableBlockSize -
      (ioreq->irAddress % vol.flash.erasableBlockSize))
    return flBadParameter;

  checkStatus(vol.flash.write(&vol.flash, ioreq->irAddress, ioreq->irData,
	      (int)ioreq->irByteCount, ioreq->irFlags));
  return flOK;
}

/*----------------------------------------------------------------------*/
/*		             p h y s i c a l E r a s e	 		                      */
/*								 	                                                    */
/* Erase physical units.						                                    */
/*									                                                    */
/* Parameters:								                                          */
/*	vol		: Pointer identifying drive			                              */
/*	irUnitNo	: First unit to erase.				                            */
/*	irUnitCount	: Number of units to erase.			                        */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

static FLStatus physicalErase(Volume vol, IOreq FAR2 *ioreq)
{
  if (ioreq->irUnitNo + (long)ioreq->irUnitCount >
      vol.flash.chipSize * vol.flash.noOfChips / vol.flash.erasableBlockSize)
    return flBadParameter;

  checkStatus(vol.flash.erase(&vol.flash, (int)ioreq->irUnitNo, (int)ioreq->irUnitCount));
  return flOK;
}

#endif /* FL_READ_ONLY */
#endif /* LOW_LEVEL */


/*----------------------------------------------------------------------*/
/*                   s o c k e t I n f o                */
/*									*/
/* Get socket Information (window base address)          */
/*									*/
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive         */
/*  baseAddress : pointer to receive window base address   */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus socketInfo(Volume vol, unsigned long FAR2 *baseAddress)
{
  *baseAddress = (long)(vol.socket->window.baseAddress) << 12;
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                   f l B u i l d G e o m e t r y                      */
/*									*/
/* Get C/H/S information of the disk according to number of sectors.    */
/*									*/
/* Parameters:                                                          */
/*  capacity    : Number of Sectors in Volume                           */
/*  cylinders   : Pointer to Number of Cylinders                        */
/*  heads       : Pointer to Number of Heads                            */
/*  sectors     : Pointer to Number of Sectors per Track                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void flBuildGeometry(unsigned long capacity, unsigned long FAR2 *cylinders,
                     unsigned long FAR2 *heads,unsigned long FAR2 *sectors)
{
  unsigned long temp;

  *cylinders = 1024;                 /* Set number of cylinders to max value */
  *heads = 16L;                      /* Max out number of heads */
  temp = (*cylinders) * (*heads);    /* Compute divisor for heads */
  *sectors = capacity / temp;        /* Compute value for sectors per track */
  if (capacity % temp) {             /* If no remainder, done! */
    (*sectors)++;                    /* Else, increment number of sectors */
    temp = (*cylinders) * (*sectors);/* Compute divisor for heads */
    *heads = capacity / temp;        /* Compute value for heads */
    if (capacity % temp) {           /* If no remainder, done! */
      (*heads)++;                    /* Else, increment number of heads */
      temp = (*heads) * (*sectors);  /* Compute divisor for cylinders */
      *cylinders = (unsigned long)(capacity / temp);
    }
  }
}

/*----------------------------------------------------------------------*/
/*                   v o l u m e I n f o                */
/*									*/
/* Get general volume Information.                                    */
/*									*/
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive                             */
/*  irHandle    : Drive number (0, 1, ...)                              */
/*  irData      : pointer to VolumeInfoRecord                           */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus volumeInfo(Volume vol, IOreq FAR2 *ioreq)
{
  IOreq ioreq2;
  VolumeInfoRecord FAR2 *info = (VolumeInfoRecord FAR2 *)(ioreq->irData);
#ifdef LOW_LEVEL
  PhysicalInfo physicalInfo;
  char wasLowLevelMounted = 1;
#endif
  TLInfo tlInfo;
  unsigned long eraseCyclesPerUnit;

  ioreq2.irHandle = ioreq->irHandle;
#ifdef LOW_LEVEL
  if (!(vol.flags & VOLUME_LOW_LVL_MOUNTED)) {
    checkStatus(mountLowLevel(&vol));
    wasLowLevelMounted = 0;
  }
  ioreq2.irData = &physicalInfo;
  checkStatus(getPhysicalInfo(&vol, &ioreq2));
  info->flashType = physicalInfo.type;
  info->physicalUnitSize = (unsigned short)physicalInfo.unitSize;
  info->physicalSize = physicalInfo.mediaSize;
  info->DOCType = physicalInfo.mediaType;
#endif /* LOW_LEVEL */

  tffscpy(info->driverVer,driverVersion,sizeof(driverVersion));
  tffscpy(info->OSAKVer,OSAKVersion,sizeof(OSAKVersion));
  checkStatus(socketInfo(&vol, &(info->baseAddress)));
  checkStatus(vol.tl.getTLInfo(vol.tl.rec,&tlInfo));
  info->logicalSectors = tlInfo.sectorsInVolume;
  info->bootAreaSize = tlInfo.bootAreaSize;

  flBuildGeometry( tlInfo.sectorsInVolume,
                   (unsigned long FAR2 *)&(info->cylinders),
                   (unsigned long FAR2 *)&(info->heads),
                   (unsigned long FAR2 *)&(info->sectors) );

#ifdef LOW_LEVEL
  /* 1 million erase cycles for NAND flash, 100,000 for NOR */
  eraseCyclesPerUnit = physicalInfo.mediaType == FL_NOT_DOC ? 100000L : 1000000L;
  info->lifeTime = (char)(((tlInfo.eraseCycles /
		   (eraseCyclesPerUnit * (physicalInfo.mediaSize / physicalInfo.unitSize)))
		   % 10) + 1);
  if(!wasLowLevelMounted)
    dismountLowLevel(&vol);
#endif
  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           b d C a l l   				                                  */
/*									                                                    */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.	  */
/*                                                                      */
/* Parameters:                                                          */
/*	function	: file-system function code (listed below)	              */
/*	ioreq		: IOreq structure				                                    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

FLStatus bdCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq)
{
  Volume vol = NULL;
  FLStatus status;
#if defined(FILES) && FILES>0
  File *file;
#endif

  if (!initDone)
    checkStatus(flInit());


#if defined(FILES) && FILES>0
  /* Verify file handle if applicable */
  switch (functionNo) {
    case FL_FIND_FILE:
      if (!(ioreq->irFlags & FIND_BY_HANDLE))
	break;
#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE
    case FL_JOIN_FILE:
    case FL_SPLIT_FILE:
#endif
#endif /* FL_READ_ONLY  */
    case FL_CLOSE_FILE:
    case FL_READ_FILE:
#ifndef FL_READ_ONLY
    case FL_WRITE_FILE:
#endif
    case FL_SEEK_FILE:
    case FL_FIND_NEXT_FILE:
      if (ioreq->irHandle < FILES &&
	  (fileTable[ioreq->irHandle].flags & FILE_IS_OPEN)) {
	file = fileTable + ioreq->irHandle;
	pVol = file->fileVol;
      }
      else
	return flBadFileHandle;
  }

  if (pVol == NULL) 	/* irHandle is drive no. */
#endif
  {if (ioreq->irHandle >= noOfDrives)
    return flBadDriveHandle;
  pVol = &vols[ioreq->irHandle];}

#ifdef WRITE_PROTECTION
  if(vol.flags&VOLUME_WRITE_PROTECTED)
     if(
       #if defined(FILES) && FILES>0
	 /* fatlite functions */
	(functionNo==FL_OPEN_FILE) ||
	(functionNo==FL_CLOSE_FILE) ||
	(functionNo==FL_WRITE_FILE) ||
	(functionNo==FL_DELETE_FILE) ||
	(functionNo==FL_RENAME_FILE) ||
	(functionNo==FL_MAKE_DIR) ||
	(functionNo==FL_REMOVE_DIR) ||
	(functionNo==FL_SPLIT_FILE) ||
	(functionNo==FL_JOIN_FILE) ||
	(functionNo==FL_FLUSH_BUFFER) ||
       #endif
	0
	 /* blockdev functions */
       #ifndef FL_READ_ONLY
	||(functionNo==BD_FORMAT_VOLUME) ||
	(functionNo==FL_DEFRAGMENT_VOLUME)
       #ifdef ABS_READ_WRITE
	||(functionNo==FL_ABS_WRITE)

	||(functionNo==FL_ABS_DELETE)
       #endif

       #ifdef LOW_LEVEL
	||(functionNo==FL_PHYSICAL_WRITE)

	||(functionNo==FL_PHYSICAL_ERASE)
       #endif
       #endif /* FL_READ_ONLY */
       )
	 return flWriteProtect;

#endif /* WRITE_PROTECTION  */

  checkStatus(setBusy(&vol,ON));       /* Let everyone know we are here */

  /* Nag about mounting if necessary */
  switch (functionNo) {

#if FILES > 0
    case FL_LAST_FAT_FUNCTION:
      status = flBadFunction;
      goto flCallExit;
#endif
    case FL_UPDATE_SOCKET_PARAMS:
      break;

    case FL_ABS_MOUNT:
    case FL_MOUNT_VOLUME:
#ifndef FL_READ_ONLY
    case BD_FORMAT_VOLUME:
#endif
    #ifdef LOW_LEVEL
      if (vol.flags & VOLUME_LOW_LVL_MOUNTED)
	dismountLowLevel(&vol);  /* mutual exclusive mounting */
    #endif
      break;

#ifdef LOW_LEVEL
    case FL_GET_PHYSICAL_INFO:
    case FL_PHYSICAL_READ:
#ifndef FL_READ_ONLY
    case FL_PHYSICAL_WRITE:
    case FL_PHYSICAL_ERASE:
#endif
#ifndef BDK_ACCESS
      if (vol.flags & VOLUME_ABS_MOUNTED) {
	status = flGeneralFailure;  /* mutual exclusive mounting */
	goto flCallExit;
      }
#endif
      if (!(vol.flags & VOLUME_LOW_LVL_MOUNTED)) {
	status = mountLowLevel(&vol);  /* automatic low level mounting */
      }
      else {
	status = flOK;
      #ifndef FIXED_MEDIA
	status = flMediaCheck(vol.socket);
	if (status == flDiskChange)
	  status = mountLowLevel(&vol); /* card was changed, remount */
      #endif
      }

      if (status != flOK) {
	dismountLowLevel(&vol);
	goto flCallExit;
      }

      break;
#endif /* LOW_LEVEL */

     default:
      if (vol.flags & VOLUME_ABS_MOUNTED) {
	FLStatus status = flOK;
      #ifndef FIXED_MEDIA
	status = flMediaCheck(vol.socket);
      #endif
	if (status != flOK)
	  dismountVolume(&vol);
      }

      if (!(vol.flags & VOLUME_ABS_MOUNTED)
      #if defined(FILES) && FILES>0
	&& (functionNo > FL_LAST_FAT_FUNCTION)
      #endif
      )
      {
	status = flNotMounted;
	goto flCallExit;
      }

      switch (functionNo) {
	case FL_DISMOUNT_VOLUME:
	case FL_CHECK_VOLUME:
#ifndef FL_READ_ONLY
	case FL_DEFRAGMENT_VOLUME:
#endif
	case FL_ABS_READ:
#ifndef FL_READ_ONLY
	case FL_ABS_WRITE:
	case FL_ABS_DELETE:
#ifdef WRITE_PROTECTION
	case FL_WRITE_PROTECTION :
#endif

#endif

	case FL_SECTORS_IN_VOLUME:
	  case FL_VOLUME_INFO:
	  break;

	default:
	  if (!(vol.flags & VOLUME_MOUNTED)) {
	    status = flNotMounted;
	    goto flCallExit;
	  }
      }
  }

  /* Execute specific function */
  switch (functionNo) {

#if defined(FILES) && FILES > 0
#ifndef FL_READ_ONLY
    case FL_FLUSH_BUFFER:
      status = flushBuffer(&vol);
      break;
#endif /* FL_READ_ONLY  */
    case FL_OPEN_FILE:
      status = openFile(&vol,ioreq);
      break;

    case FL_CLOSE_FILE:
      status = closeFile(file);
      break;
#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE
    case FL_JOIN_FILE:
      status = joinFile(file, ioreq);
      break;

    case FL_SPLIT_FILE:
      status = splitFile(file, ioreq);
      break;
#endif
#endif  /* FL_READ_ONLY */
    case FL_READ_FILE:
      status = readFile(file,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_WRITE_FILE:
      status = writeFile(file,ioreq);
      break;
#endif
    case FL_SEEK_FILE:
      status = seekFile(file,ioreq);
      break;

    case FL_FIND_FILE:
      status = findFile(&vol,file,ioreq);
      break;

    case FL_FIND_FIRST_FILE:
      status = findFirstFile(&vol,ioreq);
      break;

    case FL_FIND_NEXT_FILE:
      status = findNextFile(file,ioreq);
      break;

    case FL_GET_DISK_INFO:
      status = getDiskInfo(&vol,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_DELETE_FILE:
      status = deleteFile(&vol,ioreq,FALSE);
      break;

#ifdef RENAME_FILE
    case FL_RENAME_FILE:
      status = renameFile(&vol,ioreq);
      break;
#endif

#ifdef SUB_DIRECTORY
    case FL_MAKE_DIR:
      status = makeDir(&vol,ioreq);
      break;

    case FL_REMOVE_DIR:
      status = deleteFile(&vol,ioreq,TRUE);
      break;
#endif
#endif /* FL_READ_ONLY */

    case FL_MOUNT_VOLUME:
      status = mountVolume(&vol);
      break;
#endif /* FILES > 0 */

    case FL_DISMOUNT_VOLUME:
      status = dismountVolume(&vol);
      break;

    case FL_CHECK_VOLUME:
      status = flOK;		/* If we got this far */
      break;

    case FL_UPDATE_SOCKET_PARAMS:
      status = updateSocketParameters(flSocketOf(ioreq->irHandle), ioreq->irData);
      break;
#ifndef FL_READ_ONLY
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
    case FL_DEFRAGMENT_VOLUME:
      status = defragmentVolume(&vol,ioreq);
      break;
#endif
#endif  /* FL_READ_ONLY */
#ifndef FL_READ_ONLY
#ifdef FORMAT_VOLUME
    case BD_FORMAT_VOLUME:
      status = bdFormatVolume(&vol,ioreq);
      break;
#endif
#endif /* FL_READ_ONLY */
    case FL_SECTORS_IN_VOLUME:
      status = sectorsInVolume(&vol,ioreq);
      break;

#ifdef ABS_READ_WRITE
    case FL_ABS_MOUNT:
      status = absMountVolume(&vol);
      break;
    case FL_ABS_READ:
      status = absRead(&vol,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_ABS_WRITE:
      status = absWrite(&vol,ioreq);
      break;
#endif
    case FL_ABS_ADDRESS:
      status = absAddress(&vol,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_ABS_DELETE:
      status = absDelete(&vol,ioreq);
      break;
#endif
    case FL_GET_BPB:
      status = getBPB(&vol,ioreq);
      break;
#ifndef FL_READ_ONLY
#ifdef WRITE_PROTECTION
    case FL_WRITE_PROTECTION :
	 status = writeProtect(&vol,ioreq);
	 break;
#endif
#endif /* FL_READ_ONLY */
#endif

#ifdef LOW_LEVEL
    case FL_GET_PHYSICAL_INFO:
      status = getPhysicalInfo(&vol, ioreq);
      break;

    case FL_PHYSICAL_READ:
      status = physicalRead(&vol, ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_PHYSICAL_WRITE:
      status = physicalWrite(&vol, ioreq);
      break;

    case FL_PHYSICAL_ERASE:
      status = physicalErase(&vol, ioreq);
      break;
#endif /* FL_READ_ONLY */
#endif /* LOW_LEVEL */
    case FL_VOLUME_INFO:
      status = volumeInfo(&vol, ioreq);
      break;

    default:
      status = flBadFunction;
  }

#if defined(FILES) && FILES > 0
#ifndef FL_READ_ONLY
  if (buffer.checkPoint) {
    FLStatus st = flushBuffer(&vol);
    if (status == flOK)
      status = st;
  }
#endif
#endif

flCallExit:
  if(status==flOK)
    status = setBusy(&vol,OFF);
  else
    setBusy(&vol,OFF);			/* We're leaving */

  return status;
}

#if POLLING_INTERVAL != 0

/*----------------------------------------------------------------------*/
/*      	   s o c k e t I n t e r v a l R o u t i n e		*/
/*									*/
/* Routine called by the interval timer to perform periodic socket      */
/* actions and handle the watch-dog timer.				*/
/*									*/
/* Parameters:                                                          */
/*      None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Routine called at time intervals to poll sockets */
static void socketIntervalRoutine(void)
{
  unsigned volNo;
  Volume vol = vols;

  flMsecCounter += POLLING_INTERVAL;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++)
    if (flTakeMutex(&execInProgress)) {
#ifdef FL_BACKGROUND
      if (vol.flags & VOLUME_ABS_MOUNTED)
	/* Allow background operation to proceed */
	vol.tl.tlSetBusy(vol.tl.rec,OFF);
#endif
      flIntervalRoutine(vol.socket);
      flFreeMutex(&execInProgress);
    }
}

#endif

/*----------------------------------------------------------------------*/
/*		            f l I n i t					*/
/*									*/
/* Initializes the FLite system, sockets and timers.			*/
/*									*/
/* Calling this function is optional. If it is not called,		*/
/* initialization will be done automatically on the first FLite call.	*/
/* This function is provided for those applications who want to		*/
/* explicitly initialize the system and get an initialization status.	*/
/*									*/
/* Calling flInit after initialization was done has no effect.		*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flInit(void)
{
  if (!initDone) {
    unsigned volNo;
    Volume vol = vols;

#ifdef SINGLE_BUFFER
    /* create mutex protecting FLite shared buffer */
    if (flCreateMutex(&execInProgress) != flOK)
      return flGeneralFailure;
#endif
#ifdef ENVIRONMENT_VARS

 flSetEnvVar();
 if(flUse8Bit==1)
    {
    tffscpy=flmemcpy;
    tffscmp=flmemcmp;
    tffsset=flmemset;
    }
 else
    {
    tffscpy = flcpy;
    tffsset = flset;
    tffscmp = flcmp;
    }

#endif /* ENVIRONMENT_VARS */
    for (volNo = 0; volNo < DRIVES; volNo++, pVol++) {
      vol.socket = flSocketOf(volNo);
      vol.socket->volNo = volNo;
      vol.flags = 0;
#ifdef WRITE_PROTECTION
      vol.password[0]=vol.password[1]=0;
#endif
    }
    initDone = TRUE;

  #if FILES > 0
    initFS();
  #endif

    flSysfunInit();

  #ifdef FL_BACKGROUND
    flCreateBackground();
  #endif
    noOfTLs = 0;
    checkStatus(flRegisterComponents());

  #ifdef COMPRESSION
    checkStatus(flRegisterZIP());
  #endif

    checkStatus(flInitSockets());

  #if POLLING_INTERVAL > 0
    checkStatus(flInstallTimer(socketIntervalRoutine,POLLING_INTERVAL));
  #endif
  #ifndef SINGLE_BUFFER
    for (volNo = 0,pVol = vols;
         volNo < noOfDrives; volNo++,pVol++) {
    /* create mutex protecting FLite volume access */
      if (flCreateMutex(&execInProgress) != flOK)
        return flGeneralFailure;
    }
  #endif
  }

  return flOK;
}

#ifdef EXIT

/*----------------------------------------------------------------------*/
/*		            f l E x i t					*/
/*									*/
/* If the application ever exits, flExit should be called before exit.  */
/* flExit flushes all buffers, closes all open files, powers down the   */
/* sockets and removes the interval timer.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:                                                             */
/*	Nothing								*/
/*----------------------------------------------------------------------*/

void flExit(void)
{
  extern unsigned long currentAddress;
  unsigned volNo;
  Volume vol = vols;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++) {
    if (setBusy(&vol,ON) == flOK) {
      dismountVolume(&vol);
#ifdef LOW_LEVEL
      dismountLowLevel(&vol);
#endif
      flExitSocket(vol.socket);
      flFreeMutex(&execInProgress);  /* free the mutex that was taken in setBusy(ON) */
#ifndef SINGLE_BUFFER
      /* delete mutex protecting FLite volume access */
      flDeleteMutex(&execInProgress);
#endif

    }
  }

#if POLLING_INTERVAL != 0
  flRemoveTimer();
#endif

#ifdef SINGLE_BUFFER
  /* delete mutex protecting FLite shared buffer */
  flDeleteMutex(&execInProgress);
#endif
  #ifdef ALLOCTST
  out_data_sz();
  #endif
  noOfMTDs = noOfTLs = noOfDrives = 0;
  currentAddress = 0L;
  initDone = FALSE;
}

#ifdef __BORLANDC__

#pragma exit flExit

#include <dos.h>

static int cdecl flBreakExit(void)
{
  flExit();

  return 0;
}

static void setCBreak(void)
{
  ctrlbrk(flBreakExit);
}

#pragma startup setCBreak

#endif

#endif