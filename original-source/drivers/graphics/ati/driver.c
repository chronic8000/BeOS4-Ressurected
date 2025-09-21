#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>

#include <graphic_driver.h>
/* this is for sprintf() */
#include <stdio.h>
/* this is for string compares */
#include <string.h>

/* many things are included via this file */
#include "ati_private.h"

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#else
#define	ddprintf(a)
#endif

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_ATI_DEVICES	8
#define ATI_VENDOR			0x1002

#if !defined(GRAPHICS_DEVICE_PATH)
#define GRAPHICS_DEVICE_PATH ""
#endif

int32	api_version = 2;

typedef struct device_info device_info;

typedef struct {
	timer		te;		/* timer entry for program_timer() */
	device_info	*di;	/* pointer owning device */
	bigtime_t	when_target;
} timer_info;

struct device_info {
	uint32
		is_open;
	area_id
		shared_area;
	shared_info
		*si;
	int32
		can_interrupt;
	timer_info
		ti_a, ti_b;
	timer_info
		*current_timer;
	thread_id
		tid;	/* only used if we're faking interrupts */
#if DEBUG > 0
	uint32
		interrupt_count;
#endif
	pci_info
		pcii;
	char
		name[B_OS_NAME_LENGTH];
};

typedef struct {
	uint32
#if DEBUG > 0
		total_interrupts,
#endif
		count;		/* number of devices actually found */
	int32
		ben;		/* integer part of driver-wide benaphore */
	sem_id
		sem;		/* semaphore part of driver-wide benaphore */
	char
		*device_names[MAX_ATI_DEVICES+1];	/* device name pointer storage */
	device_info
		di[MAX_ATI_DEVICES];	/* device specific stuff */
} DeviceData;

static status_t open_hook (const char* name, uint32 flags, void** cookie);
static status_t close_hook (void* dev);
static status_t free_hook (void* dev);
static status_t read_hook (void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook (void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook (void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(device_info *di);
static void unmap_device(device_info *di);
static void define_cursor(shared_info *si, uchar *xorMask, uchar *andMask, uint16 width, uint16 height, uint16 hot_x, uint16 hot_y);
static void probe_devices(void);
static int32 ati_interrupt(void *data);
static int atidump(int argc, char **argv);
static bool FindROM(uint8 *rom);

static DeviceData		*pd;
static pci_module_info	*pci_bus;
static device_hooks graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL
};

static uint16 ati_device_list[] = {
	0x4742,	/* ATI 3D Rage Pro AGP (normal) */
	0x4744,	/* ATI 3D Rage Pro AGP 1x */
	0x4749,	/* ATI 3D Rage Pro PCI */
#if EMACHINE
	0x474D,	/* ATI 3D Rage Pro PCI */
#endif
	0x4750,	/* ATI 3D Rage Pro PCI */
	0x4751,	/* ATI 3D Rage Pro (limited 3D) */
	0x4754,	/* ATI 3D Rage PCI/Rage II PCI */
	0x4755,	/* ATI 3D Rage II+ PCI */
	0x4756,	/* ATI 3D Rage IIC PCI */
	0x4757,	/* ATI 3D Rage IIC AGP (yet another version) */
	/* 0x4758,	 ATI Mach 64 GX - as reported by chrish@qnx.com */
	0x475A,	/* ATI 3D Rage IIc AGP */
	0x4C42,	/* ATI 3D Rage LT Pro AGP (normal) */
	0x4C44,	/* ATI 3D Rage LT Pro AGP 1x */
	0x4C49,	/* ATI 3D Rage LT Pro PCI */
	// 0x4C4D,	/* ATI Rage Mobility P3D */
	0x4C50,	/* ATI 3D Rage LT Pro PCI */
	0x4C51,	/* ATI 3D Rage LT Pro PCI (Limited 3D?) */
	0
};

static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{ATI_VENDOR, ati_device_list},
	{0x0000, NULL}
};

/*
	init_hardware() - Returns B_OK if one is
	found, otherwise returns B_ERROR so the driver will be unloaded.
*/
status_t
init_hardware(void) {
	long		pci_index = 0;
	pci_info	pcii;
	bool		found_one = FALSE;
	
	/* choke if we can't find the PCI bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* while there are more pci devices */
	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
		int vendor = 0;
		
		ddprintf(("AKD init_hardware(): checking pci index %d, device 0x%04x/0x%04x\n", pci_index, pcii.vendor_id, pcii.device_id));
		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == pcii.device_id ) {
						
						ddprintf(("AKD: we support this device\n"));
						found_one = TRUE;
						goto done;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
		/* next pci_info struct, please */
		pci_index++;
	}
	ddprintf(("AKD: init_hardware - no supported devices\n"));

done:
	/* put away the module manager */
	put_module(B_PCI_MODULE_NAME);
	return (found_one ? B_OK : B_ERROR);
}

