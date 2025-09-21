/*
 * National Semiconductor DP83815 PCI 10/100 Ethernet driver
 * CopyRight (c) 2000 Be Inc., All Rights Reserved.
 * written by Hznk Sackett
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

/* PCI vendor and device ID's for DP83815 */
#define VENDOR_ID	0x100b		/* National Semiconductor */
#define DEVICE_ID	0x0020		/* dp83815 */

/* globals for driver instances */ 

#define kDevName "dp83815"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			
#define MAX_CARDS 			 4			/* maximum number of driver instances */


/*
 *  Choose "memory mapped" or "io port" pci bus access.
 */
#define IO_PORT_PCI_ACCESS false
#define MEMORY_MAPPED_PCI_ACCESS true

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
static int dp83815(int argc, char **argv);
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
/* diagnostic debug flags - compile in here or set while running with debugger "dp83815" command */
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO )
#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN )
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | SEQ)
//#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | RX | TX | INTERRUPT)


#define ETHER_DEBUG(flags, run_time_flags, args...) if (flags & run_time_flags) dprintf(kDevName ": " args)


/* ring buffer sizes - tune for optimal performance */
#define TINY_RING_BUFFERS_FOR_TESTING false
#if TINY_RING_BUFFERS_FOR_TESTING
#define TX_BUFFERS             4L 		/* must be a power of 2 */
#define RX_BUFFERS             4L		/* must be a power of 2 */
#else
#define TX_BUFFERS             16L		/* must be a power of 2 */
#define RX_BUFFERS             32L		/* must be a power of 2 */
#endif
#define BUFFER_SIZE            2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE         1514		/* 1514 + 4 bytes checksum added by card hardware */
#define ETH_CRC_LEN            4
#define MIN_FRAME_SIZE         60

#define TX_MASK               (TX_BUFFERS - 1)
#define RX_MASK               (RX_BUFFERS - 1)

static pci_module_info        *gPCIModInfo;
static char                   *gDevNameList[MAX_CARDS+1];
static pci_info               *gDevList[MAX_CARDS+1];
static int32                  gOpenMask = 0;


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
static int32 interrupt_hook(void *_device);


/* Hardware specific register & descriptors */

#define MAX_MULTI 32


enum chip_registers {

	DP_CR      =	0x00,		/* command register */
	DP_CFG     = 	0x04,		/* config & media status */
	DP_MEAR    =	0x08,		/* eeprom access */
	DP_PTSSCR  =	0x0c,		/* pci test control register */
	DP_ISR     =	0x10, 		/* interrupt status register */
	DP_IMR     =	0x14, 		/* interrupt mask register */
	DP_IER     =	0x18,		/* interrupt enable register */
	DP_TXDP    =	0x20,		/* transmit descriptor pointer */
	DP_TXCFG   =	0x24,		/* transmit configuration register */
	DP_RXDP    =	0x30,		/* receive descriptor pointer */
	DP_RXCFG   =	0x34,		/* receive configuration register */
	DP_CCSR    =	0x3C,		/* clock run and status register */
	DP_WCSR    =	0x40,		/* wake command status register */
	DP_PCR     =	0x44,		/* pause control regiter */
	DP_RFCR    =	0x48,		/* rx filter control register */
	DP_RFDR    =	0x4C,		/* rx filter data register */
	DP_BRAR    =	0x50,		/* Boot rom addrese */
	DP_BRDR    =	0x54,		/* Boot rom data address */
	DP_SRR     =	0x58, 		/* silicon revision */	
	DP_MIBC    =	0x5c,		/* managment info base control */
	DP_BMCR    =	0x80,		/* basic mode control register */
	DP_BMSR    =	0x84,		/* basic mode status register */
	DP_PHYIDR1 =	0x88, 		/* physyical ID reg 1 */
	DP_PHYIDR2 =	0x8c, 		/* physyical ID reg 2 */
	DP_DP_ANAR =	0x90,		/* auto negotiate register */
	DP_ANLPAR  =	0x94,		/* auto neg link partner */
	DP_PHYSTS  =	0xC0,		/* phy status register */
	DP_MICR    =	0xC4,		/* MII Interrupt Control reg */
	DP_MISR    =	0xC8,		/* MII Interrupt Status reg */
	DP_FCSCR   =	0xD0,		/* False carrier sense count */
	DP_RECR    =	0xD4,		/* rx error count reg */
	DP_PCSR    = 	0xD8,		/* PCS status reg */
	DP_PHYCR   =	0xE4,		/* PYY control reg */
	DP_TBTSCR  =	0xE8,		/* 10 BaseT status control reg */

};


/* Command Register Bits (DP_CR) */
enum dp_cr_bits {
	DP_CR_TXE   =	0x00000001,		/* Transmit Enable */
	DP_CR_TXD   =	0x00000002,		/* Transmit Disable */
	DP_CR_RXE   =	0x00000004,		/* Receiver Enable */
	DP_CR_RXD   =	0x00000008,		/* Receiver Disable */
	DP_CR_TXR   =	0x00000010,		/* Transmit Reset */
	DP_CR_RXR   =	0x00000020,		/* Receiver Reset */
	DP_CR_SWI   =	0x00000080,		/* Software Interrupt */
	DP_CR_RST   =	0x00000100,		/* Reset */
};


/* Interrupt Mask Register Bit Masks (DP_IMR) and ISR bits (DP_ISR) */
enum isr_bits {
	DP_RXOK   	=	0x00000001, /* Rx ok */
	DP_RXDESC 	=	0x00000002, /* Rx Descriptor */
	DP_RXERR  	=	0x00000004, /* Rx packet Error */
	DP_RXEARLY	=	0x00000008, /* Rx Early Threshold */
	DP_RXIDLE 	=	0x00000010, /* Rx Idle */
	DP_RXORN  	=	0x00000020, /* Rx Overrun */
	DP_TXOK   	=	0x00000040, /* Tx Packet Ok */
	DP_TXDESC 	=	0x00000080, /* Tx Descriptor */
	DP_TXERR  	=	0x00000100, /* Tx Packet Error */
	DP_TXIDLE 	=	0x00000200, /* Tx Idle */
	DP_TXURN  	=	0x00000400, /* Tx Underrun */
	DP_MIB    	=	0x00000800, /* MIB Service */
	DP_SWI    	=	0x00001000, /* Software Interrupt */
	DP_PME    	=	0x00002000, /* Power Management Event */
	DP_PHY    	=	0x00004000, /* Phy Interrupt */
	DP_HIERR  	=	0x00008000, /* High Bits error set */
	DP_RXSOVR 	=	0x00010000, /* Rx Status FIFO Overrun */
	DP_RTABT  	=	0x00100000, /* Recieved Target Abort */
	DP_RMABT  	=	0x00200000, /* Recieved Master Abort */
	DP_SSERR  	=	0x00400000, /* Signaled System Error */
	DP_DPERR  	=	0x00800000, /* Detected Parity Error */
	DP_RXRCMP 	=	0x01000000, /* Receive Reset Complete */
	DP_TXRCMP 	=	0x02000000, /* Transmit Reset Complete */
};

/* Interrupt Enable Register Bit Masks (DP_IER) */
enum chip_interrupt_enable_bits {
	DP_IER_IE    	=	0x00000001, /* Interrupt Enable */
	DP_IER_ID    	=	0x00000000, /* Interrupt Disable */

};

#if 0
/* tx & rx descriptor bit definitions */
enum cmdsts {
	DP_OWN     = 	0x80000000,
	DP_MORE    =	0x40000000,
	DP_INTR    =	0x20000000,
	DP_SUPCRC  =	0x10000000,
	DP_OK      =	0x08000000,
	DP_SIZE    =    0x00000FFF,
};
#endif
	

