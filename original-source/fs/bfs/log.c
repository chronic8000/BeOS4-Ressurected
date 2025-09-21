#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bfs.h"

typedef struct log_entry {
	int       num_entries;          /* # of entries in the blocks array */
	int       max_entries;          /* max # of entries we have space for */
	block_run blocks[1];            /* an array of block_runs */
} log_entry;


typedef struct entry_list {
	int                num_blocks;  /* number of blocks in the log_entry */
	log_entry         *lent;
	struct entry_list *next;
} entry_list;

typedef struct log_handle {
	entry_list *el;
	int         flags;       
    uint        num_blocks;         /* total # of blocks in this transaction */
	uint        num_el;             /* # of entry list structures */
	uint        num_blocks_flushed; /* # of blocks actually written to disk */
	dr9_off_t       start, end;         /* where in the log this guy goes */
	bfs_info   *bfs;
} log_handle;

#define LH_REALLY_FLUSH    0x0001



#define BLOCKS_PER_LOG_ENTRY (bfs->blocks_per_le)
#define MAX_LOG_SIZE         (256 * 1024)

/*
   this function insures that the log entry buffer isn't too large
   (where too large is defined by me to be 256k).  This should probably
   be a configuration option....
*/   
void
calc_blocks_per_le(bfs_info *bfs)
{
	int tmp;

	/* this formula calculates the number of blocks we can fit address with
	   one file system block (minus 8 bytes for the log_entry header info)
	   and then adds one to account for the log_entry block itself.
	*/   
	tmp = (((bfs->dsb.block_size-8) / sizeof(block_run))+1);

	if ((tmp * bfs->dsb.block_size) > MAX_LOG_SIZE)
		bfs->blocks_per_le = MAX_LOG_SIZE / bfs->dsb.block_size;
	else
		bfs->blocks_per_le = tmp;

#if 0
	if (bfs->blocks_per_le > 1)
		bfs->blocks_per_le /= 2;
#endif
}


int
create_log(bfs_info *bfs)
{
	int num_log_blocks = 512;
	
	bfs->log_sem = create_sem(1, "log");
	if (bfs->log_sem < 0)
		return ENOMEM;

	/* XXXdbg -- umm, can we do better on the sizing of the log? */
	if (bfs->dsb.num_blocks >= 4096)
		num_log_blocks = 2048;

	if (pre_allocate_block_run(bfs,0,num_log_blocks,&bfs->dsb.log_blocks) != 0) {
		delete_sem(bfs->log_sem);
		bfs->log_sem = -1;
		return ENOSPC;
	}


	bfs->dsb.log_start = bfs->cur_log_end = bfs->dsb.log_end = 0;
	calc_blocks_per_le(bfs);

	bfs->log_block_nums = malloc(bfs->blocks_per_le * sizeof(dr9_off_t));
	if (bfs->log_block_nums == NULL) {
		delete_sem(bfs->log_sem);
		bfs->log_sem = -1;
		return ENOMEM;
	}

	return 0;
}

void log_flusher(void *arg, int phase);


