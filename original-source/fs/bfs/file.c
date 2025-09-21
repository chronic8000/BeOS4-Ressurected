#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "bfs.h"
#include "btlib/btree.h"

#define I_WROTE_THIS_FILE  0x01000000  /* a flag for the file cookie */

static int lock_inode_metadata(bfs_info *bfs, bfs_inode *bi);
static int unlock_inode_metadata(bfs_info *bfs, bfs_inode *bi);

int
reserve_space(bfs_info *bfs, dr9_off_t num_blocks)
{
	atomic_add(&bfs->reserved_space, num_blocks);
	if (NUM_FREE_BLOCKS(bfs) < (off_t)bfs->reserved_space) {
		atomic_add(&bfs->reserved_space, -num_blocks);
		return ENOSPC;
	}
	
	return 0;
}

void
unreserve_space(bfs_info *bfs, dr9_off_t num_blocks)
{
	atomic_add(&bfs->reserved_space, -num_blocks);
}



bfs_inode *
make_file(bfs_info *bfs, bfs_inode *dir, uint mode_bits, vnode_id *result)
{
	char        buff[128];
	bfs_inode  *bi = NULL;
	dr9_off_t   bnum;
	inode_addr  ia;

	bi = allocate_inode(bfs, dir, S_IFREG);
	if (bi == 0)
		return NULL;

	bnum = inode_addr_to_vnode_id(bi->inode_num);

	bi->etc = (binode_etc *)calloc(sizeof(binode_etc), 1);
	if (bi->etc == NULL)
		goto cleanup;
	
	sprintf(buff, "isem:%d:%d:%d",
			bi->inode_num.allocation_group,
			bi->inode_num.start,
			bi->inode_num.len);


	if (new_lock(&bi->etc->lock, buff) != 0)
		goto cleanup;

	bi->parent     = dir->inode_num;
	bi->uid        = geteuid();
	bi->gid        = getegid();
	bi->mode      |= (S_IFREG | mode_bits);
	vnode_id_to_inode_addr(bi->attributes, 0);
	bi->etc->btree      = NULL;
	bi->etc->counter    = 0;

#if 0
	if (update_inode(bfs, bi) != 0)
		goto cleanup;
#endif

	bi->etc->old_size  = -1;
	bi->etc->old_mtime = -1;

	*result = inode_addr_to_vnode_id(bi->inode_num);
	bi->etc->vnid = *result;

	return bi;


 cleanup:
	if (bi->etc) {
		if (bi->etc->lock.s > 0)
			free_lock(&bi->etc->lock);
		bi->etc->lock.s = -1;

		free(bi->etc);
		bi->etc = NULL;
	}

	ia = bi->inode_num;
	release_block(bfs->fd, bnum);
	free_inode(bfs, &ia);

	return NULL;
}



int
bfs_create(void *ns, void *_dir, const char *name, int omode,
		   int mode_bits, vnode_id *vnid, void **cookie)
{
	int       *fcookie = NULL, err;
	dr9_off_t  res = BT_ANY_RRN;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir = (bfs_inode *)_dir, *bi = NULL;

	CHECK_INODE(dir);
	if (S_ISDIR(dir->mode) == 0)
		return EINVAL;

	if (check_permission(dir, O_WRONLY) == 0)
		return EACCES;

	mode_bits &= S_IUMSK;   /* make sure no garbage bits are set */
	
	if (name == NULL || *name == '\0' || strchr(name, '/') || 
		strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EINVAL;

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	acquire_sem(bfs->sem);
	LOCK(dir->etc->lock);

	if (dir->flags & INODE_DELETED) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);

		return ENOENT;
	}


	dir->etc->counter++;
	if (bt_find(dir->etc->btree, (bt_chrp)name, strlen(name), &res) == BT_OK) {
		int ret = 0;
	    if (res == 0) 
	    	bfs_die("create: vnid == 0 (dir 0x%x, name %s)\n", dir, name);


		/*
		   if we get here, the file already exists so check that what 
		   we're doing is kosher
		*/
		if (get_vnode(bfs->nsid, res, (void **)&bi) != 0) {
			printf("can't get vnode for vnid 0x%lx\n", (ulong)res);
			UNLOCK(dir->etc->lock);
			release_sem(bfs->sem);
			return ENOENT;
		}

		if ((bi->flags & INODE_NO_CACHE) || check_permission(bi, omode) == 0) {
			/* bad boy! don't have permission to do this */
			ret = EACCES;
		}

		if (ret == 0) {
			/* exclusive create fails because it already exists */
			if (omode & O_EXCL) {
				ret = EEXIST;
			} else {
				/* else, it's a normal open */
				fcookie = (int *)malloc(sizeof(int));
				if (fcookie)
					*fcookie = (omode & 0xffff);
				*cookie = (void *) fcookie;
				
				*vnid = res;
			}
		}

		if (ret == 0 && S_ISDIR(bi->mode)) {
			/* can't create something that's a directory */
			ret = EISDIR;
   		}


		if (ret == 0 && (omode & O_TRUNC) && bi->data.size != 0) {
			int err;
			
			LOCK(bi->etc->lock);
			
			bfs->cur_lh = start_transaction(bfs);
			if (bfs->cur_lh == NULL) {
				printf("create: can't start transaction (dir @ 0x%x)!\n", bi);
				ret = ENOMEM;
			} else {
				err = set_file_size(bfs, bi, 0);
				if (err == 0)
					end_transaction(bfs, bfs->cur_lh);
				else
					abort_transaction(bfs, bfs->cur_lh);
			}

			UNLOCK(bi->etc->lock);
		}

		if (ret != 0) {
			if (fcookie)
				free(fcookie);
			put_vnode(bfs->nsid, bi->etc->vnid);
		}

		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return ret;
	}

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		printf("create:2: can't start transaction!\n");
		return ENOMEM;
	}

	/* make sure there's enough room for all bt_insert()'s to work */
	if (reserve_space(bfs, NUM_INSERT_BLOCKS) != 0) {
		abort_transaction(bfs, bfs->cur_lh);
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return ENOSPC;
	}
	

	fcookie = (int *)malloc(sizeof(int));
	if (fcookie)
		*fcookie = (omode & 0xffff);
	*cookie = (void *) fcookie;
				
	if ((bi = make_file(bfs, dir, mode_bits, vnid)) == NULL) {
		goto cleanup;
	}
			
	if (create_name_attr(bfs, bi, name) != 0) {
		printf("bfs: create: bi @ 0x%x can't add name attribte %s?\n",bi,name);
		goto cleanup;
	}

	/* printf("created: %6d %s\n", bi->etc->vnid, name); */
	dir->etc->counter++;
	if (bt_insert(dir->etc->btree, (bt_chrp)name, strlen(name), *vnid, 0) != BT_OK){
		bt_perror(dir->etc->btree, "create");
		printf("*** failed to insert file %s (vnid 0x%Lx freeblk %Ld)\n", 
               name, *vnid, bfs->dsb.num_blocks - bfs->dsb.used_blocks);
		/* here we still force the call to bt_delete() just to be safe */
		goto file_cleanup;
	}

	dir->last_modified_time = CUR_TIME;
	update_inode(bfs, dir);

	if (add_to_main_indices(bfs, name, bi) != 0) {
		printf("add_to_main_indices failed\n");
		goto file_cleanup;
	}
			
	/* it's time to get soiled honey. you're no longer a virgin */
	bi->flags &= ~INODE_VIRGIN;
	update_inode(bfs, bi);

	if ((err = new_vnode(bfs->nsid, *vnid, bi)) != 0)
		bfs_die("new_vnode failed for vnid %Ld: %s\n", *vnid, strerror(err));

	unreserve_space(bfs, NUM_INSERT_BLOCKS);
	end_transaction(bfs, bfs->cur_lh);

	notify_listener(B_ENTRY_CREATED, bfs->nsid, dir->etc->vnid, 0, *vnid, name); 

	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);
	return 0;


 file_cleanup:
	dir->etc->counter++;
	bt_delete(dir->etc->btree, (bt_chrp)name, strlen(name), (dr9_off_t *)vnid);

 cleanup1:
	if (bi) {
		block_run inum = bi->inode_num;
		
		free_data_stream(bfs, bi);
		free_lock(&bi->etc->lock);
		free(bi->etc);
		bi->etc = NULL;
		bi->flags |=  INODE_DELETED;  /* so no one will try to load it */
		bi->flags &= ~INODE_VIRGIN;   /* break out pending read_vnode() calls */

		release_block(bfs->fd, *vnid);
		free_inode(bfs, &inum);
	}
	
 cleanup:
	if (fcookie)
		free(fcookie);

	unreserve_space(bfs, NUM_INSERT_BLOCKS);
	abort_transaction(bfs, bfs->cur_lh);

	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);
	return ENOENT;
}



