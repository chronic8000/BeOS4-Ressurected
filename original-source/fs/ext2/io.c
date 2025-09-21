// Miscellaneous functions that I haven't put somewhere else. 
#include "ext2.h"
#include "io.h"
#include "file_extent.h"

#ifdef DEBUG
int blocks_gotten = 0;
#endif

// This function reads in just an inode.
status_t ext2_read_inode(ext2_fs_struct *ext2, ext2_inode *inode, uint32 inode_num)
{
	unsigned int block, offset;
	char *temp;

	// Calculate the block and offset
	inode_num--;
	block = ext2->gd[inode_num / ext2->info.s_inodes_per_group].bg_inode_table;
	block += (inode_num % ext2->info.s_inodes_per_group) / ext2->info.s_inodes_per_block;
	offset = (inode_num % ext2->info.s_inodes_per_block) * sizeof(ext2_inode);

	if(block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_read_inode invalid cache block %d.\n", block));
		return B_ERROR;
	}

	DEBUGPRINT(IO, 5, ("ext2_read_inode reading inode %ld, in group %ld, found at block %d, offset %d.\n",
		inode_num+1, inode_num / ext2->info.s_inodes_per_group, block, offset));

	// Get a pointer to the block and copy the data
	temp = (char *)ext2_get_cache_block(ext2, block);
	if(!temp) {
		ERRPRINT(("ext2_read_inode failed to get needed cache block (%d).\n", block));
		return B_ERROR;
	}
	memcpy(inode, temp + offset, sizeof(ext2_inode));
	ext2_release_cache_block(ext2, block);

#if __BIG_ENDIAN
	// Byte-swap the inode
	LENDIAN_TO_HOST16(inode->i_mode);
	LENDIAN_TO_HOST16(inode->i_uid);
	LENDIAN_TO_HOST32(inode->i_size);
	LENDIAN_TO_HOST32(inode->i_atime);
	LENDIAN_TO_HOST32(inode->i_ctime);
	LENDIAN_TO_HOST32(inode->i_mtime);
	LENDIAN_TO_HOST32(inode->i_dtime);
	LENDIAN_TO_HOST16(inode->i_gid);
	LENDIAN_TO_HOST16(inode->i_links_count);
	LENDIAN_TO_HOST32(inode->i_blocks);
	LENDIAN_TO_HOST32(inode->i_flags);
	{
		int i;
		for(i=0; i<=EXT2_N_BLOCKS; i++) {
			LENDIAN_TO_HOST32(inode->i_block[i]);	
		}
	}
	LENDIAN_TO_HOST32(inode->i_version);
	LENDIAN_TO_HOST32(inode->i_file_acl);
	LENDIAN_TO_HOST32(inode->i_dir_acl);
	LENDIAN_TO_HOST32(inode->i_faddr);			
#endif
	
	DEBUGPRINT(IO, 6, ("ext2_read_inode file size %d.\n", inode->i_size));

	return B_NO_ERROR;
}


