#include "binder_connection.h"
#include <drivers/KernelExport.h>
#include <malloc.h>
#include <lock.h>

// #pragma mark Types

typedef struct cookie_info {
	void   *cookie;
	/* use separate user and kernel references for debugging purposes */
	int    primary_ref_count;
	int    secondary_ref_count;
} cookie_info;

#define SHARED_COOKIE_TYPES (COOKIE_ALL_TYPE_COUNT - COOKIE_NORMAL_TYPE_COUNT)
typedef struct shared_cookie_info {
	void   *cookie;
	/* use separate user and kernel references for debugging purposes */
	int    primary_ref_count[SHARED_COOKIE_TYPES];
	int    secondary_ref_count[SHARED_COOKIE_TYPES];
	int    last_type_to_release;
} shared_cookie_info;

struct binder_connection {
	binder_connection   *next;
	binder_connection   *prev;
	team_id              team;
	bool                 team_died;
	lock                 cookie_lock;
	cookie_info         *cookies[COOKIE_NORMAL_TYPE_COUNT ];
	shared_cookie_info  *shared_cookies;
	int                  cookies_allocated[COOKIE_NORMAL_TYPE_COUNT + 1];
	int                  cookies_used[COOKIE_NORMAL_TYPE_COUNT + 1];
};

// #pragma mark Globals

static lock                connections_lock;
static binder_connection   connection_head_tail;

// #pragma mark -

status_t
init_binder_connection()
{
	status_t err;
	err = new_lock(&connections_lock, "binder connections lock");
	if(err < B_OK)
		goto err0;
	connection_head_tail.team = 0;
	connection_head_tail.team_died = true;
	connection_head_tail.next = &connection_head_tail;
	connection_head_tail.prev = &connection_head_tail;
err0:
	return err;
}

void
uninit_binder_connection()
{
	binder_connection *c = connection_head_tail.next;
	while(c != &connection_head_tail) {
		dprintf("binder -- uninit_binder_connection: team %d still connected\n",
		        (int)c->team);
	}
	free_lock(&connections_lock);
}

// #pragma mark -

static void
cleanup_cookie(binder_cookie_type type, void *cookie)
{
	switch(type) {
		case COOKIE_HOST_CONNECTION:
			cleanup_host_connection(cookie);
			break;
		case COOKIE_CLIENT_CONNECTION:
			cleanup_client_connection(cookie);
			break;
		case COOKIE_PROPERTY_ITERATOR:
			cleanup_property_iterator(cookie);
			break;
		case COOKIE_TRANSACTION_STATE:
			cleanup_transaction_state(cookie);
			break;
		case COOKIE_OBSERVER:
			cleanup_observer(cookie);
			break;
		//case COOKIE_GET_PROPERTY_REPLY:
		//	cleanup_get_property_reply(cookie);
		//	break;
		default:
			dprintf("binder(%d) -- cleanup_cookie: "
			        "no cleanup function for cookie type %d, %p\n",
			        (int)find_thread(NULL), type, cookie);
	}
}

/* call with c->cookie_lock held */
static cookie_info *
lookup_cookie_info(binder_connection *c, binder_cookie_type type, void *cookie)
{
	int i;
	if(type >= COOKIE_NORMAL_TYPE_COUNT) {
		dprintf("binder(%d) -- lookup_cookie_info: bad type %d\n",
		        (int)find_thread(NULL), type);
		return NULL;
	}
	for(i = 0; i < c->cookies_used[type]; i++) {
		if(c->cookies[type][i].cookie == cookie) {
			return &c->cookies[type][i];
		}
	}
	return NULL;
}

static shared_cookie_info *
lookup_shared_cookie_info(binder_connection *c, void *cookie)
{
	int i;
	for(i = 0; i < c->cookies_used[COOKIE_SHARED]; i++) {
		if(c->shared_cookies[i].cookie == cookie) {
			return &c->shared_cookies[i];
		}
	}
	return NULL;
}

