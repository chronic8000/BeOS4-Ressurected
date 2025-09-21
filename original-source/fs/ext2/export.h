#ifndef EXPORT_H

#define EXPORT_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "fsproto.h"

int ext2_read_vnode(void *ns, vnode_id vnid, char r, void **node);
int	ext2_write_vnode(void *ns, void *node, char r);
int	ext2_remove_vnode(void *ns, void *node, char r);
/*int	ext2_secure_vnode(void *ns, void *node);*/

int	ext2_walk(void *ns, void *base, const char *file, char **newpath,
					vnode_id *vnid);

int	ext2_access(void *ns, void *node, int mode);

int	ext2_create(void *ns, void *dir, const char *name,
					int omode, int perms, vnode_id
*vnid, void **cookie);
int	ext2_mkdir(void *ns, void *dir, const char *name,	int perms);
int	ext2_symlink(void *ns, void *dir, const char *name,
					const char *path);
int ext2_link(void *ns, void *dir, const char *name, void *node);

int	ext2_rename(void *ns, void *olddir, const char *oldname,
					void *newdir, const char *newname);
int	ext2_unlink(void *ns, void *dir, const char *name);
int	ext2_rmdir(void *ns, void *dir, const char *name);
int	ext2_readlink(void *ns, void *node, char *buf, size_t *bufsize);

int ext2_opendir(void *ns, void *node, void **cookie);
int	ext2_closedir(void *ns, void *node, void *cookie);
int ext2_free_dircookie(void *ns, void *node, void *cookie);
int	ext2_rewinddir(void *ns, void *node, void *cookie);
int	ext2_readdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);

int	ext2_open(void *ns, void *node, int omode, void **cookie);
int	ext2_close(void *ns, void *node, void *cookie);
int ext2_free_cookie(void *ns, void *node, void *cookie);
int ext2_read(void *ns, void *node, void *cookie, off_t pos, void *buf,
					size_t *len);
int ext2_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len);
int	ext2_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len);
/*int	ext2_setflags(void *ns, void *node, void *cookie, int flags);*/
int	ext2_rstat(void *ns, void *node, struct stat *);
int ext2_wstat(void *ns, void *node, struct stat *, long mask);
int	ext2_fsync(void *ns, void *node);
/*int	ext2_initialize(const char *devname, void *parms, size_t len);*/
int	ext2_mount(nspace_id nsid, const char *devname, ulong flags,
					void *parms, size_t len, void
**data, vnode_id *vnid);
int	ext2_unmount(void *ns);
int	ext2_sync(void *ns);
int ext2_rfsstat(void *ns, struct fs_info *);
int ext2_wfsstat(void *ns, struct fs_info *, long mask);


/*
int	ext2_open_indexdir(void *ns, void **cookie);
int	ext2_close_indexdir(void *ns, void *cookie);
int ext2_free_indexdircookie(void *ns, void *node, void *cookie);
int	ext2_rewind_indexdir(void *ns, void *cookie);
int	ext2_read_indexdir(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
int	ext2_create_index(void *ns, const char *name, int type, int
flags);
int	ext2_remove_index(void *ns, const char *name);
int	ext2_rename_index(void *ns, const char *oldname,
					const char *newname);
int	ext2_stat_index(void *ns, const char *name, struct index_info
*buf);
*/
int	ext2_open_attrdir(void *ns, void *node, void **cookie);
int	ext2_close_attrdir(void *ns, void *node, void *cookie);
int ext2_free_attrdircookie(void *ns, void *node, void *cookie);
int	ext2_rewind_attrdir(void *ns, void *node, void *cookie);
int	ext2_read_attrdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
/*
int	ext2_remove_attr(void *ns, void *node, const char *name);
int	ext2_rename_attr(void *ns, void *node, const char *oldname,
					const char *newname);
*/
int	ext2_stat_attr(void *ns, void *node, const char *name,
					struct attr_info *buf);

/*
int	ext2_write_attr(void *ns, void *node, const char *name, int type,
					const void *buf, size_t *len, off_t pos);
*/
int	ext2_read_attr(void *ns, void *node, const char *name, int type,
					void *buf, size_t *len, off_t pos);
/*
int	ext2_open_query(void *ns, const char *query, ulong flags,
					port_id port, long token, void
**cookie);
int	ext2_close_query(void *ns, void *cookie);
int ext2_free_querycookie(void *ns, void *node, void *cookie);
int	ext2_read_query(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
*/

#ifdef __cplusplus
}
#endif

#endif
