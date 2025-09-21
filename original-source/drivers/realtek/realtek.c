/*
 * RealTek 8139 PCI 10/100 Ethernet driver
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

/* PCI vendor and device ID's for Reaktek RTL8139 */
#define VENDOR_ID_REALTEK       0x10EC
#define DEVICE_ID_RTL8129       0x8129
#define DEVICE_ID_RTL8139       0x8139

/* Accton MPX 5030/5038 device ID. */
#define VENDOR_ID_ACCTON        0x1113
#define DEVICE_ID_EN5030	    0x1211

/* Delta Electronics Vendor ID.*/
#define DELTA_VENDORID          0x1500
/* Delta device IDs.*/
#define DELTA_DEVICEID_8139     0x1360
/* Addtron vendor ID.*/
#define ADDTRON_VENDORID        0x4033
#define ADDTRON_DEVICEID_8139   0x1360



/* globals for driver instances */ 
#define kDevName "rtl8139"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4			/* maximum number of driver instances */


/*  Bus logic problems exists with memory mapped IO on this chip
 *  so we use port IO
 */

#define write8(address, value)			(*gPCIModInfo->write_io_8)((address), (value))
#define write16(address, value)			(*gPCIModInfo->write_io_16)((address), (value))
#define write32(address, value)			(*gPCIModInfo->write_io_32)((address), (value))
#define read8(address)					((*gPCIModInfo->read_io_8)(address))
#define read16(address)					((*gPCIModInfo->read_io_16)(address))
#define read32(address)					((*gPCIModInfo->read_io_32)(address))


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
/* diagnostic debug flags - compile in here or set while running with debugger "AcmeRoadRunner" command */
#define DEFAULT_DEBUG_FLAGS ( ERR | INFO )
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN )

void ETHER_DEBUG(int32 debug_mask, int32 enabled, char * format, ...) {
	if (debug_mask & enabled) {
		va_list		args;
		char		s[4096];
		va_start(args, format);
		vsprintf( s, format, args );
		va_end(args);
		dprintf("%s",s);
	}
}

void dump_packet( const char * msg, unsigned char * buf, uint16 size) {

	uint16 j;
	
	dprintf("%s dumping %x size %d", msg, buf, size);
	for (j=0; j<size; j++) {
		if ((j & 0xF) == 0) dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
	dprintf("\n");
}

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


/* Symbolic offsets to registers. */
enum RTL8129_registers {
	MAC0=0,						/* Ethernet hardware address. */
	MAR0=8,						/* Multicast filter. */
	TxStatus0=0x10,				/* Transmit status (Four 32bit registers). */
	TxAddr0=0x20,				/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,					/* A general-purpose counter. */
	RxMissed=0x4C,				/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	FlashReg=0x54, GPPinData=0x58, GPPinDir=0x59, MII_SMI=0x5A, HltClk=0x5B,
	MultiIntr=0x5C, TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	/* Undocumented registers, but required for proper operation. */
	FIFOTMS=0x70,	/* FIFO Test Mode Select */
	CSCR=0x74,	/* Chip Status and Configuration Register. */
	PARA78=0x78, PARA7c=0x7c,	/* Magic transceiver parameter register. */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000, TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

/* Twister tuning parameters from RealTek.
   Completely undocumented, but required to tune bad links. */
enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};
unsigned long param[4][4]={
	{0x0cb39de43,0x0cb39ce43,0x0fb38de03,0x0cb38de43},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0bb39de43,0x0bb39ce43,0x0bb39ce83,0x0bb39ce83}
};

#define MAX_MULTI 32 			/* maximum number of multi cast addresses filtered */

/* ring buffer sizes - tune for optimal performance */
#define TX_BUFFERS					4		/* must be a power of 2 */
#define RX_BUFFERS					32		/* must be a power of 2 */
#define BUFFER_SIZE					2048	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MTU_SIZE					1514	/* 1514 data + 4 bytes crc */
#define TX_MASK 					(TX_BUFFERS - 1) 