int
bfs_symlink(void *ns, void *_dir, const char *name, const char *path)
{
	int        path_len, mode_bits = 0777;
	status_t   err = 0;
	vnode_id   vnid;
	dr9_off_t      res = BT_ANY_RRN;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir = (bfs_inode *)_dir, *bi = NULL;

	CHECK_INODE(dir);
	if (S_ISDIR(dir->mode) == 0)
		return EINVAL;

	if (check_permission(dir, O_WRONLY) == 0)
		return EACCES;

	mode_bits &= S_IUMSK;   /* make sure no garbage bits are set */
	
	if (name == NULL || *name == '\0' || strchr(name, '/') || 
		strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EINVAL;

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	acquire_sem(bfs->sem);
	LOCK(dir->etc->lock);

	if (dir->flags & INODE_DELETED) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);

		return ENOENT;
	}


	dir->etc->counter++;
	if (bt_find(dir->etc->btree, (bt_chrp)name, strlen(name), &res) == BT_OK) {
	    if (res == 0) 
	    	bfs_die("create: vnid == 0 (dir 0x%x, name %s)\n", dir, name);

		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return EEXIST;
	}	


	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		printf("create:2: can't start transaction!\n");
		return ENOMEM;
	}

	/* make sure there's enough room for all bt_insert()'s to work */
	if (reserve_space(bfs, NUM_INSERT_BLOCKS) != 0) {
		abort_transaction(bfs, bfs->cur_lh);
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return ENOSPC;
	}

	if ((bi = make_file(bfs, dir, mode_bits, &vnid)) == NULL) {
		printf("make file failed\n");
		goto cleanup;
	}
			
	if (create_name_attr(bfs, bi, name) != 0) {
		printf("bfs symlink: bi @ 0x%x can't add name attribte %s?\n",bi,name);
		goto cleanup;
	}

	bi->mode &= ~S_IFREG;
	bi->mode |= S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;

	/* printf("created: %6d %s\n", bi->etc->vnid, name); */
	dir->etc->counter++;
	if (bt_insert(dir->etc->btree, (bt_chrp)name, strlen(name), vnid, 0) != BT_OK){
		bt_perror(dir->etc->btree, "create");
		printf("*** failed to insert symlink %s (vnid 0x%Lx freeblk %Ld)\n", 
               name, vnid, bfs->dsb.num_blocks - bfs->dsb.used_blocks);
		/* here we still force the call to bt_delete() just to be safe */
		goto file_cleanup;
	}


	/* short symlinks are stored in the i-node directly.  longer symlinks
	   get written to the data stream of the file (and their data is logged
	   because it's file system meta-data).
    */   
	path_len = strlen(path);
	if (path_len+1 <= sizeof(data_stream)) {
		strcpy((char *)&bi->data, path);
	} else {
		size_t len = path_len + 1;
		bi->flags |= (LONG_SYMLINK | INODE_LOGGED);
		err = write_data_stream(bfs, bi, 0, path, &len);
		if (err != 0)
			goto file_cleanup;
	}


	dir->last_modified_time = CUR_TIME;
	update_inode(bfs, dir);

	bi->flags &= ~INODE_VIRGIN;    /* clear it */
	update_inode(bfs, bi);         /* write it */
	bi->flags |=  INODE_VIRGIN;    /* set it again */

	if (add_to_main_indices(bfs, name, bi) != 0) {
		printf("symlink: add_to_main_indices failed\n");
		goto file_cleanup;
	}

	/*
	  Here we clear the VIRGIN bit, then call new_vnode() and then call
	  put_vnode() so that we don't have to call bfs_write_vnode() directly.  
	  This avoids any untoward situations where bfs_write_vnode() might do 
	  things that would continue this transaction and cause the i-node to
	  be written to the log again with the virgin bit set.
    */
	bi->flags &= ~INODE_VIRGIN;    /* clear it (again) */

	if ((err = new_vnode(bfs->nsid, vnid, bi)) != 0)
		bfs_die("new_vnode failed for vnid %Ld: %s\n", vnid, strerror(err));

	put_vnode(bfs->nsid, bi->etc->vnid);
	
	unreserve_space(bfs, NUM_INSERT_BLOCKS);
	end_transaction(bfs, bfs->cur_lh);


	notify_listener(B_ENTRY_CREATED, bfs->nsid, dir->etc->vnid, 0, vnid, name);

	
	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);
	return 0;

 file_cleanup:
	dir->etc->counter++;
	bt_delete(dir->etc->btree, (bt_chrp)name, strlen(name),(dr9_off_t *)&vnid);

 cleanup:
	if (bi) {
		block_run inum = bi->inode_num;
		
		free_lock(&bi->etc->lock);
		free(bi->etc);
		bi->etc = NULL;
		bi->flags |=  INODE_DELETED;  /* so no one will try to load it */
		bi->flags &= ~INODE_VIRGIN;   /* break out pending read_vnode() calls */

		release_block(bfs->fd, vnid);
		free_inode(bfs, &inum);
	}

	unreserve_space(bfs, NUM_INSERT_BLOCKS);
	abort_transaction(bfs, bfs->cur_lh);

	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);
	return ENOENT;
}



int
bfs_readlink(void *ns, void *node, char *buf, size_t *bufsize)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	int		   err;
	size_t	   l;


	LOCK(bi->etc->lock);

	if (!S_ISLNK(bi->mode)) {
		err = EINVAL;
		UNLOCK(bi->etc->lock);
		return err;
	}

	if (bi->flags & LONG_SYMLINK) {
		err = read_data_stream(bfs, bi, 0, buf, bufsize);
	} else {
		l = strlen((char *)&bi->data);
		if (l > *bufsize)
			memcpy(buf, &bi->data, *bufsize);
		else
			memcpy(buf, &bi->data, l);

		*bufsize = l;
		err = 0;
	}
	

	UNLOCK(bi->etc->lock);
	
	return err;
}


/* only used by bfs_free_cookie */
static void
update_indices_if_needed(bfs_info *bfs, bfs_inode *bi, int cookie)
{
	if (bfs->flags & FS_READ_ONLY)  /* read-only fs == nothing to do */
		return;

	if (S_ISDIR(bi->mode))          /* don't keep size/time indices for dirs */
		return;

	/* if it's a symlink (and not a long symlink), then bail out early */
	if (S_ISLNK(bi->mode) && ((bi->flags & LONG_SYMLINK) == 0))
		return;
	
	if (bi->flags & INODE_DELETED)  /* and we don't care about dead files */
		return;
	

	if ((cookie & I_WROTE_THIS_FILE) ||
		(bi->data.size == 0 && bi->etc->old_size < 0)) {

		bfs->cur_lh = start_transaction(bfs);
		while (bfs->cur_lh == NULL) {
			printf("bfs_close: can't start transaction, waiting for mem\n");
			snooze(10000);
			bfs->cur_lh = start_transaction(bfs);
		}

		if ((bi->flags & INODE_WAS_WRITTEN) || 
            bi->last_modified_time == bi->create_time) {

			bi->flags &= ~INODE_WAS_WRITTEN;

			update_last_modified_time_index(bfs, bi);

			/* free up any extra pre-allocated blocks (but only for files) */
			if (S_ISDIR(bi->mode) == 0 && bi->data.size > 0)
				trim_preallocated_blocks(bfs, bi);
		}

		if (bi->data.size != bi->etc->old_size) {
			update_size_index(bfs, bi);
		}
			
		update_inode(bfs, bi);

		/* notify any listener of the changes. */
		notify_listener(B_STAT_CHANGED, bfs->nsid, 0, 0, bi->etc->vnid, NULL);

		end_transaction(bfs, bfs->cur_lh);
	}
}



