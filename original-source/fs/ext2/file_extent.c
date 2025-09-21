#include "ext2.h"
#include "io.h"
#include "file_extent.h"
#include "block_alloc.h"
#include "prealloc.h"

// This function will calculate the place where a block pointer should be located, and the path to get to it.
status_t ext2_calculate_block_pointer_pos(ext2_fs_struct *ext2, uint32 block_to_find, uint32 *level, uint32 pos[])
{
	uint32 block_ptr_per_block, block_ptr_per_2nd_block;
	
	// See if it's in the direct blocks
	if(block_to_find < EXT2_NDIR_BLOCKS) {
		*level = 0;
		pos[0] = block_to_find;
		return B_NO_ERROR;
	}
	
	block_ptr_per_block = ext2->info.s_block_size / sizeof(uint32);
	block_to_find -= EXT2_NDIR_BLOCKS;
	// See if it's in the first indirect block
	if(block_to_find < block_ptr_per_block) {
		*level = 1;
		pos[0] = EXT2_IND_BLOCK;	
		pos[1] = block_to_find;
		return B_NO_ERROR;
	}

	block_to_find -= block_ptr_per_block;
	block_ptr_per_2nd_block = block_ptr_per_block * block_ptr_per_block;
	// See if it's in the second indirect block
	if(block_to_find < (block_ptr_per_2nd_block)) {
		*level = 2;
		pos[0] = EXT2_DIND_BLOCK;
		pos[1] = block_to_find / block_ptr_per_block;
		pos[2] = block_to_find % block_ptr_per_block;		
		return B_NO_ERROR;
	}
	
	block_to_find -= block_ptr_per_2nd_block;
	// See if it's in the third indirect block
	if(block_to_find < (block_ptr_per_2nd_block * block_ptr_per_block)) {
		*level = 3;
		pos[0] = EXT2_TIND_BLOCK;
		pos[1] = block_to_find / block_ptr_per_2nd_block;
		pos[2] = (block_to_find % block_ptr_per_2nd_block) / block_ptr_per_block;
		pos[3] = (block_to_find % block_ptr_per_2nd_block) % block_ptr_per_block;
		return B_NO_ERROR;	
	}
	
	// The block requested must be too big.
	return B_ERROR;
}

// This function returns a pointer to the cache block that corresponds to the indirect block pointer.
status_t ext2_get_indirect_block_pointer_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 **cache_block, uint32 level, uint32 pos[], uint32 *block_loaded)
{
	uint32 current_level = 0;
	uint32 current_block = 0, last_block;
	uint32 *block = NULL;

	DEBUGPRINT(EXTENT, 5, ("ext2_get_indirect_block_pointer_cache_block entry.\n"));

	if((level > 3) || (level == 0)) return EINVAL;

	// Dig down into the indirect blocks. When done, current_block should point to the target.
	while(current_level < level) {
		if(current_level == 0) {
			// read the direct block, simulates a prior loop
			current_block = file_vnode->inode.i_block[pos[0]];
		}
		if((current_block == 0) || (current_block >= ext2->num_cache_blocks)) {
			ERRPRINT(("ext2_get_indirect_block_pointer_cache_block trying to open invalid cache block.\n"));
			*cache_block = NULL;
			return B_ERROR;
		}
		last_block = current_block;
		current_level++;
		*block_loaded = current_block;
		DEBUGPRINT(EXTENT, 7, ("ext2_get_indirect_block_pointer_cache_block opening block %ld.\n", current_block));
		block = (uint32 *)ext2_get_cache_block(ext2, current_block);
		if(!block) {
			ERRPRINT(("ext2_get_indirect_block_pointer_cache_block had error reading from cache.\n"));
			*cache_block = NULL;
			return EIO;
		} 
		if(current_level < level) {
			current_block = B_LENDIAN_TO_HOST_INT32(block[pos[current_level]]);
			ext2_release_cache_block(ext2, last_block);
		}
	}
	DEBUGPRINT(EXTENT, 6, ("ext2_get_indirect_block_pointer_cache_block exiting.\n"));
	*cache_block = block;
	return B_NO_ERROR;
}

