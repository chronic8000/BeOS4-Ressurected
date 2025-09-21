/*
 * $Log:   V:/Flite/archives/FLite/src/BLOCKDEV.H_V  $
   
      Rev 1.8   Mar 12 2000 19:16:22   veredg
   Add definitions for fatlite functions
   
      Rev 1.7   Jan 20 2000 17:53:50   vadimk
   rename lowlevelformat to flformatvolume

      Rev 1.6   Jan 20 2000 16:31:56   danig
   In flVolumeInfo: get CHS only if ABS_READ_WRITE is defined

      Rev 1.5   Jan 20 2000 11:47:14   vadimk
   the  lifeTime field of  VolumeInfoRecord structure  was moved under LOW_LEVEL definition

      Rev 1.4   Jan 13 2000 18:02:00   vadimk
   TrueFFS OSAK 4.1

      Rev 1.3   Jul 12 1999 16:53:08   marinak
   dosFormat call is passed from blockdev.c to fatlite.c

      Rev 1.2   23 Feb 1999 20:13:54   marina
   new user call flAbsMount; TL_FORMAT_ONLY flag to avoid DOS format

      Rev 1.1   23 Dec 1998 18:31:12   amirban
   flSectorsInVolume always defined

      Rev 1.0   22 Dec 1998 14:03:36   marina
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

#ifndef BLOCKDEV_H
#define BLOCKDEV_H

#include "flreq.h"

/*----------------------------------------------------------------------*/
/*		           b d C a l l   				                                  */
/*									                                                    */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.	  */
/*                                                                      */
/* Parameters:                                                          */
/*	function	: Block device driver function code (listed below)	      */
/*	ioreq		: IOreq structure				                                    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

typedef enum {
#if FILES > 0
  FL_OPEN_FILE,
  FL_CLOSE_FILE,
  FL_READ_FILE,
  FL_WRITE_FILE,
  FL_SEEK_FILE,
  FL_FIND_FILE,
  FL_FIND_FIRST_FILE,
  FL_FIND_NEXT_FILE,
  FL_GET_DISK_INFO,
  FL_DELETE_FILE,
  FL_RENAME_FILE,
  FL_MAKE_DIR,
  FL_REMOVE_DIR,
  FL_SPLIT_FILE,
  FL_JOIN_FILE,
  FL_FLUSH_BUFFER,
  FL_LAST_FAT_FUNCTION,
#endif
  FL_MOUNT_VOLUME,
  FL_DISMOUNT_VOLUME,
  FL_CHECK_VOLUME,
  FL_DEFRAGMENT_VOLUME,
  FL_ABS_MOUNT,
  FL_ABS_READ,
  FL_ABS_WRITE,
  FL_ABS_ADDRESS,
  FL_ABS_DELETE,
  FL_GET_BPB,
  FL_GET_PHYSICAL_INFO,
  FL_PHYSICAL_READ,
  FL_PHYSICAL_WRITE,
  FL_PHYSICAL_ERASE,
  FL_UPDATE_SOCKET_PARAMS,
  FL_SECTORS_IN_VOLUME,
  BD_FORMAT_VOLUME,
  FL_WRITE_PROTECTION,
  FL_VOLUME_INFO
} FLFunctionNo;


FLStatus bdCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq);

#if FILES > 0
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		     f l F l u s h B u f f e r                                    */
/*									                                                    */
/* If there is relevant data in the RAM buffer then writes it on        */
/*   the flash memory.                                                  */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)                                */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

#define flFlushBuffer(ioreq)	bdCall(FL_FLUSH_BUFFER,ioreq)

#endif                                  /* READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l O p e n F i l e				*/
/*									*/
/* Opens an existing file or creates a new file. Creates a file handle  */
/* for further file processing.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irFlags		: Access and action options, defined below	*/
/*	irPath		: path of file to open             		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irHandle	: New file handle for open file                 */
/*                                                                      */
/*----------------------------------------------------------------------*/

/** Values of irFlags for flOpenFile: */

#define ACCESS_MODE_MASK	3	/* Mask for access mode bits */

/* Individual flags */

#define	ACCESS_READ_WRITE	1	/* Allow read and write */
#define ACCESS_CREATE		2	/* Create new file */

/* Access mode combinations */
#define OPEN_FOR_READ		0	/* open existing file for read-only */
#define	OPEN_FOR_UPDATE		1	/* open existing file for read/write access */
#define OPEN_FOR_WRITE		3	/* create a new file, even if it exists */


