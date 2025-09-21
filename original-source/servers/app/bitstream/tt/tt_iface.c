/* #define D_Stack */
/*******************************************************************************
*                                                                              *
*  Copyright 1991-1995 as an unpublished work by Bitstream Inc., Cambridge, MA *
*                                                                              *
*         These programs are the sole property of Bitstream Inc. and           *
*           contain its proprietary and confidential information.              *
*                                                                              *
*******************************************************************************/
/********************* Revision Control Information **************************
 *
*     $Header: //bliss/user/archive/rcs/true_type/tt_iface.c,v 14.1 97/05/06 10:56:48 shawn Exp $
*     $Log:	tt_iface.c,v $
 * Revision 14.1  97/05/06  10:56:48  shawn
 * Added conditional validation of Directory Table - 'Validate_TT_Dir()'.
 * 
 * Revision 14.0  97/03/17  17:55:32  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.8  97/01/14  17:41:02  shawn
 * Add "privsfnt.h" for access to required Prototype statements.
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 10.7  96/07/02  13:40:44  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 10.6  96/06/21  14:52:59  shawn
 * Initialized boolean contour type flag 'outside' in tt_make_char_idx();
 * 
 * Revision 10.5  96/03/28  11:43:10  shawn
 * Added INCL_GRAY to '#if (Any bitmap modules included?)'.
 * 
 * Revision 10.4  96/03/14  13:38:52  shawn
 * QUADRATIC_CURVES call to fn_curve() now passes P[1] as third point
 * 
 * Revision 10.3  95/11/17  09:53:25  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 10.2  95/04/11  13:03:48  shawn
 * Removed '#include <malloc.h>' - defined in <stdlib.h>
 * 
 * Revision 10.1  95/04/11  12:25:39  roberte
 * Correct some #include statements for ANSI, allowing APOLLO backdoor
 * define for building with our older compiler.
 * 
 * Revision 10.0  95/02/15  16:26:28  roberte
 * Release
 * 
 * Revision 9.4  95/01/24  09:42:55  mark
 * add special quad output for truetype.
 * 
 * Revision 9.3  95/01/12  14:46:38  roberte
 * Updated Copyright header.
 * 
 * Revision 9.2  95/01/11  13:18:29  shawn
 * Cast the sfntDirectory parameter to a type (sfnt_OffsetTable *)
 * Changed the traceFunc parameter to a type TraceFuncType
 * 
 * Revision 9.1  95/01/04  16:40:11  roberte
 * Release
 * 
 * Revision 8.6  95/01/03  13:37:00  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.5  94/10/26  16:27:57  roberte
 * Removed errant freeing of memoryBases[0-2] in tt_release_font().
 * These must stay allocated. A tt_shutdown() function would be good for this.
 * 
 * Revision 8.4  94/10/21  12:30:53  roberte
 * Added PARAMS2 macro to internal call to tt_get_pair_kern_idx.
 * This would have failed in reentrant allocation model.
 * 
 * Revision 8.3  94/08/18  10:26:01  roberte
 * Added platformID, specificID combination 0xffff, 0xffff for
 * loading a TT font but not specifying a cmap. Allows direct
 * indexing - ability to load any font regardless of what
 * cmaps it contains.
 * 
 * Revision 8.2  94/07/28  11:26:11  roberte
 * Moved "key" initialization to before check of key->offsetTableMap[sfnt_kerning].
 * Added release of memoryBases 0-2 to tt_release_font().  
 * Added (size_t) casts to mallocs of memoryBases[]. Added proper include
 * files for data type "size_t".
 * 
 * Revision 8.1  94/07/19  10:53:25  roberte
 * Added API for pair kerning: tt_get_pair_kern() and tt_get_pair_kern_idx().
 * Added conditional API, based on INCL_VERTICAL setting. If vertical support
 * is on, then tt_make_char and tt_make_char_idx pass an additional parameter
 * to new function _tt_make_char and _tt_make_char_idx. This parameter (vertical
 * TRUE/FALSE) is passed down into the processor so that fs_NewGlyph will know
 * when to substitute vertical versions of characters, when possible.
 * Addition of API tt_make_vert_char and tt_make_vert_char_idx which also use the
 * new "internal" API. 
 * If INCL_VERTICAL=0, new internal "underbar" functions don't exist, and things
 * are just as they were in previous releases of processor. No extra programming
 * interface layer unless vertical writing compile flag is on.
 * N.B. the vertical interface will generate horizontal characters when no vertical
 * version of the character is in the mort table. It seems the right way to do it.
 * 
 * Revision 8.0  94/05/04  09:35:20  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  12:00:17  roberte
 * Release
 * 
 * Revision 6.63  94/04/01  16:06:50  roberte
 * Removed now un-needed mini-prototype of tt_set_spglob().
 * 
 * Revision 6.62  94/03/23  10:54:52  roberte
 * Added new entry point tt_get_char_width_orus() and tt_get_char_width_idx_orus().
 * 
 * Revision 6.61  94/03/09  09:51:27  roberte
 * Added MAX and ABS macros.
 * Put into OLDWAY block the pointsize setting code in tt_set_specs().
 * Replaced that code with expression suggested by Ted. This should
 * solve problem when cosine produces something VERY close to zero
 * that resulted in pointsize set to 0 or 1 at 90 and 270 rotations.
 * 
 * Revision 6.60  94/02/03  12:48:02  roberte
 * Added test for NULLness before allocating the memoryBases[0..2]. Test based on memoryBases[0].
 *  Added some debug features.
 * 
 * Revision 6.59  93/12/10  12:01:07  roberte
 * Promoted the ShortXX variables in tt_rendercurve to int32. Problems on 
 * some cross-compilers.
 * 
 * Revision 6.58  93/11/03  13:20:28  roberte
 * Blocked compile of essentially whole file with conditional #if PROC_TRUETYPE
 * 
 * Revision 6.57  93/10/22  15:16:25  roberte
 * Changed paramters of split_Qbez Ax- Cy to int32.
 * Changed type of midx, midy, p1x, p1y, p2x, p2y to int32.
 * Problems on IBM platform.
 * 
 * Revision 6.56  93/10/19  12:05:38  roberte
 * Fixed memory leak caused by realloc of memoryBases[5,6,7]. Now
 * always free these in reverse order and allocate in order. Fragmentation
 * on PC was the actual problem. realloc() now no longer called.
 * 
 * Revision 6.54  93/09/27  12:18:36  mark
 * pointSize must always be positive, even if yymult is negative.  Set it
 * to absolute value, and let matrix calculation invert the sucker.
 * 
 * Revision 6.53  93/08/30  15:23:30  roberte
 * Added blank line to restore file.
 * 
 * Revision 6.52  93/07/28  13:34:42  mark
 * declare split_Qbez as void, since it returns nothing.
 * 
 * Revision 6.51  93/07/16  16:37:43  mark
 * Large scale revision of quadratic bezier handling functions (tt_rendercurve and split_Qbez)
 * 1.)  Code distance function inline using numeric approximation
 * 2.)  Convert coordinates to 16 bit form before subdividing to performance
 *      and to avoid overflow on area calculation
 * 3.)  Remove unnecessary extra count from recursive split_Qbex
 * 4.)  Remove error return from split_Qbez
 * 
 * Revision 6.50  93/07/08  16:01:46  ruey
 * add INCL_QUICK to include intercepts_t and sp_globals.intercepts
 * 
 * Revision 6.49  93/06/24  17:48:55  roberte
 * Employ the FLOOR() macro for speedup.
 * 
 * Revision 6.48  93/05/21  09:46:01  roberte
 * Removed OLDWAY blocks from speedup work.
 * Added INCL_PCLETTO logic so this option can add PCLETTO handling on top of TrueType handling.
 * 
 * Revision 6.47  93/05/18  10:37:07  roberte
 * Corrected typo in tt_load_font_params() call when #ifdef PCLETTO.
 * 
 * Revision 6.46  93/05/18  09:49:26  roberte
 * Standard C speedup work.
 * 
 * Revision 6.45  93/04/16  11:16:27  roberte
 * Changed declaration of tt_error() to match prototype.
 * 
 * Revision 6.44  93/03/15  13:24:03  roberte
 * Release
 * 
 * Revision 6.43  93/03/12  12:00:07  glennc
 * Make sure memoryBases 5, 6, and 7 are zero when opening
 * a new font.
 * 
 * Revision 6.42  93/03/12  11:26:10  glennc
 * Code cleanup for allocation/reallocation of memoryBases 5, 6
 * and 7 (bad structure of if statements caused problems).
 * 
 * Revision 6.41  93/03/11  15:51:19  roberte
 * Changed #if __MSDOS to #ifdef MSDOS.
 * 
 * Revision 6.40  93/03/10  10:46:35  glennc
 * For Apple scan converter, make sure we reallocate the 5, 6, and 7
 * memoryBases if already allocated and deallocate them when the
 * font is closed.
 * 
 * Revision 6.39  93/03/10  09:39:45  roberte
 * Changed compile time test for inclusion of malloc.h to #ifdef MSDOS
 * 
 * Revision 6.38  93/03/04  14:31:58  roberte
 * Moved lpoint_t (point tag) structure from newscan.c and tt_iface.c to fontscal.h.
 * 
 * Revision 6.37  93/03/04  13:48:29  roberte
 * Remove setting of platformID and specificID from the tt_reset() function.
 * These parameters must be set at time of fontload, when mapping is computed.
 * 
 * Revision 6.36  93/03/04  11:56:54  roberte
 * Added new function tt_load_font_params() which really does font load.
 * The existing tt_load_font() now just calls this with the params for
 * platformID and specificID as we had them set before.
 * 
 * Revision 6.35  93/02/10  15:10:58  roberte
 * Removed un-needed stack allocation of plaid in tt_make_char_idx.
 * 
 * Revision 6.34  93/02/08  16:11:54  roberte
 * Removed DAVID's code to set 4 local variables to the character
 * bounding box via a call to fs_FindBitMapSize.  We have a better
 * solution now in 4-in-1 that costs much less code.
 * 
 * Revision 6.33  93/01/29  10:58:54  roberte
 * Changed reference to sp_globals.plaid reflecting move of that
 * element back to common area of SPEEDO_GLOBALS.
 * 
 * Revision 6.32  93/01/27  15:32:51  glennc
 * Added traceFunc to tt_set_specs (only if NEEDTRACEFUNC is defined).
 * 
 * Revision 6.31  93/01/26  15:18:00  roberte
 * Corrected inadvertant use of PARAMS1 macro for dump_bitmap. Now PARAMS2.
 * 
 * Revision 6.30  93/01/26  13:38:24  roberte
 * Added PARAMS1 and PARAMS2 macros to all reentrant function calls and definitions.
 * Changed return type of split_Qbez, tt_rendercurve (int16) and dump_bitmap ().
 * Purged some int's to int16's.
 * 
 * Revision 6.29  93/01/26  10:31:35  roberte
 * Changed calls to sp_open_bitmap, sp_set_bitmap_bits and sp_close_bitmap back to no prefix.
 * 
 * Revision 6.28  93/01/25  16:51:25  roberte
 * Silly cleanup of code.
 * #ifdef'ed dump_bitmap() with INCL_APPLESCAN.  Renamed its' calls
 * to open_bitmap, set_bitmap_bits and close_bitmap to have "sp_" prefix.
 * 
 * Revision 6.27  93/01/25  13:20:45  roberte
 * Removed unused local variables declared in tt_make_char().
 * 
 * Revision 6.26  93/01/25  13:02:09  roberte
 * Changed function tt_make_char() and tt_get_char_width() to call tt_make_char_idx()
 * and tt_get_char_width_idx() respectively.  #defining GLYPH_INDEX is no longer
 * sensible or needed.
 * 
 * Revision 6.25  93/01/22  15:54:47  roberte
 * Removed 2 unreferenced varibles.
 * 
 * Revision 6.24  93/01/20  09:57:29  davidw
 * 80 column cleanup.
 * 
 * Revision 6.23  93/01/13  11:49:20  glennc
 * Add NEEDTRACEFUNC compile flag. If not defined, tt_make_char_idx has normal
 * behavior. If it is defined, tt_make_char_idx take a second argument, a
 * traceFunc to be placed in the gridFit.traceFunc member.
 * 
 * Revision 6.22  93/01/08  15:28:17  roberte
 * Changes reflecting addition of emResolution, emResRnd, sfnt_?min/max, iPtr,
 * glyph_in, oPtr, glyph_out, globalMatrix, abshift and abround to
 * SPEEDO_GLOBALS union structure.
 * 
 * Revision 6.21  92/12/29  10:10:29  roberte
 * Include spdo_prv.h, changed function declaration to older 
 * K&R style, and changed bad cast of bitmap->baseAddr
 * 
 * Revision 6.20  92/11/25  14:59:00  davidw
 * Changed pointSize reclmation from yy & xymult to account for landscape
 * orientation.
 * 
 * Revision 6.19  92/11/25  14:13:25  davidw
 * Removed function util_BitCount(), no longer used.
 * cleaned up code, comments to reflect ANSI "C"
 * 80 column cleanup
 * 
 * Revision 6.18  92/11/24  13:34:07  laurar
 * include fino.h
 * 
 * Revision 6.17  92/11/11  09:17:44  davidw
 * changed tt_make_char to use actual character wide bounding box instead of
 * the font-wide bounding box
 * 80 column cleanup
 * removed DAVIDW0 switches, except for debugging printf's
 * 
 * Revision 6.16  92/11/10  14:57:03  davidw
 * fixed debugging messages (again)
 * 
 * Revision 6.15  92/11/10  14:52:08  davidw
 * turn off debugging messages
 * 
 * Revision 6.14  92/11/10  14:19:12  davidw
 * made work sharing switchable using DAVIDW0 define
 * 
 * Revision 6.13  92/11/10  14:04:11  davidw
 * working on char bounding box problem
 * 
 * Revision 6.12  92/11/10  14:02:59  davidw
 * working on char bounding box problem
 * 
 * Revision 6.11  92/11/09  17:01:55  davidw
 * working on do_char_bbox
 * added comments
 * cleaned up code for 80 columns
 * 
 * Revision 6.10  92/11/09  14:50:50  davidw
 * added comments to code for clarity
 * 
 * Revision 6.9  92/11/09  14:41:36  davidw
 * 80 column cleanup, working on do_char_bbox(), not fixed yet, now uses font
 * wide bbox instead of char_bbox().
 * 
 * Revision 6.8  92/11/09  14:14:04  davidw
 * WIP: new character bbox code to replace do_char_bbox()
 * 
 * Revision 6.7  92/11/09  12:32:42  roberte
 * Added #ifdef PCLETTO setting of platformID for shutting of
 * cmap code during tt_reset() and tt_load_font().
 * 
 * Revision 6.6  92/11/02  12:21:06  roberte
 * Added routine tt_get_char_width_idx() so support
 * of Unicode and glyph index is consonant with the make_char functionality.
 * 
 * Revision 6.5  92/10/30  13:39:06  roberte
 * Added tenuous support for glyph code (Unicode) to tt_get_char_width().
 * 
 * Revision 6.4  92/09/28  14:26:56  roberte
 * Added param.scan. references for union fields bottomClip and topClip.
 * 
 * Revision 6.3  92/02/21  13:56:45  mark
 * Removed format checks from read_SfntOffsetTable since they are
 * verifying unused fields which are not set the same by certain
 * applications.
 * 
 * Revision 6.2  91/09/12  12:57:05  mark
 * add support for indexed glyph access (tt_make_char_idx) so that
 * we can bypass encoding table.  Enabled by flag GLYPH_INDEX
 * and only used (currently) by gsample.
 * 
 * Revision 6.1  91/08/14  16:49:23  mark
 * Release
 * 
 * Revision 5.2  91/08/14  15:11:11  mark
 * remove superfluous report_error call with MALLOC_FAILURE
 * when tt_set_spglob fails.
 * 
 * Revision 5.1  91/08/07  12:30:27  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:59:46  mark
 * fix rcs control strings.
 *  
*/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/true_type/tt_iface.c,v 14.1 97/05/06 10:56:48 shawn Exp $"
#endif

