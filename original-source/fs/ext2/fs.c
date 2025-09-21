#include <scsi.h>

#include "ext2.h"
#include "io.h"

static int lock_removable_device(int fd, bool state)
{
	return ioctl(fd, B_SCSI_PREVENT_ALLOW, &state, sizeof(state));
}

// Called whenever the filesystem should be mounted.
int	ext2_mount(nspace_id nsid, const char *devname, ulong flags,
					void *parms, size_t len, 
					void **data, vnode_id *vnid)
{
	ext2_fs_struct *ext2;
	int start;
	vnode *newNode;
	status_t err;
	device_geometry geo;

#ifdef ENABLE_DPRINTF
	{
		bool old_state;
		old_state = set_dprintf_enabled(1); 
		DEBUGPRINT(FS, 4, ("set_dprintf_enabled(1) = %d.\n", old_state));
	}
#endif

#ifdef LOAD_SYM
	// Try to load the driver symbols.
	DEBUGPRINT(FS, 4, ("ext2_mount loading symbols.\n"));

	if(load_driver_symbols("ext2")==B_ERROR) {
		DEBUGPRINT(FS, 4, ("ext2_mount Error loading driver symbols.\n"));
	} else {
		DEBUGPRINT(FS, 4, ("ext2_mount Driver symbols loaded.\n"));
	}
#endif


	DEBUGPRINT(FS, 4, ("ext2_mount entry, args (nsid=%d, devname='%s', flags=%lx, size=%ld)\n",
		(int)nsid, devname, flags, len));

	// Make a new object
	ext2 = ext2_create_ext2_struct();
	if(!ext2) {
		ERRPRINT(("ext2_mount error creating ext2 struct.\n"));
		err = ENOMEM;
		goto error;
	}
	
	DEBUGPRINT(FS, 5, ("ext2_mount just created ext2 struct.\n"));
		
	ext2->device_name = (char *)malloc(strlen(devname)+1);
	if(!ext2->device_name) {
		ERRPRINT(("ext2_mount cannot allocate space for storing the device name.\n"));
		err = ENOMEM;
		goto error1;
	}
	strcpy(ext2->device_name, devname); 
	ext2->nsid = nsid;
	
	// open read-only for now
	DEBUGPRINT(FS, 5, ("ext2_mount opening device read-only for inspection\n"));
	ext2->fd = open(ext2->device_name, O_RDONLY | O_BINARY);
	if (ext2->fd < 0) {
		ERRPRINT(("ext2_mount unable to open %s\n", ext2->device_name));
		err = ENODEV;
		goto error1;
	}
	
	// get device characteristics
	if (ioctl(ext2->fd, B_GET_GEOMETRY, &geo) < 0) {
		struct stat st;
		if ((fstat(ext2->fd, &st) >= 0) && S_ISREG(st.st_mode)) {
			// support mounting disk images
			geo.bytes_per_sector = 0x200;
			geo.sectors_per_track = 1;
			geo.cylinder_count = st.st_size / 0x200;
			geo.head_count = 1;
			geo.read_only = !(st.st_mode & S_IWUSR);
			geo.removable = true;
			if (ioctl(ext2->fd, 10000, NULL) < 0) 
				ERRPRINT(("warning, could not make file uncacheable.\n"));
		} else {
			ERRPRINT(("error getting device geometry\n"));
			err = EBADF;
			goto error1;
		}
	}
	if ((geo.bytes_per_sector != 0x200) && (geo.bytes_per_sector != 0x400)
		 && (geo.bytes_per_sector != 0x800) && (geo.bytes_per_sector != 0x1000)) {
		ERRPRINT(("ext2_mount unsupported device block size (%lu)\n", geo.bytes_per_sector));
		err = EBADF;
		goto error1;
	}
	if (geo.removable) {
		DEBUGPRINT(FS, 4, ("ext2_mount %s is removable\n", ext2->device_name));
		ext2->is_removable = true;
	}
#ifndef RO
	if (geo.is_read_only) {
		ext2->is_read_only = true;
	} else {
		if(flags & B_MOUNT_READ_ONLY)
			ext2->is_read_only = true;
		else
			ext2->is_read_only = false;
	}				
#else
	ext2->is_read_only = true;
#endif	
		
	DEBUGPRINT(FS, 5, ("ext2_mount opening device.\n"));
		
	// Open the device, if needed
	if(!ext2->is_read_only) {
		close(ext2->fd);
		ext2->fd=open(ext2->device_name,O_RDWR | O_BINARY);
		if(ext2->fd < 0) {		
			ERRPRINT(("ext2_mount device open failed on %s\n", ext2->device_name));
			err = ENODEV;
			goto error1;
		}
	}

	// Set non-removable flag
	if (flags & B_FS_IS_REMOVABLE)
		lock_removable_device(ext2->fd, true);

	// Read in the superblock
	DEBUGPRINT(FS, 5, ("ext2_mount reading superblock.\n"));
	err = read_pos(ext2->fd, 1024, &ext2->sb, 1024);	
	DEBUGPRINT(FS, 5, ("ext2_mount read %d bytes.\n", (int)err));
	if(err != 1024) {
		ERRPRINT(("ext2_mount error loading superblock from device %s\n", ext2->device_name));
		err = EBADF;
		goto error1;
	}		

	// Need to byte-swap stuff around
	LENDIAN_TO_HOST32(ext2->sb.s_inodes_count);
	LENDIAN_TO_HOST32(ext2->sb.s_blocks_count);
	LENDIAN_TO_HOST32(ext2->sb.s_r_blocks_count);
	LENDIAN_TO_HOST32(ext2->sb.s_free_blocks_count);
	LENDIAN_TO_HOST32(ext2->sb.s_free_inodes_count);
	LENDIAN_TO_HOST32(ext2->sb.s_first_data_block);
	LENDIAN_TO_HOST32(ext2->sb.s_log_block_size);
	LENDIAN_TO_HOST32(ext2->sb.s_log_frag_size);
	LENDIAN_TO_HOST32(ext2->sb.s_blocks_per_group);
	LENDIAN_TO_HOST32(ext2->sb.s_frags_per_group);
	LENDIAN_TO_HOST32(ext2->sb.s_inodes_per_group);
	LENDIAN_TO_HOST32(ext2->sb.s_mtime);
	LENDIAN_TO_HOST32(ext2->sb.s_wtime);
	LENDIAN_TO_HOST16(ext2->sb.s_mnt_count);
	LENDIAN_TO_HOST16(ext2->sb.s_max_mnt_count);
	LENDIAN_TO_HOST16(ext2->sb.s_magic);
	LENDIAN_TO_HOST16(ext2->sb.s_state);
	LENDIAN_TO_HOST16(ext2->sb.s_errors);
	LENDIAN_TO_HOST16(ext2->sb.s_minor_rev_level);
	LENDIAN_TO_HOST32(ext2->sb.s_lastcheck);
	LENDIAN_TO_HOST32(ext2->sb.s_checkinterval);
	LENDIAN_TO_HOST32(ext2->sb.s_creator_os);
	LENDIAN_TO_HOST32(ext2->sb.s_rev_level);
	LENDIAN_TO_HOST16(ext2->sb.s_def_resuid);
	LENDIAN_TO_HOST16(ext2->sb.s_def_resgid);
	// The next few fields are for EXT2_DYNAMIC_REV filesystems only.
	LENDIAN_TO_HOST32(ext2->sb.s_first_ino);
	LENDIAN_TO_HOST16(ext2->sb.s_inode_size);
	LENDIAN_TO_HOST16(ext2->sb.s_block_group_nr);
	LENDIAN_TO_HOST32(ext2->sb.s_feature_compat);
	LENDIAN_TO_HOST32(ext2->sb.s_feature_incompat);
	LENDIAN_TO_HOST32(ext2->sb.s_feature_ro_compat);
	LENDIAN_TO_HOST32(ext2->sb.s_algorithm_usage_bitmap);		
					
	// Check to make sure the magic number is right
	if(ext2->sb.s_magic != 0xEF53) {
		// Must not be a linux partition, abort
		ERRPRINT(("ext2_mount superblock's magic number doesn't match, read 0x%x.\n", ext2->sb.s_magic));
		err = EBADF;
		goto error1;
	}
	
	// Check for revision level
	// We don't know about anything greater than EXT2_DYNAMIC_REV
	if(ext2->sb.s_rev_level > EXT2_MAX_SUPP_REV) {
		ERRPRINT(("ext2_mount revision level of volume is too high (%d)\n", ext2->sb.s_rev_level));
		err = EBADF;
		goto error1;
	}

	// Check for RO incompatible flags
	// if, after masking out the ones we can handle, there are bits left in the flag,
	// don't mount it.
	if(ext2->sb.s_rev_level == EXT2_DYNAMIC_REV) {
		if((ext2->sb.s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPP) != 0) {
			ERRPRINT(("ext2_mount incompatible features enabled on this volume.\n"));
			err = EBADF;
			goto error1;
		}
	
		// print some pretty info
		DEBUGPRINT(FS, 4, ("ext2_mount volume revision level: %d\n", ext2->sb.s_rev_level));
		DEBUGPRINT(FS, 4, ("ext2_mount volume feature flags:\n"));
		if(ext2->sb.s_feature_compat & EXT2_FEATURE_COMPAT_DIR_PREALLOC)
			DEBUGPRINT(FS, 4, ("\tdirectory preallocation\n"));
		if(ext2->sb.s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE)
			DEBUGPRINT(FS, 4, ("\tfiletype in directory\n"));
		if(ext2->sb.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)
			DEBUGPRINT(FS, 4, ("\tsparse superblock\n"));
		if(ext2->sb.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
			DEBUGPRINT(FS, 4, ("\tlarge file\n"));
		if(ext2->sb.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
			DEBUGPRINT(FS, 4, ("\tbtree dir\n"));
		
		// XXX - check for RO compatible flags if this driver goes read/write
	}
	
	// It's read in, now lets calculate some filesystem constants
	ext2->info.s_block_size = (1024 << ext2->sb.s_log_block_size);
	if((ext2->info.s_block_size != 1024) && (ext2->info.s_block_size != 2048) && (ext2->info.s_block_size != 4096)) {
		ERRPRINT(("ext2_mount block size is invalid. read %d\n", ext2->info.s_block_size));
		err = EBADF;
		goto error1;
	}
	ext2->info.s_inodes_per_block = ext2->info.s_block_size / sizeof(ext2_inode);
	if((ext2->info.s_blocks_per_group = ext2->sb.s_blocks_per_group)==0) {
		err = EBADF;
		goto error1;
	}
	if((ext2->info.s_inodes_per_group = ext2->sb.s_inodes_per_group)==0) {
		err = EBADF;
		goto error1;
	}
	ext2->info.s_desc_per_block = ext2->info.s_block_size / sizeof(ext2_group_desc);
	ext2->info.s_groups_count = ext2->sb.s_blocks_count / ext2->sb.s_blocks_per_group;
	if((ext2->sb.s_blocks_count % ext2->sb.s_blocks_per_group) > 0) {
		// There are a few blocks in the last group, so one more group
		ext2->info.s_groups_count++;
	}
	ext2->info.s_num_sectors = ext2->sb.s_blocks_count * (2 << ext2->sb.s_log_block_size);

	DEBUGPRINT(FS, 4, ("ext2_mount block size is %d bytes.\n", ext2->info.s_block_size));
	DEBUGPRINT(FS, 4, ("ext2_mount there are %d blocks.\n", ext2->sb.s_blocks_count));
	// Now, lets read in the group descriptors
	DEBUGPRINT(FS, 4, ("ext2_mount reading group descriptors, count = %d.\n", ext2->info.s_groups_count));
	ext2->gd = (ext2_group_desc *)malloc(sizeof(ext2_group_desc) * ext2->info.s_groups_count);
	if(!ext2->gd) {
		ERRPRINT(("ext2_mount error allocating memory for group descriptors.\n"));
		err = ENOMEM;
		goto error1;
	}
	// calculate the position of the first group descriptor table
	if(ext2->info.s_block_size == 4096) {
		start = 4096;
	} else {
		start = 2048;
	}
	{
		size_t length = sizeof(ext2_group_desc) * ext2->info.s_groups_count;
		err = read_pos(ext2->fd, start, ext2->gd, length);
		if(err != length) {
			ERRPRINT(("ext2_mount error loading group descriptors from device %s\n", ext2->device_name));
			err = EBADF;
			goto error1;
		}
	}
#if __BIG_ENDIAN
	// If we're BIG ENDIAN, loop through and byte-swap all of the group descriptors
	{
		unsigned int i;
		for(i=0; i < ext2->info.s_groups_count; i++) {
			LENDIAN_TO_HOST32(ext2->gd[i].bg_block_bitmap);
			LENDIAN_TO_HOST32(ext2->gd[i].bg_inode_bitmap);
			LENDIAN_TO_HOST32(ext2->gd[i].bg_inode_table);
			LENDIAN_TO_HOST16(ext2->gd[i].bg_free_blocks_count);
			LENDIAN_TO_HOST16(ext2->gd[i].bg_free_inodes_count);
			LENDIAN_TO_HOST16(ext2->gd[i].bg_used_dirs_count);
		}
	}
#endif
	
#ifndef RO	
	// Allocate the space for the group descriptor dirty flags
	ext2->gd_to_flush = (bool *)malloc(sizeof(bool) * ext2->info.s_groups_count);
	if(!ext2->gd_to_flush) {
		ERRPRINT(("ext2_mount error allocating memory for group descriptor dirty flags.\n"));
		err = ENOMEM;
		goto error1;
	}
	memset(&ext2->gd_to_flush[0], 0, sizeof(bool) * ext2->info.s_groups_count);
#endif

	// Initialize the cache
	err = ext2_init_cache(ext2);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_mount error initializing cache.\n"));
		err = ENOMEM;
		goto error1;
	}

	// Ok, now lets read in the root inode, just to be nice and get the inode list going.
	err = ext2_read_vnode(ext2, ext2->root_vnid, 1, (void *)&newNode);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_mount error loading root vnode.\n"));
		goto error1;
	}

	DEBUGPRINT(FS, 4, ("ext2_mount root inode is %Ld.\n", ext2->root_vnid));
	
	err = new_vnode (nsid, ext2->root_vnid, newNode);
	if(err < B_NO_ERROR) {
		err = ENOMEM;
		goto error1;
	}
	
	*data=ext2;
	*vnid=ext2->root_vnid;

	DEBUGPRINT(FS, 4, ("ext2_mount finished mounting fs.\n"));

	return B_NO_ERROR;
	
error1:
	if(ext2) ext2_remove_ext2_struct(ext2);
error:
	return (err >= B_NO_ERROR) ? EINVAL : err;
	
}