#define flOpenFile(ioreq)	bdCall(FL_OPEN_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l C l o s e F i l e				*/
/*									*/
/* Closes an open file, records file size and dates in directory and    */
/* releases file handle.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Handle of file to close.                      */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flCloseFile(ioreq)      bdCall(FL_CLOSE_FILE,ioreq)

#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE

/*------------------------------------------------------------------------*/
/*		      f l S p l i t F i l e                               */
/*                                                                        */
/* Splits the file into two files. The original file contains the first   */
/* part, and a new file (which is created for that purpose) contains      */
/* the second part. If the current position is on a cluster               */
/* boundary, the file will be split at the current position. Otherwise,   */
/* the cluster of the current position is duplicated, one copy is the     */
/* first cluster of the new file, and the other is the last cluster of the*/
/* original file, which now ends at the current position.                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to split.                                  */
/*      irPath          : Path name of the new file.                      */
/*                                                                        */
/* Returns:                                                               */
/*	irHandle        : handle of the new file.                         */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flSplitFile(ioreq)     bdCall(FL_SPLIT_FILE,ioreq)


/*------------------------------------------------------------------------*/
/*		      f l J o i n F i l e                                 */
/*                                                                        */
/* joins two files. If the end of the first file is on a cluster          */
/* boundary, the files will be joined there. Otherwise, the data in       */
/* the second file from the beginning until the offset that is equal to   */
/* the offset in cluster of the end of the first file will be lost. The   */
/* rest of the second file will be joined to the first file at the end of */
/* the first file. On exit, the first file is the expanded file and the   */
/* second file is deleted.                                                */
/* Note: The second file will be open by this function, it is advised to  */
/*	 close it before calling this function in order to avoid          */
/*	 inconsistencies.                                                 */
/*                                                                        */
/* Parameters:                                                            */
/*	file            : file to join to.                                */
/*	irPath          : Path name of the file to be joined.             */
/*                                                                        */
/* Return:                                                                */
/*	FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flJoinFile(ioreq)     bdCall(FL_JOIN_FILE,ioreq)

#endif /* SPLIT_JOIN_FILE */
#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l R e a d F i l e				*/
/*									*/
/* Reads from the current position in the file to the user-buffer.	*/
/* Parameters:                                                          */
/*	irHandle	: Handle of file to read.                       */
/*      irData		: Address of user buffer			*/
/*	irLength	: Number of bytes to read. If the read extends  */
/*			  beyond the end-of-file, the read is truncated */
/*			  at the end-of-file.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual number of bytes read			*/
/*----------------------------------------------------------------------*/

#define flReadFile(ioreq)	bdCall(FL_READ_FILE,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		      f l W r i t e F i l e				*/
/*									*/
/* Writes from the current position in the file from the user-buffer.   */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Handle of file to write.			*/
/*      irData		: Address of user buffer			*/
/*	irLength	: Number of bytes to write.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual number of bytes written		*/
/*----------------------------------------------------------------------*/

#define flWriteFile(ioreq)	bdCall(FL_WRITE_FILE,ioreq)

#endif  /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		      f l S e e k F i l e				*/
/*									*/
/* Sets the current position in the file, relative to file start, end or*/
/* current position.							*/
/* Note: This function will not move the file pointer beyond the	*/
/* beginning or end of file, so the actual file position may be		*/
/* different from the required. The actual position is indicated on     */
/* return.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: File handle to close.                         */
/*      irLength	: Offset to set position.			*/
/*	irFlags		: Method code					*/
/*			  SEEK_START: absolute offset from start of file  */
/*			  SEEK_CURR:  signed offset from current position */
/*			  SEEK_END:   signed offset from end of file    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irLength	: Actual absolute offset from start of file	*/
/*----------------------------------------------------------------------*/

/** Values of irFlags for flSeekFile: */

#define	SEEK_START	0	/* offset from start of file */
#define	SEEK_CURR	1	/* offset from current position */
#define	SEEK_END	2	/* offset from end of file */


