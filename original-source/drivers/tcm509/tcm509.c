 /*
 * Ethernet Driver 3c5x9 - ISA/PCMCIA
 * CopyRight (c) 2000 Be Inc., All Rights Reserved.
 */

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <module.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <ether_driver.h>
#include <driver_settings.h>


enum media_physical {
	RJ45 = 0,
	COAX = 0xC000,
	AUI = 0x4000,
};
enum media_speed {
	TEN = 10,
	HUNDRED = 100,
	GIG = 1000
};
enum media_duplex {
	HALF = 0,
	FULL = 1
};

// defined in makefile
//#define PCMCIA_CARD true
//#define ISA_PNP_CARD true

#if PCMCIA_CARD

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

#define MAX_DEVS        8
static dev_link_t *     devs[MAX_DEVS];
static int              ndevs = 0;
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4
 
#define kDevName "tcm589"

#else /* ISA PNP */

#include <ISA.h>
#include <config_manager.h>
#include <isapnp.h>

/* ISA vendor and product ID's */
#define VENDOR_ID_0	'T'
#define VENDOR_ID_1	'C'
#define VENDOR_ID_2	'M'
#define DEVICE_ID        0x509

/* globals for driver instances */ 
#define kDevName "tcm509"
#define MAX_DEVS			 4			/* maximum number of driver instances */

#endif	/* ISAPNP or PCMCIA bus */

#define kDevDir "net/" kDevName "/"

#define DEVNAME_LENGTH		64			


/* debug flags */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define BUS_IO    0x0040		/* reads and writes across the [pci,isa,pcmcia,cardbus] bus */
#define SEQ		  0x0080		/* trasnmit & receive TCP/IP sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off for final release */

/* diagnostic debug flags - compile in here or set while running with debugger "3c589" command */
#define DEBUG_FLAGS ( ERR | INFO  )


#define MAX_CARDS 4
#define BUFFER_SIZE			2048L
#define MAX_FRAME_SIZE		1514			/* 1514 + 4 bytes checksum */


static isa_module_info		*isa;
static char 				*gDevNameList[MAX_CARDS+1];
static int32 				gOpenMask = 0;

struct ether_dev_info  {
	uint32	irq;
	uint32 	port;
} gDevInfo[MAX_CARDS+1];

typedef struct ether_dev_info ether_dev_info;


#if ISA_PNP_CARD
#define write8(io_addr, byte) 	(*isa->write_io_8)((io_addr), (byte))
#define write16(io_addr, word) 	(*isa->write_io_16)((io_addr), (word))
#define write32(io_addr, lword) (*isa->write_io_32)((io_addr), (lword))
#define read8(io_addr) 			(*isa->read_io_8)(io_addr)
#define read16(io_addr) 		(*isa->read_io_16)(io_addr)
#define read32(io_addr) 		(*isa->read_io_32)(io_addr)
#endif

#if PCMCIA_CARD
#define write8(io_addr, byte) 	(*pci->write_io_8)((io_addr), (byte))
#define write16(io_addr, word) 	(*pci->write_io_16)((io_addr), (word))
#define write32(io_addr, lword) (*pci->write_io_32)((io_addr), (lword))
#define read8(io_addr) 			(*pci->read_io_8)(io_addr)
#define read16(io_addr) 		(*pci->read_io_16)(io_addr)
#define read32(io_addr) 		(*pci->read_io_32)(io_addr)
#endif

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

/* Driver Entry Points */
_EXPORT status_t init_hardware(void );
_EXPORT status_t init_driver(void );
_EXPORT void uninit_driver(void );
_EXPORT const char** publish_devices(void );
_EXPORT device_hooks *find_device(const char *name );


/* Hardware specific definitions */

#define TX_BUFFERS 2 			/* tx fifo size / 1514. To Do: check value of tx fifo */
#define RX_BUFFERS  32			/* must be a power of 2 */
#define RX_BUFFER_MASK  (RX_BUFFERS-1)


/* Chip Command  - see page 6-2 of the "Etherlink III Parallel Tasking
	ISA, EISA, Micro Channel, and PCMCIA Adapters Driversr Technical Reference"
	3Com Manual Part No. 09-0398-002B */
enum ChipCommands {
	GlobalReset=0x0<<11, SelectWindow=0x1<<11, StartCoax=0x2<<11,
	RxDisable=0x3<<11, RxEnable=0x4<<11, RxReset=0x5<<11, RxDiscard=0x8<<11,
	TxEnable=0x9<<11, TxDisable=0xA<<11, TxReset=0xB<<11,
	RequestInterruptCmd=0xC<<11, AckInterrupt=0xD<<11, SetIntrMask=0xE<<11,
	SetReadZeroMask=0xF<<11, SetRxFilter=0x10<<11, SetRxEarlyThreshold=0x11<<11,
	SetTxAvailableThreshold=0x12<<11, SetTxStartThreshold=0x13<<11, StatsEnable=0x15<<11,
	StatsDisable=0x16<<11, StopCoax=0x17<<11, PowerUp=0x1B<<11,
	PowerDown=0x1C<<11, PowerAuto=0x1D<<11,
};
enum ChipStatus {
	InterruptLatch = 0x0001, AdapterFail = 0x0002, TxComplete = 0x0004,
	TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
	InterruptRequest = 0x0040, UpdateStatistics = 0x0080, CmdInProgress = 0x1000,
};
/* hardware receiver address filters */
enum chip_filters {
	RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxPromiscuous = 8,
};


/* Offsets common to all windows */
enum WxCommon {
		Wx_CMD=0x0e, Wx_STATUS=0x0e,
};


/* Register window 0 offsets, the window used for setup & config. */
enum Window0 {
	ADDRESS_CONFIG = 0x6,
	EEPROM_CMD = 0xA,
	EEPROM_DATA	= 0xC,	
};

/* Register window 1 offsets, the window used in normal operation. */
enum Window1 {
	TX_DATA=0x00,	RX_DATA=0x00,
	RX_STATUS=0x08, TX_STATUS=0x0B,
	TX_FREE=0x0C,
};
	
#define W0_IRQ		0x08		/* Window 0: Set IRQ line in bits 12-15. */


#define W4_MEDIA	0x0A		/* Window 4: Various transcvr/media bits. */
enum win4_media {
	PassUpCrc=1<<2, SqeStats=1<<3, Jabber=1<<6, LinkBeatEnable=1<<7, LinkDetect=1<<11,
	};

#define SET_WINDOW(win_num) write16(device->port + Wx_CMD, SelectWindow + (win_num))


/* per driver intance globals */
typedef struct {
	uint8			macAddr[6];								/* ethernet mac address */
	uint16			irq;									/* our IRQ line */
	int32			devID; 									/* device identifier: 0-n */
	sem_id			ilock, olock;							/* i/o semaphores */
	int32			readLock, writeLock;					/* reentrant reads/writes not allowed */
	int32			blockFlag;								/* for blocking or nonblocking reads */
	uint32			port; 									/* Base address of hostRegs */
	uint16			media;									/* speed duplex media settings */
	uint16			speed;
	uint16			duplex;
	uint32			debug;									/* debuging level */

	area_id			rx_buf_area; 							/* receive descriptor and buffer areas */
	uchar			*rx_buf[RX_BUFFERS];					/* receive buffers */
	uint32			rx_desc[RX_BUFFERS];					/* rececive buffer descriptors */
	int16			rx_received, rx_acked;					/* index to in use tx & rx ring buffers */
	spinlock		WriteFifo;								/* write fifo spinlock */
	int32			WritePending;							/* FIFO write queue flag */
	uint32			MultiOn;								/* receive multicast frames */
	uint32			PromiscOn;								/* receive multicast frames */

	/* statistics */
	uint32			handled_int, unhandled_int;				/* interrupts rececived & handled */
	uint32			stats_rx, stats_tx;						/* receive & transmit frames counts */
	uint32			stats_rx_overrun, stats_tx_underrun;
	uint32			stats_rx_len_err, stats_tx_len_err;
	uint32			stats_rx_frame_err, stats_rx_crc_err;
	uint32			stats_collisions;  						/* error stats */

} ether_dev_info_t;


