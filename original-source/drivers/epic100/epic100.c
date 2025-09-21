/*
 * Epic100 Network Card Support - PCI x86
 * Copyright (c) 2001 Be Incorporated
 * Written by Chris Liscio <liscio@be.com>
 */

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <stdarg.h>
#include <PCI.h>
#include <fc_lock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include "ether_driver.h"

/* PCI vendor and device ID's specific to the Epic100 */
#define VENDOR_ID        0x10B8
#define DEVICE_ID        0x0005

/* globals for driver instances */ 

#define kDevName "epic100"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64
#define MAX_CARDS 			 4			/* maximum number of driver instances */

#define write8(address, value)			(*gPCIModInfo->write_io_8)((address), (value))
#define write16(address, value)			(*gPCIModInfo->write_io_16)((address), (value))
#define write32(address, value)			(*gPCIModInfo->write_io_32)((address), (value))
#define read8(address)					((*gPCIModInfo->read_io_8)(address))
#define read16(address)					((*gPCIModInfo->read_io_16)(address))
#define read32(address)					((*gPCIModInfo->read_io_32)(address))

/* debug flags */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define PCI_IO    0x0040		/* pci reads and writes */
#define SEQ		  0x0080		/* trasnmit & receive TCP/IP sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off on final release */

/* Hardware specific definitions */
/* ring buffer sizes  */
#define TX_BUFFERS             16
#define RX_BUFFERS             32        /* Must be a power of 2 */

/* Bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256		/* Rounded down to 4 byte units. */
#define RX_FIFO_THRESH 1		/* 0-3, 0==32, 64,96, or 3==128 bytes  */

#define ETH_ZLEN			60				/* Min. octets in frame */
#define PKT_BUF_SZ			1536
#define BUFFER_SIZE			2048L           /* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */

/* Offsets to registers, using the SMC names */
#define EPIC_COMMAND 		0x00
#define EPIC_INTSTAT 		0x04
#define EPIC_INTMASK 		0x08
#define EPIC_GENCTL  		0x0C
#define EPIC_NVCTL   		0x10
#define EPIC_EECTL   		0x14
#define EPIC_PCIBURSTCNT 	0x18
#define EPIC_TEST1     		0x1C
#define EPIC_CRCCNT    		0x20
#define EPIC_ALICNT    		0x24
#define EPIC_MPCNT     		0x28
#define EPIC_MIICTRL   		0x30
#define EPIC_MIIDATA   		0x34
#define EPIC_MIICFG    		0x38
#define EPIC_LAN0        	0x40  /* MAC Address */
#define EPIC_MC0         	0x50  /* Multicast filter table */
#define EPIC_RXCTRL      	0x60
#define EPIC_TXCTRL     	0x70
#define EPIC_TXSTAT    		0x74
#define EPIC_PRXCDAR   		0x84
#define EPIC_RXSTAT    		0xA4
#define EPIC_EARLYRX   		0xB0
#define EPIC_PTXCDAR   		0xC4
#define EPIC_TXTHRESH  		0xDC

/* Interrupt Register Bits */
#define EPIC_TXIDLE			0x40000
#define EPIC_RXIDLE			0x20000
#define EPIC_INTRSUMMARY	0x10000
#define EPIC_PCIBUSERR170   0x07000
#define EPIC_PCIBUSERR175	0x01000
#define EPIC_PHYEVENT175	0x08000
#define EPIC_RXSTARTED		0x00800
#define EPIC_RXEARLYWARN	0x00400
#define EPIC_CNTFULL		0x00200
#define EPIC_TXUNDERRUN		0x00100
#define EPIC_TXEMPTY		0x00080
#define EPIC_TXDONE			0x00020
#define EPIC_RXERROR		0x00010
#define EPIC_RXOVERFLOW		0x00008
#define EPIC_RXFULL			0x00004
#define EPIC_RXHEADER		0x00002
#define EPIC_RXDONE			0x00001

/* Serial EEPROM section */

/* EEPROM_Ctrl bits */
#define EE_SHIFT_CLK	0x04
#define EE_CS			0x02
#define EE_DATA_WRITE	0x08
#define EE_WRITE_0		0x01
#define EE_WRITE_1		0x09
#define EE_DATA_READ	0x10
#define EE_ENB			(0x0001 | EE_CS)

/* delay between EEPROM clock transitions */
#define eeprom_delay(nanosec) do { ; } while (0)

/* The EEPROM commands include the always-set leading bit */
#define EE_WRITE_CMD	(5 << 6)
#define EE_READ64_CMD	(6 << 6)
#define EE_READ256_CMD	(6 << 8)
#define EE_ERASE_CMD	(7 << 6)

/* MII Defines */
#define MII_READOP		1
#define MII_WRITEOP		2

/* diagnostic debug flags - compile in here or set while running with debugger "EPIC100" command */
#define DEBUG_FLAGS ( ERR )

/* a little support function define */
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

/* for serial debug command...uncomment this if you want to use it. */

/* Some globals...yucky, but necessary...*/
static pci_module_info		*gPCIModInfo;
static fc_lock_module_info	*gFC;
static char 				*gDevNameList[MAX_CARDS+1];
static pci_info 			*gDevList[MAX_CARDS+1];
static int32 				gOpenMask = 0;

const char** publish_devices(void ) {
	dprintf(kDevName ": publish_devices()\n" );
	dprintf(*gDevNameList);
	dprintf("\n");
	return (const char **)gDevNameList;
}

/* Driver Entry Points */
__declspec(dllexport) status_t init_hardware(void );
__declspec(dllexport) status_t init_driver(void );
__declspec(dllexport) void uninit_driver(void );
__declspec(dllexport) const char** publish_devices(void );
__declspec(dllexport) device_hooks *find_device(const char *name );

typedef struct _EPIC_TX_DESC EPIC_TX_DESC;
typedef struct _EPIC_RX_DESC EPIC_RX_DESC;

/* Transmit and receive frame descriptor */
typedef struct frame_descriptor {
    uint32 reserved;						/* hardware specfic frame descriptor */
} frame_desc_t;

struct _EPIC_TX_DESC {
	int16 			status;
	uint16 			txlength;
	uint32 			bufaddr;
	uint16 			buflength;
	uint16			control;
	uint32		 	next;
};
struct _EPIC_RX_DESC {
	int16 			status;
	uint16 			rxlength;
	uint32 			bufaddr;
	uint16 			buflength;
	uint16			control;
	uint32			next;
};

