#include "ntfs.h"
#include "attributes.h"
#include "extent.h"
#include "io.h"
#include "compress.h"
#include "compression_buf.h"

static status_t ntfs_read_pos(ntfs_fs_struct *ntfs, off_t dev_offset, size_t length, char *buf)
{
	uint64 block = dev_offset / ntfs->cache_block_size;
	uint32 offset = dev_offset % ntfs->cache_block_size;
	char *buf_pos = buf;
	
	if(offset > 0) {
		char *ptr;
		size_t toread = min(length, ntfs->cache_block_size - offset);
		
		if(block >= ntfs->num_cache_blocks) {
			ERRPRINT(("ntfs_read_pos: cache block # is too big! (%Ld)\n", block));
			return EIO;
		}
		
		ptr = get_block(ntfs->fd, block, ntfs->cache_block_size);
		if(!ptr) return EIO;
		
		memcpy(buf_pos, ptr + offset, toread);
		
		release_block(ntfs->fd, block);
		block++;
		buf_pos += toread;
		length -= toread;
	}
	
	if(length >= ntfs->cache_block_size) {
		status_t err;

		if((block + length / ntfs->cache_block_size) >= ntfs->num_cache_blocks) {
			ERRPRINT(("ntfs_read_pos: cache block # is too big! (%Ld)\n", block + length / ntfs->cache_block_size));
			return EIO;
		}

		err = cached_read(ntfs->fd, block, buf_pos, length / ntfs->cache_block_size, ntfs->cache_block_size);
		if(err < B_NO_ERROR) return err;

		block += length / ntfs->cache_block_size;
		buf_pos += (length / ntfs->cache_block_size) * ntfs->cache_block_size;
		length -= (length / ntfs->cache_block_size) * ntfs->cache_block_size;
	}

	if(length > 0) {
		void *ptr;
		
		if(block >= ntfs->num_cache_blocks) {
			ERRPRINT(("ntfs_read_pos: cache block # is too big! (%Ld)\n", block));
			return EIO;
		}

		ptr = get_block(ntfs->fd, block, ntfs->cache_block_size);
		if(!ptr) return EIO;
		
		memcpy(buf_pos, ptr, length);
		
		release_block(ntfs->fd, block);
	}

	return B_OK;
}

static status_t ntfs_release_decompressed_compression_group_ptr(ntfs_fs_struct *ntfs, decompression_buf *buf)
{	
	return ntfs_return_dbuf(ntfs, buf->index);
}

