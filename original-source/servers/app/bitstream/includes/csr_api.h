/*****************************************************************************
*                                                                            *
*                         Copyright 1993 - 96                                *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/************************** C S R _  A P I  . H ******************************
 *                                                                           *
 * Character shape recorder public header file.                              *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1)  2 Nov 93  jsc  Created                                               *
 *                                                                           *
 *     $Header: R:/src/CSR/rcs/CSR_API.H 6.9 1997/03/18 15:59:34 SHAWN release $
 *                                                                                    
 *     $Log: CSR_API.H $
 *     Revision 6.9  1997/03/18 15:59:34  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 6.8  1997/02/12 17:45:32  JohnC
 *     Changed default value of INCL_COMPRESSED_PFR to 1
 *     Replaced DISABLE_GLYPH_MUTATION with
 *     ENABLE_GLYPH_MUTATION as a character options flag.
 *     Revision 6.7  1997/02/10 22:35:04  JohnC
 *     Added DISABLE_GLYPH_MUTATION to the set of flags available
 *     for controlling character recording options.
 *     Revision 6.6  1997/01/13 22:47:48  JohnC
 *     Moved public compile time options from csr_int.h to here.
 *     Revision 6.5  1997/01/08 18:45:56  JOHNC
 *     Added OPPOSING_CONTOURS to list of contour type options.
 *     Revision 6.4  1996/12/27 15:57:39  john
 *     Added contourType to csrFontInfo_t struct.
 * Revision 6.3  96/11/15  17:11:30  john
 * Made support of compressed PFR dependent upon INCL_COMPRESSED_PFR.
 * 
 * Revision 6.2  96/11/14  18:02:33  john
 * Added function prototype and error definition for CsrSetPfrType().
 * 
 * Revision 6.1  96/10/10  14:07:48  mark
 * Release
 * 
 * Revision 5.2  96/10/02  15:24:41  john
 * Added definitions for two new error return values:
 * 	CSR_N_FONTS_ERR
 * 	CSR_PFR_SIZE_ERR
 * 
 * Revision 5.1  96/08/27  11:50:45  mark
 * Release
 * 
 * Revision 4.2  96/06/03  09:56:14  john
 * Added stem snap fields to srFontInfo_t struct.
 * Added definition for CSR_N_STEMSNAPS_ERR.
 * 
 * Revision 4.1  96/03/14  14:14:30  mark
 * Release
 * 
 * Revision 3.2  96/03/14  14:12:50  mark
 * rename PCONTEXT to PCSPCONTEXT and protect type share with
 * csr so that csp_api.h and csr_api.h can be included in the
 * same file
 * 
 * Revision 3.1  96/03/05  13:50:33  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:49:38  mark
 * Release
 * 
 * Revision 1.4  95/11/22  15:53:29  john
 * Added DISABLE_COMPOUND_GLYPHS to character recording options.
 * 
 * Revision 1.3  95/11/21  09:21:28  john
 * Added structure and constant definitions and function
 * prototype for new function CsrSetCharOptions().
 * 
 * Revision 1.2  95/09/01  17:48:36  john
 * Added curve fit tolerance value to fontInfo struct.
 * Added error return value if it is out of range.
 * 
 * Revision 1.1  95/08/10  18:20:56  john
 * Initial revision
 * 
 * Revision 10.1  95/07/31  10:24:14  mark
 * Release
 * 
 * Revision 9.2  95/07/19  12:39:53  mark
 * Using PCSRCONTEXT for void functions leaves dangling commas that aren't portable.
 * 
 * Revision 9.1  95/04/26  15:48:03  mark
 * Release
 * 
 * Revision 8.2  95/03/28  16:13:48  john
 * New fields added to csrFontInfo_t structure to support
 *     supplied blue values etc.
 * New error return definitions added.
 * 
 * Revision 8.1  95/03/13  15:48:07  mark
 * Release
 * 
 * Revision 7.4  95/02/24  11:26:56  john
 * bmapSpecs struct renamed csrBmapSpecs and modified to include
 *     bitmap delivery format specs and fractional bitmap
 *     positioning and escapement values.
 * CsrSetBitmapParams function prototype eliminated.
 * 
 * Revision 7.3  95/02/07  17:02:02  john
 * Added definitions for bitmap recording.
 * 
 * Revision 7.2  94/09/29  09:41:48  mark
 * move include of csr_opt.h from csr_int.h, since it REENTRANT can
 * affect the API
 * 
 * Revision 7.1  94/08/26  16:16:29  mark
 * Release
 * 
 * Revision 6.1  94/08/25  14:08:54  mark
 * Release
 * 
 * Revision 4.4  94/08/15  15:06:28  john
 * Cosmetic changes to header.
 * 
 * Revision 4.3  94/07/21  14:44:43  mark
 * context argument is now only passed to ExecChar callback function.
 * 
 * Revision 4.2  94/07/08  16:28:52  mark
 * change calling sequences for REENTRANT option.
 * 
 * Revision 4.1  94/07/05  09:55:01  mark
 * Release
 * 
 * Revision 3.1  94/06/20  08:59:58  mark
 * Release
 * 
 * Revision 2.4  94/06/13  17:41:18  john
 * New error code CSR_TEMP_FILE_OFLO_ERR (15) added.
 * 
 * Revision 2.3  94/06/07  10:02:43  john
 * Definitions for bold style added.
 * 
 * Revision 2.2  94/05/27  14:57:39  john
 * Updates to support extendable font style support.
 * 
 * Revision 2.1  94/04/25  14:43:16  mark
 * Release
 * 
 * Revision 1.5  94/04/15  14:56:56  john
 * Added CSR_TEMP_FILE_ERR to flag internal temp file access problem.
 * 
 * Revision 1.4  94/03/29  17:09:01  john
 * Updated and rationalized error return values.
 * 
 * Revision 1.3  94/01/26  13:41:37  mark
 * add RCS tags
 * 
 * 
 *                                                                           *
 ****************************************************************************/

