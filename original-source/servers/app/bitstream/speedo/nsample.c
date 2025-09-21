/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                         U.S. Patent No 4,785,391                           *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*****************************************************************************/
/********************* Revision Control Information **********************************
*                                                                                    
*     $Header: //bliss/user/archive/rcs/speedo/nsample.c,v 33.0 97/03/17 17:45:42 shawn Release $
*                                                                                    
*     $Log:	nsample.c,v $
*       Revision 33.0  97/03/17  17:45:42  shawn
*       TDIS Version 6.00
*       
*       Revision 32.2  96/07/02  12:05:06  shawn
*       Changed boolean to btsbool and NULL to BTSNULL.
*       Reordered sp_close_bitmap() and sp_set_bitmap_pixels() in bitmap function array bfuncs.
*       
*       Revision 32.1  96/06/05  15:42:08  shawn
*       Added support for the GrayWriter.
*       
*       Revision 32.0  95/02/15  16:35:49  shawn
*       Release
*       
*       Revision 31.2  95/01/12  11:16:42  roberte
*       Updated Copyright header.
*       
*       Revision 31.1  95/01/04  16:36:13  roberte
*       Release
*       
*       Revision 30.2  94/12/30  11:31:04  roberte
*       Included <sys/types.h> non-MSDOS builds.
*       
*       Revision 30.1  94/12/21  15:00:15  shawn
*        Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
*       
*       Revision 30.0  94/05/04  09:32:41  roberte
*       Release
*       
*       Revision 29.2  94/04/08  14:37:03  roberte
*       Release
*       
*       Revision 29.0  94/04/08  11:43:18  roberte
*       Release
*       
*       Revision 28.48  94/02/10  13:20:49  roberte
*       Enhanced if Microsoft macro to include Borland as well.
*       
*       Revision 28.47  93/08/31  09:36:42  roberte
*       Release
*       
*       Revision 28.46  93/08/27  14:22:25  roberte
*       Added, at Karen's request, the function sp_change_specs() which
*       we have shipped to several customers on request for a quick
*       method of changing the font specs when only changing pointsize.
*       This function is not called in the nsample program.
*       
*       Revision 28.45  93/08/16  14:29:15  roberte
*       Added INCL_QUICK flag for MULTIDEV declaration of bitmap_t bfuncs, and call
*       to sp_set_bitmap_device()
*       
*       Revision 28.44  93/05/17  10:12:07  roberte
*       Did nothing - testing RCS system.
*       
*       Revision 28.43  93/05/17  10:06:23  roberte
*       Moved #define SET_SPCS here. Put the global sp_globals into the "root" in this module.
*       
*       Revision 28.42  93/05/04  10:20:05  roberte
*       Employed PARAMS1&2 macro and GDECL throughout.
*       
*       Revision 28.37  93/03/15  12:45:51  roberte
*       Release
*       
*       Revision 28.11  93/03/11  15:21:20  roberte
*       changed all #if __MSDOS to #ifdef MSDOS
*       
*       Revision 28.10  93/03/09  12:08:01  roberte
*        Replaced #if INTEL tests with #ifdef MSDOS as appropriate.
*       
*       Revision 28.8  93/01/04  11:37:15  roberte
*       Changed declaration of "lines" to an int so that sscanf will work.
*       
*       Revision 28.7  92/11/24  12:45:31  laurar
*       delete control Z.
*       
*       Revision 28.6  92/10/19  17:42:57  laurar
*       renamed ptsize to lines (for lines per em) which is more accurate.
*       
*       Revision 28.4  92/10/07  11:58:23  laurar
*       add user option for ptsize;
*       clean up compiler warnings with some casts.
*       
*       Revision 28.3  92/10/01  15:36:22  laurar
*       nothing!
*       
*       Revision 28.2  92/10/01  15:08:13  laurar
*       output entire character encoding instead of just one character.
*       
*       Revision 28.1  92/06/25  13:39:37  leeann
*       Release
*       
*       Revision 27.1  92/03/23  13:59:18  leeann
*       Release
*       
*       Revision 26.1  92/01/30  16:57:46  leeann
*       Release
*       
*       Revision 25.2  91/07/10  11:50:03  leeann
*       make a dummy _fmalloc for unix
*       
*       Revision 25.1  91/07/10  11:04:12  leeann
*       Release
*       
*       Revision 24.2  91/07/10  11:00:52  leeann
*       A better fix for font data far in small and medium memory models
*       
*       Revision 23.1  91/07/09  17:58:43  leeann
*       Release
*       
*       Revision 22.1  91/01/23  17:17:51  leeann
*       Release
*       
*       Revision 21.2  91/01/22  13:34:15  leeann
*       accomodate font far and small data model
*       
*       Revision 21.1  90/11/20  14:37:21  leeann
*       Release
*       
*       Revision 20.2  90/11/19  12:49:12  leeann
*       Fix problem with INCL_KEYS and PROTOS_AVAIL bot set to one
*       
*       Revision 20.1  90/11/12  09:29:58  leeann
*       Release
*       
*       Revision 19.1  90/11/08  10:19:37  leeann
*       Release
*       
*       Revision 18.1  90/09/24  10:08:08  mark
*       Release
*       
*       Revision 17.1  90/09/13  15:58:27  mark
*       Release name rel0913
*       
*       Revision 16.1  90/09/11  13:08:46  mark
*       Release
*       
*       Revision 15.1  90/08/29  10:03:19  mark
*       Release name rel0829
*       
*       Revision 14.1  90/07/13  10:39:14  mark
*       Release name rel071390
*       
*       Revision 13.2  90/07/13  10:20:10  mark
*       fix use of get_cust_no for compilers that support
*       function prototyping
*       
*       Revision 13.1  90/07/02  10:38:14  mark
*       Release name REL2070290
*       
*       Revision 12.2  90/05/09  15:30:03  mark
*       fix reference to sp_get_cust_no so that it compiles
*       properly under REENTRANT_ALLOC with PROTOS_AVAIL
*       
*       Revision 12.1  90/04/23  12:12:00  mark
*       Release name REL20
*       
*       Revision 11.2  90/04/23  12:09:39  mark
*       remove MODE_0 reference
*       
*       Revision 11.1  90/04/23  10:12:07  mark
*       Release name REV2
*       
*       Revision 10.4  90/04/21  10:48:18  mark
*       added samples of use of multiple device support option
*       
*       Revision 10.3  90/04/18  14:07:30  mark
*       change bytes_read to a ufix16
*       
*       Revision 10.2  90/04/12  13:44:48  mark
*       add code to check for standard customer number
*       
*       Revision 10.1  89/07/28  18:09:05  mark
*       Release name PRODUCT
*       
*       Revision 9.2  89/07/28  18:06:30  mark
*       fix argument declaration of sp_open_outline
*       
*       Revision 9.1  89/07/27  10:22:21  mark
*       Release name PRODUCT
*       
*       Revision 8.1  89/07/13  18:18:48  mark
*       Release name Product
*       
*       Revision 7.1  89/07/11  09:00:29  mark
*       Release name PRODUCT
*       
*       Revision 6.3  89/07/09  13:19:46  mark
*       changed open_bitmap to use new high resolution
*       bitmap positioning
*       
*       Revision 6.2  89/07/09  11:47:19  mark
*       added check font font size > 64k if INCL_LCD = 0
*       to give meaningful error message and abort
*       
*       Revision 6.1  89/06/19  08:34:20  mark
*       Release name prod
*       
*       Revision 5.1  89/05/01  17:53:01  mark
*       Release name Beta
*       
*       Revision 4.2  89/05/01  17:12:59  mark
*       use standard includes if prototypes are available
*       
*       Revision 4.1  89/04/27  12:13:57  mark
*       Release name Beta
*       
*       Revision 3.2  89/04/27  10:28:20  mark
*       Change handling for character indices in main loop,
*       ignore no character data error in report_error
*       Use 16 bit parameter to fread and malloc
*       
*       Revision 3.1  89/04/25  08:26:59  mark
*       Release name beta
*       
*       Revision 2.2  89/04/12  12:17:01  mark
*       added stuff for far stack and font
*       
*       Revision 2.1  89/04/04  13:33:33  mark
*       Release name EVAL
*       
*       Revision 1.14  89/04/04  13:19:41  mark
*       Update copyright text
*       
*       Revision 1.13  89/03/31  14:47:10  mark
*       recode to use public speedo.h and not using redefinition
*       of all functions (code reentrant version inline)
*       
*       Revision 1.12  89/03/30  11:33:30  john
*       Decryption keys 9, 10, 11 deleted.
*       
*       Revision 1.11  89/03/30  08:32:57  john
*       report_error() messages corrected.
*       
*       Revision 1.10  89/03/29  16:14:11  mark
*       changes for slot independence and dynamic/reentrant
*       data allocation
*       
*       Revision 1.9  89/03/28  10:50:44  john
*       Added first char index mechanism
*       Replaced NEXT_WORD_U() with read_2b().
*       
*       Revision 1.8  89/03/24  16:29:39  john
*       Import width error (6) deleted.
*       Error 12 text updated.
*       
*       Revision 1.7  89/03/24  15:14:30  mark
*       Clean up format for release
*       
*                                                                                    
*************************************************************************************/


