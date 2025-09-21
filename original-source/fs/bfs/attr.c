#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bfs.h"
#include "btlib/btree.h"

#ifdef __BEOS__
#include <fs_attr.h>
#else
#include "fs_attr.h"
#endif


#define NEXT_SD(sd)                                          \
         (small_data *)((char *)sd + sizeof(*sd) +           \
						sd->name_size + sd->data_size)

#define SD_DATA(sd)                                          \
         (void *)((char *)sd + sizeof(*sd) + (sd->name_size-sizeof(sd->name)))


#define NAME_MAGIC  0x13  /* identifies the special "hidden" name attribute */


static int remove_big_attr(bfs_info *bfs, bfs_inode *bi, const char *name);



static small_data *
find_small_data(bfs_inode *bi, const char *name)
{
	int         nlen = strlen(name), maxlen;
	small_data *sd, *end;

	sd  = (small_data *)&bi->small_data[0];
	end = (small_data *)((char *)bi + bi->inode_size);


	/* first find a free slot */
	for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
		maxlen = (nlen > sd->name_size) ? nlen : sd->name_size;
		if (sd->name[0] != '\0')
			if (strncmp(sd->name, name, maxlen) == 0)
				break;
	}		
		
	if (((char *)sd + sizeof(*sd)) >= ((char *)end))
		return NULL;

	return sd;
}


static int
create_small_data(bfs_inode *bi, const char *name, int type, const void *buf, size_t *len)
{
	int name_len = strlen(name), size_needed, old_size;
	small_data *sd, *end, *next, *onext;

	sd  = (small_data *)&bi->small_data[0];
	end = (small_data *)((char *)bi + bi->inode_size);

	size_needed = (name_len - sizeof(sd->name)) + *len;


	/* iterate through till we hit the free region at the end */
	for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
		if (sd->name[0] == '\0')
			break;
	}		
		
	if (((char *)sd + sizeof(*sd)) >= ((char *)end))
		return ENOSPC;

	if (size_needed > sd->data_size)
		return ENOSPC;

	onext = NEXT_SD(sd);

	old_size = sd->data_size;

	sd->type = type;
	
	strcpy(sd->name, name);
	sd->name_size = name_len;

	memcpy(SD_DATA(sd), buf, *len);
	sd->data_size = *len;

	/* printf("creating sd @ 0x%x (%s %d %d)\n", sd, name, sd->name_size,
	   sd->data_size); */

	next = NEXT_SD(sd);
	if (next < end)
		memset(next, 0, (char *)end - (char *)next);
		
	if (((char *)next + sizeof(*next)) < ((char *)end)) {
		next->data_size = (char *)end - (char *)next - sizeof(*next);
	}

	return 0;
}


static int
update_small_data(small_data *sd, const void *buf)
{
	memcpy(SD_DATA(sd), buf, sd->data_size);

	return 0;
}



static int
read_small_data(bfs_inode *bi, const char *name, void *buf, size_t *len, int type)
{
	small_data *sd;

	sd = find_small_data(bi, name);

	if (sd) {
		int amount = sd->data_size;
		if (*len >= 0 && amount > *len)
			amount = *len;

		memcpy(buf, SD_DATA(sd), amount);
		*len = amount;
	} else
		return ENOENT;

	return 0;
}

int
delete_small_data(bfs_inode *bi, const char *name)
{
	int         nlen = strlen(name), maxlen = 0;
	small_data *sd, *end, *next, *prev = NULL;

	sd  = (small_data *)&bi->small_data[0];
	end = (small_data *)((char *)bi + bi->inode_size);


	/* first find the attribute we want */
	for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
		maxlen = (nlen > sd->name_size) ? nlen : sd->name_size;
		if (sd->name[0] != '\0') {
			/* check to make sure that the strcmp won't go too far */
			if ((char *)&sd->name[0] + nlen >= (char *)end)
				continue;

			if (strncmp(sd->name, name, maxlen) == 0)
				break;
		}

		prev = sd;
	}		
		
	if (((char *)sd + sizeof(*sd)) >= ((char *)end) ||
		strncmp(sd->name, name, maxlen) != 0)
		return ENOENT;

	/* printf("deleting sd @ 0x%x (%s %d %d)\n", sd, name, sd->name_size,
	   sd->data_size); */

	sd->name[0] = '\0';  /* mark this guy as ready, available and willing */
	
	next = NEXT_SD(sd);

	if (next < end) {
		char *tmp;

		memcpy(sd, next, ((char *)end - (char *)next));

		/*
		   get a pointer to just after the area we copied so we can clear it
		   to zero.  this is important: there could be junk out there that
		   would confuse us later.
		*/
		tmp = (char *)sd + ((char *)end - (char *)next);
		if (tmp < (char *)end)
			memset(tmp, 0, ((char *)end) - tmp);
	}


	next = sd;
	for(; ((char *)next + sizeof(*next)) < ((char *)end); next=NEXT_SD(next)) {
		if (next->name[0] == '\0')
			break;
	}

	if (next < end)
		memset(next, 0, ((char *)end - (char *)next));

	if (((char *)next + sizeof(small_data)) < (char *)end) {
		next->type      = 0;
		next->name[0]   = '\0';
		next->name_size = 0;
		next->data_size = ((char *)end - ((char *)next + sizeof(*next)));
	}
	
	return 0;
}

