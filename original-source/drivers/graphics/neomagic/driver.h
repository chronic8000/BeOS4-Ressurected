/*
   	This contains the structures needed to interface
   	with the pseudo-driver
*/

#ifndef __NEOMAGIC_DRIVER_H
#define __NEOMAGIC_DRIVER_H

#include <KernelExport.h>

#include <graphics_p/neomagic/neomagic.h>

/*****************************************************************************
 * Definitions.
 */
#define inb(p) (*pci_bus->read_io_8)(p)
#define outb(p,v) (*pci_bus->write_io_8)(p,v)
#define inw(p) (*pci_bus->read_io_16)(p)
#define outw(p,v) (*pci_bus->write_io_16)(p,v)
#define inl(p) (*pci_bus->read_io_32)(p)
#define outl(p,v) (*pci_bus->write_io_32)(p,v)

#define	get_pci(pci,o,s)	(*pci_bus->read_pci_config)((pci)->bus, \
							    (pci)->device, \
							    (pci)->function, \
							    (o), (s))
#define	set_pci(pci,o,s,v)	(*pci_bus->write_pci_config)((pci)->bus, \
							     (pci)->device, \
							     (pci)->function, \
							     (o), (s), (v))

#define MAX_PCI_DEVICES		32
#define MAX_CARDS		4

#define INT_CHECK_TIMEOUT 500000

/*
 * GRAPHICS_DEVICE_PATH will be set to non-NULL in the Makefile for
 * debug/development builds.
 */
#ifndef	GRAPHICS_DEVICE_PATH
#define	GRAPHICS_DEVICE_PATH	""
#endif

/****************************************************************************
 * Structure definitions.
 */
typedef struct devinfo {
	struct timer	di_int_check_timer;	/* This MUST come first for the Kernel Timer handler to succeed !! - nasty typecasting ! */
	pci_info	di_PCI;
	area_id		di_BaseAddr0ID; 		/* Banshee 2D/3D & AGP Memory Mapped Registers */
	area_id		di_BaseAddr1ID; 		/* Banshee Memory Mapped Frame Buffer */
	area_id		di_BaseAddr2ID; 		/* Banshee Memory Mapped Frame Buffer */
	area_id		di_neomagicCardInfo_AreaID; 
	neomagic_card_info	*di_neomagicCardInfo;

	area_id		di_MemMgr_AreaID; 

	uint32		di_SlotNum;
	
	unsigned char *di_CursorCopy;	/* We keep a copy of the cursor so that we can restore it */
	
	uint32		di_Opened;
	int32		di_IRQEnabled;
	uint32		di_NInterrupts;	// DEBUG
	uint32		di_NVBlanks;	// DEBUG
	uint32		di_LastIrqMask;	// DEBUG
	char		di_Name[B_OS_NAME_LENGTH];
} devinfo;

typedef struct driverdata {
	Bena4		dd_DriverLock;	/*  Driver-wide benaphore	*/
	uint32		dd_NDevs;	/*  # of devices found		*/
	uint32		dd_NInterrupts;	/*  # of IRQs from this device	*/
	char		*dd_DevNames[MAX_CARDS + 1];  /*  For export  */
	devinfo		dd_DI[MAX_CARDS];	/* Device-specific stuff	*/
} driverdata;


#endif 