status_t
init_driver(void) {

	int index;

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	/* initialize the benaphore */
	pd->ben = 0;
	pd->sem = create_sem(0, "ATI Kernel Driver");
	/* find all of our supported devices */
	probe_devices();
#if DEBUG > 0
	add_debugger_command("atidump", atidump, "dump ATI kernel driver persistant data");
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
	int index;

#if DEBUG > 0
	remove_debugger_command("atidump", atidump);
#endif

	/* free the driver data */
	delete_sem(pd->sem);
	free(pd);
	pd = NULL;

	/* put the pci module away */
	put_module(B_PCI_MODULE_NAME);
}

static status_t map_device(device_info *di) {
	/* default: frame buffer in [0], control regs in [1] */
	int regs = 2;
	int fb   = 0;
	char buffer[B_OS_NAME_LENGTH];
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);

	/* enable memory mapped IO, disable VGA I/O */
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong |= 0x00000002;
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_command, 4, tmpUlong);
#if 0
	/* enable ROM decoding */
	tmpUlong = get_pci(PCI_rom_base, 4);
	tmpUlong |= 0x00000001;
	set_pci(PCI_rom_base, 4, tmpUlong);
#endif
	/* map the areas */
	sprintf(buffer, "%04X_%04X_%02X%02X%02X regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	si->regs_area = map_physical_memory(
		buffer,
		(void *) di->pcii.u.h0.base_registers[regs],
		di->pcii.u.h0.base_register_sizes[regs],
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void **)&(si->regs));
	/* return the error if there was some problem */
	if (si->regs_area < 0) return si->regs_area;
	sprintf(buffer, "%04X_%04X_%02X%02X%02X rom",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
#if 0
	si->rom_area = map_physical_memory(
		buffer,
		(void *) /* 0x000c0000 */di->pcii.u.h0.rom_base,
		di->pcii.u.h0.rom_size,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA,
		(void **)&(si->rom));
	/* return the error if there was some problem */
	if (si->rom_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		return si->rom_area;
	}
	/* Yikes! Some motherboards with on-board ATI chips don't implement the PCI ROM. */
	/* See if this is one of them */
	if (!FindROM(si->rom)) {
		/* oops, no rom on the PCI bus */
		delete_area(si->rom_area);
#endif
		/* try the motherboard BIOS location */
		si->rom_area = map_physical_memory(
			buffer,
			(void *) 0x000c0000 /* di->pcii.u.h0.rom_base */,
			0x00020000,
			B_ANY_KERNEL_ADDRESS,
			B_READ_AREA,
			(void **)&(si->rom));
		/* return the error if there was some problem */
		if (si->rom_area < 0) {
			delete_area(si->regs_area);
			si->regs_area = -1;
			return si->rom_area;
		}
		/* check for it here */
		if (!FindROM(si->rom)) {
			/* oops, not the right ROM on the motherboard either */
			delete_area(si->rom_area);
			delete_area(si->regs_area);
			si->regs_area = si->rom_area = -1;
			return B_ERROR;
		}
#if 0
	}
#endif

	sprintf(buffer, "%04X_%04X_%02X%02X%02X framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	si->fb_area = map_physical_memory(
		buffer,
		(void *) di->pcii.u.h0.base_registers[fb],
		di->pcii.u.h0.base_register_sizes[fb],
#if defined(__INTEL__)
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
		B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
		B_READ_AREA + B_WRITE_AREA,
		&(si->framebuffer));

#if defined(__INTEL__)
	if (si->fb_area < 0) {	/* try to map this time without write combining */
		si->fb_area = map_physical_memory(
			buffer,
			(void *) di->pcii.u.h0.base_registers[fb],
			di->pcii.u.h0.base_register_sizes[fb],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			&(si->framebuffer));

		if (si->fb_area < 0) {	/* try to map this time without block address */
			si->fb_area = map_physical_memory(
				buffer,
				(void *) di->pcii.u.h0.base_registers[fb],
				di->pcii.u.h0.base_register_sizes[fb],
				B_ANY_KERNEL_ADDRESS,
				B_READ_AREA + B_WRITE_AREA,
				&(si->framebuffer));
		}
	}
#endif
		
	/* if there was an error, delete our other areas */
	if (si->fb_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		delete_area(si->rom_area);
		si->rom_area = -1;
	}
	/* remember the DMA address of the frame buffer for BDirectWindow purposes */
	si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[fb];
	/* in any case, return the result */
	return si->fb_area;
}

