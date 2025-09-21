/*****************************************************************************
*                                                                            *
*                        Copyright 1993 - 97                                 *
*          as an unpublished work by Bitstream Inc., Cambridge, MA           *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/



/*************************** C S P _ A P I . C *******************************
 *                                                                           *
 * Character shape player application program interface module.              *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  Changes since TrueDoc Release 2.0:                                       *
 *
 *     $Header: r:/src/CSP/rcs/CSP_API.C 7.10 1997/03/18 16:04:18 SHAWN release $
 *                                                                                    
 *     $Log: CSP_API.C $
 *     Revision 7.10  1997/03/18 16:04:18  SHAWN
 *     TrueDoc Imaging System Version 6.0
 *     Revision 7.9  1997/02/11 18:04:52  JohnC
 *     Made CspIsCacheEnabled() conditional on INCL_CACHE
 *     Revision 7.8  1997/02/06 19:26:01  MARK
 *     Add CspIsCacheEnabled function to return the state of
 *      pCspState->cacheEnabled
 *     Revision 7.7  1996/12/13 15:58:58  john
 *     Fixed bug in CspGetRawCharWidth() when using compressed PFR.
 * Revision 7.6  96/11/20  13:24:32  john
 * Updated ListChars function to be independent of PFR format
 *     by using FindChar rather than direct char list decoding.
 * 
 * Revision 7.5  96/11/14  16:45:48  john
 * Calls to CspExecChar and CspFindChar changed to use global function
 *     pointers to select the appropriate gps and char table format.
 * 
 * Revision 7.4  96/11/06  12:46:36  john
 * Eliminated compilation of undocumented API functions by default.
 * 
 * Revision 7.3  96/11/05  17:43:49  john
 * Added sub-pixel positioning logic.
 * 
 * Revision 7.2  96/10/29  17:46:12  john
 * Added ANTIALIASED_ALT1_OUTPUT capability.
 * 
 * Revision 7.1  96/10/10  14:04:26  mark
 * Release
 * 
 * Revision 6.1  96/08/27  11:56:59  mark
 * Release
 * 
 * Revision 5.10  96/08/14  12:34:47  mark
 * fix bizarre bug that cropped up in MSVC that could
 * only be fixed by re-typing the lines defining CSP_DEBUG and
 * including stdio.h.  Presumably, this was caused by a line
 * ending anomoly, or some unprintable character.
 * 
 * Revision 5.9  96/07/02  14:13:01  mark
 * change NULL to BTSNULL
 * 
 * Revision 5.8  96/06/20  17:18:54  john
 * Added new functions CspListCharsAlt1() and CspSetOutputSpecsAlt1()
 *     to enhance reentrant mode flexibility.
 * 
 * Revision 5.7  96/04/29  17:21:06  john
 * Updates to eliminate warning messages.
 * 
 * Revision 5.6  96/04/04  12:49:41  john
 * Moved save operation for outline options from csp_tcb.c
 *     (where the pointer may no longer be valid) to CspSetOutputSpecs().
 * 
 * Revision 5.5  96/03/29  18:43:06  shawn
 * Changed declaration/parameter of function GetspGlobalPtr() to 'void' for 4in1.
 * 
 * Revision 5.4  96/03/29  15:20:34  shawn
 * Changed GetspGlobalPtr parameter from 'cspGlobals_t' to 'void' for 4in1.
 * 
 * Revision 5.3  96/03/26  17:53:21  john
 * Added new API function CspGetUniquePhysicalFontCount().
 * 
 * Revision 5.2  96/03/21  11:36:55  mark
 * change boolean type to btsbool.
 * 
 * Revision 5.1  96/03/14  14:19:59  mark
 * Release
 * 
 * Revision 4.2  96/03/14  14:09:34  mark
 * rename PCONTEXT to PCSPCONTEXT 
 * 
 * Revision 4.1  96/03/05  13:44:54  mark
 * Release
 * 
 * Revision 3.1  95/12/29  10:28:28  mark
 * Release
 * 
 * Revision 2.1  95/12/21  09:45:25  mark
 * Release
 * 
 * Revision 1.5  95/10/13  14:15:57  john
 * SetBitmap() callback is now skipped if there is no bitmap image.
 * The current position, however is still updated.
 * 
 * Revision 1.4  95/09/18  18:10:37  john
 * Added CspSetMissingChar() and CspUnsetMissingChar() as new API functions.
 * Modified CspDoString() and CspDoStringWidth() to substitute the missing
 * character code for character codes that are not found.
 * 
 * Revision 1.3  95/08/15  16:39:26  john
 * Font bounding box adjustment done by CspGetFontSpecs for
 *     stroked and bold fontStyle is now inhibited if
 *     bold/stroke thickness is negative.
 * 
 * Revision 1.2  95/08/13  14:35:01  john
 * CspSetPfr now resets current font to 0xffff.
 * CspSetFont now requires a font to be selected as well
 *     as no change in fontCode for a no font change exit.
 * 
 * Revision 1.1  95/08/10  16:44:18  john
 * Initial revision
 * 
 *                                                                           *
 ****************************************************************************/

#ifndef CSP_DEBUG
#define CSP_DEBUG 0
#endif

#if CSP_DEBUG
#include <stdio.h>
#endif

#include "csp_int.h"                    /* Public and internal header */

#if PROC_TRUEDOC || (! INCL_TPS)

#if INCL_CACHE
#include "cachemgr.h"
#endif

#if CSP_DEBUG
#include "csp_dbg.h"
#endif

#if REENTRANT
#else
cspGlobals_t *pCspGlobals;
#if INCL_CACHE
void *pCmGlobals;
#endif
#endif

#if !INCL_TPS || REENTRANT
#undef sp_globals
#define sp_globals ((cspGlobals_t *)pCspGlobals)->spGlobals
#endif

/* Character shape player state for calling sequence checking */
#define CSPOPENMASK        0x3fff
#define CSPOPEN            0x3a5b
#define CSPOUTPUTSPECSSET  0x4000
#define CSPFONTSELECTED    0x8000

/* CspDoChar skips updating current position when escapement 
 * vector does not intersect output bounding box
 * Allowed values are:
 *  0: Always update current position
 *  1: Skip update if appropriate
 */
#ifndef DO_CHAR_OPTION_1
#define DO_CHAR_OPTION_1   1
#endif

/* Default outline options */
static options_t defaultOutlineOptions =
    {
    0,
    BTSNULL,
    BTSNULL,
    BTSNULL,
    0
    };

/* Sub-pixel position rounding tables */
#if INCL_ANTIALIASED_OUTPUT
static fix31 subdivRound[] = 
	{
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100, 
	0x00000080, 0x00000040, 0x00000020, 0x00000010, 
	0x00000008, 0x00000004, 0x00000002, 0x00000001
	};
	
static fix31 subdivFix[] =
	{
	0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000,
	0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
	0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0,
	0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe
	};
#endif


/* Local function prototypes */
LOCAL_PROTO
int ListChars(
    cspGlobals_t *pCspGlobals,
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM),
    long userArg);

LOCAL_PROTO
int SetOutputSpecs(
    cspGlobals_t *pCspGlobals,
    outputSpecs_t *pOutputSpecs,
    long userArg);

FUNCTION
int CspInitBitmapCache(
	PCACONTEXT
    void *pBuffer,
    long nBytes)
/*
 *  Initializes the bitmap cache
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 */
{
#if INCL_CACHE
#if REENTRANT 
void *pCmGlobals;
#endif

CmInitializeCache(&pCmGlobals, nBytes, (char *)pBuffer); 

#if REENTRANT
*pCacheContext = pCmGlobals;
#else
if (pCspGlobals != BTSNULL)
    ((cspGlobals_t *)pCspGlobals)->pCmGlobals = pCmGlobals;
#endif
#endif

return CSP_NO_ERR;
}


