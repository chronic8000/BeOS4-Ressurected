/* ++++++++++
	fx_driver.c

This is a small driver to return information on the PCI bus and allow
an app to find memory mappings for devices. The only really complicated
part of this driver is the code to switch back to VGA mode in uninit,
largely this code is taken from the Glide Sources

+++++ */

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdlib.h>
#include <string.h>
#include <graphic_driver.h>
#include <3dfx/sst1/3dfx_driver.h>
#include "3dfx_private.h"

/*------------------------------------------------------------------------*/

//#define THDFX_DEBUG
#if 0//def	THDFX_DEBUG
#define ddprintf dprintf
#else
#define ddprintf
#endif

#define THDFX_VID 0x121A
#define THDFX_DID 0x0001
#define THDFX2_DID 0x0002

#define MAX_PCI_DEVICES 32
#define MAX_CARDS	2

#define inb(p) (*pci->read_io_8)(p)
#define outb(p,v) (*pci->write_io_8)(p,v)

#define DEVICE_NAME 		"3dfx/"
#define NUM_OPENS			10		  // Max. number concurrent opens for device
#define GET_NTH_PCI_INFO	(*pci->get_nth_pci_info)

/*
 * GRAPHICS_DEVICE_PATH will be set to non-NULL in the Makefile for
 * debug/development builds.
 */
#ifndef	GRAPHICS_DEVICE_PATH
#define	GRAPHICS_DEVICE_PATH	""
#endif

typedef struct
{
	uint32 id;						  /* card number  */
	uint32 *thdfx;					  /* pointer to 3dfx h/w */
	uint32 deviceid;
} priv_data;

_EXPORT status_t init_driver (void);
_EXPORT void uninit_driver (void);
_EXPORT const char **publish_devices (void);
_EXPORT device_hooks *find_device (const char *);

static long fx_open (const char *name, ulong flags, void **cookie);
static long fx_close (void *cookie);
static long fx_free (void *cookie);
static long fx_control (void *cookie, ulong msg, void *buf, size_t len);
static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes);
static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes);

uint32 guard_b[65536];

static pci_module_info *pci;
static char pci_name[] = B_PCI_MODULE_NAME;

static int32 num_cards;
static uint32 p6FenceVar;

typedef struct sst1_device_rec
{
	pci_info pciinfo;
	area_id area;
	area_id wc_area;
	area_id uc_area;
	uint32 *base;
	char name[256];
	int32 open_count;
	long open_lock;
	uint32 slot;
	uint32 *linear_address;
} sst1_device;

static sst1_device _V2_Devices[MAX_CARDS+1];
static char *_V2_Name[MAX_CARDS+1];

uint32 guard_e[65536];

static device_hooks thdfx_device_hooks = {
	fx_open,
	fx_close,
	fx_free,
	fx_control,
	fx_read,
	fx_write
};

void resetPassThru (int board_num);

/*------------------------------------------------------------------------*/

static void init_guard()
{
	memset( guard_b, 0xcc, sizeof( guard_b ));
	memset( guard_e, 0xcc, sizeof( guard_e ));
}

static void check_guard()
{
	int32 ct;
	for( ct=0; ct<65536; ct++ )
	{
		if( guard_b[ct] != 0xcccccccc )
			kprintf( "Damaged guard_b at %ld   value=%08x \n", ct, guard_b[ct] );
		if( guard_e[ct] != 0xcccccccc )
			kprintf( "Damaged guard_e at %ld   value=%08x \n", ct, guard_e[ct] );
	}
}


static cpu_status enter_crit (spinlock * lock)
{
	cpu_status status;
	status = disable_interrupts ();
	acquire_spinlock (lock);
	return status;
}

/*------------------------------------------------------------------------*/

static void exit_crit (spinlock * lock, cpu_status status)
{
	release_spinlock (lock);
	restore_interrupts (status);
}

/*------------------------------------------------------------------------*/

