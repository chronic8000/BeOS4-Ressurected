/*****************************************************************************
*                                                                            *
*  Copyright 1993, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                         U.S. Patent No 4,785,391                           *
*                           Other Patent Pending                             *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
*
*  $Header: //bliss/user/archive/rcs/4in1/fdump.c,v 14.0 97/03/17 17:19:39 shawn Release $
*
*  $Log:	fdump.c,v $
 * Revision 14.0  97/03/17  17:19:39  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.6  97/03/17  09:18:54  shawn
 * Corrected calculation of char_per_font in last of multiple fonts.
 * 
 * Revision 13.5  97/03/11  12:01:02  shawn
 * Modified to support '/y:??' command line option for independent y-size specification.
 * Modified to initialize boldValue for all processors.
 * 
 * Revision 13.4  96/08/20  12:45:43  shawn
 * Added RCS/Header keyword.
 * 
 * Revision 13.3  96/08/12  15:26:21  shawn
 * Changed 'boolean' to 'btsbool' and 'NULL' to 'BTSNULL'.
 * Added an example of INCL_MULTIDEV using new routine md_set_bitmap_pixels().
 * Added support for Graywriter with example screen display.
 * Added cspg_OpenNullCharShapeResource() and cspg_CloseCharShapeResource().
 * Added correct access to pCspGlobals and sp_global_ptr.
 * Added reentrant parameters to cspListCalBack().
 * Added t1_dynamic_load() t1_get_bytes() for RESTRICTED_ENVIRONMENT.
 * Added swap of the x & y Resolution values in soft font header.
 * 
 * Revision 13.2  95/11/17  10:24:50  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 13.1  95/05/12  11:21:25  roberte
 * Added some #if PROC_TRUETYPE conditionals that should have been here.
 * 
 * Revision 13.0  95/02/15  16:37:48  roberte
 * Release
 * 
 * Revision 12.5  95/02/01  16:41:25  shawn
 * Converted prototype for fs_SetUpKey() to a GLOBAL_PROTO for the K&R conversion utility
 * 
 * Revision 12.4  95/02/01  15:48:47  shawn
 * Added STACKFAR to local variable gspecs to match tspecs.Gen_specs
 * 
 * Revision 12.3  95/01/26  15:37:02  shawn
 * Inserted GLOBAL_PROTO Macro to cspg_OpenCharShapeResource() Prototype statements
 * 
 * Revision 12.2  95/01/11  13:53:02  shawn
 * Changed type of function tt_get_font_fragment() to (void *)
 * 
 * Revision 12.1  95/01/04  16:44:13  roberte
 * Release
 * 
 * Revision 11.8  94/12/30  11:23:59  roberte
 *  ANSI-ize all function prototypes and declarations. Employ FUNCTION,
 *  GLOBAL_PROTO and LOCAL_PROTO throughout. Make all functions
 *  explicitly ANSI. First pass.
 * 
 * Revision 11.7  94/12/28  11:35:03  roberte
 * Have main() return a value.
 * 
 * Revision 11.6  94/12/19  17:03:55  roberte
 * Added some sp_report_error() calls for TrueDoc CSP errors.
 * 
 * Revision 11.5  94/12/08  16:33:32  roberte
 * Warning suppression using correct casts.
 * Changed checking of fseek() to be more portable.
 * 
 * Revision 11.4  94/11/15  13:34:17  roberte
 * Set default for scan_lines to 24 lines, in case no command line args seen.
 * 
 * Revision 11.3  94/11/11  10:29:33  roberte
 * First intergation of TrueDoc - static model working OK.
 * Upgrade for 300/600 dpi conditional builds.
 * 
 * Revision 11.2  94/08/19  11:23:20  roberte
 * Created parameterized titling interface. The title function
 * is now hard-wired to 12 point Courier, in support of LJ2D printers.
 * Still some unexplained problems with LJ2 printers (but not the 2D).
 * Added gFontName variable, which for each technology is now
 * set to font name font in the font file. For TrueType, this
 * is rather non-trivial since the TT rasterizer ignores the name
 * table. So this code provides a useful example of examining some
 * of the TT header stuff, and in particular the name table.
 * Type1 font_name required a Type1 loader change to fill
 * in the sp_globals.processor.type1.current_font->font_name and
 * full_name members.
 * 
 * Revision 11.1  94/07/28  11:30:51  roberte
 * Make use of new pf_load_pcl_font() API.
 * 
 * Revision 11.0  94/05/04  09:39:46  roberte
 * Release
 * 
 * Revision 10.3  94/04/11  15:47:06  roberte
 * Accurate setting of fdump global no_font_chars from each
 * font file as appropriate to technology.
 * 
 * Revision 10.2  94/04/08  14:38:02  roberte
 * Release
 * 
 * Revision 10.1  94/04/08  13:54:53  roberte
 * Removed local declaration of ufe_struct tspecs in setup_eo_font(). Use global.
 * 
 * Revision 10.0  94/04/08  12:39:50  roberte
 * Release
 * 
 * Revision 9.7  94/04/08  11:17:24  roberte
 * Removed explicit settings of sp_globals.metric_resolution. These all set now in fi_set_specs() from font file.
 * 
 * Revision 9.6  94/02/03  16:26:05  roberte
 * Added D2.0 speedo font test.
 * Fixed parameter type in tt_release_font_fragment making BorlandC complain.
 * 
 * Revision 9.5  94/01/12  13:01:05  roberte
 * Made able to handle TrueType files > 64K.
 * 
 * Revision 9.4  93/11/09  13:50:35  roberte
 * Removed the WANT_SLOWER conditional which would foul up
 * composed character callbacks for PCLETTOs. This program is
 * still not PCLETTO ready, but it is closer.
 * 
 * Revision 9.3  93/10/05  12:53:05  roberte
 * Prettied up indenting and added handling of REMOTE char data from compressed RFS file.
 * 
 * Revision 9.2  93/08/30  15:57:28  roberte
 * Release
 * 
 * Revision 2.29  93/08/26  15:21:14  weili
 * Checked out revision 2.21 and added some code to make it run under UNIX
 * 
 * Revision 2.21  93/04/01  14:42:09  roberte
 * Really implemented FONTFAR and added code for INCL_LCD option so speedo fonts > 64K can be used.
 * 
 * Revision 2.20  93/03/25  12:58:43  weili
 * corrected the calculation mistake for "length" in sp_set_bitmap_bits()
 * 
 * Revision 2.19  93/03/15  13:53:52  roberte
 * Release
 * 
 * Revision 1.7  93/03/15  13:48:04  roberte
 * Added comments.
 * 
 * Revision 1.6  93/03/12  00:22:36  roberte
 * Added code to release/unload TrueType and PCL font memory.
 * 
 * Revision 1.5  93/03/11  10:49:48  weili
 * Changed type of argc to int from short.
 * 
 * Revision 1.4  93/03/10  19:54:43  weili
 * added log header
 * added "if (!set_width) in the beginning of
 * SetWidthToPixelWidth to avoid run time error
 * changed the declaration of argc from short to
 * int for portability
 * modified get_char_encoding() to fix the segmentation
 * fault.
 *                                                   *
*****************************************************************************/

/* !!!!!!!!!!!  RESTRICTED_ENVIRON NOT FUNCTIONING IN THIS .EXE !!!!! */

#ifdef MSDOS
#include <stddef.h>
#endif

#include <stdio.h>
#include <setjmp.h>

#ifdef MSDOS
#include <malloc.h>
#include <stdlib.h>
#else
#include <sys/types.h>
#endif

#include <string.h>

#ifdef MSDOS
#include <dos.h>
#endif

#include <math.h>

#ifdef MSDOS
#include <conio.h>
#endif

#include <time.h>



#define SET_SPCS
#include "spdo_prv.h"
#include "fino.h"

#if PROC_TRUEDOC
#include "csp_api.h"
#endif

#include "type1.h"
#include "fnt_a.h"
#include "tr_fdata.h"
#include "errcodes.h"
#include "hp_readr.h"
#include "finfotbl.h"
#include "ufe.h"		/* Unified front end */
#include "pcl5fnt.h"

#if PROC_TRUETYPE
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "sc.h"
#include "fnt.h"
#include "fontscal.h"
#include "fsglue.h"
#endif /* PROC_TRUETYPE */

#define DPI 300

/* Maximum bitmap width */
/* Enough for Roman characters at 300DPI */
#define MAX_BITS  256

#define THE_MODE MODE_QUICK

#if THE_MODE == MODE_GRAY
#define VIDEO_MAX       200
#define HORZ            320
#define VERT            195
#else
#define VIDEO_MAX       480
#define HORZ            640
#define VERT            470
#endif

#if defined(PCLETTO) || INCL_PCLETTO
#define MAX_CHARS	260

typedef struct
{
	ufix16 Unicode;
	ufix16 CharIndex;
	char *DataPtr;
	ufix32 DataLen;
}
GdirType;

GdirType gGdir[MAX_CHARS];
#endif /* defined(PCLETTO) || INCL_PCLETTO */

/***** STATIC FUNCTIONS *****/

LOCAL_PROTO	btsbool	open_font(char *pathname);
LOCAL_PROTO	btsbool	setup_type1_font(TPS_GPTR1);
LOCAL_PROTO	btsbool	setup_speedo_font(TPS_GPTR1);
LOCAL_PROTO	btsbool	setup_tt_font(TPS_GPTR1);
LOCAL_PROTO	btsbool	setup_eo_font(TPS_GPTR1);

#if PROC_TRUEDOC
LOCAL_PROTO btsbool setup_csp_font(TPS_GPTR1);
#endif

/* Routine for exemplifying INCL_MULTIDEV support */
LOCAL_PROTO     void md_set_bitmap_pixels(SPGLOBPTR2 fix15 y, fix15 xbit1,
                                                     fix15 num_grays,
                                                     void *gray_buf);

LOCAL_PROTO	void close_type1(void);
LOCAL_PROTO	void *get_char_encoding(ufix16 char_index);
LOCAL_PROTO     void close_font_file(void);

LOCAL_PROTO	fix15 read_2b(ufix8* pointer);
LOCAL_PROTO     fix31 read_4b(ufix8* pointer);

GLOBAL_PROTO int main(int argc, char *argv[]);


#ifdef MSDOS
/* vga driver */
LOCAL_PROTO void vga_CLS(void);
LOCAL_PROTO void set_backgd(void);
LOCAL_PROTO void vga_hline (int x, int y, int length);
LOCAL_PROTO void vga_ghline (int x, int y, int length, void *gray_buf);
#endif


/***** STATIC VARIABLES *****/
#if DEBUG
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

/* no longer have this from stdef.h */
#define   ABS(a)    (((a) >  0) ? (a) : -(a))

static	char	pathname[100];      /* Name of font file to be output */
static	char	gFontName[100];		/* Name of font */

static  	fix15  raswid;             /* raster width  */
static  	fix15  rashgt;             /* raster height */
static  	fix15  offhor;             /* horizontal offset from left edge of emsquare */
static  	fix15  offver;             /* vertical offset from baseline */
static  	fix15  y_cur;              /* Current y value being generated and printed */

static  	char   line_of_bits[2 * MAX_BITS + 1]; /* Buffer for row of generated bits */
static		char   title_str[64];

static  	fix15  x_scan_lines = 24;
static  	fix15  y_scan_lines = 24;
static          fix15  x_pointsize = 16;
static          fix15  y_pointsize = 16;

#if INCL_LCD
static  buff_t char_data;          /* Buffer descriptor for character data */
#endif

static  ufix16		minchrsz;       /* minimum character buffer size */
static	ufix16		char_index;
static	ufix8		*character;     /* can either be a charname for type1 */
					/* or a char index for speedo and tt */

static  int		x_dev, y_dev ;