#define START_TRANSACTION   0
#define HAVE_TRANSACTION    1

static int
write_big_attr(bfs_info *bfs, bfs_inode *bi, const char *name, int type,
			   const void *buf, size_t *len, dr9_off_t pos,
			   int have_transaction)
{
	int        err = 0;
	char      *old_data;
	size_t     old_size;
	vnode_id  attr_vnid;
	bfs_inode *attr = NULL;
	
	/* printf("creating big attr %s %d\n", name, *len); */

	if (have_transaction == 0) {
		bfs->cur_lh = start_transaction(bfs);
		if (bfs->cur_lh == NULL) {
			printf("write_big_attr: no mem for transaction (bi @ 0x%x)!\n",bi);
			return ENOMEM;
		}
	}

	/* make sure there's enough room for all bt_insert()'s to work */
	if (reserve_space(bfs, NUM_INSERT_BLOCKS) != 0) {
		if (have_transaction == 0)
			abort_transaction(bfs, bfs->cur_lh);
		return ENOSPC;
	}

	attr_vnid = block_run_to_bnum(bi->attributes);
	if (attr_vnid == 0) {
		bi->etc->attrs = make_dir(bfs, bi, S_ATTR_DIR | 0666, &attr_vnid);
		if (bi->etc->attrs) {
			bi->attributes = bi->etc->attrs->inode_num;
			update_inode(bfs, bi);
		} else {
			err = ENOSPC;
		}
	} else if (bi->etc->attrs == NULL) {
		err = bfs_read_vnode(bfs, attr_vnid, 1, (void **)&bi->etc->attrs);
	}

	if (err) {
		goto cleanup0;
	}

	if ((err = lookup(bi->etc->attrs, name, &attr_vnid)) != 0) {
		if (err != ENOENT) {
			goto cleanup0;
		}

		err = 0;
		attr = make_file(bfs, bi->etc->attrs, S_ATTR | 0666, &attr_vnid);
		if (attr != NULL) {
			inode_addr ia;

			bi->etc->attrs->etc->counter++;
			if (bt_insert(bi->etc->attrs->etc->btree, (bt_chrp)name,
						  strlen(name), attr_vnid, 0) != BT_OK){
				bt_perror(bi->etc->attrs->etc->btree, "attr_create");
				printf("*** failed to insert attr %s (vnid 0x%Lx freeblk %Ld)\n", 
              		   name, attr_vnid, bfs->dsb.num_blocks - bfs->dsb.used_blocks);

				/* force deleting the name just in case it was added in memory */
				bi->etc->attrs->etc->counter++;
				bt_delete(bi->etc->attrs->etc->btree, (bt_chrp)name, strlen(name),
						  (dr9_off_t *)&attr_vnid);
				err = ENOSPC;

				ia = attr->inode_num;
				bfs_write_vnode(bfs, attr, 0);
				free_inode(bfs, &ia);
				attr = NULL;
			} else {
				int err;

				attr->type = type;

				if ((err = new_vnode(bfs->nsid, attr_vnid, attr)) != 0)
					bfs_die("attr1: new vnode failed vnid %Ld: %s\n", attr_vnid, strerror(err));

				/* it's time to grow up little girl... */
				attr->flags &= ~INODE_VIRGIN;

				update_inode(bfs, attr);
			}
		} else {
			err = ENOSPC;
		}

		/* make sure it doesn't exist in the small_data area */
		if (err == 0 && delete_small_data(bi, name) == 0)
			update_inode(bfs, bi);

	} else {
		if ((err = get_vnode(bfs->nsid, attr_vnid, (void **)&attr)) != 0)
			goto cleanup0;

		if (attr->data.size < MAX_INDEXED_DATA_SIZE) {
			old_data = (char *)malloc(MAX_INDEXED_DATA_SIZE);
			old_size = MAX_INDEXED_DATA_SIZE;
			err = read_data_stream(bfs, attr, 0, old_data, &old_size);
			if (err == 0 && old_size != 0) {
				delete_from_index_if_needed(bfs, name, attr->type, old_data,
											old_size, bi);
			}
			free(old_data);
		}
	}
	
	if (attr) {
		/*
		   XXXdbg this is a hack to prevent write_data_stream() from starting
		   a new transaction since we already have one
		*/
		attr->flags |= NO_TRANSACTION; 

		if (err == 0)
			err = write_data_stream(bfs, attr, pos, buf, len);

		attr->flags &= ~NO_TRANSACTION;

		if (err == 0 && pos == 0) {
			/* if the attribute we just wrote is indexed, update the index */
			update_index_if_needed(bfs, name, type, buf, *len, bi);
		}
		
		update_inode(bfs, attr);
	}

	unreserve_space(bfs, NUM_INSERT_BLOCKS);

	if (have_transaction == 0) {
		if (err == 0)
			end_transaction(bfs, bfs->cur_lh);
		else
			abort_transaction(bfs, bfs->cur_lh);
	}

	if (attr)
		put_vnode(bfs->nsid, attr->etc->vnid);

	return err;


	
 cleanup0:
	unreserve_space(bfs, NUM_INSERT_BLOCKS);

	if (have_transaction == 0)
		abort_transaction(bfs, bfs->cur_lh);

	return err;
}


