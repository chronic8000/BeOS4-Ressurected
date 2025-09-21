#ifndef _READ_H
#define _READ_H



extern status_t read_from_file(nspace *vol, vnode *node, off_t pos,	void *buf, size_t *len, filecookie *cookie);



#endif // _READ_H
