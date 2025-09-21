#ifndef _CIFSFS_ATTR_H_
#define _CIFSFS_ATTR_H_

status_t set_mime_type(vnode *node, const char *filename);

int cifs_open_attrdir(void *_vol, void *_node, void **_cookie);
int cifs_close_attrdir(void *_vol, void *_node, void *_cookie);
int cifs_free_attrcookie(void *_vol, void *_node, void *_cookie);
int cifs_rewind_attrdir(void *_vol, void *_node, void *_cookie);
int cifs_read_attrdir(void *_vol, void *_node, void *_cookie, long *num, 
	struct dirent *buf, size_t bufsize);
int cifs_stat_attr(void *_vol, void *_node, const char *name, struct attr_info *buf);
int cifs_read_attr(void *_vol, void *_node, const char *name, int type, void *buf, 
	size_t *len, off_t pos);
int cifs_write_attr(void *_vol, void *_node, const char *name, int type,
	const void *buf, size_t *len, off_t pos);

#endif
