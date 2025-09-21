/***********************************************************************************************
	FILE:		FINFOTBL.H
***********************************************************************************************/
/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
* $Header: //bliss/user/archive/rcs/fit/finfotbl.h,v 6.0 97/03/17 17:36:13 shawn Release $
*
* $Log:	finfotbl.h,v $
 * Revision 6.0  97/03/17  17:36:13  shawn
 * TDIS Version 6.00
 * 
 * Revision 4.3  96/12/05  11:06:21  shawn
 * Finalized support for the LJ5/LJ5M/LJ6L printers.
 * 
 * Revision 4.2  96/11/27  13:53:47  shawn
 * Modified to support the Pseudo-Light Courier family.
 * 
 * Revision 4.1  96/07/02  12:21:17  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 4.0  95/02/15  16:49:17  shawn
 * Release
 * 
 * Revision 3.4  95/02/13  10:31:57  roberte
 * Added table entries for Compressed/Combined TrueDoc 80 font set
 * PFR (LJ4 + PS35).
 * 
 * Revision 3.3  95/01/10  08:57:52  roberte
 * Updated copyright header.
 * 
*****************************************************************************/


#if INCL_MFT
#if PROC_TRUEDOC
***Intentional compile error: INCL_MFT and PROC_TRUEDOC incompatible configuration combination!
#endif
#ifdef HAVE_STICK_FONT
#define N_LOGICAL_FONTS	182
#else
#define N_LOGICAL_FONTS	181
#endif
#else	/* else !INCL_MFT */
#if PROC_TRUEDOC && defined(COMPRESSED_PFR)
#ifdef HAVE_STICK_FONT
#define N_LOGICAL_FONTS	165
#else
#define N_LOGICAL_FONTS	164
#endif
#else
#ifdef HAVE_STICK_FONT
#define N_LOGICAL_FONTS	155
#else
#define N_LOGICAL_FONTS	154
#endif
#endif
#endif /* INCL_MFT */


#define NULLCHARPTR	(ufix8 *)0
#define NEXT_NONE	0x0100
#define NEXT_ABSTARG	0x0200
#define	NEXT_RELTARG	0x0400

#define BUCKET		(NEXT_ABSTARG|0x0000)
#define SERIF		(NEXT_ABSTARG|0X0001)
#define SERIF_BOLD	(NEXT_ABSTARG|0x0002)
#define SANS		(NEXT_ABSTARG|0x0003)
#define SANS_BOLD	(NEXT_ABSTARG|0x0004)
#define STICK		(NEXT_ABSTARG|0x0005)

#define UP_ONE		(NEXT_RELTARG|0x00ff)
#define UP_TWO		(NEXT_RELTARG|0x00fe)
#define UP_THREE	(NEXT_RELTARG|0x00fd)
#define UP_FOUR		(NEXT_RELTARG|0x00fc)
#define UP_FIVE		(NEXT_RELTARG|0x00fb)

#define UP_NINE         (NEXT_RELTARG|0x00f7)

#define DOWN_ONE	(NEXT_RELTARG|0x0001)
#define DOWN_TWO	(NEXT_RELTARG|0x0002)
#define DOWN_THREE	(NEXT_RELTARG|0x0003)
#define DOWN_FOUR	(NEXT_RELTARG|0x0004)
#define DOWN_FIVE	(NEXT_RELTARG|0x0005)
#define DOWN_SIX	(NEXT_RELTARG|0x0006)

#define NEXT_UNKNOWN	NEXT_NONE


enum {pdlPCL=128, pdlPostScript, pdlGDI, pdlSupport};

typedef struct VFNT_HEAD  /* Logical font header */
{
ufix16  size;             /* always 88 */
ufix8   format,           /* always 12 */
        font_type;        /* 0=7-bit,1=8-bit,2=PC-8,10=unbound scalable */
ufix16  style_msb,        /* high byte of style word */
        baseline,         /* top of em to baseline, PCPU */
        cell_width,       /* PCPU */
        cell_height;      /* PCPU */
ufix8   orient;           /* always 0 */
btsbool spacing;          /* 0 = fixed, 1 = proportional */
ufix16  symbol_set,       /* HP symbol set (always 0)*/
        pitch,            /* default HMI for monospace, PCPU. 
                             0 if proportional font */
        height,           /* always 192 */
        x_height;         /* height of lowercase x from baseline, PCPU */
fix7    width_type;       /* -2 condensed to +2 expanded */
ufix8   style_lsb;        /* 0 upright, 1 italic */
fix7    stroke_weight;    /* -7 to 7, 0 being normal */
ufix8   typeface_lsb,     /* bitsid from tdf */
        typeface_msb,     /* always 0 */
        serif_style;      /* same as cvthpf uses */
ufix8   quality;          /* font quality */
fix7    placement;        /* placement of chars relative to baseline */
fix7    uline_dist;       /* baseline to center of underline, PCPU/4 ?? */
ufix8   old_uline_height; /* thickness of underline, PCPU */
ufix16  reserved1,        /* reserved */
        reserved2,        /* reserved */
        reserved3,        /* reserved */
        num_outlines;     /* number of outlines to download (unused) */
ufix8   pitch_ext,        /* extended 8 bits for pitch field */
        height_ext;       /* extended 8 bits for height field */
ufix16  cap_height;       /* distance from capline to the baseline */
ufix32  font_number;      /* vendor-assigned font number */
char    font_name[16];    /* font name string */
ufix16  scale_factor,     /* scale factor in design window units */
        master_x_res,     /* horizontal pixel resolution */
        master_y_res;     /* vertical pixel resolution */
fix15   uline_pos;        /* position of underline */
ufix16  uline_height,     /* thickness of underline */
        lre_thresh,       /* low resolution enhancement threshold */
        italic_angle;     /* tangent of italic angle times 2**15 */
ufix32  char_complement_msw, /* character complement MSW */
        char_complement_lsw; /* character complement LSW */	
ufix16  data_size;        /* not used */
} pclHdrType;


typedef struct FontInfoType_t
{
char		stringName[48];		/* font alias name */
pclHdrType	pclHdrInfo;		/* attribute/metric info about font in PCL Intellifont format */
ufix8		pdlType;		/* font emulation type (pdlPCL ... pdlSupport) */
ufix8		*addr;			/* to be filled in after font is loaded or ROM burned */
ufix16		nextSearchEncode;	/* hi,lo byte encoded indicator of next index */
ufix16		resolutionReq;		/* required resolution for Merge Font Technology */
} FontInfoType;

#define RES_NATIVE		0
#define	RES_1000		1000
#define	RES_2048		2048
#define	RES_8782		8782