/* per driver intance globals */
typedef struct {
	ether_address_t mac_address;								/* my ethernet address */
	int             mc_count;                                   /* number of multicasts */
	uchar           mc_filter[8];  								/* multicast hash filter */
	int32			devID; 										/* device identifier: 0-n */
	uint32          flags;
	pci_info		*pciInfo;
	fc_lock			ilock, olock;								/* i/o fc_locks */
	int32			readLock, writeLock;						/* reentrant reads/writes not allowed */
	int32			blockFlag;									/* for blocking or nonblocking reads */
	area_id			tx_desc_area, tx_buf_area; 					/* transmit descriptor and buffer areas */
	area_id			rx_desc_area, rx_buf_area; 					/* receive descriptor and buffer areas */

	spinlock		rx_spinlock;								/* for read_hook() and rx_interrupt() */
	
	uchar			*tx_buf[TX_BUFFERS], *rx_buf[RX_BUFFERS];	/* tx and rx buffers */
	EPIC_TX_DESC	tx_desc[TX_BUFFERS];						/* tx frame descriptors */
	EPIC_RX_DESC    rx_desc[RX_BUFFERS];						/* rx frame descriptors */
	int				cur_tx, cur_rx;								/* current (last) transmit and receive slots */
	int				dirty_tx, dirty_rx;							/* dirty tx and rx indicators */
	int16			rx_received, rx_acked;						/* index to in use tx & rx ring buffers */
	int16			tx_sent, tx_acked;							/* index to oldest used tx & rx ring buffers */
	uint32			reg_base; 									/* Base address of hostRegs */
	uint32			media;										/* speed duplex media settings */
	char			phys[4];									/* MII device addresses */

	uint32			debug;										/* debuging level */
	/*  statistics */
	uint32			handled_int, unhandled_int;					/* interrupts rececived & handled */
	uint32			stats_rx, stats_tx;							/* receive & transmit frames counts */
	uint32			stats_rx_err, stats_tx_err;
	uint32			stats_rx_overrun, stats_tx_underrun;
	uint32			stats_collisions;  							/* error stats */

	uint32			tx_full;									/* transmit full indicator */
} dev_info_t;

#if DEBUG_VERSION
#define DEBUG( device, flags, format... ) if ( flags & device->debug ) dprintf( kDevName ": " format )
#define DEBUGGER_COMMAND 1
#else
#define DEBUG( device, flags, format... )
#endif

#if DEBUGGER_COMMAND
dev_info_t * gdev;
static int 		EPICEthernet(int argc, char **argv);					   /* serial debug command */
#endif

/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len);
static int32    interrupt_hook(void *_device);                	   /* interrupt handler */
static void 	set_rx_mode(dev_info_t *device);
static int32    get_pci_list(pci_info *info[], int32 maxEntries);  /* Get pci_info for each device */
static status_t free_pci_list(pci_info *info[]);                   /* Free storage used by pci_info list */
static status_t enable_addressing(dev_info_t *device);             /* enable pci io address space for device */
static status_t init_ring_buffers(dev_info_t *device);             /* allocate and initialize frame buffer rings */
static void     free_ring_buffers(dev_info_t *device);             /* allocate and initialize frame buffer rings */
static void     get_mac_address(dev_info_t *device);               /* get ethernet address */
static status_t setpromisc(dev_info_t * device);                   /* set hardware to receive all packets */
static status_t domulti(dev_info_t *device, uint8 *addr);		   /* add multicast address to hardware filter list */
static void     reset_device(dev_info_t *device);                  /* reset the lan controller (NIC) hardware */
static void 	dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
static void 	dump_rx_desc(dev_info_t *device);				   /* dump hardware receive descriptor structures */
static void 	dump_tx_desc(dev_info_t *device);				   /* dump hardware transmit descriptor structures */
static void		clear_statistics(dev_info_t *device);			   /* clear statistics counters */
static status_t allocate_resources(dev_info_t *device);     	   /* allocate fc_locks & spinlocks */
static void     free_resources(dev_info_t *device);                /* deallocate fc_locks & spinlocks */

static device_hooks gDeviceHooks =  {
	open_hook, 			/* -> open entry point */
	close_hook,         /* -> close entry point */
	free_hook,          /* -> free entry point */
	control_hook,       /* -> control entry point */
	read_hook,          /* -> read entry point */
	write_hook,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,                /* -> deselect entry point */
	NULL,               /* -> readv */
	NULL                /* -> writev */
};

