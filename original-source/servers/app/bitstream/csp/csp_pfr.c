/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ P F R . C *******************************
 *                                                                           *
 * Character shape player portable font resource access functions            *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_PFR.C 7.9 1997/03/18 16:04:30 SHAWN release $
 *                                                                                    
 *     $Log: CSP_PFR.C $
 *     Revision 7.9  1997/03/18 16:04:30  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.8  1997/02/10 22:28:17  JohnC
 *     Added initialization required for interpretation of adjustment vectors in compressed PFRs.
 *     Revision 7.7  1997/01/28 20:10:16  JohnC
 *     Corrected wrong sign for gpsOffset by adding a cast.
 *     Revision 7.6  1997/01/13 15:24:37  JohnC
 *     Fixed bug reading PFR version number
 *     Revision 7.5  1996/11/14 16:55:04  john
 *     Additions to support expansion and buffering of character tables from
 *         compressed PFRs.
 * Revision 7.4  96/11/08  15:07:38  john
 * Streamlined character record size calculation with a table.
 * Modified CspLoadGps() to skip loading if gps buffer already
 *     contains the requested glyph prog string.
 * 
 * Revision 7.3  96/11/06  17:26:57  john
 * Setting up globals related to PFR bitmap handling and hint interpretation
 *     are now conditionally compiled.
 * 
 * Revision 7.2  96/11/06  12:51:15  john
 * Made PFR bitmap handling functionality conditionally compilable.
 * 
 * Revision 7.1  96/10/10  14:05:57  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:58:31  mark
 * Release
 * 
 * Revision 5.7  96/07/02  14:14:01  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.6  96/07/01  17:47:14  shawn
 * Cast nBytes to (fix31) in call to ReadResourceData() from CspGetPfrHeaderData().
 * 
 * Revision 5.5  96/06/10  16:23:51  shawn
 * Cast '43' to long in call to ReadResourceData() from CspGetPfrHeaderData().
 * 
 * Revision 5.4  96/05/31  17:14:05  john
 * Added support for stem snap tables in PFR.
 * 
 * Revision 5.3  96/04/29  17:31:54  john
 * Updates to eliminate warning messages.
 * 
 * Revision 5.2  96/03/26  18:07:09  john
 * Bug fix: Moved setting of LARGEPFTS flag to point
 *     before call to set up phys font tables. This
 *     is needed to correctly handle phys fonts larger
 *     than 64K. Could happen with lotsa bitmaps.
 * Added FONT_SUPPLEMENT flag to fontInfo to indicate that
 *     the font is a supplement to one in an earlier PFR.
 * 
 * Revision 5.1  96/03/14  14:21:32  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:24  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:30:09  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:59  mark
 * Release
 * 
 * Revision 1.5  95/12/13  09:10:46  john
 * Fixed crash on PFR version > 3
 * 
 * Revision 1.4  95/11/28  09:16:32  john
 * Replaced an empty switch statements which generates
 * bad code with Diab C compiler.
 * 
 * Revision 1.3  95/11/07  13:50:28  john
 * Added space budgeting and allocation for the new oru tables
 * used for transforming interpolated coordinates.
 * 
 * Revision 1.2  95/09/14  15:02:59  john
 * Added setting of default value for blueShift.
 * 
 * Revision 1.1  95/08/10  16:45:31  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/

#ifndef CSP_DEBUG
#define CSP_DEBUG   0
#endif

#if CSP_DEBUG
#include <stdio.h>
#endif

#include "csp_int.h"                    /* Private header */ 

#if PROC_TRUEDOC || ! INCL_TPS

#if CSP_DEBUG
#include "csp_dbg.h"
#endif


/* Character record size table */
static ufix8 charRecSizeTable[] =
	{
	4, 5, 6, 7, 5, 6, 7,  8,
	5, 6, 7, 8, 6, 7, 8,  9,
	5, 6, 7, 8, 6, 7, 8,  9,
	6, 7, 8, 9, 7, 8, 9, 10
	};


/***** Local function prototypes *****/
LOCAL_PROTO
int SetupPfrTable(
    cspGlobals_t *pCspGlobals,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[]);

