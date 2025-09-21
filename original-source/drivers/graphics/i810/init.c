/* :ts=8 bk=0
 *
 * init.c:	Additional initialization/teardown code.  Hooks called
 *		directly by the kernel are in drv.c and dev.c.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/driver_settings.h>
#include <drivers/genpool_module.h>
#include <dinky/bena4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>

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


/*  Tables to map DRAMPOP PCI register values to RAM amounts.  */
static uint16	ramsizemap_i810[] = {	/*  In megabytes  */
	0,	8,	0,	16,
	16,	24,	32,	32,
	48,	64,	64,	96,
	128,	128,	192,	256
};

static uint16	ramsizemap_i815[] = {	/*  Still in megabytes  */
	0,	32,	32,	48,
	64,	64,	96,	128,
	0,	128,	128,	192,
	256,	256,	256,	512
};
	

typedef struct bridgemap {
	uint16	bm_GfxPCIID;
	uint16	bm_BridgePCIID;
	uint16	*bm_RamSizeMap;
} bridgemap;

static bridgemap	bridgemaps[] = {
	{ DEVICEID_I810,	0x7120,	ramsizemap_i810 },
	{ DEVICEID_I810_DC100,	0x7122, ramsizemap_i810 },
	{ DEVICEID_I810E,	0x7124, ramsizemap_i810 },
	{ DEVICEID_I815_FSB100,	0x1110,	ramsizemap_i815 },
	{ DEVICEID_I815_NOAGP,	0x1110,	ramsizemap_i815 },
	{ DEVICEID_I815,	0x1130,	ramsizemap_i815 }
};


/****************************************************************************
 * Map device memory into local address space.
 */
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
	 * Map "framebuffer RAM" (really the PCI aperture to graphics-mapped
	 * system RAM).
	 */
	ci->ci_BaseAddr0_DMA = di->di_PCI.u.h0.base_registers_pci[0];
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
		goto err;	/*  Look down  */
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
		goto err;	/*  Look down  */
	di->di_BaseAddr1ID = retval;
	
	if ((retval = probehost (di, &di->di_PCI)) < 0)
		goto err;	/*  Look down  */

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
	di->di_CursorBase_DMA	=
	di->di_TakenBase_DMA	=
	di->di_TakenSize	= 0;

	ci->ci_BaseAddr0_DMA	= 0;
	ci->ci_BaseAddr0	=
	ci->ci_BaseAddr1	= NULL;
	ci->ci_Regs		= NULL;
}



