#include "be_glue.h"

/***********************************************************************************/

/* 0 : 0.2500, 0.5000, 0.7500
   1 : 0.1667, 0.5000, 0.8333
   2 : 0.1875, 0.5000, 0.8125
   3 : 0.2109, 0.5000, 0.7891
   */
#define ALIGN 2
#define SEUIL 8

/* shared globals */
static  int        debug;
static  int        diffusion_init = 0;
static	diff_entry t_diff[512];

/* extension for user define output module. */ 
LOCAL_PROTO char init_out_user(SPEEDO_GLOBALS *sp_global_ptr, specs_t *specs);
LOCAL_PROTO char begin_char_user(SPEEDO_GLOBALS *sp_global_ptr, point_t, point_t, point_t);
LOCAL_PROTO void begin_sub_char_user(SPEEDO_GLOBALS *sp_global_ptr, point_t, point_t, point_t);
LOCAL_PROTO void begin_contour_user(SPEEDO_GLOBALS *sp_global_ptr, point_t, char);
LOCAL_PROTO void curve_user(SPEEDO_GLOBALS *sp_global_ptr, point_t, point_t, point_t, fix15);
LOCAL_PROTO void line_user(SPEEDO_GLOBALS *sp_global_ptr, point_t);
LOCAL_PROTO void end_contour_user(SPEEDO_GLOBALS *sp_global_ptr);
LOCAL_PROTO void end_sub_char_user(SPEEDO_GLOBALS *sp_global_ptr);
LOCAL_PROTO char end_char_user(SPEEDO_GLOBALS *sp_global_ptr);
GLOBAL_PROTO char sp_init_userout(SPEEDO_GLOBALS *sp_global_ptr, specs_t *specs);

#define MAX_BITS  10000             /* Max line length of generated bitmap */

void init_diffusion_table();

/* Called by character generator to initialize a buffer prior
   to receiving bitmap data. */
FUNCTION void WDECL sp_open_bitmap(SPEEDO_GLOBALS *sp_global_ptr,
								   fix31 x_set_width,
								   fix31 y_set_width,
								   fix31 xorg,
								   fix31 yorg,
								   fix15 xsize,
								   fix15 ysize)
{
	int                 i, j, bitmap_size;
	cell                *cell_line;
	char                *ptr;
	uchar               *bitmap;
	fc_rect             *bound;
	boolean             negative;
	fc_escape           *escape;
	fc_renderer_context *ctxt;
	
	ctxt = (fc_renderer_context*)(sp_global_ptr->UserPtr);
	ctxt->error = 0;
	if (xsize > MAX_BITS)
		xsize = MAX_BITS;
	/* allocate memory for char header (except edges).
	   Reserved space for spacing information. */
	ptr = (char*)(ctxt->fc_get_mem)(ctxt->set_ptr,
									sizeof(fc_rect)+sizeof(fc_escape)+sizeof(fc_spacing));
	/* record character bounding box */
	bound = (fc_rect*)ptr;
	bound->left = ((xorg+(1<<15))>>16);
	bound->bottom = ((-yorg+(1<<15))>>16)-1;
	bound->right = bound->left+xsize-1;
	bound->top = bound->bottom-(ysize-1);
	/* record escapement infos */
	escape = (fc_escape*)(ptr+sizeof(fc_rect));
	escape->x_escape = (float)x_set_width*(1.0/65536.0);
	escape->y_escape = -(float)y_set_width*(1.0/65536.0);
	
	/* special preparation for each mode */
	if (ctxt->mode_id == MODE_2D) {
		ctxt->cur_line = 0;
		ctxt->run_count = 0;
		ctxt->rashgt = ysize;
	}
	else if (ctxt->mode_id == MODE_GRAY) {
		ctxt->raswid = ((xsize+1)>>1);
		ctxt->rashgt = ysize;
		bitmap_size = ctxt->raswid * ctxt->rashgt;
		ctxt->gray_bitmap = (ctxt->fc_get_mem)(ctxt->set_ptr, bitmap_size);
		memset(ctxt->gray_bitmap, 0, bitmap_size);
	}
}

/* code a list of run in bw_bitmap run length encoding */
static void flush_cur_line(fc_renderer_context *ctxt) {
	int        i, j, index, count;
	int        action;
	uchar      *buf;
	ushort     tmp1, tmp2;
	ushort     *list;
	
	/* empty case */
	if (ctxt->run_count == 0) {
		buf = (uchar*)(ctxt->fc_get_mem)(ctxt->set_ptr, 1);
		*buf = 0xff;   /* end */
	}
	else {
		/* sort the runs */
		count = ctxt->run_count-2;
		if (count > 0)
			do {
				action = FALSE;
				list = ctxt->run_list;
				
				for (i=0; i<count; i+=2) {
					if (list[0] > list[2]) {
						action = TRUE;
						tmp1 = list[0];
						tmp2 = list[1];
						list[0] = list[2];
						list[1] = list[3];
						list[2] = tmp1;
						list[3] = tmp2;
					}
					list += 2;
				}
				count-=2;
			} while (action);
		/* compaction */
		i = 0;
		for (j=2; j<ctxt->run_count; j+=2) {
			if (ctxt->run_list[i+1] >= ctxt->run_list[j]) {
				if (ctxt->run_list[i+1] < ctxt->run_list[j+1]) 
					ctxt->run_list[i+1] = ctxt->run_list[j+1];
			}
			else {
				ctxt->run_list[i+2] = ctxt->run_list[j];
				ctxt->run_list[i+3] = ctxt->run_list[j+1];
				i+=2;
			}
		}
		ctxt->run_count = i+2;
		
		/* calculate the length of the byte encoded version */
		index = 0;
		count = 1;
		for (i=0; i<ctxt->run_count; i++) {
			while (ctxt->run_list[i] >= index+254) {
				index += 254;
				count++;
			}
			index = ctxt->run_list[i];
			count++;
		}
		/* encode the byte run length */
		buf = (uchar*)(ctxt->fc_get_mem)(ctxt->set_ptr, count);
		index = 0;
		for (i=0; i<ctxt->run_count; i++) {
			while (ctxt->run_list[i] >= index+254) {
				index += 254;
				*buf++ = 0xfe;  /* extender */
			}
			*buf++ = ctxt->run_list[i]-index;
			index = ctxt->run_list[i];
		}
		*buf++ = 0xff; /* end */
	}
	/* init for next line */
	ctxt->run_count = 0;
	ctxt->cur_line++;
}
	
