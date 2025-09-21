#include "ext2.h"
#include "io.h"
#include "prealloc.h"
#include "perm.h"
#include "file_extent.h"

int	ext2_access(void *ns, void *node, int mode)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;
	int retval;

	CHECK_FS_STRUCT(ext2, "ext2_access");
	CHECK_VNODE(v, "ext2_access");
	
	DEBUGPRINT(FILE, 5, ("ext2_access called on node %Ld with mode 0x%x.\n", v->vnid, mode));

	LOCKVNODE(v, 1);

	retval = ext2_check_access(ext2, v, mode);
	
	UNLOCKVNODE(v, 1);
	
	if(retval == 0)	return EACCES;

	return B_OK;
}

// This is called by the fsil whenever the file needs to be closed.
int	ext2_close(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;
	filecookie *f = (filecookie *)cookie;

	bool flush_data = false;

	CHECK_FS_STRUCT(ext2, "ext2_close");
	CHECK_VNODE(v, "ext2_close");
	CHECK_COOKIE(f, "ext2_close");

	DEBUGPRINT(FILE, 5, ("ext2_close called on node %Ld.\n", v->vnid));

	LOCKVNODE(v, 1);
	
	// Unset the open mode
	switch(f->omode) {
		case O_RDWR:
			flush_data = true;
			v->wopen--;
		case O_RDONLY:
			v->ropen--;
			break;
		case O_WRONLY:
			v->wopen--;
			flush_data = true;
	}

#ifndef RO
	if(!ext2->is_read_only)
		if(flush_data) {
			// If no one else has the file open for write, clear the preallcated blocks
			if(v->wopen == 0) {
				CleanUpPreAlloc(ext2, v);
			}
			v->pre_pos = -1;
			// Write the superblock
			ext2_write_superblock(ext2);
			// Write the modified group descriptors
			ext2_write_all_dirty_group_desc(ext2);	
		}
#endif
	
	UNLOCKVNODE(v, 1);

	return B_NO_ERROR;
}

// The fsil calls fs_free_cookie whenever it feels that the cookie can be safely
// deleted. Notice that it is seperate from the fs_close() call. 
int ext2_free_cookie(void *ns, void *node, void *cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;

	CHECK_FS_STRUCT(ext2, "ext2_free_cookie");
	CHECK_VNODE(v, "ext2_free_cookie");
	CHECK_COOKIE(cookie, "ext2_free_cookie");
		
	DEBUGPRINT(FILE, 5, ("ext2_free_cookie freeing cookie from node %Ld.\n", v->vnid));	

	// Lookie, free cookie. :)
	free(cookie);
	
	return B_NO_ERROR;
}

int	ext2_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	filecookie *f = (filecookie *)cookie;

	status_t err = B_NO_ERROR;

	CHECK_FS_STRUCT(ext2, "ext2_ioctl");
	CHECK_VNODE(v, "ext2_ioctl");
	CHECK_COOKIE(f, "ext2_ioctl");

	LOCKVNODE(v, 1);

	switch(cmd) {
		case 10001: // return the filesystem block that contains the byte number passed
		{
			uint32 block;
			uint64 *temp = buf;
			block = (*(uint32 *)buf) / ext2->info.s_block_size;
			*temp = 0; // zero out all 8 bytes of the return buffer
			err = ext2_get_block_num_from_extent(ext2, v, buf, block);
			if(err < B_NO_ERROR) {
				ERRPRINT(("ext2_ioctl had error getting block number from block table.\n"));
			}			
			break;	
		}
		case 10002 : // return real creation time
		{		
			if (buf) *(bigtime_t *)buf = v->inode.i_ctime << 16;
			break;
		}
		case 10003 : // return real last modification time
		{
			if (buf) *(bigtime_t *)buf = v->inode.i_mtime << 16;
			break;
		}
		default:
			err = EINVAL;
			ERRPRINT(("ext2_ioctl was passed invalid command (%d).\n", cmd));
	}

	UNLOCKVNODE(v, 1);
	
	return err;

}