static int read_eeprom(int32 ioBase, int location) {
	int i;
	int retval = 0;
	long ee_addr = ioBase + EPIC_EECTL;
	int read_cmd = location |
		(read32(ee_addr) & 0x40) ? EE_READ64_CMD : EE_READ256_CMD;
	dprintf(kDevName ": read_eeprom - ENTRY\n");
	write32(ee_addr, EE_ENB & ~EE_CS);
	write32(ee_addr, EE_ENB);
	/* Shift the read command bits out */
	for (i=12; i >= 0; i--)
	{
		short dataval = (read_cmd & (1 << i)) ? EE_WRITE_1 : EE_WRITE_0;
		write32(ee_addr, EE_ENB | dataval);
		eeprom_delay(100);
		write32(ee_addr, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay(150);
	}
	write32(ee_addr, EE_ENB);
	
	for (i = 16; i > 0; i--)
	{
		write32(ee_addr, EE_ENB | EE_SHIFT_CLK);
		eeprom_delay(100);
		retval = (retval << 1) | ((read32(ee_addr) & EE_DATA_READ) ? 1 : 0);
		write32(ee_addr, EE_ENB);
		eeprom_delay(100);
	}
	/* Terminate the EEPROM access */
	write32(ee_addr, EE_ENB & ~EE_CS);
	if (retval)
		dprintf(kDevName ": read_eeprom - EXIT (FAIL)\n");
	else
		dprintf(kDevName ": read_eeprom - EXIT (PASS)\n");
	return retval;
}

static int mdio_read(int32 ioBase, int phy_id, int location)
{
	int i;
	write32(ioBase + EPIC_MIICTRL, (phy_id << 9) | (location << 4) | MII_READOP);
	/* typical operation takes < 50 ticks */
	for (i=4000; i > 0; i--)
		if ((read32(ioBase + EPIC_MIICTRL) & MII_READOP) == 0)
			return read32(ioBase + EPIC_MIIDATA);
	return 0xFFFF;
}
static void mdio_write(int32 ioBase, int phy_id, int location, int value)
{
	int i;
	write32(ioBase + EPIC_MIIDATA, value);
	write32(ioBase + EPIC_MIICTRL, (phy_id << 9) | (location << 4) | MII_WRITEOP);
	for (i=10000; i > 0; i--)
	{
		if ((read32(ioBase + EPIC_MIICTRL) & MII_WRITEOP) == 0)
			break;
	}
	return;
}

/* Driver Entry Points */
status_t init_hardware(void )
{
	return B_NO_ERROR;
}

status_t init_driver()
{
	status_t 		status;
	int32			entries;
	char			devName[64];
	int32			i;
	
	dprintf(kDevName ": init_driver \n");	

	if((status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		dprintf(kDevName " Get module failed! %s\n", strerror(status ));
		return status;
	}

	if ((status = get_module( B_FC_LOCK_MODULE_NAME, (module_info **)&gFC )) != B_OK ) {
		dprintf(kDevName " Get fc_lock module failed! %s\n", strerror(status ));
		return status;
	}

	/* Find Lan cards*/
	if ((entries = get_pci_list(gDevList, MAX_CARDS )) == 0) {
		dprintf("init_driver: " kDevName " not found\n");
		free_pci_list(gDevList);
		put_module(B_PCI_MODULE_NAME );
		return B_ERROR;
	}
	
	/* Create device name list*/
	for (i=0; i<entries; i++ )
	{
		sprintf(devName, "%s%ld", kDevDir, i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
	}
	gDevNameList[i] = NULL;
	
	return B_OK;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;

	/*Free device name list*/
	for (i=0; (item=gDevNameList[i]); i++)
		free(item);
	
	/*Free device list*/
	free_pci_list(gDevList);
	put_module(B_PCI_MODULE_NAME);
}


device_hooks *find_device(const char *name)
{
	int32 	i;
	char 	*item;

	/* Find device name */
	for (i=0; (item=gDevNameList[i]); i++)
	{
		if (strcmp(name, item) == 0)
		{
			return &gDeviceHooks;
		}
	}
	return NULL; /*Device not found */
}


static status_t open_hook(const char *name, uint32 flags, void **cookie) {
	int32				devID;
	int32				mask;
	status_t			status;
	char 				*devName;
	dev_info_t 		    *device;
	int32               ioBase;
	int					mii_reg5;
	physical_entry		entry;

	(void)flags;
	
	/*	Find device name*/
	for (devID=0; (devName=gDevNameList[devID]); devID++) {
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;
	
	/* Check if the device is busy and set in-use flag if not */
	mask = 1 << devID;
	if (atomic_or(&gOpenMask, mask) &mask) {
		return B_BUSY;
	}

	/*	Allocate storage for the cookie*/
	if (!(*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));
	
	/* Setup the cookie */
	device->pciInfo = gDevList[devID];
	device->devID = devID;	
	device->debug = DEBUG_FLAGS;

	DEBUG(device, FUNCTION, kDevName ": open %s device=%p\n", name, device);
	
#if DEBUGGER_COMMAND
	gdev = device;
	add_debugger_command (kDevName, EPICEthernet, "Epic100 Ethernet driver Info");
#endif

	/* enable access to the cards address space */
	if ((status = enable_addressing(device)) != B_OK)
		goto err1;

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	ioBase = device->reg_base;
	
	clear_statistics(device);
	
	/* Init Device */
	/* reset_device(device); */
	write32(ioBase + EPIC_GENCTL, 0x4001);	

	/* Setup interrupts */
	install_io_interrupt_handler(device->pciInfo->u.h0.interrupt_line, interrupt_hook, *cookie, 0 );
	DEBUG(device, INFO, kDevName ": installed on interrupt %d\n", device->pciInfo->u.h0.interrupt_line);		

	if (init_ring_buffers(device) != B_OK)
		goto err2;
	
	write32(ioBase + EPIC_GENCTL, 0x4000);
	/* magic line...fixes SOMETHING...it's in the linux driver... */
	write32(ioBase + EPIC_TEST1, 0x0008);

	/* Pull the chip out of low-power mode, enable interrupts, and set for
	 * PCI read multiple. The MIICFG setting and strange write order are
	 * required by the details of which bits are reset.
	 */
	write32(ioBase + EPIC_MIICFG, 0x12);

	write32(ioBase + EPIC_GENCTL, (0x0412 | (RX_FIFO_THRESH<<8))); /*little endian for now */

	/* write32(ioBase + EPIC_GENCTL, (0x0432 | (RX_FIFO_THRESH<<8))); this is for big endian support */
	
	get_mac_address(device);	
	
	write32(ioBase + EPIC_TXTHRESH, TX_FIFO_THRESH);

	mii_reg5 = mdio_read(ioBase, device->phys[0], 5);
	if (mii_reg5 != 0xFFFF)
	{
		if ((mii_reg5 & 0x0100) || (mii_reg5 & 0x01C0) == 0x0040)
		{
			device->media = 1;
		}
		else if (!(mii_reg5 & 0x4000))
		{
			device->media = 0;
			mdio_write(ioBase, device->phys[0], 0, 0x1200);
		}
	}
	write32(ioBase + EPIC_TXCTRL, device->media ? 0x7F : 0x79);
	DEBUG(device, INFO, kDevName ": TXCTRL set to %4.4x and media is %d\n", device->media ? 0x7F : 0x79, (int)device->media);
	
	get_memory_map(&device->rx_desc[0], sizeof(EPIC_RX_DESC), &entry, 1);
	write32(ioBase + EPIC_PRXCDAR, (uint32)gPCIModInfo->ram_address(entry.address));
	get_memory_map(&device->tx_desc[0], sizeof(EPIC_TX_DESC), &entry, 1);
	write32(ioBase + EPIC_PTXCDAR, (uint32)gPCIModInfo->ram_address(entry.address));

	/* now we start the chip's Rx process */
	set_rx_mode(device);
	write32(ioBase + EPIC_COMMAND, 0x000A);

	/* set up our interrupt masks */
	write32(ioBase + EPIC_INTMASK, 
			  EPIC_PCIBUSERR175 | EPIC_CNTFULL    | EPIC_TXUNDERRUN | EPIC_TXDONE
			| EPIC_RXERROR      | EPIC_RXOVERFLOW | EPIC_RXFULL     | EPIC_RXHEADER | EPIC_RXDONE);
	DEBUG(device, INFO, kDevName ": open_hook - EXIT (B_NO_ERROR)\n");		
	return B_NO_ERROR;

	err2:
		free_ring_buffers(device);
		
	err1:
		free_resources(device);
		free(device);	
	
	err0:
		atomic_and(&gOpenMask, ~mask);
		DEBUG(device, ERR, kDevName ": open failed!\n");
		return status;

}

static status_t close_hook(void *_device) {
	dev_info_t *device = (dev_info_t *) _device;
	int32 ioBase = device->reg_base;
	int i;

	DEBUG(device, FUNCTION, kDevName ": close_hook - ENTRY\n");

	DEBUG(device, INFO, kDevName ": Shutting down - status was %2.2x.\n", (int)read32(ioBase + EPIC_INTSTAT));	
	/* Disable interrupts by clearing the interrupt mask. */
	write32(ioBase + EPIC_INTMASK, 0x00000000);
	/* Stop the chip's Tx and Rx DMA processes. */
	write16(ioBase + EPIC_COMMAND, 0x0061);

	/* TODO Update the error counts. */
	/* stats.rx_missed_errors, rx_frame_errors, rx_crc_errors */

	/* free all the stuff in the Rx queue */
	for (i=0; i< RX_BUFFERS; i++)
	{
		device->rx_desc[i].status = 0;
		device->rx_desc[i].buflength = 0;
		device->rx_desc[i].bufaddr = 0xBADF00D0; /*invalid address */
	}

	/* Leave the chip in low-power mode...Be Green! */
	write32(ioBase + EPIC_GENCTL, 0x0008);
		
	/* Release resources we are holding */
	
#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, EPICEthernet);
#endif
	
	free_resources(device);
	DEBUG(device, FUNCTION, kDevName ": close_hook - EXIT (B_NO_ERROR)\n");				
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	dev_info_t *device = (dev_info_t *) cookie;

	DEBUG(device, INFO, kDevName ": free_hook - ENTRY\n");
	DEBUG(device, FUNCTION, kDevName": free %p\n",device);

    /* Remove Interrupt Handler */
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, cookie );
	
	free_ring_buffers(device);

	/* Device is now available again */
	atomic_and(&gOpenMask, ~(1 << device->devID));

	free(device);
	DEBUG(device, INFO, kDevName ": free_hook - EXIT\n");
	return 0;
}


/* Standard driver control function */
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len) {
	dev_info_t *device = (dev_info_t *) cookie;

	(void)len;

	DEBUG(device, INFO, kDevName ": control_hook - ENTRY\n");
	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			DEBUG(device, FUNCTION, kDevName ": control %x ether_getaddr\n",(int)msg);
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->mac_address.ebyte[i];
			}
			return B_NO_ERROR;
		}
		case ETHER_INIT:
			DEBUG(device, FUNCTION, kDevName ": control %x init\n",(int)msg);
			return B_NO_ERROR;
			
		case ETHER_GETFRAMESIZE:
			DEBUG(device, FUNCTION, kDevName ": control %x get_framesize\n",(int)msg);
			*(uint32 *)buf = PKT_BUF_SZ;
			return B_NO_ERROR;
			
		case ETHER_ADDMULTI:
			DEBUG(device, FUNCTION, kDevName ": control %x add multi\n",(int)msg);
			return (domulti(device, (unsigned char *)buf));
		
		case ETHER_SETPROMISC:
			DEBUG(device, FUNCTION, kDevName ": control %x set promiscuous\n",(int)msg);
			DEBUG(device, INFO, kDevName ": control_hook - EXIT (B_ERROR)\n");
			return B_ERROR;
		
		case ETHER_NONBLOCK:
			DEBUG(device, FUNCTION, kDevName ": control %x blocking\n",(int)msg);

			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			DEBUG(device, INFO, kDevName ": control_hook - EXIT (B_NO_ERROR)\n");
			return B_NO_ERROR;
			
		default:
			DEBUG(device, ERR, kDevName ": unknown iocontrol %x\n",(int)msg);
			DEBUG(device, INFO, kDevName ": control_hook - EXIT (B_ERROR)\n");
			return B_ERROR;
	}
	
}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len) {
	dev_info_t  *device = (dev_info_t *) _device;
	status_t    status;
	int entry = device->cur_rx % RX_BUFFERS;
	*len = 0;

	(void)pos;

	DEBUG(device, FUNCTION, kDevName ": read_hook - ENTRY\n");

	DEBUG(device, INFO, kDevName ": read_hook - Check Semaphore: blockFlag = 0x%8.8x\n", (int)device->blockFlag);
	/* Block until data is available */
	if((status = gFC->fc_wait( &device->ilock, (device->blockFlag & B_TIMEOUT) ? 0 : B_INFINITE_TIMEOUT )) != B_NO_ERROR) {
		*len = 0;
		return status;
	}
	
	DEBUG(device, INFO, kDevName ": read_hook - Check reentrant read\n");
	/* Prevent reentrant read */
	if( atomic_or( &device->readLock, 1 ) ) {
		DEBUG(device, INFO, kDevName ": read_hook - release fc_lock\n");
		gFC->fc_signal( &device->ilock, 1, B_DO_NOT_RESCHEDULE );
		*len = 0;
		return B_ERROR;
	}

	DEBUG(device, INFO, kDevName ": read_hook - start Epic-specific data read\n");
	while (device->rx_desc[entry].status >= 0 && device->rx_buf[entry])
	{
		int status = device->rx_desc[entry].status;
		
		if (status & 0x2006)
		{
			if (status & 0x2000)
			{
				DEBUG(device, ERR, kDevName ": Oversized Ethernet frame spanned multiple buffers, status %4.4x!\n", status);
				*len = 0;
			} else if (status & 0x0006)
			{
				// increment receive error
	 			*len = 0;
			}
		} else
		{
			// malloc a new buffer and omit 4 octet CRC from length
			short pkt_len = device->rx_desc[entry].rxlength - 4;
			// now copy the data to the buffer...
			*len = pkt_len;
			memcpy(buf, device->rx_buf[entry], pkt_len);
			
#if DEBUG_VERSION	/* this is just a waste of processor cycles otherwise */
			if (device->debug & RX) {
				DEBUG(device, FUNCTION, kDevName ": read_hook - Real Buffer:\n");	
				dump_packet("rx", (unsigned char *) buf, pkt_len);
			}
#endif	
		}
		entry = (++device->cur_rx) % RX_BUFFERS;
	}
	/* 'Refill' the Rx ring buffers */
	for (; device->cur_rx - device->dirty_rx > 0; device->dirty_rx++)
	{
		entry = device->dirty_rx % RX_BUFFERS;
		device->rx_desc[entry].status = 0x8000;
	}
	/* update acked index for next read call, and release reantrant lock */
	device->readLock = 0;
	DEBUG(device, FUNCTION, kDevName ": read_hook - EXIT\n");
	return B_OK;
}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
 
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len) {
	dev_info_t  *device = (dev_info_t *) _device;
	int16        frame_size;
	status_t      status;
	uint32			flag;
	int			 	entry;

	(void)pos;

	DEBUG(device, FUNCTION, kDevName ": write_hook - ENTRY\n");	

	DEBUG(device, INFO, kDevName ": write_hook - Check Frame Size\n");	
	if( *len > PKT_BUF_SZ ) {
		DEBUG(device, ERR, kDevName ": write %d > 1536 tooo long\n",(int)*len);
		*len = PKT_BUF_SZ;
	}
	frame_size = *len;
	
	DEBUG(device, INFO, kDevName ": write_hook - Check Semaphore\n");	
	if((status = gFC->fc_wait( &device->olock, (device->blockFlag & B_TIMEOUT) ? 0 : B_INFINITE_TIMEOUT )) != B_NO_ERROR) {
		DEBUG(device, INFO, kDevName ": write_hook - Acquire fc_lock failed! EXITING\n");
		*len = 0;
		return status;
	}

	DEBUG(device, INFO, kDevName ": write_hook - Check Reentrant write\n");	
	/* Prevent reentrant write */
	if( atomic_or( &device->writeLock, 1 ) ) {
		DEBUG(device, INFO, kDevName ": write_hook - re-entrant write. EXITING\n");
		gFC->fc_signal( &device->olock, 1, B_DO_NOT_RESCHEDULE );
		*len = 0;
		return B_ERROR;
	}

#if DEBUG_VERSION
	if (device->debug & TX) {
		DEBUG(device, FUNCTION, kDevName ": write_hook - Real Buffer:\n");
		dump_packet("tx", (unsigned char *) buf, frame_size);
		DEBUG(device, FUNCTION, "\n");
	}	
#endif

	entry = device->cur_tx % TX_BUFFERS;

#if DEBUG_VERSION
	/* I don't know if this could *EVER* happen with the EPIC card...but if you have xfer problems
	   then I'd suggest that you set it in debug mode and watch for this message... */	   
	if (device->tx_desc[entry].status & 0x8000)
	{
		DEBUG(device, FUNCTION, kDevName ": write_hook - can't write over card's packets\n");
		device->writeLock = 0;
		return B_ERROR;
	}
#endif

	device->tx_desc[entry].txlength = (frame_size >= ETH_ZLEN)? frame_size : ETH_ZLEN;
	device->tx_desc[entry].buflength = frame_size;
	
	/* now copy the buffer */
	memcpy(device->tx_buf[entry], buf, frame_size);

	if (device->cur_tx - device->dirty_tx < (TX_BUFFERS>>1))
	{
		flag = 0x10; /* No interrupt */
	} else if (device->cur_tx - device->dirty_tx == (TX_BUFFERS>>1))
	{
		flag = 0x14; /* Tx-done Int. */
	} else if (device->cur_tx - device->dirty_tx < (TX_BUFFERS - 2))
	{
		flag = 0x10; /* No Tx-done Int. */
	} else
	{
		/* leave room for two additional entries */
		flag = 0x14; /* Tx Done int. */
		device->tx_full = 1;
	}
	device->tx_desc[entry].control = flag;
	device->tx_desc[entry].status = 0x8000; /*pass ownership to the chip */
	device->cur_tx++;
	/* trigger immediate transmit command */
	write32(device->reg_base + EPIC_COMMAND, 0x0004);

	DEBUG(device, INFO, kDevName ": Queued Tx packet size %d to slot %d, flag %2.2x Tx status %8.8x.\n", (int)frame_size, (int)entry, (int)flag, (int)read32(device->reg_base + EPIC_TXSTAT));
	/* Another write may now take place */
	device->writeLock = 0;
	DEBUG(device, FUNCTION, kDevName ": write_hook - EXIT\n");		
	return B_OK;
}


