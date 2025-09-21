#ifndef CFS_H
#define CFS_H

#include <SupportDefs.h>
#include <ByteOrder.h>

#ifdef USE_DMALLOC
#define calloc   dcalloc
#define realloc  drealloc
#define malloc   dmalloc
#define cfree    dcfree
#define free     dfree
#define strdup   dstrdup 
#endif

#define CFS_ID_1 B_HOST_TO_BENDIAN_INT32('cfs1')
#define CFS_ID_2 B_HOST_TO_BENDIAN_INT32('cfs2')
#define CFS_MIN_SUPPORTED_VERSION 1
#define CFS_CURRENT_VERSION 2

#define cfs_superblock_1_offset ((off_t)512)

//#define cfs_compressed_block_size 8192
#define cfs_compressed_block_size 16384
//#define cfs_compressed_block_size 32768
//#define cfs_compressed_block_size 65536

#define cfs_align_block(x) (((x) + 7) & ~7)
#define cfs_string_size(x) (cfs_align_block(strlen(x)+1))

typedef struct cfs_super_block_1 {
	int32 cfs_id_1;
	int32 cfs_version;
	int32 flags;
	int32 create_time;
	int32 dev_block_size;
	int32 fs_block_size;
	off_t size;
	off_t super_block_2;
	off_t root;
	off_t log_pos;
	off_t log_size;
	off_t reserved_space_1_pos;
	off_t reserved_space_1_size;
	off_t reserved_space_2_pos;
	off_t reserved_space_2_size;
} cfs_super_block_1;

#define CFS_SB1_FLAGS_FS_READ_ONLY 0x00000001

typedef struct cfs_super_block_2 {
	int32 cfs_id_2;
	int32 cfs_version;
	int32 flags;				/* 0 */
	int32 modification_time;
	off_t free_list;
	off_t unlinked_entries;
	uint8 volume_name[32];
	// added for version 2
	off_t free_list_end;
} cfs_super_block_2;

#define V2_SB2_LEN (sizeof(cfs_super_block_2))
#define V1_SB2_LEN (sizeof(cfs_super_block_2) - sizeof(off_t))

#define get_sb2_size(version) (((version)>1) ? V2_SB2_LEN : V1_SB2_LEN)

#define CFS_LOG_ID B_HOST_TO_BENDIAN_INT32('logh')
typedef struct cfs_log_info {
	int32	log_id;
	uint32	version;
	off_t	head;
	off_t	tail;
	uint32	last_version;
	int32	checksum;
} cfs_log_info;

#define CFS_LOG_ENTRY_ID B_HOST_TO_BENDIAN_INT32('loge')

typedef struct cfs_log_entry_header {
	uint32 log_entry_id;
	uint32 num_blocks;
	off_t  blocks[0];
//	off_t  blocks[max_blocks_per_log_entry];
} cfs_log_entry_header;
#define max_blocks_per_log_entry(dev_block_size) \
	(((dev_block_size) - sizeof(cfs_log_entry_header)) / sizeof(off_t))

// free block list structure. This is placed in head of a free space block.
// Since the smallest free space block is 8 bytes, the case of the structure
// being 8 or 16 bytes long is handled by storing 1 or 2 respectively in the lower
// 3 bits of the next pointer. This is valid because the next pointer can only point
// at 8 byte boundaries, so the bottom 3 bits are unused.
typedef struct cfs_free_block {
	off_t next;	// if next & 0x3 == 1: size = 8, if next & 0x3 == 2: size = 16			
	union {
		struct {
			off_t size;
			off_t unused;
		} v1;
		struct {
			off_t prev;
			off_t size;
		} v2;
	} u;
} cfs_free_block;

#define V2_FREEBLOCK_LEN (sizeof(cfs_free_block))
#define V1_FREEBLOCK_LEN (sizeof(cfs_free_block) - sizeof(off_t))

#define V2_FREEBLOCK_SIZE_FIELD(freeblock_ptr) ((freeblock_ptr)->u.v2.size)
#define V1_FREEBLOCK_SIZE_FIELD(freeblock_ptr) ((freeblock_ptr)->u.v1.size)
#define V2_FREEBLOCK_SIZE_FIELD_NOPTR(freeblock) ((freeblock).u.v2.size)
#define V1_FREEBLOCK_SIZE_FIELD_NOPTR(freeblock) ((freeblock).u.v1.size)

#define FREEBLOCK_SIZE_FIELD(freeblock_ptr, version) \
	(((version)>1) ? V2_FREEBLOCK_SIZE_FIELD(freeblock_ptr) : \
	V1_FREEBLOCK_SIZE_FIELD(freeblock_ptr))
#define FREEBLOCK_SIZE_FIELD_NOPTR(freeblock, version) \
	(((version)>1) ? V2_FREEBLOCK_SIZE_FIELD_NOPTR(freeblock) : \
	V1_FREEBLOCK_SIZE_FIELD_NOPTR(freeblock))

#define get_freeblock_size(version) \
	(((version)>1) ? V2_FREEBLOCK_LEN : V1_FREEBLOCK_LEN)

typedef struct cfs_data_block {
	off_t  next;
	off_t  data;
	size_t disk_size;
	size_t raw_size;			/* if zero, uncompressed */
} cfs_data_block;

#define cfs_vnid_to_offset(vnid) ((vnid) & 0xffffffffffff)
#define cfs_vnid_to_version(vnid) ((uint16)((vnid) >> 48))
#define cfs_offset_to_vnid(offset, version) \
	((off_t)(offset) | (off_t)(version) << 48)
#define cfs_inc_version(version) ((version) = ((version)+1) & 0x7fff)

typedef struct cfs_entry_info {
	off_t  parent;
	off_t  next_entry;
	off_t  data;	                /* block list / directory entries */
	off_t  attr;
	off_t  name_offset;
	uint32 create_time;
	uint32 modification_time;
	uint32 flags;
	uint16 cfs_flags;
	uint16 version;                 /* top bits of vnid */
	/* version 2 additions */
	off_t  logical_size; 			/* logical file size */
} cfs_entry_info;

#define V2_ENTRY_LEN (sizeof(cfs_entry_info))
#define V1_ENTRY_LEN (sizeof(cfs_entry_info) - sizeof(off_t))

#define get_entry_size(version) (((version)>1) ? V2_ENTRY_LEN : V1_ENTRY_LEN)

#define CFS_FLAG_COMPRESS_FILE 0x0001
#define CFS_FLAG_UNLINKED_ENTRY 0x0002

#define CFS_FLAG_DEFAULT_FLAGS CFS_FLAG_COMPRESS_FILE

#define CFS_MAX_NAME_LEN 256

#if 0
#define CFS_NODE_ENTRY_MIN_SIZE 40
#define CFS_NODE_ENTRY_NAME_LEN 256

#define CFS_NODE_FLAGS_TYPE       0x03
#define CFS_NODE_FLAGS_TYPE_ATTR  0x06
#define CFS_NODE_FLAGS_TYPE_DIR   0x01
#define CFS_NODE_FLAGS_TYPE_FILE  0x02
#define CFS_NODE_FLAGS_TYPE_LINK  0x03
#define CFS_NODE_FLAGS_EXECUTE    0x10
#define CFS_NODE_FLAGS_WRITE      0x20
#endif

#endif
