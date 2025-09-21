/*
 * AMD 79C978 PCI 10MB Ethernet 1 MB PhoneLan driver
 * CopyRight (c) 1999 Be Inc., All Rights Reserved.
 */

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <ether_driver.h>
#include <stdarg.h>
#include <PCI.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <SupportDefs.h>
#include <ByteOrder.h>

/* PCI vendor and device ID's for AMD  AM79C978 */
#define VENDOR_ID_AMD        0x1022
#define DEVICE_ID_AM79C978   0x2001

/* globals for driver instances */ 

#define kDevName "homelan"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4			/* maximum number of driver instances */



//  Choose "memory mapped" or "io port" pci bus access.
#define IO_PORT_PCI_ACCESS true
//#define MEMORY_MAPPED_PCI_ACCESS true

#if IO_PORT_PCI_ACCESS
#define write8(address, value)			(*gPCIModInfo->write_io_8)((address), (value))
#define write16(address, value)			(*gPCIModInfo->write_io_16)((address), (value))
#define write32(address, value)			(*gPCIModInfo->write_io_32)((address), (value))
#define read8(address)					((*gPCIModInfo->read_io_8)(address))
#define read16(address)					((*gPCIModInfo->read_io_16)(address))
#define read32(address)					((*gPCIModInfo->read_io_32)(address))

#else /*  MEMORY_MAPPED_PCI_ACCESS */
#define read8(address)   				(*((volatile uint8*)(address))); __eieio()
#define read16(address)  				(*((volatile uint16*)(address))); __eieio()
#define read32(address) 				(*((volatile uint32*)(address))); __eieio()
#define write8(address, data)  			(*((volatile uint8 *)(address)) = data); __eieio()
#define write16(address, data) 			(*((volatile uint16 *)(address)) = (data)); __eieio()
#define write32(address, data) 			(*((volatile uint32 *)(address)) = (data)); __eieio()
#endif 


/* debug flags */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define PCI_IO    0x0040		/* pci reads and writes */
#define SEQ		  0x0080		/* trasnmit & receive sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off on final release */

/* diagnostic debug flags - compile in here or set while running with debugger "homelan" command */
#define DEBUG_FLAGS ( ERR | INFO | WARN )

/* ring buffer sizes  */
#define LITTLE_RINGS_FOR_TESTING false
#if LITTLE_RINGS_FOR_TESTING
	#define TX_BUFFERS					  4 	/* Must be a power of 2 */
	#define TX_BUFFERS_LOG2				  2		/* 2^^TX_BUFFERS_LOG2 = TX_BUFFERS */
	#define RX_BUFFERS					  4	/* Must be a power of 2 */
	#define RX_BUFFERS_LOG2				  2	
#else
	#define TX_BUFFERS					  16 	/* Must be a power of 2 */
	#define TX_BUFFERS_LOG2				  4		/* 2^^TX_BUFFERS_LOG2 = TX_BUFFERS */
	#define RX_BUFFERS					  16	/* Must be a power of 2 */
	#define RX_BUFFERS_LOG2				  4	
#endif /* LITTLE_RINGS_FOR_TESTING */

#define TX_MASK 					(TX_BUFFERS - 1)
#define RX_MASK						(RX_BUFFERS - 1)

#define BUFFER_SIZE			2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE		1514			/* 1514 + 4 bytes checksum added by card hardware */
static pci_module_info		*gPCIModInfo;
static char 				*gDevNameList[MAX_CARDS+1];
static pci_info 			*gDevList[MAX_CARDS+1];
static int32 				gOpenMask = 0;


const char** publish_devices(void )
{
	dprintf(kDevName ": publish_devices()\n" );
	return (const char **)gDevNameList;
}


/* Driver Entry Points */
__declspec(dllexport) status_t init_hardware(void );
__declspec(dllexport) status_t init_driver(void );
__declspec(dllexport) void uninit_driver(void );
__declspec(dllexport) const char** publish_devices(void );
__declspec(dllexport) device_hooks *find_device(const char *name );



/* Hardware specific definitions for the AMD AM79C978 10MB Ether/ 1 MB Phonelan */

enum AM79C978_register_access {
	RDP=0x10, RAP=0x12, RESET=0x14, BDP=0x16
};

enum MEDIA_SPEED_AND_DUPLEX {
	MEDIA_DEFAULT = 1<<7,		// auto select
	MEDIA_FORCED = 0x8000
};

/* Transmit and receive frame descriptor */
typedef struct frame_descriptor {
    uint32 base;
    int16 length;
    uint16 status;
    uint32 misc;
    uint32 reserved;
} frame_desc_t;

/* Initialization block*/
typedef struct init_block {
    uint16 mode;
    uint16 tx_rx_len;
    uint8  mac_addr[6];
    uint16 reserved;
    uint32 filter[2];
    uint32 rx_ring;
    uint32 tx_ring;
} init_block_t;


/* per driver intance globals */
typedef struct {
	ether_address_t mac_address;								/* my ethernet address */
	int32			devID; 										/* device identifier: 0-n */
	int32			chip_revision;								/* version of the silicon */
	pci_info		*pciInfo;
	uint16			irq;										/* our IRQ line */
	sem_id			ilock, olock;								/* i/o semaphores */
	int32			readLock, writeLock;						/* reentrant reads/writes not allowed */
	int32			blockFlag;									/* for blocking or nonblocking reads */
	area_id			tx_desc_area, tx_buf_area; 					/* transmit descriptor and buffer areas */
	area_id			rx_desc_area, rx_buf_area; 					/* receive descriptor and buffer areas */
	spinlock		rx_spinlock;								/* for read_hook() and rx_interrupt() */
	spinlock		bus_spinlock;								/* for bus io */
	uchar			*tx_buf[TX_BUFFERS], *rx_buf[RX_BUFFERS];	/* tx and rx buffers */
	frame_desc_t	*tx_desc[TX_BUFFERS], *rx_desc[RX_BUFFERS];	/* tx and rx frame descriptors */
	init_block_t	*init_block;								/* used to setup the hardware */
	int16			rx_received, rx_acked;						/* index to in use tx & rx ring buffers */
	int16			tx_sent, tx_acked;							/* index to oldest used tx & rx ring buffers */
	uint32			reg_base; 									/* Base address of hostRegs */
	uint16          csr1,csr2;									/* physical address of init_block */
	uint16			media;										/* speed duplex media settings */
	uint32			debug;										/* debuging level */
	/*  statistics */
	uint32			handled_int, unhandled_int;					/* interrupts rececived & handled */
	uint32			stats_rx, stats_tx;							/* received & transmitted frames */
	uint32			collisions, missed_rx, mem_err, bit14_err;  /* error stats */
// todo - hashed mutlicast filtering
//	ether_address_t multi[MAX_MULTI];							/* multicast address list */
//	uint16	nmulti;												/* number of multicast addresses */
	uint32 version;
} dev_info_t;


