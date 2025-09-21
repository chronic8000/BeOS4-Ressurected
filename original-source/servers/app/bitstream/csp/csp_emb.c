/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ E M B . C *******************************
 *                                                                           *
 *  Character outline emboldening logic.                                     *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_EMB.C 7.5 1997/03/18 16:04:24 SHAWN release $
 *                                                                                    
 *     $Log: CSP_EMB.C $
 *     Revision 7.5  1997/03/18 16:04:24  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.4  1997/02/06 19:24:02  MARK
 *     Added test in Intersect to use midpoint if the adjacent points are
 *     within 2 subpixel units.  This avoids potential downstream overflow.
 *     Revision 7.3  1996/10/29 22:37:42  john
 *     Changed all outline interface functions to static.
 *     Added a public struct containing the entry points.
 * Revision 7.2  96/10/22  14:39:02  john
 * Modified ProcSegment() for improved handling of negative
 *     emboldening at points adjacent to curves.
 * 
 * Revision 7.1  96/10/10  14:05:00  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:34  mark
 * Release
 * 
 * Revision 5.3  96/07/02  14:13:24  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.2  96/03/21  11:43:47  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:36  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:28  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:07  mark
 * Release
 * 
 * Revision 2.2  95/12/27  11:31:27  john
 * Fixed bug in miter join sanity check.
 * 
 * Revision 2.1  95/12/21  09:46:04  mark
 * Release
 * 
 * Revision 1.5  95/12/12  14:21:05  john
 * Removed unreferenced local variables.
 * Added explicit casts to 4 conversions from fix31 to fix15.
 * 
 * Revision 1.4  95/11/16  15:41:38  roberte
 * Block out file for TPS build with PROC_TRUEDOC == 0
 * 
 * Revision 1.3  95/11/07  13:45:52  john
 * Extended hint processing function to handle secondary
 * edge hints.
 * 
 * Revision 1.2  95/08/31  09:16:12  john
 * Major rewrite of contour processing functions. Direct curve
 *     emboldening is now supported.
 * Round line join is now supported.
 * 
 * Revision 1.1  95/08/10  16:44:51  john
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
#if INCL_STROKE || INCL_BOLD

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

#define MAX_LINE_JOIN_SEGS 2
#define rnd16 0x00008000
#define SHORTLONGMUL(shortval,longfrac) ((fix31)((fix31)(shortval) * (longfrac)))


/* Local function prototypes */
LOCAL_PROTO 
btsbool CspInitEmbolden(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg);

LOCAL_PROTO 
btsbool CspBeginCharEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO 
void CspBeginSubCharEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO 
void CspCharHintEmbolden(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint);

LOCAL_PROTO 
void CspBeginContourEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside);

LOCAL_PROTO 
void CspCurveEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    point_t P2, 
    point_t P3, 
    fix15 depth);

LOCAL_PROTO 
void CspLineEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1);

LOCAL_PROTO 
void CspEndContourEmbolden(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO 
btsbool CspEndSubCharEmbolden(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO 
btsbool CspEndCharEmbolden(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO 
void ProcSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg);

LOCAL_PROTO 
fix15 DoMiterLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs);

LOCAL_PROTO 
fix15 DoRoundLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg0, 
    segment_t *pSeg, 
    segment_t *pLineJoinSegs);

LOCAL_PROTO 
fix15 DoSimpleRoundLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs) ;

LOCAL_PROTO 
fix15 DoBevelLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg0, 
    segment_t *pSeg, 
    segment_t *pLineJoinSegs);

LOCAL_PROTO 
void SetConsVectors(
    cspGlobals_t *pCspGlobals,
    point_t *pDirVect,
    point_t *pNormVect,
    point_t *pNormVectOrus,
    point_t *pAdjVect);

LOCAL_PROTO 
void OutputSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg);

LOCAL_PROTO 
int Intersect(
    segment_t *pSeg1,
    segment_t *pSeg2,
    point_t *pPint);

LOCAL_PROTO 
void BuildLineSegment(
    segment_t *pSeg,
    point_t P1,
    point_t P2);

LOCAL_PROTO 
void BuildCurveSegment(
    segment_t *pSeg,
    point_t P1,
    point_t P2,
    point_t P3);

LOCAL_PROTO 
btsbool InvertMatrix(
    cspGlobals_t *pCspGlobals,
    fix15 *pVecLenTransX,
    fix15 *pVecLenTransY);

LOCAL_PROTO 
ufix32 isqrt(
    ufix32 x);

LOCAL_PROTO 
long LongDiv(
    long num,
    long denom);

#if CSP_DEBUG
LOCAL_PROTO 
void ShowSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg);
#endif

/* Public structure of function pointers for embolden processor */
intOutlineFns_t EmboldenProcessor =
    {
    CspInitEmbolden,
    CspBeginCharEmbolden,
    CspBeginSubCharEmbolden,
    CspCharHintEmbolden,
    CspBeginContourEmbolden,
    CspCurveEmbolden,
    CspLineEmbolden,
    CspEndContourEmbolden,
    CspEndSubCharEmbolden,
    CspEndCharEmbolden
    };