#define flSeekFile(ioreq)	bdCall(FL_SEEK_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		          f l F i n d F i l e				*/
/*                                                                      */
/* Finds a file entry in a directory, optionally modifying the file     */
/* time/date and/or attributes.                                         */
/* Files may be found by handle no. provided they are open, or by name. */
/* Only the Hidden, System or Read-only attributes may be modified.	*/
/* Entries may be found for any existing file or directory other than   */
/* the root. A DirectoryEntry structure describing the file is copied   */
/* to a user buffer.							*/
/*                                                                      */
/* The DirectoryEntry structure is defined in dosformt.h		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: If by name: Drive number (0, 1, ...)		*/
/*			  else      : Handle of open file		*/
/*	irPath		: If by name: Specifies a file or directory path*/
/*	irFlags		: Options flags					*/
/*			  FIND_BY_HANDLE: Find open file by handle. 	*/
/*					  Default is access by path.    */
/*                        SET_DATETIME:	Update time/date from buffer	*/
/*			  SET_ATTRIBUTES: Update attributes from buffer	*/
/*	irDirEntry	: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irLength	: Modified					*/
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

/** Bit assignment of irFlags for flFindFile: */

#define SET_DATETIME	1	/* Change date/time */
#define	SET_ATTRIBUTES	2	/* Change attributes */
#define	FIND_BY_HANDLE	4	/* Find file by handle rather than by name */

#define	flFindFile(ioreq)	bdCall(FL_FIND_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		 f l F i n d F i r s t F i l e				*/
/*                                                                      */
/* Finds the first file entry in a directory.				*/
/* This function is used in combination with the flFindNextFile call,   */
/* which returns the remaining file entries in a directory sequentially.*/
/* Entries are returned according to the unsorted directory order.	*/
/* flFindFirstFile creates a file handle, which is returned by it. Calls*/
/* to flFindNextFile will provide this file handle. When flFindNextFile */
/* returns 'noMoreEntries', the file handle is automatically closed.    */
/* Alternatively the file handle can be closed by a 'closeFile' call    */
/* before actually reaching the end of directory.			*/
/* A DirectoryEntry structure is copied to the user buffer describing   */
/* each file found. This structure is defined in dosformt.h.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: Specifies a directory path			*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	irHandle	: File handle to use for subsequent operations. */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define	flFindFirstFile(ioreq)	bdCall(FL_FIND_FIRST_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		 f l F i n d N e x t F i l e				*/
/*                                                                      */
/* See the description of 'flFindFirstFile'.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: File handle returned by flFindFirstFile.	*/
/*	irData		: Address of user buffer to receive a		*/
/*			  DirectoryEntry structure			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define	flFindNextFile(ioreq)	bdCall(FL_FIND_NEXT_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l G e t D i s k I n f o				*/
/*									*/
/* Returns general allocation information.				*/
/*									*/
/* The bytes/sector, sector/cluster, total cluster and free cluster	*/
/* information are returned into a DiskInfo structure.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irData		: Address of DiskInfo structure                 */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned	bytesPerSector;
  unsigned	sectorsPerCluster;
  unsigned	totalClusters;
  unsigned	freeClusters;
} DiskInfo;


#define flGetDiskInfo(ioreq)	bdCall(FL_GET_DISK_INFO,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		      f l D e l e t e F i l e				*/
/*									*/
/* Deletes a file.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of file to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flDeleteFile(ioreq)	bdCall(FL_DELETE_FILE,ioreq)


#ifdef RENAME_FILE

/*----------------------------------------------------------------------*/
/*		      f l R e n a m e F i l e				*/
/*									*/
/* Renames a file to another name.					*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of existing file				*/
/*      irData		: path of new name.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flRenameFile(ioreq)	bdCall(FL_RENAME_FILE,ioreq)

#endif /* RENAME_FILE */


#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*		      f l M a k e D i r					*/
/*									*/
/* Creates a new directory.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of new directory.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flMakeDir(ioreq)	bdCall(FL_MAKE_DIR,ioreq)


/*----------------------------------------------------------------------*/
/*		      f l R e m o v e D i r				*/
/*									*/
/* Removes an empty directory.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irPath		: path of directory to remove.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flRemoveDir(ioreq)	bdCall(FL_REMOVE_DIR,ioreq)

#endif /* SUB_DIRECTORY */
#endif /* FL_READ_ONLY */

#endif /* FILES > 0 */

#ifdef PARSE_PATH

/*----------------------------------------------------------------------*/
/*		      f l P a r s e P a t h				*/
/*									*/
/* Converts a DOS-like path string to a simple-path array.		*/
/*									*/
/* Note: Array length received in irPath must be greater than the 	*/
/* number of path components in the path to convert.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*	irData		: address of path string to convert		*/
/*	irPath		: address of array to receive parsed-path. 	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

extern FLStatus flParsePath(IOreq FAR2 *ioreq);

#endif /* PARSE_PATH */

