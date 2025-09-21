/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/
/********************* Revision Control Information **********************************
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/pcl5/hpreader/hpfnt1.c,v 7.0 97/03/17 17:42:22 shawn Release $                                                                       *
*                                                                                    *
* 
*     $Log:	hpfnt1.c,v $
 * Revision 7.0  97/03/17  17:42:22  shawn
 * TDIS Version 6.00
 * 
 * Revision 6.5  96/07/02  17:05:03  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 6.4  96/07/01  13:32:57  shawn
 * Initialized uninitialized variables (X&Y).
 * 
 * Revision 6.3  96/07/01  11:03:54  shawn
 * Added casts to resolve compiler warnings.
 * Added more support for the Gray Writer output module.
 * 
 * Revision 6.2  96/03/28  11:49:55  shawn
 * Added support for the Gray Writer output module.
 * 
 * Revision 6.1  95/11/17  11:13:06  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 6.0  95/02/15  16:21:20  roberte
 * Release
 * 
 * Revision 5.2  95/01/10  08:51:25  roberte
 * Updated copyright header.
 * 
 * Revision 5.1  95/01/04  16:30:05  roberte
 * Release
 * 
 * Revision 4.7  94/12/21  16:15:57  roberte
 * Added casts of return of eo_get_char_addr() to (eo_char_t *).
 * 
 * Revision 4.6  94/12/21  15:08:04  shawn
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 4.5  94/08/08  11:16:45  roberte
 * Blocked off eo_get_char_bbox() within #if INCL_METRICS conditional.
 * 
 * Revision 4.4  94/07/21  15:48:50  roberte
 * Redid changes after port to PC. Eliminate eo_ctm variable, and make local prototypes ANSI friendly.
 * 
 * Revision 4.0  94/05/04  09:25:18  roberte
 * Release
 * 
 * Revision 3.0  94/04/08  11:53:36  roberte
 * Release
 * 
 * Revision 2.96  94/04/08  11:13:33  roberte
 * Changed location of setting of emResolution in eo_get_char_width_orus().
 * This must always be set whether success or failure.
 * 
 * Revision 2.95  94/04/08  10:16:24  roberte
 * Corrected eo_get_char_width() when calling new funct, eo_get_char_width_orus().
 * Added some comments.
 * 
 * Revision 2.94  94/03/23  10:05:11  roberte
 * Added new entry point eo_get_char_width_orus().
 * Have eo_get_char_width() call it.
 * 
 * Revision 2.93  94/02/03  11:51:30  roberte
 * Removed "static" from local function prototypes hated by TurboC compiler.
 * 
 * Revision 2.92  93/08/31  12:45:33  roberte
 * Change "init_user_out" to "sp_init_user_out"
 * 
 * Revision 2.91  93/08/30  15:36:32  roberte
 * Release
 * 
 * Revision 2.26  93/08/17  10:32:07  roberte
 * Add MODE_QUICK and also add INCL_WHITE to REENTRANT_ALLOC conditional declaring intercepts_t intercepts.
 * 
 * Revision 2.25  93/05/28  09:24:30  roberte
 * Added #if PROC_PCL conditional around entire file, after include of spdo_prv.h
 * 
 * Revision 2.24  93/05/21  11:55:55  roberte
 * Removed some inadvertant nested comments.
 * cast all 2nd parameters in calls to shift_16()
 * Corrected all local function mini-prototypes to match any real prototypes in this file.
 * 
 * Revision 2.23  93/03/15  13:08:34  roberte
 * Release
 * 
 * Revision 2.22  93/03/09  13:10:13  roberte
 * Fixed improper termination of rcsid control string.
 * 
 * Revision 2.21  93/03/08  17:18:07  roberte
 * setup_constants() set sp_globals.tcb0.mirror as well. A must for white writer! 
 * 
 * Revision 2.20  93/03/05  10:51:02  roberte
 * Added white writer initialization.
 * 
 * Revision 2.19  93/02/16  14:39:01  roberte
 * Removed need for function eo_get_outl_data_alt() by integrating
 * undocumented contour (type 5) into the higher level eo_proc_outl_data().
 * The data stream is very similar, and only a few tweaks were needed
 * to set pointers and to avoid use of curve_mask.  Now, Type 5
 * inner contours will be traversed in reverse direction as they should.
 * The define MAX_PTS was left in by accident, but this is HP's maximum
 * number of points in a character, and mya be useful someday.
 * 
 * Revision 2.18  93/02/04  09:02:04  roberte
 * Changed inner casts of pAuxX and pAuxY to fix7, fixed deltas on contours.
 * 
 * Revision 2.17  93/02/03  12:36:35  roberte
 * Reverted pfont to pointer, reverted references, and use pfontStruct data space.
 * 
 * Revision 2.15  93/01/29  16:05:04  roberte
 * Corrected references to eo_res_round and eo_oru_shift that APOLLO compiler
 * handles, and MS C++ compiler hates.
 * 
 * Revision 2.14  93/01/29  11:05:20  roberte
 * Changed reference to sp_globals.plaid to reflect its' move from union struct to common area.
 * 
 * Revision 2.13  93/01/27  12:13:12  roberte
 * Added PARAMS2 to some eo_do_trans() calls that were missing them.
 * 
 * Revision 2.12  93/01/21  13:34:59  roberte
 *  Added stack variable plaid and intercepts to make_char and bbox functions #if REENTRANT_ALLOC.
 * Removed an over-zealous error message call.
 * 
 * Revision 2.11  93/01/19  09:36:06  roberte
 * Replaced function prototypes with more complete versions of static functions.
 * Completed reentrant coding for the processeor, add GDECL macros to all affected functions.
 * 
 * Revision 2.10  93/01/15  16:06:10  roberte
 *   Changed all definitions and calls to function reguiring either the PARAMS1 or PARAMS2 macro for reentrancy.
 * 
 * Revision 2.9  93/01/15  15:16:34  roberte
 * Removed dead prototypes of linte() and moveto().
 * 
 * Revision 2.8  93/01/15  10:15:12  roberte
 * Corrections to globals search and replace of "fix".
 * 
 * Revision 2.7  93/01/15  10:06:54  roberte
 * "fix" changed to "fix15" throughout.
 * 
 * Revision 2.6  93/01/11  10:16:15  roberte
 * Changed references for most static variables now
 * contained in the union structure of sp_globals.
 * 
 * Revision 2.5  93/01/04  17:12:27  roberte
 * Changed all report_error calls to sp_report_error.
 * 
 * Revision 2.4  92/12/28  12:28:06  roberte
 *  Functions eo_begin_sub_char(), do_trans_width(): added return
 * typing as "fix15 
 * 
 * Revision 2.3  92/11/24  17:55:53  laurar
 * include fino.h
 * 
 * Revision 2.2  92/04/07  15:36:51  leeann
 * The function eo_get_bitmap width returned zero bitmap widths
 * for landscape and reverse landscape orientations.  If landscape,
 * use the y escapement component.
 * For very large characters generated by the PCLeo reader, the
 * interior contours may have lumpy curves, i.e., curves that are not smooth.
 * happens sporatically, only at sizes above 72 points, and only
 * in interior contours.  The solution is to check a different bit for
 * inside and outside contours.
 * 
 * Revision 2.1  91/04/04  11:25:28  mark
 * Release
 * 
 * Revision 1.2  91/04/04  11:20:51  mark
 * remove resolution specific constants
 * (unused in this module anyway).
 * 
 * Revision 1.1  91/03/01  13:14:39  mark
 * Initial revision
 * 
 * Revision 1.3  91/01/25  14:47:34  mike
 * - Rename 'setup_consts' to 'eo_setup_consts'
 *      Name conflict got us into compiler trouble.
 * - Fix integer overflow when computing bulge factor in 'eo_proc_2arc'
 *   Problem occurred when character data contained large shallow arcs.
 * 
 * Revision 1.2  90/09/18  16:08:55  mike
 * Extract curve data from alternate FAIS format.
 * No-op chars.
 * Fix bulge factor bug.
 * Remove all floating point code. Speed up by ~ 5 times.
 * Add eo_get_bitmap_width()
 * Fix 16-to-32 bit casting bugs.
 * 
 * Revision 1.1  90/07/11  12:45:00  mike
 * Initial revision
 * 
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/pcl5/hpreader/hpfnt1.c,v 7.0 97/03/17 17:42:22 shawn Release $";
#endif



/****************************** H P F O N T . C ******************************
 ********************** R E V I S I O N   H I S T O R Y **********************

 Module to read outline data from PCLEO font and convert to vectors for
 scan conversion.

 *****************************************************************************
    UPDATES BY JSC FEB 91
    jsc  Feb 28, 1991 Added test for clipping or squeezing in eo_set_specs().
                      Reports error 11 if any of these bits are set in the flags
                      word. Needed to prevent output modules using uninitialized
                      values when trying to clip.

    jsc  Feb 27, 1991 Added test to shift_16() to ensure that the rounding bit does
                      not cause overflow from 0xFFFF to 0x0000. (Thanks Mike!)

    jsc  Feb 25, 1991 Changed begin_char() to use eo_do_trans() for transforming
                      the corners of the bounding box. This is more accurate and
                      eliminates the need for do_trans_perm() which used
                      incorrect scaling of xoffset and yoffset.

    jsc  Feb 25, 1991 Added code to set the following sp_globals that are needed
                      by some or all of the Speedo output modules:
                          tcb.mirror
                          tcb.xmode
                          tcb.ymode
                          normal
                      Note that because tcb.xmode and tcb.ymode are each set to 4 
                      rnd_xmin and rnd_ymin are not actually used by output modules.

    jsc  Feb 22, 1991 Replaced eo_get_char_bbox() with a new version that 
                      intelligently transforms the corners of the bounding box. 

    jsc  Feb 19, 1991 Modified eo_proc_outl_data() to include reading character data
                      directly as in-line code. The new code tests for inside or
                      outside contours and sends the correct value to 
                      fn_begin_contour(). In addition, the direction of inside 
                      contours is reversed to a clockwise direction for compatibility 
                      with some of the output modules (particularly the screen-writer).

                      If the auxiliary point of a curve is within 0.25 pixels
                      of the chord mid-point in both X and Y dimensions, the curve is
                      treated as a single straight line. The static variable
                      eo_aux_thresh has been added to contain the precalculated
                      threshold in outline units.

                      The function eo_get_outl_data_alt() has been altered to include
                      the initialization aspects of eo_get_outl_data().
  
    jsc  Feb 11, 1991 Corrected bounding box error in begin_char (Thanks Mark!)

    jsc  Feb 11, 1991 Replaced moveto and lineto with in-line code

    jsc  Feb  8, 1991 ctm_adjust(), ctm_restore() eliminated.
                      xoff and yoff are handled by eo_set_trans().

    jsc  Feb  5, 1991 Replaced all calls to do_trans() with calls to eo_do_trans().
                      Input args changed to fix15 DWUs.
                      Output args changed to a single (point_t *).
                      do_trans() eliminated.

 1)  29-Jun-90  mby   Version 1.0

 2)  06-Jul-90  mby   Version 1.1  Add ability to read alternate FAIS format
                      (contour points only). See eo_get_outl_data_alt();

 3)  12-Jul-90  mby   Version 1.2  Extract curve data from alternate FAIS
                      format.

 4)   6-Aug-90  mby   Version 1.3  Fix bulge factor bug. Handle no-op chars.

 5)  15-Aug-90  mby   Version 1.4  Convert coordinate transformations to fix15 d
                      point arithmetic to speed up code.

 6)  21-Aug-90  mby   Version 1.5  Add a fast bulge factor check. At small/med
                      sizes skips bulge factor math & saves lots of time.

 6)  13-Sep-90  mby   Version 1.6
                      Remove all floating point in module -- routines
                        eo_proc_2arc(), split_arc(), ff (now gg), and dist()
                      Add eo_get_bitmap_width()
                      Fix 16-to-32 casting bugs
                      Fix bugs in eo_get_bbox(), setup_consts(), fix15 d-point
                        coordinate transformations.

 7)  29-Nov-90  mby   Version 2.0
                      Rename setup_consts to eo_setup_consts. Name conflict
                      with sp_setup_consts in set_spcs.c got us into compiler
                      trouble.

 7)  17-Jan-91  mby   Version 2.0
                      Fix integer overflow when computing bulge factor in
                      eo_proc_2arc(). Problem occurred when character data
                      contained large shallow arcs.

 *****************************************************************************
 ****************************************************************************/