// This function gets a pointer to a decompression run, ie. 16 blocks of decompressed data.
// It gets a decompression buffer, fills it (if necessary), and passes the pointer to this buffer back.
static status_t ntfs_get_decompressed_compression_group_buffer(decompression_buf **buf, ntfs_fs_struct *ntfs, extent_storage *extent, uint64 compression_group)
{
	int dbuf_index;	
	status_t err;
	int i;

//	dprintf("ntfs_get_decompressed_compression_group_buffer wants group %Ld for vnid %Ld.\n", compression_group, extent->vnid);

	dbuf_index = ntfs_find_old_dbuf_or_get_new(ntfs, extent->vnid, extent->att_id, compression_group);
	if((dbuf_index < 0) || (dbuf_index >= DECOMPRESSION_BUFFERS)) {
		err = EIO;
		goto error;
	}
	
	// Lock the buffer
	LOCK(ntfs->dbuf[dbuf_index].buf_lock);
	
	// Check the initialized flag
	if(!ntfs->dbuf[dbuf_index].initialized) {
		comp_file_extent ce;

		// We need to fill in the buffer
		if(!ntfs->dbuf[dbuf_index].buf) {
			// Lets allocate some space
			ntfs->dbuf[dbuf_index].buf = ntfs_malloc(ntfs->cluster_size * 16);
			if(!ntfs->dbuf[dbuf_index].buf) {
				ERRPRINT(("ntfs_get_decompressed_compression_group_buffer failed to allocate space for decompression buffer\n"));
				UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);
				err = ENOMEM;
				goto error1;
			}
		}
	
		memset(ntfs->dbuf[dbuf_index].buf, 0, ntfs->cluster_size * 16);
		
		if(!ntfs->dbuf[dbuf_index].compressed_buf) {
			ntfs->dbuf[dbuf_index].compressed_buf = ntfs_malloc(ntfs->cluster_size * 16);
			if(!ntfs->dbuf[dbuf_index].compressed_buf) {
				ERRPRINT(("ntfs_get_decompressed_compression_group_buffer failed to allocate space for decompression buffer\n"));
				UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);
				err = ENOMEM;
				goto error1;
			}
			memset(ntfs->dbuf[dbuf_index].compressed_buf, 0, ntfs->cluster_size * 16);			
		}					

		memset(ntfs->dbuf[dbuf_index].compressed_buf, 0, ntfs->cluster_size * 16);
	
		// Start filling the buffer
		err = ntfs_get_compression_group_extent(extent, compression_group, &ce);
		if(err < B_NO_ERROR) {
			UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);
			goto error1;
		}
		
		if((ce.flags & EXTENT_FLAG_COMPRESSED)) {
			// read in all of the raw blocks in the compression group
			for(i=0; i<min(ce.num_blocks, 16); i++) {
				err = ntfs_read_pos(ntfs, ce.blocks[i] * ntfs->cluster_size,
					ntfs->cluster_size,
					(char *)ntfs->dbuf[dbuf_index].compressed_buf + ntfs->cluster_size * i);
				if(err < B_NO_ERROR) {
					UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);					
					goto error1;
				}
			}
			
			// Decompress the blocks
			{
				uint32 inpos = 0;
				uint32 outpos = 0;
				uint32 indelta, outdelta;
				
//				dprintf("decompressing blocks:\n");
					
				while(outpos < ntfs->cluster_size * 16) {
					ntfs_decompress_block((char *)ntfs->dbuf[dbuf_index].compressed_buf + inpos,
						(char *)ntfs->dbuf[dbuf_index].buf + outpos,
						&indelta, &outdelta,
						min(4096, ntfs->cluster_size * 16 - outpos));
						
//					dprintf("indelta = %ld, outdelta = %ld\n", indelta, outdelta);
				
					if(outdelta == 0) break;
				
					inpos += indelta;
					outpos += outdelta;
				}
			}
		} else if(!(ce.flags & EXTENT_FLAG_SPARSE)) {
			// The compression group isn't compressed, so just copy
			for(i=0; i<ce.num_blocks; i++) {
				err = ntfs_read_pos(ntfs, ce.blocks[i] * ntfs->cluster_size,
					ntfs->cluster_size,
					(char *)ntfs->dbuf[dbuf_index].buf + ntfs->cluster_size * i);
				if(err < B_NO_ERROR) {
					UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);					
					goto error1;
				}
			}
		}
		// If the flag was SPARSE, the memset from above will fill the buffer with zeros
	}

	ntfs->dbuf[dbuf_index].initialized = true;

	// Unlock it
	UNLOCK(ntfs->dbuf[dbuf_index].buf_lock);

	*buf = &ntfs->dbuf[dbuf_index];

	return B_NO_ERROR;

error1:
	ntfs_release_decompressed_compression_group_ptr(ntfs, &ntfs->dbuf[dbuf_index]);	
error:
	ERRPRINT(("ntfs_get_decompressed_compression_group_buffer exiting with error code %d\n",(int)err));
	return err;	
}

