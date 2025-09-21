
#ifndef _CDDA_ATTR_H
#define _CDDA_ATTR_H

#include "fsproto.h"

int	fs_open_attrdir(void *ns, void *node, void **cookie);
int	fs_close_attrdir(void *ns, void *node, void *cookie);
int	fs_rewind_attrdir(void *ns, void *node, void *cookie);
int	fs_read_attrdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
int fs_free_attrcookie(void *ns, void *node, void *cookie);
int	fs_stat_attr(void *ns, void *node, const char *name,
					struct attr_info *buf);

int	fs_read_attr(void *ns, void *node, const char *name, int type,
					void *buf, size_t *len, off_t pos);

int	fs_write_attr(void *ns, void *node, const char *name, int type,	
					const void *buf, size_t *len, off_t pos);

char *mangledname(void *ns,void *node,const char *name);


typedef struct attrcookie
{
	ulong		counter;
} attrcookie;

#endif
