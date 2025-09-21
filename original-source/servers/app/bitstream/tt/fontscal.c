/************* Revision Control Information **********************************
*                                                                            *
*     $Header: //bliss/user/archive/rcs/true_type/fontscal.c,v 14.0 97/03/17 17:53:36 shawn Release $                                                 *
*                                                                            *
*     $Log:	fontscal.c,v $
 * Revision 14.0  97/03/17  17:53:36  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.5  97/03/04  14:11:58  shawn
 * A "key->glyphIndex = key->maxProfile.numGlyphs" is not a valid glyph index.
 * 
 * Revision 10.4  96/07/02  13:42:47  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.3  96/05/10  09:28:57  shawn
 * Removed spurious trailing ';' on an 'if' statement.
 * 
 * Revision 10.2  96/03/14  13:55:07  shawn
 * Modified to account for large advance width in precision constants calculation
 * 
 * Revision 10.1  95/11/17  09:49:01  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 10.0  95/02/15  16:23:06  roberte
 * Release
 * 
 * Revision 9.2  95/02/13  12:38:53  shawn
 * Cast arguments to ShortMulDiv() to (int32) where necessary
 * 
 * Revision 9.1  95/01/04  16:32:48  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  13:39:07  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/07/19  10:42:33  roberte
 * 
 * Correction of spelling of memory throughout.
 * Addition of vertical character substitution code and internal interface.
 * 
 * Revision 8.0  94/05/04  09:29:09  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:57:18  roberte
 * Release
 * 
 * Revision 6.96  94/03/18  10:36:23  roberte
 * Put references to fs_dropOutVal() within INCL_APPLESCAN blocks.
 * 
 * Revision 6.95  94/02/11  13:20:54  roberte
 * Removed special case handling for the unknown character in PCLETTO cases.
 * We had falsely assumed that printers would display a control space
 * for unknown characters.
 * 
 * Revision 6.94  93/10/01  17:06:10  roberte
 * Added #if PROC_TRUETYPE conditional around whole file.
 * 
 * Revision 6.93  93/09/16  10:47:38  roberte
 * Added "static" to prototype of fs__Contour().
 * 
 * Revision 6.92  93/08/31  12:48:24  roberte
 * Added check for error != MISSING_FNT_TABLE after ComputeMapping.
 * Allow missing "cmap" table in postscript TrueType fonts.
 * 
 * Revision 6.91  93/08/30  14:50:48  roberte
 * Release
 * 
 * Revision 6.46  93/05/21  09:43:05  roberte
 * Added INCL_PCLETTO logic so this option can add PCLETTO handling on top of TrueType handling. 
 * 
 * Revision 6.45  93/04/23  17:47:30  roberte
 * Added (int8 *) casts for all assignments of workSpacePtr
 * 
 * Revision 6.44  93/03/15  13:10:15  roberte
 * Release
 * 
 * Revision 6.21  93/02/08  16:10:48  roberte
 * Added #if INCL_APPLESCAN check around function fs_FindBitMapSize()
 * 
 * Revision 6.20  93/01/25  09:36:34  roberte
 * Employed PROTO macro for all function prototypes.
 * 
 * Revision 6.19  93/01/22  15:55:38  roberte
 * Changed all prototypes to use new PROTO macro.
 * 
 * Revision 6.18  92/12/29  12:48:31  roberte
 * Now includes "spdo_prv.h" first.
 * 
 * Revision 6.17  92/12/15  14:12:33  roberte
 * Commented out #pragma.
 * Changed prototype function declarations to standard K&R type.
 * 
 * Revision 6.16  92/12/15  13:04:57  roberte
 * Commented unreachable code in fs_CloseFonts().
 * 
 * Revision 6.15  92/12/15  09:58:47  roberte
 * Changed return of UNDEFINED_GLYPH in fs_NewGlyph() to be within #ifdef PCLETTO block
 * 
 * Revision 6.14  92/12/09  10:42:12  roberte
 * Restored return of UNDEFINED_GLYPH if key->glyphIndex == 0
 * to the function fs_NewGlyph().
 * 
 * Revision 6.13  92/12/07  14:07:15  davidw
 * 80 column cleanup
 * q
 * 
 * Revision 6.12  92/11/24  13:34:53  laurar
 * include fino.h
 * 
 * Revision 6.11  92/11/11  09:19:36  davidw
 * made the routine fs_FindBitMapSize() available
 * 
 * Revision 6.10  92/11/10  12:33:24  roberte
 * Changed a little thing for DAVIDW so he can play!
 * 
 * Revision 6.9  92/11/10  12:28:56  davidw
 * wired out fs_FindBitMapSize() for now so others can play.
 * 
 * Revision 6.8  92/11/10  12:25:58  davidw
 * make fs_FindBitMapSize() acessable (was under #if APPLE_SCAN)
 * change fs_NewGlyph() so it doesn't check for index of 0
 * 80 column cleanup
 * 
 * Revision 6.7  92/11/10  09:43:57  roberte
 * Corrected spelling of PCLETTO.
 * 
 * Revision 6.6  92/11/09  16:39:03  roberte
 * Added callback code #ifdef PCLETTO for translation Unicode to Index.
 * 
 * Revision 6.5  92/11/09  14:27:21  davidw
 * 80 column code cleanup
 * 
 * Revision 6.4  92/11/05  13:17:35  davidw
 * 80 column cleanup
 * 
 * Revision 6.3  92/10/15  11:49:53  roberte
 * Changed all ifdef PROTOS_AVAIL statements to if PROTOS_AVAIL.
 * 
 * Revision 6.2  92/10/08  18:05:26  laurar
 * take out printf on line 1056.
 * 
 * Revision 6.1  91/08/14  16:45:22  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:33  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:42:25  mark
 * added RCS control strings
 * 
**********************************************************************/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/true_type/fontscal.c,v 14.0 97/03/17 17:53:36 shawn Release $";
#endif

