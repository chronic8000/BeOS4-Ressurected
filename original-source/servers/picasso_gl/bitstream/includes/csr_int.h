/*****************************************************************************
*                                                                            *
*                          Copyright 1993 - 96                               *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/**************************** C S R _  I N T . H *****************************
 *                                                                           *
 * Character shape recorder internal header file                             *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1)  2 Nov 93  jsc  Created                                               *
 *                                                                           *
 *     $Header: R:/src/CSR/rcs/CSR_INT.H 6.10 1997/03/18 15:59:45 SHAWN release $
 *                                                                                    
 *     $Log: CSR_INT.H $
 *     Revision 6.10  1997/03/18 15:59:45  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 6.9  1997/02/10 22:34:00  JohnC
 *     Added definitions required for support of glyph mutation.
 *     Revision 6.8  1997/01/13 22:48:18  JohnC
 *     Moved public compile-time options from here to csr_api.h
 *     Revision 6.7  1996/12/27 15:59:45  john
 *     Added contourType to physicalFont_t struct.
 *     Changed prototype for CsrFlattenContourTree.
 * Revision 6.6  96/12/20  15:18:18  john
 * Default structure alignment changed from 2-byte to 4-byte boundaries.
 * 
 * Revision 6.5  96/12/13  15:42:31  john
 * Updates in connection with compressed glyph program string encoding.
 * 
 * Revision 6.4  96/11/20  09:58:53  john
 * Added gps offset arg to ReadGlyphElement function pointer in pfrTypeFns_t.
 * 
 * Revision 6.3  96/11/15  17:12:12  john
 * Changed access to gps functions to allow multiple versions.
 * 
 * Revision 6.2  96/11/14  18:05:20  john
 * Added global pfrType to control PFR compilation format.
 * 
 * Revision 6.1  96/10/10  14:08:46  mark
 * Release
 * 
 * Revision 5.2  96/10/02  15:41:54  john
 * Added defs for MAX_PFR_SIZE and MAX_FONTS.
 * 
 * Revision 5.1  96/08/27  11:51:40  mark
 * Release
 * 
 * Revision 4.4  96/07/03  10:40:21  mark
 * change boolean to btsbool and NULL to BTSNULL
 * 
 * Revision 4.3  96/06/13  17:18:33  john
 * Updates for stroke weight histogram analyser.
 * 
 * Revision 4.2  96/06/03  10:15:36  john
 * Added definitions for stem snap tables.
 * 
 * Revision 4.1  96/03/14  14:15:23  mark
 * Release
 * 
 * Revision 3.2  96/03/14  14:13:39  mark
 * rename PCONTEXT to PCSPCONTEXT 
 * 
 * Revision 3.1  96/03/05  13:51:25  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:50:32  mark
 * Release
 * 
 * Revision 1.6  95/11/22  15:53:59  john
 * Changed GLYPH_SIGNATURE_SIZE from 10 to 8.
 * 
 * Revision 1.5  95/11/21  09:32:42  john
 * Added character recording options struct to globals.
 * Modified CsrMatchSimpleGlyphProgStrings() proto to new spec.
 * 
 * Revision 1.4  95/11/07  14:24:46  john
 * Changes related to implementation of secondary strokes
 * and secondary edges.
 * 
 * Revision 1.3  95/09/08  14:36:50  john
 * Default MAX_EDGES increased from 24 to 30 to prevent
 * loss of main horizontal round stroke in Garamond m.
 * 
 * Revision 1.2  95/09/01  17:51:12  john
 * Added curve fitting tolerance value to physical
 *     font record.
 * 
 * Revision 1.1  95/08/10  18:21:42  john
 * Initial revision
 * 
 * Revision 10.1  95/07/31  10:25:02  mark
 * Release
 * 
 * Revision 9.2  95/05/17  15:33:21  john
 * Added fileSize global to prevent writing beyond
 *     end of temp file.
 * 
 * Revision 9.1  95/04/26  15:48:50  mark
 * Release
 * 
 * Revision 8.2  95/03/28  16:15:16  john
 * blueFuzz moved from global to physical font record.
 * blueScale added to physical font record.
 * maxBlueZoneSize eliminated as a global variable.
 * 
 * Revision 8.1  95/03/13  15:48:54  mark
 * Release
 * 
 * Revision 7.9  95/03/10  17:50:53  john
 * Added a global pointer to maintain lowest address
 *     used by character data in memory modes 2 and 3.
 * 
 * Revision 7.8  95/02/24  11:33:51  john
 * Changes associated with renamed bmapSpecs struct and incorporation
 *     of bitmap format specs into it.
 * 
 * Revision 7.7  95/02/07  17:01:29  john
 * Added definitions for bitmap recording.
 * 
 * Revision 7.6  94/12/27  13:09:28  john
 * nCharacters field of physical font record changed from
 *     ufix16 to fix31 to allow 65536 characters in font.
 * 
 * Revision 7.5  94/12/22  16:09:34  john
 * maxVertStrokes and maxHorizStrokes removed from global
 *     data structure.
 * 
 * Revision 7.4  94/12/22  16:01:46  john
 * Type of nCharacters element of physical font record
 *     changed from fix15 to ufix16 to accomodate more
 *     than 32K characters in a physical font.
 * 
 * Revision 7.3  94/09/29  09:41:16  mark
 * move include of csr_opt.h to csr_api.h, since it REENTRANT can
 * affect the API
 * 
 * Revision 7.2  94/09/09  15:54:26  john
 * Maximum scale factor for components of compound characters
 *     default value changed from 2 to 1. It is not safe to
 *     use values greater than unity because of the possibility
 *     of fixed point arithmetic overflow in the player.
 * 
 * Revision 7.1  94/08/26  16:17:33  mark
 * Release
 * 
 * Revision 6.1  94/08/25  14:10:00  mark
 * Release
 * 
 * Revision 4.6  94/08/08  17:47:15  john
 * Curve fitting tolerance tightened from .005 em to .004 em.
 * Curve fitting iteration limit reduced from 5 to 4.
 * polyPoint_t struct extended to accomodate new curve fitting data.
 * 
 * Revision 4.5  94/07/21  14:46:21  mark
 * context argument is now only passed to ExecChar callback function.
 * 
 * Revision 4.4  94/07/14  16:16:14  john
 * Eliminated the following structure definitions:
 *     readState_t
 *     keyPoint_t
 *     edgeState_t
 *     miscDetectState_t
 * Modified the contour_t struct to replace in-line edge arrays
 *     with pointers.
 * Eliminated the following function protos:
 *     CsrInitEdgeDetect
 *     CsrDoEdgeDetect
 * Added a function proto for CsrConsEdgeList
 * 
 * Revision 4.3  94/07/11  13:54:46  john
 * Added memory manager pointers to globals data structure.
 * CsrInitAlloc() replaced by CsrInitBuffer().
 * 
 * Revision 4.2  94/07/08  16:31:25  mark
 * change calling sequences for REENTRANT option.
 * 
 * Revision 4.1  94/07/05  09:56:02  mark
 * Release
 * 
 * Revision 3.1  94/06/20  09:00:59  mark
 * Release
 * 
 * Revision 2.15  94/06/14  16:19:25  john
 * Previous log message corrected.
 * 
 * Revision 2.14  94/06/14  16:17:16  john
 * TEMP_BLOCK_SIZE default changed from 256 to 1024.
 * 
 * Revision 2.13  94/06/14  09:23:22  john
 * fontStyle changed from ufix16 to fix15.
 * 
 * Revision 2.12  94/06/06  17:22:08  john
 * Default BLUE_FUZZ changed from 0.001 to 0.002.
 * 
 * Revision 2.11  94/06/06  09:55:17  john
 * Added BASELINE_FUZZ constant.
 * 
 * Revision 2.10  94/06/03  17:56:07  john
 * Increased default value of MAX_EDGES from 20 to 24.
 * 
 * Revision 2.9  94/05/27  14:58:34  john
 * Updates to support extendable font style support.
 * 
 * Revision 2.8  94/05/27  12:55:28  john
 * STD_MITER_LIMIT added to list of compile-time constants.
 * 
 * Revision 2.7  94/05/26  16:30:31  john
 * UP_IN and UP_OUT changed to RIGHT_IN and RIGHT_OUT.
 * 
 * Revision 2.6  94/05/20  17:45:23  john
 * Function proto for CsrConfirmCornerType replaced by function
 *     proto for CsrCornerType.
 * 
 * Revision 2.5  94/05/12  15:18:09  john
 * Function proto for CsrFlattenGlyphTree() added.
 * 
 * Revision 2.4  94/05/12  12:14:20  john
 * Changed default MAX_ORUS from 16 to 20.
 * Changed default MAX_CHAR_ELEMENTS from 10 to 5.
 * 
 * Revision 2.3  94/05/11  14:01:11  john
 * Compile-time definitions and a new global variable added in
 *     support of the updated curve-fitting module.
 * 
 * Revision 2.2  94/05/05  14:33:14  john
 * MAX_SCALE_FACTOR added as compile-time constant.
 * 
 * Revision 2.1  94/04/25  14:44:16  mark
 * Release
 * 
 * Revision 1.13  94/04/15  14:57:37  john
 * Implemented memory management levels 2 and 3.
 * 
 * Revision 1.12  94/04/07  10:41:38  john
 * Structure definitions for physical and logical font records updated.
 * Function prototype updates associated with restructuring required
 *     to support memory management levels 2 and 3.
 * 
 * Revision 1.11  94/03/29  17:09:49  john
 * Additions to support call sequence checking.
 * Function prototype changes for updates to memory
 *     manager and error return values.
 * 
 * Revision 1.10  94/03/21  10:44:59  john
 * Modifications to support memory management mode 1
 * 
 * Revision 1.9  94/03/15  15:54:51  john
 * pGlyphRecord removed from character_t structure.
 * CsrConsGlyph() args and return value changed.
 * 
 * Revision 1.8  94/03/15  10:22:14  john
 * Added modeFlags to csrGlobals structure to support
 *     memory management modes and accelerated
 *     blue value collection.
 * 
 * Revision 1.7  94/02/22  16:39:54  john
 * New field, pP0, added to poly processing state structure.
 * 
 * Revision 1.6  94/02/16  17:24:16  john
 * CsrConfirmCornerType() changed from static to global.
 *     Function prototype added.
 * 
 * Revision 1.5  94/02/10  15:40:48  john
 * polyProcState structure changed to support new
 * corner detection logic.
 * 
 * Revision 1.4  94/02/03  16:52:15  john
 * Added stdHW to physical font structure.
 * 
 * Revision 1.3  94/01/26  13:49:17  mark
 * add RCS tags
 * 
 * 
 *                                                                           *
 ****************************************************************************/

