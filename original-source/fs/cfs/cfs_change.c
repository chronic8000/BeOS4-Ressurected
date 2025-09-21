#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_entry.h"
#include "cfs_vnode.h"
#include "cfs_free_space.h"
#include <KernelExport.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

typedef enum {
	CFS_UNLINK_NO_DIRS,
	CFS_UNLINK_DIRS_ONLY,
	CFS_UNLINK_ANY_TYPE
} unlink_type_t;

static status_t
cfs_create_entry(cfs_info *fsinfo, cfs_node *dir, const char *name, int perms,
                 const char *symlink, int omode, vnode_id *vnid, void **cookie)
{
	status_t err;
	cfs_entry_info direntry;
	size_t name_size;
	size_t symlink_size = 0;
	off_t entry_location;
	off_t entry_vnid;
	off_t prev_entry_vnid;
	off_t next_entry_vnid;
	cfs_node *node;
	cfs_transaction *transaction;

	PRINT_FLOW(("cfs_create_entry: dir %s, entry %s\n", dir->name, name));

	if (name == NULL || *name == '\0' || strchr(name, '/') || 
		strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return B_BAD_VALUE;

	if(strlen(name) >= CFS_MAX_NAME_LEN)
		return B_BAD_VALUE;

	name_size = cfs_string_size(name);
	
	direntry.parent = 0;
	direntry.next_entry = 0;
	direntry.data = 0;
	direntry.attr = 0;
	direntry.name_offset = 0;
	direntry.create_time = time(NULL);
	direntry.modification_time = time(NULL);
	direntry.flags = perms & S_IUMSK;
	direntry.cfs_flags = CFS_FLAG_DEFAULT_FLAGS;
	direntry.logical_size = 0;

	if(symlink) {
		direntry.flags |= S_IFLNK;
		symlink_size = (strlen(symlink) + 1 + 7) & ~7;
	}
	else if(vnid) {
		direntry.flags |= S_IFREG;
	}
	else {
		direntry.flags |= S_IFDIR;
	}

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR)
		return err;

	direntry.version = cfs_inc_version(fsinfo->last_entry_version);
	
	if(dir->parent == 0) {
		PRINT_ERROR(("cfs_create_entry: parent deleted\n"));
		err = B_ENTRY_NOT_FOUND;
		goto err1_1;
	}

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err1_1;
	
	if(!S_ISDIR(dir->flags)) {
		err = B_BAD_VALUE;
		goto err1;
	}

	err = cfs_lookup_dir(fsinfo, dir, name,
	                     &prev_entry_vnid, &entry_vnid, &next_entry_vnid);
	if(err == B_NO_ERROR) {
		if(vnid == NULL || (omode & O_EXCL)) {
			err = B_FILE_EXISTS;
			goto err1;
		}
		err = get_vnode(fsinfo->nsid, entry_vnid, (void**)&node);
		if(err != B_NO_ERROR)
			goto err1;
		if(S_ISDIR(node->flags)) {
			err = B_IS_A_DIRECTORY;
			put_vnode(fsinfo->nsid, entry_vnid);
			goto err1;
		}
		if(!S_ISREG(node->flags)) {
			/* need to handle symlinks */
			err = B_FILE_EXISTS;
			put_vnode(fsinfo->nsid, entry_vnid);
			goto err1;
		}
		if(omode & O_TRUNC) {
			PRINT_FLOW(("cfs_create_entry: O_TRUNC\n"));
			err = cfs_set_file_size(fsinfo, transaction, node, 0);
			if(err != B_NO_ERROR) {
				put_vnode(fsinfo->nsid, entry_vnid);
				goto err1;
			}
		}
		err = cfs_open(fsinfo, node, omode, cookie);
		if(err != B_NO_ERROR) {
			put_vnode(fsinfo->nsid, entry_vnid);
			goto err1;
		}
		*vnid = entry_vnid;
		goto file_exists;
	}

	err = cfs_sub_transaction(fsinfo, transaction, ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 1 +
	                          (get_entry_size(fsinfo->fs_version) + name_size + symlink_size) /
	                          fsinfo->cache_block_size +
	                          cfs_insert_dir_log_blocks);
	if(err != B_NO_ERROR)
		goto err1;

	err = allocate_disk_space(fsinfo, transaction, &entry_location,
	                          get_entry_size(fsinfo->fs_version) + name_size + symlink_size, METADATA);
	if(err != B_NO_ERROR)
		goto err1;
	
	if(symlink) {
		direntry.data = entry_location + get_entry_size(fsinfo->fs_version) + name_size;
		err = cfs_write_disk(fsinfo, transaction, direntry.data,
		                     symlink, strlen(symlink)+1);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_create_entry: write failed, %s\n", strerror(err)));
			goto err2;
		}
	}
	direntry.name_offset = entry_location + get_entry_size(fsinfo->fs_version);
	err = cfs_write_disk(fsinfo, transaction, direntry.name_offset,
	                     name, strlen(name)+1);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_create_entry: write failed, %s\n", strerror(err)));
		goto err2;
	}

	entry_vnid = cfs_offset_to_vnid(entry_location, direntry.version);

	err = cfs_write_entry_info(fsinfo, transaction, entry_vnid, &direntry);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_create_entry: write failed, %s\n", strerror(err)));
		goto err2;
	}

	err = get_vnode(fsinfo->nsid, entry_vnid, (void**)&node);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_insert_dir(fsinfo, transaction, dir, node);
	if(err != B_NO_ERROR)
		goto err3;

	if(vnid) {
		err = cfs_open(fsinfo, node, omode, cookie);
		if(err != B_NO_ERROR)
			goto err3;
		*vnid = entry_vnid;
	}
	else {
		put_vnode(fsinfo->nsid, entry_vnid);
	}

	notify_listener(B_ENTRY_CREATED, fsinfo->nsid, dir->vnid, 0, entry_vnid,
	                name);

