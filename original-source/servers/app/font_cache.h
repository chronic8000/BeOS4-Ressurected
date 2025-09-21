/* ++++++++++
	FILE:	font_cache.h
	REVS:	$Revision: 1.32 $
	NAME:	pierre
	DATE:	Tue Jan 28 10:45:35 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _FONT_CACHE_H
#define _FONT_CACHE_H

#include <sys/stat.h>
#include <stdio.h>
#include <OS.h>
#include <SupportDefs.h>
#include "font_defs.h"

// Be sure to look here for other font definitions.
#include "shared_fonts.h"

#define ENABLE_BENAPHORES  1

#define	ENABLE_GASP 0

/*******************************************************/
/* enums */
enum {
	FC_LOCKER_COUNT       = 16,
#if _TUNE_SMALL_MEMORY
	FC_FILE_OPEN_COUNT    = 4,
	FC_MAX_COUNT_RENDERER = 2,  /* you need FC_MAX_COUNT_RENDERER < FC_FILE_OPEN_COUNT */
#else
	FC_FILE_OPEN_COUNT    = 8,
	FC_MAX_COUNT_RENDERER = 4,  /* you need FC_MAX_COUNT_RENDERER < FC_FILE_OPEN_COUNT */
#endif
	FC_MAX_DIR_COUNT      = 1024,
	FC_MAX_FILE_COUNT     = 32767,
	FC_MAX_STYLE_COUNT    = 256,
	FC_MAX_FAMILY_COUNT   = 32767,
	FC_MAX_MISS_PER_RUN   = 3,
	FC_SIZE_MAX_CUSTOM    = 19,
	FC_SIZE_MAX_SPACING   = 19,
	FC_SIZE_MIN_SPACING   = 6,
	FC_GRAY_MAX_SIZE      = 1000,
	FC_BW_MAX_SIZE        = 10000,
	FC_SYSTEM_FONT_MIN    = 7,
	FC_SYSTEM_FONT_MAX    = 100,
/*
	FC_FONT_FILE_LIST_ID  = 0x1e833479,	Initial values. Increment by 1 for each revision.
	FC_FONT_STATUS_ID     = 0x3f833479, Initial version is R3.2 and sooner.
	FC_FONT_CACHE_ID      = 0x57833479,

	FC_FONT_FILE_LIST_ID  = 0x1e83347A,	// R4b1 and R4b2
	FC_FONT_STATUS_ID     = 0x3f833479,
	FC_FONT_CACHE_ID      = 0x5783347A, // R4
*/
/*
	FC_FONT_FILE_LIST_ID  = 0x1e83347B,	// R4b3 and after
*/
	FC_FONT_STATUS_ID_1   = 0x3f833479, // R4
/*
	FC_FONT_CACHE_ID      = 0x5783347A, // R4
*/
	FC_FONT_FILE_LIST_ID  = 0x1e83347C,	// Dano
	FC_FONT_STATUS_ID     = 0x3f83347A,	// Dano
	FC_FONT_CACHE_ID      = 0x5783347C, // Dano
	FC_FACE_MAX_MASK	  = 0x7f
};

/* enum used for font file properties (flags) */
enum {
	FC_MONOSPACED_FONT        = 0x01,
	FC_DUALSPACED_FONT        = 0x02
};

/* mode values to lock string descriptor */
enum {
	FC_DO_NOT_LOCK      = 1
};

enum {
	FC_EMPTY_CHAR = 1,
	FC_DO_EDGES = 2
};

/* cache priority control */
enum {
	FC_HIGH_PRIORITY_BONUS = 100000,
	FC_LOW_PRIORITY_BONUS  = 1000
};

/* mask used to set status bits */
enum {
	FC_SIZE_CHANGED     = 0x07,
	FC_ROTATION_CHANGED = 0x07,
	FC_SHEAR_CHANGED    = 0x07,
	FC_BPP_CHANGED      = 0x05,
	FC_HINTING_CHANGED  = 0x05,
	FC_FACES_CHANGED    = 0x01,

