#include "ntfs.h"

int ntfs_find_old_dbuf_or_get_new(ntfs_fs_struct *ntfs, vnode_id vnid, uint32 att_id, uint64 compression_group)
{
	bool foundit = false;
	int i;

//	dprintf("searching for decompression buffer for vnid %Ld, att_id %ld, compression group %Ld.\n", vnid, att_id, compression_group);

	// Add one to the lock around the decompression buffers
	LOCKM(ntfs->d_buf_mlock, 1);
	
	// Lock the dbuf search semaphore to keep more than one of us looking through the list
	LOCK(ntfs->d_search_lock);

	// Look for the buffer we've been asked to find
	for(i=0; i<DECOMPRESSION_BUFFERS; i++) {
		if((ntfs->dbuf[i].vnid == vnid)	&&
			(ntfs->dbuf[i].compression_group == compression_group) &&
			(ntfs->dbuf[i].att_id == att_id)) {
			// We've found it, lets check the use counter
			if(ntfs->dbuf[i].buf != NULL) {
				if(ntfs->dbuf[i].use_count > 0) {
					// It's already in use, so lets unlock the main lock around the buffers
					UNLOCKM(ntfs->d_buf_mlock, 1);
				}
				foundit = true;
				break;
			}
		}		
	}

	if(!foundit) {
		// We didn't find the one we were asking about, so lets find a free one
		// Find the LRU
		bigtime_t oldesttime = 0;
		int oldesttimeindex = -1;
				
		for(i=0; i<DECOMPRESSION_BUFFERS; i++) {
			if(ntfs->dbuf[i].buf == NULL) {
				// This one hasn't ever been used
				oldesttimeindex = i;
				break;
			}		
			if(ntfs->dbuf[i].use_count == 0) {
				if((oldesttimeindex == -1) || (oldesttime > ntfs->dbuf[i].timestamp)) {
					oldesttime = ntfs->dbuf[i].timestamp;
					oldesttimeindex = i;
				}										
			}
		}
		if(oldesttimeindex == -1) {
			// We didn't find a free one? This should not be
			ERRPRINT(("ntfs: could not find free decompression buffer.\n"));
			return -1;
		} else {
			i = oldesttimeindex;
		}
		// make sure the initialized flag is false
		ntfs->dbuf[i].initialized = false;
		// identify this compression buffer
		ntfs->dbuf[i].vnid = vnid;
		ntfs->dbuf[i].compression_group = compression_group;
		ntfs->dbuf[i].att_id = att_id;
	}

	// Update the usecount & timestamp
	ntfs->dbuf[i].use_count++;
	ntfs->dbuf[i].timestamp = system_time();

	UNLOCK(ntfs->d_search_lock);

	return i;
}

status_t ntfs_return_dbuf(ntfs_fs_struct *ntfs, int i)
{
	status_t err = B_NO_ERROR;

	LOCK(ntfs->d_search_lock);
	
	if((i < 0) || (i >= DECOMPRESSION_BUFFERS)) {
		err = EINVAL;
		goto out;
	}		
	
	// decrement the use counter
	ntfs->dbuf[i].use_count--;
	
	if(ntfs->dbuf[i].use_count < 0) {
		ERRPRINT(("ntfs_return_dbuf decompression buffer %d was returned one to many times\n", i));		
		err = EINVAL;
		goto out;
	}
	
	if(ntfs->dbuf[i].use_count == 0) {
		// remove a lock we've held since ntfs_find_old_dbuf_or_get_new
		UNLOCKM(ntfs->d_buf_mlock, 1);
	}

out:
	UNLOCK(ntfs->d_search_lock);

	return err;
}
