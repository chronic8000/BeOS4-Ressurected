#ifndef _RENAME_H__
#define _RENAME_H__

extern status_t get_attributes(nspace *vol, vnode *dir, const char *name, ushort *_attr);
extern status_t do_rename(nspace *vol, vnode *odir, const char *oldname, vnode *ndir, const char *newname, uint32 rename_flags);


#endif // _RENAME_H__
