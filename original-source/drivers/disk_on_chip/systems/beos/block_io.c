#include <SupportDefs.h>
#include <KernelExport.h>
#include <string.h>
#include "block_io.h"
#include <malloc.h>

#include "debugoutput.h"

/*------------------------------------------------------------------------------
** scratch buffer
*/

static uint8		*scratch;		/* when reading blocks smaller
									 * than the physical block size or
									 * for unaligned memory transfers.
									*/
static sem_id		scratch_sem;	/* sem protecting the scratch buffer */
const static int	scratch_size = 32 * 1024;


/*------------------------------------------------------------------------------
** initializtion
*/

status_t
init_scratch_buffer()
{
	status_t err;
	
	scratch = malloc(scratch_size);
	if(scratch == NULL) {
		return B_NO_MEMORY;
	}
	err = scratch_sem = create_sem (1, "ata_scratchbuf_sem");
	if(err < B_NO_ERROR) {
		free(scratch);
	}
	return err;
}

void
uninit_scratch_buffer()
{
	free(scratch);
	delete_sem(scratch_sem);
}

/*------------------------------------------------------------------------------
** iovec functions
*/

static status_t
unlock_memory_vec(const iovec *vec, size_t veccount, uint32 startbyte,
                size_t size, uint32 flags)
{
	status_t	err;
	uint8		*lockbuf;
	size_t		locksize;
	size_t		unlocked_bytes = 0;
	int			i = 0;
	uint32		tmpstartbyte = startbyte;

	while(unlocked_bytes < size) {
		while(i < veccount && vec[i].iov_len <= tmpstartbyte) {
			tmpstartbyte -= vec[i].iov_len;
			i++;
		}
		if(i == veccount) {
			err = B_BAD_VALUE;
			goto err;
		}
		lockbuf = (uint8*)vec[i].iov_base+tmpstartbyte;
		locksize = min(vec[i].iov_len-tmpstartbyte, size-unlocked_bytes);
		err = unlock_memory(lockbuf, locksize, flags);
		if(err != B_NO_ERROR)
			goto err;
		tmpstartbyte += locksize;
		unlocked_bytes += locksize;
	}
	return B_NO_ERROR;
err:
	return err;
}

static status_t
lock_memory_vec(const iovec *vec, size_t veccount, uint32 startbyte,
                size_t size, uint32 flags)
{
	status_t	err;
	uint8		*lockbuf;
	size_t		locksize;
	size_t		locked_bytes = 0;
	int			i = 0;
	uint32		tmpstartbyte = startbyte;

	while(locked_bytes < size) {
		while(i < veccount && vec[i].iov_len <= tmpstartbyte) {
			tmpstartbyte -= vec[i].iov_len;
			i++;
		}
		if(i == veccount) {
			err = B_BAD_VALUE;
			goto err;
		}
		lockbuf = (uint8*)vec[i].iov_base+tmpstartbyte;
		locksize = min(vec[i].iov_len-tmpstartbyte, size-locked_bytes);
		err = lock_memory(lockbuf, locksize, flags);
		if(err != B_NO_ERROR)
			goto err;
		tmpstartbyte += locksize;
		locked_bytes += locksize;
	}
	return B_NO_ERROR;
err:
	unlock_memory_vec(vec, veccount, startbyte, locked_bytes, flags);
	return err;
}

status_t
mem2veccpy(const iovec *destvec, size_t destveccount, uint32 deststartbyte,
           uint8 *srcbuf, size_t size)
{
	uint8		*destbuf;
	size_t		chunksize;

	while(size > 0) {
		while(destveccount > 0 && destvec->iov_len <= deststartbyte) {
			deststartbyte -= destvec->iov_len;
			destvec++;
			destveccount--;
		}
		if(destveccount == 0) {
			return B_BAD_VALUE;
		}
		destbuf = (uint8*)destvec->iov_base+deststartbyte;
		chunksize = min(destvec->iov_len-deststartbyte, size);
		memcpy(destbuf, srcbuf, chunksize);
		srcbuf += chunksize;
		deststartbyte += chunksize;
		size -= chunksize;
	}
	return B_NO_ERROR;
}

status_t
vec2memcpy(uint8 *destbuf, const iovec *srcvec, size_t srcveccount,
           uint32 srcstartbyte, size_t size)
{
	uint8		*srcbuf;
	size_t		chunksize;

	while(size > 0) {
		while(srcveccount > 0 && srcvec->iov_len <= srcstartbyte) {
			srcstartbyte -= srcvec->iov_len;
			srcvec++;
			srcveccount--;
		}
		if(srcveccount == 0) {
			return B_BAD_VALUE;
		}
		srcbuf = (uint8*)srcvec->iov_base+srcstartbyte;
		chunksize = min(srcvec->iov_len-srcstartbyte, size);
		memcpy(destbuf, srcbuf, chunksize);
		destbuf += chunksize;
		srcstartbyte += chunksize;
		size -= chunksize;
	}
	return B_NO_ERROR;
}

