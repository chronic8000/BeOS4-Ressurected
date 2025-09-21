#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bfs.h"


/* internal prototypes */
static int  SetBV(BitVector *bv, int which);
static int  UnSetBV(BitVector *bv, int which);
static int  UnSetRangeBV(BitVector *bv, int lo, int hi);
static int  IsSetBV(BitVector *bv, int which);
static int  GetFreeRangeOfBits(BitVector *bv, int len, int *biggest_free_chunk);
static int  fix_up_block_run(bfs_info *bfs, block_run *br);
#if DEBUG
static void  DumpBV(BitVector *bv);
#endif


int
create_block_bitmap(bfs_info *bfs)
{
	char      *buff = NULL;
	int        ret, i, n, num_ags, blocks_per_ag, bsize, x;
	size_t     num_bytes, num_blocks;
	ssize_t    amt;
	BitVector *bv;
	dr9_off_t  disk_size;

	bfs->bbm_sem = create_sem(1, "bbm");
	if (bfs->bbm_sem < 0)
		return ENOMEM;

	bsize = bfs->dsb.block_size;

	/*
	   calculate how many bitmap blocks we'll need by rounding up the
	   number of blocks on the disk to the nearest multiple of 8 times
	   the block size (8 == number of bits in a byte).
	*/   
	n  = bfs->dsb.num_blocks / 8;
	n  = ((n + bsize - 1) & ~(bsize - 1));
	n /= bsize;

	bfs->bbm.num_bitmap_blocks = n;
	

	disk_size = bfs->dsb.num_blocks * bsize;
	blocks_per_ag = 1 + ((disk_size / (dr9_off_t)(1024*1024*1024)) / 4);


    for(i=0; i < sizeof(dr9_off_t) * 8; i++)
        if ((blocks_per_ag * bsize * 8) & (1 << i))
            break;

    if (i >= sizeof(dr9_off_t) * 8) {
        printf("*** error: blocks_per_ag is wacky! (%d)\n", 
                       blocks_per_ag);
        ret = EINVAL;
        goto err;
    }
        
    if (blocks_per_ag > 2 &&
		((blocks_per_ag * bsize * 8) & ((1 << i) - 1)) != (1 << i)) {
		/*
		   XXXdbg is there a better way to round an arbitrary int up to
		          the next power of two?
		*/
		if (blocks_per_ag < 4)
			blocks_per_ag = 4;
		else if (blocks_per_ag >= 4 && blocks_per_ag < 8)
			blocks_per_ag = 8;
		else if (blocks_per_ag >= 8)
			blocks_per_ag = 16;

        printf("-- blocks_per_ag adjusted to %d\n", blocks_per_ag);
    }


	bfs->dsb.blocks_per_ag = blocks_per_ag;
	if (bfs->dsb.blocks_per_ag * bsize * 8 > 65536) {
		blocks_per_ag = 65536 / (bsize * 8);
		printf("blocks per ag of %d overflows a block run. truncating to %d\n",
			   bfs->dsb.blocks_per_ag, blocks_per_ag);
		bfs->dsb.blocks_per_ag = blocks_per_ag;
	}


    for(i=0; i < sizeof(dr9_off_t) * 8; i++)
        if ((blocks_per_ag * bsize * 8) & (1 << i))
            break;
    if ((blocks_per_ag * bsize * 8) != (1 << i)) {
		printf("can not be! blocks per ag %d, bsize %d, i == %d\n",
			   blocks_per_ag, bsize, i);
		ret = EINVAL;
		goto err;
	}
	
    bfs->dsb.ag_shift = i;

	num_ags = ((n + (blocks_per_ag-1)) & ~(blocks_per_ag-1)) / blocks_per_ag;

	bfs->dsb.num_ags = num_ags;
	
	bfs->bbm.bv = (BitVector *)calloc(num_ags * sizeof(BitVector), 1);
	if (bfs->bbm.bv == NULL) {
		ret = ENOMEM;
		goto err;
	}


	num_bytes = bfs->bbm.num_bitmap_blocks * bfs->dsb.block_size;
	buff = (char *)calloc(num_bytes, 1);
	if (buff == NULL) {
		ret = ENOMEM;
		goto err;
	}
	
	for(i=0, bv=bfs->bbm.bv; i < num_ags; i++, bv++) {
		if (((i+1) * blocks_per_ag * bfs->dsb.block_size * 8) < bfs->dsb.num_blocks)
			bv->numbits = bfs->dsb.block_size * 8 * blocks_per_ag;
		else
			bv->numbits = bfs->dsb.num_blocks - (i * blocks_per_ag * bfs->dsb.block_size * 8);
		bv->next_free   = 0;
		bv->bits        = (int *)&(buff[i * bfs->dsb.block_size * blocks_per_ag]);
	}

	/* fill in used blocks, including the superblock */
	num_blocks = bfs->bbm.num_bitmap_blocks + 1;
	if (GetFreeRangeOfBits(bfs->bbm.bv, num_blocks, NULL) != 0) {
		printf("*** failed to allocate %Ld bits in create_bitmap\n",
			   bfs->bbm.num_bitmap_blocks + 1);

		ret = ENOSPC;
		goto err;
	}


	/* write out the bitmap blocks, starting at block # 1 */
	amt = write_blocks(bfs, 1, buff, n);
	if (amt != n)  {
		ret = EINVAL;		
		goto err;
	}

	bfs->dsb.used_blocks += num_blocks;
		
	return 0;

 err:
	if (buff)
		free(buff);
	if (bfs->bbm.bv)
		free(bfs->bbm.bv);
	bfs->bbm.bv = NULL;
	
	if (bfs->bbm_sem > 0)
		delete_sem(bfs->bbm_sem);
	bfs->bbm_sem = -1;
	return ret;
}