static	CHARACTERNAME **font_encoding;
static	eFontProcessor	font_type;
static	eFontProtocol	font_protocol;
static	ufix16		no_font_chars;

/* initialized in setup_?_font functions */
static	ufix16		*font_table = NULL;     /* allocate as much space as you need */      
static	ufix8	FONTFAR *font_buffer = NULL;    /* allocate only if needed */
static  ufix8 	FONTFAR *char_buffer;           /* Pointer to allocate Character buffer */
static	ufix32		fontbuf_size;           /* size of font buffer */

static	buff_t		gbuff;
static	eofont_t	eo_font;
static  int		row_total=0, column_total=0, page_nr=0;

static	specs_t	STACKFAR gspecs;

#if RESTRICTED_ENVIRON
static	ufix8 	*font_ptr;
static 	ufix16 	buffer_size;
/* Read buffer for Type 1 Dynamic Load */
static	unsigned char big_buffer[10240];
#else
static	font_data *font_ptr;
#endif

static	FILE *font_fp = NULL;

#define PROOF_CHAR_HEIGHT 20
#define PCL_PT_PER_INCH 72.307
#define MS_PT_PER_INCH  72.0

#define swp16(X) X = (X << 8) | ((X >> 8)&0xFF)

#include "hpf.h"        /* hp soft font defs */

static char		char_set_name[32] = "8U";
static int		hp_char_id;
static HPF_HEAD		hpfhd;		/* hp header struct */
static HPF_CHARDIR	hpfcdir;	/* ?? */
static FILE		*hpf_fp;	/* hp soft font output file descriptor */
static btsbool		do_proof;
static ufe_struct	tspecs;

char transdata[] = "\33&p1X";

static  fix15	space_char;
static  fix15	_gxmin = 1000;
static  fix15	_gymin = 1000;
static  fix15	_gxmax = 1000;
static  fix15	_gymax = 1000;

static  ufix16	bitrow[MAX_BITS/16];
static	fix31	gBitmapWidth;

static char BitsString[] = "Bitstream, Inc.";

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_WHITE || INCL_SCREEN || INCL_2D || INCL_QUICK || INCL_GRAY
bitmap_t bfuncs = { sp_open_bitmap,
                    sp_set_bitmap_bits,
                    md_set_bitmap_pixels,       /* Special version of sp_set_bitmap_pixels() */
                    sp_close_bitmap };
#endif
#if INCL_OUTLINE
outline_t ofuncs = { sp_open_outline,
                     sp_start_new_char,
                     sp_start_contour,
                     sp_curve_to,
                     sp_line_to,
                     sp_close_contour,
                     sp_close_outline };
#endif
#endif  /* INCL_MULTIDEV */

#if PROC_TRUETYPE
static	int 	JIS_code = 0, JIS_Highbyte;
#endif

#if INTEL
ufix16 endmask[16] = {  
	0x0000, 0x0080, 0x00C0, 0x00E0,
	0x00F0, 0x00F8, 0x00FC, 0x00FE,
	0x00FF, 0x80FF, 0xc0FF, 0xE0FF,
	0xF0FF, 0xF8FF, 0xFCFF, 0xFEFF};
ufix16 startmask[16] = {
	0xFFFF, 0xFF7F, 0xFF3F, 0xFF1F,
	0xFF0F, 0xFF07, 0xFF03, 0xFF01,
	0xFF00, 0x7F00, 0x3F00, 0x1F00,
	0x0F00, 0x0700, 0x0300, 0x0100};
#else   /* Anything else */
ufix16 endmask[16] = {  
	0x0000, 0x8000, 0xC000, 0xE000,
	0xF000, 0xF800, 0xFC00, 0xFE00,
	0xFF00, 0xFF80, 0xFFC0, 0xFFE0,
	0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE};
ufix16 startmask[16] = {
	0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF,
	0x0FFF, 0x07FF, 0x03FF, 0x01FF,
	0x00FF, 0x007F, 0x003F, 0x001F,
	0x000F, 0x0007, 0x0003, 0x0001};
#endif /* MSDOS or Anything BUT MSDOS */

/* function prototypes for proofing utilities */
GLOBAL_PROTO void new_hpfhd(HPF_HEAD *);
GLOBAL_PROTO void swap_cdir(HPF_CHARDIR *);
GLOBAL_PROTO void hp_proof_prologue(char *);
GLOBAL_PROTO void hpwc(char *);
GLOBAL_PROTO void layout(int total_chars);
GLOBAL_PROTO void hp_proof_prologue_no_open(void);
GLOBAL_PROTO void hp_proof_epilog_no_close(int total_chars);

#if PROC_TRUETYPE
GLOBAL_PROTO fsg_SplineKey *fs_SetUpKey(fs_GlyphInputType *inptr, unsigned stateBits, int *error);
#endif

GLOBAL_PROTO fix31 SetWidthToPixelWidth(fix31 set_width);
GLOBAL_PROTO void PrintTitle(int xX, int yY, int PointSize, int Weight, char *String);

#if PROC_TRUEDOC
#if PFR_ON_DISK
GLOBAL_PROTO FILE *cspg_OpenCharShapeResource(PCSPCONTEXT char *pathname);
#else
GLOBAL_PROTO char *cspg_OpenCharShapeResource(PCSPCONTEXT char *pathname);
#endif
#if REENTRANT_ALLOC
GLOBAL_PROTO void *cspg_OpenNullCharShapeResource(void);
#endif
GLOBAL_PROTO btsbool cspg_CloseCharShapeResource(PCSPCONTEXT1);
#endif  /* PROC_TRUEDOC */

#if PROC_TRUETYPE
LOCAL_PROTO void Read_OffsetTable(FILE *fdes,sfnt_OffsetTable	*offset_table);
LOCAL_PROTO int Read_DirectoryTable(FILE *fdes,sfnt_OffsetTable* sfntdir);
LOCAL_PROTO int Get_TableOffsetLength(FILE *fdes,sfnt_OffsetTable* sfntdir,char *dir_name, uint32 *offset, uint32 *length);
LOCAL_PROTO int Get_TableOffsetLength(FILE *fdes,sfnt_OffsetTable* sfntdir,char *dir_name, uint32 *offset, uint32 *length);
#endif


int font_idx = 0;	/* used for font within TrueDoc PFR - default 1st font  */

/****************************************************************************
   FILE:       fdump.c
   This module demonstrates how to display or print all characters in a
   sample font. "all characters" means :
      Speedo      all characters
      TrueType    whatever can find in first 640 characters
      PCLeo       whatever can find in first 256 characters      
      Type1       whatever can find in first 256 characters
*****************************************************************************/

#if (defined (M_I86CM) || defined (M_I86LM) || defined (M_I86HM) || defined (M_I86SM) || defined (M_I86MM) || defined (__TURBOC__))
/* then we are using a Microsoft or Borland compiler */
#else
/* need to have a dummy _fmalloc for unix */
FUNCTION void *_fmalloc(size_t n)
{
}
/* need to have a dummy _ffree for unix */
FUNCTION void _ffree(ufix8 *p)
{
}
#endif