LOCAL_PROTO
int SetupLogFontDir(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
int SetupLogFonts(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData);

LOCAL_PROTO
int SetupPhysFonts(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData);

LOCAL_PROTO
int SetupGlyphProgStrings(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData);

LOCAL_PROTO
int ReadResourceData(
    pfrTable_t *pPfrTable,
    ufix8  *pByte,
    fix31   nBytes,
    fix31   offset
    USERPARAM);

/***** End of local function prototypes *****/

FUNCTION
int CspGetPfrHeaderData(
    short deviceResolution,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData
    USERPARAM)
/*
 *  Checks PFR format and extracts memory size requirements from the specified
 *  PFR access table.
 *  The memory requirements for the logical font directory, blue value tables
 *  and controlled coordinate tables are accumulated over all PFRs.
 *  The memory requirements for buffering logical fonts, physical fonts and 
 *  glyph program strings are accumulated over all indirectly accessed PFRs.
 *  The minimum size for scan conversion lists based on the specified device 
 *  resolution.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_PFR_FORMAT_ERR       4  Portable Font Resource format error
 */
{
fix15   ii;
ufix8   buffer[43];
ufix8  *pByte;
pfrTable_t pfrTable[1];
ufix8	pfrType;
ufix16  version;
fix15   headerSize;
ufix16  logFontMaxSize;
fix31   logFontSectionSize;
fix31   physFontMaxSize;
fix31   bctMaxSize;
fix31   bctSetMaxSize;
fix31   pftBctSetMaxSize;
fix31   physFontSectionSize;
ufix16  gpsMaxSize;
fix31   gpsSectionSize;
ufix8   maxBlueValues;
ufix8   maxXorus;
ufix8   maxYorus;
fix15 	nBytes;
ufix8   maxStemSnapVsize;
ufix8   maxStemSnapHsize;
ufix16	maxChars;
int     errCode;

/* Initialize memory requirement data */
pPfrHeaderData->logFontDirSize = 0;
pPfrHeaderData->logFontMaxSize = 0;
pPfrHeaderData->logFontSectionSize = 0;
pPfrHeaderData->physFontMaxSize = 0;
pPfrHeaderData->bctMaxSize = 0;
pPfrHeaderData->bctSetMaxSize = 0;
pPfrHeaderData->pftBctSetMaxSize = 0;
pPfrHeaderData->physFontSectionSize = 0;
pPfrHeaderData->gpsMaxSize = 0;
pPfrHeaderData->gpsSectionSize = 0;
pPfrHeaderData->maxBlueValues = 0;
pPfrHeaderData->maxXorus = 0;
pPfrHeaderData->maxYorus = 0;
pPfrHeaderData->totalPhysFonts = 0;
pPfrHeaderData->maxStemSnapVsize = 0;
pPfrHeaderData->maxStemSnapHsize = 0;
pPfrHeaderData->maxChars = 0;

/* Accumulate memory requirements for each PFR */
for (ii = 0; ii < nPfrs; ii++)  /* For each PFR... */
    {
    /* Set up temporary PFR access table entry for this PFR */
    pfrTable[0].mode = pfrAccessTable[ii].mode;
    pfrTable[0].access = pfrAccessTable[ii].access;

    /* Read basic PFR header into header buffer */
    errCode = ReadResourceData(
        pfrTable,
        buffer,
        43L,
        0L
#if REENTRANT
        , userParam
#endif
        );
    if (errCode != 0)
        {
        return errCode;
        }
    pByte = buffer;


    /* Check PFR format is valid and get version and header size */
    if (NEXT_BYTE(pByte) != 'P')
        return CSP_PFR_FORMAT_ERR;
    if (NEXT_BYTE(pByte) != 'F')
        return CSP_PFR_FORMAT_ERR;               
    if (NEXT_BYTE(pByte) != 'R')
        return CSP_PFR_FORMAT_ERR;
    pfrType = NEXT_BYTE(pByte) - '0';
    if (pfrType != 0)
    	{
#if INCL_COMPRESSED_PFR
		if (pfrType != 1)
#endif
        return CSP_PFR_FORMAT_ERR;
        }
    version = (ufix16)NEXT_WORD(pByte);
    if (NEXT_BYTE(pByte) != 13)
        return CSP_PFR_FORMAT_ERR;
    if (NEXT_BYTE(pByte) != 10)
        return CSP_PFR_FORMAT_ERR;
    headerSize = NEXT_WORD(pByte);
    if (headerSize < 43)
        return CSP_PFR_FORMAT_ERR;

    /* Determine PFR buffering requirements */
    pPfrHeaderData->logFontDirSize += (ufix16)(NEXT_WORD(pByte) - 2);
    pByte += 2; /* Skip offset to logical font directory */
    logFontMaxSize = (ufix16)NEXT_WORD(pByte);
    logFontSectionSize = NEXT_LONG(pByte);
    pByte += 3; /* Skip first logical font offset */
    physFontMaxSize = (fix31)((ufix16)NEXT_WORD(pByte));
    physFontSectionSize = NEXT_LONG(pByte);
    pByte += 3; /* Skip offset to first physical font record */
    gpsMaxSize = (ufix16)NEXT_WORD(pByte);
    gpsSectionSize = NEXT_LONG(pByte);
    pByte += 3; /* Skip offset to first glyph program string */

    maxBlueValues = NEXT_BYTE(pByte);
    if (maxBlueValues > pPfrHeaderData->maxBlueValues)
        pPfrHeaderData->maxBlueValues = maxBlueValues;

    maxXorus = NEXT_BYTE(pByte);
    if (maxXorus > pPfrHeaderData->maxXorus)
        pPfrHeaderData->maxXorus = maxXorus;

    maxYorus = NEXT_BYTE(pByte);
    if (maxYorus > pPfrHeaderData->maxYorus)
        pPfrHeaderData->maxYorus = maxYorus;

    physFontMaxSize += (fix31)NEXT_BYTE(pByte) << 16;

    pPfrHeaderData->bmapFlags = NEXT_BYTE(pByte);

    bctMaxSize = 0;
    bctSetMaxSize = 0;
    pftBctSetMaxSize = physFontMaxSize;

    nBytes = headerSize - 43;
    if (nBytes > 0)
        {
        /* Limit extra bytes to supported fields */
        if (nBytes > 15)
            nBytes = 15;

        /* Read the extra data into the buffer */
        errCode = ReadResourceData(
            pfrTable,
            buffer,
            (fix31)nBytes,
            43L
#if REENTRANT
            , userParam
#endif
            );
        if (errCode != 0)
            {
            return errCode;
            }
        pByte = buffer;

        /* Extract the 3 version 2 fields (present if any extra data) */
        bctMaxSize = NEXT_LONG(pByte);
        bctSetMaxSize = NEXT_LONG(pByte);
        pftBctSetMaxSize = NEXT_LONG(pByte);

        /* Extract the physical font count (if present) */
        if (nBytes >= 11)
            {
            pPfrHeaderData->totalPhysFonts += NEXT_WORD(pByte);
            }

        /* Extract the stem snap table size info (if present) */
        if (nBytes >= 13)
            {
            maxStemSnapVsize = NEXT_BYTE(pByte);
            if (maxStemSnapVsize > pPfrHeaderData->maxStemSnapVsize)
                pPfrHeaderData->maxStemSnapVsize = maxStemSnapVsize;
            maxStemSnapHsize = NEXT_BYTE(pByte);
            if (maxStemSnapHsize > pPfrHeaderData->maxStemSnapHsize)
                pPfrHeaderData->maxStemSnapHsize = maxStemSnapHsize;
            }
        
        /* Extract maximum number of characters in any one font */
        if (nBytes >= 15)
        	{
        	maxChars = (ufix16)NEXT_WORD(pByte);
        	if (maxChars > pPfrHeaderData->maxChars)
        		pPfrHeaderData->maxChars = maxChars;
        	}
        }

    if (pfrAccessTable[ii].mode != PFR_ACCESS_DIRECT)
        {
        if (logFontMaxSize > pPfrHeaderData->logFontMaxSize)
            pPfrHeaderData->logFontMaxSize = logFontMaxSize;
        pPfrHeaderData->logFontSectionSize += logFontSectionSize;
        if (physFontMaxSize > pPfrHeaderData->physFontMaxSize)
            pPfrHeaderData->physFontMaxSize = physFontMaxSize;
        pPfrHeaderData->physFontSectionSize += physFontSectionSize;
        if (bctMaxSize > pPfrHeaderData->bctMaxSize)
            pPfrHeaderData->bctMaxSize = bctMaxSize;
        if (bctSetMaxSize > pPfrHeaderData->bctSetMaxSize)
            pPfrHeaderData->bctSetMaxSize = bctSetMaxSize;
        if (pftBctSetMaxSize > pPfrHeaderData->pftBctSetMaxSize)
            pPfrHeaderData->pftBctSetMaxSize = pftBctSetMaxSize;
        if (gpsMaxSize > pPfrHeaderData->gpsMaxSize)
            pPfrHeaderData->gpsMaxSize = gpsMaxSize;
        pPfrHeaderData->gpsSectionSize += gpsSectionSize;
        }
    }

/* Ensure max gps size is not greater than gps section size */
if ((fix31)pPfrHeaderData->gpsMaxSize > pPfrHeaderData->gpsSectionSize)
    {
    pPfrHeaderData->gpsMaxSize = (ufix16)pPfrHeaderData->gpsSectionSize;
    }

/* Compute minimum number of scan conversion list elements */
if (deviceResolution == 0)
    {
    deviceResolution = DEFAULT_DEVICE_RES;
    }
pPfrHeaderData->minListElements = 
    deviceResolution * MIN_LIST_SIZE_FACTOR;

#if CSP_DEBUG >= 2
ShowPfrHeaderData(pPfrHeaderData);
#endif

return CSP_NO_ERR;
}

FUNCTION 
void CspDoMemoryBudget(
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData,
    fix31 *pMinBuffSize,
    fix31 *pNormalBuffSize,
    fix31 *pMaxBuffSize)
/*
 *  Determines the memory requirements for execution of the portable font
 *  resource with the specified header and for the specified output resolution.
 *  Sets:
 *      The minimum memory required for PFR execution
 *      The normal amount of memory for efficient PFR execution
 *      The maximum amount of memory for optimum PFR execution
 *  Allows for worst-case memory wastage due to structure alignment.
 */
{
fix31   size;

size = 0;

/* Allow space for global data structure */
size += 
    sizeof(cspGlobals_t) + 
    STRUCTALIGN - 1;

/* Allow space for PFR access table */
size += 
    nPfrs * sizeof(pfrTable_t) + 
    STRUCTALIGN - 1;

/* Allow space for complete logical font directory */
size += 
    pPfrHeaderData->logFontDirSize + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of blue values */
size += 
    pPfrHeaderData->maxBlueValues * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of adjusted blue values */
size += 
    pPfrHeaderData->maxBlueValues * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of stemSnapV values */
size += 
    pPfrHeaderData->maxStemSnapVsize * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of stemSnapH values */
size += 
    pPfrHeaderData->maxStemSnapHsize * sizeof(fix15) + 
    STRUCTALIGN - 1;

#if INCL_CSPHINTS

/* Allow space for maximum number of blue zones */
size += 
    (pPfrHeaderData->maxBlueValues >> 1) * sizeof(blueZone_t) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of stemSnapV zones */
size += 
    (pPfrHeaderData->maxStemSnapVsize + 1) * sizeof(stemSnapZone_t) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of stemSnapH zones */
size += 
    (pPfrHeaderData->maxStemSnapHsize + 1) * sizeof(stemSnapZone_t) + 
    STRUCTALIGN - 1;

#endif

/* Allow space for maximum number of controlled X coordinates */
size += 
    pPfrHeaderData->maxXorus * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for unadjusted controlled X coordinates */
#if INCL_COMPRESSED_PFR
size += 
	pPfrHeaderData->maxXorus * sizeof(fix15) +
    STRUCTALIGN - 1;
#endif

/* Allow space for largest X pixel table */
size += 
    pPfrHeaderData->maxXorus * sizeof(fix15) + 
    STRUCTALIGN - 1 ;

/* Allow space for largest X interpolation break table */
size += 
    (pPfrHeaderData->maxXorus + 1) * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for largest X interpolation multiplier table */
size += 
    (pPfrHeaderData->maxXorus + 1) * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for largest X interpolation offset table */
size += 
    (pPfrHeaderData->maxXorus + 1) * sizeof(fix31) + 
    STRUCTALIGN - 1;

/* Allow space for maximum number of controlled Y coordinates */
size += 
    pPfrHeaderData->maxYorus * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for unadjusted controlled Y coordinates */
#if INCL_COMPRESSED_PFR
size += 
	pPfrHeaderData->maxYorus * sizeof(fix15) +
    STRUCTALIGN - 1;
#endif

/* Allow space for largest Y pixel table */
size += 
    pPfrHeaderData->maxYorus * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for largest Y interpolation break table */
size += 
    (pPfrHeaderData->maxYorus + 1) * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for largest Y interpolation multiplier table */
size += 
    (pPfrHeaderData->maxYorus + 1) * sizeof(fix15) + 
    STRUCTALIGN - 1;

/* Allow space for largest Y interpolation offset table */
size += 
    (pPfrHeaderData->maxYorus + 1) * sizeof(fix31) + 
    STRUCTALIGN - 1;

/* Allow space for compressed PFR character table index */
#if INCL_COMPRESSED_PFR
size += 
	pPfrHeaderData->maxChars * sizeof(charRec_t) +
    STRUCTALIGN - 1;
#endif

/* Allow space for dynamic font tables */
#if CSP_MAX_DYNAMIC_FONTS > 0
size += CspSizeNewFontTables(pPfrHeaderData);
#endif

/* Allow space for minimum number of scan conversion list elements */
size += 
    (pPfrHeaderData->minListElements * 
     (sizeof(fix15) + sizeof(fix15) + sizeof(ufix8))) + 
    (STRUCTALIGN - 1) * 3;

/* Allow space for alignment of remaining buffers */
size += (STRUCTALIGN - 1) * 4;

/* Calculate minimum buffer size */
*pMinBuffSize = size +
    pPfrHeaderData->logFontMaxSize +
    pPfrHeaderData->physFontMaxSize +
    pPfrHeaderData->bctMaxSize +
    pPfrHeaderData->gpsMaxSize;

/* Calculate normal buffer size */
*pNormalBuffSize = size +
    pPfrHeaderData->logFontSectionSize +
    pPfrHeaderData->physFontSectionSize +
    pPfrHeaderData->gpsMaxSize;

/* Calculate maximum buffer size */
*pMaxBuffSize = size +
    pPfrHeaderData->logFontSectionSize +
    pPfrHeaderData->physFontSectionSize +
    pPfrHeaderData->gpsSectionSize;
}

FUNCTION 
int CspSetupMemory(
    cspGlobals_t *pCspGlobals,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData)
/*
 *  Sets up the main memory buffer.
 *  Assumes that the global structure has been allocated.
 *  If there is not enough memory to load all PFR data into memory after
 *  all of the essential stuff is allocated, the following fallback strategy 
 *  is used to reduce memory requirements until everything fits.
 *      1. Load each glyph program string dynamically.
 *      2. Load each physical font record (incl all its bitmap character
 *         tables dynamically.
 *      3. Load each bitmap character table dynamically.
 *      4. Load each logical font dynamically.
 *         font records, physi
 *  Any memory remaining when a successful memory model is achieved is used 
 *  to increasing the size of the scan-conversion lists beyond their minimum
 *  size.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
ufix8  *pMemoryOrigin;
fix31   size;
fix31   memAvail;
fix31   nListElements;
int     errCode;

/* Set up the global data structure */
pMemoryOrigin = (ufix8 *)(CspAllocFixedLo(pCspGlobals, 0));
pCspGlobals->pMemoryOrigin = pMemoryOrigin;

pCspGlobals->userPfrCode = -1;
#if INCL_COMPRESSED_PFR
pCspGlobals->adjMode = 0;
#endif

/* Set up the internal PFR table */
errCode = SetupPfrTable(
    pCspGlobals, 
    nPfrs, 
    pfrAccessTable);
if (errCode != 0)
    {
    return errCode;
    }

/* Set up the logical font directory */
errCode = SetupLogFontDir(pCspGlobals);
if (errCode != 0)
    {
    return errCode;
    }

/* Allocate space for maximum number of blue values */
size = pPfrHeaderData->maxBlueValues * sizeof(fix15);
pCspGlobals->fontInfo.pBlueValues = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of adjusted blue values */
size = pPfrHeaderData->maxBlueValues * sizeof(fix15);
pCspGlobals->pAdjBlueValues = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of stemSnapV values */
size = pPfrHeaderData->maxStemSnapVsize * sizeof(fix15);
pCspGlobals->fontInfo.pStemSnapsV = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of stemSnapH values */
size = pPfrHeaderData->maxStemSnapHsize * sizeof(fix15);
pCspGlobals->fontInfo.pStemSnapsH = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

#if INCL_CSPHINTS

/* Allocate space for maximum number of blue zones */
size = (pPfrHeaderData->maxBlueValues >> 1) * sizeof(blueZone_t);
pCspGlobals->pBlueZones = 
    (blueZone_t *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of stemSnapV zones */
size = (pPfrHeaderData->maxStemSnapVsize + 1) * sizeof(stemSnapZone_t);
pCspGlobals->pStemSnapVzones = 
    (stemSnapZone_t *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of stemSnapH zones */
size = (pPfrHeaderData->maxStemSnapHsize + 1) * sizeof(stemSnapZone_t);
pCspGlobals->pStemSnapHzones = 
    (stemSnapZone_t *)CspAllocFixedLo(pCspGlobals, size);

#endif

/* Allocate space for maximum number of controlled X coordinates */
size = pPfrHeaderData->maxXorus * sizeof(fix15);
pCspGlobals->pXorus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for unadjusted controlled X coordinates */
#if INCL_COMPRESSED_PFR
size = pPfrHeaderData->maxXorus * sizeof(fix15);
pCspGlobals->pOrigXorus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);
#endif

/* Allocate space for largest X pixel table */
size = pPfrHeaderData->maxXorus * sizeof(fix15) ;
pCspGlobals->pXpix =  
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest X interpolation break table */
size = pPfrHeaderData->maxXorus * sizeof(fix15);
pCspGlobals->pXintOrus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest X interpolation multiplier table */
size = (pPfrHeaderData->maxXorus + 1) * sizeof(fix15);
pCspGlobals->pXintMult = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest X interpolation offset table */
size = (pPfrHeaderData->maxXorus + 1) * sizeof(fix31);
pCspGlobals->pXintOffset = 
    (fix31 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for maximum number of controlled Y coordinates */
size = pPfrHeaderData->maxYorus * sizeof(fix15);
pCspGlobals->pYorus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for unadjusted controlled Y coordinates */
#if INCL_COMPRESSED_PFR
size = pPfrHeaderData->maxYorus * sizeof(fix15);
pCspGlobals->pOrigYorus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);
#endif

/* Allocate space for largest Y pixel table */
size = pPfrHeaderData->maxYorus * sizeof(fix15);
pCspGlobals->pYpix = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest Y interpolation break table */
size = pPfrHeaderData->maxYorus * sizeof(fix15);
pCspGlobals->pYintOrus = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest Y interpolation multiplier table */
size = (pPfrHeaderData->maxYorus + 1) * sizeof(fix15);
pCspGlobals->pYintMult = 
    (fix15 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for largest Y interpolation offset table */
size = (pPfrHeaderData->maxYorus + 1) * sizeof(fix31);
pCspGlobals->pYintOffset = 
    (fix31 *)CspAllocFixedLo(pCspGlobals, size);

/* Allocate space for compressed PFR character table index */
#if INCL_COMPRESSED_PFR
size = pPfrHeaderData->maxChars * sizeof(charRec_t);
pCspGlobals->pCharTable = 
    (charRec_t *)CspAllocFixedLo(pCspGlobals, size);
#endif

memAvail = CspMemoryLeft(pCspGlobals);

#if CSP_MAX_DYNAMIC_FONTS > 0
memAvail -= CspSizeNewFontTables(pPfrHeaderData);
#endif

/* Try static loading for everything in PFR */
size = 
    pPfrHeaderData->logFontSectionSize + STRUCTALIGN - 1 +
    pPfrHeaderData->physFontSectionSize + STRUCTALIGN - 1 +
    pPfrHeaderData->gpsSectionSize + STRUCTALIGN - 1 +
    (pPfrHeaderData->minListElements * 
     (sizeof(fix15) + sizeof(fix15) + sizeof(ufix8))) +
    (STRUCTALIGN - 1) * 3;
if (size <= memAvail)
    {
    pCspGlobals->modeFlags = 0;
    goto L1;
    }

/* Try dynamic loading for each glyph program string */
size += 
    pPfrHeaderData->gpsMaxSize -
    pPfrHeaderData->gpsSectionSize;
if (size <= memAvail)
    {
    pCspGlobals->modeFlags = DYNGPS;
    goto L1;
    }

/* Try dynamic loading for each physical font with all 
 * its associated bitmap character tables */
size += 
    pPfrHeaderData->pftBctSetMaxSize -
    pPfrHeaderData->physFontSectionSize;
if (size <= memAvail)
    {
    pCspGlobals->modeFlags = DYNGPS + DYNPHYFNT;
    goto L1;
    }

/* Try dynamic loading for each bitmap character table */
size += 
    pPfrHeaderData->physFontMaxSize +
    pPfrHeaderData->bctMaxSize -
    pPfrHeaderData->pftBctSetMaxSize;
if (size <= memAvail)
    {
    pCspGlobals->modeFlags = DYNGPS + DYNPHYFNT + DYNBCT;
    goto L1;
    }

/* Try dynamically loading for each logical font */
size += 
    pPfrHeaderData->logFontMaxSize -
    pPfrHeaderData->logFontSectionSize;
if (size <= memAvail)
    {
    pCspGlobals->modeFlags = DYNGPS + DYNPHYFNT + DYNBCT + DYNLOGFNT;
    goto L1;
    }

/* Insufficient memory in any mode */
return CSP_BUFFER_OVERFLOW_ERR;

L1:
/* Set flag to read 3-byte phys font record sizes if appropriate */
if (pPfrHeaderData->physFontMaxSize >= 65536L)
    {
    pCspGlobals->modeFlags |= LARGEPFTS;
    }

/* Set up logical font buffer */
errCode = SetupLogFonts(pCspGlobals, pPfrHeaderData);
if (errCode != 0)
    return errCode;

/* Set up physical font and bitmap character table buffers */
errCode = SetupPhysFonts(pCspGlobals, pPfrHeaderData);
if (errCode != 0)
    return errCode;

/* Set up glyph program string buffer */
errCode = SetupGlyphProgStrings(pCspGlobals, pPfrHeaderData);
if (errCode != 0)
    return errCode;

/* Set up dynamic font tables */
#if CSP_MAX_DYNAMIC_FONTS > 0
errCode = CspSetupNewFontTables(pCspGlobals, pPfrHeaderData);
if (errCode != 0)
    return errCode;
#endif

/* Allocate remaining space to output module workspace */
#if !INCL_TPS
nListElements = 
    (CspMemoryLeft(pCspGlobals) - 9) /
    (sizeof(fix15) + sizeof(fix15) + sizeof(ufix8));
if (nListElements > 32767)
    nListElements = 32767;
sp_globals.car = CspAllocFixedLo(
    pCspGlobals, 
    sizeof(fix15) * nListElements);
sp_globals.cdr = CspAllocFixedLo(
    pCspGlobals, 
    sizeof(fix15) * nListElements);
sp_globals.inttype = CspAllocFixedLo(
    pCspGlobals, 
    sizeof(ufix8) * nListElements);
sp_globals.max_intercepts = (fix15)nListElements;
if ((sp_globals.car == BTSNULL) ||
    (sp_globals.cdr == BTSNULL) ||
    (sp_globals.inttype == BTSNULL))
    {
    return CSP_BUFFER_OVERFLOW_ERR;
    }
#endif

return CSP_NO_ERR;
}

FUNCTION
static int SetupPfrTable(
    cspGlobals_t *pCspGlobals,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[])
/* 
 *  Reads various information from the header of each PFR and saves it
 *  in the PFR table.
 *  Data set up is:
 *      PFR access mode
 *      PFR access information
 *      PFR version
 *		PFR type (0 = standard; 1 = compressed)
 *      PFR bitmap flags
 *      Number of physical fonts in PFR
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix31   size;
fix15   ii;
ufix8   buffer[4];
fix15   version;
ufix16  nPhysFonts;
int     errCode;

pCspGlobals->nPfrs = nPfrs;
size = nPfrs * sizeof(pfrTable_t);
pCspGlobals->pfrTable = 
    (pfrTable_t *)CspAllocFixedLo(pCspGlobals, size);
nPhysFonts = 0;
for (ii = 0; ii < nPfrs; ii++)
    {
    pCspGlobals->pfrTable[ii].mode = pfrAccessTable[ii].mode;
    pCspGlobals->pfrTable[ii].access = pfrAccessTable[ii].access;

    /* Set up the PFR type and version fields */
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        3L,
        3L
        USERARG);
    if (errCode != 0)
        return errCode;
#if INCL_COMPRESSED_PFR
	pCspGlobals->pfrTable[ii].pfrType = buffer[0] - '0';
#endif    
    version = CspReadWord(buffer + 1);
    pCspGlobals->pfrTable[ii].version = version;

    /* Set up bitmap flags */
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        1L,
        42L
        USERARG);
    if (errCode != 0)
        return errCode;
    pCspGlobals->pfrTable[ii].bmapFlags = buffer[0];

    /* Set up number of physical fonts */
    pCspGlobals->pfrTable[ii].nPhysFonts = 0;
    if (version >= 3)
        {
        errCode = ReadResourceData(
            &(pCspGlobals->pfrTable[ii]),
            buffer,
            2L,
            52L
            USERARG);
        if (errCode != 0)
            return errCode;
        pCspGlobals->pfrTable[ii].nPhysFonts = CspReadWord(buffer);
        }

    nPhysFonts += pCspGlobals->pfrTable[ii].nPhysFonts;
    }

