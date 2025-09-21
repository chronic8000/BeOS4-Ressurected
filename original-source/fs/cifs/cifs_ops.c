
#include "cifs_globals.h"
#include "vcache.h"
#include "cifs.h"
#include "dir_entry.h"
#include "create_file.h"
#include "create_dir.h"
#include "rename.h"
#include "trans2.h"
#include "attr.h"
#include "worker.h"
#include "mount.h"
#include "read.h"
#include "write.h"
#include "cifs_utils.h"
#include <lock.h>

//#define DBG_VNODE_OPS (CIFS_DEBUG_BASE+1)
#define DBG_VNODE_OPS (CIFS_DEBUG_BASE-1)

bone_socket_info_t *gSock = 0;

// Forward decls
status_t form_vnode(nspace* vol, vnode* dir_vnode, char* filename, vnode_id this_vnid, void** _node);
status_t form_root_vnode(nspace* vol, vnode* root_vnode);

status_t check_cookie(nspace* vol, vnode* node, void *cookie);

 
status_t
cifs_mount(nspace_id nsid, const char* device, ulong flags, void *_parms,
				size_t len, void **data, vnode_id *vnid) {

	parms_t*	parms = (parms_t*)_parms;
	status_t	err = B_ERROR;
	nspace*		vol = NULL;
	vnode_id 	this_vnid;
	char name[32];

	if(gSock == 0)
	{
		status_t rc;
		rc = get_module("network/bone_socket", (module_info **) &gSock);
		if(rc != B_NO_ERROR || gSock == 0)
			return rc;
	}
	
	
#ifdef DEBUG
	load_driver_symbols("cifs");
#endif

	//dprintf("CIFS client : mounting\n");

	if (device != NULL) {
		debug(-1, ("cifs_mount called with a device! ->%s<-", device));
		return EINVAL;
	}
	
	MALLOC(vol, nspace*, sizeof(struct nspace));
	if (vol == NULL) {
		err = ENOMEM;
		goto exit1;
	}

	*data = vol;
	vol->nsid = nsid;
	
	err = copy_parms(vol, parms);
	if (err != B_OK) goto exit2;

	err = InitNetworkConnection(vol);
	if (err != B_OK) {
		debug(-1, ("InitNetworkConnectionFailed\n"));
		goto exit2;
	}
	
	err = init_vcache(vol);
	if (err != B_OK) {
		debug(-1, ("couldn't alloc vcache.\n"));
		goto exit3;
	}
	
	err = form_root_vnode(vol, & vol->root_vnode);
	if (err != B_OK) {
		debug(-1, ("problem forming root vnode.\n"));
		goto exit4;
	}
	
	*vnid = vol->root_vnode.vnid;
	if (vol->Capabilities & CAP_RAW_MODE) {
		vol->io_size = min(vol->MaxRawBufferSize, 65535);
		vol->small_io_size = min((vol->MaxBufferSize - 100), 4096);
	} else {
		vol->io_size = min(vol->MaxBufferSize - 100, 16384); // XXX
		vol->small_io_size = min((vol->MaxBufferSize - 100), 4096);
	}
		
	//debug(-1,("Cifs io_size is %d\n", vol->io_size));	
	//debug(-1,("\t small_io_size is %d\n", vol->small_io_size));

	//dprintf("Disabling read cache\n");
	vol->cache_enabled = false;	

	sprintf(name, "cifs lock %x", vol->nsid);
	if ((err = new_lock(&(vol->vlock), name)) != B_OK) {
		debug(-1, ("couldnt create volume lock.\n"));
		goto exit4;
	}
	
	err = new_vnode(nsid, vol->root_vnode.vnid, & vol->root_vnode);
	if (err != B_OK) {
		debug(-1, ("cifs_mount - problem making root vnode.\n"));
		goto exit4;
	}
	
	
	vol->last_traffic_time = system_time();	
	vol->socket_state = B_OK;
	
	return B_OK;
	
exit4:
	uninit_vcache(vol);
exit3:
	KillNetworkConnection(vol);
exit2:
	FREE(vol);	
exit1:
	debug(-1,("cifs_mount ... exiting with errors.\n"));
	return err;
	
}

int cifs_access(void *_vol, void *_node, int mode) {
	
	int result = 0;
	nspace *vol = (nspace*)_vol;
	vnode *node = (vnode*)_node;
	
	/* XXXbjs - should this be inside the lock alfred? */
	if (check_access(node, mode)) 
		result = EACCES;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_access called on %s\n", node->name));
	
	debug(DBG_VNODE_OPS, ("cifs_access leaving with result %d\n", result));

	UNLOCK_VOL(vol);
	return B_OK;
}


status_t
form_root_vnode(nspace* vol, vnode* root_vnode)
{
	status_t err;
	
	debug(DBG_VNODE_OPS, ("form_root_vnode ... entering.\n"));
		
	if (root_vnode == NULL) {
		debug(-1, ("WOAH root_vnode is null in form_root_vnode, back up jack.\n"));
		return B_ERROR;
	}

	
	root_vnode->vnid = ARTIFICIAL_VNID_BITS;
	root_vnode->parent_vnid = ARTIFICIAL_VNID_BITS;
	root_vnode->mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
	root_vnode->mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	root_vnode->mode |= S_IXUSR | S_IXGRP | S_IXOTH;

	strcpy(root_vnode->name, "\\");
		
	root_vnode->contents = NULL;	
	err = update_entries(vol, root_vnode, true);
	if (err != B_OK) {
		debug(-1, ("failed on update_entries on root dir.\n"));
		return err;
	}
	
	err = add_to_vcache(vol, root_vnode->vnid, root_vnode->name,
						root_vnode->vnid, true);

	debug(DBG_VNODE_OPS, ("form_root_vnode ... exiting with %s\n", strerror(err)));
	return err;
}



