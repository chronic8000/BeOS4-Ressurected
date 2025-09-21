/* ++++++++++
	fx_driver.c

This is a small driver to return information on the PCI bus and allow
an app to find memory mappings for devices. The only really complicated
part of this driver is the code to switch back to VGA mode in uninit,
largely this code is taken from the Glide Sources

+++++ */

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/common/debug.h>

#include <stdio.h>

#include "driver.h"
#include <graphics_p/common/MemMgr/MemMgrKrnl.h>

extern void kdprintf(const char *str, ...);

/*------------------------------------------------------------------------*/

#define DEVICE_NAME 		"3dfx/"
#define NUM_OPENS			10		  // Max. number concurrent opens for device
#define GET_NTH_PCI_INFO	(*pci_bus->get_nth_pci_info)

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

uint32 guard_b[65536];

driverdata _V3_dd;

uint32 guard_e[65536];

static device_hooks thdfx_device_hooks = {
	fx_open,
	fx_close,
	fx_free,
	fx_control,
	fx_read,
	fx_write
};

void _V3_WriteReg_16( devinfo *di, uint32 reg, uint16 data )
{
	(*_V3_dd.pci_bus->write_io_16) ( di->io_base + reg, data );
}

uint16 _V3_ReadReg_16( devinfo *di, uint32 reg )
{
	return 	(*_V3_dd.pci_bus->read_io_16) ( di->io_base + reg );
}

void _V3_WriteReg_8( devinfo *di, uint32 reg, uint8 data )
{
	(*_V3_dd.pci_bus->write_io_8) ( di->io_base + reg, data );
}

uint8 _V3_ReadReg_8( devinfo *di, uint32 reg )
{
	return 	(*_V3_dd.pci_bus->read_io_8) ( di->io_base + reg );
}

static void init_guard()
{
	memset( guard_b, 0xcc, sizeof( guard_b ));
	memset( guard_e, 0xcc, sizeof( guard_e ));
}

static void check_guard()
{
	int32 ct;
	for( ct=0; ct<65536; ct++ )
	{
		if( guard_b[ct] != 0xcccccccc )
			kprintf( "Damaged guard_b at %ld   value=%08x \n", ct, guard_b[ct] );
		if( guard_e[ct] != 0xcccccccc )
			kprintf( "Damaged guard_e at %ld   value=%08x \n", ct, guard_e[ct] );
	}
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

// dprintf(("3dfx_V3_Krnl: map_device - ENTER\n"));
	/*  Create a goofy basename for the areas  */
	sprintf (name, "%04X_%04X_%02X%02X%02X",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
	len = strlen (name);

	/* Map chip registers. */
// dprintf(("3dfx_V3_Krnl: map_device - Mapping 2D Registers, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[0],  di->PCI.u.h0.base_register_sizes[0]));
	strcpy (name + len, " regs");
	retval = map_physical_memory (name,
						(void *) di->PCI.u.h0.base_registers_pci[0],
						di->PCI.u.h0.base_register_sizes[0],
						B_ANY_KERNEL_ADDRESS,
						B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr0);
	if (retval < 0)
		goto err_base0;			  /*  Look down  */
	di->BaseAddr0ID = retval;
// dprintf(("3dfx_V3_Krnl: map_device - Mapping 2D Registers, di->BaseAddr0ID = 0x%x\n", di->BaseAddr0ID));

	/*
	 * Map framebuffer RAM
	 */
// dprintf(("3dfx_V3_Krnl: map_device - Mapping Frame Buffer, base = 0x%x, size = 0x%x\n", di->PCI.u.h0.base_registers_pci[1],  di->PCI.u.h0.base_register_sizes[1]));
	strcpy (name + len, " framebuffer");
	retval = map_physical_memory (name,
									(void *) di->PCI.u.h0.base_registers_pci[1],
									di->PCI.u.h0.base_register_sizes[1],
#if defined(__INTEL__)
									B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
									B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
									B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr1);
#if defined(__INTEL__)
	/*  CPU might not do write-combining; try again.  */
	if (retval < 0)
	{
		retval = map_physical_memory (name,
			 (void *) di->PCI.u.h0.base_registers_pci[1],
			 di->PCI.u.h0.base_register_sizes[1],
			 B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void **) &ci->BaseAddr1);
	}
#endif

	if (retval < 0)
		goto err_base1;			  /*  Look down  */

	di->BaseAddr1ID = retval;
// dprintf(("3dfx_V3_Krnl: map_device - Mapping Frame Buffer, di->BaseAddr1ID = 0x%x\n", di->BaseAddr1ID));

	// Store the offset for the I/O registers
	di->io_base = di->PCI.u.h0.base_registers_pci[2];

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
// dprintf(("3dfx_V3_Krnl: map_device - Error Mapping Device returning 0x%x\n", retval));
		return (retval);
	}

	// Store the PCI register values
	for (i = 0; i <= 2; i++)
	{
		ci->base_registers_pci[i] = (uint8 *) di->PCI.u.h0.base_registers_pci[i];
	}