// Grab the nth block in the list.
status_t ext2_get_block_num_from_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 *block_num, uint32 file_block)
{
	// TO DO: This function should grab the file blocks from the filesystem
	uint32 block_pos[4];
	uint32 level;
	status_t err;

	err = ext2_calculate_block_pointer_pos(ext2, file_block, &level, block_pos);
	if(err < B_NO_ERROR) {
		ERRPRINT(("ext2_get_block_num_from_extent had error getting block pointer position.\n"));
		return err;
	}

	DEBUGPRINT(EXTENT, 5, ("ext2_get_block_num_from_extent file block %ld is at level %ld position %ld:%ld:%ld:%ld\n",
		file_block, level, block_pos[0], block_pos[1], block_pos[2], block_pos[3]));

	// Handle if it's a direct block
	if(level == 0) {
		DEBUGPRINT(EXTENT, 6, ("ext2_get_block_num_from_extent returning %d.\n", file_vnode->inode.i_block[block_pos[0]]));
		*block_num = file_vnode->inode.i_block[block_pos[0]];
		return B_NO_ERROR;
	}
	
	// Handle the indirect blocks
	{
		uint32 *block;
		uint32 open_block;

		err = ext2_get_indirect_block_pointer_cache_block(ext2, file_vnode, &block, level, block_pos, &open_block);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_get_block_num_from_extent had error reading from cache.\n"));
			return err;
		}
		*block_num = B_LENDIAN_TO_HOST_INT32(block[block_pos[level]]);
		ext2_release_cache_block(ext2, open_block);		

		DEBUGPRINT(EXTENT, 6, ("ext2_get_block_num_from_extent returning address %ld.\n", *block_num));

		return B_NO_ERROR;
	}
}

// Get the last block added.
status_t ext2_get_last_block_num_from_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 *block_num)
{
	status_t err;
	
	if(file_vnode->num_file_blocks == 0) {
		*block_num = 0;
		return B_NO_ERROR;
	} else {
		err = ext2_get_block_num_from_extent(ext2, file_vnode, block_num, file_vnode->num_file_blocks-1);
		return err;
	}	

}

