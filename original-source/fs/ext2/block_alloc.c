#ifndef RO

// This module deals with allocating and marking blocks for files. Some of the messiest code I've ever written is in here,
// but it seems to work. I still can't figure out how I came up with some of this stuff. :)

#include "ext2.h"
#include "io.h"
#include "file_extent.h"
#include "block_alloc.h"
#include "prealloc.h"
//#include <string.h>

inline uint8 check_bit(uint8 a, uint8 b)
{
	uint8 mask = 0x1 << b;
	
	return a&mask;
}

inline uint8 set_bit(uint8 a, uint8 b)
{
	uint8 mask = 0x1 << b;
	
	return a|mask;
}

inline uint8 clear_bit(uint8 a, uint b)
{
	uint8 mask = ~(0x1 << b);
	
	return a&mask;
}
	 

// The point of this function is to find n contiguous blocks in the free block table. Start in the group specified.
// This has got to be the most complicated code I've written. Surprised it works.
int ext2_find_n_free_blocks(ext2_fs_struct *ext2, uint32 *blocks, uint32 start_block, int n)
{
	char *buf;
	uint32 i;
	unsigned int group_offset;
	int found_blocks = 0;
	bool found;
	unsigned int current_group = --start_block / ext2->info.s_blocks_per_group;
	int curr_cache_block_loaded;
	// The next number is the number of blocks that can be checked in one cache block of the block table
	int blocks_per_block_table_block = ext2->info.s_block_size * 8;
	
	DEBUGPRINT(ALLOC, 5, ("ext2_find_n_free_blocks called. Starting at block %d, ", start_block+1));
	DEBUGPRINT(ALLOC, 5, ("trying to find %d free blocks.\n", n));

	acquire_sem(ext2->block_alloc_sem);
	
	// Phase 1: Start at the block immediately after the current one and record free blocks until we either have found n free blocks or
	//  we find a non-free one. Go on to phase 2 if we still have blocks to find. 
	{
		DEBUGPRINT(ALLOC, 6, ("ext2_find_n_free_blocks starting phase 1...\n"));
		
		group_offset = start_block % ext2->info.s_blocks_per_group; // This is the block into this group to start at
		curr_cache_block_loaded = ext2->GroupDesc[current_group].bg_block_bitmap + (group_offset / blocks_per_block_table_block);
	    DEBUGPRINT(ALLOC, 7, ("ext2_find_n_free_blocks loading cache block %d.\n", curr_cache_block_loaded));
		buf = (char *)ext2_get_cache_block(ext2, curr_cache_block_loaded);
		for(i=start_block; i<start_block+n; i++) {
			int byte_in_current_block = (group_offset % blocks_per_block_table_block) / 8;			
			uint8 bit_in_current_byte = (group_offset % blocks_per_block_table_block) % 8;
			
			DEBUGPRINT(ALLOC, 7, ("ext2_find_n_free_blocks checking block %d, found in group %d,", i+1, current_group));
			DEBUGPRINT(ALLOC, 7, (" byte %d, bit %d. ", byte_in_current_block, bit_in_current_byte));
			DEBUGPRINT(ALLOC, 7, ("\next2_find_n_free_blocks byte to check is 0x%x. ", buf[byte_in_current_block]));
			found = false;
			// Check the bit
			if(!check_bit(buf[byte_in_current_block], bit_in_current_byte)) {
				// Check to see if it was already in someone's preallocation list
				if(!FindPreAlloc(ext2, i+1)) {
					found = true;
				}
			}
			if(found) {
				// We have found a match, add it to the return blocks
				DEBUGPRINT(ALLOC, 7, ("free.\n"));
				blocks[found_blocks++] = i+1;
			} else {
				// Bail out. Go on to phase 2.
				DEBUGPRINT(ALLOC, 7, ("not free.\n"));
				break;
			}				
			// Check to see if we need to load in a new block table buffer to check the next block
			group_offset++;
			if(group_offset > ext2->info.s_blocks_per_group) {
				// This group is over, bail out and go to phase 2
				if(++current_group > ext2->info.s_groups_count) current_group = 0;
				group_offset = 0;
				break;
			}
			if((group_offset % blocks_per_block_table_block) == 0) {
				// We must be looking at the first block in a new cache block, need to release the current cache block loaded and read a new one
				ext2_release_cache_block(ext2, curr_cache_block_loaded);
				// We've established that we're still in the same group, so we can just add one to the last block loaded and get a new one
				buf = (char *)ext2_get_cache_block(ext2, ++curr_cache_block_loaded);
			}
		}
		// Ok, we've made it out of the for loop, might be because we bailed, though.
		ext2_release_cache_block(ext2, curr_cache_block_loaded); // Release the cache already loaded
		// See if we got through the for loop in it's entirety.
		if(found_blocks >= n) {
			// We're done, get out
			release_sem(ext2->block_alloc_sem);
			return found_blocks;
		}
	}	
	
	DEBUGPRINT(ALLOC, 6, ("ext2_find_n_free_blocks leaving phase 1, entering phase 2.\n"));
	// Phase 2: From where we last left off, try to find n-found_blocks consequtive blocks. I combined phase 3 into this one.
	// Basically, do phase 2, but as we search, insert phase 3 blocks into the block array. Simply means, as I find blocks, fill up
	// the array. After it's full, and it cant find any phase 2 blocks, it should be full of something. If phase 2 goes through, it'll
	// erase all of the blocks inserted in phase 3.
	{
		unsigned int blocks_checked = 0;
		int x = 0;
		bool load_new_cache_block;
		uint32 start_run=0;
		int phase3_pointer = found_blocks;
		uint32 current_block;
		
		
		int byte_in_current_block = (group_offset % blocks_per_block_table_block) / 8;
		uint8 bit_in_current_byte = (group_offset % blocks_per_block_table_block) % 8;
		
		curr_cache_block_loaded = ext2->GroupDesc[current_group].bg_block_bitmap + (group_offset / blocks_per_block_table_block);
		buf = (char *)ext2_get_cache_block(ext2, curr_cache_block_loaded);
		while(blocks_checked < ext2->sb.s_blocks_count) {		
			load_new_cache_block = false;
			current_block = ((current_group * ext2->info.s_blocks_per_group) + group_offset);
			DEBUGPRINT(ALLOC, 7, ("ext2_find_n_free_blocks checking block %d, found in group %d ", current_block + 1 , current_group));
			DEBUGPRINT(ALLOC, 7, ("%d %d.\n", byte_in_current_block, bit_in_current_byte));
			found = false;
			if(!check_bit(buf[byte_in_current_block], bit_in_current_byte)) {
				found = true;
			} else {
				if(!FindPreAlloc(ext2, current_block+1)) {
					found = true;
				}
			}
			
			if(!check_bit(buf[byte_in_current_block], bit_in_current_byte)) {
				// Implement phase 3 here
				if(phase3_pointer < n) {
					blocks[phase3_pointer] = ((current_group * ext2->info.s_blocks_per_group) + group_offset) + 1;
					phase3_pointer++;
				}
				if(x == 0) {
					start_run = (current_group * ext2->info.s_blocks_per_group) + group_offset;
				}
				x++; // Add one to the found pointer
				DEBUGPRINT(ALLOC, 7, ("ext2_find_n_free_blocks found free block, now block %d in a run.\n", x));
			} else {
				DEBUGPRINT(ALLOC, 7, ("ext2_find_n_free_blocks block not free.\n"));
				x = 0;
			}
				
			// Check for various things
			blocks_checked++;
			// Move the pointers ahead one
			if(++bit_in_current_byte > 7) {
				bit_in_current_byte = 0;
				byte_in_current_block++;
			}
			// Check to see if we're done
			if(x >= (n-found_blocks)) {
				// We've found a contiguous set of blocks, we now know the starting point of the run and we're pointing at the ending point
				if(start_run > current_block) {
					// Weird stuff here. Shouldn't be able to happen unless the block tables are corrupt.
				} else {
					for(i=start_run; i<= current_block; i++) {
						blocks[found_blocks] = i + 1;
						found_blocks++;
					}
					break;
				}
			}	
			// Move the group offset pointer ahead one
			group_offset++;		
			if(group_offset > ext2->info.s_blocks_per_group) {
				// We're done with this group, move to the next.
				if(++current_group > ext2->info.s_groups_count) current_group = 0;
				group_offset = 0;
				load_new_cache_block = true;
			} else {
			 	if((group_offset % blocks_per_block_table_block) == 0) {
					// We're now pointing at the first byte in a new block of the same group.
					load_new_cache_block = true;
				}
			}
			if(load_new_cache_block) {
				// We must be resetting to a new block
				byte_in_current_block = 0;
				bit_in_current_byte = 0;
				// Release the old one
				ext2_release_cache_block(ext2, curr_cache_block_loaded);
				// Get a new one
				curr_cache_block_loaded = ext2->GroupDesc[current_group].bg_block_bitmap + (group_offset / blocks_per_block_table_block);		
				buf = (char *)ext2_get_cache_block(ext2, curr_cache_block_loaded);		
			}	
		}	
		ext2_release_cache_block(ext2, curr_cache_block_loaded);
		if(found_blocks >= n) {
			// We're done
			release_sem(ext2->block_alloc_sem);
			return found_blocks;
		}
		// Phase 2 must have gone bad, make sure we still came up with enough blocks.
		if(phase3_pointer >= n) {
			// We're alright
			release_sem(ext2->block_alloc_sem);
			return phase3_pointer;
		}
		DEBUGPRINT(ALLOC, 6, ("ext2_find_n_free_blocks done with pass 2 & 3. Out of blocks.\n"));
	
		release_sem(ext2->block_alloc_sem);
		return phase3_pointer;
	}

}

