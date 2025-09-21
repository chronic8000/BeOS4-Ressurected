/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/************************** C S P _ A P I  . H *******************************
 *                                                                           *
 * Character shape player public header file.                                *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_API.H 7.13 1997/03/18 16:04:20 SHAWN release $
 *                                                                                    
 *     $Log: CSP_API.H $
 *     Revision 7.13  1997/03/18 16:04:20  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.12  1997/02/12 17:49:18  JohnC
 *     Changed default value of INCL_COMPRESSED_PFR from 0 to 1
 *     Revision 7.11  1997/02/11 18:05:06  JohnC
 *     Made CspIsCacheEnabled() proto conditional on INCL_CACHE
 *     Revision 7.10  1997/02/06 19:26:14  MARK
 *     Add CspIsCacheEnabled function to return the state of
 *      pCspState->cacheEnabled
 *     Revision 7.9  1997/01/14 21:22:24  JohnC
 *     Rearranged the public compile-time constants to be more 
 *     consistent with the documentation.
 *     Revision 7.8  1996/12/20 15:06:19  john
 *     Changed default structure alignment from 2-byte to 4-byte boundaries.
 * Revision 7.7  96/11/14  16:49:51  john
 * Added definitions for default exclusion of compressed PFR support.
 * 
 * Revision 7.6  96/11/07  15:47:21  john
 * BLACK_PIXEL definition moved here from cachemgr.h
 * 
 * Revision 7.5  96/11/06  17:24:34  john
 * Added definition to include hint interpretation by default.
 * 
 * Revision 7.4  96/11/06  12:47:19  john
 * Added definition to exclude undocumented API funcitons by default.
 * Added definition to include PFR bitmap handling by default.
 * 
 * Revision 7.3  96/11/05  17:42:56  john
 * Added sub-pixel positioning error definitions.
 * 
 * Revision 7.2  96/10/29  17:47:09  john
 * Added ANTIALIASED_ALT1_OUTPUT capability.
 * 
 * Revision 7.1  96/10/10  14:04:32  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:57:05  mark
 * Release
 * 
 * Revision 5.6  96/06/20  17:22:03  john
 * Added prototypes for new functions:
 *     CspListCharsAlt1()
 *     CspSetOutputSpecsAlt1()
 * 
 * Revision 5.5  96/05/31  17:10:05  john
 * Added stem snap fields to cspFontInfo_t.
 * 
 * Revision 5.4  96/04/10  11:35:09  shawn
 * Changed GetspGlobalPtr() function type/argument to 'void *'
 *      to avoid compiler warnings/errors.
 * 
 * Revision 5.3  96/03/29  16:15:29  john
 * Added API definitions for compound character structure
 *     delivery in outline output mode.
 * 
 * Revision 5.2  96/03/26  17:52:27  john
 * Added unique physical font stuff.
 * 
 * Revision 5.1  96/03/14  14:20:04  mark
 * Release
 * 
 * Revision 4.2  96/03/14  14:10:46  mark
 * rename PCONTEXT to PCSPCONTEXT and protect type share with
 * csr so that csp_api.h and csr_api.h can be included in the
 * same file
 * 
 * Revision 4.1  96/03/05  13:44:59  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:36  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:32  mark
 * Release
 * 
 * Revision 1.5  95/11/07  13:39:54  john
 * Outline output options_t structure extended to support
 * output of secondary stem and edge hints.
 * 
 * Revision 1.4  95/09/18  18:12:44  john
 * Added prototypes for CspSetMissingChar() and CspUnsetMissingChar().
 * 
 * Revision 1.3  95/09/14  14:52:46  john
 * Added blueShift to fontInfo structure.
 * 
 * Revision 1.2  95/08/15  15:29:04  john
 * Cosmetic change:
 *     In proto for CspSetFont, charCode changed to fontCode.
 * 
 * Revision 1.1  95/08/10  16:44:24  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/

#include "csp_opt.h"

#ifndef GLOBAL_PROTO
#define GLOBAL_PROTO
#endif
#ifndef LOCAL_PROTO
#define LOCAL_PROTO static
#endif

#ifndef INCL_TPS
#define INCL_TPS 0
#endif


/* Default values for public conditional compilation flags  */

