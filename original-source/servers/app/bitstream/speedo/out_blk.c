 


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
*     $Header: //bliss/user/archive/rcs/speedo/out_blk.c,v 33.0 97/03/17 17:46:30 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	out_blk.c,v $
*       Revision 33.0  97/03/17  17:46:30  shawn
*       TDIS Version 6.00
*       
*       Revision 32.3  97/01/14  17:44:36  shawn
*       Apply casts to avoid compiler warnings.
*       
*       Revision 32.2  96/08/06  16:27:02  shawn
*       Cast arguments in call to open_bitmap().
*       
*       Revision 32.1  96/07/02  12:06:50  shawn
*       Changed boolean to btsbool.
*       
*       Revision 32.0  95/02/15  16:36:36  shawn
*       Release
*       
*       Revision 31.2  95/01/12  11:17:05  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:37:56  roberte
*       Release
*       
*       Revision 30.1  94/12/14  22:20:47  shawn
*       Converted to ANSI 'C'
*       Modified for support by the K&R conversion utility
*       
*       Revision 30.0  94/05/04  09:34:39  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:38:07  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:44:07  roberte
*       Release
*       
*       Revision 28.48  94/04/01  15:57:16  roberte
*       Moved some static function prototypes from speedo.h to here.
*       
*       Revision 28.47  93/08/31  09:37:47  roberte
*       Release
*       
*       Revision 28.37  93/03/15  12:46:58  roberte
*       Release
*       
*       Revision 28.10  93/01/07  12:06:01  roberte
*       Corrected comments.
*       
*       Revision 28.9  93/01/04  16:40:09  roberte
*       Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
*       
*       Revision 28.8  92/12/30  17:45:18  roberte
*       Functions no longer renamed in spdo_prv.h now declared with "sp_"
*       Use PARAMS1&2 macros throughout.
*       
*       Revision 28.7  92/12/14  15:09:50  weili
*       Modified sp_proc_intercepts_black(PARAMS1) to fix the -x bits
*       problems caused by characters below 6lines
*       
*       Revision 28.6  92/11/24  10:59:54  laurar
*       include fino.h
*       
*       Revision 28.5  92/11/19  15:17:01  roberte
*       Release
*       
*       Revision 28.1  92/06/25  13:40:42  leeann
*       Release
*       
*       Revision 27.1  92/03/23  14:00:21  leeann
*       Release
*       
*       Revision 26.1  92/01/30  16:59:19  leeann
*       Release
*       
*       Revision 25.1  91/07/10  11:05:26  leeann
*       Release
*       
*       Revision 24.1  91/07/10  10:39:00  leeann
*       Release
*       
*       Revision 23.1  91/07/09  18:00:02  leeann
*       Release
*       
*       Revision 22.3  91/06/19  14:33:32  leeann
*       move code into set_clip_parameters() in out_util
*       
*       Revision 22.2  91/05/23  16:15:47  mark
*       get formfeed off of conditional line.
*       test font0003.spd 50
*       
*       Revision 22.1  91/01/23  17:19:04  leeann
*       Release
*       
*       Revision 21.1  90/11/20  14:38:22  leeann
*       Release
*       
*       Revision 20.2  90/11/20  13:15:40  leeann
*       fixed clipping precision
*       
*       Revision 20.1  90/11/12  09:30:58  leeann
*       Release
*       
*       Revision 19.1  90/11/08  10:20:38  leeann
*       Release
*       
*       Revision 18.2  90/11/07  15:38:05  leeann
*       implement clipping for rotation of 90, 180, and 270 degrees
*       
*       Revision 18.1  90/09/24  10:10:30  mark
*       Release
*       
*       Revision 17.1  90/09/13  15:59:34  mark
*       Release name rel0913
*       
*       Revision 16.1  90/09/11  13:19:31  mark
*       Release
*       
*       Revision 15.1  90/08/29  10:04:06  mark
*       Release name rel0829
*       
*       Revision 14.4  90/08/29  09:55:03  judy
*       fix syntax error in interchar spacing
*       
*       Revision 14.3  90/08/28  17:23:29  judy
*       fix interchar spacing fix - xmode = 4 has no rounding error
*       
*       Revision 14.2  90/08/28  16:32:01  judy
*       fix inter-character spacing bug in end_char: add the round
*       error based on the xmode and ymode type to either xorg or
*       yorg.
*       
*       Revision 14.1  90/07/13  10:40:16  mark
*       Release name rel071390
*       
*       Revision 13.1  90/07/02  10:39:27  mark
*       Release name REL2070290
*       
*       Revision 12.3  90/06/26  08:58:11  leeann
*       When CLIPPED characters go into banding, save the
*       correct ymin and ymax
*       
*       Revision 12.2  90/06/06  16:40:53  judy
*       fix inter-character spacing
*       
*       Revision 12.1  90/04/23  12:12:43  mark
*       Release name REL20
*       
*       Revision 11.1  90/04/23  10:12:57  mark
*       Release name REV2
*       
*       Revision 10.7  90/04/20  09:48:00  judy
*       add the rounding error from xformation back
*       into the left edge of the character, end_char_black
*       
*       Revision 10.6  90/04/10  13:27:16  mark
*       put resetting of xmin/xmax back into end_char
*       so that collected bounding boxes will work
*       
*       Revision 10.5  90/04/04  13:19:48  mark
*       added x clipping to proc_x_intercepts
*       
*       Revision 10.4  90/03/30  14:58:39  mark
*       remove out_wht and add out_scrn and out_util
*       
*       Revision 10.3  89/09/17  22:37:44  roger
*       Fixed minor bug with revision 10.2
*       
*       Revision 10.2  89/08/29  10:52:02  roger
*       Added 10 lines of code in line() for a 2 to 4 percent speed improvement
*       
*       Revision 6.3  89/07/09  14:46:28  mark
*       make specsarg argument to init_black GLOBALFAR
*                                                     
*       
*       Revision 6.2  89/07/09  13:18:03  mark
*       >> changed arguments to sp_open_bitmap to contain
*       new high resolution positioning information
*       
*       Revision 6.1  89/06/19  08:35:25  mark
*       Release name prod
*       
*       Revision 5.6  89/06/16  16:50:08  mark
*       correct calculation of bounding box
*       
*       Revision 5.5  89/06/06  17:23:28  mark
*       add curve depth to output module curve functions
*       
*       Revision 5.4  89/06/02  17:06:30  mark
*       refer to intercept lists via symbol sp_intercepts instead
*       of sp_globals so that they may conditionally allocated off
*       stack for reentrant mode
*       
*       Revision 5.3  89/06/01  16:53:36  mark
*       changed declaration of begin_char_black to boolean,
*       return TRUE
*       
*       Revision 5.2  89/05/15  10:31:22  mark
*       fix calculation of bounding box (xmin,xmax,ymin,ymax) to round outward
*       to avoid violations in the event of interpolated bounding boxes
*       
*       Revision 5.1  89/05/01  17:54:19  mark
*       Release name Beta
*       
*       Revision 4.2  89/05/01  17:14:58  mark
*       remove unreferenced local variables
*       
*       Revision 4.1  89/04/27  12:15:57  mark
*       Release name Beta
*       
*       Revision 3.2  89/04/26  16:57:58  mark
*       when adding dropped pixels, make sure they don't violate xmax
*       
*       Revision 3.1  89/04/25  08:29:01  mark
*       Release name beta
*       
*       Revision 2.2  89/04/12  12:12:01  mark
*       added stuff for far stack and font
*       
*       Revision 2.1  89/04/04  13:35:21  mark
*       Release name EVAL
*       
*       Revision 1.7  89/04/04  13:21:37  mark
*       Update copyright text
*       
*       Revision 1.6  89/03/31  14:47:54  mark
*       change arguments to open_bitmap
*       change speedo.h to spdo_prv.h
*       eliminate thresh
*       change fontware comments to speedo
*       
*       Revision 1.5  89/03/30  17:49:13  john
*       Calculation of normal moved to set_spcs.c
*       Bitmap extents and initial band simplified to take
*       advantage of reliable bounding box data.
*       
*       Revision 1.4  89/03/29  16:09:13  mark
*       changes for slot independence and dynamic/reentrant
*       data allocation
*       
*       Revision 1.3  89/03/22  18:17:12  csdf
*       3/22/89 cdf  Added "Dumb" pixel catching in x-direction
*       
*       Revision 1.2  89/03/21  13:27:49  mark
*       change name from oemfw.h to speedo.h
*       
*       Revision 1.1  89/03/15  12:33:38  mark
*       Initial revision
*                                                                                 *
*                                                                                    *
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/out_blk.c,v 33.0 97/03/17 17:46:30 shawn Release $";
#endif




