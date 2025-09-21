/* ++++++++++
	FILE:	font_renderer.cpp
	REVS:	$Revision: 1.5 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:31:46 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <OS.h>

#include "font_renderer.h"
#include "font_data.h"
#include "font_cache.h"
#include "as_support.h"
#include "system.h"

#include <support2/Debug.h>

using namespace B::Support2;

void fc_font_processor::process_spacing(int32 /*bpp*/, fc_char *my_char, fc_set_entry *) {
	my_char->spacing._reserved_ = 0;
	my_char->spacing.char_offset = 0;
	my_char->spacing.offset = 0;
	
	// This old code tried to adjust the character spacing to look better on
	// the screen, but often failed horribly at the task.  At this point it
	// looks like just taking the spacing information given to us by FontFusion
	// is by far the best approach.
#if 0
	int32		i, j, width, row, v_left, v_right, auto_kern, escape, delta;
	int32		left[4], right[4], limit[5];
	uint8		*buf;
	float		fdelta;	
	uint32		step;

	/* calculate bitmap width */
	width = my_char->bbox.right - my_char->bbox.left + 1;
	row = (width+1)>>1;

	if (width > 0) {
		/* init band tables */
		for (i=0; i<4; i++)
			left[i] = right[i] = 3;
		limit[0] = 100000;
		limit[1] = 0;
		limit[2] = se->limit[0];
		limit[3] = se->limit[1];
		limit[4] = -100000;
		
		/* get a pointer on the grayscale bitmap or the RLE black and white */
		buf = ((uint8*)my_char)+CHAR_HEADER_SIZE;
		/* scan bitmap line per line */
		for (j=my_char->bbox.top; j<=my_char->bbox.bottom; j++) {
			/* scan a grayscale line */
			if (bpp == FC_GRAY_SCALE || bpp == FC_TV_SCALE) {
				/* scan for available space on left side */
				if ((buf[0]&0x66) == 0x00) {
					if ((buf[1] & 0x60) == 0)    v_left = 3;
					else                         v_left = 2;
				} else if ((buf[0] & 0x60) == 0) v_left = 1;
				else                             v_left = 0;
				if (v_left > width) v_left = width;
				/* move the pointer to the next line */
				buf += row;
				/* scan for space available on right side */
				if (width&1) {
					if ((buf[-1]&0x66) == 0x00) {
						if ((buf[-2]&0x66) == 0x00)     v_right = 3;
						else if ((buf[-2] & 0x06) == 0) v_right = 2;
						else                            v_right = 1;
					} else                              v_right = 0;
				}
				else {
					if ((buf[-1]&0x66) == 0x00) {
						if ((buf[-2] & 0x06) == 0)    v_right = 3;
						else                          v_right = 2;
					} else if ((buf[-1] & 0x06) == 0) v_right = 1;
					else                              v_right = 0;
				}
				if (v_right > width) v_right = width;
			}
			/* scan an RLE black and white line */
			else {
				v_left = buf[0];
				if (v_left > width)
					v_left = width;
				v_right = width;
				while ((step = *buf++) != 0xff)
					v_right -= step;
			}
			/* clip and register that in band table */
			for (i=0; i<4; i++)
				if ((j<=limit[i]) && (j>=limit[i+1])) {
					if (v_left  < left[i] ) left[i]  = v_left;
					if (v_right < right[i]) right[i] = v_right;
				}
		}
		/* calculate bitmap escapement */
		auto_kern = left[0]+right[0];
		for (i=1; i<4; i++)
			if (auto_kern > (left[i]+right[i]))
				auto_kern = (left[i]+right[i]);
		if (auto_kern > (1-my_char->bbox.left))
			auto_kern = 1-my_char->bbox.left;
		width = my_char->bbox.right-my_char->bbox.left+2;
		escape = (int)my_char->escape.x_escape;
		if ((width-auto_kern) > escape)
			escape = width-auto_kern;
		my_char->spacing.escape = escape;
		/* calculate bitmap offset */
		delta = width-(int)my_char->escape.x_escape;
		if (delta < 0)
			my_char->spacing.offset = 0;
		else if (auto_kern == 0)
			my_char->spacing.offset = my_char->bbox.left;
		else if (delta <= auto_kern)
			my_char->spacing.offset = my_char->bbox.left+delta/2;
		else {
			if (left[1] < left[2])
				v_left = left[1];
			else
				v_left = left[2];
			if (right[1] < right[2])
				v_right = right[1];
			else
				v_right = right[2];
			if (v_right == 0)
				delta = auto_kern;
			else if (v_left == 0)
				delta = 0;
			else if (v_right < v_left)
				delta = (auto_kern+1)/2;
			else
				delta = auto_kern/2;
			my_char->spacing.offset = my_char->bbox.left+delta;
		}
		/* for CHAR_SPACING, guess an optimisation offset for rounding */
		my_char->spacing.char_offset = 
			(int8)(-16.0*((float)my_char->spacing.offset+0.5*((float)escape-my_char->escape.x_escape)));
		/* default for now */
		my_char->spacing._reserved_ = 0;
	}
	else {
		/* default NULL value */
		my_char->spacing._reserved_ = 0;
		my_char->spacing.char_offset = 0;
		/* by default, the bitmap escapement is the round of the real escapement */
		my_char->spacing.escape = (int)(my_char->escape.x_escape+0.5);
		/* by default, the offset is 0 */
		my_char->spacing.offset = 0;
	}
#endif
}

