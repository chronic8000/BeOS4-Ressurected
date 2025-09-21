int read_data_stream(bfs_info *bfs, bfs_inode *bi,
					 dr9_off_t pos, char *buf, size_t *len);
int write_data_stream(bfs_info *bfs, bfs_inode *bi,
					  dr9_off_t pos, const char *buf, size_t *len);

int set_file_size(bfs_info *bfs, bfs_inode *bi, dr9_off_t new_size);
int free_data_stream(bfs_info *bfs, bfs_inode *bi);

/* XXXdbg this function will go away eventually */
void bnum_for_pos(bfs_info *bfs, bfs_inode *bi, dr9_off_t *data);

		
void trim_preallocated_blocks(bfs_info *bfs, bfs_inode *bi);