/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	dev_info_t *device = (dev_info_t *) _device;
	int32 handled = B_UNHANDLED_INTERRUPT;
	uint32 invoke_scheduler = 0;
	uint32 status;
	int32 worklimit = 20;			/* process as many frames as reasonable, but limit time in interrupt */
	int32 ioBase = device->reg_base;
	
	DEBUG(device, FUNCTION, kDevName ": interrupt_hook - ENTRY\n");
 
	do {	
		status = read32(ioBase + EPIC_INTSTAT);
		/* ack all of the current interrupt sources ASAP */
		write32(ioBase + EPIC_INTSTAT, status & 0x00007FFF);

		if ((status & EPIC_INTRSUMMARY) == 0)
		{
			DEBUG(device, INTERRUPT, kDevName ": ISR - INTRSUMMARY\n");
			//handled = B_INVOKE_SCHEDULER;
			break;
		}	
		if (status & (EPIC_RXDONE | EPIC_RXSTARTED | EPIC_RXEARLYWARN))
		{
			invoke_scheduler = 1;   /* Set because the interrupt was from the NIC, not some other device sharing the interrupt line */
			device->handled_int++;  		/* update stats */
			DEBUG(device, INTERRUPT, kDevName ": ISR - RXDONE | RXSTARTED | RXEARLYWARN\n");
			if (status & EPIC_RXDONE)
			{
				DEBUG(device, INTERRUPT, kDevName ": ISR - RXDONE\n");
				gFC->fc_signal(&device->ilock, 1, B_DO_NOT_RESCHEDULE);
			}			
		}
		if (status & (EPIC_TXEMPTY | EPIC_TXDONE))
		{
			int dirty_tx;
			int i;
			invoke_scheduler = 1;
			device->handled_int++;  		/* update stats */
			DEBUG(device, INTERRUPT, kDevName ": ISR - TXEMPTY | TXDONE\n");
			for (dirty_tx = device->dirty_tx, i=0; dirty_tx < device->cur_tx; dirty_tx++, i++)
			{
				int entry = dirty_tx % TX_BUFFERS;				
				int txstatus = device->tx_desc[entry].status;
				if (txstatus < 0) {
					DEBUG(device, INTERRUPT, kDevName ": ISR - packet %d still not tx'ed! status %8.8x\n", entry, txstatus);
					break; /* still hasn't been tx'ed */
				}
				if (!(txstatus & 0001)) {
					DEBUG(device, INTERRUPT, kDevName ": ISR - tx major error!\n");
					/* there was a major error, log it */
					device->stats_tx_err++;
				} else {
					DEBUG(device, INTERRUPT, kDevName ": ISR - else thing...\n");
					device->stats_collisions += (txstatus >> 8) & 15;
					device->stats_tx++;
				}
				/* empty the data pointed to by the buffer address */
				/* I don't know if this is necessary in a release version of the driver */				
				/*memset(device->tx_buf[entry], 0, PKT_BUF_SZ);*/
				
				DEBUG(device, INTERRUPT, kDevName ": ISR - status of entry %d was %4.4x\n", entry, device->tx_desc[entry].status);
				device->tx_desc[entry].status = 0;
				DEBUG(device, INTERRUPT, kDevName ": ISR - olock fc_locks left is %d\n", (int)device->olock.count);
				gFC->fc_signal(&device->olock, 1, B_DO_NOT_RESCHEDULE);
			}
			device->dirty_tx = dirty_tx;
		}
		/*  check uncommon events all at once */
		if (status & (EPIC_CNTFULL | EPIC_TXUNDERRUN | EPIC_RXOVERFLOW | EPIC_PCIBUSERR175)) {
			DEBUG(device, INTERRUPT, kDevName ": ISR - CNTFULL | TXUNDERRUN | RXOVERFLOW | PCIBUSERR\n");
			handled = B_INVOKE_SCHEDULER;   /* Set because the interrupt was from the NIC, not some other device sharing the interrupt line */
			device->handled_int++;  		/* update stats */
			if (status == 0xFFFFFFFF) { /* chip failed */
				DEBUG(device, INTERRUPT, kDevName ": ISR - Chip Failed!\n");
				break;
			}
			/* always update error counts to avoid overhead later */
			if (status & EPIC_CNTFULL) {
				DEBUG(device, INTERRUPT, kDevName ": ISR - CNTFULL\n");
			}	
			if (status & EPIC_TXUNDERRUN) {
				DEBUG(device, INTERRUPT, kDevName ": ISR - TXUNDERRUN\n");
				write32(ioBase + EPIC_TXTHRESH, 1536);
				/* restart transmit process */
				write32(ioBase + EPIC_COMMAND, 0x0080);
			}
			if (status & EPIC_RXOVERFLOW) {
				DEBUG(device, INTERRUPT, kDevName ": ISR - RXOVERFLOW\n");			
				device->stats_rx_err++;
			}
			if (status & EPIC_PCIBUSERR175)
			{
				DEBUG(device, INTERRUPT, kDevName ": ISR - PCIBUSERR175\n");
				/* very badd! */
				if (status & 0x8000000) {
					DEBUG(device, INTERRUPT, kDevName " ISR - PCI Target Abort\n");
				}
				if (status & 0x4000000) {
					DEBUG(device, INTERRUPT, kDevName " ISR - PCI Master Abort\n");
				}
				if (status & 0x2000000) {
					DEBUG(device, INTERRUPT, kDevName " ISR - PCI Address Parity Error\n");
				}
				if (status & 0x1000000) {
					DEBUG(device, INTERRUPT, kDevName " ISR - PCI Data Parity Error\n");
				}
				/* restart the card due to the harsh error... */
				reset_device(device);
			}
			/* clear all error sources */
			write32(ioBase + EPIC_INTSTAT, status & 0x7F18);
		}
		if (--worklimit < 0)
		{
			DEBUG(device, INTERRUPT, kDevName ": ISR - work limit hit\n");
			handled = B_INVOKE_SCHEDULER;   /* Set because the interrupt was from the NIC, not some other device sharing the interrupt line */
			device->handled_int++;  		/* update stats */
			/* clear all interrupt sources...too much done */
			write32(ioBase + EPIC_INTSTAT, 0x0001FFFF);
			break;
		}
	} while(1);

	DEBUG(device, FUNCTION, kDevName ": interrupt_hook - EXIT\n");

	if ( invoke_scheduler )
		return B_INVOKE_SCHEDULER;
	else
		return handled;
}


