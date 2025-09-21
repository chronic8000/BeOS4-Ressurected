#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include <errno.h>
#include <string.h>
#include <KernelExport.h>
#include <fs_info.h>

typedef off_t dr9_off_t;
#include <lock.h>
#include <cache.h>

//status_t cfs_flush_disk_buffer(cfs_info *fsinfo, off_t pos, off_t size, bool invalidate_buffer)
//{
//	return B_NO_ERROR;
//}


status_t 
cfs_read_disk_uncached(cfs_info *fsinfo, off_t pos, void *buffer,
                       size_t buffer_size)
{
	ssize_t read_size;
	read_size = read_pos(fsinfo->dev_fd, pos, buffer, buffer_size);
	if(read_size < buffer_size) {
		if(read_size < 0)
			return errno;
		else
			return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

status_t 
cfs_write_disk_uncached(cfs_info *fsinfo, off_t pos, void *buffer,
                       size_t buffer_size)
{
	ssize_t write_size;
	write_size = write_pos(fsinfo->dev_fd, pos, buffer, buffer_size);
	if(write_size < buffer_size) {
		if(write_size < 0)
			return errno;
		else
			return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

status_t 
cfs_read_disk_etc(cfs_info *fsinfo, off_t pos, void *buffer,
                  size_t *buffer_size, size_t min_read_size)
{
	size_t size_left;
	off_t curpos;
	uint8 *dest;
	
	if(pos < fsinfo->super_block_2_pos || pos+min_read_size > fsinfo->size) {
		PRINT_ERROR(("cfs_read_disk: position %Ld outsize fs bounds\n", pos));
		return B_IO_ERROR;
	}
	//if(pos & 7) {
	//	dprintf("cfs_read_disk: bad position, %Ld\n", pos);
	//	return B_IO_ERROR;
	//}
	
	curpos = pos & ~((off_t)fsinfo->cache_block_size-1);
	size_left = *buffer_size;
	dest = buffer;
	
	if(pos+size_left > fsinfo->size) {
		size_left = fsinfo->size-pos;
	}
	
	*buffer_size = 0;
	
	//dprintf("cfs_read_disk_etc %Ld %p %ld min %ld\n",
	//        pos, buffer, *buffer_size, min_read_size);

	while(size_left > 0) {
		uint8 *cache_block;
		size_t block_used = fsinfo->cache_block_size;
		if(curpos < pos)
			block_used -= (pos-curpos);
		if(size_left < block_used)
			block_used = size_left;

	//dprintf("cfs_read_disk_etc curpos %Ld %p %ld left %ld\n",
	//        curpos, dest, *buffer_size, size_left);
		
		cache_block = get_block(fsinfo->dev_fd, curpos/fsinfo->cache_block_size,
		                        fsinfo->cache_block_size);
		if(cache_block == NULL) {
			if(*buffer_size < min_read_size) {
				PRINT_ERROR(("cfs_read_disk: could not get block at %Ld\n",
				             curpos));
				return B_IO_ERROR;
			}
			else
				return B_NO_ERROR;
		}
			
		if(curpos < pos) {
			memcpy(dest, cache_block+(pos-curpos), block_used);
		}
		else {
			memcpy(dest, cache_block, block_used);
		}
		release_block(fsinfo->dev_fd, curpos / fsinfo->cache_block_size);
		curpos += fsinfo->cache_block_size;
		size_left -= block_used;
		dest += block_used;
		*buffer_size += block_used;
	}
	return B_NO_ERROR;
}

status_t 
cfs_read_disk(cfs_info *fsinfo, off_t pos, void *buffer, size_t buffer_size)
{
	return cfs_read_disk_etc(fsinfo, pos, buffer, &buffer_size, buffer_size);
}

static status_t 
cfs_write_disk_etc(cfs_info *fsinfo, cfs_transaction *transaction, off_t pos,
                   const void *buffer, size_t buffer_size, bool flush)
{
	status_t err;
	size_t size_left;
	off_t curpos;
	const uint8 *src;
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(fsinfo->fs_flags & B_FS_IS_READONLY)
		return B_READ_ONLY_DEVICE;

	if(pos < fsinfo->super_block_2_pos || pos+buffer_size > fsinfo->size) {
		PRINT_ERROR(("cfs_write_disk: position %Ld outsize fs bounds\n", pos));
		return B_IO_ERROR;
	}
	//if(pos & 7) {
	//	dprintf("cfs_write_disk: bad position, %Ld\n", pos);
	//	return B_IO_ERROR;
	//}

	curpos = pos & ~((off_t)fsinfo->cache_block_size - 1);
	size_left = buffer_size;
	src = buffer;
	
	if(pos+size_left > fsinfo->size) {
		size_left = fsinfo->size-pos;
	}

	//dprintf("cfs_write_disk 0x%Lx %p %ld\n",
	//        pos, buffer, buffer_size);
	
	while(size_left > 0) {
		uint8 *cache_block;
		size_t block_used = fsinfo->cache_block_size;
		if(curpos < pos)
			block_used -= (pos-curpos);
		if(size_left < block_used)
			block_used = size_left;

	//dprintf("cfs_write_disk curpos 0x%Lx %p left %ld, block_used %ld\n",
	//        curpos, src, size_left, block_used);
		
		//cache_block = get_block(fsinfo->dev_fd, curpos/fsinfo->dev_block_size,
		//                        fsinfo->dev_block_size);

		if(!flush) {
			err = cfs_sub_transaction(fsinfo, transaction, 1);
			if(err != B_NO_ERROR)
				return err;
		}

		cache_block = get_block_logged(fsinfo, transaction, curpos);
		if(cache_block == NULL) {
			PRINT_ERROR(("cfs_write_disk: could not get block at %Ld\n",
			             curpos));
			return B_IO_ERROR;
		}
			
		if(curpos < pos) {
			memcpy(cache_block+(pos-curpos), src, block_used);
		}
		else {
			memcpy(cache_block, src, block_used);
		}
		//mark_blocks_dirty(fsinfo->dev_fd, curpos/fsinfo->dev_block_size, 1);
		//release_block(fsinfo->dev_fd, curpos/fsinfo->dev_block_size);
		//if(flush)
		//	flush_blocks(fsinfo->dev_fd, curpos/fsinfo->dev_block_size, 1);
		curpos += fsinfo->cache_block_size;
		size_left -= block_used;
		src += block_used;
	}
	return B_NO_ERROR;
}

status_t 
cfs_write_disk_cached(cfs_info *fsinfo, cfs_transaction *transaction,
                      off_t pos, const void *buffer, size_t buffer_size)
{
	return cfs_write_disk_etc(fsinfo, transaction, pos, buffer,
	                          buffer_size, false);
}

status_t 
cfs_write_disk(cfs_info *fsinfo, cfs_transaction *transaction,
               off_t pos, const void *buffer, size_t buffer_size)
{
	return cfs_write_disk_etc(fsinfo, transaction, pos, buffer,
	                          buffer_size, true);
}

status_t
cfs_read_entry_info(cfs_info *fsinfo, off_t vnid, cfs_entry_info *entry)
{
	status_t err;
	off_t location = cfs_vnid_to_offset(vnid);

	entry->logical_size = 0; // if ver1, the following read wont overwrite this
	err = cfs_read_disk(fsinfo, location, entry, get_entry_size(fsinfo->fs_version));
	if(err != B_NO_ERROR)
		return err;

	if(entry->version != cfs_vnid_to_version(vnid)) {
		PRINT_ERROR(("cfs_read_entry: entry at %Ld, expected version %d, "
		             "got %d\n", location, cfs_vnid_to_version(vnid),
		             entry->version));
		return B_ENTRY_NOT_FOUND;
	}
	return B_NO_ERROR;
}

status_t
cfs_write_entry_info(cfs_info *fsinfo, cfs_transaction *transaction,
                     off_t vnid, cfs_entry_info *entry)
{
	if(entry->version != cfs_vnid_to_version(vnid)) {
		PRINT_INTERNAL_ERROR(("cfs_write_entry: entry version wrong\n"));
		return B_ERROR;
	}
	return cfs_write_disk(fsinfo, transaction, cfs_vnid_to_offset(vnid),
	                      entry, get_entry_size(fsinfo->fs_version));
}

status_t
cfs_read_entry(cfs_info *fsinfo, off_t vnid, cfs_entry_info *entry, char *name)
{
	status_t err;
	ssize_t readlen;
	int i;
	
	err = cfs_read_entry_info(fsinfo, vnid, entry);
	if(err != B_NO_ERROR)
		return err;
	
	readlen = CFS_MAX_NAME_LEN;
	err = cfs_read_disk_etc(fsinfo, entry->name_offset, name, &readlen, 1);
	if(err != B_NO_ERROR)
		return err;

	for(i = 0; i < readlen; i++) {
		if(name[i] == '\0')
			break;
	}
	if(i == readlen) {
		PRINT_ERROR(("cfs_read_entry: bad entry name\n"));
		return B_IO_ERROR;
	}
	return B_NO_ERROR;
}