#include "csr_opt.h"

/***** Default values for public overridable compile-time settings *****/

/*
 *  Auxiliary data size threshold for temp file storage
 *  Auxiliary data exceeding this size is stored in the temporary file
 *  rather than in the main memory buffer.
 *  Units are bytes
 */
#ifndef AUX_DATA_THRESH
#define AUX_DATA_THRESH     256
#endif

/*
 *  Default limit on number of recorded bitmap resolutions
 *  Reasonable values are in the range 0 (no bitmaps) to 100
 *	Adds MAX_BMAP_RES_VALUES * 4 bytes to the memory requirements
 *	for each physical font.
 */
#ifndef MAX_BMAP_RES_VALUES
#define MAX_BMAP_RES_VALUES   8 /* Max number of recorded bmap resolutions */
#endif

/*
 *  Minimum buffer size for glyph matching
 *  Glyph matching is inhibited if the total memory allocated
 *  to the character shape recorder buffer is less than this amount.
 *  Units are bytes
 */
#ifndef GLYPH_MATCHING_THRESH
#define GLYPH_MATCHING_THRESH 45000
#endif

/*
 *  Include compressed PFR capability by default
 *	Set to 0 to exclude code supporting the capability
 */
#ifndef INCL_COMPRESSED_PFR
#define INCL_COMPRESSED_PFR  1
#endif

/*
 *	Compile in non-reentrant mode by default
 *	Set to 1 to enable reentrant compilation
 */
#ifndef REENTRANT
#define REENTRANT 0
#endif

/* 
 *  Structure alignment rules
 *  Allowed values are:
 *      1: Byte boundaries
 *      2: Even byte boundaries
 *      4: Long word boundaries (default)
 */
#ifndef STRUCTALIGN
#define STRUCTALIGN           4
#endif


/***** Define constants used for re-entrant version *****/
#if REENTRANT

#define PCSRCONTEXT void *pCsrGlobals,
#define USERPARAM , long userParam

#else

#define PCSRCONTEXT
#define USERPARAM

#endif


/***** Public structure definitions *****/  
/* Common Types shared with CSP are protected from redundant definition */
#ifndef _COMMON_TYPES_DEFINED
#define _COMMON_TYPES_DEFINED

/* Style specs for normal (filled) output */
typedef struct styleSpecsFilled_tag
    {
    short dummy;
    } styleSpecsFilled_t;

/* Style specs for stroked output */
typedef struct styleSpecsStroked_tag
    {
    short strokeThickness;
    short lineJoinType;
    long  miterLimit;
    } styleSpecsStroked_t;