/*
	File:		FontScaler.c

	Contains:	xxx put contents here (or delete the whole line) xxx

	Written by:	xxx put name of writer here (or delete the whole line) xxx

	Copyright:	) 1988-1990 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

		<13>	12/20/90	RB		Add error return to NewGlyph if glyph index
									is out of range.  [mr]
		<12>	12/10/90	RB		Fix findbitmapsize to correctly return
									error when point has migrated outside
									-32768,32767 range. [cel]
		<11>	11/27/90	MR		Need two scalars: one for (possibly
									rounded) outlines and cvt, and one (always
									fractional) metrics. [rb]
		<10>	11/21/90	RB		Allow client to disable DropOutControl by
									returning a NIL pointer to memoryarea[7].
									Also make it clear that we inhibit
									DOControl whenever we band. [This is a
									reversion to 8, so mr's initials are added
									by proxy]
		 <9>	11/13/90	MR		(dnf) Revert back to revision 7 to fix a
									memory-trashing bug (we hope). Also fix
									signed/unsigned comparison bug in outline
									caching.
		 <8>	11/13/90	RB		Fix banding so that we can band down to one
									row, using only enough bitmap memory and
									auxillary memory for one row.[mr]
		 <7>	 11/9/90	MR		Add Default return to fs_dropoutval.
									Continue to fiddle with banding. [rb]
		 <6>	 11/5/90	MR		Remove FixMath.h from include list. Clean
									up Stamp macros. [rb]
		 <5>	10/31/90	MR		Conditionalize call to ComputeMapping (to
									avoid linking MapString) [ha]
		 <4>	10/31/90	MR		Add bit-field option for integer or
									fractional scaling [rb]
		 <3>	10/30/90	RB		[MR] Inhibit DropOutControl when Banding
		 <2>	10/20/90	MR		Restore changes since project died.
									Converting to smart math routines, integer
									ppem scaling. [rb]
		<16>	 7/26/90	MR		don't include ToolUtils.h
		<15>	 7/18/90	MR		Fix return bug in GetAdvanceWidth, internal
									errors are now ints.
		<14>	 7/14/90	MR		remove unused fields from FSInfo
		<13>	 7/13/90	MR		Ansi-C fixes, rev. for union in FSInput
		<11>	 6/29/90	RB		Thus endeth the too long life of encryption
		<10>	 6/21/90	MR		Add calls to ReleaseSfntFrag
		 <9>	 6/21/90	RB		add scanKind info to fs_dropoutVal
		 <8>	  6/5/90	MR		remove fs_MapCharCodes
		 <7>	  6/1/90	MR		Did someone say MVT? Yuck!!! Out, damn
									routine.
		 <6>	  6/1/90	RB		fixed bandingbug under dropout control
		 <4>	  5/3/90	RB		added dropoutval function.  simplified
									restore outlines. support for new
									scanconverter in contourscan,
									findbitmapsize, saveoutlines,
									restoreoutlines.
		 <3>	 3/20/90	CL		Changed to use fpem (16.16) instead of
									pixelsPerEm (int) Removed call to
									AdjustTransformation (not needed with fpem)
									Added call to RunXFormPgm Removed
									WECANNOTDOTHIS #ifdef Added fs_MapCharCodes
		 <2>	 2/27/90	CL		New error code for missing but needed
									table. (0x1409 ). New CharToIndexMap Table
									format.  Fixed transformed component bug.
	   <3.6>	11/15/89	CEL		Put an else for the ifdef WeCanNotDoThis so
									Printer compile
									could use more effecient code.
	   <3.5>	11/14/89	CEL		Left Side Bearing should work right for any
									transformation. The phantom points are in,
									even for components in a composite glyph. 
									They should also work for transformations.
									Device metric are passed out in the output
									data structure. This should also work with
									transformations. Another leftsidebearing
									along the advance width vector is also
									passed out. whatever the metrics are for
									the component at it's level. Instructions
									are legal in components. Instructions are
									legal in components. The transformation is
									internally automatically normalized. This
									should also solve the overflow problem we
									had. Now it is legal to pass in zero as the
									address of memory when a piece of the sfnt
									is requested by the scaler. If this happens
									the scaler will simply exit with an error
									code ! Five unnecessary element in the
									output data structure have been deleted.
									(All the information is passed out in the
									bitmap data structure) fs_FindBMSize now
									also returns the bounding box.
	   <3.4>	 9/28/89	CEL		fs_newglyph did not initialize the output
									error. Caused routine to return error from
									previous routines.
	   <3.3>	 9/27/89	CEL		Took out devAdvanceWidth and
									devLeftSideBearing.
	   <3.2>	 9/25/89	CEL		Changed the NEED_PROTOTYPE ifdef to use the
									NOT_ON_THE_MAC flag that existed previously.
	   <3.1>	 9/15/89	CEL		Changed dispatch schemeI Calling
									conventions through a trap needed to match
									Macintosh pascal. Pascal can not call C
									unless there is extra mucky glue. Bug that
									caused text not to appearI The font scaler
									state was set up correctly but the sfnt was
									purged. It was reloaded and the clientid
									changed but was still the same font. Under
									the rules of the FontScaler fs_newsfnt
									should not have to be called again to reset
									the state. The extra checks sent back a
									BAD_CLIENTID_ERROR so QuickDraw would think
									it was a bad font and not continue to draw.
	   <3.0>	 8/28/89	sjk		Cleanup and one transformation bugfix
	   <2.4>	 8/17/89	sjk		Coded around MPW C3.0 bug
	   <2.3>	 8/14/89	sjk		1 point contours now OK
	   <2.2>	  8/8/89	sjk		Improved encryption handling
	   <2.1>	  8/2/89	sjk		Fixed outline caching bug
	   <2.0>	  8/2/89	sjk		Just fixed EASE comment
	   <1.5>	  8/1/89	sjk		Added composites and encryption. Plus some
									enhancementsI
	   <1.4>	 6/13/89	SJK		Comment
	   <1.3>	  6/2/89	CEL		16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem
									behavior, pretty much so
	   <1.2>	 5/26/89	CEL		EASE messed up on RcS comments
	  <%1.1>	 5/26/89	CEL		Integrated the new Font Scaler 1.0 into
									Spline Fonts
	   <1.0>	 5/25/89	CEL		Integrated 1.0 Font scaler into Bass code
									for the first timeI
	To Do:
*/
/*		<3+>	 3/20/90	mrr		Conditionalized error checking in
									fs_SetUpKey.
									Compiler option for stamping memory areas
									for debugging
									Removed error field from FSInfo structure.
									Added call to RunFontProgram
									Added private function prototypes.
									Optimizations from diet clinic
*/

#include <setjmp.h>

#include "spdo_prv.h"

#if PROC_TRUETYPE
#include "fino.h"
/** FontScalerUs Includes **/
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "sc.h"
#include "fnt.h"
#include "fontscal.h"
#include "fsglue.h"
#include "privsfnt.h"
#include "privfosc.h"


#define LOOPDOWN(n)		for (--n; n >= 0; --n)

#define OUTLINEBIT    0x02

#define SETJUMP(key, error)	if ( error = setjmp(key->env) ) return( error )

#ifdef SEGMENT_LINK
/* #pragma segment FONTSCALER_C */
#endif

#ifndef NOT_ON_THE_MAC
#define USE_OUTLINE_CACHE
#endif

/* PRIVATE PROTOTYPES */
LOCAL_PROTO int32 fs__Contour(fs_GlyphInputType *, fs_GlyphInfoType *, btsbool);
GLOBAL_PROTO int32 fs_SetSplineDataPtrs(fs_GlyphInputType *, fs_GlyphInfoType *);
GLOBAL_PROTO int32 fs_SetInternalOffsets(fs_GlyphInputType *, fs_GlyphInfoType *);

#if INCL_APPLESCAN
LOCAL_PROTO int32 fs_dropOutVal(fsg_SplineKey *key);
#endif	 /* INCL_APPLESCAN */

#if INCL_VERTICAL
LOCAL_PROTO uint16 fs_VerticalSubstitution(fsg_SplineKey *key, ufix16 glyphIndex);
#endif

#ifdef	DEBUGSTAMP
#define STAMPEXTRA		4
#define STAMP			'sfnt'
	
FUNCTION void SETSTAMP(Ptr p)
	{
		*((int32*)((p) - STAMPEXTRA)) = STAMP;
	}

FUNCTION void CHECKSTAMP(Ptr p)
	{
		if (*((int32*)((p) - STAMPEXTRA)) != STAMP) Debugger();
	}

#else
#define STAMPEXTRA		0
#define SETSTAMP(p)
#define CHECKSTAMP(p)
#endif

#ifdef RELEASE_MEM_FRAG

FUNCTION void dummyReleaseSfntFrag(void *data)
{
	/* not much here ... */
}
#endif


/*
 *	Set up the key in case memory has moved or been purged.
 */

FUNCTION fsg_SplineKey *fs_SetUpKey(register fs_GlyphInputType *inptr, register unsigned stateBits, int *error)
{
	register fsg_SplineKey* key;

#ifdef DEBUG		/* <4> */
	if ( !(key = (fsg_SplineKey*)inptr->memoryBases[KEY_PTR_BASE]) ) {
		*error = NULL_KEY_ERR;
		return 0;
	}
	if ( !(key->memoryBases = inptr->memoryBases) ) {
		*error = NULL_MEMORY_BASES_ERR;
		return 0;
	}
	if ( !(key->sfntDirectory = (sfnt_OffsetTable*)inptr->sfntDirectory) ) {
		*error = NULL_SFNT_DIR_ERR;
		return 0;
	}
	if ( !(key->GetSfntFragmentPtr = inptr->GetSfntFragmentPtr) ) {
		*error = NULL_SFNT_FRAG_PTR_ERR;
		return 0;
	}
#else
	key = (fsg_SplineKey*)inptr->memoryBases[KEY_PTR_BASE];
	key->memoryBases = inptr->memoryBases;
	key->sfntDirectory = (sfnt_OffsetTable*)inptr->sfntDirectory;
	key->GetSfntFragmentPtr = inptr->GetSfntFragmentPtr;
#endif

#ifdef RELEASE_MEM_FRAG
	if (inptr->ReleaseSfntFrag)
		key->ReleaseSfntFrag = inptr->ReleaseSfntFrag;
	else
		key->ReleaseSfntFrag = dummyReleaseSfntFrag;
#endif

	if ( (key->state & stateBits) != stateBits ) {
		*error = OUT_OFF_SEQUENCE_CALL_ERR;
		return 0;
	}
	key->clientID = inptr->clientID;
	*error = NO_ERR;

	return key;
}	/* end fs_SetUpKey() */