#define TX_FIFO_THRESH 256					/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024. */
#define RX_FIFO_THRESH	4		/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4		/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4		/* Calculate as 16<<val. */

/* Size of the in-memory receive ring. */
#define RX_BUF_LEN_IDX	3			/* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)

/* per driver instance globals */
typedef struct {
	int32           devID; 						/* device identifier: 0-n */
	pci_info        *pciInfo;
	uint16          irq;						/* our IRQ line */
	sem_id          ilock, olock;				/* i/o semaphores */
	int32			readLock, writeLock;		/* to prevent reentrant read & write */
	area_id         tx_buf_area; 				/* transmit descriptor and buffer areas */
	area_id         rx_buf_area; 				/* receive descriptor and buffer areas */
	uint32          base; 						/* Base address of hostRegs */
	uint32          debug;						/* debuging level */
	int32           blockFlag;					/* sync or async io calls */
	ether_address_t mac_address;				/* my ethernet address */
	ether_address_t multi[MAX_MULTI];			/* multicast addresses */
	uint16          nmulti;						/* number of multicast addresses */
	
	uint32          tx_sent, tx_acked;			/* tx buffer indexes - sent and confirmed sent */
	uchar           *tx_buf_virt[TX_BUFFERS];	/* Tx buffers - virtual addresses */
	uint32          tx_buf_phys[TX_BUFFERS];	/* Tx buffers - physical addresses */
	uint32          tx_threshold;				/* transit threshold */

	uint32          cur_rx;						/* Index into the Rx buffer of next Rx pkt. */
	uchar          	*rx_buf_virt[1];			/* receive buffers - virtual address */
	uint32          rx_buf_phys[1];				/* receive buffers - physical address */

	char phys[4];								/* MII device addresses. */
	char twistie, twist_cnt;					/* Twister tune state. */
	unsigned int full_duplex:1;					/* Full-duplex operation requested. */
	unsigned int duplex_lock:1;
	unsigned int default_port:4;				/* Last dev->if_port value. */
	unsigned int media2:4;						/* Secondary monitored media port. */
	unsigned int medialock:1;					/* Don't sense media type. */
	unsigned int mediasense:1;					/* Media sensing in progress. */
	uchar 		 promiscuous;					/* receive all frames */

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
static int32    interrupt_hook(void *_device);

static int32 get_pci_list(pci_info *info[], int32 maxEntries); 	/* Get pci_info for each device */
static status_t free_pci_list(pci_info *info[]);					/* Free storage used by pci_info list */
static status_t enable_addressing(dev_info_t *device); 			/* enable pci io address space for device */
static status_t init_ring_buffers(dev_info_t *device);				/* allocate and initialize frame buffer rings */
static void free_ring_buffers(dev_info_t *device);					/* allocate and initialize frame buffer rings */
static status_t allocate_resources(dev_info_t *device);			/* allocate semaphores & spinlocks */
static void  free_resources(dev_info_t *device);					/* deallocate semaphores & spinlocks */
static void get_mac_address(dev_info_t *device);					/* get ethernet address */
static void reset_device(dev_info_t *device);				/* reset the lan controller (NIC) hardware */
static int read_eeprom(dev_info_t *device, uint32 location, uint32 addr_len);
static void rx_init(dev_info_t *device);

/* serial debug command */
static int rtl8139(int argc, char **argv);

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
	
	dprintf(kDevName ": init_driver\n");

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
	
	dprintf(kDevName ": Init Ok. Found %ld devices.\n", i );

	return B_OK;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;

	//dprintf(kDevName ": uninit_driver\n");

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

	dprintf(kDevName ": find_device() %s\n", name);

	/* Find device name */
	for (i=0; (item=gDevNameList[i]); i++)
	{
		if (strcmp(name, item) == 0)
		{
			dprintf(kDevName ": Found Device!\n");
			return &gDeviceHooks;
		}
	}
	return NULL; /*Device not found */
}

