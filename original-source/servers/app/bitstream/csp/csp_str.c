/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ S T R . C *******************************
 *                                                                           *
 * Character outline stroking logic.                                         *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_STR.C 7.2 1996/10/29 22:41:50 john preliminary $
 *                                                                                    
 *     $Log: CSP_STR.C $
 *     Revision 7.2  1996/10/29 22:41:50  john
 *     Changed outline interface functions to static.
 *     Added a public struct containing the entry points.
 * Revision 7.1  96/10/10  14:06:18  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:58:54  mark
 * Release
 * 
 * Revision 5.3  96/07/02  14:14:41  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.2  96/03/21  11:40:31  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:21:54  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:46:49  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:30:32  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:47:21  mark
 * Release
 * 
 * Revision 1.4  95/11/16  16:05:18  roberte
 *  Block out file for TPS build with PROC_TRUEDOC == 0
 * 
 * Revision 1.3  95/11/07  13:46:30  john
 * Extended hint processing function to handle secondary
 * edge hints.
 * 
 * Revision 1.2  95/08/31  09:26:32  john
 * CspCurveStroke() function added.
 * 
 * Revision 1.1  95/08/10  16:45:48  john
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
#if INCL_STROKE 

#if CSP_DEBUG
#include "csp_dbg.h"
#endif


extern intOutlineFns_t EmboldenProcessor;

/* Local function prototypes */

LOCAL_PROTO
btsbool CspInitStroke(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg);

LOCAL_PROTO
btsbool CspBeginCharStroke(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO
void CspBeginSubCharStroke(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO
void CspCharHintStroke(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint);

LOCAL_PROTO
void CspBeginContourStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside);

LOCAL_PROTO
void CspCurveStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    point_t P2, 
    point_t P3, 
    fix15 depth);

LOCAL_PROTO
void CspLineStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1);

