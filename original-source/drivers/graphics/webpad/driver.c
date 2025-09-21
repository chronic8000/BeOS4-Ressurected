#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>

#include <graphic_driver.h>
/* this is for sprintf() */
#include <stdio.h>
/* this is for string compares */
#include <string.h>

/* many things are included via this file */
#include "webpad_private.h"

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#else
#define	ddprintf(a)
#endif

#define get_pci(o, s) (*pci_bus->read_pci_config)(di.pcii.bus, di.pcii.device, di.pcii.function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(di.pcii.bus, di.pcii.device, di.pcii.function, (o), (s), (v))
#define inb(a) (*pci_bus->read_io_8)(a)
#define outb(a, v) (*pci_bus->write_io_8)(a, v)

/* for def_fs */
int32	api_version = B_CUR_DRIVER_API_VERSION;

typedef struct {
	int32
		is_open;
	area_id
		shared_area;
	shared_info
		*si;
	pci_info
		pcii;

#if GXLV_DO_VBLANK
	int32
		can_interrupt;
	thread_id
		tid;	/* only used if we're faking interrupts */
#if DEBUG > 0
	uint32
		interrupt_count;
#endif
#endif
} device_info;

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(void);
static void unmap_device(void);

#if GXLV_DO_VBLANK
static void define_cursor(shared_info *si, uchar *xorMask, uchar *andMask, uint16 width, uint16 height, uint16 hot_x, uint16 hot_y);
static int mpdump(int argc, char **argv);
#endif

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

static device_info di;
static benaphore driver_lock;
static pci_module_info  *pci_bus;

/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	status_t result;
	system_info si;
	result = get_system_info(&si);
	if(result != B_OK)
		goto err;
	if(si.cpu_type != B_CPU_CYRIX_GXm &&
	   si.cpu_type != B_CPU_NATIONAL_GEODE_GX1)
	   	result = B_ERROR;
	/* check for the 5530 here as well? */
err:
	ddprintf(("gxm init_hardware() returns %.8lx\n", result));
	return result;
}

status_t
init_driver(void) {
	long pci_index = 0;
	int found = 0;
	di.is_open = 0;

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
	{
		ddprintf(("gxm init_driver() can't get PCI module :-(\n"));
		return B_ERROR;
	}
	/* find the 5530 */
	while (!found && ((*pci_bus->get_nth_pci_info)(pci_index++, &(di.pcii)) == B_NO_ERROR))
	{
		ddprintf(("GKD: checking pci index %d, device 0x%04x/0x%04x/0x%02x\n", pci_index, di.pcii.vendor_id, di.pcii.device_id, di.pcii.function));
		if ((di.pcii.vendor_id == 0x1078) && (di.pcii.device_id == 0x0104) && (di.pcii.function == 0x04))
		{
			found = 1;
			break;
		}
	}
	if (!found)
	{
		put_module(B_PCI_MODULE_NAME);
		ddprintf(("gxm init_driver() unable to find gxm device\n"));
		return B_ERROR;
	}
	INIT_BEN(driver_lock);
	ddprintf(("gxm init_driver() wins\n"));
	return B_OK;
}

void uninit_driver(void) {
	ddprintf(("gxm uninit_driver()\n"));
	put_module(B_PCI_MODULE_NAME);
	DELETE_BEN(driver_lock);
}

static const char *device_names[2] = {
	"graphics/webpad",
	NULL
};

const char **
publish_devices(void) {
	/* return the list of supported devices */
	return device_names;
}

device_hooks *
find_device(const char *name) {
	if (strcmp(name, device_names[0]) == 0)
		return &graphics_device_hooks;
	return NULL;
}

