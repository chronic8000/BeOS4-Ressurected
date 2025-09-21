/*****************************************************************************
*                                                                            *
*  Copyright 1994, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/
/***************************** CSPGLUE. C ***********************************
 * This module contains glue functions for 4-in-1 to the TrueDoc Character Set Player
 * $Header: //bliss/user/archive/rcs/4in1/cspglue.c,v 14.0 97/03/17 17:16:01 shawn Release $
 *
 * $Log:	cspglue.c,v $
 * Revision 14.0  97/03/17  17:16:01  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.10  97/01/14  17:47:51  shawn
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 13.9  96/12/12  14:24:24  shawn
 * Corrected the spelling of the maxBuffSize variable for MSDOS.
 * 
 * Revision 13.8  96/12/11  12:23:59  shawn
 * Added a test for "PFR1" compressed PFR processing.
 * 
 * Revision 13.7  96/12/05  11:00:21  shawn
 * Corrected a problem in cspg_AllocCspBuffer() where MSDOS was trying to allocate
 *      more than 64K of memory.
 * Added code to control the emboldening of TrueDoc fonts.
 * 
 * Revision 13.6  96/07/02  16:41:24  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 13.5  96/07/01  09:44:18  shawn
 * Changed PCONTEXT mmacro to PCSPCONTEXT.
 * Added function cspg_OpenNullCharShapeResource() for reentrant globals allocation.
 * 
 * Revision 13.4  96/03/28  11:51:32  shawn
 * Added support for the Gray Writer output module.
 * Inserted typedefs to cast direct output function pointers.
 * 
 * Revision 13.3  96/02/07  13:35:13  roberte
 * Corrected manipulation of font and output matrices so that
 * glue functions can set specs properly in all mirrors, rotations and etc.
 * 
 * Revision 13.2  95/04/11  12:27:53  roberte
 * Large overhaul to the set specs functionality. 
 * Corrected mirror setting which was untouched before:
 * Using TT before CSP cause it to be set wrong.
 * Improved internals for setting specified point size
 * from PFR recorded at any point size. Maintains transfromation
 * information while setting to input specs * identity, while
 * correcting for font that may have been recorded in a cartesian
 * system that is upside down (to us). This incarnation works with
 * the latest Player as well as previous Player releases.
 * Requires FixDiv(), which is conditionally defined in this
 * module (if !PROC_TRUETYPE). Otherwise, FixDiv in TT is called.
 * 
 * Revision 13.1  95/02/23  16:18:04  roberte
 * Moved setting of metric_resolution and orus_per_em here, since we
 * are already calling a query function for another purpose.
 * Added functions to set sp_globals.tcb.mirror. When dirtied by running
 * some TrueType characters, produced white scan lines mid character.
 * This was because TT had set the mirror flag, but we failed to set it
 * properly. Affected only ScreenWriter.
 *
 * Revision 13.0  95/02/15  16:37:25  roberte
 * Release
 * 
 * Revision 12.3  95/02/01  14:29:29  shawn
 * Use typedefs to cast all callback function assignments
 * 
 * Revision 12.2  95/01/26  15:05:40  shawn
 * Added GLOBAL_PROTO Macro to the CspGetLogicalFontIndex() Prototype
 * 
 * Revision 12.1  95/01/04  16:43:31  roberte
 * Release
 * 
 * Revision 1.8  94/12/28  11:00:27  roberte
 * Added pointer to void function casts on 3 assignments in 
 * cspg_SetOutputSpecs() to silence compiler warnings.
 * 
 * Revision 1.7  94/12/27  12:26:27  roberte
 * Corrections to reentrant model for macro usage...
 * 
 * Revision 1.6  94/12/27  09:27:42  roberte
 * Changed conditionals for ...CharShapeResource() function, adding
 * new multiple API cspg_OpenMultCharShapeResource(). This functionality
 * is included by default, but may be excluded by #defining MULTIPLE_PFRS 0
 * in the fino.h file or by compile-time option over-riding this default.
 * Now, the single file-based interface from the alpha release always exists,
 * but is configurable to page from disk or RAM-load entire file, just as before.
 * The new API is for multiple ROM-based PFRS. We need a way to exclude the
 * single file-based interface on demand.
 * 
 * Revision 1.5  94/12/23  13:01:47  roberte
 * Multiple PFR programming interface testing.
 * Fixed indexing of pfrAccessTable[] in cspg_OpenCharShapeResource()
 * for MULTIPLE_PFRS.
 * 
 * Revision 1.4  94/12/21  15:01:50  shawn
 * 
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 1.3  94/12/19  16:59:39  roberte
 * Added some sp_report_error() calls for TrueDoc CSP errors.
 * Changed cspg_OpenCharShapeResource() to call CspOpenAlt1() api.
 * Added conditional for ROM-only multiple PFR API for this function.
 * Not tested yet! 
 * Removed hard-wire to FILLED_STYLE in cspg_SetOutputSpecs(),
 * currently in OLDWAY block.
 * 
 * Revision 1.2  94/12/08  16:31:21  roberte
 * Remove glue layer now served by direct output functionality in TrueDoc CSP.
 * Now use font matrix from the recorded PFR rather than unit matrix in setting specs.
 * Corrections to REENTRANT implementation for the beta.
 * 
 * Revision 1.1  94/11/14  10:48:17  roberte
 * Initial revision
 * 
 *	Created:	10/19/94 roberte
 *****************************************************************************/
             
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef MSDOS
#include <malloc.h>
#include <dos.h>
#else
#include <sys/types.h>
#endif

