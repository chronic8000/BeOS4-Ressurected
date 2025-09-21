#include "cfs_vnode.h"
#include "cfs_entry.h"
#include "cfs_debug.h"
#include <KernelExport.h>
#include <malloc.h>
#include <cfs_ioctls.h>
#include <string.h>

typedef struct file_cookie {
	int mode;
	uint32 mod_time;
} file_cookie;

int 
cfs_access(void *ns, void *node, int mode)
{
	return B_NO_ERROR;
}

int 
cfs_readlink(void *ns, void *_node, char *buf, size_t *bufsize)
{
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	size_t len;
	status_t err;
	
	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR) {
		return err;
	}

	if (!S_ISLNK(node->flags)) {
		err = B_BAD_VALUE;
		goto err;
	}
	len = strlen(node->u.link.link);
	if(len > *bufsize)
		len = *bufsize;
	else
		*bufsize = len;
	memcpy(buf, node->u.link.link, len);

	//dprintf("cfs_readlink: len %ld, %s -> %s\n", len, node->name, node->u.link.link);
	err = B_NO_ERROR;
err:
	cfs_unlock_read(fsinfo);
	return err;
}

int 
cfs_open(void *ns, void *_node, int omode, void **cookie)
{
	file_cookie *f;
	cfs_node *node = (cfs_node *)_node;

	//dprintf("cfs_open %p %p %x %p\n", ns, node, omode, cookie);

	f = malloc(sizeof(file_cookie));
	if(f == NULL) {
		return B_NO_MEMORY;
	}
	f->mode = omode;
	f->mod_time = node->mod_time;
	*cookie = f;
	
	return B_NO_ERROR;
}

int 
cfs_close(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR;
}

static status_t
update_mod_time(cfs_info *fsinfo, cfs_node *node, uint32 new_mod_time)
{
	status_t err, tmp_err;
	cfs_transaction *transaction;

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR)
		goto err1;
	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_sub_transaction(fsinfo, transaction, 2);
	if(err != B_NO_ERROR)
		goto err3;

	if(new_mod_time > node->mod_time) {
		node->mod_time = new_mod_time;
		err = cfs_update_node(fsinfo, transaction, node);
		if(err == B_NO_ERROR)
			notify_listener(B_STAT_CHANGED, fsinfo->nsid, 0, 0, node->vnid,
			                NULL);
	}
err3:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err2:
	cfs_unlock_write(fsinfo);
err1:
	return err;
}

int 
cfs_free_cookie(void *ns, void *_node, void *_cookie)
{
	file_cookie *cookie = _cookie;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;

	if(cookie->mod_time > node->mod_time) {
		update_mod_time(fsinfo, node, cookie->mod_time);
	}
	free(cookie);
	return B_NO_ERROR;
}

int 
cfs_read(void *ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;

	if (S_ISDIR(node->flags)) {
		err = EISDIR;
		goto err1;
	}

	if(!S_ISREG(node->flags)) {
		err = B_BAD_VALUE;
		goto err1;
	}

	if(pos < 0) {
		err = B_BAD_VALUE;
		goto err1;
	}

	if(*len < 0) {
		err = B_BAD_VALUE;
		goto err1;
	}

	if(*len == 0) {
		err = B_NO_ERROR;
		goto err1;
	}

	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR)
		goto err1;

	err = cfs_read_data(fsinfo, node, pos, buf, len);

	cfs_unlock_read(fsinfo);
err1:
	if(err != B_NO_ERROR) {
		*len = 0;
	}
	
	return err;
}

