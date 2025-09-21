#include <BeBuild.h>

#if FIXED
vnode_ops bfs_ops =  {
#else
vnode_ops fs_entry = {
#endif
      &bfs_read_vnode,
      &bfs_write_vnode,
      &bfs_remove_vnode,
      NULL,                  /* secure_vnode */
      &bfs_walk,
      &bfs_access,
      &bfs_create,
      &bfs_mkdir,
      &bfs_symlink,          /* symlink */
      NULL,                  /* link */
      &bfs_rename,
      &bfs_unlink,
      &bfs_rmdir,
      &bfs_readlink,         /* readlink */

      &bfs_opendir,
      &bfs_closedir,
      &bfs_free_dircookie,
      &bfs_rewinddir,
      &bfs_readdir,

      &bfs_open,
      &bfs_close,
      &bfs_free_cookie,
      &bfs_read,
      &bfs_write,
      NULL, /* readv */
      NULL, /* writev */
      &bfs_ioctl,
      &bfs_setflags,
      &bfs_rstat,
      &bfs_wstat,
      &bfs_fsync,
      &bfs_initialize,
      &bfs_mount,
      &bfs_unmount,
      &bfs_sync,

      &bfs_rfsstat,           /* XXX */
      &bfs_wfsstat,           /* XXX */

      &bfs_select,
      &bfs_deselect,

      &bfs_open_indexdir,
      &bfs_close_indexdir,
      &bfs_free_index_dircookie,
      &bfs_rewind_indexdir,
      &bfs_read_indexdir,

      &bfs_create_index,
      &bfs_remove_index,
      NULL,
      &bfs_stat_index,

      &bfs_open_attrdir,
      &bfs_close_attrdir,
      &bfs_free_attr_dircookie,
      &bfs_rewind_attrdir,
      &bfs_read_attrdir,

      &bfs_write_attr,
      &bfs_read_attr,
      &bfs_remove_attr,
      NULL,
      &bfs_stat_attr,

      &bfs_open_query,
      &bfs_close_query,
      &bfs_free_query_cookie,
      &bfs_read_query,
      &bfs_wake_vnode,
      &bfs_suspend_vnode
};

int32	api_version = B_CUR_FS_API_VERSION;