FUNCTION int main(int argc, char *argv[])
{
	btsbool		setup, getoutchar;
	unsigned char 	*buf, ch;
	ufix16  	char_id;

	int		total_chars;
	ufix16		char_per_font;
	char		tempstr[16];

	int i, j, k, hor_nr, ver_nr, hor_offset, ver_offset, font_nr;

#ifdef MSDOS
	union REGS	regs;
#endif


	/* check for proper usage */
	if (argc < 2) 
		usage();

#if DYNAMIC_ALLOC || (REENTRANT_ALLOC && !PROC_TRUEDOC)
	/* Allocate the Speedo Globals data structure */
#ifdef MSDOS
if (sizeof(SPEEDO_GLOBALS GLOBALFAR *) > sizeof(char *))
        if ( !(sp_global_ptr = (SPEEDO_GLOBALS GLOBALFAR *)_fmalloc(sizeof(SPEEDO_GLOBALS)) ))
                {
                printf("Unable to allocate Speedo Globals.\n");
                exit(1);
                }
        else
                _fmemset(sp_global_ptr, (char)0, sizeof(SPEEDO_GLOBALS));
else
#endif  /* MSDOS */
        if ( !(sp_global_ptr = (SPEEDO_GLOBALS GLOBALFAR *)malloc(sizeof(SPEEDO_GLOBALS)) ))
                {
                printf("Unable to allocate Speedo Globals.\n");
                exit(1);
                }
        else
                memset(sp_global_ptr, (char)0, sizeof(SPEEDO_GLOBALS));
#endif  /* DYNAMIC_ALLOC || (REENTRANT_ALLOC && !PROC_TRUEDOC) */

#if REENTRANT_ALLOC && PROC_TRUEDOC
pCspGlobals = cspg_OpenNullCharShapeResource(); /* Get the TrueDoc Context Pointer */
sp_global_ptr = GetspGlobalPtr(pCspGlobals);    /* Get the Speedo Globals Pointer  */
#endif  /* REENTRANT_ALLOC && PROC_TRUEDOC */

	sprintf(pathname, argv[1]);

	i = 2;	/* parse arguments, if any, after file name: */

	while (i < argc)
	{
		if ( strncmp(argv[i], "/p", 2) == 0)
		{   
			do_proof = TRUE;
			if (argv[i][2] == ':')
				sscanf(&argv[i][3],"%hd",&x_pointsize);

                        y_pointsize = x_pointsize;
		}
#ifdef MSDOS
		else if ( strncmp(argv[i], "/s", 2) == 0)
		{
			if (argv[i][2] == ':')
			{
				sscanf(&argv[i][3],"%hd",&x_scan_lines);
				if (x_scan_lines > 600)
					x_scan_lines = 600;
                                y_scan_lines = x_scan_lines;
			}
			else
				x_scan_lines = y_scan_lines = 24;
		}
#endif /* MSDOS */
                else if ( strncmp(argv[i], "/y:", 3) == 0)
                {
                
                        if (do_proof)
                        {

                                if (argv[i][2] == ':')
                                        sscanf(&argv[i][3],"%hd",&y_pointsize);

                        }
#ifdef MSDOS
                        else
                        {

                                if (argv[i][2] == ':')
                                        sscanf(&argv[i][3],"%hd",&y_scan_lines);

                                if (y_scan_lines > 600)
                                        y_scan_lines = 600;

                        }
#endif /* MSDOS */
                }
		else if ( strncmp(argv[i], "/f:", 3) == 0)
		{
			font_idx = atoi((char *)&argv[i][3]);
		}
		else
			usage();
		i++;
	}

#ifndef MSDOS
	/* assert any non PC-DOS requirements */
	do_proof = TRUE;
#endif

	if (!open_font(pathname))
	{
		fprintf(stderr,"Unable to open font file %s\n",pathname);
		exit (1);
	}

	if ( (buf = (unsigned char *)malloc((size_t)20)) == NULL)
	{
		fprintf(stderr,"Unable to allocate temporary buffer.\n");
		exit(1);
	}

	fread(buf, 1, 10, font_fp);
	fseek(font_fp, 0L, SEEK_SET);

#if INCL_GRAY
        sp_globals.pixelSize = 6;
#endif

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_WHITE || INCL_SCREEN || INCL_2D || INCL_QUICK || INCL_GRAY
    set_bitmap_device(&bfuncs, sizeof(bfuncs));
#endif
#if INCL_OUTLINE
    set_outline_device(&ofuncs, sizeof(ofuncs));
#endif
#endif

	/* determine type of font */
	if ( strncmp(buf, "%!", 2) == 0 )
		/* configured for Type 1 font */
		setup = setup_type1_font(TPS_GLOB1);        
	else if (buf[0]  == 0x80)
		/* check for pfb type1 font */
		setup = setup_type1_font(TPS_GLOB1);   
	else if ( (strncmp(buf, "D1.0",4) == 0) ||
		  (strncmp(buf, "D2.0",4) == 0) )
		/* configured for Speedo font */
		setup = setup_speedo_font(TPS_GLOB1);       
	else if (strncmp(buf, ")s", 3) == 0)
		setup = setup_eo_font(TPS_GLOB1);
	else 
		{/* could be TrueDoc PFR or TrueType */
		btsbool isTT = TRUE;	/* assume TT until proven otherwise */
		if (fseek(font_fp, -5L, SEEK_END) == 0)	/* position to last 5 bytes of file */
			{
			if (fread(buf, 1, 5, font_fp) == 5)
				{
				if (strncmp((char *)buf, "$PFR$", 5) == 0)
					{/* PFR for sure */
					isTT = FALSE;
					close_font_file(); /* cspg_OpenCharShapeResource will re-open */
#if PROC_TRUEDOC
#if REENTRANT_ALLOC
                                                /* Close the Null Csp Context */
                                        if (!cspg_CloseCharShapeResource(TPS_GLOB1))
                                                {
                                                printf("Unable to close Csp.\n");
                                                exit(1);
                                                }
#endif

					/* call glue func to open PFR, PFR_ON_DISK must be 1 */
					/* see SELSYM.c or 4TEST.c demo for PFR_ON_DISK == 0 support example */
					font_fp = cspg_OpenCharShapeResource(
#if REENTRANT_ALLOC
									    &pCspGlobals,
#endif
									    pathname);

					if (font_fp)
						setup = setup_csp_font(TPS_GLOB1);
					else
						{
						setup = FALSE;	/* well, we couldn't open the PFR! */
						fprintf(stderr,"Unable to open TrueDoc PFR: %s\n", pathname);
						exit(1);
						}
#else
					fprintf(stderr,"Not configured for TrueDoc PFRs!\n");
					exit(1);
#endif /* PROC_TRUEDOC */
					}
				}
			}
		if (isTT)
			{
			/* always rewind file to beginning: */
			fseek(font_fp, 0L, SEEK_SET);
			setup = setup_tt_font(TPS_GLOB1);
			}
		}

	free(buf);

	if (!setup)
		exit(setup);			/* error in setup */


	/* output to the printer */
	if (do_proof)
	{
		char     command[128];


		/* open output file */
		hp_proof_prologue("tmpout.bin");

		switch(font_type)
		{
		case procType1:
			sprintf(title_str, "Type1 Font %s", gFontName);
			fprintf(stderr, "Processing Type1 font %s ...\n", pathname);
			break;
		case procSpeedo:
			sprintf(title_str, "Speedo Font %s", gFontName);
			fprintf(stderr, "Processing Speedo font %s ...\n", pathname);
			break;
		case procTrueDoc:
			sprintf(title_str, "TrueDoc Font %s", gFontName);
			fprintf(stderr, "Processing TrueDoc PFR %s FontIndex %d ...\n", pathname, font_idx);
			break;
		case procTrueType:
			sprintf(title_str, "TrueType Font %s", gFontName);
			fprintf(stderr, "Processing TrueType font %s ...\n", pathname);
			break;
		case procPCL:
			sprintf(title_str, "PCLeo Font %s", gFontName);
			fprintf(stderr, "Processing PCLeo font %s ...\n", pathname);
			break;
		}

                /* Reset */
		hpwc("\033E");

		/* Delete all soft fonts */
		hpwc("\033*c0F");

		PrintTitle( 150/*xX*/, 150/*yY*/, 12/*PointSize*/, 3/*Weight*/, title_str/*String*/);
		PrintTitle( 1920/*xX*/, 150/*yY*/, 12/*PointSize*/, 0/*Weight*/, BitsString/*String*/);
                           
                /* Move to X=150 */
		hpwc("\033*p150X");		

		if ( no_font_chars <= 256 )
			font_nr = 1;
		else
		{
			font_nr =  no_font_chars / 256;
			if ( no_font_chars % 256 )
				font_nr++;
		}

                char_index = 0;

		for ( i=0; i < font_nr; i++)
		{
			hp_proof_prologue_no_open();
			hp_char_id = 0;

			if ( i == (font_nr-1) )
				char_per_font = (no_font_chars <= 256) ? no_font_chars : no_font_chars % 256;
                        else
                                char_per_font = 256;

			for ( j=0; j < char_per_font; j++ )   /* For each character in encoding */
			{
				character = get_char_encoding(char_index);
				getoutchar = (character != (void *)0);

				if (getoutchar)
				{
					gBitmapWidth = SetWidthToPixelWidth(fi_get_char_width(TPS_GLOB2 character));

					if (fi_make_char(TPS_GLOB2 character))
						hp_char_id++;
#if PROC_SPEEDO && INCL_REMOTE
                                        else
                                                /* done with possible remote character data, clear the FourIn1Flags */
                                                if (sp_globals.FourIn1Flags & BIT0)
                                                        sp_globals.FourIn1Flags ^= BIT0;
#endif
				}
				char_index++;

			}       /* End of charaacter loop */

			total_chars = hp_char_id;

                                /* finished, restore order to the Shire... */
			hp_proof_epilog_no_close(total_chars);

		}       /* End of font loop */

		page_nr++;

                /* Print page number */
		sprintf(tempstr, "Page %d", page_nr);

		PrintTitle( 1050/*xX*/, 3000/*yY*/, 12/*PointSize*/, 0/*Weight*/, tempstr/*String*/);

                /* Print the current page */
		hpwc("\033&l0H");

                /* Reset */
		hpwc("\33E");

                /* Handle non-300DPI proofs */
		if (DPI != 300)
			hpwc("\033%%-12345X");

		fclose(hpf_fp);
		return 0;
	}
#ifdef MSDOS
	else /* output on the screen */
	{


#if THE_MODE == MODE_GRAY

                /* set VGA to 320x200/256 color */
                regs.x.ax = 0x0013;
                int86(0x10, &regs, &regs);

                /* set the DAC to gray values */
                for (i=0; i<64; i++)
                {
                        j = 63 - i;
                        /* Modify individual DAC registers */
                        regs.x.ax = 0x1010;
                        regs.x.bx = (ufix16)i;                  /* DAC register # */
                        regs.x.dx = (ufix16)j<<8 | 0x00;        /* Red */
                        regs.x.cx = (ufix16)j<<8 | (ufix8)j;    /* Green & Blue */
                        int86(0x10, &regs, &regs);
                }

                /* Convert to true grayscale values */
                regs.x.ax = 0x101b;
                regs.x.bx = 0;                  /* Starting DAC # */
                regs.x.cx = 64;                 /* Number of DACs to modify */
                int86(0x10, &regs, &regs);
#else
                /* Set VGA to 640x480/16 color */
		regs.x.ax = 0x0012;
		int86(0x10, &regs, &regs);
#endif

		set_backgd();

		hor_nr = HORZ / ( x_scan_lines + x_scan_lines/2 ) ;
		ver_nr = VERT / ( y_scan_lines + y_scan_lines/2 ) ;
		hor_offset = ( HORZ - hor_nr * (x_scan_lines + x_scan_lines/2) ) / 2 ;
		ver_offset = ( VERT - ver_nr * (y_scan_lines + y_scan_lines/2) ) / 2 ;

		char_index = 0;

		while  ( char_index < no_font_chars )
		{
			y_dev = y_scan_lines + ver_offset ;
			i=0;
			while ( i < ver_nr )
			{	
				x_dev = hor_offset ;
				j=0;
				while ( char_index < no_font_chars && j < hor_nr )
				{
					character = get_char_encoding(char_index);
					getoutchar = (character != (void *)0);

					if (getoutchar)
					{
						if (fi_make_char(TPS_GLOB2 character))
						{
							x_dev += x_scan_lines + x_scan_lines/2 ;
							j++;
						}
#if PROC_SPEEDO && INCL_REMOTE
						else
                                                        /* done with possible remote character data, clear the FourIn1Flags */
                                                        if (sp_globals.FourIn1Flags & BIT0)
                                                                sp_globals.FourIn1Flags ^= BIT0;
#endif
					}
					char_index++;
				}
				i++;
				y_dev += y_scan_lines + y_scan_lines/2;
			}

			/* set cursor position */
			regs.x.ax = 0x0200;
			regs.x.bx = 0;

#if THE_MODE == MODE_GRAY
			regs.x.dx = (int) 0x3100 ;
#else
			regs.x.dx = (int) 0x1c00 ;
#endif
			int86(0x10, &regs, &regs);

			/* press key to continue */
			printf("\n Press any key to continue");
			ch=getch();
			vga_CLS();
			set_backgd();

		}

		/* reset vga to be text mode */
		regs.x.ax = 0x0002;
		int86(0x10, &regs, &regs);
	}
#endif

	switch(font_type)
	{
	case procType1:
		close_type1();
		break;
	case procTrueType:
#if PROC_TRUETYPE
		tt_release_font(PARAMS1);
#endif
		break;
	case procPCL:
#if PROC_PCL
		pf_unload_font();
#endif
		break;
	}

        /* Close the font file */
	close_font_file();

        /* Free the font table memory */
	if (font_table)
		free(font_table);
                                        
        /* Free the font buffer memory */
	if (font_buffer)
	{
#if INCL_LCD
		if (font_type == procSpeedo)
			if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
				_ffree(char_buffer);
			else
				free(char_buffer);
#endif
		if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
			_ffree(font_buffer);
		else
			free(font_buffer);
	}

	return 0;

}  /* end of main function */


/****************************************************************************
  open_font()
      Open the sample font.
  RETURNS:
      TRUE if open succeeded
      FALSE if open failed
*****************************************************************************/
FUNCTION LOCAL_PROTO btsbool open_font(char *pathname)
{
	font_fp = fopen(pathname, "rb");
	
        if (font_fp == NULL)
		return FALSE;

	return TRUE;
}


/****************************************************************************
  close_font()
      close the sample font.
*****************************************************************************/
FUNCTION LOCAL_PROTO void close_font_file(void)
{
	if (font_fp)
		fclose(font_fp);

	return;
}


/****************************************************************************
  fill_font_buffer()
      Allocate a font buffer and load the whole font into that buffer.
  RETURNS:
      TRUE if buffer loaded successfully
      FALSE if load buffer failed 
*****************************************************************************/
FUNCTION btsbool fill_font_buffer(ufix8 **ptr, ufix32 size, FILE *fp)
{
	ufix32		bytes_read, tmp;
	ufix16		bytes;

#ifdef MSDOS
	if (size > 65535)
	{
		if ( (*ptr = (ufix8 *)halloc(size,1)) == NULL )
		{

			fprintf(stderr,"Couldn't allocate font buffer.\n");
			return(FALSE);
		}
		bytes_read = 0;
		bytes = 65535;
		while (bytes > 0)
		{
			tmp = fread(*ptr + bytes_read, 1, bytes, fp);
			if ((ufix16)tmp != bytes)
			{
				fprintf(stderr,"Error reading font file.\n");
				return(FALSE);
			}
			bytes_read += tmp;
			tmp = size - bytes_read;
			bytes = (tmp > 65535) ? 65535 : (ufix16)tmp;
		}
	}
	else
#endif
	{
		if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
			*ptr = (ufix8 FONTFAR *)_fmalloc((size_t)size);
		else
			*ptr = (ufix8 *)malloc((size_t)size);
		if (*ptr == NULL)
		{
			fprintf(stderr,"Couldn't allocate font buffer.\n");
			return(FALSE);
		}
		bytes_read = fread(*ptr, 1, (ufix16)size, fp);
		if (bytes_read != size)
		{
			fprintf(stderr,"Error reading font file.\n");
			return(FALSE);
		}
	}
	return(TRUE);
} /* end of fill_font_buffer */

