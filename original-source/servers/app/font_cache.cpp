/* ++++++++++
   FILE:	font_cache.c
   REVS:	$Revision: 1.52 $
   NAME:	pierre
   DATE:	Tue Jan 28 10:45:43 PST 1997
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
#ifndef	SERVER_H
#include "server.h"
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
#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif
#ifndef _SAM_H
#include "sam.h"
#endif
#ifndef LOCK_H
#include "lock.h"
#endif

#include "token.h"

#include <Debug.h>

bigtime_t       b_font_change_time;

sem_id          file_sem;
long            file_lock;
sem_id          font_list_sem;
long            font_list_lock;
sem_id          open_file_sem;
long            open_file_lock;

fc_dir_entry    *dir_table;
int             dir_table_count;

fc_file_entry   *file_table;
int             *file_hash;
int             file_table_count;
int             free_file;
int             file_hmask;

fc_locker       *family_locker;
fc_family_entry *family_table;
int             *family_hash;
fc_family_entry	**family_ordered_index;
bool			family_ordered_index_is_valid;
int32			family_ordered_index_count;
int             family_table_count;
int             free_family;
int             family_hmask;

fc_count_name   *style_name;

fc_locker       *draw_locker[FC_LOCKER_COUNT];
fc_locker       *set_locker;
fc_set_entry    *set_table;
int             *set_hash;
int             set_table_count;
int             free_set;
int             set_hmask;
int             set_uid;  /* +1 when a set_entry is trashed,
								    or tables are reallocated */
int             garbage_uid;
int             last_garbage_uid;
int             set_hit;

sem_id          render_sem;
long            render_lock;
sem_id          garbage_sem;
long            garbage_lock;

fc_cache_status bw_status, gray_status;
fc_cache_status fc_bw_system_status, fc_gray_system_status;

int32           preferred_antialiasing = FC_GRAY_SCALE;
int32           aliasing_min_limit = 0;
int32           aliasing_max_limit = 0;
int32           hinting_limit = 17;

file_item       file_open_list[FC_FILE_OPEN_COUNT];

renderer_item   r_ctxt_list[FC_MAX_COUNT_RENDERER];

/*****************************************************************/
/* general management API */

static long fc_launcher(void *data) {	
	sam_thread_in(0);

	fc_lock_font_list();
	/* init the cache content */
	fc_load_font_cache();
	fc_unlock_font_list();

	sam_thread_out();
	return 0;
}

static void init_empty_tables() {
	int              i;

	/* init the font dir list */
	dir_table_count = 4;
	dir_table = (fc_dir_entry*)grMalloc(sizeof(fc_dir_entry)*dir_table_count,"fc:dir_table",MALLOC_CANNOT_FAIL);
	for (i=0; i<dir_table_count; i++)
		dir_table[i].dirname = 0L;

	/* init the font file list */
	file_table_count = 32;
	file_hmask = 2*file_table_count-1;
	file_table = (fc_file_entry*)grMalloc(sizeof(fc_file_entry)*file_table_count,"fc:file_table",MALLOC_CANNOT_FAIL);
	file_hash = (int*)grMalloc(sizeof(int)*file_table_count*2,"fc:file_hash",MALLOC_CANNOT_FAIL);
	for (i=0; i<file_table_count; i++) {
		file_table[i].filename = 0L;
		file_table[i].next = i+1;
		file_table[i].status = 0;
		file_table[i].block_bitmaps = 0L;
		file_table[i].char_ranges = 0L;
	}
	for (i=0; i<2*file_table_count; i++)
		file_hash[i] = -1;
	file_table[file_table_count-1].next = -1;
	free_file = 0;

	/* init the font family list */
	family_table_count = 8;
	family_hmask = 2*family_table_count-1;
	family_table = (fc_family_entry*)grMalloc(sizeof(fc_family_entry)*family_table_count,"fc:family_table",MALLOC_CANNOT_FAIL);
	family_ordered_index = (fc_family_entry**)grMalloc(sizeof(fc_family_entry*)*family_table_count,"fc:family_ordered_index",MALLOC_CANNOT_FAIL);
	family_ordered_index_is_valid = false;
	family_hash = (int*)grMalloc(sizeof(int)*family_table_count*2,"fc:family_hash",MALLOC_CANNOT_FAIL);
	for (i=0; i<family_table_count; i++) {
		family_table[i].next = i+1;
		family_table[i].file_count = 0;
	}
	family_table[family_table_count-1].next = -1;
	free_family = 0;
	for (i=0; i<2*family_table_count; i++)
		family_hash[i] = -1;
	/* init the style name list */
	style_name = new fc_count_name(8);

}

