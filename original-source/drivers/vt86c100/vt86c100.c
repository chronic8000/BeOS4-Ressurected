/*
 * VIA VT86c100 PCI 10/100 Ethernet driver
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

/* PCI vendor and device ID's for VIA vt86c100 */
#define VENDOR_ID_VIA       0x1106
#define DEVICE_ID_VT86C100  0x3043

/* globals for driver instances */ 

#define kDevName "vt86c100"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4			/* maximum number of driver instances */


/*
 *  Choose "memory mapped" or "io port" pci bus access.
 */
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


/* for serial debug command */
#define DEBUGGER_COMMAND true
static int vt86c100(int argc, char **argv);
void dump_packet( const char * msg, unsigned char * buf, uint16 size);


/* debug flags */
enum debug_flags {
    ERR       = 0x0001,
    INFO      = 0x0002,
    RX        = 0x0004,    /* dump received frames */
    TX        = 0x0008,    /* dump transmitted frames */
    INTERRUPT = 0x0010,    /* interrupt calls */
    FUNCTION  = 0x0020,    /* function calls */
    PCI_IO    = 0x0040,    /* pci reads and writes */
    SEQ		  = 0x0080,    /* trasnmit & receive TCP/IP sequence sequence numbers */
    WARN	  = 0x0100,    /* Warnings - off on final release */
};
/* diagnostic debug flags - compile in here or set while running with debugger "vt86c100" command */
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO )
#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN )

void ETHER_DEBUG(int32 debug_mask, int32 enabled, char * format, ...) {
	if (debug_mask & enabled) {
		va_list		args;
		char		s[4096];
		va_start(args, format);
		vsprintf( s, format, args );
		va_end(args);
		dprintf(kDevName ": %s",s);
	}
}

/* ring buffer sizes - tune for optimal performance */
#define TINY_RING_BUFFERS_FOR_TESTING false
#if TINY_RING_BUFFERS_FOR_TESTING
#define TX_BUFFERS					4L 		/* must be a power of 2 */
#define RX_BUFFERS					4L		/* must be a power of 2 */
#else
#define TX_BUFFERS					16L 	/* must be a power of 2 */
#define RX_BUFFERS					32L		/* must be a power of 2 */
#endif
#define BUFFER_SIZE					2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE		1514			/* 1514 + 4 bytes checksum added by card hardware */
#define MIN_FRAME_SIZE		60

#define TX_MASK 					(TX_BUFFERS - 1)
#define RX_MASK						(RX_BUFFERS - 1)
static pci_module_info		*gPCIModInfo;
static char 				*gDevNameList[MAX_CARDS+1];
static pci_info 			*gDevList[MAX_CARDS+1];
static int32 				gOpenMask = 0;


const char** publish_devices(void )
{
	dprintf(kDevName ": publish_devices()\n" );
	return (const char **)gDevNameList;
}

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
static int32 interrupt_hook(void *_device);


/* Hardware specific definitions for the VIA vt86c100 Lan Controller */
#define MAX_MULTI 		16 /* maximum number of multicast addresses */

#define MII_DELAY 		4000

enum memory_space_registers {
	PAR0=0, PAR1=1, PAR2=2, PAR3=3, PAR4=4, PAR5=5, RCR=6, TCR=7,
	CR0=8, CR1=9, ISR0=0x0C, ISR1=0xD, IMR0=0xE, IMR1=0xF,
	MAR0=0x10, MAR1=0x11, MAR2=0x12, MAR3=0x13, MAR4=0x14, MAR5=0x15, MAR6=0x16, MAR7=0x17,
	RDA0=0x18, RDA1=0x19, RDA2=0x1A, RDA3=0x1B, TDA0=0x1C, TDA1=0x1D, TDA2=0x1E, TDA3=0x1F,
	MPHY=0x6C, MIISR=0x6D, BCR0=0x6E, BCR1=0x6F, MIICR=0x70, MIIAD=0x71, MIIDATA=0x72, EECSR=0x74, TEST=0x75,
	CFGA=0x78, CFGB=0x79, CFGC=0x7A, CFGD=0x7B, MPAC0=0x7C, MPAC1=0x7D, CRCC0=0x7E, CRCC1=0x7F
};

enum chip_commands {
	VR_CMD_INIT=0x0001,     VR_CMD_START=0x0002,      VR_CMD_STOP=0x0004,
	VR_CMD_RX_ON=0x0008,    VR_CMD_TX_ON=0x0010,      VR_CMD_TX_GO=0x0020,
	VR_CMD_RX_GO=0x0040,    VR_CMD_RSVD=0x0080,       VR_CMD_RX_EARLY=0x0100,
	VR_CMD_TX_EARLY=0x0200, VR_CMD_FULLDUPLEX=0x0400, VR_CMD_TX_NOPOLL=0x0800,
    VR_CMD_RESET=0x8000
};

enum isr_bits {
	PRX=0x0001,		PTX=0x0002,		RXE=0x0004,		TXE=0x0008,
	TU=0x0010,		RU=0x0020,		BE=0x0040,		CNT=0x0080,
	ERI=0x0100,		ETI=0x0200,		OVFI=0x0400,	PKRACE=0x0800,
	NORBF=0x1000,	ABTI=0x2000,	SRCI=0x4000,	KEYI=0x8000,
};

enum rcr_bits {													   // rx control regsiter
	SEP=0x01,		AR=0x02,		AM=0x04,		AB=0x08,	   // rx_err, rx_runts, rx_multi, rx_broadcast
	PRO=0x10,		RFT32=0x20,		RFT128=0x40,	RFT256=0x60,   // rx_promisc, rx_fifo_thresholds...
	RFT512=0x80,	RFT1024=0xA0,	RRSF=0xE0,					   // rx_store_and_forward
};


