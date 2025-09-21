status_t ntfs_load_data_attribute(vnode *file_vnode, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_index_alloc_attribute(vnode *file_vnode, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_file_name_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_std_info_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_volume_name_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_index_root_attribute(vnode *v, void *buf, ntfs_fs_struct *ntfs);
status_t ntfs_load_att_list_attribute(vnode *file_vnode, void *buf, ntfs_fs_struct *ntfs, uint64 *MFT_records, uint32 *num_MFT_records);