#include <setjmp.h>

#ifdef APOLLO
/* old APOLLO compiler */
#include <sys/types.h>
#else
#include <stdlib.h>
#endif

#include "spdo_prv.h"

#if PROC_TRUETYPE
#include "fino.h"
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "sc.h"
#include "fnt.h"
#include "fontscal.h"
#include "fsglue.h"
#include "truetype.h"
#include "privsfnt.h"

#include <math.h>

#ifdef MSDOS
#include <malloc.h>
#else
GLOBAL_PROTO void *malloc(size_t size);
#endif

#define SETJUMP(key, error)	if ( error = setjmp(key->env) ) return( error )

#ifdef D_Stack
extern btsbool examineStack;
#endif

#define GET_WORD(A)  ((ufix16)((ufix16)A[0] << 8) | ((ufix8)A[1]))
#define AP_PIXSHIFT 6	/* subpixel unit shift used by Apple scan converter */

#define ABS(x)	((x<0)?-x:x)
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))


#ifdef OLDWAY
/* these used to be static variables, now in sp_globals: */
static ufix16    emResolution;
static ufix16    emResRnd;
	/* Fontwide bounding box */
static fix15     sfnt_xmin;
static fix15     sfnt_xmax;
static fix15     sfnt_ymin;
static fix15     sfnt_ymax;
fs_GlyphInputType  *iPtr, glyph_in;
fs_GlyphInfoType   *oPtr, glyph_out;
static transMatrix globalMatrix;
static fix15 abshift;
static fix15 abround;
#endif

