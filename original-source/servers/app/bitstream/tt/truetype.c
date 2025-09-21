/*
 *	truetype.c
 *                                                                           *
 *	sample mainline program for truetype rasterizer
 *
*/

/*	Revision Control Information
 *
 *	$Header: //bliss/user/archive/rcs/true_type/truetype.c,v 14.0 97/03/17 17:55:20 shawn Release $
 *
 *	$Log:	truetype.c,v $
 * Revision 14.0  97/03/17  17:55:20  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.7  96/07/02  13:45:50  shawn
 * Changed boolean to btsbool and NULL to BTSNULL.
 * 
 * Revision 10.6  96/06/05  15:33:22  shawn
 * Added support for the GrayWriter.
 * Added support for INCL_MULTIDEV.
 * 
 *
 * Revision 10.5  95/11/17  09:55:01  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 10.4  95/07/28  14:48:44  shawn
 * Enhanced 'CMAP' examination code to expand to glyph indices
 * -DDEBUG_CMAP=0  font & CMAP headers only
 * -DDEBUG_CMAP=1  font & CMAP subheaders only
 * -DDEBUG_CMAP=2  font & CMAP headers + glyph indices
 * 
 * Revision 10.3  95/05/16  12:37:03  shawn
 * Added support for Format 6/Trimmed-Table Character Mapping
 * Changed Character Map processing to a case statement
 * Cleaned up printf() column alignment and spacing
 * 
 * Revision 10.2  95/04/12  08:38:10  shawn
 * Make CMAP debugging function through a command line argument '-DDEBUG_CMAP=1'
 * 
 * Revision 10.1  95/04/11  11:13:42  shawn
 * Changed 'fdes' to '*fdes' in FUNCTIONs ExamineCMAP(), Read_OffsetTable(), Read_DirectoryTable(), Get_TableOffsetLength()
 * Changed 'offset' and 'length' to '*offset' and '*length' in Get_TableOffsetLength()
 * 
 * Revision 10.0  95/02/15  16:26:10  roberte
 * Release
 * 
 * Revision 9.4  95/02/03  10:56:42  roberte
 * Added non-MSDOS #include <sys/types.h> to get size_t.
 * 
 * Revision 9.3  95/01/24  15:43:16  shawn
 * Changed memset, ffree and fmalloc to ANSI
 * 
 * Revision 9.2  95/01/11  13:11:37  shawn
 * Changed type of tt_get_font_fragment() to (void *)
 * 
 * Revision 9.1  95/01/04  16:39:39  roberte
 * Release
 * 
 * Revision 8.2  95/01/03  13:50:04  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.1  94/11/11  11:15:52  roberte
 * Changed include files so this module could declare a fsg_SplineKey pointer
 * to use for determing total number of characters in the font.
 * Now calls tt_load_font_params() with 0xffff for platformID and specificID.
 * These ID's now signal NO CMAP. Previous assumption of the ever-present 1,0
 * cmap was proven false. Byte map is not required.
 * Watch out, for Far Eastern fonts, this makes an awful lot of glyphs! Even
 * all the dummy glyphs put in cmap to collapse segments for smaller cmaps.
 * 
 * Revision 8.0  94/05/04  09:34:52  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  12:00:01  roberte
 * Release
 * 
 * Revision 6.93  94/04/01  16:12:30  roberte
 * Changed tt_load_font() call to equivalent tt_load_font_params() call.
 * Added 'cmap' examination code. Probably came from TTQUERY code distributed
 * by Microsoft. Anyway, public domain, and here integrated with ttsample
 * by turning on the INCL_CMAP flag when building this module. Turn on DEBUG_CMAP
 * to see printf's of the results. This code is easily adapted to making
 * some query functions (does this font have a Unicode table, and if so,
 * what PlatformID, SpecificID pair will access it?).
 * 
 * Revision 6.92  94/02/03  12:47:21  roberte
 * Adapted Microsoft macro to include TurboC as well.
 * 
 * Revision 6.91  93/08/30  14:53:57  roberte
 * Release
 * 
 * Revision 6.53  93/08/26  16:09:00  roberte
 * More portable declaration (extern) of malloc().
 * 
 * Revision 6.52  93/06/03  11:08:15  roberte
 * Corrected PARAMS macro used in declaration of sp_open_outline(), now PARAMS2
 * 
 * Revision 6.51  93/05/21  09:45:42  roberte
 * Added INCL_PCLETTO logic so this option can add PCLETTO handling on top of TrueType handling.
 * 
 * Revision 6.50  93/05/18  10:14:37  roberte
 * Restored make_char call to tt_make_char().
 * 
 * Revision 6.49  93/05/18  09:51:00  roberte
 * Corrected for loop for characters.
 * 
 * Revision 6.48  93/05/18  09:46:15  roberte
 * Implement GLOBALFAR for SPEEDO_GLOBALS.
 * 
 * Revision 6.47  93/04/23  17:51:09  roberte
 *  Moved define SET_SPCS from tt_specs.c to truetype.c.
 * #ifdef'd out keypress code.
 *  .
 * 
 * Revision 6.46  93/03/18  10:30:58  roberte
 * Changed back to making characters by Apple Unicode.
 * 
 * Revision 6.45  93/03/17  11:02:12  roberte
 * Returned this sample program to making character index.
 * 
 * Revision 6.44  93/03/15  13:23:34  roberte
 * Release
 * 
 * Revision 6.15  93/03/11  20:04:12  roberte
 * changed #if __MSDOS to #ifdef MSDOS
 * 
 * Revision 6.14  93/03/10  17:11:48  roberte
 * Changed type of main() to int.
 * 
 * Revision 6.13  93/03/09  12:13:58  roberte
 *  Replaced #if INTEL tests with #if __MSDOS as appropriate.
 * Worked on include file list.
 * 
 * Revision 6.12  93/01/26  13:35:17  roberte
 * Added PARAMS1 and PARAMS2 macros to all reentrant function calls and definitions. 
 * Added conditional allocation of sp_global_ptr so DYNAMIC_ALLOC and REENTRANT_ALLOC
 * are testable in this module.
 * 
 * Revision 6.11  93/01/20  09:55:51  davidw
 * 80 column cleanup
 * 
 * Revision 6.10  93/01/20  09:25:18  davidw
 * 80 column and ANSI cleanup.
 * 
 * Revision 6.9  92/12/02  15:48:11  laurar
 * change between outline mode and black mode on command line.
 * 
 * Revision 6.7  92/11/10  10:09:49  davidw
 * made output mode switchable
 * 
 * Revision 6.6  92/11/09  17:02:42  davidw
 * changed specs.flags to reflect mode set by -DINCL_OUTLINE flag in makefile,
 * no longer hardwired to MODE_BLACK
 * 
 * Revision 6.5  92/10/28  09:53:39  davidw
 * removed control Z from EOF
 * 
 * Revision 6.4  92/10/19  17:57:27  laurar
 * change ptsize variable to lines per em.
 * 
 * Revision 6.3  92/10/08  18:31:21  laurar
 * take out SUPRESS define; add "q to quit" message;
 * add include <stdlib.h> for INTEL.
 * 
 * Revision 6.1  91/08/14  16:49:10  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:30:15  mark
 * Release
 * 
 * Revision 4.3  91/08/07  12:05:18  mark
 * remove MODE_APPLE reference
 * 
 * Revision 4.2  91/08/07  11:58:40  mark
 * add rcs control strings
 * 
*/

