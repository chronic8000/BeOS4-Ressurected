/*
 * $Log:   V:/Flite/archives/FLite/src/NFTLLITE.C_V  $
 * 
 *    Rev 1.68   16 Mar 2000 11:11:08   dimitrys
 * Add casting (ANANDUnitNo) to noOfBootUnits 
 * 
 *    Rev 1.67   Mar 15 2000 15:31:16   vadimk
 * Initialization of the status variable in the defragment
 *   function was added
 *
 *    Rev 1.66   14 Mar 2000 17:18:02   dimitrys
 * Defragmentation bug fixed: always returns real number
 *   of free sectors
 *
 *    Rev 1.65   12 Mar 2000 13:58:30   dimitrys
 * Get rid of warnings
 *
 *    Rev 1.64   05 Mar 2000 18:11:50   dimitrys
 * Set ucache & scache pointer to NULL in flRegisterNFTL
 *
 *    Rev 1.63   05 Mar 2000 14:12:26   dimitrys
 * Add setting pointers to Physical and Virtual Units Table
 *   and NFTL Cache to NULL in dismountNFTL() call
 *
 *    Rev 1.62   02 Mar 2000 18:07:48   dimitrys
 * Check if S_CACHE Size >= 64 Kb when MALLOC
 *   gets 'unsigned' == 2 bytes as a parameter and set
 *   S_CACHE OFF
 *
 *    Rev 1.61   02 Mar 2000 15:48:08   dimitrys
 * Move scacheTable to NFTLLITE.C file
 *
 *    Rev 1.60   23 Feb 2000 17:48:38   dimitrys
 * Switch to bigger unit if noOfUnits >= 12K in order to
 *   keep noOfUnits smaller than 12K (192M DOC Size)
 *
 *    Rev 1.59   21 Feb 2000 11:38:26   dimitrys
 * Mount problems during reset cycling fixed:
 *   - Function setUnitData(): in case if replacementUnitNo
 *     equals to UnitNo return generalFailure to prevent
 *     loop creating
 *   - Function foldUnit(): loop of Unit Chain is bounded to
 *     2 * MAX_UNIT_CHAIN
 *   - Function allocateAndWriteBlock(): loop of Unit Chain
 *     is bounded to 2 * MAX_UNIT_CHAIN and in this case
 *     new unit assignment and folding is forced
 *
 *    Rev 1.58   10 Feb 2000 17:32:46   dimitrys
 * Fix bounding of chain up to 2*MAX_UNIT_CHAIN
 *
 *    Rev 1.57   10 Feb 2000 14:41:40   dimitrys
 * Bound chain search in virtual2Physical() function
 *
 *    Rev 1.56   02 Feb 2000 18:04:28   dimitrys
 * Convert first unit of MDOC of any floor > 0 to BAD in
 *   order to make it  unchangable and force internal
 *   EEprom mode
 *
 *    Rev 1.55   Jan 23 2000 17:27:46   vadimk
 * Put flusenftlcache under ENVIRONMENT_VARS
 * define
 *
 *    Rev 1.54   Jan 20 2000 18:13:38   vadimk
 * return *sectorsNeeded in case of single unit chain folding in defragment
 * remove non ANSI C comments
 *
 *    Rev 1.53   Jan 19 2000 15:48:08   vadimk
 * check extra area in the isErased function
 *
 *    Rev 1.52   Jan 16 2000 14:39:06   vadimk
 * "fl" prefix was added to the environment variables
 *
 *    Rev 1.51   Jan 13 2000 18:29:52   vadimk
 * TrueFFS OSAK 4.1
 *
 *    Rev 1.50   07 Oct 1999 10:17:56   dimitrys
 * Big Unit (32 Kb) "flGeneralFailure" problem fixed
 *  - function assignUnit(): (...) <= UNIT_MAX_COUNT
 *
 *    Rev 1.49   Aug 03 1999 12:00:06   marinak
 * Wear Leveling of static files
 *
 *    Rev 1.48   Jul 29 1999 18:52:50   marinak
 * WriteSector bug fixed
 *
 *    Rev 1.47   Jul 12 1999 18:06:00   marinak
 * Unite U_CACHE and S_CACHE definitions
 *
 *    Rev 1.46   Jun 20 1999 16:06:18   dimitrys
 * Force writing of Original and Spare Original Units if virtual medium size was changed (use value changed)
 *
 *    Rev 1.45   Jun 10 1999 15:35:08   marinak
 * Wrong amount bad block  checking in formatNFTL is fixed
 *
 *    Rev 1.44   May 25 1999 16:50:20   Amirban
 * Moved init of NFTL cache vars to initNFTL
 *
 *    Rev 1.43   16 Mar 1999 14:29:02   Dmitry
 * Mount problems during power cycling fixed:
 *  - Function lastInChain: loop bounded to MAX_UNIT_CHAIN (20)
 *  - Function formatChain: exit from loop if formatUnit failed
 *  - Function mountNFTL: run formatChain on every ORPHAN unit
 *
 *    Rev 1.42   03 Mar 1999 11:46:52   marina
 * fix compilation problem
 *
 *    Rev 1.41   28 Feb 1999 19:06:58   Dmitry
 * Bad block processing in new format fixed (UNIT_BAD_MOUNT)
 *
 *    Rev 1.40   25 Feb 1999 12:05:38   Dmitry
 * Add EXTRA_LARGE define for special unit size calculation
 *
 *    Rev 1.39   24 Feb 1999 14:48:46   marina
 * formatNFTL bug fix
 *
 *    Rev 1.38   24 Feb 1999 14:27:42   marina
 * put TLrec back
 *
 *    Rev 1.37   23 Feb 1999 21:10:32   marina
 * formatNFTL bugs fix
 *
 *    Rev 1.36   14 Feb 1999 16:49:12   amirban
 * Format progress callback
 *
 *    Rev 1.35   14 Feb 1999 16:22:28   amirban
 * Changed mapSector from map to read
 *
 *    Rev 1.34   31 Jan 1999 19:54:40   marina
 * writeMultiSector
 *
 *    Rev 1.33   13 Jan 1999 18:37:46   marina
 * Always define sectorsInVolume
 *
 *    Rev 1.32   12 Jan 1999 14:49:06   amirban
 * Extra-large
 *
 *    Rev 1.32   10 Jan 1999 18:28:32   amirban
 * Extra-large
 *
 *    Rev 1.31   05 Jan 1999 13:38:00   amirban
 * NFTL cache without MALLOC
 *
 *    Rev 1.30   03 Jan 1999 12:07:56   amirban
 * Correct size of NFTL cache
 *
 *    Rev 1.29   28 Dec 1998 14:26:28   marina
 * Prepare for unconditional dismount. Get rid of warnings.
 *
 *    Rev 1.28   27 Oct 1998  7:53:50   Dimitry
 * Add erase of Original and SpareOriginal Units after
 *   format in case when new bootimage size bigger than previous
 *
 *    Rev 1.27   26 Oct 1998 18:02:18   marina
 * in function flRegisterNFTL formatRoutine initialization
 *    is called if not defined FORMAT_VOLUME
 *
 *    Rev 1.26   24 Sep 1998 16:47:26   amirban
 * Two minor bugs
 *
 *    Rev 1.25   03 Sep 1998 13:58:58   ANDRY
 * better DEBUG_PRINT
 *
 *    Rev 1.24   03 Sep 1998 13:03:24   ANDRY
 * bug fix in copySector()
 *
 *    Rev 1.23   02 Sep 1998 17:00:16   ANDRY
 * U- and S-cache
 *
 *    Rev 1.22   16 Aug 1998 20:30:00   amirban
 * TL definition changes for ATA & ZIP
 *
 *    Rev 1.21   01 Mar 1998 12:59:46   amirban
 * Add parameter to mapSector, and use fast-mount procedure
 *
 *    Rev 1.20   23 Feb 1998 17:08:38   Yair
 * Added casts
 *
 *    Rev 1.19   23 Nov 1997 17:20:52   Yair
 * Get rid of warnings (with Danny)
 *
 *    Rev 1.18   11 Nov 1997 15:26:22   ANDRY
 * get rid of compiler warnings
 *
 *    Rev 1.17   10 Sep 1997 16:30:10   danig
 * Got rid of generic names
 *
 *    Rev 1.16   04 Sep 1997 16:18:06   danig
 * Debug messages
 *
 *    Rev 1.15   31 Aug 1997 15:21:04   danig
 * Registration routine return status
 *
 *    Rev 1.14   28 Aug 1997 16:44:46   danig
 * Share buffer with MTD
 *
 *    Rev 1.13   21 Aug 1997 14:06:28   unknown
 * Unaligned4
 *
 *    Rev 1.12   14 Aug 1997 14:10:42   danig
 * Fixed defragment bug
 *
 *    Rev 1.11   28 Jul 1997 14:51:16   danig
 * volForCallback
 *
 *    Rev 1.10   24 Jul 1997 17:57:42   amirban
 * FAR to FAR0
 *
 *    Rev 1.9   20 Jul 1997 18:27:44   danig
 * Initialize vol.flash in formatNFTL
 *
 *    Rev 1.8   07 Jul 1997 15:22:28   amirban
 * Ver 2.0
 *
 *    Rev 1.7   29 Jun 1997 17:56:36   danig
 * Comments
 *
 *    Rev 1.6   29 Jun 1997 15:00:12   danig
 * Fixed formatted size
 *
 *    Rev 1.5   08 Jun 1997 17:02:52   amirban
 * SECTOR_USED is default
 *
 *    Rev 1.4   03 Jun 1997 17:08:16   amirban
 * setBusy change
 *
 *    Rev 1.3   01 Jun 1997 13:43:40   amirban
 * Big-endian & use sector-map caching
 *
 *    Rev 1.2   25 May 1997 17:51:10   danig
 * Adjust unitSizeBits so header unit fits in one unit
 *
 *    Rev 1.1   25 May 1997 15:20:46   danig
 * Changes to format
 *
 *    Rev 1.0   18 May 1997 17:57:56   amirban
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

#include "nftllite.h"
#include "diskonc.h"

static FLStatus foldUnit(Anand vol, ANANDUnitNo virtualUnitNo);
static FLStatus allocateUnit(Anand vol, ANANDUnitNo *);

static Anand vols[DRIVES];

#ifdef NFTL_CACHE
/* translation table for Sector Flags cache */
static unsigned char scacheTable[4] = { SECTOR_DELETED, /* 0 */
					SECTOR_IGNORE,  /* 1 */
					SECTOR_USED,    /* 2 */
					SECTOR_FREE };  /* 3 */
#endif /* NFTL_CACHE */