/*** FONT SCALER INTERFACE ***/

FUNCTION int32 fs_OpenFonts(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	if ( outputPtr ) {
		outputPtr->memorySizes[KEY_PTR_BASE] = fsg_KeySize( ) + STAMPEXTRA;
		outputPtr->memorySizes[VOID_FUNC_PTR_BASE]
			= fsg_InterPreterDataSize() + STAMPEXTRA;
		outputPtr->memorySizes[SCAN_PTR_BASE]
			= fsg_ScanDataSize() + STAMPEXTRA;
			/* we need the sfnt for these */
		outputPtr->memorySizes[WORK_SPACE_BASE] = 0;
		outputPtr->memorySizes[PRIVATE_FONT_SPACE_BASE]	= 0;
			/* we need the grid fitted outline for these */
		outputPtr->memorySizes[BITMAP_PTR_1] = 0;
		outputPtr->memorySizes[BITMAP_PTR_2] = 0;
		outputPtr->memorySizes[BITMAP_PTR_3] = 0;
	} else
		return NULL_OUTPUT_PTR_ERR;
	if ( inputPtr ) {
		inputPtr->memoryBases[KEY_PTR_BASE]  			= 0;
		inputPtr->memoryBases[VOID_FUNC_PTR_BASE]  		= 0;
		inputPtr->memoryBases[SCAN_PTR_BASE]   			= 0;
		inputPtr->memoryBases[WORK_SPACE_BASE]  		= 0;
		inputPtr->memoryBases[PRIVATE_FONT_SPACE_BASE]	= 0;
		inputPtr->memoryBases[BITMAP_PTR_1]  			= 0;
		inputPtr->memoryBases[BITMAP_PTR_2]  			= 0;
		inputPtr->memoryBases[BITMAP_PTR_3]  			= 0;
	} else
		return NULL_INPUT_PTR_ERR;
	return NO_ERR;
}	/* end fs_OpenFonts() */



FUNCTION int32 fs_Initialize(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register fsg_SplineKey 			*key;
	fnt_GlobalGraphicStateType		tmpGS;

	if (outputPtr != 0) {

		if (inputPtr == 0)
			return NULL_INPUT_PTR_ERR;

		SETSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
		SETSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
		SETSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

		key = (fsg_SplineKey *)inputPtr->memoryBases[KEY_PTR_BASE];
		if (key == 0)
			return NULL_KEY_ERR;

		key->memoryBases = inputPtr->memoryBases;


		tmpGS.function = (FntFunc*)key->memoryBases[VOID_FUNC_PTR_BASE];
		if (tmpGS.function != 0) {
			fnt_Init( &tmpGS );
			key->state = INITIALIZED;
		} else
			return VOID_FUNC_PTR_BASE_ERR;

		CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
		CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
		CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

		return NO_ERR;
	}
		return NULL_OUTPUT_PTR_ERR;
}	/* end fs_Initialize() */


/*
 *	This guy asks for memory for points, instructions, fdefs and idefs
 */

FUNCTION int32 fs_NewSfnt(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	int error;
/*	register fsg_SplineKey* key = fs_SetUpKey(inputPtr, INITIALIZED, &error); */
	register fsg_SplineKey* key;
	key = fs_SetUpKey(inputPtr, INITIALIZED, &error);
	if (!key) return error;
	SETJUMP(key, error);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

		/* Map offset and length table */
	sfnt_DoOffsetTableMap( key );

	{
		sfnt_FontHeader* fontHead =
			(sfnt_FontHeader*)sfnt_GetTablePtr(key, sfnt_fontHeader, true );
		sfnt_HorizontalHeader* horiHead =
			(sfnt_HorizontalHeader*)sfnt_GetTablePtr(key,sfnt_horiHeader,true);

		if ( SWAPL(fontHead->magicNumber) != SFNT_MAGIC )
			return BAD_MAGIC_ERR;

		key->emResolution = SWAPW(fontHead->unitsPerEm);
		key->fontFlags = SWAPW(fontHead->flags);
		key->numberOf_LongHorMetrics = SWAPW(horiHead->numberOf_LongHorMetrics);

		RELEASESFNTFRAG(key, horiHead);
		RELEASESFNTFRAG(key, fontHead);
	}
	{
		void* p = sfnt_GetTablePtr( key, sfnt_maxProfile, true );
		key->maxProfile = *((sfnt_maxProfileTable*)p);
		RELEASESFNTFRAG(key, p);
	}

	outputPtr->memorySizes[PRIVATE_FONT_SPACE_BASE]	=
		fsg_PrivateFontSpaceSize( key ) + STAMPEXTRA;
	outputPtr->memorySizes[WORK_SPACE_BASE]	=
		fsg_WorkSpaceSetOffsets( key ) + STAMPEXTRA;

#ifndef NEVERCOMPUTEMAPPING
	error = sfnt_ComputeMapping(key,
								inputPtr->param.newsfnt.platformID,
								inputPtr->param.newsfnt.specificID);
	if ( (error != 0) && (error != MISSING_SFNT_TABLE) )
		return error;
#endif

	key->state = INITIALIZED | NEWSFNT;
	key->scanControl = 0;
		
	/*
	 *	Can't run font program yet, we don't have any memory for the graphic
	 *	state. Mark it to be run in NewTransformation.
	 */
	key->executeFontPgm = true;

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

	return NO_ERR;
}	/* end fs_NewSfnt() */



FUNCTION int32 fs_NewTransformation(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register fsg_SplineKey 		*key;
	int16						resolution;
	int							error;

	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT, &error)))
		return error;
	SETJUMP(key, error);
			
	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);

	SETSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	SETSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	key->currentTMatrix = *inputPtr->param.newtrans.transformMatrix;
	key->fixedPointSize = inputPtr->param.newtrans.pointSize;
	key->pixelDiameter	= inputPtr->param.newtrans.pixelDiameter;

	if ( inputPtr->param.newtrans.xResolution >=  inputPtr->param.newtrans.yResolution )
        {   
        	short yRes = inputPtr->param.newtrans.yResolution;
		resolution = inputPtr->param.newtrans.xResolution;
		if ( resolution != yRes )
                {
			key->currentTMatrix.transform[0][1] =
				ShortMulDiv(key->currentTMatrix.transform[0][1],
							(int32)yRes, (int32)resolution );
			key->currentTMatrix.transform[1][1] =
				ShortMulDiv(key->currentTMatrix.transform[1][1],
							(int32)yRes, (int32)resolution );
			key->currentTMatrix.transform[2][1] =
				ShortMulDiv( key->currentTMatrix.transform[2][1],
							(int32)yRes, (int32)resolution );
		}
        }
        else
        {
		short xRes = inputPtr->param.newtrans.xResolution;
		resolution = inputPtr->param.newtrans.yResolution;
		key->currentTMatrix.transform[0][0] =
			ShortMulDiv(key->currentTMatrix.transform[0][0],
						(int32)xRes, (int32)resolution );
		key->currentTMatrix.transform[1][0] =
			ShortMulDiv(key->currentTMatrix.transform[1][0], (int32)xRes, (int32)resolution);
		key->currentTMatrix.transform[2][0] =
			ShortMulDiv(key->currentTMatrix.transform[2][0], (int32)xRes, (int32)resolution);
	}
	key->metricScalar =
		ShortMulDiv( key->fixedPointSize, (int32)resolution, (int32)POINTSPERINCH );

	/*
	 *	Modifies key->fpem and key->currentTMatrix.
	 */
	fsg_ReduceMatrix( key );

	if (key->fontFlags & USE_INTEGER_SCALING)
		key->interpScalar = (key->metricScalar + 0x8000) & 0xffff0000;
	else
		key->interpScalar = key->metricScalar;

	/***********************************************************************
	 *	At this point, we have
	 *		fixedPointSize = user defined fixed
	 *		metricScalar   = fixed scaler for scaling metrics
	 *		interpScalar   = fixed scaler for scaling outlines/CVT
	 *		pixelDiameter  = user defined fixed
	 *		currentTMatrix = 3x3 user transform and non-squareness resolution
	*/ 

		/* get premultipliers if any, also called in sfnt_ReadSFNT */
	fsg_InitInterpreterTrans(key);

	/*
	 *	This guy defines FDEFs and IDEFs.  The table is optional
	 */
	if (key->executeFontPgm)
        {

		error = fsg_SetDefaults(key);
		if(error != 0)
			return error;

		error = fsg_RunFontProgram(key, inputPtr->param.newtrans.traceFunc);
		if (error != 0)
			return error;

		key->executeFontPgm = false;
	}

	if ( inputPtr->param.newtrans.traceFunc )
        {
		/* Do this now so we do not confuse font editors */
		/* Run the pre program and scale the control value table */
		/* Sets key->executePrePgm to false */

		error = fsg_NewTransformation(key, inputPtr->param.newtrans.traceFunc);
		if (error != 0)
			return error;
	}
        else
        {
		/* otherwise postpone as long as possible */
		key->executePrePgm = true;
	}

	key->state = INITIALIZED | NEWSFNT | NEWTRANS;
	
	outputPtr->scaledCVT =
		(F26Dot6 *)(key->memoryBases[PRIVATE_FONT_SPACE_BASE] +
		key->offset_controlValues);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	return NO_ERR;
}	/* end fs_NewTransformation() */