/* Style specs for pseudo-bold output */
typedef struct styleSpecsBold_tag
    {
    short boldThickness;
    } styleSpecsBold_t;

/* Style specifications */
typedef union styleSpecs_tag
    {
    styleSpecsFilled_t styleSpecsFilled;
    styleSpecsStroked_t styleSpecsStroked;
    styleSpecsBold_t styleSpecsBold;
    } styleSpecs_t;

/* Font attributes */
typedef struct fontAttributes_tag
    {
    long  fontMatrix[4];
    short fontStyle;
    styleSpecs_t styleSpecs;
    } fontAttributes_t;

/* Font style values */
#define FILLED_STYLE    0
#define STROKED_STYLE   1
#define BOLD_STYLE      2

/* Line join types */
#define MITER_LINE_JOIN 0
#define ROUND_LINE_JOIN 1
#define BEVEL_LINE_JOIN 2
#endif  

/* Bitmap specs structure */
typedef struct csrBmapSpecs_tag
    {
    long  xPos;             /* X coord of left edge of bitmap (16.16 pixels) */
    long  yPos;             /* Y coord of first row of bitmap (16.16 pixels) */
    short xSize;            /* Width of bitmap image (whole pixels) */
    short ySize;            /* Height of bitmap image (whole pixels) */
    long  xEscapement;      /* X escapement (16.16 pixels) */
    long  yEscapement;      /* Y escapement (16.16 pixels) */
    short pixelColor;       /* Bit representation of black (0 or 1) */
    short alignment;        /* Byte alignment of each row (0, 1, 2 or 4) */
    short inverted;         /* Rows in increasing (0) or decreasing (1) Y value */ 
    } csrBmapSpecs_t;

/* Font information structure */
typedef struct csrFontInfo_tag
    {
    short outlineResolution;
    short metricsResolution;
    void *pAuxData;
    long  nAuxBytes;
    short *pBlueValues;
    short nBlueValues;
    short blueFuzz;
    short blueScale;
    short stdVW;
    short stdHW;
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
    short nBmapResValues;
    long *pBmapResValues;
    short curveTolerance;
    short nStemSnapsV;
    short nStemSnapsH;
    short *pStemSnapsV;
    short *pStemSnapsH;
    short contourType;
    } csrFontInfo_t;

/* Contour type constants */
#define UNSPECIFIED_CONTOURS	0
#define POSTSCRIPT_CONTOURS		1
#define TRUETYPE_CONTOURS		2
#define OPPOSING_CONTOURS		3

/* Character recording options structure */
typedef struct charOptions_tag
    {
    unsigned short charFlags;
    } charOptions_t;

/* Character flag bit assignments */
#define DISABLE_ALL_V_STEMS     0x0001
#define DISABLE_ALL_H_STEMS     0x0002
#define DISABLE_ALL_STEMS       0x0003
#define DISABLE_SEC_V_STEMS     0x0004
#define DISABLE_SEC_H_STEMS     0x0008
#define DISABLE_SEC_STEMS       0x000c
#define DISABLE_SEC_V_EDGES     0x0010
#define DISABLE_SEC_H_EDGES     0x0020
#define DISABLE_SEC_EDGES       0x0030
#define EXACT_GLYPH_MATCHING    0x0100
#define DISABLE_COMPOUND_GLYPHS 0x0200
#define ENABLE_GLYPH_MUTATION   0x0400

/***** End of public structure definitions *****/


/***** Public function prototypes *****/
int CsrOpen(
#if REENTRANT
    void **pContext,
#endif
    void *pBuffer,
    long nBytes,
    int (*WriteTempFile)(
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM),
    int (*ReadTempFile)(
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM),
    int (*WriteResourceData)(
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM)
    USERPARAM
    );

#if INCL_COMPRESSED_PFR
int CsrSetPfrType(
    PCSRCONTEXT
	short pfrType);
#endif

int CsrSetFontSpecs(
    PCSRCONTEXT
    char *pFontID,
    fontAttributes_t *pFontAttributes,
    void (*GetFontInfo)(
        char *pFontID,
        csrFontInfo_t *pFontInfo
        USERPARAM),
    void *(*GetCharID)(
   	    char asciiCode
        USERPARAM),
    int (*ExecChar)(
        PCSRCONTEXT
        void *pCharID
        USERPARAM),
    unsigned short *pFontCode);