static void reset_device(dev_info_t * device) {
	uint32 i;
	
	/* Check that the reset started in the open hook has completed */
	for (i = 0; i < 1000; i++) {
		if ((read8(device->base + ChipCmd) & CmdReset) == 0)
			break;
		snooze(10);
	}
	ETHER_DEBUG(WARN, device->debug, "Chip reset after %d\n", i);

	get_mac_address(device);

	/* set chips address filter to receive frames sent to its mac address */
	/* Mac address filtering doesn't work - may need to do 32 bit writes here */
	for (i = 0; i < 6; i++)
		write8(device->base + MAC0 + i, device->mac_address.ebyte[i]);

//write32(device->base, 0x77c74000); write32(device->base+4, 0x00000c63);
//write32(device->base, 0x0040c777); write32(device->base+4, 0x00000c63);

	dprintf("Mac Addr ");
	for (i = 0; i < 6; i++) {
		dprintf("%2.2x : ", read8(device->base + i));
	}
	dprintf("\n");
	
	ETHER_DEBUG(WARN, device->debug, "MAC0 %x %x\n", read32(device->base+MAC0),read32(device->base+MAC0+4));

	rx_init(device);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	write8(device->base + ChipCmd, CmdRxEnb | CmdTxEnb);
	write32(device->base + RxConfig, (RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST<<8));
	write32(device->base + TxConfig, (TX_DMA_BURST<<8)|0x03000000);
	device->tx_threshold = (TX_FIFO_THRESH<<11) & 0x003f0000;



	write8(device->base + Cfg9346, 0xC0);
	write8(device->base + Config1, device->full_duplex ? 0x60 : 0x20);
	write8(device->base + Cfg9346, 0x00);

	write32(device->base + RxBuf, device->rx_buf_phys[0]);

	/* Start the chip's Tx and Rx process. */
	write32(device->base + RxMissed, 0);

	rx_init(device);

	write8(device->base + ChipCmd, CmdRxEnb | CmdTxEnb);

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
	if (!devName)  return EINVAL;
	
	mask = 1 << devID;   /* Check if the device is busy and set in-use flag if not */
	if (atomic_or(&gOpenMask, mask) &mask) {
		return B_BUSY;
	}
	/* Allocate storage for the per instance driver globals */
	if (!(*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));
	
	device->pciInfo = gDevList[devID];
	device->devID = devID;

	
	device->debug = DEFAULT_DEBUG_FLAGS;

	ETHER_DEBUG(FUNCTION, device->debug, "open %s device=%x\n", name, device);
	
	gdev = device;  /* global for debug comman - works for one driver instance only */
	add_debugger_command (kDevName, rtl8139, "Ethernet driver Info");

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	if ((status = enable_addressing(device)) != B_OK) { /* enable access to the cards address space */
		goto err1;
	}
	write8(device->base + Config1, 0); 					/* Bring the chip out of low-power mode. */
	write8(device->base + ChipCmd, CmdReset); 			/* reset the chip. */
	
	if (init_ring_buffers(device) != B_OK) { 			/* allocate and initialize frame buffer rings & descriptors */
		goto err2;
	}
	reset_device(device); 								/* hardware specific reset */
		
	/* install and enable interrupts */
	install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, *cookie, 0 );
	
	write16(device->base + IntrMask,
		PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver| TxErr | TxOK | RxErr | RxOK);
		
	// Start mii_poll deamon
	//	register_kernel_daemon( media_watch, device, 50 );
	
	ETHER_DEBUG( WARN, device->debug, "open ok\n" );
	
	return B_OK; // We are ready to go!
	
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


	/* Disable interrupts by clearing the interrupt mask. */
	write16(device->base + IntrMask, 0x0000);

	/* Stop the chip's Tx and Rx DMA processes. */
	write8(device->base + ChipCmd, 0x00);

	/* Update the error counts. */
	write16(device->base + RxMissed,0);

	/* Green! Put the chip in low-power mode. */
	write8(device->base + Cfg9346, 0xC0);
	write8(device->base + Config1, 0x03);
	write8(device->base + HltClk, 'H');		/* 'R' would leave the clock running. */

	
	/* Release resources we are holding */
	remove_debugger_command (kDevName, rtl8139);
	
	free_resources(device);
			
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	dev_info_t *device = (dev_info_t *) cookie;

	ETHER_DEBUG(FUNCTION, device->debug, ": free %x\n",device);

	// Remove MII Poll daemon
	//	unregister_kernel_daemon( mii_poll, device );
	
	// Remove Interrupt Handler
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, cookie );

	free_ring_buffers(device);
	
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

	ETHER_DEBUG(FUNCTION, device->debug, "IO control %x\n",msg);

	switch (msg) {
		case ETHER_GETADDR: {
			uint8 i;
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->mac_address.ebyte[i];
				dprintf( "%2.2x:", ((uint8 *)buf)[i]);
			}
			return B_OK;
		}
		case ETHER_INIT:
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			*(uint32 *)buf = 1514;
			return B_OK;
			
		case ETHER_ADDMULTI:
			device->nmulti = true;
			dprintf("multcast faked for now\n");