/* DP_DESC_CMDSTS - Descriptor Command and Status Definitions */
enum descriptor_cmdsts_bits {
	DP_DESC_CMDSTS_SIZE       	=	0x00000FFF, /* Size of data in bytes */
	DP_DESC_CMDSTS_TX_CCNT    	=	0x000F0000, /* Collision Count */
	DP_DESC_CMDSTS_TX_EC      	=	0x00100000, /* Excessive Collisions */
	DP_DESC_CMDSTS_TX_OWC     	=	0x00200000, /* Out of window collns */
	DP_DESC_CMDSTS_TX_ED      	=	0x00400000, /* Excessive deferrals */
	DP_DESC_CMDSTS_TX_TD      	=	0x00800000, /* Transmit deferrals */
	DP_DESC_CMDSTS_TX_CRS     	=	0x01000000, /* Carrier sense lost */
	DP_DESC_CMDSTS_TX_TFU     	=	0x02000000, /* Tx FIFO underrun */
	DP_DESC_CMDSTS_TX_TXA     	=	0x04000000, /* Tx abort */
	DP_DESC_CMDSTS_RX_COL     	=	0x00010000, /* Collision */
	DP_DESC_CMDSTS_RX_LBP     	=	0x00020000, /* Loopback packet */
	DP_DESC_CMDSTS_RX_FAE     	=	0x00040000, /* Frame align error */
	DP_DESC_CMDSTS_RX_CRCE    	=	0x00080000, /* CRC error */
	DP_DESC_CMDSTS_RX_ISE     	=	0x00100000, /* Invalid symbol error */
	DP_DESC_CMDSTS_RX_RUNT    	=	0x00200000, /* Runt packet */
	DP_DESC_CMDSTS_RX_LONG    	=	0x00400000, /* Long packet */
	DP_DESC_CMDSTS_RX_DEST    	=	0x01800000, /* Destination Class */
	DP_DESC_CMDSTS_RX_DEST_REJ	=	0x00000000, /*  Packet Rejected */
	DP_DESC_CMDSTS_RX_DEST_UNI	=	0x00800000, /*  Unicast packet */
	DP_DESC_CMDSTS_RX_DEST_MC 	=	0x01000000, /*  Multicast packet */
	DP_DESC_CMDSTS_RX_DEST_BC 	=	0x01800000, /*  Broadcast packet */
	DP_DESC_CMDSTS_RX_RXO     	=	0x02000000, /* Receive overrun */
	DP_DESC_CMDSTS_RX_RXA     	=	0x04000000, /* Receive aborted */
	DP_DESC_CMDSTS_OK         	=	0x08000000, /* Packet OK */
	DP_DESC_CMDSTS_TX_SUPCRC  	=	0x10000000, /* Supress CRC */
	DP_DESC_CMDSTS_RX_INCCRC  	=	0x10000000, /* Include CRC */
	DP_INTR                     =	0x20000000, /* Interrupt */
	DP_DESC_CMDSTS_MORE         =	0x40000000, /* More descriptors */
	DP_OWN                      =	0x80000000, /* Descr owner (consumer) */
};

#define DP_DESC_CMDSTS_TX_COLLISIONS_GET(cmdsts)								\
        (((cmdsts) & DP_DESC_CMDSTS_TX_CCNT) >> 16)

#define DP_DESC_CMDSTS_TX_ERRORS	(DP_DESC_CMDSTS_TX_CCNT |			\
                                     DP_DESC_CMDSTS_TX_EC   |			\
                                     DP_DESC_CMDSTS_TX_OWC  |			\
                                     DP_DESC_CMDSTS_TX_ED   |			\
                                     DP_DESC_CMDSTS_TX_CRS  |			\
                                     DP_DESC_CMDSTS_TX_TFU  |			\
                                     DP_DESC_CMDSTS_TX_TXA)				

#define DP_DESC_CMDSTS_RX_ERRORS   (DP_DESC_CMDSTS_RX_RXA	|	\
									DP_DESC_CMDSTS_RX_RXO	|	\
									DP_DESC_CMDSTS_RX_LONG	|	\
									DP_DESC_CMDSTS_RX_RUNT	|	\
									DP_DESC_CMDSTS_RX_CRCE	|	\
									DP_DESC_CMDSTS_RX_ISE	|	\
									DP_DESC_CMDSTS_RX_COL	|	\
									DP_DESC_CMDSTS_RX_FAE)



/* 
 * Recieve Filter/Match Control Register Bit Masks (DP_RFCR)
 *
 * It is used to control and configure the DP83815 Recieve Filter Control logic
 * The RFC logic is used to configure destination address filtering of incoming
 * packets.
 */

enum rx_filter_bits {
	DP_RFCR_RFADDR        	=	0x000003FF, /* Rx Filter Extended RegAdd */ 
	DP_RFCR_RFADDR_PMATCH1	=	0x00000000, /* Perfect Match octets 1-0 */
	DP_RFCR_RFADDR_PMATCH2	=	0x00000002, /* Perfect Match octets 3-2 */
	DP_RFCR_RFADDR_PMATCH3	=	0x00000004, /* Perfect Match octets 5-4 */ 
	DP_RFCR_RFADDR_PCOUNT1	=	0x00000006, /* Pattern Count 1-0 */
	DP_RFCR_RFADDR_PCOUNT2	=	0x00000008, /* Pattern Count 3-2 */
	DP_RFCR_RFADDR_SOPAS1 	=	0x0000000A, /* SecureOn Password 1-0 */
	DP_RFCR_RFADDR_SOPAS2 	=	0x0000000C, /* SecureOn Password 3-2 */
	DP_RFCR_RFADDR_SOPAS3 	=	0x0000000E, /* SecureOn Password 5-4 */
	DP_RFCR_RFADDR_FMEM_LO	=	0x00000200, /* Rx filter memory start */
	DP_RFCR_RFADDR_FMEM_HI	=	0x000003FE, /* Rx filter memory end */
	DP_RFCR_ULM           	=	0x00080000, /* U/L bit Mask */
	DP_RFCR_UHEN          	=	0x00100000, /* Unicast Hash Enable */
	DP_RFCR_MHEN          	=	0x00200000, /* Multicast Hash Enable */
	DP_RFCR_AARP          	=	0x00400000, /* Accept ARP Packets */
	DP_RFCR_APAT          	=	0x07800000, /* Accept On Pattern Match */
	DP_RFCR_APM           	=	0x08000000, /* Accept on Perfect match */
	DP_RFCR_AAU           	=	0x10000000, /* Accept All Unicast */
	DP_RFCR_AAM           	=	0x20000000, /* Accept All Multicast */
	DP_RFCR_AAB           	=	0x40000000, /* Accept All Broadcast */
	DP_RFCR_RFEN          	=	0x80000000, /* Rx Filter Enable */
};

typedef struct frame_descriptor {
volatile	uint32 link;
volatile	uint32 cmdsts;
volatile	uint32 bufptr;
} frame_desc_t;


//#include "fc_lock.c"
// ***
// Flow Control Lock
// ***

typedef struct fc_lock
{
	int32			count;
	spinlock		slock;
	int32			waiting;
	sem_id			sem;
} fc_lock;

status_t create_fc_lock( struct fc_lock *fc, int32 count, const char *name );
void delete_fc_lock( struct fc_lock *fc );
// Note: fc_wait() is not safe if can be called by multiple threads;
// Use a benaphore lock in this case.
status_t fc_wait( struct fc_lock *fc, bigtime_t timeout );
bool fc_signal( struct fc_lock *fc, int32 count, int32 sem_flags );

status_t create_fc_lock( struct fc_lock *fc, int32 count, const char *name )
{
	if( (fc->sem = create_sem( 0, name )) < 0 )
		return fc->sem;
	set_sem_owner( fc->sem, B_SYSTEM_TEAM );
	fc->count = count;
	fc->slock = 0;
	fc->waiting = 0;
	return B_OK;
}

void delete_fc_lock( struct fc_lock *fc )
{
	delete_sem( fc->sem );
}

status_t fc_wait( struct fc_lock *fc, bigtime_t timeout )
{
	cpu_status	cpu;
	
	// Lock while performing test & set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	
	// Test flow control condition and block if required 
	while( fc->count <= 0 )
	{
		status_t 	status;
		
		// Set waiting flag and release lock
		fc->waiting = 1;
		release_spinlock( &fc->slock );
		restore_interrupts( cpu );
		
		// Wait for signal
		if( (status = acquire_sem_etc( fc->sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, timeout )) != B_OK )
		{
			// Clear bits
			fc->waiting = 0;
			return status;
		}
		
		// Lock and test again
		cpu = disable_interrupts();
		acquire_spinlock( &fc->slock );
	}
	fc->count--; // Decrement count
	
	// Release lock and return B_OK because condition has been met
	release_spinlock( &fc->slock );
	restore_interrupts( cpu );
	return B_OK;
}

