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
/*************************  T R _ L D F N T . C ******************************
 *                                                                           *
 * This is the Type A font loader for testing PS QEM 2.0                     *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 * $Header: //bliss/user/archive/rcs/type1/tr_ldfnt.c,v 33.0 97/03/17 17:59:51 shawn Release $
 *
 * $Log:	tr_ldfnt.c,v $
 * Revision 33.0  97/03/17  17:59:51  shawn
 * TDIS Version 6.00
 * 
 * Revision 32.6  96/07/02  12:50:38  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 32.5  96/06/10  17:44:15  shawn
 * Cast parameters for proper K&R functionality.
 * 
 * Revision 32.4  96/06/05  15:14:23  shawn
 * Major rewrite for performance (Behne & Behne).
 * Fixed RESTRICTED_ENVIRON conditional.
 * 
 * Revision 32.3  96/03/22  11:36:29  shawn
 * Changed to 'const char *s' in sscanf() prototype.
 * 
 * Revision 32.2  96/01/31  14:14:52  shawn
 * Added PROTOTYPE statements and made code ANSI to eliminate compiler warnings.
 * 
 * Revision 32.1  95/11/17  11:04:15  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 32.0  95/02/15  16:46:18  shawn
 * Release
 * 
 * Revision 31.4  95/01/25  13:17:38  shawn
 * Changed LPS1/2 MACROs to LPSFAR MACRO
 * 
 * Revision 31.3  95/01/10  09:25:57  roberte
 * Updated copyright header.
 * 
 * Revision 31.2  95/01/05  17:08:30  shawn
 * Remove LOCAL_PROTO for tr_error() which exists in tr_data.h
 * 
 * Revision 31.1  95/01/04  16:37:18  roberte
 * Release
 * 
 * Revision 30.4  94/12/16  15:20:42  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 30.3  94/10/25  09:56:52  roberte
 * Remove include of stdef.h. Data types now seen in nested include of btstypes.h from spe
 * Remove include of stdef.h. Data types now seen in nested include of btstypes.h from speedo.h.
 * 
 * Revision 30.2  94/08/19  10:38:59  roberte
 * Correct pointer references to current_font->full_name and current_font->font_name.
 * 
 * Revision 30.1  94/08/19  09:38:52  roberte
 * Added strcpy of font+name and full_name to the SPEEDO_GLOBALS
 * members from the locl_font_ptr. These had been empty.
 * 
 * Revision 30.0  94/05/04  09:30:26  roberte
 * Release
 * 
 * Revision 29.0  94/04/08  12:10:21  roberte
 * Release
 * 
 * Revision 28.35  94/02/24  15:19:57  mark
 * fix previous fix so that it works!
 * 
 * Revision 28.34  94/02/24  11:09:49  mark
 * make sure current_font element of sp_globals is set for 
 * not-RESTRICTED_ENVIRON configurations.
 * 
 * Revision 28.33  94/02/03  12:41:12  roberte
 * Moved some local prototypes to head of file where TurboC likes them better.
 * 
 * Revision 28.32  93/10/12  12:55:33  roberte
 * Added more explicit casting of some assignments and parameters passed to functions.
 * 
 * Revision 28.31  93/09/16  14:34:45  roberte
 * Corrected calculation of rtemp, used to check zero bounding box.
 * This would have falsely detected zero box in some cases.
 * Now sum the width and depth.
 * 
 * Revision 28.30  93/08/30  15:44:41  roberte
 * Release
 * 
 * Revision 28.27  93/05/28  09:18:01  roberte
 * Added #if PROC_TYPE1 conditional around entire file, after include of spdo_prv.h
 * 
 * Revision 28.26  93/05/06  12:14:46  roberte
 * Replaced missing comma in function prototype- a typo.
 * 
 * Revision 28.25  93/04/23  17:54:29  roberte
 * Moved function prototypes outside of #if RESTRICTED_ENVIRONMENT mess.
 * They were plopped in the wrong place, and RESTRICTED_ENVIRONMENT ended
 * up missing an #endif.
 * Also removed extraneous prototype of tr_read_long()
 * 
 * Revision 28.24  93/03/15  13:10:44  roberte
 * Release
 * 
 * Revision 28.17  93/03/11  15:47:59  roberte
 * Changed #if __MSDOS to #ifdef MSDOS.
 * 
 * Revision 28.16  93/03/09  13:08:18  roberte
 * Tended to #include files, various platforms.
 * 
 * Revision 28.15  93/03/09  12:16:29  roberte
 *  Replaced #if INTEL tests with #ifdef MSDOS as appropriate.
 * 
 * Revision 28.14  93/01/21  13:22:59  roberte
 * Reentrant code work.  Added macros to support sp_global_ptr parameter pass in all essential call threads.
 * 
 * Revision 28.13  93/01/14  10:13:07  roberte
 * Changed all data references to sp_globals.processor.type1.<varname> since these are all part of union structure there. 
 * 
 * Revision 28.12  93/01/04  17:22:43  roberte
 * Changed all the report_error calls back to sp_report_error to 
 * be in line with the spdo_prv.h changes.
 * 
 * Revision 28.11  92/12/28  11:22:39  roberte
 * Correctly typed tr_error() return value as void.
 * 
 * Revision 28.10  92/12/15  13:00:06  roberte
 * Added #if PROTOS_AVAIL conditional around prototype of compstr().
 * Changed read_long() to tr_read_long() so won't conflict with
 * sp_read_long() macros.
 * 
 * Revision 28.9  92/12/02  11:50:16  laurar
 * change calls to sp_report_error to report_error, so that the function will
 * be called properly with the DLL.
 * 
 * Revision 28.8  92/11/24  13:15:16  laurar
 * include fino.h
 * 
 * Revision 28.7  92/11/19  15:35:16  weili
 * Release
 * 
 * Revision 26.9  92/11/18  18:55:50  laurar
 * Add STACKFAR.
 * 
 * Revision 26.8  92/11/16  18:28:46  laurar
 * Add STACKFAR for Windows; add #ifdef's for functions that Windows calls
 * differently; add function compstr() which calls the appropriate strcmp
 * library function depending on the platform.
 * 
 * Revision 26.7  92/10/21  09:58:24  davidw
 * Turned off DEBUG
 * 
 * Revision 26.6  92/10/19  09:36:25  davidw
 * Changed the default FontBBox values to -2000,-2000 2000, 2000 as per Adobe Type 1 Font Book
 * To fix bug of all zero FontBBox.
 * 
 * Revision 26.5  92/10/16  16:40:28  davidw
 * WIP: working on t1 bug, not done yet
 * 
 * Revision 26.4  92/10/01  12:12:46  laurar
 * include stdio.h for PC (NULL is defined there);
 * initialize local_font_ptr->no_subrs, so if font has to 
 * be unloaded, it will not free unallocated memory.
 * 
 * Revision 26.3  92/09/28  16:45:10  roberte
 * Changed "#include "fnt.h" to "#include "fnt_a.h".
 * Same include file needs different name for 4in1.
 * 
 * Revision 26.2  92/07/22  21:29:09  ruey
 * take out redundant define for FONTNAMESIZE. 
 * 
 * Revision 26.1  92/06/26  10:25:49  leeann
 * Release
 * 
 * Revision 25.1  92/04/06  11:42:13  leeann
 * Release
 * 
 * Revision 24.1  92/03/23  14:10:29  leeann
 * Release
 * 
 * Revision 23.2  92/03/23  11:49:16  leeann
 * accept empty name strings
 * consider replaced characters in read_string (allows a space to
 * exist between strings)
 * 
 * 
 * Revision 23.1  92/01/29  17:01:29  leeann
 * Release
 * 
 * Revision 22.2  92/01/28  14:28:43  leeann
 * support strangely constructed fonts by accepting the first
 * font BBox encountered, regardless of whether it is in the
 * font dictionary or the private dictionary.
 * 
 * Revision 22.1  92/01/20  13:32:52  leeann
 * Release
 * 
 * Revision 21.1  91/10/28  16:45:18  leeann
 * Release
 * 
 * Revision 20.1  91/10/28  15:28:59  leeann
 * Release
 * 
 * Revision 18.2  91/10/22  15:59:14  leeann
 * support radix numbers
 * 
 * Revision 18.1  91/10/17  11:40:34  leeann
 * Release
 * 
 * Revision 17.5  91/10/07  09:55:36  leeann
 * fix to add_encoding for RESTRICTED_ENVIRON
 * 
 * Revision 17.4  91/09/24  16:46:11  leeann
 * when unloading font - don't try to free staticly allocated notdef
 * 
 * Revision 17.3  91/09/24  16:16:44  leeann
 * add tr_get_leniv function
 * 
 * Revision 17.2  91/09/24  11:50:00  leeann
 * allow four bytes following eexec to be > 126
 * 
 * Revision 17.1  91/06/13  10:45:12  leeann
 * Release
 * 
 * Revision 16.2  91/06/13  10:26:20  leeann
 * malloc and fill encoding structures properly when
 * NAME_STRUCT is on
 * 
 * Revision 16.1  91/06/04  15:35:51  leeann
 * Release
 * 
 * Revision 15.4  91/06/04  15:23:57  leeann
 * add RESTRICTED_ENVIRON functions to replace sscanf
 * 
 * Revision 15.3  91/05/15  14:05:12  leeann
 * declare sp_globals.processor.type1.current_font to be extern
 * 
 * Revision 15.2  91/05/13  13:54:05  leeann
 * put fnt_file_type default assignment inside PFB conditional
 * 
 * Revision 15.1  91/05/08  18:08:00  leeann
 * Release
 * 
 * Revision 14.1  91/05/07  16:29:48  leeann
 * Release
 * 
 * Revision 13.3  91/05/07  13:49:50  leeann
 * initialize font type to PFA
 * 
 * Revision 13.2  91/05/06  09:51:39  leeann
 * fix get encoding array for RESTRICTED_ENVIRON
 * 
 * Revision 13.1  91/04/30  17:04:29  leeann
 * Release
 * 
 * Revision 12.1  91/04/29  14:54:51  leeann
 * Release
 * 
 * Revision 11.5  91/04/24  17:47:21  leeann
 * read the useropt.h file
 * 
 * Revision 11.4  91/04/24  10:37:40  leeann
 * change default values for bluescale, blueshift, and bluefuzz
 * 
 * Revision 11.3  91/04/23  10:43:48  leeann
 * support Hybrid fonts
 * 
 * Revision 11.2  91/04/10  13:19:23  leeann
 *  support character names as structures
 * 
 * Revision 11.1  91/04/04  10:58:26  leeann
 * Release
 * 
 * Revision 10.4  91/04/03  17:53:04  leeann
 * make tag_bytes a fix31
 * 
 * Revision 10.3  91/03/26  09:51:00  leeann
 * fix index on read binary function
 * 
 * Revision 10.2  91/03/22  15:57:59  leeann
 * clean up code, performance improvements
 * 
 * Revision 10.1  91/03/14  14:30:47  leeann
 * Release
 * 
 * Revision 9.2  91/03/14  14:17:02  leeann
 * fixup function declarations, global variables
 * 
 * Revision 9.1  91/03/14  10:06:17  leeann
 * Release
 * 
 * Revision 8.7  91/03/13  17:32:32  leeann
 * add support for RESTRICTED_ENVIRON
 * 
 * Revision 8.6  91/02/20  16:08:52  leeann
 * make PFB support conditional with INCL_PFB flag
 * 
 * Revision 8.5  91/02/20  09:03:25  joyce
 * *** empty log message ***
 * 
 * Revision 8.4  91/02/14  10:40:20  joyce
 * Optimized the pfb code
 * 
 * Revision 8.3  91/02/13  16:05:56  joyce
 * Provided loader with the ability to load .pfb files
 * Major changes:
 * (1) Added new function: get_tag(), which reads the first
 *     six bytes of each binary/ascii section of the file
 *     and determines the mode (ascii or binary) and the
 *     number of bytes for the next section
 * (2) Changed the next_byte function to accomodate .pfb
 *     files by converting binary bytes to ascii.
 * 
 * Revision 8.2  91/01/31  13:46:13  leeann
 * read int as short in "read_int" function
 * 
 * Revision 8.1  91/01/30  19:03:11  leeann
 * Release
 * 
 * Revision 7.3  91/01/30  18:54:11  leeann
 * clarify integer sizes
 * 
 * Revision 7.2  91/01/22  14:38:22  leeann
 * correct spelling of include ctypes=>ctype
 * 
 * Revision 7.1  91/01/22  14:27:23  leeann
 * Release
 * 
 * Revision 6.5  91/01/22  14:20:13  leeann
 * include file ctypes.h added
 * 
 * Revision 6.4  91/01/17  19:16:29  joyce
 * Made standard_encoding function global (removed static declaration)
 * to be called by an application. The function now checks whether
 * a special encoding array has aleady been allocated, and if so,
 * deallocates it.
 * 
 * Revision 6.3  91/01/17  18:32:03  joyce
 * added code to tr_set_encode - if unable to allocate
 * memory for encoding array at any point, then deallocate
 * all memory allocated up to that point, and free encoding pointer
 * 
 * Revision 6.2  91/01/16  18:20:23  joyce
 * fixed tr_unload_font to free all allocated memory
 * in the font structure, and to check whether a pointer
 * is valid before calling free
 * 
 * Revision 6.1  91/01/16  10:53:26  leeann
 * Release
 * 
 * Revision 5.9  91/01/15  17:51:55  leeann
 * turn off timer
 * 
 * Revision 5.8  91/01/15  17:21:19  joyce
 * Added new function, tr_error, which:
 *   1. Calls sp_report_error with error code
 *   2. Calls unload_font to free any memory that might have
 *      been allocated
 * All calls to sp_report_error have been changed to tr_error
 * 
 * Revision 5.7  91/01/10  18:34:20  joyce
 * Fixed error with ".notdef" string in tr_set_encode
 * 
 * Revision 5.6  91/01/10  12:08:00  leeann
 * put font definition in include file, fix get_byte bug
 * 
 * Revision 5.5  91/01/10  11:23:05  joyce
 * 1. Changed direct _sPrintf error messages to calls to sp_report_error
 * 2. Added 3 functions: tr_set_encode, tr_get_encode, tr_unload_font
 * 
 * Revision 5.4  91/01/07  19:55:11  leeann
 * optimize, remove static buffers, remove file access code
 * 
 * Revision 5.3  91/01/07  10:59:52  joyce
 * optimized switch statement
 * 
 * Revision 5.2  90/12/26  16:58:20  leeann
 * put timing code into the loader
 * 
 * Revision 5.1  90/12/12  17:20:05  leeann
 * Release
 * 
 * Revision 4.2  90/12/12  17:15:01  leeann
 * fix syntax error, change STORAGESIZE
 * 
 * Revision 4.1  90/12/12  14:45:49  leeann
 * Release
 * 
 * Revision 3.2  90/12/06  15:22:31  leeann
 * declare malloc, set font type to binary in fopen
 * 
 * Revision 3.1  90/12/06  10:28:14  leeann
 * Release
 * 
 * Revision 2.1  90/12/03  12:56:55  mark
 * Release
 * 
 * Revision 1.2  90/12/03  12:21:49  joyce
 * Changed include line to reference new include file names:
 * fnt_a.h -> fnt.h, ps_qem.h -> type1.h
 * 
 * Revision 1.1  90/11/30  11:27:46  joyce
 * Initial revision
 * 
 * Revision 1.4  90/11/29  15:14:06  leeann
 * change function names, allow multiple font load
 * 
 * Revision 1.3  90/11/19  15:50:11  joyce
 * changed function names to fit spec
 * 
 * Revision 1.2  90/09/17  13:27:34  roger
 * put in rcsid[] for RCS and put in a ";" so that a 
 * goto will work on the pc
 * 
 * Revision 1.1  90/08/13  15:27:10  arg
 * Initial revision
 * 
 *                                                                           *
 *  1) 14 Mar 90  jsc  Created                                               *
 *                                                                           *
 *  2) 30 Mar 90  jsc  Modified to accept either hex or binary eexec         *
 *                     encrypted data                                        *
 *                                                                           *
 *  3) 23 Jul 90  jsc  Modified put_binary() to defer execution of character *
 *                     string decryption to run time.                        *
 *                                                                           *
 ****************************************************************************/

static char     rcsid[] = "$Header: //bliss/user/archive/rcs/type1/tr_ldfnt.c,v 33.0 97/03/17 17:59:51 shawn Release $";

