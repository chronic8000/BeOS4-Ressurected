/******************************************************************************
*                                                                             *
*  Copyright 1988, 1989, 1990, 1995 as an unpublished work by Bitstream Inc., *
*  Cambridge, MA                                                              *
*                         U.S. Patent No 4,785,391                            *
*                           Other Patent Pending                              *
*                                                                             *
*         These programs are the sole property of Bitstream Inc. and          *
*           contain its proprietary and confidential information.             *
*                                                                             *
******************************************************************************/
/********************* Revision Control Information **********************************
*                                                                                    
*     $Header: //bliss/user/archive/rcs/type1/nsample.c,v 33.0 97/03/17 17:58:48 shawn Release $                                                                        *
*                                                                                    
*     $Log:	nsample.c,v $
 * Revision 33.0  97/03/17  17:58:48  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.5  96/07/02  12:53:39  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 32.4  96/06/05  15:07:04  shawn
 * Added support for the GrayWriter.
 * Added support for the new Type 1 font loader.
 * Fixed RESTRICTED_ENVIRON condition.
 * 
 * Revision 32.3  96/01/31  14:13:40  shawn
 * Added PROTOTYPE statements to eliminate compiler warnings.
 * 
 * Revision 32.2  96/01/31  10:33:32  shawn
 * Made code ANSI to eliminate compiler warnings.
 * 
 * Revision 32.1  95/11/17  11:02:24  shawn
 * Changed lines from fix15 to ufix32 for '%ld' sscanf function
 * 
 * Revision 32.0  95/02/15  16:44:40  shawn
 * Release
 * 
 * Revision 31.2  95/01/10  09:27:12  roberte
 * Updated copyright header.
 * 
 * Revision 31.1  95/01/04  16:34:12  roberte
 * Release
 * 
 * Revision 30.2  94/12/16  15:26:56  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 30.1  94/10/25  09:55:49  roberte
 * Remove include of stdef.h. Data types now seen in
 * nested include of btstypes.h from speedo.h
 * 
 * Revision 30.0  94/05/04  09:27:56  roberte
 * Release
 * 
 * Revision 29.0  94/04/08  12:08:54  roberte
 * Release
 * 
 * Revision 28.31  94/02/03  13:15:45  roberte
 * Added TurboC to macro previously isolating only Microsoft Compiler.
 * 
 * Revision 28.30  93/08/30  15:42:59  roberte
 * Release
 * 
 * Revision 28.26  93/05/04  12:33:38  roberte
 * Made module more REENTRANT_ALLOC and GLOBALFAR aware.
 * 
 * Revision 28.25  93/04/23  17:53:56  roberte
 * #ifdef'd out keypress code.
 * 
 * Revision 28.24  93/03/15  13:07:00  roberte
 * Release
 * 
 * Revision 28.12  93/03/11  15:54:35  roberte
 * Changed #if __MSDOS to #ifdef MSDOS.
 * 
 * Revision 28.11  93/03/10  17:13:06  roberte
 * Changed type of main() from void to int.
 * 
 * Revision 28.10  93/03/09  12:16:54  roberte
 *  Replaced #if INTEL tests with #if __MSDOS as appropriate.
 * 
 * Revision 28.9  93/02/01  16:14:18  roberte
 * Added default setting of lines per em if only 2 parameters supplied.
 * 
 * Revision 28.8  93/01/21  13:20:12  roberte
 * Added macros for reentrant code PARAMS1 and PARAMS2. Corrected call to tr_get_encode().
 * 
 * Revision 28.7  92/11/19  15:33:43  weili
 * Release
 * 
 * Revision 26.6  92/10/21  09:58:06  davidw
 * Turned off DEBUG
 * 
 * Revision 26.5  92/10/20  09:48:47  davidw
 * removed bogus zero_bbox field reference
 * 
 * Revision 26.4  92/10/19  17:35:18  laurar
 * change fnt.h to fnt_a.h
 * 
 * Revision 26.3  92/10/16  16:36:58  davidw
 * WIP: t1 zero FontBBox bug, not working yet
 * 
 * Revision 26.2  92/10/05  17:43:27  laurar
 * added q to quit line;
 * cast MODE_BLACK (in call to tr_set_specs) as a long so
 * that it would work on the PC.
 * 
 * Revision 26.1  92/06/26  10:23:50  leeann
 * Release
 * 
 * Revision 25.1  92/04/06  11:40:38  leeann
 * Release
 * 
 * Revision 24.1  92/03/23  14:08:26  leeann
 * Release
 * 
 * Revision 23.1  92/01/29  16:59:36  leeann
 * Release
 * 
 * Revision 22.1  92/01/20  13:31:02  leeann
 * Release
 * 
 * Revision 21.2  91/11/19  18:26:54  leeann
 * include useropt.h as the first file
 * 
 * Revision 21.1  91/10/28  16:43:19  leeann
 * Release
 * 
 * Revision 20.1  91/10/28  15:27:05  leeann
 * Release
 * 
 * Revision 18.1  91/10/17  11:38:41  leeann
 * Release
 * 
 * Revision 17.1  91/06/13  10:43:02  leeann
 * Release
 * 
 * Revision 16.1  91/06/04  15:33:51  leeann
 * Release
 * 
 * Revision 15.1  91/05/08  18:06:06  leeann
 * Release
 * 
 * Revision 14.1  91/05/07  16:27:35  leeann
 * Release
 * 
 * Revision 13.2  91/05/06  09:50:51  leeann
 * fix get encoding array for RESTRICTED_ENVIRON
 * 
 * Revision 13.1  91/04/30  17:02:08  leeann
 * Release
 * 
 * Revision 12.1  91/04/29  14:52:51  leeann
 * Release
 * 
 * Revision 11.3  91/04/29  14:29:11  leeann
 * *** empty log message ***
 * 
 * Revision 11.2  91/04/10  13:16:52  leeann
 *  support character names as structures
 * 
 * Revision 11.1  91/04/04  10:56:07  leeann
 * Release
 * 
 * Revision 10.2  91/03/25  13:55:19  leeann
 * put in a dummy function to see how
 * long it takes to just read the font
 * 
 * Revision 10.1  91/03/14  14:27:42  leeann
 * Release
 * 
 * Revision 9.2  91/03/14  14:15:28  leeann
 * fix for loop
 * 
 * Revision 9.1  91/03/14  10:04:27  leeann
 * Release
 * 
 * Revision 8.5  91/03/13  16:18:38  leeann
 * Support RESTRICTED_ENVIRON
 * 
 * Revision 8.4  91/02/13  16:04:31  joyce
 * Added new error message to sp_report_error (TR_INV_FILE)
 * 
 * Revision 8.3  91/02/12  12:55:11  leeann
 * change tr_make_char and tr_set_specs to boolean
 * 
 * Revision 8.2  91/02/04  17:41:36  leeann
 * declare external functions
 * 
 * Revision 8.1  91/01/30  19:01:15  leeann
 * Release
 * 
 * Revision 7.2  91/01/30  18:57:24  leeann
 * take out keys.h, unload font when done
 * 
 * Revision 7.1  91/01/22  14:25:17  leeann
 * Release
 * 
 * Revision 6.1  91/01/16  10:51:42  leeann
 * Release
 * 
 * Revision 5.6  91/01/15  17:40:58  leeann
 * fixup comments
 * 
 * Revision 5.5  91/01/15  17:15:57  joyce
 * 1. Added exit of program when tr_load_font fails
 * 2. Added new case to report_error switch
 * 
 * Revision 5.4  91/01/10  12:06:01  leeann
 * fix get_byte bug
 * 
 * Revision 5.3  91/01/10  11:20:56  joyce
 * 1. Add code to test new loader functions (get_encode, etc)
 * 2. Add new error cases to sp_report_error
 * 
 * Revision 5.2  91/01/07  19:57:53  leeann
 * put in fileio and add open_font, get_byte, and close_font functions
 * 
 * Revision 5.1  90/12/12  17:18:30  leeann
 * Release
 * 
 * Revision 4.1  90/12/12  14:44:12  leeann
 * Release
 * 
 * Revision 3.1  90/12/06  10:26:35  leeann
 * Release
 * 
 * Revision 2.1  90/12/03  12:55:17  mark
 * Release
 * 
 * Revision 1.2  90/11/29  17:22:14  joyce
 * Changed interface to load_font and make_char
 * 
 * Revision 1.1  90/11/28  14:05:52  joyce
 * Initial revision
 * 
 * Revision 1.3  90/09/17  13:20:04  roger
 * put in rcsid[] to help RCS
 * 
 * Revision 1.2  90/09/14  16:33:01  roger
 * obliqued fonts now work, also added pointsize to command line
 * 
 * Revision 1.1  90/08/13  15:27:53  arg
 * Initial revision
 * 
  
/*************************** N S A M P L E . C *******************************
 *                                                                           *
 *                 TYPE 1 CHARACTER GENERATOR TEST MODULE                    *
 *                                                                           *
 * This is an illustration of what external resources are required to        *
 * load a type 1 outline and use the type 1 character generator to generate  *
 * bitmaps or scaled outlines according to the desired specification.        *
 * This program loads a type 1 outline, defines a set of character           *
 * generation specifications, generates bitmap (or outline) data for each    *
 * character in the font and prints them on the standard output.             *
 *                                                                           *
 *                                                                           *
 ****************************************************************************/

