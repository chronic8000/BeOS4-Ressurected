#include "cfs.h"
#include "cfs_vnode.h"
#include "cfs_debug.h"
#include <Debug.h>
#include <lock.h>
#include <cache.h>
#include <KernelExport.h>
#include <iovec.h>
#include <errno.h>
#include <string.h>

typedef struct log_entry {
	struct log_entry  *next;
	off_t              bnum;
	uint8             *block_data;
	bool               written;
	int                last_sub_transaction;
} log_entry;

struct cfs_transaction {
	log_entry  *entries;
	int         num_entries;
	int         flushed_entries;
	int         sub_transaction_index;
	int         sub_transaction_limit;
	int         sub_transaction_entries;
	bool        active;
	bigtime_t   start_time;
	bigtime_t   mod_time;
};

struct cfs_log_transaction {
	struct cfs_log_transaction  *prev;
	log_entry                   *entries;
	int                          remaining_entry_count;
	sem_id                       complete_sem;
	off_t                        log_tail;
};

//typedef struct cfs_transaction {
//	struct cfs_transaction  *prev;
//	log_entry               *entries;
//	int                      num_entries;
//	int                      num_written;
//	sem_id                   complete_sem;
//} cfs_transaction;

static void block_written(off_t bnum, size_t blockcount, void *cookie);
static status_t write_log(cfs_info *fsinfo, cfs_log_transaction *transaction);
static status_t cfs_complete_transaction(cfs_info *fsinfo,
                                         cfs_transaction *transaction);
static void cfs_log_flusher(void *fsinfo, int phase);

int
cfs_max_log_blocks(cfs_info *fsinfo)
{
	/*
	n * bs + hs * ceil(n / bph) <= ls
	
	nh = ceil(ls / (hs + bph * bs))
	n = floor((ls - nh * hs) / bs)
	*/
	
	int header_size = max(sizeof(cfs_log_entry_header), fsinfo->_dev_block_size);
	int max_entry_size = header_size +
	                     max_blocks_per_log_entry(fsinfo->_dev_block_size) *
	                     fsinfo->cache_block_size;
	int header_blocks = (fsinfo->log_size - fsinfo->cache_block_size +
	                     max_entry_size - 1) / max_entry_size;
	return (fsinfo->log_size - fsinfo->cache_block_size - header_blocks *
	        header_size) / fsinfo->cache_block_size;
}

static bool
log_info_valid(cfs_log_info *li, int index, int index_count)
{
	if(li->log_id != CFS_LOG_ID)
		return false;
	if(li->version != li->last_version + 1)
		return false;
	if(li->version % index_count != index)
		return false;
	if(li->log_id + li->version + (uint32)li->head + (uint32)li->tail +
	   li->last_version != li->checksum)
		return false;
	return true;
}

static status_t
read_log_info(cfs_info *fsinfo)
{
	status_t err;
	off_t log_head = 0;
	off_t log_tail = 0;
	uint32 log_version = 0;
	int i;
	int info_count = fsinfo->log_info_count;

	cfs_log_info log_info;

	PRINT_FLOW(("cfs: read_log_info\n"));
	
	for(i = 0; i < info_count; i++) {
		err = cfs_read_disk_uncached(fsinfo, fsinfo->log_info_pos[i], &log_info,
		                             sizeof(log_info));
		if(err == B_NO_ERROR && log_info_valid(&log_info, i, info_count)) {
			if(log_head < fsinfo->log_base ||
			   log_version / info_count * info_count + i == log_info.version) {
				log_head = log_info.head;
				log_tail = log_info.tail;
				log_version = log_info.version;
			}
		}
		else {
			PRINT_WARNING(("cfs_read_log_info: log info %d is bad\n", i));
		}
	}
	if(log_head < fsinfo->log_base) {
		PRINT_ERROR(("cfs_read_log_info: "
		             "could not find valid log info block\n"));
		return B_ERROR;
	}
	
	fsinfo->log_version = log_version;
	fsinfo->log_head = log_head;
	fsinfo->log_tail = log_tail;

	return B_NO_ERROR;
}
	
