#include <string.h>
#include "bfs.h"
#ifdef __BEOS__
#include <fs_info.h>
#else
#include "fs_info.h"
#endif

int
bfs_rfsstat(void *ns, struct fs_info *fst)
{
	bfs_info *bfs = (bfs_info *)ns;

/* printf("bfs_rfstat called (%d)\n", find_thread(NULL)); */

	memset(fst, 0, sizeof(*fst));

/*	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0); */

	fst->flags = B_FS_HAS_MIME | B_FS_HAS_ATTR | B_FS_HAS_QUERY |
				 B_FS_IS_PERSISTENT;
	if (bfs->flags & FS_READ_ONLY)
		fst->flags |= B_FS_IS_READONLY;

	if (bfs->flags & FS_IS_REMOVABLE)
		fst->flags |= B_FS_IS_REMOVABLE;

	fst->block_size   = bfs->dsb.block_size;
	fst->io_size      = 64 * 1024;  /* XXXdbg for now this is ok... */
	fst->total_blocks = bfs->dsb.num_blocks;
	fst->free_blocks  = bfs->dsb.num_blocks - bfs->dsb.used_blocks;

	strncpy(fst->volume_name, bfs->dsb.name, sizeof(fst->volume_name));

	/* ensure null termination */
	fst->volume_name[sizeof(fst->volume_name) - 1] = '\0';

	strcpy(fst->fsh_name, "bfs");

/*	release_sem_etc(bfs->sem, MAX_READERS, 0); */
	
	return 0;
}


int
bfs_wfsstat(void *ns, struct fs_info *fst, long mask)
{
	int       ret = EINVAL;
	bfs_info *bfs = (bfs_info *)ns;
	
	if (bfs->flags & FS_READ_ONLY) {
		return EINVAL;
	}
	
	if (mask & WFSSTAT_NAME) {
		if (strcmp(fst->volume_name, "boot") == 0)
			return EINVAL;

		if (strchr(fst->volume_name, '/'))
			return EINVAL;
	}

	acquire_sem_etc(bfs->sem, MAX_READERS, 0, 0);

	if (mask & WFSSTAT_NAME) {
		strncpy(bfs->dsb.name, fst->volume_name, sizeof(bfs->dsb.name));

		/* ensure null termination */
		bfs->dsb.name[sizeof(bfs->dsb.name) - 1] = '\0';
		ret = write_super_block(bfs);
	}

	release_sem_etc(bfs->sem, MAX_READERS, 0);
	
	return ret;
}
