#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include "cfs_free_space.h"
#include <KernelExport.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

status_t cfs_load_blocklist(cfs_info *fsinfo, cfs_node *node)
{
	status_t err = B_NO_ERROR;
	off_t filesize = 0; // size of disk data and blocklist
	off_t logical_size = 0;
	off_t next_data;
	cfs_vblock_list *last_block = NULL;
	cfs_entry_info entry;

	if(!S_ISREG(node->flags)) {
		return B_ERROR;
	}
			
	// see if the blocklist was previously loaded, the list may
	// still be null if there is no list.
	if(node->u.file.is_blocklist_loaded == true) {
		goto out;
	}

	err = acquire_sem_etc(fsinfo->blocklist_loader_lock, 1, B_CAN_INTERRUPT, 0);
	if(err < B_NO_ERROR) {
		PRINT_ERROR(("cfs_load_blocklist: acquire_sem_etc failed %s\n",
		             strerror(err)));
		goto out;
	}

	if(node->u.file.is_blocklist_loaded == true) {
		goto err1;
	}

	err = cfs_read_entry_info(fsinfo, (off_t)node->vnid, &entry);
	if(err != B_NO_ERROR) {
		PRINT_WARNING(("cfs_load_blocklist: id 0x%016Lx failed %s\n",
		               node->vnid, strerror(err)));
		goto err1;
	}

	next_data = entry.data;

	while(next_data != 0) {
		cfs_vblock_list *new_block = NULL;
		cfs_data_block block;
		
		if(next_data <= 0 || next_data > fsinfo->size) {
			PRINT_ERROR(("cfs_load_blocklist: vnid 0x%016Lx, name %s, "
			             "blocklist links out of filesystem bounds\n",
			             node->vnid, node->name));
			err = B_IO_ERROR;
			break;
		}
		if(filesize > fsinfo->size) {
			PRINT_ERROR(("cfs_load_blocklist: vnid 0x%016Lx, name %s, "
			             "blocklist does not end\n",
			              node->vnid, node->name));
			err = B_IO_ERROR;
			break;
		}
		
		err = cfs_read_disk(fsinfo, next_data, &block, sizeof(block));
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_load_blocklist: vnid 0x%016Lx, name %s could "
			             "not read blocklist\n", node->vnid, node->name));
			break;
		}
		if(block.data <= 0 ||
		   block.data + block.disk_size > fsinfo->size) {
			if(block.data != 0 || block.disk_size != 0 || block.raw_size == 0) {
				PRINT_ERROR(("cfs_load_blocklist: vnid 0x%016Lx, name %s, "
				             "file data out of filesystem bounds\n",
				             node->vnid, node->name));
				err = B_IO_ERROR;
				break;
			}
		}
		filesize += block.disk_size + sizeof(block);

		new_block = calloc(1, sizeof(cfs_vblock_list));
		if(new_block == NULL) {
			err = B_NO_MEMORY;
			break;
		}
		new_block->data_pos = block.data;
		if(block.raw_size != 0) {
			new_block->flags = 1;
			new_block->size = block.raw_size;
			new_block->compressed_size = block.disk_size;
		}
		else {
			new_block->flags = 0;
			new_block->size = block.disk_size;
		}
		logical_size += new_block->size;
		new_block->location = next_data;
		next_data = block.next;
		if(last_block)
			last_block->next = new_block;
		else
			node->u.file.datablocks = new_block;
		last_block = new_block;
	}

	// remove the half-loaded blocklist if there was an err
	if(next_data != 0) {
		cfs_vblock_list *cur, *next;

		PRINT_ERROR(("cfs_load_blocklist: error loading in blocklist for vnid %Ld\n", node->vnid));

		cur = node->u.file.datablocks;
		while(cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
		node->u.file.datablocks = NULL;
		node->logical_size = 0;
		if(err >= B_NO_ERROR) {
			err = B_IO_ERROR;
			goto err1;
		}
	}
	
	// even if the list is zero, it is still loaded
	node->u.file.is_blocklist_loaded = true;
	
	if(fsinfo->fs_version == 1) {
		node->logical_size = logical_size;
	}

err1:
	release_sem_etc(fsinfo->blocklist_loader_lock, 1, 0);
out:
	return err;
}