static int32 get_pci_list(pci_info *info[], int32 maxEntries)
{
	status_t status;
	int32 i, entries;
	pci_info	*item;
	
	item = (pci_info *)malloc(sizeof(pci_info));
	
	for (i=0, entries=0; entries<maxEntries; i++) {
		if ((status = gPCIModInfo->get_nth_pci_info(i, item)) != B_OK)
			break;
		if ( (item->vendor_id == VENDOR_ID) && (item->device_id == DEVICE_ID) ) {
			/* check if the device really has an IRQ */
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf(kDevName " found with invalid IRQ - check IRQ assignement\n");
				continue;
			}
			dprintf(kDevName " found at IRQ %d \n", item->u.h0.interrupt_line);
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
		}
	}
	info[entries] = NULL;
	free(item);
	return entries;
}

static status_t free_pci_list(pci_info *info[])
{
	int32 		i;
	pci_info	*item;
	
	for (i=0; (item=info[i]); i++)
		free(item);
	return B_OK;
}


static status_t enable_addressing(dev_info_t *device)
{
	// Turn on I/O port decode, Memory Address Decode, and Bus Mastering
	gPCIModInfo->write_pci_config(device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, PCI_command, 2,
		PCI_command_io | PCI_command_memory | PCI_command_master |
		gPCIModInfo->read_pci_config( device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, PCI_command, 2));

	device->reg_base = device->pciInfo->u.h0.base_registers[0];
	
	DEBUG(device, PCI_IO,  kDevName ": reg_base=%x\n", (int)device->reg_base);

	return B_OK;
}




