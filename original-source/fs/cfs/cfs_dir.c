#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include <KernelExport.h>
#include <malloc.h>

status_t
cfs_opendir_etc(cfs_node *node, void **cookie, bool attr)
{
	cfs_dir_cookie *c;
	PRINT_FLOW(("cfs_opendir: %s\n", node->name));
	if(!attr) {
		if(!S_ISDIR(node->flags)) {
			PRINT_FLOW(("cfs_opendir: %s, not a directory\n", node->name));
			return ENOTDIR;
		}
	}
	c = malloc(sizeof(cfs_dir_cookie));
	if(c == NULL)
		return B_NO_MEMORY;
	cfs_rewinddir_etc(c, attr);
	*cookie = c;
	return B_NO_ERROR;
}

void
cfs_rewinddir_etc(cfs_dir_cookie *c, bool attr)
{
	c->last_vnid = 0;
	c->index = attr ? 2 : 0;
	c->last_name[0] = '\0';
}

status_t 
cfs_read_dir_etc(cfs_info *fsinfo, cfs_node *node, cfs_dir_cookie *c, long *num,
                 struct dirent *buf, size_t bufsize, bool attr)
{
	status_t err;
	char *entry_name = NULL;
	char *last_entry_name = NULL;

	//PRINT_FLOW(("cfs_read_dir_etc: %s, index %d, last entry %s\n",
	//           node->name, c->index, c->last_name));

	if(!attr) {
		if(!S_ISDIR(node->flags)) {
			PRINT_ERROR(("cfs_read_dir_etc: %s, not a directory\n", node->name));
			return ENOTDIR;
		}
	}

	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR)
		return err;
	
	if(c->index == 0) {
		buf->d_ino = node->vnid;
		strcpy(buf->d_name, ".");
		buf->d_reclen = 1;
		*num = 1;
	}
	else if(c->index == 1) {
		buf->d_ino = node->parent;
		strcpy(buf->d_name, "..");
		buf->d_reclen = 2;
		*num = 1;
	}
	else {
		off_t curvnid;
		cfs_entry_info entry;

		entry_name = malloc(CFS_MAX_NAME_LEN);
		if(entry_name == NULL) {
			err = B_NO_MEMORY;
			goto err;
		}
		last_entry_name = malloc(CFS_MAX_NAME_LEN);
		if(last_entry_name == NULL) {
			err = B_NO_MEMORY;
			goto err;
		}
		last_entry_name[0] = '\0';

		if(c->last_vnid == 0) {
			if(attr) {
				curvnid = node->attr_vnid;
			}
			else {
				curvnid = node->u.dir.first_entry;
			}
		}
		else {
			curvnid = c->last_vnid;
			err = cfs_read_entry(fsinfo, curvnid, &entry, entry_name);
			if(err ||
			   entry.parent != node->vnid ||
			   strcmp(entry_name, c->last_name) != 0) {
				PRINT_FLOW(("cfs_read_dir_etc: entry %Ld, parent %Ld name %s "
				            "shuld be %s, scan from start\n", curvnid,
				            entry.parent, entry_name, c->last_name));
				if(attr) {
					curvnid = node->attr_vnid;
				}
				else {
					curvnid = node->u.dir.first_entry;
				}
			} else
				curvnid = entry.next_entry;
		}
	
		*num = 0;
		while(curvnid != 0) {
			err = cfs_read_entry(fsinfo, curvnid, &entry, entry_name);
			if(err)
				goto err;

			//PRINT_FLOW(("cfs_read_dir_etc: %s, index %d, entry %s next %Ld\n",
			//           node->name, c->index, entry_name, entry.next_entry));
			
			if(entry.parent != node->vnid) {
				PRINT_ERROR(("cfs_read_dir_etc: entry 0x%016Lx, parent 0x%016Lx"
				             " shuld be 0x%016Lx\n",
				             curvnid, entry.parent, node->vnid));
				err = B_IO_ERROR;
				goto err;
			}
			if(strcmp(entry_name, last_entry_name) <= 0) {
				PRINT_ERROR(("cfs_read_dir_etc: entries not sorted, "
				             "vnid 0x%016Lx\n", curvnid));
				err = B_IO_ERROR;
				goto err;
			}
			if(strcmp(entry_name, c->last_name) > 0) {
				strcpy(c->last_name, entry_name);
				c->last_vnid = curvnid;
	
				buf->d_ino = curvnid;
				strcpy(buf->d_name, entry_name);
				buf->d_reclen = strlen(entry_name);
				*num = 1;
				break;
			}
			{
				char *tmp = entry_name;
				entry_name = last_entry_name;
				last_entry_name = tmp;
			}
			curvnid = entry.next_entry;
		}
	}
	c->index++;
	err = B_NO_ERROR;
