#ifndef _BFS_H
#define _BFS_H

#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "compat.h"

#include "fsproto.h"

#include "lock.h"
#include "cache.h"




typedef struct block_run
{
	int32   allocation_group;
	uint16  start;
	uint16  len;       /* in blocks */
} block_run;


#define block_run_to_bnum(x)                                       \
           ((((dr9_off_t)(x).allocation_group) << bfs->dsb.ag_shift) | \
			 (dr9_off_t)(x).start)

#define bnum_to_block_run(br, bn) {                                    \
		  (br).allocation_group = ((bn) >> (bfs->dsb.ag_shift));       \
		  (br).start = ((bn) & ((1 << bfs->dsb.ag_shift) - 1));        \
		  if ((br).allocation_group >= bfs->dsb.num_ags)               \
			  bfs_die("br @ 0x%lx bad ag %d (max %d) bnum %Ld %s:%d\n",\
					  (ulong)&(br), (br).allocation_group, bfs->dsb.num_ags, \
					  (dr9_off_t)(bn), __FILE__, __LINE__);            \
	  }

typedef block_run inode_addr;

#define inode_addr_to_vnode_id(ia)      block_run_to_bnum(ia)
#define vnode_id_to_inode_addr(ia, vn)  bnum_to_block_run(ia, vn)

#define NUM_DIRECT_BLOCKS    12

typedef struct data_stream
{
	block_run direct[NUM_DIRECT_BLOCKS];
	dr9_off_t     max_direct_range;
	block_run indirect;
	dr9_off_t     max_indirect_range;
	block_run double_indirect;
	dr9_off_t     max_double_indirect_range;  /* not really necessary */
	dr9_off_t     size;
} data_stream;


struct bt_index;
struct bfs_inode;

typedef struct index_callback {
	int (*func)(int               type_of_event,
				struct bfs_inode *bi,
				int              *old_result,
				void             *arg);
	void *arg;

	struct index_callback *next;
} index_callback;

/* types of events that an index_callback can have */
#define NEW_FILE    0x0001
#define DELETE_FILE 0x0002
#define MODIFY_FILE 0x0004


struct my_hash_table;

typedef struct binode_etc {      /* these fields are needed when in memory */
	vnode_id          vnid;
	struct bt_index  *btree;     /* only if this is a directory */
	uint              counter;   /* incremented on access to the btree */
    lock              lock;      /* guard access to this inode */
	dr9_off_t         old_size;  /* needed for size index maintenance */
	bigtime_t         old_mtime; /* needed for mod. time index maintenance */
	char             *tmp_block; /* for un-cached files, NULL otherwise */
	int32			  ag_hint;   /* where to start looking for free blocks */
	int32			  ind_block_index_hint; /* where to insert in an indirect block */

	struct bfs_inode *attrs;     /* inode of attribute directory */

	index_callback   *callbacks; /* only for indices (used for live queries) */
	struct my_hash_table *ht;    /* only for INODE_NO_CACHE files */
} binode_etc;


#define MAX_INODE_ACCESS  1024

#define TIME_SCALE   (16)                    /* amount to scale time_t's by */ 
#define TIME_MASK    ((1 << TIME_SCALE) - 1)
#define MAKE_TIME(x) (((bigtime_t)((uint)(x)) << TIME_SCALE) | \
 				       (bigtime_t)((bfs->counter++) & TIME_MASK))
#define CUR_TIME     MAKE_TIME(time(NULL))

#define MAX_INDEXED_DATA_SIZE 256  /* max size of data we can index */

/*
   inodes are always at least this big (because we store extra data
   after the bfs_inode structure).  the actual size of the inode
   is either MIN_INODE_SIZE or the block size of the file system,
   whichever is larger.
*/   
#define MIN_INODE_SIZE  512