#ifdef VAL_TTDIR
LOCAL_PROTO btsbool Validate_TT_Dir(int32 clientID, sfnt_OffsetTable *sfntdir);
#endif

LOCAL_PROTO int read_SfntOffsetTable(ufix8 *ptr, sfnt_OffsetTable *dir);
LOCAL_PROTO int loadTable(ufix8 *ptr, sfnt_OffsetTable *sfntdir);
LOCAL_PROTO int32 do_char_bbox(SPGLOBPTR2 fs_GlyphInputType *iPtr, Fixed *xmin, Fixed *ymin, Fixed *xmax, Fixed *ymax);
LOCAL_PROTO void tt_error(SPGLOBPTR2 int32 error);

#if INCL_VERTICAL
LOCAL_PROTO btsbool _tt_make_char(SPGLOBPTR2 ufix16 char_code, btsbool vertical);
LOCAL_PROTO btsbool _tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, btsbool vertical);
#endif



FUNCTION btsbool tt_reset(SPGLOBPTR1)
/*  This function initializes the TrueType font interpreter. Call it
    once at startup time. Calls the TrueType Interpreter functions
    fs_OpenFonts and fs_Initialize. */

{
int32   err;

sp_globals.processor.truetype.iPtr = &sp_globals.processor.truetype.glyph_in;
sp_globals.processor.truetype.oPtr = &sp_globals.processor.truetype.glyph_out;

sp_globals.processor.truetype.iPtr->param.gridfit.styleFunc = BTSNULL;

sp_globals.processor.truetype.iPtr->GetSfntFragmentPtr = tt_get_font_fragment;
sp_globals.processor.truetype.iPtr->ReleaseSfntFrag = tt_release_font_fragment;

err = fs_OpenFonts(sp_globals.processor.truetype.iPtr,
				   sp_globals.processor.truetype.oPtr);
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

if (!sp_globals.processor.truetype.iPtr->memoryBases[0] )
	{/* only allocate if pointers are NULL (use memoryBases[0] as the test value) */
	if ((sp_globals.processor.truetype.iPtr->memoryBases[0] =
		(char *)malloc((size_t)sp_globals.processor.truetype.oPtr->memorySizes[0])) == BTSNULL)
    	goto malerr1;
	if ((sp_globals.processor.truetype.iPtr->memoryBases[1] =  
		(char *)malloc((size_t)sp_globals.processor.truetype.oPtr->memorySizes[1])) == BTSNULL)
    	goto malerr1;
	if ((sp_globals.processor.truetype.iPtr->memoryBases[2] =  
		(char *)malloc((size_t)sp_globals.processor.truetype.oPtr->memorySizes[2])) == BTSNULL)
    	goto malerr1;
	}

err = fs_Initialize(sp_globals.processor.truetype.iPtr,
				   sp_globals.processor.truetype.oPtr);
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

return TRUE;

malerr1:
  sp_report_error(PARAMS2 MALLOC_FAILURE);           /* malloc error */
  return FALSE;
} /* end tt_reset() */



	/*  This function calls fs_NewSfnt whenever a new font is needed. */

FUNCTION btsbool tt_load_font(SPGLOBPTR2 int32 fontHandle)
{
#ifdef PCLETTO
	return(tt_load_font_params(PARAMS2 fontHandle, 0xffff, 0));
#else
	return(tt_load_font_params(PARAMS2 fontHandle, 1, 0));
#endif
}

	/*  This function calls fs_NewSfnt whenever a new font is needed. */

FUNCTION btsbool tt_load_font_params(SPGLOBPTR2 int32 fontHandle, uint16 platID, uint16 specID)
{
int32   err;
ufix8  *sfptr;

sp_globals.processor.truetype.iPtr->clientID = fontHandle;
#if INCL_PCLETTO
/* set pcletto flag is platorm ID is 0xffff, but specific ID is not 0xffff */
/* That combination is reserved for loading a TT font without any cmap (direct index only) */
sp_globals.processor.truetype.iPtr->isPCLETTO = ( (platID == 0xffff) && (specID != 0xffff) );
#endif

sp_globals.processor.truetype.iPtr->param.newsfnt.platformID = platID;

sp_globals.processor.truetype.iPtr->param.newsfnt.specificID = specID;

if ((sp_globals.processor.truetype.iPtr->sfntDirectory = 
	(int32 *)malloc(sizeof(sfnt_OffsetTable))) == BTSNULL)
    goto malerr2;

sfptr = (ufix8 *)sp_globals.processor.truetype.iPtr->GetSfntFragmentPtr (sp_globals.processor.truetype.iPtr->clientID, 0L, 12L);

if (!read_SfntOffsetTable (sfptr, (sfnt_OffsetTable*)sp_globals.processor.truetype.iPtr->sfntDirectory))
    {
    tt_release_font_fragment(sfptr);
    tt_error(PARAMS2 (int32)MALLOC_FAILURE);         /* bad data in sfnt offset table */
    return(FALSE);
}

tt_release_font_fragment(sfptr);

sfptr = (ufix8 *)sp_globals.processor.truetype.iPtr->GetSfntFragmentPtr (sp_globals.processor.truetype.iPtr->clientID, 12L, ((sfnt_OffsetTable *)sp_globals.processor.truetype.iPtr->sfntDirectory)->numOffsets * 16L);

if (!loadTable (sfptr, (sfnt_OffsetTable*)sp_globals.processor.truetype.iPtr->sfntDirectory)) {
    tt_release_font_fragment(sfptr);
    tt_error(PARAMS2 (int32)MALLOC_FAILURE);         /* can't load sfnt pointers */
    return(FALSE);
}

tt_release_font_fragment(sfptr);

#ifdef VAL_TTDIR
if (!Validate_TT_Dir(sp_globals.processor.truetype.iPtr->clientID, (sfnt_OffsetTable *)sp_globals.processor.truetype.iPtr->sfntDirectory))
	return(FALSE);
#endif

err = fs_NewSfnt (sp_globals.processor.truetype.iPtr,
				  sp_globals.processor.truetype.oPtr);
if (err != NO_ERR) {
    tt_error(PARAMS2 err);
    return(FALSE);
}

if ((sp_globals.processor.truetype.iPtr->memoryBases[3] =  
	(char *)malloc((size_t)sp_globals.processor.truetype.oPtr->memorySizes[3])) == BTSNULL)
    goto malerr2;
if ((sp_globals.processor.truetype.iPtr->memoryBases[4] =  
	(char *)malloc((size_t)sp_globals.processor.truetype.oPtr->memorySizes[4])) == BTSNULL)
    goto malerr2;

#if INCL_APPLESCAN
/* [GAC] Make sure memoryBases 5, 6, and 7 are zero! */
sp_globals.processor.truetype.iPtr->memoryBases[5] =
    sp_globals.processor.truetype.iPtr->memoryBases[6] =
	sp_globals.processor.truetype.iPtr->memoryBases[7] = BTSNULL;
#endif /* INCL_APPLESCAN */

err = fs_sfntBBox (	sp_globals.processor.truetype.iPtr,
					&sp_globals.processor.truetype.sfnt_xmin,
					&sp_globals.processor.truetype.sfnt_ymin,
					&sp_globals.processor.truetype.sfnt_xmax,
					&sp_globals.processor.truetype.sfnt_ymax,
					&sp_globals.processor.truetype.emResolution );
if (err != NO_ERR){
    tt_error(PARAMS2 err);        /* sfnt has no 'head' section */
    return(FALSE);
}

sp_globals.processor.truetype.emResRnd = 
	sp_globals.processor.truetype.emResolution >> 1;

return(TRUE);

malerr2:
  tt_error(PARAMS2 (int32)MALLOC_FAILURE);           /* malloc error */
  return(FALSE);
}	/* end tt_load_font() */



FUNCTION LOCAL_PROTO void tt_error(SPGLOBPTR2 int32 error)
{
	sp_report_error(PARAMS2 (fix15)error);
	tt_release_font(PARAMS1);
}

FUNCTION btsbool tt_release_font(SPGLOBPTR1)

