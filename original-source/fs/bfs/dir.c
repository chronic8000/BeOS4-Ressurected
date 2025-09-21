#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bfs.h"
#include "btlib/btree.h"


bfs_inode *
make_dir(bfs_info *bfs, bfs_inode *dir, uint mode_bits, vnode_id *result)
{
	int        data_type = BT_STRING;
	char       buff[128];
	dr9_off_t      bnum, parent;
	bfs_inode *bi;
	BT_INDEX  *b;
	inode_addr ia;

	bi = allocate_inode(bfs, dir, S_IFDIR | mode_bits);
	if (bi == NULL)
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

	if (mode_bits & S_STR_INDEX)
		data_type = BT_STRING;
	else if (mode_bits & S_INT_INDEX)
		data_type = BT_INT;
	else if (mode_bits & S_UINT_INDEX)
		data_type = BT_UINT;
	else if (mode_bits & S_LONG_LONG_INDEX)
		data_type = BT_LONG_LONG;
	else if (mode_bits & S_ULONG_LONG_INDEX)
		data_type = BT_ULONG_LONG;
	else if (mode_bits & S_FLOAT_INDEX)
		data_type = BT_FLOAT;
	else if (mode_bits & S_DOUBLE_INDEX)
		data_type = BT_DOUBLE;
	else
		mode_bits |= S_STR_INDEX;   /* force it to be a string */
	
	bi->mode     |= (S_IFDIR | mode_bits);
	bi->parent    = dir->inode_num;
	bi->uid       = geteuid();
	bi->gid       = getegid();
	bi->flags    |= INODE_LOGGED;
	vnode_id_to_inode_addr(bi->attributes, 0);


    b = bt_optopen(BT_INODE,    bi,
				   BT_BFS_INFO, bfs,
				   BT_CFLAG,    BT_NOCACHE,
				   BT_CACHE,    0,
				   BT_PSIZE,    1024,
				   BT_DTYPE,    data_type,
				   BT_OMODE,    1,           /* forces creation */
				   0);

	if (b == NULL)
		goto cleanup;

	bi->etc->btree     = b;
	bi->etc->counter   = 0;
	bi->etc->old_size  = -1;
	bi->etc->old_mtime = -1;
	
	parent = inode_addr_to_vnode_id(dir->inode_num);
	if (parent <= 0)    /* only happens when creating the root dir of a fs */
		parent = bnum;
	
	if ((mode_bits & S_INDEX_DIR) == 0 && (mode_bits & S_ATTR_DIR) == 0) {
		/* XXXdbg -- need to check errors here! */
		bt_insert(b, (bt_chrp)".",  1, bnum, 0);
		bt_insert(b, (bt_chrp)"..", 2, parent, 0);
	}


	*result       = bnum;
	bi->etc->vnid = *result;

	/* c'mere baby.  i'm gonna make you a real woman... */
	bi->flags &= ~INODE_VIRGIN;
	
	update_inode(bfs,  bi);


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


static void
add_vol_id(bfs_info *bfs, bfs_inode *bi)
{
	long long volid;
	size_t    len = sizeof(volid);
	status_t  x;

	volid = (system_time() + (bi->create_time * 1000000)) ^ bi->create_time;
	len = sizeof(volid);
	x = bfs_write_attr(bfs, bi, "be:volume_id", B_UINT64_TYPE,
					   &volid, &len, 0);
}


int
create_root_dir(bfs_info *bfs)
{
	vnode_id   result, err;
	bfs_inode  *bi, tmp;
	
	vnode_id_to_inode_addr(tmp.inode_num, 0);
	
	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		printf("create_root_dir: can't start transaction!\n");
		return ENOMEM;
	}
	
	if ((bi = make_dir(bfs, &tmp, S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, &result)) == NULL) {
		abort_transaction(bfs, bfs->cur_lh);
		return EINVAL;
	}
		
	add_vol_id(bfs, bi);

	bi->parent = bi->inode_num;
	update_inode(bfs, bi);


	end_transaction(bfs, bfs->cur_lh);

	if ((err = new_vnode(bfs->nsid, result, bi)) != 0) 
		bfs_die("mkdir: new vnode failed for vnid %Ld: %s\n", result, strerror(err));

	bfs->dsb.root_dir = bi->inode_num;
	bfs->root_dir     = bi;

	return 0;
}