/* call with c->cookie_lock held */
static status_t
free_cookie(binder_connection *c, binder_cookie_type type, int index)
{
	int i;

	if(type > COOKIE_SHARED)
		type = COOKIE_SHARED;

	i = index;
	if(i >= c->cookies_used[type]) {
		dprintf("binder(%d) -- free_cookie: cookie info out of range\n",
		        (int)find_thread(NULL));
		return B_ERROR;
	}
	c->cookies_used[type]--;
	if(type == COOKIE_SHARED)
		while(i < c->cookies_used[type]) {
			c->shared_cookies[i] = c->shared_cookies[i + 1];
			i++;
		}
	else
		while(i < c->cookies_used[type]) {
			c->cookies[type][i] = c->cookies[type][i + 1];
			i++;
		}
	if(c->cookies_used[type] < c->cookies_allocated[type] / 8) {
		void *new_cookies;
		void *old_cookies;
		int new_allocation_count;
		size_t cookie_size;
		dprintf("binder(%d) -- free_cookie: resize cookie type %d\n",
		        (int)find_thread(NULL), type);
		new_allocation_count = c->cookies_allocated[type] / 4;
		if(new_allocation_count < 4)
			new_allocation_count = 4;
		if(type == COOKIE_SHARED) {
			cookie_size = sizeof(shared_cookie_info);
			old_cookies = c->shared_cookies;
		}
		else {
			cookie_size = sizeof(cookie_info);
			old_cookies = c->cookies[type];
		}
		new_cookies = realloc(old_cookies, new_allocation_count * cookie_size);
		if(new_cookies == NULL) {
			dprintf("binder(%d) -- free_cookie: resize failed\n",
			        (int)find_thread(NULL));
			return B_OK; /* data is still valid */
		}
		
		if(type == COOKIE_SHARED)
			c->shared_cookies = new_cookies;
		else
			c->cookies[type] = new_cookies;
		c->cookies_allocated[type] = new_allocation_count;
	}
	return B_OK;
}

// #pragma mark -

binder_connection *
new_binder_connection()
{
	int i;
	thread_info ti;

	binder_connection *c = malloc(sizeof(binder_connection));
	if(c == NULL)
		return NULL;
	get_thread_info(find_thread(NULL),&ti);
	c->team = ti.team;
	c->team_died = false;
	if(new_lock(&c->cookie_lock, "binder cookie lock") < B_OK) {
		free(c);
		return NULL;
	}
	for(i = 0; i < COOKIE_NORMAL_TYPE_COUNT; i++) {
		c->cookies[i] = NULL;
		c->cookies_allocated[i] = 0;
		c->cookies_used[i] = 0;
	}
	c->shared_cookies = NULL;
	c->cookies_allocated[COOKIE_SHARED] = 0;
	c->cookies_used[COOKIE_SHARED] = 0;
	
	LOCK(connections_lock);
	c->next = &connection_head_tail;
	c->prev = connection_head_tail.prev;
	c->next->prev = c;
	c->prev->next = c;
	UNLOCK(connections_lock);
	return c;
}