// This funnction simply fills the buffer from a file at starting point pos and length read_len. The only thing
//  it expects is a vnode with a valid file block table.
int ext2_read_file(ext2_fs_struct *ext2, vnode *file_vnode, void *buf, uint32 pos, size_t read_len, size_t *bytes_read)
{
	uchar *buf_pos = (uchar *)buf;
	uint32 block;
	uint32 fs_block;
    uint32 offset=0;
	size_t length_to_read=0;
	status_t err;

	DEBUGPRINT(IO, 5, ("ext2_read_file called with vnode %Ld, file position %ld, length %ld.\n",
		file_vnode->vnid, pos, read_len));

	*bytes_read = 0;

	// Calculate the starting block into the file to read.
	block = pos / ext2->info.s_block_size;
	// Calculate the offset into the block to read
	offset = pos % ext2->info.s_block_size;

	// Handle the partial first block
	if(offset != 0) {	
		uchar *temp_buf;
		length_to_read = min(read_len, ext2->info.s_block_size - offset);
		DEBUGPRINT(IO, 6, ("ext2_read_file reading partial first block @ offset %d into block %ld for %ld bytes.\n", (int)offset, block, length_to_read));			
		err = ext2_get_block_num_from_extent(ext2, file_vnode, &fs_block, block++);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_read_file had problem getting correct block from block table.\n"));
			return err;
		}
		if(fs_block > ext2->num_cache_blocks) {
			ERRPRINT(("ext2_read_file got an invalid block to read from the block list.\n"));
			return EIO;
		}
		if(fs_block != 0) {
			DEBUGPRINT(IO, 7, ("ext2_read_file reading buffer starting at block %ld + offset %d.\n", fs_block, (int)offset));
			temp_buf = ext2_get_cache_block(ext2, fs_block);
			if(!temp_buf) {
				ERRPRINT(("ext2_read_file had problem getting block from cache.\n"));
				return EIO;
			}
			memcpy(buf_pos, temp_buf + offset, length_to_read); 
			ext2_release_cache_block(ext2, fs_block);			
		} else {
			memset(buf, 0, length_to_read);
			DEBUGPRINT(IO, 7, ("ext2_read_file file block %ld is zero, filling buffer with zeros.\n", block));
		}  
		read_len -= length_to_read;
		*bytes_read += length_to_read;
		buf_pos += length_to_read;
	}

	// Handle the middle blocks
	while((read_len / ext2->info.s_block_size) > 0) {
		err = ext2_get_block_num_from_extent(ext2, file_vnode, &fs_block, block++);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_read_file had problem getting correct block from block table.\n"));
			return err;
		}
		if(fs_block > ext2->num_cache_blocks) {
			ERRPRINT(("ext2_read_file got an invalid block to read from the block list.\n"));
			return EIO;
		}
		if(fs_block != 0) {
			DEBUGPRINT(IO, 7, ("ext2_read_file reading buffer starting at fs block %ld.\n",  fs_block));	
			err = cached_read(ext2->fd, fs_block, buf_pos, 1, ext2->info.s_block_size);
			if(err < B_NO_ERROR) {
				ERRPRINT(("ext2_read_file cached_read returned error. Probably IO problem.\n"));
				return err;
			}
		} else {
			memset(buf_pos, 0, length_to_read);
			DEBUGPRINT(IO, 5, ("ext2_read_file file block %ld is zero, filling buffer with zeros.\n", block));
		}
		buf_pos += ext2->info.s_block_size;
		read_len -= ext2->info.s_block_size;
		*bytes_read += ext2->info.s_block_size;
	}

	// Handle the end blocks
	if(read_len > 0) {
		uchar *temp_buf;

		length_to_read = read_len;
		DEBUGPRINT(IO, 6, ("ext2_read_file reading partial last block %ld for %ld bytes.\n", block, length_to_read));			

		err = ext2_get_block_num_from_extent(ext2, file_vnode, &fs_block, block);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_read_file had problem getting correct block from block table.\n"));
			return err;
		}
		if(fs_block > ext2->num_cache_blocks) {
			ERRPRINT(("ext2_read_file got an invalid block to read from the block list.\n"));
			return EIO;
		}
		if(fs_block != 0) {
			DEBUGPRINT(IO, 7, ("ext2_read_file reading buffer starting at block %ld for %ld bytes.\n", fs_block, length_to_read));
			temp_buf = ext2_get_cache_block(ext2, fs_block);
			if(!temp_buf) {
				ERRPRINT(("ext2_read_file had problem getting block from cache.\n"));
				return EIO;
			}
			memcpy(buf_pos, temp_buf, length_to_read); 
			ext2_release_cache_block(ext2, fs_block);			
		} else {
			DEBUGPRINT(IO, 7, ("ext2_read_file file block %ld is zero, filling buffer with zeros.\n", block));
			memset(buf_pos, 0, length_to_read);
		}  
		*bytes_read += length_to_read;
	}


	DEBUGPRINT(IO, 6, ("ext2_read_file exiting, read %ld bytes.\n", *bytes_read));

	return B_NO_ERROR;

}

// This function will return a pointer to a block in the cache subsystem of the size of the cluster.
void *ext2_get_cache_block(ext2_fs_struct *ext2, uint32 cache_block)
{
	DEBUGPRINT(IO, 8, ("ext2_get_cache_block called on cache block %ld. Now %d blocks checked out.\n", cache_block, ++blocks_gotten));
	
	if(cache_block >= ext2->num_cache_blocks) {
		ERRPRINT(("ext2_get_cache_block asked to get bad block (%ld).\n", cache_block));
		return NULL;
	}
	
	return get_block(ext2->fd, cache_block, ext2->info.s_block_size);
}

