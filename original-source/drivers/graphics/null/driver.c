/* standard kernel driver stuff */
#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>

/* this is for the standardized portion of the driver API */
/* currently only one operation is defined: B_GET_ACCELERANT_SIGNATURE */
#include <graphic_driver.h>

/* this is for sprintf() */
#include <stdio.h>

/* this is for string compares */
#include <string.h>

/* The private interface between the accelerant and the kernel driver. */
#include "DriverInterface.h"

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#else
#define	ddprintf(a)
#endif

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES	1

/* Tell the kernel what revision of the driver API we support */
int32	api_version = 2;

/* these structures are private to the kernel driver */
typedef struct device_info device_info;

struct device_info {
	uint32		is_open;			/* a count of how many times the devices has been opened */
	area_id		shared_area;		/* the area shared between the driver and all of the accelerants */
	shared_info	*si;				/* a pointer to the shared area, for convenience */
	char		name[B_OS_NAME_LENGTH];	/* where we keep the name of the device for publishing and comparing */
};

typedef struct {
	uint32		count;				/* number of devices actually found */
	benaphore	kernel;				/* for serializing opens/closes */
	char		*device_names[MAX_DEVICES+1];	/* device name pointer storage */
	device_info	di[MAX_DEVICES];	/* device specific stuff */
} DeviceData;

/* prototypes for our private functions */
static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static void probe_devices(void);
#if DEBUG > 0
static int ramdump(int argc, char **argv);
#endif

static DeviceData		*pd;
static device_hooks graphics_device_hooks = {
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

/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	return B_OK;
}

status_t
init_driver(void) {

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		return B_ERROR;
	}
	/* initialize the benaphore */
	INIT_BEN(pd->kernel);
	/* find all of our supported devices */
	probe_devices();
#if DEBUG > 0
	add_debugger_command("ramdump", ramdump, "dump RAMBUFFER kernel driver persistant data");
#endif
	return B_OK;
}

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return (const char **)pd->device_names;
}

device_hooks *
find_device(const char *name) {
	int index = 0;
	while (pd->device_names[index]) {
		if (strcmp(name, pd->device_names[index]) == 0)
			return &graphics_device_hooks;
		index++;
	}
	return NULL;

}

void uninit_driver(void) {

#if DEBUG > 0
	remove_debugger_command("ramdump", ramdump);
#endif

	/* free the driver data */
	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;
}

static void probe_devices(void) {
	uint32 count = 0;
	device_info *di = pd->di;

	/* if we match a supported vendor */
	while (count < MAX_DEVICES) {
		/* publish the device name */
		sprintf(di->name, "graphics/0_null_graphics_%02lx", count);
		ddprintf(("RKD: making /dev/%s\n", di->name));
		/* remember the name */
		pd->device_names[count] = di->name;
		/* mark the driver as available for R/W open */
		di->is_open = 0;
		/* mark areas as not yet created */
		di->shared_area = -1;
		/* mark pointer to shared data as invalid */
		di->si = NULL;
		/* inc pointer to device info */
		di++;
		/* inc count */
		count++;
	}

	/* propagate count */
	pd->count = count;
	/* terminate list of device names with a null pointer */
	pd->device_names[pd->count] = NULL;
	ddprintf(("RKD probe_devices: %ld supported devices\n", pd->count));
}