/*************************** N S A M P L E . C *******************************
 *                                                                           *
 *                 SPEEDO CHARACTER GENERATOR TEST MODULE                    *
 *                                                                           *
 * This is an illustration of what external resources are required to        *
 * load a Speedo outline and use the Speedo character generator to generate  *
 * bitmaps or scaled outlines according to the desired specification.        *                                                    *
 *                                                                           *
 * This program loads a Speedo outline, defines a set of character           *
 * generation specifications, generates bitmap (or outline) data for each    *
 * character in the font and prints them on the standard output.             *
 *                                                                           *
 * If the font buffer is too small to hold the entire font, the first        *
 * part of the font is loaded. Character data is then loaded dynamically     *
 * as required.                                                              *
 *                                                                           *
 ****************************************************************************/

#include <stdio.h>

#ifdef MSDOS
#include <stdlib.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#else
#include <sys/types.h>
#endif

#define SET_SPCS

#include "spdo_prv.h"           /* General definition for make_bmap */
#include "keys.h"               /* Font decryption keys */

#ifdef MSDOS
GLOBAL_PROTO main(int argc, char *argv[]);
#else
GLOBAL_PROTO void* malloc(size_t n);
#endif

#define DEBUG  0

#if DEBUG
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#define MAX_BITS  256              /* Max line length of generated bitmap */

