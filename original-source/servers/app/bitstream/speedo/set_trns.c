/* #define D_AutoIzones */


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
*     $Header: //bliss/user/archive/rcs/speedo/set_trns.c,v 33.0 97/03/17 17:47:47 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	set_trns.c,v $
*       Revision 33.0  97/03/17  17:47:47  shawn
*       TDIS Version 6.00
*       
*       Revision 32.6  97/01/14  17:46:06  shawn
*       Apply casts to avoid compiler warnings.
*       
*       Revision 32.5  96/12/16  09:40:46  shawn
*       Changed sp_calculate_x_scale() & sp_calculate_y_scale() prototypes to Global.
*       
*       Revision 32.4  96/12/13  17:08:36  shawn
*       Added parameter 'fix31 ppo' to function declaration of sp_calculate_x_pix().
*       
*       Revision 32.3  96/08/02  11:27:29  shawn
*       Changed zone_pix assignment in sp_calculate_x_pix() from 0x7ffff to 0x7fff.
*       
*       Revision 32.2  96/07/02  12:03:32  shawn
*       Changed boolean to btsbool.
*       
*       Revision 32.1  95/11/17  10:12:14  shawn
*       Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
*       
*       Revision 32.0  95/02/15  16:39:14  shawn
*       Release
*       
*       Revision 31.3  95/01/24  16:48:10  shawn
*       Corrected 'fix151em_bot_pix' to 'fix15 em_bot_pix' in sp_calculate_y_pix()
*       
*       Revision 31.2  95/01/12  11:31:29  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:42:11  roberte
*       Release
*       
*       Revision 30.2  94/12/21  15:02:14  shawn
*        Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
*       
*       Revision 30.1  94/10/24  10:23:25  roberte
*       Added conditional #if PROC_SPEEDO so processor may be excluded in 4-in-1 builds.
*       
*       Revision 30.0  94/05/04  09:41:06  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:40:44  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:46:14  roberte
*       Release
*       
*       Revision 28.52  94/04/01  15:58:33  roberte
*       Moved some static function prototypes from speedo.h to here.
*       
*       Revision 28.51  94/03/10  16:51:43  roberte
*       Changed a comment.
*       
*       Revision 28.50  94/03/09  15:21:53  roberte
*       Added initialization code to fill in mult[] and offset[] arrays
*       of speedo_globals in support of AUTOIZONES for compressed fonts.
*       New function, sp_SetInterpolation().
*       Removed dead INCL_PACKED_IZONES after testing.
*       
*       Revision 28.49  94/02/10  13:25:13  roberte
*        Upgrade for MUTABLE fonts.
*       
*       Revision 28.48  94/01/05  12:26:47  roberte
*       Changes for upgraded Speedo compiler.
*       Added "packed" interpolation zones and relative ORU table format.
*       Rewrite of sp_read_oru_table(), now requires the "format" parameter.
*       
*       Revision 28.47  93/08/31  09:39:35  roberte
*       Release
*       
*       Revision 28.37  93/03/15  13:01:37  roberte
*       Release
*       
*       Revision 28.10  93/01/12  12:21:44  roberte
*       Renamed sp_plaid.pix to sp_plaid.spix to avoid ambiguities with type1.
*       
*       Revision 28.9  93/01/08  14:05:03  roberte
*       Changed references to sp_globals. for pspecs, orus_per_em, curves_out, multrnd, pixfix and mpshift.
*       
*       Revision 28.8  93/01/04  16:43:18  roberte
*       Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
*       
*       Revision 28.7  92/12/30  17:51:27  roberte
*       Functions no longer renamed in spdo_prv.h now declared with "sp_"
*       Use PARAMS1&2 macros throughout.
*       
*       Revision 28.6  92/11/24  10:58:52  laurar
*       include fino.h
*       
*       Revision 28.5  92/11/19  15:18:33  roberte
*       Release
*       
*       Revision 28.1  92/06/25  13:42:28  leeann
*       Release
*       
*       Revision 27.1  92/03/23  14:02:07  leeann
*       Release
*       
*       Revision 26.1  92/01/30  17:01:45  leeann
*       Release
*       
*       Revision 25.1  91/07/10  11:07:25  leeann
*       Release
*       
*       Revision 24.1  91/07/10  10:40:59  leeann
*       Release
*       
*       Revision 23.1  91/07/09  18:01:58  leeann
*       Release
*       
*       Revision 22.1  91/01/23  17:21:25  leeann
*       Release
*       
*       Revision 21.1  90/11/20  14:40:51  leeann
*       Release
*       
*       Revision 20.1  90/11/12  09:36:29  leeann
*       Release
*       
*       Revision 19.2  90/11/11  09:41:32  leeann
*       surround compute_isw_scale with ifdef INCL_ISW
*       
*       Revision 19.1  90/11/08  10:25:55  leeann
*       Release
*       
*       Revision 18.2  90/11/06  19:03:19  leeann
*       fix bugs when combining imported setwidth and squeezing
*       
*       Revision 18.1  90/09/24  10:17:16  mark
*       Release
*       
*       Revision 17.2  90/09/14  15:03:48  leeann
*       if the setwidth_orus are zero, just set the isw_scale to one.
*       
*       Revision 17.1  90/09/13  16:02:11  mark
*       Release name rel0913
*       
*       Revision 16.2  90/09/13  15:18:20  leeann
*       allow imported setwidth of zero
*       
*       Revision 16.1  90/09/11  13:22:58  mark
*       Release
*       
*       Revision 15.2  90/09/05  11:27:44  leeann
*       apply scale factor to offset when doing imported setwidths
*       
*       Revision 15.1  90/08/29  10:05:59  mark
*       Release name rel0829
*       
*       Revision 14.1  90/07/13  10:42:57  mark
*       Release name rel071390
*       
*       Revision 13.2  90/07/12  17:16:39  judy
*       fixed 2 bugs: initialized spacing error variable
*       and corrected negative error case of going from
*       zone 1 to 0.
*       
*       Revision 13.1  90/07/02  10:41:59  mark
*       Release name REL2070290
*       
*       Revision 12.3  90/06/26  09:00:07  leeann
*       correct xof
*       compute correct xoffset when SQUEEZING in the x direction
*       
*       Revision 12.2  90/06/20  15:59:31  leeann
*       Fix squeezing bugs
*       
*       Revision 12.1  90/04/23  12:14:26  mark
*       Release name REL20
*       
*       Revision 11.1  90/04/23  10:14:41  mark
*       Release name REV2
*       
*       Revision 10.10  90/04/23  09:43:16  mark
*       add GDECL statements to new squeezing functions
*       
*       
*       Revision 10.9  90/04/19  10:11:52  judy
*       add inter-character spacing fix. Store
*       temp. variable of non-interpolated
*       position of 1st zone in setup_pix_table.
*       
*       Revision 10.8  90/04/17  12:10:18  leeann
*       add code to support imported setwidths
*       
*       Revision 10.7  90/04/11  13:04:41  leeann
*       fix x and y pixel value calculations, change
*       squeeze compilation flag to be INCL_SQUEEZING
*       
*       Revision 10.6  90/04/10  14:18:55  leeann
*       fixup y_scale calculations
*       
*       Revision 10.5  90/04/05  15:15:40  leeann
*       added code to calculate SQUEEZED pixel positions
*       
*       Revision 10.4  90/03/29  16:49:31  leeann
*       make read_oru_table visible outside this source
*       (took of "static" declaration)
*       
*       Revision 10.3  90/03/28  13:51:13  leeann
*       new function skip_orus added
*       
*       Revision 10.2  90/03/27  14:53:27  leeann
*       Include new functions skip_control_zone, skip_interpolation_zone
*       
*       Revision 10.1  89/07/28  18:13:16  mark
*       Release name PRODUCT
*       
*       Revision 9.1  89/07/27  10:26:45  mark
*       Release name PRODUCT
*       
*       Revision 8.1  89/07/13  18:22:34  mark
*       Release name Product
*       
*       Revision 7.1  89/07/11  09:05:15  mark
*       Release name PRODUCT
*       
*       Revision 6.2  89/07/09  14:49:38  mark
*       change tcb manipulating functions to take GLOBALFAR pointers
*       
*       Revision 6.1  89/06/19  08:38:03  mark
*       Release name prod
*       
*       Revision 5.6  89/06/05  17:22:13  mark
*       reference plaid data (orus, pix, mult, offset) via new symbol
*       sp_globals so that they may conditionally be allocated off the
*       stack for reentrant mode
*       
*       Revision 5.5  89/05/24  18:25:37  john
*       Updated c_pix[] to contain min value when constraint
*       is inactive. Min values is now used whether or not
*       a constraint is active.
*       
*       Revision 5.4  89/05/24  10:34:15  john
*       Corrected 16-bit overflow bug in interpolation
*       coefficient computation.
*       
*       Revision 5.3  89/05/16  15:45:51  john
*       Interpolation coefficient accuracy improved
*       
*       Revision 5.2  89/05/03  17:00:41  mark
*       correct no rules version of sp_plaid_tcb to
*       remove accounting for obsolete length
*       
*       Revision 5.1  89/05/01  17:57:26  mark
*       Release name Beta
*       
*       Revision 4.2  89/05/01  17:16:12  mark
*       declare tmpufix16 if INCL_EXT is true
*       
*       Revision 4.1  89/04/27  12:20:26  mark
*       Release name Beta
*       
*       Revision 3.2  89/04/26  17:00:29  mark
*       use static void, not void static
*       
*       Revision 3.1  89/04/25  08:33:38  mark
*       Release name beta
*       
*       Revision 2.3  89/04/12  12:16:17  mark
*       added stuff for far stack and font
*       
*       Revision 2.2  89/04/10  17:10:00  mark
*       Modified pointer declarations that are used to refer
*       to font data to use FONTFAR symbol, which will be used
*       for Intel SS != DS memory models
*       Modified plaid_tcb, read_oru_table, setup_pix_table and
*       setup_int_table to receive a pointer and return the resulting 
*       pointer, rather than receiving a pointer to a pointer
*       
*       Revision 2.1  89/04/04  13:39:34  mark
*       Release name EVAL
*       
*       Revision 1.6  89/04/04  13:28:33  mark
*       Update copyright text
*       
*       Revision 1.5  89/03/31  14:45:30  mark
*       change speedo.h to spdo_prv.h
*       change comments from fontware to speedo
*       
*       Revision 1.4  89/03/31  12:25:02  john
*       modified to use new NEXT_WORD macro.
*       
*       Revision 1.3  89/03/29  16:13:27  mark
*       changes for slot independence and dynamic/reentrant
*       data allocation
*       
*       Revision 1.2  89/03/21  13:34:41  mark
*       change name from oemfw.h to speedo.h
*       
*       Revision 1.1  89/03/15  12:36:14  mark
*       Initial revision
*                                                                                 *
*                                                                                    *
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/set_trns.c,v 33.0 97/03/17 17:47:47 shawn Release $";
#endif




