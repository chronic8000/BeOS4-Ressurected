/*****************************************************************************
*                                                                            *
*  Copyright 1992, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/
/***************************** FRONTEND. C ***********************************
 * This is the front end processor for Four-In-One                           *
 * $Header: //bliss/user/archive/rcs/4in1/frontend.c,v 14.0 97/03/17 17:20:10 shawn Release $
 *
 * $Log:	frontend.c,v $
 * Revision 14.0  97/03/17  17:20:10  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.7  97/01/14  17:48:45  shawn
 * Removed unused variables.
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 13.6  96/08/06  17:03:36  shawn
 * Cast argument in call to sp_get_char_id().
 * 
 * Revision 13.5  96/07/02  16:55:51  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 13.4  96/03/21  16:35:04  shawn
 * Cast arguments to resolve compiler warnings.
 * 
 * Revision 13.3  95/11/17  10:27:18  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 13.2  95/06/15  13:30:25  roberte
 * Employed FoutIn1Flags (BIT2) for flagging whether tt_reset()
 * has been called. Must avoid calling it more than once!
 * 
 * Revision 13.1  95/02/23  16:15:17  roberte
 * Removed setting of metric_resolution and orus_per_em here.
 * These set faster in the cspg_SetOutputSpecs() call.
 * 
 * Revision 13.0  95/02/15  16:38:44  roberte
 * Release
 * 
 * Revision 12.2  95/01/11  13:55:54  shawn
 * Define 'save' versions of sp_Globals callbacks using callback typedefs
 * Cast mismatched function assignments using sp_Globals callback typedefs
 * 
 * Revision 12.1  95/01/04  16:46:43  roberte
 * Release
 * 
 * Revision 11.10  95/01/03  12:24:03  roberte
 * Fixed problem with Pcleo processing when reetrant and with TrueDoc.
 * 
 * Revision 11.9  94/12/30  12:50:04  roberte
 * Correct bb_do_nothing() and bb_its_true() for ANSI reentrant
 * parameter macros. Also added GLOBAL_PROTO to each of their prototypes.
 * 
 * Revision 11.8  94/12/28  11:37:56  roberte
 * Assignments of intPtr and strPtr broken up and casted to silence warnings.
 * Argument to se_set_specs() cast to silence warnings.
 * 
 * Revision 11.7  94/12/28  09:16:16  roberte
 * Change bbox code for TrueDoc to new interface, CspGetScaledCharBBox().
 * 
 * Revision 11.6  94/12/21  15:03:17  shawn
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 11.5  94/12/19  17:05:48  roberte
 * Added some sp_report_error() calls for TrueDoc CSP errors.
 * Changed casts of bounding box elements in CSP bbox code.
 * 
 * Revision 11.4  94/12/08  16:38:55  roberte
 * More efficient get_any_char_bbox() re-write.
 * Changed all INCL_TPS tests to INCL_TRUEDOC for consistency.
 * 
 * Revision 11.3  94/11/11  10:32:17  roberte
 * First intergation of TrueDoc - static model working OK.
 * Attention to reentrancy macros, partial testing seems OK.
 * Added reentrancy macros to bb_* functions (get_any_char_bbox())
 * which were not up to reentrancy.
 * 
 * Revision 11.2  94/08/08  11:17:51  roberte
 * Blocked off fi_get_char_bbox() and get_any_char_bbox() within #if INCL_METRICS conditionals.
 * 
 * Revision 11.1  94/07/20  09:03:39  roberte
 * Removed bb_?Min/Max variables used for get_any_char_bbox() function.
 * This functionality now uses the sp_globals.bmap_?min/max members
 * to accumulate bbox extrema. Moved the bb_somCall variable to member
 * of SPEEDO_GLOBALS. This functionality can now support reentrant allocation option.
 * 
 * Revision 11.0  94/05/04  09:41:45  roberte
 * Release
 * 
 * Revision 10.2  94/04/08  14:39:02  roberte
 * Release
 * 
 * Revision 10.0  94/04/08  12:41:16  roberte
 * Release
 * 
 * Revision 9.8  94/04/04  09:51:33  roberte
 * Added entry point fi_get_char_width_orus(). Returns width as stored in the font
 * file, as well as the resolution units of the font file.
 * Corrected direct index logic for Type1 make_char() and get_char_width().
 * Added setting of sp_globals.metric_resolution at set_specs() call.
 * 
 * Revision 9.7  94/03/08  14:28:34  roberte
 * Added handlers for protoDirectIndex when processor is Type1.
 * Covered both make_char and get_char_width. Also, suppress
 * error return in fi_set_specs() for Type1 and protoDirectIndex.
 * New entry points already added to tr_mkchr.c.
 * 
 * Revision 9.6  94/02/22  15:29:41  roberte
 * Added some missing STACKFAR modifiers to char_id param declarations
 * in fi_get_char_bbox() and get_any_char_bbox().
 * 
 * Revision 9.5  94/02/03  16:49:33  roberte
 * Added a cast of char_id to make some compilers happy.
 * 
 * Revision 9.4  93/11/19  14:24:24  roberte
 * Made CanonicalToFileIndex() respect the FourIn1Flags & BIT0: translation is
 * meaningless if DOCH's salted away in RemoteCharData[]. Could fail falsely!
 * Also change for loops using no_chars_avail to now look up FH_NCHRL (# chars
 * in layout). This will allow these to miss the occasional ghost characters
 * at the end of some fonts; they need not be sorted or searched in this list.
 * 
 * Revision 9.3  93/10/01  10:48:41  roberte
 * Removed use of old sp_globals.numChars field, replaced with .no_chars_avail.
 * Added conditional INCL_REMOTE code for new Compressed Resident Font Suite
 * using remote characters.
 * 
 * Revision 9.2  93/08/30  15:58:38  roberte
 * Release
 * 
 * Revision 2.28  93/08/23  12:30:49  roberte
 * Check success before assigning *intPtr in BCIDtoIndex()
 * 
 * Revision 2.27  93/08/17  14:54:50  weili
 * Added CMT_EMPTY testing and error handling
 * 
 * Revision 2.26  93/07/30  15:23:44  roberte
 * BuildSortedSpeedoIDList(), indices stored need to be first_char_idx + i,
 * won't work for SWAs and Kanji the way it was.
 * 
 * Revision 2.25  93/07/08  11:29:53  roberte
 * Declare locals sv_init_out, sv_begin_char and sv_end_char as PFB's instead of PFV's.
 * 
 * Revision 2.24  93/06/24  16:58:58  roberte
 * Added cast to UNKNOWN in if test in the functions DoTranslate().
 * Sparc cc had trouble with this one, Thanks Ted!
 * 
 * Revision 2.23  93/06/15  13:39:04  roberte
 * Added PROC_PCL block around HAVE_MSL2INDEX conditional.
 * 
 * Revision 2.22  93/05/21  10:07:08  roberte
 * Squeezed out some extra lines of code in key functions.
 * 
 * Revision 2.21  93/05/04  11:24:18  roberte
 * Changed some errant PARAMS1&2 macros used in function prototypes to PROTO_DECL1&1s.
 * Also made call to BuildSortedMSLIDList REENTRANT_ALLOC aware.
 * 
 * Revision 2.20  93/04/23  18:02:48  roberte
 *  Added casts in function ptr assigns of the sv_ funct ptrs to boolean function ptrs.
 * 
 * Revision 2.19  93/03/15  13:54:58  roberte
 * Release
 * 
 * Revision 2.14  93/03/01  11:02:44  roberte
 * Added hooks to MsltoIndex() under compile time option HAVE_MSL2INDEX.
 * 
 * Revision 2.13  93/02/23  16:56:31  roberte
 * Added #include of finfotbl.h before #include of ufe.h.
 * 
 * Revision 2.12  93/02/19  12:20:48  roberte
 * Added optimization to speed up BuildSorted list function.  The resident fonts
 * are in sorted order, so optimized on that.  The function will switch itself into
 * more rigorous and slow merge sort mode if it detects an element out of sort order.
 * 
 * Revision 2.11  93/02/08  16:15:18  roberte
 * Added stuff for bounding box functions for TrueType and Type1.
 * Changed locally prototyped functions to use PROTO macro.
 * 
 * Revision 2.10  93/01/29  08:55:19  roberte
 * Added reentrant code macros PARAMS1 and PARAMS2 to support REENTRANT_ALLOC. 
 * 
 * Revision 2.9  93/01/06  13:46:32  roberte
 * Changed references to processor_type, gCharProtocol, gDestProtocol, gMustTranslate, gCurrentSymbolsSet, numChars
 * and gSortedBCIDList.  These are now a part of sp_globals.
 * 
 * Revision 2.8  93/01/04  17:35:23  roberte
 * Changed stray read_word_u to sp_read_word_u and report_error to sp_report_error.
 * 
 * Revision 2.7  93/01/04  17:18:40  laurar
 * put WDECL in front of f_get_char_width.
 * change read_word_u() to sp_read_word_u().
 * 
 * Revision 2.6  93/01/04  16:56:50  roberte
 * Changed all references to new union fields of SPEEDO_GLOBALS to sp_globals.processor.speedo prefix.
 * 
 * Revision 2.5  92/12/29  14:28:53  roberte
 * Some prototypes moved to ufe.h, support for not PROTOS_AVAIL addressed.
 * 
 * Revision 2.4  92/12/15  13:34:53  roberte
 * Changed all prototype function declarations to
 * standard function declarations ala K&R.
 * 
 * Revision 2.3  92/12/09  16:39:32  laurar
 * add STACKFAR to pointers.
 * 
 * Revision 2.2  92/12/02  12:29:16  laurar
 * call report_error instead of sp_report_error;
 * fi_reset initializes the callback structure for the Windows DLL.
 * 
 * Revision 2.1  92/11/24  17:18:44  laurar
 * include fino.h
 * 
 * Revision 2.0  92/11/19  15:39:15  roberte
 * Release
 * 
 * Revision 1.23  92/11/18  18:50:08  laurar
 * Add RESTRICTED_ENVIRON functions again.
 * 
 * Revision 1.22  92/11/17  15:47:29  laurar
 * Add function definitions for RESTRICTED_ENVIRON.
 * 
 * Revision 1.21  92/11/12  16:51:05  roberte
 * Corrected sort indexing problem in BCIDtoIndex for new code.
 * 
 * Revision 1.20  92/11/04  09:22:25  roberte
 * Added support for non BCID sorted speedo files.  Now builds a sorted
 * BSearch list at fi_set_specs() when sp_globals.gCharProtocol != protoDirectIndex.
 * 
 * Revision 1.19  92/11/03  13:54:32  laurar
 * Include type1.h for CHARACTERNAME declaration.
 * 
 * Revision 1.18  92/11/03  12:24:00  roberte
 * Added support for tr_get_char_width() returning a real.
 * Converts it to fix31 units of 65536ths of an em for
 * consistency with other processors.
 * 
 * 
 * Revision 1.17  92/11/02  18:30:33  laurar
 * Add WDECL for Windows CALLBACK function declaration (for the DLL), it is contained in a macro called WDECL;
 * add STACKFAR for parameters that are pointers (also for DLL).
 * 
 * Revision 1.15  92/10/21  10:16:08  roberte
 * Improved logic of direct indexing and error catching for protoDirectIndex
 * for TRUETYPE and TYPE1 processors. Added new error code, 5001, for
 * direct indexing unsupported.  TrueType now able to support both
 * direct indexing and Glyphcode indexing, dependant on compile time
 * option GLYPH_INDEX.
 * 
 * Revision 1.14  92/10/20  17:13:01  roberte
 * Removed calls to CanonicalToFileIndex() if sp_globals.gDestProtocol==protoUnicode.
 * Adjusted CanonicalToFileIndex() to NOT have a case for protoUnicode.
 * Reflects cahnge to 4sample.c, now calls tt_make_char instead tt_make_char_idx.
 * This allows full support for Unicode glyph codes!!!
 * 
 * Revision 1.13  92/10/19  13:03:32  roberte
 * Removed bogus way of getting bitmap width.
 * 
 * Revision 1.12  92/10/16  15:19:35  roberte
 * Added possibly temporary function rf_get_bitmap_width() for
 * support of proofing accuracy.  May need permanent, and better
 * support of getting accurate pixel width.
 * 
 * Revision 1.11  92/10/15  17:35:46  roberte
 * Added support for PROTOS_AVAIL compile time option. Also changed test used
 * before calling CanonicalToFileIndex().  Now tests input protocol NOT
 * one of the direct indexing modes, and destination protocol NOT PSName.
 * 
 * Revision 1.10  92/10/14  13:18:33  roberte
 * Updated all calls to CanonicalToFileIndex() to copy value to translate
 * into a local, so the calls won't change value passed in as parameter.
 * 
 * Revision 1.9  92/09/29  14:11:03  weili
 * moved the preprocessor directives into ufe.h
 * 
 * Revision 1.8  92/09/28  19:12:02  weili
 * separated the font processors with preprocessor directives
 * 
 * Revision 1.7  92/09/26  15:29:14  roberte
 * Added copyright header and RCS marker. Prettied up
 * code and added comments.  Corrected casting to silence
 * compiler warnings.  Moved the setting of sp_globals.numChars
 * to the fi_reset() function, so these won't be repeatedly set
 * on each character translation in BCIDtoIndex().
 * 
 *
 *
 *                                                                           *
 *****************************************************************************/

