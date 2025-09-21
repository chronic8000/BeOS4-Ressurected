#ifndef _FONT_DEFS_H
#define _FONT_DEFS_H

#include "gr_types.h"

#ifdef __cplusplus
class SPath;
#endif

enum {
	FC_MAX_STRING      = 128
};

enum {
	FC_INVALIDATE      = 0,
	
	FC_BLACK_AND_WHITE = 1,		// packed 1 bit per pixel
	FC_TV_SCALE        = 2,		// packed 4 bits per pixel, 3 used
	FC_GRAY_SCALE      = 3,		// packed 4 bits per pixel, 3 used
	
	FC_GASP_UNKNOWN	   = 4,
	
	FC_MAX_BPP         = 5
};

typedef struct {
	int         x;
	int         y;
} fc_point;

typedef struct {
	float       left;
	float       top;
	float       right;
	float       bottom;
} fc_bbox;

/**********************************************************************

                           WARNING  WARNING

   WARNING : fc_char is created by several memory allocation. So its
   global size is not sizeof(fc_char) but the size return by the next
   macro. Also, people should be extremely careful when modifying or
   adding item in fc_char or any fc_char item. */

typedef struct {
	float       left;
	float       right;
} fc_edge;

typedef struct {
	int8		char_offset;	/* offset in 1/16 of pixels, in B_CHAR_SPACING mode */
	int8        offset;         /* left offset in escapement for bitmap mode */
	uint16		_reserved_;		/* for future extension */
} fc_spacing;

typedef struct {
	short       left;           /* bounding box of the string relative */
	short       top;            /* to the current origin point */
	short       right;
	short       bottom;
} fc_rect;

typedef struct {
	float       x_escape;       /* escapement to next character, */
	float       y_escape;
	int32		x_bm_escape;	/* escapement for bitmap spacing */
	int32		y_bm_escape;
} fc_escape;

/**********************************************************************

                           WARNING  WARNING

   WARNING : fc_char is created by several memory allocation. So its
   global size is not sizeof(fc_char) but the size return by the next
   macro. Also, people should be extremely careful when modifying or
   adding item in fc_char or any fc_char item. */

#define CHAR_HEADER_SIZE (sizeof(fc_edge)+sizeof(fc_rect)+sizeof(fc_escape)+sizeof(fc_spacing))

typedef struct {
	fc_edge     edge;              /* edges of the scalable character */
	fc_rect     bbox;              /* bounding box relative to the
									  baseline origin point (bitmap) */
	fc_escape   escape;            /* all the character escapements */
	fc_spacing  spacing;           /* information for advanced spacing modes */
	                               /* the character bitmap follows just there :
									  - 2 pixels/byte in 3 bits per pixel,
									    byte-aligned for each line.
									  - run encoded for 1 bits per pixel,
									    [run0, run1, run0, run1, ..., 0xff]
										[0xfe for 254+more] */
} fc_char;

typedef struct {
	rect        bbox;                        /* bounding box of the string */
	uchar       bits_per_pixel;
	uchar       _reserved_;
	ushort      char_count;                  /* character count of the string */
	uint32      set_hit_ref;                 /* character count of the string */
	fc_point    offsets[FC_MAX_STRING];      /* offset of each char */
	fc_char     *chars[FC_MAX_STRING];       /* the char descriptors array */ 							   
} fc_string;

typedef struct {
	int         count;
	fpoint		offsets[FC_MAX_STRING];
	fc_escape   escapes[FC_MAX_STRING];
} fc_escape_table;


typedef struct {
	int         count;
	fc_edge     edges[FC_MAX_STRING];
} fc_edge_table;


typedef struct {
	int			count;
	frect		bbox[FC_MAX_STRING];
} fc_bbox_table;


#ifdef __cplusplus
typedef struct {
	int         count;
	SPath		*glyphs[FC_MAX_STRING];
} fc_glyph_table;
#endif

#endif
