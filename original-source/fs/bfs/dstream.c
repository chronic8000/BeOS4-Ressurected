#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <memory.h>

#include "bfs.h"


#define MIN_DBL_IND_ALLOCATION_SIZE		(4 * 1024)

#define SIMPLE_CHECK_BNUM(bnum) {                                             \
                           if (bnum > bfs->dsb.num_blocks || bnum <= 0)       \
                               bfs_die("bad bnum 0x%Lx (max 0x%Lx) @ %s:%d\n",\
									   bnum, bfs->dsb.num_blocks,             \
									   __FILE__, __LINE__);                   \
                         }

#define CHECK_BNUM_WITH_CLEANUP(bnum, cleanup_label) {                        \
                           if (bnum > bfs->dsb.num_blocks || bnum <= 0) {     \
                               bfs_die("bad bnum 0x%Lx (max 0x%Lx) bi 0x%x @ "\
									   "%s:%d\n",bnum, bfs->dsb.num_blocks,   \
									   bi, __FILE__, __LINE__);               \
							   goto cleanup_label;                            \
						   }                                                  \
                         }

#define CHECK_BNUM(bnum) {                                                    \
                           if (bnum > bfs->dsb.num_blocks || bnum <= 0)       \
                               bfs_die("bad bnum 0x%Lx (max 0x%Lx) bi 0x%x @ "\
									   "%s:%d\n",bnum, bfs->dsb.num_blocks,   \
									   bi, __FILE__, __LINE__);               \
                         }

#define BLOCK_SIZE_ROUND_UP(x)     \
             (((off_t)(x) + (off_t)(bfs->dsb.block_size-1)) & ~(off_t)(bfs->dsb.block_size-1))

#define BLOCK_SIZE_ROUND_DOWN(x)   \
             (((off_t)(x) & ~(off_t)(bfs->dsb.block_size-1)))


#define clear_block_run_fields(br) \
	(br).allocation_group = (br).start = (br).len = 0


typedef struct dbl_indirect_pos {
	unsigned int dbl_ind_block_index;
	unsigned int ind_br_index;
	unsigned int ind_block_index;
	unsigned int br_index;
	int error;
} dbl_indirect_pos;

static int grow_indirect_range(bfs_info *bfs, bfs_inode *bi,
							   block_run *data_br_ptr, char *block, int *last);
static int get_min_dbl_ind_alloc_size(bfs_inode *bi);
static int shrink_dbl_indirect(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size);
static dr9_off_t allocate_indirect_pages(struct bfs_info *bfs,
										 struct bfs_inode *bi,
										 struct block_run *ind,
										 struct block_run *d, int *err);
static void get_dbl_indirect_pos(bfs_info *bfs, bfs_inode *bi,
								 dr9_off_t offset, dbl_indirect_pos *pos);
static int shrink_indirect(bfs_info *bfs, bfs_inode *bi,
						   dr9_off_t rounded_size);

static int free_indirect_block_run(bfs_info *bfs, bfs_inode *bi,
								   block_run *ind_br_ptr, int bsize,
								   int start, int do_log);
#define WRITE_LOG 1
#define NO_LOG    0


static int
search_indirect_page(block_run *map, int bsize,
					 dr9_off_t bnum, dr9_off_t *sumptr, block_run *br)
{
	int   i;
	
	for(i=0; i < bsize / sizeof(block_run); *sumptr+=map->len, map++, i++) {
		if (map->start == 0 && map->len == 0)   /* then nothing more to do */
			break;

		if (bnum >= *sumptr && bnum < *sumptr + map->len) {
			br->allocation_group = map->allocation_group;
			br->start            = map->start + (bnum - *sumptr);
			br->len              = map->len   - (bnum - *sumptr);
			
			return 0;
		}
	}
	return ENOENT;
}

static int
file_pos_to_block_run(bfs_info *bfs, dr9_off_t pos, bfs_inode *bi, block_run *br)
{
	int    i, ret, bsize = bfs->dsb.block_size,
		   no_cache = bi->flags & INODE_NO_CACHE;
	dr9_off_t  bnum, sum;
	char  *block = NULL, *block2 = NULL;
	data_stream *ds = &bi->data;
	my_hash_table *ht = bi->etc->ht;
	
	bnum = pos / (dr9_off_t) bsize;
	
	if (pos < ds->max_direct_range) {
		sum = 0;
		for(i=0; i < NUM_DIRECT_BLOCKS; sum += ds->direct[i].len, i++) {
			if (ds->direct[i].start == 0 && ds->direct[i].len == 0)
				break;

			if (bnum >= sum && bnum < sum + ds->direct[i].len) {
				br->allocation_group = ds->direct[i].allocation_group;
				br->start            = ds->direct[i].start + (bnum - sum);
				br->len              = ds->direct[i].len   - (bnum - sum);

				return 0;
			}
		}

		if (i < NUM_DIRECT_BLOCKS)
			return ENOENT;
	} 

	
	if (pos < ds->max_indirect_range) {
		dr9_off_t tmp;
		
		sum = ds->max_direct_range / bsize;

		tmp = block_run_to_bnum(ds->indirect);
		for (i=0; i < ds->indirect.len; i++) {
			block_run *map;

			CHECK_BNUM_WITH_CLEANUP(tmp+i, ind_cleanup0);
			if (no_cache)
				block = hash_lookup(ht, tmp+i);
			else
				block = get_block(bfs->fd, tmp+i, bsize);
			
			if (block == NULL) {
				printf("failed here block %Ld pos %Ld\n", tmp+i, pos);
				return EINVAL;
			}

			map = (block_run *)block;
			if (map->start == 0  &&  map->len == 0) {
				/* nothing more to search here */
				if (no_cache == 0)
					release_block(bfs->fd, tmp+i);
				block = NULL;
				break;
			}
				
			ret = search_indirect_page((block_run *)block, bsize, bnum, &sum, br);
			if (no_cache == 0)
				release_block(bfs->fd, tmp+i);
			block = NULL;
			
			if (ret == 0)
				return 0;
		}

		return ENOENT;

	ind_cleanup0:
		if (block && no_cache == 0 && i < ds->indirect.len)
			release_block(bfs->fd, tmp+i);
		return ENOENT;
	} 



	if (pos < ds->max_double_indirect_range) {
		dbl_indirect_pos dbl_ind_pos;
		dr9_off_t b, dbl_ind_bnum, ind_bnum;
		dr9_off_t offset;
		block_run *ind_br_ptr, *br_ptr;
		int dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);
		unsigned int ind_block_index, ind_br_index,
			br_index, dbl_ind_block_index, tmp;

		offset = pos - ds->max_indirect_range; 
		get_dbl_indirect_pos(bfs, bi, offset, &dbl_ind_pos);
		if (dbl_ind_pos.error != 0) {
			return dbl_ind_pos.error;
		}

		dbl_ind_block_index = dbl_ind_pos.dbl_ind_block_index;
		ind_br_index = dbl_ind_pos.ind_br_index;
		ind_block_index = dbl_ind_pos.ind_block_index;
		br_index = dbl_ind_pos.br_index;

		/* 
		 * Now that we have all the indices, get the correct block run.
		 */

		b = block_run_to_bnum(ds->double_indirect);
		dbl_ind_bnum = b+dbl_ind_block_index;
		CHECK_BNUM_WITH_CLEANUP(dbl_ind_bnum, dbl_cleanup0);
		if (no_cache)
			block = hash_lookup(ht, dbl_ind_bnum);
		else
			block = get_block(bfs->fd, dbl_ind_bnum, bsize);
		if (block == NULL) {
			return EINVAL;
		}


		ind_br_ptr = &((block_run *) block)[ind_br_index];
		if (ind_br_ptr->len != dbl_ind_alloc_size/bsize) {
			bfs_die("*** bad ind. block run, pos %Ld ds @ 0x%lx, br @ 0x%lx\n",
					pos, (ulong) ds, ind_br_ptr);
		}

		b = block_run_to_bnum(*ind_br_ptr);
		ind_bnum = b + ind_block_index;
		CHECK_BNUM_WITH_CLEANUP(ind_bnum, dbl_cleanup1);

		if (no_cache == 0)
			release_block(bfs->fd, dbl_ind_bnum);
		block      = NULL;
		ind_br_ptr = NULL;

		if (no_cache)
			block2 = hash_lookup(ht, ind_bnum);
		else
			block2 = get_block(bfs->fd, ind_bnum, bsize);
		if (block2 == NULL) {
			return EINVAL;
		}

		br_ptr = &((block_run *) block2)[br_index];
		if (br_ptr->len == 0  &&  br_ptr->start == 0) {
			if (no_cache == 0)
				release_block(bfs->fd, ind_bnum);
			return ENOENT;
		}
		
		br->allocation_group = br_ptr->allocation_group;
		tmp = ((offset % dbl_ind_alloc_size) / bfs->dsb.block_size);
		br->start = br_ptr->start + tmp;
		br->len   = br_ptr->len - tmp;

		if (no_cache == 0)
			release_block(bfs->fd, ind_bnum);
		block2 = NULL;
		br_ptr = NULL;

		bnum = block_run_to_bnum(*br);
		CHECK_BNUM_WITH_CLEANUP(bnum, dbl_cleanup0);
		
		return 0;

	dbl_cleanup1:
		if (no_cache == 0)
			release_block(bfs->fd, dbl_ind_bnum);
	dbl_cleanup0:
		return ENOENT;
	}


	return ENOENT;
}

/* XXXdbg this function will go away eventually (when vm and the cache
   are unified
*/
void
bnum_for_pos(bfs_info *bfs, bfs_inode *bi, dr9_off_t *data)
{
	dr9_off_t     pos = *data;
	block_run br;

	if (file_pos_to_block_run(bfs, pos, bi, &br) != 0)
		*data = (dr9_off_t)-1;
	else
		*data = block_run_to_bnum(br);
}