status_t
cifs_unmount(void *_vol) {

	int			result = B_NO_ERROR;
	nspace*		vol = (nspace*)_vol;
	
	int32		dummy;
	
	debug(-1, ("cifs_unmount ... entering.\n"));

#if 0
	acquire_sem(vol->worker_sem);
	delete_sem(vol->worker_sem);
	wait_for_thread(vol->worker_thread, &dummy);
#endif
	
	uninit_vcache(vol);
	KillNetworkConnection(vol);
	free_lock(&(vol->vlock));
	FREE(vol);

#ifdef DEBUG
	dprintf("cifs checking memory...\n");
	check_mem();
#endif
	
	debug(-1, ("cifs_ummount ... exiting.\n"));
	return result;
}



status_t do_opendir(void *_vol, void *_node, void **_cookie)
{
	status_t err;
	nspace*			vol = (nspace*)_vol;
	vnode*			node = (vnode*) _node;
	dircookie*		cookie;
	
	debug(DBG_VNODE_OPS, ("cifs_opendir ... entering for ->%s<-\n", node->name));

	if (! (node->mode & S_IFDIR)) {
		debug(-1, ("cifs_opendir error: vnode not a directory\n"));
		err = ENOTDIR;
		goto exit1;
	}
	
	MALLOC(cookie, dircookie*, sizeof(dircookie));
	if (cookie == NULL) {
		debug(-1, ("cifs_opendir error : nomem\n"));
		err = ENOMEM;
		goto exit1;
	}
	cookie->index = 0;
	cookie->previous = ARTIFICIAL_VNID_BITS - 1;
	cookie->omode = 0;

#ifdef DELAY_OPENDIR
	cookie->index = -3;
#else
	cookie->index = -2;
	err = update_entries(vol, node, true);
	if (result != B_OK) {
		debug(-1, ("update_entries failed in opendir\n"));
		FREE(cookie);
		*_cookie = NULL;
		goto exit1;
	}
	cookie->next = node->dir_contents;

#endif

	*_cookie = (void*)cookie;	
	err = B_OK;

exit1:
	debug(DBG_VNODE_OPS, ("cifs_opendir ... exiting\n"));
	return err;
}

status_t
cifs_opendir(void *_vol, void *_node, void **_cookie)
{
	status_t err;
	nspace *vol = (nspace*)_vol;
	
	LOCK_VOL(vol);
	err = do_opendir(_vol, _node, _cookie);
	UNLOCK_VOL(vol);
	return err;
}


int cifs_readdir(void *_vol, void *_dir, void *_cookie, long *num,
					struct dirent *entry, size_t bufsize) {
// Opendir sets the cookie index to -2.  We'll increment, and get that entry's info.
	int			result = ENOENT;
	nspace*		vol = (nspace*)_vol;
	vnode*		dir = (vnode*)_dir;
	dircookie*	cookie = (dircookie*)_cookie;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_readdir ... entering on ->%s<- index: %d\n", dir->name, cookie->index));

#ifdef DELAY_OPENDIR
	if (cookie->index == -3) {
		cookie->index = -2;
		cookie->previous = ARTIFICIAL_VNID_BITS - 1;
		result = update_entries(vol, dir, true);
		if (result != B_OK) {
			debug(-1, ("update_entries failed in readdir\n"));
			goto exit1;
		}
		//dprintf("cifs_readdir : just performed update on %s", dir->full_name);
	}
#endif	

	if (dir->contents == NULL) {
		// For some reason, dir->contents has gone away.  So update it.
		if ((result = update_entries(vol,dir,true)) != B_OK) {
			debug(-1, ("cifs : readdir given NULL dir or dir->contents and cant update\n"));
			goto exit1;
		}
	}
		


	// how to we add this into the new skiplist stuff?
	if ((cookie->index + 1) > NumInSL(dir->contents)) {
		*num = 0;
		result = B_OK;
		goto exit1;
	}

	if (cookie->index == -2) {
		strcpy(entry->d_name, ".");
		entry->d_ino = dir->vnid;
		entry->d_reclen = 2;
		entry->d_dev = vol->nsid;
		*num = 1;
		cookie->index++;
		result = B_OK;
		goto exit1;
	} else if (cookie->index == -1) {
		strcpy(entry->d_name, "..");
		entry->d_ino = dir->vnid;
		entry->d_reclen = 3;
		entry->d_dev = vol->nsid;
		*num = 1;
		cookie->index++;
		result = B_OK;
		goto exit1;
	}
	
	
	result = get_entry(vol, dir, entry, cookie);
	
	if (result == B_OK) {
		*num = 1;
		//dprintf("returning info on ->%s<-\n", entry->d_name);
	} else {
		*num = 0;
		debug(-1, ("cifs_readdir : %s couldnt get entry index %d\n", dir->name, cookie->index));
		UNLOCK_VOL(vol);
		return B_OK;
	}
	
exit1:
	debug(DBG_VNODE_OPS, ("cifs_readdir ... exiting\n"));
	UNLOCK_VOL(vol);
	return result;
	
}




int cifs_rewinddir(void *_vol, void* _node, void* _cookie) {
	
	nspace* 	vol = _vol;
	vnode*		vnode = _node;
	dircookie*	cookie = (dircookie*)_cookie;

	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_rewinddir ... entering.\n"));

#ifdef DELAY_OPENDIR
	cookie->index = -3;
	cookie->previous = 0;
	cookie->omode = 0;
#else
	cookie->index = -2;
#endif

	debug(DBG_VNODE_OPS, ("cifs_reqindir ... exiting.\n"));
	UNLOCK_VOL(vol);
	return B_OK;
}




int cifs_free_dircookie(void* _vol, void* _node, void* _cookie) {

	nspace*		vol = (nspace*)_vol;
	vnode*		node = (vnode*)_node;
	dircookie*	cookie = (dircookie*)_cookie;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_free_dircookie ... exiting\n"));
	
	if (cookie == NULL) {
		debug(-1, ("cifs_free_dircookie called with null cookie.\n"));
		UNLOCK_VOL(vol);
		return EINVAL;
	}
	

	FREE(cookie);
	
	debug(DBG_VNODE_OPS, ("cifs_free_dircookie ... exiting\n"));
	UNLOCK_VOL(vol);
	return B_OK;

}



int cifs_closedir(void *_vol, void *_node, void *_cookie) {

	debug(DBG_VNODE_OPS, ("cifs_closedir ... entering ... exiting ... cya\n"));
	return B_NO_ERROR;
}