/*----------------------------------------------------------------------*/
/*		         u n i t B a s e A d d r e s s			*/
/*									*/
/* Returns the physical address of a unit.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*                                                                      */
/* Returns:                                                             */
/*	physical address of unitNo					*/
/*----------------------------------------------------------------------*/

static CardAddress unitBaseAddress(Anand vol, ANANDUnitNo unitNo)
{
  return (CardAddress)unitNo << vol.unitSizeBits;
}


/*----------------------------------------------------------------------*/
/*		         g e t U n i t D a t a 				*/
/*									*/
/* Get virtual unit No. and replacement unit no. of a unit.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		  : Physical unit number			*/
/*	virtualUnitNo	  : Receives the virtual unit no.		*/
/*	replacementUnitNo : Receives the replacement unit no.		*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void getUnitData(Anand vol,
			ANANDUnitNo unitNo,
			ANANDUnitNo *virtualUnitNo,
			ANANDUnitNo *replacementUnitNo)
{
  ANANDUnitHeader unitData;

#ifdef NFTL_CACHE
  /* check ANANDUnitHeader cache first */
  if (vol.ucache != NULL) {
      /* on cache miss read ANANDUnitHeader from flash and re-fill cache */
      if ((vol.ucache[unitNo].virtualUnitNo == 0xDEAD) &&
	  (vol.ucache[unitNo].replacementUnitNo == 0xDEAD)) {
          vol.flash.read(&vol.flash,
	    	         unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
		         &unitData,
		         sizeof(ANANDUnitHeader),
		         EXTRA);

	  vol.ucache[unitNo].virtualUnitNo =
	      LE2(unitData.virtualUnitNo) | LE2(unitData.spareVirtualUnitNo);
	  vol.ucache[unitNo].replacementUnitNo =
	      LE2(unitData.replacementUnitNo) | LE2(unitData.spareReplacementUnitNo);
      }

      *virtualUnitNo     = vol.ucache[unitNo].virtualUnitNo;
      *replacementUnitNo = vol.ucache[unitNo].replacementUnitNo;
  }
  else
#endif /* NFTL_CACHE */
  {   /* no ANANDUnitHeader cache */
      vol.flash.read(&vol.flash,
	    	     unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
		     &unitData,
		     sizeof(ANANDUnitHeader),
		     EXTRA);

      /* Mask out any 1 -> 0 bit faults by or'ing with spare data */
      *virtualUnitNo = LE2(unitData.virtualUnitNo) |
	               LE2(unitData.spareVirtualUnitNo);
      *replacementUnitNo = LE2(unitData.replacementUnitNo) |
	                   LE2(unitData.spareReplacementUnitNo);
  }
}

/*----------------------------------------------------------------------*/
/*		         s e t U n i t D a t a 				*/
/*									*/
/* Set virtual unit No. and replacement unit no. of a unit.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		  : Physical unit number			*/
/*	virtualUnitNo	  : Virtual unit no.				*/
/*	replacementUnitNo : Replacement unit no.			*/
/*									*/
/* Returns:								*/
/*	FLStatus	  : 0 on success, failed otherwise		*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitData(Anand vol,
			  ANANDUnitNo unitNo,
			  ANANDUnitNo virtualUnitNo,
			  ANANDUnitNo replacementUnitNo)
{
  ANANDUnitHeader unitData;
  ANANDUnitNo newVirtualUnitNo, newReplacementUnitNo;

  if( replacementUnitNo == unitNo )              /* prevent chain loop */
    return flGeneralFailure;

  toLE2(unitData.virtualUnitNo,virtualUnitNo);
  toLE2(unitData.spareVirtualUnitNo,virtualUnitNo);
  toLE2(unitData.replacementUnitNo,replacementUnitNo);
  toLE2(unitData.spareReplacementUnitNo,replacementUnitNo);

  checkStatus(vol.flash.write(&vol.flash,
			       unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
			       &unitData,
			       sizeof(ANANDUnitHeader),
			       EXTRA));

#ifdef NFTL_CACHE
  /* purge ANANDUnitHeader cache to force re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo     = 0xDEAD;
      vol.ucache[unitNo].replacementUnitNo = 0xDEAD;
  }
#endif /* NFTL_CACHE */

  /* Verify the new unit data */
  getUnitData(&vol,unitNo,&newVirtualUnitNo, &newReplacementUnitNo);
  if (virtualUnitNo != newVirtualUnitNo ||
      replacementUnitNo != newReplacementUnitNo)
    return flWriteFault;
  else
    return flOK;
}

/*----------------------------------------------------------------------*/
/*		         g e t N e x t U n i t 				*/
/*									*/
/* Get next unit in chain.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		  : Physical unit number			*/
/*									*/
/* Returns:								*/
/* 	Physical unit number of the unit following unitNo in the chain.	*/
/*	If such unit do not exist, return ANAND_NO_UNIT.			*/
/*----------------------------------------------------------------------*/

static ANANDUnitNo getNextUnit(Anand vol, ANANDUnitNo unitNo)
{
  ANANDUnitNo virtualUnitNo, replacementUnitNo;

  if (!(vol.physicalUnits[unitNo] & UNIT_REPLACED))
    return ANAND_NO_UNIT;

  getUnitData(&vol,unitNo,&virtualUnitNo,&replacementUnitNo);

  return replacementUnitNo;
}


#ifdef NFTL_CACHE

/*----------------------------------------------------------------------*/
/*	     g e t S e c t o r F l a g s F r o m C a c h e		*/
/*									*/
/* Get sector flags from Sector Cache.    				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: starting address of the sector		*/
/*                                                                      */
/* Returns:                                                             */
/*	sector flags (SECTOR_USED, SECTOR_DELETED etc.)			*/
/*----------------------------------------------------------------------*/
static unsigned char getSectorFlagsFromCache(Anand vol, CardAddress address)
{
  return scacheTable[(vol.scache[address >> (SECTOR_SIZE_BITS+2)] >>
		     (((unsigned int)address >> 8) & 0x7)) & 0x3];
}


/*----------------------------------------------------------------------*/
/*	         s e t S e c t o r F l a g s C a c h e   		*/
/*									*/
/* Get sector flags from Sector Cache.    				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: starting address of the sector		*/
/*	sectorFlags	: one of SECTOR_USED, SECTOR_DELETED etc.	*/
/*                                                                      */
/*----------------------------------------------------------------------*/
static void setSectorFlagsCache(Anand vol, CardAddress address,
				unsigned char sectorFlags)
{
  register unsigned char tmp, val;

  if (vol.scache == NULL)
    return;

  tmp = vol.scache[address >> (SECTOR_SIZE_BITS+2)];

  switch(sectorFlags) {
    case SECTOR_USED:          val = S_CACHE_SECTOR_USED;    break;
    case SECTOR_FREE:          val = S_CACHE_SECTOR_FREE;    break;
    case SECTOR_DELETED:       val = S_CACHE_SECTOR_DELETED; break;
    default:/* SECTOR_IGNORE */val = S_CACHE_SECTOR_IGNORE;  break;
  }

  switch (((unsigned int)address >> 8) & 0x7) {
    case 0: tmp = (tmp & 0xfc) | (val     ); break;  /* update bits 0..1 */
    case 2: tmp = (tmp & 0xf3) | (val << 2); break;  /*        bits 2..3 */
    case 4: tmp = (tmp & 0xcf) | (val << 4); break;  /*        bits 4..5 */
    case 6: tmp = (tmp & 0x3f) | (val << 6); break;  /*        bits 6..7 */
  }

  vol.scache[address >> (SECTOR_SIZE_BITS+2)] = tmp;
}

#endif /* NFTL_CACHE */



/*----------------------------------------------------------------------*/
/*      	        g e t S e c t o r F l a g s			*/
/*									*/
/* Get sector status. 							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	sectorAddress		: Physical address of the sector	*/
/*                                                                      */
/* Returns:                                                             */
/*	Return the OR of the two bytes in the sector status area (the	*/
/*	bytes should contain the same data).				*/
/*----------------------------------------------------------------------*/

static unsigned char getSectorFlags(Anand vol, CardAddress sectorAddress)
{
  unsigned char flags[2];

#ifdef NFTL_CACHE
  if (vol.scache != NULL) {
    /* check for Sector Flags cache hit */
    flags[0] = getSectorFlagsFromCache(&vol, sectorAddress);
    if (flags[0] != SECTOR_IGNORE)
      return flags[0];
  }
#endif /* NFTL_CACHE */

  vol.flash.read(&vol.flash,
		  sectorAddress + SECTOR_DATA_OFFSET,
		  &flags,
		  sizeof flags,
		  EXTRA);

#ifdef NFTL_CACHE
  /* update Sector Flags cache */
  setSectorFlagsCache(&vol, sectorAddress, (unsigned char)(flags[0] | flags[1]));
#endif

  return (unsigned char)(flags[0] | flags[1]);
}


/*----------------------------------------------------------------------*/
/*		         v i r t u a l 2 P h y s i c a l		*/
/*									*/
/* Translate virtual sector number to physical address.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector number				*/
/*                                                                      */
/* Returns:                                                             */
/*	physical address of sectorNo					*/
/*----------------------------------------------------------------------*/

static CardAddress virtual2Physical(Anand vol, SectorNo sectorNo)
{
  unsigned unitOffset = (unsigned)((sectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS);
  CardAddress prevSectorAddress = ANAND_UNASSIGNED_ADDRESS;
  ANANDUnitNo unitNo;
  ANANDUnitNo chainBound = 0;

  /* follow the chain */
  for (unitNo = vol.virtualUnits[((ANANDUnitNo)(sectorNo / vol.sectorsPerUnit))];
       ( (unitNo != ANAND_NO_UNIT) && (chainBound < 2*MAX_UNIT_CHAIN) );
       unitNo = getNextUnit(&vol,unitNo)) {
    CardAddress sectorAddress = unitBaseAddress(&vol,unitNo) + unitOffset;
    unsigned char sectorFlags = getSectorFlags(&vol,sectorAddress);

    if (sectorFlags == SECTOR_FREE)
      break;

    if (sectorFlags != SECTOR_IGNORE)
      prevSectorAddress = sectorFlags != SECTOR_DELETED ? sectorAddress :
						    ANAND_UNASSIGNED_ADDRESS;
    chainBound++;
  }

  return prevSectorAddress;
}


/*----------------------------------------------------------------------*/
/*		         g e t F o l d M a r k				*/
/*									*/
/* Get the fold mark a unit.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*                                                                      */
/* Returns:                                                             */
/*	Return the OR of the two words in the fold mark area (the words	*/
/*	should be identical)						*/
/*----------------------------------------------------------------------*/

static unsigned short getFoldMark(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short foldMark[2];

  vol.flash.read(&vol.flash,
		  unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
		  &foldMark, sizeof foldMark,
		  EXTRA);

  return foldMark[0] | foldMark[1];
}


/*----------------------------------------------------------------------*/
/*		         g e t U n i t T a i l e r			*/
/*									*/
/* Get the erase record of a unit.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*	eraseMark	: Receives the erase mark of the unit		*/
/*	eraseCount	: Receives the erase count of the unit		*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void getUnitTailer(Anand vol,
			  ANANDUnitNo unitNo,
			  unsigned short *eraseMark,
			  unsigned long *eraseCount)
{
  UnitTailer unitTailer;

  vol.flash.read(&vol.flash,
		  unitBaseAddress(&vol,unitNo) + UNIT_TAILER_OFFSET,
		  &unitTailer,
		  sizeof(UnitTailer),
		  EXTRA);

  /* Mask out any 1 -> 0 bit faults by or'ing with spare data */
  *eraseMark = LE2(unitTailer.eraseMark) | LE2(unitTailer.eraseMark1);
  *eraseCount = LE4(unitTailer.eraseCount);
}

