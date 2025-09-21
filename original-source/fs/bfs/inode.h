
int  init_inodes(bfs_info *bfs);
void shutdown_inodes(bfs_info *bfs);

int        update_inode(bfs_info *bfs, bfs_inode *bi);
int        free_inode(bfs_info *bfs, inode_addr *ia);
bfs_inode *allocate_inode(bfs_info *bfs, bfs_inode *parent, int mode);
	
