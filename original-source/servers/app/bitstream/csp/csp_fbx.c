/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 95                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ F B X . C *******************************
 *                                                                           *
 * Character shape player functions for font bounding box calculations.      *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_FBX.C 7.2 1997/03/18 16:04:24 SHAWN release $
 *                                                                                    
 *     $Log: CSP_FBX.C $
 *     Revision 7.2  1997/03/18 16:04:24  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:05:06  mark
 *     Release
 * Revision 6.1  96/08/27  11:57:41  mark
 * Release
 * 
 * Revision 5.1  96/03/14  14:20:42  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:36  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:13  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:10  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:44:56  john
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

FUNCTION
void CspTransBBox(
    cspGlobals_t *pCspGlobals,
    CspBbox_t *pFontBBox)
/*
 *  Transforms each corner of the specified font bounding box 
 *  into device coordinates using the current transformation matrix.
 *  The bounding box is assumed to include any extensions due to 
 *  stroking, emboldening or any other applied font styling.
 *  Constructs a bounding box in device coordinates around the four
 *  transformed points. Adds a one-pixel boundary to the resulting 
 *  bounding box to allow for non-linear scaling and a possible 
 *  half-pixel shift of the final bitmap such as occurs in stroking
 *  mode..
 */
{
fix15   ii;
fix15   xorus, yorus;
fix15   xpix, ypix;
fix15   xmin, ymin, xmax, ymax;

for (ii = 0; ii < 4; ii++)
    {
    switch (ii)
        {
    case 0:
        xorus = pFontBBox->xmin;
        yorus = pFontBBox->ymin;
        break;

    case 1:
        xorus = pFontBBox->xmax;
        break;

    case 2:
        yorus = pFontBBox->ymax;
        break;

    case 3:
        xorus = pFontBBox->xmin;
        break;
        }

    xpix = 
        (fix15)(
         ((fix31)xorus * sp_globals.tcb.xxmult + 
          (fix31)yorus * sp_globals.tcb.xymult + 
          sp_globals.tcb.xoffset +
          sp_globals.mprnd) >>
         sp_globals.mpshift);

    ypix = 
        (fix15)(
         ((fix31)xorus * sp_globals.tcb.yxmult + 
          (fix31)yorus * sp_globals.tcb.yymult + 
          sp_globals.tcb.yoffset +
          sp_globals.mprnd) >>
         sp_globals.mpshift);

    if (ii == 0)
        {
        xmin = xmax = xpix;
        ymin = ymax = ypix;
        }
    else
        {
        if (xpix < xmin)
            xmin = xpix;
        if (xpix > xmax)
            xmax = xpix;
        if (ypix < ymin)
            ymin = ypix;
        if (ypix > ymax)
            ymax = ypix;
        }
    }

pCspGlobals->Pmin.x = xmin - sp_globals.onepix;
pCspGlobals->Pmin.y = ymin - sp_globals.onepix;
pCspGlobals->Pmax.x = xmax + sp_globals.onepix;
pCspGlobals->Pmax.y = ymax + sp_globals.onepix;

#if CSP_DEBUG >= 2
printf("CspTransBBox output:\n");
printf("    Pmin = (%3.1f, %3.1f)\n",
    (real)pCspGlobals->Pmin.x / sp_globals.onepix,
    (real)pCspGlobals->Pmin.y / sp_globals.onepix);
printf("    Pmax = (%3.1f, %3.1f)\n",
    (real)pCspGlobals->Pmax.x / sp_globals.onepix,
    (real)pCspGlobals->Pmax.y / sp_globals.onepix);
#endif
}

FUNCTION
void CspSetClipWindows(
    cspGlobals_t *pCspGlobals)