// This function simply fills the buffer from an extent.
status_t ntfs_read_extent(ntfs_fs_struct *ntfs, extent_storage *extent, void *buf, off_t pos, size_t read_len, size_t *bytes_read)
{
	char *buf_pos = (char *)buf;
	status_t err;
    off_t offset=0;
	size_t length_to_read=0;
	size_t bytes_left = read_len;

	DEBUGPRINT(EXTENT, 5, ("ntfs_read_extent entry. pos %Ld read_len %ld.\n", pos, read_len));

	*bytes_read = 0;

	if(!extent->compressed) {
		uint64 block;

		// Calculate the starting block into the file to read.
		block = pos/ntfs->cluster_size;
		// Calculate the offset into the block to read
		offset = pos % ntfs->cluster_size;

		DEBUGPRINT(EXTENT, 6, ("\tstarting extent block %Ld, offset %Ld.\n", block, offset));
				
		while(bytes_left > 0) {	
			uint64 fs_block;
			// There are a couple of situations here:
			//  1. This block is entirely within the initialized data section (probably)
			//  2. This block is at\crosses the boundary
			//  3. This block is in the uninitialized data section				

			length_to_read = min(bytes_left, ntfs->cluster_size - offset);
			
			// Lets handle case 1 & 2 here, we'll 'fix' for case 2 in a minute
			if(block <= extent->uninit_data_start_block) {
				// It's into a block
				err = ntfs_get_block_from_extent(extent, &fs_block, block);
				if(err < B_NO_ERROR) return err;

				err = ntfs_read_pos(ntfs, fs_block * ntfs->cluster_size + offset,
					length_to_read, buf_pos);
				if(err < B_NO_ERROR) return err;
			}
			
			// Handle the case 2 fix
			// We may have to 'erase' some of the data we just read in with zeros
			if(block == extent->uninit_data_start_block) {
				int32 zero_start_in_buf;
				int32 zero_len = length_to_read - zero_start_in_buf;

				// start by calculating where the zero's should go if we were to put them
				zero_start_in_buf = extent->uninit_data_start_offset - offset;
				if(zero_start_in_buf < 0) zero_start_in_buf = 0;

				// figure the length we'll have to do
				zero_len = length_to_read - zero_start_in_buf;

				// If zero_len > 0, we need to erase some of the data we just read in
				if(zero_len > 0) {
					memset(buf_pos + zero_start_in_buf, 0, zero_len);
				}
			}
			
			// case 3, just write some zeros
			if(block > extent->uninit_data_start_block) {
				memset(buf_pos, 0, length_to_read);
			}
				
			bytes_left -= length_to_read;
			(*bytes_read) += length_to_read;
			buf_pos += length_to_read;
			offset = 0;
			block++;
		}
	} else {
		// compressed read
		decompression_buf *db;

		// Calculate the starting group into the file to read.
		uint64 group = pos/(ntfs->cluster_size * 16);
		// Calculate the offset into the group to read
		offset = pos % (ntfs->cluster_size * 16);
	
		DEBUGPRINT(EXTENT, 6, ("\tstarting compressed @ extent group %Ld, offset %Ld.\n", group, offset));
		
		while(bytes_left > 0) {	
			// It's into a group
			length_to_read = min(bytes_left, ntfs->cluster_size * 16 - offset);

			err = ntfs_get_decompressed_compression_group_buffer(&db, ntfs, extent, group);
			if(err < B_NO_ERROR) return err;

			memcpy(buf_pos, (char *)db->buf + offset, length_to_read);			
			ntfs_release_decompressed_compression_group_ptr(ntfs, db);
		
			bytes_left -= length_to_read;
			(*bytes_read) += length_to_read;
			buf_pos += length_to_read;
			offset = 0;
			group++;
		}	
	}
	DEBUGPRINT(EXTENT, 5, ("ntfs_read_extent exiting, read %ld bytes.\n", *bytes_read));

	return B_NO_ERROR;
}

// This function performs the fixup technique
status_t ntfs_fixup_buffer(void *buf, uint32 length)
{
	uint16 fixup_offset = B_LENDIAN_TO_HOST_INT16(*((uint16 *)((char *)buf + 0x4)));
	uint16 fixup_list_size = B_LENDIAN_TO_HOST_INT16(*((uint16 *)((char *)buf + 0x6)) - 1);
	uint16 *fixup_list = (uint16 *)((char *)buf + fixup_offset);
	uint32 sectors = length/512;
	int i;
	uint16 *fixup_spot;
	
	DEBUGPRINT(IO, 8, ("ntfs_fixup_buffer called with buffer length %ld.\n", length));
	DEBUGPRINT(IO, 8, ("\tfixup offset = %d\n\tfixup list size = %d\n\tfixup_pattern = 0x%x.\n", fixup_offset, fixup_list_size, fixup_list[0]));
	
	if(fixup_list_size < sectors) {
		return B_ERROR;
	}
		
	for(i=1; i<=sectors; i++) {
		fixup_spot = ((uint16 *)((char *)buf + i*512 - 0x2));
		if(*fixup_spot == fixup_list[0]) {
			*fixup_spot = fixup_list[i];	
		} else {
			ERRPRINT(("ntfs_fixup_buffer found invalid pattern, 0x%x\n", *fixup_spot));
			return B_ERROR;
		}	
	}

	return B_NO_ERROR;
}

