/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/**************************** C S P _ I N T . H ******************************
 *                                                                           *
 * Character shape player internal header file.                              *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_INT.H 7.11 1997/03/18 16:04:27 SHAWN release $
 *                                                                                    
 *     $Log: CSP_INT.H $
 *     Revision 7.11  1997/03/18 16:04:27  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.10  1997/02/10 22:27:30  JohnC
 *     Added definitions connected with adjustment vector interpretation in compressed PFRs.
 *     Revision 7.9  1997/01/14 21:20:29  JohnC
 *     Revision 7.8  1996/12/13 16:09:01  john
 *     Added globals for compressed glyph program string interpreter.
 * Revision 7.7  96/11/14  16:50:30  john
 * Updates to support compressed PFR support.
 * 
 * Revision 7.6  96/11/08  15:06:09  john
 * Changed default MIN_GRAY from 64 to 0.
 * Added globals to keep track of current gps buffer contents
 *     to avoid redundant loading.
 * 
 * Revision 7.5  96/11/06  17:25:31  john
 * Globals related to hint interpretation and PFR bitmap handling
 *     are now conditionally compiled.
 * 
 * Revision 7.4  96/11/06  12:50:39  john
 * Made PFR bitmap handling functionality conditionally compilable.
 * 
 * Revision 7.3  96/11/05  17:50:04  john
 * Added function prototype for CspSetTcb().
 * 
 * Revision 7.2  96/10/29  22:33:59  john
 * Added globals used by bitmap filter module.
 * Removed function protos for outline and bitmap delivery functions.
 * 
 * Revision 7.1  96/10/10  14:05:24  mark
 * Release
 * 
 * Revision 6.2  96/09/09  16:38:25  john
 * Removed "typedef" from structure declarations already typedef'd.
 * 
 * Revision 6.1  96/08/27  11:57:58  mark
 * Release
 * 
 * Revision 5.8  96/08/07  17:33:18  roberte
 * Corrected to ANSI spec forward reference decls of:
 * cspGlobals_tag, intOutlineFns_tag, intBitmapFns_tag,
 * bmapCallbackFns_tag, outlCallbackFns_tag and dirCallbackFns_tag
 * This problem is known as "mutually-referring structures" and is
 * related to rules regarding "function prototype scope." The problem is
 * avoided by typedef'ing the struct <something tag> to a type, then
 * when the <something tag> is typedef'ed, the type name is not inserted
 * before the semi-colon.
 * 
 * Revision 5.7  96/06/20  17:24:13  john
 * Updates in support of CspListCharsAlt1() and CspSetOutputSpecsAlt1().
 * 
 * Revision 5.6  96/05/31  17:12:53  john
 * Added definitions for stem snap table support.
 * 
 * Revision 5.5  96/04/10  11:36:55  shawn
 * Restructured MACROs for sp_globals/sp_intercepts and added an sp_intercepts
 *      MACRO for the case of 'INCL_TPS && REENTRANT'.
 * 
 * Revision 5.4  96/03/29  16:16:18  john
 * Added global glyphSpecs to support compound character
 *     structure delivery in outline output mode.
 * 
 * Revision 5.3  96/03/26  17:52:50  john
 * Added unique physical font stuff.
 * 
 * Revision 5.2  96/03/21  11:43:01  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:20:59  mark
 * Release
 * 
 * Revision 4.2  96/03/14  14:11:34  mark
 * rename PCONTEXT to PCSPCONTEXT 
 * 
 * Revision 4.1  96/03/05  13:45:52  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:29:30  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:46:27  mark
 * Release
 * 
 * Revision 1.5  95/11/07  13:41:12  john
 * Internal character hint structure extended to support
 * horizontal and vertical secondary edges.
 * Separated oru tables into basic tables used for controlled
 * coordinate lookup and interpolation table used for marking
 * discontinuities in piecewise-linear transformations.
 * 
 * Revision 1.4  95/09/18  18:13:09  john
 * Added globals for the missing character mechanism.
 * 
 * Revision 1.3  95/09/14  14:54:59  john
 * Added default blueShift value of .007 em.
 * Removed unused names of PFR header fields.
 * Added new globals blueShiftPix and bluePixRnd.
 * 
 * Revision 1.2  95/08/31  09:21:32  john
 * Globals for stroke/bold processing updated for new module.
 * Prototype for CspSplitCurve updated to include new arg.
 * 
 * Revision 1.1  95/08/10  16:45:10  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/


#include "csp_api.h"
#include "btstypes.h"


/* Min scan conversion list elements per dpi of device resolution */
#ifndef MIN_LIST_SIZE_FACTOR
#define MIN_LIST_SIZE_FACTOR  6     
#endif

/* Absolute miter limit */
#ifndef ABS_MITER_LIMIT
#define ABS_MITER_LIMIT   32768     /* Units of 1/65535 em */
#endif

/* Minimum gray value for pixel with white neighbors in scanline */
#ifndef MIN_GRAY
#define MIN_GRAY              0     /* Units of 1/256 black */
#endif