int
replay_log(bfs_info *bfs)
{
	int        i, j, bsize, count;
	void      *block;
	dr9_off_t      log_base, num_log_blocks, lstart, lend, bnum, dest_bnum;
	log_entry *lent;

	lstart = bfs->dsb.log_start;
	lend   = bfs->dsb.log_end;
	if (lstart == lend) {
		return 0;
	}

	bsize          = bfs->dsb.block_size;
	log_base       = block_run_to_bnum(bfs->dsb.log_blocks);
	num_log_blocks = bfs->dsb.log_blocks.len;

	printf("File system not shutdown cleanly!\n");

	if (lstart < lend)
		count = lend - lstart;
	else {
		count = (num_log_blocks - lstart) + lend;
	}

	/*
	   first go through and validate the log.  the validation is
	   pretty weak but we don't have a lot to go on since this is
	   only a block log and doesn't know anything about the
	   transactions it has logged.
	*/   
	printf("Validating log: blocks %Ld - %Ld\n", lstart, lend);
	while (count > 0) {
		lent = (log_entry *)get_block(bfs->fd, log_base + lstart, bsize);
		if (lent == NULL) {
			printf("error reading log header block %Ld\n", lstart);
			goto cleanup;
		}

		if (lent->max_entries != BLOCKS_PER_LOG_ENTRY - 1 ||
			lent->num_entries >= BLOCKS_PER_LOG_ENTRY - 1 ||
			lent->num_entries < 0) {

			printf("weird looking log entry @ bnum %Ld (max 0x%x num 0x%x)\n",
				   lstart, lent->max_entries, lent->num_entries);
			release_block(bfs->fd, log_base + lstart);
			goto cleanup;
		}
		
		for(i=0; i < lent->num_entries; i++) {
			bnum = (lstart + i + 1) % num_log_blocks;

			dest_bnum = block_run_to_bnum(lent->blocks[i]);
			if (dest_bnum <= 0 || dest_bnum >= bfs->dsb.num_blocks) {
				printf("log sez to replay to block %Ld but max is %Ld\n",
					   dest_bnum, bfs->dsb.num_blocks);
				release_block(bfs->fd, log_base + lstart);
				goto cleanup;
			}
		}

		release_block(bfs->fd, log_base + lstart);

		lstart  = (lstart + i + 1) % num_log_blocks;
		count  -= (i + 1);
	}

	/* reset these guys because the validation loop messed with them */
	lstart = bfs->dsb.log_start;
	lend   = bfs->dsb.log_end;

	if (lstart < lend)
		count = lend - lstart;
	else {
		count = (num_log_blocks - lstart) + lend;
	}

	printf("Playing back log: blocks %Ld - %Ld\n", lstart, lend);

	while (count > 0) {
		lent = (log_entry *)get_block(bfs->fd, log_base + lstart, bsize);
		if (lent == NULL) {
			printf("error reading log header block %Ld\n", lstart);
			return EBADF;
		}

		if (lent->max_entries != BLOCKS_PER_LOG_ENTRY - 1 ||
			lent->num_entries >= BLOCKS_PER_LOG_ENTRY - 1 ||
			lent->num_entries < 0) {

			printf("weird looking log entry @ bnum %Ld (max 0x%x num 0x%x)\n",
				   lstart, lent->max_entries, lent->num_entries);
			printf("aborting log play back!\n");
			release_block(bfs->fd, log_base + lstart);
			break;
		}
			
		for(i=0; i < lent->num_entries; i++) {
			bnum = (lstart + i + 1) % num_log_blocks;

			block = get_block(bfs->fd, log_base + bnum, bsize);
			if (block == NULL) {
				printf("bad log block num %Ld\n", log_base + bnum);
				count = 0;           /* force outer loop to quit too */
				break;
			}

			dest_bnum = block_run_to_bnum(lent->blocks[i]);

{
	bfs_inode *bi;

	bi = (bfs_inode *)block;
	if (bi->magic1 == INODE_MAGIC1) {
		if (bi->flags & INODE_VIRGIN) {
			printf("@@@@@@@@@ a virgin inode in the log??? (dest bnum %Ld)\n",
				   dest_bnum);
			bi->flags &= ~INODE_VIRGIN;
		}
	}
}

	/* printf("log block %4Ld (%4Ld) rewriting %3d:%5d:%3d (bnum 0x%.5Lx)\n",
				   bnum, log_base + bnum,
				   lent->blocks[i].allocation_group,
				   lent->blocks[i].start,
				   lent->blocks[i].len,
				   dest_bnum); */
				   
			if (write_phys_blocks(bfs->fd, dest_bnum, block, 1, bsize) != 0) {
				printf("error re-playing log block (i = %d, dest %Ld)\n", i,
					   dest_bnum);
				count = 0;           /* force outer loop to quit too */
				break;
			}

			release_block(bfs->fd, log_base + bnum);
		}

		release_block(bfs->fd, log_base + lstart);
		lstart  = (lstart + i + 1) % num_log_blocks;
		count  -= (i + 1);
/*		printf("-----------------------------------------------------\n"); */
	}

	if (lstart != lend) {
		printf("******* hmmm, that's odd, lstart 0x%Lx and lend 0x%Lx\n",
			   lstart, lend);
		return EINVAL;
	}

	
	bfs->dsb.log_start = bfs->dsb.log_end;
	bfs->cur_log_end   = bfs->dsb.log_end;
	write_super_block(bfs);
	printf("Done replaying log!\n");

	return 0;

 cleanup:
	printf("Log playback aborted.  Some transactions/data may be lost.\n");

	bfs->dsb.log_start = bfs->dsb.log_end;
	bfs->cur_log_end   = bfs->dsb.log_end;

	/*
	   XXXdbg returning success here is dangerous. for now we'll accept
	   the danger though it might not be wise in the long run.
	*/
	return 0;

}