#include "spdo_prv.h"               /* General definitions for Speedo    */
#include "fino.h"
#include "finfotbl.h"
#include "ufe.h"					/* Unified Front End definitions */

#if PROC_TRUEDOC
#ifdef LATER
#include "csp_int.h" /* also includes csp_api.h */
#else
#include "csp_api.h"
#endif
#endif /* PROC_TRUEDOC */

/***** GLOBAL VARIABLES *****/

/*****  GLOBAL FUNCTIONS *****/
#if	WINDOWS_4IN1
callback_struct callback_ptrs;
#endif

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/

LOCAL_PROTO btsbool DoTranslate(PROTO_DECL2 void STACKFAR *char_id, ufix16 STACKFAR *intPtr, char STACKFAR *strPtr);
LOCAL_PROTO btsbool BCIDtoIndex(PROTO_DECL2 ufix16 STACKFAR *intPtr);
LOCAL_PROTO btsbool CanonicalToFileIndex(PROTO_DECL2 ufix16 STACKFAR *intPtr);
LOCAL_PROTO void BuildSortedSpeedoIDList(PROTO_DECL1);
#ifdef HAVE_MSL2INDEX
GLOBAL_PROTO void BuildSortedMSLIDList (TPS_GPTR1);
#endif

#if RESTRICTED_ENVIRON
GLOBAL_PROTO btsbool get_any_char_bbox(PROTO_DECL2 ufix8 STACKFAR *font_ptr, void STACKFAR *char_id, bbox_t *bbox, eFontProcessor fontType);
#else
GLOBAL_PROTO btsbool get_any_char_bbox(PROTO_DECL2 void STACKFAR *char_id, bbox_t *bbox, eFontProcessor fontType);
#endif