err:
	if(last_entry_name != NULL) free(last_entry_name);
	if(entry_name != NULL) free(entry_name);
	cfs_unlock_read(fsinfo);
	return err;
}

int 
cfs_opendir(void *ns, void *node, void **cookie)
{
	return cfs_opendir_etc((cfs_node *)node, cookie, false);
}

int 
cfs_closedir(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR;
}

int 
cfs_free_dircookie(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

int 
cfs_rewinddir(void *ns, void *node, void *cookie)
{
	cfs_rewinddir_etc((cfs_dir_cookie *)cookie, false);
	return B_NO_ERROR;
}

int 
cfs_readdir(void *ns, void *node, void *cookie, long *num,
            struct dirent *buf, size_t bufsize)
{
	return cfs_read_dir_etc((cfs_info *)ns, (cfs_node *)node,
	                        (cfs_dir_cookie *)cookie, num, buf, bufsize, false);
}

status_t
cfs_lookup_dir(cfs_info *fsinfo, cfs_node *dir, const char *entry_name,
               off_t *prev_entry_vnid, off_t *entry_vnid_ptr,
               off_t *next_entry_vnid)
{
	status_t err;
	off_t entry_vnid;
	char *cur_entry_name;
	char *last_entry_name = NULL;

	err = cfs_debug_check_read_lock(fsinfo);
	if(err < B_NO_ERROR)
		return err;
	
	if (!S_ISDIR(dir->flags)) {
		PRINT_ERROR(("cfs_lookup_dir: base directory is not a directory\n"));
		return ENOTDIR;
	}

	cur_entry_name = malloc(CFS_MAX_NAME_LEN);
	if(cur_entry_name == NULL) {
		return B_NO_MEMORY;
	}

	last_entry_name = malloc(CFS_MAX_NAME_LEN);
	if(last_entry_name == NULL) {
		err = B_NO_MEMORY;
		goto err;
	}
	last_entry_name[0] = '\0';

	*prev_entry_vnid = 0;
	entry_vnid = dir->u.dir.first_entry;
	*next_entry_vnid = entry_vnid;
	while(entry_vnid != 0) {
		cfs_entry_info entry;
		int strcmp_result;
		
		err = cfs_read_entry(fsinfo, entry_vnid, &entry, cur_entry_name);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_lookup_dir: read failed\n"));
			goto err;
		}
		if(entry.parent != dir->vnid) {
			PRINT_ERROR(("cfs_lookup_dir: bad entry 0x%016Lx, parent 0x%016Lx "
			             "should be 0x%016Lx\n", entry_vnid, entry.parent,
			             dir->vnid));
			err = B_IO_ERROR;
			goto err;
		}
		if(cfs_vnid_to_offset(entry.next_entry) < 0 ||
		   cfs_vnid_to_offset(entry.next_entry) > fsinfo->size) {
			PRINT_ERROR(("cfs_lookup_dir: vnid 0x%016Lx, name %s, next "
			             "directory entry stored out of filesystem bounds\n",
			             entry_vnid, cur_entry_name));
			err = B_IO_ERROR;
			goto err;
		}
		if(strcmp(cur_entry_name, last_entry_name) <= 0) {
			PRINT_ERROR(("cfs_lookup_dir: entries not sorted, "
			             "vnid 0x%016Lx\n", entry_vnid));
			err = B_IO_ERROR;
			goto err;
		}
		strcmp_result = strcmp(cur_entry_name, entry_name);
		if(strcmp_result == 0) {
			if(entry_vnid_ptr != NULL)
				*entry_vnid_ptr = entry_vnid;
			*next_entry_vnid = entry.next_entry;
			err = B_NO_ERROR;
			goto done;
		}
		else if(strcmp_result > 0) {
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		{
			char *tmp = cur_entry_name;
			cur_entry_name = last_entry_name;
			last_entry_name = tmp;
		}
		*prev_entry_vnid = entry_vnid;
		entry_vnid = entry.next_entry;
		*next_entry_vnid = entry_vnid;
	}
	err = B_ENTRY_NOT_FOUND;
done:
err:
	if(last_entry_name != NULL) free(last_entry_name);
	if(cur_entry_name != NULL) free(cur_entry_name);
	return err;
}

status_t 
cfs_insert_dir(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *dir,
               cfs_node *node)
{
	status_t err;
	off_t prev = 0;
	off_t next = 0;
	cfs_entry_info entry;
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err < B_NO_ERROR)
		return err;
	
	err = cfs_lookup_dir(fsinfo, dir, node->name, &prev, NULL, &next);
	if(err == B_NO_ERROR) {
		return B_FILE_EXISTS;
	}
	if(err != B_ENTRY_NOT_FOUND)
		return err;

	err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
	if(err != B_NO_ERROR)
		return err;

	node->parent = dir->vnid;
	entry.parent = dir->vnid;
	entry.next_entry = next;

	err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
	if(err != B_NO_ERROR)
		return err;

	if(prev == 0) {
		dir->u.dir.first_entry = node->vnid;
		dir->mod_time = time(NULL);
		cfs_update_node(fsinfo, transaction, dir);
	}
	else {
		err = cfs_read_entry_info(fsinfo, prev, &entry);
		if(err != B_NO_ERROR)
			return err;
		entry.next_entry = node->vnid;
		err = cfs_write_entry_info(fsinfo, transaction, prev, &entry);
		if(err != B_NO_ERROR)
			return err;
		dir->mod_time = time(NULL);
		cfs_update_node(fsinfo, transaction, dir);
	}
	return B_NO_ERROR;
}