#ifdef DEBUG
int dump_ag(int argc, char **argv);
int dump_bitmap(int argc, char *argv[]);
#endif

int
init_block_bitmap(bfs_info *bfs)
{
	char      *buff = NULL;
	int        i, n, ret, num_ags, blocks_per_ag, bsize, x;
	size_t     num_bytes;
	ssize_t    amt;
	BitVector *bv;

	bfs->bbm_sem = create_sem(1, "bbm");
	if (bfs->bbm_sem < 0)
		return ENOMEM;

	bsize = bfs->dsb.block_size;

	n  = bfs->dsb.num_blocks / 8;
	n  = ((n + bsize - 1) & ~(bsize - 1));
	n /= bsize;

	bfs->bbm.num_bitmap_blocks = n;
	
	blocks_per_ag = bfs->dsb.blocks_per_ag;
		
	num_ags = ((n + (blocks_per_ag-1)) & ~(blocks_per_ag-1)) / blocks_per_ag;
	if (bfs->dsb.num_ags != num_ags) {
		printf("*** warning num_ags is wrong! (%d != %d)\n",
			   bfs->dsb.num_ags, num_ags);
		bfs->dsb.num_ags = num_ags;
	}

	if (bfs->flags & FS_READ_ONLY) {
		bfs->bbm.bv = NULL;    /* we don't need to allocate it */
		return 0;
	}

	bfs->bbm.bv = (BitVector *)calloc(num_ags * sizeof(BitVector), 1);
	if (bfs->bbm.bv == NULL) {
		ret = ENOMEM;
		goto err;
	}


	num_bytes = bfs->bbm.num_bitmap_blocks * bfs->dsb.block_size;
	buff = (char *)malloc(num_bytes);
	if (buff == NULL) {
		ret = ENOMEM;
		goto err;
	}
	

	amt = read_blocks(bfs, 1, buff, n);
	if (amt != n) {
		printf("loading bitmap: failed to read %d bitmap blocks\n", n);
		ret = EINVAL;
		goto err;
	}

	for(i=0, bv=bfs->bbm.bv; i < num_ags; i++, bv++) {
		if (((i+1) * blocks_per_ag * bfs->dsb.block_size * 8) < bfs->dsb.num_blocks)
			bv->numbits = bfs->dsb.block_size * 8 * blocks_per_ag;
		else
			bv->numbits = bfs->dsb.num_blocks - (i * blocks_per_ag * bfs->dsb.block_size * 8);
		bv->next_free   = 0;
		bv->bits        = (int *)&(buff[i * bfs->dsb.block_size * blocks_per_ag]);
	}

#ifdef DEBUG
	add_debugger_command("agdump", dump_ag,
					 "agdump <bfs ptr> <ag #> - dump an allocation group");
# ifndef BOOT
	add_debugger_command("bitmap", dump_bitmap,
					 "bitmap <bfs_addr> <start> <end> - dump an allocation bitmap");
# endif /* BOOT */	
#endif

	if (IsSetBV(&bfs->bbm.bv[0], 0) == 0) {
		printf("bfs: danger! super block is not allocated! patching up...\n");
		SetBV(&bfs->bbm.bv[0], 0);
		write_blocks(bfs, 1, bfs->bbm.bv[0].bits, 1);
	}

	if (check_block_run(bfs, &bfs->dsb.log_blocks, 1) == 0) {
		printf("bfs: danger! log blocks not allocated!\n");
		fix_up_block_run(bfs, &bfs->dsb.log_blocks);
	}

	if (check_block_run(bfs, &bfs->dsb.root_dir, 1) == 0) {
		printf("bfs: danger! root dir not allocated!\n");
		fix_up_block_run(bfs, &bfs->dsb.root_dir);
	}

	if (check_block_run(bfs, &bfs->dsb.indices, 1) == 0) {
		printf("bfs: danger! index dir not allocated!\n");
		fix_up_block_run(bfs, &bfs->dsb.indices);
	}

	printf("Checking block bitmap...\n");
	sanity_check_bitmap(bfs);
	printf("Done checking bitmap.\n");


	return 0;

 err:
	if (buff)
		free(buff);
	if (bfs->bbm.bv)
		free(bfs->bbm.bv);
	bfs->bbm.bv = NULL;
	if (bfs->bbm_sem > 0)
		delete_sem(bfs->bbm_sem);
	bfs->bbm_sem = -1;
	return ret;
}