#define CSR_INT

#include "csr_api.h"

/***** Default values for private overridable compile-time settings *****/

/*
 *	Max size of Portable Font Resource
 *	This is limited to the largest unsigned number representable by 3 bytes
 */
#ifndef MAX_PFR_SIZE
#define MAX_PFR_SIZE   16777215
#endif

/*
 *	Max number of fonts in Portable Font Resource
 */
#ifndef MAX_FONTS
#define MAX_FONTS   	  32767
#endif

/*
 *  Max number of edges per dimension in any contour
 *  Allowed values are 0 to 63 
 */
#ifndef MAX_EDGES
#define MAX_EDGES            30
#endif

/*
 *  Max number of controlled coordinates in either dimension
 *  Allowed values are 0 to 63 
 */
#ifndef MAX_ORUS
#define MAX_ORUS             20
#endif

/*
 *  Max number of stemsnap values in either dimension
 *  Allowed values are 0 to 12 
 */
#ifndef MAX_STEMSNAPS
#define MAX_STEMSNAPS        12
#endif

/*
 *	Minimum gap between adjacent stem snap values
 *	Units are 1/65536 em
 *	Reasonable values are 100 to 350
 */
#ifndef MIN_STEM_SNAP_GAP
#define MIN_STEM_SNAP_GAP  262
#endif

