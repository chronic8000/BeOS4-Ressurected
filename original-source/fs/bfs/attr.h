int bfs_read_attr(void *ns, void *node, const char *name, int type,
				  void *buf, size_t *len, dr9_off_t pos);
int bfs_write_attr(void *ns, void *node, const char *name, int type,
				   const void *buf, size_t *len, dr9_off_t pos);
int bfs_remove_attr(void *ns, void *node, const char *name);
int bfs_move_attr(void *ns, void *nodea, void *nodeb);

int	bfs_open_attrdir(void *ns, void *node, void **cookie);
int	bfs_close_attrdir(void *ns, void *node, void *cookie);
int bfs_free_attr_dircookie(void *ns, void *node, void *cookie);
int	bfs_rewind_attrdir(void *ns, void *node, void *cookie);
int	bfs_read_attrdir(void *ns, void *node, void *cookie, long *num,
					 struct dirent *buf, size_t bufsize);

int bfs_stat_attr(void *ns, void *node,const char *name,struct attr_info *buf);

int free_attributes(bfs_info *bfs, bfs_inode *bi);

int create_name_attr(bfs_info *bfs, bfs_inode *bi, const char *name);
int get_name_attr(bfs_info *bfs, bfs_inode *bi, char *buf, int len);
int change_name_attr(bfs_info *bfs, bfs_inode *bi, const char *newname);
int kill_name_attr(bfs_info *bfs, bfs_inode *bi);

int dump_small_data(int argc, char **argv);

int internal_read_attr(bfs_info *bfs, bfs_inode *bi, const char *name,
					   int type, void *buf, size_t *len, dr9_off_t pos);
int internal_stat_attr(bfs_info *bfs, bfs_inode *bi, const char *name,
					   struct attr_info *buf);
