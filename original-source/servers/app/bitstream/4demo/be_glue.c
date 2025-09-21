#include "be_glue.h"
#include "as_support.h"
#include <Debug.h>

/***********************************************************************************/

/***** STATIC FUNCTIONS *****/

LOCAL_PROTO	 fix15 read_2b(ufix8 *pointer);
GLOBAL_PROTO fix31 read_4b(ufix8 *pointer);

/***********************************************************************************/

/* Called by character generator to report an error. Since character data not available
   is one of those errors that happens many times, don't report it to user. */
FUNCTION void WDECL sp_report_error(SPGLOBPTR2 fix15 n) {
	switch(n) {
	case 1:
		xprintf("Insufficient font data loaded\n");
		break;

	case 3:
		xprintf("Transformation matrix out of range\n");
		break;

	case 4:
		xprintf("Font format error\n");
		break;

	case 5:
		xprintf("Requested specs not compatible with output module\n");
		break;

	case 7:
		xprintf("Intelligent transformation requested but not supported\n");
		break;

	case 8:
		xprintf("Unsupported output mode requested\n");
		break;

	case 9:
		xprintf("Extended font loaded but only compact fonts supported\n");
		break;

	case 10:
		xprintf("Font specs not set prior to use of font\n");
		break;

	case 12:
		xprintf("Mysterious bug 12\n");
		break;

	case 13:
		xprintf("Track kerning data not available()\n");
		break;

	case 14:
		xprintf("Pair kerning data not available()\n");
		break;

	case TR_NO_ALLOC_FONT:
		xprintf("Mysterious bug TR_NO_ALLOC_FONT\n");
		break;

	case TR_NO_SPC_STRINGS:
		xprintf("*** Cannot malloc space for charstrings \n");
		break;

	case TR_NO_SPC_SUBRS:
		xprintf("*** Cannot malloc space for subrs\n");
		break;

	case TR_NO_READ_SLASH:
		xprintf("*** Cannot read / before charactername in Encoding\n");
		break;

	case TR_NO_READ_STRING:
		xprintf("*** Cannot read / or end token for CharString\n");
		break;

	case TR_NO_READ_FUZZ:
		xprintf("*** Cannot read BlueFuzz value\n");
		break;

	case TR_NO_READ_SCALE:
		xprintf("*** Cannot read BlueScale value\n");
		break;

	case TR_NO_READ_SHIFT:
		xprintf("*** Cannot read BlueShift value\n");
		break;

	case TR_NO_READ_VALUES:
		xprintf("*** Cannot read BlueValues array\n");
		break;

	case TR_NO_READ_ENCODE:
		xprintf("*** Cannot read Encoding index\n");
		break;

	case TR_NO_READ_FAMILY:
		xprintf("*** Cannot read FamilyBlues array\n");
		break;

	case TR_NO_READ_FAMOTHER:
		xprintf("*** Cannot read FamilyOtherBlues array\n");
		break;

	case TR_NO_READ_BBOX0:
		xprintf("*** Cannot read FontBBox element 0\n");
		break;

	case TR_NO_READ_BBOX1:
		xprintf("*** Cannot read FontBBox element 1\n");
		break;

	case TR_NO_READ_BBOX2:
		xprintf("*** Cannot read FontBBox element 2\n");
		break;

	case TR_NO_READ_BBOX3:
		xprintf("*** Cannot read FontBBox element 3\n");
		break;

	case TR_NO_READ_MATRIX:
		xprintf("*** Cannot read FontMatrix\n");
		break;

	case TR_NO_READ_NAMTOK:
		xprintf("*** Cannot read FontName / token\n");
		break;

	case TR_NO_READ_NAME:
		xprintf("*** Cannot read FontName\n");
		break;

	case TR_NO_READ_BOLD:
		xprintf("*** Cannot read ForceBold value\n");
		break;

	case TR_NO_READ_FULLNAME:
		xprintf("*** Cannot read FullName value\n");
		break;

	case TR_NO_READ_LANGGRP:
		xprintf("*** Cannot read LanguageGroup value\n");
		break;

	case TR_NO_READ_OTHERBL:
		xprintf("*** Cannot read OtherBlues array\n");
		break;

	case TR_NO_READ_SUBRTOK:
		xprintf("*** Cannot read RD token for subr\n");
		break;

	case TR_NO_READ_STRINGTOK:
		xprintf("*** Cannot read RD token in charstring\n");
		break;

	case TR_NO_READ_STDHW:
		xprintf("*** Cannot read StdHW value\n");
		break;

	case TR_NO_READ_STDVW:
		xprintf("*** Cannot read StdVW value\n");
		break;

	case TR_NO_READ_SNAPH:
		xprintf("*** Cannot read StemSnapH array\n");
		break;

	case TR_NO_READ_SNAPV:
		xprintf("*** Cannot read StemSnapV array\n");
		break;

	case TR_NO_READ_BINARY:
		xprintf("*** Cannot read binary data size for Subr\n");
		break;

	case TR_NO_READ_EXECBYTE:
		xprintf("*** Cannot read a byte after eexec\n");
		break;

	case TR_NO_READ_CHARNAME:
		xprintf("*** Cannot read charactername\n");
		break;

	case TR_NO_READ_STRINGBIN:
		xprintf("*** Cannot read charstring binary data\n");
		break;

	case TR_NO_READ_STRINGSIZE:
		xprintf("*** Cannot read charstring size\n");
		break;

	case TR_NO_READ_DUPTOK:
		xprintf("*** Cannot read dup token for subr\n");
		break;

	case TR_NO_READ_ENCODETOK:
		xprintf("*** Cannot read dup, def or readonly token for Encoding\n");
		break;

	case TR_NO_READ_EXEC1BYTE:
		xprintf("*** Cannot read first byte after eexec\n");
		break;

	case TR_NO_READ_LENIV:
		xprintf("*** Cannot read lenIV value\n");
		break;

	case TR_NO_READ_LITNAME:
		xprintf("*** Cannot read literal name after /\n");
		break;

	case TR_NO_READ_STRINGNUM:
		xprintf("*** Cannot read number of CharStrings\n");
		break;

	case TR_NO_READ_NUMSUBRS:
		xprintf("*** Cannot read number of Subrs\n");
		break;

	case TR_NO_READ_SUBRBIN:
		xprintf("*** Cannot read subr binary data\n");
		break;

	case TR_NO_READ_SUBRINDEX:
		xprintf("*** Cannot read subr index\n");
		break;

	case TR_NO_READ_TOKAFTERENC:
		xprintf("*** Cannot read token after Encoding\n");
		break;

	case TR_NO_READ_OPENBRACKET:
		xprintf("*** Cannot read { or [ in FontBBox\n");
		break;

	case TR_EOF_READ:
		xprintf("*** End of file read\n");
		break;

	case TR_MATRIX_SIZE:
		xprintf("*** FontMatrix has wrong number of elements\n");
		break;

	case TR_PARSE_ERR:
		xprintf("*** Parsing error in Character program string\n");
		break;

	case TR_TOKEN_LARGE:
		xprintf("*** Token too large\n");
		break;

	case TR_TOO_MANY_SUBRS:
		xprintf("*** Too many subrs\n");
		break;

	case TR_NO_SPC_ENC_ARR:
		xprintf("*** Unable to allocate storage for encoding array \n");
		break;

	case TR_NO_SPC_ENC_ENT:
		xprintf("*** Unable to malloc space for encoding entry\n");
		break;

	case TR_NO_FIND_CHARNAME:
		xprintf("*** get_chardef: Cannot find char name\n");
		break;

	case TR_INV_FILE:
		xprintf("*** Not a valid Type1 font file\n");
		break;

	case TR_BUFFER_TOO_SMALL:
		xprintf("*** Buffer provided too small to store essential data for type1 fonts \n");
		break;

	case TR_BAD_RFB_TAG:
		xprintf("*** Invalid Tag found in charactaer data\n");
		break;

	default:
		xprintf("###################### report_error(%d)\n", n);
		break;
	}
}