/*----------------------------------------------------------------------*/
/*		         s e t U n i t T a i l e r			*/
/*									*/
/* Set the erase record of a unit.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*	eraseMark	: Erase mark to set				*/
/*	eraseCount	: Erase count to set				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitTailer(Anand vol,
			    ANANDUnitNo unitNo,
			    unsigned short eraseMark,
			    unsigned long eraseCount)
{
  UnitTailer unitTailer;

  toLE2(unitTailer.eraseMark,eraseMark);
  toLE2(unitTailer.eraseMark1,eraseMark);
  toLE4(unitTailer.eraseCount,eraseCount);

  return vol.flash.write(&vol.flash,
			  unitBaseAddress(&vol,unitNo) + UNIT_TAILER_OFFSET,
			  &unitTailer,
			  sizeof(UnitTailer),
			  EXTRA);
}

/*----------------------------------------------------------------------*/
/*      	             i n i t N F T L				*/
/*									*/
/* Initializes essential volume data as a preparation for mount or	*/
/* format.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	flash		: Flash media mounted on this socket		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initNFTL(Anand vol, FLFlash *flash)
{
  long int size = 1;

  vol.wearLevel.alarm = 0xff;
  vol.wearLevel.currUnit = ANAND_NO_UNIT;

  if (flash == NULL || !(flash->flags & NFTL_ENABLED)) {
    DEBUG_PRINT(("Debug: media is not fit for NFTL format.\n"));
    return flUnknownMedia;
  }

  vol.flash = *flash;

  vol.physicalUnits = NULL;
  vol.virtualUnits = NULL;

#ifdef NFTL_CACHE
  vol.ucache = NULL;
  vol.scache = NULL;
#endif

  for (vol.erasableBlockSizeBits = 0; size < vol.flash.erasableBlockSize;
       vol.erasableBlockSizeBits++, size <<= 1);
  vol.unitSizeBits = vol.erasableBlockSizeBits;

  vol.noOfUnits = (unsigned short)((vol.flash.noOfChips * vol.flash.chipSize) >> vol.unitSizeBits);

  /* Adjust unit size so header unit fits in one unit */
  while (vol.noOfUnits * sizeof(ANANDPhysUnit) + SECTOR_SIZE > (1UL << vol.unitSizeBits)) {
    vol.unitSizeBits++;
    vol.noOfUnits >>= 1;
  }
  /* Bound number of units to find room in 64 Kbytes Segment */
  if( (vol.noOfUnits >= MAX_UNIT_NUM) && (vol.unitSizeBits < MAX_UNIT_SIZE_BITS) ) {
    vol.unitSizeBits++;
    vol.noOfUnits >>= 1;
  }

  vol.badFormat = TRUE;	/* until mount completes*/
  vol.mappedSectorNo = UNASSIGNED_SECTOR;
  /*get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  vol.buffer = flBufferOf(flSocketNoOf(vol.flash.socket));
  vol.countsValid = 0;		/* No units have a valid count yet */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	            i n i t T a b l e s				*/
/*									*/
/* Allocates and initializes the dynamic volume table, including the	*/
/* unit tables and secondary virtual map.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initTables(Anand vol)
{
  /* Allocate the conversion tables */
#ifdef MALLOC
  vol.physicalUnits = (ANANDPhysUnit *) MALLOC(vol.noOfUnits * sizeof(ANANDPhysUnit));
  vol.virtualUnits = (ANANDUnitNo *) MALLOC(vol.noOfVirtualUnits * sizeof(ANANDUnitNo));
  if (vol.physicalUnits == NULL ||
      vol.virtualUnits == NULL) {
    DEBUG_PRINT(("Debug: failed allocating conversion tables for NFTL.\n"));
    return flNotEnoughMemory;
  }
#else
  char *heapPtr;

  heapPtr = vol.heap;
  vol.physicalUnits = (ANANDPhysUnit *) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(ANANDPhysUnit);
  vol.virtualUnits = (ANANDUnitNo *) heapPtr;
  heapPtr += vol.noOfVirtualUnits * sizeof(ANANDUnitNo);
  if (heapPtr > vol.heap + sizeof vol.heap) {
    DEBUG_PRINT(("Debug: not enough memory for NFTL conversion tables.\n"));
    return flNotEnoughMemory;
  }
#endif

  return flOK;
}

/*----------------------------------------------------------------------*/
/*      	            m a r k U n i t B a d			*/
/*									*/
/* Mark a unit as bad in the conversion table and the bad units table.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical number of bad unit			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus markUnitBad(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short eraseMark;
  unsigned long eraseCount;

  vol.physicalUnits[unitNo] = UNIT_BAD_MOUNT;

  getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount);

  return setUnitTailer(&vol,unitNo,0,eraseCount);
}


/*----------------------------------------------------------------------*/
/*		          f o r m a t U n i t				*/
/*									*/
/* Format one unit. Erase the unit, and mark the physical units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Unit to format				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatUnit(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short eraseMark;
  unsigned long eraseCount;
  FLStatus status;

  if (!isAvailable(unitNo))
    return flWriteFault;

  if (vol.physicalUnits[unitNo] == ANAND_UNIT_FREE)
    vol.freeUnits--;
  setUnavail(unitNo);

  getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount);

#ifdef NFTL_CACHE
  /* purge ANANDUnitHeader cache to force re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo     = 0xDEAD;
      vol.ucache[unitNo].replacementUnitNo = 0xDEAD;
  }

  /*
   * Purge the Sector Flags cache (set entries for all the unit's
   * sectors to SECTOR_FREE).
   */
  {
    register CardAddress sectorAddress;

    for (sectorAddress = ((CardAddress)unitNo << vol.unitSizeBits);
         sectorAddress < ((CardAddress)(unitNo + 1) << vol.unitSizeBits);
         sectorAddress += (1UL << SECTOR_SIZE_BITS))
      setSectorFlagsCache(&vol, sectorAddress, SECTOR_FREE);
  }
#endif /* NFTL_CACHE */

  status = vol.flash.erase(&vol.flash,
			    unitNo << (vol.unitSizeBits - vol.erasableBlockSizeBits),
			    1 << (vol.unitSizeBits - vol.erasableBlockSizeBits));
  if (status != flOK) {
    markUnitBad(&vol,unitNo);	/* make sure unit format is not valid */
    return status;
  }

  vol.eraseSum++;
  eraseCount++;
  if (eraseCount == 0)		/* was hex FF's */
    eraseCount++;

  checkStatus(setUnitTailer(&vol,unitNo,ERASE_MARK,eraseCount));

  vol.physicalUnits[unitNo] = ANAND_UNIT_FREE;
  vol.freeUnits++;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	        w r i t e A n d C h e c k 			*/
/*									*/
/* Write one sector. 							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: Physical address of the sector to write to	*/
/*	fromAddress	: Buffer of data to write			*/
/*	flags		: Write flags (ECC, overwrite etc.)		*/
/*									*/
/* Returns:                                                             */
/* 	Status 		: 0 on success, failed otherwise.		*/
/*----------------------------------------------------------------------*/

static FLStatus writeAndCheck(Anand vol,
			    CardAddress address,
			    void FAR1 *fromAddress,
			    unsigned flags)
{
  FLStatus status = vol.flash.write(&vol.flash,address,fromAddress,SECTOR_SIZE,flags);
  if (status == flWriteFault) {  /* write failed, ignore this sector */
    unsigned char sectorFlags[2];

    sectorFlags[0] = sectorFlags[1] = SECTOR_IGNORE;
#ifdef NFTL_CACHE
    setSectorFlagsCache(&vol, address, SECTOR_IGNORE);
#endif
    vol.flash.write(&vol.flash,address + SECTOR_DATA_OFFSET,sectorFlags,sizeof sectorFlags,EXTRA);
  }
#ifdef NFTL_CACHE
  else
    setSectorFlagsCache(&vol, address, SECTOR_USED);
#endif

  return status;
}


/*----------------------------------------------------------------------*/
/*      	        c o p y S e c t o r	 			*/
/*									*/
/* Copy one sector to another.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sourceSectorAddress	: Physical address of Sector to copy 	*/
/*				  from.					*/
/*	targetSectorAddress	: Physical address of sector to copy	*/
/*				  to.					*/
/*                                                                      */
/* Returns:                                                             */
/* 	FLStatus       		: 0 on success, failed otherwise.	*/
/*----------------------------------------------------------------------*/

static FLStatus copySector(Anand vol,
			 CardAddress sourceSectorAddress,
			 CardAddress targetSectorAddress)
{
  unsigned flags = EDC;

  vol.flash.socket->remapped = TRUE;
  if (vol.flash.read(&vol.flash,
		     sourceSectorAddress,
		     nftlBuffer,
		     SECTOR_SIZE,
		     EDC) == flDataError) {
    /* If there is an uncorrectable ECC error, copy the data as is */
    unsigned short sectorDataInfo[4];

    vol.flash.read(&vol.flash,
		    sourceSectorAddress,
		    sectorDataInfo,
		    sizeof sectorDataInfo,
		    EXTRA);
    checkStatus(vol.flash.write(&vol.flash,
				 targetSectorAddress,
				 sectorDataInfo,
				 sizeof sectorDataInfo,
				 EXTRA));
#ifdef NFTL_CACHE
    {
      unsigned char sectorFlags = (sectorDataInfo[3] & 0xff) | (sectorDataInfo[3] >> 8);
      setSectorFlagsCache(&vol, targetSectorAddress, sectorFlags);
    }
#endif

    flags &= ~EDC;
  }

  return writeAndCheck(&vol,targetSectorAddress,nftlBuffer,flags);
}

