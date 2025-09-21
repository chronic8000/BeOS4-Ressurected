#include "cfs.h"
#include "cfs_vnode.h"
#include "cfs_debug.h"
//#include "cfs_entry.h"
#include "cfs_compress.h"
#include "cfs_free_space.h"
#include <KernelExport.h>
//#include <errno.h>
//#include <string.h>
#include <malloc.h>

static status_t
cfs_update_datablock(cfs_info *fsinfo, cfs_transaction *transaction,
                     cfs_vblock_list *vblock, cfs_vblock_list *new_vblock)
{
	status_t err;
	cfs_data_block block;

	if(new_vblock->location != vblock->location) {
		PRINT_INTERNAL_ERROR(("cfs_update_datablock: can't change "
		                      "block location\n"));
		return B_ERROR;
	}
	if(memcmp(vblock, new_vblock, sizeof(cfs_vblock_list)) == 0)
		return B_NO_ERROR;
		
	err = cfs_read_disk(fsinfo, vblock->location, &block, sizeof(block));
	if(err != B_NO_ERROR)
		return err;

	if(new_vblock->next != NULL)
		block.next = new_vblock->next->location;
	else
		block.next = 0;
	block.data = new_vblock->data_pos;
	if(new_vblock->flags) {
		block.disk_size = new_vblock->compressed_size;
		block.raw_size = new_vblock->size;
	}
	else {
		block.disk_size = new_vblock->size;
		block.raw_size = 0;
	}

	err = cfs_write_disk(fsinfo, transaction, vblock->location,
	                     &block, sizeof(block));
	if(err != B_NO_ERROR)
		return err;
	
	*vblock = *new_vblock;
	return B_NO_ERROR;
}

static status_t
cfs_read_compressed_block(cfs_info *fsinfo, cfs_vblock_list *block,
                          size_t offset, void *buffer, size_t len)
{
	status_t err;
	uint8 *cbuffer = NULL;

	if(block->compressed_size == 0) {
		if(block->data_pos != 0) {
			PRINT_INTERNAL_ERROR(("cfs_read_compressed_block: empty block "
			                      "had data pointer: %Ld\n", block->data_pos));
			return B_IO_ERROR;
		}
		memset(buffer, 0, len);
		return B_NO_ERROR;
	}
	if(block->data_pos == 0) {
		PRINT_INTERNAL_ERROR(("cfs_read_compressed_block: no data pointer for "
		                      "compressed block at %Ld\n", block->location));
		return B_IO_ERROR;
	}

	if(offset > block->size || offset+len > block->size) { /* unsigned offset */
		PRINT_INTERNAL_ERROR(("cfs_read_compressed_block: tried to read "
		                      "%ld-%ld from block of size %Ld\n",
		                      offset, offset+len-1, block->size));
		return B_ERROR;
	}

	if(block->size > cfs_compressed_block_size) {
		PRINT_INTERNAL_ERROR(("cfs_read_compressed_block: compressed block "
			"%ld is too big (%Ld)\n", offset, block->size));
		return B_ERROR;
	}
	
	LOCK(fsinfo->decompression_buf_lock);
	// check to see if the global decompression buffer has it already
	if(fsinfo->last_decompression_offset != block->data_pos) {
		// has some other or no data in the buffer
		
		// the buffer was never used, get some memory
		if(fsinfo->decompression_buf == NULL) {
			fsinfo->decompression_buf = malloc(cfs_compressed_block_size);
			if(fsinfo->decompression_buf == NULL) {
				err = B_NO_MEMORY;
				goto err2;
			} 
		}
		
		// unlock to do the i/o
		UNLOCK(fsinfo->decompression_buf_lock);
		
		cbuffer = malloc(block->compressed_size);
		if(cbuffer == NULL) {
			err = B_NO_MEMORY;
			goto err1;
		}
		err = cfs_read_disk(fsinfo, block->data_pos, cbuffer,
		                    block->compressed_size);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_read_compressed_block: could not read from disk\n"));
			goto err1;
		}

		// we've finished the i/o, check to see if the buffer is still ok
		LOCK(fsinfo->decompression_buf_lock);
		// check to see if the global decompression buffer has it already again
		if(fsinfo->last_decompression_offset != block->data_pos) {
			// clear the decompression buf info first
			fsinfo->last_decompression_offset = -1;

			err = cfs_decompress(cbuffer, block->compressed_size,
			                     fsinfo->decompression_buf, block->size);
			if(err != B_NO_ERROR) {
				goto err2;
			}	
			// update the info about the decompression buf
			fsinfo->last_decompression_offset = block->data_pos;
		}
	}		
		
	// copy the data out of the buffer
	memcpy(buffer, fsinfo->decompression_buf + offset, len);

	err = B_NO_ERROR;