FUNCTION
int CspSetCacheParams(
     PCSPCONTEXT
     short int enabled, 
     short int capacity,
     short int pixelColor,
     short int alignment,
     short int inverted)
/*
 *  Initializes the bitmap cache
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 */
{
#if INCL_CACHE
if (((cspGlobals_t *)pCspGlobals)->pCmGlobals == BTSNULL)
    return CSP_NO_ERR;

if (capacity == -1)
    capacity = CACHE_CAPACITY;
if (pixelColor == -1)
    pixelColor = BLACK_PIXEL;
if (alignment == -1)
    alignment = BMAP_ALIGNMENT;
if (inverted == -1)
    inverted = INVERT_BITMAP;

if ((ufix8)pixelColor != 
    ((cmGlobals_t *)(((cspGlobals_t *)pCspGlobals)->pCmGlobals))->blackPixel ||
    (ufix16)alignment != 
    ((cmGlobals_t *)(((cspGlobals_t *)pCspGlobals)->pCmGlobals))->bitmapAlignment ||
    (btsbool)inverted != 
    ((cmGlobals_t *)(((cspGlobals_t *)pCspGlobals)->pCmGlobals))->invertBitmap)
    {
    CmReinitCache((cspGlobals_t *)pCspGlobals);
    }

CmInitParams(
    (cspGlobals_t *)pCspGlobals, 
    enabled, 
    capacity,
    pixelColor,
    alignment,
    inverted);
#endif

return CSP_NO_ERR;
}


FUNCTION
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
    USERPARAM)
/*
 *  Standard function for opening the character shape player for one PFR
 *  accessed via a specified callback function.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Insufficient memory allocated
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_PFR_FORMAT_ERR       4  Portable Font Resource format error
 */
{
cspPfrAccess_t pfrAccessTable[1];

pfrAccessTable[0].mode = PFR_ACCESS_INDIRECT;
pfrAccessTable[0].access.ReadResourceData = ReadResourceData;
return CspOpenAlt1(
#if REENTRANT
    pContext,
    pCmGlobals,
#endif
    deviceResolution,
    1,
    pfrAccessTable,
    AllocCspBuffer
#if REENTRANT
    , userParam
#endif
    );
}

FUNCTION 
int CspOpenAlt1(
	PREENT
    short deviceResolution,
    short nPfrs,
    cspPfrAccess_t pfrAccessTable[],
    void *(*AllocCspBuffer)(
        long minBuffSize,
        long normalBuffSize,
        long maxBuffSize,
        long *pNBytes
        USERPARAM)
    USERPARAM)
/*
 *  Alternative function for initializing the character shape player.
 *  Either this function or CspOpen() should be called to open the player.
 *  This one handles any combination of multiple and/or pre-loaded PFRs.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_BUFFER_OVERFLOW_ERR  2  Insufficient memory allocated
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_PFR_FORMAT_ERR       4  Portable Font Resource format error
 */
{
#if REENTRANT
cspGlobals_t *pCspGlobals;
#endif
pfrHeaderData_t pfrHeaderData;
fix31   minBuffSize;
fix31   normalBuffSize;
fix31   maxBuffSize;
long    nBytes;
void   *pBuffer;
int     errCode;

/* Extract data from PFR headers */
errCode = CspGetPfrHeaderData(
    deviceResolution,
    nPfrs,
    pfrAccessTable,
    &pfrHeaderData
#if REENTRANT
    , userParam
#endif
    );
if (errCode != 0)
    {
    return errCode;
    }

/* Figure out how much memory we need for PFR execution */
CspDoMemoryBudget(
    nPfrs,
    pfrAccessTable,
    &pfrHeaderData,
    &minBuffSize,
    &normalBuffSize,
    &maxBuffSize);

/* Ask the client to allocate a memory buffer */
pBuffer = AllocCspBuffer(
    (long)minBuffSize,
    (long)normalBuffSize,
    (long)maxBuffSize,
    &nBytes
#if REENTRANT
    ,userParam
#endif
    );
if (nBytes < minBuffSize)       /* Less than min requested? */
    {
    return CSP_BUFFER_OVERFLOW_ERR;
    }

/* Initialize the CSP memory manager; allocate global structure */
if (!CspInitAlloc(
    &pCspGlobals,
    pBuffer, 
    nBytes))
    {
    return CSP_BUFFER_OVERFLOW_ERR;
    }

/* Save user parameter as a global */
#if REENTRANT
    pCspGlobals->userArg = userParam;
#endif

/* Allocate memory and set up internal data structures */
errCode = CspSetupMemory(
    pCspGlobals,
    nPfrs,
    pfrAccessTable,
    &pfrHeaderData);
if (errCode != 0)
    {
    return errCode;
    }

/* Initialize bitmap cache */
#if INCL_CACHE
	{
	fix15	i;
	
	pCspGlobals->pCmGlobals = pCmGlobals;
	for (i = 0; i < 4; i++)
		pCspGlobals->cacheOutputMatrix[i] = 0;
	pCspGlobals->cacheHintStatus = 0;
	}
#endif

/* Mark CSP open */
pCspGlobals->cspState = CSPOPEN;

/* Initialize missing character mechanism */
pCspGlobals->missingCharCode = 0;
pCspGlobals->missingCharActive = FALSE;

/* Return pointer to CSP context (if re-entrant) */
#if REENTRANT
*pContext = (void *)pCspGlobals; 
#endif

return CSP_NO_ERR;
}

FUNCTION
int CspSetMissingChar(
    PCSPCONTEXT
    unsigned short missingCharCode)
/*
 *  Enables missing character substitution for string processing
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 */
{
/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

((cspGlobals_t *)pCspGlobals)->missingCharCode = missingCharCode;
((cspGlobals_t *)pCspGlobals)->missingCharActive = TRUE;

return CSP_NO_ERR;
}

FUNCTION
int CspUnsetMissingChar(PCSPCONTEXT1)
/*
 *  Disables missing character substitution.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 */
{
/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

((cspGlobals_t *)pCspGlobals)->missingCharActive = FALSE;

return CSP_NO_ERR;
}

FUNCTION
int CspSetPfr(
    PCSPCONTEXT
    short pfrCode)
/*
 *  Selects the specified portable font resource.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_PFR_CODE_ERR        11  PFR code out of range
 */
{
/* Check CSP is open */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Check PFR code is valid */
if (pfrCode >= ((cspGlobals_t *)pCspGlobals)->nPfrs)
    {
    return CSP_PFR_CODE_ERR;
    }
    
/* Save user-selected PFR code */
((cspGlobals_t *)pCspGlobals)->userPfrCode = pfrCode;

/* Mark no font selected */
((cspGlobals_t *)pCspGlobals)->cspState &= ~CSPFONTSELECTED;
((cspGlobals_t *)pCspGlobals)->fontCode = 0xffff;

return CSP_NO_ERR;
}

FUNCTION
unsigned short CspGetFontCount(
    PCSPCONTEXT1)
/*
 *  Returns a count of the number of fonts in the current portable
 *  font resource (zero if CSP is not open).
 *  If multiple PFRs are active, the value returned is normally
 *  the total number of fonts over the set of PFRs. If a specific
 *  PFR has been selected by calling CspSetPfr(), the number returned
 *  applies only to the currently selected PFR.
 * 
 */
{
fix15   pfrCode;

if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)                    /* CSP not open? */
    {
    return 0;
    }

pfrCode = ((cspGlobals_t *)pCspGlobals)->userPfrCode;
if (pfrCode >= 0)               /* Local font numbering? */
    {
    return ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].nLogicalFonts;
    }
else                            /* Global font numbering? */
    {
    return ((cspGlobals_t *)pCspGlobals)->nLogicalFonts;
    }
}

#if CSP_MAX_DYNAMIC_FONTS > 0
FUNCTION
unsigned short CspGetUniquePhysicalFontCount(
    PCSPCONTEXT1)