//#define ETHER_DEBUG(flags, run_time_flags, args...) if (flags & run_time_flags) dprintf(args)
void ETHER_DEBUG(ether_dev_info_t * device, int32 flags, char * format, ...) {
	if (device->debug & flags) {
		va_list		args;
		char		s[4096];
		va_start(args, format);
		vsprintf( s, format, args );
		va_end(args);
		dprintf("%s",s);
	}
}

/* for serial debug command*/
#define DEBUGGER_COMMAND true
#if DEBUGGER_COMMAND
ether_dev_info_t * gdev;
static int 		tcm5x9(int argc, char **argv);
#endif


/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len);
static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length );
static int32    interrupt_hook(void *_device);                	   /* interrupt handler */

static void     get_mac_address(ether_dev_info_t *device);               /* get ethernet address */
static status_t setpromisc(ether_dev_info_t * device, uint32 On);        /* set hardware to receive all packets */
static status_t domulti(ether_dev_info_t *device, uint8 *addr);		   /* add multicast address to hardware filter list */
static void     reset_device(ether_dev_info_t *device);                  /* reset the lan controller (NIC) hardware */
static void 	dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
static void 	dump_rx_desc(ether_dev_info_t *device);				   /* dump hardware receive descriptor structures */
static void 	dump_tx_desc(ether_dev_info_t *device);				   /* dump hardware transmit descriptor structures */
static void 	dump_window(ether_dev_info_t *device, uchar window);	   /* dump hardware registers */
static void		clear_statistics(ether_dev_info_t *device);			   /* clear statistics counters */
static status_t allocate_resources(ether_dev_info_t *device);     	   /* allocate semaphores & spinlocks */
static void     free_resources(ether_dev_info_t *device);                /* deallocate semaphores & spinlocks */
static status_t init_ring_buffers(ether_dev_info_t *device);
static void		free_ring_buffers(ether_dev_info_t *device);

static int32 	get_isa_list(int32 maxEntries );
static uint16 	read_eeprom(ether_dev_info_t *device, int32 index);
static void		load_media_settings(uint16 *media, uint16 *speed, uint16 *duplex);
static uint8 	load_irq_settings(uint16 *irq, uint32 *port);


#if PCMCIA_CARD
status_t pc_init_driver(void);
void pc_uninit_driver(void);
const char **pc_publish_devices(void);
#endif


static device_hooks gDeviceHooks =  {
	open_hook, 			/* -> open entry point */
	close_hook,         /* -> close entry point */
	free_hook,          /* -> free entry point */
	control_hook,       /* -> control entry point */
	read_hook,          /* -> read entry point */
	write_hook,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,               /* -> deselect entry point */
	NULL,               /* -> readv */
	writev_hook         /* -> writev */
};


/* Driver Entry Points */


status_t init_hardware (void)
{
	return B_NO_ERROR;
}


#if PCMCIA_CARD
char				*pci_name = B_PCI_MODULE_NAME;
pci_module_info		*pci;

#endif


