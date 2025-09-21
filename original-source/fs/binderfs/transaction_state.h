#ifndef TRANSACTION_STATE_H
#define TRANSACTION_STATE_H

enum single_transaction_stage {
	STAGE_INIT = 0,
	STAGE_DONE = 0,
	STATE_START_HOSTING_BASE            = 10000,
	STATE_OPEN_PROPERTIES_ALL_BASE      = 21000,
	STAGE_OPEN_PROPERTIES_SINGLE_BASE   = 22000,
	STAGE_GET_NEXT_ENTRY_BASE           = 23000,
	STATE_NEXT_PROPERTY_BASE            = 24000
};

struct port_pool;
struct node_list;
struct vnode;

/* all members must be 0 on non reflected return */

struct state_ref_node {
	int         stage;
	//port_pool  *reply_port;
};

struct state_update_redirect {
	int         stage;
	//port_pool  *reply_port;
};

struct state_create_link {
	int                         stage;
	state_ref_node              ref_node;
};

struct state_link_nodes {
	int                         stage;
	//vnode                      *to_node;
	union {
		state_create_link       create_link;
		state_update_redirect   update_redirect;
	};
};

struct state_break_links {
	int                       stage;
	state_update_redirect     update_redirect;
};

struct state_open_properties_single {
	int              stage;
	//port_pool       *reply_port;
	//union {
	//	state_ref_node   ref_node;
	//};
};

struct state_get_next_entry {
	int          stage;
	//port_pool   *reply_port;
};

struct state_open_properties_all {
	int stage;
	int index;
	union {
		state_get_next_entry            get_next_entry;
		state_open_properties_single	open_properties_single;
	};
};

struct state_next_property {
	int stage;
	union {
		state_open_properties_all open_properties_all;
		struct {
			int stage;
			int index;
			int next_index;
			state_get_next_entry            get_next_entry;
		} stage_find;
	};
};

struct state_close_properties {
	int          index;
	int          stage;
	//port_pool   *reply_port;
};

struct state_connect {
	state_ref_node   ref_node;
};

struct state_get_property_once {
	int              stage;
	//port_pool       *reply_port;
	port_id          sync_port;
	state_connect    connect;
};

struct state_put_property_once {
	int              stage;
	//port_pool       *reply_port;
};

struct state_get_property {
	int                         stage;
	int                         index;
	//node_list                  *nodes;
	state_get_property_once     get_property_once;
};

struct state_put_property {
	int                         stage;
	int                         index;
	//node_list                  *nodes;
	state_put_property_once     put_property_once;
};

struct state_start_hosting {
	int              stage;
};

struct transaction_resources {
	port_pool    *reply_port;
	port_pool    *reply_port_2; /* get_property need to hold the reply port */
	node_list    *nodes;
	vnode        *node;
};

struct transaction_state {
	int                       command_type;
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
	};
};

#endif