/*----------------------------------------------------------------------*/
/*		      f l M o u n t V o l u m e                         */
/*                                                                      */
/* Mounts, verifies or dismounts the Flash medium.                      */
/*                                                                      */
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.                                                          */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)                      */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flMountVolume(ioreq)	bdCall(FL_MOUNT_VOLUME,ioreq)

/*----------------------------------------------------------------------*/
/*                  f l A b s M o u n t V o l u m e                     */
/*                                                                      */
/* Mounts, verifies or dismounts the Flash medium.                      */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)                      */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flAbsMountVolume(ioreq)	bdCall(FL_ABS_MOUNT,ioreq)


/*----------------------------------------------------------------------*/
/*		   f l D i s m o u n t V o l u m e			                          */
/*									                                                    */
/* Dismounts the volume.                   				                      */
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed.						                                    */
/*									                                                    */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			                          */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus		: 0 on success, otherwise failed                        */
/*----------------------------------------------------------------------*/

#define flDismountVolume(ioreq)	bdCall(FL_DISMOUNT_VOLUME,ioreq)


/*----------------------------------------------------------------------*/
/*		     f l C h e c k V o l u m e				                            */
/*									                                                    */
/* Verifies that the current volume is mounted.				                  */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			                          */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

#define flCheckVolume(ioreq)	bdCall(FL_CHECK_VOLUME,ioreq)

#ifndef FL_READ_ONLY
#ifdef DEFRAGMENT_VOLUME

/*----------------------------------------------------------------------*/
/*		      f l D e f r a g m e n t V o l u m e		                      */
/*									                                                    */
/* Performs a general defragmentation and recycling of non-writable	    */
/* Flash areas, to achieve optimal write speed.				                  */
/*                                                                      */
/* NOTE: The required number of sectors (in irLength) may be changed    */
/* (from another execution thread) while defragmentation is active. In  */
/* particular, the defragmentation may be cut short after it began by	  */
/* modifying the irLength field to 0.					                          */
/*									                                                    */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			                          */
/*	irLength	: Minimum number of sectors to make available             */
/*			  for writes.					                                          */
/*                                                                      */
/* Returns:                                                             */
/*	irLength	: Actual number of sectors available for writes	          */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

#define flDefragmentVolume(ioreq)	bdCall(FL_DEFRAGMENT_VOLUME,ioreq)

#endif /* DEFRAGMENT_VOLUME */


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*                    f l F o r m a t V o l u m e                       */
/*                                                                      */
/* Performs  formatting of the DiskOnChip.                              */
/*  All existing data is destroyed.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)                      */
/*	irFlags		: TL_FORMAT: Translation layer formatting       */
/*			  TL_FORMAT_IF_NEEDED: Do FTL formatting only   */
/*				      if current format is invalid      */
/*                      : FAT_ONLY_FORMAT -  FAT only formatting        */
/*			  ZIP_FORMAT: Create compressed drive           */
/*                      : TL_FORMAT_ONLY  Translation layer             */
/*                                        formatting without FAT format */
/*	irData		: Address of FormatParams structure to use      */
/*			  (defined in dosformt.h)                       */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

/** Values of irFlags for flLowLevelFormat: */

#define FAT_ONLY_FORMAT	0	/* FAT only formatting */
#define	TL_FORMAT	1	/* Translation layer formatting */
#define	TL_FORMAT_IF_NEEDED 2	/* Translation layer formatting if necessary only */
#define TL_FORMAT_ONLY  8       /* Translation layer formatting without FAT format */
#define	ZIP_FORMAT	4	/* Create compressed drive */

#define flFormatVolume(ioreq) bdCall(BD_FORMAT_VOLUME,ioreq)

#endif /* FORMAT_VOLUME */


#endif /*FL_READ_ONLY */


/*----------------------------------------------------------------------*/
/*		 f l S e c t o r s I n V o l u m e			                          */
/*									                                                    */
/* Returns number of virtual sectors in volume. 			                  */
/*									                                                    */
/* In case the inserted volume is not mounted, returns current status.  */
/*                                                                      */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			                          */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*      irLength        : number of virtual sectors in volume           */
/*----------------------------------------------------------------------*/