	FC_SET_CHANGED       = 0x01,
	FC_MATRIX_CHANGED    = 0x02,
	FC_TUNED_CHANGED     = 0x04,
	FC_ALL_CHANGED       = 0x07
};

/* values for fc_set_file command */
enum {
	FC_ENABLE_FILE      = 1,
	FC_DISABLE_FILE     = 2,
	FC_ANALYSE_FILE     = 3,
	FC_CHECK_FILE       = 4
};

/* family name extension status */
enum {
	FC_NOT_EXTENDED		= 0x01,
	FC_MASTER_EXTENDED	= 0x02,
	FC_SLAVE_EXTENDED	= 0x03
};

/* enum mask used for style status */
enum {
	FC_STYLE_ENABLE      = 0x01
};

/* special font renderer mode */
enum {
	FC_RENDER_BBOX 		= 1
};

/*******************************************************/
/* structures */

class SPath;

typedef struct {
	int32    xxmult;
	int32    yxmult;
	int32    xymult;
	int32    yymult;
	int32    xoffset;
	int32    yoffset;
} fc_matrix;


typedef struct {
	char            *dirname;   /* == 0 for unused */
	uchar           status;
	uchar           type;
	uchar           w_status;
	uchar           _reserved_;
	time_t          mtime;
} fc_dir_entry;


typedef uint32 fc_block_bitmap[256/32];
typedef struct {
	int32 first;
	int32 last;
} fc_char_range;


typedef struct {
	/* general file identification and properties */
	char            *filename;  /* == 0 for unused */
	union {
		int         key;
		int         next;       /* link to next free if unused */
	};
	ushort          dir_id;
	uchar           status;
	uchar           file_type;
	time_t          mtime;

	/* properties of real font files */
	ushort          family_id;
	ushort          style_id;
	uint            flags;
	float           ascent;
	float           descent;
	float           leading;
	float           size;
	uint16			faces;
	uint16			gasp[8];
	float			bbox[4];
	uint32			blocks[3];
	uint8			char_blocks[256];	// has_glyph for codes 0x0000-0xffff
	fc_block_bitmap	*block_bitmaps;
	int32			range_count;		// has_glyph for codes 0x10000 and above
	fc_char_range	*char_ranges;
} fc_file_entry;

static inline bool fc_fast_has_glyph(fc_file_entry* fe, uint16 code)
{
	return ( fe->block_bitmaps[fe->char_blocks[code/0x100]][(code&0xff)/32]
			 & (1<<(code&31)) )
			? true : false;
}


typedef struct {
	FILE    *fp;
	FILE    *fp_tuned;
	int     file_id;        /* or -1 if free */
	int     tuned_file_id;  /* or -1 if free */
	int     hit_ref;
	char    used;           /* 0 if available, 1 if used */
	char    _reserved_[3];
} file_item;


typedef struct {
	struct fc_renderer_context *renderer;
	int                        file_id;        /* or -1 if free */
	bigtime_t                  last_used;      /* system time of last use */
	char                       used;           /* 0 if available, 1 if used */
	char                       _reserved_[3];
} renderer_item;


typedef struct {
	int          file_id;       /* id of the main font file attached */
	uint16       key;
	uint16       offset;        /* data section, offset of beginning and length */
	float        rotation;      /* description */
	float        shear;
	uint32       hmask;
	uchar        bpp;
	uchar        used;          /* 0 if unused */
	uint16       size;
} fc_tuned_entry;


typedef struct {
	uint32       total_length;
	uint16       family_length;
	uint16       style_length;
	float        rotation;
	float        shear;
	uint32       hmask;
	uint16       size;
	uint8        bpp;
	uint8        version;
	uint32       _reserved_[2];
} fc_tuned_file_header; 


