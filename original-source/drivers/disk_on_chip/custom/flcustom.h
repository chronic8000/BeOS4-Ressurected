/*
 * $Log:   V:/Flite/archives/OSAK/custom/FLCUSTOM.H_V  $
 * 
 *    Rev 1.1   Jun 06 1999 20:26:34   marinak
 * OSAK121
 * 
 *    Rev 1.0   May 23 1999 13:12:18   marinak
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

/*	Customization for BeOS. Copyright (c) Be Inc. 2000	*/

#ifndef FLCUSTOM_H
#define FLCUSTOM_H

/* Driver&OSAK Version numbers */
#define driverVersion   "BeIA 4.1"
#define OSAKVersion     "4.1"

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

#define DRIVES	1


/* Number of open files
 *
 * Defines the maximum number of files that may be open at a time.
 */

#define FILES	0


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


/* #define FAT_12BIT */


/* Maximum supported medium size
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 */

/*#define PARSE_PATH*/


/* Maximum supported medium size
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 */


#define MAX_VOLUME_MBYTES	144


/* Assumed card parameters
 *
 * This issue is relevant only if you are not defining any dynamic allocation
 * routines in flsystem.h.
 *
 * The following are assumptions about parameters of the Flash media.
 * They affect the size of the heap buffer allocated for the translation 
 * layer.
 */

#define ASSUMED_FTL_UNIT_SIZE	0x20000l	/* Intel interleave-2 (NOR) */
#define	ASSUMED_VM_LIMIT	0x10000l	/* limit at 64 KB */
#define ASSUMED_NFTL_UNIT_SIZE	0x4000l		/* NAND */


/* Number of buffers
 *
 * Normally two sector buffers are needed for each drive, one for the FAT
 * layer, another for the translation layer and MTD.
 * If you will uncomment the following line you will have only one sector
 * buffer which will be used by all the sockets and all the layers.
 * This will come at the cost of both read and write performance.
 * NOTE: this option is relevant only for NOR devices.
 */

/* #define SINGLE_BUFFER */


/* Absolute read & write
 *
 * Uncomment the following line if you want to be able to read & write
 * sectors by absolute sector number (i.e. without regard to files and
 * directories).
 */

#define ABS_READ_WRITE

/* Low level operations
 *
 * Uncomment the following line if you want to do low level operations
 * (i.e. read from a physical address, write to a physical address, 
 * erase a unit according to its physical unit number).
 */

#define LOW_LEVEL  


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

/* #define MIN_CLUSTER_SIZE   4 */


/*
 *
 *		Socket Hardware Customization
 *		-----------------------------
 */

/* Vpp voltage
 *
 * If your socket does not supply 12 volts, comment the following line. In
 * this case, you will be able to work only with Flash cards that do not
 * require external 12 Volt Vpp.
 *
 */

#define SOCKET_12_VOLTS


/* Fixed or removable media
 *
 * If your Flash media is fixed, uncomment the following line.
 */

#define FIXED_MEDIA


/* Interval timer
 *
 * The following defines a timer polling interval in milliseconds. If the
 * value is 0, an interval timer is not installed.
 *
 * If you select an interval timer, you should provide an implementation
 * for 'flInstallTimer' defined in flsysfun.h.
 *
 * An interval timer is not a must, but it is recommended. The following
 * will occur if an interval timer is absent:
 *
 * - Card changes can be recognized only if socket hardware detects them.
 * - The Vpp delayed turn-off procedure is not applied. This may downgrade
 *   write performance significantly if the Vpp switching time is slow.
 * - The watchdog timer that guards against certain operations being stuck
 *   indefinitely will not be active.
 */

#define	POLLING_INTERVAL 1500		/* Polling interval in millisec.
					   if 0, no polling is done */

/*
 *
 *		       Other issues
 *		       ------------
 */

/* Maximum MTD's and Translation Layers
 *
 * Define here the maximum number of Memory Technology Drivers and
 * Translation Layers that may be installed. Note that the actual
 * number installed is determined by which components are installed in
 * 'flRegisterComponents' (flcustom.c)
 */

#define MTDS	5	/* Up to 5 MTD's */

#define	TLS	3	/* Up to 3 Translation Layers */


/* Background erasing
 *
 * If you include support for Flash technology that has suspend-for-write
 * capability, you can gain considerable write performance and improve
 * real-time response for your write operations by including background erase
 * capability. In some cases, you can gain performance by including this
 * feature even if no suspend-for-write capability is supported. See the
 * FLite manual for complete details.
 *
 * On the other hand, this feature adds to required code & RAM, makes
 * necessary some additional customization on your part, and depends on
 * compiler features that, while defined as ANSI-C standard, may have
 * a problematic implementation in your particular environment. See the
 * FLite manual for complete details.
 *
 * Uncomment the following line to support background erasing.
 */

/* #define FL_BACKGROUND */


/* NFTL cash
 *
 * Enable NFTL-cache
 * Turning on this option improves performance but requires additional RAM resources.
 * The NAND Flash Translation Layer (NFTL) is a specification for storing data on DiskOnChip
 *   in a way that enables to access that data as a Virtual Block Device.
 * If this option is on then NFTL keeps in RAM table of following format:
 *   Physical Unit number    Virtual Unit number   Replacement Unit number
 *         ppp                            vvv                rrr
 * Whenever it is needed to change table entry,
 *   NFTL updates it in the RAM table and on the DiskOnChip.
 * If NFTL has to read table entry then you can save time on reading sector from DiskOnChip.
 * Accessing data described in the table is done when user read/write API function is called.
 */

#define NFTL_CACHE

/* enable environment variables  */
/* #define ENVIRONMENT_VARS */

/* Support standard IOCTL interface.
 * Turning IOCTL_INTERFACE */
/* #define IOCTL_INTERFACE */

/*  Enable write protection of the device   */
/* #define WRITE_PROTECTION */
/* Definitions required for write protection */
#ifdef WRITE_PROTECTION
#define ABS_READ_WRITE
#define SCRAMBLE_KEY_1  647777603l
#define SCRAMBLE_KEY_2  232324057l
#endif

/* Enables access to the BDK partition */
/* #define BDK_ACCESS */
/* Definitions required for BDK operations */
#ifdef BDK_ACCESS
#define LOW_LEVEL
#endif

#endif
