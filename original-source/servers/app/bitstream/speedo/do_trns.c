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
*     $Header: //bliss/user/archive/rcs/speedo/do_trns.c,v 33.0 97/03/17 17:44:47 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	do_trns.c,v $
*       Revision 33.0  97/03/17  17:44:47  shawn
*       TDIS Version 6.00
*       
*       Revision 32.3  97/01/14  17:44:12  shawn
*       Apply casts to avoid compiler warnings.
*       
*       Revision 32.2  96/07/02  12:02:09  shawn
*       Changed boolean to btsbool.
*       
*       Revision 32.1  95/11/17  10:08:51  shawn
*       Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
*       
*       Revision 32.0  95/02/15  16:33:28  shawn
*       Release
*       
*       Revision 31.2  95/01/12  11:16:10  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:31:10  roberte
*       Release
*       
*       Revision 30.2  94/12/21  14:58:49  shawn
*         Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
*       
*       Revision 30.1  94/10/24  10:22:07  roberte
*       Added conditional #if PROC_SPEEDO so processor may be excluded in 4-in-1 builds.
*       
*       Revision 30.0  94/05/04  09:26:05  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:35:14  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:42:04  roberte
*       Release
*       
*       Revision 28.52  94/04/01  15:55:03  roberte
*       Added INCL_PLAID_OUT record_x/y_int() callbacks for AUTOIZONES blocks.
*       Moved some static function prototypes from speedo.h to here.
*       
*       Revision 28.51  94/03/11  16:10:50  roberte
*       Tuned performance of sp_set_x_int() and sp_set_y_int() by
*       saving the position of the last search in the edge lists.
*       These function now decide whether to search in ascending
*       or descending order from this saved position.
*       
*       Revision 28.50  94/03/09  15:19:07  roberte
*       Removed dead INCL_CLOSE_CONTOUR defines, post testing.
*       Added new INCL_AUTOIZONES code blocks, for auto setting of X/YINT zones
*       from edge list.
*       Added functions sp_set_x_int() and sp_set_y_int().
*       
*       Revision 28.49  94/02/10  13:16:46  roberte
*        Upgrade for MUTABLE fonts.
*       
*       Revision 28.48  94/01/05  12:22:49  roberte
*       Changes for the new Speedo compiler upgrades.
*       Automatic contour-closing of open contours.
*       Added HLINE and VLINE commands.
*       Added 5 byte Quadrant curve commands.
*       Broke out point transformation into separate function.
*       
*       Revision 28.47  93/08/31  09:35:24  roberte
*       Release
*       
*       Revision 28.37  93/03/15  12:44:17  roberte
*       Release
*       
*       Revision 28.11  93/01/12  12:19:58  roberte
*       Renamed sp_plaid.pix to sp_plaid.spix to avoid ambiguities with type1
*       
*       Revision 28.10  93/01/08  14:04:14  roberte
*       Changed references to sp_globals. for pspecs, orus_per_em, curves_out, multrnd, pixfix and mpshift.
*       
*       Revision 28.9  93/01/07  12:04:58  roberte
*       Corrected references for function ptr reference curves_out moved from union to common area of SPEEDO_GLOBALS.
*       
*       Revision 28.8  93/01/04  16:39:54  roberte
*       Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
*       
*       Revision 28.7  92/12/30  17:44:05  roberte
*       Functions no longer renamed in spdo_prv.h now declared with "sp_"  
*       Use PARAMS1&2 macros throughout.
*       
*       Revision 28.6  92/11/24  10:54:58  laurar
*       include fino.h
*       
*       Revision 28.5  92/11/19  15:13:01  roberte
*       Release
*       
*       Revision 28.1  92/06/25  13:38:18  leeann
*       Release
*       
*       Revision 27.1  92/03/23  13:54:54  leeann
*       Release
*       
*       Revision 26.1  92/01/30  16:56:07  leeann
*       Release
*       
*       Revision 25.1  91/07/10  11:02:47  leeann
*       Release
*       
*       Revision 24.1  91/07/10  10:36:17  leeann
*       Release
*       
*       Revision 23.1  91/07/09  17:56:45  leeann
*       Release
*       
*       Revision 22.1  91/01/23  17:15:57  leeann
*       Release
*       
*       Revision 21.1  90/11/20  14:35:58  leeann
*       Release
*       
*       Revision 20.1  90/11/12  09:20:22  leeann
*       Release
*       
*       Revision 19.1  90/11/08  10:18:16  leeann
*       Release
*       
*       Revision 18.1  90/09/24  09:50:48  mark
*       Release
*       
*       Revision 17.2  90/09/14  15:01:47  leeann
*       save bounding box in orus if the INCL_ISW flag is set
*       
*       Revision 17.1  90/09/13  15:57:19  mark
*       Release name rel0913
*       
*       Revision 16.1  90/09/11  12:54:24  mark
*       Release
*       
*       Revision 15.1  90/08/29  10:02:41  mark
*       Release name rel0829
*       
*       Revision 14.1  90/07/13  10:38:19  mark
*       Release name rel071390
*       
*       Revision 13.1  90/07/02  10:37:15  mark
*       Release name REL2070290
*       
*       Revision 12.1  90/04/23  12:11:25  mark
*       Release name REL20
*       
*       Revision 11.1  90/04/23  10:11:22  mark
*       Release name REV2
*       
*       Revision 10.3  90/04/12  13:06:05  leeann
*       change compilation option for squeezing to INCL_SQUEEZING
*       
*       Revision 10.2  90/03/29  16:44:28  leeann
*       Added set_flags argument to read_bbox
*       added SQUEEZE code to save the oru bbox when
*       set_flag is TRUE
*       
*       Revision 10.1  89/07/28  18:08:02  mark
*       Release name PRODUCT
*       
*       Revision 9.1  89/07/27  10:21:11  mark
*       Release name PRODUCT
*       
*       Revision 8.1  89/07/13  18:17:51  mark
*       Release name Product
*       
*       Revision 7.1  89/07/11  08:59:24  mark
*       Release name PRODUCT
*       
*       Revision 6.2  89/07/09  14:48:14  mark
*       since get_args takes a STACKFAR pointer to the point returned,
*       transformation failed if stack and globals had different 
*       locations.  Now retrieve P0 into local and assign to global copy.
*       
*       Revision 6.1  89/06/19  08:33:23  mark
*       Release name prod
*       
*       Revision 5.4  89/06/06  17:23:13  mark
*       add curve depth to output module curve functions
*       
*       Revision 5.3  89/06/02  17:03:54  mark
*       use sp_plaid symbol for referring to plaid tables so
*       that these buffers can be conditionally allocated
*       off the stack for reentrant mode.
*       
*       Revision 5.2  89/05/17  16:26:03  john
*       Inhibited conversion of curve to vector when curves_out
*       is on and adjusted depth is zero or less.
*       
*       Revision 5.1  89/05/01  17:51:32  mark
*       Release name Beta
*       
*       Revision 4.1  89/04/27  12:12:10  mark
*       Release name Beta
*       
*       Revision 3.1  89/04/25  08:25:08  mark
*       Release name beta
*       
*       Revision 2.3  89/04/12  12:11:30  mark
*       added stuff for far stack and font
*       
*       Revision 2.2  89/04/10  17:09:00  mark
*       Modified pointer declarations that are used to refer
*       to font data to use FONTFAR symbol, which will be used
*       for Intel SS != DS memory models
*       Modified read_bbox and get_args to receive a pointer and 
*       return the resulting pointer, rather than receiving a 
*       pointer to a pointer
*       
*       Revision 2.1  89/04/04  13:31:49  mark
*       Release name EVAL
*       
*       Revision 1.9  89/04/04  13:17:24  mark
*       Update copyright text
*       
*       Revision 1.8  89/04/03  09:36:01  mark
*       added break; statement to default with null scope because
*       Microsoft C gives questionable syntax error if you don't
*       
*       Revision 1.7  89/03/31  16:58:41  john
*       Default curve support removed.
*       
*       Revision 1.6  89/03/31  14:44:20  mark
*       change speedo.h to spdo_prv.h
*       change comments from fontware to speedo
*       
*       Revision 1.5  89/03/31  12:14:52  john
*       modified to use new NEXT_WORD macro.
*       
*       Revision 1.4  89/03/30  17:47:58  john
*       sp_read_bbox(PARAMS1) rewritten.
*       
*       Revision 1.3  89/03/29  16:08:34  mark
*       changes for slot independence and dynamic/reentrant
*       data allocation
*       
*       Revision 1.2  89/03/21  13:26:05  mark
*       change name from oemfw.h to speedo.h
*       
*       Revision 1.1  89/03/15  12:29:02  mark
*       Initial revision
*                                                                                 *
*                                                                                    *
*************************************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/speedo/do_trns.c,v 33.0 97/03/17 17:44:47 shawn Release $";
#endif


