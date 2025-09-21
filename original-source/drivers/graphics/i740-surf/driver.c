/* :ts=8 bk=0
 *
 * driver.c:	Core driver for Intel I740 card.
 *
 * Leo L. Schwab					1998.11.05
 */
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <drivers/genpool_module.h>
#include <device/graphic_driver.h>
#include <kernel/OS.h>
#include <dinky/bena4.h>
#include <stdio.h>
#include <string.h>

#include <graphics_p/i740-surf/i740.h>
#include <graphics_p/i740-surf/debug.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * #defines
 */
#define CHECKBIT(var,device,bitfield) \
	if (var & device##_Mask (bitfield)) {\
		dprintf(("--- IRQ: "#device"_"#bitfield" Set\n"));\
	}


/****************************************************************************
 * Prototypes.
 */
static status_t init_locks (struct devinfo *di, team_id team);
static void dispose_locks (struct devinfo *di);
static status_t map_device (register struct devinfo *di);
static void unmap_device (struct devinfo *di);
static void probe_devices (void);
static void clearinterrupts (register struct i740_card_info *ci);
static void enableinterrupts (register struct i740_card_info *ci,
			      int enable);
static int32 i740_interrupt (void *data);
static status_t open_hook (const char *name, uint32 flags, void **cookie);
static status_t read_hook (void *dev, off_t pos, void *buf, size_t *len);
static status_t write_hook (void *dev, off_t pos, const void *buf, size_t *len);
static status_t close_hook (void *dev);
static status_t free_hook (void *dev);
static status_t control_hook (void *dev, uint32 msg, void *buf, size_t len);
static int i740dump (int argc, char **argv);
static int idxinb (int ac, char **av);
static int idxoutb (int ac, char **av);


/****************************************************************************
 * Globals.
 */
pci_module_info		*pci_bus;
genpool_module		*gpm;
BPoolInfo		*gfbpi;
driverdata		*i740dd;


static uint32		ncalls;

static device_hooks	graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL
};


/****************************************************************************
 * {Sem,Ben}aphore initialization.
 */
static status_t
init_locks (struct devinfo *di, team_id team)
{
	i740_card_info	*ci;
	i740_card_ctl	*cc;
	status_t	retval;

	ci = di->di_I740CardInfo;
	cc = di->di_I740CardCtl;

	/*
	 * Initialize semaphores for the driver.
	 */
	if ((retval = create_sem (1, "I740 CRTC lock")) < 0)
		return (retval);
	ci->ci_CRTCLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "I740 CLUT lock")) < 0)
		return (retval);
	ci->ci_CLUTLock = retval;
	set_sem_owner (retval, team);

	/*  VBlank semaphore gets zero thread count  */
	if ((retval = create_sem (0, "I740 VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);

	/*
	 * Accelerant-only locks
	 */
	if ((retval = BInitOwnedBena4 (&cc->cc_EngineLock,
				       "I740 rendering engine lock",
				       team)) < 0)
		return (retval);

	return (B_OK);
}

static void
dispose_locks (struct devinfo *di)
{
	i740_card_info	*ci;
	i740_card_ctl	*cc;

	ci = di->di_I740CardInfo;
	cc = di->di_I740CardCtl;

	BDisposeBena4 (&cc->cc_EngineLock);

	if (ci->ci_VBlankLock > 0)
		delete_sem (ci->ci_VBlankLock), ci->ci_VBlankLock = -1;
	if (ci->ci_CLUTLock > 0)
		delete_sem (ci->ci_CLUTLock), ci->ci_CLUTLock = -1;
	if (ci->ci_CRTCLock > 0)
		delete_sem (ci->ci_CRTCLock), ci->ci_CRTCLock = -1;
}


/****************************************************************************
 * Map device memory into local address space.
 */
static status_t
map_device (register struct devinfo *di)
{
	i740_card_info	*ci = di->di_I740CardInfo;
	status_t	retval;
	uint32		mappedio;
	int		len;
	char		name[B_OS_NAME_LENGTH];

	/*  Create a goofy basename for the areas  */
	sprintf (name, "%04X_%04X_%02X%02X%02X",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (name);

	/*
	 * Map framebuffer RAM
	 */
	ci->ci_BaseAddr0_DMA = (uint8 *) di->di_PCI.u.h0.base_registers_pci[0];
	strcpy (name + len, " framebuffer");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[0],
			      di->di_PCI.u.h0.base_register_sizes[0],
#if defined(__INTEL__)
			      B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
			      B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr0);
#if defined(__INTEL__)
	if (retval < 0)
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		retval = map_physical_memory
			  (name,
			   (void *) di->di_PCI.u.h0.base_registers[0],
			   di->di_PCI.u.h0.base_register_sizes[0],
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr0);
#endif

	if (retval < 0)
		goto err_base0;	/*  Look down  */
	di->di_BaseAddr0ID = retval;

	/*
	 * Map chip registers.
	 */
	strcpy (name + len, " regs");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[1],
			      di->di_PCI.u.h0.base_register_sizes[1],
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr1);
	if (retval < 0)
		goto err_base1;	/*  Look down  */
	di->di_BaseAddr1ID = retval;

	if (retval < 0) {
		delete_area (di->di_BaseAddr1ID);
		di->di_BaseAddr1ID = -1;
		ci->ci_BaseAddr1 = NULL;
err_base1:	delete_area (di->di_BaseAddr0ID);
		di->di_BaseAddr0ID = -1;
		ci->ci_BaseAddr0 = NULL;
err_base0:	return (retval);
	}

	return (B_OK);
}

static void
unmap_device (struct devinfo *di)
{
	register i740_card_info *ci = di->di_I740CardInfo;

dprintf (("+++ Unmapping device %s\n", di->di_Name));
	if (di->di_BaseAddr1ID >= 0)
		delete_area (di->di_BaseAddr1ID), di->di_BaseAddr1ID = -1;
	if (di->di_BaseAddr0ID >= 0)
		delete_area (di->di_BaseAddr0ID), di->di_BaseAddr0ID = -1;
	ci->ci_BaseAddr0	=
	ci->ci_BaseAddr0_DMA	=
	ci->ci_BaseAddr1	= NULL;
	ci->ci_FBBase		= 0;
}


/****************************************************************************
 * Surf the PCI bus for devices we can use.
 */
static void
probe_devices (void)
{
	long	pci_index = 0;
	uint32	count = 0;
	uint32	pcicmd;
	devinfo	*di = i740dd->dd_DI;

	pcicmd = PCI_command_memory |
#if (DEBUG  ||  FORCE_DEBUG)
		 PCI_command_io |
#endif
		 PCI_command_master;

	while ((count < MAXDEVS)  &&
	       ((*pci_bus->get_nth_pci_info)(pci_index, &(di->di_PCI)) ==
	        B_NO_ERROR))
	{
		if (di->di_PCI.vendor_id == VENDORID_INTEL  &&
		    di->di_PCI.device_id == DEVICEID_I740)
		{
			/*
			 * Found a card we can deal with; create a device
			 * for it.
			 */
			sprintf (di->di_Name,
				 "graphics%s/%04X_%04X_%02X%02X%02X",
				 GRAPHICS_DEVICE_PATH,
				 di->di_PCI.vendor_id, di->di_PCI.device_id,
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);
			i740dd->dd_DevNames[count] = di->di_Name;
dprintf (("+++ making /dev/%s\n", di->di_Name));

			/*  Initialize to...  uh...  uninitialized.  */
			di->di_BaseAddr0ID		=
			di->di_BaseAddr1ID		=
			di->di_I740CardInfo_AreaID	= -1;
			di->di_NInterrupts		=
			di->di_NVBlanks			=
			di->di_LastIrqMask		= 0;

			/*
			 * Enable memory and I/O access.  (It took me half a
			 * day to figure out I needed to do this.)
			 */
			set_pci (&di->di_PCI,
				 PCI_command,
				 2,
				 get_pci (&di->di_PCI, PCI_command, 2) |
				  pcicmd);

			di++;
			count++;
		}
		pci_index++;
	}
	i740dd->dd_NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	i740dd->dd_DevNames[count] = NULL;
dprintf (("+++ probe_devices: %d supported devices\n", count));
}


/****************************************************************************
 * Interrupt management.
 */
static void
clearinterrupts (register struct i740_card_info *ci)
{
	/*
	 * Enable VBLANK interrupt (and *only* that).
	 */
	ci->ci_Regs->IMR = ~MASKEXPAND (IxR_VBLANK); /*  IIR status enable  */
	ci->ci_Regs->IIR = MASKEXPAND (IxR__ALLIRQS);  /*  Clear sources  */
}

static void
enableinterrupts (register struct i740_card_info *ci, int enable)
{
	if (enable)
		ci->ci_Regs->IER |= MASKEXPAND (IxR_VBLANK);
	else
		ci->ci_Regs->IER &= ~MASKEXPAND (IxR_VBLANK);
}

static int32
i740_interrupt (void *data)
{
	register devinfo	*di;
	register i740_card_info	*ci;
	register uint32		bits;

	di = data;
	ncalls++;
	if (!di->di_IRQEnabled)
		return (B_UNHANDLED_INTERRUPT);

	ci = di->di_I740CardInfo;
	if (!(bits = ci->ci_Regs->IIR))
		/*  Wasn't us.  */
		return (B_UNHANDLED_INTERRUPT);

	if (i740dd)
		i740dd->dd_NInterrupts++;
	di->di_NInterrupts++;
	di->di_LastIrqMask = bits;

	/*
	 * Most common case (not to mention the only case) is VBLANK;
	 * handle it first.
	 */
	if (GetBF (bits, IxR_VBLANK)) {
		register i740_card_ctl	*cc;
		register int32		*flags;
		int32			dummy;

		cc = di->di_I740CardCtl;

		/*  Writing the bit clears it.  */
		ci->ci_Regs->IIR = MASKEXPAND (IxR_VBLANK);

		di->di_NVBlanks++;
		flags = &cc->cc_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE) {
			/*
			 * Change the framebuffer base address here.
			 * The regs_CRTC[] values have already been set up.
			 */
			register vgaregs *vregs;

			vregs = &ci->ci_Regs->VGARegs;

			dummy = vregs->r.VGA_ST01;
			vregs->w.VGA_AR_Idx = 0;/*  Enable attribute access */
			dummy = vregs->r.VGA_ST01;/*  Reset phase again   */

			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_17_10,
				  cc->cc_CRTC[CR_FBBASE_17_10]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_9_2,
				  cc->cc_CRTC[CR_FBBASE_9_2]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_31_24,
				  cc->cc_CRTC[CR_FBBASE_31_24]);
			IDXREG_W (vregs,
				  VGA_CR,
				  CR_FBBASE_23_18,
				  cc->cc_CRTC[CR_FBBASE_23_18]);

			vregs->w.VGA_AR_Idx  = AR_HPAN;
			vregs->w.VGA_AR_ValW = cc->cc_ATTR[AR_HPAN];
			vregs->w.VGA_AR_Idx  = FIELDDEF (AR_IDX, ACCESS, LOCK);
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR) {
			/*
			 * Move the hardware cursor.
			 */
			movecursor (di, cc->cc_MousePosX, cc->cc_MousePosY);
		}

		/*  Release the vblank semaphore.  */
		if (ci->ci_VBlankLock >= 0) {
			int32 blocked;

			if (get_sem_count (ci->ci_VBlankLock,
					   &blocked) == B_OK  &&
			    blocked < 0)
				release_sem_etc (ci->ci_VBlankLock,
						 -blocked,
						 B_DO_NOT_RESCHEDULE);
		}
		bits &= ~MASKEXPAND (IxR_VBLANK);
	}

	if (bits) {
		/*  Where did this come from?  */
		dprintf (("+++ Weird I740 interrupt (tell ewhac): \
IIR = 0x%08lx\n", bits));
		/*  Clear extraneous sources.  */
		ci->ci_Regs->IIR = bits;
	}

	return (B_HANDLED_INTERRUPT);
}


