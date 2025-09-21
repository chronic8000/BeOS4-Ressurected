#ifndef EXPORT_H

#define EXPORT_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "fsproto.h"

int ntfs_read_vnode(void *ns, vnode_id vnid, char r, void **node);
int	ntfs_write_vnode(void *ns, void *node, char r);
int	ntfs_remove_vnode(void *ns, void *node, char r);
/*int	ntfs_secure_vnode(void *ns, void *node);*/

int	ntfs_walk(void *ns, void *base, const char *file, char **newpath,
					vnode_id *vnid);

int	ntfs_access(void *ns, void *node, int mode);

int	ntfs_create(void *ns, void *dir, const char *name,
					int omode, int perms, vnode_id
*vnid, void **cookie);
int	ntfs_mkdir(void *ns, void *dir, const char *name,	int perms);
int	ntfs_symlink(void *ns, void *dir, const char *name,
					const char *path);
int ntfs_link(void *ns, void *dir, const char *name, void *node);

int	ntfs_rename(void *ns, void *olddir, const char *oldname,
					void *newdir, const char *newname);
int	ntfs_unlink(void *ns, void *dir, const char *name);
int	ntfs_rmdir(void *ns, void *dir, const char *name);
int	ntfs_readlink(void *ns, void *node, char *buf, size_t *bufsize);

int ntfs_opendir(void *ns, void *node, void **cookie);
int	ntfs_closedir(void *ns, void *node, void *cookie);
int ntfs_free_dircookie(void *ns, void *node, void *cookie);
int	ntfs_rewinddir(void *ns, void *node, void *cookie);
int	ntfs_readdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);

int	ntfs_open(void *ns, void *node, int omode, void **cookie);
int	ntfs_close(void *ns, void *node, void *cookie);
int ntfs_free_cookie(void *ns, void *node, void *cookie);
int ntfs_read(void *ns, void *node, void *cookie, off_t pos, void *buf,
					size_t *len);
int ntfs_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len);
int	ntfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len);
/*int	ntfs_setflags(void *ns, void *node, void *cookie, int flags);*/

int	ntfs_rstat(void *ns, void *node, struct stat *);
int ntfs_wstat(void *ns, void *node, struct stat *, long mask);
int	ntfs_fsync(void *ns, void *node);

/*int	ntfs_initialize(const char *devname, void *parms, size_t len);*/
int	ntfs_mount(nspace_id nsid, const char *devname, ulong flags,
					void *parms, size_t len, void
**data, vnode_id *vnid);
int	ntfs_unmount(void *ns);
int	ntfs_sync(void *ns);
int ntfs_rfsstat(void *ns, struct fs_info *);
int ntfs_wfsstat(void *ns, struct fs_info *, long mask);


/*
int	ntfs_open_indexdir(void *ns, void **cookie);
int	ntfs_close_indexdir(void *ns, void *cookie);
int ntfs_free_indexdircookie(void *ns, void *node, void *cookie);
int	ntfs_rewind_indexdir(void *ns, void *cookie);
int	ntfs_read_indexdir(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
int	ntfs_create_index(void *ns, const char *name, int type, int
flags);
int	ntfs_remove_index(void *ns, const char *name);
int	ntfs_rename_index(void *ns, const char *oldname,
					const char *newname);
int	ntfs_stat_index(void *ns, const char *name, struct index_info
*buf);
*/
int	ntfs_open_attrdir(void *ns, void *node, void **cookie);
int	ntfs_close_attrdir(void *ns, void *node, void *cookie);
int ntfs_free_attrdircookie(void *ns, void *node, void *cookie);
int	ntfs_rewind_attrdir(void *ns, void *node, void *cookie);
int	ntfs_read_attrdir(void *ns, void *node, void *cookie, long *num, struct dirent *buf, size_t bufsize);
//int	ntfs_remove_attr(void *ns, void *node, const char *name);
//int	ntfs_rename_attr(void *ns, void *node, const char *oldname, const char *newname);
int	ntfs_stat_attr(void *ns, void *node, const char *name, struct attr_info *buf);

//int	ntfs_write_attr(void *ns, void *node, const char *name, int type, const void *buf, size_t *len, off_t pos);
int	ntfs_read_attr(void *ns, void *node, const char *name, int type, void *buf, size_t *len, off_t pos);
/*
int	ntfs_open_query(void *ns, const char *query, ulong flags,
					port_id port, long token, void
**cookie);
int	ntfs_close_query(void *ns, void *cookie);
int ntfs_free_querycookie(void *ns, void *node, void *cookie);
int	ntfs_read_query(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
*/

#ifdef __cplusplus
}
#endif

#endif
