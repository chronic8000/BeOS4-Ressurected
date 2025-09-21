#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_free_space.h"
#include <KernelExport.h>
#include <string.h>

#define WATCH_FREE_SPACE 0
#define DEBUG_FREE_SPACE 0

#if DEBUG_FREE_SPACE
#define D(x) dprintf("%s %d: ", __FUNCTION__, __LINE__); dprintf x
#else
#define D(x)
#endif

#if WATCH_FREE_SPACE
static status_t cfs_verify_free_block_list(cfs_info *fsinfo, off_t free_space_offset, off_t size);
static void cfs_change_debug_free_space_counter(off_t delta);
off_t free_space_size = -1;
bool free_space_size_initialized = false;
#else 
#define cfs_verify_free_block_list(a, b, c)
#define cfs_change_debug_free_space_counter(a)
#endif

static status_t
cfs_read_free_block(cfs_info *fsinfo, off_t free_block_offset,
                    cfs_free_block *free_block)
{
	status_t err;
	off_t free_block_size;

	if(free_block_offset < fsinfo->super_block_2_pos || free_block_offset >= fsinfo->size) {
		PRINT_ERROR(("cfs_read_free_block: free block at %Ld outsize "
		             "fs bounds\n", free_block_offset));
		return B_IO_ERROR;
	}
	if(free_block_offset & 7) {
		PRINT_ERROR(("cfs_read_free_block: bad free block offset, %Ld\n",
		             free_block_offset));
		return B_IO_ERROR;
	}

	free_block_size = get_freeblock_size(fsinfo->fs_version);
	if(free_block_offset + 8 == fsinfo->size) {
		free_block_size = 8;
	}
	
	if(fsinfo->fs_version > 1) {
		if(free_block_offset + 16 == fsinfo->size) {
			free_block_size = 16;
		}
	}
	
	err = cfs_read_disk(fsinfo, free_block_offset, free_block,
	                    free_block_size);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_read_free_block: could not read free block at %Ld, "
		             "%s\n", free_block_offset, strerror(err)));
		return err;
	}
	
	switch(free_block->next & 7) {
		case 0:
			if(free_block_size < get_freeblock_size(fsinfo->fs_version)) {
				PRINT_ERROR(("cfs_read_free_block: free block @ %Ld goes past end of volume\n",
					free_block_offset));
				return B_IO_ERROR;
			}				
			if(FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) < get_freeblock_size(fsinfo->fs_version)) {
				PRINT_ERROR(("cfs_read_free_block: free block size %Ld is not "
				             "valid\n", FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)));
				return B_IO_ERROR;
			}
			break;
		case 1:
			FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) = 8;
			break;
		case 2:
			if(fsinfo->fs_version > 1) {
				if(free_block_size < 16) {
					PRINT_ERROR(("cfs_read_free_block: free block @ %Ld goes past end of volume\n",
						free_block_offset));
					return B_IO_ERROR;
				}				
				V2_FREEBLOCK_SIZE_FIELD(free_block) = 16;
				break;
			}
			// if fs_version not > 1, case 2 is invalid, so fall through
		default:
			PRINT_ERROR(("cfs_read_free_block: free block @ %Ld has invalid flag %d in next ptr\n",
				free_block_offset, (int)(free_block->next & 7)));
			return B_IO_ERROR;
	}
	
	// remove the flags from next ptr
	free_block->next &= ~7;
	
	if(FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) & 7) {
		PRINT_ERROR(("cfs_read_free_block: free block @ %Ld has non valid size %Ld\n",
			free_block_offset , FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)));
		return B_IO_ERROR;
	}
														
	if(free_block->next >= fsinfo->size) {
		PRINT_ERROR(("cfs_read_free_block: next free block at %Ld outside "
		             "fs bounds\n", free_block->next));
		return B_IO_ERROR;
	}

	if(free_block->next != 0 &&
		free_block->next <= free_block_offset + FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)) {
		PRINT_ERROR(("cfs_read_free_block: bad next free block offset, "
		             "%Ld <= %Ld+%Ld\n", free_block->next, free_block_offset,
		             FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)));
		return B_IO_ERROR;
	}
	
	if(fsinfo->fs_version > 1) {
		// verify prev pointer
		if(FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) > 8) {
			if(free_block->u.v2.prev != 0) {
				if(free_block->u.v2.prev < fsinfo->super_block_2_pos ||
				  (free_block->u.v2.prev + 16) >= free_block_offset) {
					PRINT_ERROR(("cfs_read_free_block: bad prev free block ptr %Ld in block %Ld\n",
						free_block->u.v2.prev, free_block_offset));
					return B_IO_ERROR;
				}
			}
		}
	}
		
	return B_NO_ERROR;
}

static status_t
cfs_write_free_block(cfs_info *fsinfo, cfs_transaction *transaction,
                     off_t free_block_offset, cfs_free_block *free_block)
{
	status_t err;
	size_t writesize;
	
	if(free_block_offset < 0 || free_block_offset >= fsinfo->size) {
		PRINT_ERROR(("cfs_write_free_block: free block at %Ld outsize fs "
		             "bounds\n", free_block_offset));
		return B_ERROR;
	}
	if(free_block_offset & 7) {
		PRINT_ERROR(("cfs_write_free_block: bad free block offset, %Ld\n",
		             free_block_offset));
		return B_ERROR;
	}
	if(free_block->next < 0 || free_block->next >= fsinfo->size) {
		PRINT_ERROR(("cfs_write_free_block: next free block at %Ld outsize "
		             "fs bounds\n", free_block->next));
		return B_ERROR;
	}
	if(free_block->next & 7) {
		PRINT_ERROR(("cfs_write_free_block: bad next free block offset, %Ld\n",
		             free_block->next));
		return B_ERROR;
	}
	if(free_block->next != 0 &&
	   free_block->next <= free_block_offset + FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)) {
		PRINT_ERROR(("cfs_write_free_block: bad next free block offset, "
		             "%Ld <= %Ld+%Ld\n", free_block->next, free_block_offset,
		             FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)));
		return B_ERROR;
	}
	if((FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) & 7) ||
		FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) <= 0) {
		PRINT_ERROR(("cfs_write_free_block: bad free block size, %Ld\n",
		             FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)));
		return B_ERROR;
	}
	if(free_block_offset + FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version) > fsinfo->size) {
		PRINT_ERROR(("cfs_write_free_block: free block at %Ld would extend "
					 "past end of fs\n", free_block_offset));
		return B_ERROR;
	}
	
	switch(FREEBLOCK_SIZE_FIELD(free_block, fsinfo->fs_version)) {
		case 8:		
			free_block->next |= 1;
			writesize = 8;			
			break;
		case 16:
			if(fsinfo->fs_version > 1) {
				free_block->next |= 2;
				writesize = 16;
				break;
			}
		default:
			writesize = get_freeblock_size(fsinfo->fs_version);
	}
			
	err = cfs_write_disk(fsinfo, transaction, free_block_offset, free_block,
	                     writesize);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_write_free_block: could not write free block, %s\n",
		             strerror(err)));
		return err;
	}
	return B_NO_ERROR;
}

