


/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                         U.S. Patent No 4,785,391                           *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/
/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/speedo/out_scrn.c,v 33.0 97/03/17 17:46:54 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	out_scrn.c,v $
*       Revision 33.0  97/03/17  17:46:54  shawn
*       TDIS Version 6.00
*       
*       Revision 32.2  96/08/06  16:35:21  shawn
*       Cast arguments in call to open_bitmap().
*       
*       Revision 32.1  96/07/02  12:09:02  shawn
*       Changed boolean to btsbool.
*       
*       Revision 32.0  95/02/15  16:37:12  shawn
*       Release
*       
*       Revision 31.2  95/01/12  11:29:12  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:39:23  roberte
*       Release
*       
*       Revision 30.3  94/12/28  10:51:52  roberte
*       Added cast to call of sp_vert_line_screen() to silence compiler warnings.
*       
*       Revision 30.2  94/12/14  22:50:42  shawn
*       Converted to ANSI 'C'
*       Modified for support by the K&R conversion utility
*       
*       Revision 30.1  94/06/03  11:44:02  roberte
*       In sp_line_screen(), promote local temp1, temp2 to fix31's.
*       Change calc of dx_dy to stage-wise << 16, avoids overflow
*       on some SPARC stations.
*       
*       Revision 30.0  94/05/04  09:36:15  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:38:59  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:44:39  roberte
*       Release
*       
*       Revision 28.48  94/04/01  15:57:31  roberte
*       Moved some static function prototypes from speedo.h to here.
*       
*       Revision 28.47  93/08/31  09:38:21  roberte
*       Release
*       
*       Revision 28.38  93/03/23  12:43:40  mark
*       When unwinding right-left pairs of intercepts by swapping
*       them, don't swap the END_INT bit.
*       
*       Revision 28.37  93/03/15  12:47:19  roberte
*       Release
*       
*       Revision 28.11  93/02/23  13:24:01  roberte
*       Added parens to eliminate possible ambiguity in && protion of || statement.
*       
*       Revision 28.10  93/01/07  12:06:33  roberte
*       Corrected two comments.
*       
*       Revision 28.9  93/01/04  16:41:08  roberte
*       Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
*       
*       Revision 28.8  92/12/30  17:46:25  roberte
*       Functions no longer renamed in spdo_prv.h now declared with "sp_"
*       Use PARAMS1&2 macros throughout.
*       
*       Revision 28.7  92/12/14  14:55:08  weili
*       Modified the procedure "proc_intercepts_screen" to
*       fix the -x bits problems caused by characters below
*       6 lines
*       
*       Revision 28.6  92/11/24  11:02:21  laurar
*       include fino.h
*       
*       Revision 28.5  92/11/19  15:17:20  roberte
*       Release
*       
*       Revision 28.2  92/10/19  11:24:15  weili
*       modified sp_proc_intercepts_screen(PARAMS1) to fix screen writer bug with overlapping contours such as the lower
*       case m with serifs and pixel catching bug with pixels outside the bounding box
*       
*       Revision 28.1  92/06/25  13:41:04  leeann
*       Release
*       
*       Revision 27.1  92/03/23  14:00:47  leeann
*       Release
*       
*       Revision 26.2  92/02/27  21:50:21  mark
*       make sure that identical intercepts sort left first
*       
*       Revision 26.1  92/01/30  16:59:52  leeann
*       Release
*       
*       Revision 25.4  92/01/29  12:00:20  leeann
*       correct code which swaps intercepts when
*       right intercept preceeds left. - The error
*       was that in the case of banding, rather than
*       linking linking to the previous intercept after
*       the swap, it was alway linking to the first
*       intercept.
*       
*       Revision 25.3  91/11/27  12:32:41  mark
*       fixed infinite loop in sp_proc_intercepts caused by extremely
*       kinked outlines at small sizes.
*       
*       Revision 25.2  91/10/23  17:05:34  leeann
*       don't let pixel catching logic go to left of minimum bbox
*       
*       Revision 25.1  91/07/10  11:05:49  leeann
*       Release
*       
*       Revision 24.1  91/07/10  10:39:25  leeann
*       Release
*       
*       Revision 23.1  91/07/09  18:00:25  leeann
*       Release
*       
*       Revision 22.2  91/06/19  14:34:17  leeann
*       move code into set_clip_parameters() in out_util
*       
*       Revision 22.1  91/01/23  17:19:32  leeann
*       Release
*       
*       Revision 21.1  90/11/20  14:38:47  leeann
*       Release
*       
*       Revision 20.2  90/11/20  13:16:51  leeann
*       fixed clipping precisionfixed clipping precision
*       
*       Revision 20.1  90/11/12  09:31:22  leeann
*       Release
*       
*       Revision 19.1  90/11/08  10:21:03  leeann
*       Release
*       
*       Revision 18.2  90/11/07  15:40:57  leeann
*        implement clipping for rotation of 90, 180, and 270 degrees
*       
*       Revision 18.1  90/09/24  10:11:02  mark
*       Release
*       
*       Revision 17.1  90/09/13  16:05:24  mark
*       Release name rel0913
*       
*       Revision 16.1  90/09/11  13:20:10  mark
*       Release
*       
*       Revision 15.1  90/08/29  10:08:50  mark
*       Release name rel0829
*       
*       Revision 14.4  90/08/29  09:56:53  judy
*        fix syntax error in interchar spacing
*       
*       Revision 14.3  90/08/28  17:24:48  judy
*       fix interchar spacing fix - xmode = 4 has no rounding error
*       
*       Revision 14.2  90/08/28  16:36:25  judy
*       fix inter-character spacing bug in end_char: add the round
*       error based on the xmode and ymode type to either xorg or yorg.
*       
*       Revision 14.1  90/07/13  10:46:57  mark
*       Release name rel071390
*       
*       Revision 13.1  90/07/02  10:45:42  mark
*       Release name REL2070290
*       
*       Revision 12.8  90/06/29  15:30:47  mark
*       when checking for opposing convex points in Y, and 
*       jplus1 has to be inverted to account for a mirror
*       transformation, check for wrapping the end of the contour
*       
*       Revision 12.7  90/06/26  08:59:00  leeann
*        When CLIPPED characters go into banding, save the
*        correct ymin and ymax
*       
*       Revision 12.6  90/06/18  16:19:52  mark
*       augment termination condition to terminate on node
*       where x0 = y0 = (?)ffff and x3 = y3 = (?+1)0000
*       
*       Revision 12.5  90/06/18  12:25:52  mark
*       remove dependency on setting of P0, use x0_spxl, y0_spxl instead
*       also, clean up debug statements so they are useful.
*       
*       Revision 12.4  90/06/06  16:42:03  judy
*       fix inter-character spacing
*       
*       Revision 12.3  90/06/01  15:25:34  mark
*       use mirror flag to determine whether an up or down vector
*       is a left edge.  ALso need to check mirror to decide
*       if a local minimum is a whitespace valley for continuity
*       checking.
*       
*       Revision 12.2  90/05/31  17:01:56  mark
*       Skipping of continuity check in band mode caused linked 
*       list index not to be properly updated
*       
*       Revision 12.1  90/04/23  12:16:55  mark
*       Release name REL20
*       
*       Revision 11.1  90/04/23  10:17:14  mark
*       Release name REV2
*       
*       Revision 1.6  90/04/23  09:43:45  mark
*       use sp_intercepts for intercept lists so that reentrant
*       configurations will work
*       
*       Revision 1.5  90/04/20  11:48:23  judy
*       put resetting of xmin/xmax back into end_char.
*       add rounding error from xform back into left
*       edge of char, end_char.
*       
*       Revision 1.4  90/04/18  08:35:53  mark
*       fix treatment of selfintersecting paths
*       
*       Revision 1.3  90/04/09  11:55:45  mark
*       declare scan_curve and vert_line functions as void
*       
*       Revision 1.2  90/04/06  12:35:18  mark
*       enable y clipping through band_min/max 
*       also include curve scanning
*       
*       Revision 1.1  90/03/30  14:28:17  mark
*       Initial revision
*       
*                                                                                 *
*                                                                                    *
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/out_scrn.c,v 33.0 97/03/17 17:46:54 shawn Release $";
#endif