int
bfs_write_attr(void *ns, void *node, const char *name, int type,
			   const void *buf, size_t *len, dr9_off_t pos)
{
	int         ret = 0;
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node, *index = NULL;
	small_data *sd;

    /* printf("write attr: bi 0x%x n %s t 0x%x b 0x%x s %d pos %Ld\n", bi, 
              name, type, buf, *len, pos); */
	if (bfs->flags & FS_READ_ONLY) {
		*len = 0;
		return EROFS;
	}

	if (name == NULL || *name == '\0' || 
        strlen(name) >= B_FILE_NAME_LENGTH-1 || *name == NAME_MAGIC) {
		*len = 0;
		return EINVAL;
	}

	/* only allow the volume-id to be written once */
	if (bfs->root_dir == bi && strcmp(name, "be:volume_id") == 0) {
		if (find_small_data(bi, name))
			return EINVAL;
	}

	/* attributes that are prefixed with "sys:" are system attributes
	   and only writable by root */
	if ( (strncmp(name,"sys:",4) == 0) && (geteuid() != 0) ) {
		return EPERM;
	}
	

	/* printf("write_attr: bi 0x%x: %s type 0x%x buf 0x%x len 0x%x pos %Ld\n",
              bi, name, type, buf, *len, pos); */

	acquire_sem(bfs->sem);
	LOCK(bi->etc->lock);

	/*
	  to prevent a deadlock with the index getting flushed after we've
	  started a transaction but before we've gotten the index, we pre-fetch
	  the index before starting the transaction.  we don't really need it
	  except to lock it in memory.  if get_index() fails, we don't care,
	  we just won't do the corresponding put_vnode().

      note: we don't do this for the name, size, or last_modified time
            indices because they're always locked and loaded.
    */  
    if (strcmp(name, "name") == 0 ||
        strcmp(name, "size") == 0 ||
        strcmp(name, "last_modified") == 0 ||
        get_index(bfs, name, &index) != 0) {

        index = NULL;
	}

	/* if the attribute is small, try and put it in the small_data area */
	if (*len < (bfs->dsb.block_size - sizeof(bfs_inode))) {
		bfs->cur_lh = start_transaction(bfs);
		if (bfs->cur_lh == NULL) {
			if (index)
				put_vnode(bfs->nsid, index->etc->vnid);

			UNLOCK(bi->etc->lock);
			release_sem(bfs->sem);

			printf("write_attr: can't start transaction (bi @ 0x%x)!\n", bi);
			*len = 0;
			
			return ENOMEM;
		}
		
		sd = find_small_data(bi, name);
		if (sd == NULL) {
			ret = create_small_data(bi, name, type, buf, len);
		} else {
			delete_from_index_if_needed(bfs, name, sd->type, SD_DATA(sd),
										sd->data_size, bi);
		
			if (*len != sd->data_size) {
				delete_small_data(bi, name);
				ret = create_small_data(bi, name, type, buf, len);
			} else {
				update_small_data(sd, buf);
			}
		}

		if (ret == 0) {
			remove_big_attr(bfs, bi, name);   /* make sure it's not here */
			
			/* if the attribute we just wrote is indexed, update the index */
			update_index_if_needed(bfs, name, type, buf, *len, bi);

			update_inode(bfs, bi);
			end_transaction(bfs, bfs->cur_lh);
		} else {
			abort_transaction(bfs, bfs->cur_lh);
		}

		if (ret == ENOSPC) {
			ret = write_big_attr(bfs, bi, name, type, buf, len, pos,
								 START_TRANSACTION);
		}
	} else {
		ret = write_big_attr(bfs, bi, name, type, buf, len, pos,
							 START_TRANSACTION);
	}


	notify_listener(B_ATTR_CHANGED, bfs->nsid, 0, 0, bi->etc->vnid, name);

	if (index)
		put_vnode(bfs->nsid, index->etc->vnid);

	UNLOCK(bi->etc->lock);
	release_sem(bfs->sem);
	
	if (ret < 0)
		*len = 0;

	return ret;
}