//**************************************************************************
//  Space selection, on the fly...
//
//  This function is able to set any configuration available on the add-on
//  without restarting the machine. Until now, all cards works pretty well
//  with that. As Window's 95 is not really doing the same, we're still a
//  bit afraid to have a bad surprize some day... We'll see.
//  The configuration include space (depth and resolution), CRT_CONTROL and
//  refresh rate.
//**************************************************************************

static status_t
vid_selectmode (
register struct devinfo		*di,
register struct i740_card_info	*ci,
register display_mode		*dm
)
{
	status_t	retval;

	/*
	 * We are about to Change The World, so grab everything.
	 */
	acquire_sem (ci->ci_CRTCLock);
	acquire_sem (ci->ci_CLUTLock);

	enableinterrupts (ci, FALSE);
	retval = SetupCRTC (di, ci, dm);
	clearinterrupts (ci);
	enableinterrupts (ci, TRUE);

	/*
	 * The driver pays no attention to the accelerant's locks, since
	 * they can be trashed.  Anyone calling this function better be
	 * prepared to keep the rendering engine locked down.
	 */
	release_sem (ci->ci_CLUTLock);
	release_sem (ci->ci_CRTCLock);

	return (retval);
}


/****************************************************************************
 * Kernel entry points.
 */
/*
 * This is actually a misnomer; very little initialization is done here; it
 * just detects if there's a card installed that we can handle.
 */