pCspGlobals->nPhysFonts = nPhysFonts;

return CSP_NO_ERR;
}

FUNCTION
static int SetupLogFontDir(
    cspGlobals_t *pCspGlobals)
/* 
 *  Allocates space in the main memory buffer and builds a logical 
 *  font directory in memory.
 *  If multiple PFRs are active, a contiguous logical font directory
 *  is created.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
ufix16  nLogicalFonts;
fix15   ii;
ufix8   buffer[26];
ufix8  *pByte;
ufix8  *pDest;
ufix16  logFontDirSize;
fix31   logFontDirOffset;
int     errCode;

nLogicalFonts = 0;
for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    pCspGlobals->pfrTable[ii].baseFontCode = nLogicalFonts;
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        26L,
        12L
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    logFontDirOffset = (fix31)((ufix16)NEXT_WORD(pByte));
    pByte += 5;
    pCspGlobals->pfrTable[ii].logFontFirstOffset = NEXT_LONG(pByte);
    pByte += 5;
    pCspGlobals->pfrTable[ii].physFontFirstOffset = NEXT_LONG(pByte);
    pByte += 5;
    pCspGlobals->pfrTable[ii].gpsFirstOffset = NEXT_LONG(pByte);
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        2L,
        logFontDirOffset
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    pCspGlobals->pfrTable[ii].nLogicalFonts = (ufix16)NEXT_WORD(pByte);
    nLogicalFonts += pCspGlobals->pfrTable[ii].nLogicalFonts;
    }
pCspGlobals->nLogicalFonts = nLogicalFonts;

/* Allocate space for complete logical font directory */
pCspGlobals->pLogFontDir = (ufix8 *)(CspAllocFixedLo(
    pCspGlobals,
    (fix31)nLogicalFonts * 5));
