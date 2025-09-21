/*
 * $Log:   V:/Flite/archives/FLite/src/diskonc.c_v  $
 * 
 *    Rev 1.56   Apr 09 2000 11:50:04   vadimk
 * Perform  waitforready if waitrforreadywithyeldCPU failed .
 *
 *    Rev 1.55   Mar 21 2000 11:36:26   vadimk
 * get rid of warnings
 *
 *    Rev 1.54   16 Mar 2000 11:28:56   dimitrys
 * Insert Memory Access Macros into parentheses
 *
 *    Rev 1.53   Mar 14 2000 15:17:02   vadimk
 * Implementation of waitForReadyWithYieldCPU is changed
 *
 *    Rev 1.52   Feb 29 2000 17:32:48   vadimk
 * Function waitForReadyWithYieldCPU was added.
 *
 *    Rev 1.51   21 Feb 2000 11:36:14   dimitrys
 * Add waitForReady() call in case read operation enters
 *   into Serial Read Cycle
 *
 *    Rev 1.50   Feb 03 2000 17:51:20   vadimk
 * Remove flyieldcpu environment variable
 *
 *    Rev 1.49   Jan 23 2000 11:54:36   danig
 * Initialize vol.mediaType
 *
 *    Rev 1.48   Jan 17 2000 12:09:34   vadimk
 * "fl" prefix was added to the environment variables
 *
 *    Rev 1.47   Jan 13 2000 18:03:22   vadimk
 * TrueFFS OSAK 4.1
 *
 *    Rev 1.46   15 Nov 1999 15:18:50   dimitrys
 * Add 512 Mbit Toshiba and Samsung flash support
 *   (big address type - 4 byte address cycle),
 * Add 256 Mbit Samsung flash support
 *
 *    Rev 1.45   11 Oct 1999 13:27:56   dimitrys
 * ECC/EDC bug with Floors fixed (address was
 *    changed before 'second try' call),
 * Unproper 2 Mb Flash processing fixed.
 *
 *    Rev 1.44   10 Oct 1999 11:04:38   dimitrys
 * 256 Mbit Toshiba Flash Support,
 * Function: selectFloor - always select current floor,
 * Function: doc2Erase - reset flash before erasing
 *
 *    Rev 1.43   Jul 15 1999 13:19:46   marinak
 * Remove obsolete calls to the function writeSignals
 *
 *    Rev 1.42   14 Apr 1999 13:44:24   marina
 * NDoc2Window FAR0* was changed to NDoc2Window
 *
 *    Rev 1.41   25 Feb 1999 16:18:52   marina
 * setWindowSize was passed from MTD
 *
 *    Rev 1.40   25 Feb 1999 11:58:00   marina
 * fix include problem
 *
 *    Rev 1.39   23 Feb 1999 21:11:22   marina
 * separate customizable part
 *
 *    Rev 1.38   21 Feb 1999 13:45:44   Dmitry
 * EEprom mode writing bug fixed (alias range <= 256 bytes)
 * Floors identification bug fixed
 *
 *    Rev 1.37   14 Feb 1999 16:22:12   amirban
 * Changed flWriteFault to flDataError
 *
 *    Rev 1.36   24 Dec 1998 12:14:06   marina
 * Conditional compilation
 *
 *    Rev 1.35   23 Dec 1998 20:51:02   marina
 * millenium
 *
 *    Rev 1.33   29 Oct 1998  6:58:18   Dimitry
 * Don't swap buffer before checkAndFix function
 *
 *    Rev 1.32   27 Sep 1998  7:15:28   Dimitry
 * Add chkASICmode in doc2Read, doc2Write and doc2Erase functions
 *
 *    Rev 1.31   03 Sep 1998 13:58:56   ANDRY
 * better DEBUG_PRINT
 *
 *    Rev 1.30   10 Jun 1998 10:42:22   ANDRY
 * entire routine checkErase() is under #define VERIFY_ERASE
 *
 *    Rev 1.29   10 Jun 1998 10:40:56   ANDRY
 * bug fix under #define VERIFY_WRITE
 *
 *    Rev 1.28   20 May 1998  8:00:44   Yair
 * Add several DiskOnChip processing and 128 MBit flash processing
 *
 *    Rev 1.27   23 Feb 1998 17:08:48   Yair
 * Added casts
 *
 *    Rev 1.26   10 Feb 1998 17:47:00   DIMITRY
 * Leave CE after Command On ( new flash spec )
 *
 *    Rev 1.25   16 Dec 1997 11:39:56   ANDRY
 * host address range to search for DiskOnChip window is a parameter of the
 * registration routine of DiskOnChip socket component
 *
 *    Rev 1.24   23 Nov 1997 17:21:12   Yair
 * Get rid of warnings (with Danny)
 *
 *    Rev 1.23   23 Sep 1997 18:52:16   danig
 * Initialize junk
 *
 *    Rev 1.22   18 Sep 1997 10:12:08   danig
 * Warnings
 *
 *    Rev 1.21   10 Sep 1997 16:14:30   danig
 * Got rid of generic names
 *
 *    Rev 1.20   08 Sep 1997 18:10:48   danig
 * Fixed setAddress for big-endian
 *
 *    Rev 1.19   04 Sep 1997 19:30:40   danig
 * Debug messages
 *
 *    Rev 1.18   04 Sep 1997 14:30:58   DIMITRY
 * Add Reset in 'windowBase' function +
 * Set 0-floor after floor identification
 *
 *    Rev 1.17   04 Sep 1997 10:04:02   DIMITRY
 * ECC/EDC in hardware
 *
 *    Rev 1.16   31 Aug 1997 15:02:34   danig
 * Registration routine return status
 *
 *    Rev 1.15   28 Aug 1997 16:56:50   danig
 * Buffer and remapped per drive
 *
 *    Rev 1.14   31 Jul 1997 14:23:22   danig
 * Changed junk and zeroes to be local variables
 *
 *    Rev 1.13   28 Jul 1997 15:33:54   danig
 * Moved standard typedefs to flbase.h
 *
 *    Rev 1.12   27 Jul 1997 14:06:14   danig
 * FAR -> FAR0, physicalToPointer, no fltimer.h
 *
 *    Rev 1.11   24 Jul 1997 16:42:30   danig
 * Reset before Erase ( for Toshiba 8 Mb )
 *
 *    Rev 1.10   21 Jul 1997 18:56:22   danig
 * nandBuffer static
 *
 *    Rev 1.9   15 Jul 1997 16:17:40   danig
 * Ver 2.0
 *
 *    Rev 1.8   13 Jul 1997 18:08:04   danig
 * Initialize totalFloors & floorSize
 *
 *    Rev 1.7   30 Jun 1997 15:00:08   DIMITRY
 * Always init MTD with noOfUnits = 0.
 *
 *    Rev 1.6   22 Jun 1997 15:18:16   danig
 * No header file, moved page flags to flash.h
 *
 *    Rev 1.5   22 Jun 1997 14:13:04   danig
 * Documentation
 *
 *    Rev 1.4   18 Jun 1997 16:25:22   DIMITRY
 *
 *    Rev 1.3   17 Jun 1997 17:22:28   DIMITRY
 * Major code reorg + fast Toshiba ( AmirBan )
 *
 *    Rev 1.2   25 May 1997 19:20:24   danig
 * Changes in EDC & media size calculation in doc2Erase
 *
 *    Rev 1.1   25 May 1997 14:47:06   danig
 * Changed writeOneSector for Toshiba 2M, and fixed noOfBlocks bug
 *
 *    Rev 1.0   20 May 1997 13:11:50   danig
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/


#include "reedsol.h"
#include "docsys.h"


unsigned long currentAddress = 0L;

static NFDC21Vars mtdVars[DRIVES];

      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*   Miscellaneous routines   */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/

/* Yield CPU time in msecs */
#define YIELD_CPU 10
/* maximum waiting time in msecs */
#define MAX_WAIT  30