static char     rcsid[] = "$Header: //bliss/user/archive/rcs/type1/nsample.c,v 33.0 97/03/17 17:58:48 shawn Release $";

#include <string.h>
#include <stdio.h>
#define SET_SPCS
#include "spdo_prv.h"		/* General definition for make_bmap */
#include "type1.h"
#include "fnt_a.h"
#include "tr_fdata.h"
#include "errcodes.h"

#ifdef MSDOS
#include <stddef.h>
#include <malloc.h>
#include <stdlib.h>
#else
GLOBAL_PROTO void *malloc(size_t size);
GLOBAL_PROTO void exit(int status);
#endif

#ifndef DEBUG
#define DEBUG   0
#endif

#if DEBUG
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#define MAX_BITS  256		/* Max line length of generated bitmap */

/***** GLOBAL FUNCTIONS *****/

GLOBAL_PROTO int main(int argc, char *argv[]);
GLOBAL_PROTO btsbool open_font(char *pathname);
GLOBAL_PROTO void close_font(void);

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC FUNCTIONS *****/

/***** STATIC VARIABLES *****/

static char     pathname[100];	/* Name of font file to be output */

static fix15    raswid;		/* raster width  */
static fix15    rashgt;		/* raster height */
static fix15    offhor;		/* horizontal offset from left edge of emsquare */
static fix15    offver;		/* vertical offset from baseline */
static fix15    set_width;	/* character set width */
static fix15    y_cur;		/* Current y value being generated and printed */