status_t init_driver (void)
{
	int32 i, len;

	char devStr[1024];

	ddprintf ("3dfx_sst1: init_driver - Enter - PCI 3dfx-driver: %s %s\n", __DATE__, __TIME__);
	init_guard();

	if (get_module (pci_name, (module_info **) & pci))
		return ENOSYS;

	memset( _V2_Name, 0, sizeof( _V2_Name ));
	memset( _V2_Devices, 0, sizeof( _V2_Devices ));
	for (i = 0; i < MAX_CARDS+1; i++)
	{
		_V2_Devices[i].area = -1;
		_V2_Devices[i].wc_area = -1;
		_V2_Devices[i].uc_area = -1;
		_V2_Devices[i].open_count = NUM_OPENS;
	}
	
	num_cards = 0;
	for (i = 0;; i++)
	{
		sst1_device *dev = &_V2_Devices[num_cards];
		if (GET_NTH_PCI_INFO (i, &dev->pciinfo) != B_NO_ERROR)
		{
			break;
		}
		if ((dev->pciinfo.vendor_id == THDFX_VID) &&
			 ((dev->pciinfo.device_id == THDFX_DID) ||
			  (dev->pciinfo.device_id == THDFX2_DID)))
		{
			ddprintf ("3dfx_sst1: init_driver i=%ld  num_cards=%ld \n", i, num_cards );
			/* map the Voodoo Memory */
			/* First we try and map with Write Combining enabled - this may fail on some
			 * Systems (notable original Pentium & AMD Clone CPUs) if it does we try again
			 * without write combining
			 */
			dev->area = map_physical_memory ("3dfx Area",
						(void *) (dev->pciinfo.u.h0.base_registers[0]),
						dev->pciinfo.u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS,
						B_READ_AREA + B_WRITE_AREA, (void *) &dev->base);

			if ( dev->area <= B_ERROR )
			{
				ddprintf("3dfx_sst1: init_driver - Problems mapping /dev/3dfx_voodoo/%d device registers\n", num_cards);
				return B_ERROR;
			}

			/* We also calculate and store the linear address range associated with this
			 * card. This is so we can access the card ourselves in the uninit routine to
			 * toggle the VGA Passthru
			 */
			{
				area_info info;
				get_area_info (dev->area, &info);
				dev->linear_address = (uint32 *) info.address;
				ddprintf ("3dfx_sst1: Linear Address is 0x%x\n", dev->linear_address);
			}

			/* correct for offset within page */
			dev->base += (dev->pciinfo.u.h0.base_registers[0] & (B_PAGE_SIZE - 1)) / sizeof (uint32);

			/* add this card's name to driver's name list */
			_V2_Name[num_cards] = dev->name;
			dev->slot = i;

			sprintf ( dev->name, "graphics%s/%04X_%04X_%02X%02X%02X",
						GRAPHICS_DEVICE_PATH, dev->pciinfo.vendor_id, dev->pciinfo.device_id,
						dev->pciinfo.bus, dev->pciinfo.device, dev->pciinfo.function);

			ddprintf ("3dfx_sst1: init_driver - /dev/%s: slot num: %d base address: %08x\n", dev->name, dev->slot, dev->base);

			num_cards++;
			if (num_cards == MAX_CARDS)
			{
				ddprintf ("3dfx_sst1: Maximum number 3dfx cards installed\n");
				break;
			}
		}
	}

	if (num_cards == 0)
	{
		ddprintf ("3dfx_sst1: No 3dfx cards installed\n");
		return B_ERROR;
	}
	else
	{
		ddprintf ("3dfx_sst1: %d 3dfx card(s) installed\n", num_cards);
		return B_NO_ERROR;
	}
}

static long fx_open (const char *name, ulong flags, void **cookie)
{
	int card;
	priv_data *pd;
	cpu_status status;
	sst1_device *dev;

	ddprintf ("3dfx_sst1: fx_open - ENTER, name is %s\n", name);

	/* Error if no cards found */
	if (num_cards == 0)
	{
		ddprintf ("3dfx_sst1: fx_open - No Cards Installed\n");
		return B_ERROR;
	}
	/* do we have a match for name? */
	for (card = 0; card < num_cards; card++)
	{
		dev = &_V2_Devices[card];
		if (strcmp (name, dev->name) == 0)
			break;
	}

	/* error if not found */
	if (card == num_cards)
	{
		ddprintf ("3dfx_sst1: fx_open - Did not find a match for %s\n", name);
		return B_ERROR;
	}
	
	/* critical section */
	status = enter_crit (&dev->open_lock);

	/* ensure we don't open the driver too many times */
	if (dev->open_count < 1)
	{
		exit_crit (&dev->open_lock, status);
		ddprintf ("3dfx_sst1: fx_open - Open count for card[%d] < 1\n", card);
		return B_ERROR;
	}

	/* allocate some private storage */
	pd = (priv_data *) calloc (1, sizeof (*pd));
	*cookie = (void *) pd;

	/* initialize it */
	pd->id = card;
	pd->thdfx = dev->base;
	pd->deviceid = dev->pciinfo.device_id;

	dev->open_count -= 1;

	ddprintf ("3dfx_sst1: fx_open - successful open of /dev/%s\n", dev->name);
	exit_crit (&dev->open_lock, status);
	return B_NO_ERROR;

 err_exit:
	free (cookie);
	ddprintf ("3dfx_sst1: fx_open - error opening /dev/%s\n", dev->name);
	exit_crit (&dev->open_lock, status);
	return B_ERROR;
}

