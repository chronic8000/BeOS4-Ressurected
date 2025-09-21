/*****************************************************************************
*                                                                            *
*                        Copyright 1995                                      *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** O U T _ G R A Y . C *****************************
 *                                                                           *
 * 4-in-1/TrueDoc Print System antialiased output (grayscale) functions.     *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *
 *     $Header: //bliss/user/archive/rcs/speedo/out_gray.c,v 33.0 97/03/17 17:46:35 shawn Release $
 *                                                                                    
 *     $Log:	out_gray.c,v $
 * Revision 33.0  97/03/17  17:46:35  shawn
 * TDIS Version 6.00
 * 
 * Revision 1.6  96/08/06  16:33:37  shawn
 * Cast arguments in call to open_bitmap().
 * 
 * Revision 1.5  96/07/02  12:11:33  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 1.4  96/05/24  10:14:23  shawn
 * Corrected RCS strings.
 * 
 * 
 ****************************************************************************/


#include "spdo_prv.h"           /* General definitions for 4-in-1 */

#define DEBUG  0

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


#if INCL_GRAY

#ifndef GRAY_BUFF_SIZE
#define GRAY_BUFF_SIZE  8
#endif

#define MAX_INTERCEPTS_GRAY MAX_INTERCEPTS

/* Edge codes */
#define BOTTOM_EDGE 0
#define RIGHT_EDGE  1
#define TOP_EDGE    2
#define LEFT_EDGE   3
#define UNDEF_EDGE  4

/* Winding number flags */
#define LEADING_EDGE     0x80
#define TRAILING_EDGE    0x40
#define FRACT_AREA       0x3f

/* Scanline interpretation states */
#define WHITE           0
#define GRAY            1
#define BLACK           2

#define startPixel sp_globals.startPixel
#define currentPixel sp_globals.currentPixel
#define x_band sp_globals.x_band
#define y_band sp_globals.y_band
#define doublePixelArea sp_globals.doublePixelArea
#define doublePixelHeight sp_globals.doublePixelHeight

/* Pixel intensity table */
static ufix8 gammaTable[] =
    {
      0,   4,   8,  12,  16,  20,  24,  28, 
     32,  36,  40,  45,  49,  53,  57,  61,
     65,  69,  73,  77,  81,  85,  89,  93,
     97, 101, 105, 109, 113, 117, 121, 126,
    130, 134, 138, 142, 146, 150, 154, 158,
    162, 166, 170, 174, 178, 182, 186, 190,
    194, 198, 202, 206, 211, 215, 219, 223,
    227, 231, 235, 239, 243, 247, 251, 255
    };

/* Local function prototypes */
LOCAL_PROTO
void InitIntercepts(SPGLOBPTR1);

LOCAL_PROTO
void AddIntercept(SPGLOBPTR1);

LOCAL_PROTO
void ProcIntercepts(SPGLOBPTR1);

#if DEBUG
LOCAL_PROTO
void ShowIntercepts(SPGLOBPTR1);
#endif


FUNCTION
btsbool sp_init_gray(SPGLOBPTR2 specs_t GLOBALFAR *specsarg)
/*
 *  Initializes the antialiased output module.
 *  Returns TRUE if output module can accept requested specifications.
 *  Returns FALSE otherwise.
 */
{
#if DEBUG
printf("\nINIT_GRAY()\n");
printf("     Mode Flags = %8.0x\n", specsarg->flags);
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

if (sp_globals.pixelSize < 2)   /* Only 1 bit/pixel */
    return FALSE;

return TRUE;
}



