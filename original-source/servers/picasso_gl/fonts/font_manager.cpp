/* ++++++++++
	FILE:	font_manager.cpp
	REVS:	$Revision: 1.10 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:40:47 PST 1997
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
/* Font dir API */

/*****************   IMPORTANT WARNING    ****************/

/* the font file and dir functions DO NOT HAVE TO LOCK the file benaphore
   AT ALL TIME. They ONLY need to own it when then want TO MODIFY data in
   the dir or the file font lists. The only other client accessing those
   list can access it read only, so that's enough. Another benaphore should
   assure that only one thread is going into any of those functions (except
   get_pathname) at the same time. */

bool fc_write_dir_table(FILE *fp) {
	int         i, length;
	char        buffer[6];

	/* write version id */
	fc_write32(buffer, FC_FONT_FILE_LIST_ID);
	if (fwrite(buffer, 1, 4, fp) != 4) return false;
	/* write dir count */ 
	fc_write32(buffer, dir_table_count);
	if (fwrite(buffer, 1, 4, fp) != 4) return false;
	/* write all the dir entry */
	for (i=0; i<dir_table_count; i++) {
		if (dir_table[i].dirname == 0L) {
			buffer[0] = 0;
			buffer[1] = 0;
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
		}
		else {
			length = strlen(dir_table[i].dirname);
			fc_write16(buffer, length);
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
			if (fwrite(dir_table[i].dirname, 1, length, fp) != (size_t)length) return false;
			buffer[0] = dir_table[i].status;
			buffer[1] = dir_table[i].type;
			fc_write32(buffer+2, (uint32)dir_table[i].mtime);
			if (fwrite(buffer, 1, 6, fp) != 6) return false;
		}
	}
	return true;
}

bool fc_read_dir_table(FILE *fp) {
	int         i, length, dir_count;
	char        buffer[6];

	/* read dir count */ 
	if (fread(buffer, 1, 4, fp) != 4) return false;
	if (fc_read32(buffer) != FC_FONT_FILE_LIST_ID) return false;
	/* read dir count */ 
	if (fread(buffer, 1, 4, fp) != 4) return false;
	dir_count = fc_read32(buffer);
	if ((dir_count > FC_MAX_DIR_COUNT) || (dir_count == 0)) return false;
	dir_table = (fc_dir_entry*)grMalloc(sizeof(fc_dir_entry)*dir_count,"fc:dir_table",MALLOC_CANNOT_FAIL);
	dir_table_count = dir_count;
	/* read all the dir entry */
	for (i=0; i<dir_table_count; i++) {
		dir_table[i].dirname = 0L;
		if (fread(buffer, 1, 2, fp) != 2) goto exit_error;
		length = fc_read16(buffer);
		if (length != 0) {
			dir_table[i].dirname = (char*)grMalloc(length+1,"fc:dir_table.dirname",MALLOC_CANNOT_FAIL);
			if (fread(dir_table[i].dirname, 1, length, fp) != (size_t)length) return false;
			dir_table[i].dirname[length] = 0;
			if (fread(buffer, 1, 6, fp) != 6) goto exit_error;
			dir_table[i].status = buffer[0];
			dir_table[i].type = buffer[1];
			dir_table[i].mtime = (time_t)fc_read32(buffer+2);
			if (dir_table[i].status >= FC_MAX_DIR_STATUS) goto exit_error;
			if (dir_table[i].type >= FC_MAX_DIR_TYPE) goto exit_error;
			if (dir_table[i].status == 0) goto exit_error;
			if (dir_table[i].type == 0) goto exit_error;
		}
	}
	return true;
 exit_error:
    for (; i>=0; i--)
		if (dir_table[i].dirname)
			grFree(dir_table[i].dirname);
	grFree(dir_table);
	return false;
}

void fc_release_dir_table() {
	int        i;

	for (i=0; i<dir_table_count; i++)
		if (dir_table[i].dirname)
			grFree(dir_table[i].dirname);
	grFree(dir_table);
}

int fc_get_dir_count(int *ref_id) {
	int          i, cnt;
	fc_dir_entry *de;
	
	cnt = 0;
	de = dir_table;
	for (i=0; i<dir_table_count; i++) {
		if (de->dirname != 0) {
			if (cnt == 0)
				*ref_id = i;
			cnt++;
		}
		de++;
	}
	return cnt;
}

char *fc_next_dir(int *ref_id, uchar *flag, int *dir_id, uchar *type) {
	int         id;
	char        *name;

	id = *ref_id;
	if (id < dir_table_count) {
		*dir_id = id;
		*flag = dir_table[id].status;
		*type = dir_table[id].type;
		name = dir_table[id].dirname;
		id++;
		while (id < dir_table_count) {
			if (dir_table[id].dirname != 0)
				break;
			id++;
		}
		*ref_id = id;
	}
	else {
		name = 0L;
		*flag = 0;
		*type = 0;
	}
	return name;
}