// Gives the block back to the cache
status_t ext2_release_cache_block(ext2_fs_struct *ext2, uint32 cache_block)
{
	DEBUGPRINT(IO, 8, ("ext2_release_cache_block releasing cache block %ld. Now %d blocks checked out.\n", cache_block, --blocks_gotten));

	if(cache_block >= ext2->num_cache_blocks) {
		ERRPRINT(("ext2_release_cache_block asked to release bad block (%ld).\n", cache_block));
		return EINVAL;
	}

	return release_block(ext2->fd, cache_block);
}

// Basically the same as ext2_get_cache_block(), but it works on the nth block of a file.
void *ext2_get_file_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block)
{
	uint32 dev_block;
	status_t err;

	DEBUGPRINT(IO, 5, ("ext2_get_file_cache_block called on vnode %Ld, file block %ld.\n", file_vnode->vnid, file_block));

	err = ext2_get_block_num_from_extent(ext2, file_vnode, &dev_block, file_block);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_get_file_cache_block had error getting block from block map.\n"));
		return NULL;
	}
	DEBUGPRINT(IO, 7, ("ext2_get_file_cache_block dev_block=%ld\n", dev_block));
	if(dev_block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_get_file_cache_block invalid cache_block %ld calculated from block %ld of vnode %Ld.\n",
			dev_block, file_block, file_vnode->vnid));
		return NULL;
	}	
	return get_block(ext2->fd, dev_block, ext2->info.s_block_size);

}

// Releases the nth block for a file to the cache system
status_t ext2_release_file_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block)
{
	status_t err;
	uint32 block;
	
	err = ext2_get_block_num_from_extent(ext2, file_vnode, &block, file_block);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_release_file_cache_block had problem getting block from block list.\n"));
		return EIO;
	}
	if(block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_release_file_cache_block invalid cache_block %ld calculated from block %ld of vnode %Ld.\n",
			block, file_block, file_vnode->vnid));
		return B_ERROR;
	}	
	
	DEBUGPRINT(IO, 6, ("ext2_release_file_cache_block returning block %ld to the cache.\n", block));
	
	return release_block(ext2->fd, block);
}

#ifndef RO

// NOTE: This function expects all of the required file blocks to be loaded into the extents
int ext2_write_file(ext2_fs_struct *ext2, vnode *file_vnode, void *buf, unsigned int pos, unsigned int write_len)
{
	int block;
	off_t offset;
	size_t length_to_write;
	char *temp_buf;
	int bytes_written = 0;
	char *buf_pos;

	if(ext2->read_only) {
		ERRPRINT(("ext2_write_file called in read-only mode.\n"));
		return B_ERROR;
	}

	// Calculate the starting block into the file to write.
	block = pos/ext2->info.s_block_size;
	// Calculate the offset into the block to write
	offset = pos % ext2->info.s_block_size;

	if(offset != 0) {	
		int fs_block = 1;
		// It's into a block
		length_to_write = min(write_len, ext2->info.s_block_size - offset);
		
		if(fs_block != 0) {
			temp_buf = (char *)ext2_get_file_cache_block(ext2, file_vnode, block);			
			memcpy(temp_buf, buf, length_to_write);
			ext2_mark_file_block_dirty(ext2, file_vnode, block);
			ext2_release_file_cache_block(ext2, file_vnode, block);
//			retval = cached_read_pos(fd, fs_block * ext2->info.s_block_size + offset, buf, length_to_read);
//			DEBUGPRINT2("ext2_read_file reading buffer starting at block %d + offset %d.\n", fs_block, offset);
//			DEBUGPRINT1("ext2_read_file read_pos returned %d.\n", retval);
		} else {
			ERRPRINT(("ext2_write_file trying to write to a file with a zero block. FIX.\n"));
//			memset(buf, 0, length_to_read);
//			DEBUGPRINT1("ext2_read_file file block %d is zero, filling buffer with zeros.\n", block);
		}  
		write_len -= length_to_write;
		bytes_written += length_to_write;
	} else {
		block--;
	}

	buf_pos = (char *)buf;
	buf_pos+=offset;
	while(write_len>0) {
		length_to_write = min(ext2->info.s_block_size, write_len);
//		offset = GetBlockFromExtent(file_vnode, ++block)*ext2->info.s_block_size;
		offset = 1;
		if(1) {
			temp_buf = (char *)ext2_get_file_cache_block(ext2, file_vnode, ++block);
			memcpy(temp_buf, buf, length_to_write);
			ext2_mark_file_block_dirty(ext2, file_vnode, block);
			ext2_release_file_cache_block(ext2, file_vnode, block);			
//			retval = cached_read_pos(fd, offset, buf_pos, length_to_read);
//			DEBUGPRINT1("ext2_read_file reading buffer starting at  physical offset %ld.\n",  offset);	
//			DEBUGPRINT1("ext2_read_file read_pos returned %d.\n", retval);
		} else {
			memset(buf_pos, 0, length_to_write);
//			DEBUGPRINT1("ext2_read_file file block %d is zero, filling buffer with zeros.\n", block);
		}
		buf_pos += length_to_write;
		write_len -= length_to_write;
		bytes_written += length_to_write;
	}

//	DEBUGPRINT(IO, 5, ("ext2_write_file wrote %d bytes.\n", bytes_written));
	return bytes_written;
}