status_t init_driver()
{
	status_t 		status;
	int32			entries;
	char			devName[64];
	int32			i;
	

#if PCMCIA_CARD
	status = get_module(pci_name, (module_info **)&pci);
	if(status != B_NO_ERROR)
		goto err1;

	if ((status = pc_init_driver()) != B_OK) {
		goto err2;
	};

	return B_NO_ERROR;
	
err2:
	pc_uninit_driver();
	put_module(pci_name);
err1:
	dprintf("pc_init_card failed %x\n", status);
	return status;


#endif

#if ISA_PNP_CARD
	if((status = get_module(B_ISA_MODULE_NAME, (module_info **)&isa)) != B_OK) {
		dprintf(kDevName " Get isa module failed! %s\n", strerror(status ));
		return status;
	}
		
	/* Find Lan cards*/
	if ((entries = get_isa_list(MAX_CARDS )) == 0) {
		dprintf(kDevName " not found\n");
		put_module( B_ISA_MODULE_NAME );
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
#endif
	
	return B_OK;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;

#if PCMCIA_CARD
	pc_uninit_driver();
#else 
	/*Free device name list*/
	for (i=0; (item=gDevNameList[i]); i++)
		free(item);
	
	/*Free device list*/
	put_module(B_ISA_MODULE_NAME);

#endif

}


device_hooks *find_device(const char *name)
{
	int32 	i;
	char 	*item;


#if PCMCIA_CARD		// to do: fix for multiple pc cards

	return &gDeviceHooks;

#else  // ISA_PNP_CARD
	
	/* Find device name */
	for (i=0; (item=gDevNameList[i]); i++)
	{
		if (strcmp(name, item) == 0)
		{
			return &gDeviceHooks;
		}
	}

	dprintf("device %s not found\n", name);
	
	return NULL; /*Device not found */

#endif
}


static status_t open_hook(const char *name, uint32 flags, void **cookie) {

	int32				devID;
	int32				mask;
	status_t			status;
	char 				*devName;
	ether_dev_info_t 	*device;
	uint16 				i;
	int 				nd;



#if PCMCIA_CARD
devID=0;				// to do: fix for multiple instances
#else 
	/*	Find device name*/
	for (devID=0; (devName=gDevNameList[devID]); devID++) {
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;
#endif
	
	/* Check if the device is busy and set in-use flag if not */
	mask = 1 << devID;
	if (atomic_or(&gOpenMask, mask) &mask) {
		dprintf(kDevName " busy\n");
		return B_BUSY;
	}

	/*	Allocate storage for the cookie*/
	if (!(*cookie = device = (ether_dev_info_t *)malloc(sizeof(ether_dev_info_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(ether_dev_info_t));
	


#if PCMCIA_CARD

    for (nd = 0; nd < MAX_DEVS; nd++) {
// dprintf("comparing %s to %s\n", devs[nd]->dev->dev_name, name);
   if ((devs[nd] != NULL) &&
        (strcmp(devs[nd]->dev->dev_name, name) == 0))
        break;
    }
	if (nd != MAX_DEVS){
		device->irq = devs[nd]->irq.AssignedIRQ;
		device->port = devs[nd]->io.BasePort1;
		dprintf("%s: irq %d / iobase 0x%04x\n",devs[nd]->dev->dev_name,
				device->irq, device->port);
	
	
	
	
	} else {
		dprintf("err dev not found \n");
   		free(device);
		status=B_ERROR;
		goto err1;
	}
#endif
#if ISA_PNP_CARD
	/* per driver intance globals */
	device->devID = devID;
	device->irq = gDevInfo[devID].irq;
	device->port = gDevInfo[devID].port;
#endif
		
	device->debug = DEBUG_FLAGS;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ": open %s device=%x ", name, device);

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	
#if DEBUGGER_COMMAND
	gdev = device;
	add_debugger_command (kDevName, tcm5x9, "Ethernet driver Info");
#endif

	write16(device->port + Wx_CMD, TxReset);
	write16(device->port + Wx_CMD, RxReset);
	write16(device->port + Wx_CMD, SetReadZeroMask);

	/* allocate and initialize frame buffer rings & descriptors */
	if (init_ring_buffers(device) != B_OK)
		goto err1;

	
	get_mac_address(device);

	install_io_interrupt_handler( device->irq, interrupt_hook, *cookie, 0 );


//	ETHER_DEBUG(device, INFO, " irq=%x port=%x status=%4.4x.\n",
//		device->irq, device->port, read16(device->port + Wx_STATUS));
	
#ifdef ISA_PNP_CARD

	write16(device->port + 4, 1); /* in ISA we must set, in EISA BIOS sets -- is this needed? rhs*/
	/* Set the IRQ line. */
	write16(device->port + W0_IRQ, (device->irq << 12) | 0x0f0);
#endif

	/* Set the station address in window 2 each time opened. */
	SET_WINDOW(2);
	for (i = 0; i < 6; i++)
		write8(device->port + i, device->macAddr[i]);

	load_media_settings(&device->media, &device->speed, &device->duplex);

	if (device->media == COAX ) {
		SET_WINDOW(0);
		write16(device->port + ADDRESS_CONFIG, (read16(device->port + ADDRESS_CONFIG) & 0xFFF) | COAX);

		SET_WINDOW(4);
		write16(device->port + W4_MEDIA, read16(device->port + W4_MEDIA)  & ~( Jabber | LinkBeatEnable));
		write16(device->port + Wx_CMD, RxReset);
		write16(device->port + Wx_CMD, TxReset);
		snooze (70000); 
		write16(device->port + Wx_CMD, StartCoax);
		dprintf(kDevName " COAX media = %x\n", read16(device->port + W4_MEDIA));	
	} else  if (device->media == AUI) {
		SET_WINDOW(0);
		write16(device->port + ADDRESS_CONFIG, (read16(device->port + ADDRESS_CONFIG) & 0xFFF) | AUI);
		SET_WINDOW(4);
		write16(device->port + W4_MEDIA, /*read16(device->port + W4_MEDIA) |*/ ~(Jabber | LinkBeatEnable));
		write16(device->port + Wx_CMD, RxReset);
		write16(device->port + Wx_CMD, TxReset);
		snooze (70000); 
		dprintf(kDevName " 10BaseT media = %x\n", read16(device->port + W4_MEDIA));
	
	} else { /* default: 10BaseT twisted pair rj45 */
		SET_WINDOW(0);
		write16(device->port + ADDRESS_CONFIG, (read16(device->port + ADDRESS_CONFIG) & 0xFFF) | RJ45);
		SET_WINDOW(4);
		write16(device->port + W4_MEDIA, /*read16(device->port + W4_MEDIA) |*/ Jabber | LinkBeatEnable);
		write16(device->port + Wx_CMD, RxReset);
		write16(device->port + Wx_CMD, TxReset);
		snooze (70000); 
		dprintf(kDevName " 10BaseT media = %x\n", read16(device->port + W4_MEDIA));
	} 

	/* Clear Statistics */
	write16(device->port + Wx_CMD, StatsDisable);
	SET_WINDOW(6);
	for (i = 0; i < 9; i++)
		read8(device->port + i);
	read16(device->port + 10);
	read16(device->port + 12);

	SET_WINDOW(1);

	/* set recevie filter for broadcast and our mac address */
	write16(device->port + Wx_CMD, SetRxFilter | RxStation | RxBroadcast);
	/* Statistics on */
//	write16(device->port + Wx_CMD, StatsEnable);
	write16(device->port + Wx_CMD, StatsDisable);
//dprintf(kDevName " statistics disabled \n");
	/* Receiver and Transmitter on  */
	write16(device->port + Wx_CMD, RxEnable);
	write16(device->port + Wx_CMD, TxEnable);

	write16(device->port + Wx_CMD, SetReadZeroMask | 0xff);

	
	/* Enable interrupts */
//	write16(device->port + Wx_CMD, AckInterrupt | InterruptLatch | TxAvailable | RxEarly | InterruptRequest);
	write16(device->port + Wx_CMD, SetIntrMask|InterruptLatch|AdapterFail|TxAvailable|TxComplete|RxComplete|UpdateStatistics);

	/* Interrupts are enabled and the hardware register Window is set to 1.
	   If you want to call SET_WINDOW() now, say for gathering & updating statistics,
	   you will need to add spinlocks to protect conflicts between the interrupt code
	   and the other driver code to be safe on multi-processor machines.
	*/

 	return B_NO_ERROR;
		
	err1:
		free_resources(device);
		free(device);	
	
	err0:
		atomic_and(&gOpenMask, ~mask);
		ETHER_DEBUG(device, ERR,  " open failed!\n");
		return status;
}

static status_t close_hook(void *_device) {
	ether_dev_info_t *device = (ether_dev_info_t *) _device;

	ETHER_DEBUG(device, FUNCTION, kDevName ": close_hook\n");



	/* Turn off the receiver and transmitter. */
	write16(device->port + Wx_CMD, RxDisable);
	write16(device->port + Wx_CMD, TxDisable);

	/* turn off interrupts */
	write16(device->port + Wx_CMD, SetIntrMask | 0);

	/* Switching back to window 0 disables the IRQ. */
	SET_WINDOW(0);

	// if coax 
		/* Turn off coax xcvr */
//		write16(device->port + Wx_CMD, StopCoax);
//	else 
		/* Disable link beat and jabber, media may change on next open */
	{
		SET_WINDOW(4);
		write16(device->port + W4_MEDIA, read16(device->port + W4_MEDIA) & ~(Jabber | LinkBeatEnable));
	}

#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, tcm5x9);
#endif
	
	free_resources(device);

dprintf("close hook exit\n");			
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	ether_dev_info_t *device = (ether_dev_info_t *) cookie;


	ETHER_DEBUG(device, FUNCTION, kDevName": free %x\n",device);
    /* Remove Interrupt Handler */
	remove_io_interrupt_handler( device->irq, interrupt_hook, cookie );

	free_ring_buffers(device);
	
	/* Device is now available again */
	atomic_and(&gOpenMask, ~(1 << device->devID));

	free(device);
	return 0;
}


/* Standard driver control function */
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len)
{
	ether_dev_info_t *device = (ether_dev_info_t *) cookie;

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x ether_getaddr\n",msg);
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->macAddr[i];
			}
			return B_NO_ERROR;
		}
		case ETHER_INIT:
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x init\n",msg);
			return B_NO_ERROR;
			
		case ETHER_GETFRAMESIZE:
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x get_framesize\n",msg);
			*(uint32 *)buf = MAX_FRAME_SIZE;
			return B_NO_ERROR;
			
		case ETHER_ADDMULTI:
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x add multi\n",msg);
			return (domulti(device, (unsigned char *)buf));
		
		case ETHER_SETPROMISC:
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x set promiscuous\n",msg);
			return setpromisc(device, *(uint32 *)buf);
		
		case ETHER_NONBLOCK:
			ETHER_DEBUG(device, FUNCTION, kDevName ": control %x blocking\n",msg);

			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_NO_ERROR;
			
		default:
			ETHER_DEBUG(device, ERR, kDevName ": unknown iocontrol %x\n",msg);
			return B_ERROR;
	}

}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	ether_dev_info_t  *device = (ether_dev_info_t *) _device;
	status_t    status;
			
	/* Block until data is available */
	if ((status = acquire_sem_etc(device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0)) != B_NO_ERROR) {
		*len = 0;
		return status;
	}
	/* Prevent reentrant read */
	if (atomic_or(&device->readLock, 1 )) {
		release_sem_etc( device->ilock, 1, 0 );
		*len = 0;
		return B_ERROR;
	}

	*len = device->rx_desc[device->rx_acked] & 0x7ff;
	
	memcpy(buf, device->rx_buf[device->rx_acked], *len);
	
	device->rx_acked = (device->rx_acked + 1) & RX_BUFFER_MASK;
	
	if (device->debug & RX) {
		if (*len)
			dump_packet("rx", buf, *len);
	}

	device->readLock = 0; /* release reantrant lock */
	return B_OK;
}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len)
{
	ether_dev_info_t  *device = (ether_dev_info_t *) _device;
	int16        frame_size;
	status_t      status;
	int16         tx_needed;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ":write buf %x len %d (decimal)\n",buf,*len);
 
	if( *len > MAX_FRAME_SIZE ) {
		ETHER_DEBUG(device, ERR, kDevName ": write %d > MAX_FRAME_SIZE tooo long\n",*len);
		*len = MAX_FRAME_SIZE;
	}
	frame_size = *len;
	

	/* Prevent reentrant write */
	if (atomic_or( &device->writeLock, 1 )) {
		ETHER_DEBUG(device, ERR, "write_hook: reentrant write\n"); /* this should never happen */
		snooze(5000);
		*len = 0;
		return B_ERROR;
	}
	
	tx_needed = ((*len + 3) & ~0x3) + 4; /* round up to long, add 4 bytes for checksum */
	{	/* critical region - see interrupt_hook */
		cpu_status former; 
		uint16 tx_free;
		
		former = disable_interrupts();
		acquire_spinlock( &device->WriteFifo );
	
		tx_free = read16(device->port + TX_FREE);
//dprintf("write_hook needed=%x free = %x\n", tx_needed, tx_free);		
		if (tx_needed < tx_free - 1520) {	/* round up to long, add checksum */
			;
		} else {
			device->WritePending = tx_needed;		
		}
		release_spinlock( &device->WriteFifo );
		restore_interrupts(former);
		
	}
	
	if (device->WritePending) {

#if 0			
int32		sem_count;
get_sem_count(device->olock, &sem_count );

dprintf("write_hook %x needed count=%x \n", device->WritePending, sem_count); /* tx full */
#endif
		write16(device->port + Wx_CMD, SetTxAvailableThreshold + 1520); /* generate an interrupt when space is free */

		if( (status = acquire_sem_etc( device->olock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR ) {
			*len = 0;
			device->writeLock = 0;
			return B_ERROR;
		}	
	} 
	/* write the frame header */
	
	write16(device->port + TX_DATA, *len);
	write16(device->port + TX_DATA, 0);
	/* write the frame data */
	{
		uint32 *bufL = (uint32 *) buf;
		uint16 len_in_longs = ( *len + 3) >> 2;
		/* ... and the packet rounded to a doubleword. */
		while (len_in_longs--) 
			write32(device->port + TX_DATA, *bufL++);
	}

	if (device->debug & TX) {
		dump_packet("tx", (unsigned char *) buf, frame_size);
	}

	/* Another write may now take place */
	device->writeLock = 0;
	return B_OK;
}

static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length ) {

	return B_ERROR;
};