int
bfs_free_cookie(void *ns, void *file, void *cookie)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi = (bfs_inode *)file;

	CHECK_INODE(bi);

	if (bi->flags & LOCKED_FS) {   /* have to do special stuff... */
		LOCK(bi->etc->lock);
		release_sem(bfs->log_sem);
		UNLOCK(bi->etc->lock);
		bi->flags &= ~LOCKED_FS;
		release_sem_etc(bfs->sem, MAX_READERS, 0);

	}

	acquire_sem(bfs->sem);
	LOCK(bi->etc->lock);

	update_indices_if_needed(bfs, bi, *(int *)cookie);

	if (bi->flags & INODE_NO_CACHE) {
		unlock_inode_metadata(bfs, bi);
		shutdown_hash_table(bi->etc->ht);
		free(bi->etc->ht);
		bi->etc->ht = NULL;
		bi->flags &= ~INODE_NO_CACHE;
	}

	UNLOCK(bi->etc->lock);
	release_sem(bfs->sem);

	if(cookie)
		free(cookie);
	
	return 0;
}


int
bfs_read_vnode(void *ns, vnode_id vnid, char r, void **file)
{
	int          i, count = 0;
	char         buff[B_OS_NAME_LENGTH];
	dr9_off_t        bnum, tmp_bnum;
	bfs_info    *bfs = (bfs_info *)ns;
	bfs_inode   *bi;
	inode_addr   ia;

    if (vnid <= 0 || vnid >= bfs->dsb.num_blocks) {
		printf("read_vnode: uuuhhh dude, like vnid %Ld really sucks. (r %d)\n",
			   vnid, r);
		return ENOENT;
    }

	vnode_id_to_inode_addr(ia, vnid);
	ia.len = bfs->dsb.inode_size / bfs->dsb.block_size;

	bnum    = block_run_to_bnum(ia);
	if ((bfs->dsb.inode_size / bfs->dsb.block_size) != 1) {
		printf("inode_size %d and block size %d are not equal\n",
			   bfs->dsb.inode_size, bfs->dsb.block_size);
		return ENOENT;
	}

	/* this gets the block from disk and locks it in the cache */
restart:
	bi = get_block(bfs->fd, bnum, bfs->dsb.block_size);
	if (bi == NULL)
		return ENOMEM;

	if (bi->magic1 != INODE_MAGIC1) {
		printf("rvn: %d: inode @ 0x%lx (%Ld) has bad magic 0x%x\n",
			   getpid(), (ulong)bi, vnid, bi->magic1);
		/* dump_inode(bi); */
		goto bail;
	}
	
	if (memcmp(&bi->inode_num, &ia, sizeof(inode_addr)) != 0) {
		printf("inode %d %d %d is corrupt (bad inode num %d %d %d)\n",
			   ia.allocation_group, ia.start, ia.len,
			   bi->inode_num.allocation_group, bi->inode_num.start,
			   bi->inode_num.len);
		goto bail;
	}

	if (bi->flags & INODE_VIRGIN) {
		if (count++ > 200) {
			bfs_die("bfs: %d: time-out while trying to soil the virgin i-node %Ld (count %d)\n",
				   getpid(), vnid, count);
			goto bail;
		}
		release_block(bfs->fd, bnum);
		bi = NULL;
		snooze(2000);
		goto restart;	
	}

	if (bi->flags & INODE_DELETED) {
		printf("bfs: thid %d trying to read deleted i-node %Ld\n", getpid(),
			   vnid);
		goto bail;
	}

	if ((bi->flags & LONG_SYMLINK) || S_ISLNK(bi->mode) == 0) {
		if (bi->data.size < 0 || bi->data.max_double_indirect_range < 0 ||
			bi->data.max_indirect_range < 0 || bi->data.max_direct_range < 0) {
			printf("*** bad file size(s) for bi 0x%x\n", bi);
			goto bail;
		}


		if (bi->data.max_double_indirect_range &&
			bi->data.size > bi->data.max_double_indirect_range) {
			printf("bi 0x%x: size > max dbl ind suckage\n", bi);
			goto bail;
		} else if (bi->data.max_double_indirect_range == 0 &&
				   bi->data.max_indirect_range &&
				   bi->data.size > bi->data.max_indirect_range) {
			printf("bi 0x%x: size > max ind crappage\n", bi);
			goto bail;
		} else if (bi->data.max_double_indirect_range == 0 &&
				   bi->data.max_indirect_range == 0 &&
				   bi->data.max_direct_range &&
				   bi->data.size > bi->data.max_direct_range) {
			printf("bi 0x%x: size > direct really blows\n", bi);
			goto bail;
		}


		tmp_bnum = block_run_to_bnum(bi->data.indirect);
		if (tmp_bnum < 0 || tmp_bnum >= bfs->dsb.num_blocks) {
			printf("bi 0x%lx: dude, like indirect bnum %Ld sucks.\n",bi,tmp_bnum);
			goto bail;
		}

		tmp_bnum = block_run_to_bnum(bi->data.double_indirect);
		if (tmp_bnum < 0 || tmp_bnum >= bfs->dsb.num_blocks) {
			printf("bi x%lx: dbl indirect bnum %Ld blows.\n", bi, tmp_bnum);
			goto bail;
		}

		for(i=0; i < NUM_DIRECT_BLOCKS; i++) {
			tmp_bnum = block_run_to_bnum(bi->data.direct[i]);
			if (tmp_bnum < 0 || tmp_bnum >= bfs->dsb.num_blocks) {
				printf("bi 0x%lx: direct brun %d bites the big one.\n", bi, i);
				goto bail;
			}
			if (tmp_bnum == 0)
				break;
		}
	}

	tmp_bnum = block_run_to_bnum(bi->parent);
 	if (tmp_bnum == 0 && bnum != block_run_to_bnum(bfs->dsb.root_dir)) {
		printf("read_vnode: warning: parent == 0, bi @ 0x%x, inode # %Ld\n",
			   bi, block_run_to_bnum(bi->inode_num));
	}

	if (tmp_bnum < 0 || tmp_bnum > bfs->dsb.num_blocks) {
		printf("bi 0x%lx: must be a bastard child because parent %Ld is bad\n", 
              bi, tmp_bnum);
		goto bail;
	}

	tmp_bnum = block_run_to_bnum(bi->attributes);
	if (tmp_bnum < 0 || tmp_bnum >= bfs->dsb.num_blocks) {
		printf("bi @ 0x%lx: the attribute dir at %Ld reeks of putrefaction.\n",
               bi, tmp_bnum);
		goto bail;
	}
	
	if (bi->inode_size != bfs->dsb.inode_size) {
		printf("*** load: bad size, inode %d %d %d is potentially corrupt\n",
			   ia.allocation_group, ia.start, ia.len);
		bi->inode_size = bfs->dsb.inode_size;
	}


#if 0
	if (check_block_run(bfs, &ia) == 0)   /* XXXdbg definitely remove! */
		bfs_die("read_vnode: inode %d:%d:%d is not allocated in bitmap!\n",
				ia.allocation_group, ia.start, ia.len);
#endif

	bi->etc = (binode_etc *)calloc(sizeof(binode_etc), 1);
	if (bi->etc == NULL) {
		release_block(bfs->fd, bnum);
		return ENOMEM;
	}

	if (S_ISLNK(bi->mode) == 0)
		bi->etc->old_size = bi->data.size;
	bi->etc->old_mtime = bi->last_modified_time;

	bi->etc->vnid = inode_addr_to_vnode_id(bi->inode_num);

	if (S_ISDIR(bi->mode)) {
		bi->etc->btree = bt_optopen(BT_INODE,    bi,
							   BT_BFS_INFO, bfs,
							   BT_CFLAG,    BT_NOCACHE,
							   BT_CACHE,    0,
							   BT_PSIZE,    1024,
							   0);

		if (bi->etc->btree == NULL) {
			free(bi->etc);
			bi->etc = NULL;
			release_block(bfs->fd, bnum);
			printf("read_vnode: (bi @ 0x%x %Ld) btree == NULL??!? (thid %d)\n",
				   bi, vnid, getpid());
			return ENOMEM;
		}
	} else {
		bi->etc->btree = NULL;
	}

	bi->etc->counter = 0;

	bi->flags &= PERMANENT_FLAGS;   /* only allow permanent flags in */

	sprintf(buff, "isem:%d:%d:%d", ia.allocation_group, ia.start, ia.len);

	if (new_lock(&bi->etc->lock, buff) != 0) {
		if (bi->etc->btree)
			bt_close(bi->etc->btree);
		free(bi->etc);
		bi->etc = NULL;
		release_block(bfs->fd, bnum);
		return ENOMEM;
	}
 
	bi->etc->vnid = vnid;
	
	*file = (void *)bi;
	return 0;

bail:
	release_block(bfs->fd, bnum);
	return EINVAL;
}


