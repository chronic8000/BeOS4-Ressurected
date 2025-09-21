#ifndef RO
int FillVnodePreallocList(ext2_fs_struct *ext2, vnode *file_vnode);
uint32 GetBlockFromFilePreallocList(ext2_fs_struct *ext2, vnode *file_vnode);
int CleanUpPreAlloc(ext2_fs_struct *ext2, vnode *file_vnode);
int AddBlockToPreAllocList(ext2_fs_struct *ext2, uint32 block_num);
int RemoveBlockFromPreAllocList(ext2_fs_struct *ext2, uint32 block_num);
bool FindPreAlloc(ext2_fs_struct *ext2, uint32 block_num);
#endif
