#include "port_pool.h"
#include "binder_node.h"
#include "debug_flags.h"
#include "binder_driver.h"
#include <drivers/KernelExport.h>
#include <lock.h>
#include <malloc.h>

// #pragma mark Globals

static lock        port_pool_lock;
static port_pool  *free_ports;
static port_pool  *active_ports;

// #pragma mark -

status_t 
init_port_pool()
{
	status_t err;
	
	err = new_lock(&port_pool_lock, "binder port_pool lock");
	if(err < B_OK)
		goto err1;
	free_ports = NULL;
	active_ports = NULL;
	err = B_OK;
err1:
	return err;
}

void 
uninit_port_pool()
{
	while(free_ports != NULL) {
		port_pool *p = free_ports;
		free_ports = p->next;
		delete_port(p->port);
		free(p);
	}
	while(active_ports != NULL) {
		port_pool *p = active_ports;
		active_ports = p->next;
		dprintf("binder -- uninit_port_pool: port %d still active\n",
		        (int)p->port);
		delete_port(p->port);
		free(p);
	}
	free_lock(&port_pool_lock);
}

// #pragma mark -

static team_id
current_team()
{
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	return ti.team;
}

/* call with inPortLock held */
static bool
is_active(port_pool *requested_pp)
{
	port_pool *pp;
	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp == requested_pp)
			return true;
	}
	return false;
}

bool 
is_port_valid(port_pool *pp)
{
	bool status;
	if(pp == NULL) {
		dprintf("binder(%d) -- is_port_valid: port == NULL\n",
		        (int)find_thread(NULL));
		return false;
	}
	LOCK(port_pool_lock);
	status = is_active(pp);
	UNLOCK(port_pool_lock);
	if(!status) {
		dprintf("binder(%d) -- is_port_valid: port not active\n",
		        (int)find_thread(NULL));
	}
	return status;
}

void
dump_port_state()
{
	port_pool *pp;
	LOCK(port_pool_lock);
	dprintf("binder: inactive ports:\n");
	for(pp = free_ports; pp != NULL; pp = pp->next) {
		dprintf("binder: port %d\n", (int)pp->port);
	}
	dprintf("binder: active ports:\n");
	for(pp = free_ports; pp != NULL; pp = pp->next) {
		dprintf("binder: port %d handled by %d requested by %d\n",
		        (int)pp->port, (int)pp->handled_by, (int)pp->client_team);
		if(pp->blocks != NULL) {
			if(is_active(pp->blocks)) {
				dprintf("binder:   blocks port %d handled by %d requested "
				        "by %d\n", (int)pp->blocks->port,
				        (int)pp->blocks->handled_by,
				        (int)pp->blocks->client_team);
			}
			else {
				dprintf("binder:   blocks invalid port\n");
			}
		}
		if(pp->blocked_on != NULL) {
			if(is_active(pp->blocked_on)) {
				dprintf("binder:   blocked on port %d handled by %d requested"
				        " by %d\n", (int)pp->blocked_on->port,
				        (int)pp->blocked_on->handled_by,
				        (int)pp->blocked_on->client_team);
			}
			else {
				dprintf("binder:   blocked on invalid port\n");
			}
		}
		
	}
	UNLOCK(port_pool_lock);
}

/* call with inPortLock held */
static status_t
allocate_port()
{
	if(free_ports == NULL) {
		port_id thePort;
		thePort = create_port(1,"registry inPort");
		if(thePort < 0)
			return thePort;

		free_ports = (port_pool *)malloc(sizeof(port_pool));
		if(free_ports == NULL) {
			delete_port(thePort);
			return B_NO_MEMORY;
		}

		free_ports->ownedBy = -1;
		free_ports->client_team = -1;
		free_ports->port = thePort;
		free_ports->next = NULL;
	}
	return B_OK;
}