#ifdef RCSSTATUS
static char rcsid[] = "$Header: //bliss/user/archive/rcs/true_type/truetype.c,v 14.0 97/03/17 17:55:20 shawn Release $";
#endif

#ifdef MSDOS
#include <stdlib.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <setjmp.h>
#define SET_SPCS
#include "spdo_prv.h"
#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "sfnt.h"
#include "sc.h"
#include "fnt.h"
#include "fontscal.h"
#include "fsglue.h"

#define DEBUG	0        

#ifdef  DEBUG_CMAP
#undef  INCL_CMAP               
#define INCL_CMAP	1       /* Include the CMAP examination code */
#else
#undef  INCL_CMAP
#define INCL_CMAP       0       /* Exclude the CMAP examination code */
#endif

#if INCL_CMAP
GLOBAL_PROTO void ExamineCMAP(FILE *fdes);
#endif


/*** EXTERNAL FUNCTIONS ***/

/*** STATIC VARIABLES ***/
#define FONT_BUFFER_SIZE  1000000      /* Size of font buffer */
#define GET_WORD(A)  ((ufix16)((ufix16)A[0] << 8) | ((ufix8)A[1]))
#define MAX_BITS  256	/* Max line length of generated bitmap */

static  fix15  raswid;          /* raster width  */
static  fix15  rashgt;          /* raster height */
static  fix15  offhor;          /* horizontal offset from left edge of emsquare */
static  fix15  offver;          /* vertical offset from baseline */
static  fix15  set_width;	/* character set width */
static  fix15  y_cur;		/* Current y value being generated and printed */
static ufix16  no_font_chars;