#define THE_MODE  MODE_BLACK

/***** GLOBAL FUNCTIONS *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC FUNCTIONS *****/

LOCAL_PROTO fix31 read_4b(ufix8 FONTFAR *ptr);
LOCAL_PROTO fix15 read_2b(ufix8 FONTFAR *ptr);

/***** STATIC VARIABLES *****/
static  char            pathname[100];  /* Name of font file to be output */
static  ufix8 FONTFAR  *font_buffer;    /* Pointer to allocated Font buffer */
static  ufix8 FONTFAR  *char_buffer;    /* Pointer to allocate Character buffer */
static  buff_t          font;           /* Buffer descriptor for font data */

static	ufix16         *font_table = BTSNULL;  /* allocate as much space as you need */

#if INCL_LCD
static  buff_t char_data;          /* Buffer descriptor for character data */
#endif

static  FILE  *fdescr;          /* Speedo outline file descriptor */
static  ufix16 char_index;      /* Index of character to be generated */
static  ufix16 char_id;         /* Character ID */
static  ufix16 minchrsz;        /* minimum character buffer size */

static  ufix8  key[] = 
{
    KEY0, 
    KEY1, 
    KEY2, 
    KEY3, 
    KEY4, 
    KEY5, 
    KEY6, 
    KEY7, 
    KEY8
};                             /* Font decryption key */

