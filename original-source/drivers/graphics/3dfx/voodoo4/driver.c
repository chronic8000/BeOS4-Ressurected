/* ++++++++++
	fx_driver.c

This is a small driver to return information on the PCI bus and allow
an app to find memory mappings for devices. The only really complicated
part of this driver is the code to switch back to VGA mode in uninit,
largely this code is taken from the Glide Sources

+++++ */

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/common/debug.h>

#include <stdio.h>

#include "driver.h"
#include "sliaa.h"
#include <graphics_p/common/MemMgr/MemMgrKrnl.h>

/*------------------------------------------------------------------------*/

#define DEVICE_NAME 		"3dfx/"
#define NUM_OPENS			10		  // Max. number concurrent opens for device

typedef struct
{
	devinfo *di;
	uint32 memMgrKey;
} per_open_data;

_EXPORT status_t init_hardware (void);
_EXPORT status_t init_driver (void);
_EXPORT void uninit_driver (void);
_EXPORT const char **publish_devices (void);
_EXPORT device_hooks *find_device (const char *);

static long fx_open (const char *name, ulong flags, void **cookie);
static long fx_close (void *cookie);
static long fx_free (void *cookie);
static long fx_control (void *cookie, ulong msg, void *buf, size_t len);
static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes);
static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes);

static int thdfxdump (int argc, char **argv);


driverdata _V5_dd;

static device_hooks thdfx_device_hooks = {
	fx_open,
	fx_close,
	fx_free,
	fx_control,
	fx_read,
	fx_write
};

void _V5_WriteReg_32( devinfo *di, uint32 reg, uint32 data )
{
	(*_V5_dd.pci_bus->write_io_32) ( di->info_io_base + reg, data );
}

uint32 _V5_ReadReg_32( devinfo *di, uint32 reg )
{
	return 	(*_V5_dd.pci_bus->read_io_32) ( di->info_io_base + reg );
}

void _V5_WriteReg_16( devinfo *di, uint32 reg, uint16 data )
{
	(*_V5_dd.pci_bus->write_io_16) ( di->info_io_base + reg, data );
}

uint16 _V5_ReadReg_16( devinfo *di, uint32 reg )
{
	return 	(*_V5_dd.pci_bus->read_io_16) ( di->info_io_base + reg );
}

void _V5_WriteReg_8( devinfo *di, uint32 reg, uint8 data )
{
	(*_V5_dd.pci_bus->write_io_8) ( di->info_io_base + reg, data );
}

uint8 _V5_ReadReg_8( devinfo *di, uint32 reg )
{
	return 	(*_V5_dd.pci_bus->read_io_8) ( di->info_io_base + reg );
}


/****************************************************************************
 * {Sem,Ben}aphore initialization.
 */
static status_t init_locks (register struct thdfx_card_info *ci, team_id team)
{
	status_t retval;

	/*  Initialize {sem,ben}aphores the client will need.  */
	if ((retval = initOwnedBena4 (&ci->CRTCLock, "3Dfx CRTC lock", team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->CLUTLock, "3Dfx CLUT lock", team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->EngineLock, "3Dfx rendering engine lock", team)) < 0)
		return (retval);

	if ((retval = create_sem (0, "3Dfx VBlank semaphore")) < 0)
		return (retval);
	ci->VBlankLock = retval;
	set_sem_owner (retval, team);

	return (B_OK);
}

static void dispose_locks (register struct thdfx_card_info *ci)
{
	if (ci->VBlankLock >= 0)
	{
		ci->VBlankLock = -1;
	}
	disposeBena4 (&ci->EngineLock);
	disposeBena4 (&ci->CLUTLock);
	disposeBena4 (&ci->CRTCLock);
}

/****************************************************************************
 * Map device memory into local address space.
 */