// This function is called by the fsil whenever it needs to bring a vnode into existance for
// any reason that requires one. I use it as an opportunity to add the vnode to the global vnode list.
// It is called when I use get_vnode in the fs_walk function, so it is reentrant.
int ext2_read_vnode(void *ns, vnode_id vnid, char r, void **node)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode *v;
	status_t err = B_NO_ERROR;

	CHECK_FS_STRUCT(ext2, "ext2_read_vnode");

	DEBUGPRINT(VNODE, 5, ("ext2_read_vnode reading vnode %Ld, reentry = %d.\n", vnid, r));

	// create the vnode struct
	v = (vnode *)malloc(sizeof(vnode));
	if(!v) {
		ERRPRINT(("ext2_read_vnode had problem allocating space for vnode.\n"));
		return ENOMEM;
	}
	// Initialize the vnode
	err = ext2_initialize_vnode(v, vnid);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_read_vnode had problem initializing vnode.\n"));
		return err;
	}

	// Read in the inode
	err = ext2_read_inode(ext2, &v->inode, v->vnid);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_read_vnode failed to load inode.\n"));
		ext2_deallocate_vnode(v);
		return err;
	}
	
	// Calculate the number of file blocks
	if(v->inode.i_size == 0) v->num_file_blocks = 0;
	else v->num_file_blocks = (v->inode.i_size-1)/ext2->info.s_block_size + 1;
	
	*node = v;

	return err;
}

