void init_debugging(bfs_info *bfs);
void print_sizes(void);
void dump_block_run(block_run *da);
void dump_inode_addr(inode_addr *ia);
void dump_data_stream(data_stream *ds);
void dump_inode(bfs_inode *inode);
void dump_disk_super_block(bfs_info *bfs, disk_super_block *sb);
void dump_dir(bfs_info *bfs, inode_addr *ia);
void dump_bfs(bfs_info *bfs);