/*************************** O U T _ S C R N . C *****************************
 *                                                                           *
 * This is an output module for screen-writer mode.                          *
 *                                                                           *
 *****************************************************************************/


#include "spdo_prv.h"               /* General definitions for Speedo   */
#include "fino.h"

#define   DEBUG      0
#define   LOCAL      static
#define   ABS(X)     ( (X < 0) ? -X : X)

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif


/***** GLOBAL VARIABLES *****/

/***** GLOBAL FUNCTIONS *****/

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/
LOCAL_PROTO void sp_add_intercept_screen(SPGLOBPTR2 fix15 y, fix31 x);
LOCAL_PROTO void sp_proc_intercepts_screen(SPGLOBPTR1);



#if INCL_SCREEN

FUNCTION btsbool sp_init_screen(SPGLOBPTR2 specs_t FONTFAR *specsarg)
/*
 * init_out0() is called by sp_set_specs(PARAMS1) to initialize the output module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
{

#if DEBUG
printf("INIT_SCREEN()\n");
#endif

return (TRUE);
}
#endif


#if INCL_SCREEN

FUNCTION btsbool sp_begin_char_screen(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/* Called once at the start of the character generation process
 */
{

#if DEBUG
printf("BEGIN_CHAR_SCREEN(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n", 
                    (real)Psw.x / (real)sp_globals.onepix, (real)Psw.y / (real)sp_globals.onepix,
                    (real)Pmin.x / (real)sp_globals.onepix, (real)Pmin.y / (real)sp_globals.onepix,
                    (real)Pmax.x / (real)sp_globals.onepix, (real)Pmax.y / (real)sp_globals.onepix);
#endif

if (sp_globals.pixshift > 8)
	sp_intercepts.fracpix = sp_globals.onepix << (8 - sp_globals.pixshift);
else
	sp_intercepts.fracpix = sp_globals.onepix >> (sp_globals.pixshift - 8);

sp_init_char_out(PARAMS2 Psw,Pmin,Pmax);

return TRUE;
}
#endif



#if INCL_SCREEN

FUNCTION void sp_begin_contour_screen(SPGLOBPTR2 point_t P1, btsbool outside)
/* Called at the start of each contour
 */
{

#if DEBUG
printf("BEGIN_CONTOUR_SCREEN(%3.1f, %3.1f, %s)\n", 
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix, outside? "outside": "inside");
#endif

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
sp_globals.y_pxl = (sp_globals.y0_spxl + sp_globals.pixrnd) >> sp_globals.pixshift;
}
#endif


#if INCL_SCREEN

