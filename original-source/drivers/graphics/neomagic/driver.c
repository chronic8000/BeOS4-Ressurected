/* ++++++++++
	driver.c
e
This is a small driver to return information on the PCI bus and allow
an app to find memory mappings for devices.

+++++ */

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/debug.h>
#include <graphics_p/neomagic/neomagic_ioctls.h>

#include <stdio.h>

#include "driver.h"

/*------------------------------------------------------------------------*/

#define NUM_OPENS			10			// Max. number concurrent opens for device
#define GET_NTH_PCI_INFO	(*pci_bus->get_nth_pci_info)

typedef struct 
{
	uint32			id;					/* card number  */
	uint32			deviceid;
} priv_data;

typedef struct
{
	devinfo			*di;
} per_open_data;

_EXPORT status_t init_hardware(void);
_EXPORT status_t init_driver(void);
_EXPORT void uninit_driver(void);
_EXPORT const char ** publish_devices(void);
_EXPORT device_hooks * find_device(const char *);

static long nm_open (const char* name, ulong flags, void** cookie);
static long nm_close (void* cookie);
static long nm_free(void* cookie);
static long nm_control (void* cookie, ulong msg, void* buf, size_t len);
static long nm_read (void* cookie, off_t pos, void* buf, size_t* nbytes);
static long nm_write (void* cookie, off_t pos, const void* buf, size_t* nbytes);

static status_t read_vgareg(uint32 reg, uint8 *value);
static status_t write_vgareg(uint32 reg, uint8 value);

static int neomagicdump (int argc, char **argv);
static int idxinb (int ac, char **av);
static int idxoutb (int ac, char **av);

pci_module_info *pci_bus;
//static char pci_name[] = B_PCI_MODULE_NAME;

driverdata		*neomagicdd;

static spinlock		splok;
static uint32		ncalls;
uint32 io_base_offset;

static device_hooks neomagic_device_hooks = 
{
	nm_open,
	nm_close,
	nm_free,
	nm_control,
	nm_read,
	nm_write
};

/****************************************************************************
 * {Sem,Ben}aphore initialization.
 */
