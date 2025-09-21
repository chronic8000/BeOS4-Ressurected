/* :ts=8 bk=0
 *
 * init.c:	Additional initialization/teardown code.  Hooks called
 *		directly by the kernel are in drv.c and dev.c.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.06
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/driver_settings.h>
#include <drivers/genpool_module.h>
#include <dinky/bena4.h>
#include <stddef.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/debug.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * Local prototypes.
 */
static void unmap_device(struct devinfo *di);
static status_t probehost(struct devinfo *di, struct pci_info *cl_pci);
static status_t init_locks(struct devinfo *di, team_id team);
static void dispose_locks(struct devinfo *di);


/****************************************************************************
 * Globals.
 */
extern pci_module_info	*gpci_bus;
extern genpool_module	*ggpm;
extern driverdata	*gdd;
extern BPoolInfo	*gfbpi;


/****************************************************************************
 * Map device memory into local address space.
 */
static status_t
map_device (register struct devinfo *di, const char *shared_name)
{
	gfx_card_info	*ci = di->di_GfxCardInfo;
	LynxEMAddrMap	*lam;	/*  Used to keep 'sizeof()' happy  */
	status_t	retval;
	uint32		baseaddr;
	int		len;
	char		name[B_OS_NAME_LENGTH];

	/*  Create a goofy basename for the areas  */
	strcpy (name, shared_name);
	len = strlen (name);

	/*
	 * All LynxEM chip facilites, including integrated framebuffer RAM,
	 * are accessed through a single PCI aperture.  However, we want
	 * different CPU cache settings for FB RAM vs. chip registers.  So
	 * we perform perform separate mappings on different address ranges
	 * within the aperture.  See src/inc/graphics_p/lynemdefs.h for the
	 * address ranges.
	 *
	 * Map framebuffer RAM first.
	 */
	ci->ci_BaseAddr0_DMA = (uint8 *) di->di_PCI.u.h0.base_registers_pci[0];
	baseaddr = di->di_PCI.u.h0.base_registers[0];
	strcpy (name + len, " framebuffer");
	retval =
	 map_physical_memory (name,
			      (void *) (baseaddr +
			                offsetof (LynxEMAddrMap,
			                          lam_FrameBuffer)),
			      sizeof (lam->lam_FrameBuffer),
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
			   (void *) (baseaddr +
			             offsetof (LynxEMAddrMap,
			                       lam_FrameBuffer)),
			   sizeof (lam->lam_FrameBuffer),
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr0);
#endif

	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_BaseAddr0ID = retval;

	/*
	 * Map chip rendering and I/O aliased registers.
	 */
	strcpy (name + len, " draw regs");
	retval =
	 map_physical_memory (name,
			      (void *) (baseaddr +
			                offsetof (LynxEMAddrMap,
			                          lam_DrawRegs)),
			      sizeof (lam->lam_DrawRegs),
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_DPRegs);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_DPRegsID = retval;

	strcpy (name + len, " video regs");
	retval =
	 map_physical_memory (name,
			      (void *) (baseaddr +
			                offsetof (LynxEMAddrMap,
			                          lam_VideoRegs)),
			      sizeof (lam->lam_DrawRegs),
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_VPRegs);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_VPRegsID = retval;

	strcpy (name + len, " VGA regs");
	retval =
	 map_physical_memory (name,
			      (void *) (baseaddr +
			                offsetof (LynxEMAddrMap,
			                          lam_IOAlias)),
			      sizeof (lam->lam_IOAlias),
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_VGARegs);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_VGARegsID = retval;
	

	if (retval < 0) {
err:		unmap_device (di);
		return (retval);
	}

	return (B_OK);
}

static void
unmap_device (struct devinfo *di)
{
	register gfx_card_info *ci = di->di_GfxCardInfo;

dprintf (("+++ Unmapping device %s\n", di->di_Name));
	if (di->di_VGARegsID)
		delete_area (di->di_VGARegsID), di->di_VGARegsID = 0;
	if (di->di_VPRegsID)
		delete_area (di->di_VPRegsID), di->di_VPRegsID = 0;
	if (di->di_DPRegsID)
		delete_area (di->di_DPRegsID), di->di_DPRegsID = 0;
	if (di->di_BaseAddr0ID)
		delete_area (di->di_BaseAddr0ID), di->di_BaseAddr0ID = 0;

	ci->ci_BaseAddr0	=
	ci->ci_BaseAddr0_DMA	= NULL;
	ci->ci_VPRegs		= NULL;
	ci->ci_DPRegs		= NULL;
	ci->ci_VGARegs		= NULL;
}


/****************************************************************************
 * Look for all cards on the PCI bus we can support and create device entries
 * for them.
 */
void
probe_devices (void)
{
	long	pci_index = 0;
	uint32	count = 0;
	uint32	pcicmd;
	devinfo	*di = gdd->dd_DI;

	pcicmd = PCI_command_memory |
#if DEBUG
		 PCI_command_io |
#endif
		 PCI_command_master;

	while ((count < MAXDEVS)  &&
	       ((*gpci_bus->get_nth_pci_info)(pci_index, &(di->di_PCI)) ==
	        B_NO_ERROR))
	{
		if (di->di_PCI.vendor_id == VENDORID_SMI  &&
		    (di->di_PCI.device_id == DEVICEID_LYNXEM  ||
		     di->di_PCI.device_id == DEVICEID_LYNXEMPLUS))
		{
			/*
			 * Found a card we can deal with; create a device
			 * for it.
			 */
			sprintf (di->di_Name,
				 "graphics/lynxem_%02X%02X%02X",
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);
			gdd->dd_DevNames[count] = di->di_Name;
dprintf (("+++ making /dev/%s\n", di->di_Name));

			/*  Initialize to...  uh...  uninitialized.  */
			di->di_BaseAddr0ID		=
			di->di_GfxCardInfo_AreaID	=
			di->di_NInterrupts		=
			di->di_NVBlanks			=
			di->di_LastIrqMask		= 0;

			/*
			 * Enable memory and I/O access.
			 */
			set_pci (&di->di_PCI,
				 PCI_command,
				 sizeof (uint16),
				 get_pci (&di->di_PCI,
				          PCI_command,
				          sizeof (uint16))
				 | pcicmd);

			di++;
			count++;
		}
		pci_index++;
	}
	gdd->dd_NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	gdd->dd_DevNames[count] = NULL;
dprintf (("+++ probe_devices: %d supported devices\n", count));
}


