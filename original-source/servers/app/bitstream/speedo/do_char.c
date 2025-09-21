

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
*     $Header: //bliss/user/archive/rcs/speedo/do_char.c,v 33.0 97/03/17 17:44:39 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	do_char.c,v $
*       Revision 33.0  97/03/17  17:44:39  shawn
*       TDIS Version 6.00
*       
*       Revision 32.8  97/01/14  17:43:32  shawn
*       Apply casts to avoid compiler warnings.
*       
*       Revision 32.7  96/12/16  09:39:37  shawn
*       Added Global Prototype statements for sp_calculate_x_scale() & sp_calculate_y_scale().
*       
*       Revision 32.6  96/12/13  16:53:22  shawn
*       Corrected second point parameter to '*p2' in sp_review_bounding_box().
*       
*       Revision 32.5  96/07/02  12:01:22  shawn
*       Changed boolean to btsbool and NULL to BTSNULL.
*       
*       Revision 32.4  96/07/01  13:32:01  shawn
*       Corrected uninitialized (p2) variable warning.
*       
*       Revision 32.3  96/04/11  16:36:02  shawn
*       Changed code to reflect the move of MULTIDEV speedo structure members
*            from the Speedo specific union member to the global speedo area.
*       
*       Revision 32.2  96/03/28  11:36:16  shawn
*       Added INCL_GRAY to '#if (Any bitmap modules included?)'.
*       
*       Revision 32.1  95/11/17  10:08:00  shawn
*       Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
*       
*       Revision 32.0  95/02/15  16:33:11  shawn
*       Release
*       
*       Revision 31.3  95/01/24  16:36:33  shawn
*       Corrected Macro SPGLOBptr2 to SPGLOBPTR2 in sp_make_char()
*       
*       Revision 31.2  95/01/12  11:15:48  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:29:24  roberte
*       Release
*       
*       Revision 30.5  94/12/21  14:57:28  shawn
*       Conversion of all function prototypes and declaration to true ANSI type.
*       
*       Revision 30.4  94/10/24  10:21:02  roberte
*       Added condition #if PROC_SPEEDO so processor may be excluded in 4-in-1 builds.
*       
*       Revision 30.3  94/07/20  08:44:02  roberte
*       Added check for compound-compound character to sp_make_comp_char().
*       
*       Revision 30.2  94/06/07  08:54:43  roberte
*       Added code for Merged Font Technology (INCL_MFT blocks).
*       
*       Revision 30.1  94/05/11  15:32:45  roberte
*       Changed test of no_bytes for <= 0 in sp_get_char_org().
*       Thanks Sven!
*       
*       Revision 30.0  94/05/04  09:24:33  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:34:52  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:41:49  roberte
*       Release
*       
*       Revision 28.53  94/04/01  15:53:42  roberte
*       
*       
*       Added static function prototypes that were in speedo.h
*       
*       Revision 28.52  94/03/23  10:49:50  roberte
*       Added new entry point for sp_get_char_width_orus().
*       Define sp_get_char_width() in terms of new function.
*       
*       Revision 28.51  94/03/09  15:17:50  roberte
*       Added proper bit field checking of sp_globals.FourIn1Flags BIT0.
*       
*       Revision 28.50  94/02/10  13:14:12  roberte
*       Upgrade for MUTABLE fonts.
*       
*       Revision 28.48  93/10/01  10:54:00  roberte
*       Added INCL_REMOTE conditionals to support remote characters
*       in new Compressed Resident Font Suite. Added new function
*       sp_is_remote_char() for testing new glyph data type, and
*       storing binary information in the sp_globals.RemoteCharData array.
*       
*       Revision 28.47  93/08/31  09:35:10  roberte
*       Release
*       
*       Revision 28.41  93/08/16  14:21:32  roberte
*       Added fudge factor to bounding box calculation when compound character.
*       Moved INLC_ISW conditional assignment to code section of sp_do_make_char.
*       Added INCL_QUICK flag for REENTRANT_ALLOC declaration of intercepts_t intercepts.
*       
*       Revision 28.40  93/05/21  11:01:47  roberte
*       Removed two inadvertantly nested commetns.
*       
*       Revision 28.39  93/04/06  12:29:55  roberte
*       Added || INCL_WHITE in REENTRANT_ALLOC section declaring and assigning
*       intercepts on the stack.
*       
*       Revision 28.38  93/04/06  10:10:47  roberte
*       Corrected assignments of sp_globals.processor.speedo.plaid when REENTRANT_ALLOC.
*       
*       Revision 28.37  93/03/15  12:44:04  roberte
*       Release
*       
*       Revision 28.15  93/03/10  17:05:13  roberte
*          metric_resolution moved from union struct to common area. Oops!
*       
*       Revision 28.14  93/01/29  15:45:54  roberte
*       Changed references to specs_valid to reflect change to common area of SPEEDO_GLOBALS.
*       
*       Revision 28.13  93/01/29  11:03:29  roberte
*       Changed reference to sp_globals.plaid to reflect its' move from union struct to common area.
*       
*       Revision 28.12  93/01/29  09:08:31  roberte
*       Added cast of "tmp" in call to sp_plaid_tcb()
*       
*       Revision 28.11  93/01/12  09:45:57  roberte
*       Corrected unfortunate mis-spelling of sp_do_make_char caused
*       by sed's blindness.
*       
*       Revision 28.10  93/01/08  14:02:21  roberte
*       Changed references to sp_globals. for pspecs, orus_per_em, curves_out, multrnd, pixfix and mpshift.
*       
*       Revision 28.9  93/01/07  12:02:37  roberte
*       Corrected references for intercepts related items moved from union to common area of SPEEDO_GLOBALS.
*       
*       Revision 28.8  93/01/04  16:20:24  roberte
*       Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
*       
*       Revision 28.7  92/12/30  17:42:57  roberte
*       Functions no longer renamed in spdo_prv.h now declared with "sp_"
*       Use PARAMS1&2 macros throughout.
*       
*       Revision 28.6  92/11/24  10:54:36  laurar
*       include fino.h
*       
*       Revision 28.5  92/11/19  15:12:50  roberte
*       Release
*       
*       Revision 28.3  92/10/29  11:52:31  roberte
*       Inserted STACKFAR macro before references to pchar_data and pchar_data.
*       
*       Revision 28.2  92/09/17  10:10:15  weili
*       In sp_make_comp_char(PARAMS1), added some code to adjust the bounding box with
*       the imported setwidth if the character is rotated.
*       
*       Revision 28.1  92/06/25  13:38:05  leeann
*       Release
*       
*       Revision 27.1  92/03/23  13:54:26  leeann
*       Release
*       
*       Revision 26.1  92/01/30  16:55:52  leeann
*       Release
*       
*       Revision 25.1  91/07/10  11:02:32  leeann
*       Release
*       
*       Revision 24.1  91/07/10  10:36:02  leeann
*       Release
*       
*       Revision 23.1  91/07/09  17:56:29  leeann
*       Release
*       
*       Revision 22.1  91/01/23  17:15:40  leeann
*       Release
*       
*       Revision 21.2  91/01/21  18:04:43  leeann
*       make pointer in get_char_org a FONTFAR
*       
*       Revision 21.1  90/11/20  14:35:42  leeann
*       Release
*       
*       Revision 20.1  90/11/12  09:20:07  leeann
*       Release
*       
*       Revision 19.1  90/11/08  10:17:59  leeann
*       Release
*       
*       Revision 18.2  90/11/06  19:02:02  leeann
*       fix bugs when combining imported setwidth and squeezing
*       
*       Revision 18.1  90/09/24  09:50:30  mark
*       Release
*       
*       Revision 17.4  90/09/19  18:10:20  leeann
*       make preview_bounding_box visible when squeezing
*       
*       Revision 17.3  90/09/17  10:49:00  mark
*       Change INCL_WHITE conditional for allocation of sp_intercepts 
*       to INCL_SCREEN, so that it is used.
*       
*       Revision 17.2  90/09/14  15:01:06  leeann
*       changes for imported setwidth = 1 into src area
*       
*       Revision 17.1  90/09/13  15:56:56  mark
*       Release name rel0913
*       
*       Revision 16.2  90/09/13  15:13:55  leeann
*       allow imported widths of zero
*       
*       Revision 16.1  90/09/11  12:54:03  mark
*       Release
*       
*       Revision 15.2  90/09/05  11:29:03  leeann
*       recalculate constants when necessary for precision
*       with imported setwidths
*       
*       Revision 15.1  90/08/29  10:02:25  mark
*       Release name rel0829
*       
*       Revision 14.2  90/08/23  16:14:17  leeann
*       for imported set width, check if the imported width
*       is greater than the max in the font, and if it is,
*       reset the maximum.
*       
*       Revision 14.1  90/07/13  10:37:50  mark
*       Release name rel071390
*       
*       Revision 13.1  90/07/02  10:36:48  mark
*       Release name REL2070290
*       
*       Revision 12.3  90/06/26  08:55:59  leeann
*       compute squeezed bounding box for composite characters
*       before the characters are generated
*       
*       Revision 12.2  90/06/06  18:05:43  leeann
*       Correct parameter to sp_make_char_isw
*       
*       Revision 12.1  90/04/23  12:11:10  mark
*       Release name REL20
*       
*       Revision 11.1  90/04/23  10:11:05  mark
*       Release name REV2
*       
*       Revision 10.10  90/04/23  09:42:14  mark
*       fix declaration of do_make_char to match reentrant requirements
*       
*       Revision 10.9  90/04/21  10:47:12  mark
*       if multidevice support is enabled, make sure that the
*       device is specified before imaging characters.
*       
*       Revision 10.8  90/04/18  10:53:58  mark
*       add function sp_get_char_bbox
*       
*       Revision 10.7  90/04/17  09:32:51  leeann
*       make imported setwidth parameter 1/65536 of a pixel
*       
*       Revision 10.6  90/04/11  13:07:15  leeann
*       change squeezing compilation option to INCL_SQUEEZING,
*       include make_char_isw function
*       
*       Revision 10.5  90/04/05  15:14:24  leeann
*       set squeezing_compound flag apropriately
*       
*       Revision 10.4  90/03/29  16:43:16  leeann
*       Added set_flags argument to read_bbox
*       Added SQUEEZE code to make_simp_char to read_bbox
*       before calling plaid_tcb
*       
*       Revision 10.3  90/03/26  15:48:42  mark
*       calculate subpixel setwidth values for normal and compound characters
*       using metric_resolution and specs.xxmult
*       also change get_char_width similarly
*       
*       Revision 10.2  89/09/11  11:39:10  mark
*       correct declaration of stackfar pointer to fontfar pointer argument
*       in functions sp_get_posn_arg and sp_get_scale_arg so that code works
*       with Microsoft C when stackfar and fontfar are not equivalent.
*       *** EOF *** 
*       
*       Revision 10.1  89/07/28  18:07:36  mark
*       Release name PRODUCT
*       
*       Revision 9.1  89/07/27  10:20:41  mark
*       Release name PRODUCT
*       
*       Revision 8.1  89/07/13  18:17:24  mark
*       Release name Product
*       
*       Revision 7.1  89/07/11  08:58:56  mark
*       Release name PRODUCT
*       
*       Revision 6.2  89/07/09  11:37:08  mark
*       only allocate intercept lists if one of the bitmap output
*       modules is included
*       
*       Revision 6.1  89/06/19  08:33:00  mark
*       Release name prod
*       
*       Revision 5.3  89/06/02  16:58:09  mark
*       In the reentrant model, allocate space for intercept
*       lists and plaid tables on the stack
*       
*       Revision 5.2  89/06/01  16:56:04  mark
*       only process outline data if begin_char returns TRUE
*       
*       Revision 5.1  89/05/01  17:50:55  mark
*       Release name Beta
*       
*       Revision 4.2  89/05/01  16:17:51  john
*       bug in sp_get_char_org(PARAMS1) corrected:
*       16 bit offsets in char directory now cast to unsigned
*       before being cast into fix31.
*       
*       Revision 4.1  89/04/27  12:11:30  mark
*       Release name Beta
*       
*       Revision 3.1  89/04/25  08:24:27  mark
*       Release name beta
*       
*       Revision 2.5  89/04/24  15:52:48  john
*       Two pixel allowance added to compound char
*       bounding box.
*       
*       Revision 2.4  89/04/12  13:17:24  mark
*       correct far pointer declarations of get_posn_args 
*       and get_scale_args
*       
*       Revision 2.3  89/04/12  12:10:52  mark
*       added stuff for far stack and font
*       
*       Revision 2.2  89/04/10  17:06:57  mark
*       Modified pointer declarations that are used to refer
*       to font data to use FONTFAR symbol, which will be used
*       for Intel SS != DS memory models
*       Also changed use of read_bbox and plaid_tcb to return 
*       updated pointer, rather than passing a pointer to a pointer
*       
*       Revision 2.1  89/04/04  13:31:13  mark
*       Release name EVAL
*       
*       Revision 1.10  89/04/04  13:16:21  mark
*       Update copyright text
*       
*       Revision 1.9  89/03/31  14:42:23  mark
*       Change speedo.h to spdo_prv.h
*       
*       Revision 1.8  89/03/31  12:14:14  john
*       modified to use new NEXT_WORD macro.
*       
*       Revision 1.7  89/03/30  17:47:21  john
*       sp_read_long(PARAMS1) moved to set_spcs.c
*       
*       Revision 1.6  89/03/29  16:07:47  mark
*       changes for slot independence and dynamic/reentrant
*       data allocation
*       
*       Revision 1.5  89/03/24  16:48:01  john
*       Direct access to character directory implemented.
*       
*       Revision 1.4  89/03/23  17:51:21  john
*       Added expansion joint to character header data
*       
*       Revision 1.3  89/03/23  11:50:59  john
*       Dynamic char data loader modified to correct compound character
*       bug. New offset arg added to sp_load_char_data(PARAMS1)
*       
*       Revision 1.2  89/03/21  13:25:27  mark
*       change name from oemfw.h to speedo.h
*       
*       Revision 1.1  89/03/15  12:28:44  mark
*       Initial revision
*                                                                                 *
*                                                                                    *
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/do_char.c,v 33.0 97/03/17 17:44:39 shawn Release $";
#endif



