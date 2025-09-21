/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ O T L . C *******************************
 *                                                                           *
 * Character shape player functions for outputting character outlines.       *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_OTL.C 7.6 1997/03/18 16:04:29 SHAWN release $
 *                                                                                    
 *     $Log: CSP_OTL.C $
 *     Revision 7.6  1997/03/18 16:04:29  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.5  1996/11/05 18:05:10  john
 *     Removed updating of global setwidth value.
 * Revision 7.4  96/11/05  17:54:13  john
 * RRemoved update to global setwidth value.\
 * 
 * Revision 7.3  96/10/30  10:44:37  john
 * Removed bogus declaration of CspSetContourDirection().
 * 
 * Revision 7.2  96/10/29  22:35:06  john
 * Moved CspSetContourDirection() out of this module to allow
 *     outline output to be optionally omitted.
 * Changed all outline interface functions to static.
 * Added a public struct containing entry points.
 * 
 * Revision 7.1  96/10/10  14:05:43  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:58:17  mark
 * Release
 * 
 * Revision 5.5  96/07/02  14:13:47  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.4  96/06/20  17:26:05  john
 * Updates to support CspSetOutputSpecsAlt1().
 * 
 * Revision 5.3  96/03/29  16:19:13  john
 * Added GlyphSpecs callbacks for delivery of compound
 *     character structure information.
 * 
 * Revision 5.2  96/03/21  11:41:51  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:21:19  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:10  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:52  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:46  mark
 * Release
 * 
 * Revision 1.4  95/12/12  14:22:57  john
 * Corrected delivery of thresh to Vedge() and Hedge() callbacks.
 * 
 * Revision 1.3  95/11/07  13:47:03  john
 * Extended hint processing function to handle secondary
 *  edge hints.
 * 
 * Revision 1.2  95/08/31  09:23:26  john
 * Curve splitting if required is now called from CspCurveToOutl.
 * 
 * Revision 1.1  95/08/10  16:45:27  john
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

#if INCL_CSPOUTLINE

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

#define MAX_INTERCEPTS_OTL MAX_INTERCEPTS

/* Local function prototypes */

LOCAL_PROTO
btsbool CspInitOutOutl(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *pSpecs);

LOCAL_PROTO
btsbool CspBeginCharOutl(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax);

LOCAL_PROTO
void CspBeginSubCharOutl(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax);

LOCAL_PROTO
void CspCharHintOutl(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint);

LOCAL_PROTO
void CspBeginContourOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    btsbool outside);

LOCAL_PROTO
void CspCurveToOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15 depth);

LOCAL_PROTO
void CspLineToOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1);