// Marks the block in the bitmap and updates the sb and Group Descriptors to what the it should be.
int ext2_mark_blocks(ext2_fs_struct *ext2, uint32 *blocks, int n, bool mark)
{
	int i;
	int group;
	int byte_in_group;
	uint32 block;
	uint8 bit_in_byte;
	char *buf = NULL;
	uint32 last_cache_loaded = 0;

	DEBUGPRINT(ALLOC, 5, ("ext2_mark_blocks called, "));

	acquire_sem(ext2->block_marker_sem);

	for(i=0; i<n; i++) {
		group = (blocks[i]-1)/ext2->info.s_blocks_per_group;
		byte_in_group = ((blocks[i]-1) % ext2->info.s_blocks_per_group) / 8;
		//if((((blocks[i]-1) % ext2->info.s_blocks_per_group) % 8) > 0) byte_in_group++;
		block = ext2->GroupDesc[group].bg_block_bitmap + (byte_in_group / ext2->info.s_block_size);
		bit_in_byte = (blocks[i]-1)  % 8;
		// Now, lets load in the cache block if it isn't already
		if(block != last_cache_loaded) {
			// Release the old cache block loaded
			if(last_cache_loaded != 0) {
				ext2_mark_blocks_dirty(ext2, last_cache_loaded, 1);
				ext2_release_cache_block(ext2, last_cache_loaded);
			}
			// Get the new one
			buf = (char *)ext2_get_cache_block(ext2, block);
			last_cache_loaded = block;
		}
		DEBUGPRINT(ALLOC, 6, ("block %d can be found in group/byte/bit %d", blocks[i], group));
		DEBUGPRINT(ALLOC, 6, ("/%d/%d.\n", byte_in_group, bit_in_byte));

		// Set the bit to whatever mark says
		if(mark) {
			DEBUGPRINT(ALLOC, 7, ("setting byte 0x%x to 0x%x.\n", buf[byte_in_group % (ext2->info.s_block_size / sizeof(int))])),
		//		set_bit(buf[byte_in_group % (ext2->info.s_block_size / sizeof(int))], bit_in_byte));
			buf[byte_in_group % ext2->info.s_block_size] = set_bit(buf[byte_in_group % ext2->info.s_block_size], bit_in_byte);
			// Subtract one block from the superblock
			ext2->sb.s_free_blocks_count--;					
			// Subtract one from the proper group descriptor
			ext2->GroupDesc[(blocks[i]-1) / ext2->info.s_blocks_per_group].bg_free_blocks_count--;
			ext2->GroupDescToFlush[(blocks[i]-1) / ext2->info.s_blocks_per_group] = true;
			DEBUGPRINT(ALLOC, 7, ("ext2_mark_blocks just marked group descriptor %d dirty.\n", (blocks[i]-1) / ext2->info.s_blocks_per_group)); 
		} else {
			buf[byte_in_group % ext2->info.s_block_size] = clear_bit(buf[byte_in_group % ext2->info.s_block_size], bit_in_byte);
			// Add one block to the superblock
			ext2->sb.s_free_blocks_count++;					
			// Add one to the proper group descriptor
			ext2->GroupDesc[(blocks[i]-1) / ext2->info.s_blocks_per_group].bg_free_blocks_count++;
			ext2->GroupDescToFlush[(blocks[i]-1) / ext2->info.s_blocks_per_group] = true;
			DEBUGPRINT(ALLOC, 7, ("ext2_mark_blocks just marked group descriptor %d dirty.\n", (blocks[i]-1) / ext2->info.s_blocks_per_group)); 

		}
	}
	// release the cache block
	if(last_cache_loaded != 0) {
		ext2_mark_blocks_dirty(ext2, last_cache_loaded, 1);
		ext2_release_cache_block(ext2, last_cache_loaded);
	}
	
	release_sem(ext2->block_marker_sem);

	return n;	
}

