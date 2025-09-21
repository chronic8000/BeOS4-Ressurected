/* ++++++++++
	FILE:	font_style.cpp
	REVS:	$Revision: 1.7 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:32:11 PST 1997
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

/*****************************************************************/
/* Font style API */

bool fc_write_style_table(FILE *fp, fc_family_entry *family) {
	int             i, j, length;
	char            buffer[24];
	fc_style_entry  *se;
	
	/* write style count */ 
	fc_write32(buffer, family->style_table_count);
	if (fwrite(buffer, 1, 4, fp) != 4) return false;
	/* write all the style entry */
	for (i=0; i<family->style_table_count; i++) {
		se = family->style_table+i;
		if (se->name == 0L) {
			buffer[0] = 0xff;
			buffer[1] = 0xff;
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
		}
		else {
			length = strlen(se->name);
			fc_write16(buffer, length);
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
			if (fwrite(se->name, 1, length, fp) != (size_t)length) return false;
			fc_write32(buffer+0, (uint32)se->file_id);
			buffer[4] = se->status;
			buffer[5] = se->used;
			buffer[6] = se->flags;
			buffer[7] = se->faces;
			for (j=0; j<8; j++)
				fc_write16(buffer+(8+2*j), se->gasp[j]);
			if (fwrite(buffer, 1, 24, fp) != 24) return false;
			if (!fc_write_tuned_table(fp, se)) return false;
		}
	}
	return true;
}

bool fc_read_style_table(FILE *fp, fc_family_entry *family) {
	int             i, j, length, style_count;
	char            buffer[24];
	char            *tmp;
	fc_style_entry  *se;

	/* read style count */ 
	if (fread(buffer, 1, 4, fp) != 4) return false;
	style_count = fc_read32(buffer);
	if ((style_count > FC_MAX_STYLE_COUNT) || (style_count <= 0)) return false;
	family->style_table = (fc_style_entry*)grMalloc(sizeof(fc_style_entry)*style_count,
													"fc:family.style_table",MALLOC_CANNOT_FAIL);
	if (family->style_table == NULL) return false;
	family->style_table_count = style_count;
	/* read all the style entry */
	for (i=0; i<style_count; i++) {
		se = family->style_table+i;
		se->tuned_hash = 0L;
		se->tuned_table = 0L;
		if (fread(buffer, 1, 2, fp) != 2) goto exit_error;
		length = fc_read16(buffer);
		if (length == 0xffff)
			se->name = 0L;
		else {
			tmp = (char*)grMalloc(length+1,"fc:tmp",MALLOC_CANNOT_FAIL);
			if (tmp == NULL)
				goto exit_error;
			if (fread(tmp, 1, length, fp) != (size_t)length) {
				grFree(tmp);
				goto exit_error;
			}
			tmp[length] = 0;
		    se->name = style_name->register_name(tmp);
			grFree(tmp);
			
			if (fread(buffer, 1, 24, fp) != 24) goto exit_error;
			se->file_id = fc_read32(buffer+0);
			se->status = buffer[4];
			se->used = buffer[5];
			se->flags = buffer[6];
			se->faces = buffer[7];
			for (j=0; j<8; j++)
				se->gasp[j] = fc_read16(buffer+(8+2*j));
			if (se->file_id >= file_table_count) goto exit_error;
			if (!fc_read_tuned_table(fp, se)) goto exit_error;
			if (se->gasp[1] > 3) goto exit_error;
			if (se->gasp[3] > 3) goto exit_error;
			if (se->gasp[5] > 3) goto exit_error;
			if (se->gasp[7] > 3) goto exit_error;
			if ((se->gasp[2] <= se->gasp[0]) &&
				(se->gasp[2] != 0xffff)) goto exit_error;
			if ((se->gasp[4] <= se->gasp[2]) &&
				(se->gasp[4] != 0xffff)) goto exit_error;
			if (se->gasp[6] != 0xffff) goto exit_error;
		}
	}
	return true;
 exit_error:
	for (i--; i>=0; i--) {
		se = family->style_table+i;
		if (se->tuned_table) {
			grFree(se->tuned_table);
			grFree(se->tuned_hash);
		}
	}
	grFree(family->style_table);
	return false;
}