/* for serial debug */
#define DEBUGGER_COMMAND true
#if DEBUGGER_COMMAND
dev_info_t * gdev;
#endif


/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len);

static int32    get_pci_list(pci_info *info[], int32 maxEntries);  /* Get pci_info for each device */
static status_t free_pci_list(pci_info *info[]);                   /* Free storage used by pci_info list */
static status_t enable_addressing(dev_info_t *device);             /* enable pci io address space for device */
static status_t init_ring_buffers(dev_info_t *device);             /* allocate and initialize frame buffer rings */
static void     free_ring_buffers(dev_info_t *device);             /* allocate and initialize frame buffer rings */
static void		clear_statistics(dev_info_t *device);			   /* clear statistics counters */
static status_t allocate_resources(dev_info_t *device);       /* allocate semaphores & spinlocks */
static void     free_resources(dev_info_t *device);         	   /* deallocate semaphores & spinlocks */
static void     get_mac_address(dev_info_t *device);               /* get ethernet address */
static void     reset_device(dev_info_t *device);                  /* reset the lan controller (NIC) hardware */
static void     media_init(dev_info_t *device);                    /* initialize the media, speed and duplex settings */
static int32    homelan_interrupt(void *_device);                  /* interrupt handler */
void 			dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
static int 		homelan(int argc, char **argv);						   /* serial debug command */
static status_t domulti(dev_info_t *device, uint8 *addr);		   /* add multicast address to hardware filter list */
static void 	restart_device(dev_info_t *device);				   /* when all else fails */



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

                                                        

#define DEBUG(device, flags, args...) if (device->debug & flags) dprintf(args)


/* register access functions */
static uint16 read_csr(dev_info_t * device, uint32 csr) {
	uint16 value; 
	cpu_status  	former;
	
	former = disable_interrupts();
	acquire_spinlock( &device->bus_spinlock );
    write16(device->reg_base + RAP,csr);
    value =  read16(device->reg_base+RDP);
	release_spinlock( &device->bus_spinlock );
	restore_interrupts(former);

	DEBUG(device, PCI_IO, "read_csr[%3.3lu]=%8.8ux\n",csr, value);   
	return value;
}

static void write_csr(dev_info_t * device, uint16 csr, uint16 value) {
	cpu_status  	former;

	DEBUG(device, PCI_IO,"write_csr[%3.3d]=%8.8x\n",csr, value);   
	former = disable_interrupts();
	acquire_spinlock( &device->bus_spinlock );
	write16(device->reg_base + RAP, csr);
    write16 (device->reg_base + RDP, value);
	release_spinlock( &device->bus_spinlock );
	restore_interrupts(former);
}


static uint16 read_bcr(dev_info_t * device, uint16 bcr) {
    uint16 value;
	cpu_status  	former;

	former = disable_interrupts();
	acquire_spinlock( &device->bus_spinlock );
	write16(device->reg_base + RAP, bcr);
    value = read16(device->reg_base + BDP);
	release_spinlock( &device->bus_spinlock );
	restore_interrupts(former);
	DEBUG(device, PCI_IO, "read_bcr[%3.3d]=%8.8x\n",bcr, value);   
	return value;
}

static void write_bcr(dev_info_t * device, uint16 bcr, uint16 value) {
	cpu_status  	former;

	DEBUG(device, PCI_IO,"write_bcr[%3.3d]=%8.8x\n",bcr, value);   
	former = disable_interrupts();
	acquire_spinlock( &device->bus_spinlock );
    write16(device->reg_base + RAP, bcr);
    write16(device->reg_base + BDP, value);
	release_spinlock( &device->bus_spinlock );
	restore_interrupts(former);
}

static void dump_csrs(dev_info_t * device, char * msg) {
	kprintf("\n----------%s\n", msg);
	kprintf("csr000=%8.8x csr001=%8.8x csr002=%8.8x csr003=%8.8x csr004=%8.8x csr005=%8.8x csr007=%8.8x csr008=%8.8x\n",
		read_csr(device, 0),read_csr(device, 1),read_csr(device, 2),
		read_csr(device, 3),read_csr(device, 4),read_csr(device, 5),
		read_csr(device, 7),read_csr(device, 8));
	kprintf("csr009=%8.8x csr010=%8.8x csr011=%8.8x csr012=%8.8x csr013=%8.8x csr014=%8.8x csr015=%8.8x csr024=%8.8x\n",
		read_csr(device, 9),read_csr(device, 10),read_csr(device, 11),
		read_csr(device, 12),read_csr(device, 13),read_csr(device, 14),
		read_csr(device, 15),read_csr(device, 24));
	kprintf("csr025=%8.8x csr030=%8.8x csr031=%8.8x csr047=%8.8x csr049=%8.8x csr076=%8.8x csr078=%8.8x csr000=%8.8x\n",
		read_csr(device, 25),read_csr(device, 30),read_csr(device, 31),
		read_csr(device, 47),read_csr(device, 49),read_csr(device, 76),
		read_csr(device, 78),read_csr(device, 0));
	kprintf("\n----------\n");


}
static void dump_bcrs(dev_info_t * device, char * msg) {
	kprintf("\n----------%s\n", msg);
	kprintf("bcr009=%8.8x bcr018=%8.8x bcr020=%8.8x bcr032=%8.8x bcr033=%8.8x bcr034=%8.8x bcr048=%8.8x bcr049=%8.8x \n",
		read_bcr(device, 9),read_bcr(device, 18),read_bcr(device, 20),
		read_bcr(device, 32), read_bcr(device, 33), read_bcr(device, 34), read_bcr(device, 48),
		read_bcr(device, 49));
	kprintf("\n----------\n");


}


/*
 * Driver Entry Points
 */
