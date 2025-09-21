
int bfs_open_query(void *ns, const char *query, ulong flags,
				   port_id port, long token, void **cookie);
int bfs_read_query(void *ns, void *cookie, long *num,
				   struct dirent *buf, size_t bufsize);
int bfs_close_query(void *ns, void *cookie);
int bfs_free_query_cookie(void *ns, void *node, void *cookie);