int
bfs_mkdir(void *ns, void *_dir, const char *name, int mode_bits)
{
	status_t   err = 0;
	vnode_id   vnid = BT_ANY_RRN;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir = (bfs_inode *)_dir, *bi = NULL;
	char *s;

	CHECK_INODE(dir);

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	if (S_ISDIR(dir->mode) == 0)
		return EINVAL;

	if (check_permission(dir, O_WRONLY) == 0)
		return EACCES;


	mode_bits &= S_IUMSK;   /* make sure no bogus stuff is passed in */

	/* 
 	 * This is to allow passing mkdir() a name with a '/' at the
	 * end.
	 */	
	s = strchr(name, '/');
	if (s && *(s+1) != 0)
		return EINVAL;

	if (name == NULL || *name == '\0' ||  
		strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return EINVAL;
	}

	acquire_sem(bfs->sem);
	LOCK(dir->etc->lock);

	if (dir->flags & INODE_DELETED) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);

		return ENOENT;
	}

	dir->etc->counter++;
	if (bt_find(dir->etc->btree, (bt_chrp)name, strlen(name), (dr9_off_t *)&vnid) == BT_OK) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);

		return EEXIST;
	}


	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		printf("mkdir: can't start transaction (dir @ 0x%x)!\n", dir);
		return ENOMEM;
	}

	/* make sure there's enough room for all bt_insert()'s to work */
	if (reserve_space(bfs, NUM_INSERT_BLOCKS) != 0) {
		abort_transaction(bfs, bfs->cur_lh);
		UNLOCK(dir->etc->lock);
		release_sem(bfs->sem);
		return ENOSPC;
	}

	if ((bi = make_dir(bfs, dir, mode_bits, &vnid)) == NULL)
		goto cleanup;
			
	if (create_name_attr(bfs, bi, name) != 0) {
		printf("bfs: mkdir: bi @ 0x%x can't add name attribte %s?\n",bi,name);
		goto cleanup;
	}

	dir->etc->counter++;
	if (bt_insert(dir->etc->btree, (bt_chrp)name, strlen(name), vnid, 0) != BT_OK){
		bt_perror(dir->etc->btree, "create");
		printf("*** failed to insert dir %s (vnid 0x%Lx freeblk %Ld)\n", 
               name, vnid, bfs->dsb.num_blocks - bfs->dsb.used_blocks);
		/* here we still force the call to bt_delete() just to be safe */
		goto file_cleanup;
	}

	dir->last_modified_time = CUR_TIME;
	update_inode(bfs, dir);

	/* set this so no one fucks with it till we're ready */
	bi->flags |= INODE_VIRGIN;   

	if ((mode_bits & S_INDEX_DIR) == 0) {
		if (add_to_main_indices(bfs, name, bi) != 0) {
			goto file_cleanup;
		}
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
	bt_delete(dir->etc->btree, (bt_chrp)name, strlen(name), (dr9_off_t *)&vnid);

 cleanup:
	if (bi) {
		block_run inum = bi->inode_num;
		
		free_data_stream(bfs, bi);
		free_lock(&bi->etc->lock);
		free(bi->etc);
		bi->etc = NULL;
		bi->flags |=  INODE_DELETED;  /* so no one will try to load it */
		bi->flags &= ~INODE_VIRGIN;   /* breaks out pending read_vnode() calls */
		release_block(bfs->fd, vnid);
		free_inode(bfs, &inum);
	}

	unreserve_space(bfs, NUM_INSERT_BLOCKS);
	abort_transaction(bfs, bfs->cur_lh);

	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);
	return ENOENT;
}


#define DIR_IS_DELETED  0x829