int
bfs_write_vnode(void *ns, void *node, char r)
{
	dr9_off_t      bnum;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	
	if (node == NULL) {
		printf("** warning: write_vnode passed null data!\n");
		return 0;
	}

	CHECK_INODE(bi);

	
	bnum = block_run_to_bnum(bi->inode_num);
	if (bnum == 0) {
		printf("write_vnode: bi 0x%x has a zero inode_num?!?\n", bi);
	}


	if (block_run_to_bnum(bi->parent) == 0) {
		if (bi != bfs->root_dir)
			printf("write_vnode: bi @ 0x%x has a zero parent inode num\n",bi);
	}

	/* printf("WVN: bi @ 0x%x %d:%d\n", bi, bnum, bi->inode_num.len); */

	/* if there's any directory schtuff hanging around, close it out */
	if (bi->etc->btree) {
		bt_close(bi->etc->btree);
		bi->etc->btree = NULL;
	}


	/* 
	   We can't trim pre-allocated blocks on indices
	   because it can cause a deadlock where an index is
	   being flushed and wants to start a transaction but
	   someone else who is in remove_vnode (and already
	   has a transaction) wants to delete themselves from
	   the index being flushed.  it's obscure but it does
	   happen.

	   not trimming pre-allocated blocks on indices means
	   that they should grow more nicely over time (i.e. 
	   they will be more contiguous).

	   note that we also trim pre-allocated blocks on
	   long symlinks (otherwise they'll consume 64k of
	   disk space which is a tad bit wasteful).
	*/
	if ((S_ISDIR(bi->mode) && S_ISINDEX(bi->mode) == 0) ||
		(S_ISLNK(bi->mode) && (bi->flags & LONG_SYMLINK))) {

		trim_preallocated_blocks(bfs, bi);
	}

	if (bi->etc->attrs)
		bfs_write_vnode(bfs, bi->etc->attrs, 1);

	if (bi->etc->lock.s > 0) {
		free_lock(&bi->etc->lock);
	}

	free(bi->etc);
	bi->etc = NULL;

	/* 
	   have to do this here for symlinks.  this allows any pending
       read_vnode() calls to complete 
    */
	if (bi->flags & INODE_VIRGIN)
		bi->flags &= ~INODE_VIRGIN;

	release_block(bfs->fd, bnum);
	
	return 0;
}



int
check_permission(bfs_inode *bi, int omode)
{
	int     access_type, mask;
	mode_t  mode = bi->mode;
	uid_t   uid;
	gid_t   gid;

	uid = geteuid();
	gid = getegid();

	if (uid == 0)
		return 1;

	mask = (omode & O_ACCMODE);

	if (mask == O_RDONLY)
		access_type = S_IROTH;
	else if (mask == O_WRONLY)
		access_type = S_IWOTH;
	else if (mask == O_RDWR)
		access_type = (S_IROTH | S_IWOTH);
	else {
		printf("strange looking omode 0x%x\n", omode);
		access_type = S_IROTH;
	}

    if (uid == bi->uid)
        mode >>= 6;
    else if (gid == bi->gid)
        mode >>= 3;

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}

int
check_access(bfs_inode *bi, int access_type)
{
	mode_t  mode = bi->mode;
	uid_t   uid;
	gid_t   gid;

	/* access() uses the real and not effective IDs, so sayeth Stevens, 4.7 */
	uid = getuid();
	gid = getgid();

	if (uid == 0)
		return 1;

    if (uid == bi->uid)
        mode >>= 6;
    else if (gid == bi->gid)
        mode >>= 3;

    if (((mode & access_type & S_IRWXO) == access_type))
        return 1;

	return 0;
}

int
bfs_open(void *ns, void *file, int omode, void **cookie)
{
	int         *fcookie, ret = 0, err = 0;
	bfs_info    *bfs = (bfs_info *)ns;
	bfs_inode   *bi  = (bfs_inode *)file;

	CHECK_INODE(bi);

	if (check_permission(bi, omode) == 0)
		return EACCES;
	

	if (bi->flags & INODE_NO_CACHE)
		return EACCES;


	fcookie = (int *)malloc(sizeof(int));
	if (fcookie)
		*fcookie = (omode & 0xffff);
	*cookie = (void *) fcookie;

	if ((omode & O_TRUNC) && bi->data.size != 0 && !S_ISDIR(bi->mode)) {
		int err;
			
		LOCK(bi->etc->lock);
			
		bfs->cur_lh = start_transaction(bfs);
		if (bfs->cur_lh == NULL) {
			printf("create: can't start transaction (dir @ 0x%x)!\n", bi);
			ret = ENOMEM;
		} else {
			err = set_file_size(bfs, bi, 0);
			if (err == 0)
				end_transaction(bfs, bfs->cur_lh);
			else
				abort_transaction(bfs, bfs->cur_lh);
		}

		UNLOCK(bi->etc->lock);
	}

	return ret;
}


int
bfs_access(void *ns, void *file, int omode)
{
	bfs_info    *bfs = (bfs_info *)ns;
	bfs_inode   *bi  = (bfs_inode *)file;

	CHECK_INODE(bi);
	
	if (check_access(bi, omode) == 0)
		return EACCES;
	
	return 0;
}


int
bfs_close(void *ns, void *file, void *cookie)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi = (bfs_inode *)file;
	
	CHECK_INODE(bi);

	if (S_ISLNK(bi->mode))
		return EINVAL;

	return 0;
}


int
bfs_read(void *ns, void *node, void *cookie, dr9_off_t pos, void *buf, size_t *len)
{
	ssize_t    res;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;


	CHECK_INODE(bi);

	if (S_ISDIR(bi->mode)) {
		*len = 0;
		return EISDIR;
	}

	if (S_ISLNK(bi->mode)) {
		*len = 0;
		return EINVAL;
	}

	
	LOCK(bi->etc->lock);

	res = read_data_stream(bfs, bi, pos, buf, len);

	UNLOCK(bi->etc->lock);

	return res;
}


int
bfs_write(void *ns, void *node, void *cookie, dr9_off_t pos, const void *buf, size_t *len)
{
	int       *fcookie = (int *)cookie;
	ssize_t    res;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;

	CHECK_INODE(bi);

	if (bfs->flags & FS_READ_ONLY) {
		*len = 0;
		return EROFS;
	}

	if (S_ISDIR(bi->mode)) {
		*len = 0;
		return EISDIR;
	}

	if (S_ISLNK(bi->mode)) {
		*len = 0;
		return EINVAL;
	}

	LOCK(bi->etc->lock);

	if (*fcookie & O_APPEND)
		pos = bi->data.size;

	*fcookie |= I_WROTE_THIS_FILE;

	res = write_data_stream(bfs, bi, pos, buf, len);
	bi->last_modified_time = CUR_TIME;

	UNLOCK(bi->etc->lock);

	return res;
}


#ifdef DEBUG
void
printk(BT_INDEX *b, uchar *buf, int rl, off_t rv)
{
	uchar *cp;

	switch(bt_dtype(b)) {
		case	BT_STRING:
			buf[rl] = '\0';
			cp = buf;
			(void)printf("%10Ld - ",rv);
			while(*cp != '\0') {
				if(isprint(*cp))
					(void)printf("%c",*cp);
				else
					(void)printf("(0x%x)",*cp);
				cp++;
			}
			break;

		case	BT_INT:
			(void)printf("%10Ld - %d", rv, *(int *)buf);
			break;

		case	BT_UINT:
			(void)printf("%10Ld - %u", rv, *(uint *)buf);
			break;

		case	BT_LONG_LONG:
			(void)printf("%10Ld - %Ld", rv, *(long long *)buf);
			break;

		case	BT_ULONG_LONG:
			(void)printf("%10Ld - %Lu", rv, *(unsigned long long *)buf);
			break;

		case	BT_FLOAT:
			(void)printf("%10Ld - %f", rv, *(float *)buf);
			break;

		case	BT_DOUBLE:
			(void)printf("%10Ld - %f", rv, *(double *)buf);
			break;
	}
	(void)printf("\n");
}