/*
 * Compute the glyph index from the character code.
 */

#if INCL_VERTICAL
FUNCTION int32 fs_NewGlyph(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr, btsbool vertical)
#else
FUNCTION int32 fs_NewGlyph(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
#endif
{
	int error;
	register fsg_SplineKey* key = fs_SetUpKey(inputPtr, 0, &error);

	if (key == 0)
		return error;
	SETJUMP(key, error);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	key->state = INITIALIZED | NEWSFNT | NEWTRANS;	/* clear all other bits */

	if ( inputPtr->param.newglyph.characterCode != NONVALID ) {
#ifdef PCLETTO	 /* exclusively PCLETTO processing */
		uint16 glyphIndex =
			tt_UnicodeToIndex( inputPtr->param.newglyph.characterCode );
#else			/* TRUETYPE and possibly PCLETTO handling */
		uint16 glyphIndex;
#if INCL_PCLETTO	/* PCLETTO handling included */
			if (inputPtr->isPCLETTO)
				glyphIndex = tt_UnicodeToIndex( inputPtr->param.newglyph.characterCode );
			else
				glyphIndex = key->mappingF( key, inputPtr->param.newglyph.characterCode );
#else /* PCLETTO handling not included */
			glyphIndex = key->mappingF( key, inputPtr->param.newglyph.characterCode );
#endif
#endif
		outputPtr->glyphIndex = glyphIndex;
		key->glyphIndex = glyphIndex;
		outputPtr->numberOfBytesTaken = key->numberOfBytesTaken;
	} else {
#if INCL_VERTICAL
		if (vertical)
			key->glyphIndex = outputPtr->glyphIndex = fs_VerticalSubstitution(key, inputPtr->param.newglyph.glyphIndex);
		else
#endif
			key->glyphIndex = outputPtr->glyphIndex =
				inputPtr->param.newglyph.glyphIndex;
		outputPtr->numberOfBytesTaken = 0;
	}

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	if(key->glyphIndex >= SWAPW(key->maxProfile.numGlyphs))
		return INVALID_GLYPH_INDEX;

#ifdef WANT_CTRL_SPACE_FOR_UNKNOWN_CHARS
/* LJ4 printers display the box character for unknown chars */
#ifdef PCLETTO
	if (key->glyphIndex == 0)
		return (UNDEFINED_GLYPH);
#else
#if INCL_PCLETTO
	if ((key->glyphIndex == 0) && inputPtr->isPCLETTO)
		return (UNDEFINED_GLYPH);
#endif
#endif
#endif /* WANT_CTRL_SPACE_FOR_UNKNOWN_CHARS */
	return NO_ERR;
}	/* end fs_NewGlyph() */


/*
 * this call is optional.
 *
 * can be called right after fs_NewGlyph()
 */

FUNCTION int32 fs_GetAdvanceWidth(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	int error;
	register fsg_SplineKey*	key =
		fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS, &error);

	if (key == 0)
		return error;
	SETJUMP(key, error);

	sfnt_ReadSFNTMetrics( key, key->glyphIndex );
	
	outputPtr->metricInfo.advanceWidth.x =
		ShortMulDiv( key->metricScalar, (int32)key->nonScaledAW, (int32)key->emResolution );
	outputPtr->metricInfo.advanceWidth.y = 0;

	if (!key->identityTransformation)
		fsg_FixXYMul(	&outputPtr->metricInfo.advanceWidth.x,
						&outputPtr->metricInfo.advanceWidth.y,
						&key->currentTMatrix );

	return NO_ERR;
}	/* end fs_GetAdvanceWidth() */


/*
 * This function is called from the Bitstream interface routine,
 *   tt_get_char_width
 * The set width of the character index is returned in units of 1/65536 em.
 * Not to be confused with fs_GetAdvanceWidth, which uses the transformation
 *   matrix to return the escapement vector in pixel units.
 */

FUNCTION int32 fs_GetCharWidth(fs_GlyphInputType *inputPtr, ufix16 glyph_index, ufix16 *width)
{
    register fsg_SplineKey  *key;
    int error;

    key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT, &error);
    if (!key) return error;
    SETJUMP(key, error);

    sfnt_ReadSFNTMetrics( key, glyph_index );
    *width = key->nonScaledAW;
    return NO_ERR;
}


/*
 * This function is called from the Bitstream interface module.
 * It returns 1 if the TrueType transformation matrix (reduced form) is the
 * identity matrix; otherwise returns 0.
 */

FUNCTION int32 fs_IdentityTransform(fs_GlyphInputType *inputPtr, int *iden)
{
    register fsg_SplineKey  *key;
    int error;

    key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS, &error);
    if (!key) return error;
    SETJUMP(key, error);

    if (key->identityTransformation)
        *iden = 1;
    else
        *iden = 0;

    return NO_ERR;
}


/*
 * This function is called from the Bitstream interface module.
 * A (x0, y0) is put through the TrueType transformation and (x1, y1)
 * is returned.
 */

FUNCTION int32 fs_TransformPoint(fs_GlyphInputType *inputPtr, int16 x0, int16 y0, Fixed *x1, Fixed *y1)
{
    register fsg_SplineKey  *key;
    int error;

    key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS, &error);
    if (!key) return error;
    SETJUMP(key, error);

    *x1 = ShortMulDiv( key->metricScalar, (int32)x0, (int32)key->emResolution );
    *y1 = ShortMulDiv( key->metricScalar, (int32)y0, (int32)key->emResolution );

    if ( ! key->identityTransformation )
        fsg_FixXYMul( x1, y1, &key->currentTMatrix );

    return NO_ERR;
}


/*
 * This function is called from the Bitstream interface routine, tt_load_font
 * The Bitstream output modules need to have the fontwide bounding box and
 * the fontspace resolution (oruperem).
 */