/* Default value for blue shift */
#ifndef BLUE_SHIFT
#define BLUE_SHIFT          459	    /* Units of 1/65536 em */
#endif

/* Default filter function buffer */
#ifndef FILTER_FN_BUFF_SIZE
#define FILTER_FN_BUFF_SIZE 1024
#endif

#if INCL_TPS

#if REENTRANT    

#define GLOBALPTR1 sp_globals_t *spGlobalPtr
#define GLOBALPTR GLOBALPTR1,
#define GDECL sp_globals_t *sp_global_ptr;

#else

#define GLOBALPTR1 void
#define GLOBALPTR
#define GDECL

#endif /* REENTRANT */

#else

#if REENTRANT

#define GLOBALPTR1 void *pCspGlobals
#define GLOBALPTR GLOBALPTR1,
#define GDECL cspGlobals_t *pCspGlobals;

#else

#define GLOBALPTR1 void
#define GLOBALPTR
#define PARAMS1 pCspGlobals
#define PARAMS2 PARAMS1,
#define GDECL cspGlobals_t *pCspGlobals;

#endif /* REENTRANT */

#define PROTO_DECL1 void
#define PROTO_DECL2
#define PARAMS1 pCspGlobals
#define PARAMS2 PARAMS1,

#endif /* INCL_TPS */


/***** MACRO DEFINITIONS *****/
#define NEXT_BYTE(P) (*(P++))
#define NEXT_WORD(P) (P+=2, ((fix15)P[-2] << 8) + P[-1])
#define NEXT_LONG(P) (P+=3, ((ufix32)P[-3] << 16) + ((ufix32)P[-2] << 8) + P[-1])


/***** STRUCTURE DEFINITIONS *****/

/* Forward reference tag typedefs: */
typedef struct cspGlobals_tag cspGlobals_t;
typedef struct intOutlineFns_tag intOutlineFns_t;
typedef struct intBitmapFns_tag intBitmapFns_t;
typedef struct bmapCallbackFns_tag bmapCallbackFns_t;
typedef struct outlCallbackFns_tag outlCallbackFns_t;
#if INCL_DIR
typedef struct dirCallbackFns_tag dirCallbackFns_t;
#endif

/* Memory requirements data extracted from PFR headers */
typedef struct pfrHeaderData_tag
    {
    fix31   minListElements;     /* Minimum size of scan conversion lists */ 
    ufix16  logFontDirSize;      /* Size of logical font directory */
    ufix16  logFontMaxSize;      /* Size of largest logical font record */
    fix31   logFontSectionSize;  /* Size of logical font section of PFR */
    fix31   physFontMaxSize;     /* Size of largest physical font record */
    fix31   bctMaxSize;          /* Size of largest bitmap char table */
    fix31   bctSetMaxSize;       /* Size of largest set of bitmap char tables */
    fix31   pftBctSetMaxSize;    /* Size of largest phys font w/ bmap char tbls */
    fix31   physFontSectionSize; /* Size of physical font section of PFR */
    ufix16  gpsMaxSize;          /* Size of largest glyph prog string */
    fix31   gpsSectionSize;      /* Size of glyph prog string section of PFR */
    ufix8   maxBlueValues;       /* Size of required blue value tables */
    ufix8   maxXorus;            /* Size of required controlled X tables */
    ufix8   maxYorus;            /* Size of required controlled Y tables */
    ufix8   bmapFlags;           /* Bitmap flags */
    fix31   totalPhysFonts;      /* Total number of physical fonts available */
    ufix8   maxStemSnapVsize;    /* Size of largest stemSnapV table */
    ufix8   maxStemSnapHsize;    /* Size of largest stemSnapH table */
    ufix16	maxChars;			 /* Max number of chars in any compressed PFR font */
    } pfrHeaderData_t;

/* Internal PFR access table */
typedef struct pfrTable_tag
    {
    short   mode;                /* PFR access mode */
    cspPfrAccessInfo_t access;   /* PFR access information */
    ufix16  version;             /* PFR version number */
#if INCL_COMPRESSED_PFR
    ufix8	pfrType;			 /* PFR type (0 = standard; 1 = compressed) */
#endif
    ufix8   bmapFlags;           /* Bitmap flags */
    ufix16  baseFontCode;        /* Global font code of first log font */
    ufix16  nLogicalFonts;       /* Number of logical fonts in PFR */
    fix31   logFontFirstOffset;  /* Offset to first logical font */
    ufix8  *pFirstLogFont;       /* Pointer to first logical font */
    ufix16  nPhysFonts;          /* Number of physical fonts in PFR */
    fix31   physFontFirstOffset; /* Offset to first physical font */
    ufix8  *pFirstPhysFont;      /* Pointer to first physical font */
    fix31   gpsFirstOffset;      /* Offset to first glyph program string */
    ufix8  *pFirstGps;           /* Pointer to first glyph program string */
#if CSP_MAX_DYNAMIC_FONTS > 0
    ufix16  baseFontRefNumber;   /* Global phys font code of first phys font */
#endif
    } pfrTable_t;

