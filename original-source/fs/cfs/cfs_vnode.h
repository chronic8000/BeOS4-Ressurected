#ifndef CFS_VNODE_H
#define CFS_VNODE_H

#include "cfs.h"
#include <fsproto.h>
#include <lock.h>

typedef struct cfs_vblock_list {
	struct cfs_vblock_list  *next;
	uint32                   flags;			/* 0 uncompressed, 1 compressed */
	off_t                    data_pos;
	off_t                    size;
	size_t                   compressed_size;
	off_t                    location;      /* disk location of the entry */
} cfs_vblock_list;

typedef struct cfs_file {
	cfs_vblock_list  *datablocks;
	bool			 is_blocklist_loaded;
} cfs_file;

typedef struct cfs_node cfs_node;

typedef struct cfs_directory {
	off_t             first_entry;
} cfs_directory;

typedef struct cfs_link {
	char             *link;
} cfs_link;

struct cfs_node {
	off_t             vnid;
	off_t             parent;
	off_t             attr_vnid;
	char             *name;
	uint32            create_time;
	uint32            mod_time;
	uint32            flags;
	uint32            cfs_flags;
	off_t             logical_size;			// new to version 2
	union {
		cfs_file      file;
		cfs_directory dir;
		cfs_link   link;
	} u;
};

#define CFS_MAX_READERS 1024

#define CFS_MAX_IO_BUFFER_SIZE 32768

typedef struct cfs_transaction cfs_transaction;
typedef struct cfs_log_transaction cfs_log_transaction;

typedef struct cfs_info {
	nspace_id nsid;
	int       dev_fd;
	size_t    _dev_block_size;
	size_t    cache_block_size;
	int       fs_flags;
	uint32    fs_version;
	off_t     size;
	off_t     super_block_2_pos;
	off_t     root_pos;
	off_t     log_base;
	off_t     log_size;
	off_t     *log_info_pos;
	size_t    log_info_count;
	uint32    log_version;
	off_t     log_head;
	off_t     log_tail;
	off_t     reserved_space_pos;
	off_t     reserved_space_size;
	int32     create_time;
	int32     modification_time;
	cfs_node  *root_dir;
	char      name[33];
	sem_id    read_write_lock;
	uint16    last_entry_version;

	cfs_log_transaction *logged_transactions;
	cfs_transaction *current_transaction;

	int32 debug_read_lock_held;
	int32 debug_write_lock_held;
	
	// global decompression buffer
	off_t     last_decompression_offset;
	void	  *decompression_buf;
	lock      decompression_buf_lock;
	
	// free space counter
	off_t     free_space_count;
	
	// blocklist loader sem
	sem_id    blocklist_loader_lock;
} cfs_info;

typedef struct cfs_dir_cookie {
	off_t last_vnid;
	int   index;
	char  last_name[CFS_MAX_NAME_LEN];
} cfs_dir_cookie;

/* in cfs.c */
status_t cfs_read_superblock2(cfs_info *fsinfo, cfs_super_block_2 *sb2);

/* in cfs_io.c */

/* use to read/write blocks from uncached area of disk (superblock1 and log) */
status_t cfs_read_disk_uncached(cfs_info *fsinfo, off_t pos, void *buffer,
                                size_t buffer_size);
status_t cfs_write_disk_uncached(cfs_info *fsinfo, off_t pos, void *buffer,
                                 size_t buffer_size);

/* use to read filesystem and user data */
status_t cfs_read_disk(cfs_info *fsinfo, off_t pos,
                       void *buffer, size_t buffer_size);
status_t cfs_read_disk_etc(cfs_info *fsinfo, off_t pos, void *buffer,
                           size_t *buffer_size, size_t min_read_size);

/* use to write user data */
status_t cfs_write_disk_cached(cfs_info *fsinfo, cfs_transaction *transaction,
                               off_t pos, const void *buffer,
                               size_t buffer_size);
/* use to write filesystem data */
status_t cfs_write_disk(cfs_info *fsinfo, cfs_transaction *transaction,
                        off_t pos, const void *buffer, size_t buffer_size);