int 
cfs_read_vnode(void *ns, vnode_id vnid, char r, void **node)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *new_node;
	cfs_entry_info entry;
	char *entry_name;
	
	PRINT_FLOW(("cfs_read_vnode: id 0x%016Lx\n", vnid));

	if(cfs_vnid_to_offset(vnid) < 0 ||
	   cfs_vnid_to_offset(vnid) > ((cfs_info*)ns)->size) {
		PRINT_ERROR(("cfs_read_vnode: bad vnid, 0x%016Lx\n", vnid));
		return B_BAD_VALUE;
	}
	
	entry_name = malloc(CFS_MAX_NAME_LEN);
	if(entry_name == NULL) {
		return B_NO_MEMORY;
	}

	new_node = malloc(sizeof(cfs_node));
	if(new_node == NULL) {
		err = B_NO_MEMORY;
		goto err0;
	}
	new_node->name = NULL;

	err = cfs_read_entry(fsinfo, (off_t)vnid, &entry, entry_name);
	if(err != B_NO_ERROR) {
		PRINT_WARNING(("cfs_read_vnode: id 0x%016Lx failed %s\n",
		               vnid, strerror(err)));
		goto err1;
	}

	PRINT_FLOW(("cfs_read_vnode: id 0x%016Lx, name %s, parent 0x%016Lx\n",
	            vnid, entry_name, entry.parent));

	new_node->vnid = (off_t)vnid;
	new_node->parent = entry.parent;
	new_node->name = strdup(entry_name);
	new_node->create_time =  entry.create_time;
	new_node->mod_time =  entry.modification_time;
	new_node->attr_vnid = entry.attr;
	new_node->flags = entry.flags;
	new_node->cfs_flags = entry.cfs_flags;
	new_node->logical_size = entry.logical_size; // if v1, this will be zero
	if(entry.cfs_flags & CFS_FLAG_UNLINKED_ENTRY)
		new_node->parent = 0;
		
	switch(entry.flags & S_IFMT) {
		case S_IFDIR: {
			new_node->flags |= S_IFDIR;
			new_node->u.dir.first_entry = entry.data;
			if(cfs_vnid_to_offset(entry.data) < 0 ||
			   cfs_vnid_to_offset(entry.data) > fsinfo->size) {
				PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx, name %s, directory"
				             " entries stored out of filesystem bounds\n",
				             vnid, new_node->name));
				err = B_IO_ERROR;
				goto err1;
			}
		} break;

		case S_IFREG: {
			new_node->flags |= S_IFREG;
			new_node->u.file.datablocks = NULL;
			new_node->u.file.is_blocklist_loaded = false;

			if(fsinfo->fs_version <= 1) {
				// we need to load the blocklist now, so we can fill in the
				// vnode's logical_size field, which is in the inode in version 2+
				new_node->logical_size = 0;
				err = cfs_load_blocklist(fsinfo, new_node);
				if(err < B_NO_ERROR) {
					PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx error loading blocklist\n", vnid));
					err = B_IO_ERROR;
					goto err1;
				}
			}
		} break;

		case S_IFLNK: {
			char *link;
			size_t linksize;

			new_node->flags |= S_IFLNK;
			if(entry.data <= 0 || entry.data > fsinfo->size) {
				PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx, name %s, "
				             "symlink stored out of filesystem bounds\n",
				             vnid, new_node->name));
				err = B_IO_ERROR;
				goto err1;
			}
			
			link = malloc(PATH_MAX+1);
			if(link == NULL) {
				err = B_NO_MEMORY;
				goto err1;
			}
			linksize = PATH_MAX+1;
			err = cfs_read_disk_etc(fsinfo, entry.data, link, &linksize, 1);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx, name %s could not "
				             "read symlink\n", vnid, new_node->name));
				free(link);
				goto err1;
			}
			link[PATH_MAX] = '\0';
			if(linksize < strlen(link)) {
				PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx, name %s bad "
				             "link\n", vnid, new_node->name));
				free(link);
				err = B_IO_ERROR;
				goto err1;
			}
			new_node->u.link.link = strdup(link);
			free(link);
			if(new_node->u.link.link == NULL) {
				err = B_NO_MEMORY;
				goto err1;
			}
		} break;

		default:
			PRINT_ERROR(("cfs_read_vnode: vnid 0x%016Lx, name %s bad type\n",
			             vnid, new_node->name));
			err = B_BAD_VALUE;
			goto err1;
	}

	*node = new_node;
	//dprintf("cfs_read_vnode: vnid %Ld, name %s\n", vnid, new_node->name);
	err = B_NO_ERROR;
	goto done;
	
