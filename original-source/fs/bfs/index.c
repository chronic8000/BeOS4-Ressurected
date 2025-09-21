/*
   this file contains the interface to the indexing code.  it maintains
   the main global indices (name, size, creation time, last modified time)
   as well as provide the glue to get to user indices.

   this file is dedicated to the greatness that is Led Zepplin.
*/   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "bfs.h"
#include "btlib/btree.h"
#ifdef __BEOS__
#include <fs_index.h>
#include <Mime.h>
#endif

static void call_callbacks(bfs_inode *index, bfs_inode *file,
						   int type_of_event, int *res);

int
make_index(bfs_info *bfs, bfs_inode *dir, bfs_inode **res, const char *name,
		   int mode)
{
	int      ret;
	vnode_id result = BT_ANY_RRN;
	
	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	LOCK(dir->etc->lock);

	dir->etc->counter++;
	if (bt_find(dir->etc->btree,(bt_chrp)name,strlen(name),&result) == BT_OK) {
		ret = EEXIST;
		goto err0;
	}
	

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		printf("make_index: can't start transaction (dir @ 0x%x)!\n", dir);
		ret = ENOMEM;
		goto err0;
	}

	*res = make_dir(bfs, dir, mode, &result);
	if (*res == NULL) {
		ret = ENOSPC;
		goto err;
	}

	dir->etc->counter++;
	if (bt_insert(dir->etc->btree, (bt_chrp)name, strlen(name),result,0) != BT_OK) {
		ret = ENOSPC;
		goto err;
	}

	end_transaction(bfs, bfs->cur_lh);
	UNLOCK(dir->etc->lock);
	
	return 0;

err:
	printf("XXXdbg -- need to remove the index!\n");
	abort_transaction(bfs, bfs->cur_lh);
err0:
	UNLOCK(dir->etc->lock);
	return ret;
}


int
create_indices(bfs_info *bfs)
{
	vnode_id    result;
	bfs_inode  *bi, _bi;
	inode_addr  ia;
	
	bi = &_bi;
	
	vnode_id_to_inode_addr(bi->inode_num, 0);
	
	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);
	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {;
		printf("create_indices: can't start transaction!\n");
		release_sem_etc(bfs->sem, MAX_READERS, 0);
		return ENOMEM;
	}

	if ((bi = make_dir(bfs, bi, S_INDEX_DIR | S_IFDIR | S_IRWXU, &result)) == 0) {
		abort_transaction(bfs, bfs->cur_lh);
		goto err;
	}

	bi->parent = bi->inode_num;
	update_inode(bfs, bi);

	end_transaction(bfs, bfs->cur_lh);

	bfs->index_index = bi;
	bfs->dsb.indices = bi->inode_num;

	ia = bfs->dsb.indices;

	if (make_index(bfs, bi, &bfs->name_index, "name",
				   S_INDEX_DIR | S_IFDIR) != 0)
		goto err;

	if (make_index(bfs, bi, &bfs->size_index, "size",
				   S_INDEX_DIR | S_LONG_LONG_INDEX | S_IFDIR) != 0)
		goto err;

#if 0
	if (make_index(bfs, bi, &bfs->create_time_index, "created",
				   S_INDEX_DIR | S_LONG_LONG_INDEX | S_IFDIR) != 0)
		goto err;
#endif

	if (make_index(bfs, bi, &bfs->last_modified_time_index, "last_modified",
				   S_INDEX_DIR | S_LONG_LONG_INDEX | S_IFDIR) != 0)
		goto err;
	
	release_sem_etc(bfs->sem, MAX_READERS, 0);
	return 0;

 err:
	release_sem_etc(bfs->sem, MAX_READERS, 0);
	return EINVAL;

}


int
get_sys_index(bfs_info *bfs, const char *name, bfs_inode **index)
{
	int        ret;
	dr9_off_t      id = BT_ANY_RRN;
	vnode_id   vnid;
	bfs_inode *bi;
	
 	if (bfs->index_index == NULL)
		return ENOENT;

	bi = (bfs_inode *)bfs->index_index;
	LOCK(bi->etc->lock);
	
	bi->etc->counter++;
	if (bt_find(bi->etc->btree, (bt_chrp)name, strlen(name), &id) != BT_OK) {
		UNLOCK(bi->etc->lock);
		return ENOENT;
	}
	
	vnid = (vnode_id)id;

	ret = bfs_read_vnode(bfs, vnid, 0, (void **)index);
	UNLOCK(bi->etc->lock);

	return ret;
}