int
read_data_stream(bfs_info *bfs, bfs_inode *bi,
				 dr9_off_t pos, char *buf, size_t *_len)
{
	int        file_pos_is_good = 1, bsize = bfs->dsb.block_size;
	char      *block = NULL;
	dr9_off_t  bnum;
	size_t     amt_read = 0, len = *_len;
	block_run  br;
	
	CHECK_INODE(bi);

	if (len >= INT_MAX) {
		printf("warning: tried to read %u bytes!\n", len);

		*_len = 0;
		return EFBIG;
	}

	if (pos < 0)
		pos = 0;

	if (pos >= bi->data.size) {
		*_len = 0;

		return 0;
	}

	if (pos + (dr9_off_t)len > bi->data.size)
		len = bi->data.size - pos;
	

	if (file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
		*_len = 0;

		CHECK_INODE(bi);

		return EINVAL;
	}


	if ((pos % bfs->dsb.block_size) != 0) {  /* work up to a block boundary */
		char *ptr;
		
		bnum = block_run_to_bnum(br);
		CHECK_BNUM(bnum);

		if (bi->flags & INODE_NO_CACHE) {
			ssize_t amt;
			
			amt = read_phys_blocks(bfs->fd, bnum, bi->etc->tmp_block,
								   1, bfs->dsb.block_size);
			if (amt != 0) {
				*_len = amt_read;

				return EINVAL;
			}
			
			block = bi->etc->tmp_block;
		} else {
			if ((block = get_block(bfs->fd, bnum, bsize)) == NULL) {
				*_len = 0;

				CHECK_INODE(bi);

				return EINVAL;
			}
		}
		
		ptr = &((char *)block)[(pos % bfs->dsb.block_size)];
		if (len <= bfs->dsb.block_size - (pos % bfs->dsb.block_size))
			amt_read = len;
		else
			amt_read = bfs->dsb.block_size - (pos % bfs->dsb.block_size);

		pos += amt_read;

		/*  get the run mapping the new position */

		if (amt_read < len && file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
			*_len = amt_read;

			CHECK_INODE(bi);

			return EINVAL;
		}
		
		memcpy(buf, ptr, amt_read);

		if ((bi->flags & INODE_NO_CACHE) == 0) {
			release_block(bfs->fd, bnum);
		}
	}

	while (amt_read < len) {
		bnum = block_run_to_bnum(br);
		if ((amt_read + (br.len * bfs->dsb.block_size)) > len)
			br.len = (len - amt_read) / bfs->dsb.block_size;

		if (br.len == 0)
			break;
		
		CHECK_BNUM(bnum);

		if (bi->flags & INODE_NO_CACHE) {
			ssize_t amt;
			
			amt = read_phys_blocks(bfs->fd, bnum, &buf[amt_read], br.len,
									bfs->dsb.block_size);
			if (amt != 0) {
				*_len = amt_read;

				CHECK_INODE(bi);

				return 0;
			}
		} else {
			if (cached_read(bfs->fd, bnum, &buf[amt_read], br.len,
							bfs->dsb.block_size) != 0) {
				*_len = amt_read;

				CHECK_INODE(bi);

				return 0;
			}
		}

		pos      += br.len * bfs->dsb.block_size;
		amt_read += br.len * bfs->dsb.block_size;

		if (amt_read < len) {
			if (file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
				file_pos_is_good = 0;
				break;
			}
		}
	}

	if (amt_read < len) {   /* have to clean up a last partial block */
		if (file_pos_is_good == 0) {
			if (file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
				printf("*** read: wanted pos %Ld but can't get it!\n", pos);
				*_len = amt_read;

				CHECK_INODE(bi);

				return 0;
			}
		}

		bnum = block_run_to_bnum(br);
		CHECK_BNUM(bnum);
		if (bi->flags & INODE_NO_CACHE) {
			ssize_t amt;
			
			amt = read_phys_blocks(bfs->fd, bnum, bi->etc->tmp_block,
								   1, bfs->dsb.block_size);
			if (amt != 0) {
				*_len = amt_read;

				return 0;
			}
			
			block = bi->etc->tmp_block;
		} else {
			if ((block = get_block(bfs->fd, bnum, bsize)) == NULL) {
				*_len = amt_read;

				CHECK_INODE(bi);

				return 0;
			}
		}
		
		memcpy(&buf[amt_read], block, (len - amt_read));
		amt_read = len;

		if ((bi->flags & INODE_NO_CACHE) == 0) {
			release_block(bfs->fd, bnum);
		}
	}

	*_len = amt_read;

	CHECK_INODE(bi);

	return 0;
}



int
free_data_stream(bfs_info *bfs, bfs_inode *bi)
{
	int        i, j, ret = EINVAL,
	           bsize = bfs->dsb.block_size;
	dr9_off_t  bnum;

	CHECK_INODE(bi);

	if (bfs->cur_lh == NULL)
		return ENOMEM;

	bi->etc->ag_hint = 0;
	bi->etc->ind_block_index_hint = 0;   /* always clear this out */

	/*
	 * first the direct blocks 
	 */
	bi->data.max_direct_range = 0;
	for(i=0; i < NUM_DIRECT_BLOCKS; i++) {
		if (block_run_to_bnum(bi->data.direct[i]) == 0) {
			if (block_run_to_bnum(bi->data.indirect) !=  0) {
				printf("*** danger: empty slots in direct blocks but indirect "
					   "blocks exist!\n");
				break;
			}
			if (block_run_to_bnum(bi->data.double_indirect) !=  0) {
				printf("*** danger: empty slots in direct blocks but double "
					   "indirect blocks exist!\n");
				break;
			}

			for(j=i+1; j < NUM_DIRECT_BLOCKS; j++) {
				if (block_run_to_bnum(bi->data.direct[j]) != 0)
					bfs_die("bfs: bi @ 0x%x has direct blocks set after a "
							"zero (i = %d, j = %d)\n", bi, i, j);
			}

			bi->data.size = 0;

			return 0;
		}
		
		if (free_block_run(bfs, &bi->data.direct[i]) != 0) {
			bfs_die("bad block run(1)! (%d, bi @ 0x%lx)\n", i, (ulong)bi);
			return EINVAL;
		}

		/* clear this baby out! */
		clear_block_run_fields(bi->data.direct[i]);
	}

	/* 
	 * now the indirect blocks, if any 
	 */

	bnum = block_run_to_bnum(bi->data.indirect);
	if (bnum == 0) {
		ret = 0;
		goto cleanup;
	}

	if (free_indirect_block_run(bfs, bi, &bi->data.indirect, bsize,
								0, NO_LOG) != 0) {
		bfs_die("error freeing indirect blocks for bi @ 0x%lx\n", (ulong)bi);
		goto cleanup;
	}

	bi->data.max_indirect_range = 0;

	if (free_block_run(bfs, &bi->data.indirect) != 0) {
		bfs_die("bad block run(2)! (%d, bi @ 0x%lx)\n", i, (ulong)bi);
		goto cleanup;
    }

	/* clear the indirect metadata out */
	clear_block_run_fields(bi->data.indirect);
	
	/* 
	 * and finally, the double indirect blocks, if any 
	 */

	bnum = block_run_to_bnum(bi->data.double_indirect);
	if (bnum == 0) {
		ret = 0;
		goto cleanup;
	}

	if (free_dbl_ind_block_run(bfs, bi, &bi->data.double_indirect,
							   bsize, 0) != 0) {
		bfs_die("Couldn't free dbl ind block run, bi @ 0x%lx\n", bi);
		goto cleanup;
	}
	bi->data.max_double_indirect_range = 0;

	if (free_block_run(bfs, &bi->data.double_indirect) != 0) {
		bfs_die("couldn't free double indirect page (bi @ 0x%lx)\n", (ulong)bi);
		goto cleanup;
	}
	clear_block_run_fields(bi->data.double_indirect);

	CHECK_INODE(bi);
	ret = 0;
	
 cleanup:
	bi->data.size = 0;

	return ret;
}