#define flSectorsInVolume(ioreq)	bdCall(FL_SECTORS_IN_VOLUME,ioreq)


#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*		           f l A b s R e a d 		                                  */
/*									                                                    */
/* Reads absolute sectors by sector no.					                        */
/*									                                                    */
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			                          */
/*      irData		: Address of user buffer to read into		              */
/*	irSectorNo	: First sector no. to read (sector 0 is the	            */
/*			  DOS boot sector).				                                      */
/*	irSectorCount	: Number of consectutive sectors to read	            */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*	irSectorCount	: Number of sectors actually read		                  */
/*----------------------------------------------------------------------*/

#define flAbsRead(ioreq)	bdCall(FL_ABS_READ,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		         f l A b s W r i t e 				*/
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

#define flAbsWrite(ioreq)	bdCall(FL_ABS_WRITE,ioreq)
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

#define flAbsAddress(ioreq)		bdCall(FL_ABS_ADDRESS,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		         f l A b s D e l e t e 				*/
/*									*/
/* Marks absolute sectors by sector no. as deleted.			*/
/*									*/
/* Parameters:                                                          */
/*	irHandle	: Drive number (0, 1, ...)			*/
/*  irSectorNo  : First sector no. to delete (sector 0 is the    */
/*			  DOS boot sector).				*/
/*	irSectorCount	: Number of consectutive sectors to delete	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*	irSectorCount	: Number of sectors actually deleted		*/
/*----------------------------------------------------------------------*/

#define flAbsDelete(ioreq)	bdCall(FL_ABS_DELETE,ioreq)

#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*		           f l G e t B P B 				*/
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

#define flGetBPB(ioreq)		bdCall(FL_GET_BPB,ioreq)


#ifndef FL_READ_ONLY
#ifdef WRITE_PROTECTION
/*----------------------------------------------------------------------*/
/*              f l W r i t e P r o t e c t i o n                       */
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

#define flWriteProtection(ioreq) bdCall(FL_WRITE_PROTECTION,ioreq)
#endif
#endif /* FL_READ_ONLY */
#endif /* ABS_READ_WRITE */


#ifdef LOW_LEVEL

/*----------------------------------------------------------------------*/
/*			  P h y s i c a l I n f o			*/
/*									*/
/* A structure that holds physical information about the media. The 	*/
/* information includes JEDEC ID, unit size and media size. Pointer	*/
/* to this structure is passed to the function flGetPhysicalInfo where  */
/* it receives the relevant data.					*/
/*									*/
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned short	type;			/* Flash device type (JEDEC id) */
  char          mediaType;          /* type of media (DOC/MDOC etc.) */
  long int		unitSize;		/* Smallest physically erasable size
						   (with interleaving taken in account) */
  long int		mediaSize;		/* media size */
  long int		chipSize;		/* individual chip size */
  int			interleaving;		/* device interleaving */
} PhysicalInfo;

/* media types*/
#define FL_NOT_DOC  0
#define FL_DOC      1
#define FL_MDOC     2

/*----------------------------------------------------------------------*/
/*		         f l G e t P h y s i c a l I n f o 		*/
/*									*/
/* Get physical information of the media. The information includes	*/
/* JEDEC ID, unit size and media size.					*/
/*									*/
/* Parameters:								*/
/*	irHandle	: Drive number (0,1,..)				*/
/*      irData		: Address of user buffer to read physical	*/
/*			  information into.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flGetPhysicalInfo(ioreq)	bdCall(FL_GET_PHYSICAL_INFO, ioreq)

/*----------------------------------------------------------------------*/
/*		             f l P h y s i c a l R e a d	 	*/
/*									*/
/* Read from a physical address.					*/
/*									*/
/* Parameters:								*/
/*	irHandle	: Drive number (0,1,..)				*/
/*	irAddress	: Physical address to read from.		*/
/*	irByteCount	: Number of bytes to read.			*/
/*      irData		: Address of user buffer to read into.		*/
/*	irFlags		: Method mode					*/
/*                        OVERWRITE: Overwriting non-erased area        */
/*                        EDC:       Activate ECC/EDC	                */
/*                        EXTRA:     Read/write spare area              */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flPhysicalRead(ioreq)		bdCall(FL_PHYSICAL_READ,ioreq)