/*************************** S E T _ T R N S . C *****************************
 *                                                                           *
 * This module is called from do_char.c to set up the intelligent            *
 * transformation for one character (or sub-character of a composite         *
 * character.
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 16 Dec 88  jsc  Created                                               *
 *                                                                           *
 *  2) 20 Jan 89  jsc  Option to read interpolation length removed. Bit 6 of *
 *                     interpolation zone format byte is now ignored.        *
 *                                                                           *
 *  3) 23 Jan 89  jsc  Constraint limit test changed to round accurately     *
 *                                                                           *
 *  4) 28 Feb 89  jsc  Plaid data monitoring functions added.                *
 *                                                                           *
 ****************************************************************************/


#include "spdo_prv.h"               /* General definitions for Speedo   */
#include "fino.h"

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#if PROC_SPEEDO
/***** LOCAL MACROS     *****/

#define SQUEEZE_X_ORU(A,B,C) ((((fix31)A * (fix31)B) + C) >> 16)
#define ABS(A) ((A < 0)? -A:A) /* absolute value */
#define IMPORT_FACTOR    \
	shift = 16;\
	while (*x_factor > (0x7fffffffL / (isw_scale >> (16 - shift))))\
		shift--;\
    	*x_factor = (*x_factor * (isw_scale>>(16-shift))) >> shift;

/***** GLOBAL VARIABLES *****/

/*****  GLOBAL FUNCTIONS *****/

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/
#if INCL_RULES
#if INCL_AUTOIZONES
LOCAL_PROTO void sp_SetInterpolation(SPGLOBPTR2 fix15 dimension);
#endif
#endif

LOCAL_PROTO void sp_constr_update(SPGLOBPTR1);

#if INCL_SQUEEZING || INCL_ISW
LOCAL_PROTO void sp_calculate_x_pix(SPGLOBPTR2 ufix8 start_edge,
						   ufix8 end_edge,
						   ufix16 constr_nr,
						   fix31 x_scale,
						   fix31 x_offset,
						   fix31 ppo,
						   fix15 setwidth_pix);
#endif

#if INCL_SQUEEZING
LOCAL_PROTO void sp_calculate_y_pix(SPGLOBPTR2 ufix8 start_edge,
						   ufix8 end_edge,
						   ufix16 constr_nr,
						   fix31 top_scale,
						   fix31 bottom_scale,
						   fix31 ppo,
						   fix15 emtop_pix,
						   fix15 embot_pix);

GLOBAL_PROTO btsbool sp_calculate_x_scale(SPGLOBPTR2 fix31 *x_factor, fix31 *x_offset, fix15 no_X_ctrl_zones);
GLOBAL_PROTO btsbool sp_calculate_y_scale(SPGLOBPTR2 fix31 *top_scale, fix31 *bottom_scale, fix15 first_Y_zone, fix15 no_Y_ctrl_zones);
#endif

#if INCL_RULES
LOCAL_PROTO void sp_setup_pix_table(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
											   ufix8 FONTFAR *STACKFAR *pp2,
											   btsbool short_form,
											   fix15 no_X_ctrl_zones, fix15 no_Y_ctrl_zones);
LOCAL_PROTO void sp_setup_int_table(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
											   ufix8 FONTFAR *STACKFAR *pp2,
											   fix15 no_X_int_zones, fix15 no_Y_int_zones);
#endif



FUNCTION void sp_init_tcb(SPGLOBPTR1)
/*
 * Called by sp_make_char(PARAMS1) and sp_make_comp_char(PARAMS1) to initialize the current
 * transformation control block to the top level transformation.
 */
{
sp_globals.tcb = sp_globals.tcb0;
}


FUNCTION void sp_scale_tcb(SPGLOBPTR2 tcb_t GLOBALFAR *ptcb, fix15 x_pos, fix15 y_pos, fix15 x_scale, fix15 y_scale)
/*
 * Called by sp_make_comp_char(PARAMS1) to apply position and scale for each of the
 * components of a compound character.
 */
{     
fix15 xx_mult = ptcb->xxmult;
fix15 xy_mult = ptcb->xymult;
fix31 x_offset = ptcb->xoffset;
fix15 yx_mult = ptcb->yxmult;
fix15 yy_mult = ptcb->yymult;
fix31 y_offset = ptcb->yoffset;

ptcb->xxmult = TRANS(xx_mult, x_scale, (fix31)SCALE_RND, SCALE_SHIFT);
ptcb->xymult = TRANS(xy_mult, y_scale, (fix31)SCALE_RND, SCALE_SHIFT);
ptcb->xoffset = MULT16(xx_mult, x_pos) + MULT16(xy_mult, y_pos) + x_offset;
ptcb->yxmult = TRANS(yx_mult, x_scale, (fix31)SCALE_RND, SCALE_SHIFT);
ptcb->yymult = TRANS(yy_mult, y_scale, (fix31)SCALE_RND, SCALE_SHIFT);
ptcb->yoffset = MULT16(yx_mult, x_pos) + MULT16(yy_mult, y_pos) + y_offset;

sp_type_tcb(PARAMS2 ptcb); /* Reclassify transformation types */
}


FUNCTION void sp_skip_interpolation_table(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
													 ufix8 FONTFAR *STACKFAR *pp2,
													 ufix8 format)
{
fix15 i,n;
ufix8 intsize[9];
ufix8   tmpufix8;

intsize[0] = 1;
intsize[1] = 2;
intsize[2] = 3;
intsize[3] = 1;
intsize[4] = 2;
intsize[5] = 1;
intsize[6] = 2;
intsize[7] = 0;
intsize[8] = 0;

n = ((format & BIT6)? (fix15)NEXT_BYTE(*pp1): 0) +
    ((format & BIT7)? (fix15)NEXT_BYTE(*pp1): 0);
for (i = 0; i < n; i++)          /* For each entry in int table ... */
    {
    format = NEXT_BYTE(*pp1); /* Read format byte */
	if ((format & (BIT6 | BIT7)) == (BIT6 | BIT7)) /* Packed start/end point spec? */
		{
		;	/* nothing to do */
		}
	else if (format & BIT7)           /* Short Start/End point spec? */
        {
        *pp1 += 1;                      /* Skip Start/End point byte */
        }
    else
        {
        tmpufix8 = format & 0x7;
#if INCL_MUTABLE
        if (tmpufix8 != 0)              /* Not type 0 from field? */
            sp_get_adjustment(PARAMS2 pp2);        /* Skip adjustment vector */
#endif
        *pp1 += intsize[tmpufix8];      /* Skip Start point spec */
        tmpufix8 = (format >> 3) & 0x7;
#if INCL_MUTABLE
        if (tmpufix8 != 0)              /* Not type 0 to field? */
            sp_get_adjustment(PARAMS2 pp2);        /* Skip adjustment vector */
#endif
        *pp1 += intsize[(format >> 3) & 0x7]; /* Skip End point spec */
        }
    }
}

FUNCTION void sp_skip_control_zone(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
											  ufix8 FONTFAR *STACKFAR *pp2,
											  ufix8 format)
{
fix15    i,n;
ufix16   tmpufix16;
fix15    constr;

n = sp_globals.processor.speedo.no_X_orus + sp_globals.processor.speedo.no_Y_orus - 2;
for (i = 0; i < n; i++)          /* For each entry in control table ... */
    {
    if (format & BIT4)
        *pp1 += 1;               /* Skip short form From/To fields */
    else
        *pp1 += 2;            /* Skip FROM and TO fields */
    /* skip constraints field */
    constr = NEXT_BYTES (*pp1, tmpufix16);

    }
}


#if INCL_RULES
#else

FUNCTION void sp_plaid_tcb(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
									  ufix8 FONTFAR *STACKFAR *pp2,
									  ufix8 format)
