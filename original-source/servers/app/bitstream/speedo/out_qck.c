
/*****************************************************************************
*                                                                            *
*              Copyright 1989-95, as an unpublished work by                  *
*                      Bitstream Inc., Cambridge, MA                         *
*                   U.S. Patent Nos 4,785,391 5,099,435                      *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/

/************************** O U T _ Q C K . C ********************************
*                                                                           *
* QuickWriter is a high-performance, high-quality output module for 4-in-1. *
*                                                                           *
********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *     $Header: //bliss/user/archive/rcs/speedo/out_qck.c,v 33.0 97/03/17 17:46:47 shawn Release $                                                                    *
 *                                                                                    *
 *     $Log:	out_qck.c,v $
 * Revision 33.0  97/03/17  17:46:47  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.5  96/08/06  16:30:08  shawn
 * Cast arguments in call to open_bitmap().
 * 
 * Revision 32.4  96/07/02  12:10:54  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 32.3  95/09/06  17:20:16  john
 * Test for clipped salients is now done for vectors that
 *     are entirely clipped as well as vectors that start
 *     in the clipped region and end in the active band.
 *     This avoids spurious salients in the top scanline
 *     of second and subsequent bands.
 * 
 * Revision 32.2  95/03/15  17:19:49  john
 *  Added some casts to eliminate compiler warnings.
 * Updated calculation of dxdy in line_to to handle vectors
 *     longer than 32K sub-pixels correctly.
 * Corrected intercept sorting problem with >= 3 elements.
 * 
 * Revision 32.1  95/03/07  13:58:54  mark
 * when sorting intercept list, search list until absolute beginning, 
 * ignore first_sub_char_offset
 * 
 * Revision 32.0  95/02/15  16:36:57  shawn
 * Release
 * 
 * Revision 31.4  95/02/03  10:49:27  roberte
 * Changed optimization gate for accruing bitmap extrema. 
 * Was failing when the height or width collapsed to 0.
 * Now test in sp_line_quick() for dx <= 0 and dy <= 0.
 * 
 * Revision 31.3  95/01/12  12:28:39  mark
 * in init_intercepts, set intercept_oflo to default value of false, the
 * override if no_y_lists excedes max_intercepts.  Previously, this setting
 * was getting made then cleared, confusing the quickwriter at certain sizes.
 * 
 * Revision 31.2  95/01/12  11:28:57  roberte
 * Updated Copyright header.
 * 
 * Revision 31.1  95/01/04  16:38:40  roberte
 * Release
 * 
 * Revision 30.3  94/12/14  21:57:54  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 30.2  94/10/12  13:27:24  mark
 * fix typo
 * 
 * Revision 30.1  94/10/12  13:25:46  mark
 * change y_band calculations to be the same as other output modules.
 * 
 * Revision 30.0  94/05/04  09:35:29  roberte
 * Release
 * 
 * Revision 29.2  94/04/08  14:38:34  roberte
 * Release
 * 
 * Revision 29.0  94/04/08  11:44:26  roberte
 * Release
 * 
 * Revision 28.51  94/02/14  14:28:06  roberte
 * Fixed overflow of dx and dy locals in sp_line_quick()
 * at certain sizes.
 * 
 * Revision 28.50  94/02/03  12:52:42  roberte
 * Corrected a couple of #if MSDOS to #ifdef MSDOS. 
 * #define the INLINE macro only if distinctly a Microsoft Compiler.
 * 
 * Revision 28.49  94/02/02  14:26:38  roberte
 * Unfixed pn_open_intercepts increment.
 * 
 * Revision 28.48  94/01/26  17:20:10  roberte
 * Corrected intended decrement of *pn_open_intercepts in
 * sp_assemble_group_quick() which caused infinite loops
 * on some rare characters. Thanks Doug.
 * 
 * Revision 28.47  93/08/31  09:38:08  roberte
 * Release
 * 
 * Revision 1.8  93/08/19  16:35:38  roberte
 * Changed the limits on test of left_possible and right_possible to
 * allow the writer to fill more discontinuities on small characters.
 * 
 * Revision 1.7  93/07/26  16:47:04  ruey
 * take out macro recursion
 * 
 * Revision 1.6  93/07/08  15:11:18  ruey
 * set Quickwriter as an option of output modules
 * 
 * Revision 1.5  93/07/07  17:18:15  ruey
 * put static variables into speedo globals
 * 
 * Revision 1.4  93/07/02  16:03:21  ruey
 * substitude sp_props with sp_intercepts.inttype
 * 
 * Revision 1.3  93/07/01  10:19:20  ruey
 * fix RCS log file
 * 
 * Revision 1.2  93/06/30  12:35:18  ruey
 * Add PROTO, PARAMS1, PARAMS2
 *       
 * Revision 1.1  93/06/27  14:28:17  ruey
 * 11 May 93  jsc  Created                                             *
 *                                                                           *
 *****************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/out_qck.c,v 33.0 97/03/17 17:46:47 shawn Release $";
#endif

#include "spdo_prv.h"           /* General definitions for Speedo   */

/* Set DEBUG = 1 for internal consistency checking only */
/* Set DEBUG = 2 for consistency checking and full trace */
#define	DEBUG	0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", (int)X)
#else
#define SHOW(X)
#endif

#if (defined (M_I86CM) || defined (M_I86LM) || defined (M_I86HM) || defined (M_I86SM) || defined (M_I86MM))
/* then we are using a Microsoft compiler */
#define INLINE __inline
#else
#define INLINE
#endif

#define MAX_INTERCEPTS_QUICK MAX_INTERCEPTS

#if INCL_QUICK

/* New type definition for intercept properties */
typedef ufix8 props_t;

/* Local function declarations */
LOCAL_PROTO void sp_init_intercepts_quick(SPGLOBPTR1);
LOCAL_PROTO void sp_add_intercept_quick(SPGLOBPTR2 fix15 y, fix15 x, props_t props_value);
LOCAL_PROTO void sp_mark_salient_quick(SPGLOBPTR2 fix15 y, fix15 i1, fix15 i2);
LOCAL_PROTO void sp_proc_intercepts_quick(SPGLOBPTR1);
LOCAL_PROTO void sp_flush_intercepts_quick(SPGLOBPTR1);
LOCAL_PROTO void sp_close_horiz_gaps_quick(SPGLOBPTR2 fix15 y);
LOCAL_PROTO void sp_close_diag_gaps_quick(SPGLOBPTR2 fix15 y);
LOCAL_PROTO btsbool sp_assemble_group_pair_quick(SPGLOBPTR2
    fix15 i1, 
    fix15 i2, 
    fix15 *pi1_start, 
    fix15 *pi1_end, 
    fix15 *pi2_start, 
    fix15 *pi2_end);
LOCAL_PROTO void sp_assemble_group_quick(SPGLOBPTR2
    fix15 i, 
    props_t up_salient_mask, 
    props_t down_salient_mask, 
    fix15 min_open_intercepts,
    fix15 *pi_start,
    fix15 *pi_end,
    fix15 *pn_open_intercepts);
LOCAL_PROTO void sp_proc_group_pair_quick(SPGLOBPTR2
    fix15 i1_start, 
    fix15 i1_end, 
    fix15 i2_start, 
    fix15 i2_end);
LOCAL_PROTO void sp_close_gap_quick(SPGLOBPTR2
    fix15 i1_start, 
    fix15 i1_end, 
    fix15 i2_start, 
    fix15 i2_end );
LOCAL_PROTO void sp_sort_intercepts_quick(SPGLOBPTR2 fix15 y);
LOCAL_PROTO void sp_output_complex_scanline_quick(SPGLOBPTR2 fix15 y, fix15 scanline);
LOCAL_PROTO btsbool sp_extend_zero_length_run_quick(SPGLOBPTR2
    fix15 avail_pix_left, 
    fix15 extendibility_left,
    fix15 avail_pix_right, 
    fix15 extendibility_right,
    fix15 *px_start, 
    fix15 *px_end);
LOCAL_PROTO void sp_check_scanline_quick(SPGLOBPTR2 fix15 y);

#if DEBUG >= 1
LOCAL_PROTO void print_intercepts(SPGLOBPTR2 fix15 y);
#endif

/* Intercept list property definitions */
#define SUB_CHAR_COUNT           0x03   /* MUST be at ls end of word */
#define COMPOUND_SCANLINE_FLAG   0x04
#define COMPLEX_SCANLINE_FLAG    0x08
#define COMPLEX_INTERVAL_BELOW   0x10
#define SOLID_SALIENT_FLAG       0x20
#define HOLLOW_SALIENT_FLAG      0x40

/* Individual intercept property definitions */
#define EXTENDIBILITY            0x03   /* MUST be at ls end of word */
#define BEGIN_SOLID_SALIENT      0x04
#define END_SOLID_SALIENT        0x08
#define BEGIN_HOLLOW_SALIENT     0x10
#define END_HOLLOW_SALIENT       0x20
#define UP_INTERCEPT             0x80