void
delete_binder_connection(binder_connection *c)
{
	int type;
	for(type = 0; type < COOKIE_NORMAL_TYPE_COUNT + 1; type++) {
		if(c->cookies_used[type] > 0) {
			int i;
			dprintf("binder(%d) -- delete_binder_connection: %d cookies of "
			        "type %d are still active\n",
			        (int)find_thread(NULL), c->cookies_used[type], type);
			for(i = 0; i < c->cookies_used[type]; i++) {
				if(type == COOKIE_SHARED) {
					int sub_type;
					if(c->shared_cookies[i].secondary_ref_count[0] > 0 ||
					   c->shared_cookies[i].secondary_ref_count[1] > 0) {
						dprintf("binder(%d) -- delete_binder_connection: "
						        "cookie %p refcount p %d s %d p %d s %d\n",
						        (int)find_thread(NULL),
						        c->shared_cookies[i].cookie,
						        c->shared_cookies[i].primary_ref_count[0],
						        c->shared_cookies[i].secondary_ref_count[0],
						        c->shared_cookies[i].primary_ref_count[1],
						        c->shared_cookies[i].secondary_ref_count[1]);
					}
					for(sub_type = 0; sub_type < SHARED_COOKIE_TYPES; sub_type++) {
						while(c->shared_cookies[i].primary_ref_count[sub_type]-- > 0) {
							cleanup_cookie(COOKIE_SHARED + sub_type,
							               c->shared_cookies[i].cookie);
						}
					}
				}
				else {
					if(c->cookies[type][i].secondary_ref_count > 0) {
						dprintf("binder(%d) -- delete_binder_connection: "
						        "cookie %p refcount p %d s %d\n",
						        (int)find_thread(NULL),
						        c->cookies[type][i].cookie,
						        c->cookies[type][i].primary_ref_count,
						        c->cookies[type][i].secondary_ref_count);
					}
					while(c->cookies[type][i].primary_ref_count-- > 0) {
						cleanup_cookie(type, c->cookies[type][i].cookie);
					}
				}
			}
		}
		if(type == COOKIE_SHARED)
			free(c->shared_cookies);
		else
			free(c->cookies[type]);
	}
	free_lock(&c->cookie_lock);
	LOCK(connections_lock);
	c->next->prev = c->prev;
	c->prev->next = c->next;
	UNLOCK(connections_lock);
	free(c);
}

// #pragma mark -

static status_t
allocate_cookie(binder_connection *c, int type)
{
	if(type > COOKIE_SHARED)
		type = COOKIE_SHARED;
	if(c->cookies_used[type] >= c->cookies_allocated[type]) {
		void *new_cookies;
		void *old_cookies;
		int new_allocation_count;
		size_t cookie_size;
		if(c->cookies_allocated[type] <= 0)
			new_allocation_count = 4;
		else
			new_allocation_count = c->cookies_allocated[type] * 4;
		if(type == COOKIE_SHARED) {
			cookie_size = sizeof(shared_cookie_info);
			old_cookies = c->shared_cookies;
		}
		else {
			cookie_size = sizeof(cookie_info);
			old_cookies = c->cookies[type];
		}
		new_cookies = realloc(old_cookies, new_allocation_count * cookie_size);
		if(new_cookies == NULL)
			return B_NO_MEMORY;
		if(type < COOKIE_NORMAL_TYPE_COUNT)
			c->cookies[type] = new_cookies;
		else
			c->shared_cookies = new_cookies;
		c->cookies_allocated[type] = new_allocation_count;
	}
	return B_OK;
}

status_t
add_cookie(binder_connection *c, binder_cookie_type type, void *cookie)
{
	status_t err;
	if(type >= COOKIE_ALL_TYPE_COUNT) {
		dprintf("binder(%d) -- add_cookie: bad type %d\n",
		        (int)find_thread(NULL), type);
		return B_ERROR;
	}
	if(cookie == NULL) {
		dprintf("binder(%d) -- add_cookie: type %d, cannot add NULL cookie\n",
		        (int)find_thread(NULL), type);
		return B_ERROR;
	}
	LOCK(c->cookie_lock);
	if(type < COOKIE_NORMAL_TYPE_COUNT) {
		cookie_info *ci;
		ci = lookup_cookie_info(c, type, cookie);
		if(ci == NULL) {
			err = allocate_cookie(c, type);
			if(err < B_OK)
				goto err;
			ci = &c->cookies[type][c->cookies_used[type]];
			ci->cookie = cookie;
			ci->primary_ref_count = 1;
			ci->secondary_ref_count = 1;
			c->cookies_used[type]++;
		}
		else {
			ci->primary_ref_count++;
			ci->secondary_ref_count++;
		}
	}
	else {
		int sub_type = type - COOKIE_SHARED;
		shared_cookie_info *ci;
		ci = lookup_shared_cookie_info(c, cookie);
		if(ci == NULL) {
			int i;
			err = allocate_cookie(c, COOKIE_SHARED);
			if(err < B_OK)
				goto err;
			ci = &c->shared_cookies[c->cookies_used[COOKIE_SHARED]];
			ci->cookie = cookie;
			for(i = 0; i < SHARED_COOKIE_TYPES; i++) {
				ci->primary_ref_count[i] = 0;
				ci->secondary_ref_count[i] = 0;
			}
			ci->primary_ref_count[sub_type] = 1;
			ci->secondary_ref_count[sub_type] = 1;
			ci->last_type_to_release = -1;
			c->cookies_used[COOKIE_SHARED]++;
		}
		else {
			ci->primary_ref_count[sub_type]++;
			ci->secondary_ref_count[sub_type]++;
		}
	}
	err = B_OK;
err:
	UNLOCK(c->cookie_lock);
	return err;
}

