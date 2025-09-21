/* ++++++++++
	FILE:	font_context.cpp
	REVS:	$Revision: 1.7 $
	NAME:	pierre
	DATE:	Thu Apr  3 10:31:36 PST 1997
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

void fc_params::update_hash_key() {
	int     val, val2;

	val = _bits_per_pixel;
	val2 = _family_id;
	val += (val<<4);
	val2 += (val2<<3);
	val ^= _algo_faces;
	val2 ^= _hinting;
	val += (val<<3);
	val2 += (val2<<2);
	val ^= _style_id;
	val2 ^= _size;
	val += (val<<5);
	val2 += (val2<<4);
	val ^= *((int*)&_rotation);
	val2 ^= *((int*)&_shear);
	_hash_key = ((val<<11)|(val>>21))^val2;			
}

int fc_params::check_params(const fc_params *ref) const {
	if (ref->hash_key() != _hash_key)
		return 0;
	if ((ref->family_id() != _family_id) ||
		(ref->style_id() != _style_id) ||
		(ref->size() != _size) ||
		(ref->rotation() != _rotation) ||
		(ref->shear() != _shear) ||
		(ref->algo_faces() != _algo_faces) ||
		(ref->bits_per_pixel() != _bits_per_pixel) ||
		(ref->hinting() != _hinting))
		return 0;
	return 1;
}

void fc_params::copy(fc_params *ref) {
	_hash_key = ref->hash_key();
	_family_id = ref->family_id();
	_style_id = ref->style_id();
	_size = ref->size();
	_rotation = ref->rotation();
	_shear = ref->shear();
	_algo_faces = ref->algo_faces();
	_bits_per_pixel = ref->bits_per_pixel();
	_hinting = ref->hinting();
}

fc_cache_status *fc_params::cache_status() {
	switch (_bits_per_pixel) {
	case FC_BLACK_AND_WHITE :
		return &bw_status;
	case FC_TV_SCALE :
	case FC_GRAY_SCALE :
		return &gray_status;
	default:
		return 0L;
	}
}

void fc_set_entry::invalidate() {
	invalid = TRUE;
	garbage_uid++;
}

fc_context::fc_context() {
	_params._rotation = 0.0;
	_params._shear = 0.0;
	_params._size = 10;
	_params._algo_faces = 0;
	_escape_size = 10;
	_final_flags = _flags = 0;
	_style_faces = 0;
	_spacing_mode = B_CHAR_SPACING;
	_encoding = 0;
	_matrix.xoffset = 0;
	_matrix.yoffset = 0;
	_origin_x = 0.0;
	_origin_y = 0.0;
	set_id = 0;
	uid = set_uid;
	locker = draw_locker[0];
	update_bits_per_pixel();
	update_hinting();
	set_font_ids(0xfffe, 0xfffe, false);
	_params.status = FC_TUNED_CHANGED|FC_MATRIX_CHANGED;
	_params.update_hash_key();
}

fc_context::fc_context(const fc_context_packet *packet) {
	_params.status = 0;
	/* size */
	if (packet->size <= 0.0)
		_escape_size = 0.0;
	else if (packet->size > FC_BW_MAX_SIZE)
		_escape_size = FC_BW_MAX_SIZE;
	else
		_escape_size = packet->size;
	_params.set_size((int)_escape_size);
	/* encoding */
	if (packet->encoding < FC_SYMBOL_SET_COUNT)
		_encoding = packet->encoding;
	else
		_encoding = 0;
	/* flags */
	_final_flags = _flags = packet->flags;
	/* spacing mode */
	if (packet->spacing_mode < B_SPACING_COUNT) 
		_spacing_mode = packet->spacing_mode;
	else
		_spacing_mode = B_CHAR_SPACING;
	/* rotation and shear */
	_params.set_rotation(packet->rotation);
	if (packet->shear < (-3.14159/4.0))
		_params.set_shear(-3.14159/4.0);
	else if (packet->shear > (3.14159/4.0))
		_params.set_shear(3.14159/4.0);
	else
		_params.set_shear(packet->shear);
	update_bits_per_pixel();
	update_hinting();
	/* set family, style and file descriptor. Can force virtual spacing mode */
	set_font_ids(packet->f_id, packet->s_id);
	set_faces(packet->faces);
	/* init matrix */
	_matrix.xoffset = 0;
	_matrix.yoffset = 0;
	_origin_x = 0.0;
	_origin_y = 0.0;
	/* init default parameter */
	_params.status |= FC_MATRIX_CHANGED;
	set_matrix();
	_params.update_hash_key();
	set_id = 0;
	uid = set_uid;
	locker = draw_locker[0];
}