status_t cfs_read_entry_info(cfs_info *fsinfo, off_t vnid,
                             cfs_entry_info *entry);
status_t cfs_read_entry(cfs_info *fsinfo, off_t vnid,
                        cfs_entry_info *entry, char *name);
status_t cfs_write_entry_info(cfs_info *fsinfo, cfs_transaction *transaction,
                              off_t vnid, cfs_entry_info *entry);

/* in cfs_lock.c */
status_t cfs_debug_check_read_lock(cfs_info *fsinfo);
status_t cfs_debug_check_write_lock(cfs_info *fsinfo);

status_t cfs_lock_read(cfs_info *fsinfo);
status_t cfs_lock_read_nointerrupt(cfs_info *fsinfo);
void cfs_unlock_read(cfs_info *fsinfo);
status_t cfs_lock_write(cfs_info *fsinfo);
status_t cfs_lock_write_nointerrupt(cfs_info *fsinfo);
void cfs_unlock_write(cfs_info *fsinfo);

/* in cfs_vnode.c */
status_t cfs_remove_entry(cfs_info *fsinfo, off_t vnid, uint32 flags);
status_t cfs_update_node(cfs_info *fsinfo, cfs_transaction *transaction,
                         cfs_node *node);
status_t cfs_add_unlinked_entry(cfs_info *fsinfo, cfs_transaction *transaction,
                                off_t vnid);
status_t cfs_cleanup_unlinked_entries(cfs_info *fsinfo);
status_t cfs_load_blocklist(cfs_info *fsinfo, cfs_node *node);

/* in cfs_dir.c */
status_t cfs_opendir_etc(cfs_node *node, void **cookie, bool attr);
void cfs_rewinddir_etc(cfs_dir_cookie *c, bool attr);
status_t cfs_read_dir_etc(cfs_info *fsinfo, cfs_node *node, cfs_dir_cookie *c,
                          long *num, struct dirent *buf, size_t bufsize,
                          bool attr);

status_t cfs_lookup_dir(cfs_info *fsinfo, cfs_node *dir, const char *entry_name,
                        off_t *prev_entry_vnid, off_t *entry_location,
                        off_t *next_entry_vnid);
status_t cfs_insert_dir(cfs_info *fsinfo, cfs_transaction *transaction,
                        cfs_node *dir, cfs_node *node);
status_t cfs_remove_dir(cfs_info *fsinfo, cfs_transaction *transaction,
                        cfs_node *dir, cfs_node *node);

#define cfs_insert_dir_log_blocks 6
#define cfs_remove_dir_log_blocks 6

/* in cfs_data.c */
status_t cfs_set_file_size(cfs_info *fsinfo, cfs_transaction *transaction,
                           cfs_node *node, off_t size);
status_t cfs_read_data(cfs_info *fsinfo, cfs_node *node, off_t pos,
                       void *buf, size_t *len);
status_t cfs_write_data(cfs_info *fsinfo, cfs_transaction *transaction,
                        cfs_node *node, off_t pos, const void *buf,
                        size_t *len, bool append);

/* in cfs_log.c */
//int cfs_available_log_space(cfs_info *fsinfo);
int cfs_max_log_blocks(cfs_info *fsinfo);

status_t cfs_start_transaction(cfs_info *fsinfo,
                               cfs_transaction **transaction_ptr);
status_t cfs_sub_transaction(cfs_info *fsinfo, cfs_transaction *transaction,
                             int max_blocks);
status_t cfs_end_transaction(cfs_info *fsinfo, cfs_transaction *transaction);
void *get_block_logged(cfs_info *fsinfo, cfs_transaction *transaction,
                       off_t location);
status_t cfs_flush_log(cfs_info *fsinfo);
status_t cfs_flush_log_blocks(cfs_info *fsinfo);
status_t cfs_init_log(cfs_info *fsinfo);
void cfs_uninit_log(cfs_info *fsinfo);

#endif