/****************************************************************************
  setup_type1_font()
      This function handles all the initializations for the Type1 font:
      .  setup the font protocol type and font type by calling fi_reset()
      .  load the font buffer by calling tr_load_font()
      .  setup the values for transformation metrics and call fi_set_specs()
  RETURNS:
      TRUE if successful
      FALSE if failed
*****************************************************************************/
FUNCTION LOCAL_PROTO btsbool setup_type1_font(TPS_GPTR1)
{

#if PROC_TYPE1
                         
#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	font_type = procType1;
	font_protocol = protoPSName;

	fi_reset(TPS_GLOB2 font_protocol, font_type);  /* initialize processor */

	/* load font */
	if ( !tr_load_font(PARAMS2 &font_ptr) )
	{
		fprintf(stderr, "cannot load type 1 font\n");
		return(FALSE);
	}

  
        /* Calculate the requested character size */
        if (do_proof)
        {
                x_scan_lines = DPI / MS_PT_PER_INCH * x_pointsize;
                y_scan_lines = DPI / MS_PT_PER_INCH * y_pointsize;
        }

	/* Initialization - set specs */  
	tspecs.Matrix[0] = (real) x_scan_lines;
	tspecs.Matrix[1] = (real) 0.0;
	tspecs.Matrix[2] = (real) 0.0;
	tspecs.Matrix[3] = (real) y_scan_lines;
	tspecs.Matrix[4] = (real) 0.0;
	tspecs.Matrix[5] = (real) 0.0;

	if ((tspecs.Gen_specs = (specs_t STACKFAR *)malloc((size_t)sizeof(specs_t))) == NULL)
	{
		fprintf(stderr,"Can not allocate structure.\n");
		return(FALSE);
	}

	tspecs.Gen_specs->flags =  THE_MODE;
        tspecs.Gen_specs->boldValue = 0L;

        tspecs.Gen_specs->out_info = 0;

	tspecs.Font.org = (unsigned char *)font_ptr;

	if (!fi_set_specs(TPS_GLOB2 &tspecs))
	{
		fprintf(stderr,"Unable to set requested specs.\n");
		return (FALSE);
	}

	font_encoding = tr_get_encode(PARAMS1);

	no_font_chars = sp_globals.processor.type1.current_font->no_charstrings;

            /* get font name into gFontName */
	strcpy(gFontName, sp_globals.processor.type1.current_font->font_name);
#else
	fprintf(stderr, "Not configured for Type1 fonts\n");
	return(FALSE);
#endif

	return(TRUE);
}


