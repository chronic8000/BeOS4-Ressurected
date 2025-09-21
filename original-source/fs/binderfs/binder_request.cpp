#include "binder_request.h"
#include <drivers/KernelExport.h>
#include "binder_node.h"
#include "port_pool.h"
#include "debug_flags.h"

#define BINDER_REQUEST_TIMEOUT 10000000
#define BINDER_REQUEST_SHORT_TIMEOUT 1000000

static bool 
is_local(port_id port)
{
	port_info pi;
	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);
	get_port_info(port,&pi);
	return (ti.team == pi.team);
}

status_t 
send_binder_request(vnode *node, binder_request_basic *request,
                    size_t request_size, port_pool **reply_port)
{
	status_t err;
	bigtime_t timeout = BINDER_REQUEST_TIMEOUT;
	port_id outPort = node->vnid.address.port;

	if(reply_port != NULL && is_local(node->vnid.address.port)) {
		dprintf("binderfs(%d) -- send_binder_request: WARNING port %d "
		        "owned by current team\n"
		        "                  waitReply %d, port %d, thread %d, cmd %d\n",
		        (int)find_thread(NULL), (int)node->vnid.address.port,
		        reply_port != NULL,
		        (int)node->vnid.address.port, (int)find_thread(NULL),
		        request->type);
		timeout = BINDER_REQUEST_SHORT_TIMEOUT;
	}

	request->request_size = request_size;
	if(reply_port == NULL)
		request->reply_port = -1;
	else {
		if(*reply_port != NULL) {
			dprintf("binderfs(%d) -- send_binder_request: *reply_port not NULL,"
			        " %p\n", (int)find_thread(NULL), *reply_port);
			//kernel_debugger("binder_breakpoint");
			return B_ERROR;
		}
		err = obtain_port(node, &outPort, reply_port);
		if(err != B_OK)
			return err;
		request->reply_port = (*reply_port)->port;
	}
	request->token = node->vnid.address.token;
	
#if TRACE_COMMUNICATION
	dprintf("binderfs(%d) -- send_binder_request: write command to port %d\n",
	        (int)find_thread(NULL), (int)outPort);
#endif	
	err = write_port_etc(outPort, BINDER_REQUEST_CODE, request, request_size,
	                     B_TIMEOUT | B_CAN_INTERRUPT, timeout);
	if(err < B_OK) {
		if(reply_port != NULL)
			release_port(node, *reply_port);
		dprintf("binderfs(%d) -- send_binder_request: port write on %d failed, "
		        "%s\n", (int)find_thread(NULL), (int)outPort, strerror(err));
	}
	return err;
}

status_t 
send_binder_request(vnode *node, binder_request_type type,
                    port_pool **reply_port)
{
	binder_request_basic req;
	req.type = type;
	return send_binder_request(node, &req, sizeof(req), reply_port);
}

status_t 
send_binder_request(vnode *node, binder_request_type type, uint32 arg,
                    port_pool **reply_port)
{
	binder_request_one_arg req;
	req.type = type;
	req.arg = arg;
	return send_binder_request(node, &req, sizeof(req), reply_port);
}

status_t 
send_binder_request(vnode *node, binder_request_type type, const char *string,
                    void *data, size_t data_size, port_pool **reply_port)
{
	status_t err;
	int string_size = strlen(string);
	struct binder_request_128b : public binder_request_basic {
		uint8	data[128];
	};
	binder_request_128b *reqp;
	binder_request_128b small_req;
	size_t request_size = sizeof(binder_request_basic) +
	                      4 + string_size + 4 + data_size;
	if(request_size <= sizeof(binder_request_128b)) {
		reqp = &small_req;
	}
	else {
		reqp = (binder_request_128b *)malloc(request_size);
		if(reqp == NULL)
			return B_NO_MEMORY;
	}
	reqp->type = type;
	uint8 *datap = reqp->data;
	*((uint32*)datap) = string_size;
	datap += 4;
	memcpy(datap, string, string_size);
	datap += string_size;
	*((uint32*)datap) = data_size;
	datap += 4;
	if(data_size > 0)
		memcpy(datap, data, data_size);
	err = send_binder_request(node, reqp, request_size, reply_port);
	if(reqp != &small_req)
		free(reqp);
	return err;
}