fc_context::fc_context(int family_id, int style_id, int size, bool allow_disable) {
	_params.status = 0;
	/* size */
	if (size <= 0)
		size = 0;
	else if (size > FC_BW_MAX_SIZE)
		size = FC_BW_MAX_SIZE;
	_params.set_size(size);
	_params._algo_faces = 0;
	_params._rotation = 0.0;
	_params._shear = 0.0;	
	_escape_size = size;
	_final_flags = _flags = 0;
	_style_faces = 0;
	_spacing_mode = B_CHAR_SPACING;
	_encoding = 0;	
	_matrix.xoffset = 0;
	_matrix.yoffset = 0;
	_origin_x = 0.0;
	_origin_y = 0.0;
	set_id = 0;
	uid = set_uid;
	locker = draw_locker[0];
	update_bits_per_pixel();
	update_hinting();
	set_font_ids(family_id, style_id, allow_disable);
	_params.status |= FC_MATRIX_CHANGED;
	set_matrix();
	_params.update_hash_key();
}

fc_context::fc_context(const fc_context *ctxt) {
	set_to(ctxt);
}

fc_context::~fc_context() {
}

fc_context& fc_context::operator=(const fc_context& c)
{
	if (this != &c) set_to(&c);
	return *this;
}

uint32 fc_context::combine_flags(uint32 flags) const
{
	uint32 newFlags = _flags;
	
	if ((flags&B_ANTIALIASING_MASK) != 0)
		newFlags = (newFlags&~B_ANTIALIASING_MASK) | (flags&B_ANTIALIASING_MASK);
	if ((flags&B_HINTING_MASK) != 0)
		newFlags = (newFlags&~B_HINTING_MASK) | (flags&B_HINTING_MASK);
	if ((flags&B_GLOBAL_OVERLAY_MASK) != 0)
		newFlags = (newFlags&~B_GLOBAL_OVERLAY_MASK) | (flags&B_GLOBAL_OVERLAY_MASK);
	
	return newFlags;
}

void fc_context::set_faces(uint16 faces)
{
	// Pass any algorithmic faces to the parameters.
	_params.set_algo_faces(faces&(B_BOLD_FACE|B_ITALIC_FACE)&~_style_faces);
}

uint16 fc_context::faces() const
{
	uint16 face = _params.algo_faces()
				| (_style_faces == B_REGULAR_FACE ? 0 : _style_faces);
	return (face == 0 ? _style_faces : face);
}