/*----------------------------------------------------------------------*/
/*      	        l a s t I n C h a i n	 			*/
/*									*/
/* Find last unit in chain.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: Start the search from this unit		*/
/*                                                                      */
/* Returns:                                                             */
/* 	Physical unit number of the last unit in chain.			*/
/*----------------------------------------------------------------------*/

static ANANDUnitNo lastInChain(Anand vol, ANANDUnitNo unitNo)
{
  ANANDUnitNo firstVirtualUnitNo, firstReplacementUnitNo;
  ANANDUnitNo lastUnit = unitNo, nextUnitNo;
  ANANDUnitNo chainBound = 0;

  getUnitData(&vol,unitNo,&firstVirtualUnitNo,&firstReplacementUnitNo);
  nextUnitNo = firstReplacementUnitNo;

  while( (nextUnitNo < vol.noOfUnits) &&  /* Validate replacement unit no. */
	 (chainBound < 2*MAX_UNIT_CHAIN) ) {
    ANANDUnitNo nextVirtualUnitNo, nextReplacementUnitNo;

    if( !isAvailable(nextUnitNo) )
      break;
    getUnitData(&vol,nextUnitNo,&nextVirtualUnitNo,&nextReplacementUnitNo);
    if( nextVirtualUnitNo != (firstVirtualUnitNo | ANAND_REPLACING_UNIT) )
      break;        /* Virtual unit no. not validated */
    lastUnit = nextUnitNo;
    nextUnitNo = nextReplacementUnitNo;
    chainBound++;
  }

  return lastUnit;
}

/*----------------------------------------------------------------------*/
/*		           a s s i g n U n i t				*/
/*									*/
/* Assigns a virtual unit no. to a unit					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*	virtualUnitNo	: Virtual unit number to assign			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus assignUnit(Anand vol, ANANDUnitNo unitNo, ANANDUnitNo virtualUnitNo)
{
  ANANDUnitNo newVirtualUnitNo, newReplacementUnitNo;
  ANANDUnitNo oldVirtualUnitNo, oldReplacementUnitNo;
  FLStatus status;
  ANANDUnitNo firstUnitNo = vol.virtualUnits[virtualUnitNo];

  /* Assign the new unit */
  newVirtualUnitNo = virtualUnitNo;
  if (firstUnitNo != ANAND_NO_UNIT)
    newVirtualUnitNo |= ANAND_REPLACING_UNIT;
  newReplacementUnitNo = ANAND_NO_UNIT;
  vol.physicalUnits[unitNo] = 0;
  vol.freeUnits--;
  status = setUnitData(&vol,unitNo,newVirtualUnitNo,newReplacementUnitNo);
  if (status != flOK) {
    markUnitBad(&vol,unitNo);
    return status;
  }

  /* Add unit to chain */
  if (firstUnitNo != ANAND_NO_UNIT) {
    ANANDUnitNo oldUnitNo;

    /* If unit is frozen, don't attempt to chain (folding not-in-place) */
    if (!isAvailable(firstUnitNo))
      return flOK;

    oldUnitNo = lastInChain(&vol,firstUnitNo);
    getUnitData(&vol,oldUnitNo,&oldVirtualUnitNo,&oldReplacementUnitNo);
    if (oldReplacementUnitNo != ANAND_NO_UNIT)
      status = flWriteFault;	/* can't write here, so assume failure */
    else {
      oldReplacementUnitNo = unitNo;
      vol.physicalUnits[oldUnitNo] |= UNIT_REPLACED;
      status = setUnitData(&vol,oldUnitNo,oldVirtualUnitNo,oldReplacementUnitNo);
    }
    if (status != flOK) {
      formatUnit(&vol,unitNo); /* Get rid of the allocated unit quickly */
      setUnavail(firstUnitNo); /* freeze the chain */

      return status;
    }
    if (vol.countsValid > virtualUnitNo && firstUnitNo != oldUnitNo)
      if (countOf(firstUnitNo) + countOf(oldUnitNo) <= UNIT_MAX_COUNT)
	vol.physicalUnits[firstUnitNo] += countOf(oldUnitNo);
      else
        return flGeneralFailure;
  }
  else
    vol.virtualUnits[virtualUnitNo] = unitNo;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           f o r m a t C h a i n			*/
/*									*/
/* Format all the units in a chain. Start from the last one and go	*/
/* backwards until unitNo is reached.					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Format the chain from this unit onwards	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatChain(Anand vol, ANANDUnitNo unitNo)
{
  /* Erase the chain from end to start */
  vol.physicalUnits[unitNo] &= ~UNIT_COUNT;  /* Reenable erase of this unit */
  for (;;) {
    /* Find last unit in chain */
    ANANDUnitNo unitToErase = lastInChain(&vol,unitNo);

    if( formatUnit(&vol,unitToErase) != flOK )
      break;

    if (unitToErase == unitNo)
      break;    /* Erased everything */
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                        S w a p U n i t s                             */
/*									*/
/*----------------------------------------------------------------------*/

FLStatus swapUnits(Anand vol)
{
  ANANDUnitNo i,unitNo,virtualUnitNo,replacementUnitNo;

  if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
    return flOK;

  for(i=0,unitNo=vol.virtualUnits[vol.wearLevel.currUnit];
      (unitNo==ANAND_NO_UNIT) && (i<vol.noOfVirtualUnits);i++) {

    vol.wearLevel.currUnit++;
    if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
      vol.wearLevel.currUnit = 0;

    unitNo=vol.virtualUnits[vol.wearLevel.currUnit];
  }

  if(unitNo==ANAND_NO_UNIT) /*The media is empty*/
    return flOK;

  virtualUnitNo = vol.wearLevel.currUnit;

  vol.wearLevel.currUnit++;
  if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
    vol.wearLevel.currUnit = 0;

  if (!isAvailable(unitNo))
    return foldUnit(&vol,virtualUnitNo);

  if(vol.physicalUnits[unitNo] & UNIT_REPLACED)
    return foldUnit(&vol,virtualUnitNo);
  else {
    checkStatus(allocateUnit(&vol,&replacementUnitNo));
    return assignUnit(&vol,replacementUnitNo,virtualUnitNo);
  }
}

/*----------------------------------------------------------------------*/
/*      	             f o l d U n i t				*/
/*									*/
/* Copy all the sectors that hold valid data in the chain to the last   */
/* unit of the chain and erase the chain. 				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	virtualUnitNo	: Virtual unit number of the first unit in 	*/
/*			  chain.					*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus foldUnit(Anand vol, ANANDUnitNo virtualUnitNo)
{
  ANANDUnitNo unitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo targetUnitNo, chainBound;
  unsigned long foldMark;
  SectorNo virtualSectorNo;
  CardAddress targetSectorAddress;
  unsigned newSectorCount, i;

  vol.unitsFolded++;

/* Find target unit */
  if (!isAvailable(unitNo)) {
			/* If this unit is frozen, */
			/* allocate a new unit to fold into */
    checkStatus(allocateUnit(&vol,&targetUnitNo));
    checkStatus(assignUnit(&vol,targetUnitNo,virtualUnitNo));
  }
  else {		/* Default. Fold into end of chain */
    targetUnitNo = unitNo;

    for (chainBound=0;( chainBound < 2*MAX_UNIT_CHAIN );chainBound++) {
      ANANDUnitNo nextUnitNo = getNextUnit(&vol,targetUnitNo);
      if (nextUnitNo == ANAND_NO_UNIT)
	break;
      targetUnitNo = nextUnitNo;
    }
  }

/* Mark unit as currently folded */
  foldMark = FOLDING_IN_PROGRESS * 0x10001l;


  vol.flash.write(&vol.flash,
		   unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
		   &foldMark,
		   sizeof foldMark,
		   EXTRA);

  setUnavail(unitNo);	/* Freeze this unit chain */

  /* Copy all sectors to target unit */
  virtualSectorNo = (SectorNo)virtualUnitNo * vol.sectorsPerUnit;
  targetSectorAddress = unitBaseAddress(&vol,targetUnitNo);
  newSectorCount = 0;
  for (i = 0; i < vol.sectorsPerUnit; i++, virtualSectorNo++,
       targetSectorAddress += SECTOR_SIZE) {
    CardAddress sourceSectorAddress = virtual2Physical(&vol,virtualSectorNo);
    if (sourceSectorAddress != ANAND_UNASSIGNED_ADDRESS) {
      newSectorCount++;
      if (sourceSectorAddress != targetSectorAddress) {
	checkStatus(copySector(&vol,sourceSectorAddress,targetSectorAddress));
	vol.parasiteWrites++;
      }
    }
  }

  if (newSectorCount > 0) {	/* Some sectors remaining*/
    /* Mark target unit as original */
    checkStatus(setUnitData(&vol,targetUnitNo,virtualUnitNo,ANAND_NO_UNIT));
    /* Set target unit in virtual unit table */
    vol.virtualUnits[virtualUnitNo] = targetUnitNo;
    vol.physicalUnits[targetUnitNo] &= ~UNIT_COUNT;
    vol.physicalUnits[targetUnitNo] |= newSectorCount;
  }
  else {
    if (unitNo != targetUnitNo) {
/*    If there is a chain to delete ... */
/*    mark unit as completed folding, pending erase */
      unsigned long foldMark = FOLDING_COMPLETE * 0x10001l;

      vol.flash.write(&vol.flash,
		       unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
		       &foldMark,
		       sizeof foldMark,
		       EXTRA);
    }

    vol.virtualUnits[virtualUnitNo] = ANAND_NO_UNIT;
  }

  /* Erase source units */

  return formatChain(&vol,unitNo);
}


/*----------------------------------------------------------------------*/
/*		           s e t U n i t C o u n t			*/
/*									*/
/* Count the number of sectors in a unit that hold valid data.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Physical unit number				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setUnitCount(Anand vol, ANANDUnitNo unitNo)
{
  unsigned int i;
  SectorNo sectorNo;
  ANANDUnitNo physicalUnitNo = vol.virtualUnits[unitNo];
  if (physicalUnitNo == ANAND_NO_UNIT)
    return;

  /* Get a count of the valid sector in this unit */
  vol.physicalUnits[physicalUnitNo] &= ~UNIT_COUNT;

  sectorNo = (SectorNo)unitNo * vol.sectorsPerUnit;
  for (i = 0; i < vol.sectorsPerUnit; i++, sectorNo++) {
    CardAddress sectorAddress = virtual2Physical(&vol,sectorNo);
    if (sectorAddress != ANAND_UNASSIGNED_ADDRESS) {
      ANANDUnitNo currUnitNo = (ANANDUnitNo)(sectorAddress >> vol.unitSizeBits);
      if (vol.physicalUnits[currUnitNo] & UNIT_REPLACED)
	currUnitNo = physicalUnitNo;
      vol.physicalUnits[currUnitNo]++;
    }
  }
}