#include "spdo_prv.h"               /* General definitions for Speedo    */

#if PROC_PCL
#include "fino.h"
#include "hp_readr.h"

#define   DEBUG   0

/* this is not a guess: */
#define MAX_PTS	398

#if DEBUG
#include <math.h>
#include <stdio.h>
#define SHOWL   printf("\n");
#define SHOWI(X) printf("    X = %d\n", X)
#define SHOWR(X) printf("    X = %f\n", X)
#define SHOWB(X) printf("    X = %s\n", X? "TRUE": "FALSE")
#else
#define SHOWL
#define SHOWI(X)
#define SHOWR(X)
#define SHOWB(X)
#endif

/* Miscellaneous constants */
#define   MAX_DWU           16383
#define   SC_DEV_MAX          128   /* Max arc-split error in 1/256 pixel */
#define   AUX_DEV_MAX          64   /* Max arc-ignore error in 1/256 pixel */
#define   EO_T_NULL             0
#define   EO_T_MOVE             1
#define   EO_T_LINE             2
#define   EO_T_ARC              3
#define   EO_T_END_CONTOUR      4
#define   EO_T_END_CHAR         5
#define   EO_T_ERROR            6

#define   ABS(X)     ( (X > 0) ? X : -X)
#define   MAX(a, b) ((a) > (b) ? (a) : (b))

typedef  struct
    {
    fix15   x;
    fix15   y;
    }   coord_t;

typedef  struct
    {
    fix31   x;
    fix31   y;
    }   lcoord_t;

typedef  struct
    {
    ufix8  *pchar;                  /* pointer to character data */
    ufix8  *ptree;                  /* pointer to beginning of contour tree */
    ufix8  *pcont;                  /* pointer to beginning of each contour */
    fix15   nr_contour;             /* # contours in character */
    fix15   contour_index;          /* index of current contour */
    fix15   nr_cp, nr_ap;           /* # contour, auxiliary pts in current contour */
    fix15   c_offset, a_offset;     /* offset to current point from beginning of contour */
    fix15   nr_allp;                /* total # of all pts in character + 1 */
    btsbool end_contour;            /* tells us we're at the end */
    }   ospecs_t;

/***** STATIC VARIABLES *****/
static ufix8  eo_orush_arr[2][13] = {
                  { 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 },
                  { 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1 }  };

#ifdef OLDWAY
static eospecs_t  eo_specs;         /* specs for scan conversion */
static ctm_t  eo_ctm_perm;          /* ditto; in compound chars this matrix remains
                                       unaffected by the character offset */
static fix15  eo_prod_shift, eo_prod_round;
static fix15  eo_oru_shift, eo_oru_round;
static fix15  eo_res_round;         /* rounding term when dividing by ORU per em */
static fix15  eo_aux_thresh;        /* Max dx or dy for aux point for which straight line is ok */
static fix31  eo_spl_thresh;        /* maximum arc splitting error in ORU's;
                                       stored in 16.16 fix15 d point */
static ufix16 eo_eff_lpm;           /* "effective" lines per em, rounded up */
#endif /* OLDWAY */

#ifdef FOR_YOUR_INFORMATION
static real   ggvalue[18] = {                           /* ordinate =     */
                 .000000, .031220, .062258, .092940,   /* 0, 1, 2, 3/16  */
                 .123106, .152611, .181335, .209178,   /* 4, 5, 6, 7/16  */
                 .236068, .261951, .286796, .310590,   /* 8, 9, 10, 11/16 */
                 .333333, .355041, .375735, .395447,   /* 12, 13, 14, 15/16 */
                 .414214, .480625 };                   /* 1.0, 1.25 */
#endif /* FOR_YOUR_INFORMATION */
    /* for the function tan(x) * (sin^2(x) + cos(x) - 1) / sin^2(x) */
static ufix16 ggivalue[18] = {                          /* ordinate =     */
                      0,  4092,  8160, 12182,           /* 0, 1, 2, 3/16  */
                  16136, 20003, 23768, 27417,           /* 4, 5, 6, 7/16  */
                  30942, 34334, 37591, 40710,           /* 8, 9, 10, 11/16 */
                  43691, 46536, 49248, 51832,           /* 12, 13, 14, 15/16 */
                  54292, 62996 };                       /* 1.0, 1.25 */

static fix15  sqrt_table[17] = {                        /* ordinate =     */
                      0,  4034,  7951, 11760,           /* 1.00, 1.06, 1.13, 1.19 */
                  15471, 19090, 22624, 26078,           /* 1.25, 1.31, 1.38, 1.44 */
                  29458, 32768, 36013, 39196,           /* 1.50, 1.56, 1.63, 1.69 */
                  42320, 45389, 48406, 51373,           /* 1.75, 1.81, 1.88, 1.94 */
                  54292 };                              /* 2.00 */
static ufix16 sqrt_table_d = 0x1000;
static ufix16 sqrt_table_m = 0x0FFF;
static ufix16 sqrt_table_r = 0x0800;

#ifdef DEBUG_ALT
static ufix16 eo_index;
#endif

/***** STATIC FUNCTIONS *****/
/** static to hpfnt1.c **/

LOCAL_PROTO btsbool eo_setup_consts(SPGLOBPTR1);
LOCAL_PROTO btsbool eo_proc_outl_data(SPGLOBPTR2 ufix8 *pChar, fix15 *errno);

#ifdef DEFUNCT
LOCAL_PROTO fix15 eo_get_outl_data_alt(ospecs_t *cdata, coord_t *contp, lcoord_t *auxp);
#endif

LOCAL_PROTO btsbool eo_proc_2arc(SPGLOBPTR2 fix15 xa, fix15 ya, fix31 xb, fix31 yb, fix15 xc, fix15 yc);
LOCAL_PROTO btsbool split_arc(SPGLOBPTR2 fix31 x0, fix31 y0, fix31 x1, fix31 y1, fix15 beta);
LOCAL_PROTO fix15 bsign(fix15 x0, fix15 y0, fix15 x1, fix15 y1, fix15 x2, fix15 y2);
LOCAL_PROTO fix31 gg(fix31 xx);
LOCAL_PROTO fix31 dist(fix31 x0, fix31 y0);
LOCAL_PROTO fix15 eo_inq_compchar(ufix8 *pchar);
LOCAL_PROTO btsbool eo_get_component(ufix8 *pchar, fix15 nn, fix15 *index, fix15 *xoff, fix15 *yoff);
LOCAL_PROTO fix15 eo_get_swidth(ufix8 *pchar);
LOCAL_PROTO void eo_get_bbox(SPGLOBPTR2 eo_char_t *pchar, fix15 *xmin, fix15 *ymin, fix15 *xmax, fix15 *ymax);
LOCAL_PROTO btsbool eo_begin_char(SPGLOBPTR2 eo_char_t *pchar);
LOCAL_PROTO fix15 eo_begin_sub_char(SPGLOBPTR2 eo_char_t *pchar_sub, fix15 comp_xoff, fix15 comp_yoff);
LOCAL_PROTO fix15 do_trans_width(SPGLOBPTR2 fix31 xin, fix31 *xout, fix31 *yout);
LOCAL_PROTO fix15 shift_left_32(fix31 arg, ufix32 *out);
LOCAL_PROTO fix15 bit_count(fix31 arg);
LOCAL_PROTO fix15 shift_16(fix31 arg, ufix32 *out);
LOCAL_PROTO fix31 mult_122(fix15 args, fix31 argl);


FUNCTION fix31 eo_get_char_width(SPGLOBPTR2 ufix16 char_index)

/*  Returns: character set width in 1/65536 em.
    eo_set_specs _m_u_s_t_ be called before this function */

{
ufix32	width;	/* set width		*/
ufix16	emRes;	/* em resolution	*/

width = (ufix32)eo_get_char_width_orus(PARAMS2 char_index, &emRes);
width = ((width << 16) + sp_globals.processor.pcl.eo_res_round) / emRes;
return (width);
}

FUNCTION fix31 eo_get_char_width_orus(SPGLOBPTR2 ufix16 char_index, ufix16 *emResolution)

/*  Returns: character set width in 1/65536 em.
    eo_set_specs _m_u_s_t_ be called before this function */

{
eo_char_t  *pchar;
ufix32      width;

/* always set emResolution first: */
*emResolution = sp_globals.processor.pcl.eo_specs.pfont->resolution;

if ((pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, char_index)) == BTSNULL)
    return((fix31)0);

if (!sp_globals.specs_valid  ||  sp_globals.processor.pcl.eo_specs.pfont->resolution <= 0)
    {
    sp_report_error(PARAMS2 EEO_WIDTH_SPECS_NOTVAL);
    return((fix31)0);
    }

width = (fix31)eo_get_swidth(pchar);
return(width);
}


FUNCTION fix15 eo_get_bitmap_width(SPGLOBPTR2 ufix16 char_index)

/*  Returns: character set width rounded to nearest pixel.
    eo_set_specs _m_u_s_t_ be called before this function */

{
eo_char_t  *pchar;
fix31       wx, wy;


if ((pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, char_index)) == BTSNULL)
    return((fix31)0);

if (!sp_globals.specs_valid  ||  sp_globals.processor.pcl.eo_specs.pfont->resolution <= 0)
    {
    sp_report_error(PARAMS2 EEO_WIDTH_SPECS_NOTVAL);
    return((fix31)0);
    }

do_trans_width(PARAMS2 ((fix31)eo_get_swidth(pchar)) << 16, &wx, &wy);

if (wx == 0) /**** check for landscape orientations ****/
    wx = wy; /**** and use y escapement component   ****/
if (wx < 0)
    wx = -wx;

return((fix15)((wx + 0x8000) >> 16));
}


#if INCL_METRICS
FUNCTION btsbool eo_get_char_bbox(SPGLOBPTR2 ufix16 char_index, bbox_t *pBBox)
/*
 *  Returns:
 *      TRUE if successful
 *      FALSE if char_index does not exit in font
 *      Calls sp_report_error(PARAMS2 ) if:
 *          specs not valid
 *          Character not in font
 *          Compound character is nested
 *          Compound character component not in font
 *          Compound character has illegal component index
 *          Unrecognizable character data
 *          
 *  Output: character bounding box in 1/65536 pixels
 */
{
fix15     ii, jj;
fix15     errno;          /* Error number */
fix15   comp_index;     /* Index of component of compound character */
fix15   comp_xoff;      /* X offset of compound character component (DWUs) */
fix15   comp_yoff;      /* Y offset of compound character component (DWUs) */
fix15     nn;             /* Number of components in compound character */
eo_char_t  *pchar;      /* Pointer to current character data */
eo_char_t  *pchar_sub;  /* Pointer to sub-character data */
fix15   xmin;           /* Minimum X coord in char or sub-char (DWUs) */
fix15   xmax;           /* Maximum X coord in char or sub-char (DWUs) */
fix15   ymin;           /* Minimum Y coord in char or sub-char (DWUs) */
fix15   ymax;           /* Maximum Y coord in char or sub-char (DWUs) */
fix15   xx;             /* X coord of currently selected point (DWUs) */
fix15   yy;             /* Y coord of currently selected point (DWUs) */
point_t P;              /* Intelligently transformed current point (sub-pixels) */

/* Check that set_specs() has been called */
if (!sp_globals.specs_valid  ||  
    sp_globals.processor.pcl.eo_specs.pfont->resolution <= 0)
    {
    errno = EEO_WIDTH_SPECS_NOTVAL;
    goto error;
    }

/* Get address of character */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, char_index);
if (pchar == BTSNULL)
    {
    errno = EEO_DOES_NOT_EXIST;
    goto error;
    }

