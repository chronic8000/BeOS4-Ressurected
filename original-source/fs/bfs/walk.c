#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bfs.h"
#include "btlib/btree.h"


int
bfs_walk(void *ns, void *_dir, const char *file, char **newpath,vnode_id *vnid)
{
	int        err = 0;
	bfs_info  *bfs = (bfs_info *)ns;
	bfs_inode *dir, *entry;
	char      *np;

	dir = (bfs_inode *)_dir;

	CHECK_INODE(dir);

	acquire_sem(bfs->sem);
	LOCK(dir->etc->lock);

	// Check the execute bit
	{
		uid_t uid = geteuid();		

		if(uid != 0) {
			mode_t mode = dir->mode;
			gid_t gid = getegid();
						
			if (uid == dir->uid)
				mode >>= 6;
			else if (gid == dir->gid)
				mode >>= 3;
					
			if((mode & S_IXOTH) != S_IXOTH) {
				err = EACCES;
				goto done;
			}
		}	
	}

	if (lookup(dir, file, vnid) != 0) {
		err = ENOENT;
		goto done;
	}

    if (*vnid <= 0 || *vnid >= bfs->dsb.num_blocks) {
		printf("walk: dir 0x%x file %s bad vnid %Ld?!? (max == %Ld)\n",
			   dir, file, *vnid, bfs->dsb.num_blocks);
		return ENOENT;
 	}

	entry = NULL;
	if (get_vnode(bfs->nsid, *vnid, (void **)&entry) != 0) {
		err = ENOENT;
		goto done;
	}

	if (entry == NULL) {
		/* bfs_die("hey! the kernel is lying to me! (vnid 0x%Lx = NULL)\n", *vnid); */
		printf("hey! the kernel is lying to me! (vnid 0x%Lx = NULL)\n", *vnid);
		err = ENOENT;
		goto done;
	}

	/* XXXdbg -- if it's a link we have to do special stuff */
	if (S_ISLNK(entry->mode) && newpath) {
		if (entry->flags & LONG_SYMLINK) {
			char *p;
			size_t len = entry->data.size;

			p = (char *)malloc(entry->data.size);
			if (p == NULL) {
				err = ENOMEM;
				put_vnode(bfs->nsid, *vnid);
				goto done;
			}

			*p = '\0';
			err = read_data_stream(bfs, entry, 0, p, &len);
			if (err == 0 && len == entry->data.size)
				err = new_path(p, &np);
			free(p);
		} else {
			err = new_path((char *)&entry->data, &np);
		}

		put_vnode(bfs->nsid, *vnid);

		if (err) {
			goto done;
		}
			
		*newpath = np;
	}

 done:
	UNLOCK(dir->etc->lock);
	release_sem(bfs->sem);

	return err;
}