static int
read_big_attr(bfs_info *bfs, bfs_inode *bi, const char *name, int type,
			  void *buf, size_t *len, dr9_off_t pos)
{
	int        err = 0;
	vnode_id  attr_vnid;
	bfs_inode *attr = NULL;
	
	if (bi->etc->attrs == NULL) {
		attr_vnid = block_run_to_bnum(bi->attributes);
		if (attr_vnid == 0) {
			return ENOENT;
		}

		err = bfs_read_vnode(bfs, attr_vnid, 1, (void **)&bi->etc->attrs);
		if (err)
			return err;
	}

	if ((err = lookup(bi->etc->attrs, name, &attr_vnid)) != 0)
		return err;

	if ((err = get_vnode(bfs->nsid, attr_vnid, (void **)&attr)) != 0)
		return err;

	if (err == 0 && attr)
		err = read_data_stream(bfs, attr, pos, buf, len);

	if (attr)
		put_vnode(bfs->nsid, attr->etc->vnid);

	return err;
}



/*
   this is an internal version of read_attr that does no
   locking (that's left up to the caller to handle.  this
   allows us to re-enter the attribute code when a file is
   already locked for other reasons.
*/   
int
internal_read_attr(bfs_info *bfs, bfs_inode *bi, const char *name, int type,
				   void *buf, size_t *len, dr9_off_t pos)
{
	int       ret;

	ret = read_small_data(bi, name, buf, len, type);
	if (ret == ENOENT)
		ret = read_big_attr(bfs, bi, name, type, buf, len, pos);

	if (ret < 0)
		*len = 0;

	return ret;
}


int
bfs_read_attr(void *ns, void *node, const char *name, int type,
			  void *buf, size_t *len, dr9_off_t pos)
{
	int       ret;
	bfs_info *bfs  = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;

	LOCK(bi->etc->lock);

    /* printf("read_attr bi 0x%x: %s\n", bi, name); */

	ret = read_small_data(bi, name, buf, len, type);
	if (ret == ENOENT)
		ret = read_big_attr(bfs, bi, name, type, buf, len, pos);

	UNLOCK(bi->etc->lock);
	
	if (ret < 0)
		*len = 0;

	return ret;
}

static int
remove_big_attr(bfs_info *bfs, bfs_inode *bi, const char *name)
{
	int        err = 0;
	char      *old_data;
	size_t     old_size = 0;
	vnode_id  attr_vnid = BT_ANY_RRN;
	bfs_inode *attr = NULL;

	/* printf("deleting big attr %s\n", name); */
	
	if (bi->etc->attrs == NULL) {
		attr_vnid = block_run_to_bnum(bi->attributes);
		if (attr_vnid == 0)
			return ENOENT;

		err = bfs_read_vnode(bfs, attr_vnid, 1, (void **)&bi->etc->attrs);
		if (err)
			return err;
	}


	bi->etc->attrs->etc->counter++;
	if (bt_delete(bi->etc->attrs->etc->btree, (bt_chrp)name, strlen(name),
				  (dr9_off_t *)&attr_vnid) != BT_OK) {
		return ENOENT;
	}


	if ((err = get_vnode(bfs->nsid, attr_vnid, (void **)&attr)) != 0)
		return err;
	
	if (attr->data.size <= MAX_INDEXED_DATA_SIZE) {
		old_data = (char *)malloc(MAX_INDEXED_DATA_SIZE);
		old_size = MAX_INDEXED_DATA_SIZE;
		err = read_data_stream(bfs, attr, 0, old_data, &old_size);
		if (err == 0 && old_size != 0) {
			delete_from_index_if_needed(bfs, name, attr->type, old_data,
										old_size, bi);
		}
		free(old_data);
	}

	remove_vnode(bfs->nsid, attr_vnid);
	put_vnode(bfs->nsid, attr_vnid);


 cleanup0:
	return err;
}