//			rx_init(device);
			return B_OK;
		
		case ETHER_SETPROMISC:
			device->promiscuous = true;
			rx_init(device);
			return B_OK;
		
		case ETHER_NONBLOCK:
			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_NO_ERROR;
			
		default:
			ETHER_DEBUG(ERR,device->debug, "unknown iocontrol %x\n",msg);
			return B_ERROR;
	}

}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	dev_info_t *device = (dev_info_t *) _device;
	ulong buflen;
	int packet_len;
	status_t status;
	

	if((status = acquire_sem_etc( device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0 )) != B_NO_ERROR) {
		*len = 0;
		return status;
	}

	if( atomic_or( &device->readLock, 1 ) ) {
		release_sem_etc( device->ilock, 1, 0 );
		ETHER_DEBUG(ERR, device->debug, "reentrant read()\n");
		*len = 0;
		return B_ERROR;
	}
	
//	ETHER_DEBUG(WARN, device->debug,"read() rx[%4.4x] bufaddr %4.4x free to %4.4x, Cmd %2.2x.\n",
//	   device->cur_rx, read16(device->base+RxBufAddr), read16(device->base+RxBufPtr), read8(device->base + ChipCmd));

	{
	uchar * rx_ring = device->rx_buf_virt[0];
	uint16 cur_rx = device->cur_rx;
	int32 ring_offset = cur_rx % RX_BUF_LEN;
	uint32 rx_status = *(uint32*)(rx_ring + ring_offset);
	int rx_size = rx_status >> 16;
		
		*len = min(*len, rx_size);
//		ETHER_DEBUG(WARN, device->debug, "rx len=%x rx_size=%x\n", *len, rx_size);
		
		if (rx_status & RxTooLong) {
				ETHER_DEBUG(WARN, device->debug, "Ethernet frame tooo big, status %4.4x!\n",rx_status);
				*len = 0;
				status=B_ERROR;
				/* The chip hangs here. */
		} else if (rx_status & (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign)) {
				ETHER_DEBUG(WARN, device->debug, "%Ethernet frame error status=%4.4x.\n", rx_status);
			/* Reset the receiver, based on RealTek recommendation. (Bug?) */
			device->cur_rx = 0;
			write8(device->base + ChipCmd, CmdTxEnb);
			write8(device->base + ChipCmd, CmdRxEnb | CmdTxEnb);
			write32(device->base + RxConfig, (RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST<<8));
			rx_init(device);
			*len = 0;
			status = B_ERROR;
		} else {
			/* frame wraps - copy in 2 parts */
			if (ring_offset+ *len +4 > RX_BUF_LEN) {
				int semi_count = RX_BUF_LEN - ring_offset - 4;
				memcpy(buf, &rx_ring[ring_offset + 4], semi_count);
				memcpy(buf, rx_ring, *len-semi_count);
			} else {
				memcpy(buf, &rx_ring[ring_offset + 4], *len);
			}
			status = B_OK;
		}
		cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
		write16(device->base + RxBufPtr, cur_rx - 16);
		device->cur_rx = cur_rx;

		{	
		int32 semcount;
		get_sem_count(device->ilock, &semcount);
		
		ETHER_DEBUG(WARN,device->debug, "DoneRead, sem=%d current %4.4x BufAddr %4.4x, free to %4.4x, Cmd %2.2x.\n",
			semcount, device->cur_rx, read16(device->base + RxBufAddr),read16(device->base + RxBufPtr), read8(device->base + ChipCmd));

#if 0
		if ( ((read16(device->base + RxBufAddr) - device->cur_rx > 4) )&& (semcount == 0) && ((read8(device->base + ChipCmd) & 1) == 0))
		 {
				ETHER_DEBUG(WARN, device->debug, "Extra Release_Sem\n");
				release_sem_etc( device->ilock, 1, 0 );
		}
#endif
		}

/* remove this when you figure out how to make mac address filtering work with the hardware */
#define PROMISC_HACK true
#if PROMSIC_HACK	
	{ int i;
		for( i=5; i>0; i-- )
		{
			if( ((char *) buf[i] != device->mac_address.ebyte[i] )
			&& ((char *) buf[i] != 0xff) )
			{
				*len = 0;
				status = B_ERROR;
				break;
			}
		}
	}
#endif

//		 if (*len) *len -= 4;
		 if ( (*len) && (device->debug & RX)) {
			dump_packet("rx", buf, *len);
		}
	}
	device->readLock = 0;
	return status;

}


