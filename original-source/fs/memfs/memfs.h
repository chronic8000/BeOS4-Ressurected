#ifndef _MEMFS_H
#define _MEMFS_H

#ifdef USE_DMALLOC
#define calloc   dcalloc
#define realloc  drealloc
#define malloc   dmalloc
#define cfree    dcfree
#define free     dfree
#define strdup   dstrdup 
#endif

#define MEMFS_MAGIC 'MMFS' 
#define MEMFS_NODE_MAGIC 'NodE'
#define MEMFS_FILE_MAGIC 'FilE'

#define MEMFS_BLOCKSIZE       B_PAGE_SIZE
#define MEMFS_MIN_SIZE			(64*1024)
#define MEMFS_MAX_SIZE        (1024 *1024 * 1024)

#define	MEMFS_VNODE_FLAG_DELETED	0x8000
#define	MEMFS_VNODE_FLAG_ATTR		0x4000
#define	MEMFS_VNODE_FLAG_UNUSED2	0x2000
#define	MEMFS_VNODE_FLAG_UNUSED3	0x1000

#define	S_ISATTR(m) (((m) & S_ATTR) == S_ATTR)

enum {
	MEMFS_IOCTL_DUMP_INFO = 40010,
	MEMFS_IOCTL_DUMP_DIR_INFO,
	MEMFS_IOCTL_DUMP_VNODE_INFO,
	MEMFS_IOCTL_DUMP_ALL_VNODES,
	MEMFS_IOCTL_DUMP_FRAG_LIST
};

enum {
	MEMFS_READ = 0,
	MEMFS_WRITE
};

enum {
	MEMFS_DIR = 0,
	MEMFS_ATTRDIR
};

#define DIR_MODE  (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) /* -rwxr-xr-x */
#define LINK_MODE (S_IRWXU | S_IRWXG | S_IRWXO )                    /* -rwxrwxrwx */

#define WSTAT_PROT_OWNER (WSTAT_MODE | WSTAT_UID | WSTAT_GID) 
#define WSTAT_PROT_OTHER (~(WSTAT_PROT_OWNER | WSTAT_PROT_TIME))
#define WSTAT_PROT_TIME  (0x0)

#define CHECK_INFO(A,S) if ( !(A) || (A)->magic !=  MEMFS_MAGIC ) { \
	dprintf ( "memfs: %s: bad memfs_info (0x%p)\n", S, A ); \
	return EINVAL; }

#define CHECK_NODE(A,S) if ( !(A) || (A)->magic !=  MEMFS_NODE_MAGIC ) { \
	dprintf ( "memfs: %s: bad vnode (0x%p)\n", S, A ); \
	return EINVAL; }

#define CHECK_FILE(A,S) if ( !(A) || (A)->magic !=  MEMFS_FILE_MAGIC ) { \
	dprintf ( "memfs: %s: bad file_info (0x%p)\n", S, A ); \
	return EINVAL; }

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

#define MAX_READERS 1024

#define lock_writer(info) acquire_sem_etc(info->lock, MAX_READERS, 0, 0)
#define unlock_writer(info) release_sem_etc(info->lock, MAX_READERS, 0)
#define lock_reader(info) acquire_sem_etc(info->lock, 1, 0, 0)
#define unlock_reader(info) release_sem_etc(info->lock, 1, 0)

typedef struct memfs_info  memfs_info;
typedef struct vnode       vnode;
typedef struct file_info   file_info;

#define RUN_CONTAINER_SIZE	15

struct file_run_container {
	struct  file_run_container *next;
	uint32  num_runs;
	struct  file_run {
		uint32 block;
		uint32 len;
	} run[RUN_CONTAINER_SIZE];
};

#define FRAG_SIZE 64
#define FRAG_SIZE_ROUNDUP(size) ROUNDUP(size, FRAG_SIZE)
#define NUM_FRAGS_PER_BLOCK (B_PAGE_SIZE / FRAG_SIZE)

struct frag_desc {
	struct frag_desc *prev;
	struct frag_desc *next;
	struct frag_desc *next_unused;
	uint32 start;
	uint32 len;
	vnode  *owner;			/* vnode that has it as it's last frag, or NULL if it's free */
};

struct file_data {
	int                        attr_type;	/* if an attribute, the type is stored here */
	struct frag_desc           *last_frag;
	uint32                     frag_size;
	struct file_run_container  *run_head;
};

/* represents each object in the fs: file, directory */
struct vnode {
	uint32      magic;      /* MEMFS_NODE_MAGIC */
	nspace_id   nsid;
	vnode_id    vnid;
	char        *name;
	time_t      ctime;      /* time file created */
	time_t      mtime;      /* time last modified */
	uid_t       uid;
	gid_t       gid;
	mode_t      mode;       /* aka oflags */
	uint32      flags;
	vnode       *parent;    /* parent vnode */
	vnode       *next;      /* next sibling in (attr)dir */	
	vnode       *prev;      /* prev sibling in (attr)dir */
	file_info   *jar;       /* list of open cookies */
	vnode       *all_next;  /* next & prev ptrs in global list of all vnodes */
	vnode       *all_prev;
	size_t      size;
	vnode       *attr_head; /* start of attrdir */
	union {	
		vnode   *dir_head;      /* for directories */	
		char    *symlink;       /* for symbolic links */	
		struct  file_data data; /* for file data */
	} u;
};

/* file/dir cookie */
struct file_info {
	uint32      magic;		/* MEMFS_FILE_MAGIC */
	file_info   *next_fc;
	vnode_id    owner_vnid;
	vnode       *next;
	mode_t      omode;
	int         pos;
};


#define PAGE_TABLE_SIZE (B_PAGE_SIZE / sizeof(uint32))

/* the filesystem namespace structure */
struct memfs_info {
	uint32              magic;          /* MEMFS_MAGIC */
	char                name[B_OS_NAME_LENGTH];
	uint32              next_version;
	nspace_id           nsid;
	uint32              flags;
	long                max_size;       /* in bytes */
	long                size;           /* in bytes */
	long                vnode_count;
	area_id             area;           /* used to store large blocks */
	uint8               *area_ptr;
	uint8               *area_bitmap;   /* used to store a bitmap representing used blocks in the area */
	uint32              area_size;      /* size in B_PAGE_SIZE blocks */
	sem_id              lock;
	vnode               *root;
	vnode               *all_head;      /* list of all vnodes in this nspace */
	struct frag_desc    *all_frags;
	struct frag_desc	*unused_frags;
	uint32              vnode_pgdir;    /* block at which the top of a 2-level pagetable contains */
	                                    /* the vnode hash */
	/* The following bitmap stores which pagetables have free holes in them,
	so the free entry finder can scan faster by skipping full page tables */
	uint8				vnode_page_full_bitmap[PAGE_TABLE_SIZE / 8];
};

#endif
