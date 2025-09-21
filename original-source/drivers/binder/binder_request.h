#ifndef BINDER_REQUEST_H
#define BINDER_REQUEST_H

#include <binder_driver.h>
#include <binder_request_types.h>
#include <SupportDefs.h>

struct binder_node;
struct port_pool;
struct reflection_struct;
struct binder_request_basic;
struct binder_reply_basic;

status_t send_binder_request(struct binder_node *node,
                             struct binder_request_basic *request,
                             size_t request_size,
                             struct port_pool **reply_port);

status_t send_binder_request_no_arg(struct binder_node *node,
                                    binder_request_type type,
                                    struct port_pool **reply_port);

status_t send_binder_request_one_arg(struct binder_node *node,
                                     binder_request_type type,
                                     uint32 arg, struct port_pool **reply_port);

status_t send_binder_request_name_and_data(struct binder_node *node,
                                           binder_request_type type,
                                           const char *string,
                                           void *data, size_t data_size,
                                           struct port_pool **reply_port);

status_t get_binder_reply_status(struct binder_node *node,
                                 struct port_pool *reply_port,
                                 struct reflection_struct *reflection);

status_t get_binder_reply(struct  binder_node *node,
                          struct port_pool *reply_port,
                          struct reflection_struct *reflection,
                          struct binder_reply_basic *reply_buf,
                          size_t reply_buf_size);

status_t get_binder_reply_dynamic(struct binder_node *node,
                                  struct port_pool *reply_port,
                                  struct reflection_struct *reflection,
                                  struct binder_reply_basic **reply_buf,
                                  size_t *reply_buf_size);

status_t free_binder_reply(struct binder_reply_basic *reply_buf);

#endif