int
bfs_rmdir(void *ns, void *_dir, const char *name)
{
	int        err = 0;
	vnode_id   vnid;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir, *entry;

	dir = (bfs_inode *)_dir;

	CHECK_INODE(dir);

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	if (S_ISDIR(dir->mode) == 0)
		return EINVAL;

	if (check_permission(dir, O_WRONLY) == 0)
		return EACCES;

	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return EPERM;
	
	acquire_sem(bfs->sem);

	/* XXXdbg -- if the directory size could shrink by doing the deletion
	   we'd need to be careful with code in dstream.c that might try to
	   lock this directory a second time for exclusive access.  Other places
	   in this file (and others) that call bt_delete() would need to be
	   checked as well.
	*/   
	LOCK(dir->etc->lock);

	if ((err = lookup(dir, name, &vnid)) != 0)
		goto done0;

	if ((err = get_vnode(bfs->nsid, vnid, (void **)&entry)) != 0)
		goto done0;
	

	/*
	   XXXdbg need to check that the execute bit of the dir_vnode is set
	   and if not, return EPERM
	*/   

	if (dir->mode & S_ISVTX) {
		/* if the sticky bit is set, do_unlink enforces ownership */
		err = do_unlink(bfs, dir, name, TRUE, geteuid());
	} else {
		err = do_unlink(bfs, dir, name, TRUE, 0);
	}
	
	/*
	   mark the len field as invalid so that we can fail any future
	   lookups to this inode.  this 
	*/
	if (err == 0) {
		entry->parent.len = DIR_IS_DELETED;
		/* bnum_to_block_run(entry->parent, 0); */
	}

	put_vnode(bfs->nsid, entry->etc->vnid);

	/* only call abort or end_transaction if the transaction was started
	   (and that can only happen if err == 0 or EINVAL */
	if (err == EINVAL)
		abort_transaction(bfs, bfs->cur_lh);
	else if (err == 0)
		end_transaction(bfs, bfs->cur_lh);
	
 done0:
	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);

	return err;
}


/*
   the dir sem must be held to call this function.
*/
int
lookup(bfs_inode *dir, const char *name, vnode_id *result)
{
	dr9_off_t      res = BT_ANY_RRN;
	BT_INDEX  *b;

	if (dir->parent.len == DIR_IS_DELETED)
		return ENOENT;

	b = (BT_INDEX *)dir->etc->btree;
	if (b == NULL) {
		return ENOENT;
	}

	dir->etc->counter++;
	if (bt_find(b, (bt_chrp)name, strlen(name), &res) != BT_OK) {
		return ENOENT;
	}

	*result = (vnode_id)res;

	return 0;
}


/*
   the dir sem must be held to call this function.
*/
int
dir_is_empty(bfs_info *bfs, bfs_inode *dir)
{
	char  name[B_FILE_NAME_LENGTH];
	int   i, nlen, val = 0, err;
	dr9_off_t ino;
	
	dir->etc->counter++;
	if (bt_goto(dir->etc->btree, BT_BOF) != BT_OK) {
		printf("inode: 0x%Lx had btree problems (1)!\n", dir->etc->vnid);
		goto done;
	}
	
	/* this skips over the entries for "." and ".." */
	for(i=0; i < 2; i++) {
		nlen = sizeof(name);

		dir->etc->counter++;
		err = bt_traverse(dir->etc->btree, BT_EOF, (bt_chrp)name, nlen,
						  &nlen, &ino);
		if (err != BT_OK) {    /* then the directory is not empty! */
			printf("inode: 0x%Lx had btree problems (2)!\n", dir->etc->vnid);
			goto done;
		}
	}

	dir->etc->counter++;
	nlen = sizeof(name);
	err = bt_traverse(dir->etc->btree, BT_EOF, (bt_chrp)name, nlen,
					  &nlen, &ino);
	if (err == BT_EOF) {		/* then the directory is empty! */
		val = 1;
	} else if (err != BT_OK) {
		printf("inode: 0x%Lx had btree problems (3) (err %d)!\n",
			   dir->etc->vnid, err);
	}
	

 done:
	return val;
}