/*
 *  Min number of elements for compound character encoding
 *  Allowed values are 2 to MAX_CHAR_ELEMENTS - 1 
 */
#ifndef MIN_CHAR_ELEMENTS
#define MIN_CHAR_ELEMENTS     2   /* Min elements in compound char */
#endif

/*
 *  Max number of elements for compound character encoding
 *  Allowed values are MIN_CHAR_ELEMENTS + 1 to 100
 *  Note that each unit imcrement requires an additional 
 *	24 bytes of stack space and 36 bytes of work buffer
 *	
 */
#ifndef MAX_CHAR_ELEMENTS
#define MAX_CHAR_ELEMENTS     5   /* Max elements in compound char */
#endif

/*
 *  Maximum amount by which elements of compound characters may be scaled 
 *  Units are 1/65536
 *  Values much larger than 65536 may cause arithmetic overflow in CSP
 *  Values less than 65536 waste compounding opportunities
 */
#ifndef MAX_SCALE_FACTOR
#define MAX_SCALE_FACTOR  65536
#endif

/*
 *  Memory allocation block size for character shape processing
 *  Units are bytes
 */
#ifndef POLY_BLOCK_SIZE
#define POLY_BLOCK_SIZE      10
#endif

/*
 *  Memory allocation block size for adjustment vector accumulation
 *  Units are bytes
 *	Reasonable values are from 10 to 100 (must be at least 10)
 */
#if INCL_COMPRESSED_PFR
#ifndef ADJ_VECTOR_BLOCK_SIZE
#define ADJ_VECTOR_BLOCK_SIZE      10
#endif
#endif
/*
 *  Block size in bytes for temporary file management 
 *  Should be power of 2 in range 256 to 4096
 */
#ifndef TEMP_BLOCK_SIZE
#define TEMP_BLOCK_SIZE    1024
#endif

/*
 *  Minimum thickness for strokes used in hinting
 *  Units are 1/65536 em
 *  Reasonable values are in the range 300 to 1000
 */
#ifndef MIN_STROKE_THICKNESS
#define MIN_STROKE_THICKNESS 655
#endif

/*
 *  Minimum deviation for a secondary edge
 *  Units are 1/65536 em
 *  Reasonable values are in the range 0 to 260
 */
#ifndef MIN_SECONDARY_EDGE_DEV
#define MIN_SECONDARY_EDGE_DEV 131
#endif

/*
 *  Maximum deviation for a secondary edge
 *  Units are 1/65536 em
 *  Reasonable values are in the range 200 to 1000
 */
#ifndef MAX_SECONDARY_EDGE_DEV
#define MAX_SECONDARY_EDGE_DEV 655
#endif

/*
 *  Overshoot suppression limit
 *  This is the rendering size in pixels per em above which 
 *  overshoot suppression is turned off
 */
#ifndef BLUE_SCALE
#define BLUE_SCALE           44
#endif

/*
 *  Blue zone tolerance
 *  Horizontal stroke edges that are within this distance of a
 *  blue zone are treated as if they were within the blue zone.
 *  Units are 1/65536 em
 *  Reasonable values are in the range 0 to 250
 */
#ifndef BLUE_FUZZ
#define BLUE_FUZZ           132
#endif

/*
 *  Baseline tolerance
 *  Horizontal stroke edges that are within this distance of the
 *  baseline are treated as if they were on the baseline
 *  Units are 1/65536 em
 *  Reasonable values are in the range 0 to 500
 */
#ifndef BASELINE_FUZZ
#define BASELINE_FUZZ       350
#endif

/*
 *  Curve fitting tolerance
 *  This is the tolerance used for the curve fitting process
 *  Units are 1/65536 em
 *  Reasonable values are in the range 50 to 500
 */
#ifndef CURVE_TOLERANCE
#define CURVE_TOLERANCE     262
#endif

/*
 *  Curve fitting iteration limit
 *  This is the maximum number of curve-fitting iterations permitted 
 *  for any curve segment.
 *  Reasonable values are 1 to 10
 */
#ifndef CURVE_ITERATION_LIMIT
#define CURVE_ITERATION_LIMIT 4
#endif