#if PROC_TRUEDOC
GLOBAL_PROTO int cspg_SetOutputSpecs(PCSPCONTEXT specs_t STACKFAR *specsarg);
#endif

/*****************************************************************************
 * fi_reset()
 *   Initialize the processors
 *
 ****************************************************************************/

#if WINDOWS_4IN1
FUNCTION void WDECL fi_reset(TPS_GPTR2 callback_struct STACKFAR *fcn_ptrs, eFontProtocol protocol, eFontProcessor f_type)
#else
FUNCTION void fi_reset(TPS_GPTR2 eFontProtocol protocol, eFontProcessor f_type)
#endif
{

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	sp_globals.processor_type = f_type;
	sp_globals.gCharProtocol = protocol;

#if   WINDOWS_4IN1
   /* initialize pointers to callback functions. */
   callback_ptrs.sp_report_error = fcn_ptrs->sp_report_error;
   callback_ptrs.sp_open_bitmap = fcn_ptrs->sp_open_bitmap;
   callback_ptrs.sp_set_bitmap_bits = fcn_ptrs->sp_set_bitmap_bits;
   callback_ptrs.sp_close_bitmap = fcn_ptrs->sp_close_bitmap;
   callback_ptrs.sp_load_char_data = fcn_ptrs->sp_load_char_data;
   callback_ptrs.get_byte = fcn_ptrs->get_byte;
   callback_ptrs.dynamic_load = fcn_ptrs->dynamic_load;
#endif

	switch (sp_globals.processor_type) {
#if PROC_SPEEDO
	case procSpeedo:
	    sp_reset(PARAMS1); 
		sp_globals.gDestProtocol = protoBCID;
		/* canonical to canonical ? */
		sp_globals.gMustTranslate = 	(
							(sp_globals.gCharProtocol == protoMSL) ||
							(sp_globals.gCharProtocol == protoUnicode) ||
							(sp_globals.gCharProtocol == protoPSName) ||
							(sp_globals.gCharProtocol == protoUser)
							);
		break;  
#endif /* if PROC_SPEEDO */
#if PROC_TRUEDOC
	case procTrueDoc:
		sp_globals.gDestProtocol = protoBCID;
		/* canonical to canonical ? */
		sp_globals.gMustTranslate = 	(sp_globals.gCharProtocol != protoBCID);
		break;  
#endif


#if PROC_PCL
	case procPCL:   /* there is no reset for hpreader */
		sp_globals.gDestProtocol = protoMSL;
		/* canonical to canonical ? */
		sp_globals.gMustTranslate = 	(
							(sp_globals.gCharProtocol == protoBCID) ||
							(sp_globals.gCharProtocol == protoUnicode) ||
							(sp_globals.gCharProtocol == protoPSName) ||
							(sp_globals.gCharProtocol == protoUser)
							);
		break;  
#endif

#if PROC_TYPE1
	case procType1:
	    tr_init(PARAMS1); 
		sp_globals.gDestProtocol = protoPSName;
		/* canonical to canonical ? */
		sp_globals.gMustTranslate = 	(
							(sp_globals.gCharProtocol == protoMSL) ||
							(sp_globals.gCharProtocol == protoUnicode) ||
							(sp_globals.gCharProtocol == protoBCID) ||
							(sp_globals.gCharProtocol == protoUser)
							);
		break;  
#endif

#if PROC_TRUETYPE
	case procTrueType:
		if (!(sp_globals.FourIn1Flags & BIT2))	/* did we call tt_reset() before? */
	    	tt_reset(PARAMS1);	/* nope! */
		sp_globals.FourIn1Flags |= BIT2; /* we'll never call it again! */
		sp_globals.gDestProtocol = protoUnicode;
		/* canonical to canonical ? */
		sp_globals.gMustTranslate = 	(
							(sp_globals.gCharProtocol == protoMSL) ||
							(sp_globals.gCharProtocol == protoBCID) ||
							(sp_globals.gCharProtocol == protoPSName) ||
							(sp_globals.gCharProtocol == protoUser)
							);
		break;  
#endif

	default:
		/*  shouldn't get here unless bad processor type selected */
	   sp_report_error (PARAMS2 5000);
		break;
	}

	return;
}	/* end fi_reset() */