static status_t map_device (register struct devinfo *di)
{
	thdfx_card_info *ci = di->ci;
	status_t retval;
// uint32      mappedio;
	int len, i;
	char name[B_OS_NAME_LENGTH];

// dprintf(("3dfx_v4: map_device - ENTER\n"));
	/*  Create a goofy basename for the areas  */
	sprintf (name, "%04X_%04X_%02X%02X%02X",
				di->PCI.vendor_id, di->PCI.device_id, di->PCI.bus, di->PCI.device, di->PCI.function);
	len = strlen (name);

	/*
	//This was almost enough to make it work as a 2nd card.
	(*pci_bus->write_pci_config) (di->PCI.bus, di->PCI.device, 0, 4, 2, 3);
	di->info_io_base = di->PCI.u.h0.base_registers_pci[2];
	outl (di->info_io_base + 0x04, 0x01840320);
	outl (di->info_io_base + 0x08, 0x00000000);
	outl (di->info_io_base + 0x0c, 0x00003fff);
	outl (di->info_io_base + 0x10, 0x00000000);
	outl (di->info_io_base + 0x14, 0x0f008001);
	outl (di->info_io_base + 0x18, 0x5916a9a9);
	outl (di->info_io_base + 0x1c, 0x40200031);
	outl (di->info_io_base + 0x20, 0x8000049e);
	outl (di->info_io_base + 0x24, 0x00000ff0);
	outl (di->info_io_base + 0x28, 0x00001340);
	outl (di->info_io_base + 0x2c, 0x00000000);
	outl (di->info_io_base + 0x30, 0x00000000);
	outl (di->info_io_base + 0x34, 0x00000000);
	*/

	/*
	 * Map chip registers.
	 */
// dprintf(("3dfx_v4: map_device - Mapping 2D Registers, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[0],  di->PCI.u.h0.base_register_sizes[0]));
	strcpy (name + len, " regs");
	retval =
		map_physical_memory (name,
									(void *) di->PCI.u.h0.base_registers_pci[0],
									di->PCI.u.h0.base_register_sizes[0],
									B_ANY_KERNEL_ADDRESS,
									B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr0);
	if (retval < 0)
		goto err_base0;			  /*  Look down  */
	di->BaseAddr0ID = retval;
// dprintf(("3dfx_v4: map_device - Mapping 2D Registers, di->BaseAddr0ID = 0x%x\n", di->BaseAddr0ID));

	/*
	 * Map framebuffer RAM
	 */
// dprintf(("3dfx_v4: map_device - Mapping Frame Buffer, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[1],  di->PCI.u.h0.base_register_sizes[1]));
	strcpy (name + len, " framebuffer");
	retval =
		map_physical_memory (name,
									(void *) di->PCI.u.h0.base_registers_pci[1],
									di->PCI.u.h0.base_register_sizes[1],
#if defined(__INTEL__)
									B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
									B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
									B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr1);
#if defined(__INTEL__)
	if (retval < 0)
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		retval = map_physical_memory
			(name,
			 (void *) di->PCI.u.h0.base_registers_pci[1],
			 di->PCI.u.h0.base_register_sizes[1],
			 B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr1);
#endif

	if (retval < 0)
		goto err_base1;			  /*  Look down  */

	di->BaseAddr1ID = retval;
// dprintf(("3dfx_v4: map_device - Mapping Frame Buffer, di->BaseAddr1ID = 0x%x\n", di->BaseAddr1ID));

	// Store the offset for the I/O registers
	di->info_io_base = di->PCI.u.h0.base_registers_pci[2];

	if (retval < 0)
	{
		delete_area (di->BaseAddr1ID);
		di->BaseAddr1ID = -1;
		ci->BaseAddr1 = NULL;
	 err_base1:
		delete_area (di->BaseAddr0ID);
		di->BaseAddr0ID = -1;
		ci->BaseAddr0 = NULL;
	 err_base0:
// dprintf(("3dfx_v4: map_device - Error Mapping Device returning 0x%x\n", retval));
		return (retval);
	}

	// Store the PCI register values
	for (i = 0; i <= 2; i++)
	{
		ci->base_registers_pci[i] = (uint8 *) di->PCI.u.h0.base_registers_pci[i];
	}
// dprintf(("3dfx_v4: map_device - EXIT\n"));
	return (B_OK);
}

static void unmap_device (struct devinfo *di)
{
	register thdfx_card_info *ci = di->ci;

//dprintf (("3dfx_v4: unmap_device - ENTER di->name =  %s\n", di->Name));

	if (di->int_check_timer.hook == 0)
	{
		cancel_timer (&(di->int_check_timer));
		di->int_check_timer.hook = 0;
	}

	if (di->BaseAddr1ID >= 0)
	{
		delete_area (di->BaseAddr1ID);
		di->BaseAddr1ID = -1;
	}
	if (di->BaseAddr0ID >= 0)
	{
		delete_area (di->BaseAddr0ID);
		di->BaseAddr0ID = -1;
	}
	ci->BaseAddr0 = ci->BaseAddr1_DMA = ci->BaseAddr1 = NULL;
	ci->FBBase = ci->FBBase_DMA = NULL;
//dprintf (("3dfx_v4: unmap_device - EXIT\n"));
}

/****************************************************************************
 * Surf the PCI bus for devices we can use.
 */