int cifs_walk(void *_vol, void *_dir, const char *file, char **newpath, vnode_id *_vnid) {

	nspace*		vol = (nspace*)_vol;
	vnode*		dir = (vnode*)_dir;
	vnode_id	local_vnid;
	vnode*		vn = NULL;
	char		is_dir = 0;
	int			result = ENOENT;
		
	LOCK_VOL(vol);

	debug(DBG_VNODE_OPS, ("cifs_walk .. entering ... dir vnid: %Lx, file: %s\n", dir->vnid, file));
	
		//dm_full_dump();

// Check that dir is actually a directory vnode.
	if (! (dir->mode & S_IFDIR)) {
		debug(DBG_VNODE_OPS, ("called walk on a non-dir.\n"));
		goto exit1;
	}
	
	// Check if we got called with a deleted directory.
	result = vcache_vnid_to_loc(vol, dir->vnid, NULL, NULL, NULL);
	if (result != B_OK) {
		debug(DBG_VNODE_OPS, ("called walk on a deleted directory\n"));
		goto exit1;
	}	

	if (strcmp(file, ".") == 0) {
		debug(DBG_VNODE_OPS, ("called walk on ->.<-\n"));
		local_vnid = dir->vnid;
		result = B_OK;
		goto exit2;
	} 
	
	if (strcmp(file, "..") == 0) {
		debug(DBG_VNODE_OPS, ("called walk on ->..<-\n"));
		local_vnid = dir->parent_vnid;
		result = B_OK;
		goto exit2;
	}	


	result = vcache_loc_to_vnid(vol, file, dir->vnid, & local_vnid, & is_dir);
	if (result != B_OK) {
		// Shouldn't see if our table needs to be updated?
		// cifs_read_vnode(vol, dir->vnid, true,)
		// --nope, what if someone else has that the dir vnode?
		
		// what if we put a lock on the dir entries in the vnode?
		// result = update_entries(vol, dir);
		
		
		debug(DBG_VNODE_OPS, ("cifs_walk ... couldn't find %s, updating.\n", file));
		result = update_entries(vol, dir, false);

		if (result != B_OK) {
			debug(-1, ("cifs_walk ... update_entries error is %d\n", result));
			*_vnid = 0;
			goto exit1;
		}
		result = vcache_loc_to_vnid(vol, file, dir->vnid, & local_vnid, & is_dir);
	}

// Why do we want vn to be filled?

exit2:
	if (result == B_OK) {
		*_vnid = local_vnid;
		result = get_vnode(vol->nsid, local_vnid, (void **)& vn);
	}

exit1:
	debug(DBG_VNODE_OPS, ("cifs_walk exiting: err %s vnid %Lx\n",strerror(result),local_vnid));
	UNLOCK_VOL(vol);
	return result;
}


status_t
cifs_read_vnode(void *_vol, vnode_id vnid, char reenter, void **_node)
{
	status_t err;
	nspace*		vol = (nspace*)_vol;
	vnode_id	dir_vnid;
	vnode*		dir_vnode = NULL;
	char		is_dir;
	char		*filename;
	
	
	if (!reenter) LOCK_VOL(vol);
	
	debug(DBG_VNODE_OPS, ("cifs_read_vnode ... entering, vnid is %Lx, reenter is %x.\n", vnid,  reenter));
	
	//dm_full_dump();
	
	MALLOC(filename, char*, 256);
	if (filename == NULL) {
		err = ENOMEM;
		goto exit1;
	}
	
	err = vcache_vnid_to_loc(vol, vnid, & filename, & dir_vnid, & is_dir);
	// why should this usually work?  Because when we read a directory,
	// we put in all of its contents into the hash table.
	if (err != B_OK) {
		debug(-1, ("couldn't find vnid %Lx.\n", vnid));
		goto exit2;
	}
	
	err = get_vnode(vol->nsid, dir_vnid, (void**)& dir_vnode);
	if (err != B_OK) {
		debug(-1, ("cifs_walk ... woah dude, get vnode failed on the parent dir\n"));
		goto exit2;
	}
	
	err = form_vnode(vol, dir_vnode, filename, vnid, _node);
	

exit3:	
	put_vnode(vol->nsid, dir_vnid);
exit2:
	FREE(filename);
exit1:
	debug(DBG_VNODE_OPS, ("cifs_read_vnode ... exiting with result %s\n", strerror(err)));
	if (!reenter) UNLOCK_VOL(vol);
	
	return err;
}





int cifs_write_vnode(void *_vol, void *_node, char reenter) {

	nspace*		vol = (nspace*)_vol;
	vnode*		node = (vnode*)_node;
	int			result = B_OK;

	int_dir_entry *entry = NULL;
	int_dir_entry *follower = NULL;


	if (node == NULL) return B_ERROR;
	
	if (! reenter) LOCK_VOL(vol);

	debug(DBG_VNODE_OPS, ("cifs_write_vnode ... entering on vnode_id %Lx %s\n", node->vnid, node->name));

	if (node->contents)
		FreeSL(node->contents);
	
	if (! (node->vnid == vol->root_vnode.vnid)) {
		FREE(node);
	}
	
	debug(DBG_VNODE_OPS, ("cifs_write_vnode ... exiting.\n"));

	if (! reenter) UNLOCK_VOL(vol);

	return result;

}