static off_t
cfs_load_free_space_counter(cfs_info *info)
{
	// the info is not cached, so we need to count the size
	status_t err;
	cfs_super_block_2 sb2;
	cfs_free_block free_block;
	off_t free_space = 0;

	err = cfs_debug_check_read_lock(info);
	if(err != B_NO_ERROR)
		return 0;
	
	err = cfs_read_superblock2(info, &sb2);
	if(err != B_NO_ERROR)
		return 0;

	free_block.next = sb2.free_list;
	while(free_block.next) {
		off_t free_block_offset = free_block.next;
		
		err = cfs_read_free_block(info, free_block_offset, &free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("get_free_space: could not read free block, %s\n",
			             strerror(err)));
			return 0;
		}
		free_space += FREEBLOCK_SIZE_FIELD_NOPTR(free_block, info->fs_version);
		if(free_space > info->size) {
			PRINT_ERROR(("get_free_space: bad free block list, "
			             "free space > total space\n"));
			return 0;
		}
	}
	return free_space;
}

static void
cfs_update_free_space_counter(cfs_info *info, off_t free_space_delta)
{
	if(info->free_space_count < 0) {
		// calling this function should mean that the changes to
		// the free space list have already been made, therefore
		// we dont have apply the free_space_delta.
		info->free_space_count = cfs_load_free_space_counter(info);
	} else {
		info->free_space_count += free_space_delta;
	}
}

off_t
cfs_get_free_space(cfs_info *info)
{
	off_t free_space;
	if(info->free_space_count < 0) {
		free_space = cfs_load_free_space_counter(info);
		info->free_space_count = free_space;
	} else {
		// the info is cached
		free_space = info->free_space_count;
	}
	return free_space;
}

static status_t
_free_disk_space_v1(cfs_info *fsinfo, cfs_transaction *transaction,
                off_t pos, off_t size, cfs_super_block_2 *sb2)
{
	status_t err;
	cfs_free_block prev_free_block;
	off_t free_block_offset = 0;
	bool add_free_block;
	off_t orig_size = size;

	prev_free_block.next = sb2->free_list;
	while(prev_free_block.next < pos && prev_free_block.next != 0) {
		free_block_offset = prev_free_block.next;

		err = cfs_read_free_block(fsinfo, free_block_offset, &prev_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
			             strerror(err)));
			return err;
		}
		if(pos < free_block_offset + V1_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block)) {
			PRINT_ERROR(("cfs_free_disk_space: start of the block is already "
			             "free\n"));
			return B_IO_ERROR;
		}
	}
	if(prev_free_block.next != 0 && prev_free_block.next < pos+size) {
		PRINT_ERROR(("cfs_free_disk_space: end of the block is already free\n"));
		return B_IO_ERROR;
	}

	if(prev_free_block.next == pos+size) {
		cfs_free_block next_free_block;
		//dprintf("cfs_free_disk_space: append to next free block\n");

		err = cfs_read_free_block(fsinfo, prev_free_block.next, &next_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
			             strerror(err)));
			return err;
		}
		size += V1_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block);
		prev_free_block.next = next_free_block.next;
	}

	if(free_block_offset + V1_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) == pos) {
		//dprintf("cfs_free_disk_space: append to end of last free block\n");
		size += V1_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block);
		pos = free_block_offset;
		add_free_block = false;
	}
	else {
		add_free_block = true;
	}

	{
		cfs_free_block new_free_block;
		new_free_block.next = prev_free_block.next;
		V1_FREEBLOCK_SIZE_FIELD_NOPTR(new_free_block) = size;
		err = cfs_write_free_block(fsinfo, transaction, pos, &new_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write free block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	if(add_free_block) {
		if(free_block_offset == 0) {
			sb2->free_list = pos;
			err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
			                     sb2, get_sb2_size(fsinfo->fs_version));
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_free_disk_space: could not write superblock, "
				             "%s\n", strerror(err)));
				return err;
			}
		}
		else {
			prev_free_block.next = pos;
			err = cfs_write_free_block(fsinfo, transaction, free_block_offset,
			                           &prev_free_block);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_free_disk_space: could not write free block, "
				             "%s\n", strerror(err)));
				return err;
			}
		}
	}

	// update the free space counter
	cfs_update_free_space_counter(fsinfo, orig_size);

	return B_NO_ERROR;
}