FUNCTION 
static btsbool CspInitEmbolden(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg)
{
fix15   strokeThickness;
fix15   vecLenTransX;   /* Sub-pix per oru for horiz vectors */
fix15   vecLenTransY;   /* Sub-pix per oru for vert vectors */
fix15   vStrokePix, hStrokePix;
fix15   vHalfStrokePix, hHalfStrokePix;
fix15   halfStroke;
real    rMiterLimit;
real    rMiterLimitSquared;
btsbool retValue;

#if CSP_DEBUG  >= 2
printf("\nCspInitEmbolden\n");
printf("    Bold thickness = %d\n", 
    (int)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.strokeThickness);
printf("    Line join = %d\n", 
    (int)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType);
printf("    Miter limit = %3.1f\n",
    (real)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit / 
    65536.0);
#endif

/* Save current transformation matrix (orus to sub-pixels) */
pCspGlobals->matrix[0] = specsarg->ctm[0];
pCspGlobals->matrix[1] = specsarg->ctm[1];
pCspGlobals->matrix[2] = specsarg->ctm[2];
pCspGlobals->matrix[3] = specsarg->ctm[3];

/* Invert transformation matrix if necessary */
retValue = InvertMatrix(
    pCspGlobals,
    &vecLenTransX,
    &vecLenTransY);

strokeThickness = 
    (fix15)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.strokeThickness;

/* Increase stroke thickness to map to at least a half pixel */
if (!pCspGlobals->regular_trans)
    {
    fix15   minHStrokeOrus;
    fix15   minVStrokeOrus;
    fix15   minStrokeOrus;

    minVStrokeOrus = 0;
    if (vecLenTransX > 0)
        {
        minVStrokeOrus = (fix15)(
            ((sp_globals.pixrnd << sp_globals.mpshift) +
             (vecLenTransX >> 1)) /
            vecLenTransX);
        }

    minHStrokeOrus = 0;
    if (vecLenTransY > 0)
        {
        minHStrokeOrus = (fix15)(
            ((sp_globals.pixrnd << sp_globals.mpshift) +
             (vecLenTransY >> 1)) /
            vecLenTransY);
        }

    minStrokeOrus = 
        (minHStrokeOrus <= minVStrokeOrus)?
        minHStrokeOrus:
        minVStrokeOrus;

    if (strokeThickness >= 0)
        {
        if (strokeThickness < minStrokeOrus)
            strokeThickness = minStrokeOrus;
        }
    }

/* Calculate vertical stroke thickness in whole pixels */
vStrokePix = (fix15)(
     (((fix31)strokeThickness * 
       vecLenTransX) + 
      sp_globals.mprnd) >>
     sp_globals.mpshift);
vStrokePix =
    (vStrokePix >= 0) && (vStrokePix < sp_globals.onepix)?
    sp_globals.onepix:
    (vStrokePix + sp_globals.pixrnd) & sp_globals.pixfix;
vHalfStrokePix = (vStrokePix + 1) >> 1;

/* Calculate horizontal stroke thickness in whole pixels */
hStrokePix = (fix15)(
     (((fix31)strokeThickness * 
       vecLenTransY) + 
      sp_globals.mprnd) >>
     sp_globals.mpshift);
hStrokePix = 
    (hStrokePix >= 0) && (hStrokePix < sp_globals.onepix)?
    sp_globals.onepix:
    (hStrokePix + sp_globals.pixrnd) & sp_globals.pixfix;
hHalfStrokePix = (hStrokePix + 1) >> 1;

/* Set horizontal alignment adjustment */
switch(sp_globals.tcb.xmode)
    {
case 0:                         /* Xpix depends on Xorus only */
    pCspGlobals->xGridAdj = vHalfStrokePix;
    break;
        
case 1:                         /* Xpix depends on -Xorus only */
    pCspGlobals->xGridAdj = -vHalfStrokePix;
    break;
        
case 2:                         /* Xpix depends on Yorus only */
    pCspGlobals->xGridAdj = hHalfStrokePix;
    break;

case 3:                         /* Xpix depends on -Yorus only */
    pCspGlobals->xGridAdj = -hHalfStrokePix;
    break;

default:                        /* Xpix depends on Xorus and Yorus */
    pCspGlobals->xGridAdj = 0;
    break;
    }

/* Set vertical alignment adjustment */
switch(sp_globals.tcb.ymode)
    {
case 0:                         /* Ypix depends on Yorus only */
    pCspGlobals->yGridAdj = hHalfStrokePix;
    break;
        
case 1:                         /* Ypix depends on -Yorus only */
    pCspGlobals->yGridAdj = -hHalfStrokePix;
    break;
        
case 2:                         /* Ypix depends on Xorus only */
    pCspGlobals->yGridAdj = vHalfStrokePix;
    break;

case 3:                         /* Ypix depends on -Xorus only */
    pCspGlobals->yGridAdj = -vHalfStrokePix;
    break;

default:                        /* Ypix depends on Xorus and Yorus */
    pCspGlobals->yGridAdj = 0;
    break;
    }

/* Determine if incoming outlines are mirrorred */
pCspGlobals->mirror =
    (CspLongMult(pCspGlobals->matrix[0], pCspGlobals->matrix[3]) - 
     CspLongMult(pCspGlobals->matrix[1], pCspGlobals->matrix[2])) < 0;

/* Set half stroke thickness in oru space */
pCspGlobals->halfStrokeOrus = 
    (strokeThickness + 1) >> 1;

/* Set half stroke thickness in pixel space */
pCspGlobals->halfStrokePix = 
    vHalfStrokePix;

halfStroke = 
    pCspGlobals->regular_trans?
    pCspGlobals->halfStrokePix:
    pCspGlobals->halfStrokeOrus;

/* Set up for selected line join type */
switch (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType)
    {
case MITER_LINE_JOIN:
    rMiterLimit = 
        (real)pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.miterLimit /
        65536.0;
    rMiterLimitSquared = rMiterLimit * rMiterLimit;
    if (rMiterLimitSquared < 1.0)
        rMiterLimitSquared = 1.0;
    pCspGlobals->minDenom = (fix31)(
        (real)(((fix31)halfStroke * halfStroke) << 2) /
        rMiterLimitSquared); 
    break;

case ROUND_LINE_JOIN:
    pCspGlobals->minDenom = 
        ((fix31)halfStroke * halfStroke) << 1;
    break;

default:
case BEVEL_LINE_JOIN:
    break;
    }

/* Initialize downstream output module(s) */
if (pCspGlobals->embOutlFns.InitOut(
    pCspGlobals,
    specsarg))
    return retValue;

return FALSE;                   /* Matrix is singular or InitOut failed */
}

FUNCTION 
static btsbool CspBeginCharEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{
return pCspGlobals->embOutlFns.BeginChar(
    pCspGlobals,
    Psw,
    Pmin,
    Pmax);
}

FUNCTION 
static void CspBeginSubCharEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{
pCspGlobals->embOutlFns.BeginSubChar(
    pCspGlobals,
    Psw,
    Pmin,
    Pmax);
}

FUNCTION
static void CspCharHintEmbolden(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint)
{
charHint_t charHint;

if (pCspGlobals->embOutlFns.CharHint != BTSNULL)
    {
    charHint.type = pCharHint->type;
    switch(pCharHint->type)
        {
    case CSP_VSTEM:
    case CSP_VSTEM2:
        charHint.hint.vStem.x1 = 
            pCharHint->hint.vStem.x1;
        charHint.hint.vStem.x2 = 
            pCharHint->hint.vStem.x2 + (pCspGlobals->xGridAdj << 1);
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        break;

    case CSP_HSTEM:
    case CSP_HSTEM2:
        charHint.hint.hStem.y1 = 
            pCharHint->hint.hStem.y1;
        charHint.hint.hStem.y2 = 
            pCharHint->hint.hStem.y2 + 
            (pCspGlobals->yGridAdj << 1);
        if (pCharHint->hint.hStem.y2 == 
            pCharHint->hint.hStem.y1)  /* Ghost stroke? */
            {
            if (pCharHint->hint.hStem.y1 <= 0)
                {
                charHint.hint.hStem.y2 = 
                    charHint.hint.hStem.y1; 
                }
            else
                {
                charHint.hint.hStem.y1 = 
                    charHint.hint.hStem.y2; 
                }
            }
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        break;

    case CSP_V_LEAD_EDGE2:
        charHint.hint.vEdge.x = 
            pCharHint->hint.vEdge.x;
        charHint.hint.vEdge.dx = 
            pCharHint->hint.vEdge.dx;
        charHint.hint.vEdge.thresh = 
            pCharHint->hint.vEdge.thresh;
        break;

    case CSP_V_TRAIL_EDGE2:
        charHint.hint.vEdge.x = 
            pCharHint->hint.vEdge.x + 
            (pCspGlobals->xGridAdj << 1);
        charHint.hint.vEdge.dx = 
            pCharHint->hint.vEdge.dx;
        charHint.hint.vEdge.thresh = 
            pCharHint->hint.vEdge.thresh;
        break;

    case CSP_H_LEAD_EDGE2:
        charHint.hint.hEdge.y = 
            pCharHint->hint.hEdge.y;
        charHint.hint.hEdge.dy = 
            pCharHint->hint.hEdge.dy;
        charHint.hint.hEdge.thresh = 
            pCharHint->hint.hEdge.thresh;
        break;

    case CSP_H_TRAIL_EDGE2:
        charHint.hint.hEdge.y = 
            pCharHint->hint.hEdge.y + 
            (pCspGlobals->yGridAdj << 1);
        charHint.hint.hEdge.dy = 
            pCharHint->hint.hEdge.dy;
        charHint.hint.hEdge.thresh = 
            pCharHint->hint.hEdge.thresh;
        break;
        }
    }
}