{/* frees memory, checking pointers for nullness, in reverse order of allocation: */
	sp_globals.processor.truetype.iPtr->clientID = 0;

/* [GAC] release 5, 6, and 7 on close (if Apple) */
#if INCL_APPLESCAN
	if (sp_globals.processor.truetype.iPtr->memoryBases[7]) {
	    free(sp_globals.processor.truetype.iPtr->memoryBases[7]);
	    sp_globals.processor.truetype.iPtr->memoryBases[7] = BTSNULL;
	}
	if (sp_globals.processor.truetype.iPtr->memoryBases[6]) {
	    free(sp_globals.processor.truetype.iPtr->memoryBases[6]);
	    sp_globals.processor.truetype.iPtr->memoryBases[6] = BTSNULL;
	}
	if (sp_globals.processor.truetype.iPtr->memoryBases[5]) {
	    free(sp_globals.processor.truetype.iPtr->memoryBases[5]);
	    sp_globals.processor.truetype.iPtr->memoryBases[5] = BTSNULL;
	}
#endif /* INCL_APPLESCAN */

	if (sp_globals.processor.truetype.iPtr->memoryBases[4])
		{
		free(sp_globals.processor.truetype.iPtr->memoryBases[4]);
		sp_globals.processor.truetype.iPtr->memoryBases[4] = BTSNULL;
		}
	if (sp_globals.processor.truetype.iPtr->memoryBases[3])
		{
		free(sp_globals.processor.truetype.iPtr->memoryBases[3]);
		sp_globals.processor.truetype.iPtr->memoryBases[3] = BTSNULL;
		}
	if (((sfnt_OffsetTable *)sp_globals.processor.truetype.iPtr->sfntDirectory)->table)
		{
		free(((sfnt_OffsetTable *)sp_globals.processor.truetype.iPtr->sfntDirectory)->table);
		((sfnt_OffsetTable *)sp_globals.processor.truetype.iPtr->sfntDirectory)->table = BTSNULL;
		}
	if (sp_globals.processor.truetype.iPtr->sfntDirectory)
		{
		free(sp_globals.processor.truetype.iPtr->sfntDirectory);
		sp_globals.processor.truetype.iPtr->sfntDirectory = BTSNULL;
		}

	return FALSE;	/* success */
}


/*
 *	THIS FUNCTION USES FLOATING POINT 
 *
 *  This function calls fs_NewSfnt whenever a new font is needed.
*/

/* [GAC] Need to add ability for tt_set_specs to allow a traceFunc */

#ifdef NEEDTRACEFUNC
FUNCTION btsbool tt_set_specs(SPGLOBPTR2 specs_t *pspecs, TraceFuncType traceFunc)
#else /* ifdef NEEDTRACEFUNC */
FUNCTION btsbool tt_set_specs(SPGLOBPTR2 specs_t *pspecs)
#endif /* ifdef NEEDTRACEFUNC */
{
double  dps;
int32   err;
btsbool tmp_err;
fix15 nn;

#ifdef OLDWAY
if (pspecs->yymult == 0)
   {
		/* can't use yymult for the point size */
	  if (pspecs->xymult < 0) 
      {
			/* landscape mode */
    		sp_globals.processor.truetype.iPtr->param.newtrans.pointSize = 
         		-pspecs->xymult;
     	}
   else 
      {
			/* portrait mode */
    		sp_globals.processor.truetype.iPtr->param.newtrans.pointSize = 
         		pspecs->xymult;
     	}
   } 
else 
   {
		/* use yymult as the pointSize (in 16.16 representation) */
   if (pspecs->yymult > 0)
      {
     	sp_globals.processor.truetype.iPtr->param.newtrans.pointSize = 
		        pspecs->yymult; 
      }
   else
      {
     	sp_globals.processor.truetype.iPtr->param.newtrans.pointSize = 
		        -pspecs->yymult; 
      }
   }
#else
    sp_globals.processor.truetype.iPtr->param.newtrans.pointSize =
		MAX(MAX(ABS(pspecs->xxmult),ABS(pspecs->xymult)),MAX(ABS(pspecs->yymult),ABS(pspecs->yxmult)));
#endif

	/* for the target display (marking) device, establish resolution */
sp_globals.processor.truetype.iPtr->param.newtrans.xResolution = 72;
sp_globals.processor.truetype.iPtr->param.newtrans.yResolution = 72;

	/*
	 *	pixelDiameter is set to sqrt(2) (0x16a0a) in 16.16 as suggested by
	 *	FS Client Interface doc
	*/
sp_globals.processor.truetype.iPtr->param.newtrans.pixelDiameter = 0x16a0a;

	/* get the current transformation matrix */
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix = 
	&sp_globals.processor.truetype.globalMatrix;

	/*
	 *	Initialize the 3 x 3 matrix array  transformMatrix[row][column]
	 *	(see  key->currentTMatrix.transform[3][3])
	*/
dps = (double)sp_globals.processor.truetype.iPtr->param.newtrans.pointSize;
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[0][0] = (Fixed)(pspecs->xxmult * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[1][0] = (Fixed)(pspecs->xymult * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[2][0] = (Fixed)(pspecs->xoffset * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[0][1] = (Fixed)(pspecs->yxmult * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[1][1] = (Fixed)(pspecs->yymult * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[2][1] = (Fixed)(pspecs->yoffset * 65536.0 / dps);
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[0][2] = 0L;
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[1][2] = 0L;
sp_globals.processor.truetype.iPtr->param.newtrans.transformMatrix->transform[2][2] = 1L << 30;

/* [GAC] may need trace function */
#ifdef NEEDTRACEFUNC
sp_globals.processor.truetype.iPtr->param.newtrans.traceFunc = traceFunc;
#else /* ifdef NEEDTRACEFUNC */
sp_globals.processor.truetype.iPtr->param.newtrans.traceFunc = BTSNULL;
#endif /* ifdef NEEDTRACEFUNC */

err = fs_NewTransformation (sp_globals.processor.truetype.iPtr, 
							sp_globals.processor.truetype.oPtr);
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

tmp_err = tt_set_spglob(PARAMS2 pspecs,
						sp_globals.processor.truetype.sfnt_xmin,
						sp_globals.processor.truetype.sfnt_ymin,
						sp_globals.processor.truetype.sfnt_xmax,
						sp_globals.processor.truetype.sfnt_ymax,
						sp_globals.processor.truetype.emResolution);
if (!tmp_err)
    return(FALSE);

	/*
	 *	Transform vectors of type 'Fixed' (16.16) to Speedo subpixel
	 *	resolution units
	 */
nn = 16 - sp_globals.pixshift;
if ((sp_globals.processor.truetype.abshift = sp_globals.pixshift - AP_PIXSHIFT) < 0)
    sp_globals.processor.truetype.abround = 1 << (-sp_globals.processor.truetype.abshift-1);
else if (sp_globals.processor.truetype.abshift == 0)
    sp_globals.processor.truetype.abround = 0;

return(TRUE);
}	/* end tt_set_specs() */


#define  INIT          0
#define  MOVE          1
#define  LINE          2
#define  CURVE         3
#define  END_CONTOUR   4
#define  END_CHAR      5
#define  ERROR        -1

#if INCL_VERTICAL
FUNCTION LOCAL_PROTO btsbool _tt_make_char(SPGLOBPTR2 ufix16 char_code, btsbool vertical)
#else
FUNCTION btsbool tt_make_char(SPGLOBPTR2 ufix16 char_code)
#endif
{
int32    err;


sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = char_code;

	/*
	 * Compute the glyph index from the character code.
 	*/
#if INCL_VERTICAL
err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, vertical);
#else
err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr);
#endif
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

/* PRR modif in */
if (sp_globals.processor.truetype.oPtr->glyphIndex == 0)
	return (FALSE);
/* PRR modif out */

#if INCL_VERTICAL
#ifdef NEEDTRACEFUNC
return (_tt_make_char_idx(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex, (TraceFuncType)BTSNULL, vertical));
#else
return (_tt_make_char_idx(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex, vertical));
#endif
#else
#ifdef NEEDTRACEFUNC
return (tt_make_char_idx(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex, (TraceFuncType)BTSNULL));
#else
return (tt_make_char_idx(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex));
#endif
#endif
}



/* make character at physical glyph index */

/* [GAC] Need to add ability for tt_make_char_idx to allow a traceFunc */

#if INCL_VERTICAL

#ifdef NEEDTRACEFUNC
FUNCTION LOCAL_PROTO btsbool _tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, TraceFuncType traceFunc, btsbool vertical)
#else
FUNCTION LOCAL_PROTO btsbool _tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, btsbool vertical)
#endif /* ifdef NEEDTRACEFUNC */

#else /* ! INCL_VERTICAL */

#ifdef NEEDTRACEFUNC
FUNCTION btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, TraceFuncType traceFunc)
#else
FUNCTION btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx)
#endif /* ifdef NEEDTRACEFUNC */

