/* ++++++++++
	FILE:	font_set.cpp
	REVS:	$Revision: 1.6 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:32:17 PST 1997
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

// This is the character as it is stored in the file, which is slightly
// different than the in-memory version due to code changes.

typedef struct {
	float       left;
	float       right;
} fc_tuned_edge;

typedef struct {
	short       left;           /* bounding box of the string relative */
	short       top;            /* to the current origin point */
	short       right;
	short       bottom;
} fc_tuned_rect;

typedef struct {
	float       x_escape;       /* escapement to next character, */
	float       y_escape;          
} fc_tuned_escape;

typedef struct {
	fc_tuned_edge     edge;        /* edges of the scalable character */
	fc_tuned_rect     bbox;        /* bounding box relative to the
									  baseline origin point (bitmap) */
	fc_tuned_escape   escape;      /* all the character escapements */
} fc_tuned_char;

#define CHAR_TUNED_HEADER_SIZE (sizeof(fc_tuned_edge)+sizeof(fc_tuned_rect)+sizeof(fc_tuned_escape))

/*****************************************************************/
/* Font set API */

void fc_flush_font_set(fc_context *context) {	
	uint32               pipo_hit_ref;
	fc_set_entry         *se;

	se = context->lock_set_entry(1, &pipo_hit_ref);
	se->invalidate();
	set_uid++;
	se->bonus = -FC_HIGH_PRIORITY_BONUS;
	context->unlock_set_entry();
//	context->cache_status()->flush();
	context->cache_status()->assert_mem();
}