err1:
	if(new_node->name != NULL) free(new_node->name);
	free(new_node);
err0:
done:
	if(entry_name != NULL) free(entry_name);
	return err;
}

int 
cfs_write_vnode(void *ns, void *_node, char r)
{
	cfs_node *node = (cfs_node *)_node;
		
	PRINT_FLOW(("cfs_write_vnode: vnid 0x%016Lx, name %s\n",
	            ((cfs_node *)node)->vnid, ((cfs_node *)node)->name));
	if(S_ISLNK(node->flags)) {
		free(node->u.link.link);
	}
	else if(S_ISREG(node->flags)) {
		cfs_vblock_list *cur, *next;
		cur = node->u.file.datablocks;
		while(cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	free(node->name);
	free(node);
	return B_NO_ERROR;
}

status_t
cfs_add_unlinked_entry(cfs_info *fsinfo, cfs_transaction *transaction,
                       off_t vnid)
{
	status_t err;
	cfs_super_block_2 sb2;
	cfs_entry_info entry;

	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR)
		return err;
	err = cfs_read_entry_info(fsinfo, vnid, &entry);
	if(err != B_NO_ERROR)
		return err;

	entry.parent = 0;
	entry.next_entry = sb2.unlinked_entries;
	entry.cfs_flags |= CFS_FLAG_UNLINKED_ENTRY;
	sb2.unlinked_entries = vnid;
	
	err = cfs_write_entry_info(fsinfo, transaction, vnid, &entry);
	if(err != B_NO_ERROR)
		return err;

	if(entry.next_entry != 0) {
		cfs_entry_info next_entry;
		err = cfs_read_entry_info(fsinfo, entry.next_entry, &next_entry);
		if(err != B_NO_ERROR)
			return err;
		next_entry.parent = vnid;
		err = cfs_write_entry_info(fsinfo, transaction, entry.next_entry,
		                           &next_entry);
		if(err != B_NO_ERROR)
			return err;
	}
	err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
	                     &sb2, get_sb2_size(fsinfo->fs_version));
	if(err != B_NO_ERROR)
		return err;

	return B_NO_ERROR;
}

static status_t
cfs_remove_unlinked_entry(cfs_info *fsinfo, cfs_transaction *transaction,
                          off_t vnid)
{
	status_t err;
	cfs_entry_info entry;

	err = cfs_read_entry_info(fsinfo, vnid, &entry);
	if(err != B_NO_ERROR)
		return err;

	if(entry.next_entry != 0) {
		cfs_entry_info next_entry;
		err = cfs_read_entry_info(fsinfo, entry.next_entry, &next_entry);
		if(err != B_NO_ERROR)
			return err;
		next_entry.parent = entry.parent;
		err = cfs_write_entry_info(fsinfo, transaction, entry.next_entry,
		                           &next_entry);
		if(err != B_NO_ERROR)
			return err;
	}

	if(entry.parent == 0) {
		cfs_super_block_2 sb2;
		err = cfs_read_superblock2(fsinfo, &sb2);
		if(err != B_NO_ERROR)
			return err;
		if(sb2.unlinked_entries != vnid) {
			PRINT_ERROR(("cfs_remove_unlinked_entry: prev pointer lost\n"));
			return B_ERROR;
		}
		sb2.unlinked_entries = entry.next_entry;
		err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
		                     &sb2, get_sb2_size(fsinfo->fs_version));
		if(err != B_NO_ERROR)
			return err;
	}
	else {
		cfs_entry_info prev_entry;
	
		err = cfs_read_entry_info(fsinfo, entry.parent, &prev_entry);
		if(err != B_NO_ERROR)
			return err;
		if(prev_entry.next_entry != vnid) {
			PRINT_ERROR(("cfs_remove_unlinked_entry: prev wrong\n"));
			return B_ERROR;
		}
		prev_entry.next_entry = entry.next_entry;
		err = cfs_write_entry_info(fsinfo, transaction, entry.parent,
		                           &prev_entry);
		if(err != B_NO_ERROR)
			return err;
	}
	return B_NO_ERROR;
}

