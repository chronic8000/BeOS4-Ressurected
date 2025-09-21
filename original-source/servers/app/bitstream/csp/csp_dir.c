/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ D I R . C *******************************
 *                                                                           *
 * Character shape player functions for outputting character outlines to     *
 * user defined output module.                                               *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_DIR.C 7.3 1997/03/18 16:04:23 SHAWN release $
 *                                                                                    
 *     $Log: CSP_DIR.C $
 *     Revision 7.3  1997/03/18 16:04:23  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.2  1996/10/29 22:28:14  john
 *     Outline functions changed to static.
 *     Added public struct defining entry points.
 * Revision 7.1  96/10/10  14:04:55  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:29  mark
 * Release
 * 
 * Revision 5.3  96/07/02  14:13:16  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.2  96/03/21  11:43:18  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:30  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:23  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:02  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:57  mark
 * Release
 * 
 * Revision 1.2  95/08/31  09:13:55  john
 * Dummy CspCurveToDir() function replaced by a real one that
 *     splits the curve into vectors and calls CspLineToDir()
 *     for each vector produced.
 * 
 * Revision 1.1  95/08/10  16:44:47  john
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

#if INCL_DIR
#if PROC_TRUEDOC || ! INCL_TPS

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

#if INCL_TPS
#if REENTRANT
#define DIRPARAM1 &(pCspGlobals->spGlobals)
#define DIRPARAM2 DIRPARAM1 ,
#else
#define DIRPARAM1
#define DIRPARAM2 
#endif
#else
#define DIRPARAM1 pCspGlobals
#define DIRPARAM2 DIRPARAM1 ,
#endif


/* Local function prototypes */

LOCAL_PROTO
btsbool CspInitOutDir(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *pSpecs);

LOCAL_PROTO
btsbool CspBeginCharDir(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax);

LOCAL_PROTO
void CspBeginSubCharDir(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax);

LOCAL_PROTO
void CspBeginContourDir(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    btsbool outside);

LOCAL_PROTO
void CspCurveToDir(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15 depth);

LOCAL_PROTO
void CspLineToDir(
    cspGlobals_t *pCspGlobals,
    point_t P1);

LOCAL_PROTO
void CspEndContourDir(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndSubCharDir(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndCharDir(
    cspGlobals_t *pCspGlobals);

/* Pulic structure of function pointers for direct output module */
intOutlineFns_t DirectOutput =
    {
    CspInitOutDir,
    CspBeginCharDir,
    CspBeginSubCharDir,
    BTSNULL,
    CspBeginContourDir,
    CspCurveToDir,
    CspLineToDir,
    CspEndContourDir,
    CspEndSubCharDir,
    CspEndCharDir
    };

FUNCTION
static btsbool CspInitOutDir(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *pSpecs)
{
specs_t oldspecs;

oldspecs.flags = 0;
oldspecs.xxmult = sp_globals.tcb.ctm[0] / pCspGlobals->fontInfo.outlineResolution;
oldspecs.yxmult = sp_globals.tcb.ctm[1] / pCspGlobals->fontInfo.outlineResolution;
oldspecs.xymult = sp_globals.tcb.ctm[2] / pCspGlobals->fontInfo.outlineResolution;
oldspecs.yymult = sp_globals.tcb.ctm[3] / pCspGlobals->fontInfo.outlineResolution;
 
pCspGlobals->dirCallbackFns.InitOut(DIRPARAM2 (specs_t *)&oldspecs);
return TRUE;
}

FUNCTION
static btsbool CspBeginCharDir(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax)
{
/* Save escapement vector for later output */
pCspGlobals->dirCallbackFns.BeginChar(DIRPARAM2 Psw,Pmin,Pmax);
return TRUE;
}

FUNCTION
static void CspBeginSubCharDir(
    cspGlobals_t *pCspGlobals,
    point_t Psw,
    point_t Pmin,
    point_t Pmax)
{
pCspGlobals->dirCallbackFns.BeginSubChar(DIRPARAM2 Psw,Pmin,Pmax);
}

FUNCTION
static void CspBeginContourDir(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    btsbool outside)
{
sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
pCspGlobals->dirCallbackFns.BeginContour(DIRPARAM2 P1, outside);
}

FUNCTION
static void CspCurveToDir(
    cspGlobals_t *pCspGlobals,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15 depth)
{
point_t P0;

P0.x = sp_globals.x0_spxl;
P0.y = sp_globals.y0_spxl;
if (pCspGlobals->dirCallbackFns.CurveTo == BTSNULL)
    {
    CspSplitCurve(
        pCspGlobals,
        P0,
        P1,
        P2,
        P3,
        depth,
        CspLineToDir);
    }
else
    {
    pCspGlobals->dirCallbackFns.CurveTo(DIRPARAM2 P1, P2, P3, depth);
    sp_globals.x0_spxl = P3.x;
    sp_globals.y0_spxl = P3.y;
    }
}

FUNCTION
static void CspLineToDir(
    cspGlobals_t *pCspGlobals,
    point_t P1)
{
pCspGlobals->dirCallbackFns.LineTo(DIRPARAM2 P1);
sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
}

FUNCTION
static void CspEndContourDir(
    cspGlobals_t *pCspGlobals)
{
pCspGlobals->dirCallbackFns.EndContour(DIRPARAM1);
}

FUNCTION
btsbool CspEndSubCharDir(
    cspGlobals_t *pCspGlobals)
{
pCspGlobals->dirCallbackFns.EndSubChar(DIRPARAM1);

return TRUE;
}

FUNCTION
static btsbool CspEndCharDir(
    cspGlobals_t *pCspGlobals)
{
return pCspGlobals->dirCallbackFns.EndChar(DIRPARAM1);
}
#endif /*INCL_DIR*/
#endif /*PROC_TRUEDOC*/