static status_t
pp_set_owner(port_pool *pp, team_id owner)
{
	if(pp->ownedBy != owner) {
		status_t err;
		err = set_port_owner(pp->port, owner);
		if(err == B_BAD_PORT_ID) {
			pp->port = create_port(1,"registry inPort");
			err = set_port_owner(pp->port, owner);
		}
		if(err != B_OK)
			return err;
		pp->ownedBy = owner;
	}
	return B_OK;
}

status_t
obtain_port(binder_node *node, port_id *outPort, port_pool **replyPort)
{
	status_t err;
	port_pool *tp;
	port_pool *pp;
	port_pool *dependent_request = NULL;

	LOCK(port_pool_lock);

	err = allocate_port();
	if(err != B_OK)
		goto err1;

	for(tp = active_ports; tp != NULL; tp = tp->next) {
		if(tp->client_team == node->host) {
			port_pool *pp = tp;
			//dprintf("binder(%d) -- obtain_port: target busy, check path:\n",
			//        (int)find_thread(NULL));
			while(pp->blocked_on != NULL) {
				//dprintf("binder(%d) -- obtain_port: team %d waiting on port "
				//        "%d, thread %d\n",
				//        (int)find_thread(NULL), (int)pp->client_team,
				//        (int)pp->port, (int)pp->handled_by);
				if(!is_active(pp->blocked_on)) {
					dprintf("binder(%d) -- obtain_port: port %d is blocked "
					        "on port not in active list\n",
					        (int)find_thread(NULL), (int)tp->port);
					break;
				}
				pp = pp->blocked_on;
			}
			//dprintf("binder(%d) -- obtain_port: team %d waiting on port "
			//        "%d, thread %d\n",
			//        (int)find_thread(NULL), (int)pp->client_team,
			//        (int)pp->port, (int)pp->handled_by);
			if(pp->handled_by == find_thread(NULL)) {
				//dprintf("binder(%d) -- obtain_port: reflect punt\n",
				//        (int)find_thread(NULL));
				if(write_port_etc(tp->port, BINDER_PUNT_CODE, NULL, 0,
				                  B_TIMEOUT | B_CAN_INTERRUPT, 1000000)
				   != B_OK) {
					dprintf("binder(%d) -- obtain_port: reflect punt, "
					        "write port failed, port %d\n",
					        (int)find_thread(NULL), (int)tp->port);
				}
				*outPort = tp->port;
				break;
			}
			else {
				//dprintf("binder(%d) -- obtain_port: not us\n",
				//        (int)find_thread(NULL));
			}
			//dprintf("binder -- obtain_port: possible deadlock, "
			//        "%d <- %d, %d <- %d\n", (int)current_team(), (int)node->host,
			//        (int)tp->client_team, (int)tp->ownedBy);
			//goto err1;
		}
	}

	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp->handled_by == find_thread(NULL)) {
			if(dependent_request != NULL &&
			   dependent_request->blocked_on == NULL) {
				if(pp->blocked_on == NULL)
					dprintf("binder(%d) -- obtain_port: port %d and %d is "
					        "waiting on this thread\n", (int)find_thread(NULL),
					        (int)pp->port, (int)dependent_request->port);
			}
			else {
				dependent_request = pp;
			}
		}
	}

	{
		port_pool *pp = free_ports;
		err = pp_set_owner(pp, node->host);
		if (err != B_OK)
			goto err1;
#if TRACE_PORTS
		if(dependent_request != NULL)
			dprintf("binder(%d) -- obtain_port: got port %d, linked %d\n",
			        (int)find_thread(NULL), (int)pp->port,
			        (int)dependent_request->port);
		else
			dprintf("binder(%d) -- obtain_port: got port %d\n",
			        (int)find_thread(NULL), (int)pp->port);
#endif
		pp->client_team = current_team();
		pp->handled_by = -1;
		pp->blocked_on = NULL;
		pp->blocks = dependent_request;
		if(dependent_request != NULL) {
			if(dependent_request->blocked_on)
				dprintf("binder(%d) -- obtain_port: linked port %d was "
				        "already blocked\n", (int)find_thread(NULL),
				        (int)dependent_request->port);
			dependent_request->blocked_on = pp;
		}
	
		free_ports = pp->next;
		pp->next = active_ports;
		active_ports = pp;
		UNLOCK(port_pool_lock);
		*replyPort = pp;
	}
	return B_OK;