int
bfs_opendir(void *ns, void *node, void **cookie)
{
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	dir_cookie *dc  = (dir_cookie *)cookie;

	CHECK_INODE(bi);

	if (S_ISDIR(bi->mode) == 0) {
		return EINVAL;
	}

	if (S_ISLNK(bi->mode)){
		return EINVAL;
	}
	
#if 1
	if (!(bi->mode & S_INDEX_DIR)) 
		if (check_permission(bi, O_RDONLY) == 0)
			return EACCES;
#else
	if (check_permission(bi, O_RDONLY) == 0)
		return EACCES;
#endif	
	*cookie = (void *)dc = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (dc == NULL)
		return ENOMEM;
	
	dc->name[0] = '\0';

	LOCK(bi->etc->lock);
	dc->counter = bi->etc->counter;
	UNLOCK(bi->etc->lock);

	return 0;
}

int
bfs_closedir(void *ns, void *node, void *cookie)
{
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;

	CHECK_INODE(bi);

	if (S_ISDIR(bi->mode) == 0) {
		printf("*** closedir on something that's not a directory\n");
		return EPERM;
	}
		
	return 0;
}

int
bfs_rewinddir(void *ns, void *node, void *cookie)
{
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	dir_cookie *dc  = (dir_cookie *)cookie;

	CHECK_INODE(bi);

	if (S_ISDIR(bi->mode) == 0)
		return EINVAL;

	if (cookie == NULL) {
		printf("invalid cookie passed to rewwindir!\n");
		return EINVAL;
	}

	dc->name[0] = '\0';

	LOCK(bi->etc->lock);
	dc->counter = bi->etc->counter;
	UNLOCK(bi->etc->lock);

	return 0;
}

int	bfs_readdir(void *ns, void *node, void *cookie, long *num,
				struct dirent *buf, size_t bufsize)
{
	int            err, llen;
	bfs_info      *bfs  = (bfs_info *)ns;
	bfs_inode     *bi   = (bfs_inode *)node;
	dir_cookie    *dc   = (dir_cookie *)cookie;
	struct dirent *dent = (struct dirent *)buf;

	CHECK_INODE(bi);

	if (cookie == NULL) {
		printf("invalid cookie passed to readdir!\n");
		return EINVAL;
	}

	LOCK(bi->etc->lock);

	if (dc->name[0] == '\0') {
		bi->etc->counter++;
		if (bt_goto(bi->etc->btree, BT_BOF) != BT_OK) {
			bt_perror(bi->etc->btree, "dir: read: goto");
			UNLOCK(bi->etc->lock);
			return ENOTDIR;
		}

		dc->counter = bi->etc->counter;
	}

	if (dc->counter != bi->etc->counter) {
		if (bt_find(bi->etc->btree, (bt_chrp)dc->name, strlen(dc->name),
					(bt_off_t *)&dc->vnid) == BT_ERR) {
			bt_perror(bi->etc->btree, "readdir reset");
			printf("got an error searching for %s (0x%Lx)!\n",
					dc->name, dc->vnid);
		}
	}

	/* XXXdbg -- should read multiple dirent's if possible!!! */
	llen = bufsize - sizeof(*dent);
	bi->etc->counter++;
	err = bt_traverse(bi->etc->btree, BT_EOF, (bt_chrp)&dent->d_name[0],
					  llen, &llen, (dr9_off_t *)&dent->d_ino);
	
	dc->counter = bi->etc->counter;

	if (err == BT_EOF) {
		*num = 0;
	} else if (err != BT_OK) {
		bt_perror(bi->etc->btree, "dir: read: traverse");
		UNLOCK(bi->etc->lock);
		return ENOTDIR;
	} else if (err == BT_OK) {
		dent->d_name[llen] = '\0';  /* have to null terminate it */
		dent->d_reclen = (sizeof(*dent)-1) + (llen+1);	/* dirent's d_name contains 1 char */
		*num = 1;

		strcpy(dc->name, &dent->d_name[0]);
		dc->vnid = dent->d_ino;
	}

	UNLOCK(bi->etc->lock);

	return 0;
}


int
bfs_free_dircookie(void *ns, void *node, void *cookie)
{
#if 0   /* these aren't needed for now */
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	dir_cookie *dc  = (dir_cookie *)cookie;
#endif

	if (cookie == NULL) {
		printf("invalid cookie passed to free dircookie!\n");
		return EINVAL;
	}

	free(cookie);

	return 0;
}