static status_t
write_log_info(cfs_info *fsinfo, off_t log_head, off_t log_tail)
{
	status_t err;
	cfs_log_info *log_info;
	int index = (fsinfo->log_version + 1) % fsinfo->log_info_count;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;
	
	if(fsinfo->_dev_block_size < sizeof(*log_info)) {
		PRINT_INTERNAL_ERROR(("cfs: write_log_info dev block size (%ld) "
		                      "is too small\n", fsinfo->_dev_block_size));
		return B_ERROR;
	}
	
	log_info = calloc(fsinfo->_dev_block_size, 1);
	
	log_info->log_id = CFS_LOG_ID;
	log_info->version = fsinfo->log_version + 1;
	log_info->head = log_head;
	log_info->tail = log_tail;
	log_info->last_version = fsinfo->log_version;
	log_info->checksum = log_info->log_id + log_info->version + log_info->head +
	                     log_info->tail + log_info->last_version;

	PRINT_FLOW(("cfs: write_log_info %d, version %ld at %Ld\n",
	            index, log_info->version, fsinfo->log_info_pos[index]));
	
	err = cfs_write_disk_uncached(fsinfo, fsinfo->log_info_pos[index],
	                              log_info, fsinfo->_dev_block_size);
	free(log_info);
	if(err != B_NO_ERROR)
		return err;
	
	ioctl(fsinfo->dev_fd, B_FLUSH_DRIVE_CACHE);

	fsinfo->log_version++;
	fsinfo->log_head = log_head;
	fsinfo->log_tail = log_tail;
	
	return B_NO_ERROR;
}

static void
cfs_free_log_transaction(cfs_log_transaction *log_transaction)
{
	while(log_transaction->entries != 0) {
		log_entry *entry = log_transaction->entries;
		if(!entry->written) {
			PRINT_ERROR(("cfs_free_log_transaction: entry #%Ld was not "
			             "written\n", entry->bnum));
		}
		log_transaction->entries = entry->next;
		free(entry);
	}
	if(log_transaction->complete_sem != -1)
		delete_sem(log_transaction->complete_sem);
	free(log_transaction);
}

static status_t
playback_block(cfs_info *fsinfo, off_t log_pos, off_t block_pos)
{
	status_t err;
	log_entry *e;
	off_t bnum = block_pos / fsinfo->cache_block_size;

	if(block_pos < fsinfo->super_block_2_pos ||
	   block_pos + fsinfo->cache_block_size > fsinfo->size) {
		PRINT_ERROR(("cfs_playback_block: position %Ld outsize fs bounds\n",
		             block_pos));
		return B_IO_ERROR;
	}

	e = malloc(sizeof(*e));
	if(e == NULL)
		return B_NO_MEMORY;
	e->next = NULL;
	e->bnum = bnum;
	e->written = false;
	e->last_sub_transaction = 0;
	e->block_data = get_empty_block(fsinfo->dev_fd, bnum,
	                                fsinfo->cache_block_size);
	if(e->block_data == NULL) {
		err = B_ERROR;
		goto err1;
	}
	mark_blocks_dirty(fsinfo->dev_fd, bnum, 1);
	err = cfs_read_disk_uncached(fsinfo, log_pos, e->block_data,
	                             fsinfo->cache_block_size);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_playback_block: could not read log block at %Ld, "
		             "%s\n", log_pos, strerror(err)));
		goto err2;
	}
	if(fsinfo->logged_transactions == NULL) {
		cfs_log_transaction *log_transaction = malloc(sizeof(*log_transaction));
		if(log_transaction == NULL) {
			err = B_NO_MEMORY;
			goto err2;
		}
		log_transaction->prev = NULL;
		log_transaction->entries = NULL;
		log_transaction->remaining_entry_count = 0;
		log_transaction->complete_sem =
			create_sem(0, "cfs_playback_transaction_complete");
		if(log_transaction->complete_sem < 0) {
			err = log_transaction->complete_sem;
			PRINT_ERROR(("cfs_playback_block: create_sem failed %s\n",
			            strerror(err)));
			free(log_transaction);
			goto err2;
		}
		log_transaction->log_tail = fsinfo->log_tail;
		fsinfo->logged_transactions = log_transaction;
	}
	err = set_blocks_info(fsinfo->dev_fd, &bnum, 1, block_written,
	                      fsinfo->logged_transactions);
	if(err != B_NO_ERROR)
		goto err2;
	e->next = fsinfo->logged_transactions->entries;
	fsinfo->logged_transactions->entries = e;
	fsinfo->logged_transactions->remaining_entry_count++;
	return B_NO_ERROR;