bool fc_signal( struct fc_lock *fc, int32 count, int32 sem_flags )
{
	cpu_status	cpu;
	bool		sig;
	
	// Lock while performing test and set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	
	fc->count += count; // Increment count
	
	// If someone is waiting and the condion is met, clear waiting flag and set sig flag
	if( fc->waiting )
	{
		fc->waiting = 0;
		sig = true;
	}
	else
		sig = false;
	
	release_spinlock( &fc->slock );
	restore_interrupts( cpu );
	
	// We do the actual signaling outside of the lock
	if( sig )
		release_sem_etc( fc->sem, 1, sem_flags );
		
	return sig;
}
//end #include fc_lock...

/* per driver instance globals */
typedef struct {
	int32			devID; 										/* device identifier: 0-n */
	pci_info		*pciInfo;
	uint16			irq;										/* our IRQ line */
	fc_lock			ilock, olock;								/* flow control:  input (rx) & output (tx) locks */
	int32			readLock, writeLock;						/* reentrant reads/writes not allowed */
	area_id			reg_area; 									/* Area used for memory mapped IO */
	area_id			tx_desc_area, tx_buf_area; 					/* transmit descriptor and buffer areas */
	area_id			rx_desc_area, rx_buf_area; 					/* receive descriptor and buffer areas */
	uchar			*tx_buf[TX_BUFFERS], *rx_buf[RX_BUFFERS];	/* tx and rx buffers */
	frame_desc_t	*tx_desc[TX_BUFFERS], *rx_desc[RX_BUFFERS];	/* tx and rx frame descriptors */
	int16			tx_wh, tx_isr;								/* tx ring buf indexes - write hook & isr */
	int16			rx_rh, rx_isr;								/* rx ring buf indexes - read_hook and interrupt service routine*/
	int16			rx_free, tx_sent;							/* number of free given to the card for processing */
	int16			rx_idle;									/* rx state machine state */
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




/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device,off_t pos,const void *buf,size_t *len);

static int32 get_pci_list(pci_info *info[], int32 maxEntries); 	/* Get pci_info for each device */
static status_t free_pci_list(pci_info *info[]);					/* Free storage used by pci_info list */
static status_t enable_addressing(dev_info_t *device); 			/* enable pci io address space for device */
static status_t init_ring_buffers(dev_info_t *device);				/* allocate and initialize frame buffer rings */
static void free_ring_buffers(dev_info_t *device);					/* allocate and initialize frame buffer rings */
static status_t allocate_resources(dev_info_t *device);			/* allocate semaphores & spinlocks */
static void  free_resources(dev_info_t *device);					/* deallocate semaphores & spinlocks */
static void get_mac_address(dev_info_t *device);					/* get ethernet address */
static status_t reset_device(dev_info_t *device);						/* reset the lan controller (NIC) hardware */
static void dump_rx_descriptors(dev_info_t * device);
static void dump_tx_descriptors(dev_info_t * device);
static status_t domulti( dev_info_t *device, char *addr );
static void phy_reset (dev_info_t * device);
static void setpromisc(dev_info_t * device, uint32 on);
static void set_station_address(dev_info_t * device);

static status_t rx_interrupt(dev_info_t *device, uint32 isr_reg);
static status_t tx_interrupt(dev_info_t *device);

/* eeprom access prototypes */
void DisplayEEPROM(dev_info_t *device);
void ReadMACAddressFromEEProm(dev_info_t *device, uint16 *nicAddress );
void WriteEEWord(dev_info_t *device, uint32 wordOffset, uint16 value );
int checkSumCheck( dev_info_t * device );
void ReadEEWord( dev_info_t * device, uint32 offset, uint16* pEeData );
void eeput(dev_info_t * device,  uint32 value, int nbits );
uint16 eeget(dev_info_t * device, int nbits );
void writeEeEnable(dev_info_t *device);
void writeEeDisable(dev_info_t * device);


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

	dprintf(kDevName ": find_device() %s\n", name);

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
	physical_entry		entry;
			
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

	/* Allocate storage for per driver instance global data */
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
	add_debugger_command (kDevName, dp83815, "Ethernet driver Info");
#endif

	if (allocate_resources(device) != B_OK) {
		goto err1;
	}	
	/* enable access to the cards address space */
	if ((status = enable_addressing(device)) != B_OK)
		goto err1;

	if ((status = reset_device(device)) != B_OK)	/* start from a known state */
		goto err1;

	/* Load Mac Address from EEPROM */
	get_mac_address(device);


	/* allocate and initialize frame buffer rings & descriptors */
	if (init_ring_buffers(device) != B_OK)
		goto err2;
	
	
	/* Install the Tx and Rx queues on the device */
	get_memory_map( (uint8 *)device->tx_desc[0], 4, &entry, 1 );
	write32(device->base + DP_TXDP, B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));
//dprintf("tx_desc=%8.8x pa=%8.8x\n", device->tx_desc[0],  B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));


	get_memory_map( (uint8 *)device->rx_desc[0], 4, &entry, 1 );
	write32(device->base + DP_RXDP, B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));
//dprintf("rx_desc=%8.8x pa=%8.8x\n", device->rx_desc[0],  B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address(entry.address)));

	
	
	// Setup interrupts
	install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, interrupt_hook, *cookie, 0 );
	

    /* reset the physical interface */
    phy_reset(device);

    {
		enum tx_config_settings {
			DP_TXCFG_DRTH         =	0x0000003F, /* Tx Drain Threshold */
			DP_TXCFG_FLTH         =	0x00003F00, /* Tx Fill Threshold */
			DP_MXDMA              =	0x00700000, /* Max DMA Burst Size */ 
			DP_MXDMA_1            =	0x00100000, /* 1 32-bit word */
			DP_MXDMA_2            =	0x00200000, /* 2 32-bit words */
			DP_MXDMA_4            =	0x00300000, /* 4 32-bit words */
			DP_MXDMA_8            =	0x00400000, /* 8 32-bit words */
			DP_MXDMA_16           =	0x00500000, /* 16 32-bit words */
			DP_MXDMA_32           =	0x00600000, /* 32 32-bit words */
			DP_MXDMA_64   		  =	0x00700000, /* 64 32-bit words */
			DP_MXDMA_128          =	0x00000000, /* 128 32-bit words */
			DP_TXCFG_IFG          =	0x0C000000, /* Interframe gap Time */ 
			DP_TXCFG_ATP          =	0x10000000, /* Automatic Transmit Pad */ 
			DP_TXCFG_MLB          =	0x20000000, /* MAC Loopback */
			DP_TXCFG_HBI          =	0x40000000, /* HeartBeat Ignore */
			DP_TXCFG_CSI          =	0x80000000, /* Carrier Sense Ignore */
		};
	
		enum tx_config_defaults {
			TX_DRAIN_THRESHOLD_DEFAULT = 0x30,
			TX_FILL_TRESHOLD_DEFAULT  = 0x100,
		};
		/* Recieve Configuration Register Bit Masks (DP_RXCFG) */
		enum rx_config_bits {
			DP_RXCFG_DRTH     	=	0x0000003E, /* Rx Drain Threshold */
			DP_RXCFG_ALP      	=	0x08000000, /* Accept Long Packets */
			DP_RXCFG_ATX      	=	0x10000000, /* Accept transmit packets */
			DP_RXCFG_ARP      	=	0x40000000, /* Accept Runt Packets */
			DP_RXCFG_AEP      	=	0x80000000, /* Accept Errored Packets */
		};
		
		enum rx_config_defaults {
				RX_DRAIN_THRESHOLD_DEFAULT = 0x8
		};
	
	    /* Setup transmit & receive control */
		if (device->duplex) {
			write32 (device->base + DP_TXCFG,
				DP_TXCFG_ATP | DP_MXDMA_32 | TX_FILL_TRESHOLD_DEFAULT | TX_DRAIN_THRESHOLD_DEFAULT |
			       DP_TXCFG_CSI | DP_TXCFG_HBI);
			write32(device->base + DP_RXCFG, RX_DRAIN_THRESHOLD_DEFAULT | DP_MXDMA_32 | DP_RXCFG_ATX);
		} else {
			write32 (device->base + DP_TXCFG,
				DP_TXCFG_ATP | DP_MXDMA_32 | TX_FILL_TRESHOLD_DEFAULT | TX_DRAIN_THRESHOLD_DEFAULT );
			write32(device->base + DP_RXCFG, RX_DRAIN_THRESHOLD_DEFAULT | DP_MXDMA_32);
		}
	}
    /* Setup the ethernet address */
    set_station_address (device);

    /* Receive perfect match and broadcast packets */
    write32(device->base + DP_RFCR, 0);
    write32(device->base + DP_RFCR, (DP_RFCR_AAB  | /* all broadcast pkts */
                              DP_RFCR_APM  | /* perfect match pkts */
                              DP_RFCR_RFEN));

                    
    /* Turn on device interrupts */