long fc_set_dir(char *name, int enable, int type) {
	int               i, id;
	struct stat       stat_buf;
	fc_dir_entry      *de;
	fc_file_entry     *fe;
	
	/* check if it's already in the list */
	for (id=0; id<dir_table_count; id++)
		if (dir_table[id].dirname != 0L)
			if (strcmp(dir_table[id].dirname, name) == 0) {
				de = dir_table+id;
				goto dir_found;
			}
	/* if it's a new directory, check its existence and type */
	if (stat(name, &stat_buf) != 0)
		return B_ERROR;
	if (!S_ISDIR(stat_buf.st_mode))
		return B_ERROR;
	/* look for an empty entry, and initialise it */
	fc_lock_file();
	for (id=0; id<dir_table_count; id++)
		if (dir_table[id].dirname == 0L)
			goto entry_found;
	/* extend the dir table */
	family_locker->lock_write();
	dir_table = (fc_dir_entry*)grRealloc(dir_table, sizeof(fc_dir_entry)*dir_table_count*2,"fc:dir_table",MALLOC_CANNOT_FAIL);
	for (i=dir_table_count; i<2*dir_table_count; i++)
		dir_table[i].dirname = 0L;
	id = dir_table_count;
	dir_table_count *= 2;
	family_locker->unlock_write();
 entry_found:
	de = dir_table+id;
	de->dirname = grStrdup(name,"fc:dirname",MALLOC_CANNOT_FAIL);
	de->status = 0;
	de->mtime = 0;
	if (type != 0)
		de->type = type;
	else
		de->type = FC_USER_FOLDER;
	fc_unlock_file();
	/* check if it's currently accessible */
 dir_found:
	if (stat(name, &stat_buf) != 0)
		enable = 0;
	/* change enable status, if needed */
	if ((enable && ((de->status & FC_DIR_ENABLE) == 0)) ||
		((!enable) && (de->status & FC_DIR_ENABLE))) {
		/* update dir status */
		fc_lock_file();
		if (enable)
			enable = FC_DIR_ENABLE;
		else
			enable = 0;
		de->status = (de->status & (~FC_DIR_ENABLE)) | enable;
		fc_unlock_file();
		/* update file status */
		fe = file_table;
		for (i=0; i<file_table_count; i++) {
			if ((fe->filename != 0L) && (fe->dir_id == id))
				fc_set_file_status(fe, enable, FC_DIR_ENABLE);
			fe++;
		}
	}
	return B_NO_ERROR;
}

long fc_remove_dir(char *name) {
	int           i, j, index;
	fc_dir_entry  *de;
	fc_file_entry *fe;

	for (i=0; i<dir_table_count; i++)
	  if (dir_table[i].dirname != NULL)
		if (strcmp(dir_table[i].dirname, name) == 0) {
			de = dir_table+i;
			/* remove all the files in this directory */
			fe = file_table;
			for (j=0; j<file_table_count; j++) {
				if ((fe->filename != 0L) && (fe->dir_id == i))
					fc_remove_file(fe);
				fe++;
			}
			/* reindex the hash table */
			fc_lock_file();
			for (j=0; j<2*file_table_count; j++)
				file_hash[j] = -1;
			for (j=0; j<file_table_count; j++)
				if (file_table[j].filename != 0L) {
					index = file_table[j].key & file_hmask;
					while (file_hash[index] >= 0)
						index = (index+1) & file_hmask;
					file_hash[index] = j;
				}
			/* free the dir entry name */
			grFree(de->dirname);
			de->dirname = 0L;
			fc_unlock_file();
			return B_NO_ERROR;
		}
	/* unknown dir */
	return B_ERROR;
}