static void probe_devices ()
{
	long pci_index = 0;
	devinfo *di = _V5_dd.DI;
	uint32 count = 0;

	dprintf (("3dfx_v4: probe_devices - ENTER\n"));

	while ((count < MAX_CARDS) &&
			 ((*_V5_dd.pci_bus->get_nth_pci_info) (pci_index, &(di->PCI)) == B_NO_ERROR))
	{
		dprintf(("3dfx_v4: probe_devices - PCI Card #%d, vendor_id = 0x%x, device_id = 0x%x\n",
					 pci_index, di->PCI.vendor_id, di->PCI.device_id));

		if ((di->PCI.vendor_id == VENDORID_3DFX) && (
			  (di->PCI.device_id == DEVICEID_V5_6) ||
			  (di->PCI.device_id == DEVICEID_V5_7) ||
			  (di->PCI.device_id == DEVICEID_V5_8) || (di->PCI.device_id == DEVICEID_V5_5500)))
		{
			sprintf (di->Name,
						"graphics%s/0%04X_%04X_%02X%02X%02X",
						GRAPHICS_DEVICE_PATH,
						di->PCI.vendor_id, di->PCI.device_id,
						di->PCI.bus, di->PCI.device, di->PCI.function);

			_V5_dd.DevNames[count] = di->Name;
			di->SlotNum = pci_index;

			dprintf (("3dfx_v4: probe_devices - /dev/%s: slot num: %d\n", di->Name, pci_index));

			{
				uint32 temp = (*_V5_dd.pci_bus->read_pci_config) (di->PCI.bus, di->PCI.device, 0, DEVICEID, 2);
				di->info_Chips = 1;

				while (temp == (*_V5_dd.pci_bus->read_pci_config) (di->PCI.bus, di->PCI.device, di->info_Chips, DEVICEID, 2))
					di->info_Chips++;

				di->info_Memory = di->PCI.u.h0.base_register_sizes[1];
				di->info_io_base = di->PCI.u.h0.base_registers_pci[2];
				di->info_io_size = di->PCI.u.h0.base_register_sizes[2];
				dprintf (("*** Number of chips %ld,   Memory = %ld MB \n", di->info_Chips, di->info_Memory / (1024 * 1024)));
				dprintf (("*** IO base = %x   size = %x \n", di->info_io_base, di->info_io_size));
			}

			/*  Initialize to...  uh...  uninitialized.  */
			di->BaseAddr0ID = -1;
			di->BaseAddr1ID = -1;
			di->ci_AreaID = -1;
			di->NInterrupts = 0;
			di->NVBlanks = 0;
			di->LastIrqMask = 0;

			di++;
			count++;
		}
		pci_index++;
	}

	_V5_dd.NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	_V5_dd.DevNames[count] = NULL;

	if (count == 0)
	{
		dprintf (("3dfx_v4: No 3dfx cards installed - EXIT\n"));
		return;
	}
	else
	{
		dprintf (("3dfx_v4: %d 3dfx card(s) installed - EXIT\n", count));
		return;
	}
}

/*
 * This is actually a misnomer; very little initialization is done here; it
 * just detects if there's a card installed that we can handle.
 */
status_t init_hardware (void)
{
	pci_info pcii;
	int32 idx = 0;
	bool found_one = FALSE;

	dprintf (("3dfx_v4: init_hardware - Enter - PCI 3dfx-driver: %s %s\n", __DATE__, __TIME__));

	if ((idx = get_module (B_PCI_MODULE_NAME, (module_info **) & _V5_dd.pci_bus)) != B_OK)
		/*  Well, really, what's the point?  */
		return ((status_t) idx);

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0; (*_V5_dd.pci_bus->get_nth_pci_info) (idx, &pcii) == B_NO_ERROR; idx++)
	{
		if ((pcii.vendor_id == VENDORID_3DFX) &&
			 (
			  (pcii.device_id == DEVICEID_V5_6) ||
			  (pcii.device_id == DEVICEID_V5_7) ||
			  (pcii.device_id == DEVICEID_V5_8) || (pcii.device_id == DEVICEID_V5_5500)))
		{
			/*  My, that looks interesting...  */
//       dprintf(("3dfx_v4: init_hardware - found 3Dfx hardware @ %d\n", idx));
			found_one = TRUE;
			break;
		}
	}

	put_module (B_PCI_MODULE_NAME);
	_V5_dd.pci_bus = NULL;
	return (found_one ? B_OK : B_ERROR);
}

/*------------------------------------------------------------------------*/

status_t init_driver (void)
{
	int32 retval;
// int32 i, len;
// char devStr[1024];

#ifdef THDFX_DEBUG
	set_dprintf_enabled (TRUE);
#endif

	dprintf (("3dfx_v4: init_driver - Enter - PCI 3dfx-driver: %s %s\n", __DATE__, __TIME__));

	/*
	 * Get a handle for the PCI bus
	 */
	if ((retval = get_module (B_PCI_MODULE_NAME, (module_info **) & _V5_dd.pci_bus)) != B_OK)
		return (retval);

	if ((retval = initOwnedBena4 (&_V5_dd.DriverLock, "3Dfx kernel driver", B_SYSTEM_TEAM)) < 0)
		return (retval);

	/*  Find all of our supported devices  */
	probe_devices ();

#if defined(THDFX_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
	add_debugger_command ("thdfxdump", thdfxdump, "thdfxdump - dump 3Dfx kernel driver data");
#endif

	dprintf (("3dfx_v4: init_driver - EXIT\n"));
	return (B_OK);
}

