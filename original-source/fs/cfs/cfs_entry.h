#ifndef CFS_ENTRY_H
#define CFS_ENTRY_H

#include <fsproto.h>

#include "cfs_pm.h"

int cfs_read_vnode(void *ns, vnode_id vnid, char r, void **node);
int cfs_write_vnode(void *ns, void *node, char r);
int cfs_remove_vnode(void *ns, void *node, char r);
/*int cfs_secure_vnode(void *ns, void *node);*/

int cfs_walk(void *ns, void *base, const char *file, char **newpath,
             vnode_id *vnid);

int cfs_access(void *ns, void *node, int mode);

int cfs_create(void *ns, void *dir, const char *name, int omode, int perms,
               vnode_id *vnid, void **cookie);
int cfs_mkdir(void *ns, void *dir, const char *name,	int perms);
int cfs_symlink(void *ns, void *dir, const char *name, const char *path);
/*int cfs_link(void *ns, void *dir, const char *name, void *node);*/

int cfs_rename(void *ns, void *olddir, const char *oldname, void *newdir,
               const char *newname);
int cfs_unlink(void *ns, void *dir, const char *name);
int cfs_rmdir(void *ns, void *dir, const char *name);

int cfs_readlink(void *ns, void *node, char *buf, size_t *bufsize);

int cfs_opendir(void *ns, void *node, void **cookie);
int cfs_closedir(void *ns, void *node, void *cookie);
int cfs_free_dircookie(void *ns, void *node, void *cookie);

int cfs_rewinddir(void *ns, void *node, void *cookie);
int cfs_readdir(void *ns, void *node, void *cookie, long *num,
                struct dirent *buf, size_t bufsize);

int cfs_open(void *ns, void *node, int omode, void **cookie);
int cfs_close(void *ns, void *node, void *cookie);
int cfs_free_cookie(void *ns, void *node, void *cookie);
int cfs_read(void *ns, void *node, void *cookie, off_t pos, void *buf,
             size_t *len);
int cfs_write(void *ns, void *node, void *cookie, off_t pos, const void *buf,
              size_t *len);
/*int cfs_readv(void *ns, void *node, void *cookie, off_t pos, const iovec *vec,
              size_t count, size_t *len);
int cfs_writev(void *ns, void *node, void *cookie, off_t pos, const iovec *vec,
               size_t count, size_t *len);*/
int cfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf,
              size_t len);
/*int cfs_setflags(void *ns, void *node, void *cookie, int flags);*/

int cfs_rstat(void *ns, void *node, struct stat *);
int cfs_wstat(void *ns, void *node, struct stat *, long mask);
int cfs_fsync(void *ns, void *node);
/*int cfs_initialize(const char *devname, void *parms, size_t len);*/

int cfs_mount(nspace_id nsid, const char *devname, ulong flags, void *parms,
              size_t len, void **data, vnode_id *vnid);
int cfs_unmount(void *ns);
int cfs_sync(void *ns);
int cfs_rfsstat(void *ns, struct fs_info *);
int cfs_wfsstat(void *ns, struct fs_info *, long mask);

/*int cfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref,
               selectsync *sync);
int cfs_deselect(void *ns, void *node, void *cookie, uint8 event,
                 selectsync *sync);*/

/*int cfs_open_indexdir(void *ns, void **cookie);
int cfs_close_indexdir(void *ns, void *cookie);
int cfs_free_indexdircookie(void *ns, void *node, void *cookie);
int cfs_rewind_indexdir(void *ns, void *cookie);
int cfs_read_indexdir(void *ns, void *cookie, long *num, struct dirent *buf,
                      size_t bufsize);
int cfs_create_index(void *ns, const char *name, int type, int flags);
int cfs_remove_index(void *ns, const char *name);
int cfs_rename_index(void *ns, const char *oldname, const char *newname);
int cfs_stat_index(void *ns, const char *name, struct index_info *buf);*/

int cfs_open_attrdir(void *ns, void *node, void **cookie);
int cfs_close_attrdir(void *ns, void *node, void *cookie);
int cfs_free_attrcookie(void *ns, void *node, void *cookie);
int cfs_rewind_attrdir(void *ns, void *node, void *cookie);
int cfs_read_attrdir(void *ns, void *node, void *cookie, long *num,
                     struct dirent *buf, size_t bufsize);
int cfs_write_attr(void *ns, void *node, const char *name, int type,
                   const void *buf, size_t *len, off_t pos);
int cfs_read_attr(void *ns, void *node, const char *name, int type,
                  void *buf, size_t *len, off_t pos);
int cfs_remove_attr(void *ns, void *node, const char *name);
/*int cfs_rename_attr(void *ns, void *node, const char *oldname,
                    const char *newname);*/
int cfs_stat_attr(void *ns, void *node, const char *name,
                  struct attr_info *buf);


/*int cfs_open_query(void *ns, const char *query, ulong flags, port_id port,
                   long token, void **cookie);
int cfs_close_query(void *ns, void *cookie);
int cfs_free_querycookie(void *ns, void *node, void *cookie);
int cfs_read_query(void *ns, void *cookie, long *num,
                   struct dirent *buf, size_t bufsize);*/

#endif