// This function returns information about the filesystem.
int ext2_rfsstat(void *ns, struct fs_info *info)
{
	ext2_fs_struct 	*ext2 = ns;

	CHECK_FS_STRUCT(ext2, "ext2_rfsstat");

	DEBUGPRINT(FS, 5, ("ext2_rfsstat entry.\n"));
												
	info->flags=B_FS_IS_PERSISTENT | B_FS_HAS_MIME; // | B_FS_HAS_ATTR;
	if(ext2->is_read_only) info->flags |=  B_FS_IS_READONLY;
	if(ext2->is_removable) info->flags |= B_FS_IS_REMOVABLE;
	strcpy (info->device_name,ext2->device_name);
	
	// Calculate the volume name. 
	if (!ext2->volume_name) {
		ext2->volume_name = (char *)malloc(16+1);
		if(!ext2->volume_name) {
			ERRPRINT(("ext2_rfsstat could not allocate space for volume name. \n"));
			return ENOMEM;
		}
		if(strlen(ext2->sb.s_volume_name) >0) {
			memcpy(ext2->volume_name, ext2->sb.s_volume_name, sizeof(ext2->sb.s_volume_name));
			ext2->volume_name[sizeof(ext2->sb.s_volume_name)] = '\0';
		} else {
			strcpy(ext2->volume_name, "ext2 untitled");
		}	
	}
		
	strcpy (info->volume_name, ext2->volume_name);	
	strcpy (info->fsh_name,"ext2");

	info->io_size=ext2->info.s_block_size;
	info->block_size=ext2->info.s_block_size;
	info->total_blocks=ext2->sb.s_blocks_count;
	info->free_blocks=ext2->sb.s_free_blocks_count;
	info->total_nodes=ext2->sb.s_inodes_count;
	info->free_nodes=ext2->sb.s_free_inodes_count;

	return B_NO_ERROR;
}