if (pCspGlobals->pLogFontDir == BTSNULL)
    return CSP_BUFFER_OVERFLOW_ERR;

/* Set up contiguous logical font directory */
pDest = pCspGlobals->pLogFontDir;
for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        4L,
        10L
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    logFontDirSize = (ufix16)NEXT_WORD(pByte);
    logFontDirOffset = (ufix16)NEXT_WORD(pByte);
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        pDest, 
        (fix31)(logFontDirSize - 2),
        logFontDirOffset + 2
        USERARG);
    if (errCode != 0)
        return errCode;
    pDest += logFontDirSize - 2;
    }

#if CSP_DEBUG >= 2
ShowLogFontDir(pCspGlobals);
#endif

return CSP_NO_ERR;
}

FUNCTION
static int SetupLogFonts(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData)
/* 
 *  Allocates space in the main memory buffer for logical font records
 *  loaded from indirectly accessed PFRs.
 *  If dynamic logical font record loading is in effect, the logical
 *  font buffer size is equal to the largest logical font record in any
 *  indirectly-accessed PFR.
 *  If static logical font record loading is in effect, the logical font
 *  buffer size is equal to the sum of the sizes of the logical font sections 
 *  in all indirectly accessed PFRs. All these logical font records are loaded 
 *  into the buffer at this time.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix31   size;
ufix8  *pLogFonts;
fix15   ii;
ufix8   buffer[3];
ufix8  *pByte;
fix31   logFontSectionSize;
fix31   logFontFirstOffset;
int     errCode;

/* Allocate logical font buffer */
size = (pCspGlobals->modeFlags & DYNLOGFNT)?
    pPfrHeaderData->logFontMaxSize:
    pPfrHeaderData->logFontSectionSize;
pLogFonts = (ufix8 *)(CspAllocFixedLo(pCspGlobals, size));
if (pLogFonts == BTSNULL)
    {
    return CSP_BUFFER_OVERFLOW_ERR;
    }

