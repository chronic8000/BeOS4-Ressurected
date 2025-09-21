/* ++++++++++
	FILE:	font_family.cpp
	REVS:	$Revision: 1.5 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:32:02 PST 1997
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
#include <ByteOrder.h>
#include <Debug.h>

/*****************************************************************/
/* Font family API */ 

bool fc_write_family_table(FILE *fp) {
	int             i, length;
	char            buffer[14];
	
	/* write family count */ 
	fc_write32(buffer, family_table_count);
	if (fwrite(buffer, 1, 4, fp) != 4) return FALSE;
	/* write all the family entry */
	for (i=0; i<family_table_count; i++) {
		if (family_table[i].file_count == 0) {
			buffer[0] = 0;
			buffer[1] = 0;
			if (fwrite(buffer, 1, 2, fp) != 2) return FALSE;
		}
		else {
			length = strlen(family_table[i].name);
			if (family_table[i].extension_status == FC_NOT_EXTENDED)
				length += 4;
			if (length == 0) length = 1;
			fc_write16(buffer, length);
			if (fwrite(buffer, 1, 2, fp) != 2) return FALSE;
			if (fwrite(family_table[i].name, 1, length, fp) != length) return FALSE;
			fc_write32(buffer+0, family_table[i].hash_key);
			fc_write32(buffer+4, family_table[i].file_count);
			fc_write16(buffer+8, family_table[i].enable_count);
			buffer[10] = family_table[i].extension_status;
			buffer[11] = family_table[i]._reserved_[0];
			buffer[12] = family_table[i]._reserved_[1];
			buffer[13] = family_table[i]._reserved_[2];
			if (fwrite(buffer, 1, 14, fp) != 14) return FALSE;
			if (!fc_write_style_table(fp, family_table+i)) return FALSE;
		}
	}
	return TRUE;
}

bool fc_read_family_table(FILE *fp) {
	int             i, length, index;
	char            buffer[14];

	/* read family count */ 
	if (fread(buffer, 1, 4, fp) != 4) return FALSE;
	family_table_count = fc_read32(buffer);
	if ((family_table_count > FC_MAX_FAMILY_COUNT) || (family_table_count == 0)) return FALSE;
	family_table = (fc_family_entry*)grMalloc(sizeof(fc_family_entry)*family_table_count,"fc:family_table",MALLOC_CANNOT_FAIL);
	if (family_table == NULL) return FALSE;
	family_ordered_index = (fc_family_entry**)grMalloc(sizeof(fc_family_entry*)*family_table_count,"fc:family_ordered_index",MALLOC_CANNOT_FAIL);
	if (family_ordered_index == NULL) {
		grFree(family_table);
		return FALSE;
	}
	family_ordered_index_is_valid = false;
	/* read all the family entry */
	style_name = new fc_count_name(8);
	for (i=0; i<family_table_count; i++) {
		family_table[i].name = 0L;
		if (fread(buffer, 1, 2, fp) != 2) goto exit_error;
		length = fc_read16(buffer);
		if (length == 0)
			family_table[i].file_count = 0L;
		else {
			family_table[i].name = (char*)grMalloc(length+1,"fc:family_table.name",MALLOC_CANNOT_FAIL);
			if (family_table[i].name == NULL)
				goto exit_error;
			if (fread(family_table[i].name, 1, length, fp) != length) {
				grFree(family_table[i].name);
				goto exit_error;
			}
			family_table[i].name[length] = 0;
			
			if (fread(buffer, 1, 14, fp) != 14) goto exit_error;
			family_table[i].hash_key = fc_read32(buffer+0);
			family_table[i].file_count = fc_read32(buffer+4);
			family_table[i].enable_count = fc_read16(buffer+8);
			family_table[i].extension_status = buffer[10];
			family_table[i]._reserved_[0] = buffer[11];
			family_table[i]._reserved_[1] = buffer[12];
			family_table[i]._reserved_[2] = buffer[13];

			if (family_table[i].hash_key != fc_hash_family_name(family_table[i].name))
				goto exit_error;
			if (family_table[i].file_count > file_table_count) goto exit_error;
			if (!fc_read_style_table(fp, family_table+i)) goto exit_error;
			if ((family_table[i].extension_status != FC_NOT_EXTENDED) &&
				(family_table[i].extension_status != FC_MASTER_EXTENDED) &&
				(family_table[i].extension_status != FC_SLAVE_EXTENDED)) goto exit_error;
		}
	}
	/* update hash table */
	family_hmask = family_table_count*2-1;
	family_hash = (int*)grMalloc(sizeof(int)*(family_hmask+1),"fc:family_hash",MALLOC_CANNOT_FAIL);
	if (family_hash == NULL) goto exit_error;
	for (i=0; i<=family_hmask; i++)
		family_hash[i] = -1;
	free_family = -1;
	for (i=0; i<family_table_count; i++) {
		if (family_table[i].name == 0) {
			family_table[i].next = free_family;
			free_family = i;
		}
		else {
			index = family_table[i].hash_key & family_hmask;
			while (family_hash[index] != -1)
				index = (index+1) & family_hmask;
			family_hash[index] = i;
		}
	}	
	return TRUE;
 exit_error:
	for (; i>=0; i--) {
		if (family_table[i].name != 0L)
			grFree(family_table[i].name);
		if (i>0)
			fc_release_style_table(family_table+i-1);
	}
	grFree(family_table);
	grFree(family_ordered_index);
	delete style_name;
	return FALSE;
}