#ifndef NO_EDC_MODE
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*            EDC control     */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/




/*----------------------------------------------------------------------*/
/*		          e c c O N r e a d				*/
/*									*/
/* Enable ECC in read mode and reset it.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void eccONread (FLFlash vol)
{
  flWrite8bitReg(&vol,NECCconfig,ECC_RESET);
  flWrite8bitReg(&vol,NECCconfig,ECC_EN);
}

/*----------------------------------------------------------------------*/
/*		          e c c O n w r i t e				*/
/*									*/
/* Enable ECC in write mode and reset it.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/
static void  eccONwrite (FLFlash vol)
{
  flWrite8bitReg(&vol,NECCconfig,ECC_RESET);
  flWrite8bitReg(&vol,NECCconfig,(ECC_RW | ECC_EN));
}

#endif
/*----------------------------------------------------------------------*/
/*		          e c c O F F					*/
/*									*/
/* Disable ECC.								*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void eccOFF (FLFlash vol)
{
  flWrite8bitReg(&vol,NECCconfig,ECC_RESERVED);
}

#ifndef NO_EDC_MODE
/*----------------------------------------------------------------------*/
/*		          e c c E r r o r				*/
/*									*/
/* Check for EDC error.							*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/
static FLBoolean  eccError (FLFlash vol)
{
  register int i;
  volatile Reg8bitType junk = 0;
  Reg8bitType ret;

  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    for(i=0;( i < 2 ); i++)
      junk += flRead8bitReg(&vol,NECCconfig);
    ret = flRead8bitReg(&vol,NECCconfig);
  }
  else {
    for(i=0;( i < 2 ); i++)
      junk += flRead8bitReg(&vol,NECCstatus);
    ret = flRead8bitReg(&vol,NECCstatus);
  }
  ret &= ECC_ERROR;
  return ((FLBoolean)ret);

}

#endif

	    /*컴컴컴컴컴컴컴컴컴컴�...*/
            /*    Auxiliary methods   */
	    /*컴컴컴컴컴컴컴컴컴컴�...*/

/*----------------------------------------------------------------------*/
/*		          m a k e C o m m a n d				*/
/*									*/
/* Set Page Pointer to Area A, B or C in page.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	cmd	: receives command relevant to area			*/
/*	addr	: receives the address to the right area.		*/
/*	modes	: mode of operation (EXTRA ...)				*/
/*                                                                      */
/*----------------------------------------------------------------------*/
static void  makeCommand ( FLFlash vol, PointerOp *cmd, CardAddress *addr, int modes )
{
  unsigned long offset;

  if ( !(vol.flags & BIG_PAGE) ) {           /* 2 Mb components */
    if( modes & EXTRA ) {
      offset = (*addr) & (SECTOR_SIZE - 1);
      *cmd = AREA_C;
      if( offset < EXTRA_LEN )         /* First half of extra area  */
        *addr += 0x100;                /* ... assigned to 2nd page  */
      else                             /* Second half of extra area */
        *addr -= EXTRA_LEN;            /* ... assigned to 1st page  */
    }
    else
      *cmd = AREA_A;
  }
  else {                                           /* 4 Mb components */
    offset = (unsigned short)(*addr) & NFDC21thisVars->pageMask; /* offset within device Page */
    *addr -= offset;                             /* align at device Page */

    if(modes & EXTRA)
      offset += SECTOR_SIZE;

    if( offset < NFDC21thisVars->pageAreaSize )  /* starting in area A */
      *cmd = AREA_A;
    else if( offset < NFDC21thisVars->pageSize ) /* starting in area B */
      *cmd = AREA_B;
    else                                   /* got into area C    */
      *cmd = AREA_C;

    offset &= (NFDC21thisVars->pageAreaSize - 1); /* offset within area of device Page */
    *addr += offset;
  }
}

/*----------------------------------------------------------------------*/
/*		          b u s y					*/
/*									*/
/* Check if the selected flash device is ready.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	Zero is ready.							*/
/*									*/
/*----------------------------------------------------------------------*/
static FLBoolean busy (FLFlash vol)
{
  register int i;
  Reg8bitType stat;
  volatile Reg8bitType junk = 0;
  Reg8bitType ret;
    /* before polling for BUSY status perform 4 read operations from
       CDSN_control_reg */

  for(i=0;( i < 4 ); i++ )
    junk += flRead8bitReg(&vol,NNOPreg);

    /* read BUSY status */

  stat = flRead8bitReg(&vol,Nsignals);

    /* after BUSY status is obtained perform 2 read operations from
       CDSN_control_reg */

  for(i=0;( i < 2 ); i++ )
    junk += flRead8bitReg(&vol,NNOPreg);
  ret = (!(stat & (Reg8bitType)RB));
  return ((FLBoolean)ret);
}

/*----------------------------------------------------------------------*/
/*		          w a i t F o r R e a d y			*/
/*									*/
/* Wait until flash device is ready or timeout.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* Returns:								*/
/*	FALSE if timeout error, otherwise TRUE.				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean  waitForReady (FLFlash vol)
{
  int i;
  for(i=0;( i < BUSY_DELAY ); i++)
  {
    if( busy(&vol) )
    {
      continue;
    }



    return( TRUE );                     /* ready at last.. */
  }

  DEBUG_PRINT(("Debug: timeout error in NFDC 2148.\n"));
  return( FALSE );
}


/*----------------------------------------------------------------------*/
/*              w a i t F o r R e a d y W i t h Y i e l d C P U         */
/*                                                                      */
/* Wait until flash device is ready or timeout.                         */
/* The function yields CPU while it waits till flash is ready           */
/*                                                                      */
/* Parameters:                                                          */
/*  vol   : Pointer identifying drive                                   */
/* Returns:                                                             */
/*  FALSE if timeout error, otherwise TRUE.                             */
/*                                                                      */
/*----------------------------------------------------------------------*/


static FLBoolean  waitForReadyWithYieldCPU (FLFlash vol, int millisecToSleep)
{
   int i;

   for (i=0;  i < (millisecToSleep / YIELD_CPU); i++) {
       flsleep(YIELD_CPU);
       if( busy(&vol) )
             continue;
       return( TRUE );                     /* ready at last.. */
   }

   DEBUG_PRINT(("Debug: timeout error in NFDC 2148.\n"));
   return( FALSE );
}




/*----------------------------------------------------------------------*/
/*		          w r i t e S i g n a l s			*/
/*									*/
/* Write to CDSN_control_reg.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	val	: Value to write to register				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  writeSignals (FLFlash vol, Reg8bitType val)
{
  register int i;
  volatile Reg8bitType junk = 0;

  flWrite8bitReg(&vol,Nsignals,val);

  /* after writing to CDSN_control perform 2 reads from there */

  for(i = 0;( i < 2 ); i++ )
    junk += flRead8bitReg(&vol,NNOPreg);
}