int fc_save_full_set(fc_context    *context,
					 ushort        *dir_id,
					 char          *filename,
					 char          *pathname,
					 fc_file_entry **fet) {
	int                  i, j, length, count, total_length;
	int                  name_length, family_name_length, style_name_length;
	char                 buffer[4];
	char                 *family_name, *style_name;
	FILE                 *fp;
	uint16               ref_array[256];
	uint32               pipo_hit_ref;
	fc_char              *ch;
	fc_tuned_char        ch_buffer;
	fc_string			 draw;
	fc_hset_item         hset_buffer;
	fc_hset_item         *htable;
	fc_set_entry         *se;
	fc_dir_entry     	 *d;
	fc_file_entry        *fe;
	fc_escape_table      e_table;
	fc_tuned_file_header tfh;

	/* first, flush the set out of the cache (for real) */
	fc_flush_font_set(context);
	/* lock the set entry in the cache */
	se = context->lock_set_entry(0, &pipo_hit_ref);
	atomic_add(&se->bonus, FC_HIGH_PRIORITY_BONUS);
	/* get the file entry of the tuned file, create it if necessary */
	if (context->file_id == 0xffff)
		goto exit_bad_error;
	fe = file_table+context->file_id;
	if (context->tuned_file_id == 0xffff) {
		/* calculate the name of the tuned font file */
		if (context->file_id < file_table_count) {
			d = dir_table+fe->dir_id;
			i = strlen(d->dirname);
			memcpy(pathname, d->dirname, i);
			pathname[i++] = '/';		
			length = strlen(fe->filename);
			memcpy(pathname+i, fe->filename, length);
			i += length;
			/* remove suffix */
			for (j=i-1; j>i-length; j--)
				if (pathname[j] == '.') {
					i = j;
					break;
				}
			/* add size terminator */
			pathname[i++] = '_';
			sprintf(pathname+i, "%d", context->size());
		}
		else goto exit_bad_error;
		/* check if the file doesn't already exists. If so, abort... */
		fp = fopen(pathname, "rb");
		if (fp != NULL) {
			fclose(fp);
			goto exit_no_error;
		}
		/* generate all the glyphs available in the set entry, except the surrogates */
		context->unlock_set_entry();
		for (i=0x0000; i<0x10000; i+=0x40) {
			if (i == 0xd800) {
				xprintf(",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
				i = 0xe000;
			}
			if ((i & 0xfff) == 0)
				xprintf("\n[%04x-%04x] -> ", i, i+4095);
			xprintf(".");
			for (j=0; j<0x40; j++) {
				if (i+j == 0) continue;
				ref_array[j] = i+j;
			}
			fc_get_escapement(context, &e_table, ref_array, 0x40, &draw);
		}
		xprintf("\n\n");
		se = context->lock_set_entry(1, &pipo_hit_ref);
		if (context->file_id < file_table_count) {
			fe = file_table+context->file_id;
			length = strlen(fe->filename);
			memcpy(filename, fe->filename, length);
			/* remove suffix */
			for (i=length-1; i>0; i--)
				if (filename[i] == '.') {
					length = i;
					break;
				}
			/* add size terminator */
			filename[length++] = '_';
			sprintf(filename+length, "%d", context->size());			
			/* create the file entry */
			*fet = fc_set_file(fe->dir_id, filename, -1);
			*dir_id = fe->dir_id;
		}
		else goto exit_bad_error;
		/* write the new version of the tuned font file */
		fc_get_pathname(pathname, *fet);
		fp = fopen(pathname, "wb");
		xprintf("Try to open : %s (%x)\n", pathname, fp);
		if (fp == NULL) {
			xprintf("open failed !\n");
			goto exit_bad_error;
		}
		/* write family and style names */
		family_name = fc_get_family_name(context->family_id());
		style_name = fc_get_style_name(context->family_id(), context->style_id());
		family_name_length = strlen(family_name);
		style_name_length = strlen(style_name);
		name_length = family_name_length+style_name_length+2;
		if (fseek(fp, 4+sizeof(fc_tuned_file_header), SEEK_SET) != 0)
			goto exit_bad_file;
		if (fwrite(family_name, 1, family_name_length+1, fp) != family_name_length+1)
			goto exit_bad_file;
		if (fwrite(style_name, 1, style_name_length+1, fp) != style_name_length+1)
			goto exit_bad_file;
		/* create a short version of the hash table coding only defined characters */
		count = 0;
		for (i=0; i<=se->char_hmask; i++)
			if (se->hash_char[i].offset >= 0)
				count++;
		j = 0;
		xprintf("Count of defined char : %d\n", count);
		count += (count+2)>>2;
		while (count > 0) {
			count >>= 1;
			j++;
		}
		count = 1<<j;
		xprintf("Store using : %d\n", count);
		htable = (fc_hset_item*)grMalloc(sizeof(fc_hset_item)*count,"fc:htable",MALLOC_CANNOT_FAIL);
		for (i=0; i<count; i++)
			htable[i].offset = -1;
		for (i=0; i<=se->char_hmask; i++)
			if (se->hash_char[i].offset >= 0) {
				j = i&(count-1);
				while (htable[j].offset != -1)
					j = (j+1)&(count-1);
				htable[j].code[0] = se->hash_char[i].code[0];
				htable[j].code[1] = se->hash_char[i].code[1];
				htable[j].offset = se->hash_char[i].offset;
			}
		/* save copies of the bitmap buffers */
		j = 4+sizeof(fc_tuned_file_header)+name_length+sizeof(fc_hset_item)*count;
		fseek(fp, j, SEEK_SET);
		for (i=0; i<count; i++)
			if (htable[i].offset >= 0) {
				ch = (fc_char*)(se->char_buf+htable[i].offset);
				htable[i].offset = j;
				fc_write32(&ch_buffer.edge.left, *((uint32*)&ch->edge.left));
				fc_write32(&ch_buffer.edge.right, *((uint32*)&ch->edge.right));
				fc_write16(&ch_buffer.bbox.left, ch->bbox.left);
				fc_write16(&ch_buffer.bbox.top, ch->bbox.top);
				fc_write16(&ch_buffer.bbox.right, ch->bbox.right);
				fc_write16(&ch_buffer.bbox.bottom, ch->bbox.bottom);
				fc_write32(&ch_buffer.escape.x_escape, *((uint32*)&ch->escape.x_escape));
				fc_write32(&ch_buffer.escape.y_escape, *((uint32*)&ch->escape.y_escape));
				if (fwrite(&ch_buffer, 1, CHAR_TUNED_HEADER_SIZE, fp) != CHAR_TUNED_HEADER_SIZE)
					goto exit_bad_file;
				j += CHAR_TUNED_HEADER_SIZE;
				length = ((ch->bbox.right-ch->bbox.left+2)>>1)*(ch->bbox.bottom-ch->bbox.top+1);
				if (length > 0) {
					if (fwrite(((char*)ch)+CHAR_HEADER_SIZE, 1, length, fp) != length)
						goto exit_bad_file;
					j += length;
				}
			}
		total_length = j;
		/* save a copy of that new hash table */
		if (fseek(fp, 4+sizeof(fc_tuned_file_header)+name_length, SEEK_SET) != 0)
			goto exit_bad_file;
		for (i=0; i<count; i++) {
			fc_write32(&hset_buffer.offset, htable[i].offset);
			fc_write16(&hset_buffer.code[0], htable[i].code[0]);
			fc_write16(&hset_buffer.code[1], htable[i].code[1]);
			if (fwrite(&hset_buffer, 1, sizeof(fc_hset_item), fp) != sizeof(fc_hset_item))
				goto exit_bad_file;
		}
		grFree(htable);
		/* put tuned font file signature */
		buffer[0] = '|';
		buffer[1] = 'B';
		buffer[2] = 'e';
		buffer[3] = ';';
		fseek(fp, 0, SEEK_SET);
		if (fwrite(buffer, 1, 4, fp) != 4)
			goto exit_bad_file;
		/* put the tuned file header */
		fc_write32(&tfh.total_length, total_length);
		fc_write16(&tfh.family_length, family_name_length);
		fc_write16(&tfh.style_length, style_name_length);
		fc_write32(&tfh.rotation, *((int32*)&context->params()->_rotation));
		fc_write32(&tfh.shear, *((int32*)&context->params()->_shear));
		fc_write32(&tfh.hmask, count-1);
		fc_write16(&tfh.size, context->size());
		tfh.bpp = context->bits_per_pixel();
		tfh.version = 0;
		if (fwrite(&tfh, 1, sizeof(fc_tuned_file_header), fp) != sizeof(fc_tuned_file_header))
			goto exit_bad_file;
		sync();
		fclose(fp);
	}
	else {
		*fet = file_table+context->tuned_file_id;
		fc_get_pathname(pathname, *fet);
	}
 exit_no_error:
	/* unlock the set entry in the cache */
	context->unlock_set_entry();
	/* finally, flush the set out of the cache */
	fc_flush_font_set(context);
	return B_NO_ERROR;
 exit_bad_file:
 	/* close and remove the tuned bitmap file as creation failed */
 	fclose(fp);
 	remove(pathname);
 exit_bad_error:
	/* unlock the set entry in the cache */
	context->unlock_set_entry();
	/* finally, flush the set out of the cache */
	fc_flush_font_set(context);
	return B_ERROR;
}

bool fc_read_set_from_file(FILE *fp, fc_set_entry *set) {
	int               i, count, count0, code0, code1, index, offset, hmask;
	char              buffer[4];
	fc_hset_item      *new_hash, *hash;
	fc_font_processor *processor;

	/* read count. Reallocate hash_table if necessary */
	if (fread(buffer, 1, 4, fp) != 4) return FALSE;	

	count0 = fc_read32(buffer);
	count = count0 + set->char_count;
	hmask = 1;
	while (count >= hmask)
		hmask *= 2;
	hmask = (hmask*2)-1;

	hash = set->hash_char;

	if (hmask > set->char_hmask) {
		new_hash = (fc_hset_item*)grMalloc(sizeof(fc_hset_item)*(hmask+1),"fc:new_hash",MALLOC_CANNOT_FAIL);
		for (i=0; i<=hmask; i++)
			new_hash[i].offset = -1;
		for (i=0; i<=set->char_hmask; i++) {
			if (hash[i].offset == -1)
				continue;
			index = ((((hash[i].code[0]<<3)^(hash[i].code[0]>>2))+hash[i].code[1])) & hmask;
			while (new_hash[index].offset != -1)
				index = (index+1) & hmask;
			new_hash[index].offset = hash[i].offset;
			new_hash[index].code[0] = hash[i].code[0];
			new_hash[index].code[1] = hash[i].code[1];
		}
		atomic_add(&set->params.cache_status()->cur_mem,
				   sizeof(fc_hset_item)*(hmask-set->char_hmask));	        
		set->hash_char = new_hash;
		grFree(hash);
		hash = new_hash;
		set->char_hmask = hmask;
	}

	processor = new fc_font_processor();
	/* read character code and fc_char. Index them in the hash table. */
	for (i=0; i<count0; i++) {
		if (fread(buffer, 1, 4, fp) != 4) goto error;
		code0 = fc_read16(buffer+0);
		code1 = fc_read16(buffer+2);
		index = (((code0<<3)^(code0>>2))+code1)&set->char_hmask;
		while (hash[index].offset != -1) {
			if ((hash[index].code[0] == code0) && (hash[index].code[1] == code1))
				goto char_found;
			index = (index+1)&set->char_hmask;			
		}
		offset = set->char_buf_end;
		if (!fc_read_char_from_file(fp, set, (FC_GET_MEM)fc_get_mem)) goto error;
		set->char_buf_end = (set->char_buf_end+3)&0xfffffffc;
		hash[index].offset = offset;
		hash[index].code[0] = code0;
		hash[index].code[1] = code1;
		set->char_count++;
		processor->process_spacing(set->params.bits_per_pixel(),
								   (fc_char*)(set->char_buf+offset), set);
		continue;
	char_found:
		if (!fc_seek_char_from_file(fp)) goto error;
	}
	delete processor;
	return TRUE;
 error:
	delete processor;
	return FALSE;
	
}

bool fc_write_set_to_file(FILE *fp, fc_set_entry *set) {
	int            i, j, count;
	char           buffer[4];
	fc_char        *fchar;

	/* write count */
	count = 0;
	for (i=0; i<=set->char_hmask; i++)
		if (set->hash_char[i].offset >= 0)
			count++;
	fc_write32(buffer+0, count);
	if (fwrite(buffer, 1, 4, fp) != 4) return FALSE;
	/* write character code and fc_char */
	j = 0;
	for (i=0; i<=set->char_hmask; i++)
		if (set->hash_char[i].offset >= 0) {
			fc_write16(buffer+0, set->hash_char[i].code[0]);
			fc_write16(buffer+2, set->hash_char[i].code[1]);
			if (fwrite(buffer, 1, 4, fp) != 4) return FALSE;
			fchar = (fc_char*)(set->char_buf+set->hash_char[i].offset);
			if (!fc_write_char_to_file(fp, fchar)) return FALSE;
			j++;
		}
	return TRUE;
}

bool fc_read_char_from_file(FILE *fp, fc_set_entry *set, FC_GET_MEM get_mem) {
	int          size_h, size_v, bitmap_size;
	float        f_table[4];
	float        edge_r, edge_l, escape_x, escape_y;
	int16        top, left, right, bottom;
	fc_tuned_char ch;
	fc_char      *fchar;
	
	if (fread(&ch, 1, CHAR_TUNED_HEADER_SIZE, fp) != CHAR_TUNED_HEADER_SIZE)
		return FALSE;
	/* read values, big endian */
	*((uint32*)&f_table[0]) = fc_read32(&ch.edge.right);
	*((uint32*)&f_table[1]) = fc_read32(&ch.edge.left);
	*((uint32*)&f_table[2]) = fc_read32(&ch.escape.x_escape);
	*((uint32*)&f_table[3]) = fc_read32(&ch.escape.y_escape);
	edge_r = f_table[0];
	edge_l = f_table[1];
	escape_x = f_table[2];
	escape_y = f_table[3];
	top = fc_read16(&ch.bbox.top);
	left = fc_read16(&ch.bbox.left);
	right = fc_read16(&ch.bbox.right);
	bottom = fc_read16(&ch.bbox.bottom);
	/* check that the value are reasonnable */
	size_h = right-left+1;
	size_v = bottom-top+1;
	bitmap_size = ((size_h+1)>>1)*size_v;
	if ((size_h < 0) || (size_v < 0) ||
		(bitmap_size > 128*1024) || (bitmap_size < 0) ||
		(left < -512) || (right > 1024) ||
		(top < -1024) || (bottom > 512) ||
		(((edge_r < -2.0) || (edge_r > 2.0) ||
		  (edge_l < -2.0) || (edge_l > 2.0)) &&
		 (edge_l != 1234567.0)) ||
		(escape_x < -1000.0) || (escape_x > 1000.0) ||
		(escape_y < -1000.0) || (escape_y > 1000.0))
		return FALSE;
	if (bitmap_size < 0)
		bitmap_size = 0;
	/* allocate the memory to store the character header and bitmap, fill the header */
	fchar = (fc_char*)(get_mem)(set, bitmap_size+CHAR_HEADER_SIZE);
	fchar->edge.left = edge_l;
	fchar->edge.right = edge_r;
	fchar->escape.x_escape = escape_x;
	fchar->escape.y_escape = escape_y;
	fchar->escape.x_bm_escape = int32(escape_x+.5);
	fchar->escape.y_bm_escape = int32(escape_y+.5);
	fchar->bbox.top = top;
	fchar->bbox.left = left;
	fchar->bbox.right = right;
	fchar->bbox.bottom = bottom;
	fchar->spacing._reserved_ = 0;
	fchar->spacing.char_offset = 0;
	/* read the grayscale bitmap */
	if (bitmap_size > 0)
		if (fread((void*)((char*)fchar+CHAR_HEADER_SIZE), 1, bitmap_size, fp) != bitmap_size)
			memset((void*)((char*)fchar+CHAR_HEADER_SIZE), 0L, bitmap_size);
	return TRUE;
}

bool fc_seek_char_from_file(FILE *fp) {
	int          size_h, size_v, bitmap_size;
	int16        top, left, right, bottom;
	fc_tuned_char ch;
	
	if (fread(&ch, 1, CHAR_TUNED_HEADER_SIZE, fp) != CHAR_TUNED_HEADER_SIZE)
		return FALSE;
	/* read values, big endian */
	top = fc_read16(&ch.bbox.top);
	left = fc_read16(&ch.bbox.left);
	right = fc_read16(&ch.bbox.right);
	bottom = fc_read16(&ch.bbox.bottom);
	/* check that the value are reasonnable */
	size_h = right-left+1;
	size_v = bottom-top+1;
	bitmap_size = ((size_h+1)>>1)*size_v;
	if ((size_h < 0) || (size_v < 0) ||
		(bitmap_size > 128*1024) || (bitmap_size < 0) ||
		(left < -512) || (right > 1024) ||
		(top < -1024) || (bottom > 512))
		return FALSE;
	if (bitmap_size > 0)
		fseek(fp, bitmap_size, SEEK_CUR); 
	return TRUE;
}

bool fc_write_char_to_file(FILE *fp, fc_char *fchar) {
	int          size_h, size_v, bitmap_size;
	fc_tuned_char ch;
	
	/* read values, big endian */
	fc_write32(&ch.edge.right, *((uint32*)&fchar->edge.right));
	fc_write32(&ch.edge.left, *((uint32*)&fchar->edge.left));
	fc_write32(&ch.escape.x_escape, *((uint32*)&fchar->escape.x_escape));
	fc_write32(&ch.escape.y_escape, *((uint32*)&fchar->escape.y_escape));
	fc_write16(&ch.bbox.top, fchar->bbox.top);
	fc_write16(&ch.bbox.left, fchar->bbox.left);
	fc_write16(&ch.bbox.right, fchar->bbox.right);
	fc_write16(&ch.bbox.bottom, fchar->bbox.bottom);
	if (fwrite(&ch, 1, CHAR_TUNED_HEADER_SIZE, fp) != CHAR_TUNED_HEADER_SIZE)
		return FALSE;
	/* check that the value are reasonnable */
	size_h = fchar->bbox.right-fchar->bbox.left+1;
	size_v = fchar->bbox.bottom-fchar->bbox.top+1;
	bitmap_size = ((size_h+1)>>1)*size_v;
	if (bitmap_size > 0)
		if (fwrite(((char*)fchar)+CHAR_HEADER_SIZE, 1, bitmap_size, fp) != bitmap_size)
			return FALSE;
	return TRUE;
}

void fc_purge_set_list(int family_id, int style_id, fc_tuned_entry *te) {
	int           i;
	fc_set_entry  *se;
	
	set_locker->lock_read();
	se = set_table;
	for (i=0; i<set_table_count; i++)
		if (se->char_count >= 0)
			if ((se->params.family_id() == family_id) &&
				(se->params.style_id() == style_id))
				if ((te == 0L) ||
					((se->params.rotation() == te->rotation) &&
					 (se->params.shear() == te->shear) &&
					 (se->params.size() == te->size) &&
					 (se->params.bits_per_pixel() == te->bpp))) {
					draw_locker[i&(FC_LOCKER_COUNT-1)]->lock_write();
					se->invalidate();
//					se->bonus = -FC_HIGH_PRIORITY_BONUS;
					draw_locker[i&(FC_LOCKER_COUNT-1)]->unlock_write();
				}
	set_locker->unlock_read();
}