int
shrink_dstream(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size)
{
	int        i, brs_per_block, ind_blks_per_block;
	dr9_off_t  rounded_size, ind_bnum, dbl_ind_bnum, bnum, sum;
	dr9_off_t  max_dbl_ind_offset, new_size_offset, rounded_size_offset;
	block_run *indirect, *dbl_indirect, *ind_br_ptr;
	int bsize = bfs->dsb.block_size;
	int 	j = -1;
	dr9_off_t b;
	int dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);

	CHECK_INODE(bi);


	/*
	 * Check for some quick shortcuts
	 */
	
	if (new_size == 0) {
		return free_data_stream(bfs, bi);
	}

	if (bi->data.max_double_indirect_range &&
		new_size > bi->data.max_double_indirect_range) {
		/*
		   we compute the difference from max_indirect_range because it
		   is that offset that must be a multiple of the double indirect
		   allocation size.
		*/   
		new_size_offset    = new_size - bi->data.max_indirect_range;
		max_dbl_ind_offset = bi->data.max_double_indirect_range -
			                 bi->data.max_indirect_range;

		if (max_dbl_ind_offset % dbl_ind_alloc_size)   /* sanity check */
			bfs_die("bi 0x%x bogus dbl ind size\n", bi);
		

		rounded_size_offset =
			(new_size_offset+(off_t)(dbl_ind_alloc_size-1)) & ~(off_t)(dbl_ind_alloc_size-1);

		if (rounded_size_offset == max_dbl_ind_offset) {
			bi->data.size = new_size;
			return 0;
		}
	} 

	rounded_size = BLOCK_SIZE_ROUND_UP(new_size);
	
	if (rounded_size == BLOCK_SIZE_ROUND_UP(bi->data.size)) {
		bi->data.size = new_size;
		return 0;
	}

	/* the # of block_runs per file system block (for indirect blocks) */
	ind_blks_per_block = brs_per_block = bsize / sizeof(block_run);

	if (new_size < bi->data.max_double_indirect_range) {

		if (rounded_size <= bi->data.max_indirect_range) {
			/* Kill everything under the dbl_indirect block */
			int ret;
			ret = free_dbl_ind_block_run(bfs, bi, &bi->data.double_indirect, 
										 bsize, 0);
			if (ret != 0) {
				return EINVAL;
			}
			bi->data.size = bi->data.max_indirect_range;
			if (free_block_run(bfs, &bi->data.double_indirect) != 0) {
				/*
				 * Clear the double indirect block run fields out
				 * even if free_block_run() failed, because it is better
				 * to leak the dbl ind block run blocks (only 4)
				 * than give the indication of having data in the
				 * double_indirect blocks.
				 */
				clear_block_run_fields(bi->data.double_indirect);
				return EINVAL;
			}
			clear_block_run_fields(bi->data.double_indirect);
			bi->data.max_double_indirect_range = 0;
		} else { 
			rounded_size_offset = ((new_size - bi->data.max_indirect_range) +
								   (dbl_ind_alloc_size - 1)) &
								  ~(dbl_ind_alloc_size - 1);

			if (shrink_dbl_indirect(bfs, bi, rounded_size_offset +
									bi->data.max_indirect_range) != 0) {
				/*
				 * XXXmani How do we adjust for the size here?
				 */
				return EINVAL;
			}
			bi->data.max_double_indirect_range = rounded_size_offset +
				                                 bi->data.max_indirect_range;
		}
	}

	if (new_size < bi->data.max_indirect_range) {
		if (bi->data.max_double_indirect_range != 0) {
			bfs_die("shrink_dstream: max_double_ind_range = %d (!= 0) "
					"when new_size (%d) < max_indirect_range (%d)\n",
					bi->data.max_double_indirect_range, new_size,
					bi->data.max_indirect_range);
		}

		bi->etc->ind_block_index_hint = 0;   /* always clear this out */

		if (rounded_size <= bi->data.max_direct_range) {
			/* kill everything under the indirect block */
			int ret;
			ret = free_indirect_block_run(bfs, bi, &bi->data.indirect, bsize,
										  0, NO_LOG);
			if (ret != 0  ||  free_block_run(bfs, &bi->data.indirect) != 0) 
				return EINVAL;
			clear_block_run_fields(bi->data.indirect);
			bi->data.max_indirect_range = 0;
		} else {
			if (shrink_indirect(bfs, bi, rounded_size) != 0) 
				return EINVAL;
			bi->data.max_indirect_range = rounded_size;
		}
	}

	if (new_size < bi->data.max_direct_range) {
		if (bi->data.max_double_indirect_range != 0  ||
			bi->data.max_indirect_range != 0) {
			bfs_die("shrink_dstream: max_dbl_ind_range = %d "
					"max_ind_range = %d when new_size (%d) "
					"< max_direct_range (%d)\n",
					bi->data.max_double_indirect_range,	bi->data.max_indirect_range,
					new_size, bi->data.max_direct_range);
		}

		for(i=0, sum=0; i < NUM_DIRECT_BLOCKS && sum < rounded_size; i++) {
			if (bi->data.direct[i].start == 0 && bi->data.direct[i].len == 0)
				break;
			
			sum += bi->data.direct[i].len * bfs->dsb.block_size;
		}

		if (sum < rounded_size && i >= NUM_DIRECT_BLOCKS)
			bfs_die("2: should not be here: bi @ 0x%lx sum %Ld, i %d, "
					"brs_per_block %d, new_size %Ld\n", bi, sum, i,
					brs_per_block, new_size);

		if (sum > rounded_size) { /* then we have to shrink this block run */
			dr9_off_t     tmp;
			block_run to_free;

			i--;  /* because the for loop would have gone one beyond */
			
			to_free = bi->data.direct[i];

			tmp = to_free.len - ((sum - rounded_size) / bfs->dsb.block_size);
			to_free.start          += tmp;
			to_free.len            -= tmp;
			bi->data.direct[i].len  = tmp;

			if (to_free.len && free_block_run(bfs, &to_free) != 0) {
				bfs_die("bad br: 3: for direct %d, bi @ 0x%lx\n", i, (ulong)bi);
 			}
			clear_block_run_fields(to_free);
			
			i++;  /* so the for loop below free's the right things */
		}

		for(; i < NUM_DIRECT_BLOCKS; i++) {
			if (bi->data.direct[i].len &&
				free_block_run(bfs, &bi->data.direct[i]) != 0) {
				bfs_die("bad br: 4: for direct %d, bi @ 0x%lx\n", i, (ulong)bi);
			}
			clear_block_run_fields(bi->data.direct[i]);
		}

		bi->data.max_direct_range = rounded_size;
	}
	
	bi->data.size = new_size;
	CHECK_INODE(bi);

	return 0;
}