/*****************************************************************************
 * fi_make_char()
 *   Call the character generator for this font type
 *
 ****************************************************************************/

#if RESTRICTED_ENVIRON
FUNCTION btsbool WDECL fi_make_char(TPS_GPTR2 ufix8 STACKFAR *font_ptr, void STACKFAR *char_id)
#else
FUNCTION btsbool WDECL fi_make_char(TPS_GPTR2 void STACKFAR *char_id)
#endif
{
char STACKFAR *strPtr, strBuffer[32];
ufix16 STACKFAR *intPtr, translatedValue;
btsbool return_value = FALSE;	/* assume failure */

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	strPtr = (char STACKFAR *)char_id;
	intPtr = (ufix16 STACKFAR *)strPtr;
	if (sp_globals.gMustTranslate || (sp_globals.gCharProtocol == protoSymSet) || (sp_globals.gCharProtocol == protoPSEncode) )
		{/* do any and all translations */
		intPtr = (ufix16 STACKFAR *)&translatedValue;
		strPtr = (char STACKFAR *)strBuffer;
		if (!DoTranslate(PARAMS2 char_id, intPtr, strPtr))
			return(return_value); /* already FALSE */
		}
	if ( (sp_globals.gDestProtocol == protoBCID) || (sp_globals.gDestProtocol == protoMSL)
				|| (sp_globals.gDestProtocol == protoUser) )
		{
		translatedValue = *intPtr;
		intPtr = &translatedValue;
		if (!CanonicalToFileIndex(PARAMS2 intPtr))
			return(return_value); /* already FALSE */
		}

	switch (sp_globals.processor_type) {
#if PROC_SPEEDO
	case procSpeedo:
		return_value = sp_make_char(PARAMS2 *intPtr);
#if INCL_REMOTE
		if (!return_value)
			{/* check if flagged as a remote character */
			if (sp_is_remote_char(PARAMS2 *intPtr))
				sp_globals.FourIn1Flags |= BIT0;	/* set up for remote character stuff */
			}
#endif
		break;  
#endif /* if PROC_SPEEDO */	
#if PROC_TRUEDOC
	case procTrueDoc:
		{
		long pX=0, pY=0;
		int errCode;
		errCode = (btsbool)CspDoChar(CSP_PARAMS2 (unsigned short)*intPtr, &pX, &pY);
		if (errCode && (errCode != 7))
			sp_report_error(PARAMS2 (fix15)(errCode+1000));
		return_value = !errCode;
		}
		break;
#endif
#if PROC_PCL
	case procPCL:
	    return_value = eo_make_char(PARAMS2 *intPtr); 
		break;
#endif

#if PROC_TYPE1
	case procType1:
		if (sp_globals.gCharProtocol != protoDirectIndex)
#if RESTRICTED_ENVIRON
	    	return_value = tr_make_char(PARAMS2 font_ptr, (CHARACTERNAME *)strPtr);
#else
	    	return_value = tr_make_char(PARAMS2 (CHARACTERNAME *)strPtr);
#endif
		else
#if RESTRICTED_ENVIRON
	    	return_value = tr_make_char_idx(PARAMS2 font_ptr,*intPtr);
#else
	    	return_value = tr_make_char_idx(PARAMS2 *intPtr);
#endif
		break;  
#endif

#if PROC_TRUETYPE
	case procTrueType:
		if (sp_globals.gCharProtocol == protoDirectIndex)
	    	return_value = tt_make_char_idx(PARAMS2 *intPtr); 
		else
	    	return_value = tt_make_char(PARAMS2 *intPtr); 
		break;  
#endif

	default:
		/*  shouldn't get here unless bad processor type selected */
		sp_report_error (PARAMS2 5000);
		/* return_value already set to FALSE */
		break;
	}
	return(return_value);
}	/* end fi_make_char() */

/****************************************************************************
 *	fi_set_specs()
 *		set parameters for the next character
 ****************************************************************************/

FUNCTION btsbool WDECL fi_set_specs(TPS_GPTR2 ufe_struct STACKFAR *pspecs)
{
	btsbool success = FALSE;

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

#if CMT_EMPTY
   if ( (sp_globals.gMustTranslate == TRUE) || ((sp_globals.gCharProtocol == protoSymSet) && (sp_globals.gDestProtocol != protoBCID)) )
   {
      sp_report_error (PARAMS2 5002);
      return (FALSE);
   }
#endif

	switch (sp_globals.processor_type) {

#if PROC_SPEEDO
	case procSpeedo:

		success = sp_set_specs(PARAMS2 pspecs->Gen_specs);
		if (success)
			{/* set up these statics for BSearching */
#if INCL_REMOTE
			if ( (sp_globals.gCharProtocol != protoDirectIndex) && !(sp_globals.FourIn1Flags & BIT0))
#endif
				BuildSortedSpeedoIDList(PARAMS1);
			}
        break;
#endif /* if PROC_SPEEDO */

#if PROC_TRUEDOC
	case procTrueDoc:

		success = (btsbool)cspg_SetOutputSpecs(CSP_PARAMS2 pspecs->Gen_specs);
		break;
#endif

#if PROC_PCL
    case procPCL:

		success = eo_set_specs(PARAMS2 (eospecs_t *)pspecs->Gen_specs);
#ifdef HAVE_MSL2INDEX
		/* Enable if YOU write this function !!!  */
		if (success)
			{/* set up these statics for BSearching */
			if (sp_globals.gCharProtocol != protoDirectIndex)
				BuildSortedMSLIDList(CSP_PARAMS1);
			}
#endif
		if (success)
			sp_globals.metric_resolution = sp_globals.processor.pcl.eo_specs.pfont->resolution;
                break;
#endif

#if PROC_TYPE1
    case procType1:
		success = tr_set_specs(PARAMS2	pspecs->Gen_specs->flags,
								(real STACKFAR*)pspecs->Matrix,
								(ufix8 STACKFAR*)pspecs->Font.org);
		if (success)
			sp_globals.metric_resolution = 1000;
                break;
#endif

#if PROC_TRUETYPE
    case procTrueType:
		success = tt_set_specs(PARAMS2 pspecs->Gen_specs);
		if (success)
			sp_globals.metric_resolution = sp_globals.processor.truetype.emResolution;
                break;
#endif

    default:
                /*  shouldn't get here unless bad processor type selected */
                sp_report_error (PARAMS2 5000);
                break;

    } /* end switch(sp_globals.processor_type) */

	return success;
} /* end fi_set_specs() */