static void init_empty_set() {
	int              i;

	/* init the font set list */
	set_table_count = 16;
	set_hmask = 2*set_table_count-1;
	set_table = (fc_set_entry*)grMalloc(sizeof(fc_set_entry)*set_table_count,"fc:set_table",MALLOC_CANNOT_FAIL);
	set_hash = (int*)grMalloc(sizeof(int)*set_table_count*2,"fc:set_hash",MALLOC_CANNOT_FAIL);
	for (i=0; i<set_table_count; i++) {
		set_table[i].next = i+1;
		set_table[i].char_count = -1;   /* marker for unused set */
		set_table[i].invalid = TRUE;   /* marker for unused set */
	}
	set_table[set_table_count-1].next = -1;
	free_set = 0;
	for (i=0; i<2*set_table_count; i++)
		set_hash[i] = -1;
	set_uid = 0L;
	garbage_uid = 0L;
	last_garbage_uid = 0L;
}

void fc_start() {
	int              i;

	/* init benaphore to protect file */
	file_lock = 0L;
	font_list_lock = 0L;
	render_lock = 0L;
	garbage_lock = 0L;
	open_file_lock = 0L;
#if ENABLE_BENAPHORES
	file_sem = create_sem(0, "font file lock");
	font_list_sem = create_sem(0, "font list lock");
	render_sem = create_sem(0, "font renderer lock");
	garbage_sem = create_sem(0, "font garbage collect lock");
	open_file_sem = create_sem(0, "font open file lock");
#else
	file_sem = create_sem(1, "font file lock");
	font_list_sem = create_sem(1, "font list lock");
	render_sem = create_sem(FC_MAX_COUNT_RENDERER, "font renderer lock");
	garbage_sem = create_sem(1, "font garbage collect lock");
	open_file_sem = create_sem(1, "font open file lock");
#endif
	
	/* init paths to various font files */
	fc_init_file_paths();

	/* init the list of open file */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++) {
		file_open_list[i].file_id = -1;
		file_open_list[i].tuned_file_id = -1;
		file_open_list[i].fp_tuned = 0L;
		file_open_list[i].used = 0;
	}

	/* init the list of renderer context */
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++) {
		r_ctxt_list[i].renderer = fc_create_new_context();
		r_ctxt_list[i].file_id = -1;
		r_ctxt_list[i].last_used = 0;
		r_ctxt_list[i].used = 0;
	}
	
	/* init benaphore to protect families */
	family_locker = new fc_locker(1000);

	/* init the set locker list */
	for (i=0; i<FC_LOCKER_COUNT; i++)
		draw_locker[i] = new fc_locker(1000);
	set_locker = new fc_locker(1000);

	/* init low level character processing */
	fc_init_diffusion_table();

	/* init last font modification time */
	b_font_change_time = 0;

	/* set the current cache settings (they are initialized by the caller, in SFont) */
	fc_check_cache_settings();

	/* load the font file list */
	if (!fc_load_font_file_list())
		init_empty_tables();
	/* reset all the default folders */
	fc_set_default_font_file_list();		

	/* run a first scan for update, then save the current state */
	fc_scan_all_dir(FALSE);
	fc_save_font_file_list();

	init_empty_set();

	/* init/restore cache content in background */
	xspawn_thread((thread_entry)fc_launcher, "font cache launcher", B_NORMAL_PRIORITY, 0L);
}