#include "spdo_prv.h"

#ifdef LATER
#include "csp_int.h" /* also includes csp_api.h */
#else
#include "csp_api.h" /* also includes csp_api.h */

#include "fscdefs.h"
#include "fontmath.h" /* prototype of FixDiv() */

/* Simplified specs structure for output module compatibility */
typedef struct CspSpecs_tag
    {
    fix31 ctm[4];
    ufix32 flags;
    } CspSpecs_t;
#endif

#include "fino.h"

#if DEBUG
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#if PROC_TRUEDOC

/* typedefs for casting csp-dircet output function pointers */
typedef void (*DirInitOut)(void);
typedef void (*DirBeginChar)(void);
typedef void (*DirBeginSubChar)(void);
typedef void (*DirBeginContour)(void);
typedef void (*DirCurveTo)(void);
typedef void (*DirLineTo)(void);
typedef void (*DirEndContour)(void);
typedef void (*DirEndSubChar)(void);
typedef void (*DirEndChar)(void);

#define  PIXMAX      3          /* maximum pixel deviation from linear scaling */
                                /* 3 seems like a reasonable guess */
#define ABS(x)	((x<0)?-x:x)
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))

GLOBAL_PROTO unsigned short CspGetLogicalFontIndex(PCSPCONTEXT1);
GLOBAL_PROTO void cspg_type_tcb(SPGLOBPTR2 tcb_t GLOBALFAR *ptcb);
GLOBAL_PROTO void cspg_setup_tcb(SPGLOBPTR2 tcb_t GLOBALFAR *ptcb);
GLOBAL_PROTO static fix31 cspg_setup_offset(SPGLOBPTR2 fix31 input_offset);
GLOBAL_PROTO fix31 CspLongMult( fix31 x, fix31 y);

LOCAL_PROTO fix15 cspg_setup_mult(SPGLOBPTR2 fix31 input_mult);

static void *CspBufPtr;         /* CSP buffer pointer */

static FILE *pfrFilePtr;        /* PFR file pointer */

#if PFR_ON_DISK
static long  pdFileOffset;      /* Offset to start of PFR */
#else
static char *pfrBufPtr;         /* PFR buffer pointer */
#endif

#if !PROC_TRUETYPE
#ifdef NOT_ON_THE_MAC

/* otherwise, these functions accessible in TT rasterizer code: */
FUNCTION static long CompDiv(long src1, long *src2)
{
        register unsigned long src2hi = src2[0], src2lo = src2[1];
        int negative = (long)(src2hi ^ src1) < 0;

        if ((long)src2hi < 0)
                if (src2lo = -src2lo)
                        src2hi = ~src2hi;
                else
                        src2hi = -src2hi;
        if (src1 < 0)
                src1 = -src1;
        {       register unsigned long src1hi, src1lo;
                unsigned long result = 0, place = 0x40000000;

                if ((src1hi = src1) & 1)
                        src1lo = 0x80000000;
                else
                        src1lo = 0;

                src1hi >>= 1;
                src2hi += (src2lo += src1hi) < src1hi;          /* round the result */

                if (src2hi > src1hi || src2hi == src1hi && src2lo >= src1lo)
                        if (negative)
                                return NEGINFINITY;
                        else
                                return POSINFINITY;
                while (place && src2hi)
                {       src1lo >>= 1;
                        if (src1hi & 1)
                                src1lo += 0x80000000;
                        src1hi >>= 1;
                        if (src1hi < src2hi)
                        {       src2hi -= src1hi;
                                src2hi -= src1lo > src2lo;
                                src2lo -= src1lo;
                                result += place;
                        }
                        else if (src1hi == src2hi && src1lo <= src2lo)
                        {       src2hi = 0;
                                src2lo -= src1lo;
                                result += place;
                        }
                        place >>= 1;
                }
                if (src2lo >= src1)
                        result += src2lo/src1;
                if (negative)
                        return -result;
                else
                        return result;
        }
}
FUNCTION static long FixDiv (Fixed fxA, Fixed fxB)
{
    long alCompProd[2];
    
    alCompProd[0] = fxA >> 16;
    alCompProd[1] = fxA << 16;
    return CompDiv (fxB, alCompProd);
}

