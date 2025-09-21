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
#ifndef	_I810_H
#include <graphics_p/i810/i810.h>
#endif


/****************************************************************************
 * Driver Implementation Notes:
 *
 * Maybe I'm getting old, or maybe this driver has exceeded the complexity
 * of Disney Animation Studio (a project where I was able to keep everything
 * in my head).  Whatever the case, I find I have to write all this down to
 * remember what the hell I'm doing.
 *
 * The driver's data is organized as follows:
 *
 *             +-----------------+
 *             | driverdata  gdd | driver global data
 *             +-----------------+
 *                      |
 * +----------------------------------------+  per-device data components (3):
 * | devinfo        di                      |   device private data
 * | gfx_card_info  di->di_GfxCardInfo (ci) |   read-only device public data
 * | gfx_card_ctl   di->di_GfxCardCtl  (cc) |   writable device public data
 * +----------------------------------------+
 *
 * The number of devinfos per driver is currently hard-coded at four, but
 * there's no reason this can't be dynamic.
 *
 * Each opener of a device gets an openerinfo, which contains a pointer to a
 * devinfo.  There's no reason why gfx_card_ctl can't be different for every
 * opener.  (In fact, there's no reason why gfx_card_ctl can't in fact be the
 * openerinfo.  Must explore this idea...)
 *
 * Each opener can set a different display mode; the guy to most recently
 * set the display mode "wins" (yes, this is potentially chaotic; more
 * complete, forward-thinking architecture is required here).  When he dies
 * or otherwise closes the device, the next most recent mode set is restored.
 * This is managed via openerinfo.oi_Node and gfx_card_info.ci_OpenerList.
 */


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

#define	VENDORID_INTEL		0x8086	/*  Of course.  */
#define	DEVICEID_I810		0x7121
#define	DEVICEID_I810_DC100	0x7123
#define	DEVICEID_I810E		0x7125
#define	DEVICEID_I815_FSB100	0x1102
#define	DEVICEID_I815_NOAGP	0x1112
#define	DEVICEID_I815		0x1132

#define	SIZEOF_CURSORPAGE	(4 << 10)	/*  4K  */


/****************************************************************************
 * Structure definitions.
 */
typedef struct devinfo {
	pci_info	di_PCI;

	/*  HW Mappings  */
	area_id		di_BaseAddr0ID;
	area_id		di_BaseAddr1ID;
	area_id		di_CursorBaseUCID;
	area_id		di_HWStatusPageUCID;

	/*  Allocated data structures  */
	area_id		di_GfxCardInfo_AreaID;
	area_id		di_GfxCardCtl_AreaID;
	area_id		di_HWStatusPage_AreaID;
	area_id		di_GTT_AreaID;
	area_id		di_CursorBase_AreaID;
	area_id		di_ExtraBase_AreaID;

	gfx_card_info	*di_GfxCardInfo;/*  Public read-only		*/
	gfx_card_ctl	*di_GfxCardCtl;	/*  Public read/write		*/
	vuint32		*di_ExtraBase;	/*  Extra system RAM for FB	*/
	uint32		*di_GTT;	/*  Never write through here	*/
	uint32		di_HWStatusPage_DMA;
	uint32		di_CursorBase_DMA;
	uint32		di_TakenBase_DMA;	/*  "Taken" gfx sys RAM	*/
	uint32		di_TakenSize;	/*  Sys RAM "taken" for gfx	*/
	uint32		di_ExtraSize;	/*  Extra sys RAM for gfx	*/
	uint32		di_CacheSize;	/*  Size of optional gfx cache	*/
	uint16		di_GTTSize;	/*  In **32-bit entries**	*/
	uint16		di_FSBFreq;	/*  Front-side bus freq. (MHz)	*/

	display_mode	*di_DispModes;
	uint32		di_NDispModes;

	uint32		di_Opened;
	uint32		di_NInterrupts;	// DEBUG
	uint32		di_NVBlanks;	// DEBUG
	uint32		di_LastIrqMask;	// DEBUG
	char		di_Name[B_OS_NAME_LENGTH];
} devinfo;

typedef struct openerinfo {
	Node		oi_Node;
	devinfo		*oi_DI;
	dispconfig	oi_DC;
	uint32		oi_GenPoolOwnerID;
} openerinfo;

typedef struct driverdata {
	sem_id		dd_DriverLock;	/*  Driver-wide semaphore	*/
	uint32		dd_NDevs;	/*  # of devices found		*/
	uint32		dd_NInterrupts;	/*  # of IRQs from this device	*/
	char		*dd_DevNames[MAXDEVS + 1];  /*  For export  */
	devinfo		dd_DI[MAXDEVS];	/* Device-specific stuff	*/
} driverdata;


#endif	/*  __DRIVER_H  */