typedef struct {
	char           *name;         /* 0L for unused style */
	int32          file_id;       /* id of the main font file attached */
	uint8          status;
	uint8          used;          /* = 0 if no main font file */
	uint8          flags;         /* mirror of the font file flags */
	uint8          faces;		  /* 6 bits face incoding */
	uint16         tuned_count;   /* = 0 if empty */
	uint16         tuned_hmask;
	fc_tuned_entry *tuned_table;  /* = 0L when unused */
	uint16         *tuned_hash;   /* >= 0x8000 for unused entries */
	uint16		   gasp[8];
} fc_style_entry;


typedef struct {
	char         *name;
	union {
		int      next;
		int      hash_key;
	};
	int             file_count;    /* 0 for unused family */
	fc_style_entry  *style_table;
	ushort          style_table_count;
	ushort          enable_count;
	uint8			extension_status;
	uint8			_reserved_[3];
} fc_family_entry;


typedef struct {
	int32       offset;
	uint16      code[2];
} fc_hset_item;

typedef struct {
	uint16		endCount;
	uint16		startCount;
	uint16		idDelta;
	uint16		idRangeOffset;
} fc_cmap_segment;

/* class definition */
class fc_locker {
 private:
	int      reader_count;
	long     write_lock;
	sem_id   write_sem;
	long     lock;
	sem_id   sem;
 public:
	fc_locker(int count);
	~fc_locker();
	void lock_write();
	void unlock_write();
	void lock_read();
	void unlock_read();
};


class fc_cache_status {
 public:
	uint32    bits_per_pixel;
	int32     shear_bonus;
	int32     rotation_bonus;
	uint32    oversize;
	int32     oversize_bonus;
	long      cur_mem;
	long      max_mem;
	uint32    limit_size;

	fc_cache_status();
	~fc_cache_status();

	void      set(int bits_per_pixel,
				  int shear, int rotation,
				  int over, int over_bonus,
				  int max_mem, int limit_size);
	void      copy(fc_cache_status *from);
	void      add(fc_cache_status *sum);
	size_t    flattened_size() const;
	bool      flatten(char* buffer) const;
	bool      unflatten(const char* buffer, size_t size);
	int       bonus_setting(struct fc_context *context);
	void      assert_mem();
	void	  flush();
	void      get_settings(fc_font_cache_settings *set);
	void      set_settings(fc_font_cache_settings *set, int bpp);
 private:
	int       do_collect(int size);
};


typedef struct fc_params {
	uchar       status;         /* status bit (to reconigned modified context) */
	uchar       _bits_per_pixel; /* font context description */
	uchar       _hinting;
	uchar       _pad;
	ushort      _family_id;
	ushort      _style_id;
	ushort      _size;
	uint16      _algo_faces;     /* Handle B_BOLD_FACE and B_ITALIC_FACE */
	float       _rotation;
	float       _shear;
	int         _hash_key;       /* hash key of the current font context */

	void        update_hash_key();
	int         check_params(const fc_params *ref) const;
	void        copy(fc_params *ref);

#ifdef __INTEL__
	fc_params() : _rotation(0.0), _shear(0.0) {}
#endif

	fc_cache_status *cache_status();
	inline void  set_bits_per_pixel(int depth);
	inline void  set_hinting(bool enabled);
	inline void  set_family_id(int id);
	inline void  set_style_id(int id);
	inline void  set_size(int size);
	inline void  set_algo_faces(uint16 faces);
	inline void  set_rotation(float alpha);
	inline void  set_shear(float alpha);
	inline int   bits_per_pixel() const;
	inline bool  hinting() const;
	inline int   family_id() const;
	inline int   style_id() const;
	inline int   size() const;
	inline uint16 algo_faces() const;
	inline float rotation() const;
	inline float shear() const;
	inline int   hash_key() const;
} fc_params;


class fc_font_renderer;

class fc_context {
 public:
	uint16      file_id;
	uint16      tuned_file_id;      /* 0xffff if not valid */
	uint8       file_type;
	uint8       _reserved_;
	uint16      tuned_offset;       /* offset of beginning of hash table */
	uint32      tuned_hmask;
	uint32      _file_flags;
	float       delta_space_x;
	float   	delta_space_y;