err2:
	UNLOCK(fsinfo->decompression_buf_lock);
err1:
	if(cbuffer != NULL) free(cbuffer);	
	return err;
}

status_t 
cfs_read_data(cfs_info *fsinfo, cfs_node *node, off_t pos,
              void *buf, size_t *len)
{
	status_t err;
	off_t cur_filepos = 0;
	cfs_vblock_list *cur_block;
	size_t request_len = *len;
	uint8 *cur_buffer = buf;

	*len = 0;
	
	if(!S_ISREG(node->flags)) {
		return B_BAD_VALUE;
	}

	err = cfs_load_blocklist(fsinfo, node);
	if(err != B_NO_ERROR)
		return err;

	cur_block = node->u.file.datablocks;
	
//dprintf("cfs_read_data: file %s, pos %Ld, len %ld\n", node->name, pos, request_len);

	while(request_len > 0) {
		off_t readpos;
		size_t readlen;
//dprintf("cfs_read: ... pos %Ld, len %ld, fpos %Ld\n", pos, request_len, cur_filepos);
		while(cur_block && cur_filepos + cur_block->size <= pos) {
//dprintf("cfs_read: skip block size %ld\n", cur_block->size);
			cur_filepos += cur_block->size;
			cur_block = cur_block->next;
		}
		if(cur_block == NULL) {
			err = B_NO_ERROR;
			goto err;
		}
		if(request_len > cur_block->size - (pos-cur_filepos))
			readlen = cur_block->size - (pos-cur_filepos);
		else
			readlen = request_len;
//dprintf("cfs_read: current block size %ld at %Ld\n", cur_block->size, cur_block->data_pos);
		if(cur_block->flags) {
			err = cfs_read_compressed_block(fsinfo, cur_block, pos-cur_filepos,
			                                cur_buffer, readlen);
			if(err != B_NO_ERROR) {
				goto err;
			}
		}
		else {
			readpos = cur_block->data_pos + pos-cur_filepos;
			err = cfs_read_disk_etc(fsinfo, readpos, cur_buffer, &readlen, 1);
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_read_data: could not read raw data\n"));
				goto err;
			}
		}
		*len += readlen;
		cur_buffer += readlen;
		request_len -= readlen;
		pos += readlen;
	}
	return B_NO_ERROR;
err:
	return err;
}

static status_t
cfs_add_compressed_block(cfs_info *fsinfo, cfs_transaction *transaction,
                         cfs_node *node, cfs_vblock_list **last_vblock,
                         off_t size)
{
	status_t err;
	cfs_data_block new_data_block;
	off_t location;
	cfs_vblock_list *new_vblock;
	cfs_entry_info entry;
	bool entry_loaded = false;
	
	PRINT_FLOW(("cfs_add_compressed_block entry: size %Ld\n", size));
	
	{
		/* allocate (4/7) + new block (2) + prev block (2) */
		int max_transaction_size = ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version)+2+2;
		
		if(fsinfo->fs_version > 1) {
			/* + update size in ent (2) */
			max_transaction_size += 2;
		}
		
		err = cfs_sub_transaction(fsinfo, transaction, max_transaction_size);
		if(err != B_NO_ERROR)
			goto err1;
	}		

	err = allocate_disk_space(fsinfo, transaction, &location,
	                          sizeof(new_data_block), METADATA);
	if(err != B_NO_ERROR)
		goto err1;

	new_vblock = malloc(sizeof(*new_vblock));
	if(new_vblock == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}
	new_vblock->next = NULL;
	new_vblock->flags = 1;
	new_vblock->data_pos = 0;
	new_vblock->size = size;
	new_vblock->compressed_size = 0;
	new_vblock->location = location;

	new_data_block.next = 0;
	new_data_block.data = 0;
	new_data_block.disk_size = 0;
	new_data_block.raw_size = size;

	err = cfs_write_disk(fsinfo, transaction, location, &new_data_block,
	                     sizeof(new_data_block));
	if(err != B_NO_ERROR)
		goto err3;

	if(*last_vblock) {
		cfs_data_block last_block;
		off_t last_block_location = (*last_vblock)->location;

		err = cfs_read_disk(fsinfo, last_block_location, &last_block,
		                    sizeof(last_block));
		if(err != B_NO_ERROR)
			goto err3;

		if(last_block.next != 0) {
			err = B_IO_ERROR;
			PRINT_ERROR(("cfs_expand_compressed_file: last_block is wrong\n"));
			goto err3;
		}
		last_block.next = location;

		err = cfs_write_disk(fsinfo, transaction, last_block_location,
		                     &last_block, sizeof(last_block));
		if(err != B_NO_ERROR)
			goto err3;
	}
	else {
		err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
		if(err != B_NO_ERROR)
			goto err3;
		entry_loaded = true;

		if(entry.data != 0) {
			err = B_IO_ERROR;
			PRINT_ERROR(("cfs_expand_compressed_file: entry.data is wrong\n"));
			goto err3;
		}

		entry.data = location;
	}
	
	if(fsinfo->fs_version > 1) {
		// update the logical size field in the entry
		if(entry_loaded == false) {		
			err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
			if(err != B_NO_ERROR)
				goto err4;
			entry_loaded = true;
		}
		
		entry.logical_size += size;
	}

	// write out the entry if it's been loaded for whatever reason
	if(entry_loaded == true) {
		err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
		if(err != B_NO_ERROR)
			goto err4;		
	}

	// update the memory copy of the node's size after all possible failure points
	node->logical_size += size;

	// add the new_vblock to the memory copy of the blocklist after
	// all possible failure points
	if(*last_vblock) {
		(*last_vblock)->next = new_vblock;
	} else {
		node->u.file.datablocks = new_vblock;
	}						
		
	*last_vblock = new_vblock;
	return B_NO_ERROR;
	
