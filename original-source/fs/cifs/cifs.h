#ifndef _CIFS_H
#define _CIFS_H

#include "cifs_globals.h"

extern void strtoupper(char *string);
extern void get_netbios_name(char *in_name, char *new_name, int server);
extern int dump_vector(int sockid, struct iovec *iov, int iovcnt);
extern int init_netbios_connection(nspace *vol);
extern status_t kill_netbios_connection(nspace *vol);
extern void put_nb_length(nb_header_t *header, int len);
extern int nt_open_file(nspace *vol, vnode *node, int omode, filecookie *cookie);
extern int nt_send_open_file(nspace *vol, vnode *node, int omode);
extern int nt_recv_open_file(nspace *vol, vnode *node, filecookie *cookie);
extern int close_file(nspace *vol, vnode *node, filecookie *cookie);
extern status_t tree_disconnect(nspace *vol);
extern status_t new_open_file(nspace *vol, vnode *node, int omode, filecookie *cookie);
extern status_t do_delete(nspace *vol, const char *fullname, int is_dir);
extern status_t get_full_name(nspace *vol, vnode *dir, const char *name, char **val);
extern status_t new_filecookie(filecookie **fc);
extern void del_filecookie(filecookie *fc);


#endif // _CIFS_H