int
grow_dstream(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size)
{
	int 	i, ag, bsize = bfs->dsb.block_size, avg_run_length = 0;
	int 	num_allocations = 1, len_sum = 0;
	char 	*block = NULL;
	int 	exact;
	dr9_off_t     amount;
	dr9_off_t     num_blocks, num_blocks_orig, real_num_blocks;
	int		tmp_num_blocks;
	block_run tmp_br, *lbr;
	int		ret;
	int dbl_ind_alloc_size;
	int last_br_index = 0;
	dbl_indirect_pos lastpos;

	amount = new_size - bi->data.size;

	if (amount == 0)
		return 0;

	if (amount < 0) {
		printf("** grow_dstream called w/negative size (old: %Ld, new %Ld)!\n",
			   bi->data.size, new_size);
	}
	
	CHECK_INODE(bi);

	/* this is in case we don't really have to resize... */
	if ((amount + bi->data.size) <= bi->data.max_direct_range ||
		(amount + bi->data.size) <= bi->data.max_indirect_range ||
		(amount + bi->data.size) <= bi->data.max_double_indirect_range) {

        if (file_pos_to_block_run(bfs, bi->data.size, bi, &tmp_br) != 0){
printf("can't find file position %Ld for bi 0x%lx (amount %Ld new_size %Ld)\n",
       bi->data.size, (ulong)bi, amount, new_size);
			return EINVAL;
		}

		bi->data.size = new_size;
		if (bi->data.size > bi->data.max_direct_range &&
			bi->data.max_indirect_range == 0 &&
			bi->data.max_double_indirect_range == 0)
			bfs_die("I am hosed (2) (bi @ 0x%x)\n", bi);

		CHECK_INODE(bi);

		return 0;
	}

	/* here we subtract off any pre-allocated blocks that we already have */
	if (bi->data.size < bi->data.max_direct_range) {
		amount -= (bi->data.max_direct_range - bi->data.size);
	} else if (bi->data.size < bi->data.max_indirect_range) {
		amount -= (bi->data.max_indirect_range - bi->data.size);
	} else if (bi->data.size < bi->data.max_double_indirect_range) {
		amount -= (bi->data.max_double_indirect_range - bi->data.size);
	}

	if (amount <= 0)
		bfs_die("oh shit! amount == %Ld (size %Ld bi @ 0x%lx)\n", amount,
				bi->data.size, (ulong)bi);

	num_blocks = BLOCK_SIZE_ROUND_UP(amount) / bfs->dsb.block_size;
	
	/*
	  first check if there's enough space.  we skip the check for directories
	  because directory allocations have to succeed (they reserve space up
	  front before starting the transaction).  

	  The checks are as follows: never let a file allocate the last 128 
	  blocks of a disk (if the disk is bigger than 100 meg) and never
	  let the number of blocks we want plus the number of reserved blocks
	  be larger than the number of free blocks.
	*/
	if (S_ISDIR(bi->mode) == 0) {
		if (NUM_FREE_BLOCKS(bfs) <= 128 && (bfs->dsb.num_blocks*bfs->dsb.block_size) > (100*1024*1024)) {
			printf("Low # free blocks watermark hit! (# free == %Ld)\n",
				   NUM_FREE_BLOCKS(bfs));
			return ENOSPC;
		}

		if ((bfs->reserved_space+num_blocks) >= NUM_FREE_BLOCKS(bfs)) {
			printf("no space for %Ld blocks (only %Ld available)\n", 	
				   num_blocks, NUM_FREE_BLOCKS(bfs));
			return ENOSPC;
		}
	}

	/*
	  Only do pre-allocation if there is a reasonable amount of free
	  disk space available (i.e. more than roughly 4% of the disk).

	  Also, we *don't* want to do pre-allocation for attributes or
	  attribute directories because they don't go through the normal
	  open/write/close routines and so they wouldn't ever get trimmed
	  back and thus they would waste/leak disk space.
	*/
	real_num_blocks = num_blocks;   /* save off the "real" allocation amount */
	if (NUM_FREE_BLOCKS(bfs) > (bfs->dsb.num_blocks >> 8)) {
		exact = TRY_HARD_ALLOCATION;
		if (((bi->mode & (S_ATTR | S_ATTR_DIR)) == 0) && num_blocks < 64) {
			num_blocks = 64;
		}
	} else {
		exact = LOOSE_ALLOCATION;
	}


	if (bi->etc->ag_hint == 0) {
		if (S_ISDIR(bi->mode) == 0)
			bi->etc->ag_hint = ag = (bi->inode_num.allocation_group + 1) % bfs->dsb.num_ags;
		else
			bi->etc->ag_hint = ag = bi->inode_num.allocation_group;
	} else {
		ag = bi->etc->ag_hint;
	}
	dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);	

	num_blocks_orig = num_blocks;
	while(num_blocks > 0) {
		/* can't ever allocate more blocks than are in one ag */
		if (num_blocks > (bfs->dsb.blocks_per_ag * bfs->dsb.block_size * 8))
			tmp_num_blocks = (bfs->dsb.blocks_per_ag * bfs->dsb.block_size * 8);
		else
			tmp_num_blocks = num_blocks;

		/* the len field of a block_run is 16-bits, so don't overflow it */
		if (tmp_num_blocks > USHRT_MAX)  
			tmp_num_blocks = USHRT_MAX;

		
		/*
		 * We want to avoid the situation where we continually search for
		 * a large block run on a very fragmented disk.  In this situation,
		 * always looking for long block runs tremendously degrades
		 * performance.  To get around this problem, we maintain an
		 * average of block run lengths and default to a small block run
		 * length when the average of previously obtained block runs
		 * is less than 10 (we pick the number 10 somewhat arbitrarily).
		 */
		if (bi->data.max_double_indirect_range) {
			exact = EXACT_ALLOCATION;
			tmp_num_blocks = dbl_ind_alloc_size/bsize;
		} else if (num_allocations > 3 && avg_run_length < 10 && real_num_blocks >= 32) {
			exact = LOOSE_ALLOCATION;
			tmp_num_blocks = 32;
		}
		
		ret = allocate_block_run(bfs, ag, tmp_num_blocks, &tmp_br, exact);
		if (ret != 0  || 
			(tmp_br.len != dbl_ind_alloc_size/bsize  &&
			 bi->data.max_double_indirect_range)) {
			dr9_off_t old_size;

			/* just make sure we're not failing on a pre-allocation... */
			if (ret != 0 && (num_blocks_orig - num_blocks) >= real_num_blocks){
				break;
			}

			if (ret == 0) {
				free_block_run(bfs, &tmp_br);
			}

			/* printf("can't allocate data blocks for bi 0x%lx (want %d)\n",
				   (ulong)bi, tmp_num_blocks); */

			old_size = bi->data.size;

			/* bump up the size so we can call shrink_dstream() */
			bi->flags |= TRIMMING_INODE;
			bi->data.size += (num_blocks_orig - num_blocks) * bsize;
			shrink_dstream(bfs, bi, old_size);
			bi->flags &= ~TRIMMING_INODE;

			return ENOSPC;
		}

		bi->etc->ag_hint = tmp_br.allocation_group;
		
		if (bi->data.max_double_indirect_range == 0) {
			avg_run_length = (len_sum += tmp_br.len) / num_allocations++; 
		}

		/*
		  here we try to avoid failing to allocate pre-allocation blocks
		  when we already have enough space for the amount we really need
		  to grow by.  if we try to pre-allocate blocks on a highly
		  fragmented disk, we might wind up allocating small block runs
		  that aren't needed or forcing ourselves to allocate an indirect
		  page when we don't even need one.
		*/
		if ((num_blocks_orig - num_blocks) >= real_num_blocks &&
			((num_allocations > 3 && avg_run_length < 4) ||
				tmp_br.len < 16)) {

			free_block_run(bfs, &tmp_br);
			break;
		}

		ag = tmp_br.allocation_group;
		num_blocks -= tmp_br.len;

		/* now we have to find where to insert this block run */
		if (bi->data.size <= bi->data.max_direct_range &&
            bi->data.max_indirect_range == 0) {
			lbr = &bi->data.direct[0];
			for(i=0; i < NUM_DIRECT_BLOCKS; i++, lbr++) {
				if (lbr->allocation_group == 0 && lbr->start == 0) {
					break;
				}
			}

			/* check if we can append this block run */
			if (i > 0 && i < NUM_DIRECT_BLOCKS) {  
				lbr--;
				if (lbr->allocation_group == tmp_br.allocation_group &&
					(int)lbr->start + (int)lbr->len == (int)tmp_br.start &&
					(int)lbr->len + (int)tmp_br.len <= (int)USHRT_MAX) {

					lbr->len += tmp_br.len;   /* append it! */
					bi->data.max_direct_range += tmp_br.len * bfs->dsb.block_size;
					continue;
				}				
				lbr++;
			}
			
			if (i < NUM_DIRECT_BLOCKS) {
				*lbr = tmp_br;
				bi->data.max_direct_range += tmp_br.len * bfs->dsb.block_size;
				continue;
			}
		}

		/*
		 * Indirect blocks
		 */

		if ((bi->data.size <= bi->data.max_indirect_range ||
			bi->data.max_indirect_range == 0) && 
			bi->data.max_double_indirect_range == 0) {

			/* grow the indirect range */
			dr9_off_t tmp, b;
			int err;
			int i, len;

			if (bi->data.indirect.len == 0) { 
				err = 0;
				b = allocate_indirect_pages(bfs, bi, &bi->data.indirect, &tmp_br,&err);
				if (b < 0) {
					if (err == ENOSPC) {
						dr9_off_t old_size;
						old_size = bi->data.size;

						bi->flags |= TRIMMING_INODE;
						/* bump up the size so we can call shrink_dstream() */
						bi->data.size += (num_blocks_orig - num_blocks) * bsize;
						/* need to subtract off tmp_br.len because it was
						   added to num_blocks up above but it hasn't been
						   added to the file yet
					    */   
						bi->data.size -= (tmp_br.len * bsize);
						shrink_dstream(bfs, bi, old_size);
						bi->flags &= ~TRIMMING_INODE;
					
					}

					free_block_run(bfs, &tmp_br);
					return err;
				}
			} else {
				b = block_run_to_bnum(bi->data.indirect);
			}
				
			for (i=bi->etc->ind_block_index_hint; i < bi->data.indirect.len; i++) {
				tmp = b+i;
				CHECK_BNUM(tmp);
				if ((block = get_block(bfs->fd, tmp, bsize)) == NULL) {
					dr9_off_t old_size;
					old_size = bi->data.size;

					bi->flags |= TRIMMING_INODE;
					/* bump up the size so we can call shrink_dstream() */
					bi->data.size += (num_blocks_orig - num_blocks) * bsize;
					/* need to subtract off tmp_br.len because it was
					   added to num_blocks up above but it hasn't been
					   added to the file yet
					*/   
					bi->data.size -= (tmp_br.len * bsize);

					shrink_dstream(bfs, bi, old_size);
					bi->flags &= ~TRIMMING_INODE;
					
					free_block_run(bfs, &tmp_br);
					return EINVAL;
				}
								   
				if (grow_indirect_range(bfs, bi, &tmp_br, block, &last_br_index) == 0) {
					if (bi->data.max_indirect_range == 0) {
						bi->data.max_indirect_range = bi->data.max_direct_range;
					}
					bi->data.max_indirect_range += tmp_br.len * bsize;

					if (log_write_blocks(bfs, bfs->cur_lh, tmp, block, 1, 1) != 1)
						printf("* error writing indirect page %Ld bi 0x%lx\n",
							   tmp, (ulong)bi);

					bi->etc->ind_block_index_hint = i;
					release_block(bfs->fd, tmp);
					break;
				} else {
					bi->etc->ind_block_index_hint = 0; /* just to be safe */
					release_block(bfs->fd, tmp);
				}
			}

			if (i < bi->data.indirect.len) {
				/* we successfully grew the indirect range */
				continue;
			}
		}

		/*
		 * Double indirect blocks
		 */

		if (bi->data.size <= bi->data.max_double_indirect_range ||
			bi->data.max_double_indirect_range == 0) {
			int br_len = dbl_ind_alloc_size/bsize;
			int err;

			if (tmp_br.len != br_len  ||  bi->data.max_double_indirect_range == 0) {
				/*
				 * have to reallocate this block run. this should
				 * only happen if max_double_indirect_range is 0.
				 */
				if (bi->data.max_double_indirect_range != 0) {
					bfs_die("Why am I here: tmp_br len = %d, but we wanted %d\n",
							tmp_br.len, br_len);
				}

				num_blocks += tmp_br.len;
				free_block_run(bfs, &tmp_br);

				ret = allocate_block_run(bfs, ag, br_len, &tmp_br,
										 EXACT_ALLOCATION);

				if (ret == 0  &&  tmp_br.len == br_len) {				
					ag = tmp_br.allocation_group;
					num_blocks -= tmp_br.len;
				} else {
					dr9_off_t old_size;

					if (ret == 0) {
						free_block_run(bfs, &tmp_br);
					}
					
					printf("can't allocate a block run for bi 0x%lx "
						   "(want %d) (2)\n", (ulong)bi, br_len);
					
					old_size = bi->data.size;
					
					bi->flags |= TRIMMING_INODE;
					/* bump up the size so we can call shrink_dstream() */
					bi->data.size += (num_blocks_orig - num_blocks) * bsize;

					shrink_dstream(bfs, bi, old_size);
					bi->flags &= ~TRIMMING_INODE;
					
					return ENOSPC;
				}

				lastpos.dbl_ind_block_index = -1;
				lastpos.ind_br_index = -1;
				lastpos.ind_block_index = -1;
				lastpos.br_index = -1;
			}

			ret = grow_dbl_indirect(bfs, bi, &tmp_br, &lastpos, &err);
			if (ret != 0) {
				dr9_off_t  old_size;
				old_size = bi->data.size;

				bi->flags |= TRIMMING_INODE;
				/* bump up the size so we can call shrink_dstream() */
				bi->data.size += (num_blocks_orig - num_blocks) * bsize;
				/* need to subtract off tmp_br.len because it was
				   added to num_blocks up above but it hasn't been
				   added to the file yet
				*/   
				bi->data.size -= (tmp_br.len * bsize);

				shrink_dstream(bfs, bi, old_size);
				bi->flags &= ~TRIMMING_INODE;
				
				free_block_run(bfs, &tmp_br);
				return ret;
			}
			

			if (bi->data.max_double_indirect_range == 0) {
				bi->data.max_double_indirect_range = bi->data.max_indirect_range;
			}

			bi->data.max_double_indirect_range += tmp_br.len * bsize;
			continue;
		}
	}

	bi->data.size = new_size;

	if (bi->data.size > bi->data.max_direct_range &&
		bi->data.max_indirect_range == 0 &&
		bi->data.max_double_indirect_range == 0)
			bfs_die("I am hosed (3) (bi @ 0x%x)\n", bi);

	if (bi->data.max_double_indirect_range == 0 && 
        bi->data.max_indirect_range != 0 &&
		bi->data.max_indirect_range < bi->data.size)
        bfs_die("grow_dstream: data size %d but max_indirect %d\n", 
				bi->data.size, bi->data.max_indirect_range);

	CHECK_INODE(bi);

	return 0;
}


/*
   the inode to be operated on should have been fully locked
   before calling this function and a transaction should also
   have been started.
*/   
int
set_file_size(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size)
{
	int ret;
	
	if (new_size < 0)
		return EINVAL;

	if (new_size == bi->data.size)
		return 0;

	if (new_size < bi->data.size)
		ret = shrink_dstream(bfs, bi, new_size);
	else
		ret = grow_dstream(bfs, bi, new_size);

	update_inode(bfs, bi);

	return ret;
}