void fc_release_style_table(fc_family_entry *family) {
	int              i;
	fc_style_entry   *se;
	
	for (i=0; i<family->style_table_count; i++) {
		se = family->style_table+i;
		if (se->tuned_table) {
			grFree(se->tuned_table);
			grFree(se->tuned_hash);
		}
	}
	grFree(family->style_table);
}

bool fc_write_tuned_table(FILE *fp, fc_style_entry *style) {
	int             i;
	char            buffer[24];
	fc_tuned_entry  *te;
	
	if (style->tuned_table == 0L) {	
		fc_write32(buffer, 0);
		if (fwrite(buffer, 1, 4, fp) != 4) return false;
	}
	else {
		/* write tuned count */
		fc_write32(buffer, style->tuned_hmask);
		if (fwrite(buffer, 1, 4, fp) != 4) return false;
		/* write all the tuned entry */
		for (i=0; i<(style->tuned_hmask+1)/2; i++) {
			te = style->tuned_table+i;
			if (te->used == 0) {
				buffer[0] = 0;
				if (fwrite(buffer, 1, 1, fp) != 1) return false;
			}
			else {
				buffer[0] = te->used;
				buffer[1] = te->bpp;
				fc_write16(buffer+2, te->size);
				fc_write32(buffer+4, te->hmask);
				fc_write32(buffer+8, (uint32)*((float*)&te->shear));
				fc_write32(buffer+12, (uint32)*((float*)&te->rotation));
				fc_write16(buffer+16, te->offset);
				fc_write16(buffer+18, te->key);
				fc_write32(buffer+20, te->file_id);
				if (fwrite(buffer, 1, 24, fp) != 24) return false;
			}
		}
	}
	return true;	
}

bool fc_read_tuned_table(FILE *fp, fc_style_entry *style) {	
	int             i, index, hmask;
	char            buffer[24];
    uint32          buf32;
	fc_tuned_entry  *te;

	if (fread(buffer, 1, 4, fp) != 4) return false;
	hmask = fc_read32(buffer+0);
	if (hmask == 0) {
		/* no tuned fonts */
		style->tuned_count = 0;
		style->tuned_hmask = 0;
	}
	else {
		/* set tuned count */
		style->tuned_hmask = hmask;
		style->tuned_table = (fc_tuned_entry*)grMalloc(	sizeof(fc_tuned_entry)*(hmask+1)/2,
														"fc:style.tuned_table",MALLOC_CANNOT_FAIL);
		if (style->tuned_table == NULL) goto exit_error;
		/* write all the tuned entry */
		for (i=0; i<(hmask+1)/2; i++) {
			te = style->tuned_table+i;
			if (fread(buffer, 1, 1, fp) != 1) goto exit_error;
			te->used = buffer[0];
			if (te->used) {
				if (fread(buffer, 1, 23, fp) != 23) goto exit_error;
				te->bpp = buffer[0];
				te->size = fc_read16(buffer+1);
				te->hmask = fc_read32(buffer+3);
				buf32 = fc_read32(buffer+7);
				te->shear = *((float*)&buf32);
				buf32 = fc_read32(buffer+11);
				te->rotation = *((float*)&buf32);
				te->offset = fc_read16(buffer+15);
				te->key = fc_read16(buffer+17);
				te->file_id = fc_read32(buffer+19);

				if (te->file_id >= file_table_count) goto exit_error;
				if (te->key != sub_style_key(te->size, &te->rotation, &te->shear, te->bpp))
					goto exit_error;
				if (((te->hmask&(te->hmask+1)) != 0) || (te->hmask < 3)) goto exit_error;
				if (te->bpp >= FC_MAX_BPP) goto exit_error;
				if ((te->size <= 1) || (te->size > FC_BW_MAX_SIZE)) goto exit_error;			
			}
		}
		/* init the hash table, and the tuned count */
		style->tuned_count = 0;
		style->tuned_hash = (uint16*)grMalloc(sizeof(uint16)*(hmask+1),"fc:style.tuned_hash",MALLOC_CANNOT_FAIL);
		if (style->tuned_hash == NULL) goto exit_error;
		for (i=0; i<=hmask; i++)
			style->tuned_hash[i] = 0xffff;
		te = style->tuned_table;
		for (i=0; i<(hmask+1)/2; i++) {
			if (te->used) {
				style->tuned_count++;
				index = te->key & hmask;
				while (style->tuned_hash[index] != 0xffff)
					index = (index+1) & hmask;
				style->tuned_hash[index] = i;
			}
			te++;
		}
	}
	return true;
 exit_error:
 	if (style->tuned_table)
		grFree(style->tuned_table);
	if (style->tuned_hash)
		grFree(style->tuned_hash);
	style->tuned_table = 0L;
	return false;
}