//#define IMR0_MASK		0x7FFF   /* everthing except KEYI = magic packet  */
#define IMR0_MASK		0x5AFF 


enum gfcd {
	VT_MAGIC = 4,
};
	

/* Recevie & Transmit Descriptors for VIA vt86c100 */
enum rdes0 {
	rx_owns       = 0x80000000,		/* card owns true, host owns false */
	rx_ext_len    = 0x78000000,		/* extended len >1518 byte frames - for future use or proprietary protocols */
	rx_frame_len  =	0x07FF0000,     /* received frame length */
	rx_ok         = 0x00008000,
	rx_multicast  = 0x00002000,
	rx_broadcast  = 0x00001000,
	rx_macaddress = 0x00000800,		/* perfect match on this devices ethernet address */
	rx_sop		  = 0x00000200,		/* start of packet */
	rx_eop        = 0x00000100,		/* end of packet */
	rx_buf_err    = 0x00000080,
	rx_sbe        = 0x00000040, 	/* system bus error */
	rx_runt       = 0x00000020,     /* undersized frame */
	rx_oversize   = 0x00000010, 	/* oversized frame */
	rx_fov        = 0x00000008,		/* fifo overflow */
	rx_fae        = 0x00000004, 	/* frame allignement error */
	rx_crc        = 0x00000002,		/* crc error */
	rx_err        = 0x00000001,     /* crc | fae | fov | sbe */

};

enum rdes1 {
	rx_ic         = 0x00080000,		/* rx interrupt control - rx generates interrupt when 1 */
	rx_chain      = 0x00008000,		/* 1 means chained buffer in next descriptor */
	rx_len		  = 0x000007FF, 	/* received len for this descriptor, total will be in last */
};

enum tdes0 {
	tx_owns       = 0x80000000,
	tx_ok		  = 0x00008000,
	tx_jabber     = 0x00004000,
	tx_serr       = 0x00002000,    /* system error */
	tx_crs        = 0x00000400,    /* carrier sense lost during transmit */
	tx_owc        = 0x00000200,    /* late collisions */
	tx_abt        = 0x00000100,    /* tx aborted, excessive collisions */
	tx_cdh        = 0x00000080,    /* heartbeat - 10BaseT */
	tx_ncr        = 0x00000078,    /* number of collision retries counter */
	tx_udf        = 0x00000002,    /* fifo underflow - trying to transmit from an empty queue */
	tx_dft        = 0x00000001,    /* transmit deferred because carrier asserted */
};

enum tdes1 {
	tx_ic         = 0x00800000,		/* tx interrupt control - tx generates interrupt when 1 */
	tx_eop        = 0x00400000,		/* end of packet */
	tx_sop		  = 0x00200000,		/* start of packet */
	tx_crc		  = 0x00010000,     /* 1 means disable crc generation */
	tx_chain      = 0x00008000,		/* 1 means chained buffer in next descriptor */
	tx_len		  = 0x000007FF, 	/* fragment or frame buffer size */
};

typedef volatile struct frame_descriptor {
	uint32 des0;
	uint32 des1;
	uint32 buf;
	uint32 next;
	uint32 align_pad0;	/* pad to align on ~1f boundaries */
	uint32 align_pad1;
	uint32 align_pad2;
	uint32 align_pad3;
} frame_desc_t;


/* per driver intance globals */
typedef struct {
	int32			devID; 										/* device identifier: 0-n */
	pci_info		*pciInfo;
	uint16			irq;										/* our IRQ line */
	sem_id			ilock, olock;								/* i/o semaphores */
	int32			readLock, writeLock;						/* reentrant reads/writes not allowed */
	area_id			tx_desc_area, tx_buf_area; 					/* transmit descriptor and buffer areas */
	area_id			rx_desc_area, rx_buf_area; 					/* receive descriptor and buffer areas */
	uchar			*tx_buf[TX_BUFFERS], *rx_buf[RX_BUFFERS];	/* tx and rx buffers */
	frame_desc_t	*tx_desc[TX_BUFFERS], *rx_desc[RX_BUFFERS];	/* tx and rx frame descriptors */
	int16			tx_wh, tx_isr;								/* tx ring buf indexes - write hook & isr */
	int16			rx_rh, rx_isr;								/* rx ring buf indexes - read_hook and interrupt service routine*/
	int16			rx_free, tx_sent;							/* number of free given to the card for processing */
	spinlock		rx_lock, tx_lock;							/* spinlock ring buffer index updates */
	uint32			base; 										/* Base address of hostRegs */
	uint32			debug;										/* debuging level */
	int32			blockFlag;									/* sync or async io calls */
	ether_address_t mac_address;								/* my ethernet address */
	ether_address_t multi[MAX_MULTI];							/* multicast addresses */
	uint16			nmulti;										/* number of multicast addresses */
	uint16  		duplex;

	uint32 version;
	/* statistics */
} dev_info_t;

/* for serial debug command */
dev_info_t * gdev;

/* End Hardware Specific code */


/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *len);