/***************************** D O - C H A R . C *****************************
 *                                                                           *
 * This is the top level module for processing one simple or composite       *
 * character.
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 15 Dec 88  jsc  Created                                               *
 *                                                                           *
 *  2) 27 Jan 89  jsc  fw_sp_get_char_id(PARAMS1) added.                               *
 *                     fw_sp_get_char_width(PARAMS1) added.                            *
 *                     make_simple_char() and sp_make_comp_char(PARAMS1) changed from  *
 *                     void to boolean to return FALSE if error              *
 *                                                                           *
 *  3) 30 Jan 89  jsc  fw_sp_get_char_width(PARAMS1) changed from ufix16 to fix31      *
 *                                                                           *
 *  4)  6 Feb 89  jsc  Conditional compilation of metrics functions.         *
 *                                                                           *
 *                     Conditional compilation of dynamic character data     *
 *                     loading.                                              *
 *                                                                           *
 *  5)  9 Feb 89  jsc  Kerning access functions added.                       *
 *                                                                           *
 *  6)  2 Mar 89  jsc  Corrected char index bounds checking bug.             *
 *                                                                           *
 ****************************************************************************/

#include "spdo_prv.h"               /* General definitions for Speedo    */
#include "fino.h"

#define   DEBUG   0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#if PROC_SPEEDO
/***** GLOBAL VARIABLES *****/

/*****  GLOBAL FUNCTIONS *****/

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/

#if INCL_MUTABLE
LOCAL_PROTO ufix8 FONTFAR *sp_get_char_adj_org(SPGLOBPTR2 ufix8 FONTFAR *p0);
#endif

#if  INCL_ISW       
LOCAL_PROTO btsbool sp_do_make_char(SPGLOBPTR2 ufix16 char_index);
LOCAL_PROTO btsbool sp_reset_xmax(SPGLOBPTR2 fix31 xmax);
#endif

#if INCL_SQUEEZING
GLOBAL_PROTO btsbool sp_calculate_x_scale(SPGLOBPTR2 fix31 *x_factor, fix31 *x_offset, fix15 no_X_ctrl_zones);
GLOBAL_PROTO btsbool sp_calculate_y_scale(SPGLOBPTR2 fix31 *top_scale, fix31 *bottom_scale, fix15 first_Y_zone, fix15 no_Y_ctrl_zones);
#endif

#if INCL_ISW || INCL_SQUEEZING
LOCAL_PROTO void sp_preview_bounding_box(SPGLOBPTR2 ufix8 FONTFAR *pp1, ufix8 FONTFAR *pp2, ufix8 format);
#endif

LOCAL_PROTO btsbool sp_make_simp_char(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2, ufix8 format);
LOCAL_PROTO btsbool sp_make_comp_char(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2);
LOCAL_PROTO ufix8 FONTFAR *sp_get_char_org(SPGLOBPTR2 ufix16 char_index, btsbool top_level);
LOCAL_PROTO fix15 sp_get_posn_arg(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1, ufix8 FONTFAR *STACKFAR *pp2, ufix8 format);
LOCAL_PROTO fix15 sp_get_scale_arg(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1, ufix8 FONTFAR *STACKFAR *pp2, ufix8 format);
LOCAL_PROTO ufix16 sp_get_alt_width(SPGLOBPTR2 ufix16 char_index, ufix16 resolutionReq, btsbool *success);