status_t init_hardware(void )
{
	status_t		status;

	return B_NO_ERROR;
}


status_t init_driver()
{
	status_t 		status;
	int32			entries;
	char			devName[64];
	int32			i;
	
//	dprintf(kDevName ": init_driver ");	

	if((status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		dprintf( kDevName " Get module failed! %s\n", strerror(status ));
		return status;
	}
		
	// Find Lan cards
	if ((entries = get_pci_list(gDevList, MAX_CARDS )) == 0) {
		dprintf("init_driver: " kDevName " not found\n");
		free_pci_list(gDevList);
		put_module(B_PCI_MODULE_NAME );
		return B_ERROR;
	}
	
	// Create device name list
	for (i=0; i<entries; i++ )
	{
		sprintf(devName, "%s%ld", kDevDir, i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
	}
	gDevNameList[i] = NULL;
	
//	dprintf(kDevName ": Init Ok. Found %ld devices.\n", i );

	return B_OK;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;

	// Free device name list
	for (i=0; (item=gDevNameList[i]); i++)
		free(item);
	
	// Free device list
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
	dev_info_t 			*device;
	uint32 				val;
	
	// Find device name
	for (devID=0; (devName=gDevNameList[devID]); devID++) {
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;
	
	// Check if the device is busy and set in-use flag if not
	mask = 1 << devID;
	if (atomic_or(&gOpenMask, mask) &mask) {
		return B_BUSY;
	}

	// Allocate storage for the cookie
	if (!(*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));
	
	/* Setup the cookie */
	device->pciInfo = gDevList[devID];
	device->devID = devID;

	
	device->debug = DEBUG_FLAGS;
	
	DEBUG(device, FUNCTION, kDevName ": open %s device=%x\n", name, device);
	
#if DEBUGGER_COMMAND
	gdev = device;
	add_debugger_command (kDevName, homelan, "Ethernet driver Info");
#endif

	/* enable access to the cards address space */
	if ((status = enable_addressing(device)) != B_OK)
		goto err1;

//	dump_csrs(device,"Initial Values\0");

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	
	clear_statistics(device);
	
	device->media = MEDIA_DEFAULT;

	// Init Device
	reset_device(device);
	
	get_mac_address(device);

	/* allocate and initialize frame buffer rings & descriptors */
	if (init_ring_buffers(device) != B_OK)
		goto err2;
	
	media_init(device);

	// Setup interrupts
	install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, homelan_interrupt, *cookie, 0 );
	
	/* load init block & enable interrupts */
	write_csr(device, 1, device->csr1);
	write_csr(device, 2, device->csr2);

	val  = read_csr(device, 58); /* set software style to 2  (alias of BCR 20 ) */
	dprintf("media init csr58=%x setting to 0200\n", val);
	write_csr(device, 58, 0x202);
	
	write_csr(device, 4, 0x915);  /* more options */

	val  = read_csr(device, 80);
	dprintf(kDevName " open csr80=%x setting to 9C10\n", val);
	write_csr(device, 80, 0x9c10);

	write_csr(device, 0, 1); 		/* tell chip to read init_block */

	{ 	int16 j = 10;
	   
		while (j--) if (read_csr(device, 0) & 0x0100) break;
		
		if (j <= 0) {
			DEBUG(device, ERR, "Init Block failed\n");
			dump_csrs(device, "Init_Block_failure\0");
		}
		
		/* start receiver and transmitter */
	    write_csr(device, 0, 0x0472);
		
		DEBUG(device, INFO, kDevName ": open ok after %d csr0 reads\n", j);
	}
	
	// Start mii_poll deamon
	//	register_kernel_daemon( media_watch, device, 50 );
	
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

	DEBUG(device, FUNCTION, kDevName ": close");

	/* Stop the chip */
	write_csr(device, 0, 4);  /* stop chip and dma */
		
	/* Release resources we are holding */
	

#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, homelan);
#endif
	
	free_resources(device);
			
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	dev_info_t *device = (dev_info_t *) cookie;

	DEBUG(device, FUNCTION, kDevName": free %x\n",device);

	// Remove MII Poll daemon
	//	unregister_kernel_daemon( mii_poll, device );
	
	// Remove Interrupt Handler
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, homelan_interrupt, cookie );

	
	free_ring_buffers(device);
	
	// Free Areas
#if MEMORY_MAPPED_PCI_ACCESS
	delete_area(device->reg_area);
#endif

	// Device is now available again
	atomic_and(&gOpenMask, ~(1 << device->devID));

	free(device);
	return 0;
}


/*
 * Standard driver control function
 */
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len)
{
	dev_info_t *device = (dev_info_t *) cookie;

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			DEBUG(device, FUNCTION, kDevName ": control %x ether_getaddr\n",msg);
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->mac_address.ebyte[i];
			}
			return B_NO_ERROR;
		}
		case ETHER_INIT:
			DEBUG(device, FUNCTION, kDevName ": control %x init\n",msg);
			return B_NO_ERROR;
			
		case ETHER_GETFRAMESIZE:
			DEBUG(device, FUNCTION, kDevName ": control %x get_framesize\n",msg);
			*(uint32 *)buf = 1514;
			return B_NO_ERROR;
			
		case ETHER_ADDMULTI:
			DEBUG(device, FUNCTION, kDevName ": control %x add multi\n",msg);
			return (domulti(device, (unsigned char *)buf));
		
		case ETHER_SETPROMISC:
			DEBUG(device, FUNCTION, kDevName ": control %x set promiscuous\n",msg);
			return B_ERROR;
		
		case ETHER_NONBLOCK:
			DEBUG(device, FUNCTION, kDevName ": control %x blocking\n",msg);

			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_NO_ERROR;
			
		default:
			DEBUG(device, ERR, kDevName ": unknown iocontrol %x\n",msg);
			return B_ERROR;
	}

}

void rx_list_next(dev_info_t * device, uint16 *rx_now) {
	cpu_status  	former;
	status_t		status = B_OK;
	
	former = disable_interrupts();
	acquire_spinlock( &device->rx_spinlock );

	device->rx_desc[*rx_now]->misc = 0;
	device->rx_desc[*rx_now]->status = B_HOST_TO_LENDIAN_INT16(0x8000);
	device->rx_desc[*rx_now]->length =  B_HOST_TO_LENDIAN_INT16(-BUFFER_SIZE);
	*rx_now = (*rx_now+1) & RX_MASK;
	device->rx_acked = *rx_now;
		
	release_spinlock( &device->rx_spinlock );
	restore_interrupts(former);

}
/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	dev_info_t  *device = (dev_info_t *) _device;
	status_t    status;
	uint16		bad_packets = 0;
	int16		limit = RX_BUFFERS; 
	uint16      rx_now;
	uint8 		packet_status;
	bool 		done = false;
	
	
	rx_now = device->rx_acked;
