status_t dev_dgl_open		(const char *name, uint32 flags, void **cookie);
status_t dev_dgl_close		(void *cookie);
status_t dev_dgl_free		(void *cookie);
status_t dev_dgl_control	(void *cookie, uint32 op, void *data, size_t len);
status_t dev_dgl_read		(void *cookie, off_t position, void *data, size_t *len);
status_t dev_dgl_write		(void *cookie, off_t position, const void *data, size_t *len);