// Allocates n number of blocks for a file and puts it in the file extents. Uses the preallocation buffer to get the stuff done.
int ext2_allocate_blocks_for_file(ext2_fs_struct *ext2, vnode *file_vnode, int num_blocks)
{
	int i;
	DEBUGPRINT(ALLOC, 5, ("ext2: ext2_allocate_blocks_for_file called on vnode %Ld to allocate %d blocks.\n", file_vnode->vnid, num_blocks));

	for(i=0; i<num_blocks; i++) {
		uint32 new_block;
		new_block = GetBlockFromFilePreallocList(ext2, file_vnode);
		if(new_block == 0) {
			ERRPRINT(("Ext2: ext2_allocate_blocks_for_file could not allocate new block for file.\n"));
			return i;
		}
		DEBUGPRINT(ALLOC, 5, ("Ext2: AllocateBlockForFile adding block %ld from preallocation buffer.\n", new_block));
		if(ext2_add_block_to_extent(ext2, file_vnode, new_block) == B_ERROR) {
			ERRPRINT(("Ext2: ext2_add_block_to_extent failed, probably out of space to allocate to block pointer tables.\n"));
			return i;
		}
		ext2_mark_blocks_in_use(ext2, &new_block, 1);
		RemoveBlockFromPreAllocList(ext2, new_block);
		// Add a block to the inode
		file_vnode->inode.i_blocks += ext2->info.s_block_size / 512;						
	}	
	// We should be done allocating blocks
	return num_blocks;	
}

// Duh.
int ext2_mark_blocks_free(ext2_fs_struct *ext2, uint32 *blocks, int n)
{
	return ext2_mark_blocks(ext2, blocks, n, false);
}

// Again, duh.
int ext2_mark_blocks_in_use(ext2_fs_struct *ext2, uint32 *blocks, int n)
{
	return ext2_mark_blocks(ext2, blocks, n, true);
}

// This simply erases the specified block.
int ext2_clear_block(ext2_fs_struct *ext2, uint32 block)
{
	int *buf;
	
	buf = (int *)ext2_get_cache_block(ext2, block);
	memset(buf, 0, ext2->info.s_block_size);
	ext2_mark_blocks_dirty(ext2, block, 1);
	ext2_release_cache_block(ext2, block);	

	return B_NO_ERROR;
}

#endif
