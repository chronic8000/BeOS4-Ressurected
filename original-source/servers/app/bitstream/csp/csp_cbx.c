/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 96                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ C B X . C *******************************
 *                                                                           *
 * Character shape player character bounding box accumulator                 *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_CBX.C 7.4 1997/03/18 16:04:21 SHAWN release $
 *                                                                                    
 *     $Log: CSP_CBX.C $
 *     Revision 7.4  1997/03/18 16:04:21  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.3  1996/11/05 17:46:30  john
 *     Eliminated update of global setwidth value.
 * Revision 7.2  96/10/29  22:26:45  john
 * Outline delivery functions changed to static.
 * Added public struct containing entry points.
 * 
 * Revision 7.1  96/10/10  14:04:40  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:14  mark
 * Release
 * 
 * Revision 5.2  96/03/21  11:38:16  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:13  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:08  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:47  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:41  mark
 * Release
 * 
 * Revision 1.1  95/08/10  16:44:33  john
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
btsbool CspInitOutBBox(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg);

LOCAL_PROTO
btsbool CspBeginCharBBox(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO
void CspBeginSubCharBBox(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax);

LOCAL_PROTO
void CspBeginContourBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside);

LOCAL_PROTO
void CspCurveToBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    point_t P2, 
    point_t P3, 
    fix15 depth);

LOCAL_PROTO
void CspLineToBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1);

LOCAL_PROTO
void CspEndContourBBox(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndSubCharBBox(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
btsbool CspEndCharBBox(
    cspGlobals_t *pCspGlobals);

LOCAL_PROTO
void CheckPoint(
    cspGlobals_t *pCspGlobals,
    point_t P);

/* Public structure of function pointers for character bounding box output module */
intOutlineFns_t charBBox =
    {
    CspInitOutBBox,
    CspBeginCharBBox,
    CspBeginSubCharBBox,
    BTSNULL,
    CspBeginContourBBox,
    CspCurveToBBox,
    CspLineToBBox,
    CspEndContourBBox,
    CspEndSubCharBBox,
    CspEndCharBBox
    };

FUNCTION
static btsbool CspInitOutBBox(
    cspGlobals_t *pCspGlobals,
    CspSpecs_t *specsarg)
{
return TRUE;
}

FUNCTION
static btsbool CspBeginCharBBox(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{
/* Initialize character bounding box */
sp_globals.xmin = 32000;
sp_globals.xmax = -32000;
sp_globals.ymin = 32000;
sp_globals.ymax = -32000;

return TRUE;
}

FUNCTION
static void CspBeginSubCharBBox(
    cspGlobals_t *pCspGlobals,
    point_t Psw, 
    point_t Pmin, 
    point_t Pmax)
{
}

FUNCTION
static void CspBeginContourBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    btsbool outside)
{
CheckPoint(pCspGlobals, P1);
}

FUNCTION
static void CspLineToBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1)
{
CheckPoint(pCspGlobals, P1);
}

FUNCTION
static void CspCurveToBBox(
    cspGlobals_t *pCspGlobals,
    point_t P1, 
    point_t P2, 
    point_t P3, 
    fix15 depth)
{
CheckPoint(pCspGlobals, P1);
CheckPoint(pCspGlobals, P2);
CheckPoint(pCspGlobals, P3);
}

FUNCTION
static void CspEndContourBBox(
    cspGlobals_t *pCspGlobals)
{
}


FUNCTION
static btsbool CspEndSubCharBBox(
    cspGlobals_t *pCspGlobals)
{
return TRUE;
}

FUNCTION
static btsbool CspEndCharBBox(
    cspGlobals_t *pCspGlobals)
{
if (sp_globals.xmin > sp_globals.xmax)
    {
    sp_globals.xmin = 0;
    sp_globals.ymin = 0;
    sp_globals.xmax = 0;
    sp_globals.ymax = 0;
    }

return TRUE;
}

FUNCTION
static void CheckPoint(
    cspGlobals_t *pCspGlobals,
    point_t P)
{
if (P.x < sp_globals.xmin)
    sp_globals.xmin = P.x;
if (P.y < sp_globals.ymin)
    sp_globals.ymin = P.y;
if (P.x > sp_globals.xmax)
    sp_globals.xmax = P.x;
if (P.y > sp_globals.ymax)
    sp_globals.ymax = P.y;
}
#endif /*PROC_TRUEDOC*/