/*----------------------------------------------------------------------*/
/*		          s e l e c t C h i p				*/
/*									*/
/* Write to deviceSelector register.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	dev	: Chip to select.					*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  selectChip (FLFlash vol, Reg8bitType dev)
{
  flWrite8bitReg(&vol,NdeviceSelector,dev);
}

/*----------------------------------------------------------------------*/
/*		          c h k A S I C m o d e				*/
/*									*/
/* Check mode of ASIC and if RESET set to NORMAL. 			*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void chkASICmode (FLFlash vol)
{
  if( flRead8bitReg(&vol,NDOCstatus) == ASIC_CHECK_RESET ) {
    flWrite8bitReg(&vol,NDOCcontrol,ASIC_NORMAL_MODE);
    flWrite8bitReg(&vol,NDOCcontrol,ASIC_NORMAL_MODE);
    NFDC21thisVars->currentFloor = 0;
  }
}

/*----------------------------------------------------------------------*/
/*		          s e t A S I C m o d e				*/
/*									*/
/* Set mode of ASIC.							*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	mode	: mode to set.						*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setASICmode (FLFlash vol, Reg8bitType mode)
{
  NDOC2window p = (NDOC2window)flMap(vol.socket, 0);
  flPreInitWrite8bitReg(vol.socket->volNo,p,NDOCcontrol,mode);
  flPreInitWrite8bitReg(vol.socket->volNo,p,NDOCcontrol,mode);
}

static FLBoolean checkToggle(unsigned driveNo, NDOC2window memWinPtr)
{
  volatile Reg8bitType toggle1;
  volatile Reg8bitType toggle2;

  if(flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) == CHIP_ID_MDOC ) {
    toggle1 = flPreInitRead8bitReg(driveNo,memWinPtr,NECCconfig);
    toggle2 = toggle1 ^ flPreInitRead8bitReg(driveNo,memWinPtr,NECCconfig);
  }
  else {
    toggle1 = flPreInitRead8bitReg(driveNo,memWinPtr,NECCstatus);
    toggle2 = toggle1 ^ flPreInitRead8bitReg(driveNo,memWinPtr,NECCstatus);
  }
  if( (toggle2 & TOGGLE) == 0 )
    return TRUE;
  return FALSE;
}


FLBoolean checkWinForDOC(unsigned driveNo, NDOC2window memWinPtr)
{
  /* set ASIC to RESET MODE */
  flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_RESET_MODE);
  flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_RESET_MODE);

  flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_NORMAL_MODE);
  flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_NORMAL_MODE);

  if( (flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) !=CHIP_ID_DOC &&
       flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) != CHIP_ID_MDOC))
    return FALSE;
  if(checkToggle(driveNo,memWinPtr))
    return FALSE;

  return TRUE;
}


/*----------------------------------------------------------------------*/
/*      	       w i n d o w B a s e A d d r e s s		*/
/*									*/
/* Return the host base address of the window.				*/
/* If the window base address is programmable, this routine selects     */
/* where the base address will be programmed to.			*/
/*									*/
/* Parameters:                                                          */
/*	driveNo    :    FLite drive No (0..DRIVES-1)                 	*/
/*      lowAddress, 	                                                */
/*      lowAddress :	host memory range to search for DiskOnChip 2000 */
/*                      memory window                                   */
/*                                                                      */
/* Returns:                                                             */
/*	Host physical address of window divided by 4 KB			*/
/*                                                                      */
/* NOTE: This routine is directly called from registration routine of   */
/*       the DiskOnChip 2000 socket component (see DOCSOC.C)            */
/*                                                                      */
/*----------------------------------------------------------------------*/

unsigned flDocWindowBaseAddress(unsigned driveNo, unsigned long lowAddress,
						  unsigned long highAddress)
{
  NDOC2window memWinPtr;
  FLBoolean stopSearch = FALSE;
  volatile unsigned char deviceSearch;

  /* if memory range to search for DiskOnChip 2000 window is not specified */
  /* assume the standard x86 PC architecture where DiskOnChip 2000 appears */
  /* in a memory range reserved for BIOS expansions                        */
  if (lowAddress == 0x0L) {
    lowAddress  = START_ADR;
    highAddress = STOP_ADR;
  }

  if( currentAddress < lowAddress ) {  /* first enter into function */
    currentAddress = lowAddress;
    for( ; currentAddress <= highAddress; currentAddress += DOC_WIN) {
      /* Find Bios Expansion 55 AA signature */
      memWinPtr = (NDOC2window )physicalToPointer(currentAddress,DOC_WIN,driveNo);
      /*if( memWinPtr->IPLpart1[0] != 0x55 || memWinPtr->IPLpart1[1] != 0xAA )
        continue;*/
      /* set ASIC to RESET MODE */
      flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_RESET_MODE);
      flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_RESET_MODE);
    }
    currentAddress = lowAddress;       /* current address initialization */
  }
  for( ; currentAddress <= highAddress; currentAddress += DOC_WIN) {
    /* Find Bios Expansion 55 AA signature */
    memWinPtr = (NDOC2window)physicalToPointer(currentAddress,DOC_WIN,driveNo);
    /* set ASIC to NORMAL MODE */
    flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_NORMAL_MODE);
    flPreInitWrite8bitReg(driveNo,memWinPtr,NDOCcontrol,ASIC_NORMAL_MODE);

    if( (flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) !=CHIP_ID_DOC &&
         flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) != CHIP_ID_MDOC))
      if( stopSearch == TRUE )  /* DiskOnChip was found */
        break;
      else continue;

    if( stopSearch == FALSE ) {
      /* detect card - identify bit toggles on consequitive reads */
      if(checkToggle(driveNo,memWinPtr))
        continue;
      /* DiskOnChip found */
      if( flPreInitRead8bitReg(driveNo,memWinPtr,NchipId)) {
        flPreInitWrite8bitReg(driveNo,memWinPtr,NaliasResolution,ALIAS_RESOLUTION);
      }
      else {
        flPreInitWrite8bitReg(driveNo,memWinPtr,NdeviceSelector,ALIAS_RESOLUTION);
      }
      stopSearch = TRUE;
      lowAddress = currentAddress;   /* save DiskOnChip address */
    }
    else { /* DiskOnChip found, continue to skip aliases */
      if( (flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) != CHIP_ID_DOC) &&
          (flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) != CHIP_ID_MDOC) )
        break;
      /* detect card - identify bit toggles on consequitive reads */
      if(checkToggle(driveNo,memWinPtr))
        break;
      /* check for Alias */
      deviceSearch = (unsigned char)((flPreInitRead8bitReg(driveNo,memWinPtr,NchipId) == CHIP_ID_MDOC) ?
                flPreInitRead8bitReg(driveNo,memWinPtr,NaliasResolution) :
                flPreInitRead8bitReg(driveNo,memWinPtr,NdeviceSelector));
      if( deviceSearch != ALIAS_RESOLUTION )
        break;
    }
  }
  if( stopSearch == FALSE )  /* DiskOnChip 2000 memory window not found */
    return( 0 );

  return((unsigned)(lowAddress >> 12));
}

/*----------------------------------------------------------------------*/
/*		          s e t A d d r e s s				*/
/*									*/
/* Latch address to selected flash device.				*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	address	: address to set.					*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setAddress (FLFlash vol, CardAddress address)
{
  address &= (vol.chipSize * vol.interleaving - 1);  /* address within flash device */

  if ( vol.flags & BIG_PAGE ) {
    /*
       bits  0..7     stays as are
       bit      8     is thrown away from address
       bits 31..9 ->  bits 30..8
    */
    address = ((address >> 9) << 8)  |  ((unsigned char)address);
  }

  writeSignals (&vol, FLASH_IO | ALE | CE);

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(&vol,NslowIO,(Reg8bitType)address);
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)address);
  flWrite8bitReg(&vol,NslowIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(&vol,NslowIO,(Reg8bitType)(address >> 16));
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 16));
  if( vol.flags & BIG_ADDR ) {
    flWrite8bitReg(&vol,NslowIO,(Reg8bitType)(address >> 24));
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 24));
  }
#else
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)address);
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 16));
  if( vol.flags & BIG_ADDR )
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(address >> 24));
#endif
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,(Reg8bitType)0);

  writeSignals (&vol, ECC_IO | FLASH_IO | CE);
}

