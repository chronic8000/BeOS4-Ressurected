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
*     $Header: //bliss/user/archive/rcs/pcl5/hpreader/hp_readr.h,v 7.0 97/03/17 17:42:16 shawn Release $                                                                       *
*                                                                                    *
*     $Log:	hp_readr.h,v $
 * Revision 7.0  97/03/17  17:42:16  shawn
 * TDIS Version 6.00
 * 
 * Revision 6.1  96/07/02  17:04:24  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 6.0  95/02/15  16:21:06  roberte
 * Release
 * 
 * Revision 5.2  95/01/10  08:50:59  roberte
 * Updated copyright header.
 * 
 * Revision 5.1  95/01/04  16:29:31  roberte
 * Release
 * 
 * Revision 4.3  94/12/27  10:39:54  roberte
 * Change all SPGLOBPTR2 macros to PROTO_DECL2.
 * 
 * Revision 4.2  94/12/21  16:12:03  roberte
 * Put back prototype for eo_get_char_addr().
 * 
 * Revision 4.1  94/12/21  15:07:37  shawn
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 4.0  94/05/04  09:24:46  roberte
 * Release
 * 
 * Revision 3.0  94/04/08  11:53:26  roberte
 * Release
 * 
 * Revision 2.93  94/04/01  15:47:38  roberte
 * Inserted PROTOTYPE macro before all function prototypes.
 * 
 * Revision 2.92  94/03/23  10:04:38  roberte
 * Added prototype for new entry point, eo_get_char_width_orus().
 * 
 * Revision 2.91  93/08/30  15:36:22  roberte
 * Release
 * 
 * Revision 2.23  93/03/15  13:08:13  roberte
 * Release
 * 
 * Revision 2.16  93/03/09  13:09:36  roberte
 * Fixed improper termination of rcsid control string.\
 * 
 * Revision 2.15  93/02/03  11:54:38  roberte
 * put the pfont field of eospecs_t back to pointer.
 * 
 * Revision 2.14  93/02/02  16:07:18  roberte
 * Changed eofont_t field pfont from a pointer to a structure.
 * 
 * Revision 2.13  93/01/18  14:32:01  roberte
 * Removed prototype of eo_get_char_addr.
 * 
 * Revision 2.12  93/01/18  14:29:54  roberte
 * Changed type of 1st formal parameter of eo_get_char_addr() prototype to dir_t *.
 * 
 * Revision 2.11  93/01/18  14:15:54  roberte
 * Moved prototypes of static functions to appropriate source files.
 * 
 * Revision 2.10  93/01/18  12:34:33  roberte
 * Corrected use of PROTO_DECL1 macros which were used in conjunction with "void" mistakenly.
 * 
 * Revision 2.9  93/01/15  15:15:48  roberte
 * Added PROTO_DECL1 and PROTO_DECL2 macros to function prototypes as appropriate..
 * 
 * Revision 2.8  93/01/15  10:07:32  roberte
 * "fix" changed to "fix15" throughout.
 * 
 * Revision 2.7  93/01/15  09:44:45  roberte
 * Added function prototypes using PROTO macro of all functs in hpfnt1.c and hpfnt2.c,
 * static functions included and duly noted.
 * 
 * Revision 2.6  93/01/11  12:00:07  roberte
 * renamed types azone_t and stem_snap_t to eo_azone_t and eo_stem_snap_t to avoid conflict with type1.
 * 
 * Revision 2.5  93/01/11  11:26:41  roberte
 * renamed fbbox_t data type to eo_fbbox_t so won't conflict with type1's.
 * 
 * Revision 2.4  93/01/11  09:36:55  roberte
 * Moved MAX_BREAKS, MAX_BLUE_ZONES and MAX_EDGES from hpfnt2.c to here.
 * 
 * Revision 2.3  93/01/11  09:14:21  roberte
 * Added compile time flag hp_readr_h.
 * Moved structures define in hpfnt2.c to here:
 *      fbbox_t
 *      azone_t
 *      stem_snap_t
 *      edge_t
 * 
 * Revision 2.2  92/11/19  15:08:44  roberte
 * Release
 * 
 * Revision 2.1  91/04/04  11:25:20  mark
 * Release
 * 
 * Revision 1.2  90/09/18  16:04:12  mike
 * Added "ctm_t" data type; new error messages.
 * 
 * Revision 1.1  90/07/11  12:44:54  mike
 * Initial revision
 * 
*                                                                                    *
*************************************************************************************/

#ifndef hp_readr_h
#define hp_readr_h

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/pcl5/hpreader/hp_readr.h,v 7.0 97/03/17 17:42:16 shawn Release $";
#endif


#define GET_WORD(A)  ((ufix16)((ufix16)A[0] << 8) | ((ufix8)A[1]))