FUNCTION 
static void CspBeginContourEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside)
{
#if CSP_DEBUG >= 2
printf("\nCspBeginContourEmbolden(%3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix);
#endif

pCspGlobals->firstSegment = TRUE;
pCspGlobals->secondSegment = FALSE;
pCspGlobals->firstPoint = TRUE;
pCspGlobals->boldP0 = P1;
}

FUNCTION 
static void CspCurveEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15 depth)
{
segment_t seg;
point_t adjVect1, adjVect2;
fix31   dirChange;

#if CSP_DEBUG  >= 2
printf("\nCspCurveEmbolden(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix,
    (real)P2.x / (real)sp_globals.onepix, 
    (real)P2.y / (real)sp_globals.onepix,
    (real)P3.x / (real)sp_globals.onepix, 
    (real)P3.y / (real)sp_globals.onepix);
#endif

/* Set up curve segment */
seg.type = CSP_CRVE_SEG;

/* Compute initial direction vector */
seg.dirVectIn.x = P1.x - pCspGlobals->boldP0.x;
seg.dirVectIn.y = P1.y - pCspGlobals->boldP0.y;
if ((seg.dirVectIn.x == 0) && 
    (seg.dirVectIn.y == 0))
    {
    seg.dirVectIn.x = P2.x - pCspGlobals->boldP0.x;
    seg.dirVectIn.y = P2.y - pCspGlobals->boldP0.y;
    if ((seg.dirVectIn.x == 0) && 
        (seg.dirVectIn.y == 0))
        {
        seg.dirVectIn.x = P3.x - pCspGlobals->boldP0.x;
        seg.dirVectIn.y = P3.y - pCspGlobals->boldP0.y;
        }
    }

/* Compute final direction vector */
seg.dirVectOut.x = P3.x - P2.x;
seg.dirVectOut.y = P3.y - P2.y;
if ((seg.dirVectOut.x == 0) && 
    (seg.dirVectOut.y == 0))
    {
    seg.dirVectOut.x = P3.x - P1.x;
    seg.dirVectOut.y = P3.y - P1.y;
    if ((seg.dirVectOut.x == 0) && 
        (seg.dirVectOut.y == 0))
        {
        seg.dirVectOut.x = P3.x - pCspGlobals->boldP0.x;
        seg.dirVectOut.y = P3.y - pCspGlobals->boldP0.y;
        }
    }

/* Exit if null curve */
if ((seg.dirVectIn.x == 0) && 
    (seg.dirVectIn.y == 0))
    return;

/* Set up construction vectors */
SetConsVectors(
    pCspGlobals,
    &seg.dirVectIn, 
    &seg.normVectIn,
    &seg.normVectInOrus,
    &adjVect1);
SetConsVectors(
    pCspGlobals,
    &seg.dirVectOut, 
    &seg.normVectOut,
    &seg.normVectOutOrus,
    &adjVect2);

/* Calculate the adjustments to the curve control points */
dirChange =
    ((fix31)seg.dirVectIn.x * 
     (fix31)seg.dirVectOut.y) - 
    ((fix31)seg.dirVectOut.x * 
     (fix31)seg.dirVectIn.y);
if (pCspGlobals->mirror)
    dirChange = - dirChange;
if (dirChange < 0)              /* Curve is towards positive normal? */
    {
    adjVect1.x = -adjVect1.x;
    adjVect1.y = -adjVect1.y;
    }
else                            /* Curve is away from positive normal? */
    {
    adjVect2.x = -adjVect2.x;
    adjVect2.y = -adjVect2.y;
    }

/* Relocate the curve segment */
seg.ptIn.x = 
    pCspGlobals->boldP0.x + seg.normVectIn.x + pCspGlobals->xGridAdj;
seg.ptIn.y = 
    pCspGlobals->boldP0.y + seg.normVectIn.y + pCspGlobals->yGridAdj;
seg.ptC1.x = 
    P1.x + seg.normVectIn.x + adjVect1.x + pCspGlobals->xGridAdj;
seg.ptC1.y = 
    P1.y + seg.normVectIn.y + adjVect1.y + pCspGlobals->yGridAdj;
seg.ptC2.x = 
    P2.x + seg.normVectOut.x + adjVect2.x + pCspGlobals->xGridAdj;
seg.ptC2.y = 
    P2.y + seg.normVectOut.y + adjVect2.y + pCspGlobals->yGridAdj;
seg.ptOut.x = 
    P3.x + seg.normVectOut.x + pCspGlobals->xGridAdj;
seg.ptOut.y = 
    P3.y + seg.normVectOut.y + pCspGlobals->yGridAdj;

/* Process relocated curve segment */
ProcSegment(pCspGlobals, &seg);

pCspGlobals->boldP0 = P3;
}

