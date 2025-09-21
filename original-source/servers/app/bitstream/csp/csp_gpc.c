/*****************************************************************************
*                                                                            *
*                         Copyright 1996 - 97                                *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ G P C . C *******************************
 *                                                                           *
 * Character shape player functions for interpreting glyph program strings   *
 * in a compressed PFR.														 *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *     $Header: r:/src/CSP/rcs/CSP_GPC.C 1.12 1997/03/18 16:04:25 SHAWN release $
 *                                                                                    
 *     $Log: CSP_GPC.C $
 *     Revision 1.12  1997/03/18 16:04:25  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 1.11  1997/02/19 14:43:37  MARK
 *     fix warnings on implicit casts
 *     Revision 1.10  1997/02/12 22:35:06  JohnC
 *     Outline output of adjusted outlines have xScale and/or yScale set to 
 *     zero to indicate adjustment. 
 *     This should be used by font generators to inhibit subr matching.
 *     Revision 1.9  1997/02/10 22:26:25  JohnC
 *     Added interpretation of compound glyph adjustment vector
 *     Revision 1.8  1996/12/20 15:07:12  john
 *     Corrected the value of the "outside" flag passed to BeginContour() calls.
 * Revision 1.7  96/12/17  17:57:55  john
 * Replaced Read#Nibbles() with faster version.
 * 
 * Revision 1.6  96/12/17  13:03:23  john
 * Fixed compressed glyph program string encoding problem.
 * 
 * Revision 1.5  96/12/16  14:49:04  john
 * Removed left-over debug definition.
 * 
 * Revision 1.4  96/12/13  16:08:23  john
 * Implemented interpreter for compressed glyph program strings.
 * 
 * Revision 1.3  96/11/22  16:55:35  john
 * Replaced ReadOruTable() with one that reads compressed oru table
 *     format.
 * 
 * Revision 1.2  96/11/20  11:30:13  john
 * Updates to handle compressed compound glyphs.
 * 
 * Revision 1.1  96/11/14  16:57:52  john
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

#if PROC_TRUEDOC || ! INCL_TPS
#if INCL_COMPRESSED_PFR

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

#define NEXT_EXT_LONG(P) \
	(P+=4, ((ufix32)P[-4] << 24) + \
	((ufix32)P[-3] << 16) + \
	((ufix32)P[-2] << 8) + \
	P[-1])

/* Instruction opcodes */
#define LINE1	0
#define HLINE2	1
#define VLINE2	2
#define HLINE3	3
#define VLINE3	4
#define GLINE	5
#define GMOVE	6
#define HVCRV1	7
#define VHCRV1	8
#define HVCRV2	9
#define VHCRV2	10
#define HVCRV3	11
#define VHCRV3	12
#define GCRV2	13
#define GCRV3	14
#define GCRV4	15

