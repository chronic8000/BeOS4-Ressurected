#include <Drivers.h>
#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include <SupportDefs.h>
#include <malloc.h>
#include <string.h>
typedef off_t dr9_off_t;
#include <lock.h>
#include <cache.h>
#include <KernelExport.h>
#include <errno.h>

static status_t
read_sb1(cfs_info *fsinfo)
{
	cfs_super_block_1 sb1;

	if(read_pos(fsinfo->dev_fd, cfs_superblock_1_offset, &sb1, sizeof(sb1)) <
	   sizeof(sb1))
		return B_FILE_ERROR;
	if(sb1.cfs_id_1 != CFS_ID_1)
		return B_FILE_ERROR;
	if(sb1.cfs_version > CFS_CURRENT_VERSION) {
		PRINT_ERROR(("cfs: version # (%ld) is too high. Max supported is %d\n",
			sb1.cfs_version, CFS_CURRENT_VERSION));
		return B_FILE_ERROR;
	}
	if(sb1.cfs_version < CFS_MIN_SUPPORTED_VERSION) {
		PRINT_ERROR(("cfs: version # (%ld) is too low. Min supported is %d\n",
			sb1.cfs_version, CFS_MIN_SUPPORTED_VERSION));
		return B_FILE_ERROR;
	}
	if(sb1.dev_block_size <= 0) {
		PRINT_ERROR(("cfs: invalid dev block size %ld\n", sb1.dev_block_size));
		return B_FILE_ERROR;
	}
	if(sb1.fs_block_size <= 0) {
		PRINT_ERROR(("cfs: invalid fs block size %ld\n", sb1.fs_block_size));
		return B_FILE_ERROR;
	}
	if(sb1.fs_block_size % sb1.dev_block_size != 0) {
		PRINT_ERROR(("cfs: fs block size (%ld) is not a multiple of dev "
		             "block size (%ld)\n", sb1.fs_block_size,
		             sb1.dev_block_size));
		return B_FILE_ERROR;
	}
	if(sb1.size % sb1.fs_block_size != 0) {
		PRINT_ERROR(("cfs: fs size (%Ld) is not divisible by %ld\n",
		             sb1.size, sb1.fs_block_size));
		return B_FILE_ERROR;
	}
	if(sb1.super_block_2 % sb1.fs_block_size != 0) {
		PRINT_ERROR(("cfs: superblock2 location (%Ld) is not divisible by %ld\n",
		             sb1.super_block_2, sb1.fs_block_size));
		return B_FILE_ERROR;
	}
	if(sb1.flags & CFS_SB1_FLAGS_FS_READ_ONLY)
		fsinfo->fs_flags |= B_FS_IS_READONLY;
	if((fsinfo->fs_flags & B_FS_IS_READONLY) == 0 &&
	   (sb1.log_pos == 0 || sb1.log_size == 0)) {
		PRINT_ERROR(("cfs: cannot mount writeable filesystem without log\n"));
		return B_FILE_ERROR;
	}

	fsinfo->fs_version = sb1.cfs_version;
	fsinfo->_dev_block_size = sb1.dev_block_size;
	fsinfo->cache_block_size = sb1.fs_block_size;
	fsinfo->create_time = sb1.create_time;
	fsinfo->size = sb1.size;
	fsinfo->super_block_2_pos = sb1.super_block_2;
	fsinfo->root_pos = sb1.root;
	fsinfo->reserved_space_pos = sb1.reserved_space_1_pos;
	fsinfo->reserved_space_size = sb1.reserved_space_1_size;
	
	if(sb1.log_pos == 0 || sb1.log_size == 0) {
		fsinfo->log_base = 0;
		fsinfo->log_size = 0;
		fsinfo->log_info_pos = NULL;
		fsinfo->log_info_count = 0;
	}
	else {
		fsinfo->log_base = sb1.log_pos + fsinfo->_dev_block_size;
		fsinfo->log_size = sb1.log_size - 2 * fsinfo->_dev_block_size;
		fsinfo->log_info_pos = malloc(sizeof(off_t) * 2);
		if(fsinfo->log_info_pos == NULL)
			return B_NO_MEMORY;
		fsinfo->log_info_pos[0] = sb1.log_pos;
		fsinfo->log_info_pos[1] = fsinfo->log_base + fsinfo->log_size;
		fsinfo->log_info_count = 2;
	}
	return B_NO_ERROR;
}