/*
   this just checks if a vnode exists (returns true if it does,
   false otherwise).
*/   
int
index_exists(bfs_info *bfs, const char *name)
{
	int        ret = 0;
	dr9_off_t  id  = BT_ANY_RRN;
	bfs_inode *bi;
	
	if (bfs->index_index == NULL)
		return 0;

	bi = (bfs_inode *)bfs->index_index;
	LOCK(bi->etc->lock);
	
	bi->etc->counter++;
	ret = bt_find(bi->etc->btree, (bt_chrp)name, strlen(name), &id);
	if (ret == BT_OK)
		ret = 1;
	else
		ret = 0;


	UNLOCK(bi->etc->lock);
	return 1;
}


/*
   this routine calls get_vnode() which means that you need to call
   put_vnode() when you're done with the inode that this function
   finds.
*/   
int
get_index(bfs_info *bfs, const char *name, bfs_inode **index)
{
	int        ret;
	dr9_off_t      id = BT_ANY_RRN;
	vnode_id   vnid;
	bfs_inode *bi;
	
	if (bfs->index_index == NULL)
		return ENOENT;

	bi = (bfs_inode *)bfs->index_index;
	LOCK(bi->etc->lock);
	
	bi->etc->counter++;
	if (bt_find(bi->etc->btree, (bt_chrp)name, strlen(name), &id) != BT_OK) {
		UNLOCK(bi->etc->lock);
		return ENOENT;
	}
	
	vnid = (vnode_id)id;

	ret = get_vnode(bfs->nsid, id, (void **)index);

	/* don't allow queries on deleted indices */
	if (ret == 0 && ((*index)->flags & INODE_DELETED)) {
		put_vnode(bfs->nsid, id);
		ret = ENOENT;
	}
		

	UNLOCK(bi->etc->lock);
	return ret;
}


int
init_indices(bfs_info *bfs)
{
	int err;
	vnode_id vnid;

	vnid = inode_addr_to_vnode_id(bfs->dsb.indices);

	if (vnid == 0)   /* then no indices on this filesystem */
		return 0;
	
	bfs->index_index = NULL;
	if (bfs_read_vnode(bfs, vnid, 0, (void **)&bfs->index_index) != 0)
		return EINVAL;

	if (bfs->index_index->etc->btree == NULL)
		goto err;

	err = get_sys_index(bfs, "name", &bfs->name_index);
	if (err == ENOENT)
		printf("bfs 0x%x: No Name index!\n", bfs);
	else if (err)
		goto err;

	err = get_sys_index(bfs, "size", &bfs->size_index);
	if (err == ENOENT)
		printf("bfs 0x%x: No Size index!\n", bfs);
	else if (err)
		goto err;

#if 0
	err = get_sys_index(bfs, "created", &bfs->create_time_index);
	if (err == ENOENT)
		printf("bfs 0x%x: No Name index!\n", bfs);
	else if (err)
		goto err;
#endif

	err = get_sys_index(bfs,"last_modified",&bfs->last_modified_time_index);
	if (err == ENOENT)
		printf("bfs 0x%x: No Last Modified index!\n", bfs);
	else if (err)
		goto err;

	return 0;

 err:
	bfs_write_vnode(bfs, bfs->index_index, 0);

	if (bfs->name_index)
		bfs_write_vnode(bfs, bfs->name_index, 0);

	if (bfs->size_index)
		bfs_write_vnode(bfs, bfs->size_index, 0);

	if (bfs->create_time_index)
		bfs_write_vnode(bfs, bfs->create_time_index, 0);

	if (bfs->last_modified_time_index)
		bfs_write_vnode(bfs, bfs->last_modified_time_index, 0);
		
	return EINVAL;
}

void
shutdown_indices(bfs_info *bfs)
{
	if (bfs->last_modified_time_index)
		bfs_write_vnode(bfs, bfs->last_modified_time_index, 0);
	bfs->last_modified_time_index = NULL;
	
	if (bfs->create_time_index)
		bfs_write_vnode(bfs, bfs->create_time_index, 0);
	bfs->create_time_index = NULL;
	
	if (bfs->size_index)
		bfs_write_vnode(bfs, bfs->size_index, 0);
	bfs->size_index = NULL;
	
	if (bfs->name_index)
		bfs_write_vnode(bfs, bfs->name_index, 0);
	bfs->name_index = NULL;
	
	if (bfs->index_index)
		bfs_write_vnode(bfs, bfs->index_index, 0);
	bfs->index_index = NULL;
}

