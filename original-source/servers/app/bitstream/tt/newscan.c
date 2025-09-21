/*******************************************************************************
*                                                                              *
*  Copyright 1991-1995 as an unpublished work by Bitstream Inc., Cambridge, MA *
*                                                                              *
*         These programs are the sole property of Bitstream Inc. and           *
*           contain its proprietary and confidential information.              *
*                                                                              *
*******************************************************************************/
/********************* Revision Control Information **************************
 *
*     $Header: //bliss/user/archive/rcs/true_type/newscan.c,v 14.0 97/03/17 17:54:18 shawn Release $
*     $Log:	newscan.c,v $
 * Revision 14.0  97/03/17  17:54:18  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.1  96/07/02  13:46:59  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.0  95/02/15  16:24:33  roberte
 * Release
 * 
 * Revision 9.2  95/01/12  14:47:10  roberte
 *  Updated Copyright header.
 * 
********************* Revision Control Information **************************/

#ifndef NOSCCS
static char SccsId[] = "%W% %G%";
#endif

/* History
 * 25-Feb-93 TBP In "distance" function cast values from long int to double FP 
 *               before squaring to prevent an overflow. The overflow
 *               was resulting negitive values being passed to the square root 
 *               function which then complained about DOMAIN errors.
 * 
 * 18-Feb-93 TBP The Apple Times font UpperCase C contains two contours. 
 *               The 2nd contour contains no points. We now deal with this 
 *               case correctly.
 *
 * 05-Feb-93 TBP rewrote the ns_ProcOutl function with many fewer special
 *	 	 cases. This solves problems we were having with spikes,
 *		 actually they were more like lumps, on the sides of
 *		 characters. The problem showed up in the Apple version of
 *		 the Times-Bold-Font on the upper case O as well as others.
 * 04-Feb-93 TBP Original source from bitstream
 */

#include "spdo_prv.h"
#if PROC_TRUETYPE
#include "fscdefs.h"
#include "fontscal.h"
#include <math.h>


#define  INIT          0
#define  MOVE          1
#define  LINE          2
#define  CURVE         3
#define  END_CONTOUR   4
#define  END_CHAR      5
#define  ERROR        -1
#define  NOP   		  -2

#ifdef OLDWAY
/* those of these that remain are tucked into fsGlyphInfoType structure: */
static btsbool  begin_contour;	/* MOVE flag */
static btsbool  conoffcurpt;	/* set when CONsecutive OFF-CURve PoinTs are encountered */
static uint16      o_contour;	/* contour offset:  [ 0 - numberOfContours-1 ] */
static int      o_point;	/* point offset in character */
static fix15    spt;		/* start point of a contour */
static fix15    ept;		/* end point of a contour */
static fix15    cpt;		/* current point of a contour */
static lpoint_t *vect;		/* vectors from curve rendering are held here */
static F26Dot6 *xCoord, *yCoord;/* coordinates from fs_GlyphInfoType struct */
static uint8   *onCurve;	/* 1 if point on curve, 0 if not */
#endif

GLOBAL_PROTO	void	ns_ProcOutlSetUp(fs_GlyphInfoType *oPtr, lpoint_t *v);
GLOBAL_PROTO	int		ns_ProcOutl(fs_GlyphInfoType *oPtr);



FUNCTION void ns_ProcOutlSetUp(fs_GlyphInfoType *oPtr, lpoint_t *v)
{				/* Initialize character */
    oPtr->begin_contour = TRUE;
    oPtr->o_contour = 0;
    oPtr->vect = v;
    oPtr->xCoord = oPtr->xPtr;
    oPtr->yCoord = oPtr->yPtr;
}

#if INCL_APPLESCAN
/* bit 1 could be set signalling possible OVERLAP */
#define ONCURVE(x) ( oPtr->onCurve[x] & 0x01 )
#else
/* only bit 0 is possibly set */
#define ONCURVE(x) ( oPtr->onCurve[x] )
#endif

#define OFF_OFF_OFF 0
#define OFF_OFF_ON  1
#define OFF_ON_OFF  2
#define OFF_ON_ON   3
#define ON_OFF_OFF 4
#define ON_OFF_ON  5
#define ON_ON_OFF  6
#define ON_ON_ON   7

FUNCTION int ns_ProcOutl(fs_GlyphInfoType *oPtr)

