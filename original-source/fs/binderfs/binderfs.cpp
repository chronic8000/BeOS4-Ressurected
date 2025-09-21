#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <KernelExport.h>
#include "support_p/BinderPriv.h"

#include "transaction_state.h"
#include "binder_request.h"
#include "file_cookie.h"
#include "port_pool.h"
#include "binder_node.h"
#include "debug_flags.h"

extern "C" {
	#include <fsproto.h>
	#include <lock.h>
}

#define printf(a)
// dprintf a
#define checkpoint
// dprintf("registryfs: thid=%d -- %s:%d -- %s\n",find_thread(NULL),__FILE__,__LINE__,__FUNCTION__);

static status_t add_ref_node(vnode *vn);
static int unref_node(vnode *vn);
static int32 notify_callback(vnode *vn, notify_callback_struct *ncs, bool lockEm=true);
static bool is_local(binder_cmd *cmd, port_id port);
static int free_property_iterator_cookie(void *_cookie);

/* VNode flags */
enum {
	vnodeRedirect	= 0x01,
	vnodeHosted		= 0x02,
	vnodeDied		= 0x04
};

struct internal_connect_struct {
	reflection_struct      *reflection;
	state_connect          *state;
	transaction_resources  *resources;
};

struct vnode_link {
	vnode *				link;
	uint8				fromFlags;
	uint8				toFlags;
};

struct single_cookie {
	vnode *				vn;
	void *				userCookie;
	bool				opened;
	char *				nextName;
	int32				nextNameLen;
	int32				err;
	
	status_t	AllocateName(int32 size);
};

status_t
single_cookie::AllocateName(int32 len)
{
	if(len <= 0) {
		dprintf("binderfs -- AllocateName: tried to allocate space for empty string\n");
		return B_BAD_VALUE;
	}
	if(nextNameLen >= len)
		return B_OK;

	char *newNextName = (char*)realloc(nextName, len + 1);
	if(newNextName == NULL)
		return B_NO_MEMORY;

	nextNameLen = len;
	nextName = newNextName;
	return B_OK;
}


struct multi_cookie {
	int32				count;
	team_id				client;
	int					fd;
	bool				opened;
	single_cookie		cookies[1];
};

struct mountparms {
	port_id				port;
	int32				token;
};


struct links_copy {
	int32 count;
	vnode_link *links;
	
	links_copy(vnode *vn, bool needLock=true) {
		if (needLock) LOCK(vn->ns->linkLock);
		count = 0;
		if (vn->linkCount) {
			links = (vnode_link*)malloc(vn->linkCount*sizeof(vnode_link));
			if (links) {
				memcpy(links,vn->links,vn->linkCount*sizeof(vnode_link));
				count = vn->linkCount;
				for (int32 i=0;i<count;i++)
					add_ref_node(links[i].link);
			}
		} else
			links = NULL;
		if (needLock) UNLOCK(vn->ns->linkLock);
	}

	~links_copy() {
		if (links) {
			for (int32 i=0;i<count;i++)
				unref_node(links[i].link);
			free(links);
		}
	};
};

void
free_node_list(node_list *list)
{
	if(list == NULL)
		return;
	for(int i = 0; i < list->count; i++) {
		unref_node(list->node[i]);
	}
	free(list);
}

