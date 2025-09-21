#ifndef RO
int ext2_find_n_free_blocks(ext2_fs_struct *ext2, uint32 *blocks, uint32 start_block, int n);
int ext2_mark_blocks(ext2_fs_struct *ext2, uint32 *blocks, int n, bool mark);
int ext2_allocate_blocks_for_file(ext2_fs_struct *ext2, vnode *file_vnode, int num_blocks);
int ext2_mark_blocks_free(ext2_fs_struct *ext2, uint32 *blocks, int n);
int ext2_mark_blocks_in_use(ext2_fs_struct *ext2, uint32 *blocks, int n);
int ext2_clear_block(ext2_fs_struct *ext2, uint32 block);
#endif