fc_font_renderer::fc_font_renderer(fc_context *context) {
	int64              max, val;
	bigtime_t          curtime;
	int                i, imax;
	char               fullname[PATH_MAX];

	ctxt = context;
	/* safety mode, for non existing font files */
	if (context->file_id == 0xffff)
		goto abort_invalid_f_id;

	/* lock the renderer */
	fc_lock_render();
	fc_lock_open_file();
	
	/* check for a renderer that is already set to this file */
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++) {
		if ((r_ctxt_list[i].file_id == context->file_id) &&
			(r_ctxt_list[i].used == 0))
			goto found_renderer;
	}
	/* well then, check for empty or oldest entry */
	max = 0;
	imax = -1;
	curtime = system_time();
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++) {
		/* If the renderer is not initialized, just use it without question */
		if (r_ctxt_list[i].file_id < 0)
			goto found_renderer;
		
		/* Compute relative weight of renderer, to determine which to flush */
		if (r_ctxt_list[i].used == 0) {
			val = fc_context_weight(r_ctxt_list[i].renderer);
			if (val <= 0)
				val = 1;
			val = ( ((curtime-r_ctxt_list[i].last_used)*100000) / val ) - val;
			//fprintf(stderr, "Checking renderer #%d: token=0x%x, val=%Ld, max=%Ld\n",
			//		i, r_ctxt_list[i].file_id, val, max);
			if (imax < 0 || val > max) {
				max = val;
				imax = i;
			}
		}
	}
	if (imax >= 0) {
		i = imax;
		goto found_renderer;
	}
	
	goto abort_no_r_ctxt;