 private:
	fc_params   _params;
	float       _ascent;
	float       _descent;
	float       _leading;

	float       _escape_size;
	uchar       _encoding;
	uchar       _spacing_mode;
	uint16      _style_faces;
	uint32      _flags;
	uint32      _final_flags;
	
	int         set_id;            /* internal glue */
	int         uid;
	fc_locker   *locker;
	
	fc_matrix   _matrix;
	
	float       _origin_x;
	float       _origin_y;
	
	int         last_write_mode; 

	fc_context(const fc_context&);
	
 public:
	fc_context();
	fc_context(const fc_context_packet *packet);
	fc_context(int family_id, int style_id, int size, bool allow_disable = FALSE);
	fc_context(const fc_context *ctxt);
	~fc_context();
	
	fc_context& operator=(const fc_context& c);
	
struct fc_set_entry  *lock_set_entry(int               write_mode,
									 uint32            *set_hit_ref,
									 fc_font_renderer  *render = 0L);
	void             unlock_set_entry();

	inline void      set_bits_per_pixel(int depth);
	inline void      set_hinting(bool enabled);
	inline void      set_family_id(int id);
	inline void      set_style_id(int id);
	inline void      set_rotation(float alpha);
	inline void      set_shear(float alpha);
	inline void      set_escape_size(float size);
	inline void      set_encoding(int id);
	inline void      set_spacing_mode(int mode);
	inline void      set_flags(uint32 flags);
	uint32           combine_flags(uint32 flags) const;
	inline void      set_final_flags(uint32 flags);
	void             set_faces(uint16 faces);
	inline void      set_origin(float x, float y);
	bool             set_font_ids(int family_id, int style_id,
								  bool allow_disable = false, bool keep_faces = false);
	void             set_to(const fc_context* c);
	void             set_context_from_packet(const fc_context_packet *packet, uint mask);
	void             get_packet_from_context(fc_context_packet *packet) const;
	
	void             update_bits_per_pixel();
	void             update_hinting();
	
	inline const fc_params *params() const;
	inline fc_params *params();
	inline int       bits_per_pixel() const;
	inline bool      hinting() const;
	inline int       family_id() const;
	inline int       style_id() const;
	inline int       size() const;
	inline float     rotation() const;
	inline float     shear() const;
	inline int       hash_key() const;
	inline float     escape_size() const;
	inline int       encoding() const;
	inline int       spacing_mode() const;
	inline uint32    flags() const;
	inline uint32    final_flags() const;
	uint16           faces() const;
	inline float     origin_x() const;
	inline float     origin_y() const;

	inline int       ascent() const;
	inline void      get_metrics(float *ascent, float *descent, float *leading) const;

	inline fc_matrix *matrix();
	void             set_matrix();

    inline fc_cache_status *cache_status();
	int              calculate_step();
	inline int       check_params(const fc_params *ref) const;
	void             debug();
	
	bool			 check_set_entry(char *debug_string,
									 bool no_lock = false,
									 void *hash_ptr = NULL);
};


class fc_font_processor {
 public:
	void        process_spacing(int32 bpp, fc_char *my_char, fc_set_entry *se);
};

class SPath;

class fc_font_renderer: public fc_font_processor {
 public:
	int          fp;
	int          hcache_offset;
	FILE         *fp_tuned;
	uchar        safety;
	uchar        _reserved_;
	uint16       open_file_index;
	uint32       hmask;
	uint32       tuned_offset;
	fc_context   *ctxt;
	fc_hset_item hcache[32];
	fc_renderer_context *r_ctxt;
	
	fc_font_renderer(fc_context *context);
	~fc_font_renderer();
	int         reset(int special_mode = 0);
	int			first_time_init(fc_file_entry *fe);
	int         render_char(const ushort *code, fc_set_entry *se);
	int         render_default_char(fc_set_entry *se);
	SPath		*get_char_outline(const ushort *code);
	void        process_edges(const ushort *code, fc_char *my_char, int set_mode = 0);
	void		process_bbox(const ushort *code, fc_bbox *bbox);
 private:
	int         set_specs(int default_flag);      
};