// opposite of ext2_read_inode
int ext2_write_inode(ext2_fs_struct *ext2, ext2_inode *inode, int inode_num)
{
	unsigned int block, offset;
	char *temp;
	ext2_inode tempinode;
	ext2_inode *inode_ptr;
	
	if(ext2->read_only) {
		ERRPRINT(("ext2_write_inode called in ReadOnly mode.\n"));
		return B_ERROR;
	}
	
	// Calculate the block and offset
	inode_num--;
	block = ext2->gd[inode_num / ext2->info.s_inodes_per_group].bg_inode_table;
	block += (inode_num % ext2->info.s_inodes_per_group) / ext2->info.s_inodes_per_block;
	offset = (inode_num % ext2->info.s_inodes_per_block) * sizeof(ext2_inode);

	if(block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_write_inode invalid cache block %d.\n", block));
		return B_ERROR;
	}	

#if __BIG_ENDIAN
	// Copy the inode to a new one
	memcpy(&tempinode, inode, sizeof(ext2_inode));
	inode_ptr = &tempinode;
	
	// Byte-swap the inode
	HOST_TO_LENDIAN16(tempinode.i_mode);
	HOST_TO_LENDIAN16(tempinode.i_uid);
	HOST_TO_LENDIAN32(tempinode.i_size);
	HOST_TO_LENDIAN32(tempinode.i_atime);
	HOST_TO_LENDIAN32(tempinode.i_ctime);
	HOST_TO_LENDIAN32(tempinode.i_mtime);
	HOST_TO_LENDIAN32(tempinode.i_dtime);
	HOST_TO_LENDIAN16(tempinode.i_gid);
	HOST_TO_LENDIAN16(tempinode.i_links_count);
	HOST_TO_LENDIAN32(tempinode.i_blocks);
	HOST_TO_LENDIAN32(tempinode.i_flags);
	{
		int i;
		for(i=0; i<=EXT2_N_BLOCKS; i++) 
			HOST_TO_LENDIAN32(tempinode.i_block[i]);	
	}
	HOST_TO_LENDIAN32(tempinode.i_version);
	HOST_TO_LENDIAN32(tempinode.i_file_acl);
	HOST_TO_LENDIAN32(tempinode.i_dir_acl);
	HOST_TO_LENDIAN32(tempinode.i_faddr);			
#else
	inode_ptr = inode;
#endif

	// Get a pointer to the block and copy the data
	temp = (char *)ext2_get_cache_block(ext2, block);
	memcpy(temp + offset, inode_ptr, sizeof(ext2_inode));
	ext2_mark_blocks_dirty(ext2, block, 1);
	ext2_release_cache_block(ext2, block);
	
	return B_NO_ERROR;
}	