int
init_log(bfs_info *bfs)
{
	int       ret = 0;
	dr9_off_t log_base, num_log_blocks, lstart, lend, bnum, dest_bnum;

	bfs->log_sem = create_sem(1, "log");
	if (bfs->log_sem < 0)
		return ENOMEM;

	calc_blocks_per_le(bfs);

	if (bfs->flags & FS_READ_ONLY)
		return 0;

	printf("Max log buffer == %dk\n",
		   (bfs->blocks_per_le * bfs->dsb.block_size) / 1024);

	bfs->log_block_nums = malloc(bfs->blocks_per_le * sizeof(dr9_off_t));
	if (bfs->log_block_nums == NULL) {
		delete_sem(bfs->log_sem);
		bfs->log_sem = -1;
		return ENOMEM;
	}

#ifndef USER
	if ((bfs->flags & FS_READ_ONLY) == 0) /* only do this for RW filesystems */
		register_kernel_daemon(log_flusher, bfs, 11);
#endif

	if (bfs->dsb.log_start != bfs->dsb.log_end)
		ret = replay_log(bfs);

	if (ret == 0)
		bfs->cur_log_end = bfs->dsb.log_end;

	return ret;
}


void
sync_log(bfs_info *bfs)
{
	force_log_flush(bfs);          /* it grabs the necessary semaphores */
	flush_device(bfs->fd, 0);
}



/*
   this can only be called if we are *not* in the middle of a transaction
   but we must hold the log semaphore!
*/   
void
force_log_flush(bfs_info *bfs)
{
	acquire_sem(bfs->log_sem);

	if (bfs->active_lh)
		bfs_die("*** error! force_log_flush called during a transaction\n");
	else if (bfs->cur_lh) {   /* need to force it to flush */
		bfs->active_lh = 1;
		bfs->log_owner = getpid();
		bfs->cur_lh->flags |= LH_REALLY_FLUSH;
		end_transaction(bfs, bfs->cur_lh);
	} else {
		release_sem(bfs->log_sem);
	}
}



void
dump_completed_log_entries(bfs_info *bfs)
{
	log_list *ll;

	for(ll=bfs->completed_log_entries; ll; ll=ll->next) {
		printf("ll @ 0x%lx : start %6Ld end %6Ld\n", (ulong)ll,
			   ll->start, ll->end);
	}
}



void
shutdown_log(bfs_info *bfs)
{
	if (bfs->log_sem > 0) {
#ifndef USER
		if ((bfs->flags & FS_READ_ONLY) == 0)
			unregister_kernel_daemon(log_flusher, bfs);
#endif
		force_cache_flush(bfs->fd, 1);
		force_log_flush(bfs);
		
		if (bfs->completed_log_entries) {
			printf("there should not still be an completed log entries\n");
			dump_completed_log_entries(bfs);
			bfs_die("I'm hosed\n");
		}

		if (bfs->cur_lh) {
			entry_list *el, *elnext;
			for(el=bfs->cur_lh->el; el; el=elnext) {
				if (el->lent)
					free(el->lent);
				elnext = el->next;
				free(el);
			}

			free(bfs->cur_lh);
		}

		if (bfs->log_block_nums)
			free(bfs->log_block_nums);
		bfs->log_block_nums = NULL;

		delete_sem(bfs->log_sem);
	}
	bfs->log_sem = -1;
}



