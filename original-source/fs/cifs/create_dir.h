#ifndef _CREATE_DIR_H_
#define _CREATE_DIR_H_

extern status_t create_dir(nspace *vol, vnode *dir, const char *name, int perms);
extern status_t create_dir_send(nspace *vol, vnode *dir, const char *name, int perms);
extern status_t create_dir_recv(nspace *vol, vnode *dir);

#endif // _CREATE_DIR_H_
