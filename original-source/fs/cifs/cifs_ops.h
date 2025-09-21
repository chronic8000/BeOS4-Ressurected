#ifndef _CIFS_OPS_H_
#define _CIFS_OPS_H_

extern int cifs_mount(nspace_id nsid, const char *device, ulong flags, void *_parms, size_t len, void **data, vnode_id *vnid);
extern int form_root_vnode(nspace *vol, vnode *root_vnode);
extern int cifs_unmount(void *_vol);
extern int cifs_access(void *_vol, void *_node, int mode);
extern int cifs_ioctl(void *_vol, void *_node, void *_cookie, int code, void *buf, size_t len);

extern int cifs_opendir(void *_vol, void *_node, void **_cookie);
extern int cifs_readdir(void *_vol, void *_dir, void *_cookie, long *num, struct dirent *entry, size_t bufsize);
extern int cifs_rewinddir(void *_vol, void *_node, void *_cookie);
extern int cifs_free_dircookie(void *_vol, void *_node, void *_cookie);
extern int cifs_closedir(void *_vol, void *_node, void *_cookie);
extern int cifs_walk(void *_vol, void *_dir, const char *file, char **newpath, vnode_id *_vnid);
extern int cifs_read_vnode(void *_vol, vnode_id vnid, char reenter, void **_node);
extern int cifs_write_vnode(void *_vol, void *_node, char reenter);
extern int cifs_rfsstat(void *_vol, struct fs_info *fss);
extern int cifs_wfsstat(void *_vol, struct fs_info *fss, long mask);
extern int form_dir_vnode(nspace *vol, vnode *dir_vnode, char *filename, vnode_id this_vnid, void **_node);
extern int form_file_vnode(nspace *vol, vnode *dir_vnode, char *filename, vnode_id this_vnid, void **_node);
extern int cifs_rstat(void *_vol, void *_node, struct stat *st);
extern int cifs_open(void *_vol, void *_node, int omode, void **_cookie);
extern int cifs_create(void *_vol, void *_dir, const char *name, int omode, int perms, vnode_id *vnid, void **_cookie);
extern int cifs_read(void *_vol, void *_node, void *_cookie, off_t pos, void *buf, size_t *len);
extern int cifs_close(void *_vol, void *_node, void *_cooke);
extern int cifs_free_cookie(void *_vol, void *_node, void *_cooke);
extern int cifs_wstat(void *_vol, void *_node, struct stat *st, long mask);
extern int cifs_write(void *_vol, void *_node, void *_cookie, off_t pos, const void *buf, size_t *len);
extern int cifs_unlink(void *_vol, void *_dir, const char *name);
extern int cifs_rename(void *_vol, void *_odir, const char *oldname, void *_ndir, const char *newname);
extern int cifs_mkdir(void *_vol, void *_dir, const char *name, int perms);

extern int cifs_remove_vnode(void *_vol, void *_node, char reenter);
extern int cifs_rmdir(void *_vol, void *_dir, const char *name);


#endif // _CIFS_OPS_H_