/*
 *  Default miter limit
 *  Units are 1/65536
 *  Reasonable values are in the range 65536 (no mitering) to 
 *  655360 (angles >= 11 degrees are mitered)
 */
#ifndef STD_MITER_LIMIT
#define STD_MITER_LIMIT  131072
#endif

/* 
 *  PFR_BLACK_PIXEL defines the bit value used to represent a black pixel 
 *  in the bitmap image stored in the PFR.
 *  Allowed values are:
 *      0: Bit = 0 represents black pixel
 *      1: Bit = 1 represents black pixel
 */
#ifndef PFR_BLACK_PIXEL
#define PFR_BLACK_PIXEL 1
#endif

/* 
 *  PFR_INVERT_BITMAP defines the order of the rows in the bitmap image 
 *  stored in the PFR.
 *  Allowed values are:
 *      0: Rows stored in increasing order of Y value
 *      1: Rows stored in decreasing order of Y value
 */
#ifndef PFR_INVERT_BITMAP
#define PFR_INVERT_BITMAP 0
#endif


/***** Other compile-time constants and macro definitions *****/
#define GLYPH_SIGNATURE_SIZE  8

#define MAX_BLUE_VALUES       8

#include "btstypes.h"

#define ABS(X) (((X) >= 0)? (X): -(X))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


/***** Global structure definitions *****/

/* Union of memory pointer and temp file stream offset */
typedef union vmPtr_tag
    {
    void   *ptr;
    fix31   offset;
    } vmPtr_t;

/* Coordinate matching transformation */
typedef struct matchTrans_tag
    {
    fix31   scale; 
    fix31   position;
    fix31   tolerance;
    } matchTrans_t;

/* Compound character element record */
typedef struct charElement_tag
    {
    fix31   xScale;
    fix31   xPosition;
    fix31   yScale;
    fix31   yPosition;
    fix31   glyphProgStringOffset;
    ufix16  glyphProgStringSize;
    ufix8   matchSpec;
    } charElement_t;

/* Range structure for outline resolution units */
typedef struct oruRange_tag
    {
    fix15   nom;
    ufix8   deltaLo;
    ufix8   deltaHi;
    } oruRange_t;

/* Glyph program string execution context */
typedef struct gpsExecContext_tag
    {
    fix15   nTotalXorus;
    fix15   nBasicXorus;
    oruRange_t xOruTable[MAX_ORUS];
    fix15   nTotalYorus;
    fix15   nBasicYorus;
    oruRange_t yOruTable[MAX_ORUS];
    fix15   xPrevValue;
    fix15   yPrevValue;
#if INCL_COMPRESSED_PFR
    fix15   xPrev2Value;
    fix15   yPrev2Value;
#endif    
    } gpsExecContext_t;

/* Character shape buffer point record */
typedef struct polyPoint_tag
    {
    fix15   x;
    fix15   y;
    ufix16  props;
    fix15   xFit;
    fix15   yFit;
    ufix16  t;
    } polyPoint_t;

/* Character shape buffer point properties */
#define CTRL_POINT    0x01
#define LEFT_CURVE    0x02
#define RIGHT_CURVE   0x04

/* Segment division point record */
typedef struct segPoint_tag
    {
    polyPoint_t *pP;
    fix15   dxIn;
    fix15   dyIn;
    fix15   dxOut;
    fix15   dyOut;
    ufix16  pointProps;
    } segPoint_t;

/* Polygon processing state */
typedef struct polyProcState_tag
    {
    polyPoint_t *pFirstPoint;   /* First point in polygon buffer */
    fix15   buffSize;           /* Current size of polygon buffer */
    fix15   nPoints;            /* Total number of points in buffer */
    fix15   nSavedPoints;       /* Number of wraparound points in buffer */
    fix15   pipeNeeds;          /* Number of points needed to fill pipeline */
    polyPoint_t *pP0;           /* Possible pre-pipeline point  */
    polyPoint_t *pP1;           /* First point in three-point pipeline */
    polyPoint_t *pP2;           /* Second point in three-point pipeline */
    segPoint_t segStart;        /* Start of current segment */
    } polyProcState_t;

/* Contour point and segment division point properties */
#define CRVDEPTH           0x000f /* Curve depth */
#define ONCURVE            0x0000 /* On-curve point */
#define OFFCURVE           0x0010 /* Off-curve (control) point */
#define TANGENT            0x0020 /* Tangent point */
#define INFLECTION         0x0040 /* Inflection point */
#define CURVED_EXTREME     0x0080 /* Curved extreme point */
#define CORNER             0x0300 /* Corner point */
#define LEFT_CORNER        0x0100 /* Left corner */
#define RIGHT_CORNER       0x0200 /* Right corner */
#define RIGHT_IN           0x0400 /* dxIn >= 0 */
#define RIGHT_OUT          0x0800 /* dxOut >= 0 */
#define HORIZ_EDGE_IN      0x1000 /* Horizontal edge end point */
#define HORIZ_EDGE_OUT     0x2000 /* Horizontal edge start point */
#define VERT_EDGE_IN       0x4000 /* Horizontal edge end point */
#define VERT_EDGE_OUT      0x8000 /* Horizontal edge start point */

/* Bounding box structure */
typedef struct bbox_tag
    {
    fix15   xmin;
    fix15   ymin;
    fix15   xmax;
    fix15   ymax;
    } bbox_t;