/****************************************************************************
  setup_speedo_font()
      This function handles all the initializations for the Speedo font:
      .  setup the font protocol type and font type by calling fi_reset()
      .  load the font buffer by calling fill_font_buffer()
      .  setup the values for transformation metrics and call fi_set_specs()
  RETURNS:
      TRUE if successful
      FALSE if failed
*****************************************************************************/
FUNCTION LOCAL_PROTO btsbool setup_speedo_font(TPS_GPTR1)
{

#if PROC_SPEEDO

ufix16	first_char_index, i;
ufix8   temp[16];             /* temp buffer for first 16 bytes of font */
ufix32  minbufsz;
ufix16  bytes_read;           /* Number of bytes read from font file */
ufix8   FONTFAR *byte_ptr;


	if (!do_proof)
		x_pointsize = y_pointsize = 1;

	font_protocol = protoDirectIndex;
	font_type = procSpeedo;

	fi_reset(TPS_GLOB2 font_protocol, font_type);

	/* get minimum font buffer size - read first 16 bytes to get the minimum
	   size field from the header, then allocate buffer dynamically  */

	bytes_read = fread(temp, sizeof(ufix8), 16, font_fp);

	if (bytes_read != 16)
	{
		fprintf(stderr,"****** Error on reading %s: %x\n", pathname, bytes_read);
		fclose(font_fp);
		exit(1);
	}
#if INCL_LCD
	minbufsz = (ufix32)read_4b(temp+FH_FBFSZ);
#else
	minbufsz = (ufix32)read_4b(temp+FH_FNTSZ);
	if (minbufsz >= 0x10000)
	{
		fprintf(stderr,"****** Cannot process fonts greater than 64K - use dynamic character loading configuration option\n");
		fclose(font_fp);
		exit(1);
	}
#endif
	if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
		font_buffer = (ufix8 FONTFAR *)_fmalloc((size_t)minbufsz);
	else
		font_buffer = (ufix8 *)malloc((size_t)minbufsz);

	if (font_buffer == NULL)
	{
		fprintf(stderr,"****** Unable to allocate memory for font buffer\n");
		fclose(font_fp);
		exit(1);
	}

#if DEBUG
	printf("Loading font file %s\n", pathname);
#endif

	fseek(font_fp, (ufix32)0, 0);
	if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
	{
		byte_ptr = font_buffer;
		for (i=0; i< minbufsz; i++)
		{
			int ch;
			ch = getc(font_fp);
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
		bytes_read = fread((ufix8 *)font_buffer, sizeof(ufix8), (ufix16)minbufsz, font_fp);
		if (bytes_read == 0)
		{
			fprintf(stderr,"****** Error on reading %s: %x\n", pathname, bytes_read);
			fclose(font_fp);
			exit(1);
		}
	}

#if INCL_LCD
	/* now allocate minimum character buffer */

	minchrsz = read_2b(font_buffer+FH_CBFSZ);

	if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
		char_buffer = (ufix8 FONTFAR *)_fmalloc((size_t)minchrsz);
	else
		char_buffer = (ufix8*)malloc((size_t)minchrsz);

	if (char_buffer == NULL)
	{
		fprintf(stderr,"****** Unable to allocate memory for character buffer\n");
		fclose(font_fp);
		exit(1);
	}
#endif

	first_char_index = read_2b(font_buffer + FH_FCHRF);
	no_font_chars = read_2b(font_buffer + FH_NCHRL);

	/* allocate font_table */
	if ( (font_table = (ufix16 *)malloc((size_t)(no_font_chars*sizeof(ufix16)))) == NULL)
	{
		fprintf(stderr,"Can't allocate font_table.\n");
		return(FALSE);
	}

	/* DO ENCRYPTION HERE */

	gbuff.org = font_buffer;
	gbuff.no_bytes = bytes_read;

        /* Calculate the requested character size */
        if (do_proof)
        {
                x_scan_lines = DPI / MS_PT_PER_INCH * x_pointsize;
                y_scan_lines = DPI / MS_PT_PER_INCH * y_pointsize;
        }

	gspecs.xxmult = (long)x_scan_lines << 16;
	gspecs.yymult = (long)y_scan_lines << 16;
	gspecs.xymult = 0L;
	gspecs.yxmult = 0L;

	gspecs.xoffset = 0L;
	gspecs.yoffset = 0L;

	gspecs.flags = THE_MODE;
        gspecs.boldValue = 0L;

	gspecs.out_info = 0;           /* output module information  */

	gspecs.pfont = &gbuff;
	tspecs.Gen_specs = &gspecs;

	if (!fi_set_specs(TPS_GLOB2 &tspecs))
	{
		fprintf(stderr, "cannot set requested specs\n");    
		return(FALSE);
	}

	/* loading font indecis into font_table. */
	for (i=0; i < no_font_chars; i++)
		font_table[i] = i + first_char_index;

        /* get font name into gFontName */
	strncpy(gFontName, font_buffer+FH_FNTNM, 70);
	return(TRUE);
#else
	fprintf(stderr,"Not configured for Speedo fonts!\n");
	return(FALSE);
#endif
}

#if PROC_TRUEDOC

/* cspListCallBack() used to fill in font_table for direct indexing all TrueDoc font chars */
static int cspListIndex;
FUNCTION int cspListCallBack(TPS_GPTR2 unsigned short code USERPARAM)
{
	font_table[cspListIndex] = (ufix16)code;
	cspListIndex++;
	return(0);
}

/*****************************************************************************
 * setup_csp_font()
 *
 *
 *****************************************************************************/
FUNCTION LOCAL_PROTO btsbool setup_csp_font(TPS_GPTR1)
{
unsigned short  fontRefNumber;
cspFontInfo_t   fontInfo;
int             errCode;
int             no_fonts;

fontAttributes_t fontAttributes;

#if REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

	font_protocol = protoBCID;
	font_type = procTrueDoc;

	fi_reset(TPS_GLOB2 (ufix16)font_protocol, (ufix16)font_type);

	no_fonts = (ufix16)CspGetFontCount(TPS_GLOB1);

        /* Validate the requested font number */
	if (font_idx >= no_fonts)
		font_idx = 0;

	if ( !(errCode = CspSetFont(TPS_GLOB2 font_idx)) )
	{

		/* after CspSetFont() we make this call to get the font's resolution */
		errCode = CspGetFontSpecs(TPS_GLOB2 &fontRefNumber, &fontInfo, &fontAttributes);

                /* Use the font's metric resolution to determine the correct PT_PER_INCH */
                if (do_proof)
                        if (fontInfo.metricsResolution == 8782)
                        {
                                x_scan_lines = DPI / PCL_PT_PER_INCH * x_pointsize;
                                y_scan_lines = DPI / PCL_PT_PER_INCH * y_pointsize;
                        }
                        else
                        {
                                x_scan_lines = DPI / MS_PT_PER_INCH * x_pointsize;
                                y_scan_lines = DPI / MS_PT_PER_INCH * y_pointsize;
                        }

		gspecs.xxmult = (long)x_scan_lines << 16;
		gspecs.yymult = (long)y_scan_lines << 16;
		gspecs.xymult = 0L;
		gspecs.yxmult = 0L;

		gspecs.xoffset = 0L;
		gspecs.yoffset = 0L;

		gspecs.flags = THE_MODE;
                gspecs.boldValue = 0L;

		gspecs.out_info = 0;           /* output module information  */

		tspecs.Gen_specs = &gspecs;

		/* Set character generation specifications */
		if (!fi_set_specs(TPS_GLOB2 &tspecs))
                {
			fprintf(stderr, "Cannot set requested specs\n");    
			return FALSE;
                }
	
		/* after specs set, can make this inquiry: */
		errCode = CspGetFontSpecs(TPS_GLOB2 &fontRefNumber, &fontInfo, &fontAttributes);

		if (!errCode)
			no_font_chars = fontInfo.nCharacters;
		else
			sp_report_error(PARAMS2 errCode+1000);

                /* Copy font name to gFontName */
                if (fontInfo.pFontID)
                        strcpy(gFontName, fontInfo.pFontID);

	}

	/* allocate font_table */
	if ( (font_table = (ufix16 *)malloc((size_t)(no_font_chars*sizeof(ufix16)))) == NULL)
	{
		fprintf(stderr,"Can't allocate font_table.\n");
		return(FALSE);
	}

        /* Initialize the CSP charcter list index */
	cspListIndex = 0;

	/* Store font index values into the table */
	errCode = CspListChars(TPS_GLOB2 cspListCallBack);

	if (errCode)
		sp_report_error(PARAMS2 errCode+1000);

	return	TRUE;
}
#endif /* PROC_TRUEDOC */


/****************************************************************************
  setup_tt_font()
      This function handles all the initializations for the TrueTyp font:
      .  setup the font protocol type and font type by calling fi_reset()
      .  load the font buffer by calling tt_load_font_params()
      .  setup the values for transformation metrics and call fi_set_specs()
  RETURNS:
      TRUE if successful
      FALSE if failed
*****************************************************************************/
FUNCTION LOCAL_PROTO btsbool setup_tt_font(TPS_GPTR1)
{
#if PROC_TRUETYPE

sfnt_OffsetTable	offset_table;
uint32 offset, length ;
ufix16 i;
int error;
fsg_SplineKey *key;

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif
	
	font_protocol = protoDirectIndex;
	font_type = procTrueType;

	fi_reset(TPS_GLOB2 font_protocol, font_type);

	fseek(font_fp, 0L, SEEK_END);
	fontbuf_size = ftell(font_fp);
	fseek(font_fp, 0L, SEEK_SET);

        /* Load font without a character map */
	if (!tt_load_font_params(PARAMS2 (ufix32)font_fp, 0xffff, 0xffff))
	{
		fprintf(stderr, "cannot load truetype font\n");
		return(FALSE);
	}

        /* Calculate the requested character size */
        if (do_proof)
        {
                x_scan_lines = DPI / MS_PT_PER_INCH * x_pointsize;
                y_scan_lines = DPI / MS_PT_PER_INCH * y_pointsize;
        }

	gspecs.xxmult = (long)x_scan_lines << 16;
	gspecs.yymult = (long)y_scan_lines << 16;
	gspecs.xymult = 0L;
	gspecs.yxmult = 0L;

	gspecs.xoffset = 0L;
	gspecs.yoffset = 0L;

	gspecs.flags = THE_MODE;
        gspecs.boldValue = 0L;

        gspecs.out_info = 0;    /* Output module information */

	tspecs.Gen_specs = &gspecs;

	if (!fi_set_specs(TPS_GLOB2 &tspecs))
	{
		fprintf(stderr,"Cannot set requested specs.\n");
		return(FALSE);
	}

	key = fs_SetUpKey(sp_globals.processor.truetype.iPtr, 0, &error);
	if (!key)
		no_font_chars = 640;
	else
		no_font_chars = SWAPW(key->maxProfile.numGlyphs);

	if ( (font_table = (ufix16 *)malloc((size_t)(no_font_chars * sizeof (ufix16))) ) == NULL)
	{
		fprintf(stderr,"Cannot allocate font encoding table.\n");
		return(FALSE);
	}

        /* Store glyph index values into the table */
	for (i=0; i < no_font_chars; i++)
		font_table[i] = i;

        /* get font name into gFontName */
	Read_OffsetTable(font_fp,&offset_table);
	Read_DirectoryTable(font_fp,&offset_table);

	Get_TableOffsetLength(font_fp,&offset_table,"name",&offset,&length);
	Read_NameTable(font_fp,offset,length, gFontName);

	return(TRUE);
#else
	fprintf(stderr, "Not configured for TrueType fonts\n");
	return(FALSE);
#endif
}

#if PROC_PCL
typedef struct
	{
	ufix8	*font;
	ufix16	last_index;
	fix31	*char_offset;
	} dir_t;
#endif
/****************************************************************************
  setup_eo_font()
      This function handles all the initializations for the PCLeo font:
      .  setup the font protocol type and font type by calling fi_reset()
      .  load the font buffer by calling pf_load_pcl_font()
      .  setup the values for transformation metrics and call fi_set_specs()
  RETURNS:
      TRUE if successful
      FALSE if failed
*****************************************************************************/
FUNCTION LOCAL_PROTO btsbool setup_eo_font(TPS_GPTR1)
{

#if PROC_PCL

ufix16 i;
dir_t	*dirTPtr;
extern PC5HEAD  pcl5head;

#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

        font_type = procPCL;
	font_protocol = protoDirectIndex;

	fi_reset(TPS_GLOB2 font_protocol, font_type);

	if ( !pf_load_pcl_font(pathname, &eo_font, font_fp) )
	{
		fprintf(stderr,"Can not load PCL font\n");
		return(FALSE);
	}

        /* Calculate the requested character size */
        if (do_proof)
        {
                x_scan_lines = DPI / PCL_PT_PER_INCH * x_pointsize;
                y_scan_lines = DPI / PCL_PT_PER_INCH * y_pointsize;
        }

	gspecs.xxmult = (long)x_scan_lines << 16;
	gspecs.yymult = (long)y_scan_lines << 16;
	gspecs.xymult = 0L << 16;                       /* Coeff of Y to calculate X pixels */
	gspecs.yxmult = 0L << 16;                       /* Coeff of X to calculate Y pixels */
	gspecs.xoffset = 0L << 16;                      /* Position of X origin */
	gspecs.yoffset = 0L << 16;                      /* Position of Y origin */

	gspecs.flags = THE_MODE;
        gspecs.boldValue = 0L;

        gspecs.out_info = 0;    /* Output module information */

	gspecs.pfont = (buff_t STACKFAR *)&eo_font;

	tspecs.Gen_specs = &gspecs;	/* setup intermediary structure */

	/* Set character generation specifications */
	if (!fi_set_specs(TPS_GLOB2 &tspecs))
	{
		fprintf(stderr, "Cannot set requested specs\n");
		return (FALSE);
	}

	dirTPtr = sp_globals.processor.pcl.eo_specs.pfont->eofont;
	no_font_chars = dirTPtr->last_index + 1;

	if ( (font_table = (ufix16 *)malloc((size_t)(no_font_chars * sizeof (ufix16))) ) == NULL)
	{
		fprintf(stderr,"Cannot allocate font encoding table.\n");
		return(FALSE);
	}

	for (i=0; i < no_font_chars; i++)
		font_table[i] = i;

        /* get font name into gFontName */
	strncpy(gFontName, pcl5head.fontname, 16);
	return(TRUE);
#else
	fprintf(stderr, "Not configured for PCLeo fonts\n");
	return(FALSE);
#endif
}

/****************************************************************************
  close_type1()
      close type1 font
*****************************************************************************/
FUNCTION LOCAL_PROTO void close_type1(void)
{
#if PROC_TYPE1
	tr_unload_font(font_ptr);
#endif
	return;
}

/****************************************************************************
   get_char_encoding()  
      get the pointer into the font table based on character index
   RETURNS:
      A pointer to a void.  It can either be a pointer to a character
      name for type1 or a pointer to a character index for other fonts
*****************************************************************************/
FUNCTION LOCAL_PROTO void *get_char_encoding(ufix16 char_index)

/* character is a pter to a void, because it will either be a pter to */
/* a short or a char depending on font type. If the character was not */
/* retrieved successfully, then function returns false else it returns */
/* true. */
{
	void *ch = (void *)0; /* assume failure */

	switch(font_type)
	{
	case procType1:
		(ufix8 *)ch = (ufix8 *)font_encoding[char_index];
		if (strcmp((char *)ch,".notdef") == 0)
			ch = (void *)0;
		break;
	case procTrueType:
	case procSpeedo:
	case procPCL:
	case procTrueDoc:
		(ufix16 *)ch = (ufix16 *)&font_table[char_index];
		break;
	}

	return(ch);
}


FUNCTION void sp_report_error(SPGLOBPTR2 fix15 n)
/*
 * Called by character generator to report an error.
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
		break;
	}
	return ;
} /* end of sp_report_error */


FUNCTION void sp_open_bitmap(
			SPGLOBPTR2
			fix31 x_set_width,
			fix31 y_set_width,   /* Set width vector */
			fix31 xorg,    /* Pixel boundary at left extent of bitmap character */
			fix31 yorg,    /* Pixel boundary at right extent of bitmap character */
			fix15 xsize,    /* Pixel boundary of bottom extent of bitmap character */
			fix15 ysize     /* Pixel boundary of top extent of bitmap character */
			)
/* 
* Called by character generator to initialize a buffer prior
* to receiving bitmap data.
*/
{
	fix15 i, j;
	union REGS	regs;

#if DEBUG
	printf("open_bitmap(%3.1f, %3.1f, %3.1f, %3.1f, %d, %d)\n",
	(real)x_set_width / 65536.0, (real)y_set_width / 65536.0,
	(real)xorg / 65536.0, (real)yorg / 65536.0, (int)xsize, (int)ysize);
#endif

	if (do_proof == TRUE)
	{
		/* create a HP soft font from the speedo font */
		ufix16 size;
		fix15 xmin, ymin, xmax, ymax;
		ufix32 mult, bitmapWidth;

		raswid = xsize;
		rashgt = ysize;
		offhor = xorg >> 16;
		offver = yorg >> 16;

		xmin = offhor;
		ymin = offver;
		xmax = xmin + xsize - 1;
		ymax = ymin + ysize - 1;

		if (raswid > MAX_BITS)
			raswid = MAX_BITS;

		hpfcdir.format = 4;
		hpfcdir.continuation = 0;
		hpfcdir.size = 14;
		hpfcdir.class = 1;
		hpfcdir.orientation = 0;
		hpfcdir.reserved = 0;
		hpfcdir.xoff = offhor;
		hpfcdir.yoff = offver + rashgt;

		bitmapWidth = gBitmapWidth;
		hpfcdir.set_width = bitmapWidth * 4;

		hpfcdir.width = raswid;
		hpfcdir.height = rashgt;


#if INTEL
		swap_cdir(&hpfcdir);
#endif
		if (xmin < _gxmin) _gxmin = xmin;
		if (ymin < _gymin) _gymin = ymin;
		if (xmax > _gxmax) _gxmax = xmax;
		if (ymax > _gymax) _gymax = ymax;

		size = (((raswid + 7)/8) * rashgt);
		if (size == 0)
		{
			size = 1;
			hpfcdir.width = 8;
			hpfcdir.height = 1;
		}

		/* write out character header informaton... */
		fprintf(hpf_fp, ESC_CHARID, hp_char_id);                /* character ID... */
		fprintf(hpf_fp, ESC_CHARDATA, size+sizeof(hpfcdir));	/* length of data... */
		fwrite(&hpfcdir, 1, sizeof(hpfcdir), hpf_fp);           /* ... and actual binary data */

		if (raswid <= 0)
		{
			y_cur = 0;
			fwrite(&y_cur,1,1,hpf_fp);
		}
		y_cur = 0;

	} /* end do_proof */
	else
	{
		raswid = xsize;
		rashgt = ysize;
		offhor = (fix15)(xorg >> 16);
		offver = (fix15)(yorg >> 16);

		if (raswid > VIDEO_MAX)
			raswid = VIDEO_MAX;

#if DEBUG
		printf("\nCharacter index = %x, ID = %d\n", (unsigned char) char_index, char_id);
		printf("set width = %3.1f, %3.1f\n", (real)x_set_width / 65536.0, (real)y_set_width / 65536.0);
		printf("X offset  = %d\n", offhor);
		printf("Y offset  = %d\n\n", offver);
#endif

		y_cur = 0;
	}
} /* end of sp_open_bitmap */


FUNCTION void sp_set_bitmap_bits (
			SPGLOBPTR2
			fix15     y,     /* Scan line (0 = first row above baseline) */
			fix15     xbit1, /* Pixel boundary where run starts */
			fix15     xbit2  /* Pixel boundary where run ends */
			)
/* 
* Called by character generator to write one row of pixels 
* into the generated bitmap character.                               
*/
{
	short i;
	fix15 length;
	union REGS	regs;

#if DEBUG
	printf("set_bitmap_bits(%d, %d, %d)\n", (int)y, (int)xbit1, (int)xbit2);
#endif


	if (do_proof == TRUE)
	{
		fix15 first_word, last_word;
		fix15 bytwid;

		/* Add character date to the current HP soft font */

		/* Clip runs beyond end of buffer */
		if (xbit1 > MAX_BITS)
			xbit1 = MAX_BITS;
		if (xbit2 > MAX_BITS)
			xbit2 = MAX_BITS;

		/* Output backlog lines if any */
		while (y_cur != y)
                {
			/* put out last bit row */
			bytwid = (raswid + 7) / 8;

			fwrite(bitrow,1,bytwid,hpf_fp);

			for (i = 0; i < (bytwid+1)/2; i++)
				bitrow[i] = 0;

			y_cur++;
		}

		/* Add bits to current line */
		first_word = xbit1>>4;
		last_word = xbit2>>4;

		if (first_word == last_word)
                {
			bitrow[first_word] |= (startmask[xbit1&0xF] & endmask[xbit2&0xF]);
			return;
		}

		bitrow[first_word++] |= startmask[xbit1&0xF];

		while (first_word < last_word)
			bitrow[first_word++] = 0xFFFF;

		if ((xbit2&0xF) != 0)
			bitrow[last_word] |= endmask[xbit2&0xF];

	} /* end do_proof */
#ifdef MSDOS
	else  /* output to screen */
	{
		/* Clip runs beyond end of buffer */
		if (xbit1 > VIDEO_MAX)
			xbit1 = VIDEO_MAX;
		if (xbit2 > VIDEO_MAX)
			xbit2 = VIDEO_MAX;

		length = xbit2-xbit1;

		if (x_dev + offhor + xbit1 + length > HORZ)
			length = HORZ - x_dev - xbit1;

		vga_hline(x_dev+offhor+xbit1,y_dev-rashgt-offver+y,length);

	}
#endif

} /* sp_set_bitmap_bits */


FUNCTION void sp_set_bitmap_pixels (
			SPGLOBPTR2
			fix15     y,         /* Scan line (0 = first row above baseline) */
			fix15     xbit1,     /* Pixel boundary where run starts */
			fix15     num_grays, /* Number of gray pixels */
                        void     *gray_buf   /* Array of gray values */
                        )
/* 
* Called by character generator to write one row of gray pixels 
* into the generated bitmap character.                               
*/
{
	short i;
	fix15 length, xbit2;
	union REGS	regs;

#if DEBUG
	printf("set_bitmap_pixels(%d, %d, %d)\n", (int)y, (int)xbit1, (int)num_grays);
#endif
                           
        /* Calculate the xbit2 value */
        xbit2 = xbit1 + num_grays;

	if (do_proof == TRUE)
	{
		fix15 first_word, last_word;
		fix15 bytwid;

		/* Add character date to the current HP soft font */

		/* Clip runs beyond end of buffer */
		if (xbit1 > MAX_BITS)
			xbit1 = MAX_BITS;
		if (xbit2 > MAX_BITS)
			xbit2 = MAX_BITS;

		/* Output backlog lines if any */
		while (y_cur != y)
                {
			/* put out last bit row */
			bytwid = (raswid + 7) / 8;

			fwrite(bitrow,1,bytwid,hpf_fp);

			for (i = 0; i < (bytwid+1)/2; i++)
				bitrow[i] = 0;

			y_cur++;
		}

		/* Add bits to current line */
		first_word = xbit1>>4;
		last_word = xbit2>>4;

		if (first_word == last_word)
                {
			bitrow[first_word] |= (startmask[xbit1&0xF] & endmask[xbit2&0xF]);
			return;
		}

		bitrow[first_word++] |= startmask[xbit1&0xF];

		while (first_word < last_word)
			bitrow[first_word++] = 0xFFFF;

		if ((xbit2&0xF) != 0)
			bitrow[last_word] |= endmask[xbit2&0xF];

	} /* end do_proof */
#ifdef MSDOS
	else  /* output to screen */
	{
		/* Clip runs beyond end of buffer */
		if (xbit1 > VIDEO_MAX)
			xbit1 = VIDEO_MAX;
		if (xbit2 > VIDEO_MAX)
			xbit2 = VIDEO_MAX;

		length = xbit2-xbit1;

		if (x_dev + offhor + xbit1 + length > HORZ)
			length = HORZ - x_dev - xbit1;

		vga_ghline(x_dev+offhor+xbit1,y_dev-rashgt-offver+y,length,gray_buf);
	}
#endif

} /* sp_set_bitmap_pixels */


/* This function is used to exemplify the INCL_MULTIDEV condition   */
/* In this mode (INCL_MULTIDEV) this function is called in place of */
/* sp_set_bitmap_pixels() routine. This routine in turn calls the   */
/* sp_set_bitmap_bits() routine. */

FUNCTION LOCAL_PROTO void md_set_bitmap_pixels (
			SPGLOBPTR2
			fix15     y,         /* Scan line (0 = first row above baseline) */
			fix15     xbit1,     /* Pixel boundary where run starts */
			fix15     num_grays, /* Number of gray pixels */
                        void     *gray_buf   /* Array of gray values */
                        )
{
sp_set_bitmap_bits(PARAMS2  y, xbit1, (xbit1 + num_grays));
}


FUNCTION void sp_close_bitmap(SPGLOBPTR1)
/* 
 * Called by character generator to indicate all bitmap data
 * has been generated.
 */
{
#if DEBUG
	printf("close_bitmap()\n");
#endif

	if (do_proof == TRUE)
	{
		unsigned int i;
		int bytwid;

		/* close the newly created HP soft font file */

		bytwid = (raswid + 7) / 8;

		fwrite(bitrow,1,bytwid,hpf_fp);

		for (i = 0;i < (bytwid+1)/2;i++)
			bitrow[i] = 0;

		while (y_cur != rashgt)
                {
			/* put out last bit row */
			bytwid = (raswid + 7) / 8;
			fwrite(bitrow,1,bytwid,hpf_fp);
			y_cur++;
		}

		return;

	} /* end do_proof */
}


#if INCL_OUTLINE
FUNCTION void sp_open_outline(
			SPGLOBPTR2
			fix31 x_set_width,
			fix31 y_set_width,  /* Transformed escapement vector */
			fix31  xmin,                           /* Minimum X value in outline */
			fix31  xmax,                           /* Maximum X value in outline */
			fix31  ymin,                           /* Minimum Y value in outline */
			fix31  ymax                            /* Maximum Y value in outline */
			)
/*
 * Called by character generator to initialize prior to
 * outputting scaled outline data.
 */
{
	printf("\nopen_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
	(real)x_set_width / 65536.0, (real)y_set_width / 65536.0,
	(real)xmin / 65536.0, (real)xmax / 65536.0, (real)ymin / 65536.0, (real)ymax / 65536.0);
}

FUNCTION void sp_start_new_char(SPGLOBPTR1)
/*
 * Called by character generator to initialize prior to
 * outputting scaled outline data for a sub-character in a compound
 * character.
 */
{
	printf("start_new_char()\n");
}

FUNCTION void sp_start_contour(
			SPGLOBPTR2
			fix31    x,       /* X coordinate of start point in 1/65536 pixels */
			fix31    y,       /* Y coordinate of start point in 1/65536 pixels */
			btsbool outside   /* TRUE if curve encloses ink (Counter-clockwise) */
			)
/*
 * Called by character generator at the start of each contour
 * in the outline data of the character.
 */
{
	printf("start_contour(%3.1f, %3.1f, %s)\n", 
	(real)x / 65536.0, (real)y / 65536.0, 
	outside? "outside": "inside");
}

FUNCTION void sp_curve_to(
			SPGLOBPTR2
			fix31 x1,       /* X coordinate of first control point in 1/65536 pixels */
			fix31 y1,       /* Y coordinate of first control  point in 1/65536 pixels */
			fix31 x2,       /* X coordinate of second control point in 1/65536 pixels */
			fix31 y2,       /* Y coordinate of second control point in 1/65536 pixels */
			fix31 x3,       /* X coordinate of curve end point in 1/65536 pixels */
			fix31 y3        /* Y coordinate of curve end point in 1/65536 pixels */
)
/*
 * Called by character generator onece for each curve in the
 * scaled outline data of the character. This function is only called if curve
 * output is enabled in the set_specs() call.
 */
{
	printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n", 
	(real)x1 / 65536.0, (real)y1 / 65536.0,
	(real)x2 / 65536.0, (real)y2 / 65536.0,
	(real)x3 / 65536.0, (real)y3 / 65536.0);
}

