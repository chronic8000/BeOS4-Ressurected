uint64 ntfs_time_to_posix_time(uint64 posix_time);
status_t ntfs_unicode_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen);
status_t ntfs_utf8_to_unicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen);
void ntfs_toupper_unicode(ntfs_fs_struct *ntfs, char *a, int len);
int ntfs_strcasecmp_unicode(ntfs_fs_struct *ntfs, uint16 *a, int a_len, uint16 *b, int b_len);
int ntfs_strcmp_unicode(uint16 *a, int a_len, uint16 *b, int b_len);