node_list *
get_node_list(vnode *start_node, uint8 linkFromFlags, uint8 linkToFlags = 0,
              bool include_start = true, bool lock_list = true)
{
	vnode end;
	vnode *curr;
	int list_count;
	node_list *list = NULL;

	curr = start_node;
	if(lock_list)
		LOCK(start_node->ns->linkLock);
	start_node->tmp_ptr = &end;
	list_count = 1;
	while(curr != &end) {
		if(curr == NULL) {
			dprintf("binderfs -- get_node_list: curr == NULL - 1\n");
			goto err;
		}
		for(int i = 0; i < curr->linkCount; i++)
			if((curr->links[i].fromFlags & linkFromFlags) ||
			   (curr->links[i].toFlags & linkToFlags)) {
				vnode *node = curr->links[i].link;
				if(node->tmp_ptr != NULL) {
					dprintf("binderfs -- get_node_list: node %p already "
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
	list = (node_list *)malloc(sizeof(node_list) + sizeof(vnode*) * list_count);
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
			dprintf("binderfs -- get_node_list: curr == NULL - 2\n");
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
		if(curr == NULL) {
			dprintf("binderfs -- get_node_list: curr == NULL - 3\n");
			break;
		}
		vnode *node = curr;
		curr = curr->tmp_ptr;
		node->tmp_ptr = NULL;
	}
	if(lock_list)
		UNLOCK(start_node->ns->linkLock);
	return list;
}

node_list *
get_call_order_node_list(vnode *start_node)
{
	vnode bottom;
	vnode *stack1, *stack2, *stack3;
	stack1 = start_node;
	stack2 = &bottom;
	stack3 = &bottom;
	int stack3_count = 0;
	node_list *list = NULL;
	
	LOCK(start_node->ns->linkLock);
	start_node->tmp_ptr = &bottom;
	while(stack1 != &bottom || stack2 != &bottom) {
		if(stack1 != &bottom) {
			vnode *base_node = stack1;
			if(base_node == NULL) {
				dprintf("binderfs -- get_call_order_node_list: "
				        "base_node == NULL - 1\n");
				goto err;
			}
			stack1 = stack1->tmp_ptr;
			base_node->tmp_ptr = stack2;
			stack2 = base_node;
			for(int i = base_node->linkCount - 1; i >= 0 ; i--)
				if(base_node->links[i].fromFlags & linkFilter) {
					vnode *node = base_node->links[i].link;
					if(node->tmp_ptr != NULL) {
						dprintf("binderfs: node %p already visited\n",
						        node);
					}
					else {
						node->tmp_ptr = stack1;
						stack1 = node;
					}
				}
		}
		if(stack1 == &bottom && stack2 != &bottom) {
			vnode *base_node = stack2;
			if(base_node == NULL) {
				dprintf("binderfs -- get_call_order_node_list: "
				        "base_node == NULL - 1\n");
				goto err;
			}
			stack2 = stack2->tmp_ptr;
			base_node->tmp_ptr = stack3;
			stack3 = base_node;
			stack3_count++;
			for(int i = base_node->linkCount - 1; i >= 0 ; i--)
				if(base_node->links[i].fromFlags & linkInherit) {
					vnode *node = base_node->links[i].link;
					if(node->tmp_ptr != NULL) {
						dprintf("binderfs: node %p already visited\n",
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
	                           sizeof(vnode*) * stack3_count);
	if(list == NULL) {
		goto err;
	}
	list->count = stack3_count;
	while(stack3_count > 0) {
		if(stack3 == NULL || stack3 == &bottom) {
			dprintf("binderfs -- get_call_order_node_list: stack3 == NULL\n");
			for(int i = stack3_count; i < list->count; i++)
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
		if(stack3 == NULL) {
			dprintf("binderfs -- get_call_order_node_list-2: stack3 == NULL\n");
			break;
		}
		vnode *node = stack3;
		stack3 = stack3->tmp_ptr;
		node->tmp_ptr = NULL;
	}
	UNLOCK(start_node->ns->linkLock);
	return list;
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
free_transaction_state(nspace *ns, transaction_state *state)
{
	if(state->resources.reply_port != NULL) {
		if(!is_port_valid(ns, state->resources.reply_port)) {
			dprintf("binderfs(%d) -- free_transaction_state: bad reply port\n",
			        (int)find_thread(NULL));
		}
		else {
			dprintf("binderfs(%d) -- free_transaction_state: cleanup reply port"
			        " %d\n", (int)find_thread(NULL),
			        (int)state->resources.reply_port->port);
			delete_port(ns, state->resources.reply_port);
		}
		if(state->resources.nodes != NULL) {
			dprintf("binderfs(%d) -- free_transaction_state: cleanup node list"
			        " %p\n", (int)find_thread(NULL), state->resources.nodes);
			free_node_list(state->resources.nodes);
		}
		if(state->resources.node != NULL) {
			dprintf("binderfs(%d) -- free_transaction_state: release node "
			        " %016Lx\n", (int)find_thread(NULL),
			        state->resources.node->vnid.value);
			put_vnode(ns->nsid, state->resources.node->vnid.value);
		}
	}
	free(state);
}

void 
file_cookie::clear(nspace *ns)
{
	if(type == FILE_COOKIE_BUFFER) {
		free(data.buffer.buffer);
		data.buffer.bufferSize = 0;
	} else if (type == FILE_COOKIE_TRANSACTION_STATE) {
		free_transaction_state(ns, data.transaction_state.trans);
	} else if (type == FILE_COOKIE_PROPERTY_ITERATOR) {
		free_property_iterator_cookie(data.property_iterator.cookie);
	}
	type = FILE_COOKIE_UNUSED;
}


struct gehnaphore_autolock {
		lock &	m_lock;
	
						gehnaphore_autolock(lock &_lock) : m_lock(_lock) { LOCK(m_lock); }
						~gehnaphore_autolock() { UNLOCK(m_lock); };
};

/*	Link flags can be any of linkInherit, linkFilter and linkAugment

	(fromFlags & linkInherit)
		If a get or put fails or falls through on this node, try the linked node
	(fromFlags & linkFilter)
		Filter any gets or puts through the linked node before trying this node
	(fromFlags & linkAugment)
		This node's property *directory* is augmented with the linked node's
*/	

static team_id
current_team()
{
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	return ti.team;
}

extern "C" {
	static int		uspacefs_read_vnode(void *ns, vnode_id vnid, char r, void **node);
	static int		uspacefs_write_vnode(void *ns, void *node, char r);
	static int		uspacefs_remove_vnode(void *ns, void *node, char r);
	static int		uspacefs_secure_vnode(void *ns, void *node);
	static int		uspacefs_walk(void *ns, void *base, const char *file,
							char **newpath, vnode_id *vnid);
	static int		uspacefs_access(void *ns, void *node, int mode);
	static int		uspacefs_create(void *ns, void *dir, const char *name,
						int perms, int omode, vnode_id *vnid, void **cookie);
	static int      uspacefs_rename(void *_ns, void *_olddir, const char *_oldname,
						void *_newdir, const char *_newname);
	static int		uspacefs_mkdir(void *ns, void *dir, const char *name, int perms);
	static int		uspacefs_unlink(void *ns, void *dir, const char *name);
	static int		uspacefs_rmdir(void *ns, void *dir, const char *name);
	static int		uspacefs_opendir(void *ns, void *node, void **cookie);
	static int		uspacefs_closedir(void *ns, void *node, void *cookie);
	static int		uspacefs_free_dircookie(void *ns, void *node, void *cookie);
	static int		uspacefs_rewinddir(void *ns, void *node, void *cookie);
	static int		uspacefs_readdir(void *ns, void *node, void *cookie,
						long *num, struct dirent *buf, size_t bufsize);
	static int		uspacefs_rstat(void *ns, void *node, struct stat *st);
	static int		uspacefs_wstat(void *ns, void *node, struct stat *st, long mask);
	static int      uspacefs_fsync(void *ns, void *node);
	static int		uspacefs_open(void *ns, void *node, int omode, void **cookie);
	static int		uspacefs_close(void *ns, void *node, void *cookie);
	static int		uspacefs_free_cookie(void *ns, void *node, void *cookie);
	static int		uspacefs_read(void *ns, void *node, void *cookie, off_t pos,
							void *buf, size_t *len);
	static int		uspacefs_write(void *ns, void *node, void *cookie, off_t pos,
							const void *buf, size_t *len);
	static int		uspacefs_ioctl(void *ns, void *node, void *cookie, int cmd,
							void *buf, size_t len);
	static int		uspacefs_mount(nspace_id nsid, const char *device, ulong flags,
							void *parms, size_t len, void **data, vnode_id *vnid);
	static int		uspacefs_unmount(void *ns);
	static int		uspacefs_rfsstat(void *ns, struct fs_info *);
	static int      uspacefs_wfsstat(void *ns, struct fs_info *, long mask);
	static int		uspacefs_sync(void *ns);
#if 0
	static int      uspacefs_initialize(const char *devname, void *parms, size_t len);
	static int      uspacefs_open_attrdir(void *_ns, void *_node, void **_cookie);
	static int      uspacefs_close_attrdir(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_free_attr_dircookie(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_rewind_attrdir(void *_ns, void *_node, void *_cookie);
	static int      uspacefs_read_attrdir(void *_ns, void *_node, void *_cookie,
										int32 *num, struct dirent *buf,
										size_t bufsize);
	static int      uspacefs_read_attr(void *_ns, void *_node, const char *name,
									 int type, void *buf, size_t *len,
									 dr9_off_t pos);
	static int      uspacefs_write_attr(void *_ns, void *_node, const char *name,
									 int type, const void *buf, size_t *len,
									 dr9_off_t pos);
	static int      uspacefs_stat_attr(void *_ns, void *_node, const char *name,
									 struct attr_info *buf);
	static int		uspacefs_symlink(void *_ns, void *_dir, const char *name, const char *path);
	static int		uspacefs_readlink(void *ns, void *node, char *buf, size_t *bufsize);
#endif
	
	vnode_ops fs_entry =  {
		&uspacefs_read_vnode,
		&uspacefs_write_vnode,
		&uspacefs_remove_vnode,
		&uspacefs_secure_vnode,
		&uspacefs_walk,
		&uspacefs_access,
		&uspacefs_create,
		NULL,					/* &uspacefs_mkdir, */
		NULL,					/* &uspacefs_symlink, */
		NULL,
		NULL,					/* &uspacefs_rename, */
		NULL,					/* &uspacefs_unlink, */
		NULL,					/* &uspacefs_rmdir, */
		NULL,					/* &uspacefs_readlink, */

		&uspacefs_opendir,
		&uspacefs_closedir,
		&uspacefs_free_dircookie,
		&uspacefs_rewinddir,
		&uspacefs_readdir,

		&uspacefs_open,
		&uspacefs_close,
		&uspacefs_free_cookie,
		&uspacefs_read,
		&uspacefs_write,
		NULL, 											/* readv */
		NULL, 											/* writev */
		&uspacefs_ioctl,
		NULL,
		&uspacefs_rstat,
		&uspacefs_wstat,
		&uspacefs_fsync,
		NULL,											/* &uspacefs_initialize, */
		&uspacefs_mount,
		&uspacefs_unmount,
		&uspacefs_sync,
	
		&uspacefs_rfsstat,
		NULL, 											/* &uspacefs_wfsstat, */
	
		NULL,
		NULL,
	
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	
		NULL,					/* &uspacefs_open_attrdir, */
		NULL,					/* &uspacefs_close_attrdir, */
		NULL,					/* &uspacefs_free_attr_dircookie, */
		NULL,					/* &uspacefs_rewind_attrdir, */
		NULL,					/* &uspacefs_read_attrdir, */
	
		NULL,											/* &uspacefs_write_attr, */
		NULL,											/* &uspacefs_read_attr, */
	    NULL,											/* &uspacefs_remove_attr, */
		NULL,											/* &uspacefs_rename_attr, */
		NULL,											/* &uspacefs_stat_attr, */
		
		NULL,
		NULL,
		NULL,
		NULL
	};
	
	int32 api_version = B_CUR_FS_API_VERSION;

	int sys_open_vn(bool kernel, nspace_id nsid, vnode_id vnid, const char *path, int omode, bool coe);
	int sys_ioctl(bool kernel, int fd, int cmd, void *arg, size_t sz);
	int sys_close(bool kernel, int fd);
}

enum registry_property_type {
	undefined_type = 0,
	string,
	number,
	object,
	remote_object
};

static bool 
is_local(binder_cmd */*cmd*/, port_id port)
{
	port_info pi;
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	get_port_info(port,&pi);
	return (ti.team == pi.team);
}

static status_t
add_ref_node(vnode *vn)
{
	status_t err;
	int32 old;

	err = get_vnode(vn->ns->nsid,vn->vnid.value,(void**)&vn);
	if(err < B_OK)
		goto err;
	old = atomic_add(&vn->refCount,1);
#if TRACE_REFS
	{
		int total_refs = atomic_add(&vn->ns->node_ref_count, 1) + 1;
		dprintf("binderfs(%d) -- add_ref_node: %016Lx rc %x (total %d)\n",
		        (int)find_thread(NULL), vn->vnid.value, (int)old+1, total_refs);
	}
#endif
	if(old == 65536 || old == 0) {
		dprintf("binderfs(%d) -- add_ref_node: ERROR node %016Lx not "
		        "referenced\n", (int)find_thread(NULL), vn->vnid.value);
		//kernel_debugger("binderfs");
	}
	return B_OK;
err:
	return err;
}

static status_t
ref_node(vnode *vn, reflection_struct &reflection, state_ref_node &state,
         transaction_resources &resources)
{
	status_t err;

	if(state.stage == 0) {
		int32 old;
		err = get_vnode(vn->ns->nsid,vn->vnid.value,(void**)&vn);
		if(err < B_OK)
			goto err1;
		old = atomic_add(&vn->refCount,1);
		if (old == 65536) {
			state.stage = 1;
		}
#if TRACE_NODES
		if(old == 0) {
			int total_count = atomic_add(&vn->ns->node_count, 1) + 1;
			dprintf("binderfs(%d) -- ref_node: unhosted node created %016Lx, "
			        "nodes %d\n",
			        (int)find_thread(NULL), vn->vnid.value, total_count);
		}
#endif
#if TRACE_REFS
		int total_refs = atomic_add(&vn->ns->node_ref_count, 1) + 1;
		dprintf("binderfs(%d) -- ref_node: %016Lx rc %x (total %d)\n",
		        (int)find_thread(NULL), vn->vnid.value, (int)old+1, total_refs);
#endif
	}

	if(state.stage == 1) {
		if(vn->host == current_team()) {
			reflection.add_cmd(REFLECT_CMD_ACQUIRE,
			                   vn->vnid.address.token);
#if TRACE_REFLECTION
			dprintf("binderfs(%d) -- ref_node: reflect acquire\n",
			        (int)find_thread(NULL));
#endif
			state.stage = 3;
			return B_OK;
		}
		err = send_binder_request(vn, BINDER_REQUEST_CONNECT,
		                          &resources.reply_port);
		if(err != B_OK) {
			dprintf("binderfs(%d) -- ref_node: send_binder_request failed, "
			        "%s\n", (int)find_thread(NULL), strerror(err));
			goto err2;
		}
		state.stage = 2;
	}
	if(state.stage == 2) {
		err = get_binder_reply(vn, resources.reply_port, reflection);
		if(err != B_OK)
			goto err2;
		if(reflection.active)
			return B_OK;
		printf(("Connecting to node %016Lx --> %d\n",vn->vnid.value,(int)err));
	}
	err = B_OK;
	goto done;

err2:
	atomic_add(&vn->refCount, -1);
	put_vnode(vn->ns->nsid, vn->vnid.value);
err1:
done:
	state.stage = 0;
	resources.reply_port = NULL;
	return err;
}

static int
unref_node(vnode *vn)
{
	int32 err = B_OK;
	int32 old = atomic_add(&vn->refCount,-1);

#if TRACE_REFS
	int total_refs = atomic_add(&vn->ns->node_ref_count, -1) - 1;
	dprintf("binderfs(%d) -- unref_node: %016Lx rc %x (total %d)\n",
	        (int)find_thread(NULL), vn->vnid.value, (int)old - 1, total_refs);
#endif
	if (old == 65537) {
#if TRACE_NODES
		dprintf("binderfs(%d) -- unref_node: no more clients for node %016Lx\n",
		        (int)find_thread(NULL), vn->vnid.value);
#endif
		err = send_binder_request(vn, BINDER_REQUEST_DISCONNECT, NULL);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- unref_node: send_binder_request failed, "
			        "%s\n", (int)find_thread(NULL), strerror(err));
		}
		printf(("Disconnecting from node %016Lx --> %d\n",vn->vnid.value, (int)err));
	}
#if TRACE_NODES
	if(old == 1) {
		int total_count = atomic_add(&vn->ns->node_count, -1) - 1;
		dprintf("binderfs(%d) -- unref_node: node deleted %016Lx, "
		        "nodes %d\n",
		        (int)find_thread(NULL), vn->vnid.value, total_count);
	}
#endif
	
	put_vnode(vn->ns->nsid,vn->vnid.value);
	
	return err;
}

static int
create_link(vnode *linkFrom, vnode *linkTo, uint32 linkFlags,
            reflection_struct &reflection, state_create_link &state,
            transaction_resources &resources)
{
	status_t err;
	if (!linkFlags) return B_ERROR;

	gehnaphore_autolock _auto(linkFrom->ns->linkLock);
#if TRACE_LINKS
	dprintf("binderfs(%d) -- create_link: from %016Lx to %016Lx\n",
	        (int)find_thread(NULL), linkFrom->vnid.value, linkTo->vnid.value);
#endif
	
	if(state.stage == 0) {
		int32 i,j,count = linkFrom->linkCount;
		for (i=0;i<count;i++) {
			if (linkFrom->links[i].link == linkTo) {
				for (j=0;j<linkTo->linkCount;j++) {
					if (linkTo->links[j].link == linkFrom) break;
				}
				if (j==linkTo->linkCount) {
					dprintf("binderfs(%d) -- create_link: vnode links are "
					        "corrupt (1)!\n", (int)find_thread(NULL));
					return B_ERROR;
				}
				if (linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
					dprintf("binderfs(%d) -- create_link: vnode links are "
					        "corrupt (2)!\n", (int)find_thread(NULL));
					return B_ERROR;
				}
				linkFrom->links[i].fromFlags |= linkFlags;
				linkTo->links[j].toFlags |= linkFlags;
				return B_OK;
			}
		}
		state.stage = 1;
	}

	if(state.stage == 1) {
		err = ref_node(linkFrom, reflection, state.ref_node, resources);
		if(err < B_OK)
			goto err1;
		if(reflection.active)
			return B_OK;
		state.stage = 2;
	}
	if(state.stage == 2) {
		err = ref_node(linkTo, reflection, state.ref_node, resources);
		if(err < B_OK)
			goto err2;
		if(reflection.active)
			return B_OK;

#if TRACE_LINKS
		dprintf("binderfs(%d) -- create_link: 1 Getting vnode %016Lx\n",
		        (int)find_thread(NULL), linkFrom->vnid.value);
		dprintf("binderfs(%d) -- create_link: 2 Getting vnode %016Lx\n",
		        (int)find_thread(NULL), linkTo->vnid.value);
#endif
		
		int32 i,j,count = linkFrom->linkCount;
		for (i=0;i<count;i++) {
			if (linkFrom->links[i].link == linkTo) {
				for (j=0;j<linkTo->linkCount;j++) {
					if (linkTo->links[j].link == linkFrom) break;
				}
				if (j==linkTo->linkCount) {
					dprintf("binderfs(%d) -- create_link: vnode links are "
					        "corrupt (1)!\n", (int)find_thread(NULL));
					err = B_ERROR;
					goto err3;
				}
				if (linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
					dprintf("binderfs(%d) -- create_link: vnode links are "
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

		vnode_link *new_fromlinks;
		vnode_link *new_tolinks;
		new_fromlinks = (vnode_link*)realloc(linkFrom->links,(linkFrom->linkCount+1)*sizeof(vnode_link));
		if(new_fromlinks == NULL) {
			err = B_NO_MEMORY;
			goto err3;
		}
		new_tolinks = (vnode_link*)realloc(linkTo->links,(linkTo->linkCount+1)*sizeof(vnode_link));
		if(new_tolinks == NULL) {
			err = B_NO_MEMORY;
			goto err3;
		}
		
		linkFrom->links = new_fromlinks;
		linkFrom->links[linkFrom->linkCount].link = linkTo;
		linkFrom->links[linkFrom->linkCount].fromFlags = linkFlags;
		linkFrom->links[linkFrom->linkCount].toFlags = 0;
		linkFrom->linkCount++;
	
		linkTo->links = new_tolinks;
		linkTo->links[linkTo->linkCount].link = linkFrom;
		linkTo->links[linkTo->linkCount].toFlags = linkFlags;
		linkTo->links[linkTo->linkCount].fromFlags = 0;
		linkTo->linkCount++;
	}
	else {
		err = B_ERROR;
		dprintf("binderfs(%d) -- create_link: bad state, stage = %d\n",
		        (int)find_thread(NULL), state.stage);
	}

	state.stage = 0;
	return B_OK;

abort:
err3:
	unref_node(linkTo);
err2:
	unref_node(linkFrom);
err1:
	state.stage = 0;
	return err;
}

static int
destroy_link(vnode *base, vnode *linkToMatch, uint32 linkFlags, bool from=true)
{
	vnode *linkTo,*linkFrom,*tmp;
	int t,i,j,k;

	gehnaphore_autolock _auto(base->ns->linkLock);

#if TRACE_LINKS
	dprintf("binderfs(%d) -- destroy_link: base   = %016Lx, from = %d\n",
	        (int)find_thread(NULL), base->vnid.value, (int)from);
#endif

	for (t=0;t<base->linkCount;t++) {
		i = t;
		if (!linkToMatch || (base->links[i].link == linkToMatch)) {
#if TRACE_LINKS
			dprintf("binderfs(%d) -- destroy_link:\n"
			        "                              from   = %d\n"
			        "                              base   = %016Lx\n"
			        "                              linkTo = %016Lx\n",
			        (int)find_thread(NULL), (int)from, base->vnid.value,
			        base->links[i].link->vnid.value);
#endif
			linkFrom = base;
			linkTo = base->links[i].link;
			for (j=0;j<linkTo->linkCount;j++) {
#if TRACE_LINKS
				dprintf("binderfs(%d) -- destroy_link: "
				        "linkTo.links[%d] = %016Lx\n", (int)find_thread(NULL),
				        j, linkTo->links[j].link->vnid.value);
#endif
				if (linkTo->links[j].link == linkFrom) break;
			}
			if (j==linkTo->linkCount) {
				dprintf("binderfs(%d) -- destroy_link: vnode links are "
				        "corrupt (1)!\n", (int)find_thread(NULL));
				return B_ERROR;
			}
			if (linkFrom->links[i].fromFlags != linkTo->links[j].toFlags) {
				dprintf("binderfs(%d) -- destroy_link: vnode links are "
				        "corrupt (2)!\n", (int)find_thread(NULL));
				return B_ERROR;
			}
			if (!from) {
				k=i; i=j; j=k;
				tmp=linkTo; linkTo=linkFrom; linkFrom=tmp;
			}

			notify_callback_struct ncs;
			ncs.event = B_SOMETHING_CHANGED;
			ncs.name[0] = 0;
			notify_callback(linkFrom,&ncs,false);

			linkFrom->links[i].fromFlags &= ~linkFlags;
			linkTo->links[j].toFlags &= ~linkFlags;

			if (!linkTo->links[j].toFlags && !linkTo->links[j].fromFlags) {
				for (k=j;k<linkTo->linkCount-1;k++) linkTo->links[k] = linkTo->links[k+1];
				linkTo->linkCount--;
				unref_node(linkFrom);
#if TRACE_LINKS
				dprintf("binderfs(%d) -- destroy_link: 1 Putting vnode %016Lx"
				        "\n", (int)find_thread(NULL), linkFrom->vnid.value);
#endif
				if (!from && !linkToMatch) t--;
			}
			if (!linkFrom->links[i].fromFlags && !linkFrom->links[i].toFlags) {
				for (k=i;k<linkFrom->linkCount-1;k++) linkFrom->links[k] = linkFrom->links[k+1];
				linkFrom->linkCount--;
				unref_node(linkTo);
#if TRACE_LINKS
				dprintf("binderfs(%d) -- destroy_link: 2 Putting vnode %016Lx"
				        "\n", (int)find_thread(NULL), linkTo->vnid.value);
#endif
				if (from && !linkToMatch) t--;
			}
			if (linkToMatch) return B_OK;
		}
	}
	
	if (linkToMatch) {
		dprintf("binderfs(%d) -- destroy_link: vnode link not found (2)!\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	
	return B_OK;
}

static status_t
update_redirect(vnode *vn, reflection_struct &reflection,
                state_update_redirect &state, transaction_resources &resources)
{
	status_t err;
	//dprintf("binderfs(%d) -- update_redirect: entry stage == %d\n",
	//        (int)find_thread(NULL), state.stage);
	if(state.stage == 0) {
		bool hadRedirect,needRedirect = false;
		{
			gehnaphore_autolock _auto(vn->ns->linkLock);
			hadRedirect = ((vn->flags & vnodeRedirect) != 0);
			needRedirect = false;
			if (vn->linkCount) needRedirect = true;
			if (needRedirect) vn->flags |= vnodeRedirect;
			else vn->flags &= ~vnodeRedirect;
		}
	
#if TRACE_REDIRECT
		dprintf("binderfs(%d) -- update_redirect: %Lx team %d redirect %d -> %d\n",
		        (int)find_thread(NULL), vn->vnid.value, (int)vn->host,
		        hadRedirect, needRedirect);
#endif	
		if(needRedirect != hadRedirect) {
			if(is_local(NULL, vn->vnid.address.port)) {
#if TRACE_REFLECTION
				dprintf("binderfs(%d) -- update_redirect: reflect %sredirect\n",
				        (int)find_thread(NULL), needRedirect ? "" : "un");
#endif
				reflection.add_cmd(needRedirect ?
				                   REFLECT_CMD_REDIRECT :
				                   REFLECT_CMD_UNREDIRECT,
				                   vn->vnid.address.token);
				state.stage = 2;
				return B_OK;
			} else {
				err = send_binder_request(vn, BINDER_REQUEST_REDIRECT,
				                          needRedirect,
				                          &resources.reply_port);
				if(err != B_OK) {
					dprintf("binderfs(%d) -- update_redirect: "
					        "send_binder_request failed, %s\n",
					        (int)find_thread(NULL), strerror(err));
					goto err;
				}
				state.stage = 1;
			}
		}
		else {
			state.stage = 2;
		}
	}
	if(state.stage == 1) {
		err = get_binder_reply(vn, resources.reply_port, reflection);
		if(err < B_OK)
			goto err;
		if(reflection.active)
			return B_OK;
	}
	else if(state.stage != 2) {
		dprintf("binderfs(%d) -- update_redirect: bad state, stage == %d\n",
		        (int)find_thread(NULL), state.stage);
	}
	err = B_OK;
err:
	state.stage = 0;
	resources.reply_port = NULL;
	return err;
}

static status_t
link_nodes(nspace *ns, vnode *node, binder_cmd *cmd,
           transaction_state *full_state, uint32 linkFlags)
{
	state_link_nodes &state = full_state->link_nodes;
	transaction_resources &resources = full_state->resources;
	status_t err = B_OK;

	//dprintf("link_nodes entry, stage = %d\n", state.stage);
	if(state.stage == 0) {
		if(!(node->flags & vnodeHosted)) {
			dprintf("binderfs(%d) -- link_nodes: from node, %016Lx, must be "
			        "hosted!\n", (int)find_thread(NULL), node->vnid.value);
			err = B_ERROR;
			goto err1;
		}
		
		if(resources.node != NULL) {
			dprintf("binderfs(%d) -- link_nodes: resources.node not NULL, %p\n",
			        (int)find_thread(NULL), resources.node);
			err = B_ERROR;
			goto err1;
		}
		err = get_vnode(ns->nsid, cmd->data.vnid.value,
		                (void**)&resources.node);
		if(err < B_OK)
			goto err1;

		if(node == resources.node) {
			dprintf("binderfs(%d) -- link_nodes: from and to nodes are the "
			        "same!\n", (int)find_thread(NULL));
			err = B_BAD_VALUE;
			goto err2;
		}
	
		if(!(resources.node->flags & vnodeHosted)) {
			dprintf("binderfs(%d) -- link_nodes: to node, %016Lx, must be "
			        "hosted!\n", (int)find_thread(NULL),
			        resources.node->vnid.value);
			err = B_ERROR;
			goto err2;
		}
		state.stage = 1;
	}
	if(state.stage == 1) {
		//dprintf("link_nodes: create_link\n");
		err = create_link(node, resources.node, linkFlags, cmd->reflection,
		                  state.create_link, resources);
		if(err < B_OK)
			goto err2;
		if(cmd->reflection.active)
			return B_OK;
		state.stage = 2;
	}
	if(state.stage == 2) {
		//dprintf("link_nodes: update_redirect\n");
		err = update_redirect(node, cmd->reflection, state.update_redirect,
		                      resources);
		if(err < B_OK)
			goto err2;
		if(cmd->reflection.active)
			return B_OK;
		//update_redirect(me,cmd); // do we need to update both?

		notify_callback_struct ncs;
		ncs.event = B_SOMETHING_CHANGED;
		ncs.name[0] = 0;
		notify_callback(node,&ncs);
	}
	else {
		dprintf("binderfs(%d) -- link_nodes: bad state, stage == %d\n",
		        (int)find_thread(NULL), state.stage);
	}
	//dprintf("link_nodes: return %s\n", strerror(err));
err2:
	put_vnode(ns->nsid, cmd->data.vnid.value);
	resources.node = NULL;
err1:
	state.stage = 0;
	return err;
}

static status_t
break_links(nspace */*ns*/, vnode *vn, binder_cmd *cmd,
            transaction_state *full_state)
{
	state_break_links &state = full_state->break_links;
	transaction_resources &resources = full_state->resources;
	status_t err;

	if(state.stage == 0) {
		uint32 flags = cmd->data.break_links.flags;
		if (flags & linkFrom) {
			checkpoint
			destroy_link(vn,NULL,flags&(linkInherit|linkAugment|linkFilter),true);
		}
		if (flags & linkTo) {
			checkpoint
			destroy_link(vn,NULL,flags&(linkInherit|linkAugment|linkFilter),false);
		}
		state.stage = 1;
	}
	if(state.stage == 1) {
		err = update_redirect(vn, cmd->reflection, state.update_redirect,
		                      resources);
		if(err < B_OK)
			goto err;
		if(cmd->reflection.active)
			return B_OK;
	}
	err = B_OK;
err:
	state.stage = 0;
	return err;
}

static int
unlink_node(vnode *node)
{
	reflection_struct reflection;
	state_update_redirect state;
	transaction_resources resources;
	memset(&state, 0, sizeof(state));
	memset(&resources, 0, sizeof(resources));

	destroy_link(node,NULL,linkAll,true);	
	destroy_link(node,NULL,linkAll,false);

	reflection.count = 1;
	while(reflection.count > 0) {
		status_t err;
		reflection.count = 0;
		reflection.active = false;
		err = update_redirect(node, reflection, state, resources);
		if(err < 0)
			return err;
		if(reflection.active) {
			dprintf("binderfs(%d) -- unlink_node: ignored reflection\n",
			        (int)find_thread(NULL));
		}
	}
	return B_OK;
}

#define PROP_DIR_UP		0x01
#define PROP_DIR_DOWN	0x02
#define PROP_DIR_IN		0x04

static status_t
get_property_once(vnode *node, get_property_struct *gps,
                  reflection_struct &reflection,
                  state_get_property_once &state,
                  transaction_resources &resources)
{
	status_t err = B_ERROR;

	//dprintf("binderfs(%d) -- get_property_once: stage %d, name %s\n",
	//        (int)find_thread(NULL), state.stage, gps->name);

	if(state.stage == 0) {
		err = send_binder_request(node, BINDER_REQUEST_GET_PROPERTY, gps->name,
		                          gps->args, gps->argsSize,
		                          &resources.reply_port_2);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- get_property_once: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err1;
		}
		state.stage = 1;
	}
	if(state.stage == 1) {
		if(resources.reply_port_2 == NULL) {
			dprintf("binderfs(%d) -- get_property_once: ERROR "
			        "reply_port == NULL\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err1;
		}
	
		binder_reply_get_property *reply;
		size_t reply_size;
	
		err = get_binder_reply(node, resources.reply_port_2, reflection,
		                       (binder_reply_basic**)&reply, &reply_size);
		if(err < B_OK)
			goto err1;
		if(reflection.active)
			return B_OK;

		if(reply == NULL) {
			dprintf("binderfs(%d) -- get_property_once: ERROR reply == NULL, "
			        "size %d\n",
			        (int)find_thread(NULL), (int)reply_size);
			err = B_ERROR;
			goto err1;
		}
		if(reply_size < sizeof(binder_reply_get_property_status)) {
			dprintf("binderfs(%d) -- get_property_once: bad reply size, "
			        "size %d\n",
			        (int)find_thread(NULL), (int)reply_size);
			free_binder_reply(reply);
			err = B_ERROR;
			goto err1;
		}
		state.sync_port = reply->sync_port;
		gps->result = reply->get_result;
	
		state.stage = 3;
		if(!gps->result.error) {
			gps->streamSize = 0;
			if(reply_size < sizeof(binder_reply_get_property) ||
			   reply_size < sizeof(binder_reply_get_property) + reply->rlen) {
				if(reply_size < sizeof(binder_reply_get_property))
					dprintf("binderfs(%d) -- get_property_once: bad reply "
					        "size, no space for reply->rlen\n",
					        (int)find_thread(NULL));
				else
					dprintf("binderfs(%d) -- get_property_once: bad reply "
					        "size, reply size %d reply->rlen %d\n",
					        (int)find_thread(NULL), (int)reply_size,
					        (int)reply->rlen);
				gps->result.error = B_ERROR;
			}
			else {
				if(reply->rlen <= gps->returnBufSize) {
					memcpy(gps->returnBuf, reply->rbuf, reply->rlen);
		
					registry_property_type t = ((registry_property_type)*((int32*)gps->returnBuf));
					if ((reply->rlen>=12) && (t == object)) {
						binder_vnode_id *vnid = (binder_vnode_id*)(((uint8*)gps->returnBuf)+4+sizeof(dev_t));
						gps->returnDescriptor = sys_open_vn(false,node->ns->nsid,vnid->value,NULL,O_RDWR|O_CLOEXEC,true);
						//dprintf("binderfs: get_property_once, vn = %Lx, fd = %d\n",
						//        vnid->value, (int)gps->returnDescriptor);
						
						if(gps->returnDescriptor < 0)
							err = gps->returnDescriptor;
						else
							state.stage = 2;
					}
				} else {
					gps->streamSize = reply->rlen;
					gps->returnDescriptor = sys_open_vn(false, node->ns->nsid,
					                                    node->vnid.value, NULL,
					                                    O_RDWR|O_CLOEXEC,true);
					//dprintf("binderfs: get_property_once, vn = %Lx, fd = %d\n",
					//        vn->vnid.value, (int)gps->returnDescriptor);
					if(gps->returnDescriptor < 0)
						err = gps->returnDescriptor;
					else {
						buffer_t buf;
						buf.data = reply->rbuf;
						buf.size = reply->rlen;
						err = sys_ioctl(false, gps->returnDescriptor,
						                BINDER_ASSOCIATE_BUFFER, &buf, 0);
						if(err < B_OK) {
							sys_close(false, gps->returnDescriptor);
							gps->returnDescriptor = -1;
						}
					}
				}
			}
		}
		free_binder_reply(reply);
	}
	if(state.stage == 2) {
		internal_connect_struct c;
		c.reflection = &reflection;
		c.state = &state.connect;
		c.resources = &resources;
		err = sys_ioctl(false, gps->returnDescriptor,
		                BINDER_INTERNAL_CONNECT, &c, 0);
		if(reflection.active)
			return B_OK;
		if(err < B_OK) {
			sys_close(false, gps->returnDescriptor);
			gps->returnDescriptor = -1;
		}
		state.stage = 3;
	}
	if(state.stage == 3) {
		if(state.sync_port >= 0) {
	#if TRACE_COMMUNICATION
			dprintf("binderfs(%d) -- get_property_once: send sync to %d\n",
			        (int)find_thread(NULL), (int)state.sync_port);
	#endif
			unregister_reply_thread(node->ns, state.sync_port, true);
			release_port(node, resources.reply_port_2);
			if(write_port_etc(state.sync_port, BINDER_SYNC_CODE, NULL, 0,
			                  B_TIMEOUT | B_CAN_INTERRUPT, 1000000) != B_OK) {
				dprintf("binderfs(%d) -- get_property_once: sync, "
				        "write port failed, port %d\n",
				        (int)find_thread(NULL), (int)state.sync_port);
			}
		}
	}
	else {
		dprintf("binderfs(%d) -- get_property_once: bad state, stage = %d\n",
		        (int)find_thread(NULL), state.stage);
		err = B_ERROR;
	}
//err2:
	//release_port(vn, *inPort);
err1:
	//dprintf("binderfs(%d) -- get_property_once: done, %s\n",
	//        (int)find_thread(NULL), strerror(err));
	resources.reply_port_2 = NULL;
	state.stage = 0;
	state.sync_port = 0;
	return err;
}

static status_t
get_property(vnode *base_node, binder_cmd *cmd, transaction_state *full_state)
{
	state_get_property &state = full_state->get_property;
	transaction_resources &resources = full_state->resources;

	status_t err = B_OK;
	//dprintf("binderfs(%d) -- get_property: enter state.stage == %d\n",
	//        (int)find_thread(NULL), state.stage);
	if(state.stage == 0) {
		if(resources.nodes != NULL) {
			dprintf("binderfs -- get_property: nodes != NULL in stage 0\n");
			return B_ERROR;
		}
		resources.nodes = get_call_order_node_list(base_node);
		if(resources.nodes == NULL)
			return B_NO_MEMORY;
		state.stage = 1;
		state.index = 0;
	}
	if(resources.nodes == NULL) {
		dprintf("binderfs -- get_property: nodes == NULL\n");
		return B_ERROR;
	}
	//dprintf("binderfs(%d) -- get_property: stage %d, index %d / %d\n",
	//        (int)find_thread(NULL), state.stage, state.index,
	//        state.nodes->count);
	
	while(state.index < resources.nodes->count) {
		if(state.stage == 1) {
			vnode *node = resources.nodes->node[state.index];
			if(node->host == cmd->reflection.team) {
#if TRACE_REFLECTION
				dprintf("binderfs(%d) -- get_property: reflect get\n",
				        (int)find_thread(NULL));
#endif
				cmd->reflection.add_cmd(REFLECT_CMD_GET,
				                        node->vnid.address.token);
				state.stage = 2;
				return B_OK;
			}
			err = get_property_once(node, &cmd->data.get, cmd->reflection,
			                        state.get_property_once, resources);
			if(cmd->reflection.active)
				return B_OK;
			if(err < B_OK)
				cmd->data.get.result.error = err;
			state.stage = 2;
		}
		if(state.stage == 2) {
			if(!cmd->data.get.result.error) {
				err = B_OK;
				goto done;
			}
			state.index++;
			state.stage = 1;
		}
		if(state.stage != 1) {
			dprintf("binderfs -- get_property: bad state state.stage == %d\n",
			        state.stage);
			return B_ERROR;
		}
	}
	cmd->data.get.result.error = ENOENT;	
	cmd->data.get.result.resultCacheable = false;
	err = ENOENT;
done:
	free_node_list(resources.nodes);
	resources.nodes = NULL;
	state.stage = 0;
	state.index = 0;
	return err;
}

static int
put_property_once(vnode *node, put_property_struct *pps,
                  reflection_struct &reflection,
                  state_put_property_once &state,
                  transaction_resources &resources)
{
	status_t err;

	if(state.stage == 0) {
		err = send_binder_request(node, BINDER_REQUEST_PUT_PROPERTY, pps->name,
		                          pps->value, pps->valueLen,
		                          &resources.reply_port);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- put_property_once: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state.stage = 1;
	}
	if(state.stage == 1) {
		if(resources.reply_port == NULL) {
			dprintf("binderfs(%d) -- put_property_once: "
			        "ERROR reply_port == NULL\n", (int)find_thread(NULL));
			err = B_ERROR;
			goto err;
		}
		binder_reply_put_property reply;
		err = get_binder_reply(node, resources.reply_port, reflection,
		                       &reply, sizeof(reply));
		if(err < B_OK)
			goto err;
		if(reflection.active)
			return B_OK;
		pps->result = reply.put_result;
	}
	err = B_OK;
err:
	resources.reply_port = NULL;
	state.stage = 0;
	return err;
}

static status_t
put_property(vnode *base_node, binder_cmd *cmd, transaction_state *full_state)
{
	state_put_property &state = full_state->put_property;
	transaction_resources &resources = full_state->resources;

	status_t err = B_OK;
	if(state.stage == 0) {
		if(resources.nodes != NULL) {
			dprintf("binderfs -- put_property: nodes != NULL in stage 0\n");
			return B_ERROR;
		}
		resources.nodes = get_call_order_node_list(base_node);
		if(resources.nodes == NULL)
			return B_NO_MEMORY;
		state.stage = 1;
		state.index = 0;
	}
	while(state.index < resources.nodes->count) {
		if(state.stage == 1) {
			vnode *node = resources.nodes->node[state.index];
			if(node->host == cmd->reflection.team) {
#if TRACE_REFLECTION
				dprintf("binderfs(%d) -- put_property: reflect put\n",
				        (int)find_thread(NULL));
#endif
				cmd->reflection.add_cmd(REFLECT_CMD_PUT,
				                        node->vnid.address.token);
				state.stage = 2;
				return B_OK;
			}
			err = put_property_once(node, &cmd->data.put, cmd->reflection,
			                        state.put_property_once, resources);
			if(cmd->reflection.active)
				return B_OK;
			if(err < B_OK) {
				cmd->data.get.result.error = err;
				cmd->data.put.result.fallthrough = true;
			}
			state.stage = 2;
		}
		if(state.stage == 2) {
			if(!cmd->data.get.result.error)
				goto done;
			if(!cmd->data.put.result.fallthrough)
				goto done;
			state.index++;
			state.stage = 1;
		}
	}
	cmd->data.put.result.error = ENOENT;	
	cmd->data.put.result.fallthrough = false;
done:
	free_node_list(resources.nodes);
	resources.nodes = NULL;
	state.stage = 0;
	return B_OK;
}

static status_t
one_notify_callback(vnode *node, notify_callback_struct *ncs)
{
	char str[280];
	sprintf(str,"%08x%s",(unsigned int)ncs->event,ncs->name);

	//dprintf("binderfs(%d) -- notify_callback: notify listeners on %016Lx\n",
	//        (int)find_thread(NULL), node->vnid.value);
	//dprintf("binderfs(%d) -- notify_callback: %s\n",
	//        (int)find_thread(NULL), str);

	return notify_listener(B_ATTR_CHANGED, node->ns->nsid, 0, 0,
	                       node->vnid.value, str);
}

static status_t
notify_callback(vnode *node, notify_callback_struct *ncs, bool lockEm)
{
	status_t err;
	node_list *list;
	//dprintf("binderfs(%d) -- notify_callback: node %016Lx\n",
	//        (int)find_thread(NULL), node->vnid.value);

	list = get_node_list(node, 0, linkFilter|linkInherit|linkAugment,
	                     false, lockEm);
	if(list == NULL)
		return B_NO_MEMORY;

	err = one_notify_callback(node, ncs);
	ncs->event = B_SOMETHING_CHANGED | (ncs->event & B_NAME_KNOWN);
	for(int i = 0; i < list->count; i++) {
		status_t tmp_err = one_notify_callback(list->node[i], ncs);
		if(tmp_err < B_OK)
			err = tmp_err;
	}
	free_node_list(list);
	return err;
}

#if 0
static int32 notify_callback(vnode *vn, notify_callback_struct *ncs, bool lockEm)
{
	int32 err = one_notify_callback(vn,ncs);
	uint32 event = ncs->event;
	links_copy links(vn,lockEm);
	for (int32 i=0;i<links.count;i++) {
		if (links.links[i].toFlags & (linkFilter|linkInherit|linkAugment)) {
			ncs->event = B_SOMETHING_CHANGED;
			if (event & B_NAME_KNOWN) ncs->event |= B_NAME_KNOWN;
			notify_callback(links.links[i].link,ncs,lockEm);
		}
	}
	
	return err;
}
#endif

static status_t
get_next_entry_reply(single_cookie *c, reflection_struct &reflection,
                     port_pool *reply_port)
{
	status_t err;
	binder_reply_next_entry *reply;
	size_t reply_size;

	err = get_binder_reply(c->vn, reply_port, reflection,
	                       (binder_reply_basic**)&reply, &reply_size);
	if(err < B_OK) {
		dprintf("binderfs(%d) -- get_next_entry_reply: get_binder_reply "
		        "failed %s\n", (int)find_thread(NULL), strerror(err));
		goto err1;
	}
	if(reflection.active)
		return B_OK;

	if(reply_size < sizeof(binder_reply_status)) {
		err = B_ERROR;
		goto err2;
	}
	if(reply->result < B_OK) {
		err = reply->result;
		goto err2;
	}
	if(reply_size < sizeof(binder_reply_next_entry)) {
		err = B_ERROR;
		goto err2;
	}
	if(reply_size != sizeof(binder_reply_next_entry) + reply->namelen) {
		err = B_ERROR;
		goto err2;
	}

	err = c->AllocateName(reply->namelen);
	if(err < B_OK)
		goto err2;
	memcpy(c->nextName, reply->name, reply->namelen);
	c->nextName[reply->namelen] = 0;
err2:
	free_binder_reply(reply);
err1:
	return err;
}

static status_t
open_properties_single(single_cookie *c, reflection_struct &reflection,
                       state_open_properties_single &state,
                       transaction_resources &resources)
{
	enum {
		STAGE1 = STAGE_OPEN_PROPERTIES_SINGLE_BASE,
		STAGE2
	};
	status_t err;
	c->err = B_ERROR;
	c->nextNameLen = 0;
	
	if(state.stage == STAGE_INIT) {
		err = send_binder_request(c->vn, BINDER_REQUEST_OPEN_PROPERTIES,
		                          &resources.reply_port);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- open_properties_single: send_binder_reques"
			        "t failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state.stage = STAGE1;
	}
	if(state.stage == STAGE1) {
		binder_reply_open_open_properties reply;
		err = get_binder_reply(c->vn, resources.reply_port, reflection,
		                       &reply, sizeof(reply));
		if(reflection.active)
			return B_OK;
		if(err < B_OK)
			goto err;
		resources.reply_port = NULL;
		if(reply.result < B_OK) {
			err = reply.result;
			goto err;
		}
		c->userCookie = reply.user_cookie;
		state.stage = STAGE2;
	}
	if(state.stage == STAGE2) {
		//err = ref_node(c->vn, reflection, state.ref_node);
		//if(err < B_OK)
		//	goto err;
		//if(reflection.active)
		//	return B_OK;
		c->opened = true;
		err = B_OK;
	}
	else {
		dprintf("binderfs(%d) -- open_properties_single: bad state, stage %d\n",
		        (int)find_thread(NULL), state.stage);
		err = B_ERROR;
	}
err:
	state.stage = STAGE_DONE;
	return err;
}

static status_t
get_next_entry(single_cookie *c, reflection_struct &reflection,
               state_get_next_entry &state, transaction_resources &resources)
{
	enum {
		STAGE1 = STAGE_GET_NEXT_ENTRY_BASE,
	};
	status_t err;
	if(state.stage == STAGE_INIT) {
		if(!c->opened) {
			err = B_ERROR;
			goto err;
		}
		err = send_binder_request(c->vn, BINDER_REQUEST_NEXT_PROPERTY,
		                          (uint32)c->userCookie, &resources.reply_port);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- get_next_entry: send_binder_request "
			        "failed, %s\n", (int)find_thread(NULL), strerror(err));
			goto err;
		}
		state.stage = STAGE1;
	}
	if(state.stage == STAGE1) {
		err = get_next_entry_reply(c, reflection, resources.reply_port);
		if(err < B_OK)
			goto err;
		if(reflection.active)
			return B_OK;
		err = B_OK;
	}
	else {
		dprintf("binderfs(%d) -- get_next_entry: bad state, stage %d\n",
		        (int)find_thread(NULL), state.stage);
		err = B_ERROR;
	}
err:
	resources.reply_port = NULL;
	state.stage = STAGE_DONE;
	return err;
}

static status_t
new_property_iterator(vnode *node, multi_cookie **mcp)
{
	node_list *list;
	list = get_node_list(node, linkAugment);
	if(list == NULL)
		return B_NO_MEMORY;

	multi_cookie *mc = (multi_cookie*)
		malloc(sizeof(multi_cookie) + sizeof(single_cookie) * (list->count-1));
	if(mc == NULL)
		return B_NO_MEMORY;
	mc->count = list->count;
	mc->client = -1;
	mc->opened = false;
	for(int i = 0; i < mc->count; i++) {
		mc->cookies[i].vn = list->node[i];
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
open_properties(vnode *node, binder_cmd *cmd)
{
	status_t err;
	int fd;

	cmd->data.property_iterator.fd = -1;
	err = fd = sys_open_vn(false, node->ns->nsid, node->vnid.value, NULL, O_RDWR, true);
	if(err < B_OK)
		goto err1;
	err = sys_ioctl(false, fd, BINDER_INIT_PROPERTY_ITERATOR, &fd, 0);
	if(err < B_OK)
		goto err2;
	cmd->data.property_iterator.fd = fd;
	return B_OK;

err2:
	sys_close(false, fd);
err1:
	return err;
}

static int
free_property_iterator_cookie(void *_cookie)
{
	status_t err;
	multi_cookie *mc = (multi_cookie*)_cookie;

	for(int i = 0; i < mc->count; i++) {
		single_cookie *sc = mc->cookies + i;
		if(sc->nextName)
			free(sc->nextName);
		if(sc->opened) {
			dprintf("binderfs -- free_property_iterator_cookie: node at %d "
			        "still open\n", i);
			if(mc->client != sc->vn->host) {
				port_pool *reply_port = NULL;
				err = send_binder_request(sc->vn,
				                          BINDER_REQUEST_CLOSE_PROPERTIES,
				                          (uint32)sc->userCookie, &reply_port);
				if(err < B_OK) {
					dprintf("binderfs(%d) -- free_property_iterator_cookie: "
					        "send_binder_request failed, %s\n",
					        (int)find_thread(NULL), strerror(err));
				}
				else {
					reflection_struct reflection;
					reflection.count = 1;
					while(reflection.count > 0) {
						reflection.count = 0;
						reflection.active = false;
						err = get_binder_reply(sc->vn, reply_port, reflection);
						if(reflection.active) {
							dprintf("binderfs(%d) -- free_property_iterator_"
							        "cookie: ignored relection\n",
							        (int)find_thread(NULL));
						}
					}
				}
			}
		}
		unref_node(sc->vn);
	}
	free(mc);
	return B_OK;
}

static status_t
process_reflected_next_property(binder_cmd *cmd, single_cookie *sc)
{
	status_t err;
	
	sc->err = cmd->data.property_iterator.result;
	if(sc->err < B_OK)
		return B_OK;
	if(!sc->opened) {
		sc->userCookie = cmd->data.property_iterator.local_cookie;
		sc->opened = true;
	}

	int len = cmd->data.property_iterator.len;
	err = sc->AllocateName(len);
	if(err < B_OK)
		return err;
	strncpy(sc->nextName, cmd->data.property_iterator.name, len);
	sc->nextName[len] = '\0';
	//dprintf("binderfs -- process_reflected_next_property: got %s\n", sc->nextName);
	return B_OK;
}

static int
copystring(char *dest, const char *src, int maxlen)
{
	int len = 0;
	if(src == NULL)
		dprintf("binderfs -- copystring: src == NULL\n");
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
                    state_open_properties_all &state,
                    transaction_resources &resources)
{
	enum {
		STAGE_OPEN_SINGLE = STATE_OPEN_PROPERTIES_ALL_BASE,
		STAGE_OPEN_SINGLE_REFLECTED,
		STAGE_WAIT_FOR_SINGLE,
		STATE_OPEN_PROPERTIES_ALL_END
	};
	
	if(state.stage == STAGE_INIT) {
		if(state.index != 0) {
			dprintf("binderfs(%d) -- open_properties_all: bad state, "
			        "stage == 0, index == %d\n",
			        (int)find_thread(NULL), state.index);
			return B_ERROR;
		}
		state.stage = STAGE_OPEN_SINGLE;
	}
	else if(state.stage < STATE_OPEN_PROPERTIES_ALL_BASE ||
	        state.stage >= STATE_OPEN_PROPERTIES_ALL_END) {
		dprintf("binderfs(%d) -- open_properties_all: bad state, "
		        "stage == %d\n",
		        (int)find_thread(NULL), state.stage);
		return B_ERROR;
	}

	//dprintf("binderfs(%d) -- next_property: open %d-%d\n",
	//        (int)find_thread(NULL), state.stage_0.index, (int)mc->count);
	while(state.index < mc->count) {
		single_cookie *sc = mc->cookies + state.index;
		if(state.stage == STAGE_OPEN_SINGLE) {
			if(sc->vn->host == cmd->reflection.team) {
				mc->client = cmd->reflection.team;
				cmd->data.property_iterator.local_cookie = sc->userCookie;
				cmd->reflection.add_cmd(REFLECT_CMD_OPEN_PROPERTIES,
				                        sc->vn->vnid.address.token);
#if TRACE_REFLECTION
				dprintf("binderfs(%d) -- next_property: reflect "
				        "open_properties\n", (int)find_thread(NULL));
#endif
				state.stage = STAGE_OPEN_SINGLE_REFLECTED;
				return B_OK;
			}
			sc->err = open_properties_single(sc, cmd->reflection,
			                                 state.open_properties_single,
                                             resources);
			if(cmd->reflection.active)
				return B_OK;
			state.stage = STAGE_WAIT_FOR_SINGLE;
		}
		if(state.stage == STAGE_WAIT_FOR_SINGLE) {
			sc->err = get_next_entry(sc, cmd->reflection, state.get_next_entry,
			                         resources);
			if(cmd->reflection.active)
				return B_OK;
		}
		if(state.stage == STAGE_OPEN_SINGLE_REFLECTED) {
			process_reflected_next_property(cmd, sc);
		}
		state.stage = STAGE_OPEN_SINGLE;
		state.index++;
	}
	mc->opened = true;
	state.index = 0;
	state.stage = STAGE_DONE;
	return B_OK;
}

static status_t
next_property(binder_cmd *cmd, file_cookie *cookie,
              transaction_state *full_state)
{
	state_next_property &state = full_state->next_property;
	transaction_resources &resources = full_state->resources;
	enum {
		STAGE_OPEN = STATE_NEXT_PROPERTY_BASE,
		STAGE_START_FIND,
		STAGE_FIND,
		STAGE_RETURN_NEXT,
		STAGE_REFLECT_NEXT,
		STATE_NEXT_PROPERTY_END,

		SUB_STAGE_GET_NEXT,
		SUB_STAGE_REFLECT_GET_NEXT,
		SUB_STAGE_DONE
	};
	status_t err = B_ERROR;
	multi_cookie *mc = cookie->get_property_iterator();
	if(mc == NULL) {
		dprintf("binderfs -- next_property: multi_cookie == NULL\n");
		return B_ERROR;
	}
	
	//dprintf("binderfs(%d) -- next_property: node count %d stage %d\n",
	//        (int)find_thread(NULL), (int)mc->count, state.stage);

	if(mc->count <= 0) {
		return ENOENT;
	}

	if(state.stage == STAGE_INIT) {
		if(!mc->opened)
			state.stage = STAGE_OPEN;
		else
			state.stage = STAGE_START_FIND;
	}
	else if(state.stage < STATE_NEXT_PROPERTY_BASE ||
	        state.stage >= STATE_NEXT_PROPERTY_END) {
		dprintf("binderfs(%d) -- next_property: bad state, "
		        "stage == %d\n",
		        (int)find_thread(NULL), state.stage);
	}
	if(state.stage == STAGE_OPEN) {
		err = open_properties_all(mc, cmd, state.open_properties_all,
		                          resources);
		if(err < B_OK)
			goto err;
		if(cmd->reflection.active)
			return B_OK;
		state.stage = STAGE_START_FIND;
	}
	if(state.stage == STAGE_START_FIND) {
		state.stage_find.index = 0;
		state.stage_find.next_index = -1;
		state.stage = STAGE_FIND;
	}
	if(state.stage == STAGE_FIND) {
		single_cookie *sc_next = NULL;
		if(state.stage_find.next_index >= 0)
			sc_next = mc->cookies + state.stage_find.next_index;
		single_cookie *sc = mc->cookies + state.stage_find.index;
		while(state.stage_find.index < mc->count) {
			if(state.stage_find.stage == STAGE_INIT) {
				state.stage_find.stage = SUB_STAGE_DONE;
				if(sc->err == B_OK) {
					if(state.stage_find.next_index < 0) {
						state.stage_find.next_index = state.stage_find.index;
						sc_next = sc;
					}
					else {
						int32 cmp = strcmp(sc_next->nextName, sc->nextName);
						if(cmp == 0) {
							state.stage_find.stage = SUB_STAGE_GET_NEXT;
						}
						else if(cmp > 0) {
							state.stage_find.next_index = state.stage_find.index;
							sc_next = sc;
						}
					}
				}
			}
			if(state.stage_find.stage == SUB_STAGE_GET_NEXT) {
				if(sc->vn->host == cmd->reflection.team) {
					mc->client = cmd->reflection.team;
					cmd->data.property_iterator.local_cookie =
						sc->userCookie;
					cmd->reflection.add_cmd(REFLECT_CMD_NEXT_PROPERTY,
					                        sc->vn->vnid.address.token);
#if TRACE_REFLECTION
					dprintf("binderfs(%d) -- next_property: reflect "
					        "next_property\n", (int)find_thread(NULL));
#endif
					state.stage_find.stage = SUB_STAGE_REFLECT_GET_NEXT;
					return B_OK;
				}
				sc->err = get_next_entry(sc, cmd->reflection,
				                         state.stage_find.get_next_entry,
				                         resources);
				if(cmd->reflection.active)
					return B_OK;
				state.stage_find.stage = 0;
			}
			if(state.stage_find.stage == SUB_STAGE_REFLECT_GET_NEXT) {
				process_reflected_next_property(cmd, sc);
				state.stage_find.stage = STAGE_DONE;
			}
			if(state.stage_find.stage == SUB_STAGE_DONE) {
				state.stage_find.stage = STAGE_DONE;
				state.stage_find.index++;
				sc++;
			}
			if(state.stage_find.stage != STAGE_DONE) {
				dprintf("binderfs(%d) -- next_property: bad state, "
				        "state.stage_find.stage == %d\n",
				        (int)find_thread(NULL), state.stage_find.stage);
				return B_ERROR;
			}
		}
		state.stage_find.index = 0;
		state.stage = STAGE_RETURN_NEXT;
	}
	if(state.stage == STAGE_RETURN_NEXT) {
		if(state.stage_find.next_index >= 0) {
			if(state.stage_find.next_index >= mc->count) {
				dprintf("binderfs(%d) -- next_property: next_index bad, %d\n",
				        (int)find_thread(NULL), state.stage_find.next_index);
				return B_ERROR;
			}
			single_cookie *sc = mc->cookies + state.stage_find.next_index;
			//strncpy(cmd->data.property_iterator.name,
			copystring(cmd->data.property_iterator.name, sc->nextName,
			           sizeof(cmd->data.property_iterator.name) - 1);
			//dprintf("binderfs(%d) -- next_property: found name \"%s\" at %d\n",
			//        (int)find_thread(NULL), cmd->data.property_iterator.name,
			//        state.stage_find.next_index);
	
			if(sc->vn->host == cmd->reflection.team) {
				cmd->data.property_iterator.local_cookie = sc->userCookie;
				cmd->reflection.add_cmd(REFLECT_CMD_NEXT_PROPERTY,
				                        sc->vn->vnid.address.token);
#if TRACE_REFLECTION
					dprintf("binderfs(%d) -- next_property: reflect "
					        "next_property\n", (int)find_thread(NULL));
#endif
				state.stage = STAGE_REFLECT_NEXT;
				return B_OK;
			}

			sc->err = get_next_entry(sc, cmd->reflection,
			                         state.stage_find.get_next_entry,
			                         resources);
			//dprintf("binderfs(%d) -- next_property: get_next_entry, %s\n",
			//        (int)find_thread(NULL), strerror(sc->err));
			if(cmd->reflection.active)
				return B_OK;
			
			err = B_OK;
		}
		else {
			err = ENOENT;		
		}
		state.stage_find.next_index = 0;
	}
	if(state.stage == STAGE_REFLECT_NEXT) {
		if(state.stage_find.next_index >= mc->count) {
			dprintf("binderfs(%d) -- next_property: next_index bad, %d\n",
			        (int)find_thread(NULL), state.stage_find.next_index);
			return B_ERROR;
		}
		single_cookie *sc = mc->cookies + state.stage_find.next_index;
		if(sc->nextName == NULL)
			dprintf("binderfs(%d) -- next_property: stage 3 index %d, "
			        "nextName = NULL\n",
			        (int)find_thread(NULL), state.stage_find.next_index);
		else {
			char *tmp_str = strdup(sc->nextName);
			if(tmp_str == NULL) {
				dprintf("binderfs(%d) -- next_property: could not allocate "
				        "string\n", (int)find_thread(NULL));
				state.stage_find.next_index = 0;
				err = B_NO_MEMORY;
				goto err;
			}
			process_reflected_next_property(cmd, sc);
			copystring(cmd->data.property_iterator.name, tmp_str,
			           sizeof(cmd->data.property_iterator.name) - 1);
			free(tmp_str);
		}
		state.stage = STAGE_DONE;
		state.stage_find.next_index = 0;
		err = B_OK;
	}
err:
	state.stage = STAGE_DONE;
	return err;
}

static status_t
close_properties(binder_cmd *cmd, file_cookie *cookie,
                 transaction_state *full_state)
{
	state_close_properties &state = full_state->close_properties;
	transaction_resources &resources = full_state->resources;

	multi_cookie *mc = cookie->get_property_iterator();
	if(mc == NULL) {
		dprintf("binderfs -- close_properties: multi_cookie == NULL\n");
		return B_ERROR;
	}
	if (!mc->count) {
		cmd->data.property_iterator.result = ENOENT;
		return B_OK;
	}
	if(mc->fd < 0) {
		dprintf("binderfs -- close_properties: bad fd\n");
		return B_ERROR;
	}

	while(state.index < mc->count) {
		single_cookie *sc = mc->cookies + state.index;
		if(sc->opened) {
			if(state.stage == 0) {
				if(mc->client == sc->vn->host) {
					cmd->data.property_iterator.local_cookie = sc->userCookie;
					cmd->reflection.add_cmd(REFLECT_CMD_CLOSE_PROPERTIES,
					                        sc->vn->vnid.address.token);
#if TRACE_REFLECTION
					dprintf("binderfs(%d) -- close_properties: reflect\n",
					        (int)find_thread(NULL));
#endif
					state.stage = 2;
					return B_OK;
				}
				else {
					status_t err;
					err = send_binder_request(sc->vn,
					                          BINDER_REQUEST_CLOSE_PROPERTIES,
					                          (uint32)sc->userCookie,
					                          &resources.reply_port);
					if(err < B_OK) {
						dprintf("binderfs(%d) -- close_properties: "
						        "send_binder_request failed, %s\n",
						        (int)find_thread(NULL), strerror(err));
					}
					else
						state.stage = 1;
				}
			}
			if(state.stage == 1) {
				status_t err;
				err = get_binder_reply(sc->vn, resources.reply_port,
				                       cmd->reflection);
				if(cmd->reflection.active)
					return B_OK;
				resources.reply_port = NULL;
				if(err == B_OK)
					sc->opened = false;
			}
			if(state.stage == 2) {
				sc->opened = false;
			}
			state.stage = 0;
			//unref_node(sc->vn);
		}
		state.index++;
	}
	state.index = 0;
	sys_close(false, mc->fd);
	cmd->data.property_iterator.result = B_OK;
	return B_OK;
}


static int
uspacefs_read_vnode(void *_ns, vnode_id vnid, char r, void **node)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *vn = NULL;

	if (vnid == ns->rootID.value) {
		*node = ns->root;
		return B_OK;
	}

	vn = (vnode*)malloc(sizeof(vnode));
	if(vn == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}
	vn->ns = ns;
	vn->links = NULL;
	vn->linkCount = 0;
	vn->refCount = 0;
	vn->flags = 0;
	vn->vnid.value = vnid;
	vn->host = -1;
	port_info pi;
	get_port_info(vn->vnid.address.port,&pi);
	vn->host = pi.team;
	vn->tmp_ptr = NULL;

#if TRACE_REFS
	{
		int total_count = atomic_add(&vn->ns->vnode_count, 1) + 1;
		dprintf("binderfs(%d) -- read_vnode: %016Lx active nodes %d\n",
		        (int)find_thread(NULL), vnid, total_count);
	}
#endif

	*node = vn;
	return B_OK;

//err2:
//	free(vn);
err1:
	return err;
}

static int
uspacefs_write_vnode(void *_ns, void *_vn, char r)
{
	int32 err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *vn = (vnode*)_vn;

	if (vn == ns->root) return B_OK;

#if TRACE_REFS
	{
		int total_count = atomic_add(&vn->ns->vnode_count, -1) - 1;
		dprintf("binderfs(%d) -- write_vnode: %016Lx active nodes %d\n",
		        (int)find_thread(NULL), vn->vnid.value, total_count);
	}
#endif

	if(vn->tmp_ptr != NULL)
		dprintf("binderfs -- uspacefs_write_vnode: %016Lx tmp_ptr != NULL\n",
		        vn->vnid.value);
	if(vn->refCount != 0)
		dprintf("binderfs -- uspacefs_write_vnode: %016Lx refCount != 0, %x\n",
		        vn->vnid.value, (int)vn->refCount);
	unlink_node(vn);
	if (vn->links) free(vn->links);
	free(vn);

	return err;
}

static int
uspacefs_remove_vnode(void *_ns, void *_vn, char r)
{
	/* this should not be called as we never call remove_vnode */
	dprintf("binderfs(%d): ERROR uspacefs_remove_vnode was called\n",
	        (int)find_thread(NULL));
	return uspacefs_write_vnode(_ns, _vn, r);
}

static int
uspacefs_secure_vnode(void */*_ns*/, void */*_node*/)
{
	int32 err = 0;
	return err;
}

static int
uspacefs_walk(void *_ns, void *_base, const char *filename,
		char **/*newpath*/, vnode_id *vnid)
{
	int32 err = 0;
//	int32 size;
	nspace *ns = (nspace*)_ns;
	vnode *fred,*base = (vnode*)_base;
	//char path[256];
/*
	printf("walking: base:0x%08x(%d:%d), name:'%s'\n",
		base,base->vnid.address.port,base->vnid.address.token,filename);
*/
	if ((filename[0] == 0) || (filename[0] == '/')) return EINVAL;

	if (!strcmp(filename,"."))
		*vnid = base->vnid.value;
	else {
		return ENOENT;
	}
	
	err = get_vnode(ns->nsid,*vnid,(void**)&fred);

//noget:;
//err1:
	return err;
}

static int
uspacefs_access(void */*_ns*/, void */*_node*/, int /*mode*/)
{
	return 0;
}

static int
uspacefs_create(void */*_ns*/, void */*_dir*/, const char */*name*/,
			  int /*omode*/, int /*perms*/, vnode_id */*vnid*/, void **/*cookie*/)
{
	return EPERM;
}

static int
uspacefs_rstat(void *_ns, void *_node, struct stat *st)
{
	//int32 tmp,err = 0;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;

	st->st_dev = ns->nsid;
	st->st_ino = node->vnid.value;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_blksize = 64 * 1024;
	st->st_crtime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	st->st_atime = 0;
	st->st_mode = S_IFDIR;
	st->st_size = 0;

	return 0;
}

static int
uspacefs_wstat(void */*_ns*/, void */*_node*/, struct stat */*st*/, long /*mask*/)
{
	return EPERM;
}

static int
uspacefs_open(void */*_ns*/, void *_node, int /*omode*/, void **cookie)
{
	file_cookie *c;
	vnode *node = (vnode*)_node;

	//dprintf("binderfs: open %Lx, omode %x\n", vn->vnid.value, omode);

	c = (file_cookie*)malloc(sizeof(file_cookie));
	if(c == NULL)
		return B_NO_MEMORY;
	c->type = FILE_COOKIE_UNUSED;
	*cookie = c;
#if TRACE_FDS
	dprintf("binderfs(%d) -- open: fd opened for %016Lx\n",
	        (int)find_thread(NULL), node->vnid.value);
#else
	node = node;
#endif
	return B_OK;
}

static int
uspacefs_close(void */*_ns*/, void *_node, void *_cookie)
{
	//int32 err = B_OK;
	//nspace *ns = (nspace*)_ns;
	vnode *vn = (vnode*)_node;
	file_cookie *c = (file_cookie*)_cookie;

	switch(c->type) {
		case FILE_COOKIE_HOST_CONNECTION: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: host fd closed for %016Lx\n",
			        (int)find_thread(NULL), vn->vnid.value);
#endif
			printf(("Hosting ceased for node %016Lx\n",vn->vnid.value));
#if TRACE_NODES
			dprintf("binderfs(%d) -- close: Hosting ceased for node %016Lx\n",
			        (int)find_thread(NULL), vn->vnid.value);
#endif
#if TRACE_REFS
			int32 old =
#endif
			atomic_add(&vn->refCount, -65535);
#if TRACE_REFS
			dprintf("binderfs(%d) -- close: %016Lx rc %x\n",
			        (int)find_thread(NULL), vn->vnid.value, (int)old - 65535);
#endif
			vn->flags = (vn->flags|vnodeDied) & ~vnodeHosted;
			destroy_link(vn,NULL,linkAll,true);	
			destroy_link(vn,NULL,linkAll,false);
	//		unlink_node(vn);
			unref_node(vn);
		} break;

		case FILE_COOKIE_CLIENT_CONNECTION: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: client fd closed for %016Lx\n",
			        (int)find_thread(NULL), vn->vnid.value);
#endif
			//dprintf("binderfs(%d) -- uspacefs_close: %016Lx, "
			//        "client connection terminated\n",
			//        (int)find_thread(NULL), vn->vnid.value);
			unref_node(vn);
		} break;

		case FILE_COOKIE_TRANSACTION_STATE: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: transaction state fd closed for "
			        "%016Lx\n", (int)find_thread(NULL), vn->vnid.value);
#endif
		} break;

		case FILE_COOKIE_PROPERTY_ITERATOR: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: property iterator fd closed for "
			        "%016Lx\n", (int)find_thread(NULL), vn->vnid.value);
#endif
		} break;

		case FILE_COOKIE_BUFFER: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: property buffer fd closed for "
			        "%016Lx\n", (int)find_thread(NULL), vn->vnid.value);
#endif
		} break;

		case FILE_COOKIE_UNUSED: {
#if TRACE_FDS
			dprintf("binderfs(%d) -- close: unused fd closed for "
			        "%016Lx\n", (int)find_thread(NULL), vn->vnid.value);
#endif
		} break;

		default:
			dprintf("binderfs(%d) -- close: fd of unknown type %d closed for %0"
			        "16Lx\n", (int)find_thread(NULL), c->type, vn->vnid.value);
	}
	return B_OK;
}

static int
uspacefs_free_cookie(void *_ns, void */*_node*/, void *_cookie)
{
	nspace *ns = (nspace*)_ns;
	file_cookie *c = (file_cookie*)_cookie;
	if (c) {
		c->clear(ns);
		free(c);
	}
	return 0;
}

static int
uspacefs_read(void */*_ns*/, void */*_node*/, void *_cookie, off_t pos,
		void *buf, size_t *len)
{
	//nspace *ns = (nspace*)_ns;
	//vnode *node = (vnode*)_node;
	file_cookie *c = (file_cookie*)_cookie;
	
	if(c->type == FILE_COOKIE_BUFFER &&
	   c->data.buffer.buffer && c->data.buffer.bufferSize) {
		int32 size = *len;
		if(pos + size > c->data.buffer.bufferSize)
			size = c->data.buffer.bufferSize - pos;
		if(size > 0) {
			*len = size;
			memcpy(buf, c->data.buffer.buffer + pos, size);
			return B_OK;
		}
	}
	*len = 0;
	return EPERM;
};

static int
uspacefs_write(void *_ns, void *_node, void */*_cookie*/, off_t /*pos*/,
		const void *buf, size_t *len)
{
	int32 err = EPERM;
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	//file_cookie *c = (file_cookie*)_cookie;
	if(node == ns->root) {
		if(*len == 5 && strncmp("dump\n", (const char *)buf, 5) == 0) {
			dump_port_state(ns);
			return B_OK;
		}
	}
	return err;
}

const char * ioctl_name[] = {
	"BINDER_NOTIFY_CALLBACK",
	"BINDER_GET_PROPERTY",
	"BINDER_PUT_PROPERTY",
	"BINDER_STACK",
	"BINDER_INHERIT",
	"BINDER_START_HOSTING",
	"BINDER_BREAK_LINKS",
	"BINDER_CONNECT",
	"BINDER_OPEN_PROPERTIES",
	"BINDER_NEXT_PROPERTY",
	"BINDER_CLOSE_PROPERTIES",
	"BINDER_OPEN_NODE"
};

#if 0
static int
start_hosting_root(vnode *node, file_cookie *c, binder_cmd */*cmd*/)
{
	if(c->type == FILE_COOKIE_HOST_CONNECTION)
		return B_OK;
	if(node->refCount >= 65536)
		return B_ERROR;
	printf(("Hosting started for node %016Lx\n",node->vnid.value));
	c->type = FILE_COOKIE_HOST_CONNECTION;
	node->flags |= vnodeHosted;
	int32 old = atomic_add(&node->refCount, 65535);
#if TRACE_REFS
	dprintf("binderfs(%d) -- start_hosting_root: %016Lx rc %x\n",
	        (int)find_thread(NULL), node->vnid.value, (int)old + 65536);
#endif
	if(old > 1)
		return 1;
	return B_OK;
}

#endif

static status_t
start_hosting(vnode *node, binder_cmd *cmd, file_cookie *c,
              state_start_hosting &state)
{
	enum {
		STAGE_REFLECT_ACQUIRE = STATE_START_HOSTING_BASE,
		STATE_START_HOSTING_END
	};
	if(state.stage == STAGE_INIT) {
		if(node->host != current_team()) {
			dprintf("binderfs(%d) -- start_hosting: node %016Lx, wrong team\n",
			        (int)find_thread(NULL), node->vnid.value);
			return B_ERROR;
		}
		if(node->flags & vnodeHosted) {
			dprintf("binderfs(%d) -- start_hosting: node %016Lx already "
			        "hosted\n", (int)find_thread(NULL), node->vnid.value);
			return B_BAD_VALUE;
		}
		if(c->type != FILE_COOKIE_UNUSED) {
			dprintf("binderfs(%d) -- start_hosting: fd already in use, "
			        "type %d\n", (int)find_thread(NULL), c->type);
			return B_BAD_VALUE;
		}
		node->flags = (node->flags | vnodeHosted) & ~vnodeDied;

		c->type = FILE_COOKIE_HOST_CONNECTION;
		vnode *dummy;
		get_vnode(node->ns->nsid, node->vnid.value,(void**)&dummy);
		int32 old = atomic_add(&node->refCount, 65536);
#if TRACE_REFS
		int total_refs = atomic_add(&node->ns->node_ref_count, 1) + 1;
		dprintf("binderfs(%d) -- start_hosting: %016Lx rc %x (total %d)\n",
		        (int)find_thread(NULL), node->vnid.value, (int)old+65536,
		        total_refs);
#endif
#if TRACE_NODES
		if(old == 0) {
			int total_count = atomic_add(&node->ns->node_count, 1) + 1;
			dprintf("binderfs(%d) -- start_hosting: hosted node created %016Lx,"
			        " nodes %d\n",
			        (int)find_thread(NULL), node->vnid.value, total_count);
		}
		else {
			dprintf("binderfs(%d) -- start_hosting: start hosting for node "
			        "%016Lx\n", (int)find_thread(NULL), node->vnid.value);
		}
#endif
		if(old > 0) {
#if TRACE_REFLECTION
			dprintf("binderfs(%d) -- start_hosting: node %016Lx, reflect "
			        "acquire\n", (int)find_thread(NULL), node->vnid.value);
#endif
			cmd->reflection.add_cmd(REFLECT_CMD_ACQUIRE,
			                        node->vnid.address.token);
			state.stage = STAGE_REFLECT_ACQUIRE;
			return B_OK;
		}
	}
	else if(state.stage < STATE_START_HOSTING_BASE ||
	        state.stage >= STATE_START_HOSTING_END) {
		dprintf("binderfs(%d) -- start_hosting: bad state, stage == %d\n",
		        (int)find_thread(NULL), state.stage);
		return B_ERROR;
	}

	state.stage = STAGE_DONE;
	return B_OK;
}

static status_t
stop_hosting(vnode *node, binder_cmd *cmd, file_cookie *c)
{
#if TRACE_NODES
	dprintf("binderfs(%d) -- stop_hosting: node %016Lx\n",
	        (int)find_thread(NULL), vn->vnid.value);
#endif
retry:
	int32 old = atomic_add(&node->refCount, -65535);
#if TRACE_REFS
	dprintf("binderfs(%d) -- stop_hosting: %016Lx rc %x\n",
	        (int)find_thread(NULL), node->vnid.value, (int)old - 65535);
#endif
	if(old > 65536) {
		old = atomic_add(&node->refCount, 65535); // not too safe
		if(old == 1) {
			dprintf("binderfs(%d) -- stop_hosting: %016Lx node refrenced "
			        "then not, retry\n",
			        (int)find_thread(NULL), node->vnid.value);
			goto retry;
		}

		dprintf("binderfs(%d) -- stop_hosting: %016Lx node refrenced, abort\n",
		        (int)find_thread(NULL), node->vnid.value);
		return B_NOT_ALLOWED;
	}
	node->flags = (node->flags|vnodeDied) & ~vnodeHosted;
	destroy_link(node,NULL,linkAll,true);	
	destroy_link(node,NULL,linkAll,false);
	unref_node(node);
	c->type = FILE_COOKIE_UNUSED;
	return B_OK;
}


static status_t
connect(vnode *node, file_cookie *c, reflection_struct *reflection,
        state_connect *state, transaction_resources *resources)
{
	status_t err;
	if(node->flags & vnodeDied) {
		return B_BAD_VALUE;
	}
	err = ref_node(node, *reflection, state->ref_node, *resources);
	if(err < B_OK)
		return err;
	c->type = FILE_COOKIE_CLIENT_CONNECTION;
	if(reflection->active)
		return B_OK;
	return B_OK;
}

static transaction_state *
start_transaction(vnode */*node*/, reflection_struct &reflection, int command_type)
{
	transaction_state *trans;
	if (reflection.state == -1) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		reflection.team = ti.team;
		trans = new_transaction_state(command_type);
	} else {
		status_t err;
		err = sys_ioctl(false, reflection.state, BINDER_FETCH_TRANSACTION_STATE,
		                &trans, sizeof(trans));
		if(err < B_OK)
			return NULL;
	}
	
	return trans;
}

static status_t
reflect_transaction(vnode *node, reflection_struct &reflection,
                    transaction_state *trans)
{
	status_t err;

	if(reflection.state >= 0)
		return B_OK;
	reflection.state = sys_open_vn(false, node->ns->nsid,
	                               node->ns->rootID.value, NULL,
	                               O_RDWR | O_CLOEXEC, true);
	//dprintf("binderfs: reflect_transaction, vn = %Lx, fd = %d\n",
	//        node->ns->rootID.value, (int)reflection.state);
	if(reflection.state < 0)
		return reflection.state;

	err = sys_ioctl(false, reflection.state, BINDER_ASSOCIATE_TRANSACTION_STATE,
	                trans, 0);
	if(err < B_OK) {
		dprintf("binderfs: reflect_transaction "
		        "BINDER_ASSOCIATE_TRANSACTION_STATE failed, fd = %d\n",
		        (int)reflection.state);
		sys_close(false, reflection.state);
		reflection.state = -1;
	}
	return err;
}

static void
cleanup_transaction(nspace *ns, reflection_struct &reflection,
                    transaction_state *trans)
{
	if(reflection.state >= 0)
		sys_close(false, reflection.state);
	else
		free_transaction_state(ns, trans);
}

static int
do_binder_command(vnode *node, file_cookie *c, binder_cmd *cmd)
{
	switch (cmd->command) {
		case BINDER_CONNECT:
		case BINDER_START_HOSTING:
		case BINDER_GET_PROPERTY:
		case BINDER_PUT_PROPERTY:
		case BINDER_NEXT_PROPERTY:
		case BINDER_CLOSE_PROPERTIES:
		case BINDER_STACK:
		case BINDER_INHERIT:
		case BINDER_BREAK_LINKS:
		case BINDER_STOP_HOSTING:
			break;

		case BINDER_NOTIFY_CALLBACK:	return notify_callback(node,&cmd->data.notify);
		case BINDER_OPEN_PROPERTIES:	return open_properties(node,cmd);
		//case BINDER_OPEN_NODE:          return open_node(node->ns, cmd);
		default:						return B_BAD_VALUE;
	}

	status_t err;
	transaction_state *trans;
	trans = start_transaction(node, cmd->reflection, cmd->command);
	if(trans == NULL) {
		dprintf("binderfs -- do_binder_command: trans == NULL\n");
		return B_ERROR;
	}
	if(trans->command_type != cmd->command) {
		dprintf("binderfs: transaction command does not match current command\n");
	}
	switch (cmd->command) {
		case BINDER_CONNECT:
			err = connect(node, c, &cmd->reflection, &trans->connect,
			              &trans->resources);
			break;

		case BINDER_START_HOSTING:
			//if(node->vnid.value == node->ns->rootID.value)
			//	err = start_hosting_root(node,c,cmd);
			//else
				err = start_hosting(node, cmd, c, trans->start_hosting);
			break;
		
		case BINDER_STOP_HOSTING:
			err = stop_hosting(node, cmd, c);
			break;
			
		case BINDER_STACK:
			err = link_nodes(node->ns, node, cmd, trans,
			                 linkFilter | linkAugment);
			break;

		case BINDER_INHERIT:
			err = link_nodes(node->ns, node, cmd, trans,
			                 linkInherit | linkAugment);
			break;

		case BINDER_BREAK_LINKS:
			err = break_links(node->ns, node, cmd, trans);
			break;

		case BINDER_GET_PROPERTY:
			err = get_property(node, cmd, trans);
			break;

		case BINDER_PUT_PROPERTY:
			err = put_property(node, cmd, trans);
			break;

		case BINDER_NEXT_PROPERTY:
			err = next_property(cmd, c, trans);
			break;

		case BINDER_CLOSE_PROPERTIES:
			err = close_properties(cmd, c, trans);
			break;

		default:
			dprintf("binderfs: do_binder_command called with invalid command\n");
			err = B_ERROR;
	}
	if(cmd->reflection.active) {
		err = reflect_transaction(node, cmd->reflection, trans);
		return err;
	}
	cleanup_transaction(node->ns, cmd->reflection, trans);
	return err;
}

static int
uspacefs_ioctl(void *_ns, void *_node, void *_cookie, int precmd, void *buf, size_t /*len*/)
{  
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	file_cookie *c = (file_cookie*)_cookie;
	
	switch (precmd) {
		case BINDER_CMD: {
			status_t err;
			binder_cmd *cmd = (binder_cmd*)buf;
			cmd->reflection.count = 0;
			cmd->reflection.active = false;
#if TRACE_COMMANDS
			if (cmd->command < _BINDER_COMMAND_COUNT)
				dprintf("binderfs(%d) -- ioctl: %s%s\n",
				        (int)find_thread(NULL), ioctl_name[cmd->command],
				        cmd->reflection.state >= 0 ? " (reenter reflected)" : "");
	        else
	        	dprintf("binderfs(%d) -- ioctl: unknown command %d\n",
				        (int)find_thread(NULL), (int)cmd->command);
#endif
			err = do_binder_command(node,c,cmd);
#if TRACE_COMMANDS
			if (cmd->command < _BINDER_COMMAND_COUNT)
				dprintf("binderfs(%d) -- ioctl: %s done rc %d, %s\n",
				       (int)find_thread(NULL), ioctl_name[cmd->command],
				       cmd->reflection.count, strerror(err));
#endif
			return err;
		}

		case BINDER_ASSOCIATE_BUFFER:
			return c->associate_buffer(ns, (buffer_t*)buf);

		case BINDER_ASSOCIATE_TRANSACTION_STATE:
			c->associate_transaction_state(ns, (transaction_state*)buf);
			return B_OK;

		case BINDER_FETCH_TRANSACTION_STATE:
			*((transaction_state**)buf) = c->get_transaction_state();;
			return B_OK;

		case BINDER_INIT_PROPERTY_ITERATOR: {
			status_t err;
			multi_cookie *cookie;
			err = new_property_iterator(node, &cookie);
			if(err != B_OK)
				return err;
			cookie->fd = *((int*)buf);
			c->associate_property_iterator_state(ns, cookie);
			return B_OK;
		}
		
		case BINDER_REGISTER_REPLY_THREAD:
			return register_reply_thread(ns, (port_id)buf);

		case BINDER_UNREGISTER_REPLY_THREAD:
			return unregister_reply_thread(ns, (port_id)buf);
		
		case BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT:
			return optain_sync_reply_port(ns, (port_id)buf);
		
		case BINDER_FREE_SYNC_REPLY_PORT:
			return release_port(ns, (port_id)buf, find_thread(NULL));

		case BINDER_INTERNAL_CONNECT: {
			internal_connect_struct *cs = (internal_connect_struct *)buf;
			return connect(node, c, cs->reflection, cs->state, cs->resources);
		}

		default:
			return B_BAD_VALUE;
	}
}

void display_ns(nspace *ns)
{
	ns = ns;
	printf(("nspace struct {\n"));
	printf(("    nsid          = %Ld\n",(int64)ns->nsid));
	printf(("    rootID->value = 0x%016Lx\n",ns->rootID.value));
	printf(("    rootID->port  = %d\n",(int)ns->rootID.address.port));
	printf(("    rootID->token = %d\n",(int)ns->rootID.address.token));
	printf(("    root          = 0x%08x\n",(int)ns->root));
	printf(("    device        = '%s'\n",ns->device));
	printf(("    name          = '%s'\n",ns->name));
	printf(("    fsname        = '%s'\n",ns->fsname));
	printf(("    flags         = 0x%02x\n",(int)ns->flags));
	printf(("}\n"));
}

static int
uspacefs_mount(nspace_id nsid, const char *device, ulong /*flags*/, void *parms,
		size_t /*len*/, void **_ns, vnode_id *_vnid)
{
	status_t err = B_OK;
	//port_id sync_port = -1;
	port_info pi;
	mountparms *p = (mountparms*)parms;
	nspace *ns = (nspace*)malloc(sizeof(nspace));

	dprintf("binderfs: mounting\n");

	if(ns == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}

	ns->nsid = nsid;
	ns->rootID.address.port = p->port;
	ns->rootID.address.token = p->token;
	ns->inPortLock.c = 1;
	err = ns->inPortLock.s = create_sem(0,"registry inPort lock");
	if(err < B_NO_ERROR)
		goto err2;
	ns->inPortFree = NULL;
	ns->inPortActive = NULL;
	ns->name = NULL;
	ns->fsname = NULL;
	//ns->threadBitmap = NULL;
	ns->linkLock.c = 1;
	err = ns->linkLock.s = create_sem(0,"registry link lock");
	if(err < B_NO_ERROR)
		goto err3;
//	ns->linkLock.m_lockValue = -1;
	if (!device) device = "";
	ns->device = strdup(device);
	if(ns->device == NULL) {
		err = B_NO_MEMORY;
		goto err4;
	}

	if (!(ns->root = (vnode*)malloc(sizeof(vnode)))) { err = ENOMEM; goto err5; };
	ns->root->ns = ns;
	ns->root->links = NULL;
	ns->root->linkCount = 0;
	ns->root->refCount = 0; //65536;
	ns->root->flags = 0; //vnodeHosted;
	ns->root->vnid = ns->rootID;
	get_port_info(ns->root->vnid.address.port,&pi);
	ns->root->host = pi.team;

	ns->node_count = 0;
	ns->vnode_count = 1;
	ns->node_ref_count = 0;
#if 0
	{
		port_pool *reply_port = NULL;
		binder_reply_status reply;
		err = send_binder_request(ns->root, BINDER_REQUEST_MOUNT, &reply_port);
		if(err != B_OK)
			goto err6;
		reflection_struct reflection;
		reflection.count = 0;
		reflection.active = false;
		err = get_binder_reply(ns->root, reply_port, reflection,
		                       &reply, sizeof(reply));
		if(err != B_OK)
			goto err6;
		sync_port = reply.sync_port;
	}
	if(sync_port >= 0) {
		//send_data(thid,0,0,0);
		err = write_port_etc(sync_port, BINDER_SYNC_CODE, NULL, 0,
		                     B_TIMEOUT | B_CAN_INTERRUPT, 1000000);
		if(err < B_OK) {
			dprintf("binderfs(%d) -- uspacefs_mount: sync, "
			        "write port failed, port %d\n",
			        (int)find_thread(NULL), (int)sync_port);
		}
	}
#endif
	ns->flags = B_FS_IS_SHARED;
	ns->name = "registry";
	ns->fsname = "registryfs";

	display_ns(ns);
	
	system_info si;
	get_system_info(&si);
	//if (!(ns->threadBitmap = Bitmap::New(si.max_threads))) goto err8;

	if (new_vnode(nsid,ns->rootID.value,ns->root) != 0) goto err6;

	if (_vnid) *_vnid = ns->rootID.value;
	if (_ns) *_ns = ns;
	return B_OK;

#if 0
err8:
	if(err >= B_OK)
		err = B_ERROR;
	uspacefs_unmount(ns);	/* this is ugly */
	dprintf("binderfs: mount failed 2, %s\n", strerror(err));
	return err;
//err7:
	//free(ns->name);
#endif
err6:
	free(ns->root);
err5:
	free(ns->device);
//	if (ns->notifyThread >= 0) kill_thread(ns->notifyThread);
err4:
	delete_sem(ns->linkLock.s);
err3:
	delete_ports(ns);
err2:
	free(ns);
err1:
	dprintf("binderfs: mount failed, %s\n", strerror(err));
	return err;
}

static int
uspacefs_opendir(void *_ns, void *_node, void **/*cookie*/)
{
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode *)_node;

	if(node == ns->root) {
		return B_OK;
	}
	return ENOENT;
}

static int
uspacefs_closedir(void */*ns*/, void */*node*/, void */*cookie*/)
{
	return B_OK;
}

static int
uspacefs_free_dircookie(void */*ns*/, void */*node*/, void */*cookie*/)
{
	return B_OK;
}

static int
uspacefs_rewinddir(void */*ns*/, void */*node*/, void */*cookie*/)
{
	return B_OK;
}

static int
uspacefs_readdir(void */*ns*/, void */*node*/, void */*cookie*/, long */*num*/,
                 struct dirent */*buf*/, size_t /*bufsize*/)
{
	return ENOENT;
}

static int
uspacefs_rfsstat(void *_ns, struct fs_info * fss)
{
	nspace *ns = (nspace*)_ns;

	fss->dev = 0;
	fss->root = ns->rootID.value;
	fss->flags = ns->flags;
	fss->block_size = 0;
	fss->io_size = 0;
	fss->total_blocks = 0;
	fss->free_blocks = 0;
	strncpy(fss->device_name,ns->device,sizeof(fss->device_name));
	strncpy(fss->volume_name,ns->name,sizeof(fss->volume_name));
	strncpy(fss->fsh_name,ns->fsname,sizeof(fss->fsh_name));
	return 0;
}

static int
uspacefs_unmount(void *_ns)
{
//	int32 err = 0;
	nspace *ns = (nspace*)_ns;

#if 0	/* can't unmount with references anyway */
	port_pool *reply_port = NULL;
	err = send_binder_request(ns->root, BINDER_REQUEST_UNMOUNT, &reply_port);
	if(err == B_OK) {
		reflection_struct reflection;
		err = get_binder_reply(ns->root, reply_port, reflection);
	}
#endif
	
	free(ns->root);
	//free(ns->name);
	//free(ns->fsname);
	free(ns->device);
	//if (ns->threadBitmap) ns->threadBitmap->Delete();
	delete_ports(ns);
	free(ns);
	return B_OK;
}

static int
uspacefs_fsync(void */*_ns*/, void */*_node*/)
{
	return B_OK;
}

static int
uspacefs_sync(void */*_ns*/)
{
	return 0;
}