static status_t map_device() {
	shared_info *si = di.si;
	uint8 CCR3, GCR;
	cpu_status	cs;
	uint32 gx_base, mc_gbase_add, cx5530_base;

#define GX_CCR_INDEX	0x22
#define GX_CCR_DATA		0x23
#define GX_CCR3			0xC3
#define GX_CCR3_MAPEN	0x10
#define GX_GCR			0xB8
#define GX_GCR_BASE		0x03
#define GX_GCR_SCRATCH	0x0C

	ddprintf(("grabbing GCR\n"));
	/* disable interrupts */
	cs = disable_interrupts();

	/* save the CCR3 reg */
	//outb(GX_CCR_INDEX, GX_CCR3);
	//CCR3 = inb(GX_CCR_DATA);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(CCR3) : "d"(GX_CCR_DATA));

	/* set MAPEN in CCR3 */
	//outb(GX_CCR_INDEX, GX_CCR3);
	// outb(GX_CCR_DATA, CCR3 | GX_CCR3_MAPEN);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(CCR3 | GX_CCR3_MAPEN), "d"(GX_CCR_DATA));

#if 0
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(ccr3_after) : "d"(GX_CCR_DATA));
#endif

	/* find GX_BASE and scratch pad info */
	//outb(GX_CCR_INDEX, GX_GCR);
	//GCR = inb(GX_CCR_DATA);
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_GCR), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("inb %%dx, %%al \n" : "=a"(GCR) : "d"(GX_CCR_DATA));

#if 0
	if ((GCR & 0x03) == 0)
	{
		/* uh, it's off.  make a wild guess */
		GCR |= 0x01;
		__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_GCR_BASE), "d"(GX_CCR_INDEX));
		__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GCR), "d"(GX_CCR_DATA));
	}
#endif
	/* restore MAPEN setting */
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(GX_CCR3), "d"(GX_CCR_INDEX));
	__asm__ __volatile__ ("outb %%al,%%dx \n" :            : "a"(CCR3), "d"(GX_CCR_DATA));

	/* restore interrupt state */
	restore_interrupts(cs);
	ddprintf(("orig CCR3: %.2x\n", CCR3));
	ddprintf(("got GCR: %.2x\n", GCR));

	/* someone disabled the device, and we're not going to change it */
	if ((GCR & 0x03) == 0)
	{
		ddprintf(("Yikes! gxm graphics disabled!\n"));
		return B_ERROR;
	}
	gx_base = 0x40000000 * (GCR & GX_GCR_BASE);
	ddprintf(("mapping regs at %.8lx\n", gx_base));
	/* map the areas */
	si->regs_area = map_physical_memory(
		"gxlv regs",
		(void *) gx_base,
		0x10000,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void*)&(si->regs));
	/* return the error if there was some problem */
	if (si->regs_area < 0)
	{
		ddprintf(("gxm map_device() couldn't map %.8lx as gxlv regs\n", gx_base));
		return si->regs_area;
	}
	/* now that the area above is mapped, find out where the BIOS put the frame buffer */
	mc_gbase_add = *((uint32 *)(si->regs + MC_GBASE_ADD)) << 19;
	ddprintf(("MC_GBASE_ADD: %.8lx\n", mc_gbase_add));
	ddprintf(("BC_DRAM_TOP: %.8lx\n", *((uint32 *)(si->regs + 0x8000))));
	si->fb_area = map_physical_memory(
		"gxlv framebuffer",
		(void *) (gx_base + 0x00800000),
#if 0
		0x8000000,				// FFB: FIXME - get real size
		// 0x200000,
#else
		2 * 1024 * 1024,
#endif
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		&(si->framebuffer));
	/* bail on error */
	if (si->fb_area < 0)
	{
		ddprintf(("gxm map_device() couldn't map %.8lx as gxlv framebuffer\n", mc_gbase_add));
		goto delete_regs;
	}
	si->framebuffer_dma = (void *)mc_gbase_add;

	/* find out where the BIOS mapped it */
	cx5530_base = get_pci(0x10, 4) & ~0x0fff;

	ddprintf(("mapping CS5530 at %.8lx\n", cx5530_base));
	si->dac_area = map_physical_memory(
		"CS5530 graphics",
		(void *) cx5530_base,
		B_PAGE_SIZE,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void*)&(si->dac));

	/* bail on error */
	if (si->dac_area < 0)
	{
		ddprintf(("gxm map_device() couldn't map %.8lx as CS5530 graphics\n", cx5530_base));
		goto delete_fb;
	}

	/* all is well */
	return B_OK;

delete_fb:
	delete_area(si->fb_area);
	si->fb_area = -1;

delete_regs:
	delete_area(si->regs_area);
	si->regs_area = -1;
	/* in any case, return the result */
	return B_ERROR;
}