/* The character outlines are described in a 2nd-order B-spline format.
   There are 2 kinds of points: those on the curve and those not on the
   curve. All combinations of points on and off the curve are legal.

   A straight line segment is described by 2 consecutive points on the curve.
   A non-straight B-spline can be broken kown into 1 or more quadratic Bezier
   curve segments. Each Bezier is described by 3 points (A, B, C). Points
   A and C are on the curve; point B is a tangent point outside the curve.
   The tangent direction at point C is the oPtr->vector BC. The parametric equation
   for the quadratic Bezier curve is:
        F(t) = (1-t)*(1-t)*A + 2*t*(1-t)*B + t*t*C   [ 0<=t<=1 ]

   When a quadratic B-spline has consecutive off-curve points, it can be broken
   down into quadratic Bezier splines by creating new points on the curve
   exactly in between every 2 consecutive points lying outside the curve.

   [Preliminary Developer Note, Copyright 1989, Apple Computer, Inc.]
*/
{
    int             btype;
    int             prev, curr, next;
    int             npt;

    if (oPtr->begin_contour) {	/* Process first point of contour (MOVE) */
	/* Loop to skip all contours with NO points */
	do {
	    if (oPtr->o_contour >= oPtr->numberOfContours) {
		return (END_CHAR);	/* all done */
	    }
	    oPtr->o_contour++;
	} while (oPtr->startPtr[oPtr->o_contour-1] == oPtr->endPtr[oPtr->o_contour-1]);
	oPtr->spt = oPtr->startPtr[oPtr->o_contour-1];
	oPtr->ept = oPtr->endPtr[oPtr->o_contour-1];
	oPtr->cpt = oPtr->spt;
    }
    do {
	prev = oPtr->cpt - 1;
	curr = oPtr->cpt;
	next = oPtr->cpt + 1;

	npt = (oPtr->ept - oPtr->spt) + 1;

	if (prev < oPtr->spt)
	    prev += npt;
	if (prev > oPtr->ept)
	    prev -= npt;
	if (curr > oPtr->ept)
	    curr -= npt;
	if (next > oPtr->ept)
	    next -= npt;

	/*
		 * Because we process both a moveto at the start and a
		 * end contour at the end we are not done until we have
		 * processed the number of points plus two.
		 */
	if (oPtr->cpt == oPtr->ept + 2) {
	    oPtr->begin_contour = TRUE;
	    return (END_CONTOUR);
	}
	/* Handle all combinations of on and off curve here */
	switch (ONCURVE(prev) << 2 | ONCURVE(curr) << 1 | ONCURVE(next)) {
	    case OFF_OFF_OFF:
		oPtr->vect[0].x = (oPtr->xCoord[prev] + oPtr->xCoord[curr]) >> 1;
		oPtr->vect[0].y = (oPtr->yCoord[prev] + oPtr->yCoord[curr]) >> 1;
		oPtr->vect[1].x = oPtr->xCoord[curr];
		oPtr->vect[1].y = oPtr->yCoord[curr];
		oPtr->vect[2].x = (oPtr->xCoord[next] + oPtr->xCoord[curr]) >> 1;
		oPtr->vect[2].y = (oPtr->yCoord[next] + oPtr->yCoord[curr]) >> 1;
		btype = CURVE;
		break;

	    case OFF_OFF_ON:
		oPtr->vect[0].x = (oPtr->xCoord[prev] + oPtr->xCoord[curr]) >> 1;
		oPtr->vect[0].y = (oPtr->yCoord[prev] + oPtr->yCoord[curr]) >> 1;
		oPtr->vect[1].x = oPtr->xCoord[curr];
		oPtr->vect[1].y = oPtr->yCoord[curr];
		oPtr->vect[2].x = oPtr->xCoord[next];
		oPtr->vect[2].y = oPtr->yCoord[next];
		btype = CURVE;
		break;

	    case OFF_ON_ON:
	    case OFF_ON_OFF:
		oPtr->vect[0].x = oPtr->xCoord[curr];
		oPtr->vect[0].y = oPtr->yCoord[curr];
		btype = NOP;
		break;

	    case ON_ON_ON:
	    case ON_ON_OFF:
		oPtr->vect[0].x = oPtr->xCoord[curr];
		oPtr->vect[0].y = oPtr->yCoord[curr];
		btype = LINE;
		break;

	    case ON_OFF_OFF:
		oPtr->vect[0].x = oPtr->xCoord[prev];
		oPtr->vect[0].y = oPtr->yCoord[prev];
		oPtr->vect[1].x = oPtr->xCoord[curr];
		oPtr->vect[1].y = oPtr->yCoord[curr];
		oPtr->vect[2].x = (oPtr->xCoord[next] + oPtr->xCoord[curr]) >> 1;
		oPtr->vect[2].y = (oPtr->yCoord[next] + oPtr->yCoord[curr]) >> 1;
		btype = CURVE;
		break;

	    case ON_OFF_ON:
		oPtr->vect[0].x = oPtr->xCoord[prev];
		oPtr->vect[0].y = oPtr->yCoord[prev];
		oPtr->vect[1].x = oPtr->xCoord[curr];
		oPtr->vect[1].y = oPtr->yCoord[curr];
		oPtr->vect[2].x = oPtr->xCoord[next];
		oPtr->vect[2].y = oPtr->yCoord[next];
		btype = CURVE;
		break;
	}

	oPtr->cpt++;

	/*
	 	 * If this is the first point on this contour
	 	 * Return a move instead of a line or curve.
	 	 */
	if (oPtr->begin_contour) {
	    oPtr->begin_contour = FALSE;
	    if (btype == CURVE)
		oPtr->vect[0] = oPtr->vect[2];
	    return (MOVE);
	}
    } while (btype == NOP);

    return (btype);
}
#ifdef OLDCURVES
/*
*
*      THIS FUNCTION APPROXIMATES EUCLIDEAN DISTANCE- ROUGHLY
*
*/

FUNCTION long distance(long x0, long y0, long x1, long y1)
{
/*---------------------------------------------------------
This new algorithm replaces the old distance function with a
conservative numeric approximation. The approximation used
is the length of the longer side plus 1/3 the length of the
shorter side. This is guaranteed to produce a result smaller
than the actual distance. The low side approximation is needed
since the distance calculated by TrueType is the chord connecting
the two knots on the curve. The benefit should be in the 
neighborhood of 8-10 percent over the previous performance of
this routine. Since approximation always yields a smaller value
than the former algorithm, the splitting depth is always greater
than or equal to that calculated by the old routine, so curve
quality is never compromised. Thanks to Mark G.
---------------------------------------------------------*/
long d2, dx, dy;
	dx = x0-x1;
	if (dx < 0)
		dx = -dx; /* absolute value of delta x */
	dy = y0 - y1;
	if (dy < 0)
		dy = -dy; /* absolute value of delta y */
	if (dx > dy)
		return(dx + (dy / 3));	 /* delta x > delta y */
	else
		return(dy + (dx / 3));	 /* delta y > delta x */
}
#endif
#endif /* PROC_TRUETYPE */