/* Called by character generator to write one row of pixels 
   into the generated bitmap character. */
FUNCTION void WDECL sp_set_bitmap_bits(SPEEDO_GLOBALS *sp_global_ptr,
									   fix15 y, fix15 xbit1, fix15 xbit2)
{
	fc_renderer_context *ctxt;
	
	ctxt = (fc_renderer_context*)(sp_global_ptr->UserPtr);
	/* clip inside the limits */
	if (xbit1 < 0)
		xbit1 = 0;
	if (xbit1 >= MAX_BITS)
		return;
	if (xbit2 >= MAX_BITS)
		xbit2 = MAX_BITS;
	if (xbit1 >= xbit2)
		return;
	
	if (ctxt->mode_id == MODE_2D) {
		/* flush out all the lines from the current line to the new line */
		while (y > ctxt->cur_line)
			flush_cur_line(ctxt);
		/* record this run */
		if (ctxt->run_count < MAX_RUN_LIST) {
			ctxt->run_list[ctxt->run_count++] = xbit1;
			ctxt->run_list[ctxt->run_count++] = xbit2;
		}
	}
	else if (ctxt->mode_id == MODE_GRAY) {
		int       x1, x2;
		char      *buf;

		/* left and right limit (left include, right exclude) */
		x1 = xbit1;
		x2 = xbit2;
		/* set potential non double extremas to black */
		buf = ctxt->gray_bitmap + ctxt->raswid*y;
		if (x2 & 1)
			buf[x2>>1] |= 0x70;
		if (x1 & 1)
			buf[x1>>1] |= 0x07;
		/* set all the paired pixels to black */
		x1 = (x1+1)>>1;
		x2 = x2>>1;
		for (; x1<x2; x1++)
			buf[x1] = 0x77;
	}
}

/* Called by output module to write one row of gray pixels
   into the generated bitmap character. */
FUNCTION void WDECL sp_set_bitmap_pixels(SPEEDO_GLOBALS *sp_global_ptr,
										 fix15            y,
										 fix15            xbit1,
										 fix15            num_grays,
										 void             *gray_buf)

{
	int                 x1, x2;
	char                *buf;
	fc_renderer_context *ctxt;
	
	ctxt = (fc_renderer_context*)(sp_global_ptr->UserPtr);
	/* left and right limit (left include, right exclude) */
	if (xbit1 < 0)
		x1 = 0;
	else
		x1 = xbit1;
	if (x1 >= MAX_BITS)
		return;
    x2 = x1 + num_grays;
	if (x2 >= MAX_BITS)
		x2 = MAX_BITS;
	/* copy potential non double extremas */
	buf = ctxt->gray_bitmap + ctxt->raswid*y;
	if (x2 & 1)
		buf[x2>>1] |= ((char*)gray_buf)[num_grays-1]<<4;
	if (x1 & 1)
		buf[x1>>1] |= *((char*)gray_buf)++;
	/* copy all the paired pixels */
	x1 = (x1+1)>>1;
	x2 = x2>>1;
	for (; x1<x2; x1++) {
		buf[x1] = (((char*)gray_buf)[0]<<4) | ((char*)gray_buf)[1];
		((char*)gray_buf) += 2;
	}
}

/* Called by character generator to indicate all bitmap data
   has been generated. */
FUNCTION void WDECL sp_close_bitmap(SPEEDO_GLOBALS *sp_global_ptr) {
	fc_renderer_context *ctxt;
	
	ctxt = (fc_renderer_context*)(sp_global_ptr->UserPtr);
	if (ctxt->mode_id == MODE_2D) {
		/* flush out all the remaining lines */
		while (ctxt->rashgt > ctxt->cur_line)
			flush_cur_line(ctxt);		
	}
}

/* character rendering entry */
int fc_make_char(struct fc_renderer_context *ctxt,
				 void *char_desc, void *set, FC_GET_MEM get_mem) {
	int        ret_val;
	bbox_t     bbox;
	fc_edge    *edge;
	ufe_struct tspecs;

	/* global copy of the memory allocator hook */
	ctxt->fc_get_mem = get_mem;
	ctxt->set_ptr = set;
	/* calculate edges in left relative coordinates */
	edge = (fc_edge*)(ctxt->fc_get_mem)(ctxt->set_ptr, sizeof(fc_edge));
	edge->left = 1234567.0;
	edge->right = 1234568.0;
	/* call renderer */
	ret_val = false;

	switch (ctxt->was_load) {
	case 1 :
		ret_val = fi_make_char(&ctxt->sp_global, char_desc);
		if (ctxt->error) {	
			ctxt->mode_id = MODE_GRAY;
			ctxt->gspecs.flags = MODE_GRAY;		
			tspecs.Gen_specs = &ctxt->gspecs;		
			if (!fi_set_specs(&ctxt->sp_global, &tspecs))
				return FALSE;
			ret_val = fi_make_char(&ctxt->sp_global, char_desc);
			if (ctxt->error)
				ret_val = false;
		}
		break;
	case 2 :
	 	{
			CHARACTERNAME	*name;
			
			name = unicode_2_psname(((uint16*)char_desc)[0], 11,
									tr_get_encode(&ctxt->sp_global));
			if (name == NULL)
				ret_val = false;
			else
        		ret_val = tr_make_char(&ctxt->sp_global, name);
			if (ctxt->error)
				ret_val = false;
		}
		break;
	}
	return ret_val;
}