err1:
	UNLOCK(port_pool_lock);
	return err;
}

status_t
register_reply_thread(port_id port)
{
	status_t    err;
	port_pool  *pp;

	err = B_ERROR;
	LOCK(port_pool_lock);
	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp->port == port) {
#if TRACE_PORTS
			dprintf("binder(%d) -- register_reply_thread: port %d\n",
			        (int)find_thread(NULL), (int)port);
#endif
			pp->handled_by = find_thread(NULL);
			err = B_OK;
			break;
		}
	}
	UNLOCK(port_pool_lock);
	if(err != B_OK)
		dprintf("binder(%d) -- register_reply_thread: port %d not found\n",
		        (int)find_thread(NULL), (int)port);
	return err;
}

status_t
unregister_reply_thread(port_id port, bool unlink_blocks)
{
	status_t    err;
	port_pool  *pp;
	
	err = B_ERROR;
	LOCK(port_pool_lock);
	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp->port == port) {
#if TRACE_PORTS
			dprintf("binder(%d) -- unregister_reply_thread: port %d\n",
			        (int)find_thread(NULL), (int)port);
#endif
			if(pp->handled_by == find_thread(NULL)) {
				pp->handled_by = -1;
				if(unlink_blocks && pp->blocks != NULL) {
					if(!is_active(pp->blocks)) {
						dprintf("binder(%d) -- unregister_reply_thread: port %d, blocks port not in "
						        "active list\n", (int)find_thread(NULL), (int)pp->port);
						err = B_ERROR;
					}
					else {
						pp->blocks->blocked_on = NULL;
						pp->blocks = NULL;
					}
				}
				err = B_OK;
				break;
			}
			else {
				dprintf("binder(%d) -- unregister_reply_thread: ERROR port %d "
				        "was handled by %d\n",
				        (int)find_thread(NULL), (int)port, (int)pp->handled_by);
			}
		}
	}
	UNLOCK(port_pool_lock);
	if(err != B_OK)
		dprintf("binder(%d) -- unregister_reply_thread: port %d not found\n",
		        (int)find_thread(NULL), (int)port);
	return err;
}

status_t
optain_sync_reply_port(port_id reply_port)
{
	status_t    err = B_ERROR;
	port_pool  *pp;
	port_id     sync_port = -1;
	
	LOCK(port_pool_lock);
	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp->port == reply_port) {
			if(pp->blocked_on != NULL) {
				dprintf("binder(%d) -- optain_sync_reply_port: port %d "
				        "is already blocked\n",
				        (int)find_thread(NULL), (int)reply_port);
				err = B_ERROR;
				goto err;
			}
			if(pp->handled_by != find_thread(NULL)) {
				dprintf("binder(%d) -- optain_sync_reply_port: ERROR port %d "
				        "was handled by thread %d\n",
				        (int)find_thread(NULL), (int)reply_port, (int)pp->handled_by);
				err = B_ERROR;
				goto err;
			}
			err = allocate_port();
			if(err != B_OK)
				goto err;

			{
				port_pool *npp = free_ports;
				pp_set_owner(npp, pp->client_team);
				if (err != B_OK)
					goto err;
				pp->handled_by = -1;
				npp->client_team = current_team();
				npp->handled_by = -1;
				npp->blocked_on = NULL;
				npp->blocks = NULL;
				free_ports = npp->next;
				npp->next = active_ports;
				active_ports = npp;
				sync_port = npp->port;
				err = B_OK;
			}
			break;
		}
	}