int
add_to_main_indices(bfs_info *bfs, const char *name, bfs_inode *file)
{
	int        err;
	bfs_inode *index;

	if (bfs->name_index) {
		index = (bfs_inode *)bfs->name_index;
		LOCK(index->etc->lock);

		if (index->etc->btree == NULL)
			bfs_die("null name index btree?!? (index @ 0x%lx)\n",(ulong)index);
		
		index->etc->counter++;
		if ((err = bt_insert(index->etc->btree, (bt_chrp)name, strlen(name),
							 file->etc->vnid, 1)) != BT_OK) {
			bt_perror(index->etc->btree, "error during main name index add");
			if (bt_errno(index->etc->btree) == BT_NOSPC) {
				printf("*** failed to insert %s (%Ld) in name index\n",
					   name, file->etc->vnid);
				UNLOCK(index->etc->lock);
				return EINVAL;
			} else if (bt_errno(index->etc->btree) == BT_DUPRRN)
				printf("*** name %s (%Ld) already in name index\n", name,
					   file->etc->vnid);
			else
				bfs_die("*** failed to insert %s in name index\n", name);
		}
			
		call_callbacks(bfs->name_index, file, NEW_FILE, NULL);

		UNLOCK(index->etc->lock);
	}
	
	if (S_ISDIR(file->mode))
		return 0;

	if (bfs->create_time_index) {
		index = (bfs_inode *)bfs->create_time_index;
		LOCK(index->etc->lock);
		
		if (index->etc->btree == NULL)
			bfs_die("null create time index btree?!? (index @ 0x%lx)\n",
					(ulong)index);

		index->etc->counter++;
		if ((err = bt_insert(index->etc->btree, (bt_chrp)&file->create_time,
							 sizeof(bigtime_t),file->etc->vnid, 1)) != BT_OK) {
			dr9_off_t val;

			bt_perror(index->etc->btree, "add: create time index add");
			if (bt_errno(index->etc->btree) == BT_NOSPC) {
				printf("*** failed to insert %s (bi @ 0x%lx) in creation time"
					   " index (err %d)\n", name, file, err);
				UNLOCK(index->etc->lock);
				
				/* delete the file from the main name index since we failed */
				index = bfs->name_index;
				if (index == NULL)
					return EINVAL;
			
				LOCK(index->etc->lock);
				val = file->etc->vnid;
				index->etc->counter++;
				if ((err = bt_delete(index->etc->btree, (bt_chrp)name,
									 strlen(name), &val)) != BT_OK) {
					bt_perror(index->etc->btree, "add: name index delete");
				}
				UNLOCK(index->etc->lock);
			
				return EINVAL;
			} else if (bt_errno(index->etc->btree) == BT_DUPRRN)
				printf("*** %s time %Ld (bi @ 0x%lx) already in creation time"
					   " index (err %d)\n", name, file->create_time, file,err);
			else
				bfs_die("*** failed to insert %s (bi @ 0x%lx) in creation time"
						" index (err %d)\n", name, file, err);
		}

		call_callbacks(bfs->create_time_index, file, NEW_FILE, NULL);

		UNLOCK(index->etc->lock);
	}
	
	if (S_ISLNK(file->mode)) {
		update_last_modified_time_index(bfs, file);
	}


	return 0;
}


int
del_from_main_indices(bfs_info *bfs, const char *name, bfs_inode *file)
{
	int        err;
	dr9_off_t      val;
	bfs_inode *index;

	if (bfs->name_index) {
		index = (bfs_inode *)bfs->name_index;
		LOCK(index->etc->lock);

		val = file->etc->vnid;
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree, (bt_chrp)name, strlen(name),
							 &val)) != BT_OK) {
			if (err != BT_NF) {
				bt_perror(index->etc->btree, "del: name index delete");
				bfs_die("*** failed to delete %s (0x%Lx) from name index\n",
						name,val);
			}
		}
			
		call_callbacks(bfs->name_index, file, DELETE_FILE, NULL);

		UNLOCK(index->etc->lock);
	}
	
	kill_name_attr(bfs, file);     /* so other queries won't find it */

	if (S_ISDIR(file->mode))
		return 0;

	if (bfs->create_time_index) {
		index = (bfs_inode *)bfs->create_time_index;
		LOCK(index->etc->lock);

		val = file->etc->vnid;
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree, (bt_chrp)&file->create_time,
							 sizeof(bigtime_t), &val)) != BT_OK) {
			if (err != BT_NF) {
				bt_perror(index->etc->btree, "del: create time index delete");
				bfs_die("*** failed to delete %s (0x%Lx) from creation time "
						"index\n", name, val);
			}
		}

		call_callbacks(bfs->create_time_index, file, DELETE_FILE, NULL);

		UNLOCK(index->etc->lock);
	}


	if (bfs->last_modified_time_index && file->etc->old_mtime >= 0) {

		index = (bfs_inode *)bfs->last_modified_time_index;
		LOCK(index->etc->lock);

		val = file->etc->vnid;
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree,
							 (bt_chrp)&file->last_modified_time,
							 sizeof(bigtime_t), &val)) != BT_OK) {
			if (err == BT_NF) {
				val = file->etc->vnid;
				err = bt_delete(index->etc->btree,
								 (bt_chrp)&file->etc->old_mtime,
								 sizeof(bigtime_t), &val);
				if (err == BT_NF)
					printf("failed to delete bi 0x%x from mtime index\n", file);
			}

			if (err != BT_OK && err != BT_NF) {
				bt_perror(index->etc->btree, "del: modified time index delete");
				bfs_die("*** failed to delete %s (0x%Lx) from modified time "
						"index\n", name, val);
			}
		}

		call_callbacks(bfs->last_modified_time_index, file, DELETE_FILE, NULL);

		UNLOCK(index->etc->lock);
	}

	file->etc->old_mtime = -1;

	if (S_ISLNK(file->mode))
		return 0;

	if (bfs->size_index && file->etc->old_size >= 0) {
		index = (bfs_inode *)bfs->size_index;
		LOCK(index->etc->lock);

		val = file->etc->vnid;
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree, (bt_chrp)&file->etc->old_size,
							 sizeof(dr9_off_t), &val)) != BT_OK) {
			if (err == BT_NF) {
				val = file->etc->vnid;
				err = bt_delete(index->etc->btree, (bt_chrp)&file->data.size,
								 sizeof(dr9_off_t), &val);
				if (err == BT_NF)
					printf("could not delete size for file 0x%x\n", file);
			}

			if (err != BT_OK && err != BT_NF) {
				bt_perror(index->etc->btree, "del: size index delete");
				bfs_die("*** failed to delete %s (0x%Lx) from size index\n",
						name, val);
			}
		}

		call_callbacks(bfs->size_index, file, DELETE_FILE, NULL);

		UNLOCK(index->etc->lock);
	}

	file->etc->old_size = -1;

	return 0;
}