/* Bitmap flags bit assignments */
#define PFR_BLACK_PIXEL BIT_0    /* Bit representation of black pixel */
#define PFR_INVERT_BMAP BIT_1    /* Set if bitmaps in decreasing Y order */

/* Character record */
typedef struct charRec_tag
	{
	ufix16	charCode;
	fix15	setWidth;
	ufix16	gpsSize;
	fix31	gpsOffset;
	} charRec_t;

#if CSP_MAX_DYNAMIC_FONTS > 0
/* Physical font record */
typedef struct physFont_tag
    {
    fix15   pfrCode;
    ufix16  nextPhysFont;
    fix31   offset;
    fix31   size;
    } physFont_t;

/* Dynamic font record */
typedef struct newFont_tag
    {
    ufix16  fontCode;
    ufix16  physFontCode;
    fontAttributes_t fontAttributes;
    CspBbox_t fontBBox;
    unsigned short nCharacters;
    fix15   outlineResolution;
    } newFont_t;
#endif

/* Blue zone structure */
#if INCL_CSPHINTS
typedef struct blueZone_tag
    {
    fix15   minPix;             /* Lower limit of snapping zone */
    fix15   maxPix;             /* Upper limit of snapping zone */
    fix15   refPix;             /* Unrounded alignment value */
    } blueZone_t;
#endif

/* Stem snap structure */
#if INCL_CSPHINTS
typedef struct stemSnapZone_tag
    {
    fix15   minPix;             /* Lower limit of snapping zone */
    fix15   maxPix;             /* Upper limit of snapping zone */
    fix15   refPix;             /* Unrounded alignment value */
    } stemSnapZone_t;
#endif

/* Compound character element record */
typedef struct element_tag
    {
    fix15  	xScale;             /* X scale factor in CSP_SCALE_SHIFT units */
    fix15  	yScale;             /* Y scale factor in CSP_SCALE_SHIFT units */
    fix15  	xPosition;          /* X position in orus */
    fix15  	yPosition;          /* Y position in orus */
    fix31  	offset;             /* Offset to element gps */
    ufix16 	size;              	/* Size of element gps */
    } element_t;

#define CSP_SCALE_SHIFT 12
#define CSP_SCALE_RND (1 << 11)

/* Point structure */
#if ! INCL_TPS
typedef struct point_tag
    {
    fix15  x;
    fix15  y;
    } point_t;
#endif

/* Character hint structures */
typedef struct vStem_tag
    {
    fix15   x1;
    fix15   x2;
    } vStem_t;

typedef struct hStem_tag
    {
    fix15   y1;
    fix15   y2;
    } hStem_t;

typedef struct vEdge_tag
    {
    fix15   x;
    fix15   dx;
    fix15   thresh;
    } vEdge_t;

typedef struct hEdge_tag
    {
    fix15   y;
    fix15   dy;
    fix15   thresh;
    } hEdge_t;

typedef struct charHint_tag
	{
    fix15   type;
    union
        {
        vStem_t vStem;
        hStem_t hStem;
        vEdge_t vEdge;
        hEdge_t hEdge;
        } hint;
    } charHint_t;

#define CSP_VSTEM         1
#define CSP_HSTEM         2
#define CSP_VSTEM2        3
#define CSP_HSTEM2        4
#define CSP_V_LEAD_EDGE2  5
#define CSP_V_TRAIL_EDGE2 6
#define CSP_H_LEAD_EDGE2  7
#define CSP_H_TRAIL_EDGE2 8

/* Simplified specs structure for output module compatibility */
typedef struct CspSpecs_tag
    {
    fix31  ctm[4];
    ufix32 flags;
    } CspSpecs_t;

/* Flags definitions for simplified specs structure */
#define CURVES_OUT     0X0008  /* Output module accepts curves              */
#define BOGUS_MODE     0X0010  /* Linear scaling mode                       */
#define CONSTR_OFF     0X0020  /* Inhibit constraint table                  */
#define IMPORT_WIDTHS  0X0040  /* Imported width mode                       */
#define SQUEEZE_LEFT   0X0100  /* Squeeze left mode                         */
#define SQUEEZE_RIGHT  0X0200  /* Squeeze right mode                        */
#define SQUEEZE_TOP    0X0400  /* Squeeze top mode                          */
#define SQUEEZE_BOTTOM 0X0800  /* Squeeze bottom mode                       */
#define CLIP_LEFT      0X1000  /* Clip left mode                            */
#define CLIP_RIGHT     0X2000  /* Clip right mode                           */
#define CLIP_TOP       0X4000  /* Clip top mode                             */
#define CLIP_BOTTOM    0X8000  /* Clip bottom mode                          */

/* Speedo global data structure moved to external file */

#if INCL_TPS
#include "speedo.h"
#else
#include "spglobal.h"
#endif

