#include "port_pool.h"
#include "binder_node.h"
#include "debug_flags.h"
#include <drivers/KernelExport.h>

/******************************************************************************
* Port pool functions:
*/

static team_id
current_team()
{
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	return ti.team;
}

/* call with inPortLock held */
static bool
is_active(nspace *ns, port_pool *requested_pp)
{
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp == requested_pp)
			return true;
	}
	return false;
}

bool 
is_port_valid(nspace *ns, port_pool *pp)
{
	bool status;
	if(pp == NULL) {
		dprintf("binderfs(%d) -- is_port_valid: port == NULL\n",
		        (int)find_thread(NULL));
		return false;
	}
	LOCK(ns->inPortLock);
	status = is_active(ns, pp);
	UNLOCK(ns->inPortLock);
	if(!status) {
		dprintf("binderfs(%d) -- is_port_valid: port not active\n",
		        (int)find_thread(NULL));
	}
	return status;
}

void
dump_port_state(nspace *ns)
{
	LOCK(ns->inPortLock);
	dprintf("binderfs: inactive ports:\n");
	for(port_pool *pp = ns->inPortFree; pp != NULL; pp = pp->next) {
		dprintf("binderfs: port %d\n", (int)pp->port);
	}
	dprintf("binderfs: active ports:\n");
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		dprintf("binderfs: port %d handled by %d requested by %d\n",
		        (int)pp->port, (int)pp->handled_by, (int)pp->client_team);
		if(pp->blocks != NULL)
			if(is_active(ns, pp->blocks)) {
				dprintf("binderfs:   blocks port %d handled by %d requested "
				        "by %d\n", (int)pp->blocks->port,
				        (int)pp->blocks->handled_by,
				        (int)pp->blocks->client_team);
			}
			else {
				dprintf("binderfs:   blocks invalid port\n");
			}
		if(pp->blocked_on != NULL)
			if(is_active(ns, pp->blocked_on)) {
				dprintf("binderfs:   blocked on port %d handled by %d requested"
				        " by %d\n", (int)pp->blocked_on->port,
				        (int)pp->blocked_on->handled_by,
				        (int)pp->blocked_on->client_team);
			}
			else {
				dprintf("binderfs:   blocked on invalid port\n");
			}
		
	}
	UNLOCK(ns->inPortLock);
}

/* call with inPortLock held */
static status_t
allocate_port(nspace *ns)
{
	if (!ns->inPortFree) {
		port_id thePort;
		thePort = create_port(1,"registry inPort");
		if(thePort < 0)
			return thePort;

		ns->inPortFree = (port_pool*)malloc(sizeof(port_pool));
		if(ns->inPortFree == NULL) {
			delete_port(thePort);
			return B_NO_MEMORY;
		}

		ns->inPortFree->ownedBy = -1;
		ns->inPortFree->client_team = -1;
		ns->inPortFree->port = thePort;
		ns->inPortFree->next = NULL;
	}
	return B_OK;
}

static status_t
set_port_owner(port_pool *pp, team_id owner)
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
obtain_port(vnode *vn, port_id *outPort, port_pool **replyPort)
{
	status_t err;
	LOCK(vn->ns->inPortLock);
	port_pool *dependent_request = NULL;

	err = allocate_port(vn->ns);
	if(err != B_OK)
		goto err1;

	for(port_pool *tp = vn->ns->inPortActive; tp != NULL; tp = tp->next) {
		if(tp->client_team == vn->host) {
			port_pool *pp = tp;
			//dprintf("binderfs(%d) -- obtain_port: target busy, check path:\n",
			//        (int)find_thread(NULL));
			while(pp->blocked_on != NULL) {
				//dprintf("binderfs(%d) -- obtain_port: team %d waiting on port "
				//        "%d, thread %d\n",
				//        (int)find_thread(NULL), (int)pp->client_team,
				//        (int)pp->port, (int)pp->handled_by);
				if(!is_active(vn->ns, pp->blocked_on)) {
					dprintf("binderfs(%d) -- obtain_port: port %d is blocked "
					        "on port not in active list\n",
					        (int)find_thread(NULL), (int)tp->port);
					break;
				}
				pp = pp->blocked_on;
			}
			//dprintf("binderfs(%d) -- obtain_port: team %d waiting on port "
			//        "%d, thread %d\n",
			//        (int)find_thread(NULL), (int)pp->client_team,
			//        (int)pp->port, (int)pp->handled_by);
			if(pp->handled_by == find_thread(NULL)) {
				//dprintf("binderfs(%d) -- obtain_port: reflect punt\n",
				//        (int)find_thread(NULL));
				if(write_port_etc(tp->port, BINDER_PUNT_CODE, NULL, 0,
				                  B_TIMEOUT | B_CAN_INTERRUPT, 1000000)
				   != B_OK) {
					dprintf("binderfs(%d) -- obtain_port: reflect punt, "
					        "write port failed, port %d\n",
					        (int)find_thread(NULL), (int)tp->port);
				}
				*outPort = tp->port;
				break;
			}
			else {
				//dprintf("binderfs(%d) -- obtain_port: not us\n",
				//        (int)find_thread(NULL));
			}
			//dprintf("binderfs -- obtain_port: possible deadlock, "
			//        "%d <- %d, %d <- %d\n", (int)current_team(), (int)vn->host,
			//        (int)tp->client_team, (int)tp->ownedBy);
			//goto err1;
		}
	}

	for(port_pool *pp = vn->ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp->handled_by == find_thread(NULL)) {
			if(dependent_request != NULL &&
			   dependent_request->blocked_on == NULL) {
				if(pp->blocked_on == NULL)
					dprintf("binderfs(%d) -- obtain_port: port %d and %d is "
					        "waiting on this thread\n", (int)find_thread(NULL),
					        (int)pp->port, (int)dependent_request->port);
			}
			else {
				dependent_request = pp;
			}
		}
	}

	{
		port_pool *pp = vn->ns->inPortFree;
		err = set_port_owner(pp, vn->host);
		if (err != B_OK)
			goto err1;
#if TRACE_PORTS
		if(dependent_request != NULL)
			dprintf("binderfs(%d) -- obtain_port: got port %d, linked %d\n",
			        (int)find_thread(NULL), (int)pp->port,
			        (int)dependent_request->port);
		else
			dprintf("binderfs(%d) -- obtain_port: got port %d\n",
			        (int)find_thread(NULL), (int)pp->port);
#endif
		pp->client_team = current_team();
		pp->handled_by = -1;
		pp->blocked_on = NULL;
		pp->blocks = dependent_request;
		if(dependent_request != NULL) {
			if(dependent_request->blocked_on)
				dprintf("binderfs(%d) -- obtain_port: linked port %d was "
				        "already blocked\n", (int)find_thread(NULL),
				        (int)dependent_request->port);
			dependent_request->blocked_on = pp;
		}
	
		vn->ns->inPortFree = pp->next;
		pp->next = vn->ns->inPortActive;
		vn->ns->inPortActive = pp;
		UNLOCK(vn->ns->inPortLock);
		*replyPort = pp;
	}
	return B_OK;