// This is a no-brainer.
int	ext2_unmount(void *ns)
{
	ext2_fs_struct 	*ext2 = ns;

	CHECK_FS_STRUCT(ext2, "ext2_unmount");

	DEBUGPRINT(FS, 5, ("ext2_unmount entry.\n"));
	
	ext2_remove_ext2_struct(ext2);
		
#ifdef USE_DMALLOC
	DEBUGPRINT(FS, 4, ("ext2_unmount checking memory usage to see if there are any leftover blocks.\n"));
	check_mem();
	DEBUGPRINT(FS, 4, ("ext2_unmount memory check complete.\n"));
#endif		
		
	return B_NO_ERROR;
}

// This function is called whenever the fsil figures it doesn't need a vnode anymore.
// The vnode is supposed to be deleted.
int	ext2_write_vnode(void *ns, void *node, char r)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;

	CHECK_FS_STRUCT(ext2, "ext2_write_vnode");
	CHECK_VNODE(v, "ext2_write_vnode");
	
	DEBUGPRINT(VNODE, 5, ("ext2_write_vnode called on node %Ld, reentry = %d.\n", v->vnid, r));

	return ext2_deallocate_vnode(v);
}

#ifndef RO
// When you want to remove a vnode. I'm still a bit unclear on this one. fs_unlink removes
// the file entry, but this one removes the vnode? I figured that fs_unlink would get to
// decide for itself. It probably does, it just called remove_node, and the fsil will call this.
int	ext2_remove_vnode(void *ns, void *node, char r)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;

	TOUCH(ext2); TOUCH(v);
	
	if(!ext2 ||  CHECK_E2FS_MAGIC(ext2)<B_OK) {
		ERRPRINT(("ext2_remove_vnode was passed bogus fs struct.\n"));
		return EINVAL;
	}

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_remove_vnode called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(VNODE, 5, ("ext2_remove_vnode called on node %Ld, reentry = %d.\n", v->vnid, r));
	
	return B_ERROR;
}