void fc_dump_cache_state(char *filename) {
	int		i, j, k;
	FILE	*fp;
	
	fp = fopen(filename, "w");
	if (fp == NULL)
		return;
	for (i=0; i<file_table_count; i++) {
		if (file_table[i].filename != NULL) {
_sPrintf("file_table[%d].filename : %08x\n", i, file_table[i].filename);
			fprintf(fp, "File %s : dir_id:%d, status:%d, type:%d, family:%d, style:%d\n",
						file_table[i].filename, file_table[i].dir_id,
						file_table[i].status, file_table[i].file_type,
						file_table[i].family_id, file_table[i].style_id);
		}
	}
	for (i=0; i<family_table_count; i++) {
		if (family_table[i].file_count > 0) {
			fprintf(fp, "Family[%d] %s : key:%08x, %d file(s), %d style(s), enable_count:%d, ext_status:%d\n", i,
						family_table[i].name,
						family_table[i].hash_key,
						family_table[i].file_count,
						family_table[i].style_table_count,
						family_table[i].enable_count,
						family_table[i].extension_status);
			for (j=0; j<family_table[i].style_table_count; j++) {
				if (family_table[i].style_table[j].name != NULL) {
					fprintf(fp, "  Style[%d] %s : file:%ld, status:%d, used:%d, "
								"flags:%d, faces:%x, count:%d, hmask:%d\n", j,
								family_table[i].style_table[j].name,
								family_table[i].style_table[j].file_id,
								family_table[i].style_table[j].status,
								family_table[i].style_table[j].used,
								family_table[i].style_table[j].flags,
								family_table[i].style_table[j].faces,
								family_table[i].style_table[j].tuned_count,
								family_table[i].style_table[j].tuned_hmask);
					if (family_table[i].style_table[j].tuned_table != 0L) {
						for (k=0; k<family_table[i].style_table[j].tuned_count; k++) {
							fprintf(fp, "    Tuned[%d] : file:%d, key:%08x, offset:%d, rot:%f, "
										"shear:%f, hmask:%08lx, size:%d, bpp:%d, used:%d\n", k,
										family_table[i].style_table[j].tuned_table[k].file_id,
										family_table[i].style_table[j].tuned_table[k].key,
										family_table[i].style_table[j].tuned_table[k].offset,
										family_table[i].style_table[j].tuned_table[k].rotation,
										family_table[i].style_table[j].tuned_table[k].shear,
										family_table[i].style_table[j].tuned_table[k].hmask,
										family_table[i].style_table[j].tuned_table[k].size,
										family_table[i].style_table[j].tuned_table[k].bpp,
										family_table[i].style_table[j].tuned_table[k].used);
						}
					}
				}
			}
		}
	}
	fclose(fp);
}

void fc_scan_all_dir(bool force_check) {
	int            i, dir_id, enable;
	DIR            *dp;
	struct stat    stat_buf;
	fc_dir_entry   *de;
	fc_file_entry  *fe;
	struct dirent  *d_ent;

	fc_flush_open_file_cache();
	for (i=0; i<dir_table_count; i++)
		dir_table[i].w_status = 2;
	for (enable=0; enable<2; enable++) {
		de = dir_table;
		for (dir_id=0; dir_id<dir_table_count; dir_id++) {
			if (de->dirname != 0L) {
				/* if the directory can not be found, then disable it */
				if (stat(de->dirname, &stat_buf) != 0) {
					xprintf("Dir no available : %s\n", de->dirname);
					fc_set_dir(de->dirname, false);
					de->w_status = 0; 
					/* remove all files in that disappeared folder */
					fe = file_table;
					for (i=0; i<file_table_count; i++) {
						if ((fe->filename != 0) && (fe->dir_id == dir_id))
							fc_remove_file(fe);
						fe++;
					}
					goto next_dir;
				}
				/* scan all the files in the directory */
				if ((stat_buf.st_mtime != de->mtime) || force_check || (de->w_status == 1)) {
					de->mtime = stat_buf.st_mtime;
					dp = opendir(de->dirname);
					xprintf("Enter scan loop...\n");
					int    cnt = 0;
					while ((d_ent = readdir(dp)) != NULL) {
						if (d_ent->d_name[0] == '.') {
							if (d_ent->d_name[1] == 0)
								continue;
							if ((d_ent->d_name[1] == '.') && (d_ent->d_name[2] == 0))
								continue;
						}
						xprintf("Loop %3d ", cnt++);
						fc_set_file(dir_id, d_ent->d_name,
									(enable)?FC_ANALYSE_FILE:FC_CHECK_FILE);
					}
					xprintf("Exit scan loop...\n");
					closedir(dp);
					/* remove the old files that are no longer scanned */
					fe = file_table;
					for (i=0; i<file_table_count; i++) {
						if ((fe->filename != 0) && (fe->dir_id == dir_id)) {
							if (fe->status & FC_FILE_SCANNED) {
								fc_lock_file();
								fe->status &= ~FC_FILE_SCANNED;
								fc_unlock_file();
							}
							else
								fc_remove_file(fe);
						}
						fe++;
					}
					de->w_status--;
					fc_set_dir(de->dirname, true);
					goto next_dir;
				}
				/* nothing to be checked in that directory */
 				xprintf("No changes in dir : %s\n", de->dirname);
				fc_set_dir(de->dirname, true);
	 			de->w_status = 0;
			}
		next_dir:
			de++;
		}
	}
	
#if 0	
	for (i=0; i<family_table_count; i++) {
		int   j; 
		 
		if (family_table[i].file_count > 0) {
			xprintf("[%d] family:%s, f_count:%d, e_count:%d, s_count:%d\n", i,
					family_table[i].name,
					family_table[i].file_count,
	    			family_table[i].enable_count,
	    			family_table[i].style_table_count);
	    	for (j=0; j<family_table[i].style_table_count; j++) {
	    		if (family_table[i].style_table[j].name != 0L) {
	    			xprintf("  style:%s, t_count:%d\n",
	    					family_table[i].style_table[j].name,
	    					family_table[i].style_table[j].tuned_count);
	    		}
			}
    	}
	}
#endif
}