FUNCTION void sp_curve_screen(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
{
fix31   X0;
fix31   Y0;
fix31   X1;
fix31   Y1;
fix31   X2;
fix31   Y2;
fix31   X3;
fix31   Y3;

#if DEBUG
printf("CURVE_SCREEN(%6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f)\n", 
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix,
    (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
    (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix);
#endif


if (sp_globals.extents_running) /* Accumulate actual character extents if required */
    {
    if (P3.x > sp_globals.bmap_xmax)         
        sp_globals.bmap_xmax = P3.x;
    if (P3.x < sp_globals.bmap_xmin)
        sp_globals.bmap_xmin = P3.x;
    if (P3.y > sp_globals.bmap_ymax)
        sp_globals.bmap_ymax = P3.y;
    if (P3.y < sp_globals.bmap_ymin)
        sp_globals.bmap_ymin = P3.y;
    }

X0 = ((fix31)sp_globals.x0_spxl << sp_globals.poshift) + (fix31)32768;
Y0 = ((fix31)sp_globals.y0_spxl << sp_globals.poshift) + (fix31)32768;
X1 = ((fix31)P1.x << sp_globals.poshift) + (fix31)32768;
Y1 = ((fix31)P1.y << sp_globals.poshift) + (fix31)32768;
X2 = ((fix31)P2.x << sp_globals.poshift) + (fix31)32768;
Y2 = ((fix31)P2.y << sp_globals.poshift) + (fix31)32768;
X3 = ((fix31)P3.x << sp_globals.poshift) + (fix31)32768;
Y3 = ((fix31)P3.y << sp_globals.poshift) + (fix31)32768;

if (((Y0 - Y3) * sp_globals.tcb.mirror) > 0)
	{
	sp_intercepts.leftedge = LEFT_INT;
	}
else
	{
	sp_intercepts.leftedge = 0;
	}

sp_scan_curve_screen(PARAMS2 X0,Y0,X1,Y1,X2,Y2,X3,Y3);
sp_globals.x0_spxl = P3.x;
sp_globals.y0_spxl = P3.y;
sp_globals.y_pxl = (P3.y + sp_globals.pixrnd) >> sp_globals.pixshift;   /* calculate new end-scan sp_globals.line */
}

FUNCTION void sp_scan_curve_screen(SPGLOBPTR2
								   fix31 X0,
								   fix31 Y0,
								   fix31 X1,
								   fix31 Y1,
								   fix31 X2,
								   fix31 Y2,
								   fix31 X3,
								   fix31 Y3)
/* Called for each curve in the transformed character if curves out enabled */
{
fix31 Pmidx;
fix31 Pmidy;
fix31 Pctrl1x;
fix31 Pctrl1y;
fix31 Pctrl2x;
fix31 Pctrl2y;

#if DBGCRV
printf("SCAN_CURVE_SCREEN(%6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f)\n", 
    (real)(X0-32768) / 65536.0, (real)(Y0-32768) / 65536.0,
    (real)(X1-32768) / 65536.0, (real)(Y1-32768) / 65536.0,
    (real)(X2-32768) / 65536.0, (real)(Y2-32768) / 65536.0,
    (real)(X3-32768) / 65536.0, (real)(Y3-32768) / 65536.0);
#endif

if (((Y3 >> 16)) == (Y0 >> 16) || (Y3+1) == Y0 || Y3 == (Y0+1))
    {
	return;
    }
if ((X3 >> 16) == (X0 >> 16))
    {
    sp_vert_line_screen(PARAMS2 X3,(fix15)(Y0>>16),(fix15)(Y3>>16));
	return;
    }
Pmidx = (X0 + (X1 + X2) * 3 + X3 + 4 ) >> 3;
Pmidy = (Y0 + (Y1 + Y2) * 3 + Y3 + 4 ) >> 3;

Pctrl1x = (X0 + X1 + 1 ) >> 1;
Pctrl1y = (Y0 + Y1 + 1) >> 1;
Pctrl2x = (X0 + (X1 << 1) + X2 + 2 ) >> 2;
Pctrl2y = (Y0 + (Y1 << 1) + Y2 + 2 ) >> 2;
sp_scan_curve_screen(PARAMS2 X0,Y0, Pctrl1x, Pctrl1y, Pctrl2x,Pctrl2y, Pmidx,Pmidy);

Pctrl1x = (X1 + (X2 << 1) + X3 + 2 ) >> 2;
Pctrl1y = (Y1 + (Y2 << 1) + Y3 + 2 ) >> 2;
Pctrl2x = (X2 + X3 + 1 ) >> 1;
Pctrl2y = (Y2 + Y3 + 1 ) >> 1;
sp_scan_curve_screen(PARAMS2 Pmidx,Pmidy, Pctrl1x,Pctrl1y, Pctrl2x,Pctrl2y, X3,Y3);
}             


FUNCTION void sp_vert_line_screen(SPGLOBPTR2 fix31 x, fix15 y1, fix15 y2)
{                                                 

#if DBGCRV
printf("VERT_LINE_SCREEN(%6.4f, %6.4f, %6.4f)\n",
    (real)(x - 32768) / 65536.0, 
    (real)(y1 - 32768) / 65536.0,
    (real)(y2 - 32768) / 65536.0);
#endif

if (sp_globals.intercept_oflo) 
    return;

if (y1 > y2)                                   /* Line goes downwards ? */
	{
    if (y1 > (sp_globals.y_band.band_max + 1)) /* Start point above top of band? */
        y1 = sp_globals.y_band.band_max + 1;   /* Adjust start point to top of band */
    if (y2 < sp_globals.y_band.band_min)       /* End point below bottom of band? */
        y2 = sp_globals.y_band.band_min;       /* Adjust end point bottom of band */

	y1 -= sp_globals.y_band.band_min;          /* Translate start point to band origin */
	y2 -= sp_globals.y_band.band_min;          /* Translate end point to band origin */

	while (y2 < y1)                            /* At least one intercept left? */
		{
		sp_add_intercept_screen(PARAMS2 --y1, x);           /* Add intercept */
		}
	}
else if (y2 > y1)                              /* Line goes upwards ? */
	{
    if (y1 < sp_globals.y_band.band_min)       /* Start point below bottom of band? */
        y1 = sp_globals.y_band.band_min;       /* Adjust start point to bottom of band */
    if (y2 > (sp_globals.y_band.band_max + 1)) /* End point above top of band? */
        y2 = sp_globals.y_band.band_max + 1;   /* Adjust end point to top of band */

	y1 -= sp_globals.y_band.band_min;          /* Translate start point to band origin */
	y2 -= sp_globals.y_band.band_min;          /* Translate end point to band origin */

	while (y1 < y2)                            /* At least one intercept left? */
		{
		sp_add_intercept_screen(PARAMS2 y1++, x);           /* Add intercept */
		}
	}


}
	
#endif


#if INCL_SCREEN

FUNCTION void sp_line_screen(SPGLOBPTR2 point_t P1)
/* Called for each vector in the transformed character */
{
register fix15     how_many_y;       /* # of intercepts at y = n + 1/2  */
register fix15     yc;               /* Current scan-line */
         fix31     temp1;            /* various uses */
         fix31     temp2;            /* various uses */
register fix31     dx_dy;            /* slope of line in 16.16 form */
register fix31     xc;               /* high-precision (16.16) x coordinate */
         fix15     x0,y0,x1,y1;      /* PIX.FRAC start and endpoints */

x0 = sp_globals.x0_spxl;                 /* get start of line (== current point) */
y0 = sp_globals.y0_spxl;
sp_globals.x0_spxl = x1 = P1.x; /* end of line */
sp_globals.y0_spxl = y1 = P1.y; /*  (also update current point to end of line) */

yc = sp_globals.y_pxl;                   /* current scan line = end of last line */
sp_globals.y_pxl = (y1 + sp_globals.pixrnd) >> sp_globals.pixshift;   /* calculate new end-scan sp_globals.line */


#if DEBUG
printf("LINE_SCREEN(%3.4f, %3.4f)\n", 
       (real)P1.x/(real)sp_globals.onepix, 
       (real)P1.y/(real)sp_globals.onepix);
#endif

if (sp_globals.extents_running)
    {
    if (sp_globals.x0_spxl > sp_globals.bmap_xmax)         
        sp_globals.bmap_xmax = sp_globals.x0_spxl;
    if (sp_globals.x0_spxl < sp_globals.bmap_xmin)
        sp_globals.bmap_xmin = sp_globals.x0_spxl;
    if (sp_globals.y0_spxl > sp_globals.bmap_ymax)
        sp_globals.bmap_ymax = sp_globals.y0_spxl;
    if (sp_globals.y0_spxl < sp_globals.bmap_ymin)
        sp_globals.bmap_ymin = sp_globals.y0_spxl;
    }

if (sp_globals.intercept_oflo) return;

if ((how_many_y = sp_globals.y_pxl - yc) == 0) return; /* Don't draw a null line */

xc = (fix31)(x0 + sp_globals.pixrnd) << (16 - sp_globals.pixshift); /* Original x coordinate with built in  */
                                            /* rounding. 16.16 form */

if (how_many_y < 0)
	{
	yc--; /* Predecrment downward lines */
	}

if ((how_many_y * sp_globals.tcb.mirror) < 0)
	{
	sp_intercepts.leftedge = LEFT_INT;
	}
else
	{
	sp_intercepts.leftedge = 0;
	}

if (yc > sp_globals.y_band.band_max) /* Is start point above band? */
    {
    if (sp_globals.y_pxl > sp_globals.y_band.band_max) return; /* line has to go down! */
    how_many_y = sp_globals.y_pxl - (yc = sp_globals.y_band.band_max) - 1; /* Yes, limit it */
    }

if (yc < sp_globals.y_band.band_min)   /* Is start point below band? */
    {
    if (sp_globals.y_pxl < sp_globals.y_band.band_min) return; /* line has to go up! */
    how_many_y = sp_globals.y_pxl - (yc = sp_globals.y_band.band_min);   /* Yes, limit it */
    }

if ( (temp1 = (x1 - x0)) == 0)  /* check for vertical line */
    {
    dx_dy = 0L; /* Zero slope, leave xc alone */
    goto skip_calc;
    }
          
/* calculate dx_dy at 16.16 fixed point */

/* execute the following in stages to prevent overflow on some SPARC platforms:
	dx_dy = ( (fix31)temp1 << 16 )/(fix31)(y1 - y0);
*/
dx_dy = ( (fix31)temp1 << 15 )/(fix31)(y1 - y0);
dx_dy <<= 1;

/* We have to check for a @#$%@# possible multiply overflow  */
/* by doing another @#$*& multiply.  In assembly language,   */
/* the program could just check the OVerflow flag or whatever*/
/* works on the particular processor.  This C code is meant  */
/* to be processor independant.                              */

temp1 = (yc << sp_globals.pixshift) - y0 + sp_globals.pixrnd;
/* This sees if the sign bits start at bit 15 */
/* if they do, no overflow has occurred       */

temp2 = (fix15)(MULT16(temp1,(fix15)(dx_dy >> 16)) >> 15);

if (  (temp2 != (fix15)0xFFFF) &&
      (temp2 != 0x0000)   )
    {  /* Overflow. Pick point closest to yc + .5 */
    if (ABS(temp1) < ABS((yc << sp_globals.pixshift) - y1 + sp_globals.pixrnd))
        { /* use x1 instead of x0 */
        xc = (fix31)(x1 + sp_globals.pixrnd) << (16 - sp_globals.pixshift);
        }
    goto skip_calc;
    }
/* calculate new xc at the center of the *current* scan line */
/* due to banding, yc may be several lines away from y0      */
/*  xc += (yc + .5 - y0) * dx_dy */
/* This multiply generates a subpixel delta. */
/* So we shift it to be a 16.16 delta */

xc += ((fix31)temp1 * dx_dy) >> sp_globals.pixshift;

skip_calc:

yc -= sp_globals.y_band.band_min; /* yc is now an offset relative to the band */

if (how_many_y < 0)
    {   /* Vector down */
    if ((how_many_y += yc + 1) < 0) how_many_y = 0; /* can't go below 0 */
    while(yc >= how_many_y)
        {
        sp_add_intercept_screen(PARAMS2 yc--,xc); 
        xc -= dx_dy;
        }
    }
    else
    {   /* Vector up */
     /* check to see that line doesn't extend beyond top of band */
    if ((how_many_y += yc) > sp_globals.no_y_lists) how_many_y = sp_globals.no_y_lists;
    while(yc != how_many_y)
        {
        sp_add_intercept_screen(PARAMS2 yc++,xc); 
        xc += dx_dy;
        }
    }
}
#endif


#if INCL_SCREEN

FUNCTION void sp_end_contour_screen(SPGLOBPTR1)
/* Called after the last vector in each contour */
{

#if DEBUG
printf("END_CONTOUR_SCREEN()\n");
#endif

sp_intercepts.inttype[sp_globals.next_offset-1] |= END_INT;
}
#endif


#if INCL_SCREEN

FUNCTION btsbool sp_end_char_screen(SPGLOBPTR1)
/* Called when all character data has been output
 * Return TRUE if output process is complete
 * Return FALSE to repeat output of the transformed data beginning
 * with the first contour
 */
{

fix31 xorg;
fix31 yorg;


#if DEBUG
printf("END_CHAR_SCREEN()\n");
#endif

if (sp_globals.first_pass)
    {
    if (sp_globals.bmap_xmax >= sp_globals.bmap_xmin)
        {
        sp_globals.xmin = (sp_globals.bmap_xmin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
        sp_globals.xmax = (sp_globals.bmap_xmax + sp_globals.pixrnd) >> sp_globals.pixshift;
        }
    else
        {
        sp_globals.xmin = sp_globals.xmax = 0;
        }
    if (sp_globals.bmap_ymax >= sp_globals.bmap_ymin)
        {

#if INCL_CLIPPING
set_clip_parameters();
if ( !(sp_globals.specs.flags & CLIP_TOP))
#endif
            sp_globals.ymax = (sp_globals.bmap_ymax + sp_globals.pixrnd) >> sp_globals.pixshift;

#if INCL_CLIPPING
if ( !(sp_globals.specs.flags & CLIP_BOTTOM))
#endif

        sp_globals.ymin = (sp_globals.bmap_ymin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
        }
    else
        {
        sp_globals.ymin = sp_globals.ymax = 0;
        }

    /* add in the rounded out part (from xform.) of the left edge */
    if (sp_globals.tcb.xmode == 0)    /* for X pix is function of X orus only add the round */
    	xorg = (((fix31)sp_globals.xmin << 16) + (sp_globals.rnd_xmin << sp_globals.poshift));
    else
        if (sp_globals.tcb.xmode == 1) /* for X pix is function of -X orus only, subtr. round */
        	xorg = (((fix31)sp_globals.xmin << 16) - (sp_globals.rnd_xmin << sp_globals.poshift)) ;
        else
        	xorg = (fix31)sp_globals.xmin << 16;   /* for other cases don't use round on x */
           
    if (sp_globals.tcb.ymode == 2)   /* for Y pix is function of X orus only, add round error */ 
    	yorg = (((fix31)sp_globals.ymin << 16) + (sp_globals.rnd_xmin << sp_globals.poshift));
    else
        if (sp_globals.tcb.ymode == 3) /* for Y pix is function of -X orus only, sub round */
        	yorg = (((fix31)sp_globals.ymin << 16) - (sp_globals.rnd_xmin << sp_globals.poshift));
        else                          /* all other cases have no round error on yorg */
         	yorg = (fix31)sp_globals.ymin << 16;

    open_bitmap(sp_globals.set_width.x, sp_globals.set_width.y, xorg, yorg,
		(fix15)(sp_globals.xmax - sp_globals.xmin),
                (fix15)(sp_globals.ymax -  sp_globals.ymin));

    if (sp_globals.intercept_oflo)
        {
        sp_globals.y_band.band_min = sp_globals.ymin;
        sp_globals.y_band.band_max = sp_globals.ymax;
        sp_init_intercepts_out(PARAMS1);
        sp_globals.first_pass = FALSE;
        sp_globals.extents_running = FALSE;
        return FALSE;
        }
    else
        {
        sp_proc_intercepts_screen(PARAMS1);
        close_bitmap();
        return TRUE;
        }
    }
else
    {
    if (sp_globals.intercept_oflo)
        {
        sp_reduce_band_size_out(PARAMS1);
        sp_init_intercepts_out(PARAMS1);
        return FALSE;
        }
    else
        {
        sp_proc_intercepts_screen(PARAMS1);
        if (sp_next_band_out(PARAMS1))
            {
            sp_init_intercepts_out(PARAMS1);
            return FALSE;
            }
        close_bitmap();
        return TRUE;
        }
    }
}
#endif


#if INCL_SCREEN

FUNCTION LOCAL void sp_add_intercept_screen(SPGLOBPTR2 fix15 y, fix31 x)
/*  Called by line() to add an intercept to the intercept list structure */
{
register fix15 from;   /* Insertion pointers for the linked list sort */
register fix15 to;
register fix15 xloc;
register fix15 xfrac;

#if DEBUG
printf("    Add intercept(%2d, %d)\n", y + sp_globals.y_band.band_min,x);

/* Bounds checking IS done in debug mode */
if (y < 0)       /* Y value below bottom of current band? */
    {
    printf(" Intecerpt less than 0!!!\007\n");
    return;
    }

if (y > (sp_globals.no_y_lists - 1))              /* Y value above top of current band? */
    {
    printf(" Intercept too big for band!!!!!\007\n");
    return;
    }
#endif

/* Store new values */

sp_intercepts.car[sp_globals.next_offset] = xloc = (fix15)(x >> 16);
sp_intercepts.inttype[sp_globals.next_offset] = sp_intercepts.leftedge | (xfrac = ((x >> 8) & FRACTION));

/* Find slot to insert new element (between from and to) */

from = y; /* Start at list head */

while( (to = sp_intercepts.cdr[from]) != 0) /* Until to == end of list */
    {
    if (xloc < sp_intercepts.car[to]) /* If next item is larger than or same as this one... */
        goto insert_element; /* ... drop out and insert here */
	else if (xloc == sp_intercepts.car[to])
		{
		if (xfrac < (sp_intercepts.inttype[to] & FRACTION) ||
				xfrac == ((sp_intercepts.inttype[to] & FRACTION) && sp_intercepts.leftedge))
	        goto insert_element; /* ... drop out and insert here */
		}
    from = to; /* move forward in list */
    }

insert_element: /* insert element "sp_globals.next_offset" between elements "from" */
                /* and "to" */

sp_intercepts.cdr[from] = sp_globals.next_offset;
sp_intercepts.cdr[sp_globals.next_offset] = to;

if (++sp_globals.next_offset >= MAX_INTERCEPTS) /* Intercept buffer full? */
    {
    sp_globals.intercept_oflo = TRUE;
/* There may be a few more calls to "add_intercept" from the current line */
/* To avoid problems, we set next_offset to a safe value. We don't care   */
/* if the intercept table gets trashed at this point                      */
    sp_globals.next_offset = sp_globals.first_offset;
    }
}

#endif


#if INCL_SCREEN

FUNCTION LOCAL void sp_proc_intercepts_screen(SPGLOBPTR1)
/*  Called by sp_make_char to output accumulated intercept lists
 *  Clips output to sp_globals.xmin, sp_globals.xmax, sp_globals.ymin, sp_globals.ymax boundaries
 */
{
register fix15 i,j, jplus1, iminus1;
fix15 k,nextk, previ;
register fix15 from, to;          /* Start and end of run in pixel units   
                            relative to left extent of character  */
register fix15 y;
register fix15 scan_line;
         fix15 first_y, last_y;
         fix15 xsave;
         fix15 xmin, xmax;
         btsbool clipleft, clipright;


fix15 diff, wind_num=0;

/* fixed the -x bits problems cause by characters below 6 lines */
if ( sp_globals.xmax <= sp_globals.xmin )
    return;


#if DEBUG
printf("\nPROC_INTERCEPTS_SCREEN: Intercept lists before:\n");
#endif

#if INCL_CLIPPING
if ((sp_globals.specs.flags & CLIP_LEFT) != 0)
    clipleft = TRUE;
else
    clipleft = FALSE;
if ((sp_globals.specs.flags & CLIP_RIGHT) != 0)
    clipright = TRUE;
else
    clipright = FALSE;
if (clipleft || clipright)
        {
        xmax = sp_globals.clip_xmax + sp_globals.xmin;
        xmin = sp_globals.clip_xmin + sp_globals.xmin;
        }
if (!clipright)
        xmax = ((sp_globals.set_width.x+32768L) >> 16);
#endif

if ((first_y = sp_globals.y_band.band_max) >= sp_globals.ymax)    
    first_y = sp_globals.ymax - 1;               /* Clip to sp_globals.ymax boundary */

if ((last_y = sp_globals.y_band.band_min) < sp_globals.ymin)      
    last_y = sp_globals.ymin;                    /* Clip to sp_globals.ymin boundary */

last_y  -= sp_globals.y_band.band_min;

#if DEBUG
/* Print out all of the intercept info */
scan_line = sp_globals.ymax - first_y - 1;

for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
    {
    i = y;                            /* Index head of intercept list */
    while ((i = sp_intercepts.cdr[i]) != 0)         /* Link to next intercept if present */
        {
        if ((from = sp_intercepts.car[i] - sp_globals.xmin) < 0)
            from = 0;                 /* Clip to sp_globals.xmin boundary */
        i = sp_intercepts.cdr[i];                   /* Link to next intercept */
        if (i == 0)                   /* End of list? */
            {
            printf("****** proc_intercepts: odd number of intercepts\n");
            break;
            }
        if ((to = sp_intercepts.car[i]) > sp_globals.xmax)
            to = sp_globals.xmax - sp_globals.xmin;         /* Clip to sp_globals.xmax boundary */
        else
            to -= sp_globals.xmin;
        printf("    Y = %2d (scanline %2d): %d %d:\n", 
            y + sp_globals.y_band.band_min, scan_line, from, to);
        }
    }
#endif

/* CHECK INTERCEPT LIST FOR DROPOUT AND WINDING, FIX IF NECESSARY  */

for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--)
    {
	previ = y;
    i = sp_intercepts.cdr[y];                            /* Index head of intercept list */
    while (i  != 0)         /* Link to next intercept if present */
        {
	j = i;
        i = sp_intercepts.cdr[i];              /* Link to next intercept */
	if (sp_intercepts.inttype[i] & LEFT_INT)
	{
	        /* counting the even odd winding number to pair
		   the intercept list correctly */
		if (sp_intercepts.inttype[j] & LEFT_INT)
		{
		    wind_num = 2;
		    while (wind_num != 0)
		    {
			i = sp_intercepts.cdr[i];
			if (sp_intercepts.inttype[i] & LEFT_INT)
			    wind_num++;
			else
			    wind_num--;
		    }
		    sp_intercepts.cdr[j] = i;

		}
		else
		{
				/* this code is catching winding - In the case
				   where a right edge preceeds a left edge,
				   swap the edges. */
		    xsave = sp_intercepts.car[j];
		    sp_intercepts.car[j] = sp_intercepts.car[i];
		    sp_intercepts.car[i] = xsave;

		    xsave = sp_intercepts.inttype[j];
		    sp_intercepts.inttype[j] = (sp_intercepts.inttype[i] & FRACTION) | (xsave & END_INT);
		    sp_intercepts.inttype[i] = (sp_intercepts.inttype[i] & END_INT) | 
                                  ((xsave & FRACTION) | LEFT_INT);

		    sp_intercepts.cdr[previ] = i;
		    sp_intercepts.cdr[j] = sp_intercepts.cdr[i];
		    sp_intercepts.cdr[i] = j;
		    i = j;
		    j = sp_intercepts.cdr[previ];
		}
	}

        if (sp_intercepts.car[j] < sp_globals.xmin)
            sp_intercepts.car[j] = sp_globals.xmin; /* Clip to sp_globals.xmin boundary */

        if (sp_intercepts.car[i] > sp_globals.xmax)
            sp_intercepts.car[i] = sp_globals.xmax;

        if (sp_intercepts.car[j] >= sp_intercepts.car[i])
	{
	    if ((ufix16)(sp_intercepts.inttype[j] & FRACTION)  +
	    (ufix16)(sp_intercepts.inttype[i] & FRACTION) > sp_intercepts.fracpix)
		/*
		   this code is trying to check if the right intercept
		   exceed sp_globals.xmax, if it does, decrement the
		   left intercept instead of incrementing the right
		   intercept
		*/
 		if (sp_intercepts.car[i] >= sp_globals.xmax)
 		    --sp_intercepts.car[j];
 	        else
 		    ++sp_intercepts.car[i];

	    else
	    {
		/* note: this code is trying to catch */
		/* pixel dropout,and it adds a pixel either */
		/*to the left or right depending on where the */
		/* center lies.  If the pixel center is on */
		/* the left - put it on the right anyway if */
		/* putting it on the left will give us a neg */
		/* start value */
		if (sp_intercepts.car[j] == sp_globals.xmin)
		    ++sp_intercepts.car[i];
		else
		    --sp_intercepts.car[j];
	    }
	}
	if (sp_globals.first_pass)
			{
			if (sp_intercepts.inttype[i-1] & END_INT)
				{
				for (iminus1 = i+1; !(sp_intercepts.inttype[iminus1] & END_INT); iminus1++)
					;
				}
			else
				iminus1 = i-1;
	
			if (sp_intercepts.inttype[j] & END_INT)
				{
				for (jplus1 = j-1; !(sp_intercepts.inttype[jplus1] & END_INT); jplus1--)
					;
				jplus1++;
				}
			else
				jplus1 = j+1;

			if ((sp_intercepts.inttype[iminus1] & LEFT_INT)) 
				{
				if ( sp_intercepts.car[jplus1] > sp_intercepts.car[i])
					{
					diff = sp_intercepts.car[jplus1] - sp_intercepts.car[i];
					sp_intercepts.car[i] += diff/2;
	                sp_intercepts.car[jplus1] -= diff/2;
					if (diff & 1)
						{
						if ((ufix16)(sp_intercepts.inttype[i] & FRACTION) + (ufix16)(sp_intercepts.inttype[jplus1] & FRACTION) > sp_intercepts.fracpix)
							sp_intercepts.car[i] ++;
						else
							sp_intercepts.car[jplus1]--;
						}
					}
				}
			else if (!(sp_intercepts.inttype[jplus1] & LEFT_INT))
				{
				if (sp_intercepts.car[iminus1] < sp_intercepts.car[j])
					{
					diff = sp_intercepts.car[j] - sp_intercepts.car[iminus1];
					sp_intercepts.car[j] -= diff/2;
	                sp_intercepts.car[iminus1] += diff/2;
					if (diff & 1)
						{
						if ((ufix16)(sp_intercepts.inttype[j] & FRACTION) + 
	                                       (ufix16)(sp_intercepts.inttype[iminus1] & FRACTION) > sp_intercepts.fracpix)
							sp_intercepts.car[iminus1]++;
						else
							sp_intercepts.car[j]--;
						}
					}
				}
			if (sp_globals.tcb.mirror == -1) 
				{
				if (sp_intercepts.inttype[j-1] & END_INT)
					{
					for (jplus1 = j+1; !(sp_intercepts.inttype[jplus1] & END_INT); jplus1++)
						;
					}
				else
					{
					jplus1 = j-1;
					}
				}

			if (!(sp_intercepts.inttype[jplus1] & LEFT_INT) && 
                           sp_intercepts.car[j] > sp_intercepts.car[jplus1])
				{
				k = sp_intercepts.cdr[y - 1];
				while (k > 0)
					{
					nextk = sp_intercepts.cdr[k];
					if (!(sp_intercepts.inttype[k] & LEFT_INT) && 
	                        (sp_intercepts.inttype[nextk] & LEFT_INT) && 
						         sp_intercepts.car[nextk] > sp_intercepts.car[jplus1])
						{
						if ((diff=sp_intercepts.car[j] - sp_intercepts.car[k]) > 0)
							{
							if (diff <= (sp_intercepts.car[nextk] - sp_intercepts.car[jplus1]))
								{
								sp_intercepts.car[j] -= diff/2;
		    		            sp_intercepts.car[k] += diff/2;
								if (diff & 1)
									{
									if ((ufix16)(sp_intercepts.inttype[j] & FRACTION) + 
	                        	               (ufix16)(sp_intercepts.inttype[k] & FRACTION) > sp_intercepts.fracpix)
										sp_intercepts.car[j]--;
									else
										sp_intercepts.car[k]++;
									}
								}
							else
								{
						 		diff = sp_intercepts.car[nextk] - sp_intercepts.car[jplus1];
								sp_intercepts.car[nextk] -= diff/2;
		    		            sp_intercepts.car[jplus1] += diff/2;
								if (diff & 1)
									{
									if ((ufix16)(sp_intercepts.inttype[jplus1] & FRACTION) + 
	                        	               (ufix16)(sp_intercepts.inttype[nextk] & FRACTION) > sp_intercepts.fracpix)
										sp_intercepts.car[nextk]--;
									else
										sp_intercepts.car[jplus1]++;
									}
								}
							}
						break;
						}
					k = nextk;
					}
				}
			if (j > 0 && sp_intercepts.car[j-1] > sp_intercepts.car[i] && !(sp_intercepts.inttype[j-1] & END_INT))
				{
				diff = sp_intercepts.car[j-1] - sp_intercepts.car[i];
				sp_intercepts.car[i] += diff/2;
	            sp_intercepts.car[j-1] -= diff/2;
				if (diff & 1)
					{
					if ((ufix16)(sp_intercepts.inttype[i] & FRACTION) + (ufix16)(sp_intercepts.inttype[j-1] & FRACTION) > sp_intercepts.fracpix)
						sp_intercepts.car[i]++;
					else
						sp_intercepts.car[j-1]--;
					}
				}
			if (sp_intercepts.car[i+1] < sp_intercepts.car[j] && !(sp_intercepts.inttype[i] & END_INT))
				{
				diff = sp_intercepts.car[j] - sp_intercepts.car[i+1];
				sp_intercepts.car[j] -= diff/2;
	            sp_intercepts.car[i+1] += diff/2;
				if (diff & 1)
					{
					if ((ufix16)(sp_intercepts.inttype[j] & FRACTION) + (ufix16)(sp_intercepts.inttype[i+1] & FRACTION) > sp_intercepts.fracpix)
						sp_intercepts.car[i+1]++;
					else
						sp_intercepts.car[j]--;
					}
	
				}
	        }
		previ = i;   /* look at the next intercept for this scan line*/
		i = sp_intercepts.cdr[i];
	    }
	}

#if DEBUG
printf("\nPROC_INTERCEPTS_SCREEN: Intercept lists after:\n");
/* Print out all of the intercept info */
scan_line = sp_globals.ymax - first_y - 1;

for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
    {
    i = y;                            /* Index head of intercept list */
    while ((i = sp_intercepts.cdr[i]) != 0)         /* Link to next intercept if present */
        {
        if ((from = sp_intercepts.car[i] - sp_globals.xmin) < 0)
            from = 0;                 /* Clip to sp_globals.xmin boundary */
        i = sp_intercepts.cdr[i];                   /* Link to next intercept */
        if (i == 0)                   /* End of list? */
            {
            printf("****** proc_intercepts: odd number of intercepts\n");
            break;
            }
        if ((to = sp_intercepts.car[i]) > sp_globals.xmax)
            to = sp_globals.xmax - sp_globals.xmin;         /* Clip to sp_globals.xmax boundary */
        else
            to -= sp_globals.xmin;
        printf("    Y = %2d (scanline %2d): %d %d:\n", 
            y + sp_globals.y_band.band_min, scan_line, from, to);
        }
    }
#endif                                                            

/*  INTERCEPTS ALL PATCHED, NOW DRAW THE IMAGE */
scan_line = sp_globals.ymax - first_y - 1;

for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
    {
    i = sp_intercepts.cdr[y];                            /* Index head of intercept list */
    while (i  != 0)         /* Link to next intercept if present */
        {
        from = sp_intercepts.car[i];
        i = sp_intercepts.cdr[i];                   /* Link to next intercept */
        to = sp_intercepts.car[i];
#if INCL_CLIPPING
        if (clipleft)
                        {
                        if (to <= xmin)
				{
		                i = sp_intercepts.cdr[i];
                                continue;
				}
                        if (from < xmin)
                                from = xmin;
                        }
        if (clipright)
                        {
                        if (from >= xmax)
				{
		                i = sp_intercepts.cdr[i];
                                continue;
				}
                        if (to > xmax)
                                to = xmax;
                        }
#endif
        set_bitmap_bits(scan_line, from-sp_globals.xmin, to-sp_globals.xmin);
	i = sp_intercepts.cdr[i];
        }
    }
}

#endif