/*
 * The write() system call - upper layer interface to the ethernet driver
 */
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *length)
{
	dev_info_t *device = (dev_info_t *) _device;
	size_t	frame_size;
	int tx_now;
	status_t status;

	ETHER_DEBUG(FUNCTION, device->debug, " write buf %x len %d (decimal)\n", buf, *length);

#if 0
	if( !(device->linkStatus & 0x0004) ){
		snooze( 10000 );
		*length = 0;
		return B_ERROR;
	}
#endif

	if( (status = acquire_sem_etc(device->olock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR) {
		*length = 0;
		return status;
	}
	
	if( atomic_or(&device->writeLock, 1)) {	/* Protect againsts reentrant write */
		release_sem_etc(device->olock, 1, 0);
		*length = 0;
		return B_ERROR;
	}

	/* check length */
	if( *length > MTU_SIZE )  *length = frame_size = MTU_SIZE;
	else if (*length < 60) frame_size= 60; /* chip doesn't pad */
	else frame_size = *length;
		
	if (device->debug & TX) {
//		dump_packet("tx", (unsigned char *) buf, 32);  // limit display
		dump_packet("tx", (unsigned char *) buf, *length);
	}

	tx_now = device->tx_sent & TX_MASK;
	memcpy(device->tx_buf_virt[tx_now], buf, frame_size);  /* Copy data to buffer */
	
	write32(device->base + TxAddr0 + tx_now*4, device->tx_buf_phys[tx_now]); /* does hardware require this write? */
	write32(device->base + TxStatus0 + tx_now*4, device->tx_threshold | frame_size);

	device->tx_sent = (device->tx_sent + 1 ) & TX_MASK;
	
	device->writeLock = 0;	/* another write may now happen */
	return B_OK;
}

/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	dev_info_t *device = (dev_info_t *) _device;
	uint16 status;
	int32 handled = B_UNHANDLED_INTERRUPT;
	int16 worklimit = 200;
	uchar link_changed = 0;
		
	do {
		/* get and clear card hardware interrupt indicator flags */
		status = read16(device->base + IntrStatus);

		/* Acknowledge all of the current interrupt sources ASAP, but
		an first get an additional status bit from CSCR. */
		if ((status & RxUnderrun)  &&  read16(device->base + CSCR) & CSCR_LinkChangeBit) {
			link_changed = 1;
			ETHER_DEBUG(WARN, device->debug, "ISR - Link Change status=%x CSCR=%x\n",
				status, read16(device->base + CSCR));
		}
		write16(device->base + IntrStatus, status);

		ETHER_DEBUG(INTERRUPT,device->debug, "status=%x new status=%x\n", status, read16(device->base + IntrStatus));

		if ((status & (PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxFIFOOver|TxErr|TxOK|RxErr|RxOK)) == 0)
			break;

		handled = B_INVOKE_SCHEDULER;

		if (status & (RxOK|RxUnderrun|RxOverflow|RxFIFOOver)) {
			if ((read8(device->base + ChipCmd) & RxBufEmpty) == 0) {
					int ring_offset = device->cur_rx % RX_BUF_LEN;
					uint32 rx_status = *(uint32 *)(device->rx_buf_virt[0] + ring_offset);
					int rx_size = rx_status >> 16;
//				dprintf( "rx_isr: status=%x cmd=%x rx_status=%x size=%x\n",
//					status, (read8(device->base + ChipCmd)),rx_status, rx_size);				
				release_sem_etc(device->ilock, 1, B_DO_NOT_RESCHEDULE );
				return handled;
			}
		}

		if (status & (TxOK | TxErr)) {
			int32 tx_count = 0;
			int32 limit = ((device->tx_sent & TX_MASK) - device->tx_acked) & TX_MASK;

#if 0
			{			
			int32 semcount;
			get_sem_count(device->olock, &semcount);
			dprintf("TX_ISR limit=%d acked=%d sent=%d status=%d sem_count=%d\n ",
				limit, device->tx_acked, device->tx_sent, status, semcount);
			dprintf("tx[0]=%x tx[1]=%x tx[2]=%x tx[3]=%x\n",
				read32(device->base + TxStatus0 + 0),read32(device->base + TxStatus0 + 4),
				read32(device->base + TxStatus0 + 8),read32(device->base + TxStatus0 + 12));

			}
#endif
			while (limit-- > 0) {
				int txstatus = read32(device->base + TxStatus0 + device->tx_acked*4);

				if ( ! (txstatus & (TxStatOK | TxUnderrun | TxAborted)))
					break;			/* It still hasn't been Txed */

				/* Note: TxCarrierLost is always asserted at 100mbps. */
				if (txstatus & (TxOutOfWindow | TxAborted)) {
					/* There was an major error, log it. */
						ETHER_DEBUG(WARN, device->debug,"ISR TX error status %8.8x.\n", txstatus);
					if (txstatus&TxAborted) {
						write32(device->base + TxConfig, (TX_DMA_BURST<<8)|0x03000001);
					}
				} else {
					if (txstatus & TxUnderrun) {
						/* Add 64 to the Tx FIFO threshold. */
						if (device->tx_threshold <  0x00300000)
							device->tx_threshold += 0x00020000;
					}
				}
				tx_count++;	/* this many buffers are free */
				device->tx_acked = (device->tx_acked + 1) & TX_MASK;
			}
			if (tx_count) {
				release_sem_etc(device->olock, tx_count, B_DO_NOT_RESCHEDULE );
			
			}
		}

		/* Check uncommon errors. */
		if (status & (PCIErr|PCSTimeout |RxUnderrun|RxOverflow|RxFIFOOver|TxErr|RxErr)) {
				ETHER_DEBUG(WARN, device->debug, "ISR err %8.8x.\n", status);

			if (status == 0xffffffff)
				break;
			/* Update the error count. */

			if ((status & RxUnderrun)  &&  link_changed) {
				int lpar = read16(device->base + NWayLPAR);
				int duplex = (lpar&0x0100) || (lpar & 0x01C0) == 0x0040; 
				if (device->full_duplex != duplex) {
					device->full_duplex = duplex;
					write8(device->base + Cfg9346, 0xC0);
					write8(device->base + Config1, device->full_duplex ? 0x60 : 0x20);
					write8(device->base + Cfg9346, 0x00);
				}
				status &= ~RxUnderrun;
			}

			if (status & RxOverflow) {
				device->cur_rx = read16(device->base + RxBufAddr) & (RX_BUF_LEN-1);
				write16(device->base + RxBufPtr, device->cur_rx - 16);
			}
			if (status & PCIErr) {
				ETHER_DEBUG(ERR, device->debug,"PCI Bus error %4.4x.\n",status);
			}
		}
		if (--worklimit < 0) {
			ETHER_DEBUG(WARN, device->debug, "Too mnny interrupts status=0x%4.4x.\n",status);
			/* Clear all interrupt sources. */
//			write16(device->base + IntrStatus, 0xffff);
			break;
		}
	
	} while (true);

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
        if (   (item->vendor_id == VENDOR_ID_REALTEK)&& (item->device_id == DEVICE_ID_RTL8139)
			|| (item->vendor_id == VENDOR_ID_ACCTON)&& (item->device_id == DEVICE_ID_EN5030)
		 /* || add clone vendors and devices here */ 
		   ) {
			/* check if the device really has an IRQ */
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf(kDevName " found with invalid IRQ - check IRQ assignement");
				continue;
			}
			dprintf(kDevName " found at IRQ %x \n", item->u.h0.interrupt_line);
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

	device->base = (uint32) device->pciInfo->u.h0.base_registers[0];
	
	ETHER_DEBUG(PCI_IO,  device->debug, "base=%x\n", device->base);

	return B_OK;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t init_ring_buffers(dev_info_t *device)
{
	uint32 	size;
	physical_entry		entry;
	uint16 i;
	
	/* zero indexes */
	device->tx_acked = device->tx_sent = device->cur_rx = 0;

	/* create transmit buffer area */
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_buf_area = create_area( kDevName "tx buffers", (void **) device->tx_buf_virt,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, "create tx buffer area failed %x \n", device->tx_buf_area);
		return device->tx_buf_area;  /* memory allocation failed */
	}
	
	/* init tx ring buffer addresses: virtual and physical */
	for ( i = 0; i < TX_BUFFERS; i++) {
		device->tx_buf_virt[i] = device->tx_buf_virt[0] + (i * BUFFER_SIZE);
		get_memory_map( (uint8 *)device->tx_buf_virt[i], 4, &entry, 1 );
		device->tx_buf_phys[i] = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));

		if (device->debug & WARN)
			dprintf(kDevName " tx_buf_virt[%3.3d]=%8.8x tx_buf_phys[%3.3d]=%8.8x\n",
				i,device->tx_buf_virt[i],i,device->tx_buf_phys[i]);
	}	

	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( kDevName " rx buffers", (void **) device->rx_buf_virt,
		B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA )) < 0) {
		ETHER_DEBUG(ERR, device->debug, "create rx buffer area failed %x \n", device->rx_buf_area);
		delete_area(device->tx_buf_area);
		return device->rx_buf_area;
	}	
	/* get rx buffer area physical address */