/*----------------------------------------------------------------------*/
/*		          c o m m a n d					*/
/*									*/
/* Latch command byte to selected flash device.				*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	code	: Command to set.					*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void command(FLFlash vol, Reg8bitType code)
{
  writeSignals (&vol, FLASH_IO | CLE | CE);

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(&vol,NslowIO,code);
#endif

  flWrite8bitReg(&vol,NFDC21thisIO,code);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,code);
}

/*----------------------------------------------------------------------*/
/*		          s e l e c t F l o o r				*/
/*									*/
/* Select floor (0 .. totalFloors-1).					*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	address	: Select floor for this address.			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void selectFloor (FLFlash vol, CardAddress *address)
{
  if( NFDC21thisVars->totalFloors > 1 ) {
    unsigned char floorToUse = (unsigned char)((*address) / NFDC21thisVars->floorSize);

    NFDC21thisVars->currentFloor = floorToUse;
    flWrite8bitReg(&vol,NASICselect,floorToUse);
    *address -= (floorToUse * NFDC21thisVars->floorSize);
  }
}

/*----------------------------------------------------------------------*/
/*		          m a p W i n					*/
/*									*/
/* Map window to selected flash device.					*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	address	: Map window to this address.				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void mapWin (FLFlash vol, CardAddress *address)
{
  /* NOTE: normally both ways to obtain DOC 2000 window segment should
             return the same value. */
  NFDC21thisWin = (NDOC2window)flMap(vol.socket, 0);
  selectFloor (&vol, address);

  /* select chip within floor */
  selectChip (&vol, (Reg8bitType)((*address) / (vol.chipSize * vol.interleaving))) ;
}

/*----------------------------------------------------------------------*/
/*                        r d B u f                                     */
/*									*/
/* Auxiliary routine for Read(), read from page.			*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	buf	: Buffer to read into.					*/
/*	howmany	: Number of bytes to read.				*/
/*									*/
/*----------------------------------------------------------------------*/

static void rdBuf (FLFlash vol, unsigned char FAR1 *buf, int howmany)
{
  volatile Reg8bitType junk = 0;
  register int i;
#ifdef SLOW_IO_FLAG
  /* slow flash requires first read to be done from CDSN_Slow_IO
     and only second one from CDSN_IO - this extends read access */

  for( i = 0 ;( i < howmany ); i++ ) {
    junk = flRead8bitReg(&vol,NslowIO);
    buf[i] = (unsigned char)flRead8bitReg(&vol,NFDC21thisIO+(i & 0x01));
  }
#else
  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    junk += flRead8bitReg(&vol,NreadPipeInit);
    howmany--;
    i = MIN( howmany, MDOC_ALIAS_RANGE );
    docread( &vol,NFDC21thisIO,buf,i);
  }
  else i = 0;
  if( howmany > i )
    docread( &vol,NFDC21thisIO,buf+i,howmany-i);

  if( NFDC21thisVars->flags & MDOC_ASIC )
    buf[howmany] = flRead8bitReg(&vol,NreadLastData);
#endif
}

/*----------------------------------------------------------------------*/
/*		          w r B u f					*/
/*									*/
/* Auxiliary routine for Write(), write to page from buffer.		*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	buf	: Buffer to write from.					*/
/*	howmany	: Number of bytes to write.				*/
/*									*/
/*----------------------------------------------------------------------*/

static void  wrBuf (FLFlash vol, const unsigned char FAR1 *buf, int howmany )
{
#ifdef SLOW_IO_FLAG
  register int i;
  /* slow flash requires first write go to CDSN_Slow_IO and
     only second one to CDSN_IO - this extends write access */

  for ( i = 0 ;( i < howmany ); i++ ) {
    flWrite8bitReg(&vol,NslowIO,(Reg8bitType)buf[i]);
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)buf[i]);
  }
#else
  docwrite( &vol,(void FAR1 *)buf,howmany);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,(Reg8bitType)0);
#endif
}

/*----------------------------------------------------------------------*/
/*		          w r S e t					*/
/*									*/
/* Auxiliary routine for Write(), set page data.			*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*	ch	: Set page to this byte					*/
/*	howmany	: Number of bytes to set.				*/
/*									*/
/*----------------------------------------------------------------------*/

static void  wrSet (FLFlash vol, const Reg8bitType ch, int howmany )
{
#ifdef SLOW_IO_FLAG
    register int i;
    /* slow flash requires first write go to CDSN_Slow_IO and
       only second one to CDSN_IO - this extends write access */

    for (i = 0 ;( i < howmany ); i++ ) {
      flWrite8bitReg(&vol,NslowIO,(Reg8bitType)ch);
      flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)ch);
    }
#else
    docset(&vol,howmany,ch);
    if( NFDC21thisVars->flags & MDOC_ASIC )
      flWrite8bitReg(&vol,NwritePipeTerm,(Reg8bitType)0);
#endif
}

/*----------------------------------------------------------------------*/
/*		          r e a d S t a t u s				*/
/*									*/
/* Read status of selected flash device.				*/
/*									*/
/* Parameters:                                                          */
/*	vol	: Pointer identifying drive				*/
/*									*/
/* Returns:                                                          	*/
/*      Chip status.							*/
/*									*/
/*----------------------------------------------------------------------*/

static Reg8bitType readStatus (FLFlash vol)
{
  Reg8bitType chipStatus;
  volatile Reg8bitType junk = 0;

  flWrite8bitReg(&vol,NFDC21thisIO,READ_STATUS);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,READ_STATUS);
  writeSignals (&vol, FLASH_IO | CE | WP);

  junk += flRead8bitReg(&vol,NslowIO);

  chipStatus = flRead8bitReg(&vol,NFDC21thisIO);
  return chipStatus;
}

/*----------------------------------------------------------------------*/
/*		          r e a d C o m m a n d				*/
/*									*/
/* Issue read command.							*/
/*									*/
/* Parametes:                                                          	*/
/*	vol	: Pointer identifying drive				*/
/*      cmd	: Command to issue (according to area). 		*/
/*	addr	: address to read from.					*/
/*									*/
/*----------------------------------------------------------------------*/

static void readCommand (FLFlash vol, PointerOp  cmd, CardAddress addr)
{
  command (&vol, (Reg8bitType)cmd);  /* move flash pointer to respective area of the page */
  setAddress (&vol, addr);
  waitForReady(&vol);
}

/*----------------------------------------------------------------------*/
/*		          w r i t e C o m m a n d			*/
/*									*/
/* Issue write command.							*/
/*									*/
/* Parametes:                                                          	*/
/*	vol	: Pointer identifying drive				*/
/*      cmd	: Command to issue (according to area). 		*/
/*	addr	: address to write to.					*/
/*									*/
/*----------------------------------------------------------------------*/

static void writeCommand (FLFlash vol, PointerOp  cmd, CardAddress addr)
{
  if( vol.flags & FULL_PAGE ) {
    command (&vol, RESET_FLASH);
    waitForReady(&vol);
    if( cmd != AREA_A ) {
    #ifdef SLOW_IO_FLAG
      flWrite8bitReg(&vol,NslowIO,(unsigned char)cmd);
    #endif
      flWrite8bitReg(&vol,NFDC21thisIO,(unsigned char)cmd);
      if( NFDC21thisVars->flags & MDOC_ASIC )
        flWrite8bitReg(&vol,NwritePipeTerm,(unsigned char)cmd);
    }
  }
  else
    command (&vol, (Reg8bitType)cmd); /* move flash pointer to respective area of the page */

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(&vol,NslowIO,SERIAL_DATA_INPUT);
#endif

  flWrite8bitReg(&vol,NFDC21thisIO,SERIAL_DATA_INPUT);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,SERIAL_DATA_INPUT);

  setAddress (&vol, addr);

  waitForReady(&vol);
}

/*----------------------------------------------------------------------*/
/*		          w r i t e E x e c u t e			*/
/*									*/
/* Execute write.							*/
/*									*/
/* Parametes:                                                          	*/
/*	vol	: Pointer identifying drive				*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus      	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus writeExecute (FLFlash vol)
{
  command (&vol, SETUP_WRITE);             /* execute page program */
  waitForReady(&vol);

  if( readStatus(&vol) & DUB(FAIL) ) {
    DEBUG_PRINT(("Debug: NFDC 2148 write failed.\n"));
    return( flWriteFault );
  }

  return( flOK );
}