found_renderer:;
	/* at this point, "i" is the index of the renderer; set it up */
	//fprintf(stderr, "Using renderer #%d (token: old=0x%x, new=0x%x)\n",
	//		i, r_ctxt_list[i].file_id, context->file_id);
	r_ctxt = r_ctxt_list[i].renderer;
	r_ctxt_list[i].used = 1;
	r_ctxt_list[i].file_id = context->file_id;
	
	/* check if the file is already open */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++) {
		if ((file_open_list[i].file_id == context->file_id) &&
			(file_open_list[i].used == 0)) {
			open_file_index = i;
			goto file_open;			
		}
	}
	/* if not, check for an empty entry */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++) {
		if ((file_open_list[i].file_id == -1) &&
			(file_open_list[i].used == 0)) {
			open_file_index = i;
			goto open_file;
		}
	}
	/* if not, check for the oldest unused entry */
	max = -1000000;
	imax = -1;
	for (i=0; i<FC_FILE_OPEN_COUNT; i++) {
		if (file_open_list[i].used == 0) {
			val = set_hit - file_open_list[i].hit_ref;
			if (val > max) {
				max = val;
				imax = i;
			}
		}
	}
	
	if (imax == -1)
		goto abort_r_ctxt;
	
	open_file_index = imax;
	fc_close_and_flush_file(file_open_list[open_file_index].fp);
	if (file_open_list[open_file_index].fp_tuned)
		fclose(file_open_list[open_file_index].fp_tuned);
	file_open_list[open_file_index].tuned_file_id = -1;
	file_open_list[open_file_index].fp_tuned = 0L;
 open_file:
	fc_lock_file();
	if (context->file_id < file_table_count)
		fc_get_pathname(fullname, file_table+context->file_id);
	else
		fullname[0] = 0;
	fc_unlock_file();
	file_open_list[open_file_index].fp = fopen(fullname, "rb");
	if (file_open_list[open_file_index].fp == NULL)
		goto abort_r_ctxt;
	file_open_list[open_file_index].file_id = context->file_id;
 file_open:
	file_open_list[open_file_index].used = 1;
	/* try to open the tuned file if necessary */
	if (context->tuned_file_id == 0xffff) {
		if (file_open_list[open_file_index].fp_tuned)
			fclose(file_open_list[open_file_index].fp_tuned);
		file_open_list[open_file_index].tuned_file_id = -1;
		file_open_list[open_file_index].fp_tuned = 0L;
	}
	else if (file_open_list[open_file_index].tuned_file_id != context->tuned_file_id) {
		fc_lock_file();
		if (context->tuned_file_id < file_table_count) {
			if (file_open_list[open_file_index].fp_tuned != 0L)
				fclose(file_open_list[open_file_index].fp_tuned);
			fc_get_pathname(fullname, file_table+context->tuned_file_id);
			file_open_list[open_file_index].tuned_file_id = context->tuned_file_id;
			file_open_list[open_file_index].fp_tuned = fopen(fullname, "rb");
			if (file_open_list[open_file_index].fp_tuned == NULL)
				file_open_list[open_file_index].tuned_file_id = -1;
		}
		fc_unlock_file();
	}
	/* register the access, set bitstream file link */	
	file_open_list[open_file_index].hit_ref = set_hit;
	fc_set_file(r_ctxt, file_open_list[open_file_index].fp,
				file_open_list[open_file_index].file_id);
	fp_tuned = file_open_list[open_file_index].fp_tuned;
	if (fp_tuned != 0L) {
		hcache_offset = -32;
		tuned_offset = context->tuned_offset; 
		hmask = context->tuned_hmask;
	}
	fc_unlock_open_file();
	safety = 0;
	return;
	
abort_r_ctxt:
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++)
		if (r_ctxt == r_ctxt_list[i].renderer) {
			r_ctxt_list[i].used = 0;
			break;
		}
	fc_release_font(r_ctxt);	
abort_no_r_ctxt:
	fc_unlock_open_file();
	fc_unlock_render();
abort_invalid_f_id:
	safety = 1;
	return;
}

fc_font_renderer::~fc_font_renderer() {
	int       i;

	/* revert special mode glue */
	if (safety&1) return;
	
	/* atomic operation */
	fc_lock_open_file();
	file_open_list[open_file_index].used = 0;
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++)
		if (r_ctxt == r_ctxt_list[i].renderer) {
			r_ctxt_list[i].last_used = system_time();
			r_ctxt_list[i].used = 0;
			break;
		}
	fc_release_font(r_ctxt);	
	fc_unlock_open_file();
			
	fc_unlock_render();
}

int fc_font_renderer::reset(int special_mode) {
	/* nothing to do in safety mode */
	if (safety & 1)
		return true;
	/* special mode glue */
	if (special_mode) {
		if (special_mode == FC_RENDER_BBOX) {
			ctxt->params()->_size = 250;
			ctxt->set_matrix();
		}
		safety |= 2;
	}
	return set_specs(0);
}

int	fc_font_renderer::first_time_init(fc_file_entry *) {
	if (!reset())
		return false;
	return true;
}

char *fc_get_mem(fc_set_entry *se, int size) {
	int             step, diff;
	char            *retval;
	fc_cache_status *cs;
	
	diff = se->char_buf_size - se->char_buf_end;
	if (size > diff) {
		step = se->char_buf_step*(1+((size-diff)/se->char_buf_step));
		se->char_buf_size += step;
		cs = se->params.cache_status();
		se->char_buf = (char*)grRealloc(se->char_buf, se->char_buf_size,"fc:char_buf",MALLOC_CANNOT_FAIL);
		atomic_add(&cs->cur_mem, step);
		if (se->char_buf_step < 4096)
			se->char_buf_step *= 2;
	}
	retval = se->char_buf+se->char_buf_end;
	se->char_buf_end += size;
	return retval;
}