/* Reads 2-byte field from font buffer. */ 
FUNCTION static fix15 read_2b(ufix8 *pointer) {
	fix15 temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer);
	return temp;
}

/* Reads 4-byte field from font buffer. */ 
FUNCTION fix31 read_4b(ufix8 *pointer) {
	fix31 temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer);
	return temp;
}

/****************************************************************************
  setup_tt_font()
		This function handles all the initializations for the TrueTyp font:
		.  setup the font protocol type and font type by calling fi_reset()
		.  load the font buffer by calling tt_load_font()
		.  setup the values for transformation metrics and call fi_set_specs()
		.  load font indecis into font_table if users chose to show all characters 
  RETURNS:
		TRUE if successful
		FALSE if failed
*****************************************************************************/

fc_renderer_context *fc_create_new_context() {
	fc_renderer_context   *ctxt;

	ctxt = (fc_renderer_context*)grMalloc(sizeof(fc_renderer_context),"fc:ctxt",MALLOC_CANNOT_FAIL);
	memset(ctxt, 0, sizeof(fc_renderer_context));
	ctxt->sp_global.UserPtr = ctxt;
	ctxt->was_load = 0;
	return ctxt;
}

void fc_set_file(fc_renderer_context *ctxt, FILE *fp, uint16 token) {
	ctxt->font_fp = fp;
}

