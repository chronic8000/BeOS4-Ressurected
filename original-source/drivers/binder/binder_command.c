#include "binder_command.h"
#include "binder_request.h"
#include "binder_node.h"
#include "transaction_state.h"
#include "binder_connection.h"
#include "port_pool.h"
#include "binder_driver.h"
#include "debug_flags.h"
#include <support/SupportDefs.h>
#include <drivers/KernelExport.h>
#include <lock.h>
#include <malloc.h>
#include <string.h>
//#include <stdio.h>

#define	B_SOMETHING_CHANGED		0x00000001
#define B_NAME_KNOWN			0x80000000

// #pragma mark Globals

static lock              link_lock;
static binder_node_id    root_node_id;
static sem_id            root_node_set_sem;

#if TRACE_NODES
static int32 global_node_count;
#endif
#if TRACE_REFS
static int32 global_ref_count;
#endif

// #pragma mark Prototypes

static status_t connect(binder_connection *c, binder_connect_cmd *connect_cmd,
                        reflection_struct *reflection, state_connect *state,
                        transaction_resources *resources);

static status_t add_ref_node(binder_node *vn);
static status_t unref_node(binder_node *vn);
static status_t one_notify_callback(binder_node_id, notify_callback_struct *,
                                    state_one_notify *, reflection_struct *,
                                    transaction_resources *);
static status_t notify_callback(binder_node *vn, notify_callback_struct *ncs,
                                state_notify *state, reflection_struct *reflection,
                                transaction_resources *resources, bool lockEm/*=true*/);

// #pragma mark Types

typedef struct {
	reflection_struct      *reflection;
	state_connect          *state;
	transaction_resources  *resources;
} internal_connect_struct;

typedef struct node_link {
	binder_node            *link;
	uint8                   fromFlags;
	uint8                   toFlags;
} node_link;

typedef enum {
	undefined_type = 0,
	string,
	number,
	object,
	remote_object
} registry_property_type;

typedef struct {
	binder_node           *node;
	void                  *userCookie;
	bool                   opened;
	char                  *nextName;
	int32                  nextNameLen;
	int32                  err;
} single_cookie;

typedef struct {
	int32				count;
	team_id				client;
//	int					fd;
	bool				opened;
	single_cookie		cookies[1];
} multi_cookie;

//status_t	AllocateName(int32 size);

// #pragma mark -

status_t
init_binder_command()
{
	status_t err;
	root_node_id.port = -1;
	err = new_lock(&link_lock, "binder link lock");
	if(err < B_OK)
		goto err0;
	err = root_node_set_sem = create_sem(0, "binder root node set");
	if(err < B_OK)
		goto err1;
	err = init_binder_node();
	if(err < B_OK)
		goto err2;
	err = init_port_pool();
	if(err < B_OK)
		goto err3;
#if TRACE_NODES
	global_node_count = 0;
#endif
#if TRACE_REFS
	global_ref_count = 0;
#endif
	return B_OK;

//err4:
//	uninit_port_pool();
err3:
	uninit_binder_node();
err2:
	delete_sem(root_node_set_sem);
err1:
	free_lock(&link_lock);
err0:
	return err;
}

void
uninit_binder_command()
{
	uninit_port_pool();
	uninit_binder_node();
	free_lock(&link_lock);
	if(root_node_set_sem >= 0)
		delete_sem(root_node_set_sem);
}

// #pragma mark -

status_t
check_state(const char *function_name, bool entry, int stage,
            int first_valid_stage, int last_valid_stage, void *state,
            size_t state_size, struct transaction_resources *resources,
            struct reflection_struct *reflection, status_t function_err)
{
	resources = resources;
#if TRACE_STATE
	if(entry)
		dprintf("binder(%d) -- %s: entry stage %d%s\n",
		        (int)find_thread(NULL), function_name,
		        stage, reflection->active ? " WARNING reflection active" : "");
	else 
		dprintf("binder(%d) -- %s: exit stage %d return %s\n",
		        (int)find_thread(NULL), function_name, stage,
		        reflection->active ? "-Reflection-" :
		        strerror(function_err));
#endif
	if(entry) {
		if(reflection->active) {
			dprintf("binder(%d) -- %s: entry state has active reflection\n",
			        (int)find_thread(NULL), function_name);
			return B_ERROR;
		}
		if(stage >= first_valid_stage && stage < last_valid_stage)
			return B_OK;
	}
	else {
		if(reflection->active) {
			if(function_err != B_OK) {
				dprintf("binder(%d) -- %s: exit state has active reflection, "
				        "with invalid return value, %s\n",
				        (int)find_thread(NULL), function_name,
				        strerror(function_err));
				return B_ERROR;
			}
			if(stage >= first_valid_stage && stage < last_valid_stage)
				return B_OK;
			else {
				dprintf("binder(%d) -- %s: exit state has active reflection, "
				        "with invalid stage, %d\n",
				        (int)find_thread(NULL), function_name, stage);
				return B_ERROR;
			}
		}
		else if(stage != STAGE_DONE) {
			dprintf("binder(%d) -- %s: exit state with no reflection, "
			        "has invalid stage, %d\n",
			        (int)find_thread(NULL), function_name, stage);
			return B_ERROR;
		}
	}
#if 0
	{
		uint8 *sp = (uint8*)resources;
		uint8 *ep = sp + sizeof(*resources);
		while(sp < ep) {
			if(*sp++ != 0) {
				dprintf("binder(%d) -- %s: %s state, stage 0, "
				        "resources is non zero\n",
				        (int)find_thread(NULL), function_name,
				        entry ? "entry" : "exit");
				return B_ERROR;
			}
		}
	}
#endif
	{
		uint8 *sp = state;
		uint8 *ep = sp + state_size;
		while(sp < ep) {
			if(*sp++ != 0) {
				dprintf("binder(%d) -- %s: %s state, stage 0, is non zero\n",
				        (int)find_thread(NULL), function_name,
				        entry ? "entry" : "exit");
				return B_ERROR;
			}
		}
	}
	return B_OK;
}

// #pragma mark -

static team_id
current_team()
{
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	return ti.team;
}

static bool 
is_local(port_id port)
{
	port_info pi;
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	get_port_info(port,&pi);
	return (ti.team == pi.team);
}

// #pragma mark -

void
free_node_list(node_list *list)
{
	int i;
	if(list == NULL)
		return;
	for(i = 0; i < list->count; i++) {
		unref_node(list->node[i]);
	}
	free(list);
}

typedef struct observer_iterrator {
	binder_node_observer *end;
	binder_node_observer *next;
	binder_node_observer *circle_guard;
	status_t status;
} observer_iterrator;

static void
init_observer_iterrator(observer_iterrator *iterator, binder_node *node)
{
	iterator->end = &node->observers;
	iterator->circle_guard = iterator->next = node->observers.next;
	iterator->status = B_OK;
}

static bool
next_observer(observer_iterrator *iterator, binder_node_observer **next_ptr)
{
	if(iterator->next == iterator->end) {
		*next_ptr = NULL;
		return false;
	}
	*next_ptr = iterator->next;

	if(iterator->circle_guard != iterator->end)
		iterator->circle_guard = iterator->circle_guard->next;
	if(iterator->circle_guard != iterator->end)
		iterator->circle_guard = iterator->circle_guard->next;
	if(iterator->circle_guard == iterator->next) {
		dprintf("binder(%d) -- next_observer: bad observer list, "
		        "circle detected\n", (int)find_thread(NULL));
		iterator->status = B_ERROR;
		return false;
	}
	iterator->next = iterator->next->next;
	return true;
}

static int
build_node_list(binder_node *start_node, binder_node *end,
                uint8 linkFromFlags, uint8 linkToFlags)
{
	binder_node *curr = start_node;
	int list_count;
	start_node->tmp_ptr = end;
	list_count = 1;
	while(curr != end) {
		int i;
		if(curr == NULL) {
			dprintf("binder -- build_node_list: curr == NULL - 1\n");
			return -1;
		}
		for(i = 0; i < curr->link_count; i++)
			if((curr->links[i].fromFlags & linkFromFlags) ||
			   (curr->links[i].toFlags & linkToFlags)) {
				binder_node *node = curr->links[i].link;
				if(node->tmp_ptr != NULL) {
					dprintf("binder -- build_node_list: node %p already "
					        "visited\n", node);
				}
				else {
					node->tmp_ptr = curr->tmp_ptr;
					curr->tmp_ptr = node;
					list_count++;
				}
			}
		curr = curr->tmp_ptr;
	}
	return list_count;
}

static void
cleanup_node_list(binder_node *start_node, binder_node *end)
{
	binder_node *next = start_node;
	while(next != end) {
		binder_node *node = next;
		if(node == NULL) {
			dprintf("binder -- cleanup_node_list: node == NULL - 3\n");
			return;
		}
		next = next->tmp_ptr;
		node->tmp_ptr = NULL;
	}
}

static observer_list *
get_observer_list(binder_node *start_node, bool lock_list, bool *node_has_links)
{
	binder_node end;
	binder_node *curr_head = start_node;
	int node_count;
	int observer_count = 0;
	observer_list *list = NULL;

	if(lock_list)
		LOCK(link_lock);

	node_count = build_node_list(start_node, &end, 0,
	                             linkFilter|linkInherit|linkAugment);
	if(node_count < 1)
		goto err;
	*node_has_links = node_count > 1;

	observer_count = 0;
	{
		int i;
		binder_node *curr;
		curr = start_node;
		for(i = 0; i < node_count; i++) {
			observer_iterrator iterator;
			binder_node_observer *observer;
			if(curr == NULL || curr == &end) {
				dprintf("binder -- get_observer_list: curr == NULL - 1\n");
				goto err;
			}
			init_observer_iterrator(&iterator, curr);
			while(next_observer(&iterator, &observer)) {
				observer_count++;
			}
			if(iterator.status != B_OK)
				goto err;
			curr = curr->tmp_ptr;
		}
	}
	list = malloc(sizeof(observer_list) +
	              sizeof(binder_node_id) * (observer_count - 1));
	if(list == NULL) {
		goto err;
	}
	list->count = 0;

	while(node_count > 0) {
		observer_iterrator iterator;
		binder_node_observer *observer;
		if(curr_head == NULL || curr_head == &end) {
			dprintf("binder -- get_observer_list: curr_head == NULL - 2\n");
			goto err;
		}
		init_observer_iterrator(&iterator, curr_head);
		while(next_observer(&iterator, &observer)) {
			if(list->count >= observer_count) {
				dprintf("binder -- get_observer_list: observer_count %d == "
				        "list->count\n", observer_count);
				goto err;
			}
			list->node_id[list->count] = observer->client_id;
			list->count++;
		}
		if(iterator.status != B_OK)
			goto err;
		{
			binder_node *tmp_node = curr_head;
			curr_head = curr_head->tmp_ptr;
			tmp_node->tmp_ptr = NULL;
		}
		node_count--;
	}
err:
	cleanup_node_list(curr_head, &end);
	if(lock_list)
		UNLOCK(link_lock);
	return list;
}

static node_list *
get_node_list(binder_node *start_node, uint8 linkFromFlags, uint8 linkToFlags,
              bool include_start, bool lock_list)
{
	binder_node end;
	binder_node *curr;
	int list_count;
	node_list *list = NULL;

	curr = start_node;
	if(lock_list)
		LOCK(link_lock);
	start_node->tmp_ptr = &end;
	list_count = 1;
	while(curr != &end) {
		int i;
		if(curr == NULL) {
			dprintf("binder -- get_node_list: curr == NULL - 1\n");
			goto err;
		}
		for(i = 0; i < curr->link_count; i++)
			if((curr->links[i].fromFlags & linkFromFlags) ||
			   (curr->links[i].toFlags & linkToFlags)) {
				binder_node *node = curr->links[i].link;
				if(node->tmp_ptr != NULL) {
					dprintf("binder -- get_node_list: node %p already "
					        "visited\n", node);
				}
				else {
					node->tmp_ptr = curr->tmp_ptr;
					curr->tmp_ptr = node;
					list_count++;
				}
			}
		curr = curr->tmp_ptr;
	}
	list = (node_list *)malloc(sizeof(node_list) +
	                           sizeof(binder_node *) * (list_count - 1));
	if(list == NULL) {
		goto err;
	}
	if(include_start) {
		curr = start_node;
	}
	else {
		curr = start_node->tmp_ptr;
		start_node->tmp_ptr = NULL;
		list_count--;
	}
	list->count = 0;
	while(list_count > 0) {
		list_count--;
		if(curr == NULL || curr == &end) {
			dprintf("binder -- get_node_list: curr == NULL - 2\n");
			goto err;
		}
		list->node[list->count] = curr;
		curr = curr->tmp_ptr;
		list->node[list->count]->tmp_ptr = NULL;
		add_ref_node(list->node[list->count]);
		list->count++;
	}
err:
	while(curr != &end) {
		binder_node *node = curr;
		if(curr == NULL) {
			dprintf("binder -- get_node_list: curr == NULL - 3\n");
			break;
		}
		curr = curr->tmp_ptr;
		node->tmp_ptr = NULL;
	}
	if(lock_list)
		UNLOCK(link_lock);
	return list;
}