/*
 *  Returns a count of the number of distinct physical fonts in 
 *  the current portable font resource (zero if CSP is not open).
 *  If multiple PFRs are active, the value returned is after
 *  dynamically merging identical physical fonts over the set of PFRs. 
 *  If a specific PFR has been selected by calling CspSetPfr(), the 
 *  number returned  applies only to the currently selected PFR.
 */
{
fix15   pfrCode;
ufix16  firstPhysFontCode;
ufix16  nPhysFonts;
ufix16  nextPhysFont;
ufix16  ii;
unsigned short fontCount;

if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)                    /* CSP not open? */
    {
    return 0;
    }

pfrCode = ((cspGlobals_t *)pCspGlobals)->userPfrCode;
if (pfrCode >= 0)               /* Local font numbering? */
    {
    firstPhysFontCode = 
        ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].baseFontRefNumber;
    nPhysFonts = 
        ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].nPhysFonts;
    }
else                            /* Global font numbering? */
    {
    firstPhysFontCode = 0;
    nPhysFonts = ((cspGlobals_t *)pCspGlobals)->nPhysFonts;
    }

fontCount = 0;
for (ii = firstPhysFontCode; 
    ii < (firstPhysFontCode + nPhysFonts); 
    ii++)
    {
    nextPhysFont = 
        ((cspGlobals_t *)pCspGlobals)->pPhysFontTable[ii].nextPhysFont;
    if (nextPhysFont == 0xffff)
        {
        fontCount++;
        }
    }
return fontCount;
}
#endif






#if CSP_MAX_DYNAMIC_FONTS > 0
FUNCTION
int CspCreateDynamicFont(
    PCSPCONTEXT
    char *pFontID,
    short attrOutlRes,
    fontAttributes_t *pFontAttributes,
    unsigned short *pFontCode)
/*
 *  Creates a new dynamic logical font based on the physical font with the
 *  specified font ID and the specified attributes. Attribute fields in
 *  outline resolution units are based on the specified attribute outline
 *  resolution.
 *  Sets *pFontCode to the global logical font code of the new font.
 *  The newly created font is _not_ automatically selected.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_FONT_STYLE_ERR       9  Illegal value for fontStyle
 *      CSP_LINE_JOIN_TYPE_ERR  10  Illegal value for lineJoinType
 *      CSP_FONT_ID_ERR         12  Font ID not found
 *      CSP_DYN_FONT_OFLO_ERR   13  Dynamic font table overflow
 */
{
/* Check CSP is open */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

return CspCreateNewFont(
    (cspGlobals_t *)pCspGlobals,
    pFontID,
    attrOutlRes,
    pFontAttributes,
    pFontCode);
}
#endif

#if CSP_MAX_DYNAMIC_FONTS > 0
FUNCTION
int CspDisposeDynamicFont(
    PCSPCONTEXT
    unsigned short fontCode)
/*
 *  Reverses the result of calling CspCreateDynamicFont() for the 
 *  specified global font code.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_FONT_CODE_ERR        5  Font code out of range
 */
{
/* Check CSP is open */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Remove the dynamic font and exit */
return CspDisposeNewFont(
    (cspGlobals_t *)pCspGlobals,
    fontCode);
}
#endif

#if CSP_MAX_DYNAMIC_FONTS > 0
FUNCTION
unsigned short CspGetDynamicFontCount(
    PCSPCONTEXT1)
/*
 *  Returns a count of the number of fonts created by CspCreateDynamicFont()
 *  that have not been destroyed by calling CspDestroyDynamicFont().
 */
{
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)                    /* CSP not open? */
    {
    return 0;
    }

return (unsigned short)(((cspGlobals_t *)pCspGlobals)->nNewFonts);
}
#endif

FUNCTION
int CspSetFont(
    PCSPCONTEXT
    unsigned short fontCode)
/*
 *  Selects the specified font.
 *  If global font numbering is in effect and multiple PFRs are open,
 *  the appropriate PFR is determined from the font number.
 *  If local font numbering is in effect, the specified font number
 *  is converted into a global font code.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_FONT_CODE_ERR        5  Font code out of range
 */
{
fix15   pfrCode;
int     errCode;

/* Check CSP is open */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
/* Check if font code refers to dynamic font */
if (fontCode >= ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
    {
    /* Exit if the requested font is already selected */
    if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPFONTSELECTED) && 
        (fontCode == ((cspGlobals_t *)pCspGlobals)->fontCode))
        {
        return CSP_NO_ERR;
        }

    /* Load and select dynamic font */
    errCode = CspSetNewFont((cspGlobals_t *)pCspGlobals, fontCode);
    if (errCode != 0)
        return errCode;

    /* Mark font selected */
    ((cspGlobals_t *)pCspGlobals)->cspState |= CSPFONTSELECTED;

    /* Set up data paths and transformation constants */
    if (((cspGlobals_t *)pCspGlobals)->cspState & CSPOUTPUTSPECSSET)
        {
        errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
        if (errCode != 0)
            return errCode;
        }
    return CSP_NO_ERR;
    }

((cspGlobals_t *)pCspGlobals)->pNewFont = BTSNULL;
#endif

/* Check font code; convert it to global numbering */
pfrCode = ((cspGlobals_t *)pCspGlobals)->userPfrCode;
if (pfrCode >= 0)           /* Local font numbering? */
    {
    if (fontCode >= 
        ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].nLogicalFonts)
        {
        return CSP_FONT_CODE_ERR;
        }
    fontCode += 
        ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].baseFontCode;
    }
else                        /* Global font numbering? */
    {
    if (fontCode >= ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
        {
        return CSP_FONT_CODE_ERR;
        }
    for (
        pfrCode = 0;
        (fontCode >= 
         (((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].baseFontCode +
          ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].nLogicalFonts));
        pfrCode++)
        {
        }
    }

/* Exit if the requested font is already selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPFONTSELECTED) && 
    (fontCode == ((cspGlobals_t *)pCspGlobals)->fontCode))
    {
    return CSP_NO_ERR;
    }

/* Load and select specified logical font */
errCode = CspLoadLogicalFont(
    ((cspGlobals_t *)pCspGlobals),
    pfrCode,
    fontCode);
if (errCode != 0)
    return errCode;

/* Mark font selected */
((cspGlobals_t *)pCspGlobals)->cspState |= CSPFONTSELECTED;

/* Set up data paths and transformation constants */
if (((cspGlobals_t *)pCspGlobals)->cspState & CSPOUTPUTSPECSSET)
    {
    errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
    if (errCode != 0)
        return errCode;
    }

return CSP_NO_ERR;
}


FUNCTION
int CspGetFontSpecs(
    PCSPCONTEXT
    unsigned short *pFontRefNumber,
    cspFontInfo_t *pFontInfo,
    fontAttributes_t *pFontAttributes)
/*
 *  Responds with information about the currently selected font.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix15   pfrCode;

/* Verify CSP is open and a font is selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
if (((cspGlobals_t *)pCspGlobals)->fontCode >= 
    ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
    {
    int     errCode;

    errCode = CspGetNewFontSpecs(
        (cspGlobals_t *)pCspGlobals,
        pFontRefNumber,
        pFontInfo,
        pFontAttributes);
    if (errCode != 0)
        return errCode;
    goto L1;
    } 
#endif

*pFontRefNumber = ((cspGlobals_t *)pCspGlobals)->fontRefNumber;
pfrCode = ((cspGlobals_t *)pCspGlobals)->userPfrCode;
if (pfrCode < 0)            /* Global font numbering? */
    {
    pfrCode = ((cspGlobals_t *)pCspGlobals)->physFontPfrCode;
    *pFontRefNumber += 
        ((cspGlobals_t *)pCspGlobals)->pfrTable[pfrCode].baseFontCode;
    }

*pFontInfo = 
    ((cspGlobals_t *)pCspGlobals)->fontInfo;