/* Encoding/Decoding table for GCRV2 argument format */
ufix16 gcrv2FormatTable[] =
	{
	(3 << 10) + (3 << 8) + (3 << 6) + (3 << 4) + (3 << 2) + (3 << 0),
	(0 << 10) + (3 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(3 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (0 << 2) + (3 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (3 << 2) + (0 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(0 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (0 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (0 << 0),
	(0 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(0 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(1 << 10) + (0 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (0 << 2) + (1 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (0 << 0),	
	};
	
/* Decoding table for HVCRV2 argument format */
ufix16 hvcrv2Table[] =
	{
	0x0451, 0x0452, 0x0461, 0x0462, 0x0491, 0x0492, 0x04a1, 0x04a2,	
	0x0851, 0x0852, 0x0861, 0x0862, 0x0891, 0x0892, 0x08a1, 0x08a2
	};

/* Decoding table for VHCRV2 argument format */
ufix16 vhcrv2Table[] =
	{
	0x0154, 0x0158, 0x0164, 0x0168, 0x0194, 0x0198, 0x01a4, 0x01a8,
	0x0254, 0x0258, 0x0264, 0x0268, 0x0294, 0x0298, 0x02a4, 0x02a8
	};

/* Decoding table for arg formats of 0-2 except 0, 0 */
ufix16 gcrv3Table2[] =
	{
	0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0008, 0x0009, 0x000a
	};
	
/* Decoding table for arg formats of 1-2 except 0, 0 */
ufix16 gcrv3Table3[] =
	{
	0x0005, 0x0006, 0x0009, 0x000a
	};
	
	
/* Local function prototypes */

LOCAL_PROTO
void DoSetWidth(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec);

LOCAL_PROTO
void DoSimpleChar(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte,
    ufix8 *pLastByte,
    ufix8  format);

LOCAL_PROTO
void ReadOruTable(
    cspGlobals_t *pCspGlobals,
    ufix8 format,
    ufix8 **ppBuff);

LOCAL_PROTO
void ReadGlyphElement(
    ufix8 **ppBuff,
    element_t *pElement,
    fix31 *pGpsOffset);

LOCAL_PROTO
void ReadGpsSegment(
    cspGlobals_t *pCspGlobals,
    nibble_t *pNibble,
    fix15	*pType,
    point_t	 points[]);

LOCAL_PROTO
void ReadGpsPoint(
    cspGlobals_t *pCspGlobals,
    nibble_t *pNibble,
    ufix16	argFormat,
    fix15	values[]);

LOCAL_PROTO
fix15 GetControlledCoord(
    cspGlobals_t *pCspGlobals,
    fix15	dimension,
    fix15	n);

LOCAL_PROTO
void AdjustCoord(
	cspGlobals_t *pCspGlobals,
	fix15	*pValue,
	fix15	dimension);
	
LOCAL_PROTO
ufix8 ReadNibbleBack(
	nibble_t *pNibble);

LOCAL_PROTO
ufix8 ReadNibble(
	nibble_t *pNibble);
	
LOCAL_PROTO
ufix8 Read2Nibbles(
	nibble_t *pNibble);

LOCAL_PROTO
fix15 Read3Nibbles(
	nibble_t *pNibble);


FUNCTION
int CspExecCompressedChar(
    cspGlobals_t *pCspGlobals,
    unsigned short charCode,
    btsbool clipped)
/*
 *  Executes the specified character in the currently selected font.
 *  The output is delivered as a bitmap or outline depending upon
 *  the values of the output module function pointers set by the
 *  most recent call to CspSetOutputSpecs().
 *  If the character is specified to be clipped, the set width is
 *  transformed into device coordinates but no image is generated.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
charRec_t charRecord;
ufix8  *pByte, *pLastByte;
ufix8  *pGps;
ufix8  *pSubGps;
ufix8   format;
fix15   nExtraItems;
fix15   extraItemSize;
fix15   ii;   
fix15   shift;
fix31   round;
point_t Psw;
point_t Porg;
fix15   nElements;
element_t element;
tcb_t   tcb;
fix31	totalSize;
int		errCode;

#if CSP_DEBUG >= 2
printf("CspExecCompressedChar(%c)\n", 
    (char)charCode);
#endif

/* Find the character */
charRecord.charCode = charCode;
#if CSP_MAX_DYNAMIC_FONTS > 0
if (pCspGlobals->fontCode >= pCspGlobals->nLogicalFonts)
    {
    errCode = CspFindNewFontChar(pCspGlobals, &charRecord);
    }
else
#endif
    {
    errCode = CspFindCompressedChar(pCspGlobals, &charRecord);
    }
if (errCode != 0)              /* Character not found? */
    {
    return errCode;
    }

/* Read and transform the escapement vector into device coordinates */
DoSetWidth(pCspGlobals, &charRecord);

/* Use PFR bitmap if available and appropriate */
#if INCL_PFR_BITMAPS
	{
	int     errCode;
	
	errCode = CspExecCharBitmap(
	    pCspGlobals,
	    charCode,
	    clipped);
	if (errCode == 0)
	    return CSP_NO_ERR;

	if (errCode != CSP_CHAR_CODE_ERR)
	    return errCode;
	}
#endif

/* Exit if no character image is needed */
if (clipped)
    {
    return CSP_NO_ERR;
    }

/* Load the glyph program string */
pGps = CspLoadGps(
	pCspGlobals, 
	charRecord.gpsSize, 
	charRecord.gpsOffset);
if (pGps == BTSNULL)
    return CSP_READ_RESOURCE_ERR;

/* Save glyph specifications for use by (outline) output module */
#if INCL_CSPOUTLINE
pCspGlobals->glyphSpecs.glyphType = (*pGps & BIT_7)?
    COMP_GLYPH_HEAD:
    SIMP_GLYPH;
pCspGlobals->glyphSpecs.glyphOffset = charRecord.gpsOffset;
#endif

/* Begin character output */
shift = 16 - sp_globals.pixshift;
round = (1L << shift) >> 1;
Psw.x = (fix15)((sp_globals.set_width.x + round) >> shift);
Psw.y = (fix15)((sp_globals.set_width.y + round) >> shift);
pCspGlobals->rawOutlFns.BeginChar(
    pCspGlobals,
    Psw, 
    pCspGlobals->Pmin, 
    pCspGlobals->Pmax);

/* Execute glyph program string */
pCspGlobals->adjMode = 0;
pLastByte = pGps + charRecord.gpsSize - 1;
format = NEXT_BYTE(pGps);
if (format & BIT_7)             /* Compound character? */
    {
    /* Skip over extra items in compound character expansion joint */
    if (format & BIT_6)         /* Extra items present? */
        {
        nExtraItems = (fix15)NEXT_BYTE(pGps);
        for (ii = 0; ii < nExtraItems; ii++)
            {
            extraItemSize = NEXT_BYTE(pGps);
            pGps += extraItemSize + 1;
            }
        }

    nElements = format & 0x3f;
    tcb = sp_globals.tcb;       /* Save trans control block */
    Porg.x = Porg.y = 0;        /* Set dummy value for sub char set width */
    do
        {
        fix31	gpsOffset;
        
        totalSize = charRecord.gpsSize;
        gpsOffset = charRecord.gpsOffset;
        pByte = pGps;
		pCspGlobals->adjNibble.pByte = pLastByte;
		pCspGlobals->adjNibble.phase = 1;
		pCspGlobals->nZeros = 0;
        for (ii = 0; ii < nElements; ii++)
            {
            ReadGlyphElement(&pByte, &element, &gpsOffset);
            sp_globals.tcb = tcb;
            	
#if INCL_CSPOUTLINE
            if ((pCspGlobals->outputSpecs.outputType == OUTLINE_OUTPUT) &&
                (pCspGlobals->outlineOptions.flags & ENABLE_GLYPH_SPECS))
                {
                pCspGlobals->glyphSpecs.glyphType = COMP_GLYPH_ELEMENT;

                pCspGlobals->glyphSpecs.glyphOffset = element.offset;

                pCspGlobals->glyphSpecs.xScale = 
                    (long)element.xScale << (16 - CSP_SCALE_SHIFT);
                if (pCspGlobals->outlineOptions.glyphSpecsFlags & DISABLE_X_SCALE)
                    element.xScale = 1 << CSP_SCALE_SHIFT;

                pCspGlobals->glyphSpecs.yScale = 
                    (long)element.yScale << (16 - CSP_SCALE_SHIFT);
                if (pCspGlobals->outlineOptions.glyphSpecsFlags & DISABLE_Y_SCALE)
                    element.yScale = 1 << CSP_SCALE_SHIFT;

                pCspGlobals->glyphSpecs.xPosition = 
                    (long)element.xPosition << 16;
                if (pCspGlobals->outlineOptions.glyphSpecsFlags & DISABLE_X_SHIFT)
                    element.xPosition = 0;

                pCspGlobals->glyphSpecs.yPosition = 
                    (long)element.yPosition << 16;
                if (pCspGlobals->outlineOptions.glyphSpecsFlags & DISABLE_Y_SHIFT)
                    element.yPosition = 0;
                }
#endif

			/* Enable adjustment mechanism if required */
            if (element.xScale == 0)
            	{
            	element.xScale = 1 << CSP_SCALE_SHIFT;
            	pCspGlobals->adjMode |= X_ADJ_ACTIVE;
            	}
            if (element.yScale == 0)
            	{
            	element.yScale = 1 << CSP_SCALE_SHIFT;
            	pCspGlobals->adjMode |= Y_ADJ_ACTIVE;
            	}

            CspScaleTrans(pCspGlobals, &element);
            pSubGps = CspAddGps(pCspGlobals, element.size, element.offset, totalSize); 
            if (pSubGps == BTSNULL)
                return CSP_READ_RESOURCE_ERR;
            totalSize += element.size;
            pLastByte = pSubGps + element.size - 1;
            pCspGlobals->rawOutlFns.BeginSubChar(
                pCspGlobals,
                Porg, 
                pCspGlobals->Pmin, 
                pCspGlobals->Pmax);
            format = NEXT_BYTE(pSubGps);
#if CSP_DEBUG
            if (format & BIT_7)
                {
                printf("*** CspDoChar: Sub character is compound\n");
                }
#endif
            do
                {
                DoSimpleChar(pCspGlobals, pSubGps, pLastByte, format);
                } while (!pCspGlobals->rawOutlFns.EndSubChar(pCspGlobals));
            }
       	sp_globals.rnd_xmin = 0; /* Reset position adjustment */
        } while (!pCspGlobals->rawOutlFns.EndChar(pCspGlobals));
    sp_globals.tcb = tcb;       /* Restore transformation control block */
    pCspGlobals->adjMode = 0;	/* Turn off adjustment mechanism */
    }
else                            /* Simple character? */
    {
    do
        {
        DoSimpleChar(pCspGlobals, pGps, pLastByte, format);
        } while (!pCspGlobals->rawOutlFns.EndChar(pCspGlobals));
    }
return CSP_NO_ERR;
}

FUNCTION 
int CspFindCompressedChar(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec)
/*
 *  Seaches the current character table for the character code given
 *	in the specified character record.
 *  If successful, fills in the setWidth, gpsSize and gpsOffset fields of
 *	character record.
 *	If the specified char code is not found, the char record is updated
 *	to the next available character code if there is one.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
ufix16  targetCharCode, thisCharCode;
fix31   iLow;
fix31   iHigh;
fix31   iMid;
int		errCode;

/* Character not found if no characters in font */
errCode = CSP_CHAR_CODE_ERR;
iHigh = (fix31)pCspGlobals->fontInfo.nCharacters - 1;
if (iHigh < 0)
	return errCode;

/* Search character list for specified character code */
iLow = 0;
targetCharCode = pCharRec->charCode;
do
    {
    iMid = (iLow + iHigh) >> 1;
    thisCharCode = pCspGlobals->pCharTable[iMid].charCode;
    if (thisCharCode == targetCharCode)
        {
		pCharRec->charCode = thisCharCode;
        errCode = CSP_NO_ERR;
        break;
        }
    if (thisCharCode > targetCharCode)
        {
		pCharRec->charCode = thisCharCode;
        iHigh = iMid - 1;
        }
    else
        {
        iLow = iMid + 1;
        }
    } while (iLow <= iHigh);
    
/* Update character record with set width, gps size and gps offset */
pCharRec->setWidth = pCspGlobals->pCharTable[iMid].setWidth;
pCharRec->gpsSize = pCspGlobals->pCharTable[iMid].gpsSize;
pCharRec->gpsOffset = pCspGlobals->pCharTable[iMid].gpsOffset;

return errCode;
}

FUNCTION
static void DoSetWidth(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec)
/*
 *  Reads the set width from the character record, forms it into
 *  and escapement vector and transforms the escapement vector.
 *  Sets the escapement vector 16.16 units.
 */
{
fix31  *ctm;
fix31  charSetWidth;

charSetWidth = 
    (((fix31)pCharRec->setWidth << 16) + (pCspGlobals->fontInfo.metricsResolution >> 1)) / 
    pCspGlobals->fontInfo.metricsResolution;

switch (pCspGlobals->fontAttributes.fontStyle)
    {
case FILLED_STYLE:
    break;

case STROKED_STYLE:
    charSetWidth += 
        (((fix31)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.strokeThickness << 16) + 
         (pCspGlobals->fontInfo.outlineResolution >> 1)) / 
        pCspGlobals->fontInfo.outlineResolution;
    break;

case BOLD_STYLE:
    charSetWidth += 
        (((fix31)pCspGlobals->fontAttributes.styleSpecs.styleSpecsBold.boldThickness << 16) + 
         (pCspGlobals->fontInfo.outlineResolution >> 1)) / 
        pCspGlobals->fontInfo.outlineResolution;
    break;
    }

pCspGlobals->charSetWidth = charSetWidth;
ctm = sp_globals.tcb.ctm;
if (pCspGlobals->verticalEscapement)
    {
    sp_globals.set_width.x = CspLongMult(charSetWidth, ctm[2]);
    sp_globals.set_width.y = CspLongMult(charSetWidth, ctm[3]);
    }
else
    {
    sp_globals.set_width.x = CspLongMult(charSetWidth, ctm[0]);
    sp_globals.set_width.y = CspLongMult(charSetWidth, ctm[1]);
    }
}

FUNCTION
static void DoSimpleChar(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte,
    ufix8 *pLastByte,
    ufix8  format)
/*
 *  Executes the simple glyph program string stored in the specified 
 *  byte range.
 */
{
ufix8  *pExtraItems;
fix15   nExtraItems;
fix15   extraItemSize;
fix15   ii;
nibble_t nibble;
fix15	type;
point_t P00, P0;
point_t points[3];
btsbool	moveFlag;
btsbool	contourOpen;
fix31	v0, v1;

/* Initialize adjustment vector interpreter */
pCspGlobals->xAdjValue = 0;
pCspGlobals->yAdjValue = 0;
pCspGlobals->xPrevOrigValue = 0;
pCspGlobals->yPrevOrigValue = 0;

/* Read controlled coordinate list */
ReadOruTable(pCspGlobals, format, &pByte);

/* Skip over extra data in simple gps expansion joint */
pExtraItems = BTSNULL;
if (format & BIT_3)             /* Extra items present? */
    {
    pExtraItems = pByte;
    nExtraItems = (fix15)NEXT_BYTE(pByte);
    for (ii = 0; ii < nExtraItems; ii++)
        {
        extraItemSize = NEXT_BYTE(pByte);
        pByte += extraItemSize + 1;
        }
    }

/* Form strokes and process them */
CspDoStrokes(pCspGlobals, pExtraItems);

/* Execute the body of the glyph program string */
pCspGlobals->xPrev2Value = 0;
pCspGlobals->yPrev2Value = 0;
pCspGlobals->xPrevValue = 0;
pCspGlobals->yPrevValue = 0;
nibble.pByte = pByte;
nibble.phase = 0;
type = -1;
contourOpen = FALSE;
while ((nibble.pByte < pLastByte) ||
	((nibble.pByte == pLastByte) && (nibble.phase == 0)))
    {
    ReadGpsSegment(pCspGlobals, &nibble, &type, points);
    if (type == 0)		/* Move instruction? */
    	{
    	/* Close previous contour if open */
		if (contourOpen)
            {
            contourOpen = FALSE;
            if (P00.x != P0.x || P00.y != P0.y)
                {
                pCspGlobals->rawOutlFns.LineTo(pCspGlobals, P00);
                }

            pCspGlobals->rawOutlFns.EndContour(pCspGlobals);
            }

        v0 = ((fix31)pCspGlobals->yPrevValue << 16) +
        	(fix31)pCspGlobals->xPrevValue;
        P00 = points[0];
    	moveFlag = TRUE;
    	}
    else				/* Line or curve instruction? */
    	{
    	if (moveFlag)
    		{
    		moveFlag = FALSE;

        	v1 = ((fix31)pCspGlobals->yPrevValue << 16) +
        		(fix31)pCspGlobals->xPrevValue;

	        /* Open new contour */
	        pCspGlobals->rawOutlFns.BeginContour(
	            pCspGlobals,
	            P00, 
	            (btsbool)(v1 >= v0));
	        P0 = P00;
	        contourOpen = TRUE;
    		}
    	if (type == 1)	/* Line instruction? */
    		{
        	pCspGlobals->rawOutlFns.LineTo(
        		pCspGlobals, 
        		points[0]);
        	P0 = points[0];
    		}
    	else			/* Curve instruction? */
    		{
	        pCspGlobals->rawOutlFns.CurveTo(
	            pCspGlobals,
	            points[0], 
	            points[1], 
	            points[2],
	            -1);
	        P0 = points[2];
    		}
		}
	}

if (contourOpen)
    {
    if (P00.x != P0.x || P00.y != P0.y)
        {
        pCspGlobals->rawOutlFns.LineTo(pCspGlobals, P00);
        }

    pCspGlobals->rawOutlFns.EndContour(pCspGlobals);
    }
}        

FUNCTION 
static void ReadOruTable(
    cspGlobals_t *pCspGlobals,
    ufix8 format,
    ufix8 **ppBuff) 
/*
 *  Reads the X and Y controlled coordinate table.
 */
{
ufix8 *pBuff;
fix15   nXorus, nYorus;
fix15   nXorusRead, nYorusRead;
ufix8   oruFormat;
fix15   prevValue;
fix15   ii;
btsbool	extraLongFormat;
fix15	formatPhase;
fix15	nibblePhase;
fix15	delta;
fix15  *pOruTable;
ufix8	variableFormat;
fix15	adj;

pBuff = *ppBuff;

/* Read X and Y oru counts; set extra long format flag */
extraLongFormat = FALSE;
switch(format & 3)
	{
case 0:
	nXorus = nYorus = 0;
	break;
	
case 1:
    nXorus = *pBuff & 0x0f;
    nYorus = (*pBuff >> 4) & 0x0f;
    pBuff++;
    break;
    
case 3:
	extraLongFormat = TRUE;
case 2:
    nXorus = *(pBuff++);
    nYorus = *(pBuff++);
    break;
    }

/* Read and save the table values */
nXorusRead = nYorusRead = 0;
nibblePhase = 0;
formatPhase = 0;
delta = 0;
while(TRUE)
	{
	if (nXorusRead < nXorus)
		{
		ii = nXorusRead++;
		if (ii == 0)
			{
			pOruTable = pCspGlobals->pOrigXorus;
			oruFormat = (format & BIT_4)? BIT_0: 0;
			prevValue = 0;
			}
		}
	else if (nYorusRead < nYorus)
		{
		ii = nYorusRead++;
		if (ii == 0)
			{
			pOruTable = pCspGlobals->pOrigYorus;
			oruFormat = (format & BIT_5)? BIT_0: 0;
			prevValue = 0;
			}
		}
	else
		{
		break;
		}
	
	/* Set oru value format for values other than first in each dimension */
	if (ii > 0)
		{
		if (format & BIT_6)		/* Variable formatting? */
			{
			if (formatPhase == 0)
				{
			    /* Read format nibble if variable format values */
		    	if (nibblePhase == 0)
		    		{
		    		variableFormat = *pBuff >> 4;
		    		nibblePhase = 1;
		    		}
		    	else
		    		{
		    		variableFormat = *(pBuff++) & 0x0f;
		    		nibblePhase = 0;
		    		}
				formatPhase = 3;
				}
			else
				{
				variableFormat >>= 1;
				formatPhase--;
				}
			oruFormat = variableFormat;
			}
		else					/* Fixed formatting? */
			{
			oruFormat = 0;
			}
		}
	
    /* Read and save the relative oru value */
    if ((oruFormat & BIT_0) == 0)
    	{
    	/* Read unsigned value from next two nibbles */
    	if (nibblePhase == 0)
    		{
    		delta = (fix15)(*(pBuff++));
    		}
    	else
    		{
    		delta = (fix15)((ufix8)(*(pBuff++) << 4));
    		delta += (fix15)((ufix8)(*pBuff >> 4));
    		}
    	}
    else
    	{
    	if (extraLongFormat)
    		{
	    	/* Read signed value from next four nibbles */
	    	if (nibblePhase == 0)
	    		{
	    		delta = (fix15)((fix7)(*(pBuff++))) << 8;
	    		delta += (fix15)(*(pBuff++));
	    		}
	    	else
	    		{
	    		delta = (fix15)((fix7)((*(pBuff++) & 0x0f) << 4)) << 8;
	    		delta += (fix15)(*(pBuff++)) << 4;
	    		delta += (fix15)((ufix8)(*pBuff >> 4));
	    		}
    		}
    	else
    		{
	    	/* Read signed value from next three nibbles */
	    	if (nibblePhase == 0)
	    		{
	    		delta = (fix15)((fix7)(*(pBuff++))) << 4;
	    		delta += (fix15)(*pBuff >> 4);
	    		nibblePhase = 1;
	    		}
	    	else
	    		{
	    		delta = (fix15)((fix7)(*(pBuff++) << 4)) << 4;
	    		delta += (fix15)(*(pBuff++));
	    		nibblePhase = 0;
	    		}
			}
		}
    pOruTable[ii] = prevValue + delta;
    prevValue = pOruTable[ii];
    }

/* Select Y oru table */
pOruTable = pCspGlobals->pOrigYorus;

/* Insert leading ghost edge if required */
if (format & BIT_2)
	{
	for (ii = nYorus - 1; ii >= 0; ii--)
		{
		pOruTable[ii + 1] = pOruTable[ii];
		}
	nYorus++;
	}

/* Insert trailing ghost edge if required */
if (nYorus & 1)
	{
	pOruTable[nYorus] = pOruTable[nYorus - 1];
	nYorus++;
	}
	    
if (nibblePhase)
	pBuff++;				/* Round to next byte boundary */

/* Set up controlled X coordinate table */
adj = 0;
pCspGlobals->nXorus = pCspGlobals->nBasicXorus = nXorus;
for (ii = 0; ii < nXorus; ii++)
	{
	pCspGlobals->pXorus[ii] = pCspGlobals->pOrigXorus[ii];
	if (pCspGlobals->adjMode & X_ADJ_ACTIVE)
		{
		adj += CspGetAdjustmentValue(pCspGlobals);
		pCspGlobals->pXorus[ii] += adj;
		}
	}
	
/* Set up controlled Y coordinate table */
adj = 0;
pCspGlobals->nYorus = pCspGlobals->nBasicYorus = nYorus;
for (ii = 0; ii < nYorus; ii++)
	{
	pCspGlobals->pYorus[ii] = pCspGlobals->pOrigYorus[ii];
	if (pCspGlobals->adjMode & X_ADJ_ACTIVE)
		{
		adj += CspGetAdjustmentValue(pCspGlobals);
		pCspGlobals->pYorus[ii] += adj;
		}
	}

*ppBuff = pBuff;
}

FUNCTION
static void ReadGlyphElement(
    ufix8 **ppBuff,
    element_t *pElement,
    long *pGpsOffset)
/*
 *  Reads one element of a compound glyph program string starting
 *  at the specified point, puts the element specs into the element 
 *  structure and updates the buffer pointer.
 */
{
ufix8  *pBuff;
ufix8   format;
fix15	ii;
fix15	scale;
fix15	pos;
ufix16	gpsSize;
fix31	gpsOffset;
ufix32	tmpufix32;

pBuff = *ppBuff;

/* Read glyph element format */
format = *(pBuff++);            

/* Read scale and position data for X and Y dimensions */
for (ii = 0;;)
	{
	switch (format % 6)
		{
	case 0:
		scale = 1 << CSP_SCALE_SHIFT;
		pos = 0;
		break;
		
	case 1:
		scale = 1 << CSP_SCALE_SHIFT;
		pos = (fix15)((fix7)(NEXT_BYTE(pBuff)));
		break;
		
	case 2:
		scale = 1 << CSP_SCALE_SHIFT;
		pos = NEXT_WORD(pBuff);
		break;

	case 3:
		scale = NEXT_WORD(pBuff);
		pos = (fix15)((fix7)(NEXT_BYTE(pBuff)));
		break;
		
	case 4:
		scale = NEXT_WORD(pBuff);
		pos = NEXT_WORD(pBuff);
		break;
		
	case 5:
		scale = 0;
		pos = 0;
		break;
		}
	format /= 6;
	if (ii != 0)
		{
		pElement->yScale = scale;
		pElement->yPosition = pos;
		break;
		}
	ii++;
	pElement->xScale = scale;
	pElement->xPosition = pos;
	}

/* Read glyph prog string size/offset info */
switch(format)
	{
case 0:
	gpsSize = (ufix16)NEXT_BYTE(pBuff);
	gpsOffset = *pGpsOffset - (fix31)gpsSize;
	*pGpsOffset = gpsOffset;
	break;
	
case 1:
	gpsSize = (ufix16)NEXT_BYTE(pBuff) + 256;
	gpsOffset = *pGpsOffset - (fix31)gpsSize;
	*pGpsOffset = gpsOffset;
	break;

case 2:
	gpsSize = (ufix16)NEXT_WORD(pBuff);
	gpsOffset = *pGpsOffset - (fix31)gpsSize;
	*pGpsOffset = gpsOffset;
	break;

case 3:
	tmpufix32 = (ufix32)NEXT_LONG(pBuff);
	gpsSize = (ufix16)(tmpufix32 >> 15);
	gpsOffset = *pGpsOffset - ((fix31)tmpufix32 & 0x7fff);
	break;

case 4:
	tmpufix32 = (ufix32)NEXT_LONG(pBuff);
	gpsSize = (ufix16)(tmpufix32 >> 15);
	gpsOffset = ((fix31)tmpufix32 & 0x7fff);
	break;

case 5:
	tmpufix32 = (ufix32)NEXT_EXT_LONG(pBuff);
	gpsSize = (ufix16)(tmpufix32 >> 23);
	gpsOffset = ((fix31)tmpufix32 & 0x7fffff);
	break;

case 6:
	gpsSize = (ufix16)NEXT_WORD(pBuff);
	gpsOffset = (fix31)NEXT_LONG(pBuff);
	break;
	}
pElement->size = gpsSize;		
pElement->offset = gpsOffset;

/* Update gps byte pointer */
*ppBuff = pBuff;
}

FUNCTION
static void ReadGpsSegment(
    cspGlobals_t *pCspGlobals,
    nibble_t *pNibble,
    fix15	*pType,
    point_t	 points[])
/*
 *	Reads one opcode and its associated arguments.
 * 	Sets *pType to the type of instruction:
 *		0: Move
 *		1: Line
 *		2: Curve
 */
{
fix15	values[6];
ufix8	opcode;
ufix16	argFormat;
ufix16	format1, format2, format3;
fix15	nTabs;
fix15	nPoints;
fix15	ii, jj;
fix15	xOrus, yOrus;
fix15	xPix, yPix;

values[0] = pCspGlobals->xPrevValue;
values[1] = pCspGlobals->yPrevValue;

opcode = (*pType >= 0)? ReadNibble(pNibble): GMOVE;
switch (opcode)
    {
case LINE1:	/* Horiz or vert line to controlled coordinate */
	*pType = 1;
	argFormat =  (ufix16)ReadNibble(pNibble);
	nTabs = (argFormat & BIT_2)?
		(argFormat & 0x07) - 8:
		(argFormat & 0x07) + 1;
	if (argFormat & BIT_3)	/* Vertical line? */
		{
		values[1] = GetControlledCoord(pCspGlobals, 1, nTabs);
		}
	else					/* Horizontal line? */
		{
		values[0] = GetControlledCoord(pCspGlobals, 0, nTabs);
		}
	goto L0;
		
case HLINE2: 	/* Horizontal line to signed 1-byte relative orus */
	*pType = 1;
	values[0] = 
		pCspGlobals->xPrevValue + 
		(fix15)((fix7)Read2Nibbles(pNibble));
	goto L0;
	
case VLINE2:	/* Vertical line to signed 1-byte relative orus */
	*pType = 1;
	values[1] = 
		pCspGlobals->yPrevValue + 
		(fix15)((fix7)Read2Nibbles(pNibble));
	goto L0;
	
case HLINE3:	/* General horizontal line */
	*pType = 1;
	values[0] = 
		pCspGlobals->xPrevValue +
		Read3Nibbles(pNibble);
	goto L0;
	
case VLINE3:	/* General vertical line */
	*pType = 1;
	values[1] = 
		pCspGlobals->yPrevValue +
		Read3Nibbles(pNibble);
L0:
	pCspGlobals->xPrev2Value = pCspGlobals->xPrevValue;
	pCspGlobals->yPrev2Value = pCspGlobals->yPrevValue;
	pCspGlobals->xPrevValue = values[0];
	pCspGlobals->yPrevValue = values[1];
	nPoints = 1;
	break;
	
case GLINE:		/* General line */
	*pType = 1;
	argFormat = ReadNibble(pNibble);
	ReadGpsPoint(pCspGlobals, pNibble, argFormat, values);
	nPoints = 1;
	break;
	
case GMOVE:		/* Move instruction */
	*pType = 0;
	argFormat = ReadNibble(pNibble);
	ReadGpsPoint(pCspGlobals, pNibble, argFormat, values);
	nPoints = 1;
	break;
	
case HVCRV1:	/* Horizontal-to-vertical quadrant */
	argFormat = 0x08a2;
	goto L1;

case VHCRV1:
	argFormat = 0x02a8;
	goto L2;
	
case HVCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = hvcrv2Table[argFormat];
	goto L1;		
	
case VHCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = vhcrv2Table[argFormat];
	goto L2;
	
case HVCRV3:
	argFormat = Read2Nibbles(pNibble);
	argFormat = 
		((argFormat & 0xc0) << 4) +
		((argFormat & 0x3c) << 2) +
		(argFormat & 0x03);
L1:	/* Horizontal-to-vertical curve quadrant */
	*pType = 2;
	ReadGpsPoint(pCspGlobals, pNibble, argFormat, values);
	values[2] = GetControlledCoord(pCspGlobals, 0, 0);
	values[3] = values[1];
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 4), values + 2);
	values[4] = values[2];
	values[5] = GetControlledCoord(pCspGlobals, 1, 0);
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 8), values + 4);
	nPoints = 3;
	break;
	
case VHCRV3:
	argFormat = Read2Nibbles(pNibble);
	argFormat = argFormat << 2;
L2:	/* Vertical-to-horizontal curve quadrant */
	*pType = 2;
	ReadGpsPoint(pCspGlobals, pNibble, argFormat, values);
	values[2] = values[0];
	values[3] = GetControlledCoord(pCspGlobals, 1, 0);
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 4), values + 2);
	values[4] = GetControlledCoord(pCspGlobals, 0, 0);
	values[5] = values[3];
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 8), values + 4);
	nPoints = 3;
	break;
	
case GCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = gcrv2FormatTable[argFormat];
	goto L3;
	
case GCRV3:
	argFormat = Read2Nibbles(pNibble);
	format1 = gcrv3Table2[argFormat & 0x07];
	format2 = gcrv3Table3[(argFormat >> 3) & 0x03];
	format3 = gcrv3Table2[(argFormat >> 5) & 0x07];
	argFormat = (format3 << 8) + (format2 << 4) + format1;
	goto L3;
	
case GCRV4:
	argFormat = ReadNibble(pNibble);
	argFormat = (argFormat << 8) + Read2Nibbles(pNibble);
	
L3:	/* General curve */
	*pType = 2;
	values[0] += pCspGlobals->xPrevValue - pCspGlobals->xPrev2Value;
	values[1] += pCspGlobals->yPrevValue - pCspGlobals->yPrev2Value;
	ReadGpsPoint(pCspGlobals, pNibble, argFormat, values);
	values[2] = values[0];
	values[3] = values[1];
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 4), values + 2);
	values[4] = values[2];
	values[5] = values[3];
	ReadGpsPoint(pCspGlobals, pNibble, (ufix16)(argFormat >> 8), values + 4);
	nPoints = 3;
	break;
	}
	