dprintf("rx_area=%x\n", device->rx_buf_area);

	get_memory_map( (uint8 *)device->rx_buf_virt[0], 4, &entry, 1 );
	device->rx_buf_phys[0] = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
	if (device->debug & WARN) {
		dprintf( kDevName "rx_buf_virt=%8.8x rx_buf_phys=%8.8x\n", device->rx_buf_virt[0], device->rx_buf_phys[0]);
	}	

	return B_OK;
}

static void free_ring_buffers(dev_info_t *device) {

		delete_area(device->tx_buf_area);
		delete_area(device->rx_buf_area);
}


/* Bits in RxConfig. */
enum rx_mode_bits {
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,
};

static void rx_init(dev_info_t *device) {

	int32 rx_mode=0;

	ETHER_DEBUG(WARN, device->debug, "rx_init prev config = %8.8x.\n", read32(device->base + RxConfig));

	if ( true || device->promiscuous) {
		ETHER_DEBUG(INFO,device->debug, "Promiscuous mode on.\n");
		rx_mode |= AcceptAllPhys;
	} 
	if (device->nmulti) {
		rx_mode |=  AcceptMulticast;
	} 

	rx_mode |= AcceptBroadcast | AcceptMulticast | AcceptMyPhys;

	/* We can safely update without stopping the chip. */
	write8(device->base + RxConfig, rx_mode);
	write32(device->base + MAR0 + 0, 0xffffffff);
	write32(device->base + MAR0 + 4, 0xffffffff);
	
	ETHER_DEBUG(WARN, device->debug, "rx_init exit config = %8.8x.\n", read32(device->base + RxConfig));

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


/* fetch the chips ethernet mac address */
static void get_mac_address(dev_info_t *device) {
	uchar j;
	uint16 eeword;
	
	for (j=0; j<3; j++) {
		uchar addr_len = 6;
			eeword = read_eeprom(device, j+7, addr_len);
			device->mac_address.ebyte[j*2] = eeword & 0xff;
			device->mac_address.ebyte[j*2+1] = eeword >> 8;
	}	

	dprintf(kDevName ": Ethernet address: ");
	for (j=0; j<6; j++) 	
		dprintf(" %x", device->mac_address.ebyte[j]);
	dprintf("\n");
}



/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   hit the alt sysreq keys, and type "rtl8139 {cmd} <cr>"
*/
static int
rtl8139(int argc, char **argv)
{
	uint16 i,j,found=0xFFFF;
	const char * usage = "usage: vt86ether [globals,stats,rx,tx,pci,functions,write]\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'g':
			kprintf("dump interesting globals here\n");
			break; 
		case 's':
			kprintf("dump stats\n");



			break; 
		case 'r':
			gdev->debug ^= RX;
			if (gdev->debug & RX) 
				kprintf("Recieve packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
			break; 
		case 't':
			gdev->debug ^= TX;
			if (gdev->debug & TX) 
				kprintf("Transmit packet trace Enabled\n");
			else 			
				kprintf("Transmit packet trace Disabled\n");
			break; 
		case 'w':
			gdev->debug ^= WARN;
			if (gdev->debug & WARN) 
				kprintf("Warnings ON\n");
			else 			
				kprintf("Warnings off\n");
			break; 
		case 'x':
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
	/* to do: fix tx indexing so we can use all 4 tx buffers, no TX_BUFFERS-1 */
	  
	if ((device->olock = create_sem(TX_BUFFERS-1, kDevName " tx")) < 0) {
		dprintf(kDevName " create tx sem failed %x \n", device->olock);
		return (device->olock);
	}
	set_sem_owner(device->olock, B_SYSTEM_TEAM);
	
	return (B_OK);
}

static void free_resources(dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->olock);
		dprintf("Free other sync stuff here\n");
}


/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x04	/* EEPROM shift clock. */
#define EE_CS			0x08	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x02	/* EEPROM chip data in. */
#define EE_WRITE_0		0x00
#define EE_WRITE_1		0x02
#define EE_DATA_READ	0x01	/* EEPROM chip data out. */
#define EE_ENB			(0x80 | EE_CS)

/* Delay between EEPROM clock transitions.
   No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
 */

#define eeprom_delay()	read32(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD	5
#define EE_READ_CMD		6
#define EE_ERASE_CMD	7

static int read_eeprom(dev_info_t *device, uint32 location, uint32 addr_len)
{
	int i;
	uint32 retval = 0;
	uint32 ee_addr = (uint32) device->base + Cfg9346;
	uint32 read_cmd = location | (EE_READ_CMD << addr_len);

	write8(ee_addr, EE_ENB & ~EE_CS);
	write8(ee_addr, EE_ENB);

	/* Shift the read command bits out. */
	for (i = 4 + addr_len; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		write8(ee_addr, EE_ENB | dataval);
		eeprom_delay();
		write8(ee_addr,EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}
	write8(ee_addr, EE_ENB);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		write8(ee_addr, EE_ENB | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((read8(ee_addr) & EE_DATA_READ) ? 1 : 0);
		write8(ee_addr, EE_ENB);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	write8(ee_addr, ~EE_CS);
	return retval;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