/* Set of pointers to internal outline processing functions */
struct intOutlineFns_tag
    {
    btsbool (*InitOut)(
        cspGlobals_t *pCspGlobals,
        CspSpecs_t *pSpecs);
    btsbool (*BeginChar)( 
        cspGlobals_t *pCspGlobals,
        point_t Psw,
        point_t Pmin,
        point_t Pmax);
    void (*BeginSubChar)(
        cspGlobals_t *pCspGlobals,
        point_t Psw,
        point_t Pmin,
        point_t Pmax);
    void (*CharHint)(
        cspGlobals_t  *pCspGlobals,
        charHint_t *pCharHint);
    void (*BeginContour)(
        cspGlobals_t *pCspGlobals,
        point_t P,
        btsbool outside);
    void (*CurveTo)(
        cspGlobals_t *pCspGlobals,
        point_t P1,
        point_t P2,
        point_t P3,
        fix15 depth);
    void (*LineTo)(
        cspGlobals_t *pCspGlobals,
        point_t P);
    void (*EndContour)(cspGlobals_t *pCspGlobals);
    btsbool (*EndSubChar)(cspGlobals_t *pCspGlobals);
    btsbool (*EndChar)(cspGlobals_t *pCspGlobals);
    };

/* Set of pointers to internal bitmap processing functions */
struct intBitmapFns_tag
    {
    void (*SetBitmap)(
        cspGlobals_t *pCspGlobals,
        bmapSpecs_t bmapSpecs,
        void *pBitmap);
    void (*OpenBitmap)(
        cspGlobals_t *pCspGlobals,
        long int xEsc,
        long int yEsc,
        long int xOrg,
        long int yOrg,
        short int xSize,
        short int ySize);
    void (*SetBitmapBits)(
        cspGlobals_t *pCspGlobals,
        short y,
        short xStart,
        short xEnd);
    void (*SetBitmapPixels)(
        cspGlobals_t *pCspGlobals,
        fix15 y,
        fix15 xStart,
        fix15 xSize,
        void *pPixels);
    void (*CloseBitmap)(
        cspGlobals_t *pCspGlobals);
    };
    
/* Set of pointers to bitmap output functions */
struct bmapCallbackFns_tag
    {
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
    void (*SetBitmapPixels)(
        short y,
        short xStart,
        short xSize,
        void *pPixels
        USERPARAM);
    void (*CloseBitmap)(USERPARAM1);
    };

/* Set of pointers to outline output functions */
struct outlCallbackFns_tag
    {
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
    };
    
/* Set of pointers to direct output functions */
#if INCL_DIR
#if INCL_TPS
#if REENTRANT
#define DIRECTARG1 sp_globals_t *pSpGlobals,
#define DIRECTARG sp_globals_t *pSpGlobals
#else
#define DIRECTARG1 
#define DIRECTARG void
#endif
#else
#define DIRECTARG1 void *pCspGlobals,
#define DIRECTARG void *pCspGlobals
#endif

struct dirCallbackFns_tag 
    {
    btsbool (*InitOut)(
        DIRECTARG1
        specs_t *specsArg);
    btsbool (*BeginChar)(
        DIRECTARG1
        point_t Psw,
        point_t Pmin,
        point_t Pmax
        );
    void (*BeginSubChar)(
        DIRECTARG1
        point_t Psw,
        point_t Pmin,
        point_t Pmax
        );
    void (*BeginContour)(
        DIRECTARG1
        point_t P1,
        btsbool outside
        );
    void (*CurveTo)(
        DIRECTARG1
        point_t P1,
        point_t P2,
        point_t P3,
        fix15 depth
        );
    void (*LineTo)(
        DIRECTARG1 
        point_t P1
        );
    void (*EndContour)(
        DIRECTARG
        );
    void (*EndSubChar)(
        DIRECTARG
        );
    btsbool (*EndChar)(
        DIRECTARG
        );
    };
#endif

#if INCL_STROKE || INCL_BOLD
/* Segment structure */
typedef struct segment_tag
    {
    fix15   type;           /* Type of segment */
    point_t ptIn;           /* Start point in segment */
    point_t ptOut;          /* End point in segment */
    point_t dirVectIn;      /* Direction vector at start point */
    point_t dirVectOut;     /* Direction vector at end point */
    point_t normVectIn;     /* Normal const vector at start point (subpix) */
    point_t normVectOut;    /* Normal const vector at end point (subpix) */
    point_t normVectInOrus; /* Normal const vector at start point (orus) */
    point_t normVectOutOrus; /* Normal const vector at end point (orus) */
    point_t ptC1;           /* First curve control point */
    point_t ptC2;           /* Second curve control point */
    } segment_t;

/* Segment type definitions */
#define CSP_LINE_SEG 0
#define CSP_CRVE_SEG 1
#endif

#if INCL_ANTIALIASED_OUTPUT
typedef struct pixelProps_tag
    {
    fix15   xOrg;       /* Left edge of pixel (sub-pixel units) */
    fix15   yOrg;       /* Bottom edge of pixel (sub-pixel units) */
    fix15   xIn;        /* Local X coord of contour entry point */
    fix15   yIn;        /* Local Y coord of contour entry point */
    fix15   xOut;       /* Local X coord of contour exit point */
    fix15   yOut;       /* Local Y coord of contour exit point */
    fix31   area;       /* Pixel area within contour */
    ufix8   edgeCodes;  /* Edge codes for entry and exit */
    } pixelProps_t;