static char     line_of_bits[2 * MAX_BITS + 1];	/* Buffer for row of generated bits */

ufix16          char_index;
CHARACTERNAME  *char_name;

#if RESTRICTED_ENVIRON
ufix8          *font_ptr;
static ufix16   buffer_size;
#else
font_data      *font_ptr;
#endif

#if WINDOWS_4IN1
unsigned char   big_buffer[10240];	/* temp buffer to store char string */
#endif

FILE           *font_fp;

#define READTIME 0

#if READTIME
GLOBAL_PROTO void dummy_loader(void);
#endif

#if (defined (M_I86CM) || defined (M_I86LM) || defined (M_I86HM) || defined (M_I86SM) || defined (M_I86MM) || defined (__TURBOC__))
  /* then we are using a Microsoft or Borland compiler */
#else
  /* need to have a dummy _fmalloc for unix */

FUNCTION void *_fmalloc(size_t size)
{
}
  /* need to have a dummy _fmemset for unix */
FUNCTION void *_fmemset(void *s, unsigned char c, size_t n)
{
}
  /* need to have a dummy _ffree for unix */
FUNCTION void _ffree(void *ptr)
{
}
#endif

#if DYNAMIC_ALLOC
   SPEEDO_GLOBALS GLOBALFAR *sp_global_ptr;
#endif