/* 
 * Called by sp_make_simp_char(PARAMS1) and sp_make_comp_char(PARAMS1) to set up the controlled
 * coordinate table and skip all other intelligent scaling rules embedded
 * in the character data.
 * Updates pointers to first byte after plaid data.
 * This is used only if intelligent scaling is not supported in the
 * configuration definitions.
 */
{
fix15  i, n;



sp_globals.processor.speedo.no_X_orus = (format & BIT2)?
    (fix15)NEXT_BYTE(*pp1):
    0;
sp_globals.processor.speedo.no_Y_orus = (format & BIT3)?
    (fix15)NEXT_BYTE(*pp1):
    0;
sp_read_oru_table(PARAMS2 pp1, pp2, format);        /* Updates no_X/Y/orus */
sp_globals.processor.speedo.Y_edge_org = sp_globals.processor.speedo.no_X_orus;

/* Skip over control zone table */
sp_skip_control_zone(PARAMS2 pp1, pp2, format);

/* Skip over interpolation table */
sp_skip_interpolation_table(PARAMS2 pp1, pp2, format);
}
#endif


#if INCL_RULES

FUNCTION void sp_plaid_tcb(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
									  ufix8 FONTFAR *STACKFAR *pp2,
									  ufix8 format)
/* 
 * Called by sp_make_simp_char(PARAMS1) and sp_make_comp_char(PARAMS1) to set up the controlled
 * coordinate table and process all intelligent scaling rules embedded
 * in the character data.
 * Updates pointers to first byte after plaid data.
 * This is used only if intelligent scaling is enabled in the
 * configuration definitions.
 */
{
fix15 no_X_ctrl_zones;
fix15 no_Y_ctrl_zones;
fix15 no_X_int_zones;
fix15 no_Y_int_zones;

#if INCL_PLAID_OUT         /* Plaid data monitoring included? */
begin_plaid_data();
#endif

sp_constr_update(PARAMS1);           /* Update constraint table if required */

sp_globals.processor.speedo.no_X_orus = (format & BIT2)?  
    (fix15)NEXT_BYTE(*pp1):
    0;
sp_globals.processor.speedo.no_Y_orus = (format & BIT3)?
    (fix15)NEXT_BYTE(*pp1):
    0;
sp_read_oru_table(PARAMS2 pp1, pp2, format);  /* Updates no_X/Y/orus to include zero values */
sp_globals.processor.speedo.Y_edge_org = (ufix8)sp_globals.processor.speedo.no_X_orus;                                                  
if (sp_globals.processor.speedo.no_X_orus > 1)         /* 2 or more controlled X coordinates? */
    sp_globals.tcb.xmode = sp_globals.tcb.xtype; /* Enable intelligent scaling in X */

if (sp_globals.processor.speedo.no_Y_orus > 1)         /* 2 or more controlled Y coordinates? */
    sp_globals.tcb.ymode = sp_globals.tcb.ytype; /* Enable intelligent scaling in Y */

no_X_ctrl_zones = sp_globals.processor.speedo.no_X_orus - 1;
no_Y_ctrl_zones = sp_globals.processor.speedo.no_Y_orus - 1;
sp_setup_pix_table(PARAMS2 pp1, pp2, (btsbool)(format & BIT4), 
    no_X_ctrl_zones, no_Y_ctrl_zones);

no_X_int_zones = (format & BIT6)?
    (fix15)NEXT_BYTE(*pp1):
    0;
no_Y_int_zones = (format & BIT7)?
    (fix15)NEXT_BYTE(*pp1):
    0;
sp_globals.processor.speedo.Y_int_org = (ufix8)no_X_int_zones;
#if INCL_AUTOIZONES
if (sp_globals.FourIn1Flags & BIT1)
	sp_globals.FourIn1Flags ^= BIT1;	/* assume no auto-izones */
if (!no_X_int_zones && !no_Y_int_zones)
	{
	sp_globals.FourIn1Flags |= BIT1;	/* auto-izones */
	sp_globals.processor.speedo.Y_int_org = sp_globals.processor.speedo.no_X_orus + 1;;
	sp_SetInterpolation(PARAMS2 'X');
	sp_SetInterpolation(PARAMS2 'Y');
#if DEBUG
{
int ii,jj;
printf("\nX INT TABLE\n");
for (ii = 0; ii <= sp_globals.processor.speedo.no_X_orus; ii++)
    {
    printf("%2d %7.4f %7.4f\n",
        ii, 
        (real)sp_plaid.mult[ii] / (real)(1 << sp_globals.multshift), 
        (real)sp_plaid.offset[ii] / (real)(1 << sp_globals.multshift));
    }
printf("\nY INT TABLE\n");
for (ii = 0; ii <= sp_globals.processor.speedo.no_Y_orus; ii++)
    {
    jj = ii + sp_globals.processor.speedo.Y_int_org;
    printf("%2d %7.4f %7.4f\n",
        ii, 
        (real)sp_plaid.mult[jj] / (real)(1 << sp_globals.multshift), 
        (real)sp_plaid.offset[jj] / (real)(1 << sp_globals.multshift));
    }
}
#endif /* DEBUG */
	}
else
#endif
sp_setup_int_table(PARAMS2 pp1, pp2, no_X_int_zones, no_Y_int_zones);

#if INCL_PLAID_OUT         /* Plaid data monitoring included? */
end_plaid_data();
#endif

}
#endif


#if INCL_RULES

FUNCTION LOCAL_PROTO void sp_constr_update(SPGLOBPTR1)
/*
 * Called by sp_plaid_tcb(PARAMS1) to update the constraint table for the current
 * transformation.
 * This is always carried out whenever a character is generated following
 * a change of font or scale factor or after initialization.     
 */
{
fix31    ppo;
fix15    xppo;
fix15    yppo;
ufix8 FONTFAR  *p1;             /* Pointer to constraint data */
ufix8 FONTFAR  *p2;             /* Pointer to constr adjustment vector */
fix15    no_X_constr;
fix15    no_Y_constr;
fix15    i, j, k, l, n;
fix15    ppm;
ufix8    format;
ufix8    format1;
fix15    limit;
ufix16   constr_org;
fix15    constr_nr;
fix15    size;
fix31    off;
ufix16   min;     
fix15    orus;
ufix16   pix; 
ufix16   tmpufix16;  /* in extended mode, macro uses secnd term */

if (sp_globals.processor.speedo.constr.data_valid &&         /* Constr table already done and ... */
    (sp_globals.tcb.xppo == sp_globals.processor.speedo.constr.xppo) && /* ... X pix per oru unchanged and ... */
    (sp_globals.tcb.yppo == sp_globals.processor.speedo.constr.yppo))   /* ... Y pix per oru unchanged? */
    {
    return;                      /* No need to update constraint table */
    }

sp_globals.processor.speedo.constr.xppo = xppo = sp_globals.tcb.xppo;   /* Update X pixels per oru indicator */
sp_globals.processor.speedo.constr.yppo = yppo = sp_globals.tcb.yppo;   /* Update Y pixels per oru indicator */
sp_globals.processor.speedo.constr.data_valid = TRUE;        /* Mark constraint table valid */

p1 = sp_globals.processor.speedo.constr.org;            /* Point to first byte of constraint data */
no_X_constr = NEXT_BYTES(p1, tmpufix16); /* Read nmbr of X constraints */
no_Y_constr = NEXT_BYTES(p1, tmpufix16); /* Read nmbr of Y constraints */

p2 = sp_globals.processor.speedo.constr.adj_org;         /* Point to first constraint adjustment vector */

i = 0;
constr_org = 0;
n = no_X_constr;
ppo = xppo;
for (j = 0; ; j++)
    {
    sp_globals.processor.speedo.c_act[i] = FALSE;            /* Flag constraint 0 not active */
    sp_globals.processor.speedo.c_pix[i++] = 0;              /* Constraint 0 implies no minimum */
    sp_globals.processor.speedo.c_act[i] = FALSE;            /* Flag constraint 1 not active */
    sp_globals.processor.speedo.c_pix[i++] = sp_globals.onepix; /* Constraint 1 implies min 1 pixel*/

    ppm = (fix15)((ppo * (fix31)sp_globals.orus_per_em) >> sp_globals.multshift);

    for (k = 0; k < n; k++)
        {
        format = NEXT_BYTE(p1);        /* Read format byte */
        limit = (fix15)NEXT_BYTE(p1);  /* Read limit field */
#if INCL_MUTABLE
        limit += sp_get_adjustment(PARAMS2 &p2);   /* Execute adjustment vector */
#endif
        sp_globals.processor.speedo.c_act[i] = 
            ((ppm < limit) || (limit == 255)) &&
            sp_globals.processor.speedo.constr.active;
        if (sp_globals.processor.speedo.c_act[i])            /* Constraint active? */
            {
            if ((format & BIT1) &&          /* Constraint specified and ... */
                (constr_nr = constr_org +
                    ((format & BIT0)?       /* Read unsigned constraint value */
                    NEXT_WORD(p1): 
                    (fix15)NEXT_BYTE(p1)),
                 sp_globals.processor.speedo.c_act[constr_nr])) /* ... and specified constraint active? */ 
                {
                pix = sp_globals.processor.speedo.c_pix[constr_nr]; /* Use constrained pixel value */
                format1 = format;
                for (l = 2; l > 0; l--)     /* Skip 2 arguments */
                    {
                    format1 >>= 2;
                    if (size = format1 & 0x03)
                        p1 += size - 1;
                    }
#if INCL_MUTABLE
                sp_get_adjustment(PARAMS2 &p2);    /* Skip orus adjustment vector */
#endif
                }
            else                 /* Constraint absent or inactive? */
                {
                orus = (format & BIT2)? /* Read unsigned oru value */
                    NEXT_WORD(p1):
                    (fix15)NEXT_BYTE(p1);
#if INCL_MUTABLE
                orus += sp_get_adjustment(PARAMS2 &p2); /* Execute adjustment vector */
#endif

                if (format & BIT5) /* Specified offset value? */
                    {
                    off = (fix31)((format & BIT4)? /* Read offset value */
                        NEXT_WORD(p1):
                        (fix7)NEXT_BYTE(p1));
                    off = (off << (sp_globals.multshift - 6)) + sp_globals.multrnd;
                    }
                else             /* Unspecified (zero) offset value? */
                    {
                    off = sp_globals.multrnd;
                    }

                pix = (ufix16)(((fix31)orus * ppo + off) >> sp_globals.mpshift) & sp_globals.pixfix;
                }
            }
        else                     /* Constraint inactive? */
            {
            format1 = format;
            for (l = 3; l > 0; l--) /* Skip over 3 arguments */
                {
                if (size = format1 & 0x03)
                    p1 += size - 1;
                format1 >>= 2;
                }
            pix = 0;
#if INCL_MUTABLE
            sp_get_adjustment(PARAMS2 &p2);        /* Skip orus adjustment vector */
#endif
            }

        if (format & 0xc0) /* Specified minimum value? */
            {
            min = (format & BIT7)? /* Read unsigned minimum value */
                (ufix16)NEXT_BYTE(p1) << sp_globals.pixshift:
                sp_globals.onepix;
            }
        else             /* Unspecified (zero) minimum value? */
            {
            min = 0;
            }

        sp_globals.processor.speedo.c_pix[i] = (pix < min)? min: pix;
        i++;
        }
    if (j) break;                /* Finished if second time around loop */
    constr_org = sp_globals.processor.speedo.Y_constr_org = i;
    n = no_Y_constr;
    ppo = yppo;
    }

#if DEBUG
printf("\nCONSTRAINT TABLE\n");
n = no_X_constr + 2;
for (i = 0; i < n; i++)
    {
    printf("%3d   ", i);
    if (sp_globals.processor.speedo.c_act[i])
        {
        printf("T ");
        }
    else
        {
        printf("F ");
        }
    printf("%5.1f\n", ((real)sp_globals.processor.speedo.c_pix[i] / (real)sp_globals.onepix));
    }
printf("--------------\n");
n = no_Y_constr + 2;
for (i = 0; i < n; i++)
    {
    j = i + sp_globals.processor.speedo.Y_constr_org;
    printf("%3d   ", i);
    if (sp_globals.processor.speedo.c_act[j])
        {
        printf("T ");
        }
    else
        {
        printf("F ");
        }
    printf("%5.1f\n", ((real)sp_globals.processor.speedo.c_pix[j] / (real)sp_globals.onepix));
    }
#endif

}
#endif