// ext2_open is called whenever a read/write is going to happen. Allocates the file cookie.
int	ext2_open(void *ns, void *node, int omode, void **cookie)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	filecookie *f;

	CHECK_FS_STRUCT(ext2, "ext2_open");
	CHECK_VNODE(v, "ext2_open");

	DEBUGPRINT(FILE, 5, ("ext2_open called on vnode %Ld, open mode = 0x%x.\n", v->vnid, omode));

	// XXX - Handle O_APPEND later

	if(ext2->is_read_only) {
		// Allow no write-like modes if RO filesystem
		if((omode & O_WRONLY) || (omode & O_RDWR) || (omode & O_TRUNC)) {
			return EROFS;
		}
	}
	
	// Allow no write-like modes on directories
	if(S_ISDIR(v->inode.i_mode)) {
		if((omode & O_WRONLY) || (omode & O_RDWR) || (omode & O_TRUNC)) {
			return EISDIR;
		}
	}

	LOCKVNODE(v, 1);
	
	// Check permissions	
	if (ext2_check_permission(ext2, v, omode) == 0)
		return EACCES;	
		
	// Set the open mode
	switch(omode) {
		case O_RDWR:
			v->ropen++;
		case O_WRONLY:
			v->wopen++;
			break;
		case O_RDONLY:
			v->ropen++;
	}
	
	// Make a new cookie.
	f=(filecookie *)malloc(sizeof(filecookie));
	if(!f) {
		ERRPRINT(("ext2_open couldn't allocate memory for cookie.\n"));
		UNLOCKVNODE(v, 1);
		return ENOMEM;
	}

	UNLOCKVNODE(v, 1);

	*cookie=f;

	// Set the open mode	
	f->omode = omode;
	
	return B_NO_ERROR;
}

// This function is called whenever the fsil wants to read a file. Pretty simple stuff
int ext2_read(void *ns, void *node, void *cookie, off_t pos, void *buf, size_t *len)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	filecookie *f = (filecookie *)cookie;
	size_t length_to_read;
	status_t err;

	CHECK_FS_STRUCT(ext2, "ext2_read");
	CHECK_VNODE(v, "ext2_read");
	CHECK_COOKIE(f, "ext2_read");

	DEBUGPRINT(FILE, 5, ("ext2_read called on vnode %Ld, starting pos %Ld length %ld.\n", v->vnid, pos, *len));	

	// Check if it's a directory
	if(S_ISDIR(v->inode.i_mode)) {
		*len = 0;
		return EISDIR;
	}
		
	// If it's not a regular file, we can't read from it
	if(!S_ISREG(v->inode.i_mode)) {
		*len = 0;
		return B_NO_ERROR;
	}

	// Check the permissions first
	if(f->omode == O_WRONLY) {
		ERRPRINT(("ext2_read tried to read from a file in write only mode.\n"));
		*len = 0;
		return EACCES;
	}

	// Check position
	if(pos < 0) {
		*len = 0;
		return EINVAL;
	}

	LOCKVNODE(v, 1);

	if(v->inode.i_size < pos) {
		*len = 0;
		err = B_NO_ERROR;
		goto out;
	}

	// Make sure the passed read_len isn't too big.	
	length_to_read = min(*len,v->inode.i_size - pos);

	DEBUGPRINT(FILE, 6, ("ext2_read calling ext2_read_file with length %ld.\n", length_to_read));

	// Let ext2_read_file handle it.	
	err = ext2_read_file(ext2, v, buf, pos, length_to_read, len);

out:
	UNLOCKVNODE(v, 1);

	return err;
}