FILE *fc_get_file(fc_renderer_context *ctxt) {
	return ctxt->font_fp;
}

int fc_setup_tt_font(struct fc_renderer_context *ctxt,
					 int bpp, int gray_level, int *matrix, int default_flag)
{
	ufe_struct	    tspecs;
	eFontProtocol	font_protocol;

	if (default_flag)
		font_protocol = protoDirectIndex;
	else
		font_protocol = protoUnicode;

	fi_reset(&ctxt->sp_global, font_protocol, procTrueType);

	if (ctxt->was_load == 0) {
		fc_release_font(ctxt);
		if (!tt_load_font_params(&ctxt->sp_global, (ufix32)ctxt->font_fp, 3, 1))
			if (!tt_load_font_params(&ctxt->sp_global, (ufix32)ctxt->font_fp, 0, 0))
				return false;
		ctxt->was_load = 1;
	}
	
	ctxt->sp_global.pixelSize = 3;

	ctxt->gspecs.xxmult = matrix[0];
	ctxt->gspecs.xymult = matrix[1];
	ctxt->gspecs.yxmult = matrix[2];
	ctxt->gspecs.yymult = matrix[3];
	ctxt->gspecs.xoffset = matrix[4];
	ctxt->gspecs.yoffset = matrix[5];

	if (bpp == 1)
		ctxt->mode_id = MODE_2D;
	else if ((gray_level == 0) && (matrix[1] == 0) && (matrix[2] == 0))
		ctxt->mode_id = MODE_USER;
	else
		ctxt->mode_id = MODE_GRAY;
	
	ctxt->gspecs.flags = ctxt->mode_id;
	
	tspecs.Gen_specs = &ctxt->gspecs;

	if (!fi_set_specs(&ctxt->sp_global, &tspecs))
		return false;
	return true;
}

int fc_setup_ps_font(struct fc_renderer_context *ctxt,
					 int bpp, int gray_level, int *matrix, int default_flag)
{
	ufe_struct	    tspecs;
	eFontProtocol	font_protocol;

	if (default_flag)
		return false;
	fi_reset(&ctxt->sp_global, protoPSEncode, procType1);

	if (ctxt->was_load == 0) {
		if (!tr_load_font(&ctxt->sp_global, &ctxt->font_ptr, (void*)ctxt))
			return false;
		ctxt->was_load = 2;
	}
	
	ctxt->sp_global.pixelSize = 3;

	/* WARNING : be careful, for some weird reason, the postscript matrix
	   is inverted compared to the TrueType reference matrix !! */
	tspecs.Matrix[0] = ((float)matrix[0])*(1.0/65536.0);
	tspecs.Matrix[1] = ((float)matrix[2])*(1.0/65536.0); /* M[0,1] and M[1,0] */
	tspecs.Matrix[2] = ((float)matrix[1])*(1.0/65536.0); /* are inverted !!   */
	tspecs.Matrix[3] = ((float)matrix[3])*(1.0/65536.0);
	tspecs.Matrix[4] = ((float)matrix[4])*(1.0/65536.0);
	tspecs.Matrix[5] = ((float)matrix[5])*(1.0/65536.0);
	memcpy(ctxt->matrix, tspecs.Matrix, sizeof(double)*6);

	if (bpp == 1)
		ctxt->mode_id = MODE_2D;
	else if ((gray_level == 0) && (matrix[1] == 0) && (matrix[2] == 0))
		ctxt->mode_id = MODE_USER;
	else
		ctxt->mode_id = MODE_GRAY;
		
	ctxt->gspecs.flags = ctxt->mode_id;
	
	tspecs.Gen_specs = &ctxt->gspecs;
	tspecs.Font.org = (uchar*)ctxt->font_ptr;

	if (!fi_set_specs(&ctxt->sp_global, &tspecs))
		return false;
	return true;
}