int32 get_pci_list(pci_info *info[], int32 maxEntries); 	/* Get pci_info for each device */
status_t free_pci_list(pci_info *info[]);					/* Free storage used by pci_info list */
status_t enable_addressing(dev_info_t *device); 			/* enable pci io address space for device */
status_t init_ring_buffers(dev_info_t *device);				/* allocate and initialize frame buffer rings */
void free_ring_buffers(dev_info_t *device);					/* allocate and initialize frame buffer rings */
status_t allocate_resources(dev_info_t *device);			/* allocate semaphores & spinlocks */
void  free_resources(dev_info_t *device);					/* deallocate semaphores & spinlocks */
void get_mac_address(dev_info_t *device);					/* get ethernet address */
void reset_device(dev_info_t *device);						/* reset the lan controller (NIC) hardware */
void CheckDuplex(dev_info_t *device);
void dump_rx_descriptors(dev_info_t * device);
void dump_tx_descriptors(dev_info_t * device);
static status_t domulti( dev_info_t *device, char *addr );
void setpromisc(dev_info_t * device, uint32 on);

uint16 MII_Read(uint16 MII_register, dev_info_t * device);

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



/*
 * Driver Entry Points
 */
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
	
//	dprintf(kDevName ": init_driver\n");

	if((status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		dprintf(kDevName " Get module failed! %s\n", strerror(status ));
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

//	dprintf(kDevName ": uninit_driver\n");

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

//	dprintf(kDevName ": find_device() %s\n", name);

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
	dev_info_t 		*device;
			
	for (devID=0; (devName=gDevNameList[devID]); devID++) {  /* Find device name */
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;
	
	mask = 1 << devID;   /* Check if the device is busy and set in-use flag if not */
	if (atomic_or(&gOpenMask, mask) &mask) {
		return B_BUSY;
	}

	/* Allocate storage for the per driver instance global data */
	if (!(*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));
	
	device->pciInfo = gDevList[devID];
	device->devID = devID;
	
	device->debug = DEFAULT_DEBUG_FLAGS;
	
	ETHER_DEBUG(FUNCTION, device->debug, "open %s device=%x\n", name, device);
	
#if DEBUGGER_COMMAND
	gdev = device;  /* global for debug comman - works for one driver instance only */
	add_debugger_command (kDevName, vt86c100, "Ethernet driver Info");
#endif

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	/* enable access to the cards address space */
	if ((status = enable_addressing(device)) != B_OK)
		goto err1;

	/* Load Mac Address from EEPROM */
	get_mac_address(device);

	/* allocate and initialize frame buffer rings & descriptors */
	if (init_ring_buffers(device) != B_OK)
		goto err2;
	
	// Init Device
	
	reset_device(device);
	
	// Setup interrupts
	install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, *cookie, 0 );
	
	// Start mii_poll deamon
	//	register_kernel_daemon( media_watch, device, 50 );
		   	
	write8(device->base + BCR0, 0x06);  /* set TCR RCR threshold */
	write8(device->base + BCR1, 0x00);
	write8(device->base + RCR, 0x2c); 
	write8(device->base + TCR, 0x60);

 	CheckDuplex(device);
	if (device->duplex) {
		write8(device->base+CFGD, VT_MAGIC);
		write16(device->base + CR0, VR_CMD_FULLDUPLEX);

	} else {
		write16(device->base + CR0,(read16(device->base + CR0) & ~VR_CMD_FULLDUPLEX));
	}

	{
		uint16 cr0 = read16(device->base + CR0);
		cr0 &= 0xFFFB;
		write16(device->base + CR0,     VR_CMD_TX_NOPOLL | VR_CMD_START | VR_CMD_TX_ON | VR_CMD_RX_ON | cr0);
	}  


	{int j;
		for (j=0; j<6; j++) {
			dprintf("MII reg %x = %x\n", j, MII_Read(j,device));
		}
	}

	write16(device->base + IMR0, IMR0_MASK);	/* turn on interrupts */

	dprintf(kDevName " open ok CR0=%x\n", read16(device->base + CR0));	
	
	return B_OK;
	
	err2:
		free_ring_buffers(device);
		
	err1:
		free_resources(device);
		free(device);	
	
	err0:
		atomic_and(&gOpenMask, ~mask);
		dprintf(kDevName ": open failed!\n");
		return status;

}

static status_t close_hook(void *_device) {
	dev_info_t *device = (dev_info_t *) _device;


	/* Stop the chip */

	write16(device->base + IMR0, 0x0000);			/* turn off interrupts */
	write16(device->base + CR0, VR_CMD_STOP);		/* turn off rx tx */


#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, vt86c100);
#endif
	
	free_resources(device);
	
	/* Reset all the statistics*/
//	ETHER_DEBUG(INFO, device->debug, "Zero stats\n");
	
	/* Reset to notify others that this port and irq is available again*/
	
	return (B_NO_ERROR);
}


static status_t free_hook(void *_device) {
	dev_info_t *device = (dev_info_t *) _device;

	ETHER_DEBUG(FUNCTION, device->debug, "free %x\n",device);

	// Remove MII Poll daemon
	//	unregister_kernel_daemon( mii_poll, device );
	
	// Remove Interrupt Handler
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, device );

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

	ETHER_DEBUG(FUNCTION, device->debug, "control %x\n",msg);

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			for (i=0; i<6; i++)
				((uint8 *)buf)[i] = device->mac_address.ebyte[i];
			return B_OK;
		}
		case ETHER_INIT:
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			*(uint32 *)buf = MAX_FRAME_SIZE;
			return B_OK;
			
		case ETHER_ADDMULTI:
			domulti(device, (char *) buf);
			return B_OK;
		
		case ETHER_SETPROMISC:
			setpromisc(device, *(uint32 *)buf );
			return B_OK;
			
		case ETHER_NONBLOCK:
			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_OK;
			
		default:
			ETHER_DEBUG(ERR,device->debug, "Unknown IOcontrol %x\n",msg);
			return B_ERROR;
	}

}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	dev_info_t *device = (dev_info_t *) _device;
	status_t    status;
	uint32 		frame_status;
	uint16		frame_size;