int
dump_btree(void *ptr)
{
	uchar      buf[1024];
	int        ret;
	int        retlen;
	off_t      junk;
	BT_INDEX  *b;
	bfs_inode *bi;


	b = (BT_INDEX *)ptr;

	if ((ulong)b < 0x3100) {
		printf("address 0x%lx is suspicious... bailing out\n", b);
		return 0;
	}

	bi = (bfs_inode *)b;
	if (bi->magic1 == INODE_MAGIC1) {
		if (bi->etc && bi->etc->btree)
			b = bi->etc->btree;
		else {
			printf("you gave a bfs_inode pointer but there is no btree!\n");
			return 0;
		}
	} else if (b->sblk.magic != 0x69f6c2e8) {
		printf("btree @ 0x%x is not a btree!\n", b);
		return 0;
	}

	dump_btree_header(b);


	if(bt_goto(b, BT_BOF) == BT_ERR) {
		bt_perror(b, "cannot goto BOF");
		return 0;
	}

	while((ret = bt_traverse(b,BT_EOF,buf,sizeof(buf),&retlen,&junk))==BT_OK) {
		printk(b, buf, retlen, junk);
	}

	if(ret == BT_ERR)
		bt_perror(b,"error traversing!");

	return 0;
}
#endif


static int
lock_inode_metadata(bfs_info *bfs, bfs_inode *bi)
{
	int         i, j;
	dr9_off_t   bnum, bnum2;
	void       *ptr;
	my_hash_table *ht = bi->etc->ht;
					 
	bnum = block_run_to_bnum(bi->data.indirect);
	if (bnum) {     /* then lock this in the cache */
		for(i=0; i < bi->data.indirect.len; i++) {
			ptr = get_block(bfs->fd, bnum + i, bfs->dsb.block_size);
			hash_insert(ht, bnum + i, ptr);
		}
	}
	
	bnum = block_run_to_bnum(bi->data.double_indirect);
	if (bnum) {          /* then lock this in the cache too */
		int        k;
		block_run *br;

		for(i=0; i < bi->data.double_indirect.len; i++) {
			br = get_block(bfs->fd, bnum + i, bfs->dsb.block_size);
			hash_insert(ht, bnum + i, br);
			
			for(j=0; j < bfs->dsb.block_size/sizeof(block_run); j++) {
				bnum2 = block_run_to_bnum(br[j]);
				if (bnum2 == 0)
					break;
				
				for(k=0; k < br[j].len; k++) {
					ptr = get_block(bfs->fd, bnum2 + k, bfs->dsb.block_size);
					hash_insert(ht, bnum2 + k, ptr);
				}
			}
		}
	}

	return 0;   /* no problems */
}



static int
unlock_inode_metadata(bfs_info *bfs, bfs_inode *bi)
{
	int        i, j;
	dr9_off_t  bnum, bnum2;
					 
	bnum = block_run_to_bnum(bi->data.indirect);
	if (bnum) {     /* then lock this in the cache */
		for(i=0; i < bi->data.indirect.len; i++)
			release_block(bfs->fd, bnum + i);
	}
	
	bnum = block_run_to_bnum(bi->data.double_indirect);
	if (bnum) {          /* then lock this in the cache too */
		int        k;
		block_run *br;

		for(i=0; i < bi->data.double_indirect.len; i++) {
			br = get_block(bfs->fd, bnum + i,
						   bfs->dsb.block_size);
			
			for(j=0; j < bfs->dsb.block_size/sizeof(block_run); j++) {
				bnum2 = block_run_to_bnum(br[j]);
				if (bnum2 == 0)
					break;
				
				for(k=0; k < br[j].len; k++)
					release_block(bfs->fd, bnum2 + k);
			}

			/* have to do it twice: once to match the get_block() above
			   and once to match the get_block() in lock_inode_metadata()
		    */   
			release_block(bfs->fd, bnum+i);
			release_block(bfs->fd, bnum+i);
		}
	}

	return 0;   /* no problems */
}


int
bfs_ioctl(void *ns, void *node, void *cookie, int code, void *buf, size_t len)
{
	int        err = 0;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;

	LOCK(bi->etc->lock);

	switch (code) {
		/* XXXdbg -- give this number a name and put it in a header file */
	case 10000 : /* make the file cache inhibited if it is a regular file */
		         if (S_ISDIR(bi->mode) || S_ISLNK(bi->mode)) {
					 err = EINVAL;
				 } else {
					 printf("setting vnid 0x%Lx (bi@ 0x%lx) to be UN-CACHED\n",
							bi->etc->vnid, (ulong)bi);
					 bi->flags |= INODE_NO_CACHE;

					 bi->etc->tmp_block = (char *)malloc(bfs->dsb.block_size);
					 if (bi->etc->tmp_block == NULL) {
						 err = ENOMEM;
						 break;
					 }
	
					 bi->etc->ht = calloc(sizeof(my_hash_table), 1);
					 if (bi->etc->ht == NULL) {
						 err = ENOMEM;
						 break;
					 }

					 init_hash_table(bi->etc->ht);
					 lock_inode_metadata(bfs, bi);
				 }
				 break;

	case 10001 : { /* get the physical block location for a file position */
		         	dr9_off_t *bnum = (dr9_off_t *)buf;

					bnum_for_pos(bfs, bi, bnum);
					break;
		         }

	case 10002 : { /* get the real creation time for a file */
		         	bigtime_t *tm = (bigtime_t *)buf;

					if (tm)
                    	*tm = bi->create_time;
					break;
		         }

	case 10003 : { /* get the real last modification time for a file */
		         	bigtime_t *tm = (bigtime_t *)buf;

					if (tm)
                    	*tm = bi->last_modified_time;
					break;
		         }

	case 10004 : { /* write new boot sector code */
					struct {
						uint32 magic;
						uint32 bufflen;
						uchar buff[1];
					} *params = buf;
					system_info info;

 					err = EINVAL;
					if ((bfs->original_bootblock) &&
							/* only allow on x86 */
							(get_system_info(&info) == B_OK) &&
							(info.platform_type == B_AT_CLONE_PLATFORM) &&
							/* rudimentary sanity checks */
							(params->magic == 'yzzO') &&
							(params->bufflen <= bfs->dsb.block_size)) {
						UNLOCK(bi->etc->lock);
						acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);
						memcpy(bfs->original_bootblock, params->buff, params->bufflen);
						err = write_super_block(bfs);
						release_sem_etc(bfs->sem, MAX_READERS, 0);
						LOCK(bi->etc->lock);
					}
					break;
				 }

	case 10005 : {  /*
					   lock the file system for dangerous operations...
					   note that we unlock the i-node because we want
					   to use the same locking order down below that
					   we use elsewhere in the fs.  this isn't a big
					   deal since we're not really modifying the i-node
					   in question here.
					*/
					UNLOCK(bi->etc->lock);

					sync_log(bfs);

					/*
					   yeah, there's a window here where another transaction
					   could start before we lock-down the fs... let's hope
					   people aren't running fs tests while doing this...
					*/

					acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);
					LOCK(bi->etc->lock);
					acquire_sem(bfs->log_sem);

					bi->flags |= LOCKED_FS;
					break;
				 }

	case 10006 : {  /* un-lock the file system (in the same order as above) */
					if (bi->flags & LOCKED_FS) {
						release_sem(bfs->log_sem);
						UNLOCK(bi->etc->lock);

						bi->flags &= ~LOCKED_FS;
						release_sem_etc(bfs->sem, MAX_READERS, 0);

						/* have to re-lock this because it gets unlocked below */
						LOCK(bi->etc->lock);
					} else {
						printf("Hah! You tried to trick me.\n");
					}

					break;
				 }

	case 10007 : {  /*
					  replace the bitmap for the running bfs.  this is like
					  super scary dangerous shit...
				    */
	  				if (bi->flags & LOCKED_FS) {
						int len = bfs->bbm.num_bitmap_blocks * bfs->dsb.block_size;

						memcpy(bfs->bbm.bv[0].bits, buf, len);
						write_blocks(bfs, 1, bfs->bbm.bv[0].bits, bfs->bbm.num_bitmap_blocks);
						sanity_check_bitmap(bfs);

					} else {
						printf("hey hombre you need to lock bfs to replace the bitmap!\n");
					}
					break;
				 }

	case 69666 : {
		            /*
					   auto-fragment a disk.
					   evil and dangerous but necessary for debugging
					*/
		            fragment_bitmap(bfs, buf);
					break;
	             }