err4:
	// undo the damage done when updating the on-disk version of the last_vblock
	// when making the next pointer point to the newly allocated data.
	// make the next pointer point back to null
	if(*last_vblock) {
		status_t tmp_err;
		cfs_data_block last_block;
		off_t last_block_location = (*last_vblock)->location;
		
		tmp_err = cfs_read_disk(fsinfo, last_block_location, &last_block,
		                    sizeof(last_block));
		if(tmp_err != B_NO_ERROR) {
			// whoa! we can't fix this one
			goto err3;
		}

		// it should have been 0 before, so set it back
		last_block.next = 0;

		tmp_err = cfs_write_disk(fsinfo, transaction, last_block_location,
		                     &last_block, sizeof(last_block));
	}	
err3:
	if(new_vblock != NULL) free(new_vblock);
err2:
	free_disk_space(fsinfo, transaction, location, sizeof(new_data_block));
err1:
	return err;
}

// returns number of runs allocated. zero is legal too
static int
cfs_expand_compressed_file(cfs_info *fsinfo, cfs_transaction *transaction,
                           cfs_node *node, cfs_vblock_list *last_vblock,
                           off_t size)
{
	status_t err;
	cfs_vblock_list *prev_vblock;
	off_t remsize = size;
	int blocks_allocated = 0;
	
	prev_vblock = last_vblock;
	while(remsize > 0) {
		off_t block_size = min(cfs_compressed_block_size, remsize);

		err = cfs_add_compressed_block(fsinfo, transaction, node, &prev_vblock,
		                               block_size);
		if(err != B_NO_ERROR)
			return err;
		remsize -= block_size;
		blocks_allocated++;
	}
	return blocks_allocated;
}