class fc_count_name {
 public:	
	typedef struct {
		char        *name;
		int         count;
	} item;

	item            *table;
	int             table_count;
	
	fc_count_name(int count0);
	~fc_count_name();

	char *register_name(char *name);
	void unregister_name(char *name);
};

/* struct with class definition dependencies */

typedef struct fc_set_entry {
	int         hit_ref;
	union {
		long    bonus;
		int     next;
	};
	fc_params   params;

	int          offset_default; /* -1 by default */
	int          char_count;  /* -1 for unused set entry */
	int          char_hmask;
	fc_hset_item *hash_char;
	char         *char_buf;
	int          char_buf_end;
	int          char_buf_size;
	int          char_buf_step;

	signed char  limit[2];    /* limit position for advanced spacing */
	uchar        invalid;     /* set to true when the set need to be trash asap */
	char         flags;		  /* inherited from its context */

	void         invalidate();
} fc_set_entry;

/*****************************************************************/
/* inline code for object support class */

void fc_params::set_bits_per_pixel(int depth) {
	if (_bits_per_pixel != depth) {
		_bits_per_pixel = depth;
		status |= FC_BPP_CHANGED;
	}
}

void fc_params::set_hinting(bool enabled) {
	if (_hinting != enabled) {
		_hinting = enabled;
		status |= FC_HINTING_CHANGED;
	}
}

void fc_params::set_family_id(int id) {
	_family_id = id;
}

void fc_params::set_style_id(int id) {
	_style_id = id;
}

void fc_params::set_size(int s) {
	if (_size != s) {
		_size = s;
		status |= FC_SIZE_CHANGED;
	}
}

void fc_params::set_algo_faces(uint16 f) {
	if (_algo_faces != f) {
		_algo_faces = f;
		status |= FC_FACES_CHANGED;
	}
}

void fc_params::set_rotation(float alpha) {
	if (_rotation != alpha) {
		_rotation = alpha;
		status |= FC_ROTATION_CHANGED;
	}
}

void fc_params::set_shear(float alpha) {
	if (_shear != alpha) {
		_shear = alpha;
		status |= FC_SHEAR_CHANGED;
	}
}

int fc_params::bits_per_pixel() const {
	return _bits_per_pixel;
}

bool fc_params::hinting() const {
	return _hinting;
}

int fc_params::family_id() const {
	return _family_id;
}

int fc_params::style_id() const {
	return _style_id;
}

int fc_params::size() const {
	return _size;
}

uint16 fc_params::algo_faces() const {
	return _algo_faces;
}

float fc_params::rotation() const {
	return _rotation;
}

float fc_params::shear() const {
	return _shear;
}

int fc_params::hash_key() const {
	return _hash_key;
}

void fc_context::set_bits_per_pixel(int depth) {
	_params.set_bits_per_pixel(depth);
}

void fc_context::set_hinting(bool enabled) {
	_params.set_hinting(enabled);
}

void fc_context::set_family_id(int id) {
	_params.set_family_id(id);
}

void fc_context::set_style_id(int id) {
	_params.set_style_id(id);
}

void fc_context::set_rotation(float alpha) {
	_params.set_rotation(alpha);
}

void fc_context::set_shear(float alpha) {
	_params.set_shear(alpha);
}

int fc_context::bits_per_pixel() const {
	return _params.bits_per_pixel();
}

bool fc_context::hinting() const {
	return _params.hinting();
}

int fc_context::family_id() const {
	return _params.family_id();
}

int fc_context::style_id() const {
	return _params.style_id();
}

int fc_context::size() const {
	return _params.size();
}

float fc_context::rotation() const {
	return _params.rotation();
}

float fc_context::shear() const {
	return _params.shear();
}