/*----------------------------------------------------------------------*/
/*      	             f o l d B e s t C h a i n			*/
/*									*/
/* Find the best chain to fold and fold it.A good chain to fold is a	*/
/* long chain with a small number of sectors that hold valid data.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Receives the physical unit no. of the first 	*/
/*			  unit in the chain that was folded.		*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus foldBestChain(Anand vol, ANANDUnitNo *unitNo)
{
  unsigned leastCount = vol.sectorsPerUnit + 1;
  unsigned longestChain = 0;
  ANANDUnitNo virtualUnitNo = (ANANDUnitNo)0, u;

  for (u = 0; u < vol.noOfVirtualUnits; u++) {
    if (vol.countsValid <= u) {
      setUnitCount(&vol,u);
      vol.countsValid = u + 1;
    }

    if (vol.virtualUnits[u] != ANAND_NO_UNIT) {
      unsigned char pU = vol.physicalUnits[vol.virtualUnits[u]];
      unsigned unitCount = pU & UNIT_COUNT;

      if (leastCount >= unitCount && ((pU & UNIT_REPLACED) || unitCount == 0)) {
	if (unitCount > 0) {
	  int chainLength = 0;

	  /* Compare chain length */
	  ANANDUnitNo nextUnitNo = getNextUnit(&vol,vol.virtualUnits[u]);
	  while (nextUnitNo != ANAND_NO_UNIT) {
	    chainLength++;
	    nextUnitNo = getNextUnit(&vol,nextUnitNo);
	  }
	  if (leastCount == unitCount && longestChain >= (unsigned)chainLength)
	    continue;
	  longestChain = chainLength;
	}
	leastCount = unitCount;
	virtualUnitNo = u;
	if (leastCount == 0)
	  break;
      }
    }
  }

  if (leastCount > vol.sectorsPerUnit)
    return flNotEnoughMemory;	/* No space at all */
  else {
    *unitNo = vol.virtualUnits[virtualUnitNo];
    return foldUnit(&vol,virtualUnitNo);
  }
}


/*----------------------------------------------------------------------*/
/*      	             a l l o c a t e U n i t			*/
/*									*/
/* Find a free unit to allocate, erase it if necessary.			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/* 	unitNo		: Receives the physical number of the allocated */
/*			  unit 						*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus allocateUnit(Anand vol, ANANDUnitNo *unitNo)
{
  ANANDUnitNo originalUnit = vol.roverUnit;

  if (vol.freeUnits) {
    do {
      unsigned char unitFlags;

      if (++vol.roverUnit >= vol.noOfUnits)
	vol.roverUnit = vol.bootUnits;

      unitFlags = vol.physicalUnits[vol.roverUnit];

      if (unitFlags == ANAND_UNIT_FREE) { /* found a free unit, if not erased, */
	unsigned short eraseMark;
	unsigned long eraseCount;

	getUnitTailer(&vol,vol.roverUnit,&eraseMark,&eraseCount);
	if (eraseMark != ERASE_MARK) {
	  if (formatUnit(&vol,vol.roverUnit) != flOK)
	    continue;	/* this unit is bad, find another */
	}

	*unitNo = vol.roverUnit;
	return flOK;
      }
    } while (vol.roverUnit != originalUnit);
  }

  return foldBestChain(&vol,unitNo);  /* make free units by folding the best chain */
}

/*----------------------------------------------------------------------*/
/*      	             m a p S e c t o r				*/
/*									*/
/* Maps and returns location of a given sector no.			*/
/* NOTE: This function is used in place of a read-sector operation.	*/
/*									*/
/* A one-sector cache is maintained to save on map operations.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to read				*/
/*	physAddress	: Optional pointer to receive sector address	*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to physical sector location. NULL returned if sector	*/
/*	does not exist.							*/
/*----------------------------------------------------------------------*/

static const void FAR0 *mapSector(Anand vol, SectorNo sectorNo, CardAddress *physAddress)
{
  if (sectorNo != vol.mappedSectorNo || vol.flash.socket->remapped) {
    if (sectorNo >= vol.virtualSectors)
      vol.mappedSector = NULL;
    else {
      vol.mappedSectorAddress = virtual2Physical(&vol,sectorNo);

      if (vol.mappedSectorAddress == ANAND_UNASSIGNED_ADDRESS)
	vol.mappedSector = NULL;	/* no such sector */
      else {
	vol.mappedSector = nftlBuffer;
	if (vol.flash.read(&vol.flash,vol.mappedSectorAddress,nftlBuffer,SECTOR_SIZE,EDC) != flOK)
	  return dataErrorToken;
      }
    }
    vol.mappedSectorNo = sectorNo;
    vol.flash.socket->remapped = FALSE;
  }

  if (physAddress)
    *physAddress = vol.mappedSectorAddress;

  return vol.mappedSector;
}


/* Mounting and formatting */

#define UNIT_ORPHAN	0x10

/*----------------------------------------------------------------------*/
/*		           m o u n t U n i t				*/
/*									*/
/* Mount one unit. Read the relevant data from the unit header and 	*/
/* update the conversion tables. 					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: Unit to mount					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus mountUnit(Anand vol, ANANDUnitNo unitNo,
                          unsigned long* eraseCount)
{
  ANANDUnitNo virtualUnitNo, replacementUnitNo;
  unsigned short eraseMark;
  ANANDPhysUnit *pU = &vol.physicalUnits[unitNo];

  getUnitData(&vol,unitNo,&virtualUnitNo,&replacementUnitNo);
  getUnitTailer(&vol,unitNo,&eraseMark,eraseCount);

  if (virtualUnitNo == ANAND_NO_UNIT ||
      eraseMark != ERASE_MARK) {  /* this unit is not assigned */
    *pU = ANAND_UNIT_FREE;
  }
  else {  /* this unit is assigned */
    *pU &= UNIT_ORPHAN;
    if (replacementUnitNo < vol.noOfUnits) {
      *pU |= UNIT_REPLACED;
      if (isAvailable(replacementUnitNo) ||
	  isReplaced(replacementUnitNo))
	/* Mark replacement unit as non-orphan */
	vol.physicalUnits[replacementUnitNo] &= ~UNIT_ORPHAN;
    }
    if (!(virtualUnitNo & ANAND_REPLACING_UNIT)) {
      unsigned short foldMark;
      ANANDUnitNo physUnitNo;

      if (virtualUnitNo >= vol.noOfVirtualUnits)
	return flBadFormat;

      foldMark = getFoldMark(&vol,unitNo);
      physUnitNo = vol.virtualUnits[virtualUnitNo];
      if (foldMark == FOLDING_COMPLETE)
	formatChain(&vol,unitNo);
      else if (physUnitNo == ANAND_NO_UNIT || !isAvailable(physUnitNo)) {
	/* If we have duplicates, it's OK if one of them is currently folded */
	vol.virtualUnits[virtualUnitNo] = unitNo;
	*pU &= ~UNIT_ORPHAN;

	if (foldMark == FOLDING_IN_PROGRESS) {
	  *pU &= ~UNIT_COUNT;
	  *pU |= UNIT_UNAVAIL;
	}
	if (physUnitNo != ANAND_NO_UNIT)
	  formatChain(&vol,physUnitNo);	/* Get rid of old chain */
      }
      else if (foldMark == FOLDING_IN_PROGRESS)
	formatChain(&vol,unitNo);
      else
	return flBadFormat;	/* We have a duplicate to a unit that */
				/* is not currently folded. That's bad. */
    }
  }

  return flOK;
}

/*----------------------------------------------------------------------*/
/*      	     a l l o c a t e A n d W r i t e S e c t o r	*/
/*									*/
/* Write to sectorNo. if necessary, allocate a free sector first.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no. to write			*/
/*	fromAddress	: Address of sector data. 			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus allocateAndWriteSector(void* rec,
				     SectorNo sectorNo,
				     void FAR1 *fromAddress)
{
  Anand vol = (Anand*)rec;
  ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo / vol.sectorsPerUnit);
  ANANDUnitNo firstUnitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo unitNo;
  unsigned unitOffset = (unsigned)((sectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS);
  unsigned unitChainLength = 1;
  FLBoolean sectorExists = FALSE;

  /* If we can't write to this unit, must fold it first */
  if (firstUnitNo != ANAND_NO_UNIT && !isAvailable(firstUnitNo)) {
    checkStatus(foldUnit(&vol,virtualUnitNo));
    firstUnitNo = vol.virtualUnits[virtualUnitNo];
  }

  /* Find a unit to write this sector */

  unitNo = firstUnitNo;
  while ((unitNo != ANAND_NO_UNIT) && (unitChainLength < 2*MAX_UNIT_CHAIN)) {
    unsigned char sectorFlags = getSectorFlags(&vol,unitBaseAddress(&vol,unitNo) + unitOffset);
    if (sectorFlags == SECTOR_FREE)
      break;
    if (sectorFlags != SECTOR_IGNORE)
      sectorExists = sectorFlags == SECTOR_USED;
    unitNo = getNextUnit(&vol,unitNo);
    unitChainLength++;
  }

  if (unitChainLength == 2*MAX_UNIT_CHAIN)       /* unit points to itself */
    unitNo = ANAND_NO_UNIT;                      /* force folding */

  if (unitNo == ANAND_NO_UNIT) {
    if (unitChainLength >= MAX_UNIT_CHAIN)
      checkStatus(foldUnit(&vol,virtualUnitNo));
    checkStatus(allocateUnit(&vol,&unitNo));
    checkStatus(assignUnit(&vol,unitNo,virtualUnitNo));
    firstUnitNo = vol.virtualUnits[virtualUnitNo];

  }

  if (!isAvailable(unitNo))
    return flGeneralFailure;

  checkStatus(writeAndCheck(&vol,unitBaseAddress(&vol,unitNo) + unitOffset,fromAddress,EDC));

  if (vol.countsValid > virtualUnitNo) {
    if (unitNo != firstUnitNo && !(vol.physicalUnits[unitNo] & UNIT_REPLACED)) {
      if (countOf(unitNo) < UNIT_MAX_COUNT)	/* Increment block count */
	vol.physicalUnits[unitNo]++;
      else
	return flGeneralFailure;

      if (sectorExists)	/* Decrement block count */
	if (countOf(firstUnitNo) > 0)
	  vol.physicalUnits[firstUnitNo]--;
	else
	    return flGeneralFailure;
    }
    else if (!sectorExists) {
      if (countOf(firstUnitNo) < UNIT_MAX_COUNT)  /* Increment block count */
	vol.physicalUnits[firstUnitNo]++;
      else
	return flGeneralFailure;
    }
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	          w r i t e S e c t o r				*/
/*									*/
/* Writes a sector.							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no. to write			*/
/*	fromAddress	: Data to write					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus writeSector(Anand vol, SectorNo sectorNo, void FAR1 *fromAddress)
{
  FLStatus status = flWriteFault;
  int i;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo >= vol.virtualSectors)
    return flSectorNotFound;

  if(vol.wearLevel.currUnit!=ANAND_NO_UNIT) {
    vol.wearLevel.alarm++;
    if(vol.wearLevel.alarm>=WLnow) {
      vol.wearLevel.alarm = 0;
      checkStatus(swapUnits(&vol));
    }
  }

  vol.sectorsWritten++;
  for (i = 0; i < 4 && status == flWriteFault; i++) {
    if (vol.mappedSectorNo == sectorNo)
      vol.mappedSectorNo = UNASSIGNED_SECTOR;
    status = allocateAndWriteSector(&vol,sectorNo,fromAddress);
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*      	         d e l e t e S e c t o r			*/
/*									*/
/* Marks contiguous sectors as deleted.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: First sector no. to delete			*/
/*	noOfSectors	: No. of sectors to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus deleteSector(Anand vol, SectorNo sectorNo, SectorNo noOfSectors)
{
  SectorNo iSector;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo + noOfSectors > vol.virtualSectors)
    return flSectorNotFound;

  for (iSector = 0; iSector < noOfSectors; iSector++, sectorNo++,
       vol.sectorsDeleted++) {

    CardAddress sectorAddress = virtual2Physical(&vol,sectorNo);
    if (sectorAddress != ANAND_UNASSIGNED_ADDRESS) {
      unsigned char sectorFlags[2];
      ANANDUnitNo currUnitNo;

      /* Check that the unit is writable, and if not, fold it first */
      ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo / vol.sectorsPerUnit);
      ANANDUnitNo unitNo = vol.virtualUnits[virtualUnitNo];
      if (!isAvailable(unitNo)) {
	checkStatus(foldUnit(&vol,virtualUnitNo));
	sectorAddress = virtual2Physical(&vol,sectorNo);
      }

      /* Mark sector deleted */
      sectorFlags[0] = sectorFlags[1] = SECTOR_DELETED;
