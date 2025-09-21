#ifndef _BE_GLUE_H
#define _BE_GLUE_H

#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <OS.h>
#include <stdarg.h>

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
#include "basic_types.h"
#include "font_renderer.h"
#include "ufe.h"		/* Unified front end */

#define MAX_INTER_PER_LINE   32
#define MAX_LINE             32
#define MAX_COLUMN           32
#define MAX_LINE2            24
#define MAX_COLUMN2          24
#define MAX_RUN_LIST        128

typedef struct diff_entry {
	char   total;
	char   diff[4];
	unsigned char  enable; /* the bit n for the direction coming from n */
	               /* bit 4 to mark a cell with potential diffusion */
	               /* bit 5 to mark a cell set to black */
} diff_entry;

typedef struct cell {
	char   total;
	char   diff[4];
	unsigned char  enable; /* the bit n for the direction coming from n */
	               /* bit 4 to mark a cell with potential diffusion */
	               /* bit 5 to mark a cell set to black */
	char   total0;
	char   out;
} cell;

typedef struct cell_ref {
	cell             *c;
	struct cell_ref  *next;
} cell_ref;

#define  MAX_DIFFUSE  (24+4*4+1)

/* struct per instance */
typedef struct fc_renderer_context {
	int32            error;
	int32            was_load;
	fix15            raswid;             /* raster width  */
	fix15            rashgt;             /* raster height */
	specs_t          gspecs;
	FILE             *font_fp;
	font_data		 *font_ptr;
	void			 *glyph_path;
	float			 bbox[4];
	double			 matrix[6];
	SPEEDO_GLOBALS   sp_global;
	int              mode_id;
	int              cur_line;
	int              run_count;
	char             *gray_bitmap;
	void             *set_ptr;
	FC_GET_MEM       fc_get_mem;
	int              escape_h, escape_v;
	int              cur_v, cur_h;	
	int              offset_h, offset_v;
	int              base_h, base_v;
	int              size_h, size_v, size3_h, size3_v;
	int              s3_min_h, s3_max_h, s3_min_v, s3_max_v;
	cell_ref         *end_list;
	float            inter_h[(MAX_LINE+1) * 3 * (MAX_INTER_PER_LINE+1)];
	char             open_stat[(MAX_LINE+1) * 3 * (MAX_INTER_PER_LINE+1)];
	int              index[(MAX_LINE+1) * 3];	
	ushort           run_list[(MAX_RUN_LIST+1)];
	char             fill_array[(MAX_LINE+1) * 3 * (MAX_COLUMN+1) * 3];
	cell             cell_array[(MAX_LINE2+1) * (MAX_COLUMN2+1)];
	cell_ref         *diff_index[(MAX_DIFFUSE+1)];
	cell_ref         diff_list[(MAX_COLUMN2+1)*(MAX_LINE2+1)];
} fc_renderer_context;

#ifdef __cplusplus
extern "C" {
#endif

CHARACTERNAME *unicode_2_psname(uint16 code, int32 iso_set, CHARACTERNAME **names);

void sp_open_outline_wrap(SPEEDO_GLOBALS *sp_global_ptr, fix31 x_width, fix31 y_width,
						  fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax);
void sp_start_new_char_wrap(SPEEDO_GLOBALS *sp_global_ptr);
void sp_start_contour_wrap(SPEEDO_GLOBALS *sp_global_ptr, fix31 x, fix31 y, char outside);
void sp_curve_to_wrap(SPEEDO_GLOBALS *sp_global_ptr,
					  fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3);
void sp_line_to_wrap(SPEEDO_GLOBALS *sp_global_ptr, fix31 x, fix31 y);
void sp_close_contour_wrap(SPEEDO_GLOBALS *sp_global_ptr);
void sp_close_outline_wrap(SPEEDO_GLOBALS *sp_global_ptr);

#ifdef __cplusplus
}
#endif

#endif