fc_style_entry *fc_set_new_style(fc_family_entry *ff, char *name) {
	int             i, id;
	fc_style_entry  *fs;

	/* look for an empty entry */
	id = -1;
	for (i=0; i<ff->style_table_count; i++)
		if (ff->style_table[i].name == 0L) {
			id = i;
			break;
		}
	/* need to extend the style table */
	if (id == -1) {
		ff->style_table =
			(fc_style_entry*)grRealloc(	ff->style_table,
										sizeof(fc_style_entry)*2*ff->style_table_count,
										"fc:style_table",MALLOC_CANNOT_FAIL);
		fs = ff->style_table+ff->style_table_count;
		for (i=0; i<ff->style_table_count; i++) {
			fs->name = 0L;
			fs->status = 0;
			fs++;
		}
		id = ff->style_table_count;
		ff->style_table_count *= 2;
	}
	/* init the new entry (just the name, no link to files) */
	fs = ff->style_table+id;
	fs->name = style_name->register_name(name);
	fs->file_id = -1;
	fs->status = 0;
	fs->tuned_count = 0;
	fs->tuned_hmask = 0;
	fs->tuned_table = 0L;
	fs->tuned_hash = 0L;
	return fs;
}

int fc_set_style(int family_id, char *name, int file_id,
				 uchar flags, uint16 faces, uint16 *gasp, bool enable) {
	int             id;
	fc_family_entry *ff;
	fc_style_entry  *fs;

	/* check if the style is already registred */
	family_locker->lock_write();
	ff = family_table+family_id;
	if (ff->file_count > 0) {
		fs = ff->style_table;
		for (id=0; id<ff->style_table_count; id++) {
			if (fs->name != 0L)
				if (strcmp(fs->name, name) == 0) {
					if (fs->file_id >= 0)
						goto style_found;
					goto set_main_file;
				}
			fs++;
		}
	}
	/* get a new entry and initialize it */
	fs = fc_set_new_style(ff, name);
	id = fs-ff->style_table;
 set_main_file:
	fs->file_id = file_id;
	fs->flags = flags;
	fs->faces = faces;
	memcpy((char*)fs->gasp, (char*)gasp, sizeof(fs->gasp));
	if (fs->status & FC_STYLE_ENABLE) {
		ff->enable_count++;
		family_ordered_index_is_valid = false;				
		b_font_change_time = system_time();
		set_uid++;
	}
 style_found:
	/* check that the file_id is correct */
	if (fs->file_id != file_id)
		id = -1;
	/* change status */
	else {
		if (enable) {
			if ((fs->status & FC_STYLE_ENABLE) == 0) {
				fs->status |= FC_STYLE_ENABLE;
				ff->enable_count++;
				family_ordered_index_is_valid = false;				
				b_font_change_time = system_time();
				set_uid++;
			}
		}
		else {
			if (fs->status & FC_STYLE_ENABLE) {
				fs->status &= ~FC_STYLE_ENABLE;
				ff->enable_count--;
				family_ordered_index_is_valid = false;				
				b_font_change_time = system_time();
				set_uid++;
			}
		}			
	}
	family_locker->unlock_write();
	return id;
}