int cifs_rfsstat(void *_vol, struct fs_info * fss) {

	nspace*		vol = (nspace*)_vol;
	int			i;
	int 		result = B_ERROR;
	static uint32 last_access_time = 0;
	int			adj;
	
	
	LOCK_VOL(vol);
	
	debug(DBG_VNODE_OPS, ("cifs_rfsstat ... entering.\n"));
	
	
// So, when Tracker opens a window on the desktop, it makes lots of
// stat calls.  This will ensure that remote rfs calls are made with no less
// than RFSSTAT_DELAY seconds between them.
#define RFSSTAT_DELAY 5
	if ((last_access_time != 0) && ((real_time_clock() - last_access_time) <= RFSSTAT_DELAY)) {
		fss->block_size = vol->fss_block_size;
		fss->total_blocks = vol->fss_total_blocks;
		fss->free_blocks = vol->fss_free_blocks;
		//dprintf("rfsstat delay in effect\n");
	} else {
		//result = do_rfsstat(vol, &(vol->fss_block_size), &(vol->fss_total_blocks), &(vol->fss_free_blocks));
		
		//dprintf("trying oldstyle rfsstat\n");
		
		result = oldstyle_rfsstat(vol);
		
		if (result != B_OK) {
			dprintf("DO_RFSSTAT FAILED : err %d %s\n", result, strerror(result));
			memset(fss, 0, sizeof(struct fs_info));
			goto exit1;
		}	
		fss->block_size = vol->fss_block_size;
		fss->total_blocks = vol->fss_total_blocks;
		fss->free_blocks = vol->fss_free_blocks;

		last_access_time = real_time_clock();
	}
#undef RFSSTAT_DELAY	
	
	

	// fss->dev and fss->root filled in by kernel

	
	//	These two flags must be set in order for Tracker to use these
	// 	volumes properly.
	fss->flags = B_FS_IS_SHARED | B_FS_IS_PERSISTENT;
	
	// IO size - specifies buffer size for file copying
	//fss->io_size = vol->MaxBufferSize;
	fss->io_size = vol->io_size;
	
	// Device name.
	strcpy(fss->device_name, vol->Params.Unc);

	// Volume name.
	//strcpy(fss->volume_name, vol->Share);
	
	// We are what the hood makes of us.
	if ((vol->Params.LocalShareName) && ((vol->Params.LocalShareName)[0] != 0))
		strcpy(fss->volume_name, vol->Params.LocalShareName);
	else
		strcpy(fss->volume_name, vol->Params.Unc);
	
	// vyt: should sanitize name as well
#if 0
	for (i=10;i>0;i--)
		if (fss->volume_name[i] != ' ')
 			break;
	fss->volume_name[i+1] = 0;
	for (;i>=0;i--) {
		if ((fss->volume_name[i] >= 'A') && (fss->volume_name[i] <= 'Z'))
			fss->volume_name[i] += 'a' - 'A';
	}
#endif

	// File system name
	strcpy(fss->fsh_name, "cifs");


exit1:
	debug(DBG_VNODE_OPS, ("cifs_rfsstat ... exiting.\n"));
	UNLOCK_VOL(vol);
	return B_OK;
}






int cifs_wfsstat(void *_vol, struct fs_info * fss, long mask) {

// Read only for now.
	debug(DBG_VNODE_OPS, ("cifs_wstat called\n"));
	return B_OK;
	
}

status_t
form_vnode(nspace* vol, vnode* dir_vnode, char* filename, vnode_id this_vnid,
				void** _node) {

	status_t 	err;
	vnode*		this_vnode;
	int			big_enough;
	
	debug(DBG_VNODE_OPS, ("form_vnode entering for file %s\n", filename));
	
		//dm_full_dump();

	
	// Malloc vnode
	MALLOC(this_vnode, vnode*, sizeof(vnode));
	if (this_vnode == NULL) {
		debug(0, ("out of mem.\n"));
		err = ENOMEM;
		goto exit1;
	}

	// Setup vnid's, null out dir_contents, set small_name
	*_node = this_vnode;
	this_vnode->vnid = this_vnid;
	this_vnode->parent_vnid = dir_vnode->vnid;
	this_vnode->contents = NULL;
	strncpy(this_vnode->name, filename, 255);
	

	this_vnode->last_update_time = 0;

	// Set mime type
	set_mime_type(this_vnode, (const char*)this_vnode->name);

	// Fill in the rest of the vnode with data from it's directory
	err = fill_vnode(vol, dir_vnode, this_vnode);
	if (err == B_OK) {
		debug(DBG_VNODE_OPS, ("form_vnode exiting clean.\n"));
		return B_OK;
	}

exit2:
	FREE(this_vnode);
exit1:
	debug(DBG_VNODE_OPS, ("form_vnode ... exiting poorly with result %d\n", err));
	return err;

}

status_t
do_unlink(nspace *vol, vnode *dir, const char *name) {

	status_t err;
	vnode *node = NULL;
	vnode_id vnid;
	char is_dir;

	err = vcache_loc_to_vnid(vol, name, dir->vnid, & vnid, & is_dir);
	if (err != B_OK) {
		debug(-1, ("do_unlink called on non-existent entity\n"));
		err = ENOENT;
		goto exit1;
	}
	
	if (is_dir) {
		debug(-1, ("do_unlink called on directory\n"));
		err = EISDIR;
		goto exit1;
	}
	
	// Now I'm following Victor's advice
	err = get_vnode(vol->nsid, vnid, (void**) & node);
	
	// Check if this node is writable
	if (! (node->mode & S_IWUSR)) {
		debug(-1, ("do_unlink called on non-writable node\n"));
		err = EACCES;
		goto exit2;
	}
	
	// Ok, now I'm following dbg's book on how to do this.
	err = notify_listener(B_ENTRY_REMOVED, vol->nsid, dir->vnid, 0, vnid, NULL);
	
	err = remove_vnode(vol->nsid, vnid);
	if (err != B_OK) goto exit2;
		
	// Now remove references to this file
	err = remove_from_vcache(vol, vnid);
	err = del_in_dir(dir, vnid);
		
	
exit2:
	put_vnode(vol->nsid, vnid);
exit1:	
	debug(DBG_VNODE_OPS, ("do_unlink exiting with %s\n", strerror(err)));
	return err;
}


status_t	
cifs_unlink(void *_vol, void *_dir, const char *name)
{
	status_t err;
	nspace *vol = (nspace *)_vol;
	vnode *dir = (vnode *)_dir;

	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_unlink called: dir %s, name %s\n", dir->name, name));

	if (strcmp(name, ".") == 0) {
		err = EPERM;
		goto exit1;
	}
	
	if (strcmp(name, "..") == 0) {
		err = EPERM;
		goto exit1;
	}
	
	err = do_unlink(vol,dir,name);

exit1:
	debug(DBG_VNODE_OPS, ("cifs_unlink exiting\n"));
	UNLOCK_VOL(vol);
	return err;
}	