void fc_release_font(struct fc_renderer_context *ctxt) {
	if (ctxt->was_load == 1)
		tt_release_font(&ctxt->sp_global);
	else if (ctxt->was_load == 2)
		tr_unload_font(ctxt->font_ptr);
	ctxt->was_load = 0;
}

/* macintosh isolatin encoding for postscript type 1 translation */
extern uint16 *b_symbol_set_table[];

CHARACTERNAME *unicode_2_psname(uint16 code, int32 iso_set, CHARACTERNAME **names) {
	int			i;
	uint16		*set;
	
	if (code < 0x0080)
		return names[code];
	set = b_symbol_set_table[iso_set];
	for (i=0x0080; i<0x0100; i++)
		if (set[i] == code)
			return names[i];
	return NULL;
}

/* character outline rendering entry */
int fc_outline_call_glue(struct fc_renderer_context *ctxt, void *char_desc) {
	bool		ret_val;
	ufe_struct	tspecs;	
	
	/* we need to set the matrix and the font file again for postscript fonts */
	memcpy(tspecs.Matrix, ctxt->matrix, sizeof(double)*6);
	tspecs.Font.org = (uchar*)ctxt->font_ptr;

	tspecs.Gen_specs = &ctxt->gspecs;		
	if (!fi_set_specs(&ctxt->sp_global, &tspecs))
		return false;
		
	ret_val = false;
	switch (ctxt->was_load) {
	case 1 :
		ret_val = fi_make_char(&ctxt->sp_global, char_desc);
		break;
	case 2 :
	 	{
			CHARACTERNAME	*name;
			
			name = unicode_2_psname(((uint16*)char_desc)[0], 11,
									tr_get_encode(&ctxt->sp_global));
			if (name == NULL)
				ret_val = false;
			else
        		ret_val = tr_make_char(&ctxt->sp_global, name);
		}
		break;
	}
	return ret_val;
}

/*---------------------------------------------------------------------*/
/* OUTLINE GLUE */

FUNCTION void WDECL sp_open_outline(	SPEEDO_GLOBALS *sp_global_ptr, 
										fix31	x_width,
										fix31	y_width,
										fix31	xmin,
										fix31	xmax,
										fix31	ymin,
										fix31	ymax) {
	sp_open_outline_wrap(sp_global_ptr, x_width, y_width, xmin, xmax, ymin, ymax);
}

FUNCTION void WDECL sp_start_new_char(SPEEDO_GLOBALS *sp_global_ptr) {
	sp_start_new_char_wrap(sp_global_ptr);
}

FUNCTION void WDECL sp_start_contour(	SPEEDO_GLOBALS *sp_global_ptr,
										fix31	x,
										fix31	y,
										char	outside) {
	sp_start_contour_wrap(sp_global_ptr, x, y, outside);
}

FUNCTION void WDECL sp_curve_to(	SPEEDO_GLOBALS *sp_global_ptr,
									fix31	x1,
									fix31	y1,
									fix31	x2,
									fix31	y2,
									fix31	x3,
									fix31	y3) {
	sp_curve_to_wrap(sp_global_ptr, x1, y1, x2, y2, x3, y3);
}

FUNCTION void WDECL sp_line_to(	SPEEDO_GLOBALS *sp_global_ptr,
								fix31	x,
								fix31	y) {
	sp_line_to_wrap(sp_global_ptr, x, y);
}

FUNCTION void WDECL sp_close_contour(SPEEDO_GLOBALS *sp_global_ptr) {
	sp_close_contour_wrap(sp_global_ptr);
}

FUNCTION void WDECL sp_close_outline(SPEEDO_GLOBALS *sp_global_ptr) {
	sp_close_outline_wrap(sp_global_ptr);
}