/*
   this assumes a transaction has been started already and that
   update_inode() will be called afterwards.
*/   
int
update_create_time_index(bfs_info *bfs, bfs_inode *bi, bigtime_t new_time)
{
	int        err;
	dr9_off_t      val;
	bfs_inode *index;

	if (S_ISDIR(bi->mode))
		return 0;

	if (bfs->create_time_index == NULL) {
		bi->create_time = new_time;
		return 0;
	}
		
	index = (bfs_inode *)bfs->create_time_index;
	LOCK(index->etc->lock);

	val = bi->etc->vnid;
	index->etc->counter++;
	if ((err = bt_delete(index->etc->btree, (bt_chrp)&bi->create_time,
						 sizeof(bigtime_t), &val)) != BT_OK) {
		if (err != BT_NF) {
			bt_perror(index->etc->btree, "update: create time index delete");
			bfs_die("*** failed to delete time %Ld (vnid 0x%Lx) from creation "
					"time index\n",	bi->create_time, bi->etc->vnid);
		}
	}

	call_callbacks(bfs->create_time_index, bi, DELETE_FILE, NULL);

	bi->create_time = new_time;

	index->etc->counter++;
	if ((err = bt_insert(index->etc->btree, (bt_chrp)&bi->create_time,
						 sizeof(bigtime_t), bi->etc->vnid, 1)) != BT_OK) {
		bt_perror(index->etc->btree, "update: create time index add");
		if (bt_errno(index->etc->btree) == BT_NOSPC) {
			printf("*** failed to insert time %Ld (vnid 0x%Lx) in creation "
				   "time index (err %d)\n", bi->create_time, bi->etc->vnid, err);
			UNLOCK(index->etc->lock);
			return ENOSPC;
		} else if (bt_errno(index->etc->btree) == BT_DUPRRN)
			printf("*** time %Ld (vnid 0x%Lx) already in creation time index "
				   "(err %d)\n", bi->create_time, bi->etc->vnid, err);
		else 
			bfs_die("*** failed to insert time %Ld (vnid 0x%Lx) in creation "
					"time index (err %d)\n", bi->create_time, bi->etc->vnid, err);
	}

	call_callbacks(bfs->create_time_index, bi, NEW_FILE, NULL);

	UNLOCK(index->etc->lock);
	return 0;
}