static  char   line_of_bits[2 * MAX_BITS + 1]; /* Buf for row of gen'd bits */

#if (defined (M_I86CM) || defined (M_I86LM) || defined (M_I86HM) || defined (M_I86SM) || defined (M_I86MM) || defined(__TURBOC__))
  /* then we are using a Microsoft or Borland compiler */
#else
  
  /* need to have a dummy _fmalloc for unix */

FUNCTION void *_fmalloc(size_t size)
{
}
  
  /* need to have a dummy _fmemset for unix */

FUNCTION void *_fmemset(void *s, int c, size_t n)
{
}
  
  /* need to have a dummy _ffree for unix */

FUNCTION void _ffree(void *ptr)
{
}

#endif


FUNCTION int main(int argc, char *argv[])
{
   FILE     *fdes;
   int32    handle;     /* handle to sfnt resource ==> iPtr.clientID */
   ufix16   i;
   ufix8    user;
   specs_t  specs;
   int      lines, outl_mode;

#if REENTRANT_ALLOC
   SPEEDO_GLOBALS GLOBALFAR* sp_global_ptr;
#endif

extern fsg_SplineKey* fs_SetUpKey PROTO((fs_GlyphInputType *inptr, unsigned stateBits, int *error));
	int error;
	fsg_SplineKey *key;
   
    if (argc < 2)
        {
        fprintf(stderr, "Usage: ttsample <font filename> <lines per em> <outline mode>\n");
        fprintf(stderr, "Default lines per em is 25.\n");
        fprintf(stderr, "outline mode = 1 for Outline Output. Default is the Black Writer.\n");
        exit(1);
        }
        
    fdes = fopen ((char *)argv[1], "rb");
    if (fdes == BTSNULL)
        {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        exit(1);
        }

    if (argc >= 3)
        {
        sscanf(argv[2],"%d",&lines);

        if (lines <= 6)
            {
            fprintf(stderr,"invalid pointsize\n");
            fclose(fdes);
            exit(1);
            }
        }
    else
        lines = 25;

    if (argc >= 4)
        sscanf(argv[3], "%d", &outl_mode);
    else
        outl_mode = 0;

#if INCL_CMAP
	/* take a look around the headers and cmap before processing font */
    ExamineCMAP(fdes);
#endif

#if DYNAMIC_ALLOC || REENTRANT_ALLOC
    if (sizeof(SPEEDO_GLOBALS GLOBALFAR *) > sizeof(char *))
        sp_global_ptr = (SPEEDO_GLOBALS GLOBALFAR *)_fmalloc(sizeof(SPEEDO_GLOBALS));
    else
        sp_global_ptr = (SPEEDO_GLOBALS *)malloc(sizeof(SPEEDO_GLOBALS));

    if (sizeof(SPEEDO_GLOBALS GLOBALFAR *) > sizeof(char *))
        _fmemset(sp_global_ptr,(ufix8)0,sizeof(SPEEDO_GLOBALS));
    else
        memset(sp_global_ptr,(ufix8)0,sizeof(SPEEDO_GLOBALS));
#endif
        
    if (!tt_reset(PARAMS1))
        {
        fprintf(stderr, "*** tt_reset fails\n\n");
        exit(1);
        }
        
    handle = (ufix32)fdes;

        /* platformID=0xffff and specificID=0xffff means no cmap access - direct index only */

    if (!tt_load_font_params (PARAMS2 handle, 0xffff /*platformID*/, 0xffff /*specificID*/))
        {
        fprintf(stderr, "*** tt_load_font fails\n\n");
        exit(1);
        }
        
    specs.xxmult = (long)lines << 16;
    specs.xymult = 0L;
    specs.xoffset = 0L;
    specs.yxmult = 0L;
    specs.yymult = (long)lines << 16;
    specs.yoffset = 0L;

    if (outl_mode)
        specs.flags = MODE_OUTLINE;
    else
        specs.flags = MODE_BLACK;

#if INCL_GRAY
    sp_globals.pixelSize = 6;   /* Must set '1<x>9' prior to set_specs() call */
#endif


    if (!tt_set_specs (PARAMS2 &specs))
        {
        fprintf(stderr, "*** tt_set_specs fails\n\n");
        exit(1);
        }
        
    key = fs_SetUpKey(sp_globals.processor.truetype.iPtr, 0, &error);

    if (!key)
        no_font_chars = 640;
    else
        no_font_chars = SWAPW(key->maxProfile.numGlyphs);

        /* Character generation loop */

    for (i=0; i<no_font_chars; i++)
        {

        fprintf (stderr,"\nmaking character index 0x%x at size %d\n", (int)i, lines);

	if (!tt_make_char_idx (PARAMS2 i))
            fprintf(stderr, "*** tt_make_char_idx() fails\n\n");

#ifdef WANT_KEYPRESS
        fprintf(stderr,"Type Q to quit or any other key to continue: ");
        user = getchar();
        if (user == 'q' || user == 'Q')
            break;
#endif

        }       /* End of character generation loop */

    tt_release_font(PARAMS1);

    exit(0);

}