static  fix15  raswid;          /* raster width  */
static  fix15  rashgt;          /* raster height */
static  fix15  offhor;          /* horizontal offset from left edge of emsquare */
static  fix15  offver;          /* vertical offset from baseline */
static  fix15  set_width;       /* character set width */
static  fix15  y_cur;           /* Current y value being generated and printed */

static  char   line_of_bits[2 * MAX_BITS + 1]; /* Buffer for row of generated bits */

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_WHITE || INCL_SCREEN || INCL_2D || INCL_QUICK || INCL_GRAY
bitmap_t bfuncs = { sp_open_bitmap, sp_set_bitmap_bits, sp_set_bitmap_pixels, sp_close_bitmap };
#endif

#if INCL_OUTLINE
outline_t ofuncs = { sp_open_outline, sp_start_new_char, sp_start_contour, sp_curve_to,
                     sp_line_to, sp_close_contour, sp_close_outline };
#endif
#endif  /* INCL_MULTIDEV */

ufix8   temp[16];             /* temp buffer for first 16 bytes of font */

#if (defined (M_I86CM) || defined (M_I86LM) || defined (M_I86HM) || defined (M_I86SM) || defined (M_I86MM) || defined (__TURBOC__))
  /* then we are using a Microsoft or Borland compiler */
void far *_fmalloc(size_t size);
#else
  /* need to have a dummy _fmalloc for unix */
FUNCTION void *_fmalloc(size_t size)
{
}
#endif

