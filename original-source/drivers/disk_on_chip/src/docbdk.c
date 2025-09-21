/*
 * $Log:   V:/Flite/archives/OSAK/src/docbdk.c_V  $
 * 
 *    Rev 1.11   Apr 09 2000 16:43:08   vadimk
 * Remove parameters from debug messages,
 * remove c++ comments 
 *
 *    Rev 1.10   Apr 06 2000 20:11:36   vadimk
 * Names are changed in the BDK struct
 *
 *    Rev 1.9   Apr 06 2000 19:52:44   vadimk
 * add check for actualReadLen and actualUpdateLen in
 * bdkReadBlock&bdkWriteBlock
 *
 *    Rev 1.8   Mar 21 2000 12:01:24   vadimk
 * get rid of warnings
 *
 *    Rev 1.7   Mar 01 2000 13:12:00   vadimk
 * Disable EDC mechanism when signoffset = 0
 *
 *    Rev 1.6   29 Feb 2000 18:08:10   dimitrys
 * Check if 'startingBlock' inside the BDK partition
 *
 *    Rev 1.5   Feb 27 2000 16:48:20   vadimk
 * Read full partition bug was fixed
 * Signoffset bug was fixed
 *
 *    Rev 1.4   Feb 24 2000 11:27:06   vadimk
 * complete image update bug was fixed
 * write more than one block bug was fixed
 * read till real partition size is accessed bug was fixed
 *
 *    Rev 1.3   Jan 20 2000 16:30:12   danig
 * #ifdef BDK_ACCESS
 *
 *    Rev 1.2   Jan 16 2000 11:06:44   vadimk
 * Remove unnecessary includes
 *
 *    Rev 1.1   Jan 16 2000 10:38:18   vadimk
 * Remove unnecessary loop from getPhysAddressOfUnit function
 *
 *    Rev 1.0   Jan 13 2000 17:42:58   vadimk
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
#include "blockdev.h"
#include "bddefs.h"
#include "docbdk.h"

#ifdef BDK_ACCESS

/*******************************************************************/
/*   g e t P h y s A d d r e s sO f U n i t                        */
/*  Return the physical address of the unit in the BDK partition   */
/*  Parameters :                                                   */
/*    vol  :  Volume                                               */
/*    startUnit : unit in the BDK partiton                         */
/*  Return :                                                       */
/*       physical address of the unit in the BDK partition         */
/*       or NULL if failed                                         */
/*******************************************************************/
static CardAddress getPhysAddressOfUnit(Volume vol, word startUnit )
{
  word iBlock;
  byte sbuffer[SIGNATURE_LEN];
  IOreq ioreq;
  FLStatus status;
  BDKVol *bdkVol=&vol.bdk;
  ioreq.irHandle = (FLHandle) (pVol - vols);
  ioreq.irData=sbuffer;
  ioreq.irByteCount=SIGNATURE_LEN;
  ioreq.irFlags=EXTRA;

  for(iBlock=bdkVol->startImageBlock;
      ( (startUnit > 0) && (iBlock < bdkVol->endImageBlock) ); iBlock++) {
    ioreq.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
    status=flPhysicalRead(&ioreq);
    if(status!=flOK)
        {
        DEBUG_PRINT(("(EXTRA)read failed\n"));
        return 0;
        }

    if( tffscmp( (void FAR1 *)sbuffer,
		 (void FAR1 *)(bdkVol->signBuffer), SIGNATURE_NAME ) == 0 )
      startUnit--;
  }
  return( (CardAddress)iBlock << bdkVol->erasableBlockBits );
}



/*-------------------------------------------------------------------
 * getBootAreaInfo - Set 'startImageBlock', 'endImageBlock
 *             and 'bootImageSize' according to 'signature' of Boot Image.
 *             Returns OK on success, error code otherwise.
 *-------------------------------------------------------------------*/
/*IOreq FAR2 *ioreq
  if (ioreq->irHandle >= noOfDrives)
    return flBadDriveHandle;
  pVol = &vols[ioreq->irHandle];
  */