#define   _TYPE1_       /* define this constant so that the */
#include "spdo_prv.h"    /* speedo read functions do not get */
#undef     _TYPE1_      /* substituted for the type1 read functions. */

#if PROC_TYPE1
#include "fino.h"
#include "type1.h"

#ifdef MSDOS
#include <stdio.h>
#endif

#include <Debug.h>

/* you need one of the following includes, depending on your system: */
#if defined(__STDC__) || defined(vms)
#include <string.h>
#else
#include <memory.h>
#endif

#include <math.h>
#include "fnt_a.h"
#include "errcodes.h"
#include "tr_fdata.h"
#include <ctype.h>

#if TIMEIT
#include <sys/types.h>
#endif

#ifdef	MSDOS
#define	LPSFAR  far
#else
#define	LPSFAR
#endif

#ifndef DEBUG
#define DEBUG   0
#endif

#ifndef RESTRICTED_DEBUG
#define RESTRICTED_DEBUG        0
#endif

#ifndef CHARSTRING_DEBUG
#define CHARSTRING_DEBUG        0
#endif

#if   WINDOWS_4IN1
#include <windows.h>
#endif

#if DEBUG
#include <stdio.h>
#define SHOW(X) _sPrintf("    X = %d\n", X)
#else
#define SHOW(X)
#endif

/* Flag bit assignments for character type table */
#define CTRL_CHAR  1
#define SPACE_CHAR 2
#define PUNCT_CHAR 4
#define HEX_DIGIT  8
#define EOL_CHAR  16
#define OTHER_CHAR 0

#if RESTRICTED_ENVIRON
#define MAX_CHARSTRINGS_LOADED 256	/* Maximum number of charstrings to load */
#define MAX_SUBRS_LOADED       500	/* Maximum number of subrs to load */
#endif

#define CHARNAMESIZE   32	/* Maximum character name size */

extern unsigned char *charname_tbl[];	/* Extended encoding table */

static ufix16   decrypt_c1 = 52845;
static ufix16   decrypt_c2 = 22719;

#define DECRYPT_C1      ((ufix16) 52845L)
#define DECRYPT_C2      ((ufix16) 22719L)

/* Character type table for parsing */
static fix15    char_type[] =
{
 CTRL_CHAR,			/* 00 */
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 SPACE_CHAR,			/* Tab */
 SPACE_CHAR | EOL_CHAR,		/* Line feed */
 CTRL_CHAR,
 CTRL_CHAR,
 SPACE_CHAR | EOL_CHAR,		/* Carriage return */
 CTRL_CHAR,
 CTRL_CHAR,

 CTRL_CHAR,			/* 10 */
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,
 CTRL_CHAR,

 SPACE_CHAR,			/* 20 Space */
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,			/* 23 numbersign */
 OTHER_CHAR,
 PUNCT_CHAR,			/* 25 % */
 OTHER_CHAR,
 OTHER_CHAR,
 PUNCT_CHAR,			/* 28 ( */
 PUNCT_CHAR,			/* 29 ) */
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 PUNCT_CHAR,			/* 2F / */

 OTHER_CHAR | HEX_DIGIT,	/* 30 0 */
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR,
 OTHER_CHAR,
 PUNCT_CHAR,			/* 3C < */
 OTHER_CHAR,
 PUNCT_CHAR,			/* 3E > */
 OTHER_CHAR,

 OTHER_CHAR,			/* 40 @ */
 OTHER_CHAR | HEX_DIGIT,	/* 41 A */
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,

 OTHER_CHAR,			/* 50 P */
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 PUNCT_CHAR,			/* 5B [ */
 OTHER_CHAR,
 PUNCT_CHAR,			/* 5D ] */
 OTHER_CHAR,
 OTHER_CHAR,

 OTHER_CHAR,			/* 60 ' */
 OTHER_CHAR | HEX_DIGIT,	/* 61 a */
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR | HEX_DIGIT,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,

 OTHER_CHAR,			/* 70 p */
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 OTHER_CHAR,
 PUNCT_CHAR,			/* 7B { */
 OTHER_CHAR,
 PUNCT_CHAR,			/* 7D } */
 OTHER_CHAR,
 CTRL_CHAR,
};

#ifdef OLDWAY
/* global pointer to current_font */
extern font_data STACKFAR *current_font;
#endif

#if NAME_STRUCT
static CHARACTERNAME notdef_struct = {7, (unsigned char *) ".notdef"};
static CHARACTERNAME *notdef = &notdef_struct;
#define CHARSIZE sizeof(CHARACTERNAME)+strlen(charactername)
#else
static CHARACTERNAME *notdef = (CHARACTERNAME *) ".notdef";
#define CHARSIZE strlen(charactername)+1
#endif

/* external functions used: */
GLOBAL_PROTO fix31 t1_get_bytes(font_data *font, void *buffer, fix31 offset, fix31 length);

GLOBAL_PROTO void *malloc(size_t size);
GLOBAL_PROTO void  free(void *ptr);

/* local non-static functions */
GLOBAL_PROTO char STACKFAR *tr_get_font_name(SPGLOBPTR1);
GLOBAL_PROTO char STACKFAR *tr_get_full_name(SPGLOBPTR1);
GLOBAL_PROTO char STACKFAR *tr_get_family_name(SPGLOBPTR1);

GLOBAL_PROTO unsigned char STACKFAR* tr_get_subr(SPGLOBPTR2 int i);
GLOBAL_PROTO unsigned char STACKFAR* tr_get_chardef(SPGLOBPTR2 CHARACTERNAME STACKFAR *charname);

GLOBAL_PROTO btsbool tr_get_isFixedPitch(SPGLOBPTR1);

GLOBAL_PROTO fix15 tr_get_UnderlinePostition(SPGLOBPTR1);
GLOBAL_PROTO fix15 tr_get_UnderlineThickness(SPGLOBPTR1);
GLOBAL_PROTO fix15 tr_get_paint_type(SPGLOBPTR1);
GLOBAL_PROTO fix15 tr_get_leniv(SPGLOBPTR1);
GLOBAL_PROTO fix15 read_subrs(SPGLOBPTR2 font_data STACKFAR *font_ptr, char *buffer, fix15 ignore_subrs);

GLOBAL_PROTO void tr_get_font_matrix(SPGLOBPTR2 real STACKFAR *matrix);

GLOBAL_PROTO font_hints_t STACKFAR* tr_get_font_hints(SPGLOBPTR1);
GLOBAL_PROTO fbbox_t STACKFAR* tr_get_font_bbox(SPGLOBPTR1);

#if RESTRICTED_ENVIRON
#if WINDOWS_4IN1
GLOBAL_PROTO unsigned char STACKFAR* WDECL dynamic_load(ufix32 file_position, fix15 num_bytes, ufix8 success);
#endif
GLOBAL_PROTO ufix8 *t1_dynamic_load(ufix32 file_position, fix15 num_bytes);

GLOBAL_PROTO char STACKFAR* STACKFAR* WDECL tr_get_encode(SPGLOBPTR2 ufix8 STACKFAR *font_ptr);
GLOBAL_PROTO btsbool tr_set_encode(SPGLOBPTR2 ufix8 STACKFAR *font_ptr, char STACKFAR *set_array[256]);
#else
GLOBAL_PROTO CHARACTERNAME **tr_get_encode(SPGLOBPTR1);
GLOBAL_PROTO int tr_set_encode(SPGLOBPTR2 CHARACTERNAME STACKFAR *set_array[256]);
#endif

#define compstr( buff, string ) ((short) strcmp((char *) buff, (char *) string))

/* static function prototypes: */

LOCAL_PROTO btsbool inquire_about_space(font_data STACKFAR *font_ptr, ufix16 space_needed);
LOCAL_PROTO btsbool base2decimal(fix15 base, char STACKFAR *buffer, fix15 STACKFAR *number);
LOCAL_PROTO btsbool read_int(SPGLOBPTR2 fix15 STACKFAR *pdata);
LOCAL_PROTO btsbool tr_read_long(SPGLOBPTR2 fix31 STACKFAR *pdata);
LOCAL_PROTO btsbool read_long_array(SPGLOBPTR2 fix31 STACKFAR *pdata, fix15 STACKFAR *pn);
LOCAL_PROTO btsbool read_real(SPGLOBPTR2 real STACKFAR *pdata);
LOCAL_PROTO btsbool read_real_array(SPGLOBPTR2 real STACKFAR *pdata, fix15 STACKFAR *pn);
LOCAL_PROTO btsbool read_btsbool(SPGLOBPTR2 btsbool STACKFAR *pdata);
LOCAL_PROTO btsbool read_byte(font_data STACKFAR *font_ptr, ufix8 STACKFAR *pbyte);
LOCAL_PROTO btsbool parse_tag(font_data STACKFAR *font_ptr, fix31 *tag_bytes, ufix8 STACKFAR *tag_string);
LOCAL_PROTO btsbool add_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr, fix15 index, ufix8 STACKFAR *charactername);

LOCAL_PROTO int do_command(ufix16 command);
LOCAL_PROTO int do_operand(fix31 operand);

LOCAL_PROTO void clear_hints(font_data STACKFAR *font_ptr);
LOCAL_PROTO void asc2bin_buffer(fix15 byte_count, unsigned char STACKFAR *char_data);
LOCAL_PROTO void set_file_pos(font_data STACKFAR *font_ptr, fix31 offset);

LOCAL_PROTO fix31 get_bytes(font_data STACKFAR *font_ptr, ufix8 *dest, ufix16 len);
LOCAL_PROTO fix31 get_file_pos(font_data STACKFAR *font_ptr);

LOCAL_PROTO fix15 check_tag(font_data STACKFAR *font_ptr);
LOCAL_PROTO fix15 skip_binary(font_data STACKFAR *font_ptr, fix15 count);
LOCAL_PROTO fix15 skip_rest_of_token(font_data STACKFAR *font_ptr);
LOCAL_PROTO fix15 get_byte(font_data STACKFAR *font_ptr, ufix8 *byte);
LOCAL_PROTO fix15 read_token(SPGLOBPTR2 char STACKFAR *buf, fix15 count, fix15 ignore_space_chars);
LOCAL_PROTO fix15 read_binary(font_data STACKFAR *font_ptr, char STACKFAR *buf, fix15 count);
LOCAL_PROTO fix15 read_string(font_data STACKFAR *font_ptr, char STACKFAR *buf, fix15 count);
LOCAL_PROTO fix15 asctohex(ufix8 asciivalue);
LOCAL_PROTO fix15 read_plain_token(font_data STACKFAR *font_ptr, char STACKFAR *buf, fix15 count, fix15 ignore_space_chars);
LOCAL_PROTO fix15 read_encrypted_token(font_data STACKFAR *font_ptr, char STACKFAR *buf, fix15 count, fix15 ignore_space_chars);
LOCAL_PROTO fix15 read_and_decrypt(font_data STACKFAR *font_ptr, ufix8 STACKFAR *buf, fix15 len);

LOCAL_PROTO ufix8 decrypt_byte(font_data STACKFAR *font, ufix8 byte);
LOCAL_PROTO ufix8 *fill_get_byte_buf(font_data STACKFAR *font_ptr);


#if RESTRICTED_ENVIRON
LOCAL_PROTO btsbool clear_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr);
LOCAL_PROTO btsbool get_space(SPGLOBPTR2 ufix16 space_needed);
LOCAL_PROTO void unload_charstring(SPGLOBPTR1);

#if INCL_PFB
LOCAL_PROTO btsbool remove_tags(SPGLOBPTR2 fix31 tag_position, ufix8 STACKFAR *char_data, fix15 total_bytes);
#endif

#else
LOCAL_PROTO void clear_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr);
#endif

#if DEBUG
LOCAL_PROTO void print_binary(SPGLOBPTR2 ufix8 *buf, fix15 n);
LOCAL_PROTO void print_encoding(font_data STACKFAR *font_ptr);
LOCAL_PROTO void print_long_array(fix31 data[], fix15 n);
LOCAL_PROTO void print_real_array(real data[], int n);
#endif

#if RESTRICTED_ENVIRON

FUNCTION long latol(char LPSFAR *lpS, short STACKFAR *nChars)
{
	long            r = 0, s = 1;
	short           len = 0;

	if (*lpS == '-') {
		s = -1;
		++lpS;
		len++;
	}
	while (*lpS >= '0' && *lpS <= '9') {
		r *= 10;
		r += *lpS++ - '0';
		len++;
	}

	if (nChars)
		*nChars = len;

	return r * s;
}

FUNCTION float latof(char LPSFAR *lpS)
{
	float           tmp1, tmp2;
	short           i;

	tmp1 = (float) latol(lpS, (short STACKFAR*)&i);

	lpS += i;

	if (*lpS == '.') {
		long            div = 1;

		lpS++;

		while (*lpS == '0') {
			++lpS;
			div *= 10;
		}

		tmp2 = (float) latol(lpS, (short STACKFAR *)BTSNULL);

		while (tmp2 >= 1.0)
			tmp2 /= 10.0;

		tmp1 += tmp2 / (float) div;
	}
	return tmp1;
}
#endif
/* end #if RESTRICTED_ENVIRON */

/* prr modif begin */
#define TOKEN_BUFFER_SIZE	256
#define	TOKEN_BUFFER_MAX	229
/* prr modif end */

