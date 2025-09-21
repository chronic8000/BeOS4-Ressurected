#ifndef PORT_POOL_H
#define PORT_POOL_H

#include <kernel/OS.h>

struct nspace;
struct vnode;

struct port_pool
{
	team_id				ownedBy;
	thread_id			handled_by;
	team_id				client_team;
	port_id				port;
	port_pool *			next;
	port_pool *			blocked_on;
	port_pool *			blocks;
};

status_t obtain_port(vnode *vn, port_id *outPort, port_pool **replyPort);
status_t register_reply_thread(nspace *ns, port_id port);
status_t unregister_reply_thread(nspace *ns, port_id port, bool unlink_blocks = false);
status_t optain_sync_reply_port(nspace *ns, port_id reply_port);
status_t release_port(nspace *ns, port_id port, thread_id thread);
void release_port(vnode *vn, port_pool *pp);
void delete_port(nspace *ns, port_pool *pp);
void delete_ports(nspace *ns);
bool is_port_valid(nspace *ns, port_pool *pp); /* debug only */

void dump_port_state(nspace *ns);

#endif

