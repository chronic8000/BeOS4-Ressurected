#include "cfs.h"
#include "cfs_entry.h"
#include "cfs_free_space.h"
#include "cfs_vnode.h"
#include "cfs_debug.h"
#include <KernelExport.h>
#include <errno.h>
#include <string.h>

int 
cfs_rfsstat(void *ns, struct fs_info *info)
{
	status_t err;
	cfs_info *cfsinfo = (cfs_info *)ns;

	err = cfs_lock_read(cfsinfo);
	if(err < B_NO_ERROR)
		goto err;

	//dprintf("cfs_rfsstat\n");

	memset(info, 0, sizeof(*info));
	info->root = cfsinfo->root_pos;
	info->flags = cfsinfo->fs_flags;
	info->block_size = 1;
	info->io_size = cfs_compressed_block_size;
	info->total_blocks = cfsinfo->size;
	info->free_blocks = cfs_get_free_space(cfsinfo);
	strcpy(info->volume_name, cfsinfo->name);
	//dprintf("volumename = %s\n", cfsinfo->name);
	strcpy(info->fsh_name, "cfs");

	cfs_unlock_read(cfsinfo);

	err = B_NO_ERROR;

err:
	return err;
}

int 
cfs_wfsstat(void *ns, struct fs_info *info, long mask)
{
	status_t err;
	status_t tmp_err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_transaction *transaction;
	
	switch(mask) {
		case WFSSTAT_NAME: {
			cfs_super_block_2 sb2;
					
			err = cfs_lock_write(fsinfo);
			if(err < B_NO_ERROR)
				goto err0;
	
			err = cfs_start_transaction(fsinfo, &transaction);
			if(err != B_NO_ERROR)
				goto err1;
	
			err = cfs_sub_transaction(fsinfo, transaction, 1);
			if(err != B_NO_ERROR)
				goto err2;
	
			err = cfs_read_superblock2(fsinfo, &sb2);
			if(err != B_NO_ERROR)
				goto err2;		
	
			strncpy(sb2.volume_name, info->volume_name, sizeof(sb2.volume_name));
			strncpy(fsinfo->name, info->volume_name, sizeof(sb2.volume_name));
			fsinfo->name[sizeof(fsinfo->name) - 1] = '\0';
	
			err = cfs_write_disk(fsinfo, transaction, fsinfo->super_block_2_pos,
			                     &sb2, get_sb2_size(fsinfo->fs_version));
			break;
		}
		default:
			err = B_NOT_ALLOWED;
			goto err0;
	}

err2:
	tmp_err = cfs_end_transaction(fsinfo, transaction);
	if(err == B_NO_ERROR)
		err = tmp_err;
err1:
	cfs_unlock_write(fsinfo);
err0:
	return err;
}