/* Represent black pixel as 1 by default */
#ifndef BLACK_PIXEL
#define BLACK_PIXEL 		  1
#endif

/* Maximum number of dynamic fonts that can be active */
#ifndef CSP_MAX_DYNAMIC_FONTS
#define CSP_MAX_DYNAMIC_FONTS 0
#endif

/* Default device resolution is 600 dots per inch */
#ifndef DEFAULT_DEVICE_RES
#define DEFAULT_DEVICE_RES  600
#endif

/* Exclude antialiased output module by default */
#ifndef INCL_ANTIALIASED_OUTPUT
#define INCL_ANTIALIASED_OUTPUT  0
#endif

/* Include emboldening by default */
#ifndef INCL_BOLD
#define INCL_BOLD             1
#endif

/* Include bitmap cache manager by default */
#ifndef INCL_CACHE
#define INCL_CACHE            1
#endif

/* Include compressed PFR capability by default */
#ifndef INCL_COMPRESSED_PFR
#define INCL_COMPRESSED_PFR  1
#endif

/* Include hint mechanism by default */
#ifndef INCL_CSPHINTS
#define INCL_CSPHINTS  1
#endif

/* Include outline output module by default (unless TPS) */
#ifndef INCL_CSPOUTLINE
#if INCL_TPS
#define INCL_CSPOUTLINE 0
#else
#define INCL_CSPOUTLINE 1
#endif
#endif

/* Include quickwriter output module by default (unless TPS) */
#ifndef INCL_CSPQUICK
#if INCL_TPS
#define INCL_CSPQUICK 0
#else
#define INCL_CSPQUICK 1
#endif
#endif

/* Exclude direct output by default (unless TPS) */
#ifndef INCL_DIR
#if INCL_TPS
#define INCL_DIR 1
#else
#define INCL_DIR 0
#endif
#endif

/* Include PFR bitmap handling by default */
#ifndef INCL_PFR_BITMAPS
#define INCL_PFR_BITMAPS  1
#endif

/* Exclude bitmap filtering module by default */
#ifndef INCL_STD_BMAP_FILTER
#define INCL_STD_BMAP_FILTER  0
#endif

/* Include stroked output module by default */
#ifndef INCL_STROKE
#define INCL_STROKE           1
#endif

/* Exclude undocumented API calls by default */
#ifndef INCL_UNDOCUMENTED_API
#define INCL_UNDOCUMENTED_API  0
#endif

/* Default value for reentrant flag */
#if INCL_TPS
#if REENTRANT_ALLOC
#define REENTRANT 1
#endif
#endif

#ifndef REENTRANT
#define REENTRANT 0
#endif

/* Default value for structure alignment */
#ifndef STRUCTALIGN
#define STRUCTALIGN           4
#endif


/***** Definitions to support conditional reentrant compilation *****/

#if REENTRANT

#define PCSPCONTEXT1 void *pCspGlobals
#define PCSPCONTEXT PCSPCONTEXT1,
#define PCACONTEXT void **pCacheContext,
#define PREENT void **pContext, void *pCmGlobals,
#define USERPARAM1 long userParam
#define USERPARAM , long userParam

#else

#define PCSPCONTEXT1 void
#define PCSPCONTEXT
#define PCACONTEXT
#define PREENT
#define USERPARAM1 void
#define USERPARAM

#endif


/***** Public structure definitions *****/

/* PFR access structures */
typedef union cspPfrAccessInfo_tag
    {
    int (*ReadResourceData)(
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM);
    struct
        {
        int (*ReadResourceData)(
            void *pBuffer,
            short nBytes,
            long offset,
            long pfrContext
            USERPARAM);
        long pfrContext;
        } indirectAlt1;
    void *pPfr;
    } cspPfrAccessInfo_t;

typedef struct cspPfrAccess_tag
    {
    short mode;
    cspPfrAccessInfo_t access;
    } cspPfrAccess_t;

/* PFR access mode constants */
#define PFR_ACCESS_INDIRECT      0
#define PFR_ACCESS_INDIRECT_ALT1 1
#define PFR_ACCESS_DIRECT        2

