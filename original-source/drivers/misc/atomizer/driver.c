/*******************************************************************************
/
/	File:			driver.c
/
/	Description:	Kernel driver implementing user-space atomizer API
/
/	Copyright 1999, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#include <OS.h>
#include <KernelExport.h>

/* this is for sprintf() */
#include <stdio.h>
/* this is for string compares */
#include <string.h>

#include <atomizer_driver.h>

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#else
#define	ddprintf(a)
#endif

#define find_or_make_atomizer(string) (*atomizer_bus->find_or_make_atomizer)(string)
#define delete_atomizer(atomizer) (*atomizer_bus->delete_atomizer)(atomizer)
#define atomize(atomizer, string, create) (*atomizer_bus->atomize)(atomizer, string, create)
#define string_for_token(atomizer, atom) (*atomizer_bus->string_for_token)(atomizer, atom)
#define get_next_atomizer_info(cookie, info) (*atomizer_bus->get_next_atomizer_info)(cookie, info)
#define get_next_atom(atomizer, cookie) (*atomizer_bus->get_next_atom)(atomizer, cookie)

int32	api_version = 2;

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);

static atomizer_module_info	*atomizer_bus;
static device_hooks hacks_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

status_t
init_driver(void) {

	ddprintf(("get_module(%s)\n", B_ATOMIZER_MODULE_NAME));
	/* get a handle for the pci bus */
	if (get_module(B_ATOMIZER_MODULE_NAME, (module_info **)&atomizer_bus) != B_OK)
		return B_ERROR;

	return B_OK;
}

static const char * device_names[] = {
	"misc/atomizer",
	NULL
};

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return device_names;
}

device_hooks *
find_device(const char *name) {
	int index = 0;
	while (device_names[index]) {
		if (strcmp(name, device_names[index]) == 0)
			return &hacks_device_hooks;
		index++;
	}
	return NULL;

}

void uninit_driver(void) {
	/* put the pci module away */
	ddprintf(("ATOMIZER: about to put away atomizer module\n"));
	put_module(B_ATOMIZER_MODULE_NAME);
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	int32 index = 0;

	ddprintf(("ATOMIZER open_hook(%s, %lu, %p)\n", name, flags, cookie));

	/* find the device name in the list of devices */
	while (device_names[index] && (strcmp(name, device_names[index]) != 0)) index++;
	if (!device_names[index]) return B_ERROR;

	*cookie = NULL;
	
	return B_OK;
}

/* ----------
	read_hook - does nothing, gracefully
----- */
static status_t
read_hook (void* dev, off_t pos, void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}


/* ----------
	write_hook - does nothing, gracefully
----- */
static status_t
write_hook (void* dev, off_t pos, const void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}

/* ----------
	close_hook - does nothing, gracefully
----- */
static status_t
close_hook (void* dev)
{
	ddprintf(("ATOMIZER close_hook(%p)\n", dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_OK;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
	ddprintf(("ATOMIZER free_hook()\n"));
	/* all done */
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	status_t result = B_DEV_INVALID_IOCTL;

	ddprintf(("ioctl: %lu, buf: %p, len: %lu\n", msg, buf, len));
	switch (msg) {
		case B_ATOMIZER_FIND_OR_MAKE_ATOMIZER: {
			atomizer_find_or_make_atomizer *foma = (atomizer_find_or_make_atomizer*)buf;
			if (foma->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				foma->atomizer = find_or_make_atomizer(foma->name);
				result = B_OK;
			}
		} break;
		case B_ATOMIZER_DELETE_ATOMIZER: {
			atomizer_delete_atomizer *da = (atomizer_delete_atomizer*)buf;
			if (da->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				result = delete_atomizer(da->atomizer);
			}
		} break;
		case B_ATOMIZER_ATOMIZE: {
			atomizer_atomize *aa = (atomizer_atomize*)buf;
			if (aa->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				aa->atom = atomize(aa->atomizer, aa->string, aa->create);
				if (aa->atom) result = B_OK;
			}
		} break;
		case B_ATOMIZER_STRING_FOR_TOKEN: {
			atomizer_string_for_token *sft = (atomizer_string_for_token*)buf;
			if (sft->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				const char *string;
				string = string_for_token(sft->atomizer, sft->atom);
				if (string) {
					result = B_OK;
					sft->length = strlen(string);
					memcpy(sft->string, string, sft->max_size);
					sft->string[sft->max_size - 1] = '\0';
				}
			}
		} break;
		case B_ATOMIZER_GET_NEXT_ATOMIZER_INFO: {
			atomizer_get_next_atomizer_info *gnai = (atomizer_get_next_atomizer_info*)buf;
			if (gnai->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				result = get_next_atomizer_info(gnai->cookie, gnai->info);
			}
		} break;
		case B_ATOMIZER_GET_NEXT_ATOM: {
			atomizer_get_next_token *gnt = (atomizer_get_next_token*)buf;
			if (gnt->magic == ATOMIZER_PRIVATE_DATA_MAGIC) {
				gnt->atom = get_next_atom(gnt->atomizer, gnt->cookie);
				if (gnt->atom) result = B_OK;
			}
		} break;
	}
	return result;
}