FLStatus getBootAreaInfo( IOreq FAR2 *ioreq/*char FAR2 *signature*/,unsigned long  startingBlock )
{
  IOreq ior;
  Volume vol;
  byte sbuffer[SIGNATURE_LEN];
  word numBlock,iBlock,ebsize;
  BDKVol *bdkVol;
  FLStatus status;
  if (ioreq->irHandle >= noOfDrives)
    return flBadDriveHandle;
  pVol = &vols[ioreq->irHandle];
  bdkVol=&vol.bdk;
  bdkVol->erasableBlockBits=bdkVol->startImageBlock = 0;
  bdkVol->bootImageSize = bdkVol->realBootImageSize = 0L;
  bdkVol->erasableBlockSize=(unsigned short)vol.flash.erasableBlockSize;
 /* bdkVol->bdkSignOffset=8;*/
  ebsize=bdkVol->erasableBlockSize;
  while(ebsize>1)
    {
    ebsize>>=1;
    bdkVol->erasableBlockBits++;
    }
  bdkVol->noOfBlocks=ioreq->irLength>>bdkVol->erasableBlockBits;
  ior.irHandle=ioreq->irHandle;
  ior.irData=sbuffer;
  ior.irByteCount=SIGNATURE_LEN;
  ior.irFlags=EXTRA;

  for(numBlock=0,iBlock=0;( iBlock < bdkVol->noOfBlocks );iBlock++) {
    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
    ior.irFlags=EXTRA;
    status=flPhysicalRead(&ior);
    if(status!=flOK)
      {
      DEBUG_PRINT(("(EXTRA)read failed \n"));
      return status;
      }

    if( tffscmp( (void FAR1 *)sbuffer,
       (void FAR1 *)ioreq->irData, SIGNATURE_NAME ) == 0 ) {
       if( numBlock == 0 )
          bdkVol->startImageBlock = iBlock;
       numBlock++;
       if(( bdkVol->realBootImageSize == 0L )&&(numBlock>startingBlock))
          if( tffscmp((void FAR1 *)&sbuffer[SIGNATURE_NAME],
            ( void FAR1 *)"FFFF",SIGNATURE_NUM) == 0 )
               bdkVol->realBootImageSize = (dword)numBlock << bdkVol->erasableBlockBits;
    }
    else {                   /* Check for ANAND signature */
      ior.irAddress = (CardAddress)iBlock << bdkVol->erasableBlockBits;
      ior.irFlags=0;
      status=flPhysicalRead(&ior);
      if(status!=flOK)
         {
         DEBUG_PRINT(("read failed \n"));
         return status;
         }
      if( tffscmp( (void FAR1 *)sbuffer,
		   (void FAR1 *)"ANAND", ANAND_LEN ) == 0 )
         break;
    }
  }
  if( numBlock == 0 )
    return( flNoSpaceInVolume );

  bdkVol->endImageBlock = iBlock;
  bdkVol->bootImageSize = (dword)numBlock << bdkVol->erasableBlockBits;

  if( numBlock <= startingBlock )
    return( flNoSpaceInVolume );

  if(bdkVol->realBootImageSize==0)
            bdkVol->realBootImageSize=(dword)(numBlock-startingBlock) << bdkVol->erasableBlockBits;;

  return( flOK );
}