/* Bounding box structure */
typedef struct CspBbox_tag
    {
    short xmin;
    short ymin;
    short xmax;
    short ymax;
    }  CspBbox_t;

typedef struct CspScaledBbox_tag
    {
    long xmin;
    long ymin;
    long xmax;
    long ymax;
    }  CspScaledBbox_t;

/* Common Types shared with CSR are protected from redundant definition */
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

/* Line join types */
#define MITER_LINE_JOIN 0
#define ROUND_LINE_JOIN 1
#define BEVEL_LINE_JOIN 2

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
#define FILLED_STYLE       0
#define STROKED_STYLE      1
#define BOLD_STYLE         2
#endif

/* Font information structure */
typedef struct cspFontInfo_tag
    {
    short outlineResolution;
    short metricsResolution;
    void *pAuxData;
    long  nAuxBytes;
    short *pBlueValues;
    short nBlueValues;
    short blueFuzz;
    short blueScale;
    short blueShift;
    short stdVW;
    short stdHW;
    char *pFontID;
    CspBbox_t fontBBox;
    unsigned short nCharacters;
#if CSP_MAX_DYNAMIC_FONTS > 0
    unsigned short fontFlags;
#endif
    short nStemSnapsV;
    short nStemSnapsH;
    short *pStemSnapsV;
    short *pStemSnapsH;
    } cspFontInfo_t;

/* Font information flags bit assignments */
#if CSP_MAX_DYNAMIC_FONTS > 0
#define CSP_FONT_SUPPLEMENT 1
#endif

/* Character information structure */
typedef struct cspCharInfo_tag
    {
    short hWidth;
    short vWidth;
    CspBbox_t charBBox;
    } cspCharInfo_t;

/* Glyph specs structure */
typedef struct glyphSpecs_tag
    {
    short glyphType;            /* Glyph type */
    short pfrCode;              /* Index of PFR containing glyph */
    long  glyphOffset;          /* Glyph offset within PFR */
    long  xScale;               /* X scale factor in 16.16 units */
    long  yScale;               /* Y scale factor in 16.16 units */
    long  xPosition;            /* X position in 16.16 units */
    long  yPosition;            /* Y position in 16.16 units */
    } glyphSpecs_t;

/* Glyph types */
#define SIMP_GLYPH          0x0000  /* Simple glyph */
#define COMP_GLYPH_HEAD     0x0001  /* Compound glyph head */
#define COMP_GLYPH_ELEMENT  0x0002  /* Compound glyph element */

/* Options structure for outline output */
typedef struct options_tag
    {
    unsigned short flags;
    void (*Vedge)(
        short x,
        short dx,
        short thresh
        USERPARAM);
    void (*Hedge)(
        short y,
        short dy,
        short thresh
        USERPARAM);
    void (*GlyphSpecs)(
        glyphSpecs_t *pGlyphSpecs
        USERPARAM);
    unsigned short glyphSpecsFlags;
    } options_t;

/* Options flags bit assignments */
#define REV_ALL_CTRS        0x0001  /* Reverse all contours */
#define REV_INNER_CTRS      0x0002  /* Reverse inner contours only */
#define ENABLE_SEC_STEMS    0x0004  /* Enable secondary stem hint output */
#define ENABLE_SEC_EDGES    0x0008  /* Enable secondary edge hint output */
#define ENABLE_GLYPH_SPECS  0x0010  /* Enable glyph specifications delivery */

/* Glyph specs flags bit assignments */
#define DISABLE_X_SCALE     0x0001  /* Disable X scale in comp char elements */
#define DISABLE_Y_SCALE     0x0002  /* Disable Y scale in comp char elements */
#define DISABLE_X_SHIFT     0x0004  /* Disable X shift in comp char elements */
#define DISABLE_Y_SHIFT     0x0008  /* Disable Y shift in comp char elements */

/* Bitmap specifications */
typedef struct bmapSpecs_tag
    {
    short xPos;
    short yPos;
    short xSize;
    short ySize;
    } bmapSpecs_t;

typedef struct charBBoxOutputSpecs_tag
    {
    long outputMatrix[4];
    } charBBoxOutputSpecs_t;

