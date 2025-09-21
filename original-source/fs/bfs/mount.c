#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <Errors.h>

#include "bfs.h"
#include "btlib/btree.h"

#ifndef USER
#include "bfs_vnops.h"
#endif

int
super_block_is_sane(bfs_info *bfs)
{
	dr9_off_t num_dev_blocks;
	int block_size;

	if (bfs->dsb.magic1 != SUPER_BLOCK_MAGIC1 ||
		bfs->dsb.magic2 != SUPER_BLOCK_MAGIC2 ||
		bfs->dsb.magic3 != SUPER_BLOCK_MAGIC3) {

		printf("warning: super block magic numbers are wrong:\n");
		printf("0x%x (0x%x) 0x%x (0x%x) 0x%x (0x%x)\n",
			   bfs->dsb.magic1, SUPER_BLOCK_MAGIC1,
			   bfs->dsb.magic2, SUPER_BLOCK_MAGIC2,
			   bfs->dsb.magic3, SUPER_BLOCK_MAGIC3);
		return EBADF;
	}

	if ((bfs->dsb.block_size % bfs->dev_block_size) != 0) {
		printf("warning: fs block size %d not a multiple of ",
				bfs->dsb.block_size);
		printf(" device block size %d\n", bfs->dev_block_size);

		return EBADF;
	}

	block_size = get_device_block_size(bfs->fd);
	if (block_size == 0) {
		printf("warning: could not fetch block size\n");
		return EBADF;
	}

	/* make sure that the partition is as big as the super block
       says it is */
	num_dev_blocks = get_num_device_blocks(bfs->fd);
	if (bfs->dsb.num_blocks * bfs->dsb.block_size >
				num_dev_blocks * block_size) {
		printf("warning: fs blocks %Lx larger than device blocks %Lx\n",
				bfs->dsb.num_blocks * (bfs->dsb.block_size/block_size),
				num_dev_blocks);
		return B_PARTITION_TOO_SMALL;
	}

	if (bfs->dsb.block_size != (1 << bfs->dsb.block_shift)) {
		int i;
		
		printf("warning: block_shift %d does not match block size %d\n",
			   bfs->dsb.block_shift, bfs->dsb.block_size);

		if (bfs->dsb.block_shift > 8 && bfs->dsb.block_shift < 16) {
			printf("setting block_size to %d\n", (1 << bfs->dsb.block_shift));
			bfs->dsb.block_size = (1 << bfs->dsb.block_shift);
		} else {
			for(i=0; i < sizeof(int) * 8; i++)
				if ((1 << i) == bfs->dsb.block_size)
					break;

			if (i >= sizeof(int) * 8 || i > 16) {
				printf("neither block_size nor block_shift make sense!\n");
				return EBADF;
			}

			bfs->dsb.block_shift = i;
			printf("setting block_shift to %d\n", i);
		}
	}

	/* XXXdbg - need more super-block sanity checks here */

	return B_OK;
}


void
check_for_vol_id(bfs_info *bfs, bfs_inode *bi)
{
	long long volid;
	off_t     offset = 0;
	size_t    len = sizeof(volid);
	status_t  x;

	x = internal_read_attr(bfs, bi, "be:volume_id",B_UINT64_TYPE,&volid,&len,0);
	if (x == ENOENT) {
		volid = (system_time() + (bi->create_time * 1000000)) ^ bi->create_time;
		len = sizeof(volid);
		x = bfs_write_attr(bfs, bi, "be:volume_id", B_UINT64_TYPE,
						   &volid, &len, 0);
	}
}