static void rx_interrupt(ether_dev_info_t * device) {
	uint16 rx_status;
	
	if ((rx_status = read16(device->port + RX_STATUS)) > 0) {
		if (rx_status & 0x4000) { /* Error, update stats. */
			short error = rx_status & 0x3800;

			switch (error) {
			case 0x0000:		device->stats_rx_overrun++; break;
			case 0x0800:		device->stats_rx_len_err++; break;
			case 0x1000:		device->stats_rx_frame_err++; break;
			case 0x1800:		device->stats_rx_len_err++; break;
			case 0x2000:		device->stats_rx_frame_err++; break;
			case 0x2800:		device->stats_rx_crc_err++; break;
			}
			
			write16(device->port + Wx_CMD, RxDiscard); 					/* discard the frame (with or without errs) */

		} else { /* good frame received */
			short rx_len = rx_status & 0x7ff;
			short rx_len_in_longs = (rx_len+3) >> 2; 	/* round up */
			int j = 0;

			if (rx_len > MAX_FRAME_SIZE) {
				ETHER_DEBUG(device, ERR, "rx len > 1514 %d\n", rx_len);
				rx_len = min(MAX_FRAME_SIZE, rx_len);
			}

			device->rx_desc[device->rx_received] = rx_len;

			/* copy the data from the card hardware PIO register --- be careful with odd length frames */
			{
			uint32 *rx_buf = (uint32 *) device->rx_buf[device->rx_received];
				while (rx_len_in_longs--) {
					*rx_buf++ = read32(device->port + RX_DATA);
				}
			}
			device->rx_received = (device->rx_received + 1) & RX_BUFFER_MASK;
			release_sem_etc( device->ilock, 1, B_DO_NOT_RESCHEDULE );
			device->stats_rx++;
			write16(device->port + Wx_CMD, RxDiscard); 					/* discard the frame (with or without errs)  */
		}
		{ 	uint32 limit = 1000;
			while ((read16(device->port + Wx_STATUS) & CmdInProgress) && (limit--)){} ; /* wait for discard completion */
		}
	}
}

/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	ether_dev_info_t *device = (ether_dev_info_t *) _device;
	int32 handled = B_UNHANDLED_INTERRUPT;
	int32 status;
			
	if ((status = read16(device->port + Wx_STATUS)) & InterruptLatch) {
	
		ETHER_DEBUG(device, INTERRUPT, kDevName ": ISR %x\n", status);	
		handled = B_INVOKE_SCHEDULER;
		device->handled_int++;  						/* update stats */

		if (status & RxComplete) {	
			rx_interrupt(device);
		}
		if (status & TxAvailable) {
			uint16 tx_free;
#if 0
			int32		write_sem_count;

			get_sem_count( device->olock, &write_sem_count );
			dprintf("ISR sem=%x writeP=%x avail=%x\n", write_sem_count,
				device->WritePending, read16(device->port+TX_FREE));
#endif	

			acquire_spinlock( &device->WriteFifo );
			tx_free = read16(device->port + TX_FREE);
			if (device->WritePending) {
			/* round up to long, add checksum */
//dprintf("ISR releaseing sem WritePending=%x <-0\n");
				release_sem_etc( device->olock, 1, B_DO_NOT_RESCHEDULE );
				device->WritePending = 0;
			} else {
				kprintf("tx_avail - this should never happen %x\n", device->WritePending);
			}
			release_spinlock( &device->WriteFifo );
		}
		
		if (status & (AdapterFail | RxEarly | UpdateStatistics | TxComplete)) {
			dprintf(kDevName " ISR unusual int %x %s %s %s %s\n", status,
				status & AdapterFail ? "AdapterFail":" ",
				status & RxEarly ? "RxEarly":" ",
				status & UpdateStatistics ? "UpdateStatistics":" ",
				status & TxComplete ? "TxComplete":" "
			);
			if (status & AdapterFail) {
				dump_window(device, 1);
				write16(device->port + Wx_CMD, RxReset);
				/* Set the Rx filter to the current state. */
				write16(device->port + Wx_CMD, SetRxFilter | RxStation | RxBroadcast |
					(device->MultiOn ? RxMulticast : 0)| (device->PromiscOn ? RxPromiscuous : 0));
				write16(device->port + Wx_CMD, RxEnable); /* Re-enable the receiver. */
			}
		}
		write16(device->port+ Wx_CMD, AckInterrupt | InterruptRequest | (status & 0xff)); /* clear the interrupt */
	}
		
	return handled;
}

static uchar
log2(uint32 mask)
{
  uchar value;

  mask >>= 1;
  for (value = 0; mask; value++)
        mask >>= 1;

  return value;
}