status_t
remove_cookie(binder_connection *c, binder_cookie_type type, void *cookie)
{
	status_t err;
	LOCK(c->cookie_lock);
	if(type < COOKIE_NORMAL_TYPE_COUNT) {
		cookie_info *ci;
		ci = lookup_cookie_info(c, type, cookie);
		if(ci == NULL) {
			dprintf("binder(%d) -- remove_cookie: bad cookie, %p type %d\n",
			        (int)find_thread(NULL), cookie, type);
			err = B_BAD_VALUE;
			goto err;
		}
		if(ci->primary_ref_count < 1) {
			dprintf("binder(%d) -- remove_cookie: cookie, %p "
			        "type %d has no secondary reference\n",
			        (int)find_thread(NULL), cookie, type);
			kernel_debugger("remove_cookie underflow");
		}
		ci->primary_ref_count--;
		if(ci->primary_ref_count > 0) {
			//dprintf("binder(%d) -- remove_cookie: "
			//        "cleanup cookie %p type %d, primary refcount is now %d\n",
			//        (int)find_thread(NULL), cookie, type,
			//        ci->primary_ref_count);
			cleanup_cookie(type, cookie);
		}
		if(ci->secondary_ref_count == 0 &&
		   ci->primary_ref_count == 0) {
			dprintf("binder(%d) -- remove_cookie: "
			        "error no more references to cookie %p type %d\n",
			        (int)find_thread(NULL), cookie, type);
			free_cookie(c, type, ci - c->cookies[type]);
			cleanup_cookie(type, cookie);
		}
		err = B_OK;
	}
	else {
		int sub_type = type - COOKIE_SHARED;
		shared_cookie_info *ci;
		ci = lookup_shared_cookie_info(c, cookie);
		if(ci == NULL) {
			dprintf("binder(%d) -- remove_cookie: bad cookie, %p type %d\n",
			        (int)find_thread(NULL), cookie, type);
			err = B_BAD_VALUE;
			goto err;
		}
		if(ci->primary_ref_count[sub_type] < 1) {
			dprintf("binder(%d) -- remove_cookie: cookie, %p "
			        "type %d has no secondary reference\n",
			        (int)find_thread(NULL), cookie, type);
			kernel_debugger("remove_cookie underflow");
		}
		ci->primary_ref_count[sub_type]--;
		if(ci->primary_ref_count[0] > 0 ||
		   ci->primary_ref_count[1] > 0) {
			//dprintf("binder(%d) -- remove_cookie: cleanup cookie %p type %d, "
			//        "primary refcount is now %d, %d\n",
			//        (int)find_thread(NULL), cookie, type,
			//        ci->primary_ref_count[0], ci->primary_ref_count[1]);
			cleanup_cookie(type, cookie);
		}
		else
			ci->last_type_to_release = type;
		if(ci->secondary_ref_count[0] == 0 &&
		   ci->primary_ref_count[0] == 0 &&
		   ci->secondary_ref_count[1] == 0 &&
		   ci->primary_ref_count[1] == 0) {
			dprintf("binder(%d) -- remove_cookie: "
			        "error no more references to cookie %p type %d\n",
			        (int)find_thread(NULL), cookie, type);
			free_cookie(c, type, ci - c->shared_cookies);
			cleanup_cookie(type, cookie);
		}
		err = B_OK;
	}
err:
	UNLOCK(c->cookie_lock);
	return err;
}

