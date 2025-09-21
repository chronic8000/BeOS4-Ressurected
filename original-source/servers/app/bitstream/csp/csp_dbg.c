/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ D B G . C *******************************
 *                                                                           *
 * Character shape player debug printing functions                           *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_DBG.C 7.2 1997/03/18 16:04:22 SHAWN release $
 *                                                                                    
 *     $Log: CSP_DBG.C $
 *     Revision 7.2  1997/03/18 16:04:22  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:04:48  mark
 *     Release
 * Revision 6.1  96/08/27  11:57:22  mark
 * Release
 * 
 * Revision 5.2  96/05/31  17:10:38  john
 * ShowPfrHeaderData() updated to print max size of V and H stem snap tables.
 * 
 * Revision 5.1  96/03/14  14:20:21  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:16  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:55  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:50  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:44:41  john
 * Initial revision
 * 
 * Revision 1.1  95/08/10  11:30:04  john
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

#include "csp_int.h"                    /* Public and internal header */

#if CSP_DEBUG
#include "csp_dbg.h"
#endif


#if CSP_DEBUG
static char *accessMode[] = {"indir0", "indir1", "direct"};


FUNCTION 
void ShowPfrHeaderData(
    pfrHeaderData_t *pPfrHeaderData)
{
printf("\nPortable Font Resource header\n");

printf("    Total size of all logical font directory entries = %ld\n",
    (long)pPfrHeaderData->logFontDirSize);

printf("    Max size of any logical font = %ld\n",
    (long)pPfrHeaderData->logFontMaxSize);

printf("    Total size of all logical font sections = %ld\n",
    (long)pPfrHeaderData->logFontSectionSize);

printf("    Max size of any physical font = %ld\n",
    (long)pPfrHeaderData->physFontMaxSize);

printf("    Max size of any bitmap character table = %ld\n",
    (long)pPfrHeaderData->bctMaxSize);

printf("    Max size of any set of bitmap character tables = %ld\n",
    (long)pPfrHeaderData->bctSetMaxSize);

printf("    Max size of any phys font with bitmap char tables = %ld\n",
    (long)pPfrHeaderData->pftBctSetMaxSize);

printf("    Total size of all physical font sections (incl bmap char tbles) = %ld\n",
    (long)pPfrHeaderData->physFontSectionSize);

printf("    Max size of any glyph prog string = %ld\n",
    (long)pPfrHeaderData->gpsMaxSize);

printf("    Total size of all glyph program string sections = %ld\n",
    (long)pPfrHeaderData->gpsSectionSize);

printf("    Max number of blue values = %ld\n",
    (long)pPfrHeaderData->maxBlueValues);

printf("    Max number of controlled X coordinates = %ld\n",
    (long)pPfrHeaderData->maxXorus);

printf("    Max number of controlled Y coordinates = %ld\n",
    (long)pPfrHeaderData->maxYorus);

printf("    Min number of scan conversion list elements = %ld\n",
    (long)pPfrHeaderData->minListElements);

printf("    Total number of physical fonts = %ld\n",
    (long)pPfrHeaderData->totalPhysFonts);

printf("    Max number of stemSnapV values = %ld\n",
    (long)pPfrHeaderData->maxStemSnapVsize);

printf("    Max number of stemSnapH values = %ld\n",
    (long)pPfrHeaderData->maxStemSnapHsize);
}

FUNCTION
void ShowLogFontDir(
    cspGlobals_t *pCspGlobals)
{
int  ii;
ufix8 *pByte;
ufix16 size;
fix31  offset;

printf("\nLogical Font Directory\n");
printf("    Number of logical fonts = %d\n", 
    pCspGlobals->nLogicalFonts);

if (pCspGlobals->nLogicalFonts > 0)
    {
    printf("    Font   Size Offset\n");
    }

for (ii = 0; ii < pCspGlobals->nLogicalFonts; ii++)
    {
    pByte = pCspGlobals->pLogFontDir + (5 * ii);
    size = (ufix16)NEXT_WORD(pByte);
    offset = NEXT_LONG(pByte);
    printf("    %4d %6d %6ld\n", ii, size, offset);

    }
}