#if ISA_PNP_CARD
static int32 get_isa_list(int32 maxEntries)
{
	status_t	status;
	int32 		i;
	uint64 		cookie = 0;
	int32 		entries = 0;
	uint32		pid;
	struct isa_device_info info;
	config_manager_for_driver_module_info *cfg_mgr;

		
    status = get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&cfg_mgr);
	if (status != B_OK) {
		dprintf("Get config mgr module failed %s\n",strerror(status ));
		return 0;
	}

	MAKE_EISA_PRODUCT_ID((EISA_PRODUCT_ID *)&pid, VENDOR_ID_0, VENDOR_ID_1, VENDOR_ID_2, DEVICE_ID, 1);	

	
	for (i=0, entries=0; entries<maxEntries; i++) {
		struct device_configuration *config;
		resource_descriptor r;
		status_t size;

		if (cfg_mgr->get_next_device_info(B_ISA_BUS, &cookie, &info.info, sizeof(info)) != B_OK)
			break;

		/* match the vendor and device id */
		if (!EQUAL_EISA_PRODUCT_ID((EISA_PRODUCT_ID)pid, (EISA_PRODUCT_ID)info.vendor_id))
				continue;

		/* found a card */
		size = cfg_mgr->get_size_of_current_configuration_for(cookie);
		if (size < 0) {
			dprintf("Error getting size of configuration %x\n", size);
			continue;
		}

		config = malloc(size);
		if (!config) {
			dprintf(kDevName "Out of memory\n");
			put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
			break;
		}

		status = cfg_mgr->get_current_configuration_for(cookie, config, size);
		if (status < B_OK) {
			dprintf(kDevName "Error getting current configuration\n");
			free(config);
			continue;
		}

		/* sanity check card */
		if ((cfg_mgr->count_resource_descriptors_of_type(config, B_IRQ_RESOURCE) != 1) ||
				(cfg_mgr->count_resource_descriptors_of_type(config, B_DMA_RESOURCE) != 0) ||
				(cfg_mgr->count_resource_descriptors_of_type(config, B_IO_PORT_RESOURCE) != 1) ||
				(cfg_mgr->count_resource_descriptors_of_type(config, B_MEMORY_RESOURCE) != 0)) {
			dprintf(kDevName " Bad resource counts IRQ=%d  DMA=%d PORT=%d MEM=%d\n",
				cfg_mgr->count_resource_descriptors_of_type(config, B_IRQ_RESOURCE),
				cfg_mgr->count_resource_descriptors_of_type(config, B_DMA_RESOURCE),
				cfg_mgr->count_resource_descriptors_of_type(config, B_IO_PORT_RESOURCE),
				cfg_mgr->count_resource_descriptors_of_type(config, B_MEMORY_RESOURCE));
			free(config);
			continue;
		}

		status = cfg_mgr->get_nth_resource_descriptor_of_type(config, 0, B_IRQ_RESOURCE, &r, sizeof(r));
		if (status < 0) {
			dprintf(kDevName "Error getting IRQ %x\n", status);
			free(config);
			continue;
		}

		gDevInfo[entries].irq = log2(r.d.m.mask);
		
		status = cfg_mgr->get_nth_resource_descriptor_of_type(config, 0, B_IO_PORT_RESOURCE, &r, sizeof(r));
		free(config);
		if (status < 0) {
			dprintf("Error getting I/O range %x\n", status);
			continue;
		}

		if (r.d.r.len != 0x10) {
			dprintf("I/O range must be 0x20 not %x\n", r.d.r.len);
			continue;
		}

		gDevInfo[entries].port = r.d.r.minbase;
		dprintf(kDevName " at irq %x port %x\n", gDevInfo[entries].irq, gDevInfo[entries].port);
		entries++;
	}

	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);

	/* check for jumpered ISA kernel settings - just support 1 card for now */
	{ uint16 irq; uint32 port;
		if (load_irq_settings(&irq, &port)) {
		
			gDevInfo[0].irq = irq;
			gDevInfo[0].port = port;
			return 1;
		}
	}

	return entries;
}

#endif /* ISA_PNP_CARD */

#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t init_ring_buffers(ether_dev_info_t *device)
{
	uint32 	size;
	physical_entry		entry;
	uint16 i;

	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(device, ERR, kDevName " create rx buffer area failed %x \n", device->rx_buf_area);
		return device->rx_buf_area;
	}

	/* init rx buffer */
	for ( i = 0; i < RX_BUFFERS; i++) {
		device->rx_buf[i] = device->rx_buf[0] + (i * BUFFER_SIZE);
	}

	/* initialize frame indexes */
	device->rx_received = device->rx_acked = 0;

	return B_OK;
}

static void free_ring_buffers(ether_dev_info_t *device) {

		delete_area(device->rx_buf_area);

}


static void get_mac_address(ether_dev_info_t *device) {

	uint16 j, word_read;
	
	ETHER_DEBUG(device, INFO, kDevName " mac address ");
	
	SET_WINDOW(0);

	for (j=0; j<3; j++) {
		word_read = read_eeprom(device, j);
		device->macAddr[j*2] = (word_read >> 8) & 0xFF;
		device->macAddr[j*2+1] = word_read & 0xFF;
	}

	/* print mac address */
	for (j=0; j<6; j++) {
		dprintf("%2.2x%c", device->macAddr[j], j==5?' ':':');
	}
	dprintf("\n");
}


/* set hardware so all packets are received. */
static status_t setpromisc(ether_dev_info_t * device, uint32 On) {
	ETHER_DEBUG(device, FUNCTION, kDevName ":setpormisc\n");

	device->PromiscOn = (On ? RxPromiscuous: 0);

	write16(device->port + Wx_CMD, SetRxFilter | RxStation | RxBroadcast | device->MultiOn | device->PromiscOn );
	
	dprintf("Promiscuous Rx %s\n", (On ? "On" : "Off"));
	
	return(B_ERROR);
}

static status_t domulti(ether_dev_info_t *device, uint8 *addr) {
	
	ETHER_DEBUG(device, FUNCTION,kDevName ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
					addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	device->MultiOn = RxMulticast;
	
	write16(device->port + Wx_CMD, SetRxFilter | RxStation | RxBroadcast |
					device->MultiOn | device->PromiscOn );
	
	return (B_NO_ERROR);
}

static void reset_device(ether_dev_info_t *device) {
	ETHER_DEBUG(device, FUNCTION,kDevName ": reset_device reset the NIC hardware\n"); 
};

static void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d \n", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
}

static void dump_rx_desc(ether_dev_info_t *device) {
#if 0
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("rx desc %8.8x \n", device->rx_desc);
	for (j=0; j < RX_BUFFERS; j++ ) {
		kprintf("rx_desc[%2.2d]=...\n", j);
	}
#endif
}

static void dump_tx_desc(ether_dev_info_t *device) {
#if 0
	uint16 j;
	/* kprintf is used here since this is called from the serial debug command */
	kprintf("tx desc %8.8x \n", device->tx_desc);
	
	for (j=0; j < TX_BUFFERS; j++ ) {
		kprintf("tx_desc[%2.2d]...\n",j);
	}
#endif
}

static void dump_window(ether_dev_info_t *device, uchar window) {
	uint16 j;
	
	if (window > 8) {
		dprintf("no such window %d\n", window);
		return;
	}
	dprintf("3c589 window %d\n", window);

	SET_WINDOW(window);
	for (j=0; j<16; j += 2) {
		dprintf("%2.2d: %8.8x\n", j, read16(device->port + j));
	}
}



