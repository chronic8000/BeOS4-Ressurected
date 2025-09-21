/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ T C B . C *******************************
 *                                                                           *
 * Character shape player transformation control block functions.            *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_TCB.C 7.9 1997/03/18 16:04:32 SHAWN release $
 *                                                                                    
 *     $Log: CSP_TCB.C $
 *     Revision 7.9  1997/03/18 16:04:32  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.8  1997/02/04 18:46:04  JohnC
 *     Corrected calculation of maximum absolute pixel value in CspSetTrans()
 *     Revision 7.7  1996/11/06 17:53:24  john
 *     oops... the filter is needed if the filter width *or* height is
 *         greater than 1.
 * Revision 7.6  96/11/06  17:30:43  john
 * The bitmap filter is no longer inserted in the bitmap processing pipeline
 *     if both the height and width of the filter window are 1.
 * 
 * Revision 7.5  96/11/06  12:51:44  john
 * Made PFR bitmap handling functionality conditionally compilable.
 * 
 * Revision 7.4  96/11/06  11:09:40  john
 * Fixed crash with CspGetCharSpecs().
 * 
 * Revision 7.3  96/11/05  17:56:43  john
 * SetTcb() changed to global CspSetTcb().
 * SetupBmapCache() updated to detect change of sub-pixel accuracy.
 * 
 * Revision 7.2  96/10/29  22:45:09  john
 * Updated to handle ANTIALIASED_ALT1_OUTPUT mode.
 * 
 * Revision 7.1  96/10/10  14:06:23  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:58:59  mark
 * Release
 * 
 * Revision 5.6  96/07/02  14:14:48  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.5  96/04/29  17:37:15  john
 * Updates to eliminate warning messages.
 * 
 * Revision 5.4  96/04/04  12:51:18  john
 * Moved save operation for outline options from CspDoSetup()
 *     (where the pointer may no longer be valid) to
 *      CspSetOutputSpecs() in csp_api.c.
 * 
 * Revision 5.3  96/03/29  16:17:09  john
 * Updated default outline options to include extensions
 *     for compound character structure delivery in
 *     outline output mode.
 * 
 * Revision 5.2  96/03/21  11:40:14  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:21:59  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:53  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:30:37  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:47:25  mark
 * Release
 * 
 * Revision 1.3  95/11/07  13:54:58  john
 * Modifications in support of the extended options_t structure.
 * 
 * Revision 1.2  95/08/31  09:28:25  john
 * Changes related to direct curve emboldening and the new
 *     definition of CspSplitCurve().
 * 
 * Revision 1.1  95/08/10  16:45:52  john
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

#if INCL_CACHE
#include "cachemgr.h" 
#endif

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

extern intOutlineFns_t charBBox;

#if INCL_BOLD || INCL_STROKE
extern intOutlineFns_t EmboldenProcessor;
#endif

#if INCL_STROKE
extern intOutlineFns_t StrokeProcessor;
#endif

#if INCL_CSPQUICK
extern intOutlineFns_t QuickWriter;
#endif

#if INCL_CSPOUTLINE
static fix31 unitMatrix[4] = {1L << 16, 0, 0, 1L << 16};
extern intOutlineFns_t OutlineOutput;
#endif

#if INCL_DIR
extern intOutlineFns_t DirectOutput;
#endif

#if INCL_ANTIALIASED_OUTPUT
extern intOutlineFns_t AntialiasedOutput;
#endif

#if INCL_CACHE
extern intBitmapFns_t BitmapCache;
#endif

extern intBitmapFns_t BitmapDestination;


/* Local function prototypes */

LOCAL_PROTO
fix15 SetupMult(
    cspGlobals_t *pCspGlobals,
    fix31 inputMult);

LOCAL_PROTO
void SetupBmapCache(
    cspGlobals_t *pCspGlobals);

#if INCL_ANTIALIASED_OUTPUT && INCL_STD_BMAP_FILTER
LOCAL_PROTO
void SetupBmapFilter(
    cspGlobals_t *pCspGlobals);
#endif

FUNCTION
int CspDoSetup(
    cspGlobals_t *pCspGlobals)
/*
 *  This function should be called to update the internal state whenever
 *  any of the following conditions change:
 *      Current font attributes
 *      Current physical font
 *      Current output specs
 *  It selects and initializes the required output module.
 *  Sets up internal data paths for outline processing.
 *  Sets up transformation constants.
 *  In bitmap output or antialiased output modes, the internal data paths 
 *  are set up for direct or cached ouput depending upon whether the cache 
 *  is available, enabled and the size an average character bitmap relative 
 *  to the total cache size. 
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix15   outputType;
fix31   *fontMatrix;
fix31   *outputMatrix;
fix15   bboxAdj;
fix15   halfStrokeOrus;
fix31   maxMiterLimit;
fix15   ii;
CspSpecs_t specsarg;
intOutlineFns_t *pOutlFns;

/* Ensure that the top level physical font is loaded */
#if CSP_MAX_DYNAMIC_FONTS > 0
if  (pCspGlobals->fontCode >= pCspGlobals->nLogicalFonts)
    {
    int     errCode;

    errCode = CspLoadTopPhysFont(pCspGlobals);
    if (errCode != 0)
        return errCode;
    }