FUNCTION 
static void CspLineEmbolden(
    cspGlobals_t *pCspGlobals,
    point_t P1)
{
segment_t seg;

#if CSP_DEBUG  >= 2
printf("\nCspLineEmbolden(%3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix);
#endif

seg.type = CSP_LINE_SEG;

seg.dirVectIn.x = P1.x - pCspGlobals->boldP0.x;
seg.dirVectIn.y = P1.y - pCspGlobals->boldP0.y;
seg.dirVectOut = seg.dirVectIn;

/* Exit if null vector */
if ((seg.dirVectIn.x == 0) && (seg.dirVectIn.y == 0))
    return;

/* Add construction vectors */
SetConsVectors(
    pCspGlobals,
    &seg.dirVectIn, 
    &seg.normVectIn,
    &seg.normVectInOrus,
    BTSNULL);
seg.normVectOut = seg.normVectIn;
seg.normVectOutOrus = seg.normVectInOrus;

/* Relocate vector to new position */
seg.ptIn.x = 
    pCspGlobals->boldP0.x + seg.normVectIn.x + pCspGlobals->xGridAdj;
seg.ptIn.y = 
    pCspGlobals->boldP0.y + seg.normVectIn.y + pCspGlobals->yGridAdj;
seg.ptOut.x = 
    P1.x + seg.normVectOut.x + pCspGlobals->xGridAdj;
seg.ptOut.y = 
    P1.y + seg.normVectOut.y + pCspGlobals->yGridAdj;

/* Process the relocated line segment */
ProcSegment(pCspGlobals, &seg);

pCspGlobals->boldP0 = P1;
}

FUNCTION 
static void CspEndContourEmbolden(
    cspGlobals_t *pCspGlobals)
{
#if CSP_DEBUG  >= 2
printf("\nCspEndContourEmbolden\n");
#endif

/* Flush contour output */
ProcSegment(pCspGlobals, BTSNULL);
}

FUNCTION 
static btsbool CspEndSubCharEmbolden(
    cspGlobals_t *pCspGlobals)
{
return pCspGlobals->embOutlFns.EndSubChar(
    pCspGlobals);
}

FUNCTION 
static btsbool CspEndCharEmbolden(
    cspGlobals_t *pCspGlobals)
{
return pCspGlobals->embOutlFns.EndChar(
    pCspGlobals);
}

FUNCTION
static void ProcSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg)
/*
 *  Processes the specified segment.
 *  If this is the first segment in the contour, it is saved in seg0.
 *  If this is the second segment in the contour, the steps are:
 *      construct the line joins between seg0 and the segment
 *      save the updated first segment seg0 in seg00
 *      output the line join segments
 *      save the second segment in seg0
 *  For the remaining segments in the contour, the steps are:
 *      construct the line joins between seg0 and the segment
 *      output the updated seg0
 *      output the line join segments
 *      save the new segment in seg0
 *  If the pointer to the new segment is BTSNULL signifying the end of the
 *  contour, the steps are:
 *      construct the line joins between seg0 and seg00
 *      output the updated seg0
 *      output the line join segments
 *      output the updated seg00
 */
{
fix31   dirChange;
segment_t lineJoinSegs[MAX_LINE_JOIN_SEGS];
fix15   nLineJoinSegs;
point_t Pint;
fix15   x, y;
fix15   ii;

#if CSP_DEBUG >= 2
ShowSegment(pCspGlobals, pSeg);
#endif

/* Special processing for first segment in contour */
if (pCspGlobals->firstSegment)
    {
    if (pSeg != BTSNULL)
        {
        pCspGlobals->seg0 = *pSeg;
        }
    pCspGlobals->firstSegment = FALSE;
    pCspGlobals->secondSegment = TRUE;
    return;
    }

/* Wrap to saved first segment if end of contour */
if (pSeg == BTSNULL)
    {
    pSeg = &(pCspGlobals->seg00);
    }

/* Determine action based on corner type */
if  (((pCspGlobals->regular_trans) && (pCspGlobals->halfStrokePix == 0)) || 
     ((!pCspGlobals->regular_trans) && (pCspGlobals->halfStrokeOrus == 0)))
    {
    goto L3;                    /* No emboldening -- no line join */
    }
dirChange = 
    ((fix31)pCspGlobals->seg0.dirVectOut.x * 
     (fix31)pSeg->dirVectIn.y) - 
    ((fix31)pSeg->dirVectIn.x * 
     (fix31)pCspGlobals->seg0.dirVectOut.y);
if (pCspGlobals->mirror)
    dirChange = -dirChange;
if (pCspGlobals->halfStrokeOrus < 0)
    dirChange = -dirChange;
if (dirChange <= 0)
    {
    if (dirChange == 0)     
        {
        if (((pCspGlobals->seg0.dirVectOut.x ^ pSeg->dirVectIn.x) < 0) ||
            ((pCspGlobals->seg0.dirVectOut.y ^ pSeg->dirVectIn.y) < 0))
            {
            goto L2;            /* 180 degree turn -- convex line join */
            }
        goto L3;                /* No turn -- no line join */
        }
    goto L1;                    /* Turn towards normal -- concave line join */
    }
goto L2;                        /* Turn away from normal -- convex line join */

L1: 
/* Find intersection point */
if (Intersect(&(pCspGlobals->seg0), pSeg, &Pint) != 0)
    goto L3;                    /* No line join if vectors don't intersect */

/* Bevel join if intersection is before start of arriving vector */
if (pCspGlobals->seg0.type != CSP_LINE_SEG)
    {
    x = Pint.x - pCspGlobals->seg0.ptC2.x;
    y = Pint.y - pCspGlobals->seg0.ptC2.y;
    }
else
    {
    x = Pint.x - pCspGlobals->seg0.ptIn.x;
    y = Pint.y - pCspGlobals->seg0.ptIn.y;
    }
if (((fix31)x * pCspGlobals->seg0.dirVectOut.x +
     (fix31)y * pCspGlobals->seg0.dirVectOut.y) < 0)
    goto L1A;

/* Bevel join if intersection is after end of departing vector */
if (pSeg->type != CSP_LINE_SEG)
    {
    x = pSeg->ptC1.x - Pint.x;
    y = pSeg->ptC1.y - Pint.y;
    }
else
    {
    x = pSeg->ptOut.x - Pint.x;
    y = pSeg->ptOut.y - Pint.y;
    }
if (((fix31)x * pSeg->dirVectIn.x +
     (fix31)y * pSeg->dirVectIn.y) < 0)
    goto L1A;

/* Mitered concave line join */
pCspGlobals->seg0.ptOut = pSeg->ptIn = Pint;
nLineJoinSegs = 0;
goto L4;

L1A:
/* Beveled concave line join */
nLineJoinSegs = 1;      /* Bevel line join */
lineJoinSegs[0].type = CSP_LINE_SEG;
lineJoinSegs[0].ptIn = pCspGlobals->seg0.ptOut;
lineJoinSegs[0].ptOut = pSeg->ptIn;
goto L4;

L2: 
/* Convex line join */
switch (pCspGlobals->fontAttributes.styleSpecs.styleSpecsStroked.lineJoinType)
    {
case MITER_LINE_JOIN:
    nLineJoinSegs = DoMiterLineJoin(
        pCspGlobals, 
        &(pCspGlobals->seg0), 
        pSeg, 
        lineJoinSegs);
    break;

case ROUND_LINE_JOIN:
    nLineJoinSegs = DoRoundLineJoin(
        pCspGlobals, 
        &(pCspGlobals->seg0), 
           pSeg, 
         lineJoinSegs);
    break;

case BEVEL_LINE_JOIN:
default:
    nLineJoinSegs = DoBevelLineJoin(
        pCspGlobals, 
        &(pCspGlobals->seg0), 
        pSeg, 
        lineJoinSegs); 
    break;
    }
goto L4;

L3: 
/* No line join */
nLineJoinSegs = 0;

L4: 
/* Save or output segment prior to line join */
if (pCspGlobals->secondSegment) /* Second segment in contour? */
    {
    pCspGlobals->secondSegment = FALSE;

    /* Save first segment for output later */
    pCspGlobals->seg00 = pCspGlobals->seg0;
    }
else
    {
    /* Output the previous segment */
    OutputSegment(pCspGlobals, &(pCspGlobals->seg0));
    }

#if CSP_DEBUG
if (nLineJoinSegs > MAX_LINE_JOIN_SEGS)
    printf("*** ProcSegment: Error 1\n");
#endif

/* Output the line join segments (if any) */
for (ii = 0; ii < nLineJoinSegs; ii++)
    {
    OutputSegment(pCspGlobals, lineJoinSegs + ii);
    }

if (pSeg == &(pCspGlobals->seg00)) /* End of contour? */
    {
    /* Output the first segment in contour */
    OutputSegment(pCspGlobals, pSeg);

    /* Signal the end of the contour */
#if CSP_DEBUG  >= 2
    printf("EndContour ()\n"); 
#endif
    pCspGlobals->embOutlFns.EndContour(
        pCspGlobals);
    }
else                            /* Not end of contour? */
    {
    pCspGlobals->seg0 = *pSeg;               /* Save the current segment */
    }
}