*pFontAttributes = 
    ((cspGlobals_t *)pCspGlobals)->fontAttributes;

L1:

#if INCL_STROKE || INCL_BOLD
    /* Adjust font information fields for derivative fonts */
    {
    fix15   strokeThickness;
    fix31   miterLimit;
    short   bboxAdj;
    fix15   ii;

    switch (((cspGlobals_t *)pCspGlobals)->fontAttributes.fontStyle)
        {

#if INCL_STROKE
    case STROKED_STYLE:
        strokeThickness = 
            pFontAttributes->styleSpecs.styleSpecsStroked.strokeThickness;
        pFontInfo->stdVW = strokeThickness;
        pFontInfo->stdHW = strokeThickness;
        miterLimit = pFontAttributes->styleSpecs.styleSpecsStroked.miterLimit;
        goto L2;
#endif

#if INCL_BOLD
    case BOLD_STYLE:
        strokeThickness = 
            pFontAttributes->styleSpecs.styleSpecsBold.boldThickness;
        pFontInfo->stdVW += strokeThickness;
        pFontInfo->stdHW += strokeThickness;
        miterLimit = 0x40000;
        goto L2;
#endif

    L2:
        bboxAdj = (short)(
            (strokeThickness * (miterLimit + 0x10000) + 0x1ffff) >> 17);
        if (bboxAdj > 0)
            {
            pFontInfo->fontBBox.xmin -= bboxAdj;
            pFontInfo->fontBBox.ymin -= bboxAdj;
            pFontInfo->fontBBox.xmax += bboxAdj;
            pFontInfo->fontBBox.ymax += bboxAdj;
            }

        for (ii = 0; ii < pFontInfo->nBlueValues; ii++)
            {
            ((cspGlobals_t *)pCspGlobals)->pAdjBlueValues[ii] = 
                pFontInfo->pBlueValues[ii];
            if (ii >= 2)
                ((cspGlobals_t *)pCspGlobals)->pAdjBlueValues[ii] += strokeThickness;
            }
        pFontInfo->pBlueValues = 
            ((cspGlobals_t *)pCspGlobals)->pAdjBlueValues;
        }
    }
#endif

return CSP_NO_ERR;
}

FUNCTION
int CspListChars(
    PCSPCONTEXT
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM))
/*
 *  Calls the specified function once for each character in the
 *  currently selected font. The calls are in increasing order of 
 *  character code.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_LIST_CHAR_FN_ERR     6  ListCharFn() returned error
 */
{
int		errCode;

#if REENTRANT
errCode = ListChars(
	(cspGlobals_t *)pCspGlobals, 
	ListCharFn, 
	((cspGlobals_t *)pCspGlobals)->userArg);
#else
errCode = ListChars(
	(cspGlobals_t *)pCspGlobals, 
	ListCharFn, 
	0L);
#endif
return errCode;
}

#if REENTRANT
FUNCTION
int CspListCharsAlt1(
    PCSPCONTEXT
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM)
    USERPARAM)
/*
 *  Alternative version of CspListChars() that specifies the value of the
 *  user argument to be delivered with each call of the specified callback 
 *  function. The override has no effect on subsequent calls to CspListChars().
 *  Available in reentrant mode only.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_LIST_CHAR_FN_ERR     6  ListCharFn() returned error
 */
{
int		errCode;

errCode = ListChars(
	(cspGlobals_t *)pCspGlobals, 
	ListCharFn, 
	userParam);
return errCode;
}
#endif

FUNCTION
static int ListChars(
    cspGlobals_t *pCspGlobals,
    int (*ListCharFn)(
        PCSPCONTEXT
        unsigned short charCode
        USERPARAM),
    long userArg)
