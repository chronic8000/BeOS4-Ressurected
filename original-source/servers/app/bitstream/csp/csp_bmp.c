/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ B M P . C *******************************
 *                                                                           *
 * Character shape player functions for outputting character bitmaps         *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_BMP.C 7.7 1997/03/18 16:04:20 SHAWN release $
 *                                                                                    
 *     $Log: CSP_BMP.C $
 *     Revision 7.7  1997/03/18 16:04:20  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.6  1996/11/08 16:21:03  john
 *     Fixed a crash when cache included but not initialized.
 * Revision 7.5  96/11/07  15:49:09  john
 * Added pixel table lookup to SetBitmapPixels data delivery.
 * SetBitmapPixels data is now inverted if BLACK_PIXEL is zero.
 * 
 * Revision 7.4  96/11/06  12:49:13  john
 * Made PFR bitmap handling functionality conditionally compilable.
 * 
 * Revision 7.3  96/11/05  17:44:32  john
 * Corrected problem in which defined bitmap escapement would
 *     not be used unless an image was generated.
 * 
 * Revision 7.2  96/10/29  17:47:48  john
 * Modified SetBitmapPixels() to always expect 8-bit pixel data.
 * 
 * Revision 7.1  96/10/10  14:04:36  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:10  mark
 * Release
 * 
 * Revision 5.4  96/07/02  14:13:10  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.3  96/06/20  17:22:57  john
 * Updates to support CspListCharsAlt1() and CspSetOutputSpecsAlt1().
 * 
 * Revision 5.2  96/03/21  11:37:59  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:09  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:04  mark
 * Release
 * 
 * Revision 3.2  96/02/27  16:23:22  mark
 * Add parentheses to guarantee the sign extension of
 * negative nibbles for x offset (only caused problems
 * with Microsoft compiler).
 * 
 * Revision 3.1  95/12/29  10:28:42  mark
 * Release
 * 
 * Revision 2.2  95/12/28  09:22:22  john
 * CspSetBitmapBits() now exits if run length <= 0 pixels.
 * 
 * Revision 2.1  95/12/21  09:45:36  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:44:28  john
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
#if INCL_CACHE
#include "cachemgr.h"
#endif

#if PROC_TRUEDOC || ! INCL_TPS

#if CSP_DEBUG
#include "csp_dbg.h"
static fix15 xSizeSaved, ySizeSaved;
#endif

/* Local type definitions */
typedef struct state_tag
    {
    ufix8   mask;
    ufix8   black;
    ufix8  *pByte;
    fix31   bitsLeft;
    fix31   whiteLeft;
    fix31   blackLeft;
    void (*GetPixRun)(struct state_tag *pState);
    fix31   xSize;
    fix15   x0, x1;
    fix15   y;
    fix15   yIncrement;
    } state_t;

typedef void (*GetPixRun_t)(struct state_tag *pState);

/* Local function prototypes */
#if INCL_PFR_BITMAPS

LOCAL_PROTO
ufix8 *FindCharBitmap(
    cspGlobals_t *pCspGlobals,
    ufix16 charCode);

LOCAL_PROTO
btsbool GetPixRunInRow(
    state_t *pState);

LOCAL_PROTO
void GetPixRun0(
    state_t *pState);

LOCAL_PROTO
void GetPixRun1(
    state_t *pState);

LOCAL_PROTO
void GetPixRun2(
    state_t *pState);

#endif

LOCAL_PROTO
void CspOpenBitmap(
    cspGlobals_t *pCspGlobals,
    fix31 x_width,
    fix31 y_width,
    fix31 x_org,
    fix31 y_org,
    fix15 x_size,
    fix15 y_size);

LOCAL_PROTO
void CspSetBitmapBits(
    cspGlobals_t *pCspGlobals,
    fix15 y,
    fix15 xStart,
    fix15 xEnd);  
    
#if INCL_ANTIALIASED_OUTPUT
LOCAL_PROTO
void CspSetBitmapPixels(
    cspGlobals_t *pCspGlobals,
    fix15 y,
    fix15 xStart,
    fix15 xSize,
    void *pPixels);
