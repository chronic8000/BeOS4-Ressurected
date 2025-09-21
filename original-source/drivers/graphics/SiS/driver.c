// Driver definitions
#include "driver.h"
#include "driver_ioctl.h"

// Functions headers
#include "setup.h"
#include "sisClock.h"
#include "sisCRTC.h"
#include "sisCursor.h"
#include "sisDPMS.h"
#include "sisVideoModes.h"

// Driver functions to export

status_t         init_hardware(void);
const char       **publish_devices();
device_hooks     *find_device(const char *name);
status_t         init_driver(void);
void             uninit_driver(void);
int32    api_version=2;  

// **************
status_t _set_display_mode (devinfo *di, display_mode *m) ;

// prototypes

static void probe_devices (void);

static status_t open_hook		(const char *name, uint32 flags, void **cookie);
static status_t read_hook		(void *cookie, off_t pos, void *buf, size_t *len);
static status_t write_hook		(void *cookie, off_t pos, const void *buf, size_t *len);
static status_t close_hook		(void *cookie);
static status_t free_hook		(void *cookie);
static status_t control_hook	(void *cookie, uint32 msg, void *buf, size_t len);

static void dispose_locks (register struct sis_card_info *ci);
static status_t init_locks (register struct sis_card_info *ci, team_id team);

// Interrupt Routines .H -- SiS630 tested only
int32 sis_interrupt (void *data);
void clearinterrupts (devinfo *di);
void enableinterrupts (devinfo *di, int enable);

// Globals
driverdata			*sis_dd ;
isa_module_info		*isa_bus;
pci_module_info		*pci_bus;
genpool_module		*ggpm ;
BPoolInfo			*gfbpi;

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



///////////////////////////////
// INIT_LOCKS & DISPOSE_LOCKS
///////////////////////////////

static status_t init_locks (register struct sis_card_info *ci, team_id team) {
	status_t	retval;

	/*  Initialize {sem,ben}aphores the client will need.  */
	if ((retval = initOwnedBena4 (&ci->ci_CRTCLock,
				      "sis CRTC lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_CLUTLock,
				      "sis CLUT lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_EngineLock,
				      "sis rendering engine lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_EngineRegsLock,
				      "sis engine registers lock",
				      team)) < 0)
		return (retval);
	if ((retval = initOwnedBena4 (&ci->ci_SequencerLock,
				      "sis sequencer lock",
				      team)) < 0)
		return (retval);

	if ((retval = create_sem (0, "sis VBlank semaphore")) < 0)
		return (retval);
	ci->ci_VBlankLock = retval;
	set_sem_owner (retval, team);
	return (B_OK);
}

static void dispose_locks (register struct sis_card_info *ci) {
	if (ci->ci_VBlankLock >= 0)
		delete_sem (ci->ci_VBlankLock), ci->ci_VBlankLock = -1;
	disposeBena4 (&ci->ci_SequencerLock);
	disposeBena4 (&ci->ci_EngineRegsLock);
	disposeBena4 (&ci->ci_EngineLock);
	disposeBena4 (&ci->ci_CLUTLock);
	disposeBena4 (&ci->ci_CRTCLock);
}


///////////////
// FIRST_OPEN
///////////////

/* --- Create globals for driver and accelerant. --- */

