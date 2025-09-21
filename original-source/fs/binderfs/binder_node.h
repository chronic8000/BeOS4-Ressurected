#ifndef BINDER_NODE_H
#define BINDER_NODE_H

#include <kernel/OS.h>
#include "support_p/BinderPriv.h"
extern "C" {
	#include <fsproto.h>
	#include <lock.h>
}
struct vnode_link;
struct port_pool;
struct Bitmap;

struct nspace {
	nspace_id			nsid;
	binder_vnode_id		rootID;
	vnode *				root;
    char *				device;
    char *				name;
    char *				fsname;
	int32				flags;
	lock				linkLock;
	lock				inPortLock;
	port_pool *			inPortFree;
	port_pool *			inPortActive;
	//Bitmap *			threadBitmap;

	int32				node_count;
	int32				vnode_count;
	int32				node_ref_count;
};

struct vnode {
	nspace *			ns;
	binder_vnode_id		vnid;
	vnode_link *		links;
	uint16				linkCount;
	uint16				flags;
	int32				refCount;
	team_id				host;

	vnode				*tmp_ptr; /* NULL when linkLock free */
};

struct node_list {
	int                 count;
	vnode              *node[0];
};

#endif

