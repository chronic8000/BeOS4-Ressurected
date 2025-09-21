
/*
 * $Log:   V:/Flite/archives/FLite/src/FLTL.C_V  $
 * 
 *    Rev 1.7   Jan 13 2000 18:26:50   vadimk
 * TrueFFS OSAK 4.1
 * 
 *    Rev 1.6   Jun 24 1999 11:51:32   marinak
 * write/read MultiSector initialization
 * 
 *    Rev 1.5   23 Dec 1998 17:34:44   amirban
 * Add status to noFormat
 * 
 *    Rev 1.4   26 Oct 1998 18:06:34   marina
 * function noFormat is added
 * 
 *    Rev 1.3   16 Aug 1998 20:29:38   amirban
 * TL definition changes for ATA & ZIP
 * 
 *    Rev 1.2   28 Jul 1997 14:48:06   danig
 * Call to setPowerOnCallback in flMount
 * 
 *    Rev 1.1   20 Jul 1997 17:14:54   amirban
 * Format change
 * 
 *    Rev 1.0   07 Jul 1997 15:23:10   amirban
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
#include "fltl.h"


int noOfTLs;	/* No. of translation layers actually registered */

TLentry tlTable[TLS];


/*----------------------------------------------------------------------*/
/*      	             f l M o u n t 				*/
/*									*/
/* Mount a translation layer						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	tl		: Where to store translation layer methods	*/
/*	useFilters	: Whether to use filter translation-layers	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flMount(unsigned volNo, TL *tl, FLBoolean useFilters)
{
  FLFlash flash, *volForCallback = NULL;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  FLStatus flashStatus = flIdentifyFlash(socket,&flash);
  if (flashStatus != flOK && flashStatus != flUnknownMedia)
    return flashStatus;

  tl->recommendedClusterInfo = NULL;
  tl->writeMultiSector = NULL;
  tl->readSectors = NULL;

  for (iTL = 0; iTL < noOfTLs && status != flOK; iTL++)
    if (tlTable[iTL].formatRoutine != NULL)	/* not a block-device filter */
      status = tlTable[iTL].mountRoutine(volNo,tl,flashStatus == flOK ? &flash : NULL,&volForCallback);

  if (status == flOK) {
    if (volForCallback)
      volForCallback->setPowerOnCallback(volForCallback);

    if (useFilters)
      for (iTL = 0; iTL < noOfTLs; iTL++)
	if (tlTable[iTL].formatRoutine ==  NULL)	/* block-device filter */
	  if (tlTable[iTL].mountRoutine(volNo,tl,NULL,NULL) == flOK)
	    break;
  }

  return status;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*      	             f l F o r m a t 				*/
/*									*/
/* Formats the Flash volume						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flFormat(unsigned volNo, FormatParams FAR1 *formatParams)
{
  FLFlash flash;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  FLStatus flashStatus = flIdentifyFlash(socket,&flash);
  if (flashStatus != flOK && flashStatus != flUnknownMedia)
    return flashStatus;

  for (iTL = 0; iTL < noOfTLs && status == flUnknownMedia; iTL++)
    if (tlTable[iTL].formatRoutine != NULL)	/* not a block-device filter */
      status = tlTable[iTL].formatRoutine(volNo,formatParams,flashStatus == flOK ? &flash : NULL);

  return status;
}

#endif


FLStatus noFormat (unsigned volNo, FormatParams FAR1 *formatParams, FLFlash *flash)
{
  return flOK;
}
