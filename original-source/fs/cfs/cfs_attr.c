#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include "cfs_free_space.h"
#include <KernelExport.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

int 
cfs_open_attrdir(void *ns, void *node, void **cookie)
{
	return cfs_opendir_etc((cfs_node *)node, cookie, true);
}

int 
cfs_close_attrdir(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR;
}

int 
cfs_free_attrcookie(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

int 
cfs_rewind_attrdir(void *ns, void *node, void *cookie)
{
	cfs_rewinddir_etc((cfs_dir_cookie *)cookie, true);
	return B_NO_ERROR;
}

int 
cfs_read_attrdir(void *ns, void *node, void *cookie, long *num,
                 struct dirent *buf, size_t bufsize)
{
	return cfs_read_dir_etc((cfs_info *)ns, (cfs_node *)node,
	                        (cfs_dir_cookie *)cookie, num, buf, bufsize, true);
}

static status_t
cfs_lookup_attr(cfs_info *fsinfo, cfs_node *base, const char *entry_name,
                off_t *prev_entry_vnid, off_t *entry_vnid_ptr,
                off_t *next_entry_vnid, cfs_entry_info *entry)
{
	status_t err;
	off_t entry_vnid;
	char *cur_entry_name;
	err = cfs_debug_check_read_lock(fsinfo);
	if(err < B_NO_ERROR)
		return err;

	cur_entry_name = malloc(CFS_MAX_NAME_LEN);
	if(cur_entry_name == NULL) {
		return B_NO_MEMORY;
	}
	
	*prev_entry_vnid = 0;
	entry_vnid = base->attr_vnid;
	*next_entry_vnid = entry_vnid;
	while(entry_vnid != 0) {
		int strcmp_result;
		
		err = cfs_read_entry(fsinfo, entry_vnid, entry, cur_entry_name);
		if(err != B_NO_ERROR)
			goto err;
		if(entry->parent != base->vnid) {
			PRINT_ERROR(("cfs_lookup_attr: bad entry id 0x%016Lx, parent "
			             "0x%016Lx should be 0x%016Lx\n", entry_vnid,
			             entry->parent, base->vnid));
			err = B_IO_ERROR;
			goto err;
		}
		if(cfs_vnid_to_offset(entry->next_entry) < 0 ||
		   cfs_vnid_to_offset(entry->next_entry) > fsinfo->size) {
			PRINT_ERROR(("cfs_lookup_attr: vnid 0x016%Lx, name %s, next "
			             "directory entry stored out of filesystem bounds\n",
			             entry_vnid, cur_entry_name));
			err = B_IO_ERROR;
			goto err;
		}
		strcmp_result = strcmp(cur_entry_name, entry_name);
		if(strcmp_result == 0) {
			if(entry_vnid_ptr != NULL)
				*entry_vnid_ptr = entry_vnid;
			*next_entry_vnid = entry->next_entry;
			err = B_NO_ERROR;
			goto done;
		}
		else if(strcmp_result > 0) {
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		*prev_entry_vnid = entry_vnid;
		entry_vnid = entry->next_entry;
		*next_entry_vnid = entry_vnid;
	}
	err = B_ENTRY_NOT_FOUND;
done:
err:
	if(cur_entry_name != NULL) free(cur_entry_name);
	return err;
}

static status_t
cfs_add_attr(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *node,
             const char *name, int type, off_t *vnid)
{
	status_t err;
	cfs_entry_info attr_entry;
	size_t attr_entry_size;
	off_t prev_attr, next_attr;
	off_t new_location;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	err = cfs_lookup_attr(fsinfo, node, name, &prev_attr, vnid, &next_attr,
	                      &attr_entry);
	if(err != B_ENTRY_NOT_FOUND)
		return err;	/* attr already exists, or fatal error occured */
			
	attr_entry.parent = node->vnid;
	attr_entry.next_entry = next_attr;
	attr_entry.data = 0;
	attr_entry.attr = type;
	attr_entry.flags = S_ATTR | S_IFREG;
	attr_entry.version = cfs_inc_version(fsinfo->last_entry_version);
	attr_entry.logical_size = 0;

	attr_entry_size = get_entry_size(fsinfo->fs_version) + cfs_string_size(name);

	/* 4/7 blocks for freespace list + max blocks used by entry (1 +
	   entrysize/blocksize) + 2 blocks for prev attr or parent entry */
	err = cfs_sub_transaction(fsinfo, transaction, ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 1 +
	                          attr_entry_size / fsinfo->cache_block_size + 2);
	if(err != B_NO_ERROR)
		return err;

	err = allocate_disk_space(fsinfo, transaction, &new_location,
	                          attr_entry_size, METADATA);
	if(err != B_NO_ERROR)
		return err;

	attr_entry.name_offset = new_location + get_entry_size(fsinfo->fs_version);
	err = cfs_write_disk(fsinfo, transaction, attr_entry.name_offset,
	                     name, cfs_string_size(name));
	if(err != B_NO_ERROR)
		goto err;

	*vnid = cfs_offset_to_vnid(new_location, attr_entry.version);

	err = cfs_write_entry_info(fsinfo, transaction, *vnid, &attr_entry);
	if(err != B_NO_ERROR)
		goto err;

	if(prev_attr == 0) {
		node->attr_vnid = *vnid;
		err = cfs_update_node(fsinfo, transaction, node);
		if(err != B_NO_ERROR)
			goto err;
	}
	else {
		cfs_entry_info prev_attr_entry;

		err = cfs_read_entry_info(fsinfo, prev_attr, &prev_attr_entry);
		if(err != B_NO_ERROR)
			goto err;
		if(prev_attr_entry.next_entry != attr_entry.next_entry) {
			PRINT_ERROR(("cfs_add_attr: attr entry list changed\n"));
			err = B_IO_ERROR;
			goto err;
		}
		prev_attr_entry.next_entry = *vnid;
		err = cfs_write_entry_info(fsinfo, transaction, prev_attr,
		                           &prev_attr_entry);
		if(err != B_NO_ERROR)
			goto err;
	}
	return B_NO_ERROR;
	
err:
	free_disk_space(fsinfo, transaction, new_location, attr_entry_size);
	return err;
}

int 
cfs_write_attr(void *ns, void *_node, const char *name, int type,
               const void *buf, size_t *len, off_t pos)
{
	status_t err, tmp_err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	cfs_node *attr_node;
	off_t attr_vnid;
	cfs_transaction *transaction;

	PRINT_FLOW(("cfs_write_attr: file %s, attr %s, pos %Ld, len %ld\n",
	            node->name, name, pos, *len));

	if(strlen(name) >= CFS_MAX_NAME_LEN) {
		err = B_BAD_VALUE;
		goto err1;
	}

	err = cfs_lock_write(fsinfo);
	if(err)
		goto err1;

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err1_1;

	err = cfs_add_attr(fsinfo, transaction, node, name, type, &attr_vnid);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_read_vnode(fsinfo, attr_vnid, true, (void **)&attr_node);
	if(err != B_NO_ERROR)
		goto err2;
	
	err = cfs_write_data(fsinfo, transaction, attr_node, pos, buf, len, false);
	cfs_write_vnode(fsinfo, attr_node, true);
err2:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err1_1:
	cfs_unlock_write(fsinfo);
err1:
	if(err != B_NO_ERROR)
		*len = 0;
	return err;
}

int 
cfs_read_attr(void *ns, void *_node, const char *name, int type,
              void *buf, size_t *len, off_t pos)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	cfs_node *attr_node;
	cfs_entry_info attr_entry;
	off_t attr_vnid, prev_attr, next_attr;

	PRINT_FLOW(("cfs_read_attr: file %s, attr %s, pos %Ld, len %ld\n",
	            node->name, name, pos, *len));

	if(strlen(name) >= CFS_MAX_NAME_LEN) {
		err = B_BAD_VALUE;
		goto err1;
	}

	err = cfs_lock_read(fsinfo);
	if(err)
		goto err1;
	
	err = cfs_lookup_attr(fsinfo, node, name, &prev_attr, &attr_vnid,
	                      &next_attr, &attr_entry);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_read_vnode(fsinfo, attr_vnid, true, (void **)&attr_node);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_read_data(fsinfo, attr_node, pos, buf, len);
	cfs_write_vnode(fsinfo, attr_node, true);
err2:
	cfs_unlock_read(fsinfo);
err1:
	if(err != B_NO_ERROR)
		*len = 0;
	return err;
}

int 
cfs_stat_attr(void *ns, void *_node, const char *name, struct attr_info *buf)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	cfs_node *attr_node;

	off_t prev_attr, attr_vnid, next_attr;
	cfs_entry_info attr_entry;

	PRINT_FLOW(("cfs_stat_attr\n"));

	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR) {
		return err;
	}
	
	err = cfs_lookup_attr(fsinfo, node, name, &prev_attr, &attr_vnid,
	                      &next_attr, &attr_entry);
	if(err != B_NO_ERROR)
		goto err;
		
	buf->type = attr_entry.attr;

	err = cfs_read_vnode(fsinfo, attr_vnid, true, (void **)&attr_node);
	if(err != B_NO_ERROR)
		goto err;

	if(S_ISREG(attr_node->flags)) {
		buf->size = attr_node->logical_size;
	} else {
		buf->size = 0;
	}
	cfs_write_vnode(fsinfo, attr_node, true);

	err = B_NO_ERROR;