status_t
cfs_remove_dir(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *dir,
               cfs_node *node)
{
	status_t err;
	off_t prev = 0;
	off_t vnid = 0;
	off_t next = 0;
	cfs_entry_info entry;
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err < B_NO_ERROR)
		return err;
	
	err = cfs_lookup_dir(fsinfo, dir, node->name, &prev, &vnid, &next);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_remove_dir: entry %s not found\n", node->name));
		return err;
	}
	if(node->vnid != vnid) {
		PRINT_ERROR(("cfs_remove_dir: entry %s, dir id 0x%016Lx does not match "
		             "vnode 0x%016Lx\n", node->name, vnid, node->vnid));
		return err;
	}
	
	if(prev == 0) {
		dir->u.dir.first_entry = next;
		dir->mod_time = time(NULL);
		err = cfs_update_node(fsinfo, transaction, dir);
		if(err != B_NO_ERROR) {
			dir->u.dir.first_entry = vnid;
			return err;
		}
	}
	else {
		err = cfs_read_entry_info(fsinfo, prev, &entry);
		if(err != B_NO_ERROR)
			return err;
		entry.next_entry = next;
		err = cfs_write_entry_info(fsinfo, transaction, prev, &entry);
		if(err != B_NO_ERROR)
			return err;

		dir->mod_time = time(NULL);
		cfs_update_node(fsinfo, transaction, dir);
	}

	node->parent = 0;
	err = cfs_update_node(fsinfo, transaction, node);
	if(err != B_NO_ERROR)
		return err;

	return B_NO_ERROR;
}