static status_t
init_locks (register struct neomagic_card_info *ci, team_id team)
{
	status_t	retval;

	/*  Initialize {sem,ben}aphores the client will need.  */
	if ((retval = initOwnedBena4 (&ci->ci_CRTCLock,
				      "Neomagic CRTC lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_CLUTLock,
				      "Neomagic CLUT lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_EngineLock,
				      "Neomagic rendering engine lock",
				      team)) < 0)
		return (retval);

	if ((retval = create_sem (0, "Neomagic VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);

	return (B_OK);
}

static void
dispose_locks (register struct neomagic_card_info *ci)
{
	if (ci->ci_VBlankLock >= 0)
	{
		ci->ci_VBlankLock = -1;
	}
	disposeBena4 (&ci->ci_EngineLock);
	disposeBena4 (&ci->ci_CLUTLock);
	disposeBena4 (&ci->ci_CRTCLock);
}


/****************************************************************************
 * Map device memory into local address space.
 */
static status_t
map_device (register struct devinfo *di)
{
	neomagic_card_info	*ci = di->di_neomagicCardInfo;
	status_t	retval;
	int		len, i;
	char		name[B_OS_NAME_LENGTH];


//	dprintf(("neomagic: map_device - ENTER\n"));
	/*  Create a goofy basename for the areas  */
	sprintf (name, "%04X_%04X_%02X%02X%02X",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	len = strlen (name);

	/*
	 * Map framebuffer RAM
	 */
//	dprintf(("neomagic: map_device - Mapping Frame Buffer, base = 0x%x, size = 0x%x\n", di->di_PCI.u.h0.base_registers_pci[1],  di->di_PCI.u.h0.base_register_sizes[1]));
	strcpy (name + len, " framebuffer");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers_pci[0],
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
			   (void *) di->di_PCI.u.h0.base_registers_pci[1],
			   di->di_PCI.u.h0.base_register_sizes[1],
			   B_ANY_KERNEL_BLOCK_ADDRESS,
			   B_READ_AREA | B_WRITE_AREA,
			   (void **) &ci->ci_BaseAddr1);
#endif
	if (retval < 0)
		goto err_base0;	/*  Look down  */
	di->di_BaseAddr0ID = retval;
//	dprintf(("neomagic: map_device - Mapping Frame Buffer, di->di_BaseAddr1ID = 0x%x\n", di->di_BaseAddr1ID));
	
	/*
	 * Map chip registers.
	 */
//	dprintf(("neomagic: map_device - Mapping 2D Registers, base = 0x%x, size = 0x%x\n", di->di_PCI.u.h0.base_registers_pci[0],  di->di_PCI.u.h0.base_register_sizes[0]));
	strcpy (name + len, " regs");
	retval =
	 map_physical_memory (name,
			      (void *) di->di_PCI.u.h0.base_registers_pci[1],
			      di->di_PCI.u.h0.base_register_sizes[1],
			      B_ANY_KERNEL_ADDRESS,
			      B_READ_AREA | B_WRITE_AREA,
			      (void **) &ci->ci_BaseAddr1);

	if (retval < 0)
		goto err_base1;	/*  Look down  */

	di->di_BaseAddr1ID = retval;
//	dprintf(("neomagic: map_device - Mapping 2D Registers, di->di_BaseAddr0ID = 0x%x\n", di->di_BaseAddr0ID));

	if (retval < 0) {
		delete_area (di->di_BaseAddr1ID);
		di->di_BaseAddr1ID = -1;
		ci->ci_BaseAddr1 = NULL;
err_base1:
		delete_area (di->di_BaseAddr0ID);
		di->di_BaseAddr0ID = -1;
		ci->ci_BaseAddr0 = NULL;
err_base0:
	dprintf(("neomagic: map_device - Error Mapping Device returning 0x%x\n", retval));
	return (retval);
	}

	// Store the PCI register values
	for (i=0; i <= 2; i++)
	{
		ci->ci_base_registers_pci[i] = (uint8*) di->di_PCI.u.h0.base_registers_pci[i];
	}
//	dprintf(("neomagic: map_device - EXIT\n"));
	return (B_OK);
}

static void
unmap_device (struct devinfo *di)
{
	register neomagic_card_info *ci = di->di_neomagicCardInfo; 

//dprintf (("neomagic: unmap_device - ENTER di->di_name =  %s\n", di->di_Name));

	if (di->di_int_check_timer.hook == 0)
	{
		cancel_timer(&(di->di_int_check_timer));
		di->di_int_check_timer.hook = 0;
	}
	
	if (di->di_BaseAddr1ID >= 0)
	{
		delete_area (di->di_BaseAddr1ID);
		di->di_BaseAddr1ID = -1;
	}
	if (di->di_BaseAddr0ID >= 0)
	{
		delete_area (di->di_BaseAddr0ID);
		di->di_BaseAddr0ID = -1;
	}
	ci->ci_BaseAddr0	=
	ci->ci_BaseAddr0_DMA	=
	ci->ci_BaseAddr1	= NULL;
	ci->ci_FBBase		=
	ci->ci_FBBase_DMA	= NULL;
//dprintf (("neomagic: unmap_device - EXIT\n"));
}


/****************************************************************************
 * Surf the PCI bus for devices we can use.
 */
static void probe_devices()
{
	long	pci_index = 0;
	devinfo	*di =neomagicdd->dd_DI;
	uint32	count = 0;

//	dprintf(("neomagic: probe_devices - ENTER\n"));

	while ((count < MAX_CARDS)  &&
	       ((*pci_bus->get_nth_pci_info)(pci_index, &(di->di_PCI)) ==
	        B_NO_ERROR))
	{

//	dprintf(("neomagic: probe_devices - PCI Card #%d, vendor_id = 0x%x, device_id = 0x%x\n", pci_index, di->di_PCI.vendor_id, di->di_PCI.device_id));

		if ((di->di_PCI.vendor_id== VENDORID_NEOMAGIC) && 
			(
			 (di->di_PCI.device_id == DEVICEID_NM2070) || 
			 (di->di_PCI.device_id == DEVICEID_NM2090) || 
			 (di->di_PCI.device_id == DEVICEID_NM2093) || 
			 (di->di_PCI.device_id == DEVICEID_NM2097) || 
			 (di->di_PCI.device_id == DEVICEID_NM2160) || 
			 (di->di_PCI.device_id == DEVICEID_NM2200) ||
			 (di->di_PCI.device_id == DEVICEID_NM256ZV)
			)) 
		{		
			sprintf (di->di_Name,
				 "graphics%s/%04X_%04X_%02X%02X%02X",
				 GRAPHICS_DEVICE_PATH,
				 di->di_PCI.vendor_id, di->di_PCI.device_id,
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);

			neomagicdd->dd_DevNames[count] = di->di_Name;
			di->di_SlotNum = pci_index;

//			dprintf(("neomagic: probe_devices - /dev/%s: slot num: %d\n", di->di_Name, pci_index));

			/*  Initialize to...  uh...  uninitialized.  */
			di->di_BaseAddr0ID		=
			di->di_BaseAddr1ID		=
			di->di_neomagicCardInfo_AreaID	= -1;
			di->di_NInterrupts		=
			di->di_NVBlanks			=
			di->di_LastIrqMask		= 0;

			di++;
			count++;
		}

		pci_index++;

	}

	neomagicdd->dd_NDevs = count;

	/*  Terminate list of device names with a null pointer  */
	neomagicdd->dd_DevNames[count] = NULL;

	if (count == 0)
	{
		dprintf(("neomagic: No Neomagic cards installed - EXIT\n"));
		return;
	}
	else
	{
		dprintf(("neomagic: %d Neomagic card(s) installed - EXIT\n", count));
		return;
	}
}


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
	
//	dprintf(("neomagic: init_hardware - Enter - PCI neomagic-driver: %s %s\n", __DATE__, __TIME__));
	
	if ((idx = get_module (B_PCI_MODULE_NAME,
			       (module_info **) &pci_bus)) != B_OK)
		/*  Well, really, what's the point?  */
		return ((status_t) idx);

	/*  Search all PCI devices for something interesting.  */
	for (idx = 0;
	     (*pci_bus->get_nth_pci_info)(idx, &pcii) == B_NO_ERROR;
	     idx++)
	{
		if ((pcii.vendor_id == VENDORID_NEOMAGIC)  &&
		    (
		    (pcii.device_id == DEVICEID_NM2070) ||
		    (pcii.device_id == DEVICEID_NM2090) ||
		    (pcii.device_id == DEVICEID_NM2093) ||
		    (pcii.device_id == DEVICEID_NM2097) ||
		    (pcii.device_id == DEVICEID_NM2160) ||
		    (pcii.device_id == DEVICEID_NM2200) ||
		    (pcii.device_id == DEVICEID_NM256ZV)
		    ))
		{
			/*  My, that looks interesting...  */
//			dprintf(("neomagic: init_hardware - found Neomagic hardware @ %d\n", idx));
			found_one = TRUE;
			break;
		}
	}

	put_module (B_PCI_MODULE_NAME);  pci_bus = NULL;
	return (found_one ?  B_OK :  B_ERROR);
}

/*------------------------------------------------------------------------*/

status_t 
init_driver (void)
{
	int32 retval;
//	int32 i, len;
//	char devStr[1024];
	
//	dprintf(("neomagic: init_driver - Enter - PCI Neomagic-driver: %s %s\n", __DATE__, __TIME__));

	/*
	 * Get a handle for the PCI bus
	 */
	if ((retval = get_module (B_PCI_MODULE_NAME,
				  (module_info **) &pci_bus)) != B_OK)
		return (retval);

	/*  Create driver's private data area.  */
	if (!(neomagicdd = (driverdata *) calloc (1, sizeof (driverdata)))) {
		retval = B_ERROR;
		goto ackphft;	/*  Look down  */
	}

	if ((retval = initOwnedBena4 (&neomagicdd->dd_DriverLock,
				      "Neomagic kernel driver",
				      B_SYSTEM_TEAM)) < 0)
	{
		free (neomagicdd);
ackphft:	put_module (B_PCI_MODULE_NAME);
		return (retval);
	}
	/*  Initialize spinlock  */
	splok = 0;

	/*  Find all of our supported devices  */
	probe_devices ();

#if defined(NEOMAGIC_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
	add_debugger_command ("neomagicdump",
			      neomagicdump,
			      "neomagicdump - dump Neomagic kernel driver data");
	add_debugger_command ("idxinb",
			      idxinb,
			      "idxinb - dump VGA-style indexed I/O registers");
	add_debugger_command ("idxoutb",
			      idxoutb,
			      "idxoutb - write VGA-style indexed I/O registers");
#endif

//	dprintf(("neomagic: init_driver - EXIT\n"));
	return (B_OK);
}

/****************************************************************************
 * Interrupt management.
 */
static void
clearinterrupts (register struct neomagic_card_info *ci)
{
}

static void
enableinterrupts (register struct neomagic_card_info *ci, int enable)
{
//	dprintf(("neomagic: enableinterrupts - ENTER ci = 0x%x, enable = %d\n", ci, enable));
	if (enable)
		dprintf(("neomagic: enableinterrupts - Turn interrupts on\n"));
	else
		dprintf(("neomagic: enableinterrupts - Turn interrupts off\n"));
//	dprintf(("neomagic: enableinterrupts - EXIT\n"));
}

static int32
neomagic_interrupt (void *data)
{
	register devinfo	*di;
	register neomagic_card_info	*ci;
	register uint32		bits = 0;
	bool resched;

	resched=false;

	di = data;
	ncalls++;
	if (!(di->di_IRQEnabled))
		return (B_UNHANDLED_INTERRUPT);

	ci = di->di_neomagicCardInfo;
//	bits = ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl;
	
	if (!(bits & (1 << 8)))
		return (B_UNHANDLED_INTERRUPT);		/* Not a Vertical Sync Interrupt */

	/*  Clear the interrupt bit.  */
//	ci->ci_Regs->regs_2d_u.regs_2d.intrCtrl = (bits	& (~(1 << 8))) | (1 << 31);

	if (neomagicdd)
		neomagicdd->dd_NInterrupts++;
	di->di_NInterrupts++;
	di->di_LastIrqMask = bits;

	/*
	 * Most common case (not to mention the only case) is VBLANK;
	 * handle it first.
	 */
	if (bits && (1 << 8))
	{
		register int32	*flags;

		di->di_NVBlanks++;
		flags = &ci->ci_IRQFlags;
		if (atomic_and (flags, ~IRQF_SETFBBASE) & IRQF_SETFBBASE) {
			/*
			 * Change the framebuffer base address here.
			 */
			uint32 fb_offset, stride;
			uint32 regBase;
		
			if (ci->ci_DisplayAreaX == 0)
			{
				regBase = io_base_offset;
				fb_offset = (uint32)ci->ci_FBBase - (uint32) ci->ci_BaseAddr1;
				stride = ci->ci_BytesPerRow;
				fb_offset += ci->ci_DisplayAreaY*stride;
//				ISET32(vidDesktopStartAddr, (fb_offset & SST_VIDEO_START_ADDR) <<
//							SST_VIDEO_START_ADDR_SHIFT);
			}
		}
		if (atomic_and (flags, ~IRQF_MOVECURSOR) & IRQF_MOVECURSOR) {
			/*
			 * Move the hardware cursor.
			 */
//			uint32 regBase = io_base_offset;
//	    ISET32(hwCurLoc, ((ci->ci_MousePosX - ci->ci_MouseHotX + 64) << SST_CURSOR_X_SHIFT) | ((ci->ci_MousePosY - ci->ci_MouseHotY + 64) << SST_CURSOR_Y_SHIFT));
		}

		/*  Release the vblank semaphore.  */
		if (ci->ci_VBlankLock >= 0) {
			int32 blocked;

			if (get_sem_count (ci->ci_VBlankLock,
					   &blocked) == B_OK  &&
					    blocked < 0) {
				release_sem_etc (ci->ci_VBlankLock,
						 -blocked,
						 B_DO_NOT_RESCHEDULE);
				resched = true;
			}
		}
	}

	if (bits) {
		/*  Where did this come from?  */
		dprintf (("neomagic: neomagic_interrupt -  Weird Neomagic interrupt (tell awk): bits = 0x%08lx\n", bits));
	}

	return resched?(B_INVOKE_SCHEDULER):(B_HANDLED_INTERRUPT);
}

int32
int_check_handler(timer *t)
{
	neomagic_card_info	*ci;
	devinfo		*di;

	di = (devinfo *) t;
	
	ci = di->di_neomagicCardInfo;

	if (di->di_NVBlanks > 0)
		ci->ci_IRQFlags |= IRQF__ENABLED;
	else
	{
		ci->ci_IRQFlags &= ~IRQF__ENABLED;
	}
	
	return B_HANDLED_INTERRUPT;
}
 
/****************************************************************************
 * Device hook (and support) functions.
 */
static status_t
firstopen (struct devinfo *di)
{
	neomagic_card_info	*ci;
	thread_id	thid;
	thread_info	thinfo;
	status_t	retval = B_OK;
	int		n;
	char		shared_name[B_OS_NAME_LENGTH];

//dprintf(("neomagic: firstopen - ENTER - di = 0x%08lx\n", di));

	/*
	 * Create globals for driver and accelerant.
	 */
	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
		 di->di_PCI.vendor_id, di->di_PCI.device_id,
		 di->di_PCI.bus, di->di_PCI.device, di->di_PCI.function);
	n = (sizeof (struct neomagic_card_info) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
				   (void **) &di->di_neomagicCardInfo,
				   B_ANY_KERNEL_ADDRESS,
				   n * B_PAGE_SIZE,
				   B_FULL_LOCK,
				   B_READ_AREA | B_WRITE_AREA)) < 0)
		return (retval);
	di->di_neomagicCardInfo_AreaID = retval;

	ci = di->di_neomagicCardInfo;
	memset (ci, 0, sizeof (*ci));

	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (ci, thinfo.team)) < 0)
	{
		dprintf(("neomagic: firstopen - init_locks returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->di_neomagicCardInfo_AreaID >= 0) {
			delete_area (di->di_neomagicCardInfo_AreaID);
			di->di_neomagicCardInfo_AreaID = -1;
			di->di_neomagicCardInfo = NULL;
		}
	}

	/*  Map the card's resources into memory.  */
	if ((retval = map_device (di)) < 0)
	{
		dprintf(("neomagic: firstopen map_device() returned 0x%x\n", retval));
		dispose_locks (ci);
		if (di->di_neomagicCardInfo_AreaID >= 0) {
			delete_area (di->di_neomagicCardInfo_AreaID);
			di->di_neomagicCardInfo_AreaID = -1;
			di->di_neomagicCardInfo = NULL;
		}
	}
	
	// Initialize ci_regs so that the interrupt handler works correctly
//	ci->ci_Regs = (neomagicRegs *) ci->ci_BaseAddr0;

#if 0
	/* Add a kernel timer - when this 'goes off' we should check to see whether
	 * any VBL interupts have occured - if they haven't we need to change the setting
	 * of ci_IRQFlags. This should catch some (notably Diamonds) cards which do not seem to
	 * enable Interrupts correctly !
	 */
	add_timer(&(di->di_int_check_timer), &int_check_handler, INT_CHECK_TIMEOUT, B_ONE_SHOT_RELATIVE_TIMER);
	
	/*  Install interrupt handler  */
	if ((di->di_PCI.u.h0.interrupt_line != 0) &&
		(di->di_PCI.u.h0.interrupt_line != 0xff) &&
		((retval = install_io_interrupt_handler(di->di_PCI.u.h0.interrupt_line, neomagic_interrupt, (void *) di, 0)) == B_OK))
	{
		dprintf(("neomagic: firstopen - install_io_interrupt_handle() returned 0x%x, set IRQF__ENABLED\n", retval));
		ci->ci_IRQFlags |= IRQF__ENABLED;
	}
	else
	{
		dprintf(("neomagic: firstopen - install_io_interrupt_handle() returned 0x%x, not setting IRQF__ENABLED\n", retval));
		retval = B_OK;
	}
	
 	clearinterrupts (ci);

	/* notify the interrupt handler that this card is a candidate */
	if (ci->ci_IRQFlags & IRQF__ENABLED)
	{
	 	di->di_IRQEnabled = 1;
	 	enableinterrupts (ci, TRUE);
	}
#endif
	
	/*  Copy devname to shared area so device may be reopened...  */
	strcpy (ci->ci_DevName, di->di_Name);


	/* Note exactly which card variant this is */
	switch (di->di_PCI.device_id)
	{
		case DEVICEID_NM2070:
		case DEVICEID_NM2090:
		case DEVICEID_NM2093:
		case DEVICEID_NM2097:
		case DEVICEID_NM2160:
		case DEVICEID_NM2200:
		case DEVICEID_NM256ZV:
			ci->ci_device_id = di->di_PCI.device_id;
			break;
		default:
			ci->ci_device_id = 0;
			break;
	}
	
	/*  All done, return the status.  */
//	dprintf(("neomagic: firstopen returning 0x%08x\n", retval));
	return (retval);
}