FUNCTION int main(int argc, char *argv[])
{
	ufix8           user;
	real            matrix[6];
	ufix32          lines;

	CHARACTERNAME **font_encoding;

#if RESTRICTED_ENVIRON
        ufix16          *encoding_array;
#endif


#if REENTRANT_ALLOC
   SPEEDO_GLOBALS GLOBALFAR *sp_global_ptr;
#endif

#if DYNAMIC_ALLOC || REENTRANT_ALLOC

/* allocate speedo global data structure, if called for: */

if (sizeof(SPEEDO_GLOBALS GLOBALFAR *) > sizeof(char *))
	sp_global_ptr = (SPEEDO_GLOBALS GLOBALFAR *)_fmalloc(sizeof(SPEEDO_GLOBALS));
else
        sp_global_ptr = (SPEEDO_GLOBALS *)malloc(sizeof(SPEEDO_GLOBALS));

if (sizeof(SPEEDO_GLOBALS GLOBALFAR *) > sizeof(char *))
   _fmemset(sp_global_ptr,(ufix8)0,sizeof(SPEEDO_GLOBALS));
else
   memset(sp_global_ptr,(ufix8)0,sizeof(SPEEDO_GLOBALS));

#endif

	/* check for proper usage */

if (argc < 2)
        {
	fprintf(stderr, "Usage: nsample {fontfile} {lines} \n\n");
	exit(1);
	}

	/* initialize the type1 processor */

tr_init(PARAMS1);

sprintf(pathname, argv[1]);

if (argc >= 3)
        {
        sscanf(argv[2],"%ld",&lines);
	if (lines <= 6)
                {
		fprintf(stderr,"invalid pointsize\n");
		exit(1);
                }
        }
else
	lines = 25;

	/* Load Type 1 outline file */

if (!open_font(pathname))
        {
	fprintf(stderr, "Unable to open font file %s\n", pathname);
	exit(1);
        }

#if READTIME

dummy_loader();
exit(1);

#else
#if RESTRICTED_ENVIRON

        /* For the example type 1 font 0003A___.PFB */
        /* The minimum buffer size is 14310 - No charstrings loaded */
        /* The maximum buffer size is 30000 - All charstrings loaded */

buffer_size = 15000;
font_ptr = malloc(buffer_size);

if (tr_load_font(PARAMS2 font_ptr, buffer_size) == FALSE)
	exit(1);

#else

if (tr_load_font(PARAMS2 &font_ptr) == FALSE)
	exit(1);

#endif
#endif				/* dummy loader */


	/* Initialization */

	matrix[0] = (real) lines;
	matrix[1] = (real) 0.0;
	matrix[2] = (real) 0.0;
	matrix[3] = (real) lines;
	matrix[4] = (real) 0.0;
	matrix[5] = (real) 0.0;

#if INCL_GRAY
        sp_globals.pixelSize = 6;       /* Must be set '1<x<9' prior to set_specs() call */
#endif

	if (!tr_set_specs(PARAMS2 (ufix32)MODE_BLACK, matrix, (ufix8 *)font_ptr))
                {
		fprintf(stderr, "Unable to set requested specs.\n");
		exit(1);
                }

#if RESTRICTED_ENVIRON
        /* Standard Encoding? */
        if ( ((font_data *)font_ptr)->encoding_offset == (ufix16)BTSNULL)
                font_encoding = (CHARACTERNAME **)tr_get_encode(PARAMS2 font_ptr);
        else
                encoding_array = (ufix16 *)tr_get_encode(PARAMS2 font_ptr);
#else
	font_encoding = tr_get_encode(PARAMS1);
#endif

	/* output all of the characters in the encoding array */

	for (char_index = 0; char_index < 256; char_index++)
                {

                        /* For each character in encoding */

#if RESTRICTED_ENVIRON
                /* Standard Encoding? */
                if (((font_data *)font_ptr)->encoding_offset == (ufix16)BTSNULL)
                        char_name = (CHARACTERNAME *)font_encoding[char_index];
                else
                        char_name = (CHARACTERNAME *)((ufix32)encoding_array[char_index] + (ufix32)font_ptr);
#else
                char_name = font_encoding[char_index];
#endif
#if NAME_STRUCT
		if (strcmp((char *)char_name->char_name, ".notdef") != 0)
#else
		if (strcmp((char *)char_name, ".notdef") != 0)
#endif
                        {
#if RESTRICTED_ENVIRON
			if (!tr_make_char(PARAMS2 font_ptr, char_name))
#else
			if (!tr_make_char(PARAMS2 char_name))
#endif
				fprintf(stderr, "Unable to make requested character \"%s\"\n", char_name);
#ifdef WANT_KEYPRESS
			fprintf(stderr, "Type Q to quit or any other key to continue: ");
			user = getchar();
			if (user == 'q' || user == 'Q')
				break;
#endif
                        }
                }

	tr_unload_font((font_data *)font_ptr);
	close_font();
	exit(0);

}