#if RESTRICTED_ENVIRON
FUNCTION btsbool WDECL tr_load_font(SPGLOBPTR2 ufix8 STACKFAR *font_ptr, ufix16 buffer_size, void *fc_ctxt)
#else
FUNCTION btsbool tr_load_font(SPGLOBPTR2 font_data **font_ptr, void *fc_ctxt)
#endif
{

#if RESTRICTED_STATS
	ufix16          encoding_data_start, encoding_data_end, subr_data_start, subr_data_end, start_charstring_names, end_charstring_names;
	charstrings_t  *rest_strings;
#endif

#if RESTRICTED_ENVIRON
	ufix32          old_file_byte_count;
#endif

/* prr modif begin */
	ufix8           buffer[TOKEN_BUFFER_SIZE];
/* prr modif end */
	fix15           i;
	real            tempr;
	fix15           tempi;
	btsbool         tempb;
	btsbool         foundFontBBox;
	btsbool         found_ForceBold;
	btsbool         found_StdHW;
	btsbool         found_StdVW;
	fix15           index;
	char            charactername[CHARNAMESIZE];
	fix15           max_no_charstrings;
	fix15           charstring_size;
	ufix8           byte;
	btsbool         char_str_read;
	btsbool         subrs_found;
	btsbool         private_read;
	btsbool         ignore_private;
	btsbool         hybrid_font;
                                                    
	font_data       STACKFAR *local_font_ptr;

#if TIMEIT
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
	struct timeb    ElapseBegin, ElapseEnd;
	long            CpuBegin, CpuEnd;
	long            seconds, result_seconds, result_milliseconds;
	float           real_time;
#endif

#if RESTRICTED_ENVIRON
	ufix16         space_needed;
	ufix8          STACKFAR *charstring_value_ptr;
	charstrings_t  STACKFAR *chars_ptr;
#endif

#if TIMEIT
	/* get the system clock */
	ftime(&ElapseBegin);

	/* get the cpu clock */
	CpuBegin = clock();
#endif

#if RESTRICTED_ENVIRON
	if (buffer_size < sizeof(font_data)) {
		sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
		return FALSE;
	}
	sp_globals.processor.type1.current_font = (font_data STACKFAR *)local_font_ptr = (font_data STACKFAR*) font_ptr;
	local_font_ptr->offset_from_top = sizeof(font_data);
	local_font_ptr->offset_from_bottom = buffer_size;
	local_font_ptr->no_charstrings_loaded = 0;
#else
	if ((local_font_ptr = (font_data *) malloc(sizeof(font_data))) == BTSNULL) {
		sp_report_error(PARAMS2 TR_NO_ALLOC_FONT);
		return FALSE;
	}
	sp_globals.processor.type1.current_font = (font_data STACKFAR *)local_font_ptr;
#endif

/* PRR modif begin */
	memset((char*)local_font_ptr, 0, sizeof(font_data));
	local_font_ptr->ClientData = fc_ctxt;
/* PRR modif end */        
        
    local_font_ptr->buf = local_font_ptr->buf_start;
    local_font_ptr->buf_end = local_font_ptr->buf_start;
    local_font_ptr->buf_size = font_data_BUF_SIZE;
    local_font_ptr->offset = 0;

    local_font_ptr->full_name[0] = 0;
    local_font_ptr->font_name[0] = 0;
    local_font_ptr->family_name[0] = 0;
	local_font_ptr->no_subrs = 0;
	local_font_ptr->paint_type = 0;
	local_font_ptr->replaced_avail = FALSE;
	local_font_ptr->decrypt_mode = FALSE;
	local_font_ptr->hex_mode = FALSE;
	local_font_ptr->no_charstrings = 0;
	local_font_ptr->font_bbox.xmin = 0.0;
	local_font_ptr->font_bbox.ymin = 0.0;
	local_font_ptr->font_bbox.xmax = 0.0;
	local_font_ptr->font_bbox.ymax = 0.0;
	local_font_ptr->leniv = 4;
    local_font_ptr->isFixedPitch = FALSE;
    local_font_ptr->UnderlinePosition = 0;
    local_font_ptr->UnderlineThickness = 0;
    local_font_ptr->proc_level = 0;
    local_font_ptr->str_level = 0;

	clear_hints(local_font_ptr);

    hybrid_font = FALSE;
	char_str_read = FALSE;
	subrs_found = FALSE;
	private_read = FALSE;
	ignore_private = FALSE;
	foundFontBBox = FALSE;
    found_ForceBold = FALSE;
    found_StdHW = FALSE;
    found_StdVW = FALSE;

#if INCL_PFB
    local_font_ptr->tag_bytes = 0x7fffffffL;
    local_font_ptr->font_file_type = 0;

	/* Read first byte in file */
	if (read_byte(local_font_ptr, buffer) == 0)
		return FALSE;

	/* if 80 hex (tag code), assume it's a .pfb file */
	if (buffer[0] == 0x80) {
		if (read_binary(local_font_ptr, (char STACKFAR *)buffer+1, 5) != 5) {
			tr_error(PARAMS2 TR_INV_FILE, local_font_ptr);
			return FALSE;
		}

        if (parse_tag(local_font_ptr, &local_font_ptr->tag_bytes, buffer) == 0) {
			tr_error(PARAMS2 TR_INV_FILE, local_font_ptr);
 			return FALSE;
		}
	}
	else {
		if (buffer[0] != '%') {
			if (read_binary(local_font_ptr, (char STACKFAR *)buffer+1, 5) != 5) {
				tr_error(PARAMS2 TR_INV_FILE, local_font_ptr);
				return FALSE;
			}

			local_font_ptr->tag_bytes = *(fix31 *)buffer;
			local_font_ptr->tag_bytes -= 2;

			if (read_byte(local_font_ptr, buffer) == 0)
				return FALSE;

			local_font_ptr->replaced_avail = TRUE;
			local_font_ptr->replaced_byte = buffer[0];

			local_font_ptr->font_file_type = 1;
		}
	}
#endif  /* INCL_PFB */

/* prr modif begin */
	while (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) >= 0) {
/* prr modif end */
		if (local_font_ptr->proc_level > 0)
			continue;
		if (local_font_ptr->str_level > 0)
			continue;

		if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"/") == 0) {	/* / token? */
/* prr modif begin */
			if (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) < 0) {
/* prr modif end */
				tr_error(PARAMS2 TR_NO_READ_LITNAME, local_font_ptr);
				return FALSE;
			}

			switch (buffer[0]) {
			case 'B':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"BlueValues") == 0) {	/* /BlueValues? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;

					if (local_font_ptr->font_hints.no_blue_values > 0)
						continue;

					if (!read_long_array(PARAMS2 local_font_ptr->font_hints.pblue_values, &local_font_ptr->font_hints.no_blue_values)) {
						tr_error(PARAMS2 TR_NO_READ_VALUES, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("BlueValues = ");
					print_long_array(local_font_ptr->font_hints.pblue_values, local_font_ptr->font_hints.no_blue_values);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"BlueScale") == 0) {	/* /BlueScale? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (!read_real(PARAMS2 (real STACKFAR*)&tempr)) {
						tr_error(PARAMS2 TR_NO_READ_SCALE, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_hints.blue_scale = tempr;
#if DEBUG
					_sPrintf("BlueScale = %7.5f\n", local_font_ptr->font_hints.blue_scale);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"BlueShift") == 0) {	/* /BlueShift? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (!tr_read_long(PARAMS2 &(local_font_ptr->font_hints.blue_shift))) {                   
						tr_error(PARAMS2 TR_NO_READ_SHIFT, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("BlueShift = %ld\n", local_font_ptr->font_hints.blue_shift);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"BlueFuzz") == 0) {	/* /BlueFuzz? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;

					if (!tr_read_long(PARAMS2 &(local_font_ptr->font_hints.blue_fuzz))) {
						tr_error(PARAMS2 TR_NO_READ_FUZZ, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("BlueFuzz = %ld\n", local_font_ptr->font_hints.blue_fuzz);
#endif
				}
				break;

			case 'F':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FullName") == 0) {	/* /FullName? */
					if (private_read)
						continue;
					if (read_string(local_font_ptr, local_font_ptr->full_name, FULLNAMESIZE) < 0) {
						tr_error(PARAMS2 TR_NO_READ_FULLNAME, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("FullName = %s\n", local_font_ptr->full_name);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FontName") == 0) {	/* /FontName? */
					if (private_read)
						continue;
/* prr modif begin */
					if (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) < 0) {
/* prr modif end */
						tr_error(PARAMS2 TR_NO_READ_NAMTOK, local_font_ptr);
						return FALSE;
					}
					if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"/") != 0)
						continue;	/* Ignore unless followed by '/' */

					if (read_token(PARAMS2 (char STACKFAR*)local_font_ptr->font_name, FONTNAMESIZE, 1) < 0) {
						tr_error(PARAMS2 TR_NO_READ_NAME, local_font_ptr);
						return FALSE;
					}
					strcpy(sp_globals.processor.type1.current_font->font_name, local_font_ptr->font_name);
#if DEBUG
					_sPrintf("FontName =  %s\n", local_font_ptr->font_name);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FontMatrix") == 0) {	/* /FontMatrix? */
					if (private_read)
						continue;
					if (!read_real_array(PARAMS2 (real STACKFAR*)local_font_ptr->font_matrix, (fix15 STACKFAR*)&tempi)) {
						tr_error(PARAMS2 TR_NO_READ_MATRIX, local_font_ptr);
						return FALSE;
					}
					if (tempi != 6) {
						tr_error(PARAMS2 TR_MATRIX_SIZE, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("FontMatrix = ");
					print_real_array(local_font_ptr->font_matrix, tempi);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FontBBox") == 0) {
					real rtemp;	/* temp to check for zero FontBBox */

					if (foundFontBBox)	/* accept only the first FontBBox found */
						continue;
					foundFontBBox = TRUE;

					if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {      
						/* Read open bracket or brace */
						tr_error(PARAMS2 TR_NO_READ_OPENBRACKET, local_font_ptr);
						return FALSE;
					}

					if ((compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"{") != 0) && (strcmp((char *)buffer, "[") != 0)) {
						tr_error(PARAMS2 TR_NO_READ_OPENBRACKET, local_font_ptr);	/* ERROR MESSAGE "**** Cannot read { or [ delimitors" */
						return FALSE;
					}

					if (!read_real(PARAMS2 (real STACKFAR*)&tempr)) {
						tr_error(PARAMS2 TR_NO_READ_BBOX0, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_bbox.xmin = tempr;

					if (!read_real(PARAMS2 (real STACKFAR*)&tempr)) {
						tr_error(PARAMS2 TR_NO_READ_BBOX1, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_bbox.ymin = tempr;

					if (!read_real(PARAMS2 (real STACKFAR*)&tempr)) {
						tr_error(PARAMS2 TR_NO_READ_BBOX2, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_bbox.xmax = tempr;

					if (!read_real(PARAMS2 (real STACKFAR*)&tempr)) {
						tr_error(PARAMS2 TR_NO_READ_BBOX3, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_bbox.ymax = tempr;

					/*	Here we compare the sum of all coordinates against
					 *	an impossibly small character to check for a passed
					 *	zero font bounding box.
					*/
					rtemp = local_font_ptr->font_bbox.xmax - local_font_ptr->font_bbox.xmin;
					rtemp += local_font_ptr->font_bbox.ymax - local_font_ptr->font_bbox.ymin;

					if (fabs(rtemp) < 0.01) {
						/*
						 *	A zero sized FontBBox was passed along,
						 *	force the FontBBox to the default BBox
						 *	as specified in the Adobe Type 1 Font format book,
						 *	page 26.
						*/
						local_font_ptr->font_bbox.xmin = -2000.0;
						local_font_ptr->font_bbox.ymin = -2000.0;
						local_font_ptr->font_bbox.xmax = 2000.0;
						local_font_ptr->font_bbox.ymax = 2000.0;
					}
#if DEBUG
					_sPrintf("FontBBox = {%3.1f %3.1f %3.1f %3.1f}\n",
							local_font_ptr->font_bbox.xmin,
							local_font_ptr->font_bbox.ymin,
							local_font_ptr->font_bbox.xmax,
							local_font_ptr->font_bbox.ymax);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FamilyName") == 0) {	/* /FamilyName? */
					if (ignore_private)
						continue;
					if (read_string(local_font_ptr, local_font_ptr->family_name, FAMILYNAMESIZE) < 0) {
/*						tr_error(PARAMS2 TR_NO_READ_FAMILY, local_font_ptr);*/
						return FALSE;
					}
#if DEBUG
					_sPrintf("FamilyName = %s\n", local_font_ptr->family_name);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FamilyBlues") == 0) {	/* /FamilyBlues? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;

					if (!read_long_array(PARAMS2 local_font_ptr->font_hints.pfam_blues, &(local_font_ptr->font_hints.no_fam_blues))) {
						tr_error(PARAMS2 TR_NO_READ_FAMILY, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("FamilyBlues = ");
					print_long_array(local_font_ptr->font_hints.pfam_blues, local_font_ptr->font_hints.no_fam_blues);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FamilyOtherBlues") == 0) {	/* /FamilyOtherBlues? */
					if (private_read == 0)
						continue;

					if (!read_long_array(PARAMS2 local_font_ptr->font_hints.pfam_other_blues, &(local_font_ptr->font_hints.no_fam_other_blues))) {
						tr_error(PARAMS2 TR_NO_READ_FAMOTHER, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("FamilyOtherBlues = ");
					print_long_array(local_font_ptr->font_hints.pfam_other_blues, local_font_ptr->font_hints.no_fam_other_blues);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"ForceBold") == 0) {	/* /ForceBold? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (found_ForceBold)
						continue;

					if (!read_btsbool(PARAMS2 &(local_font_ptr->font_hints.force_bold))) {
						tr_error(PARAMS2 TR_NO_READ_BOLD, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("ForceBold = %s\n", local_font_ptr->font_hints.force_bold ? "true" : "false");
#endif
				}
				break;

			case 'S':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"StdHW") == 0) {	/* /StdHW? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (found_StdHW)
						continue;

					if (!read_real_array(PARAMS2 (real STACKFAR*)&tempr, (fix15 STACKFAR*)&tempi) || (tempi != 1)) {
						tr_error(PARAMS2 TR_NO_READ_STDHW, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_hints.stdhw = tempr;
					found_StdHW = TRUE;
#if DEBUG
					_sPrintf("StdHW = %3.1f\n", local_font_ptr->font_hints.stdhw);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"StdVW") == 0) {	/* /StdVW? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (found_StdVW)
						continue;

					if (!read_real_array(PARAMS2 (real STACKFAR*)&tempr, (fix15 STACKFAR*)&tempi) || (tempi != 1)) {
						tr_error(PARAMS2 TR_NO_READ_STDVW, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->font_hints.stdvw = tempr;
					found_StdVW = TRUE;
#if DEBUG
					_sPrintf("StdVW = %3.1f\n", local_font_ptr->font_hints.stdvw);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"StemSnapH") == 0) {	/* /StemSnapH? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;

					if (!read_real_array(PARAMS2 (real STACKFAR *)local_font_ptr->font_hints.pstem_snap_h,
						(fix15 STACKFAR*)&(local_font_ptr->font_hints.no_stem_snap_h))) {
						tr_error(PARAMS2 TR_NO_READ_SNAPH, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("StemSnapH = ");
					print_real_array(local_font_ptr->font_hints.pstem_snap_h, local_font_ptr->font_hints.no_stem_snap_h);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"StemSnapV") == 0) {	/* /StemSnapV? */
					if (ignore_private)
						continue;
					if (private_read == 0)
						continue;
					if (!read_real_array(PARAMS2 local_font_ptr->font_hints.pstem_snap_v,
						(fix15 STACKFAR*)&(local_font_ptr->font_hints.no_stem_snap_v))) {
						tr_error(PARAMS2 TR_NO_READ_SNAPV, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("StemSnapV = ");
					print_real_array(local_font_ptr->font_hints.pstem_snap_v, local_font_ptr->font_hints.no_stem_snap_v);
					_sPrintf("\n");
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"Subrs") == 0) {	/* /Subrs? */
					if (private_read == 0)
						continue;

					if (hybrid_font) {
#if LOW_RES
						if (subrs_found) {
							if (read_subrs(PARAMS2 local_font_ptr, (char *)buffer, 1) == 0)
								return FALSE;
							else
								break;
						}
#else
						if (subrs_found == 0) {
							if (read_subrs(PARAMS2 local_font_ptr, (char *)buffer, 1) == 0)
								return FALSE;
							else {
								subrs_found = TRUE;
								break;
							}
						}
#endif
					}
					else if (subrs_found) {
						if (read_subrs(PARAMS2 local_font_ptr, (char *)buffer, 1) == 0)
							return FALSE;
						else
							break;
					}

					subrs_found = TRUE;

#if RESTRICTED_STATS
					subr_data_start = local_font_ptr->offset_from_top;
#endif

					if (read_subrs(PARAMS2 local_font_ptr, (char *)buffer, 0) == 0)
						return FALSE;

#if RESTRICTED_STATS
					subr_data_end = local_font_ptr->offset_from_top - 1;
#endif

#if DEBUG
					_sPrintf( "All subrs loaded\n\n" );
#endif
				}
				break;

			case 'U':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"UniqueID") == 0) {	/* /UniqueID? */
					if (private_read)
						continue;
					if (!tr_read_long(PARAMS2 &(local_font_ptr->font_hints.unique_id)))
						continue;
#if DEBUG
					_sPrintf("UniqueID =  %ld\n", local_font_ptr->font_hints.unique_id);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"UnderlinePosition") == 0) {       /* /UnderlinePosition? */
					if (!read_int(PARAMS2 (fix15 *) &local_font_ptr->UnderlinePosition))
						return FALSE;

#if DEBUG
					_sPrintf("UnderlinePosition = %d\n", local_font_ptr->UnderlinePosition);
#endif
				}
				else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"UnderlineThickness") == 0) {       /* /UnderlineThickness? */
					if (!read_int(PARAMS2 (fix15 *) &local_font_ptr->UnderlineThickness))
						return FALSE;

#if DEBUG
					_sPrintf("UnderlineThickness = %d\n", local_font_ptr->UnderlineThickness);
#endif
				}
				break;

			case 'E':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"Encoding") == 0) {	/* /Encoding? */
					if (private_read)
						continue;

/* prr modif begin */
					if (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) < 0) {       
/* prr modif end */
						tr_error(PARAMS2 TR_NO_READ_TOKAFTERENC, local_font_ptr);
						return FALSE;
					}
					if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"StandardEncoding") == 0) {
#if RESTRICTED_ENVIRON
						local_font_ptr->encoding_offset = (ufix16)BTSNULL;
#else
						local_font_ptr->encoding = BTSNULL;
						standard_encoding(local_font_ptr);
#endif
#if DEBUG
						_sPrintf("Encoding = StandardEncoding\n");
#endif
						continue;
					}
#if RESTRICTED_ENVIRON
					/*
					 * get the space for the encoding
					 * array 
					 */
					/* is there room ? */
					space_needed = 256 * sizeof(ufix16);
					if (!get_space(PARAMS2 space_needed))
                                        {
						/* abandon ship */
						sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
						return FALSE;
					}
					/* allocate the space */
					local_font_ptr->encoding_offset = local_font_ptr->offset_from_top;
					local_font_ptr->offset_from_top += space_needed;
					if (clear_encoding(PARAMS2 local_font_ptr) == FALSE)
						return FALSE;
#else
					/* allocate the 256 pointer table */
					if ((local_font_ptr->encoding = (CHARACTERNAME **) malloc(256 * sizeof(CHARACTERNAME *))) == BTSNULL)
                                        {
						tr_error(PARAMS2 TR_NO_SPC_ENC_ARR, local_font_ptr);
						return FALSE;
					}
/* PRR modif begin */
memset((char*)local_font_ptr->encoding, 0, 256 * sizeof(CHARACTERNAME *));
/* PRR modif end */
					/*
					 * set all the entries to point to
					 * not defined 
					 */
					clear_encoding(PARAMS2 local_font_ptr);
#endif
#if RESTRICTED_STATS
					encoding_data_start = local_font_ptr->offset_from_top;
#endif
					while (TRUE) {	/* Loop over encoding vector entries */
						if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {
							tr_error(PARAMS2 TR_NO_READ_ENCODETOK, local_font_ptr);
							return FALSE;
						}
						while (TRUE) {	/* Look for encoding entry or end of encoding table */
							/* Start of encoding entry */
							if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"dup") == 0)
								break;
							/* End of encoding table */
							if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"def") == 0)
								goto end_encoding;
							/* End of encoding table */
							if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"readonly") == 0)
								goto end_encoding;
							if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {
								tr_error(PARAMS2 TR_NO_READ_ENCODETOK, local_font_ptr);
								return FALSE;
							}
						}

						if (!read_int(PARAMS2 (fix15 STACKFAR*)&tempi)) {
							tr_error(PARAMS2 TR_NO_READ_ENCODE, local_font_ptr);
							return FALSE;
						}
						index = tempi;

						if (read_token(PARAMS2 (char STACKFAR*)buffer, 10, 1) < 0) {	/* Read / before character name */
							tr_error(PARAMS2 TR_NO_READ_SLASH, local_font_ptr);
							return FALSE;
						}
						if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"/") != 0) {	/* Not / character? */
							tr_error(PARAMS2 TR_NO_READ_SLASH, local_font_ptr);
							return FALSE;
						}
						if (read_token(PARAMS2 (char STACKFAR*)charactername, CHARNAMESIZE, 1) < 0) {	/* Read character name */
							tr_error(PARAMS2 TR_NO_READ_CHARNAME, local_font_ptr);
							return FALSE;
						}
						if (!add_encoding(PARAMS2 local_font_ptr, (fix15)index, (ufix8 STACKFAR *)charactername)) {
							tr_error(PARAMS2 TR_NO_SPC_ENC_ENT, local_font_ptr);
							return FALSE;
						}
					}
			end_encoding:;