// Sync's the entire filesystem
int	ext2_sync(void *ns)
{
	ext2_fs_struct 	*ext2 = ns;

	TOUCH(ext2);
	
	if(!ext2 ||  CHECK_E2FS_MAGIC(ext2)<B_OK) {
		ERRPRINT(("ext2_sync was passed bogus fs struct.\n"));
		return EINVAL;
	}

	DEBUGPRINT(FS, 5, ("ext2_sync called.\n"));

	return B_NO_ERROR;
}

// This function sets information about the filesystem. Also where the volume label can be set if 
// it is compiled in read/write mode.
int ext2_wfsstat(void *ns, struct fs_info *info, long mask)
{
	ext2_fs_struct 	*ext2 = ns;

	TOUCH(ext2);
	
	if(!ext2 ||  CHECK_E2FS_MAGIC(ext2)<B_OK) {
		ERRPRINT(("ext2_wfsstat was passed bogus fs struct.\n"));
		return EINVAL;
	}

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_wfsstat called on Read Only filesystem.\n"));
		return EROFS;
	}
			
	DEBUGPRINT(FS, 5, ("ext2_wfsstat entry.\n"));			

	if(mask == WFSSTAT_NAME) {
		char temp_name[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		
		DEBUGPRINT(FS, 6, ("ext2_wfsstat called with WFSSTAT_NAME mask.\n"));
		// Lets check to make sure it is reasonably long
		if(strlen(info->volume_name) > sizeof(ext2->sb.s_volume_name)) {
			// It's too long
			return B_ERROR;
		}
		// We need to change the name of the volume
		strcpy(temp_name, info->volume_name);
		memcpy(ext2->sb.s_volume_name, info->volume_name, sizeof(ext2->sb.s_volume_name));
		memcpy(ext2->volume_name, temp_name, sizeof(temp_name));
		// Lets write it to the cache
		ext2_write_superblock(ext2);
	}
			
	return B_NO_ERROR;
}


#endif