err2:
	release_block(fsinfo->dev_fd, bnum);
err1:
	free(e);
	return err;
}

static status_t
playback_log(cfs_info *fsinfo)
{
	status_t err;
	off_t log_head;
	int i;
	bool wrapped = false;
	cfs_log_entry_header *log_header;
	
	if(fsinfo->_dev_block_size <= sizeof(cfs_log_entry_header) + 8) {
		PRINT_INTERNAL_ERROR(("playback_log: dev block size %ld is too small\n",
		                      fsinfo->_dev_block_size));
		return B_ERROR;
	}

	log_header = malloc(fsinfo->_dev_block_size);
	if(log_header == NULL)
		return B_NO_MEMORY;

	log_head = fsinfo->log_head;
	while(log_head != fsinfo->log_tail) {
		err = cfs_read_disk_uncached(fsinfo, log_head, log_header,
		                             fsinfo->_dev_block_size);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_playback_log: could not read log entry header at "
			             "%Ld, %s\n", log_head, strerror(err)));
			goto err;
		}
		if(log_header->log_entry_id != CFS_LOG_ENTRY_ID) {
			PRINT_ERROR(("cfs_playback_log: bad log entry header at %Ld\n",
			             log_head));
			err = B_ERROR;
			goto err;
		}
		if(log_header->num_blocks < 1 ||
		   log_header->num_blocks >
		   max_blocks_per_log_entry(fsinfo->_dev_block_size)) {
			PRINT_ERROR(("cfs_playback_log: bad log entry header at %Ld, "
			             "num blocks %ld\n", log_head, log_header->num_blocks));
			err = B_ERROR;
			goto err;
		}
		log_head += fsinfo->_dev_block_size;
		for(i=0; i < log_header->num_blocks; i++) {
			if(log_head + fsinfo->cache_block_size >
			   fsinfo->log_base + fsinfo->log_size) {
				if(wrapped) {
					PRINT_ERROR(("cfs_playback_log: log does not end\n"));
					err = B_ERROR;
					goto err;
				}
				wrapped = true;
				log_head = fsinfo->log_base;
			}
			err = playback_block(fsinfo, log_head, log_header->blocks[i]);
			if(err != B_NO_ERROR)
				goto err;
			log_head += fsinfo->cache_block_size;
		}
		if(log_head >= fsinfo->log_base + fsinfo->log_size) {
			if(wrapped) {
				PRINT_ERROR(("cfs_playback_log: log does not end\n"));
				err = B_ERROR;
				goto err;
			}
			wrapped = true;
			log_head = fsinfo->log_base;
		}
	}
	err = B_NO_ERROR;
err:
	free(log_header);
	return err;
}

status_t
cfs_init_log(cfs_info *fsinfo)
{
	status_t err;

	if((fsinfo->fs_flags & B_MOUNT_READ_ONLY) == 0 && fsinfo->log_size == 0)
		return B_NO_ERROR;

	err = read_log_info(fsinfo);
	if(err)
		return err;
	
	if(fsinfo->log_head != fsinfo->log_tail) {
		if(fsinfo->fs_flags & B_MOUNT_READ_ONLY) {
			PRINT_ERROR(("cfs_init_log: cannot playback log on read-only "
			             "filesytem\n"));
			return B_NOT_ALLOWED;
		}
		err = playback_log(fsinfo);
		if(err != B_NO_ERROR)
			return err;
	}
	if((fsinfo->fs_flags & B_MOUNT_READ_ONLY) == 0) {
		err = register_kernel_daemon(cfs_log_flusher, fsinfo, 10 /* 1 sec */ );
		if(err < B_NO_ERROR)
			return err;
	}
	return B_NO_ERROR;
}

