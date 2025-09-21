#include "cfs.h"
#include "cfs_debug.h"
#include "cfs_vnode.h"
#include "cfs_entry.h"
#include <KernelExport.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

status_t 
cfs_debug_check_read_lock(cfs_info *fsinfo)
{
	if(fsinfo->debug_read_lock_held <= 0 && fsinfo->debug_write_lock_held != 1) {
		dprintf("cfs_debug_check_read_lock: read lock count %ld, "
		        "write lock count %ld\n",
		        fsinfo->debug_read_lock_held, fsinfo->debug_write_lock_held);
		kernel_debugger("cfs_lock problem\n");
		return B_ERROR;
	}
	return B_OK;
}

status_t 
cfs_debug_check_write_lock(cfs_info *fsinfo)
{
	if(fsinfo->debug_write_lock_held != 1) {
		dprintf("cfs_debug_check_write_lock: read lock count %ld, "
		        "write lock count %ld\n",
		        fsinfo->debug_read_lock_held, fsinfo->debug_write_lock_held);
		kernel_debugger("cfs_lock problem\n");
		return B_ERROR;
	}
	return B_OK;
}

static status_t 
_cfs_lock_read(cfs_info *fsinfo, int flags)
{
	status_t err;
	err = acquire_sem_etc(fsinfo->read_write_lock, 1, flags, 0);
	if(err < B_NO_ERROR) {
		PRINT_ERROR(("cfs_lock_read: acquire_sem_etc failed %s\n",
		             strerror(err)));
	}
	else {
		int32 old_readcount;
		old_readcount = atomic_add(&fsinfo->debug_read_lock_held, 1);
		if(old_readcount < 0 || old_readcount >= CFS_MAX_READERS) {
			dprintf("cfs_lock_read: read count was %ld\n", old_readcount);
			kernel_debugger("cfs_lock problem\n");
		}
	}
	return err;
}

status_t 
cfs_lock_read(cfs_info *fsinfo)
{
	return _cfs_lock_read(fsinfo, B_CAN_INTERRUPT);
}

status_t 
cfs_lock_read_nointerrupt(cfs_info *fsinfo)
{
	return _cfs_lock_read(fsinfo, 0);
}

void 
cfs_unlock_read(cfs_info *fsinfo)
{
	int32 old_readcount;
	old_readcount = atomic_add(&fsinfo->debug_read_lock_held, -1);
	if(old_readcount <= 0 || old_readcount > CFS_MAX_READERS) {
		dprintf("cfs_unlock_read: read count was %ld\n", old_readcount);
		kernel_debugger("cfs_lock problem\n");
	}
	release_sem(fsinfo->read_write_lock);
}

static status_t 
_cfs_lock_write(cfs_info *fsinfo, int flags)
{
	status_t err;
	err = acquire_sem_etc(fsinfo->read_write_lock, CFS_MAX_READERS, flags, 0);
	if(err < B_NO_ERROR) {
		dprintf("_cfs_lock_write: acquire_sem_etc failed %s\n", strerror(err));
	}
	else {
		int32 old_writecount;
		old_writecount = atomic_add(&fsinfo->debug_write_lock_held, 1);
		if(old_writecount != 0) {
			dprintf("cfs_lock_write: write count was %ld\n", old_writecount);
			kernel_debugger("cfs_lock problem\n");
		}
	}
	return err;
}

status_t 
cfs_lock_write(cfs_info *fsinfo)
{
	return _cfs_lock_write(fsinfo, B_CAN_INTERRUPT);
}

status_t 
cfs_lock_write_nointerrupt(cfs_info *fsinfo)
{
	return _cfs_lock_write(fsinfo, 0);
}

void 
cfs_unlock_write(cfs_info *fsinfo)
{
	int32 old_writecount;
	old_writecount = atomic_add(&fsinfo->debug_write_lock_held, -1);
	if(old_writecount != 1) {
		dprintf("cfs_unlock_write: write count was %ld\n", old_writecount);
		kernel_debugger("cfs_lock problem\n");
	}
		
	release_sem_etc(fsinfo->read_write_lock, CFS_MAX_READERS, 0);
}