status_t
init_hardware (void)
{
	pci_info	pcii;
	int32		idx = 0;
	bool		found_one = FALSE;
	
	if ((idx = get_module (B_PCI_MODULE_NAME,
			       (module_info **) &pci_bus)) != B_OK)
		/*  Well, really, what's the point?  */
		return ((status_t) idx);

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0;
	     (*pci_bus->get_nth_pci_info)(idx, &pcii) == B_NO_ERROR;
	     idx++)
	{
		if (pcii.vendor_id == VENDORID_INTEL  &&
		    pcii.device_id == DEVICEID_I740)
		{
			/*  My, that looks interesting...  */
			found_one = TRUE;
			break;
		}
	}

	put_module (B_PCI_MODULE_NAME);  pci_bus = NULL;
	return (found_one ?  B_OK :  B_ERROR);
}

status_t
init_driver (void)
{
	status_t retval;

	/*
	 * Get a handle for the PCI bus
	 */
	if ((retval = get_module (B_PCI_MODULE_NAME,
				  (module_info **) &pci_bus)) != B_OK)
		return (retval);

	/*
	 * Open the genpool module.
	 */
	if ((retval = get_module (B_GENPOOL_MODULE_NAME,
				  (module_info **) &gpm)) != B_OK)
		goto err0;

	/*  Create driver's private data area.  */
	if (!(i740dd = (driverdata *) calloc (1, sizeof (driverdata)))) {
		retval = B_NO_MEMORY;
		goto err1;	/*  Look down  */
	}

	if ((retval = BInitOwnedBena4 (&i740dd->dd_DriverLock,
				       "i740 kernel driver",
				       B_SYSTEM_TEAM)) < 0)
	{
		free (i740dd);
err1:		put_module (B_GENPOOL_MODULE_NAME);
err0:		put_module (B_PCI_MODULE_NAME);
		return (retval);
	}
	/*  Find all of our supported devices  */
	probe_devices ();

#if (DEBUG  ||  FORCE_DEBUG)
	add_debugger_command ("i740dump",
			      i740dump,
			      "i740dump - dump I740 kernel driver data");
	add_debugger_command ("idxinb",
			      idxinb,
			      "idxinb - dump VGA-style indexed I/O registers");
	add_debugger_command ("idxoutb",
			      idxoutb,
			      "idxoutb - write VGA-style indexed I/O registers");
#endif

	return (B_OK);
}