void
cfs_uninit_log(cfs_info *fsinfo)
{
	if((fsinfo->fs_flags & B_MOUNT_READ_ONLY) == 0)
		unregister_kernel_daemon(cfs_log_flusher, fsinfo);
}

status_t
cfs_start_transaction(cfs_info *fsinfo, cfs_transaction **transaction_ptr)
{
	status_t err;
	cfs_transaction *new_transaction;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(fsinfo->current_transaction != NULL) {
		*transaction_ptr = fsinfo->current_transaction;
		if(fsinfo->current_transaction->active) {
			PRINT_INTERNAL_ERROR(("cfs_start_transaction: transaction already "
			                      "active (%p)\n", fsinfo->current_transaction));
			return B_ERROR;
		}

		if(fsinfo->current_transaction->num_entries >
		   cfs_max_log_blocks(fsinfo) / 2) {
			PRINT_FLOW(("cfs_start_transaction: flushing old transaction\n"));
			err = cfs_complete_transaction(fsinfo, fsinfo->current_transaction);
			if(err != B_NO_ERROR)
				return err;
		}

		return B_NO_ERROR;
	}
	
	new_transaction = malloc(sizeof(cfs_transaction));
	if(new_transaction == NULL)
		return B_NO_MEMORY;

	new_transaction->entries = NULL;
	new_transaction->num_entries = 0;
	new_transaction->flushed_entries = 0;
	new_transaction->sub_transaction_index = 0;
	new_transaction->sub_transaction_limit = 0;
	new_transaction->sub_transaction_entries = 0;
	new_transaction->active = true;

	*transaction_ptr = new_transaction;
	fsinfo->current_transaction = new_transaction;
	return B_NO_ERROR;
}

static void
block_written(off_t bnum, size_t blockcount, void *cookie)
{
	cfs_log_transaction *log_transaction = cookie;
	log_entry *e;
	
	for(e = log_transaction->entries; e != NULL; e = e->next) {
		if(e->bnum == bnum)
			break;
	}
	if(e == NULL) {
		PRINT_INTERNAL_ERROR(("cfs_log: block_written: block %Ld not in log\n",
		                      bnum));
		return;
	}
	if(e->written) {
		PRINT_INTERNAL_ERROR(("cfs_log: block_written: block %Ld already "
		                      "written\n", bnum));
		return;
	}
	e->written = true;
	if(atomic_add(&log_transaction->remaining_entry_count, -1) == 1)
		release_sem(log_transaction->complete_sem);
}

static status_t
cfs_complete_transaction(cfs_info *fsinfo, cfs_transaction *transaction)
{
	status_t err, tmp_err;
	log_entry *e;
	cfs_log_transaction *log_transaction;
	
	if(transaction != fsinfo->current_transaction) {
		PRINT_INTERNAL_ERROR(("cfs_complete_transaction: %p, does not match "
		                      "current transaction, %p\n", transaction,
		                      fsinfo->current_transaction));
		return B_ERROR;
	}
	
	if(transaction->num_entries == 0) {
		return B_NO_ERROR;
	}
	
	log_transaction = malloc(sizeof(*log_transaction));
	if(log_transaction == NULL)
		return B_NO_MEMORY;
	
	log_transaction->prev = NULL;
	log_transaction->entries = transaction->entries;
	log_transaction->remaining_entry_count = transaction->num_entries;

	err = write_log(fsinfo, log_transaction);
	if(err != B_NO_ERROR)
		goto err1;

	log_transaction->complete_sem = create_sem(0, "cfs_transaction_complete");
	if(log_transaction->complete_sem < 0) {
		err = log_transaction->complete_sem;
		PRINT_ERROR(("cfs_complete_transaction: create_sem failed %s\n",
		            strerror(err)));
		goto err1;
	}

	for(e = transaction->entries; e != NULL; e = e->next) {
		off_t bnum = e->bnum;
		tmp_err = set_blocks_info(fsinfo->dev_fd, &bnum,
		                          1, block_written, log_transaction);
		if(err == B_NO_ERROR && tmp_err < B_NO_ERROR) {
			err = tmp_err;
			atomic_add(&log_transaction->remaining_entry_count, -1);
			/* XXX may want to abort the transaction here */
		}
	}
	if(log_transaction->remaining_entry_count == 0) {
		cfs_free_log_transaction(log_transaction);
	}
	else {
		log_transaction->prev = fsinfo->logged_transactions;
		fsinfo->logged_transactions = log_transaction;
	}
	transaction->entries = NULL;
	transaction->flushed_entries += transaction->num_entries;
	transaction->num_entries = 0;
	transaction->sub_transaction_limit = 0;
	transaction->sub_transaction_entries = 0;
	
	return err;
err1:
	free(log_transaction);
	return err;
}