#ifdef NFTL_CACHE
      setSectorFlagsCache(&vol, sectorAddress, SECTOR_DELETED);
#endif
      vol.flash.write(&vol.flash,
		       sectorAddress + SECTOR_DATA_OFFSET,
		       &sectorFlags,
		       sizeof sectorFlags,
		       EXTRA);

      currUnitNo = (ANANDUnitNo)(sectorAddress >> vol.unitSizeBits);
      if (vol.physicalUnits[currUnitNo] & UNIT_REPLACED)
	currUnitNo = vol.virtualUnits[virtualUnitNo];
      if (vol.countsValid > virtualUnitNo)
	if (countOf(currUnitNo) > 0)
	  vol.physicalUnits[currUnitNo]--;	/* Decrement block count */
	else
	  return flGeneralFailure;
    }
  }

  return flOK;
}

/*----------------------------------------------------------------------*/
/*      	          t l S e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: ON (1) = operation entry			*/
/*			  OFF(0) = operation exit			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus tlSetBusy(Anand vol, FLBoolean state)
{
  return flOK;
}

#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)

/*----------------------------------------------------------------------*/
/*      	            d e f r a g m e n t				*/
/*									*/
/* Performs unit allocations to arrange a minimum number of writable	*/
/* sectors.                                                             */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorsNeeded	: Minimum required sectors			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus defragment(Anand vol, long FAR2 *sectorsNeeded)
{
  ANANDUnitNo dummyUnitNo, firstFreeUnit = ANAND_NO_UNIT;
  FLBoolean firstRound = TRUE;
  FLStatus status = flOK;

  if( (*sectorsNeeded) == -1 ) { /* fold single chain */
    if (vol.badFormat)
      return flBadFormat;

    status = foldBestChain(&vol,&dummyUnitNo);
    if( (status != flOK) && (vol.freeUnits == 0) )
      return status;
    *sectorsNeeded = vol.freeUnits * vol.sectorsPerUnit;
    return flOK;
  }
  while ((SectorNo)vol.freeUnits * vol.sectorsPerUnit < ((SectorNo)(*sectorsNeeded))) {
    if (vol.badFormat)
      return flBadFormat;

    status = allocateUnit(&vol,&dummyUnitNo);
    if( status != flOK )
      break;
    if (firstRound) {              /* remember the first free unit */
      firstFreeUnit = dummyUnitNo;
      firstRound = FALSE;
    }
    else if (firstFreeUnit == dummyUnitNo) {
      /* We have wrapped around, all the units that were marked as free
	 are now erased, and we still don't have enough space. */
      status = foldBestChain(&vol,&dummyUnitNo); /* make more free units */
      if( status != flOK )
        break;
    }
  }

  *sectorsNeeded = (long)vol.freeUnits * vol.sectorsPerUnit;

  return status;
}

#endif

/*----------------------------------------------------------------------*/
/*      	        s e c t o r s I n V o l u m e			*/
/*									*/
/* Gets the total number of sectors in the volume			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	Number of sectors in the volume					*/
/*----------------------------------------------------------------------*/

static SectorNo sectorsInVolume(Anand vol)
{
  return vol.virtualSectors;
}

/*----------------------------------------------------------------------*/
/*      	         d i s m o u n t N F T L			*/
/*									*/
/* Dismount NFTL volume							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void dismountNFTL(Anand vol)
{
#ifdef MALLOC
  if( vol.physicalUnits != NULL )
    FREE(vol.physicalUnits);
  if( vol.virtualUnits != NULL )
    FREE(vol.virtualUnits);
  vol.physicalUnits = NULL;
  vol.virtualUnits = NULL;

#ifdef NFTL_CACHE
  if( vol.ucache != NULL )
    FREE(vol.ucache);
  if( vol.scache != NULL )
    FREE(vol.scache);
  vol.ucache = NULL;
  vol.scache = NULL;
#endif /* NFTL_CACHE */
#endif /* MALLOC */
}