#endif             

/* Set up the adjusted font bounding box */
#if CSP_MAX_DYNAMIC_FONTS > 0
pCspGlobals->adjFontBBox = 
    (pCspGlobals->fontCode >= pCspGlobals->nLogicalFonts)?
    pCspGlobals->pNewFont->fontBBox:
    pCspGlobals->fontInfo.fontBBox;
#else
pCspGlobals->adjFontBBox = 
    pCspGlobals->fontInfo.fontBBox;
#endif

/* Set up data paths required for font outline processing */
outputType = (fix15)pCspGlobals->outputSpecs.outputType;
specsarg.flags = 0;
switch (pCspGlobals->fontAttributes.fontStyle)
    {
default:
case FILLED_STYLE:
	pOutlFns = &pCspGlobals->rawOutlFns;
    break;

#if INCL_BOLD
case BOLD_STYLE:
    specsarg.flags = 1;
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType = 
        MITER_LINE_JOIN;
    pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit = 
        0x40000L;

    /* Deliver raw outlines to the embolden processor */
    pCspGlobals->rawOutlFns = EmboldenProcessor;

    goto L1;
#endif  /* INCL_BOLD */       

#if INCL_STROKE
case STROKED_STYLE:

    /* Deliver raw outlines to the stroke processor */
    pCspGlobals->rawOutlFns = StrokeProcessor;
#endif  /* INCL_STROKE */

#if INCL_STROKE || INCL_BOLD
L1:
    /* Deliver emboldened/stroked outlines to output module */
    pOutlFns = &pCspGlobals->embOutlFns;

    /* Font bounding box expansion of half the stroke thickness */
    halfStrokeOrus = 
        (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.strokeThickness >> 1) + 1;
    bboxAdj = halfStrokeOrus;

    /* Font bounding box expansion for mitered corner extensions */
    if ((halfStrokeOrus > 0) &&
        (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType == 
         MITER_LINE_JOIN))
        {
        maxMiterLimit = 
            ((fix31)pCspGlobals->fontInfo.outlineResolution *
             ABS_MITER_LIMIT +
             (halfStrokeOrus >> 1)) /
            halfStrokeOrus;
        if (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit >
            maxMiterLimit)
            {
            pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit =
                maxMiterLimit;
            }
        bboxAdj = (fix15)(
            (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit *
             halfStrokeOrus) >> 16) + 1;
        }

    /* Add another half stroke to accomodate position adjustment */
    bboxAdj += halfStrokeOrus;  

    pCspGlobals->adjFontBBox.xmin -= bboxAdj;
    pCspGlobals->adjFontBBox.ymin -= bboxAdj;
    pCspGlobals->adjFontBBox.xmax += bboxAdj;
    pCspGlobals->adjFontBBox.ymax += bboxAdj;

    break;
#endif  /* INCL_STROKE || INCL_BOLD */
    }

/* Required transformation = output matrix * font matrix */
outputMatrix = pCspGlobals->outputSpecs.specs.charBBox.outputMatrix;
#if INCL_CSPOUTLINE
fontMatrix = 
    (outputType == OUTLINE_OUTPUT)?
    unitMatrix:
    pCspGlobals->fontAttributes.fontMatrix;
#else
fontMatrix = 
    pCspGlobals->fontAttributes.fontMatrix;
#endif
sp_globals.tcb.ctm[0] = 
    CspLongMult(outputMatrix[0], fontMatrix[0]) +
    CspLongMult(outputMatrix[2], fontMatrix[1]);

sp_globals.tcb.ctm[1] = 
    CspLongMult(outputMatrix[1], fontMatrix[0]) +
    CspLongMult(outputMatrix[3], fontMatrix[1]);

sp_globals.tcb.ctm[2] = 
    CspLongMult(outputMatrix[0], fontMatrix[2]) +
    CspLongMult(outputMatrix[2], fontMatrix[3]);

sp_globals.tcb.ctm[3] = 
    CspLongMult(outputMatrix[1], fontMatrix[2]) +
    CspLongMult(outputMatrix[3], fontMatrix[3]);

#if CSP_DEBUG >= 2
printf("Font matrix = [%3.1f %3.1f %3.1f %3.1f]\n",
    (real)fontMatrix[0] / 65536.0,
    (real)fontMatrix[1] / 65536.0,
    (real)fontMatrix[2] / 65536.0,
    (real)fontMatrix[3] / 65536.0);