file_exists:
	err = cfs_end_transaction(fsinfo, transaction);
	if(err != B_NO_ERROR)
		goto err1_1;

	cfs_unlock_write(fsinfo);

	return B_NO_ERROR;

err3:
	put_vnode(fsinfo->nsid, entry_vnid);
err2:
	free_disk_space(fsinfo, transaction, entry_location,
	                sizeof(direntry) + name_size + symlink_size);
err1:
	cfs_end_transaction(fsinfo, transaction);
err1_1:
	cfs_unlock_write(fsinfo);
	return err;
}

int 
cfs_create(void *ns, void *dir, const char *name, int omode, int perms,
           vnode_id *vnid, void **cookie)
{
	return cfs_create_entry((cfs_info *)ns, (cfs_node *)dir, name, perms,
	                        NULL, omode, vnid, cookie);
}

int 
cfs_mkdir(void *ns, void *dir, const char *name, int perms)
{
	return cfs_create_entry((cfs_info *)ns, (cfs_node *)dir, name, perms,
	                        NULL, 0, NULL, NULL);
}

int 
cfs_symlink(void *ns, void *dir, const char *name, const char *path)
{
	return cfs_create_entry((cfs_info *)ns, (cfs_node *)dir, name,
	                        S_IRWXU | S_IRWXG | S_IRWXO, path,
	                        0, NULL, NULL);
}

static status_t
cfs_check_not_parent(cfs_info *fsinfo, cfs_node *dir, vnode_id not_parent)
{
	status_t err;
	int count = 0;
	vnode_id parent_id = dir->parent;

	err = cfs_debug_check_read_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	while(parent_id != fsinfo->root_pos) {
		cfs_node *parent_node;
		
		if(count++ > PATH_MAX) {
			PRINT_ERROR(("cfs_check_not_parent: circular directory structure\n"));
			return B_ERROR;
		}
		count++;
		if(parent_id == not_parent) {
			PRINT_WARNING(("cfs_check_not_parent: parent found\n"));
			return B_BAD_VALUE;
		}

		err = get_vnode(fsinfo->nsid, parent_id, (void**)&parent_node);
		if(err != B_NO_ERROR)
			return err;
		parent_id = parent_node->parent;
		err = put_vnode(fsinfo->nsid, parent_node->vnid);
		if(err != B_NO_ERROR)
			return err;
	}
	return B_NO_ERROR;
}

#define do_unlink_node_log_blocks (cfs_remove_dir_log_blocks+3)

