#ifndef BINDER_NODE_H
#define BINDER_NODE_H

#include <kernel/OS.h>

#include "binder_driver.h"

struct node_link;

/* binder_node flags */
enum {
	vnodeRedirect	= 0x01,
	vnodeHosted		= 0x02,
	vnodeDied		= 0x04
};

typedef struct binder_node_observer {
	binder_node_id               client_id;
	struct binder_node_observer  *next;
	struct binder_node_observer  *prev;
} binder_node_observer;

typedef struct binder_node {
	struct binder_node    *next;
	binder_node_observer   observers;
	int32                  primary_ref_count;
	int32                  secondary_ref_count;
	binder_node_id         nodeid;
	struct node_link      *links;
	uint16                 link_count;
	uint16                 flags;
	team_id                host;
	

	struct binder_node  *tmp_ptr; /* NULL when linkLock free */
} binder_node;

typedef struct node_list {
	int                 count;
	binder_node        *node[1];
} node_list;

typedef struct observer_list {
	int                 count;
	binder_node_id      node_id[1];
} observer_list;

/* find node and get secondary reference */
status_t get_binder_node(binder_node_id nodeid, binder_node **node);
/* get secondary reference */
void get_ref_binder_node(binder_node *node);
/* release secondary reference */
status_t return_binder_node(binder_node *node);

/* create node and get secondary reference */
status_t new_binder_node(binder_node_id nodeid, binder_node **node);

status_t init_binder_node();
void uninit_binder_node();

int kdebug_dump_binder_nodes(bool start);
void kdebug_dump_binder_node(uint32 addr, uint32 *next_addr);

#endif