#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		             f l P h y s i c a l W r i t e	 	*/
/*									*/
/* Write to a physical address.						*/
/*									*/
/* Parameters:								*/
/*	irHandle	: Drive Number (0,1,..)				*/
/*	irAddress	: Physical address to write to.			*/
/*	irByteCount	: Number of bytes to write.			*/
/*      irData		: Address of user buffer to write from.		*/
/*	irFlags		: Method mode					*/
/*                        OVERWRITE: Overwriting non-erased area        */
/*                        EDC:       Activate ECC/EDC	                */
/*                        EXTRA:     Read/write spare area              */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flPhysicalWrite(ioreq)		bdCall(FL_PHYSICAL_WRITE,ioreq)

/* Bit assignment of irFlags for flPhysicalRead or flPhysicalWrite: */
/*   ( defined in file flflash.h )                                  */
/* #define OVERWRITE	1	*//* Overwriting non-erased area  */
/* #define EDC		2	*//* Activate ECC/EDC		*/
/* #define EXTRA	4	*//* Read/write spare area	*/

/*----------------------------------------------------------------------*/
/*		             f l P h y s i c a l E r a s e	 	*/
/*									*/
/* Erase physical units.						*/
/*									*/
/* Parameters:								*/
/* 	irHandle	: Drive number (0,1,..)				*/
/*	irUnitNo	: First unit to erase.				*/
/*	irUnitCount	: Number of units to erase.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flPhysicalErase(ioreq)		bdCall(FL_PHYSICAL_ERASE,ioreq)

#endif /* FL_READ_ONLY */
#endif /* LOW_LEVEL */


/*----------------------------------------------------------------------*/
/*		 f l U p d a t e S o c k e t P a r a m s		*/
/*									*/
/* Pass socket parameters to the socket interface layer.		*/
/* This function should be called after the socket parameters (like	*/
/* size and base) are known. If these parameters are known at 		*/
/* registration time then there is no need to use this function, and	*/
/* the parameters can be passed to the registration routine.		*/
/* The structure passed in irData is specific for each socket interface.*/
/*									*/
/* Parameters:								*/
/*	irHandle	: volume number					*/
/*	irData		: pointer to structure that hold socket         */
/*			  parameters					*/
/*									*/
/* Returns:								*/
/*	FLStatus 	: 0 on success, otherwise failed.		*/
/*----------------------------------------------------------------------*/

#define flUpdateSocketParams(ioreq)	bdCall(FL_UPDATE_SOCKET_PARAMS,ioreq)



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

extern void flExit(void);

#endif /* EXIT */


/*----------------------------------------------------------------------*/
/*        V o l u m e I n f o R e c o r d     */
/*									*/
/* A structure that holds general information about the media. The   */
/* information includes Physical Info (see flGetPhysicalInfo), Logical  */
/* partition (number of sectors and CHS), boot area size, S/W versions  */
/* Media life-time etc.                                                 */
/* A pointer to this structure is passed to the function flVolumeInfo   */
/* where it receives the relevant data.                                 */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned long logicalSectors;     /*  number of logical sectors */
  unsigned long bootAreaSize;       /*  boot area size              */
  unsigned long  baseAddress;       /*  physical base address       */
#ifdef LOW_LEVEL
  unsigned short flashType;         /*  JEDEC id of the flash       */
  unsigned long physicalSize;       /*  physical size of the media  */
  unsigned short physicalUnitSize;  /*  Erasable block size         */
  char DOCType;                     /*  DiskOnChip type (MDoc/Doc2000) */
  char lifeTime;                    /*  Life time indicator for the media (1-10)     */
                                    /*  1 - the media is fresh,                 */
                                    /*  10 - the media is close to its end of life */

#endif
  char driverVer[10];               /*  driver version (NULL terminated string) */
  char OSAKVer[10];                 /*  OSAK version that driver is based on (NULL terminated string) */
#ifdef ABS_READ_WRITE
  unsigned long cylinders;              /*  Media.....                          */
  unsigned long heads;		    	/*            geometry......		*/
  unsigned long sectors; 	    	/*    			parameters.	*/
#endif
} VolumeInfoRecord;

/*----------------------------------------------------------------------*/
/*             f l V o l u m e I n f o    */
/*									*/
/* Get general information about the media.                             */
/*									*/
/* Parameters:								*/
/*	irHandle	: Drive number (0,1,..)				*/
/*      irData    : Address of user buffer to read general */
/*			  information into.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flVolumeInfo(ioreq) bdCall(FL_VOLUME_INFO,ioreq)


#endif