#if DEBUG > 0
static int ramdump(int argc, char **argv) {
	int i;

	kprintf("RAM BUFFER Kernel Driver Persistant Data\n\nThere are %ld card(s)\n", pd->count);
	kprintf("Driver wide benahpore: %ld/%ld\n", pd->kernel.ben, pd->kernel.sem);

	for (i = 0; i < pd->count; i++) {
		device_info *di = &(pd->di[i]);
		shared_info *si = di->si;
		if (si) {
			kprintf("  cursor: %d,%d\n", si->cursor.x, si->cursor.y);
			kprintf("  flags:");
			if (si->flags & RKD_MOVE_CURSOR) kprintf(" RKD_MOVE_CURSOR");
			if (si->flags & RKD_PROGRAM_CLUT) kprintf(" RKD_PROGRAM_CLUT");
			if (si->flags & RKD_SET_START_ADDR) kprintf(" RKD_SET_START_ADDR");
			kprintf("  vblank semaphore id: %ld\n", si->vblank);
		}
		kprintf("\n");
	}
	return 1; /* the magic number for success */
}
#endif

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	int32 index = 0;
	device_info *di;
	shared_info *si;
	status_t	result = B_OK;
	char shared_name[B_OS_NAME_LENGTH];
	physical_entry	pe;

	(void)flags;
	ddprintf(("RKD open_hook(%s, %ld, 0x%08lx)\n", name, flags, (uint32)cookie));

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	while (pd->device_names[index] && (strcmp(name, pd->device_names[index]) != 0)) index++;

	/* for convienience */
	di = &(pd->di[index]);

	/* make sure no one else has write access to the common data */
	AQUIRE_BEN(pd->kernel);

	/* if it's already open for writing */
	if (di->is_open) {
		/* mark it open another time */
		goto mark_as_open;
	}
	/* create the shared area */
	sprintf(shared_name, "NULL GRAPHICS %02lX shared", index);
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS, ((sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK, 0);
	if (di->shared_area < 0) {
		/* return the error */
		result = di->shared_area;
		goto done;
	}

	/* save a few dereferences */
	si = di->si;

	/* map the device */
	/* get 1MB of memory.  Use B_CONTIGUOUS to support BDirectWindow. */
	sprintf(shared_name, "NULL GRAPHICS %02lX framebuffer", index);
	si->fb_area = create_area(shared_name, (void **)&(si->framebuffer), B_ANY_KERNEL_ADDRESS, 1024 * 1024, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (si->fb_area < 0) goto free_shared;

	/* get the pci address of the RAM frame buffer */
	get_memory_map(si->framebuffer, 1024 * 1024, &pe, 1);
	si->framebuffer_pci = pe.address;

	result = B_OK;

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;
	
	goto done;


free_shared:
	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

done:
	/* end of critical section */
	RELEASE_BEN(pd->kernel);

	/* all done, return the status */
	ddprintf(("open_hook returning 0x%08lx\n", result));
	return result;
}

/* ----------
	read_hook - does nothing, gracefully
----- */
static status_t
read_hook (void* dev, off_t pos, void* buf, size_t* len)
{
	(void)dev;(void)pos;(void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}


/* ----------
	write_hook - does nothing, gracefully
----- */
static status_t
write_hook (void* dev, off_t pos, const void* buf, size_t* len)
{
	(void)dev;(void)pos;(void)buf;
	*len = 0;
	return B_NOT_ALLOWED;
}

/* ----------
	close_hook - does nothing, gracefully
----- */
static status_t
close_hook (void* dev)
{
	(void)dev;
	ddprintf(("RKD close_hook(%08lx)\n", (uint32)dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
	device_info *di = (device_info *)dev;
	shared_info	*si = di->si;

	ddprintf(("RKD free_hook() begins...\n"));
	/* lock the driver */
	AQUIRE_BEN(pd->kernel);

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

	/* free framebuffer area */
	delete_area(si->fb_area);
	si->fb_area = -1;
	si->framebuffer = si->framebuffer_pci = 0;

	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

unlock_and_exit:
	/* mark the device available */
	di->is_open--;
	/* unlock the driver */
	RELEASE_BEN(pd->kernel);
	ddprintf(("RKD free_hook() ends.\n"));
	/* all done */
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	device_info *di = (device_info *)dev;
	status_t result = B_DEV_INVALID_IOCTL;
	(void)len;
	/* ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len)); */
	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			strcpy(sig, "null.accelerant");
			result = B_OK;
		} break;
		
		/* PRIVATE ioctl from here on */
		case RAM_GET_PRIVATE_DATA: {
			ram_get_private_data *gpd = (ram_get_private_data *)buf;
			if (gpd->magic == RAM_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
		} break;
	}
	return result;
}
