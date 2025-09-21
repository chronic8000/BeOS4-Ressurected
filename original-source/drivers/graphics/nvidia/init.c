/* :ts=8 bk=0
 *
 * init.c:	Additional initialization/teardown code.  Hooks called
 *		directly by the kernel are in drv.c and dev.c.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.08.22
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/genpool_module.h>
#include <dinky/bena4.h>

#include <stdio.h>

#include <graphics_p/nvidia/nvidia.h>
#include <graphics_p/nvidia/debug.h>

#include "driver.h"
#include "protos.h"


/****************************************************************************
 * Local prototypes.
 */
static void unmap_device(struct devinfo *di);
static status_t init_locks(struct devinfo *di, team_id team);
static void dispose_locks(struct devinfo *di);


/****************************************************************************
 * Globals.
 */
extern pci_module_info	*gpci_bus;
extern genpool_module	*ggpm;
extern driverdata	*gdd;
extern BPoolInfo	*gfbpi;

static const char	nv3[] = "nv3";
static const char	nv4[] = "nv4";
static const char	nv10[] = "nv10";
static const char	nv20[] = "nv20";

static const char	riva128[] = "RIVA 128";
static const char	rivatnt[] = "RIVA TNT";
static const char	rivatnt2[] = "RIVA TNT2";
static const char	geforce[] = "GeForce";
static const char	geforce2[] = "GeForce2";
static const char	geforce3[] = "GeForce3";

static const supportedcard	supportedcards[] = {
 {	VENDORID_NVIDIA_SGS,	DEVICEID_RIVA128,	NV3Setup,	nv3,
	"SGS-Thompson RIVA 128",	riva128
 },
 {	VENDORID_NVIDIA,	DEVICEID_RIVA128,	NV3Setup,	nv3,
	"NVidia RIVA 128",		riva128
 },
 {	VENDORID_NVIDIA,	DEVICEID_TNT,		NV4Setup,	nv4,
	"NVidia RIVA TNT",		rivatnt
 },
 {	VENDORID_NVIDIA,	DEVICEID_TNT2,		NV4Setup,	nv4,
	"NVidia RIVA TNT2",		rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_UTNT2,		NV4Setup,	nv4,
	"NVidia RIVA TNT2 Ultrq",	rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_VTNT2,		NV4Setup,	nv4,
	"NVidia RIVA TNT2 Vanta",	rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_UVTNT2,	NV4Setup,	nv4,
	"NVidia RIVA TNT2 M64",		rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_TNT2_A,	NV4Setup,	nv4,
	"NVidia RIVA TNT2 (A)",		rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_TNT2_B,	NV4Setup,	nv4,
	"NVidia RIVA TNT2 (B)",		rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_ITNT2,		NV4Setup,	nv4,
	"NVidia RIVA TNT2 Aladdin",	rivatnt2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE256,	NV10Setup,	nv10,
	"NVidia GeForce 256",		geforce
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCEDDR,	NV10Setup,	nv10,
	"NVidia GeForce 256 DDR",	geforce
 },
 {	VENDORID_NVIDIA,	DEVICEID_QUADRO,	NV10Setup,	nv10,
	"NVidia Quadro",		geforce
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2MX,	NV10Setup,	nv10,
	"NVidia GeForce2 MX",		geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2MX2,	NV10Setup,	nv10,
	"NVidia GeForce2 MX",		geforce2
 },
// {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2GO,	NV10Setup,	nv10,
//	"NVidia GeForce2Go",		geforce2
// },
 {	VENDORID_NVIDIA,	DEVICEID_QUADRO2MXR,	NV10Setup,	nv10,
	"NVidia Quadro2 MXR",		geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2GTS,	NV10Setup,	nv10,
	"NVidia GeForce2 GTS",		geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2GTS2,	NV10Setup,	nv10,
	"NVidia GeForce2 GTS",		geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE2ULTRA,	NV10Setup,	nv10,
	"NVidia GeForce2 Ultra",	geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_QUADRO2PRO,	NV10Setup,	nv10,
	"NVidia Quadro2 Pro",		geforce2
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE3,	NV20Setup,	nv20,
	"NVidia GeForce3",		geforce3
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE3_1,	NV20Setup,	nv20,
	"NVidia GeForce3 (rev 1)",	geforce3
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE3_2,	NV20Setup,	nv20,
	"NVidia GeForce3 (rev 2)",	geforce3
 },
 {	VENDORID_NVIDIA,	DEVICEID_GEFORCE3_3,	NV20Setup,	nv20,
	"NVidia GeForce3 (rev 3)",	geforce3
 }
};
#define	NSUPPORTEDCARDS	(sizeof (supportedcards) / sizeof (supportedcard))


/****************************************************************************
 * Map device memory into local address space.
 */
const struct supportedcard *
findsupportedcard (register struct pci_info *pci)
{
	register int	i;
	
	for (i = 0;  i < (int)NSUPPORTEDCARDS;  i++)
		if (pci->vendor_id == supportedcards[i].sc_VendorID  &&
		    pci->device_id == supportedcards[i].sc_DeviceID)
			return (&supportedcards[i]);

	return (NULL);
}


static status_t
map_device (register struct devinfo *di, const char *shared_name)
{
	gfx_card_info	*ci = di->di_GfxCardInfo;
	status_t	retval;
	int		len;
	char		name[B_OS_NAME_LENGTH];

	/*  Create a goofy basename for the areas  */
	strcpy (name, shared_name);
	len = strlen (name);

	/*
	 * Map chip registers.
	 */
	strcpy (name + len, " regs");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[0],
			      di->di_PCI.u.h0.base_register_sizes[0],
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr0);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_BaseAddr0ID = retval;

	/*
	 * Map "framebuffer RAM"
	 */
	ci->ci_BaseAddr1_DMA = (uint8 *) di->di_PCI.u.h0.base_registers_pci[1];
	strcpy (name + len, " framebuffer");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers[1],
			      di->di_PCI.u.h0.base_register_sizes[1],