/*****************************************************************/
/* Font file API */

/******************* SEE WARNING FOR DIR ********************/

bool fc_write_file_table(FILE *fp) {
	int         i, j, length;
	char        buffer[86];

	/* write file count */ 
	fc_write32(buffer, file_table_count);
	if (fwrite(buffer, 1, 4, fp) != 4) return false;
	/* write all the dir entry */
	for (i=0; i<file_table_count; i++) {
		//printf("Writing file entry #%d (%s)\n", i, file_table[i].filename);
		if (file_table[i].filename == 0L) {
			buffer[0] = 0;
			buffer[1] = 0;
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
		}
		else {
			length = strlen(file_table[i].filename);
			fc_write16(buffer, length);
			if (fwrite(buffer, 1, 2, fp) != 2) return false;
			if (fwrite(file_table[i].filename, 1, length, fp) != (size_t)length) return false;

			fc_write32(buffer+0, file_table[i].key);
			fc_write16(buffer+4, file_table[i].dir_id);
			buffer[6] = file_table[i].status;
			buffer[7] = file_table[i].file_type;
			fc_write32(buffer+8, (uint32)file_table[i].mtime);
			fc_write16(buffer+12, file_table[i].family_id);
			fc_write16(buffer+14, file_table[i].style_id);
			fc_write32(buffer+16, file_table[i].flags);
			fc_write32(buffer+20, *((uint32*)&file_table[i].ascent));
			fc_write32(buffer+24, *((uint32*)&file_table[i].descent));
			fc_write32(buffer+28, *((uint32*)&file_table[i].leading));
			fc_write32(buffer+32, *((uint32*)&file_table[i].size));
			fc_write16(buffer+36, file_table[i].faces);
			for (j=0; j<8; j++)
				fc_write16(buffer+(38+j*2), file_table[i].gasp[j]);
			for (j=0; j<4; j++)
				fc_write32(buffer+(54+j*4), *((uint32*)&file_table[i].bbox[j]));
			for (j=0; j<3; j++)
				fc_write32(buffer+(70+j*4), file_table[i].blocks[j]);
			length = 0;
			if (file_table[i].block_bitmaps)
				for (j=0; j<0x100; j++)
					if (file_table[i].char_blocks[j] >= length)
						length = file_table[i].char_blocks[j]+1;
			length *= sizeof(fc_block_bitmap);
			fc_write32(buffer+82, length);
			if (fwrite(buffer, 1, 86, fp) != 86) return false;
			if (length > 0) {
				//printf("Writing %d byte block bitmap (%d bytes total)\n",
				//		length, length+256);
				if (fwrite(file_table[i].char_blocks, 1, 256, fp) != 256) return false;
				if (fwrite(file_table[i].block_bitmaps, 1, length, fp) != (size_t)length) return false;
			}
			if (file_table[i].range_count > 0 && file_table[i].char_ranges != NULL) {
				fc_write32(buffer, file_table[i].range_count);
				if (fwrite(buffer, 1, sizeof(int32), fp) != sizeof(int32)) return false;
				//printf("Writing %ld character ranges\n", file_table[i].range_count);
				for (j=0; j<file_table[j].range_count; j++) {
					fc_write32(buffer, file_table[i].char_ranges[j].first);
					fc_write32(buffer+sizeof(int32), file_table[i].char_ranges[j].last);
					if (fwrite(buffer, 1, sizeof(int32)*2, fp) != sizeof(int32)*2) return false;
				}
			} else {
				fc_write32(buffer, 0);
				if (fwrite(buffer, 1, sizeof(int32), fp) != sizeof(int32)) return false;
				//printf("Writing 0 character ranges\n");
			}
/*_sPrintf("dir:%d, st:%d, ft:%d, tm:%x, f:%d, s:%d, fl:%x, (%f,%f,%f,%f), %d, %x\n",
		 file_table[i].dir_id,
		 file_table[i].status,
		 file_table[i].file_type,
		 file_table[i].mtime,
		 file_table[i].family_id,
		 file_table[i].style_id,
		 file_table[i].flags,
		 file_table[i].ascent,
		 file_table[i].descent,
		 file_table[i].leading,
		 file_table[i].size,
		 file_table[i].faces,
		 file_table[i].gasp[3]);*/
		}
	}
	return true;
}