err:
	UNLOCK(port_pool_lock);
	if(err != B_OK) {
		dprintf("binder(%d) -- optain_sync_reply_port: port %d not found\n",
		        (int)find_thread(NULL), (int)reply_port);
		return err;
	}
#if TRACE_PORTS
	dprintf("binder(%d) -- optain_sync_reply_port: got port %d\n",
	        (int)find_thread(NULL), (int)sync_port);
#endif
	return sync_port;
}

/* call with inPortLock held */
static status_t
unlink_port(port_pool *pp)
{
	status_t err = B_OK;

	if(pp == NULL) {
		dprintf("binder(%d) -- unlink_port: port == NULL\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	if(active_ports == pp)
		active_ports = pp->next;
	else {
		port_pool *prev = active_ports;
		while(prev != NULL && prev->next != pp)
			prev = prev->next;
		if(prev != NULL)
			prev->next = pp->next;
		else {
			dprintf("binder(%d) -- unlink_port: port %p not in active list\n",
			        (int)find_thread(NULL), pp);
			return B_ERROR;
		}
	}
	if(pp->blocks != NULL) {
		if(!is_active(pp->blocks)) {
			dprintf("binder(%d) -- unlink_port: port %d, blocks port not in "
			        "active list\n", (int)find_thread(NULL), (int)pp->port);
			err = B_ERROR;
		}
		else
			pp->blocks->blocked_on = NULL;
	}
	if(pp->blocked_on) {
		if(!is_active(pp->blocked_on)) {
			dprintf("binder(%d) -- unlink_port: port %d, blocked on port not "
			        "in active list\n", (int)find_thread(NULL), (int)pp->port);
			err = B_ERROR;
		}
		else {
			pp->blocked_on->blocks = NULL;
			dprintf("binder(%d) -- unlink_port: port %d, was blocked on "
			        "port %d\n", (int)find_thread(NULL), (int)pp->port,
			        (int)pp->blocked_on->port);
			err = B_ERROR;
		}
	}
	return err;
}

void
pp_delete_port(port_pool *pp)
{
	LOCK(port_pool_lock);
	if(unlink_port(pp) < B_OK) {
		dprintf("binder(%d) -- pp_delete_port: port %p failed\n",
		        (int)find_thread(NULL), pp);
	}
	else {
		dprintf("binder(%d) -- pp_delete_port: port %d\n",
		        (int)find_thread(NULL), (int)pp->port);
		delete_port(pp->port);
		free(pp);
	}
	UNLOCK(port_pool_lock);
}

status_t
release_port_by_id(port_id port, thread_id thread)
{
	status_t    err = B_ERROR;
	port_pool  *pp;

#if TRACE_PORTS
	dprintf("binder(%d) -- release_port: port %d (sync port)\n",
	        (int)find_thread(NULL), (int)port);
#endif
	LOCK(port_pool_lock);
	for(pp = active_ports; pp != NULL; pp = pp->next) {
		if(pp->port == port) {

//			if(pp->handled_by != thread) {
//				dprintf("binder(%d) -- release_port: ERROR port %d was "
//				        "handled by %d\n", (int)find_thread(NULL),
//				        (int)pp->port, (int)pp->handled_by);
//			}
//			else {
				if(unlink_port(pp) == B_OK) {
					pp->next = free_ports;
					free_ports = pp;
				}
				else {
					delete_port(pp->port);
					free(pp);
				}
				err = B_OK;
				break;
//			}
		}
	}
	UNLOCK(port_pool_lock);
	return err;
}

void release_port(port_pool *pp)
{
	LOCK(port_pool_lock);
	if(unlink_port(pp) == B_OK) {
#if TRACE_PORTS
		dprintf("binder(%d) -- release_port: port %d\n",
		        (int)find_thread(NULL), (int)pp->port);
#endif
		pp->next = free_ports;
		free_ports = pp;
	}
	else {
		dprintf("binder(%d) -- release_port: port %p failed\n",
		        (int)find_thread(NULL), pp);
	}
	UNLOCK(port_pool_lock);
}