for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        3L,
        16L
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    logFontSectionSize = NEXT_LONG(pByte);
    logFontFirstOffset = pCspGlobals->pfrTable[ii].logFontFirstOffset;

    if (pCspGlobals->pfrTable[ii].mode == PFR_ACCESS_DIRECT)
	    {
        pCspGlobals->pfrTable[ii].pFirstLogFont = 
            (ufix8 *)(pCspGlobals->pfrTable[ii].access.pPfr) + 
            logFontFirstOffset; 
        }
    else if (pCspGlobals->modeFlags & DYNLOGFNT)
        {
        pCspGlobals->pfrTable[ii].pFirstLogFont = pLogFonts;
        }
    else                        /* Static log font buffering? */
        {
        pCspGlobals->pfrTable[ii].pFirstLogFont = pLogFonts;
        errCode = ReadResourceData(
            &(pCspGlobals->pfrTable[ii]),
            pLogFonts, 
            logFontSectionSize,
            logFontFirstOffset
            USERARG);
        if (errCode != 0)
            return errCode;
        pLogFonts += logFontSectionSize;
        }
    }

/* Mark no logical font selected */
pCspGlobals->fontCode = 0xffff;

#if CSP_DEBUG >= 2
ShowPfrTable(pCspGlobals);
#endif

return CSP_NO_ERR;
}

FUNCTION
static int SetupPhysFonts(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData)
/* 
 *  Allocates space in the main memory buffer to store physical font records
 *  loaded from indirectly accessed PFRs.
 *  If static physical font record loading is in effect, the physical font
 *  buffer size is equal to the sum of the sizes of the physical font sections
 *  in each of the indirectly accessed PFRs. All physical font sections are 
 *  loaded into the buffer at this time. Physical font sections include
 *  possible bitmap character tables after the standard character tables
 *  at the end of each physical font record.
 *  If dynamic physical font record loading is in effect, the physical
 *  font buffer size is large enough to hold any physical font record from
 *  any indirectly accessed PFR together with its character table and all 
 *  of its bitmap character tables.
 *  If both dynamic physical font record loading and dynamic bitmap character
 *  table loading are in effect, the physical font buffer size is large enough 
 *  to hold any physical font record from any indirectly accessed PFR together 
 *  with its character table and any one of its bitmap character tables.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix31   buffSize;
ufix8  *pPhysFonts;
fix15   ii;
ufix8   buffer[3];
ufix8  *pByte;
fix31   physFontSectionSize;
fix31   physFontFirstOffset;
int     errCode;

/* Allocate physical font buffer */
if (pCspGlobals->modeFlags & DYNPHYFNT)
    {
    if (pCspGlobals->modeFlags & DYNBCT) /* Dynamic PFT plus one BCT? */
        {
        buffSize = 
            pPfrHeaderData->physFontMaxSize + 
            pPfrHeaderData->bctMaxSize;
        }
    else                        /* Dynamic PFT plus all of its BCTs? */
        {
        buffSize = pPfrHeaderData->pftBctSetMaxSize;
        }
    }
else                            /* All physical fonts loaded? */
    {
    buffSize = pPfrHeaderData->physFontSectionSize;
    }
pPhysFonts = (ufix8 *)(CspAllocFixedLo(pCspGlobals, buffSize));

#if CSP_DEBUG
if (pPhysFonts == BTSNULL)
    {
    printf("SetupPhysFonts: Memory overflow\n");
    return CSP_BUFFER_OVERFLOW_ERR;
    }
#endif

/* Initialize physical font buffer */
for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        3L,
        24L
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    physFontSectionSize = NEXT_LONG(pByte);
    physFontFirstOffset = pCspGlobals->pfrTable[ii].physFontFirstOffset;

    if (pCspGlobals->pfrTable[ii].mode == PFR_ACCESS_DIRECT)
	    {
        pCspGlobals->pfrTable[ii].pFirstPhysFont = 
            (ufix8 *)(pCspGlobals->pfrTable[ii].access.pPfr) + 
            physFontFirstOffset; 
        }
    else if (pCspGlobals->modeFlags & DYNPHYFNT)
        {
        pCspGlobals->pfrTable[ii].pFirstPhysFont = pPhysFonts;
        }
    else                        /* Static phys font buffering? */
        {
        pCspGlobals->pfrTable[ii].pFirstPhysFont = pPhysFonts;
        errCode = ReadResourceData(
            &(pCspGlobals->pfrTable[ii]),
            pPhysFonts, 
            physFontSectionSize,
            physFontFirstOffset
            USERARG);
        if (errCode != 0)
            return errCode;
        pPhysFonts += physFontSectionSize;
        }
    }

/* Mark no physical font selected */
pCspGlobals->physFontOffset = -1;

return CSP_NO_ERR;
}

FUNCTION
static int SetupGlyphProgStrings(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData)
/* 
 *  Allocates space in the main memory buffer to store glyph program strings
 *  loaded from indirectly accessed PFRs.
 *  If dynamic glyph program string loading is in effect, the glyph program
 *  string buffer size is equal to the largest glyph program string (including
 *  all elements of compound glyphs) in any indirectly-accessed PFR.
 *  If static glyph program string loading is in effect, the glyph program
 *  string buffer size is equal to the sum of the sizes of the glyph program 
 *  string sections of each of the indirectly accessed PFRs. All these glyph 
 *  program string sections are loaded into the buffer at this time.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Memory buffer overflow 
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix31   buffSize;
ufix8  *pGlyphProgStrings;
fix15   ii;
ufix8   buffer[3];
ufix8  *pByte;
fix31   gpsSectionSize;
fix31   gpsFirstOffset;
int     errCode;

/* Allocate glyph program string buffer */
buffSize = (pCspGlobals->modeFlags & DYNGPS)?
    (fix31)pPfrHeaderData->gpsMaxSize:
    pPfrHeaderData->gpsSectionSize;
pGlyphProgStrings = (ufix8 *)(CspAllocFixedLo(pCspGlobals, buffSize));

#if CSP_DEBUG
if (pGlyphProgStrings == BTSNULL)
    {
    printf("SetupGlyphProgStrings: Memory overflow\n");
    return CSP_BUFFER_OVERFLOW_ERR;
    }
#endif

for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[ii]),
        buffer,
        3L,
        32L
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = buffer;
    gpsSectionSize = NEXT_LONG(pByte);
    gpsFirstOffset = pCspGlobals->pfrTable[ii].gpsFirstOffset;

    if (pCspGlobals->pfrTable[ii].mode == PFR_ACCESS_DIRECT)
	    {
        pCspGlobals->pfrTable[ii].pFirstGps = 
            (ufix8 *)(pCspGlobals->pfrTable[ii].access.pPfr) + 
            gpsFirstOffset; 
        }
    else if (pCspGlobals->modeFlags & DYNGPS)
        {
        pCspGlobals->pfrTable[ii].pFirstGps = pGlyphProgStrings;
        }
    else                        /* Static phys font buffering? */
        {
        pCspGlobals->pfrTable[ii].pFirstGps = pGlyphProgStrings;
        errCode = ReadResourceData(
            &(pCspGlobals->pfrTable[ii]),
            pGlyphProgStrings, 
            gpsSectionSize,
            gpsFirstOffset
            USERARG);
        if (errCode != 0)
            return errCode;
        pGlyphProgStrings += gpsSectionSize;
        }
    }

/* Mark no glyph program string selected */
pCspGlobals->gpsOffset = -1;

return CSP_NO_ERR;
}

FUNCTION
int CspLoadLogicalFont(
    cspGlobals_t *pCspGlobals,
    fix15 pfrCode,
    ufix16 fontCode)
/*
 *  Makes the specified logical font the current font and extracts its
 *  data fields into global varables.
 *  The font code is interpreted as a global font code.
 *  If the associated physical font is different from the current 
 *  physical font, the new physical font is selected and its data fields 
 *  are extracted into global variables.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
ufix8  *pByte;
fix31   logFontSize;
fix31   logFontOffset;
fix15   ii;
ufix8   format;
fix15   nExtraItems;
fix15   extraItemSize;
fix31   physFontSize;
fix31   physFontOffset;
int     errCode;

/* Check if required logical font already selected */
if (fontCode == pCspGlobals->fontCode)
    {
    return CSP_NO_ERR;
    }

if ((pCspGlobals->pfrTable[pfrCode].mode == PFR_ACCESS_DIRECT) ||
    ((pCspGlobals->modeFlags & DYNLOGFNT) == 0))
    {
    pByte = pCspGlobals->pLogFontDir + fontCode * 5 + 2;
    logFontOffset = NEXT_LONG(pByte);  /* Read offset for required font */
    pByte = 
        pCspGlobals->pfrTable[pfrCode].pFirstLogFont +
        (logFontOffset - pCspGlobals->pfrTable[pfrCode].logFontFirstOffset);
    }
else                            /* Dynamic logical font loading? */
    {
    pByte = pCspGlobals->pLogFontDir + fontCode * 5;
    logFontSize = (fix31)((ufix16)NEXT_WORD(pByte));
    logFontOffset = NEXT_LONG(pByte);
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[pfrCode]),
        pCspGlobals->pfrTable[pfrCode].pFirstLogFont,
        logFontSize,
        logFontOffset
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = pCspGlobals->pfrTable[pfrCode].pFirstLogFont;
    }