int
bfs_remove_attr(void *ns, void *node, const char *name)
{
	int         ret = ENOENT;
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	small_data *sd;

	if (*name == NAME_MAGIC)
		return EINVAL;

	/* attributes that are prefixed with "sys:" are system attributes
	   and only writable by root */
	if ( (strncmp(name,"sys:",4) == 0) && (geteuid() != 0) ) {
		return EPERM;
	}
	
	/* can't delete the vol-id attr on the root directory */
	if (bfs->root_dir == bi && strcmp(name, "be:volume_id") == 0)
		return EINVAL;

	if (bfs->flags & FS_READ_ONLY)
		return EROFS;

	/* printf("remove_attr bi 0x%x: %s\n", bi, name); */
	
	acquire_sem(bfs->sem);
	LOCK(bi->etc->lock);

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL) {
		UNLOCK(bi->etc->lock);
		release_sem(bfs->sem);
		printf("remove_attr: can't start transaction (bi @ 0x%x)!\n", bi);
		return ENOMEM;
	}

	sd = find_small_data(bi, name);
	if (sd != NULL) {
		delete_from_index_if_needed(bfs, name, sd->type, SD_DATA(sd),
									sd->data_size, bi);
		
		delete_small_data(bi, name);
		update_inode(bfs, bi);
		ret = 0;
	} else {
		ret = remove_big_attr(bfs, bi, name);
	}

	end_transaction(bfs, bfs->cur_lh);
	UNLOCK(bi->etc->lock);
	release_sem(bfs->sem);
	
	notify_listener(B_ATTR_CHANGED, bfs->nsid, 0, 0, bi->etc->vnid, name);

	return ret;
}


static int
stat_big_attr(bfs_info *bfs, bfs_inode *bi, const char *name,
			  struct attr_info *ai)
{
	int        err = 0;
	vnode_id  attr_vnid;
	bfs_inode *attr = NULL;
	
	if (bi->etc->attrs == NULL) {
		attr_vnid = block_run_to_bnum(bi->attributes);
		if (attr_vnid == 0) {
			return ENOENT;
		}

		err = bfs_read_vnode(bfs, attr_vnid, 1, (void **)&bi->etc->attrs);
		if (err)
			return err;
	}

	if ((err = lookup(bi->etc->attrs, name, &attr_vnid)) != 0)
		return err;

	if ((err = get_vnode(bfs->nsid, attr_vnid, (void **)&attr)) != 0)
		return err;
	

	if (err == 0 && attr) {
		ai->type = attr->type;
		ai->size = attr->data.size;
		put_vnode(bfs->nsid, attr->etc->vnid);
	}

	return err;
}


int
bfs_stat_attr(void *ns, void *node, const char *name,
			  struct attr_info *buf)
{
	int ret = ENOENT;
	bfs_info   *bfs = (bfs_info *)ns;
	bfs_inode  *bi  = (bfs_inode *)node;
	small_data *sd;

	LOCK(bi->etc->lock);
	
	sd = find_small_data(bi, name);
	if (sd != NULL) {
		buf->type = sd->type;
		buf->size = sd->data_size;
		ret = 0;
	} else {
		ret = stat_big_attr(bfs, bi, name, buf);
	}
	
	UNLOCK(bi->etc->lock);

	return ret;
}

int
internal_stat_attr(bfs_info *bfs, bfs_inode *bi, const char *name,
				   struct attr_info *buf)
{
	int ret = ENOENT;
	small_data *sd;

	sd = find_small_data(bi, name);
	if (sd != NULL) {
		buf->type = sd->type;
		buf->size = sd->data_size;
		ret = 0;
	} else {
		ret = stat_big_attr(bfs, bi, name, buf);
	}
	
	return ret;
}


typedef struct attr_dir
{
	int        counter, in_attr_dir;
	char       name[B_FILE_NAME_LENGTH];
} attr_dir;

int
bfs_open_attrdir(void *ns, void *node, void **cookie)
{
	dr9_off_t  bnum;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	attr_dir  *ad;

	ad = (attr_dir *)malloc(sizeof(*ad));
	if (ad == NULL)
		return ENOMEM;


	ad->counter     = 0;
	ad->name[0]     = '\0';
	ad->in_attr_dir = 0;

	LOCK(bi->etc->lock);

	bnum = block_run_to_bnum(bi->attributes);
	if (bnum != 0 && bi->etc->attrs == NULL) {
		int err;
		
		err = bfs_read_vnode(bfs, bnum, 1, (void **)&bi->etc->attrs);
		if (err != 0) {
			free(ad);
			UNLOCK(bi->etc->lock);
			return err;
		}
	}

	UNLOCK(bi->etc->lock);

	*cookie = (void *)ad;

	return 0;
}


int
bfs_close_attrdir(void *ns, void *node, void *cookie)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	attr_dir  *ad  = (attr_dir *)cookie;

	ad->counter     = 0;
	ad->name[0]     = '\0';
	ad->in_attr_dir = 0;

	return 0;
}

int
bfs_free_attr_dircookie(void *ns, void *node, void *cookie)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	attr_dir  *ad  = (attr_dir *)cookie;
	
	free(ad);

	return 0;
}