bool fc_context::set_font_ids(int f_id, int s_id, bool allow_disable, bool keep_faces) {
	int              i, index;
	uint16			 *gasp;
	ushort           key;
	fc_tuned_entry   *te;
	fc_style_entry   *se;
	fc_family_entry  *fe;

	family_locker->lock_read();
	/* check that the id range is valid */
	if ((f_id < 0) || (f_id >= family_table_count))
		goto error;
	fe = family_table+f_id;
	/* check that the id is valid */
	if (fe->file_count == 0)
		goto error;
	/* check that disabled are allowed or that the family is not disabled */
	if (!allow_disable)
		if (fe->enable_count == 0)
			goto error;
	/* check that the id range is valid */
	if ((s_id < 0) || (s_id >= fe->style_table_count))
		goto error;
	/* check that the id is valid */
	se = fe->style_table + s_id;
	if (se->name == 0L)
		goto error;
	/* check that disabled are allowed or that the family is not disabled */
	if (!allow_disable)
		if ((se->status & FC_STYLE_ENABLE) == 0)
			goto error;
	file_id = se->file_id;

	/* check if we need to resolve bits_per_pixel() */
#if GASP_ENABLE
	if (bits_per_pixel() == FC_GASP_UNKNOWN) {
		gasp = se->gasp;
//		_sPrintf("GASP: %04x %04x %04x %04x %04x %04x %04x %04x\n",
//				 gasp[0], gasp[1], gasp[2], gasp[3], gasp[4], gasp[5], gasp[6], gasp[7]);
		while (gasp[0] < size()) gasp += 2;
		if (gasp[1] & 0x0002)
			_params.set_bits_per_pixel(FC_GRAY_SCALE);
		else
			_params.set_bits_per_pixel(FC_BLACK_AND_WHITE);
//		_sPrintf("Need to do GASP resolution ! (%d) [%d,%d]\n", bits_per_pixel(),
//				 gasp[0], gasp[1]);
	}
#endif

	/* check if we can use a hand-tuned substitute */
	if (se->tuned_count != 0) {
		key = sub_style_key(size(), &_params._rotation, &_params._shear, bits_per_pixel());
		index = key & se->tuned_hmask;
		/* look for the key (and size) in the hash table */
		while ((i = se->tuned_hash[index]) != 0xffff) {
			te = se->tuned_table+i;
			if (te->key == key)
				if ((te->size == size()) &&
					/* check real match */
					(te->rotation == rotation()) &&
					(te->shear == shear()) &&
					(te->bpp == bits_per_pixel()) &&
					(true == hinting())) {
					tuned_file_id = te->file_id;
					tuned_offset = te->offset;
					tuned_hmask = te->hmask;
					goto tuned_found;
				}
			index = (index+1) & se->tuned_hmask;
		}
	}
	/* no match. We will used the scalable font */
	tuned_file_id = 0xffff;
 tuned_found:
	
	family_locker->unlock_read();
	fc_lock_file();
	file_type = file_table[file_id].file_type;
	_ascent = file_table[file_id].ascent;
	_descent = file_table[file_id].descent;
	_leading = file_table[file_id].leading;
	
	if (keep_faces) {
		uint old_faces = faces();
		_style_faces = file_table[file_id].faces;
		set_faces(old_faces);
	} else {
		_style_faces = file_table[file_id].faces;
		_params.set_algo_faces(0);
	}
	
	/* Switch automatically the spacing mode between BITMAP and FIXED, depending
	   if the font is monospaced or not. */
	_file_flags = file_table[file_id].flags;
	if (_file_flags & FC_MONOSPACED_FONT) {
		if (spacing_mode() == B_BITMAP_SPACING)
			set_spacing_mode(B_FIXED_SPACING);
	}
	else {
		if (spacing_mode() == B_FIXED_SPACING)
			set_spacing_mode(B_BITMAP_SPACING);
	}
	
	set_family_id(f_id);
	set_style_id(s_id);
	fc_unlock_file();
	return 1;
 error:
	family_locker->unlock_read();
	set_family_id(0xffff);
	set_style_id(0xffff);
	file_id = 0xffff;
	tuned_file_id = 0xffff;	
	_ascent = 0.8;
	_descent = 0.2;
	_leading = 0.05;
	_style_faces = 0;
	_file_flags = 0L;
	return 0;
}

void fc_context::set_to(const fc_context *ctxt) {
	file_id = ctxt->file_id;
	tuned_file_id = ctxt->tuned_file_id;
	file_type = ctxt->file_type;
	tuned_offset = ctxt->tuned_offset;
	tuned_hmask = ctxt->tuned_hmask;
	_file_flags = ctxt->_file_flags;
	delta_space_x = ctxt->delta_space_x;
	delta_space_y = ctxt->delta_space_y;
	_params = ctxt->_params;
	_ascent = ctxt->_ascent;
	_descent = ctxt->_descent;
	_leading = ctxt->_leading;
	_escape_size = ctxt->_escape_size;
	_encoding = ctxt->_encoding;
	_spacing_mode = ctxt->_spacing_mode;
	_flags = ctxt->_flags;
	_final_flags = ctxt->_final_flags;
	_style_faces = ctxt->_style_faces;
	set_id = ctxt->set_id;
	uid = ctxt->uid;
	locker = ctxt->locker;
	_matrix = ctxt->_matrix;
	_origin_x = ctxt->_origin_x;
	_origin_y = ctxt->_origin_y;
}

