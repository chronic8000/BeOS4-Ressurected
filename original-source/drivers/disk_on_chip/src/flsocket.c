
/*
 * $Log:   V:/Flite/archives/FLite/src/FLSOCKET.C_V  $
 * 
 *    Rev 1.27   Jan 13 2000 18:22:14   vadimk
 * TrueFFS OSAK 4.1
 * 
 *    Rev 1.26   06 Jan 1999 14:56:52   marina
 * include fatlite.h was removed because of separation.
 * 
 *    Rev 1.25   23 Nov 1998 12:16:54   amirban
 * Correct definition of updateSocketParameters
 * 
 *    Rev 1.24   03 Sep 1998 13:58:58   ANDRY
 * better DEBUG_PRINT
 *
 *    Rev 1.23   26 Nov 1997 18:22:50   danig
 * updateSocketParameters
 *
 *    Rev 1.22   11 Nov 1997 17:34:58   danig
 * flUpdateSocketParams
 *
 *    Rev 1.21   29 Sep 1997 14:22:36   danig
 * flExitSocket
 *
 *    Rev 1.21   28 Sep 1997 18:24:02   danig
 * flExitSocket
 *
 *    Rev 1.20   10 Sep 1997 16:27:54   danig
 * Got rid of generic names
 *
 *    Rev 1.19   04 Sep 1997 16:23:36   danig
 * Debug messages
 *
 *    Rev 1.18   28 Aug 1997 17:49:24   danig
 * Buffer & remapped per socket
 *
 *    Rev 1.17   17 Aug 1997 15:10:48   danig
 * Turn off the card change indicator in flResetCardChanged
 *
 *    Rev 1.16   27 Jul 1997 11:59:38   amirban
 * Socket.volNo initialization
 *
 *    Rev 1.15   27 Jul 1997 10:12:12   amirban
 * physicalToPointer and FAR -> FAR0
 *
 *    Rev 1.14   20 Jul 1997 17:15:38   amirban
 * No watchDogTimer
 *
 *    Rev 1.13   07 Jul 1997 15:21:46   amirban
 * Ver 2.0
 *
 *    Rev 1.12   08 Jun 1997 17:03:22   amirban
 * power on callback
 *
 *    Rev 1.11   15 Apr 1997 19:13:48   danig
 * Added SOCKET_12_VOLTS before a call to vppOff.
 *
 *    Rev 1.10   25 Nov 1996 17:23:38   danig
 * Changed doNotDisturb from Boolean to counter
 *
 *    Rev 1.9   20 Nov 1996 16:24:34   danig
 * added remapped to socketSetBusy.
 *
 *    Rev 1.8   08 Oct 1996 12:17:52   amirban
 * Defined remapped
 *
 *    Rev 1.7   18 Aug 1996 13:48:38   amirban
 * Comments
 *
 *    Rev 1.6   15 Aug 1996 11:51:16   amirban
 * Wait state mistake
 *
 *    Rev 1.5   31 Jul 1996 14:31:14   amirban
 * Background stuff, and 5V Vpp
 *
 *    Rev 1.4   09 Jul 1996 14:36:52   amirban
 * CPU_i386 define
 *
 *    Rev 1.3   01 Jul 1996 15:41:44   amirban
 * init for all sockets
 *
 *    Rev 1.2   09 Jun 1996 18:16:30   amirban
 * Correction of socketSetBusy
 *
 *    Rev 1.1   03 Jun 1996 16:20:26   amirban
 * Typo fix
 *
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
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

unsigned noOfDrives = 0;	/* No. of drives actually registered */

static FLSocket vols[DRIVES];

#ifndef SINGLE_BUFFER
#ifdef MALLOC
static FLBuffer *volBuffers[DRIVES];
#else
static FLBuffer volBuffers[DRIVES];
#endif
#endif

/*----------------------------------------------------------------------*/
/*      	          f l S o c k e t N o O f 			*/
/*									*/
/* Gets the volume no. connected to a socket				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/* 	volume no. of socket						*/
/*----------------------------------------------------------------------*/