int
bfs_rewind_attrdir(void *ns, void *node, void *cookie)
{
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	attr_dir  *ad  = (attr_dir *)cookie;

	ad->counter     = 0;
	ad->name[0]     = '\0';
	ad->in_attr_dir = 0;

	return 0;
}

int
bfs_read_attrdir(void *ns, void *node, void *cookie, long *num,
				 struct dirent *buf, size_t bufsize)
{
	int        nlen;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *bi  = (bfs_inode *)node;
	attr_dir  *ad  = (attr_dir *)cookie;

	LOCK(bi->etc->lock);

 restart:

	if (ad->in_attr_dir == 0) {
		small_data *sd, *end;

		sd  = (small_data *)&bi->small_data[0];
		end = (small_data *)((char *)bi + bi->inode_size);
		
		if (ad->counter == 0 && ad->name[0] == '\0') {

			for(; ((char *)sd + sizeof(*sd)) < ((char *)end);
				sd = NEXT_SD(sd)) {
				if (sd->name[0] != '\0' && sd->name[0] != NAME_MAGIC)
					break;
			}

			if (((char *)sd + sizeof(*sd)) >= ((char *)end)) {
				ad->name[0] = '\0';
				ad->counter = 0;
				
				ad->in_attr_dir = 1;
				goto restart;
			}
			
			buf->d_ino = ++ad->counter;
			buf->d_reclen = sd->name_size;
			strncpy(buf->d_name, sd->name, sd->name_size);
			buf->d_name[sd->name_size] = 0;

			strncpy(ad->name, sd->name, sd->name_size);
			ad->name[sd->name_size] = 0;

			*num = 1;
			
			UNLOCK(bi->etc->lock);

			return 0;
		}

		/* search for the current name and go one beyond it */
		for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
			if (sd->name[0] != '\0' && sd->name[0] != NAME_MAGIC) {
				int nlen = strlen(ad->name), maxlen;
			    maxlen = (nlen > sd->name_size) ? nlen : sd->name_size;
				if (strncmp(sd->name, ad->name, maxlen) == 0) {
					sd = NEXT_SD(sd);
					break;
				}
			}
		}		
		
		/* now skip all the intervening free small_data's */
		for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
			if (sd->name[0] != '\0' && sd->name[0] != NAME_MAGIC)
				break;
		}

		/* at this point we either point to a valid small_data or we're
		   beyond the end of the small_data area */
		if (((char *)sd + sizeof(*sd)) >= ((char *)end)) {
			if (bi->etc->attrs) {
				ad->name[0] = '\0';
				ad->counter = 0;
				
				ad->in_attr_dir = 1;
				goto restart;
			}

			*num = 0;
		} else {
			buf->d_ino = ++ad->counter;
			buf->d_reclen = sd->name_size;
			strncpy(buf->d_name, sd->name, sd->name_size);
			buf->d_name[sd->name_size] = '\0';

			strncpy(ad->name, sd->name, sd->name_size);
			ad->name[sd->name_size] = '\0';

			*num = 1;
		}
	} else {                       /* we're in the attribute directory */
		int      llen, err = 0;
		vnode_id attr_vnid = BT_ANY_RRN;

		if (bi->etc->attrs == NULL) {
			*num = 0;
			UNLOCK(bi->etc->lock);
			return 0;
		}


		if (ad->name[0] == '\0') {

			bi->etc->attrs->etc->counter++;
			if (bt_goto(bi->etc->attrs->etc->btree, BT_BOF) != BT_OK) {
				bt_perror(bi->etc->attrs->etc->btree, "dir: read: goto");
				UNLOCK(bi->etc->lock);
				return ENOTDIR;
			}

			ad->counter = bi->etc->attrs->etc->counter;
		}
		
		if (ad->counter != bi->etc->attrs->etc->counter) {
			bi->etc->attrs->etc->counter++;
			if (bt_find(bi->etc->attrs->etc->btree, (bt_chrp)ad->name,
						strlen(ad->name), (bt_off_t *)&attr_vnid) == BT_ERR) {
				bt_perror(bi->etc->attrs->etc->btree, "readdir reset");
				printf("read_attr_dir: error searching for %s (bi 0x%lx)!\n",
					   ad->name, bi);
			}
		}

		llen = bufsize - sizeof(*buf) - 1;
		bi->etc->attrs->etc->counter++;
		err = bt_traverse(bi->etc->attrs->etc->btree, BT_EOF,
						  (bt_chrp)&buf->d_name[0],
						  llen, &llen, (dr9_off_t *)&buf->d_ino);
	
		ad->counter = bi->etc->attrs->etc->counter;
		

		if (err == BT_EOF) {
			*num = 0;
		} else if (err != BT_OK) {
			bt_perror(bi->etc->attrs->etc->btree, "attr_dir: read: traverse");
			UNLOCK(bi->etc->lock);
			return ENOTDIR;
		} else if (err == BT_OK) {
			buf->d_name[llen] = '\0';  /* have to null terminate it */
			buf->d_reclen = llen;
			*num = 1;

			strcpy(ad->name, &buf->d_name[0]);
		}
	}

	UNLOCK(bi->etc->lock);

	return 0;
}