FUNCTION 
btsbool sp_init_quick(SPGLOBPTR2 specs_t GLOBALFAR *specsarg)
/*
 * init_out_quick() is called by sp_set_specs() to initialize the output 
 * module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
{

#if DEBUG >= 2
printf("INIT_QUICK()\n");
printf("    Mode flags = %8.0x\n", specsarg->flags);
#endif

if (specsarg->flags & 
    (CURVES_OUT |               /* Curves not supported */
     CLIP_LEFT |                /* Clipping not supported */
     CLIP_RIGHT |
     CLIP_TOP |
     CLIP_BOTTOM))
    {
    return FALSE;
    }

return TRUE;
}

FUNCTION 
btsbool sp_begin_char_quick(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/*
 *  Called once at the start of the character generation process
 */
{

#if DEBUG >= 2
printf("BEGIN_CHAR_QUICK(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n", 
    (real)Psw.x / (real)sp_globals.onepix, (real)Psw.y / (real)sp_globals.onepix,
    (real)Pmin.x / (real)sp_globals.onepix, (real)Pmin.y / (real)sp_globals.onepix,
    (real)Pmax.x / (real)sp_globals.onepix, (real)Pmax.y / (real)sp_globals.onepix);
#endif

/* Save escapement vector for later use in open_bitmap() */
sp_globals.set_width.x = (fix31)Psw.x << sp_globals.poshift;
sp_globals.set_width.y = (fix31)Psw.y << sp_globals.poshift;

/* Set initial band to cover all scanlines touched or intercepted by character */
sp_globals.y_band.band_min = (Pmin.y - sp_globals.onepix + 1) >> sp_globals.pixshift;
sp_globals.y_band.band_max = (Pmax.y + sp_globals.onepix - 1) >> sp_globals.pixshift;

/* Initialize intercept storage */
sp_init_intercepts_quick(PARAMS1);

/* Initialize bitmap bounding box */
if (sp_globals.normal)          /* Bitmap bounding box already known accurately? */
    {
    sp_globals.bmap_xmin = Pmin.x;
    sp_globals.bmap_xmax = Pmax.x;
    sp_globals.bmap_ymin = Pmin.y;
    sp_globals.bmap_ymax = Pmax.y;
    sp_globals.extents_running = FALSE;
    }
else                            /* Bitmap bounding box not known accurately? */
    {
    sp_globals.bmap_xmin = 32000;
    sp_globals.bmap_xmax = -32000;
    sp_globals.bmap_ymin = 32000;
    sp_globals.bmap_ymax = -32000;
    sp_globals.extents_running = TRUE;
    }

sp_globals.first_pass = TRUE;

return TRUE;
}

FUNCTION 
void sp_begin_sub_char_quick(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/*
 * Called at the start of each sub-character in a composite character
 */
{

#if DEBUG >= 2
printf("BEGIN_SUB_CHAR_QUICK(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n", 
    (real)Psw.x / (real)sp_globals.onepix, (real)Psw.y / (real)sp_globals.onepix,
    (real)Pmin.x / (real)sp_globals.onepix, (real)Pmin.y / (real)sp_globals.onepix,
    (real)Pmax.x / (real)sp_globals.onepix, (real)Pmax.y / (real)sp_globals.onepix);
#endif

sp_globals.sp_first_sub_char_offset = sp_globals.next_offset;

/* Set safe band of scanlines touched or intersected by sub-character
 * We need to allow a 5 pixel boundary in case inner features break
 * through the character bounding box 
 */
sp_globals.sp_sub_band_min = ((Pmin.y + sp_globals.pixrnd) >> sp_globals.pixshift) - 5;
sp_globals.sp_sub_band_max = ((Pmax.y + sp_globals.pixrnd) >> sp_globals.pixshift) + 4; 

/* Ensure actual bitmap extents are collected for compound characters */
if (sp_globals.first_pass &&
    (!sp_globals.extents_running))
	{
    sp_globals.bmap_xmin = 32000;
    sp_globals.bmap_xmax = -32000;
    sp_globals.bmap_ymin = 32000;
    sp_globals.bmap_ymax = -32000;
    sp_globals.extents_running = TRUE;
	}
}

FUNCTION 
void sp_begin_contour_quick(SPGLOBPTR2 point_t P1, btsbool outside)
/*
 * Called at the start of each contour
 */
{
#if DEBUG >= 2
printf("BEGIN_CONTOUR_QUICK(%3.1f, %3.1f, %s)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix, 
    outside? "outside": "inside");
#endif

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;
sp_globals.y_pxl = (sp_globals.y0_spxl + sp_globals.pixrnd) >> sp_globals.pixshift;
sp_globals.sp_banding_active = TRUE;
sp_globals.sp_first_contour_offset = sp_globals.next_offset;

sp_globals.sp_old_yend = -32000;

sp_globals.sp_clipped_first_salient = FALSE;
sp_globals.sp_clipped_salient = FALSE;

/* Check space left in intercept structure */
sp_globals.sp_intercept_monitoring =
    (sp_globals.y_band.band_max - sp_globals.y_band.band_min) >= 
    (MAX_INTERCEPTS_QUICK - sp_globals.next_offset);
}

FUNCTION 
void sp_line_quick(SPGLOBPTR2 point_t P1)
/*
 * Called for each vector in the transformed character
 * Determines which scanlines that fall within the current band are
 * intercepted and calls sp_add_intercept_quick to record each intercept in
 * the intercept structure.
 */
{
fix15   xstart_spxl;    /* X coord of start point (sub-pixel units) */
fix15   ystart_spxl;    /* Y coord of start point (sub-pixel units) */
fix31   dx;             /* X increment (sub-pixel units) */
fix31   dy;             /* Y increment (sub-pixel units) */
fix15   ystart;         /* First scanline to be intercepted */
fix15   yend;           /* Last scanline to be intercepted */
fix15   space_left;     /* Space left in the intercept structure */
fix15   x;              /* X coord of each intercept (pixel units) */
fix15   y;              /* Y coord of each intercept (i'cept list index) */
fix31   xx;             /* X coord of each intercept (16.16 units) */
fix31   dxdy;           /* Delta X for each scanline (16.16 units) */
fix31   tmpfix31;
btsbool salient_detected;
btsbool banding_active_next;
fix15   tmpfix15;

#if DEBUG >= 2
printf("LINE_QUICK(%3.4f, %3.4f)\n", 
   (real)P1.x/(real)sp_globals.onepix, 
   (real)P1.y/(real)sp_globals.onepix);
#endif

xstart_spxl = sp_globals.x0_spxl;
dx = (fix31)P1.x - sp_globals.x0_spxl;
sp_globals.x0_spxl = P1.x;

ystart_spxl = sp_globals.y0_spxl;
dy = (fix31)P1.y - sp_globals.y0_spxl;
sp_globals.y0_spxl = P1.y;

/* Accumulate actual character bounding box if not already known */
if (sp_globals.extents_running)
    {
    if (dx >= 0)
        {
        if (sp_globals.x0_spxl > sp_globals.bmap_xmax)         
            {
            sp_globals.bmap_xmax = sp_globals.x0_spxl;
            }
        }
    if (dx <= 0)
        {
        if (sp_globals.x0_spxl < sp_globals.bmap_xmin)
            {
            sp_globals.bmap_xmin = sp_globals.x0_spxl;
            }
        }

    if (dy >= 0)
        {
        if (sp_globals.y0_spxl > sp_globals.bmap_ymax)
            {
            sp_globals.bmap_ymax = sp_globals.y0_spxl;
            }
        }
    if (dy <= 0)
        {
        if (sp_globals.y0_spxl < sp_globals.bmap_ymin)
            {
            sp_globals.bmap_ymin = sp_globals.y0_spxl;
            }
        }
    }

/* Compute first and last scanlines intercepted by vector */
ystart = sp_globals.y_pxl;
sp_globals.y_pxl = yend = (P1.y + sp_globals.pixrnd) >> sp_globals.pixshift;   
if (yend < ystart)
    {
    ystart--;
    }
else if (yend > ystart)
    {
    yend--;
    }
else
    {
    return;
    }

/* Check if intercept overflow has already occured */
if (sp_globals.intercept_oflo)
    {
    return;
    }

/* Check for change in Y direction */
if (salient_detected = (ystart == sp_globals.sp_old_yend))
    {
    sp_globals.sp_intercept_monitoring =
        (sp_globals.y_band.band_max - sp_globals.y_band.band_min) >= 
        (MAX_INTERCEPTS_QUICK - sp_globals.next_offset);
    }
sp_globals.sp_old_yend = yend;

/* Check if last scanline intercepted is outside current band */
banding_active_next =
    (yend > sp_globals.y_band.band_max) ||
    (yend < sp_globals.y_band.band_min);

/* Clip vector to current band */
if (banding_active_next || sp_globals.sp_banding_active)
    {
    if (sp_globals.y_band.band_min > sp_globals.y_band.band_max) /* Band is null? */
        {
        sp_globals.sp_banding_active = TRUE;
        return;
        }

    if (ystart > sp_globals.y_band.band_max)
        {
        if (salient_detected)
            {
            salient_detected = FALSE;
            if (sp_globals.next_offset == sp_globals.sp_first_contour_offset)
                {
                sp_globals.sp_clipped_first_salient = TRUE;
                }
            else
                {
                sp_globals.sp_clipped_salient = TRUE;
                }
            }
        if (yend > sp_globals.y_band.band_max)
            {
            sp_globals.sp_banding_active = TRUE;
            return;
            }
        ystart = sp_globals.y_band.band_max;
        }

    if (ystart < sp_globals.y_band.band_min)
        {
        if (salient_detected)
            {
            salient_detected = FALSE;
            if (sp_globals.next_offset == sp_globals.sp_first_contour_offset)
                {
                sp_globals.sp_clipped_first_salient = TRUE;
                }
            else
                {
                sp_globals.sp_clipped_salient = TRUE;
                }
            }
        if (yend < sp_globals.y_band.band_min)
            {
            sp_globals.sp_banding_active = TRUE;
            return;
            }
        ystart = sp_globals.y_band.band_min;
        }

    if (yend > sp_globals.y_band.band_max)
        {
        yend = sp_globals.y_band.band_max;
        }

    if (yend < sp_globals.y_band.band_min)
        {
        yend = sp_globals.y_band.band_min;
        }
    
    if (!banding_active_next)
        {
        sp_globals.sp_clipped_salient = FALSE;
        }
    }
sp_globals.sp_banding_active = banding_active_next;

if (sp_globals.sp_intercept_monitoring)
    {
    space_left = MAX_INTERCEPTS_QUICK - sp_globals.next_offset;
    tmpfix15 = yend - ystart;
    if ((tmpfix15 >= space_left) ||
        (-tmpfix15 >= space_left))
        {
        sp_globals.intercept_oflo = TRUE;
#if DEBUG >= 2
        printf("    Intercept overflow detected\n");
#endif
        return;
        }
    }

/* Make Y values relative to start of current band */
y = ystart = ystart - sp_globals.y_band.band_min;
yend -= sp_globals.y_band.band_min;

if (dx == 0)                    /* Vertical vector? */
    {
    x = (xstart_spxl + sp_globals.pixrnd) >> sp_globals.pixshift;
    if (dy > 0)                 /* Direction is up? */
        {
        do
            {
            sp_add_intercept_quick(PARAMS2 y, x, (props_t)UP_INTERCEPT);
            }
        while (++y <= yend);
        }
    else                        /* Direction is down? */
    	{
        do
            {
            sp_add_intercept_quick(PARAMS2 y, x, (props_t)0);
            }
        while (--y >= yend);
        }
	}
else                            /* Neither horizontal nor vertical */
    {
    /* Compute slope as signed 16.16 value */
    dxdy = ((ufix32)(dx >= 0? dx: -dx) << 16) / (ufix32)(dy >= 0? dy: -dy);
    if ((dx >= 0) ^ (dy >= 0))
        {
        dxdy = -dxdy;
        }

#ifdef  MSDOS
    tmpfix31 = (fix31)(xstart_spxl + sp_globals.pixrnd) << (16 - sp_globals.pixshift);
    xx = (fix31)(((ystart + sp_globals.y_band.band_min) << sp_globals.pixshift) - ystart_spxl + sp_globals.pixrnd);
    xx = ((xx * dxdy) >> sp_globals.pixshift) + tmpfix31;
#else
    xx  = 
        ((fix31)(xstart_spxl + sp_globals.pixrnd) << (16 - sp_globals.pixshift)) +
        (((((ystart + sp_globals.y_band.band_min) << sp_globals.pixshift) - ystart_spxl + sp_globals.pixrnd) 
            * dxdy) >> sp_globals.pixshift);
#endif

    if (dy > 0)                 /* Upwards direction? */
        {
		while (TRUE)
			{
            sp_add_intercept_quick(PARAMS2 y, (fix15)(xx >> 16), (props_t)UP_INTERCEPT);
            if (++y > yend)
            	break;
            xx += dxdy;
            }
        }
    else                        /* Downwards direction? */
        {
		while (TRUE)
			{
            sp_add_intercept_quick(PARAMS2 y, (fix15)(xx >> 16), (props_t)0);
            if (--y < yend)
            	break;
            xx -= dxdy;
            }
        }
    }

if (salient_detected)           /* Salient at start point? */
    {
    y = yend - ystart;          /* Number of intercepts - 1 */
    if (y < 0)
        {
        y = -y;
        }
    y = sp_globals.next_offset - y - 1; /* End salient intercept index */
    sp_mark_salient_quick(PARAMS2 ystart, y - 1, y);
    }
}


FUNCTION
void sp_curve_quick(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
/*
 * This is a dummy function that is never called 
 */
{
}


FUNCTION 
void sp_end_contour_quick(SPGLOBPTR1)
/* 
 * Called after the last vector in each contour
 * Tests for the presence of a salient between the first and last
 * intercepts of the contour.
 */
{

#if DEBUG >= 2
printf("END_CONTOUR_QUICK()\n");
#endif

if ((sp_globals.next_offset - sp_globals.sp_first_contour_offset >= 2) &&
    !sp_globals.sp_clipped_first_salient &&
    !sp_globals.sp_clipped_salient &&
    (sp_globals.sp_old_yend >= sp_globals.y_band.band_min) &&
    (sp_globals.sp_old_yend <= sp_globals.y_band.band_max))
    {
    if ((sp_intercepts.inttype[sp_globals.next_offset - 1] & UP_INTERCEPT) &&
        ((sp_intercepts.inttype[sp_globals.sp_first_contour_offset] & UP_INTERCEPT) == 0) &&
        (sp_globals.y_pxl - 1 == sp_globals.sp_old_yend))
        {
        sp_mark_salient_quick(PARAMS2 
            sp_globals.sp_old_yend - sp_globals.y_band.band_min, 
            sp_globals.next_offset - 1, 
            sp_globals.sp_first_contour_offset);
        }
    else if
        (((sp_intercepts.inttype[sp_globals.next_offset - 1] & UP_INTERCEPT) == 0) &&
        (sp_intercepts.inttype[sp_globals.sp_first_contour_offset] & UP_INTERCEPT) &&
        (sp_globals.y_pxl == sp_globals.sp_old_yend))
        {
        sp_mark_salient_quick(PARAMS2 
            sp_globals.sp_old_yend - sp_globals.y_band.band_min, 
            sp_globals.next_offset - 1, 
            sp_globals.sp_first_contour_offset);
        }
    }
}


FUNCTION 
void sp_end_sub_char_quick(SPGLOBPTR1)
/*
 * Called after the last contour in each sub-character in a compound character.
 * Marks scanlines with runs from 2 or more sub-characters compound.
 * Marks scanline intervals adjacent to compound scanlines complex.
 */
{
fix15   first_y;
fix15   last_y;
fix15   y;

#if DEBUG >= 2
printf("END_SUB_CHAR_QUICK()\n");
#endif

if (sp_globals.next_offset != sp_globals.sp_first_sub_char_offset) /* Non-blank sub-char? */
    {
    /* Determine range of Y values affected by sub-character */
    first_y = (sp_globals.y_band.band_max < sp_globals.sp_sub_band_max)?
        sp_globals.y_band.band_max:
        sp_globals.sp_sub_band_max;

    last_y = (sp_globals.y_band.band_min > sp_globals.sp_sub_band_min)?
        sp_globals.y_band.band_min:
        sp_globals.sp_sub_band_min;

    /* Adjust Y values to be relative to bottom of current Y band */
    first_y -= sp_globals.y_band.band_min;
    last_y -= sp_globals.y_band.band_min;

    for (y = first_y; y >= last_y; y--)
        {
        if (sp_intercepts.cdr[y] >= sp_globals.sp_first_sub_char_offset) /* New intercepts added? */
            {
            switch(sp_intercepts.inttype[y] & SUB_CHAR_COUNT) /* How many sub_chars? */
                {
            case 0:             /* First sub-char at this y value? */
                sp_intercepts.inttype[y]++;  /* Increment sub_char count */
                break;

            case 1:             /* Second sub_char at this y value? */
                sp_intercepts.inttype[y]++;  /* Increment sub-char count */
                sp_intercepts.inttype[y] |= 
                    COMPLEX_INTERVAL_BELOW +
                    COMPOUND_SCANLINE_FLAG;
                if ((y + 1) < (fix15)sp_globals.no_y_lists)
                    {
                    sp_intercepts.inttype[y + 1] |= COMPLEX_INTERVAL_BELOW;
                    }
                break;

            default:            /* Third and subsequent sub-chars? */
                break;
                }
            }
        }    
    }
}


FUNCTION 
btsbool sp_end_char_quick(SPGLOBPTR1)
/* Called when all character data has been output
 * Return TRUE if output process is complete
 * Return FALSE to repeat output of the transformed data beginning
 * with the first contour
 * Calls open_bitmap() to inititiate the bitmap output process.
 * Calls sp_proc_intercepts_quick() to output each band of intercepts
 * Calls sp_flush_intercepts_quick() to output the last scanline
 * Calls close_bitmap() to terminate the bitmap output process
 */
{
fix31 xorg;
fix31 yorg;
fix15 tmpfix15;

#if DEBUG >= 2
printf("END_CHAR_QUICK()\n");
printf("Transformed character bounding box is %3.1f, %3.1f, %3.1f, %3.1f\n", 
    (real)sp_globals.bmap_xmin / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_ymin / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_xmax / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_ymax / (real)sp_globals.onepix);
#endif

if (sp_globals.first_pass)
    {
    if (sp_globals.bmap_xmax >= sp_globals.bmap_xmin) /* Non-blank character? */
        {
        sp_globals.xmin = (sp_globals.bmap_xmin + sp_globals.pixrnd - 1) >> sp_globals.pixshift;
        sp_globals.ymin = (sp_globals.bmap_ymin + sp_globals.pixrnd - 1) >> sp_globals.pixshift;
        sp_globals.xmax = (sp_globals.bmap_xmax + sp_globals.pixrnd) >> sp_globals.pixshift;
        sp_globals.ymax = (sp_globals.bmap_ymax + sp_globals.pixrnd) >> sp_globals.pixshift;
        }
    else                        /* Blank character? */
        {
        sp_globals.xmin = sp_globals.xmax = 0;
        sp_globals.ymin = sp_globals.ymax = 0;
        }

    /* Origin is at bottom left of bitmap bounding box */
    xorg = (fix31)sp_globals.xmin << 16;
   	yorg = (fix31)sp_globals.ymin << 16;

    /* Restore fractional components of origin */
    if (sp_globals.tcb.xmode == 0)          /* X pix is function of X orus only? */
        {
    	xorg += (fix31)sp_globals.rnd_xmin << sp_globals.poshift; /* Add rounding error */

        }
    else if (sp_globals.tcb.xmode == 1)     /* X pix is function of -X orus only? */
        {
      	xorg -= (fix31)sp_globals.rnd_xmin << sp_globals.poshift; /* Subtract rounding error */
        }

    if (sp_globals.tcb.ymode == 2)          /* Y pix is function of X orus only? */
        {
    	yorg += (fix31)sp_globals.rnd_xmin << sp_globals.poshift; /* Add rounding error */
        }
    else if (sp_globals.tcb.ymode == 3)     /* Y pix is function of -X orus only? */
        {
      	yorg -= (fix31)sp_globals.rnd_xmin << sp_globals.poshift; /* Subtract rounding error */
        }

    open_bitmap(
        sp_globals.set_width.x, 
        sp_globals.set_width.y, 
        xorg, 
        yorg,
        (fix15)(sp_globals.xmax - sp_globals.xmin), 
        (fix15)(sp_globals.ymax - sp_globals.ymin));

    if (sp_globals.intercept_oflo)      /* Intercept overflow on first pass? */
        {
        sp_globals.y_band.band_max = sp_globals.ymax - 1;
        sp_globals.y_band.band_min = (sp_globals.ymin + sp_globals.y_band.band_max) >> 1;
        sp_init_intercepts_quick(PARAMS1);
        sp_globals.first_pass = FALSE;
        sp_globals.extents_running = FALSE;
        return FALSE;
        }
    else                        /* No intercept overflow on first pass? */
        {
        sp_proc_intercepts_quick(PARAMS1);      /* Output accumulated bitmap data */
        sp_flush_intercepts_quick(PARAMS1);     /* Flush pipelined bitmap data */
        close_bitmap();         /* Close bitmap output */
        return TRUE;            /* Flag successful completion */
        }
    }
else                            /* Second or subsequent pass? */
    {
    if (sp_globals.intercept_oflo)      /* Intercept overflow? */
        {
        sp_globals.y_band.band_min = (sp_globals.y_band.band_min + sp_globals.y_band.band_max) >> 1; /* Halve band size */
        sp_init_intercepts_quick(PARAMS1);      /* Init intercept storage for reduced  band */
        return FALSE;
        }

    sp_proc_intercepts_quick(PARAMS1);          /* Output accumulated bitmap data */
    if (sp_globals.y_band.band_min <= sp_globals.ymin) /* All bands completed? */
        {
        sp_flush_intercepts_quick(PARAMS1);     /* Flush pipelined bitmap data */
        close_bitmap();         /* Close bitmap output */
        return TRUE;
        }

    /* Move down to next band */
    tmpfix15 = sp_globals.y_band.band_max - sp_globals.y_band.band_min;
    sp_globals.y_band.band_max = sp_globals.y_band.band_min;
    sp_globals.y_band.band_min = sp_globals.y_band.band_max - tmpfix15;
    if (sp_globals.y_band.band_min < sp_globals.ymin)  /* New band extends beyond bottom of char? */
        {
        sp_globals.y_band.band_min = sp_globals.ymin;  /* Reduce band size to fit character */
        }
    sp_init_intercepts_quick(PARAMS1);          /* Init intercept structure for new band */
    return FALSE;
    }
}


FUNCTION 
static void sp_init_intercepts_quick(SPGLOBPTR1)
/*
 *  Called to initialize intercept storage data structure
 *  for the currently defined band
 */
{
fix15 y;

#if DEBUG >= 2
printf("Init intercepts (Y band from %d to %d)\n", sp_globals.y_band.band_min, sp_globals.y_band.band_max);
#endif 

sp_globals.no_y_lists = 
    (fix31)sp_globals.y_band.band_max - sp_globals.y_band.band_min + 1;
sp_globals.intercept_oflo = sp_globals.no_y_lists >= MAX_INTERCEPTS_QUICK;
if (sp_globals.intercept_oflo)  /* List table won't fit? */
    {
    sp_globals.no_y_lists = MAX_INTERCEPTS_QUICK;
   	sp_globals.y_band.band_min = 
        sp_globals.y_band.band_max - MAX_INTERCEPTS_QUICK + 1;
    }

for (y = 0; y < (fix15)sp_globals.no_y_lists; y++) /* For each y value in band... */
    {
    sp_intercepts.cdr[y] = 0;              /* Mark each intercept list empty */
    sp_intercepts.inttype[y] = 0;            /* Clear all intercept list flags */
    }

sp_intercepts.inttype[sp_globals.no_y_lists - 1] |= COMPLEX_INTERVAL_BELOW;
sp_globals.sp_first_sub_char_offset = sp_globals.next_offset = 
    (fix15)sp_globals.no_y_lists;
sp_globals.sp_horiz_gap_found = FALSE;
}
						

FUNCTION INLINE
static void sp_add_intercept_quick(SPGLOBPTR2 fix15 y, fix15 x, props_t props_value)
/*
 *  Called by line_quick() to add one intercept to the intercept list 
 *  structure.
 *  Assumes that the caller has checked that there is sufficient space.
 */
{
fix15 from;             /* Insertion pointers for the linked list sort */
fix15 to;

#if DEBUG >= 2
printf("    Add intercept(%2d, %d, %0.4x)\n", 
    y + sp_globals.y_band.band_min, x, props_value);
#endif

#if DEBUG >= 1
if (y < 0)                      /* Y value below bottom of current band? */
    {
    printf("*** Intecerpt Y value below bottom of band\n");
    return;
    }

if (y >= (fix15)sp_globals.no_y_lists)         /* Y value above top of current band? */
    {
    printf("*** Intercept Y value above top of band\n");
    return;
    }

if (sp_globals.next_offset > MAX_INTERCEPTS_QUICK)
    {
    printf("*** Intercept list overflow\n");
    return;
    }
#endif

sp_intercepts.car[sp_globals.next_offset] = x;
sp_intercepts.inttype[sp_globals.next_offset] = props_value;

/* Find slot to insert new element (between from and to) */
from = y;                       /* Start at list head */
while (
    (to = sp_intercepts.cdr[from]) >= 
    sp_globals.sp_first_sub_char_offset) /* To end of list */
    {
    if (x <= sp_intercepts.car[to])        /* Time to insert new item? */
        {
        break;                  /* Drop out of loop and insert here */
        }
    from = to;                  /* Move down intercept list */
    }

/* Insert or append to end of list */
sp_intercepts.cdr[from] = sp_globals.next_offset;
sp_intercepts.cdr[sp_globals.next_offset++] = to;
}


FUNCTION
static void sp_mark_salient_quick(SPGLOBPTR2 fix15 y, fix15 i1, fix15 i2)
/*
 *  Marks the intercepts at i1 and i2 as the beginning and end respectively
 *  of a salient.
 *  If the up intercept is to the right of the down intercept, the
 *  salient type is marked solid; otherwise it is marked hollow.
 *  Note that the designations solid and hollow are arbitrary and take
 *  on exchanged meanings when mirror imaging is in effect.
 *  Also sets the appropriate scanline y flags associated with the salient.
 */
{
fix15   y_interval;
fix15   i_up;
fix15   i_dn;

#if DEBUG >= 2
if (y < 0)
    {
    printf("*** sp_mark_salient_quick(): y < 0\n");
    }
if (y >= (fix15)sp_globals.no_y_lists)
    {
    printf("*** sp_mark_salient_quick(): y >= sp_globals.no_y_lists\n");
    }
#endif

if (sp_intercepts.inttype[i1] & UP_INTERCEPT)
    {
    i_up = i1;
    i_dn = i2;
    y_interval = y + 1;
    }
else
    {
    i_up = i2;
    i_dn = i1;
    y_interval = y;
    }

if (sp_intercepts.car[i_up] > sp_intercepts.car[i_dn]) /* Solid salient? */
    {
    sp_intercepts.inttype[i1] |= BEGIN_SOLID_SALIENT;
    sp_intercepts.inttype[i2] |= END_SOLID_SALIENT;
    sp_intercepts.inttype[y] |= SOLID_SALIENT_FLAG;
    if (sp_intercepts.inttype[y] & HOLLOW_SALIENT_FLAG)
        {
        sp_globals.sp_horiz_gap_found = TRUE;
        }
    if (y_interval < (fix15)sp_globals.no_y_lists)
        {
        sp_intercepts.inttype[y_interval] |= COMPLEX_INTERVAL_BELOW;
        }
#if DEBUG >= 2
    printf("    Solid salient detected at Y = %d\n", y + sp_globals.y_band.band_min);
#endif
    }
else                            /* Hollow salient? */
    {
    sp_intercepts.inttype[i1] |= BEGIN_HOLLOW_SALIENT;
    sp_intercepts.inttype[i2] |= END_HOLLOW_SALIENT;
    sp_intercepts.inttype[y] |= HOLLOW_SALIENT_FLAG;
    if (sp_intercepts.inttype[y] & SOLID_SALIENT_FLAG)
        {
        sp_globals.sp_horiz_gap_found = TRUE;
        }
    if (y_interval < (fix15)sp_globals.no_y_lists)
        {
        sp_intercepts.inttype[y_interval] |= COMPLEX_INTERVAL_BELOW;
        }
#if DEBUG >= 2
    printf("    Hollow salient detected at Y = %d\n", y + sp_globals.y_band.band_min);
#endif
    }
}


FUNCTION 
static void sp_proc_intercepts_quick(SPGLOBPTR1)
/*
 *  Called by end_char_quick() to output accumulated intercept lists
 *  for the current band.
 */
{
fix15   first_y;
fix15   y;
fix15   scanline;
fix15   i1;
fix15   j1;
fix15   i2;
fix15   j2;
fix15   x_start1;
fix15   x_start2;
fix15   x_end1;
fix15   x_end2;

first_y = (fix15)sp_globals.no_y_lists - 1;

#if DEBUG >= 2
printf("\nsp_proc_intercepts_quick:\n");
#endif

/* Close gaps (if any) within scanlines */
if (sp_globals.sp_horiz_gap_found)
    {
    for (y = first_y; y > 0; y--)
        {
        if ((sp_intercepts.inttype[y] &
            (SOLID_SALIENT_FLAG + HOLLOW_SALIENT_FLAG)) ==
            (SOLID_SALIENT_FLAG + HOLLOW_SALIENT_FLAG))
            {
            sp_close_horiz_gaps_quick(PARAMS2 y);
            }
        }
    }

scanline = sp_globals.ymax - sp_globals.y_band.band_min - first_y - 1;
for (y = first_y; y > 0; y--, scanline++)
    {
#if DEBUG >= 2
    print_intercepts(y);        /* Print intercept list for this scanline */
#endif
    if (sp_intercepts.inttype[y] & 
        (COMPLEX_INTERVAL_BELOW |
         COMPLEX_SCANLINE_FLAG))
        {
        sp_close_diag_gaps_quick(PARAMS2 y);
        if (sp_intercepts.inttype[y] & COMPOUND_SCANLINE_FLAG)
            {
            sp_sort_intercepts_quick(PARAMS2 y);
            }

        sp_output_complex_scanline_quick(PARAMS2 y, scanline);
        sp_check_scanline_quick(PARAMS2 y - 1);
        }
    else						/* Simple scanline? */
        {
		if ((i1 = sp_intercepts.cdr[y]) == 0) /* Intercept list empty? */
    		{
    		continue;
    		}

		i2 = sp_intercepts.cdr[y - 1];     /* First intercept in next scanline */
		if (sp_intercepts.car[i2] < sp_globals.xmin) /* Outside bitmap bounding box? */
    		{
    		sp_intercepts.inttype[y - 1] |= COMPLEX_SCANLINE_FLAG;
#if DEBUG >= 1
    		printf("*** sp_proc_intercepts_quick: First intercept outside bbox on lower scanline\n");
#endif
		    }

		do
    		{
#if DEBUG >= 1
    		if (i1 == 0)
        		{
        		printf("*** sp_proc_intercepts_quick: Too few intercepts on upper scanline\n");
        		}
    		if ((sp_intercepts.cdr[i1] == 0) || (sp_intercepts.cdr[i2] == 0))
        		{
        		printf("*** sp_proc_intercepts_quick: Odd number of intercepts\n");
        		}
#endif
            x_start1 = sp_intercepts.car[i1];
            j1 = sp_intercepts.cdr[i1];
            x_end1 = sp_intercepts.car[j1];

            x_start2 = sp_intercepts.car[i2];
            j2 = sp_intercepts.cdr[i2];
            x_end2 = sp_intercepts.car[j2];
    		
    		/* Fill diagonal gaps if present */
    		if (x_start2 > x_end1)		/* Lower run starts after upper one ends? */
        		{
#if DEBUG >= 2
        		printf("simple_close_gap(%d, -, -, %d)\n", x_end1, x_start2);
#endif
        		x_end1 = sp_intercepts.car[i2] = (x_end1 + x_start2) >> 1;
        		}
    		else if (x_start1 > x_end2) /* Upper run starts after lower one ends? */
        		{
#if DEBUG >= 2
        		printf("simple_close_gap(-, %d, %d, -)\n", x_start1, x_end2);
#endif
        		sp_intercepts.car[j2] = x_start1 = (x_end2 + x_start1) >> 1;
        		}

    		/* Output run of bits */
    		set_bitmap_bits(
        		scanline, 
        		x_start1 - sp_globals.xmin, 
        		x_end1 - sp_globals.xmin);

    		/* Check for zero_length run on next scanline */
    		if (x_start2 == x_end2) /* Zero-length run on next line? */
        		{
        		sp_intercepts.inttype[y - 1] |= COMPLEX_SCANLINE_FLAG;
        		if (x_start1 < x_start2)
            		{
            		sp_intercepts.inttype[i2]++; /* Mark start of run extendible */
            		}
        		if (x_end1 > x_end2)
            		{
            		sp_intercepts.inttype[j2]++; /* Mark end of run extendible */
            		}
        		}

    		/* Check for overlapping runs on next scanline */
    		if (((sp_intercepts.inttype[i2] ^ sp_intercepts.inttype[j2]) & UP_INTERCEPT) == 0) 
        		{
        		sp_intercepts.inttype[y - 1] |= COMPLEX_SCANLINE_FLAG;
        		}
        		
			i1 = sp_intercepts.cdr[j1];
    		} 
    	while ((i2 = sp_intercepts.cdr[j2]) != 0);

#if DEBUG >= 1
		if (i1 != 0)
    		{
    		printf("*** sp_proc_intercepts_quick: Too few intercepts on lower scanline\n");
    		}
#endif

		if (x_end2 > sp_globals.xmax)
    		{
   			sp_intercepts.inttype[y - 1] |= COMPLEX_SCANLINE_FLAG;
#if DEBUG >= 1
    		printf("*** sp_proc_intercepts_quick: Last intercept outside bbox on next scanline\n");
#endif
    		}
        }
    }
}


FUNCTION 
static void sp_flush_intercepts_quick(SPGLOBPTR1)
/*
 *  Called by end_char_quick() to output accumulated intercept lists
 *  in last scanline (if any).
 */
{
fix15   y;
fix15   scanline;

#if DEBUG >= 2
printf("\nsp_flush_intercepts_quick:\n");
#endif

if (sp_globals.no_y_lists > 0)          /* At least one scanline? */
    {
    y = 0;

#if DEBUG >= 2
    print_intercepts(y);
#endif

    if ((sp_intercepts.inttype[y] &
        (SOLID_SALIENT_FLAG + HOLLOW_SALIENT_FLAG)) ==
        (SOLID_SALIENT_FLAG + HOLLOW_SALIENT_FLAG))
        {
        sp_close_horiz_gaps_quick(PARAMS2 y);
        }

    if (sp_intercepts.inttype[y] & COMPOUND_SCANLINE_FLAG)
        {
        sp_sort_intercepts_quick(PARAMS2 y);
        }

	scanline = sp_globals.ymax - sp_globals.y_band.band_min - y - 1;
    sp_output_complex_scanline_quick(PARAMS2 y, scanline);
    }
}


FUNCTION 
static void sp_close_horiz_gaps_quick(SPGLOBPTR2 fix15 y)
/*
 *  Called by sp_proc_intercepts_quick() and sp_flush_intercepts_quick() to close any gaps 
 *  within a specified scanline. Candidate scanlines are those that contain
 *  both hollow and solid salients.
 *  Each intercept in the scanline is tested to see if it the beginning
 *  or end of a salient. 
 *  A hollow salient count and a solid salient count start at zero.
 *  The appropriate count is incremented or decremented for each beginging and 
 *  each end of a salient. Incrementing is carried out for up intercepts; 
 *  decrementing for down intercepts. 
 *  Simultaneously, a winding number is incremented for each up intercept
 *  and decremented for each down intercept.
 *  A gap to be closed is identified by detecting an interval between a pair 
 *  of consecutive intercepts for which the current winding number is zero 
 *  and both accumulated salient counts are non-zero.
 *  A gap is closed by extending the preceding and following runs to the
 *  min-point of the gap.
 */
{
fix15   winding_number;
fix15   hollow_salient_count;
fix15   solid_salient_count;
fix15   i;

#if DEBUG >= 2
printf("sp_close_horiz_gaps_quick(%d)\n", y);
#endif

winding_number = 0;
hollow_salient_count = 0;
solid_salient_count = 0;
i = sp_intercepts.cdr[y];
while (i != 0)
    {
    if (sp_intercepts.inttype[i] & UP_INTERCEPT)
        {
        winding_number++;

        if (sp_intercepts.inttype[i] & BEGIN_SOLID_SALIENT)
            {
            solid_salient_count++;
            }

        if (sp_intercepts.inttype[i] & END_SOLID_SALIENT)
            {
            solid_salient_count++;
            }

        if (sp_intercepts.inttype[i] & BEGIN_HOLLOW_SALIENT)
            {
            hollow_salient_count++;
            }

        if (sp_intercepts.inttype[i] & END_HOLLOW_SALIENT)
            {
            hollow_salient_count++;
            }
        }
    else
        {
        winding_number--;

        if (sp_intercepts.inttype[i] & BEGIN_SOLID_SALIENT)
            {
            solid_salient_count--;
            }

        if (sp_intercepts.inttype[i] & END_SOLID_SALIENT)
            {
            solid_salient_count--;
            }

        if (sp_intercepts.inttype[i] & BEGIN_HOLLOW_SALIENT)
            {
            hollow_salient_count--;
            }

        if (sp_intercepts.inttype[i] & END_HOLLOW_SALIENT)
            {
            hollow_salient_count--;
            }
        }

    if ((winding_number == 0) &&
        (hollow_salient_count != 0) &&
        (solid_salient_count != 0))
        {

#if DEBUG >= 2
        printf("sp_close_horiz_gaps_quick(%d, %d)\n", 
            sp_intercepts.car[i], sp_intercepts.car[sp_intercepts.cdr[i]]);
#endif

        sp_intercepts.car[i] = 
            sp_intercepts.car[sp_intercepts.cdr[i]] = (sp_intercepts.car[i] + sp_intercepts.car[sp_intercepts.cdr[i]]) >> 1;
        }
    i = sp_intercepts.cdr[i];
    }

#if DEBUG >= 1
if (solid_salient_count || hollow_salient_count)
    {
    printf("*** sp_close_horiz_gaps_quick: non-zero salient count at end of list\n");
    }
#endif
}


FUNCTION 
static void sp_close_diag_gaps_quick(SPGLOBPTR2 fix15 y)
/*
 *  Called by sp_proc_intercepts_quick() to close any diagnoal gaps between the 
 *  specified scanline and the scanline immediatly below it.
 *  Accomplished by pairing groups of runs from the upper and lower scanlines
 *  and then detecting and closing any gaps between them.
 */
{
fix15   i1;
fix15   i2;
fix15   i1_start;
fix15   i1_end;
fix15   i2_start;
fix15   i2_end;

#if DEBUG >= 1
if ((y < 1) || (y >= (fix15)sp_globals.no_y_lists))
    {
    printf("*** sp_close_diag_gaps_quick(%d): y out of range\n", y);
    }
#endif

i1 = sp_intercepts.cdr[y];                    /* Index of head of upper intercept list */
i2 = sp_intercepts.cdr[y - 1];                /* Index to head of lower intercept list */
while ((i1 != 0) &&
    (sp_assemble_group_pair_quick(PARAMS2 i1, i2, &i1_start, &i1_end, &i2_start, &i2_end)))
    {
    sp_proc_group_pair_quick(PARAMS2 i1_start, i1_end, i2_start, i2_end);
    i1 = sp_intercepts.cdr[i1_end];
    i2 = sp_intercepts.cdr[i2_end];
    }
}


FUNCTION 
static btsbool sp_assemble_group_pair_quick(SPGLOBPTR2
											fix15 i1,
											fix15 i2,
											fix15 *pi1_start,
											fix15 *pi1_end,
											fix15 *pi2_start,
											fix15 *pi2_end)
/*
 * Called by sp_proc_intercepts_quick() to assemble a corresponding pair of groups
 * of one or more runs on the upper and lower scanlines.
 */
{
fix15   n_open_intercepts;
fix15   min_open_intercepts;

sp_assemble_group_quick(PARAMS2 
    i1,
    (props_t)(END_SOLID_SALIENT | END_HOLLOW_SALIENT),
    (props_t)(BEGIN_SOLID_SALIENT | BEGIN_HOLLOW_SALIENT),
    (fix15)1,
    pi1_start,
    pi1_end,
    &n_open_intercepts);

if (n_open_intercepts == 0)
    {
    return FALSE;
    }

while (TRUE)
    {
    min_open_intercepts = n_open_intercepts;
    sp_assemble_group_quick(PARAMS2 
        i2,
        (props_t)(BEGIN_SOLID_SALIENT | BEGIN_HOLLOW_SALIENT),
        (props_t)(END_SOLID_SALIENT | END_HOLLOW_SALIENT),
        min_open_intercepts,
        pi2_start,
        pi2_end,
        &n_open_intercepts);

#if DEBUG >= 1
    if (n_open_intercepts < min_open_intercepts)
        {
        printf("*** sp_assemble_group_pair_quick: Can't assemble matching group (2)\n");
        }
#endif

    if (n_open_intercepts == min_open_intercepts)
        {
        return TRUE;
        }

    min_open_intercepts = n_open_intercepts;
    sp_assemble_group_quick(PARAMS2 
        i1,
        (props_t)(END_SOLID_SALIENT | END_HOLLOW_SALIENT),
        (props_t)(BEGIN_SOLID_SALIENT | BEGIN_HOLLOW_SALIENT),
        min_open_intercepts,
        pi1_start,
        pi1_end,
        &n_open_intercepts);

#if DEBUG >= 1
    if (n_open_intercepts < min_open_intercepts)
        {
        printf("*** sp_assemble_group_pair_quick: Can't assemble matching group (1)\n");
        }
#endif

    if (n_open_intercepts == min_open_intercepts)
        {
        return TRUE;
        }
    }
}
 

FUNCTION
static void sp_assemble_group_quick(SPGLOBPTR2
   									fix15 i, 
    								props_t up_salient_mask, 
    								props_t down_salient_mask, 
    								fix15 min_open_intercepts,
    								fix15 *pi_start,
    								fix15 *pi_end,
    								fix15 *pn_open_intercepts)
/*
 *  Identifies the next group in the given intercept list
 *  that has at least the specified number of open intercepts.
 *  An open intercept is one that is not associated with a
 *  relevent salient. The up_salient_mask selects relevant
 *  salients for up intercepts. The down-salient_mask selects
 *  relevant salients for down intercepts.
 *  A group consists of at least one run of pixels that ends
 *  with a zero winding number.
 */
{
fix15   winding_number;
fix15   salient_count;

*pn_open_intercepts = 0;        

if (i == 0)                     /* Empty intercept list? */
    {
    return;
    }

winding_number = 0;
salient_count = 0;
*pi_start = i;
while (i != 0)                  /* For each intercept... */
    {
    if (sp_intercepts.inttype[i] & UP_INTERCEPT) /* Up intercept? */
        {
        winding_number++;
        if (sp_intercepts.inttype[i] & up_salient_mask)
            {
            salient_count++;
            }
        else
            {
            *pn_open_intercepts += 1;
            }
        }
    else                        /* Down intercept? */
        {
        winding_number--;
        if (sp_intercepts.inttype[i] & down_salient_mask)
            {
            salient_count--;
            }
        else
            {
            *pn_open_intercepts += 1;
            }
        }

    if ((winding_number == 0) &&
        (salient_count == 0))   /* End of group? */
        {
        if (*pn_open_intercepts >= min_open_intercepts)
            {
            *pi_end = i;
            return;
            }

        if (*pn_open_intercepts == 0) /* Isolated terminal? */
            {
            *pi_start = sp_intercepts.cdr[i];
            }
        }

    i = sp_intercepts.cdr[i];
    }
}

FUNCTION
static void sp_proc_group_pair_quick(SPGLOBPTR2
									 fix15 i1_start,
									 fix15 i1_end,
									 fix15 i2_start,
									 fix15 i2_end)
/*
 *  Processes a group of one or more runs from the upper scanline and the
 *  corresponding group of one or more runs from the lower scanline.
 *  Increments the extendibility of zero-length groups in either scanline
 *  whose corresponding group of one or more runs in the other scanline 
 *  extend beyond them.
 *  Fills any detected gaps by choosing the runs that require
 *  the minimum extensions to maintain continuity.
 */
{
fix15   x1_start;
fix15   x1_end;
fix15   x2_start;
fix15   x2_end;
fix15   i1_gap_start;
fix15   i2_gap_start;
fix15   i1;
fix15   i2;
fix15   winding_number1;
fix15   winding_number2;

#if DEBUG >= 2
printf("sp_proc_group_pair_quick(%d, %d, %d, %d)\n", 
    sp_intercepts.car[i1_start], 
    sp_intercepts.car[i1_end], 
    sp_intercepts.car[i2_start],
    sp_intercepts.car[i2_end]);
#endif

/* Increment extendability of zero-length runs */
x1_start = sp_intercepts.car[i1_start];
x1_end = sp_intercepts.car[i1_end];
x2_start = sp_intercepts.car[i2_start];
x2_end =sp_intercepts.car[i2_end];

if (x1_start == x1_end)
    {
    if (x2_start < x1_start)
        {
        sp_intercepts.inttype[i1_start]++;
        }
    if (x2_end > x1_end)
        {
        sp_intercepts.inttype[i1_end]++;
        }
    }

if (x2_start == x2_end)
    {
    if (x1_start < x2_start)
        {
        sp_intercepts.inttype[i2_start]++;
        }
    if (x1_end > x2_end)
        {
        sp_intercepts.inttype[i2_end]++;
        }
    }

/* Detect and fill gaps within the pair of corresponding groups */
i1_gap_start = i2_gap_start = 0;
i1 = i1_start;
i2 = i2_start;
winding_number1 = 0;
winding_number2 = 0;
while (TRUE)                    /* Loop for each detected gap */
    {
    do                          /* Loop to find next gap */
        {
        if (i1 == 0)            /* At end of group in upper scanline? */
            {
            if (i2 == 0)        /* At end of group in lower scanline? */
                {
                return;
                }
            goto L2;            /* Step across lower scanline */
            }

        if (i2 == 0)            /* At end of group in upper scanline? */
            goto L1;            /* Step across upper scanline */

        if (sp_intercepts.car[i1] > sp_intercepts.car[i2]) /* Next intercept in lower scanline? */
            goto L2;            /* Step across lower scanline */

    	/* Step one intercept across upper scanline */
    L1: if (sp_intercepts.inttype[i1] & UP_INTERCEPT)
            {
            winding_number1++;
            }
        else
            {
            winding_number1--;
            }

        if (winding_number1 == 0) /* Start of gap in upper scanline? */
            {
            i1_gap_start = i1;
            i1 = (i1 != i1_end)? sp_intercepts.cdr[i1]: 0;
            }
        else
            {
            i1 = sp_intercepts.cdr[i1];
            }
        continue;

    	/* Step one intercept across lower scanline */
    L2: if (sp_intercepts.inttype[i2] & UP_INTERCEPT)
            {
            winding_number2++;
            }
        else
            {
            winding_number2--;
            }

        if (winding_number2 == 0) /* Start of gap in lower scanline? */
            {
            i2_gap_start = i2;
            i2 = (i2 != i2_end)? sp_intercepts.cdr[i2]: 0;
            }
        else
            {
            i2 = sp_intercepts.cdr[i2];
            }
        continue;
        }
    while ((winding_number1 != 0) || (winding_number2 != 0));

    if ((i1 == 0) && (i2 == 0))
        {
        return;
        }
    sp_close_gap_quick(PARAMS2 i1_gap_start, i1, i2_gap_start, i2);
    }
}


FUNCTION
static void sp_close_gap_quick(SPGLOBPTR2
						       fix15 i1_start,
							   fix15 i1_end,
							   fix15 i2_start,
							   fix15 i2_end)
/*
 *  Given a pair of overlapping gaps on the upper and lower
 *  scanlines sp_close_gap_quick() adjusts the rightmost gap start
 *  point and the leftmost gap end point to coincide at
 *  the mid point between them
 */
{
fix15   i;
fix15   i_start;
fix15   i_end;
fix15   length;
fix15   min_length;
fix15   i_start_min;
fix15   i_end_min;
btsbool initialized;

#if DEBUG >= 2
printf("sp_close_gap_quick(");

if (i1_start != 0) 
    printf("%d", sp_intercepts.car[i1_start]);
else 
    printf("-"); 

printf(", ");

if (i1_end != 0) 
    printf("%d", sp_intercepts.car[i1_end]);
else 
    printf("-"); 

printf(", ");

if (i2_start != 0) 
    printf("%d", sp_intercepts.car[i2_start]);
else 
    printf("-"); 

printf(", ");

if (i2_end != 0) 
    printf("%d", sp_intercepts.car[i2_end]);
else 
    printf("-"); 

printf(")\n");
#endif

/* Find shortest path for gap closing */
initialized = FALSE;
for (i = 0; i < 4; i++)
    {
    switch (i)
        {
    case 0:
        i_start = i1_start;
        i_end = i1_end;
        break;

    case 1:
        i_start = i2_start;
        i_end = i2_end;
        break;

    case 2:
        i_start = i1_start;
        i_end = i2_end;
        break;

    case 3:
        i_start = i2_start;
        i_end = i1_end;
        break;
        }

    if ((i_start == 0) ||
        (i_end == 0))
        {
        continue;
        }

    length = sp_intercepts.car[i_end] - sp_intercepts.car[i_start];
    if ((!initialized) ||
        (length < min_length))
        {
        min_length = length;
        i_start_min = i_start;
        i_end_min = i_end;
        initialized = TRUE;
        }
    }

/* Adjust selected start and end points to close gap */
if (initialized)
    {
    sp_intercepts.car[i_start_min] = sp_intercepts.car[i_end_min] =
        (sp_intercepts.car[i_start_min] + sp_intercepts.car[i_end_min]) >> 1;
    }
}


FUNCTION
static void sp_sort_intercepts_quick(SPGLOBPTR2 fix15 y)
/*
 * Called by sp_proc_intercepts_quick() to sort the intercepts in the given
 * scanline into ascending order.
 * This is only required for those scanlines in compound characters
 * that have received runs from two or more sub-characters. For
 * simple characters (and scanlines in compound characters that only 
 * receive runs from one sub-character) sorting is not needed as it
 * has already been accomplished by sp_add_intercepts_quick() which inserts
 * new intercepts in ascending order.
 */
{
fix15   i1;
fix15   j1;
fix15   i2;
fix15   j2;
fix15   i1_prev;

i1 = sp_intercepts.cdr[y];
if (i1 == 0)                    /* Intercept list is empty? */
    {
    return;
    }

/* Find end of main intercept list */
while (TRUE)
    {
    j1 = sp_intercepts.cdr[i1];
    if (j1 == 0)                /* End of intercept list? */
        {
        return;
        }
    if (sp_intercepts.car[i1] > sp_intercepts.car[j1]) /* Intercept out of order? */
        {
        sp_intercepts.cdr[i1] = 0;         /* Terminate main list */
        i2 = j1;                /* Start of list to be merged */
        break;
        }
    i1 = j1;                    /* Continue down intercept list */
    }

/* Merge list to be merged into main intercept list */
while (TRUE)
    {
    i1_prev = y;
    while ((i1 = sp_intercepts.cdr[i1_prev]) != 0) /* For each elem of main list... */
        {
        if (sp_intercepts.car[i2] <= sp_intercepts.car[i1]) /* Time to merge sublist? */
            {
            sp_intercepts.cdr[i1_prev] = i2; /* Link main list into sublist */
        L1: j2 = sp_intercepts.cdr[i2];
            if (j2 == 0)        /* Last intercept in last sublist? */
                {
                sp_intercepts.cdr[i2] = i1; /* Link back into main list */
                return;
                }
            if (sp_intercepts.car[i2] > sp_intercepts.car[j2]) /* Last intercept in sublist? */
                {
                sp_intercepts.cdr[i2] = i1; /* Link back into main list */
                i2 = j2;        /* Start of next sublist */
                i1_prev = y;    /* Restart main list */
                continue;
                }
            if (sp_intercepts.car[j2] <= sp_intercepts.car[i1]) /* Incl next intercept in merge? */
                {
                i2 = j2;        /* Next element of sublist */
                goto L1;
                }
            sp_intercepts.cdr[i2] = i1;    /* Link back into main list */
            i2 = j2;            /* Remainder of sublist */
            }
        i1_prev = i1;
        }
    sp_intercepts.cdr[i1_prev] = i2;       /* Link to remaining sublist */
    while (TRUE)
        {
        if ((j2 = sp_intercepts.cdr[i2]) == 0) /* End of last sublist? */
            {
            return;
            }
        if (sp_intercepts.car[i2] > sp_intercepts.car[j2])  /* End of sublist? */
            {
            sp_intercepts.cdr[i2] = 0;     /* Terminate merged sublist */
            i2 = j2;            /* Start of next sublist */
            break;
            }
        i2 = j2;
        }
    }
}


FUNCTION
static void sp_output_complex_scanline_quick(SPGLOBPTR2 fix15 y, fix15 scanline)
/*
 *  This is a less restrictive, more comprhensive, slower version of the function
 *  function included within sp_proc_intercepts_quick.
 *  Runs of pixels may overlap but the individual intercepts are
 *  assumed to be in strict left-to-right order.
 */
{
fix15   i;
fix15   j;
fix15   old_x_end;
fix15   winding_number;
fix15   x_start;
fix15   x_end;
fix15   avail_pix_left;
fix15   avail_pix_right;

old_x_end = sp_globals.xmin - 2;
winding_number = 0;
i = sp_intercepts.cdr[y];
while (i != 0)                  /* Not end of intercept list? */
    {
    x_start = sp_intercepts.car[i];        /* Start of pixel run */
L1: if (sp_intercepts.inttype[i] & UP_INTERCEPT)
        {
        winding_number++;
        }
    else
        {
        winding_number--;
        }
    j = sp_intercepts.cdr[i];
    x_end = sp_intercepts.car[j];
    if (sp_intercepts.inttype[j] & UP_INTERCEPT)
        {
        winding_number++;
        }
    else
        {
        winding_number--;
        }

    if (winding_number != 0)    /* Overlapping runs? */
        {
        i = sp_intercepts.cdr[j];
        goto L1;
        }

    if (x_start < sp_globals.xmin)
        {
#if DEBUG >= 1
        printf("*** sp_output_complex_scanline_quick: Character BBox violation\n");
#endif
        x_start = sp_globals.xmin;
        if (x_end < x_start)
            {
            x_end = x_start;
            }
        }

    if (x_end > sp_globals.xmax)
        {
#if DEBUG >= 1
        printf("*** sp_output_complex_scanline_quick: Character BBox violation\n");
#endif
        x_end = sp_globals.xmax;
        if (x_end < x_start)
            {
            x_start = x_end;
            }
        }

    /* Check for zero-length run of pixels */
    if (x_start == x_end)
        {
        avail_pix_left = x_start - old_x_end;
        avail_pix_right = 
            ((sp_intercepts.cdr[j] != 0)? sp_intercepts.car[sp_intercepts.cdr[j]]: sp_globals.xmax + 2) - x_end;
        if (sp_extend_zero_length_run_quick(PARAMS2 
            avail_pix_left, 
            (fix15)(sp_intercepts.inttype[i] & EXTENDIBILITY),
            avail_pix_right, 
            (fix15)(sp_intercepts.inttype[j] & EXTENDIBILITY),
            &x_start, 
            &x_end))
            {
            set_bitmap_bits(scanline, x_start - sp_globals.xmin, x_end - sp_globals.xmin);
            }
        }
    else
        {
        set_bitmap_bits(scanline, x_start - sp_globals.xmin, x_end - sp_globals.xmin);
        }

    i = sp_intercepts.cdr[j];
    old_x_end = x_end;
    }
}


FUNCTION 
static btsbool sp_extend_zero_length_run_quick(SPGLOBPTR2 
    										   fix15 avail_pix_left,
											   fix15 extendibility_left,
    										   fix15 avail_pix_right,
											   fix15 extendibility_right,
    										   fix15 *px_start,
											   fix15 *px_end)
/*
 * Called by sp_output_complex_scanline_quick() to extend a zero-length run
 * of pixels by one pixel to the left or right. No extension is applies
 * if the zero-length run is touching its neighbor.
 * The choice of left or right extension is made on the basis of:
 *     avoiding touching or overlapping the adjacent run of pixels
 *     avoiding extending a run outside the character bounding box
 * If both directions are available, the zero-length run is extended
 * in the direction associated with the maximum extendibility.
 * If both directions are available and have equal extendibility, the
 * zero-length run is extended to the right.
 */
{
btsbool left_possible;
btsbool right_possible;

left_possible = 
    (avail_pix_left > 0) &&
    (avail_pix_right > -1) && 
    (*px_start > sp_globals.xmin);

right_possible = 
    (avail_pix_right > 0) &&
    (avail_pix_left > -1) &&
    (*px_end < sp_globals.xmax);

if (left_possible && right_possible)
    {
    if (extendibility_left > extendibility_right)
        {
#if DEBUG >= 2
        printf("Zero-length run at %d extended to left\n", *px_start);
#endif
        *px_start -= 1;
        }
    else
        {
#if DEBUG >= 2
        printf("Zero-length run at %d extended to right\n", *px_start);
#endif
        *px_end += 1;
        }
    return TRUE;
    }

if (left_possible)
    {
#if DEBUG >= 2
    printf("Zero-length run at %d extended to left\n", *px_start);
#endif
    *px_start -= 1;
    return TRUE;
    }

if (right_possible)
    {
#if DEBUG >= 2
    printf("Zero-length run at %d extended to right\n", *px_start);
#endif
    *px_end += 1;
    return TRUE;
    }

#if DEBUG >= 2
    printf("Zero-length run at %d not extended\n", *px_start);
#endif
return FALSE;
}


FUNCTION
static void sp_check_scanline_quick(SPGLOBPTR2 fix15 y)
/*
 * Checks the specified scanline
 * Sets the COMPLEX_SCANLINE_FLAG if:
 *    - the first or last intercept is outside the character bounding box.
 *    - even-odd fill does not produce the same result as winding number fill.
 *    - zero-length runs need to be extended.
 */
{
fix15   i;
fix15   j;

if ((i = sp_intercepts.cdr[y]) == 0)
    {
    return;
    }

/* Check first intercept within bitmap bounding box */
if (sp_intercepts.car[i] < sp_globals.xmin)
    {
    sp_intercepts.inttype[y] |= COMPLEX_SCANLINE_FLAG;
    }

/* Check odd and even intercept have opposing directions */
do
    {
    j = sp_intercepts.cdr[i];
    if (((sp_intercepts.inttype[i] ^ sp_intercepts.inttype[j]) & UP_INTERCEPT) == 0)
        {
        sp_intercepts.inttype[y] |= COMPLEX_SCANLINE_FLAG;
        }
    if (sp_intercepts.car[i] == sp_intercepts.car[j])
        {
        sp_intercepts.inttype[y] |= COMPLEX_SCANLINE_FLAG;
        }
    i = sp_intercepts.cdr[j];
    }
while (i != 0);

/* Check last intercept within bitmap bounding box */
if (sp_intercepts.car[j] > sp_globals.xmax)
    {
    sp_intercepts.inttype[y] |= COMPLEX_SCANLINE_FLAG;
    }
}


#if DEBUG >= 2

FUNCTION
static void print_intercepts(SPGLOBPTR2 fix15 y)
/*
 * Prints one row of intercepts for debugging purposes
 * Each row is printed in the following format:
 *   2 ( 15) ----h |  3 -----v    4 --(--^ |  5 ---)-v    6 -----^ |
 * The first number is the scanline number in its output form
 * The second number in parens is the absolute scanline number.
 * The funny symbols before the first vertical bar refer to the
 * intercept list itself:
 *     # Compound scanline has runs from 2 or more sub-chars
 *     * Complex scanline
 *     ! Complex interval below this scanline
 *     s Scanline contains one or more solid salients
 *     h Scanline contains one or more hollow salients
 * After the first vertical bar, each intercept is printed as the actual
 * X intercept value followed by a series of symbols. The symbols for
 * each intercept are:
 *     [ Start of solid salient
 *     ] Start of solid salient
 *     ( Start of hollow salient
 *     ) Start of hollow salient
 *     e Pixel run is extendible by adjusting this intercept
 *     ^ Up intercept
 *     v Down intercept
 * A vertical bar separates each pair of intercepts.
 */
{
fix15   i, j;
fix15   scanline;
fix15   from;
fix15   to;

if ((i = sp_intercepts.cdr[y]) == 0)
    {
    return;
    }

scanline = sp_globals.ymax - sp_globals.y_band.band_min - y - 1;
printf("%3d (%3d) %c%c%c%c%c |", 
    scanline, 
    y + sp_globals.y_band.band_min,
    (sp_intercepts.inttype[y] & COMPOUND_SCANLINE_FLAG)? '#': '-',
    (sp_intercepts.inttype[y] & COMPLEX_SCANLINE_FLAG)? '*': '-',
    (sp_intercepts.inttype[y] & COMPLEX_INTERVAL_BELOW)? '!': '-',
    (sp_intercepts.inttype[y] & SOLID_SALIENT_FLAG)? 's': '-',
    (sp_intercepts.inttype[y] & HOLLOW_SALIENT_FLAG)? 'h': '-');

while (i != 0)                  /* Link to next intercept if present */
    {
    from = sp_intercepts.car[i];
    j = sp_intercepts.cdr[i];              /* Link to next intercept */
    if (j == 0)                 /* End of list? */
        {
        printf("\n*** print_intercepts: odd number of intercepts\n");
        break;
        }
    to = sp_intercepts.car[j];
    printf("%3d %c%c%c%c%c%c  %3d %c%c%c%c%c%c |", 
        from, 
        (sp_intercepts.inttype[i] & BEGIN_SOLID_SALIENT)? '[': '-',
        (sp_intercepts.inttype[i] & END_SOLID_SALIENT)? ']': '-',
        (sp_intercepts.inttype[i] & BEGIN_HOLLOW_SALIENT)? '(': '-',
        (sp_intercepts.inttype[i] & END_HOLLOW_SALIENT)? ')': '-',
        (sp_intercepts.inttype[i] & EXTENDIBILITY)? 'e': '-',
        (sp_intercepts.inttype[i] & UP_INTERCEPT)? '^': 'v',
        to,
        (sp_intercepts.inttype[j] & BEGIN_SOLID_SALIENT)? '[': '-',
        (sp_intercepts.inttype[j] & END_SOLID_SALIENT)? ']': '-',
        (sp_intercepts.inttype[j] & BEGIN_HOLLOW_SALIENT)? '(': '-',
        (sp_intercepts.inttype[j] & END_HOLLOW_SALIENT)? ')': '-',
        (sp_intercepts.inttype[j] & EXTENDIBILITY)? 'e': '-',
        (sp_intercepts.inttype[j] & UP_INTERCEPT)? '^': 'v');
    i = sp_intercepts.cdr[j];
    }
printf("\n");
}
#endif

#endif