static status_t
_free_disk_space_v2(cfs_info *fsinfo, cfs_transaction *transaction,
                off_t pos, off_t size, cfs_super_block_2 *sb2)
{
	// all of the possible blocks that can be touched (3 * 2) + 1 = 7
	status_t err;
	cfs_free_block prev_free_block;
	cfs_free_block next_free_block;
	cfs_free_block new_free_block;
	off_t prev_free_block_offset = 0;
	off_t next_free_block_offset = 0;
	off_t last_full_free_block_offset = 0;
	bool add_free_block = false;
	bool rewrite_prev_block = false;
	bool rewrite_next_block = false;
	bool update_superblock2 = false;
	
#if WATCH_FREE_SPACE
	off_t orig_pos = pos;
#endif
	off_t orig_size = size;
	
	memset(&new_free_block, 0, sizeof(cfs_free_block));
	
	D(("entry, walking...\n"));
	
	// walk until we find the block immediately before where this one will be
	// and the one we may have to point back at.
	prev_free_block.next = sb2->free_list;
	while(prev_free_block.next > 0 && prev_free_block.next < pos) {		
		prev_free_block_offset = prev_free_block.next;
		
		//D(("going to read in block %Ld\n", prev_free_block_offset));
			
		err = cfs_read_free_block(fsinfo, prev_free_block_offset, &prev_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
			             strerror(err)));
			return err;
		}
		
		if(pos < prev_free_block_offset + V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block)) {
			PRINT_ERROR(("cfs_free_disk_space: start of the block is already "
			             "free\n"));
			return B_IO_ERROR;
		}
		
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) > 8) {
			// do some freelist verification while we're here
			if(prev_free_block.u.v2.prev != last_full_free_block_offset) {
				PRINT_ERROR(("cfs_free_disk_space: freelist block %Ld's prev pointer is invalid."
					" should be %Ld is %Ld\n", prev_free_block_offset,
					last_full_free_block_offset, prev_free_block.u.v2.prev));
				return B_IO_ERROR;
			}
	
			// save it into last_full_free_block
			last_full_free_block_offset = prev_free_block_offset;	
		}
	}
	
	D(("after first search. prev %Ld, last_full %Ld, last_last_full %Ld\n",
		prev_free_block_offset, last_full_free_block_offset, last_last_full_free_block_offset));
	
	// if we can, read in the next free block (the one after the spot we will be)
	if(prev_free_block.next > 0) {
		next_free_block_offset = prev_free_block.next;
		err = cfs_read_free_block(fsinfo, next_free_block_offset, &next_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
			             strerror(err)));
			return err;
		}
	}
	
	D(("next %Ld\n", next_free_block_offset));
	
	// verify that the free block doesn't overlap the next free block
	if(next_free_block_offset != 0 && next_free_block_offset < pos+size) {
		PRINT_ERROR(("cfs_free_disk_space: end of the block is already free\n"));
		return B_IO_ERROR;
	}

	// see if we can coalesce the new free block with the next block
	// note: in this case, the new block is always > 8 bytes long, so we
	// always have to deal with the prev pointer of the new block
	if(next_free_block_offset > 0 && next_free_block_offset == pos+size) {
		D(("going to coalesce %Ld with next block %Ld\n", pos, next_free_block_offset));	

		// see if the superblock is already pointing at the block being merged into
		if(sb2->free_list_end == next_free_block_offset) {
			update_superblock2 = true;
			sb2->free_list_end = pos;
			D(("sb2->free_list_end = %Ld (prev)\n", sb2->free_list_end));
		}
		if(sb2->free_list == next_free_block_offset) {
			update_superblock2 = true;
			sb2->free_list = pos;
			D(("sb2->free_list = %Ld (prev)\n", sb2->free_list));
		}
		
		add_free_block = true;
		size += V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block);
		new_free_block.next = next_free_block.next;
		
		// point to the last full free block, which may include prev
		new_free_block.u.v2.prev = last_full_free_block_offset;
		
		D(("newpos = %Ld, new.size = %Ld, new.prev = %Ld, new.next = %Ld\n", pos, size, new_free_block.u.v2.prev, new_free_block.next));

		if(prev_free_block_offset > 0) {
			rewrite_prev_block = true;
	
			prev_free_block.next = pos;
			D(("prev.next = %Ld (pos)\n", prev_free_block.next));
		}
		
		// walk forward until we see a block that has a back pointer
		// and point it to us. This will then be the next next_free_block
		next_free_block_offset = next_free_block.next;
		while(next_free_block_offset > 0) {
			err = cfs_read_free_block(fsinfo, next_free_block_offset, &next_free_block);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
				             strerror(err)));
				return err;
			}
			if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) > 8) break;
			next_free_block_offset = next_free_block.next;
		}
			
		D(("next = %Ld\n", next_free_block_offset));
		
		if(next_free_block_offset > 0) {
			// we found a block with a prev pointer
			// set it's prev pointer to us
			next_free_block.u.v2.prev = pos;
			rewrite_next_block = true;
			D(("next.prev = %Ld\n", pos));
		} else {
			// there was no large block past us, so make the superblock2 point at us
			update_superblock2 = true;
			sb2->free_list_end = pos;
			D(("sb2->free_list_end = %Ld (pos)\n", sb2->free_list_end));
		}			
	}

	// see if we can be appended to the end of the previous block
	// note: in this case, the new block is always > 8 bytes long, so we
	// always have to deal with the prev pointer of the new block
	if(prev_free_block_offset > 0 &&
		prev_free_block_offset + V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) == pos) {
		//dprintf("cfs_free_disk_space: append to end of last free block\n");

		D(("going to coalesce %Ld with prev block %Ld\n", pos, prev_free_block_offset));	

		// see if the superblock is already pointing at the block being merged
		if(sb2->free_list_end == pos) {
			update_superblock2 = true;
			sb2->free_list_end = prev_free_block_offset;
			D(("sb2->free_list_end = %Ld (prev)\n", sb2->free_list_end));
		}
		if(sb2->free_list == pos) {
			update_superblock2 = true;
			sb2->free_list = prev_free_block_offset;
			D(("sb2->free_list = %Ld (prev)\n", sb2->free_list));
		}
		
		add_free_block = true;
		size += V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block);
		pos = prev_free_block_offset;
		
		// only update the next pointer if it doesn't contain anything. It may have been
		// initialized in a coalesce with next previously
		if(new_free_block.next == 0) new_free_block.next = next_free_block_offset;
		// point to the last full free block before prev
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) > 8) {
			new_free_block.u.v2.prev = prev_free_block.u.v2.prev;
		} else {
			new_free_block.u.v2.prev = last_full_free_block_offset;
		}

		D(("newpos = %Ld, new.size = %Ld, new.next = %Ld, new.prev = %Ld\n",
			pos, size, new_free_block.next, new_free_block.u.v2.prev));

		// undo any writes that were going to happen on the prev
		// block, which we just merged with
		rewrite_prev_block = false;

		if(next_free_block_offset > 0) {	
			// if the next block is valid, push it up until we point to a next
			// block with a prev pointer (>8 bytes in size)
			if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) <= 8) {
				// the next block has no prev pointer, so walk forward to find a new one that does
				D(("next.size <= 8\n"));
				// walk forward until we see a block that has a back pointer
				// and point it to us. This will then be the next_free_block
				next_free_block_offset = next_free_block.next;
				while(next_free_block_offset > 0) {
					err = cfs_read_free_block(fsinfo, next_free_block_offset, &next_free_block);
					if(err != B_NO_ERROR) {
						PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
						             strerror(err)));
						return err;
					}
					if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) > 8) break;
					next_free_block_offset = next_free_block.next;
				}
				D(("next = %Ld\n", next_free_block_offset));
			}
		}						
	
		if(next_free_block_offset > 0) {
			// set it's prev pointer to us, if it isn't already
			if(next_free_block.u.v2.prev != pos) {
				next_free_block.u.v2.prev = pos;
				rewrite_next_block = true;
				D(("next_free.prev = %Ld (pos)\n", next_free_block.u.v2.prev));
			}
		} else {
			// there are no blocks past us with a prev pointer,
			// so the superblock needs to point at us, if it doesn't already
			if(sb2->free_list_end != pos) {
				sb2->free_list_end = pos;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (pos)\n", sb2->free_list_end));
			}
		}
	}
	
	if(!add_free_block) {
		// if we're here, we were unable to attach the new block to
		// the beginning of the next block or the end of the prev block
		// so we're going to have to add ourselves as a separate free block
		// and change the prev and next blocks to point at us

		D(("adding free_block\n"));

		add_free_block = true;
		new_free_block.next = next_free_block_offset;
	
		D(("new_free.next = %Ld (next_free)\n", new_free_block.next));
		
		if(prev_free_block_offset > 0) {
			prev_free_block.next = pos;
			rewrite_prev_block = true;
			D(("prev.next = %Ld (pos)\n", prev_free_block.next));
		} else {
			// we're the first one
			sb2->free_list = pos;
			update_superblock2 = true;
			D(("sb2->free_list = %Ld (pos)\n", sb2->free_list));
		}
	
		if(size > 8) {
			D(("we're adding size > 8\n"));
	
			// update the new prev ptr
			new_free_block.u.v2.prev = last_full_free_block_offset;
			D(("new_free.prev = %Ld (last_full)\n", new_free_block.u.v2.prev));
		
			if(next_free_block_offset > 0) {
				// push the next pointer up until it points to a block that has a prev ptr, or
				// goes past the end of the list
				if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) <= 8) {
					next_free_block_offset = next_free_block.next;
					while(next_free_block_offset > 0) {
						err = cfs_read_free_block(fsinfo, next_free_block_offset, &next_free_block);
						if(err != B_NO_ERROR) {
							PRINT_ERROR(("cfs_free_disk_space: could not read free block, %s\n",
							             strerror(err)));
							return err;
						}
						if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) > 8) break;
						next_free_block_offset = next_free_block.next;
					}
				}				
				D(("next = %Ld\n", next_free_block_offset));
			}
							
			if(next_free_block_offset > 0) {	
				// update the next block to point at us
				next_free_block.u.v2.prev = pos;
				rewrite_next_block = true;
				D(("next.prev = %Ld (pos)\n", next_free_block.u.v2.prev));
			} else {
				// we must be the last large block, so we need to update the superblock
				sb2->free_list_end = pos;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (pos)\n", sb2->free_list_end));
			}					
		}
	}

	// write out the prev block
	if(rewrite_prev_block) {
		D(("writing prev @ %Ld\n", prev_free_block_offset));
		err = cfs_write_free_block(fsinfo, transaction, prev_free_block_offset, &prev_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write prev block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	
	// write out the new free block
	if(add_free_block) {
		V2_FREEBLOCK_SIZE_FIELD_NOPTR(new_free_block) = size;
		D(("writing new @ %Ld, size %Ld\n", pos, size));
		err = cfs_write_free_block(fsinfo, transaction, pos, &new_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write free block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
		
	// write out the next block
	if(rewrite_next_block) {
		D(("writing next @ %Ld\n", next_free_block_offset));	
		err = cfs_write_free_block(fsinfo, transaction, next_free_block_offset, &next_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write next block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	
	// update the superblock
	if(update_superblock2) {
		D(("writing superblock2\n"));
		err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
		                     sb2, get_sb2_size(fsinfo->fs_version));
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write superblock, "
			             "%s\n", strerror(err)));
			return err;
		}
	}		
	
	// part of an optional set of code to verify the free space pointer
	cfs_change_debug_free_space_counter(orig_size);
	cfs_verify_free_block_list(fsinfo, orig_pos, orig_size);

	// update the free space counter
	cfs_update_free_space_counter(fsinfo, orig_size);
	
	D(("end\n"));
	
	return B_NO_ERROR;
}

status_t 
free_disk_space(cfs_info *fsinfo, cfs_transaction *transaction,
                off_t pos, off_t size)
{
	status_t err;
	cfs_super_block_2 sb2;

	D(("pos %Ld, size %Ld\n", pos, size));

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(size & 7) {
		PRINT_INTERNAL_ERROR(("cfs_free_disk_space: size %Ld is not a "
		                      "multiple of 8\n", size));
		return B_ERROR;
	}
	if(size == 0) {
		PRINT_FLOW(("cfs_free_disk_space: warning, size is 0\n"));
		return B_NO_ERROR;
	}
	if(size < 0) {
		PRINT_INTERNAL_ERROR(("cfs_free_disk_space: size is negative %Ld\n",
		                      size));
		return B_ERROR;
	}

	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_free_disk_space: error reading superblock2\n"));
		return err;
	}

	switch(fsinfo->fs_version) {
		case 1:
			return _free_disk_space_v1(fsinfo, transaction, pos, size, &sb2);
		case 2:
		default:
			return _free_disk_space_v2(fsinfo, transaction, pos, size, &sb2);
	}
}