int
update_last_modified_time_index(bfs_info *bfs, bfs_inode *bi)
{
	int        err;
	dr9_off_t      val;
	bfs_inode *index;

	if (S_ISDIR(bi->mode))
		return 0;

	if (bfs->last_modified_time_index == NULL) {
		return 0;
	}
		
	index = (bfs_inode *)bfs->last_modified_time_index;
	LOCK(index->etc->lock);
	val = bi->etc->vnid;
	if (bi->etc->old_mtime >= 0) {
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree, (bt_chrp)&bi->etc->old_mtime,
							 sizeof(bigtime_t), &val)) != BT_OK) {
			if (err != BT_NF) {
				bt_perror(index->etc->btree, "update: modified time index delete");
				bfs_die("*** failed to delete time %Ld (vnid 0x%Lx) from "
						"modified time index\n", bi->etc->old_mtime,
						bi->etc->vnid);
			}
		}

		call_callbacks(bfs->last_modified_time_index, bi, DELETE_FILE, NULL);
	}

	bi->etc->old_mtime = bi->last_modified_time;

	index->etc->counter++;
	if ((err = bt_insert(index->etc->btree, (bt_chrp)&bi->last_modified_time,
						 sizeof(bigtime_t), bi->etc->vnid, 1)) != BT_OK) {
		bt_perror(index->etc->btree, "update: modified time index add");
		if (bt_errno(index->etc->btree) == BT_NOSPC) {
			printf("*** failed to insert time %Ld (vnid 0x%Lx) in modified "
					"time index (err %d)\n", bi->last_modified_time,
					bi->etc->vnid, err);
			UNLOCK(index->etc->lock);
			return ENOSPC;
		} else if (bt_errno(index->etc->btree) == BT_DUPRRN) 
			printf("*** time %Ld (vnid 0x%Lx) already in modified time index "
				   "(err %d)\n", bi->last_modified_time, bi->etc->vnid, err);
		else 
			bfs_die("*** failed to insert time %Ld (vnid 0x%Lx) in modified "
					"time index (err %d)\n", bi->last_modified_time,
					bi->etc->vnid, err);
	}

	call_callbacks(bfs->last_modified_time_index, bi, NEW_FILE, NULL);


	UNLOCK(index->etc->lock);
	return 0;
}

int
update_size_index(bfs_info *bfs, bfs_inode *bi)
{
	int        err;
	dr9_off_t      val;
	bfs_inode *index;

	if (S_ISDIR(bi->mode) || S_ISLNK(bi->mode))
		return 0;

	if (bfs->size_index == NULL) {
		return 0;
	}
		
	if (bi->data.size == bi->etc->old_size) {
		printf("update size index for bi 0x%x but size == old_size?\n", bi);
		return 0;
	}

	index = (bfs_inode *)bfs->size_index;
	LOCK(index->etc->lock);

	val = bi->etc->vnid;
	if (bi->etc->old_size >= 0) {
		index->etc->counter++;
		if ((err = bt_delete(index->etc->btree, (bt_chrp)&bi->etc->old_size,
							 sizeof(dr9_off_t), &val)) != BT_OK) {
			if (err != BT_NF) {
				bt_perror(index->etc->btree, "update: size index delete");
				bfs_die("*** failed to delete size %Ld (vnid 0x%Lx) from size "
					"time index\n",	bi->etc->old_size, bi->etc->vnid);
			}
		}

		call_callbacks(bfs->size_index, bi, DELETE_FILE, NULL);
	}

	bi->etc->old_size = bi->data.size;

	index->etc->counter++;
	if ((err = bt_insert(index->etc->btree, (bt_chrp)&bi->data.size,
						 sizeof(dr9_off_t), bi->etc->vnid, 1)) != BT_OK) {
		bt_perror(index->etc->btree, "update: size index add");
		if (bt_errno(index->etc->btree) == BT_NOSPC) {
			printf("*** failed to insert size %Ld (vnid 0x%Lx) in size "
					"index (err %d)\n", bi->data.size, bi->etc->vnid, err);
			UNLOCK(index->etc->lock);
			return ENOSPC;
		} else if (bt_errno(index->etc->btree) == BT_DUPRRN)
			printf("*** size %Ld (vnid 0x%Lx) alread in size index (err %d)\n",
				   bi->data.size, bi->etc->vnid, err);
		else 
			bfs_die("*** failed to insert size %Ld (vnid 0x%Lx) in size "
					"index (err %d)\n", bi->data.size, bi->etc->vnid, err);
	}

	call_callbacks(bfs->size_index, bi, NEW_FILE, NULL);

	UNLOCK(index->etc->lock);
	return 0;
}


int
update_name_index(bfs_info *bfs, bfs_inode *bi,
				  const char *oldname, const char *newname)
{
	int      err;
	vnode_id vnid;
	
	if (bfs->name_index == NULL)
		return 0;

	LOCK(bfs->name_index->etc->lock);

	vnid = bi->etc->vnid;
	bfs->name_index->etc->counter++;
	if ((err = bt_delete(bfs->name_index->etc->btree, (bt_chrp)oldname,
						 strlen(oldname), (dr9_off_t *)&vnid)) != BT_OK) {
		if (err != BT_NF) {
			bt_perror(bfs->name_index->etc->btree, "rename");
			printf("error deleting %s from main name index during "
				   "rename\n", oldname);
		}
	}

	call_callbacks(bfs->name_index, bi, DELETE_FILE, NULL);

	change_name_attr(bfs, bi, newname);

	bfs->name_index->etc->counter++;
	if (bt_insert(bfs->name_index->etc->btree, (bt_chrp)newname,
				  strlen(newname), vnid, 1) != BT_OK) {
		printf("error inserting %s into name index during rename\n",
			   oldname);
		bt_perror(bfs->name_index->etc->btree, "rename");
	}

	call_callbacks(bfs->name_index, bi, NEW_FILE, NULL);

	UNLOCK(bfs->name_index->etc->lock);

	return 0;
}