typedef struct bfs_inode
{
	int32        magic1;
	inode_addr   inode_num;
	int32        uid;
	int32        gid;
	int32        mode;
	int32        flags;
	bigtime_t    create_time;
	bigtime_t    last_modified_time;
	inode_addr   parent;        /* points to the directory that contains us */
	inode_addr   attributes;    /* points to an attribute inode "directory" */
	uint32       type;          /* if this is an attribute, this is its type */

	int32        inode_size;    /* in bytes of the entire inode */
	binode_etc  *etc;           /* fields only used in memory */

	data_stream  data;
	int32        pad[4];        /* for the future... */
	int32        small_data[1]; /* small_data follows in memory... */
} bfs_inode;


#define INODE_MAGIC1      0x3bbe0ad9

#define INODE_IN_USE      0x00000001
#define ATTR_INODE        0x00000004  /* this inode refers to an attribute */
#define INODE_LOGGED      0x00000008  /* log all i/o to this inode's data */
#define INODE_DELETED     0x00000010  /* this inode has been deleted */
#define INODE_VIRGIN      0x00000020  /* this i-node isn't ready yet */
#define LONG_SYMLINK      0x00000040  /* symlink is stored separately */

#define PERMANENT_FLAGS   0x0000ffff  /* mask for permanent flags */

#define INODE_NO_CACHE    0x00010000  /* never cache any data for this inode */
#define INODE_WAS_WRITTEN 0x00020000  /* this inode has had its data written */
#define NO_TRANSACTION    0x00040000  /* don't start transactions for us */
#define TRIMMING_INODE    0x00080000  /* i-node is being trimmed back */
#define LOCKED_FS         0x00100000  /* this i-node locked the fs */

#define CHECK_INODE(bi)    {                                        \
	if ((bi)->magic1 != INODE_MAGIC1) {                             \
		bfs_die("inode @ 0x%lx: magic is wrong (0x%x) at %s:%d\n",  \
				(ulong)(bi), (bi)->magic1, __FILE__, __LINE__);     \
		return EBADF;                                               \
	} else if ((bi)->inode_size != bfs->dsb.inode_size) {           \
		bfs_die("inode @ 0x%lx: size is bad (0x%x) at %s:%d\n",     \
				(ulong)(bi), (bi)->inode_size, __FILE__, __LINE__); \
		return EBADF;                                               \
    } else if (bi->etc == NULL) {                                   \
		bfs_die("inode @ 0x%lx: null etc ptr at %s:%d\n",           \
				(ulong)(bi), __FILE__, __LINE__);                   \
		return EBADF;                                               \
	} else if (bi->data.size < 0) {                                 \
		bfs_die("inode @ 0x%lx: size is less than zero %s:%d\n",    \
				(ulong)(bi), __FILE__, __LINE__);                   \
		return EBADF;                                               \
	}                                                               \
}



/*
   we keep a packed array of these structures in the "small_data" field
   of the disk_inode structure.  The data follows the name bytes.
*/   
typedef struct small_data {
	uint32    type;
	uint16    name_size;
	uint16    data_size;
	char      name[1];
} small_data;

typedef struct dir_cookie {
	vnode_id  vnid;
	ulong     counter;
	char      name[B_FILE_NAME_LENGTH];
} dir_cookie;




typedef struct disk_super_block          /* super block as it is on disk */
{
	char         name[B_OS_NAME_LENGTH];
	int32        magic1;
	int32        fs_byte_order;

	uint32       block_size;             /* in bytes */
	uint32       block_shift;            /* block_size == (1 << block_shift) */

    dr9_off_t    num_blocks;
	dr9_off_t    used_blocks;

	int32        inode_size;             /* # of bytes per inode */

	int32        magic2;
	int32        blocks_per_ag;          /* in blocks */
    int32        ag_shift;               /* # of bits to shift to get ag num */
	int32        num_ags;                /* # of allocation groups */
	int32        flags;                  /* if it's clean, etc */

	block_run    log_blocks;             /* a block_run of the log blocks */
	dr9_off_t    log_start;              /* block # of the beginning */
	dr9_off_t    log_end;                /* block # of the end of the log */

	int32        magic3;
	inode_addr   root_dir;
	inode_addr   indices;

	int32        pad[8];                 /* extra stuff for the future */
} disk_super_block;


