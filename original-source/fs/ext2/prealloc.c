#ifndef RO

#include "ext2.h"
#include "file_extent.h"
#include "block_alloc.h"
#include "prealloc.h"

// These functions serve to get blocks for a file through a preallocation list.

int FillVnodePreallocList(ext2_fs_struct *ext2, vnode *file_vnode)
{
	int blocks_found, z;
	status_t err;
	uint32 last_block;
	
	DEBUGPRINT(ALLOC, 5, ("FillVnodePreallocList entry, calling ext2_find_n_free_blocks.\n"));

	err = ext2_get_last_block_num_from_extent(ext2, file_vnode, &last_block);
	if(err < B_NO_ERROR) {
		ERRPRINT(("FillVnodePreallocList had problem calculating last block in file.\n"));
		return 0;
	}

	blocks_found = ext2_find_n_free_blocks(ext2, file_vnode->pre_blocks, last_block+1, 8);
	DEBUGPRINT(ALLOC, 5, ("FillVnodePreallocList just called ext2_find_n_free_blocks, found these blocks: "));
	for(z=0; z<blocks_found; z++) {
		AddBlockToPreAllocList(ext2, file_vnode->pre_blocks[z]);
		DEBUGPRINT(ALLOC, 5, ("%d ", (int)file_vnode->pre_blocks[z]));
	}
	DEBUGPRINT(ALLOC, 5, ("\n"));
	file_vnode->pre_pos = 0;

	return blocks_found;
	
}

uint32 GetBlockFromFilePreallocList(ext2_fs_struct *ext2, vnode *file_vnode)
{
	uint32 block;
		
	if((file_vnode->pre_pos >7) | (file_vnode->pre_pos < 0)) {
		// We need to fill the prealloc buffer again
		{
			int blocks_in_list = FillVnodePreallocList(ext2, file_vnode);
		 	// If we find less than 8 blocks, we should just quit. May fix this later.
		 	if(blocks_in_list < 8) {
		 		ERRPRINT(("GetBlockFromFilePreallocList found less than 8 blocks to fill the prealloc buffer.\n"));
		 		return 0;
		 	}
		}
	}
	
	block = file_vnode->pre_blocks[file_vnode->pre_pos];
	file_vnode->pre_pos++; 
	

	return block;

}

// Remove all of the prealloced blocks for a certain vnode.
int CleanUpPreAlloc(ext2_fs_struct *ext2, vnode *file_vnode)
{
	int i;

	DEBUGPRINT(ALLOC, 5, ("CleanUpPreAlloc called on vnode %Ld.\n", file_vnode->vnid));

	if((file_vnode->pre_pos >= 0) && (file_vnode->pre_pos <= 7)) {
		for(i=file_vnode->pre_pos; i<=8-file_vnode->pre_pos; i++) {
			RemoveBlockFromPreAllocList(ext2, *(file_vnode->pre_blocks + i));
			DEBUGPRINT(ALLOC, 5, ("CleanUpPreAlloc removing block %d from list.\n", (int)*(file_vnode->pre_blocks + i)));
		}
	}
	
	return B_NO_ERROR;		
	
}

// These functions exist to manipulate a block preallocation list that keeps track of which blocks are preallocated. It saves
// time because the block allocators no longer have to look at the block bitmap table on the disk. A block can be preallocated without
// any needs for writes.

// Add a block to the preallocation list
int AddBlockToPreAllocList(ext2_fs_struct *ext2, uint32 block_num)
{
	prealloc_item *temp;
	acquire_sem(ext2->prealloc_sem);


	temp = (prealloc_item *)malloc(sizeof(prealloc_item));

	if(!ext2->fFirstPreAlloc) {
		// Put me at the head of the list 
		ext2->fFirstPreAlloc = temp;
		temp->next = NULL;
	} else {
		temp->next = ext2->fFirstPreAlloc;
		ext2->fFirstPreAlloc = temp;
	}
	
	ext2->fNumPreAllocs++;
	
	release_sem(ext2->prealloc_sem);
	
	return B_NO_ERROR;
	
}

// Remove the block from the preallocation list
int RemoveBlockFromPreAllocList(ext2_fs_struct *ext2, uint32 block_num)
{
	prealloc_item *temp, *temp1=NULL;
	acquire_sem(ext2->prealloc_sem);

	
	// Have to go find it first, and cant use the search function because of the locks
	if(ext2->fNumPreAllocs <=0) {
		release_sem(ext2->prealloc_sem);
		return B_ERROR;
	}
	
	// Look for it 
	temp = ext2->fFirstPreAlloc;
	while((temp) && (temp->block_num != block_num)) { 
		temp1 = temp;
		temp = temp->next;
	}
	// See if we found it or went past the end
	if((temp) && (temp->block_num == block_num)) {
		if(temp == ext2->fFirstPreAlloc) {
			// Remove it from the head of the list
			ext2->fFirstPreAlloc = temp->next;
		} else {
			// Point the previous past this one
			temp1->next = temp->next;
		}
		// Lets delete it
		free(temp);
	} else {
	    release_sem(ext2->prealloc_sem);
		return B_ERROR;
	}
		
	ext2->fNumPreAllocs--;		
	
	release_sem(ext2->prealloc_sem);	
		
	return B_NO_ERROR;
}

// Return true or false if the block is in the preallocation list
bool FindPreAlloc(ext2_fs_struct *ext2, uint32 block_num)
{
	prealloc_item *temp;

	acquire_sem(ext2->prealloc_sem);

	temp = ext2->fFirstPreAlloc;
	while((temp) && (temp->block_num != block_num))
		temp = temp->next;
	if(temp) 
		if(temp->block_num == block_num) {
			release_sem(ext2->prealloc_sem);	
			return true;
		}

	release_sem(ext2->prealloc_sem);	
	return false;
}

#endif