/* Extract font matrix */
for (ii = 0; ii < 4; ii++)
    {
    pCspGlobals->fontAttributes.fontMatrix[ii] = 
        (long)(NEXT_LONG(pByte)) << 8;
    }

/* Extract font attributes */
format = NEXT_BYTE(pByte);
if (format & BIT_2)             /* Stroked style? */
    {
    pCspGlobals->fontAttributes.fontStyle = STROKED_STYLE;
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.strokeThickness = 
        (format & BIT_3)?
        (short)NEXT_WORD(pByte):
        (short)NEXT_BYTE(pByte);
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType = 
        (short)(format & 0x03);
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit = 
        (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType == 
         MITER_LINE_JOIN)?
        (long)NEXT_LONG(pByte):
        0L;
    }
else if (format & BIT_4)        /* Bold style? */
    {
    pCspGlobals->fontAttributes.fontStyle = BOLD_STYLE;
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsBold.boldThickness = 
        (format & BIT_5)?
        (short)NEXT_WORD(pByte):
        (short)NEXT_BYTE(pByte);
    }
else                            /* Filled style? */
    {
    pCspGlobals->fontAttributes.fontStyle = FILLED_STYLE;
    }

/* Skip over extra data in logical font record expansion joint */
if (format & BIT_6)             /* Extra items present? */
    {
    nExtraItems = (fix15)NEXT_BYTE(pByte);
    for (ii = 0; ii < nExtraItems; ii++)
        {
        extraItemSize = NEXT_BYTE(pByte);
        pByte += extraItemSize + 1;
        }
    }

/* Make this the current logical font */
pCspGlobals->fontCode = fontCode;

/* Get size and offset of associated physical font */
physFontSize = (fix31)((ufix16)NEXT_WORD(pByte));
physFontOffset = NEXT_LONG(pByte);
if (pCspGlobals->modeFlags & LARGEPFTS)
    {
    physFontSize += (fix31)NEXT_BYTE(pByte) << 16;
    }

/* Select associated physical font */
errCode = CspLoadPhysicalFont(
    pCspGlobals,
    pfrCode, 
    physFontOffset, 
    physFontSize);

return errCode;
}

FUNCTION
int CspLoadPhysicalFont(
    cspGlobals_t *pCspGlobals,
    fix15 pfrCode,
    fix31 physFontOffset,
    fix31 physFontSize)
/*
 *  Selects the specified physical font loading it into the physical font
 *  buffer if necessary. Extracts all data fields into global variables.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
ufix8  *pByte, *pByte1;
fix15   ii, jj;
ufix8   format;
ufix8	sizeField;
fix15   nExtraItems;
fix15   extraItemSize;
fix31   bctSetSize;
int     errCode;

/* Exit if requested physical font is already selected */
if ((pfrCode == pCspGlobals->physFontPfrCode) &&
    (physFontOffset == pCspGlobals->physFontOffset))
    {
    return CSP_NO_ERR;
    }

/* Point to new physical font record (loading it if necessary) */
if ((pCspGlobals->pfrTable[pfrCode].mode == PFR_ACCESS_DIRECT) ||
    ((pCspGlobals->modeFlags & DYNPHYFNT) == 0))
    {
    pByte = 
        pCspGlobals->pfrTable[pfrCode].pFirstPhysFont +
        (physFontOffset - pCspGlobals->pfrTable[pfrCode].physFontFirstOffset); 
    }
else                            /* Dynamic physical font loading? */
    {
    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[pfrCode]),
        pCspGlobals->pfrTable[pfrCode].pFirstPhysFont,
        physFontSize,
        physFontOffset
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = pCspGlobals->pfrTable[pfrCode].pFirstPhysFont;
    }

/* Get font reference number */
pCspGlobals->fontRefNumber = (ufix16)NEXT_WORD(pByte);

/* Get outline resolution */
pCspGlobals->fontInfo.outlineResolution = NEXT_WORD(pByte);

/* Get metrics resolution */
pCspGlobals->fontInfo.metricsResolution = NEXT_WORD(pByte);

/* Get font bounding box */
pCspGlobals->fontInfo.fontBBox.xmin = (short)NEXT_WORD(pByte);
pCspGlobals->fontInfo.fontBBox.ymin = (short)NEXT_WORD(pByte);
pCspGlobals->fontInfo.fontBBox.xmax = (short)NEXT_WORD(pByte);
pCspGlobals->fontInfo.fontBBox.ymax = (short)NEXT_WORD(pByte);

format = NEXT_BYTE(pByte);

pCspGlobals->verticalEscapement = ((format & BIT_0) != 0);
pCspGlobals->standardSetWidth = (format & BIT_2)?
    0:
    NEXT_WORD(pByte);

/* Process extra data in physical font record expansion joint */
pCspGlobals->pPftExtraItems = BTSNULL;
pCspGlobals->fontInfo.pFontID = BTSNULL;
pCspGlobals->fontInfo.nStemSnapsV = 0;
pCspGlobals->fontInfo.nStemSnapsH = 0;
pCspGlobals->modeFlags &= ~BMAPS_AVAIL; /* Clear bitmaps available flag */
if (format & BIT_7)             /* Extra items present? */
    {
    pCspGlobals->pPftExtraItems = pByte;
    nExtraItems = (fix15)NEXT_BYTE(pByte);
    for (ii = 0; ii < nExtraItems; ii++)
        {
        extraItemSize = NEXT_BYTE(pByte);
        switch(NEXT_BYTE(pByte))
            {
        case 1:		/* Bitmap info */
            pCspGlobals->modeFlags |= BMAPS_AVAIL;
            break;

        case 2:		/* Font ID */
            pCspGlobals->fontInfo.pFontID = (char *)pByte;
            break;

		case 3:		/* Stem snap tables */
			pByte1 = pByte;
			sizeField = NEXT_BYTE(pByte1);
			pCspGlobals->fontInfo.nStemSnapsV = (sizeField >> 4) & 0x0f;
			for (jj = 0; jj < pCspGlobals->fontInfo.nStemSnapsV; jj++)
				{
				pCspGlobals->fontInfo.pStemSnapsV[jj] = NEXT_WORD(pByte1);
				}
			pCspGlobals->fontInfo.nStemSnapsH = sizeField & 0x0f;
			for (jj = 0; jj < pCspGlobals->fontInfo.nStemSnapsH; jj++)
				{
				pCspGlobals->fontInfo.pStemSnapsH[jj] = NEXT_WORD(pByte1);
				}
			break;
			
        default:
            break;
            }
        pByte += extraItemSize;
        }
    }

/* Skip over auxiliary data */
pCspGlobals->fontInfo.nAuxBytes = NEXT_LONG(pByte);
pCspGlobals->fontInfo.pAuxData = pByte;
pByte += pCspGlobals->fontInfo.nAuxBytes;

/* Get blue values */
pCspGlobals->fontInfo.nBlueValues = NEXT_BYTE(pByte);
for (ii = 0; ii < pCspGlobals->fontInfo.nBlueValues; ii++)
    {
    pCspGlobals->fontInfo.pBlueValues[ii] = NEXT_WORD(pByte);
    }

/* Get blue fuzz value */
pCspGlobals->fontInfo.blueFuzz = (fix15)NEXT_BYTE(pByte);

/* Get blue scale value */
pCspGlobals->fontInfo.blueScale = (fix15)NEXT_BYTE(pByte);

/* Set default value for blue shift */
pCspGlobals->fontInfo.blueShift = (fix15)(
    (((fix31)BLUE_SHIFT * 
      pCspGlobals->fontInfo.outlineResolution) + 
     0x8000L) >>
    16);

/* Get stem snap values */
pCspGlobals->fontInfo.stdVW = NEXT_WORD(pByte);
pCspGlobals->fontInfo.stdHW = NEXT_WORD(pByte);

/* Set up character access constants */
pCspGlobals->fontInfo.nCharacters = 
    (unsigned short)((ufix16)NEXT_WORD(pByte));
pCspGlobals->charFormat = format;
pCspGlobals->pFirstChar = pByte;
pCspGlobals->charRecSize = 
	(fix15)charRecSizeTable[(format >> 1) & 0x1f];

/* Set up pointers to appropritate ExecChar and FindChar functions */
pCspGlobals->ExecChar = CspExecChar;
pCspGlobals->FindChar = CspFindChar;