FUNCTION void sp_line_to(
			SPGLOBPTR2
			fix31 x,       /* X coordinate of vector end point in 1/65536 pixels */
			fix31 y        /* Y coordinate of vector end point in 1/65536 pixels */
			)
/*
 * Called by character generator onece for each vector in the
 * scaled outline data for the character. This include curve data that has
 * been sub-divided into vectors if curve output has not been enabled
 * in the set_specs() call.
 */
{
	printf("line_to(%3.1f, %3.1f)\n", 
	(real)x / 65536.0, (real)y / 65536.0);
}


FUNCTION void sp_close_contour(SPGLOBPTR1)
/*
 * Called by character generator at the end of each contour
 * in the outline data of the character.
 */
{
	printf("close_contour()\n");
}


FUNCTION void sp_close_outline(SPGLOBPTR1)
/*
 * Called by character generator at the end of output of the
 * scaled outline of the character.
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
	fix15 temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer);
	return temp;
}

FUNCTION fix31 read_4b(ufix8 *pointer)
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


/* end of "sp_" routines */

#if PROC_TYPE1
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
#endif


#if PROC_TRUETYPE
FUNCTION void *tt_get_font_fragment (int32 clientID, int32 offset, int32 length)

/* Returns a pointer to data extracted from the sfnt.
   tt_get_font_fragment must be initialized in the fs_GlyphInputType
   record before fs_NewSfnt is called.  */
{
	FILE  *fdes;
	ufix8 *ptr;

	fdes = (FILE *)clientID;
	ptr = (ufix8 *)malloc((size_t)length);

	if (ptr == NULL)
		return ((void *)NULL);

	if (fseek(fdes, offset, 0) != 0)
		return ((void *)NULL);

	if (fread(ptr,1,(size_t)length,fdes) != length)
		return ((void *)NULL);

	return((void *)ptr);
}

FUNCTION void tt_release_font_fragment (void *ptr)

/* Returns the memory at pointer to the heap
  */
{
	free(ptr);
}


#if defined(PCLETTO) || INCL_PCLETTO

static ufix16 theIdx;
FUNCTION char *tt_get_char_addr (ufix16 char_index, int32 *length)
{
	/* in this Application, theIdx has always been set in tt_UnicodeToIndex() */
	    ufix16 i;
	if (gGdir[theIdx].CharIndex == char_index)
	{/* odds are pretty good it will be! */
		*length = gGdir[theIdx].DataLen;
		return(gGdir[theIdx].DataPtr);
	}
	else
		for (i = 0; i < no_font_chars; i++)
		{
			if (gGdir[i].CharIndex == char_index)
			{
				*length = gGdir[i].DataLen;
				return(gGdir[i].DataPtr);
			}
		}
	*length = 0;
	return 0;
}

/*****************************************************************************
 * UNICompare()
 *   Comparison function for BSearch when called by tt_UnicodeToIndex()
 *	RETURNS:	result of NumComp() (-1, 0 or 1 like strcmp())
 *
 ****************************************************************************/
FUNCTION fix15    UNICompare (
			SPGLOBPTR2
			fix31 idx,
			void STACKFAR *keyValue
			)
{
	fix15 NumComp(ufix16 STACKFAR *, ufix16 STACKFAR *);
	return(NumComp( (ufix16 STACKFAR *)keyValue, (ufix16 STACKFAR*)&gGdir[idx].Unicode));
}

FUNCTION ufix16 tt_UnicodeToIndex(ufix16 char_code)
{/* gGdir[] is sorted in Unicode order */
	ufix16 outIndex;
	btsbool success = FALSE;
	outIndex = 0;
#if REENTRANT_ALLOC
	success = BSearch (sp_global_ptr,  (fix15 STACKFAR*)&outIndex, UNICompare,
	(void STACKFAR *)&char_code, (fix31)MAX_CHARS);
#else
	success = BSearch ((fix15 STACKFAR*)&outIndex, UNICompare,
	(void STACKFAR *)&char_code, (fix31)MAX_CHARS);
#endif
	theIdx = outIndex;
	if (success)
		return(gGdir[outIndex].CharIndex);
	return(success);
}
#endif /* defined(PCLETTO) || INCL_PCLETTO */
#endif /* PROC_TRUETYPE */