#define MIN_FREE_LOG_BLOCKS  128  /* max # of blocks a transaction can touch */

void
check_free_log_space(bfs_info *bfs)
{
    int   counter = 0;
	dr9_off_t free_blocks, num_log_blocks;

	num_log_blocks = bfs->dsb.log_blocks.len;

	while (1) {
		counter++;

		/* compute how many free blocks are in the log */
		if (bfs->dsb.log_end >= bfs->dsb.log_start &&
			bfs->cur_log_end >= bfs->dsb.log_end) {
			free_blocks = (num_log_blocks - bfs->cur_log_end) + bfs->dsb.log_start;
		} else {
			free_blocks = bfs->dsb.log_start - bfs->cur_log_end;
		}

	
		if (free_blocks >= MIN_FREE_LOG_BLOCKS) {  /* then it's ok to go on */
			break;
		}


		if (counter > 16384) {     /* XXXdbg is this a big enough number? */
			printf("completed log entries:\n");
			dump_completed_log_entries(bfs);
			bfs_die("*** log just isn't flushing (counter %d)!\n", counter);
		}


		/*
		   first we try and flush out dirty log blocks to hopefully complete
		   any outstanding transactions (so log space is free'd up).
		*/
		force_cache_flush(bfs->fd, 1);
		
		/*
		   if we've been through this a few times, it's possible that
		   there is a buffered transaction that we're sitting on so
		   we release the log sem and try to flush that out.
		*/   
		if ((counter % 8) == 0) {
			if ((counter % 512) == 0)
               printf("log is taking a loooonng time to flush! (counter %d)\n",
					  counter);

			release_sem(bfs->log_sem);

			force_log_flush(bfs);

			acquire_sem(bfs->log_sem);
		}
	}
}



log_handle *
start_transaction(bfs_info *bfs)
{
	log_entry  *lent;
	entry_list *el;
	log_handle *lh;

	if (bfs->flags & FS_READ_ONLY) {
		printf("start transaction called on a read-only device! (bfs 0x%x)\n",
			   bfs);
		return NULL;
	}


	if (getpid() == bfs->log_owner && bfs->active_lh) {
		atomic_add((long *)&bfs->active_lh, 1);
		return bfs->cur_lh;
	}

    /* printf("start transaction\n"); */
	acquire_sem(bfs->log_sem);

	check_free_log_space(bfs);      /* make sure there is enough room */


	bfs->last_log  = system_time();
	bfs->log_owner = getpid();

	if (bfs->cur_lh) {      /* just keep appending to the same log entry! */
		if (bfs->active_lh) {
			bfs_die("start_transaction called with an active transaction!\n");
			return NULL;
		}

		bfs->active_lh = 1;
		return bfs->cur_lh; 
	}

	lent = (log_entry *)malloc(BLOCKS_PER_LOG_ENTRY * bfs->dsb.block_size);
	if (lent == NULL) {
		goto bad;
	}

	el = (entry_list *)malloc(sizeof(entry_list));
	if (el == NULL) {
		free(lent);
		goto bad;
	}
	

	lh = (log_handle *)malloc(sizeof(log_handle));
	if (lh == NULL) {
		free(lent);
		free(el);
		goto bad;
	}
		
	lent->num_entries = 0;
	lent->max_entries = BLOCKS_PER_LOG_ENTRY - 1;
		   
	el->num_blocks         = 0;
	el->lent               = lent;
	el->next               = NULL;
	
	lh->el                 = el;
	lh->flags              = 0;
	lh->num_blocks         = 1;    /* have to account for the log hdr block */
	lh->num_el             = 1;
	lh->num_blocks_flushed = 0;
	lh->bfs                = bfs;
	lh->start = lh->end    = -1;

	
	bfs->active_lh = 1;
	return lh;


bad:
	bfs->log_owner = -1;
	release_sem(bfs->log_sem);
	return NULL;
}