/*
 *  Calls the specified function once for each character in the
 *  currently selected font. The calls are in increasing order of 
 *  character code. In reentrant mode, the value of the specified 
 *  user argument delivered with each callback.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_LIST_CHAR_FN_ERR     6  ListCharFn() returned error
 */
{
charRec_t  charRec;
ufix16  charCode;
int     errCode;

/* Verify CSP is open and a font is selected */
if ((pCspGlobals->cspState & (CSPOPENMASK + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Call specified function for each char in current font */
#if CSP_MAX_DYNAMIC_FONTS > 0
if (pCspGlobals->fontCode >= 
    pCspGlobals->nLogicalFonts)
    {
    errCode = CspListNewFontChars(
        (cspGlobals_t *)pCspGlobals,
        ListCharFn,
        userArg);
    if (errCode != 0)
        return errCode;
    } 
else                            /* PFR font? */
#endif
    {
	charRec.charCode = 0;
	while (TRUE)
    	{
    	charCode = charRec.charCode;
		errCode = ((cspGlobals_t *)pCspGlobals)->FindChar(
			(cspGlobals_t *)pCspGlobals, 
			&charRec);
    	if ((errCode == 0) ||
    		((errCode == CSP_CHAR_CODE_ERR) && (charRec.charCode > charCode)))
    		{
#if REENTRANT
        	errCode = ListCharFn(
            	pCspGlobals,
            	charRec.charCode,
            	userArg);
#else
        	errCode = ListCharFn(charRec.charCode);
#endif
        	if (errCode != 0)
            	return CSP_LIST_CHAR_FN_ERR;
	    	charRec.charCode++;
    		}
    	else
	    	{
	    	break;
	    	}
        }
    }

return CSP_NO_ERR;
}

FUNCTION
int CspGetCharSpecs(
    PCSPCONTEXT
    unsigned short charCode,
    cspCharInfo_t *pCharInfo)
/*
 *  Responds with information about the specified character in
 *  the currently selected font.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
fix15   outlineResolution;
outputSpecs_t outputSpecs;
fontAttributes_t fontAttributes;
int     errCode, errCode1;

/* Verify CSP is open and a font is selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
if (((cspGlobals_t *)pCspGlobals)->fontCode >= 
    ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
    {
    errCode = CspLoadTopPhysFont((cspGlobals_t *)pCspGlobals);
    if (errCode != 0)
        return errCode;
    }
#endif

outlineResolution = 
    ((cspGlobals_t *)pCspGlobals)->fontInfo.outlineResolution;

/* Save current output specs and font attributes */
outputSpecs = ((cspGlobals_t *)pCspGlobals)->outputSpecs;
fontAttributes = ((cspGlobals_t *)pCspGlobals)->fontAttributes;

/* Change font attributes for unit font matrix */
((cspGlobals_t *)pCspGlobals)->fontAttributes.fontMatrix[0] = 1L << 16;
((cspGlobals_t *)pCspGlobals)->fontAttributes.fontMatrix[1] = 0L << 16;
((cspGlobals_t *)pCspGlobals)->fontAttributes.fontMatrix[2] = 0L << 16;
((cspGlobals_t *)pCspGlobals)->fontAttributes.fontMatrix[3] = 1L << 16;

/* Change output type for bounding box collection */
((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType = 0;

/* Change output matrix to scale to outline resolution units */
((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.charBBox.outputMatrix[0] = 
((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.charBBox.outputMatrix[3] = 
    (fix31)outlineResolution << 16;
((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.charBBox.outputMatrix[1] = 
((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.charBBox.outputMatrix[2] = 
    0L;

/* Set up data paths and transformation constants */
errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
if (errCode != 0)
    return errCode;

/* Execute the specified character (image generation enabled) */
errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
    ((cspGlobals_t *)pCspGlobals),
    charCode, 
    FALSE);
if (errCode == 0)
    {
    /* Extract escapement vector */
    pCharInfo->hWidth = (short)(
        (sp_globals.set_width.x + 32768L) >> 16);
    pCharInfo->vWidth = (short)(
        (sp_globals.set_width.y + 32768L) >> 16);
    
    /* Extract character bounding box */
    pCharInfo->charBBox.xmin = 
        (sp_globals.xmin + 
         sp_globals.pixrnd) >> 
        sp_globals.pixshift;
    pCharInfo->charBBox.ymin = 
        (sp_globals.ymin + 
         sp_globals.pixrnd) >> 
        sp_globals.pixshift;
    pCharInfo->charBBox.xmax = 
        (sp_globals.xmax + 
         sp_globals.pixrnd) >> 
        sp_globals.pixshift;
    pCharInfo->charBBox.ymax = 
        (sp_globals.ymax + 
         sp_globals.pixrnd) >> 
        sp_globals.pixshift;
    }

/* Restore output specs and font attributes */
((cspGlobals_t *)pCspGlobals)->outputSpecs = outputSpecs;
((cspGlobals_t *)pCspGlobals)->fontAttributes = fontAttributes;

/* Restore data paths and transformation constants */
if (((cspGlobals_t *)pCspGlobals)->cspState & CSPOUTPUTSPECSSET)
    {
    errCode1 = CspDoSetup((cspGlobals_t *)pCspGlobals);
    if (errCode == 0)
    	{
        errCode = errCode1;
        }
    }

return errCode;
}

FUNCTION
int CspSetOutputSpecs(
    PCSPCONTEXT
    outputSpecs_t *pOutputSpecs)
/*
 *  Selects the specified output mode and saves a copy of the associated
 *  information in global variables.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_OUTPUT_TYPE_ERR      8  Undefined output type
 *      CSP_PIXEL_SIZE_ERR      14  Pixel size out of range
 *	    CSP_FILTER_WIDTH_ERR    15  Filter width out of range
 *	    CSP_FILTER_HEIGHT_ERR   16  Filter height out of range
 *	    CSP_X_POSN_SUBDIV_ERR   17  X position accuracy out of range
 *	    CSP_Y_POSN_SUBDIV_ERR   18  Y position accuracy out of range
 */
{
int     errCode;
#if REENTRANT
errCode = SetOutputSpecs(
	(cspGlobals_t *)pCspGlobals,
	pOutputSpecs,
	((cspGlobals_t *)pCspGlobals)->userArg);
#else
errCode = SetOutputSpecs(
	(cspGlobals_t *)pCspGlobals,
	pOutputSpecs,
	0L);
#endif
return errCode;
}

#if REENTRANT
FUNCTION
int CspSetOutputSpecsAlt1(
    PCSPCONTEXT
    outputSpecs_t *pOutputSpecs
    USERPARAM)
/*
 *	Alternative version of CspSetOutputSpecs() that allows specification
 *  of the value of the user argument to be returned with each callback
 *  function specified in the output specs. This override does not affect
 *  behavior of subsequent calls to CspSetOutputSpecs.
 *  Available in reentrant mode only.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_OUTPUT_TYPE_ERR      8  Undefined output type
 *      CSP_PIXEL_SIZE_ERR      14  Pixel size out of range
 *	    CSP_FILTER_WIDTH_ERR    15  Filter width out of range
 *	    CSP_FILTER_HEIGHT_ERR   16  Filter height out of range
 */
{
int     errCode;

errCode = SetOutputSpecs(
	(cspGlobals_t *)pCspGlobals,
	pOutputSpecs,
	userParam);
return errCode;
}
#endif

FUNCTION
static int SetOutputSpecs(
    cspGlobals_t *pCspGlobals,
    outputSpecs_t *pOutputSpecs,
    long userArg)
/*
 *  Selects the specified output mode and saves a copy of the associated
 *  information in global variables. In reentrant mode, the specified value
 *  of the user argument is delivered with each subsequent callback function
 *  associated with the selected output mode.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_OUTPUT_TYPE_ERR      8  Undefined output type
 *      CSP_PIXEL_SIZE_ERR      14  Pixel size out of range
 *	    CSP_FILTER_WIDTH_ERR    15  Filter width out of range
 *	    CSP_FILTER_HEIGHT_ERR   16  Filter height out of range
 *	    CSP_X_POSN_SUBDIV_ERR   17  X position accuracy out of range
 *	    CSP_Y_POSN_SUBDIV_ERR   18  Y position accuracy out of range
 */
{
int     errCode;

/* Verify CSP is open */
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)
    {
    return CSP_CALL_SEQUENCE_ERR ;
    }

/* Save requested output specifications */
((cspGlobals_t *)pCspGlobals)->outputSpecs = *pOutputSpecs;

/* Save user arg for output callbacks */
#if REENTRANT
((cspGlobals_t *)pCspGlobals)->userArgOutput = userArg;
#endif

switch (pOutputSpecs->outputType)
    {
#if INCL_CSPQUICK
case BITMAP_OUTPUT:
    break;
#endif

#if INCL_CSPOUTLINE
case OUTLINE_OUTPUT:
    ((cspGlobals_t *)pCspGlobals)->outlineOptions = 
        (pOutputSpecs->specs.outline.pOptions != BTSNULL)?
        *(pOutputSpecs->specs.outline.pOptions):
        defaultOutlineOptions;
    break;
#endif

#if INCL_DIR
case DIRECT_OUTPUT:
    break;
#endif

#if INCL_ANTIALIASED_OUTPUT
case ANTIALIASED_OUTPUT:
    if ((pOutputSpecs->specs.pixmap.pixelSize < 1) || 
    	(pOutputSpecs->specs.pixmap.pixelSize > 8))
        {
        return CSP_PIXEL_SIZE_ERR;
        }
    break;
	
case ANTIALIASED_ALT1_OUTPUT:
    if ((pOutputSpecs->specs.pixmap1.pixelSize < 1) || 
    	(pOutputSpecs->specs.pixmap1.pixelSize > 8))
        {
        return CSP_PIXEL_SIZE_ERR;
        }
        
    if ((pOutputSpecs->specs.pixmap1.filterWidth < 1) || 
    	(pOutputSpecs->specs.pixmap1.filterWidth > 3))
        {
        return CSP_FILTER_WIDTH_ERR;
        }
        
    if ((pOutputSpecs->specs.pixmap1.filterHeight < 1) || 
    	(pOutputSpecs->specs.pixmap1.filterHeight > 3))
        {
        return CSP_FILTER_HEIGHT_ERR;
        }
        
    if (pOutputSpecs->specs.pixmap1.xPosSubdivs > 4)
        {
        return CSP_X_POSN_SUBDIV_ERR;
        }
        
    if (pOutputSpecs->specs.pixmap1.yPosSubdivs > 4)
        {
        return CSP_Y_POSN_SUBDIV_ERR;
        }
        
    break;
#endif /* INCL_ANTIALIASED_OUTPUT */

default:
    return CSP_OUTPUT_TYPE_ERR;
    }

/* Flag output specs set */
((cspGlobals_t *)pCspGlobals)->cspState |= CSPOUTPUTSPECSSET;

/* Set up data paths and transformation constants */
if (((cspGlobals_t *)pCspGlobals)->cspState & CSPFONTSELECTED)
    {
    errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
    if (errCode != 0)
        return errCode;
    }

return CSP_NO_ERR;
}

#if INCL_CACHE
FUNCTION
int CspIsCacheEnabled(PCSPCONTEXT1)
/*
 *  Returns state of cache for current font
 *  Returns:
 *      0  Cache not enabled
 *      1  Cache enabled
 */
{
/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPOUTPUTSPECSSET + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPOUTPUTSPECSSET + CSPFONTSELECTED))
    {
    return 0;
    }

return (((cspGlobals_t *)pCspGlobals)->cacheEnabled);
}
#endif

FUNCTION
int CspDoChar(
    PCSPCONTEXT
    unsigned short charCode,
    long *pXpos,
    long *pYpos)
/*
 *  Executes the specified character from the currently selected font 
 *  for the currently selected output specifications. 
 *  In bitmap or antialiased output mode, a bitmap (or pixel map) of the 
 *  specified character is generated and written at the specified position 
 *  in device coordinates. 
 *  After the bitmap is written, the position is incremented by the
 *  character escapement.
 *  In outline output mode, the transformed character hints and outline
 *  are delivered by calls to the outline output callback functions.
 *  In direct output mode, the transformed outline is delivered by calls
 *  to the direct output callback functions.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 *      CSP_CACHE_ERR           -1  Internal cache error
 */
{
long	xPos, yPos;
ufix16	xyPosFract;
btsbool noImage;
point_t Pmin, Pmax;
int errCode;

#if INCL_ANTIALIASED_OUTPUT
btsbool xyPosAdjust;
ufix8 	xPosSubdivs, yPosSubdivs;
#endif

#if INCL_CACHE
char_desc_t charDesc;
bmapSpecs_t bmapSpecs;
chardata_hdr *pCharData;
#endif

#if INCL_TPS && REENTRANT
intercepts_t intercepts;
sp_globals.intercepts = &intercepts;
#endif

/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPOUTPUTSPECSSET + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPOUTPUTSPECSSET + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

switch (((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType)
    {
case BITMAP_OUTPUT:
case ANTIALIASED_OUTPUT:
case ANTIALIASED_ALT1_OUTPUT:
    /* Test if character is completely outside output bounding box */
    yPos = *pYpos;
	if ((yPos < ((cspGlobals_t *)pCspGlobals)->outerClipYmin) ||
        (yPos > ((cspGlobals_t *)pCspGlobals)->outerClipYmax))
        {
#if DO_CHAR_OPTION_1
        /* Exit if we don't need to update current position */
        if (!((cspGlobals_t *)pCspGlobals)->verticalEscapement &&
        	(sp_globals.tcb.ctm[1] == 0))
        	{
        	return CSP_NO_ERR;
        	}
#endif
        noImage = TRUE;
        }
    else
    	{
    	xPos = *pXpos;
    	noImage =
    		(xPos < ((cspGlobals_t *)pCspGlobals)->outerClipXmin) ||
    		(xPos > ((cspGlobals_t *)pCspGlobals)->outerClipXmax);
		}

	/* Determine sub-pixel position */
	xyPosFract = 0;
#if INCL_ANTIALIASED_OUTPUT
	xyPosAdjust = FALSE;
	if (!noImage &&
		((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType == ANTIALIASED_ALT1_OUTPUT)
		{
		xPosSubdivs = (ufix8)(
			((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.pixmap1.xPosSubdivs);
		if (xPosSubdivs != 0)
			{
			xPos = 
				(xPos + 
				 subdivRound[xPosSubdivs]) &
				subdivFix[xPosSubdivs];
			xyPosFract = (ufix16)xPos;
			xPos &= 0xffff0000;
			xyPosAdjust = TRUE;
			}
			
		yPosSubdivs = (ufix8)(
			((cspGlobals_t *)pCspGlobals)->outputSpecs.specs.pixmap1.yPosSubdivs);
		if (yPosSubdivs != 0)
			{
			yPos = 
				(yPos + 
				 subdivRound[yPosSubdivs]) &
				subdivFix[yPosSubdivs];
			xyPosFract += (ufix16)yPos >> 8;
			yPos &= 0xffff0000;
			xyPosAdjust = TRUE;
			}
		}	
#endif

#if INCL_CACHE
    if (((cspGlobals_t *)pCspGlobals)->cacheEnabled)
        {
        charDesc.fontCode = (unsigned short)((cspGlobals_t *)pCspGlobals)->fontCode;
        charDesc.charCode = charCode;
        charDesc.instCode = (unsigned short)xyPosFract;
        pCharData = CmFindChar(
          	((cspGlobals_t *)pCspGlobals)->pCmGlobals,
           	&charDesc);

        if (!noImage)           /* Character image required? */
            {
            if (pCharData != BTSNULL) /* Char found in cache? */
                {
                CmUpdateLru(
                	((cspGlobals_t *)pCspGlobals)->pCmGlobals,
                    pCharData);
                }
            else                /* Char not found in cache? */
                {
			   	/* Adjust transformation for sub-pixel position */
#if INCL_ANTIALIASED_OUTPUT
				if (xyPosAdjust)
					{
					sp_globals.tcb.xoffset = (fix31)(
						(ufix16)(xyPosFract & 0xff00) >>
						(16 - sp_globals.multshift));

					sp_globals.tcb.yoffset = (fix31)(
						(ufix16)(xyPosFract << 8) >>
						(16 - sp_globals.multshift));
							
					CspSetTcb((cspGlobals_t *)pCspGlobals);
					}
#endif	
                pCharData = CmMakeChar(
                	(cspGlobals_t *)pCspGlobals,
                	((cspGlobals_t *)pCspGlobals)->pCmGlobals,
                	&charDesc);
    	        if (pCharData == BTSNULL)
                    {
                    return CSP_CHAR_CODE_ERR;
                    }
                }
                
            /* Deliver bitmap image from cache */
            bmapSpecs.xPos = (short)(
            	(xPos + pCharData->cacheSpecs.xPos + 32768L) >> 16);
            bmapSpecs.yPos = (short)(
            	(yPos + pCharData->cacheSpecs.yPos + 32768L) >> 16);
            bmapSpecs.xSize = pCharData->cacheSpecs.xSize;
            bmapSpecs.ySize = pCharData->cacheSpecs.ySize;
            if ((bmapSpecs.xSize > 0) &&
                (bmapSpecs.ySize > 0))
                {
                (*((cspGlobals_t *)pCspGlobals)->bmapCallbackFns.SetBitmap)(
                	bmapSpecs,
                    (char *)pCharData + sizeof(char_desc_t) + sizeof(chardata_hdr)
                    USERARG2);
                }
            goto L1;
            }

        /* Update current position if char found in cache */
        if (pCharData != BTSNULL) /* Char found in cache? */
            {
L1:			*pXpos += pCharData->cacheSpecs.xEscapement;
            *pYpos += pCharData->cacheSpecs.yEscapement;
            return CSP_NO_ERR;
            }
        }
#endif

    /* Save current position for bitmap output */
    ((cspGlobals_t *)pCspGlobals)->xPosPix = xPos;
    ((cspGlobals_t *)pCspGlobals)->yPosPix = yPos;
    
   	/* Adjust transformation for sub-pixel positioning */
#if INCL_ANTIALIASED_OUTPUT
	if (xyPosAdjust)
		{
		sp_globals.tcb.xoffset = (fix31)(
			(ufix16)(xyPosFract & 0xff00) >>
			(16 - sp_globals.multshift));

		sp_globals.tcb.yoffset = (fix31)(
			(ufix16)(xyPosFract << 8) >>
			(16 - sp_globals.multshift));
				
		CspSetTcb((cspGlobals_t *)pCspGlobals);
		}
#endif	

	/* Test if character is partially outside output bounding box */
    if (!noImage && (
    	(xPos < ((cspGlobals_t *)pCspGlobals)->innerClipXmin) ||
        (xPos > ((cspGlobals_t *)pCspGlobals)->innerClipXmax) ||
        (yPos < ((cspGlobals_t *)pCspGlobals)->innerClipYmin) ||
        (yPos > ((cspGlobals_t *)pCspGlobals)->innerClipYmax)))
        {
        /* Save font bounding box */
        Pmin = ((cspGlobals_t *)pCspGlobals)->Pmin;
        Pmax = ((cspGlobals_t *)pCspGlobals)->Pmax;
       
       	/* Clip font bounding box to output bounding box */
       	CspClipFontBBox(((cspGlobals_t *)pCspGlobals), pXpos, pYpos);
       		
        /* Execute character (image generation enabled) */
		errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
            ((cspGlobals_t *)pCspGlobals),
            charCode, 
            FALSE);

        /* Restore font bounding box */
        ((cspGlobals_t *)pCspGlobals)->Pmin = Pmin;
        ((cspGlobals_t *)pCspGlobals)->Pmax = Pmax;
		}
	else	/* Character completely within output bounding box? */
		{		
	    /* Execute character */
         errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
	        ((cspGlobals_t *)pCspGlobals),
	        charCode, 
	        noImage);
	    }
	break;

#if INCL_CSPOUTLINE || INCL_DIR
case OUTLINE_OUTPUT:
case DIRECT_OUTPUT:
    /* Execute character (image generation enabled) */
	errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
        ((cspGlobals_t *)pCspGlobals),
        charCode, 
        FALSE);
    break;
#endif
    }

/* Update current point in device space */
*pXpos += sp_globals.set_width.x;
*pYpos += sp_globals.set_width.y;

return errCode;
}

FUNCTION
int CspDoCharWidth(
    PCSPCONTEXT
    unsigned short charCode,
    long *pXpos,
    long *pYpos)
/*
 *  Executes the specified character from the currently selected font 
 *  for the currently selected output specifications. 
 *  The current position in device coordinates is incremented by the
 *  character escapement.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
#if INCL_CACHE
char_desc_t charDesc;
chardata_hdr *pCharData;
#endif
int errCode;

/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPOUTPUTSPECSSET + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPOUTPUTSPECSSET + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if INCL_CACHE
if (((cspGlobals_t *)pCspGlobals)->cacheEnabled)
    {
    charDesc.charCode = charCode;
    charDesc.fontCode = ((cspGlobals_t *)pCspGlobals)->fontCode;
    pCharData = CmFindChar(
      	((cspGlobals_t *)pCspGlobals)->pCmGlobals,
       	&charDesc);

    /* Update current position if char found in cache */
    if (pCharData != BTSNULL) /* Char found in cache? */
        {
        *pXpos += pCharData->cacheSpecs.xEscapement;
        *pYpos += pCharData->cacheSpecs.yEscapement;
        return CSP_NO_ERR;
        }
    }
#endif

/* Execute character (image generation disabled) */
errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
    ((cspGlobals_t *)pCspGlobals),
    charCode, 
    TRUE);
if (errCode != 0)
    return errCode;

/* Update current point in device space */
*pXpos += sp_globals.set_width.x;
*pYpos += sp_globals.set_width.y;

return CSP_NO_ERR;
}

FUNCTION
int CspDoString(
    PCSPCONTEXT
    unsigned short modeFlags,
    short length,
    void *pString,
    long *pXpos,
    long *pYpos)
/*  Executes the each of the characters in the specified string using
 *  the currently selected font and the currently selected output 
 *  specifications.
 *  *pXpos and *pYpos are updated with the escapement of each character
 *  in the string.
 *  The mode flags have the following meaning:
 *      Bit 0:  16-bit character codes
 *      Bit 1:  Round X-escapement after each character
 *      Bit 2:  Round Y-escapement after each character
 *  In bitmap output mode, a bitmap image of each character is generated 
 *  and written at the current position in device coordinates. 
 *  After the bitmap is written, the current position is incremented by the
 *  character escapement ready for the next character.
 *  In outline output mode, the transformed character hints and outline
 *  are delivered by calls to the outline output callback functions.
 *  In direct output mode, the transformed outline is delivered by calls
 *  to the direct output callback functions.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 *      CSP_CACHE_ERR           -1  Internal cache error
 */
{
unsigned short charCode;
fix15   ii;
long    round = 32768L;
long    fix = 65535L << 16;
int     errCode;

#if DO_CHAR_OPTION_1
switch (((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType)
    {
case BITMAP_OUTPUT:
case ANTIALIASED_OUTPUT:
case ANTIALIASED_ALT1_OUTPUT:
    /* Test if character is completely outside output bounding box */
	if ((*pYpos < ((cspGlobals_t *)pCspGlobals)->outerClipYmin) ||
        (*pYpos > ((cspGlobals_t *)pCspGlobals)->outerClipYmax))
        {
        if (!((cspGlobals_t *)pCspGlobals)->verticalEscapement &&
        	(sp_globals.tcb.ctm[1] == 0))
        	{
        	return CSP_NO_ERR;
        	}
        }
    }
#endif
 
ii = 0;
while (TRUE)
    {
    charCode = (modeFlags & CSP_STRING_16)?
        ((ufix16 *)pString)[ii++]:
        (ufix16)(((ufix8 *)pString)[ii++]);
    if ((charCode == 0) && (length == 0))
        break;

L1: errCode = CspDoChar(
#if REENTRANT
        pCspGlobals,
#endif
        charCode,
        pXpos,
        pYpos);
    if (errCode != 0)
        {
        if ((errCode == CSP_CHAR_CODE_ERR) &&
            ((cspGlobals_t *)pCspGlobals)->missingCharActive &&
            (charCode != ((cspGlobals_t *)pCspGlobals)->missingCharCode))
            {
            charCode = ((cspGlobals_t *)pCspGlobals)->missingCharCode;
            goto L1;
            }
        return errCode;
        }

    if (modeFlags & CSP_ROUND_X_ESCAPEMENT)
        *pXpos = (*pXpos + round) & fix;
    if (modeFlags & CSP_ROUND_Y_ESCAPEMENT)
        *pYpos = (*pYpos + round) & fix;
    if ((length != 0) && (ii >= length))
        break;
    }

return CSP_NO_ERR;
}

FUNCTION
int CspDoStringWidth(
    PCSPCONTEXT
    unsigned short modeFlags,
    short length,
    void *pString,
    long *pXpos,
    long *pYpos)
/*  Executes the each of the characters in the specified string using
 *  the currently selected font and the currently selected output 
 *  specifications.
 *  *pXpos and *pYpos are updated with the escapement of each character
 *  in the string.
 *  The mode flags have the following meaning:
 *      Bit 0:  16-bit character codes
 *      Bit 1:  Round X-escapement after each character
 *      Bit 2:  Round Y-escapement after each character
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 *      CSP_CACHE_ERR           -1  Internal cache error
 */
{
unsigned short charCode;
fix15   ii;
long    round = 32768L;
long    fix = 65535L << 16;
int     errCode;

ii = 0;
while (TRUE)
    {
    charCode = (modeFlags & CSP_STRING_16)?
        ((ufix16 *)pString)[ii++]:
        (ufix16)(((ufix8 *)pString)[ii++]);
    if ((charCode == 0) && (length == 0))
        break;

L1: errCode = CspDoCharWidth(
#if REENTRANT
        pCspGlobals, 
#endif
        charCode,
        pXpos,
        pYpos);
    if (errCode != 0)
        {
        if ((errCode == CSP_CHAR_CODE_ERR) &&
            ((cspGlobals_t *)pCspGlobals)->missingCharActive &&
            (charCode != ((cspGlobals_t *)pCspGlobals)->missingCharCode))
            {
            charCode = ((cspGlobals_t *)pCspGlobals)->missingCharCode;
            goto L1;
            }
        return errCode;
        }
    if (modeFlags & CSP_ROUND_X_ESCAPEMENT)
        *pXpos = (*pXpos + round) & fix;
    if (modeFlags & CSP_ROUND_Y_ESCAPEMENT)
        *pYpos = (*pYpos + round) & fix;
    if ((length != 0) && (ii >= length))
        break;
    }

return CSP_NO_ERR;
}

FUNCTION
int CspClose(PCSPCONTEXT1)
/*
 *  Closes the character shape player
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 */

{
if ((((cspGlobals_t *)pCspGlobals)->cspState & CSPOPENMASK) != 
    CSPOPEN)                    /* CSP not open? */
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if REENTRANT
#else
pCspGlobals = BTSNULL;             /* Mark globals unallocated */
#endif

return CSP_NO_ERR;
}

/***** Undocumented API functions *****/
#if INCL_UNDOCUMENTED_API

FUNCTION
int CspTransformPoint(
    PCSPCONTEXT
    long   x,
    long   y,
    long   *pXt,
    long   *pYt)
/*
 *  Transforms the point (x, y) from character coordinates (16.16 outline 
 *  resolution units) into device coordinates (16.16 pixels) using the 
 *  current transformation matrix.
 *  The transformed point, relative to the character origin, is written
 *  into (*pXt, *pYt).
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API call sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 */
{
fix15   outlineResolution;
fix31   ctm[4];
fix15   ii;
fix31   xt, yt;

/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPOUTPUTSPECSSET + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPOUTPUTSPECSSET + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

#if CSP_MAX_DYNAMIC_FONTS > 0
if (((cspGlobals_t *)pCspGlobals)->fontCode >= 
    ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
    {
    int     errCode;

    errCode = CspLoadTopPhysFont((cspGlobals_t *)pCspGlobals);
    if (errCode != 0)
        return errCode;
    }
#endif

/* Convert current transformation matrix to pixels per oru */
outlineResolution = ((cspGlobals_t *)pCspGlobals)->fontInfo.outlineResolution;
for (ii = 0; ii < 4; ii++)
    {
    ctm[ii] = 
        (sp_globals.tcb.ctm[ii] + (outlineResolution >> 1)) / 
        outlineResolution;
    }

xt =
    CspLongMult(x, ctm[0]) +
    CspLongMult(y, ctm[2]);

yt =
    CspLongMult(x, ctm[1]) +
    CspLongMult(y, ctm[3]);
 
*pXt = xt;
*pYt = yt;

return CSP_NO_ERR;
}

FUNCTION
int CspSetFontSpecs(
    PCSPCONTEXT
    fontAttributes_t *pFontAttributes)
/*
 *  Overrides the font matrix and other rendering attributes of
 *  the currently selected font.
 *  The bitmap cache is cleared to ensure that there are no
 *  characters in the cache associated with the previous
 *  values of the font attributes.
 *
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_FONT_STYLE_ERR       9  Illegal value for fontStyle
 *      CSP_LINE_JOIN_TYPE_ERR  10  Illegal value for lineJoinType
 */
{
int     errCode;

/* Verify CSP is open and a font is selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Verify values are legal */
switch(pFontAttributes->fontStyle)
    {
case FILLED_STYLE:
    break;

case BOLD_STYLE:
    break;

case STROKED_STYLE:
    switch (pFontAttributes->styleSpecs.styleSpecsStroked.lineJoinType)
        {
    case MITER_LINE_JOIN:
    case ROUND_LINE_JOIN:
    case BEVEL_LINE_JOIN:
        break;

    default:
        return CSP_LINE_JOIN_TYPE_ERR;
        }
    break;

default:
    return CSP_FONT_STYLE_ERR;
    }

/* Update data from font attributes structure */
((cspGlobals_t *)pCspGlobals)->fontAttributes = *pFontAttributes;

/* Set up data paths and transformation constants */
if (((cspGlobals_t *)pCspGlobals)->cspState & CSPOUTPUTSPECSSET)
    {
    errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
    if (errCode != 0)
        return errCode;
    }

/* Clear the bitmap cache */
#if INCL_CACHE
CmReinitCache((cspGlobals_t *)pCspGlobals);
#endif

return CSP_NO_ERR;
}

FUNCTION
int CspGetScaledCharBBox(
    PCSPCONTEXT
    unsigned short charCode,
    CspScaledBbox_t *pBBox)
/*
 *  Measures the bounding box of the specified character in the
 *  currently selected font. Assigns to *pBBox the resulting bounding
 *  box expressed in 16.16 device coordinates.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_READ_RESOURCE_ERR    3  ReadResourceData() returned error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
short   outputType;
int     errCode;

/* Check CSP is open, output specs set and font selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPOUTPUTSPECSSET + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPOUTPUTSPECSSET + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Save current output type */
outputType = ((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType;

/* Change output type to select bounding box accumulator */
((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType = 0;

/* Set up data paths and transformation constants */
errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);
if (errCode != 0)
    return errCode;

/* Execute character with image generation enabled */
errCode = ((cspGlobals_t *)pCspGlobals)->ExecChar(
    ((cspGlobals_t *)pCspGlobals),
    charCode, 
    FALSE);
if (errCode != 0)
    {
    /* Restore output type */
    ((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType = outputType;

    /* Set up data paths and transformation constants */
    CspDoSetup((cspGlobals_t *)pCspGlobals);

    return errCode;
    }

/* Extract character bounding box */
pBBox->xmin = 
    (fix31)sp_globals.xmin << sp_globals.poshift;
pBBox->ymin = 
    (fix31)sp_globals.ymin << sp_globals.poshift;
pBBox->xmax = 
    (fix31)sp_globals.xmax << sp_globals.poshift;
pBBox->ymax = 
    (fix31)sp_globals.ymax << sp_globals.poshift;

/* Restore output type */
((cspGlobals_t *)pCspGlobals)->outputSpecs.outputType = outputType;

/* Set up data paths and transformation constants */
errCode = CspDoSetup((cspGlobals_t *)pCspGlobals);

return errCode;
}
#endif

/***** Undocumented functions used only by TrueDoc Printing Systems *****/

#if INCL_TPS
FUNCTION
int CspGetRawCharWidth(
    PCSPCONTEXT
    unsigned short charCode,
    short *pXWidth,
    short *pYWidth)
/*
 *  Accesses the specified character in the currently selected font.
 *  The X and Y components of the width value (in metrics resolution units) 
 *  are written to *pXWidth  and *pYWidth respectively.
 *  Returns:
 *      CSP_NO_ERR               0  Normal return
 *      CSP_CALL_SEQUENCE_ERR    1  API calling sequence error
 *      CSP_CHAR_CODE_ERR        7  Char not found in current font
 */
{
charRec_t charRecord;
int		errCode;

/* Verify CSP is open and a font is selected */
if ((((cspGlobals_t *)pCspGlobals)->cspState & (CSPOPENMASK + CSPFONTSELECTED)) != 
    (CSPOPEN + CSPFONTSELECTED))
    {
    return CSP_CALL_SEQUENCE_ERR;
    }

/* Find the character */
charRecord.charCode = charCode;
#if CSP_MAX_DYNAMIC_FONTS > 0
/* Check if font code refers to dynamic font */
if (((cspGlobals_t *)pCspGlobals)->fontCode >= 
    ((cspGlobals_t *)pCspGlobals)->nLogicalFonts)
    {
    errCode = CspFindNewFontChar(
        (cspGlobals_t *)pCspGlobals, &charRecord);
    }
else                            /* PFR font? */
#endif
    {
    errCode = ((cspGlobals_t *)pCspGlobals)->FindChar(
    	(cspGlobals_t *)pCspGlobals, &charRecord);
    }
if (errCode != 0)          /* Character not found? */
    {
    return errCode;
    }

/* Put the width value in the appropriate axis */
if (((cspGlobals_t *)pCspGlobals)->verticalEscapement)
    {
    *pXWidth = 0;
    *pYWidth = charRecord.setWidth;
    }
else
    {
    *pXWidth = charRecord.setWidth;
    *pYWidth = 0;
    }

return CSP_NO_ERR;
}

#if REENTRANT
FUNCTION
void *GetspGlobalPtr(
    PCSPCONTEXT1)
/*
 *	Acquire a pointer to spGlobals member of a cspGlobals_t pointer 
 */
{
	return(&sp_globals);	/* this macros to "(pCspGlobals->spGlobals)" */
}
#endif /* REENTRANT */

FUNCTION
unsigned short CspGetLogicalFontIndex(
    PCSPCONTEXT1)
/*
 *	Return the current logical font index set in cspGlobals_t pointer
 */
{
	return((unsigned short) ((cspGlobals_t *)pCspGlobals)->fontCode);
}
#endif /* INCL_TPS */

#endif /* PROC_TRUEDOC */