#endif /* INCL_VERTICAL */

{
int      ptype;
lpoint_t vstore[3];
int16    nn;
int32    err;
Fixed    xmin, ymin, xmax, ymax;
fix31    round;
btsbool  outside;
point_t  Pmove, Pline, Pcurve[3];
point_t  Psw, Pbbx_min, Pbbx_max;

#if INCL_OUTLINE && !QUADRATIC_CURVES
register fix31 temp;
#endif


#if REENTRANT_ALLOC
#if INCL_BLACK || INCL_WHITE || INCL_SCREEN || INCL_2D || INCL_QUICK || INCL_GRAY
intercepts_t intercepts;
sp_globals.intercepts = &intercepts;
#endif
#endif


#ifdef D_Stack
printf("***begin tt_make_char_idx()\n");
examineStack = TRUE;
#endif
nn = 16 - sp_globals.pixshift;
round = ((fix31)1 << (nn-1));

/* Initialize contour type flag */
outside = TRUE;

sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = 0xFFFF;
sp_globals.processor.truetype.iPtr->param.newglyph.glyphIndex = char_idx;
#if INCL_VERTICAL
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, vertical)) != NO_ERR)
#else
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
#endif
    {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
	}

#ifdef NEEDTRACEFUNC
sp_globals.processor.truetype.iPtr->param.gridfit.traceFunc = traceFunc;
#else /* ifdef NEEDTRACEFUNC */
sp_globals.processor.truetype.iPtr->param.gridfit.traceFunc = BTSNULL;
#endif /* ifdef NEEDTRACEFUNC */

sp_globals.processor.truetype.iPtr->param.gridfit.styleFunc = BTSNULL;

if (sp_globals.pspecs->flags & BOGUS_MODE)
	err = fs_ContourNoGridFit (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr);
else
	err = fs_ContourGridFit (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr);


#ifdef D_Stack
printf("***end tt_make_char_idx()\n");
examineStack = FALSE;
#endif
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