#if 0
	{
	int32 semcount;
	get_sem_count(device->ilock, &semcount);
	dprintf("RD entry rx[%d] rx_rcvd=%d sem=%d 0=%x 1=%x 2=%x 3=%x ", rx_now, device->rx_received, semcount,
		device->rx_desc[0]->status,	device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
	}
#endif
	while ( (!done) && (limit-- > 0)) {

		// Block until data is available
		if((status = acquire_sem_etc( device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0 )) != B_NO_ERROR) {
			*len = 0;
			return status;
		}
		// Protect againsts reentrant read
		if( atomic_or( &device->readLock, 1 ) ) {
			release_sem_etc( device->ilock, 1, 0 );
//			DEBUG(device, ERR, kDevName ": reentrant read\n");
			*len = 0;
			return B_ERROR;
		}

		/* three cases, frame is good, bad, or we don't own the descriptor */
		packet_status = device->rx_desc[rx_now]->status >> 8;
		if (packet_status == 0x03)  {		/* we own the buffer, and it looks goood */
			uint16 frame_size = B_LENDIAN_TO_HOST_INT16(device->rx_desc[rx_now]->misc);
			if ((frame_size < 64) || (frame_size > 1518)) {    /* error packet - update stats & discard */
				DEBUG(device, ERR, kDevName ": Bad frame size %d \n", frame_size);
			} else {	/* received a good packet */
				done = true; /* one frame per read */
				frame_size = min(frame_size,1514);
				frame_size = min(frame_size, *len);
				memcpy( buf, device->rx_buf[rx_now], frame_size );
				{ unsigned short  *seq = (unsigned short *) device->rx_buf[rx_now];
				*len = frame_size;
				DEBUG(device, SEQ, " R%4.4x ", seq[20]);  /* sequence number */
				}
				 if (device->debug & RX) {
					dump_packet("rx: ",(unsigned char *)buf, frame_size);
				}
			}
			/* return ownership of the buffer back to card for good and bad frames */
			rx_list_next(device,  &rx_now);		
		
		} else if (packet_status & 0x80) {
			{
			int32 semcount;
			get_sem_count(device->ilock, &semcount);
			DEBUG(device, ERR,"RD 80 rx[%d] rx_rcvd=%d sem=%d 0=%x 1=%x 2=%x 3=%x ", rx_now, device->rx_received, semcount,
				device->rx_desc[0]->status,	device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
			}
		
			/* card owns!? indexes are out of sync */
			{ int16 resync_limit = device->rx_received;
			while ((rx_now != resync_limit) && (device->rx_desc[rx_now]->status & 0x8000))
				rx_now = (rx_now+1) & RX_MASK;
			}
			DEBUG(device, ERR," RESYNC[%d]\n",rx_now);
			device->rx_acked = rx_now;
			break;
		} else {
			{
			int32 semcount;
			get_sem_count(device->ilock, &semcount);
			DEBUG(device, ERR, "RD FrameERR rx[%d+] rx_rcvd=%d sem=%d 0=%x 1=%x 2=%x 3=%x\n", rx_now, device->rx_received, semcount,
				device->rx_desc[0]->status,	device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
			}
			/* return ownership of the buffer back to card for good and bad frames */
			rx_list_next(device,  &rx_now);		
			
			// Restarting hangs the card at the moment, so dont do it.
			// -alfred Nov 4
			//restart_device(device);
			*len = 0;	
			device->readLock = 0;
			return B_ERROR;
		}
	}
	
	/* update acked index for next read call, and release reantrant lock */
	device->readLock = 0;
	return B_OK;
}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len)
{
	dev_info_t  *device = (dev_info_t *) _device;
	int16        frame_size;
	status_t      status;
	int16         tx_now;
	
	DEBUG(device, FUNCTION, kDevName ":write buf %x len %d (decimal)\n",buf,*len);
 

	if( *len > MAX_FRAME_SIZE ) {
		DEBUG(device, ERR, kDevName ": write %d > 1514 tooo long\n",*len);
		*len = MAX_FRAME_SIZE;
	}
	frame_size = *len;
	
	if( (status = acquire_sem_etc( device->olock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR ) {
		*len = 0;
		return status;
	}

	// Protect againsts reentrant write
	if( atomic_or( &device->writeLock, 1 ) ) {
		release_sem_etc( device->olock, 1, 0 );
		*len = 0;
		return B_ERROR;
	}

	if (device->debug & TX) {
		dump_packet("tx", (unsigned char *) buf, frame_size);
	}
 	
	if (device->tx_desc[device->tx_sent]->status & 0x8000) {		/* card owns!? */
		DEBUG(device, ERR, kDevName ": can't write over card's buf[%d]\n",tx_now);
		device->writeLock = 0;
		return B_ERROR;
	}

	// Copy data to buffer
	memcpy( device->tx_buf[device->tx_sent], buf, frame_size );

    device->tx_desc[device->tx_sent]->length = B_HOST_TO_LENDIAN_INT16(-frame_size);
    device->tx_desc[device->tx_sent]->misc = 0;
    device->tx_desc[device->tx_sent]->status = B_HOST_TO_LENDIAN_INT16(0x8300);

    device->tx_sent = (device->tx_sent + 1) & TX_MASK;
	
    write_csr(device, 0, 0x0048);		/* transmit demand | interrupts on */
	
	/* Another write may now take place */
	device->writeLock = 0;
	return B_OK;
}


static void rx_interrupt(dev_info_t *device) {
	int16 j, limit, rx_now;
	uint16 rx_count = 0;
	
	rx_now = device->rx_received;

	/* scan the ring buffer for received frames */
	acquire_spinlock( &device->rx_spinlock );
	limit = ((RX_MASK - (device->rx_received - device->rx_acked)) & RX_MASK);
	for ( j=limit; j >= 0; j--) {
		if (device->rx_desc[rx_now]->status & 0x8000) break;
		rx_count++;
		rx_now = (rx_now+1) & RX_MASK;
	}

	if (rx_count) { /* signal data has arrived */
		device->rx_received = (device->rx_received + rx_count) & RX_MASK;
		release_spinlock( &device->rx_spinlock );
		device->stats_rx += rx_count;
		release_sem_etc( device->ilock, rx_count, B_DO_NOT_RESCHEDULE );
#if 0
		{
		int32 semcount;
		get_sem_count(device->ilock, &semcount);
		dprintf("ISR rx[%d] acked=%d sem=%d counted %d limit=%d 0=%x 1=%x 2=%x 3=%x\n", rx_now, device->rx_acked, semcount, rx_count,limit,device->rx_desc[0]->status,	device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
		}
#endif 
	} else { /* got a spurious interrupt, or pointers are out of sync */
		int32 semcount;
		release_spinlock( &device->rx_spinlock );
		DEBUG(device, ERR,"ISR sync rx[%d] 0=%x 1=%x 2=%x 3=%x\n", rx_now, device->rx_desc[0]->status,device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
		limit = RX_BUFFERS-1;
		while (limit--) {
			if ((device->rx_desc[rx_now]->status & 0x8000)==0) break;
			rx_now = (rx_now +1) & RX_MASK;
		}
		device->rx_received = rx_now;
		DEBUG(device, ERR, "ISR resync to %x\n", rx_now);
	}
}


static void tx_interrupt(dev_info_t *device) {
	int16 tx_now = device->tx_acked; 
	int16 limit = ((TX_MASK - (device->tx_sent - device->tx_acked)) & TX_MASK);
	int16 tx_count = 0;
	int16 j;
	
	DEBUG(device, FUNCTION, kDevName ": tx int\n");
	
	for ( j=limit; j >= 0; j--) {
		if ((device->tx_desc[tx_now]->status & 0x8000) || (device->tx_desc[tx_now]->status == 0)) break; /* card owns or free buffer */
		if (device->tx_desc[tx_now]->status & 0x4000) {
			/* check misc & log stats for type of error */
			;
		}
		device->tx_desc[tx_now]->status = 0; /* mark the buffer so we don't count again */
		tx_count++;
		tx_now = (tx_now + 1) & TX_MASK;
	}

	if (tx_count) { /* signal data has been sent */
		device->tx_acked = (device->tx_acked + tx_count) & TX_MASK;
		device->stats_tx += tx_count;
		release_sem_etc( device->olock, tx_count, B_DO_NOT_RESCHEDULE );
	}

}


static void restart_device(dev_info_t *device) {
	int i;
	
	dprintf("restart_device: Still Some Work to Do Here\n");

	write_csr(device, 0, 4);  /* stop the chip */ 
	
	reset_device(device);

	for ( i = 0; i < TX_BUFFERS; i++) {
		device->tx_desc[i]->status = 0;
		device->tx_desc[i]->length = 0;
	}

	for ( i = 0; i < RX_BUFFERS; i++) {
		device->rx_desc[i]->length = B_HOST_TO_LENDIAN_INT16(-BUFFER_SIZE);
		device->rx_desc[i]->status = B_HOST_TO_LENDIAN_INT16(0x8000);
	}

	/* initialize indexes */
	device->tx_sent = device->tx_acked = device->rx_received = device->rx_acked = 0;
	
	media_init(device);
		
	device->init_block->mode = 0x03;	

	/* load init block & enable interrupts */
	write_csr(device, 1, device->csr1);
	write_csr(device, 2, device->csr2);
	
	write_csr(device, 4, 0x915);  /* undocumented value */
	write_csr(device, 0, 1); 		/* tell chip to read init_block */

	{ 	int16 j = 10;
	   
		while (j--) if (read_csr(device, 0) & 0x0100) break;
		
		if (j <= 0) {
			DEBUG(device, ERR, "Init Block failed\n");
			dump_csrs(device, "Init_Block_failure\0");
		}
		
		/* start receiver and transmitter */
	    write_csr(device, 0, 0x0472);
		
		DEBUG(device, INFO, kDevName ": err reset complete %d csr0 reads\n", j);
	}
}


static void err_interrupt(dev_info_t *device, uint16 isr) {
	
	int16 i;
	
	DEBUG(device, ERR, kDevName " err interrupt\n");
		
	if (isr &  0x4000) {
		DEBUG(device, ERR, kDevName ": undefined isr err %x\n", isr);
		device->bit14_err++;
//		restart_device(device);
		return;
	}
	if (isr & 0x2000) {
		device->collisions++;
	}
	if (isr & 0x1000) {
		device->missed_rx++;
	}
	if (isr & 0x800) {
		device->mem_err++;
		DEBUG(device, ERR, kDevName " MEM ERR\n");
//		restart_device(device);
	}
}

/* service interrupts generated by the Lan controller (card) hardware */
static int32
homelan_interrupt(void *_device)
{
	dev_info_t *device = (dev_info_t *) _device;
	uint16 isr, rap;
	int32 handled = B_UNHANDLED_INTERRUPT;
	int16 worklimit = 40;
			
	DEBUG(device, INTERRUPT, "ISR_ENTRY %8.8x\n", read_csr(device,0));	
	
	while (((isr = read_csr(device, 0) & 0xFF80)) && worklimit-- > 0) {
		
		handled = true;
		device->handled_int++;
		
		DEBUG(device, INTERRUPT, "ISR_LOOP %8.8x\n", isr);
	
		if (isr & 0x8000) {
			DEBUG(device, WARN, kDevName ": isr err %x\n", isr);
			err_interrupt(device, isr);
		}

		if (isr & 0x0400) {
			DEBUG(device, INTERRUPT, kDevName ": isr rx %x\n",isr);	
			rx_interrupt(device);
		}
		if (isr & 0x0200) {
			DEBUG(device, INTERRUPT, kDevName ": isr tx %x\n",isr);	
			tx_interrupt(device);
		}
		
		write_csr(device, 0, (isr & 0x7F00) | 0x0040); /* ack (clear) and re-enable interrupts */

	}
		
	if (handled == B_UNHANDLED_INTERRUPT) {
		device->unhandled_int++;
	}
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
		if ((item->vendor_id == VENDOR_ID_AMD)&& (item->device_id == DEVICE_ID_AM79C978)) {
			/* check if the device really has an IRQ */
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf(kDevName " found with invalid IRQ - check IRQ assignement");
				continue;
			}
			dprintf(kDevName " found at IRQ %x ", item->u.h0.interrupt_line);
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
		}
	}
	info[entries] = NULL;
	free(item);
	dprintf(kDevName " get_pci_list %d entries\n", entries);
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

#if IO_PORT_PCI_ACCESS
	device->reg_base = device->pciInfo->u.h0.base_registers[0];
#endif /* IO_PORT_PCI_ACCESS */

#if MEMORY_MAPPED_PCI_ACCESS
	{
	int32		base, size, offset, i;
	int32		*ptr;
	const		MemoryReg = 1;
	
	base = device->pciInfo->u.h0.base_registers[MemoryReg];
	size = device->pciInfo->u.h0.base_register_sizes[MemoryReg];
	
	/* use "poke" with the "pci" command from a terminal session to check these values */
	DEBUG(device, PCI_IO,  kDevName ": PCI base=%x size=%x\n", base, size);

	// Round down to nearest page boundary
	base = base & ~(B_PAGE_SIZE-1);
	
	// Adjust the size
	offset = device->pciInfo->u.h0.base_registers[MemoryReg] - base;
	size += offset;
	size = (size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

	DEBUG(device, PCI_IO,  kDevName ": Now PCI base=%x size=%x offset=%x\n", base, size, offset);
		
	if ((device->reg_area = map_physical_memory(kDevName " Regs", (void *)base, size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &device->reg_base)) < 0)
		return B_ERROR;

	device->reg_base = device->reg_base + offset;  //  /sizeof (int32) ??
	}
#endif /*MEMORY_MAPPED_PCI_ACCESS */
	
	/* sanity check - can we access expected values ? */
	{
		uint16 PartID = 0x262;
	  	if (( 0xfff & read_csr(device, 89)) != 0x262) {
			DEBUG(device, ERR, kDevName ": Unable to access hardware\n");
			return B_ERROR;
		}
		device->chip_revision = (read_csr(device, 88) & 0xF00) >> 8;
	}
	DEBUG(device, PCI_IO,  kDevName ": reg_base=%x\n", device->reg_base);

	return B_OK;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t init_ring_buffers(dev_info_t *device)
{
	uint32 	size;
	physical_entry		entry;
	uint16 i;
		
	/* create transmit buffer area */
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_buf_area = create_area( kDevName " tx buffers", (void **) device->tx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		DEBUG(device, ERR, kDevName " create tx buffer area failed %x \n", device->tx_buf_area);
		return device->tx_buf_area;
	}
	/* create tx descriptor area */
	size = RNDUP( sizeof(frame_desc_t) * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_desc_area = create_area( kDevName " tx descriptors", (void **) device->tx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		DEBUG(device, ERR, kDevName " create tx descriptor area failed %x \n", device->tx_desc_area);
		delete_area(device->tx_buf_area);
		return device->tx_desc_area;
	}


	/* init descriptors with physical addresses */
	for ( i = 0; i < TX_BUFFERS; i++) {
		device->tx_buf[i] = device->tx_buf[0] + (i * BUFFER_SIZE);
		device->tx_desc[i] = (frame_desc_t *) (((uint32)device->tx_desc[0]) + (i * sizeof(frame_desc_t)));
		get_memory_map( (uint8 *)device->tx_buf[i], 4, &entry, 1 );
		device->tx_desc[i]->status = 0;
		device->tx_desc[i]->base = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		device->tx_desc[i]->length = 0;
	}
	if (device->debug & TX) {
		for ( i = 0; i < TX_BUFFERS; i++) {
			dprintf("tx_buf[%3.3d]=%8.8x base=%8.8x len=%8.8x status=%8.8x misc=%8.8x\n", i,device->tx_buf[i],
				device->tx_desc[i]->base, device->tx_desc[i]->length, device->tx_desc[i]->status, device->tx_desc[i]->misc);
		}
	}	



	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		DEBUG(device, ERR, kDevName " create rx buffer area failed %x \n", device->rx_buf_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		return device->rx_buf_area;
	}
	/* create rx descriptor area */
	size = RNDUP( sizeof(frame_desc_t) * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_desc_area = create_area( kDevName " rx descriptors", (void **) device->rx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		DEBUG(device, ERR, kDevName " create rx descriptor area failed %x \n", device->rx_desc_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		return device->rx_desc_area;
	}

	/* init rx buffer descriptors */
	for ( i = 0; i < RX_BUFFERS; i++) {
		device->rx_buf[i] = device->rx_buf[0] + (i * BUFFER_SIZE);
		device->rx_desc[i] = (frame_desc_t *) (((uint32)device->rx_desc[0]) + (i * sizeof(frame_desc_t)));
		get_memory_map( (uint8 *)device->rx_buf[i], 4, &entry, 1 );
		device->rx_desc[i]->base = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		device->rx_desc[i]->length = B_HOST_TO_LENDIAN_INT16(-BUFFER_SIZE);
		device->rx_desc[i]->status = B_HOST_TO_LENDIAN_INT16(0x8000);
	}
	if (device->debug & RX) {
		for ( i = 0; i < RX_BUFFERS; i++) {
			dprintf("rx_buf[%3.3d]=%8.8x base=%8.8x length=%8.8x status=%8.8x misc=%8.8x\n", i,device->rx_buf[i],
				device->rx_desc[i]->base, device->rx_desc[i]->length, device->rx_desc[i]->status, device->rx_desc[i]->misc);
		}
	}	

	/* set up init block at end of descriptors */
	device->init_block = (init_block_t *) ((uint32)device->rx_desc[RX_BUFFERS-1] + (sizeof(frame_desc_t)));
	RNDUP((uint32)device->init_block, 4);	/* allign to long */

	/* initialize indexes */
	device->tx_sent = device->tx_acked = device->rx_received = device->rx_acked = 0;

	/* initilize init block */
	device->init_block->mode = B_HOST_TO_LENDIAN_INT16(0x3); 	/* turn off xmitter & rcver */
	device->init_block->tx_rx_len = B_HOST_TO_LENDIAN_INT16((TX_BUFFERS_LOG2 << 12) | (RX_BUFFERS_LOG2 << 4));
    for (i = 0; i < 6; i++) {
		device->init_block->mac_addr[i] = device->mac_address.ebyte[i];
	}    
	get_memory_map( (uint8 *)device->rx_desc[0], 4, &entry, 1 );
	device->init_block->rx_ring = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
 	get_memory_map( (uint8 *)device->tx_desc[0], 4, &entry, 1 );
	device->init_block->tx_ring = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
	/* to do: add multicast address filtering based on hash -
	   for now, set to accept all mutlicast addresses and let the hardware filter it */
//	device->init_block->filter[0] = device->init_block->filter[1] = 0;
	device->init_block->filter[0] = device->init_block->filter[1] = 0xFFFFFFFF;
	
#if 0
	dprintf("init_block mode=%8.8x tx_rx_len=%8.8x mac=%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x \n \
		filter[0]=%8.8x filter[1]=%8.8x rx_ring=%8.8x tx_ring=%8.8x\n",
		device-> init_block->mode, device-> init_block->tx_rx_len,
		device-> init_block->mac_addr[0], device-> init_block->mac_addr[1], device-> init_block->mac_addr[2],
		device-> init_block->mac_addr[3], device-> init_block->mac_addr[4], device-> init_block->mac_addr[5],
		device-> init_block->filter[0],device-> init_block->filter[1],
		device-> init_block->rx_ring,device-> init_block->tx_ring);
#endif

	/* set up hardware init block */
 	get_memory_map( (uint8 *)device->init_block, 4, &entry, 1 );
	device->csr1=B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address )) & 0xffff;
	device->csr2=B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address )) >> 16;
	write_csr(device, 1,  device->csr1);
	write_csr(device, 2,  device->csr2);

	return B_OK;
}

static void free_ring_buffers(dev_info_t *device) {

		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		delete_area(device->rx_desc_area);

}

/* initialize the media, speed and duplex settings */
static void media_init(dev_info_t *device) {
	
	uint16 duplex, link_partner, val;  

	val = read_bcr(device, 49);		/* select and lock media 10BT */
	DEBUG(device, INFO, kDevName "media_init BCR49 = %x setting to 0x8180\n", val);
	write_bcr(device, 49, 0x8180);	
	
	/* advertise 10BT FD, turn on autonegotiate, and check link partner report */
	write_bcr(device, 33, 0x3c4);
	write_bcr(device, 34, 0x61);

	val = read_bcr(device, 32);
	DEBUG(device, INFO, "media_init start auto neg BCR32 = %x setting to 0x4820\n", val);
	write_bcr(device, 32, 0x4820);

	write_bcr(device, 33, 0x3c5);
	link_partner = read_bcr(device, 34);
	
	duplex = 0;
	if (link_partner & 0x40) { /* link partner advertised full duplex 10BT */
		duplex = 1; 		   /* see IEEE 802.3 Supplement section 28.2.4.1.4 and Annex 28b, and don't */
							   /*   forget to reverse the bit order! */
	}

	write_bcr(device, 32, 0xc0);  /* reset phy - required after a hard reset */
	write_bcr(device, 32, 0);
	snooze(5000); 			/* allow time for the phy interface to reset */

	if (duplex) {
		DEBUG(device, INFO, kDevName " Full Duplex link partner shows %x\n", link_partner);
		write_bcr(device, 9, duplex);
	} else {
		DEBUG(device, INFO, kDevName " Half Duplex link partner shows %x\n", link_partner);
		write_bcr(device, 9, duplex);
	}


	/* check for 10BT link - if not found, go to phone lan */
	{ 	uint16 link_check = read_bcr(device, 33);
		if ((link_check & 0x1000) == 0) { /* no 10BaseT link, try phonelan */
			DEBUG(device, INFO, kDevName " Phone Line Wiring Media Selected\n");
	
			reset_device(device);
			
			write_bcr(device, 33, 0x3b1);		/* address 0x1D register 0x11 - phonelan */
			val = read_bcr(device, 34);
			DEBUG(device, INFO, kDevName " media_init phy at 0x1D reg 0x11 = %x setting to 0x1300\n", val);
			write_bcr(device, 33, 0x3b1);
			write_bcr(device, 34, 0x1300);
	
			val = read_bcr(device, 49);		/* select and lock media */
			DEBUG(device, INFO, kDevName " media_init BCR49 = %x setting to 0x8181\n", val);
			write_bcr(device, 49, 0x8181);	
	
			val = read_bcr(device, 2);
			dprintf("BCR2 was %x\n",val);
			write_bcr(device, 2, val | 2); /* turn on phy management interrutps - see csr7 and bcr32 */
			
			val = read_csr(device, 124);	/* set GPSI bit  - marked as for internal test use only */
			dprintf(kDevName " CSR(124) test reg was %x no is %x\n", val, val | 0x10);
			write_csr(device, 124, val | 0x10);
		}
	}



	write_csr(device, 0, 0);
	write_csr(device, 0, 4);		/* stop  NIC */
	
	device->init_block->mode = device->media;
}




static void get_mac_address(dev_info_t *device) {

	int j;
	DEBUG(device, INFO, kDevName ": Mac address: ");
	for (j=0; j<6; j++) {
		device->mac_address.ebyte[j] = read8((uint32)device->reg_base +j);
		DEBUG(device, INFO," %x", device->mac_address.ebyte[j]);
	}
	DEBUG(device, INFO," chip revision %x\n", device->chip_revision);
}


static void reset_device(dev_info_t *device) {
	uint32 				*tx_desc_phys, *rx_desc_phys;
	physical_entry		entry;

	DEBUG(device, FUNCTION, kDevName ": reset device\n");

	read16(device->reg_base + RESET);		/* reset the card */
	write_bcr(device, 20, 2);		/* set software "style" - 32bit, pci, burts modes */

}


/* set hardware so that all packets are received. */
status_t setpromisc(dev_info_t * device) {
	
	DEBUG(device, FUNCTION, kDevName ":setpormisc\n");
	return(B_ERROR);
	//return(B_NO_ERROR);
}

static status_t domulti(dev_info_t *device, uint8 *addr)
{
	
	/* Right now the InitBlock filters[] are set to 0xffffffff so we
	   accept all multicast frames and let the upper layer software filter it.
	   ToDo: add code to hash the address, set the filter[] values, and
	   stop and restart the chip with the new init block.
	*/
	DEBUG(device, FUNCTION,kDevName ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
					addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);

	return (B_NO_ERROR);
}



void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d \n", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
}




void dump_rx_desc(dev_info_t *device) {
	uint16 j;
	

	kprintf("rx desc %8.8x \n", device->rx_desc);
	
	for (j=0; j < RX_BUFFERS; j++ ) {
		kprintf("rx_desc[%2.2d].base=%8.8x len=%4.4d status=%4.4x misc=%8.8x\n",
			j, device->rx_desc[j]->base, device->rx_desc[j]->length,
			device->rx_desc[j]->status, device->rx_desc[j]->misc);
	
	}

}
void dump_tx_desc(dev_info_t *device) {
	uint16 j;
	

	kprintf("rx desc %8.8x \n", device->rx_desc);
	
	for (j=0; j < TX_BUFFERS; j++ ) {
		kprintf("tx_desc[%2.2d].base=%8.8x len=%4.4d status=%4.4x misc=%8.8x buf=%8.8x\n",
			j, device->tx_desc[j]->base, device->tx_desc[j]->length,
			device->tx_desc[j]->status, device->tx_desc[j]->misc, &device->tx_buf[j]);
	
	}

}



/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   hit the alt-sysreq (on Intel) to enter the debugger <cr>"
*/
#if DEBUGGER_COMMAND
static int
homelan(int argc, char **argv)
{
	uint16 i,j;
	const char * usage = "usage: homelan [ bcr_set dec_reg hex_val, csr_set dec_reg hex_val, Descriptor_dump, Function_calls, \
Hardware_reg_dump, \n Interrupts, Numbered_packet_trace, PCI_IO, Stats, Rx_trace, Tx_trace, Write hex_addr hex_val]\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'b':
		case 'B':
				if (i < 4) { kprintf("usage: bcr dec_reg hex_value\n"); return 0; }
				write_bcr(gdev, (int) strtol (argv[j+1], (char **) NULL, 10),
										  (int) strtol (argv[j+2], (char **) NULL, 16));
				kprintf("Wrote bcr[%d] <- %x\n",  (int) strtol (argv[j+1], (char **) NULL, 10),
					(int) strtol (argv[j+2], (char **) NULL, 16));
				j +=2; i-=2;
				break;
		case 'c':
		case 'C':
				if (i < 4) { kprintf("usage: write_csr addr_dec value_hex\n"); return 0; }
				write_csr(gdev, (int) strtol (argv[j+1], (char **) NULL, 10),
										  (int) strtol (argv[j+2], (char **) NULL, 16));
				kprintf("Wrote csr[%d] <- %x\n",  (int) strtol (argv[j+1], (char **) NULL, 10),
					(int) strtol (argv[j+2], (char **) NULL, 16));
				j +=2; i-=2;
				break;
		case 'd':
		case 'D':
			kprintf("rx_receviced = %x rx_acked = %x \n", gdev->rx_received, gdev->rx_acked);
			dump_rx_desc(gdev);

			kprintf("tx_sent = %x tx_acked = %x \n", gdev->tx_sent, gdev->tx_acked);
			dump_tx_desc(gdev);
			break; 
		case 'F':
		case 'f':
			gdev->debug ^= FUNCTION;
			if (gdev->debug & FUNCTION) 
				kprintf("Function() call trace Enabled\n");
			else 			
				kprintf("Function() call trace Disabled\n");
			break; 
		case 'H':
		case 'h':
			dump_bcrs(gdev, "BCR: ");
			dump_csrs(gdev, "CSR: ");
			break; 
		case 'I':
		case 'i':
			gdev->debug ^= INTERRUPT;
			if (gdev->debug & INTERRUPT) 
				kprintf("Interrupt trace Enabled\n");
			else 			
				kprintf("Interrupt trace Disabled\n");
			break; 
		case 'N':
		case 'n':
			gdev->debug ^= SEQ;
			if (gdev->debug & SEQ) 
				kprintf("Sequence numbers packet trace Enabled\n");
			else 			
				kprintf("Sequence numbers packet trace Disabled\n");
			break; 
		case 'S':
		case 's':
			kprintf("homelan statistics\n");			
			kprintf("received %d,  transmitted %d frames\n", gdev->stats_rx, gdev->stats_tx);
			kprintf("collisions = %d missed receives = %d memory errors = %d bit14_err = %d\n",
				gdev->collisions, gdev->missed_rx, gdev->mem_err, gdev->bit14_err);
			kprintf("handled %d and unhandled %d interrupts\n", gdev->handled_int, gdev->unhandled_int);
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
		case 'P':
		case 'p':
			gdev->debug ^= PCI_IO;
			if (gdev->debug & PCI_IO) 
				kprintf("PCI IO trace Enabled\n");
			else 			
				kprintf("PCI IO trace Disabled\n");
			break; 
		case 'W':
		case 'w':
				if (i < 4) { kprintf("usage: write  hex_addr dec_value\n"); return 0; }
				kprintf("Wrote %x <- %x\n",  (int) strtol (argv[j+1], (char **) NULL, 10),
					(int) strtol (argv[j+2], (char **) NULL, 16));
				// code to write here
				j +=2; i-=2;
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
	device->collisions = device->missed_rx = device->mem_err = device->bit14_err = 0;
	
}

/*
 * Allocate and initialize semaphores and spinlocks.
 */
static status_t allocate_resources(dev_info_t *device) {
		
	/* Setup Semaphores */
	if ((device->ilock = create_sem(0, kDevName " rx")) < 0) {
		dprintf(kDevName " create rx sem failed %x \n", device->ilock);
		return (device->ilock);
	}
	set_sem_owner(device->ilock, B_SYSTEM_TEAM);
	
	/* intialize tx semaphore with the number of free tx buffers */
	if ((device->olock = create_sem(TX_BUFFERS, kDevName " tx")) < 0) {
		delete_sem(device->ilock);
		dprintf(kDevName " create tx sem failed %x \n", device->olock);
		return (device->olock);
	}
	set_sem_owner(device->olock, B_SYSTEM_TEAM);

	device->readLock = device->writeLock = 0;

	device->rx_spinlock = 0;
	device->bus_spinlock = 0;

	device->blockFlag = 0;
		
	return (B_OK);
}

static void free_resources(dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->olock);
}



int32	api_version = B_CUR_DRIVER_API_VERSION;