void
shutdown_block_bitmap(bfs_info *bfs)
{
	if (bfs->bbm.bv) {
		if (bfs->bbm.bv->bits) {
			free(bfs->bbm.bv->bits);
			bfs->bbm.bv->bits = NULL;
		}
		free(bfs->bbm.bv);
		bfs->bbm.bv = NULL;
	}

	if (bfs->bbm_sem > 0)
		delete_sem(bfs->bbm_sem);
	bfs->bbm_sem = -1;
}


int
real_allocate_blocks(bfs_info *bfs, uint ag, int num_blocks, block_run *br,
					 int do_log_write, int exact)
{
	int        start = -1, i, n, len, bsize = bfs->dsb.block_size, nblocks;
	int        biggest_free_chunk = 0, max_free_chunk = 1, ag_of_max_chunk = 0;
	char      *ptr;
	BitVector *bv;

	if (bfs->flags & FS_READ_ONLY)
		return EPERM;

	acquire_sem(bfs->bbm_sem);

	if (num_blocks > (bfs->dsb.num_blocks - bfs->dsb.used_blocks)) {
		release_sem(bfs->bbm_sem);
		return ENOSPC;
	}

	for(nblocks=num_blocks; nblocks >= 1; nblocks /= 2) {
		for(i=0; i < bfs->dsb.num_ags; i++) {
			bv = &bfs->bbm.bv[(ag + i) % bfs->dsb.num_ags];
			start = GetFreeRangeOfBits(bv, nblocks, &biggest_free_chunk);
 			if (start != -1)
				break;
 
			if (biggest_free_chunk > max_free_chunk) {
				ag_of_max_chunk = (ag + i) % bfs->dsb.num_ags;
				max_free_chunk  = biggest_free_chunk;
			}

			if (exact == LOOSE_ALLOCATION && biggest_free_chunk > 0 &&
				biggest_free_chunk >= (nblocks>>4)) {
				nblocks = biggest_free_chunk;
				start = GetFreeRangeOfBits(bv, nblocks, NULL);
				if (start != -1)
					break;
			}
		}

		if (start != -1 || exact == EXACT_ALLOCATION)
			break;

		if (start == -1 && exact == TRY_HARD_ALLOCATION && max_free_chunk > 1) {
			if (max_free_chunk < nblocks)
				nblocks = max_free_chunk;

			bv = &bfs->bbm.bv[ag_of_max_chunk];
			start = GetFreeRangeOfBits(bv, nblocks, &biggest_free_chunk);
			if (start != -1) {
				i  = 0;
				ag = ag_of_max_chunk;
				break;
			}
		}

		if (exact == LOOSE_ALLOCATION && nblocks > max_free_chunk*2) {
			nblocks = max_free_chunk * 2;  /* times 2 because of the loop continuation */
		}

		max_free_chunk = 1;
	}

	if (start == -1) {
		release_sem(bfs->bbm_sem);
		return ENOSPC;
	}
	
	num_blocks = nblocks;

	br->allocation_group = (ag + i) % bfs->dsb.num_ags;
	br->start            = (ushort)start;
	br->len              = (ushort)num_blocks;

	/*
	   calculate the block number of this allocation group. the +1 is
	   accounting for the super block.
    */
	n  = (br->allocation_group * bfs->dsb.blocks_per_ag) + 1; 
	n += (start / 8 / bsize);

	/* 
		The variable "len" is the number of bitmap blocks we must 
		write.  The calculation takes the bitmap block number of 
		the last block allocated and subtracts the bitmap block number 
		of the first block allocated to get the number of bitmap 
		blocks that we are touching.
	*/
	len = (((start + num_blocks - 1) / 8 / bsize) - (start / 8 / bsize)) + 1;

	if (start + br->len > (bfs->dsb.blocks_per_ag * bsize * 8)) {
		bfs_die("allocated blocks out of the ag? start %d len %d (max %d)\n",
				start, br->len, bsize * 8);
	}
	
	ptr = (char *)bv->bits + (((start / 8) / bsize) * bsize);

	if (do_log_write)  {
		for(i=0; i < len; i++, ptr += bsize) {
			if (log_write_blocks(bfs, bfs->cur_lh, n+i, ptr, 1, 1) != 1) {
				printf("error:1 failed to write bitmap block run %d:1!\n",n+i);
				release_sem(bfs->bbm_sem);
				return EINVAL;
			}
		}
	} else if (write_blocks(bfs, n, ptr, len) != len) {
		printf("error: 2 failed to write back bitmap block @ block %d!\n", n);
		release_sem(bfs->bbm_sem);
		return EINVAL;
	}

	bfs->dsb.used_blocks += num_blocks;
	release_sem(bfs->bbm_sem);

	if (br->allocation_group >= bfs->dsb.num_ags)
		bfs_die("bogus ag: %d (max %x)\n", br->allocation_group,
				bfs->dsb.num_ags);

	bfs->dsb.flags = BFS_DIRTY;

	return 0;
}

