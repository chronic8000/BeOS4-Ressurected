#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include <KernelExport.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

int 
cfs_walk(void *ns, void *base, const char *file, char **newpath, vnode_id *vnid)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *dir = (cfs_node *)base;
	off_t prev_vnid, next_vnid;

	if(dir == NULL) {
		PRINT_INTERNAL_ERROR(("cfs_walk: no base directory\n"));
		return B_ERROR;
	}

	err = cfs_lock_read(fsinfo);
	if(err < B_NO_ERROR) {
		return err;
	}

	if (!S_ISDIR(dir->flags)) {
		PRINT_FLOW(("cfs_walk: base directory is not a directory\n"));
		err = ENOTDIR;
		goto err;
	}
	
	if(strcmp(file, ".") == 0) {
		*vnid = dir->vnid;
		goto found_entry;
	}
	if(strcmp(file, "..") == 0) {
		if(dir->parent == 0) {
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		*vnid = dir->parent;
		goto found_entry;
	}
	
	err = cfs_lookup_dir(fsinfo, dir, file, &prev_vnid, vnid, &next_vnid);
	if(err != B_NO_ERROR)
		goto err;
found_entry:
	{
		cfs_node *node;
		//dprintf("cfs_walk: found entry %s\n", file);
		err = get_vnode(fsinfo->nsid, *vnid, (void **)&node);
		if(err != B_NO_ERROR)
			goto err;

		if(S_ISLNK(node->flags)) {
			//dprintf("cfs_walk: found link %s, newpath %p\n", file, newpath);
		}
		if(S_ISLNK(node->flags) && newpath) {
			char *np;
			err = new_path(node->u.link.link, &np);
			put_vnode(fsinfo->nsid, *vnid);
			if(!err)
				*newpath = np;
			goto err;
		}
	}
	err = B_NO_ERROR;
err:
	cfs_unlock_read(fsinfo);
	return err;
}