#ifdef DEBUG
	 case 79321 : {
		 dump_btree((void *)buf);
		 break;
	 }
#endif

	 case 82969 : {
		 char     *name = (char *)buf;
		 dr9_off_t vnid;

		 if (S_ISDIR(bi->mode) == 0) {
			 err = EINVAL;
			 break;
		 }
		 
		 bfs->cur_lh = start_transaction(bfs);
		 if (bfs->cur_lh == NULL) {
			 err = ENOMEM;
			 break;
		 }

		 printf("force deleting %s from bi @ 0x%lx\n", name, bi);
		 bi->etc->counter++;
		 if (bt_delete(bi->etc->btree, (bt_chrp)name, strlen(name),
					   &vnid) != BT_OK) {
			 bt_perror(bi->etc->btree, "force delete");
			 printf("could not force delete %s\n", name);
		 }

		 end_transaction(bfs, bfs->cur_lh);

		 break;
	 }

	default: err = EINVAL;
			 break;
	}

	UNLOCK(bi->etc->lock);

	return err;
}

int
bfs_setflags(void *ns, void *node, void *cookie, int flags)
{
	int       *fcookie = (int *)cookie;

	*fcookie = ((*fcookie & ~(O_NONBLOCK | O_APPEND)) |
		(flags & (O_NONBLOCK | O_APPEND)));
	return 0;
}

int
bfs_rstat(void *ns, void *node, struct stat *st)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi = (bfs_inode *)node;

	CHECK_INODE(bi);

	st->st_dev     = bfs->nsid;
	st->st_ino     = inode_addr_to_vnode_id(bi->inode_num);
	st->st_mode    = bi->mode;
	st->st_nlink   = 1;             /* XXXdbg -- what about hard links? */
	st->st_uid     = bi->uid;
	st->st_gid     = bi->gid;
	if (S_ISLNK(bi->mode))
		st->st_size= 0;
	else {
		/* don't look at bi->data.size if it's being trimmed currently */
		while (bi->flags & TRIMMING_INODE)
			snooze(1000);

		st->st_size= bi->data.size;
	}
	st->st_blksize = 64 * 1024;
	st->st_atime   = time(NULL);
	st->st_mtime   = (time_t)(bi->last_modified_time >> TIME_SCALE);
	st->st_ctime   = st->st_mtime;
	st->st_crtime  = (time_t)(bi->create_time >> TIME_SCALE);

	return 0;
}

#define WSTAT_PROT_OWNER (WSTAT_MODE | WSTAT_UID | WSTAT_GID)
#define WSTAT_PROT_TIME  (WSTAT_ATIME | WSTAT_MTIME | WSTAT_CRTIME)
#define WSTAT_PROT_OTHER (~(WSTAT_PROT_OWNER | WSTAT_PROT_TIME))

int
bfs_wstat(void *ns, void *node, struct stat *st, long mask)
{
	int        err = 0;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi = (bfs_inode *)node;
	bigtime_t  new_time;
	uid_t      uid;
	
	if (mask == 0)
		return 0;

	CHECK_INODE(bi);

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	uid = geteuid();

	LOCK(bi->etc->lock);
	
	if (uid != 0) {
		/* if we're not root, we need to enforce some permissions... */
		if ((mask & WSTAT_PROT_OWNER) && (bi->uid != uid)) {
			/* chown/chgrp/chmod only allowed on files that we own */
			UNLOCK(bi->etc->lock);
			return EACCES;
		}
		if ((mask & WSTAT_PROT_TIME) && (bi->uid != uid) && (check_permission(bi, O_WRONLY) == 0)) {
			/* the time changing operations should go through if you
			own the file or you have write permissions */
			UNLOCK(bi->etc->lock);
			return EACCES;
		}			
		if ((mask & WSTAT_PROT_OTHER) && (check_permission(bi, O_WRONLY) == 0)) {
			/* other operations just use regular permission bits */
			UNLOCK(bi->etc->lock);
			return EACCES;
		}
	}
	
	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		UNLOCK(bi->etc->lock);
		printf("write_stat: can't start transaction (bi @ 0x%x)!\n", bi);
		return ENOMEM;
	}

	if (mask & WSTAT_MODE) {
		bi->mode = (bi->mode & S_IFMT) | (st->st_mode & S_IUMSK);
	}

	if (mask & WSTAT_UID) {
		if((uid == 0) || (st->st_uid == uid)) {
			bi->uid = st->st_uid;
		}
	}

	if (mask & WSTAT_GID) {
		if((uid == 0) || (st->st_gid == getegid())) {
			bi->gid = st->st_gid;
		}
	}

	if (mask & WSTAT_SIZE) {
		if (S_ISDIR(bi->mode)) {
			err = EISDIR;
		} else {
			err = set_file_size(bfs, bi, st->st_size);
		}

		if (err)
			goto done;
	}

	if (mask & WSTAT_CRTIME) {
		new_time = MAKE_TIME(st->st_crtime);
		if (S_ISDIR(bi->mode) == 0)
			update_create_time_index(bfs, bi, new_time);
		else
			bi->create_time = new_time;
	}


	if ((mask & WSTAT_MTIME) || (mask & WSTAT_SIZE)) {
		if (mask & WSTAT_MTIME) {
			new_time = MAKE_TIME(st->st_mtime);
		} else {
			new_time = CUR_TIME;
		}

		bi->last_modified_time = new_time;
		if (S_ISDIR(bi->mode) == 0) {
			update_last_modified_time_index(bfs, bi);
		}
	}


	if (mask & WSTAT_ATIME) {
		/* hah! nothing to do because we're wanky and don't maintain atime */
	}

	update_inode(bfs, bi);

	/* notify any listener of the changes. */
	notify_listener(B_STAT_CHANGED, bfs->nsid, 0, 0, bi->etc->vnid, NULL);

 done:
	end_transaction(bfs, bfs->cur_lh);
	UNLOCK(bi->etc->lock);

	return err;
}