// returns number of runs allocated. zero is legal too
static int
cfs_expand_uncompressed_file(cfs_info *fsinfo, cfs_transaction *transaction,
                             cfs_node *node, cfs_vblock_list *last_vblock,
                             off_t size)
{
	int err;
	off_t d_location = 0;
	off_t m_location = 0;
	off_t bytes_added = 0;
	cfs_data_block added_data_block;
	cfs_vblock_list *new_vblock = NULL;
	off_t bytes_to_add_to_last_block = 0;
	int blocks_alloced = 0;
	cfs_entry_info entry;
	bool entry_loaded = false;

	if(size <= 0) {
		PRINT_INTERNAL_ERROR(("cfs_expand_uncompressed_file: size %Ld <= 0\n", size));
		return 0;
	}

	{
		/* allocate space (4/7) + new datablock (2) + prev datablock(2) */
		int max_transaction_size = ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 2 + 2;
		
		if(fsinfo->fs_version > 1) {
			/* + allocate space (4/7) + update entry (2) */
			max_transaction_size += ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 2;
		}
		
		err = cfs_sub_transaction(fsinfo, transaction, max_transaction_size);
		if(err < B_NO_ERROR)
			return err; 
	}		

	// load & write this now to make sure the blocks are locked in the cache
	if(fsinfo->fs_version > 1) {
		err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
		if(err < B_NO_ERROR)
			goto err;
		entry_loaded = true;
		err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
		if(err < B_NO_ERROR)
			goto err;
	}
	
//	dprintf("cfs_expand_uncompressed_file: size = %Ld\n", size);
	// see if we can use some of the last block before expanding it
	if(last_vblock && ((cfs_align_block(last_vblock->size) - last_vblock->size) > 0)) {
		bytes_to_add_to_last_block = min(size, cfs_align_block(last_vblock->size) - last_vblock->size);
		
		size -= bytes_to_add_to_last_block;
		bytes_added += bytes_to_add_to_last_block;
		if(size == 0) {
			// we need to write the last data vblock out now
			cfs_data_block last_block;
			off_t last_block_location = last_vblock->location;
			err = cfs_read_disk(fsinfo, last_block_location, &last_block,
			                    sizeof(last_block));
			if(err < B_NO_ERROR)
				goto err;
	
			last_block.disk_size += bytes_to_add_to_last_block;
	
			err = cfs_write_disk(fsinfo, transaction, last_block_location,
			                     &last_block, sizeof(last_block));
			if(err < B_NO_ERROR)
				goto err;
			last_vblock->size += bytes_to_add_to_last_block;
			err = B_NO_ERROR;
			goto out;		
		}
	} 

//	dprintf("done, size = %Ld, bytes_to_add_to_last_block = %Ld\n", size, bytes_to_add_to_last_block);

	// Allocate data block first
	err = allocate_disk_space(fsinfo, transaction, &d_location,
	                          cfs_align_block(size), FILEDATA);
	if(err < B_NO_ERROR)
		goto err;
	
//	dprintf("allocated %Ld bytes at location 0x%Lx\n", size, d_location);
	
	// see if the new block can be coalesced
	if(last_vblock && ((last_vblock->data_pos + last_vblock->size + bytes_to_add_to_last_block) == d_location)) {
		cfs_data_block last_block;
		off_t last_block_location = last_vblock->location;
		err = cfs_read_disk(fsinfo, last_block_location, &last_block,
		                    sizeof(last_block));
		if(err < B_NO_ERROR)
			goto err2;

		last_block.disk_size += bytes_to_add_to_last_block + size;

		err = cfs_write_disk(fsinfo, transaction, last_block_location,
		                     &last_block, sizeof(last_block));
		if(err < B_NO_ERROR)
			goto err2;
		last_vblock->size += bytes_to_add_to_last_block + size;
		bytes_added += size;
	}
	else {

		new_vblock = malloc(sizeof(*new_vblock));
		if(new_vblock == NULL) {
			err = B_NO_MEMORY;
			goto err;
		}
		err = allocate_disk_space(fsinfo, transaction, &m_location,
		                          cfs_align_block(sizeof(cfs_data_block)), METADATA);
		if(err < B_NO_ERROR)
			goto err1;

		added_data_block.next = 0;
		added_data_block.data = d_location;
		added_data_block.disk_size = size;
		added_data_block.raw_size = 0;

		new_vblock->next = NULL;
		new_vblock->flags = 0;
		new_vblock->data_pos = d_location;
		new_vblock->size = size;
		new_vblock->location = m_location;

		// write the metadata out
		err = cfs_write_disk(fsinfo, transaction, m_location, &added_data_block,
		                     sizeof(added_data_block));
		if(err < B_NO_ERROR)
			goto err2;

		if(last_vblock) {
			cfs_data_block last_block;
			off_t last_block_location = last_vblock->location;
			err = cfs_read_disk(fsinfo, last_block_location, &last_block,
			                    sizeof(last_block));
			if(err < B_NO_ERROR)
				goto err2;
	
			if(last_block.next != 0) {
				err = B_IO_ERROR;
				PRINT_ERROR(("cfs_expand_uncompressed_file: entry.data is wrong\n"));
				goto err2;
			}
			last_block.disk_size += bytes_to_add_to_last_block;			
			last_block.next = m_location;
	
			err = cfs_write_disk(fsinfo, transaction, last_block_location,
			                     &last_block, sizeof(last_block));
			if(err < B_NO_ERROR)
				goto err2;
			last_vblock->size += bytes_to_add_to_last_block;
			last_vblock->next = new_vblock;
		}
		else {	
			if(entry_loaded == false) {
				err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
				if(err < B_NO_ERROR)
					goto err2;
				entry_loaded = true;
			}
			if(entry.data != 0) {
				err = B_IO_ERROR;
				dprintf("cfs_write: entry.data is wrong\n");
				goto err2;
			}
			entry.data = m_location;
		}
		blocks_alloced++;
		bytes_added += size;	
	}
out:
	// deal with loading and writing or just writing the entry out if we need to
	if(fsinfo->fs_version > 1) {
		// update the entry
		if(entry_loaded == false) {
			err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
			if(err < B_NO_ERROR)
				goto err2;
			entry_loaded = true;
		}	
		entry.logical_size += bytes_added;
	}

	// write the entry out if it's loaded for whatever reason
	if(entry_loaded == true) {
		err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
		if(err < B_NO_ERROR)
			goto err2;
	}
	PRINT_FLOW(("cfs_expand_uncompressed_file: file expanded\n"));

	if(last_vblock == NULL) {
		node->u.file.datablocks = new_vblock;
		node->u.file.is_blocklist_loaded = true;
	}		

	// delay updating this field until we know the entry was written out
	node->logical_size += bytes_added;

	return blocks_alloced;

err2:
	if(d_location != 0) 
		free_disk_space(fsinfo, transaction, d_location,
		                cfs_align_block(size));
	if(m_location != 0) 
		free_disk_space(fsinfo, transaction, m_location,
		                cfs_align_block(sizeof(cfs_data_block)));
err1:
	if(new_vblock != NULL) free(new_vblock);
err:
	return err;
}