FUNCTION void sp_report_error(SPGLOBPTR2 fix15 n)
/*
 * Called by character generator to report an error. 
 *
 * Since character data not available is one of those errors that happens many
 * times, don't report it to user 
 */
{

	switch (n)
        {
	case 1:
		printf("Insufficient font data loaded\n");
		break;

	case 3:
		printf("Transformation matrix out of range\n");
		break;

	case 4:
		printf("Font format error\n");
		break;

	case 5:
		printf("Requested specs not compatible with output module\n");
		break;

	case 7:
		printf("Intelligent transformation requested but not supported\n");
		break;

	case 8:
		printf("Unsupported output mode requested\n");
		break;

	case 9:
		printf("Extended font loaded but only compact fonts supported\n");
		break;

	case 10:
		printf("Font specs not set prior to use of font\n");
		break;

	case 12:
		break;

	case 13:
		printf("Track kerning data not available()\n");
		break;

	case 14:
		printf("Pair kerning data not available()\n");
		break;

	case TR_NO_ALLOC_FONT:
		break;

	case TR_NO_SPC_STRINGS:
		printf("*** Cannot malloc space for charstrings \n");
		break;

	case TR_NO_SPC_SUBRS:
		printf("*** Cannot malloc space for subrs\n");
		break;

	case TR_NO_READ_SLASH:
		printf("*** Cannot read / before charactername in Encoding\n");
		break;

	case TR_NO_READ_STRING:
		printf("*** Cannot read / or end token for CharString\n");
		break;

	case TR_NO_READ_FUZZ:
		printf("*** Cannot read BlueFuzz value\n");
		break;

	case TR_NO_READ_SCALE:
		printf("*** Cannot read BlueScale value\n");
		break;

	case TR_NO_READ_SHIFT:
		printf("*** Cannot read BlueShift value\n");
		break;

	case TR_NO_READ_VALUES:
		printf("*** Cannot read BlueValues array\n");
		break;

	case TR_NO_READ_ENCODE:
		printf("*** Cannot read Encoding index\n");
		break;

	case TR_NO_READ_FAMILY:
		printf("*** Cannot read FamilyBlues array\n");
		break;

	case TR_NO_READ_FAMOTHER:
		printf("*** Cannot read FamilyOtherBlues array\n");
		break;

	case TR_NO_READ_BBOX0:
		printf("*** Cannot read FontBBox element 0\n");
		break;

	case TR_NO_READ_BBOX1:
		printf("*** Cannot read FontBBox element 1\n");
		break;

	case TR_NO_READ_BBOX2:
		printf("*** Cannot read FontBBox element 2\n");
		break;

	case TR_NO_READ_BBOX3:
		printf("*** Cannot read FontBBox element 3\n");
		break;

	case TR_NO_READ_MATRIX:
		printf("*** Cannot read FontMatrix\n");
		break;

	case TR_NO_READ_NAMTOK:
		printf("*** Cannot read FontName / token\n");
		break;

	case TR_NO_READ_NAME:
		printf("*** Cannot read FontName\n");
		break;

	case TR_NO_READ_BOLD:
		printf("*** Cannot read ForceBold value\n");
		break;

	case TR_NO_READ_FULLNAME:
		printf("*** Cannot read FullName value\n");
		break;

	case TR_NO_READ_LANGGRP:
		printf("*** Cannot read LanguageGroup value\n");
		break;

	case TR_NO_READ_OTHERBL:
		printf("*** Cannot read OtherBlues array\n");
		break;

	case TR_NO_READ_SUBRTOK:
		printf("*** Cannot read RD token for subr\n");
		break;

	case TR_NO_READ_STRINGTOK:
		printf("*** Cannot read RD token in charstring\n");
		break;

	case TR_NO_READ_STDHW:
		printf("*** Cannot read StdHW value\n");
		break;

	case TR_NO_READ_STDVW:
		printf("*** Cannot read StdVW value\n");
		break;

	case TR_NO_READ_SNAPH:
		printf("*** Cannot read StemSnapH array\n");
		break;

	case TR_NO_READ_SNAPV:
		printf("*** Cannot read StemSnapV array\n");
		break;

	case TR_NO_READ_BINARY:
		printf("*** Cannot read binary data size for Subr\n");
		break;

	case TR_NO_READ_EXECBYTE:
		printf("*** Cannot read a byte after eexec\n");
		break;

	case TR_NO_READ_CHARNAME:
		printf("*** Cannot read charactername\n");
		break;

	case TR_NO_READ_STRINGBIN:
		printf("*** Cannot read charstring binary data\n");
		break;

	case TR_NO_READ_STRINGSIZE:
		printf("*** Cannot read charstring size\n");
		break;

	case TR_NO_READ_DUPTOK:
		printf("*** Cannot read dup token for subr\n");
		break;

	case TR_NO_READ_ENCODETOK:
		printf("*** Cannot read dup, def or readonly token for Encoding\n");
		break;

	case TR_NO_READ_EXEC1BYTE:
		printf("*** Cannot read first byte after eexec\n");
		break;

	case TR_NO_READ_LENIV:
		printf("*** Cannot read lenIV value\n");
		break;

	case TR_NO_READ_LITNAME:
		printf("*** Cannot read literal name after /\n");
		break;

	case TR_NO_READ_STRINGNUM:
		printf("*** Cannot read number of CharStrings\n");
		break;

	case TR_NO_READ_NUMSUBRS:
		printf("*** Cannot read number of Subrs\n");
		break;

	case TR_NO_READ_SUBRBIN:
		printf("*** Cannot read subr binary data\n");
		break;

	case TR_NO_READ_SUBRINDEX:
		printf("*** Cannot read subr index\n");
		break;

	case TR_NO_READ_TOKAFTERENC:
		printf("*** Cannot read token after Encoding\n");
		break;

	case TR_NO_READ_OPENBRACKET:
		printf("*** Cannot read { or [ in FontBBox\n");
		break;

	case TR_EOF_READ:
		printf("*** End of file read\n");
		break;

	case TR_MATRIX_SIZE:
		printf("*** FontMatrix has wrong number of elements\n");
		break;

	case TR_PARSE_ERR:
		printf("*** Parsing error in Character program string\n");
		break;

	case TR_TOKEN_LARGE:
		printf("*** Token too large\n");
		break;

	case TR_TOO_MANY_SUBRS:
		printf("*** Too many subrs\n");
		break;

	case TR_NO_SPC_ENC_ARR:
		printf("*** Unable to allocate storage for encoding array \n");
		break;

	case TR_NO_SPC_ENC_ENT:
		printf("*** Unable to malloc space for encoding entry\n");
		break;

	case TR_NO_FIND_CHARNAME:
		printf("*** get_chardef: Cannot find char name\n");
		break;

	case TR_INV_FILE:
		printf("*** Not a valid Type1 font file\n");
		break;

	case TR_BUFFER_TOO_SMALL:
		printf("*** Buffer provided too small to store essential data for type1 fonts \n");
		break;

	case TR_BAD_RFB_TAG:
		printf("*** Invalid Tag found in charactaer data\n");
		break;

	default:
		printf("report_error(%d)\n", n);
		break;
	}
}