/* Contour point record */
typedef struct contourPoint_tag
    {
    fix15   x;
    fix15   y;
    ufix16  pointProps;
    } contourPoint_t;

/* Edge record */
typedef struct edge_tag
    {
    ufix16  edgeProps;
    ufix16  edgeID;
    fix15   value;
    fix15   position;
    fix15   start;
    fix15   end;
    } edge_t;

/* Edge properties */
#define LEFT_EDGE      0x01
#define RIGHT_EDGE     0x02
#define BOTTOM_EDGE    0x04
#define TOP_EDGE       0x08
#define CURVED_EDGE    0x10
#define LEADING_EDGE   0x20
#define USED_EDGE      0x40
#define PRIMARY_EDGE   0x80

/* Stroke record */
typedef struct stroke_tag
    {
    oruRange_t leadingEdge;
    oruRange_t trailingEdge;
    fix15   score;
    } stroke_t;

/* Contour record */
typedef struct contour_tag
    {
    struct contour_tag *pSiblings;
    struct contour_tag *pDaughters;
    fix15   nPoints;
    contourPoint_t *pContourPoints;
    fix15   nOutsideCorners;
    fix15   nInsideCorners;
    fix15   outerStartPoint;
    fix15   innerStartPoint;
    bbox_t  contourBBox;
    ufix16  contourProps;
    fix15   nVertEdges;
    edge_t *pVertEdges;
    fix15   nHorizEdges;
    edge_t *pHorizEdges;
    } contour_t;

/* Contour props bit assignments */
#define CTACTCCW 1 /* Actual contour direction is counter-clockwise */
#define CTDESCCW 2 /* Desired contour direction is counter-clockwise */

/* Glyph record */
typedef struct glyph_tag
    {
    struct glyph_tag *pNextDown;
    struct glyph_tag *pNextEqual;
    struct glyph_tag *pNextUp;
    ufix16  glyphProps;
    bbox_t  glyphBBox;
    ufix8   signature[GLYPH_SIGNATURE_SIZE];
    ufix16  glyphProgStringSize;
    fix31   glyphProgStringOffset;
    } glyph_t;

/* Glyph properties */
#define GLFCHARTYPE   0x01
#define GLFSIMPLE     0x00
#define GLFCOMPOUND   0x01

/* Character record */
typedef struct character_tag
    {
    struct  character_tag *pNextDown;
    struct  character_tag *pNextUp;
    ufix16  charCode;
    ufix8   asciiCode;
    fix15   setWidth;
    ufix16  glyphProgStringSize;
    fix31   glyphProgStringOffset;
    } character_t;

/* Bitmap character record */
typedef struct bmapChar_tag
    {
    struct  bmapChar_tag *pNextDown;
    struct  bmapChar_tag *pNextUp;
    ufix16  charCode;
    ufix16  bmapGpsSize;
    fix31   bmapGpsOffset;
    } bmapChar_t;

/* Bitmap character size record */
typedef struct bmapSize_tag
    {
    struct  bmapSize_tag *pNextBmapSize;
    ufix32  ppm;
    btsbool active;
    ufix8   bmapSizeProps;
    ufix16  nBmapChars;
    bmapChar_t *pBmapChars;
    fix31   bmapCharsOffset;
    } bmapSize_t;

/* Bitmap character size record props bit assignments */
#define BCSBIGCCD  0x10     /* Two-byte character codes */
#define BCSBIGSIZ  0x20     /* Two-byte glyph prog string sizes */
#define BCSBIGOFF  0x40     /* Three-byte glyph prog string offsets */

/* Bitmap character header record */
typedef struct bmapInfo_tag
    {
    int (*EnableBitmaps)(
        char *pFontID,
        fontAttributes_t *pFontAttributes
        USERPARAM);
    int (*ExecCharBitmap)(
        PCSRCONTEXT
        void *pCharID,
        unsigned short xppm,
        unsigned short yppm
        USERPARAM);
    fix15   nBmapResValues;
    fix31   bmapResValues[MAX_BMAP_RES_VALUES];
    fix15   nBmapSizes;
    bmapSize_t *pRootBmapSize;
    } bmapInfo_t;

/* Physical font record */
typedef struct physicalFont_tag
    {
    ufix16  fontRefNumber;
    fix15   nFontIDbytes;
    vmPtr_t pFontID;
    fix31   nAuxBytes;
    vmPtr_t pAuxData;
    struct  physicalFont_tag *pNextPhysicalFont;
    struct  logicalFont_tag *pRootLogicalFont;
    fix15   outlineResolution;
    fix15   metricsResolution;
    fix15   standardSetWidth;
    ufix16  physicalFontProps;
    bbox_t  fontBBox;
    fix15   nBlueValues;
    fix15   blueValues[MAX_BLUE_VALUES];
    ufix8   blueFuzz;
    ufix8   blueScale;
    fix15   stdVW;
    fix15   stdHW;
    fix15   nStemSnapsV;
    fix15   stemSnapsV[MAX_STEMSNAPS];
    fix15   nStemSnapsH;
    fix15   stemSnapsH[MAX_STEMSNAPS];
    fix31   nCharacters;
    character_t *pRootCharacter;
    fix31   physicalFontSize;
    fix31   physicalFontOffset;
    fix31   charDataSize;
    glyph_t *pRootGlyph;
    vmPtr_t pBmapInfo;
    fix15   curveTolerance; /* Curve tolerance in 1/65536 em units */
    fix15	contourType;
    } physicalFont_t;