err1:
	UNLOCK(vn->ns->inPortLock);
	return err;
};

status_t
register_reply_thread(nspace *ns, port_id port)
{
	status_t err = B_ERROR;
	LOCK(ns->inPortLock);
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp->port == port) {
#if TRACE_PORTS
			dprintf("binderfs(%d) -- register_reply_thread: port %d\n",
			        (int)find_thread(NULL), (int)port);
#endif
			pp->handled_by = find_thread(NULL);
			err = B_OK;
			break;
		}
	}
	UNLOCK(ns->inPortLock);
	if(err != B_OK)
		dprintf("binderfs(%d) -- register_reply_thread: port %d not found\n",
		        (int)find_thread(NULL), (int)port);
	return err;
}

status_t
unregister_reply_thread(nspace *ns, port_id port, bool unlink_blocks)
{
	status_t err = B_ERROR;
	LOCK(ns->inPortLock);
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp->port == port) {
#if TRACE_PORTS
			dprintf("binderfs(%d) -- unregister_reply_thread: port %d\n",
			        (int)find_thread(NULL), (int)port);
#endif
			if(pp->handled_by == find_thread(NULL)) {
				pp->handled_by = -1;
				if(unlink_blocks && pp->blocks != NULL) {
					if(!is_active(ns, pp->blocks)) {
						dprintf("binderfs(%d) -- unregister_reply_thread: port %d, blocks port not in "
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
				dprintf("binderfs(%d) -- unregister_reply_thread: ERROR port %d "
				        "was handled by %d\n",
				        (int)find_thread(NULL), (int)port, (int)pp->handled_by);
			}
		}
	}
	UNLOCK(ns->inPortLock);
	if(err != B_OK)
		dprintf("binderfs(%d) -- unregister_reply_thread: port %d not found\n",
		        (int)find_thread(NULL), (int)port);
	return err;
}

status_t
optain_sync_reply_port(nspace *ns, port_id reply_port)
{
	status_t err = B_ERROR;
	port_id sync_port = -1;
	
	LOCK(ns->inPortLock);
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp->port == reply_port) {
			if(pp->blocked_on != NULL) {
				dprintf("binderfs(%d) -- optain_sync_reply_port: port %d "
				        "is already blocked\n",
				        (int)find_thread(NULL), (int)reply_port);
				err = B_ERROR;
				goto err;
			}
			if(pp->handled_by != find_thread(NULL)) {
				dprintf("binderfs(%d) -- optain_sync_reply_port: ERROR port %d "
				        "was handled by thread %d\n",
				        (int)find_thread(NULL), (int)reply_port, (int)pp->handled_by);
				err = B_ERROR;
				goto err;
			}
			err = allocate_port(ns);
			if(err != B_OK)
				goto err;

			port_pool *npp = ns->inPortFree;
			set_port_owner(npp, pp->client_team);
			if (err != B_OK)
				goto err;
			pp->handled_by = -1;
			npp->client_team = current_team();
			npp->handled_by = -1;
			npp->blocked_on = NULL;
			npp->blocks = NULL;
			ns->inPortFree = npp->next;
			npp->next = ns->inPortActive;
			ns->inPortActive = npp;
			sync_port = npp->port;
			err = B_OK;
			break;
		}
	}