#endif /* NOT_ON_THE_MAC */
#endif /* !PROC_TRUETYPE */

FUNCTION ufix32 ReadLong(ufix8 *pBuff)
{
fix31 result;

result = (ufix32)(*(pBuff++));
result = (result << 8) + (ufix32)(*(pBuff++));
result = (result << 8) + (ufix32)(*(pBuff++));

return result;
}


#if PFR_ON_DISK

FUNCTION int cspg_ReadResourceData(
    void *pBuffer,
    short nBytes,
    long offset
	USERPARAM)
/*
 *  Reads nBytes of data from the character shape resource to
 *  the specified buffer starting offset bytes from the 
 *  beginning of the character shape resource.
 */
{

#if REENTRANT
FILE *pfrFilePtr = (FILE *)userParam;	/* the userParam is our RFS file pointer */
#endif

if (fseek(pfrFilePtr, pdFileOffset + offset, 0) != 0)	/* pdFileOffset is unfortunately a global VAR */
    return 1;

if (fread(pBuffer,1,(size_t)nBytes,pfrFilePtr) != (unsigned)nBytes)
    return 2;

return 0;
}
#endif  /* PFR_ON_DISK */


FUNCTION
void *cspg_AllocCspBuffer(
    long minBuffSize,
    long normalBuffSize,
    long maxBuffSize,
    long *pNBytes
	USERPARAM)
{
void *pMem;

#ifdef SOMEDAY
switch (cspMemoryMode)
    {
case 3:
    *pNBytes = maxBuffSize;
    pMem = (void *)malloc((size_t)maxBuffSize);
    if (pMem != BTSNULL)
        return pMem;

case 2:
    *pNBytes = normalBuffSize;
    pMem = (void *)malloc((size_t)normalBuffSize);
    if (pMem != BTSNULL)
        return pMem;

case 1:
    *pNBytes = minBuffSize;
    return (void *)malloc((size_t)minBuffSize);

default:
    *pNBytes = cspMemoryMode;
    return (void *)malloc((size_t)cspMemoryMode);
    }
#else

#ifdef MSDOS

        /* Check for optimal buffer size */
                            
    if (maxBuffSize < 0x10000)
        normalBuffSize = maxBuffSize;
        
    if (normalBuffSize > 0xFFFF)
        normalBuffSize = minBuffSize;

#endif

	/* Allocate "normal" buffer size */

    pMem = (void *)malloc((size_t)normalBuffSize);

        /* Check the results */

    if (pMem)
	*pNBytes = normalBuffSize;
    else
        *pNBytes = 0;

        /* Save the buffer pointer */

    CspBufPtr = pMem;

    return pMem;

#endif
}

/* cspg_OpenCharShapeResource - find character shape resource and load it */

#if MULTIPLE_PFRS
/* Interface to opening multiple PFRs (ROM based or not file-based) */

FUNCTION btsbool cspg_OpenMultCharShapeResource(PCSPCONTEXT int nPFRs, char *pfrArray[])
{
#define MAX_PFRS 8	/* no more than 8 at a time */
int	ii, errCode;


cspPfrAccess_t pfrAccessTable[MAX_PFRS];
btsbool success = FALSE;
long userparam = 0;

#if REENTRANT
void  **ppCspGlobals;
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
#endif

	if (nPFRs <= MAX_PFRS)
		{
		for (ii = 0; ii < nPFRs; ii++)
			{
			pfrAccessTable[ii].mode = PFR_ACCESS_DIRECT;
			pfrAccessTable[ii].access.pPfr = pfrArray[ii];
			}

		errCode  = CspOpenAlt1(
#if REENTRANT
					pCspGlobals,
					BTSNULL,	/* pCmGlobals */
#endif
					300             /* dpi of output device, arbitrary, affects size of buffer */,
    					(short)nPFRs,   /* number of PFRs */
    					pfrAccessTable,
    					cspg_AllocCspBuffer
#if REENTRANT
                                        , userparam
#endif
  				      );


#if REENTRANT
                ppCspGlobals = pCspGlobals;
                sp_global_ptr = GetspGlobalPtr(*ppCspGlobals);
#endif
                        /* Save the Csp buffer pointer */
                sp_globals.processor.truedoc.CspBufPtr = CspBufPtr;

		if (errCode)
			sp_report_error(PARAMS2 (fix15)(errCode+1000));
		else
			success = TRUE;
		}

	return(success);
}
#endif

