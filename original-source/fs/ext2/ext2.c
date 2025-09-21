// This module contains some of the initialization and deallocation routines. 

#include "ext2.h"

uint8 ext2_debuglevels[] = {
	0, // ALL
	0, // IO
	0, // FILE
	0, // DIR
	0, // EXTENT
	0, // ALLOC
	4, // FS
	0, // VNODE
	0  // ATTR
};


// Creates a new instance of the filesystem
// Much more error checking needs to be done here.
ext2_fs_struct *ext2_create_ext2_struct()
{
	ext2_fs_struct *newstruct;
		
	DEBUGPRINT(FS, 5, ("ext2_create_ext2_struct called.\n"));

	newstruct = (ext2_fs_struct *)calloc(1, sizeof(ext2_fs_struct));
	if(!newstruct) {
		ERRPRINT(("ext2_create_ext2_struct could not allocate memory for fs structure.\n"));
		return NULL;
	}

	newstruct->magic = EXT2_FS_STRUCT_MAGIC;
	
	newstruct->fd = -1;

	newstruct->root_vnid = ROOTINODE;

	newstruct->is_read_only = true; // RO until proven otherwise

#ifndef RO
	DEBUGPRINT(FS, 5, ("ext2_create_ext2_struct allocating semaphores.\n"));

	newstruct->block_alloc_sem = newstruct->block_marker_sem = newstruct->prealloc_sem = -1;

	newstruct->block_alloc_sem=create_sem (1,"ext2 block alloc sem");
	if (newstruct->block_alloc_sem<B_NO_ERROR) {
		goto semerror;
	}
	set_sem_owner (newstruct->block_alloc_sem,B_SYSTEM_TEAM);

	newstruct->block_marker_sem=create_sem (1,"ext2 block marker sem");
	if (newstruct->block_marker_sem<B_NO_ERROR) {
		goto semerror;
	}
	set_sem_owner (newstruct->block_marker_sem,B_SYSTEM_TEAM);
	
	newstruct->prealloc_sem=create_sem (1,"ext2 block preallocation sem");
	if (newstruct->prealloc_sem<B_NO_ERROR) {
		goto semerror;
	}
	set_sem_owner (newstruct->prealloc_sem,B_SYSTEM_TEAM);
#endif	

	return newstruct;

#ifndef RO
semerror:		
	ERRPRINT(("error creating fs semaphores, aborting mount...\n"));
	ext2_remove_ext2_struct(newstruct);
	return NULL;
#endif
}

status_t ext2_remove_ext2_struct(ext2_fs_struct *removeit)
{
	DEBUGPRINT(FS, 5, ("ext2_remove_ext2_struct destructing for nsid %ld.\n", removeit->nsid)); 
	
	if((!removeit) || (CHECK_E2FS_MAGIC(removeit)<B_OK)) {
		ERRPRINT(("ext2_remove_ext2_struct passed bogus structure.\n"));
		return B_ERROR;
	}
	
	DEBUGPRINT(FS, 5, ("removing all allocated memory and semaphores.\n"));
	// Clear out some stuff.
	if(removeit->gd) free(removeit->gd);
	if(removeit->device_name) free(removeit->device_name);
	if(removeit->volume_name) free(removeit->volume_name);
#ifndef RO
	if(removeit->gdToFlush) free(removeit->gdToFlush);
	if(removeit->block_alloc_sem>=0) delete_sem (removeit->block_alloc_sem);
	if(removeit->block_marker_sem>=0) delete_sem(removeit->block_marker_sem);
	if(removeit->prealloc_sem>=0) delete_sem(removeit->prealloc_sem);
#endif
		
	DEBUGPRINT(FS, 5, ("removing cache blocks.\n"));
	// Deallocate the cache
	if (removeit->is_cache_initialized) {
		if(removeit->is_read_only) 
			remove_cached_device_blocks (removeit->fd,NO_WRITES);
		else
			remove_cached_device_blocks (removeit->fd,ALLOW_WRITES);
	}	

	DEBUGPRINT(FS, 5, ("closing file device.\n"));
	// Close the device
	if(removeit->fd>0) close(removeit->fd);
	
	DEBUGPRINT(FS, 5, ("removing the fs structure..."));
	free(removeit);
	DEBUGPRINT(FS, 5, ("done.\n"));
	
	return B_NO_ERROR;
}

status_t ext2_init_cache (ext2_fs_struct *ext2)
{	
	status_t err;

	DEBUGPRINT(FS, 5, ("ext2_init_cache entry.\n"));

	if(ext2->fd<0) return B_ERROR;

	ext2->num_cache_blocks = ext2->info.s_num_sectors/(ext2->info.s_block_size/512);
	
	DEBUGPRINT(FS, 5, ("ext2_init_cache initializing cache for %ld cache blocks of size %d.\n",
		ext2->num_cache_blocks, ext2->info.s_block_size));
	
	err = init_cache_for_device(ext2->fd, ext2->num_cache_blocks);
	if(err < B_NO_ERROR) return err;
		
	ext2->is_cache_initialized=true;
	
	return B_NO_ERROR;
}

status_t ext2_initialize_vnode(vnode *new_vnode, vnode_id id)
{
	if(!new_vnode) return EINVAL;
	
	memset(new_vnode, 0, sizeof(vnode));

#ifndef RO
	new_vnode->pre_pos = 8;
#endif
	new_vnode->vnid=id;
	new_vnode->magic = VNODE_MAGIC;

#ifndef RO
	// Set up the vnode's semaphore
	{
		char str[128];
		sprintf(str, "ext2 vnode %Ld sem", id);
		
		new_mlock(&new_vnode->vnode_mlock, MAX_READERS, str);
		if(new_vnode->vnode_mlock.s <= 0) {
			ERRPRINT(("ext2_add_vnode_to_list error allocating semaphore for vnode %Ld.\n", id));
			return B_ERROR;
		}
		set_sem_owner(new_vnode->vnode_mlock.s, B_SYSTEM_TEAM);
	}
#endif	
	return B_NO_ERROR;
}

status_t ext2_deallocate_vnode(vnode *killit)
{	
	if(!killit) return B_ERROR;
	
	if(CHECK_VNODE_MAGIC(killit) < B_NO_ERROR) return B_ERROR;
	
#ifndef RO
	if(killit->vnode_mlock.s >0) free_mlock(&killit->vnode_mlock);
#endif
	free(killit);
	
	return B_NO_ERROR;
}

