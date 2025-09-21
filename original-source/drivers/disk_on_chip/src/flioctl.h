/*
 * $Log:   V:/Flite/archives/OSAK/src/flioctl.h_V  $
 * 
 *    Rev 1.3   May 17 2000 11:54:52   vadimk
 * remove // commnets
 *
 *    Rev 1.2   Feb 21 2000 16:01:30   vadimk
 * add FL_UNLOCK definition
 *
 *    Rev 1.1   Jan 16 2000 13:30:56   danig
 * FL_IOCTL_START
 *
 *    Rev 1.0   Jan 13 2000 17:53:12   vadimk
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

#include "flbase.h"
#include "dosformt.h"
#include "blockdev.h"
#ifdef BDK_ACCESS
#include "docbdk.h"
#endif

#ifndef FLIOCTL_H
#define FLIOCTL_H

#ifdef IOCTL_INTERFACE

/* In every call to flIOctl function, the irFlags field in the structure
   IOreq should hold one of the following: */
typedef enum{FL_IOCTL_GET_INFO = FL_IOCTL_START,
	     FL_IOCTL_DEFRAGMENT,
             FL_IOCTL_WRITE_PROTECT,
             FL_IOCTL_MOUNT_VOLUME,
             FL_IOCTL_FORMAT_VOLUME,
             FL_IOCTL_BDK_OPERATION,
	     FL_IOCTL_DELETE_SECTORS,
             FL_IOCTL_READ_SECTORS,
             FL_IOCTL_WRITE_SECTORS} flIOctlFunctionNo;


FLStatus flIOctl(IOreq FAR2 *);


/* In every call to flIOctl function, the irData field in the structure
   IOreq should point to the structure defined below. The fields
   inputRecord and outputRecord should point to structures which are
   specific to each IOctl function as defined in this file. */
typedef struct {
  void FAR1 *inputRecord;
  void FAR1 *outputRecord;
} flIOctlRecord;


/* General output record that returns only status. */
typedef struct {
  FLStatus status;
} flOutputStatusRecord;



/* Input and output records for the different IOCTL functions: */
/* =========================================================== */

/* Get disk information (FL_IOCTL_GET_INFO) */
/* Input record: NULL */
/* Output record: */
typedef struct {
  VolumeInfoRecord info;  /* VolumeInfoRecord is defined in blockdev.h */
  FLStatus status;
} flDiskInfoOutput;


#ifdef DEFRAGMENT_VOLUME
/* Defragment volume (FL_IOCTL_DEFRAGMENT) */
/* Input record: */
typedef struct {
  long requiredNoOfSectors;   /* Minimum number of sectors to make available.
 				 if -1 then a quick garbage collection operation
                                 is invoked. */
} flDefragInput;
/* Outout record: */
typedef struct {
  long actualNoOfSectors;     /* Actual number of sectors available */
  FLStatus status;
} flDefragOutput;
#endif


#ifdef WRITE_PROTECTION
/* Write protection (FL_IOCTL_WRITE_PROTECT) */
/* Input record: */
typedef struct {
  unsigned char type;        /*  type of operation: FL_PROTECT\FL_UNPROTECT */
  long password[2];          /*  password  */
} flWriteProtectInput;
#define FL_PROTECT   0
#define FL_UNPROTECT 1
#define FL_UNLOCK    2
/* Output record: flOutputStatusRecord */
#endif

/* Mount volume (FL_IOCTL_MOUNT_VOLUME) */
/* Input record: */
typedef struct {
  unsigned char type;        /*  type of operation: FL_MOUNT\FL_DISMOUNT */
} flMountInput;
#define FL_MOUNT  	0
#define FL_DISMOUNT	1
/* Output record: flOutputStatusRecord */


#ifdef FORMAT_VOLUME
/* Format volume (FL_IOCTL_FORMAT_VOLUME) */
/* Input record: */
typedef struct {
  unsigned char formatType;   /* type of format as defined in blockdev.h */
  FormatParams fp;	      /* Format parameters structure (defined in dosformt.h) */
} flFormatInput;
/* Output record: flOutputStatusRecord */
#endif

#ifdef BDK_ACCESS
/* BDK operations read\write\erase\create (FL_IOCTL_BDK_OPERATION) */
/* Input record: */
typedef struct {
  unsigned char type;           /* type of operation: BDK_INIT_READ\BDK_READ\BDK_INIT_WRITE\
                             BDK_WRITE\BDK_ERASE\BDK_CREATE */
  BDKStruct bdkStruct;    /* parameters for BDK operations (defined in doc_bdk.h) */
} flBDKOperationInput;
#define BDK_INIT_READ   0
#define BDK_READ        1
#define BDK_INIT_WRITE  2
#define BDK_WRITE       3
#define BDK_ERASE       4
#define BDK_CREATE      5

/* Output record: flOutputStatusRecord */
#endif                                  /* BDK_ACCESS  */


#ifdef ABS_READ_WRITE
/* Delete logical sectors (FL_IOCTL_DELETE_SECTORS) */
/* Input record: */
typedef struct {
  long firstSector;		/* First logical sector to delete */
  long numberOfSectors;		/* Number of sectors to delete */
} flDeleteSectorsInput;
/* Output record: flOutputStatusRecord */



/* read & write logical sectors (FL_IOCTL_READ_SECTORS & FL_IOCTL_WRITE_SECTORS) */
/* Input record: */
typedef struct {
  long firstSector;  		/* first logical sector */
  long numberOfSectors; 	/* Number of sectors to read\write */
  char FAR1 *buf;               /* Data to read\write */
} flReadWriteInput;
/* Output record: */
typedef struct {
  long numberOfSectors; 	/* Actual Number of sectors read\written */
  FLStatus status;
} flReadWriteOutput;
#endif

#endif
#endif