static status_t
cfs_expand_file(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *node,
                cfs_vblock_list *last_vblock, off_t size)
{
	if(node->cfs_flags & CFS_FLAG_COMPRESS_FILE)
		return cfs_expand_compressed_file(fsinfo, transaction, node,
		                                  last_vblock, size);
	else
		return cfs_expand_uncompressed_file(fsinfo, transaction, node,
		                                    last_vblock, size);
}

static status_t
cfs_truncate_file(cfs_info *fsinfo, cfs_transaction *transaction,
                  cfs_node *node, cfs_vblock_list *last_vblock, off_t size)
{
	status_t err = B_NO_ERROR;
	cfs_vblock_list *vblock;
	cfs_entry_info entry;
	bool entry_loaded = false;

	if(last_vblock == NULL) {
		vblock = node->u.file.datablocks;
	}
	else {
		vblock = last_vblock->next;
	}
	
	if(size > 0) {
		cfs_vblock_list vblock_update = *vblock;
		off_t old_size = vblock->size;
		if(vblock->flags && vblock->data_pos != 0) {
			dprintf("cfs_truncate_file: can't truncate compressed block at %Ld "
			        "(%Ld to %Ld)\n", vblock->location, vblock->size,
			        size);

			dprintf("cfs_truncate_file: compressed block size: %ld\n",
			        vblock->compressed_size);
			return B_NOT_ALLOWED;
		}

		{
			/* update datablock (2) + free (4/7) */
			int max_transaction_size = 2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version);
			
			if(fsinfo->fs_version > 1) {
				/* + update entry (2) */
				max_transaction_size += 2;
			}
			
			err = cfs_sub_transaction(fsinfo, transaction, max_transaction_size);
			if(err < B_NO_ERROR)
				return err; 
		}			

		vblock_update.size = size;
		err = cfs_update_datablock(fsinfo, transaction, vblock,
		                           &vblock_update);
		if(err != B_NO_ERROR)
			return err;
		if(vblock->data_pos != 0) {
			err = free_disk_space(fsinfo, transaction, vblock->data_pos +
			                      cfs_align_block(size),
			                      cfs_align_block(old_size) -
			                      cfs_align_block(size));
			if(err != B_NO_ERROR)
				return err;
		}
		last_vblock = vblock;
		vblock = vblock->next;
		
		// update the entry's logical size field
		if(fsinfo->fs_version > 1) {
			err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
			if(err < B_NO_ERROR)
				return err;
			entry_loaded = true;
			
			entry.logical_size -= old_size - size;
			
			err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
			if(err < B_NO_ERROR)
				return err;
				
		}
		node->logical_size -= old_size - size;					
	}
	
	while(vblock != NULL) {
		off_t disk_size;
		off_t logical_size = vblock->size;
		if(vblock->flags) {
			disk_size = vblock->compressed_size;
		}
		else {
			disk_size = vblock->size;
		}
		
		{
			/* update prev datablock / entry (2) + free info (4/7) + free data (4/7) */
			int max_transaction_size = 2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version) * 2;
			
			if(fsinfo->fs_version > 1) {
				/* + update entry (2) */
				max_transaction_size += 2;
			}
			
			err = cfs_sub_transaction(fsinfo, transaction, max_transaction_size);
			if(err < B_NO_ERROR)
				return err; 
		}		

		if(last_vblock) {
			cfs_vblock_list last_vblock_update = *last_vblock;
			last_vblock_update.next = vblock->next;
			err = cfs_update_datablock(fsinfo, transaction, last_vblock,
			                           &last_vblock_update);
			if(err != B_NO_ERROR)
				return err;
		}
		else {
			if(entry_loaded == false) {
				err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
				if(err != B_NO_ERROR)
					return err;
				entry_loaded = true;
			}
			if(entry.data != vblock->location) {
				err = B_IO_ERROR;
				PRINT_ERROR(("cfs_truncate_file: entry.data (%Ld) is wrong, "
				             "should be %Ld\n", entry.data, vblock->location));
				return err;
			}
			if(vblock->next != NULL)
				entry.data = vblock->next->location;
			else
				entry.data = 0;
			err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
			if(err != B_NO_ERROR)
				return err;
			node->u.file.datablocks = vblock->next;
		}

		err = free_disk_space(fsinfo, transaction, vblock->location,
		                      sizeof(cfs_data_block));
		if(err != B_NO_ERROR)
			return err;

		if(vblock->data_pos > 0) {
			err = free_disk_space(fsinfo, transaction, vblock->data_pos,
			                      cfs_align_block(disk_size));
			if(err != B_NO_ERROR)
				return err;
		}		
		{
			cfs_vblock_list *tmp_vblock = vblock;
			vblock = vblock->next;
			free(tmp_vblock);
		}
		
		// update the entry's logical size field
		if(fsinfo->fs_version > 1) {
			if(entry_loaded == false) {
				err = cfs_read_entry_info(fsinfo, node->vnid, &entry);
				if(err < B_NO_ERROR)
					return err;
				entry_loaded = true;
			}

			entry.logical_size -= logical_size;
			
			err = cfs_write_entry_info(fsinfo, transaction, node->vnid, &entry);
			if(err < B_NO_ERROR)
				return err;		

		}
		node->logical_size -= logical_size;
	}
	return err;
}