/* Physical font props bit assignments */
#define PFTESCTYP  0x0003   /* Escapement direction */
#define PFTESCUDF  0x0000   /* Escapement direction undefined */
#define PFTESCHOR  0x0001   /* Horizontal escapement established */
#define PFTESCVER  0x0002   /* Vertical escapement established */
#define PFTWIDVAR  0x0004   /* Variable width font */
#define PFTBIGCCD  0x0010   /* Two-byte character codes */
#define PFTBIGSIZ  0x0020   /* Two-byte glyph prog string sizes */
#define PFTBIGOFF  0x0040   /* Three-byte glyph prog string offsets */
#define PFTBBOXOK  0x0080   /* Font bounding box data valid */
#define PFTASCNZ   0x0100   /* Font contains non-zero ascii codes */
#define PFTASCNCC  0x0200   /* Font contains ascii codes != char codes */
#define PFTDIRTY   0x0400   /* Char/glyph/Bmap data has been updated */
#define PFTAUXTMP  0x0800   /* Aux data stored in temporary file */
#define PFTSSVSET  0x1000   /* StemSnapV table set from fontInfo */
#define PFTSSHSET  0x2000   /* StemSnapH table set from fontInfo */

/* Logical font record */
typedef struct logicalFont_tag
    {
    ufix16  logicalFontNumber;
    ufix16  physicalFontNumber;
    struct  logicalFont_tag *pNextLogicalFont;
    fix31   fontMatrix[4];
    fix15   fontStyle;
    styleSpecs_t styleSpecs;
    btsbool bmapInputActive;
    } logicalFont_t;

/* Portable font resource map */
typedef struct pfrMap_tag
    {
    fix31   headerOffset;
    fix31   logicalFontDirOffset;
    fix31   firstLogicalFontOffset;
    fix31   firstPhysicalFontOffset;
    fix31   firstGpsOffset;
    fix31   trailerOffset;
    fix31   size;
    fix31   maxLftSize;
    fix31   maxPftSize;
    fix31   maxBctSize;
    fix31   maxBctSetSize;
    fix31   maxPftBctSetSize;
    fix15   maxStemSnapVsize;
    fix15   maxStemSnapHsize;
    ufix16	maxChars;
    } pfrMap_t;

/* Temporary file access state */
typedef struct tempFileState_tag
    {
    fix31   fileSize;           /* Current size of temp file */
    fix15   nextFreeBlock;      /* Next free block in temp file */
    fix15   bytesPerBlock;      /* Number of bytes per block */
    fix15   blockShift;         /* Shift right to access temp file block */
    fix15   byteMask;           /* Mask to extract byte offset within block */
    fix15   currentBlock;       /* Current block in temp file I/O buffer */
    btsbool updateFlag;         /* Current block needs to be written */
    } tempFileState_t;

/* Glyph program string management functions */
typedef struct pfrTypeFns_tag
	{
	btsbool (*ConsSimpleGps)(
    	struct csrGlobals_tag *pCsrGlobals,
    	contour_t *pContourGroup,
    	btsbool allGroups,
    	gpsExecContext_t *pGpsExecContext,
    	glyph_t *pGlyph);

	btsbool (*ConsCompoundGps)(
    	struct csrGlobals_tag *pCsrGlobals,
    	charElement_t charElement[],
    	int  nGlyphElements, 
    	glyph_t *pGlyph);
    
	btsbool (*MatchSimpleGps)(
    	struct csrGlobals_tag *pCsrGlobals,
    	ufix8  *pSourceGps,
    	ufix8  *pTargetGps,
    	ufix16	sourceGpsSize,
    	ufix16	targetGpsSize,
    	btsbool (*MatchCoord)(
        	fix15 sourceCoord,
        	fix15 targetCoord,
        	matchTrans_t *pMatchTrans),
    	matchTrans_t *pXmatchTrans,
    	matchTrans_t *pYmatchTrans);
    
	btsbool (*MatchCompoundGps)(
    	struct csrGlobals_tag *pCsrGlobals,
    	ufix8  *pSourceGps,
    	ufix8  *pTargetGps,
    	ufix16	sourceGpsSize,
    	ufix16	targetGpsSize,
    	fix31   sourceGpsOffset,
    	fix31   targetGpsOffset);
    
	void (*ReadOruTable)(
    	ufix8 format,
    	ufix8 **ppBuff, 
    	gpsExecContext_t *pGpsExecContext);
    
	void (*ReadGlyphElement)(
    	ufix8 **ppBuff,
    	charElement_t *pGlyphElement,
    	fix31  *pGpsOffset);
	} pfrTypeFns_t;
	