FUNCTION int32 fs_sfntBBox(fs_GlyphInputType *inputPtr, int16 *font_xmin, int16 *font_ymin,
						   int16 *font_xmax, int16 *font_ymax, uint16 *oruperem)
{
    register fsg_SplineKey  *key;
    int error;
    int16 advanceWidth;
    sfnt_FontHeader* fontHead;
    sfnt_HorizontalHeader* horiHead;

    key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT, &error);
    if (!key) return error;
    SETJUMP(key, error);

    fontHead = (sfnt_FontHeader*)sfnt_GetTablePtr( key, sfnt_fontHeader, true );
    *font_xmin = SWAPW(fontHead->xMin);
    *font_ymin = SWAPW(fontHead->yMin);
    *font_xmax = SWAPW(fontHead->xMax);
    *font_ymax = SWAPW(fontHead->yMax);
    *oruperem  = SWAPW(fontHead->unitsPerEm);
    RELEASESFNTFRAG(key, fontHead);

    horiHead = (sfnt_HorizontalHeader*)sfnt_GetTablePtr( key, sfnt_horiHeader, true );
    advanceWidth = (int16)SWAPW(horiHead->advanceWidthMax);
    if (advanceWidth > *font_xmax)
        *font_xmax = advanceWidth;
    RELEASESFNTFRAG(key, horiHead);

    return NO_ERR;
}



FUNCTION LOCAL_PROTO int32 fs__Contour(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr, btsbool useHints)
{
	register fsg_SplineKey 		*key;
	int error;

	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS, &error)))
		return error;
	SETJUMP(key, error);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	/* potentially do delayed pre program execution */
	if ( key->executePrePgm ) {
		/* Run the pre program and scale the control value table */
		if ( error = fsg_NewTransformation( key, 0L ) )
			return error;
	}
	
	key->outputPtr 		= outputPtr;
	key->styleFunc		= inputPtr->param.gridfit.styleFunc;
#if INCL_PCLETTO
	if (error = fsg_GridFit( key, inputPtr->param.gridfit.traceFunc, useHints, inputPtr->isPCLETTO)) 	/* THE CALL */
#else
	if (error = fsg_GridFit( key, inputPtr->param.gridfit.traceFunc, useHints)) 	/* THE CALL */
#endif
		return error;

	{
		int8* workSpacePtr			= (int8 *)key->memoryBases[WORK_SPACE_BASE];
		fsg_OffsetInfo* offsetPtr	= &(key->elementInfoRec.offsets[1]);

		outputPtr->xPtr		= (int32 *)(workSpacePtr + offsetPtr->newXOffset);
		outputPtr->yPtr		= (int32 *)(workSpacePtr + offsetPtr->newYOffset);
		outputPtr->startPtr	= (int16 *)(workSpacePtr + offsetPtr->startPointOffset);
		outputPtr->endPtr	= (int16 *)(workSpacePtr + offsetPtr->endPointOffset);
		outputPtr->onCurve	= (uint8 *)(workSpacePtr + offsetPtr->onCurveOffset);
	}
	outputPtr->numberOfContours	= key->elementInfoRec.interpreterElements[GLYPHELEMENT].nc;
	outputPtr->scaledCVT = ((F26Dot6 *)key->memoryBases[PRIVATE_FONT_SPACE_BASE] + key->offset_controlValues);
	
	outputPtr->outlinesExist = (uint16)(key->glyphLength != 0);

	key->state = INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH;

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	return NO_ERR;
}



FUNCTION int32 fs_ContourNoGridFit(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	return fs__Contour( inputPtr, outputPtr, false );
}



FUNCTION int32 fs_ContourGridFit(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	return fs__Contour( inputPtr, outputPtr, true );
}

#if INCL_APPLESCAN

FUNCTION int32 fs_ContourScan(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{

	register fsg_SplineKey 		*key;
	register sc_BitMapData		*bitRecPtr;
	register fnt_ElementType 	*elementPtr;
	register int8				*workSpacePtr;
	register fsg_OffsetInfo		*offsetPtr;
	sc_GlobalData				*scPtr;
	sc_CharDataType				charData;
	int32 						scanControl;
	int16						lowBand, highBand;
	uint16 						nx, ny;
	int							error;

	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN, &error)))
		return error;
	SETJUMP(key, error);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	bitRecPtr = &key->bitMapInfo;

#ifdef USE_OUTLINE_CACHE
	if ( !key->outlineIsCached )
#endif
	{
		elementPtr			= &(key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
		workSpacePtr		= (int8 *)key->memoryBases[WORK_SPACE_BASE];
		offsetPtr			= &(key->elementInfoRec.offsets[GLYPHELEMENT]);

		
		charData.x			= (int32 *)(workSpacePtr + offsetPtr->newXOffset);
		charData.y			= (int32 *)(workSpacePtr + offsetPtr->newYOffset);
		charData.ctrs		= elementPtr->nc;
		charData.sp			= (int16 *)(workSpacePtr + offsetPtr->startPointOffset);
		charData.ep			= (int16 *)(workSpacePtr + offsetPtr->endPointOffset);
		charData.onC		= (int8 *)(workSpacePtr + offsetPtr->onCurveOffset);
	}
#ifdef USE_OUTLINE_CACHE
	else
	{
		register int32 *src = inputPtr->param.scan.outlineCache;
		register int32 numPoints;
		
		if ( *src != OUTLINESTAMP ) return TRASHED_OUTLINE_CACHE;	
		src	+= 4;		/* skip over stamp and 3 bitmap memory areas  */
		
		bitRecPtr->wide = *src++;
		bitRecPtr->high = *src++;
		bitRecPtr->xMin = *src++;
		bitRecPtr->yMin = *src++;
		bitRecPtr->xMax = *src++;
		bitRecPtr->yMax = *src++;
		bitRecPtr->nXchanges =  *src++;
		bitRecPtr->nYchanges =  *src++;
		key->scanControl = 		*src++;
		key->imageState  = 		*src++;
		
		
		{ /* some sanity checking */
			if (! ( bitRecPtr->xMin >= -32768 && bitRecPtr->xMax <= 32767 && bitRecPtr->yMin >= -32768 && bitRecPtr->yMax <= 32767 )) {
				return POINT_MIGRATION_ERR;
			}
		}

		{
			int16* wordptr = (int16*)src;
			/* # of contours */
			charData.ctrs = *wordptr++;
			
			/* start points */
			charData.sp = wordptr;
			wordptr += charData.ctrs;
			
			/* end points */
			charData.ep = wordptr;
			wordptr += charData.ctrs;
		
			src = (int32*)wordptr;
		}
		numPoints = charData.ep[charData.ctrs-1] + 1;
		
		/* x coordinates */
		charData.x = src;
		src += numPoints;
		
		/* y coordinates */
		charData.y = src;
		src += numPoints;
		
		{
			int8* byteptr = (int8*)src;
			/* on curve flags */
			charData.onC = byteptr;
			byteptr += numPoints;
			if ( *byteptr != (int8)OUTLINESTAMP2 ) {
				return TRASHED_OUTLINE_CACHE;	
			}
		}
	}
#endif		/* use outline cache */

		scPtr				= (sc_GlobalData *)key->memoryBases[SCAN_PTR_BASE];
		
		nx 					= bitRecPtr->xMax - bitRecPtr->xMin;
		if( nx == 0 ) ++nx;
		
		scanControl = fs_dropOutVal( key );
		/* set up banding.  Assume highBand is 1 higher than highest scanrow <10>  */		
		highBand = inputPtr->param.scan.topClip;	
		lowBand  = inputPtr->param.scan.bottomClip;
		
		/* 	If topclip < bottom clip there is no banding by convention  */
		if( highBand <= lowBand )
		{
			highBand = bitRecPtr->yMax;
			lowBand = bitRecPtr->yMin;
		}
		/* check for out of bounds band request  							<10> */
		if( highBand > bitRecPtr->yMax ) highBand = bitRecPtr->yMax;
		if( lowBand  < bitRecPtr->yMin )  lowBand = bitRecPtr->yMin;
		
/* 11/16/90 rwb - We now allow the client to turn off DOControl by returning a NIL pointer
to the memory area used by DOC.  This is done so that in low memory conditions, the
client can get enough memory to print something.  We also always turn off DOC if the client
has requested banding.  Both of these conditions may change in the future.  Some versions
of TT may simply return an error condition when the NIL pointer to memoryarea 7 is
provided.  We also need to rewrite the scan converter routines that fill the bitmap 
under dropout conditions so that they use noncontiguous memory for the border scanlines
that need to be present for doing DOC.  This will allow us to do DOC even though we are
banding, providing there is enough memory.  By preflighting the fonts so that the request
for memory for areas 6 and 7 from findBitMapSize is based on actual need rather than
worse case analysis, we may also be able to reduce the memory required to do DOC in all
cases and particulary during banding.
*/
		/* inhibit DOControl if banding 									<10> */
		if( highBand < bitRecPtr->yMax || lowBand > bitRecPtr->yMin ) scanControl = 0;

		/* Allow client to turn off DOControl                               <10> */
		if( key->memoryBases[BITMAP_PTR_3] == 0 ) scanControl = 0;
		
		bitRecPtr->bitMap	= (uint32 *)key->memoryBases[BITMAP_PTR_1];		

		if( scanControl )					
		{
			char* memPtr		= (char*)key->memoryBases[BITMAP_PTR_3];
			bitRecPtr->xLines 	= (int16*)memPtr;
			
			memPtr				+= (bitRecPtr->nXchanges+2) * nx * sizeof(int16);
			bitRecPtr->xBase 	= (int16**)memPtr;
			
			ny					= bitRecPtr->yMax - bitRecPtr->yMin;
			if( ny == 0 ) ++ny;
			
			memPtr				= (char *)key->memoryBases[BITMAP_PTR_2];
			bitRecPtr->yLines 	= (int16*)memPtr;
			
			memPtr 				+= (bitRecPtr->nYchanges+2) * ny * sizeof(int16);
			bitRecPtr->yBase 	= (int16**)memPtr;
		}
		else
		{
			char*	memPtr;
			ny					= highBand - lowBand;
			if( ny == 0 ) ++ny;
			memPtr				= (char *)key->memoryBases[BITMAP_PTR_2];
			bitRecPtr->yLines 	= (int16*)memPtr;
			
			memPtr 				+= (bitRecPtr->nYchanges+2) * ny * sizeof(int16);
			bitRecPtr->yBase 	= (int16**)memPtr;
		}
	if (error = sc_ScanChar( &charData, scPtr, bitRecPtr, lowBand, highBand, scanControl ))
		return error;
	{
		register BitMap* bm = &outputPtr->bitMapInfo;
		bm->baseAddr		= (int8 *)bitRecPtr->bitMap;
		bm->rowBytes		= bitRecPtr->wide >> 3;
		bm->bounds.top		= bitRecPtr->yMin;
		bm->bounds.left		= bitRecPtr->xMin;
		bm->bounds.bottom	= bitRecPtr->yMin + ny;
		bm->bounds.right	= bitRecPtr->xMin + nx;
	}
	
	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	return NO_ERR;
}