/****************************************************************************
 * Interrupt management.
 */
static void clearinterrupts (register struct thdfx_card_info *ci)
{
	/* Enable VBLANK interrupt (and *only* that). */
	uint32 temp = _V5_ReadReg( ci, V5_2D_INTR_CTRL );
	temp &= ~(1 << 8);	/*  Clear Vertical Sync (Rising Edge) Interrupt generated flag  */
	temp |= 1 << 31;	/*  Write '1' to Extern pin pci_inta (active  low) to clear external PCI Interrupt  */
	_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, temp );
}

static void enableinterrupts (register struct thdfx_card_info *ci, int enable)
{
	uint32 temp = _V5_ReadReg( ci, V5_2D_INTR_CTRL );
	if (enable)
		temp |= 1 << 2;	/* Enable Vertical Sync (Rising Edge) Interrupts */
	else
		temp &= ~(1 << 2);
	_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, temp );
}

static int32 thdfx_interrupt (void *data)
{
	register devinfo *di;
	register thdfx_card_info *ci;
	register uint32 bits;
	bool resched;

	resched = false;

	di = data;
	_V5_dd.NCalls++;
	if (!(di->IRQEnabled))
		return (B_UNHANDLED_INTERRUPT);

	ci = di->ci;
	bits = _V5_ReadReg( ci, V5_2D_INTR_CTRL );

	if (!(bits & (1 << 8)))
		return (B_UNHANDLED_INTERRUPT);	/* Not a Vertical Sync Interrupt */

	/*  Clear the interrupt bit.  */
	_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, (bits & (~(1 << 8))) | (1 << 31) );

	_V5_dd.NInterrupts++;
	di->NInterrupts++;
	di->LastIrqMask = bits;

	/*
	 * Most common case (not to mention the only case) is VBLANK;
	 * handle it first.
	 */
	if (bits && (1 << 8))
	{
		register int32 *flags;

		di->NVBlanks++;
		flags = &ci->IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE)
		{
			/*
			 * Change the framebuffer base address here.
			 */
			uint32 fb_offset, stride;
			uint32 regBase;

			if (ci->DisplayAreaX == 0)
			{
				regBase = di->info_io_base;
				fb_offset = (uint32) ci->FBBase - (uint32) ci->BaseAddr1;
				stride = ci->BytesPerRow;
				fb_offset += ci->DisplayAreaY * stride;
				_V5_WriteReg_32 ( di, V5_VID_DESKTOP_START_ADDR,
						(fb_offset & SST_VIDEO_START_ADDR) << SST_VIDEO_START_ADDR_SHIFT);
			}
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR)
		{
			_V5_WriteReg_32 ( di, V5_VID_CUR_LOC,
					  ((ci->MousePosX - ci->MouseHotX + 64) << SST_CURSOR_X_SHIFT) |
					  ((ci->MousePosY - ci->MouseHotY + 64) << SST_CURSOR_Y_SHIFT));
		}

		/*  Release the vblank semaphore.  */
		if (ci->VBlankLock >= 0)
		{
			int32 blocked;

			if (get_sem_count (ci->VBlankLock, &blocked) == B_OK && blocked < 0)
			{
				release_sem_etc (ci->VBlankLock, -blocked, B_DO_NOT_RESCHEDULE);
				resched = true;
			}
		}
	}

#if 0
	if (bits)
	{
		/*  Where did this come from?  */
		dprintf (
					("3dfx: thdfx_interrupt -  Weird 3dfx interrupt (tell awk): bits = 0x%08lx\n",
					 bits));
	}
#endif
	return resched ? (B_INVOKE_SCHEDULER) : (B_HANDLED_INTERRUPT);
}

int32 int_check_handler (timer * t)
{
	thdfx_card_info *ci;
	devinfo *di;

	di = (devinfo *) t;
	ci = di->ci;

	if (di->NVBlanks > 0)
		ci->IRQFlags |= IRQF__ENABLED;
	else
		ci->IRQFlags &= ~IRQF__ENABLED;

	return B_HANDLED_INTERRUPT;
}

/****************************************************************************
 * Device hook (and support) functions.
 */