int 
cfs_write(void *ns, void *_node, void *_cookie, off_t pos,
          const void *buf, size_t *len)
{
	status_t err, tmp_err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	file_cookie *cookie = _cookie;
	cfs_transaction *transaction;

	if (S_ISDIR(node->flags)) {
		err = EISDIR;
		goto err0_1;
	}

	if(!S_ISREG(node->flags)) {
		err = B_BAD_VALUE;
		goto err0_1;
	}

	if(pos < 0) {
		err = B_BAD_VALUE;
		goto err0_1;
	}

	if(*len < 0) {
		err = B_BAD_VALUE;
		goto err0_1;
	}

	if(*len == 0) {
		err = B_NO_ERROR;
		goto err0_1;
	}		

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR)
		goto err0_1;

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err_2;
				
	// if we're writing to the start of the file, and the file len is 0
	if(pos == 0 && *len >= 4 && node->logical_size == 0) {
		// look for the CELF header (0x7f43454c)
		if(*(uint32 *)buf == 0x4c45437f) {
			// clear the compressed flag if it's set
			if((node->cfs_flags & CFS_FLAG_COMPRESS_FILE) != 0) {
				uint32 old_flags = node->cfs_flags;

				node->cfs_flags &= ~CFS_FLAG_COMPRESS_FILE;
				PRINT_FLOW(("found CELF header, clearing compression flag\n"));
		
				// write the entry back out
				err = cfs_sub_transaction(fsinfo, transaction, 2);
				if(err != B_NO_ERROR)
					goto err1_3; 					

				err = cfs_update_node(fsinfo, transaction, node);
				if(err != B_NO_ERROR) {
					// restore the old flags in the memory copy
					node->cfs_flags = old_flags;
					goto err1_3;
				}
			}
		}
	}

	err = cfs_write_data(fsinfo, transaction, node, pos, buf, len,
	                     (cookie->mode & O_APPEND) == O_APPEND);

	cookie->mod_time = time(NULL);

err1_3:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err_2:
	cfs_unlock_write(fsinfo);
err0_1:
	if(err != B_NO_ERROR)
		*len = 0;
	return err;
}

int 
cfs_rstat(void *ns, void *_node, struct stat *stat)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;

	memset(stat, 0, sizeof(*stat));
			
	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR)
		goto err;

	if(S_ISREG(node->flags)) {
		stat->st_size = node->logical_size;
		//dprintf("cfs_rstat file %s size = %Ld\n", node->name, stat->st_size);
	}

	stat->st_dev = fsinfo->nsid;
	stat->st_nlink = 1;

	stat->st_ino = node->vnid;
	stat->st_blksize = cfs_compressed_block_size;
	stat->st_mode = node->flags;

	stat->st_atime = node->mod_time;
	stat->st_mtime = node->mod_time;
	stat->st_ctime = node->mod_time;
	stat->st_crtime = node->create_time;

	cfs_unlock_read(fsinfo);
err:
	return err;
}

int 
cfs_wstat(void *ns, void *_node, struct stat *stat, long mask)
{
	status_t err, tmp_err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	cfs_transaction *transaction;

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR)
		goto err1;
	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err2;

	//dprintf("cfs_wstat mask = 0x%08ld\n", mask);

	if(mask & WSTAT_MODE) {
		node->flags = (node->flags & S_IFMT) | (stat->st_mode & S_IUMSK);
	}
	if (mask & WSTAT_UID) {
		err = B_NOT_ALLOWED;
		goto err3;
	}
	if (mask & WSTAT_GID) {
		err = B_NOT_ALLOWED;
		goto err3;
	}
	if(mask & WSTAT_SIZE) {
		//dprintf("cfs_wstat size = %Ld\n", stat->st_size);
		if (S_ISDIR(node->flags)) {
			err = EISDIR;
			goto err3;
		} else {
			err = cfs_set_file_size(fsinfo, transaction, node, stat->st_size);
			if(err != B_NO_ERROR)
				goto err3;
		}
	}
	if (mask & WSTAT_CRTIME) {
		node->create_time = stat->st_crtime;
	}

	if ((mask & WSTAT_MTIME) || (mask & WSTAT_SIZE)) {
		if (mask & WSTAT_MTIME) {
			node->mod_time = stat->st_mtime;
		} else {
			node->mod_time = time(NULL);
		}
		//if (S_ISDIR(bi->mode) == 0) {
		//	update_last_modified_time_index(bfs, bi);
		//}
	}
	err = cfs_sub_transaction(fsinfo, transaction, 2);
	if(err != B_NO_ERROR)
		goto err3;
	err = cfs_update_node(fsinfo, transaction, node);
	if(err != B_NO_ERROR)
		goto err3;
	
	notify_listener(B_STAT_CHANGED, fsinfo->nsid, 0, 0, node->vnid, NULL);
	err = B_NO_ERROR;