printf("Output matrix = [%3.1f %3.1f %3.1f %3.1f]\n",
    (real)outputMatrix[0] / 65536.0,
    (real)outputMatrix[1] / 65536.0,
    (real)outputMatrix[2] / 65536.0,
    (real)outputMatrix[3] / 65536.0);
printf("Transformation matrix = [%3.1f %3.1f %3.1f %3.1f]\n",
    (real)sp_globals.tcb.ctm[0] / 65536.0,
    (real)sp_globals.tcb.ctm[1] / 65536.0,
    (real)sp_globals.tcb.ctm[2] / 65536.0,
    (real)sp_globals.tcb.ctm[3] / 65536.0);
#endif                             

/* Set up transformation constants */
CspSetTrans(pCspGlobals);

/* Set up data paths required by output specifications */
switch(outputType)
    {
case 0:
	*pOutlFns = charBBox;
	break;

#if INCL_CSPQUICK
case BITMAP_OUTPUT:
	*pOutlFns = QuickWriter;
	
    pCspGlobals->pixelSize = 1;

    CspSetClipWindows(pCspGlobals); 

    SetupBmapCache(pCspGlobals);

    /* Set up callback functions */
    pCspGlobals->bmapCallbackFns.SetBitmap = 
        pCspGlobals->outputSpecs.specs.bitmap.SetBitmap;
    pCspGlobals->bmapCallbackFns.OpenBitmap = 
        pCspGlobals->outputSpecs.specs.bitmap.OpenBitmap;
    pCspGlobals->bmapCallbackFns.SetBitmapBits = 
        pCspGlobals->outputSpecs.specs.bitmap.SetBitmapBits;
    pCspGlobals->bmapCallbackFns.SetBitmapPixels = 
        BTSNULL;
    pCspGlobals->bmapCallbackFns.CloseBitmap = 
        pCspGlobals->outputSpecs.specs.bitmap.CloseBitmap;

    break;
#endif

#if INCL_CSPOUTLINE
case OUTLINE_OUTPUT:
	*pOutlFns = OutlineOutput;

    /* Set up callback functions */
    pCspGlobals->outlCallbackFns.Vstem = 
        pCspGlobals->outputSpecs.specs.outline.Vstem;
    pCspGlobals->outlCallbackFns.Hstem = 
        pCspGlobals->outputSpecs.specs.outline.Hstem;
    pCspGlobals->outlCallbackFns.MoveTo =
        pCspGlobals->outputSpecs.specs.outline.MoveTo;
    pCspGlobals->outlCallbackFns.LineTo =
        pCspGlobals->outputSpecs.specs.outline.LineTo;
    pCspGlobals->outlCallbackFns.CurveTo =
        pCspGlobals->outputSpecs.specs.outline.CurveTo;
    pCspGlobals->outlCallbackFns.ClosePath =
        pCspGlobals->outputSpecs.specs.outline.ClosePath;

    break;
#endif

#if INCL_DIR
case DIRECT_OUTPUT:
	*pOutlFns = DirectOutput;

    /* Set up callback functions */
    pCspGlobals->dirCallbackFns.InitOut =
        (btsbool (*)(DIRECTARG1 specs_t *specsArg))
        pCspGlobals->outputSpecs.specs.direct.InitOut;
    pCspGlobals->dirCallbackFns.BeginChar = 
        (btsbool (*)(DIRECTARG1 point_t Psw, point_t Pmin, point_t Pmax))
        pCspGlobals->outputSpecs.specs.direct.BeginChar;
    pCspGlobals->dirCallbackFns.BeginSubChar = 
        (void (*)(DIRECTARG1 point_t Psw, point_t Pmin, point_t Pmax))
        pCspGlobals->outputSpecs.specs.direct.BeginSubChar;
    pCspGlobals->dirCallbackFns.BeginContour = 
        (void (*)(DIRECTARG1 point_t P1, btsbool outside))
        pCspGlobals->outputSpecs.specs.direct.BeginContour;
    pCspGlobals->dirCallbackFns.CurveTo = 
        (void (*)(DIRECTARG1 point_t P1, point_t P2, point_t P3, fix15 depth))
        pCspGlobals->outputSpecs.specs.direct.CurveTo;
    pCspGlobals->dirCallbackFns.LineTo = 
        (void (*)(DIRECTARG1 point_t P1))
        pCspGlobals->outputSpecs.specs.direct.LineTo;
    pCspGlobals->dirCallbackFns.EndContour = 
        (void (*)(DIRECTARG))
        pCspGlobals->outputSpecs.specs.direct.EndContour;
    pCspGlobals->dirCallbackFns.EndSubChar = 
        (void (*)(DIRECTARG))
        pCspGlobals->outputSpecs.specs.direct.EndSubChar;
    pCspGlobals->dirCallbackFns.EndChar = 
        (btsbool (*)(DIRECTARG))
        pCspGlobals->outputSpecs.specs.direct.EndChar;

    break;
#endif

#if INCL_ANTIALIASED_OUTPUT
case ANTIALIASED_OUTPUT:
    pCspGlobals->pixelSize = 
        pCspGlobals->outputSpecs.specs.pixmap.pixelSize;

#if INCL_CSPQUICK
	*pOutlFns = (pCspGlobals->pixelSize == 1)? QuickWriter: AntialiasedOutput;
#else
	*pOutlFns = AntialiasedOutput;
#endif

    /* Set up clipping windows */
    CspSetClipWindows(pCspGlobals); 

    /* Set up bitmap cache */
    SetupBmapCache(pCspGlobals);
    
    /* Set up bitmap callback functions */
    pCspGlobals->bmapCallbackFns.SetBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap.SetBitmap;
    pCspGlobals->bmapCallbackFns.OpenBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap.OpenBitmap;
    pCspGlobals->bmapCallbackFns.SetBitmapBits = 
        pCspGlobals->outputSpecs.specs.pixmap.SetBitmapBits;
    pCspGlobals->bmapCallbackFns.SetBitmapPixels = 
        pCspGlobals->outputSpecs.specs.pixmap.SetBitmapPixels;
    pCspGlobals->bmapCallbackFns.CloseBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap.CloseBitmap;

    break;

case ANTIALIASED_ALT1_OUTPUT:
    pCspGlobals->pixelSize = 
        pCspGlobals->outputSpecs.specs.pixmap1.pixelSize;

#if INCL_CSPQUICK
	*pOutlFns = (pCspGlobals->pixelSize == 1)? QuickWriter: AntialiasedOutput;
#else
	*pOutlFns = AntialiasedOutput;
#endif

    /* Set up clipping windows */
    CspSetClipWindows(pCspGlobals); 

    /* Set up bitmap cache */
    SetupBmapCache(pCspGlobals);
    
#if INCL_STD_BMAP_FILTER
    /* Set up bitmap filter if specified */
	SetupBmapFilter(pCspGlobals);
#endif

    /* Set up callback functions */
    pCspGlobals->bmapCallbackFns.SetBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap1.SetBitmap;
    pCspGlobals->bmapCallbackFns.OpenBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap1.OpenBitmap;
    pCspGlobals->bmapCallbackFns.SetBitmapBits = 
        pCspGlobals->outputSpecs.specs.pixmap1.SetBitmapBits;
    pCspGlobals->bmapCallbackFns.SetBitmapPixels = 
        pCspGlobals->outputSpecs.specs.pixmap1.SetBitmapPixels;
    pCspGlobals->bmapCallbackFns.CloseBitmap = 
        pCspGlobals->outputSpecs.specs.pixmap1.CloseBitmap;

    break;
#endif
    }