void fc_stop() {
	int       i;

	/* wait for the launcher to complete before doing the shutdown */
	fc_lock_font_list();
	/* remove all the files from the list */
	for (i=0; i<dir_table_count; i++)
		if (dir_table[i].dirname != 0L)
			fc_remove_dir(dir_table[i].dirname);
	/* free all the font set currently used */
	for (i=0; i<set_table_count; i++)
		if (set_table[i].char_count != -1) {
			grFree(set_table[i].hash_char);
			grFree(set_table[i].char_buf);
		}
	/* close all files currently opened */
	for (i=0; i<FC_FILE_OPEN_COUNT; i++)
		if (file_open_list[i].file_id != -1) {
			fc_close_and_flush_file(file_open_list[i].fp);
			if (file_open_list[i].fp_tuned)
				fclose(file_open_list[i].fp_tuned);
		}
	/* free all the renderer context */
	for (i=0; i<FC_MAX_COUNT_RENDERER; i++)
		grFree(r_ctxt_list[i].renderer);
	
	/* free the set table and hash set table */
	grFree(set_table);
	grFree(set_hash);
	/* free the dir table */
	grFree(dir_table);
	/* free the file list and the file table */
	grFree(file_hash);
	grFree(file_table);
	/* free the family table, list and hash table */
	grFree(family_table);
	grFree(family_ordered_index);
	grFree(family_hash);
	/* uninit paths to various font files */
	fc_uninit_file_paths();
	/* release the benaphores for sets */
	for (i=0; i<FC_LOCKER_COUNT; i++)
		delete draw_locker[i];
	delete set_locker;
	/* release the benaphore for family */
	delete style_name;
	delete family_locker;
	/* release the benaphore for file */
	delete_sem(open_file_sem);
	delete_sem(garbage_sem);
	delete_sem(render_sem);
	delete_sem(file_sem);
	delete_sem(font_list_sem);
}

/*****************************************************************/
/* Font list/font cache save and restore */

/* default pathname access */
static char *list_path = NULL;
static char *data_path = NULL;

static char *font_path_def = NULL;
static char *font_path_opt = NULL;
//static char *font_path_def2 = NULL;
static char *font_path_opt2 = NULL;
static char *font_path_def3 = NULL;
static char *font_path_opt3 = NULL;

char *fc_init_file_path (directory_which type, const char *filename)
{
	char	path[PATH_MAX];
	char	*p;
	
	find_directory (type, -1, true, path, PATH_MAX);
	strcat (path, "/");
	strcat (path, filename);
	p = (char *)grMalloc(strlen(path)+1,"fc:path",MALLOC_CANNOT_FAIL);
	strcpy (p, path);
	return p;
}
	
void fc_init_file_paths()
{
	list_path = fc_init_file_path (B_USER_SETTINGS_DIRECTORY, "fonts_list");
	data_path = fc_init_file_path (B_USER_SETTINGS_DIRECTORY, "fonts_cache");

	font_path_def = fc_init_file_path (B_BEOS_FONTS_DIRECTORY, "ttfonts");
	font_path_opt = fc_init_file_path (B_USER_FONTS_DIRECTORY, "ttfonts");
//	font_path_def2 = fc_init_file_path (B_BEOS_FONTS_DIRECTORY, "psfonts");
	font_path_opt2 = fc_init_file_path (B_USER_FONTS_DIRECTORY, "psfonts");
	font_path_def3 = fc_init_file_path (B_BEOS_FONTS_DIRECTORY, "ffsfonts");
	font_path_opt3 = fc_init_file_path (B_USER_FONTS_DIRECTORY, "ffsfonts");
}

void fc_uninit_file_paths()
{
	if (list_path) {
		grFree (list_path);
		list_path = NULL;
	}
	if (data_path) {
		grFree (data_path);
		data_path = NULL;
	}
	if (font_path_def) {
		grFree (font_path_def);
		font_path_def = NULL;
	}
	if (font_path_opt) {
		grFree (font_path_opt);
		font_path_opt = NULL;
	}
/*	if (font_path_def2) {
		grFree (font_path_def2);
		font_path_def2 = NULL;
	}*/
	if (font_path_opt2) {
		grFree (font_path_opt2);
		font_path_opt2 = NULL;
	}
	if (font_path_def3) {
		grFree (font_path_def3);
		font_path_def3 = NULL;
	}
	if (font_path_opt3) {
		grFree (font_path_opt3);
		font_path_opt3 = NULL;
	}
}

void fc_check_cache_settings() {
	int                cookie[2];
	fc_cache_status    sum_bw, sum_gray;
	fc_cache_status    *cs;

	sum_bw.copy(&fc_bw_system_status);
	sum_gray.copy(&fc_gray_system_status);
	cookie[0] = 0;
	cookie[1] = 0;
	while (TRUE) {
		tokens->get_token_by_type(cookie, TT_FONT_CACHE, ANY_USER, (void**)&cs);
		if (cs == 0) break;
		xprintf("Add a token...\n");
		if (cs->bits_per_pixel == FC_GRAY_SCALE || cs->bits_per_pixel == FC_TV_SCALE)
			cs->add(&sum_gray);
		else
			cs->add(&sum_bw);
	}
	bw_status.copy(&sum_bw);
	gray_status.copy(&sum_gray);
}