int CsrDoChar(
    PCSRCONTEXT
    unsigned short charCode,
    void *pCharID,
    char asciiCode);

int CsrSetCharOptions(
    PCSRCONTEXT
    charOptions_t *pCharOptions);

int CsrMoveTo(
    PCSRCONTEXT
    short x,
    short y);

int CsrLineTo(
    PCSRCONTEXT
    short x,
    short y);

int CsrQuadraticTo(
    PCSRCONTEXT
    short x1,
    short y1,
    short x2,
    short y2);

int CsrCubicTo(
    PCSRCONTEXT
    short x1,
    short y1,
    short x2,
    short y2,
    short x3,
    short y3);

int CsrSetBitmap(
    PCSRCONTEXT
    csrBmapSpecs_t *pBmapSpecs,
    void *pBitmap);

int CsrWriteResource(
#if REENTRANT
    void *pCsrContext
#else
    void
#endif
    );

int CsrClose(
#if REENTRANT
    void *pCsrContext
#else
    void
#endif
    );

/***** End of public function prototypes *****/


/***** Error return definitions *****/
#define CSR_NO_ERR               0   /* Normal return */
#define CSR_CALL_SEQUENCE_ERR    1   /* API call sequence error */
#define CSR_BUFFER_OVERFLOW_ERR  2   /* Memory buffer overflow */
#define CSR_WRITE_TEMP_FILE_ERR  3   /* WriteTempFile() returned error */
#define CSR_READ_TEMP_FILE_ERR   4   /* ReadTempFile() returned error */
#define CSR_LINE_JOIN_TYPE_ERR   5   /* LineJoinType out of range */
#define CSR_OUTLINE_RES_ERR      6   /* OutlineResolution out of range */
#define CSR_METRICS_RES_ERR      7   /* MetricsResolution out of range */
#define CSR_N_AUX_BYTES_ERR      8   /* Auxiliary data too large */
#define CSR_CONTOUR_SYNTAX_ERR   9   /* Contour syntax error */
#define CSR_EXEC_CHAR_ERR       10   /* ExecChar() returned error */
#define CSR_ESCAPEMENT_ERR      11   /* Escapement direction error */
#define CSR_ASCII_CODE_ERR      12   /* Inconsistent ascii code usage */
#define CSR_WRITE_RESOURCE_ERR  13   /* WriteResourceData() returned error */
#define CSR_FONT_STYLE_ERR      14   /* Font style out of range */
#define CSR_TEMP_FILE_OFLO_ERR  15   /* Temp file overflow error */
#define CSR_BMAP_RES_SPECS_ERR  16   /* Bitmap resolution spec error */
#define CSR_SET_BMAP_SPECS_ERR  17   /* Bitmap specs value out of range */
#define CSR_EXEC_CHAR_BMAP_ERR  18   /* ExecCharBmap() returned error */
#define CSR_N_BLUE_VALUES_ERR   19   /* Illegal number of blue values */
#define CSR_NEG_BLUE_ZONE_ERR   20   /* Negative blue zone size */
#define CSR_BLUE_ZONE_SIZE_ERR  21   /* Blue zone too large */
#define CSR_BLUE_ZONE_SEP_ERR   22   /* Blue zone separation error */
#define CSR_CURVE_TOLERANCE_ERR 23   /* Curve tolerance out of range */
#define CSR_N_STEMSNAPS_ERR     24   /* Illegal number of stemsnap values */
#define CSR_N_FONTS_ERR         25   /* Too many fonts */
#define CSR_PFR_SIZE_ERR        26   /* PFR size limit exceeded */
#define CSR_PFR_TYPE_RANGE_ERR	27	 /* PFR type out of range */
#define CSR_CONTOUR_TYPE_ERR	28	 /* Contour type out of range */
#define CSR_CONTOUR_DIR_ERR		29	 /* Outer contour has wrong direction */

#define CSR_TEMP_FILE_ERR       -1   /* Internal temp file access error */
#define CSR_BMAP_UPDATE_ERR     -2   /* Internal bitmap updating error */
#define CSR_BLUE_VALS_OFLO_ERR  -3   /* Blue value table overflow */


