/* todo: add hash table for faster lookup */

#include "binder_node.h"
#include <drivers/KernelExport.h>
#include <malloc.h>
#include <lock.h>

// #pragma mark Globals

static lock                binder_nodes_lock;
static binder_node        *binder_nodes;

// #pragma mark -

void
get_ref_binder_node(binder_node *node)
{
	atomic_add(&node->secondary_ref_count, 1);
}


status_t 
get_binder_node(binder_node_id nodeid, binder_node **matched_node)
{
	status_t err = B_NAME_NOT_FOUND;
	binder_node *node;
	bool found_unhosted_node = false;

	LOCK(binder_nodes_lock);
	for(node = binder_nodes; node != NULL; node = node->next) {
		if(node->nodeid.port > nodeid.port)
			break;
		if(node->nodeid.port == nodeid.port) {
			if(node->nodeid.token == nodeid.token) {
				if(node->flags & vnodeHosted) {
					*matched_node = node;
					atomic_add(&node->secondary_ref_count, 1);
					err = B_OK;
					break;
				}
				else
					found_unhosted_node = true;
			}
			else if(node->nodeid.token > nodeid.token)
				break;
		}
	}
	UNLOCK(binder_nodes_lock);
	if(err != B_OK) {
		dprintf("binder(%d) -- get_binder_node: node %d-%d %s\n",
		        (int)find_thread(NULL), (int)nodeid.port, (int)nodeid.token,
		        found_unhosted_node ? "is no longer hosted" : "not found");
	}
	return err;
}

status_t
return_binder_node(binder_node *node)
{
	int32 old;
	status_t err;

	old = atomic_add(&node->secondary_ref_count, -1);
	if(old > 1)
		return B_OK;

	LOCK(binder_nodes_lock);
	if(node->primary_ref_count != 0) {
		dprintf("binder(%d) -- return_binder_node: error, node %d-%d, "
		        "primary_ref_count %d\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, (int)node->primary_ref_count);
		err = B_ERROR;
		goto err;
	}
	if(node->tmp_ptr != NULL) {
		dprintf("binder(%d) -- return_binder_node: error, node %d-%d, "
		        "tmp_ptr == %p\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token, node->tmp_ptr);
	}
	if(node == binder_nodes)
		binder_nodes = node->next;
	else {
		binder_node *prev_node = binder_nodes;
		while(prev_node != NULL && prev_node->next != node)
			prev_node = prev_node->next;
		if(prev_node == NULL) {
			dprintf("binder(%d) -- return_binder_node: error, node %d-%d, "
			        "not found\n",
			        (int)find_thread(NULL), (int)node->nodeid.port,
			        (int)node->nodeid.token);
			err = B_ERROR;
			goto err;
		}
		prev_node->next = node->next;
	}
	free(node);
	err = B_OK;
err:
	UNLOCK(binder_nodes_lock);
	return err;
}

status_t 
new_binder_node(binder_node_id nodeid, binder_node **new_node)
{
	status_t err;
	binder_node *next_node = binder_nodes;
	binder_node *prev_node = NULL;

	LOCK(binder_nodes_lock);
	while(next_node != NULL) {
		if(next_node->nodeid.port > nodeid.port)
			break;
		if(next_node->nodeid.port == nodeid.port) {
			if(next_node->nodeid.token == nodeid.token &&
			   !(next_node->flags & vnodeDied)) {
				dprintf("binder(%d) -- new_binder_node: node %d-%d aready "
				        "exists\n", (int)find_thread(NULL), (int)nodeid.port,
				        (int)nodeid.token);
				err = B_NAME_IN_USE;
				goto err1;
			}
			else if(next_node->nodeid.token > nodeid.token)
				break;
		}
		prev_node = next_node;
		next_node = next_node->next;
	}
	{
		binder_node *node;
		node = malloc(sizeof(binder_node));
		if(node == NULL) {
			dprintf("binder(%d) -- new_binder_node: no memory\n",
			        (int)find_thread(NULL));
			err = B_NO_MEMORY;
			goto err1;
		}
		node->next = next_node;
		node->primary_ref_count = 0;
		node->secondary_ref_count = 1;
		node->nodeid = nodeid;
		node->links = NULL;
		node->link_count = 0;
		node->flags = 0;
		node->host = -1;
		node->observers.client_id.port = -1;
		node->observers.next = &node->observers;
		node->observers.prev = &node->observers;
		{
			port_info pi;
			get_port_info(nodeid.port, &pi);
			node->host = pi.team;
		}
		node->tmp_ptr = NULL;
		if(prev_node == NULL)
			binder_nodes = node;
		else
			prev_node->next = node;
		*new_node = node;
	}
	err = B_OK;
err1:
	UNLOCK(binder_nodes_lock);
	return err;
}

// #pragma mark -

status_t 
init_binder_node()
{
	status_t err;
	err = new_lock(&binder_nodes_lock, "binder nodes lock");
	if(err < B_OK)
		goto err0;
	binder_nodes = NULL;
	err = B_OK;
err0:
	return err;
}

void 
uninit_binder_node()
{
	while(binder_nodes != NULL) {
		binder_node *node = binder_nodes;
		binder_nodes = node->next;
		dprintf("binder(%d) -- uninit_binder_node: delete node %d-%d\n",
		        (int)find_thread(NULL), (int)node->nodeid.port,
		        (int)node->nodeid.token);
		free(node);
	}
	free_lock(&binder_nodes_lock);
}

// #pragma mark -

int 
kdebug_dump_binder_nodes(bool start)
{
	static int node_count;
	static binder_node *node;
	int line_count = 20;
	if(start) {
		node_count = 0;
		node = binder_nodes;
		kprintf("binder nodes:\n");
		line_count--;
	}
	while(line_count-- > 0 && node != NULL) {
		node_count++;
		kprintf("node %d-%d: flags %x, ref count p %d s %d\n",
		        (int)node->nodeid.port, (int)node->nodeid.token, node->flags,
		        (int)node->primary_ref_count, (int)node->secondary_ref_count);
		node = node->next;
	}
	if(node != NULL) {
		kprintf("more...\n");
		return B_KDEBUG_CONT;
	}
	kprintf("node count: %d\n", node_count);
	return 0;
}

void 
kdebug_dump_binder_node(uint32 addr, uint32 *next_addr)
{
	binder_node *node;
	binder_node_observer *observer;
	int line_count = 20;

	if(addr == 0)
		node = binder_nodes;
	else
		node = (binder_node *)addr;
	if(node == NULL)
		return;
	kprintf("node @ %p: nodeid %d-%d\n",
	        node, (int)node->nodeid.port, (int)node->nodeid.token);
	kprintf("ref count:         primary %d, secondary %d\n",
	        (int)node->primary_ref_count, (int)node->secondary_ref_count);
	kprintf("host:              team %d\n", (int)node->host);
	kprintf("flags:             %x\n", node->flags);
	kprintf("number of links:   %d\n", node->link_count);
	observer = node->observers.next;
	while(observer != &node->observers) {
		if(line_count-- <= 0) {
			kprintf("more observers at %p\n", observer);
			break;
		}
		kprintf("observer: %d-%d @ %p\n", (int)observer->client_id.port,
		        (int)observer->client_id.token, observer);
		observer = observer->next;
	}
	kprintf("next node at %p\n", node->next);
	*next_addr = (uint32)node->next;
}