int fc_context::hash_key() const {
	return _params.hash_key();
}

int fc_context::check_params(const fc_params *params) const {
	return _params.check_params(params);
}

fc_cache_status *fc_context::cache_status() {
	return _params.cache_status();
}

const fc_params *fc_context::params() const {
	return &_params;
}

fc_params *fc_context::params() {
	return &_params;
}

void fc_context::set_escape_size(float size) {
	if (_escape_size != size) {
		_escape_size = size;
		if (size < 1.0) size = 1.0;
		_params.set_size((int)size);
	}
}

void fc_context::set_encoding(int id) {
	_encoding = id;
}

void fc_context::set_spacing_mode(int mode) {
	_spacing_mode = mode;
}

void fc_context::set_flags(uint32 flags) {
	_final_flags = _flags = flags;
}

void fc_context::set_final_flags(uint32 flags) {
	_final_flags = flags;
}

void fc_context::set_origin(float x, float y) {
	_origin_x = x;
	_origin_y = y;
}

float fc_context::escape_size() const {
	return _escape_size;
}

int fc_context::encoding() const {
	return _encoding;
}

int fc_context::spacing_mode() const {
	return _spacing_mode;
}

uint32 fc_context::flags() const {
	return _flags;
}

uint32 fc_context::final_flags() const {
	return _final_flags;
}

float fc_context::origin_x() const {
	return _origin_x;
}

float fc_context::origin_y() const {
	return _origin_y;
}

int fc_context::ascent() const {
	return (int)(_ascent*_escape_size);
}

void fc_context::get_metrics(float *as, float *des, float *lead) const {
	*as = _ascent*_escape_size;
	*des = _descent*_escape_size;
	*lead = _leading*_escape_size;
}

fc_matrix *fc_context::matrix() {
	return &_matrix;
}

inline ushort sub_style_key(ushort size, float *rot, float *sh, uchar bpp) {
	ushort       key;

	key = size*19;
	key ^= *(ushort*)rot;
	key += *(ushort*)sh;
	key ^= bpp;
	return key;
}

/*****************************************************************/
/* global font descriptors */

extern sem_id          file_sem;
extern long            file_lock;
extern sem_id          font_list_sem;
extern long            font_list_lock;
extern sem_id          open_file_sem;
extern long            open_file_lock;

extern fc_dir_entry    *dir_table;
extern int             dir_table_count;

extern fc_file_entry   *file_table;
extern int             *file_hash;
extern int             file_table_count;
extern int             free_file;
extern int             file_hmask;

extern fc_locker       *family_locker;
extern fc_family_entry *family_table;
extern fc_family_entry **family_ordered_index;
extern bool			   family_ordered_index_is_valid;
extern int32		   family_ordered_index_count;
extern int             *family_hash;
extern int             family_table_count;
extern int             free_family;
extern int             family_hmask;

extern fc_count_name   *style_name;

extern fc_locker       *draw_locker[FC_LOCKER_COUNT];
extern fc_locker       *set_locker;
extern fc_set_entry    *set_table;
extern int             *set_hash;
extern int             set_table_count;
extern int             free_set;
extern int             set_hmask;
extern int             set_uid;  /* +1 when a set_entry is trashed,
								    or tables are reallocated */
extern int             garbage_uid;
extern int             last_garbage_uid;
extern int             set_hit;

extern sem_id          render_sem;
extern long            render_lock;
extern sem_id          garbage_sem;
extern long            garbage_lock;

extern fc_cache_status bw_status, gray_status;
extern fc_cache_status fc_bw_system_status, fc_gray_system_status;
extern file_item       file_open_list[FC_FILE_OPEN_COUNT];

extern int32           preferred_antialiasing;
extern int32           aliasing_min_limit;
extern int32           aliasing_max_limit;
extern int32           hinting_limit;

extern bigtime_t       b_font_change_time;

extern renderer_item   r_ctxt_list[FC_MAX_COUNT_RENDERER];