FUNCTION
static fix15 DoMiterLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs) 
/*
 *  Generates a miter line join between the specified segments.
 *  If the preceding segment is a vector, it is extended to the
 *  miter join point. It the following segment is a vector, it
 *  is extended to the miter join point. An extra line segment
 *  is generated in the case of adjacent curve segments.
 *  If the miter limit is exceeded, or if the segments are 
 *  parallel, or if the intersection is before the end of the
 *  first segment or after the start of the second segment,
 *  a bevel join segment is  generated.
 *  Returns the number of line join segments written (0, 1 or 2
 *  for successful miter joins, 1 if a bevel line join is
 *  generated.
 */
{
fix15   x, y;
point_t Pint;
fix15   nLineJoinSegs;

if (pCspGlobals->regular_trans)
    {
    x = pSeg1->normVectOut.x + pSeg2->normVectIn.x;
    y = pSeg1->normVectOut.y + pSeg2->normVectIn.y;
    }
else
    {
    x = pSeg1->normVectOutOrus.x + pSeg2->normVectInOrus.x;
    y = pSeg1->normVectOutOrus.y + pSeg2->normVectInOrus.y;
    }
if (((fix31)x * x + (fix31)y * y) <
    pCspGlobals->minDenom)      /* Exceeds miter limit? */
    {
    goto L1;
    }

/* Find miter intersection point */
if (Intersect(pSeg1, pSeg2, &Pint) != 0)
    goto L1;

/* Bevel join if intersection is before end of first segment */
x = Pint.x - pSeg1->ptOut.x;
y = Pint.y - pSeg1->ptOut.y;
if (((fix31)x * pSeg1->dirVectOut.x +
     (fix31)y * pSeg1->dirVectOut.y) < 0)
    goto L1;

/* Bevel join if intersection is after start of second segment */
x = pSeg2->ptIn.x - Pint.x;
y = pSeg2->ptIn.y - Pint.y;
if (((fix31)x * pSeg2->dirVectIn.x +
     (fix31)y * pSeg2->dirVectIn.y) < 0)
    goto L1;

nLineJoinSegs = 0;
if (pSeg1->type == CSP_LINE_SEG)
    {
    pSeg1->ptOut = Pint;
    }
else
    {
    BuildLineSegment(pLineJoinSegs, pSeg1->ptOut, Pint);
    nLineJoinSegs++;
    }
if (pSeg2->type == CSP_LINE_SEG)
    {
    pSeg2->ptIn = Pint;
    }
else
    {
    BuildLineSegment(pLineJoinSegs + nLineJoinSegs, Pint, pSeg2->ptIn);
    nLineJoinSegs++;
    }

#if CSP_DEBUG
if (nLineJoinSegs > 2)
    printf("*** DoMiterLineJoin: Error 1\n");
#endif

return nLineJoinSegs;

L1: 
return DoBevelLineJoin(
    pCspGlobals,
    pSeg1,
    pSeg2,
    pLineJoinSegs);
}

FUNCTION
static fix15 DoRoundLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs) 
/*
 *  Generates a round line join between the specified segments and
 *  writes the line join segments into the line join segment array.
 *  The line join consists of 1 curve segment (2 segments if the angle 
 *  exceeds 90 degrees).
 *  Returns the number of segments written (1 or 2)
 */
{
fix15   x, y;
segment_t seg;
fix15   nLineJoinSegs;

if (pCspGlobals->regular_trans)
    {
    x = pSeg1->normVectOut.x + pSeg2->normVectIn.x;
    y = pSeg1->normVectOut.y + pSeg2->normVectIn.y;
    }
else
    {
    x = pSeg1->normVectOutOrus.x + pSeg2->normVectInOrus.x;
    y = pSeg1->normVectOutOrus.y + pSeg2->normVectInOrus.y;
    }
if (((fix31)x * x + (fix31)y * y) >=
    pCspGlobals->minDenom)      /* Quarter circle or less? */
    {
    return DoSimpleRoundLineJoin(
        pCspGlobals, pSeg1, pSeg2, pLineJoinSegs);
    }

/* Set up zero-length vector at mid point of arc */
seg.type = CSP_LINE_SEG;
seg.dirVectIn.x = seg.dirVectOut.x =
    pSeg2->normVectIn.x - pSeg1->normVectOut.x;
seg.dirVectIn.y = seg.dirVectOut.y =
    pSeg2->normVectIn.y - pSeg1->normVectOut.y;
SetConsVectors(
    pCspGlobals,
    &seg.dirVectIn, 
    &seg.normVectIn,
    BTSNULL,
    BTSNULL);
seg.normVectOut = seg.normVectIn;
seg.ptIn.x =  seg.ptOut.x =
    pSeg1->ptOut.x -
    pSeg1->normVectOut.x +
    seg.normVectIn.x;
seg.ptIn.y = seg.ptOut.y =
    pSeg1->ptOut.y -
    pSeg1->normVectOut.y +
    seg.normVectIn.y;

/* Construct round line join for first half */
nLineJoinSegs = DoSimpleRoundLineJoin(
    pCspGlobals, pSeg1, &seg, pLineJoinSegs);

/* Construct round line join for second half */
nLineJoinSegs += DoSimpleRoundLineJoin(
    pCspGlobals, &seg, pSeg2, pLineJoinSegs + nLineJoinSegs);

return nLineJoinSegs;
}