#if RESTRICTED_STATS
					encoding_data_end = local_font_ptr->offset_from_top - 1;
#endif
#if DEBUG
					print_encoding(local_font_ptr);
#endif
				}
				break;

			case 'P':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"Private") == 0) {	/* /Private? */
					if (read_int(PARAMS2 (fix15 *)&tempi) == 0)
						break;
					if (read_token(PARAMS2 (char *)buffer, 20, 1) <0 )
						break;
					if (compstr(buffer, (ufix8 *) "dict") != 0)
						break;
					if (read_token(PARAMS2 (char *)buffer, 20, 1) <0 )
						break;
					if (compstr(buffer, (ufix8 *) "dup") != 0)
						break;
					if (read_token(PARAMS2 (char *)buffer, 20, 1) <0 )
						break;
					if (compstr(buffer, (char *) "begin") != 0)
						break;

					private_read = TRUE;
				}
				break;

			case 'O':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"OtherBlues") == 0) {	/* /OtherBlues? */
					if (ignore_private)
						continue;
					 if (local_font_ptr->font_hints.no_other_blues >0)
						continue;

					if (!read_long_array(PARAMS2 local_font_ptr->font_hints.pother_blues, &(local_font_ptr->font_hints.no_other_blues))) {
						tr_error(PARAMS2 TR_NO_READ_OTHERBL, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("OtherBlues = ");
					print_long_array(local_font_ptr->font_hints.pother_blues, local_font_ptr->font_hints.no_other_blues);
					_sPrintf("\n");
#endif
				}
				break;

			case 'L':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"LanguageGroup") == 0) {	/* /LanguageGroup? */
					if (ignore_private)
						continue;
					if (!tr_read_long(PARAMS2 &(local_font_ptr->font_hints.language_group))) {
						tr_error(PARAMS2 TR_NO_READ_LANGGRP, local_font_ptr);
						return FALSE;
					}
#if DEBUG
					_sPrintf("LanguageGroup = %d\n", local_font_ptr->font_hints.language_group);
#endif
				}
				break;

			case 'l':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"lenIV") == 0) {	/* /lenIV? */
					if (ignore_private)
						continue;
					if (!read_int(PARAMS2 (fix15 STACKFAR*)&tempi)) {
						tr_error(PARAMS2 TR_NO_READ_LENIV, local_font_ptr);
						return FALSE;
					}
					local_font_ptr->leniv = tempi;
#if DEBUG
					_sPrintf("lenIV = %d\n", (int) local_font_ptr->leniv);
#endif
				}
				break;

			case 'i':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"isFixedPitch") == 0) {
					if (!read_btsbool(PARAMS2 &(local_font_ptr->isFixedPitch))) {
/*						tr_error(PARAMS2 TR_NO_READ_BOLD, local_font_ptr); */
						return FALSE;
					}
              
#if DEBUG
					_sPrintf("isFixedPitch = %s\n", local_font_ptr->isFixedPitch ? "true" : "false" );
#endif
				}
				break;

			case 'h':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"hires") == 0)
					hybrid_font = TRUE;
				break;

			case 'C':
				if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"CharStrings") == 0) {	/* /CharStrings? */
					ufix16 temp;

					if (char_str_read)	/* CharStrings already read? */
						continue;
                
					if (hybrid_font) {
/* prr modif begin */
						if (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) < 0) {
/* prr modif end */
							tr_error(PARAMS2 TR_NO_READ_LITNAME, local_font_ptr);
							return FALSE;
						}
						/* This implementation of Hybrid fonts relies on Adobe's implementation in Optima. They           
						 * put the first set of Charstrings (we assume these are low res) after an "if", and the
						 * second after an ifelse. We do not interpret the if or ifelse operators. */
#if LOW_RES
						while (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"if") != 0)
#else
						while (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"ifelse") != 0)
#endif
/* prr modif begin */
							if (read_token(PARAMS2 (char STACKFAR*)buffer, TOKEN_BUFFER_MAX, 1) < 0) {
/* prr modif end */
								tr_error(PARAMS2 TR_NO_READ_LITNAME, local_font_ptr);
								return FALSE;
							}
					}
					if (!read_int(PARAMS2 (fix15 STACKFAR*)&tempi)) {
						tr_error(PARAMS2 TR_NO_READ_STRINGNUM, local_font_ptr);
						return FALSE;
					}
					max_no_charstrings = tempi;

#if RESTRICTED_ENVIRON
					/* is there room ? */
					space_needed = sizeof(charstrings_t) * max_no_charstrings;
					if (!inquire_about_space(local_font_ptr, space_needed)) {
						/* abandon ship */
						sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
						return FALSE;
					}
					/* allocate the space */
					chars_ptr = (charstrings_t STACKFAR*) ((ufix32) font_ptr + (ufix32) local_font_ptr->offset_from_top);
					local_font_ptr->charstrings_offset = local_font_ptr->offset_from_top;
					local_font_ptr->offset_from_top += space_needed;
#if RESTRICTED_DEBUG
					_sPrintf("CharStrings table loaded at offset %x\n", local_font_ptr->charstrings_offset);
#endif
#else   /* !RESTRICTED_ENVIRON */
					/*
					 * malloc the space for the
					 * charstrings array 
					 */
					if ((local_font_ptr->charstrings = (charstrings_t *)malloc(sizeof(charstrings_t) * max_no_charstrings)) == BTSNULL) {
						tr_error(PARAMS2 TR_NO_SPC_STRINGS, local_font_ptr);
						return FALSE;
					}
/* PRR modif begin */
memset((char*)local_font_ptr->charstrings, 0, sizeof(charstrings_t) * max_no_charstrings);
/* PRR modif end */
#endif
#if RESTRICTED_STATS
					start_charstring_names = local_font_ptr->offset_from_top;
#endif
					for (i = 0; i < max_no_charstrings; i++) {
						if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {
							tr_error(PARAMS2 TR_NO_READ_STRING, local_font_ptr);
							return FALSE;
						}

						while (TRUE) {	/* Look for character name or end of charstrings */
							if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"/") == 0)
								break;
							if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"end") == 0)
								goto end_charstrings;
							if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {
								tr_error(PARAMS2 TR_NO_READ_STRING, local_font_ptr);
								return FALSE;
							}
						}
						if (read_token(PARAMS2 (char STACKFAR*)charactername, CHARNAMESIZE, 0) < 0) {
							tr_error(PARAMS2 TR_NO_READ_CHARNAME, local_font_ptr);
							return FALSE;
						}
#if RESTRICTED_ENVIRON
						/* store character name */
						space_needed = strlen(charactername) + 1;
						if (!get_space(PARAMS2 space_needed)) {
							/* abandon ship */
							sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
							return FALSE;
						}
						/* allocate the space */
						chars_ptr[local_font_ptr->no_charstrings].key_offset = local_font_ptr->offset_from_top;
#if RESTRICTED_DEBUG
						_sPrintf("Charname %s loaded at offset %x\n", charactername, local_font_ptr->offset_from_top);
#endif
#if   WINDOWS_4IN1
						lstrcpy((char STACKFAR*) ((ufix32) local_font_ptr + (ufix32) local_font_ptr->offset_from_top), charactername);
#else
						strcpy((char *) ((ufix32) local_font_ptr + (ufix32) local_font_ptr->offset_from_top), charactername);
#endif
						local_font_ptr->offset_from_top += space_needed;
#else   /* !RESTRICTED_ENVIRON */
						/*
						 * allocate storage for
						 * character name 
						 */
						if ((local_font_ptr->charstrings[local_font_ptr->no_charstrings].key = (CHARACTERNAME *) malloc(CHARSIZE)) == BTSNULL) {
							tr_error(PARAMS2 TR_NO_SPC_STRINGS, local_font_ptr);
							return FALSE;
						}
#if NAME_STRUCT
						local_font_ptr->charstrings[local_font_ptr->no_charstrings].key->char_name =
							(unsigned char *) ((ufix32) (local_font_ptr->charstrings[local_font_ptr->no_charstrings].key) + (ufix32) sizeof(CHARACTERNAME));
#endif
						strcpy((char *)local_font_ptr->charstrings[local_font_ptr->no_charstrings].key, charactername);

#endif  /* !RESTRICTED_ENVIRON */

						if (!read_int(PARAMS2 (fix15 STACKFAR*)&tempi)) {
							tr_error(PARAMS2 TR_NO_READ_STRINGSIZE, local_font_ptr);
							return FALSE;
						}
						charstring_size = tempi;
#if DEBUG
						_sPrintf("CharString %s, %d bytes\n", charactername, charstring_size);
#endif
#if !RESTRICTED_ENVIRON
						/*
						 * allocate storage for
						 * character program string  
						 */
						if ((local_font_ptr->charstrings[local_font_ptr->no_charstrings].value = (ufix8 *) malloc(charstring_size)) == BTSNULL) {
							tr_error(PARAMS2 TR_NO_SPC_STRINGS, local_font_ptr);
							return FALSE;
						}
#endif
						if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0) {
							tr_error(PARAMS2 TR_NO_READ_STRINGTOK, local_font_ptr);
							return FALSE;
						}
#if RESTRICTED_ENVIRON
						chars_ptr[local_font_ptr->no_charstrings].decrypt_mode = local_font_ptr->decrypt_mode;
						chars_ptr[local_font_ptr->no_charstrings].decryption_key = local_font_ptr->decrypt_r;
						chars_ptr[local_font_ptr->no_charstrings].file_position = get_file_pos(local_font_ptr);
						chars_ptr[local_font_ptr->no_charstrings].charstring_size = charstring_size;
						chars_ptr[local_font_ptr->no_charstrings].hex_mode = local_font_ptr->hex_mode;
						old_file_byte_count = get_file_pos(local_font_ptr);
#if INCL_PFB
						chars_ptr[local_font_ptr->no_charstrings].tag_bytes = local_font_ptr->tag_bytes;
#endif
						if (!inquire_about_space(local_font_ptr, charstring_size)) {
#if RESTRICTED_DEBUG
							_sPrintf("No room to load charstring data for %s\n", charactername);
#endif
							chars_ptr[local_font_ptr->no_charstrings].value_offset = 0;
							if (skip_binary(local_font_ptr, charstring_size) != charstring_size) {
								tr_error(PARAMS2 TR_NO_READ_STRINGBIN, local_font_ptr);
								return FALSE;
							}
						}
						else {
							local_font_ptr->no_charstrings_loaded++;
							charstring_value_ptr = (ufix8 STACKFAR*) font_ptr + local_font_ptr->offset_from_bottom - charstring_size;

							chars_ptr[local_font_ptr->no_charstrings].value_offset = local_font_ptr->offset_from_bottom - charstring_size;
							if (read_binary(local_font_ptr, (char STACKFAR *)charstring_value_ptr, charstring_size) != charstring_size) {
								tr_error(PARAMS2 TR_NO_READ_STRINGBIN, local_font_ptr);
								return FALSE;
							}
							local_font_ptr->offset_from_bottom -= charstring_size;
#if RESTRICTED_DEBUG
							_sPrintf("CharString for %s loaded at offset %x\n", charactername, local_font_ptr->offset_from_bottom);
#endif
						}
						chars_ptr[local_font_ptr->no_charstrings].file_bytes = get_file_pos(local_font_ptr) - old_file_byte_count;
#else   /* !RESTRICTED_ENVIRON */
						if (read_binary(local_font_ptr, (char STACKFAR *)local_font_ptr->charstrings[local_font_ptr->no_charstrings].value,
							(fix15)charstring_size) != charstring_size) {
							tr_error(PARAMS2 TR_NO_READ_STRINGBIN, local_font_ptr);
							return FALSE;
						}
						local_font_ptr->charstrings[local_font_ptr->no_charstrings].size = charstring_size;
#if DEBUG
						_sPrintf("CharString for %s loaded at offset %x\n", charactername, local_font_ptr->charstrings[local_font_ptr->no_charstrings].value);
#if CHARSTRING_DEBUG
						print_binary(PARAMS2 (ufix8 *)local_font_ptr->charstrings[local_font_ptr->no_charstrings].value, charstring_size);
#endif
#endif
#endif  /* !RESTRICTED_ENVIRON */
						local_font_ptr->no_charstrings++;
					}
			end_charstrings:
#if RESTRICTED_STATS
					end_charstring_names = local_font_ptr->offset_from_top - 1;
#endif
#if RESTRICTED_ENVIRON
#if RESTRICTED_DEBUG
					{
						fix15 charstrings_not_loaded;

						charstrings_not_loaded = local_font_ptr->no_charstrings - local_font_ptr->no_charstrings_loaded;

						_sPrintf("%d Charstrings loaded\n", local_font_ptr->no_charstrings - charstrings_not_loaded);
						if (charstrings_not_loaded != 0)
							_sPrintf("%d Charstrings not loaded\n", charstrings_not_loaded);
						_sPrintf("\n");
					}
#endif  /* RESTRICTED_DEBUG */
#else
#if DEBUG
					_sPrintf("%d Charstrings loaded\n\n", local_font_ptr->no_charstrings);
