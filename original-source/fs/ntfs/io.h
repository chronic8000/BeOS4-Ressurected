status_t ntfs_read_extent(ntfs_fs_struct *ntfs, extent_storage *extent, void *buf, off_t pos, size_t read_len, size_t *bytes_read);
status_t ntfs_fixup_buffer(void *buf, uint32 length);
status_t ntfs_read_FILE_record(ntfs_fs_struct *ntfs, FILE_REC record_num, char *buf);
status_t ntfs_load_vnode_from_MFT(ntfs_fs_struct *ntfs, vnode *file_vnode);