status_t
cfs_read_superblock2(cfs_info *fsinfo, cfs_super_block_2 *sb2)
{
	status_t err;
	
	sb2->free_list_end = 0;
	err = cfs_read_disk(fsinfo, fsinfo->super_block_2_pos, sb2, get_sb2_size(fsinfo->fs_version));
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_read_superblock2: could not read superblock, %s\n",
		             strerror(err)));
		return err;
	}
	if(sb2->cfs_id_2 != CFS_ID_2) {
		PRINT_ERROR(("cfs_read_superblock2: superblock2 id wrong\n"));
		return B_IO_ERROR;
	}
	if(sb2->cfs_version != fsinfo->fs_version) {
		PRINT_ERROR(("cfs_read_superblock2: version # (%ld) does not match main superblock (%ld)\n",
			sb2->cfs_version, fsinfo->fs_version));
		return B_IO_ERROR;
	}
		
	return B_NO_ERROR;
}

static status_t
read_sb2(cfs_info *fsinfo)
{
	status_t err;
	cfs_super_block_2 sb2;

	err = cfs_read_superblock2(fsinfo, &sb2);
	if(err != B_NO_ERROR)
		return err;

	/* sb2.flags; no flags yet */
	fsinfo->modification_time = sb2.modification_time;
	strncpy(fsinfo->name, sb2.volume_name, sizeof(fsinfo->name) - 1);
	fsinfo->name[sizeof(fsinfo->name) - 1] = 0;

	return B_NO_ERROR;
}

int
cfs_mount(nspace_id nsid, const char *devname, ulong flags, void *parms,
          size_t len, void **data, vnode_id *vnid)
{
	status_t err;
	cfs_info *info;
	device_geometry dg;
	struct stat st;
	bool dg_valid = false;
	
	PRINT_FLOW(("cfs_mount dev %s\n", devname));
	PRINT_FLOW(("cfs_mount: fsdriver version 2\n"));

#ifdef USE_DMALLOC
	dprintf("cfs_unmount: checking memory\n");
	check_mem();
#endif

	err = B_NO_MEMORY;
	info = malloc(sizeof(cfs_info));
	if(info == NULL)
		goto err0;
	info->nsid = nsid;
	info->fs_flags = B_FS_IS_PERSISTENT
	               | B_FS_HAS_MIME
	               | B_FS_HAS_ATTR
	            /* | B_FS_HAS_QUERY */;
	if(flags & B_MOUNT_READ_ONLY)
		info->fs_flags |= B_FS_IS_READONLY;
	info->dev_fd = open(devname, O_RDONLY);
	if(info->dev_fd < B_NO_ERROR) {
		err = errno;
		goto err1;
	}
	if(ioctl(info->dev_fd , B_GET_GEOMETRY, &dg) >= 0) {
		dg_valid = true;
		if(dg.read_only) {
			info->fs_flags |= B_FS_IS_READONLY;
		}
	}
	if (fstat(info->dev_fd, &st) < 0) {
		PRINT_ERROR(("could not stat %s to determine if it is a device!\n",
		             devname));
		err = EBADF;
		goto err2;
	}
	if (S_ISREG(st.st_mode)) {
		/* it's a reglar file: make access to it uncached! */
		PRINT_INFO(("cfs mounting file, try to disable cache\n"));
		if (ioctl(info->dev_fd, 10000, NULL) < 0) {
			PRINT_WARNING(("cfs mount warning: could not make file access "
			               "uncached\n"));
			err = EBADF;
			goto err2;
		}
	}

	info->logged_transactions = NULL;
	info->current_transaction = NULL;
	info->last_entry_version = 0;
	err = read_sb1(info);
	if(err < B_NO_ERROR)
		goto err2;
	PRINT_INFO(("cfs_mount: fs size %Ld\n", info->size));

	if(dg_valid && info->_dev_block_size < dg.bytes_per_sector) {
		PRINT_WARNING(("cfs_mount: fs blocksize (%ld) is less than the "
		               "device block size (%ld)\n",
		               info->_dev_block_size, dg.bytes_per_sector));
	}

	if((info->fs_flags & B_FS_IS_READONLY) == 0) {
		int rwfd = open(devname, O_RDWR);
		if(rwfd < 0) {
			PRINT_WARNING(("could not reopen %s read/write, mounting "
			               "filesystem read-only\n", devname));
			info->fs_flags |= B_FS_IS_READONLY;
		}
		else {
			close(info->dev_fd);
			info->dev_fd = rwfd;
			if(S_ISREG(st.st_mode)) {
				if(ioctl(info->dev_fd, 10000, NULL) < 0) {
					PRINT_WARNING(("cfs mount warning: could not make file "
					               "access uncached\n"));
					err = EBADF;
					goto err3;
				}
			}
		}
	}

	err = info->read_write_lock = create_sem(CFS_MAX_READERS, "cfs_lock");
	if(err < B_NO_ERROR)
		goto err3;

	err = info->blocklist_loader_lock = create_sem(1, "cfs_blocklist_loader_lock");
	if(err < B_NO_ERROR)
		goto err3a;

	// initialize decompression buffer data
	info->last_decompression_offset = 0;
	info->decompression_buf = NULL;		
	new_lock(&info->decompression_buf_lock, "cfs_decompression_buf_lock");
	if(info->decompression_buf_lock.s < 0)
		goto err4;		

	// Initialize the free space counter
	info->free_space_count = -1;
		
info->debug_read_lock_held = 0;
info->debug_write_lock_held = 0;

	err = init_cache_for_device(info->dev_fd,
	                            info->size / info->cache_block_size);
	if(err < B_NO_ERROR)
		goto err4a;
	
	err = cfs_init_log(info);
	if(err != B_NO_ERROR)
		goto err5;
	
	if((info->fs_flags & B_FS_IS_READONLY) == 0) {
		err = cfs_cleanup_unlinked_entries(info);
		if(err != B_NO_ERROR)
			goto err6;
	}
	
	err = cfs_lock_read(info);
	if(err < B_NO_ERROR)
		goto err6;

	err = read_sb2(info);
	if(err != B_NO_ERROR)
		goto err7;

	err = cfs_read_vnode(info, info->root_pos, 1, (void**)&info->root_dir);
	if(err < B_NO_ERROR)
		goto err7;

	err = new_vnode(nsid, info->root_pos, (void *)info->root_dir);
	if(err < B_NO_ERROR)
		goto err8;

	cfs_unlock_read(info);
	*vnid = info->root_pos;
	*data = info;
	PRINT_FLOW(("cfs_mount: done, root id %Ld data %p\n", info->root_pos, info));
	return B_NO_ERROR;

err8:	cfs_write_vnode(info, info->root_dir, 1);
err7:	cfs_unlock_read(info);
err6:	cfs_uninit_log(info);
err5:	remove_cached_device_blocks(info->dev_fd, NO_WRITES);
err4a:  free_lock(&info->decompression_buf_lock);
err4:	delete_sem(info->blocklist_loader_lock);
err3a:	delete_sem(info->read_write_lock);
err3:	free(info->log_info_pos);
err2:	close(info->dev_fd);
err1:	free(info);
err0:	PRINT_FLOW(("cfs_mount dev %s failed, %s\n", devname, strerror(err)));
		if(err >= B_NO_ERROR)
			err = B_ERROR;
		return err;
}