// Writes all of the group descriptors that have the modified flag set.
int ext2_write_all_dirty_group_desc(ext2_fs_struct *ext2)
{
	unsigned int i;
	
	//DEBUGPRINT("ext2_write_all_dirty_group_desc called.\n");
	
	if(ext2->read_only)
		ERRPRINT(("ext2_write_all_dirty_group_desc called in ReadOnly mode.\n"));
	
	
	for(i=0; i<ext2->info.s_groups_count; i++) {
	//	DEBUGPRINT1("ext2_write_all_dirty_group_desc testing group %d.\n", i);
		if(ext2->gd_to_flush[i] == true) {
			ext2_write_group_desc(ext2, i);
		}
	}
	
	return B_NO_ERROR;

}

// Writes the group descriptor in memory to disk.
int ext2_write_group_desc(ext2_fs_struct *ext2, int group_num)
{
	unsigned int block, offset;
	char *temp;
	ext2_group_desc tempdesc, *desc_ptr;
	
//	DEBUGPRINT(IO, 5, ("ext2_write_group_desc called on group %d.\n", group_num));
	
	if(ext2->read_only) {
		ERRPRINT(("ext2_write_group_desc called in ReadOnly mode.\n"));
		return B_ERROR;
	}
	
	// Calculate the starting block
	if(ext2->info.s_block_size == 1024) {
		block = 2;
	} else {
		block = 1;
	}
	
	// See if it's in the next block, and push the block pointer up if necessary
	block += group_num / ext2->info.s_desc_per_block;
	
	// Calculate the offset into the block
	offset = (group_num % ext2->info.s_desc_per_block) * sizeof(ext2_group_desc);
	
	// Check for bad block. It'd be pretty hard to get this to happen, but what the hey...
	if(block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_write_group_desc invalid cache block %d.\n", block));
		return B_ERROR;
	}
	
//	DEBUGPRINT(IO, 5, ("Cext2::ext2_write_group_desc calculates block at block %d offset %d.\n", block, offset));

#if __BIG_ENDIAN
	// Copy the data to the temporary group descriptor
	memcpy(&tempdesc, &ext2->gd[group_num], sizeof(ext2_group_desc));
	desc_ptr = &tempdesc;

	// Do the byte-swapping
	HOST_TO_LENDIAN32(tempdesc.bg_block_bitmap);
	HOST_TO_LENDIAN32(tempdesc.bg_inode_bitmap);
	HOST_TO_LENDIAN32(tempdesc.bg_inode_table);
	HOST_TO_LENDIAN16(tempdesc.bg_free_blocks_count);
	HOST_TO_LENDIAN16(tempdesc.bg_free_inodes_count);
	HOST_TO_LENDIAN16(tempdesc.bg_used_dirs_count);
#else
	desc_ptr = &ext2->gd[group_num];
#endif	

	// Get a pointer to the block and copy the data
	temp = (char *)ext2_get_cache_block(ext2, block);
	memcpy(temp + offset, desc_ptr, sizeof(ext2_group_desc));
	ext2_mark_blocks_dirty(ext2, block, 1);
	ext2_release_cache_block(ext2, block);

	ext2->gd_to_flush[group_num] = false;

	return B_NO_ERROR;
}

