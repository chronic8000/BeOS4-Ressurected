/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 95                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ C R V . C *******************************
 *                                                                           *
 * Character shape player curve processing functions                         *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_CRV.C 7.2 1997/03/18 16:04:21 SHAWN release $
 *                                                                                    
 *     $Log: CSP_CRV.C $
 *     Revision 7.2  1997/03/18 16:04:21  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.1  1996/10/10 14:04:44  mark
 *     Release
 * Revision 6.1  96/08/27  11:57:18  mark
 * Release
 * 
 * Revision 5.2  96/03/21  11:44:34  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:17  mark
 * Release
 * 
 * Revision 4.1  96/03/05  13:45:12  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:51  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:45  mark
 * Release
 * 
 * Revision 1.6  95/09/21  08:54:23  john
 * Reduced stack utilization in recursive version of CspSplitCurve.
 * 
 * Revision 1.5  95/09/19  15:57:37  john
 * Added an alternative curve splitter that uses iteration
 * instead of recursion for environments that need to keep
 * stack space usage to a minimum.
 * 
 * Revision 1.4  95/09/18  13:46:33  john
 * CspSplitCurve now uses tail recursion for second half
 * of curve -- its about 20% faster.
 * 
 * Revision 1.3  95/09/14  14:53:16  john
 * Positive depth values are now correctly interpreted.
 * 
 * Revision 1.2  95/08/31  09:10:51  john
 * Definition of CspSplitCurve now has pointer to LineTo function
 *     as an extra argument.
 * Also CspSplitCurve splits to a measured half-pixel tolerance
 *     if a negative depth value is specified.
 * 
 * Revision 1.1  95/08/10  16:44:37  john
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


/*
 *  Set to 1 to use iterative curve splitter in place of 
 *  the recursive curve splitter. 
 *  The performance is usually similar but you might want 
 *  to do this if:
 *      - function entry and exit is slow on your platform
 *      - Stack space usage needs to be kept to a minimum
 */
#ifndef ITERATIVE_CURVE_SPLIT
#define ITERATIVE_CURVE_SPLIT 0
#endif


#if ITERATIVE_CURVE_SPLIT

typedef struct splitList_tag
	{
	fix15	depth;
	point_t P0;
	point_t P1;
	point_t P2;
	} splitList_t;

#define MAX_SPLIT_DEPTH 8
	
FUNCTION 
void CspSplitCurve(
    cspGlobals_t *pCspGlobals,
    point_t P0,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15   depth,
    void (*LineTo)(
        cspGlobals_t *pCspGlobals,
        point_t P))