status_t
cfs_sub_transaction(cfs_info *fsinfo, cfs_transaction *transaction,
                    int max_blocks)
{
	status_t err;
	int max_log_blocks = cfs_max_log_blocks(fsinfo);

	if(fsinfo->fs_flags & B_MOUNT_READ_ONLY)
		return B_READ_ONLY;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;

	if(max_blocks > max_log_blocks) {
		PRINT_INTERNAL_ERROR(("cfs_sub_transaction: subtransaction (%d)"
		                      " is too big for the log (%d)\n", max_blocks,
		                      max_log_blocks));
		return B_ERROR;
	}
	
	if(max_blocks + transaction->num_entries > max_log_blocks) {
		PRINT_FLOW(("cfs_sub_transaction: ne %d + n %d > max %d, flushing\n",
		            transaction->num_entries, max_blocks, max_log_blocks));
		err = cfs_complete_transaction(fsinfo, transaction);
		if(err != B_NO_ERROR)
			return err;
	}

	transaction->sub_transaction_index++;
	transaction->sub_transaction_limit = max_blocks;
	transaction->sub_transaction_entries = 0;
	return B_NO_ERROR;
}

status_t
cfs_end_transaction(cfs_info *fsinfo, cfs_transaction *transaction)
{
	status_t err;
	
	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;
		
	transaction->active = false;
	return B_NO_ERROR;
}

void *
get_block_logged(cfs_info *fsinfo, cfs_transaction *transaction, off_t location)
{
	status_t err;
	void *block;
	log_entry *pe = NULL;
	log_entry *e;
	off_t rblock = location / fsinfo->cache_block_size;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return NULL;

	if(rblock * fsinfo->cache_block_size != location) {
		PRINT_INTERNAL_ERROR(("get_block_logged: location not on block boundary\n"));
		return NULL;
	}
	
	for(e = transaction->entries; e != NULL; e = e->next) {
		if(e->bnum == rblock)
			goto entry_found;
		if(e->bnum > rblock)
			break;
		pe = e;
	}
	e = malloc(sizeof(log_entry));
	if(e == NULL)
		return NULL;

	block = get_block(fsinfo->dev_fd, rblock, fsinfo->cache_block_size);
	if(block == NULL) {
		free(e);
		return NULL;
	}
	mark_blocks_dirty(fsinfo->dev_fd, rblock, 1);

	e->bnum = rblock;
	e->block_data = block;
	e->written = false;
	e->last_sub_transaction = -1;
	if(pe == NULL) {
		e->next = transaction->entries;
		transaction->entries = e;
	}
	else {
		e->next = pe->next;
		pe->next = e;
	}
	if(transaction->num_entries == 0)
		transaction->start_time = system_time();
	transaction->num_entries++;
	
entry_found:
	transaction->mod_time = system_time();
	if(e->last_sub_transaction != transaction->sub_transaction_index) {
		e->last_sub_transaction = transaction->sub_transaction_index;
		transaction->sub_transaction_entries++;
	}
#if 1
	if(transaction->sub_transaction_entries >
	   transaction->sub_transaction_limit) {
		PRINT_ERROR(("cfs_get_block_logged: sub transaction limit exeeded "
		             "(%d of %d)\n", transaction->sub_transaction_entries,
		             transaction->sub_transaction_limit));
		//kernel_debugger("cfs sub transaction limit exeeded\n");
	}
#endif
	return e->block_data;
}