FUNCTION main(int argc, char *argv[])
{
   int          lines;
   specs_t      specs;                  /* Bundle of character generation specs  */
   ufix32       first_char_index;       /* Index of first character in font */
   ufix32       no_layout_chars;        /* number of characters in layout */
   ufix32       i;
   ufix32       minbufsz;               /* minimum font buffer size to allocate */
   ufix16       cust_no;
   ufix16       bytes_read;             /* Number of bytes read from font file */
   ufix8        user;

   ufix8 FONTFAR *byte_ptr;
   
#if REENTRANT_ALLOC
SPEEDO_GLOBALS *sp_global_ptr;
#endif

    if (argc < 2) 
        {
        fprintf(stderr,"\nUsage: nsample {fontfile} {lines per em}\n");
        fprintf(stderr,"Lines per em is optional.  Default is 25.\n\n");
        exit (1);
        }

    sprintf(pathname, argv[1]);

        /* Load Speedo outline file */

    fdescr = fopen (pathname, "rb");
    if (fdescr == BTSNULL)
        {
        fprintf(stderr,"****** Cannot open file %s\n", pathname);
        exit(1);
        }

    if (argc > 2)
        sscanf(argv[2], "%d", &lines);
    else
        lines = 25;
        
        /* get minimum font buffer size - read first 16 bytes to get the minimum
           size field from the header, then allocate buffer dynamically  */

    bytes_read = fread(temp, sizeof(ufix8), 16, fdescr);
    
    if (bytes_read != 16)
        {
        fprintf(stderr,"****** Error on reading %s: %x\n", pathname, bytes_read);
        fclose(fdescr);
        exit(1);
        }

#if INCL_LCD
    minbufsz = (ufix32)read_4b(temp+FH_FBFSZ);
#else
    minbufsz = (ufix32)read_4b(temp+FH_FNTSZ);
    if (minbufsz >= 0x10000)
        {
        fprintf(stderr,
        "****** Cannot process fonts greater than 64K - use dynamic character loading configuration option\n");
        close(fdescr);
        exit(1);
        }
#endif

    if (sizeof(font_buffer) > sizeof(char *))
        font_buffer = (ufix8 FONTFAR *)_fmalloc((size_t)minbufsz);
    else
        font_buffer = (ufix8 *)malloc((size_t)minbufsz);

    if (font_buffer == BTSNULL)
        {
        fprintf(stderr,"****** Unable to allocate memory for font buffer\n");
        fclose(fdescr);
        exit(1);
        }

#if DEBUG
    printf("Loading font file %s\n", pathname);
#endif

    fseek(fdescr, (ufix32)0, 0);
    if (sizeof(font_buffer) > sizeof(char *))
        {
        byte_ptr = font_buffer;
        for (i=0; i< minbufsz; i++)
            {
            int ch;
            ch = getc(fdescr);
            if (ch == EOF)
                {
                fprintf (stderr,"Premature EOF in reading font buffer, %ld bytes read\n",i);
                exit(2);
                }
            *byte_ptr=(ufix8)ch;
            byte_ptr++;
            }
        bytes_read = (ufix16)i;
        }
    else
        {
        bytes_read = fread((ufix8 *)font_buffer, sizeof(ufix8), (ufix16)minbufsz, fdescr);
        if (bytes_read == 0)
            {
            fprintf(stderr,"****** Error on reading %s: %x\n", pathname, bytes_read);
            fclose(fdescr);
            exit(1);
            }
        }

#if INCL_LCD

        /* now allocate minimum character buffer */

    minchrsz = read_2b(font_buffer+FH_CBFSZ);

    if (sizeof(font_buffer) > sizeof(char *))
        char_buffer = (ufix8 FONTFAR *)_fmalloc(minchrsz);
    else
        char_buffer = (ufix8*)malloc(minchrsz);

    if (char_buffer == BTSNULL)
        {
        fprintf(stderr,"****** Unable to allocate memory for character buffer\n");
        fclose(fdescr);
        exit(1);
        }

#endif

#if DYNAMIC_ALLOC || REENTRANT_ALLOC
    sp_global_ptr = (SPEEDO_GLOBALS *)malloc(sizeof(SPEEDO_GLOBALS));
    memset(sp_global_ptr,(ufix8)0,sizeof(SPEEDO_GLOBALS));
#endif

        /* Initialization */

    sp_reset(PARAMS1);                  /* Reset Speedo character generator */

    font.org = font_buffer;
    font.no_bytes = bytes_read;

    if ((cust_no=sp_get_cust_no(PARAMS2 font)) != CUS0) /* NOT STANDARD ENCRYPTION */
        {
        fprintf(stderr,"Unable to use fonts for customer number %d\n", sp_get_cust_no(PARAMS2 font));
        fclose(fdescr);
        exit(1);
        }

#if INCL_KEYS
    sp_set_key(PARAMS2 key);            /* Set decryption key */
#endif

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_WHITE || INCL_SCREEN || INCL_2D || INCL_QUICK || INCL_GRAY
    set_bitmap_device(&bfuncs, sizeof(bfuncs));
#endif
#if INCL_OUTLINE
    set_outline_device(&ofuncs, sizeof(ofuncs));
#endif
#endif
    
    first_char_index = read_2b(font_buffer + FH_FCHRF);
    no_layout_chars = read_2b(font_buffer + FH_NCHRL);

    if ( (font_table = (ufix16 *)malloc((size_t)(no_layout_chars*sizeof(ufix16)))) == BTSNULL)
        {
        fprintf(stderr,"Can't allocate font_table.\n");
        exit(1);
        }

        /* Setup the character table */

    for (i=0; i < no_layout_chars; i++)
        font_table[i] = i + first_char_index;
        
        /* Set specifications for character to be generated */

    specs.pfont = &font;                /* Pointer to Speedo outline structure */
    specs.xxmult = (long)lines << 16;   /* Coeff of X to calculate X pixels */
    specs.xymult = 0L << 16;            /* Coeff of Y to calculate X pixels */
    specs.xoffset = 0L << 16;           /* Position of X origin */
    specs.yxmult = 0L << 16;            /* Coeff of X to calculate Y pixels */
    specs.yymult = (long)lines << 16;   /* Coeff of Y to calculate Y pixels */
    specs.yoffset = 0L << 16;           /* Position of Y origin */
    specs.out_info = BTSNULL;   
                                                       
        /* Mode flags */

    specs.flags = THE_MODE;
                    
#if INCL_GRAY
    sp_globals.pixelSize = 6;   /* Must be set '9<#>1' prior to sp_set_specs() call */
#endif

        /* Set character generation specifications */

    if (!sp_set_specs(PARAMS2 &specs))
        fprintf(stderr,"****** Cannot set requested specs\n");
    else
        {
        int incr = 5;

                /* Character generation loop */

        for(char_index = 0; char_index < no_layout_chars; char_index++, lines += incr)
            {

            char_id = sp_get_char_id(PARAMS2 char_index);

            if (!sp_make_char(PARAMS2 font_table[char_index]))
                fprintf(stderr,"****** Cannot generate character %d\n", char_index);

            if (lines >= 100)
                incr = -incr;
            else
                if (lines <= 5)
                    incr = -incr;

            }
        }

    fclose(fdescr);
    exit(0);
}