/*
 *  If depth > 0, splits curve into 2^depth vectors.
 *  If depth = 0, two vectors are generated.
 *  If depth < 0, splits curve to achieve accuracy within half pixel.
 *  Delivers the resulting vectors via the specified function.
 */
{
splitList_t splitList[MAX_SPLIT_DEPTH + 1];
splitList_t *p;
btsbool	split;

#if CSP_DEBUG >= 2
printf("CRVE(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix,
    (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
    (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix);
#endif

/* Initialize split list */
splitList[0].P0 = P3;
splitList[1].depth = 
	(depth > MAX_SPLIT_DEPTH)?
	MAX_SPLIT_DEPTH:
	depth;
splitList[1].P2 = P2;
splitList[1].P1 = P1;
splitList[1].P0 = P0;
p = splitList + 1;
	
do
	{
	if (depth < 0)                  /* Dynamic splitting mode? */
	    {
	    fix31  xx3, yy3;
	    fix15  x1;
		fix15  y1;
		fix15  x2;
		fix15  y2;
		fix15  x3;
		fix15  y3;
		fix31  doubleArea1;
		fix31  doubleArea2;
		fix31  doubleArea;
		fix31  lengthSquared;

	    split = TRUE;
	    xx3 = (fix31)(p-1)->P0.x - p->P0.x;
	    yy3 = (fix31)(p-1)->P0.y - p->P0.y;
	    if ((xx3 >= -16768) &&
	        (xx3 < 16768) &&
	        (yy3 >= -16768) &&
	        (yy3 < 16768) &&          /* Curve BBox is not too big? */
	        (p < (splitList + MAX_SPLIT_DEPTH)))			/* Space in split list? */
	        {
	        /* Relocate curve to start at origin */
	        x3 = (fix15)xx3;
	        y3 = (fix15)yy3;
	        x1 = p->P1.x - p->P0.x;
	        y1 = p->P1.y - p->P0.y;
	        x2 = p->P2.x - p->P0.x;
	        y2 = p->P2.y - p->P0.y;
	
	        /* Test if tolerance OK with 2 vectors */
	        doubleArea1 = (fix31)x1 * y3 - (fix31)x3 * y1;
	        if (doubleArea1 < 0)
	            doubleArea1 = -doubleArea1;
	        doubleArea2 = (fix31)x2 * y3 - (fix31)x3 * y2;
	        if (doubleArea2 < 0)
	            doubleArea2 = -doubleArea2;
	        doubleArea = (doubleArea1 > doubleArea2)? 
	            doubleArea1: 
	            doubleArea2;
	        doubleArea = (doubleArea + sp_globals.pixrnd) >> sp_globals.pixshift;
	        if (doubleArea < 23170)
	            {
	            lengthSquared = (fix31)x3 * x3 + (fix31)y3 * y3;
	            split = (doubleArea * doubleArea > lengthSquared);
	            }
	        }
	    }
	else                            /* Split to specified depth? */
    	{
    	split = p->depth > 1;
    	}
    
    if (split)
    	{
    	fix31  x01, x12, x23, x012, x123;
		fix31  y01, y12, y23, y012, y123;

		/* Split curve into pair of curves */
		(p+1)->P0.x = p->P0.x;
		x01 = (fix31)(p->P0.x) + p->P1.x;
		(p+1)->P1.x = (fix15)(x01 >> 1);
		x12 = (fix31)(p->P1.x) + p->P2.x;
		x012 = x01 + x12;
		(p+1)->P2.x = (fix15)((x012 + 2) >> 2);
		x23 = (fix31)(p->P2.x) + (p-1)->P0.x;
		p->P2.x = (fix15)(x23 >> 1);
		x123 = x12 + x23;
		p->P1.x = (fix15)((x123 + 2) >> 2);
		p->P0.x = (fix15)((x012 + x123 + 4) >> 3);
		
		(p+1)->P0.y = p->P0.y;
		y01 = (fix31)(p->P0.y) + p->P1.y;
		(p+1)->P1.y = (fix15)(y01 >> 1);
		y12 = (fix31)(p->P1.y) + p->P2.y;
		y012 = y01 + y12;
		(p+1)->P2.y = (fix15)((y012 + 2) >> 2);
		y23 = (fix31)(p->P2.y) + (p-1)->P0.y;
		p->P2.y = (fix15)(y23 >> 1);
		y123 = y12 + y23;
		p->P1.y = (fix15)((y123 + 2) >> 2);
		p->P0.y = (fix15)((y012 + y123 + 4) >> 3);
		
		(p+1)->depth = --p->depth;
		
		p++;
    	}
    else
    	{
    	point_t	Pmid;
    	fix31  tmpfix31;

		/* Output curve as a pair of straight lines */
		tmpfix31 = (fix31)p->P1.x + p->P2.x;
		Pmid.x = (fix15)(
		    ((tmpfix31 << 1) + tmpfix31 + p->P0.x + (p-1)->P0.x + 4) >> 3);
		
		tmpfix31 = (fix31)p->P1.y + p->P2.y;
		Pmid.y = (fix15)(
		    ((tmpfix31 << 1) + tmpfix31 + p->P0.y + (p-1)->P0.y + 4) >> 3);
		
		LineTo(pCspGlobals, Pmid);
		
		LineTo(pCspGlobals, (p-1)->P0);
		
		p--;
    	}
    } while (p != splitList);
}


#else
FUNCTION 
void CspSplitCurve(
    cspGlobals_t *pCspGlobals,
    point_t P0,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15   depth,
    void (*LineTo)(
        cspGlobals_t *pCspGlobals,
        point_t P))
/*
 *  If depth > 0, splits curve into 2^depth vectors.
 *  If depth = 0, two vectors are generated.
 *  If depth < 0, splits curve to achieve accuracy within half pixel.
 *  Delivers the resulting vectors via the specified function.
 */
{
point_t Pmid;

#if CSP_DEBUG >= 2
printf("CRVE(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix,
    (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
    (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix);
#endif

L0:
if (depth < 0)                  /* Dynamic splitting mode? */
    {
    fix31  xx3, yy3;
    fix15  x1;
    fix15  y1;
    fix15  x2;
    fix15  y2;
    fix15  x3;
    fix15  y3;
    fix31  doubleArea1;
    fix31  doubleArea2;
    fix31  doubleArea;
    fix31  lengthSquared;

    xx3 = (fix31)P3.x - P0.x;
    yy3 = (fix31)P3.y - P0.y;
    if ((xx3 >= -16768) &&
        (xx3 < 16768) &&
        (yy3 >= -16768) &&
        (yy3 < 16768))          /* Curve BBox is not too big? */
        {
        /* Relocate curve to start at origin */
        x3 = (fix15)xx3;
        y3 = (fix15)yy3;
        x1 = P1.x - P0.x;
        y1 = P1.y - P0.y;
        x2 = P2.x - P0.x;
        y2 = P2.y - P0.y;

        /* Test if tolerance OK with 2 vectors */
        doubleArea1 = (fix31)x1 * y3 - (fix31)x3 * y1;
        if (doubleArea1 < 0)
            doubleArea1 = -doubleArea1;
        doubleArea2 = (fix31)x2 * y3 - (fix31)x3 * y2;
        if (doubleArea2 < 0)
            doubleArea2 = -doubleArea2;
        doubleArea = (doubleArea1 > doubleArea2)? 
            doubleArea1: 
            doubleArea2;
        doubleArea = (doubleArea + sp_globals.pixrnd) >> sp_globals.pixshift;
        if (doubleArea < 23170)
            {
            lengthSquared = (fix31)x3 * x3 + (fix31)y3 * y3;
            if (doubleArea * doubleArea <= lengthSquared)
                goto L1;
            }
        }
    }
else                            /* Split to specified depth? */
    {
    if (--depth <= 0)           /* OK with 2 vectors? */
        goto L1;
    }

/* Output curve as a pair of curves */
    {
    fix31  x01, x12, x23, x012, x123;
    fix31  y01, y12, y23, y012, y123;
    point_t Pctrl11, Pctrl12;

    x01 = (fix31)P0.x + P1.x;
    Pctrl11.x = (fix15)(x01 >> 1);
    x12 = (fix31)P1.x + P2.x;
    x012 = x01 + x12;
    Pctrl12.x = (fix15)((x012 + 2) >> 2);
    x23 = (fix31)P2.x + P3.x;
    P2.x = (fix15)(x23 >> 1);
    x123 = x12 + x23;
    P1.x = (fix15)((x123 + 2) >> 2);
    Pmid.x = (fix15)((x012 + x123 + 4) >> 3);

    y01 = (fix31)P0.y + P1.y;
    Pctrl11.y = (fix15)(y01 >> 1);
    y12 = (fix31)P1.y + P2.y;
    y012 = y01 + y12;
    Pctrl12.y = (fix15)((y012 + 2) >> 2);
    y23 = (fix31)P2.y + P3.y;
    P2.y = (fix15)(y23 >> 1);
    y123 = y12 + y23;
    P1.y = (fix15)((y123 + 2) >> 2);
    Pmid.y = (fix15)((y012 + y123 + 4) >> 3);
    
    CspSplitCurve(pCspGlobals, P0, Pctrl11, Pctrl12, Pmid, depth, LineTo);
    
    P0 = Pmid;
    goto L0;
    }

L1:
/* Output curve as a pair of straight lines */
    {
    fix31  tmpfix31;

    tmpfix31 = (fix31)P1.x + P2.x;
    Pmid.x = (fix15)(
        ((tmpfix31 << 1) + tmpfix31 + P0.x + P3.x + 4) >> 3);

    tmpfix31 = (fix31)P1.y + P2.y;
    Pmid.y = (fix15)(
        ((tmpfix31 << 1) + tmpfix31 + P0.y + P3.y + 4) >> 3);

    LineTo(pCspGlobals, Pmid);

    LineTo(pCspGlobals, P3);
    }
}
#endif

#endif /*PROC_TRUEDOC*/