bool fc_read_file_table(FILE *fp) {
	int         i, j, index, hmask, length, file_count;
	char        buffer[86];
	uint32      buf32;

	/* read file count */
	if (fread(buffer, 1, 4, fp) != 4) return false;
	file_count = fc_read32(buffer);
	if ((file_count > FC_MAX_FILE_COUNT) || (file_count == 0)) return false;
	file_table = (fc_file_entry*)grMalloc(sizeof(fc_file_entry)*file_count,"fc:file_table",MALLOC_CANNOT_FAIL);
	file_table_count = file_count;
	/* read all the dir entry */
	for (i=0; i<file_table_count; i++) {
		//printf("Reading file entry #%d\n", i);
		file_table[i].filename = 0L;
		file_table[i].block_bitmaps = 0L;
		file_table[i].char_ranges = 0L;
		if (fread(buffer, 1, 2, fp) != 2) goto exit_error;
		length = fc_read16(buffer);
		if (length != 0) {
			file_table[i].filename = (char*)grMalloc(length+1,"fc:file_table.filename",MALLOC_CANNOT_FAIL);
			if (fread(file_table[i].filename, 1, length, fp) != (size_t)length) return false;
			file_table[i].filename[length] = 0;

			if (fread(buffer, 1, 86, fp) != 86) return false;
			file_table[i].key = fc_read32(buffer+0);
			file_table[i].dir_id = fc_read16(buffer+4);
			file_table[i].status = buffer[6];
			file_table[i].file_type = buffer[7];
			file_table[i].mtime = (time_t)fc_read32(buffer+8);
			file_table[i].family_id = fc_read16(buffer+12);
			file_table[i].style_id = fc_read16(buffer+14);
			file_table[i].flags = fc_read32(buffer+16);
			buf32 = fc_read32(buffer+20);
			file_table[i].ascent = *((float*)&buf32);
			buf32 = fc_read32(buffer+24);
			file_table[i].descent = *((float*)&buf32);
			buf32 = fc_read32(buffer+28);
			file_table[i].leading = *((float*)&buf32);
			buf32 = fc_read32(buffer+32);
			file_table[i].size = *((float*)&buf32);
			file_table[i].faces = fc_read16(buffer+36);
			for (j=0; j<8; j++)
				file_table[i].gasp[j] = fc_read16(buffer+(38+j*2));
			for (j=0; j<4; j++) {
				buf32 = fc_read32(buffer+(54+4*j));
				file_table[i].bbox[j] = *((float*)&buf32);
			}
			for (j=0; j<3; j++)
				file_table[i].blocks[j] = fc_read32(buffer+(70+4*j));
			length = fc_read32(buffer+82);
			if (length < 0) goto exit_error;
			if (length > 0) {
				if (fread(file_table[i].char_blocks, 1, 256, fp) != 256) goto exit_error;
				//printf("Allocating block bitmap size %d\n", length);
				file_table[i].block_bitmaps = (fc_block_bitmap*)
					grMalloc(length, "fc:block_bitmap", MALLOC_MEDIUM);
				if (file_table[i].block_bitmaps == NULL) goto exit_error;
				if (fread(file_table[i].block_bitmaps, 1, length, fp) != (size_t)length) goto exit_error;
			} else {
				memset(file_table[i].char_blocks, 0, 256);
			}
			if (fread(buffer, 1, sizeof(int32), fp) != sizeof(int32)) goto exit_error;
			length = fc_read32(buffer);
			if (length < 0 || length > 0x7fffffff/(ssize_t)sizeof(fc_char_range)) goto exit_error;
			if (length > 0) {
				//printf("Allocating char range size %ld\n", length*sizeof(fc_char_range));
				file_table[i].char_ranges = (fc_char_range*)
					grMalloc(length*sizeof(fc_char_range), "fc:char_range", MALLOC_MEDIUM);
				if (file_table[i].char_ranges == NULL) goto exit_error;
				//printf("Character range: %p\n", file_table[i].char_ranges);
				for (j=0; j<length; j++) {
					//printf("Reading range #%d\n", j);
					if (fread(buffer, 1, sizeof(int32)*2, fp) != sizeof(int32)*2) goto exit_error;
					file_table[i].char_ranges[j].first = fc_read32(buffer);
					file_table[i].char_ranges[j].last = fc_read32(buffer+sizeof(int32));
				}
			}
/*_sPrintf("dir:%d, st:%d, ft:%d, tm:%x, f:%d, s:%d, fl:%x, (%f,%f,%f,%f), %d, %x\n",
		 file_table[i].dir_id,
		 file_table[i].status,
		 file_table[i].file_type,
		 file_table[i].mtime,
		 file_table[i].family_id,
		 file_table[i].style_id,
		 file_table[i].flags,
		 file_table[i].ascent,
		 file_table[i].descent,
		 file_table[i].leading,
		 file_table[i].size,
		 file_table[i].faces,
		 file_table[i].gasp[3]);*/
			if (file_table[i].key != fc_hash_name(file_table[i].filename)) goto exit_error;
			if (file_table[i].dir_id >= dir_table_count) goto exit_error;
			if (file_table[i].status >= FC_MAX_FILE_STATUS) goto exit_error;
			if (file_table[i].file_type >= FC_MAX_FILE_TYPE) goto exit_error;
			if (file_table[i].file_type != FC_BAD_FILE) {
				if (file_table[i].ascent < -FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].ascent > FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].descent < -FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].descent > FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].leading < -FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].leading > FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].size < 0) goto exit_error;
				if (file_table[i].size > FC_BW_MAX_SIZE) goto exit_error;
				if (file_table[i].faces > FC_FACE_MAX_MASK) goto exit_error;
				if (file_table[i].gasp[1] > 3) goto exit_error;
				if (file_table[i].gasp[3] > 3) goto exit_error;
				if (file_table[i].gasp[5] > 3) goto exit_error;
				if (file_table[i].gasp[7] > 3) goto exit_error;
				if ((file_table[i].gasp[2] <= file_table[i].gasp[0]) &&
					(file_table[i].gasp[2] != 0xffff)) goto exit_error;
				if ((file_table[i].gasp[4] <= file_table[i].gasp[2]) &&
					(file_table[i].gasp[4] != 0xffff)) goto exit_error;
				if (file_table[i].gasp[6] != 0xffff) goto exit_error;
				for (j=0; j<4; j++)
					if ((file_table[i].bbox[j] < -100.0) || (file_table[i].bbox[j] > 100.0))
						goto exit_error;
			}
		}
	}
	/* link all unused entry and set free_file */
	file_count = 0;
	free_file = -1;
	for (i=0; i<file_table_count; i++) {
		if (file_table[i].filename == 0L) {
			file_table[i].next = free_file;
			free_file = i;
		}
		else file_count++;
	}

	/* hash all entries */	
	hmask = file_table_count*2-1;
	file_hash = (int*)grMalloc(sizeof(int)*(hmask+1),"fc:file_hash",MALLOC_CANNOT_FAIL);
	for (i=0; i<=hmask; i++)
		file_hash[i] = -1;
	file_hmask = hmask;
	
	for (i=0; i<file_table_count; i++)
		if (file_table[i].filename != 0L) {
			index = fc_hash_name(file_table[i].filename) & hmask;
			while (file_hash[index] != -1)
				index = (index+1) & hmask;
			file_hash[index] = i;
		}
	return true;
 exit_error:
 	xprintf("Bad fonts_list entry #%d (%s)\n", i, file_table[i].filename);
    for (; i>=0; i--) {
		if (file_table[i].filename)
			grFree(file_table[i].filename);
		if (file_table[i].block_bitmaps)
			grFree(file_table[i].block_bitmaps);
		if (file_table[i].char_ranges)
			grFree(file_table[i].char_ranges);
	}
	grFree(file_table);
	file_table = NULL;
	return false;
}

