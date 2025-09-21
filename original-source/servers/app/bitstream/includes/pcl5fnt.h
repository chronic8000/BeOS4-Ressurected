
/*      pcl5fnt.h       */

/*
*
*       $Header: //bliss/user/archive/rcs/4in1/pcl5fnt.h,v 14.0 97/03/17 17:22:59 shawn Release $
*
*       $Log:	pcl5fnt.h,v $
 * Revision 14.0  97/03/17  17:22:59  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.1  96/08/20  12:55:22  shawn
 * Added RCS/Header and Log keywords.
 * 
*
*/

typedef struct
    { 
    ufix8  structure;
    ufix8  width;
    ufix8  posture;
    } style_t;

typedef struct
    { 
    ufix8  vendor;
    ufix8  version;
    ufix16  value;
    } typeface_t;

typedef struct
{
ufix16  size;           /* 0   1  always 80 */
ufix8   dformat,        /* 2      always 10 */
        font_type;      /* 3      0=7-bit,1=8-bit,2=PC-8 */
style_t style;          /* 4 23   */
ufix16  baseline,       /* 6   7  bottom of em to baseline, PCPU */
        cell_width,     /* 8   9  PCPU */
        cell_height;    /* 10 11  PCPU */
ufix8   orientation,    /* 12     always 0 */
        spacing;        /* 13     0 = fixed, 1 = proportional */
ufix16  symbol_set,     /* 14 15  HP symbol set */
        pitch,          /* 16 17  default HMI for monospace, PCPU. 
                                  0 if proportional font */
        height,         /* 18 19  always 192 */
        x_height;       /* 20 21  height of lowercase x from baseline, PCPU */
fix7    width_type;     /* 22     -2 condensed to +2 expanded */
fix7    stroke_weight;  /* 24     -7 to 7, 0 being normal */
typeface_t typeface;    /* 26 25  Typeface Value(bits 0 - 8)  */ 
                        /*        Version(bits 9 -10) Vendor(bits 11-14) */ 
ufix8   serif_style,    /* 27     same as cvthpf uses */
        quality;        /* 28     always letter quality (2) */
fix7    placement,      /* 29     */
        uline_dist;     /* 30     always 0 */ 
ufix8   uline_height;   /* 31     always 0 */
ufix16  text_height,    /* 32 33  always 0 */
        text_width,     /* 34 35  always 0 */
        first_code,     /* 36 37  */
        last_code;      /* 38 39  */
ufix8   pitch_extend,   /* 40     */
        height_extend;  /* 41     */
ufix16  cap_height;     /* 42 43  */
ufix8   font_vendor_code;  /* 44  */
ufix32  font_number;    /* 45 46 47 */
                        /* 45 46 47 */ 
char    fontname[16];   /* 48-63    */
ufix16  scale_factor,   /* 64 65    */
        x_resolution,   /* 66 67    */
        y_resolution;   /* 68 69    */
fix15   mstr_uline_pos; /* 70 71    baseline to top of underline */
ufix16  mstr_uline_hght,/* 72 73    thickness of underline */
        threshold;      /* 74 75    */
fix15   italic_angle;   /* 76 77    */
ufix16  scal_data_size; /* 78 79    */
fix7   *scal_data;      /* 80 ...   */
ufix16  nobtf;          /* 80 + .scal_data_size -- Copyright notice size */
char   *copyright;      /* 80 + .scal_data_size + 3 ...    */
ufix8   checksum;       /* 80 + .scal_data_size + .nobtf
                           sigma bytes 64 through c.s. = 0 */
ufix16  total_size;     /* 80 + .scal_data_size + .notbf + 2 */
} PC5HEAD;