const char **
publish_devices (void)
{
	/*  Return the list of supported devices  */
	return (i740dd->dd_DevNames);
}

device_hooks *
find_device (const char *name)
{
	int index;

	for (index = 0;  i740dd->dd_DevNames[index];  index++) {
		if (strcmp (name, i740dd->dd_DevNames[index]) == 0)
			return (&graphics_device_hooks);
	}
	return (NULL);

}

void
uninit_driver (void)
{
dprintf (("+++ uninit_driver\n"));

	if (i740dd) {
		cpu_status cs;

#if (DEBUG  ||  FORCE_DEBUG)
		remove_debugger_command ("idxoutb", idxoutb);
		remove_debugger_command ("idxinb", idxinb);
		remove_debugger_command ("i740dump", i740dump);
#endif
		BDisposeBena4 (&i740dd->dd_DriverLock);
		free (i740dd);
		i740dd = NULL;
	}
	if (gpm)	put_module (B_GENPOOL_MODULE_NAME), gpm = NULL;
	if (pci_bus)	put_module (B_PCI_MODULE_NAME), pci_bus = NULL;
}


void
idle_device (struct devinfo *di)
{
	/* put device in lowest power consumption state */
	/* Is this a kernel function?  */
}


/****************************************************************************
 * Device hook (and support) functions.
 */