/*
 *  Sets up the clipping windows for bitmap output mode.
 *  The clipping window is simply the output bounding box expressed
 *  in 16.16 units.
 *  The outer clipping window represents the range of character
 *  positions for which the current font bounding box will be 
 *  completely outside the output bounding box.
 *  The inner clipping window represents the range of character
 *  positions for which the current font bounding box will be 
 *  completely inside the output bounding box.
 */
{
CspBbox_t *pOutputBBox;
#define longMin  -2147483647L
#define longMax  2147483647L 
#define shortMin  -32768
#define shortMax  32767

pOutputBBox = &(pCspGlobals->outputSpecs.specs.bitmap.outputBBox);
if ((pOutputBBox->xmin == 0) &&
    (pOutputBBox->ymin == 0) &&
    (pOutputBBox->xmax == 0) &&
    (pOutputBBox->ymax == 0))    /* Clipping not active? */
    {
    pCspGlobals->clipXmin = shortMin;
    pCspGlobals->outerClipXmin = longMin;
    pCspGlobals->innerClipXmin = longMin;

    pCspGlobals->clipYmin = shortMin;
    pCspGlobals->outerClipYmin = longMin;
    pCspGlobals->innerClipYmin = longMin;

    pCspGlobals->clipXmax = shortMax;
    pCspGlobals->outerClipXmax = longMax;
    pCspGlobals->innerClipXmax = longMax;

    pCspGlobals->clipYmax = shortMax;
    pCspGlobals->outerClipYmax = longMax;
    pCspGlobals->innerClipYmax = longMax;
    }
else                            /* Clipping active? */
    {
    pCspGlobals->clipXmin = pOutputBBox->xmin;
    pCspGlobals->outerClipXmin = 
        ((fix31)pOutputBBox->xmin << 16) -
        ((fix31)pCspGlobals->Pmax.x << sp_globals.poshift);
    pCspGlobals->innerClipXmin = 
        ((fix31)pOutputBBox->xmin << 16) -
        ((fix31)pCspGlobals->Pmin.x << sp_globals.poshift);

    pCspGlobals->clipYmin = pOutputBBox->ymin; 
    pCspGlobals->outerClipYmin = 
        ((fix31)pOutputBBox->ymin << 16) -
        ((fix31)pCspGlobals->Pmax.y << sp_globals.poshift);
    pCspGlobals->innerClipYmin = 
        ((fix31)pOutputBBox->ymin << 16) -
        ((fix31)pCspGlobals->Pmin.y << sp_globals.poshift);

    pCspGlobals->clipXmax = pOutputBBox->xmax; 
    pCspGlobals->outerClipXmax = 
        ((fix31)pOutputBBox->xmax << 16) -
        ((fix31)pCspGlobals->Pmin.x << sp_globals.poshift);
    pCspGlobals->innerClipXmax = 
        ((fix31)pOutputBBox->xmax << 16) -
        ((fix31)pCspGlobals->Pmax.x << sp_globals.poshift);

    pCspGlobals->clipYmax = pOutputBBox->ymax; 
    pCspGlobals->outerClipYmax = 
        ((fix31)pOutputBBox->ymax << 16) -
        ((fix31)pCspGlobals->Pmin.y << sp_globals.poshift);
    pCspGlobals->innerClipYmax = 
        ((fix31)pOutputBBox->ymax << 16) -
        ((fix31)pCspGlobals->Pmax.y << sp_globals.poshift);
    }
}

FUNCTION
void CspClipFontBBox(
    cspGlobals_t *pCspGlobals,
    long *pXpos,
    long *pYpos)
/*
 *  Clips the current transformed font bounding box for the specified
 *  character position to the output bounding box
 */ 
{
fix31 value;
fix31 limit;

value = *pXpos + ((fix31)pCspGlobals->Pmin.x << sp_globals.poshift);
limit = (fix31)pCspGlobals->clipXmin << 16;
if (value < limit)
	pCspGlobals->Pmin.x = (fix15)((limit - *pXpos) >> sp_globals.poshift);	

value = *pYpos + ((fix31)pCspGlobals->Pmin.y << sp_globals.poshift);
limit = (fix31)pCspGlobals->clipYmin << 16;
if (value < limit)
	pCspGlobals->Pmin.y = (fix15)((limit - *pYpos) >> sp_globals.poshift);	

value = *pXpos + ((fix31)pCspGlobals->Pmax.x << sp_globals.poshift);
limit = (fix31)pCspGlobals->clipXmax << 16;
if (value > limit)
	pCspGlobals->Pmax.x = (fix15)((limit - *pXpos) >> sp_globals.poshift);	

value = *pYpos + ((fix31)pCspGlobals->Pmax.y << sp_globals.poshift);
limit = (fix31)pCspGlobals->clipYmax << 16;
if (value > limit)
	pCspGlobals->Pmax.y = (fix15)((limit - *pYpos) >> sp_globals.poshift);	
}

#endif /*PROC_TRUEDOC*/

