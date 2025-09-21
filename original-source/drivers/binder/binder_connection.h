#ifndef BINDER_CONNECTION_H
#define BINDER_CONNECTION_H

#include <support/SupportDefs.h>

typedef struct binder_connection binder_connection;

typedef enum {
	COOKIE_PROPERTY_ITERATOR,
	COOKIE_TRANSACTION_STATE,
	COOKIE_OBSERVER,
	COOKIE_NORMAL_TYPE_COUNT,
	/* following types are shared */
	COOKIE_SHARED = COOKIE_NORMAL_TYPE_COUNT,
	COOKIE_HOST_CONNECTION = COOKIE_SHARED,
	COOKIE_CLIENT_CONNECTION,
	COOKIE_ALL_TYPE_COUNT
} binder_cookie_type;

void cleanup_host_connection(void *cookie);
void cleanup_client_connection(void *cookie);
void cleanup_property_iterator(void *cookie);
void cleanup_transaction_state(void *cookie);
void cleanup_observer(void *cookie);

status_t init_binder_connection();
void uninit_binder_connection();

binder_connection *new_binder_connection();
void delete_binder_connection(binder_connection *c);

status_t add_cookie(binder_connection *c, binder_cookie_type type, void *cookie);
status_t remove_cookie(binder_connection *c, binder_cookie_type type, void *cookie);
void *get_cookie_data(binder_connection *c, binder_cookie_type type,
                      uint32 cookie_id);
void return_cookie_data(binder_connection *c, binder_cookie_type type,
                        void *cookie);

int kdebug_dump_binder_connections(bool start);
int kdebug_dump_binder_connection(bool start, int team);

#endif

