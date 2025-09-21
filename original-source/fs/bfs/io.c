#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <Drivers.h>

#include "bfs.h"
#include "btlib/btree.h"

int
init_bfs_io(bfs_info *bfs)
{
	unsigned char *buff;

	if (bfs->dsb.block_size < 1024)
		return EINVAL;

	buff = (unsigned char *)malloc(bfs->dsb.block_size);
	if (buff == NULL)
		return ENOMEM;

	if (read_pos(bfs->fd, 0, buff, bfs->dsb.block_size) != bfs->dsb.block_size) {
		free(buff);
		return EINVAL;
	}

	bfs->original_bootblock = buff;

	return 0;
}


void
shutdown_bfs_io(bfs_info *bfs)
{
	if (bfs->original_bootblock) {
		free(bfs->original_bootblock);
		bfs->original_bootblock = NULL;
	}
}


ssize_t
read_blocks(bfs_info *bfs, dr9_off_t block_num, void *block, size_t nblocks)
{
	ssize_t ret;

/* printf("R 0x%x 0x%x\n", block_num, nblocks); */

	if (block_num >= bfs->dsb.num_blocks) {
		printf("Yikes! tried to read block %Ld but there are only %Ld\n",
			   block_num, bfs->dsb.num_blocks);
		return -1;
    }

	if (block_num < bfs->dsb.num_blocks &&
		block_num + nblocks > bfs->dsb.num_blocks) {

	    printf("Yikes! tried to read %d blocks starting at %Ld but only have "
			   "%Ld\n", nblocks, block_num, bfs->dsb.num_blocks);
		return -1;
    }

	ret = cached_read(bfs->fd,
					  block_num,
					  block,
					  nblocks,
					  bfs->dsb.block_size);

	if (ret == 0)
		return nblocks;
	else
		return -1;
}

ssize_t
write_blocks(bfs_info *bfs, dr9_off_t block_num, const void *block, size_t nblocks)
{
	ssize_t ret;

/* printf("W 0x%x 0x%x\n", block_num, nblocks); */

	if (bfs->flags & FS_READ_ONLY) {
		printf("write_super_block called on a read-only device! (bfs 0x%x)\n",
			   bfs);
		return -1;
	}

	if (block_num >= bfs->dsb.num_blocks) {
		printf("Yikes! tried to write block %Ld but there are only %Ld\n",
			   block_num, bfs->dsb.num_blocks);
		return -1;
	}

	if (block_num < bfs->dsb.num_blocks &&
		block_num + nblocks > bfs->dsb.num_blocks) {
		printf("Yikes! tried to write %d blocks starting at %Ld but only "
			   "have %Ld\n", nblocks, block_num, bfs->dsb.num_blocks);
		return -1;
	}

	ret = cached_write(bfs->fd,
					   block_num,
					   block,
					   nblocks,
					   bfs->dsb.block_size);

	if (ret == 0)
		return nblocks;
	else
		return -1;

	return ret;
}


int
read_super_block(bfs_info *bfs)
{
	char *buff;
	int   block_size;
	
	block_size = get_device_block_size(bfs->fd);
	if (block_size < 0)
		return EINVAL;
	bfs->dev_block_size = block_size;

	if (block_size < 1024)	
		block_size = 1024;

	buff = (char *)malloc(block_size);
	if (buff == NULL)
		return ENOMEM;

	if (read_pos(bfs->fd, 0, buff, block_size) != block_size) {
		free(buff);
		return EINVAL;
	}		

	/* look at the new location for the superblock first */
	if (((disk_super_block *)(buff+512))->magic1 == SUPER_BLOCK_MAGIC1) {
		memcpy(&bfs->dsb, buff+512, sizeof(disk_super_block));
	} else {
		memcpy(&bfs->dsb, buff, sizeof(disk_super_block));
		bfs->unsoiled_fs = 1;
	}

	free(buff);

	bfs->dev_block_size       = block_size;
	bfs->dev_block_conversion = bfs->dsb.block_size / block_size;

	return 0;
}

char music[] = "Kyuss Slo-Burn Monster Magnet Acid King Fu Manchu Sleep \
Black Sabbath Metallica Carcass KMFDM NIN Floater Banco De Gaia Children \
of the Bong Fates Warning Megadeth Cro-Mags Minor Threat Danzig Queensryche \
Brian Eno Kai Kln Sundial Iron Maiden DIO God Machine Verve Atomic Bitch Wax \
Drag Pack Rich,Alex&Ammar Altamont GammaRay B210 0x29A";

int
write_super_block(bfs_info *bfs)
{
	ssize_t  amt;

	if (bfs->flags & FS_READ_ONLY) {
		printf("write_super_block called on a read-only device! (bfs 0x%x)\n",
			   bfs);
		return EINVAL;
	}

	if (bfs->original_bootblock == NULL) {
		printf("original_bootblock not found! (bfs 0x%x)\n", bfs);
		return EINVAL;
	}

	if(ioctl(bfs->fd, B_FLUSH_DRIVE_CACHE) < 0) {
		//printf("could not flush drive cache (bfs 0x%x)\n", bfs);
	}

	bfs->dsb.flags = BFS_CLEAN;                          /* now it's clean! */
	if (bfs->unsoiled_fs)
		memcpy(bfs->original_bootblock, &bfs->dsb, sizeof(disk_super_block));
	else
		memcpy(bfs->original_bootblock+512, &bfs->dsb, sizeof(disk_super_block));

	amt = write_pos(bfs->fd, 0, bfs->original_bootblock, bfs->dsb.block_size);
	
	if (amt == bfs->dsb.block_size)
		return 0;
	else
		return -1;
}