static status_t init_ring_buffers(dev_info_t *device)
{
	uint32 				size;
	physical_entry		entry;
	uint16 				i;
	device->cur_rx = device->cur_tx = 0;
	device->dirty_rx = device->dirty_tx = 0;
	device->tx_full = 0;
	
	DEBUG(device, INFO, kDevName ": init_ring_buffers - ENTRY\n");
	
	DEBUG(device, INFO, kDevName ": create xmit buffer area\n");
	/* create transmit buffer area */
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_buf_area = create_area( kDevName " tx buffers", (void **) device->tx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) 
	{
		DEBUG(device, ERR, kDevName " create tx buffer area failed %x \n", (int)device->tx_buf_area);
		return device->tx_buf_area;
	}

	/* create tx descriptor area */
	DEBUG(device, INFO, kDevName ": create xmit descriptor area\n");
	size = RNDUP( sizeof(frame_desc_t) * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_desc_area = create_area( kDevName " tx descriptors", (void **) device->tx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) 
	{
		DEBUG(device, ERR, kDevName " create tx descriptor area failed %x \n", (int)device->tx_desc_area);
		delete_area(device->tx_buf_area);
		return device->tx_desc_area;
	}
	
	/* initialize descriptors */
	DEBUG(device, INFO, kDevName ": initialize descriptors\n");
	for ( i = 0; i < TX_BUFFERS; i++) 
	{
		device->tx_buf[i] = device->tx_buf[0] + (BUFFER_SIZE * i);
		device->tx_desc[i].status = 0x0000;
		get_memory_map(&device->tx_desc[i+1], sizeof(EPIC_TX_DESC), &entry, 1);
		device->tx_desc[i].next = (uint32)gPCIModInfo->ram_address(entry.address);
		get_memory_map(device->tx_buf[i], 4, &entry, 1);
		device->tx_desc[i].bufaddr = (uint32)gPCIModInfo->ram_address(entry.address);

	}
	/* make sure to wrap the buffer in the end */
	get_memory_map(&device->tx_desc[0], sizeof(EPIC_TX_DESC), &entry, 1);
	device->tx_desc[TX_BUFFERS-1].next = (uint32)gPCIModInfo->ram_address(entry.address);
	
	DEBUG(device, INFO, kDevName ": create rx buffer area\n");
	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) 
	{
		DEBUG(device, ERR, kDevName " create rx buffer area failed %x \n", (int)device->rx_buf_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		return device->rx_buf_area;
	}

	DEBUG(device, INFO, kDevName ": create rx descriptor area\n");
	/* create rx descriptor area */
	size = RNDUP( sizeof(frame_desc_t) * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_desc_area = create_area( kDevName " rx descriptors", (void **) device->rx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) 
	{
		DEBUG(device, ERR, kDevName " create rx descriptor area failed %x \n", (int)device->rx_desc_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		return device->rx_desc_area;
	}

	/* init rx buffer descriptors */
	DEBUG(device, INFO, kDevName ": init rx buffer descriptors\n");
	for ( i = 0; i < RX_BUFFERS; i++)
	{
		device->rx_buf[i] = device->rx_buf[0] + (BUFFER_SIZE * i);
		get_memory_map(&device->rx_desc[i+1], sizeof(EPIC_RX_DESC), &entry, 1);
		device->rx_desc[i].status = 0x8000; /* Epic is Owner */
		device->rx_desc[i].buflength = PKT_BUF_SZ;
		device->rx_desc[i].next = (uint32)gPCIModInfo->ram_address(entry.address);
		/* now set the buffer pointers */
		get_memory_map(device->rx_buf[i], 4, &entry, 1);
		device->rx_desc[i].bufaddr = (uint32)gPCIModInfo->ram_address(entry.address);
	}
	/* again...remember to wrap the ring */
	get_memory_map(&device->rx_desc[0], sizeof(EPIC_RX_DESC), &entry, 1);
	device->rx_desc[RX_BUFFERS-1].next = (uint32)gPCIModInfo->ram_address(entry.address);
	
	DEBUG(device, INFO, kDevName ": initialize frame indexes\n");
	/* initialize frame indexes */
	device->tx_sent = device->tx_acked = device->rx_received = device->rx_acked = 0;
	
	DEBUG(device, INFO, kDevName ": init_ring_buffers - EXIT\n");
	return B_OK;
}

static void free_ring_buffers(dev_info_t *device) {
	DEBUG(device, FUNCTION, kDevName ": free_ring_buffers - ENTRY\n");
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		delete_area(device->rx_desc_area);
	DEBUG(device, FUNCTION, kDevName ": free_ring_buffers - EXIT\n");
}


static void get_mac_address(dev_info_t *device) {

	int     j;
	int32   ioBase = device->reg_base;
	
	DEBUG(device, FUNCTION, kDevName ": get_mac_address - ENTER\n");
	for (j=0; j<3; j++) {
		((uint16*)device->mac_address.ebyte)[j] = read16(ioBase + EPIC_LAN0 + j*4);
	}
	DEBUG(device, INFO, kDevName ": Mac addres = %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n", 
		device->mac_address.ebyte[0], device->mac_address.ebyte[1], device->mac_address.ebyte[2], 
		device->mac_address.ebyte[3], device->mac_address.ebyte[4], device->mac_address.ebyte[5]);
	DEBUG(device, FUNCTION, kDevName ": get_mac_address - EXIT\n");
}


/* set hardware so all packets are received. */
static status_t setpromisc(dev_info_t * device) {
	DEBUG(device, FUNCTION, kDevName ":setpromisc\n");
	DEBUG(device, FUNCTION, kDevName ": No promiscuous access...sorry\n");	
	return(B_ERROR);
}

static status_t domulti(dev_info_t *device, uint8 *addr) {
	
	DEBUG(device, FUNCTION,kDevName ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
					addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	return (B_NO_ERROR);
}

static void reset_device(dev_info_t *device) {
    int32 ioBase = device->reg_base;
    physical_entry entry;
    
	DEBUG(device, FUNCTION,kDevName ": reset_device reset the NIC hardware\n"); 
	/* soft reset the chip */
	write32(ioBase + EPIC_GENCTL, 0x0001);
	/* duplicate stuff from startup code */
	write32(ioBase + EPIC_TEST1, 0x0008);
	/*this is little endian now */
	write32(ioBase + EPIC_GENCTL, 0x0412 | (RX_FIFO_THRESH<<8));
	write32(ioBase + EPIC_MIICFG, 0x12);
	/* check chip Id */
	write32(ioBase + EPIC_NVCTL,(read32(ioBase + EPIC_NVCTL) & ~0x003C) | 0x4800);
	write32(ioBase + EPIC_TXTHRESH, TX_FIFO_THRESH);
	write32(ioBase + EPIC_TXCTRL, device->media?0x7F:0x79);
	/* re-upload those ring buffers */
	get_memory_map(&device->rx_desc[0], sizeof(EPIC_RX_DESC), &entry, 1);
	write32(ioBase + EPIC_PRXCDAR, (uint32)gPCIModInfo->ram_address(entry.address));
	get_memory_map(&device->tx_desc[0], sizeof(EPIC_TX_DESC), &entry, 1);
	write32(ioBase + EPIC_PTXCDAR, (uint32)gPCIModInfo->ram_address(entry.address));
	/* start the chip's RX process */
	set_rx_mode(device);
	write32(0x000A, ioBase + EPIC_COMMAND);
	/* enable interrupts by setting the interrupt mask */
	write32(ioBase + EPIC_INTMASK, 
			  EPIC_PCIBUSERR175 | EPIC_CNTFULL | EPIC_TXUNDERRUN | EPIC_TXDONE
			| EPIC_RXERROR | EPIC_RXOVERFLOW | EPIC_RXFULL | EPIC_RXHEADER
			| EPIC_RXDONE);
};

static void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %p size %d \n", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
}


static void dump_rx_desc(dev_info_t *device) {
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("rx desc %p \n", device->rx_desc);
	for (j=0; j < RX_BUFFERS; j++ ) {
		kprintf("rx_desc[%2.2d]=...\n", j);
	}
}

static void dump_tx_desc(dev_info_t *device) {
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("tx desc %p \n", device->tx_desc);
	
	for (j=0; j < TX_BUFFERS; j++ ) {
		kprintf("tx_desc[%2.2d]...\n",j);
	}
}
static void set_rx_mode(dev_info_t *device)
{
	int32 ioBase = device->reg_base;
	unsigned char mc_filter[8];  /*multicast hash filter */
	int i;
	DEBUG(device, FUNCTION, kDevName ": set_rx_mode - ENTRY\n");
  	memset(mc_filter, 0xFF, sizeof(mc_filter));
	// set to accept multicast and broadcast packets...
  	write32(ioBase + EPIC_RXCTRL, 0x000C);
  	
	if (memcmp(mc_filter, device->mc_filter, sizeof(mc_filter))) {
		for (i = 0; i < 4; i++)
		{
			write32(ioBase + EPIC_MC0 + i*4, ((uint32*)mc_filter)[i]);
		}
		memcpy(device->mc_filter, mc_filter, sizeof(mc_filter));
	}
	DEBUG(device, FUNCTION, kDevName ": set_rx_mode - EXIT\n");
	return;
}

/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "EPICEthernet arg...",
   for example "EPICEthernet R" to enable a received packet trace.
*/
#if DEBUGGER_COMMAND
static int
EPICEthernet(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: EPICEthernet { Function_calls | Interrupts | Number_frames | PCI_IO, Stats | Rx_trace | Tx_trace }\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'F':
		case 'f':
			gdev->debug ^= FUNCTION;
			if (gdev->debug & FUNCTION) 
				kprintf("Function() call trace Enabled\n");
			else 			
				kprintf("Function() call trace Disabled\n");
			break; 
		case 'N':
		case 'n':
			gdev->debug ^= SEQ;
			if (gdev->debug & SEQ) 
				kprintf("Sequence numbers packet trace Enabled\n");
			else 			
				kprintf("Sequence numbers packet trace Disabled\n");
			break; 
		case 'R':
		case 'r':
			gdev->debug ^= RX;
			if (gdev->debug & RX) 
				kprintf("Receive packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break;
		case 'T':
		case 't':
			gdev->debug ^= TX;
			if (gdev->debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'S':
		case 's':
			kprintf(kDevName " statistics\n");			
			kprintf("received %d,  transmitted %d\n", (int)gdev->stats_rx, (int)gdev->stats_tx);
			kprintf("rx err %d,  tx err %d\n", (int)gdev->stats_rx_err, (int)gdev->stats_tx_err);
			kprintf("rx overrun %d,  tx underrun %d\n", (int)gdev->stats_rx_overrun, (int)gdev->stats_tx_underrun);
			kprintf("collisions %d\n", (int)gdev->stats_collisions);
			kprintf("handled %d and unhandled %d interrupts\n", (int)gdev->handled_int, (int)gdev->unhandled_int);
			break; 
		case 'P':
		case 'p':
			gdev->debug ^= PCI_IO;
			if (gdev->debug & PCI_IO) 
				kprintf("PCI IO trace Enabled\n");
			else 			
				kprintf("PCI IO trace Disabled\n");
			break; 
		default:
			kprintf("%s",usage);
			return 0;
		}
	}
	
	return 0;
}
#endif /* DEBUGGER_COMMAND */

static void clear_statistics(dev_info_t *device) {
	
	device->handled_int = device->unhandled_int = 0;
	device->stats_rx = device->stats_tx = 0;
	device->stats_rx_err = device->stats_tx_err = 0;
	device->stats_rx_overrun = device->stats_tx_underrun = 0;
	device->stats_collisions = 0;
}

/*
 * Allocate and initialize fc_locks and spinlocks.
 */
static status_t allocate_resources(dev_info_t *device) {
	status_t	result;
		
	/* Setup Semaphores */
	if ((result = gFC->create_fc_lock( &device->ilock, 0, kDevName " rx" ) ) < 0 ) {
		dprintf(kDevName " create rx fc_lock failed %x \n", (int)result);
		return (result);
	}
	
	if ((result = gFC->create_fc_lock( &device->olock, TX_BUFFERS, kDevName " tx" ) ) < 0 ) {
		dprintf(kDevName " create tx fc_lock failed %x \n", (int)result);
		gFC->delete_fc_lock( &device->ilock );
		return (result);
	}

	device->readLock = device->writeLock = 0;

	device->rx_spinlock = 0;

	device->blockFlag = 0;
		
	return (B_OK);
}

static void free_resources(dev_info_t *device) {
		gFC->delete_fc_lock(&device->ilock);
		gFC->delete_fc_lock(&device->olock);
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
