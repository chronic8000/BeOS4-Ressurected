#ifndef TRANSACTION_STATE_H
#define TRANSACTION_STATE_H

#include <support/SupportDefs.h>
#include "binder_command.h"

enum single_transaction_stage {
	STAGE_INIT = 0,
	STAGE_DONE = 0,
	STATE_START_HOSTING_BASE            = 10000,
	STATE_OPEN_PROPERTIES_ALL_BASE      = 21000,
	STAGE_OPEN_PROPERTIES_SINGLE_BASE   = 22000,
	STAGE_GET_NEXT_ENTRY_BASE           = 23000,
	STATE_NEXT_PROPERTY_BASE            = 24000,
	STAGE_GET_PROPERTY_BASE             = 30000,
	STAGE_GET_PROPERTY_ONCE_BASE        = 31000,
	STAGE_REF_NODE_BASE                 = 40000,
	STAGE_NOTIFY_BASE                   = 50000,
	STAGE_ONE_NOTIFY_BASE               = 51000,
	STAGE_LINK_NODES_BASE               = 60000,
	STAGE_DESTROY_LINK_BASE             = 61000,
	STAGE_BREAK_LINKS_BASE              = 62000
};

struct transaction_resources;
struct reflection_struct;

status_t check_state(const char *function_name, bool entry, int stage,
                     int first_valid_stage,
                     int last_valid_stage, void *state, size_t state_size,
                     struct transaction_resources *resources,
                     struct reflection_struct *reflection, status_t err);

#define CHECK_STATE(entry) { \
	status_t state_err; \
	state_err = check_state(function_name, entry, state->stage, STAGE_BASE, \
	                  STAGE_COUNT, state, sizeof(*state), resources, \
	                  reflection, entry ? B_ERROR : err); \
	if(state_err != B_OK) { \
		kernel_debugger("binder state error"); \
		err = state_err; \
		goto state_err; \
	} \
}

#define CHECK_ENTRY_STATE() CHECK_STATE(true)
#define CHECK_EXIT_STATE() CHECK_STATE(false);state_err:

struct port_pool;
struct node_list;
struct vnode;

/* all members must be 0 on non reflected return */

typedef struct {
	int         stage;
} state_ref_node;

typedef struct {
	int         stage;
} state_update_redirect;

typedef struct {
	int         stage;
} state_one_notify;

typedef struct {
	int                         stage;
	int                         index;
	status_t                    status;
	state_one_notify            one_notify;
} state_notify;

typedef struct {
	int                         stage;
	state_ref_node              ref_node;
} state_create_link;

typedef struct {
	int                         stage;
	union {
		state_create_link       create_link;
		state_update_redirect   update_redirect;
		state_notify            notify;
	} u;
} state_link_nodes;

typedef struct {
	int                         stage;
	int                         index;
	state_one_notify            one_notify;
} state_destroy_link;

typedef struct {
	int                         stage;
	union {
		state_update_redirect   update_redirect;
		state_destroy_link      destroy_link;
	} u;
} state_break_links;

typedef struct {
	int              stage;
} state_open_properties_single;

typedef struct {
	int          stage;
} state_get_next_entry;

typedef struct {
	int stage;
	int index;
	union {
		state_get_next_entry            get_next_entry;
		state_open_properties_single	open_properties_single;
	} u;
} state_open_properties_all;

typedef struct {
	int stage;
	union {
		state_open_properties_all open_properties_all;
		struct {
			int stage;
			int index;
			int next_index;
			state_get_next_entry            get_next_entry;
		} stage_find;
	} u;
} state_next_property;

typedef struct {
	int          index;
	int          stage;
} state_close_properties;

typedef struct {
	state_ref_node   ref_node;
} state_connect;

typedef struct {
	int              stage;
	port_id          sync_port;
	state_connect    connect;
} state_get_property_once;

typedef struct {
	int              stage;
} state_put_property_once;

typedef struct {
	int                         stage;
	int                         index;
	state_get_property_once     get_property_once;
} state_get_property;

typedef struct {
	int                         stage;
	int                         index;
	state_put_property_once     put_property_once;
} state_put_property;

typedef struct {
	int              stage;
} state_start_hosting;

typedef struct transaction_resources {
	struct port_pool           *reply_port;
	struct port_pool           *reply_port_2; /* get_property need to hold the reply port */
	struct node_list           *nodes;
	struct observer_list       *observers;
	struct binder_node         *node;	/* COOKIE_CLIENT_CONNECTION */
	binder_reply_get_property  *reply;
} transaction_resources;

typedef struct {
	binder_cmd_type           command_type;
	transaction_resources     resources;
	union {
		state_connect            connect;
		state_start_hosting      start_hosting;
		state_link_nodes         link_nodes;
		state_break_links        break_links;
		state_next_property      next_property;
		state_close_properties   close_properties;
		state_get_property       get_property;
		state_put_property       put_property;
		state_notify             notify;
	} u;
} transaction_state;

#endif