/* character rendering entry */
void fc_make_safety_char(struct fc_renderer_context *ctxt,
						 float size, int bpp, void *set, FC_GET_MEM get_mem) {
	int         i, j;
	int         hmin, hmax, dv, dh;
	char        *ptr;
	bbox_t      bbox;
	fc_edge     *edge;
	fc_rect     *bound;
	fc_escape   *escape;
	
	if (size > 250.0) size = 250.0;
	if (size < 5.0) size = 5.0;
	hmin = (int)(size*0.08+0.5);
	hmax = (int)(size*0.50+0.5);
	dv = (int)(size*0.68+0.5);
	/* allocate memory for char header (except edges).
	   Reserved space for spacing information. */
	ptr = (char*)(get_mem)(set, sizeof(fc_edge)+sizeof(fc_rect)+
						   sizeof(fc_escape)+sizeof(fc_spacing));
	/* calculate edges in left relative coordinates */
	edge = (fc_edge*)ptr;
	edge->left = size*0.08;
	edge->right = size*0.50;
	/* record character bounding box */
	bound = (fc_rect*)(ptr+sizeof(fc_edge));
	bound->left = hmin;
	bound->bottom = 0;
	bound->right = hmax-1;
	bound->top = 1-dv;
	/* record escapement infos */
	escape = (fc_escape*)(ptr+sizeof(fc_edge)+sizeof(fc_rect));
	escape->x_escape = size*0.58;
	escape->y_escape = 0.0;
	/* create a black and white box */
	if (bpp == 1) {
		ptr = (char*)(get_mem)(set, 5*dv-4);
		for (i=0; i<dv; i++) {
			if ((i==0) || (i==(dv-1))) {
				ptr[0] = 0;
				ptr[1] = hmax-hmin+1;
				ptr[2] = 0xff;
				ptr += 3;
			}
			else {
				ptr[0] = 0;
				ptr[1] = 1;
				ptr[2] = hmax-hmin-1;
				ptr[3] = 1;
				ptr[4] = 0xff;
				ptr += 5;
			}
		}
	}
	/* create a grayscale box */
	else {		
		dh = (hmax-hmin+1)>>1;
		hmax = (hmax-hmin)&1;
		ptr = (char*)(get_mem)(set, dv*dh);
		for (i=0; i<dv; i++) {
			if ((i==0) || (i==(dv-1))) {
				for (j=0; j<dh; j++)
					ptr[j] = 0x77;
			}
			else {
				ptr[0] = 0x70;
				for (j=1; j<dh-1; j++)
					ptr[j] = 0x00;
				if (hmax)
					ptr[dh-1] = 0x70;
				else
					ptr[dh-1] = 0x07;
			}
			ptr += dh;
		}		
	}
}

/* character edges only processing */
void fc_process_edges(struct fc_renderer_context *ctxt, void *char_desc, fc_edge *edge) {
	ctxt->gspecs.flags = MODE_OUTLINE;		
	ctxt->mode_id = MODE_OUTLINE;
	ctxt->error = 0;
	ctxt->glyph_path = NULL;
	if (fc_outline_call_glue(ctxt, char_desc)) {
		edge->left = ctxt->bbox[0]*(1.0/250.0);
		edge->right = ctxt->bbox[2]*(1.0/250.0);
	}
	else {
		edge->left = 0.0;
		edge->right = 0.6;
	}
}

/* bounding box only processing */
void fc_process_bbox(struct fc_renderer_context *ctxt, void *char_desc, fc_bbox *bbox) {
	ctxt->gspecs.flags = MODE_OUTLINE;		
	ctxt->mode_id = MODE_OUTLINE;
	ctxt->error = 0;
	ctxt->glyph_path = NULL;
	if (fc_outline_call_glue(ctxt, char_desc)) {
		bbox->left = ctxt->bbox[0]*(1.0/250.0);
		bbox->right = ctxt->bbox[2]*(1.0/250.0);
		bbox->top = -ctxt->bbox[3]*(1.0/250.0);
		bbox->bottom = -ctxt->bbox[1]*(1.0/250.0);	
	}
	else {
		bbox->left = 0.0;
		bbox->right = 0.6;
		bbox->top = -1.0;
		bbox->bottom = 0.1;	
	}
}

/* PART 1 : reset character recording... */
static void reset(fc_renderer_context *ctxt,
				  int xmin, int ymin, int xmax, int ymax, int escx, int escy) {
	int      i;

	ctxt->error = 0;
	ctxt->offset_h = xmin>>8;
	ctxt->offset_v = -((ymax+255)>>8);
	ctxt->base_h = ctxt->offset_h<<8;
	ctxt->base_v = (ymax+255)&0xff00;
	ctxt->size_h = ((xmax+255)>>8)-ctxt->offset_h;
	ctxt->size_v = -ctxt->offset_v-(ymin>>8);
	ctxt->size3_h = 3*ctxt->size_h;
	ctxt->size3_v = 3*ctxt->size_v;
	ctxt->escape_h = escx;
	ctxt->escape_v = escy;

	if ((ctxt->size_h > MAX_COLUMN) || (ctxt->size_v > MAX_LINE)) {
		ctxt->error = 1;
		return;
	}
	
	for (i=0;i<ctxt->size3_v;i++)
		ctxt->index[i] = 0;
}

/* PART 2 : init new contour recording */
static void from(fc_renderer_context *ctxt, int x, int y) {
	int      cur_h, cur_v;
	
	cur_h = x-ctxt->base_h;
	cur_v = ctxt->base_v-y;
	
	if (cur_h < 0)
		cur_h = 0;
	else if (cur_h > (ctxt->size_h<<8))
		cur_h = ctxt->size_h<<8;
	if (cur_v < 0)
		cur_v = 0;
	else if (cur_v > (ctxt->size_v<<8))
		cur_v = ctxt->size_v<<8;

	ctxt->cur_h = cur_h;
	ctxt->cur_v = cur_v;
}