int
allocate_block_run(bfs_info *bfs, uint ag, int num_blocks, block_run *br,
				   int exact)
{
	return real_allocate_blocks(bfs, ag, num_blocks, br, 1, exact);
}


int
pre_allocate_block_run(bfs_info *bfs, uint ag, int num_blocks, block_run *br)
{
	return real_allocate_blocks(bfs, ag, num_blocks, br, 0, LOOSE_ALLOCATION);
}


int
free_block_run(bfs_info *bfs, block_run *br)
{
	int        i, n, ag, start, len, bsize = bfs->dsb.block_size;
	char      *ptr;
	BitVector *bv;

	if (bfs->flags & FS_READ_ONLY)
		return EPERM;

	ag    = br->allocation_group;
	start = br->start;
	
	if (ag >= bfs->dsb.num_ags) {
		printf("bitmap: bad ag %d (max %d) br @ 0x%lx\n", ag, 
               bfs->dsb.num_ags, (ulong)br);
		return EINVAL;
	}

	acquire_sem(bfs->bbm_sem);

	bv = &bfs->bbm.bv[ag];
	UnSetRangeBV(bv, br->start, br->len);

	bv->is_full = 0;
	
	/* see the comment in real_allocate_block_run for more details on this */
	n  = (br->allocation_group * bfs->dsb.blocks_per_ag) + 1; 
	n += (start / 8 / bsize);

    /*
		The variable "len" is the number of bitmap blocks we must
		write.  The calculation takes the bitmap block number of
		the last block to be freed and subtracts the bitmap block number
		of the first block to be freed to get the number of bitmap
		blocks that we are touching.
    */
    len = (((start + br->len - 1) / 8 / bsize) - (start / 8 / bsize)) + 1;

	if (start + br->len > (bfs->dsb.blocks_per_ag * bsize * 8)) {
		bfs_die("free'd blocks out of the ag? start %d len %d (max %d)\n",
				start, br->len, bsize * 8);
	}
	
	ptr = (char *)bv->bits + (((start / 8) / bsize) * bsize);

	for(i=0; i < len; i++, ptr += bsize) {
		if (log_write_blocks(bfs, bfs->cur_lh, n+i, ptr, 1, 1) != 1) {
			printf("error: failed to write back bitmap block run %d:1!\n",n+i);
			release_sem(bfs->bbm_sem);
			return EINVAL;
		}
	}

	bfs->dsb.used_blocks -= br->len;

	release_sem(bfs->bbm_sem);

	bfs->dsb.flags = BFS_DIRTY;

	return 0;
}