/*-------------------------------------------------------------------
 * bdkReadInit - Init read operations on the DiskOnChip starting
 *       at 'startUnit', with a size of 'areaLen' bytes and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters: ioreq
 * Return:     flOK              - success
 *             flGeneralFailure  - DiskOnChip ASIC was not found
 *             flUnknownMedia    - fail in Flash chips recognition
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

FLStatus bdkReadInit( IOreq FAR2 *ioreq  )
{
  IOreq ior;
  word imageBlockSize;
  FLStatus status;
  PhysicalInfo physInfo;
  word startUnit;
  BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
  Volume vol=&vols[ioreq->irHandle];
  BDKVol *bdkVol=&vol.bdk;
  ior.irHandle = ioreq->irHandle;
  ior.irData = &physInfo;
  status = flGetPhysicalInfo(&ior);
  if (status != flOK) {
	DEBUG_PRINT(("Drive is not ready\n"));
  return status;
  }
 bdkVol->bdkEDC=BDKData->flags&EDC;
 ior.irData = BDKData->oldSign;
 ior.irLength=physInfo.mediaSize;
 bdkVol->bdkSignOffset = BDKData->signOffset;
  /* Set 'startImageBlock' and 'bootImageSize' of Boot Image */
 status = getBootAreaInfo( &ior, BDKData->startingBlock );

  if( status != flOK )
    return( status );

  tffscpy( (void FAR1 *)(bdkVol->signBuffer), (void FAR1 *)BDKData->oldSign, SIGNATURE_NAME );
  imageBlockSize = (word)( bdkVol->bootImageSize >> bdkVol->erasableBlockBits );
  startUnit = BDK_MIN(((word)BDKData->startingBlock/*irCount*/+1),imageBlockSize) - 1; /* to start from 0 */

  bdkVol->actualReadLen  = BDKData->length;/*areaLen;*/
  bdkVol->curReadImageBlock  = startUnit;

  if( ((dword)(imageBlockSize-startUnit) << bdkVol->erasableBlockBits) < BDKData->length )
    {
    DEBUG_PRINT(("got out of the partition \n"));
    status=flNoSpaceInVolume;
    return status/*flNoSpaceInVolume*/;
    }
  bdkVol->curReadImageAddress = getPhysAddressOfUnit(&vol, startUnit );
  return( flOK );
}

/*-------------------------------------------------------------------
 * bdkReadBlock - Read to 'buffer' from the DiskOnChip BDK Image area.
 *
 * Note: Before the first use of this function 'bdkCopyBootAreaInit'
 *       must be called
 *
 * Parameters: ioreq
 * Return:     flOK              - success
 *             flDataError       - fail in buffer reading
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/



FLStatus bdkReadBlock( IOreq FAR2 *ioreq )
{
  byte sbuffer[SIGNATURE_LEN];
  word iBlock, readLen;
  FLStatus status;
  BDKVol *bdkVol=&vols[ioreq->irHandle].bdk;
  IOreq ior;
  BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
  ior.irHandle=ioreq->irHandle;

  if(( BDKData->length/*bufferLen*/ > bdkVol->erasableBlockSize )||
     ( BDKData->length/*bufferLen*/ > bdkVol->actualReadLen ))
    return( flBadLength );


  if( (bdkVol->curReadImageAddress % bdkVol->erasableBlockSize) == 0 ) {
    ior.irData=sbuffer;
    ior.irByteCount=SIGNATURE_LEN;
    ior.irFlags=EXTRA;

    iBlock = (word)( bdkVol->curReadImageAddress >> bdkVol->erasableBlockBits );
    for(;( iBlock < bdkVol->endImageBlock ); iBlock++) {
      ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
      status=flPhysicalRead(&ior);
      if(status!=flOK)
        {
        DEBUG_PRINT(("(EXTRA)read failed \n"));
        return status;
	}

      if( tffscmp( (void FAR1 *)sbuffer,
		   (void FAR1 *)(bdkVol->signBuffer), SIGNATURE_NAME ) == 0 )
	break;
    }
    if( (bdkVol->curReadImageBlock >=
  (bdkVol->realBootImageSize >> bdkVol->erasableBlockBits)) ||
	(iBlock == bdkVol->endImageBlock) )
      return( flNoSpaceInVolume );                            /* Finish (last block) */

    bdkVol->curReadImageAddress = (dword)iBlock << bdkVol->erasableBlockBits;
    bdkVol->curReadImageBlock++;
  }

  readLen = (word)BDK_MIN(bdkVol->actualReadLen, (dword)BDKData->length/*bufferLen*/);
  ior.irData=BDKData->bdkBuffer;
  ior.irByteCount=readLen;

  if((bdkVol->bdkEDC)&&(bdkVol->bdkSignOffset!=0))
     ior.irFlags=EDC;
  else
     ior.irFlags=0;

  ior.irAddress=bdkVol->curReadImageAddress;
  status=flPhysicalRead(&ior);
  if(status!=flOK)
     {
     DEBUG_PRINT(("(EXTRA)read failed \n"));
     return status;
     }

  bdkVol->curReadImageAddress += (dword)readLen;
  bdkVol->actualReadLen  -= (dword)readLen;
  return( flOK );
}