static long fx_close (void *_cookie)
{
	ddprintf ("3dfx_sst1: fx_close\n");

	return B_NO_ERROR;
}

static long fx_free (void *cookie)
{
	priv_data *pd;

	ddprintf ("3dfx_sst1: fx_free - ENTER\n");

	pd = (priv_data *) cookie;
	/* Switch back the VGA passthru on Voodoo Cards,
	 * We do this here to ensure that programs which crash
	 * or otherwise exit abnormally return the screen to where the
	 * user can make use of it
	 */
	ddprintf ("3dfx_sst1: fx_free - calling resetPassThru(%d)\n", pd->id);
	snooze (10000);
	resetPassThru (pd->id);
	ddprintf ("3dfx_sst1: fx_free - EXIT\n");
	return B_NO_ERROR;
}

static long fx_read (void *cookie, off_t pos, void *buf, size_t * nbytes)
{
	ddprintf ("3dfx_sst1: fx_read - ENTER\n");
	return 0;
}

static long fx_write (void *cookie, off_t pos, const void *buf, size_t * nbytes)
{
	ddprintf ("3dfx_sst1: fx_write - ENTER\n");
	return 0;
}

static long fx_control (void *_cookie, ulong msg, void *buf, size_t len)
{
	priv_data *pd;
	area_id t;
	void *temp_base;
	sst1_device *dev;

	//ddprintf ("3dfx_sst1: fx_control - ENTER\n");

check_guard();
	pd = (priv_data *) _cookie;
	dev = &_V2_Devices[pd->id];

	switch (msg)
	{
	case B_GET_3D_SIGNATURE:
		ddprintf ("3dfx_sst1: fx_control(%d) - B_GET_3D_SIGNATURE\n", pd->id);
		strcpy ((char *) buf, "_CPU_libVoodoo2.so");
		break;
	case THDFX_IOCTL_GET_AREA_ID:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - area address = 0x%x\n", pd->id, dev->area);
			*(area_id *) buf = dev->area;
		}
		break;
	case THDFX_IOCTL_GET_PHYSICAL:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - physical address = 0x%x\n", pd->id, dev->base);
			*(uint32 **) buf = dev->base;
		}
		break;
	case THDFX_IOCTL_GET_SLOTNUM:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - slot number= %d\n", pd->id, dev->slot);
			*(int32 *) buf = dev->slot;
		}
		break;
	case THDFX_IOCTL_SET_UC:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - Set Uncacheable, Base = 0x%x, size = 0x%x\n", pd->id, dev->base, *(uint32 *) buf);
			dev->uc_area = map_physical_memory ("3dfx Area (UC)",
											(void *) (dev->pciinfo.u.h0.base_registers[0]), *(uint32 *) buf,
											B_ANY_KERNEL_ADDRESS | B_MTR_UC, B_READ_AREA + B_WRITE_AREA,
											(void *) &temp_base);
			if (dev->uc_area == 0)
			{
				ddprintf ("3dfx_sst1: fx_control(%d) - Set Uncacheable failed !\n");
				return B_ERROR;
			}
		}
		break;
	case THDFX_IOCTL_SET_WC:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - Set WriteCombining, Base = 0x%x, size = 0x%x\n",
						 pd->id, dev->base, *(uint32 *) buf);
			dev->wc_area = map_physical_memory ("3dfx Area (WC)",
											(void *) (dev->pciinfo.u.h0.base_registers[0]), *(uint32 *) buf,
											B_MTR_WC | B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA + B_WRITE_AREA,
											(void *) &temp_base);
			ddprintf ("returned \n");
			if (dev->wc_area == 0)
			{
				ddprintf ("3dfx_sst1: fx_control(%d) - Set WriteCombining failed !\n");
				return B_ERROR;
			}
		}
		break;
	case THDFX_IOCTL_GET_CARD_ADDR:
		{
			ddprintf ("3dfx_sst1: fx_control(%d) - card address = 0x%x\n", pd->id,
						 dev->pciinfo.u.h0.base_registers[0]);
			*(uint32 *) buf = dev->pciinfo.u.h0.base_registers[0];
		}
		break;
	case THDFX_IOCTL_WRITE_PCI_REG:
		ddprintf ("3dfx_sst1: fx_control(%d) - write register 0x%x with value 0x%x\n", pd->id,
					 ((thdfx_access_pci_reg *) buf)->reg, ((thdfx_access_pci_reg *) buf)->value);
		pci->write_pci_config (dev->pciinfo.bus,	/* bus number */
									  dev->pciinfo.device,	/* device # on bus */
									  dev->pciinfo.function,	/* function # in device */
									  ((thdfx_access_pci_reg *) buf)->reg,	/* offset in configuration space */
									  4, /* # bytes to write (1, 2 or 4) */
									  ((thdfx_access_pci_reg *) buf)->value	/* value to write */
			);
		break;
	case THDFX_IOCTL_READ_PCI_REG:
		ddprintf ("3dfx_sst1: fx_control(%d) - read register 0x%x\n", pd->id,
					 ((thdfx_access_pci_reg *) buf)->reg);
		((thdfx_access_pci_reg *) buf)->value = pci->read_pci_config (dev->pciinfo.bus,	/* bus number */
																						  dev->pciinfo.device,	/* device # on bus */
																						  dev->pciinfo.function,	/* function # in device */
																						  ((thdfx_access_pci_reg *) buf)->reg,	/* offset in configuration space */
																						  4	/* # bytes to write (1, 2 or 4) */
			);
		ddprintf ("3dfx_sst1: fx_control(%d) - read register 0x%x, value = 0x%x\n", pd->id,
					 ((thdfx_access_pci_reg *) buf)->reg, ((thdfx_access_pci_reg *) buf)->value);
		break;

	default:
		return B_ERROR;
		break;
	}

	//ddprintf ("3dfx_sst1: fx_control - EXIT\n");
	return B_NO_ERROR;
}