static void unmap_device(device_info *di) {
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);

	ddprintf(("unmap_device(%08x) begins...\n", di));
	ddprintf(("  regs_area: %d\n  fb_area: %d\n", si->regs_area, si->fb_area));
	
	/* disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong &= 0xfffffffc;
	set_pci(PCI_command, 4, tmpUlong);
	/* disable ROM decoding */
	tmpUlong = get_pci(PCI_rom_base, 4);
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_rom_base, 4, tmpUlong);
	/* delete the areas */
	if (si->rom_area >= 0) delete_area(si->rom_area);
	if (si->regs_area >= 0) delete_area(si->regs_area);
	if (si->fb_area >= 0) delete_area(si->fb_area);
	si->rom_area = si->regs_area = si->fb_area = -1;
	si->framebuffer = NULL;
	si->regs = NULL;
	si->rom = NULL;
	ddprintf(("unmap_device() ends.\n"));
}

static void probe_devices(void) {
	long pci_index = 0;
	uint32 count = 0;
	device_info *di = pd->di;

	/* while there are more pci devices */
	while ((count < MAX_ATI_DEVICES) && ((*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_NO_ERROR)) {
		int vendor = 0;
		
		ddprintf(("AKD: checking pci index %d, device 0x%04x/0x%04x\n", pci_index, di->pcii.vendor_id, di->pcii.device_id));
		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == di->pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == di->pcii.device_id ) {
						/* publish the device name */
						sprintf(di->name, "graphics%s/%04X_%04X_%02X%02X%02X",
							GRAPHICS_DEVICE_PATH,
							di->pcii.vendor_id, di->pcii.device_id,
							di->pcii.bus, di->pcii.device, di->pcii.function);
						ddprintf(("AKD: making /dev/%s\n", di->name));
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
						/* break out of this while loop */
						break;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
		/* next pci_info struct, please */
		pci_index++;
	}
	/* propagate count */
	pd->count = count;
	/* terminate list of device names with a null pointer */
	pd->device_names[pd->count] = NULL;
	ddprintf(("AKD probe_devices: %d supported devices\n", pd->count));
}

static uint32 thread_interrupt_work(int32 *flags, vuint32 *regs, shared_info *si) {
	uint32 handled = B_HANDLED_INTERRUPT;
	/* release the vblank semaphore */
	if (si->vblank >= 0) {
		int32 blocked;
		if ((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0)) {
			release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}
	return handled;
}

static int32
ati_interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	shared_info *si = di->si;
	int32 *flags = &(si->flags);
	vuint32 *regs;
	uint32 cntl;

#if DEBUG > 0
	pd->total_interrupts++;
#endif

	/* is someone already handling an interrupt for this device? */
	if (atomic_or(flags, AKD_HANDLER_INSTALLED) & AKD_HANDLER_INSTALLED) {
#if DEBUG > 0
		kprintf("AKD: Already in handler!\n");
#endif
		goto exit0;
	}
	/* get regs for ATIx() macros */
	regs = si->regs;

	cntl = regs[CRTC_INT_CNTL];
	/* did this card cause an interrupt */
	if (cntl & 0x04) {
		/* do our stuff */
		handled = thread_interrupt_work(flags, regs, si);
#if DEBUG > 0
		/* increment the counter for this device */
		di->interrupt_count++;
#endif
		/* clear the interrupt status */
		regs[CRTC_INT_CNTL] = cntl; /* 0x4 already set */
	}

	/* note that we're not in the handler any more */
	atomic_and(flags, ~AKD_HANDLER_INSTALLED);

exit0:
	return handled;				
}

static int32 timer_interrupt_func(timer *te) {
	bigtime_t now = system_time();
	/* get the pointer to the device we're handling this time */
	device_info *di = ((timer_info *)te)->di;
	shared_info *si = di->si;
	int32 *flags = &(si->flags);
	vuint32 *regs = si->regs;
	uint32 vbl_status = (regs[CRTC_INT_CNTL] & 1);
	int32 result = B_HANDLED_INTERRUPT;

	/* are we suppoesed to handle interrupts still? */
	if (atomic_and(flags, -1) & AKD_HANDLER_INSTALLED) {
		/* reschedule with same period by default */
		bigtime_t when = si->refresh_period;
		timer *to;

		/* if interrupts are "enabled", do our thing */
		if (di->can_interrupt) {
			/* insert code to sync to interrupts here */
			if (!vbl_status) {
				when -= si->blank_period - 4;
			} 
			/* do the things we do when we notice a vertical retrace */
			result = thread_interrupt_work(flags, regs, si);

		}

		/* pick the "other" timer */
		to = (timer *)&(di->ti_a);
		if (to == te) to = (timer *)&(di->ti_b);
		/* our guess as to when we should be back */
		((timer_info *)to)->when_target = now + when;
		/* reschedule the interrupt */
		add_timer(to, timer_interrupt_func, ((timer_info *)to)->when_target, B_ONE_SHOT_ABSOLUTE_TIMER);
	}

	return result;
}

void delay(bigtime_t i)
{
	bigtime_t start = system_time();
	while(system_time() - start < i)
		/* do nothing */;
}

void idle_device(device_info *di) {
	/* put device in lowest power consumption state */
}

#if DEBUG > 0
static int atidump(int argc, char **argv) {
	int i;

	kprintf("ATI Kernel Driver Persistant Data\n\nThere are %d card(s)\n", pd->count);
	kprintf("Driver wide benahpore: %d/%d\n", pd->ben, pd->sem);

	kprintf("Total seen interrupts: %d\n", pd->total_interrupts);
	for (i = 0; i < pd->count; i++) {
		device_info *di = &(pd->di[i]);
		uint16 device_id = di->pcii.device_id;
		shared_info *si = di->si;
		kprintf("  device_id: 0x%04x\n", device_id);
		kprintf("  interrupt count: %d\n", di->interrupt_count);
		if (si) {
			kprintf("  cursor: %d,%d\n", si->cursor_x, si->cursor_y);
			kprintf("  flags:");
			if (si->flags & AKD_MOVE_CURSOR) kprintf(" AKD_MOVE_CURSOR");
			if (si->flags & AKD_PROGRAM_CLUT) kprintf(" AKD_PROGRAM_CLUT");
			if (si->flags & AKD_SET_START_ADDR) kprintf(" AKD_SET_START_ADDR");
			kprintf("  vblank semaphore id: %d\n", si->vblank);
		}
		kprintf("\n");
	}
	return 1; /* the magic number for success */
}
#endif

static void lock_driver(void) {
	int	old;

	old = atomic_add (&pd->ben, 1);
	if (old >= 1) {
		acquire_sem(pd->sem);	
	}
}

static void unlock_driver(void)
{
	int	old;

	old = atomic_add (&pd->ben, -1);
	if (old > 1) {
		release_sem(pd->sem);
	}
}

static status_t open_hook (const char* name, uint32 flags, void** cookie) {
	int32 index = 0;
	device_info *di;
	shared_info *si;
	thread_id	thid;
	thread_info	thinfo;
	status_t	result = B_OK;
	pci_info	pcii;
	int			count;
	vuint32		*regs;
	char shared_name[B_OS_NAME_LENGTH];

	ddprintf(("AKD open_hook(%s, %d, 0x%08x)\n", name, flags, cookie));

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	while (pd->device_names[index] && (strcmp(name, pd->device_names[index]) != 0)) index++;

	/* for convienience */
	di = &(pd->di[index]);

	/* make sure no one else has write access to the common data */
	lock_driver();

	/* if it's already open for writing */
	if (di->is_open) {
		/* mark it open another time */
		goto mark_as_open;
	}
	/* create the shared area */
	sprintf(shared_name, "%04X_%04X_%02X%02X%02X shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 2, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (di->shared_area < 0) {
		/* return the error */
		result = di->shared_area;
		goto done;
	}

	/* save a few dereferences */
	si = di->si;

	/* save the vendor and device IDs */
	si->vendor_id = di->pcii.vendor_id;
	si->device_id = di->pcii.device_id;
	si->revision = di->pcii.revision;

	/* map the device */
	result = map_device(di);
	if (result < 0) goto free_shared;
	result = B_OK;

	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, di->name);
	if (si->vblank < 0) {
		result = si->vblank;
		goto unmap;
	}

	/* change the owner of the semaphores to the opener's team */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);

	/* assign local regs pointer for ATIxx() macros */
	regs = si->regs;

	/* disable and clear any pending interrupts - probably not required, but... */
	{
		uint32 cntl = regs[CRTC_INT_CNTL];
		cntl &= ~0x02;	/* clear int enable */
		cntl |=  0x04;	/* set int ack */
		regs[CRTC_INT_CNTL] = cntl;
	}

	si->interrupt_line = di->pcii.u.h0.interrupt_line;
	if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)){
		/* fake some kind of interrupt with a thread */
		di->can_interrupt = FALSE;
		si->flags = AKD_HANDLER_INSTALLED;
		si->refresh_period = 16666; /* fake 60Hz to start */
		si->blank_period = si->refresh_period / 20;
		di->ti_a.di = di;	/* refer to ourself */
		di->ti_b.di = di;
		di->current_timer = &(di->ti_a);
		/* program the first timer interrupt, and it will handle the rest */
		add_timer((timer *)(di->current_timer), timer_interrupt_func, si->refresh_period, B_ONE_SHOT_RELATIVE_TIMER);
	} else {
		install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, ati_interrupt, (void *)di, 0);
	}

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;
	
	goto done;