/* PART 3 : record a move to a new point */
static void moveto(fc_renderer_context *ctxt, int new_h, int new_v) {
	int     i, j, k, pos, jband, jsubband;
	int     v1, v2, s1, s2;
	int     tmp, min, max, status, cnt, cnt0;
	float   x1, x2, y1, y2, x, y, c0, c1;

	if (ctxt->error) return;
	/* translate to bitmap coordinates */
	new_h -= ctxt->base_h;
	new_v = ctxt->base_v-new_v;

	if (new_h < 0)
		new_h = 0;
	else if (new_h > (ctxt->size_h<<8))
		new_h = ctxt->size_h<<8;
	if (new_v < 0)
		new_v = 0;
	else if (new_v > (ctxt->size_v<<8))
		new_v = ctxt->size_v<<8;
	/* check it's not a horizontal */
	if (ctxt->cur_v != new_v) {
		/* determine the serial band of the 2 extremities */
#if (ALIGN == 3)
		s1 = 3*(ctxt->cur_v>>8)+(((ctxt->cur_v&0xff)+20)/74)-1;
		s2 = 3*(new_v>>8)+(((new_v&0xff)+20)/74)-1;
#elif (ALIGN == 2)
		s1 = 3*(ctxt->cur_v>>8)+(((ctxt->cur_v&0xff)+((ctxt->cur_v>>1)&0x7f)+0x3c)>>7)-1;
		s2 = 3*(new_v>>8)+(((new_v&0xff)+((new_v>>1)&0x7f)+0x3c)>>7)-1;
#elif (ALIGN == 1)
		s1 = 3*(ctxt->cur_v>>8)+(((ctxt->cur_v&0xff)+((ctxt->cur_v>>1)&0x7f)+0x40)>>7)-1;
		s2 = 3*(new_v>>8)+(((new_v&0xff)+((new_v>>1)&0x7f)+0x40)>>7)-1;
#elif (ALIGN == 0)
		s1 = 3*(ctxt->cur_v>>8)+((ctxt->cur_v>>6)&3)-1;
		s2 = 3*(new_v>>8)+((new_v>>6)&3)-1;
#endif
		/* check at least one serial band is intersected */
		if (s1 != s2) {
			/* get full coordinates in float */
			x1 = ((float)ctxt->cur_h+0.5)*(1.0/256.0);
			y1 = ((float)ctxt->cur_v+0.5)*(1.0/256.0);
			x2 = ((float)new_h+0.5)*(1.0/256.0);		
			y2 = ((float)new_v+0.5)*(1.0/256.0);
			/* calculate the coeff of the linear equation of the line */
			c1 = (x2-x1)/(y2-y1);
			c0 = x1-y1*c1;
			/* set band control */
			if (s1 < s2) {
				min = s1+1;
				max = s2;
				status = -1;
			}
			else {
				min = s2+1;
				max = s1;
				status = 1;
			}
			jband = min/3;
			jsubband = min-3*jband;
			/* record the intersection with each band */
			for (j=min; j<=max; j++) {
				/* calculate coordinate of intersection */
#if (ALIGN == 3)
				y = (float)jband+0.2109+0.2891*(float)jsubband;
#elif (ALIGN == 2)
				y = (float)jband+(17.0/96.0)+(float)(1.0/3.0)*(float)jsubband;
#elif (ALIGN == 1)
				y = (float)jband+0.16667*(float)(1+2*jsubband);
#elif (ALIGN == 0)
				y = (float)jband+0.25*(float)(1+jsubband);
#endif
				x = c0+y*c1;
				/* insert at the good position in the list */
				cnt0 = MAX_INTER_PER_LINE*j;
				cnt = cnt0+ctxt->index[j];
				if (ctxt->index[j] >= MAX_INTER_PER_LINE) {
					ctxt->error = 1;
					return;
				}
				for (pos=cnt0; pos<cnt; pos++)
					if (x < ctxt->inter_h[pos])
						break;
				for (k=cnt; k>pos; k--) {
					ctxt->inter_h[k] = ctxt->inter_h[k-1];
					ctxt->open_stat[k] = ctxt->open_stat[k-1];
				}
				ctxt->inter_h[pos] = x;
				ctxt->open_stat[pos] = status;
				ctxt->index[j]++;
				/* go to next subband, next band */
				jsubband++;
				if (jsubband == 3) {
					jsubband = 0;
					jband++;
				}
			}
		}
	}
	/* memorize the last point */
	ctxt->cur_h = new_h;
	ctxt->cur_v = new_v;
}

/* PART 4 : convert list of intersection into a filling array,
            and calculate the real bounding box */
static void pre_raster(fc_renderer_context *ctxt) {
	int     i, j, k, h, h_prev, status, col0, col;
	char    val;
	char    *fill_line, *stat;
	float   *inter;
	float   x;

	ctxt->s3_min_h = ctxt->size3_h;
	ctxt->s3_min_v = ctxt->size3_v;
	ctxt->s3_max_h = 0;
	ctxt->s3_max_v = 0;
	
	inter = ctxt->inter_h;
	stat = ctxt->open_stat;
	fill_line = ctxt->fill_array;
	for (i=0; i<ctxt->size3_v; i++) {
		col0 = 0;
		if (ctxt->index[i] != 0) {
			if (i < ctxt->s3_min_v)
				ctxt->s3_min_v = i;
			if (i >= ctxt->s3_max_v)
				ctxt->s3_max_v = i+1;
			h = (int)(256.0*inter[0]+0.5);
#if (ALIGN == 3)
			col = 3*(h>>8)+(((h&0xff)+20)/74)-1;
#elif (ALIGN == 2)
			col = 3*(h>>8)+(((h&0xff)+((h>>1)&0x7f)+0x38)>>7)-1;
#elif (ALIGN == 1)
			col = 3*(h>>8)+(((h&0xff)+((h>>1)&0x7f)+0x40)>>7)-1;
#elif (ALIGN == 0)
			col = 3*(h>>8)+((h>>6)&3)-1;
#endif
			if ((col+1) < ctxt->s3_min_h)
				ctxt->s3_min_h = col+1;
			for (;col0<=col;col0++)
				fill_line[col0] = 0;
			status = stat[0];
			for (k=1; k<ctxt->index[i]; k++) {
				h_prev = h;
				h = (int)(256.0*inter[k]+0.5);
				if ((h >= h_prev+32) || status) {
#if (ALIGN == 3)
					col = 3*(h>>8)+(((h&0xff)+20)/74)-1;
#elif (ALIGN == 2)
					col = 3*(h>>8)+(((h&0xff)+((h>>1)&0x7f)+0x38)>>7)-1;
#elif (ALIGN == 1)
					col = 3*(h>>8)+(((h&0xff)+((h>>1)&0x7f)+0x40)>>7)-1;
#elif (ALIGN == 0)
					col = 3*(h>>8)+((h>>6)&3)-1;
#endif
					if ((val == 0) && (col0 > col))
						fill_line[col] = 1;
					if (status != 0)
						val = 1;
					else {
						val = 0;
						if ((col0 > col) && (fill_line[col-1] == 1))
							fill_line[col] = 0;
					}
					for (;col0<=col;col0++)
						fill_line[col0] = val;
				}
				status += stat[k];
			}
			if (col0 > ctxt->s3_max_h)
				ctxt->s3_max_h = col0;
		}
		for (;col0<=ctxt->size3_h;col0++)
			fill_line[col0] = 0;
		inter += MAX_INTER_PER_LINE;
		stat += MAX_INTER_PER_LINE;
		fill_line += MAX_COLUMN * 3;
	}

	/* fit the bounding box to the grid (in /3) */
	ctxt->s3_min_h = (ctxt->s3_min_h/3);
	ctxt->s3_min_v = (ctxt->s3_min_v/3);
	ctxt->s3_max_h = ((ctxt->s3_max_h+2)/3);
	ctxt->s3_max_v = ((ctxt->s3_max_v+2)/3);

	/* update the offset of the bitmap, and the size of the bounding box */
	ctxt->offset_h += ctxt->s3_min_h;
	ctxt->offset_v += ctxt->s3_min_v;
	ctxt->size_h = ctxt->s3_max_h - ctxt->s3_min_h;
	ctxt->size_v = ctxt->s3_max_v - ctxt->s3_min_v;
}

