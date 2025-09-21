//status_t CalculateBlockPointerPos(ext2_fs_struct *ext2, uint32 block_to_find, uint32 *level, uint32 pos[]);
status_t ext2_get_indirect_block_pointer_cache_block(ext2_fs_struct *ext2, vnode *file_vnode, uint32 **cache_block, uint32 level, uint32 pos[], uint32 *block_loaded);
status_t ext2_get_block_num_from_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 *block_num, uint32 file_block);
status_t ext2_get_last_block_num_from_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 *block_num);

#ifndef RO
int ext2_add_block_to_extent(ext2_fs_struct *ext2, vnode *file_vnode, uint32 block);
#endif