/* Global variable structure */
typedef struct csrGlobals_tag
    {
    ufix8  *pMemoryOrigin;      /* Start of working buffer */
    ufix8  *pAllocLimitLo;      /* First address in main buffer */
    ufix8  *pAllocLimitHi;      /* Last address plus one in main buffer */
    ufix8  *pAllocNextLo;       /* First address at low end of free area */
    ufix8  *pAllocNextLoSaved;  /* Saved value of above before last alloc */
    ufix8  *pAllocNextHi;       /* Last address plus one in free area */
    ufix8  *pAllocNextHiSaved;  /* Saved value of above before last alloc */
    ufix8  *pIObuff;            /* Temp file I/O buffer */
    ufix8  *pAllocWorstHi;      /* Lowest address used by saved char data */
    tempFileState_t tempFileState; /* Temp file access state */
#if INCL_COMPRESSED_PFR    
    ufix8	pfrType;			/* PFR type */
#endif
    pfrTypeFns_t pfrTypeFns;	/* The currently selected gps manager */
    ufix8   modeFlags;          /* Global memory mode flags */
                                /* Bits 0-1: Current memory management mode */
                                /* Bit 2: Set if glyph matching enabled */
                                /* Bit 3: Set if blue value collection mode */
    ufix16  csrState;           /* Call sequence checking flags */
    int (*WriteTempFile)(       /* Callback function for writing temp file */
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM);
    int (*ReadTempFile)(        /* Callback function for reading temp file */
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM);
    int (*WriteResourceData)(   /* Callback function for writing PFR data */
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM);
    void (*GetFontInfo)(        /* Callback fn for getting font information */
        char *pFontID,
        csrFontInfo_t *pFontInfo
        USERPARAM);
    void *(*GetCharID)(         /* Callback fn for getting character ID  */
   	    char asciiCode
        USERPARAM);
    int (*ExecChar)(            /* Callback fn for executing one character */
        PCSRCONTEXT
        void *pCharID
        USERPARAM);
    physicalFont_t *pRootPhysicalFont; /* Pointer to first physical font */
    physicalFont_t *pCurrentPhysicalFont; /* Pointer to current physical font */
    fix15   nPhysicalFonts;     /* Total number of physical fonts */
    fix15   nLogicalFonts;      /* Total number of logical fonts */
    glyph_t *pRootGlyph;        /* Root of glyph tree */
    contour_t *pRootContour;    /* Root of contour tree */
    contour_t *pContour;        /* Current contour */
    polyPoint_t *pPolygon;      /* Origin of polygon buffer */
    polyProcState_t *pPolyProcState; /* Current polygon processing state */
    btsbool firstPoly;          /* TRUE until first CsrMoveTo() call */
    glyph_t *pGlyphSet;         /* Array of new glyph records */
    fix15   nGlyphs;            /* Number of new glyphs in current character */
    fix15   minStrokeThickness; /* Minimum stroke thickness (orus) */
    fix15   minSecEdgeDev;      /* Minimum secondary edge deviation (orus) */
    fix15   maxSecEdgeDev;      /* Maximum secondary edge deviation (orus) */
    ufix8  *pGpsData;           /* Pointer to first glyph prog string */
    fix31   curveTolerance;     /* Curve fitting tolerance (16.16 orus) */
    fix31   nGpsBytes;          /* Total size of all glyph prog strings */
    fix31   nextAuxDataOffset;  /* Next offset for aux data in temp file stream */
    ufix16  maxGpsSize;         /* Max size of any glyph program string */
    fix15   baselineFuzz;       /* Baseline fuzz value */
    fix15   maxBlueValues;      /* Max number of blue values in any font */
    fix15   maxXorus;           /* Max number of controlled X coords */
    fix15   maxYorus;           /* Max number of controlled Y coords */
    logicalFont_t *pRootLogicalFont; /* Sorted list of all logical fonts */
#if REENTRANT
    long userArg;
#endif
    ufix16  charCode;           /* Character code during bitmap recording */
    fix31   ppm;                /* Pixels per em during bitmap recording */
    ufix16  edgeID;             /* ID for labeling groups of edges */
    charOptions_t charOptions;  /* Character recording options */
#if INCL_COMPRESSED_PFR
    fix15  *pAdjustments;		/* Adjustment vector for current compound glyph */
    fix15	adjBuffSize;		/* Size of adjustment vector buffer */
    fix15	adjVectorSize;		/* Size of adjustment vector */
#endif
    } csrGlobals_t;

#if REENTRANT

#define CONTEXTARG pCsrGlobals,
#define USERARG  , pCsrGlobals->userArg

#else

#define CONTEXTARG
#define USERARG  

#endif

/* Mode flags bit assignments */
#define MMODE           0x03    /* Memory management mode */
#define GLYPHMATCHING   0x04    /* Glyph matching enabled */
#define BLUEVALUEMODE   0x08    /* Blue value data gathering mode */

/* Character shape recorder state flag bit assignments */
#define CSROPENMASK          0x0fff
#define CSROPEN              0x0a5b
#define CSRFONTSELECTED      0x1000
#define CSREXECCHARSTATE     0x2000
#define CSREXECCHARBMAPSTATE 0x4000
#define CSRGOODCONTOURS      0x8000

/* Temporary file stream assignments */
#define GPS_TEMP_STREAM 0
#define PFT_TEMP_STREAM 1
#define LFT_TEMP_STREAM 2
#define FID_TEMP_STREAM 3
#define AUX_TEMP_STREAM 4
#define GLF_TEMP_STREAM 5
#define BMP_TEMP_STREAM 6
#define BASE_CHR_TEMP_STREAM 7


/***** Internal function prototypes *****/
csrGlobals_t *CsrInitBuffer(
    void *pBuffer,
    fix31 buffSize);

btsbool CsrCheckAllocs(
    csrGlobals_t *pCsrGlobals,
    fix15 nAllocs, 
    fix31 allocSizes[]);