static status_t
do_unlink_node(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *dir,
               cfs_node *node, unlink_type_t unlink_type)
{
	status_t err;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(unlink_type == CFS_UNLINK_DIRS_ONLY) {
		if(!S_ISDIR(node->flags))
			return ENOTDIR;
	}
	else if(unlink_type == CFS_UNLINK_NO_DIRS) {
		if(S_ISDIR(node->flags))
			return EISDIR;
	}

	if(S_ISDIR(node->flags)) {
		if(node->u.dir.first_entry != 0)
			return ENOTEMPTY;
	}

	err = cfs_remove_dir(fsinfo, transaction, dir, node);
	if(err != B_NO_ERROR)
		return err;
	
	err = cfs_add_unlinked_entry(fsinfo, transaction, node->vnid);
	if(err != B_NO_ERROR)
		return err;
	
	err = remove_vnode(fsinfo->nsid, node->vnid);
	if(err != B_NO_ERROR)
		return err;

	notify_listener(B_ENTRY_REMOVED,
	                fsinfo->nsid, dir->vnid, 0, node->vnid, NULL);
	return B_NO_ERROR;
}

static status_t
do_unlink(cfs_info *fsinfo, cfs_node *dir, const char *name,
          unlink_type_t unlink_type)
{
	status_t err, tmp_err;
	cfs_node *node = NULL;
	cfs_transaction *transaction;
	off_t vnid;
	off_t prev_entry_offset;
	off_t next_entry_offset;

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR)
		goto err1;

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_lookup_dir(fsinfo, dir, name,
	                     &prev_entry_offset, &vnid, &next_entry_offset);
	if(err != B_NO_ERROR)
		goto err3;
		
	err = get_vnode(fsinfo->nsid, vnid, (void**)&node);
	if(err != B_NO_ERROR)
		goto err3;
	
	err = cfs_sub_transaction(fsinfo, transaction, do_unlink_node_log_blocks);
	if(err != B_NO_ERROR)
		goto err3;

	err = do_unlink_node(fsinfo, transaction, dir, node, unlink_type);
err3:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err2:
	cfs_unlock_write(fsinfo);
	if(node != NULL)
		put_vnode(fsinfo->nsid, vnid);
err1:
	return err;
}