LOCAL_PROTO
void CspEndContourStroke(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
btsbool CspEndSubCharStroke(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
btsbool CspEndCharStroke(
    cspGlobals_t *pCspGlobals);

/* Public structure of function pointers for stroked output module */
intOutlineFns_t StrokeProcessor =
    {
    CspInitStroke,
    CspBeginCharStroke,
    CspBeginSubCharStroke,
    CspCharHintStroke,
    CspBeginContourStroke,
    CspCurveStroke,
    CspLineStroke,
    CspEndContourStroke,
    CspEndSubCharStroke,
    CspEndCharStroke
    };

FUNCTION 
static btsbool CspInitStroke(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg)
{

#if CSP_DEBUG >= 2
printf("CspInitStroke\n");
#endif

return EmboldenProcessor.InitOut(pCspGlobals, specsarg);
}


FUNCTION 
static btsbool CspBeginCharStroke(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{

pCspGlobals->firstPass = TRUE;

return EmboldenProcessor.BeginChar(pCspGlobals, Psw, Pmin, Pmax);
}

FUNCTION 
static void CspBeginSubCharStroke(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{

pCspGlobals->firstPass = TRUE;

EmboldenProcessor.BeginSubChar(pCspGlobals, Psw, Pmin, Pmax);
}

FUNCTION
static void CspCharHintStroke(
    cspGlobals_t *pCspGlobals,
    charHint_t *pCharHint)
{
charHint_t charHint;

if ((pCspGlobals->embOutlFns.CharHint != BTSNULL) &&
    (pCspGlobals->firstPass))
    {
    charHint.type = pCharHint->type;
    switch(charHint.type)
        {
    case CSP_VSTEM:
    case CSP_VSTEM2:
        charHint.hint.vStem.x1 = 
            pCharHint->hint.vStem.x1;
        charHint.hint.vStem.x2 = 
            pCharHint->hint.vStem.x1 + (pCspGlobals->xGridAdj << 1);
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);

        if (pCharHint->hint.vStem.x2 != pCharHint->hint.vStem.x1)
            {
            charHint.hint.vStem.x1 = 
                pCharHint->hint.vStem.x2;
            charHint.hint.vStem.x2 = 
                pCharHint->hint.vStem.x2 + (pCspGlobals->xGridAdj << 1);
            pCspGlobals->embOutlFns.CharHint(
                pCspGlobals,
                &charHint);
            }

        break;

    case CSP_HSTEM:
    case CSP_HSTEM2:
        charHint.hint.hStem.y1 = 
            pCharHint->hint.hStem.y1;
        charHint.hint.hStem.y2 = 
            pCharHint->hint.hStem.y1 + (pCspGlobals->yGridAdj << 1);
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);

        if (pCharHint->hint.hStem.y2 != pCharHint->hint.hStem.y1)
            {
            charHint.hint.hStem.y1 = 
                pCharHint->hint.hStem.y2;
            charHint.hint.hStem.y2 = 
                pCharHint->hint.hStem.y2 + (pCspGlobals->yGridAdj << 1);
            pCspGlobals->embOutlFns.CharHint(
                pCspGlobals,
                &charHint);
            }

        break;

    case CSP_V_LEAD_EDGE2:
    case CSP_V_TRAIL_EDGE2:
        charHint.hint.vEdge.x = 
            pCharHint->hint.vEdge.x;
        charHint.hint.vEdge.dx = 
            pCharHint->hint.vEdge.dx;
        charHint.hint.vEdge.thresh = 
            pCharHint->hint.vEdge.thresh;
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        charHint.hint.vEdge.x += 
            (pCspGlobals->xGridAdj << 1);
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        break;

    case CSP_H_LEAD_EDGE2:
    case CSP_H_TRAIL_EDGE2:
        charHint.hint.hEdge.y = 
            pCharHint->hint.hEdge.y;
        charHint.hint.hEdge.dy = 
            pCharHint->hint.hEdge.dy;
        charHint.hint.hEdge.thresh = 
            pCharHint->hint.hEdge.thresh;
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        charHint.hint.hEdge.y += 
            (pCspGlobals->yGridAdj << 1);
        pCspGlobals->embOutlFns.CharHint(
            pCspGlobals,
            &charHint);
        break;
        }
    }
}

FUNCTION 
static void CspBeginContourStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside)
{

#if CSP_DEBUG >= 2
printf("\nCspBeginContourStroke(%3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix);
#endif

EmboldenProcessor.BeginContour(pCspGlobals, P1, outside);
}

FUNCTION 
static void CspCurveStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15   depth)
{
#if CSP_DEBUG  >= 2
printf("\nCspLineStroke(%3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix);
#endif

EmboldenProcessor.CurveTo(pCspGlobals, P1, P2, P3, depth);
}

FUNCTION 
static void CspLineStroke(
    cspGlobals_t *pCspGlobals,
    point_t P1)
{

#if CSP_DEBUG  >= 2
printf("\nCspLineStroke(%3.1f, %3.1f)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix);
#endif

EmboldenProcessor.LineTo(pCspGlobals, P1);
}


FUNCTION 
static void CspEndContourStroke(
    cspGlobals_t *pCspGlobals)
{

#if CSP_DEBUG  >= 2
printf("CspEndContourStroke\n");
#endif

EmboldenProcessor.EndContour(pCspGlobals);
}

FUNCTION 
static btsbool CspEndSubCharStroke(
    cspGlobals_t *pCspGlobals)
{
btsbool retValue;

pCspGlobals->halfStrokePix = -pCspGlobals->halfStrokePix;
pCspGlobals->halfStrokeOrus = -pCspGlobals->halfStrokeOrus;

if (pCspGlobals->firstPass)
    {
    pCspGlobals->firstPass = FALSE;
    return FALSE;
    }

retValue = EmboldenProcessor.EndSubChar(pCspGlobals);

if (!retValue)
    pCspGlobals->firstPass = TRUE;

return retValue;
}

FUNCTION 
static btsbool CspEndCharStroke(
    cspGlobals_t *pCspGlobals)
{
btsbool retValue;

#if CSP_DEBUG  >= 2
printf("EndCharStroke()\n");
#endif

pCspGlobals->halfStrokePix = -pCspGlobals->halfStrokePix;
pCspGlobals->halfStrokeOrus = -pCspGlobals->halfStrokeOrus;

if (pCspGlobals->firstPass)
    {
    pCspGlobals->firstPass = FALSE;
    return FALSE;
    }

retValue =  EmboldenProcessor.EndChar(pCspGlobals);

pCspGlobals->firstPass = TRUE;

return retValue;
}
#endif

#endif /* #if PROC_TRUEDOC || ! INCL_TPS */