device_hooks *find_device (const char *name)
{
	int index = 0;

	ddprintf ("3dfx_sst1: find_device - Enter name=%s\n", name);
	return &thdfx_device_hooks;
}

/*
** sst1InitWrite32():
**  Write 32-bit Word to specified address
**
*/
void sst1InitWrite32 (FxU32 * addr, FxU32 data)
{
	P6FENCE;
	SET (addr, data);
	P6FENCE;
}

/*
** sst1InitRead32():
**  Read 32-bit Word from specified address
**
*/
FxU32 sst1InitRead32 (FxU32 * addr)
{
	P6FENCE;
	return GET (addr);
}

// Included so compiler doesn't optimize out loop code waiting on status bits
FxU32 sst1InitReturnStatus (FxU32 * sstbase)
{
	SstRegs *sst = (SstRegs *) sstbase;

	return (IGET (sst->status));
}

/*
** sst1InitIdleFBINoNOP():
**  Return idle condition of FBI (ignoring idle status of TMU)
**  sst1InitIdleFBINoNOP() differs from sst1InitIdleFBI() in that no NOP command
**  is issued to flush the graphics pipeline.
**
**    Returns:
**      FXTRUE if FBI is idle (fifos are empty, graphics engines are idle)
**      FXFALSE if FBI has not been mapped
**
*/
FxBool sst1InitIdleFBINoNOP (FxU32 * sstbase)
{
	FxU32 cntr, l;
	SstRegs *sst = (SstRegs *) sstbase;

	if (!sst)
		return (FXFALSE);

	ISET (sst->nopCMD, 0x0);
	cntr = 0;
	l = 0;
	while (1)
	{
		l++;
		if (!(sst1InitReturnStatus (sstbase) & SST_FBI_BUSY))
		{
			if (++cntr > 5)
				break;
		}
		else
			cntr = 0;
		if (l > 1000)
			dprintf ("sst1InitIdleFBINoNOP - stuck in BUSY loop\n");
	}
	return (FXTRUE);
}