#if 0
	{
	int32 semcount;
	get_sem_count(device->ilock, &semcount);
	dprintf("RD entry rx[%d] rx_rcvd=%d sem=%d 0=%x 1=%x 2=%x 3=%x ", rx_now, device->rx_received, semcount,
		device->rx_desc[0]->status,	device->rx_desc[1]->status,device->rx_desc[2]->status,device->rx_desc[3]->status );
	}
#endif

		// Block until data is available
		if((status = acquire_sem_etc( device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0 )) != B_NO_ERROR) {
			*len = 0;
			return status;
		}
		// Protect againsts reentrant read
		if( atomic_or( &device->readLock, 1 ) ) {
			release_sem_etc( device->ilock, 1, 0 );
			ETHER_DEBUG(ERR,device->debug, "reentrant read\n");
			*len = 0;
			return B_ERROR;
		}

		ETHER_DEBUG(FUNCTION, device->debug, "read_hook buf=%x len=%x\n", (char *)buf, *len);

		/* three cases, frame is good, bad, or we don't own the descriptor */
		frame_status = device->rx_desc[device->rx_rh]->des0;

//		dprintf("rx: frame_status = %x %x, rx_rh=%x \n", frame_status >> 16, frame_status, device->rx_rh);

		if ((frame_status & rx_owns)) {		/*  card owns the buffer - this should never happen*/
			ETHER_DEBUG(ERR,device->debug,"RX buf owner err des0=%x rx_rh=%x rx_isr=%x rx_free=%x\n",
				device->rx_desc[device->rx_rh]->des0, device->rx_rh, device->rx_isr, device->rx_free);
			dump_rx_descriptors(device);
			*len = 0;
			return B_ERROR;
		}
		/* err frame */
		if (frame_status & (rx_err | rx_crc | rx_fae | rx_fov | rx_oversize | rx_runt | rx_sbe | rx_buf_err)) {
			*len = 0;
			ETHER_DEBUG(WARN, device->debug, "Rx err %x\n", frame_status);
		} else {
			uint16 frame_size = ((frame_status & rx_frame_len) >> 16) - 4; /* 4 bytes for checksum */
			if ((frame_size > MAX_FRAME_SIZE)|| (frame_size > *len )){ 
				ETHER_DEBUG(ERR, device->debug, "Rx Bad frame size %d \n", frame_size, *len);
				frame_size = min(frame_size, *len);
			}
			*len = frame_size;
			memcpy( buf, device->rx_buf[device->rx_rh], frame_size );
			if (device->debug & SEQ) 
			{ unsigned short  *seq = (unsigned short *) device->rx_buf[device->rx_rh];
				dprintf(" R%4.4x ", seq[20]);  /* sequence number */
			}
			 if (device->debug & RX) {
				dump_packet("rx: ",(unsigned char *)buf, frame_size);
			}
			
		}

		/* update indexes and buffer ownership */
		{	cpu_status  	former;
			former = disable_interrupts();
			acquire_spinlock( &device->rx_lock );
			device->rx_desc[device->rx_rh]->des0 |= rx_owns;		/* endian swap for PPC */ /* return buf to card */
			device->rx_rh = (device->rx_rh + 1) & RX_MASK;
			device->rx_free++;
			if (device->rx_free > RX_BUFFERS) dprintf("RX FREE=%d out of range\n");
			release_spinlock( &device->rx_lock );
    		restore_interrupts(former);
		}
		
		
	/* update acked index for next read call, and release reantrant lock */
	device->readLock = 0;
	return B_OK;

}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *len)
{
	dev_info_t *device = (dev_info_t *) _device;
	ulong buflen;
	int status;
	uint16 frame_size;

	ETHER_DEBUG(FUNCTION, device->debug, "write_hook buf=%x len=%x\n", (char *)buf, *len);
	
	if ( *len > MAX_FRAME_SIZE ) {
		ETHER_DEBUG(ERR, device->debug, ": write %d > 1514 tooo long\n",*len);
		*len = MAX_FRAME_SIZE;
	}
	
	if ( *len < MIN_FRAME_SIZE ) {
		*len = MIN_FRAME_SIZE;
	}
	frame_size = *len;
	
	if( (status = acquire_sem_etc( device->olock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR ) {
		*len = 0;
		return status;
	}

	// Protect against reentrant write
	if( atomic_or( &device->writeLock, 1 ) ) {
		release_sem_etc( device->olock, 1, 0 );
		*len = 0;
		ETHER_DEBUG(ERR, device->debug, "reentrant write\n");
		return B_ERROR;
	}

	if (device->tx_desc[device->tx_wh]->des0 & tx_owns) {		/* card owns!? */
		ETHER_DEBUG(ERR, device->debug, "ERR write_hook: card owns buf[%d]\n",device->tx_wh);
		device->writeLock = 0;
		return B_ERROR;
	}

	// Copy data to buffer
	memcpy( device->tx_buf[device->tx_wh], buf, frame_size );
    device->tx_desc[device->tx_wh]->des1 = tx_ic | tx_eop | tx_sop |  frame_size; 
	{	cpu_status  	former;
		former = disable_interrupts();
		acquire_spinlock( &device->tx_lock );
		device->tx_desc[device->tx_wh]->des0 |= tx_owns;		/* endian swap for PPC */ /* give buf to card */
		device->tx_wh = (device->tx_wh + 1) & TX_MASK;
		device->tx_sent++;
		release_spinlock( &device->tx_lock );
   		restore_interrupts(former);
	}	
	
	write8(device->base + CR1, read8(device->base + CR1) | VR_CMD_TX_GO);  /* tx cmd to chip */
	
	/* Another write may now take place */
	device->writeLock = 0;
	return B_OK;

}

void rx_interrupt(dev_info_t *device) {

	uint16 rxcount = 0;
	int16 limit;

	/* scan the ring for new frames */
	acquire_spinlock( &device->rx_lock);
	for ( limit=device->rx_free; limit > 0; limit--) {
		if (device->rx_desc[device->rx_isr]->des0 & rx_owns) {
#if 0
			dprintf("rx_int: owns rh=%x isr=%x free=%x limit=%x\n", device->rx_rh, device->rx_isr,
				device->rx_free, limit);
			dump_rx_descriptors(device);
#endif
			break;  /* card owns */
		}
		rxcount++;
		device->rx_isr = (device->rx_isr+1) & RX_MASK;
		device->rx_free--;
		if (device->rx_free < 0) dprintf("ISR rx_free=%d out of range\n", device->rx_free);
	}
	release_spinlock(&device->rx_lock);

	if (rxcount) { /* signal data has arrived */
		release_sem_etc( device->ilock, rxcount, B_DO_NOT_RESCHEDULE );
	} else { /* got a spurious interrupt, or pointers are out of sync */
		ETHER_DEBUG(WARN, device->debug, "rx_int: sync rh=%x isr=%x free=%x\n", device->rx_rh, device->rx_isr, device->rx_free);
		dump_rx_descriptors(device);
	}
}

void tx_interrupt(dev_info_t *device) {
		int32 tx_count = 0;
		uint16 tx_status;
		int16 limit;

		acquire_spinlock( &device->tx_lock);
		for (limit = device->tx_sent; limit > 0; limit--) {
			if (device->tx_desc[device->tx_isr]->des0 & tx_owns) {
#if 0			/* device generates extra interrupts */
				ETHER_DEBUG(ERR, device->debug, "ISR_TX owns err wh=%x isr=%x sent=%x limit=%x\n",
					device->tx_wh, device->tx_isr, device->tx_sent, limit);
				dump_tx_descriptors(device);
#endif
				break;
			}
			tx_status = device->tx_desc[device->tx_isr]->des0 & 0xFFFF;
			if (tx_status & (tx_jabber | tx_crs | tx_serr | tx_owc | tx_abt | tx_udf)) {
				dprintf("tx err %x\n", tx_status);
			
				if (tx_status & (tx_abt | tx_udf)) {		/* tx abort or underflow */
					ETHER_DEBUG(ERR, device->debug, "ISR TX abort | underflow - reseting.\n");
					write16( device->base + CR0, read16((uint32)device->base + CR0) | VR_CMD_TX_ON | VR_CMD_TX_GO);
					dprintf("TO DO: reset indexes and descriptor buffers ownership, and semaphore here\n");
				}
			}	else {	/* normal tx */
				device->tx_desc[device->tx_isr]->des0 = 0;
			}
			tx_count++;	/* this many buffers are free */
			device->tx_isr = (device->tx_isr + 1) & TX_MASK;
			device->tx_sent--;
			if ((device->tx_sent < 0) || (device->tx_sent > TX_BUFFERS)) {
				dprintf("ERR tx_sent = %d\n", device->tx_sent);
			}
		}
		release_spinlock( &device->tx_lock);
		if (tx_count) {
			release_sem_etc(device->olock, tx_count, B_DO_NOT_RESCHEDULE );
		}
}

/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	dev_info_t *device = (dev_info_t *) _device;
	unsigned char isr;
	int32 status;
	int32 handled = B_UNHANDLED_INTERRUPT;

	/* get and clear card hardware interrupt indicator flags */

//	write16(device->base + IMR0, 0); 

    status=read16(device->base + ISR0);

	ETHER_DEBUG(INTERRUPT,device->debug, "ISR %x\n", status);
	
	if (status == 0) {		/* shared interrupts go to next devices ISR */
		handled =  B_UNHANDLED_INTERRUPT;
	} else {
		write16(device->base + ISR0, status); 
		handled = B_INVOKE_SCHEDULER;
		
		if (status & ( PRX | RXE| RU )) {
			rx_interrupt(device);
		}
	
		    
		if (status & (PTX| TXE| TU)) {  /* transmit packet */
			tx_interrupt(device);	
		}
	}

//	write16(device->base + IMR0, IMR0_MASK);
	
	return handled;
}



int32 get_pci_list(pci_info *info[], int32 maxEntries)
{
	status_t status;
	int32 i, entries;
	pci_info	*item;
	
	item = (pci_info *)malloc(sizeof(pci_info));
	
	for (i=0, entries=0; entries<maxEntries; i++) {
		if ((status = gPCIModInfo->get_nth_pci_info(i, item)) != B_OK)
			break;
		if ((item->vendor_id == VENDOR_ID_VIA)&& (item->device_id == DEVICE_ID_VT86C100)) {
			/* check if the device really has an IRQ */
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf(kDevName " found with invalid IRQ - check IRQ assignement");
				continue;
			}
			dprintf(kDevName ": found at IRQ %x \n", item->u.h0.interrupt_line);
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
		}
	}
	info[entries] = NULL;
	free(item);
	return entries;
}