FUNCTION ufix16 sp_get_char_id(SPGLOBPTR2 ufix16 char_index)
/*
 * Returns character id for specified character index in currently
 * selected font.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 12 and returns 0 if character data not available.
 */
{
ufix8 FONTFAR  *pointer;      /* Pointer to character data */

if (!sp_globals.specs_valid)     /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return (ufix16)0;            /* Return zero character id */
    }

pointer = sp_get_char_org(PARAMS2 char_index, TRUE); /* Get pointer to character data */
if (pointer == BTSNULL)             /* Character data not available? */
    {
    sp_report_error(PARAMS2 12);            /* Report character data not avail */
    return (ufix16)0;            /* Return zero character id */
    }

return (ufix16)NEXT_WORD(pointer); /* Return character id */
}



#if INCL_METRICS

FUNCTION fix31 sp_get_char_width(SPGLOBPTR2 ufix16 char_index)
/*
 * Returns character set width for specified character index in currently
 * selected font in units of 1/65536 em.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 12 and returns 0 if character data not available.
 */
{
fix31    set_width;    /* Set width of character */
ufix16 emRes;
	set_width = sp_get_char_width_orus(PARAMS2 char_index, &emRes);
	if (set_width)
		set_width = ((set_width << 16) + (emRes >> 1)) / emRes;
	return (set_width);
}

FUNCTION fix31 sp_get_char_width_orus(SPGLOBPTR2 ufix16 char_index, ufix16 *emResolution)
/*
 * Returns character set width for specified character index in currently
 * selected font in ORU units.
 * Set *emResoution to font ORU res.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 12 and returns 0 if character data not available.
 */
{
ufix8 FONTFAR  *p0;             /* Main character data origin */
ufix8 FONTFAR  *p1;             /* Pointer to main character data */
fix31    set_width;				/* Set width of character */

#if INCL_MUTABLE
ufix8 FONTFAR  *p2;             /* Pointer to character adjustment data */
#endif

#if INCL_MFT
btsbool success;
#endif

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return (fix31)0;             /* Return zero character width */
    }

#if INCL_MFT
if ( (sp_globals.processor.speedo.resolutionReq != sp_globals.metric_resolution) &&
	 /* take a look in possible alternate width table : */
	 ((set_width = (fix31)sp_get_alt_width(PARAMS2 char_index, sp_globals.processor.speedo.resolutionReq, &success)) || success )
	)
	{
	*emResolution = sp_globals.processor.speedo.resolutionReq;
#if INCL_MUTABLE
	/* must set up this pointer p0 for INCL_MUTABLE build anyway */
	p0 = sp_get_char_org(PARAMS2 char_index, TRUE); /* Get pointer to character data */
	if (p0 == BTSNULL)             /* Character data not available? */
   		{
   		sp_report_error(PARAMS2 12);            /* Report character data not avail */
   		return (fix31)0;             /* Return zero character width */
   		}
#endif
	}
else	/* try getting set width from the character data itself */
#endif
	{
	p0 = sp_get_char_org(PARAMS2 char_index, TRUE); /* Get pointer to character data */
	if (p0 == BTSNULL)             /* Character data not available? */
   		{
   		sp_report_error(PARAMS2 12);            /* Report character data not avail */
   		return (fix31)0;             /* Return zero character width */
   		}
	p1 = p0 +2;                    /* Skip over character id */
	set_width = (fix31)NEXT_WORD(p1); /* Read set width  and Convert units */
	*emResolution = sp_globals.metric_resolution;
	}

#if INCL_MUTABLE
p2 = sp_get_char_adj_org(PARAMS2 p0);              /* Get origin of auxiliary char data */
set_width += sp_get_adjustment(PARAMS2 &p2);       /* Adjust set width for interpolation */
#endif
return set_width;                /* Return in ORU em units */
}
#endif


#if INCL_METRICS

FUNCTION fix15 sp_get_track_kern(SPGLOBPTR2 fix15 track, fix15 point_size)
/*
 * Returns inter-character spacing adjustment in units of 1/256
 * points for the specified kerning track and point size.
 * If the specified point size is larger than the maximum point
 * size for the specified track, the adjustment for the maximum
 * point size is used.
 * If the specified point size is smaller than the minimum point
 * size for the specified track, the adjustment for the minimum
 * point size is used.
 * If the specified point size is between the minimum point size
 * and the maximum point size for the specified track, the 
 * adjustment is interpolated linearly between the minimum and
 * maximum adjustments.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 13 and returns 0 if track kerning data not in font.
 */
{
ufix8 FONTFAR   *p1;      /* Pointer to character data */
fix15    no_tracks;    /* Number of kerning tracks in font */
ufix8    format;       /* Track kerning format byte */
fix15    i;            /* Track counter */
fix15    min_pt_size;  /* Minimum point size for track */
fix15    max_pt_size;  /* Maximum point size for track */
fix15    min_adj;      /* Adjustment for min point size */
fix15    max_adj;      /* Adjustment for max point size */
fix31    delta_pt_size;/* Max point size - min point size */
fix31    delta_adj;    /* Min adjustment - max adjustment */
fix15    adj = 0;      /* Interpolated adjustment */

#if INCL_MUTABLE
ufix16  pub_hdr_size;           /* Public header size (bytes) */
ufix16  pvt_hdr_size;           /* Private header size (bytes) */
fix31   offset;                 /* Offset to start of interpolation term mask data */
ufix8 FONTFAR  *p2;             /* Pointer to character adjustment data */
#endif

if (track == 0)                  /* Track zero selected? */
    {
    return adj;                  /* Return zero track kerning adjustment */
    }

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return adj;                  /* Return zero track kerning adjustment */
    }

no_tracks = sp_globals.processor.speedo.kern.no_tracks;      /* Number of kerning tracks */
if (track > no_tracks)           /* Required track not available? */
    {
    sp_report_error(PARAMS2 13);            /* Report track kerning data not avail */
    return adj;                  /* Return zero track kerning adjustment */
    }

p1 =  sp_globals.processor.speedo.kern.tkorg;            /* Point to start of track kern data */
#if INCL_MUTABLE
pub_hdr_size = sp_globals.processor.speedo.hdr2_org - sp_globals.processor.speedo.font_org;
pvt_hdr_size = 
    (pub_hdr_size >= (FH_PVHSZ + 2))?
    sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_PVHSZ):
    FH_NBYTE + 3;
if ((sp_globals.processor.speedo.nIntTerms == 0) ||      /* Mutability not active? */
    (pub_hdr_size < FH_NDSNA + 1) ||    /* Not a Speedo-M font? */
    (pvt_hdr_size < FH_OFTAD + 3) ||    /* No track kerning adjustment data? */
    (*(sp_globals.processor.speedo.font_org + FH_NDSNA) < 1)) /* Zero design axes? */
    {
    p2 = BTSNULL;
    }
else
    {
    offset = sp_read_long(PARAMS2 sp_globals.processor.speedo.hdr2_org + FH_OFTAD);
    p2 = sp_globals.processor.speedo.font_org + offset;  /* Point to track kerning adjustment data */
    }
#endif
for (i = 0; i < track; i++)      /* Read until track required is read */
    {
    format = NEXT_BYTE(p1); /* Read track kerning format byte */
    min_pt_size = (format & BIT0)? 
        NEXT_WORD(p1):
        (fix15)NEXT_BYTE(p1);
    min_adj = (format & BIT1)? 
        NEXT_WORD(p1):
        (fix15)NEXT_BYTE(p1);
#if INCL_MUTABLE
    min_adj += sp_get_adjustment(PARAMS2 &p2);     /* Adjust min adjustment for interpolation */
#endif
    max_pt_size = (format & BIT2)? 
        NEXT_WORD(p1):
        (fix15)NEXT_BYTE(p1);
    max_adj = (format & BIT3)? 
        NEXT_WORD(p1):
        (fix15)NEXT_BYTE(p1);
#if INCL_MUTABLE
    max_adj += sp_get_adjustment(PARAMS2 &p2);     /* Adjust max adjustment for interpolation */
#endif
    }

if (point_size <= min_pt_size)   /* Smaller than minimum point size? */
    {
    return min_adj;              /* Return minimum adjustment (1/256 points) */
    }

if (point_size >= max_pt_size)   /* Larger than maximum point size? */
    {
    return max_adj;              /* Return maximum adjustment (1/256 points) */
    }

delta_pt_size = (fix31)(max_pt_size - min_pt_size);
delta_adj = (fix31)(min_adj - max_adj);
adj = (fix15)(min_adj - 
       (((fix31)(point_size - min_pt_size) * delta_adj + 
         (delta_pt_size >> 1)) / delta_pt_size));
