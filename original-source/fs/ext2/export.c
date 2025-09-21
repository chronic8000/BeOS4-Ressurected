// This is the main export that the fsil uses to find the functions.
#include "export.h"

int32	api_version = B_CUR_FS_API_VERSION;

vnode_ops fs_entry = {

	&ext2_read_vnode,
	&ext2_write_vnode,
#ifndef RO
	&ext2_remove_vnode,
#else
	NULL,
#endif
	NULL, //	&ext2_secure_vnode,
	&ext2_walk,
	&ext2_access,
#ifndef RO
	&ext2_create,
	&ext2_mkdir,
	&ext2_symlink,
	&ext2_link,
	&ext2_rename,
	&ext2_unlink,
	&ext2_rmdir,
#else
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#endif
	&ext2_readlink,
	&ext2_opendir,
	&ext2_closedir,
	&ext2_free_dircookie,
	&ext2_rewinddir,
	&ext2_readdir,
	&ext2_open,
	&ext2_close,
	&ext2_free_cookie,
	&ext2_read,
#ifndef RO
	&ext2_write,
#else
	NULL,
#endif
	NULL, //	&ext2_readv,
	NULL, //	&ext2_writev,
	&ext2_ioctl,
	NULL, //	&ext2_setflags,
	&ext2_rstat,
#ifndef RO
	&ext2_wstat,
	&ext2_fsync,
#else
	NULL,
	NULL,
#endif
	NULL, //	&ext2_initialize,
	&ext2_mount,
	&ext2_unmount,
#ifndef RO
	&ext2_sync,
#else
	NULL,
#endif
	&ext2_rfsstat,
#ifndef RO
	&ext2_wfsstat,
#else
	NULL,
#endif
	NULL, // select
	NULL, // deselect
	NULL, //	&ext2_open_indexdir,
	NULL, //	&ext2_close_indexdir,
	NULL, //	&ext2_free_indexdircookie,
	NULL, //	&ext2_rewind_indexdir,
	NULL, //	&ext2_read_indexdir,
	NULL, //	&ext2_create_index,
	NULL, //	&ext2_remove_index,
	NULL, //	&ext2_rename_index,
	NULL, //	&ext2_stat_index,
&ext2_open_attrdir,
&ext2_close_attrdir,
&ext2_free_attrdircookie,
&ext2_rewind_attrdir,
&ext2_read_attrdir,
	NULL, //	&ext2_write_attr,
&ext2_read_attr,
	NULL, //	&ext2_remove_attr,
	NULL, //	&ext2_rename_attr,
&ext2_stat_attr,
	NULL, //	&ext2_open_query,
	NULL, //	&ext2_close_query,
	NULL, //	&ext2_free_querycookie,
	NULL, //	&ext2_read_query
};