node_list *
get_call_order_node_list(binder_node *start_node)
{
	binder_node bottom;
	binder_node *stack1, *stack2, *stack3;
	int stack3_count = 0;
	node_list *list = NULL;

	stack1 = start_node;
	stack2 = &bottom;
	stack3 = &bottom;
	
	if(start_node == NULL) {
		dprintf("binder(%d) -- get_call_order_node_list: start_node == NULL\n",
		        (int)find_thread(NULL));
		return NULL;
	}

	LOCK(link_lock);
	start_node->tmp_ptr = &bottom;
	while(stack1 != &bottom || stack2 != &bottom) {
		if(stack1 != &bottom) {
			int i;
			binder_node *base_node = stack1;
			if(base_node == NULL) {
				dprintf("binder -- get_call_order_node_list: "
				        "base_node == NULL - 1\n");
				goto err;
			}
			stack1 = stack1->tmp_ptr;
			base_node->tmp_ptr = stack2;
			stack2 = base_node;
			for(i = base_node->link_count - 1; i >= 0 ; i--)
				if(base_node->links[i].fromFlags & linkFilter) {
					binder_node *node = base_node->links[i].link;
					if(node->tmp_ptr != NULL) {
						dprintf("binder: node %p already visited\n",
						        node);
					}
					else {
						node->tmp_ptr = stack1;
						stack1 = node;
					}
				}
		}
		if(stack1 == &bottom && stack2 != &bottom) {
			int i;
			binder_node *base_node = stack2;
			if(base_node == NULL) {
				dprintf("binder -- get_call_order_node_list: "
				        "base_node == NULL - 1\n");
				goto err;
			}
			stack2 = stack2->tmp_ptr;
			base_node->tmp_ptr = stack3;
			stack3 = base_node;
			stack3_count++;
			for(i = base_node->link_count - 1; i >= 0 ; i--)
				if(base_node->links[i].fromFlags & linkInherit) {
					binder_node *node = base_node->links[i].link;
					if(node->tmp_ptr != NULL) {
						dprintf("binder: node %p already visited\n",
						        node);
					}
					else {
						node->tmp_ptr = stack1;
						stack1 = node;
					}
				}
		}
	}

	list = (node_list *)malloc(sizeof(node_list) +
	                           sizeof(binder_node *) * (stack3_count - 1));
	if(list == NULL) {
		goto err;
	}
	list->count = stack3_count;
	while(stack3_count > 0) {
		if(stack3 == NULL || stack3 == &bottom) {
			int i;
			dprintf("binder -- get_call_order_node_list: stack3 == NULL\n");
			for(i = stack3_count; i < list->count; i++)
				unref_node(list->node[i]);
			free(list);
			list = NULL;
			goto err;
		}
		stack3_count--;
		list->node[stack3_count] = stack3;
		stack3 = stack3->tmp_ptr;
		list->node[stack3_count]->tmp_ptr = NULL;
		add_ref_node(list->node[stack3_count]);
	}
err:
	while(stack3 != &bottom) {
		binder_node *node = stack3;
		if(stack3 == NULL) {
			dprintf("binder -- get_call_order_node_list-2: stack3 == NULL\n");
			break;
		}
		stack3 = stack3->tmp_ptr;
		node->tmp_ptr = NULL;
	}
	UNLOCK(link_lock);
	return list;
}

// #pragma mark -

status_t
add_reflection_command(reflection_struct *reflection,
                       reflection_cmd command, int32 token)
{
	if(reflection->active) {
		dprintf("binder(%d) -- add_reflection_command: reflection already "
		        "active\n", (int)find_thread(NULL));
		return B_ERROR;
	}
	reflection->command = command;
	reflection->token = token;
	reflection->active = true;
	return B_OK;
}

transaction_state *
new_transaction_state(int command_type)
{
	transaction_state *state;
	state = (transaction_state *)calloc(1, sizeof(transaction_state));
	if(state == NULL)
		return NULL;
	state->command_type = command_type;
	return state;
}

void
cleanup_transaction_state(void *cookie)
{
	transaction_state *state = cookie;
	if(state->resources.reply_port != NULL) {
		if(!is_port_valid(state->resources.reply_port)) {
			dprintf("binder(%d) -- cleanup_transaction_state: bad reply port\n",
			        (int)find_thread(NULL));
		}
		else {
			dprintf("binder(%d) -- cleanup_transaction_state: cleanup reply port"
			        " %d\n", (int)find_thread(NULL),
			        (int)state->resources.reply_port->port);
			pp_delete_port(state->resources.reply_port);
		}
	}
	if(state->resources.reply_port_2 != NULL) {
		if(!is_port_valid(state->resources.reply_port_2)) {
			dprintf("binder(%d) -- cleanup_transaction_state: bad reply port\n",
			        (int)find_thread(NULL));
		}
		else {
			dprintf("binder(%d) -- cleanup_transaction_state: cleanup reply port"
			        " %d\n", (int)find_thread(NULL),
			        (int)state->resources.reply_port_2->port);
			pp_delete_port(state->resources.reply_port_2);
		}
	}
	if(state->resources.nodes != NULL) {
		dprintf("binder(%d) -- cleanup_transaction_state: cleanup node list"
		        " %p\n", (int)find_thread(NULL), state->resources.nodes);
		free_node_list(state->resources.nodes);
	}
	if(state->resources.node != NULL) {
		dprintf("binder(%d) -- cleanup_transaction_state: node %d-%d still "
		        "referenced\n", (int)find_thread(NULL),
		        (int)state->resources.node->nodeid.port, 
		        (int)state->resources.node->nodeid.token);
	}
	if(state->resources.observers != NULL)
		free(state->resources.observers);
	if(state->resources.reply != NULL)
		free_binder_reply((binder_reply_basic *)state->resources.reply);
	free(state);
}

// #pragma mark -