static status_t 
_allocate_disk_space_v1(cfs_info *fsinfo, cfs_transaction *transaction,
                    off_t *pos, off_t size, int data_type, cfs_super_block_2 *sb2)
{
	status_t err;
	off_t prev_free_block_offset, free_block_offset;
	cfs_free_block free_block;
	bool prev_ptr_change = false;

	free_block_offset = sb2->free_list;
	prev_free_block_offset = 0;
	switch(data_type) {		
		default:
		case METADATA:
		case FILEDATA: {
			// walk through the free block list in forward order
			while(free_block_offset > 0) {
				err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("cfs_allocate_disk_space: could not read free block, "
					             "%s\n", strerror(err)));
					return err;
				}
				if(V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > size) break;
				prev_free_block_offset = free_block_offset;
				free_block_offset = free_block.next;
			}
			break;
		}
/*
		case METADATA: {
			cfs_free_block last_suitable_free_block;
			off_t last_suitable_free_block_offset = 0;
			off_t prev_last_suitable_free_block_offset = 0;
			// walk through the free block list in reverse order
			// this is not very optimal with only forward pointers,
			// fixed in v2 of the filesystem
			while(free_block_offset > 0) {
				err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("cfs_allocate_disk_space: could not read free block, "
					             "%s\n", strerror(err)));
					return err;
				}
				if(V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > size
					&& free_block_offset > last_suitable_free_block_offset) {
					// This is a suitable block, so save it
					prev_last_suitable_free_block_offset = prev_free_block_offset;
					last_suitable_free_block_offset = free_block_offset;
					memcpy(&last_suitable_free_block, &free_block, sizeof(cfs_free_block));
				}
				prev_free_block_offset = free_block_offset;
				free_block_offset = free_block.next;
			}
			if(last_suitable_free_block_offset > 0) {
				// we found a suitable block
				prev_free_block_offset = prev_last_suitable_free_block_offset;
				free_block_offset = last_suitable_free_block_offset;
				memcpy(&free_block, &last_suitable_free_block, sizeof(cfs_free_block));
			}
			break;
		}
*/
	}
	if(V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) < size) {
		PRINT_WARNING(("cfs_allocate_disk_space: no free block >= %Ld found\n",
		               size));
		return B_DEVICE_FULL;
	}