FUNCTION
static fix15 DoSimpleRoundLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs) 
/*
 *  Generates a round line join between the specified segments and
 *  writes the line join segment into the line join segment array.
 *  Does a bevel line join if the segments don't intersect or if
 *  the intersection point is before the end of the first segment
 *  or after the start of the second segment.
 *  Returns the number of segments written (always 1)
 */
{
point_t Pint;
fix15   x, y;

if (Intersect(pSeg1, pSeg2, &Pint) != 0)
    goto L1;

/* Bevel join if intersection is before end of first segment */
x = Pint.x - pSeg1->ptOut.x;
y = Pint.y - pSeg1->ptOut.y;
if (((fix31)x * pSeg1->dirVectOut.x +
     (fix31)y * pSeg1->dirVectOut.y) < 0)
    goto L1;

/* Bevel join if intersection is after start of second segment */
x = pSeg2->ptIn.x - Pint.x;
y = pSeg2->ptIn.y - Pint.y;
if (((fix31)x * pSeg2->dirVectOut.x +
     (fix31)y * pSeg2->dirVectOut.y) < 0)
    goto L1;

BuildCurveSegment(
    pLineJoinSegs, 
    pSeg1->ptOut, 
    Pint,
    pSeg2->ptIn);

    return 1;

L1:
return DoBevelLineJoin(
    pCspGlobals,
    pSeg1,
    pSeg2,
    pLineJoinSegs);
}

FUNCTION
static fix15 DoBevelLineJoin(
    cspGlobals_t *pCspGlobals, 
    segment_t *pSeg1, 
    segment_t *pSeg2, 
    segment_t *pLineJoinSegs) 
/*
 *  Generates a bevel line join between the specified segments and
 *  writes the line join segment into the line join segment array.
 *  Note that only type and point information is written into line
 *  join segments.
 *  Returns the number of segments written (always 1)
 */
{
pLineJoinSegs->type = CSP_LINE_SEG;
pLineJoinSegs->ptIn = pSeg1->ptOut;
pLineJoinSegs->ptOut = pSeg2->ptIn;

return 1;
}

FUNCTION
static void SetConsVectors(
    cspGlobals_t *pCspGlobals,
    point_t *pDirVect,
    point_t *pNormVect,
    point_t *pNormVectOrus,
    point_t *pAdjVect)
/*
 *  Computes the vector normal to the specified direction vector 
 *  (which must have non-zero length) whose length is equal to half
 *  the current stroke thickness.
 *  Also computes a vector parallel to the specified direction
 *  vector whose length is also half the stroke thickness.
 */
{
fix15   dx, dy;
fix31   len;
fix15   xAdj, yAdj;

dx = pDirVect->x;
dy = pDirVect->y;

if (pCspGlobals->regular_trans) /* Regular transformation matrix? */
    {
    if (dx == 0)
        {
        xAdj = 0;
        yAdj = dy >= 0? 
            pCspGlobals->halfStrokePix: 
            -pCspGlobals->halfStrokePix;
        }
    else if (dy == 0)
        {
        xAdj = dx >= 0? 
            pCspGlobals->halfStrokePix: 
            -pCspGlobals->halfStrokePix;
        yAdj = 0;
        }
    else
        {
        len = isqrt((fix31)dx * dx + (fix31)dy * dy);
        xAdj = (fix15)(
            ((fix31)dx * 
             pCspGlobals->halfStrokePix) / 
            len);
        yAdj = (fix15)(
            ((fix31)dy * 
             pCspGlobals->halfStrokePix) / 
            len);
        }

    if (pNormVect != BTSNULL)
        {
        if (pCspGlobals->mirror)
            {
            pNormVect->x = -yAdj;
            pNormVect->y =  xAdj;
            }
        else
            {
            pNormVect->x =  yAdj;
            pNormVect->y = -xAdj;
            }
        }

    if (pAdjVect != BTSNULL)
        {
        pAdjVect->x = xAdj >> 1;
        pAdjVect->y = yAdj >> 1;
        }
    }
else                            /* Irregular transformation matrix? */
    {
    fix31   xx, yy;
    fix31   ax, ay, maxabs;
    fix15   x, y;
    fix31   round;

    xx = SHORTLONGMUL(dx, pCspGlobals->imatrix[0]) + 
         SHORTLONGMUL(dy, pCspGlobals->imatrix[2]);
    xx = (xx + 512) >> 10;
    ax = (xx < 0)? -xx: xx;

    yy = SHORTLONGMUL(dx, pCspGlobals->imatrix[1]) + 
         SHORTLONGMUL(dy, pCspGlobals->imatrix[3]);
    yy = (yy + 512) >> 10;
    ay = (yy < 0)? -yy: yy;

    maxabs = (ax >= ay)? ax: ay;
    while (maxabs >= 32767)
        {
        maxabs >>= 1;
        xx >>= 1;
        yy >>= 1;
        }
    x = (fix15)xx;
    y = (fix15)yy;
    len = isqrt((fix31)x * x + (fix31)y * y);
    if (len == 0)
        {
        xAdj = yAdj = 0;
        }
    else
        {
        xAdj = (fix15)(((fix31)x * pCspGlobals->halfStrokeOrus) / len);
        yAdj = (fix15)(((fix31)y * pCspGlobals->halfStrokeOrus) / len);
        }

    if (pNormVectOrus != BTSNULL)
        {
        pNormVectOrus->x = yAdj;
        pNormVectOrus->y = -xAdj;
        }

    round = (1L << sp_globals.poshift) >> 1;
    if (pNormVect != BTSNULL)
        {
        pNormVect->x = (fix15)(
            (SHORTLONGMUL(yAdj, pCspGlobals->matrix[0]) + 
             SHORTLONGMUL(-xAdj, pCspGlobals->matrix[2]) +
             round) >>
            sp_globals.poshift);
   
        pNormVect->y = (fix15)(
            (SHORTLONGMUL(yAdj, pCspGlobals->matrix[1]) + 
             SHORTLONGMUL(-xAdj, pCspGlobals->matrix[3]) +
             round) >>
            sp_globals.poshift);
        }

    if (pAdjVect != BTSNULL)
        {
        pAdjVect->x = (fix15)(
            (SHORTLONGMUL(xAdj, pCspGlobals->matrix[0]) + 
             SHORTLONGMUL(yAdj, pCspGlobals->matrix[2]) +
             round) >>
            sp_globals.poshift) >> 1;
   
        pAdjVect->y = (fix15)(
            (SHORTLONGMUL(xAdj, pCspGlobals->matrix[1]) + 
             SHORTLONGMUL(yAdj, pCspGlobals->matrix[3]) +
             round) >>
            sp_globals.poshift) >> 1;
        }
    }
}