/* Build unpacked character table if compressed PFR */
#if INCL_COMPRESSED_PFR
if (pCspGlobals->pfrTable[pfrCode].pfrType == 1)
	{
	ufix16	defaultCharCode, charCode;
	fix15	defaultSetWidth, setWidth;
	ufix16	gpsSize;
	fix31	defaultGpsOffset, gpsOffset;
	charRec_t *pChar = pCspGlobals->pCharTable;
	
	defaultCharCode = 0;
	defaultSetWidth = 0;
	defaultGpsOffset = 0;

	for (ii = 0; ii < pCspGlobals->fontInfo.nCharacters; ii++)
		{
        ufix8	charFormat;
        
        /* Character record format byte */
        charFormat = NEXT_BYTE(pByte);
        
        /* Character code */
        switch(charFormat & 0x03)
        	{
        case 0:
        	charCode = defaultCharCode;
        	break;
        	
        case 1:
        	charCode = defaultCharCode + (ufix16)NEXT_BYTE(pByte);
        	break;
        	
        case 2:
        	charCode = defaultCharCode + (ufix16)NEXT_WORD(pByte);
        	break;
        	}
        defaultCharCode = charCode + 1;
        
        /* Character escapement */
        switch((charFormat >> 2) & 0x03)
        	{
        case 0:
        	setWidth = defaultSetWidth;
        	break;
        	
        case 1:
        	setWidth = defaultSetWidth + (fix15)NEXT_BYTE(pByte);
        	break;
        	
        case 2:
        	setWidth = defaultSetWidth - (fix15)NEXT_BYTE(pByte);
        	break;
        	
        case 3:
        	setWidth = NEXT_WORD(pByte);
        	break;
        	}
        defaultSetWidth = setWidth;
                    
        /* Glyph prog string size */
        switch((charFormat >> 4) & 0x03)
        	{
        case 0:
        	gpsSize = (ufix16)NEXT_BYTE(pByte);
        	break;
        	
        case 1:
        	gpsSize = (ufix16)NEXT_BYTE(pByte) + 256;
        	break;
        	
        case 2:
        	gpsSize = (ufix16)NEXT_BYTE(pByte) + 512;
        	break;
        	
        case 3:
        	gpsSize = (ufix16)NEXT_WORD(pByte);
        	break;
        	}

        /* Glyph prog string offset */
        switch((charFormat >> 6) & 0x03)
        	{
        case 0:
        	gpsOffset = defaultGpsOffset;
        	break;
        	
        case 1:
        	gpsOffset = defaultGpsOffset + (fix31)NEXT_BYTE(pByte);
        	break;
        	
        case 2:
        	gpsOffset = (fix31)((ufix16)NEXT_WORD(pByte));
        	break;
        	
        case 3:
        	gpsOffset = (fix31)NEXT_LONG(pByte);
        	break;
        	}
        defaultGpsOffset = gpsOffset + gpsSize;
        
        pChar->charCode = charCode;
        pChar->setWidth = setWidth;
        pChar->gpsSize = gpsSize;
        pChar->gpsOffset = gpsOffset;
        pChar++;
        }
    pCspGlobals->ExecChar = CspExecCompressedChar;
	pCspGlobals->FindChar = CspFindCompressedChar;
	}
#endif
	
/* Set up font flags */
#if CSP_MAX_DYNAMIC_FONTS > 0
pCspGlobals->fontInfo.fontFlags = 0;
ii = 
    pCspGlobals->pfrTable[pfrCode].baseFontRefNumber + 
    pCspGlobals->fontRefNumber;
if ((pCspGlobals->pPhysFontTable[ii].nextPhysFont != 0xffff) &&
    (pCspGlobals->userPfrCode < 0))
    pCspGlobals->fontInfo.fontFlags |= CSP_FONT_SUPPLEMENT;
#endif

/* Load all associated bitmap character tables if appropriate */
if ((pCspGlobals->pfrTable[pfrCode].mode != PFR_ACCESS_DIRECT) &&
    ((pCspGlobals->modeFlags & (BMAPS_AVAIL + DYNPHYFNT + DYNBCT)) == 
     (BMAPS_AVAIL + DYNPHYFNT)))
    {
    pByte = pCspGlobals->pPftExtraItems;
    nExtraItems = (fix15)NEXT_BYTE(pByte);
    for (ii = 0; ii < nExtraItems; ii++)
        {
        extraItemSize = NEXT_BYTE(pByte);
        switch(NEXT_BYTE(pByte))
            {
        case 1:
            bctSetSize = NEXT_LONG(pByte);
            errCode = ReadResourceData(
                &(pCspGlobals->pfrTable[pfrCode]),
                (pCspGlobals->pfrTable[pfrCode].pFirstPhysFont + 
                 physFontSize),
                bctSetSize,
                physFontOffset + physFontSize
                USERARG);
            return errCode;

        default:
            pByte += extraItemSize;
            }
        }
    return CSP_PFR_BMAP_ERR;
    }

/* Make this the current physical font */
pCspGlobals->physFontPfrCode = pfrCode;
pCspGlobals->physFontOffset = physFontOffset;
pCspGlobals->physFontSize = physFontSize;

/* Flag no bitmap character table selected */
#if INCL_PFR_BITMAPS
pCspGlobals->ppm = 0;
#endif

return CSP_NO_ERR;
}

#if INCL_PFR_BITMAPS
FUNCTION
int CspLoadBmapCharTable(
    cspGlobals_t *pCspGlobals)
/*
 *  Checks the current physical font for suitable bitmaps for the current 
 *  transformation.
 *  If found, selects the appropriate bitmap character table as the current
 *  bitmap character table and sets the pfr bitmap active flag. 
 *  If dynamic bitmap character table loading is in effect and the current
 *  physical font is in an indirectly accessed PFR, the bitmap character
 *  table is loaded from the PFR into the physical font buffer immediatly
 *  following the physical font itself.
 *  If no suitable bitmaps are available the PFR bitmap active flag is
 *  reset.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
ufix16  xppm, yppm;
ufix32  ppmTarget;
ufix8  *pExtraItems;
fix15   nExtraItems;
fix15   extraItemSize;
fix15   extraItemType;
fix15   ii, jj;
ufix8  *pByte;
fix31   fontBctSize;
ufix8   sizeFormat;
fix15   nBmapSizes;
ufix32  ppm;
ufix8   charFormat;
fix31   bctSize;
fix31   bctOffset;
fix31   nBmapChars;
fix15   pfrCode;
ufix8  *pFirstBmapChar;
int     errCode;

/* Mark bitmaps inactive */
pCspGlobals->modeFlags &= ~BMAPS_ACTIVE;

/* Exit if no bitmaps in current physical font */
if ((pCspGlobals->modeFlags & BMAPS_AVAIL) == 0)
    {
    return CSP_NO_ERR;
    }

/* Test if current transformation can use bitmaps */
switch (sp_globals.tcb.xmode)
    {
case 0:
case 1:
    break;

default:
    return CSP_NO_ERR;
    }

switch (sp_globals.tcb.ymode)
    {
case 0:
case 1:
    break;

default:
    return CSP_NO_ERR;
    }

/* Calculate X and Y pixels per em required */
xppm = (ufix16)(
    (((fix31)sp_globals.tcb.xppo * 
      pCspGlobals->fontInfo.outlineResolution) + 
     pCspGlobals->multrnd) >>
    sp_globals.multshift);   
yppm = (ufix16)(
    (((fix31)sp_globals.tcb.yppo * 
      pCspGlobals->fontInfo.outlineResolution) + 
     pCspGlobals->multrnd) >>
    sp_globals.multshift);   
ppmTarget = ((ufix32)yppm << 16) + xppm;

/* Check if required bitmap table is already selected */
if (ppmTarget == pCspGlobals->ppm)
    {
    pCspGlobals->modeFlags |= BMAPS_ACTIVE;
    return CSP_NO_ERR;
    }

/* Find appropriate bitmap character table */
pExtraItems = pCspGlobals->pPftExtraItems;
if (pExtraItems == BTSNULL)
    {
    return CSP_NO_ERR;
    }
nExtraItems = (fix15)NEXT_BYTE(pExtraItems);
for (ii = 0; ii < nExtraItems; ii++)
    {
    extraItemSize = (fix15)NEXT_BYTE(pExtraItems);
    extraItemType = (fix15)NEXT_BYTE(pExtraItems);
    if (extraItemType == 1)     /* Bitmap size data? */
        {
        fix15   skipBytes;

        pByte = pExtraItems;
        fontBctSize = NEXT_LONG(pByte);
        sizeFormat = NEXT_BYTE(pByte);
        nBmapSizes = (fix15)NEXT_BYTE(pByte);
        skipBytes = 6;
        if (sizeFormat & BIT_2)
            skipBytes++;
        if (sizeFormat & BIT_3)
            skipBytes++;
        if (sizeFormat & BIT_4)
            skipBytes++;
        for (jj = 0; jj < nBmapSizes; jj++)
            {
            xppm = (sizeFormat & BIT_0)?
                (ufix16)NEXT_WORD(pByte):
                (ufix16)NEXT_BYTE(pByte);
            yppm = (sizeFormat & BIT_1)?
                (ufix16)NEXT_WORD(pByte):
                (ufix16)NEXT_BYTE(pByte);
            ppm = ((ufix32)yppm << 16) + xppm;
            if (ppm >= ppmTarget)
                {
                goto L1;
                }
            pByte += skipBytes;
            }
        }
    pExtraItems += extraItemSize;
    }