status_t free_pci_list(pci_info *info[])
{
	int32 		i;
	pci_info	*item;
	
	for (i=0; (item=info[i]); i++)
		free(item);
	return B_OK;
}


status_t enable_addressing(dev_info_t *device)
{

	// Turn on I/O port decode, Memory Address Decode, and Bus Mastering
	gPCIModInfo->write_pci_config(device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, PCI_command, 2,
		PCI_command_io | PCI_command_memory | PCI_command_master |
		gPCIModInfo->read_pci_config( device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, PCI_command, 2));

#if IO_PORT_PCI_ACCESS
	device->base = device->pciInfo->u.h0.base_registers[0];
#endif /* IO_PORT_PCI_ACCESS */

#if MEMORY_MAPPED_PCI_ACCESS
	{
	int32		base, size, offset, i;
	int32		*ptr;
	const		MemoryReg = 1;
	
	base = device->pciInfo->u.h0.base_registers[MemoryReg];
	size = device->pciInfo->u.h0.base_register_sizes[MemoryReg];
	
	/* use "poke" with the "pci" command from a terminal session to check these values */
	ETHER_DEBUG(PCI_IO,  device->debug, "PCI base=%x size=%x\n", base, size);

	// Round down to nearest page boundary
	base = base & ~(B_PAGE_SIZE-1);
	
	// Adjust the size
	offset = device->pciInfo->u.h0.base_registers[MemoryReg] - base;
	size += offset;
	size = (size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

	ETHER_DEBUG(PCI_IO,  device->debug, "Now PCI base=%x size=%x offset=%x\n", base, size, offset);
		
	if ((device->reg_area = map_physical_memory(kDevName " Regs", (void *)base, size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &device->base)) < 0)
		return B_ERROR;

	device->base = device->base + offset;  //  /sizeof (int32) ??
	}
#endif /*MEMORY_MAPPED_PCI_ACCESS */
	
	ETHER_DEBUG(PCI_IO,  device->debug, "base=%x\n", device->base);

	return B_OK;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

status_t init_ring_buffers(dev_info_t *device)
{
	uint32 	size;
	
	/* create transmit buffer area */
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_buf_area = create_area( kDevName " tx buffers", (void **) device->tx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, " create tx buffer area failed %x \n", device->tx_buf_area);
		return device->tx_buf_area;
	}
	/* create tx descriptor area */
	size = RNDUP( sizeof(frame_desc_t) * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_desc_area = create_area( kDevName " tx descriptors", (void **) device->tx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, kDevName " create tx descriptor area failed %x \n", device->tx_desc_area);
		delete_area(device->tx_buf_area);
		return device->tx_desc_area;
	}
	
	/* init tx buffer descriptors */
	{	
		physical_entry		entry;
		uint16 i;
		/* init ring buffers with virtual addresses */
		for ( i = 0; i < TX_BUFFERS; i++) {
			device->tx_buf[i] = device->tx_buf[0] + (i * BUFFER_SIZE);
			device->tx_desc[i] = (frame_desc_t *) (((uint32)device->tx_desc[0]) + (i * sizeof(frame_desc_t)));
		}
		/* init descriptors with physical addresses */
		for ( i = 0; i < TX_BUFFERS; i++) {
			get_memory_map( (uint8 *)device->tx_buf[i], 4, &entry, 1 );
			device->tx_desc[i]->buf = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			get_memory_map( (uint8 *)device->tx_desc[(i+1) & TX_MASK], 4, &entry, 1 );
			device->tx_desc[i]->next = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			device->tx_desc[i]->des0 = 0;
			device->tx_desc[i]->des1 = tx_ic|tx_eop|tx_sop; /* interrupt after tx, no chaining */
		}
		if (device->debug & TX) {
			for ( i = 0; i < TX_BUFFERS; i++) {
				dprintf("tx_desc[%3.3d]=%8.8x des0=%8.8x des1=%8.8x buf=%8.8x next=%8.8x\n", i,device->tx_desc[i],
					device->tx_desc[i]->des0, device->tx_desc[i]->des1, device->tx_desc[i]->buf, device->tx_desc[i]->next);
			}
		}	
	}

	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, " create rx buffer area failed %x \n", device->rx_buf_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		return device->rx_buf_area;
	}
	/* create rx descriptor area */
	size = RNDUP( sizeof(frame_desc_t) * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_desc_area = create_area( kDevName " rx descriptors", (void **) device->rx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, " create rx descriptor area failed %x \n", device->rx_desc_area);
		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		return device->rx_desc_area;
	}
	/* init rx buffer descriptors */
	{	
		physical_entry		entry;
		uint16 i;
		for ( i = 0; i < RX_BUFFERS; i++) {
			device->rx_buf[i] = device->rx_buf[0] + (i * BUFFER_SIZE);
			device->rx_desc[i] = (frame_desc_t *) (((uint32)device->rx_desc[0]) + (i * sizeof(frame_desc_t)));
		//	dprintf("rx_buf[%3.3d] = %x\n", i, device->rx_buf[i]);
		}
	
		for ( i = 0; i < RX_BUFFERS; i++) {
			get_memory_map( (uint8 *)device->rx_buf[i], 4, &entry, 1 );
			device->rx_desc[i]->buf = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			get_memory_map( (uint8 *)device->rx_desc[(i+1) & RX_MASK], 4, &entry, 1 );
			device->rx_desc[i]->next = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			device->rx_desc[i]->des0 = rx_owns;	/* card owns it */
			device->rx_desc[i]->des1 = rx_ic | rx_sop | rx_eop | 0x600;	/* interupt on, only in chain, size 1536 */
//			device->rx_desc[i]->des1 = 0x600;	/* interupt on, only in chain, size 1536 */
		}

		if (device->debug & RX) {
			dump_rx_descriptors(device);
			snooze(200000); // let dprintfs finish
		}	
	}

	return B_OK;
}