int fc_add_sub_style(int family_id, char *name, fc_tuned_entry *te) {
	int             i, id, index, ncnt, ocnt;
	ushort          key;
	fc_style_entry  *fs;
	fc_tuned_entry  *te2;
	fc_family_entry *ff;

	/* check if the style is already registred */
	family_locker->lock_write();
	ff = family_table+family_id;
	if (ff->file_count > 0) {
		fs = ff->style_table;
		for (id=0; id<ff->style_table_count; id++) {
			if (fs->name != 0L)
				if (strcmp(fs->name, name) == 0)
					goto style_found;
			fs++;
		}
	}
	/* get a new entry and initialize it */
	fs = fc_set_new_style(ff, name);
	id = fs-ff->style_table;
 style_found:
	/* check if the sub-style is already registred */
	key = sub_style_key(te->size, &te->rotation, &te->shear, te->bpp);
	if (fs->tuned_table != 0L) {
		/* sub_style already in ? */
		index = key & fs->tuned_hmask;
		while ((i = fs->tuned_hash[index]) != 0xffff) {
			te2 = fs->tuned_table+i;
			if (te2->key == key)				
				if ((te2->rotation == te->rotation) &&
					(te2->shear == te->shear) &&
					(te2->size == te->size) &&
					(te2->bpp == te->bpp)) {
					family_locker->unlock_write();
					xprintf("Sub style exists already\n");
					return -1;
				}
			index = (index+1) & fs->tuned_hmask;
		}
		/* is there a sub_style entry left ? */
		te2 = fs->tuned_table;
		for (i=0; i< (fs->tuned_hmask+1)/2; i++) {
			if (te2->used == 0)
				goto new_sub_style;
			te2++;
		}		
	}
	/* need to create or realloc the sub_style table */
	ocnt = (fs->tuned_hmask+1)/2;
	ncnt = fs->tuned_hmask+1;
	fs->tuned_hmask = ncnt*2-1;
	fs->tuned_table = (fc_tuned_entry*)grRealloc(	fs->tuned_table, sizeof(fc_tuned_entry)*ncnt,
													"fc:tuned_table",MALLOC_CANNOT_FAIL);
	fs->tuned_hash = (uint16*)grRealloc(	fs->tuned_hash, sizeof(uint16)*2*ncnt,
											"fc:tuned_hash",MALLOC_CANNOT_FAIL);
	for (i=ocnt; i<ncnt; i++)
		fs->tuned_table[i].used = 0;
	for (i=0; i<2*ncnt; i++)
		fs->tuned_hash[i] = 0xffff;
	te2 = fs->tuned_table;
	for (i=0; i<ocnt; i++) {
		if (te2->used) {
			index = te2->key & fs->tuned_hmask;
			while (fs->tuned_hash[index] != 0xffff)
				index = (index+1) & fs->tuned_hmask;
			fs->tuned_hash[index] = i;
		}
		te2++;
	}	
 new_sub_style:
	/* init the new sub-style entry */
	te2->file_id  = te->file_id;
	te2->key      = key;
	te2->offset   = te->offset;
	te2->rotation = te->rotation;
	te2->shear    = te->shear;
	te2->hmask    = te->hmask;
	te2->bpp      = te->bpp;
	te2->used     = 1;
	te2->size     = te->size;
	fs->tuned_count++;
	index = te2->key & fs->tuned_hmask;
	while (fs->tuned_hash[index] != 0xffff)
		index = (index+1) & fs->tuned_hmask;
	fs->tuned_hash[index] = te2 - fs->tuned_table;

	family_locker->unlock_write();
	return id;
}