#if INCL_LCD

FUNCTION buff_t STACKFAR *WDECL sp_load_char_data(SPGLOBPTR2 fix31 file_offset, fix15 no_bytes, fix15 cb_offset)
/*
 * Called by Speedo character generator to request that character
 * data be loaded from the font file into a character data buffer.
 * The character buffer offset is zero for all characters except elements
 * of compound characters. If a single buffer is allocated for character
 * data, cb_offset ensures that sub-character data is loaded after the
 * top-level compound character.
 * Returns a pointer to a buffer descriptor.
 */
{
int     bytes_read, i;

ufix8  FONTFAR *byte_ptr;

#if DEBUG
    printf("\nCharacter data(%d, %d, %d) requested\n", file_offset, no_bytes, cb_offset);
#endif
    
    if (fseek(fdescr, (long)file_offset, (int)0) != 0)
        {
        printf("****** Error in seeking character\n");
        fclose(fdescr);
        exit(1);
        }

    if ((no_bytes + cb_offset) > minchrsz)
        {
        printf("****** Character buffer overflow\n");
        fclose(fdescr);
        exit(1);
        }
              
    if (sizeof(font_buffer) > sizeof(char *))
        {
        byte_ptr = char_buffer + cb_offset;
        bytes_read = 0;

        for (i=0; i<no_bytes; i++)
            {
            *byte_ptr = (ufix8)getc(fdescr);
            byte_ptr++;
            bytes_read++;
            }
        }
    else
        bytes_read = fread((char_buffer + cb_offset), sizeof(ufix8), no_bytes, fdescr);
        
    if (bytes_read != no_bytes)
        {
        printf("****** Error reading character data\n");
        fclose(fdescr);
        exit(2);
        }

#if DEBUG
    printf("Character data loaded\n");
#endif

    char_data.org = (ufix8 FONTFAR *)char_buffer + cb_offset;
    char_data.no_bytes = no_bytes;

    return &char_data;
}

#endif  /* INCL_LCD */



FUNCTION btsbool sp_change_specs(SPGLOBPTR2 fix31 scan_lines_x, fix31 scan_lines_y)