static status_t
probehost (struct devinfo *di, struct pci_info *cl_pci)
{
	pci_info	pcii;
	uint32		ramsize;
	int32		idx;
	uint16		hostdevid;
	uint16		*ramsizemap;
	uint8		reg;

	for (idx = NUMOF (bridgemaps);  --idx >= 0; ) {
		if (bridgemaps[idx].bm_GfxPCIID == cl_pci->device_id) {
			hostdevid = bridgemaps[idx].bm_BridgePCIID;
			ramsizemap = bridgemaps[idx].bm_RamSizeMap;
			break;
		}
	}
	if (idx < 0) {
		dprintf (("+++ ERROR: Can't find bridge PCI ID for 0x%04x\n",
		          cl_pci->device_id));
		return (B_ERROR);
	}

	for (idx = 0;
	     (*gpci_bus->get_nth_pci_info)(idx, &pcii) == B_NO_ERROR;
	     idx++)
	{
		if (pcii.vendor_id == cl_pci->vendor_id  &&
		    pcii.device_id == hostdevid)
		{
			uint32	tsegsize;

			/*
			 * Compute size of graphics region "taken" from
			 * system RAM.
			 */
			di->di_TakenSize = 0;
			reg = get_pci (&pcii, PCI_I810_SMRAM, 1) &
			      MASKFIELD (PCI_I810_SMRAM, GFXMODE);
			switch (reg) {
			case DEF2FIELD (PCI_I810_SMRAM, GFXMODE, ENABLE_1024K):
				di->di_TakenSize += 512 << 10;
			case DEF2FIELD (PCI_I810_SMRAM, GFXMODE, ENABLE_512K):
				di->di_TakenSize += 512 << 10;
			case DEF2FIELD (PCI_I810_SMRAM, GFXMODE, ENABLE_0K):
			case DEF2FIELD (PCI_I810_SMRAM, GFXMODE, DISABLE):
				break;
			}
			
			/*
			 * The 'TSEG' gets first crack at taking memory, and
			 * graphics gets located below that.  Determine size
			 * of TSEG.
			 */
			tsegsize = 0;
			switch (reg) {
			case DEF2FIELD (PCI_I810_SMRAM, TSEG, ENABLE_1024K):
				tsegsize += 512 << 10;
			case DEF2FIELD (PCI_I810_SMRAM, TSEG, ENABLE_512K):
				tsegsize += 512 << 10;
			case DEF2FIELD (PCI_I810_SMRAM, TSEG, ENABLE_0K):
			case DEF2FIELD (PCI_I810_SMRAM, TSEG, DISABLE):
				break;
			}

			/*
			 * Determine RAM size so we can locate "taken" memory.
			 */
			reg = get_pci (&pcii, PCI_I810_DRAMPOP, sizeof (uint8));
			ramsize = ramsizemap[FIELD2VAL (PCI_I810_DRAMPOP,
			                                DIMM0, reg)];
			ramsize += ramsizemap[FIELD2VAL (PCI_I810_DRAMPOP,
			                                 DIMM1, reg)];
			/*  Mildly cheesy hack.  */
			if (ramsizemap == ramsizemap_i815) {
				reg = get_pci (&pcii,
				               PCI_I815_DRAMPOP2,
				               sizeof (uint8));
				ramsize +=
				 ramsizemap[FIELD2VAL (PCI_I815_DRAMPOP2,
				                       DIMM2, reg)];
			}
			ramsize <<= 20;
			
			/*
			 * Establish location of "taken" memory.
			 */
			di->di_TakenBase_DMA =
			 ramsize - tsegsize - di->di_TakenSize;
dprintf (("+++ RAM sizings: Total = 0x%08lx, tseg = 0x%08lx, gfx = 0x%08lx\n",
 ramsize, tsegsize, di->di_TakenSize));
dprintf (("+++ di_TakenBase = 0x%08lx\n", di->di_TakenBase_DMA));

			/*
			 * Determine front-side bus frequency.  (Sorry about
			 * the ugliness here...)
			 */
			di->di_FSBFreq =
			 ((get_pci (&pcii, PCI_I810_GMCHCFG, 1) &
			   MASKFIELD (PCI_I810_GMCHCFG, FSBFREQ)) ==
			  DEF2FIELD (PCI_I810_GMCHCFG, FSBFREQ, 133MHZ))  ?
			 133  :  100;

			/*
			 * Set MISCC2 to the value the docs say it's
			 * supposed to have.
			 */
			reg = get_pci (&pcii, PCI_I810_MISCC2, sizeof (uint8));
			set_pci (&pcii,
				 PCI_I810_MISCC2,
				 sizeof (uint8),
				 reg | VAL2FIELD (PCI_I810_MISCC2,
						  SETTOONE, 1)
				     | VAL2FIELD (PCI_I810_MISCC2,
						  TEXTIMMEDIATEBLIT, 1)
				     | VAL2FIELD (PCI_I810_MISCC2,
						  PALETTELOADSEL, 1)
				     | VAL2FIELD (PCI_I810_MISCC2,
						  INSTPARSERCLOCK, 1));
			break;
		}
	}
	return (B_OK);
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
		if (di->di_PCI.vendor_id == VENDORID_INTEL  &&
		    (di->di_PCI.device_id == DEVICEID_I810  ||
		     di->di_PCI.device_id == DEVICEID_I810_DC100  ||
		     di->di_PCI.device_id == DEVICEID_I810E  ||
		     di->di_PCI.device_id == DEVICEID_I815_FSB100  ||
		     di->di_PCI.device_id == DEVICEID_I815_NOAGP  ||
		     di->di_PCI.device_id == DEVICEID_I815))
		{
			/*
			 * Found a card we can deal with; create a device
			 * for it.
			 */
			sprintf (di->di_Name,
				 "graphics/%04X_%04X_%02X%02X%02X",
				 di->di_PCI.vendor_id, di->di_PCI.device_id,
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);
			gdd->dd_DevNames[count] = di->di_Name;
dprintf (("+++ making /dev/%s\n", di->di_Name));

			/*  Initialize to...  uh...  uninitialized.  */
			di->di_BaseAddr0ID		=
			di->di_BaseAddr1ID		=
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
dprintf (("+++ probe_devices: %ld supported devices\n", count));
}


/****************************************************************************
 * Determine and reserve any extra system RAM the user wants for graphics
 * operations.
 *
 * The settings file is named "i810".  The parameters are:
 *
 * gfxmemsize <size>
 * reclaimtakenmem [on|off]
 *
 * 'gfxmemsize' expresses the *total* amount of graphics memory desired;
 * anything over the amount "taken" by the chip at boot time will be
 * allocated here.  'size' can have the suffix 'K' or 'M' to more
 * conveniently specify amounts of kilobytes or megabytes, respectively.
 *
 * 'reclaimtakenmem' is a boolean that tells the driver to attempt to
 * map the 'taken' memory into the GART, making it available for graphics
 * use (default TRUE).  There is evidence to suggest that some motherboards
 * don't like this, or that I bunged up the implementation, leading to
 * lockups.  So this is available to disable that bit of cleverness.
 */

static status_t
getextramem (struct devinfo *di, const char *shared_name)
{
	gfx_card_info	*ci;
	int32		retval;
	int		size, shift, reclaimtaken;
	const char	*sizestr;
	char		name[B_OS_NAME_LENGTH];
	void		*settings;

	ci = di->di_GfxCardInfo;

	/*
	 * Load driver settings file; see if we're to obtain extra RAM for
	 * the framebuffer.
	 */
	settings = load_driver_settings ("i810");
	sizestr = get_driver_parameter (settings, "gfxmemsize", "0", "0");
	reclaimtaken = get_driver_boolean_parameter
	                (settings, "reclaimtakenmem", TRUE, TRUE);

	size = strlen (sizestr);
	switch (sizestr[size-1]) {
	case 'm':
	case 'M':
		shift = 20;
		break;
	case 'k':
	case 'K':
		shift = 10;
		break;
	default:
		shift = 0;
		break;
	}

	size = strtol (sizestr, NULL, 0) << shift;

	unload_driver_settings (settings);	// 'sizestr' now invalid.
	
	if (!reclaimtaken)
		di->di_TakenSize = 0;

	if ((unsigned) size <= di->di_TakenSize) {
		/*  Do not obtain any additional RAM.  */
		di->di_ExtraSize = 0;
		di->di_ExtraBase_AreaID = 0;
		di->di_ExtraBase = NULL;
		return (B_OK);
	}
	di->di_ExtraSize = (size - di->di_TakenSize + B_PAGE_SIZE - 1) &
			   ~(B_PAGE_SIZE - 1);

	strcpy (name, shared_name);
	strcat (name, " extra FB");
	if ((retval = create_area (name,
				   (void **) &di->di_ExtraBase,
				   B_ANY_KERNEL_ADDRESS,
				   di->di_ExtraSize,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		return (retval);
	di->di_ExtraBase_AreaID = retval;
	
	return (B_OK);
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
	void		*cursorbase;

dprintf (("+++ firstopen(0x%08lx)\n", (uint32) di));

	/*
	 * Create public globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
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
	
	/*  Cursor (it's fetched out of system RAM separately)  */
	/*  Also 4K extra room for Overlay register source data  */
	strcpy (shared_name + len, " cursor");
	if ((retval = create_area (shared_name,
				   &cursorbase,
				   B_ANY_KERNEL_ADDRESS,
				   SIZEOF_CURSORPAGE + (4 << 10),
				   B_CONTIGUOUS,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err;	/*  Look down  */
	di->di_CursorBase_AreaID = retval;

	/*  Create uncached mapping for cursor image.  (Uh, Cyril...)  */
	get_memory_map (cursorbase, SIZEOF_CURSORPAGE + (4 << 10), &entry, 1);
	strcpy (shared_name + len, " cursor (UC)");
	retval = map_physical_memory (shared_name,
				      (void *) entry.address,
				      SIZEOF_CURSORPAGE + (4 << 10),
				      B_ANY_KERNEL_ADDRESS,
				      B_READ_AREA | B_WRITE_AREA,
				      &cursorbase);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_CursorBaseUCID = retval;
	
	/*  GTT (must be 4K aligned)  */
	strcpy (shared_name + len, " GTT");
	di->di_GTTSize = di->di_PCI.u.h0.base_register_sizes[0] / B_PAGE_SIZE;
	n = (di->di_GTTSize * sizeof (uint32) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_GTT,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_CONTIGUOUS,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err;	/*  Look down  */
	di->di_GTT_AreaID = retval;
	
	/*  HW Status Page (must be 4K aligned)  */
	strcpy (shared_name + len, " HW stat");
	if ((retval = create_area (shared_name,
				   &hwstatuspage,
				   B_ANY_KERNEL_ADDRESS,
				   4 << 10,	/*  Always 4K  */
				   B_CONTIGUOUS,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err;	/*  Look down  */
	di->di_HWStatusPage_AreaID = retval;
	
	/*  Create uncached mapping for HW Status Page.  */
	get_memory_map (hwstatuspage, 4 << 10, &entry, 1);
	di->di_HWStatusPage_DMA = (uint32) entry.address;
	strcpy (shared_name + len, " HW stat (UC)");
	retval = map_physical_memory (shared_name,
				      (void *) di->di_HWStatusPage_DMA,
				      4 << 10,
				      B_ANY_KERNEL_ADDRESS,
				      B_READ_AREA | B_WRITE_AREA,
				      &hwstatuspage);
	if (retval < 0)
		goto err;	/*  Look down  */
	di->di_HWStatusPageUCID = retval;
	

	ci = di->di_GfxCardInfo;
	cc = di->di_GfxCardCtl;
	memset (ci, 0, sizeof (*ci));
	memset (cc, 0, sizeof (*cc));
//	memset (di->di_GTT, 0, di->di_GTTSize * sizeof (uint32));
	memset (hwstatuspage, 0, 4 << 10);
	ci->ci_HWStatusPage = hwstatuspage;
	ci->ci_CursorBase = cursorbase;

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

	/*  Determine if optional cache RAM is present.  */
	if (((I810Regs *) ci->ci_BaseAddr1)->DRT &
	    DEF2FIELD (DRT, LOCALCACHE, 4M))
		di->di_CacheSize = 4 << 20;
	else
		di->di_CacheSize = 0;
	
	/*  Procure user-specified extra system RAM.  */
	if ((retval = getextramem (di, shared_name)) < 0)
		goto err;	/*  Look down  */

	InitList (&ci->ci_OpenerList);

	/*  Initialize the rest of the card.  */
	if ((retval = vid_init (di)) < 0) {
err:		lastclose (di);
		return (retval);
	}
	initOverlay (di);

	/*  Install interrupt handler  */
	if (install_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
					  i810_interrupt,
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
					     i810_interrupt, di);
	}

	/*  FIXME: TELL HW PGTBL IS INVALID!  */
	vid_uninit (di);

	if (gfbpi)	(ggpm->gpm_FreePoolInfo) (gfbpi), gfbpi = NULL;
	unmap_device (di);
	dispose_locks (di);
	if (di->di_ExtraBase_AreaID) {
		delete_area (di->di_ExtraBase_AreaID);
		di->di_ExtraBase_AreaID = 0;
		di->di_ExtraSize = 0;
	}
	if (di->di_HWStatusPageUCID) {
		delete_area (di->di_HWStatusPageUCID);
		di->di_HWStatusPageUCID = 0;
	}
	if (di->di_HWStatusPage_AreaID) {
		delete_area (di->di_HWStatusPage_AreaID);
		di->di_HWStatusPage_AreaID = 0;
		di->di_HWStatusPage_DMA = 0;
	}
	if (di->di_GTT_AreaID) {
		delete_area (di->di_GTT_AreaID);
		di->di_GTT_AreaID = 0;
		di->di_GTT = NULL;
	}
	if (di->di_CursorBaseUCID) {
		delete_area (di->di_CursorBaseUCID);
		di->di_CursorBaseUCID = 0;
	}
	if (di->di_CursorBase_AreaID) {
		delete_area (di->di_CursorBase_AreaID);
		di->di_CursorBase_AreaID = 0;
		di->di_CursorBase_DMA = 0;
	}
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
	if ((retval = create_sem (1, "I810 disp config lock")) < 0)
		return (retval);
	ci->ci_DispConfigLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "I810 disp buffer lock")) < 0)
		return (retval);
	ci->ci_DispBufLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "I810 VGA regs lock")) < 0)
		return (retval);
	ci->ci_VGARegsLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "I810 overlay data")) < 0)
		return (retval);
	ci->ci_OverlayLock = retval;
	set_sem_owner (retval, team);

	if ((retval = create_sem (1, "I810 shared registers")) < 0)
		return (retval);
	ci->ci_OtherRegsLock = retval;
	set_sem_owner (retval, team);

	/*  VBlank semaphore gets zero thread count  */
	if ((retval = create_sem (0, "I810 VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);

	/*
	 * Accelerant-only locks
	 */
	if ((retval = BInitOwnedBena4 (&cc->cc_EngineLock,
				       "I810 rendering engine lock",
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
	if (ci->ci_OtherRegsLock)
		delete_sem (ci->ci_OtherRegsLock), ci->ci_OtherRegsLock = 0;
	if (ci->ci_OverlayLock)
		delete_sem (ci->ci_OverlayLock), ci->ci_OverlayLock = 0;
	if (ci->ci_VGARegsLock)
		delete_sem (ci->ci_VGARegsLock), ci->ci_VGARegsLock = 0;
	if (ci->ci_DispBufLock)
		delete_sem (ci->ci_DispBufLock), ci->ci_DispBufLock = 0;
	if (ci->ci_DispConfigLock)
		delete_sem (ci->ci_DispConfigLock), ci->ci_DispConfigLock = 0;
}