//    write32(device->base + DP_IMR, (DP_RXOK  | DP_MIB | DP_RTABT | DP_RMABT | DP_SSERR | DP_PHY));
    write32(device->base + DP_IMR, (DP_RXOK|DP_RXERR| DP_RXIDLE | DP_RXORN | DP_TXERR | DP_TXIDLE | DP_TXURN
		| DP_MIB | DP_PHY | DP_RTABT | DP_RMABT | DP_SSERR));

	write32(device->base + DP_IER, DP_IER_IE);	/* enable interrupts 1 */

    /* Enable Tx/Rx */
    write32 (device->base + DP_CR, DP_CR_TXE | DP_CR_RXE);


	dprintf(kDevName " open ok \n");	

	
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
	reset_device(device);
    write32 (device->base + DP_IMR, 0);
    write32 (device->base + DP_IER, 0);
    /* disable Tx/Rx */
    write32(device->base + DP_CR, DP_CR_TXD | DP_CR_RXD);
	

#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, dp83815);
#endif
	
	free_resources(device);
	
	/* Reset all the statistics*/
	ETHER_DEBUG(INFO, device->debug, "close\n");
	
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


		// Block until data is available
		if( (status = fc_wait( &device->ilock, device->blockFlag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT )) != B_NO_ERROR ) {
			*len = 0;
			return status;
		}

		// Protect againsts reentrant read
		if( atomic_or( &device->readLock, 1 ) ) {
			fc_signal( &device->ilock, 1, B_DO_NOT_RESCHEDULE );
			ETHER_DEBUG(ERR,device->debug, "reentrant read\n");
			*len = 0;
			return B_ERROR;
		}

		ETHER_DEBUG(FUNCTION, device->debug, "read_hook buf=%x len=%x\n", (char *)buf, *len);

		/* three cases, frame is good, bad, or we don't own the descriptor */
		frame_status = device->rx_desc[device->rx_rh]->cmdsts;
		
		if ((frame_status & DP_OWN == 0)) {		/*  card owns the buffer - this should never happen*/
			ETHER_DEBUG(ERR,device->debug,"RX buf owner err cmdsts=%x rx_rh=%x rx_isr=%x rx_free=%x\n",
				device->rx_desc[device->rx_rh]->cmdsts, device->rx_rh, device->rx_isr, device->rx_free);
//			dump_rx_descriptors(device);
			*len = 0;
			return B_ERROR;
		}
		/* err frame */
		if (frame_status & (DP_DESC_CMDSTS_RX_ERRORS)) {
			*len = 0;
			ETHER_DEBUG(WARN, device->debug, "Rx err %x\n", frame_status);
		} else {
			uint16 frame_size = (frame_status & DP_DESC_CMDSTS_SIZE) - ETH_CRC_LEN;
			if ((frame_size > MAX_FRAME_SIZE)|| (frame_size > *len )){ 
				ETHER_DEBUG(ERR, device->debug, "Rx Bad frame size %d \n", frame_size, *len);
				frame_size = min(frame_size, *len);
			}
			memcpy( buf, device->rx_buf[device->rx_rh], frame_size );
			if (device->debug & SEQ) 
			{ unsigned short  *seq = (unsigned short *) device->rx_buf[device->rx_rh];
				*len = frame_size;
				dprintf(" R%4.4x ", seq[20]);  /* sequence number */
			}
			 if (device->debug & RX) {
				dump_packet("rx: ",(unsigned char *)buf, 16);
//				dump_packet("rx: ",(unsigned char *)buf, frame_size);
			}
			
		}

		/* update indexes and buffer ownership */
		{	cpu_status  	former;
			uint8 was_idle = 0;
			former = disable_interrupts();
			acquire_spinlock( &device->rx_lock );
			device->rx_desc[device->rx_rh]->cmdsts = DP_INTR | (MAX_FRAME_SIZE + ETH_CRC_LEN);
			device->rx_rh = (device->rx_rh + 1) & RX_MASK;
			device->rx_free++;


			if (device->rx_idle) {
				uint32 cmd_reg = read32(device->base + DP_CR);   		 	
				device->rx_idle--;
				was_idle++;
				write32 (device->base + DP_CR, DP_CR_RXE | (cmd_reg & 0xff));
//				if (rx_idle) {
//					reset_rx(device);
//				} else {
//					write32 (device->base + DP_CR, DP_CR_RXE | (cmd_reg & 0xff));
//				}
			}
			release_spinlock( &device->rx_lock );
    		restore_interrupts(former);

			if (was_idle) {
				was_idle--;
				ETHER_DEBUG(WARN, device->debug, "rh UNidle %x\n", was_idle);
//				dump_rx_descriptors(device);
			}
			if (device->rx_free > RX_BUFFERS) dprintf("RX FREE=%d out of range\n");
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
	
	if( (status = fc_wait( &device->olock, B_INFINITE_TIMEOUT )) != B_NO_ERROR ) {
		*len = 0;
		return status;
	}

	// Protect against reentrant write
	if( atomic_or( &device->writeLock, 1 ) ) {
		fc_signal( &device->olock, 1, B_DO_NOT_RESCHEDULE );
		*len = 0;
		ETHER_DEBUG(ERR, device->debug, "reentrant write\n");
		return B_ERROR;
	}


	if (device->tx_desc[device->tx_wh]->cmdsts & DP_OWN) {		/* card owns!? */
		ETHER_DEBUG(ERR, device->debug, "ERR write_hook: card owns buf[%d]\n",device->tx_wh);
		device->writeLock = 0;
		return B_ERROR;
	}

	/* Copy data to tx buffer */
	memcpy( device->tx_buf[device->tx_wh], buf, frame_size );
	{	cpu_status  	former;
		former = disable_interrupts();
		acquire_spinlock( &device->tx_lock );
		device->tx_desc[device->tx_wh]->cmdsts = DP_OWN | frame_size;		/* endian swap for PPC */ /* give buf to card */
		device->tx_wh = (device->tx_wh + 1) & TX_MASK;
		device->tx_sent++;
		release_spinlock( &device->tx_lock );
   		restore_interrupts(former);
	}	
	/* in case tx unit is idle? */
    write32(device->base + DP_CR, DP_CR_TXE);
	
	/* Another write may now take place */
	device->writeLock = 0;
	return B_OK;

}