#ifndef RO
// Adds a block to the extent of the file. The code is pretty messy, but I wanted to unroll the loop to 
// keep from having to build a messy loop structure. Not really sure if it's any better this way, but whatever.
int ext2_add_block_to_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 block)
{
	// TO DO: This function should now add a block to the end of the file's block table, allocating a new
	// block for the block list, if necessary.
	uint32 new_block_pos = file_vnode->num_file_blocks;
	uint32 pos[4], level;
	uint32 *block_table;
	uint32 block_loaded;
	status_t err;
	
	// Calculcate where the new block would be located.
	ext2_calculate_block_pointer_pos(ext2, new_block_pos, &level, pos);
	
	DEBUGPRINT(EXTENT, 5, ("ext2_add_block_to_extent new file block %d is at level %d ", block, level));
	DEBUGPRINT(EXTENT, 5, ("position %d:%d:", pos[0], pos[1]));
	DEBUGPRINT(EXTENT, 5, ("%d:%d\n", pos[2], pos[3]));
	
	// See if we're going to have to allocate a block pointer block. This is gross.
	if(level > 0) {
		if(pos[level] == 0) {
			uint32 new_indirect_block;
			// Let's just unroll the loop and handle all three cases seperately. The situations occour
			// when the new block pointer is at the start of an indirect block, and an indirect block needs
			// to allocated itself. When it transitions from 1st->2nd or 2nd->3rd indirect blocks, multiple
			// indirect blocks need to allocated, and this handles it.
			switch(level) {
			case 3:
				// See if we need to allocate a second indirect block
				if(pos[2] == 0) {
					// First see if we need to allocate
					if(pos[1] == 0) {
						// We must need the first space in an indirect block
						new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
						if(new_indirect_block == 0) goto error;
						// Add a block to the inode
						file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
						// Add the block to the inode
						file_vnode->inode.i_block[pos[0]] = new_indirect_block;	
						ext2_clear_block(ext2, new_indirect_block);				
						ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
						RemoveBlockFromPreAllocList(ext2, new_indirect_block);
					}
					// Get the block for the second indirect block
					new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
					if(new_indirect_block == 0) goto error;
					// Add a block to the first indirect block
					err = ext2_get_indirect_block_pointer_cache_block(ext2, file_vnode, &block_table, 1, pos, &block_loaded);
					if(err < B_NO_ERROR) {
						ERRPRINT(("ext2_add_block_to_extent had problem reading cache block.\n"));
						goto error;
					}
					block_table[pos[1]] = new_indirect_block;
					ext2_mark_blocks_dirty(ext2, block_loaded, 1);
					ext2_release_cache_block(ext2, block_loaded);
					// Update other info
					file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
					ext2_clear_block(ext2, new_indirect_block);				
					ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
					RemoveBlockFromPreAllocList(ext2, new_indirect_block);							
				}
				// Get the block for the third indirect block
				new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
				if(new_indirect_block == 0) goto error;
				// Add a block to the second indirect block
				err = ext2_get_indirect_block_pointer_cache_block(ext2, file_vnode, &block_table, 2, pos, &block_loaded);
				if(err < B_NO_ERROR) {
					ERRPRINT(("ext2_add_block_to_extent had problem reading cache block.\n"));
					goto error;
				}
				block_table[pos[2]] = new_indirect_block;
				ext2_mark_blocks_dirty(ext2, block_loaded, 1);
				ext2_release_cache_block(ext2, block_loaded);
				// Update other info
				file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
				ext2_clear_block(ext2, new_indirect_block);				
				ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
				RemoveBlockFromPreAllocList(ext2, new_indirect_block);							
				break;
			case 2:
				// First see if we need to allocate
				if(pos[1] == 0) {
					// We must need the first space in an indirect block
					new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
					if(new_indirect_block == 0) goto error;
					// Add a block to the inode
					file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
					// Add the block to the inode
					file_vnode->inode.i_block[pos[0]] = new_indirect_block;	
					ext2_clear_block(ext2, new_indirect_block);				
					ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
					RemoveBlockFromPreAllocList(ext2, new_indirect_block);
				}
				// Get the block for the second indirect block
				new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
				if(new_indirect_block == 0) goto error;
				// Add a block to the first indirect block
				err = ext2_get_indirect_block_pointer_cache_block(ext2, file_vnode, &block_table, 1, pos, &block_loaded);
				if(err < B_NO_ERROR) {
					ERRPRINT(("ext2_add_block_to_extent had problem reading cache block.\n"));
					goto error;
				}
				block_table[pos[1]] = new_indirect_block;
				ext2_mark_blocks_dirty(ext2, block_loaded, 1);
				ext2_release_cache_block(ext2, block_loaded);
				// Update other info
				file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
				ext2_clear_block(ext2, new_indirect_block);				
				ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
				RemoveBlockFromPreAllocList(ext2, new_indirect_block);						
				break;
			case 1:
				// We must need the first space in an indirect block
				new_indirect_block = GetBlockFromFilePreallocList(ext2, file_vnode);
				if(new_indirect_block == 0) goto error;
				// Add a block to the inode
				file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;
				// Add the block to the inode
				file_vnode->inode.i_block[pos[0]] = new_indirect_block;	
				ext2_clear_block(ext2, new_indirect_block);				
				ext2_mark_blocks_in_use(ext2, &new_indirect_block, 1);
				RemoveBlockFromPreAllocList(ext2, new_indirect_block);
				break;
			}
		}
	}
	
	// Add the block to the block pointer tables or inode
	if(level == 0) {
		// just need to add it to the inode
		file_vnode->inode.i_block[pos[0]] = block;
	} else {		
		err = ext2_get_indirect_block_pointer_cache_block(ext2, file_vnode, &block_table, level, pos, &block_loaded);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ext2_add_block_to_extent had problem reading cache block.\n"));
			goto error;
		}
		block_table[pos[level]] = block;
		ext2_mark_blocks_dirty(ext2, block_loaded, 1);
		ext2_release_cache_block(ext2, block_loaded);
	}			

	// Add one to the block counter
	file_vnode->num_file_blocks++;

	return B_NO_ERROR;
	
	
error:
	// We've got here most likely because there wasn't enough space to allocate a block pointer block
	// there is at least one situation that I don't know how to handle. It would occur if the filesystem ran
	// out of space when the new block transitions into a 2nd or 3rd indirect pointer. At these situations, 2 or 3
	// blocks need to be allocated before another block can be added to the table, and if the filesystem ran out of blocks
	// in the middle of allocating these 2 or 3, there would be a partial block table floating around. Later, if the 
	// fs gets enough free space to finish it off, the driver should be able to handle finishing off the table. 
	// This one should, but I'm not sure about Linux's. :)
	return B_ERROR;	

}
#endif