void *CsrAllocFixed(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void *CsrAllocFixedLo(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void *CsrAllocGrowableLo(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void *CsrGrowAllocLo(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void CsrSaveAllocLo(
    csrGlobals_t *pCsrGlobals);

void CsrRestoreAllocLo(
    csrGlobals_t *pCsrGlobals);

void *CsrAllocFixedHi(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void *CsrAllocGrowableHi(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void *CsrGrowAllocHi(
    csrGlobals_t *pCsrGlobals,
    fix31 nBytes);

void CsrSaveAllocHi(
    csrGlobals_t *pCsrGlobals);

void CsrRestoreAllocHi(
    csrGlobals_t *pCsrGlobals);

int CsrEnhanceMemMgr(
    csrGlobals_t *pCsrGlobals);

int CsrSaveCharData(
    csrGlobals_t *pCsrGlobals,
    physicalFont_t *pPhysicalFont);

int CsrRestoreCharData(
    csrGlobals_t *pCsrGlobals,
    physicalFont_t *pPhysicalFont);

int CsrFindPhysicalFont(
    csrGlobals_t *pCsrGlobals,
    char *pFontID);

int CsrSelectPhysicalFont(
    csrGlobals_t *pCsrGlobals,
    physicalFont_t *pPhysicalFont);

int CsrFindLogicalFont(
    csrGlobals_t *pCsrGlobals,
    char *pFontID,
    fontAttributes_t *pFontAttributes,
    ufix16 *pFontCode);

logicalFont_t *CsrMergeLogicalFonts(csrGlobals_t *pCsrGlobals);

int CsrFindCharacter(
    csrGlobals_t *pCsrGlobals,
    ufix16 charCode,
    void *pCharID,
    char asciiCode);

void CsrFlattenCharacterTree(
    csrGlobals_t *pCsrGlobals,
    physicalFont_t *pPhysicalFont);

int CsrOpenContour(
    csrGlobals_t *pCsrGlobals);

int CsrCloseContour(
    csrGlobals_t *pCsrGlobals);

int CsrFlattenContourTree(
	contour_t *pContourTree,
	fix15	contourType,
	fix15 *pnParents);

int CsrInitPolyProcState(
    csrGlobals_t *pCsrGlobals);

int CsrAddPoint(
    csrGlobals_t *pCsrGlobals,
    fix15 x, 
    fix15 y,
    ufix16 props);

int CsrAddQuadratic(
    csrGlobals_t *pCsrGlobals,
    fix15 x1, 
    fix15 y1,
    fix15 x2, 
    fix15 y2);

int CsrAddCubic(
    csrGlobals_t *pCsrGlobals,
    fix15 x1, 
    fix15 y1,
    fix15 x2, 
    fix15 y2,
    fix15 x3, 
    fix15 y3);

ufix16 CsrCornerType(
    csrGlobals_t *pCsrGlobals,
    fix15 dx1,
    fix15 dy1,
    fix15 dx2,
    fix15 dy2);

int CsrFlushPoly(
    csrGlobals_t *pCsrGlobals);

int CsrAddSegment(
    csrGlobals_t *pCsrGlobals,
    segPoint_t *pSegEnd);

void CsrConsEdgeList(
    csrGlobals_t *pCsrGlobals,
    contour_t *pContour,
    btsbool vertical);

int CsrConsGlyph(
    csrGlobals_t *pCsrGlobals,
    ufix16  *pGpsSize,
    fix31  *pGpsOffset);

void CsrFlattenGlyphTree(
    csrGlobals_t *pCsrGlobals);

void CsrSetGpsExecContext(
    csrGlobals_t *pCsrGlobals,
    contour_t *pContourGroup,
    btsbool allGroups,
    gpsExecContext_t *pGpsExecContext);

btsbool CsrBitmappable(
    fontAttributes_t *pFontAttributes);

int CsrInitBmapStuff(
    csrGlobals_t *pCsrGlobals,
    logicalFont_t *pLogicalFont);

int CsrFindBmapChars(
    csrGlobals_t *pCsrGlobals,
    ufix16 charCode,
    void *pCharID);

int CsrConsBmapChar(
    csrGlobals_t *pCsrGlobals,
    csrBmapSpecs_t *pBmapSpecs,
    ufix8 *pBmapImage);

void CsrFlattenBmapCharTree(
    csrGlobals_t *pCsrGlobals,
    bmapSize_t *pBmapSize);

int CsrWritePfr(
    csrGlobals_t *pCsrGlobals);

void CsrCompileWord(
    ufix16 x,
    ufix8 **ppBuff);

void CsrCompileLong(
    ufix32 x,
    ufix8 **ppBuff);

void CsrInitTempFileState(
    csrGlobals_t *pCsrGlobals);

int CsrWriteTempDataStream(
    csrGlobals_t *pCsrGlobals,
    fix15  stream,
    ufix8 *pBuffer,
    fix31  nBytes,
    fix31  offset);

int CsrReadTempDataStream(
    csrGlobals_t *pCsrGlobals,
    fix15  stream,
    ufix8 *pBuffer,
    fix31  nBytes,
    fix31  offset);

int CsrTempStreamToResourceData(
    csrGlobals_t *pCsrGlobals,
    fix15  stream,
    fix31  nBytes,
    fix31  streamOffset,
    fix31  resOffset);

void CsrGenStemSnaps(
	csrGlobals_t *pCsrGlobals,
	physicalFont_t *pPhysicalFont);
	
#if INCL_COMPRESSED_PFR
btsbool CsrAdjustSimpleGps(
    csrGlobals_t *pCsrGlobals,
    ufix8   *pSourceGps,
    ufix8   *pTargetGps,
    ufix16	sourceGpsSize,
    ufix16	targetGpsSize);
#endif