static void unmap_device() {
	shared_info *si = di.si;

	ddprintf(("unmap_device() begins...\n"));
	ddprintf(("  regs_area: %d\n  fb_area: %d\n", si->regs_area, si->fb_area));

	if (si->regs_area >= 0) delete_area(si->regs_area);
	if (si->fb_area >= 0) delete_area(si->fb_area);
	if (si->dac_area >= 0) delete_area(si->dac_area);
	si->regs_area = si->fb_area = si->dac_area = -1;
	si->framebuffer = NULL;
	si->framebuffer_dma = NULL;
	si->regs = NULL;
	si->dac = NULL;
	ddprintf(("unmap_device() ends.\n"));
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	int32 index = 0;
	shared_info *si;
	thread_id	thid;
	thread_info	thinfo;
	status_t	result = B_OK;
	int			irq;
	int			count;
	vuchar		*regs;
	char shared_name[B_OS_NAME_LENGTH];

	ddprintf(("GXM open_hook(%s, %d, 0x%08x)\n", name, flags, cookie));

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	if (strcmp(name, device_names[0]) != 0)
		return B_ERROR;

	ACQUIRE_BEN(driver_lock);
	if (di.is_open) goto mark_as_open;

	/* create the shared area */
	di.shared_area = create_area("gxm shared", (void**)&di.si, B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (di.shared_area < 0) {
		/* return the error */
		result = di.shared_area;
		goto done;
	}

	/* save a few dereferences */
	si = di.si;

	/* map the device */
	result = map_device();
	if (result < 0) goto free_shared;
	result = B_OK;

#if GXLV_DO_VBLANK
	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, name);
	if (si->vblank < 0) {
		result = si->vblank;
		goto unmap;
	}

	/* change the owner of the semaphores to the opener's team */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);
#endif

mark_as_open:
	/* mark the device open */
	di.is_open++;

	/* send the cookie to the opener */
	*cookie = &di;
	
	goto done;

#if GXLV_DO_VBLANK
delete_the_sem:
	delete_sem(si->vblank);

unmap:
	unmap_device();
#endif

free_shared:
	/* clean up our shared area */
	delete_area(di.shared_area);
	di.shared_area = -1;
	di.si = NULL;

done:

	RELEASE_BEN(driver_lock);

	/* all done, return the status */
	ddprintf(("open_hook returning 0x%08x\n", result));
	return result;
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
	ddprintf(("MKD close_hook(%08x)\n", dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/* -----------
	free_hook - close down the device
----------- */
static status_t
free_hook (void* dev) {
	shared_info	*si = di.si;
	vuchar *regs = si->regs;

	ddprintf(("GXM free_hook() begins...\n"));

	ACQUIRE_BEN(driver_lock);

	/* not the last one? bail */
	if (--di.is_open) goto unlock;

#if GXLV_DO_VBLANK
	/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
	delete_sem(si->vblank);
	si->vblank = -1;
#endif

	/* free regs and framebuffer areas */
	unmap_device();

	/* clean up our shared area */
	delete_area(di.shared_area);
	di.shared_area = -1;
	di.si = NULL;

unlock:
	/* unlock */
	RELEASE_BEN(driver_lock);

	ddprintf(("GXM free_hook() ends.\n"));
	/* all done */
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	shared_info	*si = di.si;
	status_t result = B_DEV_INVALID_IOCTL;

	/* ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len)); */
	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			/* strcpy(sig, "application/x-vnd.Be-webpad.accelerant"); */
			strcpy(sig, "webpad.accelerant");
			result = B_OK;
		} break;
		
		/* PRIVATE ioctl from here on */
		case GXM_GET_PRIVATE_DATA: {
			gxm_get_private_data *gpd = (gxm_get_private_data *)buf;
			if (gpd->magic == GXM_PRIVATE_DATA_MAGIC) {
				gpd->si = si;
				result = B_OK;
			}
		} break;
		case GXM_DEVICE_NAME: {
			gxm_device_name *gdn = (gxm_device_name *)buf;
			if (gdn->magic == GXM_PRIVATE_DATA_MAGIC) {
				strcpy(gdn->name, device_names[0]);
				result = B_OK;
			}
		} break;
	}
	return result;
}
