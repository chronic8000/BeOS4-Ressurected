#ifndef EXT2_H
#define EXT2_H


#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ByteOrder.h>
#include <KernelExport.h>

#include "export.h"
#include "ext2_fs.h"
#include "fsproto.h"
#include "lock.h"
#include "cache.h"

#ifdef USE_DMALLOC
	#include <dmalloc.h>
#else

/* allocate memory from swappable heap */
#define malloc smalloc
#define free sfree
#define calloc scalloc
#define realloc srealloc

#include <kalloc.h>

#endif

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
#define ATTR 8

extern uint8 ext2_debuglevels[];

#ifdef DEBUG
	#define DEBUGPRINT(x,y,z) if((ext2_debuglevels[x] >= y) || (ext2_debuglevels[ALL] >= y)) dprintf z
#else
	#define DEBUGPRINT(x,y,z)
#endif

#ifdef RO
	#define LOCKVNODE(v, n)
	#define UNLOCKVNODE(v, n)
#else
	#define LOCKVNODE(v, n) LOCKM((v)->vnode_mlock, (n))
	#define UNLOCKVNODE(v, n) UNLOCKM((v)->vnode_mlock, (n))
#endif

#define ERRPRINT(x) dprintf x

// Set up some byte-swapping macros
#define	LENDIAN_TO_HOST16(x) (x)=B_LENDIAN_TO_HOST_INT16(x)
#define	LENDIAN_TO_HOST32(x) (x)=B_LENDIAN_TO_HOST_INT32(x)
#define	LENDIAN_TO_HOST64(x) (x)=B_LENDIAN_TO_HOST_INT64(x)

#define HOST_TO_LENDIAN16(x) (x)=B_HOST_TO_LENDIAN_INT16(x)
#define HOST_TO_LENDIAN32(x) (x)=B_HOST_TO_LENDIAN_INT32(x)
#define HOST_TO_LENDIAN64(x) (x)=B_HOST_TO_LENDIAN_INT64(x)

#define MAX_READERS 10000

// The root inode is always 2
#define ROOTINODE 2

// Magic checking macros
#define VNODE_MAGIC 'Tamu'
#define EXT2_FS_STRUCT_MAGIC 'tooL'

#define CHECK_VNODE_MAGIC(vn) (((vn)->magic)!=VNODE_MAGIC) ? EINVAL : B_OK
#define CHECK_E2FS_MAGIC(fs) (((fs)->magic)!=EXT2_FS_STRUCT_MAGIC) ? EINVAL : B_OK

#define CHECK_VNODE(vn, func) if(!(vn) || CHECK_VNODE_MAGIC(vn)<B_OK) { \
	ERRPRINT(("ext2 vnode failed magic check in %s\n", (func))); \
	return EINVAL; \
}
#define CHECK_FS_STRUCT(fs, func) if(!(fs) || CHECK_E2FS_MAGIC(fs)<B_OK) { \
	ERRPRINT(("ext2 fs struct failed magic check in %s\n", (func))); \
	return EINVAL; \
}
#define CHECK_COOKIE(c, func) if(!(c)) { \
	ERRPRINT(("ext2 cookie was null in %s\n", (func))); \
	return EINVAL; \
}

// Feature checking macros
#define USING_FILETYPES_FEATURE(ext2) \
	(((ext2)->sb.s_rev_level == EXT2_DYNAMIC_REV) && ((ext2)->sb.s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE) != 0)

typedef struct vnode_struct {
	uint32 magic;
	vnode_id vnid;			
	
#ifndef RO
	// This semaphore is assigned to this vnode for various operations
	mlock vnode_mlock;
#endif
	// Count the number of open reads and writes
	int ropen;
	int wopen;
	
	// Copy of the ext2 inode
	ext2_inode inode;

	// points to a mime string
	const char *mime;

	// number of file blocks
	int	num_file_blocks;

#ifndef RO
	// The next part is strictly for block pre-allocation. When writing, it's always good to allocate 8 blocks at a time.
	// Store them here. As soon as all of the writers close, however, remove them from here and give back to the filesystem.
	uint32	pre_blocks[8];
	int pre_pos; // This points to the next one to be allocated for real. -1 or > 7 means nothing allocated
#endif
	bool is_dir;

	struct vnode_struct *next;
} vnode;

// Preallocation item
typedef struct prealloc_item_struct {
	uint32	block_num;	
	struct prealloc_item_struct *next;
} prealloc_item;	
	
// Will be more useful in the future
typedef struct filecookie_struct {
	int omode;
} filecookie;

typedef struct dircookie_struct {
	// Store the location of the next directory entry. 
	uint32 next_pos;
	
	// vnode id of the dir
	vnode_id dir_vnode;
	
	// Next cookie in a big list
	struct dircookie_struct *next; // unused right now
} dircookie;

typedef struct {
	// store if you're done
	bool done;
} attrdircookie;


// This is the main structure of the filesystem
typedef struct ext2_fs_struct_struct {
		uint32 magic;
		int fd;

		uint32 num_cache_blocks;
		bool   is_cache_initialized;		
		bool   is_read_only;
		bool   is_removable;

		ext2_group_desc *gd;
#ifndef RO
		bool *gd_to_flush; 		// This array has one element per group descriptor. It's purpose is to allow group descriptor flushes to
								// happen at once. When a group descriptor is modified, instead of flushing it to the cache immediately, the
								// corresponding boolean variable is set here. Later, when the WriteAllDirtyGroupDesc function is called it will
								// flush all of the group descriptors with the flag set.

		sem_id block_marker_sem;
		sem_id block_alloc_sem;
		sem_id prealloc_sem;

		// Preallocation list stuff
		prealloc_item *fFirstPreAlloc;		
		int fNumPreAllocs;
#endif
		ext2_sb_info 	 info;
		ext2_super_block sb;
		nspace_id 		 nsid;
		vnode_id 		 root_vnid;

		bool override_uid;
		uid_t uid;
		gid_t gid;

		char *device_name;
		char *volume_name;
} ext2_fs_struct;

// Function declarations		

// initialization and deinitialization	(ext2.c)
ext2_fs_struct	*ext2_create_ext2_struct();	
status_t ext2_remove_ext2_struct(ext2_fs_struct *);
status_t ext2_init_cache(ext2_fs_struct *);
status_t ext2_initialize_vnode(vnode *new_vnode, vnode_id id);
status_t ext2_deallocate_vnode(vnode *killit);
		

#endif