void fc_context::set_context_from_packet(const fc_context_packet *packet, uint mask) {
	/* size */
	if (mask & B_FONT_SIZE) {
		if (packet->size != _escape_size) {
			if (packet->size <= 0.0)
				_escape_size = 0.0;
			else if (packet->size > FC_BW_MAX_SIZE)
				_escape_size = FC_BW_MAX_SIZE;
			else
				_escape_size = packet->size;
			_params.set_size((int)_escape_size);
			if (!(mask & B_FONT_FLAGS) && !(mask & B_FONT_FAMILY_AND_STYLE)) {
				update_bits_per_pixel();
				update_hinting();
			}
		}
	}
	/* flags */
	if (mask & B_FONT_FLAGS) {
		if (_flags != packet->flags || _final_flags != packet->flags) {
			_final_flags = _flags = packet->flags;
			if (!(mask & B_FONT_FAMILY_AND_STYLE)) {
				update_bits_per_pixel();
				update_hinting();
			}
		}
	}
	/* encoding */
	if (mask & B_FONT_ENCODING) {
		if (packet->encoding < FC_SYMBOL_SET_COUNT)
			_encoding = packet->encoding;
		else
			_encoding = 0;
	}
	/* spacing mode */
	if (mask & B_FONT_SPACING) {
		if (packet->spacing_mode < B_SPACING_COUNT) 
			_spacing_mode = packet->spacing_mode;
		else
			_spacing_mode = B_CHAR_SPACING;
	}
	/* rotation and shear */
	if (mask & B_FONT_ROTATION)
		_params.set_rotation(packet->rotation);
	if (mask & B_FONT_SHEAR) {
		if (packet->shear < (-3.14159/4.0))
			_params.set_shear(-3.14159/4.0);
		else if (packet->shear > (3.14159/4.0))
			_params.set_shear(3.14159/4.0);
		else
			_params.set_shear(packet->shear);
	}
	/* set family, style and file descriptor */
	if (mask & B_FONT_FAMILY_AND_STYLE) {
		update_bits_per_pixel();
		update_hinting();
		set_font_ids(packet->f_id, packet->s_id);
		params()->status = (params()->status|FC_SET_CHANGED)&(FC_ALL_CHANGED^FC_TUNED_CHANGED);
	}
	/* faces */
	if (mask & B_FONT_FACE)
		set_faces(packet->faces);
}

void fc_context::get_packet_from_context(fc_context_packet *packet) const
{
	packet->f_id = family_id();
	packet->s_id = style_id();
	packet->size = size();
	packet->rotation = rotation();
	packet->shear = shear();
	packet->encoding = encoding();
	packet->spacing_mode = spacing_mode();
	packet->faces = faces();
	packet->flags = flags();
};

void fc_context::update_bits_per_pixel()
{
	if (params()->size() > FC_GRAY_MAX_SIZE)
		_params.set_bits_per_pixel(FC_BLACK_AND_WHITE);
	else {
		switch (_final_flags & B_ANTIALIASING_MASK) {
			case B_DISABLE_ANTIALIASING:
				_params.set_bits_per_pixel(FC_BLACK_AND_WHITE);
				break;
			case B_NORMAL_ANTIALIASING:
				_params.set_bits_per_pixel(FC_GRAY_SCALE);
				break;
			case B_TV_ANTIALIASING:
				_params.set_bits_per_pixel(FC_TV_SCALE);
				break;
			default:
#if GASP_ENABLE
				_params.set_bits_per_pixel(FC_GASP_UNKNOWN);
#else
				if (params()->size() >= aliasing_min_limit &&
						params()->size() <= aliasing_max_limit)
					_params.set_bits_per_pixel(FC_BLACK_AND_WHITE);
				else
					_params.set_bits_per_pixel(preferred_antialiasing);
#endif
		}
	}
	
	// Constrain to valid values.
	switch (_params.bits_per_pixel()) {
		case FC_BLACK_AND_WHITE:
		case FC_GRAY_SCALE:
		case FC_TV_SCALE:
			break;
		default:
			_params.set_bits_per_pixel(FC_BLACK_AND_WHITE);
			break;
	}
}

void fc_context::update_hinting()
{
	if ((_final_flags & B_HINTING_MASK) == B_DISABLE_HINTING)
		_params.set_hinting(false);
	else if ((_final_flags & B_HINTING_MASK) == B_ENABLE_HINTING)
		_params.set_hinting(true);
	else
		_params.set_hinting(params()->size() <= hinting_limit);
}

void fc_context::set_matrix() {
	float       pt_size, cs, sn, tn;
	
	switch (file_type) {
	case FC_TYPE1_FONT :
	case FC_TRUETYPE_FONT :
	case FC_STROKE_FONT :
		pt_size = 65536.0*(float)size();
		cs = cos(rotation());
		sn = sin(rotation());
		delta_space_x = cs;
		delta_space_y = -sn;
		tn = tan(shear());
		_matrix.xxmult = (int32)(pt_size*cs + 0.5);
		_matrix.xymult = (int32)(pt_size*sn + 0.5);
		_matrix.yxmult = (int32)(pt_size*(-sn+tn*cs) + 0.5);
		_matrix.yymult = (int32)(pt_size*(cs+tn*sn) + 0.5);
		_params.status &= ~FC_MATRIX_CHANGED;
		break;
	}
}