static status_t rx_interrupt(dev_info_t *device, uint32 isr_reg) {

	uint16 rxcount = 0;
	int16 limit;

	/* scan the ring for new frames */
	acquire_spinlock( &device->rx_lock);
	for ( limit=device->rx_free; limit > 0; limit--) {
		if ((device->rx_desc[device->rx_isr]->cmdsts & DP_OWN) == 0) {
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
	}
	release_spinlock(&device->rx_lock);
	
	if (device->rx_free < 0)
		ETHER_DEBUG(ERR, device->debug, "ISR rx_free=%d out of range\n", device->rx_free);

    /*receiver will sometimes quietly stall, remind the chip its on! */
	write32 (device->base + DP_CR, DP_CR_RXE);

	if (rxcount) { /* signal data has arrived */
		if( fc_signal( &device->ilock, rxcount, B_DO_NOT_RESCHEDULE ) )
			return B_INVOKE_SCHEDULER;
		else
			return B_HANDLED_INTERRUPT; // B_HANDLED_INTERRUPT
	} else { /* got extra interrupts */
		;
//		ETHER_DEBUG(WARN, device->debug, "rx_int: sync rh=%3.3d isr=%3.3d free=%3.3d isr=%8.8x cmdsts=%8.8x\n",
//			device->rx_rh, device->rx_isr, device->rx_free, isr_reg,
//			device->rx_desc[(device->rx_isr -1) & RX_MASK]->cmdsts );
//		dump_rx_descriptors(device);
	}

	return B_HANDLED_INTERRUPT;
}

static status_t tx_interrupt(dev_info_t *device) {
		int32 tx_count = 0;
		uint16 tx_status;
		int16 limit;

		acquire_spinlock( &device->tx_lock);
		for (limit = device->tx_sent; limit > 0; limit--) {
			if (device->tx_desc[device->tx_isr]->cmdsts & DP_OWN) {
#if 0			/* device generates extra interrupts */
				ETHER_DEBUG(ERR, device->debug, "ISR_TX owns err wh=%x isr=%x sent=%x limit=%x\n",
					device->tx_wh, device->tx_isr, device->tx_sent, limit);
				dump_tx_descriptors(device);
#endif
				break;
			}
			tx_status = device->tx_desc[device->tx_isr]->cmdsts;
			if (tx_status  & DP_DESC_CMDSTS_TX_ERRORS) {
				dprintf("tx err %x\n", tx_status);
#if 0
				
				if (cmdsts & DP_DESC_CMDSTS_TX_TXA) /* tx aborted */
					stats_p->tx_packets--;
				
				if (cmdsts & DP_DESC_CMDSTS_TX_TFU) /* fifo errors */
					stats_p->tx_fifo_errors++;
	
				if (cmdsts & DP_DESC_CMDSTS_TX_CRS) /* lost carrier */
					stats_p->tx_carrier_errors++;
	
				if (cmdsts & DP_DESC_CMDSTS_TX_OWC) /* out of window collisions */
					stats_p->tx_window_errors++;

#endif
			}	else {	/* normal tx */
				device->tx_desc[device->tx_isr]->cmdsts = 0;
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
			if( fc_signal( &device->olock, tx_count, B_DO_NOT_RESCHEDULE ) )
				return B_INVOKE_SCHEDULER;
			else
				return B_HANDLED_INTERRUPT;
		}
		
		return B_HANDLED_INTERRUPT;
}

/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	dev_info_t *device = (dev_info_t *) _device;
	unsigned char isr;
	int32 status;
	int32 handled = B_UNHANDLED_INTERRUPT;


	/* read (which clears)  interrupt indicators */
    status=read32(device->base + DP_ISR);

	ETHER_DEBUG(INTERRUPT,device->debug, "ISR %x\n", status);
	
	if (status == 0) {		/* shared interrupts go to next devices ISR */
		handled =  B_UNHANDLED_INTERRUPT;
	} else {
		handled = B_INVOKE_SCHEDULER;
		
		if (status & DP_RXIDLE) {		/* all the read buffers are full */
			device->rx_idle++;
		}
		if (status & ( DP_RXOK | DP_RXDESC )) {
			handled = rx_interrupt(device, status);
		}
		if (status & (DP_TXOK)) {  
			handled = tx_interrupt(device);	
		}
		
		if (status & ~(DP_TXOK|DP_RXOK|DP_RXIDLE|DP_RXDESC|DP_RXEARLY|DP_TXDESC|DP_TXIDLE| 0x000E0000))  /* unusual interrupt ? */
			ETHER_DEBUG(WARN, device->debug, "ISR %x\n", status);

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
		if ((item->vendor_id == VENDOR_ID)&& (item->device_id == DEVICE_ID)) {
			/* check if the device really has an IRQ */
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf(kDevName " found with invalid IRQ - check BIOS");
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
//	ETHER_DEBUG(INFO,  device->debug, "PCI base=%x size=%x\n", base, size);

	// Round down to nearest page boundary
	base = base & ~(B_PAGE_SIZE-1);
	
	// Adjust the size
	offset = device->pciInfo->u.h0.base_registers[MemoryReg] - base;
	size += offset;
	size = (size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

	ETHER_DEBUG(PCI_IO,  device->debug, "Now PCI base=%x size=%x offset=%x\n", base, size, offset);
		
	if ((device->reg_area = map_physical_memory(kDevName " Regs", (void *)base, size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, (void *)&device->base)) < 0)
		return B_ERROR;

	device->base = device->base + offset;  //  /sizeof (int32) ??
	}
#endif /*MEMORY_MAPPED_PCI_ACCESS */
	
	ETHER_DEBUG(INFO,  device->debug, "base=%x\n", device->base);

	return B_OK;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t init_ring_buffers(dev_info_t *device)
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
			device->tx_desc[i]->bufptr = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			get_memory_map( (uint8 *)device->tx_desc[(i+1) & TX_MASK], 4, &entry, 1 );
			device->tx_desc[i]->link = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			device->tx_desc[i]->cmdsts = DP_INTR;  /* generate an interrupt on for each tx - to do: tune for 100 MB */
		}
		if (device->debug & TX) {
			for ( i = 0; i < TX_BUFFERS; i++) {
				dprintf("tx_desc[%3.3d]=%8.8x link=%8.8x cmdsts=%8.8x buf=%8.8x \n", i,device->tx_desc[i],
					device->tx_desc[i]->link, device->tx_desc[i]->cmdsts, device->tx_desc[i]->bufptr);
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
			device->rx_desc[i]->bufptr = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			get_memory_map( (uint8 *)device->rx_desc[(i+1) & RX_MASK], 4, &entry, 1 );
			device->rx_desc[i]->link = B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
			device->rx_desc[i]->cmdsts = DP_INTR | (MAX_FRAME_SIZE + ETH_CRC_LEN); /* chip owns + gen intr on rx + max_size_frame */
		}

		if (device->debug & RX) {
			dump_rx_descriptors(device);
			snooze(200000); // let dprintfs finish
		}	
	}

	return B_OK;
}

static void free_ring_buffers(dev_info_t *device) {

		delete_area(device->tx_buf_area);
		delete_area(device->tx_desc_area);
		delete_area(device->rx_buf_area);
		delete_area(device->rx_desc_area);
}


/*
 * Allocate and initialize semaphores and spinlocks.
 */
static status_t allocate_resources(dev_info_t *device) {
	status_t result;	
	/* Setup Semaphores */
	if((result = create_fc_lock( &device->ilock, 0, kDevName " rx" )) < 0 ) {
		dprintf(kDevName " create rx fc_lock failed %x \n", result);
		return (result);
	}
	
	/* intialize tx semaphore with the number of free tx buffers - 1
	 wasting a buffer means the tx_idle condition never happens
	 which simplifies the code */
	if ((result = create_fc_lock( &device->olock, TX_BUFFERS-1, kDevName " tx" )) < 0 ) {
		delete_fc_lock(&device->ilock);
		dprintf(kDevName " create tx sem failed %x \n", result);
		return (result);
	}

	device->tx_lock = device->rx_lock = 0;
	
	return (B_OK);
}

static void free_resources(dev_info_t *device) {
		delete_fc_lock(&device->ilock);
		delete_fc_lock(&device->olock);
}



/* get_mac_address() - get the ethernet address */
static void get_mac_address(dev_info_t *device) {

	uint16 j;

	dprintf(kDevName ": Ethernet address ");

	ReadMACAddressFromEEProm(device, (uint16 *) &(device->mac_address));

	for (j=0;j<6;j++)
		dprintf("%2.2x%c", device->mac_address.ebyte[j],j==5?'\n':':');
}

/* dp83815_mac_address_set - set the ethernet address */
static void
set_station_address (dev_info_t * device)
{
    int i;
	uint16 * mac_addr_ptr;

    for (i=0, mac_addr_ptr = (uint16 *)&(device->mac_address); i<3; i++, mac_addr_ptr++) {
        write32 (device->base + DP_RFCR,  i*2);				/* select address */
        write32 (device->base + DP_RFDR, *mac_addr_ptr);	/* write value */
    }
}

/* dp83815_dev_reset - soft reset DP83815 */
static status_t reset_device(dev_info_t *device)
{
    int  timeout = 50;
    uint32 isr_val;
	
	device->rx_free = RX_BUFFERS;

    write32 (device->base + DP_CR, DP_CR_RST);
    while (timeout--) {
		isr_val = read32(device->base + DP_ISR);
        if (isr_val == 0x03008000) {
            return (B_OK);
        }
        snooze(10);
    }

	ETHER_DEBUG(ERR, device->debug, "soft reset failed %x\n", isr_val);
	return B_ERROR;
}



/* set hardware so that all packets are received. */
static void setpromisc(dev_info_t * device, uint32 on) {
	
	uint32 rx_filter = read32(device->base + DP_RFCR);

	if (on) {	
		write32(device->base + DP_RFCR, rx_filter |
			DP_RFCR_RFEN | DP_RFCR_AAB | DP_RFCR_AAM | DP_RFCR_AAU | DP_RFCR_AARP);
	} else {
		write32(device->base + DP_RFCR, rx_filter & ~DP_RFCR_AAU );
	}
}

static status_t
domulti( dev_info_t *device, char *addr )
{

	uint32 rx_filter = read32(device->base + DP_RFCR);
	// to do progam hash filters - for now accept all multicast
	write32(device->base + DP_RFCR, rx_filter | DP_RFCR_AAM );

	return (B_NO_ERROR);
}



/* Physical Configuration and Media Status Register Bit Masks (DP_CFG) */
enum phy_bits {
	DP_CFG_BEM            	=	0x00000001, /* Big Endian Mode (BM xfers) */
	DP_CFG_BROM_DIS       	=	0x00000004, /* Disable Boot ROM interface */
	DP_CFG_PESEL          	=	0x00000008, /* Parity Err Det (BM xfer) */
	DP_CFG_EXD            	=	0x00000010, /* Excessv Deferl Tmr disbl */ 
	DP_CFG_POW            	=	0x00000020, /* Prog Out of Window Timer */
	DP_CFG_SB             	=	0x00000040, /* Single Back-off */
	DP_CFG_REQALG         	=	0x00000080, /* PCI Bus Request Algorithm */
	DP_CFG_EUPHCOMP       	=	0x00000100, /* DP83810 Descriptor Compat */ 
	DP_CFG_PHY_DIS        	=	0x00000200, /* Disable internal Phy */
	DP_CFG_PHY_RST        	=	0x00000400, /* Reset internal Phy */
	DP_CFG_ANEG_SEL       	=	0x0000E000, /* Auto-nego Sel - Mask */
	DP_CFG_ANEG_SEL_10_HD 	=	0x00000000, /*  Force 10Mb Half duplex */
	DP_CFG_ANEG_SEL_100_HD	=	0x00004000, /*  Force 100Mb Half duplex */
	DP_CFG_ANEG_SEL_10_FD 	=	0x00008000, /*  Force 10Mb Full duplex */
	DP_CFG_ANEG_SEL_100_FD	=	0x0000C000, /*  Force 100Mb Full duplex */
	DP_CFG_ANEG_SEL_10_XD 	=	0x00002000, /*  Nego 10Mb Half/Full dplx */
	DP_CFG_ANEG_SEL_ALL_HD	=	0x00006000, /*  Nego 10/100 Half duplex */
	DP_CFG_ANEG_SEL_100_XD	=	0x0000A000, /*  Nego 100 Half/Full duplex */
	DP_CFG_ANEG_SEL_ALL_XD	=	0x0000E000, /*  Nego 10/100 Half/Full dplx*/
	DP_CFG_PAUSE_ADV      	=	0x00010000, /* Strap for pause capable */
	DP_CFG_PINT_ACEN      	=	0x00020000, /* Phy Intr Auto Clr Enable */ 
	DP_CFG_PHY_CFG        	=	0x00FC0000, /* Phy Configuration */
	DP_CFG_ANEG_DN        	=	0x08000000, /* Auto-negotiation Done */
	DP_CFG_POL            	=	0x10000000, /* 10Mb Polarity Indication */
	DP_CFG_FDUP           	=	0x20000000, /* Full Duplex */
	DP_CFG_SPEED100       	=	0x40000000, /* Speed 100Mb */
	DP_CFG_LNKSTS         	=	0x80000000, /* Link status */
};

/* New Phy registers and their bit mask values for Rev 3 */    
enum phy_registers {
	DP_PHY_PAGE=0xCC,	DP_PHY_DSPCFG=0xF4,
	DP_PHY_SDCFG=0xF8,	DP_PHY_TDATA=0xFC,
	DP_PHY_PAGE_VAL=0x0001, 		/* */
	DP_PHY_DSPCFG_VAL=0x5040,		/* Load/Kill C2 */
	DP_PHY_SDCFG_VAL=0x008C,		/* Raise SD off, from 4 to C */
	DP_PHY_TDATA_VAL=0x0000,		/* Set value for C2 */
	DP_PHYCR_PMDCSR_VAL=0x189C, 	/* DC Speed = 01 */
};


/* PHY Status Register */
enum phy_status_bits {
	DP_PHYSTS_LNK_VALID  	=	0x0001, /* Valid Link */
	DP_PHYSTS_SPEED_10   	=	0x0002, /* 10 Mbps Mode */
	DP_PHYSTS_FDX        	=	0x0004, /* Full Duplex Mode */
	DP_PHYSTS_LOOP       	=	0x0008, /* Loopback Enabled */
	DP_PHYSTS_ANEG_DONE  	=	0x0010, /* Auto-Neg Complete */
	DP_PHYSTS_JABBER     	=	0x0020, /* Jabbler Detected */
	DP_PHYSTS_REM_FAULT  	=	0x0040, /* Remote Fault Detected */
	DP_PHYSTS_MII_INTR   	=	0x0080, /* MII Interrupt Pending */
	DP_PHYSTS_LCWP_RX    	=	0x0100, /* Link Code Word Page Rx'd */
	DP_PHYSTS_DSCRMBL_LCK	=	0x0200, /* 100TX Descrambler Lock */
	DP_PHYSTS_SIG_DET    	=	0x0400, /* 100TX Uncond Signal Detect */
	DP_PHYSTS_FCSL       	=	0x0800, /* False Carrier Sense Latch */
	DP_PHYSTS_POL_INV    	=	0x1000, /* Polarity status */
	DP_PHYSTS_RX_ERR_LATCH	=	0x2000, /* Received error latch */
};




/* reset the physical interface */
static void phy_reset (dev_info_t * device)
{
    uint32  dp_cfg_val;
    uint16  phy_status;
    uint16  timeout;

    dp_cfg_val =  (DP_CFG_PESEL           |     /* parity error detect */
                   DP_CFG_ANEG_SEL_ALL_XD |     /* negotiate 10/100 full/half */
                   DP_CFG_PAUSE_ADV       |     /* pause capable */
                   DP_CFG_PINT_ACEN       |     /* phy intr auto clear */
                   0x00040000);                 /* phy config */

    write32(device->base + DP_CFG, dp_cfg_val | DP_CFG_PHY_RST);
    snooze (500);

    write32(device->base + DP_CFG, dp_cfg_val);
    for (timeout=1000; timeout; timeout--) {
        uint32 dp_config = read32(device->base + DP_CFG);
		if (dp_config & DP_CFG_ANEG_DN) break;
        snooze (5000);
    }
    if (timeout == 0) {
		dprintf(kDevName " autonegotiate failed \n");
    } else 

    write32 (device->base + DP_PHY_PAGE, DP_PHY_PAGE_VAL);
    write32 (device->base + DP_PHYCR, DP_PHYCR_PMDCSR_VAL);
    write32 (device->base + DP_PHY_TDATA, DP_PHY_TDATA_VAL);
    write32 (device->base + DP_PHY_DSPCFG, DP_PHY_DSPCFG_VAL);
    write32 (device->base + DP_PHY_SDCFG, DP_PHY_SDCFG_VAL);
    write32 (device->base + DP_PHY_PAGE, (uint16) 0x0000);

                        
    phy_status = read16(device->base + DP_PHYSTS);
 	
	device->duplex = (phy_status & DP_PHYSTS_FDX) ? true : false;

	dprintf (kDevName " speed=%d duplex=%s link=%s after %d uS\n",
            (phy_status & DP_PHYSTS_SPEED_10) ? 10 : 100,
            (device->duplex) ? "full" : "half",
            (phy_status & DP_PHYSTS_LNK_VALID) ? "up" : "down",
			(1000-timeout) * 5000);

}


/***********************************************************************
*									
* EEPROM.C                                                     
*									
* This is a collection of routines to interface to the EEPROM
*  for the MacPhyter Chip
*
***********************************************************************/



#define EEread          0x0180          /* .. read */

// Bit Definitions for MEAR:
#define 	EECS    		0x8	
#define 	EECLK    		0x4	
#define 	EEDO    		0x2
#define 	EEDI    		0x1

// PCI TEST CNTRL Register:
#define 	RBIST_RST		0x400		
#define 	RBIST_CLKD		0x200		
#define 	RBIST_MODE		0x100		
#define 	RBIST_EN		0x80		
#define 	RBIST_ACT		0x40		
#define 	RBIST_RXFAIL	0x20		
#define 	RBIST_TXFAIL	0x10		
#define 	RBIST_RXFFAIL	0x8		
#define 	EELOAD_EN		0x4		
#define 	EEBIST_EN		0x2		
#define 	EEBIST_FAIL		0x1


void DisplayEEPROM(dev_info_t * device)
{
int i=0;
uint16 value;

	for( i=0; i<0xC; i++ ) {
		ReadEEWord( device, i, &value );
		dprintf("\n%01X= %04X  ", i, value);
	}
	dprintf( "\n\n" );
}


void ReadMACAddressFromEEProm(dev_info_t *device, uint16 *nicAddress )
{
uint32 i;
uint16 mask;
uint16 word1 = 0;
uint16 word2 = 0;
uint16 word3 = 0;
uint16 word4 = 0;

    //
    // Read 16 bit words 6 - 9 from the EEProm.  They contain the hardwares MAC
    // address in a rather cryptic format.
    //
    ReadEEWord( device, 0x06, &word1 );
    ReadEEWord( device, 0x07, &word2 );
    ReadEEWord( device, 0x08, &word3 );
    ReadEEWord( device, 0x09, &word4 );

    //
    // Decode the cryptic format into what we can use a word at a time.
    //
    nicAddress[ 0 ] = word1 & 1;
    nicAddress[ 1 ] = word2 & 1;
    nicAddress[ 2 ] = word3 & 1;

    i = 15;
    mask = 0x2;
    while ( i-- ) {
       if ( word2 & 0x8000 ) {
          nicAddress[ 0 ] |= mask;
       }
       word2 = word2 << 1;
       mask = mask << 1;
    }

    i = 15;
    mask = 0x2;
    while ( i-- ) {
       if ( word3 & 0x8000 ) {
          nicAddress[ 1 ] |= mask;
       }
       word3 = word3 << 1;
       mask = mask << 1;
    }

    i = 15;
    mask = 0x2;
    while ( i-- ) {
       if ( word4 & 0x8000 ) {
          nicAddress[ 2 ] |= mask;
       }
       word4 = word4 << 1;
       mask = mask << 1;
    }
 }

#if 0
/************************************************
*       Function:   WriteMACAddress
*       Purpose:    Write readable form of the MAC address to EEPROM
*	Returns:    void
*************************************************/
void writeMacAddress( unsigned *nicAddress )
{
unsigned char csLow, csHigh, csSum, csVals[ 11 ];
uint32      mask;
uint32      l;
int         nbits = 9;
unsigned    i, value, sixOff, nineOff, checkSum = 0, word1, word2, word3;
unsigned    sword1, sword2, sword3, sword4;


    ReadEEWord( RegBase + DP_MEAR, 0x06, &sixOff );
	sixOff &= ~1;

    ReadEEWord( RegBase + DP_MEAR, 0x09, &nineOff );
	nineOff &= 1;

    word1 = nicAddress[ 0 ];
    word2 = nicAddress[ 1 ];
    word3 = nicAddress[ 2 ];
    
    writeEeEnable();
	
	value = sixOff;
	if( word1 & 1 ) value |= 1;
    sword1 = value;
    WriteEEWord( RegBase + DP_MEAR, 0x06, value );
    
	value = 0;
    l = 15;
    mask = 0x2;
	while( l-- )
		{
		if( word1 & 0x8000 ) value |= mask;
		mask <<= 1;
		word1 <<= 1;
    	}
    
    if( word2 & 1 ) value |= 1;
    sword2 = value;
    WriteEEWord( RegBase + DP_MEAR, 0x07, value );

	value = 0;
    l = 15;
    mask = 0x2;
	while( l-- )
		{
		if( word2 & 0x8000 ) value |= mask;
		mask <<= 1;
		word2 <<= 1;
    	}
    
    if( word3 & 1 ) value |= 1;
    sword3 = value;
    WriteEEWord( RegBase + DP_MEAR, 0x08, value );

    value = nineOff;
    l = 15;
    mask = 0x2;
	while( l-- )
		{
		if( word3 & 0x8000 ) value |= mask;
		mask <<= 1;
		word3 <<= 1;
    	}
    
    sword4 = value;
    WriteEEWord( RegBase + DP_MEAR, 0x09, value );
    
    writeEeDisable();
	
    // Calculate Checksums:
    for( l=0; l<11; l++ )
    {
    	ReadEEWord( RegBase + DP_MEAR, l, &value );
    	if( (l >= 6) || (l <= 9) )
    		{
    	    switch( l )
    	    	{
    	        case 6:
    	        	if( value != sword1 )
    	        		printf( "\nEEPROM programming failed!  Enter Alternate MAC address.\n" );
    	        	break;

    	        case 7:
    	        	if( value != sword2 )
    	        		printf( "\nEEPROM programming failed!  Enter Alternate MAC address.\n" );
    	        	break;

    	        case 8:
    	        	if( value != sword3 )
    	        		printf( "\nEEPROM programming failed!  Enter Alternate MAC address.\n" );
    	        	break;

    	        case 9:
    	        	if( value != sword4 )
    	        		printf( "\nEEPROM programming failed!  Enter Alternate MAC address.\n" );
    	        	break;
    	    	}
    	    }
    	    
    	csLow = value & 0xff;
		value >>= 8;
    	csHigh = value & 0xff;
    	
    	csVals[ l ] = csLow + csHigh;
    }
    
    // Calculate Checksums:
    csSum =  0;
    for( l=0; l<11; l++ )
    	{
        csSum += csVals[ l ];
        }
	
    csSum += 0x55;
    checkSum = (~csSum) + 1;
    checkSum = ( checkSum << 8 ) + 0x55;

    delay( 10 );

    writeEeEnable();

    WriteEEWord( RegBase + DP_MEAR, 0x0B, checkSum );

    writeEeDisable();
	
    ReadEEWord( RegBase + DP_MEAR, 0xB, &value );
   	
    if( value != checkSum ) 
       {
       printf( "\n!!WARNING Checksum not programmed correctly!\n" );
       printf( "\n  checkSUM: %04X  actual: %04X", checkSum, value );
       }
        
    checkSumCheck(  );
}
#endif

/************************************************
*       Function:   WriteEEWord
*       Purpose:    write a 16-bit value into the specified eeprom location
*	Returns:    void
*************************************************/
void WriteEEWord(dev_info_t * device, uint32 wordOffset, uint16 value )
{
uint32      mask;
uint32      l;
int			nbits = 9;
uint32 invalue = (uint32) value;

    write32( device->base + DP_MEAR, 0 );           /* clock out CS low */
    snooze( 5000 );
 
    write32( device->base + DP_MEAR, EECLK );
    snooze( 5000 );

    invalue = ((uint32)( 0x0140|wordOffset) << 16 ) | invalue;
    
    nbits = 25;

    mask = ((uint32)1) << (nbits-1);

    while( nbits-- ) {
        /* assert chip select, and setup data in */
        l = ( invalue & mask ) ? EECS | EEDI : EECS;

        write32( device->base + DP_MEAR, l );
	    snooze( 5000 );
        
        write32( device->base + DP_MEAR, l | EECLK );
        snooze( 5000 );
    
        mask >>= 1;
    }

    // IOW32( mear, 0 );           /* terminate write  */
    write32( device->base + DP_MEAR, 0 );               // terminate operation
    snooze( 5000 );
 
    /* wait for operation to complete */
    write32( device->base + DP_MEAR, EECS );        /* assert CS */
    snooze( 5000 );
 
    write32( device->base + DP_MEAR, EECS | EECLK );
    snooze( 5000 );
 
    for( l = 0; l < 10; l++ ) {
        snooze( 5000 ); {
    	int32 mear_reg = read32(device->base+ DP_MEAR);    
		if( mear_reg & EEDO )
            break;
   		}
	 }
    
    write32( device->base + DP_MEAR, 0 );           /* clock out CS low */
    snooze( 5000 );
 
    write32( device->base + DP_MEAR, EECLK );
    snooze( 5000 );

}

/************************************************
*       Function:   checkSumCheck
*       Purpose:    Use the EEBIST bit on MacPhyter to
*                   validate the EEPROM checksum
*************************************************/
int checkSumCheck( dev_info_t * device )
{
unsigned long pciTestReg = EEBIST_EN;
	
	write32( device->base + DP_PTSSCR, EEBIST_EN );
	
	while( pciTestReg & EEBIST_EN )
	{
	pciTestReg = read32( device->base + DP_PTSSCR );
	}
	{
		uint32 pcitest_reg = read32(device->base + DP_PTSSCR);
		if( pcitest_reg & EEBIST_FAIL )
		{
			dprintf( "\n!!WARNING:  Checksum FAILED!\n" );
			return( 1 );
		}
			else dprintf( "\nEEBIST Test Passes!\n" );
		}	
	return( 0 );
}

void ReadEEWord( dev_info_t * device, uint32 offset, uint16* pEeData )
{
uint16      value;

    write32( device->base + DP_MEAR, 0 );
    snooze( 5000 );
 
    write32( device->base + DP_MEAR, EECLK );       // clock out no chipselect
    snooze( 5000 );
 
    
    eeput(device, (uint32)(EEread|offset), 9 );
    
    value = eeget(device, 16 );
    
    write32( device->base + DP_MEAR, 0 );           // terminate read 
    snooze( 5000 );
 
    write32( device->base + DP_MEAR, EECLK );       // clock out no chipselect
    snooze( 5000 );
 
    *pEeData = ( uint16 )value;

    write32( device->base + DP_MEAR, 0 );           // terminate read 
    snooze( 5000 );
 
//    return( value );
}


/************************************************
*       Function:   EEPUT
*       Purpose:    Clock OUT data to EEPROM
*************************************************/
void eeput(dev_info_t * device,  uint32 value, int nbits )
{
uint32      mask = ((uint32)1) << (nbits-1);
uint32      l;

    while( nbits-- ) {
        // assert chip select, and setup data in
        l = ( value & mask ) ? EECS | EEDI : EECS;
        write32( device->base + DP_MEAR, l );
        snooze( 5000 );
 
        write32( device->base + DP_MEAR, l | EECLK );
        snooze( 5000 );
 
        mask >>= 1;
    }
}

/************************************************
*       Function:   EEGET
*       Purpose:    Clock IN data from EEPROM
*************************************************/
uint16 eeget(dev_info_t * device, int nbits )
{
uint16      mask = 1 << (nbits-1);
uint16      value = 0;

    while( nbits-- ) {
        // assert chip select, and clock in data
        write32( device->base + DP_MEAR, EECS );
        snooze( 5000 );
 
        write32( device->base + DP_MEAR, EECS | EECLK );
	    snooze( 5000 );
 
		{
    		uint32 mear_reg = read32(device->base + DP_MEAR);
		    if( mear_reg & EEDO )
                value |= mask;
	    }
		snooze( 5000 );
    
        mask >>= 1;
    }
    
    return( value );
}

/************************************************
*       Function:   WriteEeEnable
*       Purpose:    ENABLE EEPROM Write Mode
*************************************************/
void writeEeEnable(dev_info_t *device)
{
uint32      mask;
uint32      l;
int			nbits = 9;
    //
    // Start the sequence with Chip Select 0 for a 4 microsecond cycle.
    //
    write32( device->base + DP_MEAR, 0 );
    snooze( 5000 );

    write32( device->base + DP_MEAR, EECLK );
    snooze( 5000 );

    mask = ((uint32)1) << (nbits-1);

    while( nbits-- ) {
        /* assert chip select, and setup data in */
        l = ( 0x0130 & mask ) ? EECS | EEDI : EECS;
        write32( device->base + DP_MEAR, l );
        write32( device->base + DP_MEAR, l | EECLK );
        mask >>= 1;
    }

    write32( device->base + DP_MEAR, 0 );               // terminate operation
    snooze( 5000 );

    //IOW32( mear, 0 );           /* clock out CS low */
    write32( device->base + DP_MEAR, EECLK );
    snooze( 5000 );
}

/************************************************
*       Function:   WriteEEDisable
*       Purpose:    Disable EEPROM write Mode
*	Returns:    void
*************************************************/
void writeEeDisable(dev_info_t * device)
{
uint32      mask;
uint32      l;
int			nbits = 9;

    //ed->eeput( (uint32)EEwriteDisable, 9 );
    nbits = 9;
    mask = ((uint32)1) << (nbits-1);

    while( nbits-- ) {
        /* assert chip select, and setup data in */
        l = ( 0x0100 & mask ) ? EECS | EEDI : EECS;
        write32( device->base + DP_MEAR, l );
        write32( device->base + DP_MEAR, l | EECLK );
        mask >>= 1;
    }

    write32( device->base + DP_MEAR, 0 );               // terminate operation
}

/************************************************
*       Function:   doEeCheckSum()
*       Purpose:    Read EEPROM contents, calculate checksum,
*                   write it, verify the write and validate it.
*************************************************/
void doEeCheckSum(dev_info_t * device)
{
int l;
uint16 value, checkSum;
unsigned char csLow, csHigh, csSum, csVals[ 11 ];

    // Calculate Checksums:
    for( l=0; l<11; l++ )
    {
    	ReadEEWord( device, l, &value );
    	    
    	csLow = value & 0xff;
		value >>= 8;
    	csHigh = value & 0xff;
    	
    	csVals[ l ] = csLow + csHigh;
    }
    
    // Calculate Checksums:
    csSum =  0;
    for( l=0; l<11; l++ )
    	{
        csSum += csVals[ l ];
        }
	
    csSum += 0x55;
    checkSum = (~csSum) + 1;
    checkSum = ( checkSum << 8 ) + 0x55;

    snooze( 10 );

    writeEeEnable(device);

    WriteEEWord( device, 0x0B, checkSum );

    writeEeDisable(device);
	
    ReadEEWord( device, 0xB, &value );
   	
    if( value != checkSum ) 
        {
        dprintf( "\n!!WARNING Checksum not programmed correctly!\n" );
        dprintf( "\n  checkSUM: %04X  actual: %04X", checkSum, value );
        }
        
    checkSumCheck(device);
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


static void dump_rx_descriptors(dev_info_t * device) {
	uint16 i;
	uint32 cmd_reg = read32(device->base + DP_CR);

	dprintf("DumpRx rx_rh=%x rx_isr=%x rx_free=%x cmd_reg=%x\n",
		 device->rx_rh, device->rx_isr, device->rx_free, cmd_reg);

	for ( i = 0; i < RX_BUFFERS; i++) {
		dprintf("rx_desc[%3.3d]=%8.8x link=%8.8x cmdsts=%8.8x  bufptr=%8.8x\n", i, device->rx_desc[i],
			device->rx_desc[i]->link, device->rx_desc[i]->cmdsts, device->rx_desc[i]->bufptr);
	}
}

static void dump_tx_descriptors(dev_info_t * device) {
	uint16 i;
	
	for ( i = 0; i < TX_BUFFERS; i++) {
		dprintf("tx_desc[%3.3d]=%8.8x des0=%8.8x des1=%8.8x pa_buf=%8.8x next=%8.8x buf=%8.8x\n", i,device->tx_desc[i],
			device->tx_desc[i]->link, device->tx_desc[i]->cmdsts, device->tx_desc[i]->bufptr);
	}
}

/* dp83815: Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   hit the alt sysreq keys, and type "dp83815 <cr>"
*/
#if DEBUGGER_COMMAND
static int
dp83815(int argc, char **argv)
{
	uint16 i,j,found=0xFFFF;
	dev_info_t *device = gdev;
	const char * usage = "usage: dp83815 [globals,numbered rx sequence, stats,rx,tx,pci,functions,write]\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'g':
			kprintf("dp83815 globals\n");
			{
#if 0
			kprintf("CR0 = %8.8x\n", read16(device->base + CR0));
			kprintf("ISR0 = %8.8x\n", read16(device->base + ISR0));
			kprintf("RDA0 = %8.8x\n", read16(device->base + RDA0));
			kprintf("RDA2 = %8.8x\n", read16(device->base + RDA2));
			kprintf("TDA0 = %8.8x\n", read16(device->base + TDA0));
			kprintf("TDA2 = %8.8x\n", read16(device->base + TDA2));
			kprintf( "RX indexes rh=%x isr=%x free=%x\n",device->rx_rh, device->rx_isr, device->rx_free);
			kprintf( "TX indexes wh=%x isr=%x sent=%x\n",device->tx_wh, device->tx_isr, device->tx_sent);
#endif
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
		case 'n':
			device->debug ^= SEQ;
			if (device->debug & SEQ) 
				kprintf("rx ping sequence Numbers Enabled\n");
			else 			
				kprintf("rx ping sequence Numbers Disabled\n");
			break; 
		case 'r':
			device->debug ^= RX;
			if (device->debug & RX) 
				kprintf("Recieve packet trace Enabled\n");
			else 			
				kprintf("Receive packet trace Disabled\n");
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



int32	api_version = B_CUR_DRIVER_API_VERSION;


#ifdef __ZRECOVER
# include <recovery/driver_registry.h>
fixed_driver_info dp83815_driver=
{
 	"dp83815 ethernet driver",
    B_CUR_DRIVER_API_VERSION,
    init_hardware,
    publish_devices,
    find_device,
    init_driver,
    uninit_driver
};
REGISTER_STATIC_DRIVER(dp83815_driver);         
#endif
