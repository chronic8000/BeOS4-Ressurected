/*************************** H P F . H ***************************************
 *                                                                           *
 *                                                                           *
 ********************** R E V I S I O N   H I S T O R Y **********************
 *
 * $Header: //bliss/user/archive/rcs/4in1/hpf.h,v 14.0 97/03/17 17:21:51 shawn Release $
 *
 * $Log:	hpf.h,v $
 * Revision 14.0  97/03/17  17:21:51  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.1  96/08/20  12:50:03  shawn
 * Added RCS/Log keyword.
 * 
 *
 ****************************************************************************/

#ifndef hpf_h
#define hpf_h

static char hpf_h_rcsid[] = "$Header: //bliss/user/archive/rcs/4in1/hpf.h,v 14.0 97/03/17 17:21:51 shawn Release $";

typedef struct HPF_HEAD
{
ufix16  size;             /* always 64 */
ufix8   reserved1,        /* always 0 */
        font_type;        /* 0=7-bit,1=8-bit,2=PC-8 */
ufix16  reserved2,        /* always 0 */
        baseline,         /* top of em to baseline, PCPU */
        cell_width,       /* PCPU */
        cell_height;      /* PCPU */
ufix8   orient,           /* always 0 */
        spacing;          /* 0 = fixed, 1 = proportional */
ufix16  symbol_set,       /* HP symbol set */
        pitch,            /* default HMI for monospace, PCPU. 
                           0 if proportional font */
        height,           /* always 192 */
        x_height;         /* height of lowercase x from baseline, PCPU */
fix7    width_type,       /* -2 condensed to +2 expanded */
        style,            /* 0 upright, 1 italic */
        stroke_weight;    /* -7 to 7, 0 being normal */
ufix8   typeface,         /* bitsid from tdf */
        reserved3,        /* always 0 */
        serif_style;      /* same as cvthpf uses */
ufix16  reserved4;        /* always 0 */
ufix8   uline_dist;       /* baseline to center of underline, PCPU, unsigned dist below baseline */
fix7    uline_height;     /* thickness of underline, PCPU */
ufix16  text_height,      /* optimal line height, PCPU */
        text_width,       /* lc avg width, PCPU */
        reserved5,
        reserved6,
        pitch_n_height,
        reserved7,        /* always 0 */
        reserved8,
        reserved9;
char    name[16];
ufix16  xResolution; /* dots per inch */
ufix16  yResolution; /* dots per inch */
} HPF_HEAD;




typedef struct HPF_CHARDIR
{        
ufix8   format,
        continuation,
        size,
        class,
        orientation,
        reserved;
fix15   xoff,
        yoff,
        width,
        height,
        set_width;
} HPF_CHARDIR;    


#define ESC_FONTID "\033*c%uD"
#define ESC_FONTHEAD "\033)s64W"
#define ESC_FONTHEAD600 "\033)s68W"
#define ESC_CHARID "\033*c%uE"
#define ESC_CHARDATA "\033(s%uW"
#define ESC_ASSIGN_FONT_100 "\33*c0F\33*c100D"
#define ESC_MAKE_FONT_PERM "\33*c5F"
#define ESC_SELECT_FONT_100 "\33(100X"
#define ESC_TRANSDATA "\33&p1X"



#endif