/*************************************************************************************
 * I wrote this "change_specs" for an application that was changing pointsize, and
 * was playing with scan_lines_x to scale in the x dimension.  It does all the things
 * such a function needs to do. It addresses sp_globals.specs directly, gets xmin, xmax,
 * ymin and ymax from the font header, calls sp_setup_consts() to initialize some
 * speedo globals, then calls sp_setup_tcb() to effect the changes internally. 
 * It assumes you have called sp_set_specs() successfully at least once.
*************************************************************************************/
{
fix15   xmin;          /* Minimum X ORU value in font */
fix15   xmax;          /* Maximum X ORU value in font */
fix15   ymin;          /* Minimum Y ORU value in font */
fix15   ymax;          /* Maximum Y ORU value in font */

#ifdef R_ROTATE_OR_OBLIQUE
/* this demonstrates how to use the oblique and rotation reals to fill in the specs structure: */
        /* coeff of x to calculate x pixels */
    sp_globals.specs.xxmult = (long)(scan_lines_x * cos(r_rotation)) << 16;
        /* coeff of y to calculate x pixels */
    sp_globals.specs.xymult = (long)(scan_lines_y * ( sin(r_rotation)+tan(r_oblique)*cos(r_rotation) )) << 16;
        /* coeff of x to calculate y pixels */
    sp_globals.specs.yxmult = (long)(scan_lines_x * (-1*sin(r_rotation))) << 16;
        /* coeff of y to calculate y pixels */
    sp_globals.specs.yymult = (long)(scan_lines_y * ( cos(r_rotation)-tan(r_oblique)*sin(r_rotation) )) << 16;
#else /* no rotation or oblique,; change point size only: */
        /* coeff of x to calculate x pixels */
    sp_globals.specs.xxmult = (long)scan_lines_x << 16;
        /* coeff of y to calculate x pixels */
    sp_globals.specs.xymult = 0L << 16;
        /* coeff of x to calculate y pixels */
    sp_globals.specs.yxmult = 0L << 16;
        /* coeff of y to calculate y pixels */
    sp_globals.specs.yymult = (long)scan_lines_y << 16;
#endif

	/* Set up sliding point constants */
	/* Set pixel shift to accomodate largest transformed pixel value */

    xmin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FXMIN);
    xmax = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FXMAX);
    ymin = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMIN);
    ymax = sp_read_word_u(PARAMS2 sp_globals.processor.speedo.font_org + FH_FYMAX);

    if (!sp_setup_consts(PARAMS2 xmin,xmax,ymin,ymax))
        {
        sp_report_error(PARAMS2 3);           /* Requested specs out of range */
        return FALSE;
        }

	/* Setup transformation control block */

    sp_setup_tcb(PARAMS2 &sp_globals.tcb0);

    return TRUE;
}



FUNCTION void sp_report_error(SPGLOBPTR2 fix15 n)
/*
 * Called by Speedo character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
{

switch(n)
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

default:
    printf("report_error(%d)\n", n);
    break;
    }
}


FUNCTION void sp_open_bitmap(SPGLOBPTR2
				fix31 x_set_width, fix31 y_set_width,
                                fix31 xorg, fix31 yorg,
                                fix15 xsize, fix15 ysize)

/* Called by output module to initialize a buffer prior to receiving bitmap data. */
{
fix15 i;

#if DEBUG
        printf("open_bitmap(%3.1f, %3.1f, %3.1f, %3.1f, %d, %d)\n",
            (real)x_set_width / 65536.0, (real)y_set_width / 65536.0,
            (real)xorg / 65536.0, (real)yorg / 65536.0, (int)xsize, (int)ysize);
#endif
            
    raswid = xsize;
    rashgt = ysize;
    offhor = (fix15)(xorg >> 16);
    offver = (fix15)(yorg >> 16);

    if (raswid > MAX_BITS)
        raswid = MAX_BITS;

    printf("\nCharacter index = %d, ID = %d\n", char_index, char_id);
    printf("set width = %3.1f, %3.1f\n", (real)x_set_width / 65536.0, (real)y_set_width / 65536.0);
    printf("X offset  = %d\n", offhor);
    printf("Y offset  = %d\n\n", offver);

    for (i = 0; i < raswid; i++)
        {
        line_of_bits[i << 1]   = '.';
        line_of_bits[(i<<1)+1] = ' ';
        }

    line_of_bits[raswid << 1] = '\0';
    y_cur = 0;

}