FUNCTION
void ShowPfrTable(
    cspGlobals_t *pCspGlobals)
{
int  ii;

printf("\nPortable Font Resource Table\n");
printf("    Number of PFRs = %d\n", 
    pCspGlobals->nPfrs);

if (pCspGlobals->nPfrs > 0)
    {
    printf("     PFR    mode version    base   nLfts  LftOff   nPfts  PftOff  GpsOff\n");
    }

for (ii = 0; ii < pCspGlobals->nPfrs; ii++)
    {
    printf("    %4d %7s %7d %7d %7d %7ld %7d %7ld %7ld\n", 
        (int)ii, 
        accessMode[pCspGlobals->pfrTable[ii].mode],
        (unsigned int)pCspGlobals->pfrTable[ii].version,
        (unsigned int)pCspGlobals->pfrTable[ii].baseFontCode,
        (unsigned int)pCspGlobals->pfrTable[ii].nLogicalFonts,
        (long)pCspGlobals->pfrTable[ii].logFontFirstOffset,
        (unsigned int)pCspGlobals->pfrTable[ii].nPhysFonts,
        (long)pCspGlobals->pfrTable[ii].physFontFirstOffset,
        (long)pCspGlobals->pfrTable[ii].gpsFirstOffset);

    }
}

FUNCTION
void ShowBmapCharTable(
    cspGlobals_t *pCspGlobals)
/*
 *  Prints out the current bitmap character table
 */
{
ufix8  *pByte;
fix15   ii;
ufix16  charCode;
ufix16  gpsSize;
long    gpsOffset;

pByte = pCspGlobals->pFirstBmapChar;
printf("\nBitmap character table\n");
printf("    Char                  gps     gps \n");
printf("    code                 size  offset\n");
for (ii = 0; ii < pCspGlobals->nBmapChars; ii++)
    {
    charCode = (pCspGlobals->bmapCharFormat & BIT_0)?
    (ufix16)NEXT_WORD(pByte):
        (ufix16)NEXT_BYTE(pByte);
    gpsSize = (pCspGlobals->bmapCharFormat & BIT_1)?
        (ufix16)NEXT_WORD(pByte):
        (ufix16)NEXT_BYTE(pByte);
    gpsOffset = (pCspGlobals->bmapCharFormat & BIT_2)?
        NEXT_LONG(pByte):
        (fix31)((ufix16)NEXT_WORD(pByte));
    printf("   %5hd                %5hd  %6ld  \n",
        charCode, gpsSize, gpsOffset);
    }
}

#if CSP_MAX_DYNAMIC_FONTS > 0
FUNCTION
void ShowPhysFontTable(
    cspGlobals_t *pCspGlobals)
/*
 *  Shows the physical font table generated to
 *  support the creation of new logical fonts at
 *  run time.
 */
{
int  ii;
physFont_t *pPhysFont;

printf("\nPhysical Font Table\n");
printf("    Number of physical fonts = %d\n", 
    pCspGlobals->nPhysFonts);

if (pCspGlobals->nPhysFonts > 0)
    {
    printf("    Font   Next    PFR   Size Offset\n");
    }

for (ii = 0; ii < pCspGlobals->nPhysFonts; ii++)
    {
    pPhysFont = pCspGlobals->pPhysFontTable + ii;
    printf("    %4d %6d %6d %6ld %6ld\n",
        ii,
        (unsigned int)pPhysFont->nextPhysFont,
        (unsigned int)pPhysFont->pfrCode,
        (long)pPhysFont->size,
        (long)pPhysFont->offset);

    }
}
#endif

FUNCTION
void ShowFontIDtable(
    cspGlobals_t *pCspGlobals,
    fix31 fontIDtableSize)
/*
 *  Shows the font ID hash table generated to
 *  support the creation of new logical fonts at
 *  run time.
 */
{
int     ii;

printf("\nFont ID Table\n");
if (fontIDtableSize > 0)
    {
    printf("    Hash   Font\n");
    }

for (ii = 0; ii < fontIDtableSize; ii++)
    {
    printf("    %4d %6d\n",
        ii,
        (unsigned int)pCspGlobals->pFontIDtable[ii]);

    }
}


#endif