void *
get_cookie_data(binder_connection *c, binder_cookie_type type, uint32 cookie_id)
{
	int i;
	void *cookie = NULL;
	if(type >= COOKIE_ALL_TYPE_COUNT) {
		dprintf("binder(%d) -- get_cookie_data: bad type %d\n",
		        (int)find_thread(NULL), type);
		return NULL;
	}
	LOCK(c->cookie_lock);
	if(type < COOKIE_NORMAL_TYPE_COUNT) {
		for(i = 0; i < c->cookies_used[type]; i++) {
			if(c->cookies[type][i].cookie == (void*)cookie_id) {
				cookie = c->cookies[type][i].cookie;
				c->cookies[type][i].secondary_ref_count++;
				break;
			}
		}
	}
	else {
		for(i = 0; i < c->cookies_used[COOKIE_SHARED]; i++) {
			if(c->shared_cookies[i].cookie == (void*)cookie_id) {
				int sub_type = type - COOKIE_SHARED;
				cookie = c->shared_cookies[i].cookie;
				c->shared_cookies[i].secondary_ref_count[sub_type]++;
				break;
			}
		}
	}
	UNLOCK(c->cookie_lock);
	if(cookie == NULL) {
		dprintf("binder(%d) -- get_cookie_data: could not find cookie "
		        "type %d with id 0x%08lx\n",
		        (int)find_thread(NULL), type, cookie_id);
		//kernel_debugger("binder breakpoint");
	}
	return cookie;
}

void 
return_cookie_data(binder_connection *c, binder_cookie_type type, void *cookie)
{
	if(type >= COOKIE_ALL_TYPE_COUNT) {
		dprintf("binder(%d) -- return_cookie_data: bad type %d\n",
		        (int)find_thread(NULL), type);
		return;
	}
	LOCK(c->cookie_lock);
	if(type < COOKIE_NORMAL_TYPE_COUNT) {
		cookie_info *ci;
		ci = lookup_cookie_info(c, type, cookie);
		if(ci == NULL) {
			dprintf("binder(%d) -- return_cookie_data: bad cookie, %p "
			        "type %d\n", (int)find_thread(NULL), cookie, type);
		}
		else {
			if(ci->secondary_ref_count < 1) {
				dprintf("binder(%d) -- return_cookie_data: cookie, %p "
				        "type %d has no secondary reference\n",
				        (int)find_thread(NULL), cookie, type);
				kernel_debugger("return_cookie_data underflow");
			}
			ci->secondary_ref_count--;
			if(ci->secondary_ref_count == 0 &&
			   ci->primary_ref_count == 0) {
				//dprintf("binder(%d) -- return_cookie_data: "
				//        "delete cookie, %p type %d\n",
				//        (int)find_thread(NULL), cookie, type);
				free_cookie(c, type, ci - c->cookies[type]);
				cleanup_cookie(type, cookie);
			}
		}
	}
	else {
		shared_cookie_info *ci;
		int sub_type = type - COOKIE_SHARED;
		ci = lookup_shared_cookie_info(c, cookie);
		if(ci == NULL) {
			dprintf("binder(%d) -- return_cookie_data: bad cookie, %p "
			        "type %d\n", (int)find_thread(NULL), cookie, type);
		}
		else {
			if(ci->secondary_ref_count[sub_type] < 1) {
				dprintf("binder(%d) -- return_cookie_data: cookie, %p "
				        "type %d has no secondary reference\n",
				        (int)find_thread(NULL), cookie, type);
				kernel_debugger("return_cookie_data underflow");
			}
			ci->secondary_ref_count[sub_type]--;
			if(ci->secondary_ref_count[0] == 0 &&
			   ci->primary_ref_count[0] == 0 &&
			   ci->secondary_ref_count[1] == 0 &&
			   ci->primary_ref_count[1] == 0) {
				//dprintf("binder(%d) -- return_cookie_data: "
				//        "delete cookie, %p type %d\n",
				//        (int)find_thread(NULL), cookie, type);
				if(ci->last_type_to_release < 0) {
					dprintf("binder(%d) -- return_cookie_data: delete cookie, "
					        "%p type %d, last_type_to_release < 0\n",
					        (int)find_thread(NULL), cookie, type);
					kernel_debugger("binder breakpoint");
				}
				else {
					cleanup_cookie(ci->last_type_to_release, cookie);
					free_cookie(c, type, ci - c->shared_cookies);
				}
			}
		}
	}
	UNLOCK(c->cookie_lock);
}