//	dprintf("found it: free_block_offset = 0x%Lx\n\tfree_block.next = 0x%Lx\n\tfree_block.size = 0x%Lx\n",
//		free_block_offset, free_block.next, free_block.size);

	// deal with the free block we're going to eat/split
	if(V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) == size) {
		*pos = free_block_offset;
		free_block_offset = free_block.next;
		prev_ptr_change = true;
	}
	else {	
		switch(data_type) {
			default:
			case METADATA:
			case FILEDATA: 
				*pos = free_block_offset;
				free_block_offset += size;
				prev_ptr_change = true;
				break;
/*
			case METADATA:
				*pos = free_block_offset + (V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) - size);
				break;
*/
		}
		V1_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) -= size;

		err = cfs_write_free_block(fsinfo, transaction, free_block_offset,
		                           &free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_allocate_disk_space: could not write free block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	if(prev_ptr_change) {
		// the previous free block pointer will have to be changed
		if(prev_free_block_offset == 0) {
			sb2->free_list = free_block_offset;
			err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
			                     sb2, get_sb2_size(fsinfo->fs_version));
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_allocate_disk_space: could not write superblock, "
				             "%s\n", strerror(err)));
				return err;
			}
		}
		else {
			cfs_free_block prev_free_block;
			err = cfs_read_free_block(fsinfo, prev_free_block_offset,
			                          &prev_free_block);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_allocate_disk_space: could not read free block, "
				             "%s\n", strerror(err)));
				return err;
			}
			prev_free_block.next = free_block_offset;
			err = cfs_write_free_block(fsinfo, transaction, prev_free_block_offset,
			                           &prev_free_block);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_allocate_disk_space: could not write free block, "
				             "%s\n", strerror(err)));
				return err;
			}
		}
	}
	
	// update the free space counter
	cfs_update_free_space_counter(fsinfo, -size);
	
	//dprintf("cfs_allocate_disk_space: size %Ld\n", size);
	return B_NO_ERROR;
}