/*****************************************************************/
/* prototypes */

typedef char* (*FC_GET_MEM)(void*, int);

/* endianess glue */

uint16  fc_read16(const void *adr);
uint32  fc_read32(const void *adr);
void    fc_write16(void *buf, uint16 value);
void    fc_write32(void *buf, uint32 value);

/* external lock */

void    fc_lock_font_list();
void    fc_unlock_font_list();
void    fc_lock_open_file();
void    fc_unlock_open_file();
void    fc_lock_collector();
void    fc_unlock_collector();
void    fc_lock_file();
void    fc_unlock_file();
void    fc_lock_render();
void    fc_unlock_render();

/* miscelleanous */

void    fc_multiply(fc_matrix* dest, const fc_matrix* m1, const fc_matrix* m2);
int     fc_hash_name(char *name);
char    *fc_get_mem(fc_set_entry *se, int size);
void    fc_get_app_settings(int bpp, long pid, fc_font_cache_settings *set);
void    fc_set_app_settings(int bpp, long pid, fc_font_cache_settings *set);
void    fc_release_app_settings(void *data);
char*   fc_convert_font_name_to_utf8(const char *name, int32 nameLength,
                                     int32 maxLength, uint16 platformID, uint16 languageID);

/* general management */

void    fc_start();
void    fc_stop();

/* Font directory registration */

bool    fc_read_dir_table(FILE *fp);
bool    fc_write_dir_table(FILE *fp);
void    fc_release_dir_table();

int     fc_get_dir_count(int *ref_id);
char    *fc_next_dir(int *ref_id, uchar *flag, int *dir_id, uchar *type);
long    fc_set_dir(char *name, int enable, int type = 0);
long    fc_remove_dir(char *name);
void    fc_scan_all_dir(bool force_check);
void	fc_dump_cache_state(char *filename);


/* Font file registration */

bool    fc_read_file_table(FILE *fp);
bool    fc_write_file_table(FILE *fp);
void    fc_release_file_table();

bool    fc_init_file_entry(fc_file_entry *fe);	
void    fc_flush_open_file_cache();
int     fc_get_file_count(int *ref_id);
fc_file_entry *fc_next_file(int *ref_id);
fc_file_entry *fc_set_file(int dir_id, char *name, int enable);
void    fc_set_file_status(fc_file_entry *fe, int status, int mask);
void    fc_remove_file(fc_file_entry *fe);
void    fc_get_pathname(char *name, fc_file_entry *fe);
void    fc_set_char_map(fc_file_entry *fe, const uint32 *bitmap,
                        bool update_unicode_blocks = true);
bool    fc_has_glyph_range(fc_file_entry *fe, uint16 first, uint16 last);

/* Font family API */ 

bool    fc_read_family_table(FILE *fp);
bool    fc_write_family_table(FILE *fp);
void    fc_release_family_table();

int     fc_register_family(char *name, char *extension);

int     fc_get_family_count(int *ref_id);
void    fc_release_family_count();
char    *fc_next_family(int *ref_id, uchar *flag);
int     fc_get_family_id(const char *family_name);
char    *fc_get_family_name(int family_id);
int     fc_get_default_family_id();
int		fc_hash_family_name(const char *name);

/* Font style API */

bool    fc_read_style_table(FILE *fp, fc_family_entry *family);
bool    fc_write_style_table(FILE *fp, fc_family_entry *family);
void    fc_release_style_table(fc_family_entry *fe);

bool    fc_read_tuned_table(FILE *fp, fc_style_entry *style);
bool    fc_write_tuned_table(FILE *fp, fc_style_entry *style);

fc_style_entry *fc_set_new_style(fc_family_entry *ff, char *name);
int     fc_set_style(int family_id, char *name, int file_id,
					 uchar flags, uint16 faces, uint16 *gasp, bool enable);