status_t ntfs_read_FILE_record(ntfs_fs_struct *ntfs, FILE_REC record_num, char *buf)
{
	uint64 block;
	status_t err;
	
	record_num = strip_high_16(record_num);

	DEBUGPRINT(DMFT, 5, ("NTFS: ntfs_read_FILE_record called on record %Ld.\n", record_num));

	// If we're loading from the first 16 MFT records, we know where it is without consulting
	// the MFT runlist. This is necessary for reading in the MFT file record (0), which is stored
	// in the MFT itself! Yes, the MFT stores it's own runlist. Go figure. 
	if(record_num <= 0xF) {
		// We know where these are, independant of the knowledge of the $MFT file
		err = ntfs_read_pos(ntfs,
			ntfs->nbb.MFT_cluster * ntfs->cluster_size + record_num * ntfs->mft_recordsize,
			ntfs->mft_recordsize, buf);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ntfs_read_FILE_record had error reading file record %Ld.\n", record_num));
			return err;
		}
	} else {
		// We're gonna have to look up the location of the FILE record from the MFT runlist.
		data_attr *data;
		VCN mft_vcn;
		
		// Get the data attribute from the MFT. It's runlist points to the FILE record stream.
		data = ntfs_find_attribute(ntfs->MFT, ATT_DATA, 0);
		if(!data) {
			ERRPRINT(("ntfs_read_FILE_record couldn't find data attribute on MFT..\n"));
			return EINVAL;
		}

		// Calculate the MFT's VCN to start reading in
		mft_vcn = (record_num * ntfs->mft_recordsize) / ntfs->cluster_size;
		
		if(ntfs->mft_recordsize >= ntfs->cluster_size) {
	        uint64 blocks_to_read = ntfs->mft_recordsize / ntfs->cluster_size;
			while(blocks_to_read > 0) {
				err = ntfs_get_block_from_extent(&data->extent, &block, mft_vcn);
				if(err < B_NO_ERROR) {
					ERRPRINT(("ntfs_read_FILE_record could not get block from MFT.\n"));
					return err;
				}
//				DEBUGPRINT(DMFT, 6, ("\treading MFT record at block %Ld.\n", block));					
				err = ntfs_read_pos(ntfs, block * ntfs->cluster_size, ntfs->cluster_size, buf);
				if(err < B_NO_ERROR) {
					ERRPRINT(("ntfs_read_FILE_record had error reading file record %Ld.\n", record_num));
					return err;
				}
				blocks_to_read--;
				buf += ntfs->cluster_size;
				mft_vcn++;		
			}			
		} else {
			err = ntfs_get_block_from_extent(&data->extent, &block, mft_vcn);
			if(err < B_NO_ERROR) {
				ERRPRINT(("ntfs_read_FILE_record could not get block from MFT.\n"));
				return err;
			}

			err = ntfs_read_pos(ntfs,
				block * ntfs->cluster_size + (record_num * ntfs->mft_recordsize) % ntfs->cluster_size,
				ntfs->mft_recordsize, buf);
			if(err < B_NO_ERROR) {
				ERRPRINT(("ntfs_read_FILE_record had error reading file record %Ld.\n", record_num));
				return err;
			}
		}						
	}
	
	DEBUGPRINT(VNODE, 5, ("ntfs_read_FILE_record exit.\n"));

	return B_NO_ERROR;
}


