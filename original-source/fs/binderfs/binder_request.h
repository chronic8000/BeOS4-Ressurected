#ifndef BINDER_REQUEST_H
#define BINDER_REQUEST_H

#include <kernel/OS.h>
#include "support_p/BinderPriv.h"

struct vnode;

struct binder_request_basic {
	uint32					request_size;
	port_id					reply_port;
	uint32					token;
	binder_request_type		type;
};

struct binder_request_one_arg : public binder_request_basic {
	uint32	arg;
};

struct binder_reply_basic {
	port_id		sync_port;
};

struct binder_reply_status : public binder_reply_basic {
	status_t	result;
};

struct binder_reply_get_property_status : public binder_reply_basic {
	_get_status_t     get_result;
};

struct binder_reply_get_property : public binder_reply_get_property_status {
	size_t            rlen;
	uint8             rbuf[0];
};

struct binder_reply_put_property : public binder_reply_basic {
	_put_status_t     put_result;
};

struct binder_reply_open_open_properties : public binder_reply_status {
	void             *user_cookie;
};

struct binder_reply_next_entry : public binder_reply_status {
	size_t            namelen;
	char              name[0];
};

/*
	fill in request header and send to a remote binder node
*/

struct vnode;
struct port_pool;

status_t send_binder_request(vnode *node, binder_request_basic *request,
                             size_t request_size, port_pool **reply_port);
status_t send_binder_request(vnode *node, binder_request_type type,
                             port_pool **reply_port);
status_t send_binder_request(vnode *node, binder_request_type type, uint32 arg,
                             port_pool **reply_port);
status_t send_binder_request(vnode *node, binder_request_type type,
                             const char *string, void *data, size_t data_size,
                             port_pool **reply_port);

status_t get_binder_reply(vnode *node, port_pool *reply_port,
                          reflection_struct &reflection);

status_t get_binder_reply(vnode *node, port_pool *reply_port,
                          reflection_struct &reflection,
                          binder_reply_basic *reply_buf,
                          size_t reply_buf_size);
status_t get_binder_reply(vnode *node, port_pool *reply_port,
                          reflection_struct &reflection,
                          binder_reply_basic **reply_buf,
                          size_t *reply_buf_size);
status_t free_binder_reply(binder_reply_basic *reply_buf);

#endif