int
free_attributes(bfs_info *bfs, bfs_inode *bi)
{
	int        err = 0, llen;
	vnode_id  attr_vnid;
	bfs_inode *attr = NULL;
	bfs_inode *dir;
	char       name[B_FILE_NAME_LENGTH];
	char      *old_data;
	size_t     old_size = 0;
	small_data *sd, *end;
	
	sd  = (small_data *)&bi->small_data[0];
	end = (small_data *)((char *)bi + bi->inode_size);

	/*
	   first iterate over all the small data nodes and delete them from
	   any indices they may be in
	*/
	for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
		if (sd->name[0] != '\0' && sd->name[0] != NAME_MAGIC) {
			delete_from_index_if_needed(bfs, sd->name, sd->type, SD_DATA(sd),
										sd->data_size, bi);
		}
	}		
		

	if (bi->etc->attrs == NULL) {
		attr_vnid = block_run_to_bnum(bi->attributes);
		if (attr_vnid == 0) {
			return ENOENT;
		}

		err = bfs_read_vnode(bfs, attr_vnid, 1, (void **)&bi->etc->attrs);
		if (err)
			return err;

	}

	dir = bi->etc->attrs;
	
	dir->etc->counter++;
	if (bt_goto(dir->etc->btree, BT_BOF) != BT_OK) {
		bfs_remove_vnode(bfs, dir, 1);
		return 0;
	}

	while(1) {
		llen = sizeof(name) - 1;
		dir->etc->counter++;
		err = bt_traverse(dir->etc->btree, BT_EOF, (bt_chrp)name,
						  llen, &llen, (dr9_off_t *)&attr_vnid);
		if (err != BT_OK)
			break;
		name[llen] = '\0';
		
		if ((err = get_vnode(bfs->nsid, attr_vnid, (void **)&attr)) != 0) {
			printf("error deleting attribute %s from bi @ 0x%x\n",
				   name, bi);
			continue;
		}

		dir->etc->counter++;
		if (bt_delete(dir->etc->btree, (bt_chrp)name, llen,
					  (dr9_off_t *)&attr_vnid) != BT_OK) {
			printf("bi 0x%x: error deleting attr %s in free_attributes\n",
				   bi, name);
		}


		if (attr->data.size <= MAX_INDEXED_DATA_SIZE) {
			old_data = (char *)malloc(MAX_INDEXED_DATA_SIZE);
			old_size = MAX_INDEXED_DATA_SIZE;
			err = read_data_stream(bfs, attr, 0, old_data, &old_size);
			if (err == 0 && old_size != 0) {
				delete_from_index_if_needed(bfs, name, attr->type, old_data,
											old_size, bi);
			}
			free(old_data);
		}

		remove_vnode(bfs->nsid, attr_vnid);
		put_vnode(bfs->nsid, attr_vnid);
	}

	/* this deletes the attribute directory */
	bfs_remove_vnode(bfs, dir, 1);
	

	return 0;
}


/*
   this tries to free up space in the small data area by deleting
   the largest attribute to make space for "len" bytes of a new guy.
*/   
static int
kick_out_some_other_wanker(bfs_info *bfs, bfs_inode *bi, int len)
{
	int         max = -1, err;
	char        name[B_FILE_NAME_LENGTH], *data;
	size_t      data_len;
	small_data *sd, *end, *big_dude = NULL;

	sd  = (small_data *)&bi->small_data[0];
	end = (small_data *)((char *)bi + bi->inode_size);


	/* find the biggest guy in the small_data area and force him out */
	for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
		if (sd->name[0] != 0 && max < sd->name_size + sd->data_size) {
			big_dude = sd;
			max      = sd->name_size + sd->data_size;
		}
	}		

	if (big_dude == NULL)
		bfs_die("remove_other_wanker: no big dude to delete?! bi 0x%x\n", bi);

	sd = big_dude;

	strncpy(name, sd->name, sd->name_size);
	name[sd->name_size] = '\0';

	/*
	   have to copy this to the side because write_big_attr() will first
	   delete it from the small_data area (which would trash our pointers)
	   and then write_big_attr writes out the data to a separate attribute
	   file
	*/   
	data = (char *)malloc(sd->data_size);
	if (data == NULL)
		return ENOMEM;

	memcpy(data, SD_DATA(sd), sd->data_size);

	data_len = sd->data_size;
	err = write_big_attr(bfs, bi, name, sd->type, data, &data_len, 0,
						 HAVE_TRANSACTION);

	if ((big_dude = find_small_data(bi, name)) != NULL) {
		printf("oddly enough write_big_attr didn't delete %s\n", name);
		if ((err = delete_small_data(bi, name)) != 0)
			printf("delete_small_data returned %s\n", strerror(err));
	}
	
	free(data);

	return err;
}