static status_t firstopen (struct devinfo *di)
{
	thdfx_card_info *ci;
	thread_id thid;
	thread_info thinfo;
	status_t retval = B_OK;
	int n;
	char shared_name[B_OS_NAME_LENGTH];

//dprintf(("3dfx_v4: firstopen - ENTER - di = 0x%08lx\n", di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
				di->PCI.vendor_id, di->PCI.device_id, di->PCI.bus, di->PCI.device, di->PCI.function);
	n = (sizeof (struct thdfx_card_info) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
										(void **) &di->ci,
										B_ANY_KERNEL_ADDRESS,
										n * B_PAGE_SIZE, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA)) < 0)
		return (retval);
	di->ci_AreaID = retval;

	ci = di->ci;
	memset (ci, 0, sizeof (*ci));

	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (ci, thinfo.team)) < 0)
	{
//    dprintf(("3dfx_v4: firstopen - init_locks returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->ci_AreaID >= 0)
		{
			delete_area (di->ci_AreaID);
			di->ci_AreaID = -1;
			di->ci = NULL;
		}
	}

	/*  Map the card's resources into memory.  */
	if ((retval = map_device (di)) < 0)
	{
//    dprintf(("3dfx_v4: firstopen map_device() returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->ci_AreaID >= 0)
		{
			delete_area (di->ci_AreaID);
			di->ci_AreaID = -1;
			di->ci = NULL;
		}
	}

	// Initialize ci_regs so that the interrupt handler works correctly
//	ci->Regs = (thdfxRegs *) ci->BaseAddr0;

	/* Add a kernel timer - when this 'goes off' we should check to see whether
	 * any VBL interupts have occured - if they haven't we need to change the setting
	 * of ci_IRQFlags. This should catch some (notably Diamonds) cards which do not seem to
	 * enable Interrupts correctly !
	 */
	add_timer (&(di->int_check_timer), &int_check_handler, INT_CHECK_TIMEOUT,
				  B_ONE_SHOT_RELATIVE_TIMER);

	/*  Install interrupt handler  */
	if ((di->PCI.u.h0.interrupt_line != 0) && (di->PCI.u.h0.interrupt_line != 0xff) &&
		 ((retval = install_io_interrupt_handler (di->PCI.u.h0.interrupt_line, thdfx_interrupt, (void *) di,
													0)) == B_OK))
	{
		ci->IRQFlags |= IRQF__ENABLED;
	}
	else
	{
		retval = B_OK;
	}

	clearinterrupts (ci);

	/* notify the interrupt handler that this card is a candidate */
	if (ci->IRQFlags & IRQF__ENABLED)
	{
		di->IRQEnabled = 1;
		enableinterrupts (ci, TRUE);
	}

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->DevName, di->Name);

	/* Note exactly which card variant this is */
	switch (di->PCI.device_id)
	{
	case DEVICEID_V5_5500:
		ci->device_id = DEVICEID_V5_5500;
		break;
	default:
		ci->device_id = 0;
		break;
	}

	/* Create the memMgr area */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X",
				di->PCI.vendor_id, di->PCI.device_id, di->PCI.bus, di->PCI.device, di->PCI.function);

	if (__mem_Initialize (&di->MemMgr, 32 * 1024 * 1024, 4096, shared_name) < B_OK)
	{
		retval = B_ERROR;
		di->MemMgr = 0;
		ci->memArea = -1;
	}
	else
	{
		ci->memArea = di->MemMgr->areaID;
		di->MemMgr->baseOffset = (uint32) ci->BaseAddr1;
	}

	/*  All done, return the status.  */
// dprintf(("3dfx_v4: firstopen returning 0x%08x\n", retval));
	return (retval);
}

static void lastclose (struct devinfo *di)
{
// dprintf(("3dfx_v4: lastclose - ENTER\n"));

	di->IRQEnabled = 0;

	enableinterrupts (di->ci, FALSE);
	clearinterrupts (di->ci);
	remove_io_interrupt_handler (di->PCI.u.h0.interrupt_line, thdfx_interrupt, di);

	unmap_device (di);
	dispose_locks (di->ci);

	delete_area (di->ci_AreaID);
	di->ci_AreaID = -1;
	di->ci = NULL;

// dprintf(("3dfx_v4: lastclose - EXIT\n"));
}

static long fx_open (const char *name, ulong flags, void **cookie)
{
	int n;
	devinfo *di;
	status_t retval = B_OK;
	per_open_data *pod;

	dprintf (("3dfx_v4: fx_open - ENTER, name is %s\n", name));

	lockBena4 (&_V5_dd.DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0; _V5_dd.DevNames[n] && strcmp (name, _V5_dd.DevNames[n]); n++)
		;
	di = &_V5_dd.DI[n];

	if (di->Opened == 0)
	{
		if ((retval = firstopen (di)) != B_OK)
		{
//       dprintf(("3dfx_v4: fx_open - firstopen() != B_OK !\n"));
			goto err;				  /*  Look down  */
		}
	}

	/*  Mark the device open.  */
	di->Opened++;

	/*  Squirrel this away.  */
	pod = (per_open_data *) malloc (sizeof (per_open_data));
	*cookie = pod;
	pod->di = di;
	pod->memMgrKey = __mem_GetNewID (di->MemMgr);

	/*  End of critical section.  */
 err:
	unlockBena4 (&_V5_dd.DriverLock);

	/*  All done, return the status.  */
// dprintf(("3dfx_v4: fx_open - returning 0x%08x\n", retval));
	return (retval);
}