int fc_font_renderer::render_char(const ushort *code, fc_set_entry *se) {
	int      i, offset, cpt;
    int      code0, code1, index;
    int32    char_offset;
	
	/* dont render char in safety mode */
	if (safety)
		return -2;

	offset = se->char_buf_end;
	/* check tuned font file */
	if ((fp_tuned != 0L) && (code[0] != 0)) {
		code0 = code[0];
		if ((code0 & 0xfc00) == 0xd800)
			code1 = code[1];
		else
			code1 = 0;
		index = (((code0<<3)^(code0>>2))+code1)&hmask;
		for (cpt=0; cpt<100000; cpt++) {
			if ((index < hcache_offset) || (index >= (hcache_offset+32))) {
				hcache_offset = index;
				if ((hcache_offset+32) > (int32)hmask)
					hcache_offset = hmask-31;
				i = fseek(fp_tuned, tuned_offset+sizeof(fc_hset_item)*hcache_offset, SEEK_SET);
				if (fread(hcache, sizeof(fc_hset_item), 32, fp_tuned) < 32)
					goto file_corrupted;
			}
			char_offset = fc_read32(&hcache[index-hcache_offset].offset);
			if (char_offset <= 0)
				return -2;
			if ((fc_read16(&hcache[index-hcache_offset].code[0]) == code0) &&
				(fc_read16(&hcache[index-hcache_offset].code[1]) == code1)) {
				if (fseek(fp_tuned, char_offset, SEEK_SET) < 0)
					goto file_corrupted;
				if (!fc_read_char_from_file(fp_tuned, se, (FC_GET_MEM)fc_get_mem))
					goto file_corrupted;
				goto char_found;
			}
			index = (index+1)&hmask;
		}
		xprintf("######## Dead loop:\n");
		xprintf("hmask: %ld\n", hmask);
		xprintf("code: %04x-%04x\n", code0, code1);
		xprintf("context: (%d,%d,%d), %p\n",
				ctxt->family_id(), ctxt->style_id(), ctxt->size(), ctxt);
	file_corrupted:
		/* should change the status of the file in the font file manager list */
		fp_tuned = 0L;
	}

//char_not_found:
	/* normal mode */
	if (!fc_make_char(r_ctxt,
					  (void*)code,
					  (void*)se,
					  (FC_GET_MEM)fc_get_mem)) {
		se->char_buf_end = offset;
		return -2;
	}
	/* check that a grayscale character's bounding box is really minimal */
	if (se->params.bits_per_pixel() == FC_GRAY_SCALE ||
			se->params.bits_per_pixel() == FC_TV_SCALE) {
		bool			touched;
		uchar			*buf, *from;
		uchar			right_mask;
		int32			h, v, row, row4, size_h, size_v;
		int32			dvmin, dvmax, dhmin, dhmax, from_dv, from_dh, from_dh4;
		fc_char			*my_char;
		
		/* get bitmap specific parameters */
		my_char = (fc_char*)(se->char_buf+offset);
		size_v = my_char->bbox.bottom - my_char->bbox.top + 1;
		size_h = my_char->bbox.right - my_char->bbox.left + 1;
		if ((size_v == 0) || (size_h == 0))
			goto char_found;
		row = (size_h+1)>>1;
		row4 = row & 0xfffc;
		/* look for empty lines at the end */
		buf = ((uchar*)my_char)+CHAR_HEADER_SIZE;
		buf += row*(size_v-1);
		for (v=0; v<size_v; v++) {
			for (h=0; h<row4; h+=4)
				if (*((uint32*)(buf+h)) != 0)
					goto found_bottom_line;
			for (h=row4; h<row; h++)
				if (buf[h] != 0)
					goto found_bottom_line;
			buf -= row;
		}
		/* this is an empty char */
		my_char->bbox.top = 0;
		my_char->bbox.left = 0;
		my_char->bbox.right = -1;
		my_char->bbox.bottom = -1;
		se->char_buf_end = offset+CHAR_HEADER_SIZE;
		goto char_found;
found_bottom_line:
		dvmax = v;
		/* look for empty lines at the beginning */
		buf = ((uchar*)my_char)+CHAR_HEADER_SIZE;
		for (v=0; v<size_v; v++) {
			for (h=0; h<row4; h+=4)
				if (*((uint32*)(buf+h)) != 0)
					goto found_top_line;
			for (h=row4; h<row; h++)
				if (buf[h] != 0)
					goto found_top_line;
			buf += row;
		}
found_top_line:
		dvmin = v;
		/* look for empty columns on the left side */
		touched = false;
		for (h=0; h<size_h; h++) {
			buf = ((uchar*)my_char)+CHAR_HEADER_SIZE;
			buf += row*dvmin + h;
			for (v=dvmin; v<size_v-dvmax; v++) {
				if (buf[0] != 0) {
					if (buf[0] > 0x0f) {
						dhmin = h*2;
						goto found_left_border;
					}
					else touched = true;
				}
				buf += row;
			}
			if (touched) {
				dhmin = h*2+1;
				goto found_left_border;
			}
		}
		dhmin = 0;
found_left_border:
		/* look for empty columns on the left side */
		touched = false;
		for (h=row-1; h>=0; h--) {
			buf = ((uchar*)my_char)+CHAR_HEADER_SIZE;
			buf += row*dvmin + h;
			for (v=dvmin; v<size_v-dvmax; v++) {
				if (buf[0] != 0) {
					if ((buf[0] & 0x0f) != 0) {
						dhmax = size_h-(2*h+2);
						goto found_right_border;
					}
					else touched = true;
				}
				buf += row;
			}
			if (touched) {
				dhmax = size_h-(2*h+1);
				goto found_right_border;
			}
		}
		dhmax = 0;
found_right_border:
		/* do a buffer reduction if the size change */
		if (((dvmin+dvmax+dhmin) > 0) || (dhmax > ((size_h+1)&1))) {
			buf = ((uchar*)my_char)+CHAR_HEADER_SIZE;
			from = buf + row*dvmin + (dhmin>>1);
			from_dh = (size_h+1-(dhmin+dhmax))>>1;
			from_dh4 = (from_dh-1) & 0xfffc;
			from_dv = size_v-(dvmin+dvmax);
			if ((size_h^dhmin^dhmax) & 1)
				right_mask = 0xf0;
			else
				right_mask = 0xff;
			/* need to recode nibbles */
			if (dhmin & 1) {
				if (right_mask == 0xff)
					for (v=0; v<from_dv; v++) {
						for (h=0; h<from_dh; h++)
							buf[h] = (from[h]<<4)|(from[h+1]>>4);
						buf += from_dh;
						from += row;
					}
				else
					for (v=0; v<from_dv; v++) {
						for (h=0; h<from_dh-1; h++)
							buf[h] = (from[h]<<4)|(from[h+1]>>4);
						buf[h] = from[h]<<4;
						buf += from_dh;
						from += row;
					}
			}
			/* doesn't need to recode nibbles, neither change the rowbyte */
			else if (from_dh == row) {
				memmove(buf, from, row*from_dv);
				if (right_mask == 0xf0)
					for (v=from_dh-1; v<from_dv*row; v+=row)
						buf[v] &= 0xf0;
			}
			/* doesn't need to recode nibbles, but need to change the rowbyte */
			else {
				for (v=0; v<from_dv; v++) {
					for (h=0; h<from_dh4; h+=4)
						*((uint32*)(buf+h)) = *((uint32*)(from+h));
					for (; h<from_dh-1; h++)
						buf[h] = from[h];
					buf[h] = from[h] & right_mask;
					buf += from_dh;
					from += row;
				}
			}
			/* set the new set entry buffer end */
			se->char_buf_end = offset+CHAR_HEADER_SIZE+from_dh*from_dv;
		}
		/* adjust the bounding box */
		my_char->bbox.top += dvmin;
		my_char->bbox.left += dhmin;
		my_char->bbox.right -= dhmax;
		my_char->bbox.bottom -= dvmax;
	}
	
char_found:
	/* make sure that the escapement of the default character is defined */
	if (se->flags & FC_MONOSPACED_FONT)
		if (se->offset_default != -1) {
			((fc_char*)(se->char_buf+se->offset_default))->escape.x_escape = 
				((fc_char*)(se->char_buf+offset))->escape.x_escape;
			((fc_char*)(se->char_buf+se->offset_default))->escape.y_escape =
				((fc_char*)(se->char_buf+offset))->escape.y_escape;
			((fc_char*)(se->char_buf+se->offset_default))->escape.x_bm_escape = 
				((fc_char*)(se->char_buf+offset))->escape.x_bm_escape;
			((fc_char*)(se->char_buf+se->offset_default))->escape.y_bm_escape =
				((fc_char*)(se->char_buf+offset))->escape.y_bm_escape;
		}
	/* realign the buffer offset modulo 4 */
	se->char_buf_end = (se->char_buf_end+3)&0xfffffffc;
	return offset;
}