err:
	cfs_unlock_read(fsinfo);
	return err;
}

int 
cfs_remove_attr(void *ns, void *_node, const char *name)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	off_t prev_attr_vnid, attr_vnid, next_attr_vnid;
	cfs_entry_info prev_attr_entry;
	cfs_entry_info attr_entry;
	cfs_transaction *transaction;

	PRINT_FLOW(("cfs_remove_attr: file %s, attr %s\n", node->name, name));

	if(strlen(name) >= CFS_MAX_NAME_LEN) {
		return B_BAD_VALUE;
	}

	err = cfs_lock_write(fsinfo);
	if(err)
		goto err1;
		
	err = cfs_lookup_attr(fsinfo, node, name, &prev_attr_vnid,
	                      &attr_vnid, &next_attr_vnid, &attr_entry);
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err2;
	/* prev entry/parent (2) + unlink (5) */
	err = cfs_sub_transaction(fsinfo, transaction, 2 + 5);
	if(err != B_NO_ERROR)
		goto err3;
	
	if(prev_attr_vnid == 0) {
		node->attr_vnid = next_attr_vnid;
		err = cfs_update_node(fsinfo, transaction, node);
		if(err != B_NO_ERROR)
			goto err3;
	}
	else {
		err = cfs_read_entry_info(fsinfo, prev_attr_vnid, &prev_attr_entry);
		if(err != B_NO_ERROR)
			goto err3;
		if(prev_attr_entry.parent != node->vnid) {
			PRINT_ERROR(("cfs_remove_attr: entry %Ld, prev attr entry parent, "
			             "%Ld, is wrong\n", node->vnid,
			             prev_attr_entry.parent));
		}
		prev_attr_entry.next_entry = next_attr_vnid;
		err = cfs_write_entry_info(fsinfo, transaction, prev_attr_vnid,
		                           &prev_attr_entry);
		if(err != B_NO_ERROR)
			goto err3;
	}
	err = cfs_add_unlinked_entry(fsinfo, transaction, attr_vnid);
	if(err != B_NO_ERROR)
		goto err3;

	err = cfs_end_transaction(fsinfo, transaction);
	if(err != B_NO_ERROR)
		goto err2;
	
	err = cfs_remove_entry(fsinfo, attr_vnid, S_ATTR | S_IFREG);
	if(err != B_NO_ERROR)
		goto err2;

	PRINT_FLOW(("cfs_remove_attr: done, file %s, attr %s done\n",
	            node->name, name));

	cfs_unlock_write(fsinfo);
	return B_NO_ERROR;

err3:
	cfs_end_transaction(fsinfo, transaction);
err2:
	cfs_unlock_write(fsinfo);
err1:
	PRINT_FLOW(("cfs_remove_attr: failed, %s\n", strerror(err)));
	return err;
}