return adj;                      /* Return interpolated adjustment (1/256 points) */
}
#endif


#if INCL_METRICS

FUNCTION fix31 sp_get_pair_kern(SPGLOBPTR2 ufix16 char_index1, ufix16 char_index2)
/*
 * Returns inter-character spacing adjustment in units of 1/65536 em
 * for the specified pair of characters.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 14 and returns 0 if pair kerning data not in font.
 */
{
ufix8 FONTFAR  *p0;       /* Pointer to first kerning pair record */
ufix8 FONTFAR  *p1;      /* Pointer to character data */
ufix16   tmpufix16;    /* Temporary workspace */
fix15    no_pairs;     /* Number of kerning pairs in font */
ufix8    format;       /* Track kerning format byte */
btsbool  long_id;      /* TRUE if 2-byte character ids */
fix15    rec_size;     /* Number of bytes in kern pair record */
fix15    n;            /* Number of remaining kern pairs */
fix15    nn;           /* Number of kern pairs in first partition */
fix15    base;         /* Index to first record in rem kern pairs */
fix15    i;            /* Index to kern pair being tested */
fix31    adj = 0;      /* Returned value of adjustment */
fix15    adj_base;     /* Adjustment base for relative adjustments */

#if INCL_MUTABLE
fix15   ii;
ufix16  pub_hdr_size;            /* Public header size (bytes) */
ufix16  pvt_hdr_size;            /* Private header size (bytes) */
fix31   offset;                  /* Offset to start of interpolation term mask data */
ufix8 FONTFAR  *p2;              /* Pointer to character adjustment data */
#endif

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return adj;                  /* Return zero pair kerning adjustment */
    }

no_pairs = sp_globals.processor.speedo.kern.no_pairs;        /* Number of kerning pairs */
if (no_pairs == 0)               /* Pair kerning data not available? */
    {
    sp_report_error(PARAMS2 14);            /* Report pair kerning data not avail */
    return adj;                  /* Return zero pair kerning adjustment */
    }

p0 = sp_globals.processor.speedo.kern.pkorg;            /* Point to start of pair kern data */
format = NEXT_BYTE(p0);     /* Read pair kerning format byte */
if (!(format & BIT0))            /* One-byte adjustment values? */
    adj_base = NEXT_WORD(p0); /* Read base adjustment */
p1 = p0;                /* First byte of kerning pair data */
rec_size = format + 3;           /* Compute kerning pair record size */
long_id = format & BIT1;         /* Set flag for 2-byte char index */

n = no_pairs;                    /* Consider all kerning pairs */
base = 0;                        /* Set base at first kern pair record */
while (n != 0)                   /* While 1 or more kern pairs remain ... */
    {
    nn = n >> 1;                 /* Size of first partition */
    i = base + nn;               /* Index to record to be tested */
    p0 = p1 + (i * rec_size);
    tmpufix16 = NEXT_CHNDX(p0, long_id);
    if (char_index1 < tmpufix16)
        {
        n = nn;                  /* Number remaining in first partition */
        continue;
        }
    if (char_index1 > tmpufix16)
        {
        n -= nn + 1;             /* Number remaining in second partition */
        base = i + 1;            /* Base index for second partition */
        continue;
        }
    tmpufix16 = NEXT_CHNDX(p0, long_id);
    if (char_index2 < tmpufix16)
        {
        n = nn;                  /* Number remaining in first partition */
        continue;
        }
    if (char_index2 > tmpufix16)
        {
        n -= nn + 1;             /* Number remaining in second partition */
        base = i + 1;            /* Base index for second partition */
        continue;
        }
    adj = (format & BIT0)? 
        (fix31)NEXT_WORD(p0):
        (fix31)(adj_base + (fix15)NEXT_BYTE(p0));
#if INCL_MUTABLE
    pub_hdr_size = sp_globals.processor.speedo.hdr2_org - sp_globals.processor.speedo.font_org;
    pvt_hdr_size = 
        (pub_hdr_size >= (FH_PVHSZ + 2))?
        sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_PVHSZ):
        FH_NBYTE + 3;
    if ((sp_globals.processor.speedo.nIntTerms == 0) ||  /* Mutability not active? */
        (pub_hdr_size < FH_NDSNA + 1) || /* Not a Speedo-M font? */
        (pvt_hdr_size < FH_OFPAD + 3) || /* No pair kerning adjustment data? */
        (*(sp_globals.processor.speedo.font_org + FH_NDSNA) < 1)) /* Zero design axes? */
        {
        }
    else                                /* Adjustment vector execution required? */
        {
        offset = sp_read_long(PARAMS2 sp_globals.processor.speedo.hdr2_org + FH_OFPAD);
        p2 = sp_globals.processor.speedo.font_org + offset; /* Point to pair kerning adjustment data */
        p2 += i >> 8;                   /* Point to entry in adjustment vector index */
        p2 = sp_globals.processor.speedo.font_org + offset + NEXT_WORD(p2); /* Point to adj vector group */
        n = i & 0x0007;                 /* Position in adjustment vector group */
        for (ii = 0; ii < n; ii++)      /* For each adjustment vector to be skipped... */
            sp_get_adjustment(PARAMS2 &p2);        /* Skip it */
        adj += sp_get_adjustment(PARAMS2 &p2);     /* Execute required adjustment vector */
        }
#endif

    adj = ((adj << 16) + (sp_globals.orus_per_em >> 1)) / sp_globals.orus_per_em; /* Convert units */
    n = 0;                       /* No more to consider */
    }
return adj;                      /* Return pair kerning adjustment */
}
#endif



#if INCL_METRICS

FUNCTION btsbool sp_get_char_bbox(SPGLOBPTR2 ufix16 char_index, bbox_t *bbox)
{
/*
 *	returns true if character exists, false if it doesn't
 *	provides transformed character bounding box in 1/65536 pixels
 *	in the provided bbox_t structure.  Bounding box may be
 *	conservative in the event that the transformation is not
 *	normal or the character is compound.
 */

ufix8 FONTFAR *p0;
ufix8 FONTFAR *p1;
ufix8 FONTFAR  *p2;             /* Pointer to character adjustment data */
fix15 tmp;
fix15    pix_adj = 0;      /* Pixel adjustment to compound char bounding box */
ufix8    format;
point_t Pmin, Pmax;

#if REENTRANT_ALLOC
plaid_t plaid;
sp_globals.processor.speedo.plaid = &plaid;
#endif

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return FALSE;                /* Error return */
    }

sp_init_tcb(PARAMS1);                      /* Initialize transformation control block */

p0 = sp_get_char_org(PARAMS2 char_index, TRUE); /* Point to start of character data */
if (p0 == BTSNULL)             /* Character data not available? */
    {
    sp_report_error(PARAMS2 12);            /* Report character data not avail */
    return FALSE;                /* Error return */
    }

#if INCL_MUTABLE
p2 = sp_get_char_adj_org(PARAMS2 p0);              /* Get origin of auxiliary char data */
#endif

p1 = p0 + 4;					/* Skip over character id, set width */
               
tmp = format = NEXT_BYTE(p1);
if (format & BIT1)               /* Optional data in header? */
    {
    tmp = (ufix8)NEXT_BYTE(p1); /* Read size of optional data */
    p1 += tmp;         /* Skip optional data */
    }

sp_plaid_tcb(PARAMS2 &p1, &p2, (ufix8)tmp);              /* Process plaid data */
sp_read_bbox(PARAMS2 &p1, &p2, &Pmin, &Pmax,(btsbool)FALSE);        /* Read bounding box */
if (format & BIT0)
	{
	pix_adj = sp_globals.onepix << 1;          /* Allow 2 pixel expansion ... */
	Pmin.x -= pix_adj;                         /* ... of components of ... */
	Pmin.y -= pix_adj;                         /* ... compound ... */
	Pmax.x += pix_adj;                         /* ... character ... */
	Pmax.y += pix_adj;                         /* ... bounding box. */
	}
bbox->xmin  = (fix31)Pmin.x << sp_globals.poshift;
bbox->xmax  = (fix31)Pmax.x << sp_globals.poshift;
bbox->ymin  = (fix31)Pmin.y << sp_globals.poshift;
bbox->ymax  = (fix31)Pmax.y << sp_globals.poshift;
return TRUE;
}
#endif



#if INCL_ISW

