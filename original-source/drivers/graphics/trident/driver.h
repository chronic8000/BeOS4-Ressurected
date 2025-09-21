/* :ts=8 bk=0
 *
 * driver.h:	Definitions common to the kernel driver.
 *
 * Leo L. Schwab					1998.08.06
 */
#ifndef	__DRIVER_H
#define	__DRIVER_H

#ifndef	_PCI_H
#include <drivers/PCI.h>
#endif
#ifndef	_TRIDENT_H
#include <graphics_p/trident/trident.h>
#endif


/****************************************************************************
 * #defines
 */
#define	get_pci(pci,o,s)	(*pci_bus->read_pci_config)((pci)->bus, \
							    (pci)->device, \
							    (pci)->function, \
							    (o), (s))
#define	set_pci(pci,o,s,v)	(*pci_bus->write_pci_config)((pci)->bus, \
							     (pci)->device, \
							     (pci)->function, \
							     (o), (s), (v))

#define inb(a)			(*pci_bus->read_io_8)(a)
#define outb(a,b)		(*pci_bus->write_io_8)((a), (b))
#define inw(a)			(*pci_bus->read_io_16)(a)
#define outw(a,b)		(*pci_bus->write_io_16)((a), (b))


#define	MAXDEVS			4

#define	VENDORID_TRIDENT		0x1023
#define	DEVICEID_CYBERBLADE_I7_MVP4	0x8400
#define	DEVICEID_CYBERBLADE_I7		0x8420


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
	pci_info	di_PCI;
	area_id		di_BaseAddr0ID;
	area_id		di_BaseAddr1ID;
	area_id		di_BaseAddr2ID;
	area_id		di_TriCardInfo_AreaID;
	area_id		di_TriCardCtl_AreaID;
	tri_card_info	*di_TriCardInfo;
	tri_card_ctl	*di_TriCardCtl;

	display_mode	*di_DispModes;
	uint32		di_NDispModes;

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
	char		*dd_DevNames[MAXDEVS + 1];  /*  For export  */
	devinfo		dd_DI[MAXDEVS];	/* Device-specific stuff	*/
} driverdata;


#endif	/*  __DRIVER_H  */