status_t
cifs_remove_vnode(void* _vol, void* _node, char reenter)
{
	// All of the directory structs and the vcache have been cleared
	// when we get in here, so we just  need to tell the server
	// to delete the file.
	// Also, the fsil doesn't pay attention to the return value of this
	// function. 
	status_t	err;
	nspace	*vol = (nspace*)_vol;
	vnode	*node = (vnode*)_node;
	char *name=NULL;
	
	debug(DBG_VNODE_OPS, ("cifs_remove_vnode called on file ->%s<-\n", node->name));
	if (! reenter) LOCK_VOL(vol);
	
	err = get_full_name(vol, node, NULL, & name);
	if (err != B_OK) {
		debug(-1, ("cifs_remove_vnode : get_full_name failed\n"));
		goto exit2;
	}

	err = do_delete(vol, name, (node->mode & S_IFDIR));
	if (err != B_OK)
		debug(-1, ("cifs_remove_vnode : do_delete failed\n"));

	FREE(name);
exit2:
	FREE(node);
exit1:
	debug(DBG_VNODE_OPS, ("cifs_remove_vnode exiting\n"));
	if (! reenter) UNLOCK_VOL(vol);
	return err;
}


int cifs_rstat(void* _vol, void* _node, struct stat* st) {

	int			result;
	nspace*		vol = (nspace*)_vol;
	vnode*		node = (vnode*)_node;
	vnode*		dir_vnode = NULL;
	
	char*		hldr = NULL;
	int			i;

	char*		cur_name;
	ulong		next_offset;
	
	uint64		big_time;
	const uint64	mickysoft_sucks_shit = 11644473600;
	// This aptly named var is the number of seconds between
	// Jan 1st 1601 and Jan 1st 1970
	char*		show_time;
	int _tmp;
	
#define CIFSFS_RSTAT_DBG 15	
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_rstat : %s\n", node->name));
	//debug(DBG_VNODE_OPS, ("file name is %s vnid is %Lx\n", node->full_name, node->vnid));


	st->st_dev = vol->nsid;
	//debug(DBG_VNODE_OPS, ("vol->nsid is %x\n", vol->nsid));		
	st->st_ino = node->vnid;
	//debug(DBG_VNODE_OPS, ("node->vnid is %Lx\n", node->vnid));		
	st->st_mode = node->mode;
	//debug(DBG_VNODE_OPS, ("node->mode is %x\n", node->mode));

	st->st_nlink = node->nlink;
	st->st_uid = node->uid;
	st->st_gid = node->gid;
	st->st_size = node->size;
	//debug(DBG_VNODE_OPS, ("node->size is %x\n", node->size));
	
	st->st_blksize = vol->io_size;
	
	st->st_atime = node->atime;
	//debug(DBG_VNODE_OPS, ("node->atime is %x\n", node->atime));	
	st->st_ctime = node->ctime;
	//debug(DBG_VNODE_OPS, ("node->ctime is %x\n", node->ctime));		
	st->st_mtime = node->mtime;
	//debug(DBG_VNODE_OPS, ("node->mtime is %x\n", node->mtime));		
	st->st_crtime = node->crtime;
	//debug(DBG_VNODE_OPS, ("node->crtime is %x\n", node->crtime));	
	


	debug(DBG_VNODE_OPS, ("cifs_rstat ... exiting\n"));
	UNLOCK_VOL(vol);
	return B_OK;

#undef CIFSFS_RSTAT_DBG
	
}

int cifs_wstat(void *_vol, void *_node, struct stat *st, long mask) {
	
	nspace *vol = (nspace*)_vol;
	vnode *node = (vnode*)_node;
	
	int result = B_OK;
	
	LOCK_VOL(vol);

	debug(DBG_VNODE_OPS, ("cifs_wstat called!!!\n"));
	debug(45, ("here's the mask we got: %x\n", mask));
	
	if (mask & WSTAT_MTIME)
		node->mtime = st->st_mtime;
	
	if (mask & WSTAT_ATIME)
		node->atime = st->st_atime;
		
	if (mask & WSTAT_CRTIME)
		node->crtime = st->st_crtime;
	
	if (mask & WSTAT_MODE) {
		debug(45, ("wstat called with mode mask\n"));
		debug(45, ("stat->mode is %x\n", st->st_mode));
	}
	
	
	debug(DBG_VNODE_OPS, ("cifs_wstat exiting with result %d\n", result));
	UNLOCK_VOL(vol);
	return result;
}
	
status_t	
cifs_open(void* _vol, void* _node, int omode, void** _cookie) {

	status_t	err;
	nspace*		vol = (nspace*)_vol;
	vnode*		node = (vnode*)_node;
	filecookie	*cookie = NULL;

	
	LOCK_VOL(vol);
	
	debug(DBG_VNODE_OPS, ("cifs_open called on vnid: %Lx , named %s\n", node->vnid, node->name));

	if (node->mode & S_IFDIR) {
		debug(DBG_VNODE_OPS,("cifs_open called with dir, calling do_opendir\n"));
		err = do_opendir(_vol,_node,_cookie);
		UNLOCK_VOL(vol);
		return err;
	}	
	
	if (omode & O_CREAT) {
		debug(15, ("cifs_open called with O_CREAT\n"));
		goto exit1;
	}

	
	err = new_filecookie(& cookie);
	if (err != B_OK) {
		debug(-1, ("couldnt new_filecookie\n"));
		goto exit1;
	}
	
	/* Note:  Using SMB_COM_OPEN_ANDX will not give you any info
		other than a fid.  Sniffing the wire, I found that NT Server 3.51
		and NT5 beta1 do not return any other file info, _including_
		file size.
		Maybe we should use NT_CREATE_ANDX instead.
		-al Nov 8,1998
		*/
	
	if (vol->Capabilities & CAP_NT_SMBS) {
		err = nt_open_file(vol, node, omode, cookie);
	} else {
		err = new_open_file(vol, node, omode, cookie);
	}
	if (err != B_OK) {
		debug(0, ("error on open_file\n"));
		goto exit2;
	}
	
	*_cookie = cookie;
	UNLOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_open exiting with successfully"));
	return B_OK;

exit2:
	del_filecookie(cookie);
exit1:
	UNLOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_open exiting with failure : %s", strerror(err)));
	return err;	
}	
	
	
	