FUNCTION void sp_open_bitmap(SPGLOBPTR2	fix31 x_set_width,
					fix31 y_set_width,
					fix31 xorg,
					fix31 yorg,
					fix15 xsize,
					fix15 ysize)
/*
 * Called by character generator to initialize a buffer prior to receiving
 * bitmap data. 
 */
{
	fix15           i;

#if DEBUG
	printf("open_bitmap(%3.1f, %3.1f, %3.1f, %3.1f, %d, %d)\n",
	       (real) x_set_width / 65536.0, (real) y_set_width / 65536.0,
	       (real) xorg / 65536.0, (real) yorg / 65536.0, (int) xsize, (int) ysize);
#endif
	raswid = xsize;
	rashgt = ysize;
	offhor = (fix15) (xorg >> 16);
	offver = (fix15) (yorg >> 16);

	if (raswid > MAX_BITS)
		raswid = MAX_BITS;

#if NAME_STRUCT
	printf("\nCharacter index = %d, NAME = %s\n", char_index, char_name->char_name);
#else
	printf("\nCharacter index = %d, NAME = %s\n", char_index, char_name);
#endif
	printf("set width = %3.1f, %3.1f\n", (real) x_set_width / 65536.0, (real) y_set_width / 65536.0);
	printf("open bitmap hex setwidth = %x\n", x_set_width);
	printf("X offset  = %d\n", offhor);
	printf("Y offset  = %d\n\n", offver);
	for (i = 0; i < raswid; i++) {
		line_of_bits[i << 1] = '.';
		line_of_bits[(i << 1) + 1] = ' ';
	}
	line_of_bits[raswid << 1] = '\0';
	y_cur = 0;
}


FUNCTION void sp_set_bitmap_bits(SPGLOBPTR2 fix15 y, fix15 xbit1, fix15 xbit2)
/*
 * Called by character generator to write one row of pixels into the
 * generated bitmap character.                               
 */