/* PART 4b : debug output */
static void debug_print(fc_renderer_context *ctxt, char *name) {
	int        h, v;

	if (debug) {
		for (v=0; v<ctxt->size_v; v++) {
			for (h=0; h<ctxt->size_h; h++)
				xprintf("---------+");
			xprintf("\n");
			for (h=0; h<ctxt->size_h; h++)
				xprintf("   %2d    |", ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].diff[1]);
			xprintf("\n");
			for (h=0; h<ctxt->size_h; h++)
				xprintf("%2d %2d %2d |",
					   ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].diff[0],
					   ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].total,
					   ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].diff[2]);
			xprintf("\n");
			for (h=0; h<ctxt->size_h; h++)
				xprintf("   %2d  %2x|",
					   ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].diff[3],
					   ctxt->cell_array[(v+1)*MAX_COLUMN2+(h+1)].enable);
			xprintf("\n");
		}
		xprintf("%s\n\n", name);
	}
}

/* PART 5 : convert the super sampled bitmap into a automat entry */
static void post_raster(fc_renderer_context *ctxt) {
	int        h, v;
	int        code;
	char       *fill_line;
	cell       *cell_line;
	diff_entry *de;

	/* translate the super sampled bitmap into a row automat table */
	/* include the border into the automat table */
	for (v=0; v<ctxt->size_v; v++) {
		cell_line = ctxt->cell_array+1+MAX_COLUMN2*(v+1);
		fill_line = ctxt->fill_array+(3*MAX_COLUMN)*3*(ctxt->s3_min_v+v)+(3*ctxt->s3_min_h);
		for (h=0; h<ctxt->size_h; h++) {
			code =
				(fill_line[0]) +
				(fill_line[1]<<1) +
				(fill_line[2]<<2) +
				(fill_line[3*MAX_COLUMN+0]<<3) +
				(fill_line[3*MAX_COLUMN+1]<<4) +
				(fill_line[3*MAX_COLUMN+2]<<5) +
				(fill_line[6*MAX_COLUMN+0]<<6) +
				(fill_line[6*MAX_COLUMN+1]<<7) +
				(fill_line[6*MAX_COLUMN+2]<<8);
			de = t_diff+code;
			cell_line->total = de->total; 
			cell_line->total0 = de->total; 
			cell_line->out = de->total; 
			cell_line->diff[0] = de->diff[0];
			cell_line->diff[1] = de->diff[1];
			cell_line->diff[2] = de->diff[2];
			cell_line->diff[3] = de->diff[3];
			cell_line->enable = de->enable;
			cell_line++;
			fill_line+=3;
		}
	}
	/* init borders status for enable */
	for (v=0;v<ctxt->size_v;v++) {
		cell_line = ctxt->cell_array+(v+1)*MAX_COLUMN2;
		cell_line->enable = 0x80;
		cell_line->total = 0;
		cell_line->total0 = 0;
		cell_line = ctxt->cell_array+(v+1)*MAX_COLUMN2+ctxt->size_h+1;
		cell_line->enable = 0x80;
		cell_line->total = 0;
		cell_line->total0 = 0;
	}
	for (h=0;h<ctxt->size_h;h++) {
		cell_line = ctxt->cell_array+h+1;
		cell_line->enable = 0x80;
		cell_line->total = 0;
		cell_line->total0 = 0;
		cell_line = ctxt->cell_array+(ctxt->size_v+1)*MAX_COLUMN2+h+1;
		cell_line->enable = 0x80;
		cell_line->total = 0;
		cell_line->total0 = 0;
	}
	debug_print(ctxt, "First diffusion");
}

static int offset_cell[4] = { -1, -MAX_COLUMN2, 1, MAX_COLUMN2 };