status_t
cfs_cleanup_unlinked_entries(cfs_info *fsinfo)
{
	status_t err;
	cfs_super_block_2 sb2;

	err = cfs_lock_write(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR)
		goto err;

	while(sb2.unlinked_entries != 0) {
		cfs_entry_info entry;
		off_t vnid = sb2.unlinked_entries;
		
		PRINT_WARNING(("cfs_cleanup_unlinked_entries: removing entry "
		               "0x%016Lx\n", vnid));
		err = cfs_read_entry_info(fsinfo, vnid, &entry);
		if(err != B_NO_ERROR)
			goto err;
		err = cfs_remove_entry(fsinfo, vnid, entry.flags);
		if(err != B_NO_ERROR)
			goto err;
		err = cfs_read_superblock2(fsinfo, &sb2);
		if(err != B_NO_ERROR)
			goto err;
		if(sb2.unlinked_entries == vnid) {
			PRINT_INTERNAL_ERROR(("cfs_cleanup_unlinked_entries: entry 0x%016Lx"
			                      " was not properly removed\n", vnid));
			err = B_ERROR;
			goto err;
		}
	}
	err = B_NO_ERROR;
err:
	cfs_unlock_write(fsinfo);
	return err;
}

status_t
cfs_remove_entry(cfs_info *fsinfo, off_t vnid, uint32 flags)
{
	status_t err, tmp_err;
	cfs_entry_info entry;
	char *entry_name;
	cfs_transaction *transaction;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	entry_name = malloc(CFS_MAX_NAME_LEN);
	if(entry_name == NULL) {
		return B_NO_MEMORY;
	}

	err = cfs_read_entry(fsinfo, vnid, &entry, entry_name);
	if(err != B_NO_ERROR)
		goto err1;
	//if(next_entry != NULL)
	//	*next_entry = entry.next_entry;
		
	if(entry.flags != flags) {
		PRINT_ERROR(("cfs_remove_entry: entry 0x%016Lx, name %s does not "
		             "match\n", vnid, entry_name));
		err = B_ERROR;
		goto err1;
	}

	if((entry.flags & S_ATTR) == 0) {
		while(entry.attr) {
			off_t attr_vnid = entry.attr;
			cfs_entry_info attr_entry;
			//dprintf("cfs_remove_entry: vnid %Ld, name %s has attributes\n",
			//        location, entry_name);
			err = cfs_read_entry_info(fsinfo, attr_vnid, &attr_entry);
			if(err != B_NO_ERROR)
				goto err1;

			err = cfs_start_transaction(fsinfo, &transaction);
			if(err != B_NO_ERROR)
				goto err1;

			/* attr_ptr (2) + unlink (5) */
			err = cfs_sub_transaction(fsinfo, transaction, 2 + 5);
			if(err != B_NO_ERROR)
				goto err2;
			
			entry.attr = attr_entry.next_entry;
			err = cfs_write_entry_info(fsinfo, transaction, vnid, &entry);
			if(err != B_NO_ERROR)
				goto err2;

			err = cfs_add_unlinked_entry(fsinfo, transaction, attr_vnid);
			if(err != B_NO_ERROR)
				goto err2;
		
			err = cfs_end_transaction(fsinfo, transaction);
			if(err != B_NO_ERROR)
				goto err2;

			err = cfs_remove_entry(fsinfo, attr_vnid, S_ATTR | S_IFREG);
			if(err != B_NO_ERROR)
				goto err1;
		}
	}

	err = cfs_start_transaction(fsinfo, &transaction);
	if(err != B_NO_ERROR)
		goto err1;

	switch(entry.flags & S_IFMT) {
		case S_IFDIR: {
			if(entry.data != 0) {
				PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s, "
				             "directory not empty\n",
				             vnid, entry_name));
				err = B_DIRECTORY_NOT_EMPTY;
				goto err2;
			}
		} break;

		case S_IFREG: {
			err = B_NO_ERROR;
			while(entry.data != 0) {
				off_t block_pos = entry.data;
				cfs_data_block block;
				
				if(block_pos <= 0 || block_pos > fsinfo->size) {
					PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s, "
					             "blocklist links out of filesystem bounds\n",
					             vnid, entry_name));
					err = B_IO_ERROR;
					goto err2;
				}
				
				err = cfs_read_disk(fsinfo, block_pos, &block, sizeof(block));
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s coul"
					             "d not read blocklist\n", vnid, entry_name));
					goto err2;
				}
				if(block.data <= 0 ||
				   block.data + block.disk_size > fsinfo->size) {
					if(block.data != 0 || block.disk_size != 0 || block.raw_size == 0) {
						PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s,"
						             " file data out of filesystem bounds\n",
						             vnid, entry_name));
						err = B_IO_ERROR;
						goto err2;
					}
				}
				// update entry (2), free datablock header (4/7), free datablock (4/7)
				err = cfs_sub_transaction(fsinfo, transaction,
					2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version) +
					FREE_MAX_LOG_SIZE(fsinfo->fs_version));
				if(err != B_NO_ERROR)
					goto err2;
				
				// XXX - update the logical_size field here?
				// shouldn't matter if the entry's logical size field is incorrect
				entry.data = block.next;
				cfs_write_entry_info(fsinfo, transaction, vnid, &entry);

				err = free_disk_space(fsinfo, transaction, block_pos,
				                      sizeof(block));
				if(err != B_NO_ERROR)
					goto err2;
				err = free_disk_space(fsinfo, transaction, block.data,
				                      (block.disk_size+7) & ~7);
				if(err != B_NO_ERROR)
					goto err2;
			}
		} break;

		case S_IFLNK: {
			if(entry.data == 0) {
				PRINT_WARNING(("cfs_remove_entry: vnid 0x%016Lx, name %s, "
				               "symlink has no data\n",
				               vnid, entry_name));
			}
			else {
				char *link;
				size_t linksize;
				off_t link_pos = entry.data;
				if(link_pos <= 0 || link_pos > fsinfo->size) {
					PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s, "
					             "symlink stored out of filesystem bounds\n",
					             vnid, entry_name));
					err = B_IO_ERROR;
					goto err2;
				}
				link = malloc(PATH_MAX+1);
				if(link == NULL) {
					err = B_NO_MEMORY;
					goto err2;
				}
				linksize = PATH_MAX+1;
				err = cfs_read_disk_etc(fsinfo, link_pos, link, &linksize, 1);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s "
					             "could not read link\n", vnid, entry_name));
					free(link);
					goto err2;
				}
				link[PATH_MAX] = '\0';
				if(linksize < strlen(link)) {
					PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s bad "
					             "link\n", vnid, entry_name));
					free(link);
					err = B_IO_ERROR;
					goto err2;
				}
				err = cfs_sub_transaction(fsinfo, transaction,
					2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version));
				if(err != B_NO_ERROR) {
					free(link);
					goto err2;
				}
				entry.data = 0;
				cfs_write_entry_info(fsinfo, transaction, vnid, &entry);
				err = free_disk_space(fsinfo, transaction, link_pos,
				                      cfs_string_size(link));
				free(link);
				if(err != B_NO_ERROR)
					goto err2;
			}
		} break;

		default:
			PRINT_ERROR(("cfs_remove_entry: vnid 0x%016Lx, name %s bad type\n",
			             vnid, entry_name));
			err = B_BAD_VALUE;
			goto err2;
	}
	/* unlink (4) + free entry (4/7) + free name (4/7) */
	err = cfs_sub_transaction(fsinfo, transaction,
		4 + FREE_MAX_LOG_SIZE(fsinfo->fs_version) +
		FREE_MAX_LOG_SIZE(fsinfo->fs_version));
	if(err != B_NO_ERROR)
		goto err2;

	err = cfs_remove_unlinked_entry(fsinfo, transaction, vnid);
	if(err != B_NO_ERROR)
		goto err2;

	err = free_disk_space(fsinfo, transaction, entry.name_offset,
	                      cfs_string_size(entry_name));
	if(err != B_NO_ERROR)
		goto err2;

	err = free_disk_space(fsinfo, transaction, cfs_vnid_to_offset(vnid),
	                      get_entry_size(fsinfo->fs_version));
	//if(err != B_NO_ERROR)
	//	goto err;
	//err = B_NO_ERROR;