FUNCTION btsbool sp_make_char_isw(SPGLOBPTR2 ufix16 char_index, ufix32 imported_setwidth)
{
fix15   xmin;          /* Minimum X ORU value in font */
fix15   xmax;          /* Maximum X ORU value in font */
fix15   ymin;          /* Minimum Y ORU value in font */
fix15   ymax;          /* Maximum Y ORU value in font */
ufix16  return_value;

sp_globals.processor.speedo.import_setwidth_act = TRUE;
/* convert imported width to orus */
sp_globals.processor.speedo.imported_width = (sp_globals.metric_resolution * 
			    imported_setwidth) >> 16;
return_value = sp_do_make_char(PARAMS2 char_index);

if (sp_globals.processor.speedo.isw_modified_constants)
    {
    /* reset fixed point constants */
    xmin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FXMIN);
    ymin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMIN);
    ymax = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMAX);
    sp_globals.processor.speedo.constr.data_valid = FALSE;
    xmax = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FXMAX);
    if (!sp_setup_consts(PARAMS2 xmin,xmax,ymin,ymax))
        {
        sp_report_error(PARAMS2 3);           /* Requested specs out of range */
        return FALSE;
        }
    }    
return (return_value);
}

FUNCTION btsbool sp_make_char(SPGLOBPTR2 ufix16 char_index)
{
sp_globals.processor.speedo.import_setwidth_act = FALSE;
return (sp_do_make_char(PARAMS2 char_index));
}

FUNCTION LOCAL_PROTO btsbool sp_do_make_char(SPGLOBPTR2 ufix16 char_index)
#else
FUNCTION btsbool sp_make_char(SPGLOBPTR2 ufix16 char_index)
#endif
/*
 * Outputs specified character using the currently selected font and
 * scaling and output specifications.
 * Reports Error 10 and returns FALSE if no font specifications 
 * previously set.
 * Reports Error 12 and returns FALSE if character data not available.
 */
{
ufix8 FONTFAR  *p0;      /* Pointer to character data */
ufix8 FONTFAR  *p1;      /* Pointer to character data */
ufix8 FONTFAR  *p2;      /* Pointer to character adjustment data */
fix15    x_orus;
fix15    tmpfix15;
ufix8    format;
fix15    metric_resolution = sp_globals.metric_resolution;

#if INCL_MFT
btsbool success;
#endif

#if REENTRANT_ALLOC
plaid_t plaid;
#if INCL_BLACK || INCL_SCREEN || INCL_2D || INCL_WHITE || INCL_QUICK || INCL_GRAY
intercepts_t intercepts;
sp_globals.intercepts = &intercepts;
#endif
#endif

#if REENTRANT_ALLOC
sp_globals.processor.speedo.plaid = &plaid;
#endif

#if INCL_ISW
sp_globals.processor.speedo.isw_modified_constants = FALSE;
#endif

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    sp_report_error(PARAMS2 10);            /* Report font not specified */
    return FALSE;                /* Error return */
    }

#if INCL_MULTIDEV
#if INCL_OUTLINE
if (sp_globals.output_mode == MODE_OUTLINE && !sp_globals.outline_device_set)
	{
	sp_report_error(PARAMS2 2);
	return FALSE;
	}
else
#endif
	if (!sp_globals.bitmap_device_set)
		{
		sp_report_error(PARAMS2 2);
		return FALSE;
		}
#endif


sp_init_tcb(PARAMS1);                      /* Initialize transformation control block */

p0 = sp_get_char_org(PARAMS2 char_index, TRUE); /* Point to start of character data */
SHOW(p0);
if (p0 == BTSNULL)             /* Character data not available? */
    {
    sp_report_error(PARAMS2 12);            /* Report character data not avail */
    return FALSE;                /* Error return */
    }


#if INCL_MUTABLE
p2 = sp_get_char_adj_org(PARAMS2 p0);              /* Get origin of auxiliary char data */
#else
p2 = 0;
#endif

p1 = p0 + 2;					/* Skip over character id */
#if INCL_MFT
if ( (sp_globals.processor.speedo.resolutionReq != sp_globals.metric_resolution) &&
	 /* take a look in possible alternate width table : */
	 ((x_orus = (fix31)sp_get_alt_width(PARAMS2 char_index, sp_globals.processor.speedo.resolutionReq, &success)) || success )
	)
	{
	metric_resolution = sp_globals.processor.speedo.resolutionReq;
	p1 += sizeof(ufix16);
	}
else	/* get set width from the character data itself */
#endif
x_orus = NEXT_WORD(p1); /* Read set width */
#if INCL_MUTABLE
x_orus += sp_get_adjustment(PARAMS2 &p2);          /* Adjust set width for interpolation */
#endif
#if INCL_SQUEEZING || INCL_ISW
sp_globals.processor.speedo.setwidth_orus = x_orus;
#endif

#if INCL_ISW
if (sp_globals.processor.speedo.import_setwidth_act)
    x_orus = sp_globals.processor.speedo.imported_width;
#endif
sp_globals.processor.speedo.Psw.x = (fix15)((fix31)
                   (((fix31)x_orus * (sp_globals.specs.xxmult>>16) + 
                  ( ((fix31)x_orus * (sp_globals.specs.xxmult&0xffffL) )>>16) 
                  ) << sp_globals.pixshift) / metric_resolution);

sp_globals.processor.speedo.Psw.y = (fix15)(   
		  (fix31)( 
                 ((fix31)x_orus * (sp_globals.specs.yxmult>>16) + 
                ( ((fix31)x_orus * (sp_globals.specs.yxmult&0xffffL) )>>16) 
                  ) << sp_globals.pixshift) / metric_resolution);
               
format = NEXT_BYTE(p1);
if (format & BIT0)
	{/* check all compound chars for REMOTE */
	if ((format & BIT5) && !(sp_globals.FourIn1Flags & BIT0))
		{/* Older processors won't be checking these bits. They mean compound REMOTE data */
		return(FALSE);	/* but no error should be reported */
		}
	}
if (format & BIT1)               /* Optional data in header? */
    {
    tmpfix15 = (ufix8)NEXT_BYTE(p1); /* Read size of optional data */
    p1 += tmpfix15;         /* Skip optional data */
    }
if (format & BIT0)
    {
    return sp_make_comp_char(PARAMS2 p1, p2); /* Output compound character */
    }
else
    {
    return sp_make_simp_char(PARAMS2 p1, p2, format); /* Output simple character */
    }
}


FUNCTION LOCAL_PROTO btsbool sp_make_simp_char(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2, ufix8 format)
/*
 * Called by sp_make_char(PARAMS1) to output a simple (non-compound) character.
 * Returns TRUE on completion.
 */
{
point_t Pmin, Pmax;    /* Transformed corners of bounding box */

#if INCL_SQUEEZING || INCL_ISW
ufix8 FONTFAR *save_p1;
#endif

#if INCL_ISW
fix31   char_width;
fix31   isw_scale;
#endif

#if INCL_SQUEEZING
sp_globals.processor.speedo.squeezing_compound = FALSE;
if ((sp_globals.pspecs->flags & SQUEEZE_LEFT) ||
    (sp_globals.pspecs->flags & SQUEEZE_RIGHT) ||
    (sp_globals.pspecs->flags & SQUEEZE_TOP) ||
    (sp_globals.pspecs->flags & SQUEEZE_BOTTOM) )
    {
	/* get the bounding box data before processing the character */
    save_p1 = p1;
    sp_preview_bounding_box (PARAMS2 p1, p2, format);
    p1 = save_p1;
    }
#endif
#if (INCL_ISW)
if (sp_globals.processor.speedo.import_setwidth_act)
    {
    save_p1 = p1;
    sp_preview_bounding_box (PARAMS2 p1, p2, format);
    p1 = save_p1;
        /* make sure I'm not going to get fixed point overflow */
    isw_scale = sp_compute_isw_scale(PARAMS1);
    if (sp_globals.processor.speedo.bbox_xmin_orus < 0)
        char_width = SQUEEZE_MULT((sp_globals.processor.speedo.bbox_xmax_orus - sp_globals.processor.speedo.bbox_xmin_orus), isw_scale);
    else
	char_width = SQUEEZE_MULT(sp_globals.processor.speedo.bbox_xmax_orus, isw_scale);
    if (char_width >= sp_globals.processor.speedo.isw_xmax)
        if (!sp_reset_xmax(PARAMS2 char_width))
              return FALSE;
    }
#endif
sp_plaid_tcb(PARAMS2 &p1, &p2, format);              /* Process plaid data */
sp_read_bbox(PARAMS2 &p1, &p2, &Pmin, &Pmax, FALSE);      /* Read bounding box */
if (fn_begin_char(sp_globals.processor.speedo.Psw, Pmin, Pmax))     /* Signal start of character output */
	{
	do
    	{
	    sp_proc_outl_data(PARAMS2 p1, p2);              /* Process outline data */
    	}
	while (!fn_end_char());                      /* Repeat if not done */
	}
return TRUE;
}