#ifndef FL_READ_ONLY
/*   update */



/*-------------------------------------------------------------------
 * bdkWriteInit - Init update operations on the DiskOnChip starting
 *       at 'startUnit', with a size of 'areaLen' bytes and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters: ioreq
 * Return:     flOK              - success
 *             flGeneralFailure  - DiskOnChip ASIC was not found
 *             flUnknownMedia    - fail in Flash chips recognition
 *             flNoSpaceInVolume - 'areaLen' is bigger than BootImage length
 *-------------------------------------------------------------------*/


FLStatus bdkWriteInit( IOreq FAR2 *ioreq/*word startUnit, dword areaLen,
                            byte updateFlag, char FAR2 *signature*/ )
{
  word imageBlockSize;
  FLStatus status;
  IOreq ior;
  BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
  word startUnit;
  PhysicalInfo physInfo;
  Volume vol=&vols[ioreq->irHandle];
  BDKVol *bdkVol=&vol.bdk;
  bdkVol->bdkEDC=BDKData->flags&EDC;
  ior.irHandle = ioreq->irHandle;
  ior.irData = &physInfo;
  status = flGetPhysicalInfo(&ior);
  if (status != flOK) {
	DEBUG_PRINT(("Drive is not ready\n"));
  return status;
  }

  ior.irData = BDKData->oldSign;
  ior.irLength=physInfo.mediaSize;
  bdkVol->bdkSignOffset = BDKData->signOffset;
  /* Set 'startImageBlock' and 'bootImageSize' of Boot Image */
  status = getBootAreaInfo( &ior ,BDKData->startingBlock);

  if( status != flOK )
    return( status );

  tffscpy( (void FAR1 *)(bdkVol->signBuffer), (void FAR1 *)BDKData->oldSign/*signature*/, SIGNATURE_NAME );
  imageBlockSize = (word)( bdkVol->bootImageSize >> bdkVol->erasableBlockBits );
  startUnit = BDK_MIN(((word)BDKData->startingBlock/*startUnit*/+1),imageBlockSize) - 1; /* to start from 0 */

  bdkVol->actualUpdateLen  = BDKData->length/*areaLen*/;
  bdkVol->curUpdateImageBlock = startUnit;
  bdkVol->updateImageFlag = BDKData->flags&BDK_COMPLETE_IMAGE_UPDATE/*updateFlag*/;

  if( ((dword)(imageBlockSize-startUnit) << bdkVol->erasableBlockBits) < BDKData->length/*areaLen */)
    return( flNoSpaceInVolume );
  bdkVol->curUpdateImageAddress = getPhysAddressOfUnit(&vol, startUnit );
  return( flOK );
}

/*-------------------------------------------------------------------
 * bdkWriteBlock - Write 'buffer' to the DiskOnChip BDK Image area.
 *
 * Note: Before the first use of this function 'bdkUpdateBootAreaInit'
 *       must be called
 *
 * Parameters: ioreq
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

/*

irLength=bufferLen
irData = buffer
irFlags=erase_before_write
*/