/**************************** D O _ T R N S . C ******************************
 *                                                                           *
 * This module is responsible for executing all intelligent transformation   *
 * for bounding box and outline data                                         *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 16 Dec 88  jsc  Created                                               *
 *                                                                           *
 *  2) 28 Feb 89  jsc  Plaid data monitoring functions added.                *
 *                                                                           *
 ****************************************************************************/


#include "spdo_prv.h"               /* General definitions for Speedo    */
#include "fino.h"

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#if PROC_SPEEDO
/***** GLOBAL VARIABLES *****/

/***** GLOBAL FUNCTIONS *****/

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/

LOCAL_PROTO void sp_transform_point(SPGLOBPTR2 point_t STACKFAR *pP);

#if INCL_AUTOIZONES
LOCAL_PROTO void sp_set_x_int(SPGLOBPTR1);
LOCAL_PROTO void sp_set_y_int(SPGLOBPTR1);
#endif

LOCAL_PROTO void sp_split_curve(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth);
LOCAL_PROTO void sp_get_args(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1, ufix8 FONTFAR *STACKFAR *pp2, ufix8  format, point_t STACKFAR *pP);



FUNCTION void sp_read_bbox(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
									  ufix8 FONTFAR *STACKFAR *pp2,
									  point_t STACKFAR *pPmin,
									  point_t STACKFAR *pPmax,
									  btsbool set_flag)