#if INCL_METRICS
/****************************************************************************
 *	fi_get_char_width()
 *		using the current font, return the requested character's width
 ****************************************************************************/

#if RESTRICTED_ENVIRON
FUNCTION fix31 WDECL fi_get_char_width(TPS_GPTR2 ufix8 STACKFAR *font_ptr, void STACKFAR *char_id)
#else
FUNCTION fix31 WDECL fi_get_char_width(TPS_GPTR2 void STACKFAR *char_id)
#endif
{
	fix31 char_width = 0; /* assume no width */
	ufix16 STACKFAR*intPtr, translatedValue;
	char STACKFAR*strPtr, strBuffer[32];

#if PROC_TYPE1
	real tr_width;
#endif

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	intPtr = (ufix16 STACKFAR*)char_id;
	strPtr = (char STACKFAR*)char_id;
	if (sp_globals.gMustTranslate || (sp_globals.gCharProtocol == protoSymSet) || (sp_globals.gCharProtocol == protoPSEncode) )
		{/* do any and all translations */
		intPtr = &translatedValue;
		strPtr = strBuffer;
		if (!DoTranslate(PARAMS2 char_id, intPtr, strPtr))
			return(char_width); /* already 0 */
		}
	if ( (sp_globals.gDestProtocol == protoBCID) || (sp_globals.gDestProtocol == protoMSL)
				|| (sp_globals.gDestProtocol == protoUser) )
		{
		translatedValue = *intPtr;
		intPtr = &translatedValue;
		if (!CanonicalToFileIndex(PARAMS2 intPtr))
			return(char_width); /* already 0 */
		}

	switch (sp_globals.processor_type) {
#if PROC_SPEEDO
	case procSpeedo:
		char_width = sp_get_char_width(PARAMS2 *intPtr);
        break;
#endif /* if PROC_SPEEDO */

#if PROC_TRUEDOC
	case procTrueDoc:
		{
		short xWidth, yWidth;
		int errCode;

		errCode = CspGetRawCharWidth(CSP_PARAMS2 (unsigned short)*intPtr, &xWidth, &yWidth);
		if (!errCode)
			{
			char_width = (fix31)xWidth;
			char_width = ((char_width << 16) + (sp_globals.metric_resolution >> 1)) / sp_globals.metric_resolution;
			}
		else if (errCode != 7)
			sp_report_error(PARAMS2 (fix15)(errCode+1000));
		}
		break;
#endif

#if PROC_PCL
    case procPCL:
		char_width = eo_get_char_width(PARAMS2 *intPtr);
        break;
#endif

#if PROC_TYPE1
    case procType1:
		if (sp_globals.gCharProtocol != protoDirectIndex)
#if RESTRICTED_ENVIRON
			tr_width = tr_get_char_width(PARAMS2 font_ptr, (CHARACTERNAME *)strPtr);
#else
			tr_width = tr_get_char_width(PARAMS2 (CHARACTERNAME *)strPtr);
#endif
		else
#if RESTRICTED_ENVIRON
			tr_width = tr_get_char_width_idx(PARAMS2 font_ptr, *intPtr);
#else
			tr_width = tr_get_char_width_idx(PARAMS2 *intPtr);
#endif
		char_width = (fix31)(tr_width * (real)65536.0);
        break;
#endif

#if PROC_TRUETYPE
    case procTrueType:
		if (sp_globals.gCharProtocol == protoDirectIndex)
	    	char_width = tt_get_char_width_idx(PARAMS2 *intPtr); 
		else
			char_width = tt_get_char_width(PARAMS2 *intPtr);
        break;
#endif

    default:
        /*  shouldn't get here unless bad processor type selected */
        sp_report_error (PARAMS2 5000);
		/* char_width already set to 0 */
		break;
    } /* end switch(sp_globals.processor_type) */

	return char_width;
}	/* end fi_get_char_width() */

/****************************************************************************
 *	fi_get_char_width_orus()
 *		using the current font, return the requested character's width in ORUS
 *		Also return in parameter emResolution the font's ORU units
 ****************************************************************************/

#if RESTRICTED_ENVIRON
FUNCTION fix31 WDECL fi_get_char_width_orus(TPS_GPTR2 ufix8 STACKFAR *font_ptr, void STACKFAR *char_id, ufix16 *emResolution)
#else
FUNCTION fix31 WDECL fi_get_char_width_orus(TPS_GPTR2 void STACKFAR *char_id, ufix16 *emResolution)
#endif
{
	fix31 char_width = 0; /* assume no width */
	ufix16 STACKFAR*intPtr, translatedValue;
	char STACKFAR*strPtr, strBuffer[32];

#if PROC_TYPE1
	real tr_width;
#endif

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	intPtr = (ufix16 STACKFAR*)char_id;
	strPtr = (char STACKFAR*)char_id;
	if (sp_globals.gMustTranslate || (sp_globals.gCharProtocol == protoSymSet) || (sp_globals.gCharProtocol == protoPSEncode) )
		{/* do any and all translations */
		intPtr = &translatedValue;
		strPtr = strBuffer;
		if (!DoTranslate(PARAMS2 char_id, intPtr, strPtr))
			return(char_width); /* already 0 */
		}
	if ( (sp_globals.gDestProtocol == protoBCID) || (sp_globals.gDestProtocol == protoMSL)
				|| (sp_globals.gDestProtocol == protoUser) )
		{
		translatedValue = *intPtr;
		intPtr = &translatedValue;
		if (!CanonicalToFileIndex(PARAMS2 intPtr))
			return(char_width); /* already 0 */
		}

	switch (sp_globals.processor_type) {
#if PROC_SPEEDO
	case procSpeedo:
		char_width = sp_get_char_width_orus(PARAMS2 *intPtr, emResolution);
        break;
#endif /* if PROC_SPEEDO */
	
#if PROC_TRUEDOC
	case procTrueDoc:
		{
		short xWidth, yWidth;
		int errCode;

		errCode = CspGetRawCharWidth(CSP_PARAMS2 (unsigned short)*intPtr, &xWidth, &yWidth);
		if (!errCode)
			{
			char_width = (fix31)xWidth;
			*emResolution = sp_globals.metric_resolution; /* must be set by fi_set_specs() */
			}
		else if (errCode != 7)
			sp_report_error(PARAMS2 (fix15)(errCode+1000));
		}
		break;
#endif

#if PROC_PCL
    case procPCL:
		char_width = eo_get_char_width_orus(PARAMS2 *intPtr, emResolution);
        break;
#endif

#if PROC_TYPE1
    case procType1:
		if (sp_globals.gCharProtocol != protoDirectIndex)
#if RESTRICTED_ENVIRON
			tr_width = tr_get_char_width(PARAMS2 font_ptr, (CHARACTERNAME *)strPtr);
#else
			tr_width = tr_get_char_width(PARAMS2 (CHARACTERNAME *)strPtr);
#endif
		else
#if RESTRICTED_ENVIRON
			tr_width = tr_get_char_width_idx(PARAMS2 font_ptr, *intPtr);
#else
			tr_width = tr_get_char_width_idx(PARAMS2 *intPtr);
#endif
		char_width = (fix31)(tr_width*1000.0);
		*emResolution = 1000;	/* 1000 for PostScript fonts */
        break;
#endif

#if PROC_TRUETYPE
    case procTrueType:
		if (sp_globals.gCharProtocol == protoDirectIndex)
	    	char_width = tt_get_char_width_idx_orus(PARAMS2 *intPtr, emResolution); 
		else
			char_width = tt_get_char_width_orus(PARAMS2 *intPtr, emResolution);
        break;
#endif

    default:
        /*  shouldn't get here unless bad processor type selected */
        sp_report_error (PARAMS2 5000);
		/* char_width already set to 0 */
		break;
    } /* end switch(sp_globals.processor_type) */

	return char_width;
}	/* end fi_get_char_width_orus() */
#endif /* INCL_METRICS */