/*------------------------------------------------------------------------------
** read/write
*/

static
status_t
read_vec(void *cookie, off_t pos, const iovec * const vec,
         const size_t veccount, size_t *numBytes)
{
	const char *debugprefix = DEBUGPREFIX "read_vec:";
	uint32		num_drive_blks = get_capacity(cookie);
	uint32		blocksize = get_blocksize(cookie);
	uint32		bad_alignment_mask = get_bad_alignment_mask(cookie);
	uint32		maxtransfersize = get_max_transfer_size(cookie);

	bool		got_scratch_sem = false;
	bool		scratch_buffer_used = false;

	status_t	err = B_NO_ERROR;
	uint32		bytes_left = *numBytes;
	const iovec	*currvec = vec;
	size_t		currveccount = veccount;
	uint32		currstartbyte = 0;

	if(num_drive_blks == 0) {
		*numBytes = 0;
		return B_DEV_NO_MEDIA;
	}
	if(blocksize == 0) {
		*numBytes = 0;
		DE(1) dprintf("%s illegal blocksize\n", debugprefix);
		return B_ERROR;
	}

	DF(1) dprintf("%s pos %Ld, size %ld\n", debugprefix, pos, *numBytes);

	while (bytes_left > 0) {
		uint32		first_block = pos / blocksize;
		uint32		first_block_byte_offset = pos - first_block * blocksize;
		uint32		blockcount;
		uint32		transfersize;	// in bytes
		const iovec	*access_vec;
		size_t		access_vec_count;
		uint32		access_startbyte;
		iovec		scratch_vec;
		bool		unalignedvec = false;
		int			i;
		
		while(currvec->iov_len <= currstartbyte) {
			if(currveccount == 0) {
				DE(1) dprintf("%s iovec to small\n", debugprefix);
				err = B_BAD_VALUE;
				goto err;
			}
			currstartbyte -= currvec->iov_len;
			currvec++;
			currveccount--;
		}
		for(i=0; i<currveccount; i++) {
			int vecoffset = i == 0 ? currstartbyte : 0;
			if(((uint32)currvec[i].iov_base + vecoffset) & bad_alignment_mask)
				unalignedvec = true;
			if((currvec[i].iov_len - vecoffset) & bad_alignment_mask)
				unalignedvec = true;
		}

		if(first_block_byte_offset || bytes_left < blocksize || unalignedvec) {
			DF(3) dprintf("%s pos %Ld, using scratch buf\n", debugprefix, pos);

			scratch_vec.iov_base = scratch;
			scratch_vec.iov_len = scratch_size;
			access_vec = &scratch_vec;
			access_vec_count = 1;
			access_startbyte = 0;

			if (!got_scratch_sem) {
				err = acquire_sem(scratch_sem);
				if(err != B_NO_ERROR)
					break;
				got_scratch_sem = true;
			}
			
			blockcount = min( scratch_size / blocksize,
				              ( first_block_byte_offset +
				                bytes_left + blocksize - 1 ) / blocksize );
			scratch_buffer_used = true;
		}
		else {
			access_vec = currvec;
			access_vec_count = currveccount;
			access_startbyte = currstartbyte;
			blockcount = bytes_left / blocksize;
		}

		/* check for end of device */
		if (first_block >= num_drive_blks) {
			break;
		}
		if (first_block + blockcount > num_drive_blks) {
			blockcount = num_drive_blks - first_block;
		}

		if(blockcount * blocksize > maxtransfersize) {
			blockcount = maxtransfersize / blocksize;	/* limit transfersize */
		}

retry_lock_mem:
		transfersize = blockcount * blocksize;

		err = lock_memory_vec(access_vec, access_vec_count, access_startbyte,
		                      transfersize, B_DMA_IO | B_READ_DEVICE);
		if (err != B_NO_ERROR) {
			if(blockcount > 1 && err == B_NO_MEMORY) {
				DE(3) dprintf("%s could not lock memory vec of size %ld\n",
				              debugprefix, transfersize);
				blockcount /= 2;
				goto retry_lock_mem;
			}
			DE(2) dprintf("%s could not lock memory vec at 0x%08lx, size %ld\n",
			              debugprefix, (uint32)access_vec, transfersize);
			break;
		}
		err = read_blocks(cookie, first_block, blockcount, access_vec,
		                  access_vec_count, access_startbyte);
		unlock_memory_vec(access_vec, access_vec_count, access_startbyte,
		                  transfersize, B_DMA_IO | B_READ_DEVICE);

		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s read error block 0x%lx count 0x%lx\n",
			              debugprefix, first_block, blockcount);
			break;
		}

		if (scratch_buffer_used) {
			scratch_buffer_used = false;
			transfersize =
				min(transfersize - first_block_byte_offset, bytes_left);
			mem2veccpy(currvec, currveccount, currstartbyte,
			           scratch+first_block_byte_offset, transfersize);
			//memcpy(buf, access_buf + first_block_byte_offset, transfersize);
		}

		bytes_left -= transfersize; 
		pos += transfersize;
		currstartbyte += transfersize;
	}

