/* ++++++++++
	FILE:	font_support.cpp
	REVS:	$Revision: 1.7 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:31:28 PST 1997
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

/*****************************************************************/
/* big endian read/write support */

uint16 fc_read16(const void *adr) {
	return (((ulong)(((uchar*)adr)[0])<<8)+((ulong)((uchar*)adr)[1]));
}

uint32 fc_read32(const void *adr) {
	return (((ulong)(((uchar*)adr)[0])<<24)+(((ulong)((uchar*)adr)[1])<<16)+
			((ulong)(((uchar*)adr)[2])<<8)+(ulong)(((uchar*)adr)[3]));
}
	
void fc_write16(void *buf, uint16 value) {
	((uint8*)buf)[0] = (value>>8) & 0xff;
	((uint8*)buf)[1] = value & 0xff;
}

void fc_write32(void *buf, uint32 value) {
	((uint8*)buf)[0] = (value>>24) & 0xff;
	((uint8*)buf)[1] = (value>>16) & 0xff;
	((uint8*)buf)[2] = (value>>8) & 0xff;
	((uint8*)buf)[3] = value & 0xff;
}

/*****************************************************************/
/* matrix multiplication */

static int32 fixed_multiply( int32 mA, int32 mB );

void fc_multiply(fc_matrix* d, const fc_matrix* m1, const fc_matrix* m2)
{
	d->xxmult	= fixed_multiply(m1->xxmult,m2->xxmult)
				+ fixed_multiply(m1->yxmult,m2->xymult);
	d->xymult	= fixed_multiply(m1->xymult,m2->xxmult)
				+ fixed_multiply(m1->yymult,m2->xymult);
	d->xoffset	= fixed_multiply(m1->xoffset,m2->xxmult)
				+ fixed_multiply(m1->yoffset,m2->xymult)
				+ m2->xoffset;
	
	d->yxmult	= fixed_multiply(m1->xxmult,m2->yxmult)
				+ fixed_multiply(m1->yxmult,m2->yymult);
	d->yymult	= fixed_multiply(m1->xymult,m2->yxmult)
				+ fixed_multiply(m1->yymult,m2->yymult);
	d->yoffset	= fixed_multiply(m1->xoffset,m2->yxmult)
				+ fixed_multiply(m1->yoffset,m2->yymult)
				+ m2->yoffset ;
}

/* Stollen from the FontFusion engine.  (If we ever stop using it,
   this will need to be removed.  But the transformation matrix should
   really be replaced by BTransform2d, anyway.) */

static int32 fixed_multiply( int32 mA, int32 mB )
{
	uint16 mA_Hi, mA_Lo;
	uint16 mB_Hi, mB_Lo;
	uint32 d1, d2, d3;
	int32 result;
	int sign;
	
	
	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB = -mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB = -mB;
		}
	}
	/*
	result = (int32)((float)mA/65536.0 * (float)mB);
	result *= sign;
	return result;
	*/

	mA_Hi = (uint16)(mA>>16);
	mA_Lo = (uint16)(mA);
	mB_Hi = (uint16)(mB>>16);
	mB_Lo = (uint16)(mB);
	
	/*
			mB_Hi 	mB_Lo
	  X		mA_Hi 	mA_Lo
	------------------
	d1		d2		d3
	*/
	d3  = (uint32)mA_Lo * mB_Lo;		/* <<  0 */
	d2  = (uint32)mA_Lo * mB_Hi;		/* << 16 */
	d2 += (uint32)mA_Hi * mB_Lo;				/* << 16 */
	d1  = (uint32)mA_Hi * mB_Hi;		/* << 32 */
	
	result 	 = (int32)((d1 << 16) + d2 + (d3 >> 16));
	result	*= sign;
	return result; /*****/
}
	
/*****************************************************************/
/* top-level benaphoring support */

void fc_lock_font_list() {
#if ENABLE_BENAPHORES
	if (atomic_add(&font_list_lock, 1) >= 1)
#endif
		acquire_sem(font_list_sem);
}

void fc_unlock_font_list() {
#if ENABLE_BENAPHORES
	if (atomic_add(&font_list_lock, -1) > 1)
#endif
		release_sem(font_list_sem);
}

void fc_lock_open_file() {
#if ENABLE_BENAPHORES
	if (atomic_add(&open_file_lock, 1) >= 1)
#endif
		acquire_sem(open_file_sem);
}

void fc_unlock_open_file() {
#if ENABLE_BENAPHORES
	if (atomic_add(&open_file_lock, -1) > 1)
#endif
		release_sem(open_file_sem);
}

void fc_lock_collector() {
#if ENABLE_BENAPHORES
	if (atomic_add(&garbage_lock, 1) >= 1)
#endif
		acquire_sem(garbage_sem);
}

