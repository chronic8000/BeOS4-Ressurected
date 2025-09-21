/* :ts=8 bk=0
 *
 * driver.h:	Definitions common to the kernel driver.
 *
 * Leo L. Schwab					2000.07.06
 */
#ifndef	__DRIVER_H
#define	__DRIVER_H

#ifndef	_PCI_H
#include <drivers/PCI.h>
#endif
#ifndef	_LYNXEM_H
#include <graphics_p/lynxem/lynxem.h>
#endif


/****************************************************************************
 * #defines
 */
#define	get_pci(pci,o,s)	(*gpci_bus->read_pci_config)((pci)->bus, \
							     (pci)->device, \
							     (pci)->function, \
							     (o), (s))
#define	set_pci(pci,o,s,v)	(*gpci_bus->write_pci_config)((pci)->bus, \
							      (pci)->device, \
							      (pci)->function, \
							      (o), (s), (v))

#define inb(a)			(*gpci_bus->read_io_8)(a)
#define outb(a,b)		(*gpci_bus->write_io_8)((a), (b))
#define inw(a)			(*gpci_bus->read_io_16)(a)
#define outw(a,b)		(*gpci_bus->write_io_16)((a), (b))


#define	MAXDEVS			4

#define	VENDORID_SMI		0x126f
#define	DEVICEID_LYNXEM		0x0000	/*  Whatzit really?  */
#define	DEVICEID_LYNXEMPLUS	0x0712

#define	SIZEOF_CURSORPAGE	(4 << 10)	/*  4K  */


/****************************************************************************
 * Structure definitions.
 */
typedef struct devinfo {
	pci_info	di_PCI;

	/*  HW Mappings  */
	area_id		di_BaseAddr0ID;
	area_id		di_DPRegsID;
	area_id		di_VPRegsID;
	area_id		di_VGARegsID;

	/*  Allocated data structures  */
	area_id		di_GfxCardInfo_AreaID;
	area_id		di_GfxCardCtl_AreaID;

	gfx_card_info	*di_GfxCardInfo;
	gfx_card_ctl	*di_GfxCardCtl;

	display_mode	*di_DispModes;
	uint32		di_NDispModes;

	uint32		di_Opened;
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