err:
	/*
	 * figure out how many blocks we actually read
	 */
	*numBytes -= bytes_left;

	if (got_scratch_sem)
		release_sem_etc(scratch_sem, 1, B_DO_NOT_RESCHEDULE);
	
	if(err != B_NO_ERROR) {
		DE(1) dprintf("%s %s\n", debugprefix, strerror(err));
	}
	
	return err;
}

status_t
driver_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	iovec vec;
	vec.iov_base = buf;
	vec.iov_len = *len;
	return read_vec(cookie, pos, &vec, 1, len);
}

status_t
driver_readv(void *cookie, off_t pos, const iovec * const vec,
              const size_t veccount, size_t *numBytes)
{
	const char *debugprefix = DEBUGPREFIX "readv:";
	size_t size;
	int i;
	status_t err;
	
	err = lock_memory((void*)vec, sizeof(iovec)*veccount, 0);
	if(err != B_NO_ERROR) {
		DE(1) dprintf("%s could not lock iovec, %s\n",
		              debugprefix, strerror(err));
		return err;
	}

	size = 0;
	for(i=0; i<veccount; i++) {
		size += vec[i].iov_len;
	}
	*numBytes = size;

	err = read_vec(cookie, pos, vec, veccount, numBytes);

	unlock_memory((void*)vec, sizeof(iovec)*veccount, 0);
	return err;
}