#endif

/* Nibble pointer type definition */
#if INCL_COMPRESSED_PFR
typedef struct nibble_tag
	{
	ufix8 *pByte;
	fix15	phase;
	} nibble_t;
#endif	

/* Global variables structure */

struct cspGlobals_tag
    {
    ufix8  *pMemoryOrigin;
    ufix8  *pAllocLimitLo;      /* First address in main buffer */
    ufix8  *pAllocLimitHi;      /* Last address plus one in main buffer */
    ufix8  *pAllocNextLo;       /* First address at low end of free area */
    ufix8  *pAllocNextHi;       /* Last address plus one in free area */

    ufix16  cspState;           /* Current API state */
    fix15   userPfrCode;        /* Client-selected PFR */
    fix15   pixelSize;          /* Number of bits per pixel */
    ufix16  modeFlags;          /* Buffering mode flags */
    options_t outlineOptions;   /* Outline options structure */
    btsbool reverseContour;     /* Reverse current contour */

    /* Pfr access stuff */
    fix15   nPfrs;              /* Number of PFRs active */
    pfrTable_t *pfrTable;       /* Pointer to PFR table */
    ufix16  nLogicalFonts;      /* Total number of logical fonts */
    ufix8  *pLogFontDir;        /* Pointer to logical font directory */
    ufix16  nPhysFonts;         /* Total number of physical fonts */
    ufix16 *pFontIDtable;       /* Hash table for access by font ID */

    /* Current logical font specs */
    ufix16  fontCode;           /* Global font code of current logical font */
    fontAttributes_t fontAttributes; /* Current font attributes */

    /* Current physical font data */
    ufix16  fontRefNumber;      /* Local physical font number */
    fix15   physFontPfrCode;    /* PFR code of current physical font */
    fix31   physFontOffset;     /* Offset of current physical font */
    fix31   physFontSize;       /* Size of current physical font */
    cspFontInfo_t fontInfo;     /* Current physical font information */
    CspBbox_t adjFontBBox;      /* Adjusted font bounding box */
    fix15  *pAdjBlueValues;     /* Adjusted blue values */
    btsbool verticalEscapement; /* Vertical escapement flag */
    fix15   standardSetWidth;   /* Standard set width for monospace fonts */
    ufix8  *pPftExtraItems;     /* Pointer to extra data items */
    ufix8  *pFirstChar;         /* Pointer to start of character table */
    ufix8   charFormat;         /* Format byte for character table */
    fix15   charRecSize;        /* Size of character table records */
    fix15	gpsPfrCode;			/* PFR code of current glyph prog string */
    fix31	gpsOffset;			/* Offset of current glyph prog string */
   	fix31   gpsSize;            /* Number of bytes in gps buffer */
#if INCL_COMPRESSED_PFR
   	charRec_t *pCharTable;		/* Unpacked character table */
#endif
	int (*ExecChar)(
		struct cspGlobals_tag *pCspGlobals, 
    	unsigned short charCode,
    	btsbool clipped);		/* Pointer to current ExecChar() */
	int (*FindChar)(
		struct cspGlobals_tag *pCspGlobals, 
		struct charRec_tag *pCharRec); /* Pointer to current FindChar() */
    
	/* Hint interpretation globals */
#if INCL_CSPHINTS
    fix15   nBlueZones;         /* Number of alignment zones */
    blueZone_t *pBlueZones;     /* Alignment data for current size */
    fix15   blueShiftPix;       /* Current value of blue shift in pixels */
    fix15   bluePixRnd;         /* Biased rounding constant for top zone alignment */
    fix15   nStemSnapVzones;    /* Number of vertical stem snap zones */
    stemSnapZone_t *pStemSnapVzones; /* Vert stem snap table for current size */
    fix15   nStemSnapHzones;    /* Number of horizontal stem snap zones */
    stemSnapZone_t *pStemSnapHzones; /* Horiz stem snap table for current size */
#endif
    
	/* PFR bitmap handling globals */
#if INCL_PFR_BITMAPS
    fix31   nBmapChars;         /* Number of bitmap characters in current font/size */
    ufix8  *pFirstBmapChar;     /* Pointer to start of bitmap character table */
    ufix8   bmapCharFormat;     /* Format byte for bitmap character table */
    fix15   bmapCharRecSize;    /* Size of bitmap character table records */
    ufix32  ppm;                /* Current bitmap size */
#endif

    /* Dynamic font globals */
#if CSP_MAX_DYNAMIC_FONTS > 0
    physFont_t *pPhysFontTable; /* Table of available physical fonts */
    ufix16  nNewFonts;          /* Current number of client-defined fonts */
    newFont_t *pNewFontTable;   /* New font table */
    ufix16  nextNewFontCode;    /* Next font code to be assigned */
    newFont_t *pNewFont;        /* Pointer to current new font */
    fix15   transFontPfrCode;   /* Phys font PFR code for current trans setup */
#endif

    /* Current output specs */
    outputSpecs_t outputSpecs;  /* Current output specs */

    /* Internal data routing function pointers */
    intOutlineFns_t rawOutlFns; /* Raw outline destinations */
    intOutlineFns_t embOutlFns; /* Emboldened outline destinations */
    intBitmapFns_t fltBmapFns;  /* Filtered bitmap destinations */
    intBitmapFns_t rawBmapFns;  /* Bitmap destinations */

    /* Saved pointers to callback functions */
    bmapCallbackFns_t bmapCallbackFns; /* Bitmap callback functions */
    outlCallbackFns_t outlCallbackFns; /* Outline callback functions */
#if INCL_DIR
    dirCallbackFns_t dirCallbackFns; /* Direct callback functions */
#endif

    point_t Pmin;               /* Min transformed point */
    point_t Pmax;               /* Max transformed point */

    unsigned short missingCharCode;
    btsbool missingCharActive;

    /* Current character data */
    fix31   charSetWidth;       /* Set width in units of 1/65536 em */
    fix15   nXorus;             /* Number of controlled X coordinates */
    fix15   nYorus;             /* Number of controlled Y coordinates */
    fix15  *pXorus;             /* X oru table */
    fix15  *pYorus;             /* Y oru table */
    fix15  *pXpix;              /* X pixel table */
    fix15  *pYpix;              /* Y pixel table */	
    fix15   nXintOrus;          /* Number of X interpolation breaks */
    fix15   nYintOrus;          /* Number of Y interpolation breaks */
    fix15  *pXintOrus;          /* X interpolation oru table */
    fix15  *pYintOrus;          /* Y interpolation oru table */
    fix15  *pXintMult;          /* X interpolation multiplier table */
    fix15  *pYintMult;          /* Y interpolation multiplier table */
    fix31  *pXintOffset;        /* X interpolation offset table */
    fix31  *pYintOffset;        /* Y interpolation offset table */
#if INCL_COMPRESSED_PFR
    fix15  *pOrigXorus;         /* Unadjusted X oru table */
    fix15  *pOrigYorus;         /* Unadjusted Y oru table */
	fix15	nBasicXorus;		/* Number of primary controlled X coords */
	fix15	nBasicYorus;		/* Number of primary controlled Y coords */
    fix15	xPrevValue;			/* Previous X coordinate value */
    fix15	xPrev2Value;		/* Second previous X coord value */
    fix15	yPrevValue;			/* Previous Y coordinate value */
    fix15	yPrev2Value;		/* Second previous Y coord value */
    fix15	adjMode;			/* Current point adjustment mode */
    fix15	xAdjValue;			/* Current X adjustment value */
    fix15	yAdjValue;			/* Current Y adjustment value */
    fix15	nZeros;				/* Number of pending zero adjustment values */
    fix15	xPrevOrigValue;		/* Previous unadjusted X coordinate */
    fix15	yPrevOrigValue;		/* Previous unadjusted Y coordinate */
    nibble_t adjNibble;			/* Nibble pointer to adjustment vector */
#endif

    /* TrueDoc variables moved out of sp_globals_t */
	fix15   depth_adj;          /* Curve splitting depth adjustment */
    fix31   multrnd;            /* 0.5 in multiplier units */
    fix31   xPosPix;            /* X position in 16.16 device coords */
    fix31   yPosPix;            /* Y position in 16.16 device coords */
    fix31   outerClipXmin;      /* Left edge of outer clip window */
    fix31   outerClipYmin;      /* Bottom edge of outer clip window */
    fix31   outerClipXmax;      /* Right edge of outer clip window */
    fix31   outerClipYmax;      /* Top edge of outer clip window */
    fix31   innerClipXmin;      /* Left edge of inner clip window */
    fix31   innerClipYmin;      /* Bottom edge of inner clip window */
    fix31   innerClipXmax;      /* Right edge of inner clip window */
    fix31   innerClipYmax;      /* Top edge of inner clip window */
    fix15   clipXmin;           /* Left edge of clip window */
    fix15   clipYmin;           /* Bottom edge of clip window */
    fix15   clipXmax;           /* Right edge of clip window */
    fix15   clipYmax;           /* Top edge of clip window */
    fix15   bmapLeft;           /* Left edge of bitmap image */
    fix15   bmapTop;            /* Top scanline of bitmap image */

    /* Outline output module globals */
#if INCL_CSPOUTLINE
    fix15   contour;            /* Current contour */
    fix15   requiredContour;    /* Next contour to be output */
    fix15   state;              /* Contour reverser state */
    fix15   nFullBlocks;        /* Number of full blocks of points */
    fix15   nPoints;            /* Number of points in last block */
    glyphSpecs_t glyphSpecs;    /* Glyph specifications */
#endif

	/* Emboldening module globals */
#if INCL_STROKE || INCL_BOLD
    fix31   matrix[4];          /* Current transformation matrix */
    fix31   imatrix[4];         /* Inverse transformation matrix */
    btsbool regular_trans;      /* Matrix is regular */
    btsbool outside;            /* Current contour surrounds ink */
    btsbool mirror;             /* Matrix reverses contour direction */
    fix15   halfStrokePix;      /* Half stroke weight in subpixels */
    fix15   halfStrokeOrus;     /* Half stroke weight in font coordinates */
    fix31   minDenom;           /* Miter ratio or single arc threshold */
    fix15   xGridAdj;           /* Horizontal grid alignment shift */
    fix15   yGridAdj;           /* Vertical grid alignment shift */
    btsbool firstSegment;       /* Current segment is first in contour */
    btsbool secondSegment;      /* Current segment is second in contour */
    btsbool firstPoint;         /* BeginContour not yet output */
    point_t boldP0;             /* Current point in outline */
    segment_t seg00;            /* First segment in contour */
    segment_t seg0;             /* Previous segment in contour */
#endif

	/* Stroke module globals */
#if INCL_STROKE
    btsbool firstPass;
#endif

	/* Antialiased output module globals */
#if INCL_ANTIALIASED_OUTPUT
    pixelProps_t startPixel;
    pixelProps_t currentPixel;
    band_t x_band;
    fix31   doublePixelArea;
    fix15   doublePixelHeight;
#if INCL_STD_BMAP_FILTER
	ufix8   defaultFilterFnBuff[FILTER_FN_BUFF_SIZE];
	ufix8   *pFilterFnBuff;
	fix31	filterFnBuffSize;
	fix15	currentScanline;
	fix15	filterFnRowSize;
#endif
#endif

	/* Bitmap cache globals */
#if INCL_CACHE
    void *pCmGlobals;
    btsbool cacheEnabled;
    fix31   cacheOutputMatrix[4];
    ufix32	cacheHintStatus;
#endif

#if REENTRANT
    long userArg;
    long userArgOutput;
#endif

#if !INCL_TPS || REENTRANT
    sp_globals_t spGlobals; /* only if not printing system or reentrant */
#endif
	};