static status_t
firstopen (struct devinfo *di)
{
	i740_card_info	*ci;
	i740_card_ctl	*cc;
	thread_id	thid;
	thread_info	thinfo;
	status_t	retval = B_OK;
	int		n, len;
	char		shared_name[B_OS_NAME_LENGTH];

dprintf (("+++ firstopen(0x%08lx)\n", di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (shared_name);

	/*  Read-only globals  */
	strcpy (shared_name + len, " (RO)");
	n = (sizeof (struct i740_card_info) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_I740CardInfo,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err0;	/*  Look down  */
	di->di_I740CardInfo_AreaID = retval;

	/*  Read/write globals  */
	strcpy (shared_name + len, " (R/W)");
	n = (sizeof (struct i740_card_ctl) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_I740CardCtl,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err1;	/*  Look down  */
	di->di_I740CardCtl_AreaID = retval;

	ci = di->di_I740CardInfo;
	cc = di->di_I740CardCtl;
	memset (ci, 0, sizeof (*ci));
	memset (cc, 0, sizeof (*cc));

	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (di, thinfo.team)) < 0)
		goto err2;	/*  Look down  */

	/*  Map the card's resources into memory.  */
	if ((retval = map_device (di)) < 0)
		goto err2;	/*  Look down  */

	/*  Create Pool descriptor (which doesn't get used much)  */
	if (!(gfbpi = (gpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err3;	/*  Look down  */
	}
	
	/*  Initialize the rest of the card.  */
	if ((retval = i740_init (di)) < 0) {
		(gpm->gpm_FreePoolInfo) (gfbpi);
		gfbpi = NULL;
err3:		unmap_device (di);
err2:		dispose_locks (di);
		if (di->di_I740CardCtl_AreaID >= 0) {
			delete_area (di->di_I740CardCtl_AreaID);
			di->di_I740CardCtl_AreaID = -1;
			di->di_I740CardCtl = NULL;
		}
err1:		if (di->di_I740CardInfo_AreaID >= 0) {
			delete_area (di->di_I740CardInfo_AreaID);
			di->di_I740CardInfo_AreaID = -1;
			di->di_I740CardInfo = NULL;
		}
err0:		return (retval);
	}

	/*  Install interrupt handler  */
	install_io_interrupt_handler
	 (di->di_PCI.u.h0.interrupt_line, i740_interrupt, (void *) di, 0);

	clearinterrupts (ci);
	enableinterrupts (ci, TRUE);

	/* notify the interrupt handler that this card is a candidate */
	di->di_IRQEnabled = 1;

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->ci_DevName, di->di_Name);

	/*  All done, return the status.  */
dprintf (("+++ firstopen returning 0x%08x\n", retval));
	return (retval);
}

static void
lastclose (struct devinfo *di)
{
	cpu_status	cs;

dprintf (("+++ lastclose() begins...\n"));

	di->di_IRQEnabled = 0;

	enableinterrupts (di->di_I740CardInfo, FALSE);
	clearinterrupts (di->di_I740CardInfo);
	remove_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
				     i740_interrupt, di);

	if (di->di_I740CardInfo->ci_PoolID)
		(gpm->gpm_DeletePool) (di->di_I740CardInfo->ci_PoolID);
	free (di->di_DispModes, di->di_NDispModes * sizeof (display_mode));
	di->di_DispModes = NULL;
	di->di_NDispModes = 0;

	(gpm->gpm_FreePoolInfo) (gfbpi);
	gfbpi = NULL;
	unmap_device (di);
	dispose_locks (di);
	delete_area (di->di_I740CardCtl_AreaID);
	di->di_I740CardCtl_AreaID = -1;
	di->di_I740CardCtl = NULL;
	delete_area (di->di_I740CardInfo_AreaID);
	di->di_I740CardInfo_AreaID = -1;
	di->di_I740CardInfo = NULL;
dprintf (("+++ lastclose() completes.\n"));
}


static status_t
open_hook (const char *name, uint32 flags, void **cookie)
{
	i740_card_info	*ci;
	devinfo		*di;
	status_t	retval = B_OK;
	int		n;

dprintf (("+++ open_hook(%s, %d, 0x%08x)\n", name, flags, cookie));

	BLockBena4 (&i740dd->dd_DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0;
	     i740dd->dd_DevNames[n]  &&  strcmp (name, i740dd->dd_DevNames[n]);
	     n++)
		;
	di = &i740dd->dd_DI[n];

	if (di->di_Opened == 0) {
		if ((retval = firstopen (di)) != B_OK)
			goto err;	/*  Look down  */
	}

	/*  Mark the device open.  */
	di->di_Opened++;

	/*  Squirrel this away.  */
	*cookie = di;
	
	/*  End of critical section.  */
err:	BUnlockBena4 (&i740dd->dd_DriverLock);

	/*  All done, return the status.  */
dprintf (("+++ open_hook returning 0x%08x\n", retval));
	return (retval);
}

/*
 * Read, write, and close hooks.  They do nothing gracefully.  Or rather, they
 * gracefully do nothing.  Whatever...
 */
static status_t
read_hook (void *dev, off_t pos, void *buf, size_t *len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}

static status_t
write_hook (void *dev, off_t pos, const void *buf, size_t *len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}

static status_t
close_hook (void *dev)
{
	dprintf(("+++ close_hook(%08x)\n", dev));
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}

/*
 * free_hook - close down the device
 */
static status_t
free_hook (void *dev)
{
	devinfo		*di = dev;

dprintf (("+++ free_hook() begins...\n"));

	BLockBena4 (&i740dd->dd_DriverLock);

	/*  Mark the device available.  */
	if (--di->di_Opened == 0)
		lastclose (di);

	BUnlockBena4 (&i740dd->dd_DriverLock);
dprintf (("+++ free_hook() completes.\n"));

	return (B_OK);
}


/*
 * control_hook - where the real work is done
 */
static status_t
control_hook (void *dev, uint32 msg, void *buf, size_t len)
{
	devinfo		*di = dev;
	i740_card_info	*ci = di->di_I740CardInfo;
	status_t	retval = B_DEV_INVALID_IOCTL;

//if (msg != MAXIOCTL_I740 + B_SET_CURSOR_SHAPE  &&
//    msg != MAXIOCTL_I740 + B_SHOW_CURSOR)
//     dprintf (("+++ ioctl: %d, buf: 0x%08x, len: %d\n", msg, buf, len));

	switch (msg) {
	/*
	 * Stuff the accelerant needs to do business.
	 */
	/*
	 * The only PUBLIC ioctl.
	 */
	case B_GET_ACCELERANT_SIGNATURE:
		strcpy ((char *) buf, "i740.accelerant");
		retval = B_OK;
		break;
	
	/*
	 * PRIVATE ioctl from here on.
	 */
	case I740_IOCTL_GETGLOBALS:
#define	GG	((i740_getglobals *) buf)

		/*
		 * Check protocol version to see if client knows how to talk
		 * to us.
		 */
		if (GG->gg_ProtocolVersion == I740_IOCTLPROTOCOL_VERSION) {
			GG->gg_GlobalArea_CI = di->di_I740CardInfo_AreaID;
			GG->gg_GlobalArea_CC = di->di_I740CardCtl_AreaID;
			retval = B_OK;
		}
		break;

	case I740_IOCTL_GET_PCI:
#define	GSP	((i740_getsetpci *) buf)

		GSP->gsp_Value = get_pci (&di->di_PCI,
					  GSP->gsp_Offset,
					  GSP->gsp_Size);
		retval = B_OK;
		break;

	case I740_IOCTL_SET_PCI:
		set_pci (&di->di_PCI, GSP->gsp_Offset,
			 GSP->gsp_Size, GSP->gsp_Value);
		retval = B_OK;
		break;


	/*
	 * Accelerant operations which are, on balance, better handled inside
	 * the kernel device.  (Note the cutesy method of assigning ioctl 'msg'
	 * values.)
	 */
	/*  Mode configuration  */
	case MAXIOCTL_I740 + B_ACCELERANT_MODE_COUNT:
		*(uint32 *) buf = di->di_NDispModes;
		retval = B_OK;
		break;

	case MAXIOCTL_I740 + B_GET_MODE_LIST:
		memcpy (buf,
			di->di_DispModes,
			di->di_NDispModes * sizeof (display_mode));
		retval = B_OK;
		break;

	case MAXIOCTL_I740 + B_PROPOSE_DISPLAY_MODE:
#define	PDM	((i740_propose_display_mode *) buf)

		retval = propose_video_mode
			  (di, PDM->pdm_Target, PDM->pdm_Lo, PDM->pdm_Hi);
		break;

	case MAXIOCTL_I740 + B_SET_DISPLAY_MODE:
#define	SDM	((i740_set_display_mode *) buf)

		if ((retval = vid_selectmode (di, ci, SDM->sdm_Mode)) == B_OK)
		{
			memcpy (&ci->ci_CurDispMode, SDM->sdm_Mode,
				sizeof (ci->ci_CurDispMode));
		}
		break;

	case MAXIOCTL_I740 + B_GET_FRAME_BUFFER_CONFIG:
#define	FBI	((frame_buffer_config *) buf)

		FBI->frame_buffer =
		 (void *) (ci->ci_FBBase + ci->ci_BaseAddr0);
		FBI->frame_buffer_dma =
		 (void *) (ci->ci_FBBase + ci->ci_BaseAddr0_DMA);
		FBI->bytes_per_row = ci->ci_BytesPerRow;
		retval = B_OK;
		break;

	case MAXIOCTL_I740 + B_GET_PIXEL_CLOCK_LIMITS:
	 {
#define	PCL	((i740_pixel_clock_limits *) buf)
		register uint32	total_pix;

		/*
		 * Constrain low-end to 48 Hz, until a monitors database
		 * shows up...
		 */
		total_pix = PCL->pcl_DisplayMode->timing.h_total * 
			    PCL->pcl_DisplayMode->timing.v_total;
		PCL->pcl_Lo = (total_pix * 48L) / 1000L;
		PCL->pcl_Hi = CLOCK_MAX;
		retval = B_OK;
		break;
	 }

	case MAXIOCTL_I740 + B_SET_INDEXED_COLORS:
	 {
#define SIC	((i740_set_indexed_colors *) buf)	
		register int	count, idx;
		register uint8	*color;

		/*
		 * The I740 can optionally display 16- and 32-bit pixels in
		 * "3DO mode" (long story), which basically requires that a
		 * simple ramp palette be installed when 16- and 32-bit
		 * modes are active.  We therefore deliberately ignore
		 * requests to set palette entries when not in 8-bit mode.
		 */
		if (ci->ci_CurDispMode.space == B_CMAP8) {
			acquire_sem (ci->ci_CLUTLock);

			count = SIC->sic_Count;
			color = SIC->sic_ColorData;
			idx = SIC->sic_First;

			while (count--) {
				setpaletteentry (&ci->ci_Regs->VGARegs,
						 idx++,
						 color[0],
						 color[1],
						 color[2]);
				color += 3;
			}

			release_sem (ci->ci_CLUTLock);
		}
		break;
	 }


	/*  DPMS management  */
	case MAXIOCTL_I740 + B_DPMS_MODE:
		switch (IDXREG_R (&ci->ci_Regs->VGARegs, XR, XR_DPMSSYNCCTL) &
			(FIELDMASK (XR_DPMSSYNCCTL, HSYNCMODE) |
			 FIELDMASK (XR_DPMSSYNCCTL, VSYNCMODE)))
		{
		case FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, NORMAL) |
		     FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, NORMAL):
			*(uint32 *) buf = B_DPMS_ON;
			break;
		case FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, DATA) |
		     FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, NORMAL):
			*(uint32 *) buf = B_DPMS_STAND_BY;
			break;
		case FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, NORMAL) |
		     FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, DATA):
			*(uint32 *) buf = B_DPMS_SUSPEND;
			break;
		case FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, DATA) |
		     FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, DATA):
			*(uint32 *) buf = B_DPMS_OFF;
			break;
		}
		break;

	case MAXIOCTL_I740 + B_SET_DPMS_MODE:
	 {
		register vgaregs	*vregs;
		int			hpol, vpol;
		uint8			dpmsctl;

		vregs = &ci->ci_Regs->VGARegs;
		hpol =
		 (ci->ci_CurDispMode.timing.flags & B_POSITIVE_HSYNC) != 0;
		vpol =
		 (ci->ci_CurDispMode.timing.flags & B_POSITIVE_VSYNC) != 0;
		
		dpmsctl = IDXREG_R (vregs, XR, XR_DPMSSYNCCTL) &
		 	  ~(FIELDMASK (XR_DPMSSYNCCTL, HSYNCMODE) |
			    FIELDMASK (XR_DPMSSYNCCTL, HSYNCDATA) |
			    FIELDMASK (XR_DPMSSYNCCTL, VSYNCMODE) |
			    FIELDMASK (XR_DPMSSYNCCTL, VSYNCDATA));
		dpmsctl |= FIELDVAL (XR_DPMSSYNCCTL, HSYNCDATA, hpol) |
			   FIELDVAL (XR_DPMSSYNCCTL, VSYNCDATA, vpol);

		switch (*(uint32 *) buf) {
		case B_DPMS_ON:		// H:  on, V:  on
			dpmsctl |=
			 FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, NORMAL) |
			 FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, NORMAL);
			screen_on (vregs);
			break;
		case B_DPMS_STAND_BY:	// H: off, V:  on
			dpmsctl |=
			 FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, DATA) |
			 FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, NORMAL);
			screen_off (vregs);
			break;
		case B_DPMS_SUSPEND:	// H:  on, V: off
			dpmsctl |=
			 FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, NORMAL) |
			 FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, DATA);
			screen_off (vregs);
			break;
		case B_DPMS_OFF:	// H: off, V: off
			dpmsctl |=
			 FIELDDEF (XR_DPMSSYNCCTL, HSYNCMODE, DATA) |
			 FIELDDEF (XR_DPMSSYNCCTL, VSYNCMODE, DATA);
			screen_off (vregs);
			break;
		}
		IDXREG_W (vregs, XR, XR_DPMSSYNCCTL, dpmsctl);
		break;
	 }


	/*  Cursor managment  */
	case MAXIOCTL_I740 + B_SET_CURSOR_SHAPE:
#define	SCS	((i740_set_cursor_shape *) buf)

		retval = setcursorshape (di,
					 SCS->scs_XORMask, SCS->scs_ANDMask,
					 SCS->scs_Width, SCS->scs_Height,
					 SCS->scs_HotX, SCS->scs_HotY);
		break;

	case MAXIOCTL_I740 + B_SHOW_CURSOR:
		showcursor (ci, *(bool *) buf);
		retval = B_OK;
		break;

#if 0
	/*
	 * Unimplemented operations; handled either in the accelerant or not
	 * at all.
	 */
	/*  Initialization  */
	case MAXIOCTL_I740 + B_GET_CONFIG_INFO_SIZE:
	case MAXIOCTL_I740 + B_GET_CONFIG_INFO:
	case MAXIOCTL_I740 + B_SET_CONFIG_INFO:

	/*  Mode configuration  */
	case MAXIOCTL_I740 + B_GET_DISPLAY_MODE:
	case MAXIOCTL_I740 + B_MOVE_DISPLAY:
	case MAXIOCTL_I740 + B_SET_GAMMA:

	/*  Cursor managment  */
	case MAXIOCTL_I740 + B_MOVE_CURSOR:

	/*  Synchronization  */
	case MAXIOCTL_I740 + B_ACCELERANT_ENGINE_COUNT:
	case MAXIOCTL_I740 + B_ACQUIRE_ENGINE:
	case MAXIOCTL_I740 + B_RELEASE_ENGINE:
	case MAXIOCTL_I740 + B_WAIT_ENGINE_IDLE:
	case MAXIOCTL_I740 + B_GET_SYNC_TOKEN:
	case MAXIOCTL_I740 + B_SYNC_TO_TOKEN:

	/*  2D acceleration  */
	case MAXIOCTL_I740 + B_SCREEN_TO_SCREEN_BLIT:
	case MAXIOCTL_I740 + B_FILL_RECTANGLE:
	case MAXIOCTL_I740 + B_INVERT_RECTANGLE:
	case MAXIOCTL_I740 + B_FILL_SPAN:
	
	/*  3D acceleration  */
	case MAXIOCTL_I740 + B_ACCELERANT_PRIVATE_START:
#endif

	default:
		break;
	}
	return (retval);
}



/****************************************************************************
 * Debuggery.
 */
#if (DEBUG  ||  FORCE_DEBUG)
static int
i740dump (int argc, char **argv)
{
	int i;
	
	kprintf ("I740 kernel driver\nThere are %d device(s)\n",
		 i740dd->dd_NDevs);
	kprintf ("Driver-wide data: 0x%08lx\n", i740dd);
	kprintf ("Driver-wide benahpore: %d/%d\n",
		 i740dd->dd_DriverLock.b4_Sema4,
		 i740dd->dd_DriverLock.b4_FastLock);
	kprintf ("Total calls to IRQ handler: %ld\n", ncalls);
	kprintf ("Total relevant interrupts: %ld\n", i740dd->dd_NInterrupts);

	for (i = 0;  i < i740dd->dd_NDevs;  i++) {
		devinfo		*di = &i740dd->dd_DI[i];
		i740_card_info	*ci = di->di_I740CardInfo;
		i740_card_ctl	*cc = di->di_I740CardCtl;

		kprintf ("Device %s\n", di->di_Name);
		kprintf ("\tTotal interrupts: %ld\n", di->di_NInterrupts);
		kprintf ("\tVBlanks: %ld\n", di->di_NVBlanks);
		kprintf ("\tLast IRQ mask: 0x%08lx\n", di->di_LastIrqMask);
		if (ci) {
			kprintf ("  i740_card_info: 0x%08lx\n", ci);
			kprintf ("   i740_card_ctl: 0x%08lx\n", cc);
			kprintf ("  Base register 0,1: 0x%08lx, 0x%08lx\n",
				 ci->ci_BaseAddr0, ci->ci_BaseAddr1);
			kprintf ("  IRQFlags: 0x%08lx\n", cc->cc_IRQFlags);
		}
	}
	kprintf ("\n");
	return (TRUE);
}