// dprintf(("3dfx_V3_Krnl: map_device - EXIT\n"));
	return (B_OK);
}

static void unmap_device (struct devinfo *di)
{
	register thdfx_card_info *ci = di->ci;

//dprintf (("3dfx_V3_Krnl: unmap_device - ENTER di->name =  %s\n", di->Name));

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
//dprintf (("3dfx_V3_Krnl: unmap_device - EXIT\n"));
}

/****************************************************************************
 * Surf the PCI bus for devices we can use.
 */
static void probe_devices ()
{
	long pci_index = 0;
	devinfo *di = _V3_dd.DI;
	uint32 count = 0;

	while ((count < MAX_CARDS) &&
			 ((*_V3_dd.pci_bus->get_nth_pci_info) (pci_index, &(di->PCI)) == B_NO_ERROR))
	{
		if ((di->PCI.vendor_id == VENDORID_3DFX) && (
					(di->PCI.device_id == DEVICEID_BANSHEE) ||
					(di->PCI.device_id == DEVICEID_AVENGER)))
		{
			sprintf (di->Name,
						"graphics%s/%04X_%04X_%02X%02X%02X",
						GRAPHICS_DEVICE_PATH,
						di->PCI.vendor_id, di->PCI.device_id,
						di->PCI.bus, di->PCI.device, di->PCI.function);

			_V3_dd.DevNames[count] = di->Name;
			di->SlotNum = pci_index;

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

	_V3_dd.NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	_V3_dd.DevNames[count] = NULL;

	if (count == 0)
	{
		dprintf (("3dfx_V3_Krnl: No 3dfx cards installed - EXIT\n"));
		return;
	}
	else
	{
		dprintf (("3dfx_V3_Krnl: %d 3dfx card(s) installed - EXIT\n", count));
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

	dprintf (("3dfx_V3_Krnl: init_hardware - Enter - PCI 3dfx-driver: %s %s\n", __DATE__, __TIME__));

	if ((idx = get_module (B_PCI_MODULE_NAME, (module_info **) & _V3_dd.pci_bus)) != B_OK)
		return ((status_t) idx);

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0; (*_V3_dd.pci_bus->get_nth_pci_info) (idx, &pcii) == B_NO_ERROR; idx++)
	{
		if ((pcii.vendor_id == VENDORID_3DFX) && (
							(pcii.device_id == DEVICEID_BANSHEE) ||
							(pcii.device_id == DEVICEID_AVENGER)))
		{
			found_one = TRUE;
			break;
		}
	}

	put_module (B_PCI_MODULE_NAME);
	_V3_dd.pci_bus = NULL;
	
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

	init_guard();

	dprintf (("3dfx_V3_Krnl: init_driver - Enter - PCI 3dfx-driver: %s %s\n", __DATE__, __TIME__));
	
	memset( &_V3_dd, 0, sizeof( _V3_dd ));

	/* Get a handle for the PCI bus */
	if ((retval = get_module (B_PCI_MODULE_NAME, (module_info **) & _V3_dd.pci_bus)) != B_OK)
		return (retval);

	if ((retval = initOwnedBena4 (&_V3_dd.DriverLock, "3Dfx kernel driver", B_SYSTEM_TEAM)) < 0)
	{
		put_module (B_PCI_MODULE_NAME);
		return (retval);
	}

	/*  Find all of our supported devices  */
	probe_devices ();

#if defined(THDFX_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
	add_debugger_command ("thdfxdump", thdfxdump, "thdfxdump - dump 3Dfx kernel driver data");
#endif

	dprintf (("3dfx_V3_Krnl: init_driver - EXIT\n"));
	return (B_OK);
}

/****************************************************************************
 * Interrupt management.
 */
static void clearinterrupts ( thdfx_card_info *ci )
{
	/* Enable VBLANK interrupt (and *only* that). */
	uint32 temp = _V3_ReadReg( ci, V3_2D_INTR_CTRL );
	temp &= ~(1 << 8);	/*  Clear Vertical Sync (Rising Edge) Interrupt generated flag  */
	temp |= 1 << 31;	/*  Write '1' to Extern pin pci_inta (active  low) to clear external PCI Interrupt  */
	_V3_WriteReg_NC( ci, V3_2D_INTR_CTRL, temp );
}

static void enableinterrupts (register struct thdfx_card_info *ci, int enable)
{
	uint32 temp = _V3_ReadReg( ci, V3_2D_INTR_CTRL );
	if (enable)
		temp |= 1 << 2;	/* Enable Vertical Sync (Rising Edge) Interrupts */
	else
		temp &= ~(1 << 2);
	_V3_WriteReg_NC( ci, V3_2D_INTR_CTRL, temp );
}

static int32 thdfx_interrupt (void *data)
{
	devinfo *di = (devinfo *)data;
	thdfx_card_info *ci = di->ci;
	uint32 bits;
	bool resched;

	resched = false;

	_V3_dd.NCalls++;
	if (!(di->IRQEnabled))
		return (B_UNHANDLED_INTERRUPT);

	bits = _V3_ReadReg( ci, V3_2D_INTR_CTRL );

	if (!(bits & (1 << 8)))
		return (B_UNHANDLED_INTERRUPT);	/* Not a Vertical Sync Interrupt */

	/*  Clear the interrupt bit.  */
	_V3_WriteReg_NC( ci, V3_2D_INTR_CTRL, (bits & (~(1 << 8))) | (1 << 31) );

	_V3_dd.NInterrupts++;
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
				regBase = di->io_base;
				fb_offset = (uint32) ci->FBBase - (uint32) ci->BaseAddr1;
				stride = ci->BytesPerRow;
				fb_offset += ci->DisplayAreaY * stride;
				_V3_WriteReg_NC ( di->ci, V3_VID_DESKTOP_START_ADDR,
						(fb_offset & SST_VIDEO_START_ADDR) << SST_VIDEO_START_ADDR_SHIFT);
			}
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR)
		{
			_V3_WriteReg_NC ( di->ci, V3_VID_CUR_LOC,
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

	return resched ? (B_INVOKE_SCHEDULER) : (B_HANDLED_INTERRUPT);
}

int32 int_check_handler (timer * t)
{
	devinfo *di = (devinfo *) t;
	thdfx_card_info *ci = di->ci;

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

//dprintf(("3dfx_V3_Krnl: firstopen - ENTER - di = 0x%08lx\n", di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);
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
//    dprintf(("3dfx_V3_Krnl: firstopen - init_locks returned 0x%x\n", retval));
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
//    dprintf(("3dfx_V3_Krnl: firstopen map_device() returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->ci_AreaID >= 0)
		{
			delete_area (di->ci_AreaID);
			di->ci_AreaID = -1;
			di->ci = NULL;
		}
	}

	// Initialize ci_regs so that the interrupt handler works correctly
	ci->Regs = (thdfxRegs *) ci->BaseAddr0;

	/* Add a kernel timer - when this 'goes off' we should check to see whether
	 * any VBL interupts have occured - if they haven't we need to change the setting
	 * of ci_IRQFlags. This should catch some (notably Diamonds) cards which do not seem to
	 * enable Interrupts correctly !
	 */
	add_timer (&(di->int_check_timer), &int_check_handler, INT_CHECK_TIMEOUT,
				  B_ONE_SHOT_RELATIVE_TIMER);

	/*  Install interrupt handler  */
	if ((di->PCI.u.h0.interrupt_line != 0) &&
		 (di->PCI.u.h0.interrupt_line != 0xff) &&
		 ((retval
			=
			install_io_interrupt_handler (di->PCI.u.h0.interrupt_line, thdfx_interrupt, (void *) di,
													0)) == B_OK))
	{
//    dprintf(("3dfx_V3_Krnl: firstopen - install_io_interrupt_handle() returned 0x%x, set IRQF__ENABLED\n", retval));
		ci->IRQFlags |= IRQF__ENABLED;
	}
	else
	{
//    dprintf(("3dfx_V3_Krnl: firstopen - install_io_interrupt_handle() returned 0x%x, not setting IRQF__ENABLED\n", retval));
		retval = B_OK;
	}

	clearinterrupts (ci);

	/* notify the interrupt handler that this card is a candidate */
	if (ci->IRQFlags & IRQF__ENABLED)
	{
		di->IRQEnabled = 1;
		//enableinterrupts (ci, TRUE);
	}

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->DevName, di->Name);

	/* Note exactly which card variant this is */
	switch (di->PCI.device_id)
	{
	case DEVICEID_BANSHEE:
		ci->device_id = DEVICEID_BANSHEE;
		break;
	case DEVICEID_AVENGER:
		ci->device_id = DEVICEID_AVENGER;
		break;
	default:
		ci->device_id = 0;
		break;
	}

	/* Create the memMgr area */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X MemMgr",
				di->PCI.vendor_id, di->PCI.device_id,
				di->PCI.bus, di->PCI.device, di->PCI.function);

	if (__mem_Initialize (&di->MemMgr, 16 * 1024 * 1024, 4096, shared_name) < B_OK)
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

	// Make this non-zero to prevent reset by free during scan
	di->PrimOverideID = -1;

	/*  All done, return the status.  */