FUNCTION LOCAL_PROTO btsbool sp_make_comp_char(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2)
/*
 * Called by sp_make_char(PARAMS1) to output a compound character.
 * Returns FALSE if data for any sub-character is not available.
 * Returns TRUE if output completed with no error.
 */
{
point_t  Pmin, Pmax;   /* Transformed corners of bounding box */
point_t  Pssw;         /* Transformed escapement vector */
ufix8 FONTFAR  *p1_sav;         /* Saved pointer to compound character data */
ufix8 FONTFAR  *p2_sav;         /* Saved pointer to cmpd char adjustment vector */
ufix8 FONTFAR  *sub_p1;         /* Pointer to sub-character data */
ufix8 FONTFAR  *sub_p2;         /* Pointer to sub-character adjustment vector */
ufix8    format;       /* Format of DOCH instruction */
ufix16   sub_char_index; /* Index to sub-character in character directory */
fix15    x_posn;       /* X position of sub-character (outline res units) */
fix15    y_posn;       /* Y position of sub-character (outline res units) */
fix15    x_scale;      /* X scale factor of sub-character (scale units) */
fix15    y_scale;      /* Y scale factor of sub-character (scale units) */
fix15    tmpfix15;     /* Temporary workspace */
fix15    x_orus;       /* Set width in outline resolution units */
fix15    pix_adj;      /* Pixel adjustment to compound char bounding box */

#if INCL_SQUEEZING
fix31    x_factor, x_offset, top_scale, bottom_scale;
btsbool  squeezed_x, squeezed_y;
#endif

#if INCL_SQUEEZING || INCL_ISW
fix15    x_offset_pix;
#endif

#if INCL_ISW
fix31   char_width;
fix31   isw_scale;
#endif


#if INCL_SQUEEZING
sp_globals.processor.speedo.squeezing_compound = TRUE;
#endif

sp_read_bbox(PARAMS2 &p1, &p2, &Pmin, &Pmax, TRUE); /* Read bounding box data */
pix_adj = sp_globals.onepix << 1;          /* Allow 2 pixel expansion ... */
Pmin.x -= pix_adj;                         /* ... of components of ... */
Pmin.y -= pix_adj;                         /* ... compound ... */
Pmax.x += pix_adj;                         /* ... character ... */
Pmax.y += pix_adj;                         /* ... bounding box. */

#if INCL_SQUEEZING
/* scale the bounding box if necessary before calling begin_char */
squeezed_x = sp_calculate_x_scale(PARAMS2 &x_factor, &x_offset, 0);
squeezed_y = sp_calculate_y_scale(PARAMS2 &top_scale, &bottom_scale,0,0);

if (squeezed_x)
    { /* scale the x coordinates of the bbox */
    x_offset_pix = (fix15)(((x_offset >> 16) * sp_globals.tcb0.xppo)
                    >> sp_globals.mpshift);
    if ((x_offset_pix >0) && (x_offset_pix < sp_globals.onepix))
        x_offset_pix = sp_globals.onepix;
    Pmin.x = SQUEEZE_MULT (x_factor, Pmin.x) + x_offset_pix - pix_adj;
    Pmax.x = SQUEEZE_MULT (x_factor, Pmax.x) + x_offset_pix + pix_adj;
    }
if (squeezed_y)
    { /* scale the y coordinates of the bbox */
    if ((Pmin.y) < 0)
        Pmin.y = SQUEEZE_MULT (bottom_scale, Pmin.y) - pix_adj;
    else
        Pmin.y = SQUEEZE_MULT (top_scale, Pmin.y) - pix_adj;
    if ((Pmax.y) < 0)
        Pmax.y = SQUEEZE_MULT (bottom_scale, Pmax.y) + pix_adj;
    else
        Pmax.y = SQUEEZE_MULT (top_scale, Pmax.y) + pix_adj;
    }
#endif
#if (INCL_ISW)
if (sp_globals.processor.speedo.import_setwidth_act)
    {
        /* make sure I'm not going to get fixed point overflow */
    isw_scale = ((fix31)sp_globals.processor.speedo.imported_width << 16)/
                 (fix31)sp_globals.processor.speedo.setwidth_orus;
    char_width = SQUEEZE_MULT((sp_globals.processor.speedo.bbox_xmax_orus - 
                               sp_globals.processor.speedo.bbox_xmin_orus),
isw_scale);
    
    /*	Adjust the bounding box with isw if the character is rotated  */
    if (sp_globals.tcb0.xtype == 0 || sp_globals.tcb0.xtype == 1)
    {
	Pmax.x = SQUEEZE_MULT (Pmax.x, isw_scale);
	Pmin.x = SQUEEZE_MULT (Pmin.x, isw_scale);
    }
    if (sp_globals.tcb0.ytype == 2 || sp_globals.tcb0.ytype == 3)
    {
	Pmax.y = SQUEEZE_MULT (Pmax.y, isw_scale);
	Pmin.y = SQUEEZE_MULT (Pmin.y, isw_scale);
    }

    if (char_width >= sp_globals.processor.speedo.isw_xmax)
        if (!sp_reset_xmax(PARAMS2 char_width))
              return FALSE;
    }
#endif

if (fn_begin_char(sp_globals.processor.speedo.Psw, Pmin, Pmax)) /* Signal start of character data */
	{
	p1_sav = p1;
	p2_sav = p2;
	do
	    {
	    p1 = p1_sav;                 /* Point to next DOCH or END instruction */
	    p2 = p2_sav;                    /* Point to first adjustment vector */
	    while (format = NEXT_BYTE(p1))    /* DOCH instruction? */
	        {
	        sp_init_tcb(PARAMS1);                        /* Initialize transformation control block */
	        x_posn = sp_get_posn_arg(PARAMS2 &p1, &p2, format);
	        y_posn = sp_get_posn_arg(PARAMS2 &p1, &p2, (ufix8)(format >> 2));
	        x_scale = sp_get_scale_arg(PARAMS2 &p1, &p2, (ufix8)(format & BIT4));
	        y_scale = sp_get_scale_arg(PARAMS2 &p1, &p2, (ufix8)(format & BIT5));
	        sp_scale_tcb(PARAMS2 &sp_globals.tcb, x_posn, y_posn, x_scale, y_scale); /* Scale for sub-char */
	        sub_char_index = (format & BIT6)?  /* Read sub-char index */
	            (ufix16)NEXT_WORD(p1):
	            (ufix16)NEXT_BYTE(p1);          
	        sub_p1 = sp_get_char_org(PARAMS2 sub_char_index, FALSE); /* Point to start of sub-char */
	        if (sub_p1 == BTSNULL)           /* Character data not available? */
	            {
	            return FALSE;                  /* Abort character output */
	            }
#if INCL_MUTABLE
            sub_p2 = sp_get_char_adj_org(PARAMS2 sub_p1); /* Get sub-char adustment vector origin */
#endif
	        sub_p1 += 2;                  /* Skip over character id */
	        x_orus = NEXT_WORD(sub_p1);   /* Read set_width of sub-character */
#if INCL_MUTABLE
            x_orus += sp_get_adjustment(PARAMS2 &sub_p2);
#endif

			Pssw.x = (fix15)(   
					  (fix31)( 
                              ((fix31)x_orus * (sp_globals.specs.xxmult>>16) + 
                              ( ((fix31)x_orus * (sp_globals.specs.xxmult&0xffffL) )>>16) 
                             ) << sp_globals.pixshift) / sp_globals.metric_resolution);
			Pssw.y = (fix15)(   
					  (fix31)( 
                              ((fix31)x_orus * (sp_globals.specs.yxmult>>16) + 
                              ( ((fix31)x_orus * (sp_globals.specs.yxmult&0xffffL) )>>16) 
                             ) << sp_globals.pixshift) / sp_globals.metric_resolution);
               
	        format = NEXT_BYTE(sub_p1);   /* Read sub-character format */
#if INCL_REMOTE
			if (format & BIT0)
				return(FALSE);	/* compound-compound character, impossible */
#endif
	        if (format & BIT1)                 /* Optional data in header? */
	            {
	            tmpfix15 = (ufix8)NEXT_BYTE(sub_p1); /* Read size of optional data */
	            sub_p1 += tmpfix15;           /* Skip optional data */
	            }
	        sp_plaid_tcb(PARAMS2 &sub_p1, &sub_p2, format);   /* Process sub-character plaid data */
	        sp_read_bbox(PARAMS2 &sub_p1, &sub_p2, &Pmin, &Pmax, FALSE); /* Read bounding box */
	        fn_begin_sub_char(Pssw, Pmin, Pmax);  /* Signal start of sub-character data */
	        sp_proc_outl_data(PARAMS2 sub_p1, sub_p2);       /* Process sub-character data */
	        fn_end_sub_char();                    /* Signal end of sub-character data */
	        }
	    }
	while (!fn_end_char());                       /* Signal end of character; repeat if required */
	}
return TRUE;
}


#if INCL_LCD           /* Dynamic load character data supported? */