void fc_do_clean_style_and_family(fc_style_entry *fs, fc_family_entry *ff) {
	int          i, index;

	/* if the style is define, check if we need to remove it */
	if (fs != 0L) {
		if ((fs->tuned_count == 0) && (fs->file_id == -1)) {
			/* remove/disable the style if there isn't any tuned file */
			style_name->unregister_name(fs->name);
			fs->name = 0L;
			/* trash the tuned tables */
			if (fs->tuned_table == 0L) {
				grFree(fs->tuned_table);
				grFree(fs->tuned_hash);
			}
		}
	}
	/* check if we need to remove the family */
	ff->file_count--;
	if (ff->file_count == 0) {
		/* free the style table and the name of the family */
		grFree(ff->style_table);
		grFree(ff->name);
		/* add the family entry in the free family list */
		ff->next = free_family;
		free_family = ff-family_table;
		/* rehash the hash table */
		for (i=0; i<2*family_table_count; i++)
			family_hash[i] = -1;
		ff = family_table;
		for (i=0; i<family_table_count; i++) {
			if (ff->file_count != 0) {
				index = ff->hash_key & family_hmask;
				while (family_hash[index] >= 0)
					index = (index+1) & family_hmask;
				family_hash[index] = i;
			}
			ff++;
		}
	}
}

void fc_remove_style(int family_id, int style_id) {
	fc_style_entry  *fs;
	fc_family_entry *ff;

	family_locker->lock_write();
	ff = family_table+family_id;

	if (style_id >= 0) {
		fs = ff->style_table+style_id;
		fs->file_id = -1;
		if (fs->status & FC_STYLE_ENABLE) {
			ff->enable_count--;
			family_ordered_index_is_valid = false;				
			b_font_change_time = system_time();
			set_uid++;
		}		
		fc_purge_set_list(family_id, style_id, 0L);
		fc_do_clean_style_and_family(fs, ff);
	}
	else
		fc_do_clean_style_and_family(0L, ff);

	family_locker->unlock_write();
}

void fc_remove_sub_style(fc_file_entry *fe, int file_id) {
	int             i, index;
	fc_tuned_entry  *te;
	fc_style_entry  *fs;
	fc_family_entry *ff;

	/* check that the family is defined */
	if (fe->family_id == 0xffff) return;
	
	family_locker->lock_write();
	ff = family_table+fe->family_id;
	if (fe->style_id != 0xffff) {
		/* look for the tuned entry of this file */
		fs = ff->style_table+fe->style_id;
		te = fs->tuned_table;
		for (i=0; i<(fs->tuned_hmask+1)/2; i++) {
			if (te->used && (te->file_id == file_id)) {
				/* trash all the set using that style */
				fc_purge_set_list(fe->family_id, fe->style_id, te);
				/* free that tuned entry */
				te->used = 0;
				fs->tuned_count--;
				/* rehash the tuned hash table */
				for (i=0; i<=fs->tuned_hmask; i++)
					fs->tuned_hash[i] = 0xffff;
				te = fs->tuned_table;
				for (i=0; i<(fs->tuned_hmask+1)/2; i++) {
					if (te->used) {
						index = te->key & fs->tuned_hmask;
						while (fs->tuned_hash[index] != 0xffff)
							index = (index+1) & fs->tuned_hmask;
						fs->tuned_hash[index] = i;
					}
					te++;
				}
				/* free old styles and families */
				fc_do_clean_style_and_family(fs, ff);
				family_locker->unlock_write();
				return;
			}
			te++;
		}
	}
	xprintf("Try to remove a sub_style that doesn't exist !");
	family_locker->unlock_write();
	return;
}

int fc_get_style_count(int family_id, int *ref_id) {
	int             i, count;
	fc_style_entry  *fs;
	fc_family_entry *ff;
	
	family_locker->lock_read();
	count = 0;
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if ((ff->file_count > 0) && (ff->enable_count > 0)) {
			fs = ff->style_table;
			for (i=0; i<ff->style_table_count; i++) {
				if ((fs->name != 0) && ((fs->status & FC_STYLE_ENABLE) != 0)) {
					if (count == 0)
						*ref_id = i;
					count++;
				}
				fs++;
			}
		}
	}
	return count;
}