void fc_release_file_table() {
	int       i;

	for (i=0; i<file_table_count; i++) {
		if (file_table[i].filename)
			grFree(file_table[i].filename);
		if (file_table[i].block_bitmaps)
			grFree(file_table[i].block_bitmaps);
		if (file_table[i].char_ranges)
			grFree(file_table[i].char_ranges);
	}
	grFree(file_table);
	file_table = NULL;
	grFree(file_hash);
	file_hash = NULL;
}

void fc_flush_open_file_cache() {
	int                i;

	for (i=0; i<FC_MAX_COUNT_RENDERER; i++)
		fc_lock_render();
	fc_lock_open_file();
	/* check if the file is already open */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++)
		if (file_open_list[i].file_id != -1) {
			fc_close_and_flush_file(file_open_list[i].fp);
			if (file_open_list[i].fp_tuned) {
				fclose(file_open_list[i].fp_tuned);
				file_open_list[i].fp_tuned = 0L;
			}
			file_open_list[i].file_id = -1;
			file_open_list[i].tuned_file_id = -1;
		}
	fc_unlock_open_file();
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++)
		fc_unlock_render();
}

int fc_get_file_count(int *ref_id) {
	int           i, count;
	fc_file_entry *fe;
	
	count = 0;
	fe = file_table;
	for (i=0; i<file_table_count; i++) {
		if (fe->filename != 0L) {
			if (count == 0)
				*ref_id = i;
			count++;
		}
		fe++;
	}
	return count;
}

fc_file_entry *fc_next_file(int *ref_id) {
	int             id;
	fc_file_entry   *result, *fe;

	fe = file_table;
	id = *ref_id;
	if (id < file_table_count) {
		result = fe+id;
		id++;
		while (id < file_table_count) {
			if (fe[id].filename != 0L)
				break;
			id++;
		}
		*ref_id = id;
		return result;
	}
	return 0L;
}