/*
 * Called by sp_make_simp_char(PARAMS1) and sp_make_comp_char(PARAMS1) to read the 
 * bounding box data from the font.
 * Sets Pmin and Pmax to the bottom left and top right corners
 * of the bounding box after transformation into device space.
 * The units of Pmin and Pmax are sub-pixels.
 * Updates pointers to point to the byte following the
 * bounding box data.
 */
{
ufix8    format1;
ufix8    format;
fix15    i;
point_t  P;

sp_globals.processor.speedo.x_int = 0;
sp_globals.processor.speedo.y_int = sp_globals.processor.speedo.Y_int_org;

#if INCL_AUTOIZONES
sp_globals.processor.speedo.sv_yint = sp_globals.processor.speedo.no_X_orus;
sp_globals.processor.speedo.sv_xint = 0;
#endif

#if INCL_MUTABLE
sp_globals.processor.speedo.raw_x_orus = sp_globals.processor.speedo.raw_y_orus = 0;
#endif

sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.y_orus = 0;
format1 = NEXT_BYTE(*pp1);
sp_get_args(PARAMS2 pp1, pp2, format1, pPmin);

#if INCL_SQUEEZING || INCL_ISW
if (set_flag)
    {
    sp_globals.processor.speedo.bbox_xmin_orus = sp_globals.processor.speedo.x_orus;
    sp_globals.processor.speedo.bbox_ymin_orus = sp_globals.processor.speedo.y_orus;
    }
#endif

*pPmax = *pPmin;
for (i = 1; i < 4; i++)
    {
    switch(i)
        {
    case 1:
        if (format1 & BIT6)            /* Xmax requires X int zone 1? */
            sp_globals.processor.speedo.x_int++;
        format = (format1 >> 4) | 0x0c;
        break;

    case 2:
        if (format1 & BIT7)            /* Ymax requires Y int zone 1? */
            sp_globals.processor.speedo.y_int++;
        format = NEXT_BYTE(*pp1);
        break;

    case 3:
        sp_globals.processor.speedo.x_int = 0; 
        format >>= 4;
        break;

    default:
		break;
        }

    sp_get_args(PARAMS2 pp1, pp2, format, &P);
#if INCL_SQUEEZING || INCL_ISW
    if (set_flag && (i==2))
	{
	sp_globals.processor.speedo.bbox_xmax_orus = sp_globals.processor.speedo.x_orus;
	sp_globals.processor.speedo.bbox_ymax_orus = sp_globals.processor.speedo.y_orus;
	}
#endif
    if ((i == 2) || (!sp_globals.normal)) 
        {
        if (P.x < pPmin->x)
            pPmin->x = P.x;
        if (P.y < pPmin->y)
            pPmin->y = P.y;
        if (P.x > pPmax->x)
            pPmax->x = P.x;
        if (P.y > pPmax->y)
            pPmax->y = P.y;
        }
    }

#if DEBUG
printf("BBOX %6.1f(Xint 0), %6.1f(Yint 0), %6.1f(Xint %d), %6.1f(Yint %d)\n",
    (real)pPmin->x / (real)sp_globals.onepix, 
    (real)pPmin->y / (real)sp_globals.onepix, 
    (real)pPmax->x / (real)sp_globals.onepix, 
    (format1 >> 6) & 0x01,
    (real)pPmax->y / (real)sp_globals.onepix,
    (format1 >> 7) & 0x01);

#endif
}