typedef   unsigned char  eo_char_t;

typedef struct
    {
    fix15  resolution;              /* ORUs per em */
    fix15  unique_id;
    void  *eofont;                  /* ptr to table containing addrs */
    }   eofont_t;                   /* of all characters by index nr */

typedef struct
    {
    eofont_t   *pfont;               /* PCL5 font pointer */
    fix31      xxmult, xymult, xoffset,     /* transformation matrix */
               yxmult, yymult, yoffset;     /* 16.16 format */
    ufix32     flags;               /* scan conversion flags */
    void      *out_info;
    }   eospecs_t;

typedef struct
    {
    fix31      xxmult, xymult, xoffset,     /* transformation matrix */
               yxmult, yymult, yoffset;     /* 16.16 format */
    }   ctm_t;

typedef struct
    {
    ufix16  xmin;                 
    ufix16  ymin;
    ufix16  xmax;
    ufix16  ymax;
    } 
eo_fbbox_t;                        /* Font bounding box data (Design Window Units) */

typedef struct
    {
    fix15   type;               /* Type of alignment zone (top or bottom) */
    fix15   top_orus;           /* Y coord of top extent of alignment zone */
    fix15   bottom_orus;        /* Y coord of bottom extent of alignment zone */
    fix15   pix;                /* Pixel position for aligned coordinates */
    } eo_azone_t;                  /* Entry in alignment zone table */

typedef struct
    {
    fix15   min_orus;           /* Minimum size of controlled stem */
    fix15   max_orus;           /* Maximum size of controlled stem */
    fix15   pix;                /* Pixel size of controlled stem */
    } eo_stem_snap_t;              /* Entry in stem snap table */

typedef struct
    {
    fix15   type;               /* Edge type (left, right, top, bottom, straight, curved) */
    fix15   xpos;               /* X coordinate of edge reference point */
    fix15   ypos;               /* Y coordinate of edge reference point */
    fix15   min;                /* Start coordinate of edge */
    fix15   max;                /* End coordinate of edge */
    }   edge_t;


#define MAX_BLUE_ZONES       12 /* Max number of vertical alignment zones */
#define MAX_EDGES            20 /* Maximum number of edges in hor or ver edge list */
#define MAX_BREAKS           10 /* Max number of transformation breakpoints in any dimension */

#define  EEO_DOES_NOT_EXIST      2001     /* Nonexistent character */
#define  EEO_NESTED_COMP_CHAR    2002     /* Nested compound character */
#define  EEO_ILL_COMP_CHAR_REF   2003     /* Compound character ref's nonexistent character */
#define  EEO_ILL_COMP_SUBCHAR    2004     /* Illegal subcharacter index */
#define  EEO_NOT_A_PCL5_CHAR     2005     /* Illegal PCL5 character descriptor */
#define  EEO_BAD_OUTL_DATA       2006     /* 1) Unparsable data; 2) can't split arc */
#define  EEO_SPECS_OUT_OF_RANGE  2007     /* font specs out of range (eo_set_specs) */
#define  EEO_WIDTH_SPECS_NOTVAL  2008     /* font specs invalid while attempting call to
                                             eo_get_char_width(), eo_get_bitmap_width()
                                             or eo_get_char_bbox()   */


/*------------------ Function Prototypes: -----------------------*/
/* HPFNT1.C */

GLOBAL_PROTO fix31 eo_get_char_width(PROTO_DECL2 ufix16 char_index);
GLOBAL_PROTO fix31 eo_get_char_width_orus(PROTO_DECL2 ufix16 char_index, ufix16 *emResolution);
GLOBAL_PROTO fix15 eo_get_bitmap_width(PROTO_DECL2 ufix16 char_index);
GLOBAL_PROTO btsbool eo_get_char_bbox(PROTO_DECL2 ufix16 char_index, bbox_t *pBBox);
GLOBAL_PROTO btsbool eo_set_specs(PROTO_DECL2 eospecs_t *specs);
GLOBAL_PROTO btsbool eo_make_char(PROTO_DECL2 ufix16 index);

/* HPFNT2.C */
GLOBAL_PROTO btsbool eo_init_trans(PROTO_DECL2 eospecs_t *pspecs);
GLOBAL_PROTO void eo_set_trans(PROTO_DECL2 ufix8 *pChar, fix15 xoff, fix15 yoff);
GLOBAL_PROTO void eo_do_trans(PROTO_DECL2 fix15 x_abs, fix15 y_abs, point_t *pP);

/* application level callback: */
GLOBAL_PROTO char *eo_get_char_addr(PROTO_DECL2 void *eofont, ufix16 index);
#endif /* ifndef hp_readr_h */