/* sp_globals and sp_intercepts */
#if INCL_TPS

#if STATIC_ALLOC
extern sp_globals_t sp_globals;
#endif

#if DYNAMIC_ALLOC
extern sp_globals_t *sp_global_ptr;
#endif

#if REENTRANT_ALLOC
#ifdef sp_globals
#undef sp_globals
#endif
#define sp_globals (pCspGlobals->spGlobals)
#ifdef sp_intercepts
#undef sp_intercepts
#endif
#define sp_intercepts (*(pCspGlobals->spGlobals).intercepts)
#endif  /* REENTRANT_ALLOC */

#else   /* !INCL_TPS */

#ifdef sp_globals
#undef sp_globals
#endif
#define sp_globals (pCspGlobals->spGlobals)

#ifdef sp_intercepts
#undef sp_intercepts
#endif
#define sp_intercepts sp_globals

#endif  /* INCL_TPS */


#ifndef MAX_INTERCEPTS
#if INCL_TPS
#define MAX_INTERCEPTS 2000
#else
#define MAX_INTERCEPTS pCspGlobals->spGlobals.max_intercepts
#endif
#endif

#if REENTRANT

#define USERARG  , ((cspGlobals_t *)pCspGlobals)->userArg
#define USERARG2 , ((cspGlobals_t *)pCspGlobals)->userArgOutput