typedef struct bitmapOutputSpecs_tag
    {
    long outputMatrix[4];
    CspBbox_t outputBBox;
    void (*SetBitmap)(
        bmapSpecs_t bmapSpecs,
        void *pBitmap
        USERPARAM);
    void (*OpenBitmap)(
        bmapSpecs_t bmapSpecs
        USERPARAM);
    void (*SetBitmapBits)(
        short y,
        short xStart,
        short xEnd
        USERPARAM);
    void (*CloseBitmap)(USERPARAM1);
    } bitmapOutputSpecs_t;

#if INCL_CSPOUTLINE
typedef struct outlineOutputSpecs_tag
    {
    long outputMatrix[4];
    options_t *pOptions;
    void (*Vstem)(
        short x1,
        short x2
        USERPARAM);
    void (*Hstem)(
        short y1,
        short y2
        USERPARAM);
    void (*MoveTo)(
        short x,
        short y
        USERPARAM);
    void (*LineTo)(
        short x,
        short y
        USERPARAM);
    void (*CurveTo)(
        short x1,
        short y1,
        short x2,
        short y2,
        short x3,
        short y3
        USERPARAM);
    void (*ClosePath)(USERPARAM1);
    } outlineOutputSpecs_t;
#endif

#if INCL_DIR
typedef struct directOutputSpecs_tag
    {
    long outputMatrix[4];
    void *pOptions;
    void (*InitOut)(void);
    void (*BeginChar)(void);
    void (*BeginSubChar)(void);
    void (*BeginContour)(void);
    void (*CurveTo)(void);
    void (*LineTo)(void);
    void (*EndContour)(void);
    void (*EndSubChar)(void);
    void (*EndChar)(void);
    } directOutputSpecs_t;
#endif

#if INCL_ANTIALIASED_OUTPUT
typedef struct antialiasedOutputSpecs_tag
    {
    /* Note: First 6 elements must match bitmapOutputSpecs struct */
    long outputMatrix[4];
    CspBbox_t outputBBox;
	void (*SetBitmap)(
        bmapSpecs_t bmapSpecs,
        void *pBitmap
        USERPARAM);
    void (*OpenBitmap)(
        bmapSpecs_t bmapSpecs
        USERPARAM);
    void (*SetBitmapBits)(
        short y,
        short xStart,
        short xEnd
        USERPARAM);
    void (*CloseBitmap)(USERPARAM1);
    void (*SetBitmapPixels)(
        short y,
        short xStart,
        short xSize,
        void *pPixels
        USERPARAM);
	short   pixelSize;
    } antialiasedOutputSpecs_t;

typedef struct intBitmapFns_tag filterFn_t;

typedef struct antialiasedAlt1OutputSpecs_tag
    {
    /* Note: First 8 elements must match antialiasedOutputSpecs struct */
    long outputMatrix[4];
    CspBbox_t outputBBox;
    void (*SetBitmap)(
        bmapSpecs_t bmapSpecs,
        void *pBitmap
        USERPARAM);
    void (*OpenBitmap)(
        bmapSpecs_t bmapSpecs
        USERPARAM);
    void (*SetBitmapBits)(
        short y,
        short xStart,
        short xEnd
        USERPARAM);
    void (*CloseBitmap)(USERPARAM1);
    void (*SetBitmapPixels)(
        short y,
        short xStart,
        short xSize,
        void *pPixels
        USERPARAM);
    short pixelSize;
    unsigned char xHintMode;
    unsigned char yHintMode;
    unsigned char filterWidth;
    unsigned char filterHeight;
    unsigned char xPosSubdivs;
    unsigned char yPosSubdivs;
	filterFn_t *pFilterFn;
	void *pFilterFnBuff;
	long filterFnBuffSize;
	void *pPixelTable;
    } antialiasedAlt1OutputSpecs_t;

/* Hint interpretation mode flags */
#define NO_SEC_EDGES 		0x0001
#define NO_SEC_STROKES 		0x0002
#define NO_PRIMARY_STROKES 	0x0004
#define NO_STROKE_ALIGNMENT 0x0008

#endif