static status_t 
internal_get_binder_reply(vnode *node, port_pool *reply_port,
                          reflection_struct &reflection,
                          binder_reply_basic **reply_buf,
                          size_t *reply_buf_size)
{
	ssize_t size;
	status_t err;
	int32 type;
	bigtime_t timeout = BINDER_REQUEST_TIMEOUT;

	if(reflection.active) {
		dprintf("binderfs(%d) -- get_binder_reply: reflection already active\n",
		        (int)find_thread(NULL));
		return B_OK;
	}
	if(!is_port_valid(node->ns, reply_port)) {
		dprintf("binderfs(%d) -- get_binder_reply: bad reply port\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}

	if(is_local(node->vnid.address.port)) {
		timeout = BINDER_REQUEST_SHORT_TIMEOUT;
	}
#if TRACE_COMMUNICATION
	dprintf("binderfs(%d) -- get_binder_reply: wait for reply on port %d\n",
	        (int)find_thread(NULL), (int)reply_port->port);
#endif
	err = size = port_buffer_size_etc(reply_port->port,
	                                  B_TIMEOUT /*| B_CAN_INTERRUPT*/, timeout);
	if(err < B_OK) {
		dprintf("binderfs(%d) -- get_binder_reply: port stat on %d failed "
		        "(request on %d), %s\n",
		        (int)find_thread(NULL), (int)reply_port->port,
		        (int)node->vnid.address.port, strerror(size));
		goto err;
	}
	if(size > 0) {
		if(*reply_buf == NULL) {
			*reply_buf_size = size;
			*reply_buf = (binder_reply_basic *)malloc(size);
			if(*reply_buf == NULL) {
				dprintf("binderfs(%d) -- get_binder_reply: malloc failed for "
				        "reply size %d\n",
				        (int)find_thread(NULL), (int)size);
				err = B_NO_MEMORY;
				goto err;
			}
		}
		else {
			if(size != (ssize_t)*reply_buf_size) {
				dprintf("binderfs(%d) -- get_binder_reply: bad reply size %d "
				        "int port %d, should be %d\n",
				        (int)find_thread(NULL), (int)size,
				        (int)reply_port->port, (int)*reply_buf_size);
				goto err;
			}
		}
	}
	err = read_port_etc(reply_port->port, &type, *reply_buf, size,
	                    B_TIMEOUT | B_CAN_INTERRUPT, timeout);
	if(err < size) {
		dprintf("binderfs(%d) -- get_binder_reply: port read on %d failed "
		        "(request on %d), %s\n",
		        (int)find_thread(NULL), (int)reply_port->port,
		        (int)node->vnid.address.port, strerror(err));
		goto err;
	}
	if(type == BINDER_PUNT_CODE) {
#if TRACE_REFLECTION
		dprintf("binderfs(%d) -- get_binder_reply: reflect punt\n",
		        (int)find_thread(NULL));
#endif
		reflection.add_cmd(REFLECT_CMD_PUNT, reply_port->port);
		return B_OK;
	}
	if(type != BINDER_REPLY_CODE) {
		dprintf("binderfs(%d) -- get_binder_reply: invalid message type, %d, "
		        "on replyport %d\n",
		        (int)find_thread(NULL), (int)type, (int)reply_port->port);
		goto err;
	}
	if((*reply_buf)->sync_port >= 0)
		register_reply_thread(node->ns, (*reply_buf)->sync_port);
	else
		release_port(node, reply_port);
	//if(!do_not_release)
	//	release_port(vn,inPort);
	return B_OK;

err:
	delete_port(node->ns, reply_port);
	return err;
}

status_t
get_binder_reply(vnode *node, port_pool *reply_port,
                 reflection_struct &reflection,
                 binder_reply_basic **reply_buf, size_t *reply_buf_size)
{
	*reply_buf = NULL;
	*reply_buf_size = 0;
	return internal_get_binder_reply(node, reply_port, reflection, reply_buf,
	                                 reply_buf_size);
}

status_t 
get_binder_reply(vnode *node, port_pool *reply_port,
                 reflection_struct &reflection,
                 binder_reply_basic *reply_buf, size_t reply_buf_size)
{
	return internal_get_binder_reply(node, reply_port, reflection, &reply_buf,
	                                 &reply_buf_size);
}

status_t 
get_binder_reply(vnode *node, port_pool *reply_port,
                 reflection_struct &reflection)
{
	status_t err;
	binder_reply_status reply;
	binder_reply_basic *reply_buf = &reply;
	size_t reply_size = sizeof(reply);
	err = internal_get_binder_reply(node, reply_port, reflection, &reply_buf,
	                                &reply_size);
	if(err < B_OK)
		return err;
	if(reflection.active)
		return B_OK;
	return reply.result;
}



status_t 
free_binder_reply(binder_reply_basic *reply_buf)
{
	free(reply_buf);
	return B_OK;
}