FUNCTION void sp_set_bitmap_bits(SPGLOBPTR2 fix15 y, fix15 xbit1, fix15 xbit2)

/* Called by output module to write one row of black pixels into the generated bitmap character. */
{
fix15 i;

#if DEBUG
    printf("set_bitmap_bits(%d, %d, %d)\n", (int)y, (int)xbit1, (int)xbit2);
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

#if DEBUG
    printf("set_bitmap_pixels(%d, %d, %d)\n", (int)y, (int)xbit1, (int)xbit2);
#endif
    
        /* Define xbit2 */

    xbit2 = xbit1 + num_grays;

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

/* Called by output module to indicate all bitmap data has been generated. */
{

#if DEBUG
    printf("close_bitmap()\n");
#endif

    printf("    %s\n", line_of_bits);
}


#if INCL_OUTLINE

FUNCTION void sp_open_outline(SPGLOBPTR2
				fix31 x_set_width, fix31 y_set_width,
                                fix31 xmin, fix31 xmax,
                                fix31 ymin, fix31 ymax)
/* Called by output module to initialize prior to outputting scaled outline data. */
{
    printf("\nopen_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
        (real)x_set_width / 65536.0,
        (real)y_set_width / 65536.0,
        (real)xmin / 65536.0,
        (real)xmax / 65536.0,
        (real)ymin / 65536.0,
        (real)ymax / 65536.0);
}



FUNCTION void sp_start_new_char(SPGLOBPTR1)
/*
 * Called by output module to initialize prior to
 * outputting scaled outline data for a sub-character in a compound
 * character.
 */
{
    printf("start_new_char()\n");
}


FUNCTION void sp_start_contour(SPGLOBPTR2 fix31 x, fix31 y, btsbool outside)
/*
 * Called by output module at the start of each contour
 * in the outline data of the character.
 */
{
    printf("start_contour(%3.1f, %3.1f, %s)\n", 
        (real)x / 65536.0,
        (real)y / 65536.0, 
        outside? "outside": "inside");
}


FUNCTION void sp_curve_to(SPGLOBPTR2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
/*
 * Called by output module once for each curve in the
 * scaled outline data of the character. This function is only called if curve
 * output is enabled in the set_specs() call.
 */
{
    printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n", 
        (real)x1 / 65536.0, (real)y1 / 65536.0,
        (real)x2 / 65536.0, (real)y2 / 65536.0,
        (real)x3 / 65536.0, (real)y3 / 65536.0);
}


FUNCTION void sp_line_to(SPGLOBPTR2 fix31 x, fix31 y)
/*
 * Called by output module once for each vector in the
 * scaled outline data for the character. This include curve data that has
 * been sub-divided into vectors if curve output has not been enabled
 * in the set_specs() call.
 */
{
    printf("line_to(%3.1f, %3.1f)\n", (real)x / 65536.0, (real)y / 65536.0);
}



FUNCTION void sp_close_contour(SPGLOBPTR1)

/* Called by output module at the end of each contour in the outline data of the character. */
{
     printf("close_contour()\n");
}


FUNCTION void sp_close_outline(SPGLOBPTR1)

/* Called by Speedo character generator at the end of output of the scaled outline of the character. */
{
    printf("close_outline()\n");
}

#endif  /* INCL_OUTLINE */


FUNCTION fix15 read_2b(ufix8 FONTFAR *pointer)
/*
 * Reads 2-byte field from font buffer 
 */
{
fix15 temp;
        
    temp = *pointer++;
    temp = (temp << 8) + *(pointer);

    return temp;
}




FUNCTION fix31 read_4b(ufix8 FONTFAR *pointer)
/*
 * Reads 4-byte field from font buffer 
 */
{
fix31 temp;
    
    temp = *pointer++;
    temp = (temp << 8) + *(pointer++);
    temp = (temp << 8) + *(pointer++);
    temp = (temp << 8) + *(pointer);

    return temp;
}