LOCAL_PROTO
void CspEndContourOutl(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndSubCharOutl(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndCharOutl(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
void WriteBuffer(
    cspGlobals_t *pCspGlobals);

/* Public structure of function pointers for outline output module */
intOutlineFns_t OutlineOutput =
    {
    CspInitOutOutl,
    CspBeginCharOutl,
    CspBeginSubCharOutl,
    CspCharHintOutl,
    CspBeginContourOutl,
    CspCurveToOutl,
    CspLineToOutl,
    CspEndContourOutl,
    CspEndSubCharOutl,
    CspEndCharOutl
    };

FUNCTION
btsbool CspInitOutOutl(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *pSpecs)
{
return TRUE;
}

FUNCTION
btsbool CspBeginCharOutl(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax)
{
pCspGlobals->requiredContour = 0;
pCspGlobals->contour = 0;
pCspGlobals->state = -1;

/* Deliver glyph specs for simple glyph or compound glyph head */
if (pCspGlobals->outlineOptions.flags & ENABLE_GLYPH_SPECS)
    {
    pCspGlobals->glyphSpecs.pfrCode = pCspGlobals->physFontPfrCode;
    pCspGlobals->glyphSpecs.xScale = 1L << 16;
    pCspGlobals->glyphSpecs.yScale = 1L << 16;
    pCspGlobals->glyphSpecs.xPosition = 0;
    pCspGlobals->glyphSpecs.yPosition = 0;
    pCspGlobals->outlineOptions.GlyphSpecs(
        &(pCspGlobals->glyphSpecs)
        USERARG2);
    }

return TRUE;
}

FUNCTION
void CspBeginSubCharOutl(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax)
{

/* Deliver glyph specs for compound glyph element */
if ((pCspGlobals->contour == pCspGlobals->requiredContour) &&
    (pCspGlobals->state < 0) &&
    (pCspGlobals->outlineOptions.flags & ENABLE_GLYPH_SPECS))
    {
    pCspGlobals->glyphSpecs.pfrCode = pCspGlobals->physFontPfrCode;
    pCspGlobals->outlineOptions.GlyphSpecs(
        &(pCspGlobals->glyphSpecs)
        USERARG2);
    }

pCspGlobals->requiredContour = 0;
pCspGlobals->contour = 0;
pCspGlobals->state = -1;
}

FUNCTION
void CspCharHintOutl(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint)
{

if ((pCspGlobals->contour == pCspGlobals->requiredContour) &&
    (pCspGlobals->state < 0))
    {
    switch(pCharHint->type)
        {
    case CSP_VSTEM2:
        if ((pCspGlobals->outlineOptions.flags & ENABLE_SEC_STEMS) == 0)
            break;
    case CSP_VSTEM:
        pCspGlobals->outlCallbackFns.Vstem(
            (short)((pCharHint->hint.vStem.x1 + 
                     sp_globals.pixrnd) >> 
                    sp_globals.pixshift),
            (short)((pCharHint->hint.vStem.x2 +
                     sp_globals.pixrnd) >> 
                    sp_globals.pixshift)
            USERARG2);
        break;

    case CSP_HSTEM2:
        if ((pCspGlobals->outlineOptions.flags & ENABLE_SEC_STEMS) == 0)
            break;
    case CSP_HSTEM:
        pCspGlobals->outlCallbackFns.Hstem(
            (short)((pCharHint->hint.hStem.y1 +
                     sp_globals.pixrnd) >> 
                    sp_globals.pixshift),
            (short)((pCharHint->hint.hStem.y2 +
                     sp_globals.pixrnd) >> 
                    sp_globals.pixshift)
            USERARG2);
        break;

    case CSP_V_LEAD_EDGE2:
    case CSP_V_TRAIL_EDGE2:
        if (pCspGlobals->outlineOptions.flags & ENABLE_SEC_EDGES)
            {
            pCspGlobals->outlineOptions.Vedge(
                (short)((pCharHint->hint.vEdge.x +
                         sp_globals.pixrnd) >> 
                        sp_globals.pixshift),
                (short)((pCharHint->hint.vEdge.dx +
                         sp_globals.pixrnd) >> 
                        sp_globals.pixshift),
                (short)pCharHint->hint.vEdge.thresh
                USERARG2);
            }
        break;

    case CSP_H_LEAD_EDGE2:
    case CSP_H_TRAIL_EDGE2:
        if (pCspGlobals->outlineOptions.flags & ENABLE_SEC_EDGES)
            {
            pCspGlobals->outlineOptions.Hedge(
                (short)((pCharHint->hint.hEdge.y +
                         sp_globals.pixrnd) >> 
                        sp_globals.pixshift),
                (short)((pCharHint->hint.hEdge.dy +
                         sp_globals.pixrnd) >> 
                        sp_globals.pixshift),
                (short)pCharHint->hint.hEdge.thresh
                USERARG2);
            }
        break;
        }
    }
}

FUNCTION
void CspBeginContourOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    btsbool outside)
{
fix15 x, y;

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;

if (pCspGlobals->contour == pCspGlobals->requiredContour)
    {
    /* Determine if contour direction is reversed */
    CspSetContourDirection(pCspGlobals, outside);

    x = (fix15)((P1.x + sp_globals.pixrnd) >> sp_globals.pixshift);
    y = (fix15)((P1.y + sp_globals.pixrnd) >> sp_globals.pixshift);

    if (pCspGlobals->reverseContour)
        {
        pCspGlobals->nFullBlocks = 0;
        sp_intercepts.car[0] = x;
        sp_intercepts.cdr[0] = y;
        sp_intercepts.inttype[0] = 0;
        pCspGlobals->nPoints = 1;
        }
    else
        {
        pCspGlobals->outlCallbackFns.MoveTo(
            (short)x, 
            (short)y 
            USERARG2);
        }
    }
}

FUNCTION
void CspCurveToOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15 depth)
{
point_t P0;
fix15 x1, y1;
fix15 x2, y2;
fix15 x3, y3;

if (pCspGlobals->outlCallbackFns.CurveTo == BTSNULL)
    {
    P0.x = sp_globals.x0_spxl;
    P0.y = sp_globals.y0_spxl;
    CspSplitCurve(
        pCspGlobals,
        P0,
        P1,
        P2,
        P3,
        depth,
        CspLineToOutl);
    return;
    }

if (pCspGlobals->contour == pCspGlobals->requiredContour)
    {
    x1 = (fix15)((P1.x + sp_globals.pixrnd) >> sp_globals.pixshift);
    y1 = (fix15)((P1.y + sp_globals.pixrnd) >> sp_globals.pixshift);
    x2 = (fix15)((P2.x + sp_globals.pixrnd) >> sp_globals.pixshift);
    y2 = (fix15)((P2.y + sp_globals.pixrnd) >> sp_globals.pixshift);
    x3 = (fix15)((P3.x + sp_globals.pixrnd) >> sp_globals.pixshift);
    y3 = (fix15)((P3.y + sp_globals.pixrnd) >> sp_globals.pixshift);

    if (pCspGlobals->reverseContour)
        {
        if ((pCspGlobals->state < 0) ||
            (pCspGlobals->nFullBlocks <= pCspGlobals->state))
            {
            sp_intercepts.car[pCspGlobals->nPoints] = x1;
            sp_intercepts.cdr[pCspGlobals->nPoints] = y1;
            sp_intercepts.inttype[pCspGlobals->nPoints] = 1;
            pCspGlobals->nPoints++;

            sp_intercepts.car[pCspGlobals->nPoints] = x2;
            sp_intercepts.cdr[pCspGlobals->nPoints] = y2;
            sp_intercepts.inttype[pCspGlobals->nPoints] = 1;
            pCspGlobals->nPoints++;

            if (pCspGlobals->nPoints >= MAX_INTERCEPTS_OTL - 3)
                {
                pCspGlobals->nFullBlocks++;
                if ((pCspGlobals->state < 0) ||
                    (pCspGlobals->nFullBlocks <= pCspGlobals->state))
                    {
                    sp_intercepts.car[0] = x3;
                    sp_intercepts.cdr[0] = y3;
                    sp_intercepts.inttype[0] = 0;
                    pCspGlobals->nPoints = 1;
                    }
                }
            else
                {
                sp_intercepts.car[pCspGlobals->nPoints] = x3;
                sp_intercepts.cdr[pCspGlobals->nPoints] = y3;
                sp_intercepts.inttype[pCspGlobals->nPoints] = 0;
                pCspGlobals->nPoints++;
                }
            }
        }
    else
        {
        pCspGlobals->outlCallbackFns.CurveTo(
            (short)x1, 
            (short)y1, 
            (short)x2, 
            (short)y2, 
            (short)x3, 
            (short)y3 
            USERARG2);
        }
    }

sp_globals.x0_spxl = P3.x;
sp_globals.y0_spxl = P3.y;
}

FUNCTION
void CspLineToOutl(
    cspGlobals_t *pCspGlobals,
    point_t P1)
{
short x, y;

if (pCspGlobals->contour == pCspGlobals->requiredContour)
    {
    x = (fix15)((P1.x + sp_globals.pixrnd) >> sp_globals.pixshift);
    y = (fix15)((P1.y + sp_globals.pixrnd) >> sp_globals.pixshift);

    if (pCspGlobals->reverseContour)
        {
        if ((pCspGlobals->state < 0) ||
            (pCspGlobals->nFullBlocks <= pCspGlobals->state))
            {
            if (pCspGlobals->nPoints >= MAX_INTERCEPTS_OTL - 3)
                {
                pCspGlobals->nFullBlocks++;
                if ((pCspGlobals->state < 0) ||
                    (pCspGlobals->nFullBlocks <= pCspGlobals->state))
                    {
                    sp_intercepts.car[0] = x;
                    sp_intercepts.cdr[0] = y;
                    sp_intercepts.inttype[0] = 0;
                    pCspGlobals->nPoints = 1;
                    }
                }
            else
                {
                sp_intercepts.car[pCspGlobals->nPoints] = x;
                sp_intercepts.cdr[pCspGlobals->nPoints] = y;
                sp_intercepts.inttype[pCspGlobals->nPoints] = 0;
                pCspGlobals->nPoints++;
                }
            }
        }
    else
        {
        pCspGlobals->outlCallbackFns.LineTo(
            (short)x, 
            (short)y 
            USERARG2);
        }
    }

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
}

FUNCTION
void CspEndContourOutl(
    cspGlobals_t *pCspGlobals)
{
fix15   x, y;

if (pCspGlobals->contour == pCspGlobals->requiredContour)
    {
    if (pCspGlobals->reverseContour)
        {
        if (pCspGlobals->state < 0)
            {
            pCspGlobals->nPoints--;
            x = sp_intercepts.car[pCspGlobals->nPoints];
            y = sp_intercepts.cdr[pCspGlobals->nPoints];
            pCspGlobals->outlCallbackFns.MoveTo(
                (short)x, 
                (short)y 
                USERARG2);

            WriteBuffer(pCspGlobals);
            if (pCspGlobals->nFullBlocks != 0)
                {
                pCspGlobals->state = pCspGlobals->nFullBlocks - 1;
                goto L1;
                }
            }
        else 
            {
            WriteBuffer(pCspGlobals);
            if (pCspGlobals->state != 0)
                {
                pCspGlobals->state--;
                goto L1;
                }
            }
        }

    pCspGlobals->outlCallbackFns.ClosePath(
#if REENTRANT
        pCspGlobals->userArgOutput
#endif
        );
    pCspGlobals->requiredContour++;
    pCspGlobals->state = -1;
    }

L1:
pCspGlobals->contour++;
}

FUNCTION
btsbool CspEndSubCharOutl(
    cspGlobals_t *pCspGlobals)
{

if (pCspGlobals->contour != pCspGlobals->requiredContour)
    {
    pCspGlobals->contour = 0;
    return FALSE;
    }

return TRUE;
}

FUNCTION
btsbool CspEndCharOutl(
    cspGlobals_t *pCspGlobals)
{
if (pCspGlobals->contour != pCspGlobals->requiredContour)
    {
    pCspGlobals->contour = 0;
    return FALSE;
    }

return TRUE;
}

FUNCTION
static void WriteBuffer(
    cspGlobals_t *pCspGlobals)
{
fix15   x1, y1;
fix15   x2, y2;
fix15   x3, y3;

ufix8   pointType;

while(pCspGlobals->nPoints > 0)
    {
    pCspGlobals->nPoints--;
    pointType = sp_intercepts.inttype[pCspGlobals->nPoints];
    if (pointType == 0)
        {
        x1 = sp_intercepts.car[pCspGlobals->nPoints];
        y1 = sp_intercepts.cdr[pCspGlobals->nPoints];
        pCspGlobals->outlCallbackFns.LineTo(
            (short)x1, 
            (short)y1 
            USERARG2);
        }
    else
        {
        x1 = sp_intercepts.car[pCspGlobals->nPoints];
        y1 = sp_intercepts.cdr[pCspGlobals->nPoints];

        pCspGlobals->nPoints--;
        x2 = sp_intercepts.car[pCspGlobals->nPoints];
        y2 = sp_intercepts.cdr[pCspGlobals->nPoints];

        pCspGlobals->nPoints--;
        x3 = sp_intercepts.car[pCspGlobals->nPoints];
        y3 = sp_intercepts.cdr[pCspGlobals->nPoints];

        pCspGlobals->outlCallbackFns.CurveTo(
            (short)x1, 
            (short)y1, 
            (short)x2, 
            (short)y2, 
            (short)x3, 
            (short)y3 
            USERARG2);
        }
    }
}


#endif /*CSPOUTLINE*/