/* Exit if no bitmap character table available for current size */
return CSP_NO_ERR;

L1:
if (ppm != ppmTarget)
    {
    return CSP_NO_ERR;
    }

/* Get format, size and offset of bitmap character table */
charFormat = NEXT_BYTE(pByte);
bctSize = (sizeFormat & BIT_2)?
    NEXT_LONG(pByte):
    (fix31)((ufix16)NEXT_WORD(pByte));
bctOffset = (sizeFormat & BIT_3)?
    NEXT_LONG(pByte):
    (fix31)((ufix16)NEXT_WORD(pByte));
nBmapChars = (sizeFormat & BIT_4)?
    (fix31)((ufix16)NEXT_WORD(pByte)):
    (fix31)NEXT_BYTE(pByte);

/* Set up pointer to bitmap character table (loading it if necessary) */
pfrCode = pCspGlobals->physFontPfrCode;
if ((pCspGlobals->pfrTable[pfrCode].mode == PFR_ACCESS_DIRECT) ||
    ((pCspGlobals->modeFlags & DYNBCT) == 0))
    {
    pByte = 
        pCspGlobals->pfrTable[pfrCode].pFirstPhysFont +
        (pCspGlobals->physFontOffset - 
         pCspGlobals->pfrTable[pfrCode].physFontFirstOffset + 
         pCspGlobals->physFontSize +
         bctOffset);
    }
 else                            /* Dynamic bitmap char table loading? */
    {
    pFirstBmapChar =
        pCspGlobals->pfrTable[pfrCode].pFirstPhysFont +
        pCspGlobals->physFontSize;

    errCode = ReadResourceData(
        &(pCspGlobals->pfrTable[pfrCode]),
        pFirstBmapChar,
        bctSize,
        (pCspGlobals->physFontOffset +
         pCspGlobals->physFontSize +
         bctOffset)
        USERARG);
    if (errCode != 0)
        return errCode;
    pByte = pFirstBmapChar;
    }

/* Set up bitmap character table access constants */
pCspGlobals->ppm = ppmTarget;
pCspGlobals->nBmapChars = nBmapChars;
pCspGlobals->pFirstBmapChar = pByte;
pCspGlobals->bmapCharFormat = charFormat;
pCspGlobals->bmapCharRecSize = 4;
if (charFormat & BIT_0)         /* 2-byte character IDs? */
    pCspGlobals->bmapCharRecSize += 1;
if (charFormat & BIT_1)         /* 2-byte bitmap gps sizes? */
    pCspGlobals->bmapCharRecSize += 1;
if (charFormat & BIT_2)         /* 3-byte bitmap gps offsets? */
    pCspGlobals->bmapCharRecSize += 1;
pCspGlobals->modeFlags |= BMAPS_ACTIVE;

return CSP_NO_ERR;
}
#endif

FUNCTION
ufix8 *CspLoadGps(
    cspGlobals_t *pCspGlobals,
    ufix16 size,
    fix31 offset)
/*
 *  If the current PFR is directly accessed or if the specified glyph
 *  program string is already loaded, a pointer to it is returned.
 *  If the current PFR is indirectly accessed and dynamic glyph program 
 *  string buffering is in effect, the specified glyph program string 
 *  is loaded into the GPS buffer and a pointer to the first byte of the 
 *  buffer is returned. 
 *
 *  Returns BTSNULL if the required data cannot be loaded
 */
{
fix15   pfrCode;
int     errCode;

pfrCode = pCspGlobals->physFontPfrCode;
if ((pCspGlobals->pfrTable[pfrCode].mode == PFR_ACCESS_DIRECT) ||
    ((pCspGlobals->modeFlags & DYNGPS) == 0))
    {
    return pCspGlobals->pfrTable[pfrCode].pFirstGps + offset;
    }
else                            /* Dynamic glyph prog string loading? */
    {
    if ((pfrCode != pCspGlobals->gpsPfrCode) ||
    	(offset != pCspGlobals->gpsOffset))
    	{
	    errCode = ReadResourceData(
	        &(pCspGlobals->pfrTable[pfrCode]),
	        pCspGlobals->pfrTable[pfrCode].pFirstGps,
	        (fix31)size,
	        pCspGlobals->pfrTable[pfrCode].gpsFirstOffset + offset
	        USERARG);
	    if (errCode != 0)
	        return BTSNULL;
	    pCspGlobals->gpsPfrCode = pfrCode;
	    pCspGlobals->gpsOffset = offset;
    	pCspGlobals->gpsSize = (fix31)size;
    	}
    return pCspGlobals->pfrTable[pfrCode].pFirstGps;
    }
}

FUNCTION
ufix8 *CspAddGps(
    cspGlobals_t *pCspGlobals,
    ufix16 size,
    fix31 offset,
    fix31 buffOffset)
/*
 *  Adds the glyph program string with the specified size from the current 
 *  portable font resource starting at the specified offset to the
 *  set of glyph program strings currently in memory.
 *  If the required record is already in memory, the loading process
 *  is skipped.
 *
 *  Returns a pointer to the first byte of the requested string
 *  (BTSNULL if the required data cannot be loaded)
 */
{
fix15   pfrCode;
int     errCode;

pfrCode = pCspGlobals->physFontPfrCode;
if ((pCspGlobals->pfrTable[pfrCode].mode == PFR_ACCESS_DIRECT) ||
    ((pCspGlobals->modeFlags & DYNGPS) == 0))
    {
    return pCspGlobals->pfrTable[pfrCode].pFirstGps + offset;
    }
else
    {
    if (buffOffset >= pCspGlobals->gpsSize) /* Need to load sub-gps */
        {
        errCode = ReadResourceData(
            &(pCspGlobals->pfrTable[pfrCode]),
            pCspGlobals->pfrTable[pfrCode].pFirstGps + buffOffset,
            (fix31)size,
            pCspGlobals->pfrTable[pfrCode].gpsFirstOffset + offset
            USERARG);
        if (errCode != 0)
            return BTSNULL;
        pCspGlobals->gpsSize += size;
        }
    return pCspGlobals->pfrTable[pfrCode].pFirstGps + buffOffset;
    }
}

FUNCTION
static int ReadResourceData(
    pfrTable_t *pPfrTable,
    ufix8  *pByte,
    fix31   nBytes,
    fix31   offset
    USERPARAM)
/*
 *  Uses the specified PFR access method to read the specified number 
 *  of bytes starting at the specified offset into the specified buffer
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix31   nPart;
fix15   ii;
ufix8  *pSrce;
ufix8  *pDest;
int     errCode;

switch (pPfrTable->mode)
    {
case PFR_ACCESS_INDIRECT:
    while (nBytes > 0)
        {
        nPart = (nBytes <= 32767)? nBytes: 32767;
        errCode = pPfrTable->access.ReadResourceData(
            (void *)pByte,
            (short)nPart, 
            (long)offset 
#if REENTRANT
            , userParam
#endif
            );
        if (errCode != 0)
            {
            return CSP_READ_RESOURCE_ERR;
            }
        nBytes -= nPart;
        offset += nPart;
        pByte += nPart;
        }
    return CSP_NO_ERR;

case PFR_ACCESS_INDIRECT_ALT1:
    while (nBytes > 0)
        {
        nPart = (nBytes <= 32767)? nBytes: 32767;
        errCode = pPfrTable->access.indirectAlt1.ReadResourceData(
            (void *)pByte, 
            (short)nPart, 
            (long)offset,
            (long)pPfrTable->access.indirectAlt1.pfrContext
#if REENTRANT
            , userParam
#endif
            );
        if (errCode != 0)
            {
            return CSP_READ_RESOURCE_ERR;
            }
        nBytes -= nPart;
        offset += nPart;
        pByte += nPart;
        }
    return CSP_NO_ERR;

case PFR_ACCESS_DIRECT:
    pSrce = (ufix8 *)(pPfrTable->access.pPfr) + offset;
    pDest = pByte;
    for (ii = 0; ii < nBytes; ii++)
        {
        *(pDest++) = *(pSrce++);
        }
    return CSP_NO_ERR;

default:
    return CSP_NO_ERR;
    }
}

#endif /*PROC_TRUEDOC*/