/* Interface to opening single PFR (file based) */

#if PFR_ON_DISK
FUNCTION FILE *cspg_OpenCharShapeResource(PCSPCONTEXT char *pathname)
#else
FUNCTION char *cspg_OpenCharShapeResource(PCSPCONTEXT char *pathname)
#endif
{
int     nBytes;
char    smallBuff[8];
long    pfrSize;
long    pos;
int	errCode;

#if REENTRANT
void  **ppCspGlobals;
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
#endif

cspPfrAccess_t pfrAccessTable[1];

/* Open portable document file */
pfrFilePtr = fopen(pathname, "rb");
if (pfrFilePtr == BTSNULL)
    {
    printf("Cannot open portable document file %s\n", pathname);
    return BTSNULL;
    }

pos = fseek(pfrFilePtr, -8L, SEEK_END);
if (pos != 0)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

nBytes = fread(smallBuff, 1, (size_t)8, pfrFilePtr);
if (nBytes != 8)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

if (strncmp((char *)smallBuff + 3, "$PFR$", 5) != 0)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

pfrSize = (long)ReadLong((ufix8 *)smallBuff);

fseek(pfrFilePtr, -pfrSize, SEEK_END);     /* Seek to start of pfr */

#if PFR_ON_DISK
pdFileOffset = ftell(pfrFilePtr);	/* remember where it all starts */
#endif

nBytes = fread(smallBuff, 1, (size_t)8, pfrFilePtr);
if (nBytes != 8)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

#if INCL_COMPRESSED_PFR
if (strncmp(smallBuff, "PFR1", 4) != 0)
#endif
if (strncmp(smallBuff, "PFR0", 4) != 0)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

if (strncmp((char *)smallBuff + 6, "\015\012", 2) != 0)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

#if !PFR_ON_DISK
pfrBufPtr = (char *)malloc((size_t)pfrSize);
if (!pfrBufPtr)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

fseek(pfrFilePtr, -pfrSize, SEEK_END);     /* Seek to start of pfr */
nBytes = fread(pfrBufPtr, 1, (size_t)pfrSize, pfrFilePtr);
if (nBytes != pfrSize)
    {
    fclose(pfrFilePtr);
    return BTSNULL;
    }

fclose(pfrFilePtr);	/* file all read into RAM, close it */
#endif	/* !PFR_ON_DISK */

#if PFR_ON_DISK
pfrAccessTable[0].mode = PFR_ACCESS_INDIRECT;
pfrAccessTable[0].access.ReadResourceData = cspg_ReadResourceData;
#else
pfrAccessTable[0].mode = PFR_ACCESS_DIRECT;
pfrAccessTable[0].access.pPfr = pfrBufPtr;
#endif

errCode  = CspOpenAlt1(
#if REENTRANT
                        pCspGlobals,
                        BTSNULL,	/* pCmGlobals */
#endif
                        300,            /* dpi of output device */
                        1,              /* number of PFRs */
                        pfrAccessTable,
                        cspg_AllocCspBuffer
#if REENTRANT
#if PFR_ON_DISK
                        ,(long)pfrFilePtr	/* register the file pointer as process ID */
#else
                        ,(long)pfrBufPtr	/* register the buffer address as process ID */
#endif
#endif	 /* REENTRANT */
                      );


#if REENTRANT
ppCspGlobals = pCspGlobals;
sp_global_ptr = GetspGlobalPtr(*ppCspGlobals);
#endif

            /* Save the Csp buffer pointer */
sp_globals.processor.truedoc.CspBufPtr = CspBufPtr;

if (errCode)
        {
	sp_report_error(PARAMS2 (fix15)(errCode+1000));
        return BTSNULL;
        }

#if PFR_ON_DISK
return pfrFilePtr;
#else
return pfrBufPtr;
#endif

}

                                                  
#if INCL_TPS && REENTRANT