/*----------------------------------------------------------------------*/
/*		          w r i t e O n e S e c t o r			*/
/*									*/
/* Write data in one 512-byte block to flash.				*/
/* Assuming that EDC mode never requested on partial block writes.    	*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/* 	address	: Address of sector to write to.			*/
/*	buffer	: buffer to write from.					*/
/*	length	: number of bytes to write (up to sector size).		*/
/*	modes	: OVERWRITE, EDC flags etc.				*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus writeOneSector(FLFlash vol,
			       CardAddress address,
			       const void FAR1 *buffer,
			       int length,
			       int modes)
{
  unsigned char FAR1 *pbuffer = (unsigned char FAR1 *)buffer; /* to write from */
  FLStatus  status  = flOK;
#ifndef NO_EDC_MODE
  unsigned char syndrom[SYNDROM_BYTES];
  static unsigned char anandMark[2] = { 0x55, 0x55 };
#endif
  PointerOp cmd = AREA_A ;
  unsigned int prePad;
  int toFirstPage = 0, toSecondPage = 0;

#ifdef VERIFY_WRITE
  unsigned char readback[SECTOR_SIZE];
#endif

  if (flWriteProtected(vol.socket))
    return( flWriteProtect );

  mapWin(&vol, &address);                  /* select flash device */

  /* move flash pointer to areas A,B or C of page */
  makeCommand(&vol, &cmd, &address, modes);

  if( (vol.flags & FULL_PAGE)  &&  (cmd == AREA_B) ) {
    prePad = 2 + ((unsigned short) address & NFDC21thisVars->pageMask);
    writeCommand(&vol, AREA_A, address + NFDC21thisVars->pageAreaSize - prePad);
    wrSet(&vol, 0xFF, prePad);
  }
  else
    writeCommand(&vol, cmd, address);

#ifndef NO_EDC_MODE
  if( modes & EDC )
    eccONwrite(&vol);                /* ECC ON for write */
#endif

  if( !(vol.flags & BIG_PAGE) )             /* 2M on INLV=1 */
  {
		    /* write up to two pages separately */
    if( modes & EXTRA )
      toFirstPage = EXTRA_LEN - ((unsigned short)address & (EXTRA_LEN-1));
    else
      toFirstPage = CHIP_PAGE_SIZE - ((unsigned short)address & (CHIP_PAGE_SIZE-1));

    if(toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage;

    wrBuf(&vol, pbuffer, toFirstPage);                  /* starting page .. */

    if ( toSecondPage > 0 )
    {
      if (toFirstPage > 0)                       /* started on 1st page */
        checkStatus( writeExecute(&vol) );       /* done with 1st page */
      if( modes & EXTRA )
        address -= (CHIP_PAGE_SIZE + ((unsigned short)address & (EXTRA_LEN-1)));
      writeCommand(&vol, cmd, address + toFirstPage);
      wrBuf (&vol, pbuffer + toFirstPage, toSecondPage);  /* user data */
    }
  }
  else                                     /* 4M or 8M */
    wrBuf (&vol, pbuffer, length);               /* user data */

#ifndef NO_EDC_MODE
  if(modes & EDC)
  {
    register int i;

    writeSignals (&vol, ECC_IO | CE );             /* disable flash access */
	 /* 3 dummy zero-writes to clock the data through pipeline */
    if( NFDC21thisVars->flags & MDOC_ASIC ) {
      for( i = 0;( i < 3 ); i++ ) {
	flWrite8bitReg(&vol,NNOPreg,(Reg8bitType)0);
      }
    }
    else {
      wrSet (&vol, 0x00, 3 );
    }
    writeSignals (&vol, ECC_IO | FLASH_IO | CE );  /* enable flash access */

    docread(&vol,Nsyndrom,(void FAR1*)syndrom,SYNDROM_BYTES);
 #ifdef D2TST
    tffscpy(saveSyndromForDumping,syndrom,SYNDROM_BYTES);
 #endif
    eccOFF(&vol);                           /* ECC OFF  */

    wrBuf (&vol, (const unsigned char FAR1 *)syndrom, SYNDROM_BYTES);

    wrBuf (&vol, (const unsigned char FAR1 *)anandMark, sizeof(anandMark) );
  }
#endif

  checkStatus( writeExecute(&vol) );             /* abort if write failure */
  writeSignals(&vol, FLASH_IO | WP);

#ifdef VERIFY_WRITE

	       /* Read back after write and verify */

  if( modes & OVERWRITE )
    pbuffer = (unsigned char FAR1 *) buffer;     /* back to original data */

  readCommand (&vol, cmd, address); /* move flash pointer to areas A,B or C of page */

  if( !(vol.flags & BIG_PAGE) )
  {
    rdBuf (&vol, readback, toFirstPage);

    if(tffscmp (pbuffer, readback, toFirstPage) ) {
      DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\n"));
      return( flWriteFault );
    }

    if ( toSecondPage > 0 )
    {
      readCommand (&vol, AREA_A, address + toFirstPage);

      rdBuf (&vol, readback + toFirstPage, toSecondPage);

      if( tffscmp (pbuffer + toFirstPage, readback + toFirstPage, toSecondPage)) {
        DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\n"));
        return( flWriteFault );
      }
    }
  }
  else
  {
    rdBuf (&vol, readback, length);

    if( tffscmp (pbuffer, readback, length) ) {
      DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\n"));
      return( flWriteFault );
    }
  }
		 /* then ECC and special ANAND mark */

#ifndef NO_EDC_MODE
  if( modes & EDC )
  {
    rdBuf (&vol, readback, SYNDROM_BYTES);
    if( tffscmp (syndrom, readback, SYNDROM_BYTES) )
      return( flWriteFault );

    rdBuf (&vol, readback, sizeof(anandMark));
    if( tffscmp (anandMark, readback, sizeof(anandMark)) )
      return( flWriteFault );
  }
#endif

  writeSignals (&vol, FLASH_IO | WP);
  waitForReady(&vol);                            /* Serial Read Cycle Entry */
#endif /* VERIFY_WRITE */

  return( status );
}
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		          d o c 2 W r i t e				*/
/*									*/
/* Write some data to the flash. This routine will be registered as the	*/
/* write routine for this MTD.						*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/* 	address	: Address of sector to write to.			*/
/*	buffer	: buffer to write from.					*/
/*	length	: number of bytes to write (up to sector size).		*/
/*	modes	: OVERWRITE, EDC flags etc.				*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus doc2Write(FLFlash vol,
			  CardAddress address,
			  const void FAR1 *buffer,
			  int length,
			  int modes)
{
  char FAR1 *temp = (char FAR1 *)buffer;
  int block = ((modes & EXTRA) ? EXTRA_LEN : SECTOR_SIZE);
  int writeNow = block - ((unsigned short)address & (block - 1));
	      /* write in BLOCKs; first and last might be partial */

  chkASICmode(&vol);

  while( length > 0 )
  {
     if(writeNow > length)
       writeNow = length;
	      /* turn off EDC on partial block write */

    checkStatus( writeOneSector(&vol, address, temp, writeNow,
			 (writeNow != SECTOR_SIZE ? (modes &= ~EDC) : modes)) );

    length -= writeNow;
    address += writeNow;
    temp += writeNow;
    writeNow = block;          /* align at BLOCK */
  }
  return( flOK );
}
#endif
#define SECOND_TRY 0x8000

/*----------------------------------------------------------------------*/
/*		          r e a d O n e S e c t o r			*/
/*									*/
/* Read up to one 512-byte block from flash.				*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/* 	address	: Address to read from.					*/
/*	buffer	: buffer to read to.					*/
/*	length	: number of bytes to read (up to sector size).		*/
/*	modes	: EDC flag etc.						*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus readOneSector (FLFlash vol,
			       CardAddress address,
			       void FAR1 *buffer,
			       int length,
			       int modes)
{
#ifndef NO_EDC_MODE
  unsigned char    extraBytes[SYNDROM_BYTES];
#endif
  FLStatus  stat = flOK;
  PointerOp   cmd   = AREA_A;            /* default for .... */
  CardAddress addr  = address;           /* .... KM29N16000  */
  int  toFirstPage, toSecondPage;

  mapWin(&vol, &addr);

  makeCommand(&vol, &cmd, &addr, modes);  /* move flash pointer to areas A,B or C of page */

  readCommand(&vol, cmd, addr);