// Need to fix this to write all of the copies of the main superblock. Or, maybe just write them all 
// before the filesystem is unmounted. Maybe just write all of them on sync, and call that before unmount.
int ext2_write_superblock(ext2_fs_struct *ext2)
{
	int block, offset;
	char *temp;
	ext2_super_block sb, *sb_ptr;
	
	if(ext2->read_only) {
		ERRPRINT(("ext2_write_superblock called in ReadOnly mode.\n"));
		return B_ERROR;
	}
	
	// Calculate block and offset
	if(ext2->info.s_block_size == 1024) {
		block = 1;
		offset = 0;
	} else {
		block = 0;
		offset = 1024;
	}

#if __BIG_ENDIAN
	// Copy the data to a temporary sb
	memcpy(&sb, &ext2->sb, sizeof(ext2_super_block));
	sb_ptr = &sb;
	
	// Do the byte-swapping
	HOST_TO_LENDIAN32(sb.s_inodes_count);
	HOST_TO_LENDIAN32(sb.s_blocks_count);
	HOST_TO_LENDIAN32(sb.s_r_blocks_count);
	HOST_TO_LENDIAN32(sb.s_free_blocks_count);
	HOST_TO_LENDIAN32(sb.s_free_inodes_count);
	HOST_TO_LENDIAN32(sb.s_first_data_block);
	HOST_TO_LENDIAN32(sb.s_log_block_size);
	HOST_TO_LENDIAN32(sb.s_log_frag_size);
	HOST_TO_LENDIAN32(sb.s_blocks_per_group);
	HOST_TO_LENDIAN32(sb.s_frags_per_group);
	HOST_TO_LENDIAN32(sb.s_inodes_per_group);
	HOST_TO_LENDIAN32(sb.s_mtime);
	HOST_TO_LENDIAN32(sb.s_wtime);
	HOST_TO_LENDIAN16(sb.s_mnt_count);
	HOST_TO_LENDIAN16(sb.s_max_mnt_count);
	HOST_TO_LENDIAN16(sb.s_magic);
	HOST_TO_LENDIAN16(sb.s_state);
	HOST_TO_LENDIAN16(sb.s_errors);
	HOST_TO_LENDIAN16(sb.s_minor_rev_level);
	HOST_TO_LENDIAN32(sb.s_lastcheck);
	HOST_TO_LENDIAN32(sb.s_checkinterval);
	HOST_TO_LENDIAN32(sb.s_creator_os);
	HOST_TO_LENDIAN32(sb.s_rev_level);
	HOST_TO_LENDIAN16(sb.s_def_resuid);
	HOST_TO_LENDIAN16(sb.s_def_resgid);

	HOST_TO_LENDIAN32(sb.s_first_ino);
	HOST_TO_LENDIAN16(sb.s_inode_size);
	HOST_TO_LENDIAN16(sb.s_block_group_nr);
	HOST_TO_LENDIAN32(sb.s_feature_compat);
	HOST_TO_LENDIAN32(sb.s_feature_incompat);
	HOST_TO_LENDIAN32(sb.s_feature_ro_compat);
	HOST_TO_LENDIAN32(sb.s_algorithm_usage_bitmap);	

#else
	sb_ptr = &ext2->sb;
#endif
	
	// Get a pointer to the block and copy the data
	temp = (char *)ext2_get_cache_block(ext2, block);
	memcpy(temp + offset, sb_ptr, sizeof(ext2_super_block));
	ext2_mark_blocks_dirty(ext2, block, 1);
	ext2_release_cache_block(ext2, block);		
	
	return B_NO_ERROR;
}


// Mark the cache blocks as dirty. 
status_t ext2_mark_blocks_dirty(ext2_fs_struct *ext2, uint32 cache_block, int num_blocks)
{
	if(ext2->read_only) {
		ERRPRINT(("ext2_mark_blocks_dirty called in ReadOnly mode.\n"));
		return B_ERROR;
	}
	
	return mark_blocks_dirty(ext2->fd, cache_block, num_blocks);
}

// Marks the specified block of a file dirty.
status_t ext2_mark_file_block_dirty(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block)
{
	uint32 dev_block;
	status_t err;
	
	if(ext2->read_only) {
		ERRPRINT(("ext2_mark_file_block_dirty called in ReadOnly mode.\n"));
		return B_ERROR;
	}
	
	err = ext2_get_block_num_from_extent(ext2, file_vnode, &dev_block, file_block);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_mark_file_block_dirty had a problem getting correct block from block list.\n"));
		return B_ERROR;
	}
	if(dev_block > ext2->num_cache_blocks) {
		ERRPRINT(("ext2_mark_file_block_dirty invalid cache_block %ld calculated from block %ld of vnode %Ld.\n",
			dev_block, file_block, file_vnode->vnid));
		return B_ERROR;
	}
	return mark_blocks_dirty(ext2->fd, dev_block, 1);	
}

#endif // !RO


// Junk