void
trim_preallocated_blocks(bfs_info *bfs, bfs_inode *bi)
{
	dr9_off_t real_max, real_size;
	
	if (bfs->flags & FS_READ_ONLY)
		return;

	if (bi->mode & S_ATTR_DIR)
		return;  /* attribute directories are not pre-allocated */
	
	if (bi->data.size < bi->data.max_double_indirect_range)
		real_max = bi->data.max_double_indirect_range;
	else if (bi->data.size < bi->data.max_indirect_range)
		real_max = bi->data.max_indirect_range;
	else if (bi->data.size < bi->data.max_direct_range)
		real_max = bi->data.max_direct_range;
	else
		return;
	
	if (BLOCK_SIZE_ROUND_UP(bi->data.size) == real_max) 
		return;

	bfs->cur_lh = start_transaction(bfs);
	if (bfs->cur_lh == NULL)
		return;

	bi->flags |= TRIMMING_INODE;
	real_size = bi->data.size;
	bi->data.size = real_max;

	shrink_dstream(bfs, bi, real_size);
	bi->flags &= ~TRIMMING_INODE;

	update_inode(bfs, bi);

	end_transaction(bfs, bfs->cur_lh);
}


int
write_data_stream(bfs_info *bfs, bfs_inode *bi,
				  dr9_off_t pos, const char *buf, size_t *_len)
{
	int        bsize = bfs->dsb.block_size, err = 0;
	char      *block, finish_transaction = 0;
	dr9_off_t bnum, old_size;
	size_t     amt_written = 0, offset, chunk, amt, len = *_len;
	block_run  br;
	
	CHECK_INODE(bi);

	if (bfs->flags & FS_READ_ONLY) {
		*_len = 0;
		return EROFS;
    }

	if (pos < 0)
		pos = 0;

	/*
	   if we have to grow the file and it's not a logged inode (i.e. directory)
	   then start a transaction to record the growth of the file (but not its
	   data).  we don't start a transaction for directories because they should
	   already have a transaction started.
	*/
	if ((pos+len) > bi->data.size && (bi->flags & INODE_LOGGED) == 0 &&
		(bi->flags & NO_TRANSACTION) == 0) {
		bfs->cur_lh = start_transaction(bfs);
		if (bfs->cur_lh == NULL) {
			*_len = 0;
			return ENOMEM;
		}

		finish_transaction = 1;
	}
		

	old_size = bi->data.size;
		
	if (pos+len > bi->data.size) {    /* then we need to grow the file */
		if (grow_dstream(bfs, bi, pos + len) != 0) {
			printf("write_dstream: no space for %d bytes (vnid %Ld bi 0x%lx)\n",
				   len, bi->etc->vnid, (ulong)bi);
			update_inode(bfs, bi);
			if (finish_transaction)
				abort_transaction(bfs, bfs->cur_lh);

			*_len = 0;

			CHECK_INODE(bi);

			return ENOSPC;
		}

		update_inode(bfs, bi);   /* since we grew the file */

		if ((bi->flags & INODE_LOGGED) == 0   &&
			(bi->flags & NO_TRANSACTION) == 0 && finish_transaction) {
			end_transaction(bfs, bfs->cur_lh);
			finish_transaction = 0;
		}
	}

	bi->flags |= INODE_WAS_WRITTEN;


	offset = (pos % (dr9_off_t)bsize);
	if (offset != 0) {        /* then work up to a block boundary */
		
		if (file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
			bfs_die("*** write_data_stream could not find pos %Ld bi 0x%lx "
					"max size %Ld\n", pos, (ulong)bi, bi->data.size);
			err = EINVAL;
            goto error;
		}

		bnum = block_run_to_bnum(br);
		CHECK_BNUM(bnum);

		if (bi->flags & INODE_NO_CACHE) {
			ssize_t amt;
			
			amt = read_phys_blocks(bfs->fd, bnum, bi->etc->tmp_block,
								   1, bsize);
			if (amt != 0)
				goto error;
			
			block = bi->etc->tmp_block;

		} else {
			if ((block = get_block(bfs->fd, bnum, bsize)) == NULL)
				goto error;
		}

		if (len <= bsize - offset)
			chunk = len;
		else
			chunk = bsize - offset;
		
		memcpy(&((char *)block)[offset], buf, chunk);

		if (bnum == bi->etc->vnid) {
			bfs_die("trying to write to the inode?!? (bi 0x%lx, bnum 0x%Lx)\n",
					(ulong)bi, bnum);
		}

		if (bi->flags & INODE_LOGGED) {
			amt = log_write_blocks(bfs, bfs->cur_lh, bnum, block, 1, 1);
		} else if (bi->flags & INODE_NO_CACHE) {
			amt = write_phys_blocks(bfs->fd, bnum, (void *)bi->etc->tmp_block,
									1, bsize);
			if (amt == 0)
				amt = 1;
		} else {
			if (cached_write(bfs->fd, bnum, block, 1, bsize) == 0)
				amt = 1;
			else
				amt = 0;
		}

		if ((bi->flags & INODE_NO_CACHE) == 0) {
			release_block(bfs->fd, bnum);
		}
		block = NULL;

		if (amt != 1) {
			printf("write_data_stream block write bnum %Ld failed bi 0x%lx\n",
				   bnum, (ulong)bi);
			err = ENOSPC;
			goto error;
		}

		pos         += chunk;
		amt_written  = chunk;
	}


	while (amt_written < len) {
		if (file_pos_to_block_run(bfs, pos, bi, &br) != 0) {
			bfs_die("*** write_data_stream 2 could not find pos %Ld bi 0x%lx "
					"max data size %Ld\n", pos, (ulong)bi, bi->data.size);

            if (finish_transaction)
				abort_transaction(bfs, bfs->cur_lh);

			*_len = amt_written;

			CHECK_INODE(bi);

			return 0;
		}

		if ((len - amt_written) < bsize)
			break;

		bnum = block_run_to_bnum(br);
		CHECK_BNUM(bnum);
		
		if ((len - amt_written) < (br.len * bsize))
			br.len = (len - amt_written) / bsize;
		
		if (bi->etc->vnid >= bnum && bi->etc->vnid < bnum + br.len) {
			bfs_die("2:trying to write to the inode? (bi 0x%lx, bnum 0x%Lx)\n",
					(ulong)bi, bnum);
		}

		if (bi->flags & INODE_LOGGED)
			amt = log_write_blocks(bfs, bfs->cur_lh, bnum, &buf[amt_written],
								   br.len, 1);
		else if (bi->flags & INODE_NO_CACHE) {
			amt = write_phys_blocks(bfs->fd, bnum, (void *)&buf[amt_written],
									br.len, bsize);
			if (amt == 0)
				amt = br.len;
		} else {
			amt = cached_write(bfs->fd, bnum, &buf[amt_written],
							   br.len, bsize);
			if (amt == 0)
				amt = br.len;
			else
				amt = 0;
		}

		CHECK_INODE(bi);
		
		if (amt != br.len) {
			printf("write_data_stream bwrite %Ld returned %d (%d) bi 0x%lx\n",
				   bnum, amt, br.len, (ulong)bi);

            if (finish_transaction)
				abort_transaction(bfs, bfs->cur_lh);

			*_len = amt_written;

			CHECK_INODE(bi);

			return 0;
		}

		pos         += br.len * bsize;
		amt_written += br.len * bsize;
	}

	if (amt_written < len) {   /* have to clean up a last partial block */
		bnum = block_run_to_bnum(br);
		CHECK_BNUM(bnum);

		if (bi->flags & INODE_NO_CACHE) {
			ssize_t amt;
			
			amt = read_phys_blocks(bfs->fd, bnum, bi->etc->tmp_block,
								   1, bsize);
			if (amt != 0)
				goto error;
			
			block = bi->etc->tmp_block;
		} else {
			if ((old_size == BLOCK_SIZE_ROUND_DOWN(old_size) && pos >= old_size) ||
				(BLOCK_SIZE_ROUND_DOWN(old_size) < BLOCK_SIZE_ROUND_DOWN(bi->data.size)))
				block = get_empty_block(bfs->fd, bnum, bsize);
			else
				block = get_block(bfs->fd, bnum, bsize);

			if (block == NULL) {
				printf("write_data_stream block read bnum %Ld failed bi 0x%lx 1\n",
					   bnum, (ulong)bi);
				goto error;
			}
		}

		memcpy(block, &buf[amt_written], (len - amt_written));

		if (bnum == bi->etc->vnid) {
			bfs_die("trying to write to the inode?!? (bi 0x%lx, bnum 0x%Lx)\n",
					(ulong)bi, bnum);
		}

		if (bi->flags & INODE_LOGGED) {
			amt = log_write_blocks(bfs, bfs->cur_lh, bnum, block, 1, 1);
		} else if (bi->flags & INODE_NO_CACHE) {
			amt = write_phys_blocks(bfs->fd, bnum, (void *)bi->etc->tmp_block,
									1, bsize);
			if (amt == 0)
				amt = 1;
		} else {
			if (cached_write(bfs->fd, bnum, block, 1, bsize) == 0)
				amt = 1;
			else
				amt = 0;
		}
		
		*_len = amt_written;

		CHECK_INODE(bi);
		
		if ((bi->flags & INODE_NO_CACHE) == 0) {
			release_block(bfs->fd, bnum);
		}
		block = NULL;

		if (amt != 1) {
			printf("*write_data_stream bwrite bnum %Ld failed bi 0x%lx (3)\n",
				   bnum, (ulong)bi);

            if (finish_transaction)
				abort_transaction(bfs, bfs->cur_lh);

			CHECK_INODE(bi);

			return 0;
        }
		
		amt_written = len;
	}


	if (finish_transaction)
		end_transaction(bfs, bfs->cur_lh);
	
	*_len = amt_written;

	CHECK_INODE(bi);

	return 0;

 error:
	*_len = amt_written;

	CHECK_INODE(bi);

	if (finish_transaction)
		abort_transaction(bfs, bfs->cur_lh);

	return err;
}