static void load_media_settings(uint16 *media, uint16 *speed, uint16 *duplex) {
	void *handle;
	const driver_settings *settings;

	handle = load_driver_settings(kDevName);
	settings = get_driver_settings(handle);
	if (settings) {
		int i;
		for (i=0;i<settings->parameter_count;i++) {
			driver_parameter *parameter = settings->parameters + i;
			if (strcmp(parameter->name, "media") == 0) {
				if (strcmp(parameter->values[0], "coax") == 0) {
					*media = COAX;
					dprintf(kDevName " forcing coax media\n");
				} else if (strcmp(parameter->values[0], "aui") == 0) {
					*media = AUI;
					dprintf(kDevName " forcing aui media\n");
				} else if (strcmp(parameter->values[0], "rj45") == 0) {
					*media = RJ45;
					dprintf(kDevName " forcing rj45 media\n");
				}
			
			} else if( strcmp(parameter->name, "speed") == 0) {			
				*speed = strtoll(parameter->values[0], NULL, 0);
				dprintf(kDevName "forcing speed to 0x%x\n", *speed);
			} else if (strcmp(parameter->name, "duplex") == 0) {
				*duplex = strtoll(parameter->values[0], NULL, 0);
				dprintf(kDevName "forcing %s duplex\n", *duplex?"full":"half");
			}
		}
	}
	unload_driver_settings(handle);
}

uint8 load_irq_settings(uint16 *irq, uint32 *port) {

	void *handle;
	const driver_settings *settings;
	uint8 found = 0;
	
	handle = load_driver_settings(kDevName);
	settings = get_driver_settings(handle);
	if (settings) {
		int i;
		for (i=0;i<settings->parameter_count;i++) {
			driver_parameter *parameter = settings->parameters + i;
			if (strcmp(parameter->name, "irq") == 0) {
				*irq = strtoll(parameter->values[0], NULL, 0);
				dprintf(kDevName " forcing irq to 0x%x\n", *irq);
				found |= 0x1;
			} else if( strcmp(parameter->name, "port") == 0) {			
				*port = strtoll(parameter->values[0], NULL, 0);
				dprintf(kDevName " forcing port to 0x%x\n", *port);
				found |= 0x2;
			}
		}
	}
	unload_driver_settings(handle);
	if (( found == 1) || (found == 2)) dprintf (kDevName ": must specify port AND irq\n");
	if (found == 0x3) return true; else return false;
}


/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "tcm5x9 arg...",
   for example "AcmeEthernet R" to enable a received packet trace.
*/
#if DEBUGGER_COMMAND
static int
tcm5x9(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: tcm5x9 { Function_calls | Interrupts | Number_frames | BUS_IO | Stats | Rx_trace | Tx_trace }\n";	
	

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
//			kprintf("received %d,  transmitted %d\n", gdev->stats_rx, gdev->stats_tx);
//			kprintf("rx err %d,  tx err %d\n", gdev->stats_rx_err, gdev->stats_tx_err);
//			kprintf("rx overrun %d,  tx underrun %d\n", gdev->stats_rx_overrun, gdev->stats_tx_underrun);
//			kprintf("collisions %d\n", gdev->stats_collisions);
//			kprintf("handled %d and unhandled %d interrupts\n", gdev->handled_int, gdev->unhandled_int);
			break; 
		case 'b':
		case 'B':
			gdev->debug ^= BUS_IO;
			if (gdev->debug & BUS_IO) 
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

static void clear_statistics(ether_dev_info_t *device) {
	
	device->handled_int = device->unhandled_int = 0;
	device->stats_rx = device->stats_tx = 0;
//	device->stats_rx_err = device->stats_tx_err = 0;
	device->stats_rx_overrun = device->stats_tx_underrun = 0;
	device->stats_collisions = 0;
}

/*
 * Allocate and initialize semaphores and spinlocks.
 */
static status_t allocate_resources(ether_dev_info_t *device) {

	/* Setup Semaphores */
	if ((device->ilock = create_sem(0, kDevName " rx")) < 0) {
		dprintf(kDevName " create rx sem failed %x \n", device->ilock);
		return (device->ilock);
	}
	set_sem_owner(device->ilock, B_SYSTEM_TEAM);
	
	/* Intialize tx semaphore to zero.
	   The semaphore is used to block the write_hook when the free space
	   in the tx fifo is low. The interrupt routine will release the
	   semaphore as data flows out the wire.
	 */
	if ((device->olock = create_sem(0, kDevName " tx")) < 0) {
		delete_sem(device->ilock);
		dprintf(kDevName " create tx sem failed %x \n", device->olock);
		return (device->olock);
	}
	set_sem_owner(device->olock, B_SYSTEM_TEAM);

	device->readLock = device->writeLock = 0;

	device->WriteFifo = 0;
	device->WritePending= 0;

	device->blockFlag = 0;
		
	return (B_OK);
}

static void free_resources(ether_dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->olock);
}



#define		READ_EEPROM 0x80	// endian swap constants for ppc
#define		EEPROM_BUSY 0x8000

/* wait for the hardware to complete the eeprom command */
void wait_eeprom(ether_dev_info_t *device) {
	int limit = 3;

	while (limit--) {
		snooze(100);
		if ((read16(device->port+EEPROM_CMD) & EEPROM_BUSY)== 0) return;
	}
	if (limit == 0) dprintf(kDevName " error reading eeprom\n");
}

/*
 Read eeprom thru the chips window 0 registers.
 */
static uint16 read_eeprom(ether_dev_info_t *device, int32 index)
{
	write16(device->port + EEPROM_CMD,  READ_EEPROM + index);	// endian swap for ppc
	snooze(200);
//	dprintf("read eeprom %x %x\n", device->port+EEPROM_CMD, READ_EEPROM + index);
	
	wait_eeprom(device);
	
	return read16(device->port + EEPROM_DATA);
}

#if PCMCIA_CARD

/*** PCMCIA STUFF ***/

/*
   All the PCMCIA modules use PCMCIA_DEBUG to control debugging.  If
   you do not define PCMCIA_DEBUG at all, all the debug code will be
   left out.  If you compile with PCMCIA_DEBUG=0, the debug code will
   be present but disabled -- but it can then be enabled for specific
   modules at load time with a 'pc_debug=#' option to insmod.
*/
#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
static const char *version = "etherpc{i,mcia}.c v 1/2 swetland";
#define DEBUG(n, args) do { if (pc_debug>(n)) printk args; } while (0)
#else
#define DEBUG(n, args) do { } while (0)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

/* List of interrupts to choose from */
static int irq_list[4] = { -1 };

/*====================================================================*/

/*
   The event() function is this driver's Card Services event handler.
   It will be called by Card Services when an appropriate card status
   event is received.  The config() and release() entry points are
   used to configure or release a socket, in response to card
   insertion and ejection events.  They are invoked from the dummy
   event handler. 
*/

static void dummy_config(dev_link_t *link);
static void dummy_release(u_long arg);
static int dummy_event(event_t event, int priority,
		       event_callback_args_t *args);

/*
   The attach() and detach() entry points are used to create and destroy
   "instances" of the driver, where each instance represents everything
   needed to manage one actual PCMCIA card.
*/

static dev_link_t *dummy_attach(void);
static void dummy_detach(dev_link_t *);

/*
   The dev_info variable is the "key" that is used to match up this
   device driver with appropriate cards, through the card configuration
   database.
*/

static dev_info_t dev_info = "tcm589";

/*
   An array of "instances" of the dummy device.  Each actual PCMCIA
   card corresponds to one device instance, and is described by one
   dev_link_t structure (defined in ds.h).
*/