/*
// This function and the next read in the file block table for any inode. It gets all of the direct blocks and
//  recurses to get the indirect blocks. It should at a max recurse 3 times, so it's not so bad.
// It should be redone. Now that I've implemented file extents, it seems that this can be done much simpler.
//int LoadFileBlockTable(ext2_fs_struct *ext2, vnode *file_vnode)
//{
	int blocks_to_read, blocks_read;
	int curr_block_pos=0;

	DEBUGPRINT1("LoadFileBlockTable loading block table for vnode %Ld.\n", file_vnode->vnid);

	// Check to see if it's already loaded for this vnode
	if(file_vnode->num_file_blocks > 0) {
		DEBUGPRINT1("LoadFileBlockTable block table already loaded with %d blocks.\n", file_vnode->num_file_blocks);
		return file_vnode->num_file_blocks;
	}

	blocks_to_read = file_vnode->inode.i_size / ext2->info.s_block_size;
	if(file_vnode->inode.i_size % ext2->info.s_block_size > 0) {
		blocks_to_read++;
	}

	DEBUGPRINT1("LoadFileBlockTable block table size = %d.\n",blocks_to_read);


	// Read all of the direct blocks
	while(blocks_to_read>0) {
		AddBlockToExtent(file_vnode, file_vnode->inode.i_block[curr_block_pos]);
		DEBUGPRINT1("LoadFileBlockTable pointer to block %d loaded.\n", file_vnode->inode.i_block[curr_block_pos]);
		file_vnode->last_block_loaded[0] = curr_block_pos;
		file_vnode->last_written_block++;
		curr_block_pos++;
		blocks_to_read--;
		if(curr_block_pos == 12) break;
	}
	
	// Check to see if we're done.
	if(blocks_to_read<=0) {
		return curr_block_pos--;
	}

	// Well, we need to handle all of the indirect blocks
	{
		char *indirect_block;
		int indirection_level;

		for(indirection_level=1; indirection_level<=3; indirection_level++) {
			DEBUGPRINT2("LoadFileBlockTable calling LoadFileBlockTableIndirect @ block %d, indirection level %d.\n", file_vnode->inode.i_block[11+indirection_level], indirection_level);
			file_vnode->last_block_level = indirection_level;
			indirect_block = (char *)ext2_get_cache_block(ext2, file_vnode->inode.i_block[11+indirection_level]);
			blocks_read = LoadFileBlockTableIndirect(ext2, file_vnode,
				indirect_block, &blocks_to_read, indirection_level, file_vnode->last_block_loaded+1);
			DEBUGPRINT1("LoadFileBlockTableIndirect returned after reading %d blocks.\n", blocks_read);
			curr_block_pos+= blocks_read;
			ext2_release_cache_block(ext2, file_vnode->inode.i_block[11+indirection_level]);
			if((blocks_read == 0)||(blocks_to_read<=0)) {
				return curr_block_pos--;
			}
		}
		// If you've gotten to this point, the file is so big that it has used up all of the possible block pointers. Shoot yourself.
	}
				
	return curr_block_pos--;
//	return 0;
//}
*/

/*
int LoadFileBlockTableIndirect(ext2_fs_struct *ext2, vnode *file_vnode, char *block, int *blocks_remaining, int levels, int *last_block_record)
{
	unsigned int i;
	int *intblock = (int *)block;
	int blocks_read = 0;

	DEBUGPRINT("Cext2::LoadFileBlockTableIndirect called.\n");

	if(levels == 1) {
		// This is a single indirection block
		for(i=0; i<(ext2->info.s_block_size/sizeof(int)); i++) {
//			DEBUGPRINT1("Cext2::LoadFileBlockTableIndirect pointer to block %d loaded.\n", B_LENDIAN_TO_HOST_INT32(intblock[i]));
			AddBlockToExtent(file_vnode, B_LENDIAN_TO_HOST_INT32(intblock[i]));
			file_vnode->last_written_block++;
			last_block_record[0] = i;
			blocks_read++;
			(*blocks_remaining)--;
			if(*blocks_remaining==0) return blocks_read;
		}
	} else {
		// This is at least a double indirection block
		char *indirect_block;
		int	curr_block_loaded=0;
		for(i=0; i<(ext2->info.s_block_size/sizeof(int)); i++) {
			curr_block_loaded = B_LENDIAN_TO_HOST_INT32(intblock[i]);
			indirect_block = (char *)ext2_get_cache_block(ext2, curr_block_loaded);
			blocks_read += LoadFileBlockTableIndirect(ext2, file_vnode, 
				indirect_block, blocks_remaining, levels-1, last_block_record+1);
			ext2_release_cache_block(ext2, curr_block_loaded);
			if(*blocks_remaining == 0) {
				return blocks_read;
			}
		}
		ext2_release_cache_block(ext2,curr_block_loaded);
	}

	DEBUGPRINT1("Cext2::LoadFileBlockTableIndirect read %d blocks.\n", blocks_read);

	return blocks_read;
}
*/