/* Set up font hints for the current transformation */
#if INCL_CSPHINTS
CspSetFontHints(pCspGlobals);
#endif

/* Initialize selected output module */
for (ii = 0; ii < 4; ii++)
    {
    specsarg.ctm[ii] = 
        sp_globals.tcb.ctm[ii] / pCspGlobals->fontInfo.outlineResolution;
    }

pCspGlobals->rawOutlFns.InitOut(
    pCspGlobals,
    &specsarg);

return CSP_NO_ERR;
} 

FUNCTION
void CspSetTrans(
    cspGlobals_t *pCspGlobals)
/*
 *  Sets up the transformation from character coordinates to device
 *  coordinates based on:
 *      outline resolution (units per em)
 *      font bounding box (adjusted for bold/stroke extensions)
 *      current transformation matrix (16.16 units)
 *  Also, if the current physical font contains bitmap images and they
 *  match the current transformation and processing mode, the appropriate
 *  bitmap character table is loaded and bitmap processing is enabled.
 */
{
fix31   mult;
fix31   num, numcopy, maxNum;
fix31   denom, denomcopy;
#if CSP_MAX_DYNAMIC_FONTS > 0
CspBbox_t fontBBox;
#endif
CspBbox_t *pFontBBox;
fix15   ii, jj;
fix15   xmult, ymult;
fix15   x, y;
fix31   onemult;            /* 1.0 in multiplier units */

/* Determine largest absolute multiplier value (pixels per em) */
mult = sp_globals.tcb.ctm[0];
if (mult < 0)
    mult = -mult;
num = mult;

mult = sp_globals.tcb.ctm[1];
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;

mult = sp_globals.tcb.ctm[2];
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;

mult = sp_globals.tcb.ctm[3];
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;
num = (num + 32768L) >> 16;     /* Pixels per em */

denom = pCspGlobals->fontInfo.outlineResolution;      /* Orus per em */

/* Set curve splitting depth adjustment */
pCspGlobals->depth_adj = 0;
denomcopy = denom;
while ((num > denomcopy) && 
       (pCspGlobals->depth_adj < 5))
    {
    denomcopy <<= 1;
    denomcopy <<= 1;
    pCspGlobals->depth_adj++;
    }
numcopy = num << 2;
while ((numcopy <= denom) && 
       (pCspGlobals->depth_adj > -4))
    {
    numcopy <<= 1;
    numcopy <<= 1;
    pCspGlobals->depth_adj--;
    }

/* Set multiplier shift to accomodate largest multiplier value */
sp_globals.multshift = 13;            
while (num >= denom)            /* More than 1, 2, 4, ... pix per oru? */
    {
    num >>= 1;
    sp_globals.multshift--;     /* multshift is 12, 11, 10, ... */
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
if  ((pCspGlobals->fontCode >= pCspGlobals->nLogicalFonts) &&
     (pCspGlobals->fontInfo.outlineResolution != 
      pCspGlobals->pNewFont->outlineResolution))
    {
    fix31   scale, round;

    scale = 
        (((fix31)pCspGlobals->fontInfo.outlineResolution  << 16) +
         (pCspGlobals->pNewFont->outlineResolution >> 1)) /
        pCspGlobals->pNewFont->outlineResolution;
    round = 0x00008000;
    fontBBox.xmin = (fix15)(
        (pCspGlobals->adjFontBBox.xmin * scale + round) >> 16);
    fontBBox.ymin = (fix15)(
        (pCspGlobals->adjFontBBox.ymin * scale + round) >> 16);
    fontBBox.xmax = (fix15)(
        (pCspGlobals->adjFontBBox.xmax * scale + round) >> 16);
    fontBBox.ymax = (fix15)(
        (pCspGlobals->adjFontBBox.ymax * scale + round) >> 16);
    pFontBBox = &fontBBox;
    }
else
#endif
    {
    pFontBBox = &(pCspGlobals->adjFontBBox);
    }

/* Determine largest possible absolute pixel value */
maxNum = 0;
for (ii = 0; ii < 2; ii++)
    {
    if (ii == 0)
        {
        xmult = (fix15)((sp_globals.tcb.ctm[0] + 32768L) >> 16);
        ymult = (fix15)((sp_globals.tcb.ctm[2] + 32768L) >> 16);
        }
    else
        {
        xmult = (fix15)((sp_globals.tcb.ctm[1] + 32768L) >> 16);
        ymult = (fix15)((sp_globals.tcb.ctm[3] + 32768L) >> 16);
        }
    for (jj = 0; jj < 4; jj++)
        {
        switch(jj)
            {
        case 0:
            x = pFontBBox->xmin;
            y = pFontBBox->ymin;
            break;

        case 1:
            x = pFontBBox->xmax;
            break;

        case 2:
            y = pFontBBox->ymax;
            break;

        case 3:
            x = pFontBBox->xmin;
            break;
            }
        num = (fix31)x * xmult + (fix31)y * ymult;
        if (num < 0)
            num = -num;
        if (num > maxNum)
            maxNum = num;
        }
    }

/* Allow for stroke alignment, non-linear scaling and rounding */
maxNum += denom * 3;                    

/* Set sub-pixel unit system */
denom <<= 14;
sp_globals.pixshift = -1;
while ((maxNum <= denom) && 
   (sp_globals.pixshift < 8))   /* Max pixels <= 32768, 16384, 8192, ... pixels? */
    {
    maxNum <<= 1;
    sp_globals.pixshift++;      /* sp_globals.pixshift = 0, 1, 2, ... */
    }
if (sp_globals.pixshift < 0)
    sp_globals.pixshift = 0;

/* Set derivative fixed point arithmetic constants */
onemult = (fix31)1 << sp_globals.multshift;
pCspGlobals->multrnd = onemult >> 1;

sp_globals.onepix = (fix15)1 << sp_globals.pixshift;
sp_globals.pixrnd = sp_globals.onepix >> 1;
sp_globals.pixfix = -sp_globals.onepix;

sp_globals.mpshift = sp_globals.multshift - sp_globals.pixshift;
if (sp_globals.mpshift < 0)
    {
#if CSP_DEBUG
    printf("*** SetTrans: pixshift > multshift\n");
#endif
    sp_globals.pixshift = sp_globals.multshift;
    sp_globals.mpshift = 0;
    }
sp_globals.mprnd = ((fix31)1 << sp_globals.mpshift) >> 1;

sp_globals.poshift = 16 - sp_globals.pixshift;

/* Set up transformation constants */
sp_globals.tcb.xxmult = SetupMult(pCspGlobals, sp_globals.tcb.ctm[0]);
sp_globals.tcb.yxmult = SetupMult(pCspGlobals, sp_globals.tcb.ctm[1]);
sp_globals.tcb.xymult = SetupMult(pCspGlobals, sp_globals.tcb.ctm[2]);
sp_globals.tcb.yymult = SetupMult(pCspGlobals, sp_globals.tcb.ctm[3]);
sp_globals.tcb.xoffset = 0;
sp_globals.tcb.yoffset = 0;

CspSetTcb(pCspGlobals);

sp_globals.rnd_xmin = 0;

/* Transform the (adjusted) font bounding box */
CspTransBBox(pCspGlobals, pFontBBox);

/* Load bitmap character table if available */
#if INCL_PFR_BITMAPS
if ((pCspGlobals->outputSpecs.outputType == BITMAP_OUTPUT) &&
    (pCspGlobals->fontAttributes.fontStyle == FILLED_STYLE))
    {
    CspLoadBmapCharTable(pCspGlobals);
    }
else
#endif
    {
    pCspGlobals->modeFlags &= ~BMAPS_ACTIVE;
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
pCspGlobals->transFontPfrCode = pCspGlobals->physFontPfrCode;
#endif
}

FUNCTION 
static fix15 SetupMult(
    cspGlobals_t *pCspGlobals,
    fix31 inputMult)
/*
 */
{
fix15   imshift;       /* Right shift to internal format */
fix31   imdenom;       /* Divisor to internal format */
fix31   imrnd;         /* Rounding for division operation */

imshift = 15 - sp_globals.multshift;
imdenom = (fix31)pCspGlobals->fontInfo.outlineResolution << imshift;
imrnd = imdenom >> 1;

inputMult >>= 1;
if (inputMult >= 0)
    {
    return (fix15)((inputMult + imrnd) / imdenom);
    }
else
    {
    return -(fix15)((-inputMult + imrnd) / imdenom);
    }
}

FUNCTION
void SetupBmapCache(
    cspGlobals_t *pCspGlobals)
/*
 *  Sets up bitmap delivery via the bitmap cache if the cache is
 *  available, the user has enabled it and the character size is small
 *  enough to allow CACHE_CAPACITY average character bitmaps to fit in the
 *  total cache space.
 *  If the cache is enabled, it is cleared if the current pixel size or
 *  the current output matrix differ from the values associated with
 *  the current cache contents.
 *  Otherwise, sets up bitmap delivery bypassing the bitmap cache
 */
#if INCL_CACHE
{
cmGlobals_t *pCmGlobals = (cmGlobals_t *)(pCspGlobals->pCmGlobals);
fix15   xSize, ySize;
ufix32  avgCharSize;
btsbool sizeOK; 

if (pCspGlobals->pCmGlobals != BTSNULL &&
    pCmGlobals->userEnabled)
    {
    /* Check size is small enough for bitmap cache */
    xSize = (pCspGlobals->Pmax.x - pCspGlobals->Pmin.x) >> sp_globals.pixshift;
    ySize = (pCspGlobals->Pmax.y - pCspGlobals->Pmin.y) >> sp_globals.pixshift;
    avgCharSize = 
        ((((long)xSize * ySize) >> 4) * pCspGlobals->pixelSize) +
        sizeof(chardata_hdr) + 
        sizeof(char_desc_t);

    sizeOK =
        (pCmGlobals->cacheSize / avgCharSize) >= 
        (pCmGlobals->cacheCapacity);

    pCspGlobals->cacheEnabled =
        sizeOK &&
        (xSize < 4096) &&
        (ySize < 4096);
    }
else
    {
    pCspGlobals->cacheEnabled = FALSE;
    }

if (pCspGlobals->cacheEnabled)
    {
    fix31   *outputMatrix;
    btsbool cacheInvalid;
    fix15   ii;

    cacheInvalid = FALSE;

    /* Update cache output matrix nothing if it has changed */
    outputMatrix = pCspGlobals->outputSpecs.specs.charBBox.outputMatrix;
    for (ii = 0; ii < 4; ii++)
        {
        if (pCspGlobals->cacheOutputMatrix[ii] != outputMatrix[ii])
            {
            pCspGlobals->cacheOutputMatrix[ii] = outputMatrix[ii];
            cacheInvalid = TRUE;
            }
        }

    /* Check if pixel size has changed */
    cacheInvalid = 
    	cacheInvalid || 
    	(pCspGlobals->pixelSize != (fix15)(pCmGlobals->pixelSize));
    	
    /* Check if hint mode has changed */
#if INCL_ANTIALIASED_OUTPUT
	if (pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT)
		{
    	ufix32	cacheHintStatus;

	    cacheHintStatus = 
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.xHintMode << 24) +
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.yHintMode << 16) +
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.filterWidth << 12) +
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.filterHeight << 8) +
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.xPosSubdivs << 4) +
	    	((ufix32)pCspGlobals->outputSpecs.specs.pixmap1.yPosSubdivs);
		if (pCspGlobals->cacheHintStatus != cacheHintStatus)
			{
			pCspGlobals->cacheHintStatus = cacheHintStatus;
            cacheInvalid = TRUE;
			}
		}
	else 
		{
		if (pCspGlobals->cacheHintStatus != 0)
			{
			pCspGlobals->cacheHintStatus = 0;
            cacheInvalid = TRUE;
			}
		}