void fc_release_style_count() {
	family_locker->unlock_read();
}

char *fc_next_style(int family_id, int *ref_id, uchar *flag, uint16 *face) {
	int             id;
	char            *name;
	fc_style_entry  *fs;
	fc_family_entry *ff;

	ff = family_table+family_id;
	fs = ff->style_table;
	id = *ref_id;
	*flag = 0;
	*face = 0;
	if (id < ff->style_table_count) {
		name = fs[id].name;
		if (fs[id].tuned_count > 0)
			*flag |= B_HAS_TUNED_FONT;
		if (fs[id].flags & FC_MONOSPACED_FONT)
			*flag |= B_IS_FIXED;
		if (fs[id].flags & FC_DUALSPACED_FONT)
			*flag |= B_IS_FULL_AND_HALF_FIXED;
		*face = fs[id].faces;
		id++;
		while (id < ff->style_table_count) {
			if ((fs[id].name != 0) && ((fs[id].status & FC_STYLE_ENABLE) != 0))
				break;
			id++;
		}
		*ref_id = id;
	}
	else
		name = 0L;
	return name;
}

int fc_get_style_id(int family_id, const char *style_name) {
	int              i, id;
	fc_style_entry   *fs;
	fc_family_entry  *ff;

	id = -1;
	family_locker->lock_read();
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if (ff->file_count > 0) {
			fs = ff->style_table;
			for (i=0; i<ff->style_table_count; i++) {
				if (fs->name != 0L)
					if (strcmp(fs->name, style_name) == 0) {
						id = i;
						break;
					}
				fs++;
			}
		}
	}
	family_locker->unlock_read();
	return id;
}

static char unknow_style[] = "<unknown style>";

char *fc_get_style_name(int family_id, int style_id) {
	char            *name;
	fc_family_entry *ff;

	name = unknow_style;;
	if ((family_id >= 0) && (style_id >= 0)) {
		family_locker->lock_read();
		if (family_id < family_table_count) {
			ff = family_table+family_id;
			if (ff->file_count > 0)
				if (style_id < ff->style_table_count)
					name = ff->style_table[style_id].name;
		}
		family_locker->unlock_read();
	}
	return name;
}

uint fc_get_style_file_flags_and_faces(int family_id, int style_id, uint16 *faces) {
	int				file_id;
	uint            flags;
	fc_family_entry *ff;

	flags = PF_INVALID;
	*faces = 0;
	if ((family_id >= 0) && (style_id >= 0)) {
		family_locker->lock_read();
		if (family_id < family_table_count) {
			ff = family_table+family_id;
			if (ff->file_count > 0)
				if (style_id < ff->style_table_count) {
					flags = 0x0000;
					if (ff->style_table[style_id].tuned_count > 0)
						flags |= PF_HAS_TUNED_FONT;
					if (ff->style_table[style_id].flags & FC_MONOSPACED_FONT)
						flags |= PF_IS_FIXED;
					if (ff->style_table[style_id].flags & FC_DUALSPACED_FONT)
						flags |= PF_IS_HIROSHI_FIXED;
					*faces = ff->style_table[style_id].faces;
					file_id = ff->style_table[style_id].file_id;
					family_locker->unlock_read();
					fc_lock_file();
					if (file_table[file_id].file_type == FC_TYPE1_FONT)
						flags |= B_POSTSCRIPT_TYPE1_WINDOWS;
					else if (file_table[file_id].file_type == FC_STROKE_FONT)
						flags |= B_BITSTREAM_STROKE_WINDOWS;
					fc_unlock_file();
					return flags;
				}
		}
		family_locker->unlock_read();
	}
	return flags;
}