/*
   A dev_link_t structure has fields for most things that are needed
   to keep track of a socket, but there will usually be some device
   specific information that also needs to be kept track of.  The
   'priv' pointer in a dev_link_t structure can be used to point to
   a device-specific private data structure, like this.

   A driver needs to provide a dev_node_t structure for each device
   on a card.  In some cases, there is only one device per card (for
   example, ethernet cards, modems).  In other cases, there may be
   many actual or logical devices (SCSI adapters, memory cards with
   multiple partitions).  The dev_node_t structures need to be kept
   in a linked list starting at the 'dev' field of a dev_link_t
   structure.  We allocate them in the card's private data structure,
   because they generally shouldn't be allocated dynamically.

   In this case, we also provide a flag to indicate if a device is
   "stopped" due to a power management event, or card ejection.  The
   device IO routines can use a flag like this to throttle IO to a
   card that is not ready to accept it.
*/
   
typedef struct local_info_t {
    dev_node_t	node;
    int		stop;
} local_info_t;

/*
  BeOS bus manager declarations: we use the shorthand references to
  bus manager functions to make the code somewhat more portable across
  platforms.
*/
static cs_client_module_info *cs = NULL;
static ds_module_info *ds = NULL;
#define CardServices		cs->_CardServices
#define register_pccard_driver	ds->_register_pccard_driver
#define unregister_pccard_driver ds->_unregister_pccard_driver

/*====================================================================*/

static void cs_error(client_handle_t handle, int func, int ret)
{
    error_info_t err;
    err.func = func; err.retcode = ret;
    CardServices(ReportError, handle, &err);
}

/*======================================================================

    dummy_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

    The dev_link structure is initialized, but we don't actually
    configure the card at this point -- we wait until we receive a
    card insertion event.
    
======================================================================*/

static dev_link_t *dummy_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    local_info_t *local;
    int ret, i;
    
    DEBUG(0, ("dummy_attach()\n"));

    /* Find a free slot in the device table */
    for (i = 0; i < MAX_DEVS; i++)
	if (devs[i] == NULL) break;
    if (i == MAX_DEVS) {
	printk("dummy_cs: no devices available\n");
	return NULL;
    }
    
    /* Initialize the dev_link_t structure */
    link = malloc(sizeof(struct dev_link_t));
    memset(link, 0, sizeof(struct dev_link_t));
    devs[i] = link;
    ndevs++;
    link->release.function = &dummy_release;
    link->release.data = (u_long)link;

    /* Interrupt setup */
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
    if (irq_list[0] == -1)
	link->irq.IRQInfo2 = 0xdeb8;
    else
	for (i = 0; i < 4; i++)
	    link->irq.IRQInfo2 |= 1 << irq_list[i];
    link->irq.Handler = NULL;
    
    /*
      General socket configuration defaults can go here.  In this
      client, we assume very little, and rely on the CIS for almost
      everything.  In most clients, many details (i.e., number, sizes,
      and attributes of IO windows) are fixed by the nature of the
      device, and can be hard-wired here.
    */
    link->conf.Attributes = 0;
    link->conf.Vcc = 50;
    link->conf.IntType = INT_MEMORY_AND_IO;

    /* Allocate space for private device-specific data */
    local = malloc(sizeof(local_info_t));
    memset(local, 0, sizeof(local_info_t));
    link->priv = local;

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &dummy_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
	cs_error(link->handle, RegisterClient, ret);
	dummy_detach(link);
	return NULL;
    }

    return link;
} /* dummy_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void dummy_detach(dev_link_t *link)
{
    int i;
    
    DEBUG(0, ("dummy_detach(0x%p)\n", link));
    
	if(link == NULL) {
		dprintf("dummy_detach called with NULL argumant\n");
		return;
	}

    /* Locate device structure */
    for (i = 0; i < MAX_DEVS; i++)
	if (devs[i] == link) break;
    if (i == MAX_DEVS)
	return;

    /*
       If the device is currently configured and active, we won't
       actually delete it yet.  Instead, it is marked so that when
       the release() function is called, that will trigger a proper
       detach().
    */
    if (link->state & DEV_CONFIG) {
	DEBUG(0, ("dummy_cs: detach postponed, '%s' "
		  "still locked\n", link->dev->dev_name));
	link->state |= DEV_STALE_LINK;
	return;
    }

    /* Break the link with Card Services */
    if (link->handle)
	CardServices(DeregisterClient, link->handle);
    
    /* Unlink device structure, free pieces */
    devs[i] = NULL;
    if (link->priv)
	free(link->priv);
    free(link);
    ndevs--;
    
} /* dummy_detach */

/*======================================================================

    dummy_config() is scheduled to run after a CARD_INSERTION event
    is received, to configure the PCMCIA socket, and to make the
    device available to the system.
    
======================================================================*/

#define CS_CHECK(fn, h, a) \
while ((last_ret=CardServices(last_fn=(fn),h,a))!=0) goto cs_failed
#define CS_CHECK2(fn, h, a, b) \
while ((last_ret=CardServices(last_fn=(fn),h,a,b))!=0) goto cs_failed

#define CFG_CHECK(fn, h, a) \
if (CardServices(fn, h, a) != 0) goto next_entry
#define CFG_CHECK2(fn, h, a, b) \
if (CardServices(fn, h, a, b) != 0) goto next_entry

