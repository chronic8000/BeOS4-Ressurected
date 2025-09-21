/* ++++++++++
	FILE:	font_engine.cpp
	REVS:	$Revision: 1.8 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:32:24 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#ifndef _MATH_H
#include <math.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _FCNTL_H
#include <fcntl.h>
#endif
#ifndef	MACRO_H
#include "macro.h"
#endif
#ifndef	PROTO_H
#include "proto.h"
#endif
#ifndef _FONT_RENDERER_H
#include "font_renderer.h"
#endif
#ifndef _FONT_DATA_H
#include "font_data.h"
#endif
#ifndef _FONT_CACHE_H
#include "font_cache.h"
#endif

#include "as_support.h"
#include <Debug.h>

/*****************************************************************/
/* String metric and rasterizing API */

static void get_unistring_desc(fc_context *context,
								fc_string  *draw,
								const uint16 *uni_str,
								int32 count,
								int        flags,
								uint32     lock_mode) {
	int              i, ii, j, key, index, miss_count;
	int              hmask, offset;
	char             *buf;
	ushort           code0, code1;
	fc_char          *my_char, *ch;
	fc_set_entry     *se;
	fc_hset_item     *hash, *new_hash;
	fc_cache_status  *cs;
	fc_font_renderer *render;

	/* try to lock read only and check if there will be a cache miss */
	draw->set_hit_ref = 0;
	se = context->lock_set_entry(0, &draw->set_hit_ref);

	if (se->char_count < 0)
		xprintf("Very bad...\n");
	buf = se->char_buf;
	hash = se->hash_char;
	hmask = se->char_hmask;
	i = 0;
	ii = 0;
	while (i < count) {
		/* get the code of the character, including surrogates */
		code0 = uni_str[i++];
		if ((code0 & 0xfc00) == 0xd800)
			code1 = uni_str[i++];
		else
			code1 = 0;
		/* look for the character in the hash table */
		index = (((code0<<3)^(code0>>2))+code1)&hmask;
		while (TRUE) {
			if (hash[index].offset == -1)
				goto cache_miss;
			if ((hash[index].code[0] == code0) && (hash[index].code[1] == code1))
				break;
			index = (index+1)&hmask;
		}
		/* get character descriptor */
		offset = hash[index].offset;
		if (offset == -2)
		    offset = se->offset_default;
		my_char = (fc_char*)(buf+offset);
		/* check edges if necessary */
		if (flags & FC_DO_EDGES)
			if (my_char->edge.left == (float)1234567.0)
				goto cache_miss;
		/* store character descriptor */
		draw->chars[ii++] = my_char;
	}
	draw->char_count = ii;
	goto string_found;
	
 cache_miss:
	/* Read only mode failed. Make the set untrashable then release the lock */
	atomic_add(&se->bonus, 2*FC_HIGH_PRIORITY_BONUS);
	context->unlock_set_entry();
	/* Get a renderer (lazy renderer allocation could be restore if more than one) */
	render = new fc_font_renderer(context);
	if (!render->reset()) {
		/* the init of the renderer failed */
		delete render;
		render = 0L;
	}
	/* Lock the set read and write and try again */
 restart_string:
	draw->set_hit_ref = 0;
	se = context->lock_set_entry(1, &draw->set_hit_ref, render);
	hash = se->hash_char;
	hmask = se->char_hmask;
	i = 0;
	ii = 0;
	miss_count = 0;  /* will sub_cut the string if too many cache misses */
	cs = context->cache_status();
	while (i < count) {
		/* get the code of the character, including surrogates */
		code0 = uni_str[i++];
		if ((code0 & 0xfc00) == 0xd800)
			code1 = uni_str[i++];
		else
			code1 = 0;
		/* look for the character in the hash table */
		key = ((code0<<3)^(code0>>2))+code1;
		index = key&hmask;
		while (TRUE) {
			if (hash[index].offset == -1)
				break;
			if ((hash[index].code[0] == code0) && (hash[index].code[1] == code1)) {
				/* We find it. Get character descriptor */
				offset = hash[index].offset;
				if (offset == -2)
					offset = se->offset_default;
				/* Assert edges if necessary */
				if (flags & FC_DO_EDGES) {
					my_char = (fc_char*)(se->char_buf+offset);
					if (my_char->edge.left == (float)1234567.0) {
						/* count as a cache misses */
						miss_count++;
						/* default edges if no renderer */
						if (render == 0L) {
							my_char->edge.left = 0.0;
							my_char->edge.right = my_char->escape.x_bm_escape;
						}
						/* process the edges */
						else render->process_edges(uni_str+i-((code1 == 0)?1:2), my_char);
					}
				}
				my_char = (fc_char*)offset;
			    goto char_found;
			}
			index = (index+1)&hmask;
		}
		/* char not found : put nothing if renderer not available */
		if (render == 0L) {
			if (flags & FC_EMPTY_CHAR)
				draw->chars[ii++] = 0L;
			continue;
		}
		/* render the character */
		offset = render->render_char(uni_str+i-((code1 == 0)?1:2), se);
/*if (offset >= 0)
	if ((j = check_char((fc_char*)(se->char_buf+offset))) != 0) {
		xprintf_sync_in();
		xprintf("Step 05 invalid char [%d] (%x)!!\n", j, uni_str[i]);
		xprintf_sync_out();
	}*/
		if (offset == -2) {
			/* map to default char */
			if (se->offset_default == -1) {
				se->offset_default = render->render_default_char(se);
				if ((context->size() < FC_SIZE_MAX_SPACING) &&
					(context->size() >= FC_SIZE_MIN_SPACING))
					render->process_spacing(context->bits_per_pixel(),
											(fc_char*)(se->char_buf+se->offset_default), se);
			}
		}
		else {
			if ((context->size() < FC_SIZE_MAX_SPACING) &&
				(context->size() >= FC_SIZE_MIN_SPACING))
				render->process_spacing(context->bits_per_pixel(),
										(fc_char*)(se->char_buf+offset), se);
			ch = (fc_char*)(se->char_buf+offset);
			if ((flags & FC_DO_EDGES) && (ch->edge.left == (float)1234567.0))
				render->process_edges(uni_str+i-((code1 == 0)?1:2), ch);
		}
		/* reallocate the hash table if too many characters already */
		if (se->char_count > (se->char_hmask>>1)) {
			hmask = se->char_hmask*2+1;
			new_hash = (fc_hset_item*)grMalloc(sizeof(fc_hset_item)*(hmask+1),"fc:new_hash",MALLOC_CANNOT_FAIL);
			for (j=0; j<=hmask; j++)
				new_hash[j].offset = -1;
			for (j=0; j<=se->char_hmask; j++) {
				if (hash[j].offset == -1)
					continue;
				index = (((hash[j].code[0]<<3)^(hash[j].code[0]>>2))+hash[j].code[1])&hmask;
				while (new_hash[index].offset != -1)
					index = (index+1)&hmask;
				new_hash[index].offset = hash[j].offset;
				new_hash[index].code[0] = hash[j].code[0];
				new_hash[index].code[1] = hash[j].code[1];
			}
			se->hash_char = new_hash;
			grFree(hash);
			hash = new_hash;
			se->char_hmask = hmask;
	        atomic_add(&cs->cur_mem, sizeof(fc_hset_item)*(hmask+1)/2);	        
		}
		/* add the new char ref in the htable */
		index = key&hmask;
		while (hash[index].offset != -1)
			index = (index+1)&hmask;
		hash[index].offset = offset;
		hash[index].code[0] = code0;
		hash[index].code[1] = code1;
		se->char_count++;
		if (offset == -2)
			offset = se->offset_default;
		my_char = (fc_char*)offset;
		miss_count++;
	char_found:
		/* add the char desc to the list */
		draw->chars[ii++] = my_char;
		if (miss_count >= FC_MAX_MISS_PER_RUN)
			if (i < count) {
				/* will release the lock before the next loop if too many cache misses */
				context->unlock_set_entry();
				goto restart_string;
			}
	}
	if (render != 0L)
		delete render;
	buf = se->char_buf;
	for (i=0; i<ii; i++)
		draw->chars[i] = (fc_char*)(buf + (int)draw->chars[i]);
	draw->char_count = ii;
	/* make the set trashable again. Do not need atomic add as it's lock for write */
	se->bonus -= 2*FC_HIGH_PRIORITY_BONUS;

 string_found:
	/* set the locking mode */
	if (lock_mode == FC_DO_NOT_LOCK)
		atomic_add(&se->bonus, FC_HIGH_PRIORITY_BONUS);  // tested
	/* ready for string specific process */	
}

