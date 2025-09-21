#ifndef _GENERIC_WRAPPER_H
#define _GENERIC_WRAPPER_H

#include <OS.h>
status_t generic_open(const char *name, uint32 flags, void **vdata);
status_t generic_close(void *vdata);
status_t generic_free(void *vdata);
status_t generic_control(void *vdata, uint32 msg, void *buf, size_t len);
status_t generic_read(void *vdata, off_t pos, void *buf, size_t *lenp);
status_t generic_write(void *vdata, off_t pos, const void *buf,size_t *lenp);


#endif /* _GENERIC_WRAPPER_H */
