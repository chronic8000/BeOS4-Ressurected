bfs_info *open_bfs(char *device, int flags);
void      close_bfs(bfs_info *bfs);

int bfs_mount(nspace_id nsid, const char *device, ulong flags,
			  void *parms, size_t len, void **data, vnode_id *vnid);
int bfs_unmount(void *ns);

int bfs_initialize(const char *devname, void *parms, size_t len);