#endif

#if INCL_APPLESCAN

FUNCTION int32 fs_FindBitMapSize(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register fsg_SplineKey 		*key;
	register fnt_ElementType 	*elementPtr;
	register int8				*workSpacePtr;
	register fsg_OffsetInfo		*offsetPtr;
	sc_CharDataType				charData;
	register sc_BitMapData		*bitRecPtr;
	uint16						scan, byteWidth;
	int32 						numPts;
	register int32 				tmp32;
	int32						nx, size;
	int error;

	key = fs_SetUpKey(	inputPtr,
						(INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH),
						&error);
	if (!key)
		return error;

	SETJUMP(key, error);

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	key->outlineIsCached = false;

	elementPtr		= &(key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
	workSpacePtr	= (int8 *)key->memoryBases[WORK_SPACE_BASE];
	offsetPtr		= &(key->elementInfoRec.offsets[GLYPHELEMENT]);
	bitRecPtr		= &(key->bitMapInfo);

	charData.x		= (int32 *)(workSpacePtr + offsetPtr->newXOffset);
	charData.y		= (int32 *)(workSpacePtr + offsetPtr->newYOffset);
	charData.ctrs	= elementPtr->nc;
	charData.sp		= (int16 *)(workSpacePtr + offsetPtr->startPointOffset);
	charData.ep		= (int16 *)(workSpacePtr + offsetPtr->endPointOffset);
	charData.onC	= (int8 *)(workSpacePtr + offsetPtr->onCurveOffset);

	if( error = sc_FindExtrema( &charData,  bitRecPtr ) )
			return error;
	
	scan					= bitRecPtr->high;
	byteWidth				= bitRecPtr->wide >> 3;
	{
		BitMap* bm = &outputPtr->bitMapInfo;
		bm->baseAddr		= 0;
		bm->rowBytes		= byteWidth;
		{
			Rect* r = &bm->bounds;
			r->top		= bitRecPtr->yMin;
			r->left		= bitRecPtr->xMin;
			r->bottom	= bitRecPtr->yMax;
			r->right	= bitRecPtr->xMax;
		}
	}

	numPts = charData.ep[charData.ctrs-1] + 1 + PHANTOMCOUNT;
	{
		register int32 ten = 10;
		register metricsType* metric = &outputPtr->metricInfo;
		register int32 index1 = numPts-PHANTOMCOUNT+RIGHTSIDEBEARING;
		register int32 index2 = numPts-PHANTOMCOUNT+LEFTSIDEBEARING;

		metric->devAdvanceWidth.x  = (charData.x[index1] - charData.x[index2])
										<< ten;
		metric->devAdvanceWidth.y  = (charData.y[index1] - charData.y[index2])
										<< ten;
		index1 = numPts-PHANTOMCOUNT+LEFTEDGEPOINT;
		metric->devLeftSideBearing.x  = (charData.x[index1] - charData.x[index2])  << ten;
		metric->devLeftSideBearing.y  = (charData.y[index1] - charData.y[index2])  << ten;

		metric->advanceWidth.x 	  = ShortMulDiv( key->metricScalar, (int32)key->nonScaledAW, (int32)key->emResolution );
		metric->advanceWidth.y 	  = 0;
		if ( ! key->identityTransformation )
			fsg_FixXYMul( &metric->advanceWidth.x, &metric->advanceWidth.y, &key->currentTMatrix );

		index2 = numPts-PHANTOMCOUNT+ORIGINPOINT;
		metric->leftSideBearing.x = (charData.x[index1] - charData.x[index2])  << ten;
		metric->leftSideBearing.y = (charData.y[index1] - charData.y[index2])  << ten;
	
		/* store away sidebearing along the advance width vector */
		metric->leftSideBearingLine = metric->leftSideBearing;
		metric->devLeftSideBearingLine = metric->devLeftSideBearing;
	
		/* Add vector to left upper edge of bitmap for ease of positioning by client */
		tmp32 = ((int32)(bitRecPtr->xMin) << 16) - (charData.x[index1] << ten);
		metric->leftSideBearing.x		+= tmp32;
		metric->devLeftSideBearing.x	+= tmp32;
		tmp32 = ((int32)(bitRecPtr->yMax) << 16) - (charData.y[index1] << ten);
		metric->leftSideBearing.y		+= tmp32;		
		metric->devLeftSideBearing.y	+= tmp32;		
	}

	key->state = INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN;

	/* get memory for bitmap in bitMapRecord */
	if( scan == 0 ) ++scan;

	outputPtr->memorySizes[BITMAP_PTR_1] = SHORTMUL(scan, byteWidth) + STAMPEXTRA;
	
	/* get memory for yLines & yBase in bitMapRecord */
	size = (int32)scan * ((bitRecPtr->nYchanges + 2) * sizeof(int16) + sizeof(int16*)); 
	outputPtr->memorySizes[BITMAP_PTR_2] = size;

	if( fs_dropOutVal( key ) )
	{
		/* get memory for xLines and xBase - used only for dropout control */			
		nx 		= bitRecPtr->xMax - bitRecPtr->xMin;
		if( nx == 0 ) ++nx;
		size 	= nx * ((bitRecPtr->nXchanges+2) * sizeof(int16) + sizeof(int16*));
	}
	else size = 0;
	
	outputPtr->memorySizes[BITMAP_PTR_3] = size;

	CHECKSTAMP(inputPtr->memoryBases[0] + outputPtr->memorySizes[0]);
	CHECKSTAMP(inputPtr->memoryBases[1] + outputPtr->memorySizes[1]);
	CHECKSTAMP(inputPtr->memoryBases[2] + outputPtr->memorySizes[2]);
	CHECKSTAMP(inputPtr->memoryBases[3] + outputPtr->memorySizes[3]);
	CHECKSTAMP(inputPtr->memoryBases[4] + outputPtr->memorySizes[4]);

	return NO_ERR;
}
#endif /* INCL_APPLESCAN */


#ifdef USE_OUTLINE_CACHE

FUNCTION int32 fs_SizeOfOutlines(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register fsg_SplineKey 		*key;
	register fnt_ElementType 	*elementPtr;
	register int8				*workSpacePtr;
	register fsg_OffsetInfo		*offsetPtr;
	sc_CharDataType				charData;
	int32 size;
	int32 numPoints;
	int error;
	
	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN, &error)))
		return error;
	SETJUMP(key, error);

	elementPtr			= &(key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
	workSpacePtr		= (int8 *)key->memoryBases[WORK_SPACE_BASE];
	offsetPtr			= &(key->elementInfoRec.offsets[GLYPHELEMENT]);

	charData.ctrs		= elementPtr->nc;
	/* charData.sp			= (int16 *)(workSpacePtr + offsetPtr->startPointOffset); not needed */
	charData.ep			= (int16 *)(workSpacePtr + offsetPtr->endPointOffset);

	if ( charData.ctrs > 0 )
	{
		numPoints = charData.ep[charData.ctrs-1] + 1;
					/* $$$$$ Richard is going to clean up these constants that Mr. Sampo created!!!!! */
		size = (1 + 2 *  charData.ctrs) * sizeof(int16)  +  (8 + 2 *  numPoints) * sizeof(int32)
			 + (numPoints+1) * sizeof(int8) + 6 * sizeof(int32); /* <4> */
		size += 3;
		size &= ~3;
	}
	else
		size = 0;

	outputPtr->outlineCacheSize = size;
	return NO_ERR;
}