#endif
#endif  /* !RESTRICTED_ENVIRON */
					char_str_read = TRUE;
				}
			}	/* end switch (buffer[0]) */
		}	/* end if (compstr((ufix8 STACKFAR*)buffer, "/") == 0) */ 
		else if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"eexec") == 0) {	/* eexec? */
			ufix8 look_ahead[4];

			tempb = TRUE;	/* Expect hex data */
			if (!read_byte(local_font_ptr, (ufix8 STACKFAR*)&look_ahead[0])) {	/* Read first byte after eexec */
				tr_error(PARAMS2 TR_NO_READ_EXEC1BYTE, local_font_ptr);
				return FALSE;
			}
			if ((look_ahead[0] > 126) || ((char_type[look_ahead[0]] & (SPACE_CHAR | HEX_DIGIT)) == 0))	/* Not space or hex digit? */
				tempb = FALSE;	/* Expect binary data */
			for (i = 0; i < 3; i++) {
				if (!read_byte(local_font_ptr, (ufix8 STACKFAR*)&look_ahead[i + 1])) {	/* Read next byte */
					tr_error(PARAMS2 TR_NO_READ_EXECBYTE, local_font_ptr);
					return FALSE;
				}
				if ((look_ahead[i + 1] > 126) || ((char_type[look_ahead[i + 1]] & HEX_DIGIT) == 0))	/* Not hex digit? */
					tempb = FALSE;	/* Expect binary data */
			}

			local_font_ptr->decrypt_r = 55665;	/* Initialize decryption */
			local_font_ptr->decrypt_mode = TRUE;	/* Enable decryption */
			local_font_ptr->hex_mode = tempb;	/* Select binary or hex mode */
			if (tempb) {	/* skip first four hex bytes */
				look_ahead[0] = (ufix8)(asctohex(look_ahead[0]) << 4) + asctohex(look_ahead[1]);
				look_ahead[1] = (ufix8)(asctohex(look_ahead[2]) << 4) + asctohex(look_ahead[3]);
				local_font_ptr->decrypt_r = (look_ahead[0] + local_font_ptr->decrypt_r) * DECRYPT_C1 + DECRYPT_C2;
				local_font_ptr->decrypt_r = (look_ahead[1] + local_font_ptr->decrypt_r) * DECRYPT_C1 + DECRYPT_C2;
				read_byte(local_font_ptr, (ufix8 STACKFAR*)&byte);
				read_byte(local_font_ptr, (ufix8 STACKFAR*)&byte);
			}
			else {
				/* update the decryption key */
				/* skip first bytes */
				for (i = 0; i < 4; i++)
					local_font_ptr->decrypt_r = (look_ahead[i] + local_font_ptr->decrypt_r) * DECRYPT_C1 + DECRYPT_C2;
			}
		}
//		else if ((strcmp((char *)buffer, "closefile") == 0) || ((strcmp((char *)buffer, "end") == 0) && char_str_read)) {	/* closefile? */
		else if ((strcmp((char *)buffer, "closefile") == 0) || (strcmp((char *)buffer, "end") == 0)) {	/* closefile? */
			if (char_str_read) {	/* CharStrings read? */
				local_font_ptr->decrypt_mode = FALSE;
				local_font_ptr->hex_mode = FALSE;
#if !RESTRICTED_ENVIRON
				*font_ptr = local_font_ptr;
#endif
#if TIMEIT
				/* get the cpu clock end time */
				CpuEnd = clock();

				/* get the system end time */
				ftime(&ElapseEnd);
				result_seconds = ElapseEnd.time - ElapseBegin.time;
				result_milliseconds = ElapseEnd.millitm - ElapseBegin.millitm;
				real_time = (float) result_seconds + (((float) result_milliseconds) / 1000.0);

				/* print out the results */
				_sPrintf("Total elapse time is  %f \n", real_time);
				_sPrintf("Total cpu time is     %f \n", (double) (CpuEnd - CpuBegin) / 1000000.0);
#endif
#if RESTRICTED_STATS
				_sPrintf("Memory size is %x bytes\n", buffer_size);
				_sPrintf("  font_data structure loaded at offset 0\n");
				if (sp_globals.processor.type1.current_font->encoding_offset == 0)
					_sPrintf("  This font uses standard encoding.\n");
				else {
					_sPrintf("  Encoding array loaded at offset %x, total_bytes = %x\n",
					       sp_globals.processor.type1.current_font->encoding_offset,
					       256 * sizeof(ufix16));
					_sPrintf("    Encoding data starts at offset %x\n", encoding_data_start);
					_sPrintf("    Encoding data ends at offset %x\n", encoding_data_end);
				}
				_sPrintf("  Subrs structure loaded at offset %x \n", sp_globals.processor.type1.current_font->subrs_offset);
				_sPrintf("    Subrs data starts at offset %x\n", subr_data_start);
				_sPrintf("    Subrs data ends at offset %x\n", subr_data_end);
				_sPrintf("  Charstring structure loaded at offset %x, total bytes = %x\n",
				       sp_globals.processor.type1.current_font->charstrings_offset,
				       sizeof(charstrings_t) * sp_globals.processor.type1.current_font->no_charstrings);
				_sPrintf("    Charstring name data loaded at %x\n", start_charstring_names);
				_sPrintf("    Charstring name data ends at %x\n", end_charstring_names);
				rest_strings = (charstrings_t *) ((ufix32) sp_globals.processor.type1.current_font +
								  (ufix32) ((font_data *) sp_globals.processor.type1.current_font->charstrings_offset));
				/*
				 * look through the entire charstrings
				 * dictionary 
				 */
				for (i = 0; i < sp_globals.processor.type1.current_font->no_charstrings; i++) {
					/* is this character in memory ? */
					if (rest_strings[i].value_offset != 0)
						_sPrintf("Character %s loaded at offset %x\n",
						       (char *) ((ufix32) sp_globals.processor.type1.current_font + (ufix32) rest_strings[i].key_offset),
						       rest_strings[i].value_offset);
				}
#endif
				return TRUE;
			}
		}		/* if "closefile" */
	}			/* end of WHILE */
	tr_error(PARAMS2 TR_EOF_READ, local_font_ptr);	/* ERROR MESSAGE "*** End of file read" */
	return FALSE;
}

FUNCTION fix15 read_subrs( SPGLOBPTR2 font_data STACKFAR *font_ptr, char STACKFAR *buffer, fix15 ignore_subrs)
{
#if RESTRICTED_ENVIRON
        ufix16  space_needed;
        subrs_t STACKFAR *subrs_ptr;
        ufix8   STACKFAR *subr_value_ptr;
#endif
        fix15   no_subrs;

#if DEBUG
        if (ignore_subrs)
                _sPrintf("\nIgnoring subrs\n");
        else    
                _sPrintf("\nIncluding subrs\n");
#endif

        if (read_int(PARAMS2 &no_subrs) == 0)
        {
                tr_error(PARAMS2 TR_NO_READ_NUMSUBRS, font_ptr);
                return FALSE;
        }

        if (!ignore_subrs)
        {
                font_ptr->no_subrs = no_subrs;

#if RESTRICTED_ENVIRON
                space_needed = sizeof(subrs_t) * no_subrs;

                if (!get_space(PARAMS2 space_needed))
                {
                        sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
                        return FALSE;
                }

                /* allocate the space */
                subrs_ptr = (subrs_t STACKFAR *)((ufix32)font_ptr + (ufix32)font_ptr->offset_from_top);
                font_ptr->subrs_offset = font_ptr->offset_from_top;

#if RESTRICTED_DEBUG
                _sPrintf("Subrs table loaded at offset %x\n", font_ptr->offset_from_top);
#endif
                font_ptr->offset_from_top += space_needed;

#else
                if ((font_ptr->subrs = (subrs_t *)malloc(sizeof(subrs_t) * no_subrs)) == BTSNULL)
                {
                        tr_error(PARAMS2 TR_NO_SPC_SUBRS, font_ptr);
                        return FALSE;
                }
/* PRR modif begin */
memset((char*)font_ptr->subrs, 0, sizeof(subrs_t) * no_subrs);
/* PRR modif end */

#endif
        }

        while (no_subrs > 0)
        {
                fix15   index;
                fix15   subr_size;

                if (read_token(PARAMS2 buffer, 20, 1) < 0)
                {
                        tr_error(PARAMS2 TR_NO_READ_DUPTOK, font_ptr);
                        return FALSE;
                }

                while (strcmp(buffer, "dup") != 0)
                {
                        if (read_token(PARAMS2 buffer, 20, 1) < 0)
                        {
                                tr_error(PARAMS2 TR_NO_READ_DUPTOK, font_ptr);
                                return FALSE;
                        }
                }

                if (read_int(PARAMS2 &index) == 0)
                {
                        tr_error(PARAMS2 TR_NO_READ_SUBRINDEX, font_ptr);
                        return FALSE;
                }
                
                if (read_int(PARAMS2 &subr_size) == 0)
                {
                        tr_error(PARAMS2 TR_NO_READ_BINARY, font_ptr);
                        return FALSE;
                }

#if DEBUG
                _sPrintf("Subr %d, %d bytes\n", index, subr_size);
#endif

                if (read_token(PARAMS2 buffer, 20, 1) < 0)
                {
                        tr_error(PARAMS2 TR_NO_READ_SUBRTOK, font_ptr);
                        return FALSE;
                }

                if (!ignore_subrs)
                {
#if RESTRICTED_ENVIRON
                        subrs_ptr[index].subr_size = subr_size;

                        if (!get_space(PARAMS2 subr_size))
                        {
                                sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
                                return FALSE;
                        }
                        else
                                subr_value_ptr = (ufix8 STACKFAR *)font_ptr + font_ptr->offset_from_top;
                    
                        subrs_ptr[index].data_offset = font_ptr->offset_from_top;
                        if (read_binary(font_ptr, (char STACKFAR *)subr_value_ptr, subr_size) != subr_size)
                        {
                                tr_error(PARAMS2 TR_NO_READ_SUBRBIN, font_ptr);
                                return FALSE;
                        }
                    
#if RESTRICTED_DEBUG
                        _sPrintf("Subr %i loaded at offset %x\n", index, subrs_ptr[index].data_offset);
#endif

                        font_ptr->offset_from_top += subr_size;

#else
                        if ((font_ptr->subrs[index].value = (ufix8 *)malloc(subr_size)) == 0)
                        {
                                tr_error(PARAMS2 TR_NO_SPC_SUBRS, font_ptr);
                                return FALSE;
                        }
          
                        if (read_binary(font_ptr, (char STACKFAR *)font_ptr->subrs[index].value, subr_size) != subr_size)
                        {
                                tr_error(PARAMS2 TR_NO_READ_SUBRBIN, font_ptr);
                                return FALSE;
                        }
#endif
                }
                else
                {
                        if (skip_binary(font_ptr, subr_size) != subr_size)
                        {
                                tr_error(PARAMS2 TR_NO_READ_SUBRBIN, font_ptr);
                                return FALSE;
                        }
                }
                no_subrs--;
        }

        return TRUE;

}       /* end of read_subrs() */




/* base2decimal                                                         */
/* This function converts an ascii number of any base 2-36 to decimal */

FUNCTION LOCAL_PROTO btsbool base2decimal(fix15 base, char STACKFAR *buffer, fix15 STACKFAR *number)
{
	fix15           i;
	fix15           value, result;

	result = 0;
	i = 0;

#if WINDOWS_4IN1
	while (i < (fix15)lstrlen(buffer))
#else
	while (i < (fix15)strlen(buffer))
#endif
        {
		switch (buffer[i])
                {
		case '0':
			value = 0;
			break;
		case '1':
			value = 1;
			break;
		case '2':
			value = 2;
			break;
		case '3':
			value = 3;
			break;
		case '4':
			value = 4;
			break;
		case '5':
			value = 5;
			break;
		case '6':
			value = 6;
			break;
		case '7':
			value = 7;
			break;
		case '8':
			value = 8;
			break;
		case '9':
			value = 9;
			break;
		case 'A':
			value = 10;
			break;
		case 'B':
			value = 11;
			break;
		case 'C':
			value = 12;
			break;
		case 'D':
			value = 13;
			break;
		case 'E':
			value = 14;
			break;
		case 'F':
			value = 15;
			break;
		case 'G':
			value = 16;
			break;
		case 'H':
			value = 17;
			break;
		case 'I':
			value = 18;
			break;
		case 'J':
			value = 19;
			break;
		case 'K':
			value = 20;
			break;
		case 'L':
			value = 21;
			break;
		case 'M':
			value = 22;
			break;
		case 'N':
			value = 23;
			break;
		case 'O':
			value = 24;
			break;
		case 'P':
			value = 25;
			break;
		case 'Q':
			value = 26;
			break;
		case 'R':
			value = 27;
			break;
		case 'S':
			value = 28;
			break;
		case 'T':
			value = 29;
			break;
		case 'U':
			value = 30;
			break;
		case 'V':
			value = 31;
			break;
		case 'W':
			value = 32;
			break;
		case 'X':
			value = 33;
			break;
		case 'Y':
			value = 34;
			break;
		case 'Z':
			value = 35;
			break;
		case 'a':
			value = 10;
			break;
		case 'b':
			value = 11;
			break;
		case 'c':
			value = 12;
			break;
		case 'd':
			value = 13;
			break;
		case 'e':
			value = 14;
			break;
		case 'f':
			value = 15;
			break;
		case 'g':
			value = 16;
			break;
		case 'h':
			value = 17;
			break;
		case 'i':
			value = 18;
			break;
		case 'j':
			value = 19;
			break;
		case 'k':
			value = 20;
			break;
		case 'l':
			value = 21;
			break;
		case 'm':
			value = 22;
			break;
		case 'n':
			value = 23;
			break;
		case 'o':
			value = 24;
			break;
		case 'p':
			value = 25;
			break;
		case 'q':
			value = 26;
			break;
		case 'r':
			value = 27;
			break;
		case 's':
			value = 28;
			break;
		case 't':
			value = 29;
			break;
		case 'u':
			value = 30;
			break;
		case 'v':
			value = 31;
			break;
		case 'w':
			value = 32;
			break;
		case 'x':
			value = 33;
			break;
		case 'y':
			value = 34;
			break;
		case 'z':
			value = 35;
			break;
		default:
			return FALSE;
		}

		if (value > base)
			return FALSE;	/* invalid syntax */

		result = (result * base) + value;
		i = i + 1;
	}

	*number = result;
	return TRUE;
}


FUNCTION LOCAL_PROTO btsbool read_int(SPGLOBPTR2 fix15 STACKFAR *pdata)
{
	char            buffer[20];
	fix15           index;
	btsbool         found_radix;

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

	/* check if it is a radix number */
	found_radix = FALSE;
	index = 0;

	while (index < (fix15)strlen(buffer))
        {
		if (buffer[index] == '#')	/* found a radix number */
                {
			found_radix = TRUE;	/* set the flag */
			buffer[index] = 0;	/* take out the pound sign */
			break;                  /* exit the loop - the index for the */
                                                /* poundsign is saved in index  */
		}
		index++;
	}
	index++;

#if RESTRICTED_ENVIRON
	if (*buffer != '-' && *buffer != '+' && (*buffer < '0' || *buffer > '9'))
		return FALSE;
	*pdata = (fix15) latol((char STACKFAR*)buffer, (short STACKFAR*)BTSNULL);
#else

#if WINDOWS_4IN1
        if (wvs_sPrintf(pdata, "%hd", buffer) != 1)
                return FALSE;
#else
	if (sscanf(buffer, "%hd", pdata) != 1)
		return FALSE;
#endif

#endif
	/* if this is a radix number - determine the decimal value */
	if (found_radix)
        {
		if ((*pdata > 36) || (*pdata < 2))	/* pdata now contains the base */
			return FALSE;                   /* only base 2 - 36 supported */

		if (base2decimal(*pdata, (char STACKFAR*)&buffer[index], pdata) != 1)
			return FALSE;
	}
	return TRUE;
}


FUNCTION LOCAL_PROTO btsbool tr_read_long(SPGLOBPTR2 fix31 STACKFAR *pdata)
{
	char            buffer[20];

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

#if RESTRICTED_ENVIRON
	if (*buffer != '-' && *buffer != '+' && (*buffer < '0' || *buffer > '9'))
		return FALSE;
	*pdata = (fix31) latol((char STACKFAR *)buffer, (short STACKFAR *)BTSNULL);
#else

#if WINDOWS_4IN1
        if (wvs_sPrintf(pdata, "%ld", buffer) != 1)
                return FALSE;
#else
	if (sscanf(buffer, "%ld", pdata) != 1)
		return FALSE;
#endif
#endif
	return TRUE;
}



FUNCTION LOCAL_PROTO btsbool read_long_array(SPGLOBPTR2 fix31 STACKFAR *data, fix15 STACKFAR *pn)
{
	fix15           i;
	char            buffer[20];

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

	if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"[") != 0)
		return FALSE;

	for (i = 0; TRUE; i++)
        {
		if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
			return FALSE;

		if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"]") == 0)
                {
			*pn = i;
			return TRUE;
		}
#if RESTRICTED_ENVIRON
		if (*buffer != '-' && *buffer != '+' && (*buffer < '0' || *buffer > '9'))
			return FALSE;
		data[i] = (fix31) latol((char STACKFAR*)buffer, (short STACKFAR*)BTSNULL);
