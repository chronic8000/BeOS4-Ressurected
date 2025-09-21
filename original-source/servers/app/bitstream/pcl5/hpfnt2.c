/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/


/****************************** H P F N T 2 . C ******************************
 *                                                                           *
*                                                                                    *
*     $Header: //bliss/user/archive/rcs/pcl5/hpreader/hpfnt2.c,v 7.0 97/03/17 17:42:31 shawn Release $ *
*     $Log:	hpfnt2.c,v $
 * Revision 7.0  97/03/17  17:42:31  shawn
 * TDIS Version 6.00
 * 
 * Revision 6.4  96/07/02  17:06:02  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 6.3  95/11/17  15:53:44  shawn
 * Make following routines explicitly 'static' to avoid conflict
 * set_stem_tables/add_x_constraints/add_y_constraints/update_x_trans/update_y_trans
 * 
 * Revision 6.2  95/11/17  11:14:02  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 6.1  95/05/26  16:31:24  roberte
 * In first_contour(), check Class (byte 3) for anything
 * not equal to 3 (Contour Intellifont Scalable) and reject.
 * Can't get edge lists from compound characters.
 * 
 * Revision 6.0  95/02/15  16:21:40  roberte
 * Release
 * 
 * Revision 5.2  95/01/10  08:51:45  roberte
 * Updated copyright header.
 * 
 * Revision 5.1  95/01/04  16:32:00  roberte
 * Release
 * 
 * Revision 4.4  94/12/21  16:16:49  roberte
 *   Added casts of return of eo_get_char_addr() to (eo_char_t *).
 * 
 * Revision 4.3  94/12/21  15:08:47  shawn
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 4.2  94/07/21  15:50:17  roberte
 * Redid changes after port to PC. Make local prototypes ANSI friendly.
 * 
 * Revision 4.0  94/05/04  09:27:03  roberte
 * Release
 * 
 * Revision 3.0  94/04/08  11:53:54  roberte
 * Release
 * 
 * Revision 2.92  94/02/03  11:53:23  roberte
 * Removed "static" from local function prototypes hated by TurboC compiler.
 * 
 * Revision 2.91  93/08/30  15:36:57  roberte
 * Release
 * 
 * Revision 2.25  93/05/28  09:25:28  roberte
 * Added #if PROC_PCL conditional around entire file, after include of spdo_prv.h
 * 
 * Revision 2.24  93/05/21  11:57:58  roberte
 * Removed some inadvertant nested comments.
 * Corrected all local function mini-prototypes to match any real prototypes in this file.
 * Corrected some prototypes with type for eo_font_hints_t mis-spelled.
 * 
 * Revision 2.23  93/03/15  13:09:06  roberte
 * Release
 * 
 * Revision 2.15  93/02/04  09:03:03  roberte
 * Added RCS control strings.
 * 
*                                                                                    *
* 
 * Generalized Hinting Technology module for HP Type Reader                  *
 * Entries are:                                                              *
 *     eo_init_trans()  -- Initializes intelligent transformation mechanism  *
 *     eo_set_trans()   -- Sets transformation tables for current character  *
 *     eo_do_trans()    -- Executes transformation for one point             *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 28 Jan 91  jsc  Created                                               *
 *                                                                           *
 ****************************************************************************/

#include "spdo_prv.h"               /* General definitions for Speedo    */

#if PROC_PCL
#include "fino.h"
#include "hp_readr.h"


#define   DEBUG   0

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

#define   MAX(a, b) ((a) > (b) ? (a) : (b))
#define   MIN(a, b) ((a) < (b) ? (a) : (b))
#define   ABS(a)    (((a) >  0) ? (a) : -(a))

#define INCL_STDVHW           0 /* Code for stdvw and stdhw setup */
#define INCL_STEMSNAP         0 /* Code for stemsnaps setup */

#define MAX_STEMSNAPH        12 /* Max number of entries in StemSnapH array */
#define MAX_STEMSNAPV        12 /* Max number of entries in StemSnapV array */

/* Arc types in outline data */
#define LINE                  0
#define CRVE                  1

/* Interesting point properties */
#define V_EDGE_LIM         0x01 /* Vertical edge limit */
#define V_EDGE_POS         0x02 /* Vertical edge point */
#define V_EDGE_CURVED      0x04 /* True if vertical edge is curved */
#define H_EDGE_LIM         0x08 /* Horizontal edge limit */
#define H_EDGE_POS         0x10 /* Horizontal edge point */
#define H_EDGE_CURVED      0x20 /* True if horizontal edge is curved */

/* Edge detection state definitions */
#define INIT                  0 /* Looking for first edge limit */
#define START_FOUND           1 /* Looking for edge point */
#define EDGE_FOUND            2 /* Looking for edge limit */
#define IDLE                  3 /* All edges found */

/* Edge properties */
#define LEFT_EDGE          0x01
#define RIGHT_EDGE         0x02
#define BOTTOM_EDGE        0x04
#define TOP_EDGE           0x08
#define CURVED_EDGE        0x10

/* Blue zone properties */
#define BOTTOM_ZONE        0x01
#define TOP_ZONE           0x02

/* Miscellaneous constants */
#define MASTER_RESOLUTION  8782 /* resolution of the following parameters */
#define BASELINE           5380 /* Baseline position in Design Window Coordinates */
#define LEFT_REFERENCE     2980 /* Left sidebearing position in Design Window Coordinates */
#define MIN_V_STR_THICKNESS 100 /* Minimum vertical stroke thickness (DWU units) */
#define MIN_H_STR_THICKNESS 100 /* Minimum horizontal stroke thickness (DWU units) */
#define MIN_X_ORU_GAP       200 /* Minimum DWU separation between X constraints */
#define MIN_X_PIX_GAP         1 /* Minimum pix separation between X constraints */
#define MIN_Y_ORU_GAP       200 /* Minimum DWU separation between Y constraints */
#define MIN_Y_PIX_GAP         1 /* Minimum pix separation between Y constraints */
#define BLUE_SCALE          296 /* Standard BlueScale value (16.16 constant) */
#define BLUE_SHIFT           62 /* Standard BlueShift value (DWU units) */
#define BLUE_FUZZ             5 /* Standard BlueFuzz value (DWU units) */
#define VSTEM_SNAP_THRESH     2 /* Standard StemSnapV threshold (DWU units) */
#define HSTEM_SNAP_THRESH     2 /* Standard StemSnapH threshold (DWU units) */

typedef struct
    {
    fix15   stdhw;              /* Standard horiz stroke width */
    fix15   stdvw;              /* Standard vert stroke width */
    fix15   no_stem_snap_h;     /* Number of horiz stem widths (0 - 12) */
    fix15   stem_snap_h[MAX_STEMSNAPH]; /* Horiz stem widths array */
    fix15   no_stem_snap_v;     /* Number of vert stem widths (0 - 12) */
    fix15   stem_snap_v[MAX_STEMSNAPV]; /* Vert stem widths array */
    btsbool force_bold;         /* Control to force bold at small sizes */
    } 
eo_font_hints_t;                   /* Font-level hint specifications */

typedef struct
    {
    fix15   nContour;           /* Number of contours in current character */
    fix15   ContourIdx;         /* Index of contour currently being read */
    ufix8  *pContourTreeData;   /* Pointer to contour tree data for current contour */
    ufix8  *pXOrg;              /* Pointer to first X coord in current contour */
    ufix8  *pYOrg;              /* Pointer to first Y coord in current contour */
    ufix8  *pAuxXOrg;           /* Pointer to first aux X coord in current contour */
    ufix8  *pAuxYOrg;           /* Pointer to first aux Y coord in current contour */
    fix15   nPoint;             /* Number of points in current contour */
    fix15   nAuxPt;             /* Number of auxiliary points in current contour */
    ufix8   Polarity;           /* Polarity of current contour (1 = Black on left) */
    fix15   nPointsAvail;       /* Number of points already read in current contour */
    ufix8  *pX;                 /* Pointer to next X coord to be read */
    ufix8  *pY;                 /* Pointer to next Y coord to be read */
    ufix8  *pAuxX;              /* Pointer to next aux X coord to be read */
    ufix8  *pAuxY;              /* Pointer to next aux Y coord to be read */
    ufix16  X0;                 /* X coord of previous point */
    ufix16  Y0;                 /* Y coord of previous point */
    ufix8   Type1;              /* Type of segment from prev point (LINE, CRVE) */
    fix15   dX1;                /* Delta X from prev point (aux if curve) to current point */
    fix15   dY1;                /* Delta Y from prev point (aux if curve) to current point */
    ufix16  X1;                 /* X coord of current point */
    ufix16  Y1;                 /* Y coord of current point */
    fix15   AuxX1;              /* X increment for next aux point (if curve) */
    fix15   AuxY1;              /* Y increment for next aux point (if curve) */
    ufix8   Type2;              /* Type of segment to next point (LINE, CRVE) */
    fix15   dX2;                /* Delta X from current point to next point (aux if curve) */
    fix15   dY2;                /* Delta Y from current point to next point (aux if curve) */
    ufix16  X2;                 /* X coord of next point */
    ufix16  Y2;                 /* Y coord of next point */
    fix15   AuxX2;              /* X increment for aux point after (X2, Y2) (if curve) */
    fix15   AuxY2;              /* Y increment for aux point after (X2, Y2) (if curve) */
    ufix8   Type3;              /* Type of segment from next point to following point (LINE, CRVE) */
    ufix8   Props;              /* Interesting properties of current point */
    fix15   nPointsLeft;        /* Number of points before wraparound to start of is required */
    fix15   LoopCount;          /* Number of times current contour has been traversed */
    }   read_state_t;

typedef struct
    {
    fix15   pos1;               /* Coordinate of left or bottom edge of stroke */
    fix15   pos2;               /* Coordinate of right or top edge of stroke */
    fix15   priority;           /* Stroke priority (figure of merit) */
    }   stroke_t;

typedef struct
    {
    fix15   ii;                 /* Index of current blue zone */
    fix15   priority;           /* Priority of current alignement candidate */
    fix15   orus;               /* Y coordinate of current alignment (orus) */
    fix15   pix;                /* Y coordinate of current alignment (pixels) */
#if INCL_PLAID_OUT              /* Plaid data monitoring included? */
    fix15   bottom;             /* hstem bottom for plaid monitoring */
    fix15   top;                /* hstem top for plaid monitoring */
#endif
    }   blue_state_t;


/*** STATIC VARIABLES ***/
#ifdef OLDWAY
static eo_fbbox_t eo_font_bbox;       /* Fontwide bounding box */
static fix31   ctm[6];          /* Current transformation matrix */
static btsbool bogus_mode;      /* Linear transformation requested */
static fix15   x_pix_per_oru;   /* Pixels per oru in X direction */
static fix15   y_pix_per_oru;   /* Pixels per oru in Y direction */
static fix31   x_pos;           /* Pixel offset in X direction */
static fix31   y_pos;           /* Pixel offset in Y direction */
static fix15   x_off;           /* DWU offset in X direction */
static fix15   y_off;           /* DWU offset in Y direction */
static fix15 max_ppo;           /* Max X or Y pix per oru in any transformation zone */

/* Transformation control tables */
static fix15   x_trans_mode;    /* Mode for calculating transformed X */
static fix15   y_trans_mode;    /* Mode for calculating transformed Y */
                                /*   0: Linear                        */
                                /*   1: function of X only            */
                                /*   2: function of -X only           */
                                /*   3: function of Y only            */
                                /*   4: function of -Y only           */
static btsbool non_linear_X;    /* True if X values require non-linear transformation */
static btsbool non_linear_Y;    /* True if Y values require non-linear transformation */
static fix15   no_x_breaks;     /* Number of X transformation breakpoints */
static fix15   no_y_breaks;     /* Number of Y transformation breakpoints */
static fix15   Xorus[MAX_BREAKS]; /* List of X non-linear breakpoints */
static fix15   Yorus[MAX_BREAKS]; /* List of Y non-linear breakpoints */
static fix15   Xpix[MAX_BREAKS]; /* List of X pixel values at breakpoints */
static fix15   Ypix[MAX_BREAKS]; /* List of Y pixel values at breakpoints */
static fix31   Xmult[MAX_BREAKS + 1]; /* List of X multiplication coefficients */
static fix31   Ymult[MAX_BREAKS + 1]; /* List of Y multiplication coefficients */
static fix31   Xoffset[MAX_BREAKS + 1]; /* List of X transfromation constants */
static fix31   Yoffset[MAX_BREAKS + 1]; /* List of Y transformation constants */
static fix15   old_x_priority;  /* Priority of last entry in X breakpoint table */
static fix15   old_y_priority;  /* Priority of last entry in Y breakpoint table */

/*  resolution adjusted copies of standard hint and cell parameters */
fix15 eo_baseline; /* Baseline position in Design Window Coordinates */
fix15 eo_left_reference; /* Left sidebearing position in Design Window Coordinates */
fix15 eo_min_v_str; /* Minimum vertical stroke thickness (DWU units) */
fix15 eo_min_h_str; /* Minimum horizontal stroke thickness (DWU units) */
fix15 eo_min_x_oru_gap; /* Minimum DWU separation between X constraints */
fix15 eo_min_y_oru_gap; /* Minimum DWU separation between Y constraints */
fix15 eo_blue_scale; /* Standard BlueScale value (16.16 constant) */
fix15 eo_blue_shift; /* Standard BlueShift value (DWU units) */
 
/* Vertical alignment control tables */
static eo_azone_t BlueZones[MAX_BLUE_ZONES]; /* Vertical alignment zones */
static fix15   nBlueZones;      /* Number of vertical alignment zones */

/* Stem weight control tables */
static fix15   minhstemweight;  /* Minimum horizontal stem weight */
static fix15   minvstemweight;  /* Minimum vertical stem weight */
#if INCL_STDVHW
static eo_stem_snap_t hstem_std;   /* Standard horizontal stem weight */
static eo_stem_snap_t vstem_std;   /* Standard horizontal stem weight */
#endif
#if INCL_STEMSNAPS
static eo_stem_snap_t hstem_snaps[MAX_STEMSNAPH]; 
                                /* Horizontal stem control table */
static no_hstem_snaps;          /* Number of controlled hstems */
static eo_stem_snap_t vstem_snaps[MAX_STEMSNAPV]; 
                                /* Vertical stem control table */
static no_vstem_snaps;          /* Number of controlled vstems */
#endif

/* Horizontal and vertical edge lists */
static  fix15  nh_edges;        /* Number of horizontal edges */
static  edge_t h_edge_list[MAX_EDGES];
static  fix15  nv_edges;        /* Number of vertical edges */
static  edge_t v_edge_list[MAX_EDGES];

/* Plaid data monitoring mechanism */
#if INCL_PLAID_OUT              /* Plaid data monitoring included? */
static fix15   nvstems;         /* Number of vertical strokes in current character */
static fix15   vstem_left[MAX_EDGES]; /* X coord of left edge of vert stroke */
static fix15   vstem_right[MAX_EDGES]; /* X coord of right edge of vert stroke */
static fix15   nhstems;         /* Number of horizontal strokes in current character */
static fix15   hstem_bottom[MAX_EDGES]; /* Y coord of bottom edge of horiz stroke */
static fix15   hstem_top[MAX_EDGES]; /* Y coord of top edge of horiz stroke */
#endif
#endif /* OLDWAY */

/* Function Prototypes: */