status_t
cfs_set_file_size(cfs_info *fsinfo, cfs_transaction *transaction,
                  cfs_node *node, off_t size)
{
	status_t err;
	off_t cur_filepos;
	cfs_vblock_list *cur_block;
	cfs_vblock_list *last_vblock = NULL;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(!S_ISREG(node->flags)) {
		return B_BAD_VALUE;
	}

	err = cfs_load_blocklist(fsinfo, node);
	if(err != B_NO_ERROR)
		return err;
	
	cur_filepos = 0;
	cur_block = node->u.file.datablocks;
	while(cur_block && cur_filepos + cur_block->size <= size) {
		//dprintf("cfs_set_file_size: skip block size %ld\n", cur_block->size);
		cur_filepos += cur_block->size;
		last_vblock = cur_block;
		cur_block = cur_block->next;
	}
	if(cur_block == NULL && size == cur_filepos) {
		PRINT_FLOW(("cfs_set_file_size: file was already of size %Ld\n", size));
		return B_OK;
	}
	if(cur_block == NULL) {
		PRINT_FLOW(("cfs_set_file_size: file was only %Ld bytes, expand file\n",
		            cur_filepos));
		return cfs_expand_file(fsinfo, transaction, node, last_vblock,
		                       size - cur_filepos);
	}
	else {
		PRINT_FLOW(("cfs_set_file_size: file more %Ld bytes, truncate file\n",
		            size));
		return cfs_truncate_file(fsinfo, transaction, node, last_vblock,
		                         size - cur_filepos);
	}
	return B_NO_ERROR;
}

