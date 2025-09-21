/*****************************************************************************
*                                                                            *
*                       Copyright 1993 - 96                                  *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ G P S . C *******************************
 *                                                                           *
 * Character shape player functions for interpreting glyph program strings.  *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_GPS.C 7.5 1997/03/18 16:04:26 SHAWN release $
 *                                                                                    
 *     $Log: CSP_GPS.C $
 *     Revision 7.5  1997/03/18 16:04:26  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.4  1996/11/14 16:51:24  john
 *     Updates to encapsulate knowledge of character table and glyph program
 *         string formats. This was done in connection with the addition of
 *         support for compressed PFRs.
 * Revision 7.3  96/11/06  12:49:55  john
 * Made PFR bitmap handling functionality conditionally compilable.
 * 
 * Revision 7.2  96/11/05  17:51:42  john
 * DoWidth() not sets the global setwidth value.
 * Made some outline output related code conditional on INCL_CSPOUTLINE
 * 
 * Revision 7.1  96/10/10  14:05:11  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:46  mark
 * Release
 * 
 * Revision 5.4  96/07/02  14:13:31  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.3  96/03/29  16:18:04  john
 * Added glyph specs mechanism in support of compound
 *     character structure delivery in outline output mode.
 * 
 * Revision 5.2  96/03/21  11:43:59  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:47  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:40  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:18  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:15  mark
 * Release
 * 
 * Revision 1.4  95/11/28  09:14:30  john
 * Replaced two empty switch statements which generate
 * bad code with Diab C compiler.
 * 
 * Revision 1.3  95/11/07  13:47:57  john
 * Moved SetPositionAdjustment() to csp_stk.c
 * Modified interpolated coordinate transformation to 
 * use the new oru lists.
 * Did some performance tuning in main loop of DoSimpleChar().
 * 
 * Revision 1.2  95/08/31  09:17:37  john
 * Modifications related to the redefined CspSplitCurve().
 * 
 * Revision 1.1  95/08/10  16:45:00  john
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

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

	
/* Local function prototypes */

LOCAL_PROTO
void DoSetWidth(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec);

LOCAL_PROTO
void DoSimpleChar(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte,
    ufix8  format);

LOCAL_PROTO
void ReadOruTable(
    cspGlobals_t *pCspGlobals,
    ufix8 format,
    ufix8 **ppBuff);

LOCAL_PROTO
void ReadGlyphElement(
    ufix8 **ppBuff,
    element_t *pElement);


FUNCTION
int CspExecChar(
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
ufix8  *pByte;
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
printf("CspExecChar(%c)\n", 
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
    errCode = CspFindChar(pCspGlobals, &charRecord);
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
        totalSize = charRecord.gpsSize;
        pByte = pGps;
        for (ii = 0; ii < nElements; ii++)
            {
            ReadGlyphElement(&pByte, &element);
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
            CspScaleTrans(pCspGlobals, &element);
            pSubGps = CspAddGps(pCspGlobals, element.size, element.offset, totalSize); 
            if (pSubGps == BTSNULL)
                return CSP_READ_RESOURCE_ERR;
            totalSize += element.size;
            pCspGlobals->rawOutlFns.BeginSubChar(
                pCspGlobals,
                Porg, 
                pCspGlobals->Pmin, 
                pCspGlobals->Pmax
               );
            format = NEXT_BYTE(pSubGps);
#if CSP_DEBUG
            if (format & BIT_7)
                {
                printf("*** CspDoChar: Sub character is compound\n");
                }
#endif
            do
                {
                DoSimpleChar(pCspGlobals, pSubGps, format);
                } while (!pCspGlobals->rawOutlFns.EndSubChar(pCspGlobals));
            }
       	sp_globals.rnd_xmin = 0; /* Reset position adjustment */
        } while (!pCspGlobals->rawOutlFns.EndChar(pCspGlobals));
    sp_globals.tcb = tcb;       /* Restore transformation control block */
    }
else                            /* Simple character? */
    {
    do
        {
        DoSimpleChar(pCspGlobals, pGps, format);
        } while (!pCspGlobals->rawOutlFns.EndChar(pCspGlobals));
    }
return CSP_NO_ERR;
}