int fc_font_renderer::render_default_char(fc_set_entry *se) {
	int         retval;
	ushort      code[2];

	/* only rendering case for safety mode */
	if (safety) {
	render_safety:
		retval = se->char_buf_end;
		fc_make_safety_char(r_ctxt,
							se->params.size(),
							se->params.bits_per_pixel(),
							(void*)se,
							(FC_GET_MEM)fc_get_mem);
		se->char_buf_end = (se->char_buf_end+3)&0xfffffffc;
		return retval;
	}
	/* normal processing */
	if (set_specs(1) == false)
		goto render_safety;
	code[0] = code[1] = 0;
	retval = render_char(code, se);
	if (retval == -2)
		goto render_safety;
	((fc_char*)(se->char_buf+retval))->edge.right = 0.0;
	((fc_char*)(se->char_buf+retval))->edge.left = 0.0;
	if (set_specs(0) == false)
		goto render_safety;
	if (se->flags & FC_MONOSPACED_FONT) {
		((fc_char*)(se->char_buf+retval))->escape.x_escape = 
			((fc_char*)se->char_buf)->escape.x_escape;
		((fc_char*)(se->char_buf+retval))->escape.y_escape =
			((fc_char*)se->char_buf)->escape.y_escape;
	}
	return retval;
}