int
check_block_run(bfs_info *bfs, block_run *br, int state)
{
	int        i;
	BitVector *bv;

	if (bfs->flags & FS_READ_ONLY)
		return EPERM;

	acquire_sem(bfs->bbm_sem);

	bv = &bfs->bbm.bv[br->allocation_group];
	for(i=0; i < br->len; i++) {
		if ((state == 1 && !IsSetBV(bv, br->start + i)) ||
			(state == 0 && IsSetBV(bv, br->start + i))) {
			break;
		}
	}

	release_sem(bfs->bbm_sem);
 
	if (i != br->len) {
#if 0
		printf("Danger! bit %d:%d:%d (0x%Lx) is not %s (bv @ 0x%lx)\n",
			   br->allocation_group, br->start, br->len,
			   block_run_to_bnum(*br),
			   (state == 0) ? "clear" : "set",
			   (ulong)bv);
#endif
		return 0;
	}

	return 1;
}



static int
fix_up_block_run(bfs_info *bfs, block_run *br)
{
	int        i;
	BitVector *bv;

	acquire_sem(bfs->bbm_sem);

	bv = &bfs->bbm.bv[br->allocation_group];
	for(i=0; i < br->len; i++) 
		if (SetBV(bv, br->start + i) == 0)
			break;

	release_sem(bfs->bbm_sem);
 
	if (i != br->len) {
		printf("fix_up_block_run: bit %d:%d:%d (0x%Lx) didn't get set (bv @ 0x%lx)\n",
			   br->allocation_group, br->start, br->len,
			   block_run_to_bnum(*br), (ulong)bv);
		return 0;
	}

	return 1;
}


#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif



/* Set a bit in a BitVector.
 *
 *   Inputs : bv is a valid bit vector pointer
 *            which indicates which bit specifically to set in bv
 *
 *   returns : TRUE if bit was set, FALSE otherwise... 
 */
static int
SetBV(BitVector *bv, int which)
{
    int i,j;
    
    if (which >= bv->numbits || which < 0)
		return FALSE;
    
    i = which / BITS_IN_CHUNK;       /* i == index to bv->bits         */
    j = which % BITS_IN_CHUNK;       /* j == bit inside of bv->bits[i] */
    
    bv->bits[i] |= (1 << j);
    
    return TRUE;
}


/* UNSet a bit in a BitVector.
 *
 *   Inputs : bv is a valid bit vector pointer
 *            which indicates which bit specifically to unset in bv
 *
 *   returns : TRUE if bit was unset, FALSE otherwise... 
 */
static int
UnSetBV(BitVector *bv, int which)
{
    int i,j;
    
    if (which >= bv->numbits || which < 0)
		return FALSE;
    
    i = which / BITS_IN_CHUNK;       /* i == index to bv->bits         */
    j = which % BITS_IN_CHUNK;       /* j == bit inside of bv->bits[i] */
    
    bv->bits[i]   &= ~(1 << j);
    bv->next_free  = which;
    
    return TRUE;
}


static int
UnSetRangeBV(BitVector *bv, int start, int len)
{
    int i,j,k;
    
    if (start < 0 || len < 0 || start+len > bv->numbits)
		return FALSE;
    
    bv->next_free = start;
    
    for(k=start; k < start+len; k++) {
		i = k / BITS_IN_CHUNK; /* i == index to bv->bits         */
		j = k % BITS_IN_CHUNK; /* j == bit inside of bv->bits[i] */
     
		bv->bits[i] &= ~(1 << j);
    }     
    
    return TRUE;
}



/* Test if a bit is set in a bit vector... 
 *
 *  Inputs : bv, a valid bit vector pointer
 *           which, which bit to test 
 *
 *  Returns : true if the bit was set, false otherwise.
 */
static int
IsSetBV(BitVector *bv, int which)
{
    int i,j;
    
    if (which >= bv->numbits || which < 0)
		return FALSE;
    
    i = which / BITS_IN_CHUNK;
    j = which % BITS_IN_CHUNK;
    
    return (int)(bv->bits[i] & (1 << j));
}