static void
lastclose (struct devinfo *di)
{
//	dprintf(("neomagic: lastclose - ENTER\n"));

	di->di_IRQEnabled = 0;

 	enableinterrupts (di->di_neomagicCardInfo, FALSE);
 	clearinterrupts (di->di_neomagicCardInfo);
	remove_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,
				    neomagic_interrupt, di);

	unmap_device (di);

	dispose_locks (di->di_neomagicCardInfo);

	delete_area (di->di_neomagicCardInfo_AreaID);
	di->di_neomagicCardInfo_AreaID	= -1;
	di->di_neomagicCardInfo = NULL;

//	dprintf(("neomagic: lastclose - EXIT\n"));
}

static long
nm_open(const char* name, ulong flags, void** cookie)
{
  	int n;
	devinfo		*di;
	status_t	retval = B_OK;
	per_open_data *pod;

//	dprintf(("neomagic: nm_open - ENTER, name is %s\n", name));

	lockBena4 (&neomagicdd->dd_DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0;
	    neomagicdd->dd_DevNames[n]  &&  strcmp (name, neomagicdd->dd_DevNames[n]);
	     n++)
		;
	di = &neomagicdd->dd_DI[n];

	if (di->di_Opened == 0) {
		if ((retval = firstopen (di)) != B_OK)
		{
			dprintf(("neomagic: nm_open - firstopen() != B_OK !\n"));
			goto err;	/*  Look down  */
		}
	}

	/*  Mark the device open.  */
	di->di_Opened++;

	/*  Squirrel this away.  */
	pod = (per_open_data *)malloc( sizeof(per_open_data ));
	*cookie = pod;
	pod->di = di;
	
	/*  End of critical section.  */
err:
	unlockBena4 (&neomagicdd->dd_DriverLock);

	/*  All done, return the status.  */
//	dprintf(("neomagic: nm_open - returning 0x%08x\n", retval));
	return (retval);
}  