// dprintf(("3dfx_V3_Krnl: firstopen returning 0x%08x\n", retval));
	return (retval);
}

static void lastclose (struct devinfo *di)
{
// dprintf(("3dfx_V3_Krnl: lastclose - ENTER\n"));

	di->IRQEnabled = 0;

	enableinterrupts (di->ci, FALSE);
	clearinterrupts (di->ci);
	remove_io_interrupt_handler (di->PCI.u.h0.interrupt_line, thdfx_interrupt, di);

	unmap_device (di);
	dispose_locks (di->ci);

	delete_area (di->ci_AreaID);
	di->ci_AreaID = -1;
	di->ci = NULL;

// dprintf(("3dfx_V3_Krnl: lastclose - EXIT\n"));
}

static long fx_open (const char *name, ulong flags, void **cookie)
{
	int n;
	devinfo *di;
	status_t retval = B_OK;
	per_open_data *pod;

	dprintf (("3dfx_V3_Krnl: fx_open - ENTER, name is %s\n", name));

	lockBena4 (&_V3_dd.DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0; _V3_dd.DevNames[n] && strcmp (name, _V3_dd.DevNames[n]); n++)
		;
	di = &_V3_dd.DI[n];

	dprintf (("3dfx_V3_Krnl: fx_open - n=%ld  di=%p\n", n, di));

	if (di->Opened == 0)
	{
		if ((retval = firstopen (di)) != B_OK)
		{
			dprintf(("3dfx_V3_Krnl: fx_open - firstopen() != B_OK !\n"));
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
	unlockBena4 (&_V3_dd.DriverLock);

	/*  All done, return the status.  */
	dprintf (("3dfx_V3_Krnl: fx_open - returning 0x%08x\n", retval));
	return (retval);
}

static long fx_close (void *_cookie)
{
	dprintf (("3dfx_V3_Krnl: fx_close\n"));
	//put_module(pci_name);
	return B_NO_ERROR;
}

static long fx_free (void *cookie)
{
	per_open_data *pod = (per_open_data *) cookie;
	devinfo *di = pod->di;
	thdfx_card_info *ci = di->ci;

	dprintf (("3dfx_V3_Krnl: fx_free - ENTER\n"));

	lockBena4 (&_V3_dd.DriverLock);
	__mem_RemoveIndex (di->MemMgr, pod->memMgrKey);

	if (ci->OverlayOwnerID == pod->memMgrKey)
	{
		uint32 temp = ci->reg.r_V3_VID_PROC_CFG;
		temp &= ~(1 << 8);	// Disable Overlay
		temp |= 1 << 7;	// Enable Desktop
		_V3_WriteReg_NC( di->ci, V3_VID_PROC_CFG, temp );
		ci->OverlayOwnerID = -1;
	}

	if (di->PrimOverideID == pod->memMgrKey)
	{
		di->PrimOverideID = -1;
		_V3_SetVideoMode (di, di->PrimXRes, di->PrimYRes, di->PrimRefresh, di->PrimLoacClut);
	}

	if( di->engine_3d_lock == pod->memMgrKey )
	{
		di->engine_3d_lock = 0;
	}

	/*  Mark the device available.  */
	if (--di->Opened == 0)
		lastclose (di);

	unlockBena4 (&_V3_dd.DriverLock);
	dprintf (("3dfx_V3_Krnl: fx_free - EXIT\n"));

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
	while ( !(_V3_ReadReg( di->ci, V3_VID_STATUS ) & 0x3f ) )
		;
}


static long fx_control (void *_cookie, ulong msg, void *buf, size_t len)
{
	per_open_data *pod = (per_open_data *) _cookie;
	devinfo *di = pod->di;
	status_t retval = B_DEV_INVALID_IOCTL;

check_guard();

	switch (msg)
	{
		/* Stuff the accelerant needs to do business. */
	case B_GET_ACCELERANT_SIGNATURE:
		strcpy ((char *) buf, "3dfx_banshee.accelerant");
		retval = B_OK;
		break;

	case B_GET_3D_SIGNATURE:
		dprintf (("3dfx_V3_Krnl: fx_control - B_GET_3D_SIGNATURE\n"));
		strcpy ((char *) buf, "3dfx_Voodoo3.3da");
		retval = B_OK;
		break;

		/*
		 * PRIVATE ioctl from here on.
		 */
	case THDFX_IOCTL_GETGLOBALS:
		{
			thdfx_getglobals *tmp = (thdfx_getglobals *) buf;

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
			tmp->gsp_Value = (*_V3_dd.pci_bus->read_pci_config) (
				di->PCI.bus, di->PCI.device, di->PCI.function,
				tmp->gsp_Offset, tmp->gsp_Size);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_SET_PCI:
		{
			thdfx_getsetpci *tmp = (thdfx_getsetpci *) tmp;
			(*_V3_dd.pci_bus->write_pci_config) (
				di->PCI.bus, di->PCI.device, di->PCI.function,
				tmp->gsp_Offset, tmp->gsp_Size, tmp->gsp_Value);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PCI_32:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			checkForRoom( di );
			{
				uint32 *regs = (uint32 *) di->ci->BaseAddr0;
				regs[ tmp->offset >> 2 ] = tmp->val;
			}
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_READ_PCI_32:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			tmp->val = _V3_ReadReg( di->ci, tmp->offset);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PHYS_8:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			_V3_WriteReg_8 (di, tmp->offset, tmp->val);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_READ_PHYS_8:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			tmp->val = _V3_ReadReg_8 (di, tmp->offset);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_WRITE_PHYS_16:
		{
			thdfx_readwritepci32 *tmp = (thdfx_readwritepci32 *) buf;
			_V3_WriteReg_16 (di, tmp->offset, tmp->val);
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
			__mem_SetSem (di->MemMgr, pod->memMgrKey, buf);
			retval = B_OK;
			break;
		}

	case MEM_IOCTL_RELEASE_SEMS:
		{
			__mem_ReleaseSems (di->MemMgr);
			retval = B_OK;
			break;
		}

	case THDFX_IOCTL_SET_VIDEO_MODE:
		{
			thdfx_setvideomode *mode = (thdfx_setvideomode *) buf;
			if (mode->xRes)
			{
				if (_V3_SetVideoMode (di, mode->xRes, mode->yRes, mode->refresh, mode->loadClut))
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
				_V3_SetVideoMode (di, di->PrimXRes, di->PrimYRes, di->PrimRefresh, di->PrimLoacClut);
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
#define	ENABLE_TVOUT	((int *) buf)
//    dprintf(("3dfx_V3_Krnl: fx_control - THDFX_IOCTL_ENABLE_TVOUT, enable = %d\n",*ENABLE_TVOUT));
			retval = B_OK;
			break;
		}

		/* I2C Routines */
	case THDFX_IOCTL_READ_I2C_REG:
		{
#define	I2C_REG_VAL	((int *) buf)
			int32 LTemp;
			LTemp = _V3_ReadReg( di->ci, V3_VID_SERIAL_PARALLEL_PORT );
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
			LTemp = _V3_ReadReg( di->ci, V3_VID_SERIAL_PARALLEL_PORT );
			LTemp &= ~(SST_SERPAR_I2C_SCK_OUT | SST_SERPAR_I2C_DSA_OUT);	// clear all I2C bits
			LTemp |= (*I2C_REG_VAL) << 24;	// Shift in the new values for clock & data
			_V3_WriteReg_NC( di->ci, V3_VID_SERIAL_PARALLEL_PORT, LTemp );
			// Clock must be held for at least 5us so we spin here with interrupts disabled
			cs = disable_interrupts ();
			spin (5);
			restore_interrupts (cs);
			retval = B_OK;
			break;
		}

	default:
		dprintf (("3dfx_V3_Krnl: fx_control - Unknown ioctl = 0x%x\n", msg));
		break;
	}
// dprintf("3dfx: fx_control - EXIT, returning 0x%x\n", retval);
	return (retval);
}

device_hooks *find_device (const char *name)
{
	int index;
	for (index = 0; _V3_dd.DevNames[index]; index++)
	{
		if (strcmp (name, _V3_dd.DevNames[index]) == 0)
			return &thdfx_device_hooks;
	}
	return (NULL);
}

void uninit_driver (void)
{
	dprintf (("3dfx_V3_Krnl: uninit_driver - Enter\n"));

#if defined(THDFX_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
	remove_debugger_command ("thdfxdump", thdfxdump);
#endif
	dprintf (("3dfx_V3_Krnl: uninit_driver - Exit\n"));
}

const char **publish_devices (void)
{
	const char **dev_list = NULL;
	/* return the list of supported devices */
// dprintf(("3dfx_V3_Krnl: publish_devices\n"));
	dev_list = (const char **) _V3_dd.DevNames;
	return (dev_list);
}

/****************************************************************************
 * Debuggery.
 */

#if defined(THDFX_DEBUG)   || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
static int thdfxdump (int argc, char **argv)
{
	int i, k;
	uint32 readPtrL1, readPtrL2, hwreadptr;
	readPtrL1 = 1;
	readPtrL2 = 2;
	hwreadptr = 0;

	kprintf ("3Dfx kernel driver\nThere are %d device(s)\n", _V3_dd.NDevs);
	kprintf ("Driver-wide benahpore: %d/%d\n",
				_V3_dd.DriverLock.b4_Sema4, _V3_dd.DriverLock.b4_FastLock);
	kprintf ("Total calls to IRQ handler: %ld\n", _V3_dd.NCalls);
	kprintf ("Total relevant interrupts: %ld\n", _V3_dd.NInterrupts);

	for (i = 0; i < _V3_dd.NDevs; i++)
	{
		devinfo *di = &_V3_dd.DI[i];
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
			__mem_Kprint_alloc (ci->AllocFrameBuffer);

			kprintf ("  ci_AllocSentinal \n");
			__mem_Kprint_alloc (ci->AllocSentinal);

			kprintf ("  ci_AllocCursor \n");
			__mem_Kprint_alloc (ci->AllocCursor);

			for (k = 0; k < 2; k++)
			{
/*
				uint32 status;
				kprintf ("  ci_AllocFifo %x \n", k);
				__mem_Kprint_alloc (ci->AllocFifo[k]);
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

				hwreadptr = (((uint32) ci->CmdTransportInfo[k].fifoStart) +
								 readPtrL2 - (uint32) ci->CmdTransportInfo[k].fifoOffset);
				kprintf ("  ci_CmdTransportInfo[%d]: HW Read Ptr = 0x%08lx\n", k, hwreadptr);
*/
			}
		}
	}
	kprintf ("\n");
	return (B_OK);
}
#endif