Anand* getAnandRec(unsigned driveNo)
{
  return (&vols[driveNo]);
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*      	               i s E r a s e d U n i t			*/
/*									*/
/* Check if a unit is erased.                                           */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unitNo		: unit to check					*/
/*                                                                      */
/* Returns:                                                             */
/*	TRUE if unit is erased, FALSE otherwise				*/
/*----------------------------------------------------------------------*/

static FLBoolean isErased(Anand vol, ANANDUnitNo unitNo)
{
  CardAddress addr, offset;
  char ff[SECTOR_SIZE];
  char spareFF[ANAND_SPARE_SIZE];

  tffsset(ff,0xff,sizeof ff);
  for (offset = 0; offset < (1UL << vol.unitSizeBits); offset += SECTOR_SIZE) {
    addr = unitBaseAddress(&vol,unitNo) + offset;
    if (tffscmp( vol.flash.map(&vol.flash, addr, SECTOR_SIZE), ff, sizeof ff ))
      return FALSE;
    vol.flash.read(&vol.flash, addr, (void FAR1 *)spareFF,
                   ANAND_SPARE_SIZE, EXTRA);
    if (tffscmp( spareFF, ff, ANAND_SPARE_SIZE ))
      return FALSE;
  }
  return TRUE;
}

/*----------------------------------------------------------------------*/
/*      	            f o r m a t	N F T L 			*/
/*									*/
/* Perform NFTL Format.							*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume serial no.				*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*	flash		: Flash media mounted on this socket		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatNFTL(unsigned volNo, FormatParams FAR1 *formatParams, FLFlash *flash)
{
  Anand vol = &vols[volNo];
  long int unitSize;
  unsigned long prevVirtualSize;
  ANANDUnitNo iUnit, noOfBootUnits, prevOrgUnit;
  ANANDBootRecord bootRecord;
  int noOfBadUnits = 0;
  FLStatus status;
  FLBoolean forceHeaderUpdate = FALSE;
  static unsigned char checkSum[EXTRA_LEN] =
       { 0x4B, 0x00, 0xE2, 0x0E, 0x93, 0xF7, 0x55, 0x55 };
#ifdef EXTRA_LARGE
  int moreUnitBits;
  unsigned char anandFlagsTmp;
#endif /* EXTRA_LARGE */

  DEBUG_PRINT(("Debug: starting NFTL format.\n"));

  checkStatus(initNFTL(&vol,flash));

  tffsset(&bootRecord,0,sizeof(ANANDBootRecord));

  /* Find the medium boot record */
  for (vol.orgUnit = 0; vol.orgUnit < vol.noOfUnits; vol.orgUnit++) {
    vol.flash.read(&vol.flash,
		   unitBaseAddress(&vol,vol.orgUnit),
		   &bootRecord,
		   sizeof bootRecord,
		   0);
    if (tffscmp(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId) == 0)
      break;
  }

#ifdef EXTRA_LARGE
  if (vol.orgUnit >= vol.noOfUnits) {	/* first time formatting */
    bootRecord.anandFlags = 0xFF;
    moreUnitBits = 0;
    while( ((vol.noOfUnits >> moreUnitBits) > 4096) &&
	   ((vol.unitSizeBits + moreUnitBits) < MAX_UNIT_SIZE_BITS) &&
	   (bootRecord.anandFlags > 0xFC) ) {
      moreUnitBits++;
      bootRecord.anandFlags--;
    }
  }

  moreUnitBits = ~bootRecord.anandFlags & MORE_UNIT_BITS_MASK;
  if (moreUnitBits > 0) {
    vol.unitSizeBits += moreUnitBits;
    vol.noOfUnits >>= moreUnitBits;
    vol.orgUnit >>= moreUnitBits;
  }
#endif /* EXTRA_LARGE */

  if( formatParams->bootImageLen >= 0 )
    noOfBootUnits = (ANANDUnitNo)((formatParams->bootImageLen - 1) >> vol.unitSizeBits) + 1;
  else {
    noOfBootUnits = 0;
    if (vol.orgUnit < vol.noOfUnits) {
      if (LE2(bootRecord.bootUnits) > noOfBootUnits )
        noOfBootUnits = LE2(bootRecord.bootUnits);
    }
  }

  prevOrgUnit = vol.orgUnit;           /* save previous Original Unit */
  prevVirtualSize = UNAL4(bootRecord.virtualMediumSize);
  vol.bootUnits = noOfBootUnits;
  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.noOfTransferUnits = (ANANDUnitNo)formatParams->noOfSpareUnits;

  vol.noOfTransferUnits += (ANANDUnitNo)((long)(vol.noOfUnits - vol.bootUnits) *
				 (100 - formatParams->percentUse) / 100);

  if (vol.noOfUnits <= vol.bootUnits + vol.noOfTransferUnits)
    return flVolumeTooSmall;

  unitSize = 1L << vol.unitSizeBits;
  vol.noOfVirtualUnits = vol.noOfUnits-vol.bootUnits;

  checkStatus(initTables(&vol));

  for (iUnit = 0; iUnit < (vol.noOfUnits-vol.bootUnits); iUnit++)
    vol.virtualUnits[iUnit] = ANAND_NO_UNIT;


  if (vol.orgUnit >= vol.noOfUnits) {
    /* no boot record - virgin card, scan it for bad blocks */
    prevVirtualSize = 0L;

    /* Find a place for the boot record */
    for (vol.orgUnit = vol.bootUnits; vol.orgUnit < vol.noOfUnits; vol.orgUnit++)
      if (isErased(&vol,vol.orgUnit))
        break;

    if (vol.orgUnit >= vol.noOfUnits) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return flVolumeTooSmall;
    }

	    /* Generate the bad unit table */
    /* if a unit is not erased it is marked as bad */
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
      vol.physicalUnits[iUnit] = (unsigned char)(isErased(&vol,iUnit) ? ANAND_UNIT_FREE : UNIT_BAD_ORIGINAL);

  }
  else {  /* Read bad unit table from boot record */
    status = vol.flash.read(&vol.flash,
			       unitBaseAddress(&vol,vol.orgUnit) + SECTOR_SIZE,
			       vol.physicalUnits,
			       vol.noOfUnits * sizeof(ANANDPhysUnit),
			       EDC);
    if( status != flOK ) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }
  }

  /*  count bad units */
  vol.noOfTransferUnits += 2;           /* include orgUnit & spareOrgUnit */

  /* Convert first unit of MDOC of any floor > 0 to BAD in order to make it
   *  unchangable and force internal EEprom mode  */
  if( (((NFDC21Vars *)(vol.flash.mtdVars))->flags & MDOC_ASIC) ) {
    long docFloorSize;
    int  iFloor, noOfFloors, iPage;

    docFloorSize = ((NFDC21Vars *)(vol.flash.mtdVars))->floorSize;
    noOfFloors = ((NFDC21Vars *)(vol.flash.mtdVars))->totalFloors;

    for(iFloor=1;( iFloor < noOfFloors ); iFloor++) {
      iUnit = (ANANDUnitNo)((docFloorSize * (long)iFloor) >> vol.unitSizeBits);
      if( vol.physicalUnits[iUnit] == ANAND_UNIT_FREE ) {
	forceHeaderUpdate = TRUE;           /* force writing of NFTL Header */
	vol.physicalUnits[iUnit] = UNIT_BAD_ORIGINAL; /* mark as BAD */
      }
      status = vol.flash.erase(&vol.flash,
                 iUnit << (vol.unitSizeBits - vol.erasableBlockSizeBits),
	         1 << (vol.unitSizeBits - vol.erasableBlockSizeBits));
      if( status != flOK ) {
        dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }
      for(iPage=0;( iPage < 2 ); iPage++) {
        status = vol.flash.write(&vol.flash,
	           unitBaseAddress(&vol,iUnit) + iPage * SECTOR_SIZE,
		   (const void FAR1 *)checkSum, EXTRA_LEN, EXTRA);
        if( status != flOK ) {
          dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
          return status;
        }
      }
    }
  }

  /* Translate physicalUnits[] to internal representation */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    if (vol.physicalUnits[iUnit] != ANAND_UNIT_FREE)
      vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;
  }

  /* extend bootimage area if there are bad units in it */
  for( iUnit = vol.bootUnits = 0;
       (vol.bootUnits < noOfBootUnits)  &&  (iUnit < vol.noOfUnits);
       iUnit++ )
    if (isAvailable(iUnit))
      vol.bootUnits++;

  if (vol.bootUnits < noOfBootUnits) {
    dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
    return flVolumeTooSmall;
  }

  vol.bootUnits = iUnit;

  if (vol.noOfUnits <= vol.bootUnits + vol.noOfTransferUnits) {
    dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
    return flVolumeTooSmall;
  }

  /* Discount transfer units taken by the boot image */
  for (iUnit = 0; iUnit < vol.bootUnits; iUnit++)
    if (!isAvailable(iUnit))
      vol.noOfTransferUnits--;

  vol.virtualSectors = (SectorNo)((vol.noOfUnits - vol.bootUnits - vol.noOfTransferUnits) *
		       (unitSize / SECTOR_SIZE));
  vol.noOfVirtualUnits = (unsigned short)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

  /* Find a place for the boot records and protect them */
  /* NOTE : We don't erase the old orgUnits, this might cause a problem
     when formatting with bootImageLen = 0 and then formatting with
     bootImageLen = 44Kbyte */
  for (vol.orgUnit = vol.bootUnits; vol.orgUnit < vol.noOfUnits; vol.orgUnit++)
    if (vol.physicalUnits[vol.orgUnit] == ANAND_UNIT_FREE)
      break;
  vol.physicalUnits[vol.orgUnit] = UNIT_UNAVAIL;
  for (vol.spareOrgUnit = vol.orgUnit + 1;
       vol.spareOrgUnit < vol.noOfUnits;
       vol.spareOrgUnit++)
    if (vol.physicalUnits[vol.spareOrgUnit] == ANAND_UNIT_FREE)
      break;
  vol.physicalUnits[vol.spareOrgUnit] = UNIT_UNAVAIL;

  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++) {
    status = formatUnit(&vol,iUnit);
    if(status == flWriteFault) {
      if ((iUnit != vol.orgUnit) && (iUnit != vol.spareOrgUnit)) {
	noOfBadUnits++;
        vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;  /* Mark it bad in table */
	if ((noOfBadUnits+2) >= vol.noOfTransferUnits) {
	  dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
	  return status;
        }
      }
    }
    else if (status != flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    if (formatParams->progressCallback) {
      status = (*formatParams->progressCallback)
		  (vol.noOfUnits - vol.bootUnits,
		  (iUnit + 1) - vol.bootUnits);
      if(status!=flOK) {
	dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }
    }
  }

  /* Prepare the boot record header */
  for(iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {  /* Convert Bad Block table to previous state */
    if( vol.physicalUnits[iUnit] == UNIT_BAD_MOUNT )
      vol.physicalUnits[iUnit] = UNIT_BAD_ORIGINAL;
  }
#ifdef EXTRA_LARGE
  anandFlagsTmp = bootRecord.anandFlags;
#endif /* EXTRA_LARGE */
  tffsset(&bootRecord,0xff,sizeof bootRecord);
#ifdef EXTRA_LARGE
  bootRecord.anandFlags = anandFlagsTmp;
#endif /* EXTRA_LARGE */
  toLE2(bootRecord.noOfUnits,vol.noOfUnits - vol.bootUnits);
  toLE2(bootRecord.bootUnits,vol.bootUnits);
  tffscpy(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId);
  toUNAL4(bootRecord.virtualMediumSize,(CardAddress) vol.virtualSectors * SECTOR_SIZE);

  /* Write boot records, spare unit first */
  vol.physicalUnits[vol.orgUnit] = ANAND_UNIT_FREE;	/* Unprotect it */
  vol.physicalUnits[vol.spareOrgUnit] = ANAND_UNIT_FREE;	/* Unprotect it */

  if( ((prevOrgUnit != vol.orgUnit) || (forceHeaderUpdate == TRUE)) ||
      (prevVirtualSize != UNAL4(bootRecord.virtualMediumSize)) ) {
    status = formatUnit(&vol,vol.spareOrgUnit);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    status = vol.flash.write(&vol.flash,
			        unitBaseAddress(&vol,vol.spareOrgUnit) + SECTOR_SIZE,
			        vol.physicalUnits,
			        vol.noOfUnits * sizeof(ANANDPhysUnit),
                                EDC);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    status = vol.flash.write(&vol.flash,
			        unitBaseAddress(&vol,vol.spareOrgUnit),
			        &bootRecord,
			        sizeof bootRecord,
			        0);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    status = formatUnit(&vol,vol.orgUnit);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    status = vol.flash.write(&vol.flash,
                              unitBaseAddress(&vol,vol.orgUnit) + SECTOR_SIZE,
                              vol.physicalUnits,
                              vol.noOfUnits * sizeof(ANANDPhysUnit),
                              EDC);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    status = vol.flash.write(&vol.flash,
			        unitBaseAddress(&vol,vol.orgUnit),
			        &bootRecord,
			        sizeof bootRecord,
			        0);
    if(status!=flOK) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }
  } /*prevOrgUnit != vol.orgUnit ||
      prevVirtualSize != UNAL4(bootRecord.virtualMediumSize)*/

  /* Erase previous Original and SpareOriginal Unit */
  if( prevOrgUnit < vol.orgUnit )
    for (iUnit = prevOrgUnit; iUnit < vol.orgUnit; iUnit++)
      if( vol.physicalUnits[iUnit] != UNIT_BAD_ORIGINAL )
	 vol.flash.erase(&vol.flash,
	      iUnit << (vol.unitSizeBits - vol.erasableBlockSizeBits),
	      1 << (vol.unitSizeBits - vol.erasableBlockSizeBits));

  /* Protect the units we mustn't access */
  for(iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
    if( vol.physicalUnits[iUnit] == UNIT_BAD_ORIGINAL )
      vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;
    else if( iUnit < vol.bootUnits )
      vol.physicalUnits[iUnit] = UNIT_UNAVAIL;

  vol.physicalUnits[vol.orgUnit] = UNIT_UNAVAIL;
  vol.physicalUnits[vol.spareOrgUnit] = UNIT_UNAVAIL;

  DEBUG_PRINT(("Debug: finished NFTL format.\n"));

  dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
  return flOK;
}

#endif

/*----------------------------------------------------------------------*/
/*                      N F T L I n f o                                 */
/*                                                                      */
/* get NFTL information.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*  volNo       : Volume serial no.                                     */
/*  formatParams    : Address of TLInfo record                          */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus    : 0 on success, failed otherwise                        */
/*----------------------------------------------------------------------*/

static FLStatus  NFTLInfo(Anand vol, TLInfo *tlInfo)
{
  tlInfo->sectorsInVolume = vol.virtualSectors;
  tlInfo->bootAreaSize = (unsigned long)vol.bootUnits << vol.unitSizeBits;
  tlInfo->eraseCycles = vol.eraseSum;
  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	            m o u n t N F T L				*/
/*									*/
/* Mount the volume. Initialize data structures and conversion tables	*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume serial no.				*/
/*	tl		: Mounted translation layer on exit		*/
/*	flash		: Flash media mounted on this socket		*/
/*      volForCallback	: Pointer to FLFlash structure for power on	*/
/*			  callback routine.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus mountNFTL(unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback)
{
  Anand vol = &vols[volNo];
  ANANDUnitNo iUnit;
  unsigned long currEraseCount,eraseCountSum = 0;
  ANANDBootRecord bootRecord,spareBootRecord;
  FLStatus status;
#ifdef NFTL_CACHE
  unsigned long scacheSize;
#endif /* NFTL_CACHE */
#ifdef EXTRA_LARGE
  int moreUnitBits;
#endif /* EXTRA_LARGE */

  tffsset(&bootRecord,0,sizeof(ANANDBootRecord));

  DEBUG_PRINT(("Debug: starting NFTL mount.\n"));

  checkStatus(initNFTL(&vol,flash));
  *volForCallback = &vol.flash;
  vol.eraseSum = 0;
  /* Find the medium boot record */
  for (vol.orgUnit = 0; vol.orgUnit < vol.noOfUnits; vol.orgUnit++) {
    vol.flash.read(&vol.flash,
		   unitBaseAddress(&vol,vol.orgUnit),
		   &bootRecord,
		   sizeof bootRecord,
		   0);
    if (tffscmp(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId) == 0)
      break;
  }
  if (vol.orgUnit >= vol.noOfUnits) {
    DEBUG_PRINT(("Debug: not NFTL format.\n"));
    return flUnknownMedia;
  }

  for (vol.spareOrgUnit = vol.orgUnit + 1;
       vol.spareOrgUnit < vol.noOfUnits;
       vol.spareOrgUnit++) {
    vol.flash.read(&vol.flash,
		   unitBaseAddress(&vol,vol.spareOrgUnit),
		   &spareBootRecord,
		   sizeof spareBootRecord,
		   0);
    if (tffscmp(spareBootRecord.bootRecordId,"ANAND",sizeof spareBootRecord.bootRecordId) == 0)
      break;
  }
  if (vol.spareOrgUnit >= vol.noOfUnits)
    vol.spareOrgUnit = ANAND_NO_UNIT;

  /* Get media information from unit header */
  vol.noOfUnits = LE2(bootRecord.noOfUnits);
  vol.bootUnits = LE2(bootRecord.bootUnits);
  vol.virtualSectors = (SectorNo)(UNAL4(bootRecord.virtualMediumSize) >> SECTOR_SIZE_BITS);
  vol.noOfUnits += vol.bootUnits;

#ifdef EXTRA_LARGE
  moreUnitBits = ~bootRecord.anandFlags & MORE_UNIT_BITS_MASK;
  if (moreUnitBits > 0) {
    vol.unitSizeBits += moreUnitBits;
    vol.orgUnit >>= moreUnitBits;
    if (vol.spareOrgUnit != ANAND_NO_UNIT)
      vol.spareOrgUnit >>= moreUnitBits;
  }
#endif /* EXTRA_LARGE */

  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.noOfVirtualUnits = (ANANDUnitNo)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

  if(((ANANDUnitNo)(vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) >
      (vol.noOfUnits - vol.bootUnits)) ) {

    if( vol.spareOrgUnit != ANAND_NO_UNIT ) {
       vol.noOfUnits = LE2(spareBootRecord.noOfUnits);
       vol.bootUnits = LE2(spareBootRecord.bootUnits);
       vol.virtualSectors = (SectorNo)(UNAL4(spareBootRecord.virtualMediumSize) >> SECTOR_SIZE_BITS);
       vol.noOfUnits += vol.bootUnits;

#ifdef EXTRA_LARGE
       moreUnitBits = ~spareBootRecord.anandFlags & MORE_UNIT_BITS_MASK;
       if (moreUnitBits > 0) {
	 vol.unitSizeBits += moreUnitBits;
	 vol.orgUnit >>= moreUnitBits;
	 if (vol.spareOrgUnit != ANAND_NO_UNIT)
	    vol.spareOrgUnit >>= moreUnitBits;
       }
#endif /* EXTRA_LARGE */

       vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
       vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
       vol.noOfVirtualUnits = (ANANDUnitNo)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

       if ((ANANDUnitNo)(vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) >
	   (vol.noOfUnits - vol.bootUnits))
	 return flBadFormat;
    }
    else
      return flBadFormat;
  }

  checkStatus(initTables(&vol));

  vol.badFormat = FALSE;

  /* Read bad unit table from boot record */
  status = vol.flash.read(&vol.flash,
			  unitBaseAddress(&vol,vol.orgUnit) + SECTOR_SIZE,
			  vol.physicalUnits,
			  vol.noOfUnits * sizeof(ANANDPhysUnit),
			  EDC);
  if( status != flOK ) {
    if( vol.spareOrgUnit != ANAND_NO_UNIT ) {
      status = vol.flash.read(&vol.flash,
			  unitBaseAddress(&vol,vol.spareOrgUnit) + SECTOR_SIZE,
			  vol.physicalUnits,
			  vol.noOfUnits * sizeof(ANANDPhysUnit),
			  EDC);
      if( status != flOK ) {
	dismountNFTL(&vol); /* Free tables must be done after call to initTables */
	return status;
      }
    }
    else
      return status;
  }
  /* Exclude boot-image units */
  for (iUnit = 0; iUnit < vol.noOfVirtualUnits; iUnit++)
    vol.virtualUnits[iUnit] = ANAND_NO_UNIT;

  /* Translate bad unit table to internal representation */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    /* Exclude bad & protected units */
    if (iUnit < vol.bootUnits || iUnit == vol.orgUnit || iUnit == vol.spareOrgUnit ||
	vol.physicalUnits[iUnit] != ANAND_UNIT_FREE)
      if (vol.physicalUnits[iUnit] != ANAND_UNIT_FREE)
        vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;
      else
        vol.physicalUnits[iUnit] = UNIT_UNAVAIL;
  }

  /* Mount all units */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    if ((vol.physicalUnits[iUnit] != UNIT_UNAVAIL) && (vol.physicalUnits[iUnit] != UNIT_BAD_MOUNT)) {
      status = mountUnit(&vol,iUnit,&currEraseCount);
      if(status!=flOK) {
        dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }
      eraseCountSum+=currEraseCount;
      vol.eraseSum+=currEraseCount;
    }
  }

  /* Scan for orphan units, and count free units */
  vol.freeUnits = 0;
  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++) {
    ANANDPhysUnit *pU = &vol.physicalUnits[iUnit];

    if (*pU == UNIT_ORPHAN ||
	*pU == (UNIT_REPLACED | UNIT_ORPHAN)) {
       formatChain(&vol,iUnit);                 /* Get rid of orphan */
    }
    else
      if (*pU == (ANAND_UNIT_FREE & ~UNIT_ORPHAN))
	*pU = ANAND_UNIT_FREE;	/* Reference to free unit. That's OK */
  }
  /* Calculate Free Units again after formatChain */
  vol.freeUnits = 0;
  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++) {
    if( vol.physicalUnits[iUnit] == ANAND_UNIT_FREE )
      vol.freeUnits++;
  }

  /* Initialize allocation rover */
  vol.roverUnit = vol.bootUnits;

  /* Initialize statistics */
  vol.sectorsRead = vol.sectorsWritten = vol.sectorsDeleted = 0;
  vol.parasiteWrites = vol.unitsFolded = 0;

  tl->rec = &vol;
  tl->mapSector = mapSector;
  tl->writeSector = writeSector;
  tl->deleteSector = deleteSector;
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
  tl->defragment = defragment;