FUNCTION
static void OutputSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg)
/*
 *  Outputs the specified segment.
 */
{
if (pCspGlobals->firstPoint)    /* First point in the contour? */
    {
#if CSP_DEBUG  >= 2
    printf("BeginContour(%3.1f, %3.1f)\n",
        (real)pSeg->ptIn.x / sp_globals.onepix,
        (real)pSeg->ptIn.y / sp_globals.onepix);
#endif
    pCspGlobals->embOutlFns.BeginContour(
        pCspGlobals,
        pSeg->ptIn,
        pCspGlobals->outside);
    pCspGlobals->firstPoint = FALSE;
    }

switch(pSeg->type)
    {
case CSP_LINE_SEG:
#if CSP_DEBUG  >= 2
    printf("LineTo(%3.1f, %3.1f)\n",
        (real)pSeg->ptOut.x / sp_globals.onepix,
        (real)pSeg->ptOut.y / sp_globals.onepix);
#endif
    pCspGlobals->embOutlFns.LineTo(
        pCspGlobals,
        pSeg->ptOut);
    break;

case CSP_CRVE_SEG:
#if CSP_DEBUG  >= 2
    printf("CurveTo(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
        (real)pSeg->ptC1.x / sp_globals.onepix,
        (real)pSeg->ptC1.y / sp_globals.onepix,
        (real)pSeg->ptC2.x / sp_globals.onepix,
        (real)pSeg->ptC2.y / sp_globals.onepix,
        (real)pSeg->ptOut.x / sp_globals.onepix,
        (real)pSeg->ptOut.y / sp_globals.onepix);
#endif
    pCspGlobals->embOutlFns.CurveTo(
        pCspGlobals,
        pSeg->ptC1,
        pSeg->ptC2,
        pSeg->ptOut,
        -1);
    break;
    }
}

FUNCTION
static int Intersect(
    segment_t *pSeg1,
    segment_t *pSeg2,
    point_t *pPint)
/*
 *  Finds the intersection between the specified segments.
 *  Returns:
 *      0: Normal return
 *      1: Vectors are parallel
 */
{
fix15   dx1, dy1;
fix15   x2, y2;
fix15   dx2, dy2;
fix15   x, y;
fix31   denom;
real    mult;

dx1 = pSeg1->dirVectOut.x;
dy1 = pSeg1->dirVectOut.y;
x2 = pSeg2->ptIn.x - pSeg1->ptOut.x;
y2 = pSeg2->ptIn.y - pSeg1->ptOut.y;
dx2 = pSeg2->dirVectIn.x;
dy2 = pSeg2->dirVectIn.y;

if ((x2 >= -2 && x2 <= 2) && (y2 >= -2 && y2 <= 2))
	goto L1;

if (dx1 == 0)                   /* First vector is vertical? */
    {
    x = 0;
    if (dx2 == 0)				/* Second vector is also vertical? */
    	{
        goto L1;
        }
    y = y2 - (fix15)(((fix31)x2 * dy2) / dx2);
    }
else if (dy1 == 0)              /* First vector is horizontal? */
    {
    if (dy2 == 0)				/* Second vector is also horizontal? */
    	{
        goto L1;
        }
    x = x2 - (fix15)(((fix31)y2 * dx2) / dy2);
    y = 0;
    }
else if (dx2 == 0)              /* Second vector is vertical? */
    {
    x = x2;
    y = (fix15)(((fix31)x2 * dy1) / dx1);
    }
else if (dy2 == 0)
    {
    x = (fix15)(((fix31)y2 * dx1) / dy1);
    y = y2;
    }
else                            /* General case? */
    {
    denom = (fix31)dx1 * dy2 - (fix31)dx2 * dy1;
    if (denom == 0)
        {
        goto L1;
        }
    mult = (real)((fix31)x2 * dy2 - (fix31)y2 * dx2) / denom;
    x = (fix15)(mult * dx1);
    y = (fix15)(mult * dy1);
    }

/* Save intersection point */
pPint->x = pSeg1->ptOut.x + x;
pPint->y = pSeg1->ptOut.y + y;
    
return 0;

L1:
/* Vectors are parallel -- return mid-point */
pPint->x = (pSeg1->ptOut.x + pSeg2->ptIn.x) >> 1;
pPint->y = (pSeg1->ptOut.y + pSeg2->ptIn.y) >> 1;

return 1;
}

FUNCTION
static void BuildLineSegment(
    segment_t *pSeg,
    point_t P1,
    point_t P2)
/*
 *  Constructs a line segment from P1 to P2
 */
{
pSeg->type = CSP_LINE_SEG;
pSeg->ptIn = P1;
pSeg->ptOut = P2;
}

FUNCTION
static void BuildCurveSegment(
    segment_t *pSeg,
    point_t P1,
    point_t P2,
    point_t P3)
/*
 *  Constructs a curve segment that approximates a circular arc from P1 to
 *  P3 with tangents P1-P2 and P2-P3.
 */
{
pSeg->type = CSP_CRVE_SEG;
pSeg->ptIn = P1;
pSeg->ptC1.x = (fix15)(
    ((fix31)P1.x * 29491L + 
     (fix31)P2.x * 36045L + 
     rnd16) >> 16);
pSeg->ptC1.y = (fix15)(
    ((fix31)P1.y * 29491L + 
     (fix31)P2.y * 36045L + 
     rnd16) >> 16);
pSeg->ptC2.x = (fix15)(
    ((fix31)P2.x * 36045L + 
     (fix31)P3.x * 29491L + 
     rnd16) >> 16);
pSeg->ptC2.y = (fix15)(
    ((fix31)P2.y * 36045L + 
     (fix31)P3.y * 29491L + 
     rnd16) >> 16);
pSeg->ptOut = P3;
}


FUNCTION 
static btsbool InvertMatrix(
    cspGlobals_t *pCspGlobals,
    fix15 *pVecLenTransX,
    fix15 *pVecLenTransY)
/*
 *  If the transformation matrix is regular (i.e. the length of a transformed 
 *  vector is independent of angle), sets the global variable regular_trans to 
 *  TRUE, sets both *pVecLenTransX and *pVecLenTransY to the scale factor to
 *  convert vector length from outline resolution units to sub-pixels and 
 *  returns TRUE.
 *  Otherwise, sets regular_trans to FALSE, sets the global imatrix to the
 *  inverse of matrix, sets *pVecLenTransX and *pVecLenTransY to the scale 
 *  factor to convert (respectively) horizontal and vertical vector lengths 
 *  from outline resolution units to sub-pixels and returns TRUE.
 *  Sets regular_trans to TRUE and returns FALSE if the the matrix is not 
 *  regular and cannot be inverted.
 */
{
fix31   L1, L2;
fix31   d;
ufix32  trans;
fix15   shift;

L1 = CspLongMult(pCspGlobals->matrix[0], pCspGlobals->matrix[0]) + 
     CspLongMult(pCspGlobals->matrix[1], pCspGlobals->matrix[1]);

trans = isqrt(L1);
shift = sp_globals.multshift - 8;
*pVecLenTransX = (fix15)((shift >= 0)? (trans << shift): (trans >> -shift));

L2 = CspLongMult(pCspGlobals->matrix[2], pCspGlobals->matrix[2]) +
     CspLongMult(pCspGlobals->matrix[3], pCspGlobals->matrix[3]);

d = CspLongMult(pCspGlobals->matrix[0], pCspGlobals->matrix[2]) + 
    CspLongMult(pCspGlobals->matrix[1], pCspGlobals->matrix[3]);

if ((d < 4) && (d > -4))
    {                  
    d = (L1 >= L2)? L1 - L2: L2 - L1;
    if (d < 4)
        {
        pCspGlobals->regular_trans = TRUE;
        *pVecLenTransY = *pVecLenTransX;
        return TRUE;
        }
    }

trans = isqrt(L2);
*pVecLenTransY = (fix15)((shift >= 0)? (trans << shift): (trans >> -shift));

d = CspLongMult(pCspGlobals->matrix[0], pCspGlobals->matrix[3]) - 
    CspLongMult(pCspGlobals->matrix[1], pCspGlobals->matrix[2]);

if ((d < 4) && (d > -4))
    {
    return FALSE;
    }

pCspGlobals->regular_trans = FALSE;

/* Invert the matrix */
pCspGlobals->imatrix[0] = 
    (LongDiv(pCspGlobals->matrix[3], d) + sp_globals.pixrnd) >> sp_globals.pixshift;
pCspGlobals->imatrix[1] = 
    -(LongDiv(pCspGlobals->matrix[1], d) + sp_globals.pixrnd) >> sp_globals.pixshift;
pCspGlobals->imatrix[2] = 
    -(LongDiv(pCspGlobals->matrix[2], d) + sp_globals.pixrnd) >> sp_globals.pixshift;
pCspGlobals->imatrix[3] = 
    (LongDiv(pCspGlobals->matrix[0], d) + sp_globals.pixrnd) >> sp_globals.pixshift;

return TRUE;
}