// This function simply returns a string containing the name of the file this symlink is pointing to.
int	ext2_readlink(void *ns, void *node, char *buf, size_t *bufsize)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	status_t		err = B_NO_ERROR;

	CHECK_FS_STRUCT(ext2, "ext2_readlink");
	CHECK_VNODE(v, "ext2_readlink");
	
	DEBUGPRINT(FILE, 5, ("ext2_readlink called on vnode %Ld.\n", v->vnid)); 

	LOCKVNODE(v, 1);

	// Check to see if the buffer is big enough
	if(v->inode.i_size > *bufsize) {
		// The buffer is too small
		err = ENAMETOOLONG;
		goto out;
	}

	// Check for fast symlink and process accordingly
	if(v->inode.i_size <= sizeof(int)*EXT2_N_BLOCKS) {
	#if __BIG_ENDIAN
		// If it's a big endian machine, the blocks in the inode have already been byte-swapped
		// We need to swap them back, or the name will be mangled.
		// I've seen it, and it ain't pretty.
		int temp_blocks[EXT2_N_BLOCKS];
		int i;

		memcpy(&temp_blocks, v->inode.i_block, sizeof(int)*EXT2_N_BLOCKS);
		for(i=0; i<EXT2_N_BLOCKS; i++) {
			HOST_TO_LENDIAN32(temp_blocks[i]);
		}	
		memcpy(buf, &temp_blocks, v->inode.i_size);
	#else
		memcpy(buf, v->inode.i_block, v->inode.i_size);
	#endif
	} else {
		// This must be a slow symbolic link
		// Read in the first block, this contains the full path
		char *temp;
		// Check for bad block. If it's too big, it'll get caught in get_cache_block 
		if((v->inode.i_block[0] == 0)) {
			ERRPRINT(("ext2_readlink the block containing the slow symlink is 0. ummm...\n"));
			err = EINVAL;
			goto out;
		}
		temp = (char *)ext2_get_cache_block(ext2, v->inode.i_block[0]);
		if(!temp) {
			ERRPRINT(("ext2_readlink had problem getting block from cache.\n"));
			err = EIO;
			goto out;
		}			
		memcpy(buf, temp, v->inode.i_size);
		ext2_release_cache_block(ext2, v->inode.i_block[0]);
	} 

	*bufsize = v->inode.i_size;

out:
	UNLOCKVNODE(v, 1);
	return err;

}

// This function returns information about the particular file.
int	ext2_rstat(void *ns, void *node, struct stat *st)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;

	CHECK_FS_STRUCT(ext2, "ext2_rstat");
	CHECK_VNODE(v, "ext2_rstat");

	DEBUGPRINT(FILE, 5, ("ext2_rstat entry called on vnode %Ld.\n", v->vnid));

	LOCKVNODE(v, 1);

	if(ext2->override_uid) {
		st->st_uid= ext2->uid; 
		st->st_gid= ext2->gid;
	} else {
		st->st_uid= v->inode.i_uid; 
		st->st_gid= v->inode.i_gid;
	}		

	st->st_dev=ext2->nsid;
	st->st_ino=v->vnid;
	st->st_nlink=v->inode.i_links_count;
	st->st_blksize=1024*64;	// Others just picked 64k.
	st->st_atime=v->inode.i_atime; // access time
	st->st_mtime=v->inode.i_mtime; // modify time
	st->st_ctime=v->inode.i_mtime; // change time ? How is this different from modify time?
	st->st_crtime=v->inode.i_ctime; // create time
	
	DEBUGPRINT(FILE, 6, ("ext2_rstat mode of file is ox%x.\n", v->inode.i_mode));

	// If in read-only mode, mask the rest out
	st->st_mode = v->inode.i_mode;
	if(ext2->is_read_only) {
		st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}
	
	DEBUGPRINT(FILE, 6, ("ext2_rstat inode says file is %d bytes long.\n", v->inode.i_size));
	if(S_ISLNK(st->st_mode)) {
		st->st_size = 0;
	} else {
		st->st_size = v->inode.i_size;
	}

	UNLOCKVNODE(v, 1);

	return B_NO_ERROR;
}


