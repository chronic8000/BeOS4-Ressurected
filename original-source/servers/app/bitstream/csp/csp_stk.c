/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ S T K . C *******************************
 *                                                                           *
 * Character shape player stroke processing functions.                       *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_STK.C 7.7 1997/03/18 16:04:31 SHAWN release $
 *                                                                                    
 *     $Log: CSP_STK.C $
 *     Revision 7.7  1997/03/18 16:04:31  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.6  1997/02/10 22:29:04  JohnC
 *     Added adjustment vector interpretation for secondary strokes and edges.
 *     Revision 7.5  1996/12/13 16:53:34  john
 *     Fixed a bug that shut off secondary strokes and edges in output mode 5.
 * Revision 7.4  96/11/06  17:28:56  john
 * If hint interpretation is not required, this module is implemented by
 *     a simple function for setting up pix tables for primary stroke edges
 *     and vestigial interpolation tables.
 * 
 * Revision 7.3  96/11/05  17:55:32  john
 * Disabled rnd_xmin when vertical stroke hints are disabled.
 * 
 * Revision 7.2  96/10/29  22:40:52  john
 * Added selective hint disabling for use the ANTIALIASED_ALT1_OUTPUT.
 * 
 * Revision 7.1  96/10/10  14:06:13  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:58:49  mark
 * Release
 * 
 * Revision 5.7  96/07/02  14:14:34  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.6  96/06/21  17:54:17  john
 * Cast a couple of args.
 * 
 * Revision 5.5  96/06/12  10:56:39  shawn
 * Defined function pointer typedef for (*DoXstrokes)().
 * 
 * Revision 5.4  96/06/05  15:03:16  john
 * Corrected an error in the blueShift logic.
 * 
 * Revision 5.3  96/05/31  17:15:32  john
 * Replaced mechanism that prevents floating horizontal strokes
 *     from cutting through the next blue value. The new mechanism
 *     extends blue zones to ensure that any edge that is within
 *     0.25 pixels of the reference value is caught. This extension
 *     is limited to be no more than half way to the next ref value
 *     to ensure no overlapping can occur.
 * Replaced stroke control mechanism with a new one that uses
 *     stem snap tables if available in the PFR.
 * 
 * Revision 5.2  96/03/21  11:41:03  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:21:46  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:43  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:30:27  mark
 * Release
 * 
 * Revision 2.2  95/12/26  18:16:14  john
 * Removed special cases for hints inactive to fix obliquing bug.
 * 
 * Revision 2.1  95/12/21  09:47:15  mark
 * Release
 * 
 * Revision 1.6  95/12/12  14:23:45  john
 * Corrected internal handling of secondary edge thresholds.
 * 
 * Revision 1.5  95/11/30  17:38:34  john
 * Modified secondary stroke positioning to be relative
 * to parent rather than absolute.
 * 
 * Revision 1.4  95/11/07  13:52:52  john
 * Moved SetPositionAdjustment() here from csp_gps.c
 * Added support for secondary strokes.
 * Added support for secondary edges.
 * Modified interpolation table setup to omit redundant entries.
 * 
 * Revision 1.3  95/10/12  16:06:00  john
 * DoVstroke() eliminated by inlining it into CspDoVstrokes().
 * DoHstroke() eliminated by inlining it into CspDoHstrokes().
 * CspDoHstrokes() modified to:
 *     Make it easier to understand
 *     Bottom zones can now capture more than one stroke
 *     Floating zones cannot now cut thru next blue alignment
 * 
 * Revision 1.2  95/09/14  15:03:57  john
 * Replaced CspDoHstems with a new faster version that does not use
 * the functions GetFirstBlueZone and GetNextBlueZone.
 * The new function implements the blueShift mechanism.
 * It also provides a biased rounding for top zone alignment that,
 * by default, rounds up to the next whole pixel if the fractional
 * value is 1/3 or more of a pixel. A compile-time constant
 * BLUE_PIX_ROUND is provided to control this behavior.
 * 
 * Revision 1.1  95/08/10  16:45:44  john
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

#if INCL_CSPHINTS

/* Vertical alignment type definitions */
#define FLOATING         0
#define BOTTOM_ALIGNED   1
#define TOP_ALIGNED      2

/* Bias control for top alignment rounding */
#ifndef BLUE_PIX_ROUND
#define BLUE_PIX_ROUND   2731	/* Units of 1/4096 pixels */
#endif


/* Local function prototypes */