/* PART 6 : do the diffusion using the automat */
static void automat_diffusion(fc_renderer_context *ctxt) {
	int      i, j, cnt, cnt2, cnt3, h, v, quanta, angle, way, tmp, tmp2;
	int      quotas[4];
	cell     *cell_line, *target, **cur_list, *slave;
	cell_ref *next_ref;
	
	/* init empty diffusion list */
	ctxt->end_list = ctxt->diff_list;
	for (i=0; i<MAX_DIFFUSE; i++)
		ctxt->diff_index[i] = 0L;
	/* find all case that can diffuse and sort them in the radix table */
	for (v=1; v<=ctxt->size_v; v++) {
		cell_line = ctxt->cell_array+v*MAX_COLUMN2+1;
		for (h=0; h<ctxt->size_h; h++) {
			if ((cell_line->enable & 0xC0) == 0) {
				tmp = cell_line->total+(cell_line->total>>1);
				tmp2 = 0;
				for (i=0; i<4; i++)
					if (cell_line->diff[i] > 0) {
						target = cell_line+offset_cell[i];
						if ((target->enable & 0x80) == 0)
							tmp += target->diff[i^2];
						else if (target->enable & 0x20)
							tmp2 ^= 4;
					}
				ctxt->end_list->c = cell_line;
				ctxt->end_list->next = ctxt->diff_index[tmp+tmp2];
				ctxt->diff_index[tmp+tmp2] = ctxt->end_list;
				ctxt->end_list++;
			}
			else if (cell_line->enable & 0x40) {
				ctxt->end_list->c = cell_line;
				ctxt->end_list->next = ctxt->diff_index[0];
				ctxt->diff_index[0] = ctxt->end_list;
				ctxt->end_list++;
			}
			cell_line++;
		}
	}
	/* try to do as much diffusion as possible */
	for (cnt=MAX_DIFFUSE-1; cnt>0; cnt--) {
		ctxt->end_list = ctxt->diff_index[cnt];
		while (ctxt->end_list != 0L) {
			cell_line = ctxt->end_list->c;
			tmp = 0;
			tmp2 = 0;
			for (i=0; i<4; i++)
				if (cell_line->diff[i] > 0) {
					target = cell_line+offset_cell[i];
					if ((target->enable & 0x80) == 0)
						tmp += target->diff[i^2];
					else if (target->enable & 0x20)
						tmp2 ^= 4;
				}
			if (tmp == 0) {
				next_ref = ctxt->end_list->next;
				ctxt->end_list->next = ctxt->diff_index[0];
				ctxt->diff_index[0] = ctxt->end_list;
				ctxt->end_list = next_ref;
			}
			else {
				tmp += cell_line->total+(cell_line->total>>1);
				if (tmp+tmp2 < cnt) {
					next_ref = ctxt->end_list->next;
					ctxt->end_list->next = ctxt->diff_index[tmp+tmp2];
					ctxt->diff_index[tmp+tmp2] = ctxt->end_list;
					ctxt->end_list = next_ref;
				}
				else {
					/* suck power from neighboor */
					if (tmp > 20+(cell_line->total>>1))
						quanta = 20-cell_line->total;
					else
						quanta = tmp-(cell_line->total+(cell_line->total>>1));
					cell_line->total += quanta;
					if (quanta > 0) {
						tmp = 0;
						for (i=0; i<4; i++)
							if (cell_line->diff[i] > tmp) {
								angle = i;
								tmp = cell_line->diff[i];
							}
						if (cell_line->diff[(angle+1)&3] > cell_line->diff[(angle-1)&3])
							way = 1;
						else
							way = -1;
						for (i=0; i<4; i++) {
							target = cell_line+offset_cell[i];
							if ((target->enable & 0x80) == 0)
								quotas[i] = target->diff[i^2];
							else
								quotas[i] = 0; 
						}
						while (quanta > 0) {
							if (quotas[angle] > 0) {
								quanta--;
								quotas[angle]--;
							}
							angle = (angle+way)&3;
						}
						for (i=0; i<4; i++) {
							target = cell_line+offset_cell[i];
							if ((target->enable & 0x80) == 0)
								target->total += quotas[i]-target->diff[i^2];
						}
					}
					cell_line->enable |= 0x80;
					if (cell_line->total < 20) {
						next_ref = ctxt->end_list->next;
						ctxt->end_list->next = ctxt->diff_index[0];
						ctxt->diff_index[0] = ctxt->end_list;
						ctxt->end_list = next_ref;
					}
					else {
						cell_line->out = 20;
						ctxt->end_list = ctxt->end_list->next;
					}
				}
			}
		}
	}
	debug_print(ctxt, "End diffusion");

	/* process all the residu to decide what to do with them */
	ctxt->end_list = ctxt->diff_index[0];
	while (ctxt->end_list != 0L) {
		cell_line = ctxt->end_list->c;
		tmp = 0;
		cnt = 0;
		cnt2 = 0;
		cnt3 = 0;
		for (i=0; i<4; i++) {
			target = cell_line+offset_cell[i];
			if (cell_line->diff[i] == 0) {
				if (target->total > 0) {
					if (target->total == 20) {
						cnt2++;
						tmp += 8;
					}
					else if (target->diff[i^2])
						tmp += 6;
					else
						tmp += 4;
				}
				else if (target->total0 == 0)
					cnt3++;
				cnt++;
			}
			else {
				if (target->total == 20) {
					cnt2++;
					tmp++;
				}
				else {
					if ((target->total0 < cell_line->total0) &&
						(cell_line->diff[i] > 1))
						tmp -= 16;
					if (target->total0 == 0)
						cnt3++;
				}
			}
		}
		if (cnt3 >= 3)
			tmp -= 16;
		if (cell_line->enable & 0x40)
			cell_line->out = cell_line->total >> 2;
		else if (cnt == 0) {
			if (cnt2 == 4)
				cell_line->out = cell_line->total >> 3;
			else if (cnt2 == 3)
				cell_line->out = cell_line->total >> 1;
			else
				cell_line->out = cell_line->total;
		}
		else {
			if (tmp > 7)
				cell_line->out = cell_line->total >> 2;
			else if (tmp > 3)
				cell_line->out = cell_line->total >> 1;
			else if (tmp > 1)
				cell_line->out = cell_line->total-(cell_line->total>>2);
			else if (tmp > 0)
				cell_line->out = cell_line->total-(cell_line->total>>3);
			else if (tmp < 0) {
				cell_line->out = cell_line->total << 1;
				if (cell_line->out > 20) 
					cell_line->out = 20;
			}
			else
				cell_line->out = (cell_line->total+cell_line->total0+1)>>1;
		}
		ctxt->end_list = ctxt->end_list->next;
	}

	debug_print(ctxt, "End");
}