#ifndef RO
// Called when a new file is to be created
int	ext2_create(void *ns, void *dir, const char *name, int omode, int perms, vnode_id *vnid, void **cookie)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)dir;

	CHECK_FS_STRUCT(ext2, "ext2_create");
	CHECK_VNODE(v, "ext2_create");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_create called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(FILE, 5, ("ext2_create called on directory %Ld, ", v->vnid));
	DEBUGPRINT(FILE, 5, ("file '%s'\n", name));

	return B_ERROR;
}

// Sync's the file in question. I really need to implement this somehow.
int	ext2_fsync(void *ns, void *node)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)node;

	CHECK_FS_STRUCT(ext2, "ext2_fsync");
	CHECK_VNODE(v, "ext2_fsync");
	
	if(ext2->is_read_only) {
		ERRPRINT(("ext2_fsync called on Read Only filesystem.\n"));
		return EROFS;
	}
	
	DEBUGPRINT(FILE, 5, ("ext2_fsync called on node %Ld.\n", v->vnid));

	return B_NO_ERROR;
}

// Called whenever a hard link is requested. This is somewhat unknown territory, because
// none of the other filesystems on BeOS support this, so ext2 will be the first. I'm not 
// sure the kit supports it, either, so it may be impossible to link a file from user-mode.
int	ext2_link(void *ns, void *dir, const char *name, void *node)
{
	ext2_fs_struct 	*ext2 = (ext2_fs_struct *)ns;
	vnode			*v = (vnode *)dir;
	vnode			*bNode = (vnode *)node;

	CHECK_FS_STRUCT(ext2, "ext2_link");
	CHECK_VNODE(v, "ext2_link");
	CHECK_VNODE(bNode, "ext2_link");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_link called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(FILE, 5, ("ext2_link called on directory %Ld to link '%s' to  node %Ld.\n", v->vnid, name, bNode->vnid));
	return B_ERROR;
}

// link a file symbolically
int	ext2_symlink(void *ns, void *dir, const char *name, const char *path)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = dir;

	CHECK_FS_STRUCT(ext2, "ext2_symlink");
	CHECK_VNODE(v, "ext2_symlink");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_symlink called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(FILE, 5, ("ext2_symlink called on node %Ld.\n", v->vnid));
	return B_ERROR;
}

// Removes a file entry, not the vnode.
int	ext2_unlink(void *ns, void *dir, const char *name)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = dir;

	CHECK_FS_STRUCT(ext2, "ext2_unlink");
	CHECK_VNODE(v, "ext2_unlink");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_unlink called on Read Only filesystem.\n"));
		return EROFS;
	}

	DEBUGPRINT(FILE, 5, ("ext2_unlink called on node %Ld.\n", v->vnid));
	return B_ERROR;
}