status_t
cifs_create(void *_vol, void *_dir, const char *name, int omode,
	int perms, vnode_id *_vnid, void **_cookie)
{
	status_t err;
	nspace *vol = (nspace *)_vol;
	vnode *dir = (vnode *)_dir;
	vnode *node = NULL;
	filecookie	*cookie=NULL;
	vnode_id 	vnid;
	char		is_dir;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_create called in directory %s\n", dir->name));

	err = update_entries(vol, dir, false);
	if (err != B_OK) goto exit1;
	
	err = vcache_loc_to_vnid(vol, name, dir->vnid, & vnid, & is_dir);
	if (err == B_OK) {  // Then we already have this file
		if (omode & O_EXCL) {
			debug(-1, ("file already exists, but omode is o_excl\n"));
			err = EEXIST;
			goto exit1;
		}
		if (get_vnode(vol->nsid, vnid, (void**)&node) != B_OK) {
			debug(-1,  ("couldnt get vnid %Lx for file\n"));
			err = ENOENT;
			goto exit1;
		}
		
		*_vnid = vnid;
		
		// Now we have a normal open.
		if (node->mode & S_IFDIR) {
			debug(-1, ("this is a directory, exiting\n"));
			err = EISDIR;
			goto exit2;
		}
		
		err = new_filecookie(& cookie);
		if (err != B_OK) {
			debug(-1, ("couldnt new_filecookie\n"));
			goto exit2;
		}

		if (vol->Capabilities & CAP_NT_SMBS) {
			err = nt_open_file(vol, node, omode, cookie);
		} else {
			err = new_open_file(vol, node, omode, cookie);
		}
		
		if (err != B_OK) {
			debug(-1, ("error on open_file\n"));
			goto exit3;
		}
		
		
		if (omode & O_TRUNC)
			node->size = 0;
		
		cookie->omode = omode;
		*_cookie = cookie;

	} else {  // We don't have this file in our hash table, so we
			  // launch a packet, and let the server do the work.
		err = new_filecookie(& cookie);
		if (err != B_OK) {
			debug(-1, ("couldnt new_filecookie\n"));
			UNLOCK_VOL(vol);
			return err;
		}

		err = create_file(vol, dir, name, omode, perms, & node, & vnid, cookie);
		if (err != B_OK) {
			debug(-1, ("create_file failed in cifs_create\n"));
			del_filecookie(cookie);
			UNLOCK_VOL(vol);
			return err;
		}
		
		*_vnid = vnid;
		cookie->omode = omode;
		*_cookie = cookie;
		//dprintf("notify_listener: B_ENTRY_CREATED vnid %Lx name %s\n", vnid, name);		
		notify_listener(B_ENTRY_CREATED, vol->nsid, dir->vnid, 0, vnid, name);
	}

	debug(DBG_VNODE_OPS, ("ciffs_create exiting well\n"));
	UNLOCK_VOL(vol);
	return B_OK;	


exit4:
	FREE(node);
exit3:
	del_filecookie(cookie);
exit2:
	put_vnode(vol->nsid, vnid);	
exit1:
	UNLOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("ciffs_create : failure %s\n", strerror(err)));
	return err;
}




status_t
cifs_mkdir(void *_vol, void *_dir, const char *name, int perms) {

	status_t result = B_ERROR;
	nspace *vol = (nspace*)_vol;
	vnode *dir = (vnode*)_dir;
	
	vnode_id vnid;
	vnode* node = NULL;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_mkdir entering, upper dir is %s wantto make %s\n", dir->name, name));
	
	if (is_vnode_removed(vol->id, dir->vnid) > 0) {
		debug(-1, ("mkdir called in removed directory\n"));
		UNLOCK_VOL(vol);
		return EPERM;
	}
	
	if (! (dir->mode & S_IFDIR)) {
		debug(-1, ("mkdir called on a non-dir\n"));
		UNLOCK_VOL(vol);
		return EINVAL;
	}
	
	// According to dosfs, S_IFDIR is never set in the passed in perms.
	perms &= ~S_IFMT; perms |= S_IFDIR;
	
	result = create_dir(vol, dir, name, perms);
	
	if (result == B_OK) {
		vnid = generate_unique_vnid(vol);
		notify_listener(B_ENTRY_CREATED, vol->nsid, dir->vnid, 0, vnid, name);
	}
	
	debug(DBG_VNODE_OPS, ("cifs_mkdir exiting with result %d %s\n", result, strerror(result)));
	UNLOCK_VOL(vol);
	return result;
}	

status_t
cifs_rmdir(void *_vol, void *_dir, const char *name)
{
	status_t err;
	nspace	*vol = (nspace*)_vol;
	vnode	*dir = (vnode*)_dir;
	vnode *node = NULL;
	vnode_id vnid;
	char is_dir;
	
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_rmdir called: dir %s, name %s\n", dir->name, name));
	
	if ( (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0)) {
		err = EPERM;
		goto exit1;
	}
	err = vcache_loc_to_vnid(vol, name, dir->vnid, & vnid, & is_dir);
	if (err != B_OK) {
		debug(-1, ("cifs_unlink called on non-existent entity\n"));
		err = ENOENT;
		goto exit1;
	}
	if (vnid == vol->root_vnode.vnid) {
		debug(-1, ("cifs_rmdir called on root dir! Bad Dog!\n"));
		err = EPERM;
		goto exit1;
	}
	if (! is_dir) {
		debug(-1, ("cifs_rmdir called on NON-directory\n"));
		err = ENOTDIR;
		goto exit1;
	}
	
	// Now I'm following Victor's advice
	err = get_vnode(vol->nsid, vnid, (void**) & node);
	
	// Check if this node is writable
	if (! (node->mode & S_IWUSR)) {
		debug(-1, ("cifs_rmdir called on non-writable\n"));
		err = EACCES;
		goto exit2;
	}
	
	if (NumInSL(node->contents) > 0) { // xxx ???  Is this a fair test?
		debug(-1, ("cifs_rmdir called on non-empty directory\n"));
		err = ENOTEMPTY;
		goto exit2;
	}
	
	// Now remove references to this dir
	err = remove_from_vcache(vol, vnid);
	
	err = del_in_dir(dir, vnid);

	// Ok, now I'm following dbg's book on how to do this.
	err = notify_listener(B_ENTRY_REMOVED, vol->nsid, dir->vnid, 0, vnid, NULL);
	
	err = remove_vnode(vol->nsid, vnid);
	
	if (err != B_OK)
		debug(-1, ("remove_vnode returned %d\n", err));
	
exit2:	
	put_vnode(vol->nsid, vnid);
exit1:	
	debug(DBG_VNODE_OPS, ("cifs_rmdir exiting\n"));
	UNLOCK_VOL(vol);
	return err;
}	
	
	