static status_t
cfs_write_compressed_block(cfs_info *fsinfo, cfs_transaction *transaction,
                           cfs_vblock_list *block, size_t offset,
                           const void *buffer, size_t len)
{
	status_t err;
	uint8 *rbuffer = NULL;
	uint8 *cbuffer = NULL;
	size_t compressed_size;

	if(offset > block->size || offset+len > block->size) {
		PRINT_INTERNAL_ERROR(("cfs_write_compressed_block: tried to write "
		                      "%ld-%ld to block of size %Ld\n", offset,
		                      offset+len-1, block->size));
		return B_ERROR;
	}

	cbuffer = malloc(block->size);
	if(cbuffer == NULL)
		return B_NO_MEMORY;
	
	if(len < block->size) {
		PRINT_FLOW(("cfs_write_compressed_block: "
		            "offset %ld, len %ld\n", offset, len));
		
		rbuffer = malloc(block->size);
		if(rbuffer == NULL) {
			err = B_NO_MEMORY;
			goto err1;
		}
		err = cfs_read_compressed_block(fsinfo, block, 0, rbuffer, block->size);
		if(err != B_NO_ERROR) {
			goto err1;
		}
		memcpy(rbuffer + offset, buffer, len);
		cfs_compress(rbuffer, block->size, cbuffer, &compressed_size);
	}
	else {
		cfs_compress(buffer, block->size, cbuffer, &compressed_size);
	}
	if(compressed_size <= cfs_align_block(block->compressed_size)) {
		size_t old_compressed_size = block->compressed_size;
		cfs_vblock_list new_block = *block;
		new_block.compressed_size = compressed_size;

		err = cfs_write_disk_cached(fsinfo, transaction, block->data_pos,
		                            cbuffer, compressed_size);
		if(err != B_NO_ERROR) {
			goto err1;
		}
		if(compressed_size != old_compressed_size) {
			/* data info (2) + free (4/7) */
			err = cfs_sub_transaction(fsinfo, transaction, 2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version));
			if(err != B_NO_ERROR)
				goto err1;
			err = cfs_update_datablock(fsinfo, transaction, block, &new_block);
			if(err != B_NO_ERROR) {
				goto err1;
			}
			err = free_disk_space(fsinfo, transaction,
			                      block->data_pos+cfs_align_block(compressed_size),
			                      cfs_align_block(old_compressed_size) -
			                      cfs_align_block(compressed_size));
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_write_compressed_block: could not free disk "
				             "space\n"));
			}
		}
	}
	else {
		size_t old_compressed_size = block->compressed_size;
		size_t old_data_pos = block->data_pos;
		cfs_vblock_list new_block = *block;

		new_block.compressed_size = compressed_size;
		
		/* allocate (4/7) + data info (2) + free (4/7) */
		err = cfs_sub_transaction(fsinfo, transaction, ALLOCATE_MAX_LOG_SIZE(fsinfo->fs_version) + 2 + FREE_MAX_LOG_SIZE(fsinfo->fs_version));
		if(err != B_NO_ERROR)
			goto err1;

		err = allocate_disk_space(fsinfo, transaction, &new_block.data_pos,
		                          cfs_align_block(compressed_size), FILEDATA);
		if(err != B_NO_ERROR) {
			goto err1;
		}

		err = cfs_update_datablock(fsinfo, transaction, block, &new_block);
		if(err != B_NO_ERROR) {
			goto err1;
		}
		if(old_data_pos != 0) {
			err = free_disk_space(fsinfo, transaction, old_data_pos,
			                      cfs_align_block(old_compressed_size));
			if(err != B_NO_ERROR) {
				PRINT_ERROR(("cfs_write_compressed_block: could not free disk "
				             "space\n"));
			}
		}
		err = cfs_write_disk_cached(fsinfo, transaction, new_block.data_pos,
		                            cbuffer, compressed_size);
		if(err != B_NO_ERROR) {
			goto err1;
		}
	}

	// update the copy in the decompression buffer, if there is one
	LOCK(fsinfo->decompression_buf_lock);
	if(fsinfo->last_decompression_offset == block->data_pos) {
		memcpy(fsinfo->decompression_buf + offset, buffer, len);
	}
	UNLOCK(fsinfo->decompression_buf_lock);

	err = B_NO_ERROR;
err1:
	if(rbuffer != NULL) free(rbuffer);
	if(cbuffer != NULL) free(cbuffer);
	return err;
}