ssize_t
log_write_blocks(bfs_info *bfs, log_handle *lh, dr9_off_t bnum, const void *data,
				 int nblocks, int need_update)
{
	int         i, ret = 0;
	char       *ptr;
	dr9_off_t       log_bnum = -1;
	block_run   br;
	entry_list *el, *prev_el = NULL;

/* printf("lwb: %d:%d (lh 0x%x)\n", bnum, nblocks, lh); */

	if (bfs->flags & FS_READ_ONLY) {
		printf("log write blocks called on a read-only device! (bfs 0x%x)\n",
			   bfs);
		return 0;
	}

	if (bfs->active_lh == 0 || lh == NULL)
		bfs_die("log_write_blocks called but active = %d and lh = 0x%lx\n",
				bfs->active_lh, (ulong)lh);

	if (nblocks <= 0)
		bfs_die("log_write_blocks called with nblocks %d\n", nblocks);

	bfs->last_log = system_time();

	for(el=lh->el; el; el=el->next) {
		prev_el = el;
		ptr = (char *)el->lent + bfs->dsb.block_size;

		for(i=0; i < el->lent->num_entries; i++) {
			log_bnum = block_run_to_bnum(el->lent->blocks[i]);
			if (bnum >= log_bnum && bnum < log_bnum+el->lent->blocks[i].len)
				break;

			ptr += el->lent->blocks[i].len * bfs->dsb.block_size;
		}

		if (i < el->lent->num_entries)
			break;
	}

	if (el && i < el->lent->num_entries) {/* then bnum is in the log already */

		/* sanity check to avoid overlapping log writes */
		if (bnum != log_bnum || nblocks != el->lent->blocks[i].len) {
			bfs_die("overlapping log writes! bnum %Ld:%d w/%Ld:%d\n",
					bnum, nblocks, log_bnum, el->lent->blocks[i].len);
		}

		ret = cached_write(bfs->fd, bnum, data, nblocks, bfs->dsb.block_size);
		
		if (ret != 0) {
			printf("cached write failed (ret %d)\n", ret);
			return 0;
		}
	} else {
		/* then if we can fit this write in this entry, put it in there */
		if (prev_el->num_blocks + nblocks < BLOCKS_PER_LOG_ENTRY - 1){
			el = prev_el;
			i  = el->lent->num_entries;
		}

		/* if el is still null here then we have to allocate an entry_list */
		if (el == NULL) {
			el = (entry_list *)malloc(sizeof(entry_list));
			if (el == NULL) {
				printf("can't allocate a new entry list!\n");
				return 0;
			}
		
			el->lent = (log_entry *)malloc(BLOCKS_PER_LOG_ENTRY * bfs->dsb.block_size);
			if (el->lent == NULL) {
				free(el);
				printf("can't allocate a new log_entry!\n");
				return 0;
			}

			el->num_blocks        = 0;
			el->next              = NULL;
			el->lent->num_entries = 0;
			el->lent->max_entries = BLOCKS_PER_LOG_ENTRY - 1;

			ptr = (char *)el->lent + bfs->dsb.block_size;
			i   = 0;

			lh->num_el++;
			lh->num_blocks++;   /* have to account for the entry_list block */

			if (prev_el->next != NULL) {
				bfs_die("fucked up elist structure (next == 0x%lx)\n",
						(ulong)prev_el->next);
			}

			prev_el->next = el;
		}

		/* printf("writing bnum %d:%d\n", bnum, nblocks); */
		ret = cached_write_locked(bfs->fd, bnum, data, nblocks,
								  bfs->dsb.block_size);
			
		if (ret != 0) {
			if (el != prev_el) {   /* then we allocated things */
				free(el->lent);
				free(el);
			}

			printf("cached_write_locked failed! (ret == %d)\n", ret);
			return 0;
		}
			

		bnum_to_block_run(br, bnum);
		br.len = nblocks;

		el->lent->blocks[el->lent->num_entries++]  = br;
		el->num_blocks += nblocks;
		
		lh->num_blocks += nblocks;
	}


	if (el) {
		/* this copies it into the proper place in the log */
		/* printf("copying bnum %3d:%d to 0x%x in log\n", bnum,nblocks,ptr); */
		memcpy(ptr, data, nblocks * bfs->dsb.block_size);
	} else {
		bfs_die("el == NULL and it should not be!\n");
		return 0;
	}

	return nblocks;
}