FUNCTION void sp_proc_outl_data(SPGLOBPTR2 ufix8 FONTFAR *p1, ufix8 FONTFAR *p2)
/*
 * Called by sp_make_simp_char(PARAMS1) and sp_make_comp_char(PARAMS1) to read the 
 * outline data from the font.
 * The outline data is parsed, transformed into device coordinates
 * and passed to an output module for further processing.
 * Note that pointer is not updated to facilitate repeated
 * processing of the outline data when banding mode is in effect.
 */
{
ufix8    format1, format2;
point_t  P0, P1, P2, P3;
fix15    depth;
fix15    curve_count;
ufix8   edge;

sp_globals.processor.speedo.x_int = 0;
sp_globals.processor.speedo.y_int = sp_globals.processor.speedo.Y_int_org;

#if INCL_AUTOIZONES
sp_globals.processor.speedo.sv_yint = sp_globals.processor.speedo.no_X_orus;
sp_globals.processor.speedo.sv_xint = 0;
#endif

#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
record_xint((fix15)sp_globals.processor.speedo.x_int);         /* Record xint data */
record_yint((fix15)(sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org)); /* Record yint data */
#endif

#if INCL_MUTABLE
sp_globals.processor.speedo.raw_x_orus = sp_globals.processor.speedo.raw_y_orus = 0;
#endif

sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.y_orus = 0;
curve_count = 0;
while(TRUE)
    {
    format1 = NEXT_BYTE(p1);
    switch(format1 >> 4)
        {
    case 0:                        /* LINE */
        sp_get_args(PARAMS2 &p1, &p2, format1, &P1);
#if DEBUG
        printf("LINE %6.1f, %6.1f\n",
            (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix);
#endif
        fn_line(P1);
        sp_globals.processor.speedo.P0 = P1;
        continue;

    case 1:                        /* Short XINT */
        sp_globals.processor.speedo.x_int = format1 & 0x0f;
#if DEBUG
        printf("XINT %d\n", sp_globals.processor.speedo.x_int);
#endif
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
record_xint((fix15)sp_globals.processor.speedo.x_int);         /* Record xint data */
#endif
        continue;

    case 2:                        /* Short YINT */
        sp_globals.processor.speedo.y_int = sp_globals.processor.speedo.Y_int_org + (format1 & 0x0f);
#if DEBUG
        printf("YINT %d\n", sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org);
#endif
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
record_yint((fix15)(sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org)); /* Record yint data */
#endif
        continue;
         
    case 3:                        /* Miscellaneous */
        switch(format1 & 0x0f)
            {
        case 0:                    /* END */
            if (curve_count)
                {
				if (!((sp_globals.processor.speedo.P0.x == P0.x) &&
						(sp_globals.processor.speedo.P0.y == P0.y)) )
					{/* close the contour with a vector */
        			fn_line(P0);
#if DEBUG
        			printf("LINE %6.1f, %6.1f\n",
            				(real)P0.x / (real)sp_globals.onepix, (real)P0.y / (real)sp_globals.onepix);
#endif
					}
                fn_end_contour();
                }
            return;

        case 1:                     /* Long XINT */
            sp_globals.processor.speedo.x_int = NEXT_BYTE(p1);
#if DEBUG
            printf("XINT %d\n", sp_globals.processor.speedo.x_int);
#endif
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
record_xint((fix15)sp_globals.processor.speedo.x_int);         /* Record xint data */
#endif
            continue;

        case 2:                     /* Long YINT */
            sp_globals.processor.speedo.y_int = sp_globals.processor.speedo.Y_int_org + NEXT_BYTE(p1);
#if DEBUG
            printf("YINT %d\n", sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org);
#endif
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
record_yint((fix15)(sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org)); /* Record yint data */
#endif
            continue;

        default:                    /* Not used */
            continue;
            }    

    case 4:                         /* MOVE Inside */
    case 5:                         /* MOVE Outside */
        if (curve_count++)
            {
			if (!((sp_globals.processor.speedo.P0.x == P0.x) &&
					(sp_globals.processor.speedo.P0.y == P0.y)) )
				{/* close the contour with a vector */
        		fn_line(P0);
#if DEBUG
        		printf("LINE %6.1f, %6.1f\n",
            			(real)P0.x / (real)sp_globals.onepix, (real)P0.y / (real)sp_globals.onepix);
#endif
				}
            fn_end_contour();
            }                                
		
        sp_get_args(PARAMS2 &p1, &p2, format1, &P0);
		sp_globals.processor.speedo.P0 = P0;
#if DEBUG
        printf("MOVE %6.1f, %6.1f\n",
            (real)sp_globals.processor.speedo.P0.x / (real)sp_globals.onepix, (real)sp_globals.processor.speedo.P0.y / (real)sp_globals.onepix);
#endif
        fn_begin_contour(sp_globals.processor.speedo.P0, (btsbool)(format1 & BIT4));
        continue;

    case 6:                         /* Undefined */
		/* HLINE or VLINE */
		if (format1 & 0x08)
			{/* VLINE */
    		edge = sp_globals.processor.speedo.Y_edge_org + (format1 & 0x07);
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_y_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
#endif
    		sp_globals.processor.speedo.y_orus = sp_plaid.orus[edge];
#if INCL_RULES
			sp_globals.processor.speedo.y_pix = sp_plaid.spix[edge];
#endif
			}
		else
			{/* HLINE */
    		edge = (format1 & 0x07);
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_x_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
#endif
    		sp_globals.processor.speedo.x_orus = sp_plaid.orus[edge];
#if INCL_RULES
			sp_globals.processor.speedo.x_pix = sp_plaid.spix[edge];
#endif
			}
		sp_transform_point(PARAMS2 &P1);
#if DEBUG
        printf("LINE %6.1f, %6.1f\n",
            (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix);
#endif
        fn_line(P1);
        sp_globals.processor.speedo.P0 = P1;
        continue;

    case 7:                         /* Undefined */
		/* 5 byte Quadrant curve format */
		if (format1 & 0x08)
			{/* Vertical to Horizontal Quadrant */
			/* X1 unchanged */
			/* Y1 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_y_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.y_orus = sp_globals.processor.speedo.raw_y_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.y_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif

#if INCL_RULES
#if INCL_AUTOIZONES
		if (sp_globals.FourIn1Flags & BIT1)
			sp_set_y_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.y_pix = TRANS(sp_globals.processor.speedo.y_orus, sp_plaid.mult[sp_globals.processor.speedo.y_int], sp_plaid.offset[sp_globals.processor.speedo.y_int], sp_globals.mpshift);
#endif
			sp_transform_point(PARAMS2 &P1);

			/* X2 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_x_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.raw_x_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.x_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif
#if INCL_RULES
#if INCL_AUTOIZONES
			if (sp_globals.FourIn1Flags & BIT1)
				sp_set_x_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.x_pix = TRANS(sp_globals.processor.speedo.x_orus, sp_plaid.mult[sp_globals.processor.speedo.x_int], sp_plaid.offset[sp_globals.processor.speedo.x_int], sp_globals.mpshift);
#endif
			/* Y2 controlled coord (type 0) */
    		edge = sp_globals.processor.speedo.Y_edge_org + NEXT_BYTE(p1);
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_y_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
#endif
    		sp_globals.processor.speedo.y_orus = sp_plaid.orus[edge];
#if INCL_RULES
    		sp_globals.processor.speedo.y_pix = sp_plaid.spix[edge];
#endif
			sp_transform_point(PARAMS2 &P2);

			/* X3 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_x_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.raw_x_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.x_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif
#if INCL_RULES
#if INCL_AUTOIZONES
			if (sp_globals.FourIn1Flags & BIT1)
				sp_set_x_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.x_pix = TRANS(sp_globals.processor.speedo.x_orus, sp_plaid.mult[sp_globals.processor.speedo.x_int], sp_plaid.offset[sp_globals.processor.speedo.x_int], sp_globals.mpshift);
#endif
			/* Y3 unchanged */
			sp_transform_point(PARAMS2 &P3);
			}
		else
			{/* Horizontal to Vertical Quadrant */
			/* X1 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_x_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.raw_x_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.x_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif
#if INCL_RULES
#if INCL_AUTOIZONES
			if (sp_globals.FourIn1Flags & BIT1)
				sp_set_x_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.x_pix = TRANS(sp_globals.processor.speedo.x_orus, sp_plaid.mult[sp_globals.processor.speedo.x_int], sp_plaid.offset[sp_globals.processor.speedo.x_int], sp_globals.mpshift);
#endif
			/* Y1 unchanged */
			sp_transform_point(PARAMS2 &P1);

			/* X2 controlled coord (type 0) */
    		edge = NEXT_BYTE(p1);
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_x_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
#endif
    		sp_globals.processor.speedo.x_orus = sp_plaid.orus[edge];
#if INCL_RULES
    		sp_globals.processor.speedo.x_pix = sp_plaid.spix[edge];
#endif
			/* Y2 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_y_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.y_orus = sp_globals.processor.speedo.raw_y_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.y_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif
#if INCL_RULES
#if INCL_AUTOIZONES
			if (sp_globals.FourIn1Flags & BIT1)
				sp_set_y_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.y_pix = TRANS(sp_globals.processor.speedo.y_orus, sp_plaid.mult[sp_globals.processor.speedo.y_int], sp_plaid.offset[sp_globals.processor.speedo.y_int], sp_globals.mpshift);
#endif
			sp_transform_point(PARAMS2 &P2);

			/* X3 unchanged */
			/* Y3 interpolated relative (type 2) */
#if INCL_MUTABLE
    		sp_globals.processor.speedo.raw_y_orus += (fix15)((fix7)NEXT_BYTE(p1)); /* Save unadjusted oru value */
    		sp_globals.processor.speedo.y_orus = sp_globals.processor.speedo.raw_y_orus + sp_get_adjustment(PARAMS2 &p2);
#else
    		sp_globals.processor.speedo.y_orus += (fix15)((fix7)NEXT_BYTE(p1));
#endif
#if INCL_RULES
#if INCL_AUTOIZONES
			if (sp_globals.FourIn1Flags & BIT1)
				sp_set_y_int(PARAMS1);
#endif /* AUTOIZONES */
    		sp_globals.processor.speedo.y_pix = TRANS(sp_globals.processor.speedo.y_orus, sp_plaid.mult[sp_globals.processor.speedo.y_int], sp_plaid.offset[sp_globals.processor.speedo.y_int], sp_globals.mpshift);
#endif
			sp_transform_point(PARAMS2 &P3);
			}
        depth = format1 & 0x07; /* curve depth in low 3 bits */
#if DEBUG
        printf("CRVE %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %d\n",
            (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix, 
            (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
            (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix,
            depth);
#endif
        depth += sp_globals.processor.speedo.depth_adj;
        if (sp_globals.curves_out)
            {
            fn_curve(P1, P2, P3, depth);
            sp_globals.processor.speedo.P0 = P3;
            continue;
            }
        if (depth <= 0)
            {
            fn_line(P3);
            sp_globals.processor.speedo.P0 = P3;
            continue;
            }   
        sp_split_curve(PARAMS2 P1, P2, P3, depth);
        continue;

    default:                        /* CRVE */
        format2 = NEXT_BYTE(p1);
        sp_get_args(PARAMS2 &p1, &p2, format1, &P1);
        sp_get_args(PARAMS2 &p1, &p2, format2, &P2);
        sp_get_args(PARAMS2 &p1, &p2, (ufix8)(format2 >> 4), &P3);
        depth = (format1 >> 4) & 0x07;
#if DEBUG
        printf("CRVE %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %d\n",
            (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix, 
            (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
            (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix,
            depth);
#endif
        depth += sp_globals.processor.speedo.depth_adj;
        if (sp_globals.curves_out)
            {
            fn_curve(P1, P2, P3, depth);
            sp_globals.processor.speedo.P0 = P3;
            continue;
            }
        if (depth <= 0)
            {
            fn_line(P3);
            sp_globals.processor.speedo.P0 = P3;
            continue;
            }   
        sp_split_curve(PARAMS2 P1, P2, P3, depth);
        continue;
        }
    }
}


FUNCTION LOCAL_PROTO void sp_split_curve(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
/*
 * Called by sp_proc_outl_data(PARAMS1) to subdivide Bezier curves into an
 * appropriate number of vectors, whenever curves are not enabled
 * for output to the currently selected output module.
 * sp_split_curve(PARAMS1) calls itself recursively to the depth specified
 * at which point it calls line() to deliver each vector resulting
 * from the spliting process.
 */
{
fix31   X0 = (fix31)sp_globals.processor.speedo.P0.x;
fix31   Y0 = (fix31)sp_globals.processor.speedo.P0.y;
fix31   X1 = (fix31)P1.x;
fix31   Y1 = (fix31)P1.y;
fix31   X2 = (fix31)P2.x;
fix31   Y2 = (fix31)P2.y;
fix31   X3 = (fix31)P3.x;
fix31   Y3 = (fix31)P3.y;
point_t Pmid;
point_t Pctrl1;
point_t Pctrl2;

#if DEBUG
printf("CRVE(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
    (real)P1.x / (real)sp_globals.onepix, (real)P1.y / (real)sp_globals.onepix,
    (real)P2.x / (real)sp_globals.onepix, (real)P2.y / (real)sp_globals.onepix,
    (real)P3.x / (real)sp_globals.onepix, (real)P3.y / (real)sp_globals.onepix);
#endif


Pmid.x = (X0 + (X1 + X2) * 3 + X3 + 4) >> 3;
Pmid.y = (Y0 + (Y1 + Y2) * 3 + Y3 + 4) >> 3;
if ((--depth) <= 0)
    {
    fn_line(Pmid);
    sp_globals.processor.speedo.P0 = Pmid;
    fn_line(P3);
    sp_globals.processor.speedo.P0 = P3;
    }
else
    {
    Pctrl1.x = (X0 + X1 + 1) >> 1;
    Pctrl1.y = (Y0 + Y1 + 1) >> 1;
    Pctrl2.x = (X0 + (X1 << 1) + X2 + 2) >> 2;
    Pctrl2.y = (Y0 + (Y1 << 1) + Y2 + 2) >> 2;
    sp_split_curve(PARAMS2 Pctrl1, Pctrl2, Pmid, depth);
    Pctrl1.x = (X1 + (X2 << 1) + X3 + 2) >> 2;
    Pctrl1.y = (Y1 + (Y2 << 1) + Y3 + 2) >> 2;
    Pctrl2.x = (X2 + X3 + 1) >> 1;
    Pctrl2.y = (Y2 + Y3 + 1) >> 1;
    sp_split_curve(PARAMS2 Pctrl1, Pctrl2, P3, depth);
    }
}


FUNCTION void sp_get_args(SPGLOBPTR2 ufix8 FONTFAR *STACKFAR *pp1,
									 ufix8 FONTFAR *STACKFAR *pp2,
									 ufix8 format, point_t STACKFAR *pP)
/*
 * Called by sp_read_bbox(PARAMS1) and sp_proc_outl_data(PARAMS1) to read an X Y argument
 * pair from the font.
 * The format is specified as follows:
 *     Bits 0-1: Type of X argument.
 *     Bits 2-3: Type of Y argument.
 * where the 4 possible argument types are:
 *     Type 0:   Controlled coordinate represented by one byte
 *               index into the X or Y controlled coordinate table.
 *     Type 1:   Interpolated coordinate represented by a two-byte
 *               signed integer.
 *     Type 2:   Interpolated coordinate represented by a one-byte
 *               signed increment/decrement relative to the 
 *               proceding X or Y coordinate.
 *     Type 3:   Repeat of preceding X or Y argument value and type.
 * The units of P are sub-pixels.
 * Updates ppointers to point to the byte following the
 * argument pair.
 */
{
ufix8   edge;

/* Read X argument */
switch(format & 0x03)
#if INCL_MUTABLE
    {
case 0:                                 /* Index into controlled oru table */
    edge = NEXT_BYTE(*pp1);             /* Read index to oru value */
    sp_globals.processor.speedo.raw_x_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
    sp_globals.processor.speedo.x_orus = sp_plaid.orus[edge]; /* Save adjusted oru value */
#if INCL_RULES
    sp_globals.processor.speedo.x_pix = sp_plaid.spix[edge]; /* Save pixel value */
#endif
    break;

case 1:                                 /* 2 byte interpolated oru value */
    sp_globals.processor.speedo.raw_x_orus = NEXT_WORD(*pp1); /* Save unadjusted oru value */
    goto L1;

case 2:                                 /* 1 byte signed oru increment */
    sp_globals.processor.speedo.raw_x_orus += (fix15)((fix7)NEXT_BYTE(*pp1)); /* Save unadjusted oru value */
L1: 
    sp_globals.processor.speedo.x_orus = sp_globals.processor.speedo.raw_x_orus + sp_get_adjustment(PARAMS2 pp2); /* Save adjusted value */
#if INCL_RULES
#if INCL_AUTOIZONES
	if (sp_globals.FourIn1Flags & BIT1)
		sp_set_x_int(PARAMS1);
#endif /* AUTOIZONES */
    sp_globals.processor.speedo.x_pix = TRANS(
        sp_globals.processor.speedo.x_orus, 
        sp_plaid.mult[sp_globals.processor.speedo.x_int], 
        sp_plaid.offset[sp_globals.processor.speedo.x_int], 
        sp_globals.mpshift);            /* Save transformed pixel value */
#endif
    break;

default:                                /* No change in X value */
    break;
    }
#else                                   /* Non-mutable configuration? */
    {
case 0:                           /* Index to controlled oru */
    edge = NEXT_BYTE(*pp1);
    sp_globals.processor.speedo.x_orus = sp_plaid.orus[edge];
#if INCL_RULES
    sp_globals.processor.speedo.x_pix = sp_plaid.spix[edge];
#endif
    break;

case 1:                           /* 2 byte interpolated oru value */
    sp_globals.processor.speedo.x_orus = NEXT_WORD(*pp1);
    goto L1;

case 2:                           /* 1 byte signed oru increment */
    sp_globals.processor.speedo.x_orus += (fix15)((fix7)NEXT_BYTE(*pp1));
L1: 
#if INCL_RULES
#if INCL_AUTOIZONES
	if (sp_globals.FourIn1Flags & BIT1)
		sp_set_x_int(PARAMS1);
#endif /* AUTOIZONES */
    sp_globals.processor.speedo.x_pix = TRANS(sp_globals.processor.speedo.x_orus, sp_plaid.mult[sp_globals.processor.speedo.x_int], sp_plaid.offset[sp_globals.processor.speedo.x_int], sp_globals.mpshift);
#endif
    break;

default:                          /* No change in X value */
    break;
    }
#endif /* else NOT INCL_MUTABLE */

/* Read Y argument */
switch((format >> 2) & 0x03)
#if INCL_MUTABLE                        /* Mutable configuration? */
    {
case 0:                                 /* Index to controlled oru */
    edge = sp_globals.processor.speedo.Y_edge_org + NEXT_BYTE(*pp1); /* Read index into oru table */
    sp_globals.processor.speedo.raw_y_orus = sp_plaid.raw_orus[edge]; /* Save unadjusted oru value */
    sp_globals.processor.speedo.y_orus = sp_plaid.orus[edge]; /* Save adjusted oru value */
#if INCL_RULES
    sp_globals.processor.speedo.y_pix = sp_plaid.spix[edge]; /* Save transformed pixel value */
#endif
    break;

case 1:                                 /* 2 byte interpolated oru value */
    sp_globals.processor.speedo.raw_y_orus = NEXT_WORD(*pp1); /* Save unadjusted oru value */
    goto L2;

case 2:                                 /* 1 byte signed oru increment */
    sp_globals.processor.speedo.raw_y_orus += (fix15)((fix7)NEXT_BYTE(*pp1)); /* Save unadjusted oru value */

L2: 
    sp_globals.processor.speedo.y_orus = sp_globals.processor.speedo.raw_y_orus + sp_get_adjustment(PARAMS2 pp2); /* Save adjusted oru value */
#if INCL_RULES
#if INCL_AUTOIZONES
	if (sp_globals.FourIn1Flags & BIT1)
		sp_set_y_int(PARAMS1);
#endif /* AUTOIZONES */
    sp_globals.processor.speedo.y_pix = TRANS(
        sp_globals.processor.speedo.y_orus, 
        sp_plaid.mult[sp_globals.processor.speedo.y_int], 
        sp_plaid.offset[sp_globals.processor.speedo.y_int], 
        sp_globals.mpshift);            /* Save transformed pixel value */
#endif
    break;

default:                                /* No change in Y value */
    break;
    }
#else                                   /* Non-mutable configuration? */
    {
case 0:                           /* Index to controlled oru */
    edge = sp_globals.processor.speedo.Y_edge_org + NEXT_BYTE(*pp1);
    sp_globals.processor.speedo.y_orus = sp_plaid.orus[edge];
#if INCL_RULES
    sp_globals.processor.speedo.y_pix = sp_plaid.spix[edge];
#endif
    break;

case 1:                           /* 2 byte interpolated oru value */
    sp_globals.processor.speedo.y_orus = NEXT_WORD(*pp1);
    goto L2;

case 2:                           /* 1 byte signed oru increment */
    sp_globals.processor.speedo.y_orus += (fix15)((fix7)NEXT_BYTE(*pp1));
L2: 
#if INCL_RULES
#if INCL_AUTOIZONES
	if (sp_globals.FourIn1Flags & BIT1)
		sp_set_y_int(PARAMS1);
#endif /* AUTOIZONES */
    sp_globals.processor.speedo.y_pix = TRANS(sp_globals.processor.speedo.y_orus, sp_plaid.mult[sp_globals.processor.speedo.y_int], sp_plaid.offset[sp_globals.processor.speedo.y_int], sp_globals.mpshift);
#endif
    break;

default:                          /* No change in X value */
    break;
    }
#endif /* else NOT INCL_MUTABLE */

#ifdef OLDWAY
#if INCL_RULES
switch(sp_globals.tcb.xmode)
    {
case 0:                           /* X mode 0 */
    pP->x = sp_globals.processor.speedo.x_pix;
    break;

case 1:                           /* X mode 1 */
    pP->x = -sp_globals.processor.speedo.x_pix;
    break;

case 2:                           /* X mode 2 */
    pP->x = sp_globals.processor.speedo.y_pix;
    break;

case 3:                           /* X mode 3 */
    pP->x = -sp_globals.processor.speedo.y_pix;
    break;

default:                          /* X mode 4 */
#endif
    pP->x = (MULT16(sp_globals.processor.speedo.x_orus, sp_globals.tcb.xxmult) + 
             MULT16(sp_globals.processor.speedo.y_orus, sp_globals.tcb.xymult) + 
             sp_globals.tcb.xoffset) >> sp_globals.mpshift;
#if INCL_RULES
    break;
    }

switch(sp_globals.tcb.ymode)
    {
case 0:                           /* Y mode 0 */
    pP->y = sp_globals.processor.speedo.y_pix;
    break;

case 1:                           /* Y mode 1 */
    pP->y = -sp_globals.processor.speedo.y_pix;
    break;

case 2:                           /* Y mode 2 */
    pP->y = sp_globals.processor.speedo.x_pix;
    break;

case 3:                           /* Y mode 3 */
    pP->y = -sp_globals.processor.speedo.x_pix;
    break;

default:                          /* Y mode 4 */
#endif
    pP->y = (MULT16(sp_globals.processor.speedo.x_orus, sp_globals.tcb.yxmult) + 
             MULT16(sp_globals.processor.speedo.y_orus, sp_globals.tcb.yymult) + 
             sp_globals.tcb.yoffset) >> sp_globals.mpshift;
#if INCL_RULES
    break;
    }
#endif
#else /* NEWWAY */
	sp_transform_point(PARAMS2 pP);
#endif /* NEWWAY */

}


FUNCTION LOCAL_PROTO void sp_transform_point(SPGLOBPTR2 point_t STACKFAR *pP)
{
#if INCL_RULES
switch(sp_globals.tcb.xmode)
    {
case 0:                           /* X mode 0 */
    pP->x = sp_globals.processor.speedo.x_pix;
    break;

case 1:                           /* X mode 1 */
    pP->x = -sp_globals.processor.speedo.x_pix;
    break;

case 2:                           /* X mode 2 */
    pP->x = sp_globals.processor.speedo.y_pix;
    break;

case 3:                           /* X mode 3 */
    pP->x = -sp_globals.processor.speedo.y_pix;
    break;

default:                          /* X mode 4 */
#endif
    pP->x = (fix15)((MULT16(sp_globals.processor.speedo.x_orus, sp_globals.tcb.xxmult) + 
             MULT16(sp_globals.processor.speedo.y_orus, sp_globals.tcb.xymult) + 
             sp_globals.tcb.xoffset) >> sp_globals.mpshift);
#if INCL_RULES
    break;
    }

switch(sp_globals.tcb.ymode)
    {
case 0:                           /* Y mode 0 */
    pP->y = sp_globals.processor.speedo.y_pix;
    break;

case 1:                           /* Y mode 1 */
    pP->y = -sp_globals.processor.speedo.y_pix;
    break;

case 2:                           /* Y mode 2 */
    pP->y = sp_globals.processor.speedo.x_pix;
    break;

case 3:                           /* Y mode 3 */
    pP->y = -sp_globals.processor.speedo.x_pix;
    break;

default:                          /* Y mode 4 */
#endif
    pP->y = (fix15)((MULT16(sp_globals.processor.speedo.x_orus, sp_globals.tcb.yxmult) + 
             MULT16(sp_globals.processor.speedo.y_orus, sp_globals.tcb.yymult) + 
             sp_globals.tcb.yoffset) >> sp_globals.mpshift);
#if INCL_RULES
    break;
    }
#endif
}

#if INCL_RULES
#if INCL_AUTOIZONES

FUNCTION LOCAL_PROTO void sp_set_y_int(SPGLOBPTR1)
/*
 *	Walk through the edge list for Y axis and find 
 *	first oru value >= to y_orus. Set y_int from the
 *	index in the oru table.
 *	Always search up or down from current position
 *	resulting from last search for this glyph.
 */
{
/* assume ascending order search: */
fix7 incr = 1, max = sp_globals.processor.speedo.no_X_orus + sp_globals.processor.speedo.no_Y_orus - 1;
register fix15 theOrus = sp_globals.processor.speedo.y_orus;
register fix15 *oruPtr = sp_plaid.orus;
if (theOrus < oruPtr[sp_globals.processor.speedo.sv_yint])
	{/* search in descending order */
	incr = -1;
	max = sp_globals.processor.speedo.no_X_orus;
	}
for (sp_globals.processor.speedo.y_int = sp_globals.processor.speedo.sv_yint;
			sp_globals.processor.speedo.y_int != max;
			sp_globals.processor.speedo.y_int += incr)
	{
	if (incr > 0)
		{/* ascending order? */
       		if (theOrus <= oruPtr[sp_globals.processor.speedo.y_int])
           		break;
		}
	else
		{/* descending order */
       		if (theOrus > oruPtr[sp_globals.processor.speedo.y_int])
           		break;
		}
    }
/* if searching downwards, bump up if not searched down to start of edge list: */
if ((incr < 0) && (theOrus > oruPtr[sp_globals.processor.speedo.y_int]) )
		sp_globals.processor.speedo.y_int++;
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
if (sp_globals.processor.speedo.sv_yint != sp_globals.processor.speedo.y_int)
	record_yint((fix15)(sp_globals.processor.speedo.y_int - sp_globals.processor.speedo.Y_int_org)); /* Record yint data */
#endif
sp_globals.processor.speedo.y_int++;	/* account for extra entry in x mult and offset tables */

/* save result of this search for next start position */
sp_globals.processor.speedo.sv_yint = sp_globals.processor.speedo.y_int - 1;
}

FUNCTION LOCAL_PROTO void sp_set_x_int(SPGLOBPTR1)
/*
 *	Walk through the edge list for X axis and find 
 *	first oru value >= to x_orus. Set x_int from the
 *	index in the oru table.
 *	Always search up or down from current position
 *	resulting from last search for this glyph.
 */
{
/* assume ascending order search: */
fix7 incr = 1, max = sp_globals.processor.speedo.no_X_orus - 1;
register fix15 theOrus = sp_globals.processor.speedo.x_orus;
register fix15 *oruPtr = sp_plaid.orus;
if (sp_globals.processor.speedo.sv_xint && theOrus < oruPtr[sp_globals.processor.speedo.sv_xint])
	{/* search in descending order */
	incr = -1;
	max = 0;
	}
for (sp_globals.processor.speedo.x_int = sp_globals.processor.speedo.sv_xint;
		sp_globals.processor.speedo.x_int != max;
		sp_globals.processor.speedo.x_int+=incr)
   	{
	if (incr > 0)
		{/* ascending order? */
   		if (theOrus <= oruPtr[sp_globals.processor.speedo.x_int])
       			break;
		}
	else
		{/* descending order */
   		if (theOrus > oruPtr[sp_globals.processor.speedo.x_int])
       			break;
		}
   	}
/* if searching downwards, bump up if not searched down to start of edge list: */
if ((incr < 0) && (theOrus > oruPtr[sp_globals.processor.speedo.x_int]) )
	sp_globals.processor.speedo.x_int++;
#if INCL_PLAID_OUT                 /* Plaid data monitoring included? */
if (sp_globals.processor.speedo.sv_xint != sp_globals.processor.speedo.x_int)
	record_xint((fix15)sp_globals.processor.speedo.x_int);         /* Record xint data */
#endif

/* save result of this search for next start position */
sp_globals.processor.speedo.sv_xint = sp_globals.processor.speedo.x_int;
}

#endif	/* INCL_AUTOIZONES */
#endif	/* INCL_RULES */
#endif /* if PROC_SPEEDO */