static status_t
write_log(cfs_info *fsinfo, cfs_log_transaction *log_transaction)
{
	status_t err;
	log_entry *e;
	int num_headers;
	cfs_log_entry_header *log_headers;
	cfs_log_entry_header *current_log_header;
	int current_header;
	iovec *vec;
	size_t veccount = 0;
	size_t first_veccount = 0;
	ssize_t bytes_written;
	off_t new_log_tail = 0;
	int max_blocks_per_log_entry = max_blocks_per_log_entry(fsinfo->_dev_block_size);
	
	if(fsinfo->_dev_block_size <= sizeof(cfs_log_entry_header) + 8) {
		PRINT_INTERNAL_ERROR(("write_log: dev block size %ld is too small\n",
		                      fsinfo->_dev_block_size));
		return B_ERROR;
	}

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;
	
	num_headers = (log_transaction->remaining_entry_count +
	               max_blocks_per_log_entry - 1) /
	              max_blocks_per_log_entry;

	log_headers = calloc(1, num_headers * fsinfo->_dev_block_size);
	if(log_headers == NULL)
		return B_NO_MEMORY;
	vec = malloc(sizeof(iovec) * (num_headers +
	                              log_transaction->remaining_entry_count));
	if(vec == NULL) {
		err = B_NO_MEMORY;
		goto err0;
	}

	e = log_transaction->entries;
	bytes_written = 0;
	for(current_header = 0; current_header < num_headers; current_header++) {
		int curr_block = 0;
		current_log_header = (cfs_log_entry_header *)
			((uint8*)log_headers + current_header * fsinfo->_dev_block_size);
		current_log_header->log_entry_id = CFS_LOG_ENTRY_ID;
		current_log_header->num_blocks = min(
			max_blocks_per_log_entry,
			log_transaction->remaining_entry_count -
			current_header * max_blocks_per_log_entry);
		vec[veccount].iov_base = current_log_header;
		vec[veccount].iov_len = fsinfo->_dev_block_size;
		bytes_written += fsinfo->_dev_block_size;
		if(first_veccount == 0 &&
		   fsinfo->log_tail + bytes_written >
		   fsinfo->log_base + fsinfo->log_size)
			first_veccount = veccount;
		veccount++;
		while(curr_block < max_blocks_per_log_entry &&
		      curr_block + current_header * max_blocks_per_log_entry <
		      log_transaction->remaining_entry_count) {
			if(e == NULL) {
				PRINT_INTERNAL_ERROR(("write_log: internal error 1\n"));
				err = B_ERROR;
				goto err1;
			}
			current_log_header->blocks[curr_block] =
			                                e->bnum * fsinfo->cache_block_size;
			vec[veccount].iov_base = e->block_data;
			vec[veccount].iov_len = fsinfo->cache_block_size;
			bytes_written += fsinfo->cache_block_size;

			if(first_veccount == 0 &&
			   fsinfo->log_tail + bytes_written >
			   fsinfo->log_base + fsinfo->log_size)
				first_veccount = veccount;
			veccount++;
			curr_block++;
			e = e->next;
		}
	}
	if(e != NULL) {
		PRINT_INTERNAL_ERROR(("write_log: internal error 2\n"));
		err = B_ERROR;
		goto err1;
	}
	
	if(((fsinfo->log_tail + fsinfo->log_size - fsinfo->log_head) %
	    fsinfo->log_size) + bytes_written + fsinfo->cache_block_size >
	   fsinfo->log_size) {
		PRINT_FLOW(("write_log: flush log, "
		            "(head %Ld, tail %Ld, need %ld of %Ld\n",
		            fsinfo->log_head, fsinfo->log_tail,
		            bytes_written, fsinfo->log_size));
		cfs_flush_log_blocks(fsinfo);
	}
	if(((fsinfo->log_tail + fsinfo->log_size - fsinfo->log_head) %
	    fsinfo->log_size) + bytes_written + fsinfo->cache_block_size >
	   fsinfo->log_size) {
		PRINT_INTERNAL_ERROR(("write_log: log overrun, " /*"old log lost "*/
		                      "(head %Ld, tail %Ld, need %ld of %Ld\n",
		                      fsinfo->log_head, fsinfo->log_tail,
		                      bytes_written, fsinfo->log_size));
		fsinfo->log_head = fsinfo->log_tail;
	}
	
	if(first_veccount) {
		PRINT_FLOW(("write_log: part 1 at %Ld\n", fsinfo->log_tail));
		bytes_written = writev_pos(fsinfo->dev_fd, fsinfo->log_tail, vec,
		                           first_veccount);
		if(bytes_written > 0) {
			ssize_t rv;
			PRINT_FLOW(("write_log: wrote %ld bytes\n", bytes_written));
			PRINT_FLOW(("write_log: part 2 at %Ld\n", fsinfo->log_base));
			rv = writev_pos(fsinfo->dev_fd, fsinfo->log_base,
			                vec + first_veccount, veccount - first_veccount);
			if(rv < 0)
				bytes_written = rv;
			else {
				PRINT_FLOW(("write_log: wrote %ld bytes\n", rv));
				new_log_tail = fsinfo->log_base + rv;
				bytes_written += rv;
			}
		}
	}
	else {
		PRINT_FLOW(("write_log: at %Ld\n", fsinfo->log_tail));
		bytes_written = writev_pos(fsinfo->dev_fd, fsinfo->log_tail,
		                           vec, veccount);
		if(bytes_written > 0) {
			PRINT_FLOW(("write_log: wrote %ld bytes\n", bytes_written));
		}
		new_log_tail = fsinfo->log_tail + bytes_written;
		if(new_log_tail == fsinfo->log_base + fsinfo->log_size)
			new_log_tail = fsinfo->log_base;
	}
	if(bytes_written != num_headers * fsinfo->_dev_block_size +
	                    log_transaction->remaining_entry_count *
	                    fsinfo->cache_block_size) {
		if(bytes_written < 0) {
			err = errno;
			PRINT_ERROR(("cfs_log write failed, %s\n", strerror(err)));
		}
		else {
			err = B_IO_ERROR;
			PRINT_ERROR(("cfs_log short write, wrote %ld of %ld bytes\n",
			             bytes_written,
			             num_headers * fsinfo->_dev_block_size +
			             log_transaction->remaining_entry_count *
			             fsinfo->cache_block_size));
		}
		goto err1;
	}
	ioctl(fsinfo->dev_fd, B_FLUSH_DRIVE_CACHE);
	err = write_log_info(fsinfo, fsinfo->log_head, new_log_tail);
	log_transaction->log_tail = new_log_tail;
	ioctl(fsinfo->dev_fd, B_FLUSH_DRIVE_CACHE);

	//err = B_NO_ERROR;
err1:
	free(vec);
err0:
	free(log_headers);
	return err;
}