#ifndef NO_EDC_MODE
  if( modes & EDC )
    eccONread(&vol);
#endif

  if( !(vol.flags & BIG_PAGE) )
  {
    /* read up to two pages separately */
    if( modes & EXTRA )
      toFirstPage = EXTRA_LEN - ((unsigned short)addr & (EXTRA_LEN-1));
    else
      toFirstPage = CHIP_PAGE_SIZE - ((unsigned short)addr & (CHIP_PAGE_SIZE-1));

    if(toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage;

    rdBuf (&vol, (unsigned char FAR1 *)buffer, toFirstPage ); /* starting page */

    if ( toSecondPage > 0 )                  /* next page */
    {
      if( modes & EXTRA )
        addr -= (CHIP_PAGE_SIZE + ((unsigned short)addr & (EXTRA_LEN-1)));
      readCommand (&vol, cmd, addr + toFirstPage);
      rdBuf(&vol, (unsigned char FAR1 *)buffer + toFirstPage, toSecondPage );
    }
  }
  else
    rdBuf(&vol, (unsigned char FAR1 *)buffer, length );

#ifndef NO_EDC_MODE
  if( modes & EDC )
  {       /* read syndrom to let it through the ECC unit */

    rdBuf(&vol, extraBytes, SYNDROM_BYTES );

    if( eccError(&vol) )
    {     /* try to fix ECC error */
      if ( modes & SECOND_TRY )             /* 2nd try */
      {
        unsigned char syndrom[SYNDROM_BYTES];
        unsigned char tmp;

        docread(&vol,Nsyndrom,(void FAR1*)syndrom,SYNDROM_BYTES);
        tmp = syndrom[0];                          /* Swap 1 and 3 words */
        syndrom[0] = syndrom[4];
        syndrom[4] = tmp;
        tmp = syndrom[1];
        syndrom[1] = syndrom[5];
        syndrom[5] = tmp;

        if( flCheckAndFixEDC( (char FAR1 *)buffer, (char*)syndrom, 1) != NO_EDC_ERROR) {
          DEBUG_PRINT(("Debug: EDC error for NFDC 2148.\n"));
	  stat = flDataError;
        }
      }
      else                                  /* 1st try - try once more */
        return( readOneSector( &vol, address, buffer, length, modes | SECOND_TRY ) );
    }

    eccOFF(&vol);
  }
#endif

  writeSignals (&vol, FLASH_IO | WP);
  if( (modes & EXTRA) &&                    /* Serial Read Cycle Entry */
      ((length + (((unsigned short)addr) & (NFDC21thisVars->tailSize - 1)))
        == NFDC21thisVars->tailSize) )
    waitForReady(&vol);

  return( stat );
}

/*----------------------------------------------------------------------*/
/*		          d o c 2 R e a d				*/
/*									*/
/* Read some data from the flash. This routine will be registered as 	*/
/* the read routine for this MTD.					*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/* 	address	: Address to read from.					*/
/*	buffer	: buffer to read to.					*/
/*	length	: number of bytes to read (up to sector size).		*/
/*	modes	: EDC flag etc.						*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus doc2Read(FLFlash vol,
			 CardAddress address,
			 void FAR1 *buffer,
			 int length,
			 int modes)
{
  char FAR1 *temp = (char FAR1 *)buffer;
  int readNow;
  int block = (( modes & EXTRA ) ? EXTRA_LEN : SECTOR_SIZE );

  chkASICmode(&vol);
	      /* read in BLOCKs; first and last might be partial */

  readNow = block - ((unsigned short)address & (block - 1));

  while( length > 0 ) {
    if( readNow > length )
      readNow = length;
	      /* turn off EDC on partial block read */
    checkStatus( readOneSector(&vol, address, temp, readNow, (readNow != SECTOR_SIZE ?
							      (modes &= ~EDC) : modes)) );

    length -= readNow;
    address += readNow;
    temp += readNow;
    readNow = block;       /* align at BLOCK */
  }
  return( flOK );
}

#ifdef VERIFY_ERASE

/*----------------------------------------------------------------------*/
/*		          c h e c k E r a s e				*/
/*									*/
/* Check if media is truly erased (main areas of page only).		*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/* 	address	: Address of page to check.				*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus	: 0 if page is erased, otherwise writeFault.	*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus checkErase( FLFlash vol, CardAddress address )
{
  register int i, j;
  unsigned long readback[ SECTOR_SIZE/sizeof(unsigned long) ];

  for ( i = 0 ; i < vol.erasableBlockSize / SECTOR_SIZE ; i++, address += SECTOR_SIZE )
  {
    if ( doc2Read(&vol, address, (void FAR1 *)readback, SECTOR_SIZE, 0) !=  OK )
      return( flWriteFault );

    for ( j = 0 ; j < SECTOR_SIZE/sizeof(unsigned long) ; j++ )
      if ( readback[j] != 0xFFFFFFFF )
        return( flWriteFault );
  }

  return( flOK );
}

#endif /* VERIFY_ERASE */

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*		          d o c 2 E r a s e				*/
/*									*/
/* Erase number of blocks. This routine will be registered as the	*/
/* erase routine for this MTD.						*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	blockNo		: First block to erase.				*/
/*	blocksToErase   : Number of blocks to erase.			*/
/*									*/
/* Returns:                                                          	*/
/*	FLStatus		: 0 on success, otherwise failed.	*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus doc2Erase(FLFlash vol,
			  int blockNo,
			  int blocksToErase)
{
  FLStatus status = flOK;
  unsigned char  floorToUse;
  int   nextFloorBlockNo, i;
  CardAddress  startAddress = (CardAddress)blockNo * vol.erasableBlockSize,
               address = startAddress;

  if (flWriteProtected(vol.socket))
    return( flWriteProtect );

  if( blockNo + blocksToErase > NFDC21thisVars->noOfBlocks * vol.noOfChips)
    return( flWriteFault );                             /* out of media */

  chkASICmode(&vol);
	      /* handle erase accross floors */

  floorToUse = (unsigned char)(startAddress / NFDC21thisVars->floorSize);
  nextFloorBlockNo = (int)(((floorToUse + 1) * NFDC21thisVars->floorSize) /
			   vol.erasableBlockSize);

  if( blockNo + blocksToErase > nextFloorBlockNo )
  {           /* erase on higher floors */
    checkStatus( doc2Erase( &vol, nextFloorBlockNo,
			blocksToErase - (nextFloorBlockNo - blockNo)) );
    blocksToErase = nextFloorBlockNo - blockNo;
  }
	      /* erase on this floor */

  mapWin (&vol, &address);

  for (i = 0; i < blocksToErase ; i++, blockNo++ ) {
    unsigned long pageNo = ((unsigned long)blockNo * NFDC21thisVars->pagesPerBlock);

    command(&vol, RESET_FLASH);
    writeSignals (&vol, FLASH_IO | CE);
    waitForReady(&vol);

    command(&vol, SETUP_ERASE);

    writeSignals (&vol, FLASH_IO | ALE | CE);
#ifdef SLOW_IO_FLAG
    flWrite8bitReg(&vol,NslowIO,(Reg8bitType)pageNo);
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)pageNo);
    flWrite8bitReg(&vol,NslowIO,(Reg8bitType)(pageNo >> 8));
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(pageNo >> 8));
    if( vol.flags & BIG_ADDR ) {
      flWrite8bitReg(&vol,NslowIO,(Reg8bitType)(pageNo >> 16));
      flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(pageNo >> 16));
    }