#endif

LOCAL_PROTO
void CspCloseBitmap(
    cspGlobals_t *pCspGlobals);

/* Array of pointers to bitmap unpacking functions */
#if INCL_PFR_BITMAPS
static GetPixRun_t GetPixRun[] =
    {
    GetPixRun0,
    GetPixRun1,
    GetPixRun2
    };
#endif

/* Public structure of function pointers for bitmap output to application */
intBitmapFns_t BitmapDestination =
    {
    BTSNULL,
    CspOpenBitmap,
    CspSetBitmapBits,
#if INCL_ANTIALIASED_OUTPUT
    CspSetBitmapPixels,
#else
	BTSNULL,
#endif
    CspCloseBitmap
    };

#if INCL_PFR_BITMAPS

FUNCTION
int CspExecCharBitmap(
    cspGlobals_t *pCspGlobals,
    unsigned short charCode,
    btsbool clipped)
/*
 *  Looks in the current bitmap character table for a bitmap with the
 *  specified character code.
 *  If found, delivers the bitmap image via the currently selected path
 *  for bitmap output.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
ufix8  *pByte;
ufix8   bmapCharFormat;
ufix16  gpsSize;
fix31   gpsOffset;
ufix8  *pGps;
ufix8   bmapGpsFormat;
ufix8   byte;
ufix8   bmapFlags;
fix31   xPos, yPos;
fix15   xSize, ySize;
fix31   escapement;
fix31   xEscapement;
fix31   yEscapement;
fix31   xMin, xMax;
fix31   yMin, yMax;
fix15   pfrCode;
fix31   xorg, yorg;
state_t state;
fix15   y, x0, x1;

#if CSP_DEBUG >= 2
printf("CspExecCharBitmap(%c)\n", 
    (char)charCode);
#endif

/* Exit if bitmap characters not currently active */
if ((pCspGlobals->modeFlags & BMAPS_ACTIVE) == 0)
    {
    return CSP_CHAR_CODE_ERR;
    }

/* Find the character in the current bitmap character table */
pByte = FindCharBitmap(pCspGlobals, charCode);
if (pByte == BTSNULL)              /* Character not found? */
    {
    return CSP_CHAR_CODE_ERR;
    }

/* Load the bitmap glyph program string */
bmapCharFormat = pCspGlobals->bmapCharFormat;
gpsSize = (bmapCharFormat & BIT_1)?
     (ufix16)NEXT_WORD(pByte):
     (ufix16)NEXT_BYTE(pByte);
gpsOffset = (bmapCharFormat & BIT_2)?
     NEXT_LONG(pByte):
     (fix31)((ufix16)NEXT_WORD(pByte));
pGps = CspLoadGps(pCspGlobals, gpsSize, gpsOffset);
if (pGps == BTSNULL)
    return CSP_READ_RESOURCE_ERR;

/* Start processing the bitmap glyph program string */
pByte = pGps;
bmapGpsFormat = NEXT_BYTE(pByte); /* Read GPS format byte */

/* Read X and Y positions */
switch (bmapGpsFormat & 3)
    {
case 0:
    byte = NEXT_BYTE(pByte);
    xPos = (fix31)((fix7)(byte & 0xf0)) << 12;
    yPos = (fix31)((fix7)((byte << 4) & 0xf0)) << 12;
    break;

case 1:
    xPos = (fix31)((fix7)NEXT_BYTE(pByte)) << 16;
    yPos = (fix31)((fix7)NEXT_BYTE(pByte)) << 16;
    break;

case 2:
    xPos = (fix31)((fix15)NEXT_WORD(pByte)) << 8;
    yPos = (fix31)((fix15)NEXT_WORD(pByte)) << 8;
    break;

case 3:
    xPos = NEXT_LONG(pByte) << 16;
    yPos = NEXT_LONG(pByte) << 16;
    break;
    }