delete_the_sem:
	delete_sem(si->vblank);

unmap:
	unmap_device(di);

free_shared:
	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

done:
	/* end of critical section */
	unlock_driver();

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
	ddprintf(("AKD close_hook(%08x)\n", dev));
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
	vuint32 *regs = si->regs;

	ddprintf(("AKD free_hook() begins...\n"));
	/* lock the driver */
	lock_driver();

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

#if 1
	/* disable and clear any pending interrupts - probably not required, but... */
	{
		uint32 cntl = regs[CRTC_INT_CNTL];
		cntl &= ~0x02;	/* clear int enable */
		cntl |=  0x04;	/* set int ack */
		regs[CRTC_INT_CNTL] = cntl;
	}
#endif

	if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)) {
		status_t dont_care;
		/* stop our interrupt faking thread */
		di->can_interrupt = FALSE;
		si->flags = 0;
		/* wait for the thread */
		snooze(si->refresh_period * 2);
		/* cancel the timer */
		/* we don't know which one is current, so cancel them both and ignore any error */
		cancel_timer((timer *)&(di->ti_a));
		cancel_timer((timer *)&(di->ti_b));
	} else {
		/* remove interrupt handler */
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, ati_interrupt, di);
	}

	/* delete the semaphores, ignoring any errors ('cause the owning team may have died on us) */
	delete_sem(si->vblank);
	si->vblank = -1;

	/* free regs and framebuffer areas */
	unmap_device(di);

	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

