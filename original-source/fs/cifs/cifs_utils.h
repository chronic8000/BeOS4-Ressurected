#ifndef _CIFS_UTILS_H
#define _CIFS_UTILS_H



extern int char_from_ptr(uchar **p, char *c, bool advance);
extern int uchar_from_ptr(uchar **p, uchar *uc, bool advance);
extern int ushort_from_ptr(uchar **p, unsigned short *s, bool advance);
extern int ulong_from_ptr(uchar **p, unsigned long *l, bool advance);
extern int ulonglong_from_ptr(uchar **p, uint64 *big, bool advance);
extern int char_to_ptr(uchar **p, char x);
extern int uchar_to_ptr(uchar **p, uchar x);
extern int ushort_to_ptr(uchar **p, unsigned short s);
extern int ulong_to_ptr(uchar **p, unsigned long l);
extern int long_to_ptr(uchar **p, unsigned long l);
extern int str_to_ptr(uchar **p, char *string);
extern int ulonglong_to_ptr(uchar **p, uint64 big);
extern int longlong_to_ptr(uchar **p, uint64 big);
extern int unicode_to_ptr(uchar **p, char *ustring);
extern int unicode_strlen(char *unicode_string);
extern char *get_unicode_from_cstring(char *cstring);
extern void get_cstring_from_smb_unicode(uchar *buf, char *cstring, const int maxlen);
extern int put_cifs_header(nspace *vol, uchar *buf);
extern status_t translate_error(uchar errclass, ushort errcode);
extern char *getuser(char *prompt);
extern char *getpass(const char *prompt);
extern int check_permission(vnode *vn, int omode);
extern size_t get_io_size(nspace *vol, size_t req);
extern int_dir_entry *find_in_dir(vnode *dir, vnode_id vnid);
extern int_dir_entry *next_in_dir(vnode *dir, dircookie *cookie);
extern status_t del_in_dir(vnode *dir, vnode_id vnid);
extern status_t add_to_dir(vnode *dir, int_dir_entry *entry);



#endif // _CIFS_UTILS_H