static status_t 
_allocate_disk_space_v2(cfs_info *fsinfo, cfs_transaction *transaction,
                    off_t *pos, off_t size, int data_type, cfs_super_block_2 *sb2)
{
	status_t err;

	// all of the possible blocks that can be touched (3 * 2) + 1 = 7
	cfs_free_block prev_free_block;
	off_t prev_free_block_offset = 0;
	bool rewrite_prev_free_block = false;

	cfs_free_block free_block;
	off_t free_block_offset = 0;
	bool rewrite_free_block = false;

	cfs_free_block next_free_block;
	off_t next_free_block_offset = 0;
	bool rewrite_next_free_block = false;
		
	bool update_superblock2 = false;

	off_t last_full_free_block_offset = 0;
	cfs_free_block last_full_free_block;

	D(("entry\n"));

	// XXX check for loops and other list corruption here
		
	// walk through the list, finding the block that will be shortened/eaten,
	// last full previous block, previous (may be same as last full), and next block w/prev pointer
	switch(data_type) {		
		// walk through the free block list in forward order
		default:		
		case FILEDATA: {
			free_block_offset = sb2->free_list;		
			// find the block and the previous
			while(free_block_offset > 0) {
				err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
					             "%s\n", strerror(err)));
					return err;
				}
				if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > size) break;
				
				prev_free_block_offset = free_block_offset;
				memcpy(&prev_free_block, &free_block, V2_FREEBLOCK_LEN);
				
				if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
					// save this pointer as the last full free block
					last_full_free_block_offset = free_block_offset;
					memcpy(&last_full_free_block, &prev_free_block, V2_FREEBLOCK_LEN);
				}
				
				free_block_offset = free_block.next;
			}
			if(free_block_offset <= 0) break;
			// find the next block w/prev pointer
			next_free_block_offset = free_block.next;
			while(next_free_block_offset > 0) {
				err = cfs_read_free_block(fsinfo, next_free_block_offset, &next_free_block);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
					             "%s\n", strerror(err)));
					return err;
				}
				if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(next_free_block) > 8) break;
				next_free_block_offset = next_free_block.next;
			}
			break;
		}
		// start at the end of the list and walk backwards
		// NOTE: this is a different algorithm depending on if we're looking for an
		// 8 byte block or a 16 and above sized block
		case METADATA: {
			if(size >= 16) {
				D(("searching for metadata > 16 (%Ld)\n", size));
				// 16 or greater, so we walk the list backwards similar to
				// walking forwards
				free_block_offset = sb2->free_list_end;			
				// find the block, saving next along the way
				while(free_block_offset > 0) {
					err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
					if(err != B_NO_ERROR) {
						PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
						             "%s\n", strerror(err)));
						return err;
					}
					//D(("examining block %Ld: next %Ld, prev %Ld, size %Ld\n",
					//	free_block_offset, free_block.next, free_block.u.v2.prev,
					//	V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block)));
					if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > size) break;

					next_free_block_offset = free_block_offset;
					memcpy(&next_free_block, &free_block, V2_FREEBLOCK_LEN);

					free_block_offset = free_block.u.v2.prev;
				}
				
				//D(("end of walk. free = %Ld, next = %Ld\n", free_block_offset, next_free_block_offset));
				if(free_block_offset <= 0) break;
				
				if(free_block.u.v2.prev > 0) {
					// find the last full prev block
					D(("free.prev > 0: searching for last full prev\n"));
					last_full_free_block_offset = free_block.u.v2.prev;
					err = cfs_read_free_block(fsinfo, last_full_free_block_offset, &last_full_free_block);
					if(err != B_NO_ERROR) {
						PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
						             "%s\n", strerror(err)));
						return err;
					}
					//D(("last_full = %Ld\n", last_full_free_block_offset));	
					
					// find the prev block (the block that points to the free block)
					// may be same as last_full_free_block. This case happens when
					// prev is 8 bytes in size, and therefore between the last_full
					// and free blocks.
					if(last_full_free_block.next != free_block_offset) {
						// the next ptr of last_full isn't pointing at free,
						// so there must be small blocks between them
						prev_free_block_offset = last_full_free_block.next;
						while(prev_free_block_offset > 0) {
							err = cfs_read_free_block(fsinfo, prev_free_block_offset, &prev_free_block);
							if(err != B_NO_ERROR) {
								PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
								             "%s\n", strerror(err)));
								return err;
							}
							// make sure we're looking at an 8 byte block
							if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) != 8) {
								PRINT_ERROR(("_allocate_disk_space_v2: bad size on block %Ld, "
									"should be 8 is %Ld\n", prev_free_block_offset,
									V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block)));
								return B_IO_ERROR;
							}
							if(prev_free_block.next == free_block_offset) break;
							prev_free_block_offset = prev_free_block.next;
						}
					} else {
						// prev_free_block == last_full_free_block
						prev_free_block_offset = last_full_free_block_offset;
						memcpy(&prev_free_block, &last_full_free_block, V2_FREEBLOCK_LEN);
					}
					
					//D(("end of last_full search. last_full %Ld\n", last_full_free_block_offset));					
				} else if(sb2->free_list != free_block_offset) {
					// free_block's prev pointer is null, but the superblock2 does not
					// point at free_block, so there must be at least one 8 byte block
					// before free_block. We have to start at the beginning of the list
					// and walk until we find the block that points to free_block
					D(("free.prev == 0, sb2_free_list != free: walking from beginning for prev\n"));
					prev_free_block_offset = sb2->free_list;
					while(prev_free_block_offset > 0) {
						err = cfs_read_free_block(fsinfo, prev_free_block_offset, &prev_free_block);
						if(err != B_NO_ERROR) {
							PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
							             "%s\n", strerror(err)));
							return err;
						}
						// make sure we're looking at an 8 byte block
						if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block) != 8) {
							PRINT_ERROR(("_allocate_disk_space_v2: bad size on block %Ld, "
								"should be 8 is %Ld\n", prev_free_block_offset,
								V2_FREEBLOCK_SIZE_FIELD_NOPTR(prev_free_block)));
							return B_IO_ERROR;
						}
						if(prev_free_block.next == free_block_offset) break;
						prev_free_block_offset = prev_free_block.next;
					}
					D(("end of last_full search. prev %Ld\n", prev_free_block_offset));					
				} else {
					// free_block's prev pointer is null, and the superblock2 points at us,
					// so we must not have any predecessors
					D(("free.prev == 0, sb2->free_list == free: no previous ptrs\n"));
				}
			} else {
				// looking for an 8 byte block:
				// two cases here:
				//  sb2->free_list_end is valid, so start with that block and walk forwards
				//   through any 8 byte blocks we see. If there are no 8 byte blocks past
				//   the free_list_end, that block will be shortened.
				//   Also, if the block at free_list_end's next and prev ptr == 0, fall
				//   through and use the next algorithm.
				//  sb2->free_list_end is zero, so the entire freelist is composed
				//   of just 8 byte blocks. Do the old algorithm and walk forward to the
				//   end of the freelist and grab the last block.
				bool use_forward_search = false;
				
				if(sb2->free_list_end > 0) {
					// read in the end of the free list and check to see if it has
					// null prev & next pointers
					free_block_offset = sb2->free_list_end;
					err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
					if(err != B_NO_ERROR) {
						PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
						             "%s\n", strerror(err)));
						return err;
					}
					if(free_block.next == 0 && free_block.u.v2.prev == 0) {
						// use the linear forward search
						use_forward_search = true;
					}
				} else {
					// the list is composed of entirely 8 byte blocks
					// so use the linear forward search
					use_forward_search = true;
				}
			
				if(!use_forward_search) {
					// start at free_list_end and walk forward to the end
					// next block should end up null
					free_block_offset = sb2->free_list_end;			
					while(free_block_offset > 0) {
						err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
						if(err != B_NO_ERROR) {
							PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
							             "%s\n", strerror(err)));
							return err;
						}
						if(free_block.next == 0) break;
						
						prev_free_block_offset = free_block_offset;
						memcpy(&prev_free_block, &free_block, V2_FREEBLOCK_LEN);

						if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
							// save this pointer as the last full free block
							// should end up being just the block pointed to by sb2->free_list_end
							last_full_free_block_offset = free_block_offset;
							memcpy(&last_full_free_block, &prev_free_block, V2_FREEBLOCK_LEN);
						}
				
						free_block_offset = free_block.next;
					}

					if(prev_free_block_offset == 0) {
						// we were not able to walk forward any from sb2->free_list_end,
						// so it must have been the last.
						// therefore, we need to walk back once to get the last_full block
						// and then walk forward to find the prev block.

						if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) <= 8) {
							// we should not be here. If free_block is size 8, we had
							// to have walked to it from sb2->free_list_end, and in that
							// case prev_free_block_offset should be something
							PRINT_ERROR(("_allocate_disk_space_v2: free_block %Ld should be size 8, is size %Ld\n",
								free_block_offset, V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block)));
							return B_IO_ERROR;
						}

						// find the last full prev block
						// free_block is guaranteed to have a prev pointer since to be here it would
						// have had to have no next pointer as well, and the previous check would
						// have made us set the use_forward_search flag. So we will just walk back one
						// to get the last_full block.
						last_full_free_block_offset = free_block.u.v2.prev;
						err = cfs_read_free_block(fsinfo, last_full_free_block_offset, &last_full_free_block);
						if(err != B_NO_ERROR) {
							PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
							             "%s\n", strerror(err)));
							return err;
						}

						// find the prev block (the block that points to the free block) by
						// walking forward from last_full_free_block until we find a block
						// whose next ptr is free_block. It may be same as last_full_free_block.
						if(last_full_free_block.next != free_block_offset) {
							prev_free_block_offset = last_full_free_block.next;
							while(prev_free_block_offset > 0) {
								err = cfs_read_free_block(fsinfo, prev_free_block_offset, &prev_free_block);
								if(err != B_NO_ERROR) {
									PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
									             "%s\n", strerror(err)));
									return err;
								}
								if(prev_free_block.next == free_block_offset) break;
								prev_free_block_offset = prev_free_block.next;
							}
							// sanity check
							if(prev_free_block.next != free_block_offset) {
								// we had to have gotten here by walking off the end of the list
								// this makes no sense.
								PRINT_ERROR(("_allocate_disk_space_v2: walked off end of list where"
									" we shouldn't have\n"));
								return B_IO_ERROR;
							}
						} else {
							// prev_free_block == last_full_free_block
							prev_free_block_offset = last_full_free_block_offset;
							memcpy(&prev_free_block, &last_full_free_block, V2_FREEBLOCK_LEN);
						}
					}
				} else {
					// case use_forward_search
					// just start from the beginning of the list and walk forward	
					free_block_offset = sb2->free_list;
					while(free_block_offset > 0) {
						err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
						if(err != B_NO_ERROR) {
							PRINT_ERROR(("_allocate_disk_space_v2: could not read free block, "
							             "%s\n", strerror(err)));
							return err;
						}
						if(free_block.next == 0) break; // end of list

						prev_free_block_offset = free_block_offset;
						memcpy(&next_free_block, &next_free_block, V2_FREEBLOCK_LEN);
						if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
							// save this pointer as the last full free block
							last_full_free_block_offset = free_block_offset;
							memcpy(&last_full_free_block, &prev_free_block, V2_FREEBLOCK_LEN);
						}						

						free_block_offset = free_block.next;
					}					
				}				
			}
		}
	}

	if(free_block_offset == 0 || V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) < size) {
		// we couldn't find a spot
		PRINT_INFO(("_allocate_disk_space_v2: no free block >= %Ld found\n",
		               size));
		return B_DEVICE_FULL;
	}

	// at this point, we should have:
	//  free_block block that is the block we will eat/shrink
	//  prev_block block immediately previous to free_block
	//  last_full_block last block before free_block that has next & prev ptrs (may be prev_block)
	//  next_block first block after free_block that has a prev pointer
	
	D(("found block to eat/shorten. free_block %Ld, prev %Ld, last_full %Ld, next %Ld\n",
		free_block_offset, prev_free_block_offset, last_full_free_block_offset, next_free_block_offset));
	D(("free block %Ld: free.next %Ld, free.prev %Ld, free.size %Ld\n",
		free_block_offset, free_block.next, free_block.u.v2.prev, V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block)));
	// calculate where the space will be allocated
	switch(data_type) {
		default:
		case FILEDATA: 
			*pos = free_block_offset;
			break;
		case METADATA:
			*pos = free_block_offset + (V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) - size);
			break;
	}
	
	D(("spot will be %Ld, size %Ld\n", *pos, size));
	
	if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) == size) {
		// we will eat this block entirely
		
		D(("block will be eaten entirely\n"));
		
		// change the prev block to point past us
		if(prev_free_block_offset > 0) {
			prev_free_block.next = free_block.next;
			rewrite_prev_free_block = true;
			D(("prev.next = %Ld (free.next)\n", prev_free_block.next));
		}

		// if we were pointed to by the superblock, change that
		if(sb2->free_list == free_block_offset) {
			sb2->free_list = free_block.next;
			update_superblock2 = true;
			D(("sb2->free_list = %Ld (free.next)\n", sb2->free_list));
		}

		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
			// we used to be a large block, so stuff may have pointed back at us	
			if(next_free_block_offset > 0) {
				// point the next block's prev pointer to the last previous full block, or 0
				next_free_block.u.v2.prev = last_full_free_block_offset;
				rewrite_next_free_block = true;
				D(("next.prev = %Ld (last_full)\n", next_free_block.u.v2.prev));
			}
			
			// if we were pointed to by the superblock, change that
			if(sb2->free_list_end == free_block_offset) {
				sb2->free_list_end = last_full_free_block_offset;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (last_full)\n", sb2->free_list_end));				
			}
		}
	} else if(*pos == free_block_offset) {
		// we will be shrinking the block, and moving it's start up a bit
		off_t old_free_block_offset = free_block_offset;
		
		D(("block will be shrunk, start moved up\n"));
		
		free_block_offset += size;
		V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) -= size;
		rewrite_free_block = true;
		
		D(("free = %Ld, free.size = %Ld\n", free_block_offset, V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block)));
		
		// change the prev pointer to point to the new free_block
		if(prev_free_block_offset > 0) {
			prev_free_block.next = free_block_offset;
			rewrite_prev_free_block = true;
			D(("prev.next = %Ld (free)\n", prev_free_block.next));
		}

		// if we were pointed to by the superblock, change that
		if(sb2->free_list == old_free_block_offset) {
			sb2->free_list = free_block_offset;
			update_superblock2 = true;
			D(("sb2->free_list = %Ld (free)\n", sb2->free_list));
		}

		// do different stuff depending on if we are now small or not
		// we had to have been a large block before
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) == 8) {
			// we are now small, stuff may have pointed back at us

			D(("free.size == 8\n"));

			if(next_free_block_offset > 0 &&
				next_free_block.u.v2.prev == old_free_block_offset) {
				// point the next block's prev pointer to the last previous full block, or 0
				next_free_block.u.v2.prev = last_full_free_block_offset;
				rewrite_next_free_block = true;
				D(("next.prev = %Ld (last_full)\n", next_free_block.u.v2.prev));
			}			

			if(sb2->free_list_end == old_free_block_offset) {
				sb2->free_list_end = last_full_free_block_offset;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (last_full)\n", sb2->free_list_end));
			}
		} else {
			// we are still large, but we may have to modify next.prev	
			if(next_free_block_offset > 0) {
				// point the next block's prev pointer to the new free_block
				next_free_block.u.v2.prev = free_block_offset;
				rewrite_next_free_block = true;
				D(("next.prev = %Ld (free)\n", next_free_block.u.v2.prev));
			}			

			if(sb2->free_list_end == old_free_block_offset) {
				sb2->free_list_end = free_block_offset;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (free)\n", sb2->free_list_end));
			}			
		}
	} else {
		// we will be shrinking the block by just changing the size field
		D(("block will be shrunk\n"));
	
		V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) -= size;
		rewrite_free_block = true;
		
		D(("free.size = %Ld\n", V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block)));

		// do some stuff if we're small now
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) == 8) {
			// we are now small, stuff may have pointed back at us

			D(("free.size == 8\n"));

			if(next_free_block_offset > 0) {
				// point the next block's prev pointer to the last previous full block, or 0
				next_free_block.u.v2.prev = last_full_free_block_offset;
				rewrite_next_free_block = true;
				D(("next.prev = %Ld (last_full)\n", next_free_block.u.v2.prev));
			}			

			if(sb2->free_list_end == free_block_offset) {
				sb2->free_list_end = last_full_free_block_offset;
				update_superblock2 = true;
				D(("sb2->free_list_end = %Ld (last_full)\n", sb2->free_list_end));
			}
		}
	}

	// write out the prev block
	if(rewrite_prev_free_block) {
		D(("writing prev @ %Ld\n", prev_free_block_offset));
		err = cfs_write_free_block(fsinfo, transaction, prev_free_block_offset, &prev_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("_allocate_disk_space_v2: could not write prev block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	
	// write out the free block
	if(rewrite_free_block) {
		D(("writing free @ %Ld\n", free_block_offset));
		err = cfs_write_free_block(fsinfo, transaction, free_block_offset, &free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("_allocate_disk_space_v2: could not write free block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
		
	// write out the next block
	if(rewrite_next_free_block) {
		D(("writing next @ %Ld\n", next_free_block_offset));
		err = cfs_write_free_block(fsinfo, transaction, next_free_block_offset, &next_free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("_allocate_disk_space_v2: could not write next block, "
			             "%s\n", strerror(err)));
			return err;
		}
	}
	
	// update the superblock
	if(update_superblock2) {
		D(("writing superblock2\n"));
		err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
		                     sb2, get_sb2_size(fsinfo->fs_version));
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_free_disk_space: could not write superblock, "
			             "%s\n", strerror(err)));
			return err;
		}
	}		

	//dprintf("_allocate_disk_space_v2: size %Ld\n", size);
	
	// part of optional code to verify free space list
	cfs_change_debug_free_space_counter(-size);
	cfs_verify_free_block_list(fsinfo, 0, 0);

	// update the free space counter
	cfs_update_free_space_counter(fsinfo, -size);

	D(("found %Ld bytes at %Ld\n", size, *pos));

	return B_NO_ERROR;
}