// Loads all of the FILE records into a vnode
status_t ntfs_load_vnode_from_MFT(ntfs_fs_struct *ntfs, vnode *v)
{
	uint32 i = 0;
	ntfs_FILE_record *fr;
	status_t err = B_NO_ERROR;
	char *buf;
	uint32 num_MFT_records;
	uint64 *MFT_records;

	DEBUGPRINT(VNODE, 5, ("ntfs_load_vnode_from_MFT called.\n"));

	// Set up the MFT record list
	MFT_records = ntfs_malloc(sizeof(uint64) * MAX_MFT_RECORDS);	
	if(!MFT_records) return ENOMEM;
	num_MFT_records = 1;
	MFT_records[0] = v->vnid;

	buf = ntfs_malloc(ntfs->mft_recordsize);
	if(!buf) {
		ERRPRINT(("ntfs_load_vnode_from_MFT had error allocating buffer.\n"));
		ntfs_free(MFT_records);
		return ENOMEM;
	}
	
	while(i < num_MFT_records) {
		// read in the file record
		err = ntfs_read_FILE_record(ntfs, MFT_records[i], buf);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ntfs_load_vnode_from_MFT had error reading FILE record for vnid %Ld.\n", v->vnid));
			goto out;
		}

		// We should have a MFT record now, lets verify
		fr = (ntfs_FILE_record *)buf;
		if(B_LENDIAN_TO_HOST_INT32(fr->magic) != FILE_REC_MAGIC) {
			ERRPRINT(("ntfs_load_vnode_from_MFT FILE record failed magic test\n"));
			err = EINVAL;
			goto out;
		}
	
		// Ok, lets do the fixup
		err = ntfs_fixup_buffer(buf, ntfs->mft_recordsize);
		if(err < B_NO_ERROR) {
			dprintf("ntfs_load_vnode_from_MFT FILE record failed fixup\n");
			goto out;
		}
		
		// Check the in-use flag
		if(!(B_LENDIAN_TO_HOST_INT16(fr->flags) & FILE_REC_IN_USE_FLAG)) {
			dprintf("ntfs_load_vnode_from_MFT FILE record is not marked in-use.\n");
			err = EINVAL;
			goto out;
		}

		// Check the sequence number
		if(i==0) {
			// If this is the first record to read in for this vnode, stores it's sequence #
			v->sequence_num = B_LENDIAN_TO_HOST_INT16(fr->seq_number);
			// Add the sequence number to the FILE record number stored in the file_record list for this vnode
			MFT_records[0] += (uint64)v->sequence_num << 48;
			// Check to see if base_file_rec is 0. If it isn't, this record is an extension and therefore not what we want
			if(B_LENDIAN_TO_HOST_INT64(fr->base_file_rec) != 0) {
				dprintf("ntfs_load_vnode_from_MFT found nonzero base_file_rec pointer on base file record.\n");
				err = EINVAL;
				goto out;
			}
		} else {
			// If it's not the first record, verify that the base file record pointer is pointing to the base
			// and the sequence number matches
			if((strip_high_16(B_LENDIAN_TO_HOST_INT64(fr->base_file_rec)) != v->vnid) || (get_high_16(B_LENDIAN_TO_HOST_INT64(fr->base_file_rec)) != v->sequence_num)) {
				dprintf("ntfs_load_vnode_from_MFT extended FILE record failed base file record sequence number check\n");
				err = EINVAL;
				goto out;
			}
			// Now, check that the sequence number in the current file record matches the sequence number stored in the top
			// 16 bits of the record we were instructed to load. This number was loaded in a previous attribute list attribute
			// load. This verifies that the attribute list attribute is in sync with this file record
			if(get_high_16(MFT_records[i]) != B_LENDIAN_TO_HOST_INT16(fr->seq_number)) {
				dprintf("ntfs_load_vnode_from_MFT extended FILE record's sequence number does not match previous sequence #.\n");
				err = EINVAL;
				goto out;
			}
		}				
							
		err = load_attributes(ntfs, v, buf, MFT_records, &num_MFT_records);
		if(err < B_NO_ERROR) {
			ERRPRINT(("ntfs_load_vnode had error loading attributes for vnid %Ld.", v->vnid));
			goto out;
		}
		
		i++;
	}		


out:
	DEBUGPRINT(VNODE, 5, ("ntfs_load_vnode_from_MFT exit with code %ld.\n", err));
	ntfs_free(MFT_records);
	ntfs_free(buf);
	return err;
}