status_t
cfs_flush_log(cfs_info *fsinfo)
{
	status_t err;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		return err;
		
	if(fsinfo->current_transaction != NULL) {
		if(fsinfo->current_transaction->active) {
			PRINT_INTERNAL_ERROR(("cfs_flush_log: transaction active\n"));
			return B_ERROR;
		}
		else {
			cfs_transaction *transaction = fsinfo->current_transaction;
			err = cfs_complete_transaction(fsinfo, transaction);
			if(err != B_NO_ERROR)
				return err;
			fsinfo->current_transaction = NULL;
			free(transaction);
		}
	}
	return B_NO_ERROR;
}

status_t
cfs_flush_log_blocks(cfs_info *fsinfo)
{
	status_t err;
	cfs_log_transaction *log_transaction;

	err = cfs_debug_check_write_lock(fsinfo);
	if(err != B_NO_ERROR)
		goto err1;

	if(fsinfo->logged_transactions != NULL) {
		flush_device(fsinfo->dev_fd, true);
		for(log_transaction = fsinfo->logged_transactions; log_transaction != NULL;
		    log_transaction = log_transaction->prev) {
			if(log_transaction->complete_sem == -1)
				continue;
			err = acquire_sem(log_transaction->complete_sem);
			//err = acquire_sem_etc(log_transaction->complete_sem, 1,
			//                      B_RELATIVE_TIMEOUT, 10000000);
			if(err == B_NO_ERROR) {
				delete_sem(log_transaction->complete_sem);
				log_transaction->complete_sem = -1;
			}
			else {
				PRINT_ERROR(("cfs_flush_log, wait failed %s\n", strerror(err)));
				goto err2;
			}
		}
		err = write_log_info(fsinfo, fsinfo->log_tail, fsinfo->log_tail);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_flush_log, write_log_info failed %s\n", strerror(err)));
		}
		while(fsinfo->logged_transactions != NULL) {
			log_transaction = fsinfo->logged_transactions;
			fsinfo->logged_transactions = log_transaction->prev;
			cfs_free_log_transaction(log_transaction);
		}
	}