err = fs_GetAdvanceWidth(sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr);
if (err != NO_ERR){
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

err = do_char_bbox(PARAMS2 sp_globals.processor.truetype.iPtr, &xmin, &ymin, &xmax, &ymax);
if (err != NO_ERR){
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

Psw.x = (fix15)((sp_globals.processor.truetype.oPtr->metricInfo.advanceWidth.x + round) >> nn);
Psw.y = (fix15)((sp_globals.processor.truetype.oPtr->metricInfo.advanceWidth.y + round) >> nn);
Pbbx_min.x = (fix15)((xmin + round) >> nn);
Pbbx_min.y = (fix15)((ymin + round) >> nn);
Pbbx_max.x = (fix15)((xmax + round) >> nn);
Pbbx_max.y = (fix15)((ymax + round) >> nn);

#if INCL_APPLESCAN
if (sp_globals.output_mode == MODE_APPLE)
	{
	char *tptr;	/* [GAC] Need for realloc */
	btsbool realloc_5 = FALSE, realloc_6 = FALSE, realloc_7 = FALSE;

	if ((err = fs_FindBitMapSize (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
	    {
	    sp_report_error(PARAMS2 (fix15)err);
	    return(FALSE);
	    }
/* [GAC] Reallocate 5, 6, and 7 if already allocated */
	if ((tptr = sp_globals.processor.truetype.iPtr->memoryBases[5])==BTSNULL) {
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[5] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[5])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}
	else realloc_5 = TRUE;

	if ((tptr = sp_globals.processor.truetype.iPtr->memoryBases[6])==BTSNULL) {
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[6] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[6])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}
	else realloc_6 = TRUE;

	if ((tptr = sp_globals.processor.truetype.iPtr->memoryBases[7])==BTSNULL) {
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[7] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[7])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}
	else realloc_7 = TRUE;

	if (realloc_7)
	    	free(sp_globals.processor.truetype.iPtr->memoryBases[7]);
	if (realloc_6)
	    	free(sp_globals.processor.truetype.iPtr->memoryBases[6]);
	if (realloc_5)
	    	free(sp_globals.processor.truetype.iPtr->memoryBases[5]);

	if (realloc_7)
	{
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[7] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[7])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}
	if (realloc_6)
	{
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[6] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[6])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}
	if (realloc_5)
	{
	    if ((sp_globals.processor.truetype.iPtr->memoryBases[5] =
		 (char *)malloc(sp_globals.processor.truetype.oPtr->memorySizes[5])) == BTSNULL) {
		sp_report_error(PARAMS2 (fix15)MALLOC_FAILURE);
		return (FALSE);
	    }
	}

	sp_globals.processor.truetype.iPtr->param.scan.bottomClip = sp_globals.processor.truetype.oPtr->bitMapInfo.bounds.bottom;
	sp_globals.processor.truetype.iPtr->param.scan.topClip = sp_globals.processor.truetype.oPtr->bitMapInfo.bounds.top;

	if ((err = fs_ContourScan (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
	    {
	    sp_report_error(PARAMS2 (fix15)err);
	    return(FALSE);
	    }
	dump_bitmap(PARAMS2 &sp_globals.processor.truetype.oPtr->bitMapInfo,&Psw);
	}
else
#endif
	{
#ifdef FNDEBUG
	    printf("fn_begin_char: %d %d  %d %d %d %d\n", Psw.x,Psw.y, Pbbx_min.x,Pbbx_min.y, Pbbx_max.x,Pbbx_max.y);
#endif
	if (!fn_begin_char(Psw, Pbbx_min, Pbbx_max))
	    {
/*  ERROR   */
	    return(FALSE);
	    }
	ns_ProcOutlSetUp(sp_globals.processor.truetype.oPtr, vstore);
	ptype = INIT;
	while (ptype != END_CHAR)
	    {
	    ptype = ns_ProcOutl(sp_globals.processor.truetype.oPtr);
	    switch(ptype)
	        {
	        case MOVE:
	            if (sp_globals.processor.truetype.abshift >= 0)
	                {
	                Pmove.x = (fix15)(vstore[0].x << sp_globals.processor.truetype.abshift);
	                Pmove.y = (fix15)(vstore[0].y << sp_globals.processor.truetype.abshift);
	                }
	            else
	                {
	                Pmove.x = (fix15)((vstore[0].x + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                Pmove.y = (fix15)((vstore[0].y + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                }
	            fn_begin_contour(Pmove, (btsbool)outside);
#ifdef FNDEBUG
	    printf("fn_move: %d %d  %s\n", Pmove.x, Pmove.y, outside ? "outside" : "inside");
#endif
	            break;
	        case LINE:
	            if (sp_globals.processor.truetype.abshift >= 0)
	                {
	                Pline.x = (fix15)(vstore[0].x << sp_globals.processor.truetype.abshift);
	                Pline.y = (fix15)(vstore[0].y << sp_globals.processor.truetype.abshift);
	                }
	            else
	                {
	                Pline.x = (fix15)((vstore[0].x + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                Pline.y = (fix15)((vstore[0].y + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                }
	            fn_line(Pline);
#ifdef FNDEBUG
	    printf("fn_line: %d %d\n", Pline.x, Pline.y);
#endif
	            break;
	        case CURVE:
            /* The 3 points that define the curve are vstore[0], ... , vstore[2]  */
#if INCL_OUTLINE
	            if (sp_globals.curves_out)
	                {
#if ! QUADRATIC_CURVES
	                /* For curve output want to generate cubic Beziers from
	                 * quadratics. Use degree elevation [see G.Farin, _Curves_and_
	                 * _Surfaces_for_Computer_Aided_Geometric_Design_, p.45 ]   */
	                if (sp_globals.processor.truetype.abshift >= 0)
	                    {
	                    temp = (vstore[0].x + (vstore[1].x << 1)) << sp_globals.processor.truetype.abshift;
	                    Pcurve[0].x = (temp + (temp>=0 ? 1 : -1)) / 3;
	                    temp = (vstore[0].y + (vstore[1].y << 1)) << sp_globals.processor.truetype.abshift;
	                    Pcurve[0].y = (temp + (temp>=0 ? 1 : -1)) / 3;
	                    temp = ((vstore[1].x << 1) + vstore[2].x) << sp_globals.processor.truetype.abshift;
	                    Pcurve[1].x = (temp + (temp>=0 ? 1 : -1)) / 3;
	                    temp = ((vstore[1].y << 1) + vstore[2].y) << sp_globals.processor.truetype.abshift;
	                    Pcurve[1].y = (temp + (temp>=0 ? 1 : -1)) / 3;
	                    Pcurve[2].x = (fix15)(vstore[2].x << sp_globals.processor.truetype.abshift);
	                    Pcurve[2].y = (fix15)(vstore[2].y << sp_globals.processor.truetype.abshift);
	                    }
	                else
	                    {
	                    temp = vstore[0].x + (vstore[1].x << 1);
	                    Pcurve[0].x = ((temp + (temp>=0 ? 1 : -1)) / 3 + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    temp = vstore[0].y + (vstore[1].y << 1);
	                    Pcurve[0].y = ((temp + (temp>=0 ? 1 : -1)) / 3 + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    temp = (vstore[1].x << 1) + vstore[2].x;
	                    Pcurve[1].x = ((temp + (temp>=0 ? 1 : -1)) / 3 + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    temp = (vstore[1].y << 1) + vstore[2].y;
	                    Pcurve[1].y = ((temp + (temp>=0 ? 1 : -1)) / 3 + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    Pcurve[2].x = (vstore[2].x + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    Pcurve[2].y = (vstore[2].y + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
	                    }
	                fn_curve(Pcurve[0], Pcurve[1], Pcurve[2], (fix15)0);
#else
	                if (sp_globals.processor.truetype.abshift >= 0)
	                    {
	                    Pcurve[0].x = (fix15)(vstore[1].x << sp_globals.processor.truetype.abshift);
	                    Pcurve[0].y = (fix15)(vstore[1].y << sp_globals.processor.truetype.abshift);
	                    Pcurve[1].x = (fix15)(vstore[2].x << sp_globals.processor.truetype.abshift);
	                    Pcurve[1].y = (fix15)(vstore[2].y << sp_globals.processor.truetype.abshift);
	                    }
	                else
	                    {
	                    Pcurve[0].x = (fix15)((vstore[1].x + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                    Pcurve[0].y = (fix15)((vstore[1].y + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                    Pcurve[1].x = (fix15)((vstore[2].x + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                    Pcurve[1].y = (fix15)((vstore[2].y + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift);
	                    }
	                fn_curve(Pcurve[0], Pcurve[1], Pcurve[1], (fix15)0);

#endif
	                }

	            else
#endif
	                {
	                tt_rendercurve(PARAMS2 vstore[0].x, vstore[0].y, vstore[1].x, vstore[1].y,
	                                         vstore[2].x, vstore[2].y);
	                }
#ifdef FNDEBUG
	    printf("CURVE:   %d %d  %d %d  %d %d    %d\n", vstore[0].x,vstore[0].y, vstore[1].x,vstore[1].y, vstore[2].x,vstore[2].y, nn);
#endif
	            break;
	        case END_CONTOUR:
	            fn_end_contour();
#ifdef FNDEBUG
	    printf("fn_end_contour\n");
#endif
	            break;
	        case END_CHAR:
	            if (!fn_end_char())         /* continue loop if bitmap requires */
	                {                       /* more banding */
	                ns_ProcOutlSetUp(sp_globals.processor.truetype.oPtr, vstore);
	                ptype = INIT;
	                }
#ifdef FNDEBUG
	    printf("fn_end_char\n");
#endif
	            break;
	        default:
	            break;
	        }
	    }
	}

return(TRUE);
}


#if INCL_VERTICAL

/* horizontal/vertical API: */

FUNCTION btsbool tt_make_char(SPGLOBPTR2 ufix16 char_code)
{
	return(_tt_make_char(PARAMS2 char_code, FALSE));
}

FUNCTION btsbool tt_make_vert_char(SPGLOBPTR2 ufix16 char_code)
{
	return(_tt_make_char(PARAMS2 char_code, TRUE));
}

#ifdef NEEDTRACEFUNC
FUNCTION btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx, TraceFuncType traceFunc)
#else
FUNCTION btsbool tt_make_char_idx(SPGLOBPTR2 ufix16 char_idx)
#endif
{
#ifdef NEEDTRACEFUNC
	return(_tt_make_char_idx(PARAMS2 char_idx, traceFunc, FALSE));
#else
	return(_tt_make_char_idx(PARAMS2 char_idx, FALSE));
#endif
}

#ifdef NEEDTRACEFUNC
FUNCTION btsbool tt_make_vert_char_idx(SPGLOBPTR2 ufix16 char_idx, TraceFuncType traceFunc)
#else
FUNCTION btsbool tt_make_vert_char_idx(SPGLOBPTR2 ufix16 char_idx)
#endif
{
#ifdef NEEDTRACEFUNC
	return(_tt_make_char_idx(PARAMS2 char_idx, traceFunc, TRUE));
#else
	return(_tt_make_char_idx(PARAMS2 char_idx, TRUE));
#endif
}

#endif /* INCL_VERTICAL */



#if INCL_METRICS

FUNCTION fix31 tt_get_char_width_idx(SPGLOBPTR2 ufix16 char_index)

/*  Returns: ideal character set width in units of 1/65536 em.
    tt_set_specs _m_u_s_t_ be called before this function */

{
ufix16      width;
fix31     emwidth;
int32         err;

if ((err = fs_GetCharWidth (sp_globals.processor.truetype.iPtr, char_index, &width)) != NO_ERR)
    {
    sp_report_error(PARAMS2 (fix15)err);        /* */
    return(FALSE);
	}
emwidth = (((ufix32)width << 16) + sp_globals.processor.truetype.emResRnd) / sp_globals.processor.truetype.emResolution;
return(emwidth);
}

FUNCTION fix31 tt_get_char_width_idx_orus(SPGLOBPTR2 ufix16 char_index, ufix16 *emResolution)

/*  Returns: character set width.
	Sets *emResolution to font's resolution units
    tt_set_specs _m_u_s_t_ be called before this function */

{
ufix16      width;
int32         err;

if ((err = fs_GetCharWidth (sp_globals.processor.truetype.iPtr, char_index, &width)) != NO_ERR)
    {
    sp_report_error(PARAMS2 (fix15)err);        /* */
    return(FALSE);
	}
*emResolution = sp_globals.processor.truetype.emResolution;
return(width);
}

FUNCTION fix31 tt_get_char_width(SPGLOBPTR2 ufix16 char_code)

/*  Returns: ideal character set width in units of 1/65536 em.
    tt_set_specs _m_u_s_t_ be called before this function */

{
int32         err;

sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = char_code;
#if INCL_VERTICAL
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, FALSE)) != NO_ERR)
#else
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
#endif
    {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
	}
return(tt_get_char_width_idx(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex));
}

FUNCTION fix31 tt_get_char_width_orus(SPGLOBPTR2 ufix16 char_code, ufix16 *emResolution)

/*  Returns: character set width.
	Sets *emResolution to font's resolution units
    tt_set_specs _m_u_s_t_ be called before this function */

{
int32         err;

sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = char_code;
#if INCL_VERTICAL
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, FALSE)) != NO_ERR)
#else
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
#endif
    {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
	}
return(tt_get_char_width_idx_orus(PARAMS2 sp_globals.processor.truetype.oPtr->glyphIndex, emResolution));
}
#endif /* INCL_METRICS */


/*                                             */
/*      THIS FUNCTION USES FLOATING POINT      */
/*                                             */

FUNCTION int16 tt_rendercurve(SPGLOBPTR2 F26Dot6 Ax, F26Dot6 Ay, F26Dot6 Bx, F26Dot6 By, F26Dot6 Cx, F26Dot6 Cy)
{
int16  dx,dy,dx1,dy1,absdx,absdy;
int32  ShortAx,ShortAy,ShortBx,ShortBy,ShortCx,ShortCy;
int32  area2;
int16  dist;
int16  dist2;

point_t  Pline;
fix15   split_depth;        /* actual curve splitting depth */


/*  First, to improve speed and avoid those nasty overflows, convert the input
    coordinates from 26.6 format to the 16 bit format passed to the output module.  */

if (sp_globals.processor.truetype.abshift < 0)
   {
   ShortAx = (Ax + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   ShortAy = (Ay + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   ShortBx = (Bx + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   ShortBy = (By + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   ShortCx = (Cx + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   ShortCy = (Cy + sp_globals.processor.truetype.abround) >> -sp_globals.processor.truetype.abshift;
   }
else
   {
   ShortAx = Ax << sp_globals.processor.truetype.abshift;
   ShortAy = Ay << sp_globals.processor.truetype.abshift;
   ShortBx = Bx << sp_globals.processor.truetype.abshift;
   ShortBy = By << sp_globals.processor.truetype.abshift;
   ShortCx = Cx << sp_globals.processor.truetype.abshift;
   ShortCy = Cy << sp_globals.processor.truetype.abshift;
   }

/* Estimate point-line distance: point B to line AC
 *    area of triangle ABC = [(Bx-Ax)(Cy-Ay) - (By-Ay)(Cx-Ax)] / 2
 *    Now use Height = 2 * Area / Base                      */

/* First, approximate distance between A and C */

absdx = dx = (int16)(ShortCx-ShortAx);
absdy = dy = (int16)(ShortCy-ShortAy);

if (dx < 0) absdx = -dx;
if (dy < 0) absdy = -dy;
if (absdx > absdy)
   dist2 = (absdx + (absdy / 3));
else
   dist2 = (absdy + (absdx / 3));

split_depth = 0;
if (dist2 != 0)
    {

/* Now, calculate area of triange ABC */

    dx1 = (int16)(ShortBx-ShortAx);
    dy1 = (int16)(ShortBy-ShortAy);

    area2 = (int32)dx1*(int32)dy - (int32)dy1*(int32)dx;

    if (area2 < 0)
        area2 = -area2;

    dist = area2 / dist2;
    while (dist > sp_globals.pixrnd)
        {
        split_depth++;
        dist >>= 2;
        }
	}

#ifdef DIFFTEST
printf ("split depth %hd\n",split_depth);
#endif

if (split_depth > 0)
    {
    split_Qbez(PARAMS2 ShortAx, ShortAy, ShortBx, ShortBy, ShortCx, ShortCy,split_depth);
	}
else
    {
    Pline.x = (int16)ShortCx;
    Pline.y = (int16)ShortCy;
    fn_line(Pline);
#ifdef FNDEBUG
    printf("fn_line: %d %d\n", Pline.x, Pline.y);
#endif
	}
return(1);
}



FUNCTION void split_Qbez(SPGLOBPTR2 int32 Ax, int32 Ay, int32 Bx, int32 By, int32 Cx, int32 Cy, fix15 depth)
/*  Recursive subdivider for quadratic Beziers
*/
{
int32    midx, midy;
int32    p1x, p1y, p2x, p2y;
point_t Pline;

p1x = (Ax + Bx) >> 1;
p1y = (Ay + By) >> 1;
p2x = (Bx + Cx) >> 1;
p2y = (By + Cy) >> 1;
midx = (p1x + p2x) >> 1;
midy = (p1y + p2y) >> 1;

depth--;
if (depth == 0)
    {
    Pline.x = (int16)midx;
    Pline.y = (int16)midy;
    fn_line(Pline);
#ifdef FNDEBUG
    printf("fn_line: %d %d\n", Pline.x, Pline.y);
#endif
    Pline.x = (int16)Cx;
    Pline.y = (int16)Cy;
    fn_line(Pline);
#ifdef FNDEBUG
    printf("fn_line: %d %d\n", Pline.x, Pline.y);
#endif
    return;
    }
else
    {
    split_Qbez(PARAMS2 Ax, Ay, p1x, p1y, midx, midy, depth);
    split_Qbez(PARAMS2 midx, midy, p2x, p2y, Cx, Cy, depth);
    return;
    }
}




FUNCTION LOCAL_PROTO int read_SfntOffsetTable(ufix8 *ptr, sfnt_OffsetTable *dir)
{

dir->version = (ufix32)(GET_WORD(ptr)) << 16;
dir->version += GET_WORD((ptr+2));
dir->numOffsets = GET_WORD((ptr+4));
dir->searchRange = GET_WORD((ptr+6));
dir->entrySelector = GET_WORD((ptr+8));
dir->rangeShift = GET_WORD((ptr+10));

if (dir->numOffsets > 256)
    return(FALSE);                                          

return(TRUE);                   /* all is OK */
}

#ifdef VAL_TTDIR
FUNCTION LOCAL_PROTO btsbool Validate_TT_Dir(int32 clientID, sfnt_OffsetTable *sfntdir)
{
uint32 font_size, currOffset;
uint32 lastOffset, lastLength;
uint16 ii, jj, currIndex;


	/* Get the TT font file size */
	/* return(FALSE) if zero */

if (!(font_size = tt_get_font_size(clientID)))
	return(FALSE);

	/* OK - Check table offsets and lengths */

lastOffset = 0;
lastLength = 0;

	/* Check each table in the directory */

for (ii=0; ii < sfntdir->numOffsets; ii++)
{

		/* Find the next table in the file */

	currIndex = 0;
	currOffset = font_size;

	for (jj=0; jj < sfntdir->numOffsets; jj++)
	{

			/* Check for next higher offset */

		if ( (sfntdir->table[jj].offset > lastOffset) &&
			 (sfntdir->table[jj].offset <= currOffset) )
		{
			 currIndex = jj;
			 currOffset = sfntdir->table[currIndex].offset;
		}

	}

		/* Validate the current table */

		/* Is table overlapped? */

	if (sfntdir->table[currIndex].offset < (lastOffset + lastLength))
		return FALSE;

		/* Update the offset and length values */

	lastOffset = sfntdir->table[currIndex].offset;
	lastLength = sfntdir->table[currIndex].length;

		/* Is table within the file */

	if ( (uint32)(lastOffset + lastLength) > font_size )
		return FALSE;

}

	/* All tables are OK */

return TRUE;

}
#endif


FUNCTION LOCAL_PROTO int loadTable(ufix8 *ptr, sfnt_OffsetTable *sfntdir)
{
int16   ii;
ufix8  *pf;


sfntdir->table = (sfnt_DirectoryEntry *)
                 malloc(sfntdir->numOffsets * sizeof(sfnt_DirectoryEntry));
if (sfntdir->table == BTSNULL)
    return(FALSE);                      /* can't allocate memory for offset tables */
for (pf=ptr, ii=0; ii<sfntdir->numOffsets; ii++)
    {
    sfntdir->table[ii].tag = (ufix32)(GET_WORD(pf)) << 16;
    sfntdir->table[ii].tag += GET_WORD((pf+2));
    pf += 4;
    sfntdir->table[ii].checkSum = (ufix32)(GET_WORD(pf)) << 16;
    sfntdir->table[ii].checkSum += GET_WORD((pf+2));
    pf += 4;
    sfntdir->table[ii].offset = (ufix32)(GET_WORD(pf)) << 16;
    sfntdir->table[ii].offset += GET_WORD((pf+2));
    pf += 4;
    sfntdir->table[ii].length = (ufix32)(GET_WORD(pf)) << 16;
    sfntdir->table[ii].length += GET_WORD((pf+2));
    pf += 4;
}
return(TRUE);
}


FUNCTION LOCAL_PROTO int32 do_char_bbox(SPGLOBPTR2 fs_GlyphInputType *iPtr, Fixed *xmin, Fixed *ymin, Fixed *xmax, Fixed *ymax)
{
int    iden;
Fixed  x0 = 0, y0 = 0;
int32  err = TRUE;

	/*
	 * This Function returns 1 if the TrueType transformation matrix (reduced
	 * form) is the identity matrix; otherwise returns 0.
	 */
err = fs_IdentityTransform (iPtr, &iden);
if (err != NO_ERR){
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

	/* using the current transformatin matrix, transform the point */
err = fs_TransformPoint (iPtr, sp_globals.processor.truetype.sfnt_xmin, sp_globals.processor.truetype.sfnt_ymin, &x0, &y0);
if (err != NO_ERR){
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

	/* set bbox to known state */
*xmin = x0; *ymin = y0; *xmax = x0; *ymax = y0;

	/* using the current transformatin matrix, transform the point */
err = fs_TransformPoint (iPtr, sp_globals.processor.truetype.sfnt_xmax, sp_globals.processor.truetype.sfnt_ymax, &x0, &y0);
if (err != NO_ERR){
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

	/* update bbox if limits changed */
if (x0 < *xmin)
    *xmin = x0;
if (y0 < *ymin)
    *ymin = y0;
if (x0 > *xmax)
    *xmax = x0;
if (y0 > *ymax)
    *ymax = y0;

	/* if the transformation is the idenity matrix, we are done */
if (iden) {
    return NO_ERR;
}

	/* using the current transformatin matrix, transform the point */
err = fs_TransformPoint (iPtr, sp_globals.processor.truetype.sfnt_xmin, sp_globals.processor.truetype.sfnt_ymax, &x0, &y0);
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

	/* update bbox if limits changed */
if (x0 < *xmin)
    *xmin = x0;
if (y0 < *ymin)
    *ymin = y0;
if (x0 > *xmax)
    *xmax = x0;
if (y0 > *ymax)
    *ymax = y0;

	/* using the current transformatin matrix, transform the point */
err = fs_TransformPoint (iPtr, sp_globals.processor.truetype.sfnt_xmax, sp_globals.processor.truetype.sfnt_ymin, &x0, &y0);
if (err != NO_ERR) {
    sp_report_error(PARAMS2 (fix15)err);
    return(FALSE);
}

	/* update bbox if limits changed */
if (x0 < *xmin)
    *xmin = x0;
if (y0 < *ymin)
    *ymin = y0;
if (x0 > *xmax)
    *xmax = x0;
if (y0 > *ymax)
    *ymax = y0;

return NO_ERR;
}

#if INCL_APPLESCAN

#define TEST(p,bit) (p[bit>>3]&(0x80>>(bit&7)))

FUNCTION void dump_bitmap(SPGLOBPTR2 BitMap *bitmap, point_t *Psw) 
{
fix31 xsw, ysw;
fix31 xorg,yorg;
fix15 xsize,ysize;
ufix8 *ptr;
ufix16 y, xbit1, xbit2, curbit;
ufix16 xwid;

xsw = (fix31)Psw->x << sp_globals.poshift;
ysw = (fix31)Psw->y << sp_globals.poshift;

xorg = (fix31)bitmap->bounds.left << 16;
yorg = (fix31)bitmap->bounds.top << 16;

xsize = bitmap->bounds.right - bitmap->bounds.left;
ysize = bitmap->bounds.bottom - bitmap->bounds.top;


open_bitmap(xsw,ysw,xorg,yorg,xsize,ysize);

xwid = bitmap->rowBytes;
ptr = (ufix8 *)bitmap->baseAddr;

for (y = 0; y < ysize; y++)
	{
	curbit = 0;
	while (1)
		{
		while (TEST(ptr,curbit) == 0 && curbit < xsize)
			curbit++;
		if (curbit >= xsize)
			break;	
		xbit1 = curbit;
		while (TEST(ptr,curbit) != 0 && curbit < xsize)
			curbit++;
		xbit2 = curbit;
		set_bitmap_bits(y,xbit1,xbit2);
		}
	ptr += xwid;
	}

close_bitmap();
}
#endif

#if INCL_METRICS

#include "privfosc.h"	 /* extern of fs_SetUpKey() */

FUNCTION fix15 tt_get_pair_kern(SPGLOBPTR2 ufix16 char_code1, ufix16 char_code2)
/*  Returns a 32 bit 16.16 fixed.
*/
{
int32         err;
ufix16 char_index1, char_index2;

/* indirect each char_code -> translate to index using current cmap: */
sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = char_code1;
#if INCL_VERTICAL
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, FALSE)) != NO_ERR)
#else
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
#endif
    {
    sp_report_error(PARAMS2 (fix15)err);
    return((fix31)0);
	}
char_index1 = sp_globals.processor.truetype.oPtr->glyphIndex;
sp_globals.processor.truetype.iPtr->param.newglyph.characterCode = char_code2;
#if INCL_VERTICAL
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr, FALSE)) != NO_ERR)
#else
if ((err = fs_NewGlyph (sp_globals.processor.truetype.iPtr, sp_globals.processor.truetype.oPtr)) != NO_ERR)
#endif
    {
    sp_report_error(PARAMS2 (fix15)err);
    return((fix31)0);
	}
char_index2 = sp_globals.processor.truetype.oPtr->glyphIndex;
/* call the core function for kerning using indices: */
return(tt_get_pair_kern_idx(PARAMS2 char_index1, char_index2));
}

#define maxLinear	8
/* useful macro for unrolled binary search iterations: */
#define BinaryIteration \
	new32P = (uint32 *) ((int8 *)table32P + (searchRange >>= 1)); \
	if (targetIndex > SWAPL(*new32P)) table32P = new32P;

FUNCTION fix15 tt_get_pair_kern_idx(SPGLOBPTR2 ufix16 char_index1, ufix16 char_index2)
/*  Returns a 32 bit 16.16 fixed.
*/
{
/* VAR */
	int error;
	register fsg_SplineKey*	key;
	sfnt_KernHeader *kernHdrPtr;
	sfnt_KernSubTableHeader *kernSubTablePtr;
	uint16 ii, version, nTables, length, coverage;
	btsbool valid_format;
	fix15 kValue, kernValue = 0;

	/* code begins: */
	key = fs_SetUpKey(sp_globals.processor.truetype.iPtr, INITIALIZED | NEWSFNT | NEWTRANS, &error);

	if (key == 0)
		return error;
	SETJUMP(key, error);

	if (key->offsetTableMap[sfnt_kerning] != -1)
	{
		kernHdrPtr = (sfnt_KernHeader *)sfnt_GetTablePtr (key, sfnt_kerning, true);
		version = SWAPW(kernHdrPtr->version);
		nTables = SWAPW(kernHdrPtr->nTables);
		kernSubTablePtr = (sfnt_KernSubTableHeader *) ((int8 *)kernHdrPtr + 2*sizeof(uint16) /*sizeof(sfnt_KernHeader)*/ );
		for (ii = 0; ii < nTables; ii++)
		{
			version = SWAPW(kernSubTablePtr->version);
			length = SWAPW(kernSubTablePtr->length);
			coverage = SWAPW(kernSubTablePtr->coverage);
			valid_format = FALSE;
			/*
			 * Format is determined by a hack. Microsoft and Apple
			 * disagree about the order of interpretation of these
			 * coverage bits, making them virtually useless. For
			 * horizontal non-cross-streamed text we assume that
			 * either bit 1 or 9 is set to signify format 2.
			 */
			if ((coverage & 0x0202) == 0)
			{	/* format 0:
				 * sorted list of kerning pairs and values
				 */
				uint16 unitSize = 3*sizeof(uint16);
				uint16 nPairs, searchRange, entrySelector, rangeShift;
				uint16 *tableP;
				uint32 *table32P, *new32P;
				uint32 targetIndex = (char_index1 << 16) + char_index2;
				valid_format = TRUE;
				tableP = (ufix16 *)&kernSubTablePtr->coverage + 1;
				nPairs = SWAPWINC(tableP);
				if (nPairs > maxLinear)
				{
					searchRange = SWAPWINC(tableP);
					entrySelector = SWAPWINC(tableP);
					rangeShift = SWAPWINC(tableP);
					table32P = (uint32 *)tableP;
					if (targetIndex >= SWAPL(*((uint32 *)((int8 *)table32P+rangeShift))))
						table32P = (uint32 *)((int8 *)table32P + rangeShift);
					switch(entrySelector)
					{
					case 15: BinaryIteration;
					case 14: BinaryIteration;
					case 13: BinaryIteration;
					case 12: BinaryIteration;
					case 11: BinaryIteration;
					case 10: BinaryIteration;
					case  9: BinaryIteration;
					case  8: BinaryIteration;
					case  7: BinaryIteration;
					case  6: BinaryIteration;
					case  5: BinaryIteration;
					case  4: BinaryIteration;
					case  3:
					case  2: /* drop on through */
					case  1:
					case  0:
						break;
					}
				}
				else
					table32P = (uint32 *)((uint16 *)tableP + 3); /* skip search params */
				/* now linear search: */
				while (targetIndex > SWAPL(*table32P))
					table32P = (uint32 *) ((int8 *)table32P + 6);
				if (targetIndex == SWAPL(*table32P))
				{/* pow! We have a match */
					table32P++;	/* point to FWord value */
					kValue = SWAPW(*(uint16 *)table32P);
				}
			}
			else
			{	/* format 2:
				 * two dimensional array of kerning values - glyphs mapped to classes
				 * Currently un-supported. Poorly documented and rare.
				 */
				valid_format = TRUE;
				kValue = 0;	/* replace this with actual fetch code */
			}
#ifdef MICROSOFT_ONLY
			if (valid_format)
			{	/*************************************************************
				*  If we've got one of the recognizable kerning table formats,
				*  coverage tells how to handle the data we got from this sub-table.
				*************************************************************/
				if (coverage & 0x02)
				{/* if BIT1 set, values are minimums */
					if (kValue < kernValue)
						kernValue = kValue;	/* new minimum */
				}
				else
				{/* values are kerning values - accumulate */
					kernValue += kValue;
				}
				if (coverage & 0x08)
				{/* if BIT3 set, value in this table overrides accumulated value */
					kernValue = kValue;
				}
			}
#else
			if (valid_format)
				kernValue += kValue;	/* ambiguity, just accumulate */
#endif
			kernSubTablePtr += length;
		}/* end of for loop */

		RELEASESFNTFRAG(key, kernHdrPtr);
	}/* end of if kerning table was classified at font load? */
	return(kernValue);
}
#endif /* INCL_METRICS */

#endif /* PROC_TRUETYPE */
/* end of file tt_iface.c */