err2:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err1:
	free(entry_name);
	return err;
}

int 
cfs_remove_vnode(void *ns, void *_node, char r)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;

	PRINT_FLOW(("cfs_remove_vnode %p %p %d\n", ns, node, r));
	err = cfs_lock_write_nointerrupt(fsinfo);
	if(err != B_NO_ERROR) {
		goto out;
	}

	err = cfs_remove_entry(fsinfo, node->vnid, node->flags);
	cfs_unlock_write(fsinfo);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_remove_vnode: vnid 0x%016Lx, name %s returned error "
		             "%s\n", node->vnid, node->name, strerror(err)));
	}

out:
	cfs_write_vnode(ns, _node, r);
	return err;
}

status_t 
cfs_update_node(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *node)
{
	status_t err;
	cfs_entry_info entry;
	char *entry_name;
	bool entry_changed = false;
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;
	
	entry_name = malloc(CFS_MAX_NAME_LEN);
	if(entry_name == NULL) {
		return B_NO_MEMORY;
	}

	err = cfs_read_entry(fsinfo, node->vnid, &entry, entry_name);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_update_node: vnid 0x%016Lx, read failed, %s\n",
		             node->vnid, strerror(err)));
		goto err;
	}
	if(strcmp(entry_name, node->name) != 0) {
		PRINT_ERROR(("cfs_update_node: vnid 0x%016Lx, "
		             "name change not supported\n", node->vnid));
		err = B_ERROR;
		goto err;
	}

	if(node->flags != entry.flags) {
		entry.flags = node->flags;
		entry_changed = true;
	}
	if(node->cfs_flags != (entry.cfs_flags & ~CFS_FLAG_UNLINKED_ENTRY)) {
		entry.cfs_flags = node->cfs_flags |
		                  (entry.cfs_flags & CFS_FLAG_UNLINKED_ENTRY);
		entry_changed = true;
	}

	if(S_ISDIR(node->flags) && node->u.dir.first_entry != entry.data) {
		entry.data = node->u.dir.first_entry;
		entry_changed = true;
	}
	if((entry.cfs_flags & CFS_FLAG_UNLINKED_ENTRY) == 0 &&
	   node->parent != entry.parent) {
		//dprintf("entry %s, parent changed 0x%016Lx->0x%016Lx\n", entry_name, entry.parent, node->parent);
		entry.parent = node->parent;
		entry_changed = true;
	}
	if(node->attr_vnid != entry.attr) {
		entry.attr = node->attr_vnid;
		entry_changed = true;
	}
	if(node->mod_time != entry.modification_time) {
		entry.modification_time = node->mod_time;
		entry_changed = true;
	}
	if(node->create_time != entry.create_time) {
		entry.create_time = node->create_time;
		entry_changed = true;
	}
	if(fsinfo->fs_version > 1) {
		if(node->logical_size != entry.logical_size) {
			entry.logical_size = node->logical_size;
			entry_changed = true;
		}
	}

	//dprintf("cfs_update_node: entry_changed %d\n", entry_changed);

	if(!entry_changed)
		goto done;
	
	err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_update_node: vnid 0x%016Lx, write, failed, %s\n",
		             node->vnid, strerror(err)));
		goto err;
	}
done:
	err = B_NO_ERROR;
err:
	free(entry_name);
	return err;
}