LOCAL_PROTO fix15 eo_setup_mult(SPGLOBPTR2 fix31 input_mult, fix15 resolution);
LOCAL_PROTO fix31 eo_setup_offset(SPGLOBPTR2 fix31 input_offset);
LOCAL_PROTO void eo_set_mode_flags(SPGLOBPTR2 eospecs_t *pspecs);
LOCAL_PROTO void get_font_hints(SPGLOBPTR2 eofont_t *eofont, eo_font_hints_t *pFontHints);
LOCAL_PROTO void get_blue_values(SPGLOBPTR2 eofont_t *eofont);
LOCAL_PROTO btsbool add_blue_zone(SPGLOBPTR2 fix15 type, fix15 y1, fix15 y2);
LOCAL_PROTO void set_blue_zones(SPGLOBPTR2 eo_font_hints_t *pFontHints);
LOCAL_PROTO void make_edge_lists(SPGLOBPTR2 ufix8 *pchar);
LOCAL_PROTO btsbool first_contour(ufix8 *pChar, read_state_t *pReadState);
LOCAL_PROTO btsbool next_contour(ufix8 *pChar, read_state_t *pReadState);
LOCAL_PROTO btsbool read_point(read_state_t *pReadState);
LOCAL_PROTO void add_v_edge(SPGLOBPTR2 fix15 xpos, fix15 ypos, btsbool curved, fix15 start, fix15 end, ufix8 polarity);
LOCAL_PROTO void add_h_edge(SPGLOBPTR2 fix15 xpos, fix15 ypos, btsbool curved, fix15 start, fix15 end, ufix8 polarity);
LOCAL_PROTO void proc_v_edge_list(SPGLOBPTR1);
LOCAL_PROTO void test_v_stroke(SPGLOBPTR2 edge_t *pEdge1, edge_t *pEdge2, stroke_t *pStroke);
LOCAL_PROTO void do_v_stroke(SPGLOBPTR2 stroke_t *pStroke);
LOCAL_PROTO void proc_h_edge_list(SPGLOBPTR1);
LOCAL_PROTO void test_h_stroke(SPGLOBPTR2 edge_t *pEdge1, edge_t *pEdge2, stroke_t *pStroke);
LOCAL_PROTO void do_h_edge(SPGLOBPTR2 blue_state_t *pBlueState, edge_t *pEdge);
LOCAL_PROTO void do_h_stroke(SPGLOBPTR2 blue_state_t *pBlueState, stroke_t *pStroke);
LOCAL_PROTO void init_accum( blue_state_t *pBlueState);
LOCAL_PROTO void flush_accum(SPGLOBPTR2 blue_state_t *pBlueState);

LOCAL_PROTO void set_stem_tables(SPGLOBPTR2 eo_font_hints_t *pFontHints);
LOCAL_PROTO void add_x_constraint(SPGLOBPTR2 fix15 orus, fix15 pix, fix15 priority);
LOCAL_PROTO void add_y_constraint(SPGLOBPTR2 fix15 orus, fix15 pix, fix15 priority);
LOCAL_PROTO void update_x_trans(SPGLOBPTR1);
LOCAL_PROTO void update_y_trans(SPGLOBPTR1);