SPath *fc_font_renderer::get_char_outline(const ushort *code) {
	SPath		*glyph_path;

	/* only rendering case for safety mode */
	if (safety)
		return NULL;
	/* normal processing */
	if (!fc_make_outline(r_ctxt, (void*)code, (void**)&glyph_path))
		return NULL;
	return glyph_path;
}

void fc_font_renderer::process_edges(const ushort *code, fc_char *my_char, int set_mode) {
	int        old_size;

	/* only rendering case for safety mode */
	if (safety) {
		my_char->edge.right = 0.0;
		my_char->edge.left = 0.0;
		return;
	}
		
	old_size = ctxt->size();

	ctxt->params()->_size = 250;
	ctxt->set_matrix();
	set_specs(set_mode);

	fc_process_edges(r_ctxt,
					 (void*)code, &my_char->edge);
	if (my_char->edge.right < my_char->edge.left) {
		my_char->edge.right = 0.0;
		my_char->edge.left = 0.0;
	}
	else
		my_char->edge.right -= my_char->escape.x_escape/(float)old_size;

	ctxt->params()->_size = old_size;
	ctxt->set_matrix();
	set_specs(set_mode);
}

void fc_font_renderer::process_bbox(const ushort *code, fc_bbox *bbox)
{
	if (safety & 1)
		return;
	fc_process_bbox(r_ctxt, (void*)code, bbox);
/*
	bbox->top *= ctxt->escape_size();
	bbox->left *= ctxt->escape_size();
	bbox->right *= ctxt->escape_size();
	bbox->bottom *= ctxt->escape_size();
*/
}

int fc_font_renderer::set_specs(int def_flag) {	
	int          level;
	int32		 err;
	
	if (safety & 1)
		return true;
	if (ctxt->size() >= (int32)gray_status.limit_size)
		level = 1;
	else
		level = 0;
	switch (ctxt->file_type) {
	case FC_TRUETYPE_FONT :
		err = fc_setup_tt_font(r_ctxt, ctxt, level, def_flag);
		if (err == false)
			_sPrintf("fc_setup_tt_font failed !!\n");
		return err;
	case FC_TYPE1_FONT :
		err = fc_setup_ps_font(r_ctxt, ctxt, level, def_flag);
		if (err == false)
			_sPrintf("fc_setup_ps_font failed !!\n");
		return err;

	case FC_STROKE_FONT:
		err = fc_setup_stroke_font(r_ctxt, ctxt, level, def_flag);
		if (err == false)
			_sPrintf("fc_setup_stroke_font failed !!\n");
		return err;
	}
	return false;
}

void fc_close_and_flush_file(FILE *fp)
{
	fc_flush_cache((int32)fp);
	fclose(fp);
}