unlock_and_exit:
	/* mark the device available */
	di->is_open--;
	/* unlock the driver */
	unlock_driver();
	ddprintf(("AKD free_hook() ends.\n"));
	/* all done */
	return B_OK;
}

/* -----------
	control_hook - where the real work is done
----------- */
static status_t
control_hook (void* dev, uint32 msg, void *buf, size_t len) {
	device_info *di = (device_info *)dev;
	shared_info	*si = di->si;
	status_t result = B_DEV_INVALID_IOCTL;

	ddprintf(("ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));
	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE: {
			char *sig = (char *)buf;
			/* strcpy(sig, "application/x-vnd.Be-ati.accelerant"); */
			strcpy(sig, "ati.accelerant");
			result = B_OK;
		} break;
		
		/* PRIVATE ioctl from here on */
		case ATI_GET_PRIVATE_DATA: {
			ati_get_private_data *gpd = (ati_get_private_data *)buf;
			if (gpd->magic == ATI_PRIVATE_DATA_MAGIC) {
				gpd->si = si;
				result = B_OK;
			}
		} break;
		case ATI_GET_PCI: {
			ati_get_set_pci *gsp = (ati_get_set_pci *)buf;
			if (gsp->magic == ATI_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
		} break;
		case ATI_SET_PCI: {
			ati_get_set_pci *gsp = (ati_get_set_pci *)buf;
			if (gsp->magic == ATI_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
		} break;
		case ATI_DEVICE_NAME: {
			ati_device_name *adn = (ati_device_name *)buf;
			if (adn->magic == ATI_PRIVATE_DATA_MAGIC) {
				strcpy(adn->name, di->name);
				result = B_OK;
			}
		} break;
		case ATI_RUN_INTERRUPTS: {
			ati_set_bool_state *ri = (ati_set_bool_state *)buf;
			if (ri->magic == ATI_PRIVATE_DATA_MAGIC) {
 				if ((di->pcii.u.h0.interrupt_pin == 0x00) || (di->pcii.u.h0.interrupt_line == 0xff)) {
					di->can_interrupt = ri->do_it;
				} else {
					vuint32 *regs = si->regs;
					if (ri->do_it) {
						/* resume interrupts */
						regs[CRTC_INT_CNTL] = regs[CRTC_INT_CNTL] | 0x00000006; /* 0x2 - int enable, 0x4 - int ack */
					} else {
						/* disable interrupts */
						uint32 cntl = regs[CRTC_INT_CNTL];
						cntl &= ~0x02;	/* clear int enable */
						cntl |=  0x04;	/* set int ack */
						regs[CRTC_INT_CNTL] = cntl;
					}
				}
				result = B_OK;
			}
		} break;
	}
	return result;
}

static bool FindROM(uint8 *ROMPtr) {
	/* hunt for the ROM */
	int iJ;

	ddprintf(("ROMPtr = 0x%08lx\n", ROMPtr));
	
	if ((ROMPtr[0] != 0x55) || (ROMPtr[1] != 0xAA))
	{
		/* Diagnostics */
		ddprintf((" - Didn't find 0xAA55. (IsOurROM)\n"));

		return 0;
	}

	iJ = 0;
	while ((iJ < 256) && (strncmp((char *) (&(ROMPtr[iJ])), "761295520", 9) != 0))
	{
		iJ++;
	}
	if (iJ >= 256)
	{
		/* Diagnostics */
		ddprintf((" - Didn't find '761295520'. (IsOurROM)\n"));

		return 0;
	}

	iJ = 0;
	while ((iJ < 1024) && (strncmp((char *) (&(ROMPtr[iJ])), "MACH64", 6) != 0))
	{
		iJ++;
	}
	if (iJ >= 1024)
	{
		/*
			The 'GXCX' id string is used in older ROMs. This may predate the Rage PRo;
			if it does, then it should be taken out for driver versions that handle only
			Rage Pro cards.
		*/
		iJ = 0;
		while ((iJ < 1024) && (strncmp((char *) (&(ROMPtr[iJ])), "GXCX", 4) != 0))
		{
			iJ++;
		}
		if (iJ >= 1024)
		{
			/* Diagnostics */
			ddprintf((" - Didn't find 'MACH64' or 'GXCX'. (IsOurROM)\n"));

			return 0;
		}
	}

	/* Diagnostics */
	ddprintf((" - ROM looks ok. (IsOurROM)\n"));

	/* If we reach here, then the ROM looks ok. */
	return 1;
}
