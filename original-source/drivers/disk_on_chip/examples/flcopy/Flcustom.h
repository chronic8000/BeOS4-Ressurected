
/*
 * $Log:   V:/Flite/archives/OSAK/examples/flcopy/Flcustom.h_V  $
 * 
 *    Rev 1.1   Jun 13 1999 20:54:02   marinak
 * Minimal configuration that is needed
 * 
 *    Rev 1.0   May 23 1999 14:25:26   marinak
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/


#ifndef FLCUSTOM_H
#define FLCUSTOM_H

/*
 *
 *		File System Customization
 *		-------------------------
 */

/* Number of drives
 *
 * Defines the maximum number of drives (and sockets) supported.
 *
 * The actual number of drives depends on which socket controllers are
 * actually registered.
 */

#define DRIVES	2


/* Number of open files
 *
 * Defines the maximum number of files that may be open at a time.
 */

#define FILES	20


/* Sector size
 *
 * Define the log2 of sector size for the FAT & translation layers. Note 
 * that the default 512 bytes is the de-facto standard and practically 
 * the only one that provides real PC interoperability.
 */

#define SECTOR_SIZE_BITS	9


/* Formatting
 *
 * Uncomment the following line if you need to format with flFormatVolume.
 */

#define FORMAT_VOLUME


/* Defragmentation
 *
 * Uncomment the following line if you need to defragment with
 * flDefragmentVolume.
 */

#define DEFRAGMENT_VOLUME


/* Sub-directories
 *
 * Uncomment the following line if you need support for sub-directories
 */

/*#define SUB_DIRECTORY*/


/* Rename file
 *
 * Uncomment the following line if you need to rename files with flRenameFile.
 */

/*#define RENAME_FILE*/


/* Split / join file
 *
 * Uncomment the following line if you need to split or join files with
 * flSplitFile and flJoinFile
 */

/* #define SPLIT_JOIN_FILE*/


/* 12-bit FAT support
 *
 * Comment the following line if you do not need support for DOS media with
 * 12-bit FAT (typically media of 8 MBytes or less).
 */


#define FAT_12BIT


/* Maximum supported medium size
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 */

#define PARSE_PATH


/* Maximum supported medium size
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 */


#define MAX_VOLUME_MBYTES	160


/* Assumed card parameters
 *
 * This issue is relevant only if you are not defining any dynamic allocation
 * routines in flsystem.h.
 *
 * The following are assumptions about parameters of the Flash media.
 * They affect the size of the heap buffer allocated for the translation 
 * layer.
 */

#define ASSUMED_NFTL_UNIT_SIZE	0x4000l

/* Absolute read & write
 *
 * Uncomment the following line if you want to be able to read & write
 * sectors by absolute sector number (i.e. without regard to files and
 * directories).
 */

/*#define ABS_READ_WRITE*/

/* Low level operations
 *
 * Uncomment the following line if you want to do low level operations
 * (i.e. read from a physical address, write to a physical address, 
 * erase a unit according to its physical unit number).
 */

/*#define LOW_LEVEL */


/* Application exit
 *
 * If the FLite application ever exits, it needs to call flEXit before
 * exitting. Uncomment the following line to enable this.
 */

#define EXIT


/* Number of sectors per FAT cluster
 *
 * Define the minimum cluster size in sectors.
 */

#define MIN_CLUSTER_SIZE   4


/*
 * The following definitions should not be modified
*/

#define FIXED_MEDIA
#define	POLLING_INTERVAL 0
#define MTDS	1
#define	TLS	1

#endif

