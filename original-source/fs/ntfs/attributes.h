void *ntfs_find_attribute(vnode *v, uint8 attribute_to_find, uint32 num_to_find);
void *ntfs_find_attribute_by_name(vnode *v, uint8 attribute_to_find, const char *name);
status_t load_attributes(ntfs_fs_struct *ntfs, vnode *file_vnode, void *FILE_buf, uint64 *MFT_records, uint32 *num_MFT_records);

//status_t ntfs_verify_attribute_list(vnode *v);

#define ntfs_verify_attribute_list(x) 
