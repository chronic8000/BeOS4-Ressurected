struct log_handle;

struct log_handle *start_transaction(bfs_info *bfs);
ssize_t            log_write_blocks(bfs_info *bfs, struct log_handle *lh,
									dr9_off_t bnum, const void *data,
									int nblocks, int need_update);
int                end_transaction(bfs_info *bfs, struct log_handle *lh);
void               abort_transaction(bfs_info *bfs, struct log_handle *lh);

int                create_log(bfs_info *bfs);
int                init_log(bfs_info *bfs);
void               shutdown_log(bfs_info *bfs);
void               force_log_flush(bfs_info *bfs);
void               sync_log(bfs_info *bfs);