FUNCTION LOCAL_PROTO ufix8 FONTFAR *sp_get_char_org(SPGLOBPTR2 ufix16 char_index, btsbool top_level)
/*
 * Called by sp_get_char_id(PARAMS1), sp_get_char_width(PARAMS1), sp_make_char(PARAMS1) and
 * sp_make_comp_char(PARAMS1) to get a pointer to the start of the character data
 * for the specified character index.
 * Version for configuration supporting dynamic character data loading.
 * Calls sp_load_char_data(PARAMS1) to load character data if not already loaded
 * as part of the original font buffer.
 * Returns NULL if character data not available
 */
{
buff_t  STACKFAR *pchar_data;   /* Buffer descriptor requested */
ufix8 FONTFAR  *pointer;      /* Pointer into character directory */
ufix8    format;       /* Character directory format byte */
fix31    char_offset;  /* Offset of char data from start of font file */
fix31    next_char_offset; /* Offset of char data from start of font file */
fix15    no_bytes;     /* Number of bytes required for char data */

if (top_level)                        /* Not element of compound char? */
    {
#if INCL_REMOTE
	if (sp_globals.FourIn1Flags & BIT0)
		{/* remote character here, return pointer to RemoteCharData[] */
    	sp_globals.processor.speedo.cb_offset = 0;                    /* Reset char buffer offset */
		return(&sp_globals.RemoteCharData[0]);
		}
#endif
    if (char_index < sp_globals.processor.speedo.first_char_idx)  /* Before start of character set? */
        return BTSNULL;
    char_index -= sp_globals.processor.speedo.first_char_idx;
    if (char_index >= sp_globals.processor.speedo.no_chars_avail) /* Beyond end of character set? */
        return BTSNULL;
    sp_globals.processor.speedo.cb_offset = 0;                    /* Reset char buffer offset */
    }

pointer = sp_globals.processor.speedo.pchar_dir;
format = NEXT_BYTE(pointer);          /* Read character directory format byte */
pointer += char_index << 1;           /* Point to indexed character entry */
if (format)                           /* 3-byte entries in char directory? */
    {
    pointer += char_index;            /* Adjust for 3-byte entries */
    char_offset = sp_read_long(PARAMS2 pointer); /* Read file offset to char data */
    next_char_offset = sp_read_long(PARAMS2 pointer + 3); /* Read offset to next char */
    }
else
    {
    char_offset = (fix31)((ufix16)NEXT_WORD(pointer)); /* Read file offset to char data */
    next_char_offset = (fix31)((ufix16)NEXT_WORD(pointer)); /* Read offset to next char */
    }

no_bytes = (fix15)(next_char_offset - char_offset);

if (no_bytes <= 0)                    /* Character not in directory? */
    return BTSNULL;

if (next_char_offset <= sp_globals.processor.speedo.font_buff_size)/* Character data already in font buffer? */
    return sp_globals.processor.speedo.pfont->org + char_offset;  /* Return pointer into font buffer */

pchar_data = sp_load_char_data(PARAMS2 char_offset, no_bytes, sp_globals.processor.speedo.cb_offset); /* Request char data load */
if (pchar_data->no_bytes < (ufix32)no_bytes)  /* Correct number of bytes loaded? */
    return BTSNULL;

if (top_level)                        /* Not element of compound char? */
    {
    sp_globals.processor.speedo.cb_offset = no_bytes;
    }

return pchar_data->org;               /* Return pointer into character data buffer */
}
#endif

#if INCL_LCD
#else                  /* Dynamic load character data not supported? */

FUNCTION LOCAL_PROTO ufix8 FONTFAR *sp_get_char_org(SPGLOBPTR2 ufix16 char_index, btsbool top_level)
/*
 * Called by sp_get_char_id(PARAMS1), sp_get_char_width(PARAMS1), sp_make_char(PARAMS1) and
 * sp_make_comp_char(PARAMS1) to get a pointer to the start of the character data
 * for the specified character index.
 * Version for configuration not supporting dynamic character data loading.
 * Returns NULL if character data not available
 */
{
ufix8   FONTFAR *pointer;      /* Pointer into character directory */
ufix8    format;       /* Character directory format byte */
fix31    char_offset;  /* Offset of char data from start of font file */
fix31    next_char_offset; /* Offset of char data from start of font file */
fix15    no_bytes;     /* Number of bytes required for char data */

if (top_level)                        /* Not element of compound char? */
    {
#if INCL_REMOTE
	if (sp_globals.FourIn1Flags & BIT0)
		{/* remote character here, return pointer to RemoteCharData[] */
		return(&sp_globals.RemoteCharData[0]);
		}
#endif
    if (char_index < sp_globals.processor.speedo.first_char_idx)  /* Before start of character set? */
        return BTSNULL;
    char_index -= sp_globals.processor.speedo.first_char_idx;
    if (char_index >= sp_globals.processor.speedo.no_chars_avail) /* Beyond end of character set? */
        return BTSNULL;

    }

pointer = sp_globals.processor.speedo.pchar_dir;
format = NEXT_BYTE(pointer);          /* Read character directory format byte */
pointer += char_index << 1;           /* Point to indexed character entry */
if (format)                           /* 3-byte entries in char directory? */
    {
    pointer += char_index;            /* Adjust for 3-byte entries */
    char_offset = sp_read_long(PARAMS2 pointer); /* Read file offset to char data */
    next_char_offset = sp_read_long(PARAMS2 pointer + 3); /* Read offset to next char */
    }
else
    {
    char_offset = (fix31)((ufix16)NEXT_WORD(pointer)); /* Read file offset to char data */
    next_char_offset = (fix31)((ufix16)NEXT_WORD(pointer)); /* Read offset to next char */
    }

no_bytes = next_char_offset - char_offset;
if (no_bytes <= 0)                    /* Character not in directory? */
    return BTSNULL;

return sp_globals.processor.speedo.pfont->org + char_offset;      /* Return pointer into font buffer */
}
#endif



FUNCTION LOCAL_PROTO fix15 sp_get_posn_arg(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
												 ufix8 FONTFAR *STACKFAR *pp2, ufix8 format)
/*
 * Called by sp_make_comp_char(PARAMS1) to read a position argument from the
 * specified point in the font/char buffer.
 * Updates pointer to byte following position argument.
 * Returns value of position argument in outline resolution units
 */
{
fix15   posn;

switch (format & 0x03)
    {
case 1:
    posn = NEXT_WORD(*pp1);
    break;

case 2:
    posn = (fix15)((fix7)NEXT_BYTE(*pp1));
    break;

default:
    posn = 0;
    break;
    }

#if INCL_MUTABLE
posn += sp_get_adjustment(PARAMS2 pp2);            /* Adjust position for interpolation */
#endif

return posn;
}


FUNCTION LOCAL_PROTO fix15 sp_get_scale_arg(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
												  ufix8 FONTFAR *STACKFAR *pp2, ufix8 format)
/*
 * Called by sp_make_comp_char(PARAMS1) to read a scale argument from the
 * specified point in the font/char buffer.
 * Updates pointer to byte following scale argument.
 * Returns value of scale argument in scale units (normally 1/4096)
 */
{
fix15   scale;

scale = (format)?
    NEXT_WORD(*pp1):
    (fix15)ONE_SCALE;                   /* Read scale argument */

#if INCL_MUTABLE
scale += sp_get_adjustment(PARAMS2 pp2);           /* Adjust for interpolation */
#endif

return(scale);
}

#if INCL_ISW || INCL_SQUEEZING

FUNCTION LOCAL_PROTO void sp_preview_bounding_box(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2, ufix8 format)
{
point_t  Pmin, Pmax;   /* Transformed corners of bounding box */

    sp_globals.processor.speedo.no_X_orus = (format & BIT2)?
        (fix15)NEXT_BYTE(p1):
        0;
    sp_globals.processor.speedo.no_Y_orus = (format & BIT3)?
        (fix15)NEXT_BYTE(p1):
        0;
    sp_read_oru_table(PARAMS2 &p1, &p2, format);

    /* Skip over control zone table */
    sp_skip_control_zone(PARAMS2 &p1, &p2, format);

    /* Skip over interpolation table */
    sp_skip_interpolation_table(PARAMS2 &p1, &p2, format);
    /* get_args has a pathological need for this value to be set */
    sp_globals.processor.speedo.Y_edge_org = sp_globals.processor.speedo.no_X_orus;
    sp_read_bbox(PARAMS2 &p1, &p2, &Pmin, &Pmax, TRUE);        /* Read bounding bo
x */

}
#endif

#if INCL_ISW