static int
grow_indirect_range(bfs_info *bfs, bfs_inode *bi, block_run *data_br_ptr, char *block,
					int *last_br_index_ptr)
{
	/* grow the indirect range */
	dr9_off_t tmp;
	int bsize = bfs->dsb.block_size;
	block_run *lbr;
	int i;
	int start = *last_br_index_ptr;

	lbr = &((block_run *)block)[start];
	
	for(i=start; i < bsize / sizeof(block_run); i++,lbr++){
		if (lbr->allocation_group == 0 && lbr->start == 0) {
			break;
		}
	}

	/* check if we can append this block run */
	if (i > 0 && (i < bsize / sizeof(block_run))) {  
		lbr--;
		if (lbr->allocation_group == data_br_ptr->allocation_group &&
			(int)lbr->start + (int)lbr->len == (int)data_br_ptr->start &&
			(int)lbr->len + (int)data_br_ptr->len <= (int)USHRT_MAX) {

			lbr->len += data_br_ptr->len;   /* append it! */
			*last_br_index_ptr = i-1;
			return 0;
			
		}				
		lbr++;
	}
	
	if (i < bsize / sizeof(block_run)) {
		*lbr = *data_br_ptr;
		*last_br_index_ptr = i;
		return 0;
	}

	/*
	 * no more room in this indirect block.
	 */
	*last_br_index_ptr = 0;
	return -1;
}

static dr9_off_t
allocate_indirect_pages(bfs_info *bfs, bfs_inode *bi, block_run *indirect_ptr, 
						block_run *data_br_ptr, int *errptr)
{
	dr9_off_t bnum;
	int bsize = bfs->dsb.block_size;
	char *block;
	int br_len;
	int ret;
	int i;

	*errptr = 0;
	br_len = get_dbl_ind_alloc_size(bfs, bi)/bsize;
	bnum = block_run_to_bnum(*indirect_ptr);
	if (bnum == 0) {		/* have to allocate indirect pages */
		int alloc_policy = TRY_HARD_ALLOCATION;
		block_run ind;
		
		if ((bfs->dsb.num_blocks - bfs->dsb.used_blocks) < (bfs->dsb.num_blocks >> 7))
			alloc_policy = LOOSE_ALLOCATION;
	
		ret = allocate_block_run(bfs, data_br_ptr->allocation_group, br_len,
								 &ind, alloc_policy);
		if (ret != 0) {
			printf("*** failed to allocate indirect page bis 0x%lx\n", (ulong)bi);
			*errptr = ENOSPC;
			return -1;
		}

		bnum = block_run_to_bnum(ind);

		for (i=0; i < ind.len; i++) {
			/* zero out all the blocks */

			CHECK_BNUM(bnum+i);
			if ((block = get_empty_block(bfs->fd, bnum+i, bsize)) == NULL) {
				free_block_run(bfs, &ind);
				printf("can't get empty indirect block (bnum %Ld bi 0x%lx)!\n",
					   bnum,(ulong)bi);
				*errptr = ENOMEM;
				return -1;
			}					

 			if (log_write_blocks(bfs, bfs->cur_lh, bnum+i, block, 1, 1) != 1) {
				printf("* error writing ind page to log after allocation, bi @ 0x%lx, "
					   	"i = %d\n", bi, i);
			}
			release_block(bfs->fd, bnum+i);
		}

		*indirect_ptr = ind;
	}

	return bnum;
}

static int
get_dbl_ind_alloc_size(bfs_info *bfs, bfs_inode *bi)
{
	int bsize = bfs->dsb.block_size;
	int min;

	switch (bi->magic1) {
	case INODE_MAGIC1:
	default:
		min = 4096;
		break;
	}

	if (min < bsize) 
		min = bsize;

	return min;
}


static int
free_page_of_block_runs(bfs_info *bfs, bfs_inode *bi, void *block, int start)
{
	int 		i;
	dr9_off_t	bnum;
	block_run 	*br;

	if (start < 0  ||  start > bfs->dsb.block_size / sizeof(block_run)) {
		printf("free_page_of_block_runs: invalid value for start, %d\n", start);
		return -1;
	}

	br = (block_run *) block;
	for (i=start; i < bfs->dsb.block_size / sizeof(block_run); i++) {
		bnum = block_run_to_bnum(br[i]);
		if (bnum == 0) {
			break;
		}

		SIMPLE_CHECK_BNUM(bnum);

		if (free_block_run(bfs, &br[i]) != 0) {
			printf("bad block run(0)! (%d)\n", i);
			return EINVAL;
		}
		clear_block_run_fields(br[i]);
	}

	return 0;
}

static int
free_indirect_block_run(bfs_info *bfs, bfs_inode *bi, block_run *ind_br_ptr, 
						int bsize, int start, int do_log)
{
	int i;
	dr9_off_t bnum, b;
	char *block;

	if (start < 0 || start > ind_br_ptr->len) {
		bfs_die("free_ind_block_run: invalid start, = %d, bi @ 0x%lx, "
				"ind_br_ptr = 0x%x\n", start, bi, ind_br_ptr);
	}

	b = block_run_to_bnum(*ind_br_ptr);
	if (b == 0)
		return 0;

	for (i = start; i < ind_br_ptr->len; i++) {
		bnum = b + i;
		CHECK_BNUM(bnum);
		if ((block = get_block(bfs->fd, bnum, bsize)) == NULL) {
			printf("Could not get indirect block, bi @ 0x%lx, bnum = %d\n",
				   bi, bnum);
			return EINVAL;
		}

		if (free_page_of_block_runs(bfs, bi, block, 0) != 0) {
			bfs_die("Problem deleting page of block runs, bi @ 0x%lx, start = %d, i = %d, b = %d\n",
					(ulong) bi, start, i, (long)b);
		}

		if (do_log == WRITE_LOG)
			log_write_blocks(bfs, bfs->cur_lh, bnum, block, 1, 1);
		release_block(bfs->fd, bnum);
	}

	return 0;
}

static int
free_page_of_ind_block_runs(bfs_info *bfs, bfs_inode *bi, char *block, int start)
{
	int i;
	dr9_off_t bnum;
	block_run *ind_br;
	int bsize = bfs->dsb.block_size;

	if (start < 0  ||  start > bfs->dsb.block_size / sizeof(block_run)) {
		bfs_die("free_page_of_ind_block_runs: invalid value for start, %d, bi @ 0x%lx\n",
				start, bi);
	}

	ind_br = (block_run *) block;
	for (i=start; i < bsize/sizeof(block_run); i++) {
		bnum = block_run_to_bnum(ind_br[i]);
		if (bnum == 0) {
			break;
		}
		CHECK_BNUM(bnum);

		if (free_indirect_block_run(bfs, bi, &ind_br[i], bsize,
									0, NO_LOG) != 0) {
			bfs_die("Problem deleting indirect block run, bi @ 0x%lx\n", (ulong)bi);
		}
		if (free_block_run(bfs, &ind_br[i]) != 0) {
			printf("couldn't free block run in free_partial_indirect (0x%x)\n", ind_br);
			return EINVAL;
		}
		clear_block_run_fields(ind_br[i]);
	}

	return 0;
}

static int
free_dbl_ind_block_run(bfs_info *bfs, bfs_inode *bi, block_run *dbl_ind_br_ptr,
					   int bsize, int start)
{
	int i;
	dr9_off_t bnum, b, tmp;
	char *block;

	if (start < 0 || start > dbl_ind_br_ptr->len) {
		bfs_die("free_dbl_ind_block_run: invalid start, = %d, bi @ 0x%lx, "
				"dbl_ind_br_ptr = 0x%x\n", start, bi, dbl_ind_br_ptr);
	}

	b = block_run_to_bnum(*dbl_ind_br_ptr);
	if (b == 0)
		return 0;

	for (i=start; i < dbl_ind_br_ptr->len; i++) {
		bnum = b + i;
		CHECK_BNUM(bnum);
		if ((block = get_block(bfs->fd, bnum, bsize)) == NULL) {
			printf("Could not get dbl ind block, bi @ 0x%lx, bnum = %d\n",
				   bi, bnum);
			return EINVAL;
		}

		tmp = block_run_to_bnum(*(block_run *)block);
		if (tmp == 0) {
			release_block(bfs->fd, bnum);
			break;
		}
		
		if (free_page_of_ind_block_runs(bfs, bi, block, 0) != 0) {
			bfs_die("Problem deleting ind block run page, bi @ 0x%lx\n", bi);
		}

		log_write_blocks(bfs, bfs->cur_lh, bnum, block, 1, 1);
		release_block(bfs->fd, bnum);
	}

	return 0;
}


static void
get_next_dbl_indirect_pos(bfs_info *bfs, bfs_inode *bi, dr9_off_t offset,
						  dbl_indirect_pos *pos, dbl_indirect_pos *lastpos)
{
	int bsize;
	int dbl_ind_block_index, ind_br_index, ind_block_index, br_index;
	int br_len, brs_per_block;

	if (lastpos == NULL  ||
		(bi->data.max_double_indirect_range != 0  &&
		 lastpos->dbl_ind_block_index == -1)) {
		get_dbl_indirect_pos(bfs, bi, offset, pos);
		/* XXX this function should check if EFBIG happened
		 * (i.e., pos.dbl_ind_block_index < 0) */
		return;
	} else if (lastpos->dbl_ind_block_index == -1) {
		pos->br_index = 0;
		pos->ind_block_index = 0;
		pos->ind_br_index = 0;
		pos->dbl_ind_block_index = 0;
		return;
	}

	bsize = bfs->dsb.block_size;
	br_len = get_dbl_ind_alloc_size(bfs, bi)/bsize;
	brs_per_block = bsize/sizeof(block_run);

	br_index = lastpos->br_index;
	ind_block_index = lastpos->ind_block_index;
	ind_br_index = lastpos->ind_br_index;
	dbl_ind_block_index = lastpos->dbl_ind_block_index;

	br_index++;
	if (br_index >= brs_per_block) {
		br_index = 0;
		ind_block_index++;
		if (ind_block_index >= br_len) {
			ind_block_index = 0;
			ind_br_index++;

			if (ind_br_index >= brs_per_block) {
				ind_br_index = 0;
				dbl_ind_block_index++;

				if (dbl_ind_block_index >= br_len) {
					bfs_die("At end of file int get_next_dbl_indirect_pos, bi @ 0x%lx "
							"offset=%Ld, lastposaddr=0x%lx\n", bi, offset, lastpos);
				}
			}
		}
	}

	pos->br_index = br_index;
	pos->ind_block_index = ind_block_index;
	pos->ind_br_index = ind_br_index;
	pos->dbl_ind_block_index = dbl_ind_block_index;
}