/* Transform points into device coordinates */
for (ii = 0; ii < nPoints; ii++)
	{
	xOrus = values[ii << 1];
	if (pCspGlobals->adjMode & X_ADJ_ACTIVE)
		{
		AdjustCoord(pCspGlobals, &xOrus, 0);
		}
    for (jj = 0; jj < pCspGlobals->nXintOrus; jj++)
        {
        if (xOrus <= pCspGlobals->pXintOrus[jj])
            {
            break;
            }
        }
    xPix = (fix15)(
        (((fix31)xOrus * pCspGlobals->pXintMult[jj]) + 
         pCspGlobals->pXintOffset[jj]) >> 
        sp_globals.mpshift);

	yOrus = values[(ii << 1) + 1];
	if (pCspGlobals->adjMode & Y_ADJ_ACTIVE)
		{
		AdjustCoord(pCspGlobals, &yOrus, 1);
		}
    for (jj = 0; jj < pCspGlobals->nYintOrus; jj++)
        {
        if (yOrus <= pCspGlobals->pYintOrus[jj])
            {
            break;
            }
        }
    yPix = (fix15)(
        (((fix31)yOrus * pCspGlobals->pYintMult[jj]) + 
         pCspGlobals->pYintOffset[jj]) >> 
        sp_globals.mpshift);

    /* Set up X coordinate of point */
    switch(sp_globals.tcb.xmode)
        {
    case 0:                 /* X mode 0 */
        points[ii].x = xPix;
        break;
    
    case 1:                 /* X mode 1 */
        points[ii].x = -xPix;
        break;
    
    case 2:                 /* X mode 2 */
        points[ii].x = yPix;
        break;
    
    case 3:                 /* X mode 3 */
        points[ii].x = -yPix;
        break;
    
    default:                /* X mode 4 */
        points[ii].x = (fix15)(
            ((fix31)xOrus * sp_globals.tcb.xxmult + 
             (fix31)yOrus * sp_globals.tcb.xymult + 
             sp_globals.tcb.xoffset +
             sp_globals.mprnd) >>
            sp_globals.mpshift);
        break;
        }

    /* Set up Y coordinate of point */
    switch(sp_globals.tcb.ymode)
        {
    case 0:                 /* Y mode 0 */
        points[ii].y = yPix;
        break;
    
    case 1:                 /* Y mode 1 */
        points[ii].y = -yPix;
        break;
    
    case 2:                 /* Y mode 2 */
        points[ii].y = xPix;
        break;

    case 3:                 /* Y mode 3 */
        points[ii].y = -xPix;
        break;

    default:                /* Y mode 4 */
        points[ii].y = (fix15)(
            ((fix31)xOrus * sp_globals.tcb.yxmult + 
             (fix31)yOrus * sp_globals.tcb.yymult + 
             sp_globals.tcb.yoffset +
             sp_globals.mprnd) >> 
            sp_globals.mpshift);
        break;
        }
	}
}