nn = eo_inq_compchar(pchar);            /* Number of elements in character */
if (nn == 0)                            /* Simple character? */
    { 
    comp_xoff = 0;
    comp_yoff = 0;
    eo_set_trans(PARAMS2 pchar, comp_xoff, comp_yoff); /* Set up intelligent transformation */
    eo_get_bbox(PARAMS2 pchar, &xmin, &ymin, &xmax, &ymax);
    for (ii = 0; ii < 4; ii++)
        {
        switch (ii)
            {
        case 0:                         /* Select lower left corner */
            xx = xmin;
            yy = ymin;
            break;

        case 1:                         /* Select top left corner */
            xx = xmin;
            yy = ymax;
            break;

        case 2:                         /* Select lower right corner */
            xx = xmax;
            yy = ymin;
            break;

        case 3:                         /* Select top right corner */
            xx = xmax;
            yy = ymax;
            break;
            }

        eo_do_trans(PARAMS2 xx, yy, &P);        /* Transform selected corner */

        if (ii == 0)                    /* First corner of character? */
            {
            pBBox->xmin = P.x;
            pBBox->xmax = P.x;
            pBBox->ymin = P.y;
            pBBox->ymax = P.y;
            }
        else                            /* Not first corner of character? */
            {
            if (P.x < pBBox->xmin)
                pBBox->xmin = P.x;
            if (P.x > pBBox->xmax)
                pBBox->xmax = P.x;
            if (P.y < pBBox->ymin)
                pBBox->ymin = P.y;
            if (P.y > pBBox->ymax)
                pBBox->ymax = P.y;
            }
        }

    nn = 16 - sp_globals.pixshift;      /* Shift from pixshift to 16.16 */
    pBBox->xmin <<= nn;
    pBBox->ymin <<= nn;
    pBBox->xmax <<= nn;
    pBBox->ymax <<= nn;
    return TRUE;
    }

if (nn > 0)                             /* Compound character? */
    {
    for (ii = 0; ii < nn; ii++)         /* For each component... */
        {
        if (eo_get_component(pchar, ii, &comp_index, &comp_xoff, &comp_yoff))
            {
            pchar_sub = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, comp_index);
            if (pchar_sub != BTSNULL)      /* Component available? */
                {
                if (eo_inq_compchar(pchar_sub) == 0) /* Component is simple? */
                    {
                    eo_set_trans(PARAMS2 pchar_sub, comp_xoff, comp_yoff); /* Set up intelligent transformation */
                    eo_get_bbox(PARAMS2 pchar_sub, &xmin, &ymin, &xmax, &ymax);
                    for (jj = 0; jj < 4; jj++)
                        {
                        switch (jj)
                            {
                        case 0:             /* Select lower left corner */
                            xx = xmin;
                            yy = ymin;
                            break;
                   
                        case 1:             /* Select top left corner */
                            xx = xmin;
                            yy = ymax;
                            break;

                        case 2:             /* Select lower right corner */
                            xx = xmax;
                            yy = ymin;
                            break;

                        case 3:             /* Select top right corner */
                            xx = xmax;
                            yy = ymax;
                            break;
                            }
                        eo_do_trans(PARAMS2 xx, yy, &P); /* Transform selected corner */

                        if ((ii == 0) && (jj == 0)) /* First corner of first component? */
                            {
                            pBBox->xmin = P.x;
                            pBBox->xmax = P.x;
                            pBBox->ymin = P.y;
                            pBBox->ymax = P.y;
                            }
                        else                /* Not first corner of first component? */
                            {
                            if (P.x < pBBox->xmin)
                                pBBox->xmin = P.x;
                            if (P.x > pBBox->xmax)
                                pBBox->xmax = P.x;
                            if (P.y < pBBox->ymin)
                                pBBox->ymin = P.y;
                            if (P.y > pBBox->ymax)
                                pBBox->ymax = P.y;
                            }
                        }
                    }
                else                    /* Component is compound? */
                    {
                    errno = EEO_NESTED_COMP_CHAR;
                    goto error;
                    }
                }
            else                        /* Component not available? */
                {
                errno = EEO_ILL_COMP_CHAR_REF;
                goto error;
                }
            }
        else                            /* Cannot read component specification? */
            {
            errno = EEO_ILL_COMP_SUBCHAR;
            goto error;
            }
        }
    nn = 16 - sp_globals.pixshift;      /* Shift from pixshift to 16.16 */
    pBBox->xmin <<= nn;
    pBBox->ymin <<= nn;
    pBBox->xmax <<= nn;
    pBBox->ymax <<= nn;
    return TRUE; 
    }

errno = EEO_NOT_A_PCL5_CHAR;

error:
sp_report_error (PARAMS2 (fix15)errno);
return FALSE;
}
#endif /* INCL_METRICS */


FUNCTION btsbool eo_set_specs(SPGLOBPTR2 eospecs_t *specs)
{
fix31  xx_mult;
fix31  xy_mult;
fix31  yx_mult;
fix31  yy_mult;

sp_globals.specs_valid = FALSE;                    /* Flag specs not valid */

#ifdef OLDWAY
sp_globals.processor.pcl.eo_specs.pfont = specs->pfont;
#else
sp_globals.processor.pcl.eo_specs.pfont = &sp_globals.processor.pcl.pfontStruct;
#endif

sp_globals.processor.pcl.eo_specs.xxmult = specs->xxmult;
sp_globals.processor.pcl.eo_specs.xymult = specs->xymult;
sp_globals.processor.pcl.eo_specs.xoffset = specs->xoffset;
sp_globals.processor.pcl.eo_specs.yxmult = specs->yxmult;
sp_globals.processor.pcl.eo_specs.yymult = specs->yymult;
sp_globals.processor.pcl.eo_specs.yoffset = specs->yoffset;
sp_globals.processor.pcl.eo_specs.flags = specs->flags;
sp_globals.processor.pcl.eo_specs.out_info = specs->out_info;
sp_globals.processor.pcl.eo_specs.pfont->resolution = specs->pfont->resolution;
sp_globals.processor.pcl.eo_specs.pfont->unique_id = specs->pfont->unique_id;
sp_globals.processor.pcl.eo_specs.pfont->eofont = specs->pfont->eofont;
sp_globals.pspecs = &sp_globals.specs;
sp_globals.pspecs->flags = specs->flags;
sp_globals.processor.pcl.eo_res_round = sp_globals.processor.pcl.eo_specs.pfont->resolution >> 1;

/* There is no way of knowing if pfont->eofont is incorrect, since the
   internal data structure is not defined. */

sp_globals.processor.pcl.eo_eff_lpm = 
    ((ABS(sp_globals.processor.pcl.eo_specs.xxmult) + 
     ABS(sp_globals.processor.pcl.eo_specs.xymult) +
     ABS(sp_globals.processor.pcl.eo_specs.yxmult) + 
     ABS(sp_globals.processor.pcl.eo_specs.yymult)) >> 17) + 1;
sp_globals.processor.pcl.eo_aux_thresh = (((fix31)sp_globals.processor.pcl.eo_specs.pfont->resolution * AUX_DEV_MAX) / sp_globals.processor.pcl.eo_eff_lpm) >> 8;
sp_globals.processor.pcl.eo_spl_thresh = ((fix31)sp_globals.processor.pcl.eo_specs.pfont->resolution << 16) / sp_globals.processor.pcl.eo_eff_lpm / 256 * SC_DEV_MAX;

/* Set transformation control block data that may be used by output module */
xx_mult = sp_globals.processor.pcl.eo_specs.xxmult >> 16;
xy_mult = sp_globals.processor.pcl.eo_specs.xymult >> 16;
yx_mult = sp_globals.processor.pcl.eo_specs.yxmult >> 16;
yy_mult = sp_globals.processor.pcl.eo_specs.yymult >> 16;
sp_globals.tcb.mirror = sp_globals.tcb0.mirror = 
    (((xx_mult * yy_mult) - (xy_mult * yx_mult)) < 0)?
     -1:
     1;
sp_globals.tcb.xmode = 4;
sp_globals.tcb.ymode = 4;
sp_globals.normal = FALSE;

/* Set up sliding point constants */
if (!eo_setup_consts(PARAMS1))
    {
    sp_report_error(PARAMS2 EEO_SPECS_OUT_OF_RANGE);   /* Requested specs out of range */
    return FALSE;
    }

sp_globals.output_mode = (sp_globals.processor.pcl.eo_specs.flags & 0x0007);

#if INCL_USEROUT
if (!sp_init_userout(sp_globals.pspecs))
#endif

switch (sp_globals.output_mode)
    {
#if INCL_BLACK
case MODE_BLACK:                        /* Black writer output mode */
    sp_globals.init_out         = sp_init_black;
    sp_globals.begin_char       = sp_begin_char_black;
    sp_globals.begin_sub_char   = sp_begin_sub_char_out;
    sp_globals.begin_contour    = sp_begin_contour_black;
    sp_globals.curve            = sp_curve_out;
    sp_globals.line             = sp_line_black;
    sp_globals.end_contour      = sp_end_contour_out;
    sp_globals.end_sub_char     = sp_end_sub_char_out;
    sp_globals.end_char         = sp_end_char_black;
    break;
#endif

#if INCL_SCREEN
case MODE_SCREEN:                       /* Screen writer output mode */
    sp_globals.init_out         = sp_init_screen;
    sp_globals.begin_char       = sp_begin_char_screen;
    sp_globals.begin_sub_char   = sp_begin_sub_char_out;
    sp_globals.begin_contour    = sp_begin_contour_screen;
    sp_globals.curve            = sp_curve_screen;
    sp_globals.line             = sp_line_screen;
    sp_globals.end_contour      = sp_end_contour_screen;
    sp_globals.end_sub_char     = sp_end_sub_char_out;
    sp_globals.end_char         = sp_end_char_screen;
    break;
#endif

#if INCL_OUTLINE
case MODE_OUTLINE:                      /* Outline output mode */
    sp_globals.init_out         = sp_init_outline;
    sp_globals.begin_char       = sp_begin_char_outline;
    sp_globals.begin_sub_char   = sp_begin_sub_char_outline;
    sp_globals.begin_contour    = sp_begin_contour_outline;
    sp_globals.curve            = sp_curve_outline;
    sp_globals.line             = sp_line_outline;
    sp_globals.end_contour      = sp_end_contour_outline;
    sp_globals.end_sub_char     = sp_end_sub_char_outline;
    sp_globals.end_char         = sp_end_char_outline;
    break;
#endif

#if INCL_2D
case MODE_2D:                           /* 2D output mode */
    sp_globals.init_out         = sp_init_2d;
    sp_globals.begin_char       = sp_begin_char_2d;
    sp_globals.begin_sub_char   = sp_begin_sub_char_out;
    sp_globals.begin_contour    = sp_begin_contour_2d;
    sp_globals.curve            = sp_curve_out;
    sp_globals.line             = sp_line_2d;
    sp_globals.end_contour      = sp_end_contour_out;
    sp_globals.end_sub_char     = sp_end_sub_char_out;
    sp_globals.end_char         = sp_end_char_2d;
    break;
#endif

#if INCL_WHITE
case MODE_WHITE:                        /* Output mode White writer */
   sp_globals.init_out          = sp_init_white;
   sp_globals.begin_char        = sp_begin_char_white;
   sp_globals.begin_sub_char    = sp_begin_sub_char_out;
   sp_globals.begin_contour     = sp_begin_contour_white;
   sp_globals.curve             = sp_curve_out;
   sp_globals.line              = sp_line_white;
   sp_globals.end_contour       = sp_end_contour_white;
   sp_globals.end_sub_char      = sp_end_sub_char_out;
   sp_globals.end_char          = sp_end_char_white;
   break;
#endif

#if INCL_QUICK
case MODE_QUICK:                        /* Output mode Quick writer */
    sp_globals.init_out         = sp_init_quick;
    sp_globals.begin_char       = sp_begin_char_quick;
    sp_globals.begin_sub_char   = sp_begin_sub_char_quick;
    sp_globals.begin_contour    = sp_begin_contour_quick;
    sp_globals.curve            = sp_curve_quick;
    sp_globals.line             = sp_line_quick;
    sp_globals.end_contour      = sp_end_contour_quick;
    sp_globals.end_sub_char     = sp_end_sub_char_quick;
    sp_globals.end_char         = sp_end_char_quick;
    break;
#endif

#if INCL_GRAY
case MODE_GRAY:                         /* Output mode Gray writer */
    sp_globals.init_out         = sp_init_gray;
    sp_globals.begin_char       = sp_begin_char_gray;
    sp_globals.begin_sub_char   = sp_begin_sub_char_gray;
    sp_globals.begin_contour    = sp_begin_contour_gray;
    sp_globals.curve            = sp_curve_gray;
    sp_globals.line             = sp_line_gray;
    sp_globals.end_contour      = sp_end_contour_gray;
    sp_globals.end_sub_char     = sp_end_sub_char_gray;
    sp_globals.end_char         = sp_end_char_gray;
    break;
#endif


default:
    sp_report_error(PARAMS2 8);                    /* Unsupported mode requested */
    return FALSE;
    }

if (!fn_init_out(sp_globals.pspecs))
    {
    sp_report_error(PARAMS2 5);                    /* Specs not compatible with output module */
    return FALSE;
    }

if (specs->flags & 
    (SQUEEZE_LEFT |
     SQUEEZE_RIGHT |
     SQUEEZE_TOP |
     SQUEEZE_BOTTOM |
     CLIP_LEFT |
     CLIP_RIGHT |
     CLIP_TOP |
     CLIP_BOTTOM))                      /* Clipping or squeezing requested? */
    {
    sp_report_error(PARAMS2 11);                   /* Unsupported clipping or squeezing requested */
    return FALSE;
    }

if (!eo_init_trans(PARAMS2 specs))              /* Initialize intelligent transformation */
    {
    sp_report_error(PARAMS2 EEO_SPECS_OUT_OF_RANGE); /* Requested specs out of range */
    return FALSE;
    }

sp_globals.specs_valid = TRUE;
return TRUE;
}


