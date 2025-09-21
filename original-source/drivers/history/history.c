#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include "history.h"

//#define DPRINTF(x) dprintf x
#define DPRINTF(x) (void) 0
#define LOCK() acquire_sem(history_lock);
#define UNLOCK() release_sem(history_lock);
#define MAX_HISTORY_ENTRIES 30

typedef struct history_entry history_entry;

struct history_entry {
	history_entry *prev;
	history_entry *next;
	size_t size;
	uchar data[1];
};

static history_entry *history = NULL;
static history_entry *current = NULL;
static int history_count = 0;
static void *global_state = NULL;
static int global_state_size = 0;
static int global_state_alloc = 0;
static sem_id history_lock = -1;

static status_t history_open(const char *name, uint32 flags, void **cookie)
{
	return B_OK;
}

static status_t history_close(void *cookie)
{
	return B_OK;
}

static status_t history_free(void *cookie)
{
	return B_OK;
}

static status_t history_control(void *cookie, uint32 op, void *data, size_t len)
{
	status_t error = B_ERROR;
	hioctl *request = (hioctl*) data;
	history_entry *entry;
	history_entry *tmp;

	error = lock_memory(request, sizeof(hioctl), 0);
	if (error != B_OK)
		return error;

	LOCK();
	switch (op) {
		case HISTORY_PUSH: {
			error = lock_memory(request->data, request->size, B_READ_DEVICE);
			if (error != B_OK)
				break;

			/* If we've backed up, trim history entries that come after this one. */
			entry = current->next;
			current->next = NULL;
			while (entry) {
				tmp = entry;
				entry = entry->next;
				free(tmp);
				history_count--;
			}
			
			/* If there are too many history entries, trim the oldest one. */
			if (history_count >= MAX_HISTORY_ENTRIES) {
				tmp = history->next;
				history->next = history->next->next;
				history->next->prev = history;
				free(tmp);
				history_count--;
			}
			
			/* Append the new entry */
			entry = (history_entry *) malloc(sizeof(history_entry) + request->size);
			if (entry != NULL) {
				entry->next = NULL;
				entry->prev = current;
				entry->size = request->size;
				current->next = entry;
				current = entry;
				memcpy(entry->data, request->data, request->size);
				history_count++;
				error = B_OK;
			} else
				error = B_NO_MEMORY;

			unlock_memory(request->data, request->size, 0);
			break;
		}
		
		case HISTORY_PEEK: {
			request->size = current->size;
			error = B_OK;
			break;
		}
		
		case HISTORY_CLEAR: {
			while (history) {
				entry = history;
				history = history->next;
				free(entry);
			}
			
			history_count = 0;
			history = (history_entry*) malloc(sizeof(history_entry));
			if (!history) {error = B_NO_MEMORY; break;}
		
			current = history;
			current->size = 0;
			current->next = NULL;
			current->prev = NULL;
			
			error = B_OK;		
			break;
		}
		
		case HISTORY_READ: {
			if (request->size >= current->size) {
				request->size = current->size;
				error = lock_memory(request->data, request->size, 0);
				if (error != B_OK)
					break;

				memcpy(request->data, current->data, request->size);
				unlock_memory(request->data, request->size, 0);
				error = B_OK;
			} else
				error = B_ERROR;
			
			break;
		}
		
		case HISTORY_FORWARD: {
			if (current->next) {
				current = current->next;
				error = B_OK;
			} else
				error = B_ERROR;
			
			break;
		}
		
		case HISTORY_BACK: {
			if (current->prev) {
				current = current->prev;
				error = B_OK;
			} else
				error = B_ERROR;

			break;
		}

		case HISTORY_GLOBAL_PEEK: {
			request->size = global_state_size;
			error = B_OK;
			break;		
		}
		
		case HISTORY_GLOBAL_READ: {
			if (request->size >= global_state_size) {
				error = lock_memory(request->data, global_state_size, 0);
				if (error != B_OK)
					break;
	
				memcpy(request->data, global_state, global_state_size);
				request->size = global_state_size;
				unlock_memory(request->data, global_state_size, 0);
				error = B_OK;
			} else
				error = B_ERROR;
			
			break;		
		}
		
		case HISTORY_GLOBAL_WRITE: {
			error = lock_memory(request->data, request->size, B_READ_DEVICE);
			if (error != B_OK)
				break;

			if (global_state_alloc < request->size) {
				if (global_state)
					free(global_state);

				global_state = malloc(request->size);
				if (global_state == NULL) {
					global_state_alloc = 0;
					error = B_NO_MEMORY;
				} else
					global_state_alloc = request->size;
			}

			if (global_state != NULL) {
				global_state_size = request->size;
				memcpy(global_state, request->data, request->size);
				error = B_OK;
			} else
				error = B_ERROR;
			
			unlock_memory(request->data, request->size, 0);
			break;
		}
		
		default:
			error = B_ERROR;
	}

	UNLOCK();	
	unlock_memory(request, sizeof(hioctl), 0);

	return error;
}

static status_t history_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	return B_ERROR;
}

static status_t history_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	return B_ERROR;
}

static device_hooks history_hooks = {
	history_open,
	history_close,
	history_free,
	history_control,
	history_read,
	history_write,
	NULL,
	NULL,
	NULL,
	NULL	
};

_EXPORT status_t init_hardware(void)
{
	return B_OK;
}

_EXPORT const char **publish_devices()
{
	static const char *names[] = { "misc/history_cache", NULL };
	return names;
}

_EXPORT device_hooks *find_device(const char *name)
{
	return &history_hooks;
}

_EXPORT status_t init_driver(void)
{
	history = (history_entry*) malloc(sizeof(history_entry));
	if (history == NULL)
		return B_NO_MEMORY;
	
	history_lock = create_sem(1, "history_lock");
	if (history_lock < 0) {
		free(history);
		return B_NO_MORE_SEMS;
	}

	current = history;
	current->size = 0;
	current->next = NULL;
	current->prev = NULL;
	global_state = NULL;
	global_state_size = 0;
	global_state_alloc = 0;
		
	return B_OK;
}

_EXPORT void uninit_driver(void)
{
	history_entry *entry;

	while (history) {
		entry = history;
		history = history->next;
		free(entry);
	}

	free(global_state);
	delete_sem(history_lock);
}

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;
