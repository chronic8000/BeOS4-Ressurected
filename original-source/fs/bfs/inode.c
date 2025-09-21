#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "bfs.h"
#include "inode.h"


int
init_inodes(bfs_info *bfs)
{
	return 0;
}


void
shutdown_inodes(bfs_info *bfs)
{
}



bfs_inode *
allocate_inode(bfs_info *bfs, bfs_inode *parent, int mode)
{
	dr9_off_t   ag, num_blocks, bnum;
	block_run   br;
	bfs_inode  *bi;
	small_data *sd;


	if (parent) {
		ag = parent->inode_num.allocation_group;

		/* new directories (but not indices) go to the next ag */
		if (S_ISDIR(mode) && (mode & S_INDEX_DIR) == 0) 
			ag = (ag + 8) % bfs->dsb.num_ags;
	} else {
		ag = 0;
	}
	
	num_blocks = bfs->dsb.inode_size / bfs->dsb.block_size;


	if (allocate_block_run(bfs, ag, num_blocks, &br, EXACT_ALLOCATION) != 0) {
		return NULL;
	}

	if (br.allocation_group == 0 && br.start == 0)
		bfs_die("alloc_inode: br %d:%d:%d is bullshit.\n", br.allocation_group,
				br.start, br.len);

#if 0
	if (check_block_run(bfs, br) == 0)   /* XXXdbg definitely remove! */
		bfs_die("allocated br %d:%d:%d but check_block_run sez it's not!\n",
				br.allocation_group, br.start, br.len);
#endif

	bnum = block_run_to_bnum(br);
	
	bi = get_empty_block(bfs->fd, bnum, bfs->dsb.block_size);
	/* printf("AI: %d:%d\n", bnum, num_blocks); */
	if (bi == NULL) {
		free_block_run(bfs, &br);
		return NULL;
	}

	memset(bi, 0, sizeof(bfs_inode));

	bi->inode_num          = br;
	bi->inode_size         = bfs->dsb.inode_size;
	bi->create_time        = CUR_TIME;
	bi->last_modified_time = bi->create_time;
	bi->flags              = INODE_IN_USE | INODE_VIRGIN;
	bi->magic1             = INODE_MAGIC1;

	sd = (small_data *)&bi->small_data[0];
	memset(sd, 0, sizeof(sd));
	sd->data_size = bfs->dsb.inode_size -
		            sizeof(bfs_inode)   -
		            sizeof(small_data)  +
				    sizeof(int);  /* account for the last int in bfs_inode */

	return bi;
}



int
free_inode(bfs_info *bfs, inode_addr *ia)
{
	/* XXXdbg -- should we read the data on disk, zero it and write it back? */

	if (free_block_run(bfs, ia) != 0)
		printf("failed to free inode @ %d %d %d\n", ia->allocation_group,
			   ia->start, ia->len);

	return 0;
}




int
update_inode(bfs_info *bfs, bfs_inode *bi)
{
	int         num_blocks;
	dr9_off_t       bnum;

	bnum       = block_run_to_bnum(bi->inode_num);
	num_blocks = bi->inode_num.len;

	if (bnum == 0)
		bfs_die("bi @ 0x%x has a zero inode num?!?\n", bi);

	if (num_blocks != (bfs->dsb.inode_size / bfs->dsb.block_size))
		printf("inode size discrepancy %d != %d\n", bi->inode_num.len,
			   bfs->dsb.inode_size / bfs->dsb.block_size);
	
	if (bi->inode_size != bfs->dsb.inode_size) {
		bfs_die("*** update: bad size, inode %d %d %d is corrupt\n",
			   bi->inode_num.allocation_group, bi->inode_num.start,
			   bi->inode_num.len);
	}

	if ((bi->flags & INODE_IN_USE) == 0) {
		bfs_die("inode %d %d %d does not have IN_USE flag set\n",
				bi->inode_num.allocation_group, bi->inode_num.start,
				bi->inode_num.len);
	}

	/* printf("UI: %d:%d\n", bnum, num_blocks); */
	if (log_write_blocks(bfs, bfs->cur_lh, bnum, (char *)bi,
						 num_blocks, 1) != num_blocks) {
		return EINVAL;
	}

	return 0;
}

