// These functions deal with the manipulation of a list of file extents stored on each vnode.r.

#include "ntfs.h"
#include "extent.h"

// This function sets the compression flag on file extents in the compression group. This is so that if it 
// turns out that a compression group is not compressed, the flag can be set because at that point, the
// runlist decoder may be a few blocks down the road.
static void ntfs_set_compressed_flag_on_compress_group(extent_storage *extent, uint64 compress_group, bool flag)
{
	file_extent *curr;

	curr = extent->extent_head;
	while(curr) {
		if(curr->compression_group_start == compress_group) curr->flags |= EXTENT_FLAG_COMPRESSED;
		if(curr->compression_group_end == compress_group) curr->flags |= EXTENT_FLAG_LAST_GROUP_COMPRESSED;
		curr = curr->next;
	}
}

// Get the last block added.
static uint64 ntfs_get_last_block_from_extent(extent_storage *extent)
{
	file_extent *curr = extent->extent_tail;
	
	if(curr) {
		return curr->end;
	} else {
		return 0;
	}
}

static uint64 ntfs_get_last_extent_start(extent_storage *extent)
{
	file_extent *curr = extent->extent_tail;
		
	if(curr) {
		return curr->start;
	} else {
		return 0;
	}
}

static file_extent *ntfs_get_last_extent_struct(extent_storage *extent)
{
	return extent->extent_tail;
}

