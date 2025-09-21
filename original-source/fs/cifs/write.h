#ifndef _WRITE_H
#define _WRITE_H

extern status_t write_to_file(nspace *vol, vnode *node, off_t pos, const void *buf, size_t *len, filecookie *cookie);

#endif // _WRITE_H