FUNCTION
static void ReadGpsPoint(
    cspGlobals_t *pCspGlobals,
    nibble_t *pNibble,
    ufix16	argFormat,
    fix15	values[])
/*
 *	Reads one point from the glyph program string using the specified format.
 */
{
fix15	delta;

switch (argFormat & 0x03)
    {
case 0: /* Default value */
    break;

case 1: /* Delta orus in signed nibble */
    values[0] = 
    	pCspGlobals->xPrevValue + 
    	(fix15)ReadNibble(pNibble) -
    	8;
    break;

case 2:	/* Delta orus in signed byte */
	delta = (fix15)((fix7)Read2Nibbles(pNibble));
	if ((delta >= -8) && (delta < 8))
		{
		values[0] = GetControlledCoord(
			pCspGlobals,
			0,
			(fix15)((delta >= 0)? delta + 1: delta));
		}
	else
		{
    	values[0] = 
    		pCspGlobals->xPrevValue +
    		delta;
    	}
    break;

case 3:	/* Delta orus in long word */
    values[0] = 
    	pCspGlobals->xPrevValue +
    	Read3Nibbles(pNibble);
    break;
    }

pCspGlobals->xPrev2Value = pCspGlobals->xPrevValue;
pCspGlobals->xPrevValue = values[0];

switch ((argFormat >> 2) & 0x03)
    {
case 0: /* Default value */
    break;

case 1: /* Delta orus in signed nibble */
    values[1] = 
    	pCspGlobals->yPrevValue + 
    	(fix15)ReadNibble(pNibble) -
    	8;
    break;

case 2:	/* Delta orus in signed byte */
	delta = (fix15)((fix7)Read2Nibbles(pNibble));
	if ((delta >= -8) && (delta < 8))
		{
		values[1] = GetControlledCoord(
			pCspGlobals,
			1,
			(fix15)((delta >= 0)? delta + 1: delta));
		}
	else
		{
    	values[1] = 
    		pCspGlobals->yPrevValue +
    		delta;
    	}
    break;

case 3:	/* Delta orus in long word */
    values[1] = 
    	pCspGlobals->yPrevValue +
    	Read3Nibbles(pNibble);
    break;
    }

pCspGlobals->yPrev2Value = pCspGlobals->yPrevValue;
pCspGlobals->yPrevValue = values[1];
}