/*************************** O U T _ B L K . C *********************************
 *                                                                           *
 * This is an output module for black-writer mode.                           *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 16 Dec 88  jsc  Created                                               *
 *                                                                           *
 *  2) 23 Jan 89  jsc  normal computed from tcb.xtype, tcb.ytype now it is   *
 *                     no longer a member of tcb                             *
 *                                                                           *
 *  3)  7 Feb 89  cdf  Released as blackwriter version: calculates whole     *
 *                     pixel intercepts.  Does not catch any missing pixels  *
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

LOCAL_PROTO void sp_add_intercept_black(SPGLOBPTR2 fix15 y, fix15 x);
LOCAL_PROTO void sp_proc_intercepts_black(SPGLOBPTR1);



#if INCL_BLACK

FUNCTION btsbool sp_init_black(SPGLOBPTR2 specs_t GLOBALFAR *specsarg)
/*
 * init_out0() is called by sp_set_specs(PARAMS1) to initialize the output module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
{

#if DEBUG
printf("INIT_BLK()\n");
#endif

if (specsarg->flags & CURVES_OUT)
    return FALSE;           /* Curves out not supported */
return (TRUE);
}
#endif



#if INCL_BLACK

FUNCTION btsbool sp_begin_char_black(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/* Called once at the start of the character generation process */
{

#if DEBUG
printf("BEGIN_CHAR_BLACK(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n", 
                    (real)Psw.x / (real)sp_globals.onepix, (real)Psw.y / (real)sp_globals.onepix,
                    (real)Pmin.x / (real)sp_globals.onepix, (real)Pmin.y / (real)sp_globals.onepix,
                    (real)Pmax.x / (real)sp_globals.onepix, (real)Pmax.y / (real)sp_globals.onepix);
#endif

sp_init_char_out(PARAMS2 Psw,Pmin,Pmax);
return TRUE;
}
#endif


#if INCL_BLACK

FUNCTION void sp_begin_contour_black(SPGLOBPTR2 point_t P1, btsbool outside)
/* Called at the start of each contour */
{

#if DEBUG
printf("BEGIN_CONTOUR_BLACK(%3.1f, %3.1f, %s)\n", 
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix, outside? "outside": "inside");
#endif

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
sp_globals.y_pxl = (sp_globals.y0_spxl + sp_globals.pixrnd) >> sp_globals.pixshift;
}
#endif


#if INCL_BLACK

FUNCTION void sp_line_black(SPGLOBPTR2 point_t P1)
/* Called for each vector in the transformed character */
{
register fix15     how_many_y;       /* # of intercepts at y = n + 1/2  */
register fix15     yc, i;            /* Current scan-line */
         fix15     temp1;            /* various uses */
         fix15     temp2;            /* various uses */
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
printf("LINE_BLACK(%3.4f, %3.4f)\n", 
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

if (how_many_y < 0) yc--; /* Predecrment downward lines */

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

xc = (fix31)(x0 + sp_globals.pixrnd) << (16 - sp_globals.pixshift); /* Original x coordinate with built in  */
                                            /* rounding. 16.16 form */


if ( (temp1 = (x1 - x0)) == 0)  /* check for vertical line */
    {
    yc -= sp_globals.y_band.band_min; /* yc is now an offset relative to the band */
    temp1 = (fix15)(xc >> 16); 
    if (how_many_y < 0)
        {   /* Vector down */
        if ((how_many_y += yc + 1) < 0) how_many_y = 0; /* can't go below 0 */
        for (i = yc; i >= how_many_y; i--)
            sp_add_intercept_black(PARAMS2 i,temp1); 
        }
    else
        {   /* Vector up */
     /* check to see that line doesn't extend beyond top of band */
        if ((fix31)(how_many_y += yc) > sp_globals.no_y_lists)
            how_many_y = (fix15)sp_globals.no_y_lists;
        for (i = yc; i != how_many_y; i++)
            sp_add_intercept_black(PARAMS2 i,temp1); 
        }
    return;
    }
          
/* calculate dx_dy at 16.16 fixed point */

dx_dy = ( (fix31)temp1 << 16 )/(fix31)(y1 - y0);

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
      (temp2 != 0x0000) &&
      /* Overflow. Pick point closest to yc + .5 */
    (ABS(temp1) < ABS((yc << sp_globals.pixshift) - y1 + sp_globals.pixrnd)) )
    { /* use x1 instead of x0 */
    xc = (fix31)(x1 + sp_globals.pixrnd) << (16 - sp_globals.pixshift);
    }
else
    {
/* calculate new xc at the center of the *current* scan line */
/* due to banding, yc may be several lines away from y0      */
/*  xc += (yc + .5 - y0) * dx_dy */
/* This multiply generates a subpixel delta. */
/* So we shift it to be a 16.16 delta */

    xc += ((fix31)temp1 * dx_dy) >> sp_globals.pixshift;
    }

yc -= sp_globals.y_band.band_min; /* yc is now an offset relative to the band */

if (how_many_y < 0)
    {   /* Vector down */
    if (how_many_y == -1)
        sp_add_intercept_black(PARAMS2 yc, (fix15) (xc >> 16));
    else
        {
        if ((how_many_y += yc + 1) < 0) how_many_y = 0; /* can't go below 0 */
        for (i = yc; i >= how_many_y; i--)
            {
            temp1 = (fix15)(xc >> 16); 
            sp_add_intercept_black(PARAMS2 i,temp1); 
            xc -= dx_dy;
            }
        }
    }
    else
    {   /* Vector up */
     /* check to see that line doesn't extend beyond top of band */
    if (how_many_y == 1)
        sp_add_intercept_black(PARAMS2 yc, (fix15) (xc >> 16));
    else
        {
        if ((fix31)(how_many_y += yc) > sp_globals.no_y_lists)
            how_many_y = (fix15)sp_globals.no_y_lists;
        for (i = yc; i != how_many_y; i++)
            {
            temp1 = (fix15)(xc >> 16);
            sp_add_intercept_black(PARAMS2 i,temp1); 
            xc += dx_dy;
            }
        }
    }
}
#endif



#if INCL_BLACK

FUNCTION btsbool sp_end_char_black(SPGLOBPTR1)
/* Called when all character data has been output
 * Return TRUE if output process is complete
 * Return FALSE to repeat output of the transformed data beginning
 * with the first contour
 */
{

fix31 xorg;
fix31 yorg;
#if INCL_CLIPPING
fix31 bmap_max, bmap_min;
#endif

#if DEBUG
printf("END_CHAR_BLACK()\n");
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
                (fix15)(sp_globals.ymax - sp_globals.ymin));

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
        sp_proc_intercepts_black(PARAMS1);
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
        sp_proc_intercepts_black(PARAMS1);
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


#if INCL_BLACK

FUNCTION LOCAL void sp_add_intercept_black(SPGLOBPTR2 fix15 y, fix15 x)
/*  Called by line() to add an intercept to the intercept list structure */
{
register fix15 from;   /* Insertion pointers for the linked list sort */
register fix15 to;

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

sp_intercepts.car[sp_globals.next_offset] = x;

/* Find slot to insert new element (between from and to) */

from = y; /* Start at list head */

while( (to = sp_intercepts.cdr[from]) >= sp_globals.first_offset) /* Until to == end of list */
    {
    if (x <= sp_intercepts.car[to]) /* If next item is larger than or same as this one... */
        goto insert_element; /* ... drop out and insert here */
    from = to; /* move forward in list */
    }

insert_element: /* insert element "sp_globals.next_offset" between elements "from" */
                /* and "to" */

sp_intercepts.cdr[from] = (cdr_t)sp_globals.next_offset;
sp_intercepts.cdr[sp_globals.next_offset] = (cdr_t)to;

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


#if INCL_BLACK

FUNCTION LOCAL void sp_proc_intercepts_black(SPGLOBPTR1)
/*  Called by sp_make_char to output accumulated intercept lists
 *  Clips output to sp_globals.xmin, sp_globals.xmax, sp_globals.ymin, sp_globals.ymax boundaries
 */
{
register fix15 i;
register fix15 from, to;          /* Start and end of run in pixel units   
                            relative to left extent of character  */
register fix15 y;
register fix15 scan_line;
         fix15 first_y, last_y;

#if INCL_CLIPPING
fix15 xmin, xmax;
btsbool clipleft, clipright;
#endif

/* fixed the -x bits problems cause by characters below 6 lines */
if ( sp_globals.xmax <= sp_globals.xmin )
    return;

#if DEBUG
printf("\nIntercept lists:\n");
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
	xmax = sp_globals.clip_xmax;
	xmin = sp_globals.clip_xmin;
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

/* Draw the image */
scan_line = sp_globals.ymax - first_y - 1;

for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
    {
    i = y;                            /* Index head of intercept list */
    while ((i = sp_intercepts.cdr[i]) != 0)         /* Link to next intercept if present */
        {
        if ((from = sp_intercepts.car[i] - sp_globals.xmin) < 0)
            from = 0;                 /* Clip to sp_globals.xmin boundary */
        i = sp_intercepts.cdr[i];                   /* Link to next intercept */

        if ((to = sp_intercepts.car[i]) > sp_globals.xmax)
            to = sp_globals.xmax - sp_globals.xmin;         /* Clip to sp_globals.xmax boundary */
        else
            to -= sp_globals.xmin;
        if (from >= to)
			{
			if (from >= sp_globals.xmax - sp_globals.xmin)
				{
				--from ;
				}
			to = from+1;
			}
#if INCL_CLIPPING
		if (clipleft)
			{
			if (to <= xmin)
				continue;
			if (from < xmin)
				from = xmin;
			}
	if (clipright)
			{
			if (from >= xmax)
				continue;
			if (to > xmax)
				to = xmax;
			}
#endif
        set_bitmap_bits(scan_line, from, to);
        }
    }
}

#endif