fc_set_entry *fc_context::lock_set_entry(int               write_mode,
										 uint32            *set_hit_ref,
										 fc_font_renderer  *render) {
	int              i, index, size, step, bonus, offset, own_render;
	ushort           code;
	fc_char          *my_char;
	fc_set_entry     *se;
	fc_cache_status  *cs;

	last_write_mode = write_mode;
	/* lock the set tables to assure permanence of the struct */
	set_locker->lock_read();
	/* update the state if change of setting since previous call */
	se = set_table+set_id;
	if (params()->status != 0) {
		if (params()->status & FC_TUNED_CHANGED)
			set_font_ids(family_id(), style_id(), false, true);
		if (params()->status & FC_MATRIX_CHANGED)
			set_matrix();
		params()->update_hash_key();
		
		// Clear the status.  We know we took care of everything,
		// except the matrix should have been cleared by set_matrix().
		params()->status &= FC_MATRIX_CHANGED;
		
		goto find_new_set;
	}
	/* check everything again in case of possible font file change */
	if ((set_uid == uid) && !se->invalid)
		if (check_params(&se->params))
			goto end;
	uid = set_uid;
	set_font_ids(family_id(), style_id(), false, true);
	/* look for the good set_entry descriptor */
 find_new_set:
	index = hash_key()&set_hmask;
	while (set_hash[index] != -1) {
		se = set_table+set_hash[index];
		if (check_params(&se->params) && !se->invalid) {
			/* set_entry found. Update the index in the set_table */
			set_id = set_hash[index];
			locker = draw_locker[set_id & (FC_LOCKER_COUNT-1)];
			if (se->char_count < 0)
				xprintf("read exit bad 1\n");
			goto end;
		}
		index = (index+1)&set_hmask;
	}

	/************************************************************/
	/* the set_entry was not found. We need to create a new one */

	/* calculate the bonus of that set */
	cs = cache_status();
#if GASP_ENABLE
	if (cs == NULL) {
		/* May need to resolve GASP */
		set_font_ids(family_id(), style_id(), false, true);
		cs = cache_status();
	}
#endif
	bonus = cs->bonus_setting(this);
	
	/* Release the set tables */
	set_locker->unlock_read();

	/* try to allocate a renderer for advanced spacing setting if necessary */
	own_render = 0;
	if (params()->size() < FC_SIZE_MAX_SPACING) {
		if (render == 0L) {
			render = new fc_font_renderer(this);
			if (!render->reset()) {
				/* the init of the renderer failed */
				delete render;
				render = 0L;
			}
			own_render = 1;
		}
	}
	/* Lock modification access to set tables */
	set_locker->lock_write();	
	/* check again in case somebody just add that set in the cache */
	index = hash_key()&set_hmask;
	while (set_hash[index] != -1) {
		se = set_table+set_hash[index];
		if (check_params(&se->params)) {
			/* set_entry found. Update the index in the set_table */
			set_id = set_hash[index];
			locker = draw_locker[set_id & (FC_LOCKER_COUNT-1)];
			if (render && own_render)
				delete render;
			goto end_write;
		}
		index = (index+1)&set_hmask;
	}	

	if (free_set == -1) {
		/* expand the set tables. Lock all entries */
		for (i=0; i<FC_LOCKER_COUNT; i++)
			draw_locker[i]->lock_write();

		/* realloc the tables, update free list */
		set_table = (fc_set_entry*)grRealloc(set_table, sizeof(fc_set_entry)*set_table_count*2, "fc:set_table", MALLOC_CANNOT_FAIL);
		set_hash = (int*)grRealloc(set_hash, sizeof(int)*set_table_count*2*2, "fc:set_hash", MALLOC_CANNOT_FAIL);
		for (i=set_table_count; i<2*set_table_count; i++) {
			set_table[i].next = i+1;
			set_table[i].char_count = -1;
			set_table[i].invalid = TRUE;
		}
		set_table[2*set_table_count-1].next = -1;
		free_set = set_table_count;
		/* rehash the entries */
		set_hmask = 4*set_table_count-1;
		for (i=0; i<=set_hmask; i++)
			set_hash[i] = -1;
		for (i=0; i<set_table_count; i++) {
			index = set_table[i].params.hash_key() & set_hmask;
			while (set_hash[index] != -1)
				index = (index+1) & set_hmask;
			set_hash[index] = i;
		}
		set_table_count *= 2;
//		set_uid++;

		/* unlock all entries, but not the set tables */
		for (i=0; i<FC_LOCKER_COUNT; i++)
			draw_locker[i]->unlock_write();
	}

	/* get new set entry. Lock it to complete the initialisation */
	set_id = free_set;
	locker = draw_locker[set_id & (FC_LOCKER_COUNT-1)];
	locker->lock_write();
	se = set_table+set_id;
	free_set = se->next;
	/* estimation of memory allocation */
	step = calculate_step();
	se->char_hmask = 2*8-1;
	size = step + sizeof(fc_hset_item)*(se->char_hmask+1);
	/* init the new set entry */
	se->hit_ref = set_hit;
	se->params.copy(params());
	se->bonus = bonus;
	se->hash_char = (fc_hset_item*)grMalloc(sizeof(fc_hset_item)*(se->char_hmask+1),"fc:hash_char",MALLOC_CANNOT_FAIL);
	for (i=0; i<=se->char_hmask; i++)
		se->hash_char[i].offset = -1;
	se->offset_default = -1;
	se->char_buf = (char*)grMalloc(step,"fc:char_buf",MALLOC_CANNOT_FAIL);
	se->char_buf_end = 0;
	se->char_buf_size = step;
	se->char_buf_step = step;
	/* default values for the bands used for advanced spacing */
	se->limit[0] = -4;
	se->limit[1] = -8;
	/* mark the set entry as activated for the garbage collector */
	se->invalid = FALSE;
	se->flags = 0;
	if (_file_flags & FC_MONOSPACED_FONT)
		se->flags |= FC_MONOSPACED_FONT;
	if (_file_flags & FC_DUALSPACED_FONT)
		se->flags |= FC_DUALSPACED_FONT;
	se->char_count = 0;
	index = se->params.hash_key() & set_hmask;
	while (set_hash[index] != -1)
		index = (index+1) & set_hmask;
	set_hash[index] = set_id;
	/* update the status size values */
	atomic_add(&cs->cur_mem, size);
	/* unlock the entry temporary */
	locker->unlock_write();

	/***************************************************************
	  Creation completed. ready to set final lock.
	
	/* trick the system to render a first char for advanced spacing setting */
	if ((params()->size() < FC_SIZE_MAX_SPACING) && (render != 0L)) {
		/* set write mode for bitmap rendering */
		last_write_mode = TRUE;
		locker->lock_write();
		set_locker->unlock_write();
		/* try to render a "e", or the default_char if no "e" */
		code = 'e';
		offset = render->render_char(&code, se);
		if (offset == -2)
			se->offset_default = render->render_default_char(se);
		/* insert the new character in the hash table */
		index = ((code<<3)^(code>>2))&se->char_hmask;
		se->hash_char[index].offset = offset;
		se->hash_char[index].code[0] = code;
		se->hash_char[index].code[1] = 0;
		se->char_count++;
		/* set the real bands limits */
		if (offset == -2)
			offset = se->offset_default;
		my_char = (fc_char*)(se->char_buf+offset);
		se->limit[1] = my_char->bbox.top-1;
		se->limit[0] = se->limit[1]/2;
		/* set the real spacing of the first character */
		render->process_spacing(bits_per_pixel(), my_char, se);	
		if (own_render)
			delete render;
		goto last_end;
	}
 end_write:
	/* standard exit process */
	if (se->char_count < 0)
		xprintf("write exit bad\n");
	if (write_mode)
		locker->lock_write();
	else
		locker->lock_read();
	set_locker->unlock_write();
	goto last_end;
 end:
	/* exit for set_entry already existing */
	if (se->char_count < 0)
		xprintf("read exit bad\n");
	if (write_mode)
		locker->lock_write();
	else
		locker->lock_read();
	set_locker->unlock_read();
 last_end:
	set_hit++;
	if (*set_hit_ref == 0)
		*set_hit_ref = set_hit;
	else if (*set_hit_ref == se->hit_ref)
		*set_hit_ref = 0;
	se->hit_ref = set_hit;
	return se;
}