bool fc_load_font_file_list() {
	FILE     *fp;

	/* try to open the prefs file */
	fp = fopen(list_path, "rb");
	if (fp == 0L)
		return FALSE;
	if (fc_read_dir_table(fp)) {
		if (fc_read_file_table(fp)) {
			if (fc_read_family_table(fp)) {
				fclose(fp);
				return TRUE;
			}
			fc_release_file_table();
		}
		fc_release_dir_table();
	}
	fclose(fp);
	return FALSE;
}

void fc_save_font_file_list() {
	FILE     *fp;

	/* try to open the prefs file */
	fp = fopen(list_path, "wb");
	if (fp == 0L) return;
	fc_write_dir_table(fp);
	fc_write_file_table(fp);
	fc_write_family_table(fp);
	fclose(fp);
}

void fc_set_default_font_file_list() {
	bool safemode = false;
	const char* safestr = getenv("SAFEMODE");
	if (safestr && strcmp(safestr, "yes") == 0) safemode = true;
	
	fc_set_dir((char*)font_path_def, TRUE, FC_SYSTEM_FOLDER);
	if (!safemode) fc_set_dir((char*)font_path_opt, TRUE, FC_SYSTEM_FOLDER);
//	fc_set_dir((char*)font_path_def2, TRUE, FC_SYSTEM_FOLDER);
	if (!safemode) fc_set_dir((char*)font_path_opt2, TRUE, FC_SYSTEM_FOLDER);
	fc_set_dir((char*)font_path_def3, TRUE, FC_SYSTEM_FOLDER);
	if (!safemode) fc_set_dir((char*)font_path_opt3, TRUE, FC_SYSTEM_FOLDER);
}

void fc_load_font_cache() {
	int               bpp, f_id, s_id, size, fsize, ssize;
	char              buffer[15];
	char              *fname, *sname;
	FILE              *fp;
	float             rot, sh;
	uint32            buf32, pipo_hit_ref;	
	fc_context        *ctxt;
	fc_set_entry      *se;
	fc_context_packet packet;

	fp = fopen(data_path, "rb");
	if (fp == 0L) return;

	if (fread(buffer, 1, 4, fp) != 4) goto error;
	if (fc_read32(buffer) != FC_FONT_CACHE_ID) goto error;

	packet.encoding = 0;
	packet.spacing_mode = B_CHAR_SPACING;
	packet.faces = 0;
	ctxt = new fc_context();
	se = set_table;

	fname = 0L;
	sname = 0L;
	while (TRUE) {
		if (fread(buffer, 1, 15, fp) != 15) break;
		bpp = buffer[0];
		fsize = fc_read16(buffer+1);
		ssize = fc_read16(buffer+3);
		size = fc_read16(buffer+5);
		buf32 = fc_read32(buffer+7);
		rot = *((float*)&buf32);
		buf32 = fc_read32(buffer+11);
		sh = *((float*)&buf32);
		if ((fsize <= 0) || (fsize > 1024)) break; 
		if ((ssize <= 0) || (ssize > 1024)) break; 
		fname = (char*)grMalloc(fsize+1,"fc:fname",MALLOC_CANNOT_FAIL);
		sname = (char*)grMalloc(ssize+1,"fc:sname",MALLOC_CANNOT_FAIL);
		if (fread(fname, 1, fsize, fp) != fsize) break;
		if (fread(sname, 1, ssize, fp) != ssize) break;
		fname[fsize] = 0;
		sname[ssize] = 0;
		if ((bpp != FC_GRAY_SCALE) && (bpp != FC_TV_SCALE) &&
				(bpp != FC_BLACK_AND_WHITE)) break;
		f_id = fc_get_family_id(fname);
		s_id = fc_get_style_id(f_id, sname);
		if ((f_id < 0) || (s_id < 0)) break;
		if ((size < 0) || (size > FC_BW_MAX_SIZE)) break;
		packet.f_id = f_id;
		packet.s_id = s_id;
		packet.size = (float)size;
		packet.rotation = rot;
		packet.shear = sh;
		if (bpp == FC_GRAY_SCALE || bpp == FC_TV_SCALE)
			packet.flags = 0;
		else
			packet.flags = B_DISABLE_ANTIALIASING;
		ctxt->set_context_from_packet(&packet, B_FONT_ALL);
		
		se = ctxt->lock_set_entry(1, &pipo_hit_ref);		
		if (!fc_read_set_from_file(fp, se)) {			
			ctxt->unlock_set_entry();
			break;
		}
		ctxt->unlock_set_entry();
	}
	delete ctxt;

 error:	
	fclose(fp);
}