int
bfs_mount(nspace_id nsid, const char *device, ulong flags,
                    void *parms, size_t len, void **data, vnode_id *vnid)
{
	int        ret = 0, oflags = O_RDWR, is_removable;
	char       buff[128];
	bfs_info  *bfs;
	struct stat st;

	bfs = (bfs_info *)calloc(sizeof(bfs_info), 1);
	if (bfs == NULL) {
		printf("no memory for bfs structure!\n");
		return ENOMEM;
	}

	bfs->nsid = nsid;
	*data = (void *)bfs;


	sprintf(buff, "bfs:%s", device);
	bfs->sem = create_sem(MAX_READERS, buff);
	if (bfs->sem < 0) {
		printf("could not create bfs sem!\n");
		ret = ENOMEM;
		goto error0;
	}
	
	if ((flags & B_MOUNT_READ_ONLY) || device_is_read_only(device)) {
		printf("bfs: %s is read-only!\n", device);
		oflags = O_RDONLY;
		bfs->flags |= FS_READ_ONLY;
	}

	bfs->fd = open(device, oflags);
	if (bfs->fd < 0) {
		printf("could not open %s to try and mount a bfs\n", device);
		ret = ENODEV;
		goto error1;
	}
	if(oflags != O_RDONLY) {
		/* check that we can actually write to the device, in case
		   it did not report itself as read-only */
		uint8 *bitmap_block;
		off_t test_pos = 1024;
		int	dev_block_size = get_device_block_size(bfs->fd);

		if(test_pos < dev_block_size)
			test_pos = dev_block_size;
		bitmap_block = malloc(dev_block_size);
		
		if(bitmap_block == NULL) {
			ret = ENOMEM;
			goto error1_1;
		}
		if(read_pos(bfs->fd, test_pos, bitmap_block, dev_block_size) < dev_block_size) {
			printf("could not read from %s!\n", device);
			ret = EBADF;
			free(bitmap_block);
			goto error1_1;
		}
		if(write_pos(bfs->fd, test_pos, bitmap_block, dev_block_size) < dev_block_size) {
			printf("could not write to %s, mount as read only instead\n", device);
			oflags = O_RDONLY;
			bfs->flags |= FS_READ_ONLY;
			close(bfs->fd);
			bfs->fd = open(device, oflags);
			if (bfs->fd < 0) {
				printf("could not open %s to try and mount a bfs\n", device);
				ret = ENODEV;
				free(bitmap_block);
				goto error1;
			}
		}
		free(bitmap_block);
	}

	if (fstat(bfs->fd, &st) < 0) {
		printf("could not stat %s to determine if it is a device!\n", device);
		ret = EBADF;
		goto error1_1;
	}

#if defined(__BEOS__) && !defined(USER)
	if (S_ISREG(st.st_mode)) {
		/* it's a reglar file: make access to it uncached! */
		if (ioctl(bfs->fd, 10000, NULL) < 0) {
			printf("bfs mount warning: could not make file access uncached\n");
			printf("\tthis could be dangerous...\n");
		}
	}
#endif

	if (read_super_block(bfs) != 0) {
		printf("could not read super block on device %s\n", device);
		ret = EBADF;
		goto error1_1;
	}
		
	if ((ret = super_block_is_sane(bfs)) != B_OK) {
		printf("bad super block\n");
		goto error1_1;
	}

	if (strncmp(bfs->dsb.name, "__ro__", 6) == 0) {
		printf("BFS: simulating a read-only volume because of volume name\n");
		bfs->flags |= FS_READ_ONLY;
	}

	if (init_cache_for_device(bfs->fd, bfs->dsb.num_blocks) != 0) {
		printf("could not initialize cache access for fd %d\n", bfs->fd);
		ret = EBADF;
		goto error1_1;
	}

	if (init_tmp_blocks(bfs) != 0) {
		printf("could not init tmp blocks\n");
		ret = ENOMEM;
		goto error2;
	}

	if (init_bfs_io(bfs) != 0) {
		printf("could not init bfs io\n");
		ret = EBADF;
		goto error3;
	}

	if (init_log(bfs) != 0) {
		printf("could not initialize the log\n");
		ret = EBADF;
		goto error4;
	}

	if (init_inodes(bfs) != 0) {
		printf("could not initialize inodes\n");
		ret = ENOMEM;
		goto error5;
	}

	if (init_block_bitmap(bfs) != 0) {
		printf("could not initialize the block bitmap\n");
		ret = EBADF;
		goto error6;
	}

	if (init_indices(bfs) != 0) {
		printf("could not initialize the index tables\n");
		ret = EBADF;
		goto error7;
	}

	*vnid = inode_addr_to_vnode_id(bfs->dsb.root_dir);
	if (bfs_read_vnode(bfs, *vnid, 0, (void **)&bfs->root_dir) != 0) {
		printf("could not read root dir inode\n");
		ret = EBADF;
		goto error8;
	}

	if ((bfs->flags & FS_READ_ONLY) == 0)      /* make sure there is a vol-id */
		check_for_vol_id(bfs, bfs->root_dir);

	is_removable = device_is_removeable(bfs->fd);

	if (is_removable)
		bfs->flags |= FS_IS_REMOVABLE;

	if ((bfs->flags & FS_READ_ONLY) == 0 && is_removable)
		lock_removeable_device(bfs->fd, 1);

	if (new_vnode(bfs->nsid, *vnid, (void *)bfs->root_dir) != 0) {
		printf("could not initialize a vnode for the root directory!\n");
		ret = ENOMEM;
		goto error9;
	}

	init_debugging(bfs);

#ifndef USER
	printf("bfs @ 0x%x (%s) on %s\n", bfs, bfs->dsb.name, device);
#endif	
	return 0;


 error9:
	if ((bfs->flags & FS_READ_ONLY) == 0 && device_is_removeable(bfs->fd))
		lock_removeable_device(bfs->fd, 0);
 error8:
	shutdown_indices(bfs);
 error7:
	shutdown_block_bitmap(bfs);
 error6:
	shutdown_inodes(bfs);
 error5:
	shutdown_log(bfs);
 error4:
	shutdown_bfs_io(bfs);
 error3:
	shutdown_tmp_blocks(bfs);
 error2:
	remove_cached_device_blocks(bfs->fd, NO_WRITES);
 error1_1:
	close(bfs->fd);
 error1:
	delete_sem(bfs->sem);
 error0:
	memset(bfs, 0xff, sizeof(*bfs));   /* yeah, I'm paranoid */
	free(bfs);

	return ret;
}


/*
   note that the order in which things are done here is *very*
   important. don't mess with it unless you know what you're doing
*/   
int
bfs_unmount(void *ns)
{
	bfs_info *bfs = (bfs_info *)ns;
	
	if (bfs == NULL)
		return EINVAL;
	
	sync_log(bfs);

	shutdown_indices(bfs);
	shutdown_block_bitmap(bfs);
	shutdown_inodes(bfs);

	/*
	   have to do this after the above steps because the above steps
	   might actually have to do transactions
	*/
	sync_log(bfs);

	remove_cached_device_blocks(bfs->fd, ALLOW_WRITES);
	shutdown_log(bfs);

	if ((bfs->flags & FS_READ_ONLY) == 0)
		write_super_block(bfs);
	shutdown_bfs_io(bfs);

	shutdown_tmp_blocks(bfs);

	if ((bfs->flags & FS_READ_ONLY) == 0 && device_is_removeable(bfs->fd))
		lock_removeable_device(bfs->fd, 0);

	close(bfs->fd);
	
	if (bfs->sem > 0)
		delete_sem(bfs->sem);

	memset(bfs, 0xff, sizeof(*bfs));   /* trash it just to be sure */
	free(bfs);

	return 0;
}




int
bfs_initialize(const char *devname, void *parms, size_t len)
{
	return EINVAL;
}
