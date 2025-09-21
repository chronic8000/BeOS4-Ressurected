int bfs_mkdir(void *ns, void *_dir, const char *name, int perms);

int bfs_rmdir(void *ns, void *dir, const char *name);

int bfs_opendir(void *ns, void *node, void **cookie);
int bfs_closedir(void *ns, void *node, void *cookie);
int bfs_rewinddir(void *ns, void *node, void *cookie);
int	bfs_readdir(void *ns, void *node, void *cookie, long *num,
				struct dirent *buf, size_t bufsize);
int bfs_free_dircookie(void *ns, void *node, void *cookie);


bfs_inode *make_dir(bfs_info *bfs, bfs_inode *dir,
					uint mode_bits, vnode_id *result);
int create_root_dir(bfs_info *bfs);
int lookup(bfs_inode *dir, const char *name, vnode_id *result);
int dir_is_empty(bfs_info *bfs, bfs_inode *dir);
