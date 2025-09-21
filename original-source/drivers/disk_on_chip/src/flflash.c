
/*
 * $Log:   V:/Flite/archives/FLite/src/FLFLASH.C_V  $
 * 
 *    Rev 1.20   Feb 10 2000 14:17:18   veredg
 * Initialize vendorId and firstByte in function flIntelIdentify to eliminate compiler warnings
 * 
 *    Rev 1.19   Jan 16 2000 14:32:34   vadimk
 * "fl"prefix was added to the environment variables.
 * Conditional call to isRAM function was changed .
 *
 *    Rev 1.18   Jan 13 2000 18:18:28   vadimk
 * TrueFFS OSAK 4.1
 *
 *    Rev 1.17   03 Feb 1999 18:11:50   marina
 * doc2map check edc bug fix
 *
 *    Rev 1.16   03 Sep 1998 13:58:56   ANDRY
 * better DEBUG_PRINT
 *
 *    Rev 1.15   17 Feb 1998 17:44:58   ANDRY
 * 1. debug printout is moved in the right place in isRAM()
 * 2. workaround for Visual C++ for SH3 / WinCE which cannot handle 'volatile'
 *
 *    Rev 1.14   20 Nov 1997 17:41:08   danig
 * DEBUG_PRINT in isRam()
 *
 *    Rev 1.13   03 Nov 1997 16:11:32   danig
 * amdCmdRoutine receives FlashPTR
 *
 *    Rev 1.12   10 Sep 1997 16:28:48   danig
 * Got rid of generic names
 *
 *    Rev 1.11   04 Sep 1997 18:36:24   danig
 * Debug messages
 *
 *    Rev 1.10   18 Aug 1997 11:46:26   danig
 * MTDidentifyRoutine already defined in header file
 *
 *    Rev 1.9   28 Jul 1997 14:43:30   danig
 * setPowerOnCallback
 *
 *    Rev 1.8   24 Jul 1997 17:54:28   amirban
 * FAR to FAR0
 *
 *    Rev 1.7   07 Jul 1997 15:21:38   amirban
 * Ver 2.0
 *
 *    Rev 1.6   08 Jun 1997 17:03:28   amirban
 * power on callback
 *
 *    Rev 1.5   20 May 1997 13:53:34   danig
 * Defined write/read mode
 *
 *    Rev 1.4   18 Aug 1996 13:47:24   amirban
 * Comments
 *
 *    Rev 1.3   31 Jul 1996 14:30:36   amirban
 * New flash.erase definition
 *
 *    Rev 1.2   04 Jul 1996 18:20:02   amirban
 * New flag field
 *
 *    Rev 1.1   16 Jun 1996 14:02:20   amirban
 * Corrected reset method in intelIdentify
 *
 *    Rev 1.0   20 Mar 1996 13:33:08   amirban
 * Initial revision.
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


#include "flflash.h"

#define	READ_ID			0x90
#define	INTEL_READ_ARRAY        0xff
#define	AMD_READ_ARRAY		0xf0

/* MTD registration information */

int noOfMTDs = 0;

MTDidentifyRoutine mtdTable[MTDS];

FLStatus dataErrorObject;

/*----------------------------------------------------------------------*/
/*      	            f l a s h M a p				*/
/*									*/
/* Default flash map method: Map through socket window.			*/
/* This method is applicable for all NOR Flash				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to map				*/
/*	length		: Length to map (irrelevant here)		*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to required card address				*/
/*----------------------------------------------------------------------*/

static void FAR0 *flashMap(FLFlash vol, CardAddress address, int length)
{
  return flMap(vol.socket,address);
}


/*----------------------------------------------------------------------*/
/*      	            f l a s h R e a d				*/
/*									*/
/* Default flash read method: Read by copying from mapped address	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to read				*/
/*	buffer		: Area to read into				*/
/*	length		: Length to read				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus flashRead(FLFlash vol,
			CardAddress address,
			void FAR1 *buffer,
			int length,
			int mode)
{
  tffscpy(buffer,vol.map(&vol,address,length),length);

  return flOK;
}



/*----------------------------------------------------------------------*/
/*      	         f l a s h N o W r i t e			*/
/*									*/
/* Default flash write method: Write not allowed (read-only mode)	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to write				*/
/*	buffer		: Area to write from				*/
/*	length		: Length to write				*/
/*                                                                      */
/* Returns:                                                             */
/*	Write-protect error						*/
/*----------------------------------------------------------------------*/