status_t 
allocate_disk_space(cfs_info *fsinfo, cfs_transaction *transaction,
                    off_t *pos, off_t size, int data_type)
{
	status_t err;
	cfs_super_block_2 sb2;

#if 0
	dprintf("allocate_disk_space to allocate %Ld ", size);
	if(data_type == METADATA) {
		dprintf("metadata");
	} else {
		dprintf("filedata");
	}
	dprintf(" bytes\n");
#endif
	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(size <= 0) {
		PRINT_INTERNAL_ERROR(("cfs_allocate_disk_space: size %Ld is "
		                      "too small\n", size));
		return B_ERROR;
	}
	if(size & 7) {
		PRINT_INTERNAL_ERROR(("cfs_allocate_disk_space: size %Ld is not "
		                      "a multiple of 8\n", size));
		return B_ERROR;
	}

	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_allocate_disk_space: error reading superblock2\n"));
		return err;
	}

	switch(fsinfo->fs_version) {
		case 1:
			return _allocate_disk_space_v1(fsinfo, transaction, pos, size, data_type, &sb2);
		case 2:
		default:
			return _allocate_disk_space_v2(fsinfo, transaction, pos, size, data_type, &sb2);
	}
}

#if WATCH_FREE_SPACE

static void
cfs_change_debug_free_space_counter(off_t delta)
{
	free_space_size += delta;
}

