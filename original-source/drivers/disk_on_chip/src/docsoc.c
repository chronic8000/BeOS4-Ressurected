/*
 * $Log:   V:/Flite/archives/FLite/src/DOCSOC.C_V  $
 * 
 *    Rev 1.18   Jan 13 2000 18:05:24   vadimk
 * TrueFFS OSAK 4.1
 * 
 *    Rev 1.17   May 25 1999 16:54:30   marinak
 * flregisterDOCSOC bug fixed
 * 
 *    Rev 1.16   08 Apr 1999 11:52:18   marina
 * change include nfdc2148.h to include diskonc.h
 * 
 *    Rev 1.15   25 Feb 1999 16:14:18   marina
 * setWindowSize was passed form MTD
 * 
 *    Rev 1.14   23 Feb 1999 20:27:38   marina
 * include nfdc2148.h was added
 * 
 *    Rev 1.13   20 May 1998  7:38:54   Yair
 * Add several DiskOnChip processing
 * 
 *    Rev 1.12   16 Dec 1997 11:38:36   ANDRY
 * host address range to search for DiskOnChip window is a parameter of the
 * registration routine
 *
 *    Rev 1.11   11 Nov 1997 17:29:18   danig
 * updateSocketParams
 *
 *    Rev 1.10   28 Sep 1997 18:25:36   danig
 * freeSocket
 *
 *    Rev 1.9   10 Sep 1997 16:20:56   danig
 * Got rid of generic names
 *
 *    Rev 1.8   31 Aug 1997 15:03:00   danig
 * Registration routine return status
 *
 *    Rev 1.7   14 Aug 1997 15:44:08   danig
 * Register only one DOC
 *
 *    Rev 1.6   27 Jul 1997 14:08:32   danig
 * no fltimer.h, windowBaseAddress receives driveNo
 *
 *    Rev 1.5   21 Jul 1997 19:09:50   danig
 * Compile cardDetected regardless of FIXED_MEDIA
 *
 *    Rev 1.4   15 Jul 1997 16:18:22   danig
 * Ver 2.0
 *
 *    Rev 1.3   22 Jun 1997 14:11:32   danig
 * Power on callback
 *
 *    Rev 1.2   20 May 1997 13:19:02   danig
 * Changes for DOC2000, remapped and doNotDisturb
 *
 *    Rev 1.1   27 Feb 1997 17:48:36   Sasha
 * Dmitry's changes for proper rebooting
 *
 *    Rev 1.0	27 Feb 1997 13:29:04   amirban
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


#include "flsocket.h"
#include "diskonc.h"

/************************************************************************/
/*									*/
/*		Beginning of controller-private code			*/
/*									*/
/* The code and definitions in this section are specific to the 	*/
/* example 82365SL implementation, and are not relevant to a different	*/
/* implementation.							*/
/*									*/
/* You should code here any private definitions and helper routines you */
/* may need.								*/
/*									*/
/************************************************************************/


/************************************************************************/
/*									*/
/*		Beginning of controller-customizable code		*/
/*									*/
/* The function prototypes and interfaces in this section are standard	*/
/* and are used in this form by the non-customizable code. However, the */
/* function implementations are specific to the 82365SL controller.	*/
/*									*/
/* You should replace the function bodies here with the implementation	*/
/* that is appropriate for your controller.				*/
/*									*/
/* All the functions in this section have no parameters. This is	*/
/* because the parameters needed for an operation may be themselves	*/
/* depend on the controller. Instead, you should use the value in the	*/
/* 'vol' structure as parameters.                                       */
/* If you need socket-state variables specific to your implementation,	*/
/* it is recommended to add them to the 'vol' structure rather than     */
/* define them as separate static variables.				*/
/*									*/
/************************************************************************/


/*----------------------------------------------------------------------*/
/*			  c a r d D e t e c t e d			*/
/*									*/
/* Detect if a card is present (inserted)				*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	0 = card not present, other = card present			*/
/*----------------------------------------------------------------------*/

static FLBoolean cardDetected(FLSocket vol)
{
  return TRUE;
}


/*----------------------------------------------------------------------*/
/*			       V c c O n				*/
/*									*/
/* Turns on Vcc (3.3/5 Volts). Vcc must be known to be good on exit.	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void VccOn(FLSocket vol)
{
}


/*----------------------------------------------------------------------*/
/*			     V c c O f f				*/
/*									*/
/* Turns off Vcc.							*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void VccOff(FLSocket vol)
{
}


#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*			       V p p O n				*/
/*									*/
/* Turns on Vpp (12 Volts. Vpp must be known to be good on exit.	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus		: 0 on success, failed otherwise	*/
/*----------------------------------------------------------------------*/

