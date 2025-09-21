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
#ifndef	__NVIDIA_H
#include <graphics_p/nvidia/nvidia.h>
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

#define	VENDORID_NVIDIA		0x10de
#define	VENDORID_NVIDIA_SGS	0x12d2

#define	DEVICEID_RIVA128	0x0018
#define	DEVICEID_TNT		0x0020
#define	DEVICEID_TNT2		0x0028
#define	DEVICEID_UTNT2		0x0029
#define	DEVICEID_VTNT2		0x002C
#define	DEVICEID_UVTNT2		0x002D
#define	DEVICEID_TNT2_A		0x002E
#define	DEVICEID_TNT2_B		0x002F
#define	DEVICEID_ITNT2		0x00A0
#define	DEVICEID_GEFORCE256     0x0100
#define	DEVICEID_GEFORCEDDR     0x0101
#define	DEVICEID_QUADRO		0x0103
#define	DEVICEID_GEFORCE2MX	0x0110
#define	DEVICEID_GEFORCE2MX2	0x0111
#define	DEVICEID_GEFORCE2GO	0x0112
#define	DEVICEID_QUADRO2MXR	0x0113
#define	DEVICEID_GEFORCE2GTS	0x0150
#define	DEVICEID_GEFORCE2GTS2	0x0151
#define	DEVICEID_GEFORCE2ULTRA	0x0152
#define	DEVICEID_QUADRO2PRO	0x0153
#define	DEVICEID_GEFORCE3	0x0200
#define	DEVICEID_GEFORCE3_1	0x0201
#define	DEVICEID_GEFORCE3_2	0x0202
#define	DEVICEID_GEFORCE3_3	0x0203


#define	SIZEOF_CURSORPAGE	2048


/****************************************************************************
 * Structure definitions.
 */
struct devinfo;		/*  Shaddup you stupid pile of $#!+  */

typedef struct supportedcard {
	uint16		sc_VendorID;
	uint16		sc_DeviceID;
	void		(*sc_Init) (struct devinfo *di);
	const char	*sc_DevEntry;	/*  "nv3", "nv4", "nv10"	*/
	const char	*sc_Name;
	const char	*sc_ChipSet;
} supportedcard;

typedef struct devinfo {
	pci_info	di_PCI;
	const supportedcard	*di_SC;

	/*  HW Mappings  */
	area_id		di_BaseAddr0ID;
	area_id		di_BaseAddr1ID;

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