void fc_release_family_table() {
	int          i;

	for (i=0; i<family_table_count; i++) {
		if (family_table[i].name != 0L)
			grFree(family_table[i].name);
		if (i>0)
			fc_release_style_table(family_table+i-1);
	}
	grFree(family_ordered_index);
	grFree(family_hash);
	grFree(family_table);
	delete style_name;
}

int fc_register_family(char *name, char *extension) {
	int             i, index, id, length;
	bool			collision;
	font_family		extended_name;
	fc_style_entry  *fs;
	fc_family_entry *ff;

	length = strlen(name);
	if (length > (B_FONT_FAMILY_LENGTH-4)) {
		name[B_FONT_FAMILY_LENGTH-4] = 0;
		length = B_FONT_FAMILY_LENGTH-4;
	}
	strncpy(extended_name, name, length);
	extended_name[length] = '(';
	extended_name[length+1] = extension[0];
	extended_name[length+2] = extension[1];
	extended_name[length+3] = ')';
	extended_name[length+4] = 0;

	/* check if the extended name exists already */
	id = fc_get_family_id(extended_name);
	family_locker->lock_write();
	if (id >= 0) {
		family_table[id].file_count++;
		family_locker->unlock_write();
		return id;
	}
	family_locker->unlock_write();

	/* check if the non extended name is already known */
	collision = false;
	id = fc_get_family_id(name);
	family_locker->lock_write();
	if (id >= 0) {
		if ((family_table[id].name[length+1] == extension[0]) &&
			(family_table[id].name[length+2] == extension[1])) {
			family_table[id].file_count++;
			family_locker->unlock_write();
			return id;
		}
		/* We just detected a new collision on family name ! */
		else {
			collision = true;
			for (i=0; i<family_table_count; i++)
				if (family_table[i].file_count != 0)
					if (strcmp(name, family_table[i].name) == 0)
						if (family_table[i].extension_status == FC_NOT_EXTENDED) {
							family_table[i].extension_status = FC_MASTER_EXTENDED;
							family_table[i].name[length] = '(';
							break;
						}
		}
	}
	/* expand if necessary */
	if (free_family < 0) {
		family_table = (fc_family_entry*)grRealloc(family_table,
						sizeof(fc_family_entry)*2*family_table_count,"fc:family_table",MALLOC_CANNOT_FAIL);
		family_ordered_index = (fc_family_entry**)grRealloc(family_ordered_index,
								sizeof(fc_family_entry*)*2*family_table_count,"fc:family_ordered_index",MALLOC_CANNOT_FAIL);
		family_hash = (int*)grRealloc(family_hash, sizeof(int)*4*family_table_count,"fc:family_hash",MALLOC_CANNOT_FAIL);
		for (i=family_table_count; i<2*family_table_count; i++) {
			family_table[i].next = i+1;
			family_table[i].file_count = 0;
		}
		family_table[2*family_table_count-1].next = -1;
		free_family = family_table_count;
		/* reindex the hash_table */
		for (i=0; i<4*family_table_count; i++)
			family_hash[i] = -1;
		family_table_count *= 2;
		family_hmask = 2*family_table_count-1;
		ff = family_table;
		for (i=0; i<family_table_count/2; i++) {
			if (ff->file_count != 0) {
				index = ff->hash_key & family_hmask;
				while (family_hash[index] >= 0)
					index = (index+1) & family_hmask;
				family_hash[index] = i;
			}
			ff++;
		}
	}
	/* create the new family */
	id = free_family;
	ff = family_table+id;
	free_family = ff->next;
	
	ff->hash_key = fc_hash_family_name(name);
	ff->name = grStrdup(extended_name,"fc:extended_name",MALLOC_CANNOT_FAIL);

	if (collision)
		ff->extension_status = FC_SLAVE_EXTENDED;
	else {
		ff->extension_status = FC_NOT_EXTENDED;
		ff->name[length] = 0;
	}
		
	ff->_reserved_[0] = 0;
	ff->_reserved_[1] = 0;
	ff->_reserved_[2] = 0;
	ff->file_count = 1;
	ff->enable_count = 0;
	/* hash the reference */
	index = ff->hash_key & family_hmask;
	while (family_hash[index] >= 0)
		index = (index+1) & family_hmask;
	family_hash[index] = id;
	/* init style tables */
	ff->style_table_count = 4;
	ff->style_table =
		(fc_style_entry*)grMalloc(sizeof(fc_style_entry)*ff->style_table_count,"fc:style_table",MALLOC_CANNOT_FAIL);
	fs = ff->style_table;
	for (i=0; i<ff->style_table_count; i++) {
		fs->name = 0L;
		fs->status = 0;
		fs++;
	}
	family_locker->unlock_write();
	family_ordered_index_is_valid = false;
	return id;
}