/* Get the first range of free bits that is len bits long
 *
 *   Inputs : bm is a valid bitmap pointer
 *            len is the number of bits needed in the range 
 *
 *   returns: the number of the first free bit or -1 on failure
 *
 */
static int
GetFreeRangeOfBits(BitVector *bv, int len, int *biggest_free_chunk)
{
    int max_free = -1;
    int i,j,k,n, begin, base, real_count;
    int count, start, bit, is_full = 1;
  
	if (biggest_free_chunk)
		*biggest_free_chunk = -1;

    if (len <= 0 || len > bv->numbits || bv->is_full)
		return -1;
	
    count = bv->numbits / BITS_IN_CHUNK;
    if ((bv->numbits % BITS_IN_CHUNK) != 0)
		count++;
	
    base = bv->next_free / BITS_IN_CHUNK;
  
    for(i=0, real_count=0; i < count; i++, real_count++) {
		if (real_count > count)
			bfs_die("bitmap loop! count %d, real_count %d, i %d bv @ 0x%lx\n",
					count, real_count, i, (ulong)bv);

		begin = (base + i) % count;

		if (bv->bits[begin] == (chunk)(~0))
			continue;
     
		for(j=0; j < BITS_IN_CHUNK; j++) {
			if ((bv->bits[begin] & (1 << j)) == 0) {      /* aha! a free bit */
				start = (begin * BITS_IN_CHUNK) + j; 
				bit = j;  n = begin;
	   
				/* if this bit is in the range, then this BV is not full */
				if ((n*BITS_IN_CHUNK+bit) < bv->numbits)
					is_full = 0;

				/*
				 * Here we start to see if we can get a range of bits,
				 * being careful if we cross chunk boundaries.
				 */
				for(k=0; k < len && (n*BITS_IN_CHUNK+bit) < bv->numbits; k++,bit++) {
					if (((bv->bits[n]) & (1 << bit)) != 0)
						break;
	      
					if ((bit+1) == BITS_IN_CHUNK) {
						n++;
						bit = -1;  /* gets incremented on loop continuation */
					}
				}

				if (k > max_free) {
					max_free = k;
				}

				if ((n*BITS_IN_CHUNK+bit) >= bv->numbits && k < len) {
					/* i += (n - begin - 1); */
					if (n > begin)
						i += (n - begin);
					else
						i += 1;
					break;
				}

				if (k < len) { /* didn't find a big enough range here */
					i += (n - begin); /* advance over what we just checked */
					begin = n;
					j = bit;
					continue;
				}

				if ((begin*BITS_IN_CHUNK+j + len) > bv->numbits) 
					break;      /* too big for the # of bits */

				/*
				 * Now go back through and set all the bits
				 */
				bit = j; n = begin;
				for(k=0; k < len; k++, bit++) {
					bv->bits[n] |= (1 << bit);
	      
					if ((bit+1) == BITS_IN_CHUNK) {
						n++;
						bit = -1;  /* gets incremented on loop continuation */
					}
				}

				if (n < count) {
					if ((bv->bits[n] & (1 << bit)) == 0)
						bv->next_free = start + len;
				}

				return (start);
			}  /* end of if (...) aha! a free bit */
			
		}  /* end of for(j=0; ...) (loop on each bit) */
		
	}  /* end of for(i=0; ...) (loop on each chunk) */


	if (is_full)                 /* then mark this guy as full */
		bv->is_full = 1;

	if (biggest_free_chunk)
		*biggest_free_chunk = max_free;


    /* if we get here, we didn't find nuthin. */
    return -1;
}


#ifdef DEBUG

#ifndef USER
#undef  printf
#define printf kprintf
#endif  /* USER */

int
dump_ag(int argc, char **argv)
{
	int        which_ag;
	bfs_info  *bfs;
	BitVector *bv;
	
	if (argc < 3) {
		printf("%s need a bfs info struct (see `bfs') and ag #\n", argv[0]);
		return 1;
	}
		
	bfs      = (bfs_info *)strtoul(argv[1], NULL, 0);
	which_ag = strtoul(argv[2], NULL, 0);

	if (which_ag < 0 || which_ag > bfs->dsb.num_ags) {
		printf("bad ag number %d\n", which_ag);
		return 1;
	}

	bv = &bfs->bbm.bv[which_ag];
	DumpBV(bv);

	return 1;
}