#define BFS_CLEAN   0x434c454e           /* 'CLEN', for flags field */ 
#define BFS_DIRTY   0x44495254           /* 'DIRT', for flags field */ 


typedef struct block_bitmap
{
	struct _BitVector *bv;      /* an array, one per allocation group */

	dr9_off_t      num_bitmap_blocks;
} block_bitmap;


typedef struct index_list
{
	char               name[B_FILE_NAME_LENGTH];
	struct bt_index   *btree;
	struct index_list *next;
} index_list;


#define NUM_TMP_BLOCKS   16

typedef struct tmp_blocks
{
	struct tmp_blocks *next;
	char              *data;
	char              *block_ptrs[NUM_TMP_BLOCKS];
} tmp_blocks;


typedef struct log_list {
    dr9_off_t            start, end;
    struct log_list *next;
} log_list;


struct log_handle;

typedef struct bfs_info
{
	nspace_id          nsid;
	disk_super_block   dsb;     /* as read from disk */

	block_bitmap       bbm;     /* keeps track of which blocks are allocated */
	sem_id             bbm_sem;

	sem_id             sem;     /* guard access to this structure */

	int                fd;	    /* the device we're using */
	int                flags;   /* read-only, etc */

	/* size in bytes of the underlying device block size */
	uint               dev_block_size;

	/* multiply by this to get the device block # from the fs block # */
	dr9_off_t              dev_block_conversion;

	bfs_inode         *root_dir;
	bfs_inode         *index_index;    /* does not contain itself :-) */
	bfs_inode         *name_index;
	bfs_inode         *size_index;
	bfs_inode         *create_time_index;
	bfs_inode         *last_modified_time_index;

	index_list        *index;

	int                counter;        /* a counter that always increases */

	sem_id             log_sem;        /* guards access to log structures */
	dr9_off_t              cur_log_end;    /* where we are in the log currently */
    struct log_handle *cur_lh;         /* the current transaction handle */
	long               active_lh;      /* true if cur_lh is active */
	thread_id          log_owner;      /* current owner of log_sem */
	int                blocks_per_le;  /* # of blocks per log entry */
	dr9_off_t         *log_block_nums; /* used to call set_blocks_info() */
	bigtime_t          last_log;       /* time of last entry (for flushing) */

	/* this is a list of log transactions that have completed out of order */
	log_list          *completed_log_entries;


	sem_id             tmp_blocks_sem;
	tmp_blocks        *tmp_blocks;

	int                unsoiled_fs;    /* whether or not to skip sector 0 */
	uchar             *original_bootblock;

	long               reserved_space; /* # of "reserved" blocks for inserts */
} bfs_info;



#define SUPER_BLOCK_MAGIC1   0x42465331    /* BFS1 */
#define SUPER_BLOCK_MAGIC2   0xdd121031
#define SUPER_BLOCK_MAGIC3   0x15b6830e

#define BFS_BIG_ENDIAN       0x42494745    /* BIGE */
#define BFS_LITTLE_ENDIAN    0x45474942    /* EGIB */

#define MAX_READERS          1000000       /* max # of concurrent readers */

/* flags for the bfs_info flags field */
#define FS_READ_ONLY         0x00000001
#define FS_IS_REMOVABLE      0x00000002

/* how many free blocks are there on a volume */
#define NUM_FREE_BLOCKS(x) ((x)->dsb.num_blocks - (x)->dsb.used_blocks)


/* this is the minimum # of free blocks we need (worst-case) to do an insert */
#define NUM_INSERT_BLOCKS 128     /* should be enough... */


#include "bitmap.h"
#include "inode.h"
#include "dstream.h"
#include "file.h"
#include "dir.h"
#include "select_ops.h"
#include "index.h"
#include "walk.h"
#include "debug.h"
#include "io.h"
#include "util.h"
#include "mount.h"
#include "attr.h"
#include "log.h"
#include "fsinfo.h"
#include "query.h"
#include "bfs_pm.h"

/* #include "dmalloc.h" */

#ifndef USER
#define printf dprintf
#include <KernelExport.h>
#endif


#endif /* _BFS_H */