static status_t firstopen (devinfo *di) {
	sis_card_info	*ci;
	thread_id		thid;
	thread_info		thinfo;
	status_t		retval = B_OK;
	int				n;
	char			shared_name[B_OS_NAME_LENGTH];

	ddprintf (("sis:+++ firstopen(0x%08lx)\n", (ulong) di));

	di->di_opentoken = 0 ;
	
	/* Prepare shared_name */

	sprintf (shared_name, "%04X_%04X_%02X%02X%02X shared",
		di->di_PCI.vendor_id,
		di->di_PCI.device_id,
		di->di_PCI.bus,
		di->di_PCI.device,
		di->di_PCI.function
		);
		
	/* Create Card Info Area ( for accelerants ) */

	n = (sizeof (struct sis_card_info) + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	if ((retval = create_area (shared_name,
						(void **) &di->di_sisCardInfo,
						B_ANY_KERNEL_ADDRESS,
						n * B_PAGE_SIZE,
						B_FULL_LOCK,
						B_READ_AREA | B_WRITE_AREA)) < 0)
		goto err;
	
	di->di_sisCardInfo_AreaID = retval;
	ci = di->di_sisCardInfo;
	memset (ci, 0, sizeof (*ci));


	/*  Initialize {sem,ben}aphores the client will need.  */
	thid = find_thread (NULL);
	get_thread_info (thid, &thinfo);
	if ((retval = init_locks (ci, thinfo.team)) < 0)
		goto err0;


	/* Identify chipset for accelerant */
	switch(di->di_PCI.device_id) {
#if OBJ_SIS5598
		case SIS5598_DEVICEID:
			ci->ci_DeviceId = SIS5598_DEVICEID;
			break;
#endif
#if OBJ_SIS6326
		case SIS6326_DEVICEID:
			ci->ci_DeviceId = SIS6326_DEVICEID;
			break;
#endif
#if OBJ_SIS620
		case SIS620_DEVICEID:
			ci->ci_DeviceId = SIS620_DEVICEID;
			break;
#endif
#if OBJ_SIS630
		case SIS630_DEVICEID:
			ci->ci_DeviceId = SIS630_DEVICEID;
			break;
#endif
		default:
			ddprintf(("sis: FATAL: non supported device 0x%04x\n",(uint16)di->di_PCI.device_id));
			return(B_ERROR);
		}
		
	ci->ci_Device_revision = di->di_PCI.revision ;
  	
	/*  Map the card's resources into memory.  */
	if (ci->ci_DeviceId != SIS630_DEVICEID) {
		if ((retval = map_device (di)) < 0)
			goto err1;
		}
	else {
		if ((retval = sis630_map_device (di)) < 0)
			goto err1;
		}
	
	/*  Create Pool descriptor (which doesn't get used much)  */
	if (!(gfbpi = (ggpm->gpm_AllocPoolInfo) ())) {
		retval = B_NO_MEMORY;
		goto err2;
		}
	else vddprintf(("sis: Pool descriptor allocated\n"));
	
	/*  Initialize the rest of the card.  */
	if (ci->ci_DeviceId != SIS630_DEVICEID) {
		if ((retval = sis_init (di)) < 0)
			goto err3;
		}
	else {
		if ((retval = sis630_init (di)) < 0)
			goto err3;
		}

	/*  Install interrupt handler  */
	if ((di->di_PCI.u.h0.interrupt_pin == 0x00) || (di->di_PCI.u.h0.interrupt_line == 0xff)) {
		ddprintf(("sis: no interrupt (pin=%d, line=%d)\n",di->di_PCI.u.h0.interrupt_pin,di->di_PCI.u.h0.interrupt_line));
		di->di_IRQEnabled = 0 ;
		}
	else {
		// interrupts
		ddprintf(("sis: interrupt pin  = %d\n",di->di_PCI.u.h0.interrupt_pin));
		ddprintf(("sis: interrupt line = %d\n",di->di_PCI.u.h0.interrupt_line));
		if (ci->ci_DeviceId == SIS630_DEVICEID) {
			status_t retval ;
			vddprintf(("sis: installing interrupt handler\n"));
			retval = install_io_interrupt_handler(di->di_PCI.u.h0.interrupt_line, sis_interrupt, (void *) di, 0);
			if (retval == B_OK) {
				vddprintf(("sis: interrupt handler installed OK\n"));
				}
			else {
				ddprintf(("sis: interrupt hanlder installation FAILED\n"));
				}
			vddprintf(("sis: clear interrupts\n"));
			clearinterrupts (di);
			vddprintf(("sis: enabling VBlank interrupt\n"));
			enableinterrupts (di, TRUE);
			// notify the interrupt handler that this card is a candidate
			}
		else ddprintf(("sis: *** interrupts code disabled because not tested on this device - tell gregory\n"));
		}

	//  Copy devname to shared area so device may be reopened...
	strcpy (ci->ci_DevName, di->di_Name);

	/*  All done, return B_OK  */
	ddprintf (("+++ firstopen returning B_OK\n"));
   	return (B_OK);

err3:
	ddprintf(("sis: FreePoolInfo\n"));
	if (gfbpi)	(ggpm->gpm_FreePoolInfo) (gfbpi), gfbpi = NULL;	
err2:
	ddprintf(("sis: unmapping device\n"));
	unmap_device (di);	
err1:
    ddprintf(("sis: disposing locks\n"));
	dispose_locks (ci);
err0:
    if (di->di_sisCardInfo_AreaID >= 0) {
		ddprintf(("sis: deleting card_info area\n"));
		delete_area (di->di_sisCardInfo_AreaID);
		di->di_sisCardInfo_AreaID = -1;
		di->di_sisCardInfo = NULL;
		}
err:
    ddprintf(("sis: *** first open failed\n"));
	return (retval);

  }


//////////////
// LASTCLOSE
//////////////

static void lastclose (struct devinfo *di) {
	//cpu_status	cs;
    ddprintf (("sis:+++ lastclose() begins...\n"));

	/*  Uninstall interrupt handler  */
	if ((di->di_PCI.u.h0.interrupt_pin != 0x00) && (di->di_PCI.u.h0.interrupt_line != 0xff)) {
		vddprintf(("sis: disabling VBlank interrupt\n"));
		enableinterrupts (di, FALSE);
		vddprintf(("sis: clear interrupts interrupts\n"));
		clearinterrupts (di);
		vddprintf(("sis: remove interrupt handler\n"));
		remove_io_interrupt_handler (di->di_PCI.u.h0.interrupt_line,sis_interrupt, di);
		}
		
	//free (di->di_DispModes, di->di_NDispModes * sizeof (display_mode));
	//di->di_DispModes = NULL;
	//di->di_NDispModes = 0;

	unmap_device (di);
	dispose_locks (di->di_sisCardInfo);
	delete_area (di->di_sisCardInfo_AreaID);
	di->di_sisCardInfo_AreaID = -1;
	di->di_sisCardInfo = NULL;
    ddprintf (("sis:+++ lastclose() completes.\n"));
	}


//////////////
// OPEN_HOOK
//////////////

static status_t open_hook (const char *name, uint32 flags, void **cookie) {
	devinfo		*di;
	peropenerdata *pod ;
	status_t	retval = B_OK;
	int		n;

    ddprintf (("sis:+++ open_hook(%s, %d, 0x%08x)\n", name, (int)flags, (uint)cookie));
	lockBena4 (&sis_dd->dd_DriverLock);

	/*  Find the device name in the list of devices.  */
	/*  We're never passed a name we didn't publish.  */
	for (n = 0;
	     sis_dd->dd_DevNames[n]  &&  strcmp (name, sis_dd->dd_DevNames[n]);
	     n++)
		;
	di = &sis_dd->dd_DI[n];

	/* Create Per Opener Data structure */
	pod = (peropenerdata*) malloc(sizeof(peropenerdata));
	if (!pod) goto err ;
	pod->pod_di = di ;

	/* First Time the device is opened */
	if (di->di_Opened == 0) {
		if ((retval = firstopen (di)) != B_OK)
			goto err;
		}
	else di->di_opentoken++ ;

	/*  Mark the device open.  */
	di->di_Opened++;

	/*  Squirrel this away.  */
	*cookie = (void*) pod ;
	
	/*  End of critical section.  */
err:	unlockBena4 (&sis_dd->dd_DriverLock);

	/*  All done, return the status.  */
    ddprintf (("sis:+++ open_hook returning 0x%08x\n", (uint)retval));
	return (retval);
    }

//////////////////////////////////////////////////////////
// -------- Read / Write / Close : Empty hooks -------- //
//////////////////////////////////////////////////////////

static status_t read_hook (void *cookie, off_t pos, void *buf, size_t *len) {
	*len = 0;
	return B_NOT_ALLOWED;
    }

static status_t write_hook (void *cookie, off_t pos, const void *buf, size_t *len) {
	*len = 0;
	return B_NOT_ALLOWED;
    }

static status_t close_hook (void *cookie) {
	ddprintf(("sis:+++ close_hook(%08x)\n", (uint)cookie));
	/*
		We don't do anything on close: there might be still something pending
		Everything has to be done in free_hook
	*/
	return B_NO_ERROR;
    }



//////////////
// FREE_HOOK
//////////////

static status_t restore_primary_displaymode(devinfo *di) {
	di->di_SecondaryDisplayModeEnabled = 0 ;
	return _set_display_mode (di, &di->di_PrimaryDisplayMode) ;
	}

static status_t free_hook (void *cookie) {
	peropenerdata *pod ;
	devinfo		*di ;
	pod = (peropenerdata*) cookie;
	di = pod->pod_di ;

    ddprintf (("sis:+++ free_hook() begins...\n"));

	lockBena4 (&sis_dd->dd_DriverLock);

	if (di->di_SecondaryDisplayModeEnabled) restore_primary_displaymode(di) ;

	/*  Is the device still opened ? */
	if (--di->di_Opened == 0)
		lastclose (di);

	unlockBena4 (&sis_dd->dd_DriverLock);
    ddprintf (("sis:+++ free_hook() completes.\n"));

	return (B_OK);
    }



/////////////////
// CONTROL_HOOK
/////////////////

static status_t control_hook (void *cookie, uint32 msg, void *buf, size_t len) {
	peropenerdata	*pod ;
	devinfo			*di ;
	status_t		retval ; 
	pod		= (peropenerdata*) cookie;
	di		= pod->pod_di ;
	retval	= B_OK; 
	
	// vddprintf(("sis:+++control_hook, msg=%d\n",(int)msg));
	switch (msg) {

		case B_GET_ACCELERANT_SIGNATURE:
    	    vddprintf(("sis: GET_ACCELERANT_SIGNATURE sisXXXX.accelerant\n"));
#if ( OBJ_SIS5598 & OBJ_SIS6326 & OBJ_SIS620 & OBJ_SIS630 )
			strcpy ((char *) buf, "sis.accelerant");
#else
#if OBJ_SIS5598
			strcpy ((char *) buf, "sis5598.accelerant");
#endif
#if OBJ_SIS6326
			strcpy ((char *) buf, "sis6326.accelerant");
#endif
#if OBJ_SIS620
			strcpy ((char *) buf, "sis620.accelerant");
#endif
#if OBJ_SIS630
			strcpy ((char *) buf, "sis630.accelerant");
#endif
#endif
			retval = B_OK;
			break;

		case SIS_IOCTL_GETGLOBALS:
			vddprintf(("sis: SIS_IOCTL_GETGLOBALS\n"));
			if (((accelerant_getglobals *) buf)->gg_ProtocolVersion == 0) {
				((accelerant_getglobals *) buf)->gg_GlobalArea = di->di_sisCardInfo_AreaID;
				retval = B_OK;
				}
			else {
				ddprintf(("sis: GetGlobals() failed, bad protocol version\n"));
				retval = B_ERROR ;
				}
			break;

		case SIS_IOCTL_SETDPMS:
			retval = Set_DPMS_mode ( *((uint32*)buf) );
			break;
		
		case SIS_IOCTL_GETDPMS:
			*((uint32*)buf) = Get_DPMS_mode();
			break;
	
		case SIS_IOCTL_SETCLOCK:
			retval = setClock( *((uint32*)buf), di->di_sisCardInfo->ci_DeviceId );
			break;

		case SIS_IOCTL_SET_CURSOR_SHAPE:
			retval = _set_cursor_shape((data_ioctl_set_cursor_shape*)buf, di);
			break;
		
		case SIS_IOCTL_SHOW_CURSOR:
			_show_cursor(*((uint32*)buf));
			retval = B_OK ;
			break;
		
		case SIS_IOCTL_MOVE_CURSOR:
			_move_cursor(((data_ioctl_move_cursor*)buf)->screenX, ((data_ioctl_move_cursor*)buf)->screenY, di);
			break;

		case SIS_IOCTL_SET_CRT:
			Set_CRT((data_ioctl_sis_CRT_settings *)buf, di);
			retval = B_OK ;
			break;
		
		case SIS_IOCTL_INIT_CRT_THRESHOLD:
			Init_CRT_Threshold(* ((uint32*) buf), di);
			retval = B_OK ;
			break;
			
		case SIS_IOCTL_SET_COLOR_MODE:
			if (di->di_sisCardInfo->ci_DeviceId != SIS630_DEVICEID)
				set_color_mode(* ((uint32*) buf), di);
			else
				sis630_SetColorMode(* ((uint32*) buf), di);
			retval = B_OK ;
			break;	
		
		case SIS_IOCTL_MOVE_DISPLAY_AREA:
			retval = Move_Display_Area(((data_ioctl_move_display_area*)buf)->x, ((data_ioctl_move_display_area*)buf)->y, di);
			break ;

		case SIS_IOCTL_SETUP_DAC:
			setup_DAC();
			retval = B_OK ;
			break ;
		
		case SIS_IOCTL_SET_INDEXED_COLORS:
			Set_Indexed_Colors ((data_ioctl_set_indexed_colors*)buf);
			retval = B_OK ;
			break;
			
		case SIS_IOCTL_SCREEN_OFF:
			Screen_Off();
			break;
		
		case SIS_IOCTL_RESTART_DISPLAY:
			Restart_Display();
			break;
			
		case SIS_IOCTL_SET_PRIMARY_DISPLAY_MODE:
			retval = _set_display_mode (di, (display_mode *)buf) ;
			if (retval == B_OK) {
				memcpy(&di->di_PrimaryDisplayMode, buf, sizeof(di->di_PrimaryDisplayMode));
				}
			di->di_SecondaryDisplayModeEnabled = 0 ;
			break;

		case SIS_IOCTL_SET_SECONDARY_DISPLAY_MODE:
			retval = _set_display_mode (di, (display_mode *)buf) ;
			if (retval == B_OK) {
				di->di_SecondaryDisplayModeEnabled = 1 ;
				}
			break;
			
		case SIS_IOCTL_RESTORE_PRIMARY_DISPLAY_MODE:
			retval = restore_primary_displaymode(di) ;
			break;
			
		default:
			ddprintf(("sis: control_hook() error - unknown IOCTL 0x%08x\n",(uint)msg));
			retval = B_DEV_INVALID_IOCTL;
			break;
	
		} //switch
	return (retval);
	}



//////////////////
// INIT_HARWARE
//////////////////

status_t init_hardware(void) {
  pci_info	pcii;
  status_t	err;
  int32		idx = 0;
  bool		found_one = FALSE;

  // DPRINTF_ON; // turns-on debug ouput
  ddprintf(("sis: --- init_harware()\n"));

  err = get_module (B_ISA_MODULE_NAME,(module_info **) &isa_bus);
  if (err != B_OK)
	return err;
  err = get_module (B_PCI_MODULE_NAME,(module_info **) &pci_bus);
  if (err != B_OK) {
	put_module(B_ISA_MODULE_NAME);
	return err;
  }

  vddprintf(("sis: pci module loaded\n"));

  /*  Search all PCI devices for something interesting.  */
  
  for (idx = 0; (*pci_bus->get_nth_pci_info)(idx, &pcii) == B_NO_ERROR; idx++) {

#if OBJ_SIS5598
    if ((pcii.vendor_id == SIS_VENDORID)  && (pcii.device_id == SIS5598_DEVICEID)) {
    	found_one = TRUE ;
    	break;
    	}
#endif
#if OBJ_SIS6326
    if ((pcii.vendor_id == SIS_VENDORID)  && (pcii.device_id == SIS6326_DEVICEID)) {
    	found_one = TRUE ;
    	break;
    	}
#endif
#if OBJ_SIS620
    if ((pcii.vendor_id == SIS_VENDORID)  && (pcii.device_id == SIS620_DEVICEID)) {
    	found_one = TRUE ;
    	break;
    	}
#endif
#if OBJ_SIS630
    if ((pcii.vendor_id == SIS_VENDORID)  && (pcii.device_id == SIS630_DEVICEID)) {
    	found_one = TRUE ;
    	break;
    	}
#endif

	}
  
	if (found_one) {
		vddprintf(("first isa_outb()\n"));
		isa_outb(VGA_ENABLE,0x01);
  		vddprintf(("first isa_outb() didn't throw you in kerneldebug....\n"));

		isa_outb(SEQ_INDEX,0x01); /* first turn off the screen */
		isa_outb(SEQ_DATA,0x21);
		}
		
	put_module (B_PCI_MODULE_NAME); 
	put_module (B_ISA_MODULE_NAME); 
	pci_bus = NULL;
	isa_bus = NULL;

	if (!found_one) return(B_ERROR);
	else return (B_OK);
	
	}
  

/////////////////
// INIT_DRIVER
/////////////////

status_t init_driver(void) {
	status_t retval;
		
	ggpm	= NULL;
	isa_bus	= NULL;
	pci_bus	= NULL;

	ddprintf(("sis: init_driver()\n"));

	retval = get_module (B_ISA_MODULE_NAME,(module_info **) &isa_bus);
	if (retval != B_OK) goto err0 ;

	retval = get_module (B_PCI_MODULE_NAME,(module_info **) &pci_bus);
	if (retval != B_OK) goto err0 ;

	/* Open the genpool module. */
	retval = get_module (B_GENPOOL_MODULE_NAME,(module_info **) &ggpm) ;
	if (retval != B_OK) goto err0 ;

	/*  Create driver's private data area.  */
	if (!(sis_dd = (driverdata *) calloc (1, sizeof (driverdata)))) {
		retval = B_ERROR;
		goto err1 ;
		}
		
	vddprintf(("sis: driverdata structure created\n"));

	if ((retval = initOwnedBena4 (&sis_dd->dd_DriverLock,"sis kernel driver", B_SYSTEM_TEAM)) < 0) {
		free (sis_dd);
		goto err1 ;
		}

	/*  Find all of our supported devices  */
	probe_devices ();

	return (B_OK);

err1:
	if (ggpm) put_module ( B_GENPOOL_MODULE_NAME );
	ggpm	= NULL ;
	
err0:
	if (pci_bus) put_module (B_PCI_MODULE_NAME);
	if (isa_bus) put_module (B_ISA_MODULE_NAME);
	pci_bus	= NULL;
	isa_bus	= NULL;
	return (retval);
  }
  

//////////////////
// UNINIT_DRIVER
//////////////////


void uninit_driver(void) {
	ddprintf (("sis:+++ uninit_driver\n"));
	
	if (sis_dd) {
    	cpu_status cs;
		disposeBena4 (&sis_dd->dd_DriverLock);
		cs = disable_interrupts ();
		//acquire_spinlock (&splok);
		free (sis_dd);
		sis_dd = NULL;
		//release_spinlock (&splok);
		restore_interrupts (cs);
		}

	if (pci_bus) put_module (B_PCI_MODULE_NAME);
	if (isa_bus) put_module (B_ISA_MODULE_NAME);
	if (ggpm)	 put_module (B_GENPOOL_MODULE_NAME);
	
	ggpm	= NULL;
	isa_bus	= NULL;
	pci_bus	= NULL;
	}
	
	
////////////////
// FIND_DEVICE
////////////////

device_hooks *find_device (const char *name) {
  int index;
  ddprintf (("sis:+++ find_device <%s>\n",name));
  for (index = 0;  sis_dd->dd_DevNames[index];  index++) {
    if (strcmp (name, sis_dd->dd_DevNames[index]) == 0)
	  return (&graphics_device_hooks);
    }
  return (NULL);
  }



////////////////////
// PUBLISH_DEVICES
////////////////////

const char ** publish_devices (void) {
  /*  Return the list of supported devices  */
  ddprintf (("sis:+++ publish_devices\n"));
  return ((const char **)sis_dd->dd_DevNames); // initialised in probe_devices()
  }


//////////////////  
// PROBE_DEVICES
//////////////////

static void probe_devices (void) {
	long	pci_index = 0;
	uint32	count = 0;
	uint32	pcicmd;
	devinfo	*di = sis_dd->dd_DI;

	pcicmd = PCI_command_memory |
#if (DEBUG  ||  FORCE_DEBUG)
		 PCI_command_io |
#endif
		 PCI_command_master;

	while ((count < MAXDEVS)  &&
	       ((*pci_bus->get_nth_pci_info)(pci_index, &(di->di_PCI)) ==
	        B_NO_ERROR))
		{

		if ( ((di->di_PCI.vendor_id==SIS_VENDORID)&&(di->di_PCI.device_id==SIS5598_DEVICEID))
			|((di->di_PCI.vendor_id==SIS_VENDORID)&&(di->di_PCI.device_id==SIS6326_DEVICEID))
			|((di->di_PCI.vendor_id==SIS_VENDORID)&&(di->di_PCI.device_id==SIS620_DEVICEID))
			|((di->di_PCI.vendor_id==SIS_VENDORID)&&(di->di_PCI.device_id==SIS630_DEVICEID))
		   )
			{

			// Found a card we can deal with; create a device for it.
			sprintf (di->di_Name,
				 "graphics/%04X_%04X_%02X%02X%02X",
				 di->di_PCI.vendor_id, di->di_PCI.device_id,
				 di->di_PCI.bus, di->di_PCI.device,
				 di->di_PCI.function);
			sis_dd->dd_DevNames[count] = di->di_Name;
            ddprintf (("sis: +++ making /dev/%s\n", di->di_Name));

			//  Initialize to...  uh...  uninitialized.
			di->di_BaseAddr0ID		=
			di->di_BaseAddr1ID		=
			di->di_BaseAddr2ID		=
			di->di_PoolID			=
			di->di_NInterrupts		=
			di->di_NVBlanks			=
			di->di_LastIrqMask		= 0;

			/*
			 * ###FIXME:  Add code to reserve/map VGA I/O ports.
			 */
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

	sis_dd->dd_NDevs = count;

	//  Terminate list of device names with a null pointer
	sis_dd->dd_DevNames[count] = NULL;
    ddprintf (("sis: +++ probe_devices: %d supported devices\n", (int)count));
	}