FUNCTION void *tt_get_font_fragment(int32 clientID, int32 offset, int32 length)

/* Returns a pointer to data extracted from the sfnt.
   tt_get_font_fragment must be initialized in the fs_GlyphInputType
   record before fs_NewSfnt is called.  */
{
FILE  *fdes;
void  *ptr;

fdes = (FILE *)clientID;
ptr = (void *)malloc((size_t)length);

if (ptr == BTSNULL)
    return (ptr);

if (fseek(fdes, offset, 0) != 0)
    return (BTSNULL);

if (fread(ptr,1,(size_t)length,fdes) != length)
    return (BTSNULL);
    
return(ptr);
}



FUNCTION void tt_release_font_fragment(void *ptr)

/* Returns the memory at pointer to the heap
  */
{
free(ptr);
}

#if defined(PCLETTO) || INCL_PCLETTO

typedef struct
    {
    ufix16 Unicode;
    char *DataPtr;
    ufix32 DataLen;
    } GdirType;

static GdirType gGdir[640];
      

FUNCTION char *tt_get_char_addr(uint16 char_index, int32 *length)
{
ufix16 i, *intPtr;

    for (i = 0; i < no_font_chars; i++)
	{
        intPtr = gGdir[i].DataPtr - 2;

        if (*intPtr == char_index)
            {
            *length = gGdir[i].DataLen;
            return(gGdir[i].DataPtr);
            }
        }

    *length = 0;

    return 0;
}

FUNCTION ufix16 tt_UnicodeToIndex(uint16 char_code)
{
ufix16 i, *intPtr;

    for (i = 0; i < no_font_chars; i++)
        {
        if (gGdir[i].Unicode == char_code)
            {
            intPtr = gGdir[i].DataPtr - 2;
            return(*intPtr);
            }
        }

    return 0;
}

#endif /* defined(PCLETTO) || INCL_PCLETTO */