FLStatus bdkWriteBlock(IOreq FAR2 *ioreq /*byte FAR1 *buffer, word bufferLen */)
{
  byte sbuffer[SIGNATURE_LEN];
  word iBlock, writeLen, i0, j;
  FLStatus status;
  IOreq ior;
  BDKVol *bdkVol=&vols[ioreq->irHandle].bdk;
  BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
  if(( BDKData->length/*bufferLen*/ > bdkVol->erasableBlockSize )||
    ( BDKData->length/*bufferLen*/ > bdkVol->actualUpdateLen ))
    return( flBadLength );
  ior.irHandle=ioreq->irHandle;
  if( (bdkVol->curUpdateImageAddress % bdkVol->erasableBlockSize) == 0 ) {

    ior.irData=sbuffer;
    ior.irByteCount=SIGNATURE_LEN;
    ior.irFlags=EXTRA;

    iBlock = (word)( bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits );
    for(;( iBlock < bdkVol->endImageBlock ); iBlock++) {
      ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
      status=flPhysicalRead(&ior);
      if(status!=flOK)
        {
        DEBUG_PRINT(("(EXTRA)read failed \n"));
        return status;
        }

      /*readDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
        (byte FAR1 *)sbuffer, SIGNATURE_LEN ); */
      if( tffscmp( (void FAR1 *)sbuffer,
		   (void FAR1 *)(bdkVol->signBuffer), SIGNATURE_NAME ) == 0 )
	break;
    }
    if( iBlock == bdkVol->endImageBlock )
      return( flNoSpaceInVolume );
    if(BDKData->flags&ERASE_BEFORE_WRITE)
       {
       ior.irUnitNo=iBlock;
       ior.irUnitCount=1;
       status=flPhysicalErase(&ior);
       if( status != flOK )
         return( status );
       }
    /* Update signature number */
    if( (bdkVol->actualUpdateLen <= bdkVol->erasableBlockSize) &&
  (bdkVol->updateImageFlag & BDK_COMPLETE_IMAGE_UPDATE) )
      tffscpy( (void FAR1 *)&sbuffer[SIGNATURE_NAME],  /* Last block FFFF */
	       (void FAR1 *)"FFFF", SIGNATURE_NUM );
    else {                              /* sbuffer <- curUpdateImageBlock */
      for(i0=bdkVol->curUpdateImageBlock,j=SIGNATURE_LEN;
          ( j > SIGNATURE_NAME );j--) {
           sbuffer[j-1] = (i0 % 10) + '0';
           i0 /= 10;
      }
    }
    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
    ior.irByteCount=SIGNATURE_LEN;
    ior.irData=sbuffer;
    ior.irFlags=EXTRA;
    status=flPhysicalWrite(&ior);
    /*status = writeDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
          (byte FAR1 *)sbuffer, SIGNATURE_LEN );*/
    if( status != flOK )
      return( status );

    bdkVol->curUpdateImageAddress = (dword)iBlock << bdkVol->erasableBlockBits;
    bdkVol->curUpdateImageBlock++;
  }

  writeLen = (word)BDK_MIN(bdkVol->actualUpdateLen, (dword)BDKData->length/*bufferLen*/);
  ior.irAddress=bdkVol->curUpdateImageAddress;
  ior.irData=BDKData->bdkBuffer;
  ior.irByteCount=writeLen;
  if((bdkVol->bdkEDC)&&(bdkVol->bdkSignOffset!=0))
     ior.irFlags=EDC;
  else
     ior.irFlags=0;
  status=flPhysicalWrite(&ior);
  if( status != flOK )
    return( status );

  bdkVol->curUpdateImageAddress += (dword)writeLen;
  bdkVol->actualUpdateLen  -= (dword)writeLen;
  return( flOK );
}





/*   ERASE   */

/*-------------------------------------------------------------------
 * bdkERASEBootAreaInit - Init erase operations on the DiskOnChip starting
 *       at 'startUnit', with a # of 'units' and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters: 'startUnit'     - start unit in image for updating
 *             'UnitNo'       -  # of units to erase
 *             'signature'     - 4-character signature of storage units
 *
 * Return:     flOK              - success
 *             flGeneralFailure  - DiskOnChip ASIC was not found
 *             flUnknownMedia    - fail in Flash chips recognition
 *             flNoSpaceInVolume - 'areaLen' is bigger than BootImage length
 *-------------------------------------------------------------------*/

/*
irCount=startUnit;
irLenght=UnitNo;
irData=signature

*/

