#include "ntfs.h"
#include "extent.h"

extern int sprintf(char *s, const char *format, ...);

uint8 ntfs_debuglevels[] = {
	0, // ALL
	0, // IO
	0, // FILE
	0, // DIR
	0, // EXTENT
	0, // ALLOC
	4, // FS
	0, // VNODE
	0, // ATT
	0, // MALLOC
	0, // MFT
	0, // DIRWALK
	0, // RUNLIST
	0  // ATTR
};

ntfs_fs_struct *create_ntfs_fs_struct()
{
	ntfs_fs_struct *ptr;
	char text[128];
	int i;

	ptr = ntfs_malloc(sizeof(ntfs_fs_struct));
	if(!ptr) {
		ERRPRINT(("create_ntfs_fs_struct failed to allocate memory for structure.\n"));
		return NULL;
	}

	memset(ptr, 0, sizeof(ntfs_fs_struct));	

	sprintf(text, "ntfsdirlock 0x%x\n", (unsigned int)ptr);
	new_lock(&ptr->dir_lock, text);
	if (ptr->dir_lock.s <= 0) {
		goto semerror;
	}
	set_sem_owner(ptr->dir_lock.s, B_SYSTEM_TEAM);

	// Set up the decompression buffers
	for(i=0; i<DECOMPRESSION_BUFFERS; i++) {
		ptr->dbuf[i].index = i;
		
		sprintf(text, "ntfsdbuf%dsem 0x%x\n", i, (unsigned int)ptr); 
		new_lock(&ptr->dbuf[i].buf_lock, text);
		if (ptr->dbuf[i].buf_lock.s <= 0) {
			goto semerror;
		}
		set_sem_owner(ptr->dbuf[i].buf_lock.s, B_SYSTEM_TEAM);	
	}
	
	sprintf(text, "ntfsdbufsem 0x%x\n", (unsigned int)ptr);
	new_mlock(&ptr->d_buf_mlock, DECOMPRESSION_BUFFERS, text);
	if (ptr->d_buf_mlock.s <= 0) {
		goto semerror;
	}
	set_sem_owner(ptr->d_buf_mlock.s, B_SYSTEM_TEAM);

	sprintf(text, "ntfsdsearchsem 0x%x\n", (unsigned int)ptr);
	new_lock(&ptr->d_search_lock, text);
	if (ptr->d_search_lock.s <= 0) {
		goto semerror;
	}
	set_sem_owner(ptr->d_search_lock.s, B_SYSTEM_TEAM);	
	
	return ptr;
semerror:
	remove_ntfs_fs_struct(ptr);
	return NULL;
}

status_t remove_ntfs_fs_struct(ntfs_fs_struct *ntfs)
{
	int i;

	DEBUGPRINT(FS, 5, ("remove_ntfs_fs_struct entry.\n"));

	if(ntfs->device_name) ntfs_free(ntfs->device_name);

	ntfs_deallocate_vnode(ntfs->UpCase);
	ntfs_deallocate_vnode(ntfs->BITMAP);
	ntfs_deallocate_vnode(ntfs->VOL);
	ntfs_deallocate_vnode(ntfs->MFT);
	
	DEBUGPRINT(FS, 6, ("\tfreeing all decompression buffers...\n"));

	for(i=0; i<DECOMPRESSION_BUFFERS; i++) {
		if(ntfs->dbuf[i].buf_lock.s > 0) free_lock(&ntfs->dbuf[i].buf_lock);
		if(ntfs->dbuf[i].buf) ntfs_free(ntfs->dbuf[i].buf);
		if(ntfs->dbuf[i].compressed_buf) ntfs_free(ntfs->dbuf[i].compressed_buf);

	}

	DEBUGPRINT(FS, 6, ("\tremoving semaphores...\n"));

	// Remove the semaphores
	if(ntfs->dir_lock.s > 0) free_lock(&ntfs->dir_lock);
	if(ntfs->d_buf_mlock.s > 0) free_mlock(&ntfs->d_buf_mlock);
	if(ntfs->d_search_lock.s > 0) free_lock(&ntfs->d_search_lock);
	
	DEBUGPRINT(FS, 6, ("\tdeallocating cache and closing device...\n"));

	// Deallocate the cache
	if ((ntfs->cache_initialized))
		remove_cached_device_blocks(ntfs->fd, NO_WRITES);

	// Close the device
	if(ntfs->fd>0) close(ntfs->fd);	

	ntfs_free(ntfs);

	DEBUGPRINT(FS, 5, ("remove_ntfs_fs_struct exit.\n"));

	return B_NO_ERROR;
}