int cifs_read(void *_vol, void *_node, void *_cookie, off_t pos,
				void *buf, size_t *len) {
				
	nspace		*vol = (nspace*)_vol;
	vnode		*node = (vnode*)_node;
	filecookie		*cookie = (filecookie*)_cookie;
	uint32		cur_time;
	
	int			result = B_ERROR;	
	
	LOCK_VOL(vol);
	
	debug(DBG_VNODE_OPS, ("cifs_read entering\n"));
	//debug(-1, ("\tnode size is %d\n", node->size));
	//debug(-1, ("\tasked to read %d bytes\n", *len));
		
	result = read_from_file(vol, node, pos, buf, len, cookie);
	if (result != B_OK) {
		debug(-1, ("cifs_read: read_from_file failed with rselt %d\n", result));
		*len = 0;
		goto exit1;
	}

	//debug(-1, ("\twere telling someone we read %d bytes\n", *len));


	cur_time = real_time_clock();
	node->atime = cur_time;
	node->ctime = cur_time;
	
	
exit1:
	debug(DBG_VNODE_OPS, ("cifs_read exiting with result %d\n", result));
	UNLOCK_VOL(vol);
	return result;
}


status_t
cifs_write(void *_vol, void *_node, void *_cookie, off_t pos,
				const void *buf, size_t *len) {
				
	nspace		*vol = (nspace*)_vol;
	vnode		*node = (vnode*)_node;
	filecookie		*cookie = (filecookie*)_cookie;
	uint32		cur_time;
	int			result = B_ERROR;
	
	ushort		send_len = 0;
	size_t		left_to_send = 0;
	void  		*cur_buf;
	off_t		cur_pos;
	uchar*		tmp_buf;
	
	int _tmp;
		
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_write entering %d bytes at %Lx\n", *len, pos));
		
//	if (cookie->omode & O_APPEND) {
//		dprintf("writing in append mode\n");
//		pos = (node->size);
//	}
	// The fsil adjusts for append mode.



	result = write_to_file(vol, node, pos, buf, len, cookie);
	if (result != B_OK) {
		*len = 0;
		goto exit1;
	}
	cur_time = real_time_clock();

	if (pos + *len > node->size)
		node->size = pos + *len;
	
	node->mtime = cur_time;
	node->atime = cur_time;
	node->ctime = cur_time;

	cookie->flags |= WAS_WRITTEN;
	
exit1:
	debug(DBG_VNODE_OPS, ("cifs_write exiting with result %d %s, reporrint write of %d (dec)\n", result, strerror(result), *len));
	UNLOCK_VOL(vol);
	return result;
	
}

status_t
cifs_close(void *_vol, void *_node, void *_cookie)
{
	status_t err;
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	filecookie	*cookie = _cookie;
	
	debug(DBG_VNODE_OPS, ("cifs_close\n"));

	return B_OK;
}
	
	
	
status_t	
cifs_free_cookie(void *_vol, void *_node, void *_cookie)
{
	status_t err;
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	filecookie *cookie = (filecookie*)_cookie;
		
	LOCK_VOL(vol);
	debug(DBG_VNODE_OPS, ("cifs_free_cookie entering\n"));
	
	if (cookie->magic == CIFS_FILE_COOKIE)
		close_file(vol, node, cookie);
	del_filecookie(cookie);
	
	debug(DBG_VNODE_OPS, ("cifs_free_cookie exiting\n"));

	if (cookie->flags & WAS_WRITTEN)
		notify_listener(B_STAT_CHANGED, vol->nsid, 0, 0, node->vnid, NULL);

	UNLOCK_VOL(vol);
	return B_OK;
}



status_t
cifs_ioctl(void *_vol, void *_node, void *_cookie, int code, void *buf, size_t len)
{
	status_t err = EINVAL;
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	filecookie *cookie = (filecookie*)_cookie;
	
	debug(DBG_VNODE_OPS, ("cifs_ioctl\n"));
	
	switch (code) {
		case 100175 :
			LOCK_VOL(vol);
				err = update_entries(vol,node,true);
			UNLOCK_VOL(vol);			 
		break;
		
		default:
			return EINVAL;
	}
	return err;
}
	
// A little info on renaming.
// The cifs spec specifies two ways to do this - MOVE and RENAME.
// Up to Samba2.0.0, samba does _not_ support the MOVE command.
// Dont know why.  For file system call of "cifs_rename", the
// better one to use would be MOVE, because you can force a
// deletion of the newname, if it exists.
// So the heuristic goes like this - if you're connected to a non-samba
// server, use MOVE.  Otherwise, you _must_ first try to delete newname,
// then use RENAME.  Got it?
// - alfred feb 3, 1999