FUNCTION
static fix15 GetControlledCoord(
    cspGlobals_t *pCspGlobals,
    fix15	dimension,
    fix15	n)
/*
 *	Returns the nth controlled coordinate in the specified diminsion 
 *  relative to the current position.
 *	If n = 0, returns the next controlled coordinate in the
 *	current direction. If the current direction is undefined, returns
 *	the current point.
 */
{
fix15  *pOruTable;
fix15	nOrus;
fix15	z0;
fix15	z;
fix15	ii;

if (dimension == 0)			/* X dimension? */
	{
	pOruTable = pCspGlobals->pOrigXorus;
	nOrus = pCspGlobals->nBasicXorus;
	z0 = pCspGlobals->xPrev2Value;
	z = pCspGlobals->xPrevValue;
	}
else						/* Y dimension? */
	{
	pOruTable = pCspGlobals->pOrigYorus;
	nOrus = pCspGlobals->nBasicYorus;
	z0 = pCspGlobals->yPrev2Value;
	z = pCspGlobals->yPrevValue;
	}

if (n == 0)
	{
	if (z > z0)				/* Current direction is right or up? */
		n = 1;
	else if (z < z0)		/* Current direction is left or down? */
		n = -1;
	else					/* Current direction is undefined? */
		return z;
	}
	
if (n > 0)					/* Tab right/up? */
	{
	for (ii = 0; ii < nOrus; ii++)
		{
		if (pOruTable[ii] > z)
			{
			ii = ii + n - 1;
			if (ii >= nOrus)
				ii = nOrus - 1;
			return pOruTable[ii];
			}
		}
	}
else						/* Tab left/down? */
	{
	for (ii = nOrus - 1; ii >= 0; ii--)
		{
		if (pOruTable[ii] < z)
			{
			ii = ii + n + 1;
			if (ii < 0)
				ii = 0;
			return pOruTable[ii];
			}
		}
	}
return z;
}