static status_t
write_vec(void *cookie, off_t pos, const iovec * const vec,
          const size_t veccount, size_t *numBytes)
{
	const char *debugprefix = DEBUGPREFIX "write_vec:";
	uint32		num_drive_blks = get_capacity(cookie);
	uint32		blocksize = get_blocksize(cookie);
	uint32		bad_alignment_mask = get_bad_alignment_mask(cookie);
	uint32		maxtransfersize = get_max_transfer_size(cookie);

	bool		got_scratch_sem = false;

	status_t	err = B_NO_ERROR;
	uint32		bytes_left = *numBytes;
	const iovec	*currvec = vec;
	size_t		currveccount = veccount;
	uint32		currstartbyte = 0;
	
	if(num_drive_blks == 0) {
		*numBytes = 0;
		return B_DEV_NO_MEDIA;
	}
	if(blocksize == 0) {
		*numBytes = 0;
		DE(1) dprintf("%s illegal blocksize\n", debugprefix);
		return B_ERROR;
	}

	DF(1) dprintf("%s pos %Ld, size %ld\n", debugprefix, pos, *numBytes);

	while (bytes_left > 0) {
		uint32		first_block = pos / blocksize;
		uint32		first_block_byte_offset = pos - first_block * blocksize;
		uint32		blockcount;
		uint32		disk_transfersize;		// in bytes
		uint32		memory_transfersize;	// in bytes
		const iovec	*access_vec;
		size_t		access_vec_count;
		uint32		access_startbyte;
		iovec		scratch_vec;
		bool		unalignedvec = false;
		int			i;
		
		while(currvec->iov_len <= currstartbyte) {
			if(currveccount == 0) {
				DE(1) dprintf("%s iovec to small\n", debugprefix);
				err = B_BAD_VALUE;
				goto err;
			}
			currstartbyte -= currvec->iov_len;
			currvec++;
			currveccount--;
		}
		for(i=0; i<currveccount; i++) {
			int vecoffset = i == 0 ? currstartbyte : 0;
			if(((uint32)currvec[i].iov_base + vecoffset) & bad_alignment_mask)
				unalignedvec = true;
			if((currvec[i].iov_len - vecoffset) & bad_alignment_mask)
				unalignedvec = true;
		}

		if(first_block_byte_offset || bytes_left < blocksize || unalignedvec) {
			DF(3) dprintf("%s pos %Ld, using scratch buf\n", debugprefix, pos);

			scratch_vec.iov_base = scratch;
			scratch_vec.iov_len = scratch_size;
			access_vec = &scratch_vec;
			access_vec_count = 1;
			access_startbyte = 0;

			if (!got_scratch_sem) {
				err = acquire_sem(scratch_sem);
				if(err != B_NO_ERROR)
					break;
				got_scratch_sem = true;
			}
			
			if(first_block_byte_offset || bytes_left < blocksize) {
				//dprintf(DEBUGPREFIX "write: sub blocksize write "
				//        "%d bytes at offset %d in block %d\n",
				//         bytes_left, first_block_byte_offset, first_block);
				blockcount = 1;
				err = read_blocks(cookie, first_block, blockcount,
				                  access_vec, 1, 0);
				if(err != B_NO_ERROR) {
					DE(2) dprintf("%s read error on block 0x%lx\n",
					              debugprefix, first_block);
					break;
				}
				memory_transfersize = min(blocksize,
				                          first_block_byte_offset+bytes_left) -
				                      first_block_byte_offset;
			}
			else {
				blockcount = min( scratch_size / blocksize,
					              bytes_left / blocksize );
				memory_transfersize = blockcount*blocksize;
			}
			vec2memcpy(scratch+first_block_byte_offset, currvec, currveccount,
			           currstartbyte, memory_transfersize);
			//memcpy(access_buf+first_block_byte_offset, buf, memory_transfersize);
		}
		else {
			access_vec = currvec;
			access_vec_count = currveccount;
			access_startbyte = currstartbyte;
			blockcount = bytes_left / blocksize;
			memory_transfersize = blockcount*blocksize;
		}

		/* check for end of device */
		if (first_block >= num_drive_blks) {
			break;
		}
		if (first_block + blockcount > num_drive_blks) {
			blockcount = num_drive_blks - first_block;
			memory_transfersize = blockcount*blocksize;
			/* safe since only one block is transferred for the other case */
			/* and this statement cannot be reached then */
		}

		if(blockcount * blocksize > maxtransfersize) {
			blockcount = maxtransfersize / blocksize;	/* limit transfersize */
			memory_transfersize = blockcount*blocksize;
		}

retry_lock_mem:
		disk_transfersize = blockcount * blocksize;

		err = lock_memory_vec(access_vec, access_vec_count, access_startbyte,
		                      disk_transfersize, B_DMA_IO);
		if (err != B_NO_ERROR) {
			if(blockcount > 1 && err == B_NO_MEMORY) {
				DE(3) dprintf("%s could not lock memory vec of size %ld\n",
				              debugprefix, disk_transfersize);
				blockcount /= 2;
				memory_transfersize = blockcount*blocksize;
				goto retry_lock_mem;
			}
			DE(2) dprintf("%s could not lock memory vec at 0x%08lx, size %ld\n",
			              debugprefix, (uint32)access_vec, disk_transfersize);
			break;
		}
		err = write_blocks(cookie, first_block, blockcount,
		                   access_vec, access_vec_count, access_startbyte);
		unlock_memory_vec(access_vec, access_vec_count, access_startbyte,
		                  disk_transfersize, B_DMA_IO | B_READ_DEVICE);

		if (err != B_NO_ERROR) {
			DE(2) dprintf("%s write error block 0x%lx count 0x%lx\n",
			              debugprefix, first_block, blockcount);
			break;
		}
		
		bytes_left -= memory_transfersize; 
		pos += memory_transfersize;
		currstartbyte += memory_transfersize;
	}

err:
	/*
	 * figure out how many blocks we actually wrote
	 */
	*numBytes -= bytes_left;

	if (got_scratch_sem)
		release_sem_etc(scratch_sem, 1, B_DO_NOT_RESCHEDULE);
	
	if(err != B_NO_ERROR) {
		DE(1) dprintf("%s %s\n", debugprefix, strerror(err));
	}	
	
	return err;
}

status_t
driver_write(void *cookie, off_t pos, const void *buf, size_t *len)
{
	iovec vec;
	vec.iov_base = (void*)buf;
	vec.iov_len = *len;
	return write_vec(cookie, pos, &vec, 1, len);
}

status_t
driver_writev(void *cookie, off_t pos, const iovec * const vec,
              const size_t veccount, size_t *numBytes)
{
	const char *debugprefix = DEBUGPREFIX "writev:";
	size_t size;
	int i;
	status_t err;
	
	err = lock_memory((void*)vec, sizeof(iovec)*veccount, 0);
	if(err != B_NO_ERROR) {
		DE(1) dprintf("%s could not lock iovec, %s\n",
		              debugprefix, strerror(err));
		return err;
	}

	size = 0;
	for(i=0; i<veccount; i++) {
		size += vec[i].iov_len;
	}
	*numBytes = size;
	err = write_vec(cookie, pos, vec, veccount, numBytes);
	unlock_memory((void*)vec, sizeof(iovec)*veccount, 0);
	return err;
}

