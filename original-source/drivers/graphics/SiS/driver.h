#ifndef DRIVER_H
#define DRIVER_H

#include <kernel/OS.h>
#include <device/graphic_driver.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <drivers/ISA.h>
#include <Debug.h>
#include <Drivers.h>
#include <Accelerant.h>
#include <GraphicsCard.h>
#include <video_overlay.h>
#include <SupportDefs.h>
#include <stdio.h>
#include <string.h>
#include <drivers/genpool_module.h>

#include "bena4.h"
#include "sisGlobals.h"
#include "sis5598defs.h"
#include "sis6326defs.h"
#include "sis620defs.h"
#include "sis630defs.h"
#include "sis_accelerant.h"


////////////////
// STRUCTURES //
////////////////

typedef struct devinfo {
	pci_info	di_PCI;
	area_id		di_BaseAddr0ID;
	area_id		di_BaseAddr1ID;
	area_id		di_BaseAddr2ID;
	area_id		di_sisCardInfo_AreaID; // clones shared memory area
	uint32		di_PoolID;
	sis_card_info	*di_sisCardInfo;

	uint32		di_Opened;
	int32		di_IRQEnabled;
	uint32		di_NInterrupts;	// DEBUG
	uint32		di_NVBlanks;	// DEBUG
	uint32		di_LastIrqMask;	// DEBUG
	char		di_Name[B_OS_NAME_LENGTH];
	uint32		di_opentoken ;
	
	display_mode di_PrimaryDisplayMode ;
	uint32		di_SecondaryDisplayModeEnabled ;
} devinfo;

typedef struct peropenerdata {
	devinfo		*pod_di ;
	} peropenerdata ;
	

#define	MAXDEVS			4

typedef struct driverdata {
	Bena4		dd_DriverLock;	            /*  Driver-wide benaphore	*/
	uint32		dd_NDevs;	                /*  # of devices found		*/
	uint32		dd_NInterrupts;	            /*  # of IRQs from this device	*/
	char		*dd_DevNames[MAXDEVS + 1];  /*  For export  */
	devinfo		dd_DI[MAXDEVS];	            /* Device-specific stuff	*/
} driverdata;


// Driver Specific IO functions

extern pci_module_info		*pci_bus;
extern isa_module_info		*isa_bus;

#define	get_pci(pci,o,s)	(*pci_bus->read_pci_config)((pci)->bus, \
							    (pci)->device, \
							    (pci)->function, \
							    (o), (s))
#define	set_pci(pci,o,s,v)	(*pci_bus->write_pci_config)((pci)->bus, \
							     (pci)->device, \
							     (pci)->function, \
							     (o), (s), (v))

#define isa_outb(a,b)	(*isa_bus->write_io_8)((a),(b))
#define isa_inb(a)		(*isa_bus->read_io_8)((a))

#endif