FUNCTION
static void AdjustCoord(
	cspGlobals_t *pCspGlobals,
	fix15	*pValue,
	fix15	dimension)
/*
 *	Adjusts the specified value in the specified dimension
 *	To maintain synchronization with the adjustment vector,
 *	call this function in the order x1, y1, x2, y2, etc. 
 */
{
fix15	value;
fix15	nOrus;
fix15  *pOrusIn, *pOrusOut;
fix15  *pAdjValue;
fix15  *pPrevOrigValue;
fix15	ii;
fix15	deltaIn, deltaOut;

if (dimension == 0)
	{
	nOrus = pCspGlobals->nBasicXorus;
	pOrusIn = pCspGlobals->pOrigXorus;
	pOrusOut = pCspGlobals->pXorus;
	pPrevOrigValue = &(pCspGlobals->xPrevOrigValue);
	pAdjValue = &(pCspGlobals->xAdjValue);
	}
else
	{
	nOrus = pCspGlobals->nBasicYorus;
	pOrusIn = pCspGlobals->pOrigYorus;
	pOrusOut = pCspGlobals->pYorus;
	pPrevOrigValue = &(pCspGlobals->yPrevOrigValue);
	pAdjValue = &(pCspGlobals->yAdjValue);
	}

/* Adjust coordinate based on changes to controlled coordinates */
value = *pValue;
if (nOrus > 0)
	{
	if (value <= pOrusIn[0])
		{
		value += pOrusOut[0] - pOrusIn[0];
		}
	else if (value >= pOrusIn[nOrus - 1])
		{
		value += pOrusOut[nOrus - 1] - pOrusIn[nOrus - 1];
		}
	else
		{
		for (ii = 1; ii < nOrus; ii++)
			{
			if (value <= pOrusIn[ii])
				{
				break;
				}
			}
		if (value == pOrusIn[ii])
			{
			value = pOrusOut[ii];
			}
		else
			{
			deltaIn = pOrusIn[ii] - pOrusIn[ii - 1];
			deltaOut = pOrusOut[ii] - pOrusOut[ii - 1];
			value = 
				(((fix31)(value - pOrusIn[ii - 1]) * deltaOut + (deltaIn >> 1)) /
				 deltaIn) +
				pOrusOut[ii - 1];
			}
		}
	}

if (*pValue != *pPrevOrigValue)
	{
	*pPrevOrigValue = *pValue;
	*pAdjValue += CspGetAdjustmentValue(pCspGlobals);
	}
*pValue = value + *pAdjValue;
}