/* Function for acquiring the Csp Context pointer in REENTRANT mode */

FUNCTION void *cspg_OpenNullCharShapeResource(void)
{
int errCode;
void *pCspGlobals;

SPEEDO_GLOBALS STACKFAR *sp_global_ptr;

cspPfrAccess_t pfrAccessTable[1];

char dummy_pfr[] = { 0x50, 0x46, 0x52, 0x30, 0x00, 0x03, 0x0d, 0x0a,
                     0x00, 0x36, 0x00, 0x02, 0x00, 0x36, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
                     0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x40, 0x24, 0x50, 0x46, 0x52, 0x24 };


                /* Get the CSP Globals Pointer */

        pfrAccessTable[0].mode = PFR_ACCESS_DIRECT;
        pfrAccessTable[0].access.pPfr = dummy_pfr;

        errCode  = CspOpenAlt1( &pCspGlobals,
                                BTSNULL,                /* pCmGlobals */
                                300                     /* dpi of output device */,
                                1,                      /* number of PFRs */
                                pfrAccessTable,
                                cspg_AllocCspBuffer,
                                (long)dummy_pfr);	/* register the buffer address as process ID */

            /* Get the Speedo Context Pointer */
        sp_global_ptr = GetspGlobalPtr(pCspGlobals);

            /* Save the Csp buffer pointer */
        sp_global_ptr->processor.truedoc.CspBufPtr = CspBufPtr;

        if (errCode)
                {
	        sp_report_error(sp_global_ptr, errCode+1000);
                return BTSNULL;
                }

                /* Return the Context pointer */
        return pCspGlobals;
}

#endif  /* INCL_TPS && REENTRANT */


FUNCTION btsbool cspg_CloseCharShapeResource(PCSPCONTEXT1)
{
int errCode;

#if REENTRANT
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
#endif

                /* Close the player */

        errCode = CspClose(TPS_GLOB1);         
        
#if REENTRANT
        sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

                /* Free the Csp buffer */

        free(sp_globals.processor.truedoc.CspBufPtr);

                /* Return status */

        if (errCode)
                return FALSE;
        else
                return TRUE;

}