/*
   this function checks if the named attribute is indexed and if so it
   adds it to that index.  this is not the most particularly efficient
   implementation at the moment...
*/
int
update_index_if_needed(bfs_info *bfs, const char *name, int type,
					   const void *value, size_t size, bfs_inode *file)
{
	int        ret;
	bfs_inode *bi;
	double     dbl;
	float      flt;
	vnode_id   vnid = file->etc->vnid;
	
	/* can't/don't want to update these guys. */
	if (strcmp(name, "name") == 0 || strcmp(name, "size") == 0 ||
		strcmp(name, "created") == 0 || strcmp(name, "last_modified") == 0)
		return 0;
	
	if (size >= MAX_INDEXED_DATA_SIZE)
		return E2BIG;

	ret = get_index(bfs, name, &bi);
	if (ret == ENOENT)                /* then there is nothing to do */
		return 0;

	CHECK_INODE(bi);

	LOCK(bi->etc->lock);

	/* check for type mismatches (incoming data type different than tree */
	if ((type == B_STRING_TYPE && bt_dtype(bi->etc->btree) != BT_STRING) ||
		(type == B_MIME_STRING_TYPE && bt_dtype(bi->etc->btree) != BT_STRING) ||
		(type == B_INT32_TYPE && bt_dtype(bi->etc->btree) != BT_INT) ||
		(type == B_INT64_TYPE && bt_dtype(bi->etc->btree) != BT_LONG_LONG) ||
		(type == B_FLOAT_TYPE && bt_dtype(bi->etc->btree) != BT_FLOAT) ||
		(type == B_DOUBLE_TYPE && bt_dtype(bi->etc->btree) != BT_DOUBLE)) {
		UNLOCK(bi->etc->lock);
		put_vnode(bfs->nsid, bi->etc->vnid);
		return EINVAL;
	}

	if (type == B_FLOAT_TYPE) {
		memcpy(&flt, value, sizeof(float));
		value = &flt;
	} else if (type == B_DOUBLE_TYPE) {
		memcpy(&dbl, value, sizeof(double));
		value = &dbl;
	}
		
	/* so far so good.  now lets try and insert this guy into the index */
	bi->etc->counter++;
	if (bt_insert(bi->etc->btree, (bt_chrp)value, size, vnid, 1) != BT_OK) {
		bt_perror(bi->etc->btree, "update_index_if_needed");
		printf("error inserting vnid 0x%Lx into index @ 0x%x\n", vnid, bi);
	}

	call_callbacks(bi, file, NEW_FILE, NULL);

	UNLOCK(bi->etc->lock);
	put_vnode(bfs->nsid, bi->etc->vnid);

	return 0;
}



int
delete_from_index_if_needed(bfs_info *bfs, const char *name, int type,
							const void *value, size_t size, bfs_inode *file)
{
	int        ret, err;
	bfs_inode *bi;
	double     dbl;
	float      flt;
	vnode_id   vnid = file->etc->vnid;
	
	if (bfs->index_index == NULL)
		return 0;

	/* can't/don't want to update these guys. */
	if (strcmp(name, "name") == 0 || strcmp(name, "size") == 0 ||
		strcmp(name, "created") == 0 || strcmp(name, "last_modified") == 0)
		return 0;
	
	if (size >= MAX_INDEXED_DATA_SIZE)
		return E2BIG;

	ret = get_index(bfs, name, &bi);
	if (ret == ENOENT)                /* then there is nothing to do */
		return 0;

	CHECK_INODE(bi);

	LOCK(bi->etc->lock);

	/* check for type mismatches (incoming data type different than tree */
	if ((type == B_STRING_TYPE && bt_dtype(bi->etc->btree) != BT_STRING) ||
		(type == B_MIME_STRING_TYPE && bt_dtype(bi->etc->btree) != BT_STRING) ||
		(type == B_INT32_TYPE && bt_dtype(bi->etc->btree) != BT_INT) ||
		(type == B_INT64_TYPE && bt_dtype(bi->etc->btree) != BT_LONG_LONG) ||
		(type == B_FLOAT_TYPE && bt_dtype(bi->etc->btree) != BT_FLOAT) ||
		(type == B_DOUBLE_TYPE && bt_dtype(bi->etc->btree) != BT_DOUBLE)) {
		UNLOCK(bi->etc->lock);
		put_vnode(bfs->nsid, bi->etc->vnid);
		return EINVAL;
	}

	if (type == B_FLOAT_TYPE) {
		memcpy(&flt, value, sizeof(float));
		value = &flt;
	} else if (type == B_DOUBLE_TYPE) {
		memcpy(&dbl, value, sizeof(double));
		value = &dbl;
	}
		

	/* so far so good.  now lets try and delete this guy from the index */
	bi->etc->counter++;
	if ((err = bt_delete(bi->etc->btree, (bt_chrp)value, size, &vnid) != BT_OK)) {
		if (err != BT_NF) {
			bt_perror(bi->etc->btree, "delete_from_index_if_needed");
			printf("error deleting vnid 0x%Lx from index @ 0x%x\n", vnid, bi);
		}
	}

	call_callbacks(bi, file, DELETE_FILE, NULL);

	UNLOCK(bi->etc->lock);
	put_vnode(bfs->nsid, bi->etc->vnid);

	return 0;
}