#endif
  tl->sectorsInVolume = sectorsInVolume;
  tl->getTLInfo = NFTLInfo;
  tl->tlSetBusy = tlSetBusy;
  tl->dismount = dismountNFTL;

  tl->writeMultiSector = NULL;
  tl->readSectors = NULL;

  DEBUG_PRINT(("Debug: finished NFTL mount.\n"));

#ifdef NFTL_CACHE

  /* create and initialize ANANDUnitHeader cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif
  {
#ifdef MALLOC
  vol.ucache = (ucacheEntry *) MALLOC(vol.noOfUnits * sizeof(ucacheEntry));
#else
  vol.ucache = vol.ucacheBuf;
#endif /* MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
    vol.ucache = NULL;
#endif
  if (vol.ucache != NULL) {
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
      vol.ucache[iUnit].virtualUnitNo     = 0xDEAD;
      vol.ucache[iUnit].replacementUnitNo = 0xDEAD;
    }
  }
  else {
    DEBUG_PRINT(("Debug: NFTL runs without U-cache\n"));
  }

  /* create and initialize SectorFlags cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif
  {
  scacheSize = (unsigned long)vol.noOfUnits << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2);
#ifdef MALLOC
  if( (sizeof(unsigned) < sizeof(scacheSize)) &&
      (scacheSize >= 0x10000L) )            /* Out of Segment Boundary */
    vol.scache = NULL;
  else
    vol.scache = (unsigned char *) MALLOC(scacheSize);
#else
  vol.scache = vol.scacheBuf;
#endif /* MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
    vol.scache = NULL;
#endif
  if (vol.scache != NULL) {
    /*
     * Whenever SECTOR_IGNORE is found in Sector Flags cache it is double
     * checked by reading actual sector flags from flash. This is way
     * all the cache entries are initially set to SECTOR_IGNORE.
     */
    unsigned char val = (S_CACHE_SECTOR_IGNORE << 6) | (S_CACHE_SECTOR_IGNORE << 4) |
			(S_CACHE_SECTOR_IGNORE << 2) |  S_CACHE_SECTOR_IGNORE;
    unsigned long iC;

    for(iC=0;( iC < scacheSize );iC++)
      vol.scache[iC] = val;
  }
  else {
    DEBUG_PRINT(("Debug: NFTL runs without S-cache\n"));
  }

#endif /* NFTL_CACHE */

  vol.wearLevel.alarm = (unsigned char)(eraseCountSum % WLnow);
  vol.wearLevel.currUnit = (ANANDUnitNo)(eraseCountSum % vol.noOfVirtualUnits);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	         f l R e g i s t e r N F T L			*/
/*									*/
/* Register this translation layer					*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterNFTL(void)
{
#ifdef MALLOC
  unsigned i;
#endif

  if (noOfTLs >= TLS)
    return flTooManyComponents;

  tlTable[noOfTLs].mountRoutine = mountNFTL;

#ifdef FORMAT_VOLUME
  tlTable[noOfTLs].formatRoutine = formatNFTL;
#else
  tlTable[noOfTLs].formatRoutine = noFormat;
#endif
  noOfTLs++;

#ifdef MALLOC
  for(i=0;( i < DRIVES );i++) {
    vols[i].physicalUnits = NULL;
    vols[i].virtualUnits = NULL;
#ifdef NFTL_CACHE
    vols[i].ucache = NULL;
    vols[i].scache = NULL;
#endif /* NFTL_CACHE */
  }
#endif /* MALLOC */
  return flOK;
}