static long
nm_close (void* _cookie)
{
//	per_open_data *pod = (per_open_data *)_cookie;
//	devinfo *di = pod->di;

//	dprintf(("neomagic: nm_close - ENTER\n"));
	put_module (B_PCI_MODULE_NAME);  pci_bus = NULL;

//	dprintf(("neomagic: nm_close - EXIT\n"));
	return B_NO_ERROR;
}

static long
nm_free(void* cookie)
{
	per_open_data *pod = (per_open_data *)cookie;
	devinfo		*di = pod->di;

//	dprintf(("neomagic: nm_free - ENTER\n"));

	lockBena4 (&neomagicdd->dd_DriverLock);

	/*  Mark the device available.  */
	if (--di->di_Opened == 0)
		lastclose (di);

	unlockBena4 (&neomagicdd->dd_DriverLock);
//	dprintf(("neomagic: nm_free - EXIT\n"));

	return (B_OK);
}

static long
nm_read (void* cookie, off_t pos, void* buf, size_t* nbytes)
{
//	dprintf(("neomagic: nm_read - ENTER\n"));
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static long
nm_write (void* cookie, off_t pos, const void* buf, size_t* nbytes)
{
//	dprintf(("neomagic: nm_write - ENTER\n"));
	*nbytes = 0;
	return B_NOT_ALLOWED;
}

static long
nm_control (void* _cookie, ulong msg, void* buf, size_t len)
{
	per_open_data *pod = (per_open_data *)_cookie;
	devinfo		*di = pod->di;

	status_t	retval = B_DEV_INVALID_IOCTL;

	switch (msg) {
	/*
	 * Stuff the accelerant needs to do business.
	 */
	/*
	 * The only PUBLIC ioctl.
	 */
	case B_GET_ACCELERANT_SIGNATURE:
//		dprintf(("neomagic: nm_control - B_GET_ACCELERANT_SIGNATURE\n"));
		strcpy ((char *) buf, "neomagic.accelerant");
		retval = B_OK;
		break;

	case B_GET_3D_SIGNATURE:
//		dprintf(("neomagic: nm_control - B_GET_3D_SIGNATURE\n"));
		retval = B_ERROR;
		break;
	
	/*
	 * PRIVATE ioctl from here on.
	 */
	case NEOMAGIC_IOCTL_GETGLOBALS:
//		dprintf(("neomagic: nm_control - NEOMAGIC_IOCTL_GETGLOBALS\n"));
#define	GG	((neomagic_getglobals *) buf)

		/*
		 * Check protocol version to see if client knows how to talk
		 * to us.
		 */
		if (GG->gg_ProtocolVersion == NEOMAGIC_IOCTLPROTOCOL_VERSION) {
			GG->gg_GlobalArea = di->di_neomagicCardInfo_AreaID;
			retval = B_OK;
		}
		break;

	case NEOMAGIC_IOCTL_WRITE_VGAREG:
		{
#define VGAREG  ((neomagic_readwrite_vgareg *)buf) 
//			dprintf(("neomagic: nm_control - NEOMAGIC_WRITE_VGAREG, reg = 0x%x, value = 0x%x\n", VGAREG->reg, VGAREG->value));
			retval = write_vgareg(VGAREG->reg, VGAREG->value);
			break;
		}
	case NEOMAGIC_IOCTL_READ_VGAREG:
		{
			retval = read_vgareg(VGAREG->reg, &VGAREG->value);
//			dprintf(("neomagic: nm_control - NEOMAGIC_READ_VGAREG, reg = 0x%x, value = 0x%x\n", VGAREG->reg, VGAREG->value));
			break;
		}
	default:
		dprintf(("neomagic: nm_control - Unknown ioctl = 0x%x\n", msg));
		break;
	}
//	dprintf(("neomagic: nm_control - EXIT, returning 0x%x\n", retval));
	return (retval);
}

device_hooks *
find_device(const char *name) {
	int index;

//	dprintf(("neomagic: find_device - Enter name=%s\n", name));

	for (index = 0;  neomagicdd->dd_DevNames[index];  index++) {
		if (strcmp (name, neomagicdd->dd_DevNames[index]) == 0)
		{
//			dprintf(("neomagic: find_device - Match Found - returning device hooks\n"));
			return &neomagic_device_hooks;
		}
	}
// 	dprintf(("neomagic: find_device - Failed to find match - returning NULL\n"));
	return (NULL);
}

void uninit_driver(void) {
//	dprintf(("neomagic: uninit_driver - Enter\n"));

#if defined(NEOMAGIC_DEBUG)  || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
		remove_debugger_command ("neomagicdump", neomagicdump);
		remove_debugger_command ("idxinb", idxinb);
		remove_debugger_command ("idxoutb", idxoutb);
#endif
//	dprintf(("neomagic: uninit_driver - Exit\n"));
}


const char **
publish_devices(void) {
	const char **dev_list = NULL;
	/* return the list of supported devices */
//	dprintf(("neomagic: publish_devices\n"));
	dev_list = (const char **) neomagicdd->dd_DevNames;
	return (dev_list);
}

/*****
 * Reading & Writing VGA registers (located in the bottom of memory)
 */
status_t read_vgareg(uint32 reg, uint8 *value)
{
	*value = (pci_bus->read_io_8) (reg);
	return B_OK;
}

status_t write_vgareg(uint32 reg, uint8 value)
{
	(pci_bus->write_io_8) (reg, value);
	return B_OK;
}

/****************************************************************************
 * Debuggery.
 */

#if defined(NEOMAGIC_DEBUG)   || defined(FORCE_DEBUG) || defined (DEBUG_EXTENSIONS)
static int
neomagicdump (int argc, char **argv)
{
	int i;
	uint32 readPtrL1, readPtrL2, hwreadptr;
	readPtrL1 = 1; readPtrL2 = 2; hwreadptr =0;
	
	kprintf ("Neomagic kernel driver\nThere are %d device(s)\n", neomagicdd->dd_NDevs);
	kprintf ("Driver-wide data: 0x%08lx\n", neomagicdd);
	kprintf ("Driver-wide benaphore: %d/%d\n",
		 neomagicdd->dd_DriverLock.b4_Sema4,
		 neomagicdd->dd_DriverLock.b4_FastLock);
	kprintf ("Total calls to IRQ handler: %ld\n", ncalls);
	kprintf ("Total relevant interrupts: %ld\n", neomagicdd->dd_NInterrupts);

	for (i = 0;  i < neomagicdd->dd_NDevs;  i++) {
		devinfo		*di = &neomagicdd->dd_DI[i];
		neomagic_card_info	*ci = di->di_neomagicCardInfo;

		kprintf ("Device %s\n", di->di_Name);
		kprintf ("\tTotal interrupts: %ld\n", di->di_NInterrupts);
		kprintf ("\tVBlanks: %ld\n", di->di_NVBlanks);
		kprintf ("\tLast IRQ mask: 0x%08lx\n", di->di_LastIrqMask);
		if (ci) {
			kprintf ("  neomagic_card_info: 0x%08lx\n", ci);
			kprintf ("  Base register 0,1: 0x%08lx, 0x%08lx\n",
				 ci->ci_BaseAddr0, ci->ci_BaseAddr1);
			kprintf ("  IRQFlags: 0x%08lx\n", ci->ci_IRQFlags);
		}
	}
	kprintf ("\n");
	return (B_OK);
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
	register int	port;
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