/* Read X and Y dimensions */
switch ((bmapGpsFormat >> 2) & 3)
    {
case 0:
    xSize = ySize = 0;
    break;

case 1:
    byte = NEXT_BYTE(pByte);
    xSize = ((fix15)byte) >> 4;
    ySize = (fix15)byte & 0x0f;
    break;

case 2:
    xSize = (fix15)NEXT_BYTE(pByte);
    ySize = (fix15)NEXT_BYTE(pByte);
    break;

case 3:
    xSize = NEXT_WORD(pByte);
    ySize = NEXT_WORD(pByte);
    break;
    }

/* Read escapement if present */
xEscapement = 0;
yEscapement = 0;
switch((bmapGpsFormat >> 4) & 3)
    {
case 1:         /* Escapement is specified in whole pixels */
    escapement = (fix31)((fix7)NEXT_BYTE(pByte)) << 16;
    break;

case 2:         /* Escapement is specified in 8.8 units */
    escapement = (fix31)((fix15)NEXT_WORD(pByte)) << 8;
    break;

case 3:         /* Escapement is specified in 16.8 units */
    escapement = NEXT_LONG(pByte) << 8;
    break;
    }
if (pCspGlobals->verticalEscapement)
    {
    yEscapement = escapement;
    }
else
    {
    xEscapement = escapement;
    }

/* Transform bounding box and escapement into device coordinates */
switch (sp_globals.tcb.xmode)
    {
case 0:
    xMin = xPos;
    xMax = xPos + ((fix31)xSize << 16);
    break;

case 1:
    xMin = -xPos - ((fix31)xSize << 16);
    xMax = -xPos;
    xEscapement = -xEscapement;
    break;
    }
xorg = ((fix31)sp_globals.tcb.xpos << sp_globals.poshift) + xMin;

switch (sp_globals.tcb.ymode)
    {
case 0:
    yMin = yPos;
    yMax = yPos + ((fix31)ySize << 16);
    break;

case 1:
    yMin = -yPos - ((fix31)ySize << 16);
    yMax = -yPos;
    yEscapement = -yEscapement;
    break;
    }
yorg = ((fix31)sp_globals.tcb.ypos << sp_globals.poshift) + yMin;

/* Save transformaed escapement vector for later use */
switch((bmapGpsFormat >> 4) & 3)
    {
case 0:         /* Escapement is scaled linear width */
    break;

default:        /* Use escapement specified with bitmap data */
    sp_globals.set_width.x = xEscapement;
    sp_globals.set_width.y = yEscapement;
    }

/* Exit if no image needed */
if (clipped)
	{
	return CSP_NO_ERR;
	}
	
/* Open bitmap output */
pCspGlobals->rawBmapFns.OpenBitmap(
    pCspGlobals,
    sp_globals.set_width.x, 
    sp_globals.set_width.y, 
    xorg, 
    yorg,
    xSize,
    ySize);

/* Output bitmap data as series of horizontal runs */
state.bitsLeft = (fix31)xSize * ySize;
if (state.bitsLeft > 0)         /* At least 1 pixel in image? */
    {
    state.mask = 0x80;
    state.pByte = pByte;
    state.whiteLeft = 0;
    state.blackLeft = 0;
    state.GetPixRun = GetPixRun[(bmapGpsFormat >> 6) & 3];
    state.x0 = 0;
    state.x1 = 0;
    state.xSize = xSize;
    pfrCode = pCspGlobals->physFontPfrCode;
    bmapFlags = pCspGlobals->pfrTable[pfrCode].bmapFlags;
    if (bmapFlags & PFR_INVERT_BMAP) /* Read rows in descending Y order? */
        {
        state.y = ySize - 1;
        state.yIncrement = -1;
        }
    else                        /* Read rows in ascending row order? */
        {
        state.y = 0;
        state.yIncrement = 1;
        }
    state.black = (bmapFlags & PFR_BLACK_PIXEL)? 0xff: 0x00;
    while (GetPixRunInRow(&state))
        {
        switch (sp_globals.tcb.xmode)
            {
        case 0:
            x0 = state.x0;
            x1 = state.x1;
            break;
    
        case 1:
            x0 = xSize - state.x1;
            x1 = xSize - state.x0;
            break;
            }
    
        switch (sp_globals.tcb.ymode)
            {
        case 0:
            y = ySize - state.y - 1;
            break;
    
        case 1:
            y = state.y;
            break;
            }
    
    	pCspGlobals->rawBmapFns.SetBitmapBits(pCspGlobals, y, x0, x1);
        }
    }

