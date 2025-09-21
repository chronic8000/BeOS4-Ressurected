#ifndef PORT_POOL_H
#define PORT_POOL_H

#include <kernel/OS.h>

#include "binder_node.h"

typedef struct port_pool {
	team_id              ownedBy;
	thread_id            handled_by;
	team_id              client_team;
	port_id              port;
	struct port_pool    *next;
	struct port_pool    *blocked_on;
	struct port_pool    *blocks;
} port_pool;

status_t init_port_pool();
void uninit_port_pool();

status_t obtain_port(binder_node *node, port_id *outPort, port_pool **replyPort);
status_t register_reply_thread(port_id port);
status_t unregister_reply_thread(port_id port, bool unlink_blocks);
status_t optain_sync_reply_port(port_id reply_port);
status_t release_port_by_id(port_id port, thread_id thread);
void release_port(port_pool *pp);
void pp_delete_port(port_pool *pp);
bool is_port_valid(port_pool *pp); /* debug only */

void dump_port_state();

#endif