void fc_context::unlock_set_entry() {
	if (last_write_mode)
		locker->unlock_write();
	else
		locker->unlock_read();
}

int fc_context::calculate_step() {
	int        cnt, size=0;

	switch (_params._bits_per_pixel) {
	case FC_BLACK_AND_WHITE :
		size = 16*(_params._size*5);
		break;
	case FC_GRAY_SCALE :
	case FC_TV_SCALE :
		size = 7*_params._size*_params._size;
		break;
	}
	size += 16*sizeof(fc_char);
	for (cnt=0; cnt<3; cnt++)
		if (size > 4096)
			size >>= 1;
	cnt = 0;
	while (size > 1) {
		cnt++;
		size >>= 1;
	}
	/* no first allocation or initial step bigger than 4096 */
	if (cnt > 12)
		cnt = 12;
	return (1<<cnt);
}

void fc_context::debug() {
 	xprintf("********************\nstatus:%d\n", params()->status);
	xprintf("bpp:%d\n", params()->_bits_per_pixel);
	xprintf("hinting:%d\n", params()->_hinting);
	xprintf("family id:%d\n", params()->_family_id);
	xprintf("style id:%d\n", params()->_style_id);
	xprintf("size:%d\n", params()->_size);
	xprintf("algo_faces:%d\n", params()->_algo_faces);
	xprintf("rotation:%f\n", params()->_rotation);
	xprintf("shear:%f\n", params()->_shear);
	xprintf("hash key:0x%x\n", params()->_hash_key);
	xprintf("escape size:%f\n", _escape_size);
	xprintf("flags:%d\n", _flags);
	xprintf("final_flags:%d\n", _final_flags);
	xprintf("style_faces:%d\n", _style_faces);
	xprintf("spacing mode:%d\n", _spacing_mode);
	xprintf("file_flags:%d\n", _file_flags);
	xprintf("file type:%d\n", file_type);
	xprintf("file id:%d\n", file_id);
	xprintf("ascent:%f\n", _ascent);
	xprintf("descent:%f\n", _descent);
	xprintf("leading:%f\n", _leading);
	xprintf("encoding:%d\n", _encoding);
	xprintf("set_id:%d\n", set_id);
	xprintf("universal id:%d\n", uid);
	xprintf("locker:0x%x\n", locker);
	xprintf("matrix xxmult:%d\n", _matrix.xxmult);
	xprintf("matrix yxmult:%d\n", _matrix.yxmult);
	xprintf("matrix xymult:%d\n", _matrix.xymult);
	xprintf("matrix yymult:%d\n", _matrix.yymult);
	xprintf("matrix xoffset:%d\n", _matrix.xoffset);
	xprintf("matrix yoffset:%d\n", _matrix.yoffset);
	xprintf("last write mode:%d\n", last_write_mode);
	xprintf("origin x:%f\n", _origin_x);
	xprintf("origin y:%f\n", _origin_y);
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

static const uchar test_string[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._\00";
static sem_id xprintf_sem = -1;
static long xprintf_lock = 0;
static int32 error_count = 0;

int32 check_char(fc_char *my_char) {
	int32		i, err, size;
	uchar		*src_base;

	err = 0;
	if ((my_char->bbox.left < -30) ||
		(my_char->bbox.right > 30) ||
		(my_char->bbox.top < -30) ||
		(my_char->bbox.bottom > 30))
		err |= 1;
	if ((my_char->bbox.right < my_char->bbox.left-1) ||
		((my_char->bbox.right - my_char->bbox.left) > 30) ||
		(my_char->bbox.bottom < my_char->bbox.top-1) ||
		((my_char->bbox.bottom - my_char->bbox.top) > 30))
		err |= 2;
	if (err == 0) {
		size = ((my_char->bbox.right-my_char->bbox.left+2)>>1) * 
				(my_char->bbox.bottom-my_char->bbox.top+1);
		if (size < 0)
			err |= 4;
		else if (size > 0) {
			src_base = ((uchar*)my_char) + CHAR_HEADER_SIZE;
			for (i=0; i<size; i++)
				if (src_base[i] != 0)
					goto non_empty_char;
			err |= 4;
		}
		else if (size == 0)
			err |= 8;
	non_empty_char:;
	}
	return err;
}

#if 0
// get_unicode_128() no longer exists -- it has been subsumed into
// get_unicode_run() used by SFont -- so this functions can't compile.

bool fc_context::check_set_entry(char *debug_string, bool no_lock, void *hash_ptr) {
	int				count;
	char            *buf;
	char			r1[64];
	int32           key, index;
	int32           hmask, offset, i, j, err, tmp_err, safety;
	const uchar 	*string;
	uint32			hit_ref;
	ushort          uni_str[FC_MAX_STRING];
	ushort          code0, code1;
	fc_char         *my_char;
	fc_set_entry    *se;
	fc_hset_item    *hash;

	/* update the state if change of setting since previous call */
	se = set_table+set_id;
	/* check everything again in case of possible font file change */
	if ((set_uid == uid) && !se->invalid)
		if (check_params(&se->params))
			goto end;
	uid = set_uid;
	set_font_ids(family_id(), style_id(), false, true);
	/* look for the good set_entry descriptor */
	index = hash_key()&set_hmask;
	while (set_hash[index] != -1) {
		se = set_table+set_hash[index];
		if (check_params(&se->params) && !se->invalid) {
			/* set_entry found. Update the index in the set_table */
			set_id = set_hash[index];
			locker = draw_locker[set_id & (FC_LOCKER_COUNT-1)];
			goto end;
		}
		index = (index+1)&set_hmask;
	}
	return false;
	
end:
	if (se->char_count < 0) {
		xprintf("DEBUG ######### Really bad...\n");
		return false;
	}
	if (!no_lock)
		locker->lock_read();


	for (i=0; i<64; i++) r1[i] = 0;
	err = 0;
	j = 0;
	string = test_string;
	buf = se->char_buf;
	hash = se->hash_char;
	hmask = se->char_hmask;
	
	while (string != NULL) {
		/* translate the input string in unicode (including surrogate with UTF8 only) */
		string = get_unicode_128(this, string, uni_str, &count, FC_MAX_STRING);
		i = 0;
		while (i < count) {
			/* get the code of the character, including surrogates */
			code0 = uni_str[i++];
			if ((code0 & 0xfc00) == 0xd800)
				code1 = uni_str[i++];
			else
				code1 = 0;
			/* look for the character in the hash table */
			index = (((code0<<3)^(code0>>2))+code1)&hmask;
			safety = 128;
			while (true) {
				if (hash[index].offset == -1)
					goto cache_miss;
				if ((hash[index].code[0] == code0) && (hash[index].code[1] == code1))
					break;
				index = (index+1)&hmask;
				if (--safety == 0) {
					err |= 8;
					r1[i+j-1] = 8;
					goto cache_miss;
				}
			}
			/* get character descriptor */
			offset = hash[index].offset;
			if (offset == -2)
		    	offset = se->offset_default;
		    tmp_err = check_char((fc_char*)(buf+offset));
		    err |= tmp_err;
		    r1[i+j-1] |= tmp_err;
 	cache_miss:;
		}
		j += count;	
	}

	if (!no_lock)
		locker->unlock_read();

	if (err == 0)
		return true;
		
	if (xprintf_sem == -1) {
		if (atomic_add(&xprintf_lock, 1) == 0)
			xprintf_sem = create_sem(1, "xprintf sem");
		else while (xprintf_sem == -1)
			snooze(25000);
	}
	
	j = atomic_add(&error_count, 1);
	if (j < 64) {
		acquire_sem(xprintf_sem);
		xprintf("DEBUG ##### <<%s>> %s %s [%d] failed [%d], tid:%d (hash:%x/%x)\n",
				debug_string,
				fc_get_family_name(family_id()),
				fc_get_style_name(family_id(), style_id()),
				size(), j, find_thread(NULL),
				hash_ptr, hash);
		xprintf("%s\n", test_string);
		for (i=0; i<64; i++)
			xprintf("%c", (r1[i] != 0)?('0'+r1[i]):'.');
		xprintf("\n");
		release_sem(xprintf_sem);
	}
	return false;
}
#endif

void xprintf_sync(char *debug_string) {
	if (xprintf_sem == -1) {
		if (atomic_add(&xprintf_lock, 1) == 0)
			xprintf_sem = create_sem(1, "xprintf sem");
		else while (xprintf_sem == -1)
			snooze(25000);
	}
	acquire_sem(xprintf_sem);
	xprintf("##### DEBUG -> %s", debug_string);
	release_sem(xprintf_sem);
}

void xprintf_sync_in() {
	if (xprintf_sem == -1) {
		if (atomic_add(&xprintf_lock, 1) == 0)
			xprintf_sem = create_sem(1, "xprintf sem");
		else while (xprintf_sem == -1)
			snooze(25000);
	}
	acquire_sem(xprintf_sem);
}

void xprintf_sync_out() {
	release_sem(xprintf_sem);
}