// This function is called whenever the fsil wants to write a file. Pretty simple stuff, NOT!
// Of course it only sort of works if the fs is compiled in read/write mode.
int ext2_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;
	int blocks_to_add;
	int blocks_allocated;
	int bytes_over;
	filecookie *f = (filecookie *)cookie;

	CHECK_FS_STRUCT(ext2, "ext2_write");
	CHECK_VNODE(v, "ext2_write");
	CHECK_COOKIE(f, "ext2_write");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_write called on Read Only filesystem.\n"));
		return EROFS;
	}	

	DEBUGPRINT(FILE, 5, ("ext2_write called on vnode %Ld, starting pos %Ld length %ld.\n", v->vnid, pos, *len));

	if(S_ISDIR(v->inode.i_mode)) {
		DEBUGPRINT(FILE, 4, ("ext2_write called on a directory.\n"));	
		return EISDIR;
	}
	
	if(S_ISLNK(v->inode.i_mode)) {
		DEBUGPRINT(FILE, 4, ("ext2_write called on a symlink.\n"));
		return EPERM;
	}
	
	// Check the permissions first
	if(f->omode == O_RDONLY) {
		ERRPRINT(("ext2_write tried to write to a file open in read-only mode.\n"));
		return EACCES;
	}

	LOCKVNODE(v, MAX_READERS);

	// See if we need to add any file blocks
	{
		int old_blocks, new_blocks;
		bytes_over = ((pos + *len) - v->inode.i_size);
		
		old_blocks = v->inode.i_size / ext2->info.s_block_size;
		if(v->inode.i_size % ext2->info.s_block_size > 0) old_blocks++;
		
		new_blocks = (v->inode.i_size + bytes_over) / ext2->info.s_block_size;
		if((v->inode.i_size + bytes_over) % ext2->info.s_block_size > 0) new_blocks++;
	
		blocks_to_add = new_blocks - old_blocks;
		DEBUGPRINT(FILE, 7, ("ext2_write blocks_to_add = %d.\n", blocks_to_add));
	}
		
	if(blocks_to_add > 0) {
		// Allocate the blocks for the upcoming write
		if((blocks_allocated = ext2_allocate_blocks_for_file(ext2, v, blocks_to_add)) != blocks_to_add) 
			ERRPRINT(("ext2_write only able to allocate %d blocks, not %d.\n", blocks_allocated, blocks_to_add));	
	}
	
	{
		// Do the actual writing
		unsigned int toWrite;
		if(blocks_to_add > 0) {
			if(blocks_to_add == blocks_allocated) {
				toWrite = *len;
			} else {
				// This only happens if the code was unable to allocate all of the blocks
				// towrite = the new total space allocated to the file - write position. 
				int total_blocks = v->inode.i_size / ext2->info.s_block_size + blocks_allocated;
				toWrite = total_blocks * ext2->info.s_block_size - pos;
			}
		} else {
			toWrite = *len;
		}
		DEBUGPRINT(FILE, 7, ("ext2_write writing %d bytes to file.\n", toWrite));
		*len = ext2_write_file(ext2, v, (void *)buf, pos, toWrite);
	}

	// Set some stuff in the inode
	{
		// Update the size field in the inode, if more data has been put on it. Not sure if it should be done here.	
		uint32 len_maybe = (pos + *len);
		if(len_maybe > v->inode.i_size) {
			v->inode.i_size = (pos + *len);
		}
	}

	// Notify the node monitor
	notify_listener(B_STAT_CHANGED, ext2->nsid, 0, 0, v->vnid, NULL);
	
	UNLOCKVNODE(v, MAX_READERS);
	
	// Write the inode
	ext2_write_inode(ext2, &v->inode, v->vnid);

	if(blocks_to_add) {
		// Write the superblock
		ext2_write_superblock(ext2);
		// Write the modified group descriptors
		ext2_write_all_dirty_group_desc(ext2);
	}

	return B_NO_ERROR;

}

// This function sets information about the particular file.
int	ext2_wstat(void *ns, void *node, struct stat *st, long mask)
{
	ext2_fs_struct 	*ext2 = ns;
	vnode			*v = node;

	CHECK_FS_STRUCT(ext2, "ext2_wstat");
	CHECK_VNODE(v, "ext2_wstat");

	if(ext2->is_read_only) {
		ERRPRINT(("ext2_wstat called on Read Only filesystem.\n"));
		return EROFS;
	}
	
	LOCKVNODE(v, MAX_READERS);

	switch(mask) {
		case WSTAT_MODE:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_MODE.\n", v->vnid));
			break;
		case WSTAT_UID:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_UID.\n", v->vnid));
			break;
		case WSTAT_GID:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_GID.\n", v->vnid));
			break;
		case WSTAT_SIZE:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_SIZE.\n", v->vnid));
			break;
		case WSTAT_ATIME:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_ATIME.\n", v->vnid));
			break;
		case WSTAT_MTIME:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_MTIME.\n", v->vnid));
			break;
		case WSTAT_CRTIME:
			DEBUGPRINT(FILE, 5, ("ext2_wstat called on node %Ld with mask WSTAT_CRTIME.\n", v->vnid));
			break;
	}
	
	// Should put the appropriate notification calls here.	

	UNLOCKVNODE(v, MAX_READERS);
	
	return B_NO_ERROR;
}
#endif