void free_ring_buffers(dev_info_t *device) {

		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		delete_area(device->rx_desc_area);
}


/*
 * Copy a packet from the ethernet card
 */
static int
copy_packet(dev_info_t *device, unsigned char *ether_buf, int buflen)
{
	int ether_len = 0;

	ETHER_DEBUG(RX, device->debug,  "read packet in from card\n");

	return (ether_len);
}



void get_mac_address(dev_info_t *device) {
	
	int j;
	dprintf(kDevName ": Ethernet address: ");
	for (j=0; j<6; j++) {
		device->mac_address.ebyte[j] = read8((uint32)device->base + PAR0+j);
		dprintf(" %x", device->mac_address.ebyte[j]);
	}
	dprintf("\n");
}


void reset_device(dev_info_t *device) {
	physical_entry		entry;
	uint32 				max_tries;
		
	/*write TxDesc RxTDesc  to NIC */
	get_memory_map( (uint8 *)device->tx_desc[0], 4, &entry, 1 );
	write32(device->base + TDA0, B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));
//dprintf("tx_desc=%8.8x pa=%8.8x\n", device->tx_desc[0],  B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));


	get_memory_map( (uint8 *)device->rx_desc[0], 4, &entry, 1 );
	write32(device->base + RDA0, B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));
//dprintf("rx_desc=%8.8x pa=%8.8x\n", device->rx_desc[0],  B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));

	device->tx_wh = device->tx_isr = device->rx_rh = device->rx_isr = 0;	/* reset indexes */ 
	
	device->rx_free = RX_BUFFERS;
	device->tx_sent = 0;

	write16(device->base + IMR0, 0x0000);	/* turn off interrupts */
	write8(device->base + CR1, 0x80);		/* software reset chip */
	
}