status_t  ntfs_read_bios_block(ntfs_fs_struct *ntfs)
{
	int retval;
	char temp[512];
	int32 magic;
	
	DEBUGPRINT(FS, 5, ("ntfs_read_bios_block entry.\n"));
	
	retval = read_pos(ntfs->fd, 0, temp, 512);
	if(retval != 512) {
		ERRPRINT(("ntfs_read_bios_block error reading bootsector, aborting mount...\n"));
		return EIO;
	}
	
	// Check the magic number. 'NTFS' at offset 0x3
	magic = *(int32 *)(temp + 0x3);
	if(magic != B_LENDIAN_TO_HOST_INT32(0x5346544e)) {
		ERRPRINT(("ntfs_read_bios_block sez magic number does not match, aborting mount...\n"));
		return EINVAL;
	}		

	// Copy the bios block into the data structure. 
	memcpy(&ntfs->nbb.bytes_per_sector, temp+0x0b, 2);
	LENDIAN_TO_HOST16(ntfs->nbb.bytes_per_sector);
	DEBUGPRINT(FS, 6, ("\tbytes per sector (according to the boot block) = %d.\n", ntfs->nbb.bytes_per_sector));
	memcpy(&ntfs->nbb.sectors_per_cluster, temp+0x0d, 1);
	DEBUGPRINT(FS, 6, ("\tsectors per cluster = %d.\n", ntfs->nbb.sectors_per_cluster));
	// calculate the cluster size
	ntfs->cluster_size = ntfs->nbb.bytes_per_sector*ntfs->nbb.sectors_per_cluster;
	DEBUGPRINT(FS, 6, ("\tcluster size = %ld.\n", ntfs->cluster_size));
	// calculate the compressed clusters size
	ntfs->clusters_per_compressed_block = 4096 / ntfs->cluster_size;
	DEBUGPRINT(FS, 6, ("\tclusters per compressed block = %ld.\n", ntfs->clusters_per_compressed_block));
	memcpy(&ntfs->nbb.reserved_sectors, temp+0x0e, 2);
	LENDIAN_TO_HOST16(ntfs->nbb.reserved_sectors);
	memcpy(&ntfs->nbb.media_type, temp+0x15, 1);
	memcpy(&ntfs->nbb.sectors_per_track, temp+0x18, 2);
	LENDIAN_TO_HOST16(ntfs->nbb.sectors_per_track);
	memcpy(&ntfs->nbb.num_heads, temp+0x1a, 2);
	LENDIAN_TO_HOST16(ntfs->nbb.num_heads);
	memcpy(&ntfs->nbb.hidden_sectors, temp+0x1c, 4);
	LENDIAN_TO_HOST32(ntfs->nbb.hidden_sectors);	
	memcpy(&ntfs->nbb.total_sectors, temp+0x28, 8);
	LENDIAN_TO_HOST64(ntfs->nbb.total_sectors);
	DEBUGPRINT(FS, 6, ("\ttotal sectors = %Ld.\n", ntfs->nbb.total_sectors));
	memcpy(&ntfs->nbb.MFT_cluster, temp+0x30, 8);
	LENDIAN_TO_HOST64(ntfs->nbb.MFT_cluster);
	DEBUGPRINT(FS, 6, ("\tMFT cluster = %Ld.\n", ntfs->nbb.MFT_cluster));
	memcpy(&ntfs->nbb.MFT_mirror_cluster, temp+0x38, 8);
	LENDIAN_TO_HOST64(ntfs->nbb.MFT_mirror_cluster);
	DEBUGPRINT(FS, 6, ("\tMFT Mirror cluster = %Ld.\n", ntfs->nbb.MFT_mirror_cluster));
	memcpy(&ntfs->nbb.clusters_per_file_record, temp+0x40, 1);
	DEBUGPRINT(FS, 6, ("\tclusters per file record = %d.\n", ntfs->nbb.clusters_per_file_record));
	memcpy(&ntfs->nbb.clusters_per_index_block, temp+0x44, 4);
	LENDIAN_TO_HOST32(ntfs->nbb.clusters_per_index_block);
	DEBUGPRINT(FS, 6, ("\tclusters per index block = %d.\n", ntfs->nbb.clusters_per_index_block));
	memcpy(&ntfs->nbb.volume_serial, temp+0x48, 8);
	LENDIAN_TO_HOST64(ntfs->nbb.volume_serial);	

	// calculate the mft record size
	if(ntfs->nbb.clusters_per_file_record>0)
		ntfs->mft_recordsize= ntfs->cluster_size*ntfs->nbb.clusters_per_file_record;
	else
		ntfs->mft_recordsize= 1 << (- ntfs->nbb.clusters_per_file_record);
	DEBUGPRINT(FS, 6, ("\tmft recordsize = %ld.\n", ntfs->mft_recordsize));

	// check to make sure the mft record size is sane
	// 16k seems reasonable, probably something like 4k is realistic
	// also, check to make sure it's a multiple of 512
	if((ntfs->mft_recordsize >= 1024*16) || (ntfs->mft_recordsize % 512)) {
		ERRPRINT(("ntfs_read_bios_block MFT record size > 16k, doesn't look good."));
		return EINVAL;
	}		

	DEBUGPRINT(FS, 5, ("ntfs_read_bios_block exit.\n"));

	return B_NO_ERROR;
}

