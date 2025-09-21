#ifndef NTFS_H
#define NTFS_H

//#include <errno.h>
//#include <time.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <dirent.h>
#include <ByteOrder.h>
#include <KernelExport.h>

#include "export.h"
#include "ntfs_fs.h"
#include "fsproto.h"
#include "lock.h"
#include "cache.h"


#define MAX_MFT_RECORDS 2048
#define MAX_FILENAME_LENGTH 256
#define MAX_ATT_NAME_LENGTH 256
#define MAX_VOLUME_NAME_LENGTH 256

#define COMPRESS_BLOCK_SIZE 4096
#define MAX_BLOCK_COMPRESS_SIZE 4610
#define DECOMPRESSION_BUFFERS 16

#ifdef USE_DMALLOC
	#include <dmalloc.h>

	#ifndef NTFS_MALLOC
		#define ntfs_malloc malloc
		#define ntfs_free free
	#endif

#else

/* allocate memory from swappable heap */
	#ifndef NTFS_MALLOC
		#define ntfs_malloc smalloc
		#define ntfs_free sfree
	#else
		#define malloc smalloc
		#define free sfree	
	#endif
	#define calloc scalloc
	#define realloc srealloc
	#include <kalloc.h>
#endif

// Set up some byte-swapping macros
#define	LENDIAN_TO_HOST16(x) (x)=B_LENDIAN_TO_HOST_INT16(x)
#define	LENDIAN_TO_HOST32(x) (x)=B_LENDIAN_TO_HOST_INT32(x)
#define	LENDIAN_TO_HOST64(x) (x)=B_LENDIAN_TO_HOST_INT64(x)

#define HOST_TO_LENDIAN16(x) (x)=B_HOST_TO_LENDIAN_INT16(x)
#define HOST_TO_LENDIAN32(x) (x)=B_HOST_TO_LENDIAN_INT32(x)
#define HOST_TO_LENDIAN64(x) (x)=B_HOST_TO_LENDIAN_INT64(x)

#define strip_high_16(x) ((x) &  0x0000FFFFFFFFFFFFLL)
#define get_high_16(x) ((uint16)((x) >> 48))
#define high_4bits(a) (((a) & 0xF0) >> 4)
#define low_4bits(a)  ((a) & 0x0F)

/* to keep it from whining about unused vars */
#define TOUCH(x) ((void)(x))

/* define some debug stuff */
#define ALL 0
#define IO 1
#define FILE 2
#define DIR 3
#define EXTENT 4
#define ALLOC 5
#define FS 6
#define VNODE 7
#define ATT 8
#define MALLOC 9
#define DMFT 10
#define DIRWALK 11
#define RUNLIST 12
#define ATTR 13

extern uint8 ntfs_debuglevels[];

#ifdef DEBUG
	#define DEBUGPRINT(x,y,z) if((ntfs_debuglevels[x] >= y) || (ntfs_debuglevels[ALL] >= y)) dprintf z
#else
	#define DEBUGPRINT(x,y,z)
#endif

#define ERRPRINT(x) dprintf x

#define ROOTINODE 5
#define MAX_READERS 10000

// vnode structure
typedef struct vnode {
	vnode_id vnid;
	uint16 sequence_num;			

	bool is_dir;
	uint16 hard_link_count;
	
	// stores a string for the mime type
	const char *mime;

	// This is the main attribute list
	uint32 attr_list_size;
	void *attr_list;
	void *last_attr;
	
	struct vnode *next;
} vnode;

typedef struct {
	// The following uniquely identify this compression buffer
	vnode_id vnid;
	uint64 compression_group;
	uint32 att_id;
	int index;
	
	// time stamps the last time it's been gotten
	bigtime_t timestamp;	
	// just a lock associated with the buffer. Can be used by anyone.
	lock buf_lock;
	int use_count;	

	// This is used by the 
	bool initialized;

	// Dis be de data
	void *compressed_buf;
	void *buf;
} decompression_buf;

// Main filesystem structure
typedef struct  {
	int fd;
	nspace_id nsid;	

	// It locks around copying a dircookie so rewinddir cant mess it up
	// while readdir is copying it.
	lock dir_lock;

	ntfs_bios_block nbb;
	uint32 cluster_size;
	uint32 clusters_per_compressed_block;
	uint32 mft_recordsize;
	uint64 free_blocks;
	
	uint64 num_cache_blocks;
	uint32 cache_block_size;
	uint32 cache_blocks_per_cluster;
	bool cache_initialized;
	bool removable;
	
	char *device_name;
	// This stores a pointer to the vnode for the $MFT and %Volume files
	vnode *MFT;
	vnode *VOL;
	
	// points to the upcase file
	vnode *UpCase;
	uint16 upcase[64*1024]; // This stores the lower -> uppercase unicode conversion
		
	// pointa to da bitmap. used for munge bitmap ioctl
	vnode *BITMAP;

	// store a pool of decompression buffers
	decompression_buf dbuf[DECOMPRESSION_BUFFERS];
	mlock d_buf_mlock;
	lock  d_search_lock;
} ntfs_fs_struct;