/* Close bitmap output */
pCspGlobals->rawBmapFns.CloseBitmap(
    pCspGlobals);

return CSP_NO_ERR;
}

FUNCTION 
static ufix8 *FindCharBitmap(
    cspGlobals_t *pCspGlobals,
    ufix16 charCode)
/*
 *  Seaches the current bitmap character table for the specified character 
 *  code.
 *  If successful, returns a pointer to the byte immediatly following 
 *  the character code field of the character record.
 *  Returns BTSNULL if the character is not found
 */
{
ufix8  *pByte;
ufix16  charCodeFound;
fix31   iLow;
fix31   iHigh;
fix31   iMid;
btsbool longCharCode;

#if CSP_DEBUG >= 3
ShowBmapCharTable(pCspGlobals);
#endif

/* Search bitmap character table for specified character code */
iLow = 0;
iHigh = pCspGlobals->nBmapChars - 1;
longCharCode = (pCspGlobals->bmapCharFormat & BIT_0) != 0;
while (iHigh >= iLow)
    {
    iMid = (iLow + iHigh) >> 1;
    pByte = pCspGlobals->pFirstBmapChar + iMid * pCspGlobals->bmapCharRecSize;
    charCodeFound = longCharCode?
        (ufix16)NEXT_WORD(pByte):
        NEXT_BYTE(pByte);
    if (charCodeFound == charCode)
        {
        return pByte;
        }
    if (charCodeFound > charCode)
        {
        iHigh = iMid - 1;
        }
    else
        {
        iLow = iMid + 1;
        }
    }

return BTSNULL;
}

FUNCTION
static btsbool GetPixRunInRow(
    state_t *pState)
/*
 *  Collects a run of pixels from the current bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 *  Pixel runs that extend over multiple rows are divided at the
 *  row boundaries.
 *  Returns:
 *      TRUE:   Black run from (state.x1) to (state.x2) in row (state.y).
 *      FALSE:  No more black pixels in bitmap image.
 */
{
fix31   blackLeft;
fix31   x;
fix31   xSize;

if ((pState->blackLeft == 0))   /* Current run completed? */
    {
    pState->GetPixRun(pState);  /* Get another pixel run */
    }

/* Move to start of black and adjust row */
x = (fix31)pState->x1 + pState->whiteLeft;
pState->whiteLeft = 0;
xSize = pState->xSize;
while(x >= xSize)               /* Remainder of row is white? */
    {
    x -= xSize;
    pState->y += pState->yIncrement;
    }

pState->x0 = (fix15)x;
blackLeft = pState->blackLeft;
if (x + blackLeft >= xSize)     /* Remainder of row is  black? */
    {
    pState->x1 = (fix15)xSize;
    pState->blackLeft -= xSize - x;
    return TRUE;
    }

if (blackLeft > 0)              /* Remaining black fits within row? */
    {
    pState->x1 = pState->x0 + (fix15)blackLeft;
    pState->blackLeft = 0;
    return TRUE;
    }

return FALSE;
}

FUNCTION
static void GetPixRun0(
    state_t *pState)