void fc_unlock_collector() {
#if ENABLE_BENAPHORES
	if (atomic_add(&garbage_lock, -1) > 1)
#endif
		release_sem(garbage_sem);
}

void fc_lock_file() {
#if ENABLE_BENAPHORES
	if (atomic_add(&file_lock, 1) >= 1)
#endif
		acquire_sem(file_sem);
}

void fc_unlock_file() {
#if ENABLE_BENAPHORES
	if (atomic_add(&file_lock, -1) > 1)
#endif
		release_sem(file_sem);
}

void fc_lock_render() {
#if ENABLE_BENAPHORES
	if (atomic_add(&render_lock, 1) >= FC_MAX_COUNT_RENDERER)
#endif
		acquire_sem(render_sem);
}

void fc_unlock_render() {
#if ENABLE_BENAPHORES
	if (atomic_add(&render_lock, -1) > FC_MAX_COUNT_RENDERER)
#endif
		release_sem(render_sem);
}

/*****************************************************************/
/* object support */

fc_cache_status::fc_cache_status() {
	cur_mem = 0L;
}

fc_cache_status::~fc_cache_status() {
}

void fc_cache_status::set(int bits,
						  int sh, int rot,
						  int over, int over_bonus,
						  int max, int size) {
	bits_per_pixel = bits;
	shear_bonus = sh;
	rotation_bonus = rot;
	oversize = over;
	oversize_bonus = over_bonus;
	max_mem = max;
	limit_size = size;
}

void fc_cache_status::copy(fc_cache_status *from) {
	bits_per_pixel = from->bits_per_pixel;
	shear_bonus = from->shear_bonus;
	rotation_bonus = from->rotation_bonus;
	oversize = from->oversize;
	oversize_bonus = from->oversize_bonus;
	max_mem = from->max_mem;
	limit_size = from->limit_size;
}

void fc_cache_status::add(fc_cache_status *sum) {
	if (sum->shear_bonus < shear_bonus)
		sum->shear_bonus = shear_bonus;
	if (sum->rotation_bonus < rotation_bonus)
		sum->rotation_bonus = rotation_bonus;
	if (sum->oversize < oversize)
		sum->oversize = oversize;
	if (sum->oversize_bonus < oversize_bonus)
		sum->oversize_bonus = oversize_bonus;
	if (sum->max_mem < max_mem)
		sum->max_mem = max_mem;
	if (sum->limit_size > limit_size)
		sum->limit_size = limit_size;
}

size_t fc_cache_status::flattened_size() const
{
	return 28;
}

bool fc_cache_status::flatten(char* buffer) const {
	fc_write32(buffer+0, bits_per_pixel);
	fc_write32(buffer+4, shear_bonus);
	fc_write32(buffer+8, rotation_bonus);
	fc_write32(buffer+12, oversize);
	fc_write32(buffer+16, oversize_bonus);
	fc_write32(buffer+20, max_mem);
	fc_write32(buffer+24, limit_size);
	return true;
}

bool fc_cache_status::unflatten(const char* buffer, size_t size) {
	if (size < flattened_size())
		return false;
		
	bits_per_pixel = fc_read32(buffer+0);
	shear_bonus = fc_read32(buffer+4);
	rotation_bonus = fc_read32(buffer+8);
	oversize = fc_read32(buffer+12);
	oversize_bonus = fc_read32(buffer+16);
	max_mem = fc_read32(buffer+20);
	limit_size = fc_read32(buffer+24);

	if ((bits_per_pixel != FC_GRAY_SCALE) && (bits_per_pixel != FC_TV_SCALE) &&
			(bits_per_pixel != FC_BLACK_AND_WHITE))
		return false;
	return true;
}

int fc_cache_status::bonus_setting(fc_context *context) {
	int     val;

	val = 0;
	if (context->shear() != 0.0)
		val += shear_bonus;
	if (context->rotation() != 0.0)
		val += rotation_bonus;
	if (context->size() > (int32)oversize)
		val += oversize_bonus;
	return val;
}

void fc_cache_status::get_settings(fc_font_cache_settings *set) {
	set->sheared_font_bonus = shear_bonus;
	set->rotated_font_bonus = rotation_bonus;
	set->oversize_threshold = oversize;
	set->oversized_font_bonus = oversize_bonus;
	set->cache_size = max_mem;
    set->spacing_size_threshold = limit_size;
}

void fc_cache_status::set_settings(fc_font_cache_settings *set, int bpp) {
	bits_per_pixel = bpp;
	shear_bonus = set->sheared_font_bonus;
	rotation_bonus = set->rotated_font_bonus;
	oversize = (uint32)set->oversize_threshold;
	oversize_bonus = set->oversized_font_bonus;
	max_mem = set->cache_size;
    limit_size = (uint32)set->spacing_size_threshold;	
}