#if defined(__INTEL__)
			      B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
#else
			      B_ANY_KERNEL_BLOCK_ADDRESS,
#endif
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr1);
#if defined(__INTEL__)
	if (retval < 0)
		/*  *%$#@!! AMD K6 doesn't do write-combining; try again.  */
		retval = map_physical_memory
			  (name,
			   (void *) di->di_PCI.u.h0.base_registers[1],
			   di->di_PCI.u.h0.base_register_sizes[1],
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr1);
#endif

	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_BaseAddr1ID = retval;

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
	if (di->di_BaseAddr1ID)
		delete_area (di->di_BaseAddr1ID), di->di_BaseAddr1ID = 0;
	if (di->di_BaseAddr0ID)
		delete_area (di->di_BaseAddr0ID), di->di_BaseAddr0ID = 0;

	ci->ci_BaseAddr0	=
	ci->ci_BaseAddr1	=
	ci->ci_BaseAddr1_DMA	= NULL;
	ci->ci_FBBase		= 0;
}


/****************************************************************************
 * Look for all cards on the PCI bus we can support and create device entries
 * for them.
 */
void
probe_devices (void)
{
	const supportedcard	*sc;
	devinfo			*di = gdd->dd_DI;
	long			pci_index = 0;
	uint32			count = 0;
	uint32			pcicmd;

	pcicmd = PCI_command_memory |
#if (DEBUG)
		 PCI_command_io |
#endif
		 PCI_command_master;

	while ((count < MAXDEVS)  &&
	       ((*gpci_bus->get_nth_pci_info)(pci_index, &(di->di_PCI)) ==
	        B_NO_ERROR))
	{
		if ((sc = findsupportedcard (&di->di_PCI))) {
			/*
			 * Found a card we can deal with; create a device
			 * for it.
			 */
			sprintf (di->di_Name,
				 "graphics/%s_%02X%02X%02X",
				 sc->sc_DevEntry,
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);
			gdd->dd_DevNames[count] = di->di_Name;
dprintf (("+++ making /dev/%s\n", di->di_Name));

			/*  Initialize to...  uh...  uninitialized.  */
			di->di_BaseAddr0ID		=
			di->di_BaseAddr1ID		=
			di->di_GfxCardInfo_AreaID	= 0;
			di->di_NInterrupts		=
			di->di_NVBlanks			=
			di->di_LastIrqMask		= 0;
			di->di_SC			= sc;

			/*
			 * Enable memory and I/O access.
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
	gdd->dd_NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	gdd->dd_DevNames[count] = NULL;
dprintf (("+++ probe_devices: %ld supported devices\n", count));
}


/****************************************************************************
 * Device context creation and teardown.
 */
status_t
firstopen (struct devinfo *di)
{
	gfx_card_info	*ci;
	gfx_card_ctl	*cc;
	thread_id	thid;
	thread_info	thinfo;
	status_t	retval = B_OK;
	int		n, len;
	char		shared_name[B_OS_NAME_LENGTH];

dprintf (("+++ firstopen(0x%08lx)\n", (uint32)di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%s_%02X%02X%02X",
		 di->di_SC->sc_DevEntry,
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

	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (di, thinfo.team)) < 0)
		goto err;	/*  Look down  */

	/*  Map the card's resources into memory.  */
	shared_name[len] = '\0';
	if ((retval = map_device (di, shared_name)) < 0)
		goto err;	/*  Look down  */

	/*  Initialize platform-independent RIVA_HW_INST stuff.  */
	(di->di_SC->sc_Init) (di);

	/*  Create Pool descriptor (which doesn't get used much)  */
	if (!(gfbpi = (ggpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err;	/*  Look down  */
	}
	
	/*  Initialize the rest of the card.  */
	if ((retval = vid_init (di)) < 0) {
err:		lastclose (di);
		return (retval);
	}

	/*  Install interrupt handler  */
	if (install_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
					  nvidia_interrupt,
					  (void *) di,
					  0) == B_OK)
	{
		clearinterrupts (ci);
		enableinterrupts (ci, TRUE);

		/* notify the interrupt handler that this card is a candidate */
		ci->ci_IRQEnabled = TRUE;
	}

	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->ci_DevName, di->di_Name);

	/*  All done, return the status.  */
dprintf (("+++ firstopen returning 0x%08lx\n", retval));
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
					     nvidia_interrupt, di);
	}

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
	if ((retval = create_sem (1, "NVidia CRTC lock")) < 0)
		return (retval);
	ci->ci_ModeRegsLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "NVidia CLUT lock")) < 0)
		return (retval);
	ci->ci_CLUTLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "NVidia overlay lock")) < 0)
		return (retval);
	ci->ci_OverlayLock = retval;
	set_sem_owner (retval, team);

	/*  VBlank semaphore gets zero thread count  */
	if ((retval = create_sem (0, "NVidia VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);

	/*
	 * Accelerant-only locks
	 */
	if ((retval = BInitOwnedBena4 (&cc->cc_EngineLock,
				       "NVidia rendering engine lock",
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
	if (ci->ci_OverlayLock)
		delete_sem (ci->ci_OverlayLock), ci->ci_OverlayLock = 0;
	if (ci->ci_CLUTLock)
		delete_sem (ci->ci_CLUTLock), ci->ci_CLUTLock = 0;
	if (ci->ci_ModeRegsLock)
		delete_sem (ci->ci_ModeRegsLock), ci->ci_ModeRegsLock = 0;
}