FLStatus bdkEraseBootAreaInit( IOreq FAR2 *ioreq, unsigned char signOffset ,unsigned long startingBlock)
{
  word imageBlockSize;
  FLStatus status;
  IOreq ior;
  word startUnit;
  PhysicalInfo physInfo;
  Volume vol=&vols[ioreq->irHandle];
  BDKVol *bdkVol=&vol.bdk;

  ior.irHandle = ioreq->irHandle;
  ior.irData = &physInfo;
  status = flGetPhysicalInfo(&ior);
  if (status != flOK) {
	DEBUG_PRINT(("Drive is not ready\n"));
  return status;
  }

  bdkVol->bdkSignOffset = signOffset;
  ior.irData = ioreq->irData;
  ior.irLength=physInfo.mediaSize;
  /* Set 'startImageBlock' and 'bootImageSize' of Boot Image */
  status = getBootAreaInfo( &ior , startingBlock);

  if( status != flOK )
    return( status );

  tffscpy( (void FAR1 *)(bdkVol->signBuffer), (void FAR1 *)ioreq->irData/*signature*/, SIGNATURE_NAME );
  imageBlockSize = (word)( bdkVol->bootImageSize >> bdkVol->erasableBlockBits );
  startUnit = BDK_MIN(((word)ioreq->irCount/*startUnit*/+1),imageBlockSize) - 1; /* to start from 0 */

  bdkVol->actualUpdateLen  = ioreq->irLength<<bdkVol->erasableBlockBits/*areaLen*/;
  bdkVol->curUpdateImageBlock = startUnit;

  if( ((dword)(imageBlockSize-startUnit) << bdkVol->erasableBlockBits) < (dword)ioreq->irLength/*areaLen */)
    return( flNoSpaceInVolume );
  bdkVol->curUpdateImageAddress = getPhysAddressOfUnit(&vol, startUnit );
  return( flOK );
}

/*-------------------------------------------------------------------
 * bdkEraseBootAreaBlock - Write 'buffer' to the DiskOnChip BDK Image area.
 *
 * Note: Before the first use of this function 'bdkUpdateBootAreaInit'
 *       must be called
 *
 * Parameters: 'buffer'        - BDK image buffer
 *             'bufferLen'     - buffer length in bytes
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

/*

irCount=unitNo;


*/


FLStatus bdkEraseAreaBlock(IOreq FAR2 *ioreq /*byte FAR1 *buffer, word bufferLen */)
{
  byte sbuffer[SIGNATURE_LEN];
  word iBlock;
  FLStatus status;
  IOreq ior;
  BDKVol *bdkVol=&vols[ioreq->irHandle].bdk;
  ior.irHandle=ioreq->irHandle;
  if( (bdkVol->curUpdateImageAddress % bdkVol->erasableBlockSize) == 0 ) {

    ior.irData=sbuffer;
    ior.irByteCount=SIGNATURE_LEN;
    ior.irFlags=EXTRA;

    iBlock = (word)( bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits );
    for(;( iBlock < bdkVol->endImageBlock ); iBlock++) {
      ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
      status=flPhysicalRead(&ior);
      if(status!=flOK)
	{
        DEBUG_PRINT(("(EXTRA)read failed \n"));
        return status;
        }

      /*readDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
        (byte FAR1 *)sbuffer, SIGNATURE_LEN ); */
      if( tffscmp( (void FAR1 *)sbuffer,
		   (void FAR1 *)(bdkVol->signBuffer), SIGNATURE_NAME ) == 0 )
	break;
    }
    if( iBlock == bdkVol->endImageBlock )
      return( flNoSpaceInVolume );
    ior.irUnitNo=iBlock;
    ior.irUnitCount=1;
    status=flPhysicalErase(&ior);
    if( status != flOK )
      return( status );

    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
    ior.irByteCount=SIGNATURE_NAME;
    ior.irData=sbuffer;
    ior.irFlags=EXTRA;
    status=flPhysicalWrite(&ior);
    /*status = writeDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
          (byte FAR1 *)sbuffer, SIGNATURE_LEN );*/
    if( status != flOK )
      return( status );

    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset+SIGNATURE_NAME;
    ior.irByteCount=SIGNATURE_NAME;
    ior.irData="FFFF";
    ior.irFlags=EXTRA;
    status=flPhysicalWrite(&ior);
    if( status != flOK )
      return( status );

    bdkVol->curUpdateImageAddress = (dword)iBlock << bdkVol->erasableBlockBits;
  }
  else
  {
  DEBUG_PRINT(("cannot erase block from the middle\n"));
  return flWriteFault;
  }

  bdkVol->curUpdateImageAddress += (dword)1<<bdkVol->erasableBlockBits;
  return( flOK );
}