static void dummy_config(dev_link_t *link)
{
	client_handle_t handle;
	tuple_t tuple;
	cisparse_t parse;
	local_info_t *dev;
	int last_fn, last_ret, nd;
	u_char buf[64];
	win_req_t req;
	memreq_t map;
	
	handle = link->handle;
	dev = link->priv;
	
	DEBUG(0, ("dummy_config(0x%p)\n", link));
	
	/*
	   This reads the card's CONFIG tuple to find its configuration
	   registers.
	*/
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK2(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];
	
	/* Configure card */
	link->state |= DEV_CONFIG;
	
	/*
	  In this loop, we scan the CIS for configuration table entries,
	  each of which describes a valid card configuration, including
	  voltage, IO window, memory window, and interrupt settings.
	
	  We make no assumptions about the card to be configured: we use
	  just the information available in the CIS.  In an ideal world,
	  this would work for any PCMCIA card, but it requires a complete
	  and accurate CIS.  In practice, a driver usually "knows" most of
	  these things without consulting the CIS, and most client drivers
	  will only use the CIS to fill in implementation-defined details.
	*/
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t dflt = { 0 };
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK2(ParseTuple, handle, &tuple, &parse);
		
		link->conf.ConfigIndex = cfg->index;
		
		/* Does this card need audio output? */
		if (cfg->flags & CISTPL_CFTABLE_AUDIO) {
			link->conf.Attributes |= CONF_ENABLE_SPKR;
			link->conf.Status = CCSR_AUDIO_ENA;
		}
	
		/* Use power settings for Vcc and Vpp if present */
		/*  Note that the CIS values need to be rescaled */
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vcc = dflt.vcc.param[CISTPL_POWER_VNOM]/10000;

		if (cfg->vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			                  cfg->vpp1.param[CISTPL_POWER_VNOM]/10000;
		else if (dflt.vpp1.present & (1<<CISTPL_POWER_VNOM))
			link->conf.Vpp1 = link->conf.Vpp2 =
			                  dflt.vpp1.param[CISTPL_POWER_VNOM]/10000;
	
		/* Do we need to allocate an interrupt? */
		if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1)
			link->conf.Attributes |= CONF_ENABLE_IRQ;

		/* IO window settings */
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
/* The newer LinkSys Card (EC2T) doesn't work in CISTPL_IO_16BIT, it requires */
/* IO_DATA_PATH_WIDTH_AUTO to access the pc cards io space. AUTO causes the DLINK*/
/* card to run about 5% slower, and oddly, the older Linksys card to run about*/
/* 3% faster. It may make sense to pick the DATA_PATH_WIDTH differently for different*/
/* cards, for now, one size fits all, AUTO.*/
#if 0
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
#endif
			link->io.BasePort1 = io->win[0].base;
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}
		
		/* This reserves IO space but doesn't actually enable it */
		CFG_CHECK(RequestIO, link->handle, &link->io);
		
		/*
		  Now set up a common memory window, if needed.  There is room
		  in the dev_link_t structure for one memory window handle,
		  but if the base addresses need to be saved, or if multiple
		  windows are needed, the info should go in the private data
		  structure for this device.
		
		  Note that the memory window base is a physical address, and
		  needs to be mapped to virtual space with ioremap() before it
		  is used.
		*/
		if ((cfg->mem.nwin > 0) || (dflt.mem.nwin > 0)) {
			cistpl_mem_t *mem = (cfg->mem.nwin) ? &cfg->mem : &dflt.mem;
			req.Attributes = WIN_DATA_WIDTH_16|WIN_MEMORY_TYPE_CM;
			req.Base = mem->win[0].host_addr;
			req.Size = mem->win[0].len;
			req.AccessSpeed = 0;
			link->win = (window_handle_t)link->handle;
			CFG_CHECK(RequestWindow, &link->win, &req);
			map.Page = 0; map.CardOffset = mem->win[0].card_addr;
			CFG_CHECK(MapMemPage, link->win, &map);
		}
		/* If we got this far, we're cool! */
		break;
		
	next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT) dflt = *cfg;
			CS_CHECK(GetNextTuple, handle, &tuple);
	}
		
	/*
	  Allocate an interrupt line.  Note that this does not assign a
	  handler to the interrupt, unless the 'Handler' member of the
	  irq structure is initialized.
	*/
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	
	/*
	   This actually configures the PCMCIA socket -- setting up
	   the I/O windows and the interrupt mapping, and putting the
	   card and host interface into "Memory and IO" mode.
	*/
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);
	
	/*
	  At this point, the dev_node_t structure(s) need to be
	  initialized and arranged in a linked list at link->dev.
	*/
	for (nd = 0; nd < MAX_DEVS; nd++)
		if (devs[nd] == link) break;
	sprintf(dev->node.dev_name, "net/" kDevName "/%d", nd);
	dev->node.major = dev->node.minor = 0;
	link->dev = &dev->node;
	
	/* Finally, report what we've done */
	printk("%s: index 0x%02x: Vcc %d.%d",
	       dev->node.dev_name, link->conf.ConfigIndex,
	       link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1)
		printk(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		printk(", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
	printk(", io 0x%04x-0x%04x", link->io.BasePort1,
	       link->io.BasePort1+link->io.NumPorts1-1);
	if (link->io.NumPorts2)
		printk(" & 0x%04x-0x%04x", link->io.BasePort2,
	link->io.BasePort2+link->io.NumPorts2-1);
	if (link->win)
		printk(", mem 0x%06lx-0x%06lx", req.Base,
	req.Base+req.Size-1);
	printk("\n");
	
	link->state &= ~DEV_CONFIG_PENDING;
	return;

cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	dummy_release((u_long)link);
} /* dummy_config */

/*======================================================================

    After a card is removed, dummy_release() will unregister the
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void dummy_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;

    DEBUG(0, ("dummy_release(0x%p)\n", link));

    /*
       If the device is currently in use, we won't release until it
       is actually closed, because until then, we can't be sure that
       no one will try to access the device or its data structures.
    */
    if (link->open) {
	DEBUG(1, ("dummy_cs: release postponed, '%s' still open\n",
		  link->dev->dev_name));
	link->state |= DEV_STALE_CONFIG;
	return;
    }

    /* Unlink the device chain */
    link->dev = NULL;

    /*
      In a normal driver, additional code may be needed to release
      other kernel data structures associated with this device. 
    */
    
    /* Don't bother checking to see if these succeed or not */
    if (link->win)
	CardServices(ReleaseWindow, link->win);
    CardServices(ReleaseConfiguration, link->handle);
    if (link->io.NumPorts1)
	CardServices(ReleaseIO, link->handle, &link->io);
    if (link->irq.AssignedIRQ)
	CardServices(ReleaseIRQ, link->handle, &link->irq);
    link->state &= ~DEV_CONFIG;
    
    if (link->state & DEV_STALE_LINK)
	dummy_detach(link);
    
} /* dummy_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.

    When a CARD_REMOVAL event is received, we immediately set a
    private flag to block future accesses to this device.  All the
    functions that actually access the device should check this flag
    to make sure the card is still present.
    
======================================================================*/

static int dummy_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    DEBUG(1, ("dummy_event(0x%06x)\n", event));
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
	link->state &= ~DEV_PRESENT;
	if (link->state & DEV_CONFIG) {
	    ((local_info_t *)link->priv)->stop = 1;
	    link->release.expires = RUN_AT(HZ/20);
	    cs->_add_timer(&link->release);
	}
	break;
    case CS_EVENT_CARD_INSERTION:
	link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
	dummy_config(link);
	break;
    case CS_EVENT_PM_SUSPEND:
	link->state |= DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_RESET_PHYSICAL:
	/* Mark the device as stopped, to block IO until later */
	((local_info_t *)link->priv)->stop = 1;
	if (link->state & DEV_CONFIG)
	    CardServices(ReleaseConfiguration, link->handle);
	break;
    case CS_EVENT_PM_RESUME:
	link->state &= ~DEV_SUSPEND;
	/* Fall through... */
    case CS_EVENT_CARD_RESET:
	if (link->state & DEV_CONFIG)
	    CardServices(RequestConfiguration, link->handle, &link->conf);
	((local_info_t *)link->priv)->stop = 0;
	/*
	  In a normal driver, additional code may go here to restore
	  the device state and restart IO. 
	*/
	break;
    }
    return 0;
} /* dummy_event */


status_t pc_init_driver(void)
{
    client_reg_t client_reg;
    servinfo_t serv;
    bind_req_t bind;
    int i, ret;
    
    for(i=0;i<MAX_DEVS;i++) devs[i] = NULL;
    
    if (get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs) != B_OK) goto err1;
    if (get_module(DS_MODULE_NAME, (struct module_info **)&ds) != B_OK) goto err2;
	
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) goto err3;

    /* When this is called, Driver Services will "attach" this driver
       to any already-present cards that are bound appropriately. */
    register_pccard_driver(&dev_info, &dummy_attach, &dummy_detach);
    return B_OK;
err3:	
	put_module(DS_MODULE_NAME);
err2:
	put_module(CS_CLIENT_MODULE_NAME);
err1:
	return B_ERROR;
}


void pc_uninit_driver(void)
{
    int i;
    if(cs && ds){
        unregister_pccard_driver(&dev_info);
        for (i = 0; i < MAX_DEVS; i++)
        if (devs[i]) {
            if (devs[i]->state & DEV_CONFIG)
            dummy_release((u_long)devs[i]);
            dummy_detach(devs[i]);
        }
        if (ds) put_module(DS_MODULE_NAME);
        if (cs) put_module(CS_CLIENT_MODULE_NAME);
    }
}


const char **publish_devices(void)
{
    int i, nd;

dprintf("tcm589-publish_devices\n");
    for (nd = i = 0; nd < MAX_DEVS; nd++) {
		if (devs[nd] == NULL) continue;
		gDevNameList[i] = (char *)malloc(strlen(devs[nd]->dev->dev_name)+1);
		strcpy(gDevNameList[i++], devs[nd]->dev->dev_name);
    }
    gDevNameList[i] = NULL;
    return (const char **)gDevNameList;
}

#else /* ISA PNP */

const char** publish_devices(void ) {
	dprintf(kDevName ": publish_devices()\n" );
	return (const char **)gDevNameList;
}
#endif




int32	api_version = B_CUR_DRIVER_API_VERSION;