status_t
cfs_write_data(cfs_info *fsinfo, cfs_transaction *transaction, cfs_node *node,
               off_t pos, const void *buf, size_t *len, bool append)
{
	status_t err;
	off_t cur_filepos = 0;
	cfs_vblock_list *cur_block;
	off_t start_block_filepos = 0;
	cfs_vblock_list *start_block;
	cfs_vblock_list *last_vblock = NULL;
	size_t request_len = *len;
	const uint8 *cur_buffer = buf;

	*len = 0;
	
	//dprintf("cfs_write: file %s, pos %Ld, len %ld\n", node->name, pos, request_len);
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(!S_ISREG(node->flags)) {
		return B_BAD_VALUE;
	}

	err = cfs_load_blocklist(fsinfo, node);
	if(err != B_NO_ERROR)
		return err;

	// start walking through the list to find the vblock the write will start in
	cur_filepos = 0;
	cur_block = node->u.file.datablocks;
	while(cur_block && (cur_filepos + cur_block->size <= pos || append)) {
		//dprintf("cfs_write: skip block size %ld\n", cur_block->size);
		cur_filepos += cur_block->size;
		last_vblock = cur_block;
		cur_block = cur_block->next;
	}
	// at this point:
	//  cur_block = pointer to block the write should start in (or NULL)
	//  cur_filepos = logical file pos at the start of the block
	//  last_vblock = the block before cur_block
	
	// continue walking to find the vblock the write will stop in
	start_block = cur_block;
	start_block_filepos = cur_filepos;
	while(cur_block &&
	      (cur_filepos + cur_block->size < pos + request_len || append)) {
		//dprintf("cfs_write: skip block size %ld\n", cur_block->size);
		cur_filepos += cur_block->size;
		last_vblock = cur_block;
		cur_block = cur_block->next;
	}
	// at this point:
	//  cur_block = pointer to block the write will end in (or NULL)
	//  cur_filepos = logical file pos at the start of the block
	//  last_vblock = the block before cur_block
	
	if(cur_block == NULL) {
		off_t last_vblock_size = last_vblock ? last_vblock->size : 0;
		
		err = cfs_expand_file(fsinfo, transaction, node, last_vblock,
		                      append ? request_len
		                             : pos + request_len - cur_filepos);
		if(err < B_NO_ERROR)
			return err;

		if(start_block == NULL) {
			if(last_vblock) {
				if(last_vblock->size > last_vblock_size) {
					// the last vblock changed size, so it was added on to
					start_block = last_vblock;
					start_block_filepos -= last_vblock_size;					
				} else if(last_vblock->next) {	
					// the expansion must have built a new vblock on the end of the list
					start_block = last_vblock->next;
				} else {
					PRINT_INTERNAL_ERROR(("cfs_write: cfs_expand_file didn't do anything but said it did\n"));
					return B_IO_ERROR;
				}
			} else {
				// start_block == NULL && last_vblock == NULL
				// the expansion had to have built a new vblock at the head of the list
				start_block = node->u.file.datablocks;
			}
		}
		
		if(append) {
			pos = cur_filepos;
		} else {
			cur_block = start_block;
			cur_filepos = start_block_filepos;
			while(cur_block && cur_filepos + cur_block->size <= pos) {
				//dprintf("cfs_write: skip block size %ld\n", cur_block->size);
				cur_filepos += cur_block->size;
				last_vblock = cur_block;
				cur_block = cur_block->next;
			}
			start_block = cur_block;
			start_block_filepos = cur_filepos;
		}
	}
	cur_block = start_block;
	cur_filepos = start_block_filepos;

	while(request_len > 0) {
		off_t writepos;
		size_t writelen;
		//dprintf("cfs_write: ... pos %Ld, len %ld, fpos %Ld\n", pos, request_len, cur_filepos);
		if(cur_block == NULL) {
			PRINT_INTERNAL_ERROR(("cfs_write_data: write past end of file\n"));
			return B_ERROR;
		}
		if(request_len > cur_block->size - (pos-cur_filepos))
			writelen = cur_block->size - (pos-cur_filepos);
		else
			writelen = request_len;

		//dprintf("cfs_write: current block size %ld at %Ld\n", cur_block->size, cur_block->data_pos);
		if(cur_block->flags) {
			PRINT_FLOW(("cfs_write: compressed block at %Ld\n",
			            cur_block->data_pos));
			err = cfs_write_compressed_block(fsinfo, transaction, cur_block,
			                                 pos-cur_filepos, cur_buffer,
			                                 writelen);
			if(err != B_NO_ERROR)
				return err;
		}
		else {
			writepos = cur_block->data_pos + pos - cur_filepos;
			//dprintf("cfs_write: going to write: pos %Ld, len %ld\n", writepos, writelen);
			err = cfs_write_disk_cached(fsinfo, transaction, writepos,
			                            cur_buffer, writelen);
			if(err != B_NO_ERROR)
				return err;
		}
		cur_filepos += cur_block->size;
		last_vblock = cur_block;
		cur_block = cur_block->next;

		*len += writelen;
		cur_buffer += writelen;
		request_len -= writelen;
		pos += writelen;
		if(pos != cur_filepos && request_len > 0) {
			PRINT_ERROR(("cfs_write_data: pos %Ld after write does not match "
			             "next block pos %Ld\n", pos, cur_filepos));
			return B_ERROR;
		}
	}
	return B_NO_ERROR;
}
