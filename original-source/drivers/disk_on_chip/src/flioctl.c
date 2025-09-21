/*
 * $Log:   V:/Flite/archives/OSAK/src/flioctl.c_V  $
 * 
 *    Rev 1.4   Feb 24 2000 11:34:02   vadimk
 * The declaration of inputRecord and outputRecord was separated in function flIOctl
 *
 *    Rev 1.2   Feb 03 2000 14:20:18   vadimk
 * Enclose all ioctls that do write/erase/delete operations under ifndef FL_READ_ONLY
 *
 *    Rev 1.1   Jan 23 2000 11:54:02   danig
 * Call flFormatVolume instead of flLowLevelFormat
 *
 *    Rev 1.0   Jan 13 2000 17:52:00   vadimk
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

#include "flioctl.h"
#include "blockdev.h"

#ifdef IOCTL_INTERFACE

FLStatus flIOctl(IOreq FAR2 *ioreq1)
{
  IOreq ioreq2;
  void FAR1 *inputRecord;
  void FAR1 *outputRecord;

  inputRecord = ((flIOctlRecord FAR1 *)(ioreq1->irData))->inputRecord;
  outputRecord = ((flIOctlRecord FAR1 *)(ioreq1->irData))->outputRecord;
  ioreq2.irHandle = ioreq1->irHandle;

  switch (ioreq1->irFlags) {
    case FL_IOCTL_GET_INFO:
    {
      flDiskInfoOutput FAR1 *outputRec = (flDiskInfoOutput FAR1 *)outputRecord;

      ioreq2.irData = &(outputRec->info);
      outputRec->status = flVolumeInfo(&ioreq2);
      return outputRec->status;
    }

#ifdef DEFRAGMENT_VOLUME
    case FL_IOCTL_DEFRAGMENT:
    {
      flDefragInput FAR1 *inputRec = (flDefragInput FAR1 *)inputRecord;
      flDefragOutput FAR1 *outputRec = (flDefragOutput FAR1 *)outputRecord;

      ioreq2.irLength = inputRec->requiredNoOfSectors;
      outputRec->status = flDefragmentVolume(&ioreq2);
      outputRec->actualNoOfSectors = ioreq2.irLength;
      return outputRec->status;
    }
#endif /* DEFRAGMENT_VOLUME */
#ifndef FL_READ_ONLY
#ifdef WRITE_PROTECTION
    case FL_IOCTL_WRITE_PROTECT:
    {
      flWriteProtectInput FAR1 *inputRec = (flWriteProtectInput FAR1 *)inputRecord;
      flOutputStatusRecord FAR1 *outputRec = (flOutputStatusRecord FAR1 *)outputRecord;

      ioreq2.irData = inputRec->password;
      ioreq2.irFlags = inputRec->type;
      outputRec->status = flWriteProtection(&ioreq2);
      return outputRec->status;
    }
#endif /* WRITE_PROTECTION */
#endif /* FL_READ_ONLY */
    case FL_IOCTL_MOUNT_VOLUME:
    {
      flMountInput FAR1 *inputRec = (flMountInput FAR1 *)inputRecord;
      flOutputStatusRecord FAR1 *outputRec = (flOutputStatusRecord FAR1 *)outputRecord;

      if (inputRec->type == FL_DISMOUNT)
        outputRec->status = flDismountVolume(&ioreq2);
      else
        outputRec->status = flAbsMountVolume(&ioreq2);
      return outputRec->status;
    }

#ifdef FORMAT_VOLUME
    case FL_IOCTL_FORMAT_VOLUME:
    {
      flFormatInput FAR1 *inputRec = (flFormatInput FAR1 *)inputRecord;
      flOutputStatusRecord FAR1 *outputRec = (flOutputStatusRecord FAR1 *)outputRecord;

      ioreq2.irFlags = inputRec->formatType;
      ioreq2.irData = &(inputRec->fp);
      outputRec->status = flFormatVolume(&ioreq2);
      return outputRec->status;
    }
#endif /* FORMAT_VOLUME */

#ifdef BDK_ACCESS
    case FL_IOCTL_BDK_OPERATION:
    {
      flBDKOperationInput FAR1 *inputRec = (flBDKOperationInput FAR1 *)inputRecord;
      flOutputStatusRecord FAR1 *outputRec = (flOutputStatusRecord FAR1 *)outputRecord;

      ioreq2.irData = &(inputRec->bdkStruct);
      switch(inputRec->type) {
        case BDK_INIT_READ:
          outputRec->status = bdkReadInit(&ioreq2);
          break;
    case BDK_READ:
          outputRec->status = bdkReadBlock(&ioreq2);
          break;
#ifndef FL_READ_ONLY
    case BDK_INIT_WRITE:
          outputRec->status = bdkWriteInit(&ioreq2);
          break;
    case BDK_WRITE:
          outputRec->status = bdkWriteBlock(&ioreq2);
          break;
     case BDK_ERASE:
          outputRec->status = bdkErase(&ioreq2);
          break;
     case BDK_CREATE:
          outputRec->status = bdkCreate(&ioreq2);
          break;
#endif   /* FL_READ_ONLY */
	default:
	  outputRec->status = flBadParameter;
          break;
      }
      return outputRec->status;
    }

#endif /* BDK_ACCESS */

#ifdef ABS_READ_WRITE
#ifndef FL_READ_ONLY
    case FL_IOCTL_DELETE_SECTORS:
    {
      flDeleteSectorsInput FAR1 *inputRec = (flDeleteSectorsInput FAR1 *)inputRecord;
      flOutputStatusRecord FAR1 *outputRec = (flOutputStatusRecord FAR1 *)outputRecord;

      ioreq2.irSectorNo = inputRec->firstSector;
      ioreq2.irSectorCount = inputRec->numberOfSectors;
      outputRec->status = flAbsDelete(&ioreq2);
      return outputRec->status;
    }
#endif  /* FL_READ_ONLY */
    case FL_IOCTL_READ_SECTORS:
    {
      flReadWriteInput FAR1 *inputRec = (flReadWriteInput FAR1 *)inputRecord;
      flReadWriteOutput FAR1 *outputRec = (flReadWriteOutput FAR1 *)outputRecord;

      ioreq2.irSectorNo = inputRec->firstSector;
      ioreq2.irSectorCount = inputRec->numberOfSectors;
      ioreq2.irData = inputRec->buf;
      outputRec->status = flAbsRead(&ioreq2);
      outputRec->numberOfSectors = ioreq2.irSectorCount;
      return outputRec->status;
    }
#ifndef FL_READ_ONLY
    case FL_IOCTL_WRITE_SECTORS:
    {
      flReadWriteInput FAR1 *inputRec = (flReadWriteInput FAR1 *)inputRecord;
      flReadWriteOutput FAR1 *outputRec = (flReadWriteOutput FAR1 *)outputRecord;

      ioreq2.irSectorNo = inputRec->firstSector;
      ioreq2.irSectorCount = inputRec->numberOfSectors;
      ioreq2.irData = inputRec->buf;
      outputRec->status = flAbsWrite(&ioreq2);
      outputRec->numberOfSectors = ioreq2.irSectorCount;
      return outputRec->status;
    }
#endif   /* FL_READ_ONLY */
#endif  /* ABS_READ_WRITE */

    default:
      return flBadParameter;
  }
}

#endif /* IOCTL_INTERFACE */