static char convert[21] = {
/*	0x3f, 0x1b, 0x1b, 0x1b, 0x17, 0x17, 0x17, 0x12,
 	0x12, 0x12, 0x0d, 0x0d, 0x0d, 0x09, 0x09, 0x09,
	0x04, 0x04, 0x04, 0x00, 0x00 */
	0, 1, 1, 1, 2, 2, 2, 3,
	3, 3, 4, 4, 4, 5, 5, 5,
	6, 6, 6, 7, 7
};

/* PART 6b : record the result in the buffer */
static void draw(fc_renderer_context *ctxt) {
	int         i, j, bitmap_size;
	cell        *cell_line;
	char        *ptr;
	uchar       *bitmap;
	fc_rect     *bound;
	fc_escape   *escape;

	/* allocate space for header and bitmap (except edges).
	   reserved space for spacing parameters. */
	if ((ctxt->size_h <= 0) || (ctxt->size_v <= 0))
		ctxt->size_v = 0;
	bitmap_size = ((ctxt->size_h+1)>>1)*ctxt->size_v;
	ptr = (char*)(ctxt->fc_get_mem)(ctxt->set_ptr,
									sizeof(fc_rect)+sizeof(fc_escape)+
									sizeof(fc_spacing)+bitmap_size);
	/* record character bounding box */
	bound = (fc_rect*)ptr;
	if (ctxt->size_v == 0) {
		bound->left = 0;
		bound->top = 0;
		bound->right = -1;
		bound->bottom = -1;
	}
	else {
		bound->left = ctxt->offset_h;
		bound->top = ctxt->offset_v;
		bound->right = ctxt->offset_h+ctxt->size_h-1;
		bound->bottom = ctxt->offset_v+ctxt->size_v-1;
	}
	
	/* record escapement infos */
	escape = (fc_escape*)(ptr+sizeof(fc_rect));
	escape->x_escape = (float)ctxt->escape_h*(1.0/256.0);
	escape->y_escape = -(float)ctxt->escape_v*(1.0/256.0);
	/* record bitmap in gray_scale encoding */
	if (bitmap_size > 0) {
		bitmap = (uchar*)(ptr+sizeof(fc_rect)+sizeof(fc_escape)+sizeof(fc_spacing));
		for (j=0; j<ctxt->size_v; j++) {
			cell_line = ctxt->cell_array+1+(j+1)*MAX_COLUMN2;
			for (i=0; i<ctxt->size_h; i+=2) {
				if (i == (ctxt->size_h-1))
					*bitmap++ = convert[cell_line[0].out]<<4;
				else {
					*bitmap++ = (convert[cell_line[0].out]<<4) + convert[cell_line[1].out];
					cell_line += 2;
				}
			}
		}
	}
}

/* PART 7 : global character conversion control */
static void raster(fc_renderer_context *ctxt) {
	if (ctxt->error) return;

	pre_raster(ctxt);
	post_raster(ctxt);
	automat_diffusion(ctxt);
	draw(ctxt);
}

/* the shape 3*3 is encoded like that :
   0 1 2
   3 4 5
   6 7 8
   Directions are encoded like that :
     1
   0 * 2
     3
*/
/* coefficient of first diffusion for each pixel, in each direction */
static char diffusion0[4*9] = {
	1, 1, 0, 0,
	0, 2, 0, 0,
	0, 1, 1, 0,
	2, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 2, 0,
	1, 0, 0, 1,
	0, 0, 0, 2,
	0, 0, 1, 1
};

/* index of neighboor pixels for each pixels (max 4), direct connect */
static char voisins[4*9] = {
	1, 3, -1, -1,
	0, 2, 4, -1,
	1, 5, -1, -1,
	0, 4, 6, -1,
	1, 3, 5, 7,
	2, 4, 8, -1,
	3, 7, -1, -1,
	4, 6, 8, -1,
	5, 7, -1, -1
};
/* index of neighboor pixels for each pixels (max 4), excluding
   the center, including border connection */
static char voisins8[4*9] = {
	 1,  3, -1, -1,
	 0,  2,  3,  5,
	 1,  5, -1, -1,
	 0,  1,  6,  7,
	-1, -1, -1, -1,
	 1,  2,  7,  8,
	 3,  7, -1, -1,
	 3,  5,  6,  8,
     5,  7, -1, -1,
};
/* start index in order of exploration */
static start_index[8] = { 0, 2, 6, 8, 1, 3, 5, 7 };
/* rotation index around the center */
static rotation_index[9] = { 0, 1, 2, 5, 8, 7, 6, 3, 0 };