int
bfs_create_index(void *ns, const char *name, int type, int flags)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *indices = bfs->index_index;
	bfs_inode *index;

	if (*name == '\0' || indices == NULL)
		return EINVAL;

	if (strcmp(name, "name") == 0 || strcmp(name, "size") == 0 ||
		strcmp(name, "created") == 0 || strcmp(name, "last_modified") == 0)
		return EINVAL;

	/* don't allow system indices to be created by non-root users */
	if ( (strncmp(name, "sys:", 4) == 0) && (geteuid() != 0) )
		return EPERM;
	
	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);

	/* XXXdbg -- what to do with the flags argument? */

	if (type == B_STRING_TYPE || type == B_MIME_STRING_TYPE) {
		if (make_index(bfs, indices, &index, name, S_INDEX_DIR | S_IFDIR) != 0)
			goto err;
	} else if (type == B_INT32_TYPE) {
		if (make_index(bfs, indices, &index, name,
					   S_INDEX_DIR | S_INT_INDEX | S_IFDIR) != 0)
			goto err;
	} else if (type == B_INT64_TYPE) {
		if (make_index(bfs, indices, &index, name,
					   S_INDEX_DIR | S_LONG_LONG_INDEX | S_IFDIR) != 0)
			goto err;
	} else if (type == B_FLOAT_TYPE) {
		if (make_index(bfs, indices, &index, name,
					   S_INDEX_DIR | S_FLOAT_INDEX | S_IFDIR) != 0)
			goto err;
	} else if (type == B_DOUBLE_TYPE) {
		if (make_index(bfs, indices, &index, name,
					   S_INDEX_DIR | S_DOUBLE_INDEX | S_IFDIR) != 0)
			goto err;
	} else {
		/* XXXdbg -- what about unsigned data types? */
		goto err;
	}

	bfs_write_vnode(bfs, index, 0);

	release_sem_etc(bfs->sem, MAX_READERS, 0);
	return 0;

 err:
	release_sem_etc(bfs->sem, MAX_READERS, 0);
	return EINVAL;
}


int
bfs_remove_index(void *ns, const char *name)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *index_index = (bfs_inode *)bfs->index_index, index;
	status_t   err = 0;

	if (index_index == NULL)
		return ENOENT;

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	if (strcmp(name, "name") == 0 || strcmp(name, "size") == 0 ||
		strcmp(name, "created") == 0 || strcmp(name, "last_modified") == 0 ||
		strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EPERM;

	/* don't allow system indices to be deleted by non-root users */
	if ( (strncmp(name, "sys:", 4) == 0) && (geteuid() != 0) )
		return EPERM;
	
	/*
	   LOCKDOWN!  We disallow anyone else from doing anything while
	   deleting an index.
	*/
	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);
	LOCK(index_index->etc->lock);

	if (check_permission(index_index, O_WRONLY) == 0) {
		UNLOCK(index_index->etc->lock);
		release_sem_etc(bfs->sem, MAX_READERS, 0);
		return EACCES;
	}

	/*
	   note: we use do_unlink() here because this is an index and we just
	   want to free the data stream associated with it and we don't care
	   about whether it is empty or not
	*/
	err = do_unlink(bfs, index_index, name, 0, 0);

	/* only call abort or end_transaction if the transaction was started
	   (and that can only happen if err == 0 or EINVAL */
	if (err == EINVAL)
		abort_transaction(bfs, bfs->cur_lh);
	else if (err == 0)
		end_transaction(bfs, bfs->cur_lh);
	
	UNLOCK(index_index->etc->lock);
	release_sem_etc(bfs->sem, MAX_READERS, 0);

	return err;
}