/*
 * XXXmani do we really need so many dr9_off_t divides?
 */
static void
get_dbl_indirect_pos(bfs_info *bfs, bfs_inode *bi, dr9_off_t offset, 
					 dbl_indirect_pos *pos)
{
	dr9_off_t bytes_per_dir_block_run;
	dr9_off_t bytes_per_ind_block;
	dr9_off_t bytes_per_ind_block_run;
	dr9_off_t bytes_per_dbl_ind_block;
	int bsize;
	unsigned int dbl_ind_block_index, ind_br_index, ind_block_index, br_index;
	dr9_off_t offset2, offset3, offset4;
	int br_len, brs_per_block;
	int dbl_ind_alloc_size;

	pos->error = 0;

	bsize = bfs->dsb.block_size;
	dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);
	br_len = dbl_ind_alloc_size/bsize;
	brs_per_block = bsize/sizeof(block_run);

	bytes_per_dir_block_run = dbl_ind_alloc_size;
	bytes_per_ind_block = brs_per_block * bytes_per_dir_block_run;
	bytes_per_ind_block_run = br_len * bytes_per_ind_block;
	bytes_per_dbl_ind_block = brs_per_block * bytes_per_ind_block_run;
	
	dbl_ind_block_index = offset / bytes_per_dbl_ind_block;
	offset2 = offset % bytes_per_dbl_ind_block;
	if (dbl_ind_block_index >= br_len) {
		pos->error = EFBIG; /* indicates EFBIG */
		return;
		/*
		bfs_die("*** what! dbl_ind_block_index = %d, greater than %d\n", 
				dbl_ind_block_index, br_len-1);
				*/
	}

	ind_br_index = offset2 / bytes_per_ind_block_run;
	offset3 = offset2 % bytes_per_ind_block_run;
	if (ind_br_index >= brs_per_block) {
		bfs_die("*** what! ind_br_index = %d, greater than %d\n", 
				ind_br_index, brs_per_block-1);
	}
	
	ind_block_index = offset3 / bytes_per_ind_block;
	offset4 = offset3 % bytes_per_ind_block;
	if (ind_block_index >= br_len) {
		bfs_die("*** what! ind_block_index = %d, greater than %d\n",
				ind_block_index, br_len-1);
	}
	
	br_index = offset4 / bytes_per_dir_block_run;
	if (br_index >= brs_per_block) {
		bfs_die("*** what! br_index = %d, greater than %d\n",
				br_index, brs_per_block-1);
	}

	pos->dbl_ind_block_index = dbl_ind_block_index;
	pos->ind_br_index = ind_br_index;
	pos->ind_block_index = ind_block_index;
	pos->br_index = br_index;

}



static int
shrink_dbl_indirect(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size)
{
	data_stream *ds = &bi->data;
	dbl_indirect_pos pos;
	block_run *ind_br_ptr;
	unsigned int br_index;
	unsigned int ind_br_index;
	unsigned int dbl_ind_block_index;
	unsigned int ind_block_index;
	dr9_off_t dbl_ind_bnum, ind_bnum, b;
	char *block, *block2;
	int ret;
	dr9_off_t offset;
	int bsize;
	int br_len, brs_per_block;
	int dbl_ind_alloc_size;

	bsize = bfs->dsb.block_size;
	dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);
	br_len = dbl_ind_alloc_size/bsize;
	brs_per_block = bsize/sizeof(block_run);

	offset = new_size - ds->max_indirect_range - 1;

	get_dbl_indirect_pos(bfs, bi, offset, &pos);
	if (pos.error != 0) {
		bfs_die("shrink_dbl_indirect: offset 0x%Lx is somehow too big, bi @ 0x%x\n",
				offset, bi);
	}

	br_index = pos.br_index;
	ind_block_index = pos.ind_block_index;
	ind_br_index = pos.ind_br_index;
	dbl_ind_block_index = pos.dbl_ind_block_index;

	br_index++;
	if (br_index >= brs_per_block) {
		br_index = 0;
		ind_block_index++;
		if (ind_block_index >= br_len) {
			ind_block_index = 0;
			ind_br_index++;

			if (ind_br_index >= brs_per_block) {
				ind_br_index = 0;
				dbl_ind_block_index++;

				if (dbl_ind_block_index >= br_len) {
					printf("At end of file while shrinking for some reason (1)\n");
					return 0;
				}
			}
		}
	}

	if (br_index != 0) {
		b = block_run_to_bnum(ds->double_indirect);
		dbl_ind_bnum = b+dbl_ind_block_index;
		CHECK_BNUM(dbl_ind_bnum);
		if ((block = get_block(bfs->fd, dbl_ind_bnum, bsize)) == NULL) {
			return EINVAL;
		}
		ind_br_ptr = &((block_run *) block)[ind_br_index];

		b = block_run_to_bnum(*ind_br_ptr);

		ind_bnum = b+ind_block_index;
		CHECK_BNUM(ind_bnum);
		if ((block2 = get_block(bfs->fd, ind_bnum, bsize)) == NULL) {
			release_block(bfs->fd, dbl_ind_bnum);
			return EINVAL;
		}
		
		ret = free_page_of_block_runs(bfs, bi, block2, br_index);

		log_write_blocks(bfs, bfs->cur_lh, ind_bnum, block2, 1, 1);

		release_block(bfs->fd, ind_bnum);
		release_block(bfs->fd, dbl_ind_bnum);
		if (ret != 0) {
			return EINVAL;
		}
		
		br_index = 0;
		ind_block_index++;
		if (ind_block_index >= br_len) {
			ind_block_index = 0;
			ind_br_index++;
			if (ind_br_index >= brs_per_block) {
				ind_br_index = 0;
				dbl_ind_block_index++;

				if (dbl_ind_block_index >= br_len) {
					printf("At end of file while shrinking for some reason (2)\n");
					return 0;
				}
			}
		}
	}

	if (ind_block_index != 0) {
		b = block_run_to_bnum(ds->double_indirect);
		dbl_ind_bnum = b + dbl_ind_block_index;
		CHECK_BNUM(dbl_ind_bnum);
		if ((block = get_block(bfs->fd, dbl_ind_bnum, bsize)) == NULL) {
			return EINVAL;
		}
		ind_br_ptr = &((block_run *) block)[ind_br_index];
		ret = free_indirect_block_run(bfs, bi, ind_br_ptr, bsize,
									  ind_block_index, WRITE_LOG);

		if (ret != 0) {
			return EINVAL;
		}

		log_write_blocks(bfs, bfs->cur_lh, dbl_ind_bnum, block, 1, 1);
		release_block(bfs->fd, dbl_ind_bnum);

		ind_block_index = 0;
		ind_br_index++;
		if (ind_br_index >= brs_per_block) {
			ind_br_index = 0;
			dbl_ind_block_index++;

			if (dbl_ind_block_index >= br_len) {
				printf("At end of file while shrinking for some reason (3)\n");
				return 0;
			}
		}
	}

	if (ind_br_index != 0) {
		b = block_run_to_bnum(ds->double_indirect);
		dbl_ind_bnum = b  + dbl_ind_block_index;
		CHECK_BNUM(dbl_ind_bnum);
		if ((block = get_block(bfs->fd, dbl_ind_bnum, bsize)) == NULL) {
			return EINVAL;
		}
		ret = free_page_of_ind_block_runs(bfs, bi, block, ind_br_index);

		log_write_blocks(bfs, bfs->cur_lh, dbl_ind_bnum, block, 1, 1);
		release_block(bfs->fd, dbl_ind_bnum);
		if (ret != 0) {
			return EINVAL;
		}

		ind_br_index = 0;
		dbl_ind_block_index++;
		
		if (dbl_ind_block_index >= br_len) {
			printf("At end of file while shrinking for some reason (4)\n");
			return 0;
		}
	}

	ret = free_dbl_ind_block_run(bfs, bi, &ds->double_indirect, 
								 bsize, dbl_ind_block_index);
	if (ret != 0) {
		return EINVAL;
	}
	if (dbl_ind_block_index == 0) {
		free_block_run(bfs, &ds->double_indirect);
		clear_block_run_fields(ds->double_indirect);
	}
	return 0;
}