#if INCL_LCD
FUNCTION buff_t STACKFAR * WDECL sp_load_char_data(
			SPGLOBPTR2
			fix31    file_offset,  /* Offset in bytes from the start of the font file */
			fix15    no_bytes,     /* Number of bytes to be loaded */
			fix15    cb_offset     /* Offset in bytes from start of char buffer */
			)
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
	int     bytes_read,i;
	ufix8  FONTFAR *byte_ptr;

#if DEBUG
	printf("\nCharacter data(%d, %d, %d) requested\n", file_offset, no_bytes, cb_offset);
#endif
	if (fseek(font_fp, (long)file_offset, (int)0) != 0)
	{
		printf("****** Error in seeking character\n");
		fclose(font_fp);     
		exit(1);
	}

	if ((no_bytes + cb_offset) > minchrsz)
	{
		printf("****** Character buffer overflow\n");
		fclose(font_fp);     
		exit(3);
	}

	if (sizeof(ufix8 FONTFAR *) > sizeof(ufix8 *))
	{
		byte_ptr = char_buffer+cb_offset;
		bytes_read = 0;
		for (i=0; i<no_bytes; i++)
		{
			*byte_ptr = (ufix8)getc(font_fp);
			byte_ptr++;
			bytes_read++;
		}
	}
	else
		bytes_read = fread((char_buffer + cb_offset), sizeof(ufix8), no_bytes, font_fp);

	if (bytes_read != no_bytes)
	{
		printf("****** Error on reading character data\n");
		fclose(font_fp);     
		exit(2);
	}

#if DEBUG
	printf("Character data loaded\n");
#endif

	char_data.org = (ufix8 FONTFAR *)char_buffer + cb_offset;
	char_data.no_bytes = no_bytes;
	return &char_data;
}
#else
FUNCTION buff_t STACKFAR * WDECL sp_load_char_data(
			SPGLOBPTR2
			fix31    file_offset,  /* Offset in bytes from the start of the font file */
			fix15    no_bytes,     /* Number of bytes to be loaded */
			fix15    cb_offset     /* Offset in bytes from start of char buffer */
			)
{
	/* DUMMY FUNCTION */
	buff_t *it;
	return(it);
}
#endif

FUNCTION fix31 SetWidthToPixelWidth(fix31 set_width)
{
	fix31 pixel_width = 0; /* assume no width */
	ufix32 mult;

	if (!set_width)
		return(pixel_width); /* already 0 */
	if (font_type == procType1)
		mult = ABS(65536 * tspecs.Matrix[0]) + ABS(65536 * tspecs.Matrix[4]);
	else   
	    mult = ABS(tspecs.Gen_specs->xxmult) + ABS(tspecs.Gen_specs->yxmult);
	/* now calculate the width from sp_get_char_width() call */
	set_width *= sp_globals.metric_resolution;
	set_width -= (sp_globals.metric_resolution >> 1);
	set_width = (set_width >> 16);
	/* multiply RFB setwidth times high-order word of mult, add setwidth times low order word,
	   add roundoff and divide by setwidth resolution units to get pixel width */
	pixel_width = (fix15)(fix31)(((fix31)set_width * (mult >> 16) + 
	    (((fix31)set_width * (mult & 0xffffL) ) >> 16) +
	    (sp_globals.metric_resolution >> 1)) / sp_globals.metric_resolution);

	return pixel_width;
}


/****************************************************************************
  hp_proof_prologue_no_open()
      This function handles all the initializations necessary for the 
      soft font including assign an ID for the soft font and setup
      the values for the header of the soft font by callng new_hpfhd().
*****************************************************************************/
FUNCTION void hp_proof_prologue_no_open(void)
{

	/* make this soft font 100 */
	hpwc (ESC_ASSIGN_FONT_100);

	/* compose soft font header struct */
	new_hpfhd(&hpfhd);

	/* write out "here comes new soft font header" in PCL'ease */
	if (DPI != 300)
        {
		hpwc (ESC_FONTHEAD600);
		fwrite((char *)&hpfhd, 1, sizeof(hpfhd), hpf_fp);
        }
	else
        {
		hpwc (ESC_FONTHEAD);
		fwrite((char *)&hpfhd, 1, sizeof(hpfhd) - 2*sizeof(ufix16), hpf_fp);
        }

	return;
}


FUNCTION void hp_proof_prologue(char *hpoutname)
{

	/* open output hpf, write header */
	hpf_fp = fopen(hpoutname,"wb");
	if (hpf_fp == NULL)
        {
		fprintf(stderr,"Error, cannot open proof file %s\n", hpoutname);
		exit(1);
	}

	if (DPI != 300)
		{
		hpwc("\033%%-12345X@PJL\r\n");
		hpwc("@PJL SET RESOLUTION=600\r\n");
		hpwc("@PJL ENTER LANGUAGE=PCL\r\n");
		}

	hpwc("\033E");

	return;
}


/****************************************************************************
  hp_proof_epilog_no_close()
      This function selects the soft font to use and sends the characters
      to the file by calling layout().
*****************************************************************************/
FUNCTION void hp_proof_epilog_no_close(int total_chars)
{

	/* make the downloaded font temporary */
	hpwc ("\033*c100d4F");

	/* select the downloaded font for use */
	hpwc (ESC_SELECT_FONT_100);

	/* create the proof sheet */
	layout (total_chars);

	/* delete all soft fonts */
	hpwc("\033*c0F");
	return;
}


/****************************************************************************
  layout()
      This function actually reads the text file and sends the characters to
      the file and does the layout.
*****************************************************************************/
FUNCTION void layout (int total_chars)
{
	int	i, hor_nr, ver_nr;
	char command[200], tempstr[16];


	hor_nr = (int) ( 7 * (MS_PT_PER_INCH/(x_pointsize+(x_pointsize/2))) );
	ver_nr = (int) ( 9 * (MS_PT_PER_INCH/(y_pointsize*2)) );

                /* Vertical motion index */

	sprintf(command, "\33&l%.4fC", 48/(MS_PT_PER_INCH/(y_pointsize*2)) );
	hpwc(command);

                /* Horizontal motion index */

	sprintf(command, "\33&k%.4fH", 120/(MS_PT_PER_INCH/(x_pointsize+(x_pointsize/2))) );
	hpwc(command);

	hpwc("\033&f1S");    /* pop cursor position */

	for (i = 0; i < total_chars; i++) 
	{
		if (column_total % hor_nr== 0)
		{
			row_total++;
			if ( row_total > ver_nr)
			{
				page_nr++;
				sprintf(tempstr, "Page %d", page_nr);
				PrintTitle( 1050/*xX*/, 3000/*yY*/, 12/*PointSize*/, 0/*Weight*/, tempstr/*String*/);
				hpwc("\033&l0H");      /* print the current page */
				PrintTitle( 150/*xX*/, 150/*yY*/, 12/*PointSize*/, 3/*Weight*/, title_str/*String*/);
				PrintTitle( 1920/*xX*/, 150/*yY*/, 12/*PointSize*/, 0/*Weight*/, BitsString/*String*/);
				hpwc("\033*p150X");		
				hpwc (ESC_SELECT_FONT_100);
				sprintf(command, "\33&l%.4fC", 48/(MS_PT_PER_INCH/(y_pointsize*2)) );
				hpwc(command);
				sprintf(command, "\33&k%.4fH\n", 120/(MS_PT_PER_INCH/(x_pointsize+(x_pointsize/2))) );
				hpwc(command);
				row_total=1;
			}
			else
			{
				hpwc("\033*p150X");
				hpwc("\033&a+1R");	/* move down one line */
			}
		}
		fprintf(hpf_fp, "%s%c",transdata,i);
		column_total++;
	} 
	hpwc("\033&f0S");    /* push cursor position */

	return;

} /* end layout() */


/****************************************************************************
  hpwc()
      This function writes a hp command string to the proof file.
*****************************************************************************/
FUNCTION void hpwc(char *command)
{
	fprintf (hpf_fp, command);

	return;
}


/****************************************************************************
  new_hpfhd()
      This function actually composes soft font header structure.
*****************************************************************************/
FUNCTION void new_hpfhd(HPF_HEAD *p)
{

	fix15 pixel_width;
	ufix32 fixed_ratio = (ufix32)(PROOF_CHAR_HEIGHT/MS_PT_PER_INCH * (DPI * 4));


	pixel_width = gBitmapWidth;

	if (DPI == 300)
		p->size = 64;
	else
	{
		p->size = 68;
		p->reserved1 = 20;
	}

	p->font_type = 2; /* 0 = ascii 1 = roman8 2 = pc8 */
	p->baseline = 0.764 * (PROOF_CHAR_HEIGHT * DPI/PCL_PT_PER_INCH);
	p->cell_width = _gxmax = _gxmin;
	p->cell_height = _gymax = _gymin;
	p->orient = 0;  /* portrait */
	p->spacing = 0; /* fixed */

	p->symbol_set = 21; /* ascii */
	p->pitch = pixel_width*4; /* set as quarter-dots */
	p->height = PROOF_CHAR_HEIGHT * (DPI * 4) / PCL_PT_PER_INCH;
	p->x_height = 0;
	p->width_type = 0;
	p->style = 0;
	p->stroke_weight = 0;
	p->typeface = 0;
	p->serif_style = 0;
	p->text_height = p->height;
	p->text_width = p->pitch;
	strcpy(p->name,"Speedo Test");
	p->xResolution = DPI;
	p->yResolution = DPI;

#if INTEL
	swp16(p->size);
	swp16(p->baseline);
	swp16(p->cell_width);
	swp16(p->cell_height);
	swp16(p->symbol_set);
	swp16(p->pitch);
	swp16(p->height);
	swp16(p->x_height);
	swp16(p->text_height);
	swp16(p->text_width);
        swp16(p->xResolution);
        swp16(p->yResolution);
#endif

	return;
}

/****************************************************************************
  swap_cdir()
      This function swaps bytes in 16 bit field in font header.
*****************************************************************************/
FUNCTION void swap_cdir(HPF_CHARDIR *p)
{
	swp16(p->xoff);
	swp16(p->yoff);
	swp16(p->width);
	swp16(p->height);
	swp16(p->set_width);

	return;
}





#if RESTRICTED_ENVIRON
FUNCTION unsigned char *dynamic_load(ufix32 file_position, fix15 num_bytes)
{
	fix15 i;

	if (fseek(font_fp, (long)file_position, 0))
	{
		printf("Unable to do fseek on font file\n");
		exit(1);
	}

	if (num_bytes > 10240)
	{
		printf("Number of bytes dynamically requested greater than max\n");
		exit (1);
	}

	for (i=0; i< num_bytes; i++)
		big_buffer[i]=getc(font_fp);

	return big_buffer; /* return addr of big buffer */
}
#endif


#ifdef MSDOS

#define INDEXREG 0x3CE
#define VALREG 0x3CF

#define OUTINDEX(index, val)    {outp(INDEXREG, index); outp(VALREG, val);}

#define EGA_WRITE               {outp(0x3ce,1); outp(0x3cf,0x0f);}
#define EGA_RESET               {outp(0x3ce,0); outp(0x3cf,0 ); \
                                outp(0x3ce,1); \
                                outp(0x3cf,0 ); \
                                outp(0x3ce,8); \
                                outp(0x3cf,0xff);}

#define EGABASE 0xA0000000L
#define WIDTH 80L