int
bfs_fsync(void *ns, void *node)
{
	int        i, j, k, h, num_to_flush;
	off_t      bnum, ind_bnum, tmp_bnum, total_blocks = 0,
		       num_file_blocks;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi = (bfs_inode *)node;
	block_run *brun;

	if (bi->data.size == 0)   /* nothing to do */
		return 0;

	LOCK(bi->etc->lock);
	num_file_blocks = (bi->data.size / bfs->dsb.block_size) + 1;

	/* if the file is small we just do it directly */
	for(i=0; i < NUM_DIRECT_BLOCKS; i++) {
		bnum = block_run_to_bnum(bi->data.direct[i]);
		if (bnum == 0)
			break;

		num_to_flush = bi->data.direct[i].len;
		if (total_blocks+num_to_flush > num_file_blocks)
			num_to_flush = num_file_blocks - total_blocks;
		total_blocks += num_to_flush;

		flush_blocks(bfs->fd, bnum, num_to_flush);
	}

	/* now try and do the indirect blocks */
	bnum = block_run_to_bnum(bi->data.indirect);
	if (bnum) {
		for(i=0; i < bi->data.indirect.len; i++) {
			brun = (block_run *)get_block(bfs->fd, bnum + i, bfs->dsb.block_size);

			for(j=0; j < bfs->dsb.block_size/sizeof(block_run); j++) {
				tmp_bnum = block_run_to_bnum(brun[j]);
				if (tmp_bnum == 0)
					break;
				
				num_to_flush = brun[j].len;
				if (total_blocks+num_to_flush > num_file_blocks)
					num_to_flush = num_file_blocks - total_blocks;
				total_blocks += num_to_flush;

				flush_blocks(bfs->fd, tmp_bnum, num_to_flush);
			}

			release_block(bfs->fd, bnum + i);
		}
	}
	
	/* and finally the double-indirect blocks */
	bnum = block_run_to_bnum(bi->data.double_indirect);
	if (bnum) {
		block_run *br;
		
		for(i=0; i < bi->data.double_indirect.len; i++) {
			br = get_block(bfs->fd, bnum + i,
						   bfs->dsb.block_size);
		
			for(j=0; j < bfs->dsb.block_size/sizeof(block_run); j++) {
				ind_bnum = block_run_to_bnum(br[j]);
				if (ind_bnum == 0)
					break;
				
				for(k=0; k < br[j].len; k++) {
					brun = get_block(bfs->fd, ind_bnum + k,
									 bfs->dsb.block_size);

					for(h=0; h <bfs->dsb.block_size/sizeof(block_run); h++) {
						tmp_bnum = block_run_to_bnum(brun[h]);
						if (tmp_bnum == 0)
							break;
				
						num_to_flush = brun[h].len;
						if (total_blocks+num_to_flush > num_file_blocks)
							num_to_flush = num_file_blocks - total_blocks;
						total_blocks += num_to_flush;

						flush_blocks(bfs->fd, tmp_bnum, num_to_flush);
					}

					release_block(bfs->fd, ind_bnum + k);
				}
			}
		
			release_block(bfs->fd, bnum+i);
		}
	}
	UNLOCK(bi->etc->lock);
	
	return 0;

}

int
bfs_sync(void *ns)
{
	bfs_info *bfs = (bfs_info *)ns;

	sync_log(bfs);
	
	return 0;
}

int
bfs_lock(void *ns, void *node)
{
	/* XXXdbg file locking not done yet */
	return 0;
}

int
bfs_unlock(void *ns, void *node)
{
	/* XXXdbg file unlocking not done yet */
	return 0;
}

/*
   unforunately this function has to be the one to start a transaction
   because it potentially has to lock the semaphore for "name" if "name"
   happens to be a directory.  if we already had started a transaction
   before calling do_unlink() then we would have a deadlock with functions
   like bfs_create() which do all their locking before starting a
   transaction.

   If owner is non-zero (not root) we are unlinking in a directory with 
   the sticky bit set, so owner must match dir->uid or entry->uid.
*/   
int
do_unlink(bfs_info *bfs, bfs_inode *dir, const char *name, int must_be_dir, uid_t owner)
{
	int        err = 0;
	dr9_off_t      val = BT_ANY_RRN;
	vnode_id   vnid;
	bfs_inode *entry = NULL;

	dir->etc->counter++;
	if (bt_find(dir->etc->btree, (bt_chrp)name, strlen(name), &val) != BT_OK) {
		err = ENOENT;
		goto done;
	}
	
	vnid = (vnode_id)val;
	/* printf("removing: %6d %s\n", vnid, name); */

	if (vnid == 0) 
		bfs_die("do_unlink: vnid == 0 (dir 0x%x, name %s)\n", dir, name);

	if (get_vnode(bfs->nsid, vnid, (void **)&entry) != 0) {
		printf("unlink failed to get vnode for %s\n", name);
		err = EBADF;
		entry = NULL;
		goto done;
	}

	
	LOCK(entry->etc->lock);

	if (entry->flags & INODE_NO_CACHE) {
		err = EPERM;
		goto done;
	}

	if (must_be_dir == TRUE && S_ISDIR(entry->mode) == 0) {
		err = ENOTDIR;
		goto done;
	} else if (must_be_dir == FALSE &&
			   (S_ISDIR(entry->mode) && S_ISINDEX(entry->mode) == 0)) {
		err = EISDIR;
		goto done;
	}

	if ((owner != 0) && (entry->uid != owner) && (dir->uid != owner)) {
		err = EACCES;
		goto done;
	}
	
	if (S_ISDIR(entry->mode) && S_ISINDEX(entry->mode) == 0) {
		
		if (dir_is_empty(bfs, entry) == 0) {
			err = ENOTEMPTY;
		} else {
			if (remove_vnode(bfs->nsid, vnid) != 0)
				err = ENOTEMPTY;
		}

		if (err)
			goto done;
	} else {
		if (remove_vnode(bfs->nsid, vnid) != 0) {
			err = ENOTEMPTY;
			goto done;
		}
	}

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		printf("do_unlink: can't start transaction (entry @ 0x%x)!\n", entry);
		unremove_vnode(bfs->nsid, vnid);
		err = ENOMEM;
		goto done;
	}


	dir->etc->counter++;
	if (bt_delete(dir->etc->btree, (bt_chrp)name, strlen(name), &val) != BT_OK) {
		err = EINVAL;
		unremove_vnode(bfs->nsid, vnid);
		goto done;
	}

	if (entry && S_ISINDEX(entry->mode) == 0)
		del_from_main_indices(bfs, name, entry);
	else 
		;/* XXXdbg need to force deletion from the main indices */

	if (entry) {
		entry->flags |= INODE_DELETED;    /* I'm a leper: don't touch me */

		/* this is important: make sure the entry is updated on disk!!!! */
		update_inode(bfs, entry);
	}

	dir->last_modified_time = CUR_TIME;
	update_inode(bfs, dir);

	/*
	   XXXdbg -- have to go and delete this file from all the other
	   indices it may belong to as well!  We should probably do those
	   deletions as a separate transaction or else the transaction
	   size could get too large.
	*/


	if (entry) {
		notify_listener(B_ENTRY_REMOVED, bfs->nsid, dir->etc->vnid, 0, vnid, NULL); 
	}

 done:
	if (entry) {
		UNLOCK(entry->etc->lock);

		put_vnode(bfs->nsid, vnid);
	}

	return err;
}

int
bfs_unlink(void *ns, void *_dir, const char *name)
{
	int        err = 0;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir;
	
	dir = (bfs_inode *)_dir;

	CHECK_INODE(dir);

	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EPERM;
	
	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	acquire_sem(bfs->sem);
	/* XXXdbg -- if the directory size could shrink by doing the deletion
	   we'd need to be careful with code in dstream.c that might try to
	   lock this directory a second time for exclusive access.  Other places
	   in this file (and others) that call bt_delete() would need to be
	   checked as well.
	*/   
	LOCK(dir->etc->lock);

	if (check_permission(dir, O_WRONLY) == 0) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return EACCES;
	}

	/*
	   XXXdbg need to check that the execute bit of the dir_vnode is set
	   and if not, return EPERM
	*/   

	if (dir->mode & S_ISVTX) {
		err = do_unlink(bfs, dir, name, FALSE, geteuid());
	} else {
		err = do_unlink(bfs, dir, name, FALSE, 0);
	}
	
	/* only call abort or end_transaction if the transaction was started
	   (and that can only happen if err == 0 or EINVAL */
	if (err == EINVAL)
		abort_transaction(bfs, bfs->cur_lh);
	else if (err == 0)
		end_transaction(bfs, bfs->cur_lh);
	
	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);

	return err;
}



void
update_dot_dot(bfs_info *bfs, bfs_inode *old, bfs_inode *newparent)
{
	vnode_id vnid;

	vnid = inode_addr_to_vnode_id(old->parent);
	
	old->etc->counter++;
	if (bt_delete(old->etc->btree, (bt_chrp)"..", 2, (dr9_off_t *)&vnid) != BT_OK) {
		printf("error deleting `..' from dir %d:%d:%d during rename!\n",
			   old->inode_num.allocation_group, old->inode_num.start,
			   old->inode_num.len);
	}
	
	old->etc->counter++;
	if (bt_insert(old->etc->btree, (bt_chrp)"..", 2, newparent->etc->vnid, 0) != BT_OK) {
		printf("error re-inserting `..' into dir %Ld during rename\n",
			   old->etc->vnid);
		bt_perror(old->etc->btree, "rename");
	}
}



