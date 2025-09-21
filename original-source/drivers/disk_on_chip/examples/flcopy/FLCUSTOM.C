
/*
 * $Log:   V:/Flite/archives/OSAK/custom/FLCUSTOM.C_V  $
 * 
 *    Rev 1.1   Jun 06 1999 20:28:52   marinak
 * return status in flRegisterComponents
 * 
 *    Rev 1.0   May 23 1999 13:12:16   marinak
 * Initial revision.
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

FLStatus flRegisterComponents(void)
{
  /* Registering socket interface */
  #define LOW_ADDR	0xd0000l
  #define HIGH_ADDR	0xe0000l

  checkStatus(flRegisterDOCSOC(LOW_ADDR, HIGH_ADDR));	/* Register DiskOnChip socket interface */

  /* Registering MTD */
  checkStatus(flRegisterDOC2000());	/* Register diskonc.c MTD */

  /* Registering translation layer */
  checkStatus(flRegisterNFTL());	/* Register nftllite.c */

  return flOK;
}