err:
	UNLOCK(ns->inPortLock);
	if(err != B_OK) {
		dprintf("binderfs(%d) -- optain_sync_reply_port: port %d not found\n",
		        (int)find_thread(NULL), (int)reply_port);
		return err;
	}
#if TRACE_PORTS
	dprintf("binderfs(%d) -- optain_sync_reply_port: got port %d\n",
	        (int)find_thread(NULL), (int)sync_port);
#endif
	return sync_port;
}

/* call with inPortLock held */
static status_t
unlink_port(nspace *ns, port_pool *pp)
{
	status_t err = B_OK;
	if(pp == NULL) {
		dprintf("binderfs(%d) -- unlink_port: port == NULL\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	if(ns->inPortActive == pp)
		ns->inPortActive = pp->next;
	else {
		port_pool *prev = ns->inPortActive;
		while(prev != NULL && prev->next != pp)
			prev = prev->next;
		if(prev != NULL)
			prev->next = pp->next;
		else {
			dprintf("binderfs(%d) -- unlink_port: port %p not in active list\n",
			        (int)find_thread(NULL), pp);
			return B_ERROR;
		}
	}
	if(pp->blocks != NULL) {
		if(!is_active(ns, pp->blocks)) {
			dprintf("binderfs(%d) -- unlink_port: port %d, blocks port not in "
			        "active list\n", (int)find_thread(NULL), (int)pp->port);
			err = B_ERROR;
		}
		else
			pp->blocks->blocked_on = NULL;
	}
	if(pp->blocked_on) {
		if(!is_active(ns, pp->blocked_on)) {
			dprintf("binderfs(%d) -- unlink_port: port %d, blocked on port not "
			        "in active list\n", (int)find_thread(NULL), (int)pp->port);
			err = B_ERROR;
		}
		else {
			pp->blocked_on->blocks = NULL;
			dprintf("binderfs(%d) -- unlink_port: port %d, was blocked on "
			        "port %d\n", (int)find_thread(NULL), (int)pp->port,
			        (int)pp->blocked_on->port);
			err = B_ERROR;
		}
	}
	return err;
}

void
delete_port(nspace *ns, port_pool *pp)
{
	LOCK(ns->inPortLock);
	if(unlink_port(ns, pp) < B_OK) {
		dprintf("binderfs(%d) -- delete_port: port %p failed\n",
		        (int)find_thread(NULL), pp);
	}
	else {
		dprintf("binderfs(%d) -- delete_port: port %d\n",
		        (int)find_thread(NULL), (int)pp->port);
		delete_port(pp->port);
		free(pp);
	}
	UNLOCK(ns->inPortLock);
};

status_t
release_port(nspace *ns, port_id port, thread_id thread)
{
#if TRACE_PORTS
	dprintf("binderfs(%d) -- release_port: port %d (sync port)\n",
	        (int)find_thread(NULL), (int)port);
#endif
	status_t err = B_ERROR;
	LOCK(ns->inPortLock);
	for(port_pool *pp = ns->inPortActive; pp != NULL; pp = pp->next) {
		if(pp->port == port) {

//			if(pp->handled_by != thread) {
//				dprintf("binderfs(%d) -- release_port: ERROR port %d was "
//				        "handled by %d\n", (int)find_thread(NULL),
//				        (int)pp->port, (int)pp->handled_by);
//			}
//			else {
				if(unlink_port(ns, pp) == B_OK) {
					pp->next = ns->inPortFree;
					ns->inPortFree = pp;
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
	UNLOCK(ns->inPortLock);
	return err;
}

void release_port(vnode *vn, port_pool *pp)
{
	LOCK(vn->ns->inPortLock);
	if(unlink_port(vn->ns, pp) == B_OK) {
#if TRACE_PORTS
		dprintf("binderfs(%d) -- release_port: port %d\n",
		        (int)find_thread(NULL), (int)pp->port);
#endif
		pp->next = vn->ns->inPortFree;
		vn->ns->inPortFree = pp;
	}
	else {
		dprintf("binderfs(%d) -- release_port: port %p failed\n",
		        (int)find_thread(NULL), pp);
	}
	UNLOCK(vn->ns->inPortLock);
};

void delete_ports(nspace *ns)
{
	LOCK(ns->inPortLock);
	while (ns->inPortFree) {
		port_pool *p = ns->inPortFree;
		delete_port(p->port);
		ns->inPortFree = ns->inPortFree->next;
		free(p);
	};
	while (ns->inPortActive) {
		port_pool *p = ns->inPortActive;
		dprintf("binderfs -- delete_ports: port %d still active\n",
		        (int)p->port);
		delete_port(p->port);
		ns->inPortActive = ns->inPortActive->next;
		free(p);
	};
	UNLOCK(ns->inPortLock);
	delete_sem(ns->inPortLock.s);
};