#endif		

    /* Clear cache if new output matrix or new pixel size */
    if (cacheInvalid)
        {
        CmReinitCache(pCspGlobals);
        }

    /* Deliver raw bitmaps to the bitmap cache */
    pCspGlobals->rawBmapFns = BitmapCache;
    }
else                        /* Bitmap output bypasses cache? */
    {
    /* Deliver raw bitmaps to the application */
    pCspGlobals->rawBmapFns = BitmapDestination;
    }
}
#else
{
pCspGlobals->rawBmapFns = BitmapDestination;
}
#endif

#if INCL_ANTIALIASED_OUTPUT && INCL_STD_BMAP_FILTER
FUNCTION
static void SetupBmapFilter(
    cspGlobals_t *pCspGlobals)
/*
 *	Activates the bitmap filter module if a filter function is 
 *	defined and required and the available buffer is sufficient
 *	for the currently defined font and transformation matrix.
 */
{
fix15   xSize, ySize;

if ((pCspGlobals->outputSpecs.specs.pixmap1.pFilterFn != BTSNULL) &&
	((pCspGlobals->outputSpecs.specs.pixmap1.filterWidth > 1) ||
	 (pCspGlobals->outputSpecs.specs.pixmap1.filterHeight > 1)))
	{
	xSize = 
		((pCspGlobals->Pmax.x - pCspGlobals->Pmin.x) >> sp_globals.pixshift) + 
		pCspGlobals->outputSpecs.specs.pixmap1.filterWidth + 1;
	ySize = pCspGlobals->outputSpecs.specs.pixmap1.filterHeight + 1;
	pCspGlobals->pFilterFnBuff = 
		pCspGlobals->outputSpecs.specs.pixmap1.pFilterFnBuff;
	if (pCspGlobals->pFilterFnBuff == BTSNULL)
		{
		pCspGlobals->pFilterFnBuff = pCspGlobals->defaultFilterFnBuff;
		pCspGlobals->filterFnBuffSize = FILTER_FN_BUFF_SIZE;
		}
	else
		{
		pCspGlobals->filterFnBuffSize = 
			pCspGlobals->outputSpecs.specs.pixmap1.filterFnBuffSize;
		}
	if (((long)xSize * ySize) < pCspGlobals->filterFnBuffSize)
		{
		pCspGlobals->fltBmapFns = pCspGlobals->rawBmapFns;
		pCspGlobals->rawBmapFns = 
			*(pCspGlobals->outputSpecs.specs.pixmap1.pFilterFn);
		}
	}
}
#endif