/*
 *  Collects a run of pixels from a directly encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 */
{
fix31   whiteLeft;
fix31   blackLeft;
fix31   bitsLeft;
ufix8   mask;
ufix8   black;
ufix8   byte;

whiteLeft = 0;
blackLeft = 0;
mask = pState->mask;
black = pState->black;
bitsLeft = pState->bitsLeft;
if (bitsLeft <= 0)              /* End of bitmap image? */
    {
    goto L1;
    }

/* Count run of white pixels */
byte = *(pState->pByte) ^ black;
while (byte & mask)             /* White bit? */
    {
    whiteLeft++;                /* Increment white bit count */
    if (--bitsLeft <= 0)        /* End of bitmap image? */
        {
        goto L1;
        }
    mask >>= 1;
    if (mask == 0)
        {
        mask = 0x80;
        byte = *(++pState->pByte) ^ black;
        }
    }

/* Count run of black pixels */
byte = *(pState->pByte) ^ ~black;
while (byte & mask)             /* Black bit? */
    {
    blackLeft++;                /* Increment black bit count */
    if (--bitsLeft <= 0)        /* End of bitmap image? */
        {
        goto L1;
        }
    mask >>= 1;
    if (mask == 0)
        {
        mask = 0x80;
        byte = *(++pState->pByte) ^ ~black;
        }
    }

L1:
pState->whiteLeft = whiteLeft;
pState->blackLeft = blackLeft;
pState->mask = mask;
pState->bitsLeft = bitsLeft;
}

FUNCTION
static void GetPixRun1(
    state_t *pState)
/*
 *  Collects a run of pixels from a nibble run-length encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 */
{
ufix8   byte;
ufix8   newWhite;
ufix8   newBlack;
btsbool joinRequired;

pState->whiteLeft = 0;
pState->blackLeft = 0;

if (pState->bitsLeft <= 0)
    {
    return;
    }

do
    {
    byte = *(pState->pByte++);
    newWhite = byte >> 4;
    newBlack = byte & 0x0f;
    pState->bitsLeft -= (fix31)newWhite + (fix31)newBlack;
    pState->whiteLeft += (fix31)newWhite;
    pState->blackLeft += (fix31)newBlack;
    joinRequired = 
        (pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0));
    }
while (joinRequired);
}

FUNCTION
static void GetPixRun2(
    state_t *pState)
/*
 *  Collects a run of pixels from a byte run-length encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 *  Adjacent runs are combined if the first one has zero black pixels or
 *  the second one has zero white pixels.
 */
{
ufix8   newWhite;
ufix8   newBlack;
btsbool joinRequired;

pState->whiteLeft = 0;
pState->blackLeft = 0;

if (pState->bitsLeft <= 0)
    {
    return;
    }

do
    {
    newWhite = *(pState->pByte++);
    pState->whiteLeft += (fix31)newWhite;
    newBlack = *(pState->pByte++);
    pState->blackLeft += (fix31)newBlack;
    pState->bitsLeft -= (fix31)newWhite + (fix31)newBlack;
    joinRequired = 
        (pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0));
    }
while (joinRequired);
}

#endif

FUNCTION
static void CspOpenBitmap(
    cspGlobals_t *pCspGlobals,
    fix31 x_width,
    fix31 y_width,
    fix31 x_org,
    fix31 y_org,
    fix15 x_size,
    fix15 y_size)
/*
 *  Called by bitmap output module at the start of each bitmap character
 *  if bitmap cache is not active.
 */
{
bmapSpecs_t bmapSpecs;

#if CSP_DEBUG >= 2
printf("CspOpenBitmap(%3.1f, %3.1f, %3.1f, %3.1f, %d, %d)\n",
    (real)x_width / 65536.0,
    (real)y_width / 65536.0,
    (real)x_org / 65536.0,
    (real)y_org / 65536.0,
    x_size,
    y_size);
#endif

#if CSP_DEBUG
xSizeSaved = x_size;
ySizeSaved = y_size;
#endif

bmapSpecs.xPos = (short)((pCspGlobals->xPosPix + x_org + 32768L) >> 16);
pCspGlobals->bmapLeft = bmapSpecs.xPos;
bmapSpecs.yPos = (short)((pCspGlobals->yPosPix + y_org + 32768L) >> 16);
pCspGlobals->bmapTop = bmapSpecs.yPos + y_size;

bmapSpecs.xSize = (short)x_size;
bmapSpecs.ySize = (short)y_size;

if (pCspGlobals->bmapCallbackFns.OpenBitmap != BTSNULL)
    {
    pCspGlobals->bmapCallbackFns.OpenBitmap(bmapSpecs USERARG2);
    }
}