int fc_cache_status::do_collect(int mem_size) {
	typedef struct {
		int       set_id;
		int       level;
		int       size;
		int       total;
	} sort_item;
	
	int           i, cnt, imax, j, level, memo_set_hit;
	int           id, size, index;
	sort_item     list[17];
	sort_item     *si;
	fc_set_entry  *se;

	set_locker->lock_read();
	for (i=0; i<set_hmask; i++)
		if (set_hash[i] != -1)
			if (set_table[set_hash[i]].char_count < 0)
				xprintf("Before:%d item corrupted !", i);
	set_locker->unlock_read();

	size = 0L;
	cnt = 0;
	do {
		/* safety against infinite loop */		
		cnt++;
		if (cnt == 3)
			break;
		/* init empty trash list */
		for (i=0; i<16; i++) {
			list[i].set_id = -1;
			list[i].level = -20000;
			list[i].size = 0;
			list[i].total = 0;
		}
		imax = 15;
		
		/* Insure the set tables will not be moved */
		set_locker->lock_read();
		se = set_table;
		memo_set_hit = set_hit;
		for (i=0; i<set_table_count; i++) {
			if ((se->char_count >= 0) &&
				(se->params._bits_per_pixel == bits_per_pixel)) {
				/* calculate the priority of this entry */
				level = memo_set_hit - se->hit_ref;
				if (level >= 0) {
					if (se->invalid)
						level += 10*FC_HIGH_PRIORITY_BONUS;
					else {
						if (level > 20000) level = 20000;
						level -= se->bonus;
					}
					if (level > list[imax].level) {
						/* insert in the list to be trashed */
						j = imax-1;
						while (j>=0) {
							if (level <= list[j].level) break;
							j--;
						}
						j++;
						memmove((char*)(list+j+1), (char*)(list+j),
								sizeof(sort_item)*(imax+1-j));
						si = list+j;
						si->set_id = i;
						si->level = level;
						si->size = se->char_buf_size + (se->char_hmask+1)*sizeof(fc_hset_item);
						if (j == 0) {
							si->total = si->size;
							if (si->total >= mem_size-size)
								goto no_more;
							si++;
							j++;
						}
						for (; j<=imax+1; j++) {
							si->total = si[-1].total + si->size;
							if (si->total >= mem_size-size)
								break;
							si++;
						}
					no_more:
						if (j > 15) j = 15;
						imax = j;
					}
				}
			}
			se++;
		}
		set_locker->unlock_read();

		/* lock everything */
		set_locker->lock_write();
		for (i=0; i<FC_LOCKER_COUNT; i++)
			draw_locker[i]->lock_write();

		/* trashed out all the selected entries */
		for (i=0; i<=imax; i++) {
			id = list[i].set_id;
			if (id >= 0) {
				se = set_table+id;
				if ((se->hit_ref <= memo_set_hit) &&
					(se->bonus < (FC_HIGH_PRIORITY_BONUS/2))) {
					size += se->char_buf_size + (se->char_hmask+1)*sizeof(fc_hset_item);
					se->next = free_set;
					free_set = id;
					se->char_count = -1;
					se->invalid = true;
					se->params.set_family_id(0xffff);
					se->params.set_style_id(0xffff);
					se->params.update_hash_key();
					grFree(se->hash_char);
					grFree(se->char_buf);
				}
			}
		}

		/* rehash the entries */
		for (i=0; i<=set_hmask; i++)
			set_hash[i] = -1;
		for (i=0; i<set_table_count; i++) {
			if (set_table[i].char_count >= 0) {
				index = set_table[i].params.hash_key() & set_hmask;
				while (set_hash[index] != -1)
					index = (index+1) & set_hmask;
				set_hash[index] = i;
			}
		}
		
//		set_uid++;
		
		/* unlock everything */
		for (i=0; i<FC_LOCKER_COUNT; i++)
			draw_locker[i]->unlock_write();
		set_locker->unlock_write();
	}
	while (size < (3*mem_size)/4);

	set_locker->lock_read();
	for (i=0; i<set_hmask; i++)
		if (set_hash[i] != -1)
			if (set_table[set_hash[i]].char_count < 0)
				xprintf("After:%d item corrupted !", i);
	set_locker->unlock_read();

	return size;
}

void fc_cache_status::assert_mem() {
	int      diff;

	diff = cur_mem-max_mem;
	if ((diff > 0) || (garbage_uid != last_garbage_uid)) {
		/* lock for exclusif access to garbage collection */
		fc_lock_collector();
		/* Do I still need to garbage collect ? */
		diff = cur_mem-max_mem;
		if ((diff > 0) || (garbage_uid != last_garbage_uid)) {
			last_garbage_uid = garbage_uid;
			if (max_mem < 128*1024L)
				diff += 16384;
			else
				diff += max_mem>>3;
			if (diff < 0)
				diff = 0;
			diff = do_collect(diff);
			atomic_add(&cur_mem, -diff);
		}
		/* unlock exclusif access */
		fc_unlock_collector();
	}
}