fc_file_entry *fc_set_file(int dir_id, char *name, int command) {
	int               i, key, index, next_free;
	char              fullname[PATH_MAX];
	fc_context        *ctxt;
	struct stat       stat_buf;
	fc_file_entry     *fe;
	fc_font_renderer  *render;

 do_it_again:
	/* check if the file exists already */	
	key = fc_hash_name(name);
	index = key & file_hmask;
	while ((i = file_hash[index]) != -1) {
		fe = file_table+i;
		if ((fe->key == key) && (fe->dir_id == dir_id))
			if (strcmp(fe->filename, name) == 0) {
				if (fe->file_type == FC_NEW_FILE) {
					fc_remove_file(fe);
					break;
				}
				xprintf("Known font: %s\n", name);
				goto file_found;
			}
		index = (index+1)&file_hmask;
	}
	/* extend the file table if necessary */
	if (free_file < 0) {
		family_locker->lock_write();
        fc_lock_file();
		file_table = (fc_file_entry*)grRealloc(file_table,
											 sizeof(fc_file_entry)*file_table_count*2,
											 "fc:file_table",MALLOC_CANNOT_FAIL);
		file_hash = (int*)grRealloc(file_hash, sizeof(int)*4*file_table_count,"fc:file_hash",MALLOC_CANNOT_FAIL);
		for (i=file_table_count; i<2*file_table_count; i++) {
			file_table[i].filename = 0L;
			file_table[i].next = i+1;
			file_table[i].status = 0;
			file_table[i].block_bitmaps = NULL;
			file_table[i].char_ranges = NULL;
		}
		file_table[2*file_table_count-1].next = -1;
		free_file = file_table_count;
		for (i=0; i<4*file_table_count; i++)
			file_hash[i] = -1;
		file_hmask = 4*file_table_count-1;
		for (i=0; i<file_table_count; i++) {
			index = file_table[i].key & file_hmask;
			while (file_hash[index] >= 0)
				index = (index+1) & file_hmask;
			file_hash[index] = i;
		}
		file_table_count *= 2;
        fc_unlock_file();
		family_locker->unlock_write();
	}
	/* init the new file entry */
	fc_lock_file();
	fe = file_table+free_file;
	next_free = fe->next;
	fe->filename = name;
	fe->key = key;
	fe->dir_id = dir_id;
	if (command == FC_CHECK_FILE) {
		fe->file_type = FC_NEW_FILE;
		fe->mtime = 0;
	}
	else {
		if (!fc_init_file_entry(fe)) {
			fc_unlock_file();
			return 0L;
		}
	}
	/* check that the file is usable by our font renderer */
	fc_unlock_file();
	if ((fe->file_type == FC_TRUETYPE_FONT) || (fe->file_type == FC_TYPE1_FONT)
		|| (fe->file_type == FC_STROKE_FONT)) {
		ctxt = new fc_context(fe->family_id, fe->style_id, 10, true);
		render = new fc_font_renderer(ctxt);
		if (!render->first_time_init(fe)) {
			fe->file_type = FC_BAD_FILE;
			fc_remove_style(fe->family_id, fe->style_id);
		}
		delete render;
		delete ctxt;
	}
	/* Display informations on serial output */
	if ((fe->file_type == FC_TRUETYPE_FONT) || (fe->file_type == FC_TYPE1_FONT)
		|| (fe->file_type == FC_STROKE_FONT)) {
		if (fe->file_type == FC_TRUETYPE_FONT)
			xprintf("TrueType");
		else if (fe->file_type == FC_STROKE_FONT)
			xprintf("Stroke");
		else
			xprintf("PS Type1");
		xprintf(" font: %s (%s, %s) ",
				name,
				fc_get_family_name(fe->family_id),
				fc_get_style_name(fe->family_id, fe->style_id));
		if (fe->flags & FC_MONOSPACED_FONT)
			xprintf("Fixed\n");
		if (fe->flags & FC_DUALSPACED_FONT)
			xprintf("DualFixed\n");
		else
			xprintf("Proportional\n");
	}
	else if (fe->file_type == FC_BITMAP_FONT)
		xprintf("Tuned font: %s (%s, %s)\n",
				name,
				fc_get_family_name(fe->family_id),
				fc_get_style_name(fe->family_id, fe->style_id));
	else if (fe->file_type == FC_NEW_FILE)
		xprintf("New file: %s\n", name);
	else
		xprintf("Bad file: %s\n", name);
	/* set the basic enable flags */
	fc_lock_file();
	if ((fe->file_type == FC_TRUETYPE_FONT) || (fe->file_type == FC_TYPE1_FONT)
		|| (fe->file_type == FC_STROKE_FONT)) {
		fe->status = FC_FONT_FILE | FC_FILE_ENABLE;
		if (dir_table[dir_id].status & FC_DIR_ENABLE)
			fe->status |= FC_DIR_ENABLE;
	}
	/* register the new file for real */
	fe->filename = grStrdup(name,"fc:filename",MALLOC_CANNOT_FAIL);
	index = key & file_hmask;
	while (file_hash[index] != -1)
		index = (index+1)&file_hmask;
	file_hash[index] = free_file;
	free_file = next_free;
	fc_unlock_file();
	b_font_change_time = system_time();
	/* check if this is a new file */
 file_found:
	fc_get_pathname(fullname, fe);
	if (stat(fullname, &stat_buf) == 0)
		if ((stat_buf.st_mtime != fe->mtime) && (command != FC_CHECK_FILE)) {
			xprintf("File %s changed\n", name);
			fc_remove_file(fe);
			goto do_it_again;
		}
	/* change status */
	switch (command) {
	case FC_ENABLE_FILE :
		fc_set_file_status(fe, FC_FILE_ENABLE|FC_FILE_SCANNED, FC_FILE_ENABLE|FC_FILE_SCANNED);
		break;
	case FC_DISABLE_FILE :
		fc_set_file_status(fe, FC_FILE_SCANNED, FC_FILE_ENABLE|FC_FILE_SCANNED);
		break;
	case FC_CHECK_FILE :
	case FC_ANALYSE_FILE :
		fc_set_file_status(fe, FC_FILE_SCANNED, FC_FILE_SCANNED);
		break;
	}
	return fe;
}