FUNCTION void sp_report_error(SPGLOBPTR2 fix15 n)
/*
 * Called by character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
{
    printf("*** report_error 0x%X %d\n\n", n, n);
}


FUNCTION void sp_open_bitmap(SPGLOBPTR2 fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)

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
offhor = (fix15)((xorg+32768L) >> 16);
offver = (fix15)(yorg >> 16);

if (raswid > MAX_BITS)
    raswid = MAX_BITS;

printf("set width = %3.1f, %3.1f\n", (real)x_set_width / 65536.0, (real)y_set_width / 65536.0);
printf("X offset  = %d\n", offhor);
printf("Y offset  = %d\n\n", offver);

for (i = 0; i < raswid; i++)
    {
    line_of_bits[i << 1] = '.';
    line_of_bits[(i << 1) + 1] = ' ';
    }

line_of_bits[raswid << 1] = '\0';
y_cur = 0;
}


FUNCTION void sp_set_bitmap_bits(SPGLOBPTR2 fix15 y, fix15 xbit1, fix15 xbit2)

/* Called by output module to write one row of pixels into the generated bitmap character. */
{
fix15 i;

#if DEBUG
printf("set_bitmap_bits(%d, %d, %d)\n", (int)y, (int)xbit1, (int)xbit2);
#endif

if (xbit1 < 0)
    {
    printf("ERROR!!! X1 < 0!!!\n");
    exit(0);
    }

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

FUNCTION void sp_open_outline(SPGLOBPTR2 fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)

/* Called by output moduel to initialize prior to outputting scaled outline data. */
{
printf("\nopen_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
    (real)x_set_width / 65536.0, (real)y_set_width / 65536.0,
    (real)xmin / 65536.0, (real)xmax / 65536.0, (real)ymin / 65536.0, (real)ymax / 65536.0);
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
    (real)x / 65536.0, (real)y / 65536.0, 
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
printf("line_to(%3.1f, %3.1f)\n", 
    (real)x / 65536.0, (real)y / 65536.0);
}


FUNCTION void sp_close_contour(SPGLOBPTR1)

/* Called by output module at the end of each contour in the outline data of the character. */
{
printf("close_contour()\n");
}


FUNCTION void sp_close_outline(SPGLOBPTR1)

/* Called by output module at the end of output of the scaled outline of the character. */
{
printf("close_outline()\n");
}

#endif

#if INCL_CMAP
typedef struct
{
	uint16	format;
	uint16	length;
	uint16	version;
	uint16	segCountX2;
	uint16	searchRange;
	uint16	entrySelector;
	uint16	rangeShift;
	uint16	endCount;
}	segMappingTable;

typedef struct
{
	uint16	format;
	uint16	length;
	uint16	version;
	uint16*	subHeaderKey;
}	HighByteMappingTable;

typedef struct
{
	uint16 startCount;
	uint16 endCount;
        int    idDelta;
        uint16 idRangeOffset;
}	rangeCount;

typedef struct
{
	uint16 firstCode;
	uint16 entryCount;
	int16  idDelta;
	uint16 idRangeOffset;
}	subHeaders;

typedef struct
{
        ufix16  format;
        ufix16  length;
        ufix16  version;
        ufix16  firstCode;
        ufix16  entryCount;
        ufix16  glyphIdArray[1];
}       trimmedTableMapping;


static rangeCount *charRange;
static int nr_charRange;

LOCAL_PROTO void Read_OffsetTable(FILE *fdes, sfnt_OffsetTable *offset_table);

FUNCTION void ExamineCMAP(FILE *fdes)
{
	sfnt_OffsetTable	offset_table;
	uint32 offset, length ;

	Read_OffsetTable(fdes,&offset_table);
	Read_DirectoryTable (fdes,&offset_table);
	Get_TableOffsetLength (fdes,&offset_table,"cmap",&offset,&length);
	Read_MappingTable (fdes,offset,length);
}

/* read first 12 bytes of Offset Table */

FUNCTION LOCAL_PROTO void Read_OffsetTable(FILE *fdes, sfnt_OffsetTable *offset_table)
{
	ufix8 *ptr;

	ptr=(ufix8 *)tt_get_font_fragment ((int32)fdes,(int32)0,(int32)12);

	offset_table->version = (ufix32)(GET_WORD(ptr)) << 16;
	offset_table->version += GET_WORD((ptr+2));
	offset_table->numOffsets = GET_WORD((ptr+4));
	offset_table->searchRange = GET_WORD((ptr+6));
	offset_table->entrySelector = GET_WORD((ptr+8));
	offset_table->rangeShift = GET_WORD((ptr+10));

	tt_release_font_fragment (ptr);

#ifdef DEBUG_CMAP
	printf("\n sfnt version %lX.%lX\n",offset_table->version>>16,offset_table->version&0xFFFF);		/* 0x10000 (1.0) */
	printf(" Number of Tables %d\n",offset_table->numOffsets);	/* number of tables */
	printf(" searchRange %d\n",offset_table->searchRange);		/* (max2 <= numOffsets)*16 */
	printf(" entrySelector %d\n",offset_table->entrySelector);	/* log2(max2 <= numOffsets) */
	printf(" rangeShift %d\n\n",offset_table->rangeShift);		/* numOffsets*16-searchRange*/
#endif

}

/* read directory of Tables */

FUNCTION LOCAL_PROTO int Read_DirectoryTable(FILE *fdes, sfnt_OffsetTable *sfntdir)
{
	int     ii, i, size;
	ufix8  *pf;
	char name_tag[5], rev_tag[5] ;
	ufix8 *ptr ;

        size = sfntdir->numOffsets * sizeof(sfnt_DirectoryEntry);

	sfntdir->table = (sfnt_DirectoryEntry *)malloc((long)size);
	if (sfntdir->table == BTSNULL)
	{
		fprintf(stderr, "*** Read_DirectoryTable(): malloc(sfntdir->table) fails\n\n");
   		exit(1);
	}

	ptr=(ufix8 *)tt_get_font_fragment ((int32)fdes,(int32)12,(int32)size);
	if ( ptr == BTSNULL )
	{
		fprintf(stderr, "*** Read_DirectoryTable(): tt_get_font_fragment() fails\n\n");
   		exit(1);
	}

	for (pf=ptr, ii=0; ii<sfntdir->numOffsets; ii++)
	{
		sfntdir->table[ii].tag = (ufix32)(GET_WORD(pf)) << 16;
		sfntdir->table[ii].tag += GET_WORD((pf+2));

		name_tag[4] = '\0' ;
		memcpy(name_tag,&(sfntdir->table[ii].tag),4);
#if INTEL
		for ( i = 0 ; i < 4; i++)
			rev_tag[3-i]=name_tag[i];
#else
		for ( i = 0 ; i < 4; i++)
			rev_tag[i]=name_tag[i];
#endif
		rev_tag[4] = '\0' ;
#ifdef DEBUG_CMAP
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
#ifdef DEBUG_CMAP
		printf(" table offset: %7ld",sfntdir->table[ii].offset);
		printf(" table length: %5ld\n",sfntdir->table[ii].length);
#endif
	}

	tt_release_font_fragment (ptr);

	return(0);
}

FUNCTION LOCAL_PROTO int Get_TableOffsetLength(FILE *fdes, sfnt_OffsetTable *sfntdir, char *dir_name, uint32 *offset, uint32 *length)
{
	int i, j;
	char *name_tag, rev_tag[5] ;
	ufix8 *ptr ;

	*offset= *length = 0L ;

	for (i=0; i<sfntdir->numOffsets; i++)
	{
		rev_tag[4] = '\0' ;
		name_tag=(char*)&(sfntdir->table[i].tag);
#if INTEL
		for ( j = 0 ; j < 4; j++)
			rev_tag[3-j]=name_tag[j];
#else
		for ( j = 0 ; j < 4; j++)
			rev_tag[j]=name_tag[j];
#endif
		if ( strcmp(rev_tag,dir_name) == 0 )
		{
			*offset = sfntdir->table[i].offset;
			*length = sfntdir->table[i].length;
		}
	}

	return 0 ;
}

FUNCTION LOCAL_PROTO int Read_OneMappingTable(FILE *fdes, long offset, sfnt_char2IndexDirectory *idxdir)
{
int i, j, k, nr, key;
ufix8 *ptr, *array ;
ufix16 *endCount, *startCount, *idDelta, *idOffset ;
ufix16 firstCode, endCode;
uint16 glyphID;

sfnt_mappingTable mappingTable ;
segMappingTable *seg_table ;
HighByteMappingTable highbyte_table ;
trimmedTableMapping *trim_table ;
subHeaders *sub_header ;


	ptr = (ufix8 *)tt_get_font_fragment ((int32)fdes,(int32)idxdir->platform[0].offset+offset,(int32)6);

	mappingTable.format =  GET_WORD((ptr));
	mappingTable.length =  GET_WORD((ptr+2));
	mappingTable.version = GET_WORD((ptr+4));

#ifdef DEBUG_CMAP
	printf(" mapping table: format %d length %d version %d\n\n",mappingTable.format,mappingTable.length,mappingTable.version);
#endif

	tt_release_font_fragment (ptr);

	ptr = (ufix8 *)tt_get_font_fragment ((int32)fdes,(int32)idxdir->platform[0].offset+offset,(int32)mappingTable.length);
	if ( ptr == BTSNULL )
                {
		fprintf(stderr, "*** Read_OneMappingTable(): tt_get_font_fragment() fails\n\n");
   		exit(1);
                }
                
        /*****   CASE STATEMENT for MAPPING TABLE FORMATS */

        switch(mappingTable.format)
                {


                case 0:         /* Byte Encoding Table */
                         

                        array = ptr + 6 ;
#if DEBUG_CMAP == 2
                        for ( j = 0 ; j < 256 ; j++ )
                                printf(" Apple Index %3d: Glyph %3d\n",j,array[j]);
#endif
                        break;
                                

                case 2:         /* High-byte Mapping Through Table */

			highbyte_table.format =  GET_WORD((ptr));
			highbyte_table.length =  GET_WORD((ptr+2));
			highbyte_table.version = GET_WORD((ptr+4));
			highbyte_table.subHeaderKey = (uint16*)(ptr+6);

			charRange = (rangeCount *)malloc(sizeof(rangeCount)*256);

			if (charRange == BTSNULL )
                                {
				fprintf(stderr, "*** Read_OneMappingTable(): malloc(charRange) fails\n\n");
   				exit(1);
                                }

		        ptr += 518 ;

			for ( j = 0 ; j < 256 ; j++ )
                                {

				key = SWAPW(highbyte_table.subHeaderKey[j]);
#if DEBUG_CMAP == 1
				printf(" High Byte %3d: Key %u\n",j,key);
#endif
				if (key)
                                        {

					sub_header = (subHeaders*)(ptr+key);
                                             
                                                /* Check for zero entry count */

                                        if (SWAPW(sub_header->entryCount))
                                                {

					        charRange[j].startCount    = SWAPW(sub_header->firstCode);
					        charRange[j].endCount      = SWAPW(sub_header->entryCount) + charRange[j].startCount - 1;
                                                charRange[j].idDelta       = SWAPW(sub_header->idDelta);
                                                charRange[j].idRangeOffset = SWAPW(sub_header->idRangeOffset);
#if DEBUG_CMAP == 1
                                                printf("  First Code 0x%04X: Last Code 0x%04X: ID Delta %5d: ID Offset %u\n",
                                                        charRange[j].startCount | (j<<8), charRange[j].endCount | (j<<8),
                                                        charRange[j].idDelta, charRange[j].idRangeOffset);
#endif

#if DEBUG_CMAP == 2                                 
                                                for (k=charRange[j].startCount; k<=charRange[j].endCount; k++)
                                                        {
                                                        glyphID = *(charRange[j].idRangeOffset/2 + (k - charRange[j].startCount) + &(sub_header->idRangeOffset)); 
                                                        
                                                        if (glyphID)
                                                                glyphID += charRange[j].idDelta;
                                              
                                                        printf("  Character Code 0x%04X: Glyph %5d\n", (j<<8) | k, glyphID);
                                                        }
#endif 
                                                }

                                        }       /* End of if (key) */

                                }       /* End of for (j=0; j<256; j++) */

			free(charRange);
                        break;
                                

                case 4:         /* Segment Mapping to Delta Values */

                        seg_table = (segMappingTable*) ptr;
                        nr = SWAPW(seg_table->segCountX2);

                        nr = nr>>1 ;
                        endCount = (ufix16*)&(seg_table->endCount) ;
                        startCount = &endCount[nr+1] ;
                        idDelta = &endCount[2*nr+1] ;
                        idOffset = &endCount[3*nr+1] ;

                        charRange = (rangeCount *)malloc(sizeof(rangeCount)*nr);

                        if (charRange == BTSNULL)
                                {
                                fprintf(stderr, "*** Read_OneMappingTable(): malloc(charRange) fails\n\n");
                                exit(1);
                                }

                        for ( j = 0 ; j < nr ; j++ )
                                {

                                charRange[j].startCount    = SWAPW(startCount[j]);
                                charRange[j].endCount      = SWAPW(endCount[j]);
                                charRange[j].idDelta       = SWAPW(idDelta[j]);
                                charRange[j].idRangeOffset = SWAPW(idOffset[j]);
#if DEBUG_CMAP == 1
                                printf(" Range %3d: First Code 0x%04X: Last Code 0x%04X: ID Delta %5d: ID Offset %u\n",
                                        j, charRange[j].startCount, charRange[j].endCount, charRange[j].idDelta,
                                        charRange[j].idRangeOffset);
#endif        

#if DEBUG_CMAP == 2
                                for ( k=charRange[j].startCount; k<=charRange[j].endCount; k++)
                                        {
                                        if (charRange[j].idRangeOffset == 0)
                                                glyphID = (charRange[j].idDelta + k) % 65535;
                                        else
                                                {
                                                glyphID = *(charRange[j].idRangeOffset/2 + (k - charRange[j].startCount) + &idOffset[j]);
                                                
                                                if (glyphID)
                                                        glyphID += charRange[j].idDelta;
                                                
                                                printf("  Character Code 0x%04X: Glyph %5d\n", k, glyphID);
                                                }
                                        }
#endif
                                }                        
                        free(charRange);
                        break;

                case 6:         /* Trimmed Table Mapping */
                                
                        trim_table = (trimmedTableMapping*)ptr;

                        firstCode  = SWAPW(trim_table->firstCode);
                        endCode    = firstCode + SWAPW(trim_table->entryCount) - 1;

#if DEBUG_CMAP == 1
                        printf(" First Code 0x%04X: Last Code 0x%04X\n\n", firstCode, endCode);
#endif                        

#if DEBUG_CMAP == 2
                        for (j = firstCode ; j <= endCode ; j++)
                                printf("  Character Code 0x%04X: Glyph %5d\n", j, SWAPW(trim_table->glyphIdArray[j-firstCode]));
#endif
                        break;

                default:

#ifdef DEBUG_CMAP
                        printf("Undefined Character Mapping Format: %d\n", mappingTable.format);
#endif

                        break;

                }       /* End of switch (mappingTable.format) */

	tt_release_font_fragment (ptr);

	return 0 ;

}

FUNCTION LOCAL_PROTO int Read_MappingTable(FILE *fdes, long offset, long length)
{
	int     ii,i, size,nr;
	ufix8  *pf;
	ufix8 *ptr ;
	sfnt_char2IndexDirectory idxdir;


	ptr = (ufix8 *)tt_get_font_fragment ((int32)fdes,(int32)offset,(int32)length);
	idxdir.version = GET_WORD((ptr));
	idxdir.numTables = GET_WORD((ptr+2));
#ifdef DEBUG_CMAP
	printf("\n cmap version %d number of tables %d\n",idxdir.version,idxdir.numTables);
#endif
	tt_release_font_fragment (ptr);

        size = idxdir.numTables * sizeof(sfnt_platformEntry);
	ptr=tt_get_font_fragment ((int32)fdes,(int32)(offset+4),(int32)size);
	if ( ptr == BTSNULL )
	{
		fprintf(stderr, "*** Read_MappingTable(): tt_get_font_fragment() fails\n\n");
   		exit(1);
	}

	nr = idxdir.numTables ;

	for (pf=ptr, ii=0; ii<nr; ii++)
	{
		idxdir.platform[0].platformID = (GET_WORD(pf)) ;
		pf += 2;
		idxdir.platform[0].specificID = (GET_WORD(pf)) ;
		pf += 2;
		idxdir.platform[0].offset = (ufix32)(GET_WORD(pf)) << 16;
		idxdir.platform[0].offset += GET_WORD((pf+2));
		pf += 4;
#ifdef DEBUG_CMAP
		printf("\n platform ID: %d",idxdir.platform[0].platformID);
		printf(" encoding ID: %d",idxdir.platform[0].specificID);
		printf(" table offset: %ld\n",idxdir.platform[0].offset);
#endif
		Read_OneMappingTable (fdes,offset,&idxdir);
	}

	tt_release_font_fragment (ptr);

	return(0);
}

#endif /* INCL_CMAP */