/* PART 8 : initialise the automat table for diffusion */
void fc_init_diffusion_table() {
	int      i, j, k, m, tmp, index;
	int      start, next, cur;
	int      first_fil, vois_cnt, fil_cnt, line_cnt;
	int      cnt, comp_cnt, cut;
	int      diffuse, accept_diffuse, filament;
	char     f_diff[4];
	char     s_diff[4];
	char     shape[9];

	/* do it once */
	if (diffusion_init)
		return;
	diffusion_init = 1;

	for (i=0; i<512; i++) {
		/* create the shape in bitmap, and count its pixels */
		tmp = i;
		cnt = 0;
		for (j=0; j<9; j++) {
			shape[j] = (tmp & 1);
			cnt += shape[j];
			tmp >>= 1;
		}
		cnt += shape[4];
		/* do the first diffusion evaluation */
		for (k=0; k<4; k++)
			f_diff[k] = 0;
		for (j=0; j<9; j++)
			if (shape[j])
				for (k=0; k<4; k++)
					f_diff[k] += diffusion0[j*4+k];
		/* count the number of connexe component, check for filament */
		filament = 0;
		if (shape[4] == 1) {
			comp_cnt = 1;
			/* check for filament */
			line_cnt = 0;
			for (j=0; j<8; j++)
				if (shape[rotation_index[j]] != shape[rotation_index[j+1]])
					line_cnt++;
			if ((line_cnt == 4) && (cnt <= 5))
				filament = 1;
		}
		else {
			comp_cnt = 0;
			for (j=0; j<9; j++) {
				if (shape[j] == 1) {
					comp_cnt += 1;
					shape[j] = comp_cnt+1;
					for (tmp=j+1; tmp<9; tmp++) {
						for (k=j; k<9; k++)
							if (shape[k] == comp_cnt+1)
								for (m=0; m<4; m++) {
									index = voisins[k*4+m];
									if (index == -1)
										break;
									if (shape[index] == 1)
										shape[index] = comp_cnt+1;
								}
					}
				}
			}
			/* check a potential filament */
			if (comp_cnt > 1) {
				/* clean the shape again */
				for (j=0; j<9; j++)
					if (shape[j] > 1)
						shape[j] = 1;
				/* detection for filament not using the central point */
				first_fil = 1;
				for (j=0; j<8; j++) {
					start = start_index[j];
					if (shape[start] == 1) {
						shape[start] = 2;
						if (!first_fil)
							goto no_filament;
						else
							first_fil = 0;
						fil_cnt = 1;
						while (TRUE) {
							vois_cnt = 0;
							for (k=0; k<8; k++) {
								cur = voisins8[k+4*start];
								if (cur == -1)
									break;
								if (shape[cur] == 1) {
									vois_cnt++;
									if (vois_cnt > 1)
										goto no_filament;
									next = cur;
									shape[cur] = 2;
								}
							}
							if (vois_cnt > 1)
								goto no_filament;
							else if (vois_cnt == 1)
								fil_cnt++;
							else if ((fil_cnt < 2) || (fil_cnt > 3))
								goto no_filament;
							else
								break;
							start = next;
						}
					}
				}
				filament = 1;
			no_filament:;
			}
		}
		if (filament) {
			cnt += 2;
			comp_cnt = 1;
		}
		/* sort first diffusion */
		for (j=0; j<4; j++)
			s_diff[j] = f_diff[j];
		for (j=0; j<3; j++) {
			tmp = j;
			for (k=j+1; k<4; k++)
				if (s_diff[k] > s_diff[tmp])
					tmp = k;
			k = s_diff[j];
			s_diff[j] = s_diff[tmp];
			s_diff[tmp] = k;
		}
		/* freeze the diffusion property */
		if (((cnt >= SEUIL) && (comp_cnt == 1)) || filament)
			for (j=0; j<4; j++)
				f_diff[j] = 0;
		/* set the "will accept diffusion" property */
		if ((cnt == 0) ||
			(comp_cnt > 1) ||
			((shape[4] == 0) && (cnt == 7)))
			accept_diffuse = 0;
		else
			accept_diffuse = 1;
		/* record the values */
		if (cnt >= SEUIL)
			t_diff[i].total = 20;
		else
			t_diff[i].total = 2*cnt;
		for (j=0; j<4; j++) {
			if (f_diff[j] > 1)
				t_diff[i].diff[j] = f_diff[j]-1;
			else
				t_diff[i].diff[j] = f_diff[j];
		}
		tmp = 0;
		if (!accept_diffuse)
			tmp |= 0x40;
		if (f_diff[0]+f_diff[1]+f_diff[2]+f_diff[3] == 0)
			tmp |= 0x80;
		if (cnt >= SEUIL)
			tmp |= 0x21;
		t_diff[i].enable = tmp;
	}
}

/******************************************************************/

FUNCTION char sp_init_userout(SPEEDO_GLOBALS *sp_global_ptr, specs_t *specs) {
	if (sp_global_ptr->output_mode != MODE_USER)
		return FALSE;
	sp_global_ptr->init_out = init_out_user;
	sp_global_ptr->begin_char = begin_char_user;
	sp_global_ptr->begin_sub_char = begin_sub_char_user;
	sp_global_ptr->begin_contour = begin_contour_user;
	sp_global_ptr->curve = curve_user;
	sp_global_ptr->line = line_user;
	sp_global_ptr->end_contour = end_contour_user;
	sp_global_ptr->end_sub_char = end_sub_char_user;
	sp_global_ptr->end_char = end_char_user;
	return TRUE;
}

FUNCTION char init_out_user(SPEEDO_GLOBALS *sp_global_ptr, specs_t *specs) {
	return TRUE;
}

FUNCTION char begin_char_user(SPEEDO_GLOBALS *sp_global_ptr, point_t p1, point_t p2, point_t p3) {
	reset((fc_renderer_context*)sp_global_ptr->UserPtr,
		  p2.x, p2.y, p3.x, p3.y, p1.x, p1.y);
	return TRUE;
}

FUNCTION void begin_sub_char_user(SPEEDO_GLOBALS *sp_global_ptr, point_t p1, point_t p2, point_t p3) {
}

FUNCTION void begin_contour_user(SPEEDO_GLOBALS *sp_global_ptr, point_t p1, char outside) {
	from((fc_renderer_context*)sp_global_ptr->UserPtr, p1.x, p1.y);
}

FUNCTION void curve_user(SPEEDO_GLOBALS *sp_global_ptr, point_t p1, point_t p2, point_t p3, fix15 depth) {
	moveto((fc_renderer_context*)sp_global_ptr->UserPtr, p1.x, p1.y);
	moveto((fc_renderer_context*)sp_global_ptr->UserPtr, p2.x, p2.y);
	moveto((fc_renderer_context*)sp_global_ptr->UserPtr, p3.x, p3.y);
}

FUNCTION void line_user(SPEEDO_GLOBALS *sp_global_ptr, point_t p1) {
	moveto((fc_renderer_context*)sp_global_ptr->UserPtr, p1.x, p1.y);
}

FUNCTION void end_contour_user(SPEEDO_GLOBALS *sp_global_ptr) {
}

FUNCTION void end_sub_char_user(SPEEDO_GLOBALS *sp_global_ptr) {
}

FUNCTION char end_char_user(SPEEDO_GLOBALS *sp_global_ptr) {
	raster((fc_renderer_context*)sp_global_ptr->UserPtr);
	return TRUE;
}

