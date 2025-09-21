
/*
 * $Log:   V:/Flite/archives/OSAK/util/dformat/FLCUSTOM.C_V  $
 * 
 *    Rev 1.1   Nov 24 1999 18:17:50   vadimk
 * Return status in flRegisterComponents
 * 
 * 
 *    Rev 1.0   May 23 1999 14:56:18   marinak
 * Initial revision.
 * 
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/

#include "stdcomp.h"

/*----------------------------------------------------------------------*/
/*      	  f l R e g i s t e r C o m p o n e n t s		*/
/*									*/
/* Register socket, MTD and translation layer components for use	*/
/*									*/
/* This function is called by FLite once only, at initialization of the */
/* FLite system.							*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/*----------------------------------------------------------------------*/

unsigned long window = 0L;

FLStatus flRegisterComponents(void)
{
  checkStatus(flRegisterDOCSOC(window, window));	/* Register DiskOnChip socket interface */

  /* Registering MTD */
  checkStatus(flRegisterDOC2000());	/* Register diskonc.c MTD */

  /* Registering translation layer */
  checkStatus(flRegisterNFTL());	/* Register nftllite.c */

  return flOK;

}