#if INCL_METRICS
/*****************************************************************************
 *	fi_get_char_bbox()
 *		using the current font, return the bounding box of the requested
 *		character
 ****************************************************************************/

#if RESTRICTED_ENVIRON
FUNCTION btsbool fi_get_char_bbox(TPS_GPTR2 ufix8 STACKFAR *font_ptr, void STACKFAR *char_id, bbox_t *bounding_box)
#else
FUNCTION btsbool fi_get_char_bbox(TPS_GPTR2 void STACKFAR *char_id, bbox_t *bounding_box)
#endif
{

btsbool return_value = FALSE; /* assume failure */
	ufix16 STACKFAR*intPtr, translatedValue;
	char STACKFAR*strPtr, strBuffer[32];

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif


	intPtr = (ufix16 STACKFAR*)char_id;
	strPtr = (char STACKFAR*)char_id;
	if (sp_globals.gMustTranslate || (sp_globals.gCharProtocol == protoSymSet) || (sp_globals.gCharProtocol == protoPSEncode) )
		{ /* do any and all translations */
		intPtr = (ufix16 STACKFAR*)&translatedValue;
		strPtr = (char STACKFAR*)strBuffer;
		if (!DoTranslate(PARAMS2 char_id, intPtr, strPtr))
			return(return_value); /* already FALSE */
		}
	if ( (sp_globals.gDestProtocol == protoBCID) || (sp_globals.gDestProtocol == protoMSL)
				|| (sp_globals.gDestProtocol == protoUser) )
		{
		translatedValue = *intPtr;
		intPtr = (ufix16 STACKFAR*)&translatedValue;
		if (!CanonicalToFileIndex(PARAMS2 intPtr))
			return(return_value); /* already FALSE */
		}
		
	switch (sp_globals.processor_type) {
#if PROC_SPEEDO
	case procSpeedo:
		return_value = sp_get_char_bbox(PARAMS2 *intPtr, bounding_box);
        break;
#endif /* if PROC_SPEEDO */
#if PROC_TRUEDOC
	case procTrueDoc:
		{
		int errCode;
    	CspScaledBbox_t aBBox;
		errCode = CspGetScaledCharBBox(CSP_PARAMS2 (unsigned short)*intPtr, &aBBox);
		if (!errCode)
			{
			bounding_box->xmin = (fix31)aBBox.xmin;
			bounding_box->ymin = (fix31)aBBox.ymin;
			bounding_box->xmax = (fix31)aBBox.xmax;
			bounding_box->ymax = (fix31)aBBox.ymax;
			return_value = TRUE;
			}
		else if (errCode != 7)
			sp_report_error(PARAMS2 (fix15)(errCode+1000));
		}
		break;
#endif

#if PROC_PCL
    case procPCL:
		return_value = eo_get_char_bbox(PARAMS2 *intPtr, bounding_box);
        break;
#endif

#if PROC_TYPE1 | PROC_TRUETYPE
#if PROC_TRUETYPE
    case procTrueType:
#endif
#if PROC_TYPE1
    case procType1:
#endif
#if RESTRICTED_ENVIRON
		return_value = get_any_char_bbox(PARAMS2 font_ptr, intPtr, bounding_box, sp_globals.processor_type);
#else
		return_value = get_any_char_bbox(PARAMS2 intPtr, bounding_box, sp_globals.processor_type);
#endif
        break;
#endif /* PROC_TYPE1 | PROC_TRUETYPE */

    default:
        /*  shouldn't get here unless bad processor type selected */
        sp_report_error (PARAMS2 5000);
		/* return_value already == FALSE */
    } /* end switch(sp_globals.processor_type) */

	return return_value;
}	/* end fi_get_char_bbox() */
#endif /* INCL_METRICS */


/*****************************************************************************
 * DoTranslate()
 *   Master character translation wrap-around.  Calls fi_CharCodeXLate()
 *	Handles also symbol-set protocol, doing lookup in sp_globals.gCurrentSymbolSet[]
 *
 ****************************************************************************/