err2:
err1:
	return err;
}

static void
cfs_log_flusher(void *arg, int phase) /* called from kernel daemon */
{
	status_t err;
	cfs_info *fsinfo = arg;
	cfs_log_transaction *log_transaction;
	cfs_log_transaction *last_flushed_log_transaction = NULL;
	cfs_log_transaction *last_unflushed_log_transaction = NULL;
	
	err = cfs_lock_write(fsinfo);
	if(err != B_NO_ERROR) {
		PRINT_ERROR(("cfs_log_flusher: can't get write lock\n"));
		return;
	}

	for(log_transaction = fsinfo->logged_transactions; log_transaction != NULL;
	    log_transaction = log_transaction->prev) {
		if(last_flushed_log_transaction == NULL)
			last_flushed_log_transaction = log_transaction;
		if(log_transaction->complete_sem == -1)
			continue;
		err = acquire_sem_etc(log_transaction->complete_sem, 1,
		                      B_RELATIVE_TIMEOUT, 0);
		if(err == B_NO_ERROR) {
			delete_sem(log_transaction->complete_sem);
			log_transaction->complete_sem = -1;
			PRINT_FLOW(("cfs_log_flusher: transaction blocks written\n"));
		}
		else {
			PRINT_FLOW(("cfs_log_flusher: wait failed %s\n", strerror(err)));
			last_flushed_log_transaction = NULL;
			last_unflushed_log_transaction = log_transaction;
		}
	}
	if(last_flushed_log_transaction != NULL) {
		PRINT_FLOW(("cfs_log_flusher: log_head changed to %Ld\n",
		            last_flushed_log_transaction->log_tail));
		err = write_log_info(fsinfo, last_flushed_log_transaction->log_tail,
		                     fsinfo->log_tail);
		if(err != B_NO_ERROR) {
			PRINT_ERROR(("cfs_log_flusher, write_log_info failed %s\n",
			             strerror(err)));
		}
		else {
			if(last_unflushed_log_transaction != NULL) {
				if(last_unflushed_log_transaction->prev !=
				   last_flushed_log_transaction) {
					PRINT_INTERNAL_ERROR(("cfs_log_flusher internal error 1\n"));
					return;
				}
				last_unflushed_log_transaction->prev = NULL;
			}
			else {
				if(fsinfo->logged_transactions !=
				   last_flushed_log_transaction) {
					PRINT_INTERNAL_ERROR(("cfs_log_flusher internal error 2\n"));
					return;
				}
				fsinfo->logged_transactions = NULL;
			}
			while(last_flushed_log_transaction != NULL) {
				log_transaction = last_flushed_log_transaction;
				last_flushed_log_transaction = log_transaction->prev;
				cfs_free_log_transaction(log_transaction);
			}
		}
	}
	
	if(fsinfo->current_transaction != NULL) {
		if(fsinfo->current_transaction->active) {
			PRINT_INTERNAL_ERROR(("cfs_log_flusher: transaction active\n"));
		}
		else {
			cfs_transaction *transaction = fsinfo->current_transaction;
			if(system_time() - transaction->mod_time > 1000000) {
				PRINT_FLOW(("cfs_log_flusher: 1 sec idle, "
				            "complete transaction\n"));

				err = cfs_complete_transaction(fsinfo, transaction);
				if(err != B_NO_ERROR) {
					PRINT_ERROR(("cfs_log_flusher: cfs_complete_transaction "
					             "failed, %s\n", strerror(err)));
				}
				else {
					fsinfo->current_transaction = NULL;
					free(transaction);
				}
			}
		}
	}

	cfs_unlock_write(fsinfo);
}