{
	fix15           i;

#if DEBUG
	printf("set_bitmap_bits(%d, %d, %d)\n", (int) y, (int) xbit1, (int) xbit2);
#endif

	/*
	 * If xbit1 is negative, set to zero, to keep index in range of
	 * line_of_bits array 
	 */
	if (xbit1 < 0)
		xbit1 = 0;

	/* Clip runs beyond end of buffer */
	if (xbit1 > MAX_BITS)
		xbit1 = MAX_BITS;

	if (xbit2 > MAX_BITS)
		xbit2 = MAX_BITS;

	/* Output backlog lines if any */
	while (y_cur != y)
            {
	    printf("    %s\n", line_of_bits);

            for (i = 0; i < raswid; i++)
                {
                line_of_bits[i << 1]   = '.';
                line_of_bits[(i<<1)+1] = ' ';
		}

            y_cur++;
            }

	/* Add bits to current line */
	for (i = xbit1; i < xbit2; i++)
            {
            if (sp_globals.output_mode == MODE_GRAY)
                {
                line_of_bits[i << 1]   = 'F';
                line_of_bits[(i<<1)+1] = 'F';
                }
            else
		line_of_bits[i << 1] = 'X';
            }

}

FUNCTION void sp_set_bitmap_pixels(SPGLOBPTR2 fix15 y, fix15 xbit1, fix15 num_grays, void *gray_buf)
/* Called by output module to write one row of gray pixels into the generated bitmap character. */
{
ufix8 hex_array[] = {'0', '1', '2', '3',
                     '4', '5', '6', '7',
                     '8', '9', 'a', 'b',
                     'c', 'd', 'e', 'f'};

fix15 i;
fix15 xbit2;

        /* Define xbit2 */

    xbit2 = xbit1 + num_grays;


#if DEBUG
    printf("set_bitmap_pixels(%d, %d, %d)\n", (int)y, (int)xbit1, (int)xbit2);
#endif
    
        /* Clip runs beyond end of buffer */

    if (xbit1 > MAX_BITS)
        xbit1 = MAX_BITS;

    if (xbit2 > MAX_BITS)
        xbit2 = MAX_BITS;

        /* Output backlog lines if any */

    while (y_cur != y)
        {
        printf("    %s\n", line_of_bits);

        for (i = 0; i < raswid; i++)
            {
            line_of_bits[i << 1]   = '.';
            line_of_bits[(i<<1)+1] = ' ';
            }

        y_cur++;
        }

        /* Add bits to current line */

    for (i = xbit1; i < xbit2; i++)
        {
        line_of_bits[i << 1] = hex_array[((ufix8*)gray_buf)[i-xbit1] >> 4];
        line_of_bits[(i<<1) + 1] = hex_array[((ufix8*)gray_buf)[i-xbit1] & 0x0F];
        }

}

FUNCTION void sp_close_bitmap(SPGLOBPTR1)
/*
 * Called by character generator to indicate all bitmap data has been
 * generated. 
 */
{

#if DEBUG
	printf("close_bitmap()\n");
#endif
	printf("    %s\n", line_of_bits);
}


#if INCL_OUTLINE

FUNCTION void sp_open_outline(SPGLOBPTR2 fix31 x_set_width,
					 fix31 y_set_width,
                                         fix31 xmin, fix31 xmax,
                                         fix31 ymin, fix31 ymax)
/*
 * Called by character generator to initialize prior to outputting scaled
 * outline data. 
 */
{
	printf("\nopen_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
	       (real) x_set_width / 65536.0, (real) y_set_width / 65536.0,
	       (real) xmin / 65536.0, (real) xmax / 65536.0, (real) ymin / 65536.0, (real) ymax / 65536.0);
}



FUNCTION void sp_start_new_char(SPGLOBPTR1)
/*
 * Called by character generator to initialize prior to outputting scaled
 * outline data for a sub-character in a compound character. 
 */
{
	printf("start_new_char()\n");
}


FUNCTION void sp_start_contour(SPGLOBPTR2 fix31 x, fix31 y, btsbool outside)
/*
 * Called by character generator at the start of each contour in the outline
 * data of the character. 
 */
{
	printf("start_contour(%3.1f, %3.1f, %s)\n",
	       (real) x / 65536.0, (real) y / 65536.0,
	       outside ? "outside" : "inside");
}


FUNCTION void sp_curve_to(SPGLOBPTR2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
/*
 * Called by character generator onece for each curve in the scaled outline
 * data of the character. This function is only called if curve output is
 * enabled in the set_specs() call. 
 */
{
	printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
	       (real) x1 / 65536.0, (real) y1 / 65536.0,
	       (real) x2 / 65536.0, (real) y2 / 65536.0,
	       (real) x3 / 65536.0, (real) y3 / 65536.0);
}