static int
idxinb (int ac, char **av)
{
	pci_module_info	*pci;
	register int	i, port;
	register uint8	first, last;

	if (ac < 3) {
		kprintf ("usage: %s <ioreg> <firstindex> [<lastindex>]\n",
			 av[0]);
		return (TRUE);
	}

	port = strtol (av[1], NULL, 0);
	if (port <= 0) {
		kprintf ("Bad port number.\n");
		return (TRUE);
	}
	first = strtol (av[2], NULL, 0);
	if (ac == 4)
		last = strtol (av[3], NULL, 0);
	else
		last = first;

	if (get_module (B_PCI_MODULE_NAME, (module_info **) &pci)) {
		kprintf ("Can't open PCI module (?!?).\n");
		return (TRUE);
	}

	while (first <= last) {
		kprintf ("0x%04x.%02x:", port, first);
		for (i = 16;  --i >= 0; ) {
			(pci->write_io_8) (port, first++);
			kprintf (" %02x", (pci->read_io_8) (port + 1));
			if (first > last)
				break;
		}
		kprintf ("\n");
	}
	put_module (B_PCI_MODULE_NAME);
	return (TRUE);
}

static int
idxoutb (int ac, char **av)
{
	pci_module_info	*pci;
	register int	i, port;
	register uint8	idx, value;

	if (ac < 4) {
		kprintf ("usage: %s <ioreg> <index> <value>\n", av[0]);
		return (TRUE);
	}

	port = strtol (av[1], NULL, 0);
	if (port <= 0) {
		kprintf ("Bad port number.\n");
		return (TRUE);
	}
	idx = strtol (av[2], NULL, 0);
	value = strtol (av[3], NULL, 0);

	if (get_module (B_PCI_MODULE_NAME, (module_info **) &pci)) {
		kprintf ("Can't open PCI module (?!?).\n");
		return (TRUE);
	}

	(pci->write_io_8) (port, idx);
	(pci->write_io_8) (port + 1, value);

	put_module (B_PCI_MODULE_NAME);
	return (TRUE);
}
#endif