#else

#define USERARG  
#define USERARG2

#endif

/* Buffering mode flags bit allocations */
#define DYNLOGFNT       0x0001  /* Dynamic logical font loading enabled */
#define DYNPHYFNT       0x0002  /* Dynamic physical font loading enabled */
#define DYNBCT          0x0004  /* Dynamic bitmap char table loading enabled */
#define BMAPS_AVAIL     0x0008  /* Bitmaps available in current physical font */
#define DYNGPS          0x0010  /* Dynamic glyph prog string loading enabled */
#define LARGEPFTS       0x0020  /* Physical font records exceed 64K */
#define BMAPS_ACTIVE    0x0040  /* Bitmaps active in current font */

/* Current adjustment mode definitions */
#if INCL_COMPRESSED_PFR
#define X_ADJ_ACTIVE	1
#define Y_ADJ_ACTIVE	2
#endif


/***** Internal function prototypes *****/

/* Memory management functions */

GLOBAL_PROTO
btsbool CspInitAlloc(
    cspGlobals_t **pCspGlobals,
    void *pBuffer,
    long buffSize);

GLOBAL_PROTO
void *CspAllocFixedLo(
    cspGlobals_t *pCspGlobals,
    fix31 nBytes);

GLOBAL_PROTO
fix31 CspMemoryLeft(
    cspGlobals_t *pCspGlobals);

/* Main internal functions */

GLOBAL_PROTO
int CspGetPfrHeaderData(
    short deviceResolution,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData
    USERPARAM);

GLOBAL_PROTO
void CspDoMemoryBudget(
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData,
    fix31 *pMinBuffSize,
    fix31 *pNormalBuffSize,
    fix31 *pMaxBuffSize);