/*
** sst1InitVgaPassCtrl():
**  Control VGA passthrough setting
**
**
*/
FxBool sst1InitVgaPassCtrl (FxU32 * sstbase, FxU32 enable)
{
	SstRegs *sst = (SstRegs *) sstbase;

//    if(sst1InitCheckBoard(sstbase) == FXFALSE)
//        return(FXFALSE);

	ddprintf ("3dfx_sst1: sst1InitVgaPassCtrl - ENTER\n");
	if (enable)
	{
		// VGA controls monitor
		ddprintf ("3dfx_sst1: sst1InitVgaPassCtrl - Switching monitor to VGA, sst=0x%x\n", sst);
		ISET (sst->fbiInit0, (IGET (sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 0x0);
		ISET (sst->fbiInit1, IGET (sst->fbiInit1) | SST_VIDEO_BLANK_EN);
	}
	else
	{
		// SST-1 controls monitor
		ddprintf ("3dfx_sst1: sst1InitVgaPassCtrl - Switching monitor to SST-1, sst=0x%x\n", sst);
		ISET (sst->fbiInit0, (IGET (sst->fbiInit0) & ~SST_EN_VGA_PASSTHRU) | SST_EN_VGA_PASSTHRU);
		ISET (sst->fbiInit1, IGET (sst->fbiInit1) & ~SST_VIDEO_BLANK_EN);
	}

	ddprintf ("3dfx_sst1: sst1InitVgaPassCtrl - EXIT\n");
	return (FXTRUE);
}

void resetPassThru (int board_num)
{
	// This code is derived from code contained in sst1InitShutdown from sst1init.c
	SstRegs *sstPtr;
	FxU32 n = board_num;
	sst1_device *dev = &_V2_Devices[board_num];

	ddprintf ("3dfx_sst1: resetPassThru - ENTER\n");

	sstPtr = (SstRegs *) dev->linear_address;
	if (sstPtr == NULL)
	{
		ddprintf ("3dfx_sst1: resetPassThru - EXIT\n");
		return;
	}
	// Reset video unit to guarantee no contentions on the memory bus
	// Blank DAC so VGA Passthru works properly
	ddprintf ("3dfx_sst1: resetPassThru - Reseting video unit for %d\n", n);
	ISET (sstPtr->fbiInit1, IGET (sstPtr->fbiInit1) | (SST_VIDEO_RESET | SST_VIDEO_BLANK_EN));
	// Turn off dram refresh to guarantee no contentions on the
	// memory bus
	ddprintf ("3dfx_sst1: resetPassThru - Turn off dram refresh for %d\n", n);
	ISET (sstPtr->fbiInit2, IGET (sstPtr->fbiInit2) & ~SST_EN_DRAM_REFRESH);

	// Reset graphics subsystem
	ddprintf ("3dfx_sst1: resetPassThru - Reset graphics subsystem for %d\n", n);
	ISET (sstPtr->fbiInit0, IGET (sstPtr->fbiInit0) | (SST_GRX_RESET | SST_PCI_FIFO_RESET));
	sst1InitIdleFBINoNOP ((FxU32 *) sstPtr);
	ISET (sstPtr->fbiInit0, IGET (sstPtr->fbiInit0) & ~SST_PCI_FIFO_RESET);

	sst1InitIdleFBINoNOP ((FxU32 *) sstPtr);
	ISET (sstPtr->fbiInit0, IGET (sstPtr->fbiInit0) & ~SST_GRX_RESET);
	sst1InitIdleFBINoNOP ((FxU32 *) sstPtr);

	// Turnaround VGA_PASS to allow VGA monitor
	ddprintf ("3dfx_sst1: resetPassThru - Calling sst1InitVgaPassCtrl() for %d\n", n);
	sst1InitVgaPassCtrl ((FxU32 *) sstPtr, 1);
	sst1InitIdleFBINoNOP ((FxU32 *) sstPtr);

	// Set clock at 30 MHz to reduce power consumption...
	//    sst1InitComputeClkParams((float) 30.0, &sstGrxClk);
	//    if(sst1InitSetGrxClk((FxU32 *) sstPtr, &sstGrxClk) == FXFALSE)
	//      INIT_PRINTF(("sst1InitShutdown() WARNING: sst1InitSetGrxClk failed...Continuing...\n"));
	//    sst1CurrentBoard->initGrxClkDone = 0;
	ddprintf ("3dfx_sst1: resetPassThru - EXIT\n");
}

void uninit_driver (void)
{
	int i;

	ddprintf ("3dfx_sst1: uninit_driver - Enter\n");

	for (i = 0; i < num_cards; i++)
	{
		sst1_device *dev = &_V2_Devices[i];
		if (dev->area != -1)
			delete_area (dev->area);
		if (dev->wc_area != -1)
			delete_area (dev->wc_area);
		if (dev->uc_area != -1)
			delete_area (dev->uc_area);
	}
	
	put_module(pci_name);

	ddprintf ("3dfx_sst1: uninit_driver - Exit\n");
}

const char **publish_devices (void)
{
	int32 i;
	/* return the list of supported devices */
	ddprintf ("3dfx_sst1: publish_devices\n");
	
	for( i=0; i<4; i++ )
	{
		if( _V2_Name[i] )
			ddprintf ("3dfx_sst1: dev=%ld = '%s' \n", i, _V2_Name[i] );
		else
			ddprintf ("3dfx_sst1: dev=%ld = NULL \n", i );
	}
	
	return (const char **) _V2_Name;
}