static long fx_close (void *_cookie)
{
	//put_module(pci_name);
	return B_NO_ERROR;
}

static long fx_free (void *cookie)
{
	per_open_data *pod = (per_open_data *) cookie;
	devinfo *di = pod->di;
	thdfx_card_info *ci = di->ci;

// dprintf(("3dfx_v4: fx_free - ENTER\n"));

	lockBena4 (&_V5_dd.DriverLock);
	__mem_RemoveIndex (di->MemMgr, pod->memMgrKey);

	if (ci->OverlayOwnerID == pod->memMgrKey)
	{
		uint32 temp = di->ci->reg.r_V5_VID_PROC_CFG;
		temp &= ~(1 << 8);	// Disable Overlay
		temp |= 1 << 7;	// Enable Desktop
		_V5_WriteReg_NC( di->ci, V5_VID_PROC_CFG, temp );
		ci->OverlayOwnerID = -1;
	}

	if (di->PrimOverideID == pod->memMgrKey)
	{
		di->PrimOverideID = -1;
		_V5_SetVideoMode (di, di->PrimXRes, di->PrimYRes, di->PrimRefresh, di->PrimLoacClut);
//		_V5_WriteReg_32( di, SSTIOADDR (pllCtrl0), di->ci->prim_PLLReg0 );
	}

	if( di->engine_3d_lock == pod->memMgrKey )
	{
		di->engine_3d_lock = 0;
	}

	/*  Mark the device available.  */
	if (--di->Opened == 0)
		lastclose (di);

	unlockBena4 (&_V5_dd.DriverLock);
// dprintf(("3dfx_v4: fx_free - EXIT\n"));

	return (B_OK);
}