FUNCTION btsbool eo_init_trans(SPGLOBPTR2 eospecs_t *pspecs)
/*
 * Called to initialize the transformation mechanism for a 
 * new font or transformation matrix
 * Returns TRUE if successful, FALSE otherwise
 */
{
fix31   mult;               /* Successive multiplier values */
ufix32  num;                /* Numerator of largest multiplier value */
ufix32  numcopy;            /* Copy of numerator */
ufix32  denom;              /* Denominator of largest multiplier value */
eo_font_hints_t FontHints;     /* Font hints structure */

sp_globals.processor.pcl.bogus_mode = pspecs->flags & BOGUS_MODE;

/* initialize resolution specific hints */
sp_globals.processor.pcl.eo_baseline = ((fix31)BASELINE * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_left_reference = ((fix31)LEFT_REFERENCE * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_min_v_str = ((fix31)MIN_V_STR_THICKNESS * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_min_h_str = ((fix31)MIN_H_STR_THICKNESS * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_min_x_oru_gap = ((fix31)MIN_X_ORU_GAP * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_min_y_oru_gap = ((fix31)MIN_Y_ORU_GAP * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_blue_scale    = ((fix31)BLUE_SCALE * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;
sp_globals.processor.pcl.eo_blue_shift    = ((fix31)BLUE_SHIFT * (fix31)pspecs->pfont->resolution) / (fix31)MASTER_RESOLUTION;

/* Initialize fontwide bounding box */
sp_globals.processor.pcl.eo_font_bbox.xmin = 785;
sp_globals.processor.pcl.eo_font_bbox.ymin = 2663;
sp_globals.processor.pcl.eo_font_bbox.xmax = 15599;
sp_globals.processor.pcl.eo_font_bbox.ymax = 13718;

#if DEBUG
printf("FontBBox = { %d %d %d %d}\n", 
    (int)sp_globals.processor.pcl.eo_font_bbox.xmin, 
    (int)sp_globals.processor.pcl.eo_font_bbox.ymin, 
    (int)sp_globals.processor.pcl.eo_font_bbox.xmax, 
    (int)sp_globals.processor.pcl.eo_font_bbox.ymax);
#endif

/* Determine numerator and denominator of largest multiplier value */
mult = pspecs->xxmult >> 16;
if (mult < 0)
    mult = -mult;
num = mult;

mult = pspecs->xymult >> 16;
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;

mult = pspecs->yxmult >> 16;
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;

mult = pspecs->yymult >> 16;
if (mult < 0)
    mult = -mult;
if (mult > num)
    num = mult;
num++;                          /* Max absolute pixels per em (rounded up) */
denom = pspecs->pfont->resolution;            /* Orus per em */

/* Set multiplier shift to accomodate largest multiplier value */
sp_globals.multshift = 14;            
numcopy = num;
while (numcopy >= denom)        /* More than 1, 2, 4, ... pix per oru? */
    {
    numcopy >>= 1;
    sp_globals.multshift--;     /* sp_globals.multshift is 13, 12, 11, ... */
    }

sp_globals.multrnd = ((fix31)1 << sp_globals.multshift) >> 1;
sp_globals.mpshift = sp_globals.multshift - sp_globals.pixshift;
if (sp_globals.mpshift < 0)
    {
    return FALSE;
    }
sp_globals.mprnd = ((fix31)1 << sp_globals.mpshift) >> 1;
sp_globals.processor.pcl.max_ppo = 0x7fff;

/* Save linear transformation matrix in internal form */
sp_globals.processor.pcl.ctm[0] = (fix31)eo_setup_mult(PARAMS2 pspecs->xxmult,pspecs->pfont->resolution);
sp_globals.processor.pcl.ctm[1] = (fix31)eo_setup_mult(PARAMS2 pspecs->yxmult,pspecs->pfont->resolution);
sp_globals.processor.pcl.ctm[2] = (fix31)eo_setup_mult(PARAMS2 pspecs->xymult,pspecs->pfont->resolution);
sp_globals.processor.pcl.ctm[3] = (fix31)eo_setup_mult(PARAMS2 pspecs->yymult,pspecs->pfont->resolution);
sp_globals.processor.pcl.ctm[4] = eo_setup_offset(PARAMS2 pspecs->xoffset);
sp_globals.processor.pcl.ctm[5] = eo_setup_offset(PARAMS2 pspecs->yoffset);

#if DEBUG
printf("LocalMatrix = [%7.5f, %7.5f, %7.5f, %7.5f, %7.5f, %7.5f]\n",
    (real)sp_globals.processor.pcl.ctm[0] / (real)(sp_globals.multrnd << 1),
    (real)sp_globals.processor.pcl.ctm[1] / (real)(sp_globals.multrnd << 1),
    (real)sp_globals.processor.pcl.ctm[2] / (real)(sp_globals.multrnd << 1),
    (real)sp_globals.processor.pcl.ctm[3] / (real)(sp_globals.multrnd << 1),
    (real)sp_globals.processor.pcl.ctm[4] / (real)(sp_globals.multrnd << 1),
    (real)sp_globals.processor.pcl.ctm[5] / (real)(sp_globals.multrnd << 1));
#endif

/* Set transformation mode flags based on linear transformation */
eo_set_mode_flags(PARAMS2 pspecs);

if (sp_globals.processor.pcl.bogus_mode)     /* Linear transformation requested? */
    {
    return TRUE;
    }

/* Generate font-level hints */
get_font_hints(PARAMS2 pspecs->pfont, &FontHints);

/* Set blue zone tables for current transformation */
set_blue_zones(PARAMS2 &FontHints);

/* Set stem snap tables */
set_stem_tables(PARAMS2 &FontHints);
return TRUE;
}


FUNCTION LOCAL_PROTO fix15 eo_setup_mult(SPGLOBPTR2 fix31 input_mult, fix15 resolution)
/*
 * Converts multiplier in transformation
 * matrix from external to internal form.
 */
{
fix15   imshift;       /* Right shift to internal format */
fix31   imdenom;       /* Divisor in internal format */
fix31   imrnd;         /* Rounding for division operation */

imshift = 15 - sp_globals.multshift;
imdenom = (fix31)resolution << imshift;
imrnd = imdenom >> 1;

input_mult >>= 1;
if (input_mult >= 0)
    return (fix15)((input_mult + imrnd) / imdenom);
else
    return -(fix15)((-input_mult + imrnd) / imdenom);
}


FUNCTION LOCAL_PROTO fix31 eo_setup_offset(SPGLOBPTR2 fix31 input_offset)
/*
 * Converts offset in transformation
 * matrix from external to internal form.
 */
{
fix15   imshift;       /* Right shift to internal format */
fix31   imrnd;         /* Rounding for right shift operation */

imshift = 15 - sp_globals.multshift;
imrnd = ((fix31)1 << imshift) >> 1;

return ((input_offset >> 1) + imrnd) >> imshift;
}


FUNCTION LOCAL_PROTO void eo_set_mode_flags(SPGLOBPTR2 eospecs_t *pspecs)
/*
 * Sets up the transformation mode flags
 *     sp_globals.processor.pcl.non_linear_X
 *         FALSE:        X oru values transformed linearly
 *         TRUE:         X oru values transformed non-linearly
 *     sp_globals.processor.pcl.x_pix_per_oru     Conversion factor from X orus to pixels (if non-linear X)
 *     sp_globals.processor.pcl.x_pos             Required pixel offset of converted X orus (if non-linear X)
 *     sp_globals.processor.pcl.x_trans_mode      Method of calculating sp_globals.processor.pcl.Xpix:
 *         0             sp_globals.processor.pcl.Xpix = matrix[0] * sp_globals.processor.pcl.Xorus + matrix[2] * sp_globals.processor.pcl.Yorus + matrix[4]
 *         1             sp_globals.processor.pcl.Xpix = transfomed sp_globals.processor.pcl.Xorus
 *         2             sp_globals.processor.pcl.Xpix = -transfomed sp_globals.processor.pcl.Xorus
 *         3             sp_globals.processor.pcl.Xpix = transfomed sp_globals.processor.pcl.Yorus
 *         4             sp_globals.processor.pcl.Xpix = transfomed sp_globals.processor.pcl.Yorus
 *     sp_globals.processor.pcl.non_linear_Y      
 *         FALSE:        Y oru values transformed linearly
 *         TRUE:         Y oru values transformed non-linearly
 *     sp_globals.processor.pcl.y_pix_per_oru     Conversion factor from Y orus to pixels (if non-linear Y)
 *     sp_globals.processor.pcl.y_pos             Required pixel offset of converted Y orus (if non-linear X)
 *     sp_globals.processor.pcl.y_trans_mode      Method of calculating sp_globals.processor.pcl.Ypix:
 *         0             sp_globals.processor.pcl.Ypix = matrix[1] * sp_globals.processor.pcl.Xorus + matrix[3] * sp_globals.processor.pcl.Yorus + matrix[5]
 *         1             sp_globals.processor.pcl.Ypix = transfomed sp_globals.processor.pcl.Xorus
 *         2             sp_globals.processor.pcl.Ypix = -transfomed sp_globals.processor.pcl.Xorus
 *         3             sp_globals.processor.pcl.Ypix = transfomed sp_globals.processor.pcl.Yorus
 *         4             sp_globals.processor.pcl.Ypix = transfomed sp_globals.processor.pcl.Yorus
 *     sp_globals.normal True if both X and Y are along an axis
 */
{
fix15 xrange;       /* Fontwide range of X oru values */
fix15 yrange;       /* Fontwide range of Y oru values */
fix31 dxdx;         /* Variation of transformed X over X range */
fix31 dydx;         /* Variation of transformed Y over X range */
fix31 dxdy;         /* Variation of transformed X over Y range */
fix31 dydy;         /* Variation of transformed Y over Y range */
fix31 thresh;       /* 0.5 pixels in multshift units */

/* Set defaults for linear transformation */
sp_globals.processor.pcl.non_linear_X = FALSE;
sp_globals.processor.pcl.non_linear_Y = FALSE;
sp_globals.processor.pcl.x_trans_mode = 0;
sp_globals.processor.pcl.y_trans_mode = 0;
sp_globals.processor.pcl.no_x_breaks = 0;
sp_globals.processor.pcl.no_y_breaks = 0;

if (sp_globals.processor.pcl.bogus_mode)          /* Linear scaling requested? */
    goto end;

/* Calculate partial differentials */
xrange = sp_globals.processor.pcl.eo_font_bbox.xmax - sp_globals.processor.pcl.eo_font_bbox.xmin;
yrange = sp_globals.processor.pcl.eo_font_bbox.ymax - sp_globals.processor.pcl.eo_font_bbox.ymin;
dxdx = sp_globals.processor.pcl.ctm[0] * (fix31)xrange;
dydx = sp_globals.processor.pcl.ctm[1] * (fix31)xrange;
dxdy = sp_globals.processor.pcl.ctm[2] * (fix31)yrange;  
dydy = sp_globals.processor.pcl.ctm[3] * (fix31)yrange;

thresh = sp_globals.multrnd; /* 0.5 in multshift units */

if (dxdx > thresh)        /* Transformed X increases over X range? */
    {
    if (dxdy > thresh)    /* Transformed X increases over Y range? */
        goto next;
    if (dxdy < -thresh)   /* Transformed X decreases over Y range? */
        goto next;
    sp_globals.processor.pcl.non_linear_X = TRUE;  /* Transformed X is a function of +X only */
    sp_globals.processor.pcl.x_pix_per_oru = (fix15)sp_globals.processor.pcl.ctm[0];
    sp_globals.processor.pcl.x_pos = sp_globals.processor.pcl.ctm[4];
    sp_globals.processor.pcl.x_trans_mode = 1;
    goto next;
    }

if (dxdx < -thresh)       /* Transformed X decreases over X range? */
    {
    if (dxdy > thresh)    /* Transformed X increases over Y range? */
        goto next;
    if (dxdy < -thresh)   /* Transformed X decreases over Y range? */
        goto next;
    sp_globals.processor.pcl.non_linear_X = TRUE;  /* Transformed X is a function of -X only */
    sp_globals.processor.pcl.x_pix_per_oru = -(fix15)sp_globals.processor.pcl.ctm[0];
    sp_globals.processor.pcl.x_pos = -sp_globals.processor.pcl.ctm[4];
    sp_globals.processor.pcl.x_trans_mode = 2;
    goto next;
    }

if (dxdy > thresh)        /* Transformed X increases over Y range? */
    {
    sp_globals.processor.pcl.non_linear_Y = TRUE;  /* Transformed X is a function of +Y only */
    sp_globals.processor.pcl.y_pix_per_oru = (fix15)sp_globals.processor.pcl.ctm[2];
    sp_globals.processor.pcl.y_pos = sp_globals.processor.pcl.ctm[4];
    sp_globals.processor.pcl.x_trans_mode = 3;
    goto next;
    }

if (dxdy < -thresh)       /* Transformed X decreases over Y range? */
    {
    sp_globals.processor.pcl.non_linear_Y = TRUE;  /* Transformed X is a function of -Y only */
    sp_globals.processor.pcl.y_pix_per_oru = -(fix15)sp_globals.processor.pcl.ctm[2];
    sp_globals.processor.pcl.y_pos = -sp_globals.processor.pcl.ctm[4];
    sp_globals.processor.pcl.x_trans_mode = 4;
    goto next;
    }

next:
if (dydx > thresh)        /* Transformed Y increases over X range? */
    {
    if (dydy > thresh)    /* Transformed Y increases over Y range? */
        goto end;
    if (dydy < -thresh)   /* Transformed Y decreases over Y range? */
        goto end;
    sp_globals.processor.pcl.non_linear_X = TRUE;  /* Transformed Y is a function of +X only */
    sp_globals.processor.pcl.x_pix_per_oru = (fix15)sp_globals.processor.pcl.ctm[1];
    sp_globals.processor.pcl.x_pos = sp_globals.processor.pcl.ctm[5];
    sp_globals.processor.pcl.y_trans_mode = 1;
    goto end;
    }

if (dydx < -thresh)       /* Transformed Y decreases over X range? */
    {
    if (dydy > thresh)    /* Transformed Y increases over Y range? */
        goto end;
    if (dydy < -thresh)   /* Transformed Y decreases over Y range? */
       goto end;
    sp_globals.processor.pcl.non_linear_X = TRUE;  /* Transformed Y is a function of -X only */
    sp_globals.processor.pcl.x_pix_per_oru = -(fix15)sp_globals.processor.pcl.ctm[1];
    sp_globals.processor.pcl.x_pos = -sp_globals.processor.pcl.ctm[5];
    sp_globals.processor.pcl.y_trans_mode = 2;
    goto end;
    }

if (dydy > thresh)        /* Transformed Y increases over Y range? */
    {
    sp_globals.processor.pcl.non_linear_Y = TRUE;  /* Transformed Y is a function of +Y only */
    sp_globals.processor.pcl.y_pix_per_oru = (fix15)sp_globals.processor.pcl.ctm[3];
    sp_globals.processor.pcl.y_pos = sp_globals.processor.pcl.ctm[5];
    sp_globals.processor.pcl.y_trans_mode = 3;
    goto end;
    }

if (dydy < -thresh)        /* Transformed Y decreases over Y range */
    {
    sp_globals.processor.pcl.non_linear_Y = TRUE;   /* Transformed Y is a function of -Y only */
    sp_globals.processor.pcl.y_pix_per_oru = -(fix15)sp_globals.processor.pcl.ctm[3];
    sp_globals.processor.pcl.y_pos = -sp_globals.processor.pcl.ctm[5];
    sp_globals.processor.pcl.y_trans_mode = 4;
    goto end;
    }

end:
    sp_globals.normal = (sp_globals.processor.pcl.x_trans_mode != 0) && (sp_globals.processor.pcl.y_trans_mode != 0);

#if DEBUG
printf("Transformation mode flags:\n");
SHOWB(sp_globals.processor.pcl.non_linear_X);
SHOWI(sp_globals.processor.pcl.x_trans_mode);
SHOWR((real)sp_globals.processor.pcl.x_pix_per_oru / (real)(sp_globals.multrnd << 1));
SHOWR((real)sp_globals.processor.pcl.x_pos / (real)(sp_globals.multrnd << 1));
SHOWB(sp_globals.processor.pcl.non_linear_Y);
SHOWI(sp_globals.processor.pcl.y_trans_mode);
SHOWR((real)sp_globals.processor.pcl.y_pix_per_oru / (real)(sp_globals.multrnd << 1));
SHOWR((real)sp_globals.processor.pcl.y_pos / (real)(sp_globals.multrnd << 1));
#endif
}


FUNCTION LOCAL_PROTO void get_font_hints(SPGLOBPTR2 eofont_t *eofont, eo_font_hints_t *pFontHints)
/*
 * Extracts fontwide hints from the specified font
 */
{

/* Default values for hints */
pFontHints->stdhw = 0;                 /* StdHW */
pFontHints->stdvw = 0;                 /* StdVW */
pFontHints->no_stem_snap_h = 0;        /* Size of StemSnapH array */ 
pFontHints->no_stem_snap_v = 0;        /* Size of StemSnapV array */ 
pFontHints->force_bold = FALSE;        /* ForceBold */

/* Get blue values from font */
get_blue_values(PARAMS2 eofont);               /* Generate blue zone table */
}


FUNCTION LOCAL_PROTO void get_blue_values(SPGLOBPTR2 eofont_t *eofont)
/*
 * Extracts blue values from the specified font
 */
{
eo_char_t  *pchar;
ufix16      char_index;
fix15       type;
fix15       y;
fix15       baseline;
fix15       baseline_overshoot_pos;
fix15       cap_height;
fix15       cap_height_overshoot_pos;
fix15       x_height;
fix15       x_height_overshoot_pos;
fix15       fig_height;
fix15       fig_height_overshoot_pos;
fix15       ii;

sp_globals.processor.pcl.nBlueZones = 0;                         /* Clear vertical alignment table */

baseline = sp_globals.processor.pcl.eo_baseline;

char_index = 79;                        /* Look at Cap O */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access Cap O for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

baseline_overshoot_pos = sp_globals.processor.pcl.nh_edges?      /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[0].ypos:                /* Lowest edge in cap O */
    0;                                  /* Dummy value if no edge found */
SHOWI(baseline_overshoot_pos);

cap_height_overshoot_pos = sp_globals.processor.pcl.nh_edges?    /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost edge in cap O */
    0;                                  /* Dummy value if no edge found */
SHOWI(cap_height_overshoot_pos);

char_index = 72;                        /* Look at Cap H */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access Cap H for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

cap_height = sp_globals.processor.pcl.nh_edges?                  /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in cap H */
    0;                                  /* Dummy value if no edge found */
SHOWI(cap_height);

char_index = 120;                       /* Look at lowercase x */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access lowercase x for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

x_height = sp_globals.processor.pcl.nh_edges?                    /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in LC x */
    0;                                  /* Dummy value if no edge found */

char_index = 118;                       /* Look at lowercase v */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access lowercase v for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

y = sp_globals.processor.pcl.nh_edges?                           /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in LC v */
    0;                                  /* Dummy value if no edge found */
if (y < x_height)                       /* Use lower of x- and v-heights */
    x_height = y;
SHOWI(x_height);

char_index = 111;                       /* Look at lowercase o */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access lowercase o for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

x_height_overshoot_pos = sp_globals.processor.pcl.nh_edges?      /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in o */
    0;                                  /* Dummy value if no edge found */
SHOWI(x_height_overshoot_pos);

char_index = 55;                        /* Look at figure 7 */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access figure 7 for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

fig_height = sp_globals.processor.pcl.nh_edges?                  /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in figure 7 */
    0;                                  /* Dummy value if no edge found */
SHOWI(fig_height);

char_index = 48;                        /* Look at figure 0 */
pchar = (eo_char_t *)eo_get_char_addr(PARAMS2 eofont->eofont, char_index);
if (pchar != BTSNULL)                      /* Character available? */
    {
    make_edge_lists(PARAMS2 pchar);             /* List all edges in character */
    }
else                                    /* Character not available? */
    {
#if DEBUG
    printf("*** Cannot access figure 7 for BlueValue generation\n");
#endif
    sp_globals.processor.pcl.nh_edges = 0;
    }

fig_height_overshoot_pos = sp_globals.processor.pcl.nh_edges?    /* Valid edge list? */
    sp_globals.processor.pcl.h_edge_list[sp_globals.processor.pcl.nh_edges - 1].ypos:     /* Topmost horizontal edge in fig 0 */
    0;                                  /* Dummy value if no edge found */
SHOWI(fig_height_overshoot_pos);

type = BOTTOM_ZONE;
if (add_blue_zone(PARAMS2 type, baseline_overshoot_pos, baseline))
    {
    type = TOP_ZONE;
    add_blue_zone(PARAMS2 type, cap_height, cap_height_overshoot_pos);
    add_blue_zone(PARAMS2 type, x_height, x_height_overshoot_pos);
    add_blue_zone(PARAMS2 type, fig_height, fig_height_overshoot_pos);
    }

#if DEBUG
if (sp_globals.processor.pcl.nBlueZones)
    {
    printf("\nBlueValues:\n");
    for (ii = 0; ii < sp_globals.processor.pcl.nBlueZones; ii++)
        {
        printf("%s %6d %6d\n", 
            (sp_globals.processor.pcl.BlueZones[ii].type & BOTTOM_ZONE)? "Bottom Zone: ": "   Top Zone: ",
            (int)sp_globals.processor.pcl.BlueZones[ii].bottom_orus, 
            (int)sp_globals.processor.pcl.BlueZones[ii].top_orus);
        }
    }
else
    {
    printf("\nNo BlueValues:\n");
    }
#endif
}


FUNCTION LOCAL_PROTO btsbool add_blue_zone(SPGLOBPTR2 fix15 type, fix15 y1, fix15 y2)
/*
 * Verifies the the specified blue zone is valid
 * Compares the specified blue zone from y1 to y2
 * with the currently established blue zones.
 * Returns FALSE if:-
 *    the new blue zone is not valid, or 
 *    if it overlaps an existing zone, or
 *    the vertical alignment table is already full.
 * Adds the new blue zone and returns TRUE if the zone is valid
 * and no overlap or interference occurs.
 */
{
fix15   ii, jj;
fix15   min_gap;

if (sp_globals.processor.pcl.nBlueZones >= MAX_BLUE_ZONES)       /* Vertical alignment table full? */
    {
#if DEBUG
    printf("*** add_blue_zone: Blue zone table overflow\n");
#endif
    return FALSE;
    }

if (y1 >= y2)                           /* Negative blue zone size? */
    return FALSE;

if (((fix31)(y2 - y1) * (fix31)sp_globals.processor.pcl.eo_blue_scale) >= 65536L) /* Size incompatible with BlueScale? */
    return FALSE;

min_gap = (fix15)BLUE_FUZZ << 1;        /* Minimum gap between blue zones */

for (ii = 0; ii < sp_globals.processor.pcl.nBlueZones; ii++)
    {
    if ((y2 + min_gap) < sp_globals.processor.pcl.BlueZones[ii].bottom_orus) /* Safely below existing blue zone? */
        {
        for (jj = sp_globals.processor.pcl.nBlueZones; jj > ii; jj--)
            {
            sp_globals.processor.pcl.BlueZones[jj] = sp_globals.processor.pcl.BlueZones[jj - 1];
            }
        break;
        }
    if ((y1 - min_gap) <= sp_globals.processor.pcl.BlueZones[ii].top_orus) /* Not safely above existing blue zone? */
        {
        return FALSE;
        }
    }
sp_globals.processor.pcl.BlueZones[ii].type = type;
sp_globals.processor.pcl.BlueZones[ii].bottom_orus = y1;
sp_globals.processor.pcl.BlueZones[ii].top_orus = y2;
sp_globals.processor.pcl.nBlueZones++;
return TRUE;
}


FUNCTION LOCAL_PROTO void set_blue_zones(SPGLOBPTR2 eo_font_hints_t *pFontHints)
/* 
 * Sets up data in sp_globals.processor.pcl.BlueZones[] to control vertical alignment
 */
{
fix15   ii, jj;
btsbool suppr_oversht;   /* True if overshoot supression active */
fix15   orus_per_half_pix;
fix15   thresh;

if (!sp_globals.processor.pcl.non_linear_Y)       /* Hints not active in character Y dimension? */
    return;

suppr_oversht = (sp_globals.processor.pcl.y_pix_per_oru < ((fix15)sp_globals.processor.pcl.eo_blue_scale >> (16 - sp_globals.multshift)));

orus_per_half_pix = (sp_globals.multrnd + (sp_globals.processor.pcl.y_pix_per_oru >> 1)) / sp_globals.processor.pcl.y_pix_per_oru;
thresh = (orus_per_half_pix > sp_globals.processor.pcl.eo_blue_shift)?
        sp_globals.processor.pcl.eo_blue_shift:
        orus_per_half_pix;

for (ii = 0; ii < sp_globals.processor.pcl.nBlueZones; ii++)
    {
    if (sp_globals.processor.pcl.BlueZones[ii].type & BOTTOM_ZONE) /* Bottom alignment zone? */
        {
        sp_globals.processor.pcl.BlueZones[ii].pix = 
            ((((fix31)(sp_globals.processor.pcl.BlueZones[ii].top_orus - sp_globals.processor.pcl.eo_baseline) * 
               (fix31)sp_globals.processor.pcl.y_pix_per_oru) + 
              sp_globals.processor.pcl.y_pos + 
              sp_globals.multrnd) >> 
             sp_globals.mpshift) &
            sp_globals.pixfix;          /* Round zone top to nearest pixel boundary */
        if (suppr_oversht)              /* Overshoot supression active? */
            {
            sp_globals.processor.pcl.BlueZones[ii].bottom_orus -= BLUE_FUZZ;
            sp_globals.processor.pcl.BlueZones[ii].top_orus += BLUE_FUZZ;
            }
        else                            /* Overshoot supression off? */
            {          
            sp_globals.processor.pcl.BlueZones[ii].bottom_orus = sp_globals.processor.pcl.BlueZones[ii].top_orus - thresh - BLUE_FUZZ;
            sp_globals.processor.pcl.BlueZones[ii].top_orus += BLUE_FUZZ;
            }
        }
    else                                /* Top alignment zone? */
        {
        sp_globals.processor.pcl.BlueZones[ii].pix = 
            ((((fix31)(sp_globals.processor.pcl.BlueZones[ii].bottom_orus - sp_globals.processor.pcl.eo_baseline) * 
               sp_globals.processor.pcl.y_pix_per_oru) + 
              sp_globals.processor.pcl.y_pos + 
              sp_globals.multrnd) >> 
             sp_globals.mpshift) &
            sp_globals.pixfix;          /* Round zone bottom to nearest pixel boundary */
        if (suppr_oversht)              /* Overshoot supression active? */
            {
            sp_globals.processor.pcl.BlueZones[ii].top_orus += BLUE_FUZZ;
            sp_globals.processor.pcl.BlueZones[ii].bottom_orus -= BLUE_FUZZ;
            }
        else                            /* Overshoot supression off? */
            {
            sp_globals.processor.pcl.BlueZones[ii].top_orus = sp_globals.processor.pcl.BlueZones[ii].bottom_orus + thresh + BLUE_FUZZ;
            sp_globals.processor.pcl.BlueZones[ii].bottom_orus -= BLUE_FUZZ;
            }
        }
    }

#if DEBUG
if (sp_globals.processor.pcl.nBlueZones)
    printf("\nProcessed blue value data:\n");
for (ii = 0; ii < sp_globals.processor.pcl.nBlueZones; ii++)
    {
    printf("%s %6d %6d %8.3f\n", 
        (sp_globals.processor.pcl.BlueZones[ii].type & BOTTOM_ZONE)? "Bottom Zone: ": "   Top Zone: ",
        (int)sp_globals.processor.pcl.BlueZones[ii].bottom_orus, 
        (int)sp_globals.processor.pcl.BlueZones[ii].top_orus, 
        (real)sp_globals.processor.pcl.BlueZones[ii].pix / (real)sp_globals.onepix);
    }
#endif
}


FUNCTION static void set_stem_tables(SPGLOBPTR2 eo_font_hints_t *pFontHints)
/* 
 * Sets up data tables to control stem weight calculation
 */
{
fix15  ii;
fix15  cmshift;      /* Shift from 16.16 constant to multshift */
fix15  cmrnd;        /* Round from 16.16 constant to multshift */
fix15  stdvw_pix;    /* Standard vert stem wieght in pixshift units */
fix15  stdhw_pix;    /* Standard horiz stem wieght in pixshift units */
fix31  thresh1;      /* Pixel threshold in multshift units */
fix31  thresh2;      /* Pixel threshold in multshift units */
fix15  thresh;       /* Stem snap threshold in DWUs */

/* Set minimum stem weights */
if (pFontHints->force_bold)
    {
    sp_globals.processor.pcl.minvstemweight = sp_globals.onepix << 1;
    sp_globals.processor.pcl.minhstemweight = sp_globals.onepix << 1;
    }
else
    {
    sp_globals.processor.pcl.minvstemweight = sp_globals.onepix;
    sp_globals.processor.pcl.minhstemweight = sp_globals.onepix;
    }

#if INCL_STDVHW
/* Use stdVW for stdHW if latter is not specified */
if (pFontHints->stdhw == 0)
    {
    pFontHints->stdhw = pFontHints->stdvw;
    }

/* Set up standard vertical stem weight control data */
cmshift = 16 - sp_globals.multshift;
cmrnd = ((fix15)1 << cmshift) >> 1;
if (sp_globals.processor.pcl.non_linear_X)                       /* Hints active in character X dimension? */
    {
    stdvw_pix = (((fix31)pFontHints->stdvw * (fix31)sp_globals.processor.pcl.x_pix_per_oru) +
                 sp_globals.mprnd)
                >> sp_globals.mpshift;  /* Standard vstem in pixels (unrounded) */
    sp_globals.processor.pcl.vstem_std.pix = 
        (stdvw_pix + sp_globals.pixrnd) & sp_globals.pixfix;   /* Standard vstem in pixels (rounded) */
    if (sp_globals.processor.pcl.vstem_std.pix < sp_globals.processor.pcl.minvstemweight)
        sp_globals.processor.pcl.vstem_std.pix = sp_globals.processor.pcl.minvstemweight;
    thresh1 = 
        ((fix31)stdvw_pix << sp_globals.mpshift) - 
        ((5161 + cmrnd) >> cmshift);    /* stdvw_pix - 0.36 pixels in multshift units */
    thresh2 = 
        ((fix31)sp_globals.processor.pcl.vstem_std.pix << sp_globals.mpshift) - 
        ((47514 + cmrnd) >> cmshift);   /* sp_globals.processor.pcl.vstem_std.pix - .725 pixels in multshift units */
    sp_globals.processor.pcl.vstem_std.min_orus = 
        (((thresh1 > thresh2)? thresh1: thresh2) +
         (sp_globals.processor.pcl.x_pix_per_oru >> 1)) / 
        sp_globals.processor.pcl.x_pix_per_oru;                  /* Min stroke weight in orus to snap to standard vstem */
    thresh1 = 
        ((fix31)stdvw_pix << sp_globals.mpshift) + 
        ((39321 + cmrnd) >> cmshift);   /* stdvw + 0.6 pixels in multshift units */
    thresh2 = 
        ((fix31)sp_globals.processor.pcl.vstem_std.pix << sp_globals.mpshift) + 
        ((57344 + cmrnd) >> cmshift);   /* sp_globals.processor.pcl.vstem_std.pix + 0.875 pixels in multshift units */
    sp_globals.processor.pcl.vstem_std.max_orus = 
        (((thresh1 < thresh2)? thresh1: thresh2) +
         (sp_globals.processor.pcl.x_pix_per_oru >> 1)) / 
        sp_globals.processor.pcl.x_pix_per_oru;
    }

/* Set up standard horizontal stem weight control data */
if (sp_globals.processor.pcl.non_linear_Y)    /* Hints active in character Y dimension? */
    {
    stdhw_pix = (((fix31)pFontHints->stdhw * (fix31)sp_globals.processor.pcl.y_pix_per_oru) +
                 sp_globals.mprnd)
                >> sp_globals.mpshift; /* Standard hstem in pixels (unrounded) */
    sp_globals.processor.pcl.hstem_std.pix = 
        (stdhw_pix + sp_globals.pixrnd) & sp_globals.pixfix;   /* Standard hstem in pixels (rounded) */
    if (sp_globals.processor.pcl.hstem_std.pix < sp_globals.processor.pcl.minhstemweight)
        sp_globals.processor.pcl.hstem_std.pix = sp_globals.processor.pcl.minhstemweight;
    thresh1 = 
        ((fix31)stdhw_pix << sp_globals.mpshift) - 
        ((5161 + cmrnd) >> cmshift);    /* stdhw_pix - 0.36 pixels in multshift units */
    thresh2 = 
        ((fix31)sp_globals.processor.pcl.hstem_std.pix << sp_globals.mpshift) - 
        ((47514 + cmrnd) >> cmshift);   /* sp_globals.processor.pcl.hstem_std.pix - .725 pixels in multshift units */
    sp_globals.processor.pcl.hstem_std.min_orus = 
        (((thresh1 > thresh2)? thresh1: thresh2) +
         (sp_globals.processor.pcl.y_pix_per_oru >> 1)) / 
        sp_globals.processor.pcl.y_pix_per_oru;                  /* Min stroke weight in orus to snap to standard hstem */
    thresh1 = 
        ((fix31)stdhw_pix << sp_globals.mpshift) + 
        ((39321 + cmrnd) >> cmshift);   /* stdhw + 0.6 pixels in multshift units */
    thresh2 = 
        ((fix31)sp_globals.processor.pcl.hstem_std.pix << sp_globals.mpshift) + 
        ((57344 + cmrnd) >> cmshift);   /* sp_globals.processor.pcl.hstem_std.pix + 0.875 pixels in multshift units */
    sp_globals.processor.pcl.hstem_std.max_orus = 
        (((thresh1 < thresh2)? thresh1: thresh2) +
         (sp_globals.processor.pcl.y_pix_per_oru >> 1)) / 
        sp_globals.processor.pcl.y_pix_per_oru;
    }
#endif

#if INCL_STEMSNAPS
/* Set up vertical stem snap table */
if (sp_globals.processor.pcl.non_linear_X)    /* Hints active in character X dimension? */
    {
    thresh = VSTEM_SNAP_THRESH;
    no_sp_globals.processor.pcl.vstem_snaps = pFontHints->no_stem_snap_v;
    for (ii = 0; ii < no_sp_globals.processor.pcl.vstem_snaps; ii++)
        {
        sp_globals.processor.pcl.vstem_snaps[ii].min_orus = pFontHints->stem_snap_v[ii] - thresh;
        sp_globals.processor.pcl.vstem_snaps[ii].max_orus = pFontHints->stem_snap_v[ii] + thresh;
        sp_globals.processor.pcl.vstem_snaps[ii].pix = 
            ((fix31)pFontHints->stem_snap_v[ii] * 
             sp_globals.processor.pcl.x_pix_per_oru + 
             sp_globals.mprnd) >>
            sp_globals.mpshift;
        if (sp_globals.processor.pcl.vstem_snaps[ii].pix < sp_globals.processor.pcl.minvstemweight)
            sp_globals.processor.pcl.vstem_snaps[ii].pix = sp_globals.processor.pcl.minvstemweight;
        }
    }

/* Set up horizontal stem snap table */
if (sp_globals.processor.pcl.non_linear_Y)    /* Hints active in character Y dimension? */
    {
    thresh = HSTEM_SNAP_THRESH;
    no_sp_globals.processor.pcl.hstem_snaps = pFontHints->no_stem_snap_h;
    for (ii = 0; ii < no_sp_globals.processor.pcl.hstem_snaps; ii++)
        {
        sp_globals.processor.pcl.hstem_snaps[ii].min_orus = pFontHints->stem_snap_h[ii] - thresh;
        sp_globals.processor.pcl.hstem_snaps[ii].max_orus = pFontHints->stem_snap_h[ii] + thresh;
        sp_globals.processor.pcl.hstem_snaps[ii].pix = 
            ((fix31)pFontHints->stem_snap_h[ii] * 
             sp_globals.processor.pcl.y_pix_per_oru + 
             sp_globals.mprnd) >>
            sp_globals.mpshift;
        if (sp_globals.processor.pcl.hstem_snaps[ii].pix < sp_globals.processor.pcl.minhstemweight)
            sp_globals.processor.pcl.hstem_snaps[ii].pix = sp_globals.processor.pcl.minhstemweight;
        }
    }
#endif

#if DEBUG
#if INCL_STDVHW
printf("\nStandard vertical stem control:\n");
printf("%6d %6d %8.3f\n", 
    (int)sp_globals.processor.pcl.vstem_std.min_orus, 
    (int)sp_globals.processor.pcl.vstem_std.max_orus, 
    (real)sp_globals.processor.pcl.vstem_std.pix / (real)sp_globals.onepix);

printf("\nStandard horizontal stem control\n");
printf("%6d %6d %8.3f\n", 
    (int)sp_globals.processor.pcl.hstem_std.min_orus, 
    (int)sp_globals.processor.pcl.hstem_std.max_orus, 
    (real)sp_globals.processor.pcl.hstem_std.pix / (real)sp_globals.onepix);
#endif

#if INCL_STEMSNAPS
if (no_sp_globals.processor.pcl.vstem_snaps != 0)
    {
    printf("\nVertical stem snap table:\n");
    for (ii = 0; ii < no_sp_globals.processor.pcl.vstem_snaps; ii++)
        {
        printf("%6d %6d %8.3f\n", 
            (int)sp_globals.processor.pcl.vstem_snaps[ii].min_orus, 
            (int)sp_globals.processor.pcl.vstem_snaps[ii].max_orus, 
            (real)sp_globals.processor.pcl.vstem_snaps[ii].pix / (real)sp_globals.onepix);
        }
    }

if (no_sp_globals.processor.pcl.hstem_snaps != 0)
    {
    printf("\nHorizontal stem snap table:\n");
    for (ii = 0; ii < no_sp_globals.processor.pcl.hstem_snaps; ii++)
        {
        printf("%6d %6d %8.3f\n", 
            (int)sp_globals.processor.pcl.hstem_snaps[ii].min_orus, 
            (int)sp_globals.processor.pcl.hstem_snaps[ii].max_orus, 
            (real)sp_globals.processor.pcl.hstem_snaps[ii].pix / (real)sp_globals.onepix);
        }
    }
#endif
#endif
}


FUNCTION void eo_set_trans(SPGLOBPTR2 ufix8 *pChar, fix15 xoff, fix15 yoff)
/*
 * Sets up the transformation control tables for the specified character.
 */
{

#if DEBUG
void show_x_trans(PARAMS1);        /* Print piecewise-linear X transformation coefficients */
void show_y_trans(PARAMS1);        /* Print piecewise-linear Y transformation coefficients */
#endif

sp_globals.processor.pcl.x_off = xoff;               /* Save as static variable */
sp_globals.processor.pcl.y_off = yoff;               /* Save as static variable */

#if INCL_PLAID_OUT          /* Plaid data monitoring included? */
sp_globals.processor.pcl.nvstems = 0;                /* Clear vertical stem list */
sp_globals.processor.pcl.nhstems = 0;                /* Clear horizontal stem list */
#endif

if ((pChar != BTSNULL) &&      /* character supplied? */
    (!sp_globals.processor.pcl.bogus_mode))           /* nonLinear transformation requested? */
	{
	make_edge_lists(PARAMS2 pChar);     /* Build horizontal and vertical edge lists */

	sp_globals.processor.pcl.no_x_breaks = 0;            /* Initialize X transformation table */
	sp_globals.processor.pcl.no_y_breaks = 0;            /* Initialize Y transformation table */

	proc_v_edge_list(PARAMS1);         /* Process vertical edge list */
	proc_h_edge_list(PARAMS1);         /* Process horizontal edge list */
	}

update_x_trans(PARAMS1);           /* Calculate transformation coeffs for each X zone */
update_y_trans(PARAMS1);           /* Calculate transformation coeffs for each Y zone */

#if DEBUG
show_x_trans(PARAMS1);             /* Print X transformation table */
show_y_trans(PARAMS1);             /* Print Y transformation table */
#endif

#if INCL_PLAID_OUT          /* Plaid data monitoring included? */
show_plaid();               /* Output generated plaid data */
#endif
}


FUNCTION LOCAL_PROTO void make_edge_lists(SPGLOBPTR2 ufix8 *pchar)
/*
 * Scans the outline data of the current character
 * Creates lists of horizontal and vertical edges
 * found in the data 
 */
{
read_state_t ReadState;     /* Point input state */

fix15   vstate;             /* Scanner state for v edge detection */
fix15   vorg;               /* Index of first potential v edge start */
fix15   vstart;             /* Start coordinate for v edge */
fix15   vxpos;              /* X coord of v edge reference point */
fix15   vypos;              /* Y coord of v edge reference point */
btsbool vcurved;            /* True if v edge curved */
fix15   vend;               /* End coordinate for v edge */

fix15   hstate;             /* Scanner state for h edge detection */
fix15   horg;               /* Index of first potential h edge start */
fix15   hstart;             /* Start coordinate for h edge */
fix15   hxpos;              /* X coord of h edge reference point */
fix15   hypos;              /* Y coord of h edge reference point */
btsbool hcurved;            /* True if h edge curved */
fix15   hend;               /* End coordinate for h edge */


sp_globals.processor.pcl.nv_edges = 0;
sp_globals.processor.pcl.nh_edges = 0;

if (!first_contour(pchar, &ReadState))
    return;

do
    {
#if DEBUG
    show_contour_points(&ReadState);
#endif
    vstate = hstate = INIT;
    vorg = horg = -1;
    do
        {
        if (!read_point(&ReadState))    /* Read next interesting point */
            break;                      /* Quit if no interesting point found */

        switch(vstate)
            {
        case INIT:                      /* Looking for start of first vertical edge */
            if (ReadState.Props & V_EDGE_LIM) /* Vertical edge limit? */
                {
                vorg = ReadState.nPointsLeft;
                vstart = ReadState.Y1;
                vstate = START_FOUND;
                }
            break;

        case START_FOUND:               /* Looking for vertical edge point */
            if (ReadState.Props & V_EDGE_POS) /* Vertical edge point? */
                {
                vxpos = ReadState.X1;
                vypos = ReadState.Y1;
                vcurved = (ReadState.Props & V_EDGE_CURVED) != 0;
                vend = ReadState.Y1;
                if (ReadState.Props & V_EDGE_LIM) /* Vertical edge limit? */
                    {
                    add_v_edge(PARAMS2 vxpos, vypos, vcurved, vstart, vend, ReadState.Polarity);
                    vstart = vend;
                    }
                else
                    {
                    vstate = EDGE_FOUND;
                    }
                }
            else if (ReadState.Props & V_EDGE_LIM) /* Vertical edge limit? */
                {
                vstart = ReadState.Y1;
                }

            if (ReadState.nPointsLeft == vorg)
                {
                vstate = IDLE;
                }
            break;

        case EDGE_FOUND:                /* Looking for end of vertical edge */
            if ((ReadState.Props & V_EDGE_POS) && /* Another vertical edge point? */
                (ReadState.X1 != vxpos)) /* At a different position? */
                {
                if (ReadState.Type1 == LINE) /* Vertical line? */
                    {
                    vend = (vend + ReadState.Y0 + 1) >> 1;
                    }
                else
                    {
                    vend = (vend + ReadState.Y1 + 1) >> 1;
                    }
                add_v_edge(PARAMS2 vxpos, vypos, vcurved, vstart, vend, ReadState.Polarity);
                vstart = vend;
                vxpos = ReadState.X1;
                vypos = ReadState.Y1;
                vcurved = (ReadState.Props & V_EDGE_CURVED) != 0;
                vend = ReadState.Y1;
                if (ReadState.Props & V_EDGE_LIM) /* Vertical edge limit? */
                    {
                    add_v_edge(PARAMS2 vxpos, vypos, vcurved, vstart, vend, ReadState.Polarity);
                    vstart = vend;
                    vstate = START_FOUND;
                    }
                }
            else if (ReadState.Props & V_EDGE_LIM) /* Vertical edge limit? */
                {
                vend = ReadState.Y1;
                add_v_edge(PARAMS2 vxpos, vypos, vcurved, vstart, vend, ReadState.Polarity);
                vstart = vend;
                vstate = START_FOUND;
                }

            if (ReadState.nPointsLeft == vorg)
                {
                vstate = IDLE;
                }
            break;

        case IDLE:                      /* All vertical edges found */
            break;
            }

        switch(hstate)
            {
        case INIT:                      /* Looking for start of first horizontal edge */
            if (ReadState.Props & H_EDGE_LIM) /* Horizontal edge limit? */
                {
                horg = ReadState.nPointsLeft;
                hstart = ReadState.X1;
                hstate = START_FOUND;
                }
            break;

        case START_FOUND:               /* Looking for horizontal edge point */
            if (ReadState.Props & H_EDGE_POS) /* Horizontal edge point? */
                {
                hxpos = ReadState.X1;
                hypos = ReadState.Y1;
                hcurved = (ReadState.Props & H_EDGE_CURVED) != 0;
                hend = ReadState.X1;
                if (ReadState.Props & H_EDGE_LIM) /* Horizontal edge limit? */
                    {
                    add_h_edge(PARAMS2 hxpos, hypos, hcurved, hstart, hend, ReadState.Polarity);
                    hstart = hend;
                    }
                else
                    {
                    hstate = EDGE_FOUND;
                    }
                }
            else if (ReadState.Props & H_EDGE_LIM) /* Horizontal edge limit? */
                {
                hstart = ReadState.X1;
                }

           if (ReadState.nPointsLeft == horg)
                {
                hstate = IDLE;
                }
            break;

        case EDGE_FOUND:                /* Looking for end of horizontal edge */
            if ((ReadState.Props & H_EDGE_POS) && /* Another horizontal edge point? */
                (ReadState.Y1 != hypos)) /* At a different position? */
                {
                if (ReadState.Type1 == LINE) /* Horizontal line? */
                    {
                    hend = (hend + ReadState.X0 + 1) >> 1;
                    }
                else
                    {
                    hend = (hend + ReadState.X1 + 1) >> 1;
                    }
                add_h_edge(PARAMS2 hxpos, hypos, hcurved, hstart, hend, ReadState.Polarity);
                hstart = hend;
                hxpos = ReadState.X1;
                hypos = ReadState.Y1;
                hcurved = (ReadState.Props & H_EDGE_CURVED) != 0;
                hend = ReadState.X1;
                if (ReadState.Props & H_EDGE_LIM) /* Horizontal edge limit? */
                    {
                    add_h_edge(PARAMS2 hxpos, hypos, hcurved, hstart, hend, ReadState.Polarity);
                    hstart = hend;
                    hstate = START_FOUND;
                    }
                }
            else if (ReadState.Props & H_EDGE_LIM) /* Horizontal edge limit? */
                {
                hend = ReadState.X1;
                add_h_edge(PARAMS2 hxpos, hypos, hcurved, hstart, hend, ReadState.Polarity);
                hstart = hend;
                hstate = START_FOUND;
                }

           if (ReadState.nPointsLeft == horg)
                {
                hstate = IDLE;
                }
            break;

        case IDLE:                      /* All horizontal edges found */
            break;
            }

        if (ReadState.LoopCount > 2)    /* Timeout after 2 complete circuits of contour */
            break;
        } 
    while ((vstate != IDLE) || (hstate != IDLE));
    }
while (next_contour(pchar, &ReadState));

#if DEBUG
show_edge_lists();
#endif
}


FUNCTION LOCAL_PROTO btsbool first_contour(ufix8 *pChar, read_state_t *pReadState)
/*
 * Selects the first contour in the specified character
 * Sets the read state for reading points from the contour
 * Returns FALSE if there is no contour data or if the contour
 * data is not in the standard format (contourFlag = 4)
 */
{
ufix8  *pointer;
ufix16  ContourTreeOffset;
ufix16  XYCoordDataOffset;
ufix8  *pCharOutlineData;

pointer = pChar + 3;
if (*pointer != 3)
	return FALSE;	/* reject anything but class 3, Intellifont scalable non-compound */
pointer = pChar + 10;
ContourTreeOffset = GET_WORD(pointer);
if (ContourTreeOffset == 0xffff)
    return FALSE;

pCharOutlineData = pChar + 4 + ContourTreeOffset;
pointer = pCharOutlineData;
pReadState->nContour = GET_WORD(pointer);
if (pReadState->nContour == 0)          /* No contour data? */
    return FALSE;

pointer += 2;
if (GET_WORD(pointer) == 0x0003)        /* Non-standard data format? */
    return FALSE;

pReadState->ContourIdx = 0;
pReadState->pContourTreeData = pCharOutlineData + 4;
pointer = pReadState->pContourTreeData;
XYCoordDataOffset = GET_WORD(pointer);
pReadState->Polarity = pReadState->pContourTreeData[3];

pointer = pChar + 4 + XYCoordDataOffset;
pReadState->nPoint = GET_WORD(pointer);
pointer += 2;
pReadState->nAuxPt = GET_WORD(pointer);
pReadState->pXOrg = pointer + 2;
pReadState->pX = pReadState->pXOrg;
pReadState->pYOrg = pReadState->pXOrg + (pReadState->nPoint << 1);
pReadState->pY = pReadState->pYOrg;
pReadState->pAuxXOrg = pReadState->pYOrg + (pReadState->nPoint << 1);
pReadState->pAuxX = pReadState->pAuxXOrg;
pReadState->pAuxYOrg = pReadState->pAuxXOrg + pReadState->nAuxPt;
pReadState->pAuxY = pReadState->pAuxYOrg;

pReadState->nPointsLeft = pReadState->nPoint;
pReadState->nPointsAvail = 0;
pReadState->Type3 = LINE;
pReadState->Type2 = LINE;
pReadState->LoopCount = 0;
return TRUE;
}


FUNCTION LOCAL_PROTO btsbool next_contour(ufix8 *pChar, read_state_t *pReadState)
/*
 * Sets the read state for reading points from the beginning
 * of the next contour in the character
 * Returns FALSE if there in no further contour 
 */

{
ufix8  *pointer;
ufix16  XYCoordDataOffset;

if (++pReadState->ContourIdx >= pReadState->nContour)
    return FALSE;

pReadState->pContourTreeData += 8;
pointer = pReadState->pContourTreeData;
XYCoordDataOffset = GET_WORD(pointer);
pReadState->Polarity = pReadState->pContourTreeData[3];

pointer = pChar + 4 + XYCoordDataOffset;
pReadState->nPoint = GET_WORD(pointer);
pointer += 2;
pReadState->nAuxPt = GET_WORD(pointer);
pReadState->pXOrg = pointer + 2;
pReadState->pX = pReadState->pXOrg;
pReadState->pYOrg = pReadState->pXOrg + (pReadState->nPoint << 1);
pReadState->pY = pReadState->pYOrg;
pReadState->pAuxXOrg = pReadState->pYOrg + (pReadState->nPoint << 1);
pReadState->pAuxX = pReadState->pAuxXOrg;
pReadState->pAuxYOrg = pReadState->pAuxXOrg + pReadState->nAuxPt;
pReadState->pAuxY = pReadState->pAuxYOrg;

pReadState->nPointsLeft = pReadState->nPoint;
pReadState->nPointsAvail = 0;
pReadState->Type3 = LINE;
pReadState->Type2 = LINE;
pReadState->LoopCount = 0;
return TRUE;
}


FUNCTION LOCAL_PROTO btsbool read_point(read_state_t *pReadState)
/*
 * Reads the next interesting point from the currently selected contour
 * Returns TRUE if interesting point found.
 * Returns FALSE if:
 *    All points in the contour are coincident
 *    There are no interesting points found in the contour
 */
{
fix15   ii, jj;
fix15   nn;                     /* Limit on search for interesting point */
fix15   dx1_left;               /* X component of left corner threshold vector */
fix15   dy1_left;               /* Y component of left corner threshold vector */
fix15   dx1_right;              /* X component of right corner threshold vector */
fix15   dy1_right;              /* Y component of right corner threshold vector */
ufix32  adx;                    /* Absolute change in X prior to current point */
ufix32  ady;                    /* Absolute change in Y prior to current point */

nn = pReadState->nPoint << 1;           /* Limit on number of uninteresting points */
for (ii = 0; ii < nn; ii++)             /* Search for interesting point */
    {
    /* Shift previous points by one position */
    if (pReadState->Type2 == LINE)      /* Line from (X1, Y1) to (X2, Y2)? */   
        {
        pReadState->dX1 = pReadState->dX2;
        pReadState->dY1 = pReadState->dY2;
        }
    else                                /* Curve from (X1, Y1) to (X2, Y2)? */   
        {
        pReadState->dX1 = ((pReadState->X2 - pReadState->X1) << 1) - pReadState->dX2;
        pReadState->dY1 = ((pReadState->Y2 - pReadState->Y1) << 1) - pReadState->dY2;
        }

    pReadState->X0 = pReadState->X1;
    pReadState->Y0 = pReadState->Y1;
    pReadState->X1 = pReadState->X2;
    pReadState->Y1 = pReadState->Y2;
    pReadState->AuxX1 = pReadState->AuxX2;
    pReadState->AuxY1 = pReadState->AuxY2;
    pReadState->Type1 = pReadState->Type2;
    pReadState->Type2 = pReadState->Type3;

    /* Search for next non-duplicate point */
    for (jj = 0; jj < nn; jj++)
        {
        /* Read next point (X2, Y2) */
        pReadState->X2 = GET_WORD(pReadState->pX);
        pReadState->pX += 2;
        pReadState->Type3 = (pReadState->X2 & 0x4000)? LINE: CRVE;
        pReadState->X2 &= 0x3fff;
        pReadState->Y2 = GET_WORD(pReadState->pY) & 0x3fff;
        pReadState->pY += 2;
        if (pReadState->Type3 == CRVE)   /* Curve from (X2, Y2) to next point? */
            {
            pReadState->AuxX2 = (fix15)((fix7)*pReadState->pAuxX);
            pReadState->pAuxX++;
            pReadState->AuxY2 = (fix15)((fix7)*pReadState->pAuxY);
            pReadState->pAuxY++;
            }

        /* Calculate vector arriving at point (X2, Y2) */
        if (pReadState->Type2 == LINE)  /* Line from (X1, Y1) to (X2, Y2)? */
            {
            pReadState->dX2 = pReadState->X2 - pReadState->X1;
            pReadState->dY2 = pReadState->Y2 - pReadState->Y1;
            }
        else                            /* Curve from (X1, Y1) to (X2, Y2)? */
            {
            pReadState->dX2 = 
                (pReadState->AuxX1 << 1) - pReadState->X1 + pReadState->X2;
            pReadState->dY2 = 
                (pReadState->AuxY1 << 1) - pReadState->Y1 + pReadState->Y2;
            }

        /* Wraparound at end of contour */
        if (--pReadState->nPointsLeft <= 0)  /* End of contour? */
            {
            pReadState->pX = pReadState->pXOrg;
            pReadState->pY = pReadState->pYOrg;
            pReadState->pAuxX = pReadState->pAuxXOrg;
            pReadState->pAuxY = pReadState->pAuxYOrg;
            pReadState->nPointsLeft = pReadState->nPoint;
            pReadState->LoopCount++;
            }

        /* Check if (X2, Y2) is distinct from (X1, Y1) */
        if ((pReadState->dX2 != 0) ||
            (pReadState->dY2 != 0) ||
            (pReadState->nPointsAvail == 0))
            goto point_found;

        /* Replace (X1, Y1) with (X2, Y2) */
        pReadState->Type2 = pReadState->Type3;
        pReadState->X1 = pReadState->X2;
        pReadState->Y1 = pReadState->Y2;
        pReadState->AuxX1 = pReadState->AuxX2;
        pReadState->AuxY1 = pReadState->AuxY2;
        } 
    return FALSE;                       /* No distinct point found in contour */

point_found:
    /* Test if the point pipeline is full */
    if (pReadState->nPointsAvail < 2)
        {
        pReadState->nPointsAvail++;
        continue;
        }

    /* Interesting point analysis */
    pReadState->Props = 0;              /* Clear interesting point properties */

    if (pReadState->Type1 == LINE)      /* Line to current point? */
        {
        if (pReadState->Type2 == LINE)  /* Line from current point? */
            {
            /* Set 30 degree corner thresh */
            dx1_left = pReadState->dX1 - (pReadState->dY1 >> 1);
            dy1_left = pReadState->dY1 + (pReadState->dX1 >> 1);
            dx1_right = pReadState->dX1 + (pReadState->dY1 >> 1);
            dy1_right = pReadState->dY1 - (pReadState->dX1 >> 1);
            }
        else                            /* Curve from current point? */
            {
            /* Set 45 degree corner thresh */
            dx1_left = pReadState->dX1 - pReadState->dY1;
            dy1_left = pReadState->dY1 + pReadState->dX1;
            dx1_right = dy1_left;
            dy1_right = -dx1_left;
            }
        adx = (ufix32)(ABS(pReadState->dX1));
        ady = (ufix32)(ABS(pReadState->dY1));
        if ((adx << 5) < ady)           /* Within 2 degrees of vertical? */
            {
            pReadState->Props |= V_EDGE_POS + H_EDGE_LIM;
            }
        else if ((ady << 6) < adx)      /* Within 1 degree of horizontal? */
            {
            pReadState->Props |= H_EDGE_POS + V_EDGE_LIM;
            }
        }
    else                                /* Curve to current point? */
        {
        /* Set 45 degree corner thresh */
        dx1_left = pReadState->dX1 - pReadState->dY1;
        dy1_left = pReadState->dY1 + pReadState->dX1;
        dx1_right = dy1_left;
        dy1_right = -dx1_left;
        }

    /* Test for left corner */
    if ((fix31)dx1_left * (fix31)pReadState->dY2 > 
        (fix31)dy1_left * (fix31)pReadState->dX2)
        {
        pReadState->Props |= V_EDGE_LIM + H_EDGE_LIM;
        return TRUE;
        }

    /* Test for right corner */
    if ((fix31)dx1_right * (fix31)pReadState->dY2 < 
        (fix31)dy1_right * (fix31)pReadState->dX2)
        {
        pReadState->Props |= V_EDGE_LIM + H_EDGE_LIM;
        return TRUE;
        }

    if (pReadState->Props)              /* Horizontal or vertical line? */
        return TRUE;

    /* Test for minimum X extreme */
    if ((pReadState->X1 < pReadState->X0) && (pReadState->X1 < pReadState->X2))
        {
        pReadState->Props |= V_EDGE_POS + H_EDGE_LIM + V_EDGE_CURVED;
        return TRUE;
        }

    /* Test for maximum X extreme */
    if ((pReadState->X1 > pReadState->X0) && (pReadState->X1 > pReadState->X2))
        {
        pReadState->Props |= V_EDGE_POS + H_EDGE_LIM + V_EDGE_CURVED;
        return TRUE;
        }

    /* Test for minimum Y extreme */
    if ((pReadState->Y1 < pReadState->Y0) && (pReadState->Y1 < pReadState->Y2))
        {
        pReadState->Props |= H_EDGE_POS + V_EDGE_LIM + H_EDGE_CURVED;
        return TRUE;
        }

    /* Test for maximum Y extreme */
    if ((pReadState->Y1 > pReadState->Y0) && (pReadState->Y1 > pReadState->Y2))
        {
        pReadState->Props |= H_EDGE_POS + V_EDGE_LIM + H_EDGE_CURVED;
        return TRUE;
        }
    }
return FALSE;                           /* Search limit exhausted */
}


FUNCTION LOCAL_PROTO void add_v_edge(SPGLOBPTR2 fix15 xpos, fix15 ypos, btsbool curved, fix15 start, fix15 end, ufix8 polarity)
/*
 * Adds specified edge to vertical edge list.
 */
{
fix15   ii, jj;
fix15   type;
fix15   min;
fix15   max;

#if DEBUG
printf("add_v_edge(PARAMS2 %d, %d, %d, %d, %d, %d)\n", 
    (int)xpos, (int)ypos, (int)curved, (int)start, (int)end, (int)polarity);
#endif

if (start > end)                        /* Edge goes downwards? */
    {
    type = polarity? LEFT_EDGE: RIGHT_EDGE;
    min = end;
    max = start;
    }
else                                    /* Edge goes upwards? */
    {
    type = polarity? RIGHT_EDGE: LEFT_EDGE;
    min = start;
    max = end;
    }

if (curved)                             /* Edge is curved extreme? */
    type |= CURVED_EDGE;

if (sp_globals.processor.pcl.nv_edges >= MAX_EDGES)              /* Vertical edge list already full? */
    {
#if DEBUG
    printf("*** Vertical edge list overflow\n");
#endif
    return;
    }

for (ii = 0; ii < sp_globals.processor.pcl.nv_edges; ii++)       /* For each edge in list... */
    {
    if (sp_globals.processor.pcl.v_edge_list[ii].xpos >= xpos)   /* Time to insert new edge? */
        {
        if ((sp_globals.processor.pcl.v_edge_list[ii].xpos == xpos) &&
            (sp_globals.processor.pcl.v_edge_list[ii].type & LEFT_EDGE) &&
            (type & RIGHT_EDGE))
            continue;                   /* Insert left before right when positions equal */
        for (jj = sp_globals.processor.pcl.nv_edges; jj > ii; jj--)
            {
            sp_globals.processor.pcl.v_edge_list[jj] = sp_globals.processor.pcl.v_edge_list[jj - 1];
            }
        break;
        }
    }
sp_globals.processor.pcl.v_edge_list[ii].type = type;
sp_globals.processor.pcl.v_edge_list[ii].xpos = xpos;
sp_globals.processor.pcl.v_edge_list[ii].ypos = ypos;
sp_globals.processor.pcl.v_edge_list[ii].min = min;
sp_globals.processor.pcl.v_edge_list[ii].max = max;
sp_globals.processor.pcl.nv_edges++;
}


FUNCTION LOCAL_PROTO void add_h_edge(SPGLOBPTR2 fix15 xpos, fix15 ypos, btsbool curved, fix15 start, fix15 end, ufix8 polarity)
/*
 * Adds specified edge to vertical edge list.
 */
{
fix15   ii, jj;
fix15   type;
fix15   min;
fix15   max;

#if DEBUG
printf("add_h_edge(PARAMS2 %d, %d, %d, %d, %d, %d)\n", 
    (int)xpos, (int)ypos, (int)curved, (int)start, (int)end, (int)polarity);
#endif

if (start > end)                        /* Edge goes from right to left? */
    {
    type = polarity? TOP_EDGE: BOTTOM_EDGE;
    min = end;
    max = start;
    }
else                                    /* Edge goes from left to right? */
    {
    type = polarity? BOTTOM_EDGE: TOP_EDGE;
    min = start;
    max = end;
    }

if (curved)                             /* Edge is curved extreme? */
    type |= CURVED_EDGE;

if (sp_globals.processor.pcl.nh_edges >= MAX_EDGES)              /* Edge list full? */
    {
#if DEBUG
    printf("*** Horizontal edge list overflow\n");
#endif
    return;
    }

for (ii = 0; ii < sp_globals.processor.pcl.nh_edges; ii++)       /* For each edge in list... */
    {
    if (sp_globals.processor.pcl.h_edge_list[ii].ypos >= ypos)     /* Time to insert new data? */
        {
        if ((sp_globals.processor.pcl.h_edge_list[ii].ypos == ypos) &&
            (sp_globals.processor.pcl.h_edge_list[ii].type & BOTTOM_EDGE) &&
            (type & TOP_EDGE))
            continue;                   /* Insert bottom before top edges when positions equal */
        for (jj = sp_globals.processor.pcl.nh_edges; jj > ii; jj--)
            {
            sp_globals.processor.pcl.h_edge_list[jj] = sp_globals.processor.pcl.h_edge_list[jj - 1];
            }
        break;
        }
    }
sp_globals.processor.pcl.h_edge_list[ii].type = type;
sp_globals.processor.pcl.h_edge_list[ii].xpos = xpos;
sp_globals.processor.pcl.h_edge_list[ii].ypos = ypos;
sp_globals.processor.pcl.h_edge_list[ii].min = min;
sp_globals.processor.pcl.h_edge_list[ii].max = max;
sp_globals.processor.pcl.nh_edges++;
}


FUNCTION LOCAL_PROTO void proc_v_edge_list(SPGLOBPTR1)
/*
 * Processes the sorted vertical edge list.
 * Subdivides the edge list into groups of edges.
 * Each group consists of one or more bottom edges 
 * followed by one or more top edges.
 * Finds the best stroke from each group.
 * If the priority of the resulting stroke exceeds a
 * threshold, it is executed as an hstem operation.
 * If two consecutive groups fail to yield a stroke,
 * an attempt is made to find the best stroke that
 * spans the two groups.
 */
{
fix15   ii, jj;
fix15   k0;         /* Index of first left edge in current supergroup */
fix15   k1;         /* Index of first rght edge in current supergroup */
fix15   k2;         /* Index of first left edge in current group */
fix15   k3;         /* Index of first right edge in current group */
fix15   k4;         /* Index of last right edge + 1 in current group */
stroke_t stroke;    /* Current candidate stroke */

/* Look for first left edge */
for (k0 = 0; ; k0++)                    /* Scan edge list */
    {
    if (k0 >= sp_globals.processor.pcl.nv_edges)                 /* No left edges in edge list? */
        return;
    if (sp_globals.processor.pcl.v_edge_list[k0].type & LEFT_EDGE) /* Left edge found? */
        break;
    } 

k2 = k0;                                /* First left edge in edge list */
do
    {
    /* Look for next right edge */
    for (k3 = k2 + 1; ; k3++)
        {
        if (k3 >= sp_globals.processor.pcl.nv_edges)             /* No more right edges? */
            return;
        if (sp_globals.processor.pcl.v_edge_list[k3].type & RIGHT_EDGE) /* Right edge found? */
            break;
        } 

    /* Look for next left edge or end of edge list */
    for (k4 = k3 + 1; ; k4++)
        {
        if (k4 >= sp_globals.processor.pcl.nv_edges)             /* End of list? */
            break;
        if (sp_globals.processor.pcl.v_edge_list[k4].type & LEFT_EDGE) /* Left edge found? */
            break;
        } 

    stroke.priority = -1;               /* Flag no stroke found */

    /* Test all left-right combinations in group for best stroke */
    for (ii = k2; ii < k3; ii++)        /* For each left edge in group... */
        {
        for (jj = k3; jj < k4; jj++)    /* For each right edge in group... */
            {
            test_v_stroke(PARAMS2 &sp_globals.processor.pcl.v_edge_list[ii], &sp_globals.processor.pcl.v_edge_list[jj], &stroke);
            }
        }

    if (stroke.priority >= 0)           /* Best stroke found (if any) is valid? */
        {
        do_v_stroke(PARAMS2 &stroke);           /* Execute stroke */
        k0 = k4;                        /* Origin for next group and supergroup */
        }
    else if (k0 != k2)                  /* No valid stroke found in 2 consecutive groups? */
        {
        for (ii = k0; ii < k1; ii++)    /* For each left edge in first group... */
            {
            for (jj = k3; jj < k4; jj++) /* For each right edge in second group... */
                {
                test_v_stroke(PARAMS2 &sp_globals.processor.pcl.v_edge_list[ii], &sp_globals.processor.pcl.v_edge_list[jj], &stroke);
                }
            }
        if (stroke.priority >= 0)       /* Best stroke found (if any) is valid? */
            {
            do_v_stroke(PARAMS2 &stroke);       /* Execute stroke */
            k0 = k4;                    /* Origin for next group and supergroup */
            }
        else                            /* No valid stroke found in supergroup? */
            {
            k0 = k2;
            k1 = k3;
            }
        }
    else                                /* No valid stroke found in group? */
        {
        k0 = k2;
        k1 = k3;
        }
    k2 = k4;
    } while (TRUE);
}


FUNCTION LOCAL_PROTO void test_v_stroke(SPGLOBPTR2 edge_t *pEdge1, edge_t *pEdge2, stroke_t *pStroke)
/*
 * Tests if the specified edges constitute a vertical stroke.
 * If so, and the resulting stroke is better than the current stroke,
 * the current stroke is replaced by the new stroke
 */
{
fix15   stroke_top;     /* Top extent of overlapping edges */
fix15   stroke_bottom;  /* Bottom extent of overlapping edges */
fix15   length;         /* Length of overlap of edges */
fix15   thickness;      /* Distance between edges */
fix15   priority;       /* Computed stroke priority */
fix15   offset;         /* Offset between edge positions */

if ((pEdge1->type & LEFT_EDGE) == 0)    /* First edge is not a left edge? */
    return;

if ((pEdge2->type & RIGHT_EDGE) == 0)   /* Second edge is not a right edge? */
    return;

thickness = pEdge2->xpos - pEdge1->xpos;
if (thickness <= sp_globals.processor.pcl.eo_min_v_str)   /* Stroke thickness below threshold? */
    return;

stroke_bottom = MAX(pEdge1->min, pEdge2->min);
stroke_top = MIN(pEdge1->max, pEdge2->max);
length = stroke_top - stroke_bottom;    /* Length of overlapping edges */

priority = length - (thickness >> 2);   /* Base priority on length and thickness */

if ((pEdge1->type & CURVED_EDGE) &&
    (pEdge2->type & CURVED_EDGE))       /* Pair of curved edges? */
    {
    offset = pEdge1->ypos - pEdge2->ypos;
    offset = ABS(offset);
    priority -= offset;                 /* Reduce priority by offset between edges */
    }

if ((((pEdge1->type & CURVED_EDGE) == 0) &&
     (pEdge2->type & CURVED_EDGE)) || 
    ((pEdge1->type & CURVED_EDGE) &&
     ((pEdge2->type & CURVED_EDGE) == 0)))  /* Straight-curved combination? */
    {
    priority >>= 1;                     /* Reduce priority by factor of two */
    }

if (priority > pStroke->priority)       /* Better than current stroke? */
    {
    pStroke->pos1 = pEdge1->xpos;       /* Update current stroke */
    pStroke->pos2 = pEdge2->xpos;
    pStroke->priority = priority;
    }
}


FUNCTION LOCAL_PROTO void do_v_stroke(SPGLOBPTR2 stroke_t *pStroke)
/*
 * Called by proc_v_edge_list() to execute one stroke.
 * All vertical strokes are positioned as floating strokes.
 */
{
fix15   priority;       /* Priority of accumulated constraint */
fix15   cx;             /* Position of centerline of stroke in outline units */
fix15   strkwt_orus;    /* Thickness of stroke in outline units */
fix15   strkwt_pix;     /* Thickness of stroke in device units */
fix15   cx_pix;         /* Position of centerline of stroke in device units */

cx = ((pStroke->pos1 + pStroke->pos2 + 1) >> 1) + sp_globals.processor.pcl.x_off - sp_globals.processor.pcl.eo_left_reference;
strkwt_orus = pStroke->pos2 - pStroke->pos1;
strkwt_pix = 
    (((fix31)strkwt_orus * 
      sp_globals.processor.pcl.x_pix_per_oru) + 
     sp_globals.multrnd) >>
    sp_globals.mpshift;                 /* Rounded stroke weight in sub-pixel units */
if (strkwt_pix < sp_globals.processor.pcl.minvstemweight)
    strkwt_pix = sp_globals.processor.pcl.minvstemweight;
if (strkwt_pix & sp_globals.onepix)     /* Odd number of pixels in stroke? */
    {
    cx_pix = 
        (((((fix31)cx * 
            sp_globals.processor.pcl.x_pix_per_oru) + 
           sp_globals.processor.pcl.x_pos) >>
          sp_globals.mpshift) &
         sp_globals.pixfix) +
        sp_globals.pixrnd;              /* Round mid-point to nearest pixel center */
    }
else                                    /* Even number of pixels in stroke? */
    {
    cx_pix = 
        ((((fix31)cx * 
           sp_globals.processor.pcl.x_pix_per_oru) + 
          sp_globals.processor.pcl.x_pos +
          sp_globals.mprnd) >>
         sp_globals.mpshift) &
        sp_globals.pixfix;              /* Round mid-point to nearest pixel boundary */
    }
priority = pStroke->priority;
add_x_constraint(PARAMS2 cx, cx_pix, priority);

#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
sp_globals.processor.pcl.vstem_left[sp_globals.processor.pcl.nvstems] = pStroke->pos1;
sp_globals.processor.pcl.vstem_right[sp_globals.processor.pcl.nvstems] = pStroke->pos2;
sp_globals.processor.pcl.nvstems++;
#endif
}


FUNCTION static void add_x_constraint(SPGLOBPTR2 fix15 orus, fix15 pix, fix15 priority)
/*
 * Adds one constraint point to piecewise-linear transformation for X.
 * Assumes that points are added in increasing order of orus
 * If a pair of points are too close in either orus or pixels, the
 * higher priority prevails
 */
{
fix15   ii;

if (sp_globals.processor.pcl.no_x_breaks >= MAX_BREAKS)          /* Breakpoint table full? */
    {
#if DEBUG
    printf("add_x_constraint: Too many breakpoints\n");
#endif
    return;                             /* Ignore if breakpoint table full */
    }

if (sp_globals.processor.pcl.no_x_breaks)                        /* Breakpoint table not empty? */
    {
    ii = sp_globals.processor.pcl.no_x_breaks - 1;
    if (((orus - sp_globals.processor.pcl.Xorus[ii]) < sp_globals.processor.pcl.eo_min_x_oru_gap) ||
        ((pix - sp_globals.processor.pcl.Xpix[ii]) < ((fix15)MIN_X_PIX_GAP << sp_globals.pixshift)) ||
        ((fix31)(pix - sp_globals.processor.pcl.Xpix[ii]) > (fix31)sp_globals.processor.pcl.max_ppo * (fix31)(orus - sp_globals.processor.pcl.Xorus[ii])))
        {
        if (priority < sp_globals.processor.pcl.old_x_priority)
            return;
        sp_globals.processor.pcl.no_x_breaks--;
        }
    }

sp_globals.processor.pcl.Xorus[sp_globals.processor.pcl.no_x_breaks] = orus;
sp_globals.processor.pcl.Xpix[sp_globals.processor.pcl.no_x_breaks] = pix;
sp_globals.processor.pcl.no_x_breaks++;
sp_globals.processor.pcl.old_x_priority = priority;
}


FUNCTION static void update_x_trans(SPGLOBPTR1)
/*
 * Updates the interpolation coefficient tables used
 * for piecewise linear transformation of X coordinates in character
 */
{
fix15   ii;

if (sp_globals.processor.pcl.no_x_breaks == 0)
    {
    sp_globals.processor.pcl.Xmult[0] = (fix31)sp_globals.processor.pcl.x_pix_per_oru;
    sp_globals.processor.pcl.Xoffset[0] = sp_globals.processor.pcl.x_pos + sp_globals.mprnd;
    }
else
    {
    sp_globals.processor.pcl.Xmult[0] = (fix31)sp_globals.processor.pcl.x_pix_per_oru;
    sp_globals.processor.pcl.Xoffset[0] = 
        ((fix31)sp_globals.processor.pcl.Xpix[0] << sp_globals.mpshift) - 
        ((fix31)sp_globals.processor.pcl.Xorus[0] * (fix31)sp_globals.processor.pcl.x_pix_per_oru) +
        sp_globals.mprnd;

    for (ii = 1; ii < sp_globals.processor.pcl.no_x_breaks; ii++)
        {
        sp_globals.processor.pcl.Xmult[ii] = 
            ((fix31)(sp_globals.processor.pcl.Xpix[ii] - sp_globals.processor.pcl.Xpix[ii - 1]) << sp_globals.mpshift) / 
            (fix31)(sp_globals.processor.pcl.Xorus[ii] - sp_globals.processor.pcl.Xorus[ii - 1]);
        sp_globals.processor.pcl.Xoffset[ii] = 
            ((fix31)sp_globals.processor.pcl.Xpix[ii] << sp_globals.mpshift) - 
            ((fix31)sp_globals.processor.pcl.Xorus[ii] * sp_globals.processor.pcl.Xmult[ii]) +
            sp_globals.mprnd;
        }

    sp_globals.processor.pcl.Xmult[ii] = (fix31)sp_globals.processor.pcl.x_pix_per_oru;
    sp_globals.processor.pcl.Xoffset[ii] = 
        ((fix31)sp_globals.processor.pcl.Xpix[sp_globals.processor.pcl.no_x_breaks - 1] << sp_globals.mpshift) - 
        (fix31)sp_globals.processor.pcl.Xorus[sp_globals.processor.pcl.no_x_breaks - 1] * (fix31)sp_globals.processor.pcl.x_pix_per_oru +
        sp_globals.mprnd;
    }
}



FUNCTION LOCAL_PROTO void proc_h_edge_list(SPGLOBPTR1)
/*
 * Processes the sorted horizontal edge list.
 * Finds the best stroke from each group of
 * one or more bottom edges followed by one or more
 * top edges.
 * If the priority of the resulting stroke exceeds a
 * threshold, it is executed as an hstem operation.
 * If two consecutive groups fail to yield a stroke,
 * an attempt is made to find the best stroke that
 * spans the two groups.
 * Unused edges outside the range of strokes generated
 * are also evaluated as possible vertical alignment edges.
 */
 {
fix15   ii, jj;
fix15   k0;             /* Index of first bottom edge in current supergroup */
fix15   k1;             /* Index of first top edge in current supergroup */
fix15   k2;             /* Index of first bottom edge in current group */
fix15   k3;             /* Index of first top edge in current group */
fix15   k4;             /* Index of last top edge + 1 in current group */
stroke_t stroke;        /* Current candidate stroke */
blue_state_t BlueState; /* Vertical alignment accumulator state */


/* Initialize vertical alignment alignment accumulator */
init_accum(&BlueState);

/* Look for first bottom edge */
for (k0 = 0; ; k0++)                    /* Scan edge list */
    {
    if (k0 >= sp_globals.processor.pcl.nh_edges)                 /* No bottom edges in edge list? */
        return;
    if (sp_globals.processor.pcl.h_edge_list[k0].type & BOTTOM_EDGE) /* Bottom edge found? */
        break;
    } 

for (ii = 0; ii < k0; ii++)
    {
    do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Execute any top edges before first bottom edge */
    }

k2 = k0;                                /* First bottom edge */
do
    {
    /* Look for next top edge */
    for (k3 = k2 + 1; ; k3++)
        {
        if (k3 >= sp_globals.processor.pcl.nh_edges)             /* No more top edges? */
            {
            for (ii = k0; ii < sp_globals.processor.pcl.nh_edges; ii++)
                {
                do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Execute remaining free edges */
                }
            flush_accum(PARAMS2 &BlueState);  /* Flush vert alignment accumulator */
            return;
            }
        if (sp_globals.processor.pcl.h_edge_list[k3].type & TOP_EDGE)  /* Top edge found? */
            break;
        } 

    /* Look for next bottom edge or end of edge list */
    for (k4 = k3 + 1; ; k4++)
        {
        if (k4 >= sp_globals.processor.pcl.nh_edges)             /* End of edge list? */
            break;
        if (sp_globals.processor.pcl.h_edge_list[k4].type & BOTTOM_EDGE) /* Bottom edge found? */
            break;
        } 

    stroke.priority = -1;               /* Flag no stroke found */

    /* Test all bottom-top combinations in group for best stroke */
    for (ii = k2; ii < k3; ii++)        /* For each bottom edge in group... */
        {
        for (jj = k3; jj < k4; jj++)    /* For each top edge in group... */
            {
            test_h_stroke(PARAMS2 &sp_globals.processor.pcl.h_edge_list[ii], &sp_globals.processor.pcl.h_edge_list[jj], &stroke);
            }
        }

    if (stroke.priority >= 0)           /* Best stroke found (if any) is valid? */
        {
        for (ii = k0; sp_globals.processor.pcl.h_edge_list[ii].ypos < stroke.pos1; ii++)
            {
            do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Execute free edges below stroke */
            }
        do_h_stroke(PARAMS2 &BlueState, &stroke); /* Execute stroke */
        for (; ii < k4; ii++)
            {
            if (sp_globals.processor.pcl.h_edge_list[ii].ypos > stroke.pos2)
                do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Exec free edges above stroke */
            }
        k0 = k4;                        /* Origin for next group and supergroup */
        }
    else if (k0 != k2)                  /* No valid stroke found in 2 consecutive groups? */
        {
        for (ii = k0; ii < k1; ii++)    /* For each bottom edge in first group... */
            {
            for (jj = k3; jj < k4; jj++) /* For each top edge in second group... */
                {
                test_h_stroke(PARAMS2 &sp_globals.processor.pcl.h_edge_list[ii], &sp_globals.processor.pcl.h_edge_list[jj], &stroke);
                }
            }
        if (stroke.priority >= 0)       /* Best stroke found (if any) is valid? */
            {
            for (ii = k0; sp_globals.processor.pcl.h_edge_list[ii].ypos < stroke.pos1; ii++)
                {
                do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Execute bottom edges below stroke */
                }
            do_h_stroke(PARAMS2 &BlueState, &stroke); /* Execute stroke */
            for (; ii < k4; ii++)
                {
                if (sp_globals.processor.pcl.h_edge_list[ii].ypos > stroke.pos2)
                    do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Exec top edges above stroke */
                }
            k0 = k4;
            }
        else                            /* No valid stroke found in supergroup? */
            {
            for (ii = k0; ii < k2; ii++)
                {
                do_h_edge(PARAMS2 &BlueState, &sp_globals.processor.pcl.h_edge_list[ii]); /* Execute edges in first group */
                }
            k0 = k2;
            k1 = k3;
            }
        }
    else                                /* No valid stroke found in group? */
        {
        k0 = k2;
        k1 = k3;
        }
    k2 = k4;
    } while (TRUE);
}


FUNCTION LOCAL_PROTO void test_h_stroke(SPGLOBPTR2 edge_t *pEdge1, edge_t *pEdge2, stroke_t *pStroke)
/*
 * Tests if the specified edges constitute a horizontal stroke.
 * If so, and the resulting stroke is better than the current stroke,
 * the current stroke is replaced by the new stroke
 */
{
fix15   stroke_left;    /* Left extent of overlapping edges */
fix15   stroke_right;   /* Right extent of overlapping edges */
fix15   length;         /* Length of overlap of edges */
fix15   thickness;      /* Distance between edges */
fix15   priority;       /* Computed stroke priority */
fix15   offset;         /* Offset between edge positions */

if ((pEdge1->type & BOTTOM_EDGE) == 0)  /* First edge not bottom edge? */
    return;

if ((pEdge2->type & TOP_EDGE) == 0)     /* Second edge not top edge? */
    return;

thickness = pEdge2->ypos - pEdge1->ypos;
if (thickness <= sp_globals.processor.pcl.eo_min_h_str)   /* Stroke thickness below threshold? */
    return;

stroke_left = MAX(pEdge1->min, pEdge2->min);
stroke_right = MIN(pEdge1->max, pEdge2->max);
length = stroke_right - stroke_left;    /* Length of overlapping edges */

priority = length - (thickness >> 2);   /* Base priority on length and thickness */

if ((pEdge1->type & CURVED_EDGE) &&
    (pEdge2->type & CURVED_EDGE))       /* Pair of curved edges? */
    {
    offset = pEdge1->xpos - pEdge2->xpos;
    offset = ABS(offset);
    priority -= offset;                 /* Reduce priority by offset between edges */
    }

if ((((pEdge1->type & CURVED_EDGE) == 0) &&
     (pEdge2->type & CURVED_EDGE)) || 
    ((pEdge1->type & CURVED_EDGE) &&
     ((pEdge2->type & CURVED_EDGE) == 0)))  /* Straight-curved combination? */
    {
    priority >>= 1;                     /* Reduce priority by factor of two */
    }

if (priority > pStroke->priority)       /* Better than current stroke? */
    {
    pStroke->pos1 = pEdge1->ypos;       /* Update current stroke */
    pStroke->pos2 = pEdge2->ypos;
    pStroke->priority = priority;
    }
}



FUNCTION LOCAL_PROTO void do_h_edge(SPGLOBPTR2 blue_state_t *pBlueState, edge_t *pEdge)
/*
 * Called by proc_h_edge_list() to execute one edge.
 * Because edges (and embedded strokes) are delivered in ascending 
 * order, it is only necessary to merge the edge sequence with
 * the blue zone sequence which is also in ascending order.
 * If the edge of stroke is in a blue zone, a constraint to
 * position the edge at the zone's pixel position is 
 * accumulated in the constraint accumulator.
 * The constraint accumulator is flushed when the next blue
 * zone is selected
 */
{
fix15   priority;


if ((pBlueState->ii < sp_globals.processor.pcl.nBlueZones) &&    /* Possible blue zone available? */
    (sp_globals.processor.pcl.y_off == 0))                       /* No vertical shift in character? */
    {
    /* Update current blue zone if appropriate */
    while(pEdge->ypos > sp_globals.processor.pcl.BlueZones[pBlueState->ii].top_orus)
        {
        flush_accum(PARAMS2 pBlueState);
        if (++pBlueState->ii >= sp_globals.processor.pcl.nBlueZones)
            return;
        }

    if (pEdge->ypos < sp_globals.processor.pcl.BlueZones[pBlueState->ii].bottom_orus)
        return;

    if (((pEdge->type & BOTTOM_EDGE) &&
         (sp_globals.processor.pcl.BlueZones[pBlueState->ii].type & BOTTOM_ZONE)) ||
        ((pEdge->type & TOP_EDGE) &&
         (sp_globals.processor.pcl.BlueZones[pBlueState->ii].type & TOP_ZONE)))
        {
        priority = pEdge->max - pEdge->min;
        if (pEdge->type & CURVED_EDGE)
            priority >>= 1;
        if (priority > pBlueState->priority)
            {
            pBlueState->priority = priority;
            pBlueState->orus = pEdge->ypos + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;
            pBlueState->pix = sp_globals.processor.pcl.BlueZones[pBlueState->ii].pix;
#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
            pBlueState->bottom = pEdge->ypos;
            pBlueState->top = pEdge->ypos;
#endif
            }
        }
    }
}


FUNCTION LOCAL_PROTO void do_h_stroke(SPGLOBPTR2 blue_state_t *pBlueState, stroke_t *pStroke)
/*
 * Called by proc_h_edge_list() to execute one stroke
 * Because strokes (and embedded edges) are delivered in ascending 
 * order, it is only necessary to merge the stroke sequence with
 * the blue zone sequence which is also in ascending order.
 * If bottom edge of stroke is in bottom zone, a further test is made
 * to see if the top edge is in a top zone. In this special case,
 * the stroke is handled as if it were two independent edges.
 * If the bottom edge of a stroke is in a bottom zone, a constraint is
 * generated to position the stroke in device coordinates so that
 * its bottom edge is aligned with the bottom zone pixel position.
 * If the top edge of a stroke is in a top zone, a constraint is
 * generated to position the stroke in device coordinates so that
 * its top edge is aligned with the top zone pixel position.
 * If neither the bottom edge of the stroke is in a bottom zone
 * nor the top edge of the stroke is in a top zone, a constraint
 * is generated to position the stroke as a floating stroke.
 */
{
fix15   y_ref;        /* Y coord of stroke alignment edge */
fix15   priority;     /* Priority of accumulated constraint */
fix15   cy;           /* Position of centerline of stroke in outline units */
fix15   strkwt_orus;  /* Thickness of stroke in outline units */
fix15   strkwt_pix;   /* Thickness of stroke in device units */
fix15   hstrkwt_pix;  /* Half thickness of stroke in device units */
fix15   cy_pix;       /* Position of centerline of stroke in device units */
fix15   y_pix;        /* Position of top or bottom edge of stroke in device units */
fix15   y_type;       /* Type of constaining blue zone */
btsbool flush_reqd;   /* True if flush required after constaint accumulated */


if ((pBlueState->ii < sp_globals.processor.pcl.nBlueZones) &&    /* Possible blue zone available? */
    (sp_globals.processor.pcl.y_off == 0))                       /* No vertical shift in character? */
    {
    y_ref = (sp_globals.processor.pcl.BlueZones[pBlueState->ii].type & BOTTOM_ZONE)?
        pStroke->pos1:                  /* Test stroke bottom edge for bottom zones */
        pStroke->pos2;                  /* Test stroke top edge for top zones */
    while (y_ref > sp_globals.processor.pcl.BlueZones[pBlueState->ii].top_orus) /* Ref edge is above current blue zone? */
        {
        flush_accum(PARAMS2 pBlueState);        /* Flush any pending constraints */
        if (++pBlueState->ii >= sp_globals.processor.pcl.nBlueZones) /* No more blue zones? */
            goto L1;                    /* The stroke is floating */
        y_ref = (sp_globals.processor.pcl.BlueZones[pBlueState->ii].type & BOTTOM_ZONE)?
            pStroke->pos1:              /* Align bottom of stroke for bottom zones */
            pStroke->pos2;              /* Align top of stroke for top zones */
        }

    if (y_ref < sp_globals.processor.pcl.BlueZones[pBlueState->ii].bottom_orus) /* Ref edge below bottom of current zone? */
        goto L1;                        /* The stroke is floating */

    y_pix = sp_globals.processor.pcl.BlueZones[pBlueState->ii].pix; /* Alignment point for current blue zone */
    y_type = sp_globals.processor.pcl.BlueZones[pBlueState->ii].type; /* Type of current blue zone */ 
    flush_reqd = FALSE;
    if (y_type & BOTTOM_ZONE)           /* Bottom edge of stroke is in bottom zone? */
        {
        /* Test if top edge of stroke is in blue zone */
        while (pStroke->pos2 > sp_globals.processor.pcl.BlueZones[pBlueState->ii].top_orus)
            {
            flush_reqd = TRUE;          /* Switch to new blue zone requires flush */
            if (++pBlueState->ii >= sp_globals.processor.pcl.nBlueZones) /* No more blue zones? */
                {
                goto L0;                /* Only the bottom edge of stroke is controlled */
                }
            }

        if ((pStroke->pos2 >= sp_globals.processor.pcl.BlueZones[pBlueState->ii].bottom_orus) &&
            (sp_globals.processor.pcl.BlueZones[pBlueState->ii].type & TOP_ZONE))  /* Stroke top is in top zone? */
            {
            /* Accumulate constraint to align bottom edge of stroke */
            priority = pStroke->priority + 16000;
            if (priority > pBlueState->priority)
                {
                pBlueState->priority = priority;
                pBlueState->orus = pStroke->pos1 + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;
                pBlueState->pix = y_pix;
#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
                pBlueState->bottom = pStroke->pos1;
                pBlueState->top = pStroke->pos1;
#endif
                }

            flush_accum(PARAMS2 pBlueState);    /* Flush accumulated constraint */

            /* Accumulate constraint to align top edge of stroke */
            pBlueState->priority = priority;
            pBlueState->orus = pStroke->pos2 + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;
            pBlueState->pix = sp_globals.processor.pcl.BlueZones[pBlueState->ii].pix;
#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
            pBlueState->bottom = pStroke->pos2;
            pBlueState->top = pStroke->pos2;
#endif
            return;
            }
        }

L0:
    /* Stroke with one edge constrained */
    priority = pStroke->priority + 16000;
    if (priority > pBlueState->priority) /* Higher priority than existing constraint (if any)? */
        {
        pBlueState->priority = priority;
        cy = (pStroke->pos1 + pStroke->pos2 + 1) >> 1; /* Stroke mid point */
        pBlueState->orus = cy + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;
        strkwt_orus = pStroke->pos2 - pStroke->pos1;
        strkwt_pix = 
            ((((fix31)strkwt_orus * 
               (fix31)sp_globals.processor.pcl.y_pix_per_oru) + 
              sp_globals.multrnd) >>
             sp_globals.mpshift) &
            sp_globals.pixfix;          /* Nearest whole number of pixels */
        if (strkwt_pix < sp_globals.processor.pcl.minhstemweight)
            strkwt_pix = sp_globals.processor.pcl.minhstemweight;
        hstrkwt_pix = (strkwt_pix + 1) >> 1;
        if (y_type & BOTTOM_ZONE)
            {
            pBlueState->pix = y_pix + hstrkwt_pix;
            }
        else
            {
            pBlueState->pix = y_pix - hstrkwt_pix;
            }
#if INCL_PLAID_OUT                      /* Plaid data monitoring included? */
        pBlueState->bottom = pStroke->pos1;
        pBlueState->top = pStroke->pos2;
#endif
        if (flush_reqd)
            {
            flush_accum(PARAMS2 pBlueState);    /* Flush accumulated constraint */
            }
        }
    return;
    }

L1:
/* Do floating horizontal stroke */
cy = ((pStroke->pos1 + pStroke->pos2 + 1) >> 1) + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;
strkwt_orus = pStroke->pos2 - pStroke->pos1;
strkwt_pix = 
    ((((fix31)strkwt_orus * 
       (fix31)sp_globals.processor.pcl.y_pix_per_oru) + 
      sp_globals.multrnd) >>
     sp_globals.mpshift) &
    sp_globals.pixfix;                  /* Rounded stroke weight in sub-pixel units */
if (strkwt_pix < sp_globals.processor.pcl.minhstemweight)
    strkwt_pix = sp_globals.processor.pcl.minhstemweight;
if (strkwt_pix & sp_globals.onepix)     /* Odd number of pixels? */
    {
    cy_pix = 
        (((((fix31)cy * 
            (fix31)sp_globals.processor.pcl.y_pix_per_oru) + 
           sp_globals.processor.pcl.y_pos) >>
          sp_globals.mpshift) &
         sp_globals.pixfix) +     
        sp_globals.pixrnd;              /* Round mid-point to nearest pixel center */
    }
else
    {
    cy_pix = 
        ((((fix31)cy * 
           (fix31)sp_globals.processor.pcl.y_pix_per_oru) + 
          sp_globals.processor.pcl.y_pos +
          sp_globals.multrnd) >>
         sp_globals.mpshift) &
        sp_globals.pixfix;              /* Round to nearest pixel boundary */
    }
priority = pStroke->priority + 16000;
add_y_constraint(PARAMS2 cy, cy_pix, priority);

#if INCL_PLAID_OUT              /* Plaid data monitoring included? */
sp_globals.processor.pcl.hstem_bottom[sp_globals.processor.pcl.nhstems] = pStroke->pos1;
sp_globals.processor.pcl.hstem_top[sp_globals.processor.pcl.nhstems] = pStroke->pos2;
sp_globals.processor.pcl.nhstems++;
#endif
}


FUNCTION LOCAL_PROTO void init_accum(blue_state_t *pBlueState)
/*
 * Initializes the vertical constraint accumulator.
 * The constraint accumulator compares each new constraint
 * found within a blue zone with the current constraint.
 * If the new constraint is better, it replaces the current
 * constraint
 */
{
pBlueState->priority = -1;
pBlueState->ii = 0;
}


FUNCTION LOCAL_PROTO void flush_accum(SPGLOBPTR2 blue_state_t *pBlueState)
/*
 * Flushes any accumulated constraint from
 * the constraint accumulator
 */
{

if (pBlueState->priority >= 0)
    {
    add_y_constraint(PARAMS2 pBlueState->orus, pBlueState->pix, pBlueState->priority);
    pBlueState->priority = -1;

#if INCL_PLAID_OUT              /* Plaid data monitoring included? */
    sp_globals.processor.pcl.hstem_bottom[sp_globals.processor.pcl.nhstems] = pBlueState->bottom;
    sp_globals.processor.pcl.hstem_top[sp_globals.processor.pcl.nhstems] = pBlueState->top;
    sp_globals.processor.pcl.nhstems++;
#endif
    }
}



FUNCTION static void add_y_constraint(SPGLOBPTR2 fix15 orus, fix15 pix, fix15 priority)
/*
 * Adds one constraint point to piecewise-linear transformation for y
 * Assumes that points are added in increasing order of orus
 * If a pair of points are too close in either orus or pixels, the
 * higher priority prevails
 */
{
fix15   ii;

if (sp_globals.processor.pcl.no_y_breaks >= MAX_BREAKS)          /* Breakpoint table full? */
    {
#if DEBUG
    printf("*** add_y_constraint: Too many Y breaks\n");
#endif
    return;                             /* Ignore if breakpoint table full */
    }

if (sp_globals.processor.pcl.no_y_breaks)                        /* Breakpoint table not empty? */
    {
    ii = sp_globals.processor.pcl.no_y_breaks - 1;
    if (((orus - sp_globals.processor.pcl.Yorus[ii]) < sp_globals.processor.pcl.eo_min_y_oru_gap) ||
        ((pix - sp_globals.processor.pcl.Ypix[ii]) < ((fix15)MIN_Y_PIX_GAP << sp_globals.pixshift)) ||
        ((fix31)(pix - sp_globals.processor.pcl.Ypix[ii]) > (fix31)sp_globals.processor.pcl.max_ppo * (fix31)(orus - sp_globals.processor.pcl.Yorus[ii])))
        {
        if (priority < sp_globals.processor.pcl.old_y_priority)
            return;
        sp_globals.processor.pcl.no_y_breaks--;
        }
    }

sp_globals.processor.pcl.Yorus[sp_globals.processor.pcl.no_y_breaks] = orus;
sp_globals.processor.pcl.Ypix[sp_globals.processor.pcl.no_y_breaks] = pix;
sp_globals.processor.pcl.no_y_breaks++;
sp_globals.processor.pcl.old_y_priority = priority;
}


FUNCTION static void update_y_trans(SPGLOBPTR1)
/*
 * Updates the interpolation coefficient tables used
 * for piecewise linear transformation of Y coordinates in character
 */
{
fix15   ii;

if (sp_globals.processor.pcl.no_y_breaks == 0)
    {
    sp_globals.processor.pcl.Ymult[0] = (fix31)sp_globals.processor.pcl.y_pix_per_oru;
    sp_globals.processor.pcl.Yoffset[0] = sp_globals.processor.pcl.y_pos + sp_globals.mprnd;
    }
else
    {
    sp_globals.processor.pcl.Ymult[0] = (fix31)sp_globals.processor.pcl.y_pix_per_oru;
    sp_globals.processor.pcl.Yoffset[0] = 
        ((fix31)sp_globals.processor.pcl.Ypix[0] << sp_globals.mpshift) - 
        ((fix31)sp_globals.processor.pcl.Yorus[0] * (fix31)sp_globals.processor.pcl.y_pix_per_oru) +
        sp_globals.mprnd;

    for (ii = 1; ii < sp_globals.processor.pcl.no_y_breaks; ii++)
        {
        sp_globals.processor.pcl.Ymult[ii] = 
            ((fix31)(sp_globals.processor.pcl.Ypix[ii] - sp_globals.processor.pcl.Ypix[ii - 1]) << sp_globals.mpshift) / 
            (fix31)(sp_globals.processor.pcl.Yorus[ii] - sp_globals.processor.pcl.Yorus[ii - 1]);
        sp_globals.processor.pcl.Yoffset[ii] = 
            ((fix31)sp_globals.processor.pcl.Ypix[ii] << sp_globals.mpshift) - 
            ((fix31)sp_globals.processor.pcl.Yorus[ii] * sp_globals.processor.pcl.Ymult[ii]) +
            sp_globals.mprnd;
        }

    sp_globals.processor.pcl.Ymult[ii] = (fix31)sp_globals.processor.pcl.y_pix_per_oru;
    sp_globals.processor.pcl.Yoffset[ii] = 
        ((fix31)sp_globals.processor.pcl.Ypix[sp_globals.processor.pcl.no_y_breaks - 1] << sp_globals.mpshift) - 
        ((fix31)sp_globals.processor.pcl.Yorus[sp_globals.processor.pcl.no_y_breaks - 1] * (fix31)sp_globals.processor.pcl.y_pix_per_oru) +
        sp_globals.mprnd;
    }
}




FUNCTION void eo_do_trans(SPGLOBPTR2 fix15 x_abs, fix15 y_abs, point_t *pP)
/* 
 * Transforms the point (x_abs, y_abs) in design window coordinates
 * into a point (pP->x, pP->y) in device coordinates.
 */
{
fix15   ii;
fix15   X_orus;    /* X coordinate relative to left side-bearing */
fix15   Y_orus;    /* Y coordinate relative to baseline */
fix15   xtrans;    /* Non-linear transformation of X coordinate */
fix15   ytrans;    /* Non-linear transformation of Y coordinate */

X_orus = x_abs + sp_globals.processor.pcl.x_off - sp_globals.processor.pcl.eo_left_reference;
Y_orus = y_abs + sp_globals.processor.pcl.y_off - sp_globals.processor.pcl.eo_baseline;

for (ii = 0; ii < sp_globals.processor.pcl.no_x_breaks; ii++)
    {
    if (X_orus <= sp_globals.processor.pcl.Xorus[ii])
        {
        break;
        }
    }
xtrans = (((fix31)X_orus * sp_globals.processor.pcl.Xmult[ii]) + sp_globals.processor.pcl.Xoffset[ii]) >> sp_globals.mpshift;

for (ii = 0; ii < sp_globals.processor.pcl.no_y_breaks; ii++)
    {
    if (Y_orus <= sp_globals.processor.pcl.Yorus[ii])
        {
        break;
        }
    }
ytrans = (((fix31)Y_orus * sp_globals.processor.pcl.Ymult[ii]) + sp_globals.processor.pcl.Yoffset[ii]) >> sp_globals.mpshift;

switch(sp_globals.processor.pcl.x_trans_mode)  /* Switch on method of calculating transformed X value */
    {
case 1:               /* X is a function of x only */
    pP->x = xtrans;
    break;

case 2:               /* X is a function of x only */
    pP->x = -xtrans;
    break;

case 3:               /* X is a function of y only */
    pP->x = ytrans;
    break;

case 4:               /* X is a function of -y only */
    pP->x = -ytrans;
    break;

default:              /* X is a linear function of x and y */
    pP->x = 
        ((fix31)X_orus * sp_globals.processor.pcl.ctm[0] + 
         (fix31)Y_orus * sp_globals.processor.pcl.ctm[2] + 
         sp_globals.processor.pcl.ctm[4] +
         sp_globals.mprnd) >> 
        sp_globals.mpshift;
    break;
    }

switch(sp_globals.processor.pcl.y_trans_mode)  /* Switch on method of calculating transformed X value */
    {
case 1:               /* Y is a function of x only */
    pP->y = xtrans;
    break;

case 2:               /* Y is a function of -x only */
    pP->y = -xtrans;
    break;

case 3:               /* Y is a function of y only */
    pP->y = ytrans;
    break;

case 4:               /* Y is a function of -y only */
    pP->y = -ytrans;
    break;

default:              /* Y is a linear function of x and y */
    pP->y = 
        ((fix31)X_orus * sp_globals.processor.pcl.ctm[1] + 
         (fix31)Y_orus * sp_globals.processor.pcl.ctm[3] + 
         sp_globals.processor.pcl.ctm[5] +
         sp_globals.mprnd) >> 
        sp_globals.mpshift;
    break;
    }
}


#if INCL_PLAID_OUT              /* Plaid data monitoring included? */

FUNCTION LOCAL_PROTO void show_plaid(SPGLOBPTR1)
{
fix15   ii;
fix31   start;
fix31   end;
fix15   constr;
fix15   x_orus;
fix15   y_orus;
point_t P;
fix15   poshift;


constr = 0;
poshift = 16 - sp_globals.pixshift;

begin_plaid_data();
begin_ctrl_zones(sp_globals.processor.pcl.nvstems, sp_globals.processor.pcl.nhstems);
for (ii = 0; ii < sp_globals.processor.pcl.nvstems; ii++)
    {
    x_orus = sp_globals.processor.pcl.vstem_left[ii];
    y_orus = sp_globals.processor.pcl.eo_baseline;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    start = (fix31)(P.x) << poshift;
    x_orus = sp_globals.processor.pcl.vstem_right[ii];
    y_orus = sp_globals.processor.pcl.eo_baseline;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    end = (fix31)(P.x) << poshift;
    record_ctrl_zone(start, end, constr);
    }

for (ii = 0; ii < sp_globals.processor.pcl.nhstems; ii++)
    {
    x_orus = sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.hstem_bottom[ii];
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    start = (fix31)(P.y) << poshift;
    x_orus = sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.hstem_top[ii];
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    end = (fix31)(P.y) << poshift;
    record_ctrl_zone(start, end, constr);
    }

begin_int_zones((fix15)0, sp_globals.processor.pcl.nBlueZones);

for (ii = 0; ii < sp_globals.processor.pcl.nBlueZones; ii++)
    {
    x_orus = sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.BlueZones[ii].bottom_orus;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    start = (fix31)(P.y) << poshift;
    x_orus = sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.BlueZones[ii].top_orus;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    end = (fix31)(P.y) << poshift;
    record_int_zone(start, end);
    }

end_plaid_data();
}
#endif


#if DEBUG

FUNCTION LOCAL_PROTO void show_contour_points(read_state_t *pReadState)
{
fix15   ii;
ufix8  *pX;
ufix8  *pY;
ufix8  *pAuxX;
ufix8  *pAuxY;
ufix16  X, Y, X1, Y1;
fix15   AuxX, AuxY;

printf("\n     X      Y   AuxX   AuxY\n");
pX = pReadState->pX;
pY = pReadState->pY;
pAuxX = pReadState->pAuxX;
pAuxY = pReadState->pAuxY;
for (ii = 0; ii < pReadState->nPoint; ii++)
    {
    X = GET_WORD(pX);
    pX += 2;
    Y = GET_WORD(pY);
    pY += 2;
    printf("%6d %6d ", 
        (int)(X & 0x3fff), 
        (int)(Y & 0x3fff)); 
    if ((X & 0x4000) == 0)              /* Aux point before next point? */
        {
        X &= 0x3fff;
        Y &= 0x3fff;
        AuxX = (fix15)((fix7)*pAuxX);
        pAuxX++;
        AuxY = (fix15)((fix7)*pAuxY);
        pAuxY++;
        X1 = GET_WORD(pX) & 0x3fff;
        Y1 = GET_WORD(pY) & 0x3fff;
        printf("%6d %6d ",
            (int)(((X + X1 + 1) >> 1) + AuxX), 
            (int)(((Y + Y1 + 1) >> 1) + AuxY)); 
        }
    printf("\n");
    }
}
#endif


#if DEBUG
static char *edge_type_table[] = 
    {
    "Impossible 0",           /*  0 */
    "STRAIGHT LEFT  ",        /*  1 */
    "STRAIGHT RIGHT ",        /*  2 */
    "Impossible 3",           /*  3 */
    "STRAIGHT BOTTOM",        /*  4 */
    "Impossible 5",           /*  5 */
    "Impossible 6",           /*  6 */
    "Impossible 7",           /*  7 */
    "STRAIGHT TOP   ",        /*  8 */
    "Impossible 9",           /*  9 */
    "Impossible 10",          /* 10 */
    "Impossible 11",          /* 11 */
    "Impossible 12",          /* 12 */
    "Impossible 13",          /* 13 */
    "Impossible 14",          /* 14 */
    "Impossible 15",          /* 15 */
    "Impossible 16",          /* 16 */
    "CURVED LEFT    ",        /* 17 */
    "CURVED RIGHT   ",        /* 18 */
    "Impossible 19",          /* 19 */
    "CURVED BOTTOM  ",        /* 20 */
    "Impossible 21",          /* 21 */
    "Impossible 22",          /* 22 */
    "Impossible 23",          /* 23 */
    "CURVED TOP     ",        /* 24 */
    "Impossible 25",          /* 25 */
    "Impossible 26",          /* 26 */
    "Impossible 27",          /* 27 */
    "Impossible 28",          /* 28 */
    "Impossible 29",          /* 29 */
    "Impossible 30",          /* 30 */
    "Impossible 31",          /* 31 */
    };

FUNCTION LOCAL_PROTO void show_edge_lists(SPGLOBPTR1)
{
fix15   ii;

if (sp_globals.processor.pcl.nv_edges)
    {
    printf("\nVertical edges\n");
    printf("      TYPE        XPOS   YPOS    MIN    MAX\n");
    for (ii = 0; ii < sp_globals.processor.pcl.nv_edges; ii++)
        {
        printf("%s %6d %6d %6d %6d\n",
            edge_type_table[sp_globals.processor.pcl.v_edge_list[ii].type & 0x001f],
            sp_globals.processor.pcl.v_edge_list[ii].xpos,
            sp_globals.processor.pcl.v_edge_list[ii].ypos,
            sp_globals.processor.pcl.v_edge_list[ii].min,
            sp_globals.processor.pcl.v_edge_list[ii].max);
        }
    }
if (sp_globals.processor.pcl.nh_edges)
    {
    printf("\nHorizontal edges\n");
    printf("      TYPE        XPOS   YPOS    MIN    MAX\n");
    for (ii = 0; ii < sp_globals.processor.pcl.nh_edges; ii++)
        {
        printf("%s %6d %6d %6d %6d\n",
            edge_type_table[sp_globals.processor.pcl.h_edge_list[ii].type & 0x001f],
            sp_globals.processor.pcl.h_edge_list[ii].xpos,
            sp_globals.processor.pcl.h_edge_list[ii].ypos,
            sp_globals.processor.pcl.h_edge_list[ii].min,
            sp_globals.processor.pcl.h_edge_list[ii].max);
        }
    }
}
#endif


#if DEBUG

FUNCTION LOCAL_PROTO void show_x_trans(SPGLOBPTR1)
/* 
 * Prints the transformation coefficients 
 * for X coordinates in the character outline.
 * For debugging purposes only
 */
{
fix15   ii;
fix15   x_orus;
fix15   y_orus;
point_t P;


printf("\nTransformation table for X coordinates:\n");
printf("Zone edges       sp_globals.processor.pcl.Xpix       sp_globals.processor.pcl.Xmult      sp_globals.processor.pcl.Xoffset         trans\n");
printf("----------     --------   ---------- ------------      -------\n");
for (ii = 0; ii < sp_globals.processor.pcl.no_x_breaks; ii++)
    {
    x_orus = sp_globals.processor.pcl.Xorus[ii] - sp_globals.processor.pcl.x_off + sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.eo_baseline;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    printf("%10d %12.1f %12.5f %12.5f %12.5f\n",
        sp_globals.processor.pcl.Xorus[ii], 
        (real)sp_globals.processor.pcl.Xpix[ii] / (real)sp_globals.onepix, 
        (real)sp_globals.processor.pcl.Xmult[ii] / (real)(sp_globals.multrnd << 1), 
        (real)sp_globals.processor.pcl.Xoffset[ii] / (real)(sp_globals.multrnd << 1), 
        (real)P.x / (real)sp_globals.onepix);
    }
printf("                        %12.5f %12.5f\n",
    (real)sp_globals.processor.pcl.Xmult[sp_globals.processor.pcl.no_x_breaks] / (real)(sp_globals.multrnd << 1), 
    (real)sp_globals.processor.pcl.Xoffset[sp_globals.processor.pcl.no_x_breaks] / (real)(sp_globals.multrnd << 1));
return;
}
#endif

#if DEBUG

FUNCTION LOCAL_PROTO void show_y_trans(SPGLOBPTR1)
/* 
 * Prints the transformation coefficients 
 * for Y coordinates in the character outline.
 * For debugging purposes only
 */
{
fix15   ii;
fix15   x_orus;
fix15   y_orus;
point_t P;


printf("\nTransformation table for Y coordinates:\n");
printf("Zone edges       sp_globals.processor.pcl.Ypix       sp_globals.processor.pcl.Ymult      sp_globals.processor.pcl.Yoffset         trans\n");
printf("----------     --------   ---------- ------------      -------\n");
for (ii = 0; ii < sp_globals.processor.pcl.no_y_breaks; ii++)
    {
    x_orus = sp_globals.processor.pcl.eo_left_reference;
    y_orus = sp_globals.processor.pcl.Yorus[ii] - sp_globals.processor.pcl.y_off + sp_globals.processor.pcl.eo_baseline;
    eo_do_trans(PARAMS2 x_orus, y_orus, &P);
    printf("%10d %12.1f %12.5f %12.5f %12.5f\n",
        sp_globals.processor.pcl.Yorus[ii], 
        (real)sp_globals.processor.pcl.Ypix[ii] / (real)sp_globals.onepix, 
        (real)sp_globals.processor.pcl.Ymult[ii] / (real)(sp_globals.multrnd << 1), 
        (real)sp_globals.processor.pcl.Yoffset[ii] / (real)(sp_globals.multrnd << 1), 
        (real)P.y / (real)sp_globals.onepix);
    }
printf("                        %12.5f %12.5f\n",
    (real)sp_globals.processor.pcl.Ymult[sp_globals.processor.pcl.no_y_breaks] / (real)(sp_globals.multrnd << 1), 
    (real)sp_globals.processor.pcl.Yoffset[sp_globals.processor.pcl.no_y_breaks] / (real)(sp_globals.multrnd << 1));
return;
}
#endif
#endif /* PROC_PCL */
/* EOF hpfnt2.c */