#else

#if WINDOWS_4IN1
                if (wvs_sPrintf((fix31 STACKFAR*)&data[i], "%ld", buffer) != 1)
                        return FALSE;
#else
		if (sscanf(buffer, "%ld", &data[i]) != 1)
			return FALSE;
#endif
#endif
	}
}


FUNCTION LOCAL_PROTO btsbool read_real(SPGLOBPTR2 real STACKFAR *pdata)
{
	char            buffer[20];
	float           tempf;

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

#if RESTRICTED_ENVIRON
	tempf = latof((char STACKFAR*)buffer);
#else

#if WINDOWS_4IN1
        if (wvs_sPrintf((float STACKFAR*)&tempf, "%f", buffer) != 1)
                return FALSE;
#else
	if (sscanf(buffer, "%f", &tempf) != 1)
		return FALSE;
#endif
#endif
	*pdata = (real) tempf;
	return TRUE;
}


FUNCTION LOCAL_PROTO btsbool read_real_array(SPGLOBPTR2 real STACKFAR *data, fix15 STACKFAR *pn)
{
	fix15           i;
	char            buffer[20];
	float           tempf;

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

	if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"[") != 0)
		return FALSE;

	for (i = 0; TRUE; i++)
        {
		if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
			return FALSE;

		if (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"]") == 0)
                {
			*pn = i;
			return TRUE;
		}
#if RESTRICTED_ENVIRON
		tempf = latof((char STACKFAR*)buffer);
#else

#if WINDOWS_4IN1
                if (wvs_sPrintf((float STACKFAR*)&tempf, "%f", buffer) != 1)
                        return FALSE;
#else
		if (sscanf(buffer, "%f", &tempf) != 1)
			return FALSE;
#endif
#endif
		*(real STACKFAR*)(data+i) = (real) tempf;
	}
}


FUNCTION LOCAL_PROTO btsbool read_btsbool(SPGLOBPTR2 btsbool STACKFAR *pdata)
{
	char            buffer[20];

	if (read_token(PARAMS2 (char STACKFAR*)buffer, 20, 1) < 0)
		return FALSE;

	if ((compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"true") == 0) ||
	    (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"TRUE") == 0))
        {
		*pdata = TRUE;
		return TRUE;
	}

	if ((compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"false") == 0) ||
	    (compstr((ufix8 STACKFAR*)buffer, (ufix8 STACKFAR *)"FALSE") == 0))
        {
		*pdata = FALSE;
		return TRUE;
	}

	return FALSE;
}



FUNCTION LOCAL_PROTO fix15 read_token(SPGLOBPTR2 char STACKFAR *buf, fix15 count, fix15 ignore_space_chars)
{
        font_data       *font_ptr;
	fix15           len;
	ufix8           byte;

        font_ptr = sp_globals.processor.type1.current_font;

        if ((font_ptr->hex_mode == 0) && (font_ptr->tag_bytes > count))
        {
                fix31   offset;
                fix15   res;

                offset = get_file_pos(font_ptr);

                if (font_ptr->decrypt_mode)
                        res = read_encrypted_token(font_ptr, buf, count, ignore_space_chars);
                else
                        res = read_plain_token(font_ptr, buf, count, ignore_space_chars);
          
                font_ptr->tag_bytes -= get_file_pos(font_ptr) - offset;
          
                if (font_ptr->tag_bytes >= 0)
                {
                        if (res == -2)
                        {
                                sp_report_error( PARAMS2 TR_TOKEN_LARGE);
                                return(count);
                        }
                        else
                                return(res);
                }
                else
                        set_file_pos(font_ptr, offset);
        }

rd_next_line:

        if (font_ptr->replaced_avail)
        {
                byte = font_ptr->replaced_byte;
                font_ptr->replaced_avail = 0;
        }
        else
                if (read_byte(font_ptr, &byte) == 0)
                        return (-1);

        if (ignore_space_chars)
        {
                while (char_type[byte] & SPACE_CHAR)
                {
                        if (read_byte(font_ptr, &byte) == 0)
                                return (-1);
                }
        }
        else
                if (byte == ' ')
                {
                        *buf = 0;
                        return (0);
                }

        *buf++ = byte;
        len = 1;

        if (char_type[byte] & PUNCT_CHAR)
        {
                if (byte == '(')
                        font_ptr->str_level++;

                if (byte == ')')
                        font_ptr->str_level--;

                if (font_ptr->str_level == 0)
                {
                        if (byte == '%')
                        {
                                do
                                {
                                        if (read_byte(font_ptr, &byte) == 0)
                                                return (-1);
                                } while ((char_type[byte] & EOL_CHAR) == 0);

                                buf -= len;
                                goto rd_next_line;
                        }

                        if (byte == '{')
                                font_ptr->proc_level++;

                        if (byte == '}')
                                font_ptr->proc_level--;

                }

                *buf = 0;
                return (len);

        }

        count --;

        while (read_byte(font_ptr, &byte))
        {
                if (char_type[byte] & SPACE_CHAR)
                {
                        *buf = 0;
                        return (len);
                }

                if (char_type[byte] & PUNCT_CHAR)
                {
                        font_ptr->replaced_byte = byte;
                        font_ptr->replaced_avail = 1;
                        *buf = 0;
                        return (len);
                }

                if (len < count)
                {
                        *buf++ = byte;
                        len++;
                }
                else
                        sp_report_error(PARAMS2 TR_TOKEN_LARGE);

        }

        return (-1);

}


#define GET_BYTE( font, byte )                          \
{                                                       \
        if (buf >= buf_end)                             \
        {                                               \
                buf = font->buf_start;                  \
                buf_end = fill_get_byte_buf( font );    \
                if (buf >= buf_end)                     \
                        return (-1);                    \
        }                                               \
        byte = *buf++;                                  \
}

#define DECRYPT_BYTE( font, byte )                      \
{                                                       \
        register        ufix16 tmp;                     \
                                                        \
        tmp = r;                                        \
        r += byte;                                      \
        tmp >>= 8;                                      \
        byte ^= tmp;                                    \
        r *= DECRYPT_C1;                                \
        r += DECRYPT_C2;                                \
}


FUNCTION LOCAL_PROTO fix15 read_plain_token( font_data STACKFAR *font, char *token, fix15 count, fix15 ignore_space_chars)
{
        register        ufix8   *buf;
        register        ufix8   *buf_end;
        fix15           len;
        ufix8           byte;

        buf = font->buf;
        buf_end = font->buf_end;

rd_next_line:

        if (font->replaced_avail)
        {
                byte = font->replaced_byte;
                font->replaced_avail = 0;
        }
        else
                GET_BYTE( font, byte );

        if (ignore_space_chars)
        {
                while (char_type[byte] & SPACE_CHAR)
                        GET_BYTE( font, byte );
        }
        else
                if (byte == ' ')
                {
                        font->buf = buf;
                        *token = 0;
                        return (0);
                }

        *token++ = byte;
        len = 1;

        if (char_type[byte] & PUNCT_CHAR)
        {
                if (byte == '(')
                        font->str_level++;

                if (byte == ')')
                        font->str_level--;

                if (font->str_level == 0)
                {

                        if (byte == '%')
                        {
                                do
                                {
                                        GET_BYTE( font, byte );
                                } while ((char_type[byte] & EOL_CHAR) == 0);

                                token -= len;
                                goto rd_next_line;
                        }

                        if (byte == '{')
                                font->proc_level++;

                        if (byte == '}')
                                font->proc_level--;

                }

        }
        else
        {
                while (len < count)
                {
                        GET_BYTE( font, byte );

                        if (char_type[byte] & SPACE_CHAR)
                                break;

                        if (char_type[byte] & PUNCT_CHAR)
                        {
                                font->replaced_byte = byte;
                                font->replaced_avail = 1;
                                break;
                        }

                        *token++ = byte;
                        len++;
                }
        }

        font->buf = buf;

        if (len < count)
        {
                *token = 0;
                return (len);
        }
        else
        {
                *(--token) = 0;
                
                if (skip_rest_of_token(font))
                        return (-2);
                else
                        return (-1);
        }
}

FUNCTION LOCAL_PROTO fix15 read_encrypted_token(font_data *font, char *token, fix15 count, fix15 ignore_space_chars)
{
        register        ufix16  r;
        register        ufix8   *buf;
        register        ufix8   *buf_end;

                        ufix16  byte;
                        fix15   len;

        buf = font->buf;
        buf_end = font->buf_end;
        r = font->decrypt_r;

rd_next_line:

        if (font->replaced_avail)
        {
                byte = font->replaced_byte;
                font->replaced_avail = 0;
        }
        else
        {
                GET_BYTE( font, byte );
                DECRYPT_BYTE( font, byte );
        }

        if (ignore_space_chars)
        {
                while (char_type[byte] & SPACE_CHAR)
                {
                        GET_BYTE( font, byte );
                        DECRYPT_BYTE( font, byte );
                }
        }
        else
                if (byte == ' ')
                {
                        font->decrypt_r = r;
                        font->buf = buf;
                        *token = 0;
                        return (0);
                }

        *token++ = byte;
        len = 1;

        if (char_type[byte] & PUNCT_CHAR)
        {
                if (byte = '(')
                        font->str_level++;

                if (byte = ')')
                        font->str_level--;

                if (font->str_level == 0)
                {
                        if (byte == '%')
                        {
                                do
                                {
                                        GET_BYTE( font, byte );
                                        DECRYPT_BYTE( font, byte );
                                } while ((char_type[byte] & EOL_CHAR) == 0);

                                token -= len;
                                goto rd_next_line;
                        }

                        if (byte = '{')
                                font->proc_level++;

                        if (byte = '}')
                                font->proc_level--;
                }
        }
        else
        {

                while (len < count)
                {
                        GET_BYTE( font, byte );
                        DECRYPT_BYTE( font, byte );

                        if (char_type[byte] & SPACE_CHAR)
                                break;

                        if (char_type[byte] & PUNCT_CHAR)
                        {
                                font->replaced_byte = byte;
                                font->replaced_avail = 1;
                                break;
                        }

                        *token++ = byte;
                        len++;
                }
        }

        font->decrypt_r = r;
        font->buf = buf;

        if (len < count)
        {
                *token = 0;
                return (len);
        }
        else
        {
                *(--token) = 0;
                
                if (skip_rest_of_token( font ))
                        return (-2);
                else
                        return (-1);
        }
}

#undef  DECRYPT_BYTE
#undef  GET_BYTE

FUNCTION LOCAL_PROTO fix15 skip_rest_of_token(font_data STACKFAR *font)
{
        ufix8   byte;

        while (get_byte( font, &byte ))
        {
                if (char_type[byte] & SPACE_CHAR)
                        return (1);

                if (char_type[byte] & PUNCT_CHAR)
                {
                        font->replaced_byte = byte;
                        font->replaced_avail = 0;
                        return (1);
                }
        }

        return (0);
}

FUNCTION LOCAL_PROTO fix15 read_string(font_data STACKFAR *font, char STACKFAR *buf, fix15 count)

/*
 * read_string (FP, BUF, COUNT) Reads a string from a file (or stdin) Returns
 * a 0-terminated string in 'buf' with a maximum length of 'count' bytes.
 * Return value = length of string in buf (-1 if EOF found and no input). 
 */

{
        fix15   len;
        fix15   parentheses;
	ufix8   byte;

	if (font->replaced_avail)
        {
		byte = font->replaced_byte;
		font->replaced_avail = FALSE;
        }
	else
                if (!read_byte(font, (ufix8 STACKFAR *)&byte))
                        return -1;

	if (byte != '(')	/* First char not open parenthesis? */
		return -1;

	parentheses = 0;	/* Initialize buffer index */
	len = 0;		/* Initialize parenthesis count */
	count--;		/* Adjust count to leave space for terminator */

	while (read_byte( font, &byte))
	{
                if (byte == ')')
                        if (parentheses-- == 0)
                                break;

                if (len < count)
                {
                        *buf++ = byte;
                        len++;
                }
                
                if (byte == '(')
                        parentheses++;
        }

        *buf = 0;

        return (len);

}


FUNCTION LOCAL_PROTO fix15 read_binary(font_data STACKFAR *font, char STACKFAR *buf, fix15 count)

/*
 * Reads up to count bytes of binary data into the byte array buf Returns
 * number of bytes read 
 */

{
	fix15   read;

        if (font->hex_mode == 0)
        {
#if INCL_PFB
                if (count <= font->tag_bytes)
#endif
                {
                        read = read_and_decrypt( font, (ufix8 *)buf, count );
#if INCL_PFB
                        font->tag_bytes -= read;
#endif
                        return (read);
                }
        }

        for (read = 0; read < count; read++)
                if (read_byte( font, (ufix8 *)buf++ ) == 0)
                        return (read);

        return (count);

}


FUNCTION LOCAL_PROTO fix15 skip_binary( font_data STACKFAR *font, fix15 count)
{
        ufix8   buf[128];
        fix15   buf_len;
        fix15   read;

        if (font->hex_mode == 0)
        {
#if INCL_PFB
                if (count <= font->tag_bytes)
#endif
                {
                        buf_len = 128;
                        read = 0;

                        while (count > 0)
                        {
                                if (count < buf_len)
                                        buf_len = count;

                                read += read_and_decrypt( font, buf, buf_len );
                                count -= buf_len;
                        }
#if INCL_PFB
                        font->tag_bytes -= read;
#endif
                        return (read);
                }
        }

        for (read = 0; read < count; read++)
                if (read_byte( font, buf ) == 0)
                        break;

        return (read);

}

FUNCTION LOCAL_PROTO fix15 read_and_decrypt(font_data STACKFAR *font, ufix8 *buf, fix15 len)
{
        len = get_bytes( font, buf, len );

        if (font->decrypt_mode)
        {
                register        fix15   count;
                register        ufix16  r;
                register        ufix16  c1;
                register        ufix16  c2;
                register        ufix8   *plain_text;

                count = len;
                r = font->decrypt_r;
                c1 = DECRYPT_C1;
                c2 = DECRYPT_C2;
                plain_text = buf;

                while (count > 0)
                {
                        register        ufix8   tmp;

                        tmp = *plain_text;
                        *plain_text++ ^= (r >> 8);
                        r += tmp;
                        r *= c1;
                        r += c2;

                        count--;
                }

                font->decrypt_r = r;
        }

        return (len);
}

FUNCTION LOCAL_PROTO btsbool read_byte( font_data STACKFAR *font, ufix8 *pbyte)
{

        fix15   no_digits;
        ufix8   hex_digit;
        ufix8   byte;

        no_digits = 0;

        while (1)
        {
#if INCL_PFB
                if (font->tag_bytes == 0)
                        if (check_tag( font ) == 0)
                                return FALSE;
#endif
                if (get_byte( font, &byte ) == 0)
                        return FALSE;

#if INCL_PFB
                font->tag_bytes--;
#endif

                if (font->hex_mode)
                {
                        if (isalnum((char) byte) == 0)
                                continue;

                        if (no_digits == 0)
                        {
                                hex_digit = byte;
                                no_digits++;
                                continue;
                        }
                        else
                        {
                                byte = (asctohex(hex_digit) << 4) + asctohex(byte);
                                break;
                        }
                }
                else
                        break;
        }

        if (font->decrypt_mode)
        {
                register        ufix16  r;
                
                r = font->decrypt_r;

                *pbyte = byte ^ (r >> 8);
                r += byte;
                r *= DECRYPT_C1;
                r += DECRYPT_C2;

                font->decrypt_r = r;
        }
        else
                *pbyte = byte;

        return TRUE;

}

FUNCTION LOCAL_PROTO fix15 check_tag(font_data STACKFAR *font)
{
        ufix8   tag[6];

        if (font->font_file_type)
        {
                if (get_bytes(font, tag, 6) == 6)
                {
                        font->tag_bytes = *(fix31 *)tag;
                        font->tag_bytes -= 2;
                        return (1);
                }
        }
        else
        {
                if (get_byte(font, tag) && (tag[0] == 0x80))
                {
                        if (get_byte(font, tag))
                        {
                                if (tag[0] == 3)
                                        return (1);

                                if (get_bytes(font, tag, 4) == 4)
                                {
                                        font->tag_bytes = tag[0];
                                        font->tag_bytes += (fix31)tag[1] << 8;
                                        font->tag_bytes += (fix31)tag[2] << 16;
                                        font->tag_bytes += (fix31)tag[3] << 24;

                                        return (1);
                                }
                        }
                }
        }

        return (0);

}