LOCAL_PROTO
void SetBlueZones(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
void SetStemControls(
    cspGlobals_t *pCspGlobals,
    fix15 ppo,
    fix15 *pStemSnaps,
    fix15 nStemSnaps,
    fix15 stdW,
    stemSnapZone_t *pStemSnapZones,
    fix15 *pnStemSnapZones);
    
LOCAL_PROTO
void DoExtraStrokes(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte);

LOCAL_PROTO
void DoExtraEdges(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte);

LOCAL_PROTO
void DoVstrokes(
    cspGlobals_t *pCspGlobals,
    fix15 firstIndex,
    fix15 lastIndex,
    fix15 parent);

LOCAL_PROTO
void DoHstrokes(
    cspGlobals_t *pCspGlobals,
    fix15 firstIndex,
    fix15 lastIndex,
    fix15 parent);

LOCAL_PROTO
void SetInterpolation(
    cspGlobals_t *pCspGlobals,
    fix15 nBasicXorus,
    fix15 nBasicYorus);

LOCAL_PROTO
void SetPositionAdjustment(
	cspGlobals_t *pCspGlobals,
    fix15 nBasicXorus);

FUNCTION
void CspSetFontHints(
    cspGlobals_t *pCspGlobals)
/*
 *  Called whenever the current font or transformation changes (but not
 *  when the transformation is changed due to scaling or positioning
 *  of an element of a compound character).
 *  Sets up the blue zone data structures and stroke weight control structures
 *  for the current transformation
 */
{
/* Set up vertical alignment controls */
#if INCL_ANTIALIASED_OUTPUT
if ((pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT) &&
	(pCspGlobals->outputSpecs.specs.pixmap1.yHintMode & NO_STROKE_ALIGNMENT))
	{
	pCspGlobals->nBlueZones = 0;
	}
else
#endif
	{
	SetBlueZones(pCspGlobals);
	}
	
/* Set up vertical stroke weight controls */
SetStemControls(
    pCspGlobals,
    sp_globals.tcb.xppo,
    pCspGlobals->fontInfo.pStemSnapsV,
    pCspGlobals->fontInfo.nStemSnapsV,
    pCspGlobals->fontInfo.stdVW,
    pCspGlobals->pStemSnapVzones,
    &(pCspGlobals->nStemSnapVzones));
    
/* Set up horizontal stroke weight controls */
SetStemControls(
    pCspGlobals,
    sp_globals.tcb.yppo,
    pCspGlobals->fontInfo.pStemSnapsH,
    pCspGlobals->fontInfo.nStemSnapsH,
    pCspGlobals->fontInfo.stdHW,
    pCspGlobals->pStemSnapHzones,
    &(pCspGlobals->nStemSnapHzones));
    

#if CSP_DEBUG >= 2
    {
    fix15   ii;

    printf("\nBlue Zones\n");
    printf("   BOTTOM     TOP        RANGE        REF\n");
    printf("     ORUS    ORUS     MIN     MAX     PIX\n");
    for (ii = 0; ii < pCspGlobals->nBlueZones; ii++)
        {
        printf(" %8d%8d%8.3f%8.3f%8.3f\n",
            pCspGlobals->fontInfo.pBlueValues[ii * 2],
            pCspGlobals->fontInfo.pBlueValues[ii * 2 + 1],
            (real)pCspGlobals->pBlueZones[ii].minPix / (real)sp_globals.onepix,
            (real)pCspGlobals->pBlueZones[ii].maxPix / (real)sp_globals.onepix,
            (real)pCspGlobals->pBlueZones[ii].refPix / (real)sp_globals.onepix);
        }

    if (sp_globals.tcb.suppressOvershoots)
        {
        printf("    Overshoots are supressed\n");
        }
    else
        {
        printf("    Overshoots are not supressed\n");
        }
    
    printf("\nVertical stroke weight controls:\n");
    for (ii = 0; ii < pCspGlobals->nStemSnapVzones; ii++)
    	{
    	printf("    Range from %5.3f to %5.3f snaps to %5.3f\n",
        (real)pCspGlobals->pStemSnapVzones[ii].minPix / sp_globals.onepix, 
        (real)pCspGlobals->pStemSnapVzones[ii].maxPix / sp_globals.onepix,
        (real)pCspGlobals->pStemSnapVzones[ii].refPix / sp_globals.onepix);
        }
    
    printf("\Horizontal stroke weight controls:\n");
    for (ii = 0; ii < pCspGlobals->nStemSnapHzones; ii++)
    	{
    	printf("    Range from %5.3f to %5.3f snaps to %5.3f\n",
        (real)pCspGlobals->pStemSnapHzones[ii].minPix / sp_globals.onepix, 
        (real)pCspGlobals->pStemSnapHzones[ii].maxPix / sp_globals.onepix,
        (real)pCspGlobals->pStemSnapHzones[ii].refPix / sp_globals.onepix);
        }
    }
#endif
}

FUNCTION
static void SetBlueZones(
    cspGlobals_t *pCspGlobals)
/*
 *  Sets up the blue zone data structures for the current transformation
 *  Specifically the active range of transformed y values is computed 
 *  (taking account of blue fuzz and blue scale effects) and the aligned
 *  pixel value for transformed y values that fall within the active range 
 *  are computed for each blue zone.
 */
{
fix15   nBlueValues, nBlueZones;
fix15   ii, jj;
fix15   refOrus, minOrus, maxOrus;

/* Set up basic blue zone table */
nBlueValues = pCspGlobals->fontInfo.nBlueValues;
pCspGlobals->nBlueZones = nBlueZones = nBlueValues >> 1;
for (ii = 0, jj = 0; ii < nBlueValues; ii += 2, jj += 1)
    {
    refOrus = (ii == 0)?        /* Bottom zone? */
        0:
        pCspGlobals->fontInfo.pBlueValues[ii];
    pCspGlobals->pBlueZones[jj].refPix = (fix15)(
         (((fix31)refOrus * sp_globals.tcb.yppo) + 
          sp_globals.tcb.ypos) >> sp_globals.mpshift);

    minOrus =  
        pCspGlobals->fontInfo.pBlueValues[ii] - 
        pCspGlobals->fontInfo.blueFuzz;
    pCspGlobals->pBlueZones[jj].minPix = (fix15)(
         (((fix31)minOrus * sp_globals.tcb.yppo) + 
          sp_globals.tcb.ypos) >> sp_globals.mpshift);

    maxOrus =  
        pCspGlobals->fontInfo.pBlueValues[ii + 1] + 
        pCspGlobals->fontInfo.blueFuzz;
    pCspGlobals->pBlueZones[jj].maxPix = (fix15)(
         (((fix31)maxOrus * sp_globals.tcb.yppo) + 
          sp_globals.tcb.ypos) >> sp_globals.mpshift);
    }

/* Extend blue zones to catch edges within 0.25 pixels of ref position */
if (nBlueZones > 0)
	{
	fix15	thresh;
	fix15   minPix, maxPix, limitPix;

	thresh = sp_globals.pixrnd >> 1;
	minPix = pCspGlobals->pBlueZones[0].refPix - thresh;
	if (pCspGlobals->pBlueZones[0].minPix > minPix)
		pCspGlobals->pBlueZones[0].minPix = minPix;
	for (jj = 1; jj < nBlueZones; jj++)
		{
		limitPix = 
			(pCspGlobals->pBlueZones[jj - 1].maxPix +
			 pCspGlobals->pBlueZones[jj].minPix) >> 1;
			 
	    maxPix = pCspGlobals->pBlueZones[jj - 1].refPix + thresh;
	    if (maxPix > limitPix)
	    	maxPix = limitPix;
	    if (pCspGlobals->pBlueZones[jj - 1].maxPix < maxPix)
	    	pCspGlobals->pBlueZones[jj - 1].maxPix = maxPix;
	    
	    minPix = pCspGlobals->pBlueZones[jj].refPix - thresh;
	    if (minPix < limitPix)
	    	minPix = limitPix;
	    if (pCspGlobals->pBlueZones[jj].minPix > minPix)
	    	pCspGlobals->pBlueZones[jj].minPix = minPix;
	    }
	maxPix = pCspGlobals->pBlueZones[nBlueZones - 1].refPix + thresh;
	if (pCspGlobals->pBlueZones[nBlueZones - 1].maxPix < maxPix)
		pCspGlobals->pBlueZones[nBlueZones - 1].maxPix = maxPix;
	}

/* Set up other global variables for current size */
sp_globals.tcb.suppressOvershoots = 
    ((fix31)sp_globals.tcb.yppo * pCspGlobals->fontInfo.outlineResolution) < 
    ((fix31)pCspGlobals->fontInfo.blueScale << sp_globals.multshift);

pCspGlobals->bluePixRnd = 
    BLUE_PIX_ROUND >> (12 - sp_globals.pixshift);

pCspGlobals->blueShiftPix = (fix15)(
    ((fix31)pCspGlobals->fontInfo.blueShift * 
     sp_globals.tcb.yppo) >> 
    sp_globals.mpshift);
}

FUNCTION
static void SetStemControls(
    cspGlobals_t *pCspGlobals,
    fix15 ppo,
    fix15 *pStemSnaps,
    fix15 nStemSnaps,
    fix15 stdW,
    stemSnapZone_t *pStemSnapZones,
    fix15 *pnStemSnapZones)
/*
 *  Sets up a table of stem snap zones for one dimension based on the specified
 *	size, standard stroke weight and stem snap values.
 */
{
fix15   thresh, thresh1, thresh2;
fix15   ii, jj;
fix15   refPix, refPixRnd, minPix, maxPix;
fix15   cpshift;


/* Set up initial stem snap table */
thresh = sp_globals.pixrnd >> 1;
for (ii = 0; ii < nStemSnaps; ii++)
	{
	refPix = (fix15)(
     (((fix31)pStemSnaps[ii] * ppo) +
      sp_globals.mprnd) >>
     sp_globals.mpshift);
	minPix = refPix - thresh;
	maxPix = refPix + thresh;
	      
	/* Ensure no overlap between zones */
	if ((ii > 0) &&
		(pStemSnapZones[ii - 1].maxPix > minPix))
		{
		if (pStemSnapZones[ii - 1].maxPix > refPix)
			pStemSnapZones[ii - 1].maxPix = refPix;
		if (minPix < pStemSnapZones[ii - 1].refPix)
			minPix = pStemSnapZones[ii - 1].refPix;
		pStemSnapZones[ii - 1].maxPix = minPix =
			(pStemSnapZones[ii - 1].maxPix + minPix) >> 1;
		}

	/* Update the stem snap table */
	pStemSnapZones[ii].refPix = refPix;
	pStemSnapZones[ii].minPix = minPix;
	pStemSnapZones[ii].maxPix = maxPix;
	}
	
/* Exit if no main stem snap zone */
if (stdW <= 0)
	{
	*pnStemSnapZones = nStemSnaps;
	return;
	}

/* Compute main stem snap zone */
cpshift = 16 - sp_globals.pixshift;

refPix = (fix15)(
     (((fix31)stdW * ppo) +
      sp_globals.mprnd) >>
     sp_globals.mpshift);

refPixRnd = (refPix < sp_globals.onepix)?
	sp_globals.onepix:
    (refPix + sp_globals.pixrnd) & sp_globals.pixfix;

thresh1 = refPix - (fix15)(23593L >> cpshift);
thresh2 = refPixRnd - (fix15)(47513L >> cpshift);
minPix = (thresh1 > thresh2)? thresh1: thresh2;
if (minPix > refPix)
	minPix = refPix;
	
thresh1 = refPix + (fix15)(39322L >> cpshift);
thresh2 = refPixRnd + (fix15)(57344L >> cpshift);
maxPix = (thresh1 < thresh2)? thresh1: thresh2;
if (maxPix < refPix)
	maxPix = refPix;

/* Remove captured stem snap zones from table */
jj = 0;
for (ii = 0; ii < nStemSnaps; ii++)
	{
	if ((pStemSnapZones[ii].refPix >= minPix) &&
		(pStemSnapZones[ii].refPix <= maxPix))
		{
		/* Ensure main zone includes captured zones */
		if (pStemSnapZones[ii].minPix < minPix)
			minPix = pStemSnapZones[ii].minPix;
		if (pStemSnapZones[ii].maxPix > maxPix)
			maxPix = pStemSnapZones[ii].maxPix;
		}
	else
		{
		pStemSnapZones[jj++] = pStemSnapZones[ii];
		}
	}
nStemSnaps = jj;

/* Make room for main stem snap zone */
for (
	ii = nStemSnaps - 1;
	(ii >= 0) && (pStemSnapZones[ii].refPix > refPix);
	ii--)
	{
	pStemSnapZones[ii + 1] = pStemSnapZones[ii];
	}

/* Ensure that main stem snap zone does not overlap uncaptured zones */
if ((ii >= 0) && (minPix < pStemSnapZones[ii].maxPix))
	minPix = pStemSnapZones[ii].maxPix;
ii++;
if ((ii < nStemSnaps) && (maxPix > pStemSnapZones[ii].minPix))
	maxPix = pStemSnapZones[ii].maxPix;

/* Insert the main stem snap zone */
pStemSnapZones[ii].minPix = minPix;
pStemSnapZones[ii].maxPix = maxPix;
pStemSnapZones[ii].refPix = refPix;

*pnStemSnapZones = nStemSnaps + 1;
}

FUNCTION
void CspDoStrokes(
    cspGlobals_t *pCspGlobals,
    ufix8 *pExtraItems)
/*
 *  Assigns pixel positions for vertical and horizontal stroke edges
 *  Delivers character hints to the output module.
 *  Sets up interpolation coefficients for each zone in the piecewise-
 *  linear transformation.
 *  Sets up the horizontal sub-pixel adjustment to compensate for vertical
 *  stroke positioning.
 */
{
fix15   nBasicXorus, nBasicYorus;

/* Position primary vertical strokes */
#if INCL_ANTIALIASED_OUTPUT
if ((pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT) &&
	(pCspGlobals->outputSpecs.specs.pixmap1.xHintMode & NO_PRIMARY_STROKES))
	{
	fix15	ii;
	
	for (ii = 0; ii < pCspGlobals->nXorus; ii++)
		{
	    pCspGlobals->pXpix[ii] = (fix15)(
	    	(((fix31)pCspGlobals->pXorus[ii] *
	    	  sp_globals.tcb.xppo) + 
	         sp_globals.tcb.xpos) >>
	        sp_globals.mpshift);
		}
	}
else
#endif
	{
	DoVstrokes(
	    pCspGlobals, 
	    0,
	    pCspGlobals->nXorus,
	    -1);
	}
nBasicXorus = pCspGlobals->nXorus;

/* Position primary horizontal strokes */
#if INCL_ANTIALIASED_OUTPUT
if ((pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT) &&
	(pCspGlobals->outputSpecs.specs.pixmap1.yHintMode & NO_PRIMARY_STROKES))
	{
	fix15	ii;
	
	for (ii = 0; ii < pCspGlobals->nYorus; ii++)
		{
	    pCspGlobals->pYpix[ii] = (fix15)(
	    	(((fix31)pCspGlobals->pYorus[ii] *
	    	  sp_globals.tcb.yppo) + 
	         sp_globals.tcb.ypos) >>
	        sp_globals.mpshift);
		}
	}
else
#endif
	{
	DoHstrokes(
	    pCspGlobals, 
	    0,
	    pCspGlobals->nYorus,
	    -1);
	}
nBasicYorus = pCspGlobals->nYorus;

/* Deliver primary stroke hints to output module */
if (pCspGlobals->rawOutlFns.CharHint != BTSNULL)
    {
    charHint_t charHint;
    fix15   ii;

    /* Deliver vertical stroke hints (primary and secondary) */
    for (ii = 0; ii < nBasicXorus; ii += 2)
        {
        charHint.type = CSP_VSTEM;
        charHint.hint.vStem.x1 = pCspGlobals->pXpix[ii];
        charHint.hint.vStem.x2 = pCspGlobals->pXpix[ii + 1];
        pCspGlobals->rawOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        }

    /* Deliver horizontal stroke hints (primary and secondary) */
    for (ii = 0; ii < nBasicYorus; ii += 2)
        {
        charHint.type = CSP_HSTEM;
        charHint.hint.hStem.y1 = pCspGlobals->pYpix[ii];
        charHint.hint.hStem.y2 = pCspGlobals->pYpix[ii + 1];
        pCspGlobals->rawOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        }
    }

/* Execute secondary hint items */
if (pExtraItems != BTSNULL)
    {
    ufix8  *pByte;
    fix15   nExtraItems;
    fix15   extraItemSize;
    fix15   ii;

    pByte = pExtraItems;
    nExtraItems = (fix15)NEXT_BYTE(pByte);
    for (ii = 0; ii < nExtraItems; ii++)
        {
        extraItemSize = NEXT_BYTE(pByte);
        switch(NEXT_BYTE(pByte))
            {
        case 1:
            DoExtraStrokes(
                pCspGlobals, 
                pByte); 
            break;

        case 2:
            DoExtraEdges(
                pCspGlobals, 
                pByte);
            break;
            }
        pByte += extraItemSize;
        }
    }

/* Set up interpolation tables */
SetInterpolation(
    pCspGlobals, 
    nBasicXorus, 
    nBasicYorus);

/* Compute bitmap position adjustment */
SetPositionAdjustment(
    pCspGlobals,
    nBasicXorus);
}

FUNCTION
static void DoExtraStrokes(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte)
/*
 *  Interprets the given extra data item as a set of secondary
 *  strokes.
 *  Each secondary stroke is appended to the appropriate controlled
 *  coordinate list and aligned. The alignment is similar to the 
 *  standard stroke positioning rules except that floating strokes
 *  are positioned relative to their parent rather than absolutely.
 */ 
{
fix15   ii, jj, kk;
fix15  *pOrus;
fix15   nBasicOrus;
fix15   dimension;
fix15   nStrokes;
#if INCL_COMPRESSED_PFR
fix15	adjModeMask;
#endif

typedef void  (*DoXstrokesPtr)(
                        cspGlobals_t *pCspGlobals,
                        fix15 firstIndex,
                        fix15 lastIndex,
                        fix15 parent);
DoXstrokesPtr DoXstrokes;


pOrus = pCspGlobals->pXorus;
nBasicOrus = jj = pCspGlobals->nXorus;
DoXstrokes = DoVstrokes;
#if INCL_COMPRESSED_PFR
adjModeMask = X_ADJ_ACTIVE;
#endif
for (dimension = 0;;)
    {
    kk = 0;
    nStrokes = (fix15)(NEXT_BYTE(pByte));
    for (ii = 0; ii < nStrokes; ii++)
        {
        /* Read the stroke edges */
        pOrus[jj] = NEXT_WORD(pByte);
        pOrus[jj + 1] = NEXT_WORD(pByte);
        
#if INCL_COMPRESSED_PFR
		/* Adjust the edges using the adjustment vector */
		if (pCspGlobals->adjMode & adjModeMask)
			{
			pOrus[jj] += CspGetAdjustmentValue(pCspGlobals);
			pOrus[jj + 1] += CspGetAdjustmentValue(pCspGlobals);
			}
#endif

        /* Locate the parent stroke */
        while ((kk < nBasicOrus) &&
            (pOrus[jj] > pOrus[kk + 1]))
            {
            kk += 2;
            }

        /* Position the secondary stroke relative to its parent */
        DoXstrokes(
            pCspGlobals, 
            jj, 
            (fix15)(jj + 2), 
            (fix15)(((kk < nBasicOrus) && (pOrus[jj + 1] > pOrus[kk]))? kk: 0));

        /* Deliver secondary stroke hint to output module */
        if (pCspGlobals->rawOutlFns.CharHint != BTSNULL)
            {
            charHint_t charHint;

            if (dimension == 0)
                {
                charHint.type = CSP_VSTEM2;
                charHint.hint.vStem.x1 = pCspGlobals->pXpix[jj];
                charHint.hint.vStem.x2 = pCspGlobals->pXpix[jj + 1];
                }
            else
                {
                charHint.type = CSP_HSTEM2;
                charHint.hint.hStem.y1 = pCspGlobals->pYpix[jj];
                charHint.hint.hStem.y2 = pCspGlobals->pYpix[jj + 1];
                }
            pCspGlobals->rawOutlFns.CharHint(
                pCspGlobals,
                &charHint);
            }

        jj += 2;
        }
    if (dimension == 0)
        {
#if INCL_ANTIALIASED_OUTPUT
		if ((pCspGlobals->outputSpecs.outputType != ANTIALIASED_ALT1_OUTPUT) ||
			((pCspGlobals->outputSpecs.specs.pixmap1.xHintMode & (NO_PRIMARY_STROKES | NO_SEC_STROKES)) == 0))
#endif
        	{
	        pCspGlobals->nXorus = jj;
	        }
        dimension = 1;
        pOrus = pCspGlobals->pYorus;
        nBasicOrus = jj = pCspGlobals->nYorus;
        DoXstrokes = DoHstrokes;
#if INCL_COMPRESSED_PFR
		adjModeMask = Y_ADJ_ACTIVE;
#endif
        continue;
        }
    else
        {
#if INCL_ANTIALIASED_OUTPUT
		if ((pCspGlobals->outputSpecs.outputType != ANTIALIASED_ALT1_OUTPUT) ||
			((pCspGlobals->outputSpecs.specs.pixmap1.yHintMode & (NO_PRIMARY_STROKES | NO_SEC_STROKES)) == 0))
#endif
			{
        	pCspGlobals->nYorus = jj;
        	}
        break;
        }
    }

#if INCL_ANTIALIASED_OUTPUT
if ((pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT) &&
	(pCspGlobals->outputSpecs.specs.pixmap1.xHintMode & (NO_PRIMARY_STROKES | NO_SEC_STROKES)))
	{
	return;
	}
#endif

}

FUNCTION
static void DoExtraEdges(
    cspGlobals_t *pCspGlobals,
    ufix8 *pByte)
/*
 *  Interprets the given extra data item as a set of secondary edges.
 *  Each secondary edge causes one additional controlled coordinate to
 *  be appended to the appropriate controlled coordinate list.
 *  Its pixel value is aligned relative to its parent edge.
 */ 
{
fix15   ii, jj, kk;
fix15   *pOrus;
fix15   *pPix;
fix15   dimension;
fix15   nEdges;
ufix8   format;
fix15   thresh;
fix15   deltaOrus;
fix15   deltaPix;
fix15   threshPix;
fix15   parentPix;
#if INCL_COMPRESSED_PFR
fix15	adjModeMask;
#endif

pOrus = pCspGlobals->pXorus;
pPix = pCspGlobals->pXpix;
jj = pCspGlobals->nXorus;
#if INCL_COMPRESSED_PFR
adjModeMask = X_ADJ_ACTIVE;
#endif
for (dimension = 0;;)
    {
    kk = 0;
    nEdges = (fix15)(NEXT_BYTE(pByte));
    for (ii = 0; ii < nEdges; ii++)
        {
        /* Read the secondary edge, its parent and threshold */
        format = NEXT_BYTE(pByte);
        if ((format & BIT_7) == 0)
            {
            kk += (fix15)(format >> 4);
            thresh = 1 << 4;
            deltaOrus = (fix15)((fix7)(format << 4)) >> 4;
            }
        else
            {
            kk = (fix15)(format & 0x3f) - 1;
            if (kk < 0)
                {
                kk = (fix15)(NEXT_BYTE(pByte));
                }
            if (format & BIT_6)
                {
                thresh = 1 << 4;
                }
            else
                {
                thresh = (fix15)NEXT_BYTE(pByte);
                }
            deltaOrus = (fix15)((fix7)(NEXT_BYTE(pByte)));
            if (deltaOrus == 0)
                {
                deltaOrus = NEXT_WORD(pByte);
                }
            }
            
#if INCL_COMPRESSED_PFR
		/* Adjust the thresh and delta oru values using the adjustment vector */
		if (pCspGlobals->adjMode & adjModeMask)
			{
			thresh += CspGetAdjustmentValue(pCspGlobals);
			deltaOrus += CspGetAdjustmentValue(pCspGlobals);
			}
#endif

        /* Add the secondary edge to the controlled oru table */
        pOrus[jj] = pOrus[kk] + deltaOrus;

        /* Position the secondary edge relative to its parent */
        deltaPix =
            (fix15)(
             (((fix31)deltaOrus * 
               sp_globals.tcb.xppo) + 
              sp_globals.mprnd) >>
             sp_globals.mpshift);
        threshPix = (fix15)(
            ((fix31)thresh << 
             sp_globals.pixshift) >> 4);
        if ((deltaPix < threshPix) &&
            (deltaPix > -threshPix))
            {
            deltaPix = 0;
            }
        else
            {
            deltaPix = 
                (deltaPix + sp_globals.pixrnd) & 
                sp_globals.pixfix;
            }
        parentPix = pPix[kk];
        pPix[jj] = parentPix + deltaPix;

        /* Deliver edge hint to output module */
        if (pCspGlobals->rawOutlFns.CharHint != BTSNULL)
            {
            charHint_t charHint;

            if (dimension == 0)
                {
                charHint.type = ((kk & 1) == 0)? 
                    CSP_V_LEAD_EDGE2:
                    CSP_V_TRAIL_EDGE2; 
                charHint.hint.vEdge.x = parentPix;
                charHint.hint.vEdge.dx = deltaPix;
                charHint.hint.vEdge.thresh = thresh;
                }
            else
                {
                charHint.type = ((kk & 1) == 0)? 
                    CSP_H_LEAD_EDGE2:
                    CSP_H_TRAIL_EDGE2; 
                charHint.hint.hEdge.y = parentPix;
                charHint.hint.hEdge.dy = deltaPix;
                charHint.hint.hEdge.thresh = thresh;
                }
            pCspGlobals->rawOutlFns.CharHint(
                pCspGlobals,
                &charHint);
            }

        jj += 1;
        }

    if (dimension == 0)
        {
#if INCL_ANTIALIASED_OUTPUT
		if ((pCspGlobals->outputSpecs.outputType != ANTIALIASED_ALT1_OUTPUT) ||
			((pCspGlobals->outputSpecs.specs.pixmap1.xHintMode & (NO_PRIMARY_STROKES | NO_SEC_STROKES | NO_SEC_EDGES)) == 0))
#endif
			{
        	pCspGlobals->nXorus = jj;
        	}
        dimension = 1;
        pOrus = pCspGlobals->pYorus;
        pPix = pCspGlobals->pYpix;
        jj = pCspGlobals->nYorus;
#if INCL_COMPRESSED_PFR
		adjModeMask = Y_ADJ_ACTIVE;
#endif
        continue;
        }
    else
        {
#if INCL_ANTIALIASED_OUTPUT
		if ((pCspGlobals->outputSpecs.outputType != ANTIALIASED_ALT1_OUTPUT) ||
			((pCspGlobals->outputSpecs.specs.pixmap1.yHintMode & (NO_PRIMARY_STROKES | NO_SEC_STROKES | NO_SEC_EDGES)) == 0))
#endif
			{
        	pCspGlobals->nYorus = jj;
        	}
        break;
        }
    }
}

FUNCTION
static void DoVstrokes(
    cspGlobals_t *pCspGlobals,
    fix15 firstIndex,
    fix15 lastIndex,
    fix15 iParent)
/*
 *  Forms vertical strokes from the specified range of controlled 
 *  X coordinates.
 *  Assigns a pixel position to each edge of each stroke.
 */
{
fix15   ii, jj;
fix15   leftOrus, rightOrus;
fix15   widthPix;
fix31   pix;
fix15   orus;

for (ii = firstIndex; ii < lastIndex; ii += 2)
    {
    leftOrus = pCspGlobals->pXorus[ii];
    rightOrus = pCspGlobals->pXorus[ii + 1];

    /* Compute stroke weight in sub-pixel units */
    widthPix =
        (fix15)(
         (((fix31)(rightOrus - leftOrus) * 
           sp_globals.tcb.xppo) + 
          sp_globals.mprnd) >>
         sp_globals.mpshift);

	/* Snap to standard stroke weight if within range */
	for (jj = 0; jj < pCspGlobals->nStemSnapVzones; jj++)
		{
		if (widthPix < pCspGlobals->pStemSnapVzones[jj].minPix)
			{
			break;
			}
		if (widthPix <= pCspGlobals->pStemSnapVzones[jj].maxPix)
			{
			widthPix = pCspGlobals->pStemSnapVzones[jj].refPix;
			break;
			}
		}
		
    /* Round to whole pixels; min 1 pixel */
    widthPix = (widthPix < sp_globals.onepix)?
    	sp_globals.onepix:
		(widthPix + sp_globals.pixrnd) & sp_globals.pixfix;
    	
 
    /* Position stroke for optimum centerline accuracy */
    if (iParent >= 0)
        {
        orus = (fix15)(
            ((fix31)leftOrus + 
             rightOrus -
             pCspGlobals->pXorus[iParent] -
             pCspGlobals->pXorus[iParent + 1]) >> 1);
        pix =
            (((fix31)pCspGlobals->pXpix[iParent] +
              pCspGlobals->pXpix[iParent + 1] -
              widthPix) << 
             sp_globals.mpshift) >> 1;
        }
    else
        {
        orus = (fix15)(
            ((fix31)leftOrus + rightOrus) >> 1);
        pix = 
            sp_globals.tcb.xpos - 
            (((fix31)widthPix << sp_globals.mpshift) >> 1);
        }
    pCspGlobals->pXpix[ii] = (fix15)(
        ((((fix31)orus * sp_globals.tcb.xppo +
           pix) >>
          sp_globals.mpshift) +
         sp_globals.pixrnd) & 
        sp_globals.pixfix);
    pCspGlobals->pXpix[ii + 1] = pCspGlobals->pXpix[ii] + widthPix;
    }
}

FUNCTION
static void DoHstrokes(
    cspGlobals_t *pCspGlobals,
    fix15 firstIndex,
    fix15 lastIndex,
    fix15 iParent)
/*
 *  Forms horizontal strokes from specified range of controlled 
 *  Y coordinates.
 *  Splits the blue value list into bottom zones and top zones.
 *  Determines alignment by merging the stroke list with the blue
 *  zone lists.
 *  Assigns a pixel position to each edge of each stroke.
 */
{
fix15  	ii, jj, kk, ll;
fix15   nBottomZones, nTopZones;
fix15   alignMode;
fix15   bottomOrus, topOrus;
fix15   bottomPix, topPix;
fix15   overshootPix;
fix15   alignBottomPix, alignTopPix;
fix15   heightPix;
fix15   orus;
fix31   pix;

if (pCspGlobals->nBlueZones <= 0)
    {
    nBottomZones = 0;
    nTopZones = 0;
    jj = kk = 0;
    }
else
    {
    nBottomZones = 1;
    nTopZones = pCspGlobals->nBlueZones;
    jj = 0;
    kk = 1;
    }

for (ii = firstIndex; ii < lastIndex; ii += 2)
    {
    bottomOrus = pCspGlobals->pYorus[ii];
    topOrus = pCspGlobals->pYorus[ii + 1];
    alignMode = FLOATING;

    /* Align bottom edge of stroke if within bottom zone */
    if (jj < nBottomZones)      /* At least one bottom zone active? */
        {
        /* Calculate bottom of stroke in unrounded device coordinates */
        bottomPix = (fix15)(
            (((fix31)bottomOrus *
              sp_globals.tcb.yppo) + 
             sp_globals.tcb.ypos) >>
            sp_globals.mpshift);
    
        /* Align bottom edge of stroke if in bottom blue zone */
        while ((jj < nBottomZones) &&
            (bottomPix > pCspGlobals->pBlueZones[jj].maxPix))
            {
            jj++;
            }
        if ((jj < nBottomZones) &&
            (bottomPix >= pCspGlobals->pBlueZones[jj].minPix))
            {
            alignMode |= BOTTOM_ALIGNED;
            overshootPix = 
                sp_globals.tcb.suppressOvershoots?
                0:
                (pCspGlobals->pBlueZones[jj].refPix - 
                 bottomPix + 
                 sp_globals.pixrnd) &
                sp_globals.pixfix;
            alignBottomPix = 
                ((pCspGlobals->pBlueZones[jj].refPix + 
                  sp_globals.pixrnd) & 
                 sp_globals.pixfix)  - 
                overshootPix;
            }
        }

    /* Align top edge of stroke if within top zone */
    if (kk < nTopZones)         /* At least one top zone active? */
        {
        /* Calculate top of stroke in unrounded device coordinates */
        topPix = (fix15)(
    	     (((fix31)topOrus *
    	       sp_globals.tcb.yppo) + 
    	      sp_globals.tcb.ypos) >>
    	     sp_globals.mpshift);

        /* Align top edge of stroke if in top blue zone */
        while ((kk < nTopZones) &&
            (topPix > pCspGlobals->pBlueZones[kk].maxPix))
            {
            kk++;
            }
        if ((kk < nTopZones) &&
            (topPix >= pCspGlobals->pBlueZones[kk].minPix))
            {
            alignMode |= TOP_ALIGNED;
            if (sp_globals.tcb.suppressOvershoots)
                {
                overshootPix = 0;
                }
            else
                {
                overshootPix = 
                    topPix - 
                    pCspGlobals->pBlueZones[kk].refPix;
                
                overshootPix = 
                	((overshootPix < sp_globals.pixrnd) &&
                     (overshootPix >= pCspGlobals->blueShiftPix))?
                    sp_globals.onepix:
                	(overshootPix + sp_globals.pixrnd) & sp_globals.pixfix;
                }

            alignTopPix = 
                ((pCspGlobals->pBlueZones[kk].refPix +
                  pCspGlobals->bluePixRnd) &
                 sp_globals.pixfix) + 
                overshootPix;
            }
        }
								
    if (topOrus == bottomOrus)	/* Ghost stroke? */
    	{
    	heightPix = 0;
    	}
    else
    	{
    	/* Compute stroke weight in sub-pixel units */
	    heightPix = (fix15)(
	         (((fix31)(topOrus - bottomOrus) * 
	           sp_globals.tcb.yppo) + 
	          sp_globals.mprnd) >>
	         sp_globals.mpshift);
	    
		/* Snap to standard stroke weight if within range */
		for (ll = 0; ll < pCspGlobals->nStemSnapHzones; ll++)
			{
			if (heightPix < pCspGlobals->pStemSnapHzones[ll].minPix)
				{
				break;
				}
			if (heightPix <= pCspGlobals->pStemSnapHzones[ll].maxPix)
				{
				heightPix = pCspGlobals->pStemSnapHzones[ll].refPix;
				break;
				}
			}
			 
	    /* Round to whole pixels; min 1 pixel */
    	 heightPix = (heightPix < sp_globals.onepix)?
    	sp_globals.onepix:
		(heightPix + sp_globals.pixrnd) & sp_globals.pixfix;
		}

    /* Position stroke as appropriate for alignment mode */
    switch(alignMode)
        {
    case FLOATING:
        if (iParent >= 0)
            {
            orus = (fix15)(
                ((fix31)bottomOrus + 
                 topOrus -
                 pCspGlobals->pYorus[iParent] -
                 pCspGlobals->pYorus[iParent + 1]) >> 1);
            pix =
                (((fix31)pCspGlobals->pYpix[iParent] +
                  pCspGlobals->pYpix[iParent + 1] -
                  heightPix) << 
                 sp_globals.mpshift) >> 1;
            }
        else
            {
            orus = (fix15)(
                ((fix31)bottomOrus + topOrus) >> 1);
            pix = 
                sp_globals.tcb.ypos - 
                (((fix31)heightPix << sp_globals.mpshift) >> 1);
            }

        pCspGlobals->pYpix[ii] = (fix15)(
            ((((fix31)orus * sp_globals.tcb.yppo +
               pix) >>
              sp_globals.mpshift) +
             sp_globals.pixrnd) & 
            sp_globals.pixfix);
        pCspGlobals->pYpix[ii + 1] = pCspGlobals->pYpix[ii] + heightPix;
        break;

    case BOTTOM_ALIGNED:
        pCspGlobals->pYpix[ii] = alignBottomPix;
        pCspGlobals->pYpix[ii + 1] = alignBottomPix + heightPix;
        break;

    case TOP_ALIGNED:
        pCspGlobals->pYpix[ii] = alignTopPix - heightPix;
        pCspGlobals->pYpix[ii + 1] = alignTopPix;
        break;

    case BOTTOM_ALIGNED + TOP_ALIGNED:
        pCspGlobals->pYpix[ii] = alignBottomPix;
        pCspGlobals->pYpix[ii + 1] = alignTopPix;
        break;
        }
    }
}

FUNCTION
static void SetInterpolation(
    cspGlobals_t *pCspGlobals,
    fix15 nBasicXorus,
    fix15 nBasicYorus)
/*
 *  Sets up interpolation coefficients for each zone in the
 *  piecewise linear transformation tables for X and Y.
 */
{
fix15   ppo;
fix31   pos;
fix15   nBasicOrus;
fix15   nOrus;
fix15  *pOrus;
fix15  *pPix;
fix15  *pIntOrus;
fix15  *pIntMult;
fix31  *pIntOffset;
fix15   dimension;
fix15   nIntOrus;
fix15   ii, jj, kk;
btsbool extended;
fix15   deltaOrus;
fix15   orus;
fix15   pix;

ppo = sp_globals.tcb.xppo;
pos = sp_globals.tcb.xpos;
nBasicOrus = nBasicXorus;
nOrus = pCspGlobals->nXorus;
pOrus = pCspGlobals->pXorus;
pPix = pCspGlobals->pXpix;
pIntOrus = pCspGlobals->pXintOrus;
pIntMult = pCspGlobals->pXintMult;
pIntOffset = pCspGlobals->pXintOffset;
for (dimension = 0;;)           /* For X and Y ... */
    {
#if CSP_DEBUG >= 2
        {
        fix15 onemult = 1 << sp_globals.multshift;

        printf("\n%c tables\n", (dimension == 0)? 'X': 'Y');
        printf("  ORUS       PIX\n");

        for (ii = 0; ii < nOrus; ii++)
            {
            printf("%6d %9.3f\n", 
                pOrus[ii], 
                (real)pPix[ii] / sp_globals.onepix); 
            }
        }
#endif

    if (nOrus == 0)
        {
        pIntMult[0] = ppo;
        pIntOffset[0] = pos + sp_globals.mprnd;
        nIntOrus = 0;
        }
    else
        {
        /* Merge secondary hints into pre-sorted basic oru list */
        extended = nOrus > nBasicOrus;
        if (extended)
            {
            for (ii = 0; ii < nBasicOrus; ii++)
                {
                pIntOrus[ii] = ii;
                }
            for (ii = nBasicOrus; ii < nOrus; ii++)
                {
                orus = pOrus[ii];
                for (jj = ii; jj > 0; jj--)
                    {
                    if (orus >= pOrus[pIntOrus[jj - 1]])
                        {
                        break;
                        }
                    pIntOrus[jj] = pIntOrus[jj - 1];
                    }
                pIntOrus[jj] = ii;
                }
#if CSP_DEBUG >= 2
                {
                fix15 onemult = 1 << sp_globals.multshift;

                printf("\nSorted %c tables\n", (dimension == 0)? 'X': 'Y');
                printf("  ORUS       PIX\n");
        
                for (ii = 0; ii < nOrus; ii++)
                    {
                    printf("%6d %9.3f\n", 
                        pOrus[pIntOrus[ii]], 
                        (real)pPix[pIntOrus[ii]] / sp_globals.onepix); 
                    }
                }
#endif

            }

        /* First entry in interpolation table */
        if (extended)
            {
            kk = pIntOrus[0];
            pIntOrus[0] = orus = pOrus[kk];
            pix = pPix[kk];
            }
        else
            {
            pIntOrus[0] = orus = pOrus[0];
            pix = pPix[0];
            }
        pIntMult[0] = ppo;
        pIntOffset[0] = 
            ((fix31)pix << sp_globals.mpshift) - 
            ((fix31)orus * ppo) +
            sp_globals.mprnd;

        /* Remaining entries except last in int table */
        for (ii = jj = 1; ii < nOrus; ii++)
            {
            kk = extended? pIntOrus[ii]: ii;
            deltaOrus = pOrus[kk] - orus;
            if (deltaOrus > 0)
                {
                pIntOrus[jj] = orus = pOrus[kk];
                pIntMult[jj] = (fix15)(
                     ((fix31)(pPix[kk] - pix) << 
                      sp_globals.mpshift) / 
                     (fix31)deltaOrus);
                pix = pPix[kk];
                pIntOffset[jj] = 
                    ((fix31)pix << sp_globals.mpshift) - 
                    ((fix31)orus * pIntMult[jj]) +
                    sp_globals.mprnd;
                if ((pIntMult[jj] == pIntMult[jj - 1]) &&
                    (pIntOffset[jj] == pIntOffset[jj - 1]))
                    {
                    pIntOrus[jj - 1] = orus;
                    }
                else
                    {
                    jj++;
                    }
                }
            }

        /* Last entry in interpolation table */
        pIntMult[jj] = ppo;
        pIntOffset[jj] = 
            ((fix31)pix << sp_globals.mpshift) - 
            (fix31)orus * ppo +
            sp_globals.mprnd;
        nIntOrus =
            ((pIntMult[jj] != pIntMult[jj - 1]) ||
             (pIntOffset[jj] != pIntOffset[jj - 1]))?
            jj:
            jj - 1;
        }

#if CSP_DEBUG >= 2
        {
        fix15 onemult = 1 << sp_globals.multshift;

        printf("\n%c interpolation table\n", (dimension == 0)? 'X': 'Y');
        printf("  ORUS     MULT   OFFSET\n");

        for (ii = 0; ii < nIntOrus; ii++)
            {
            printf("%6d %8.4f %8.4f\n", 
                pIntOrus[ii], 
                (real)pIntMult[ii] / onemult, 
                (real)pIntOffset[ii] / onemult);
            }
        printf("       %8.4f %8.4f\n", 
            (real)pIntMult[nIntOrus] / onemult, 
            (real)pIntOffset[nIntOrus] / onemult);
        }
#endif

    if (dimension == 0)
        {
        pCspGlobals->nXintOrus = nIntOrus;
        dimension = 1;
        ppo = sp_globals.tcb.yppo;
        pos = sp_globals.tcb.ypos;
        nBasicOrus = nBasicYorus;
        nOrus = pCspGlobals->nYorus;
        pOrus = pCspGlobals->pYorus;
        pPix = pCspGlobals->pYpix;
        pIntOrus = pCspGlobals->pYintOrus;
        pIntMult = pCspGlobals->pYintMult;
        pIntOffset = pCspGlobals->pYintOffset;
        continue;
        }
    else
        {
        pCspGlobals->nYintOrus = nIntOrus;
        break;
        }
    }
}

FUNCTION
static void SetPositionAdjustment(
	cspGlobals_t *pCspGlobals,
    fix15 nBasicXorus)
/*
 *	Computes the optimum adjustment to X position to compensate for the
 *  rounding errors associated with vertical primary stroke positioning.
 *	Uses the first left edge and the last right edge that falls within the
 *	X range covered by the character escapement. The adjustment is zero if
 *	the escapement is not horizontal or primary vertical stroke hints
 *	are disabled.
 */
{
fix15	charSetWidthOrus;
fix15	leftOrus;
fix15	rightOrus;
fix15	index[2];
fix31	adj;
fix15   ii, jj, nn;
fix15   xOrus;
fix31	xPixLinear, xPixActual;

sp_globals.rnd_xmin = 0;

/* No adjustment if not horizontal escapement */
if (pCspGlobals->verticalEscapement)
	{
    return;
	}
	
#if INCL_ANTIALIASED_OUTPUT
if ((pCspGlobals->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT) &&
	(pCspGlobals->outputSpecs.specs.pixmap1.xHintMode & NO_PRIMARY_STROKES))
	{
    return;
	}
#endif

/* Compute escapement range of X values */
charSetWidthOrus = (fix15)(
	((pCspGlobals->charSetWidth * 
      pCspGlobals->fontInfo.outlineResolution) + 
     0x8000) >> 
    16);
if (charSetWidthOrus >= 0)
	{
	leftOrus = 0;
	rightOrus = charSetWidthOrus;
	}
else
	{
	leftOrus = charSetWidthOrus;
	rightOrus = 0;
	}

/* Find extreme left and right control edges within escapement range */
index[0] = -1;
index[1] = -1;
for (ii = 0; ii < nBasicXorus; ii += 2)
	{
	xOrus = pCspGlobals->pXorus[ii];
	if ((index[0] < 0) && (xOrus >= leftOrus) && (xOrus <= rightOrus))
		{
		index[0] = ii;
		}
	xOrus = pCspGlobals->pXorus[ii + 1];
	if ((xOrus >= leftOrus) && (xOrus <= rightOrus))
		{
		index[1] = ii + 1;
		}
	}
	
/* Accumulate adjustment from extreme edges */
adj = 0;
nn = 0;
for (ii = 0; ii < 2; ii++)
	{
	jj = index[ii];
	if (jj >= 0)
		{
		xOrus = pCspGlobals->pXorus[jj];
		xPixActual = pCspGlobals->pXpix[jj];
		xPixLinear = 
        	(((fix31)xOrus * sp_globals.tcb.xppo) + 
              sp_globals.mprnd) >>
               sp_globals.mpshift;
		adj += xPixLinear - xPixActual;
		nn++;
		}
	}

/* Compute mean adjustment */
if (nn > 0)
	{
	sp_globals.rnd_xmin = (adj + (nn >> 1)) / nn;
	}
}

#else

FUNCTION
void CspDoStrokes(
    cspGlobals_t *pCspGlobals,
    ufix8 *pExtraItems)
/*
 *  Assigns pixel positions for vertical and horizontal stroke edges
 *  Sets up interpolation coefficients for each zone in the piecewise-
 *  linear transformation.
 */
{
fix15	ii;

/* Position primary vertical strokes */
for (ii = 0; ii < pCspGlobals->nXorus; ii++)
	{
    pCspGlobals->pXpix[ii] = (fix15)(
    	(((fix31)pCspGlobals->pXorus[ii] *
    	  sp_globals.tcb.xppo) + 
         sp_globals.tcb.xpos) >>
        sp_globals.mpshift);
	}

/* Position primary horizontal strokes */
for (ii = 0; ii < pCspGlobals->nYorus; ii++)
	{
    pCspGlobals->pYpix[ii] = (fix15)(
    	(((fix31)pCspGlobals->pYorus[ii] *
    	  sp_globals.tcb.yppo) + 
         sp_globals.tcb.ypos) >>
        sp_globals.mpshift);
	}

/* Set up interpolation tables */
pCspGlobals->nXintOrus = 0;
pCspGlobals->pXintMult[0] = sp_globals.tcb.xppo;
pCspGlobals->pXintOffset[0] = sp_globals.tcb.xpos + sp_globals.mprnd;

pCspGlobals->nYintOrus = 0;
pCspGlobals->pYintMult[0] = sp_globals.tcb.yppo;
pCspGlobals->pYintOffset[0] = sp_globals.tcb.ypos + sp_globals.mprnd;

sp_globals.rnd_xmin = 0;
}

#endif /* INCL_CSPHINTS */

#endif /*PROC_TRUEDOC*/