unsigned flSocketNoOf(const FLSocket vol)
{
  return vol.volNo;
}


/*----------------------------------------------------------------------*/
/*      	          f l S o c k e t O f	   			*/
/*									*/
/* Gets the socket connected to a volume no.				*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no. for which to get socket		*/
/*                                                                      */
/* Returns:                                                             */
/* 	socket of volume no.						*/
/*----------------------------------------------------------------------*/

FLSocket *flSocketOf(unsigned volNo)
{
  return &vols[volNo];
}

#ifndef SINGLE_BUFFER
/*----------------------------------------------------------------------*/
/*      	          f l B u f f e r O f	   			*/
/*									*/
/* Gets the buffer connected to a volume no.				*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no. for which to get socket		*/
/*                                                                      */
/* Returns:                                                             */
/* 	buffer of volume no.						*/
/*----------------------------------------------------------------------*/

FLBuffer *flBufferOf(unsigned volNo)
{
#ifdef MALLOC
  return volBuffers[volNo];
#else
  return &volBuffers[volNo];
#endif
}
#endif /* SINGLE_BUFFER */

/*----------------------------------------------------------------------*/
/*      	        f l W r i t e P r o t e c t e d			*/
/*									*/
/* Returns the write-protect state of the media				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	0 = not write-protected, other = write-protected		*/
/*----------------------------------------------------------------------*/

FLBoolean flWriteProtected(FLSocket vol)
{
  return vol.writeProtected(&vol);
}


#ifndef FIXED_MEDIA

/*----------------------------------------------------------------------*/
/*      	      f l R e s e t C a r d C h a n g e d		*/
/*									*/
/* Acknowledges a media-change condition and turns off the condition.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flResetCardChanged(FLSocket vol)
{
  if (vol.getAndClearCardChangeIndicator)
      vol.getAndClearCardChangeIndicator(&vol);  /* turn off the indicator */

  vol.cardChanged = FALSE;
}


/*----------------------------------------------------------------------*/
/*      	          f l M e d i a C h e c k			*/
/*									*/
/* Checks the presence and change status of the media			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/* 	flOK		->	Media present and not changed		*/
/*	driveNotReady   ->	Media not present			*/
/*	diskChange	->	Media present but changed		*/
/*----------------------------------------------------------------------*/

FLStatus flMediaCheck(FLSocket vol)
{
  if (!vol.cardDetected(&vol)) {
    vol.cardChanged = TRUE;
    return flDriveNotReady;
  }

  if (vol.getAndClearCardChangeIndicator &&
      vol.getAndClearCardChangeIndicator(&vol))
    vol.cardChanged = TRUE;

  return vol.cardChanged ? flDiskChange : flOK;
}

#endif

/*----------------------------------------------------------------------*/
/*      	         f l I n i t S o c k e t s		        */
/*									*/
/* First call to this module: Initializes the controller and all sockets*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flInitSockets(void)
{
  unsigned volNo;
  FLSocket vol = vols;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++) {
    flSetWindowSpeed(&vol, 250);
    flSetWindowBusWidth(&vol, 16);
    flSetWindowSize(&vol, 2);		/* make it 8 KBytes */

    vol.cardChanged = FALSE;

  #ifndef SINGLE_BUFFER
  #ifdef MALLOC
    /* allocate buffer for this socket */
    volBuffers[volNo] = (FLBuffer *)MALLOC(sizeof(FLBuffer));
    if (volBuffers[volNo] == NULL) {
      DEBUG_PRINT(("Debug: failed allocating sector buffer.\n"));
      return flNotEnoughMemory;
    }
  #endif
  #endif

    checkStatus(vol.initSocket(&vol));

  #ifdef SOCKET_12_VOLTS
    vol.VppOff(&vol);
    vol.VppState = PowerOff;
    vol.VppUsers = 0;
  #endif
    vol.VccOff(&vol);
    vol.VccState = PowerOff;
    vol.VccUsers = 0;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	     f l G e t M a p p i n g C o n t e x t		*/