typedef struct outputSpecs_tag
    {
    short outputType;
    union
        {
        charBBoxOutputSpecs_t charBBox;
        bitmapOutputSpecs_t bitmap;
#if INCL_CSPOUTLINE
        outlineOutputSpecs_t outline;
#endif
#if INCL_DIR
        directOutputSpecs_t direct;
#endif
#if INCL_ANTIALIASED_OUTPUT
        antialiasedOutputSpecs_t pixmap;
        antialiasedAlt1OutputSpecs_t pixmap1;
#endif
        } specs;
    } outputSpecs_t;

/***** End of public structure definitions *****/


/***** Public function prototypes *****/

GLOBAL_PROTO
int CspInitBitmapCache(
	PCACONTEXT
    void *pBuffer,
    long nBytes);

GLOBAL_PROTO
int CspSetCacheParams(
     PCSPCONTEXT
     short int enabled, 
     short int capacity,
     short int pixelColor,
     short int alignment,
     short int inverted);

GLOBAL_PROTO
int CspOpen(
	PREENT
    short deviceResolution,
    int (*ReadResourceData)(
        void *pBuffer,
        short nBytes,
        long offset
        USERPARAM),
    void *(*AllocCspBuffer)(
        long minBuffSize,
        long normalBuffSize,
        long maxBuffSize,
        long *pNBytes
        USERPARAM)
    USERPARAM);

GLOBAL_PROTO
int CspOpenAlt1(
	PREENT
    short deviceResolution,
    short nPfrs,
    cspPfrAccess_t *pPfrAccessList,
    void *(*AllocCspBuffer)(
        long minBuffSize,
        long normalBuffSize,
        long maxBuffSize,
        long *pNBytes
        USERPARAM)
    USERPARAM);

GLOBAL_PROTO
int CspSetPfr(
    PCSPCONTEXT
    short pfrCode);

GLOBAL_PROTO
unsigned short CspGetFontCount(
    PCSPCONTEXT1);


#if CSP_MAX_DYNAMIC_FONTS > 0
GLOBAL_PROTO
unsigned short CspGetUniquePhysicalFontCount(
    PCSPCONTEXT1);
#endif

#if CSP_MAX_DYNAMIC_FONTS > 0
GLOBAL_PROTO
int CspCreateDynamicFont(
    PCSPCONTEXT
    char *pFontID,
    short attrOutlRes,
    fontAttributes_t *pFontAttributes,
    unsigned short *pFontCode);
#endif

#if CSP_MAX_DYNAMIC_FONTS > 0
GLOBAL_PROTO
int CspDisposeDynamicFont(
    PCSPCONTEXT
    unsigned short fontCode);
#endif

#if CSP_MAX_DYNAMIC_FONTS > 0
GLOBAL_PROTO
unsigned short CspGetDynamicFontCount(
    PCSPCONTEXT1);
#endif

GLOBAL_PROTO
int CspSetFont(
    PCSPCONTEXT
    unsigned short fontCode);

GLOBAL_PROTO
int CspGetFontSpecs(
    PCSPCONTEXT
    unsigned short *pFontRefNumber,
    cspFontInfo_t *pFontInfo,
    fontAttributes_t *pFontAttributes);

GLOBAL_PROTO
int CspListChars(
    PCSPCONTEXT
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM)
    );

#if REENTRANT
GLOBAL_PROTO
int CspListCharsAlt1(
    PCSPCONTEXT
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM)
    USERPARAM
    );
#endif

GLOBAL_PROTO
int CspGetCharSpecs(
    PCSPCONTEXT
    unsigned short charCode,
    cspCharInfo_t *pCharInfo);

GLOBAL_PROTO
int CspSetOutputSpecs(
    PCSPCONTEXT
    outputSpecs_t *pOutputSpecs);

#if REENTRANT
GLOBAL_PROTO
int CspSetOutputSpecsAlt1(
    PCSPCONTEXT
    outputSpecs_t *pOutputSpecs
    USERPARAM);
#endif

#define BITMAP_OUTPUT      		1
#define OUTLINE_OUTPUT     		2
#define DIRECT_OUTPUT      		3
#define ANTIALIASED_OUTPUT 		4
#define ANTIALIASED_ALT1_OUTPUT 5

#if INCL_CACHE
GLOBAL_PROTO
int CspIsCacheEnabled(PCSPCONTEXT1);
#endif