static FLStatus VppOn(FLSocket vol)
{
  return flOK;
}


/*----------------------------------------------------------------------*/
/*			     V p p O f f				*/
/*									*/
/* Turns off Vpp.							*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void VppOff(FLSocket vol)
{
}

#endif	/* SOCKET_12_VOLTS */


/*----------------------------------------------------------------------*/
/*			  i n i t S o c k e t				*/
/*									*/
/* Perform all necessary initializations of the socket or controller	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus		: 0 on success, failed otherwise	*/
/*----------------------------------------------------------------------*/

static FLStatus initSocket(FLSocket vol)
{
  return flOK;
}


/*----------------------------------------------------------------------*/
/*			    s e t W i n d o w				*/
/*									*/
/* Sets in hardware all current window parameters: Base address, size,	*/
/* speed and bus width. 						*/
/* The requested settings are given in the 'vol.window' structure.      */
/*									*/
/* If it is not possible to set the window size requested in		*/
/* 'vol.window.size', the window size should be set to a larger value,  */
/* if possible. In any case, 'vol.window.size' should contain the       */
/* actual window size (in 4 KB units) on exit.				*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void setWindow(FLSocket vol)
{
}


/*----------------------------------------------------------------------*/
/*      	   s e t M a p p i n g C o n t e x t			*/
/*									*/
/* Sets the window mapping register to a card address.			*/
/*									*/
/* The window should be set to the value of 'vol.window.currentPage',	*/
/* which is the card address divided by 4 KB. An address over 128KB,	*/
/* (page over 32K) specifies an attribute-space address.		*/
/*									*/
/* The page to map is guaranteed to be on a full window-size boundary.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	page		: page to map					*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setMappingContext(FLSocket vol, unsigned page)
{
}


/*----------------------------------------------------------------------*/
/*	 g e t A n d C l e a r C a r d C h a n g e I n d i c a t o r	*/
/*									*/
/* Returns the hardware card-change indicator and clears it if set.	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	0 = Card not changed, other = card changed			*/
/*----------------------------------------------------------------------*/

static FLBoolean getAndClearCardChangeIndicator(FLSocket vol)
{
  /* Note: On the 365, the indicator is turned off by the act of reading */
  return FALSE;
}



/*----------------------------------------------------------------------*/
/*			  w r i t e P r o t e c t e d			*/
/*									*/
/* Returns the write-protect state of the media 			*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	0 = not write-protected, other = write-protected		*/
/*----------------------------------------------------------------------*/

static FLBoolean writeProtected(FLSocket vol)
{
  return FALSE;
}

#ifdef EXIT
/*----------------------------------------------------------------------*/
/*      	          f r e e S o c k e t				*/
/*									*/
/* Free resources that were allocated for this socket.			*/
/* This function is called when FLite exits.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void freeSocket(FLSocket vol)
{
}
#endif  /* EXIT */

/*----------------------------------------------------------------------*/
/*             		f l R e g i s t e r D O C S O C			*/
/*									*/
/* Installs routines for DiskOnChip.					*/
/*									*/
/* Parameters:                                                          */
/*      lowAddress, 	                                                */
/*      lowAddress      : host memory range to search for DiskOnChip    */
/*                        2000 memory window                            */
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterDOCSOC(unsigned long lowAddress, unsigned long highAddress)
{
  int serialNo;

  if( noOfDrives >= DRIVES )
    return flTooManyComponents;

  for(serialNo=0;( noOfDrives < DRIVES );serialNo++,noOfDrives++) {
    FLSocket vol = flSocketOf(noOfDrives);

    vol.volNo = noOfDrives;
    /* call DiskOnChip 2000 MTD's routine to search for memory window */
    flSetWindowSize(&vol, 2);	/* 4 KBytes */
    vol.window.baseAddress =
	flDocWindowBaseAddress(vol.volNo, lowAddress, highAddress);
    if (vol.window.baseAddress == 0)	/* DiskOnChip not detected */
      break;
    vol.cardDetected = cardDetected;
    vol.VccOn = VccOn;
    vol.VccOff = VccOff;
#ifdef SOCKET_12_VOLTS
    vol.VppOn = VppOn;
    vol.VppOff = VppOff;
#endif
    vol.initSocket = initSocket;
    vol.setWindow = setWindow;
    vol.setMappingContext = setMappingContext;
    vol.getAndClearCardChangeIndicator = getAndClearCardChangeIndicator;
    vol.writeProtected = writeProtected;
    vol.updateSocketParams = NULL;
#ifdef EXIT
    vol.freeSocket = freeSocket;
#endif
  }
  if( serialNo == 0 )
    return flAdapterNotFound;

  return flOK;
}