/*									*/
/* Returns the currently mapped window page (in 4KB units)		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	unsigned int	: Current mapped page no.			*/
/*----------------------------------------------------------------------*/

unsigned flGetMappingContext(FLSocket vol)
{
  return vol.window.currentPage;
}


/*----------------------------------------------------------------------*/
/*      	                f l M a p			        */
/*									*/
/* Maps the window to a specified card address and returns a pointer to */
/* that location (some offset within the window).			*/
/*									*/
/* NOTE: Addresses over 128M are attribute memory. On PCMCIA adapters,	*/
/* subtract 128M from the address and map to attribute memory.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Byte-address on card. NOT necessarily on a 	*/
/*			  full-window boundary.				*/
/*			  If above 128MB, address is in attribute space.*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to a location within the window mapping the address.	*/
/*----------------------------------------------------------------------*/

void FAR0 *flMap(FLSocket vol, CardAddress address)
{
  unsigned pageToMap;

  if (vol.window.currentPage == UNDEFINED_MAPPING)
    vol.setWindow(&vol);
  pageToMap = (unsigned) ((address & -vol.window.size) >> 12);

  if (vol.window.currentPage != pageToMap) {
    vol.setMappingContext(&vol, pageToMap);
    vol.window.currentPage = pageToMap;
    vol.remapped = TRUE;	/* indicate remapping done */
  }

  return addToFarPointer(vol.window.base,address & (vol.window.size - 1));
}


/*----------------------------------------------------------------------*/
/*      	      f l S e t W i n d o w B u s W i d t h		*/
/*									*/
/* Requests to set the window bus width to 8 or 16 bits.		*/
/* Whether the request is filled depends on hardware capabilities.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      width		: Requested bus width				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowBusWidth(FLSocket vol, unsigned width)
{
  vol.window.busWidth = width;
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	     f l S e t W i n d o w S p e e d			*/
/*									*/
/* Requests to set the window speed to a specified value.		*/
/* The window speed is set to a speed equal or slower than requested,	*/
/* if possible in hardware.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      nsec		: Requested window speed in nanosec.		*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSpeed(FLSocket vol, unsigned nsec)
{
  vol.window.speed = nsec;
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	      f l S e t W i n d o w S i z e			*/
/*									*/
/* Requests to set the window size to a specified value (power of 2).	*/
/* The window size is set to a size equal or greater than requested,	*/
/* if possible in hardware.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      sizeIn4KBUnits : Requested window size in 4 KByte units.	*/
/*			 MUST be a power of 2.				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSize(FLSocket vol, unsigned sizeIn4KBunits)
{
  vol.window.size = (long) (sizeIn4KBunits) * 0x1000L;
	/* Size may not be possible. Actual size will be set by 'setWindow' */
  vol.window.base = physicalToPointer((long) vol.window.baseAddress << 12,
				      vol.window.size,vol.volNo);
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	     f l S o c k e t S e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: ON (1) = operation entry			*/
/*			  OFF(0) = operation exit			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSocketSetBusy(FLSocket vol, FLBoolean state)
{
  if (state == OFF) {
#if POLLING_INTERVAL == 0
    /* If we are not polling, activate the interval routine before exit */
    flIntervalRoutine(&vol);
#endif
  }
  else {
    vol.window.currentPage = UNDEFINED_MAPPING;	/* don't assume mapping still valid */
    vol.remapped = TRUE;
  }
}


/*----------------------------------------------------------------------*/
/*      	            f l N e e d V c c				*/
/*									*/
/* Turns on Vcc, if not on already					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

void flNeedVcc(FLSocket vol)
{
  vol.VccUsers++;
  if (vol.VccState == PowerOff) {
    vol.VccOn(&vol);
    if (vol.powerOnCallback)
      vol.powerOnCallback(vol.flash);
  }
  vol.VccState = PowerOn;
}


/*----------------------------------------------------------------------*/
/*      	         f l D o n t N e e d V c c			*/
/*									*/
/* Notifies that Vcc is no longer needed, allowing it to be turned off. */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVcc(FLSocket vol)
{
  if (vol.VccUsers > 0)
    vol.VccUsers--;
}