/*
   this updates log_start (if needed) for the transaction that
   occured between the blocks of start - end.
*/   
static void
update_log_start(bfs_info *bfs, dr9_off_t start, dr9_off_t end)
{
	int       update_sb = 0;
	log_list *ll, *prev=NULL, *next, *tmp;

	if (start == bfs->dsb.log_start) {
		bfs->dsb.log_start = end;
		start = end = -1;
		update_sb = 1;
	}
	
	for(ll=bfs->completed_log_entries; ll; prev=ll, ll=next) {
		if (ll->start == bfs->dsb.log_start) {
			bfs->dsb.log_start = ll->end;
			update_sb = 1;
			if (prev)
				prev->next = ll->next;
			if (ll == bfs->completed_log_entries)
				bfs->completed_log_entries = ll->next;

			next = bfs->completed_log_entries;  /* start scanning again */
			tmp  = ll;
			ll   = NULL;
			free(tmp);
		} else if (end == ll->start) {
			ll->start = start;
			next = bfs->completed_log_entries;  /* start scanning again */
			ll   = NULL;
			start = end = -1;
		} else if (start == ll->end) {
			ll->end = end;
			next = ll->next;
			start = end = -1;
		} else {
			next = ll->next;
		}
	}

	if (update_sb)
		write_super_block(bfs);
	
	if (start == -1 && end == -1)
		return;


	{
		int printed = 0;

		/* if we get here it's not contiguous with anyone else */
		while ((ll = (log_list *)malloc(sizeof(log_list))) == NULL) {
			if (printed++ == 0)
				printf("*** can't allocate space for log list entry!\n");
		}
			
		ll->start = start;
		ll->end   = end;
		ll->next  = bfs->completed_log_entries;
		bfs->completed_log_entries = ll;
	}
}


static void
update_log_end(bfs_info *bfs, dr9_off_t new_log_end)
{
	bfs->dsb.log_end = new_log_end;
	bfs->dsb.flags   = BFS_DIRTY;
}


static void
update_transaction(dr9_off_t bnum, size_t nblocks, void *arg)
{
	int         val;
	log_handle *lh = (log_handle *)arg;

/* printf("update: %3d : %3d  (0x%x)\n", bnum, nblocks, arg); */
	if (bnum < 0 || nblocks > 65535) {
		bfs_die("*** bad bnum or block (%Ld %d)\n", bnum, nblocks);
	}

	if (lh == NULL) {
		bfs_die("*** bad lh @ 0x%lx\n", (ulong)lh);
	}

	if (lh->num_el < 0 || lh->num_el > 2048) {
		bfs_die("*** bogus num_el (%d) in log_handle\n", lh->num_el);
	}


	val = atomic_add((long *)&lh->num_blocks_flushed, nblocks);
	if (val < lh->num_blocks - nblocks)  /* then the transaction isn't done */
		return;


	/* else, we're the last block flushed to update the log start ptr */
	update_log_start(lh->bfs, lh->start, lh->end);

	lh->end = -1;   /* just to be paranoid... */

	free(lh);
}