void fc_cache_status::flush() {
	int      i, diff;

	/* lock exclusif access */
	fc_lock_collector();
	
	/* flush as much stuff as possible out of the font cache */
	for (i=0; i<4; i++) {	
		diff = cur_mem;
		if (diff >= 0) {
			diff = do_collect(diff);
			atomic_add(&cur_mem, -diff);
		}
	}
	/* unlock exclusif access */
	fc_unlock_collector();
}

fc_count_name::fc_count_name(int count0) {
	int           i;
	
	table_count = count0;
	table = (item*)grMalloc(sizeof(item)*table_count,"fc:count_name:table",MALLOC_CANNOT_FAIL);
	for (i=0; i<table_count; i++)
		table[i].count = 0L;
}

fc_count_name::~fc_count_name() {
	int         i;

	for (i=0; i<table_count; i++)
		if (table[i].count > 0)
			grFree(table[i].name);
	grFree(table);
}

/* family locker should be locked before call */
char *fc_count_name::register_name(char *name) {
	int           i, index;

	/* look if this name is already known ? */
	for (i=0; i<table_count; i++)
		if (table[i].count != 0)
			if (strcmp(table[i].name, name) == 0) {
				table[i].count++;
				return table[i].name;
			}
	/* look for an empty entry */
	index = -1;
	for (i=0; i<table_count; i++)
		if (table[i].count == 0) {
			index = i;
			break;
		}
	/* extend the style_name_table */
	if (index == -1) {
		table = (item*)grRealloc(table, sizeof(item)*2*table_count,"fc:table",MALLOC_CANNOT_FAIL);
		for (i=table_count; i<2*table_count; i++)
			table[i].count = 0L;
		index = table_count;
		table_count *= 2;
	}
	/* init the new name */
	table[index].name = grStrdup(name,"fc:table.name",MALLOC_CANNOT_FAIL);
	table[index].count = 1;
	return table[index].name;
}

/* family locker should be locked before call */
void fc_count_name::unregister_name(char *name) {
	int           i;

	for (i=0; i<table_count; i++)
		if (strcmp(table[i].name, name) == 0) {
			table[i].count--;
			if (table[i].count == 0)
				grFree(table[i].name);
			return;
		}
}

#if ENABLE_BENAPHORES
fc_locker::fc_locker(int count) {
	reader_count = count;
	write_lock = 0L;
	write_sem = create_sem(0, "locker write sem");
	lock = 0L;
	sem = create_sem(0, "locker sem");
}

fc_locker::~fc_locker() {
	delete_sem(write_sem);
	delete_sem(sem);
}

void fc_locker::lock_write() {
	int     old;

	if (atomic_add(&write_lock, 1) >= 1)
		acquire_sem(write_sem);
	old = atomic_add(&lock, reader_count);
	if (old >= 1) {
		if (old < reader_count)
			acquire_sem_etc(sem, old, 0, 0);
		else
			acquire_sem_etc(sem, reader_count, 0, 0);
	}
}

void fc_locker::unlock_write() {
	int     old;

	old = atomic_add(&lock, -reader_count);
	if (old > reader_count) {
		if (old < 2*reader_count)
			release_sem_etc(sem, old-reader_count, 0);
		else
			release_sem_etc(sem, reader_count, 0);
	}
	if (atomic_add(&write_lock, -1) > 1)
		release_sem(write_sem);
}

void fc_locker::lock_read() {
	if (atomic_add(&lock, 1) >= reader_count)
		acquire_sem(sem);
}

void fc_locker::unlock_read() {
	if (atomic_add(&lock, -1) > reader_count)
		release_sem(sem);
}
#else
fc_locker::fc_locker(int count) {
	reader_count = count;
	sem = create_sem(count, "locker sem");
}

fc_locker::~fc_locker() {
	delete_sem(sem);
}

void fc_locker::lock_write() {
	acquire_sem_etc(sem, reader_count, 0, 0);
}

void fc_locker::unlock_write() {
	release_sem_etc(sem, reader_count, 0);
}

void fc_locker::lock_read() {
	acquire_sem(sem);
}

void fc_locker::unlock_read() {
	release_sem(sem);
}
#endif

/*****************************************************************/
/* miscelleanous functions */

int fc_hash_name(char *name) {
	int      key;

	key = 0;
	while (*name != 0) {
		key = ((key<<5)&0x7fffffff)|(key>>26);
		key ^= *name;
		name++;
	}
	return key;
}
