/*-------------------------------------------------------------------
 * bdkErase - erase given number of blockdsin the BDK area.
 *
 *
 * Parameters: ioreq
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

FLStatus bdkErase(IOreq FAR2 *ioreq)
{
IOreq ior;
FLStatus status;
unsigned long count=0;
BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
ior.irHandle=ioreq->irHandle;
ior.irLength=BDKData->length;
ior.irCount=BDKData->startingBlock;
ior.irData=BDKData->oldSign;
status=bdkEraseBootAreaInit(&ior,BDKData->signOffset,BDKData->startingBlock);
if(status!=flOK )
{
DEBUG_PRINT(("can't erase the area\n"));
return status;
}
for(count=0;count<BDKData->length;count++)
  {
   status=bdkEraseAreaBlock(&ior);
   if(status!=flOK )
    {
    DEBUG_PRINT(("can't erase the area\n"));
    return status;
    }

  }
return flOK;
}

/* C R E A T E */
/*-------------------------------------------------------------------
 * bdkCreateBootAreaInit - Init create operations on the DiskOnChip starting
 *       at 'startUnit', with a # of 'units' and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters: iotreq
 * Return:     flOK              - success
 *             flGeneralFailure  - DiskOnChip ASIC was not found
 *             flUnknownMedia    - fail in Flash chips recognition
 *             flNoSpaceInVolume - 'areaLen' is bigger than BootImage length
 *-------------------------------------------------------------------*/

/*
irCount=startUnit;
irLenght=UnitNo;
irData=signature

*/

FLStatus bdkCreateBootAreaInit( IOreq FAR2 *ioreq ,unsigned char signOffset , unsigned long startingBlock)
{
  word imageBlockSize;
  FLStatus status;
  IOreq ior;
  word startUnit;
  PhysicalInfo physInfo;
  Volume vol=&vols[ioreq->irHandle];
  BDKVol *bdkVol=&vol.bdk;

  ior.irHandle = ioreq->irHandle;
  ior.irData = &physInfo;
  status = flGetPhysicalInfo(&ior);
  if (status != flOK) {
    DEBUG_PRINT(("Drive is not ready\n"));
    return status;
  }

  bdkVol->bdkSignOffset = signOffset;
  ior.irData = ioreq->irData;
  ior.irLength=physInfo.mediaSize;
  /* Set 'startImageBlock' and 'bootImageSize' of Boot Image */
  status = getBootAreaInfo( &ior , startingBlock);

  if( status != flOK )
    return( status );

  tffscpy( (void FAR1 *)(bdkVol->signBuffer), (void FAR1 *)ioreq->irData/*signature*/, SIGNATURE_NAME );
  imageBlockSize = (word)( bdkVol->bootImageSize >> bdkVol->erasableBlockBits );
  startUnit = BDK_MIN(((word)ioreq->irCount/*startUnit*/+1),imageBlockSize) - 1; /* to start from 0 */

  bdkVol->actualUpdateLen  = ioreq->irLength<<bdkVol->erasableBlockBits/*areaLen*/;
  bdkVol->curUpdateImageBlock = startUnit;

  if( ((dword)(imageBlockSize-startUnit) << bdkVol->erasableBlockBits) < (dword)ioreq->irLength/*areaLen */)
    return( flNoSpaceInVolume );
  bdkVol->curUpdateImageAddress = getPhysAddressOfUnit(&vol, startUnit );
  return( flOK );
}