int
cfs_unmount(void *ns)
{
	status_t err;
	cfs_info *fsinfo = ns;

	PRINT_FLOW(("cfs_unmount %p\n", ns));
	
	if(fsinfo->debug_read_lock_held != 0 ||
	   fsinfo->debug_write_lock_held != 0) {
		PRINT_ERROR(("cfs_unmount %p, lock count not 0, read %ld write %ld\n",
		             ns, fsinfo->debug_read_lock_held,
		             fsinfo->debug_write_lock_held));
	}
	err = cfs_lock_write(fsinfo);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_unmount %p: lock failed\n", ns));
	}
	else {
		err = cfs_flush_log(fsinfo);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_unmount %p: cfs_flush_log failed\n", ns));
		}
		err = cfs_flush_log_blocks(fsinfo);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_unmount %p: cfs_flush_log_blocks failed\n", ns));
		}
		cfs_unlock_write(fsinfo);
	}
	cfs_uninit_log(fsinfo);
	remove_cached_device_blocks(fsinfo->dev_fd, ALLOW_WRITES);
	//remove_cached_device_blocks(fsinfo->dev_fd, NO_WRITES);
	//flush_device(info->dev_fd, true);
	free_lock(&fsinfo->decompression_buf_lock);
	delete_sem(fsinfo->read_write_lock);
	delete_sem(fsinfo->blocklist_loader_lock);
	if(fsinfo->decompression_buf != NULL) free(fsinfo->decompression_buf);
	if(fsinfo->log_info_pos != NULL) free(fsinfo->log_info_pos);
	close(fsinfo->dev_fd);
	free(ns);

#ifdef USE_DMALLOC
	dprintf("cfs_unmount: checking memory\n");
	check_mem();
#endif

	return B_NO_ERROR;
}

int 
cfs_sync(void *ns)
{
	status_t err, tmp_err;
	cfs_info *fsinfo = ns;

	PRINT_FLOW(("cfs_sync %p\n", ns));

	//flush_device(fsinfo->dev_fd, true);
	err = cfs_lock_write(fsinfo);
	if(err != B_NO_ERROR) {
		return err;
	}
	err = cfs_flush_log(fsinfo);
	tmp_err = cfs_flush_log_blocks(fsinfo);
	cfs_unlock_write(fsinfo);
	if(tmp_err != B_NO_ERROR)
		return tmp_err;
	return err;
}