FUNCTION LOCAL_PROTO btsbool eo_setup_consts(SPGLOBPTR1)
/*
 * Sets the following constants used for fix15 d point arithmetic:
 *      sp_globals.multshift    multipliers and products; range is 14 to 8
 ***    sp_globals.pixshift     pixels: range is 0 to 8
 ***    sp_globals.poshift      left shift from pixel to output format
 *      sp_globals.mpshift      shift from product to sub-pixels (sp_globals.multshift - sp_globals.pixshift)
 *      sp_globals.multrnd      rounding for products
 ***    sp_globals.pixrnd       rounding for pixels
 *      sp_globals.mprnd        rounding for sub-pixels
 ***    sp_globals.onepix       1 pixel in shifted pixel units
 ***    sp_globals.pixfix15       mask to eliminate fractional bits of shifted pixels
 *      sp_globals.depth_adj    curve splitting depth adjustment
 * Returns FALSE if specs are out of range
 */

{
fix15  aa, bb;
ufix16 ii;
fix15  maxpix;
fix15  multshift, multround;
fix15  rpix;
fix15  xmin, ymin, xmax, ymax;
fix31  xxa, xya, xoa, yxa, yya, yoa;

if (sp_globals.processor.pcl.eo_specs.pfont->resolution <= 0)    /* ORU per em valid? */
    return (FALSE);

xmin = -MAX_DWU;                        /* maximum allowable bounding box (the  */
ymin = -MAX_DWU;                        /* actual fontwide bbox is unavailable) */
xmax =  MAX_DWU;
ymax =  MAX_DWU;

xxa = ((sp_globals.processor.pcl.eo_specs.xxmult + 0x8000) >> 16);      
xya = ((sp_globals.processor.pcl.eo_specs.xymult + 0x8000) >> 16);
xoa = ((sp_globals.processor.pcl.eo_specs.xoffset + 0x8000) >> 16);
yxa = ((sp_globals.processor.pcl.eo_specs.yxmult + 0x8000) >> 16);
yya = ((sp_globals.processor.pcl.eo_specs.yymult + 0x8000) >> 16);
yoa = ((sp_globals.processor.pcl.eo_specs.yoffset + 0x8000) >> 16);

maxpix = 0;                             /* find maximum distance away from origin in pixels */

for (aa = xmin; ; aa += (xmax - xmin))
    {
    for (bb = ymin; ; bb += (ymax - ymin))
        {
        rpix = (xxa * aa + xya * bb)/sp_globals.processor.pcl.eo_specs.pfont->resolution + xoa;
        rpix = (rpix > 0 ? rpix : -rpix);
        if (++rpix > maxpix)
            maxpix = rpix;
        rpix = (yxa * aa + yya * bb)/sp_globals.processor.pcl.eo_specs.pfont->resolution + yoa;
        rpix = (rpix > 0 ? rpix : -rpix);
        if (++rpix > maxpix)
            maxpix = rpix;
        if (bb == ymax)  break;
        }
    if (aa == xmax)  break;
    }

ii = 16384;                             /* Max pixels <= 32768, 16384, 8192, ... pixels? */
sp_globals.pixshift = 0;

while (maxpix <= ii)                    /* pixshift range is 0 to 8 */
    {
    sp_globals.pixshift++;
    ii >>= 1;
    if (sp_globals.pixshift == 8)
        break;
    }

sp_globals.poshift = 16 - sp_globals.pixshift;
sp_globals.onepix = (fix15)1 << sp_globals.pixshift;
sp_globals.pixrnd = sp_globals.onepix >> 1;
sp_globals.pixfix = -sp_globals.onepix;

aa=2; ii=8;
while (sp_globals.processor.pcl.eo_eff_lpm > ii)
    {
    aa++;
    ii <<= 1;
    if (ii == 0x8000)                   /* up to 32K eff lpm */
        break;
    }

bb=30; ii=0x4000;
while (sp_globals.processor.pcl.eo_specs.pfont->resolution < ii)
    {                                   /* from 16K ORU per em down */
    bb--;
    ii >>= 1;
    }

sp_globals.processor.pcl.eo_prod_shift = (bb - aa) - 16;
if (sp_globals.processor.pcl.eo_prod_shift > 0)
    sp_globals.processor.pcl.eo_prod_round = (fix15)1 << (sp_globals.processor.pcl.eo_prod_shift - 1);
else
    sp_globals.processor.pcl.eo_prod_round = 0;

if (bb <= 25)                           /* < 1023 ORU per em */
    sp_globals.processor.pcl.eo_oru_shift = 4;
else if (bb <= 26)                      /* 1024 to 2047 ORU per em */
    {
    if (aa <= 8)
        sp_globals.processor.pcl.eo_oru_shift = 4;
    else
        sp_globals.processor.pcl.eo_oru_shift = 3;
    }
else if (bb <= 27)                      /* 2048 to 4095 ORU per em */
    {
    if (aa <= 5)
        sp_globals.processor.pcl.eo_oru_shift = 4;
    else
        sp_globals.processor.pcl.eo_oru_shift = 3;
    }
else if (bb == 28)                      /* 4096 to 8191 ORU per em */
    sp_globals.processor.pcl.eo_oru_shift = eo_orush_arr[0][aa-2];
else if (bb >= 29)                      /* >= 8192 ORU per em */
    sp_globals.processor.pcl.eo_oru_shift = eo_orush_arr[1][aa-2];

multshift = sp_globals.processor.pcl.eo_prod_shift - sp_globals.processor.pcl.eo_oru_shift;
sp_globals.processor.pcl.eo_oru_shift = 16 - sp_globals.processor.pcl.eo_oru_shift;
sp_globals.processor.pcl.eo_oru_round = (fix15)1 << (sp_globals.processor.pcl.eo_oru_shift - 1);

if (multshift >= 0)
    {
    sp_globals.processor.pcl.eo_ctm_perm.xxmult = ((sp_globals.processor.pcl.eo_specs.xxmult << multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.xymult = ((sp_globals.processor.pcl.eo_specs.xymult << multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.yxmult = ((sp_globals.processor.pcl.eo_specs.yxmult << multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.yymult = ((sp_globals.processor.pcl.eo_specs.yymult << multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    }
else
    {
    multshift = -multshift;
    multround = (fix15)1 << (multshift - 1);
    sp_globals.processor.pcl.eo_ctm_perm.xxmult = (((sp_globals.processor.pcl.eo_specs.xxmult + multround) >> multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.xymult = (((sp_globals.processor.pcl.eo_specs.xymult + multround) >> multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.yxmult = (((sp_globals.processor.pcl.eo_specs.yxmult + multround) >> multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    sp_globals.processor.pcl.eo_ctm_perm.yymult = (((sp_globals.processor.pcl.eo_specs.yymult + multround) >> multshift) + sp_globals.processor.pcl.eo_res_round) / sp_globals.processor.pcl.eo_specs.pfont->resolution;
    }

sp_globals.processor.pcl.eo_ctm_perm.xoffset = sp_globals.processor.pcl.eo_specs.xoffset;
sp_globals.processor.pcl.eo_ctm_perm.yoffset = sp_globals.processor.pcl.eo_specs.yoffset;

return TRUE;
}



GLOBAL_PROTO char *eo_get_char_addr(PROTO_DECL2 void *eofont, ufix16 index);

FUNCTION btsbool eo_make_char(SPGLOBPTR2 ufix16 index)

/*  Returns: TRUE - output completed, FALSE - otherwise
    This function generates a character from a PCL5 downloaded font. */

{
fix15   ii, nn;
fix15   comp_index;    /* Index of compound character component */
fix15   comp_xoff;     /* X origin of compound character component */
fix15   comp_yoff;     /* Y origin of compound character component */
fix15   errno;

eo_char_t  *pchar;
eo_char_t  *pchar_sub;

#if REENTRANT_ALLOC
#if INCL_BLACK || INCL_SCREEN || INCL_2D || INCL_WHITE || INCL_QUICK || INCL_GRAY
intercepts_t intercepts;
sp_globals.intercepts = &intercepts;
#endif
#endif

/* get address of character */
#ifdef DEBUG_ALT
eo_index = index;
#endif

if ((pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 (void *)sp_globals.processor.pcl.eo_specs.pfont->eofont, index)) == BTSNULL)
    {
    errno = EEO_DOES_NOT_EXIST;
    goto mcerror;
    }

comp_xoff = 0;
comp_yoff = 0;
nn = eo_inq_compchar(pchar);
if (nn == 0)                            /* Simple character? */
    {
    eo_set_trans(PARAMS2 pchar, comp_xoff, comp_yoff); /* Set up intelligent transformation */
    eo_begin_char(PARAMS2 pchar);               /* Output character prologue */
    do  {
        if (!eo_proc_outl_data(PARAMS2 pchar, &errno)) /* Process outline data; error? */
            goto mcerror;
        }   while (!fn_end_char());     /* Repeat for banding required? */
    return TRUE;
    }
else if (nn > 0)                        /* Compound character? */
    {
    pchar_sub = BTSNULL;
    eo_set_trans(PARAMS2 pchar_sub, comp_xoff, comp_yoff); /* Set up linear transformation */
    eo_begin_char(PARAMS2 pchar);               /* Output character prologue */
    do  {
        for (ii = 0; ii < nn; ii++)     /* For each component of character... */
            {
            if (eo_get_component(pchar, ii, &comp_index, &comp_xoff, &comp_yoff)) /* Get component origin */
                {
                pchar_sub = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, comp_index);
                if (pchar_sub != BTSNULL)  /* Valid component? */
                    {
                    if (eo_inq_compchar(pchar_sub) == 0) /* Component is simple character? */
                        {
                        eo_set_trans(PARAMS2 pchar_sub, comp_xoff, comp_yoff); /* Set up intelligent transformation */
                        eo_begin_sub_char(PARAMS2 pchar_sub, comp_xoff, comp_yoff);
                        if (!eo_proc_outl_data(PARAMS2 pchar_sub, &errno))
                            {
                            goto mcerror;
                            }
                        fn_end_sub_char();
                        }
                    else                /* Component is compound character? */
                        {
                        errno = EEO_NESTED_COMP_CHAR;
                        goto mcerror;
                        }
                    }
                else                    /* Invalid component character? */
                    {
                    errno = EEO_ILL_COMP_CHAR_REF;
                    goto mcerror;
                    }
                }
            else                        /* Cannot access component character? */
                {
                errno = EEO_ILL_COMP_SUBCHAR;
                goto mcerror;
                }
            }
        }   while (!fn_end_char());     /* if output needs more banding, repeat */
    return(TRUE);                       /*   the whole process */
    }
else
    {
    errno = EEO_NOT_A_PCL5_CHAR;
    goto mcerror;
    }

mcerror:

/* sp_report_error (PARAMS2 (fix15)errno); */
return(FALSE);
}



FUNCTION LOCAL_PROTO btsbool eo_proc_outl_data(SPGLOBPTR2 ufix8 *pChar, fix15 *errno)
/*  
 * Processes all contours in the specified character.
 * Returns TRUE if no error, else FALSE
 * 'errno' returns the error code.
 * Outside curves are processed counterclockwise
 * Inside curves are processed clocksise
 */
{
fix15    ii, jj;
ufix8   *pointer;
ufix16   ContourTreeOffset;     /* Offset to char outline data contour tree */
ufix8   *pCharOutlineData;      /* Pointer to origin of char outl data contour tree */
ufix16   ContourFlag;           /* Contour flag indicating alt format */
fix15    nContour;              /* Number of contours in character */
ufix8   *pContourTreeData;      /* Pointer to node data for current contour */
ufix16   XYCoordDataOffset;     /* Offset to XY coordinate data for current contour */
fix15    nPoint;                /* Number of points in current contour */
fix15    nAuxPt;                /* Number of aux points in current contour */
ufix8   *pXOrg;                 /* Pointer to X coord of first point */
ufix8   *pYOrg;                 /* Pointer to Y coord of first point */
ufix8   *pAuxXOrg;              /* Pointer to X coord of first aux point */
ufix8   *pAuxYOrg;              /* Pointer to Y coord of first aux point */
ufix8   *pX;                    /* Pointer to X coord of current point */
ufix8   *pY;                    /* Pointer to Y coord of current point */
ufix8   *pAuxX;                 /* Pointer to X coord of current aux point */
ufix8   *pAuxY;                 /* Pointer to Y coord of current aux point */
fix15    incr;                  /* Increment for point pointers */
fix15    aux_incr;              /* Increment for auxiliary point pointers */
ufix16   curve_mask;		/* you have to check a different bit, working
				   backwards */
ufix16   rawX;                  /* Raw X coord data for current point */
ufix16   rawY;                  /* Raw Y coord data for current point */
fix15    X = 0;                 /* X coord of current point */
fix15    Y = 0;                 /* Y coord of current point */
fix15    dX;                    /* X coord increment for current aux point */
fix15    dY;                    /* Y coord increment for current aux point */
fix15    auxX;                  /* X coord of current auxiliary point */
fix15    auxY;                  /* Y coord of current auxiliary point */
fix15    oldX;                  /* X coord of preceding point */
fix15    oldY;                  /* Y coord of preceding point */

lcoord_t auxp;
coord_t  contp;
ospecs_t cdata;
fix15    curr_pos_x, curr_pos_y;
fix15      ptype;
btsbool  outside = TRUE;
point_t  P;

pointer = pChar + 10;
ContourTreeOffset = GET_WORD(pointer);  /* Read offset to character outline data */
if (ContourTreeOffset == 0xffff)        /* No contours in this character? */
    return TRUE;

pCharOutlineData = pChar + 4 + ContourTreeOffset;
pointer = pCharOutlineData + 2;
ContourFlag = GET_WORD(pointer);
#ifdef DEFUNCT
if (ContourFlag != 3)                   /* Standard char data format? */
    {
#endif /* DEFUNCT */
	if (ContourFlag == 3)
		{/* undocumented format? */
    	if (GET_WORD((pChar + 10)) == 0xFFFF)
        	return (FALSE);         /* a no-op character! */
		pCharOutlineData = pChar + GET_WORD((pChar + 10)) + 4;
		}
    pointer = pCharOutlineData;
    nContour = GET_WORD(pointer);       /* Number of contours */
    pContourTreeData = pCharOutlineData + 4;
    for (ii = 0; ii < nContour; ii++, pContourTreeData += 8) /* For each contour... */
        {
        pointer = pContourTreeData;
        outside = (btsbool)pContourTreeData[3]; /* Read polarity of contour data */
		if (ContourFlag == 3)
			{/* undocumented format? */
			XYCoordDataOffset = 6;
			pointer = pChar + GET_WORD((pCharOutlineData + 8*ii + 4)) + 4;
    		nPoint = GET_WORD(pointer);    /* total nr of contour + aux points + 1 */
			pointer += XYCoordDataOffset;
			for (jj = 1; jj < nPoint; jj++)
				{
				if (GET_WORD((pointer + 2*jj)) == (nPoint - jj - 1))
					break;
				}
			nAuxPt = nPoint - jj - 1;
			if (nAuxPt < 0 || nAuxPt > jj)
				{
#ifdef DEBUG_ALT
        		printf("          *** Unable to find auxiliary point count data! ***\n");
#endif
        		return(FALSE);
				}
        	pXOrg = pointer;  /* Pointer to X coord of first point */
        	pYOrg = pXOrg + (nPoint << 1);  /* Pointer to Y coord of first point */
			nPoint = jj;	 /* number of contour points only */
			}
    	else
			{/* standard format */
        	XYCoordDataOffset = GET_WORD(pointer); /* Read offset to coordinate data */
        	pointer = pChar + 4 + XYCoordDataOffset;
        	nPoint = GET_WORD(pointer);     /* Number of points in contour */
        	pointer += 2;
        	nAuxPt = GET_WORD(pointer);     /* Number of auxiliary points in contour */
        	pXOrg = pointer + 2;            /* Pointer to X coord of first point */
        	pYOrg = pXOrg + (nPoint << 1);  /* Pointer to Y coord of first point */
			}

        pAuxXOrg = pYOrg + (nPoint << 1); /* Pointer to X coord of first aux point */
        pAuxYOrg = pAuxXOrg + nAuxPt;   /* Pointer to Y coord of first aux point */

        pX = pXOrg;
        pY = pYOrg;                     /* Current point is first point */

        if (outside)                    /* Outside (counter-clockwise) contour? */
            {
            pAuxX = pAuxXOrg; 
            pAuxY = pAuxYOrg;           /* Current aux point is first aux point */
            incr = 2;                   
            aux_incr = 1;               /* Processing direction is first to last */
            curve_mask = 0x8000;
            }
        else                            /* Inside (clockwise) contour? */
            {
            pAuxX = pAuxXOrg + nAuxPt - 1;
            pAuxY = pAuxYOrg + nAuxPt - 1; /* Current aux point is last aux point */ 
            incr = -2;
            aux_incr = -1;              /* Processing direction is last to first */
            curve_mask = 0x4000;
            }

        for (jj = 0; jj <= nPoint; jj++, pX += incr, pY += incr) /* For each point... */
            {
            if ((jj == nPoint) &&       /* Last point? */
                (outside))              /* Outside contour? */
                {
                pX = pXOrg;
                pY = pYOrg;             /* Current point is first point */
                }

            oldX = X;
            oldY = Y;                   /* Save preceding point */

            rawX = GET_WORD(pX);        /* Read raw X data for current point */
            rawY = GET_WORD(pY);        /* Read raw Y data for current point */

            X = (rawX & 0x3FFF);
            Y = (rawY & 0x3FFF);        /* Extract coordinate data */

            if (jj == 0)                /* First point in contour? */
                {
                eo_do_trans (PARAMS2 X, Y, &P);
                fn_begin_contour(P, outside); /* Do MOVE to transformed point */
                if (!outside)           /* Inside contour? */
                    {
                    pX = pXOrg + (nPoint << 1);
                    pY = pYOrg + (nPoint << 1); /* Wrap to end of contour */
                    }
                continue;
                }

            if ((X == oldX) &&
                (Y == oldY))            /* Duplicate point? */
                {
                if ( ContourFlag != 3 && ((rawX & curve_mask) == 0)) /* Curve ends at current point? */
                    {
                    pAuxX += aux_incr;  /* Update pointer to X coord of current aux point */
                    pAuxY += aux_incr;  /* Update pointer to Y coord of current aux point */
                    }
                continue;
                }

            if ( ContourFlag != 3 && ((rawX & curve_mask) == 0))   /* Curve ends at current point? */
                {
                dX = (fix15)((fix7 )(*pAuxX)); /* Read dX for aux point */
                pAuxX += aux_incr;      /* Update pointer to X coord of current aux point */
                dY = (fix15)((fix7 )(*pAuxY)); /* Read dY for aux point */
                pAuxY += aux_incr;      /* Update pointer to Y coord of current aux point */
 
                if ((dX >  sp_globals.processor.pcl.eo_aux_thresh) ||
                    (dX < -sp_globals.processor.pcl.eo_aux_thresh) ||
                    (dY >  sp_globals.processor.pcl.eo_aux_thresh) ||
                    (dY < -sp_globals.processor.pcl.eo_aux_thresh)) /* Aux point is significant? */
                    {
                    auxX = ((oldX + X) >> 1) + dX;
                    auxY = ((oldY + Y) >> 1) + dY;
                    if (!eo_proc_2arc(PARAMS2 
                        oldX, 
                        oldY, 
                        (fix31)auxX << 16,
                        (fix31)auxY << 16,
                        X, 
                        Y))             /* Do CRVE to current point */
                        {
                        *errno = EEO_BAD_OUTL_DATA;
                        return (FALSE);
                        }
                    continue;
                    }
                }

            eo_do_trans (PARAMS2 X, Y, &P);
            fn_line(P);                 /* Do LINE to current point */
            }
        fn_end_contour();               /* Signal end of contour */
        }
    return (TRUE);                      /* End of character */
#ifdef DEFUNCT
    }
else                                    /* Alternate character data format? */
    {
    cdata.pchar = pChar;
    cdata.ptree = BTSNULL;
    while (TRUE)
        {
        ptype = eo_get_outl_data_alt (&cdata, &contp, &auxp);
    
        switch(ptype)
            {
            case EO_T_MOVE:                 /* First point in contour */
                curr_pos_x = contp.x;
                curr_pos_y = contp.y;
                eo_do_trans (PARAMS2 curr_pos_x, curr_pos_y, &P);
                fn_begin_contour(P, outside);
                break;

            case EO_T_LINE:                 /* Line to new point */
                if ((contp.x == curr_pos_x)  && 
                    (contp.y == curr_pos_y))
                    continue;
                eo_do_trans (PARAMS2 contp.x, contp.y, &P);
                fn_line(P);
                curr_pos_x = contp.x;
                curr_pos_y = contp.y;
                break;

            case EO_T_ARC:                  /* Arc to new point */
                if ((contp.x == curr_pos_x)  && 
                    (contp.y == curr_pos_y))
                    continue;
    
                if (!eo_proc_2arc(PARAMS2 curr_pos_x, curr_pos_y,
                                  auxp.x, auxp.y, contp.x, contp.y))
                    {
                    *errno = EEO_BAD_OUTL_DATA;
                    return (FALSE);
                    }
                curr_pos_x = contp.x;
                curr_pos_y = contp.y;
                break;

            case EO_T_END_CONTOUR:          /* End of contour */
                fn_end_contour();
                break;
    
            case EO_T_END_CHAR:             /* End of character */
                return (TRUE);
   
            case EO_T_ERROR:                /* Error */
            default:
                *errno = EEO_BAD_OUTL_DATA;
                return (FALSE);
            }
        }
    }
#endif /* DEFUNCT */
}



#ifdef DEFUNCT
#define  MAX_DEV2  256

FUNCTION LOCAL_PROTO fix15 eo_get_outl_data_alt(ospecs_t *cdata, coord_t *contp, lcoord_t *auxp)

/*  This is an UNDOCUMENTED character format found in some characters in
    some HP downloadable fonts. This is our knowledge so far:
    1) The metrics offset is 12 instead of the usual 10. Bytes 14-15, where
       the metrics usually start, is 0xFFFF.
    2) Metric data is otherwise the same.
    3) Hint data is about twice as large as normal.
    4) Contour tree data is normal, but the second word is '3' instead of '5'
   *** We shall use this fact as an ad hoc rule that tells us we have
       the alternate format. ***
    5) Outline data:
       a) first word = n = contour points + auxiliary points + 1
       b) second word = 0xFFFF
       c) third word = ????
       d) then we have n words of x coordinate data. Embedded _somewhere_ in
          the middle of all this is a word 'm' that stands for the # of
          auxiliary pts.
          The number of contour pts is thus c = n - m - 1;  0 <= m <= c.
          The 2 high order bits are NOT used to indicate which segments are
          vectors and which are arcs! There is reason to believe this
          information is embedded in the hint data (note that it is much
          larger than usual), but it's not obvious how to extract it.
       e) repeat the above for n words of y coordinate data.
    6) If there is > 1 contour in the character there is some unknown
       gunk following each contour.
    7) There is some unknown stuff at the end of the character.
    "Normal", "usual", etc. refer what we see in FAIS standard characters.

    Here's how to decide on the placement of A-points relative to C-points.
    If the next A-point in sequence lies inside or on the edge of the
    rectangle defined by the 2 neighboring C-points,
    *** and if the midpoint of the 2 neighboring points is < 128 ORUs away in
        X and Y *** mby 7/16
    then use it to compute a pair of arcs. Otherwise the 2 C-points are
    connected by a vector. If all the A-points are not used up by the time
    we've reached the last C-point then we have a problem.
*/

{
ufix8  *optr;
fix15     ii;
fix15  x_aux, y_aux;
fix15  x_prv, y_prv;
fix15  xlow, xhigh, ylow, yhigh;

if (cdata->ptree == BTSNULL)               /* first time through this fcn */
    {
    if (GET_WORD((cdata->pchar + 10)) == 0xFFFF)
        return (EO_T_END_CHAR);         /* a no-op character! */
    cdata->ptree = cdata->pchar + GET_WORD((cdata->pchar + 10)) + 4;
    cdata->nr_contour = GET_WORD(cdata->ptree);
    cdata->contour_index = 0;
    cdata->nr_cp = 0;
    cdata->pcont = cdata->pchar + GET_WORD((cdata->ptree + 4)) + 4;
    }

if (cdata->contour_index >= cdata->nr_contour)
    return (EO_T_END_CHAR);

if (cdata->nr_cp == 0)                  /* beginning of each contour */
    {
    cdata->nr_allp = GET_WORD(cdata->pcont);    /* total nr of contour + aux points + 1 */
    cdata->c_offset = cdata->a_offset = 0;
    cdata->end_contour = FALSE;
#ifdef DEBUG_ALT
    printf("Alt data: index %d {%d %X %d}\n", eo_index, cdata->nr_allp,
                           GET_WORD((cdata->pcont+2)), GET_WORD((cdata->pcont+4)));
#endif
    cdata->pcont += 6;                  /* skip next two words */
    optr = cdata->pcont;
    for (ii=1; ii<cdata->nr_allp; ii++)     /* Look ahead to find # of auxiliary points */
        {
        if (GET_WORD((optr + 2*ii)) == (cdata->nr_allp - ii - 1))
            break;
        }
    cdata->nr_ap = cdata->nr_allp - ii - 1;
    cdata->nr_cp = ii;
    if (cdata->nr_ap < 0  ||  cdata->nr_ap > cdata->nr_cp)
        {
#ifdef DEBUG_ALT
        printf("          *** Unable to find auxiliary point count data! ***\n");
#endif
        return(EO_T_ERROR);
        }
    }

if (!cdata->end_contour)
    {
    if (cdata->c_offset < cdata->nr_cp)
        {
        contp->x = GET_WORD((cdata->pcont + 2*cdata->c_offset));
        contp->y = GET_WORD((cdata->pcont + 2*(cdata->nr_allp + cdata->c_offset)));
        }
    if (cdata->c_offset == 0)           /* contour begins here? */
        {
        cdata->c_offset++;          
        return (EO_T_MOVE);             /* 1st point is always MOVE */
        }
    if (cdata->c_offset == cdata->nr_cp)
        {                               /* wrap around to 1st point to close the curve */
        contp->x = GET_WORD(cdata->pcont);
        contp->y = GET_WORD((cdata->pcont + 2*cdata->nr_allp));
        cdata->end_contour = TRUE;
        }
    if (cdata->a_offset < cdata->nr_ap)  /* if there are any aux points remaining */
        {
        optr = cdata->pcont + 2*(cdata->c_offset - 1);
        x_prv = GET_WORD(optr);
        y_prv = GET_WORD((optr + 2 * cdata->nr_allp));
        optr = cdata->pcont + 2*(cdata->nr_cp + cdata->a_offset + 1);
        x_aux = GET_WORD(optr);                         /* The aux coordinates */
        y_aux = GET_WORD((optr + 2 * cdata->nr_allp));  /* are shifted 1 bit left */
        cdata->c_offset++;
        if (x_prv < contp->x)
            {
            xlow = x_prv;
            xhigh = contp->x;
            }
        else
            {
            xlow = contp->x;
            xhigh = x_prv;
            }
        if (y_prv < contp->y)
            {
            ylow = y_prv;
            yhigh = contp->y;
            }
        else
            {
            ylow = contp->y;
            yhigh = y_prv;
            }
        if (x_aux >= (xlow << 1)  &&  x_aux <= (xhigh << 1)  &&  
                    y_aux >= (ylow << 1)  &&  y_aux <= (yhigh << 1))
            {
            /* Proceed only if the aux point is close enough to the midpoint. */
            if (ABS((x_aux - (xlow + xhigh))) < MAX_DEV2  &&
                    ABS((y_aux - (ylow + yhigh))) < MAX_DEV2)
                {
                auxp->x = (fix31)x_aux << 15;   /* put auxp->x/y into 16.16 format */
                auxp->y = (fix31)y_aux << 15;
                cdata->a_offset++;
                return(EO_T_ARC);
                }
            else
                return(EO_T_LINE);
            }
        else
            return(EO_T_LINE);
        }
    else
        {
        cdata->c_offset++;
        return(EO_T_LINE);
        }
    }
else                                    /* end of contour; set up for next one */
    {
    if (cdata->a_offset != cdata->nr_ap)    /* error if aux points out of synch */
        {
#ifdef DEBUG_ALT
        printf("          *** Error ***  contour and auxiliary points out of synch\n");
#endif
        return(EO_T_ERROR);
        }
#ifdef DEBUG_ALT
    printf("          %d contour points, %d aux points\n", cdata->c_offset - 1, cdata->nr_ap);
#endif
    cdata->contour_index++;
    cdata->pcont = cdata->pchar + GET_WORD((cdata->ptree + 8*cdata->contour_index + 4)) + 4;
    cdata->nr_cp = 0;
    return(EO_T_END_CONTOUR);
    }
}
#endif /* DEFUNCT */


FUNCTION LOCAL_PROTO btsbool eo_proc_2arc(SPGLOBPTR2 fix15 xa, fix15 ya, fix31 xb, fix31 yb, fix15 xc, fix15 yc)

/*  Returns TRUE if successfully renders 2 arcs; otherwise FALSE

    Draws 2 arcs, from point A to B and from B to C. The arcs are tangent
    at B. The tangent at B (auxiliary point) is parallel to line AC (contour
    points). For each arc, compute the bulge factor, then split the arc
    recursively until the deviation between the vectors and the arc is
    less than the threshold.
*/
{
fix15    beta1, beta2;
fix31    delx, dely;
fix31    dist1, dist2;
fix31    distq;
fix31    dd, dd1;
fix31    midx, midy;
fix31    nx, ny;
fix31    px, py;
fix15    shd, shx, shy;
fix15      sign;
fix31    temp;
ufix32   utemp;
fix31    xa1, ya1;
fix31    xc1, yc1;
fix31    xalng, yalng;            /* xa, ya shifted left by 16 */
fix31    xclng, yclng;            /* xc, yc shifted left by 16 */ 
point_t  P;


/*  The following code checks to see if all the bulge factor calculations are
 *  unnecessary. If so just use straight lines to connect cont-aux-cont!
 *  This speeds things up *a lot* at small/medium point sizes.
 *  Let points A & C be adjacent contour points and point B be the auxiliary
 *  point. It can be shown empirically (for reasonable placement of points)
 *  that the distance between point B and line AC is between 2 and 4 times
 *  greater than the bulges for arcs AB and BC. If the point-line distance is
 *  smaller than 1 pixel, then the arcs will approximate straight lines to
 *  within .25 to .50 pixel. Hence no arc splitting is necessary. Estimate the
 *  point-line distance by using the point-B-midpoint-AC distance (this could
 *  come out way off on the high side, but usually B is placed fairly
 *  symmetrically.) Approximate the distance by taking the larger of delta-x
 *  or -y plus half the smaller (maximum error = 12%). Any errors caused by
 *  these approximations will cause unnecessary computation (will waste time)
 *  but will not degrade the rendering of the character.
 *  mby 8/21/90         */

xclng = (fix31)xc << 16;
yclng = (fix31)yc << 16;
temp = xb - (((fix31)xa + (fix31)xc) << 15);
delx = ABS(temp);
temp = yb - (((fix31)ya + (fix31)yc) << 15);
dely = ABS(temp);
distq = (delx >= dely) ? delx + (dely >> 1) : dely + (delx >> 1);
if ((distq >> 1) < sp_globals.processor.pcl.eo_spl_thresh)
    {
    eo_do_trans (PARAMS2 (fix15)((xb + 0x8000) >> 16), (fix15)((yb + 0x8000) >> 16), &P);
    fn_line(P);
    eo_do_trans (PARAMS2 xc, yc, &P);
    fn_line(P);
    return(TRUE);
    }

/* If quick test fails, do the whole shebang. */
sign = bsign((fix15)(xa<<1), (fix15)(ya<<1),
                (fix15)(xb>>15), (fix15)(yb>>15),
                   (fix15)(xc<<1), (fix15)(yc<<1));
                        /* Normalize variables, then compute sign of curvature. */
                        /* 1 if clockwise, -1 if ccw, 0 if collinear. */
                        /* This is the sign of the bulge factor. */

if (sign == 0)
    {
    eo_do_trans (PARAMS2 xc, yc, &P);
    fn_line(P);
    return(TRUE);                       /* transform and output a single vector */
    }

/* Solve pair of simultaneous equations  1) tangent line passing through
   point B, parallel to line AC  2) normal to chord AB passing through
   midpoint of the chord.
   All work done in sliding point arithmetic. */
xalng = (fix31)xa << 16;
yalng = (fix31)ya << 16;
xa1 = ((fix31)xa << 1) - (xb >> 15);    /* 31.1 */
ya1 = ((fix31)ya << 1) - (yb >> 15);
temp = (xa1*xa1 + ya1*ya1 + 4) >> 3;    /* long int */

        /* 32-bit (multiply) overflow can occur if temp is too large. */
if (temp < 0x20000  &&  temp > -(0x20000))
    {
    nx = ((fix31)xc - (fix31)xa) * temp;
    ny = ((fix31)yc - (fix31)ya) * temp;
    shx = shy = 0;
    }
else            /* if the product is more than 31 bits (signed), then shx|shy        */
    {           /* represent a right shift applied to temp to avoid integer overflow */
                /*  1/17/91  mby  */
    shx = MAX(0, (bit_count(temp) + bit_count((fix31)xc - (fix31)xa) - 31) );
    nx = ((fix31)xc - (fix31)xa) * (temp >> shx);
    shy = MAX(0, (bit_count(temp) + bit_count((fix31)yc - (fix31)ya) - 31) );
    ny = ((fix31)yc - (fix31)ya) * (temp >> shy);
    }
dd = ((fix31)xc - (fix31)xa) * xa1 + ((fix31)yc - (fix31)ya) * ya1;  /* 31.1 */
shd = shift_16 (dd, (ufix32 *)&dd1);
shx = shift_left_32 (nx, &utemp) - shx;
px = (utemp / dd1) << (16 - shd - shx + 1);            /* 16.16 */
if ((nx < 0) ^ (dd < 0) == 1)
    px = -px;
shy = shift_left_32 (ny, &utemp) - shy;
py = (utemp / dd1) << (16 - shd - shy + 1);            /* 16.16 */
if ((ny < 0) ^ (dd < 0) == 1)
    py = -py;
px += xb;                               /* result in 16.16 fix15 d point */
py += yb;
midx = (xalng + xb) >> 1;
midy = (yalng + yb) >> 1;
dist1 = dist(px - midx, py - midy);     /* distance between midpt of chord & solution point */
dist2 = dist(xb - midx, yb - midy);     /* distance = 1/2 the length of chord */
shx = 16 - shift_left_32 (dist1, &utemp);
if (shx > 0)                                /* These gyrations are intended */
    dist2 = (dist2 + (1 << (shx-1))) >> shx; /* to produce the quotient */
else if (shx < 0)                           /* (dist1/dist2) as an integer */
    dist2 = dist2 << (-shx);                /* with 16 fractional bits */
utemp = utemp / dist2;
beta1 = sign * gg(utemp);               /* returns bulge factor */

/* Same as above but use chord BC */
xc1 = ((fix31)xc << 1) - (xb >> 15);
yc1 = ((fix31)yc << 1) - (yb >> 15);
temp = (xc1*xc1 + yc1*yc1 + 4) >> 3;

        /* 32-bit (multiply) overflow can occur if temp is too large. */
if (temp < 0x20000  &&  temp > -(0x20000))
    {
    nx = ((fix31)xc - (fix31)xa) * temp;
    ny = ((fix31)yc - (fix31)ya) * temp;
    shx = shy = 0;
    }
else            /* if the product is more than 31 bits (signed), then shx|shy        */
    {           /* represent a right shift applied to temp to avoid integer overflow */
                /*  1/17/91  mby  */
    shx = MAX(0, (bit_count(temp) + bit_count((fix31)xc - (fix31)xa) - 31) );
    nx = ((fix31)xc - (fix31)xa) * (temp >> shx);
    shy = MAX(0, (bit_count(temp) + bit_count((fix31)yc - (fix31)ya) - 31) );
    ny = ((fix31)yc - (fix31)ya) * (temp >> shy);
    }
dd = ((fix31)xc - (fix31)xa) * xc1 + ((fix31)yc - (fix31)ya) * yc1;   /* 31.1 */
shd = shift_16 (dd, (ufix32 *)&dd1);
shx = shift_left_32 (nx, &utemp) - shx;
px = (utemp / dd1) << (16 - shd - shx + 1);
if ((nx < 0) ^ (dd < 0) == 1)
    px = -px;
shy = shift_left_32 (ny, &utemp) - shy;
py = (utemp / dd1) << (16 - shd - shy + 1);
if ((ny < 0) ^ (dd < 0) == 1)
    py = -py;
px += xb;
py += yb;
midx = (((fix31)xc << 16) + xb) >> 1;
midy = (((fix31)yc << 16) + yb) >> 1;
dist1 = dist(px - midx, py - midy);
dist2 = dist(xb - midx, yb - midy);
shx = 16 - shift_left_32 (dist1, &utemp);
if (shx > 0)
    dist2 = (dist2 + (1 << (shx-1))) >> shx;
if (shx < 0)
    dist2 = dist2 << (-shx);
utemp = utemp / dist2;
beta2 = sign * gg(utemp);               /* returns bulge factor */

/* Split arcs recursively. Output (lineto) is done from split_arc. */
if (!split_arc(PARAMS2 xalng, yalng, xb, yb, beta1))
    return(FALSE);
if (!split_arc(PARAMS2 xb, yb, xclng, yclng, beta2))
    return(FALSE);

return(TRUE);
}


FUNCTION LOCAL_PROTO btsbool split_arc(SPGLOBPTR2 fix31 x0, fix31 y0, fix31 x1, fix31 y1, fix15 beta)

/* Performs recursive subdivision of arcs until error < threshold.
   Returns TRUE
           (FALSE if can't split arc)
   Uses integer arithmetic              */

{
fix31   midx, midy;             /* midpoint (16.16) */
fix15   new_beta;
ufix32  t1, t2, t3, t4;
point_t P;

/* midx = 0.5 * (x0 + x1 - beta * (y1 - y0)); */
/* midy = 0.5 * (y0 + y1 + beta * (x1 - x0)); */
midx = (x0 + x1 - mult_122(beta, (y1 - y0))) >> 1;
midy = (y0 + y1 + mult_122(beta, (x1 - x0))) >> 1;
if (dist(midx - ((x0 + x1) >> 1), midy - ((y0 + y1) >> 1)) < sp_globals.processor.pcl.eo_spl_thresh)
    {
    eo_do_trans (PARAMS2 (fix15)((x1 + 0x8000) >> 16), (fix15)((y1 + 0x8000) >> 16), &P);
    fn_line(P);
    }
else                                    /* Subdivide the arc. */
    /* The new bulge factor is given by (sqrt(1+b^2) - 1)/b, where 'b' is the
     * old b.f. and is approximated by the series
     *    b/2 - b^3/8 + b^5/16 - b^7*5/128 + ...   */
    {
    if (beta >= 0)
        t1 = beta;
    else
        t1 = -beta;
    t2 = (t1 * t1 + 0x8000) >> 16;
    t3 = (t1 * t2 + 0x8000) >> 16;
    new_beta = (t1 >> 1) - ((t3 + 4) >> 3);
    if (beta < 0x2000)                  /* if beta < 0.125, use only  */
        goto done;                      /* 1st 2 terms of the series. */
    new_beta += (((t2 * t3 + 0x8000) >> 16) + 8) >> 4;
    if (beta < 0x4000)                  /* if beta < 0.25, use 3rd term */
        goto done;
    t4 = (t2 * t2 + 0x8000) >> 16;      /* if beta >= 0.25, use 4th term */
    new_beta -= (((5 * t3 * t4 + 0x8000) >> 16) + 64) >> 7;

done:
    if (beta < 0)
        new_beta = -new_beta;
    split_arc(PARAMS2 x0, y0, midx, midy, new_beta);
    split_arc(PARAMS2 midx, midy, x1, y1, new_beta);
    }
return (TRUE);
}
/* .1    -.000000621 +.000000003 -.000000000006
 * .125  -.000001888 +.000000018 -.00000000022
 * .15   -.000004680 +.000000065 -.0000000010
 * .2    -.000019513 +.000000486 -.000000013
 * .25               +.000002284 -.000000099
 * .3                +.000008038 -.000000504
 * .35               +.000023158 -.000001974
 * .4                +.000057596 -.000006403
 * .45               +.000127986 -.000017977
 * .46771            +.000166058 -.000025180
 * .5                +.000260147 -.000045028
*/


FUNCTION LOCAL_PROTO fix15 bsign(fix15 x0, fix15 y0, fix15 x1, fix15 y1, fix15 x2, fix15 y2)

/* Takes 3 points (x,y)
   Returns 1 if pt0-pt1-pt2 is clockwise
   Returns -1 if pt0-pt1-pt2 is counterclockwise
   Returns 0 if 3 points are collinear
*/
{
fix15   vx0, vy0, vx1, vy1;
fix31   temp1;

vx0 = x1 - x0;
vy0 = y1 - y0;
vx1 = x2 - x1;
vy1 = y2 - y1;
/* Use the cross-product to get the sine of the angle between two vectors.
   All we're really concerned with is the sign of the sine */
temp1 = (fix31)vx0 * vy1 - vx1 * vy0;
if (temp1 == 0)
    return(0);
else if (temp1 > 0)
    return(-1);
else
    return(1);
}


FUNCTION LOCAL_PROTO fix31 gg(fix31 xx)

/* 'xx' is the tangent of the angle between the tangent vector at one end of
   the arc and the chord between the endpoints.
        bulge-factor  =  gg(xx)
   Interpolate ggivalue table to evaluate function.
*/
{
fix15  ii;
ufix32 temp;

if (xx < 0)
    return ((fix31)0);
if (xx < 0x10000)
    {
    ii = ((xx & 0xF000) >> 12);
    temp = ggivalue[ii] + (((xx & 0x0FFFL) * (ggivalue[ii+1] - ggivalue[ii]) + 2048) >> 12);
    }
else if (xx <= 0x20000)
    temp = ggivalue[16] + (((xx - 0x10000L) * (ggivalue[17] - ggivalue[16]) + 8192) >> 14);
else
    return(0);
return ((temp >> 1));
}


FUNCTION LOCAL_PROTO fix31 dist(fix31 x0, fix31 y0)

/* Returns distance between point (x, y) and the origin.
   In floating point the answer is simply sqrt(x^2 + y^2)
   Here's a way to approximate sqrt
     1) rewrite as d = x * sqrt(1 + y^2 / x^2),  if y <= x
                     = x * sqrt(1 + q); 0 <= q <= 1
     2) Have a table of, say 17, linearly spaced values such as
          sqrt(1) = 1.00
          sqrt(1.0625) = 1.0308...
          sqrt(1.125) = 1.0607...
                etc.
          sqrt(2) = 1.4142...
     3) Use this table + interpolation to estimate sqrt(1+q)
     4) If there are 16 interpolations, the maximum error is ~ 0.00012
                     32 interpolations, the maximum error is ~ 0.00003
        A power of 2 helps make this run faster.
*/
{
ufix16  base, del;
ufix16  f1, f2;
ufix32  g1, g2;
int     ii;
ufix32  n1, n2, n3, n4;
int     shn, shd, shd1, shift;
ufix32  sq;

if (x0 == 0 && y0 == 0)
    return((fix31)0);

if (x0 < 0)
    x0 = -x0;

if (y0 < 0)
    y0 = -y0;

if (x0 >= y0)                           /* which is the larger dimension? */
    {
    n1 = x0;
    n2 = y0;
    }
else
    {
    n1 = y0;
    n2 = x0;
    }

shn = 2 * shift_16 (n2, (ufix32 *)&g2);           /* factor out n1 from under the root */
if ((n4 = g2 * g2) & 0x80000000)
    n3 = n4;
else
    shn -= shift_left_32 (n4, &n3);
shd1 = shift_16 (n1, (ufix32 *)&g1);
if ((n4 = g1 * g1) & 0x80000000)
    {
    g2 = (n4 + 0x8000) >> 16;
    shd = shd1 + shd1 + 16;
    }
else
    {
    shd = shd1 + shd1 + shift_16 (n4, (ufix32 *)&g2);
    }
n3 = n3 / g2;
shift = shd - shn - 16;                 /* numerator squared, shifted all the way left in a 32-bit word */
f1 = n3 >> shift;                       /* divided by denominator squared, shifted all the way left in  */
                                        /* a 16-bit word, then shifted right an appropriate amount. */
ii = f1 >> 12;
base = sqrt_table[ii];
del = sqrt_table[ii+1] - base;
f2 = base + ((f1 & sqrt_table_m) * del + sqrt_table_r) / sqrt_table_d;
n3 = 0x10000L + ((f2 + 1) >> 1);
sq = (g1 * (n3 >> 1)) >> (15 - shd1);
return(sq);
}


FUNCTION LOCAL_PROTO fix15 eo_inq_compchar(ufix8 *pchar)

{
if (pchar == BTSNULL)
    return(-1);
if (pchar[0] != 10  ||  pchar[1] != 0  ||  pchar[2] != 2)
    return(-1);
if (pchar[3] == 3)
    return(0);
else if (pchar[3] == 4)
    {
    return((fix15) pchar[6]);
    }
else
    return(-1);
}


FUNCTION LOCAL_PROTO btsbool eo_get_component(ufix8  *pchar, fix15 nn, fix15 *index, fix15 *xoff, fix15 *yoff)

/*  Returns: FALSE if error (i is out of bounds, or pchar != compound)
    Output:  index, x- and y-offsets */

{
if (pchar[0] != 10  ||  pchar[1] != 0  ||  pchar[2] != 2  ||  pchar[3] != 4)
    return(FALSE);
if (nn >= pchar[6])
    return(FALSE);
*index = GET_WORD((pchar + 6*nn + 8));
*xoff = GET_WORD((pchar + 6*nn + 10));
*yoff = GET_WORD((pchar + 6*nn + 12));
return(TRUE);
}


FUNCTION LOCAL_PROTO fix15 eo_get_swidth(ufix8 *pchar)
{
fix15  nn;

if (eo_inq_compchar(pchar))             /* Compound character? */
    {
    nn = GET_WORD((pchar + 4));
    return(nn);
    }
else                                    /* Simple character? */
    {
    nn = GET_WORD((pchar + 6));
    nn = (GET_WORD((pchar + nn + 16)) - GET_WORD((pchar + nn + 12)));
    return(nn);
    }
}


FUNCTION LOCAL_PROTO void eo_get_bbox(SPGLOBPTR2 eo_char_t *pchar, fix15 *xmin, fix15 *ymin, fix15 *xmax, fix15 *ymax)

/* Get the bounding box in DWUs.
   If simple character, return the data right from the metrics.
   If compound char, return compound bbox    */

{
fix15  comp_index, comp_xoff, comp_yoff;
fix15    ii, mm;
fix15  nn;
fix15  temp;
eo_char_t  *pchar_sub;

if ((mm = eo_inq_compchar(pchar)) == 0)   /* simple char */
    {
    nn = GET_WORD((pchar + 6));
    *xmin = GET_WORD((pchar + nn + 4));
    *ymin = GET_WORD((pchar + nn + 6));
    *xmax = GET_WORD((pchar + nn + 8));
    *ymax = GET_WORD((pchar + nn + 10));
    }
else if (mm > 0)                        /* composite char */
    {                                   /* run through each character; it must */
    *xmin = *ymin = MAX_DWU;            /* exist; it must be a simple char; read */
    *xmax = *ymax = -MAX_DWU;           /* & combine bbox's from each character. */
    for (ii=0; ii<mm; ii++)
        {
        if (eo_get_component(pchar, ii, &comp_index, &comp_xoff, &comp_yoff))
            {
            if ((pchar_sub = (eo_char_t *)eo_get_char_addr(PARAMS2 sp_globals.processor.pcl.eo_specs.pfont->eofont, comp_index)) != BTSNULL)
                {
                if (eo_inq_compchar(pchar_sub) == 0)
                    {
                    nn = GET_WORD((pchar_sub + 6));
                    temp = GET_WORD((pchar_sub + nn + 4)) + comp_xoff;
                    if (temp < *xmin)
                        *xmin = temp;
                    temp = GET_WORD((pchar_sub + nn + 6)) + comp_yoff;
                    if (temp < *ymin)
                        *ymin = temp;
                    temp = GET_WORD((pchar_sub + nn + 8)) + comp_xoff;
                    if (temp > *xmax)
                        *xmax = temp;
                    temp = GET_WORD((pchar_sub + nn + 10)) + comp_yoff;
                    if (temp > *ymax)
                        *ymax = temp;
                    }
                else  goto badchar;
                }
            else  goto badchar;
            }
        else  goto badchar;
        }
    }
return;

badchar:                                /* bad character */
*xmin = *ymin = -MAX_DWU;
*xmax = *ymax = MAX_DWU;
return;
}


FUNCTION LOCAL_PROTO btsbool eo_begin_char(SPGLOBPTR2 eo_char_t *pchar)
{
fix15    xmin, ymin;            /* Lower left corner of bounding box in DWUs */
fix15    xmax, ymax;            /* Upper right corner of bounding box in DWUs */
fix31    xx, yy;                /* Transformed end-point of escapement vector */
point_t  P;                     /* Transformed corner of bounding box */
point_t  Pmin, Pmax;            /* Bounding box of transformed character, which */
                                /* is _not_ _the_ _same_ as the transformed */
                                /* corners of the bounding box */
point_t  Pssw;                  /* Transformed escapement vector */
fix15      nn;
fix31    round;


eo_get_bbox(PARAMS2 pchar, &xmin, &ymin, &xmax, &ymax);     /* get bounding box of outline char in ORUs */

/* Transform 4 bbox corners and look for new {xmin, ymin, xmax, ymax} */
eo_do_trans(PARAMS2 xmin, ymin, &Pmin);         /* Transform lower left corner of bounding box */
Pmax = Pmin;

eo_do_trans(PARAMS2 xmin, ymax, &P);            /* Transform top left corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;

eo_do_trans(PARAMS2 xmax, ymin, &P);            /* Transform bottom right corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;

eo_do_trans(PARAMS2 xmax, ymax, &P);            /* Transform top right corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;

/* Convert to 16-bit fix15 d point (pixshift) format for output */
nn = 16 - sp_globals.pixshift;
round = ((fix15)1 << (nn - 1));
xmin = eo_get_swidth(pchar);
do_trans_width(PARAMS2 ((fix31)xmin << 16), &xx, &yy);
Pssw.x = (xx + round) >> nn;
Pssw.y = (yy + round) >> nn;
if (fn_begin_char(Pssw, Pmin, Pmax))
    return(TRUE);
else
    return(FALSE);
}


FUNCTION LOCAL_PROTO fix15 eo_begin_sub_char(SPGLOBPTR2 eo_char_t *pchar_sub, fix15 comp_xoff, fix15 comp_yoff)
{
fix15    xmin, ymin;            /* Lower left corner of bounding box in DWUs */
fix15    xmax, ymax;            /* Upper right corner of bounding box in DWUs */
fix31    xx, yy;                /* Transformed end-point of escapement vector */
point_t  P;                     /* Transformed corner of bounding box */
point_t  Pmin, Pmax;            /* Bounding box of transformed character, which */
                                /* is _not_ _the_ _same_ as the transformed */
                                /* corners of the bounding box */
point_t  Pssw;                  /* Transformed escapement vector */
fix15      nn;
fix31    round;

eo_get_bbox(PARAMS2 pchar_sub, &xmin, &ymin, &xmax, &ymax);   /* get bounding box of outline char in ORUs */

/* Transform 4 bbox corners and look for new {xmin, ymin, xmax, ymax} */
eo_do_trans(PARAMS2 xmin, ymin, &Pmin);         /* Transform lower left corner of bounding box */
Pmax = Pmin;

eo_do_trans(PARAMS2 xmin, ymax, &P);            /* Transform top left corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;

eo_do_trans(PARAMS2 xmax, ymin, &P);            /* Transform bottom right corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;

eo_do_trans(PARAMS2 xmax, ymax, &P);            /* Transform top right corner of bounding box */
if (P.x < Pmin.x)
    Pmin.x = P.x;
if (P.x > Pmax.x)
    Pmax.x = P.x;
if (P.y < Pmin.y)
    Pmin.y = P.y;
if (P.y > Pmax.y)
    Pmax.y = P.y;


/* Transform setwidth */
nn = 16 - sp_globals.pixshift;
round = ((fix15)1 << (nn - 1));
xmin = eo_get_swidth(pchar_sub);
do_trans_width(PARAMS2 ((fix31)xmin << 16), &xx, &yy); /* Transform escapement vector */
Pssw.x = (xx + round) >> nn;
Pssw.y = (yy + round) >> nn;

fn_begin_sub_char(Pssw, Pmin, Pmax);    /* Signal start of sub-character data */
}


FUNCTION LOCAL_PROTO fix15 do_trans_width(SPGLOBPTR2 fix31 xin, fix31 *xout, fix31 *yout)

/* Uses permanent CTM to transform xin (set width) in ORUs to xout,yout in pixels */

{
fix31  px, py;

px = ((xin + sp_globals.processor.pcl.eo_oru_round) >> sp_globals.processor.pcl.eo_oru_shift) * sp_globals.processor.pcl.eo_ctm_perm.xxmult;
py = ((xin + sp_globals.processor.pcl.eo_oru_round) >> sp_globals.processor.pcl.eo_oru_shift) * sp_globals.processor.pcl.eo_ctm_perm.yxmult;
if (sp_globals.processor.pcl.eo_prod_shift > 0)
    {
    *xout = (px + sp_globals.processor.pcl.eo_prod_round) >> sp_globals.processor.pcl.eo_prod_shift;
    *yout = (py + sp_globals.processor.pcl.eo_prod_round) >> sp_globals.processor.pcl.eo_prod_shift;
    }
else
    {
    *xout = px << -sp_globals.processor.pcl.eo_prod_shift;
    *yout = py << -sp_globals.processor.pcl.eo_prod_shift;
    }
}


FUNCTION LOCAL_PROTO fix15 shift_left_32(fix31 arg, ufix32 *out)

/* Extracts sign from arg. Shifts arg left to the leftmost bit.
   *** WARNING *** If this number is the numerator in an integer
   division, be careful with rounding (integer wrap)! */

{
fix15  i;
ufix32 n;

if (arg == 0)
    {
    *out = 0;
    return(0);
    }
if (arg >= 0)
    n = arg;
else
    n = -arg;

i = 0;
while ((n & 0x80000000) == 0)
    {
    n <<= 1;
    i++;
    }
*out = n;
return(i);
}


FUNCTION LOCAL_PROTO fix15 bit_count(fix31 arg)

/*  returns number of bits in arg (or -arg, if arg < 0)
    equivalent to log2(arg) + 1, rounded up to nearest integer  */

{
ufix32  test = 0x80000000;
fix15   n = 32;

if (arg == 0)
    return((fix15) 0);
if (arg < 0)
    arg = -arg;
while ((arg & test) == 0)
    {
    test >>= 1;
    n -= 1;
    }
return(n);
}


FUNCTION LOCAL_PROTO fix15 shift_16(fix31 arg, ufix32 *out)

{
fix15  i, last;
ufix32 n;

if (arg >= 0)
    n = arg;
else
    n = -arg;

if (n < 65536L)
    {
    *out = n;
    return(0);
    }

i = 0;
last = 0;
while (n & 0xFFFF0000)
    {
    last = (n & 1);
    n >>= 1;
    i++;
    }

*out = (n != 0xFFFF)? n + last: n;

return(i);
}


FUNCTION LOCAL_PROTO fix31 mult_122(fix15 args, fix31 argl)

/* Multiplies a 16-bit number times a 32-bit number.
   Returns the high 32 bits of the product. */

{
fix15     sign;
ufix32  t0, t1;
ufix32  thi, tlo;

sign = 1;
if (args < 0)
    {
    args = -args;
    sign = -sign;
    }
if (argl < 0)
    {
    argl = -argl;
    sign = -sign;
    }
t0 = (argl & 0xFFFF0000) >> 16;
t1 = (argl & 0x0000FFFF);
thi = t0 * args;
tlo = t1 * args;
t1 = ((tlo & 0xFFFF0000) >> 16) + (thi & 0x0000FFFF);
t0 = (thi & 0xFFFF0000) + t1;
if (sign < 0)
    t0 = -t0;
return(t0);
}

#endif /* PROC_PCL */
/* EOF hpfnt1.c */