static status_t
_cfs_verify_free_block_list_v2(cfs_info *fsinfo, cfs_super_block_2 *sb2, off_t free_space_offset, off_t size)
{
	status_t err;
	cfs_free_block free_block;
	off_t free_block_offset = 0;	
	off_t last_full_free_block_offset = 0;
	bool free_space_covered = false;
	off_t counted_free_space = 0;
	
	D(("verifying list\n"));
	
	free_block_offset = sb2->free_list;	
	while(free_block_offset > 0) {
		err = cfs_read_free_block(fsinfo, free_block_offset, &free_block);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("_cfs_verify_free_block_list_v2: could not read free block, %s\n",
			             strerror(err)));
			return err;
		}
		
		// check to see if this block covers the free_space_offset
		if(free_space_offset > 0) {
			if(free_block_offset <= free_space_offset &&
				(free_block_offset + V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) - 1) >= (free_space_offset + size - 1)) {
					free_space_covered = true;
					D(("block from %Ld to %Ld covers %Ld to %Ld\n",
						free_block_offset, 	free_block_offset + V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) - 1,
						free_space_offset, free_space_offset + size - 1));
			}
		}
		
		// verify prev ptr
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
			if(free_block.u.v2.prev != last_full_free_block_offset) {
				PRINT_ERROR(("_cfs_verify_free_block_list_v2: prev ptr of free block %Ld is invalid. is %Ld, should be %Ld\n",
					free_block_offset, free_block.u.v2.prev, last_full_free_block_offset));
				return B_ERROR;
			}
		}
			
		if(V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block) > 8) {
			last_full_free_block_offset = free_block_offset;
		}
		
		counted_free_space += V2_FREEBLOCK_SIZE_FIELD_NOPTR(free_block);
		
		free_block_offset = free_block.next;
	}
	
	if(sb2->free_list_end != last_full_free_block_offset) {
		PRINT_ERROR(("_cfs_verify_free_block_list_v2: sb2->free_list_end does not point to end of list. is %Ld should be %Ld\n",
			sb2->free_list_end, last_full_free_block_offset));
		return B_ERROR;
	}
	
	if(free_space_offset > 0 && free_space_covered == false) {
		PRINT_ERROR(("_cfs_verify_free_block_list_v2: free space %Ld - %Ld not entirely covered in free space list\n",
			free_space_offset, free_space_offset + size - 1));
		return B_ERROR;
	}
	
	if(free_space_size_initialized == false) {
		free_space_size = counted_free_space;
		free_space_size_initialized = true;
	} else {
		if(counted_free_space != free_space_size) {
			PRINT_ERROR(("_cfs_verify_free_block_list_v2: counted %Ld bytes free, should be %Ld bytes free\n",
				counted_free_space, free_space_size));
			return B_ERROR;
		}
	}

	D(("done verifying list\n"));

	return B_OK;
}

static status_t
cfs_verify_free_block_list(cfs_info *fsinfo, off_t free_space_offset, off_t size)
{
	status_t err;
	cfs_super_block_2 sb2;

	// read in sb2
	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR)
		return err;
	
	switch(fsinfo->fs_version) {
		default:
		case 2:
			return _cfs_verify_free_block_list_v2(fsinfo, &sb2, free_space_offset, size);
		case 1:
			return B_OK; // none for now
	}
}
#endif