FUNCTION LOCAL_PROTO btsbool sp_reset_xmax(SPGLOBPTR2 fix31 xmax)
{
fix15   xmin;          /* Minimum X ORU value in font */
fix15   ymin;          /* Minimum Y ORU value in font */
fix15   ymax;          /* Maximum Y ORU value in font */


sp_globals.processor.speedo.isw_modified_constants = TRUE;
xmin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FXMIN);
ymin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMIN);
ymax = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMAX);

if (!sp_setup_consts(PARAMS2 xmin,xmax,ymin,ymax))
    {
    sp_report_error(PARAMS2 3);           /* Requested specs out of range */
    return FALSE;
    }
sp_globals.processor.speedo.constr.data_valid = FALSE;
/* recompute setwidth */
sp_globals.processor.speedo.Psw.x = (fix15)((fix31)(
     ((fix31)sp_globals.processor.speedo.imported_width * (sp_globals.specs.xxmult>>16) +
     ( ((fix31)sp_globals.processor.speedo.imported_width *
          (sp_globals.specs.xxmult&0xffffL) )>>16)
     ) << sp_globals.pixshift) / sp_globals.metric_resolution);
sp_globals.processor.speedo.Psw.y = (fix15)(   
		  (fix31)( 
                 ((fix31)sp_globals.processor.speedo.imported_width * (sp_globals.specs.yxmult>>16) + 
                ( ((fix31)sp_globals.processor.speedo.imported_width * (sp_globals.specs.yxmult&0xffffL) )>>16) 
                  ) << sp_globals.pixshift) / sp_globals.metric_resolution);
               
return TRUE;
}
#endif

#if INCL_REMOTE

FUNCTION btsbool sp_is_remote_char(SPGLOBPTR2 ufix16 char_index)
/*
 * Outputs specified character using the currently selected font and
 * scaling and output specifications.
 * Reports Error 10 and returns FALSE if no font specifications 
 * previously set.
 * Reports Error 12 and returns FALSE if character data not available.
 */
{
ufix8 FONTFAR  *pointer;      /* Pointer to character data */
fix15    i;
fix15    tmpfix15;
ufix8    format;

#if REENTRANT_ALLOC
plaid_t plaid;
#if INCL_BLACK || INCL_SCREEN || INCL_2D || INCL_WHITE || INCL_QUICK || INCL_GRAY
intercepts_t intercepts;
sp_globals.intercepts = &intercepts;
#endif
#endif

#if REENTRANT_ALLOC
sp_globals.processor.speedo.plaid = &plaid;
#endif

if (!sp_globals.specs_valid)                /* Font specs not defined? */
    {
    return FALSE;                /* Error return */
    }



pointer = sp_get_char_org(PARAMS2 char_index, TRUE); /* Point to start of character data */
SHOW(pointer);
if (pointer == BTSNULL)             /* Character data not available? */
    {
    return FALSE;                /* Error return */
    }
for (i = 0; i < 5; i++)
	sp_globals.RemoteCharData[i] = pointer[i]; /* copy over start of possible remote character data */

pointer += 4;                    /* Skip over character id, set width */

format = NEXT_BYTE(pointer);
if (!((format & (BIT5 | BIT0)) == (BIT5 | BIT0)))
	{/* Older processors won't be checking these bits. They mean remote image data */
	return(FALSE);	/* nope, it's not a remote char */
	}
if (format & BIT1)               /* Optional data in header? */
    {
    tmpfix15 = (ufix8)NEXT_BYTE(pointer); /* Read size of optional data */
    pointer += tmpfix15;         /* Skip optional data */
	sp_globals.RemoteCharData[4] ^= BIT1; /* and shut off optional data bit in buffer */
    }
for (; i < 64/* size of RemoteCharData[] */; i++)
	sp_globals.RemoteCharData[i] = pointer[i-5]; /* copy over rest of remote character data */
return (TRUE); /* yep, it is remote */
}
#endif /* INCL_REMOTE */

#if INCL_MUTABLE

FUNCTION LOCAL_PROTO ufix8 FONTFAR *sp_get_char_adj_org(SPGLOBPTR2 ufix8 FONTFAR *p0)
/*
 * Returns a pointer to the character adjustment data for the character
 * whose origin is at p0.
 * Returns BTSNULL if no character adjustment data is available.
 */
{
ufix8 FONTFAR *p1;              /* Pointer for reading character data */
ufix8 FONTFAR *p2;              /* Pointer to char adjustment data origin */

p1 = p0 + 4;                            /* Point to format byte */
if ((NEXT_BYTE(p1) & BIT1) &&           /* Optional data present? */
    (NEXT_BYTE(p1) >= 2))               /* At least 2 bytes available */
    {
    p2 = p0 + NEXT_WORD(p1);            /* Origin of char adj data */
    }
else                                    /* No char adjustment data? */
    {
    p2 = BTSNULL;                          /* Inhibit interpolation adjustment */
    }
return p2;
}
#endif

#if INCL_MFT	/* Merged Font Technology */

FUNCTION LOCAL_PROTO ufix16 sp_get_alt_width(SPGLOBPTR2 ufix16 char_index, ufix16 resolutionReq, btsbool *success)
{
ufix16 width = 0;	/* assume no width */
ufix16 FONTFAR *widthPtr;
ufix16 ii, numTables, tableSize, tableRes;
ufix16 pub_hdr_size, pvt_hdr_size;
ufix8 FONTFAR *p1;              /* Pointer for reading data */
ufix8 FONTFAR *p2;              /* Pointer for reading data */
btsbool done, rangeErr = FALSE;
ufix32 fileOffset, tablesLength;

	*success = FALSE;	/* assume failure */
    if (char_index < sp_globals.processor.speedo.first_char_idx)  /* Before start of character set? */
        rangeErr = TRUE;
    char_index -= sp_globals.processor.speedo.first_char_idx;
    if (char_index >= sp_globals.processor.speedo.no_chars_avail) /* Beyond end of character set? */
        rangeErr = TRUE;
	if (!rangeErr) /* if there is, it will be reported by calling unit */
		{ /* Is there an unexplained gap from end of private header to character directory? */
		pub_hdr_size = sp_globals.processor.speedo.hdr2_org - sp_globals.processor.speedo.font_org;
		pvt_hdr_size = 
    				(pub_hdr_size >= (FH_PVHSZ + 2))?
    				sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_PVHSZ):
    				FH_NBYTE + 3;
		if ((pub_hdr_size + pvt_hdr_size) < (sp_globals.processor.speedo.pchar_dir - sp_globals.processor.speedo.font_org - 1))
			{/* Yes, alternate tables stuffed between header and character directory: */
			p1 = sp_globals.processor.speedo.font_org + pub_hdr_size + pvt_hdr_size;
			/* first find offset of 'altw' table(s) chain */
			done = FALSE;
			numTables = (ufix16)sp_read_word_u(PARAMS2 p1);
			p1 += sizeof(ufix16);
			for (ii = 0; !done && ii < numTables; ii++)
				{/* linear search */
				done = (*(ufix32 *)p1 == 0x616c7477);	/* 'altw' */
				if (done)
					{
					/* read a 4-byte unencrypted value: */
					fileOffset = ((ufix32)sp_read_word_u(PARAMS2 p1 + sizeof(ufix32))) << 16;
					fileOffset += (ufix32)sp_read_word_u(PARAMS2 p1 + sizeof(ufix32) + sizeof(ufix16));
					/* read another 4-byte unencrypted value: */
					tablesLength = ((ufix32)sp_read_word_u(PARAMS2 p1 + 2*sizeof(ufix32))) << 16;
					tablesLength += (ufix32)sp_read_word_u(PARAMS2 p1 + 2*sizeof(ufix32) + sizeof(ufix16));
					}
				p1 += 3*sizeof(ufix32);	/* jump to byte after this directory entry */
				}
			if (!done)
				return(width);	/* we failed, get out pronto! */
			p1 = sp_globals.processor.speedo.font_org + fileOffset;		/* start of 'altw' chain data */
			p2 = p1 + tablesLength;										/* set pointer to chains limit */
			p1 += sizeof(ufix16);										/* skip over sub-tables count */
			done = FALSE;
			while ( !done && (p1 < p2) )
				{/* walk through alternate tables chain until found or past end of chain limit */
				p1 += 4;		/* skip '????' sub-table tag identifier */
				tableSize = (ufix16)sp_read_word_u(PARAMS2 p1);
				tableRes = (ufix16)sp_read_word_u(PARAMS2 p1+sizeof(ufix16));
				if (tableRes == resolutionReq)
					{/* found the requested resolution: */
					width = (ufix16)sp_read_word_u(PARAMS2 p1 + 2*sizeof(ufix16) + 2*char_index);
					done = TRUE;
					}
				else	/* walk to next 'altw' table: */
					p1 += tableSize;
				}
			*success = done;	/* if we are already done, then we are successful */
			}
		}
	return(width);
}
#endif /* INCL_MFT */
#endif /* if PROC_SPEEDO */