static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes)
{
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes)
{
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static void checkForRoom( devinfo *di )
{
	while ( !(_V5_ReadReg_32( di, V5_VID_STATUS ) & 0x3f ) )
		;
}

static long fx_control (void *_cookie, ulong msg, void *buf, size_t len)
{
	per_open_data *pod = (per_open_data *) _cookie;
	devinfo *di = pod->di;

//	thdfx_card_info *ci = di->ci;
	status_t retval = B_DEV_INVALID_IOCTL;

	switch (msg)
	{
		/*
		 * Stuff the accelerant needs to do business.
		 */
		/*
		 * The only PUBLIC ioctl.
		 */
	case B_GET_ACCELERANT_SIGNATURE:
		strcpy ((char *) buf, "3dfx_voodoo4.accelerant");
		retval = B_OK;
		break;

	case B_GET_3D_SIGNATURE:
		dprintf (("3dfx_v4: fx_control - B_GET_3D_SIGNATURE\n"));
		strcpy ((char *) buf, "3dfx_Voodoo4.3da");
		retval = B_OK;
		break;

		/* PRIVATE ioctl from here on. */
	case THDFX_IOCTL_GETGLOBALS:
		{
			thdfx_getglobals *tmp = (thdfx_getglobals *)buf;
	
			/* Check protocol version to see if client knows how to talk to us. */
			if (tmp->gg_ProtocolVersion == THDFX_IOCTLPROTOCOL_VERSION)
			{
				tmp->gg_GlobalArea = di->ci_AreaID;
				retval = B_OK;
			}
			break;
		}

	case THDFX_IOCTL_GET_PCI:
		{
			thdfx_getsetpci *tmp = (thdfx_getsetpci *) tmp;
			tmp->gsp_Value = (*_V5_dd.pci_bus->read_pci_config) (
				di->PCI.bus, di->PCI.device, di->PCI.function,
				tmp->gsp_Offset, tmp->gsp_Size);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_SET_PCI:
		{
			thdfx_getsetpci *tmp = (thdfx_getsetpci *) tmp;
			(*_V5_dd.pci_bus->write_pci_config) (
				di->PCI.bus, di->PCI.device, di->PCI.function,
				tmp->gsp_Offset, tmp->gsp_Size, tmp->gsp_Value);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PCI_32:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			checkForRoom( di );
			_V5_WriteReg_32 ( di, tmp->offset, tmp->val);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_READ_PCI_32:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			tmp->val = _V5_ReadReg_32 ( di, tmp->offset);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PHYS_8:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			_V5_WriteReg_8 (di, tmp->offset, tmp->val);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_READ_PHYS_8:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			tmp->val = _V5_ReadReg_8 (di, tmp->offset);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PHYS_16:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			_V5_WriteReg_16 (di, tmp->offset, tmp->val);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_CHECKFORROOM:
		{
			checkForRoom( di );
			retval = B_OK;
			break;
		}

	case MEM_IOCTL_GET_MEMMGR_ID:
		{
			uint32 *d = (uint32 *) buf;
			d[0] = pod->memMgrKey;
			retval = B_OK;
			break;
		}

	case MEM_IOCTL_GET_MEMMGR_AREA:
		{
			uint32 *d = (uint32 *) buf;
			d[0] = pod->di->MemMgr->areaID;
			retval = B_OK;
			break;
		}

	case MEM_IOCTL_SET_SEM:
		{
			__mem_SetSem( di->MemMgr, pod->memMgrKey, buf );
			retval = B_OK;
			break;
		}

	case MEM_IOCTL_RELEASE_SEMS:
		{
			__mem_ReleaseSems( di->MemMgr );
			retval = B_OK;
			break;
		}
	
	case THDFX_IOCTL_SET_VIDEO_MODE:
		{
			thdfx_setvideomode *mode = (thdfx_setvideomode *) buf;
			dprintf (("3dfx_v4: set_video_mode  xRes = 0x%x\n", mode->xRes));
			if (mode->xRes)
			{
				if (_V5_SetVideoMode (di, mode->xRes, mode->yRes, mode->refresh, mode->loadClut))
				{
					if (mode->isPrimary)
					{
						di->PrimXRes = mode->xRes;
						di->PrimYRes = mode->yRes;
						di->PrimRefresh = mode->refresh;
						di->PrimLoacClut = mode->loadClut;
					}
					else
					{
						di->PrimOverideID = pod->memMgrKey;
					}
					retval = B_OK;
				}
			}
			else
			{
				_V5_SetVideoMode (di, di->PrimXRes, di->PrimYRes, di->PrimRefresh, di->PrimLoacClut);
				retval = B_OK;
			}
			break;
		}

	case THDFX_IOCTL_LOCK_3D_ENGINE:
		{
			if( !di->engine_3d_lock )
			{
				retval = B_OK;
				di->engine_3d_lock = pod->memMgrKey;
			}
			break;
		}

	case THDFX_IOCTL_UNLOCK_3D_ENGINE:
		{
			if( di->engine_3d_lock == pod->memMgrKey )
			{
				retval = B_OK;
				di->engine_3d_lock = 0;
			}
			break;
		}

		/* Enable/Disable TV Output */
	case THDFX_IOCTL_ENABLE_TVOUT:
		{
			retval = B_ERROR;
			break;
		}

		/* I2C Routines */
	case THDFX_IOCTL_READ_I2C_REG:
		{
#define	I2C_REG_VAL	((int *) buf)
			int32 LTemp;
			LTemp = _V5_ReadReg( di->ci, V5_VID_SERIAL_PARALLEL_PORT );
			LTemp &= (SST_SERPAR_I2C_SCK_IN | SST_SERPAR_I2C_DSA_IN);
			LTemp = LTemp >> 26;
			*I2C_REG_VAL = LTemp;
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_I2C_REG:
		{
#define	I2C_REG_VAL	((int *) buf)
			int32 LTemp;
			cpu_status cs;
			LTemp = _V5_ReadReg( di->ci, V5_VID_SERIAL_PARALLEL_PORT );
			LTemp &= ~(SST_SERPAR_I2C_SCK_OUT | SST_SERPAR_I2C_DSA_OUT);	// clear all I2C bits
			LTemp |= (*I2C_REG_VAL) << 24;	// Shift in the new values for clock & data
			_V5_WriteReg_NC( di->ci, V5_VID_SERIAL_PARALLEL_PORT, LTemp );
			// Clock must be held for at least 5us so we spin here with interrupts disabled
			cs = disable_interrupts ();
			spin (5);
			restore_interrupts (cs);
			retval = B_OK;
			break;
		}

	default:
		dprintf (("3dfx_v4: fx_control - Unknown ioctl = 0x%x\n", msg));
		break;
	}
// dprintf("3dfx_v4: fx_control - EXIT, returning 0x%x\n", retval);
	return (retval);
}

device_hooks *find_device (const char *name)
{
	int32 index;

	for (index = 0; _V5_dd.DevNames[index]; index++)
	{
		if (strcmp (name, _V5_dd.DevNames[index]) == 0)
		{
			return &thdfx_device_hooks;
		}
	}

	return (NULL);
}

void uninit_driver (void)
{
	dprintf(("3dfx_v4: uninit_driver - Enter\n"));

#if defined(THDFX_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
	remove_debugger_command ("thdfxdump", thdfxdump);
#endif
// dprintf(("3dfx_v4: uninit_driver - Exit\n"));
}

const char **publish_devices (void)
{
	const char **dev_list = NULL;
	dev_list = (const char **) _V5_dd.DevNames;
	return (dev_list);
}

/****************************************************************************
 * Debuggery.
 */

static void dprint_alloc (__mem_Allocation * alloc)
{
	kprintf ("    start & end  %x - %x \n", alloc->start, alloc->end);
	kprintf ("    baseAddress %p \n", alloc->address);
	kprintf ("    id %x \n", alloc->id);
	kprintf ("    lastBound %x \n", alloc->lastBound);
	kprintf ("    locked %x \n", alloc->locked);
	kprintf ("    state %x \n", alloc->state);
}

#if defined(THDFX_DEBUG)   || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
static int thdfxdump (int argc, char **argv)
{
	int i, k;
	uint32 readPtrL1, readPtrL2, hwreadptr;
	readPtrL1 = 1;
	readPtrL2 = 2;
	hwreadptr = 0;

	kprintf ("3Dfx kernel driver\nThere are %d device(s)\n", _V5_dd.NDevs);
	kprintf ("Driver-wide benahpore: %d/%d\n", _V5_dd.DriverLock.b4_Sema4, _V5_dd.DriverLock.b4_FastLock);
	kprintf ("Total calls to IRQ handler: %ld\n", _V5_dd.NCalls);
	kprintf ("Total relevant interrupts: %ld\n", _V5_dd.NInterrupts);

	for (i = 0; i < _V5_dd.NDevs; i++)
	{
		devinfo *di = &_V5_dd.DI[i];
		thdfx_card_info *ci = di->ci;

		kprintf ("Device %s\n", di->Name);
		kprintf ("\tTotal interrupts: %ld\n", di->NInterrupts);
		kprintf ("\tVBlanks: %ld\n", di->NVBlanks);
		kprintf ("\tLast IRQ mask: 0x%08lx\n", di->LastIrqMask);
		if (ci)
		{
			kprintf ("  thdfx_card_info: 0x%08lx\n", ci);
			kprintf ("  Base register 0,1: 0x%08lx, 0x%08lx\n", ci->BaseAddr0, ci->BaseAddr1);
			kprintf ("  IRQFlags: 0x%08lx\n", ci->IRQFlags);

			kprintf ("  ci_AllocFrameBuffer \n");
			dprint_alloc (ci->AllocFrameBuffer);

			kprintf ("  ci_AllocSentinal \n");
			dprint_alloc (ci->AllocSentinal);

			kprintf ("  ci_AllocCursor \n");
			dprint_alloc (ci->AllocCursor);

			for (k = 0; k < 2; k++)
			{
				kprintf ("  ci_AllocFifo %x \n", k);
				dprint_alloc (ci->AllocFifo[k]);
//          kprintf ("  ci_fifoMemSpec[%d]: ms_BaseAddr = 0x%08lx, size = 0x%08lx\n", k,  ci->fifoMemSpec[k].ms_BaseAddr, ci->fifoMemSpec[k].ms_Size);
//          kprintf ("  ci_CmdTransportInfo[%d]: fifoPtr = 0x%08lx, fifoRead = 0x%08lx, fifoRoom = 0x%08lx\n", k,
//                   ci->CmdTransportInfo[k].fifoPtr, ci->CmdTransportInfo[k].fifoRead, ci->CmdTransportInfo[k].fifoRoom);

/*
				do
				{
					if (k == 1)
						readPtrL1 = ci->Regs->regs_agp_u.regs_agp.cmdFifo1.readPtrL;
					else if (k == 0)
						readPtrL1 = ci->Regs->regs_agp_u.regs_agp.cmdFifo0.readPtrL;
					status = ci->Regs->regs_3d_u.regs_3d.status;
					if (k == 1)
						readPtrL2 = ci->Regs->regs_agp_u.regs_agp.cmdFifo1.readPtrL;
					else if (k == 0)
						readPtrL2 = ci->Regs->regs_agp_u.regs_agp.cmdFifo0.readPtrL;
				}
				while (readPtrL1 != readPtrL2);
*/

				hwreadptr = (((uint32) ci->CmdTransportInfo[k].fifoStart) +
								 readPtrL2 - (uint32) ci->CmdTransportInfo[k].fifoOffset);
				kprintf ("  ci_CmdTransportInfo[%d]: HW Read Ptr = 0x%08lx\n", k, hwreadptr);
			}
		}
	}
	kprintf ("\n");
	return (B_OK);
}

#endif