FUNCTION LOCAL_PROTO btsbool parse_tag(font_data STACKFAR *font, fix31 *tag_bytes, ufix8 *tag_string)
{
        if (font->font_file_type)
        {
                *tag_bytes = (*(fix31 *)tag_string) - 2;
                return TRUE;
        }
        else
        {
                if (*tag_string++ == 0x80)
                {
                        if (*tag_string++ == 3)
                                return TRUE;

                        *tag_bytes = *tag_string++;
                        *tag_bytes += (fix31)*tag_string++ << 8;
                        *tag_bytes += (fix31)*tag_string++ << 16;
                        *tag_bytes += (fix31)*tag_string++ << 24;

                        return TRUE;
                }
        }

        return FALSE;
}


FUNCTION LOCAL_PROTO fix15 asctohex(ufix8 asciivalue)
{

        if ((asciivalue >= '0') && (asciivalue <= '9'))
                return (asciivalue - '0');

        if ((asciivalue >= 'A') && (asciivalue <= 'F'))
                return (asciivalue - 'A' + 10);

        if ((asciivalue >= 'a') && (asciivalue <= 'f'))
                return (asciivalue - 'a' + 10);

        return (0);

}

FUNCTION LOCAL_PROTO fix15 get_byte(font_data STACKFAR *font, ufix8 *byte)
{
        if (font->buf >= font->buf_end)
                if (fill_get_byte_buf(font) <= font->buf_start)
                        return (0);

        *byte = *font->buf++;
        return (1);
}

FUNCTION LOCAL_PROTO fix31 get_bytes(font_data STACKFAR *font, ufix8 *dest, ufix16 len)
{
        fix31   buf_len;

        buf_len = font->buf_end - font->buf;

        if (len <= buf_len)
        {
                memcpy(dest, font->buf, len);
                font->buf += len;
        }
        else
        {
                if (buf_len > 0)
                {
                        memcpy(dest, font->buf, (size_t)buf_len);
                        font->buf += buf_len;
                        dest += buf_len;
                        len -= buf_len;
                }

                if (len < font->buf_size)
                {
                        if (fill_get_byte_buf(font) <= font->buf_start)
                                return (buf_len);

                        memcpy(dest, font->buf, len);
                        font->buf += len;
                }
                else
                {
                        len = t1_get_bytes(font, dest, font->offset, (fix31)len);
                        font->offset += len;
                }

                len += buf_len;
        }

        return (len);
}

FUNCTION LOCAL_PROTO ufix8 *fill_get_byte_buf( font_data STACKFAR *font)
{
        ufix8   *buf;
        fix31   read;

        buf = font->buf_start;
        read = t1_get_bytes(font, buf, font->offset, (fix31)font->buf_size);
        font->buf = buf;
        font->buf_end = buf + read;
        font->offset += read;

        return (font->buf_end);
}

FUNCTION LOCAL_PROTO fix31 get_file_pos(font_data STACKFAR *font)
{
        fix31   offset;

        offset = font->offset + (fix31)font->buf - (fix31)font->buf_end;

        return (offset);
}

FUNCTION LOCAL_PROTO void set_file_pos(font_data STACKFAR *font, fix31 offset)
{
        fix31   buf_offset;

        buf_offset = font->offset + (fix31)font->buf_start - (fix31)font->buf_end;

        if ((offset >= buf_offset) && (offset >= font->offset))
                font->buf = font->buf_start + offset - buf_offset;
        else
        {
                font->offset = offset;
                fill_get_byte_buf(font);
        }
}


#if DEBUG

FUNCTION LOCAL_PROTO void print_binary(SPGLOBPTR2 ufix8 *buf, fix15 n)
{
	ufix16          r = 4330;
	ufix8           byte;
	int             i, state;
	fix31           operand;
	ufix16          command;

	state = 0;
	for (i = 0; i < n; i++)
        {
		byte = buf[i] ^ (r >> 8);
		r = (buf[i] + r) * decrypt_c1 + decrypt_c2;

		if (i < 4)	/* Discard first 4 bytes */
			continue;

		switch (state)
                {
		case 0:	/* Initial state */
			if (byte < 32)
                        {	/* Command? */
				if (byte != 12)
                                {
					command = byte;
					do_command(command);
				}
                                else
					state = 1;
			}
                        else if (byte < 247)
                        {	/* 1-byte integer? */
				operand = (fix31) byte - 139;
				do_operand(operand);
			}
                        else if (byte < 251)
                        {	/* 2-byte positive integer? */
				operand = ((fix31) byte - 247) << 8;
				state = 2;
			}
                        else if (byte < 255)
                        {	/* 2-byte negative integer? */
				operand = -(((fix31) byte - 251) << 8);
				state = 3;
			}
                        else
                        {/* 5-byte integer? */
				operand = 0;
				state = 7;
			}
			break;

		case 1:	/* Second byte of 2-byte command */
			command = byte + 32;
			do_command(command);
			state = 0;
			break;

		case 2:	/* Second byte of 2-byte positive integer */
			operand += (fix31) byte + 108;
			do_operand(operand);
			state = 0;
			break;

		case 3:	/* Second byte of 2-byte negative integer */
			operand -= (fix31) byte + 108;
			do_operand(operand);
			state = 0;
			break;
		case 4:	/* Last byte of 5-byte integer */
			operand = (operand << 8) + (fix31) byte;
			do_operand(operand);
			state = 0;
			break;

		default:	/* Other bytes of 5-byte integer */
			operand = (operand << 8) + (fix31) byte;
			state--;
			break;
		}
	}

	if (state != 0)
		sp_report_error(PARAMS2 TR_PARSE_ERR);	/* ERROR MESSAGE "*** Parsing
						 * error in Character program
						 * string" */
#if DEBUG
        else
                _sPrintf("\n");
#endif
}



FUNCTION LOCAL_PROTO int do_command(ufix16 command)
{
	switch (command) {

	case 1:
		_sPrintf("hstem\n");
		break;

	case 3:
		_sPrintf("vstem\n");
		break;

	case 4:
		_sPrintf("vmoveto\n");
		break;

	case 5:
		_sPrintf("rlineto\n");
		break;

	case 6:
		_sPrintf("hlineto\n");
		break;

	case 7:
		_sPrintf("vlineto\n");
		break;

	case 8:
		_sPrintf("rrcurveto\n");
		break;

	case 9:
		_sPrintf("closepath\n");
		break;

	case 10:
		_sPrintf("callsubr\n");
		break;

	case 11:
		_sPrintf("return\n");
		break;

	case 13:
		_sPrintf("hsbw\n");
		break;

	case 14:
		_sPrintf("endchar\n");
		break;

	case 21:
		_sPrintf("rmoveto\n");
		break;

	case 22:
		_sPrintf("hmoveto\n");
		break;

	case 30:
		_sPrintf("vhcurveto\n");
		break;

	case 31:
		_sPrintf("hvcurveto\n");
		break;

	case 32:
		_sPrintf("dotsection\n");
		break;

	case 33:
		_sPrintf("vstem3\n");
		break;

	case 34:
		_sPrintf("hstem3\n");
		break;

	case 38:
		_sPrintf("seac\n");
		break;

	case 39:
		_sPrintf("sbw\n");
		break;

	case 44:
		_sPrintf("div\n");
		break;

	case 48:
		_sPrintf("callothersubr\n");
		break;

	case 49:
		_sPrintf("pop\n");
		break;

	case 65:
		_sPrintf("setcurrentpoint\n");
		break;

	default:
		if (command < 32)
			_sPrintf("command %d\n", command);
		else
			_sPrintf("command 12 %d\n", command - 32);
		break;

	}
}


FUNCTION LOCAL_PROTO int do_operand(fix31 operand)
{
	_sPrintf("%d ", operand);
}

#endif  /* DEBUG */


#if RESTRICTED_ENVIRON
FUNCTION LOCAL_PROTO btsbool clear_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr)
#else
FUNCTION LOCAL_PROTO void    clear_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr)
#endif
{
	int             i;

#if RESTRICTED_ENVIRON

	char           STACKFAR*notdef_ptr;
	ufix16         notdef_offset;
	ufix16         STACKFAR*encoding_ptr;
	ufix16         space_needed;

	space_needed = strlen((char *)notdef) + 1;

	if (!inquire_about_space(font_ptr, space_needed))
        {
		/* abandon ship */
		sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
		return FALSE;
	}

	/* allocate the space */
	notdef_ptr = (char STACKFAR*) ((ufix32) font_ptr + (ufix32) font_ptr->offset_from_top);
	notdef_offset = font_ptr->offset_from_top;
	font_ptr->offset_from_top += space_needed;
	encoding_ptr = (ufix16 STACKFAR*) ((ufix32) font_ptr + (ufix32) (font_ptr->encoding_offset));

#if WINDOWS_4IN1
 	lstrcpy(notdef_ptr, notdef);
#else
	strcpy(notdef_ptr, (char *)notdef);
#endif

	for (i = 0; i < 256; i++)
		encoding_ptr[i] = notdef_offset;

	return TRUE;

#else

	for (i = 0; i < 256; i++)
		font_ptr->encoding[i] = notdef;

#endif
}



FUNCTION LOCAL_PROTO btsbool add_encoding(SPGLOBPTR2 font_data STACKFAR *font_ptr, fix15 index, ufix8 STACKFAR *charactername)
{

#if NAME_STRUCT
	fix15           i;
#endif

#if RESTRICTED_ENVIRON
	char           STACKFAR *char_ptr;
	ufix16         space_needed;
	ufix16         STACKFAR *encoding_ptr;

#if WINDOWS_4IN1
	space_needed = lstrlen(charactername) + 1;
#else
	space_needed = strlen((char *)charactername) + 1;
#endif

	if (!get_space(PARAMS2 space_needed))
        {
		/* abandon ship */
		sp_report_error(PARAMS2 TR_BUFFER_TOO_SMALL);
		return FALSE;
	}

	encoding_ptr = (ufix16 STACKFAR*) ((ufix32) font_ptr + (ufix32) (font_ptr->encoding_offset));

	/* allocate the space */
	char_ptr = (char STACKFAR*) ((ufix32) font_ptr + (ufix32) font_ptr->offset_from_top);
	encoding_ptr[index] = font_ptr->offset_from_top;
	font_ptr->offset_from_top += space_needed;

#if WINDOWS_4IN1
	lstrcpy(char_ptr, charactername);
#else
	strcpy(char_ptr, (char *)charactername);
#endif

	return TRUE;

#else


	/* allocate storage for the charactername */
#if NAME_STRUCT
	if ((font_ptr->encoding[index] = (CHARACTERNAME *) malloc(sizeof(CHARACTERNAME))) == BTSNULL)
		return FALSE;
	if ((font_ptr->encoding[index]->char_name = (unsigned char *) malloc(strlen(charactername))) == BTSNULL)
		return FALSE;
	font_ptr->encoding[index]->count = strlen(charactername);

	for (i = 0; i < strlen(charactername); i++)
		font_ptr->encoding[index]->char_name[i] = charactername[i];
#else
	if ((font_ptr->encoding[index] = (CHARACTERNAME *) malloc(strlen((char *)charactername) + 1)) == BTSNULL)
		return FALSE;

#if WINDOWS_4IN1
	lstrcpy((char *)font_ptr->encoding[index], charactername);
#else
	strcpy((char *)font_ptr->encoding[index], (char *)charactername);
#endif
#endif

	return TRUE;
#endif

}


FUNCTION void standard_encoding(font_data STACKFAR *font_ptr)
{
	int     ii;

#if !RESTRICTED_ENVIRON
	/* don't deallocate charname_tbl! */
	if (font_ptr->encoding == (CHARACTERNAME **) charname_tbl)
		return;

	/* if there's a special encoding array, deallocate it */
	if (font_ptr->encoding)
        {
		for (ii = 0; ii < 256; ii++)
			if (font_ptr->encoding[ii])
				free(font_ptr->encoding[ii]);

		free(font_ptr->encoding);
	}
	/* and set encoding to standard */
	font_ptr->encoding = (CHARACTERNAME **) charname_tbl;
#endif
}


#if DEBUG

FUNCTION LOCAL_PROTO void print_encoding(font_data STACKFAR *font_ptr)
{
	int     ii;

#if RESTRICTED_ENVIRON
        ufix16          *encoding;
#else
        CHARACTERNAME  **encoding;
#endif

	_sPrintf("\nEncoding vector:\n");


#if RESTRICTED_ENVIRON
        encoding = (ufix16 STACKFAR *)((ufix32)font_ptr + (ufix32)font_ptr->encoding_offset);

        for (ii = 0; ii < 256; ii++)
                _sPrintf("%3d: %s\n", ii, (char *)((ufix32)font_ptr + (ufix32)encoding[ii]));
#else
        encoding = (CHARACTERNAME **)font_ptr->encoding;

	for (ii = 0; ii < 256; ii++)
		_sPrintf("%3d: %s\n", ii, encoding[ii]);
#endif

	_sPrintf("\n");
}
#endif


FUNCTION LOCAL_PROTO void clear_hints(font_data STACKFAR *font_ptr)
/*
 * Initializes font hint storage 
 */
{
	font_ptr->font_hints.unique_id = 0;
	font_ptr->font_hints.no_blue_values = 0;
	font_ptr->font_hints.pblue_values = font_ptr->blue_values;
	font_ptr->font_hints.no_other_blues = 0;
	font_ptr->font_hints.pother_blues = font_ptr->other_blues;
	font_ptr->font_hints.no_fam_blues = 0;
	font_ptr->font_hints.pfam_blues = font_ptr->fam_blues;
	font_ptr->font_hints.no_fam_other_blues = 0;
	font_ptr->font_hints.pfam_other_blues = font_ptr->fam_other_blues;
	font_ptr->font_hints.blue_scale = -1.0;
	font_ptr->font_hints.blue_shift = -1;
	font_ptr->font_hints.blue_fuzz = -1;
	font_ptr->font_hints.stdhw = 0.0;
	font_ptr->font_hints.stdvw = 0.0;
	font_ptr->font_hints.no_stem_snap_h = 0;
	font_ptr->font_hints.pstem_snap_h = font_ptr->stem_snap_h;
	font_ptr->font_hints.no_stem_snap_v = 0;
	font_ptr->font_hints.pstem_snap_v = font_ptr->stem_snap_v;
	font_ptr->font_hints.force_bold = FALSE;
	font_ptr->font_hints.language_group = 0;
}



#if DEBUG

FUNCTION LOCAL_PROTO void print_long_array(fix31 data[], fix15 n)
{
	fix15           i;

	_sPrintf("[");

	for (i = 0; i < n; i++)
        {
		if (i != 0)
			_sPrintf(" ");

		_sPrintf("%ld", data[i]);
	}
	_sPrintf("]");
}
#endif


#if DEBUG

FUNCTION LOCAL_PROTO void print_real_array(real data[], int n)
{
	int             i;

	_sPrintf("[");
	for (i = 0; i < n; i++)
        {
		if (i != 0)
			_sPrintf(" ");

		_sPrintf("%7.5f", data[i]);
	}
	_sPrintf("]");
}
#endif



FUNCTION char STACKFAR *tr_get_font_name(SPGLOBPTR1)
{
	return sp_globals.processor.type1.current_font->font_name;
}


FUNCTION char STACKFAR *tr_get_full_name(SPGLOBPTR1)
{
        return sp_globals.processor.type1.current_font->full_name;
}


FUNCTION char STACKFAR *tr_get_family_name(SPGLOBPTR1)
{
        return sp_globals.processor.type1.current_font->family_name;
}


FUNCTION btsbool tr_get_isFixedPitch(SPGLOBPTR1)
{
        return sp_globals.processor.type1.current_font->isFixedPitch;
}


FUNCTION fix15 tr_get_UnderlinePostition(SPGLOBPTR1)
{
        return sp_globals.processor.type1.current_font->UnderlinePosition;
}


FUNCTION fix15 tr_get_UnderlineThickness(SPGLOBPTR1)
{
        return sp_globals.processor.type1.current_font->UnderlineThickness;
}


FUNCTION void tr_get_font_matrix(SPGLOBPTR2 real STACKFAR *matrix)
{
	int             i;

	for (i = 0; i < 6; i++)
		*(real STACKFAR*)(matrix+i) = sp_globals.processor.type1.current_font->font_matrix[i];
}