static status_t
add_ref_node(binder_node *node)
{
	//status_t err;
	int32 old;

	get_ref_binder_node(node);
	old = atomic_add(&node->primary_ref_count, 1);
#if TRACE_REFS
	{
		int total_refs = atomic_add(&global_ref_count, 1) + 1;
		dprintf("binder(%d) -- add_ref_node: %d-%d rc %x (total %d)\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, (int)old+1, total_refs);
	}
#endif
	if(old == 0) {
		dprintf("binder(%d) -- add_ref_node: ERROR node %d-%d not referenced"
		        "\n", (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token);
		//kernel_debugger("binder");
	}
	return B_OK;
//err:
//	return err;
}

static status_t
ref_node(binder_node *node, reflection_struct *reflection,
         state_ref_node *state, transaction_resources *resources)
{
	const char * const function_name = "ref_node";
	enum {
		STAGE_BASE = STAGE_REF_NODE_BASE,
		STAGE_CONNECT = STAGE_BASE,
		STAGE_GET_REPLY,
		STAGE_CONNECT_REFLECTED,
		STAGE_COUNT
	};
	status_t err;

	CHECK_ENTRY_STATE();

	if(state->stage == STAGE_INIT) {
		int32 old;
		get_ref_binder_node(node);
		old = atomic_add(&node->primary_ref_count, 1);
		if(old == 0) {
			state->stage = STAGE_CONNECT;
		}
#if TRACE_NODES
		if(old == 0) {
			//int total_count = atomic_add(&global_node_count, 1) + 1;
			dprintf("binder(%d) -- ref_node: node %d-%d need connection\n",
			        (int)find_thread(NULL),
			        (int)node->nodeid.port, (int)node->nodeid.token);
		}
#endif
#if TRACE_REFS
		{
			int total_refs = atomic_add(&global_ref_count, 1) + 1;
			dprintf("binder(%d) -- ref_node: %d-%d rc %x (total %d)\n",
			        (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token, (int)old+1, total_refs);
		}
#endif
	}

	if(state->stage == STAGE_CONNECT) {
		if(node->host == current_team()) {
			add_reflection_command(reflection, REFLECT_CMD_ACQUIRE,
			                       node->nodeid.token);
#if TRACE_REFLECTION
			dprintf("binder(%d) -- ref_node: reflect acquire\n",
			        (int)find_thread(NULL));
#endif
			state->stage = STAGE_CONNECT_REFLECTED;
			return B_OK;
		}
		err = send_binder_request_no_arg(node, BINDER_REQUEST_CONNECT,
		                                 &resources->reply_port);
		if(err != B_OK) {
			dprintf("binder(%d) -- ref_node: send_binder_request failed, "
			        "%s\n", (int)find_thread(NULL), strerror(err));
			goto err2;
		}
		state->stage = STAGE_GET_REPLY;
	}
	if(state->stage == STAGE_GET_REPLY) {
		err = get_binder_reply_status(node, resources->reply_port, reflection);
		if(err != B_OK)
			goto err2;
		if(reflection->active)
			return B_OK;
		//dprintf("Connecting to node %016Lx --> %d\n",vn->vnid.value,(int)err);
	}
	if(state->stage == STAGE_CONNECT_REFLECTED) {
		err = reflection->reflection_result;
		if(err < B_OK)
			goto err2;
	}
	err = B_OK;
	goto done;

err2:
	atomic_add(&node->primary_ref_count, -1);
//err1:
done:
	state->stage = STAGE_DONE;
	resources->reply_port = NULL;
	CHECK_EXIT_STATE();
	return err;
}

static status_t
unref_node(binder_node *node)
{
	status_t err = B_OK;
	int32 old = atomic_add(&node->primary_ref_count, -1);

#if TRACE_REFS
	int total_refs = atomic_add(&global_ref_count, -1) - 1;
	dprintf("binder(%d) -- unref_node: %d-%d rc %x (total %d)\n",
	        (int)find_thread(NULL), (int)node->nodeid.port,
	        (int)node->nodeid.token, (int)old - 1, total_refs);
#endif
	if(old == 1) {
#if TRACE_NODES
		dprintf("binder(%d) -- unref_node: no more clients for node %d-%d\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token);
#endif
		err = send_binder_request_no_arg(node, BINDER_REQUEST_DISCONNECT, NULL);
		if(err < B_OK) {
			dprintf("binder(%d) -- unref_node: send_binder_request failed, "
			        "%s\n", (int)find_thread(NULL), strerror(err));
		}
		//dprintf("Disconnecting from node %016Lx --> %d\n",vn->vnid.value, (int)err);
	}
#if TRACE_NODES
	if(old == 1) {
		int total_count = atomic_add(&global_node_count, -1) - 1;
		dprintf("binder(%d) -- unref_node: node deleted %d-%d, "
		        "nodes %d\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, total_count);
	}
#endif
	return_binder_node(node);
	//put_vnode(vn->ns->nsid,vn->vnid.value);
	
	return err;
}

// #pragma mark -

/*	Link flags can be any of linkInherit, linkFilter and linkAugment

	(fromFlags & linkInherit)
		If a get or put fails or falls through on this node, try the linked node
	(fromFlags & linkFilter)
		Filter any gets or puts through the linked node before trying this node
	(fromFlags & linkAugment)
		This node's property *directory* is augmented with the linked node's
*/	

static status_t
create_link(binder_node *linkFrom, binder_node *linkTo, uint32 linkFlags,
            reflection_struct *reflection, state_create_link *state,
            transaction_resources *resources)
{
	status_t err;

	if(!linkFlags)
		return B_ERROR;

	LOCK(link_lock);
#if TRACE_LINKS
	dprintf("binder(%d) -- create_link: from %d-%d to %d-%d\n",
	        (int)find_thread(NULL),
	        (int)linkFrom->nodeid.port, (int)linkFrom->nodeid.token,
	        (int)linkTo->nodeid.port, (int)linkTo->nodeid.token);
#endif
	
	if(state->stage == 0) {
		int32 i, j;
		int32 count = linkFrom->link_count;
		
		for(i = 0; i < count; i++) {
			if(linkFrom->links[i].link == linkTo) {
				for(j = 0; j < linkTo->link_count; j++) {
					if(linkTo->links[j].link == linkFrom)
						break;
				}
				if(j == linkTo->link_count) {
					dprintf("binder(%d) -- create_link: vnode links are "
					        "corrupt (1)!\n", (int)find_thread(NULL));
					err = B_ERROR;
					goto err1;
				}
				if(linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
					dprintf("binder(%d) -- create_link: vnode links are "
					        "corrupt (2)!\n", (int)find_thread(NULL));
					err = B_ERROR;
					goto err1;
				}
				linkFrom->links[i].fromFlags |= linkFlags;
				linkTo->links[j].toFlags |= linkFlags;
				err = B_OK;
				goto done;
			}
		}
		state->stage = 1;
	}

	if(state->stage == 1) {
		err = ref_node(linkFrom, reflection, &state->ref_node, resources);
		if(err < B_OK)
			goto err1;
		if(reflection->active) {
			err = B_OK;
			goto exit_reflected;
		}
#if TRACE_LINKS
		dprintf("binder(%d) -- create_link: 1 Getting vnode %d-%d\n",
		        (int)find_thread(NULL), (int)linkFrom->nodeid.port,
		        (int)linkFrom->nodeid.token);
#endif
		state->stage = 2;
	}
	if(state->stage == 2) {
		int32 i,j,count;
		node_link *new_fromlinks;
		node_link *new_tolinks;

		err = ref_node(linkTo, reflection, &state->ref_node, resources);
		if(err < B_OK)
			goto err2;
		if(reflection->active) {
			err = B_OK;
			goto exit_reflected;
		}
#if TRACE_LINKS
		dprintf("binder(%d) -- create_link: 2 Getting vnode %d-%d\n",
		        (int)find_thread(NULL), (int)linkTo->nodeid.port,
		        (int)linkTo->nodeid.token);
#endif

		count = linkFrom->link_count;
		for(i = 0; i < count; i++) {
			if(linkFrom->links[i].link == linkTo) {
				for(j = 0; j < linkTo->link_count; j++) {
					if(linkTo->links[j].link == linkFrom)
						break;
				}
				if(j == linkTo->link_count) {
					dprintf("binder(%d) -- create_link: node links are "
					        "corrupt (1)!\n", (int)find_thread(NULL));
					err = B_ERROR;
					goto err3;
				}
				if(linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
					dprintf("binder(%d) -- create_link: node links are "
					        "corrupt (2)!\n", (int)find_thread(NULL));
					err = B_ERROR;
					goto err3;
				}
				linkFrom->links[i].fromFlags |= linkFlags;
				linkTo->links[j].toFlags |= linkFlags;
				err = B_OK;
				goto abort;
			}
		}

		new_fromlinks = (node_link *)realloc(linkFrom->links,
		                                     (linkFrom->link_count + 1) *
		                                     sizeof(node_link));
		if(new_fromlinks == NULL) {
			err = B_NO_MEMORY;
			goto err3;
		}
		new_tolinks = (node_link *)realloc(linkTo->links,
		                                   (linkTo->link_count + 1) *
		                                   sizeof(node_link));
		if(new_tolinks == NULL) {
			err = B_NO_MEMORY;
			goto err3;
		}
		
		linkFrom->links = new_fromlinks;
		linkFrom->links[linkFrom->link_count].link = linkTo;
		linkFrom->links[linkFrom->link_count].fromFlags = linkFlags;
		linkFrom->links[linkFrom->link_count].toFlags = 0;
		linkFrom->link_count++;
	
		linkTo->links = new_tolinks;
		linkTo->links[linkTo->link_count].link = linkFrom;
		linkTo->links[linkTo->link_count].toFlags = linkFlags;
		linkTo->links[linkTo->link_count].fromFlags = 0;
		linkTo->link_count++;
	}
	else {
		err = B_ERROR;
		dprintf("binder(%d) -- create_link: bad state, stage = %d\n",
		        (int)find_thread(NULL), state->stage);
	}

	err = B_OK;
	goto done;

abort:
err3:
	unref_node(linkTo);
err2:
	unref_node(linkFrom);
err1:
done:
	state->stage = 0;
exit_reflected:
	UNLOCK(link_lock);
	return err;
}

static status_t
destroy_link(binder_node *base,
             uint32 linkFlags, bool from /*=true*/,
             reflection_struct *reflection, state_destroy_link *state,
             transaction_resources *resources)
{
	const char * const function_name = "destroy_link";
	enum {
		STAGE_BASE = STAGE_DESTROY_LINK_BASE,
		STAGE_NOTIFY = STAGE_BASE,
		STAGE_COUNT
	};
	status_t err;
	binder_node *linkTo, *linkFrom;
	int t, i, j, k;

	CHECK_ENTRY_STATE();

	LOCK(link_lock);

	if(state->stage == STAGE_INIT) {
		bool node_has_links;
		bool link_changed = false;

#if TRACE_LINKS
		dprintf("binder(%d) -- destroy_link: base   = %d-%d, from = %d\n",
		        (int)find_thread(NULL), (int)base->nodeid.port,
		        (int)base->nodeid.token, (int)from);
#endif

		//dprintf("binder(%d) -- notify_callback: node %016Lx\n",
		//        (int)find_thread(NULL), node->vnid.value);
		if(resources->observers != NULL) {
			dprintf("binder(%d) -- destroy_link: observers != NULL in "
			        "STAGE_INIT\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err1;
		}
		resources->observers = get_observer_list(base, false, &node_has_links);
		if(resources->observers == NULL) {
			dprintf("binder(%d) -- destroy_link: get_observer_list returned"
			        "NULL\n", (int)find_thread(NULL));
			err = B_NO_MEMORY;
			goto err1;
		}

		for(t = 0; t < base->link_count; t++) {
			i = t;
#if TRACE_LINKS
			dprintf("binder(%d) -- destroy_link:\n"
			        "                              from   = %d\n"
			        "                              base   = %d-%d\n"
			        "                              linkTo = %d-%d\n",
			        (int)find_thread(NULL), (int)from, (int)base->nodeid.port,
			        (int)base->nodeid.token,
			        (int)base->links[i].link->nodeid.port,
			        (int)base->links[i].link->nodeid.token);
#endif
			linkFrom = base;
			linkTo = base->links[i].link;
			for(j = 0; j < linkTo->link_count; j++) {
#if TRACE_LINKS
				dprintf("binder(%d) -- destroy_link: "
				        "linkTo.links[%d] = %d-%d\n", (int)find_thread(NULL),
				        j, (int)linkTo->links[j].link->nodeid.port,
				        (int)linkTo->links[j].link->nodeid.token);
#endif
				if(linkTo->links[j].link == linkFrom)
					break;
			}
			if(j == linkTo->link_count) {
				dprintf("binder(%d) -- destroy_link: node links are "
				        "corrupt (1)!\n", (int)find_thread(NULL));
				err = B_ERROR;
				goto err2;
			}
			if(linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
				dprintf("binder(%d) -- destroy_link: node links are "
				        "corrupt (2)!\n", (int)find_thread(NULL));
				err = B_ERROR;
				goto err2;
			}
			if(!from) {
				binder_node *tmp;
				k=i; i=j; j=k;
				tmp=linkTo; linkTo=linkFrom; linkFrom=tmp;
			}

#if 0	
			{
				notify_callback_struct ncs;
				ncs.event = B_SOMETHING_CHANGED;
				ncs.name[0] = 0;
				//notify_callback(linkFrom,&ncs, &state->notify, reflection,
				//                resources, false);
				dprintf("binder(%d) -- destroy_link: notification not sent\n",
				        (int)find_thread(NULL));
			}
#endif
			
			if(linkFrom->links[i].fromFlags & linkFlags)
				link_changed = true;
			linkFrom->links[i].fromFlags &= ~linkFlags;
			linkTo->links[j].toFlags &= ~linkFlags;
	
			if(!linkTo->links[j].toFlags && !linkTo->links[j].fromFlags) {
				for(k = j; k < linkTo->link_count - 1; k++)
					linkTo->links[k] = linkTo->links[k+1];
				linkTo->link_count--;
				unref_node(linkFrom);
#if TRACE_LINKS
				dprintf("binder(%d) -- destroy_link: 1 Putting node %d-%d\n",
				        (int)find_thread(NULL), (int)linkFrom->nodeid.port,
				        (int)linkFrom->nodeid.token);
#endif
				if(!from)
					t--;
			}
			if(!linkFrom->links[i].fromFlags && !linkFrom->links[i].toFlags) {
				for(k = i; k < linkFrom->link_count - 1; k++)
					linkFrom->links[k] = linkFrom->links[k+1];
				linkFrom->link_count--;
				unref_node(linkTo);
#if TRACE_LINKS
				dprintf("binder(%d) -- destroy_link: 2 Putting node %d-%d\n",
				        (int)find_thread(NULL), (int)linkTo->nodeid.port,
				        (int)linkTo->nodeid.token);
#endif
				if(from)
					t--;
			}
		}
		if(link_changed) {
			state->stage = STAGE_NOTIFY;
			state->index = 0;
			//dprintf("binder(%d) -- destroy_link: send notifications\n",
			//        (int)find_thread(NULL));
		}
		else {
			//dprintf("binder(%d) -- destroy_link: no change\n",
			//        (int)find_thread(NULL));
			state->stage = STAGE_DONE;
		}
	}
	if(state->stage == STAGE_NOTIFY) {
		notify_callback_struct ncs;
		ncs.event = B_SOMETHING_CHANGED;
		ncs.name[0] = 0;
		if(resources->observers == NULL) {
			dprintf("binder(%d) -- destroy_link: observers == NULL in "
			        "STAGE_NOTIFY\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err2;
		}
		while(state->index < resources->observers->count) {
			one_notify_callback(resources->observers->node_id[state->index],
			                    &ncs, &state->one_notify, reflection,
			                    resources);
			if(reflection->active)
				goto reflect;
			state->index++;
		}
	}
	err = B_OK;
err2:
	state->stage = STAGE_DONE;
	state->index = 0;
	free(resources->observers);
	resources->observers = NULL;
err1:
reflect:
	if(reflection->active)
		err = B_OK;
	UNLOCK(link_lock);
	CHECK_EXIT_STATE();
	return err;
}

static status_t
update_redirect(binder_node *node, reflection_struct *reflection,
                state_update_redirect *state, transaction_resources *resources)
{
	status_t err;
	//dprintf("binder(%d) -- update_redirect: entry stage == %d\n",
	//        (int)find_thread(NULL), state.stage);
	if(state->stage == 0) {
		bool hadRedirect,needRedirect = false;
		LOCK(link_lock);
		hadRedirect = (node->flags & vnodeRedirect) != 0;
		needRedirect = false;
		if(node->link_count)
			needRedirect = true;
		if(needRedirect)
			node->flags |= vnodeRedirect;
		else
			node->flags &= ~vnodeRedirect;
		UNLOCK(link_lock);
	
#if TRACE_REDIRECT
		dprintf("binder(%d) -- update_redirect: %d-%d team %d redirect "
		        "%d -> %d\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, (int)node->host,
		        hadRedirect, needRedirect);
#endif	
		if(needRedirect != hadRedirect) {
			if(is_local(node->nodeid.port)) {
#if TRACE_REFLECTION
				dprintf("binder(%d) -- update_redirect: reflect %sredirect\n",
				        (int)find_thread(NULL), needRedirect ? "" : "un");
#endif
				add_reflection_command(reflection, needRedirect ?
				                       REFLECT_CMD_REDIRECT :
				                       REFLECT_CMD_UNREDIRECT,
				                       node->nodeid.token);
				state->stage = 2;
				return B_OK;
			} else {
				err = send_binder_request_one_arg(node, BINDER_REQUEST_REDIRECT,
				                                  needRedirect,
				                                  &resources->reply_port);
				if(err != B_OK) {
					dprintf("binder(%d) -- update_redirect: "
					        "send_binder_request failed, %s\n",
					        (int)find_thread(NULL), strerror(err));
					goto err;
				}
				state->stage = 1;
			}
		}
		else {
			state->stage = 2;
		}
	}
	if(state->stage == 1) {
		err = get_binder_reply_status(node, resources->reply_port, reflection);
		if(err < B_OK)
			goto err;
		if(reflection->active)
			return B_OK;
	}
	else if(state->stage == 2) {
		err = reflection->reflection_result;
		if(err < B_OK)
			goto err;
	}
	else {
		dprintf("binder(%d) -- update_redirect: bad state, stage == %d\n",
		        (int)find_thread(NULL), state->stage);
	}
	err = B_OK;
err:
	state->stage = 0;
	resources->reply_port = NULL;
	return err;
}

static status_t
link_nodes(binder_connection *c, binder_cmd *cmd,
           transaction_state *full_state, uint32 linkFlags)
{
	const char * const function_name = "link_nodes";
	state_link_nodes *state = &full_state->u.link_nodes;
	transaction_resources *resources = &full_state->resources;
	reflection_struct *reflection = &cmd->reflection;
	enum {
		STAGE_BASE = STAGE_LINK_NODES_BASE,
		STAGE_CREATE_LINK = STAGE_BASE,
		STAGE_UPDATE_REDIRECT,
		STAGE_NOTIFY,
		STAGE_COUNT
	};
	status_t err = B_OK;
	binder_node *node;

	CHECK_ENTRY_STATE();

	node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION, cmd->node_handle);

	if(node == NULL) {
		dprintf("binder(%d) -- link_nodes: could not find from node, 0x%08x\n",
		        (int)find_thread(NULL), (int)cmd->node_handle);
		//kernel_debugger("binder link_nodes, node == NULL");
		return B_ERROR;
	}

	//dprintf("link_nodes entry, stage = %d\n", state.stage);
	if(state->stage == STAGE_INIT) {
		if(!(node->flags & vnodeHosted)) {
			dprintf("binder(%d) -- link_nodes: from node, %d-%d, must be "
			        "hosted!\n", (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
			err = B_ERROR;
			goto err1;
		}
		
		if(resources->node != NULL) {
			dprintf("binder(%d) -- link_nodes: resources.node not NULL, %p\n",
			        (int)find_thread(NULL), resources->node);
			err = B_ERROR;
			goto err1;
		}
		resources->node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
		                                  cmd->data.node_handle);
		if(resources->node == NULL) {
			err = B_BAD_VALUE;
			goto err1;
		}
		//	binder_node *node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
		//	                                    cmd->node_handle);
		//err = find_binder_node(cmd->data.nodeid, &resources->node);
		//if(err < B_OK)
		//	goto err1;

		if(node == resources->node) {
			dprintf("binder(%d) -- link_nodes: from and to nodes are the "
			        "same!\n", (int)find_thread(NULL));
			err = B_BAD_VALUE;
			goto err2;
		}
	
		if(!(resources->node->flags & vnodeHosted)) {
			dprintf("binder(%d) -- link_nodes: to node, %d-%d, must be "
			        "hosted!\n", (int)find_thread(NULL),
			        (int)resources->node->nodeid.port,
			        (int)resources->node->nodeid.token);
			err = B_ERROR;
			goto err2;
		}
		state->stage = STAGE_CREATE_LINK;
	}
	if(state->stage == STAGE_CREATE_LINK) {
		//dprintf("link_nodes: create_link\n");
		err = create_link(node, resources->node, linkFlags, reflection,
		                  &state->u.create_link, resources);
		if(err < B_OK)
			goto err2;
		if(reflection->active)
			goto reflect;
		state->stage = STAGE_UPDATE_REDIRECT;
	}
	if(state->stage == STAGE_UPDATE_REDIRECT) {
		//dprintf("link_nodes: update_redirect\n");
		err = update_redirect(node, reflection, &state->u.update_redirect,
		                      resources);
		if(err < B_OK)
			goto err2;
		if(reflection->active)
			goto reflect;
		//update_redirect(me,cmd); // do we need to update both?
		state->stage = STAGE_NOTIFY;
	}
	if(state->stage == STAGE_NOTIFY) {
		notify_callback_struct ncs;
		ncs.event = B_SOMETHING_CHANGED;
		ncs.name[0] = 0;
		notify_callback(node, &ncs, &state->u.notify, reflection,
		                resources, true);
		if(reflection->active)
			goto reflect;
	}
	//dprintf("link_nodes: return %s\n", strerror(err));
err2:
	return_cookie_data(c, COOKIE_CLIENT_CONNECTION, resources->node);
	resources->node = NULL;
err1:
	state->stage = 0;
reflect:
	return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
	if(reflection->active)
		err = B_OK;
	CHECK_EXIT_STATE();
	return err;
}

static status_t
break_links(binder_node *node, binder_cmd *cmd,
            transaction_state *full_state)
{
	const char * const function_name = "break_links";
	state_break_links *state = &full_state->u.break_links;
	transaction_resources *resources = &full_state->resources;
	reflection_struct *reflection = &cmd->reflection;
	enum {
		STAGE_BASE = STAGE_BREAK_LINKS_BASE,
		STAGE_BREAK_TO_LINKS = STAGE_BASE,
		STAGE_UPDATE_REDIRECT,
		STAGE_COUNT
	};
	status_t err;
	uint32 flags = cmd->data.break_links.flags;

	if(node == NULL) {
		dprintf("binder(%d) -- break_links: node == NULL\n",
		        (int)find_thread(NULL));
		//kernel_debugger("binder break_links, node == NULL");
		return B_ERROR;
	}
	CHECK_ENTRY_STATE();

	if(state->stage == STAGE_INIT) {
		if (flags & linkFrom) {
			err = destroy_link(node, flags&(linkInherit|linkAugment|linkFilter),
			                   true, &cmd->reflection, &state->u.destroy_link,
			                   resources);
			if(cmd->reflection.active)
				goto reflect;
		}
		state->stage = STAGE_BREAK_TO_LINKS;
	}
	if(state->stage == STAGE_BREAK_TO_LINKS) {
		if (flags & linkTo) {
			err = destroy_link(node, flags&(linkInherit|linkAugment|linkFilter),
			                   false, &cmd->reflection, &state->u.destroy_link,
			                   resources);
			if(cmd->reflection.active)
				goto reflect;
		}
		state->stage = STAGE_UPDATE_REDIRECT;
	}
	if(state->stage == STAGE_UPDATE_REDIRECT) {
		err = update_redirect(node, &cmd->reflection, &state->u.update_redirect,
		                      resources);
		if(err < B_OK)
			goto err;
		if(cmd->reflection.active)
			goto reflect;
	}
	err = B_OK;
err:
	state->stage = STAGE_DONE;
reflect:
	if(reflection->active)
		err = B_OK;
	CHECK_EXIT_STATE();
	return err;
}

#if 0
static int
unlink_node(binder_node *node)
{
	reflection_struct reflection;
	state_update_redirect state;
	transaction_resources resources;
	memset(&state, 0, sizeof(state));
	memset(&resources, 0, sizeof(resources));

	destroy_link(node,linkAll,true);	
	destroy_link(node,linkAll,false);

	reflection.active = true;
	while(reflection.active) {
		status_t err;
		reflection.active = false;
		err = update_redirect(node, &reflection, &state, &resources);
		if(err < 0)
			return err;
		if(reflection.active) {
			dprintf("binder(%d) -- unlink_node: ignored reflection\n",
			        (int)find_thread(NULL));
			reflection.reflection_result = B_ERROR;
		}
	}
	return B_OK;
}
#endif

// #pragma mark -

static status_t
set_root_node(binder_cmd *cmd)
{
	if(root_node_id.port >= 0 && cmd->data.nodeid.port >= 0) {
		dprintf("binder(%d) -- set_root_node: root node already set\n",
		        (int)find_thread(NULL));
	}
	if(root_node_id.port < 0 && cmd->data.nodeid.port < 0) {
		dprintf("binder(%d) -- set_root_node: root node already cleared\n",
		        (int)find_thread(NULL));
	}
	root_node_id = cmd->data.nodeid;
	if(root_node_set_sem >= 0) {
		delete_sem(root_node_set_sem);
		root_node_set_sem = -1;
	}
	
	return B_OK;
}

static status_t
get_root_node(binder_cmd *cmd)
{
	if(root_node_id.port < 0) {
		dprintf("binder(%d) -- get_root_node: root node not set\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	cmd->data.nodeid = root_node_id;
	return B_OK;
}

// #pragma mark -

static void
host_died(binder_node *node)
{
	reflection_struct reflection;
	union {
		state_destroy_link      destroy_link;
		state_update_redirect   update_redirect;
	} state;
	int stage;
	transaction_resources resources;
#if TRACE_NODES
	dprintf("binder(%d) -- host_died: Hosting ceased for node %d-%d\n",
	        (int)find_thread(NULL), (int)node->nodeid.port,
	        (int)node->nodeid.token);
#endif
	//atomic_add(&node->primary_ref_count, -65535);
#if TRACE_REFS
	dprintf("binder(%d) -- host_died: %d-%d (rc %x)\n",
	        (int)find_thread(NULL), (int)node->nodeid.port,
	        (int)node->nodeid.token, (int)node->primary_ref_count);
#endif
	node->flags = (node->flags | vnodeDied) & ~vnodeHosted;

	memset(&state, 0, sizeof(state));
	memset(&resources, 0, sizeof(resources));

	stage = 0;
	reflection.active = true;
	while(reflection.active) {
		reflection.active = false;
		if(stage == 0) {
			destroy_link(node, linkAll, true, &reflection,
			             &state.destroy_link, &resources);
			if(!reflection.active)
				stage = 1;
		}
		if(stage == 1) {
			destroy_link(node, linkAll, false, &reflection,
			             &state.destroy_link, &resources);
			if(!reflection.active)
				stage = 2;
		}
		if(stage == 2) {
			update_redirect(node, &reflection, &state.update_redirect,
			                &resources);
			//if(!reflection.active)
			//	stage = 3;
	 	}
		if(reflection.active) {
			dprintf("binder(%d) -- host_died: ignored reflection\n",
			        (int)find_thread(NULL));
			reflection.reflection_result = B_ERROR;
		}
	}
}

// #pragma mark -

static status_t
start_hosting(binder_connection *c, binder_cmd *cmd,
              state_start_hosting *state)
{
	enum {
		STAGE_REFLECT_ACQUIRE = STATE_START_HOSTING_BASE,
		STATE_START_HOSTING_END
	};
	if(state->stage == STAGE_INIT) {
		//int32 old;
		status_t err;
		binder_node *node;

		err = new_binder_node(cmd->data.nodeid, &node);
		if(err < B_OK)
			return err;
		if(node->host != current_team()) {
			dprintf("binder(%d) -- start_hosting: node %d-%d, wrong team\n",
			        (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
			return B_ERROR;
		}
		if(node->flags & vnodeHosted) {
			dprintf("binder(%d) -- start_hosting: node %d-%d already "
			        "hosted\n", (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
			return B_BAD_VALUE;
		}
		//if(c->type != FILE_COOKIE_UNUSED) {
		//	dprintf("binder(%d) -- start_hosting: fd already in use, "
		//	        "type %d\n", (int)find_thread(NULL), c->type);
		//	return B_BAD_VALUE;
		//}
		node->flags = (node->flags | vnodeHosted) & ~vnodeDied;
		
		add_cookie(c, COOKIE_HOST_CONNECTION, node);
		cmd->data.connect.return_node_handle = (uint32)node;

		//c->type = FILE_COOKIE_HOST_CONNECTION;
		//vnode *dummy;
		//get_vnode(node->ns->nsid, node->vnid.value,(void**)&dummy);
		//old = atomic_add(&node->primary_ref_count, 65536);
#if TRACE_REFS
		dprintf("binder(%d) -- start_hosting: %d-%d (rc %x)\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, (int)node->primary_ref_count);
#endif
#if TRACE_NODES
//		if(old == 0) {
		if(1) {
			int total_count = atomic_add(&global_node_count, 1) + 1;
			dprintf("binder(%d) -- start_hosting: hosted node created %d-%d,"
			        " nodes %d\n",
			        (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token, total_count);
		}
		else {
			dprintf("binder(%d) -- start_hosting: start hosting for node "
			        "%d-%d\n", (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
		}
#endif
#if 0
		if(old > 0) {
#if TRACE_REFLECTION
			dprintf("binder(%d) -- start_hosting: node %d-%d, reflect "
			        "acquire\n", (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
#endif
			add_reflection_command(&cmd->reflection, REFLECT_CMD_ACQUIRE,
			                        node->nodeid.token);
			state->stage = STAGE_REFLECT_ACQUIRE;
			return B_OK;
		}
#endif
		return_cookie_data(c, COOKIE_HOST_CONNECTION, node);
	}
	else if(state->stage < STATE_START_HOSTING_BASE ||
	        state->stage >= STATE_START_HOSTING_END) {
		dprintf("binder(%d) -- start_hosting: bad state, stage == %d\n",
		        (int)find_thread(NULL), state->stage);
		return B_ERROR;
	}

	state->stage = STAGE_DONE;
	return B_OK;
}

static status_t
stop_hosting(binder_connection *c, binder_cmd *cmd)
{
	status_t err;
	//int32 old;
	binder_node *node = get_cookie_data(c, COOKIE_HOST_CONNECTION,
	                                    cmd->node_handle);
	if(node == NULL) {
		dprintf("binder(%d) -- stop_hosting: bad node handle 0x%08lx\n",
		        (int)find_thread(NULL), cmd->node_handle);
		return B_ENTRY_NOT_FOUND;
	}

#if TRACE_NODES
	dprintf("binder(%d) -- stop_hosting: node %d-%d\n",
	        (int)find_thread(NULL), (int)node->nodeid.port,
	        (int)node->nodeid.token);
#endif
//retry:
	//old = atomic_add(&node->primary_ref_count, 1);
#if TRACE_REFS
	dprintf("binder(%d) -- stop_hosting: %d-%d (rc %x)\n",
	        (int)find_thread(NULL), (int)node->nodeid.port,
	        (int)node->nodeid.token, (int)node->primary_ref_count);
#endif
	if(node->primary_ref_count > 0) {
#if 0
		old = atomic_add(&node->primary_ref_count, -1); // not too safe
		if(old == 1) {
			dprintf("binder(%d) -- stop_hosting: %d-%d node refrenced "
			        "then not, retry\n", (int)find_thread(NULL),
			        (int)node->nodeid.port, (int)node->nodeid.token);
			goto retry;
		}
#endif
		dprintf("binder(%d) -- stop_hosting: %d-%d node refrenced, abort\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token);
		err = B_NOT_ALLOWED;
		goto err;
	}
	node->flags = (node->flags|vnodeDied) & ~vnodeHosted;

	LOCK(link_lock);
	if(node->link_count > 0) {
		dprintf("binder(%d) -- stop_hosting: %d-%d node has links, abort\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token);
		err = B_NOT_ALLOWED;
	}
	else
		err = B_OK;
	UNLOCK(link_lock);
	if(err < B_OK)
		goto err;

	//destroy_link(node,linkAll,true);	
	//destroy_link(node,linkAll,false);
	//unref_node(node);
	//c->type = FILE_COOKIE_UNUSED;
	remove_cookie(c, COOKIE_HOST_CONNECTION, node);
	err = B_OK;
err:
	return_cookie_data(c, COOKIE_HOST_CONNECTION, node);
	return err;
}

void
cleanup_host_connection(void *cookie)
{
	//dprintf("binder: cleanup_host_connection not done\n");
	binder_node *node = cookie;
	host_died(node);
	return_binder_node(node);
}

static status_t
connect(binder_connection *c, binder_connect_cmd *connect_cmd,
        reflection_struct *reflection,
        state_connect *state, transaction_resources *resources)
{
	status_t err;
	binder_node *node;

#if TRACE_CONNECTION
	dprintf("binder(%d) -- connect: node %d-%d / handle 0x%08x\n",
	        (int)find_thread(NULL), (int)connect_cmd->node_id.port,
	        connect_cmd->node_id.port < 0 ? 0 : (int)connect_cmd->node_id.token,
	        (int)connect_cmd->source_node_handle);
#endif
	if(connect_cmd->node_id.port < 0) {
		node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
		                       connect_cmd->source_node_handle);
		if(node == NULL) {
			err = B_BAD_VALUE;
			goto err0;
		}
		err = get_binder_node(node->nodeid, &node);
		return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
		if(err < B_OK)
			goto err0;
	}
	else {
		err = get_binder_node(connect_cmd->node_id, &node);
		if(err < B_OK)
			goto err0;
	}

	if(node->flags & vnodeDied) {
		err = B_BAD_VALUE;
		goto err1;
	}
	
	err = ref_node(node, reflection, &state->ref_node, resources);
	if(err < B_OK)
		goto err1;
	if(reflection->active)
		goto reflection;
	return_binder_node(node);
	add_cookie(c, COOKIE_CLIENT_CONNECTION, node);
	connect_cmd->return_node_handle = (uint32)node;
	return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
	return B_OK;

reflection:
	err = B_OK;
err1:
	return_binder_node(node);
err0:
	return err;
}

static status_t
get_node_id(binder_connection *c, binder_cmd *cmd)
{
	binder_node *node;
	node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
	                       cmd->node_handle);
	if(node == NULL) {
		cmd->data.nodeid.port = -1;
		return B_BAD_VALUE;
	}
	cmd->data.nodeid = node->nodeid;
	return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
	return B_OK;
}

void
cleanup_client_connection(void *cookie)
{
	binder_node *node = cookie;
	unref_node(node);
	//dprintf("binder: cleanup_client_connection not done\n");
}

// #pragma mark -

static status_t
add_observer(binder_connection *c, binder_node *host, binder_node_id client_id)
{
	status_t err;
	binder_node_observer *new_observer;

#if TRACE_NOTIFICATIONS
	dprintf("binder(%d) -- add_observer: add %d-%d as observer of node %d-%d\n",
	        (int)find_thread(NULL), (int)client_id.port, (int)client_id.token,
	        (int)host->nodeid.port, (int)host->nodeid.token);
#endif

	new_observer = malloc(sizeof(binder_node_observer));
	if(new_observer == NULL) {
		dprintf("binder(%d) -- add_observer: no memory\n",
		        (int)find_thread(NULL));
		return B_NO_MEMORY;
	}
	new_observer->client_id = client_id;
	new_observer->next = new_observer->prev = new_observer;

	err = add_cookie(c, COOKIE_OBSERVER, new_observer);
	if(err < B_OK)
		goto err;

	LOCK(link_lock);
	new_observer->next = host->observers.next;
	new_observer->prev = &host->observers;
	new_observer->next->prev = new_observer;
	new_observer->prev->next = new_observer;
	UNLOCK(link_lock);
	return_cookie_data(c, COOKIE_OBSERVER, new_observer);

	return B_OK;
err:
	free(new_observer);
	return err;
}

void
cleanup_observer(void *cookie)
{
	binder_node_observer *observer = cookie;
#if TRACE_NOTIFICATIONS
	dprintf("binder(%d) -- cleanup_observer: remove %d-%d as observer\n",
	        (int)find_thread(NULL), (int)observer->client_id.port,
	        (int)observer->client_id.token);
#endif
	LOCK(link_lock);
	observer->next->prev = observer->prev;
	observer->prev->next = observer->next;
	UNLOCK(link_lock);
	free(observer);
}

static status_t
remove_observer(binder_connection *c, binder_node *host,
                binder_node_id client_id)
{
	status_t err;
	binder_node_observer *observer, *circle_guard_observer;

#if TRACE_NOTIFICATIONS
	dprintf("binder(%d) -- remove_observer: remove %d-%d as observer of "
	        "node %d-%d\n", (int)find_thread(NULL), (int)client_id.port,
	        (int)client_id.token, (int)host->nodeid.port,
	        (int)host->nodeid.token);
#endif
	LOCK(link_lock);
	observer = host->observers.next;
	circle_guard_observer = observer;
	err = B_ENTRY_NOT_FOUND;
	while(observer != &host->observers) {
		if(observer->client_id.port == client_id.port &&
		   observer->client_id.token == client_id.token) {
			err = B_OK;
			break;
		}

		observer = observer->next;
		if(circle_guard_observer != &host->observers)
			circle_guard_observer = circle_guard_observer->next;
		if(circle_guard_observer != &host->observers)
			circle_guard_observer = circle_guard_observer->next;
		if(circle_guard_observer != &host->observers &&
		   circle_guard_observer == observer) {
			dprintf("binder(%d) -- remove_observer: bad observer list, "
			        "circle detected, host at %p\n",
			        (int)find_thread(NULL), host);
			break;
		}
	}
	if(err == B_OK) {
		observer->next->prev = observer->prev;
		observer->prev->next = observer->next;
		observer->next = observer->prev = observer;
	}
	else {
		dprintf("binder(%d) -- remove_observer: observer from %d-%d of %d-%d "
		        "not found\n", (int)find_thread(NULL),
		        (int)client_id.port, (int)client_id.token,
		        (int)host->nodeid.port, (int)host->nodeid.token);
		//kernel_debugger("binder breakpoint");
	}
	UNLOCK(link_lock);
	if(err == B_OK) {
		if(get_cookie_data(c, COOKIE_OBSERVER, (uint32)observer) == NULL)
			dprintf("binder(%d) -- remove_observer: could not get observer "
			        "observer\n", (int)find_thread(NULL));
		else {
			remove_cookie(c, COOKIE_OBSERVER, observer);
			return_cookie_data(c, COOKIE_OBSERVER, observer);
		}
	}
	return err;
}

static status_t
one_notify_callback(binder_node_id node_id, notify_callback_struct *ncs,
                    state_one_notify *state, reflection_struct *reflection,
                    transaction_resources *resources)
{
	const char * const function_name = "one_notify_callback";
	enum {
		STAGE_BASE = STAGE_ONE_NOTIFY_BASE,
		STAGE_GET_REPLY = STAGE_BASE,
		STAGE_NOTIFY_REFLECTED,
		STAGE_COUNT
	};
	status_t err;
	binder_node tmp_node;
	memset(&tmp_node, 0, sizeof(tmp_node));
	tmp_node.nodeid = node_id;
	{
		port_info pi;
		get_port_info(node_id.port, &pi);
		tmp_node.host = pi.team;
	}

	CHECK_ENTRY_STATE();

	if(state->stage == STAGE_INIT) {
#if TRACE_NOTIFICATIONS
		dprintf("binder(%d) -- %s: send notify to node %d-%d team %d to %d\n",
		        (int)find_thread(NULL), function_name,
		        (int)node_id.port, (int)node_id.token,
		        (int)current_team(), (int)tmp_node.host);
#endif
		if(tmp_node.host == current_team()) {
			/* only B_SOMETHING_CHANGED will be sent if original command was
			   not BINDER_NOTIFY_CALLBACK */
			err = add_reflection_command(reflection, REFLECT_CMD_NOTIFY,
			                             node_id.token);
#if TRACE_REFLECTION
			dprintf("binder(%d) -- %s: reflect notify\n",
			        (int)find_thread(NULL), function_name);
#endif
			state->stage = STAGE_NOTIFY_REFLECTED;
			goto reflect;
		}

		err = send_binder_request_name_and_data(&tmp_node, BINDER_REQUEST_NOTIFY,
		                                        ncs->name, &ncs->event,
		                                        sizeof(ncs->event),
		                                        &resources->reply_port);
		state->stage = STAGE_GET_REPLY;
	}
	if(state->stage == STAGE_GET_REPLY) {
		err = get_binder_reply_status(&tmp_node, resources->reply_port, reflection);
		if(err != B_OK)
			goto err;
		if(reflection->active)
			goto reflect;
	}
	if(state->stage == STAGE_NOTIFY_REFLECTED) {
		err = reflection->reflection_result;
		if(err < B_OK)
			goto err;
	}
	//char str[280];
	//sprintf(str,"%08x%s",(unsigned int)ncs->event,ncs->name);

	//dprintf("binder(%d) -- notify_callback: notify listeners on %016Lx\n",
	//        (int)find_thread(NULL), node->vnid.value);
	//dprintf("binder(%d) -- notify_callback: %s\n",
	//        (int)find_thread(NULL), str);

	//return notify_listener(B_ATTR_CHANGED, node->ns->nsid, 0, 0,
	//                       node->vnid.value, str);
	err = B_OK;
err:
	state->stage = STAGE_DONE;
	resources->reply_port = NULL;
reflect:
	if(reflection->active)
		err = B_OK;
	CHECK_EXIT_STATE();
	return err;
}

static status_t
notify_callback(binder_node *node, notify_callback_struct *ncs,
                state_notify *state, reflection_struct *reflection,
                transaction_resources *resources, bool lockEm)
{
	const char * const function_name = "notify_callback";
	enum {
		STAGE_BASE = STAGE_NOTIFY_BASE,
		STAGE_NOTIFY = STAGE_BASE,
		STAGE_COUNT
	};
	status_t err;
//	status_t err;
//	int i;
//	node_list *list;

	CHECK_ENTRY_STATE();

	if(node == NULL) {
		dprintf("binder(%d) -- notify_callback: node != NULL\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}

	if(state->stage == STAGE_INIT) {
		bool node_has_links;
		//dprintf("binder(%d) -- notify_callback: node %016Lx\n",
		//        (int)find_thread(NULL), node->vnid.value);
		if(resources->observers != NULL) {
			dprintf("binder(%d) -- notify_callback: observers != NULL in "
			        "STAGE_INIT\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err1;
		}
		resources->observers = get_observer_list(node, lockEm, &node_has_links);
		if(resources->observers == NULL) {
			err = B_NO_MEMORY;
			goto err1;
		}
		if(node_has_links)
			ncs->event = B_SOMETHING_CHANGED | (ncs->event & B_NAME_KNOWN);
		state->stage = STAGE_NOTIFY;
		state->index = 0;
	}
	if(state->stage == STAGE_NOTIFY) {
		while(state->index < resources->observers->count) {
			err = one_notify_callback(resources->observers->node_id[state->index],
			                          ncs, &state->one_notify, reflection,
			                          resources);
			if(reflection->active)
				goto reflect;
			if(err < B_OK)
				state->status = err;
			state->index++;
		}
	}
	err = state->status;
	state->stage = STAGE_DONE;
	state->status = 0;
	state->index = 0;
	free(resources->observers);
	resources->observers = NULL;
err1:
reflect:
	if(reflection->active)
		err = B_OK;
	CHECK_EXIT_STATE();
	return err;
}

static status_t
change_observing(binder_connection *c, binder_cmd *cmd)
{
	status_t err;
	binder_node *node;
	node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION, cmd->node_handle);
	if(node == NULL) {
		err = B_BAD_VALUE;
		goto err1;
	}
	if(cmd->command == BINDER_START_WATCHING)
		err = add_observer(c, node, cmd->data.nodeid);
	else
		err = remove_observer(c, node, cmd->data.nodeid);
	return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
err1:
	if(err < B_OK) {
		dprintf("binder(%d) -- change_observing: failed, %s\n",
		        (int)find_thread(NULL), strerror(err));
	}
	return err;
}

// #pragma mark -

#define PROP_DIR_UP		0x01
#define PROP_DIR_DOWN	0x02
#define PROP_DIR_IN		0x04

static status_t
get_property_once(binder_connection *c, binder_node *node,
                  get_property_struct *gps,
                  reflection_struct *reflection,
                  state_get_property_once *state,
                  transaction_resources *resources)
{
	const char * const function_name = "get_property_once";
	enum {
		STAGE_BASE = STAGE_GET_PROPERTY_ONCE_BASE,
		STAGE_GET_REPLY = STAGE_BASE,
		STAGE_CHECK_NEW_REPLY_BUFFER,
		STAGE_PROCESS_REPLY,
		STAGE_CONNECT_TO_NODE,
		STAGE_SEND_SYNC,
		STAGE_COUNT
	};
	
	status_t err = B_ERROR;

	CHECK_ENTRY_STATE();

	//dprintf("binder(%d) -- get_property_once: stage %d, name %s\n",
	//        (int)find_thread(NULL), state.stage, gps->name);

	if(state->stage == STAGE_INIT) {
		err = send_binder_request_name_and_data(node,
		                                        BINDER_REQUEST_GET_PROPERTY,
		                                        gps->name, gps->args,
		                                        gps->argsSize,
		                                        &resources->reply_port_2);
		if(err < B_OK) {
			dprintf("binder(%d) -- get_property_once: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err1;
		}
		state->stage = STAGE_GET_REPLY;
	}
	if(state->stage == STAGE_GET_REPLY) {
		binder_reply_get_property *reply;
		size_t reply_size;

		if(resources->reply_port_2 == NULL) {
			dprintf("binder(%d) -- get_property_once: ERROR "
			        "reply_port == NULL\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err1;
		}
	
		err = get_binder_reply_dynamic(node, resources->reply_port_2,
		                               reflection, (binder_reply_basic**)&reply,
		                               &reply_size);
		if(err < B_OK)
			goto err1;
		if(reflection->active)
			return B_OK;

		if(reply == NULL) {
			dprintf("binder(%d) -- get_property_once: ERROR reply == NULL, "
			        "size %d\n",
			        (int)find_thread(NULL), (int)reply_size);
			err = B_ERROR;
			goto err1;
		}
		resources->reply = reply;
		if(reply_size < sizeof(binder_reply_get_property_status)) {
			dprintf("binder(%d) -- get_property_once: bad reply size, "
			        "size %d\n",
			        (int)find_thread(NULL), (int)reply_size);
			err = B_ERROR;
			goto err1;
		}
		state->sync_port = reply->base.status.basic.sync_port;
		gps->result = reply->base.status.get_result;
	
		state->stage = STAGE_SEND_SYNC;
		if(!gps->result.error) {
			if(reply_size < sizeof(binder_reply_get_property_base)) {
				dprintf("binder(%d) -- get_property_once: bad reply "
				        "size, no space for reply->rlen\n",
				        (int)find_thread(NULL));
				gps->result.error = B_ERROR;
			}
			else if(reply_size <
			        sizeof(binder_reply_get_property_base) + reply->base.rlen) {
				dprintf("binder(%d) -- get_property_once: bad reply "
				        "size, reply size %d reply->rlen %d\n",
				        (int)find_thread(NULL), (int)reply_size,
				        (int)reply->base.rlen);
				gps->result.error = B_ERROR;
			}
			else {
				state->stage = STAGE_PROCESS_REPLY;
			}
		}
	}
	if(state->stage == STAGE_CHECK_NEW_REPLY_BUFFER) {
		err = reflection->reflection_result;
		if(err < B_OK)
			state->stage = STAGE_SEND_SYNC;
		else
			state->stage = STAGE_PROCESS_REPLY;
	}
	if(state->stage == STAGE_PROCESS_REPLY) {
		binder_reply_get_property *reply = resources->reply;
		if(reply->base.rlen <= gps->returnBufSize) {
			registry_property_type t;
			memcpy(gps->returnBuf, reply->rbuf, reply->base.rlen);
			t = ((registry_property_type)*((int32*)gps->returnBuf));
			if ((reply->base.rlen>=12) && (t == object))
				state->stage = STAGE_CONNECT_TO_NODE;
			else
				state->stage = STAGE_SEND_SYNC;
		} else {
			dprintf("binder: returnBuf too small, add reflection\n");
			gps->returnBufSize = reply->base.rlen;
			add_reflection_command(reflection,
			                       REFLECT_CMD_REALLOCATE_GET_BUFFER,
			                       node->nodeid.token);
			state->stage = STAGE_CHECK_NEW_REPLY_BUFFER;
			return B_OK;
		}
	}
	if(state->stage == STAGE_CONNECT_TO_NODE) {
		binder_connect_cmd connect_cmd;
		connect_cmd.node_id = *((binder_node_id*)(((uint8*)gps->returnBuf)+4+sizeof(dev_t)));
		connect_cmd.source_node_handle = 0;
		err = connect(c, &connect_cmd, reflection, &state->connect, resources);
		if(reflection->active)
			return B_OK;
		if(err >= B_OK)
			gps->return_node_handle = connect_cmd.return_node_handle;
		state->stage = STAGE_SEND_SYNC;
	}
	if(state->stage == STAGE_SEND_SYNC) {
		/* do not change err in this state */
		if(resources->reply != NULL) {
			free_binder_reply((binder_reply_basic *)resources->reply);
			resources->reply = NULL;
		}
		if(state->sync_port >= 0) {
#if TRACE_COMMUNICATION
			dprintf("binder(%d) -- get_property_once: send sync to %d\n",
			        (int)find_thread(NULL), (int)state->sync_port);
#endif
			unregister_reply_thread(state->sync_port, true);
			release_port(resources->reply_port_2);
			if(write_port_etc(state->sync_port, BINDER_SYNC_CODE, NULL, 0,
			                  B_TIMEOUT | B_CAN_INTERRUPT, 1000000) != B_OK) {
				dprintf("binder(%d) -- get_property_once: sync, "
				        "write port failed, port %d\n",
				        (int)find_thread(NULL), (int)state->sync_port);
			}
		}
	}
//err2:
	//release_port(vn, *inPort);
err1:
	//dprintf("binder(%d) -- get_property_once: done, %s\n",
	//        (int)find_thread(NULL), strerror(err));
	if(resources->reply != NULL) {
		free_binder_reply((binder_reply_basic *)resources->reply);
		resources->reply = NULL;
	}
	resources->reply_port_2 = NULL;
	state->stage = STAGE_DONE;
	state->sync_port = 0;

	CHECK_EXIT_STATE();
	return err;
}

#if 0
static status_t
get_property_data(binder_connection *c, binder_cmd *cmd)
{
	binder_reply_get_property *reply;
	reply = get_cookie_data(c, COOKIE_GET_PROPERTY_REPLY, cmd->get_data.cookie);
	if(reply == NULL)
		return B_BAD_ARG;
	if(reply->rlen != cmd->get_data.buffer_size) {
		dprintf("binder(%d) -- put_property_once: "
		        "ERROR reply_port == NULL\n", (int)find_thread(NULL));
	}
}
#endif

static status_t
get_property(binder_connection *c, binder_node *base_node, binder_cmd *cmd,
             transaction_state *full_state)
{
	const char * const function_name = "get_property";
	enum {
		STAGE_BASE = STAGE_GET_PROPERTY_BASE,
		STAGE_1 = STAGE_BASE,
		STAGE_2,
		STAGE_COUNT
	};
	state_get_property *state = &full_state->u.get_property;
	transaction_resources *resources = &full_state->resources;
	reflection_struct *reflection = &cmd->reflection;
	status_t err = B_OK;
	
	CHECK_ENTRY_STATE()
	
	if(base_node == NULL) {
		dprintf("binder(%d) -- get_property: base_node == NULL\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}

	//dprintf("binder(%d) -- get_property: enter state.stage == %d\n",
	//        (int)find_thread(NULL), state.stage);
	if(state->stage == STAGE_INIT) {
		if(resources->nodes != NULL) {
			dprintf("binder -- get_property: nodes != NULL in stage 0\n");
			return B_ERROR;
		}
		resources->nodes = get_call_order_node_list(base_node);
		if(resources->nodes == NULL)
			return B_NO_MEMORY;
		state->stage = STAGE_1;
		state->index = 0;
	}
	if(resources->nodes == NULL) {
		dprintf("binder -- get_property: nodes == NULL\n");
		return B_ERROR;
	}
	//dprintf("binder(%d) -- get_property: stage %d, index %d / %d\n",
	//        (int)find_thread(NULL), state.stage, state.index,
	//        state.nodes->count);
	
	while(state->index < resources->nodes->count) {
		if(state->stage == STAGE_1) {
			binder_node *node = resources->nodes->node[state->index];
			if(node->host == cmd->reflection.team) {
#if TRACE_REFLECTION
				dprintf("binder(%d) -- get_property: reflect get\n",
				        (int)find_thread(NULL));
#endif
				add_reflection_command(&cmd->reflection, REFLECT_CMD_GET,
				                       node->nodeid.token);
				state->stage = STAGE_2;
				return B_OK;
			}
			err = get_property_once(c, node, &cmd->data.get, &cmd->reflection,
			                        &state->get_property_once, resources);
			if(cmd->reflection.active)
				return B_OK;
			if(err < B_OK)
				cmd->data.get.result.error = err;
			state->stage = STAGE_2;
		}
		if(state->stage == STAGE_2) {
			if(!cmd->data.get.result.error) {
				err = B_OK;
				goto done;
			}
			state->index++;
			state->stage = STAGE_1;
		}
		if(state->stage != STAGE_1) {
			dprintf("binder -- get_property: bad state state.stage == %d\n",
			        state->stage);
			return B_ERROR;
		}
	}
	cmd->data.get.result.error = ENOENT;	
	cmd->data.get.result.resultCacheable = false;
	err = ENOENT;
done:
	free_node_list(resources->nodes);
	resources->nodes = NULL;
	state->stage = STAGE_DONE;
	state->index = 0;
	CHECK_EXIT_STATE();
	return err;
}

static int
put_property_once(binder_node *node, put_property_struct *pps,
                  reflection_struct *reflection,
                  state_put_property_once *state,
                  transaction_resources *resources)
{
	status_t err;

	if(state->stage == 0) {
		err = send_binder_request_name_and_data(node,
		                                        BINDER_REQUEST_PUT_PROPERTY,
		                                        pps->name, pps->value,
		                                        pps->valueLen,
		                                        &resources->reply_port);
		if(err < B_OK) {
			dprintf("binder(%d) -- put_property_once: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state->stage = 1;
	}
	if(state->stage == 1) {
		binder_reply_put_property reply;
		if(resources->reply_port == NULL) {
			dprintf("binder(%d) -- put_property_once: "
			        "ERROR reply_port == NULL\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err;
		}
		err = get_binder_reply(node, resources->reply_port, reflection,
		                       &reply.basic, sizeof(reply));
		if(err < B_OK)
			goto err;
		if(reflection->active)
			return B_OK;
		pps->result = reply.put_result;
	}
	err = B_OK;
err:
	resources->reply_port = NULL;
	state->stage = 0;
	return err;
}

static status_t
put_property(binder_node *base_node, binder_cmd *cmd,
             transaction_state *full_state)
{
	state_put_property *state = &full_state->u.put_property;
	transaction_resources *resources = &full_state->resources;

	status_t err = B_OK;

	if(base_node == NULL) {
		dprintf("binder(%d) -- put_property: base_node == NULL\n",
		        (int)find_thread(NULL));
		err = B_ERROR;
		goto err1;
	}

	if(state->stage == 0) {
		if(resources->nodes != NULL) {
			dprintf("binder -- put_property: nodes != NULL in stage 0\n");
			return B_ERROR;
		}
		resources->nodes = get_call_order_node_list(base_node);
		if(resources->nodes == NULL)
			return B_NO_MEMORY;
		state->stage = 1;
		state->index = 0;
	}
	while(state->index < resources->nodes->count) {
		if(state->stage == 1) {
			binder_node *node = resources->nodes->node[state->index];
			if(node->host == cmd->reflection.team) {
#if TRACE_REFLECTION
				dprintf("binder(%d) -- put_property: reflect put\n",
				        (int)find_thread(NULL));
#endif
				add_reflection_command(&cmd->reflection, REFLECT_CMD_PUT,
				                       node->nodeid.token);
				state->stage = 2;
				return B_OK;
			}
			err = put_property_once(node, &cmd->data.put, &cmd->reflection,
			                        &state->put_property_once, resources);
			if(cmd->reflection.active)
				return B_OK;
			if(err < B_OK) {
				cmd->data.put.result.error = err;
				cmd->data.put.result.fallthrough = true;
			}
			state->stage = 2;
		}
		if(state->stage == 2) {
			if(!cmd->data.put.result.error)
				goto done;
			if(!cmd->data.put.result.fallthrough)
				goto done;
			state->index++;
			state->stage = 1;
		}
	}
	cmd->data.put.result.error = ENOENT;	
	cmd->data.put.result.fallthrough = false;
done:
	err = B_OK;
	free_node_list(resources->nodes);
	resources->nodes = NULL;
	state->stage = 0;
err1:
	return err;
}

// #pragma mark -

status_t
allocate_single_cookie_name(single_cookie *sc, int32 len)
{
	char *newNextName;

	if(len <= 0) {
		dprintf("binder -- allocate_single_cookie_name: "
		        "tried to allocate space for empty string\n");
		return B_BAD_VALUE;
	}
	if(sc->nextNameLen >= len)
		return B_OK;

	newNextName = (char*)realloc(sc->nextName, len + 1);
	if(newNextName == NULL)
		return B_NO_MEMORY;

	sc->nextNameLen = len;
	sc->nextName = newNextName;
	return B_OK;
}

static status_t
get_next_entry_reply(single_cookie *sc, reflection_struct *reflection,
                     port_pool *reply_port)
{
	status_t err;
	binder_reply_next_entry *reply;
	size_t reply_size;

	err = get_binder_reply_dynamic(sc->node, reply_port, reflection,
	                               (binder_reply_basic**)&reply, &reply_size);
	if(err < B_OK) {
		dprintf("binder(%d) -- get_next_entry_reply: get_binder_reply "
		        "failed %s\n", (int)find_thread(NULL), strerror(err));
		goto err1;
	}
	if(reflection->active)
		return B_OK;

	if(reply_size < sizeof(binder_reply_status)) {
		err = B_ERROR;
		goto err2;
	}
	if(reply->base.status.result < B_OK) {
		err = reply->base.status.result;
		goto err2;
	}
	if(reply_size < sizeof(binder_reply_next_entry_base)) {
		dprintf("binder(%d) -- get_next_entry_reply: bad reply "
		        "size, reply size %d\n",
		        (int)find_thread(NULL), (int)reply_size);
		err = B_ERROR;
		goto err2;
	}
	if(reply_size != sizeof(binder_reply_next_entry_base) + reply->base.namelen) {
		dprintf("binder(%d) -- get_next_entry_reply: bad reply "
		        "size, reply size %d reply->namelen %d\n",
		        (int)find_thread(NULL), (int)reply_size,
		        (int)reply->base.namelen);
		err = B_ERROR;
		goto err2;
	}

	err = allocate_single_cookie_name(sc, reply->base.namelen);
	if(err < B_OK)
		goto err2;
	memcpy(sc->nextName, reply->name, reply->base.namelen);
	sc->nextName[reply->base.namelen] = 0;
err2:
	free_binder_reply((binder_reply_basic *)reply);
err1:
	return err;
}

static status_t
open_properties_single(single_cookie *c, reflection_struct *reflection,
                       state_open_properties_single *state,
                       transaction_resources *resources)
{
	enum {
		STAGE1 = STAGE_OPEN_PROPERTIES_SINGLE_BASE,
		STAGE2
	};
	status_t err;
	c->err = B_ERROR;
	c->nextNameLen = 0;
	
	if(state->stage == STAGE_INIT) {
		err = send_binder_request_no_arg(c->node,
		                                 BINDER_REQUEST_OPEN_PROPERTIES,
		                                 &resources->reply_port);
		if(err < B_OK) {
			dprintf("binder(%d) -- open_properties_single: send_binder_reques"
			        "t failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state->stage = STAGE1;
	}
	if(state->stage == STAGE1) {
		binder_reply_open_open_properties reply;
		err = get_binder_reply(c->node, resources->reply_port, reflection,
		                       &reply.status.basic, sizeof(reply));
		if(reflection->active)
			return B_OK;
		if(err < B_OK)
			goto err;
		resources->reply_port = NULL;
		if(reply.status.result < B_OK) {
			err = reply.status.result;
			goto err;
		}
		c->userCookie = reply.user_cookie;
		state->stage = STAGE2;
	}
	if(state->stage == STAGE2) {
		//err = ref_node(c->vn, reflection, state.ref_node);
		//if(err < B_OK)
		//	goto err;
		//if(reflection.active)
		//	return B_OK;
		c->opened = true;
		err = B_OK;
	}
	else {
		dprintf("binder(%d) -- open_properties_single: bad state, stage %d\n",
		        (int)find_thread(NULL), state->stage);
		err = B_ERROR;
	}
err:
	state->stage = STAGE_DONE;
	return err;
}

static status_t
get_next_entry(single_cookie *sc, reflection_struct *reflection,
               state_get_next_entry *state, transaction_resources *resources)
{
	const char * const function_name = "get_next_entry";
	enum {
		STAGE_BASE = STAGE_GET_NEXT_ENTRY_BASE,
		STAGE1 = STAGE_BASE,
		STAGE_COUNT
	};
	status_t err;

	CHECK_ENTRY_STATE();

	if(state->stage == STAGE_INIT) {
		if(!sc->opened) {
			err = B_ERROR;
			goto err;
		}
		err = send_binder_request_one_arg(sc->node,
		                                  BINDER_REQUEST_NEXT_PROPERTY,
		                                  (uint32)sc->userCookie,
		                                  &resources->reply_port);
		if(err < B_OK) {
			dprintf("binder(%d) -- get_next_entry: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state->stage = STAGE1;
	}
	//if(state->stage == STAGE1) {
		err = get_next_entry_reply(sc, reflection, resources->reply_port);
		if(err < B_OK)
			goto err;
		if(reflection->active)
			return B_OK;
		err = B_OK;
	//}
err:
	resources->reply_port = NULL;
	state->stage = STAGE_DONE;

	CHECK_EXIT_STATE();
	return err;
}

static status_t
new_property_iterator(binder_node *node, multi_cookie **mcp)
{
	int i;
	node_list *list;
	multi_cookie *mc;

	list = get_node_list(node, linkAugment, 0, true, true);
	if(list == NULL)
		return B_NO_MEMORY;

	mc = (multi_cookie*)
		malloc(sizeof(multi_cookie) + sizeof(single_cookie) * (list->count-1));
	if(mc == NULL)
		return B_NO_MEMORY;
	mc->count = list->count;
	mc->client = -1;
	mc->opened = false;
	for(i = 0; i < mc->count; i++) {
		mc->cookies[i].node = list->node[i];
		mc->cookies[i].opened = false;
		mc->cookies[i].nextName = NULL;
		mc->cookies[i].nextNameLen = 0;
		mc->cookies[i].err = B_OK;
		add_ref_node(list->node[i]);
	}
	free_node_list(list);
	*mcp = mc;
	return B_OK;
}

static int
open_properties(binder_connection *c, binder_cmd *cmd)
{
	status_t err;
	multi_cookie *cookie;

	{
		binder_node *node;

		node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION, cmd->node_handle);
		if(node == NULL) {
			err = B_BAD_VALUE;
			goto err1;
		}
		err = new_property_iterator(node, &cookie);
		return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
		if(err < B_OK)
			goto err1;
	}
	err = add_cookie(c, COOKIE_PROPERTY_ITERATOR, cookie);
	if(err < B_OK)
		goto err2;
	cmd->data.property_iterator.cookie = (uint32)cookie;
	return_cookie_data(c, COOKIE_PROPERTY_ITERATOR, cookie);
	return B_OK;

err2:
	cleanup_property_iterator(cookie);
err1:
	cmd->data.property_iterator.cookie = 0;
	return err;
}

void
cleanup_property_iterator(void *_cookie)
{
	status_t err;
	int i;
	multi_cookie *mc = (multi_cookie*)_cookie;

	for(i = 0; i < mc->count; i++) {
		single_cookie *sc = mc->cookies + i;
		if(sc->nextName)
			free(sc->nextName);
		if(sc->opened) {
			dprintf("binder -- cleanup_property_iterator: node at %d "
			        "still open\n", i);
			if(mc->client != sc->node->host) {
				port_pool *reply_port = NULL;
				err = send_binder_request_one_arg(sc->node,
				                           BINDER_REQUEST_CLOSE_PROPERTIES,
				                           (uint32)sc->userCookie, &reply_port);
				if(err < B_OK) {
					dprintf("binder(%d) -- cleanup_property_iterator: "
					        "send_binder_request failed, %s\n",
					        (int)find_thread(NULL), strerror(err));
				}
				else {
					reflection_struct reflection;
					do {
						reflection.active = false;
						err = get_binder_reply_status(sc->node, reply_port,
						                              &reflection);
						if(reflection.active) {
							dprintf("binder(%d) -- free_property_iterator_"
							        "cookie: ignored relection\n",
							        (int)find_thread(NULL));
							reflection.reflection_result = B_ERROR;
						}
					} while(reflection.active);
				}
			}
		}
		unref_node(sc->node);
	}
	free(mc);
}

static status_t
process_reflected_next_property(binder_cmd *cmd, single_cookie *sc)
{
	status_t err;
	int len;
	
	sc->err = cmd->reflection.reflection_result;
	if(sc->err < B_OK)
		return B_OK;
	if(!sc->opened) {
		sc->userCookie = cmd->data.property_iterator.local_cookie;
		sc->opened = true;
	}

	len = cmd->data.property_iterator.len;
	err = allocate_single_cookie_name(sc, len);
	if(err < B_OK)
		return err;
	strncpy(sc->nextName, cmd->data.property_iterator.name, len);
	sc->nextName[len] = '\0';
	//dprintf("binder -- process_reflected_next_property: got %s\n", sc->nextName);
	return B_OK;
}

static int
copystring(char *dest, const char *src, int maxlen)
{
	int len = 0;
	if(src == NULL)
		dprintf("binder(%d) -- copystring: src == NULL\n",
		        (int)find_thread(NULL));
	else
		while(len < maxlen && *src != '\0') {
			*dest++ = *src++;
			len++;
		}
	*dest++ = '\0';
	return len;
}

static status_t
open_properties_all(multi_cookie *mc, binder_cmd *cmd,
                    state_open_properties_all *state,
                    transaction_resources *resources)
{
	enum {
		STAGE_OPEN_SINGLE = STATE_OPEN_PROPERTIES_ALL_BASE,
		STAGE_OPEN_SINGLE_REFLECTED,
		STAGE_WAIT_FOR_SINGLE,
		STATE_OPEN_PROPERTIES_ALL_END
	};
	
	if(state->stage == STAGE_INIT) {
		if(state->index != 0) {
			dprintf("binder(%d) -- open_properties_all: bad state, "
			        "stage == 0, index == %d\n",
			        (int)find_thread(NULL), state->index);
			return B_ERROR;
		}
		state->stage = STAGE_OPEN_SINGLE;
	}
	else if(state->stage < STATE_OPEN_PROPERTIES_ALL_BASE ||
	        state->stage >= STATE_OPEN_PROPERTIES_ALL_END) {
		dprintf("binder(%d) -- open_properties_all: bad state, "
		        "stage == %d\n",
		        (int)find_thread(NULL), state->stage);
		return B_ERROR;
	}

	//dprintf("binder(%d) -- next_property: open %d-%d\n",
	//        (int)find_thread(NULL), state.stage_0.index, (int)mc->count);
	while(state->index < mc->count) {
		single_cookie *sc = mc->cookies + state->index;
		if(state->stage == STAGE_OPEN_SINGLE) {
			if(sc->node->host == cmd->reflection.team) {
				mc->client = cmd->reflection.team;
				cmd->data.property_iterator.local_cookie = sc->userCookie;
				add_reflection_command(&cmd->reflection,
				                       REFLECT_CMD_OPEN_PROPERTIES,
				                       sc->node->nodeid.token);
#if TRACE_REFLECTION
				dprintf("binder(%d) -- next_property: reflect "
				        "open_properties\n", (int)find_thread(NULL));
#endif
				state->stage = STAGE_OPEN_SINGLE_REFLECTED;
				return B_OK;
			}
			sc->err = open_properties_single(sc, &cmd->reflection,
			                                 &state->u.open_properties_single,
                                             resources);
			if(cmd->reflection.active)
				return B_OK;
			state->stage = STAGE_WAIT_FOR_SINGLE;
		}
		if(state->stage == STAGE_WAIT_FOR_SINGLE) {
			sc->err = get_next_entry(sc, &cmd->reflection,
			                         &state->u.get_next_entry, resources);
			if(cmd->reflection.active)
				return B_OK;
		}
		if(state->stage == STAGE_OPEN_SINGLE_REFLECTED) {
			process_reflected_next_property(cmd, sc);
		}
		state->stage = STAGE_OPEN_SINGLE;
		state->index++;
	}
	mc->opened = true;
	state->index = 0;
	state->stage = STAGE_DONE;
	return B_OK;
}

static status_t
next_property(binder_connection *c, binder_cmd *cmd,
              transaction_state *full_state)
{
	const char * const function_name = "next_property";
	state_next_property *state = &full_state->u.next_property;
	transaction_resources *resources = &full_state->resources;
	reflection_struct *reflection = &cmd->reflection;
	multi_cookie *mc;
	enum {
		STAGE_BASE = STATE_NEXT_PROPERTY_BASE,
		STAGE_OPEN = STAGE_BASE,
		STAGE_START_FIND,
		STAGE_FIND,
		STAGE_RETURN_NEXT,
		STAGE_REFLECT_NEXT,
		STAGE_COUNT,

		SUB_STAGE_GET_NEXT,
		SUB_STAGE_REFLECT_GET_NEXT,
		SUB_STAGE_DONE
	};
	status_t err = B_ERROR;
	
	CHECK_ENTRY_STATE();
	
	//multi_cookie *mc = cookie->get_property_iterator();
	mc = get_cookie_data(c, COOKIE_PROPERTY_ITERATOR,
	                     cmd->data.property_iterator.cookie);
	if(mc == NULL) {
		dprintf("binder -- next_property: multi_cookie == NULL\n");
		return B_ERROR;
	}
	
	//dprintf("binder(%d) -- next_property: node count %d stage %d\n",
	//        (int)find_thread(NULL), (int)mc->count, state.stage);

	if(state->stage == STAGE_INIT) {
		if(mc->count <= 0) {
			err = ENOENT;
			goto err;
		}
		if(!mc->opened)
			state->stage = STAGE_OPEN;
		else
			state->stage = STAGE_START_FIND;
	}
	if(state->stage == STAGE_OPEN) {
		err = open_properties_all(mc, cmd, &state->u.open_properties_all,
		                          resources);
		if(err < B_OK)
			goto err;
		if(cmd->reflection.active)
			return B_OK;
		state->stage = STAGE_START_FIND;
	}
	if(state->stage == STAGE_START_FIND) {
		state->u.stage_find.index = 0;
		state->u.stage_find.next_index = -1;
		state->stage = STAGE_FIND;
	}
	if(state->stage == STAGE_FIND) {
		single_cookie *sc;
		single_cookie *sc_next = NULL;
		if(state->u.stage_find.next_index >= 0)
			sc_next = mc->cookies + state->u.stage_find.next_index;
		sc = mc->cookies + state->u.stage_find.index;
		while(state->u.stage_find.index < mc->count) {
			if(state->u.stage_find.stage == STAGE_INIT) {
				state->u.stage_find.stage = SUB_STAGE_DONE;
				if(sc->err == B_OK) {
					if(state->u.stage_find.next_index < 0) {
						state->u.stage_find.next_index =
							state->u.stage_find.index;
						sc_next = sc;
					}
					else {
						int32 cmp = strcmp(sc_next->nextName, sc->nextName);
						if(cmp == 0) {
							state->u.stage_find.stage = SUB_STAGE_GET_NEXT;
						}
						else if(cmp > 0) {
							state->u.stage_find.next_index =
								state->u.stage_find.index;
							sc_next = sc;
						}
					}
				}
			}
			if(state->u.stage_find.stage == SUB_STAGE_GET_NEXT) {
				if(sc->node->host == cmd->reflection.team) {
					mc->client = cmd->reflection.team;
					cmd->data.property_iterator.local_cookie =
						sc->userCookie;
					add_reflection_command(&cmd->reflection,
					                       REFLECT_CMD_NEXT_PROPERTY,
					                       sc->node->nodeid.token);
#if TRACE_REFLECTION
					dprintf("binder(%d) -- next_property: reflect "
					        "next_property\n", (int)find_thread(NULL));
#endif
					state->u.stage_find.stage = SUB_STAGE_REFLECT_GET_NEXT;
					return B_OK;
				}
				sc->err = get_next_entry(sc, &cmd->reflection,
				                         &state->u.stage_find.get_next_entry,
				                         resources);
				if(cmd->reflection.active) {
					err = B_OK;
					goto reflect;
				}
				state->u.stage_find.stage = STAGE_DONE;
			}
			if(state->u.stage_find.stage == SUB_STAGE_REFLECT_GET_NEXT) {
				process_reflected_next_property(cmd, sc);
				state->u.stage_find.stage = STAGE_DONE;
			}
			if(state->u.stage_find.stage == SUB_STAGE_DONE) {
				state->u.stage_find.stage = STAGE_DONE;
				state->u.stage_find.index++;
				sc++;
			}
			if(state->u.stage_find.stage != STAGE_DONE) {
				dprintf("binder(%d) -- next_property: bad state, "
				        "state.stage_find.stage == %d\n",
				        (int)find_thread(NULL), state->u.stage_find.stage);
				err = B_ERROR;
				goto err;
			}
		}
		state->u.stage_find.index = 0;
		state->stage = STAGE_RETURN_NEXT;
	}
	if(state->stage == STAGE_RETURN_NEXT) {
		if(state->u.stage_find.next_index >= 0) {
			single_cookie *sc = mc->cookies + state->u.stage_find.next_index;
			if(state->u.stage_find.next_index >= mc->count) {
				dprintf("binder(%d) -- next_property: next_index bad, %d\n",
				        (int)find_thread(NULL), state->u.stage_find.next_index);
				err = B_ERROR;
				goto err;
			}
			//strncpy(cmd->data.property_iterator.name,
			copystring(cmd->data.property_iterator.name, sc->nextName,
			           sizeof(cmd->data.property_iterator.name) - 1);
			//dprintf("binder(%d) -- next_property: found name \"%s\" at %d\n",
			//        (int)find_thread(NULL), cmd->data.property_iterator.name,
			//        state.stage_find.next_index);
	
			if(sc->node->host == cmd->reflection.team) {
				cmd->data.property_iterator.local_cookie = sc->userCookie;
				add_reflection_command(&cmd->reflection,
				                       REFLECT_CMD_NEXT_PROPERTY,
				                       sc->node->nodeid.token);
#if TRACE_REFLECTION
					dprintf("binder(%d) -- next_property: reflect "
					        "next_property\n", (int)find_thread(NULL));
#endif
				state->stage = STAGE_REFLECT_NEXT;
				err = B_OK;
				goto reflect;
			}

			sc->err = get_next_entry(sc, &cmd->reflection,
			                         &state->u.stage_find.get_next_entry,
			                         resources);
			//dprintf("binder(%d) -- next_property: get_next_entry, %s\n",
			//        (int)find_thread(NULL), strerror(sc->err));
			if(cmd->reflection.active) {
				err = B_OK;
				goto reflect;
			}
			err = B_OK;
		}
		else {
			err = ENOENT;		
		}
		state->u.stage_find.next_index = 0;
	}
	if(state->stage == STAGE_REFLECT_NEXT) {
		single_cookie *sc = mc->cookies + state->u.stage_find.next_index;
		if(state->u.stage_find.next_index >= mc->count) {
			dprintf("binder(%d) -- next_property: next_index bad, %d\n",
			        (int)find_thread(NULL), state->u.stage_find.next_index);
			err = B_ERROR;
			goto err;
		}
		if(sc->nextName == NULL)
			dprintf("binder(%d) -- next_property: stage 3 index %d, "
			        "nextName = NULL\n",
			        (int)find_thread(NULL), state->u.stage_find.next_index);
		else {
			char *tmp_str = strdup(sc->nextName);
			if(tmp_str == NULL) {
				dprintf("binder(%d) -- next_property: could not allocate "
				        "string\n", (int)find_thread(NULL));
				state->u.stage_find.next_index = 0;
				err = B_NO_MEMORY;
				goto err;
			}
			process_reflected_next_property(cmd, sc);
			copystring(cmd->data.property_iterator.name, tmp_str,
			           sizeof(cmd->data.property_iterator.name) - 1);
			free(tmp_str);
		}
		state->stage = STAGE_DONE;
		state->u.stage_find.next_index = 0;
		err = B_OK;
	}
err:
	state->stage = STAGE_DONE;
reflect:
	return_cookie_data(c, COOKIE_PROPERTY_ITERATOR, mc);
	CHECK_EXIT_STATE();
	return err;
}

static status_t
close_properties(binder_connection *c, binder_cmd *cmd,
                 transaction_state *full_state)
{
	state_close_properties *state = &full_state->u.close_properties;
	transaction_resources *resources = &full_state->resources;
	multi_cookie *mc;

	mc = get_cookie_data(c, COOKIE_PROPERTY_ITERATOR,
	                     cmd->data.property_iterator.cookie);
	if(mc == NULL) {
		dprintf("binder -- close_properties: multi_cookie == NULL\n");
		return B_ERROR;
	}

	while(state->index < mc->count) {
		single_cookie *sc = mc->cookies + state->index;
		if(sc->opened) {
			if(state->stage == 0) {
				if(mc->client == sc->node->host) {
					cmd->data.property_iterator.local_cookie = sc->userCookie;
					add_reflection_command(&cmd->reflection,
					                       REFLECT_CMD_CLOSE_PROPERTIES,
					                       sc->node->nodeid.token);
#if TRACE_REFLECTION
					dprintf("binder(%d) -- close_properties: reflect\n",
					        (int)find_thread(NULL));
#endif
					state->stage = 2;
					goto reflect;
				}
				else {
					status_t err;
					err = send_binder_request_one_arg(sc->node,
					                            BINDER_REQUEST_CLOSE_PROPERTIES,
					                            (uint32)sc->userCookie,
					                            &resources->reply_port);
					if(err < B_OK) {
						dprintf("binder(%d) -- close_properties: "
						        "send_binder_request failed, %s\n",
						        (int)find_thread(NULL), strerror(err));
					}
					else
						state->stage = 1;
				}
			}
			if(state->stage == 1) {
				status_t err;
				err = get_binder_reply_status(sc->node, resources->reply_port,
				                              &cmd->reflection);
				if(cmd->reflection.active)
					goto reflect;
				resources->reply_port = NULL;
				if(err == B_OK)
					sc->opened = false;
			}
			if(state->stage == 2) {
				sc->opened = false;
			}
			state->stage = 0;
			//unref_node(sc->vn);
		}
		state->index++;
	}
	state->index = 0;
	//sys_close(false, mc->fd);
	cmd->data.property_iterator.result = B_OK;
	remove_cookie(c, COOKIE_PROPERTY_ITERATOR, mc);
reflect:
	return_cookie_data(c, COOKIE_PROPERTY_ITERATOR, mc);
	return B_OK;
}

// #pragma mark -

static transaction_state *
start_transaction(binder_connection *c, reflection_struct *reflection,
                  int command_type)
{
	transaction_state *trans;
	if(reflection->cookie == 0) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		reflection->team = ti.team;
		trans = new_transaction_state(command_type);
	} else {
		trans = get_cookie_data(c, COOKIE_TRANSACTION_STATE, reflection->cookie);
	}
	
	return trans;
}

static status_t
reflect_transaction(binder_connection *c, reflection_struct *reflection,
                    transaction_state *trans)
{
	status_t err;

	if(reflection->cookie != 0) {
		return_cookie_data(c, COOKIE_TRANSACTION_STATE, trans);
		return B_OK;
	}
	err = add_cookie(c, COOKIE_TRANSACTION_STATE, trans);
	if(err < 0)
		return err;
	reflection->cookie = (uint32)trans;
	return_cookie_data(c, COOKIE_TRANSACTION_STATE, trans);
	return B_OK;
}

static void
cleanup_transaction(binder_connection *c, reflection_struct *reflection,
                    transaction_state *trans)
{
	if(reflection->cookie != 0) {
		remove_cookie(c, COOKIE_TRANSACTION_STATE, trans);
		return_cookie_data(c, COOKIE_TRANSACTION_STATE, trans);
	}
	else
		cleanup_transaction_state(trans);
}

// #pragma mark -

static int
do_binder_command(binder_connection *c, binder_cmd *cmd)
{
	status_t err;
	transaction_state *trans;

	trans = start_transaction(c, &cmd->reflection, cmd->command);
	if(trans == NULL) {
		dprintf("binder(%d) -- do_binder_command: trans == NULL\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	if(trans->command_type != cmd->command) {
		dprintf("binder(%d) -- do_binder_command: transaction command does "
		        "not match current command\n", (int)find_thread(NULL));
	}
	switch (cmd->command) {
		case BINDER_SET_ROOT_NODE_ID:
			err = set_root_node(cmd);
			break;
		
		case BINDER_GET_ROOT_NODE_ID:
			err = get_root_node(cmd);
			break;
			
		case BINDER_GET_NODE_ID:
			err = get_node_id(c, cmd);
			break;
		
		case BINDER_NOTIFY_CALLBACK: {
			binder_node *node = get_cookie_data(c, COOKIE_HOST_CONNECTION,
			                                    cmd->node_handle);
			err = notify_callback(node, &cmd->data.notify, &trans->u.notify,
			                      &cmd->reflection, &trans->resources, true);
			return_cookie_data(c, COOKIE_HOST_CONNECTION, node);
		} break;

		case BINDER_OPEN_PROPERTIES:
			err = open_properties(c, cmd);
			break;

		case BINDER_CONNECT:
			err = connect(c, &cmd->data.connect, &cmd->reflection,
			              &trans->u.connect, &trans->resources);
			break;
		
		case BINDER_DISCONNECT: {
			binder_node *node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
			                                    cmd->node_handle);
			if(node == NULL) {
				dprintf("binder(%d) -- do_binder_command: "
				        "disconnect 0x%08x, no such handle\n",
				        (int)find_thread(NULL), (int)cmd->node_handle);
				err = B_BAD_VALUE;
			}
			else {
#if TRACE_CONNECTION
				dprintf("binder(%d) -- do_binder_command: disconnect node "
				        "%d-%d / handle 0x%08x\n",
				        (int)find_thread(NULL), (int)node->nodeid.port,
				        (int)node->nodeid.token, (int)cmd->node_handle);
#endif
				err = remove_cookie(c, COOKIE_CLIENT_CONNECTION, node);
				return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
			}
		} break;

		case BINDER_START_HOSTING:
			err = start_hosting(c, cmd, &trans->u.start_hosting);
			break;
		
		case BINDER_STOP_HOSTING:
			err = stop_hosting(c, cmd);
			break;
			
		case BINDER_STACK:
			err = link_nodes(c, cmd, trans, linkFilter | linkAugment);
			break;

		case BINDER_INHERIT:
			err = link_nodes(c, cmd, trans, linkInherit | linkAugment);
			break;

		case BINDER_BREAK_LINKS: {
			binder_node *node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
			                                    cmd->node_handle);
			err = break_links(node, cmd, trans);
			return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
		} break;

		case BINDER_GET_PROPERTY: {
			binder_node *node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
			                                    cmd->node_handle);
			err = get_property(c, node, cmd, trans);
			return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
		} break;

		case BINDER_PUT_PROPERTY: {
			binder_node *node = get_cookie_data(c, COOKIE_CLIENT_CONNECTION,
			                                    cmd->node_handle);
			err = put_property(node, cmd, trans);
			return_cookie_data(c, COOKIE_CLIENT_CONNECTION, node);
		} break;

		case BINDER_NEXT_PROPERTY:
			err = next_property(c, cmd, trans);
			break;

		case BINDER_CLOSE_PROPERTIES:
			err = close_properties(c, cmd, trans);
			break;
		
		case BINDER_START_WATCHING:
		case BINDER_STOP_WATCHING:
			err = change_observing(c, cmd);
			break;

		default:
			dprintf("binder(%d) -- do_binder_command: invalid command, %d\n",
			        (int)find_thread(NULL), cmd->command);
			err = B_ERROR;
	}
	if(cmd->reflection.active) {
		err = reflect_transaction(c, &cmd->reflection, trans);
		return err;
	}
	cleanup_transaction(c, &cmd->reflection, trans);
	return err;
}

const char * ioctl_name[] = {
	"BINDER_SET_ROOT_NODE_ID",
	"BINDER_GET_ROOT_NODE_ID",
	"BINDER_GET_NODE_ID",
	"BINDER_START_HOSTING",
	"BINDER_STOP_HOSTING",
	"BINDER_CONNECT",
	"BINDER_DISCONNECT",
	"BINDER_STACK",
	"BINDER_INHERIT",
	"BINDER_BREAK_LINKS",
	"BINDER_NOTIFY_CALLBACK",
	"BINDER_GET_PROPERTY",
	"BINDER_PUT_PROPERTY",
	"BINDER_OPEN_PROPERTIES",
	"BINDER_NEXT_PROPERTY",
	"BINDER_CLOSE_PROPERTIES",
	"BINDER_START_WATCHING",
	"BINDER_STOP_WATCHING"
};

status_t
binder_control(void *cookie, uint32 precmd, void *buf, size_t len)
{
	binder_connection *c = cookie;
	len = len;
	
	switch (precmd) {
		case BINDER_GET_API_VERSION:
			return CURRENT_BINDER_API_VERSION;

		case BINDER_CMD: {
			status_t err;
			binder_cmd *cmd = (binder_cmd*)buf;
			cmd->reflection.active = false;
#if TRACE_COMMANDS
			if (cmd->command < _BINDER_COMMAND_COUNT)
				dprintf("binder(%d) -- ioctl: %s%s\n",
				        (int)find_thread(NULL), ioctl_name[cmd->command],
				        cmd->reflection.cookie != 0 ?
				        (cmd->reflection.reflection_result >= B_OK ?
				         " (sucessful reflection)" : " (failed reflection)" ) :
				        "");
	        else
	        	dprintf("binder(%d) -- ioctl: unknown command %d\n",
				        (int)find_thread(NULL), (int)cmd->command);
#endif
			err = do_binder_command(c, cmd);
#if TRACE_COMMANDS
			if (cmd->command < _BINDER_COMMAND_COUNT)
				dprintf("binder(%d) -- ioctl: %s done%s, %s\n",
				       (int)find_thread(NULL), ioctl_name[cmd->command],
				       cmd->reflection.active ? " (reflect)" : "",
				       strerror(err));
#endif
			return err;
		}

		case BINDER_REGISTER_REPLY_THREAD:
			return register_reply_thread((port_id)buf);

		case BINDER_UNREGISTER_REPLY_THREAD:
			return unregister_reply_thread((port_id)buf, false);
		
		case BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT:
			return optain_sync_reply_port((port_id)buf);
		
		case BINDER_FREE_SYNC_REPLY_PORT:
			return release_port_by_id((port_id)buf, find_thread(NULL));

		case BINDER_WAIT_FOR_ROOT_NODE: {
			status_t err;
			dprintf("binder: BINDER_WAIT_FOR_ROOT_NODE\n");
			if(root_node_id.port >= 0)
				return B_OK;
			dprintf("binder: BINDER_WAIT_FOR_ROOT_NODE acquire sem...\n");
			err = acquire_sem_etc(root_node_set_sem, 1, B_CAN_INTERRUPT, 0);
			dprintf("binder: BINDER_WAIT_FOR_ROOT_NODE acquire sem done\n");
			if(root_node_id.port >= 0)
				return B_OK;
			return err;
		}

		default:
			return B_BAD_VALUE;
	}
}