FUNCTION LOCAL_PROTO btsbool DoTranslate(SPGLOBPTR2 void STACKFAR *char_id, ufix16 STACKFAR *intPtr, char STACKFAR *strPtr)
{

ufix16 STACKFAR *iPtr;
btsbool return_value = FALSE; /* assume the worst */
	if (sp_globals.gMustTranslate)
		{/* this was set in fi_reset() */
		/* this means canonical to canonical */
		return_value = fi_CharCodeXLate  (char_id,
								(sp_globals.gDestProtocol == protoPSName) ?
								(void STACKFAR *)strPtr : (void STACKFAR *)intPtr,
								sp_globals.gCharProtocol, sp_globals.gDestProtocol);
		}
	else /* if ( (sp_globals.gCharProtocol == protoSymSet) || (sp_globals.gCharProtocol == protoPSEncode) ) */
		{/* this means symbol set lookup, then BCID to canonical */
		iPtr = (ufix16 STACKFAR *)char_id;
		if ( (*intPtr = sp_globals.gCurrentSymbolSet[*iPtr]) == (ufix16)UNKNOWN)
			return(return_value); /* already FALSE */
		return_value = fi_CharCodeXLate  ((void STACKFAR *)intPtr,
								(sp_globals.gDestProtocol == protoPSName) ?
									(void STACKFAR *)strPtr : (void STACKFAR *)intPtr,
								protoBCID, sp_globals.gDestProtocol);
		}
	return(return_value);
}

/*****************************************************************************
 * CanonicalToFileIndex()
 *   Convert input varible parameter <some canonical ID> -> actual file index
 *
 ****************************************************************************/

FUNCTION LOCAL_PROTO btsbool CanonicalToFileIndex(SPGLOBPTR2 ufix16 STACKFAR *intPtr)
{
btsbool return_value = FALSE;

	if (sp_globals.gCharProtocol != protoDirectIndex)
		{
		switch(sp_globals.gDestProtocol)
			{
			case protoBCID:
#if INCL_REMOTE
				if (sp_globals.FourIn1Flags & BIT0)
					return_value = TRUE;
				else
#endif
#if PROC_TRUEDOC
				if (sp_globals.processor_type == procTrueDoc)
					return_value = TRUE;
				else
#endif
#if PROC_SPEEDO
					return_value = BCIDtoIndex(PARAMS2 intPtr);
#else
					; /* NOP */
#endif /* else !PROC_SPEEDO */
				break;
			case protoMSL:
#if PROC_PCL
#ifdef HAVE_MSL2INDEX
				/* Enable if YOU write this function !!!  */
				return_value = MSLtoIndex(PARAMS2 intPtr);
#endif
#endif /* PROC_PCL */
				break;
			case protoUser:
				/* !!! Enable when YOU write this function !!!
				return_value = UsertoIndex(PARAMS2 intPtr);
				*/
				break;
			default:
				/* already set to FALSE, above
				return_value = FALSE;
				*/
				break;
			}
		}
	else
		return_value = TRUE; /* direct, no translation */
	return(return_value);
}

#if PROC_SPEEDO
/*****************************************************************************
 * BCIDCompare()
 *   Comparison function for BSearch when called by BCIDtoIndex()
 *		(relies upon sp_globals.processor.speedo.first_char_idx having already been set)
 *	RETURNS:	result of NumComp() (-1, 0 or 1 like strcmp())
 *
 ****************************************************************************/

FUNCTION fix15 BCIDCompare(SPGLOBPTR2 fix31 idx, void STACKFAR *keyValue)
{
	return(NumComp( (ufix16 STACKFAR *)keyValue, (ufix16 STACKFAR*)&sp_globals.gSortedBCIDList[idx].charID));
}

/*****************************************************************************
 * BCIDtoIndex()
 *   Convert input varible parameter BCID -> Speedo file index
 *		(relies upon sp_globals.processor.speedo.first_char_idx and sp_globals.processor.speedo.no_chars_avail having already been set)
 *	 Calls BSearch()
 *	RETURNS:	TRUE on success, FALSE, failure.
 *
 ****************************************************************************/

FUNCTION LOCAL_PROTO btsbool BCIDtoIndex(SPGLOBPTR2 ufix16 STACKFAR *intPtr)
{
ufix16 outValue;
btsbool success = FALSE;
ufix16	numChars;
	numChars = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_NCHRL);
	success = BSearch (PARAMS2  (fix15 STACKFAR*)&outValue, BCIDCompare,
						(void STACKFAR *)intPtr, (fix31)numChars);
	if (success)
		*intPtr = sp_globals.gSortedBCIDList[outValue].fileIndex;
	return(success);
}

/*****************************************************************************
 * BuildSortedSpeedoIDList()
 *   
 *
 *
 *	RETURNS:	nothing
 *
 ****************************************************************************/

FUNCTION LOCAL_PROTO void BuildSortedSpeedoIDList(SPGLOBPTR1)
{
fix15 i, j, k, theSlot, numAdded;
ufix16 numChars, theBCID, oldBCID = 0;
btsbool found, rigorous;
	rigorous = FALSE;
	sp_globals.gSortedBCIDList[0].charID = 0xffff;	/* the highest possible value */
	numAdded = 0;
	numChars = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_NCHRL);
	/* numChars now holds # visible glyphs in speedo font */
	for (i=0; i < numChars; i++, numAdded++)
		{
		theBCID = sp_get_char_id(PARAMS2 (ufix16)(sp_globals.processor.speedo.first_char_idx + i));
		/* now find a home for it in the list: */
		theSlot = numAdded;
		if (!rigorous && theBCID < oldBCID)
			rigorous = TRUE;
		if (rigorous)
			{
			found = FALSE;
			for (j=0; !found && (j < numAdded); j++)
				{
				if (sp_globals.gSortedBCIDList[j].charID > theBCID)
					{/* put it here */
					theSlot = j;
					found = TRUE;
					}
				}
			/* move up all items from theSlot to numAdded by 1 */
			for (k = numAdded; k > theSlot; k--)
				sp_globals.gSortedBCIDList[k] = sp_globals.gSortedBCIDList[k-1];
			}

		/* drop new item in theSlot */
		sp_globals.gSortedBCIDList[theSlot].charID = theBCID;
		sp_globals.gSortedBCIDList[theSlot].fileIndex = sp_globals.processor.speedo.first_char_idx + i;
		oldBCID = theBCID;
		}
}
#endif /* if PROC_SPEEDO */

#if INCL_METRICS
#if PROC_TRUETYPE | PROC_TYPE1
/* --------- Bounding Box functions for TrueType and Type1: ------------ */
typedef void (*PFV)();
typedef btsbool (*PFB)();

GLOBAL_PROTO void bb_do_nothing(SPGLOBPTR1);
GLOBAL_PROTO btsbool bb_its_true(SPGLOBPTR1);

GLOBAL_PROTO void bb_begin_contour(SPGLOBPTR2 point_t P1, btsbool outside);
GLOBAL_PROTO void bb_line(SPGLOBPTR2 point_t P1);
GLOBAL_PROTO void bb_curve(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth);