FUNCTION
fix15 CspGetAdjustmentValue(
	cspGlobals_t *pCspGlobals)
/*
 *	Reads and returns one adjustment value from the current adjustment vector
 */
{
nibble_t *pNibble;
fix15	val;

if (pCspGlobals->nZeros > 0)
	{
	pCspGlobals->nZeros--;
	return 0;
	}

pNibble = &(pCspGlobals->adjNibble);
if (pNibble == BTSNULL)
	return 0;
	
val = ReadNibbleBack(pNibble);
switch (val)
	{
case 0:
	return 0;

case 1:
	val = ReadNibbleBack(pNibble);
	pCspGlobals->nZeros = val + 2;
	return 0;
	
case 2:
case 3:
case 4:
	return (fix15)(val - 5);
	
case 5:
case 6:
case 7:
	return (fix15)(val - 4);

case 8:
	val = ReadNibbleBack(pNibble);
	return (fix15)(val - 35);
	
case 9:
	val = ReadNibbleBack(pNibble);
	return (fix15)(val - 19);

case 10:
	val = ReadNibbleBack(pNibble);
	return (fix15)(val + 4);

case 11:
	val = ReadNibbleBack(pNibble);
	return (fix15)(val + 20);

case 12:
	val = ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	return (fix15)(val - 291);
	
case 13:
	val = ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	return (fix15)(val + 36);
	
case 14:
	val = ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	val = (fix15)(val << 4) >> 4;
	return val;
	
default:
	val = ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	val = (val << 4) + ReadNibbleBack(pNibble);
	return val;
	}
}