int
end_transaction(bfs_info *bfs, log_handle *lh)
{
	int         i, sum, total_blocks = 0;
	dr9_off_t       bnum, log_bnum, log_base, num_log_blocks, start, curr;
	log_entry  *lent;
	entry_list *el, *next_el;

/* printf("end transaction\n"); */
	if (bfs->flags & FS_READ_ONLY) {
		printf("end transaction called on a read-only device! (bfs 0x%x)\n",
			   bfs);
		return 0;
	}


	if (lh == NULL)
		bfs_die("null log handle passed to end_transaction?\n");
	
	if (bfs->active_lh == 0 && (lh->flags & LH_REALLY_FLUSH) == 0) {
		bfs_die("end transaction called but active_lh == 0 (cur_lh 0x%lx)\n",
				(ulong)bfs->cur_lh);
	}

	/* if we've recursively started a transaction, don't end it now */
	if (atomic_add(&bfs->active_lh, -1) > 1) {
		return 0;
	}

	if (bfs->active_lh < 0)
		bfs_die("log-scroggage: active %d bfs 0x%x cur_lh 0x%x owner %d\n",
				bfs->active_lh, bfs, bfs->cur_lh, bfs->log_owner);

	bfs->last_log = system_time();

	if ((lh->flags & LH_REALLY_FLUSH) == 0) {
		/* XXXdbg -- need better logic for when to flush the transaction */
		lent = lh->el->lent;

		if (lent->num_entries < lent->max_entries-8 && lh->num_el < 2) {
			bfs->log_owner = -1;
			release_sem(bfs->log_sem);
			return 0;
		}
	}

	if (lh->num_blocks_flushed != 0) {
		bfs_die("*** danger! # blocks flushed == %d, not 0\n",
				lh->num_blocks_flushed);
	}

	lh->num_blocks_flushed = lh->num_el;  /* we'll write them down below */

	total_blocks = 0;
	for(el=lh->el; el; el=el->next) {
		for(i=0; i < el->lent->num_entries; i++) {
			total_blocks += el->lent->blocks[i].len;
		}
	}

	if (total_blocks == 0) {  /* then there's nothing really to flush! */
		lh->num_blocks_flushed = 0;  /* since we didn't really write any */
		bfs->log_owner = -1;
		release_sem(bfs->log_sem);

		return 0;
	}

	total_blocks += lh->num_el;


	log_base       = block_run_to_bnum(bfs->dsb.log_blocks);
	num_log_blocks = bfs->dsb.log_blocks.len;

	lh->start = log_bnum = bfs->cur_log_end;
	
	/* figure out where the new log end is going to be */
	bfs->cur_log_end = (bfs->cur_log_end + total_blocks) % num_log_blocks;

	/* write the log entry header (lh->lent) first! */
	start = lh->start;

	/*
	   here we write the blocks of the transaction into the log area
	   of the disk.  we try to write as contiguously as possible taking 
	   into account any wrap-around that may happen if we're at the end
	   of the log.
	*/   
	sum  = 0;
	curr = start;

#if 0 /* XXXdbg -- turn this on in the exp build */
{
	int i;
	entry_list *myel;
	bfs_inode *tmpbi;
	
	for(myel=lh->el; myel; myel=myel->next) {
		for(i=0; i < myel->num_blocks+1; i++) {
			tmpbi = (bfs_inode *)&(((char *)myel->lent)[i*bfs->dsb.block_size]);
			if (tmpbi->magic1 == INODE_MAGIC1) {
				if (tmpbi->flags & INODE_VIRGIN)
					bfs_die("virgin i-node in da log? (tmpbi 0x%x, lh 0x%x)\n",
							tmpbi, lh);
			}
		}
	}
}
#endif

	for(el=lh->el; el; curr += el->lent->num_entries + 1, el=el->next) {
		/*
		   the +1's in the block length calculations account for the
		   log_entry block that comes before every group of data blocks.
		*/   
		if ((curr + el->num_blocks + 1) < num_log_blocks) {
			if (write_phys_blocks(bfs->fd,
								  log_base + curr,
								  el->lent,
								  el->num_blocks + 1,
								  bfs->dsb.block_size) != 0)
				break;
		} else {        /* we have to wrap around and split the write in two */
			char  *ptr;
			dr9_off_t  amt;

			amt = num_log_blocks - curr;
			if (amt && write_phys_blocks(bfs->fd,
										 log_base + curr,
										 el->lent,
										 amt,
										 bfs->dsb.block_size) != 0)
				break;

			ptr = (char *)((int)amt * bfs->dsb.block_size);
			
			/* and here we wrap around to the beginning of the log */
			amt = (el->num_blocks + 1) - (num_log_blocks - curr);
			if (write_phys_blocks(bfs->fd,
								  log_base,
								  (char *)((ulong)el->lent + ptr),
								  amt,
								  bfs->dsb.block_size) != 0)
				break;

			/* have to do this because the loop increments it and we wrapped */
			curr = -(el->num_blocks + 1) + amt; 
		}

		sum += el->num_blocks + 1;
	}

	if (el) {
		bfs_die("Error writing log entry!\n");
	}

	if (sum != total_blocks) {
		bfs_die("bummer.  wrote to log and sum %d != total_blocks %d\n",
				sum, total_blocks);
	}


	lh->end = ((lh->start + total_blocks) % num_log_blocks);


	/* now update the log end pointer */
	update_log_end(bfs, lh->end);

	write_super_block(bfs);   /* XXXdbg -- this may not always be necessary */

	/* go through and set the block info and free up entry_list structures */
	for(el=lh->el; el; el=next_el) {
		int lbn = 0, j;
		
		for(i=0; i < el->lent->num_entries; i++) {
			bnum = block_run_to_bnum(el->lent->blocks[i]);
			for(j=0; j < el->lent->blocks[i].len; j++) {
				bfs->log_block_nums[lbn++] = bnum + j;

				if (lbn >= bfs->blocks_per_le) {
					set_blocks_info(bfs->fd, bfs->log_block_nums,
									lbn, update_transaction, lh);
					lbn = 0;
				}
			}
		}

		if (lbn > 0) {
			set_blocks_info(bfs->fd, bfs->log_block_nums,
							lbn, update_transaction, lh);
			lbn = 0;
		}

			/* printf("log: %d:%d\n", block_run_to_bnum(el->lent->blocks[i]),
			          el->lent->blocks[i].len); */

		next_el = el->next;

		free(el->lent);
		free(el);
	}


	if (bfs->active_lh != 0) {
		bfs_die("log is fucked up.  bfs 0x%x cur_lh 0x%x active %d thid %d\n",
				bfs, bfs->cur_lh, bfs->cur_lh, bfs->active_lh, bfs->log_owner);
	}

	bfs->log_owner = -1;
	bfs->cur_lh    = NULL;  /* don't want to access it through here anymore */
	release_sem(bfs->log_sem);

	return 0;
}


void
abort_transaction(bfs_info *bfs, log_handle *lh)
{
	end_transaction(bfs, lh);
}



void
log_flusher(void *arg, int phase)
{
	bfs_info *bfs = (bfs_info *)arg;

	/* only do this if the log has been idle for at least one second */
	if ((system_time() - bfs->last_log) < 1000000)
		return;

	if (bfs->cur_lh == NULL)
		return;

	/*
	   note: we have to grab this here in case we need to call
	   end_transaction because it expects that the semaphore is held.
	*/   
	acquire_sem(bfs->log_sem);

	if (bfs->active_lh == 0 && bfs->cur_lh) {
		bfs->active_lh = 1;
		bfs->log_owner = getpid();
		bfs->cur_lh->flags |= LH_REALLY_FLUSH;
		end_transaction(bfs, bfs->cur_lh);
	} else {
		release_sem(bfs->log_sem);
	}
	
}
