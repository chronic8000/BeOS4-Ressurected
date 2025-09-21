int bfs_create(void *ns, void *_dir, const char *name, int perms,
			   int omode, vnode_id *vnid, void **cookie);
int bfs_free_cookie(void *ns, void *file, void *cookie);
int bfs_read_vnode(void *ns, vnode_id vnid, char r, void **file);
int bfs_write_vnode(void *ns, void *node, char r);
int bfs_remove_vnode(void *ns, void *node, char r);
int bfs_open(void *ns, void *file, int omode, void **cookie);
int bfs_access(void *ns, void *file, int omode);
int bfs_close(void *ns, void *file, void *cookie);
int bfs_read(void *ns, void *node, void *cookie, dr9_off_t pos, void *buf, size_t *len);
int bfs_write(void *ns, void *node, void *cookie, dr9_off_t pos, const void *buf, size_t *len);
int bfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len);
int bfs_setflags(void *ns, void *node, void *cookie, int flags);
int bfs_rstat(void *ns, void *node, struct stat *st);
int bfs_wstat(void *ns, void *node, struct stat *st, long mask);
int bfs_lock(void *ns, void *node);
int bfs_unlock(void *ns, void *node);

int bfs_rename(void *ns, void *olddir, const char *oldname, void *newdir,
			   const char *newname);
int bfs_unlink(void *ns, void *dir, const char *name);
int bfs_sync(void *ns);
int bfs_fsync(void *ns, void *node);


int bfs_disable_notification(void *ns, void *node, void *cookie);
int bfs_enable_notification(void *ns, void *node, void *cookie);
int bfs_lock_attrs(void *ns, void *node, void *cookie, int flags);
int bfs_unlock_attrs(void *ns, void *node, void *cookie);

int do_unlink(bfs_info *bfs, bfs_inode *dir, const char *name,
			int must_be_dir, uid_t owner);

bfs_inode *make_file(bfs_info *bfs, bfs_inode *dir, uint mode_bits,
					 vnode_id *result);

int check_permission(bfs_inode *bi, int omode);

int bfs_readlink(void *ns, void *node, char *buf, size_t *bufsize);
int bfs_symlink(void *ns, void *_dir, const char *name, const char *path);
int reserve_space(bfs_info *bfs, dr9_off_t num_blocks);
void unreserve_space(bfs_info *bfs, dr9_off_t num_blocks);