FUNCTION void sp_read_oru_table(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
										   ufix8 FONTFAR *STACKFAR *pp2,
										   ufix8 format)
/*
 * Called by sp_plaid_tcb(PARAMS1) to read the controlled coordinate table from the
 * character data in the font. 
 * Updates the pointers to the byte following the controlled coordinate
 * data.
 */
{
fix15    i, j, k, n;
btsbool  zero_not_in;
btsbool  zero_added;
fix15    oru;
#if INCL_RULES
fix15    pos;
#endif
ufix8	oruFormat;
fix15	count;


i = 0;
n = sp_globals.processor.speedo.no_X_orus;
#if INCL_RULES
pos = (fix15)sp_globals.tcb.xpos;
#endif
oruFormat = 0xff;	/* assume all 2-byte words */
oru = 0;
count = 0;
for (j = 0; ; j++)
    {
    zero_not_in = TRUE;
    zero_added = FALSE;
    for (k = 0; k < n; k++)
        {
		if ( (format & BIT5) && ((count % 8) == 0) )
			oruFormat = NEXT_BYTE(*pp1);
        oru = 
				(oruFormat & BIT0) ?
				NEXT_WORD(*pp1) :
				oru + (fix15)NEXT_BYTE(*pp1);
#if INCL_MUTABLE
        oru = oru + sp_get_adjustment(PARAMS2 pp2); /* Execute adjustment vector */
#endif
		count++;
		if (format & BIT5)
			oruFormat >>= 1;
        if (zero_not_in && (oru >= 0)) /* First positive oru value? */
            {
#if INCL_RULES
            sp_plaid.spix[i] = pos;        /* Insert position in pix array */
#endif
            if (oru != 0)        /* Zero oru value omitted? */
                {
#if INCL_MUTABLE
                sp_plaid.raw_orus[i] = 0; /* Insert zero value in raw oru array */
#endif
                sp_plaid.orus[i++] = 0;   /* Insert zero value in oru array */
                zero_added = TRUE; /* Remember to increment size of array */
                }
            zero_not_in = FALSE; /* Inhibit further testing for zero ins */
            }
#if INCL_MUTABLE
        sp_plaid.raw_orus[i] = oru; /* Add raw oru value to raw array */
#endif
        sp_plaid.orus[i++] = oru;         /* Add specified oru value to array */
        }
    if (zero_not_in)             /* All specified oru values negative? */
        {
#if INCL_RULES
        sp_plaid.spix[i] = pos;            /* Insert position in pix array */
#endif
#if INCL_MUTABLE
        sp_plaid.raw_orus[i] = 0;      /* Add zero to raw oru array */
#endif
        sp_plaid.orus[i++] = 0;           /* Add zero oru value */
        zero_added = TRUE;       /* Remember to increment size of array */
        }
    if (j)                       /* Both X and Y orus read? */
        break;
    if (zero_added)                                 
        sp_globals.processor.speedo.no_X_orus++;             /* Increment X array size */
    n = sp_globals.processor.speedo.no_Y_orus;               /* Prepare to read Y oru values */
#if INCL_RULES
    pos = (fix15)sp_globals.tcb.ypos;
#endif
    }
if (zero_added)                  /* Zero Y oru value added to array? */
    sp_globals.processor.speedo.no_Y_orus++;                 /* Increment Y array size */

#if DEBUG
printf("\nX ORUS\n");
n = sp_globals.processor.speedo.no_X_orus;
for (i = 0; i < n; i++)
    {
    printf("%2d %4d\n", i, sp_plaid.orus[i]);
    }
printf("\nY ORUS\n");
n = sp_globals.processor.speedo.no_Y_orus;
for (i = 0; i < n; i++)
    {
    printf("%2d %4d\n", i, sp_plaid.orus[i + sp_globals.processor.speedo.no_X_orus]);
    }
#endif

}

#if INCL_SQUEEZING || INCL_ISW

FUNCTION LOCAL_PROTO void sp_calculate_x_pix(SPGLOBPTR2
						ufix8 start_edge, ufix8 end_edge,
						ufix16 constr_nr,
						fix31 x_scale, fix31 x_offset,
						fix31 ppo,
						fix15 setwidth_pix)