// Convert a NTFS runlist directly to the extent list
status_t ntfs_load_extent_from_runlist(extent_storage *extent, void *runlist, int32 length_to_read, uint64 blocks_to_load, bool possibly_compressed)
{
	char *curr_ptr = runlist;
	uint64 block, length;
	uint64 compression_group = 0;
	int32 blocks_loaded_in_group = 0;
	int64 offset;
	uint8 sizes;
	int i;
	bool loaded_a_block = false;
	
	DEBUGPRINT(RUNLIST, 5, ("ntfs_load_extent_from_runlist entry. length_to_read = %d\n", (int)length_to_read));
	
	if(possibly_compressed) {
		DEBUGPRINT(RUNLIST, 6, ("\tthis extent is compressed, or so it says....\n"));
		// Start the compression group at the correct spot. If this is not the first runlist for this extent, we need to 
		// start at an appropriate spot.
		{
			file_extent *temp = ntfs_get_last_extent_struct(extent);
			extent->compressed = true;
			if(temp) {
				compression_group = temp->compression_group_end + 1;
			} else {
				compression_group = 0;
			}
		}
	} else {
		extent->compressed = false;
	}
	
	while((length_to_read > 0) && (blocks_to_load)) {
		block = length = offset = 0;				
		sizes = *(uint8 *)curr_ptr;
		if(sizes == 0) {
			// both the offset and length are 0 length. Just quit
			ERRPRINT(("runlist decoder: both offset length and length length are zero. hmmm.\n"));
			return B_NO_ERROR;
		} else {
			curr_ptr++;
			length_to_read--;
			// Handle the length
			for(i=0; i<low_4bits(sizes); i++) {
				length |= (*(uint8 *)curr_ptr << 8*i);
				curr_ptr++;				
				length_to_read--;
			}
			// handle the new starting block
			for(i=0; i<high_4bits(sizes); i++) {
				offset |= (*(uint8 *)curr_ptr << 8*i);				
				curr_ptr++;
				length_to_read--;
			}
			// Handle negative offsets
			if(high_4bits(sizes)!=0) {
				// Look at the highest order bit just loaded into the offset.
				if(offset & (0x80 << 8*(high_4bits(sizes)-1))) {
					// Needs to be sign extended
					offset |= 0xFFFFFFFFFFFFFFFFLL << 8*high_4bits(sizes);
				}
			}
			
			if(length > blocks_to_load) blocks_to_load = 0;
			else blocks_to_load -= length;
			
			if(length) {
				// Lets add it to the list, it should be a new extent in the list
				file_extent *curr;
				
				DEBUGPRINT(RUNLIST, 7, ("\tgoing to do something with offset %Ld (0x%8x)", offset, (unsigned int)offset));
				DEBUGPRINT(RUNLIST, 7, (", length %Ld.\n", length));
												
				// Calculate starting block				
				if(loaded_a_block)
					block = ntfs_get_last_extent_start(extent) + offset;
				else
					block = offset;
				
				if(possibly_compressed) {
					// If the offset is zero, it's either a) starting a sparse run, or b) finishing off
					// the previous compression group. We deal with b here.
					if((offset == 0) && (blocks_loaded_in_group > 0)) {
						DEBUGPRINT(RUNLIST, 7, ("\tzero offset, finishing compress group %Ld.\n", compression_group));
						ntfs_set_compressed_flag_on_compress_group(extent, compression_group, true);
						compression_group++;		// New group
						// See if it sticks past the compression group we're finishing here
						if(length <= 16 - blocks_loaded_in_group) {
							// It doesnt, dont create an extent, just loop to the next run
							blocks_loaded_in_group = 0;		// Reset group block pointer
							continue;						// get the next runlist
						} else {
							// It does, so let the remainder leak over
							length -= 16 - blocks_loaded_in_group;
							blocks_loaded_in_group = 0;
						}	
					}
				}									

				loaded_a_block = true;
				curr = (file_extent *)ntfs_malloc(sizeof(file_extent));
				if(!curr) return B_ERROR;

				curr->next = NULL;
				if(!possibly_compressed || offset) {
					curr->flags = 0; // default the compressed flag to false							
				} else {
					curr->flags = EXTENT_FLAG_SPARSE;
				}
				curr->start =  block;
				curr->end = block + length - 1;
				curr->length = length;	
	
				if(extent->num_file_blocks == 0) {
					// Add it to the head of the list
					extent->extent_head = extent->extent_tail = curr;
				} else {
					// Add it to the tail of the list
					extent->extent_tail->next = curr;
					extent->extent_tail = curr;
				}
				
				extent->num_file_blocks += curr->length;

				if(curr->flags & EXTENT_FLAG_SPARSE) {
					DEBUGPRINT(RUNLIST, 7, ("\tnew extent, SPARSE, length %Ld\n", curr->length));			
				} else {
					DEBUGPRINT(RUNLIST, 7, ("\tnew extent, start %Ld, end %Ld, length %Ld\n", curr->start, curr->end, curr->length));
				}				
				
				// This gets nasty
				if(possibly_compressed) {
					int64 blocks_extending_past_this_compression_group = ((int64)curr->length - (16 - blocks_loaded_in_group));			
		
					curr->compression_group_start = compression_group;
					curr->start_block_into_compression_group = blocks_loaded_in_group;

					DEBUGPRINT(RUNLIST, 7, ("\tthis run extends %Ld blocks past this compression group\n", blocks_extending_past_this_compression_group));

					// Calculate where the compression group will end, if this run extends past this one
					if(blocks_extending_past_this_compression_group <= 0) {
						curr->compression_group_end = compression_group;
						blocks_loaded_in_group += curr->length;
						curr->end_block_into_compression_group = blocks_loaded_in_group - 1;	
						// If this filled this compression group, move to the next one
						if(blocks_loaded_in_group == 16) {
							blocks_loaded_in_group = 0;
							compression_group++;
						}
					} else {
						curr->compression_group_end = curr->compression_group_start + blocks_extending_past_this_compression_group / 16;
						if(blocks_extending_past_this_compression_group % 16) curr->compression_group_end++;
						blocks_loaded_in_group = blocks_extending_past_this_compression_group % 16;
						compression_group = curr->compression_group_end;
						if(blocks_loaded_in_group == 0) {
							curr->end_block_into_compression_group = 15;
							compression_group++;
						} else {
							curr->end_block_into_compression_group = blocks_loaded_in_group - 1;
						}
					}
					
					DEBUGPRINT(RUNLIST, 7, ("\tstart cgroup = %Ld, end cgroup = %Ld, start block into cgroup = %d, end block into cgroup = %d\n",
						curr->compression_group_start, curr->compression_group_end,
						curr->start_block_into_compression_group, curr->end_block_into_compression_group));
				}
			}
			if(length_to_read <=0) 
				DEBUGPRINT(RUNLIST, 6, ("\tabout to exit because length_to_read = %d.\n", (unsigned int)length_to_read));			
		}
	}
		
	DEBUGPRINT(RUNLIST, 5, ("ntfs_load_extent_from_runlist exit.\n"));		
	return B_NO_ERROR;
}

// Grab the nth block in the list.
status_t ntfs_get_block_from_extent(extent_storage *extent, uint64 *block, uint64 file_block)
{
	file_extent *curr;
	int blocks_found = 0;

	DEBUGPRINT(EXTENT, 8, ("ntfs_get_block_from_extent called on block %Ld.\n", file_block));

	if(file_block >= extent->num_file_blocks) {
		ERRPRINT(("ntfs_get_block_from_extent asked to get file block %Ld, but there's only %Ld in extent.\n", file_block, extent->num_file_blocks));
		*block = 0;
		return B_ERROR;
	}
	if(extent->compressed) return EINVAL;

	// Start walking through the list
	curr = extent->extent_head;
	while(curr) {
		if((curr->length + blocks_found) > file_block) {
			// It's in this extent
			*block = (curr->start+(file_block - blocks_found));
			return B_NO_ERROR;
		}
		blocks_found += curr->length;
		curr = curr->next;			
	}

	ERRPRINT(("ntfs_get_block_from_extent ran off the end of the list\n"));
	
	// Hmm, if we're here we must be having some problems. The only way to get here is to ask for
	// a block past the end of the extent.
	*block = 0;
	return B_ERROR;
}

