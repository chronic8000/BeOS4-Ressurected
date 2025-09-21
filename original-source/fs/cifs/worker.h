#ifndef _WORKER_H
#define _WORKER_H

extern status_t worker_func(void *_vol);
extern status_t start_worker(nspace *vol);
extern status_t ping_server(nspace *vol);
extern status_t send_ping(nspace *vol, char *data);
extern status_t recv_ping(nspace *vol, char *data);


#endif // _WORKER_H