FUNCTION fbbox_t STACKFAR* tr_get_font_bbox(SPGLOBPTR1)
{
	return &sp_globals.processor.type1.current_font->font_bbox;
}


FUNCTION fix15 tr_get_paint_type(SPGLOBPTR1)
{
	return (fix15)sp_globals.processor.type1.current_font->paint_type;
}

FUNCTION CHARACTERNAME STACKFAR *tr_encode(SPGLOBPTR2 int i)
{

#if RESTRICTED_ENVIRON
	if (sp_globals.processor.type1.current_font->encoding_offset == (ufix16)BTSNULL)
		return (CHARACTERNAME STACKFAR *) charname_tbl[i];
#else
	return (CHARACTERNAME STACKFAR *) sp_globals.processor.type1.current_font->encoding[i];
#endif
}


FUNCTION unsigned char STACKFAR* tr_get_subr(SPGLOBPTR2 int i)
{

#if RESTRICTED_ENVIRON
	subrs_t         STACKFAR *subrs_ptr;
        font_data       STACKFAR *font;
                                 
        font = sp_globals.processor.type1.current_font;

	subrs_ptr = (subrs_t STACKFAR*) ((ufix32) font + (ufix32) font->subrs_offset);
	return (unsigned char STACKFAR*) ((ufix32) font + (ufix32) subrs_ptr[i].data_offset);
#else
	return (unsigned char *) sp_globals.processor.type1.current_font->subrs[i].value;
#endif
}


#if RESTRICTED_ENVIRON
#if INCL_PFB

FUNCTION LOCAL_PROTO btsbool remove_tags(SPGLOBPTR2 fix31 tag_position, ufix8 STACKFAR *char_data, fix15 total_bytes)
{
        font_data       STACKFAR *font;
	btsbool         more_tags;

        font = sp_globals.processor.type1.current_font;

	more_tags = TRUE;

	while (more_tags)
        {
		/* get rid of those tags */
		if (parse_tag(font, &font->tag_bytes, (ufix8 STACKFAR *)&char_data[tag_position]) == FALSE)
                {
                        memcpy((void STACKFAR*) &char_data[tag_position], (void STACKFAR*) &char_data[tag_position + 6], (size_t) font->tag_bytes);
                        tag_position = tag_position + 6 + font->tag_bytes;
                        if (total_bytes - tag_position <= 0)
                                more_tags = FALSE;
                }
                else
                {
			sp_report_error(PARAMS2 TR_BAD_RFB_TAG);
			return FALSE;
		}
	}
	return TRUE;
}
#endif

FUNCTION LOCAL_PROTO void asc2bin_buffer(fix15 byte_count, unsigned char STACKFAR *char_data)
{
	int             j, k;

	/* convert the hex data to binary  */
	k = 0;
	for (j = 0; j < byte_count; j++)
        {
		while (!isalnum(*(char STACKFAR*)(char_data+k)))
			k++;
		*(unsigned char STACKFAR*)(char_data+j) = asctohex(*(unsigned char STACKFAR*)(char_data+k++)) << 4;
		while (!isalnum(*(char STACKFAR*)(char_data+k)))
			k++;
		*(unsigned char STACKFAR*)(char_data+j) = *(unsigned char STACKFAR*)(char_data+j) + asctohex(*(unsigned char STACKFAR*)(char_data+k++));
	}
}
#endif


FUNCTION unsigned char STACKFAR* tr_get_chardef(SPGLOBPTR2 CHARACTERNAME STACKFAR *charname)
{
	int             i;

#if RESTRICTED_ENVIRON
	charstrings_t   STACKFAR *ch_strings;

#if WINDOWS_4IN1
        ufix8   success;
#endif

        ufix8           STACKFAR *char_data;
        font_data       STACKFAR *font;

        font = sp_globals.processor.type1.current_font;

	ch_strings = (charstrings_t STACKFAR*)((ufix8 STACKFAR*)font + font->charstrings_offset);

	/* look through the entire charstrings dictionary */
	for (i = 0; i < font->no_charstrings; i++)
        {

		/* is this the character ? */
		if (compstr((CHARACTERNAME STACKFAR*)charname, (ufix8 STACKFAR*)font + ch_strings[i].key_offset) == 0)
                {
			/* is this character in memory ? */
			if (ch_strings[i].value_offset != 0)
				/* yes - just return the pointer */
				return (unsigned char STACKFAR*) ((ufix32) font + (ufix32) ch_strings[i].value_offset);
			else
				/*
				 * get the character data from the
				 * application 
				 */
			{
                                charstrings_t   *character;

                                character = ch_strings + i;

#if WINDOWS_4IN1
				char_data = dynamic_load(character->file_position, character->file_bytes, success);
#else
                                char_data = t1_dynamic_load(character->file_position, character->file_bytes);
#endif

#if INCL_PFB
				/*
				 * if this is a PFB file, get rid of the tag
				 * bytes 
				 */
				if (character->file_bytes > character->tag_bytes)
					if (remove_tags(PARAMS2 character->tag_bytes, char_data, character->file_bytes) == 0)
						return BTSNULL;
#endif
				/*
				 * if this is a PFA, but in hex ascii
				 * mode - convert to binary 
				*/
				if (character->hex_mode)
					asc2bin_buffer(character->file_bytes, char_data);
                                                                  

                                if (character->decrypt_mode)
                                {
                                        register        fix15 count;
                                        register        ufix16 r;
                                        register        ufix16 c1;
                                        register        ufix16 c2;
                                        register        ufix8  *plain_text;

                                        count = character->charstring_size;
                                        r = character->decryption_key;
                                        c1 = DECRYPT_C1;
                                        c2 = DECRYPT_C2;
                                        plain_text = char_data;

                                        while (count > 0)
                                        {
                                                ufix8 tmp;

                                                tmp = *plain_text;
                                                *plain_text++ = (tmp ^ (r >> 8));
                                                r = (tmp + r) * c1 + c2;

                                                count--;
                                        }
                                }
                                return (char_data);
                        }
                }
        }
#else
	for (i = 0; i < sp_globals.processor.type1.current_font->no_charstrings; i++)
		if (STRCMP((char *)charname, (char *)sp_globals.processor.type1.current_font->charstrings[i].key) == 0)
			return (unsigned char STACKFAR*) sp_globals.processor.type1.current_font->charstrings[i].value;
#endif
	sp_report_error(PARAMS2 TR_NO_FIND_CHARNAME);	/* ERROR MESSAGE "***
						 * get_chardef: Cannot find
						 * %s\n", charname); */
	return BTSNULL;
}


FUNCTION font_hints_t STACKFAR* tr_get_font_hints(SPGLOBPTR1)
{
	return &(sp_globals.processor.type1.current_font->font_hints);
}

#if RESTRICTED_ENVIRON
FUNCTION btsbool tr_set_encode(SPGLOBPTR2 ufix8 STACKFAR *font_ptr, char STACKFAR *set_array[256])
#else
FUNCTION int     tr_set_encode(SPGLOBPTR2 CHARACTERNAME STACKFAR *set_array[256])
#endif
{
	int             i, j;

#if NAME_STRUCT
#define CHSIZE sizeof(CHARACTERNAME)+set_array[i]->count
#else
#define CHSIZE strlen((char *)set_array[i])+1
#endif

#if RESTRICTED_ENVIRON
        font_data       *font;
	ufix16          space_needed;
	ufix16          save_offset;
                              
        font = (font_data *)font_ptr;

        save_offset = font->offset_from_top;

	font->offset_from_bottom = sp_globals.processor.type1.current_font->offset_from_bottom;

	if (font->encoding_offset == 0)
        {
		space_needed = 256 * sizeof(ufix16);

		if (!get_space(PARAMS2 space_needed))
			return FALSE;

		/* allocate the space */
		font->encoding_offset = font->offset_from_top;
		font->offset_from_top += space_needed;
	}

	for (i = 0; i < 256; i++)
		if (!add_encoding(PARAMS2 font, i, (ufix8 *)set_array[i]))
                {
			font->offset_from_top = save_offset;
			return FALSE;
		}

	return TRUE;
#else
	/* If null pointer passed, just return */
	if (!set_array)
		return TRUE;

	/* Else, allocate space for array of 256 char pointers */
	if ((sp_globals.processor.type1.current_font->encoding = (CHARACTERNAME **) malloc(256 * sizeof(CHARACTERNAME *))) == BTSNULL)
        {
		sp_report_error(PARAMS2 TR_NO_SPC_ENC_ARR);
		return FALSE;
	}

	/*
	 * For each element of set_array, allocate space to store the string
	 * in the font data structure. 
	 */

	for (i = 0; i < 256; i++)
        {
		if (STRCMP((char *)set_array[i], (char *)notdef) == 0)
			/* store pointer to ".notdef" string */
			sp_globals.processor.type1.current_font->encoding[i] = (CHARACTERNAME *) notdef;
		else {	/* if string is not ".notdef", allocate memory and copy it */
			/*
			 * if unable to allocate current string, free all
			 * memory allocated up to this point 
			 */

			if ((sp_globals.processor.type1.current_font->encoding[i] = (CHARACTERNAME *) malloc(CHSIZE)) == BTSNULL)
                        {
				for (j = 0; j < i; j++)
					if (sp_globals.processor.type1.current_font->encoding[j])
						free(sp_globals.processor.type1.current_font->encoding[j]);

				free(sp_globals.processor.type1.current_font->encoding);

				sp_report_error(PARAMS2 TR_NO_SPC_ENC_ARR);	/* ERROR MESSAGE "Unable
									 * to allocate storage
									 * for encoding array" */
				return FALSE;
			}
#if NAME_STRUCT
			sp_globals.processor.type1.current_font->encoding[i]->char_name =
				(unsigned char *) ((ufix32) (sp_globals.processor.type1.current_font->encoding[i])
					  + (ufix32) sizeof(CHARACTERNAME));
#endif
			STRCPY((char *)sp_globals.processor.type1.current_font->encoding[i], (char *)set_array[i]);
		}
	}
	return TRUE;
#endif
}

#if RESTRICTED_ENVIRON
FUNCTION char STACKFAR* STACKFAR* WDECL tr_get_encode(SPGLOBPTR2 ufix8 STACKFAR *font_ptr)
#else
FUNCTION CHARACTERNAME **tr_get_encode(SPGLOBPTR1)
#endif
{

#if RESTRICTED_ENVIRON
        font_data       *font;

        font = (font_data *)font_ptr;

	if (font->encoding_offset == (ufix16)BTSNULL)
		return ((char STACKFAR*STACKFAR*) charname_tbl);
	else
		return ((char STACKFAR*STACKFAR*)((ufix32)font + (ufix32)font->encoding_offset));
#else
	return (sp_globals.processor.type1.current_font->encoding);
#endif
}


FUNCTION void WDECL tr_unload_font(font_data STACKFAR *font_ptr)
{
	int             i;

#if !RESTRICTED_ENVIRON

	/*
	 * if font structure hasn't been allocated yet, nothing to do 
	 */
	if (!font_ptr)
		return;

	/* free and NULL ptrs to memory for each char_string */
	if (font_ptr->charstrings)
        {
		for (i = 0; i < font_ptr->no_charstrings; i++)
                {
			if (font_ptr->charstrings[i].key)
			{
				free(font_ptr->charstrings[i].key);
				font_ptr->charstrings[i].key = BTSNULL;
			}

			if (font_ptr->charstrings[i].value)
			{
				free(font_ptr->charstrings[i].value);
				font_ptr->charstrings[i].value = BTSNULL;
			}
		}
		free(font_ptr->charstrings);
		font_ptr->charstrings = BTSNULL;
	}
	/* free memory for subrs */
	if (font_ptr->subrs)
        {
		for (i = 0; i < font_ptr->no_subrs; i++)
                {
			if (font_ptr->subrs[i].value)
			{	
				free(font_ptr->subrs[i].value);
				font_ptr->subrs[i].value = BTSNULL;
			}
		}
		free(font_ptr->subrs);
		font_ptr->subrs = BTSNULL;
	}
	/*
	 * If not standard encoding, free each string in the encoding array,
	 * and the array itself 
	 */
	if (font_ptr->encoding && font_ptr->encoding != (CHARACTERNAME **) charname_tbl)
        {
		for (i = 0; i < 256; i++)
			if (font_ptr->encoding[i] && (font_ptr->encoding[i] != (CHARACTERNAME *) notdef))
                        {
#if NAME_STRUCT
				free(font_ptr->encoding[i]->char_name);
				font_ptr->encoding[i]->char_name = BTSNULL;
#endif
				free(font_ptr->encoding[i]);
				font_ptr->encoding[i] = BTSNULL;
			}
		free(font_ptr->encoding);
		font_ptr->encoding = BTSNULL;
	}
	/* free the font data struct */
	free((char *) font_ptr);
	font_ptr = BTSNULL;
#endif
}

FUNCTION void tr_error(SPGLOBPTR2 int errcode, font_data STACKFAR *font_ptr)
{
	sp_report_error(PARAMS2 (fix15)errcode);
	tr_unload_font(font_ptr);
}


#if RESTRICTED_ENVIRON
FUNCTION LOCAL_PROTO void unload_charstring(SPGLOBPTR1)
{
        font_data       *font;
        charstrings_t   STACKFAR *chars_ptr;
        fix15           ii;

        font = sp_globals.processor.type1.current_font;

        chars_ptr = (charstrings_t STACKFAR *)((ufix32) font + font->charstrings_offset);

        for (ii = 0; ii < font->no_charstrings; ii++)
        {
                if (chars_ptr[ii].value_offset == font->offset_from_bottom)
                {
#if RESTRICTED_DEBUG
                        _sPrintf("CharString for %s unloaded from offset %x\n", (char *)((ufix32)chars_ptr[ii].key_offset + (ufix32)font),
                                                                              font->offset_from_bottom);
#endif
                        chars_ptr[ii].value_offset = 0;
                        font->offset_from_bottom += (*(charstrings_t STACKFAR *)(chars_ptr+ii)).charstring_size;
                        font->no_charstrings_loaded--;
                        return;
                }
        }
}


FUNCTION LOCAL_PROTO btsbool get_space(SPGLOBPTR2 ufix16 space_needed)
{
        font_data       *font;
	ufix16          space_available;
                              
        font = sp_globals.processor.type1.current_font;

	/* is there room ? */
	if (inquire_about_space(font, space_needed))
		return TRUE;

	space_available = font->offset_from_bottom - font->offset_from_top;

	/* try throwing away some charstrings */
	while (font->no_charstrings_loaded != 0 && (space_available < space_needed))
        {
		unload_charstring(PARAMS1);
		space_available = font->offset_from_bottom - font->offset_from_top;
	}

	/* still no room ? */
	if (space_available < space_needed)
		return FALSE;

	return TRUE;
}

FUNCTION LOCAL_PROTO btsbool inquire_about_space(font_data STACKFAR *font, ufix16 space_needed)
{
	ufix16          space_available;

	/* is there room ? */
	space_available = font->offset_from_bottom - font->offset_from_top;

	if (space_available >= space_needed)
		return TRUE;

	return FALSE;
}
#endif

#if NAME_STRUCT

FUNCTION fix15 ns_strlen(CHARACTERNAME charname)
{
	return charname.count;
}

/* copy a charactername structer into another charactername structure */

FUNCTION fix15 ns_strcmp(CHARACTERNAME *char1, CHARACTERNAME *char2)
{
	fix15   ii;

	if (char1->count != char2->count)
		return 1;

	for (ii = 0; ii < char1->count; ii++)
		if (char1->char_name[ii] != char2->char_name[ii])
			return 1;

	return 0;
}

FUNCTION void ns_strcpy(CHARACTERNAME *char1, CHARACTERNAME *char2)
{
	fix15   ii;

	char1->count = char2->count;

	for (ii = 0; ii < char1->count; ii++)
		char1->char_name[ii] = char2->char_name[ii];

	return;
}

/* copy a string into a charactername structure */

FUNCTION void ns_string_to_struct(CHARACTERNAME *struct1, char *string1)
{
	int     ii;

	struct1->count = strlen(string1);

	for (ii = 0; ii < strlen(string1); ii++)
		struct1->char_name[ii] = string1[ii];

	return;
}
#endif

FUNCTION fix15 tr_get_leniv(SPGLOBPTR1)
{
	return (sp_globals.processor.type1.current_font->leniv);
}

#endif /* PROC_TYPE1 */
/* EOF tr_ldfnt.c */