FUNCTION 
int CspFindChar(
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
btsbool longCharCode;
ufix8	charFormat;
ufix8  *pByte;
int		errCode;

/* Character not found if no characters in font */
errCode = CSP_CHAR_CODE_ERR;
iHigh = (fix31)pCspGlobals->fontInfo.nCharacters - 1;
if (iHigh < 0)
	return errCode;

/* Search character list for specified character code */
iLow = 0;
targetCharCode = pCharRec->charCode;
charFormat = pCspGlobals->charFormat;
longCharCode = (charFormat & BIT_1) != 0;
do
    {
    iMid = (iLow + iHigh) >> 1;
    pByte = pCspGlobals->pFirstChar + iMid * pCspGlobals->charRecSize;
    thisCharCode = longCharCode?
        (ufix16)NEXT_WORD(pByte):
        (ufix16)NEXT_BYTE(pByte);
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

/* Update character record with set width */
pCharRec->setWidth = 
    (charFormat & BIT_2)?
	NEXT_WORD(pByte):
	pCspGlobals->standardSetWidth;
	
/* Ignore generic character code if present */
if (charFormat & BIT_3)
	{
 	pByte += 1;
	}
	
/* Update character record with glyph prog string size */
pCharRec->gpsSize = (charFormat & BIT_4)?
	(ufix16)NEXT_WORD(pByte):
	(ufix16)NEXT_BYTE(pByte);
	
/* Update character record with glyph prog string offset */
pCharRec->gpsOffset = (charFormat & BIT_5)?
	NEXT_LONG(pByte):
	(fix31)((ufix16)NEXT_WORD(pByte));

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
    ufix8  format)
/*
 *  Executes the simple glyph program string that starts at the 
 *  specified byte
 */
{
ufix8  *pExtraItems;
fix15   nExtraItems;
fix15   extraItemSize;
fix15   ii, jj;
btsbool firstContour;
point_t P00, P0;
point_t points[3];
point_t *pP;
ufix16  argFormat;
fix15   nArgs;
fix15   depth;
ufix8   inst;
fix15   xOrus, yOrus;
fix15   xPix, yPix;

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

/* Loop for each instruction in glyph program string */
firstContour = TRUE;
while(TRUE)
    {
    /* Read one instruction from glyph program string */
    inst = NEXT_BYTE(pByte);
    switch (inst >> 4)
        {
    case 0:                     /* End instruction? */
        if (!firstContour)
            {
            if (P00.x != P0.x || P00.y != P0.y)
                {
                pCspGlobals->rawOutlFns.LineTo(
                    pCspGlobals,
                    P00);
                }
            pCspGlobals->rawOutlFns.EndContour(
                pCspGlobals
                );
            }
        return;

    case 1:                     /* Line instruction */
        argFormat = inst;
        nArgs = 1;
        break;

    case 2:                     /* Horizontal line instruction */
        ii = inst & 0x0f;
        xOrus = pCspGlobals->pXorus[ii];
        xPix = pCspGlobals->pXpix[ii];
        argFormat = 0x000f;
        nArgs = 1;
        break;

    case 3:                     /* Vertical line instruction */
        ii = inst & 0x0f;
        yOrus = pCspGlobals->pYorus[ii];
        yPix = pCspGlobals->pYpix[ii];
        argFormat = 0x000f;
        nArgs = 1;
        break;

    case 4:                     /* Start inside contour */
    case 5:                     /* Start outside contour */
        if (firstContour)
            {
            xOrus = 0;
            if ((inst & 3) == 3)
                {
                for (jj = 0; jj < pCspGlobals->nXintOrus; jj++)
                    {
                    if (pCspGlobals->pXintOrus[jj] >= 0)
                        {
                        break;
                        }
                    }
                xPix = 
                    (fix15)(pCspGlobals->pXintOffset[jj] >> 
                     sp_globals.mpshift);
                }

            yOrus = 0;
            if ((inst & 12) == 12)
                {
                for (jj = 0; jj < pCspGlobals->nYintOrus; jj++)
                    {
                   if (pCspGlobals->pYintOrus[jj] >= 0)
                        {
                        break;
                        }
                    }
                yPix = 
                    (fix15)(pCspGlobals->pYintOffset[jj] >> 
                     sp_globals.mpshift);

                }
            }
        else
            {
            if (P00.x != P0.x || P00.y != P0.y)
                {
                pCspGlobals->rawOutlFns.LineTo(
                    pCspGlobals,
                    P00);
                }
            pCspGlobals->rawOutlFns.EndContour(
                pCspGlobals
                );
            }
        argFormat = inst;
        nArgs = 1;
        break;

    case 6:                     /* Horiz-vert curve quadrant */
        depth = (inst & 0x07) + pCspGlobals->depth_adj;
        argFormat = 14 + (8 << 4) + (11 << 8);
        nArgs = 3;
        break;

    case 7:                     /* Vert-horiz curve quadrant */
        depth = (inst & 0x07) + pCspGlobals->depth_adj;
        argFormat = 11 + (2 << 4) + (14 << 8);
        nArgs = 3;
        break;

    default:                    /* Curve instruction */
        depth = ((inst & 0x70) >> 4) + pCspGlobals->depth_adj;
        argFormat = inst;
        nArgs = 3;
        break;
        }

    /* Read required arguments from glyph program string */
    ii = 0;
    pP = points;
    while(TRUE)
        {
        /* Read X argument from glyph program string */
        switch (argFormat & 0x03)
            {
        case 0: /* 1-byte index into controlled oru table */
            jj = NEXT_BYTE(pByte);
            xOrus = pCspGlobals->pXorus[jj];
            xPix = pCspGlobals->pXpix[jj];
            break;
    
        case 1: /* 2-byte signed integer orus */
            xOrus = NEXT_WORD(pByte);
            goto L1;
    
        case 2:
            xOrus += (fix15)((fix7)(NEXT_BYTE(pByte)));
        L1: 
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
            break;
    
        case 3:
            break;
            }

        /* Read Y argument from glyph program string */
        switch ((argFormat >> 2) & 0x03)
            {
        case 0: /* 1-byte index into controlled oru table */
            jj = NEXT_BYTE(pByte);
            yOrus = pCspGlobals->pYorus[jj];
            yPix = pCspGlobals->pYpix[jj];
            break;
    
        case 1: /* 2-byte signed integer orus */
            yOrus = NEXT_WORD(pByte);
            goto L2;

        case 2:
            yOrus += (fix15)((fix7)(NEXT_BYTE(pByte)));
        L2: 
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
            break;

        case 3:
            break;
            }
    
        /* Set up X coordinate of point */
        switch(sp_globals.tcb.xmode)
            {
        case 0:                 /* X mode 0 */
            pP->x = xPix;
            break;
        
        case 1:                 /* X mode 1 */
            pP->x = -xPix;
            break;
        
        case 2:                 /* X mode 2 */
            pP->x = yPix;
            break;
        
        case 3:                 /* X mode 3 */
            pP->x = -yPix;
            break;
        
        default:                /* X mode 4 */
            pP->x = (fix15)(
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
            pP->y = yPix;
            break;
        
        case 1:                 /* Y mode 1 */
            pP->y = -yPix;
            break;
        
        case 2:                 /* Y mode 2 */
            pP->y = xPix;
            break;

        case 3:                 /* Y mode 3 */
            pP->y = -xPix;
            break;
    
        default:                /* Y mode 4 */
            pP->y = (fix15)(
                ((fix31)xOrus * sp_globals.tcb.yxmult + 
                 (fix31)yOrus * sp_globals.tcb.yymult + 
                 sp_globals.tcb.yoffset +
                 sp_globals.mprnd) >> 
                sp_globals.mpshift);
            break;
            }

        /* Test if all arguments have been read */
        if (ii >= (nArgs - 1))
            break;

        /* Set up format for next argument */
        if ((inst & BIT_7) && (ii == 0)) /* Second arg of curve? */
            {
            argFormat = (ufix16)NEXT_BYTE(pByte);
            }
        else
            {
            argFormat >>= 4;
            }

        ii++;
        pP++;
        }

    /* Execute instruction and accumulated argument list */
    switch (inst >> 4)
        {
    case 1:                     /* Line instruction */
    case 2:                     /* Horizontal line instruction */
    case 3:                     /* Vertical line instruction */
        pCspGlobals->rawOutlFns.LineTo(
            pCspGlobals,
            points[0]);
        P0 = points[0];
        break;

    case 4:                     /* Start inside contour */
        pCspGlobals->rawOutlFns.BeginContour(
            pCspGlobals,
            points[0], FALSE);
        firstContour = FALSE;
        P00 = P0 = points[0];
        break;

    case 5:                     /* Start outside contour */
        pCspGlobals->rawOutlFns.BeginContour(
            pCspGlobals,
            points[0], TRUE);
        firstContour = FALSE;
        P00 = P0 = points[0];
        break;

    case 6:                     /* Horiz-vert curve quadrant */
    case 7:                     /* Vert-horiz curve quadrant */
    default:                    /* Curve instruction */
        pCspGlobals->rawOutlFns.CurveTo(
            pCspGlobals,
            points[0], 
            points[1], 
            points[2], 
            (fix15)(depth >= 0? depth: 0));
        P0 = points[2];
        break;
        }
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
int     ii;

pBuff = *ppBuff;

nXorus = nYorus = 0;
if (format & BIT_2)             /* X and Y counts in one byte? */
    {
    nXorus = *pBuff & 0x0f;
    nYorus = (*pBuff >> 4) & 0x0f;
    pBuff++;
    }
else                            /* X and Y counts in separate bytes? */
    {
    if (format & BIT_0)
        nXorus = *(pBuff++);    /* Read size of X oru table */

    if (format & BIT_1)
        nYorus = *(pBuff++);    /* Read size of Y oru table */
    }

nXorusRead = nYorusRead = 0;
prevValue = 0;
while 
    ((nXorusRead + nYorusRead) <
     (nXorus + nYorus))
    {
    oruFormat = *(pBuff++);
    for (ii = 0; ii < 8; ii++)
        {
        if (nXorusRead < nXorus)
            {
            prevValue = pCspGlobals->pXorus[nXorusRead++] = 
                (oruFormat & BIT_0)?
                NEXT_WORD(pBuff):
                prevValue + (fix15)(*(pBuff++));
            }
        else if (nYorusRead < nYorus)
            {
            prevValue = pCspGlobals->pYorus[nYorusRead++] = 
                (oruFormat & BIT_0)?
                NEXT_WORD(pBuff):
                prevValue + (fix15)(*(pBuff++));
            }
        else
            {
            break;
            }
        oruFormat >>= 1;
        }
    }
pCspGlobals->nXorus = nXorus;
pCspGlobals->nYorus = nYorus;

*ppBuff = pBuff;
}

FUNCTION
static void ReadGlyphElement(
    ufix8 **ppBuff,
    element_t *pElement)
/*
 *  Reads one element of a compound glyph program string starting
 *  at the specified point, puts the element specs into the element 
 *  structure and updates the buffer pointer.
 */
{
ufix8  *pBuff;
ufix8   format;

pBuff = *ppBuff;

/* Read compound glyph format */
format = *(pBuff++);            

/* Read X scale factor */
pElement->xScale = (format & BIT_4)?
    NEXT_WORD(pBuff):
    1 << CSP_SCALE_SHIFT;

/* Read Y scale factor */
pElement->yScale = (format & BIT_5)?
    NEXT_WORD(pBuff):
    1 << CSP_SCALE_SHIFT;

/* Read X position */
switch(format & 0x03)
    {
case 1:
    pElement->xPosition = NEXT_WORD(pBuff);
    break;

case 2:
    pElement->xPosition = (fix15)((fix7)NEXT_BYTE(pBuff));
    break;

case 3:
    pElement->xPosition = 0;
    break;
    }

/* Read Y position */
switch((format >> 2) & 0x03)
    {
case 1:
    pElement->yPosition = NEXT_WORD(pBuff);
    break;

case 2:
    pElement->yPosition = (fix15)((fix7)NEXT_BYTE(pBuff));
    break;

case 3:
    pElement->yPosition = 0;
    break;
    }

/* Read glyph program string size */
pElement->size = (format & BIT_6)?
    (ufix16)NEXT_WORD(pBuff):
    (ufix16)(*(pBuff++));

/* Read glyph program string offset */
pElement->offset = (format & BIT_7)?
    (fix31)NEXT_LONG(pBuff):
    (fix31)((ufix16)NEXT_WORD(pBuff));

/* Update gps byte pointer */
*ppBuff = pBuff;
}

#endif /*PROC_TRUEDOC*/