int
bfs_open_indexdir(void *ns, void **cookie)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	if (bfs->index_index == NULL)
		return ENOENT;

	return bfs_opendir(ns, bfs->index_index, cookie);
}

int
bfs_close_indexdir(void *ns, void *cookie)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	return bfs_closedir(ns, bfs->index_index, cookie);
}

int
bfs_free_index_dircookie(void *ns, void *node, void *cookie)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	return bfs_free_dircookie(ns, bfs->index_index, cookie);
}

int
bfs_rewind_indexdir(void *ns, void *cookie)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	return bfs_rewinddir(ns, bfs->index_index, cookie);
}

int
bfs_read_indexdir(void *ns, void *cookie, long *num,
				  struct dirent *buf, size_t bufsize)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	return bfs_readdir(ns, bfs->index_index, cookie, num, buf, bufsize);
}


int
bfs_stat_index(void *ns, const char *name, struct index_info *ii)
{
	int        ret = 0, need_put = 0;
	bfs_info  *bfs = (bfs_info *)ns;
	vnode_id   vnid;
	bfs_inode *dir = bfs->index_index, *bi = NULL;

	memset(ii, 0, sizeof(*ii));

	if (strcmp(name, "name") == 0)
		bi = bfs->name_index;
	else if (strcmp(name, "size") == 0)
		bi = bfs->size_index;
	else if (strcmp(name, "created") == 0)
		bi = bfs->create_time_index;
	else if (strcmp(name, "last_modified") == 0)
		bi = bfs->last_modified_time_index;
	else {
		ret = get_index(bfs, name, &bi);
		need_put = 1;
	}

	if (ret != 0)
		return ret;
	
	if (bi == NULL) {
		printf("bogus dude... bi == NULL for %s\n", name);
		return ENOENT;
	}

	ii->size              = bi->data.size;
	ii->modification_time = (time_t)(bi->last_modified_time >> TIME_SCALE);
	ii->creation_time     = (time_t)(bi->create_time >> TIME_SCALE);
	ii->uid               = bi->uid;
	ii->gid               = bi->gid;
	if (bi->mode & S_STR_INDEX) {
		ii->type = B_STRING_TYPE;
	} else if (bi->mode & S_INT_INDEX) {
		ii->type = B_INT32_TYPE;
	} else if (bi->mode & S_UINT_INDEX) {
		/* XXXdbg what about signed types (for ulong long too) */
	} else if (bi->mode & S_LONG_LONG_INDEX) {
		ii->type = B_INT64_TYPE;
	} else if (bi->mode & S_ULONG_LONG_INDEX) {
	} else if (bi->mode & S_FLOAT_INDEX) {
		ii->type = B_FLOAT_TYPE;
	} else if (bi->mode & S_DOUBLE_INDEX) {
		ii->type = B_DOUBLE_TYPE;
	} else {   
		ii->type = B_STRING_TYPE;   /* force it to be _something_ */
	}
	
	if (need_put)
		put_vnode(bfs->nsid, bi->etc->vnid);
	

	return 0;
}


int
add_index_callback(bfs_inode *index,
				   int      (*func)(int               type_of_event,
									struct bfs_inode *bi,
									int              *old_result,
									void             *arg),
				   void      *arg)
{
	index_callback *ic;

	ic = (index_callback *)malloc(sizeof(*ic));
	if (ic == NULL)
		return ENOMEM;

	ic->func = func;
	ic->arg  = arg;


	LOCK(index->etc->lock);

	ic->next = index->etc->callbacks;
	index->etc->callbacks = ic;

	UNLOCK(index->etc->lock);

	return 0;
}


int
remove_index_callback(bfs_inode *index, void *arg)
{
	int             ret = 0;
	index_callback *ic, *prev_ic = NULL;

	LOCK(index->etc->lock);

	ic = index->etc->callbacks;
	for(; ic && ic->arg != arg; prev_ic=ic, ic=ic->next) {
		/* nothing */
	}

	if (ic != NULL && ic->arg == arg) {
		if (prev_ic == NULL)
			index->etc->callbacks = ic->next;
		else
			prev_ic->next = ic->next;
	} else {
		ret = ENOENT;
	}
		
	UNLOCK(index->etc->lock);
	
	if (ic != NULL && ic->arg == arg)
		free(ic);

	return ret;
}


static void
call_callbacks(bfs_inode *index, bfs_inode *file, int type_of_event, int *res)
{
	index_callback *ic;

	for(ic=index->etc->callbacks; ic; ic=ic->next) {
		ic->func(type_of_event, file, res, ic->arg);
	}
}
