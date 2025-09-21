#ifndef _CREATE_FILE_H_
#define _CREATE_FILE_H_

extern status_t create_file(nspace *vol, vnode *dir, const char *name, int omode, int perms, vnode **node, vnode_id *vnid, filecookie *cookie);
extern status_t create_file_send(nspace *vol, vnode *dir, const char *name, int omode, int perms, uint32 *cur_time);
extern status_t create_file_recv(nspace *vol, vnode *dir, filecookie *cookie);


#endif // _CREATE_FILE_H_
