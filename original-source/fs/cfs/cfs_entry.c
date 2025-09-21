#include "cfs_entry.h"

vnode_ops fs_entry = {
	&cfs_read_vnode,
	&cfs_write_vnode,
	&cfs_remove_vnode,
	NULL, /* secure_vnode */
	&cfs_walk,
	&cfs_access,
	&cfs_create,
	&cfs_mkdir,
	&cfs_symlink,
	NULL, /* link */
	&cfs_rename,
	&cfs_unlink,
	&cfs_rmdir,
	&cfs_readlink,
	&cfs_opendir,
	&cfs_closedir,
	&cfs_free_dircookie,
	&cfs_rewinddir,
	&cfs_readdir,
	&cfs_open,
	&cfs_close,
	&cfs_free_cookie,
	&cfs_read,
	&cfs_write,
	NULL, /* readv */
	NULL, /* writev */
	&cfs_ioctl,
	NULL, /* setflags */
	&cfs_rstat,
	&cfs_wstat,
	&cfs_fsync,
	NULL, /* initialize */
	&cfs_mount,
	&cfs_unmount,
	&cfs_sync,
	&cfs_rfsstat,
	&cfs_wfsstat,
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* open_indexdir */
	NULL, /* close_indexdir */
	NULL, /* free_indexdircookie */
	NULL, /* rewind_indexdir */
	NULL, /* read_indexdir */
	NULL, /* create_index */
	NULL, /* remove_index */
	NULL, /* rename_index */
	NULL, /* stat_index */

/*NULL,NULL,NULL,NULL,NULL,NULL,NULL,*/
	&cfs_open_attrdir,
	&cfs_close_attrdir,
	&cfs_free_attrcookie,
	&cfs_rewind_attrdir,
	&cfs_read_attrdir,
	&cfs_write_attr,
	&cfs_read_attr,
	&cfs_remove_attr,
	NULL, /* rename_attr */
/*NULL,*/
	&cfs_stat_attr,
	NULL, /* open_query */
	NULL, /* close_query */
	NULL, /* free_querycookie */
	NULL, /* read_query */
    &cfs_wake_vnode,
    &cfs_suspend_vnode
};

int32	api_version = B_CUR_FS_API_VERSION;