void fc_save_font_cache() {
	int               i, exit, fsize, ssize;
	char              buffer[15];
	char              *fname, *sname;
	FILE              *fp;
	fc_set_entry      *se;

	fp = fopen(data_path, "wb");
	if (fp == 0L) return;
	
	fc_write32(buffer, FC_FONT_CACHE_ID);
	if (fwrite(buffer, 1, 4, fp) != 4) goto error;

	set_locker->lock_read();
	se = set_table;	
	exit = 0;
	for (i=0; i<set_table_count; i++) {
		if ((se->char_count >= 0) &&
			((se->params.bits_per_pixel() == FC_GRAY_SCALE) ||
			 (se->params.bits_per_pixel() == FC_TV_SCALE) ||
			 (se->params.bits_per_pixel() == FC_BLACK_AND_WHITE))) {
			draw_locker[i & (FC_LOCKER_COUNT-1)]->lock_read();
			buffer[0] = se->params.bits_per_pixel();
			fname = fc_get_family_name(se->params.family_id());
			sname = fc_get_style_name(se->params.family_id(), se->params.style_id());
			fsize = strlen(fname);
			ssize = strlen(sname);
			fc_write16(buffer+1, fsize);
			fc_write16(buffer+3, ssize);
			fc_write16(buffer+5, se->params.size());
			fc_write32(buffer+7, *((uint32*)&se->params._rotation));
			fc_write32(buffer+11, *((uint32*)&se->params._shear));
			if (fwrite(buffer, 1, 15, fp) != 15) exit = 1;
			else if (fwrite(fname, 1, fsize, fp) != fsize) exit = 1;
			else if (fwrite(sname, 1, ssize, fp) != ssize) exit = 1;
			else if (!fc_write_set_to_file(fp, se)) exit = 1;
			draw_locker[i & (FC_LOCKER_COUNT-1)]->unlock_read();
		}
		se++;
		if (exit) break;
	}
	set_locker->unlock_read();

 error:	
	fclose(fp);
}

/*****************************************************************/
/* Font cache setting per app, save and restore in token space */

void fc_get_app_settings(int bpp, long pid, fc_font_cache_settings *set) {
	int                cookie[2];
	fc_cache_status    *cs;
	
	cookie[0] = 0;
	cookie[1] = 0;
	while (TRUE) {
		tokens->get_token_by_type(cookie, TT_FONT_CACHE, pid, (void**)&cs);
		if (cs == 0) break;
		if (cs->bits_per_pixel == bpp) {
			cs->get_settings(set);
			return;
		}
	}
	if (bpp == FC_GRAY_SCALE || bpp == FC_TV_SCALE)
		fc_gray_system_status.get_settings(set);
	else
		fc_bw_system_status.get_settings(set);	
}

void fc_set_app_settings(int bpp, long pid, fc_font_cache_settings *set) {
	int                cookie[2];
	fc_cache_status    *cs;
	
	cookie[0] = 0;
	cookie[1] = 0;
	while (TRUE) {
		tokens->get_token_by_type(cookie, TT_FONT_CACHE, pid, (void**)&cs);
		if (cs == 0) break;
		if (cs->bits_per_pixel == bpp) {
			cs->set_settings(set, bpp);
			return;
		}
	}
	cs = new fc_cache_status();
	cs->set_settings(set, bpp);
	tokens->new_token(pid, TT_FONT_CACHE, (void*)cs);
}

void fc_release_app_settings(void *data) {
	fc_cache_status    *cs;

	cs = (fc_cache_status*)data;
	delete cs;
	fc_check_cache_settings();
}
