FUNCTION 
static long LongDiv(
    long num,
    long denom)
{
register ufix32 src2hi,src2lo;
register fix31 src1 = denom;
int negative;
    
    src2hi = num >> 16;
    src2lo = num << 16;
    negative = (fix31)(src2hi ^ denom) < 0;
        if ((fix31)src2hi < 0)
                if (src2lo = -(fix31)src2lo)
                        src2hi = ~src2hi;
                else
                        src2hi = -(fix31)src2hi;
        if (src1 < 0)
                src1 = -src1;
        {       register unsigned long src1hi, src1lo;
                unsigned long result = 0, place = 0x40000000;

                if ((src1hi = src1) & 1)
                        src1lo = 0x80000000;
                else
                        src1lo = 0;

                src1hi >>= 1;
                src2hi += (src2lo += src1hi) < src1hi;          /* round the result */

                if (src2hi > src1hi || src2hi == src1hi && src2lo >= src1lo)
                        if (negative)
                                return 0x80000000;
                        else
                                return 0x7fffffff;
                while (place && src2hi)
                {       src1lo >>= 1;
                        if (src1hi & 1)
                                src1lo += 0x80000000;
                        src1hi >>= 1;
                        if (src1hi < src2hi)
                        {       src2hi -= src1hi;
                                src2hi -= src1lo > src2lo;
                                src2lo -= src1lo;
                                result += place;
                        }
                        else if (src1hi == src2hi && src1lo <= src2lo)
                        {       src2hi = 0;
                                src2lo -= src1lo;
                                result += place;
                        }
                        place >>= 1;
                }
                if (src2lo >= (ufix32)src1)
                        result += src2lo/(ufix32)src1;
                if (negative)
                        return -(fix31)result;
                else
                        return result;
        }
}

FUNCTION
static ufix32 isqrt(
    ufix32 x)
/*
 *  Returns the largest integer whose square is less than or equal to
 *  the given integer.
 */
{
ufix32  r, nr, m;

r = 0;
m = 0x40000000;
do
    {
    nr = r + m;
    if (nr <= x)
        {
        x -= nr;
        r = nr + m;
        }
    r >>= 1;
    m >>= 2;
    } while (m != 0);

return r;
}

#if CSP_DEBUG
FUNCTION
static void ShowSegment(
    cspGlobals_t *pCspGlobals,
    segment_t *pSeg)
{
if (pSeg == BTSNULL)
    {
    printf("\nNull Segment\n");
    return;
    }

switch (pSeg->type)
    {
case CSP_LINE_SEG:
    printf("\nLine segment:\n");
    printf("    Starts at (%3.1f, %3.1f)\n", 
        (real)pSeg->ptIn.x / sp_globals.onepix, 
        (real)pSeg->ptIn.y / sp_globals.onepix);
    printf("    Ends at (%3.1f, %3.1f)\n", 
        (real)pSeg->ptOut.x / sp_globals.onepix, 
        (real)pSeg->ptOut.y / sp_globals.onepix);
    printf("    Direction vector is (%3.1f, %3.1f)\n", 
        (real)pSeg->dirVectIn.x / sp_globals.onepix, 
        (real)pSeg->dirVectIn.y / sp_globals.onepix);
    printf("    Normal vector is (%3.1f, %3.1f)\n", 
        (real)pSeg->normVectIn.x / sp_globals.onepix, 
        (real)pSeg->normVectIn.y / sp_globals.onepix);
    break;

case CSP_CRVE_SEG:
    printf("\nCurve segment:\n");
    printf("    Starts at (%3.1f, %3.1f)\n", 
        (real)pSeg->ptIn.x / sp_globals.onepix, 
        (real)pSeg->ptIn.y / sp_globals.onepix);
    printf("    First control point is (%3.1f, %3.1f)\n", 
        (real)pSeg->ptC1.x / sp_globals.onepix, 
        (real)pSeg->ptC1.y / sp_globals.onepix);
    printf("    Second control point is (%3.1f, %3.1f)\n", 
        (real)pSeg->ptC2.x / sp_globals.onepix, 
        (real)pSeg->ptC2.y / sp_globals.onepix);
    printf("    Ends at (%3.1f, %3.1f)\n", 
        (real)pSeg->ptOut.x / sp_globals.onepix, 
        (real)pSeg->ptOut.y / sp_globals.onepix);
    printf("    Direction vector at start is (%3.1f, %3.1f)\n", 
        (real)pSeg->dirVectIn.x / sp_globals.onepix, 
        (real)pSeg->dirVectIn.y / sp_globals.onepix);
    printf("    Normal vector at start is (%3.1f, %3.1f)\n", 
        (real)pSeg->normVectIn.x / sp_globals.onepix, 
        (real)pSeg->normVectIn.y / sp_globals.onepix);
    printf("    Direction vector at end is (%3.1f, %3.1f)\n", 
        (real)pSeg->dirVectOut.x / sp_globals.onepix, 
        (real)pSeg->dirVectOut.y / sp_globals.onepix);
    printf("    Normal vector at end is (%3.1f, %3.1f)\n", 
        (real)pSeg->normVectOut.x / sp_globals.onepix, 
        (real)pSeg->normVectOut.y / sp_globals.onepix);
    break;

default:
    printf("\nIllegal segment (type = %hd)\n", pSeg->type);
    break;
    }
}
#endif
   
#endif
#endif /* #if PROC_TRUEDOC || ! INCL_TPS */