FUNCTION int32 fs_SaveOutlines(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register int32 i;
	register int8 *src;
	register int32 numPoints;
	register int8 *dest;

	register fsg_SplineKey 		*key;
	register sc_BitMapData		*bitRecPtr;
	register fnt_ElementType 	*elementPtr;
	register int8				*workSpacePtr;
	register fsg_OffsetInfo		*offsetPtr;
	sc_CharDataType				charData;
	int error;

	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN, &error)))
		return error;
	SETJUMP(key, error);

	bitRecPtr			= &key->bitMapInfo;

	elementPtr			= &(key->elementInfoRec.interpreterElements[GLYPHELEMENT]);
	workSpacePtr		= (int8 *)key->memoryBases[WORK_SPACE_BASE];
	offsetPtr			= &(key->elementInfoRec.offsets[GLYPHELEMENT]);
	charData.x			= (int32 *)(workSpacePtr + offsetPtr->newXOffset);
	charData.y			= (int32 *)(workSpacePtr + offsetPtr->newYOffset);
	charData.ctrs		= elementPtr->nc;
	charData.sp			= (int16 *)(workSpacePtr + offsetPtr->startPointOffset);
	charData.ep			= (int16 *)(workSpacePtr + offsetPtr->endPointOffset);
	charData.onC		= (int8 *)(workSpacePtr + offsetPtr->onCurveOffset);
	
	if ( charData.ctrs <= 0 ) return NO_ERR;

	numPoints = charData.ep[charData.ctrs-1] + 1;
	
	dest = (int8 *)inputPtr->param.scan.outlineCache;
	*((int32 *)dest) = OUTLINESTAMP;	dest += sizeof( int32 );


	*((int32 *)dest) = outputPtr->memorySizes[BITMAP_PTR_1];	dest += sizeof( int32 );
	*((int32 *)dest) = outputPtr->memorySizes[BITMAP_PTR_2];	dest += sizeof( int32 );
	*((int32 *)dest) = outputPtr->memorySizes[BITMAP_PTR_3];	dest += sizeof( int32 );

	/* bounding box */
	*((int32 *)dest) = bitRecPtr->wide;	dest += sizeof( int32 );  /*<4>*/
	*((int32 *)dest) = bitRecPtr->high;	dest += sizeof( int32 ); 
	*((int32 *)dest) = bitRecPtr->xMin;	dest += sizeof( int32 );
	*((int32 *)dest) = bitRecPtr->yMin;	dest += sizeof( int32 );
	*((int32 *)dest) = bitRecPtr->xMax;	dest += sizeof( int32 );
	*((int32 *)dest) = bitRecPtr->yMax;	dest += sizeof( int32 );
	*((int32 *)dest) = bitRecPtr->nXchanges;	dest += sizeof( int32 );
	*((int32 *)dest) = bitRecPtr->nYchanges;	dest += sizeof( int32 );
	*((int32 *)dest) = key->scanControl;	dest += sizeof( int32 );
	*((int32 *)dest) = key->imageState;	dest += sizeof( int32 );

	/*** save charData ***/
	/* # of contours */
	*((int16 *)dest) = charData.ctrs;	dest += sizeof( int16 );
	
	/* start points */
	src = (int8 *)charData.sp;
	for ( i = charData.ctrs; --i >= 0; ) {
		*((int16 *)dest) = *((int16 *)src);
		dest += sizeof( int16 );
		src += sizeof( int16 );
	}
	
	
	/* end points */
	src = (int8 *)charData.ep;
	for ( i = charData.ctrs; --i >= 0; ) {
		*((int16 *)dest) = *((int16 *)src);
		dest += sizeof( int16 );
		src += sizeof( int16 );
	}
	
	/* x coordinates */
	src = (int8 *)charData.x;
	for ( i = numPoints; --i >= 0; ) {
		*((int32 *)dest) = *((int32 *)src);
		dest += sizeof( int32 );
		src += sizeof( int32 );
	}
	
	/* y coordinates */
	src = (int8 *)charData.y;
	for ( i = numPoints; --i >= 0; ) {
		*((int32 *)dest) = *((int32 *)src);
		dest += sizeof( int32 );
		src += sizeof( int32 );
	}
	
	/* on curve flags */
	src = (int8 *)charData.onC;
	for ( i = numPoints; --i >= 0; ) {
		*dest++ = *src++;
	}
	*dest++ = OUTLINESTAMP2;
	key->state = (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN);
	
	return NO_ERR;
}


/* rwb - 4/21/90 - This procedure restores only enough information so that ContourScan
can continue the restoration.
*/

FUNCTION int32 fs_RestoreOutlines(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	register fsg_SplineKey 		*key;
	int32 *src;
	int error;

	if (!(key = fs_SetUpKey(inputPtr, INITIALIZED, &error)))
		return error;
	SETJUMP(key, error);

	src = inputPtr->param.scan.outlineCache;

	if ( *src++ != OUTLINESTAMP ) return TRASHED_OUTLINE_CACHE;	
	
	outputPtr->memorySizes[BITMAP_PTR_1] = *src++;
	outputPtr->memorySizes[BITMAP_PTR_2] = *src++;
	outputPtr->memorySizes[BITMAP_PTR_3] = *src++;

	key->state = (INITIALIZED | NEWSFNT | NEWTRANS | GOTGLYPH | SIZEKNOWN);
	key->outlineIsCached = true;

	return NO_ERR;
}

#endif	/* use outline cache */



FUNCTION int32 fs_CloseFonts(fs_GlyphInputType *inputPtr, fs_GlyphInfoType *outputPtr)
{
	if ( outputPtr )
		if ( inputPtr )
			return NO_ERR;
		else
			return NULL_INPUT_PTR_ERR;
	else
		return NULL_OUTPUT_PTR_ERR;
	/* return NULL_INPUT_PTR_ERR; (this code is never reached) */
}

#if INCL_APPLESCAN