void ntfs_initialize_vnode(vnode *new_vnode)
{
	memset(new_vnode, 0, sizeof(vnode));
}

void ntfs_deallocate_vnode(vnode *killit)
{
	attr_header *h, *next;
	
	DEBUGPRINT(FS, 7, ("ntfs_deallocate vnode called @ 0x%x\n", (unsigned int)killit));
	DEBUGPRINT(FS, 7, ("\tvnid is %Ld\n", killit->vnid));
	
	// Delete all of the attributes first
	h = killit->attr_list;
	while(h) {
		next = h->next;
		// Delete differently depending on the 
		switch(h->type) {
			case ATT_DATA:
			case ATT_INDEX_ALLOCATION:
				{
					data_attr *temp = (data_attr *)h;
					if(temp->name) ntfs_free(temp->name);
					if(temp->stored_local && temp->data) ntfs_free(temp->data);
					ntfs_free_extent(&temp->extent);
					break;
				}
			case ATT_INDEX_ROOT:
				{
					index_root_attr *temp = (index_root_attr *)h;
					if(temp->entries) ntfs_free(temp->entries);
					break;
				}
			case ATT_FILE_NAME:
				{
					file_name_attr *temp = (file_name_attr *)h;
					if(temp->name) ntfs_free(temp->name);
					break;	
				}
			default:
				break;
				// Doesn't matter
		}
		// Delete the attribute itself
		ntfs_free(h);	
		h = next;
	}

	ntfs_free(killit);
	
	DEBUGPRINT(FS, 7, ("ntfs_deallocate_vnode exit.\n"));
}

status_t ntfs_init_cache (ntfs_fs_struct *ntfs)
{	
	DEBUGPRINT(FS, 6, ("ntfs_init_cache entry.\n"));
	if (ntfs->fd<0)
		return B_ERROR;

	// our cache block size will be 1024 or cluster size, whichever is smaller
	ntfs->cache_block_size = 1024 <= ntfs->cluster_size ? 1024 : ntfs->cluster_size;
	ntfs->cache_blocks_per_cluster = ntfs->cluster_size / ntfs->cache_block_size;

	ntfs->num_cache_blocks = (ntfs->nbb.total_sectors / ntfs->nbb.sectors_per_cluster)
		* ntfs->cache_blocks_per_cluster;
	
	DEBUGPRINT(FS, 6, ("ntfs_init_cache initializing cache for %ld cache blocks ", (long int)ntfs->num_cache_blocks));
	DEBUGPRINT(FS, 6, ("of size %ld.\n", ntfs->cache_block_size));
	
	if (init_cache_for_device(ntfs->fd, ntfs->num_cache_blocks)<B_NO_ERROR)
		return B_ERROR;
	
	ntfs->cache_initialized = true;
	
	return B_NO_ERROR;
}