err3:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err2:
	cfs_unlock_write(fsinfo);
err1:
	return err;
}

int 
cfs_fsync(void *ns, void *node)
{
	return cfs_sync(ns);
}

static status_t
read_uint32_from_user_buffer(uint32 *dest_ptr, void *src_buf)
{
	status_t err;
	err = lock_memory(src_buf, sizeof(uint32), 0);
	if(err != B_NO_ERROR) {
		return err;
	}
	*dest_ptr = *((uint32*)src_buf);
	unlock_memory(src_buf, sizeof(uint32), 0);
	return B_NO_ERROR;
}

static status_t
write_uint32_to_user_buffer(void *dest_buf, uint32 value)
{
	status_t err;
	err = lock_memory(dest_buf, sizeof(uint32), 0);
	if(err != B_NO_ERROR) {
		return err;
	}
	*((uint32*)dest_buf) = value;
	unlock_memory(dest_buf, sizeof(uint32), 0);
	return B_NO_ERROR;
}

static status_t
write_off_t_to_user_buffer(void *dest_buf, off_t value)
{
	status_t err;
	err = lock_memory(dest_buf, sizeof(off_t), 0);
	if(err != B_NO_ERROR) {
		return err;
	}
	*((off_t*)dest_buf) = value;
	unlock_memory(dest_buf, sizeof(off_t), 0);
	return B_NO_ERROR;
}

int 
cfs_ioctl(void *ns, void *_node, void *cookie, int cmd, void *buf, size_t len)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	
	switch(cmd) {
		case B_CFS_GET_FLAGS:
			err = write_uint32_to_user_buffer(buf, node->cfs_flags);
			break;

		// XXX
		// The next two flag modification cases may allow the changes
		// to be lost if the node is never updated before being unloaded by
		// the fsil.
		case B_CFS_SET_FLAGS: {
			uint32 set_mask;
			err = read_uint32_from_user_buffer(&set_mask, buf);
			if(err != B_NO_ERROR)
				break;
			err = cfs_lock_write(fsinfo);
			if(err != B_NO_ERROR)
				break;
			node->cfs_flags |= set_mask;
			cfs_unlock_write(fsinfo);
			err = B_NO_ERROR;
			break;
		}

		case B_CFS_CLEAR_FLAGS: {
			uint32 clear_mask;
			err = read_uint32_from_user_buffer(&clear_mask, buf);
			if(err != B_NO_ERROR)
				break;
			err = cfs_lock_write(fsinfo);
			if(err != B_NO_ERROR)
				break;
			node->cfs_flags &= ~clear_mask;
			cfs_unlock_write(fsinfo);
			err = B_NO_ERROR;
			break;
		}

		case B_CFS_GET_DISK_SPACE: {
			off_t space_used = get_entry_size(fsinfo->fs_version) +
			                   cfs_string_size(node->name);
			
			err = cfs_lock_read(fsinfo);
			if(err < B_NO_ERROR)
				 break;
				
			if(S_ISREG(node->flags)) {
				cfs_vblock_list *cur;
				
				err = cfs_load_blocklist(fsinfo, node);
				if(err != B_NO_ERROR) {
					break;
				}
				
				cur = node->u.file.datablocks;
				while(cur) {
					if(cur->flags)
						space_used += cur->compressed_size;
					else
						space_used += cur->size;
					space_used += sizeof(cfs_data_block);
					cur = cur->next;
				}
			}
			else if(S_ISLNK(node->flags)) {
				space_used += cfs_string_size(node->u.link.link);
			}
			cfs_unlock_read(fsinfo);
			err = write_off_t_to_user_buffer(buf, space_used);
			break;
		}

		case B_CFS_READ_RESERVED_BLOCK:
		case B_CFS_WRITE_RESERVED_BLOCK: 
			PRINT_INTERNAL_ERROR(("cfs_ioctl: read/write reserved "
			                      "block not implemented\n"));
		default:
			err = B_NOT_ALLOWED;
	};
	
	return err;		
}


