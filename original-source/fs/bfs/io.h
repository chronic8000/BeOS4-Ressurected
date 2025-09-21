int     init_bfs_io(bfs_info *bfs);
void    shutdown_bfs_io(bfs_info *bfs);

ssize_t read_blocks(bfs_info *bfs, dr9_off_t block_num, void *block, size_t nblocks);
ssize_t write_blocks(bfs_info *bfs, dr9_off_t block_num, const void *block, size_t nblocks);
int     read_super_block(bfs_info *bfs);
int     write_super_block(bfs_info *bfs);