/*-------------------------------------------------------------------
 * bdkCreateBootAreaBlock - Create unit of BDK Image area.
 *
 * Note: Before the first use of this function 'bdkCreateBootAreaInit'
 *       must be called
 *
 * Parameters: ioreq
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

/*

irCount=unitNo;
irData=Signature;

*/


FLStatus bdkCreateAreaBlock(IOreq FAR2 *ioreq )
{
  byte sbuffer[SIGNATURE_LEN];
  word iBlock;
  FLStatus status;
  IOreq ior;
  BDKVol *bdkVol=&vols[ioreq->irHandle].bdk;
  ior.irHandle=ioreq->irHandle;
  if( (bdkVol->curUpdateImageAddress % bdkVol->erasableBlockSize) == 0 ) {

    ior.irData=sbuffer;
    ior.irByteCount=SIGNATURE_LEN;
    ior.irFlags=EXTRA;

    iBlock = (word)( bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits );
    for(;( iBlock < bdkVol->endImageBlock ); iBlock++) {
      ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
      status=flPhysicalRead(&ior);
      if(status!=flOK)
	{
        DEBUG_PRINT(("(EXTRA)read failed \n"));
        return status;
        }

      /*readDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
        (byte FAR1 *)sbuffer, SIGNATURE_LEN ); */
      if( tffscmp( (void FAR1 *)sbuffer,
		   (void FAR1 *)(bdkVol->signBuffer), SIGNATURE_NAME ) == 0 )
	break;
    }
    if( iBlock == bdkVol->endImageBlock )
      return( flNoSpaceInVolume );
    ior.irUnitNo=iBlock;
    ior.irUnitCount=1;
    status=flPhysicalErase(&ior);
    if( status != flOK )
      return( status );

    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset;
    ior.irByteCount=SIGNATURE_NAME;
    tffscpy(sbuffer,ioreq->irData,SIGNATURE_NAME);
    ior.irData=sbuffer;
    ior.irFlags=EXTRA;
    status=flPhysicalWrite(&ior);
    /*status = writeDocExtra( ((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset,
	  (byte FAR1 *)sbuffer, SIGNATURE_LEN );*/
    if( status != flOK )
      return( status );

    ior.irAddress=((CardAddress)iBlock << bdkVol->erasableBlockBits) + bdkVol->bdkSignOffset+SIGNATURE_NAME;
    ior.irByteCount=SIGNATURE_NAME;
    ior.irData="FFFF";
    ior.irFlags=EXTRA;
    status=flPhysicalWrite(&ior);
    if( status != flOK )
      return( status );


    bdkVol->curUpdateImageAddress = (dword)iBlock << bdkVol->erasableBlockBits;
  }
  else
  {
  DEBUG_PRINT(("cannot erase block from the middle\n"));
  return flWriteFault;
  }

  bdkVol->curUpdateImageAddress += (dword)1<<bdkVol->erasableBlockBits;
  return( flOK );
}



/*-------------------------------------------------------------------
 * bdkCreate - create new BDK partition .
 *
 *
 * Parameters: ioreq
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

FLStatus bdkCreate(IOreq FAR2 *ioreq)
{
IOreq ior;
FLStatus status;
unsigned long count;
BDKStruct *BDKData=(BDKStruct *)ioreq->irData;
ior.irHandle=ioreq->irHandle;
ior.irCount=0;
ior.irLength=BDKData->length;
ior.irData=BDKData->oldSign;
status=bdkCreateBootAreaInit(&ior,BDKData->signOffset,0);
if(status!=flOK )
    {
    DEBUG_PRINT(("can't erase the area\n"));
    return status;
    }
ior.irData=BDKData->newSign;
for(count=0;count<(dword)ior.irLength;count++)
   {
    status=bdkCreateAreaBlock(&ior);
    if(status!=flOK )
       {
       DEBUG_PRINT(("can't erase the area\n"));
       return status;
       }

   }
return flOK;
}


#endif /*   FL_READ_ONLY  */
#endif /* BDK_ACCESS */