FUNCTION
void CspScaleTrans(
    cspGlobals_t *pCspGlobals,
    element_t *pElement)
/*
 *  Compounds the scale and offset values in the specified element
 *  with the current transformation control block
 */
{
sp_globals.tcb.xoffset += 
    (fix31)sp_globals.tcb.xxmult * pElement->xPosition +
    (fix31)sp_globals.tcb.xymult * pElement->yPosition;

sp_globals.tcb.yoffset += 
    (fix31)sp_globals.tcb.yxmult * pElement->xPosition +
    (fix31)sp_globals.tcb.yymult * pElement->yPosition;

sp_globals.tcb.xxmult = (fix15)(
    (((fix31)sp_globals.tcb.xxmult * pElement->xScale) + CSP_SCALE_RND) >> CSP_SCALE_SHIFT);

sp_globals.tcb.xymult = (fix15)(
    (((fix31)sp_globals.tcb.xymult * pElement->yScale) + CSP_SCALE_RND) >> CSP_SCALE_SHIFT);

sp_globals.tcb.yxmult = (fix15)(
    (((fix31)sp_globals.tcb.yxmult * pElement->xScale) + CSP_SCALE_RND) >> CSP_SCALE_SHIFT);

sp_globals.tcb.yymult = (fix15)(
    (((fix31)sp_globals.tcb.yymult * pElement->yScale) + CSP_SCALE_RND) >> CSP_SCALE_SHIFT);

CspSetTcb(pCspGlobals);
}

