//ssize_t cached_read_pos(ext2_fs_struct *ext2, int fd, off_t pos, void *buf, size_t length_to_read);
status_t ext2_read_inode(ext2_fs_struct *ext2, ext2_inode *inode, uint32 inode_num);
int ext2_read_file(ext2_fs_struct *ext2, vnode *file_vnode, void *buf, uint32 pos, size_t read_len, size_t *bytes_read);
//int ext2_read_file_block(ext2_fs_struct *ext2, vnode *file_vnode, void *buf, unsigned int block);
//int ext2_read_block(ext2_fs_struct *ext2, int block, void *buf);
void *ext2_get_cache_block(ext2_fs_struct *ext2, uint32 cache_block);
status_t ext2_release_cache_block(ext2_fs_struct *ext2, uint32 cache_block);
void *ext2_get_file_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block);
status_t ext2_release_file_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block);

#ifndef RO
int ext2_write_inode(ext2_fs_struct *ext2, ext2_inode *inode, int inode_num);
int ext2_write_all_dirty_group_desc(ext2_fs_struct *ext2);
int ext2_write_group_desc(ext2_fs_struct *ext2, int group_num);
int ext2_write_superblock(ext2_fs_struct *ext2);
int ext2_write_file(ext2_fs_struct *ext2, vnode *file_vnode, void *buf, unsigned int pos, unsigned int write_len);
status_t ext2_mark_blocks_dirty(ext2_fs_struct *ext2, uint32 cache_block, int num_blocks);
status_t ext2_mark_file_block_dirty(ext2_fs_struct *ext2, vnode *file_vnode, uint32 file_block);
#endif