// #pragma mark -

int
kdebug_dump_binder_connections(bool start)
{
	static binder_connection *c;
	int count = 0;
	int type;
	if(start)
		c = connection_head_tail.next;
	while(count++ < 10 && c != &connection_head_tail) {
		kprintf("team %d at %p cookies used:", (int)c->team, c);
		for(type = 0; type < COOKIE_NORMAL_TYPE_COUNT + 1; type++) {
			kprintf(" %d", c->cookies_used[type]);
		}
		kprintf("\n");
		c = c->next;
	}
	if(c != &connection_head_tail) {
		kprintf("more...\n");
		return B_KDEBUG_CONT;
	}
	return 0;
}

int 
kdebug_dump_binder_connection(bool start, int team)
{
	static binder_connection *c, *cn;
	static int i, type;
	int count = 20;

	if(start) {
		c = cn = connection_head_tail.next;
		while(c != &connection_head_tail) {
			if(c->team == team)
				break;
			if(cn != &connection_head_tail) {
				cn = cn->next;
				if(cn != c && cn != &connection_head_tail)
					cn = cn->next;
				if(cn == c) {
					kprintf("connection list loops\n");
					return 0;
				}
			}
			c = c->next;
		}
		if(c == &connection_head_tail) {
			kprintf("no connection found for team %d\n", team);
			return 0;
		}
		kprintf("binder connection for team %d\n", team);

		type = 0;
		i = 0;
	}
	while(count-- > 0 && type < COOKIE_NORMAL_TYPE_COUNT + 1) {
		if(i == 0) {
			kprintf("  cookie type %d\n", type);
			kprintf("    cookies allocated %d\n", c->cookies_allocated[type]);
			kprintf("    cookies used %d\n", c->cookies_used[type]);
			count -= 2;
		}
		while(count-- > 0 && i < c->cookies_used[type]) {
			if(type < COOKIE_SHARED) {
				kprintf("      cookie %p p %d s %d\n", c->cookies[type][i].cookie,
				        c->cookies[type][i].primary_ref_count,
				        c->cookies[type][i].secondary_ref_count);
			}
			else {
				int j;
				shared_cookie_info *ci = &c->shared_cookies[i];
				kprintf("      cookie %p", ci->cookie);
				for(j = 0; j < SHARED_COOKIE_TYPES; j++) {
					kprintf(" type %d p %d s %d", COOKIE_SHARED + j,
					        ci->primary_ref_count[j],
					        ci->secondary_ref_count[j]);
				}
				kprintf("\n");
			}
			i++;
		}
		if(i < c->cookies_used[type])
			break;
		type++;
		i = 0;
	}
	if(type < COOKIE_NORMAL_TYPE_COUNT + 1) {
		kprintf("more...\n");
		return B_KDEBUG_CONT;
	}
	return 0;
}