void CheckDuplex(dev_info_t *device) {
		
	uint16 link_partner = MII_Read(5,device) & 0x1F8;  // see IEE 802.3 supplement section 28.2.4.1.4
	uint16 advertising = MII_Read(4,device);

dprintf(kDevName ": CheckDuplex link_parnter %x = 0x1F8 & %x; advertising %x\n",
	link_partner, MII_Read(5,device) & 0x1F8, advertising);


	switch(link_partner) {
	case 0x1e0:
		ETHER_DEBUG(INFO, device->debug, "Link Partner advertises 100MB FD\n");
		device->duplex = true;
		break;
	case 0x60:
		ETHER_DEBUG(INFO, device->debug, "Link Partner advertises 10MB FD\n");
		device->duplex = true;
		break;
	case 0xa0: 
	case 0x80: 
		ETHER_DEBUG(INFO, device->debug, "Link Partner advertises 100MB HD\n");
		device->duplex = false;
		break;
	default :
		ETHER_DEBUG(INFO, device->debug, "Link Partner Half Duplex = %x \n",link_partner);
		/* fall thru to 10MB HD */
	case 0x20:
		device->duplex = false;
	}
}


/* set hardware so that all packets are received. */
void setpromisc(dev_info_t * device, uint32 on) {
	
		dprintf("To Do : verify promics mode works ...\n");
	
		if (on) {
			dprintf("promsicuous rx on");
			write8(device->base + RCR,  PRO | AM | AB | SEP | AR | read8(device->base + RCR)); 
		} else {
			dprintf("promsicuous rx off");
			write8(device->base + RCR, ~PRO | read8(device->base + RCR)); 
		}

		dprintf(" rcr=%x\n", read8(device->base+RCR));
}

static status_t
domulti( dev_info_t *device, char *addr )
{
	int i;
	int nmulti = device->nmulti;
	
	ETHER_DEBUG(FUNCTION,device->debug, "domulti %s\n", addr);
	// to do: Multi cast address filtering is possible with specific addresses on this chip.
	// for now, we just set the chip to accept all multicast addresses.
	
	write8(device->base + RCR, AM | read8(device->base + RCR)); 

#if 0
	if (nmulti == MAX_MULTI) {
		ETHER_DEBUG(ERR, device->debug, "vt86:too many multicast addresses %d\n", nmulti);
		return (B_ERROR);
	}
	for (i = 0; i < nmulti; i++) {
		if (memcmp(&device->multi[i], addr, sizeof(device->multi[i])) == 0) {
			break;
		}
	}
	if (i == nmulti) {
		/*Only copy if it isn't there already */
		memcpy(&device->multi[i], addr, sizeof(device->multi[i]));
		device->nmulti++;
	}
	if (device->nmulti == 1) {
		dprintf("domulti() add hardware specific multicast code here\n");
	}
#endif
	return (B_NO_ERROR);
}


/* dump ethernet packets */
void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
	dprintf("\n");
}


void dump_rx_descriptors(dev_info_t * device) {
	uint16 i;
	
	for ( i = 0; i < RX_BUFFERS; i++) {
		dprintf("rx_desc[%3.3d]=%8.8x des0=%8.8x des1=%8.8x pa_buf=%8.8x next=%8.8x buf=%8.8x\n", i,device->rx_desc[i],
			device->rx_desc[i]->des0, device->rx_desc[i]->des1, device->rx_desc[i]->buf, device->rx_desc[i]->next,
			device->rx_buf[i]);
	}
}
void dump_tx_descriptors(dev_info_t * device) {
	uint16 i;
	
	for ( i = 0; i < TX_BUFFERS; i++) {
		dprintf("tx_desc[%3.3d]=%8.8x des0=%8.8x des1=%8.8x pa_buf=%8.8x next=%8.8x buf=%8.8x\n", i,device->tx_desc[i],
			device->tx_desc[i]->des0, device->tx_desc[i]->des1, device->tx_desc[i]->buf, device->tx_desc[i]->next,
			device->tx_buf[i]);
	}
}