enum {
	MAX_PENALTY = 5
};

const float SPACE_FACTOR = 0.85;

typedef struct {
	float		sp_fwidth;				/* real total escapement for spaces */
	uint16		sp_width;				/* integer total escapement for spaces */
	uint16		sp_count;				/* count of spaces */
	uint32		penalty[MAX_PENALTY];	/* penalty for kerning of 1 to MAX_PENALTY */
	int16		width;					/* offset from the previous kerning point */
	uint16		index;					/* index of the right visible character */
	int16		delta;					/* drawing offset of the right character */
} adv_item;

typedef struct {
	uint16		pre_space_width;
	uint16		pre_space_count;	
	uint16		post_space_width;
	uint16		post_space_count;	
	uint16		last_char_width;
	uint16		left_index;			/* index of the left visible character */
	int16		left_delta;			/* drawing offset of the left character */
	uint16		item_count;
	uint16		char_count;
	int16		global_delta;		/* delta escapement for every character */
	uint16		log_count[25];		/* log repartition of kerning factor
									   0      -> 0
									   1-15   -> 1
									   16-23  -> 2  (half class of power of 2)
									   24-31  -> 3
									   32-47  -> 4
									   ...
									   32768+ -> 24 */
} adv_spaces;

static uint8 propagation_left[128] = {
	0xfe, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x0d, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x0c, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x0b, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x09, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x08, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x08, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0,
	0x07, 0x06, 0x05, 0x04, 0x02, 0x01, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint8 propagation_right[128] = {
	0xfe, 0x0d, 0x0c, 0x0b, 0x09, 0x08, 0x08, 0x07, 0, 0, 0, 0, 0, 0, 0, 0,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0, 0, 0, 0, 0, 0, 0, 0,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0, 0, 0, 0, 0, 0, 0, 0,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0, 0, 0, 0, 0, 0, 0, 0,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0, 0, 0, 0, 0, 0, 0, 0,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0, 0, 0, 0, 0, 0, 0, 0,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0, 0, 0, 0, 0, 0, 0, 0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint16 penalty_tables[7*(MAX_PENALTY*3+2)] = {
    9999, 9999, 9999, 9999, 9999, 9999, 9999,
	9999, 9999, 9999, 9999, 9999, 9999, 9999,
	9999, 9999, 9999, 8000, 6000, 5000, 4000,
	3000, 2500, 2250, 2000, 1750, 1500, 1250,
	1000,  850,  710,  580,  470,  370,  280,
	 210,  160,  120,   90,   60,   40,   23,
	  11,    7,    4,    2,    1,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0,
	   0,    0,    0,    0,    0,    0,    0
};

static uint32 penalty_limit_tables[25] = {
	1,
	16, 24, 32, 48, 64, 96,
	128, 192, 256, 384, 512, 768,
	1024, 1536, 2048, 3072, 4096, 6144,
	8192, 12288, 16368, 24552, 32768, 99999999
};

#define ADV_DEBUG 0

static void get_kerning_profile(int32		bpp,
								adv_item	*it,
								fc_char		*l_char,
								fc_char		*r_char,
								adv_spaces	*spaces) {
	uint8		*data;
	int32		left[32];
	int32		right[32];
	int32		i, min, max, width, row, pt2, pt3, delta;
	uint16		*p_table;
	uint32		index, total, mask, offset, tmax;
	uint32		step;
	
	/* choose the best integer approximation for the space,
	   and then set the kerning delta. */
	it->sp_width = (int)(it->sp_fwidth + 0.5);
	delta = (int)(it->sp_fwidth*7.0 + 0.5) - 7*it->sp_width;
	
	/* check character detection bounds */
	min = l_char->bbox.top;
	if (min > r_char->bbox.top)
		min = r_char->bbox.top;
	max = l_char->bbox.bottom;
	if (max < r_char->bbox.bottom)
		max = r_char->bbox.bottom;
		
	/* scan left character first */
	width = l_char->bbox.right-l_char->bbox.left+1;
	
	tmax = ((width*7)>>2)-1;
	if (tmax+14 < width*7)
		tmax = width*7-14;
	if (tmax > 7*MAX_PENALTY)
		tmax = 7*MAX_PENALTY;
	
	for (i=0; i<l_char->bbox.top-min; i++)
		left[i] = tmax;
		
	/* handle the grayscale shape extraction */
	if (bpp == FC_GRAY_SCALE || bpp == FC_TV_SCALE) {
		data = (uint8*)((char*)l_char + CHAR_HEADER_SIZE + ((width-1)>>1));
		row = (width+1)>>1;
		offset = 7*(width&1);
		for (; i<=l_char->bbox.bottom-min; i++) {
			index = data[0];
			total = (propagation_left[index]&0x0f);
			mask = propagation_left[index]>>4;
			index = data[-1];
			total += (propagation_left[index]&mask);
			mask &= propagation_left[index]>>4;
			index = data[-2];
			total += (propagation_left[index]&mask);
			if (total > tmax)
				left[i] = tmax;
			else
				left[i] = total-offset;
			data += row;	
		}
	}
	/* handle the black and white shape extraction */
	else {
		data = (uint8*)((char*)l_char + CHAR_HEADER_SIZE);
		for (; i<=l_char->bbox.bottom-min; i++) {
			if (data[0] == 0xff) {
				left[i] = tmax;
				data++;				
			}
			else {
				offset = width;
				while ((step = *data++) != 0xff)
					offset -= step;
				left[i] = offset*7;
				if (left[i] > tmax)
					left[i] = tmax;
			}		
		}
	}
	for (; i<=max-min; i++)
		left[i] = tmax;
#if ADV_DEBUG
	xprintf("(bpp:%d) Left: ", bpp);
	for (i=0; i<=max-min; i++)
		xprintf("%d ", left[i]);
	xprintf("\n");
#endif
	/* scan right character first */
	width = r_char->bbox.right-r_char->bbox.left+1;
	
	tmax = ((width*7)>>2)-1;
	if (tmax+14 < width*7)
		tmax = width*7-14;
	if (tmax > 7*MAX_PENALTY)
		tmax = 7*MAX_PENALTY;
	
	if (width > 4) {
		pt2 = 1;
		pt3 = 2;
	} else {
		pt3 = 0;
		if (width > 2)
			pt2 = 1;
		else
			pt2 = 0;
	}
	
	for (i=0; i<r_char->bbox.top-min; i++)
		right[i] = tmax;
	/* handle the grayscale shape extraction */
	if (bpp == FC_GRAY_SCALE || bpp == FC_TV_SCALE) {
		data = (uint8*)((char*)r_char + CHAR_HEADER_SIZE);
		row = (width+1)>>1;
		for (; i<=r_char->bbox.bottom-min; i++) {
			index = data[0];
			total = (propagation_right[index]&0x0f);
			mask = propagation_right[index]>>4;
			index = data[pt2];
			total += (propagation_right[index]&mask);
			mask &= propagation_right[index]>>4;
			index = data[pt3]|7;
			total += (propagation_right[index]&mask);
			if (total > tmax)
				right[i] = tmax;
			else
				right[i] = total;
			data += row;	
		}
	}
	/* handle the black and white shape extraction */
	else {
		data = (uint8*)((char*)r_char + CHAR_HEADER_SIZE);
		for (; i<=l_char->bbox.bottom-min; i++) {
			right[i] = data[0]*7;
			if (right[i] > tmax)
				right[i] = tmax;
			while (*data++ != 0xff) {;}		
		}
	}
	for (; i<=max-min; i++)
		right[i] = tmax;
#if ADV_DEBUG
	xprintf("Right: ");
	for (i=0; i<=max-min; i++)
		xprintf("%d ", right[i]);
	xprintf("\n");
#endif
	
	/* process penalty coefficients */
	p_table = penalty_tables + (7-delta);
	index = left[0]+right[0];
	it->penalty[0] = p_table[index+28];
	it->penalty[1] = p_table[index+21];
	it->penalty[2] = p_table[index+14];
	it->penalty[3] = p_table[index+7];
	it->penalty[4] = p_table[index];
	for (i=1; i<max-min; i++) {
		index = left[i-1]+right[i]+5;
		it->penalty[0] += p_table[index+28];
		it->penalty[1] += p_table[index+21];
		it->penalty[2] += p_table[index+14];
		it->penalty[3] += p_table[index+7];
		it->penalty[4] += p_table[index];
		index = left[i]+right[i];
		it->penalty[0] += p_table[index+28];
		it->penalty[1] += p_table[index+21];
		it->penalty[2] += p_table[index+14];
		it->penalty[3] += p_table[index+7];
		it->penalty[4] += p_table[index];
		index = left[i+1]+right[i]+5;
		it->penalty[0] += p_table[index+28];
		it->penalty[1] += p_table[index+21];
		it->penalty[2] += p_table[index+14];
		it->penalty[3] += p_table[index+7];
		it->penalty[4] += p_table[index];
	}
	index = left[max-min]+right[max-min];
	it->penalty[0] += p_table[index+28];
	it->penalty[1] += p_table[index+21];
	it->penalty[2] += p_table[index+14];
	it->penalty[3] += p_table[index+7];
	it->penalty[4] += p_table[index];
	/* accumulate the penalty result in log_count */
	for (i=0; i<5; i++) {
		total = it->penalty[i];
		if (total < 16)
			spaces->log_count[(total+15)>>4]++;
		else if (total < 32768) {
			index = 2;
			while (total > 31) {
				total >>= 1;
				index += 2;
			}
			spaces->log_count[index+((total-8)>>4)]++;
		}
		else
			spaces->log_count[24]++;
	}
}

static int32 preprocess_advanced_kerning(int32		bpp,
										 fc_string	*draw,
										 int32     	tw0,
										 adv_item	*items,
										 adv_spaces	*spaces,
										 float		delta_escape) {
	int			i, count, esc0, esc1, total;
	int			i1, i2;
	int			width, width2, space_count, it_count;
	float		space_total;
	fc_char		*l_char, *r_char;
	adv_item	*it;

	/* init log counter */
	for (i=0; i<25; i++)
		spaces->log_count[i] = 0;
	/* First string pass :
	   - measure basic bitmap metrics,
	   - do the pairing between non space character,
	   - do the advanced kerning detection. */
	it = items;
	
	i1 = 0;
	space_count = 0;
	count = draw->char_count;
	spaces->char_count = count;
	space_total = 0.0;
	do {
		l_char = draw->chars[i1];
		width = l_char->bbox.right-l_char->bbox.left+1;
		if (width == 0) {
			if (l_char->escape.x_escape > 0.0) {
				space_total += l_char->escape.x_escape*SPACE_FACTOR + delta_escape;
				space_count++;
			}
		}
		else {
			spaces->left_index = i1;
			spaces->left_delta = esc0 = l_char->bbox.left;
			if (space_count == 1) space_total += 1.0;
			total = (spaces->pre_space_width = (int32)space_total);
			spaces->pre_space_count = space_count;
			it_count = 1;
			goto found_first_visible_char;
		}
		i1++;
	} while (i1<count);
	
	/* no visible characters */
	if (space_count == 1) space_total += 1.0;
	total = (spaces->pre_space_width = (int32)space_total);
	spaces->left_index = 0;
	spaces->last_char_width = 0;
	spaces->post_space_width = 0; 
	spaces->post_space_count = 0;
	spaces->pre_space_count = space_count;
	spaces->item_count = 0;
	return total-tw0;
	
found_second_visible_char:
	if (space_count == 1) space_total += 1.0;
	it->sp_fwidth = space_total;
	it->sp_count = space_count;
	it->width = width;
	it->delta = r_char->bbox.left;
	it->index = i2;
	get_kerning_profile(bpp, it, l_char, r_char, spaces);
	total += width+1+it->sp_width;
	/* next pair... */
	l_char = r_char;
	width = width2;
	i1 = i2;
	it++;
	it_count++;

found_first_visible_char:
	i2 = i1+1;
	space_count = 0;
	space_total = 0.0;
	while (i2<count) {
		r_char = draw->chars[i2];
		width2 = r_char->bbox.right-r_char->bbox.left+1;
		if (width2 == 0) {
			if (r_char->escape.x_escape > 0.0) {
				space_total += r_char->escape.x_escape*SPACE_FACTOR + delta_escape;
				space_count++;
			}
		}
		else
			goto found_second_visible_char;
		i2++;
	}
	/* end of the string */
	spaces->last_char_width = width;
	if (space_count == 1) space_total += 1.0;
	spaces->post_space_width = (int32)space_total;
	total += spaces->last_char_width + spaces->post_space_width;
	
	spaces->post_space_count = space_count;
	spaces->item_count = it_count;
	esc1 = l_char->bbox.left;
//	return total+(esc1-esc0)-tw0;
	return total-tw0;
}

static void postprocess_advanced_kerning(	int32		delta_t,
											adv_item	*items,
									   		adv_spaces	*spaces) {
	int32		i, j, k, m;
	uint32		*sort_list;
	uint32		default_list[32];
	uint32		penalty_limit, min, max, count;
	uint16		penalty_limit_count;
	adv_item	*it;

#if ADV_DEBUG
	xprintf("Delta_t : %d\n", delta_t);
	xprintf("%d items\n", spaces->item_count-1);
	for (i=0; i<spaces->item_count-1; i++) {
		xprintf("Item %d: space:(%f)%d, width:%d, delta:%d, index:%d\n", i,
				items[i].sp_fwidth, items[i].sp_width, items[i].width,
				items[i].delta, items[i].index);		
		xprintf("  [%d,%d,%d,%d,%d]\n",
				items[i].penalty[0],	
				items[i].penalty[1],	
				items[i].penalty[2],	
				items[i].penalty[3],	
				items[i].penalty[4]);	
	}	
	xprintf("  space [pre/post]: %d/%d\n",
			spaces->pre_space_width, spaces->post_space_width);
	xprintf("  left offset:%d, index:%d\n\n",
			spaces->left_delta, spaces->left_index);
	xprintf("Log count result :  [0]          -> %3d\n", spaces->log_count[0]);
	xprintf("[1-15]       -> %3d [16-23]      -> %3d\n",
			spaces->log_count[1], spaces->log_count[2]);
	xprintf("[24-31]      -> %3d [32-47]      -> %3d\n",
			spaces->log_count[3], spaces->log_count[4]);
	xprintf("[48-63]      -> %3d [64-95]      -> %3d\n",
			spaces->log_count[5], spaces->log_count[6]);
	xprintf("[96-127]     -> %3d [128-191]    -> %3d\n",
			spaces->log_count[7], spaces->log_count[8]);
	xprintf("[192-255]    -> %3d [256-383]    -> %3d\n",
			spaces->log_count[9], spaces->log_count[10]);
	xprintf("[384-511]    -> %3d [512-767]    -> %3d\n",
			spaces->log_count[11], spaces->log_count[12]);
	xprintf("[768-1023]   -> %3d [1024-1535]  -> %3d\n",
			spaces->log_count[13], spaces->log_count[14]);
	xprintf("[1536-2047]  -> %3d [2048-3071]  -> %3d\n",
			spaces->log_count[15], spaces->log_count[16]);
	xprintf("[3071-4095]  -> %3d [4096-6143]  -> %3d\n",
			spaces->log_count[17], spaces->log_count[18]);
	xprintf("[6144-8191]  -> %3d [8192-12287] -> %3d\n",
			spaces->log_count[19], spaces->log_count[20]);
	xprintf("[12288-16383]-> %3d [16384-24575]-> %3d\n",
			spaces->log_count[21], spaces->log_count[22]);
	xprintf("[24576-32767]-> %3d [32768-...]->   %3d\n",
			spaces->log_count[23], spaces->log_count[24]);
			
	xprintf("Delta_t required : %d\n", delta_t);	
#endif
	/* if delta_t is negative, we need to expand the string */
	spaces->global_delta = 0;
	while (delta_t < 0) {
		spaces->global_delta++;
		delta_t += spaces->char_count;
	}	
	/* calculate the maximum reasonnable kerning possible */
	m = 0;
	for (i=0; i<24; i++)
		m += spaces->log_count[i];
	/* compensate for very strong kerning needs */
	while (delta_t > m) {
		spaces->global_delta--;
		delta_t -= spaces->char_count;
	}
	/* look for the threshold level */
	i = 0;
	while (i < 25) {
		if (spaces->log_count[i] >= delta_t)
			goto found_threshold_level;
		delta_t -= spaces->log_count[i];
		i++;
	}
	/* Desesperate situation : not enough kerning ! */
	xprintf("Postprocess_advanced_kerning fatal error !!");
	it = items;
	for (i=0; i<spaces->item_count; i++) {
		it->width = 0;
		it->sp_width = it->sp_count;
		it++;
	}
	spaces->global_delta = -1;
	spaces->pre_space_width = 0;
	spaces->post_space_width = 0;
	return;
	
	/* Need to find the real threshold inside the threshold level */
found_threshold_level:
	if (spaces->log_count[i] == delta_t) {
		penalty_limit = penalty_limit_tables[i];
		penalty_limit_count = 0;
	}
	else {
		/* get a buffer to sort the penalties in that range */
		if (spaces->log_count[i] > 32)
			sort_list = (uint32*)grMalloc(sizeof(uint32)*spaces->log_count[i],"fc:sort_list",MALLOC_CANNOT_FAIL);
		else
			sort_list = default_list;
		/* define the range to be checked */
		if (i > 0)
			min = penalty_limit_tables[i-1];
		else
			min = 0;
		max = penalty_limit_tables[i];
		/* do the sorting */
		it = items;
		count = 0;
		for (i=0; i<spaces->item_count-1; i++) {
			j = 0;
			while ((it->penalty[j] < min) && (j<5))
				j++;
			while ((it->penalty[j] < max) && (j<5)) {
				k = 0;
				while (k<count) {
					if (sort_list[k] >= it->penalty[j]) {
						for (m=count-1; m>=k; m--)
							sort_list[m+1] = sort_list[m];
						break;
					}
					k++;
				}
				sort_list[k] = it->penalty[j];
				count++;
				j++;
			}
			it++;
		}
		/* decided where the threshold should really be */
		penalty_limit = sort_list[delta_t];
		penalty_limit_count = 0;
		for (i=delta_t-1; i>=0; i--) {
			if (sort_list[i] == penalty_limit)
				penalty_limit_count++;
			else
				break;
		}
		/* free the list if necessary */
		if (sort_list != default_list)
			grFree(sort_list);
	}
#if ADV_DEBUG
	xprintf("Penalty threshold : %d (%d)\n",
			penalty_limit, penalty_limit_count);
	xprintf("Kerning: ");
#endif
	/* post_process the item list to take the kerning into account */
	it = items;
	for (i=0; i<spaces->item_count-1; i++) {
		j = 0;
		while ((it->penalty[j] < penalty_limit) && (j<5))
			j++;
		if ((j<5) && (it->penalty[j] == penalty_limit) && (penalty_limit_count>0)) {
			penalty_limit_count--;
			j++;
		}
#if ADV_DEBUG
		xprintf("%d ", j);
#endif
		it->width -= j;
		it++;
	}
#if ADV_DEBUG
	xprintf("\n");
#endif
}

inline float fround(float f) {
	if (f >= 0.0)
		return (float)((int)(f+0.5));
	else
		return -(float)((int)(-f+0.5));
}

void fc_lock_string(fc_context *context,
					fc_string  *draw,
					const uint16 *string,
					int32      length,
					float      dnsp,
					float      dsp,
					double	   *xx,
					double	   *yy,
					uint32     lock_mode) {
	int32				i, count, h, v, hmin, hmax, vmin, vmax;
	double				x, y;
	fc_char				*my_char;
	fc_escape_table		e_table;

	/* set bpp */
	draw->bits_per_pixel = context->bits_per_pixel();
	fc_get_escapement(context, &e_table, string, length, draw, dnsp, dsp, true, true);
	
	count = e_table.count;
	if (count == 0) {
		hmin = 0;
		hmax = -1;
		vmin = 0;
		vmax = -1;
		goto end;
	}

	x = *xx+32767.0;
	y = *yy+32767.0;
	for (i=0; i<count; i++) {
		draw->offsets[i].x = (int)(x+e_table.offsets[i].h)-32767;
		draw->offsets[i].y = (int)(y+e_table.offsets[i].v)-32767;
		x += e_table.escapes[i].x_escape;
		y += e_table.escapes[i].y_escape;
	}
	*xx = x-32767.0;
	*yy = y-32767.0;
	
	/* calculate global rect frame */ 
	hmin = 1048576*1024;
	hmax = -1048576*1024;
	vmin = 1048576*1024;
	vmax = -1048576*1024;
	for (i=0; i<count; i++) {
		my_char = draw->chars[i];
		h = draw->offsets[i].x;
		v = draw->offsets[i].y;
		if ((my_char->bbox.left+h) < hmin)
			hmin = my_char->bbox.left+h;
		if ((my_char->bbox.right+h) > hmax)
			hmax = my_char->bbox.right+h;
		if ((my_char->bbox.top+v) < vmin)
			vmin = my_char->bbox.top+v;
		if ((my_char->bbox.bottom+v) > vmax)
			vmax = my_char->bbox.bottom+v;
	}
 end:
	draw->bbox.top = vmin;
	draw->bbox.left = hmin;
	draw->bbox.right = hmax;
	draw->bbox.bottom = vmax;
	/* save the new position of the pen */

	if (lock_mode == FC_DO_NOT_LOCK)
		context->unlock_set_entry();
}

void fc_unlock_string(fc_context *context) {
	context->unlock_set_entry();
	context->cache_status()->assert_mem();
}

void fc_get_escapement(fc_context      *context,
					   fc_escape_table *table,
					   const uint16    *string,
					   int32           length,
					   fc_string	   *draw,
					   float           non_space_delta,
					   float           space_delta,
					   bool			   use_real_size,
					   bool			   keep_locked) {
	int			i, j, count, tw0, h0;
	int			delta_t, reminder, kern, index, delta, sp_count, sp_width, sp_step;
	float		step_x=0, step_y=0, escape_factor, escape_ratio, real_size, rounded_size;
	float		x, y;
	fc_char		*my_char;
	adv_item	adv_items[FC_MAX_STRING-1];
	adv_item	*adv_it;
	adv_spaces	adv_sp;

	/* get the part of the string that will be really process this time */
	get_unistring_desc(context, draw, string, length, FC_EMPTY_CHAR, 0);
	/* calculate the escapement factor and escapement ratio */
	real_size = context->escape_size();
	rounded_size = (float)context->size();
	if (real_size != rounded_size)
		escape_factor = real_size/rounded_size;
	else
		escape_factor = 1.0;
	/* copy count */
	count = draw->char_count;
	table->count = count;
	if (count > 0) {
	/* process spacing and escapement, in mode "non proportional" */
		if (context->spacing_mode() == B_FIXED_SPACING) {
			/* use the real size to process the ratio */
			if (use_real_size)
				escape_ratio = 1.0;
			else
				escape_ratio = 1.0/real_size;
			for (i=0; i<count; i++) {
				my_char = draw->chars[i];
				if (my_char != 0L) {
					step_x = fround(my_char->escape.x_bm_escape*escape_factor)*escape_ratio;
					step_y = fround(my_char->escape.y_bm_escape*escape_factor)*escape_ratio;
					break;
				}
			}
			for (i=0; i<count; i++) {
				if (draw->chars[i] != 0L) {
					table->escapes[i].x_escape = step_x;
					table->escapes[i].y_escape = step_y;
				}
				else {
					table->escapes[i].x_escape = 0.0;
					table->escapes[i].y_escape = 0.0;
				}
				table->offsets[i].h = 0.0;
				table->offsets[i].v = 0.0;
			}
		}
		/* process spacing and escapement, in mode "character compatible with printing" */
		else {
			/* check delta spacing mode */
			/* in mode "character compatible with printing", by default */
			if ((context->escape_size() >= FC_SIZE_MAX_SPACING) ||
				(context->escape_size() < FC_SIZE_MIN_SPACING) ||
				( (context->spacing_mode() == B_STRING_SPACING)
					&& (context->rotation() != 0.0) ) ) {
			/* || (context->shear() != 0.0) --- Let's try to handle shearing also... */
				if (use_real_size)
					escape_ratio = escape_factor;
				else
					escape_ratio = 1.0/rounded_size;
				if ((non_space_delta == 0.0) && (space_delta == 0.0))
				/* use the rounded size to process the ratio */
					for (i=0; i<count; i++) {
						my_char = draw->chars[i];
						table->escapes[i].x_escape = my_char->escape.x_escape*escape_ratio;
						table->escapes[i].y_escape = my_char->escape.y_escape*escape_ratio;
						table->offsets[i].h = 0.0;
						table->offsets[i].v = 0.0;
					}
				else for (i=0; i<count; i++) {
				/* use the real size to process the ratio */
					my_char = draw->chars[i];
					x = my_char->escape.x_escape*escape_ratio;
					y = my_char->escape.y_escape*escape_ratio;
					if (((my_char->bbox.left-my_char->bbox.right) == 1) &&
						(my_char->escape.x_escape > 0)) {
						x += context->delta_space_x*space_delta*escape_ratio;
						y += context->delta_space_y*space_delta*escape_ratio;
					}
					else {
						x += context->delta_space_x*non_space_delta*escape_ratio;
						y += context->delta_space_y*non_space_delta*escape_ratio;		
					}
					table->escapes[i].x_escape = x;
					table->escapes[i].y_escape = y;
					table->offsets[i].h = 0.0;
					table->offsets[i].v = 0.0;
				}
			}
			/* in mode "character compatible with printing", by selection */
			else if (context->spacing_mode() == B_CHAR_SPACING) {
				if (use_real_size)
					escape_ratio = escape_factor;
				else
					escape_ratio = 1.0/rounded_size;
			/* || (context->shear() != 0.0) --- Let's try to handle shearing also... */
				if ((non_space_delta == 0.0) && (space_delta == 0.0))
				/* use the rounded size to process the ratio */
					for (i=0; i<count; i++) {
						my_char = draw->chars[i];
						table->escapes[i].x_escape = my_char->escape.x_escape*escape_ratio;
						table->escapes[i].y_escape = my_char->escape.y_escape*escape_ratio;
						table->offsets[i].h = 0.0625*(float)my_char->spacing.char_offset;
						table->offsets[i].v = 0.0;
					}
				else for (i=0; i<count; i++) {
				/* use the real size to process the ratio */
					my_char = draw->chars[i];
					x = my_char->escape.x_escape*escape_ratio;
					y = my_char->escape.y_escape*escape_ratio;
					if (((my_char->bbox.left-my_char->bbox.right) == 1) &&
						(my_char->escape.x_escape > 0)) {
						x += context->delta_space_x*space_delta*escape_ratio;
						y += context->delta_space_y*space_delta*escape_ratio;
					}
					else {
						x += context->delta_space_x*non_space_delta*escape_ratio;
						y += context->delta_space_y*non_space_delta*escape_ratio;
					}
					table->escapes[i].x_escape = x;
					table->escapes[i].y_escape = y;
					table->offsets[i].h = 0.0625*(float)my_char->spacing.char_offset;
					table->offsets[i].v = 0.0;
				}
			}
			/* string metric analysis */
			else if (context->spacing_mode() == B_STRING_SPACING) {
				/* startup point */
				x = 1e-7;
				if ((non_space_delta == 0.0) && (space_delta == 0.0)) {
					for (i=0; i<count; i++) {
						my_char = draw->chars[i];
						x += my_char->escape.x_escape*escape_factor;
					}
				}
				else {
					for (i=0; i<count; i++) {
						my_char = draw->chars[i];
						x += my_char->escape.x_escape*escape_factor;
						if (((my_char->bbox.left-my_char->bbox.right) == 1) &&
							(my_char->escape.x_escape > 0))
							x += context->delta_space_x*space_delta;
						else
							x += context->delta_space_x*non_space_delta;
					}
				}
				tw0 = (int)(x-1e-7+0.5);

				step_x = context->delta_space_x*(space_delta - non_space_delta);
				delta_t = preprocess_advanced_kerning(	context->bits_per_pixel(),
														draw,
														tw0,
														adv_items,
														&adv_sp,
														step_x);
				postprocess_advanced_kerning(delta_t, adv_items, &adv_sp);
	
				i = 0;

				h0 = 0;
				sp_count = adv_sp.pre_space_count;
				sp_width = adv_sp.pre_space_width;
				index = adv_sp.left_index;
				delta = adv_sp.left_delta;		

				adv_it = adv_items;
				for (j=0; ; j++) {
					while (i < index) {
						table->escapes[i].x_escape = (float)h0;
						table->escapes[i].y_escape = 0.0;
						table->offsets[i].h = 0.0;
						table->offsets[i].v = 0.0;
						if (sp_count == 1)
							sp_step = sp_width;
						else
							sp_step = (sp_width + (sp_count>>1))/sp_count;
						sp_width -= sp_step;
						h0 += adv_sp.global_delta + sp_step;
						sp_count--;
						i++;
					}
					
					if (i == count) break;
						
					table->escapes[i].x_escape = (float)h0;
					table->escapes[i].y_escape = 0.0;
					table->offsets[i].h = (float)(-delta);
					table->offsets[i].v = 0.0;
					i++;

					if (j < adv_sp.item_count-1) {
						h0 += adv_it->width + 1 + adv_sp.global_delta;
						sp_count = adv_it->sp_count;
						sp_width = adv_it->sp_width;
						index = adv_it->index;
						delta = adv_it->delta;
						adv_it++;
					}
					else {
						h0 += adv_sp.last_char_width + 1 + adv_sp.global_delta;
						sp_width += adv_sp.post_space_width;
						sp_count += adv_sp.post_space_count;	
						index = count;
					}
				}

				/* use the real size to process the ratio */
				if (use_real_size)
					escape_ratio = 1.0;
				else
					escape_ratio = 1.0/real_size;
				/* convert positions to escapements format */
				for (i=0; i<count-1; i++)
					table->escapes[i].x_escape =
					(table->escapes[i+1].x_escape - table->escapes[i].x_escape)*escape_ratio;
				table->escapes[count-1].x_escape =
					((float)h0 - table->escapes[count-1].x_escape)*escape_ratio;
			}
			/* bitmap metric analysis */
			else {
				if (use_real_size)
					escape_ratio = escape_factor;
				else
					escape_ratio = 1.0/rounded_size;
				if ((non_space_delta == 0.0) && (space_delta == 0.0))
				/* use the rounded size to process the ratio */
					for (i=0; i<count; i++) {
						my_char = draw->chars[i];
						table->escapes[i].x_escape = my_char->escape.x_bm_escape*escape_ratio;
						table->escapes[i].y_escape = my_char->escape.y_bm_escape*escape_ratio;
						table->offsets[i].h = -my_char->spacing.offset;
						table->offsets[i].v = 0.0;
					}
				else for (i=0; i<count; i++) {
				/* use the real size to process the ratio */
					my_char = draw->chars[i];
					x = my_char->escape.x_bm_escape*escape_ratio;
					y = my_char->escape.y_bm_escape*escape_ratio;
					if (((my_char->bbox.left-my_char->bbox.right) == 1) &&
						(my_char->escape.x_escape > 0)) {
						x += context->delta_space_x*space_delta*escape_ratio;
						y += context->delta_space_y*space_delta*escape_ratio;
					}
					else {
						x += context->delta_space_x*non_space_delta*escape_ratio;
						y += context->delta_space_y*non_space_delta*escape_ratio;
					}
					table->escapes[i].x_escape = x;
					table->escapes[i].y_escape = y;
					table->offsets[i].h = -my_char->spacing.offset;
					table->offsets[i].v = 0.0;
				}
			}
		}
	}
	if (!keep_locked)
		context->unlock_set_entry();
}

void fc_get_glyphs(fc_context *context, fc_glyph_table *g_table,
				   const uint16 *uni_str, int32 count) {
	int              i, ii;
	uint16           code0, code1;
	fc_font_renderer *render;

	/* Get a renderer (lazy renderer allocation could be restore if more than one) */
	render = new fc_font_renderer(context);
	if (!render->reset()) {
		/* the init of the renderer failed */
		delete render;
		render = NULL;
	}

	i = 0;
	ii = 0;
	while (i < count) {
		/* get the code of the character, including surrogates */
		code0 = uni_str[i++];
		if ((code0 & 0xfc00) == 0xd800)
			code1 = uni_str[i++];
		else
			code1 = 0;
		/* render the character */
		if (render == 0)
			g_table->glyphs[ii] = NULL;
		else
			g_table->glyphs[ii] = render->get_char_outline(uni_str+i-((code1 == 0)?1:2));
		ii++;
	}
	g_table->count = ii;
	
	if (render != NULL)
		delete render;
}

void fc_get_edge(fc_context *context, fc_edge_table *table, const uint16 *string, int32 count) {
	int           i;
	fc_char       *my_char;
	fc_string     draw;

	get_unistring_desc(context, &draw, string, count, FC_EMPTY_CHAR | FC_DO_EDGES, 0);
	/* process edges */
	count = draw.char_count;
	table->count = count;
	for (i=0; i<count; i++) {
		my_char = draw.chars[i];
		if (my_char != 0L) {
			table->edges[i].left = my_char->edge.left;
			table->edges[i].right = my_char->edge.right;
		}
		else {
			table->edges[i].left = 0.0;
			table->edges[i].right = 0.0;
		}
	}
	context->unlock_set_entry();
}

void fc_add_string_width(fc_context *context, const uint16 *string, int32 count,
						 float escape_factor, float *x_delta, float *y_delta) {
	int       i;
	fc_char   *my_char;
	fc_string draw;

	get_unistring_desc(context, &draw, string, count, 0, 0);
	/* check that length is not null */
	count = draw.char_count;
	if (count > 0) {
		/* process spacing and escapement, in mode "non proportional" */
		if (context->spacing_mode() == B_FIXED_SPACING) {
			i = 0;
			do {
				my_char = draw.chars[i++];
			} while (!my_char && i<count);
			if (my_char) {
				const float step_x = fround(my_char->escape.x_bm_escape*escape_factor);
				const float step_y = fround(my_char->escape.y_bm_escape*escape_factor);
				const float factor = (float)count;
				*x_delta += step_x*factor;
				*y_delta += step_y*factor;
			}
		}
		/* common part for others mode */
		else if ((context->spacing_mode() == B_BITMAP_SPACING) &&
				 (context->escape_size() < FC_SIZE_MAX_SPACING) &&
				 (context->escape_size() >= FC_SIZE_MIN_SPACING)) {
			/* || (context->shear() != 0.0) --- Let's try to handle shearing also... */
			for (i=0; i<count; i++) {
				my_char = draw.chars[i];
				*x_delta += my_char->escape.x_bm_escape*escape_factor;
				*y_delta += my_char->escape.y_bm_escape*escape_factor;
			}
		}
		else {
			for (i=0; i<count; i++) {
				my_char = draw.chars[i];
				*x_delta += my_char->escape.x_escape*escape_factor;
				*y_delta += my_char->escape.y_escape*escape_factor;
			}
		}
	}
	context->unlock_set_entry();
}

void fc_get_bbox(	fc_context		*context,
					fc_bbox_table	*b_table,
					const uint16	*string,
					int32			count,
					float			dnsp,
					float			dsp,
					int				mode,
					int				string_mode,
					double			*offset_h,
					double			*offset_v) {
	int					i, j;
	int32				idh, idv;
	float				dh, dv;
	uint16				code0, code1;
	fc_bbox				bbox;
	fc_char				*my_char;
	fc_string			draw;
	fc_escape_table		e_table;
	fc_font_renderer	*render;

	/* We want the metric of the real screen bitmap */
	if (mode == B_SCREEN_METRIC) {
		/* We just want the bounding boxes, one by one */
		if (!string_mode) {
			get_unistring_desc(context, &draw, string, count, 0, 0);
			count = draw.char_count;
			b_table->count = count;
			for (i=0; i<count; i++) {
				b_table->bbox[i].top = (float)draw.chars[i]->bbox.top;
				b_table->bbox[i].left = (float)draw.chars[i]->bbox.left;
				b_table->bbox[i].right = (float)draw.chars[i]->bbox.right;
				b_table->bbox[i].bottom = (float)draw.chars[i]->bbox.bottom;
			}
			context->unlock_set_entry();
		}
		/* We want then at their real string position */
		else {
			fc_lock_string(context, &draw, string, count, dnsp, dsp,
							offset_h, offset_v, FC_DO_NOT_LOCK);
			count = draw.char_count;
			b_table->count = count;
			for (i=0; i<count; i++) {
				my_char = draw.chars[i];
				idh = draw.offsets[i].x;
				idv = draw.offsets[i].y;
				b_table->bbox[i].top = (float)(my_char->bbox.top+idv);
				b_table->bbox[i].left = (float)(my_char->bbox.left+idh);
				b_table->bbox[i].right = (float)(my_char->bbox.right+idh);
				b_table->bbox[i].bottom = (float)(my_char->bbox.bottom+idv);
			}
			context->unlock_set_entry();
		}
	}
	/* we want the approximated floating point printing metric */
	else {
		/* if we need the escapements, then we need to extract them. In that case, the
		   escapement will control the string flow. In the other case, the string flow
		   (determined by the value of count and the "string" pointer) will just be
		   maximal as defined by get_unicode_128 */
		if (string_mode) {
			fc_get_escapement(context, &e_table, string, count, &draw, dnsp, dsp);
			count = e_table.count;
		}
		/* Init the renderer  */
		render = new fc_font_renderer(context);
		if (!render->reset(FC_RENDER_BBOX)) {
			/* the init of the renderer failed */
			delete render;
			b_table->count = 0;
			return;
		}
		/* Scan the string and process the bounding boxes */
		i = 0;
		j = 0;
		dh = 0.0;
		dv = -1.0;
		while (i < count) {
			/* get the code of the character, including surrogates */
			code0 = string[i++];
			if ((code0 & 0xfc00) == 0xd800)
				code1 = string[i++];
			else
				code1 = 0;
			render->process_bbox(string+i-((code1 == 0)?1:2), &bbox);
			if (string_mode) {
				dh = *offset_h;
				dv = *offset_v-1.0;
				*offset_h += e_table.escapes[j].x_escape*context->escape_size();
				*offset_v += e_table.escapes[j].y_escape*context->escape_size();
			}
			b_table->bbox[j].top = dv+bbox.top;
			b_table->bbox[j].left = dh+bbox.left;
			b_table->bbox[j].right = dh+bbox.right;
			b_table->bbox[j].bottom = dv+bbox.bottom;
			j++;
		}
		delete render;
		b_table->count = count;
	}
}

bool fc_has_glyphs(fc_context *context,
					const uint16 *str, int32 count, bool *has) {
	fc_file_entry* fe;
	
	fc_lock_file();
	if (context->file_id >= file_table_count)
		goto no_glyphs;
	
	fe = file_table + context->file_id;
	if (fe->block_bitmaps == NULL)
		goto no_glyphs;
		
	// check availability of each unicode character
	while (count-- > 0) {
		//printf("Character 0x%x: block=%d, index=%d, bits=0x%08lx, has=%s\n",
		//		*str, (*str)/0x100, ((*str)&0xff)/32,
		//		fe->block_bitmaps[fe->char_blocks[(*str)/0x100]][((*str)&0xff)/32],
		//		fc_fast_has_glyph(fe, *str) ? "true" : "false");
		*has++ = fc_fast_has_glyph(fe, *str++);
	}

	fc_unlock_file();
	return true;
	
no_glyphs:
	fc_unlock_file();
	while (count-- > 0)
		*has++ = false;
	return false;
}