static const char name_name[2] = { NAME_MAGIC, 0 };


/*
   any necessary transactions should have been started before
   calling this function.
*/   
int
create_name_attr(bfs_info *bfs, bfs_inode *bi, const char *name)
{
	int         i = 0;
	size_t      len = strlen(name);
	small_data *sd;
	
	sd = find_small_data(bi, name_name);
	if (sd != NULL) {
		printf("warning: bi @ 0x%x (%s) already has a name %s!\n",
			   bi, name, SD_DATA(sd));
		return 0;
	}

	while (create_small_data(bi,name_name,B_STRING_TYPE,name,&len) == ENOSPC) {
		kick_out_some_other_wanker(bfs, bi, len);

		if (i++ > bi->inode_size / sizeof(small_data))
			break;
	}

	if (i++ > bi->inode_size / sizeof(small_data))
		bfs_die("create_name_attr: no space for name %s in inode?!?\n", name);
	
	return 0;
}


int
get_name_attr(bfs_info *bfs, bfs_inode *bi, char *buf, int len)
{
	int         max;
	small_data *sd;

	sd = find_small_data(bi, name_name);
	if (sd == NULL) {
		return ENOENT;
	}

	if (*(char *)SD_DATA(sd) == '\0')  /* then this guy is dead, don't touch */
		return ENOENT;

	max = sd->data_size;
	if (max >= len)
		max = len - 1;
	memcpy(buf, SD_DATA(sd), max);
	buf[max] = '\0';

	return 0;
}

int
kill_name_attr(bfs_info *bfs, bfs_inode *bi)
{
	small_data *sd;

	sd = find_small_data(bi, name_name);
	if (sd == NULL) {
		return ENOENT;
	}

	*(char *)SD_DATA(sd) = '\0';

	return 0;
}

/*
   any necessary transactions should have been started before
   calling this function.
*/   
int
change_name_attr(bfs_info *bfs, bfs_inode *bi, const char *newname)
{
	int         i = 0;
	size_t      len = strlen(newname);
	small_data *sd;
	
	if (delete_small_data(bi, name_name) != 0) {
		printf("bfs warning: bi @ 0x%x has no name in small_data\n", bi);
	}

	while (create_small_data(bi,name_name,B_STRING_TYPE,newname,&len) == ENOSPC) {
		kick_out_some_other_wanker(bfs, bi, len);

		if (i++ > bi->inode_size / sizeof(small_data))
			break;
	}

	if (i++ > bi->inode_size / sizeof(small_data))
		bfs_die("change_name_attr: no space for name %s in inode?\n",newname);
	
	return 0;
}


#ifdef DEBUG
#ifndef USER
int
dump_small_data(int argc, char **argv)
{
	int         i;
	char        name[B_FILE_NAME_LENGTH];
	bfs_inode  *bi;
	small_data *sd, *end;

	if (argc == 1) {
		kprintf("usage: sdata <inode-addr>\n");
		return 0;
	}

	for(i=1; i < argc; i++) {
		bi = (bfs_inode *)parse_expression(argv[i]);
		if (bi == NULL)
			continue;
		
		if (bi->magic1 != INODE_MAGIC1 || bi->data.size < 0 || bi->etc ==NULL){
			kprintf("inode @ 0x%x looks suspicious. skipping it.\n", bi);
			continue;
		}

		sd  = (small_data *)&bi->small_data[0];
		end = (small_data *)((char *)bi + bi->inode_size);
		
		for(; ((char *)sd + sizeof(*sd)) < ((char *)end); sd = NEXT_SD(sd)) {
			if (sd->data_size + sd->name_size > bi->inode_size) {
				kprintf("sd @ 0x%x looks bad (%d %d)\n", sd, sd->name_size,
					   sd->data_size);
				break;
			}

			if (sd->name[0] == '\0') {
				kprintf("sd @ 0x%x: is free; size == %d\n", sd, sd->data_size);
				continue;
			} else if (sd->name[0] == NAME_MAGIC) {
				strcpy(name, "_NAME_");
			} else {
				memcpy(name, sd->name, sd->name_size);
				name[sd->name_size] = '\0';
			}
		
			kprintf("sd 0x%x: name %32s type 0x%x size %d\n",
					sd, name, sd->type, sd->data_size);
		}
	}
		
	return 0;	
}
#endif /* USER */
#endif /* DEBUG */