FUNCTION LOCAL_PROTO int32 fs_dropOutVal(fsg_SplineKey *key)
{
register int32 condition = key->scanControl;
if( !(condition & 0x3F00) )
	return 0;
if( (condition & 0xFFFF0000) == NODOCONTROL )
	return 0;
{
	register int32 imageState = key->imageState;
	if( (condition & 0x800) && ((imageState & 0xFF) > (condition & 0xFF)) )
		return 0;
	if( (condition & 0x1000) && !(imageState & ROTATED) )
		return 0;
	if( (condition & 0x2000) && !(imageState & STRETCHED) )
		return 0;
	if( (condition & 0x100) && ((imageState & 0xFF) <= (condition & 0xFF) ))
		return condition;
	if( (condition & 0x100) && ((condition & 0xFF) == 0xFF ))
		return condition;
	if( (condition & 0x200) && (imageState & ROTATED) )
		return condition;
	if( (condition & 0x400) && (imageState & STRETCHED) )
		return condition;
	return 0;
}
}
#endif /* INCL_APPLESCAN */

#if INCL_VERTICAL
/* cutoff number of units for linear/binary search decision: */
#define maxLinear	8
/* useful macro for unrolled binary search iterations: */
#define BinaryIteration \
	newP = (uint16 *) ((int8 *)lookupSingleP + (searchRange >>= 1)); \
	if (glyphIndex > *newP) lookupSingleP = newP;

/*
 *	This function is the Bitstream interface to vertical character
 *	substitution table. Tested on Microsoft files using mort table
 *	originally designed for Upper & Lower case. However, that is
 *	what is out there. If on the Macintosh, things will be different.
 *	Lessons learned from ComputeIndex4() in mapstrng.c, also notes
 *	in the unpublished Addison-Wesley "The TrueType Book", chapter 6-258.
 *	Hardly a complete interface to 'mort' tables, but should do the job
 *	for eastern languages requiring vertical character substitution.
*/

FUNCTION LOCAL_PROTO uint16 fs_VerticalSubstitution(fsg_SplineKey *key, ufix16 glyphIndex)
{
/* VAR */
sfnt_MortHeader *mortHdrPtr;
sfnt_MortChain  *chainPtr;
uint16 *subTablePtr;
btsbool found, found_feature, found_feature3, found_feature4;
register int ii, jj;
uint32 nChains;
uint16 format, coverage, nFeatureEntries, nSubTables, subTableLength;
uint16 unitSize, nUnits, searchRange, entrySelector, rangeShift;
uint16 featureType, featureSetting;
uint32 enableFlags, enableFlags3, enableFlags4, subFeatureFlags;
uint16 *tableP, *fTableP, *lookupSingleP, *newP;
	/* code begins: */
	if (key->offsetTableMap[sfnt_glyphMetamorphosis] != -1)
	{
		mortHdrPtr = (sfnt_MortHeader *)sfnt_GetTablePtr (key, sfnt_glyphMetamorphosis, true);
		chainPtr = (sfnt_MortChain *) ((int8 *)mortHdrPtr + sizeof(Fixed) + sizeof(uint32)/*sizeof(sfnt_MortHeader)*/ );
		found = FALSE;
		nChains = SWAPL(mortHdrPtr->nChains);
		for (ii = 0; !found && (ii < nChains); ii++)
		{
			tableP = (uint16 *)((int8 *)chainPtr + 2*sizeof(uint32));	/* @nFeatureEntries */
			nFeatureEntries = SWAPWINC(tableP);
			nSubTables = SWAPWINC(tableP);
			found_feature = found_feature3 = found_feature4 = FALSE;
			fTableP = tableP;			/* set feature table pointer */
			for (jj = 0; !found_feature4 && (jj < nFeatureEntries); jj++)
			{/***** go through feature array looking for:
				featureType 	== 3 OR featureType == 4
				featureSetting 	== 0 
				Notes:	
				featureType == 4 causes loop termination. If find featureType == 3,
				keep looking, might find preferred featureType == 4
				*****/
				featureType = SWAPW(*fTableP);
				featureSetting = SWAPW(*(fTableP+1));
				if ((featureType == 3) && (featureSetting == 0) )		
					{
					found_feature = found_feature3 = TRUE;
					enableFlags3 = SWAPL(*((uint32 *)(fTableP+2)));
					}
				if ((featureType == 4) && (featureSetting == 0) )		
					{
					found_feature = found_feature4 = TRUE;
					enableFlags4 = SWAPL(*((uint32 *)(fTableP+2)));
					}
				/* move along, little doggies... */
				fTableP = (ufix16 *) ((int8 *)fTableP + (2*sizeof(uint16) + 2*sizeof(uint32)));
			}
			tableP = subTablePtr = (ufix16 *) ((int8 *)tableP + (2*sizeof(uint16) + 2*sizeof(uint32)) * nFeatureEntries);
			if (found_feature4)
				{
				enableFlags = enableFlags4;	/* featureType == 4 is preferred over 3 */
				found_feature3 = FALSE;
				}
			else
				enableFlags = enableFlags3;
			for (jj = 0; found_feature && (jj < nSubTables); jj++)
			{
				subTableLength = SWAPWINC(tableP);
				coverage = SWAPWINC(tableP);
				subFeatureFlags = SWAPL(*((uint32 *)tableP));
				tableP += 2;
				if ( ( (found_feature3 && ((coverage & 0x8004) == 0x8004)) || (found_feature4 && ((coverage & 0x0004) == 0x0004)) )
						&& (subFeatureFlags & enableFlags) )
				{/* sub-table 3 to be applied only to horizontal text OR sub-table 4,
						non-contexutal substitution (bit 3 of coverage) */
				/* also enableFlags and subFeatureFlags match */
					format = SWAPWINC(tableP);
					unitSize = SWAPWINC(tableP);
					if ( (format == 6) && (unitSize == 4) )
					{/* table format is 6, simple lookup of uint16 pairs (unitSize == 4) */
						/* start searching this table: get binary search parameters: */
						found = TRUE;	/* whether table is empty or not, we found it! */
						nUnits = SWAPWINC(tableP);
						/* binary search parameters start at tableP, followed by uint16 pairs  */
						if (nUnits)
						{/* something to search */
							lookupSingleP = tableP + 3;	/* past the binary search parameters */
							/* Check glyphIndex in range of elements in the table? */
							if ( (glyphIndex < SWAPW(*lookupSingleP)) ||
									(glyphIndex > SWAPW(*(lookupSingleP+2*nUnits-2))) )
								return(glyphIndex);		/* bye, it's not in there! */

							if (nUnits > maxLinear)
							{/* unrolled binary search only if it seems efficient: */
								/* get the other binary search parameters: */
								searchRange = SWAPWINC(tableP);
								entrySelector = SWAPWINC(tableP);
								rangeShift = SWAPWINC(tableP);
								if (glyphIndex >= SWAPW(*((uint16 *)((int8 *)lookupSingleP+rangeShift))))
									lookupSingleP = (uint16 *)((int8 *)lookupSingleP + rangeShift);
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
							/* now linear search: */
							while (glyphIndex > SWAPW(*lookupSingleP))
								lookupSingleP += 2;
							if (glyphIndex == SWAPW(*lookupSingleP))
							{/* pow! We have a match */
								lookupSingleP++;	/* point to 2nd glyphIndex in the pair */
								glyphIndex = SWAPW(*lookupSingleP);
							}
						}/* eo if (nUnits) */
					}/* eo if table type and unit size are what we want */
				}/* eo if coverage indicates vertical non-contextual */
				if (!found)
				{/* right table not found yet, walk to next sub-table: */
					tableP = subTablePtr = 
						(uint16 *)((int8 *)subTablePtr + subTableLength);
				}
			}/* end of subTable loop */
			if (!found) /* right chain still not found, walk to next chain: */
				chainPtr += SWAPL(chainPtr->mortChainHeader.chainLength);
		}/* end of chains loop */
		RELEASESFNTFRAG(key, mortHdrPtr);
	}
	return (glyphIndex);	/* return new/unchanged glyphIndex */
}/* eo function fs_VerticalSubstitution() */
#endif /* INCL_VERTICAL */

#endif /* PROC_TRUETYPE */