/****************************************************************************
 * Device context creation and teardown.
 */
status_t
firstopen (struct devinfo *di)
{
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;
	physical_entry	entry;
	thread_id	thid;
	thread_info	thinfo;
	status_t	retval = B_OK;
	int		n, len;
	char		shared_name[B_OS_NAME_LENGTH];
	void		*hwstatuspage;

dprintf (("+++ firstopen(0x%08lx)\n", di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "lynxem_%02X%02X%02X",
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (shared_name);

	/*  Read-only globals  */
	strcpy (shared_name + len, " shared (RO)");
	n = (sizeof (struct gfx_card_info) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_GfxCardInfo,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		return (retval);
	di->di_GfxCardInfo_AreaID = retval;

	/*  Read/write globals  */
	strcpy (shared_name + len, " shared (R/W)");
	n = (sizeof (struct gfx_card_ctl) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_GfxCardCtl,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err;	/*  Look down  */
	di->di_GfxCardCtl_AreaID = retval;


	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	memset (ci, 0, sizeof (*ci));
	memset (cc, 0, sizeof (*cc));
	/*  Flush data to RAM  */
//	__asm__ ("wbinvd");

	/*  Initialize {sem,ben}aphores the client will need.  */
	shared_name[len] = '\0';
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (di, thinfo.team)) < 0)
		goto err;	/*  Look down  */

	/*  Map the card's resources into memory.  */
	if ((retval = map_device (di, shared_name)) < 0)
		goto err;	/*  Look down  */

	/*  Create GenPool descriptor  */
	if (!(gfbpi = (ggpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err;	/*  Look down  */
	}

	/*  Initialize the rest of the card.  */
	if ((retval = vid_init (di)) < 0) {
err:		lastclose (di);
		return (retval);
	}

#if 0
	/*  Install interrupt handler  */
	if (install_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
					  lynxem_interrupt,
					  (void *) di,
					  0) == B_OK)
	{
		clearinterrupts (ci);
		enableinterrupts (ci, TRUE);

		/* notify the interrupt handler that this card is a candidate */
		ci->ci_IRQEnabled = TRUE;
	}
#endif

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->ci_DevName, di->di_Name);

	/*  All done, return the status.  */
dprintf (("+++ firstopen returning 0x%08x\n", retval));
	return (retval);
}

void
lastclose (struct devinfo *di)
{
dprintf (("+++ lastclose() begins...\n"));

	if (di->di_GfxCardInfo->ci_IRQEnabled) {
		di->di_GfxCardInfo->ci_IRQEnabled = FALSE;

		enableinterrupts (di->di_GfxCardInfo, FALSE);
		clearinterrupts (di->di_GfxCardInfo);
		remove_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
					     lynxem_interrupt, di);
	}

	/*  FIXME: TELL HW PGTBL IS INVALID!  */
	vid_uninit (di);

	if (gfbpi)	(ggpm->gpm_FreePoolInfo) (gfbpi), gfbpi = NULL;
	unmap_device (di);
	dispose_locks (di);
	if (di->di_GfxCardCtl_AreaID) {
		delete_area (di->di_GfxCardCtl_AreaID);
		di->di_GfxCardCtl_AreaID = 0;
		di->di_GfxCardCtl = NULL;
	}
	if (di->di_GfxCardInfo_AreaID) {
		delete_area (di->di_GfxCardInfo_AreaID);
		di->di_GfxCardInfo_AreaID = 0;
		di->di_GfxCardInfo = NULL;
	}
dprintf (("+++ lastclose() completes.\n"));
}


/****************************************************************************
 * {Sem,Ben}aphore initialization.
 */
static status_t
init_locks (struct devinfo *di, team_id team)
{
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;
	status_t	retval;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;

	/*
	 * Initialize semaphores for the driver.
	 */
	if ((retval = create_sem (1, "LynxEM CRTC lock")) < 0)
		return (retval);
	ci->ci_ModeRegsLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "LynxEM CLUT lock")) < 0)
		return (retval);
	ci->ci_CLUTLock = retval;
	set_sem_owner (retval, team);

	/*  VBlank semaphore gets zero thread count  */
	if ((retval = create_sem (0, "LynxEM VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);

	/*
	 * Accelerant-only locks
	 */
	if ((retval = BInitOwnedBena4 (&cc->cc_EngineLock,
				       "LynxEM rendering engine lock",
				       team)) < 0)
		return (retval);

	return (B_OK);
}

static void
dispose_locks (struct devinfo *di)
{
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;

	BDisposeBena4 (&cc->cc_EngineLock);

	if (ci->ci_VBlankLock)
		delete_sem (ci->ci_VBlankLock), ci->ci_VBlankLock = 0;
	if (ci->ci_CLUTLock)
		delete_sem (ci->ci_CLUTLock), ci->ci_CLUTLock = 0;
	if (ci->ci_ModeRegsLock)
		delete_sem (ci->ci_ModeRegsLock), ci->ci_ModeRegsLock = 0;
}