static int
grow_dbl_indirect(bfs_info *bfs, bfs_inode *bi, block_run *data_br,
				  dbl_indirect_pos *lastpos, int *errptr)
{
	dbl_indirect_pos pos;
	int br_len;
	block_run *ind_br_ptr, *br_ptr;
	dr9_off_t b, dbl_ind_bnum, ind_bnum;
	int ag, bsize;
	char *block, *block2;
	int dbl_ind_alloc_size;
	int ret;
	int i;
	
	*errptr = 0;
	dbl_ind_alloc_size = get_dbl_ind_alloc_size(bfs, bi);
	bsize = bfs->dsb.block_size;
		
	br_len = dbl_ind_alloc_size/bsize;
	bsize = bfs->dsb.block_size;
	ag = data_br->allocation_group;
	
	/* XXXdbg -- don't forget that with cache inhibited files when we
	   allocate the double indirect blocks we need to lock them in
	   the cache (not the data blocks but the block_run blocks */

	/* bi->data.max_double_indirect_range should be a value aligned
	 * to dbl_ind_alloc_size bytes.
	 */

	/* XXXmani fix this to use get_next_dbl... later */
	if (bi->data.max_double_indirect_range == 0) {
		pos.br_index = pos.ind_block_index = pos.ind_br_index =
			pos.dbl_ind_block_index = 0;
	} else {
		get_dbl_indirect_pos(bfs, bi,
							 (bi->data.max_double_indirect_range -
							  bi->data.max_indirect_range) + 1,
							 &pos);
		if (pos.error != 0)
			return pos.error;
	}

	b = block_run_to_bnum(bi->data.double_indirect);
	if (b == 0) {
		if (pos.dbl_ind_block_index != 0) {
			bfs_die("How the hell are we here (1)! bi @ 0x%lx\n", bi);
		}
		ret = allocate_block_run(bfs, ag, br_len,
								 &bi->data.double_indirect, EXACT_ALLOCATION);
		if (ret != 0) {
			printf("*** failed to allocate dbl ind block run bi 0x%lx\n", bi);
			ret = ENOSPC;
			goto clean1;
		} else if (bi->data.double_indirect.len != br_len) {
			printf("*** failed to allocate dbl ind block run bi 0x%lx (2)\n", bi);
			ret = ENOSPC;
			goto clean2;
		}
		dbl_ind_bnum = block_run_to_bnum(bi->data.double_indirect);

		if (pos.dbl_ind_block_index != 0) {
			bfs_die("allocating new dbl ind blocks, but dbl_ind block index = %d, bi @ 0x%lx\n",
					pos.dbl_ind_block_index, bi);
		}


		for (i = 0; i < br_len; i++) {
			CHECK_BNUM(dbl_ind_bnum+i);
		
			if ((block = get_empty_block(bfs->fd, dbl_ind_bnum+i, bsize)) == NULL) {
				printf("Could not get empty dbl indirect block, bi @ 0x%lx, bnum = %d\n",
					   bi, dbl_ind_bnum+i);
				ret = ENOMEM;
				goto clean2;
			}
		
			if (bi->flags & INODE_NO_CACHE) {
				/* this locks it in the cache, baby */
				get_block(bfs->fd, dbl_ind_bnum+i, bsize);
			}
			if (log_write_blocks(bfs, bfs->cur_lh, dbl_ind_bnum+i, block, 1, 1) != 1) {
				printf("* error writing dbl ind page to log after allocation, bi @ 0x%lx\n", bi);
			}

			release_block(bfs->fd, dbl_ind_bnum+i);
		}
	} else {
		dbl_ind_bnum = b + pos.dbl_ind_block_index;
		CHECK_BNUM(dbl_ind_bnum);
	}

	if ((block = get_block(bfs->fd, dbl_ind_bnum, bsize)) == NULL) {
		printf("Could not get dbl indirect block, bi @ 0x%lx, bnum = %d\n",
			   bi, dbl_ind_bnum);
		ret	 = ENOMEM;
		goto clean2;
	}
	
	ind_br_ptr = &((block_run *) block)[pos.ind_br_index];
	b = block_run_to_bnum(*ind_br_ptr);
	if (b == 0) {
		if (pos.ind_block_index != 0) {
			bfs_die("How the hell are we here (2)! bi @ 0x%lx\n", bi);
		}
		ret = allocate_block_run(bfs, ag, br_len, ind_br_ptr, EXACT_ALLOCATION);
		if (ret != 0) {
			printf("*** failed to allocate ind block run bi 0x%lx\n", bi);
			ret = ENOSPC;
			goto clean3;
		} else if (ind_br_ptr->len != br_len) {
			printf("*** failed to allocate ind block run bi 0x%lx (2)\n", bi);
			ret = ENOSPC;
			goto clean4;
		}

		ind_bnum = block_run_to_bnum(*ind_br_ptr);

		for (i = 0; i < br_len; i++) {
			CHECK_BNUM(ind_bnum+i);

			if ((block2 = get_empty_block(bfs->fd, ind_bnum+i, bsize)) == NULL) {
				printf("Could not get empty ind block, bi @ 0x%lx, bnum = %d\n",
					   bi, ind_bnum+i);
				ret = EINVAL;
				goto clean4;
			}
			if (bi->flags & INODE_NO_CACHE) {
				/* this locks it in the cache, baby */
				get_block(bfs->fd, ind_bnum+i, bsize);
			}

			if (log_write_blocks(bfs, bfs->cur_lh, ind_bnum+i, block2, 1, 1) != 1) {
				printf("* error writing ind page (within dbl) to log after allocation, bi @ 0x%lx\n",
					   bi);
			}
			release_block(bfs->fd, ind_bnum+i);
		}
	} else {
		ind_bnum = b + pos.ind_block_index;
		CHECK_BNUM(ind_bnum);
	}
	
	if ((block2 = get_block(bfs->fd, ind_bnum, bsize)) == NULL) {
		printf("Could not get ind block, bi @ 0x%lx, bnum = %d\n",
			   bi, ind_bnum);
		ret = EINVAL;
		goto clean4;
	}

	br_ptr = &((block_run *) block2)[pos.br_index];

	b = block_run_to_bnum(*br_ptr);
	if (b != 0) {
		bfs_die("How is this? bnum is %Ld but we need an empty block, "
				"bi @ 0x%lx, br @ 0x%lx pos @ 0x%lx\n", b, bi, br_ptr, &pos);
	}
	
	*br_ptr = *data_br;

	/* XXXmani this log_write_blocks() call should be moved out of
	   this loop and be done afterwards instead of each time through */

	if (log_write_blocks(bfs, bfs->cur_lh, ind_bnum, block2, 1, 1) != 1) {
		printf("* error write ind page %Ld bi 0x%lx\n", ind_bnum, bi);
	}
	if (log_write_blocks(bfs, bfs->cur_lh, dbl_ind_bnum, block, 1, 1) != 1) {
		printf("* error writing dbl ind page %Ld bi 0x%lx\n", dbl_ind_bnum, bi);
	}

	memcpy(lastpos, &pos, sizeof(pos));
	release_block(bfs->fd, ind_bnum);
	release_block(bfs->fd, dbl_ind_bnum);
	return 0;
	
 clean4:
	free_block_run(bfs, ind_br_ptr);
	clear_block_run_fields(*ind_br_ptr);
 clean3:
	release_block(bfs->fd, dbl_ind_bnum);
 clean2:
    /* we gotta free up what we just allocated */
	if (bi->data.max_double_indirect_range == 0) {
		free_block_run(bfs, &bi->data.double_indirect);
		clear_block_run_fields(bi->data.double_indirect);
	}
 clean1:
	
	return ret;
}


static int
shrink_indirect(bfs_info *bfs, bfs_inode *bi, dr9_off_t rounded_size)
{
	dr9_off_t b, ind_bnum;
	int j, i = 0;
	block_run *indirect;
	dr9_off_t sum;
	int bsize, brs_per_block;

	bsize = bfs->dsb.block_size;
	brs_per_block = bsize/sizeof(block_run);

	sum = bi->data.max_direct_range;
	ind_bnum = b = block_run_to_bnum(bi->data.indirect);

	if (bi->data.indirect.len == 0) {
		bfs_die("*** this is bad, no indirect blocks in shrink_indirect, bi @ 0x%lx\n",
				bi);
	}

	for (j=0; j < bi->data.indirect.len; j++) {
		if (sum >= rounded_size)
			break;

		ind_bnum = b+j;
		CHECK_BNUM(ind_bnum);
		indirect = (block_run *) get_block(bfs->fd, ind_bnum, bsize);
		if (indirect == NULL) {
			printf("could not read indirect block @ %Ld, bi @ 0x%lx\n", ind_bnum, bi);
			return EINVAL;
		}

		for (i=0; i < brs_per_block; i++) {
			if (sum >= rounded_size) 
				break;

			if (indirect[i].start == 0  &&  indirect[i].len == 0) 
				break;

			sum += indirect[i].len * bsize;
		}
		release_block(bfs->fd, ind_bnum);
	}

	if (sum < rounded_size && i >= brs_per_block) 
		bfs_die("should not be here: bi @ 0x%lx sum %Ld, i %d, "
				"brs_per_block %d, rounded_size %Ld\n", bi, sum, i,
				brs_per_block, rounded_size);

	/* get the block again because we released it above */

	CHECK_BNUM(ind_bnum);
	indirect = (block_run *) get_block(bfs->fd, ind_bnum, bsize);
	if (indirect == NULL) {
		printf("could not read indirect block @ %Ld, bi @ 0x%lx (2)\n", ind_bnum, bi);
		return EINVAL;
	}

	if (i > 0  &&  sum > rounded_size) { /* then we have to shrink this block run */
		dr9_off_t tmp;
		block_run to_free;

		i--;  /* because the for loop would have gone one beyond */
		
		to_free = indirect[i];
		
		tmp = to_free.len - ((sum - rounded_size) / bsize);
		to_free.start   += tmp;
		to_free.len     -= tmp;
		indirect[i].len  = tmp;
		
		sum = rounded_size;
		
		if (to_free.len && free_block_run(bfs, &to_free) != 0) {
			bfs_die("bad br: 1: for indirect %d, bi @ 0x%lx\n", i,
					(ulong)bi);
		}
		
		i++;  /* so the free_page_of_block_runs below free's the right things */
	}

	/* free the rest of the block runs on this page */
	if (free_page_of_block_runs(bfs, bi, (void *) indirect, i) != 0) {
		bfs_die("bad br: 2: for indirect %d, bi @ 0x%lx\n", i,
				(ulong)bi);
	}

	if (i > 0)   /* then we modified only part of the page */
		log_write_blocks(bfs, bfs->cur_lh, ind_bnum, indirect, 1, 1);
	release_block(bfs->fd, ind_bnum);

	if (free_indirect_block_run(bfs, bi, &bi->data.indirect, bsize,
								j, WRITE_LOG) != 0)
		return EINVAL;
		
	if (j == 0) {
		/* really should not ever get here but let's keep this code
		 * here just to be safe. 
		 */
		free_block_run(bfs, &bi->data.indirect);
		clear_block_run_fields(bi->data.indirect);
	}

	return 0;
}

