#ifndef _DIR_ENTRY_H
#define _DIR_ENTRY_H

extern int _entry_compare(void *_keya, void *_keyb);
extern void _entry_free(void *key);
extern status_t fill_vnode(nspace *vol, vnode *dir_vnode, vnode *this_vnode);
extern status_t get_entry(nspace *vol, vnode *dir, struct dirent *extern_entry, dircookie *cookie);
extern status_t update_entries(nspace *vol, vnode *vn, bool force);
extern status_t form_dir_entries(nspace *vol, vnode *dir, int_dir_entry **dir_contents, uchar *server_dir, ushort num_entries);
extern status_t get_dir(nspace **vol, vnode *dir, char *FindString);
extern status_t oldstyle_rfsstat_send(nspace *vol);
extern status_t oldstyle_rfsstat_recv(nspace *vol);
extern status_t oldstyle_rfsstat(nspace *vol);



#endif // _DIR_ENTRY_H
