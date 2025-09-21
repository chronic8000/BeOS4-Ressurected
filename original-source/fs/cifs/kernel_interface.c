#include <BeBuild.h>
#define DEFINE_GLOBAL_VARS
#include "cifs_globals.h"
#include "cifs_ops.h"
#include "attr.h"

void malloc_failed(const char* str, int s) {
	dprintf("%s %d\n", str, s);
//	dm_full_dump();
}

vnode_ops fs_entry = {
	&cifs_read_vnode,
	&cifs_write_vnode,
	&cifs_remove_vnode,
	NULL, // secure
	&cifs_walk,
	&cifs_access, // access
	&cifs_create, 
	&cifs_mkdir, // mkdir
	NULL, // symlink
	NULL, // link
	&cifs_rename, // rename
	&cifs_unlink, // unlink
	&cifs_rmdir, // rmdir
	NULL, // readlink

	&cifs_opendir,
	&cifs_closedir,
	&cifs_free_dircookie,
	&cifs_rewinddir,
	&cifs_readdir,

	&cifs_open,
	&cifs_close,
	&cifs_free_cookie,
	&cifs_read,
	&cifs_write,
	NULL, // readv
	NULL, // writev
	&cifs_ioctl, // ioctl
	NULL, // setflags
	&cifs_rstat,
	&cifs_wstat,
	NULL, // fsync
	NULL, // initialize
	&cifs_mount,
	&cifs_unmount,
	NULL, // sync

	&cifs_rfsstat,
	NULL, // wfsstat

	NULL, // select
	NULL, // deselect

	NULL, // open_indexdir
	NULL, // close_indexdir
	NULL, // free_indexdircookie
	NULL, // rewind_indexdir
	NULL, // read_indexdir

	NULL, // create_index
	NULL, // remove_index
	NULL, // rename_index
	NULL, // stat_index

	&cifs_open_attrdir, // open_attrdir
	&cifs_close_attrdir, // close_attrdir
	&cifs_free_attrcookie, // free_attrdircookie
	&cifs_rewind_attrdir, // rewind_attrdir
	&cifs_read_attrdir, // read_attrdir

	&cifs_write_attr, // write_attr
	&cifs_read_attr, // read_attr
	NULL, // remove_attr
	NULL, // rename_attr
	&cifs_stat_attr, // stat_attr

	NULL, // open_query
	NULL, // close_query
	NULL, // free_querycookie
	NULL  // read_query
};

int32	api_version = B_CUR_FS_API_VERSION;