// Renaming is hard.  We may not have any info on either of the
// dir/name pairs given to us, and that could be cause they dont exist
// or cause we dont know abou them yet.  So, we first ask the server
// to do the rename, then update our view.
// Also, remember this is supposed to be atomic. 
// And take the trash out while you're at it.
#define DBG_RENAME 0
status_t
cifs_rename(void *_vol, void *_odir, const char *oldname,
					void *_ndir, const char *newname)
{

	status_t err;
	nspace *vol = (nspace *)_vol;
	vnode *odir = (vnode *)_odir;
	vnode *ndir = (vnode *)_ndir;
	
	vnode_id ovnid=0;
	vnode_id nvnid=0;
	vnode	*onode=NULL, *nnode=NULL;

	int_dir_entry *old_dir_entry=NULL;
	int_dir_entry *new_dir_entry=NULL;
	char *newname_fullpath=NULL;	
	char is_dir=0;
	bool oh_shit = false;
	ushort attr;
	int32 rename_flags=0;
	
	// In case something seriously screws up after we've talked to the server
	// we set this to true and just force a complete reload of both directories

	LOCK_VOL(vol);

	// Get vnodes
	err = vcache_loc_to_vnid(vol, oldname, odir->vnid, &ovnid, &is_dir);
	if (err == B_OK) {
		err = get_vnode(vol->nsid, ovnid, (void**)&onode);
		if (err != B_OK) { // Just pretend we didnt see that cache call
			debug(-1,("cifs : found vnid %x in vcache but get_vnode failed\n",ovnid));
			ovnid = 0;
		} 
	} else {
		debug(DBG_RENAME, ("couldnt get old info\n"));
	}
	err = vcache_loc_to_vnid(vol, newname, ndir->vnid, &nvnid, NULL);
	if (err == B_OK) {
		err = get_vnode(vol->nsid, nvnid, (void**)&nnode);
		if (err != B_OK) { // Just pretend we didnt see that cache call
			debug(-1,("cifs : found vnid %x in vcache but get_vnode failed\n",nvnid));
			nvnid = 0;
		}
	} else {
		debug(DBG_RENAME, ("couldnt get new info\n"));
	}


	debug(DBG_RENAME, ("changing at server\n"));
	// Change at server
	err = get_attributes(vol, odir, oldname, & attr);
	if (err != B_OK) {
		debug(-1, ("cifs_rename get_attributes failed : %s\n", strerror(err)));
		goto exit3;
	}	
	if (attr & FILE_ATTR_DIRECTORY)
		rename_flags = RENAME_IS_DIR;
	

	err = do_rename(vol, odir, oldname, ndir, newname, (rename_flags | RENAME_SAMBA_STYLE));
	
	if ((err == B_FILE_EXISTS) || (err == B_PERMISSION_DENIED)) {
		if ((err = get_full_name(vol,ndir,newname,&newname_fullpath)) == B_OK) {
			if ((err = do_delete(vol, newname_fullpath, (attr & FILE_ATTR_DIRECTORY))) == B_OK) {
				FREE(newname_fullpath);
				err = do_rename(vol, odir, oldname, ndir, newname, (rename_flags | RENAME_SAMBA_STYLE));
			}				
		}
	}
	
	if (err != B_OK) {
		debug(DBG_RENAME,("server denied rename request : %s\n", strerror(err)));
		goto exit3;
	}

	debug(DBG_RENAME,("updating view\n"));
	// Update our view of the server
	if (nvnid != 0) { // We knew about the new name, so delete it
		debug(DBG_RENAME,("we knew about new name, vnid is %Lx\n",nvnid));
		if (remove_from_vcache(vol,nvnid) == B_OK) {
			if (del_in_dir(ndir,nvnid) == B_OK) {
				notify_listener(B_ENTRY_REMOVED, vol->nsid, ndir->vnid, 0, nvnid, NULL);
			} else {
				dprintf("cifs : del_in_dir failed in nvnid in rename\n");
			}	
		} else {
			dprintf("cifs : remove_from_vcache failed in nvnid in rename\n");
		}
	}
	if (ovnid != 0) { // We knew about the old name, so move its info
		debug(DBG_RENAME,("we knew about old name, vnid is %Lx\n",ovnid));
		err = remove_from_vcache(vol, ovnid);
		if (err != B_OK) {
			dprintf("remove_from_vcache failed in ovnid\n");
		}
		err = add_to_vcache(vol, ovnid, newname, ndir->vnid, is_dir);
		if (err != B_OK) {
			dprintf("add_to_vcache failed in ovnid\n");
		}
		
		// Now switch the dir_entrys
		old_dir_entry = find_in_dir(odir, ovnid);
		if (old_dir_entry != NULL) {
			if (odir->vnid != ndir->vnid) {  // Dont have to switch
				MALLOC((new_dir_entry), int_dir_entry*, sizeof(int_dir_entry));
				if (new_dir_entry == NULL) {
					debug(-1, ("couldnt get memory for new_dir_entry\n"));
				}
				strncpy(new_dir_entry->filename, newname, 255);
				memcpy(new_dir_entry, old_dir_entry, sizeof(int_dir_entry));
				err = add_to_dir(ndir, new_dir_entry);
				if (err != B_OK) {
					FREE(new_dir_entry);
					debug(-1, ("couldnt add_to_dir new_dir_entry\n"));
				}
				old_dir_entry = NULL; // to be sure
				err = del_in_dir(odir, ovnid);
				if (err != B_OK) {
					dprintf("del_in_dir failed in ovnid\n");
				}
			} else {
				strncpy(old_dir_entry->filename, newname, 255);
			}
		} else {
			debug(-1, ("find_in_dir returned null in ovnid branch\n"));
		}
		
		// Now alter the vnode
		strncpy(onode->name, newname, 255);
		onode->parent_vnid = ndir->vnid;


		notify_listener(B_ENTRY_MOVED, vol->nsid, odir->vnid, ndir->vnid, ovnid, newname);

	} else { // We didn't know about the old location
		// Force an update of the new dir
		err = update_entries(vol, ndir, true);
		// This will make the proper B_ENTRY_CREATED notification
	}
	
	if (oh_shit) {
		dprintf("cifs_rename PANIC : %s %s to %s %s\n", odir->name,oldname,
				ndir->name,newname);
		panic("cifs_rename : oh_shit signaled\n");
	/*	if (update_entries(vol,odir,true) != B_OK)
			odir->contents = NULL;
		if (update_entries(vol,ndir,true) != B_OK)
			ndir->contents = NULL;
	*/
	}

exit3:
	if (nvnid != 0) {
		put_vnode(vol->nsid, nvnid);
	}
exit2:
 	if (ovnid != 0) {
		put_vnode(vol->nsid, ovnid);
	}
exit1:
	UNLOCK_VOL(vol);
	return err;
}