static void
DumpBV(BitVector *bv)
{
    int i, j;
    int alloc_len=0, alloced = 0,
        free_len=0,  freed = 0;

    printf("Number of bits == %d\n", bv->numbits);
    for(i=0; i < bv->numbits; i++) {
	if (IsSetBV(bv, i)) {
	    for(j=i+1; IsSetBV(bv, j) && j < bv->numbits; j++)
		;
	    printf("allocated: %5d - %5d\n", i, j-1);
	    alloc_len += (j - i);
	    alloced++;
	    i = j - 1;
	} else {
	    for(j=i+1; IsSetBV(bv, j) == 0 && j < bv->numbits; j++)
		;
	    printf("free:      %5d - %5d\n", i, j-1);
	    free_len += (j - i);
	    freed++;
	    i = j - 1;
	}
    }
    printf("Next free bit == %d\n", bv->next_free);
    printf("average allocated length %f (total %d, num %d)\n",
	   (float)alloc_len/(float)alloced, alloc_len, alloced);
    printf("average free length %f (total %d, num %d)\n",
	   (float)free_len/(float)freed, free_len, freed);
}

#ifndef BOOT

int
dump_bitmap(int argc, char *argv[])
{
	int start, end;
	bfs_info *bfs;
	int i;

	if (argc < 2) {
		printf("usage: %s <bfs_addr> <start> <end>\n", argv[0]);
		return 1;
	} else if (argc == 2) {
		bfs = (bfs_info *) parse_expression(argv[1]);
		start = 0;
		end = bfs->bbm.num_bitmap_blocks;
	} else if (argc == 3) {
		bfs = (bfs_info *) parse_expression(argv[1]);
		start = parse_expression(argv[2]);
		end = bfs->bbm.num_bitmap_blocks;
	} else {
		bfs = (bfs_info *) parse_expression(argv[1]);
		start = parse_expression(argv[2]);
		end = parse_expression(argv[3]);
	}

	for (i = start; i < end; i++) {
		printf("Allocation Group %d\n", i);
 		DumpBV(&bfs->bbm.bv[i]);
	}

	return 0;
}

#endif /* BOOT */

#endif /* DEBUG */



/*
	this is evil and dangerous but is necessary for
	debugging.  it just or's in random values in the bitmap
	so that the disk will become extremely fragmented.
*/   
void
fragment_bitmap(bfs_info *bfs, uint32 *val)
{
	int        i, j, k, *ptr;
	off_t      used_blocks = 0;
	BitVector *bv;

	srand(time(NULL) | 1);

	for(i=0; i < bfs->dsb.num_ags; i++) {
		bv = &bfs->bbm.bv[i];
		ptr = bv->bits;
		for(j=0; j < bv->numbits/(sizeof(int)*8); j++)
			ptr[j] |= (val ? *val : rand());
	}

	
	for(i=0; i < bfs->dsb.num_ags; i++) {
		bv = &bfs->bbm.bv[i];
		ptr = bv->bits;

		for(j=0; j < bv->numbits/(sizeof(int)*8); j++) {
			for(k=0; k < 32; k++)
				if (ptr[j] & (1 << k))
					used_blocks++;
		}
	}

	bfs->dsb.used_blocks = used_blocks;
	write_blocks(bfs, 1, bfs->bbm.bv[0].bits, bfs->bbm.num_bitmap_blocks);
	write_super_block(bfs);
}


void
sanity_check_bitmap(bfs_info *bfs)
{
	int        i, j, k, *ptr;
	off_t      used_blocks = 0;
	BitVector *bv;

	for(i=0; i < bfs->dsb.num_ags; i++) {
		bv = &bfs->bbm.bv[i];
		ptr = bv->bits;

		for(j=0; j < bv->numbits/(sizeof(int)*8); j++) {
			for(k=0; k < sizeof(chunk)*CHAR_BIT; k++)
				if (ptr[j] & (1 << k))
					used_blocks++;
		}
	}

	if (bfs->dsb.used_blocks != used_blocks) {
		printf("*** super block sez %Ld used blocks but it's really %Ld\n",
			   bfs->dsb.used_blocks, used_blocks);
		bfs->dsb.used_blocks = used_blocks;
	}
}