static FLStatus flashNoWrite(FLFlash vol,
			   CardAddress address,
			   const void FAR1 *from,
			   int length,
			   int mode)
{
  return flWriteProtect;
}


/*----------------------------------------------------------------------*/
/*      	         f l a s h N o E r a s e			*/
/*									*/
/* Default flash erase method: Erase not allowed (read-only mode)	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstBlock	: No. of first erase block			*/
/*	noOfBlocks	: No. of contiguous blocks to erase		*/
/*                                                                      */
/* Returns:                                                             */
/*	Write-protect error						*/
/*----------------------------------------------------------------------*/

static FLStatus flashNoErase(FLFlash vol,
			   int firstBlock,
			   int noOfBlocks)
{
  return flWriteProtect;
}

/*----------------------------------------------------------------------*/
/*      	         s e t N o C a l l b a c k			*/
/*									*/
/* Register power on callback routine. Default: no routine is 		*/
/* registered.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setNoCallback(FLFlash vol)
{
  flSetPowerOnCallback(vol.socket,NULL,NULL);
}

/*----------------------------------------------------------------------*/
/*      	         f l I n t e l I d e n t i f y			*/
/*									*/
/* Identify the Flash type and interleaving for Intel-style Flash.	*/
/* Sets the value of vol.type (JEDEC id) & vol.interleaving.	        */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	amdCmdRoutine	: Routine to read-id AMD/Fujitsu style at	*/
/*			  a specific location. If null, Intel procedure	*/
/*			  is used.                                      */
/*      idOffset	: Chip offset to use for identification		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

void flIntelIdentify(FLFlash vol,
		     void (*amdCmdRoutine)(FLFlash vol, CardAddress,
					   unsigned char, FlashPTR),
		     CardAddress idOffset)
{
  int inlv;

  unsigned char vendorId = 0;
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket,idOffset);
  unsigned char firstByte = 0;
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;

  for (inlv = 0; inlv < 15; inlv++) {	/* Increase interleaving until failure */
    flashPtr[inlv] = resetCmd;	/* Reset the chip */
    flashPtr[inlv] = resetCmd;	/* Once again for luck */
    if (inlv == 0)
      firstByte = flashPtr[0]; 	/* Remember byte on 1st chip */
    if (amdCmdRoutine)	/* AMD: use unlock sequence */
      amdCmdRoutine(&vol,idOffset + inlv, READ_ID, flashPtr);
    else
      flashPtr[inlv] = READ_ID;	/* Read chip id */
    if (inlv == 0)
      vendorId = flashPtr[0];	/* Assume first chip responded */
    else if (flashPtr[inlv] != vendorId || firstByte != flashPtr[0]) {
      /* All chips should respond in the same way. We know interleaving = n */
      /* when writing to chip n affects chip 0.				    */

      /* Get full JEDEC id signature */
      vol.type = (FlashType) ((vendorId << 8) | flashPtr[inlv]);
      flashPtr[inlv] = resetCmd;
      break;
    }
    flashPtr[inlv] = resetCmd;
  }

  if (inlv & (inlv - 1))
    vol.type = NOT_FLASH;		/* not a power of 2, no way ! */
  else
    vol.interleaving = inlv;
}