/*
 * Called by sp_setup_pix_table(PARAMS1) when X squeezing is necessary
 * to insert the correct edge in the global pix array
 */
{
fix15 zone_pix;
fix15 start_oru, end_oru;

/* compute scaled oru coordinates */
start_oru= (fix15)(SQUEEZE_X_ORU(sp_plaid.orus[start_edge], x_scale, x_offset));
end_oru   = (fix15)(SQUEEZE_X_ORU(sp_plaid.orus[end_edge], x_scale, x_offset));

if (!sp_globals.processor.speedo.c_act[constr_nr]) /* constraint inactive */
    {
    /* calculate zone width */
    zone_pix = (fix15)(((((fix31)end_oru - (fix31)start_oru) * ppo) >>
    		sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
    /* check for overflow */
    if (((end_oru-start_oru) > 0) && (zone_pix < 0))
	zone_pix = 0x7fff;
    /* check for minimum */
    if ((ABS(zone_pix)) >= sp_globals.processor.speedo.c_pix[constr_nr])
    	goto Lx;
    }
/* use the zone size from the constr table - scale it */
zone_pix = (fix15)(((SQUEEZE_MULT(x_scale,sp_globals.processor.speedo.c_pix[constr_nr]))
            + sp_globals.pixrnd) & sp_globals.pixfix);

/* look for overflow */
if ((sp_globals.processor.speedo.c_pix[constr_nr] > 0) && (zone_pix < 0))
	zone_pix = 0x7fff;

if (start_edge > end_edge)
    {
    zone_pix = -zone_pix;
    }
Lx:
/* assign pixel value to global pix array */
sp_plaid.spix[end_edge]=sp_plaid.spix[start_edge] + zone_pix;

/* check for overflow */
if (((sp_plaid.spix[start_edge] >0) && (zone_pix >0)) &&
    (sp_plaid.spix[end_edge] < 0))
	sp_plaid.spix[end_edge] = 0x7fff; /* set it to the max */

/* be sure to be in the setwidth !*/
#if INCL_ISW
if (!sp_globals.processor.speedo.import_setwidth_act) /* only check left edge if not isw only */
#endif
if ((sp_globals.pspecs->flags & SQUEEZE_LEFT) && (sp_plaid.spix[end_edge] < 0))
    sp_plaid.spix[end_edge] = 0;
if ((sp_globals.pspecs->flags & SQUEEZE_RIGHT) && 
    (sp_plaid.spix[end_edge] > setwidth_pix))
    sp_plaid.spix[end_edge] = setwidth_pix;

}
#endif


#if INCL_SQUEEZING

FUNCTION LOCAL_PROTO void sp_calculate_y_pix(SPGLOBPTR2
						ufix8 start_edge, ufix8 end_edge,
						ufix16 constr_nr,
						fix31 top_scale, fix31 bottom_scale,
						fix31 ppo,
						fix15 em_top_pix,
						fix15 em_bot_pix)
/*
 * Called by sp_setup_pix_table(PARAMS1) when Y squeezing is necessary
 * to insert the correct edge in the global pix array
 */
{
fix15 zone_pix;
fix15 start_oru, end_oru;
fix31 zone_width, above_base, below_base;

/* check whether edge is above or below the baseline                */
/* and apply appropriate scale factor to get scaled oru coordinates */
if (sp_plaid.orus[start_edge] < 0)
    start_oru =(fix15)(SQUEEZE_MULT(sp_plaid.orus[start_edge], bottom_scale));
else
    start_oru =(fix15)(SQUEEZE_MULT(sp_plaid.orus[start_edge], top_scale));

if (sp_plaid.orus[end_edge] < 0)
    end_oru =(fix15)(SQUEEZE_MULT(sp_plaid.orus[end_edge], bottom_scale));
else
    end_oru =(fix15)(SQUEEZE_MULT(sp_plaid.orus[end_edge], top_scale));

if (!sp_globals.processor.speedo.c_act[constr_nr])   /* Constraint inactive? */
   {
   /* calculate zone width */
    zone_pix = (fix15)(((((fix31)end_oru - (fix31)start_oru) * ppo)
		>> sp_globals.mpshift)+ sp_globals.pixrnd) & sp_globals.pixfix;
   /* check minimum */
    if ((ABS(zone_pix)) >= sp_globals.processor.speedo.c_pix[constr_nr])
                    goto Ly;
    }

/* Use zone size from constr table */
if ((end_oru >= 0) && (start_oru >=0))
    /* all above baseline */
    zone_pix = (fix15)(SQUEEZE_MULT(top_scale, sp_globals.processor.speedo.c_pix[constr_nr]));
else if ((end_oru <= 0) && (start_oru <=0))
    /* all below baseline */
    zone_pix = (fix15)(SQUEEZE_MULT(bottom_scale, sp_globals.processor.speedo.c_pix[constr_nr]));
else
    {
    /* mixture */
    if (start_oru > 0)
        {
	zone_width = start_oru - end_oru;
        /* get % above baseline in 16.16 fixed point */
        above_base = (((fix31)start_oru) << 16) /
		     ((fix31)zone_width) ;
        /* get % below baseline in 16.16 fixed point */
        below_base = (((fix31)-end_oru) << 16) /
		     ((fix31)zone_width) ;
	}
    else
        {
        zone_width = end_oru - start_oru;
        /* get % above baseline in 16.16 fixed point */
        above_base = (((fix31)-start_oru) << 16) /
		     ((fix31)zone_width) ;
        /* get % below baseline in 16.16 fixed point */
        below_base = (((fix31)end_oru) << 16) /
		     ((fix31)zone_width) ;
       }
    /* % above baseline * total zone * top_scale +  */
    /* % below baseline * total zone * bottom_scale */
    zone_pix = ((((above_base * (fix31)sp_globals.processor.speedo.c_pix[constr_nr]) >> 16) *
                top_scale) +
	       (((below_base * (fix31)sp_globals.processor.speedo.c_pix[constr_nr]) >> 16) *
		bottom_scale)) >> 16;
    }

/* make this zone pix fall on a pixel boundary */
zone_pix = (zone_pix + sp_globals.pixrnd) & sp_globals.pixfix;

/* if minimum is in effect make the zone one pixel */
if ((sp_globals.processor.speedo.c_pix[constr_nr] != 0) && (zone_pix < sp_globals.onepix)) 
    zone_pix = sp_globals.onepix; 
    
if (start_edge > end_edge) 
       {
        zone_pix = -zone_pix; /* Use negatve zone size */
        }
Ly:
/* assign global pix value */
sp_plaid.spix[end_edge] = sp_plaid.spix[start_edge] + zone_pix; /* Insert end pixels */

/* make sure it is in the EM !*/
if ((sp_globals.pspecs->flags & SQUEEZE_TOP) && 
    (sp_plaid.spix[end_edge] > em_top_pix))
    sp_plaid.spix[end_edge] = em_top_pix;
if ((sp_globals.pspecs->flags & SQUEEZE_BOTTOM) &&
    (sp_plaid.spix[end_edge] < em_bot_pix))
    sp_plaid.spix[end_edge] = em_bot_pix;
}


FUNCTION btsbool sp_calculate_x_scale(SPGLOBPTR2 fix31 *x_factor, fix31 *x_offset, fix15 no_X_ctrl_zones)
/*
 * Called by sp_setup_pix_table(PARAMS1) when squeezing is included
 * to determine whether X scaling is necessary.  If it is, the
 * scale factor and offset are computed.  This function returns
 * a boolean value TRUE = X squeezind is necessary, FALSE = no
 * X squeezing is necessary.
 */
{
btsbool squeeze_left, squeeze_right;
btsbool out_on_right, out_on_left;
fix15 bbox_width,set_width;
fix15 bbox_xmin, bbox_xmax;
fix15 x_offset_pix;
fix15 i;
#if INCL_ISW
fix31 isw_scale;
fix15 shift;
#endif


/* set up some flags and common calculations */
squeeze_left = (sp_globals.pspecs->flags & SQUEEZE_LEFT)? TRUE:FALSE;
squeeze_right = (sp_globals.pspecs->flags & SQUEEZE_RIGHT)? TRUE:FALSE;
bbox_xmin = sp_globals.processor.speedo.bbox_xmin_orus;
bbox_xmax = sp_globals.processor.speedo.bbox_xmax_orus;
set_width = sp_globals.processor.speedo.setwidth_orus;

if (bbox_xmax > set_width)
    out_on_right = TRUE;
else
    out_on_right = FALSE;
if (bbox_xmin < 0)
    out_on_left = TRUE;
else
    out_on_left = FALSE;
bbox_width =bbox_xmax - bbox_xmin;

/*
 * don't need X squeezing if:
 *     - X squeezing not enabled
 *     - bbox doesn't violate on left or right
 *     - left squeezing only is enabled and char isn't out on left
 *     - right squeezing only is enabled and char isn't out on right
 */

if ((!squeeze_left && !squeeze_right) || 
   (!out_on_right && !out_on_left) ||     
   (squeeze_left && !squeeze_right && !out_on_left) ||
   (squeeze_right && !squeeze_left && !out_on_right))
    return FALSE;

#if INCL_ISW
if (sp_globals.processor.speedo.import_setwidth_act)
    {
    /* if both isw and squeezing is going on - let the imported */
    /* setwidth factor be factored in with the squeeze          */
    isw_scale = sp_compute_isw_scale(PARAMS1);
    /*sp_globals.processor.speedo.setwidth_orus = sp_globals.processor.speedo.imported_width;*/
    }
else
    isw_scale = 0x10000L; /* 1 in 16.16 notation */
#endif

/* squeezing on left and right ?  */
if (squeeze_left && squeeze_right)
    {
    /* calculate scale factor */
    if (bbox_width < set_width)
	*x_factor = 0x10000L; /* 1 in 16.16 notation */
    else
	*x_factor = ((fix31)set_width<<16)/(fix31)bbox_width;
#if INCL_ISW
    IMPORT_FACTOR
#endif
    /* calculate offset */
    if (out_on_left) /* fall out on left ? */
	*x_offset = -(fix31)*x_factor * (fix31)bbox_xmin;
    /* fall out on right and I am shifting only ? */
    else if (out_on_right && (*x_factor == 0x10000L))
        *x_offset = -(fix31)*x_factor * (fix31)(bbox_xmax - set_width);
    else
	*x_offset = 0x0L; /* 0 in 16.16 notation */
    }
/* squeezing on left only and violates left */
else if (squeeze_left)
    {
    if (bbox_width < set_width) /* will it fit if I shift it ? */
	*x_factor = 0x10000L; /* 1 in 16.16 notation */
    else if (out_on_right)
	*x_factor = ((fix31)set_width<<16)/(fix31)bbox_width;
    else
	*x_factor = ((fix31)set_width<<16)/
		    (fix31)(bbox_width - (bbox_xmax-set_width));
#if INCL_ISW
    IMPORT_FACTOR
#endif
    *x_offset = (fix31)-*x_factor * (fix31)bbox_xmin;
    }

/* I must be squeezing on right, and violates right */
else 
    {
    if (bbox_width < set_width) /* will it fit if I shift it ? */
	{  /* just shift it left - it will fit in the bbox */
        *x_factor = 0x10000L; /* 1 in 16.16 notation */
#if INCL_ISW
    IMPORT_FACTOR
#endif
        *x_offset = (fix31)-*x_factor * (fix31)bbox_xmin;
	}
    else if (out_on_left)
	{
        *x_factor = ((fix31)set_width<<16)/(fix31)bbox_width;
#if INCL_ISW
    IMPORT_FACTOR
#endif
	*x_offset = 0x0L; /* 0 in 16.16 notation */
	}
    else
	{
        *x_factor = ((fix31)set_width<<16)/(fix31)bbox_xmax;
#if INCL_ISW
    IMPORT_FACTOR
#endif
	*x_offset = 0x0L; /* 0 in 16.16 notation */
 	}
    }

x_offset_pix = (fix15)(((*x_offset >> 16) * sp_globals.tcb0.xppo)
		>> sp_globals.mpshift); 

if ((x_offset_pix >0) && (x_offset_pix < sp_globals.onepix))
    x_offset_pix = sp_globals.onepix; 

/* look for the first non-negative oru value, scale and add the offset    */
/* to the corresponding pixel value - note that the pixel value           */
/* is set in read_oru_table.                                              */

/* look at all the X edges */
for (i=0; i < (no_X_ctrl_zones+1); i++)
    if (sp_plaid.orus[i] >= 0)
        {
        sp_plaid.spix[i] = (SQUEEZE_MULT(sp_plaid.spix[i], *x_factor) 
		  +sp_globals.pixrnd + x_offset_pix) & sp_globals.pixfix;
        break;
        }

return TRUE;
}


FUNCTION btsbool sp_calculate_y_scale(SPGLOBPTR2
					fix31 *top_scale, fix31 *bottom_scale, 
					fix15 first_Y_zone, fix15 no_Y_ctrl_zones)
/*
 * Called by sp_setup_pix_table(PARAMS1) when squeezing is included
 * to determine whether Y scaling is necessary.  If it is, 
 * two scale factors are computed, one for above the baseline,
 * and one for below the basline.
 * This function returns a boolean value TRUE = Y squeezind is necessary, 
 * FALSE = no Y squeezing is necessary.
 */
{
btsbool squeeze_top, squeeze_bottom;
btsbool out_on_top, out_on_bottom;
fix15 	bbox_top, bbox_bottom;
fix15 	bbox_height;
fix15   i;

/* set up some flags and common calculations */
squeeze_top = (sp_globals.pspecs->flags & SQUEEZE_TOP)? TRUE:FALSE;
squeeze_bottom = (sp_globals.pspecs->flags & SQUEEZE_BOTTOM)? TRUE:FALSE;
bbox_top = sp_globals.processor.speedo.bbox_ymax_orus;
bbox_bottom = sp_globals.processor.speedo.bbox_ymin_orus;
bbox_height = bbox_top - bbox_bottom;

if (bbox_top > EM_TOP)
    out_on_top = TRUE;
else
    out_on_top = FALSE;

if (bbox_bottom < EM_BOT)
    out_on_bottom = TRUE;
else
    out_on_bottom = FALSE;

/*
 * don't need Y squeezing if:
 *     - Y squeezing not enabled
 *     - bbox doesn't violate on top or bottom
 *     - top squeezing only is enabled and char isn't out on top
 *     - bottom squeezing only is enabled and char isn't out on bottom
 */
if ((!squeeze_top && !squeeze_bottom) || 
    (!out_on_top && !out_on_bottom) ||
    (squeeze_top && !squeeze_bottom && !out_on_top) || 
    (squeeze_bottom && !squeeze_top && !out_on_bottom)) 
    return FALSE;

if (squeeze_top && (bbox_top > EM_TOP))
    *top_scale = ((fix31)EM_TOP << 16)/(fix31)(bbox_top);
else
    *top_scale = 0x10000L;  /* 1 in 16.16 fixed point */

if (squeeze_bottom && (bbox_bottom < EM_BOT))
    *bottom_scale = ((fix31)-(EM_BOT) << 16)/(fix31)-bbox_bottom;
else
    *bottom_scale = 0x10000L;

if (sp_globals.processor.speedo.squeezing_compound)
    {
    for (i=first_Y_zone; i < (first_Y_zone + no_Y_ctrl_zones + 1); i++)
        {
        if (sp_plaid.orus[i] >= 0)
            sp_plaid.spix[i] = (SQUEEZE_MULT(sp_plaid.spix[i], *top_scale)
                              +sp_globals.pixrnd) & sp_globals.pixfix;
        else
            sp_plaid.spix[i] = (SQUEEZE_MULT(sp_plaid.spix[i], *bottom_scale)
                              +sp_globals.pixrnd) & sp_globals.pixfix;
        }
    }
return TRUE;
}
#endif


#if INCL_RULES

FUNCTION LOCAL_PROTO void sp_setup_pix_table(SPGLOBPTR2 
										ufix8 FONTFAR *STACKFAR *pp1,
										ufix8 FONTFAR *STACKFAR *pp2,
										btsbool short_form,
										fix15 no_X_ctrl_zones, fix15 no_Y_ctrl_zones)
/*
 * Called by sp_plaid_tcb(PARAMS1) to read the control zone table from the
 * character data in the font.
 * Sets up a table of pixel values for all controlled coordinates. 
 * Updates the pointers to the byte following the control zone
 * data.
 */
{
fix15    i, j, n;
fix31    ppo;  
fix31    xppo0; /* top level pixels per oru */
fix31    yppo0; /* top level pixels per oru */
ufix8    edge_org;
ufix8    edge;
ufix8    start_edge;
ufix8    end_edge;
ufix16   constr_org;
fix15    constr_nr;
fix31    zone_pix;
fix31    whole_zone; /* non-transformed value of the first X zone */
ufix16   tmpufix16;  /* in extended mode, macro uses secnd term */
#if INCL_SQUEEZING
fix31    x_scale;
fix31	 y_top_scale, y_bottom_scale;
fix31    x_offset;
btsbool  squeezed_y;
fix15    setwidth_pix, em_top_pix, em_bot_pix;
#endif

#if INCL_ISW
btsbool  imported_width;
fix31	 isw_scale;
fix15    isw_setwidth_pix;
#endif

#if INCL_ISW || INCL_SQUEEZING
btsbool squeezed_x;
#endif

#if INCL_PLAID_OUT               /* Plaid data monitoring included? */
begin_ctrl_zones(no_X_ctrl_zones, no_Y_ctrl_zones);
#endif                                                    


edge_org = 0;
constr_org = 0;
sp_globals.rnd_xmin = 0;  /* initialize the error for chars with no zone */
n = no_X_ctrl_zones;
ppo = sp_globals.tcb.xppo;
xppo0 = sp_globals.tcb0.xppo;
yppo0 = sp_globals.tcb0.yppo;
#if INCL_SQUEEZING || INCL_ISW
squeezed_x = FALSE;
#endif

#if INCL_SQUEEZING
squeezed_x = sp_calculate_x_scale (PARAMS2 &x_scale, &x_offset, no_X_ctrl_zones);
squeezed_y = sp_calculate_y_scale(PARAMS2 &y_top_scale,&y_bottom_scale,(n+1),
	     no_Y_ctrl_zones);
#if INCL_ISW
if (sp_globals.processor.speedo.import_setwidth_act == TRUE)
setwidth_pix = ((fix15)(((fix31)sp_globals.processor.speedo.imported_width * xppo0) >> 
	     sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;

else
#endif
setwidth_pix = ((fix15)(((fix31)sp_globals.processor.speedo.setwidth_orus * xppo0) >> 
	     sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
/* check for overflow */
if (setwidth_pix < 0)
	setwidth_pix = 0x7fff; /* set to maximum */
em_bot_pix = ((fix15)(((fix31)EM_BOT * yppo0) >> 
	     sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
em_top_pix = ((fix15)(((fix31)EM_TOP * yppo0) >> 
	     sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
#endif

#if INCL_ISW
/* convert to pixels */
isw_setwidth_pix = ((fix15)(((fix31)sp_globals.processor.speedo.imported_width * xppo0) >> 
	     sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
/* check for overflow */
if (isw_setwidth_pix < 0)
	isw_setwidth_pix = 0x7fff; /* set to maximum */
if (!squeezed_x && ((imported_width = sp_globals.processor.speedo.import_setwidth_act) == TRUE))
    {
    isw_scale = sp_compute_isw_scale(PARAMS1);

    /* look for the first non-negative oru value, scale and add the offset    */
    /* to the corresponding pixel value - note that the pixel value           */
    /* is set in read_oru_table.                                              */
    
    /* look at all the X edges */
        for (i=0; i < (no_X_ctrl_zones+1); i++)
        if (sp_plaid.orus[i] >= 0)
           {
           sp_plaid.spix[i] = (SQUEEZE_MULT(sp_plaid.spix[i], isw_scale)
                  +sp_globals.pixrnd) & sp_globals.pixfix;
           break;
           }

    }
#endif

for (i = 0; ; i++)               /* For X and Y control zones... */
    {
    for (j = 0; j < n; j++)      /* For each zone in X or Y... */
        {
        if (short_form)          /* 1 byte from/to specification? */
            {
            edge = NEXT_BYTE(*pp1); /* Read packed from/to spec */
            start_edge = edge_org + (edge & 0x0f); /* Extract start edge */
            end_edge = edge_org + (edge >> 4); /* Extract end edge */
            }
        else                     /* 2 byte from/to specification? */
            {
            start_edge = edge_org + NEXT_BYTE(*pp1); /* Read start edge */
            end_edge = edge_org + NEXT_BYTE(*pp1); /* read end edge */
            }
        constr_nr = constr_org +
            NEXT_BYTES(*pp1, tmpufix16); /* Read constraint number */ 
#if INCL_SQUEEZING
        if (i == 0 && squeezed_x)
	    sp_calculate_x_pix(PARAMS2 start_edge, end_edge, constr_nr,
                            x_scale, x_offset, ppo, setwidth_pix);
	else if (i == 1 && squeezed_y)
	    sp_calculate_y_pix(PARAMS2 start_edge, end_edge,constr_nr,
 		y_top_scale, y_bottom_scale, ppo, em_top_pix, em_bot_pix);
	else
	{
#endif
#if INCL_ISW
	if (i==0 && imported_width)
            sp_calculate_x_pix(PARAMS2 start_edge, end_edge, constr_nr,
                            isw_scale, 0,  ppo, isw_setwidth_pix);
	else
	{
#endif
        if (!sp_globals.processor.speedo.c_act[constr_nr])   /* Constraint inactive? */
            {
            zone_pix = ((fix31)((((fix31)sp_plaid.orus[end_edge] - (fix31)sp_plaid.orus[start_edge]) 
                                * ppo) >> sp_globals.mpshift) + sp_globals.pixrnd) & sp_globals.pixfix;
            if ((ABS(zone_pix)) >= sp_globals.processor.speedo.c_pix[constr_nr])
                goto L1;
            }
        zone_pix = (fix31)sp_globals.processor.speedo.c_pix[constr_nr]; /* Use zone size from constr table */
        if (start_edge > end_edge) /* sp_plaid.orus[start_edge] > sp_plaid.orus[end_edge]? */
            {
            zone_pix = -zone_pix; /* Use negatve zone size */
            }
    L1:
                        /* inter-character spacing fix */
        if ((j == 0) && (i == 0))      /* if this is the 1st X zone, save rounding error */
            {                          /*  get the non-xformed - xformed zone, in right direction */
            whole_zone = (((fix31)sp_plaid.orus[end_edge] - (fix31)sp_plaid.orus[start_edge]) 
                                * ppo) >> sp_globals.mpshift;
            sp_globals.rnd_xmin = whole_zone - zone_pix;
            }
        sp_plaid.spix[end_edge] = (fix15)(sp_plaid.spix[start_edge] + zone_pix); /* Insert end pixels */
#if INCL_SQUEEZING
        if (i == 0)  /* in the x direction */
            { /* brute force squeeze */
            if ((sp_globals.pspecs->flags & SQUEEZE_LEFT) && 
                (sp_plaid.spix[end_edge] < 0))
                sp_plaid.spix[end_edge] = 0;
            if ((sp_globals.pspecs->flags & SQUEEZE_RIGHT) && 
                (sp_plaid.spix[end_edge] > setwidth_pix))
                sp_plaid.spix[end_edge] = setwidth_pix;
            }
        if (i == 1) /* in the y direction */
            {  /* brute force squeeze */
            if ((sp_globals.pspecs->flags & SQUEEZE_TOP) && 
                (sp_plaid.spix[end_edge] > em_top_pix))
                sp_plaid.spix[end_edge] = em_top_pix;
            if ((sp_globals.pspecs->flags & SQUEEZE_BOTTOM) &&
                (sp_plaid.spix[end_edge] < em_bot_pix))
                sp_plaid.spix[end_edge] = em_bot_pix;
            }
#endif
#if INCL_SQUEEZING
	}
#endif
#if INCL_ISW
	}
#endif
#if INCL_PLAID_OUT               /* Plaid data monitoring included? */
        record_ctrl_zone(
            (fix31)sp_plaid.spix[start_edge] << (16 - sp_globals.pixshift), 
            (fix31)sp_plaid.spix[end_edge] << (16 - sp_globals.pixshift), 
            (fix15)(constr_nr - constr_org));
#endif
        }
    if (i)                       /* Y pixels done? */
        break;                                          
    edge_org = sp_globals.processor.speedo.Y_edge_org;       /* Prepare to process Y ctrl zones */
    constr_org = sp_globals.processor.speedo.Y_constr_org;
    n = no_Y_ctrl_zones;                      
    ppo = sp_globals.tcb.yppo;                            
    }

#if DEBUG
printf("\nX PIX TABLE\n");
n = no_X_ctrl_zones + 1;
for (i = 0; i < n; i++)
    printf("%2d %6.1f\n", i, (real)sp_plaid.spix[i] / (real)sp_globals.onepix);
printf("\nY PIX TABLE\n");
n = no_Y_ctrl_zones + 1;
for (i = 0; i < n; i++)
    {
    j = i + no_X_ctrl_zones + 1;
    printf("%2d %6.1f\n", i, (real)sp_plaid.spix[j] / (real)sp_globals.onepix);
    }
#endif

}
#endif



#if INCL_RULES

FUNCTION LOCAL_PROTO void sp_setup_int_table(SPGLOBPTR2
										ufix8 FONTFAR *STACKFAR *pp1,
										ufix8 FONTFAR *STACKFAR *pp2,
										fix15 no_X_int_zones, fix15 no_Y_int_zones)
/*
 * Called by plaid_tcb() to read the interpolation zone table from the
 * character data in the font. 
 * Sets up a table of interpolation coefficients with one entry for
 * every X or Y interpolation zone.
 * Updates the pointers to the byte following the interpolation zone
 * data.
 */
{
fix15    ii, jj, kk, ll, nn;
ufix8    tmpufix8;
ufix8    format;                /* Format byte for each interpolation zone */
ufix8    format_copy;           /* Saved format byte */
ufix8    edge_org;              /* First control edge in this dimension */
ufix8    last_edge;             /* Last control edge in this dimension */
ufix8    edge;                  /* Index to controlled oru value */
ufix16   adj_factor;            /* Fraction across zone */
fix15    rel_orus;              /* Relative oru value (to edge) */
fix15    start_orus;            /* Absolute oru value at start of int zone */
fix15    end_orus;              /* Absolute oru value at end of int zone */
fix31    zone_orus;             /* Number of orus in zone */
fix15    start_pix;             /* Pixel position at start of int zone */
fix15    end_pix;               /* Pixel position at end of int zone */

#if INCL_MUTABLE
fix15    oru_adj;               /* Adjustment from adjustment vector */
#endif

#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
begin_int_zones(no_X_int_zones, no_Y_int_zones);
#endif

ii = 0;                                 /* Start count of all interpolation zones */
edge_org = 0;
last_edge = sp_globals.processor.speedo.no_X_orus - 1;
nn = no_X_int_zones;
for (jj = 0; ; jj++)                    /* For each dimension... */
    {
    for (kk = 0; kk < nn; kk++)         /* For each interpolation zone... */
        {
        format = NEXT_BYTE(*pp1);       /* Read int zone format byte */
		if ((format & (BIT6 | BIT7)) == (BIT6 | BIT7)) /* Packed start/end point spec? */
			{
			edge = edge_org + (format & 0x07);		/* start in bits 0, 1, 2 */
			start_orus = sp_plaid.orus[edge];
			start_pix = sp_plaid.spix[edge];
			edge = edge_org + ((format & 0x38) >> 3);	/* end in bit3 3, 4, 5 */
			end_orus = sp_plaid.orus[edge];
			end_pix = sp_plaid.spix[edge];
			}
		else if (format & BIT7)              /* Short start/end point format? */
            {
            tmpufix8 = NEXT_BYTE(*pp1);

            edge = edge_org + (tmpufix8 & 0xf);
            start_orus = sp_plaid.orus[edge];
            start_pix = sp_plaid.spix[edge];

            edge = edge_org + (tmpufix8 >> 4);
            end_orus = sp_plaid.orus[edge];
            end_pix = sp_plaid.spix[edge];
            }
        else                            /* Standard start and end point spec? */
            {
            format_copy = format;
            for (ll = 0; ; ll++)        /* For start and end points... */
                {
                switch (format_copy & 0x7) /* Decode start/end point format */
                    {
                case 0:                 /* Index to control edge */
                    edge = edge_org + NEXT_BYTE(*pp1);
                    end_orus = sp_plaid.orus[edge];
                    end_pix = sp_plaid.spix[edge];
                    break;

                case 1:                 /* 1 byte fractional distance to next edge */
                    adj_factor = (ufix16)NEXT_BYTE(*pp1) << 8;
                    goto L1;

                case 2:                 /* 2 byte fractional distance to next edge */
                    adj_factor = (ufix16)NEXT_WORD(*pp1);
                L1: edge = edge_org + NEXT_BYTE(*pp1);
                    rel_orus =
                        (fix15)((((fix31)sp_plaid.orus[edge + 1] - (fix31)sp_plaid.orus[edge]) * 
                        (ufix32)adj_factor + (fix31)32768) >> 16);
#if INCL_MUTABLE
                    oru_adj = sp_get_adjustment(PARAMS2 pp2); /* Execute adjustment vector */
                    if (oru_adj != 0)   /* Adjustment required? */
                        {
                        goto L3;
                        }
#endif
                    end_orus = sp_plaid.orus[edge] + rel_orus;
                    end_pix = sp_plaid.spix[edge] +
                        (fix15)((((fix31)sp_plaid.spix[edge + 1] - (fix31)sp_plaid.spix[edge]) * 
                        (ufix32)adj_factor + (fix31)32768) >> 16);
                    break;

                case 3:                 /* 1 byte delta orus before first edge */
                    edge = edge_org;
                    rel_orus = -(fix15)NEXT_BYTE(*pp1); 
                    goto L2;

                case 4:                 /* 2 byte delta orus before first edge */
                    edge = edge_org;
                    rel_orus = -NEXT_WORD(*pp1);
                    goto L2;

                case 5:                 /* 1 byte delta orus after last edge */
                    edge = last_edge;
                    rel_orus = (fix15)NEXT_BYTE(*pp1);
                    goto L2;

                case 6:                 /* 2 byte delta orus after last edge */
                    edge = last_edge;
                    rel_orus = NEXT_WORD(*pp1);

                L2: 
#if INCL_MUTABLE
                    rel_orus += sp_get_adjustment(PARAMS2 pp2); /* Execute adjustment vector */

                L3:
                    while ((edge < last_edge) &&
                           (rel_orus > sp_plaid.orus[edge + 1] - sp_plaid.orus[edge]))
                        {
                        rel_orus -= sp_plaid.orus[edge + 1] - sp_plaid.orus[edge];
                        edge++;
                        }

                    while ((edge > edge_org) &&
                           (rel_orus < 0))
                        {
                        rel_orus += sp_plaid.orus[edge] - sp_plaid.orus[edge - 1];
                        edge--;
                        }

                    if ((edge == edge_org) &&
                        (rel_orus < 0)) /* Before first edge? */
                        {
                        goto L4;
                        }

                    if ((edge == last_edge) &&
                        (rel_orus >= 0)) /* On or after last edge? */
                        {
                        goto L4;
                        }

                    end_orus = sp_plaid.orus[edge] + rel_orus;
                    zone_orus = sp_plaid.orus[edge + 1] - sp_plaid.orus[edge];
                    adj_factor = 
                        ((ufix32)(rel_orus) << 16) / zone_orus;
                    end_pix = sp_plaid.spix[edge] +
                        ((((fix31)sp_plaid.spix[edge + 1] - (fix31)sp_plaid.spix[edge]) * 
                        (ufix32)adj_factor + (fix31)32768) >> 16);
                    break;

                L4:
#endif
                    end_orus = sp_plaid.orus[edge] + rel_orus;
                    end_pix = sp_plaid.spix[edge] + 
                        (fix15)(((fix31)rel_orus * (fix31)(jj? sp_globals.tcb.yppo: sp_globals.tcb.xppo) + 
                          sp_globals.mprnd) >> sp_globals.mpshift);
                    break;
                    }

                if (ll)                 /* Second time round loop? */
                    break;
                format_copy >>= 3;      /* Adj format to decode end point format */
                start_orus = end_orus;  /* Save start point oru value */
                start_pix = end_pix;    /* Save start point pixel value */
                }
            }

#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
        record_int_zone(
            (fix31)start_pix << (16 - sp_globals.pixshift), 
            (fix31)end_pix << (16 - sp_globals.pixshift));
#endif

        zone_orus = (fix31)end_orus - (fix31)start_orus;
        if (zone_orus == 0)
            {
            sp_plaid.mult[ii] = 0;
            }
        else
            {
            sp_plaid.mult[ii] = 
                (fix15)(((((fix31)end_pix - 
                   (fix31)start_pix) << 
                  sp_globals.mpshift) + 
                 (zone_orus >> 1)) / 
                zone_orus);
            }
        sp_plaid.offset[ii] = 
            (((((fix31)start_pix + 
                (fix31)end_pix) << 
               sp_globals.mpshift) - 
              ((fix31)sp_plaid.mult[ii] * 
               ((fix31)start_orus + 
                (fix31)end_orus))) >> 
             1) + 
            sp_globals.mprnd;
        ii++;
        }
    if (jj)                              /* Finished? */
        break;
    edge_org = sp_globals.processor.speedo.Y_edge_org;   /* Prepare to process Y ctrl zones */
    last_edge = sp_globals.processor.speedo.Y_edge_org + sp_globals.processor.speedo.no_Y_orus - 1;
    nn = no_Y_int_zones;
    }

#if DEBUG
printf("\nX INT TABLE\n");
nn = no_X_int_zones;
for (ii = 0; ii < nn; ii++)
    {
    printf("%2d %7.4f %7.4f\n",
        ii, 
        (real)sp_plaid.mult[ii] / (real)(1 << sp_globals.multshift), 
        (real)sp_plaid.offset[ii] / (real)(1 << sp_globals.multshift));
    }

printf("\nY INT TABLE\n");
nn = no_Y_int_zones;
for (ii = 0; ii < nn; ii++)
    {
    jj = ii + no_X_int_zones;
    printf("%2d %7.4f %7.4f\n",
        ii, 
        (real)sp_plaid.mult[jj] / (real)(1 << sp_globals.multshift), 
        (real)sp_plaid.offset[jj] / (real)(1 << sp_globals.multshift));
    }
#endif
}
#endif

#if INCL_ISW

FUNCTION fix31 sp_compute_isw_scale(SPGLOBPTR1)
{
fix31 isw_scale;
	
if (sp_globals.processor.speedo.setwidth_orus == 0)
    isw_scale = 0x00010000;
else
    isw_scale = ((fix31)sp_globals.processor.speedo.imported_width << 16)/
                 (fix31)sp_globals.processor.speedo.setwidth_orus;
return isw_scale;
}
#endif

#if INCL_RULES
#if INCL_AUTOIZONES

FUNCTION LOCAL_PROTO void sp_SetInterpolation(SPGLOBPTR2 fix15 dimension)
{
fix15   ppo;
fix31   pos;
fix15   nOrus;
fix15  *pOrus;
fix15  *pPix;
fix15  *pMult;
fix31  *pOffset;
fix15   ii;
fix15   deltaOrus;

if (dimension == 'X')             /* X dimension? */
    {
    ppo = sp_globals.tcb.xppo;
    pos = sp_globals.tcb.xpos;
    nOrus = sp_globals.processor.speedo.no_X_orus;
    pOrus = &sp_plaid.orus[0];
    pPix = &sp_plaid.spix[0];
    pMult = &sp_plaid.mult[0];
    pOffset = &sp_plaid.offset[0];
    }
else                            /* Y dimension? */
    {
    ppo = sp_globals.tcb.yppo;
    pos = sp_globals.tcb.ypos;
    nOrus = sp_globals.processor.speedo.no_Y_orus;
    pOrus = &sp_plaid.orus[sp_globals.processor.speedo.no_X_orus];
    pPix = &sp_plaid.spix[sp_globals.processor.speedo.no_X_orus];
    pMult = &sp_plaid.mult[sp_globals.processor.speedo.Y_int_org];
    pOffset = &sp_plaid.offset[sp_globals.processor.speedo.Y_int_org];
    }

if (nOrus == 0)
    {
    pMult[0] = ppo;
    pOffset[0] = pos + sp_globals.mprnd;
    }
else
    {
    pMult[0] = ppo;
    pOffset[0] = 
        ((fix31)pPix[0] << sp_globals.mpshift) - 
        ((fix31)pOrus[0] * ppo) +
        sp_globals.mprnd;

    for (ii = 1; ii < nOrus; ii++)
        {
        deltaOrus = pOrus[ii] - pOrus[ii - 1];
        if (deltaOrus <= 0)
            {
            pMult[ii] = 0;
            pOffset[ii] = 0;
            }
        else
            {
            pMult[ii] = 
                ((fix31)(pPix[ii] - pPix[ii - 1]) << 
                 sp_globals.mpshift) / 
                (fix31)deltaOrus;
#ifdef D_AutoIzones
			{
			fix31 pix_diff;
			fix31 pMultShift;
			real pMultGuess;
			fix15 onemult = 1 << sp_globals.multshift;
                pix_diff = (fix31)pPix[ii] - (fix31)pPix[ii - 1];
				pMultShift = pix_diff << sp_globals.mpshift;
				pMultGuess = (real)pMultShift / (real)deltaOrus / onemult;
				printf("deltaOrus = %d, sp_globals.mpshift = %d\n",
							(int)deltaOrus, (int)sp_globals.mpshift);
				printf("pix_diff = %d, pMultShift = %d, pMultGuess = %3.1f\n",
					(int)pix_diff, (int)pMultShift, pMultGuess);
			}
#endif
            pOffset[ii] = 
                ((fix31)pPix[ii] << sp_globals.mpshift) - 
                ((fix31)pOrus[ii] * pMult[ii]) +
                sp_globals.mprnd;
#ifdef D_AutoIzones
			{
			fix31 pix_shift, oruMult;
			real pOffGuess;
			fix15 onemult = 1 << sp_globals.multshift;
				pix_shift = ((fix31)pPix[ii] << sp_globals.mpshift);
				oruMult = ((fix31)pOrus[ii] * pMult[ii]);
				pOffGuess = (real)(pix_shift - oruMult + sp_globals.mprnd)/onemult;
				printf("pix_shift = %d, oruMult = %d, pOffGuess = %3.1f\n",
					(int)pix_shift, (int)oruMult, pOffGuess);
			}
#endif
            }
        }

    pMult[ii] = ppo;
    pOffset[ii] = 
        ((fix31)pPix[nOrus - 1] << sp_globals.mpshift) - 
        (fix31)pOrus[nOrus - 1] * ppo +
        sp_globals.mprnd;
    }
/* #if DEBUG >= 2 */
#ifdef D_AutoIzones
    {
    fix15 onemult = 1 << sp_globals.multshift;

    printf("\n%c interpolation table\n", (dimension == 'X')? 'X': 'Y');
    printf("  ORUS     PIX    MULT  OFFSET\n");
    for (ii = 0; ii < nOrus; ii++)
        {
        printf("%6d %7.3f %7.3f %7.3f\n", 
            pOrus[ii], 
            (real)pPix[ii] / sp_globals.onepix, 
            (real)pMult[ii] / onemult, 
            (real)pOffset[ii] / onemult);
        }
    printf("               %7.3f %7.3f\n", 
        (real)pMult[nOrus] / onemult, 
        (real)pOffset[nOrus] / onemult);
    }
#endif
}
#endif /* INCL_AUTOIZONES */
#endif /* INCL_RULES */
#endif /* if PROC_SPEEDO */