FUNCTION
btsbool sp_begin_char_gray(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/*
 *  Called once at the start of the character generation process
 */
{
#if DEBUG
printf("\nBEGIN_CHAR_GRAY(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n", 
    (real)Psw.x / (real)sp_globals.onepix, 
    (real)Psw.y / (real)sp_globals.onepix,
    (real)Pmin.x / (real)sp_globals.onepix, 
    (real)Pmin.y / (real)sp_globals.onepix,
    (real)Pmax.x / (real)sp_globals.onepix, 
    (real)Pmax.y / (real)sp_globals.onepix);
#endif

/* Save escapement vector for later use in OpenBitmap() */
sp_globals.set_width.x = (fix31)Psw.x << sp_globals.poshift;
sp_globals.set_width.y = (fix31)Psw.y << sp_globals.poshift;

/* Set initial band to specified bounding box
   allowing for some scaling jitter */
x_band.band_min = 
    (Pmin.x >> sp_globals.pixshift) - 1;
x_band.band_max = 
    (Pmax.x >> sp_globals.pixshift) + 2;
y_band.band_min = 
    (Pmin.y >> sp_globals.pixshift) - 1;
y_band.band_max = 
    (Pmax.y >> sp_globals.pixshift) + 2;

/* Initialize some area computation constants */
doublePixelArea = 2L << (sp_globals.pixshift << 1);
doublePixelHeight = 2 << sp_globals.pixshift;

/* Initialize intercept storage */
InitIntercepts(PARAMS1);

/* Initialize bitmap bounding box */
if (sp_globals.normal)          /* Bitmap bounding box is known accurately? */
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
void sp_begin_sub_char_gray(SPGLOBPTR2 point_t Psw, point_t Pmin, point_t Pmax)
/*
 *  Called at the start of each sub-character in a composite character
 */
{
#if DEBUG
printf("\nBEGIN_SUB_CHAR_GRAY(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n", 
    (real)Psw.x / (real)sp_globals.onepix, 
    (real)Psw.y / (real)sp_globals.onepix,
    (real)Pmin.x / (real)sp_globals.onepix, 
    (real)Pmin.y / (real)sp_globals.onepix,
    (real)Pmax.x / (real)sp_globals.onepix, 
    (real)Pmax.y / (real)sp_globals.onepix);
#endif
}



FUNCTION
void sp_begin_contour_gray(SPGLOBPTR2 point_t P1, btsbool outside)
/*
 *  Called at the start of each contour
 */
{
#if DEBUG
printf("\nBEGIN_CONTOUR_GRAY(%5.3f, %5.3f, %s)\n", 
    (real)P1.x / (real)sp_globals.onepix, 
    (real)P1.y / (real)sp_globals.onepix, 
    outside? "outside": "inside");
#endif

/***** Used for Emboldening *****/
/* Determine if contour direction is reversed */
/* SetContourDirection(SPGLOBPTR2 outside); */

sp_globals.x0_spxl = P1.x;
sp_globals.y0_spxl = P1.y;

currentPixel.xOrg = P1.x & sp_globals.pixfix;
currentPixel.yOrg = P1.y & sp_globals.pixfix;
currentPixel.area = 0;
currentPixel.edgeCodes = UNDEF_EDGE << 2;
}


FUNCTION
void sp_curve_gray(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
/*
 *  This is a dummy function that is never called 
 */
{
}

FUNCTION
void sp_line_gray(SPGLOBPTR2 point_t P1)
/*
 *  Called for each vector in the transformed character
 *  Determines which pixels that fall within the current band are
 *  intercepted.
 *  As each pixel intercept is completed, the area and winding
 *  number flags associated with the intercept is added to the 
 *  intercept structure.
 */
{
fix15   xstart_spxl;    /* X coord of start point (sub-pixel units) */
fix15   ystart_spxl;    /* Y coord of start point (sub-pixel units) */
fix15   xend_spxl;      /* X coord of end point (sub-pixel units) */
fix15   yend_spxl;      /* Y coord of end point (sub-pixel units) */
fix31   dx;             /* X increment (sub-pixel units) */
fix31   dy;             /* Y increment (sub-pixel units) */
fix15   xOrgLast;       /* Left edge of last pixel intercepted (spus) */
fix15   yOrgLast;       /* Bottom edge of last pixel intercepted (spus) */
fix15   yFactor;
fix31   dxdy;
fix31   dydx;
fix31   xx, yy;
fix31   xxv, xxh;
fix31   yyv, yyh;
ufix8   vTypeIn, vTypeOut, hTypeIn, hTypeOut;
fix15   xIn, yIn, xOut, yOut;
fix15   xOrgIncr, yOrgIncr;
fix15   space_left;     /* Space left in the intercept structure */
fix15   shift;
fix31   round;
fix15   xLocal, yLocal;
fix31   adx, ady;
fix31   d, dh, dv;

#if DEBUG
printf("\nLINE_GRAY(%5.3f, %5.3f)\n", 
   (real)P1.x/(real)sp_globals.onepix, 
   (real)P1.y/(real)sp_globals.onepix);
#endif

xstart_spxl = sp_globals.x0_spxl;
dx = (fix31)P1.x - sp_globals.x0_spxl;
sp_globals.x0_spxl = xend_spxl = P1.x;

ystart_spxl = sp_globals.y0_spxl;
dy = (fix31)P1.y - sp_globals.y0_spxl;
sp_globals.y0_spxl = yend_spxl = P1.y;

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

/* Check if intercept overflow has already occured */
if (sp_globals.intercept_oflo)
    {
    return;
    }

/* Check for no exit from current pixel */
xOrgLast = xend_spxl & sp_globals.pixfix;
yOrgLast = yend_spxl & sp_globals.pixfix;
if ((xOrgLast == currentPixel.xOrg) && 
    (yOrgLast == currentPixel.yOrg))
    {
    currentPixel.area += 
        (fix31)(xend_spxl - xstart_spxl) * 
        (yend_spxl + ystart_spxl - (yOrgLast << 1));
    return;
    }

if (dx == 0)                    /* Vertical vector? */
    {
    currentPixel.xOut = xend_spxl - currentPixel.xOrg;
    if (dy > 0)                 /* Direction is up? */
        {
        currentPixel.yOut = sp_globals.onepix;
        currentPixel.edgeCodes |= TOP_EDGE;
        AddIntercept(PARAMS1);
        currentPixel.yOrg += sp_globals.onepix;
        currentPixel.xIn = currentPixel.xOut;
        currentPixel.yIn = 0;
        currentPixel.edgeCodes = (BOTTOM_EDGE << 2) | TOP_EDGE;
        currentPixel.area = 0;
        while (yend_spxl >= (currentPixel.yOrg + sp_globals.onepix))
            {
            AddIntercept(PARAMS1);
            currentPixel.yOrg += sp_globals.onepix;
            }
        currentPixel.edgeCodes = BOTTOM_EDGE << 2;
        return;
        }
    else                        /* Direction is down? */
    	{
        currentPixel.yOut = 0;
        currentPixel.edgeCodes |= BOTTOM_EDGE;
        AddIntercept(PARAMS1);
        currentPixel.yOrg -= sp_globals.onepix;
        currentPixel.xIn = currentPixel.xOut;
        currentPixel.yIn = sp_globals.onepix;
        currentPixel.edgeCodes = (TOP_EDGE << 2) | BOTTOM_EDGE;
        currentPixel.area = 0;
        while (yend_spxl < currentPixel.yOrg)
            {
            AddIntercept(PARAMS1);
            currentPixel.yOrg -= sp_globals.onepix;
            }
        currentPixel.edgeCodes = TOP_EDGE << 2;
        return;
        }
	}

else if (dy == 0)               /* Horizontal vector? */
    {
    currentPixel.yOut = yend_spxl - currentPixel.yOrg;
    if (dx > 0)                 /* Direction is to the right? */
        {
        currentPixel.xOut = sp_globals.onepix;
        if (currentPixel.yOut == 0)
            {
            currentPixel.edgeCodes |= BOTTOM_EDGE;
            AddIntercept(PARAMS1);
            currentPixel.xOrg = xOrgLast;
            currentPixel.xIn = 0;
            currentPixel.yIn = 0;
            currentPixel.edgeCodes = BOTTOM_EDGE << 2;
            currentPixel.area = 0;
            return;
            }
        else
            {
            yFactor = currentPixel.yOut << 1;
            currentPixel.edgeCodes |= RIGHT_EDGE;
            currentPixel.area += 
                (fix31)(sp_globals.onepix - (xstart_spxl - currentPixel.xOrg)) *
                yFactor;
            AddIntercept(PARAMS1);
            currentPixel.xOrg += sp_globals.onepix;
            currentPixel.xIn = 0;
            currentPixel.yIn = currentPixel.yOut;
            currentPixel.edgeCodes = (LEFT_EDGE << 2) | RIGHT_EDGE;
            currentPixel.area = (fix31)(sp_globals.onepix) * yFactor;
            while (xend_spxl >= (currentPixel.xOrg + sp_globals.onepix))
                {
                AddIntercept(PARAMS1);
                currentPixel.xOrg += sp_globals.onepix;
                }
            currentPixel.edgeCodes = LEFT_EDGE << 2;
            currentPixel.area = (fix31)(xend_spxl - currentPixel.xOrg) * yFactor;
            return;
            }
        }
    else                        /* Direction is to the left? */
    	{
        currentPixel.xOut = 0;
        if (currentPixel.yOut == 0)
            {
            currentPixel.edgeCodes |= BOTTOM_EDGE;
            AddIntercept(PARAMS1);
            currentPixel.xOrg = xOrgLast;
            currentPixel.xIn = sp_globals.onepix;
            currentPixel.yIn = 0;
            currentPixel.edgeCodes = BOTTOM_EDGE << 2;
            currentPixel.area = 0;
            return;
            }
        else
            {
            currentPixel.edgeCodes |= LEFT_EDGE;
            yFactor = currentPixel.yOut << 1;
            currentPixel.area -= (fix31)(xstart_spxl - currentPixel.xOrg) * yFactor;
            AddIntercept(PARAMS1);
            currentPixel.xOrg -= sp_globals.onepix;
            currentPixel.xIn = sp_globals.onepix;
            currentPixel.yIn = currentPixel.yOut;
            currentPixel.edgeCodes = (RIGHT_EDGE << 2) | LEFT_EDGE;
            currentPixel.area = (fix31)(-sp_globals.onepix) * yFactor;
            while (xend_spxl < currentPixel.xOrg)
                {
                AddIntercept(PARAMS1);
                currentPixel.xOrg -= sp_globals.onepix;
                }
            currentPixel.edgeCodes = RIGHT_EDGE << 2;
            currentPixel.area = 
                (fix31)(xend_spxl - currentPixel.xOrg - sp_globals.onepix) * yFactor;
            return;
            }
        }
	}

/* Calculate signed values of dx/dy and dy/dx (16.16 units) */
dxdy = ((ufix32)(dx >= 0? dx: -dx) << 16) / (ufix32)(dy >= 0? dy: -dy);
dydx = ((ufix32)(dy >= 0? dy: -dy) << 16) / (ufix32)(dx >= 0? dx: -dx);
if ((dx >= 0) ^ (dy >= 0))
    {
    dxdy = -dxdy;
    dydx = -dydx;
    }

if (dx > 0)
    {
    adx = dx;
    vTypeIn = LEFT_EDGE << 2;
    vTypeOut = RIGHT_EDGE;
    xIn = 0;
    xOut = sp_globals.onepix;
    xOrgIncr = sp_globals.onepix;
    xxv = -65536L;
    yyv = dydx;
    }
else
    {
    adx = -dx;
    vTypeIn = RIGHT_EDGE << 2;
    vTypeOut = LEFT_EDGE;
    xIn = sp_globals.onepix;
    xOut = 0;
    xOrgIncr = -sp_globals.onepix;
    xxv = 65536L;
    yyv = -dydx;
    }

if (dy > 0)
    {
    ady = dy;
    hTypeIn = BOTTOM_EDGE << 2;
    hTypeOut = TOP_EDGE;
    yIn = 0;
    yOut = sp_globals.onepix;
    yOrgIncr = sp_globals.onepix;
    xxh = dxdy;
    yyh = -65536L;
    }
else
    {
    ady = -dy;
    hTypeIn = TOP_EDGE << 2;
    hTypeOut = BOTTOM_EDGE;
    yIn = sp_globals.onepix;
    yOut = 0;
    yOrgIncr = -sp_globals.onepix;
    xxh = -dxdy;
    yyh = 65536L;
    }

shift = 16 - sp_globals.pixshift;
round = (1L << shift) >> 1;

xLocal = xstart_spxl - currentPixel.xOrg;
yLocal = ystart_spxl - currentPixel.yOrg;

/* Find X intercept (16.16 local) with next horizontal pixel boundary */
if (currentPixel.yOrg != yOrgLast)
    {
    xx  = 
        ((fix31)xLocal << shift) +
        (((yOut - yLocal) * dxdy + sp_globals.pixrnd) >> sp_globals.pixshift);
    }

/* Find Y intercept (16.16 local) with next vertical pixel boundary */
if (currentPixel.xOrg != xOrgLast)
    {
    yy  = 
        ((fix31)yLocal << shift) +
        (((xOut - xLocal) * dydx + sp_globals.pixrnd) >> sp_globals.pixshift);
    }

/* Set up Bresenham control variables */
if (ady >= adx)
    {
    d = (dx >= 0)? xx - 65536L: -xx;
    dv = -65536L;
    dh = (dxdy >= 0)? dxdy: -dxdy;
    }
else
    {
    d = (dy >= 0)? 65536L - yy: yy;
    dv = (dydx >= 0)? -dydx: dydx;
    dh = 65536L;
    }

/* Loop for each pixel intercepted */
while (TRUE)
    {
    if (currentPixel.xOrg == xOrgLast)
        {
        if (currentPixel.yOrg == yOrgLast)
            {
            currentPixel.area = 
                (fix31)((xend_spxl - currentPixel.xOrg) - currentPixel.xIn) *
                ((yend_spxl - currentPixel.yOrg) + currentPixel.yIn);
            break;
            }
        goto L2;
        }
    if (currentPixel.yOrg == yOrgLast)
        {
        goto L1;
        }

    if (d > 0)                  /* Next intercept is with v-boundary? */
        {
L1:     currentPixel.xOut = xOut;
        currentPixel.yOut = (yy + round) >> shift;
        currentPixel.edgeCodes |= vTypeOut;
        currentPixel.area += 
            (fix31)(currentPixel.xOut - xLocal) *
            (currentPixel.yOut + yLocal);
        AddIntercept(PARAMS1);
        currentPixel.xOrg += xOrgIncr;
        currentPixel.xIn = xIn;
        currentPixel.yIn = currentPixel.yOut;
        currentPixel.edgeCodes = vTypeIn;
        xx += xxv;
        yy += yyv;
        d += dv;
        }
    else if (d < 0)             /* Next intercept is with h-boundary? */
        {
L2:     currentPixel.xOut = (xx + round) >> shift;
        currentPixel.yOut = yOut;
        currentPixel.edgeCodes |= hTypeOut;
        currentPixel.area += 
            (fix31)(currentPixel.xOut - xLocal) *
            (currentPixel.yOut + yLocal);
        AddIntercept(PARAMS1);
        currentPixel.yOrg += yOrgIncr;
        currentPixel.xIn = currentPixel.xOut;
        currentPixel.yIn = yIn;
        currentPixel.edgeCodes = hTypeIn;
        xx += xxh;
        yy += yyh;
        d += dh;
        }
    else                        /* Next intercept is at a corner? */
        {                                                   
        currentPixel.xOut = xOut;
        currentPixel.yOut = yOut;
        currentPixel.edgeCodes |= hTypeOut;
        currentPixel.area += 
            (fix31)(currentPixel.xOut - xLocal) *
            (currentPixel.yOut + yLocal);
        AddIntercept(PARAMS1);
        currentPixel.xOrg += xOrgIncr;
        currentPixel.yOrg += yOrgIncr;
        currentPixel.xIn = xIn;
        currentPixel.yIn = yIn;
        currentPixel.edgeCodes = hTypeIn;
        xx += xxh + xxv;
        yy += yyh + yyv;
        d += dh + dv;
        }

    /* Set up for another iteration to handle a complete pixel intercept */
    currentPixel.area = 0;
    xLocal = currentPixel.xIn;
    yLocal = currentPixel.yIn;
    }
}


FUNCTION
void sp_end_contour_gray(SPGLOBPTR1)
/* 
 *  Called after the last vector in each contour
 *  Computes the area enclosed in the first pixel of the contour
 *  and adds it to the intercept list.
 */
{
#if DEBUG
printf("\nEND_CONTOUR_GRAY()\n");
#endif

if (currentPixel.edgeCodes >= 16) /* Never left initial pixel? */
    {
    currentPixel.edgeCodes = 0;
    }
else                            /* Back to initial pixel? */
    {
    currentPixel.xOut = startPixel.xOut;
    currentPixel.area += startPixel.area;
    currentPixel.edgeCodes |= 
        (startPixel.edgeCodes & 0x3);
    }

AddIntercept(PARAMS1);
}



FUNCTION
void sp_end_sub_char_gray(SPGLOBPTR1)
/*
 *  Called after the last contour in each sub-character in a compound character.
 */
{
#if DEBUG
printf("\nEND_SUB_CHAR_GRAY()\n");
#endif
}



FUNCTION
btsbool sp_end_char_gray(SPGLOBPTR1)
/* 
 *  Called when all character data has been output
 *  Return TRUE if output process is complete
 *  Return FALSE to repeat output of the transformed data beginning
 *  with the first contour
 *  Calls open_bitmap() to inititiate the pixmap output process.
 *  Calls ProcIntercepts() to output each band of intercepts
 *  Calls close_bitmap() to terminate the pixmap output process
 */
{
fix31 xorg;
fix31 yorg;
fix15 tmpfix15;

#if DEBUG
printf("\nEND_CHAR_GRAY()\n");
printf("Transformed character bounding box is %3.1f, %3.1f, %3.1f, %3.1f\n\n", 
    (real)sp_globals.bmap_xmin / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_ymin / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_xmax / (real)sp_globals.onepix, 
    (real)sp_globals.bmap_ymax / (real)sp_globals.onepix);
#endif

if (sp_globals.first_pass)
    {
    if (sp_globals.bmap_xmax >= sp_globals.bmap_xmin) /* Non-blank character? */
        {
        sp_globals.xmin = 
            sp_globals.bmap_xmin >> sp_globals.pixshift;
        sp_globals.ymin = 
            sp_globals.bmap_ymin >> sp_globals.pixshift;
        sp_globals.xmax = 
            (sp_globals.bmap_xmax >> sp_globals.pixshift) + 1;
        sp_globals.ymax = 
            (sp_globals.bmap_ymax >> sp_globals.pixshift) + 1;
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
    if (sp_globals.tcb.xmode == 0)  /* X pix is function of X orus only? */
        {
    	xorg += (fix31)sp_globals.rnd_xmin << sp_globals.poshift;
        }
    else if (sp_globals.tcb.xmode == 1) /* X pix is function of -X orus only? */
        {
      	xorg -= (fix31)sp_globals.rnd_xmin << sp_globals.poshift;
        }

    if (sp_globals.tcb.ymode == 2)  /* Y pix is function of X orus only? */
        {
    	yorg += (fix31)sp_globals.rnd_xmin << sp_globals.poshift;
        }
    else if (sp_globals.tcb.ymode == 3) /* Y pix is function of -X orus only? */
        {
      	yorg -= (fix31)sp_globals.rnd_xmin << sp_globals.poshift;
        }

    open_bitmap(
        sp_globals.set_width.x, 
        sp_globals.set_width.y, 
        xorg, 
        yorg,
        (fix15)(sp_globals.xmax - sp_globals.xmin), 
        (fix15)(sp_globals.ymax - sp_globals.ymin));

    if (sp_globals.intercept_oflo)  /* Intercept overflow on first pass? */
        {
        y_band.band_max = sp_globals.ymax;
        y_band.band_min = (sp_globals.ymin + sp_globals.ymax) >> 1;
        InitIntercepts(PARAMS1);
        sp_globals.first_pass = FALSE;
        sp_globals.extents_running = FALSE;
        return FALSE;
        }
    else                        /* No intercept overflow on first pass? */
        {
        /* Output accumulated bitmap data */
        ProcIntercepts(PARAMS1);  

        /* Close bitmap output */
        close_bitmap();

        return TRUE;            /* Flag successful completion */
        }
    }
else                            /* Second or subsequent pass? */
    {
    if (sp_globals.intercept_oflo)  /* Intercept overflow? */
        {
        y_band.band_min = 
            (y_band.band_min + 
             y_band.band_max) >> 1;  /* Halve band size */

        /* Init intercept storage for reduced band */
        InitIntercepts(PARAMS1);
        return FALSE;
        }

    /* Output accumulated bitmap data */
    ProcIntercepts(PARAMS1);

    /* Clean up if all bands completed */
    if (y_band.band_min <= sp_globals.ymin)
        {
        /* Close bitmap output */
        close_bitmap();

        return TRUE;            /* Flag successful completion */
        }

    /* Move down to the next band */
    tmpfix15 = y_band.band_max - y_band.band_min;
    y_band.band_max = y_band.band_min;
    y_band.band_min = y_band.band_max - tmpfix15;

    /* Truncate band if it extends beyond bottom of character */
    if (y_band.band_min < sp_globals.ymin)
        {
        y_band.band_min = sp_globals.ymin;
        }

    /* Initialize intercepts structure for the new band */
    InitIntercepts(PARAMS1);

    return FALSE;               /* Flag output incomplete */
    }

}

FUNCTION
static void InitIntercepts(SPGLOBPTR1)
/*
 *  Called to initialize the intercept data structure for the 
 *  currently defined band.
 */
{
fix15 y;

#if DEBUG
printf("\nInitIntercepts:\n"); 
printf("    X band from %d to %d\n", 
    x_band.band_min, x_band.band_max);
printf("    Y band from %d to %d\n", 
    y_band.band_min, y_band.band_max);
#endif 

sp_globals.no_y_lists = 
    (fix31)y_band.band_max - y_band.band_min;
sp_globals.intercept_oflo = 
    sp_globals.no_y_lists >= (fix31)MAX_INTERCEPTS_GRAY;
if (sp_globals.intercept_oflo)  /* List table won't fit? */
    {
    sp_globals.no_y_lists = (fix31)MAX_INTERCEPTS_GRAY;
	y_band.band_min = 
        y_band.band_max - MAX_INTERCEPTS_GRAY + 1;
    }

/* Clear intercept data structure */
for (y = 0; y < (fix15)sp_globals.no_y_lists; y++)
    {
    sp_intercepts.cdr[y] = 0;   /* Mark each intercept list empty */
    sp_intercepts.inttype[y] = 0; /* Initial winding number for scanline */
    }
sp_globals.next_offset = (fix15)sp_globals.no_y_lists;
}


FUNCTION
static void AddIntercept(SPGLOBPTR1)
/*
 *  Finishes the computation of the fraction of the pixel area enclosed 
 *  within the contour and the winding number flags.
 *  Adds the current pixel to the intercept data structure
 *  If the edge code indicates that the entry to the pixel has not
 *  yet occured (first pixel in contour), no action takes place.
 */
{
fix31   area;
fix15   dblPixHeightShift;
ufix8   flags;
fix15   shift;
fix31   round;
fix15   x, y;
btsbool anotherIntercept;
ufix8   byte;
fix15   from, to;

/* Check space for at least 2 intercepts */
if (sp_globals.next_offset >= (fix31)MAX_INTERCEPTS_GRAY - 2)
    {
    sp_globals.intercept_oflo = TRUE;
    return;
    }

x = currentPixel.xOrg >> sp_globals.pixshift;
y = currentPixel.yOrg >> sp_globals.pixshift;

/* Exit if pixel is above or below the current band */
if ((y < y_band.band_min) ||
    (y >= y_band.band_max))
    {
    return;
    }

dblPixHeightShift = 1 + sp_globals.pixshift;

switch(currentPixel.edgeCodes)
    {
case 0:         /* Bottom edge to bottom edge */
    area = currentPixel.area;
    if (area < 0)
        {
        flags = LEADING_EDGE + TRAILING_EDGE;
        area += doublePixelArea;
        }
    else
        {
        flags = 0;
        }
    break;

case 1:         /* Bottom edge to right edge */
    flags = TRAILING_EDGE;
    area = currentPixel.area;
    break;

case 2:         /* Bottom edge to top edge */
    flags = TRAILING_EDGE;
    area = 
        ((fix31)(sp_globals.onepix - currentPixel.xOut) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;
 
case 3:         /* Bottom edge to left edge */
    flags = TRAILING_EDGE;                               
    area = 
        currentPixel.area +
        doublePixelArea;
    break;

case 4:         /* Right edge to bottom edge */
    flags = LEADING_EDGE;
    area = 
        currentPixel.area +
         doublePixelArea;
    break;

case 5:         /* Right edge to right edge */
    area = currentPixel.area;
    if (area < 0)
        {
        flags = LEADING_EDGE + TRAILING_EDGE;
        area += doublePixelArea;
        }
    else
        {
        flags = 0;
        }
    break;

case 6:         /* Right edge to top edge */
    flags = 0;
    area = 
        ((fix31)(sp_globals.onepix - currentPixel.xOut) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;
 
case 7:         /* Right edge to left edge */
    flags = 0;
    area = 
        currentPixel.area +
        doublePixelArea;
    break;

case 8:         /* Top edge to bottom edge */
    flags = LEADING_EDGE;
    area = 
        ((fix31)(currentPixel.xIn) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;

case 9:         /* Top edge to right edge */
    flags = LEADING_EDGE + TRAILING_EDGE;
    area = 
        ((fix31)(currentPixel.xIn) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;

case 10:        /* Top edge to top edge */
    area = 
        ((fix31)(currentPixel.xIn - currentPixel.xOut) <<
         dblPixHeightShift) +
        currentPixel.area; 
    if (area < 0)
        {
        flags = LEADING_EDGE + TRAILING_EDGE;
        area += doublePixelArea;
        }
    else
        {
        flags = 0;
        }
    break;

case 11:        /* Top edge to left edge */
    flags = 0;
    area = 
        ((fix31)(currentPixel.xIn) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;

case 12:        /* Left edge to bottom edge */
    flags = LEADING_EDGE;
    area = currentPixel.area;
    break;

case 13:        /* Left edge to right edge */
    flags = LEADING_EDGE + TRAILING_EDGE;
    area = currentPixel.area;
    break;

case 14:        /* Left edge to top edge */
    flags = LEADING_EDGE + TRAILING_EDGE;
    area = 
        ((fix31)(sp_globals.onepix - currentPixel.xOut) << 
         dblPixHeightShift) +
        currentPixel.area; 
    break;

case 15:        /* Left edge to left edge */
    area = currentPixel.area;
    if (area < 0)
        {
        flags = LEADING_EDGE + TRAILING_EDGE;
        area += doublePixelArea;
        }
    else
        {
        flags = 0;
        }
    break;

default:        /* Undefined edge to any edge */
    startPixel = currentPixel;  /* Save pixel */
    return;
    }

/* Map the area into the range 0 to 64 */
shift = (sp_globals.pixshift << 1) - 5;
if (shift >= 0)
    {
    round = (1L << shift) >> 1;
    area = (area + round) >> shift;
    }
else
    {
    area =  area << -shift;
    }

/* Special handling for 0% or 100% coverage */
if (area <= 0)                  /* Pixel is pure white? */
    {
    if (flags == 0)
        {
        return;
        }
    byte = flags;
    anotherIntercept = FALSE;
    }
else if (area >= 64)            /* Pixel is pure black? */
    {
    switch(flags)
        {
    case 0:
        x -= 1;
        byte = TRAILING_EDGE;
        anotherIntercept = TRUE;
        break;

    case TRAILING_EDGE:
        x -= 1;
        byte = flags;
        anotherIntercept = FALSE;
        break;

    case LEADING_EDGE:
        x += 1;
        byte = flags;
        anotherIntercept = FALSE;
        break;

    default:
        return;
        }
    }
else                            /* Pixel is gray */
    {
    byte = flags + (ufix8)area;
    anotherIntercept = FALSE;
    }

L1:

#if DEBUG
    printf("Intercept added at (%3d, %3d), coverage = %5.3f, flags = %c%c %s\n",
        x, 
        y,
        (real)(byte & FRACT_AREA) / 64.0,
        (byte & LEADING_EDGE)? '1': '0',
        (byte & TRAILING_EDGE)? '1': '0',
        anotherIntercept? "and ...": "");
#endif

/***** Used for Emboldening *****/
/* Invert byte if contour directions are reversed */
/* if (sp_globals.reverseContour) */
/*    byte ^= 0xff; */

if (x < x_band.band_min)        /* Pixel is left of active area? */
    {
    from = y  - y_band.band_min; 
    switch (byte >> 6)
        {
    case 1:
        sp_intercepts.inttype[from]++;
        break;

    case 2:
        sp_intercepts.inttype[from]--;
        break;
        }
    }
else if (x < x_band.band_max)   /* Pixel is within active area? */
    {
    /* Insert intercept into the intercept structure */
    sp_intercepts.car[sp_globals.next_offset] = x;
    sp_intercepts.inttype[sp_globals.next_offset] = byte;
    for (
        from = y - y_band.band_min;
        (to = sp_intercepts.cdr[from]) > 0;
        from = to)
        {
        if (x <= sp_intercepts.car[to])
            {
            break;              /* Drop out of loop and insert here */
            }
        }

    /* Insert or append to end of intercept list */
    sp_intercepts.cdr[from] = sp_globals.next_offset;
    sp_intercepts.cdr[sp_globals.next_offset++] = to;
    }

/* Check if another intercept is pending */
if (anotherIntercept)
    {
    anotherIntercept = FALSE;
    x += 2;
    byte = LEADING_EDGE;
    goto L1;
    }
}

FUNCTION
static void ProcIntercepts(SPGLOBPTR1)
/*
 *  Called by sp_end_char_gray() to output accumulated intercept lists
 *  for the current band.
 */
{
fix15   yTop, yBottom;
fix15   y;
fix15   scanline;
fix15   ii;
fix15   windingNumber;
fix15   windingNumberAdj;
fix15   nGrays;
ufix8   buffer[GRAY_BUFF_SIZE];
fix15   outShift;
fix15   state;
fix15   fractArea;
ufix8   byte;
fix15   color;
fix15   x, new_x, xStart;

#if DEBUG
ShowIntercepts(PARAMS1);
#endif

/* Pick larger of bottom of band or bottom of bitmap */
yBottom = (y_band.band_min >= sp_globals.ymin)?
    y_band.band_min:
    sp_globals.ymin;

/* Pick smaller of top of band or top of bitmap */
yTop = (y_band.band_max >= sp_globals.ymax)?
    sp_globals.ymax:
    y_band.band_max;

/* Shift from 8-bit gray level to output value */
outShift = 8 - sp_globals.pixelSize;

/* Loop for each scanline */
for (y = yTop - 1; y >= yBottom; y--)
    {
    scanline = sp_globals.ymax - y - 1;
    ii = y - y_band.band_min;

    /* Start scanline processing */
    windingNumber = (fix15)((fix7)(sp_intercepts.inttype[ii]));
    state = (windingNumber != 0)? BLACK: WHITE;
    x = xStart = x_band.band_min;
    ii = sp_intercepts.cdr[ii];

    /* Loop for each intercepted X value */
    while(TRUE)
        {
        /* Test if end of intercept list */
        if (ii == 0)
            {
            if (windingNumber != 0) /* Trailing pixels are black? */
                {
                switch(state)
                    {
                case WHITE:     /* Black pixels after white */
                    xStart = x;
                    break;

                case GRAY:      /* Black pixels after gray */
                    if (nGrays > 0)
                        {
                        set_bitmap_pixels(
                            scanline,
                            xStart - sp_globals.xmin,
                            nGrays,
                            buffer);
                        }
                    xStart = x;
                    break;

                case BLACK:     /* More black pixels */
                    break;
                    }

                /* Paint remaining pixels (if any) black */
                if (x_band.band_max > xStart)
                    {
                    set_bitmap_bits(
                        scanline,
                        xStart - sp_globals.xmin,
                        x_band.band_max - sp_globals.xmin);
                    }
                }
            else                /* Trailing pixels are white? */
                {
                switch(state)
                    {
                case WHITE:     /* More white pixels */
                    break;

                case GRAY:      /* White pixels after gray */
                    if (nGrays > 0)
                        {
                        set_bitmap_pixels(
                            scanline,
                            xStart - sp_globals.xmin,
                            nGrays,
                            buffer);
                        }
                    break;

                case BLACK:     /* White pixels after black */
                    if (x > xStart)
                        {
                        set_bitmap_bits(
                            scanline,
                            xStart - sp_globals.xmin,
                            x - sp_globals.xmin);
                        }
                    break;
                    }
                }
            break;
            }

        /* Test for gap before next intercept */
        new_x = sp_intercepts.car[ii];
        if (new_x > x)
            {
            /* Process implied pixels */
            if (windingNumber != 0) /* Implied pixels are black? */
                {
                switch(state)
                    {
                case WHITE:     /* Black pixels after white */
                    state = BLACK;
                    xStart = x;
                    break;

                case GRAY:      /* Black pixels after gray */
                    if (nGrays > 0)
                        {
                        set_bitmap_pixels(
                            scanline,
                            xStart - sp_globals.xmin,
                            nGrays,
                            buffer);
                        nGrays = 0;
                        }
                    state = BLACK;
                    xStart = x;
                    break;

                case BLACK:     /* More black pixels */
                    break;
                    }
                }
            else                /* Implied pixels are white? */
                {
                switch(state)
                    {
                case WHITE:     /* More white pixels */
                    break;

                case GRAY:      /* White pixels after gray */
                    if (nGrays > 0)
                        {
                        set_bitmap_pixels(
                            scanline,
                            xStart - sp_globals.xmin,
                            nGrays,
                            buffer);
                        nGrays = 0;
                        }
                    break;

                case BLACK:     /* White pixels after black */
                    set_bitmap_bits(
                        scanline,
                        xStart - sp_globals.xmin,
                        x - sp_globals.xmin);
                    break;
                    }
                state = WHITE;
                }
            }
        x = new_x;

        /* Accumulate color from one or more coincident intercepts */
        fractArea = 0;
        windingNumberAdj = 0;
        while (TRUE) 
            {
            byte = sp_intercepts.inttype[ii];
            if (byte & LEADING_EDGE)
                {
                windingNumber--;
                }
            fractArea += byte & FRACT_AREA;
            if (byte & TRAILING_EDGE)
                {
                windingNumberAdj++;
                }

            /* Step to next intercept */
            ii = sp_intercepts.cdr[ii];
            if (ii == 0)        /* No more intercepts? */
                {
                break;
                }
            new_x = sp_intercepts.car[ii];
            if (new_x != x)     /* Different X value? */
                {
                break;
                }
            }

        /* Convert to pixel color */
        color = (windingNumber << 6) + fractArea;
        if (color < 0)
            color = -color;

        /* Update winding number */
        windingNumber += windingNumberAdj;

        /* Process intercepted pixel group */
        if (color == 0)         /* White pixel? */
            {
            switch(state)
                {
            case WHITE:         /* Another white pixel */
                break;

            case GRAY:          /* First white pixel after gray */
                if (nGrays > 0)
                    {
                    set_bitmap_pixels(
                        scanline,
                        xStart - sp_globals.xmin,
                        nGrays,
                        buffer);
                    nGrays = 0;
                    }
                break;

            case BLACK:         /* First white pixel after black */
                set_bitmap_bits(
                    scanline,
                    xStart - sp_globals.xmin,
                    x - sp_globals.xmin);
                break;
                }
            state = WHITE;
            }
        else if (color < 64)    /* Gray pixel */
            {
            switch(state)
                {
            case WHITE:         /* First gray pixel after white */
                if ((windingNumber == 0) &&
                    ((ii == 0) || (new_x > (x + 1))) &&
                    (color < (MIN_GRAY >> 2)))
                    {
                    color = (MIN_GRAY >> 2);
                    }
                xStart = x;
                nGrays = 0;
                break;

            case GRAY:          /* Another gray pixel */
                break;

            case BLACK:         /* First gray pixel after black */
                set_bitmap_bits(
                    scanline,
                    xStart - sp_globals.xmin,
                    x - sp_globals.xmin);
                xStart = x;
                nGrays = 0;
                break;
                }

            /* Add gray pixel to buffer; flush if full */
            buffer[nGrays++] = gammaTable[color] >> outShift;
            if (nGrays >= GRAY_BUFF_SIZE)
                {
                set_bitmap_pixels(
                    scanline,
                    xStart - sp_globals.xmin,
                    nGrays,
                    buffer);
                nGrays = 0;
                xStart = x + 1;
                }
            state = GRAY;
            }
        else                    /* Black pixel? */
            {
            switch(state)
                {
            case WHITE:         /* First black pixel after white */
                state = BLACK;
                xStart = x;
                break;

            case GRAY:          /* First black pixel after gray */
                if (nGrays > 0)
                    {
                    set_bitmap_pixels(
                        scanline,
                        xStart - sp_globals.xmin,
                        nGrays,
                        buffer);
                    nGrays = 0;
                    }
                state = BLACK;
                xStart = x;
                break;

            case BLACK:         /* Another black pixel */
                break;
                }
            }
        x++;
        }
    }
}

#if DEBUG

FUNCTION
static void ShowIntercepts(SPGLOBPTR1)
{
fix15   yTop, yBottom;
fix15   x, y;
fix15   ii, jj;
ufix8   byte;

/* Pick larger of bottom of band or bottom of bitmap */
yBottom = (y_band.band_min >= sp_globals.ymin)?
    y_band.band_min:
    sp_globals.ymin;

/* Pick smaller of top of band or top of bitmap */
yTop = (y_band.band_max >= sp_globals.ymax)?
    sp_globals.ymax:
    y_band.band_max;

/* Loop for each scanline */
for (y = yTop - 1; y >= yBottom; y--)
    {
    ii = y - y_band.band_min;
    printf("%5hd: %2d| ", 
        y, (int)((fix7)sp_intercepts.inttype[ii]));
    for (
    	jj = sp_intercepts.cdr[ii];
        jj != 0;
        jj = sp_intercepts.cdr[jj])
        {
        x = sp_intercepts.car[jj];
        byte = sp_intercepts.inttype[jj];
        printf("%3d: %c %2hd %c| ",
            x,
            byte & LEADING_EDGE? '-': ' ',
            byte & FRACT_AREA,
            byte & TRAILING_EDGE? '+': ' ');
        }
    printf("\n");
    }
}

#endif  /* DEBUG */

#endif  /* INCL_GRAY */