GLOBAL_PROTO
int CspDoChar(
    PCSPCONTEXT
    unsigned short charCode,
    long *pXpos,
    long *pYpos);

GLOBAL_PROTO
int CspDoCharWidth(
    PCSPCONTEXT
    unsigned short charCode,
    long *pXpos,
    long *pYpos);

GLOBAL_PROTO
int CspSetMissingChar(
    PCSPCONTEXT
    unsigned short charCode);

GLOBAL_PROTO
int CspUnsetMissingChar(PCSPCONTEXT1);

GLOBAL_PROTO
int CspDoString(
    PCSPCONTEXT
    unsigned short modeFlags,
    short length,
    void *pString,
    long *pXpos,
    long *pYpos);

GLOBAL_PROTO
int CspDoStringWidth(
    PCSPCONTEXT
    unsigned short modeFlags,
    short length,
    void *pString,
    long *pXpos,
    long *pYpos);

/* modeFlags constants for string functions */
#define CSP_STRING_16          0x0001
#define CSP_ROUND_X_ESCAPEMENT 0x0002
#define CSP_ROUND_Y_ESCAPEMENT 0x0004

GLOBAL_PROTO
int CspClose(PCSPCONTEXT1);

/***** End of public function prototypes *****/


/***** Undocumented API functions *****/

#if INCL_UNDOCUMENTED_API

GLOBAL_PROTO
int CspTransformPoint(
    PCSPCONTEXT
    long   x,
    long   y,
    long   *pXt,
    long   *pYt);

GLOBAL_PROTO
int CspSetFontSpecs(
    PCSPCONTEXT
    fontAttributes_t *pFontAttributes);

GLOBAL_PROTO
int CspGetScaledCharBBox(
    PCSPCONTEXT
    unsigned short charCode,
    CspScaledBbox_t *pBBox);
    
#endif

/***** Undocumented functions used only by TrueDoc Printing Systems *****/

#if INCL_TPS

GLOBAL_PROTO
int CspGetRawCharWidth(
    PCSPCONTEXT
    unsigned short charCode,
    short *pXWidth,
    short *pYWidth);

#if REENTRANT
GLOBAL_PROTO
void *GetspGlobalPtr(
    PCSPCONTEXT1);
#endif

GLOBAL_PROTO
unsigned short CspGetLogicalFontIndex(
    PCSPCONTEXT1);

#endif


/***** Error return definitions *****/
#define CSP_NO_ERR               0   /* Normal return */
#define CSP_CALL_SEQUENCE_ERR    1   /* API call sequence error */
#define CSP_BUFFER_OVERFLOW_ERR  2   /* Memory buffer overflow */
#define CSP_READ_RESOURCE_ERR    3   /* ReadResourceData() returned error */
#define CSP_PFR_FORMAT_ERR       4   /* Portable Font Resource format error */
#define CSP_FONT_CODE_ERR        5   /* Font code out of range */
#define CSP_LIST_CHAR_FN_ERR     6   /* ListCharFn() returned error */
#define CSP_CHAR_CODE_ERR        7   /* Char not found in current font */
#define CSP_OUTPUT_TYPE_ERR      8   /* Undefined output type */
#define CSP_FONT_STYLE_ERR       9   /* Illegal value for fontStyle */
#define CSP_LINE_JOIN_TYPE_ERR  10   /* Illegal value for lineJoinType */
#define CSP_PFR_CODE_ERR        11   /* PFR code out of range */
#define CSP_FONT_ID_ERR         12   /* Font ID not found */
#define CSP_DYN_FONT_OFLO_ERR   13   /* Dynamic font table overflow */
#define CSP_PIXEL_SIZE_ERR      14   /* Pixel size out of range */
#define CSP_FILTER_WIDTH_ERR    15   /* Filter width out of range */
#define CSP_FILTER_HEIGHT_ERR   16   /* Filter height out of range */
#define CSP_X_POSN_SUBDIV_ERR   17   /* X position accuracy out of range */
#define CSP_Y_POSN_SUBDIV_ERR   18   /* Y position accuracy out of range */

#define CSP_CACHE_ERR           -1   /* Internal bitmap cache error */
#define CSP_PFR_BMAP_ERR        -2   /* Internal PFR bitmap error */