int
bfs_rename(void *ns, void *_olddir, const char *oldname, void *_newdir,
		   const char *newname)
{
	int        err = 0;
	vnode_id   vnid, new_vnid;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *olddir = _olddir, *newdir = _newdir, *old = NULL, *dir;
	
	if (oldname == NULL || oldname == '\0' || 
		newname == NULL || *newname == '\0' || strchr(newname, '/'))
		return EINVAL;

	/* first check if we're trying to move onto ourselves... */
	if (olddir->etc->vnid == newdir->etc->vnid && strcmp(oldname, newname) == 0)
		return EPERM;

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	if (check_permission(olddir, O_WRONLY) == 0 ||
		check_permission(newdir, O_WRONLY) == 0)
		return EACCES;


	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);
	LOCK(olddir->etc->lock);
	if (olddir->etc->vnid != newdir->etc->vnid)
		LOCK(newdir->etc->lock);

	if (lookup(olddir, oldname, &vnid) != 0) {
		err = ENOENT;
		goto done;
	}

	if (vnid == 0) 
		bfs_die("rename: vnid == 0 (olddir 0x%x, name %s)\n", olddir, oldname);

	if (vnid == olddir->etc->vnid || vnid == newdir->etc->vnid) {
		err = EINVAL;
		goto done;
	}

	/* this prevents us trying to double-lock olddir in do_unlink() */
	if (lookup(newdir, newname, &new_vnid) == 0) {
		if (new_vnid == olddir->etc->vnid) {
			err = EINVAL;
			goto done;
		}
	}

	if (get_vnode(bfs->nsid, vnid, (void **)&old) != 0) {
		err = ENOENT;
		goto done;
	}

	LOCK(old->etc->lock);

	/* if directory is sticky, we must be root or own the file or directory */
	if (olddir->mode & S_ISVTX) {
		uid_t uid = geteuid();
		if ((uid != 0) && (uid != old->uid) && (uid != olddir->uid)) {
			err = EACCES;
			goto done0;
		}
	}
	
	/* verify that we don't move one of our parents into ourselves */
	for(dir=newdir; dir; ) {
		dr9_off_t bnum;
		vnode_id  save_vnid;
		
		if (old->etc->vnid == dir->etc->vnid) {
			err = EINVAL;
			if (dir != newdir)
				put_vnode(bfs->nsid, dir->etc->vnid);
			
			goto done0;
		}

		/* get the info we need before doing the put_vnode() */
		save_vnid = dir->etc->vnid;
		bnum = block_run_to_bnum(dir->parent);
	    if (bnum == 0) {
	     	bfs_die("rename: bnum == 0 (dir 0x%x)\n", dir);
			goto done0;
		}

		if (dir != newdir)
			put_vnode(bfs->nsid, dir->etc->vnid);

		if (save_vnid == bfs->root_dir->etc->vnid) /* stop at the root */
			break;

		if ((err = get_vnode(bfs->nsid, bnum, (void **)&dir)) != 0) {
			goto done0;
		}
	}

	/* make sure there's enough room for all bt_insert()'s to work */
	if (reserve_space(bfs, NUM_INSERT_BLOCKS) != 0) {
		err = ENOSPC;
		goto done0;
	}

	/* if the other file exists, this will blow it away */
	/* if the new dir has the sticky bit, let do_unlink() know who we are */
	if(newdir->mode & S_ISVTX) {
		err = do_unlink(bfs, newdir, newname, -1, geteuid());
	} else {
		err = do_unlink(bfs, newdir, newname, -1, 0);
	}
	
	if (err != 0 && err != ENOENT) {
		printf("rename: error deleting %s from dir vn 0x%Lx (%s)\n", newname,
			   newdir->etc->vnid, strerror(err));
		goto done1;
	}

	if (err == ENOENT) {
		/* then we have to call start_transaction() since do_unlink() didn't */
		bfs->cur_lh = start_transaction(bfs);
		if (bfs->cur_lh == NULL) {
			printf("rename: can't start transaction (%s -> %s)!\n", oldname,
				   newname);
			err = ENOMEM;
			goto done1;
		}
	}

	err = 0;
	
	newdir->etc->counter++;
	if (bt_insert(newdir->etc->btree, (bt_chrp)newname, strlen(newname),vnid,0) != BT_OK) {
		printf("warning: failed to move file %s to %s (vnid 0x%Lx)\n", oldname,
			   newname, vnid);
		bt_perror(newdir->etc->btree, "rename");
		goto done2;
	}

	if (olddir != newdir)
		newdir->last_modified_time = CUR_TIME;
	update_inode(bfs, newdir);


	if (S_ISDIR(old->mode)) {
		update_dot_dot(bfs, old, newdir);
	}
	
	/*
	   here we try to be careful about failures so that we don't do too
	   much if one of these operations should fail.  it's still a serious
	   problem if any of these fail (especially since we don't deal with
	   the failures in a very graceful way) but we try to minimize the
	   damage by cascading the if statements so that 
	*/   
	olddir->etc->counter++;
	if (bt_delete(olddir->etc->btree, (bt_chrp)oldname, strlen(oldname),
				  (dr9_off_t *)&vnid) != BT_OK) {
		printf("error deleting %s from dir %d:%d:%d during rename!\n", oldname,
			   olddir->inode_num.allocation_group, olddir->inode_num.start,
			   olddir->inode_num.len);
	} else {
		old->parent = newdir->inode_num;

		olddir->last_modified_time = CUR_TIME;

		if (strcmp(oldname, newname) != 0) {
			update_name_index(bfs, old, oldname, newname);
		}
	}

	update_inode(bfs, old);

 done2:
	if (err == 0) {
		end_transaction(bfs, bfs->cur_lh);

/* printf("notify: moved %Ld - %s\n", old->etc->vnid, newname); */
		notify_listener(B_ENTRY_MOVED, bfs->nsid, olddir->etc->vnid,
						newdir->etc->vnid, old->etc->vnid, newname); 
	} else {
		abort_transaction(bfs, bfs->cur_lh);
	}

 done1:
	unreserve_space(bfs, NUM_INSERT_BLOCKS);

 done0:
	UNLOCK(old->etc->lock);
	put_vnode(bfs->nsid, old->etc->vnid);

 done:
	if (olddir->etc->vnid != newdir->etc->vnid)
		UNLOCK(newdir->etc->lock);

	UNLOCK(olddir->etc->lock);
	release_sem_etc(bfs->sem, MAX_READERS, 0);

	return err;
}



int
bfs_remove_vnode(void *ns, void *node, char re_enter)
{
	dr9_off_t   bnum;
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	binode_etc *betc;
	inode_addr  ia;
	
	if (node == NULL) {
		printf("** warning: remove_vnode passed null data!\n");
		return 0;
	}
	
	CHECK_INODE(bi);

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	if (S_ISDIR(bi->mode)) {
		if (bi->etc->btree)
			bt_close(bi->etc->btree);
		bi->etc->btree = NULL;
	}


	bnum = block_run_to_bnum(bi->inode_num);
	if (bnum == 0)
		bfs_die("remove_vnode: bi @ 0x%x has a zero inode num\n", bi);

	if (block_run_to_bnum(bi->parent) == 0)
		bfs_die("remove_vnode: bi @ 0x%x has a zero parent inode num\n", bi);

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		printf("remove vnode: can't start transaction (bi @ 0x%x)!\n", bi);
		return ENOMEM;
	}

	/* printf("free'ing %d %d %d\n", bi->inode_num.allocation_group,
		   bi->inode_num.start, bi->inode_num.len); */
	if ((bi->flags & LONG_SYMLINK) || S_ISLNK(bi->mode) == 0)
		free_data_stream(bfs, bi);

	free_attributes(bfs, bi);


	/* first save off the relevant info from the i-node */
	ia      = bi->inode_num;
	betc    = bi->etc;
	bi->etc = NULL;

	/* then free the blocks used by the i-node */
	free_inode(bfs, &ia);

	/* delete the i-node lock (nobody should be blocked on it) */
	free_lock(&betc->lock);
	betc->lock.s = -1;

	/* now release the block from the cache */
	release_block(bfs->fd, bnum);

	/* and finally free the etc pointer now that everything is gone */
	free(betc);


	end_transaction(bfs, bfs->cur_lh);
	
	return 0;
}