GLOBAL_PROTO
int CspSetupMemory(
    cspGlobals_t *pCspGlobals,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    pfrHeaderData_t *pPfrHeaderData);

GLOBAL_PROTO
int CspLoadLogicalFont(
    cspGlobals_t *pCspGlobals,
    fix15 pfrCode,
    ufix16 fontCode);

GLOBAL_PROTO
int CspLoadPhysicalFont(
    cspGlobals_t *pCspGlobals,
    fix15 pfrCode,
    fix31 physFontOffset,
    fix31 physFontSize);

GLOBAL_PROTO
ufix8 *CspLoadGps(
    cspGlobals_t *pCspGlobals,
    ufix16 size,
    fix31 offset);

GLOBAL_PROTO
ufix8 *CspAddGps(
    cspGlobals_t *pCspGlobals,
    ufix16 size,
    fix31 offset,
    fix31 buffOffset);

GLOBAL_PROTO
int CspDoSetup(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
void CspSetTrans(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
void CspScaleTrans(
    cspGlobals_t *pCspGlobals,
    element_t *pElement);

GLOBAL_PROTO
void CspSetTcb(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
void CspSetContourDirection(
    cspGlobals_t *pCspGlobals,
    btsbool outside);

GLOBAL_PROTO
void CspTransBBox(
    cspGlobals_t *pCspGlobals,
    CspBbox_t *pFontBBox);
    
GLOBAL_PROTO
void CspSetClipWindows(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
void CspClipFontBBox(
    cspGlobals_t *pCspGlobals,
    long *pXpos,
    long *pYpos);

GLOBAL_PROTO
int CspExecChar(
    cspGlobals_t *pCspGlobals,
    unsigned short charCode,
    btsbool clipped);

GLOBAL_PROTO
int CspFindChar(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec);

#if INCL_COMPRESSED_PFR
GLOBAL_PROTO
int CspExecCompressedChar(
    cspGlobals_t *pCspGlobals,
    unsigned short charCode,
    btsbool clipped);

GLOBAL_PROTO
int CspFindCompressedChar(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec);

GLOBAL_PROTO
fix15 CspGetAdjustmentValue(
	cspGlobals_t *pCspGlobals);
	
#endif

GLOBAL_PROTO
void CspSplitCurve(
    cspGlobals_t *pCspGlobals,
    point_t P0,
    point_t P1,
    point_t P2,
    point_t P3,
    fix15   depth,
    void (*LineTo)(
        cspGlobals_t *pCspGlobals,
        point_t P));

GLOBAL_PROTO
void CspSetFontHints(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
void CspDoStrokes(
    cspGlobals_t *pCspGlobals,
    ufix8 *pExtraItems);


/* Functions associated with dynamic fonts */

#if CSP_MAX_DYNAMIC_FONTS > 0

GLOBAL_PROTO
fix31 CspSizeNewFontTables(
    pfrHeaderData_t *pPfrHeaderData);

GLOBAL_PROTO
int CspSetupNewFontTables(
    cspGlobals_t *pCspGlobals,
    pfrHeaderData_t *pPfrHeaderData);

GLOBAL_PROTO
int CspCreateNewFont(
    cspGlobals_t *pCspGlobals,
    char *pFontID,
    short attrOutlRes,
    fontAttributes_t *pFontAttributes,
    unsigned short *pFontCode);

GLOBAL_PROTO
int CspDisposeNewFont(
    cspGlobals_t *pCspGlobals,
    unsigned short fontCode);

GLOBAL_PROTO
int CspSetNewFont(
    cspGlobals_t *pCspGlobals,
    ufix16 fontCode);

GLOBAL_PROTO
int CspGetNewFontSpecs(
    cspGlobals_t *pCspGlobals,
    unsigned short *pFontRefNumber,
    cspFontInfo_t *pFontInfo,
    fontAttributes_t *pFontAttributes);

GLOBAL_PROTO
int CspListNewFontChars(
    cspGlobals_t *pCspGlobals,
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM),
    long userArg);

GLOBAL_PROTO
int CspLoadTopPhysFont(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
int CspFindNewFontChar(
    cspGlobals_t *pCspGlobals,
    charRec_t *pCharRec);

#endif


/* PFR bitmap processing function */

#if INCL_PFR_BITMAPS

GLOBAL_PROTO
int CspLoadBmapCharTable(
    cspGlobals_t *pCspGlobals);

GLOBAL_PROTO
int CspExecCharBitmap(
    cspGlobals_t *pCspGlobals,
    unsigned short charCode,
    btsbool clipped);
    
#endif

/* Utility functions */

GLOBAL_PROTO
fix15 CspReadByte(ufix8 *pBuff);

GLOBAL_PROTO
fix15 CspReadWord(ufix8 *pBuff);

GLOBAL_PROTO
ufix32 CspReadLong(ufix8 *pBuff);

GLOBAL_PROTO
fix31 CspLongMult(
    fix31 x, 
    fix31 y);

/**** End of internal function prototypes *****/