#else
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)pageNo);
    flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(pageNo >> 8));
    if( vol.flags & BIG_ADDR )
      flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)(pageNo >> 16));
    if( NFDC21thisVars->flags & MDOC_ASIC )
      flWrite8bitReg(&vol,NwritePipeTerm,(Reg8bitType)0);
#endif
    writeSignals(&vol, FLASH_IO | CE);

	      /* if only one block may be erase at a time then do it */
	      /* otherwise leave it for later                        */

    command(&vol, CONFIRM_ERASE);
    if(waitForReadyWithYieldCPU(&vol,MAX_WAIT)==FALSE)
       {
        waitForReady(&vol);
       }
    if ( readStatus(&vol) & DUB(FAIL) ) {         /* erase operation failed */
      DEBUG_PRINT(("Debug: NFDC 2148 erase failed.\n"));
      status = flWriteFault;

		/* reset flash device and abort */

      command(&vol, RESET_FLASH);
      waitForReady(&vol);

      break;
    }
    else {                                    /* no failure reported */
#ifdef VERIFY_ERASE

      if ( checkErase( &vol, startAddress + i * flash.erasableBlockSize) != OK ) {
	DEBUG_PRINT(("Debug: NFDC 2148 erase failed in verification.\n"));
	return flWriteFault ;
      }

#endif  /* VERIFY_ERASE */
    }
  }       /* block loop */

#ifdef MULTI_ERASE
	/* do multiple block erase as was promised */

  command(&vol, CONFIRM_ERASE);
  waitForReadyWithYieldCPU(&vol,MAX_WAIT);

  if ( readStatus(&vol) & DUB(FAIL) ) {        /* erase operation failed */
    DEBUG_PRINT(("Debug: NFDC 2148 erase failed.\n"));
    status = flWriteFault;

		/* reset flash device and abort */

    command(&vol, RESET_FLASH);
    waitForReady(&vol);
  }
#endif   /* MULTI_ERASE */

  writeSignals (&vol, FLASH_IO | WP);
  return( status );
}

#endif

NFDC21Vars* getNFDC21Vars(unsigned driveNo)
{
  return (&(mtdVars[driveNo]));
}

/*----------------------------------------------------------------------*/
/*		          d o c 2 M a p					*/
/*									*/
/* Map through buffer. This routine will be registered as the map	*/
/* routine for this MTD.						*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/*	address	: Flash address to be mapped.				*/
/*	length	: number of bytes to map.				*/
/*									*/
/* Returns:                                                          	*/
/* 	Pointer to the buffer data was mapped to.			*/
/*									*/
/*----------------------------------------------------------------------*/

static void FAR0 *doc2Map ( FLFlash vol, CardAddress address, int length )
{
  doc2Read(&vol,address,NFDC21thisBuffer,length, 0);
  vol.socket->remapped = TRUE;
  return( (void FAR0 *)NFDC21thisBuffer );
}




/*----------------------------------------------------------------------*/
/*		          i s K n o w n M e d i a			*/
/*									*/
/* Check if this flash media is supported. Initialize relevant fields	*/
/* in data structures.							*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	vendorId_P	: vendor ID read from chip.			*/
/*	chipId_p	: chip ID read from chip.			*/
/*	dev		: dev chips were accessed before this one.	*/
/*									*/
/* Returns:                                                          	*/
/* 	TRUE if this media is supported, FALSE otherwise.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLBoolean isKnownMedia( FLFlash vol, Reg8bitType vendorId_p, Reg8bitType chipId_p, int dev )
{
  if( (NFDC21thisVars->currentFloor == 0) && (dev == 0) ) {  /* First Identification */

    NFDC21thisVars->vendorID = (unsigned short)vendorId_p;  /* remember for next chips */
    NFDC21thisVars->chipID = (unsigned short)chipId_p;
    NFDC21thisVars->pagesPerBlock = PAGES_PER_BLOCK;
    switch( vendorId_p ) {
      case DUB(0xEC) :                  /* Samsung */
	 switch( chipId_p ) {
	   case DUB(0x64) :             /* 2 Mb */
	   case DUB(0xEA) :
	      vol.type    = KM29N16000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x100;
	      vol.chipSize     = 0x200000L;
	      break;

	   case DUB(0xE3) :             /* 4 Mb */
	   case DUB(0xE5) :
	      vol.type    = KM29N32000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x400000L;
	      vol.flags       |= BIG_PAGE;
	      break;

	   case DUB(0xE6) :             /* 8 Mb */
	      vol.type    = KM29V64000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x800000L;
	      vol.flags       |= BIG_PAGE;
	      break;

	   case DUB(0x73) :             /* 16 Mb  */
	      vol.type    = KM29V128000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x1000000L;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   case DUB(0x75) :             /* 32 Mb */
	      vol.type    = KM29V256000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x2000000L;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   case DUB(0x76) :             /* 64 Mb */
	      vol.type    = KM29V512000_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x4000000L;
	      vol.flags       |= (BIG_PAGE | BIG_ADDR);
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   default :                    /* Undefined Flash */
	      return(FALSE);
	 }
	 break;

      case DUB(0x98) :                  /* Toshiba */
	 switch( chipId_p ) {
	   case DUB(0x64) :             /* 2 Mb */
	   case DUB(0xEA) :
	      vol.type    = TC5816_FLASH;
	      NFDC21thisVars->chipPageSize = 0x100;
	      vol.chipSize     = 0x200000L;
	      break;

	   case DUB(0x6B) :             /* 4 Mb */
	   case DUB(0xE5) :
	      vol.type    = TC5832_FLASH;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x400000L;
	      break;

	   case DUB(0xE6) :             /* 8 Mb */
	      vol.type    = TC5864_FLASH;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x800000L;
	      break;

	   case DUB(0x73) :             /* 16 Mb */
	      vol.type    = TC58128_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x1000000L;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   case DUB(0x75) :             /* 32 Mb */
	      vol.type    = TC58256_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x2000000L;
	      vol.flags       |= BIG_PAGE;
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   case DUB(0x76) :             /* 64 Mb */
	      vol.type    = TC58512_FLASH;
	      NFDC21thisVars->chipPageSize = 0x200;
	      vol.chipSize     = 0x4000000L;
	      vol.flags       |= (BIG_PAGE | BIG_ADDR);
	      NFDC21thisVars->pagesPerBlock *= 2;
	      break;

	   default :                    /* Undefined Flash */
              return( FALSE );
	 }
	 vol.flags |= FULL_PAGE;    /* no partial page programming */
	 break;

      default :                         /* Undefined Flash */
         return( FALSE );
    }
    return( TRUE );
  }
  else                                  /* dev != 0 */
    if( (vendorId_p == NFDC21thisVars->vendorID) && (chipId_p == NFDC21thisVars->chipID) )
      return( TRUE );

  return( FALSE );
}

/*----------------------------------------------------------------------*/
/*		          r e a d F l a s h I D				*/
/*									*/
/* Read vendor and chip IDs, count flash devices. Initialize relevant	*/
/* fields in data structures.						*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	dev		: dev chips were accessed before this one.	*/
/*									*/
/* Returns:                                                          	*/
/* 	TRUE if this media is supported, FALSE otherwise.		*/
/*									*/
/*----------------------------------------------------------------------*/