#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*      	            f l N e e d V p p				*/
/*									*/
/* Turns on Vpp, if not on already					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flNeedVpp(FLSocket vol)
{
  vol.VppUsers++;
  if (vol.VppState == PowerOff)
    checkStatus(vol.VppOn(&vol));
  vol.VppState = PowerOn;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	         f l D o n t N e e d V p p				*/
/*									*/
/* Notifies that Vpp is no longer needed, allowing it to be turned off. */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVpp(FLSocket vol)
{
  if (vol.VppUsers > 0)
    vol.VppUsers--;
}

#endif	/* SOCKET_12_VOLTS */


/*----------------------------------------------------------------------*/
/*      	    f l S e t P o w e r O n C a l l b a c k			*/
/*									*/
/* Sets a routine address to call when powering on the socket.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      routine		: Routine to call when turning on power		*/
/*	flash		: Flash object of routine			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetPowerOnCallback(FLSocket vol, void (*routine)(void *flash), void *flash)
{
  vol.powerOnCallback = routine;
  vol.flash = flash;
}



/*----------------------------------------------------------------------*/
/*      	      f l I n t e r v a l R o u t i n e			*/
/*									*/
/* Performs periodic socket actions: Checks card presence, and handles  */
/* the Vcc & Vpp turn off mechanisms.					*/
/*                                                                      */
/* The routine may be called from the interval timer or sunchronously.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flIntervalRoutine(FLSocket vol)
{
#ifndef FIXED_MEDIA
  if (vol.getAndClearCardChangeIndicator == NULL &&
      !vol.cardChanged)
    if (!vol.cardDetected(&vol))	/* Check that the card is still there */
      vol.cardChanged = TRUE;
#endif

  if (vol.VppUsers == 0) {
    if (vol.VppState == PowerOn)
      vol.VppState = PowerGoingOff;
    else if (vol.VppState == PowerGoingOff) {
      vol.VppState = PowerOff;
#ifdef SOCKET_12_VOLTS
      vol.VppOff(&vol);
#endif
    }
    if (vol.VccUsers == 0) {
      if (vol.VccState == PowerOn)
	vol.VccState = PowerGoingOff;
      else if (vol.VccState == PowerGoingOff) {
	vol.VccState = PowerOff;
	vol.VccOff(&vol);
      }
    }
  }
}

/*----------------------------------------------------------------------*/
/*		 u p d a t e S o c k e t P a r a m e t e r s		*/
/*									*/
/* Pass socket parameters to the socket interface layer.		*/
/* This function should be called after the socket parameters (like	*/
/* size and base) are known. If these parameters are known at 		*/
/* registration time then there is no need to use this function, and	*/
/* the parameters can be passed to the registration routine.		*/
/* The structure passed in irData is specific for each socket interface.*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus 	: 0 on success, otherwise failed.		*/
/*----------------------------------------------------------------------*/
FLStatus updateSocketParameters(FLSocket vol, void FAR1 *params)
{
  if (vol.updateSocketParams)
    vol.updateSocketParams(&vol, params);

  return flOK;
}


#ifdef EXIT
/*----------------------------------------------------------------------*/
/*      	      f l E x i t S o c k e t				*/
/*									*/
/* Reset the socket and free resources that were allocated for this	*/
/* socket.								*/
/* This function is called when FLite exits.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flExitSocket(FLSocket vol)
{
  flMap(&vol, 0);                           /* reset the mapping register */
  flDontNeedVcc(&vol);
  flSocketSetBusy(&vol,OFF);
  vol.freeSocket(&vol);                     /* free allocated resources */
#ifndef SINGLE_BUFFER
#ifdef MALLOC
  FREE(volBuffers[vol.volNo]);
#endif
#endif
}
#endif /* EXIT */