FUNCTION
static void CspSetBitmapBits(
    cspGlobals_t *pCspGlobals,
    fix15 y,		/* 0 = row closest to top edge of char bbox */
    fix15 xStart,	/* Relative to left edge of char bbox */
    fix15 xEnd)		/* Relative to left edge of char bbox */
{
#if CSP_DEBUG >= 2
printf("CspSetBitmapBits(%d, %d, %d)\n",
    (int)y,
    (int)xStart,
    (int)xEnd);
#endif

#if CSP_DEBUG
if (xStart < 0)
    printf("*** CspSetBitmapBits: xStart = %d\n", 
    (int)xStart); 

if (xEnd > xSizeSaved)
    printf("*** CspSetBitmapBits: xEnd = %d\n", 
    (int)xEnd); 

if (xEnd <= xStart)
    printf("*** CspSetBitmapBits: xStart = %d; xEnd = %d\n", 
    (int)xStart, 
    (int)xEnd);

if (y > ySizeSaved)
    printf("*** CspSetBitmapBits: y = %d\n", 
    (int)y);

#endif

/* Return if zero- or negative-length run */
if (xStart >= xEnd)
    return;

pCspGlobals->bmapCallbackFns.SetBitmapBits(
    (short)(pCspGlobals->bmapTop - y - 1),
    (short)(pCspGlobals->bmapLeft + xStart),
    (short)(pCspGlobals->bmapLeft + xEnd)
    USERARG2);
}

#if INCL_ANTIALIASED_OUTPUT
FUNCTION
static void CspSetBitmapPixels(
    cspGlobals_t *pCspGlobals,
    fix15 y,
    fix15 xStart,
    fix15 xSize,
    void *pPixels)
/*
 *	Deliver run of arbitrary pixel values.
 *	Note that the pixel values are 8-bit values independent of the current value
 *	of pixelSize with white represented by 0x00 and black by 0xff independent
 *	of the current value of BLACK_PIXEL or its derivative global.
 */
{
ufix8  *pPixelValue = (ufix8 *)pPixels;
ufix8  *pPixelTable;
fix15	ii;
ufix8	whitePixel = 
#if INCL_CACHE
	(pCspGlobals->pCmGlobals != BTSNULL)?
	((cmGlobals_t *)pCspGlobals->pCmGlobals)->blackPixel? 0x00: 0xff:
#endif
#if BLACK_PIXEL
	0x00;
#else
	0xff;
#endif

#if CSP_DEBUG >= 2
printf("CspSetBitmapPixels(%d, %d, %d)\n",
    (int)y,
    (int)xStart,
    (int)xSize);
#endif

#if CSP_DEBUG
if (xSize < 1)
    printf("*** CspSetBitmapPixels: xSize = %d\n", (int)xSize);
#endif

/* Convert pixel array into output pixel values */
pPixelTable = 
	(pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT)?
	(ufix8 *)pCspGlobals->outputSpecs.specs.pixmap1.pPixelTable:
	BTSNULL;
for (ii = 0; ii < xSize; ii++)
	{
	*pPixelValue = 
		(pPixelTable != BTSNULL)?	
		pPixelTable[*pPixelValue]:
		(*pPixelValue ^ whitePixel) >> (8 - pCspGlobals->pixelSize);
	pPixelValue++;
	}

/* Output the pixel values */
pCspGlobals->bmapCallbackFns.SetBitmapPixels(
    (short)(pCspGlobals->bmapTop - y - 1),
    (short)(pCspGlobals->bmapLeft + xStart),
    (short)xSize,
    pPixels
    USERARG2);
}
#endif

FUNCTION
static void CspCloseBitmap(
    cspGlobals_t *pCspGlobals)
{
#if CSP_DEBUG >= 2
printf("CspCloseBitmap()\n");
#endif

if (pCspGlobals->bmapCallbackFns.CloseBitmap != BTSNULL)
    {
    pCspGlobals->bmapCallbackFns.CloseBitmap(
#if REENTRANT
        pCspGlobals->userArgOutput
#endif
        );
    }
}
#endif /*PROC_TRUEDOC*/