status_t ntfs_count_runs_in_extent(extent_storage *extent, uint64 *count)
{
	file_extent *curr;
	uint64 last_end = 0;
	
	*count = 0;

	// Start walking through the list
	curr = extent->extent_head;
	while(curr) {
		if(*count > 0) {		
			if(curr->start != (last_end + 1)) (*count)++;
		} else {
			last_end = curr->end;
			(*count)++;
		}
		curr = curr->next;			
	}

	return B_NO_ERROR;
}


// Grab the nth block in the list. This will return an array of raw blocks making up the 16 block compressed series of blocks.
status_t ntfs_get_compression_group_extent(extent_storage *extent, uint64 target_compression_group, comp_file_extent *comp_extent)
{
	file_extent *curr;
	uint8 temp_ptr = 0;
	uint32 i;

	DEBUGPRINT(EXTENT, 8, ("ntfs_get_compression_group_extent entry, cgroup %Ld.\n", target_compression_group));

	if(extent->num_file_blocks == 0) return EINVAL;
	if(!extent->compressed) return EINVAL;

	comp_extent->num_blocks = 0;

	// Start walking through the list
	curr = extent->extent_head;
	while(curr) {
		// Get blocks from an extent that starts in the target compression group
		if(curr->compression_group_start == target_compression_group) {
			comp_extent->flags = curr->flags;
			if(!(comp_extent->flags & EXTENT_FLAG_SPARSE)) {
				// If we're not sparse, put some blocks in the returning extent
				for(i=0; i<curr->length; i++) {
					if(temp_ptr >= 16) {
						break;
					}
					comp_extent->blocks[temp_ptr] = curr->start + i;
					temp_ptr++;
				}
			} else {
				// we're sparse, so just memset this to zero.
				memset(&comp_extent->blocks[0], 0, 16 * sizeof(comp_extent->blocks[0]));
				temp_ptr = 16;
			}
		}

		// Get blocks from an extent that spans the target compression group		
		if((target_compression_group > curr->compression_group_start) && (target_compression_group <= curr->compression_group_end)) {
			uint64 delta = (16 - curr->start_block_into_compression_group) +16 * (target_compression_group - curr->compression_group_start - 1);

			// set the flags
			comp_extent->flags = curr->flags;

			// Depending on if we're looking at the last compression group this extent spans,
			// we are concerned with different flags. 			
			if(target_compression_group == curr->compression_group_end) {
				// If we're looking at the last compression group this extent spans,
				// We want to mask out the EXTENT_FLAG_COMPRESSED flag, and convert an EXTENT_FLAG_LAST_GROUP_COMPRESSED
				// flag into an EXTENT_FLAG_COMPRESSED.
				if(comp_extent->flags & EXTENT_FLAG_LAST_GROUP_COMPRESSED) {
					comp_extent->flags |= EXTENT_FLAG_COMPRESSED;
				}
			}

			// Note: The EXTENT_FLAG_SPARSE should fall through all of this. 

			if(!(comp_extent->flags & EXTENT_FLAG_SPARSE)) {
				// If we're not sparse, put some blocks in the returning extent
				for(i=0; i<min(16, curr->length - delta); i++) {
					if(temp_ptr >= 16) {
						break;
					}
					comp_extent->blocks[temp_ptr] = curr->start + delta + i;
					temp_ptr++;						
				}
			} else {
				// we're sparse, so just memset this to zero.
				memset(&comp_extent->blocks[0], 0, 16 * sizeof(comp_extent->blocks[0]));
				temp_ptr = 16;
			}
		}

		curr = curr->next;
	}
	comp_extent->num_blocks = temp_ptr;
	comp_extent->compression_group = target_compression_group;

	if(temp_ptr == 0) {
		ERRPRINT(("ntfs_get_compression_group_extent didn't find a runlist for cgroup %Ld.\n", target_compression_group));
		return B_ERROR;
	}	

/*
	// XXX - remove me
	{
		int i;
	
		dprintf("compressed extent dump for vnid %Ld:\n", extent->vnid);
		dprintf("compression group = %Ld.\n", comp_extent->compression_group);
		dprintf("flags = 0x%x\n", comp_extent->flags);
		for(i=0; i<comp_extent->num_blocks; i++) {
			dprintf("block %d = %Ld\n", i, comp_extent->blocks[i]);
		}
	}
*/

	return B_NO_ERROR;
}

// Deletes the extent link list.
void ntfs_free_extent(extent_storage *extent)
{
	if(extent) {
		file_extent *temp1, *temp2;
		temp1 = extent->extent_head;
		while(temp1) {
			temp2 = temp1->next;
			ntfs_free(temp1);
			temp1 = temp2;
		}
	}	
}

