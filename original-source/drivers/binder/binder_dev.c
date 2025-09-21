#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>
#include <string.h>
#include <stdlib.h>

#include "binder_command.h"
#include "binder_node.h"
#include "binder_connection.h"

static void dummy_function() {}
#define UNUSED_ARG(x) { if(0) { dummy_function x; } }

static int binder_debug(int argc, char **argv);

status_t
init_driver(void)
{
	status_t err;
	dprintf("binder -- init_driver\n");
	err = init_binder_command();
	if(err < B_OK)
		goto err1;
	err = init_binder_connection();
	if(err < B_OK)
		goto err2;
	err = add_debugger_command("binder", binder_debug, "dump binder info");
	if(err < B_OK)
		goto err3;
	return B_OK;
//err4:
//	remove_debugger_command("binder", binder_debug);
err3:
	uninit_binder_connection();
err2:
	uninit_binder_command();
err1:
	dprintf("binder -- init_driver: failed, %s\n", strerror(err));
	return err;
}

void
uninit_driver(void)
{
	dprintf("binder -- uninit_driver\n");
	remove_debugger_command("binder", binder_debug);
	uninit_binder_connection();
	uninit_binder_command();
	dprintf("binder -- uninit_driver: done\n");
}	

status_t
binder_open(const char *name, uint32 flags, void **cookie)
{
	UNUSED_ARG((name, flags))
	*cookie = new_binder_connection();
	if(*cookie == NULL)
		return B_NO_MEMORY;
	return B_OK;
}

status_t
binder_close(void *cookie)
{
	UNUSED_ARG((cookie))
	return B_OK;
}

status_t
binder_free(void *cookie)
{
	delete_binder_connection(cookie);
	return B_OK;
}

status_t
binder_control(void *cookie, uint32 op, void *data, size_t len);
//{
//	UNUSED_ARG((cookie, op, data, len))
//	return B_ERROR;
//}

status_t
binder_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	UNUSED_ARG((cookie, position, data))
	*numBytes = 0;
	return B_ERROR;
}

status_t
binder_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	UNUSED_ARG((cookie, position, data))
	*numBytes = 0;
	return B_ERROR;
}

const char **
publish_devices()
{
	static const char *device_list[] = {
		"misc/binder",
		NULL
	};
	dprintf("binder -- publish_devices\n");
	return device_list;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;

device_hooks *
find_device(const char *name)
{
	static device_hooks binder_device_hooks = {
		binder_open,
		binder_close,
		binder_free,
		binder_control,
		binder_read,
		binder_write,
		NULL,		/* select */
		NULL,		/* deselect */
		NULL,		/* readv */
		NULL		/* writev */
	};
	dprintf("binder -- find_device: name == %s\n", name);
	return &binder_device_hooks;
}

// #pragma mark -

static int
binder_debug(int argc, char **argv)
{
	static int command = -1;
	static uint32 node_addr = 0;
	int arg1 = 0;

	//kprintf("binder debug called, argc %d, last command %d\n", argc, command);
	//for(i = 0; i < argc; i++) {
	//	kprintf("  arg %d: %s\n", i, argv[i]);
	//}

	if(argc > 0) {
		if(argc == 2 && strcmp(argv[1], "dumpnodes") == 0) {
			command = 1;
		}
		else if(argc == 2 && strcmp(argv[1], "node") == 0) {
			command = 2;
			node_addr = 0;
		}
		else if(argc == 3 && strcmp(argv[1], "node") == 0) {
			command = 2;
			node_addr = strtoul(argv[2], NULL, 0);
		}
		else if(argc == 2 && strcmp(argv[1], "connection") == 0) {
			command = 4;
		}
		else if(argc == 3 && strcmp(argv[1], "connection") == 0) {
			command = 5;
			arg1 = atoi(argv[2]);
		}
		else {
			kprintf("usage:\n"
			        "binder node               -- dump next node\n"
			        "binder node addr          -- dump node at addr\n"
			        "binder dumpnodes          -- dump all nodes\n"
			        "binder connection         -- dump all binder connections\n"
			        "binder connection team_id -- dump connection info for team\n"
			        );
			command = -1;
			return 0;
		}
	}
	switch(command) {
		case 1:
			return kdebug_dump_binder_nodes(argc > 0);
		case 2:
			kdebug_dump_binder_node(node_addr, &node_addr);
			if(node_addr != 0)
				return B_KDEBUG_CONT;
			else
				return 0;
		case 4:
			return kdebug_dump_binder_connections(argc > 0);
		case 5:
			return kdebug_dump_binder_connection(argc > 0, arg1);
	}
	return 0;
}