int     fc_add_sub_style(int family_id, char *name, fc_tuned_entry *te);
void    fc_purge_set_list(int family_id, int style_id, fc_tuned_entry *te);
void    fc_do_clean_style_and_family(fc_style_entry *fs, fc_family_entry *ff);
void    fc_remove_style(int family_id, int style_id);
void    fc_remove_sub_style(fc_file_entry *fe, int file_id);

int     fc_get_style_count(int family_id, int *ref_id);
void    fc_release_style_count();
char    *fc_next_style(int family_id, int *ref_id, uchar *flag, uint16 *face);
int     fc_get_style_id(int family_id, const char *style_name);
char    *fc_get_style_name(int family_id, int style_id);
uint    fc_get_style_file_flags_and_faces(int family_id, int style_id, uint16 *faces);
int     fc_get_default_style_id(int family_id);
int		fc_get_closest_face_style(int family_id, int faces);
void 	fc_get_style_bbox_and_blocks(int family_id, int style_id,
									 float *bbox, uint32 *blocks);
int		fc_get_tuned_count(int family_id, int style_id);
int		fc_next_tuned(int family_id, int style_id, int ref_id, fc_tuned_font_info *info);

/* Font set API */

void    fc_flush_font_set(fc_context *fc_context);
int     fc_save_full_set(fc_context    *context,
						 ushort        *dir_id,
						 char          *filename,
						 char          *pathname,
						 fc_file_entry **fe);
bool    fc_read_set_from_file(FILE *fp, fc_set_entry *set);
bool    fc_write_set_to_file(FILE *fp, fc_set_entry *set);
bool    fc_read_char_from_file(FILE *fp, fc_set_entry *set, FC_GET_MEM get_mem);
bool    fc_seek_char_from_file(FILE *fp);
bool    fc_write_char_to_file(FILE *fp, fc_char *fchar);

/* Font list/font cache save and restore */

void	fc_init_file_paths();
void	fc_uninit_file_paths();
bool    fc_load_font_file_list();
void    fc_save_font_file_list();
void    fc_set_default_font_file_list();
void    fc_load_font_cache();
void    fc_save_font_cache();

void    fc_check_cache_settings();

/* String metric and rasterizing API */

void	fc_lock_string(fc_context *context, fc_string *draw,
						const uint16* string, int32 length,
						float non_space_delta, float space_delta,
						double *xx, double *yy, uint32 lock_mode = 0);
void	fc_unlock_string(fc_context *context); 

void	fc_get_escapement(fc_context *context, fc_escape_table *table,
							const uint16 *string, int32 length,
							fc_string *draw, float delta_nsp = 0.0, float delta_sp = 0.0,
							bool use_real_size = false, bool keep_locked = false);
void	fc_get_glyphs(fc_context *context, fc_glyph_table *g_table,
						const uint16 *string, int32 length);
void	fc_get_edge(fc_context *context, fc_edge_table *table,
					const uint16 *string, int32 length);
void	fc_get_bbox(fc_context *context, fc_bbox_table *b_table,
					const uint16 *string, int32 length,
					float delta_nsp, float delta_sp, int mode, int string_mode,
					double *offset_h, double *offset_v);
bool	fc_has_glyphs(fc_context *context,
						const uint16 *str, int32 length, bool *has);
void	fc_add_string_width(fc_context *context, const uint16 *string, int32 length,
							float escape_factor, float *inout_x, float *inout_y);

/* flush font file font engine internal cache */
void	fc_close_and_flush_file(FILE *fp);
extern "C" void	fc_flush_cache(int32 clientID);

/* Restricted to internal use and debug */
bool	load_cmap_table(FILE				*fp,
						uint32				offset,
						fc_cmap_segment		**list,
						int32				*seg_count,
						uint16				**glyphArray);
const uchar	*get_unicode_128(fc_context *ctxt,
							 const uchar *string,
							 ushort     *uni_string,
							 int        *count,
							 int        max0);
void	xprintf_sync(char *debug_string);
void	xprintf_sync_in();
void 	xprintf_sync_out();
int32	check_char(fc_char *my_char);

#endif