#define BB_INFINITY	0x7FFF

#if RESTRICTED_ENVIRON
FUNCTION btsbool get_any_char_bbox(SPGLOBPTR2 ufix8 STACKFAR *font_ptr,
											 void STACKFAR *char_id,
											 bbox_t *bbox, eFontProcessor fontType)
#else
FUNCTION btsbool get_any_char_bbox(SPGLOBPTR2 void STACKFAR *char_id, bbox_t *bbox, eFontProcessor fontType)
#endif
{

begin_sub_charFunc	sv_begin_sub_char;
begin_contourFunc	sv_begin_contour;
curveFunc			sv_curve;
lineFunc			sv_line;
end_contourFunc		sv_end_contour;
end_sub_charFunc	sv_end_sub_char;
init_outFunc		sv_init_out;
begin_charFunc		sv_begin_char;
end_charFunc		sv_end_char;

ufix16 STACKFAR *intPtr;
btsbool success = FALSE;

	intPtr = (ufix16 STACKFAR *)char_id;

		/* save original function pointers: */
	sv_init_out			= sp_globals.init_out;
   	sv_begin_char			= sp_globals.begin_char;
   	sv_begin_sub_char		= sp_globals.begin_sub_char;
   	sv_begin_contour		= sp_globals.begin_contour;
   	sv_curve			= sp_globals.curve;
   	sv_line				= sp_globals.line;
   	sv_end_contour			= sp_globals.end_contour;
   	sv_end_sub_char			= sp_globals.end_sub_char;
   	sv_end_char			= sp_globals.end_char;
	/* substitute our function pointers: */
	sp_globals.init_out		= (init_outFunc)bb_its_true;
   	sp_globals.begin_char		= (begin_charFunc)bb_its_true;
   	sp_globals.begin_sub_char	= (begin_sub_charFunc)bb_do_nothing;
   	sp_globals.begin_contour	= bb_begin_contour;
   	sp_globals.curve		= bb_curve;
   	sp_globals.line			= bb_line;
   	sp_globals.end_contour		= (end_contourFunc)bb_do_nothing;
   	sp_globals.end_sub_char		= (end_sub_charFunc)bb_do_nothing;
   	sp_globals.end_char		= (end_charFunc)bb_its_true;

	/* initialize bb_ max/min variables: */
	sp_globals.bmap_xmax = -BB_INFINITY;
	sp_globals.bmap_xmin =  BB_INFINITY;
	sp_globals.bmap_ymax = -BB_INFINITY;
	sp_globals.bmap_ymin =  BB_INFINITY;
	sp_globals.bb_someCall = FALSE;
	bbox->xmin  = (fix31)0;
	bbox->xmax  = (fix31)0;
	bbox->ymin  = (fix31)0;
	bbox->ymax  = (fix31)0;


	/* do the voo-doo: */
#if PROC_TYPE1
	if (fontType == procType1)
		{
		if (sp_globals.gCharProtocol != protoDirectIndex)
#if RESTRICTED_ENVIRON
	    	success = tr_make_char(PARAMS2 font_ptr,char_id);
#else
	    	success = tr_make_char(PARAMS2 char_id);
#endif
		else
#if RESTRICTED_ENVIRON
	    	success = tr_make_char_idx(PARAMS2 font_ptr,*intPtr);
#else
	    	success = tr_make_char_idx(PARAMS2 *intPtr);
#endif
		}
#endif

#if PROC_TRUETYPE
	if (fontType == procTrueType)
		{/* must be TrueType */
		if (sp_globals.gCharProtocol == protoDirectIndex)
	    	success = tt_make_char_idx(PARAMS2 *intPtr); 
		else
	    	success = tt_make_char(PARAMS2 *intPtr); 
		}
#endif

	/* restore original function pointers: */
	sp_globals.init_out			= sv_init_out;
   	sp_globals.begin_char		= sv_begin_char;
   	sp_globals.begin_sub_char	= sv_begin_sub_char;
   	sp_globals.begin_contour	= sv_begin_contour;
   	sp_globals.curve			= sv_curve;
   	sp_globals.line				= sv_line;
   	sp_globals.end_contour		= sv_end_contour;
   	sp_globals.end_sub_char		= sv_end_sub_char;
   	sp_globals.end_char			= sv_end_char;

	if (sp_globals.bb_someCall)
		{
		bbox->xmin  = (fix31)sp_globals.bmap_xmin << sp_globals.poshift;
		bbox->xmax  = (fix31)sp_globals.bmap_xmax << sp_globals.poshift;
		bbox->ymin  = (fix31)sp_globals.bmap_ymin << sp_globals.poshift;
		bbox->ymax  = (fix31)sp_globals.bmap_ymax << sp_globals.poshift;
		}
	return success;
}



/* --------------- the BUSINESS functions ------------ */

FUNCTION void bb_check_point(SPGLOBPTR2 point_t P1)
{
	/* check P1 x and y against current extremes: */
	if (P1.x < sp_globals.bmap_xmin)
		sp_globals.bmap_xmin = P1.x;
	if (P1.x > sp_globals.bmap_xmax)
		sp_globals.bmap_xmax = P1.x;

	if (P1.y < sp_globals.bmap_ymin)
		sp_globals.bmap_ymin = P1.y;
	if (P1.y > sp_globals.bmap_ymax)
		sp_globals.bmap_ymax = P1.y;
	sp_globals.bb_someCall = TRUE; /* show something was checked */
}

FUNCTION void bb_begin_contour(SPGLOBPTR2 point_t P1, btsbool outside)
{
	bb_check_point(PARAMS2 P1);
}

FUNCTION void bb_line(SPGLOBPTR2 point_t P1)
{
	bb_check_point(PARAMS2 P1);
}

FUNCTION void bb_curve(SPGLOBPTR2 point_t P1, point_t P2, point_t P3, fix15 depth)
{
	/* bb_check_point(PARAMS2 P1); */
	/* bb_check_point(PARAMS2 P2); */
	bb_check_point(PARAMS2 P3);
}


/* ---------------- the NOP functions ---------------- */

FUNCTION btsbool bb_its_true(SPGLOBPTR1)
{
	/* everything went splendidly! */
	return TRUE;
}

FUNCTION void bb_do_nothing(SPGLOBPTR1)
{
	/* retired- no variables, no code */
}


#endif /* PROC_TRUETYPE | PROC_TYPE1 */
#endif /* INCL_METRICS */


/* EOF: frontend.c */