int fc_get_default_style_id(int family_id) {
	int              i, id;
	fc_style_entry   *fs;
	fc_family_entry  *ff;

	id = -1;
	family_locker->lock_read();
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if (ff->file_count > 0) {
			fs = ff->style_table;
			for (i=0; i<ff->style_table_count; i++) {
				if (fs->name != 0L) {
					id = i;
					if (fs->faces == 0x40)
						goto found_it;
				}
				fs++;
			}
			fs = ff->style_table;
			for (i=0; i<ff->style_table_count; i++) {
				if (fs->name != 0L) {
					id = i;
					if ((strcmp(fs->name, "Plain") == 0) ||
						(strcmp(fs->name, "Regular") == 0) ||
						(strcmp(fs->name, "Roman") == 0))
						break;
				}
				fs++;
			}
		found_it:;
		}
	}
	family_locker->unlock_read();
	return id;
}

int fc_get_closest_face_style(int family_id, int faces) {
	int              i, val, id, d, d_max;
	fc_style_entry   *fs;
	fc_family_entry  *ff;

	id = -1;
	d_max = 32;
	family_locker->lock_read();
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if (ff->file_count > 0) {
			fs = ff->style_table;
			for (i=0; i<ff->style_table_count; i++) {
				if (fs->name != 0L) {
					val = (fs->faces ^ faces);
					d = 0;
					while (val > 0) {
						d += val & 1;
						val >>= 1;
					}
					if (d < d_max) {
						d_max = d;
						id = i;
					}
				}
				fs++;
			}
		}
	}
	family_locker->unlock_read();
	return id;
}

void fc_get_style_bbox_and_blocks(int family_id, int style_id, float *bbox, uint32 *blocks) {
	int				i;
	int32			file_id;	
	fc_family_entry *ff;

	if ((family_id >= 0) && (style_id >= 0)) {
		family_locker->lock_read();
		if (family_id < family_table_count) {
			ff = family_table+family_id;
			if (ff->file_count > 0)
				if (style_id < ff->style_table_count) {
					file_id = ff->style_table[style_id].file_id;
					family_locker->unlock_read();
					fc_lock_file();
					for (i=0; i<4; i++)
						bbox[i] = file_table[file_id].bbox[i];
					for (i=0; i<3; i++)
						blocks[i] = file_table[file_id].blocks[i];
					blocks[3] = 0L;
					fc_unlock_file();
					return;
				}
		}
		family_locker->unlock_read();
	}
	bbox[0] = bbox[1] = 0.0;
	bbox[2] = bbox[3] = -1.0;
	blocks[0] = 0L;
	blocks[1] = 0L;
	blocks[2] = 0L;
	blocks[3] = 0L;
	return;
}

int fc_get_tuned_count(int family_id, int style_id) {
	int             count;
	fc_family_entry *ff;
	
	count = 0;
	family_locker->lock_read();
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if ((ff->file_count > 0) && (style_id < ff->style_table_count))
			if (ff->style_table[style_id].name != 0L)
				count = ff->style_table[style_id].tuned_count;
	}
	family_locker->unlock_read();
	return count;
}

int fc_next_tuned(int family_id, int style_id, int ref_id, fc_tuned_font_info *info) {
	fc_tuned_entry	*ft;
	fc_style_entry  *fs;
	fc_family_entry *ff;
	
	family_locker->lock_read();
	info->size = 0.0;
	info->shear = 0.0;
	info->rotation = 0.0;
	info->flags = 0;
	info->face = 0;
	if ((family_id >= 0) && (family_id < family_table_count)) {
		ff = family_table+family_id;
		if ((ff->file_count > 0) && (style_id < ff->style_table_count))
			if (ff->style_table[style_id].name != 0L) {
				fs = ff->style_table+style_id;
				ft = fs->tuned_table+ref_id;
				while (ft->used == 0) {
					ref_id++;
					ft++;
					if (ref_id >= fs->tuned_count)
						goto end_of_list;
				}
				info->size = (float)ft->size;
				info->shear = ft->shear;
				info->rotation = ft->rotation;
				info->flags = fs->flags;
				info->face = fs->faces;
				ref_id++;
			}
	}
end_of_list:
	family_locker->unlock_read();
	return ref_id;
}