static void fc_qsort(fc_family_entry **list, int32 count) {
	int32				imin, imax;
	char				*ref_name;
	fc_family_entry		*tmp;
	
	if (count <= 1)
		return;
		
	tmp = list[count>>1];
	list[count>>1] = list[0];
	list[0] = tmp;
	
	ref_name = tmp->name;
	imin = 1;
	imax = count-1;
	
	while (imin < imax) {
		if (strcmp(list[imin]->name, ref_name) <= 0)
			imin++;
		else while (imin < imax) {
			if (strcmp(ref_name, list[imax]->name) <= 0)
				imax--;
			else {
				tmp = list[imax];
				list[imax] = list[imin];
				list[imin] = tmp;
				imin++;
				imax--;
				break;
			}
		}
	}
	if (strcmp(list[imin]->name, ref_name) > 0)
		imin--;
	tmp = list[0];
	list[0] = list[imin];
	list[imin] = tmp;
	
	fc_qsort(list, imin);
	fc_qsort(list+imin+1, count-imin-1);
}

int fc_get_family_count(int *ref_id) {
	int             i, cnt;
	fc_family_entry *ff;
	
	family_locker->lock_read();
	if (!family_ordered_index_is_valid) {
		cnt = 0;
		ff = family_table;
		for (i=0; i<family_table_count; i++) {
			if ((ff->file_count != 0) && (ff->enable_count > 0))
				family_ordered_index[cnt++] = ff;
			ff++;
		}
		fc_qsort(family_ordered_index, cnt);
		family_ordered_index_count = cnt;
		family_ordered_index_is_valid = true;
	}
	*ref_id = 0;
	return family_ordered_index_count;
}

void fc_release_family_count() {
	family_locker->unlock_read();
}

char *fc_next_family(int *ref_id, uchar *flag) {
	int         	i;
	char        	*name;
	fc_family_entry	*ff;
	
	if (*ref_id >= family_ordered_index_count)
		name = 0L;
	else {
		ff = family_ordered_index[*ref_id];
		*flag = 0;
		name = ff->name;
		for (i=0; i<ff->style_table_count; i++)
			if (ff->style_table[i].name != 0L) {
				if (ff->style_table[i].tuned_count > 0)
					*flag |= B_HAS_TUNED_FONT;
				if (ff->style_table[i].flags & FC_MONOSPACED_FONT)
					*flag |= B_IS_FIXED;
				if (ff->style_table[i].flags & FC_DUALSPACED_FONT)
					*flag |= B_IS_FULL_AND_HALF_FIXED;
			}
		(*ref_id)++;
	}
	return name;
}

int fc_get_family_id(const char *family_name) {
	int              i, id, index, key;
	fc_family_entry  *ff;
	
	key = fc_hash_family_name(family_name);
	id = -1;
	family_locker->lock_read();
	index = key & family_hmask;
	while (family_hash[index] >= 0) {
		ff = family_table+family_hash[index];
		if (ff->hash_key == key) {
			i = 0;
			while (family_name[i] != 0) {
				if (family_name[i] != ff->name[i])
					goto wrong_family;
				i++;
			}
			if (ff->name[i] != 0) {
				if (ff->extension_status != FC_MASTER_EXTENDED)
					goto wrong_family;
				if ((*(uint32*)(ff->name+i) != B_BENDIAN_TO_HOST_INT32('(TT)')) &&
			 		(*(uint32*)(ff->name+i) != B_BENDIAN_TO_HOST_INT32('(PS)')))
			 		goto wrong_family;
				if (ff->name[i+4] != 0)
					goto wrong_family;
			}
			id = family_hash[index];
			break;
		}
wrong_family:	
		index = (index+1) & family_hmask;
	}
	family_locker->unlock_read();
	return id;
}

static char unknow_family[] = "<unknown family>";

char *fc_get_family_name(int family_id) {
	char     *name;

	if (family_id == 0xffff)
		return unknow_family;
	if (family_id >= family_table_count)
		return unknow_family;
	family_locker->lock_read();
	name = family_table[family_id].name;
	family_locker->unlock_read();
	return name;
}

int fc_get_default_family_id() {
	int              i, id;
	
	id = -1;
	family_locker->lock_read();
	for (i=0; i<family_table_count; i++)
		if (family_table[i].file_count != 0) {
			id = i;
			break;			
		}
	family_locker->unlock_read();
	return id;
}

/*****************************************************************/
/* miscelleanous functions */

int fc_hash_family_name(const char *name) {
	int      key, length;

	length = strlen(name);
	if ((length > 4) &&
		((*(const uint32*)(name+length-4) == B_BENDIAN_TO_HOST_INT32('(TT)')) ||
		 (*(const uint32*)(name+length-4) == B_BENDIAN_TO_HOST_INT32('(PS)'))))
		 length -= 4;
	key = 0;
	for (;length>0 ;length--) {
		key = ((key<<5)&0x7fffffff)|(key>>26);
		key ^= *name;
		name++;
	}
	return key;
}