/* vt86ether: Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   hit the alt sysreq keys, and type "vt86ether <cr>"
*/
#if DEBUGGER_COMMAND
static int
vt86c100(int argc, char **argv)
{
	uint16 i,j,found=0xFFFF;
	dev_info_t *device = gdev;
	const char * usage = "usage: vt86c100 [globals,rx,tx,pci,functions,write, sequence_nubmer_rx]\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'g':
			kprintf("vt86c100 globals\n");
			{
			kprintf("CR0 = %8.8x\n", read16(device->base + CR0));
			kprintf("ISR0 = %8.8x\n", read16(device->base + ISR0));
			kprintf("RDA0 = %8.8x\n", read16(device->base + RDA0));
			kprintf("RDA2 = %8.8x\n", read16(device->base + RDA2));
			kprintf("TDA0 = %8.8x\n", read16(device->base + TDA0));
			kprintf("TDA2 = %8.8x\n", read16(device->base + TDA2));
			kprintf( "RX indexes rh=%x isr=%x free=%x\n",device->rx_rh, device->rx_isr, device->rx_free);
			kprintf( "TX indexes wh=%x isr=%x sent=%x\n",device->tx_wh, device->tx_isr, device->tx_sent);
			dump_rx_descriptors(device);
			dump_tx_descriptors(device);
			}
			break; 
		case 'f':
			device->debug ^= FUNCTION;
			if (device->debug & FUNCTION) 
				kprintf("Function trace Enabled\n");
			else 			
				kprintf("Function trace Disabled\n");
			break; 
			break; 
		case 'r':
			device->debug ^= RX;
			if (device->debug & RX) 
				kprintf("Recieve packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break; 
		case 's':
			device->debug ^= SEQ;
			if (device->debug & SEQ) 
				kprintf("Recieve packet ping sequence number trace Enabled\n");
			else 			
				kprintf("Recieve packet ping sequence number trace Disabled\n");
			break; 
		case 't':
			device->debug ^= TX;
			if (device->debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'p':
			break; 
		case 'w':
				if (i < 4) { kprintf("usage: write addr value\n"); return 0; }
				kprintf("Wrote %x <- %x\n",  (int) strtol (argv[j+1], (char **) NULL, 16),
					(int) strtol (argv[j+2], (char **) NULL, 16));
				j +=2; i-=2;
				break;
		default:
			kprintf("%s",usage);
			return 0;
		}
	}
	
	return 0;
}
#endif

/*
 * Allocate and initialize semaphores and spinlocks.
 */
status_t allocate_resources(dev_info_t *device) {
		
	/* Setup Semaphores */
	if ((device->ilock = create_sem(0, kDevName " rx")) < 0) {
		dprintf(kDevName " create rx sem failed %x \n", device->ilock);
		return (device->ilock);
	}
	set_sem_owner(device->ilock, B_SYSTEM_TEAM);
	
	/* intialize tx semaphore with the number of free tx buffers */
	if ((device->olock = create_sem(TX_BUFFERS, kDevName " tx")) < 0) {
		dprintf(kDevName " create tx sem failed %x \n", device->olock);
		return (device->olock);
	}
	set_sem_owner(device->olock, B_SYSTEM_TEAM);

	device->tx_lock = device->rx_lock = 0;
	
	return (B_OK);
}

void free_resources(dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->olock);
}


#define MII_READ_BIT 0x40
/* Read the Media Independent Interface - our window to the physical media */
uint16 MII_Read(uint16 MII_register,  dev_info_t *device) {
   int limit = 10;

   write8(device->base + MIIAD, MII_register);
   snooze(4000);

   write8(device->base + MIICR, MII_READ_BIT);
   snooze(4000);

	while ( (read8(device->base + MIICR) & MII_READ_BIT) && (limit--) ) {
		snooze(400);
	}

	if (limit == 0) {
		dprintf("MII_Read failed\n");
		return -1;
	} else {
		return(read16(device->base + MIIDATA));
	}

}
#if 0
void MII_Write(char byMIISetByte,char byMIISetBit,char byMIIOP,int ioaddr)
{
   int  ReadMIItmp;
   int  MIIMask;
   char byMIIAdrbak;
   char byMIICRbak;
   char byMIItemp;

   
   byMIIAdrbak=inb(byMIIAD);

   byMIICRbak=inb(byMIICR);
   outb(byMIICRbak&0x7f,byMIICR);
   snooze(4000);
   outb(byMIISetByte,byMIIAD);
   snooze(4000);

   outb(inb(byMIICR)|0x40,byMIICR);

   byMIItemp=inb(byMIICR);
   byMIItemp=byMIItemp&0x40;

   while(byMIItemp!=0)
   {
     byMIItemp=inb(byMIICR);
     byMIItemp=byMIItemp&0x40;
   }
   snooze(4000);

   ReadMIItmp=inw(wMIIDATA);
   MIIMask=0x0001;
   MIIMask=MIIMask<<byMIISetBit;


   if (byMIIOP==0){
       MIIMask=~MIIMask;
       ReadMIItmp=ReadMIItmp&MIIMask;
   }else{
       ReadMIItmp=ReadMIItmp|MIIMask;

   }
   outw(ReadMIItmp,wMIIDATA);
   snooze(4000);

   outb(inb(byMIICR)|0x20,byMIICR);
   byMIItemp=inb(byMIICR);
   byMIItemp=byMIItemp&0x20;

   while(byMIItemp!=0)
   {
     byMIItemp=inb(byMIICR);
     byMIItemp=byMIItemp&0x20;
   }
   snooze(4000);

   outb(byMIIAdrbak&0x7f,byMIIAD);
   outb(byMIICRbak,byMIICR);
   snooze(4000);

}

#endif 


int32	api_version = B_CUR_DRIVER_API_VERSION;