void fc_set_file_status(fc_file_entry *fe, int enable, int mask) {
	fc_style_entry  *fs;
	fc_family_entry *ff;

	fc_lock_file();
	fe->status = (fe->status & (~mask)) | enable;
	if (fe->file_type == FC_TRUETYPE_FONT) {
		family_locker->lock_write();
		ff = family_table+fe->family_id;
		fs = ff->style_table+fe->style_id;
		if ((fe->status & (FC_FONT_FILE|FC_FILE_ENABLE|FC_DIR_ENABLE)) ==
			(FC_FILE_ENABLE|FC_DIR_ENABLE|FC_FONT_FILE)) {
			if ((fs->status & FC_STYLE_ENABLE) == 0) {
				fs->status |= FC_STYLE_ENABLE;
				ff->enable_count++;
				b_font_change_time = system_time();
			}
		}
		else {
			if (fs->status & FC_STYLE_ENABLE) {
				fs->status &= ~FC_STYLE_ENABLE;
				ff->enable_count--;
				b_font_change_time = system_time();
			}
		}	
		family_ordered_index_is_valid = false;				
		family_locker->unlock_write();
	}
	fc_unlock_file();
}

void fc_remove_file(fc_file_entry *fe) {
	int            i, id, index;

	fc_lock_open_file();
	fc_lock_file();
	/* unregister the entry from the family table */
	switch (fe->file_type) {
	case FC_TRUETYPE_FONT :
	case FC_TYPE1_FONT :
	case FC_STROKE_FONT :
		fc_remove_style(fe->family_id, fe->style_id);
		break;
	case FC_BITMAP_FONT :
		fc_remove_sub_style(fe, fe-file_table);
		break;
	}
	grFree(fe->filename);
	fe->filename = 0L;
	grFree(fe->block_bitmaps);
	fe->block_bitmaps = NULL;
	grFree(fe->char_ranges);
	fe->char_ranges = NULL;
	/* add the entry in the free entry list */
	id = fe-file_table;
	fe->next = free_file;
	free_file = id;
	/* reindex the hash table */
	for (i=0; i<2*file_table_count; i++)
		file_hash[i] = -1;
	for (i=0; i<file_table_count; i++)
		if (file_table[i].filename != 0L) {
			index = file_table[i].key & file_hmask;
			while (file_hash[index] >= 0)
				index = (index+1) & file_hmask;
			file_hash[index] = i;
		}
	fc_unlock_file();
	/* trash the ref from the list of open files */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++) {
		if (file_open_list[i].file_id == id) {
			fc_close_and_flush_file(file_open_list[i].fp);
			if (file_open_list[i].fp_tuned) {
				fclose(file_open_list[i].fp_tuned);
				file_open_list[i].fp_tuned = 0L;
				file_open_list[i].tuned_file_id = -1;
			}
			file_open_list[i].file_id = -1;
			break;
		}
		else if (file_open_list[i].tuned_file_id == id) {
			fclose(file_open_list[i].fp_tuned);
			file_open_list[i].fp_tuned = 0L;
			file_open_list[i].tuned_file_id = -1;
			break;
		}
	}
	fc_unlock_open_file();
}

void fc_get_pathname(char *name, fc_file_entry *fe) {
	int              i, j;
	char             *src_name;
	fc_dir_entry     *d;

	d = dir_table+fe->dir_id;
	i = 0;
	j = 0;
	src_name = d->dirname;
	while (src_name[j] != 0)
		name[i++] = src_name[j++];
	name[i++] = '/';	
	j = 0;
	src_name = fe->filename;
	while (src_name[j] != 0)
		name[i++] = src_name[j++];
	name[i] = 0;
}