static char lmask[8] ={ 
0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
static char rmask[8] ={ 
0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
static char pmask[8] ={ 
0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

/****************************************************************************
  vga_hline()
      This function draws a 'length' horizontal from x,y  with black pixels. 
*****************************************************************************/
FUNCTION LOCAL_PROTO void vga_hline(int x, int y, int length)
{
int i, bits, r ;
unsigned char mask_id, mask ;
char far *base;


#if THE_MODE == MODE_GRAY

/* calculate VGA video address */
base = (char far *)(EGABASE + (long)y * HORZ + (long)x );

/* Convert byte count to bit count */
length *= 8;
bits = 8;

#else

EGA_WRITE ;

/* calculate VGA video address */
base = (char far *)(EGABASE + (long)y * WIDTH + (long)( x >> 3 ) );

mask = lmask[(r = x % 8)];
bits = (length + r - 8) ;

/* check how many bits in first byte */
if ( bits < 0 )
{
	/* only partial bits */
	mask_id = mask = pmask[r];
	bits = length ;
	for ( i = 1 ; i < bits ; i++ )
	{
		mask_id = mask_id >> 1 ;
		mask = mask | mask_id ;
	}
}
else /* partial bits in first byte and more */
        bits = 8 - r ;
#endif

while ( length > 0 )
{

#if THE_MODE == MODE_GRAY
        *base = 0x3f;           /* write black to display memory */
#else
	OUTINDEX(8,mask);       /* set mask */
        *base |= TRUE;          /* write black to display memory */
#endif

	base++;                 /* next display pixel(s) */

	length -= bits;         /* adjust the remaining length */

	if ( length < 8 )
		mask = rmask[length];
	else
		mask = 0xff;

	bits = 8;
}

EGA_RESET ;
return;
}


/****************************************************************************
  vga_ghline()
      This function draws a 'length' horizontal from x,y with gray pixels. 
*****************************************************************************/
FUNCTION LOCAL_PROTO void vga_ghline(int x, int y, int length, void *gray_buf)
{
int i, j;
char far *base;

/* initialize the gray_buf index */
j = 0;

/* calculate VGA video address */
base = (char far *)(EGABASE + (long)y * HORZ + (long)x );

/* Convert byte count to bit count */
length *= 8;

while ( length > 0 )
{
        /* write gray to display memory */
        *base = ((char*)gray_buf)[j++];
	base++;

	length -= 8;
}

EGA_RESET ;
return;
}

/****************************************************************************
  vga_CLS()
      This function clears the screen by writing the whole block with black
*****************************************************************************/
FUNCTION LOCAL_PROTO void vga_CLS(void)
{
int i, j ;
char far *base ;

EGA_WRITE ;

base = (char far *)EGABASE;

OUTINDEX(0, 0);         /* set new color */
OUTINDEX(8,0xff);       /* set mask */

for ( i = 0 ; i < VERT ; i++ )

#if THE_MODE == MODE_GRAY
        for ( j = 0 ; j < HORZ  ; j++ )
        {

                *base = 0x3f;   /* force a black write */
                base++;
        }
#else
	for ( j = 0 ; j < WIDTH ; j++ )
	{
		*base |= TRUE;  /* force a black write */
		base++ ;
	}
#endif

EGA_RESET ;
return;
}

/****************************************************************************
  set_backgd()
      This function changes all the pixels intense white.
*****************************************************************************/
FUNCTION LOCAL_PROTO void set_backgd(void)
{
int i, j ;
char far *base ;

EGA_WRITE ;

base = (char far *)EGABASE;

#if THE_MODE != MODE_GRAY
OUTINDEX(0, 0x0f);  /* set background color to be white */
OUTINDEX(8,0xff);   /* set mask */
#endif

for ( i = 0 ; i < VERT ; i++ )

#if THE_MODE == MODE_GRAY
        for ( j = 0 ; j < HORZ  ; j++ )
        {
                *base = 0;      /* force a white write */
                base++;
        }
#else
	for ( j = 0 ; j < WIDTH ; j++ )
	{
		*base |= TRUE;  /* force a white write */
		base++ ;
	}
#endif

EGA_RESET ;
return;
}
#endif

FUNCTION void PrintTitle(
				int xX, 
				int yY,
				int PointSize,
				int Weight,
				char *String)
{
char command[128], tempstr[32];
float pitch;
	command[0] = 0;	/* null terminate empty string */
	sprintf(tempstr, "\033*p%dX\033*p%dY", xX, yY);	/* build x,y loc fragment */
	strcat (command, tempstr); /* concatenate onto command string */
	strcat (command, "\033(0U");	/* concat symbol set, always 0U */
	strcat (command, "\033(s0P");	/* concat spacing, always fixed */
	strcat (command, "\033(s0S");	/* concat style, always upright */
	/* calculate pitch from pointsize, set both: */
	pitch = 1.0/(5291.0/8782.0 * PointSize / PCL_PT_PER_INCH);
	sprintf (tempstr, "\033(s%dV\033(s%2.2fH", PointSize, pitch);
	strcat (command, tempstr); /* concatenate onto command string */
	/* build weight substring: */
	sprintf(tempstr, "\033(s%dB", Weight);
	strcat (command, tempstr); /* concatenate onto command string */
	strcat (command, "\033(s4099T");	/* concatenate Typeface Family, always Courier */
	strcat (command, String);			/* append the title text */
	hpwc(command);						/* issue the command */
}

#if PROC_TRUETYPE
/* read first 12 bytes of Offset Table */
FUNCTION LOCAL_PROTO void Read_OffsetTable(FILE *fdes, sfnt_OffsetTable	*offset_table)
{
	ufix8 *ptr;

	ptr=(ufix8 *)tt_get_font_fragment ((ufix32)fdes,0L,12L);
	offset_table->version = (ufix32)(GET_WORD(ptr)) << 16;
	offset_table->version += GET_WORD((ptr+2));
	offset_table->numOffsets = GET_WORD((ptr+4));
	offset_table->searchRange = GET_WORD((ptr+6));
	offset_table->entrySelector = GET_WORD((ptr+8));
	offset_table->rangeShift = GET_WORD((ptr+10));
	tt_release_font_fragment (ptr);

#if DEBUG
	printf("sfnt version %d\n",offset_table->version);					/* 0x10000 (1.0) */
	printf("Number of Tables %d\n",offset_table->numOffsets);				/* number of tables */
	printf("searchRange %d\n",offset_table->searchRange);				/* (max2 <= numOffsets)*16 */
	printf("entrySelector %d\n",offset_table->entrySelector);			/* log2(max2 <= numOffsets) */
	printf("rangeShift %d\n",offset_table->rangeShift);				/* numOffsets*16-searchRange*/
#endif
	return ;
}

/* read directory of Tables */
FUNCTION LOCAL_PROTO int Read_DirectoryTable (FILE *fdes, sfnt_OffsetTable* sfntdir)
{
	int     ii,i, size;
	ufix8  *pf;
	char name_tag[5], rev_tag[5] ;
	ufix8 *ptr ;
         
        size = sfntdir->numOffsets * sizeof(sfnt_DirectoryEntry);

        ptr=(ufix8 *)tt_get_font_fragment ((ufix32)fdes,12L,(long)size);
	if ( ptr == NULL )
	{
                fprintf(stderr, "*** malloc fails\n\n");
                exit(1);
	}

	sfntdir->table = (sfnt_DirectoryEntry *)malloc((size_t)size);
	if (sfntdir->table == NULL)
                return(1);

	for (pf=ptr, ii=0; ii<sfntdir->numOffsets; ii++)
	{
		sfntdir->table[ii].tag = (ufix32)(GET_WORD(pf)) << 16;
		sfntdir->table[ii].tag += GET_WORD((pf+2));

		name_tag[4] = '\0' ;
		memcpy(name_tag,&(sfntdir->table[ii].tag),4);
		for ( i = 0 ; i < 4; i++)
#if INTEL
			rev_tag[3-i]=name_tag[i];
#else
			rev_tag[i]=name_tag[i];
#endif
		rev_tag[4] = '\0' ;
#if DEBUG
		printf(" table tag: %s ",rev_tag);
#endif

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
#if DEBUG
		printf(" table offset: %ld",sfntdir->table[ii].offset);
		printf(" table length: %ld\n",sfntdir->table[ii].length);
#endif
	}
	tt_release_font_fragment (ptr);

	return(0);
}

FUNCTION LOCAL_PROTO int Get_TableOffsetLength(
			FILE *fdes,
			sfnt_OffsetTable* sfntdir,
			char *dir_name,
			uint32 *offset,
			uint32 *length)
{
	int i, j;
	char *name_tag, rev_tag[5] ;
	ufix8 *ptr ;

	*offset = 0L;
	*length = 0L ;

	for (i=0; i<sfntdir->numOffsets; i++)
	{
		rev_tag[4] = '\0' ;
		name_tag=(char*)&(sfntdir->table[i].tag);
		for ( j = 0 ; j < 4; j++)
#if INTEL
			rev_tag[3-j]=name_tag[j];
#else
			rev_tag[j]=name_tag[j];
#endif
/*		printf(" table tag: %s dir name %s\n",rev_tag,dir_name); */
		if ( strcmp(rev_tag,dir_name) == 0 )
		{
			*offset = sfntdir->table[i].offset;
			*length = sfntdir->table[i].length;
		}
	}

	return 0 ;
}

FUNCTION LOCAL_PROTO int Read_NameTable(
				FILE *fdes,
				long offset,
				long length,
				char *nameStr)
{
	ufix8 *ptr, i, *name_ptr, j ;
	int nr_records ;
	sfnt_NameRecord name_record;
	char string[256];


	ptr = (ufix8 *)tt_get_font_fragment ((ufix32)fdes,offset,length);
	nr_records = GET_WORD((ptr+2));

#if DEBUG
	printf(" number of name table %d\n",nr_records);
#endif

	ptr += 6 ;
	name_ptr = ptr + 12 * nr_records ;

	for (i = 0 ; i < nr_records ; i++ )
	{
		name_record.platformID=GET_WORD((ptr));
		name_record.specificID=GET_WORD((ptr+2));
		name_record.languageID=GET_WORD((ptr+4));
		name_record.nameID=GET_WORD((ptr+6));
		name_record.length=GET_WORD((ptr+8));
		name_record.offset=GET_WORD((ptr+10));
#if DEBUG
		printf(" platformID %d specificID %d languageID %x nameID %d length %d\n",
			name_record.platformID,name_record.specificID,
			name_record.languageID,name_record.nameID,
			name_record.length);
#endif
		if ( name_record.platformID == 3 && name_record.languageID == 0x0411 )
			JIS_code = 1 ;
		if ( name_record.platformID == 3 && name_record.specificID == 2 )
			JIS_Highbyte = 1 ;

		memcpy(string,(name_ptr + name_record.offset),name_record.length);

		if( name_record.platformID==3)
		{
			for ( j = 0 ; j < name_record.length ; j++ )
				string[j]=string[j*2+1];
			string[name_record.length>>1]='\0';
		}
		else
			string[name_record.length]='\0';
#if DEBUG
		printf("name string %s\n",string);
#endif
		if (name_record.nameID == 3) /* Unique humanly readable name for the font */
			strcpy(nameStr, string);

		ptr += 12 ;
	}
	tt_release_font_fragment (ptr);

	return 0;
}
#endif /* PROC_TRUETYPE */

FUNCTION usage()
{
		printf("Usage:  fdump <fontfile> [/s:lines per em] [/p:pointsize] [/y:height] [/f:fontIndex]\n"); 
		printf("        default lines per em for screen is 24\n");
		printf("        default point size for printer is 16\n");
                printf("        /y:height units are determined by either /s or /p option\n");
		printf("        use /f:fontIndex for picking font within a PFR file.\n");
		exit(1);
}
/* EOF fdump.c */