#define DC_IN_ROOT 0x1
#define DC_AT_END  0x2
#define DC_READ_DOT 0x4
#define DC_READ_DOTDOT 0x8

typedef struct dircookie_struct {
	// stuff to store the position
	uint64 next_index_buffer; // stores the next VCN to look in
	uint32 next_pos; // stores the next position of the VCN
	int flags;
} dircookie;

typedef struct {
	bool read_sniffed_mimetype;

	uint32 data_attr_num;
} attrdircookie;

#define EXTENT_FLAG_COMPRESSED 1
#define EXTENT_FLAG_LAST_GROUP_COMPRESSED 2
#define EXTENT_FLAG_SPARSE 4

// Stores the blocks the file is stored physically on
typedef struct file_extent_struct {
	uint64 start, end;
	uint64 length;
	uint64 compression_group_start; // Stores which 16 block compression group this is part of
	uint64 compression_group_end;   // stores the last compression group this spans into
	uint8  start_block_into_compression_group; 	// stores what block into the first compression group
											   	// this extent starts at
	uint8  end_block_into_compression_group;    // stores the block into the last compression group
												// this extent expands into
	int flags;
	struct file_extent_struct *next;
} file_extent;


typedef struct comp_file_extent_struct {
	uint64 blocks[16];
	uint64 compression_group;
	int flags;
	uint8 num_blocks;
} comp_file_extent;

typedef struct {
	file_extent *extent_head;
	file_extent *extent_tail;
	bool compressed;
	uint32 att_id;
	vnode_id vnid;
	uint64 num_file_blocks;
	uint64 uninit_data_start_block;  // stores the file block where uninitialized data will start 
	uint32 uninit_data_start_offset; // stores the offset into the block where udata will start
} extent_storage;

// This overlays the attribute structures to get to the type and next pointer
typedef struct {
	uint32 type;
	void *next;
} attr_header;

// data attribute vnode structure
typedef struct {
	uint32 type;
	void *next;
	char *name;
	
	uint64 length;
	
	bool compressed;
	
	// Is the data stored entirely within the MFT record?
	bool stored_local;
	void *data; // If so, store it here
	
	// If it's not local, need to store an extent
	extent_storage extent;
} data_attr;

typedef struct {
	uint32 type;
	void *next;
	char *name;

	uint64 file_length;	
	uint8 name_length;
	uint8 name_type;
	
	FILE_REC dir_file_rec;
	
	time_t file_creation_time;
	time_t last_modification_time;
	time_t last_FILE_rec_mod_time;
	time_t last_access_time;
	
	uint64 flags;
} file_name_attr;

typedef struct {
	uint32 type;
	void *next;
	
	time_t file_creation_time;
	time_t last_modification_time;
	time_t last_FILE_rec_mod_time;
	time_t last_access_time;

	uint32 DOS_permissions;	
} std_info_attr;

typedef struct {
	uint32 type;
	void *next;
	
	char name[MAX_VOLUME_NAME_LENGTH];
} volume_name_attr;

// index allocation attribute vnode structure
typedef data_attr index_alloc_attr;

// index root attribute vnode structure
typedef struct {
	uint32 type;
	void *next;
	
	uint64 length;
	
	uint32 index_buffer_size;
	
	void *entries;
} index_root_attr;

typedef struct {
	FILE_REC record;

	char name[MAX_FILENAME_LENGTH+1];
	uint8 name_type;
} dir_entry;

// Function declarations for ntfs.c
status_t ntfs_read_bios_block(ntfs_fs_struct *ntfs);
ntfs_fs_struct *create_ntfs_fs_struct();
status_t ntfs_init_cache (ntfs_fs_struct *ntfs);
status_t remove_ntfs_fs_struct(ntfs_fs_struct *ntfs);
void ntfs_initialize_vnode(vnode *new_vnode);
void ntfs_deallocate_vnode(vnode *killit);

#ifdef NTFS_MALLOC
void *ntfs_malloc(size_t size);
void ntfs_free(void *ptr);
#endif

#endif