FUNCTION void sp_line_to(SPGLOBPTR2 fix31 x, fix31 y)
/*
 * Called by character generator onece for each vector in the scaled outline
 * data for the character. This include curve data that has been sub-divided
 * into vectors if curve output has not been enabled in the set_specs() call. 
 */
{
	printf("line_to(%3.1f, %3.1f)\n",
	       (real) x / 65536.0, (real) y / 65536.0);
}



FUNCTION void sp_close_contour(SPGLOBPTR1)
/*
 * Called by character generator at the end of each contour in the outline
 * data of the character. 
 */
{
	printf("close_contour()\n");
}


FUNCTION void sp_close_outline(SPGLOBPTR1)
/*
 * Called by character generator at the end of output of the scaled outline
 * of the character. 
 */
{
	printf("close_outline()\n");
}

#endif


FUNCTION fix15 read_2b(ufix8 *pointer)
/*
 * Reads 2-byte field from font buffer 
 */
{
	fix15           temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer);
	return temp;
}



FUNCTION fix31 read_4b(ufix8 *pointer)
/*
 * Reads 4-byte field from font buffer 
 */
{
	fix31           temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer);
	return temp;
}

FUNCTION btsbool open_font(char *pathname)
{
	font_fp = fopen(pathname, "rb");
	if (font_fp == BTSNULL)
		return FALSE;
	return TRUE;
}

FUNCTION fix31 t1_get_bytes(void *buffer, fix31 offset, fix31 length)
{

	if (fseek(font_fp, offset, 0))
        {
		printf("Unable to do fseek on font file\n");
		exit (1);
	}

        if ((length = (fix31)fread(buffer, 1, (size_t)length, font_fp)) == 0)
        {
                printf("Unable to read font file\n");
                exit (1);
        }

        return (length);
}

#if RESTRICTED_ENVIRON
FUNCTION ufix8 *t1_dynamic_load(ufix32 file_position, fix15 num_bytes)
{
        void    *buffer;

                /* Allocate num_bytes buffer */

        if (!(buffer = malloc((size_t)num_bytes)))
        {
                printf("Unable to allocate buffer\n");
                exit(1);
        }

                /* Read in the data bytes */

        if (!t1_get_bytes(buffer, (fix31)file_position, (fix31)num_bytes))
        {
                printf("Unable to execute a dynamic load\n");
                exit(1);
        }

        return (ufix8 *)buffer;
}

#if WINDOWS_4IN1
FUNCTION unsigned char STACKFAR* WDECL dynamic_load(ufix32 file_position, fix15 num_bytes, ufix8 success)
{
	fix15           i;


	if (fseek(font_fp, (long) file_position, 0))
        {
		printf("Unable to do fseek on font file\n");
		exit(1);
	}
	if (num_bytes > 10240)
        {
		printf("Number of bytes dynamically requested greater than max\n");
		exit(1);
	}
	for (i = 0; i < num_bytes; i++)
		big_buffer[i] = getc(font_fp);

	return big_buffer;	/* return addr of big buffer */
}

#endif  /* WINDOWS_4IN1 */
#endif  /* RESTRICTED_ENVIRON */


FUNCTION void close_font(void)
{
	fclose(font_fp);
}


#if  READTIME

FUNCTION void dummy_loader(void)
{
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
	struct timeb    ElapseBegin, ElapseEnd;
	long            CpuBegin, CpuEnd;
	long            seconds, result_seconds, result_milliseconds;
	float           real_time;
	ufix8           byte;

	/* get the system clock */
	ftime(&ElapseBegin);

	/* get the cpu clock */
	CpuBegin = clock();

	while (get_byte(&byte));

	/* get the cpu clock end time */
	CpuEnd = clock();

	/* get the system end time */
	ftime(&ElapseEnd);
	result_seconds = ElapseEnd.time - ElapseBegin.time;
	result_milliseconds = ElapseEnd.millitm - ElapseBegin.millitm;
	real_time = (float) result_seconds + (((float) result_milliseconds) / 1000.0);

	/* print out the results */
	printf("Total elapse time is  %f \n", real_time);
	printf("Total cpu time is     %f \n", (double) (CpuEnd - CpuBegin) / 1000000.0);
}
#endif