int 
cfs_rename(void *ns, void *_olddir, const char *oldname,
           void *_newdir, const char *newname)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *olddir = (cfs_node *)_olddir;
	cfs_node *newdir = (cfs_node *)_newdir;
	cfs_transaction *transaction;
	off_t vnid;
	cfs_node *node = NULL;

	off_t old_dest_vnid;
	cfs_node *old_dest_node = NULL;

	off_t prev_entry_vnid;
	off_t next_entry_vnid;

	off_t new_name_locaton = 0;
	size_t old_name_size = cfs_string_size(oldname);
	size_t new_name_size = cfs_string_size(newname);
	bool rename;

	char *old_name_ptr = NULL;
	cfs_entry_info new_entry;

	if(olddir->vnid == newdir->vnid &&
		old_name_size == new_name_size &&
		strcmp(oldname, newname) == 0) {
		// same file
		return B_OK;
	}

	err = cfs_lock_write(fsinfo);
	if(err < B_NO_ERROR) {
		return err;
	}
	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err1_1;

	//dprintf("cfs_rename: %s in %s to %s in %s\n", oldname, olddir->name,
	//newname, newdir->name);
	
	err = cfs_lookup_dir(fsinfo, olddir, oldname,
	                     &prev_entry_vnid, &vnid, &next_entry_vnid);
	if(err != B_NO_ERROR)
		goto err1;

	err = cfs_lookup_dir(fsinfo, newdir, newname,
	                     &prev_entry_vnid, &old_dest_vnid, &next_entry_vnid);
	if(err != B_NO_ERROR)
		old_dest_vnid = 0;
	
	err = get_vnode(fsinfo->nsid, vnid, (void**)&node);
	if(err != B_NO_ERROR)
		goto err1;

	/* alloc new name (4/7) + write name (2) + unlink old + remove old dir +
	   insert new dir + free old name (4/7) */
	err = cfs_sub_transaction(fsinfo, transaction,
	                          ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 2 + do_unlink_node_log_blocks +
	                          cfs_remove_dir_log_blocks +
	                          cfs_insert_dir_log_blocks + FREE_MAX_LOG_SIZE(fsinfo->fs_version));
	
	if(err != B_NO_ERROR)
		goto err1;

	rename = strcmp(newname, oldname) != 0;
	if(new_name_size > old_name_size) {
		err = allocate_disk_space(fsinfo, transaction, &new_name_locaton,
		                          new_name_size, METADATA);
		if(err != B_NO_ERROR)
			goto err2;
		err = cfs_write_disk(fsinfo, transaction, new_name_locaton, newname,
		                     strlen(newname)+1);
		if(err != B_NO_ERROR)
			goto err2;
	}
	if(old_dest_vnid != 0) {
		//dprintf("cfs_rename: removing %s in %s\n", newname, newdir->name);
		err = get_vnode(fsinfo->nsid, old_dest_vnid, (void**)&old_dest_node);
		if(err != B_NO_ERROR)
			goto err2;

		err = do_unlink_node(fsinfo, transaction, newdir, old_dest_node,
		                     CFS_UNLINK_ANY_TYPE);
		if(err != B_NO_ERROR)
			goto err2;
	}

	if(olddir != newdir) {
		err = cfs_check_not_parent(fsinfo, newdir, vnid);
		if(err != B_NO_ERROR)
			goto err2;
	}
	err = cfs_remove_dir(fsinfo, transaction, olddir, node);
	if(err != B_NO_ERROR)
		goto err2;
	if(rename) {
		old_name_ptr = node->name;
		node->name = strdup(newname);
		if(node->name == NULL)
			err = B_NO_MEMORY;
	}
	if(err == B_NO_ERROR) {
		err = cfs_insert_dir(fsinfo, transaction, newdir, node);
	}
	if(rename) {
		if(err == B_NO_ERROR) {
			err = cfs_read_entry_info(fsinfo, node->vnid, &new_entry);
		}
		if(new_name_size <= old_name_size) {
			if(err == B_NO_ERROR) {
				err = cfs_write_disk(fsinfo, transaction, new_entry.name_offset,
				                     newname, strlen(newname)+1);
			}
			if(err == B_NO_ERROR && new_name_size < old_name_size) {
				free_disk_space(fsinfo, transaction,
				                new_entry.name_offset + new_name_size,
				                old_name_size - new_name_size);
			}
		}
		else {
			off_t old_name_offset;
			if(err == B_NO_ERROR) {
				old_name_offset = new_entry.name_offset;
				new_entry.name_offset = new_name_locaton;
				err = cfs_write_entry_info(fsinfo, transaction, node->vnid,
				                           &new_entry);
			}
			if(err == B_NO_ERROR) {
				free_disk_space(fsinfo, transaction, old_name_offset,
				                old_name_size);
			}
		}
		if(err == B_NO_ERROR) {
			free(old_name_ptr);
		}
		else {
			free(node->name);
			node->name = old_name_ptr;
		}
	}
	if(err != B_NO_ERROR) {
		if(cfs_insert_dir(fsinfo, transaction, olddir, node) != B_NO_ERROR) {
			PRINT_ERROR(("cfs_rename: could not recover from error, "
			             "entry lost\n"));
		}
		else {
			PRINT_FLOW(("cfs_rename: failed, restored old state\n"));
		}
	}
	else {

//dprintf("cfs_rename: (%Ld) %s in %s (%Ld) was renamed to %s in %s (%Ld)\n",
//location, oldname, olddir->name, olddir->location,
//newname, newdir->name, newdir->location);

		notify_listener(B_ENTRY_MOVED, fsinfo->nsid, olddir->vnid,
		                newdir->vnid, vnid, newname);
	}
err2:
	if(err != B_NO_ERROR && new_name_locaton != 0)
		free_disk_space(fsinfo, transaction, new_name_locaton, new_name_size);
err1:
	cfs_end_transaction(fsinfo, transaction);
err1_1:
	cfs_unlock_write(fsinfo);
	if(node != NULL)
		put_vnode(fsinfo->nsid, vnid);
	if(old_dest_node != NULL)
		put_vnode(fsinfo->nsid, old_dest_vnid);
	return err;
}

int 
cfs_unlink(void *ns, void *dir, const char *name)
{
	return do_unlink((cfs_info *)ns, (cfs_node *)dir, name,
	                 CFS_UNLINK_NO_DIRS);
}

int 
cfs_rmdir(void *ns, void *dir, const char *name)
{
	return do_unlink((cfs_info *)ns, (cfs_node *)dir, name,
	                 CFS_UNLINK_DIRS_ONLY);
}