int  readFlashID ( FLFlash vol, int dev )
{
  unsigned char vendorId_p, chipId_p;
  register int i;
  volatile Reg8bitType junk = 0;

  command (&vol, READ_ID);

  writeSignals (&vol, FLASH_IO | ALE | CE | WP);
#ifdef SLOW_IO_FLAG
  flWrite8bitReg(&vol,NslowIO,(Reg8bitType)0);
#endif
  flWrite8bitReg(&vol,NFDC21thisIO,(Reg8bitType)0);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(&vol,NwritePipeTerm,(Reg8bitType)0);
  writeSignals (&vol, FLASH_IO | CE | WP);

	    /* read vendor ID */

  flDelayMsecs( 10 );                         /* 10 microsec delay */
  junk += flRead8bitReg(&vol,NslowIO);       /* read CDSN_slow_IO ignoring the data */
  for( i = 0;( i < 2 ); i++ )   /* perform 2 reads from NOP reg for delay */
    junk += flRead8bitReg(&vol,NNOPreg);

  vendorId_p = flRead8bitReg(&vol,NFDC21thisIO); /* finally read vendor ID */

	    /* read chip ID */

  flDelayMsecs( 10 );                         /* 10 microsec delay */
  junk += flRead8bitReg(&vol,NslowIO);  /* read CDSN_slow_IO ignoring the data */
  for( i = 0;( i < 2 ); i++ )   /* perform 2 reads from NOP reg for delay */
    junk += flRead8bitReg(&vol,NNOPreg);

  chipId_p = flRead8bitReg(&vol,NFDC21thisIO); /* finally read chip ID */

  if ( isKnownMedia(&vol, vendorId_p, chipId_p, dev) != TRUE )    /* no chip or diff. */
    return( FALSE );                                              /* type of flash    */

  vol.noOfChips++;

  writeSignals (&vol, FLASH_IO);

	    /* set flash parameters */

  if( (NFDC21thisVars->currentFloor == 0) && (dev == 0) )
  {
    NFDC21thisVars->pageAreaSize   = 0x100;
    NFDC21thisVars->pageSize       = NFDC21thisVars->chipPageSize;

    if ( vol.flags & BIG_PAGE )
      NFDC21thisVars->tailSize = 2 * EXTRA_LEN;  /* = 16 */
    else
      NFDC21thisVars->tailSize = EXTRA_LEN;      /* = 8 */

    NFDC21thisVars->pageMask        = (unsigned short)(NFDC21thisVars->pageSize - 1);
    vol.erasableBlockSize     = NFDC21thisVars->pagesPerBlock * NFDC21thisVars->pageSize;
    NFDC21thisVars->noOfBlocks      = (unsigned short)( vol.chipSize / vol.erasableBlockSize );
    NFDC21thisVars->pageAndTailSize = (unsigned short)(NFDC21thisVars->pageSize + NFDC21thisVars->tailSize);
  }

  return( TRUE );
}

/*----------------------------------------------------------------------*/
/*		          d o c 2 I d e n t i f y			*/
/*									*/
/* Identify flash. This routine will be registered as the		*/
/* identification routine for this MTD.                                 */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                          	*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

static FLStatus doc2Identify(FLFlash vol)
{
  unsigned long  address = 0L;
  int maxDevs, dev;
  volatile Reg8bitType toggle1;
  volatile Reg8bitType toggle2;
  unsigned char  floorCnt = 0, floor;

  DEBUG_PRINT(("debug: entering NFDC 2148 identification routine.\n"));

  vol.mtdVars = &mtdVars[flSocketNoOf(vol.socket)];

  /* get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  NFDC21thisVars->buffer = flBufferOf(flSocketNoOf(vol.socket));

  setASICmode (&vol, ASIC_NORMAL_MODE);

  flSetWindowBusWidth(vol.socket, 16);/* use 16-bits */
  flSetWindowSpeed(vol.socket, 120);  /* 120 nsec. */

	    /* assume flash parameters for KM29N16000 */

  NFDC21thisVars->floorSize = 1L;
  NFDC21thisVars->totalFloors = MAX_FLOORS;
  NFDC21thisVars->currentFloor = MAX_FLOORS;
  vol.noOfChips = 0;
  vol.chipSize = 0x200000L;	/* Assume something ... */
  vol.interleaving = 1;       /* unimportant for now  */

	  /* detect card - identify bit toggles on consequitive reads */

  NFDC21thisWin = (NDOC2window)flMap(vol.socket, 0);
  if(flPreInitRead8bitReg(vol.socket->volNo,NFDC21thisWin,NchipId) == CHIP_ID_MDOC ) {
    NFDC21thisVars->flags |= MDOC_ASIC;
    NFDC21thisVars->win_io = NIPLpart2;
    vol.mediaType = MDOC_TYPE;
  }
  else {
    NFDC21thisVars->win_io = Nio;
    vol.mediaType = DOC_TYPE;
  }

  mapWin (&vol, &address);

  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    toggle1 = flRead8bitReg(&vol,NECCconfig);
    toggle2 = toggle1 ^ flRead8bitReg(&vol,NECCconfig);
  }
  else {
    toggle1 = flRead8bitReg(&vol,NECCstatus);
    toggle2 = toggle1 ^ flRead8bitReg(&vol,NECCstatus);
  }
  if ( (toggle2 & TOGGLE) == 0 ) {
    DEBUG_PRINT(("Debug: failed to identify NFDC 2148.\n"));
    return( flUnknownMedia );
  }

           /* reset all flash devices */
  if( flRead8bitReg(&vol,NchipId) == CHIP_ID_MDOC )
    maxDevs = MAX_FLASH_DEVICES_MDOC;
  else maxDevs = MAX_FLASH_DEVICES_DOC;

  for ( NFDC21thisVars->currentFloor = 0 ;
	NFDC21thisVars->currentFloor < MAX_FLOORS ;
	NFDC21thisVars->currentFloor++ )
  {
  /* select floor */
    flWrite8bitReg(&vol,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor);

    for ( dev = 0 ; dev < maxDevs ; dev++ ) {
      selectChip(&vol, (Reg8bitType)dev );
      command(&vol, RESET_FLASH);
    }
  }
  NFDC21thisVars->currentFloor = (unsigned char)0;
  /* back to ground floor */
  flWrite8bitReg(&vol,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor);

  writeSignals (&vol, FLASH_IO | WP);

	   /* identify and count flash chips, figure out flash parameters */

  for( floor = 0; floor < MAX_FLOORS;  floor++ )
    for ( dev = 0; dev < maxDevs;  dev++ )
    {
      unsigned long  addr = address;

      mapWin(&vol, &addr);

      if ( readFlashID(&vol, dev) == TRUE )                      /* identified OK */
      {
	      floorCnt = (unsigned char)(floor + 1);
        if (floor == 0)
          NFDC21thisVars->floorSize += vol.chipSize * vol.interleaving;
          address += vol.chipSize * vol.interleaving;
        }
      else
      {
        if (floor == 0) {
          maxDevs = dev;
          NFDC21thisVars->floorSize = maxDevs * vol.chipSize * vol.interleaving;
        }
        else {
          dev = maxDevs;
	  floor = MAX_FLOORS;
        }
      }
    }
  NFDC21thisVars->currentFloor = (unsigned char)0;
  flWrite8bitReg(&vol,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor); /* back to ground floor */

  if (vol.noOfChips == 0) {
    DEBUG_PRINT(("Debug: failed to identify NFDC 2148.\n"));
    return( flUnknownMedia );
  }

  address = 0L;
  mapWin (&vol, &address);

  NFDC21thisVars->totalFloors = floorCnt;

  eccOFF(&vol);

  /* Register our flash handlers */
  #ifndef FL_READ_ONLY
  vol.write = doc2Write;
  vol.erase = doc2Erase;
  #else
  vol.erase=vol.write=NULL;
  #endif
  vol.read = doc2Read;
  vol.map = doc2Map;

  vol.flags |= NFTL_ENABLED;

  DEBUG_PRINT(("Debug: identified NFDC 2148.\n"));
  return( flOK );
}


/*----------------------------------------------------------------------*/
/*                      f l R e g i s t e r D O C 2 0 0 0 		*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterDOC2000(void)
{
  if (noOfMTDs >= MTDS)
    return( flTooManyComponents );

  mtdTable[noOfMTDs++] = doc2Identify;

  return( flOK );
}