/*----------------------------------------------------------------------*/
/*      	            i n t e l S i z e				*/
/*									*/
/* Identify the card size for Intel-style Flash.			*/
/* Sets the value of vol.noOfChips.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	amdCmdRoutine	: Routine to read-id AMD/Fujitsu style at	*/
/*			  a specific location. If null, Intel procedure	*/
/*			  is used.                                      */
/*      idOffset	: Chip offset to use for identification		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

FLStatus flIntelSize(FLFlash vol,
		     void (*amdCmdRoutine)(FLFlash vol, CardAddress,
					   unsigned char, FlashPTR),
		     CardAddress idOffset)
{
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;
  FlashPTR flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);

  if (amdCmdRoutine)	/* AMD: use unlock sequence */
    amdCmdRoutine(&vol,0,READ_ID, flashPtr);
  else
    flashPtr[0] = READ_ID;
  /* We leave the first chip in Read ID mode, so that we can		*/
  /* discover an address wraparound.					*/

  for (vol.noOfChips = 0;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips += vol.interleaving) {
    int i;

    flashPtr = (FlashPTR) vol.map(&vol,vol.noOfChips * vol.chipSize + idOffset,0);

    /* Check for address wraparound to the first chip */
    if (vol.noOfChips > 0 &&
	(FlashType) ((flashPtr[0] << 8) | flashPtr[vol.interleaving]) == vol.type)
      goto noMoreChips;	   /* wraparound */

    /* Check if chip displays the same JEDEC id and interleaving */
    for (i = (vol.noOfChips ? 0 : 1); i < vol.interleaving; i++) {
      if (amdCmdRoutine)	/* AMD: use unlock sequence */
	amdCmdRoutine(&vol,vol.noOfChips * vol.chipSize + idOffset + i,
		      READ_ID, flashPtr);
      else
	flashPtr[i] = READ_ID;
      if ((FlashType) ((flashPtr[i] << 8) | flashPtr[i + vol.interleaving]) !=
	  vol.type)
	goto noMoreChips;  /* This "chip" doesn't respond correctly, so we're done */

      flashPtr[i] = resetCmd;
    }
  }

noMoreChips:
  flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);
  flashPtr[0] = resetCmd;		/* reset the original chip */

  return (vol.noOfChips == 0) ? flUnknownMedia : flOK;
}


/*----------------------------------------------------------------------*/
/*      	                 i s R A M				*/
/*									*/
/* Checks if the card memory behaves like RAM				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	0 = not RAM-like, other = memory is apparently RAM		*/
/*----------------------------------------------------------------------*/

static FLBoolean isRAM(FLFlash vol)
{
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket,0);
  unsigned char firstByte = flashPtr[0];
  char writeChar = (firstByte != 0) ? 0 : 0xff;
  volatile int zero=0;

  flashPtr[zero] = writeChar;              /* Write something different */
  if (flashPtr[zero] == writeChar) {       /* Was it written ? */
    flashPtr[zero] = firstByte;            /* must be RAM, undo the damage */

    DEBUG_PRINT(("Debug: error, socket window looks like RAM.\n"));
    return TRUE;
  }
  return FALSE;
}


/*----------------------------------------------------------------------*/
/*      	        f l I d e n t i f y F l a s h			*/
/*									*/
/* Identify the current Flash medium and select an MTD for it		*/
/*									*/
/* Parameters:                                                          */
/*	socket		: Socket of flash				*/
/*	vol		: New volume pointer				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 = Flash was identified			*/
/*			  other = identification failed                 */
/*----------------------------------------------------------------------*/

FLStatus flIdentifyFlash(FLSocket *socket, FLFlash vol)
{
  FLStatus status = flUnknownMedia;
  int iMTD;

  vol.socket = socket;

#ifndef FIXED_MEDIA
  /* Check that we have a media */
  flResetCardChanged(vol.socket);		/* we're mounting anyway */
  checkStatus(flMediaCheck(vol.socket));
#endif

#ifdef ENVIRONMENT_VARS
if(flUseisRAM==1)
   {
#endif
   if ( isRAM(&vol))
    return flUnknownMedia;	/* if it looks like RAM, leave immediately */
#ifdef ENVIRONMENT_VARS
   }
#endif

  /* Install default methods */
  vol.type = NOT_FLASH;
  vol.mediaType = NOT_DOC_TYPE;
  vol.flags = 0;
  vol.map = flashMap;
  vol.read = flashRead;
  vol.write = flashNoWrite;
  vol.erase = flashNoErase;
  vol.setPowerOnCallback = setNoCallback;

  /* Attempt all MTD's */
  for (iMTD = 0; iMTD < noOfMTDs && status != flOK; iMTD++)
    status = mtdTable[iMTD](&vol);

  if (status != flOK) {	/* No MTD recognition */
    /* Setup arbitrary parameters for read-only mount */
    vol.chipSize = 0x100000L;
    vol.erasableBlockSize = 0x1000;
    vol.noOfChips = 1;
    vol.interleaving = 1;

    DEBUG_PRINT(("Debug: did not identify flash media.\n"));
    return flUnknownMedia;
  }

  return flOK;
}