FUNCTION
static ufix8 ReadNibbleBack(
	nibble_t *pNibble)
{
ufix8 nibble;

if (pNibble->phase == 1)
	{
	pNibble->phase = 0;
	nibble = *(pNibble->pByte) & 0x0f;
	}
else
	{
	pNibble->phase = 1;
	nibble = *(pNibble->pByte--) >> 4;
	}
return nibble;
}

FUNCTION
static ufix8 ReadNibble(
	nibble_t *pNibble)
{
if (pNibble->phase == 0)
	{
	pNibble->phase = 1;
	return *(pNibble->pByte) >> 4;
	}
else
	{
	pNibble->phase = 0;
	return *(pNibble->pByte++) & 0x0f;
	}
}

FUNCTION
static ufix8 Read2Nibbles(
	nibble_t *pNibble)
{
ufix8	byte;

if (pNibble->phase == 0)
	{
	return *(pNibble->pByte++);
	}
else
	{
	byte = *(pNibble->pByte++) << 4;
	return (*(pNibble->pByte) >> 4) + byte;
	}
}

FUNCTION
static fix15 Read3Nibbles(
	nibble_t *pNibble)
{
fix15	word;

if (pNibble->phase == 0)
	{
	word = (fix15)((fix7)(*(pNibble->pByte++))) << 4;
	word += (fix15)(*(pNibble->pByte) >> 4);
	pNibble->phase = 1;
	}
else
	{
	word = (fix15)((fix7)(*(pNibble->pByte++) << 4)) << 4;
	word += (fix15)(*(pNibble->pByte++));
	pNibble->phase = 0;
	}

if ((word >= -128) && (word < 128))
	{
	word = (word << 8) + (fix15)Read2Nibbles(pNibble);
	}

return word;
}

#endif /*INCL_COMPRESSED_PFR*/
#endif /*PROC_TRUEDOC*/

