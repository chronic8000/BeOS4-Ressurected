
#include "gameport.h"

#include <string.h>
#include <stdio.h>

#include <ISA.h>
#if !defined(_DRIVERS_H)
#include <Drivers.h>
#endif /* _DRIVERS_H */
#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */
#if !defined(_SUPPORT_DEFS_H)
#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */

#if defined (__POWERPC__)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#if DEBUG
#define ddprintf dprintf
#else
#define ddprintf (void)
#endif

#define PROBE_FOR_INSTALL 0


typedef struct card_dev {
	int port;
	void * driver;
} card_dev;


EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);

const char isa_name[] = B_ISA_MODULE_NAME;
const char generic_gameport_name[] = "generic/gameport/v1";

extern device_hooks joy_hooks;

#define NUM_CARDS 2
#define MAX_DEVS NUM_CARDS

card_dev cards[NUM_CARDS] = {
	{ GAMEPORT_DEFAULT_ISA_PORT, NULL },
	{ GAMEPORT_ALTERNATE_ISA_PORT, NULL },
};

char * names[MAX_DEVS+1] = {
	"joystick/gameport/201",
	"joystick/gameport/209",
};

static isa_module_info	*isa;
static generic_gameport_module *gameport;


/* detect presence of our hardware */
status_t 
init_hardware(void)
{
	ddprintf("gameport: init_hardware()\n");

	return B_OK;
}


status_t
init_driver(void)
{
	status_t	f;

	ddprintf("gameport: init_driver()\n");

	f = get_module(generic_gameport_name, (module_info **)&gameport);
	if (f < 0) {
		return f;
	}
	f = (*gameport->create_device)(cards[0].port, &cards[0].driver);
	if (f < 0) {
		put_module(generic_gameport_name);
		return f;
	}
	f = (*gameport->create_device)(cards[1].port, &cards[1].driver);
	if (f < 0) {
		(*gameport->delete_device)(cards[0].driver);
		return f;
	}
	return B_OK;
}


void
uninit_driver(void)
{
	ddprintf("gameport: uninit_driver()\n");

	(*gameport->delete_device)(cards[0].driver);
	(*gameport->delete_device)(cards[1].driver);
	put_module(isa_name);
	put_module(generic_gameport_name);
}


const char **
publish_devices(void)
{
	int ix;
	ddprintf("gameport: publish_devices()\n");

	for (ix=0; names[ix]; ix++) ddprintf("%s\n", names[ix]);
	return (const char **)names;
}


device_hooks *
find_device(
	const char * name)
{
	int ix;

	ddprintf("gameport: find_device()\n");

	for (ix=0; names[ix]; ix++) {
		if (!strcmp(name, names[ix])) {
			return &joy_hooks;
		}
	}
	return NULL;
}


static status_t joy_open(const char *name, uint32 flags, void **cookie);
static status_t joy_close(void *cookie);
static status_t joy_free(void *cookie);
static status_t joy_control(void *cookie, uint32 op, void *data, size_t len);
static status_t joy_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t joy_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks joy_hooks = {
    &joy_open,
    &joy_close,
    &joy_free,
    &joy_control,
    &joy_read,
    &joy_write,
	NULL,
	NULL,
	NULL,
	NULL
};



static status_t
joy_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;

	for (ix=0; ix<NUM_CARDS; ix++) {
		if (!strcmp(name, names[ix])) {
			goto all_ok;
		}
	}
	dprintf("gameport joy_open(): can't find '%s'\n", name);
	return ENODEV;

all_ok:
	return (*gameport->open_hook)(cards[ix].driver, flags, cookie);
}


static status_t
joy_close(
	void * cookie)
{
	return (*gameport->close_hook)(cookie);
}


static status_t
joy_free(
	void * cookie)
{
	return (*gameport->free_hook)(cookie);
}


static status_t
joy_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	return (*gameport->control_hook)(cookie, iop, data, len);
}


static status_t
joy_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	return (*gameport->read_hook)(cookie, pos, data, nread);
}


static status_t
joy_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	return (*gameport->write_hook)(cookie, pos, data, nwritten);
}

