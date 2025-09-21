int  create_indices(bfs_info *bfs);
int  init_indices(bfs_info *bfs);
void shutdown_indices(bfs_info *bfs);

int  add_to_main_indices(bfs_info *bfs, const char *name, bfs_inode *file);
int  del_from_main_indices(bfs_info *bfs, const char *name, bfs_inode *file);
int  bfs_create_index(void *ns, const char *name, int type, int flags);
int  bfs_remove_index(void *ns, const char *name);

int bfs_open_indexdir(void *ns, void **cookie);
int bfs_close_indexdir(void *ns, void *cookie);
int bfs_free_index_dircookie(void *ns, void *node, void *cookie);
int bfs_rewind_indexdir(void *ns, void *cookie);
int bfs_read_indexdir(void *ns, void *cookie, long *num,
					  struct dirent *buf, size_t bufsize);

int	bfs_stat_index(void *ns, const char *name, struct index_info *buf);


int  update_create_time_index(bfs_info *bfs, bfs_inode *bi,
							  bigtime_t new_time);
int  update_last_modified_time_index(bfs_info *bfs, bfs_inode *bi);
int  update_size_index(bfs_info *bfs, bfs_inode *bi);
int  update_name_index(bfs_info *bfs, bfs_inode *bi,
					   const char *oldname, const char *newname);

int  update_index_if_needed(bfs_info *bfs, const char *name, int type,
							const void *value, size_t size, bfs_inode *bi);
int  delete_from_index_if_needed(bfs_info *bfs, const char *name, int type,
								 const void *value, size_t size,bfs_inode *bi);

int  index_exists(bfs_info *bfs, const char *name);
int  get_index(bfs_info *bfs, const char *name, bfs_inode **index);


int  add_index_callback(bfs_inode *index,
						int      (*func)(int               type_of_event,
										 struct bfs_inode *bi,
										 int              *old_result,
										 void             *arg),
						void      *arg);

int  remove_index_callback(bfs_inode *index, void *arg);