FUNCTION
void CspSetTcb(
	cspGlobals_t *pCspGlobals)
/*
 *  Sets up the intelligent transformation control block for the
 *  current transformation from character coordinates to device coordinates
 */
{
sp_globals.tcb.xppo = 0;
sp_globals.tcb.yppo = 0;

sp_globals.tcb.xpos = 0;
sp_globals.tcb.ypos = 0;

sp_globals.tcb.xmode = 4;
sp_globals.tcb.ymode = 4;

sp_globals.tcb.xHintsActive = FALSE;
sp_globals.tcb.yHintsActive = FALSE;

sp_globals.normal = FALSE;              /* Output module must accumulate bbox */

if (sp_globals.tcb.xymult == 0)         /* X pix is not a function of Y orus? */
    {
    if (sp_globals.tcb.xxmult >= 0)
        {
        sp_globals.tcb.xmode = 0;       /* Xpix = Xorus * xppo + xpos */
        sp_globals.tcb.xppo = sp_globals.tcb.xxmult;
        sp_globals.tcb.xpos = sp_globals.tcb.xoffset;
        }
    else 
        {
        sp_globals.tcb.xmode = 1;       /* Xpix = -(Xorus * xppo + xpos) */
        sp_globals.tcb.xppo = -sp_globals.tcb.xxmult;
        sp_globals.tcb.xpos = -sp_globals.tcb.xoffset;
        }
    sp_globals.tcb.xHintsActive = TRUE;
    }

else if (sp_globals.tcb.xxmult == 0)    /* X pix is not a function of X orus? */
    {
    if (sp_globals.tcb.xymult >= 0)
        {
        sp_globals.tcb.xmode = 2;       /* Xpix = Yorus * yppo + ypos */
        sp_globals.tcb.yppo = sp_globals.tcb.xymult;
        sp_globals.tcb.ypos = sp_globals.tcb.xoffset;
        }
    else 
        {
        sp_globals.tcb.xmode = 3;       /* Xpix = -(Yorus * yppo + ypos) */
        sp_globals.tcb.yppo = -sp_globals.tcb.xymult;
        sp_globals.tcb.ypos = -sp_globals.tcb.xoffset;
        }
    sp_globals.tcb.yHintsActive = TRUE;
    }

if (sp_globals.tcb.yxmult == 0)         /* Y pix is not a function of X orus? */
    {
    if (sp_globals.tcb.yymult >= 0)
        {
        sp_globals.tcb.ymode = 0;       /* Ypix = Yorus * yppo + ypos */
        sp_globals.tcb.yppo = sp_globals.tcb.yymult;
        sp_globals.tcb.ypos = sp_globals.tcb.yoffset;
        }
    else 
        {
        sp_globals.tcb.ymode = 1;       /* Ypix = -(Yorus * yppo + ypos) */
        sp_globals.tcb.yppo = -sp_globals.tcb.yymult;
        sp_globals.tcb.ypos = -sp_globals.tcb.yoffset;
        }
    sp_globals.tcb.yHintsActive = TRUE;
    }
else if (sp_globals.tcb.yymult == 0)    /* Y pix is not a function of Y orus? */
    {
    if (sp_globals.tcb.yxmult >= 0)
        {
        sp_globals.tcb.ymode = 2;       /* Ypix = Xorus * xppo + xpos */
        sp_globals.tcb.xppo = sp_globals.tcb.yxmult;
        sp_globals.tcb.xpos = sp_globals.tcb.yoffset;
        }
    else 
        {
        sp_globals.tcb.ymode = 3;       /* Ypix = -(Xorus * xppo + xpos) */
        sp_globals.tcb.xppo = -sp_globals.tcb.yxmult;
        sp_globals.tcb.xpos = -sp_globals.tcb.yoffset;
        }
    sp_globals.tcb.xHintsActive = TRUE;
    }

/* Add rounding to position values */
sp_globals.tcb.xpos += sp_globals.mprnd;
sp_globals.tcb.ypos += sp_globals.mprnd;
}

FUNCTION
void CspSetContourDirection(
    cspGlobals_t *pCspGlobals,
    btsbool outside)
/*
 *  Decides if a contour should be reversed based on:
 *      - Whether it is an inside or outside contour
 *      - The user-specified contour mode flags
 *      - Second-pass output from the stroke module
 */
{
fix15   revCount;

revCount = 0;

if (pCspGlobals->outputSpecs.outputType == OUTLINE_OUTPUT)
    {
    if (pCspGlobals->outlineOptions.flags & REV_ALL_CTRS)
        revCount++;

    if ((pCspGlobals->outlineOptions.flags & REV_INNER_CTRS) &&
        (!outside))
        revCount++;
    }

#if INCL_STROKE
if ((pCspGlobals->fontAttributes.fontStyle == STROKED_STYLE) &&
    (!pCspGlobals->firstPass))
    revCount++;
#endif

pCspGlobals->reverseContour = (revCount & 1) != 0;
}

#endif /*PROC_TRUEDOC*/