FUNCTION void cspg_CurveTo(TPS_GPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
{
fn_curve(P1, P2, P3, depth);
}

FUNCTION int cspg_SetOutputSpecs(PCSPCONTEXT specs_t STACKFAR *specsarg)
{
fontAttributes_t        fontAttributes;
outputSpecs_t   outputSpecs;
cspFontInfo_t   fontInfo;
unsigned short  fontRefNumber;
int             errCode;
btsbool         isNeg;


#if REENTRANT
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

                /* Set specs to Not-Valid */
		sp_globals.specs_valid = FALSE;

                /* Copy specs to globals structure and set pointer */
		sp_globals.specs = *specsarg;
		sp_globals.pspecs = &sp_globals.specs;

		/* Select output module */
		sp_globals.output_mode = (fix15)(sp_globals.pspecs->flags & 0x0007);
		sp_globals.curves_out  = (btsbool)((sp_globals.pspecs->flags & CURVES_OUT) != 0);

		switch(sp_globals.output_mode)
		{
#if INCL_BLACK
		case MODE_BLACK:                        /* Output mode Black writer */
			sp_globals.init_out =		(init_outFunc)sp_init_black;
	   	 	sp_globals.begin_char =		(begin_charFunc)sp_begin_char_black;
   	 		sp_globals.begin_sub_char =	(begin_sub_charFunc)sp_begin_sub_char_out;
   			sp_globals.begin_contour =	(begin_contourFunc)sp_begin_contour_black;
   	 		sp_globals.curve =		(curveFunc)sp_curve_out;
   			sp_globals.line =		(lineFunc)sp_line_black;
   	 		sp_globals.end_contour =	(end_contourFunc)sp_end_contour_out;
   			sp_globals.end_sub_char =	(end_sub_charFunc)sp_end_sub_char_out;
   	 		sp_globals.end_char =		(end_charFunc)sp_end_char_black;
   	 		break;
#endif

#if INCL_SCREEN
                case MODE_SCREEN:                       /* Output mode Screen writer */
			sp_globals.init_out =		(init_outFunc)sp_init_screen;
                        sp_globals.begin_char =		(begin_charFunc)sp_begin_char_screen;
                        sp_globals.begin_sub_char =	(begin_sub_charFunc)sp_begin_sub_char_out;
   			sp_globals.begin_contour =	(begin_contourFunc)sp_begin_contour_screen;
                        sp_globals.curve =		(curveFunc)sp_curve_screen;
   			sp_globals.line	=		(lineFunc)sp_line_screen;
                        sp_globals.end_contour =	(end_contourFunc)sp_end_contour_screen;
   			sp_globals.end_sub_char =	(end_sub_charFunc)sp_end_sub_char_out;
                        sp_globals.end_char =		(end_charFunc)sp_end_char_screen;
			break;
#endif

#if INCL_OUTLINE
		case MODE_OUTLINE:                      /* Output mode Vector */
			sp_globals.init_out =		(init_outFunc)sp_init_outline;
   	 		sp_globals.begin_char =		(begin_charFunc)sp_begin_char_outline;
   	 		sp_globals.begin_sub_char =	(begin_sub_charFunc)sp_begin_sub_char_outline;
   			sp_globals.begin_contour =	(begin_contourFunc)sp_begin_contour_outline;
   	 		sp_globals.curve =		(curveFunc)sp_curve_outline;
   			sp_globals.line =		(lineFunc)sp_line_outline;
   	 		sp_globals.end_contour =	(end_contourFunc)sp_end_contour_outline;
   			sp_globals.end_sub_char =	(end_sub_charFunc)sp_end_sub_char_outline;
   	 		sp_globals.end_char =		(end_charFunc)sp_end_char_outline;
			break;
#endif

#if INCL_2D
		case MODE_2D:                           /* Output mode 2d */
			sp_globals.init_out =		(init_outFunc)sp_init_2d;
   	 		sp_globals.begin_char =		(begin_charFunc)sp_begin_char_2d;
	   		sp_globals.begin_sub_char =	(begin_sub_charFunc)sp_begin_sub_char_out;
   			sp_globals.begin_contour =	(begin_contourFunc)sp_begin_contour_2d;
   	 		sp_globals.curve =		(curveFunc)sp_curve_out;
   			sp_globals.line =		(lineFunc)sp_line_2d;
   	 		sp_globals.end_contour =	(end_contourFunc)sp_end_contour_out;
   			sp_globals.end_sub_char =	(end_sub_charFunc)sp_end_sub_char_out;
   	 		sp_globals.end_char =		(end_charFunc)sp_end_char_2d;
   	 		break;
#endif

#if INCL_WHITE
		case MODE_WHITE:                        /* Output mode White writer */
                        sp_globals.init_out =           (init_outFunc)sp_init_white;
                        sp_globals.begin_char =         (begin_charFunc)sp_begin_char_white;
                        sp_globals.begin_sub_char =     (begin_sub_charFunc)sp_begin_sub_char_out;
   	 		sp_globals.begin_contour =      (begin_contourFunc)sp_begin_contour_white;
   	 		sp_globals.curve =              (curveFunc)sp_curve_out;
   	 		sp_globals.line =               (lineFunc)sp_line_white;
   	 		sp_globals.end_contour =        (end_contourFunc)sp_end_contour_white;
   	 		sp_globals.end_sub_char =       (end_sub_charFunc)sp_end_sub_char_out;
   	 		sp_globals.end_char =           (end_charFunc)sp_end_char_white;
   	 		break;
#endif

#if INCL_QUICK
		case MODE_QUICK:                        /* Output mode Quick writer */
   	 		sp_globals.init_out =           (init_outFunc)sp_init_quick;
   	 		sp_globals.begin_char =         (begin_charFunc)sp_begin_char_quick;
   	 		sp_globals.begin_sub_char =     (begin_sub_charFunc)sp_begin_sub_char_quick;
   	 		sp_globals.begin_contour =      (begin_contourFunc)sp_begin_contour_quick;
   	 		sp_globals.curve =              (curveFunc)sp_curve_quick;
   	 		sp_globals.line =               (lineFunc)sp_line_quick;
   	 		sp_globals.end_contour =        (end_contourFunc)sp_end_contour_quick;
   	 		sp_globals.end_sub_char =       (end_sub_charFunc)sp_end_sub_char_quick;
   	 		sp_globals.end_char =           (end_charFunc)sp_end_char_quick;
   	 		break;
#endif

#if INCL_GRAY
		case MODE_GRAY:                         /* Output mode Gray writer */
   	 		sp_globals.init_out =           (init_outFunc)sp_init_gray;
   	 		sp_globals.begin_char =         (begin_charFunc)sp_begin_char_gray;
   	 		sp_globals.begin_sub_char =     (begin_sub_charFunc)sp_begin_sub_char_gray;
   	 		sp_globals.begin_contour =      (begin_contourFunc)sp_begin_contour_gray;
   	 		sp_globals.curve =              (curveFunc)sp_curve_gray;
   	 		sp_globals.line =               (lineFunc)sp_line_gray;
   	 		sp_globals.end_contour =        (end_contourFunc)sp_end_contour_gray;
   	 		sp_globals.end_sub_char =       (end_sub_charFunc)sp_end_sub_char_gray;
   	 		sp_globals.end_char =           (end_charFunc)sp_end_char_gray;
   	 		break;
#endif

		default:
    		sp_report_error(PARAMS2 8);           /* Unsupported mode requested */
    		return FALSE;
		}

                /* Initialize the output module */
		if (!fn_init_out(sp_globals.pspecs))
			{
			sp_report_error(PARAMS2 5);
			return FALSE;
			}

	/* wire in old speedo output writer for CSP callbacks: */
	outputSpecs.outputType = DIRECT_OUTPUT;

	if ( !(errCode = CspSetFont(TPS_GLOB2 CspGetLogicalFontIndex(TPS_GLOB1))) )
		{

		errCode = CspGetFontSpecs(TPS_GLOB2 &fontRefNumber, &fontInfo, &fontAttributes);
		if (!errCode)
			{

			/* acquire the font matrix font was recorded at: */
			tcb_t a_tcb;

			/* setup speedo globals */
			sp_globals.metric_resolution = fontInfo.metricsResolution;
			sp_globals.orus_per_em = fontInfo.outlineResolution;

			/* Setup transformation control block */
			cspg_setup_tcb(PARAMS2 &a_tcb);
			sp_globals.tcb0.mirror = sp_globals.tcb.mirror = a_tcb.mirror;

			isNeg = (fontAttributes.fontMatrix[3] < 0);
			if (isNeg)
				fontAttributes.fontMatrix[3] = -fontAttributes.fontMatrix[3];

                        /* Process an embolden request if required */
                        if (sp_globals.pspecs->boldValue != 0)
                                {
                                fix31 bbox_height;
		                fix31 boldORU;                

				fontAttributes.fontStyle = BOLD_STYLE;

                                bbox_height = fontInfo.fontBBox.ymax - fontInfo.fontBBox.ymin;
                                boldORU = (fix31)(bbox_height * (sp_globals.pspecs->boldValue / 65635.0));
                                fontAttributes.styleSpecs.styleSpecsBold.boldThickness = (short)boldORU;
                                }

			}
		}

        if (errCode)
                {
		sp_report_error(PARAMS2 (fix15)(errCode+1000));
                return FALSE;
                }

	/* tell CSP to call our output functions: */

	outputSpecs.specs.direct.InitOut =	(DirInitOut)sp_globals.init_out;
	outputSpecs.specs.direct.BeginChar =	(DirBeginChar)sp_globals.begin_char;
	outputSpecs.specs.direct.BeginSubChar =	(DirBeginSubChar)sp_globals.begin_sub_char;
	outputSpecs.specs.direct.BeginContour =	(DirBeginContour)sp_globals.begin_contour;
	outputSpecs.specs.direct.LineTo =	(DirLineTo)sp_globals.line;
	outputSpecs.specs.direct.EndContour =	(DirEndContour)sp_globals.end_contour;
	outputSpecs.specs.direct.EndSubChar =	(DirEndSubChar)sp_globals.end_sub_char;
	outputSpecs.specs.direct.EndChar =	(DirEndChar)sp_globals.end_char;

	/* curve function, only one which must go through glue at this point: */

	if (sp_globals.curves_out)
		outputSpecs.specs.direct.CurveTo = (DirCurveTo)cspg_CurveTo;
	else
		outputSpecs.specs.direct.CurveTo = BTSNULL;

	/* set the specs compounding with the font matrix at record time */
	outputSpecs.specs.direct.outputMatrix[0] = (long)specsarg->xxmult;
	outputSpecs.specs.direct.outputMatrix[1] = (long)specsarg->yxmult;
	outputSpecs.specs.direct.outputMatrix[2] = (long)specsarg->xymult;
	outputSpecs.specs.direct.outputMatrix[3] = (long)specsarg->yymult;

	/* reduce fontMatrix to unit size, if neccessary */
	{
	long xSize, ySize;	/* 16.16 */
	long *theMatrix = (long *)&fontAttributes.fontMatrix[0];
	/* get 16.16 lines per em in x and y dimensions */
	xSize = MAX(ABS(theMatrix[0]), ABS(theMatrix[1]));
	ySize = MAX(ABS(theMatrix[2]), ABS(theMatrix[3]));
	if (ySize > (1L << 16))
		{
		/* reduce theMatrix by size found in theMatrix to unitMatrix with other stuff preserved */
		theMatrix[0] = FixDiv((Fixed)theMatrix[0], (Fixed)xSize);
		theMatrix[1] = FixDiv((Fixed)theMatrix[1], (Fixed)xSize);
		theMatrix[2] = FixDiv((Fixed)theMatrix[2], (Fixed)ySize);
		theMatrix[3] = FixDiv((Fixed)theMatrix[3], (Fixed)ySize);
		}
	}

	errCode = CspSetFontSpecs(TPS_GLOB2 &fontAttributes);

	if (errCode)
	    {
	    sp_report_error(PARAMS2 (fix15)(errCode+1000));
	    return FALSE;
	    }

	errCode = CspSetOutputSpecs(TPS_GLOB2 &outputSpecs);

	if (errCode)
	    {
	    sp_report_error(PARAMS2 (fix15)(errCode+1000));
            return FALSE;
            }

            /* Set the valid specs flag */
        sp_globals.specs_valid = TRUE;

        return TRUE;

}


FUNCTION void cspg_setup_tcb(SPGLOBPTR2 tcb_t GLOBALFAR *ptcb)
/* 
 * Convert transformation coeffs to internal form 
 */
{

ptcb->xxmult = cspg_setup_mult(PARAMS2 sp_globals.pspecs->xxmult);
ptcb->xymult = cspg_setup_mult(PARAMS2 sp_globals.pspecs->xymult);
ptcb->xoffset = cspg_setup_offset(PARAMS2 sp_globals.pspecs->xoffset);
ptcb->yxmult = cspg_setup_mult(PARAMS2 sp_globals.pspecs->yxmult);
ptcb->yymult = cspg_setup_mult(PARAMS2 sp_globals.pspecs->yymult);
ptcb->yoffset = cspg_setup_offset(PARAMS2 sp_globals.pspecs->yoffset);

SHOW(ptcb->xxmult);
SHOW(ptcb->xymult);
SHOW(ptcb->xoffset);
SHOW(ptcb->yxmult);
SHOW(ptcb->yymult);
SHOW(ptcb->yoffset);

cspg_type_tcb(PARAMS2 ptcb); /* Classify transformation type */
}

FUNCTION static fix15 cspg_setup_mult(SPGLOBPTR2 fix31 input_mult)
/*
 * Called by setup_tcb() to convert multiplier in transformation
 * matrix from external to internal form.
 */
{
fix15   imshift;       /* Right shift to internal format */
fix31   imdenom;       /* Divisor to internal format */
fix31   imrnd;         /* Rounding for division operation */

imshift = 15 - sp_globals.multshift;
imdenom = (fix31)sp_globals.orus_per_em << imshift;
imrnd = imdenom >> 1;

input_mult >>= 1;
if (input_mult >= 0)
    return (fix15)((input_mult + imrnd) / imdenom);
else
    return -(fix15)((-input_mult + imrnd) / imdenom);
}

FUNCTION void cspg_type_tcb(SPGLOBPTR2 tcb_t GLOBALFAR *ptcb)
{
fix15   xx_mult;
fix15   xy_mult;
fix15   yx_mult;
fix15   yy_mult;

/* check for mirror image transformations */
xx_mult = ptcb->xxmult;
xy_mult = ptcb->xymult;
yx_mult = ptcb->yxmult;
yy_mult = ptcb->yymult;

ptcb->mirror = ((((fix31)xx_mult*(fix31)yy_mult)-
                     ((fix31)xy_mult*(fix31)yx_mult)) < 0) ? -1 : 1;
}

FUNCTION static fix31 cspg_setup_offset(SPGLOBPTR2 fix31 input_offset)
/*
 * Called by setup_tcb() to convert offset in transformation
 * matrix from external to internal form.
 */
{
fix15   imshift;       /* Right shift to internal format */
fix31   imrnd;         /* Rounding for right shift operation */

imshift = 15 - sp_globals.multshift;
imrnd = ((fix31)1 << imshift) >> 1;

return (((input_offset >> 1) + imrnd) >> imshift) + sp_globals.mprnd;
}


#endif /* PROC_TRUEDOC */
/* EOF cspglue.c */
