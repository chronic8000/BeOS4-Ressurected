// eepro100.c
// Intel EtherExpress Pro 10/100
// i82557-, i82558-, i82559-based PCI ethernet controllers
//
// Copyright 1999 Be, Inc.

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <OS.h>
#include <PCI.h>
#include <SupportDefs.h>
#include <ether_driver.h>
#include <endian.h>
#include <ByteOrder.h>

// constants, hardware-defined data structures, and globals
#define PCI_VENDOR_ID_INTEL 		0x8086
#define PCI_VENDOR_ID_INTEL_8255X 	0x1229

// these MUST be a power of two
#define TX_RING_SIZE	32		// must be a power of 2 *and* larger than TX_RING_WASTED
#define TX_RING_MASK    31 		// tx_ring_size - 1
#define TX_RING_WASTED	2		// this probably isn't necessary

#define RX_RING_SIZE	32		// must be a power of 2

// transmitter timeout detection
#define TX_TIMEOUT 	1000000

// Operational parameters that should not need to be changed
#define PACKET_BUF_SIZE 	1536 	// size of a preallocated Rx buffer
// recommended by Intel
#define txfifo 	8
#define rxfifo 	8

// Ethernet definitions (most are in ether_driver.h)
static const ether_address_t BROADCAST_ADDR = 
	{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }};

// Global variables
#define ANY_BUS 	-1
static pci_module_info	*pci;
static char pci_name[] = B_PCI_MODULE_NAME;

static char **devices; // for publish_devices

// Prototypes for memory mapped i/o functions
inline uint8 readb(volatile uchar* address);
inline uint16 readw(volatile uchar* address);
inline uint32 readl(volatile uchar* address);
inline void writeb(uint8 value, volatile uchar* address);
inline void writew(uint16 value, volatile uchar* address);
inline void writel(uint32 value, volatile uchar* address);

// Round up (to the nearest page when y is B_PAGE_SIZE)
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

// Entry points
_EXPORT status_t init_hardware( void );
_EXPORT status_t init_driver( void );
_EXPORT void uninit_driver( void );
_EXPORT const char** publish_devices( void );
_EXPORT device_hooks *find_device( const char *name );

//////////////////////////////////////////////////////////////////////////
// Debugging macros

//static int debug_level = 0; // Debugging is off
//static int debug_level = 1; // Debugging is on
//static int debug_level = 3; // All debugging is on
//static int debug_level = -1; // Debugging is completely silent
static int debug_level = 1;

// ERR is always printed.
// Use ERR when something goes wrong that will inibit proper operation of the card
#define DEBUG_ERR -1
#define ERR if (debug_level > DEBUG_ERR) dprintf

// INFO is always printed.
// Use INFO to put useful messages out to the user
#define DEBUG_INFO -1
#define INFO if (debug_level > DEBUG_INFO) dprintf

// DEBUG is only printed when debugging is turned on
#define DEBUG_DEBUG 0
#define DEBUG if (debug_level > DEBUG_DEBUG) dprintf

// VDEBUG is only printed when verbose debugging is turned on
#define DEBUG_VDEBUG 1
#define VDEBUG if (debug_level > DEBUG_VDEBUG) dprintf

// IDEBUG is only printed when interrupt handler debugging is turned on
#define DEBUG_IDEBUG 2
#define IDEBUG if (debug_level > DEBUG_IDEBUG) dprintf


//////////////////////////////////////////////////////////////////////////

// Register offsets into the speedo System Control Block
enum speedo_offsets
{
	SCBStatus = 0,		// Rx/Command unit status
	SCBCmd = 2, 		// Rx/Command unit command
	SCBPointer = 4, 	// General purpose pointer
	SCBPort = 8, 		// Misc. commands and operands
	SCBflash = 12,		// flash memory control
	SCBeeprom = 14, 	// EEPROM control
	SCBCtrlMDI = 16, 	// MDI interface control
	SCBEarlyRx = 20		// Early receive byte count
};

#if __INTEL__

// Commands that can be put into a command list entry
enum commands
{
	CmdNop = 0,					// NOP
	CmdIASetup = 0x00010000, 		// Station address setup
	CmdConfigure = 0x00020000, 	// Configure 
	CmdMulticastList = 0x00030000,	// Add multicast addresses 
	CmdTx = 0x00040000, 			// Transmit 
	CmdTDR = 0x00050000,			// 
	CmdDump = 0x00060000, 			// dump stats
	CmdDiagnose = 0x00070000, 		// 
	CmdSuspend = 0x40000000,	// Suspend after completion
	CmdIntr = 0x20000000, 		// Interrupt after completion
	CmdTxFlex = 0x00080000,		// Use "Flexible mode" for tx
	
	CmdEL = 0x80000000 			// End of List marker
};

// Commands and bitmasks for the SCB
enum SCBCmdBits
{
	StatMaskCX = 0x8000,			// Command completed with "I"nterrupt bit set
	StatMaskFR = 0x4000,			// Rx completed
	StatMaskCNA = 0x2000, 		// CU left active state
	StatMaskRNR = 0x1000, 		// RU is no longer in ready state
	StatMaskMDI = 0x0800, 		// MDI read or write cycle is done
	StatMaskSWI = 0x0400, 		// Software interrupt
	StatMaskFCP = 0x0100, 		// Flow Control Pause

	IntMaskCX = 0x80000000, 			// Clear CX interrupt
	IntMaskFR = 0x40000000, 			// Clear FR interrupt
	IntMaskCNA = 0x20000000, 		// Clear CNA interrupt
	IntMaskRNR = 0x10000000, 		// Clear RNR interrupt
	IntMaskER = 0x08000000, 			// Clear ER interrupt
	IntMaskFCP = 0x04000000, 		// Clear FCP interrupt
	IntMaskTriggerIntr = 0x02000000, // Trigger an interrupt
	IntMaskAll = 0x01000000,			// Do not assert INTA# (PCI interrupt pin)

	
	// Rx and Tx commands
	CUStart = 0x0010,				// Start Command Unit
	CUResume = 0x0020,				// 
	CUStatsAddr = 0x0040,			// 
	CUShowStats = 0x0050,			// 
	CUCmdBase = 0x0060,				// Command Unit base address (set to zero)
	CUDumpStats = 0x0070,			// Dump and then reset the stats counters
	RUStart = 0x0001,				// Start Receive Unit
	RUResume = 0x0002,				// 
	RUAbort = 0x0004,				// 
	RUCmdBase = 0x0006,				// set to zero
	RUResumeNoResources = 0x0007	//  
};

#define INT_MASK 0x0100

#else // BIGENDIAN

// Commands that can be put into a command list entry
enum commands
{
	CmdNop = 0,					// NOP
	CmdIASetup = 0x00000100, 		// Station address setup
	CmdConfigure = 0x00000200, 	// Configure 
	CmdMulticastList = 0x00000300,	// Add multicast addresses 
	CmdTx = 0x00000400, 			// Transmit 
	CmdTDR = 0x00000500,			// 
	CmdDump = 0x00000600, 			// dump stats
	CmdDiagnose = 0x00000700, 		// 
	CmdSuspend = 0x00000040,	// Suspend after completion
	CmdIntr = 0x00000020, 		// Interrupt after completion
	CmdTxFlex = 0x00000800,		// Use "Flexible mode" for tx
	
	CmdEL = 0x00000080 			// End of List marker
};

// Commands and bitmasks for the SCB
enum SCBCmdBits
{
	StatMaskCX = 0x0080,			// Command completed with "I"nterrupt bit set
	StatMaskFR = 0x0040,			// Rx completed
	StatMaskCNA = 0x0020, 		// CU left active state
	StatMaskRNR = 0x0010, 		// RU is no longer in ready state
	StatMaskMDI = 0x0008, 		// MDI read or write cycle is done
	StatMaskSWI = 0x0004, 		// Software interrupt
	StatMaskFCP = 0x0001, 		// Flow Control Pause

	IntMaskCX = 0x00000080, 			// Clear CX interrupt
	IntMaskFR = 0x00000040, 			// Clear FR interrupt
	IntMaskCNA = 0x00000020, 		// Clear CNA interrupt
	IntMaskRNR = 0x00000010, 		// Clear RNR interrupt
	IntMaskER = 0x00000008, 			// Clear ER interrupt
	IntMaskFCP = 0x00000004, 		// Clear FCP interrupt
	IntMaskTriggerIntr = 0x00000002, // Trigger an interrupt
	IntMaskAll = 0x00000001,			// Do not assert INTA# (PCI interrupt pin)
	
	// Rx and Tx commands
	CUStart = 0x1000,				// Start Command Unit
	CUResume = 0x2000,				// 
	CUStatsAddr = 0x4000,			// 
	CUShowStats = 0x5000,			// 
	CUCmdBase = 0x6000,				// Command Unit base address (set to zero)
	CUDumpStats = 0x7000,			// Dump and then reset the stats counters
	RUStart = 0x0100,				// Start Receive Unit
	RUResume = 0x0200,				// 
	RUAbort = 0x0400,				// 
	RUCmdBase = 0x0600,				// set to zero
	RUResumeNoResources = 0x0700	//  
};

#define INT_MASK 0x0001

#endif // __INTEL__


//////////////////////////////////////////////////////////////////////////
// Speedo3 Rx and Tx frame/buffer descriptors

// Rx frame descriptor
typedef volatile struct
{
	uint32 status;			// Status
	uint32 link; 			// Pointer to another RxFD
	uint32 rx_buf_addr;		// Generic pointer
	uint16 count;			// actual count
	uint16 size; 			// size of buffer
} RxFD;

// Tx command block
typedef volatile struct
{
	uint32 status;			// Status
	uint32 link; 			// Pointer to another TxFD
	uint32 tx_desc_addr;	// Always points to the tx_buf_addr element
	uint32 count; 			// # of TBD (=1), Tx start thresh., etc.
	// Pointers to transmit buffers (TBD) (only included because they are expected by hardware)
	uint32 tbd_pointer;		// Pointer (void*) to frame to be transmitted
	uint32 tbd_size; 		// Length of Tx frame
	uint32 tbd_pointer2; 	// 
	uint32 tbd_size2; 		// 

} TxCB;

// Driver data
typedef volatile struct
{
//	uint32 flags; 	// flags? (owner)
	uchar *pci_desc;		// descriptor PCI address
	uchar *pci_pkt; 		// packet buffer PCI address
} Flags;

// Packet buffer
typedef struct
{
	uchar buf[PACKET_BUF_SIZE];
} PacketBuffer;

//////////////////////////////////////////////////////////////////////////////////////
// Bundle them all together

// Tx ring entry 
typedef volatile struct
{
	Flags flags;			// flags
	TxCB desc; 				// command block
	PacketBuffer packet; 	// Packet buffer
} TxEntry;

// Rx ring entry
typedef volatile struct
{
	Flags flags;			// flags
	RxFD desc; 				// command block
	PacketBuffer packet; 	// Packet buffer
} RxEntry;


//////////////////////////////////////////////////////////////////////////////////////

// Selected elements of the status field
enum status_bits
{
	RxComplete = 0x8000,
	RxOK = 0x2000,
	RxErrCRC = 0x0800,
	RxErrAlign = 0x0400,
	RxErrTooBig = 0x0200,
	RxErrSymbol = 0x0010,
	RxEth2Type = 0x0020,
	RxNoMatch = 0x0004,
	RxNoIAMatch = 0x0002,
	TxUnderrun = 0x1000,
	StatusComplete = 0x8000,
	StatusOK = 0x2000
};

// Elements of the dump_statistics block. The block must be lword aligned.
typedef struct speedo_stats_t
{
	uint32 tx_good_frames;
	uint32 tx_coll16_errs;
	uint32 tx_late_colls;
	uint32 tx_underruns;
	uint32 tx_lost_carrier;
	uint32 tx_deferred;
	uint32 tx_one_colls;
	uint32 tx_multi_colls;
	uint32 tx_total_colls;
	uint32 rx_good_frames;
	uint32 rx_crc_errs;
	uint32 rx_align_errs;
	uint32 rx_resource_errs;
	uint32 rx_overrun_errs;
	uint32 rx_colls_errs;
	uint32 rx_runt_errs;
	uint32 done_marker;
} speedo_stats;

// Configuration settings for CmdConfigure
static const char i82557_config_cmd[22] =
{
	22, 0x08, 0, 0, 0, 0, 0x32, 0x03, 1, /* 1=Use MII, 2=User AUI */
	0, 0x2E, 0, 0x60, 0,
	0xF2, 0x48, 0, 0x40, /* 0x40=Force full-duplex */
	0xF2, 0x80, 0x3F, 0x05
};

static const char i82558_config_cmd[22] =
{
/* 0-3 */	22, 0x08, 0, 1,
/* 4-7 */	//0, 0, 0x2a, 0x03, // 0x2a = CI intr only
/* 4-7 */	0, 0, 0x22, 0x03, // 0x22 = CNA intr
/* 8-11 */	1, /* 1=Use MII, 2=Use AUI */
			0, 0x2E, 0,
/* 12-15 */	0x60, 0x08, 0x88, 0x68,
/* 16-19 */	0, 0x40, 0xF2, 0xBD, /* 0xBD->0xFD=Force full-duplex */
/* 20-21 */	0x31, 0x05
};

// PHY media interface chips
static const char *phys[] =
{
	"None", "i82553-A/B", "i82553-C", "i82503", "DP83840",
	"80c240", "80c24", "i82555", "unknown-8", "unknown-9",
	"DP83840A", "unknown-11", "unknown-12", "unknown-13",
	"unknown-14", "unknown-15"
};
enum phy_chips
{
	NoSuchPhy = 0, I82553AB, I82553C, I82503, DP83840,
	S80C240, S80C24, I82555,
	DP83840A = 10
};

static const char is_mii[] = { 0, 1, 1, 0, 1, 1, 0, 1 };

// Multiple card support

typedef struct card_info_t
{
	pci_info p;
	char *name;
	int open_flag;
} card_info;

static card_info *cards;
static int card_cnt = 0;

//////////////////////////////////////////////////////////////////////////

typedef struct speedo_private_t
{
	const char *dev_name;

	ether_address_t myaddr; 	// my station address

	char cmd_unit_started; 		// has command unit been started
	char rx_unit_running;

	char bus;
	char device;
	char function;
	char irq;

	bigtime_t last_rx_time; 	// last Rx time, to detect Rx hang
	bigtime_t last_tx_time; 	// last Tx time, to detect Tx hang

	// Memory management	
	area_id private_area; 		// private data
	area_id io_area; 			// memory-mapped i/o
	area_id tx_area;		// Transmit bundles
	area_id rx_area; 		// Receive bundles
	
	volatile uchar *io_base; 			// memory-mapped i/o base address	
	// Ring descriptors and buffers
	TxEntry *tx_base;
	RxEntry *rx_base;

	// For setting memory addresses on the card
	uchar *pci_tx_base; 		// PCI address of start of Tx ring
	uchar *pci_rx_base;			// PCI address of start of Rx ring

	// manipulating the rings
	uint32 cur_rx; 				// the next free Rx ring entry
	uint32 cur_tx; 				// the next free Tx ring entry
	uint32 last_rx; 			// last Rx ring entry
	uint32 dirty_tx;			// last tx command
	uint32 card_owns_rx; 		// first Rx block that the card owns
	uint32 last_tx_cmd;			// last command executed
	
	// Synchronization
	sem_id mutex; 				// use when modifying Tx Cmd list
	sem_id rx_sem;	
	sem_id tx_sem;
	sem_id hw_lock;
	
	// Multicast setup
	int mcast_count;			// number of multicast addresses
	ether_address_t mcast[64]; 	// list of multicast addresses
	TxCB mcast_cb;
	sem_id mcast_lock;

	// Configuration
	uchar config_data[22]; 		// Configuration bytes
	uint32 blocking_flag; 				// use nonblocking read
	
//	uchar rx_mode;						/* Current PROMISC/ALLMULTI setting. */

//	unsigned char full_duplex;			/* Full-duplex operation requested. */
//	unsigned char flow_ctrl;			/* Use 802.3x flow control. */
//	unsigned char rx_bug;				/* Work around receiver hang errata. */
//	unsigned char rx_bug10;			/* Receiver might hang at 10mbps. */
//	unsigned char rx_bug100;			/* Receiver might hang at 100mbps. */
//	unsigned char default_port;		/* Last dev->if_port value. */
	unsigned short phy[2];				/* PHY media interfaces available. */

	// Statistics
	speedo_stats card_stats; 

} speedo_private;

//////////////////////////////////////////////////////////////////////////

// Other prototypes
static int32 speedo_isr(void* cookie); 		// Interrupt handler
static status_t start_cmd(speedo_private *sp, int *tx_entry, TxCB **cmd, uchar **buf);
static status_t finish_cmd(speedo_private *sp, int tx_entry);

//////////////////////////////////////////////////////////////////////////


// Serial EEPROM section.

//static int read_eeprom(long ioaddr, int location, int addr_len = 6);
static int read_eeprom(volatile uchar* ioaddr, int location, int addr_len);

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x01	/* EEPROM shift clock. */
#define EE_CS			0x02	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x04	/* EEPROM chip data in. */
#define EE_WRITE_0		0x01
#define EE_WRITE_1		0x05
#define EE_DATA_READ	0x08	/* EEPROM chip data out. */
#define EE_ENB			(0x4800 | EE_CS)

/* Delay between EEPROM clock transitions.
   This will actually work with no delay on 33Mhz PCI.  */
#define eeprom_delay(nanosec)		spin(1);

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD	(5 << addr_len)
#define EE_READ_CMD		(6 << addr_len)
#define EE_ERASE_CMD	(7 << addr_len)

//static int read_eeprom(long ioaddr, int location, int addr_len = 6)
static int read_eeprom(volatile uchar* ioaddr, int location, int addr_len)
{
	unsigned short retval = 0;
	uchar* ee_addr = (uchar*)(ioaddr + SCBeeprom);
	int read_cmd = location | EE_READ_CMD;
	int i;

	
	writew((uint16)(EE_ENB & ~EE_CS), ee_addr);
	writew((uint16)EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 12; i >= 0; i--) {
		short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		writew(EE_ENB | dataval, ee_addr);
		eeprom_delay(100);
		writew(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay(150);
	}
	writew(EE_ENB, ee_addr);

	for (i = 15; i >= 0; i--) {
		writew(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay(100);
		retval = (retval << 1) | ((readw(ee_addr) & EE_DATA_READ) ? 1 : 0);
		writew(EE_ENB, ee_addr);
		eeprom_delay(100);
	}

	/* Terminate the EEPROM access. */
	writew(EE_ENB & ~EE_CS, ee_addr);
	return retval;
}

static int mdio_read(volatile long ioaddr, int phy_id, int location)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	writel(0x08000000 | (location<<16) | (phy_id<<21), (uchar*)(ioaddr + SCBCtrlMDI));
	do {
		val = readl((uchar*)(ioaddr + SCBCtrlMDI));
		if (--boguscnt < 0) {
			ERR("eepro100: mdio_read() timed out with val = %8.8x.\n", val);
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

static int mdio_write(volatile long ioaddr, int phy_id, int location, int value)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	writel(0x04000000 | (location<<16) | (phy_id<<21) | value,
		 (uchar*)(ioaddr + SCBCtrlMDI));
	do {
		val = readl((uchar*)(ioaddr + SCBCtrlMDI));
		if (--boguscnt < 0) {
			ERR("eepro100: mdio_write() timed out with val = %8.8x.\n", val);
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

//////////////////////////////////////////////////////////////////////////

/* The following inline functions may be called after calling
   map_physical_memory(). These are like their Linux counterparts 
   after calling the Linux function vremap(). The similarity is
   just to make reading Linux driver code easier. */
// written by Russ

inline uint8 readb(volatile uchar* address) 
{	
	return *((volatile uint8*)(address));
}

inline uint16 readw(volatile uchar* address) 
{
	return *((volatile uint16*)(address));
}

inline uint32 readl(volatile uchar* address) 
{	
	return *((volatile uint32*)(address));	
}

inline void writeb(uint8 value, volatile uchar* address) 
{
	*((volatile uint8*)(address)) = value;
}

inline void writew(uint16 value, volatile uchar* address) 
{
	*((volatile uint16*)(address)) = value;
}

inline void writel(uint32 value, volatile uchar* address)  
{
	*((volatile uint32*)(address)) = value;
}

//////////////////////////////////////////////////////////////////////////

// assumes "pci" is set (in init_driver)
// get_memory_map is expensive, so don't call this after open. 
unsigned long virt_to_bus(void * address)
{
	physical_entry	table[1];
	get_memory_map(address, PACKET_BUF_SIZE, table, 1);
	return (unsigned long)(*pci->ram_address)(table[0].address);
}

// utility routines

// Wait for the command unit to accept a command (typically takes 0 ticks)
inline void wait_for_cmd_done(speedo_private* sp)
{	
	int ticks = 100; // wait for no more than 100 ticks
	while ((readb((uchar*)(sp->io_base + SCBCmd))) && (--ticks >= 0))
		if (! ticks % 10) spin(1);
	if (ticks <= 0)
		ERR("eepro100: wait_for_cmd_done() timed out: something is wrong!\n");
}

// find a Speedo card on the PCI bus
status_t find_speedo(char bus, char device, char function)
{
	// iterate over the pci bus
	int i, cnt, j = 0;
	pci_module_info *pci;
	static char pci_name[] = B_PCI_MODULE_NAME;
	status_t rv = B_ERROR; // return value
	pci_info pinfo;
	pci_info *p = &pinfo;
	char namebuf[255]; // is this long enough?
	
	VDEBUG("eepro100: find_speedo()\n");
	
	if (get_module(pci_name, (module_info **)&pci))
		return B_ERROR;

	// make one pass to find cards	
	cnt = 0;
	for (i = 0; (*pci->get_nth_pci_info)(i, p) == B_NO_ERROR; i++) {
		if ((p->vendor_id != PCI_VENDOR_ID_INTEL) ||
			(p->device_id != PCI_VENDOR_ID_INTEL_8255X))
			continue;
		if (bus == ANY_BUS ||
		   (p->bus == bus && p->device == device && p->function == function)) {
			cnt++;
			DEBUG("eepro100: find_speedo(): found a card\n");
		}
	}

	// make a second pass to record pci_info

	cards = (card_info*)malloc(cnt * sizeof(card_info));
	
	for (i = 0; (*pci->get_nth_pci_info)(i, p) == B_NO_ERROR; i++) {
		if ((p->vendor_id != PCI_VENDOR_ID_INTEL) ||
			(p->device_id != PCI_VENDOR_ID_INTEL_8255X))
			continue;
		if (bus == ANY_BUS ||
		   (p->bus == bus && p->device == device && p->function == function)) {
			rv = B_OK;
			card_cnt++; // global count
			cards[j].p = pinfo;
			sprintf(namebuf, "net/eepro100/%d", j);
			cards[j].name = (char*)malloc(strlen(namebuf) + 1);
			strcpy(cards[j].name, namebuf);
			cards[j].open_flag = 0;
			INFO("eepro100: find_speedo(): setting up /dev/%s\n", cards[j].name);
			j++;
		}
	}

	put_module(pci_name);

	return rv;
}


// area_malloc gets you an area that's locked in memory
uchar * area_malloc(char *area_name, size_t size, area_id *areap)
{
	uchar *base;
	area_id id;
	size_t mysize;
	
	// Round up to the nearest page size
	mysize = RNDUP(size, B_PAGE_SIZE);
	DEBUG("eepro100: area_malloc(%s, %d): using size = %d\n", area_name, size, mysize);
	id = create_area(area_name, (void **) &base, B_ANY_KERNEL_ADDRESS,
					 mysize,
					 B_CONTIGUOUS,
					 B_READ_AREA | B_WRITE_AREA);
	if (id < B_NO_ERROR) {
		ERR("eepro100: error creating area in area_malloc(%s)\n", area_name);
		return (NULL);
	}
	if (lock_memory(base, mysize, B_DMA_IO | B_READ_DEVICE) != B_NO_ERROR) {
		delete_area(id);
		ERR("eepro100: error locking memory in area_malloc(%s)\n", area_name);
		return (NULL);
	}
	// clear the area
	memset(base, 0, mysize);
	// set the area id
	*areap = id;
	// return a pointer to the beginning of the area
	return (base);
}

// Utility routines

// Print an ethernet address
void
print_address(
			  ether_address_t *addr
			  )
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	dprintf("%s\n", buf);
}

void
printbits32(uint32 x)
{
	char bits[33];
	char pretty[40];
	int i, j;
	
	for (i = 1; i <= 32; i++) {
		bits[32 - i] = x & 1 ? '1' : '0';
		x = x >> 1;
	}
	bits[32] = 0;

	j = 0;
	for (i = 0; i < 32; i++) {
		pretty[j] = bits[i];
		if (((i + 1) & 0x03) == 0)
			pretty[++j] = ' ';
		j++;
	}
	pretty[39] = 0;
	dprintf("%s", pretty);
}
void
printbits16(uint16 x)
{
	char bits[17];
	char pretty[20];
	int i, j;
	
	for (i = 1; i <= 16; i++) {
		bits[16 - i] = x & 1 ? '1' : '0';
		x = x >> 1;
	}
	bits[17] = 0;

	j = 0;
	for (i = 0; i < 16; i++) {
		pretty[j] = bits[i];
		if (((i + 1) & 0x03) == 0)
			pretty[++j] = ' ';
		j++;
	}
	pretty[19] = 0;
	dprintf("%s", pretty);
}


void dump_rxfd(RxFD *c)
{
	dprintf("      status: 0x%8.8x    ", c->status); printbits32(c->status); dprintf("\n");
	dprintf("        link: 0x%8.8x    ", c->link); printbits32(c->link); dprintf("\n");
	dprintf(" rx_buf_addr: 0x%8.8x    ", c->rx_buf_addr); printbits32(c->rx_buf_addr); dprintf("\n");
	dprintf("        size: 0x%4.4x        ", c->size); printbits16(c->size); dprintf("\n");
	dprintf("       count: 0x%4.4x        ", c->count); printbits16(c->count); dprintf("\n");
}

void dump_txcb(TxCB *c)
{
	dprintf("TxCB dump at virt: 0x%8.8x, phys: 0x%8.8x:\n", c, virt_to_bus(c));
	dprintf("      status: 0x%8.8x    ", c->status); printbits32(c->status); dprintf("\n");
	dprintf("        link: 0x%8.8x    ", c->link); printbits32(c->link); dprintf("\n");
	dprintf("tx_desc_addr: 0x%8.8x    ", c->tx_desc_addr); printbits32(c->tx_desc_addr); dprintf("\n");
	dprintf("       count: 0x%8.8x    ", c->count); printbits32(c->count); dprintf("\n");
}

void dump_csr(uchar* ioaddr) {
	dprintf("eepro100: Command Status Register dump at 0x%8.8x:\n", ioaddr);
	dprintf("          Status word: 0x%4.4x        ", readw(ioaddr + 0x00)); printbits16(readw(ioaddr + 0x00)); dprintf("\n");
	dprintf("         Command word: 0x%4.4x        ", readw(ioaddr + 0x02)); printbits16(readw(ioaddr + 0x02)); dprintf("\n");
	dprintf("      General Pointer: 0x%8.8x    ", readl(ioaddr + 0x04)); printbits32(readl(ioaddr + 0x04)); dprintf("\n");
//	dprintf("                 PORT: 0x%8.8x    ", readl(ioaddr + 0x08)); printbits32(readl(ioaddr + 0x08)); dprintf("\n");
//	dprintf("       EEPROM Control: 0x%4.4x        ", readw(ioaddr + 0x0a)); printbits16(readw(ioaddr + 0x0a)); dprintf("\n");
//	dprintf("        Flash Control: 0x%4.4x        ", readw(ioaddr + 0x0c)); printbits16(readw(ioaddr + 0x0c)); dprintf("\n");
//	dprintf("          MDI Control: 0x%8.8x    ", readl(ioaddr + 0x10)); printbits32(readl(ioaddr + 0x10)); dprintf("\n");
//	dprintf("             Reserved: 0x%8.8x    ", readl(ioaddr + 0x14)); printbits32(readl(ioaddr + 0x14)); dprintf("\n");
//	dprintf("PMDR/Flow Control/...: 0x%8.8x    ", readl(ioaddr + 0x18)); printbits32(readl(ioaddr + 0x18)); dprintf("\n");
}

void dump_packet(const char * msg, unsigned char * buf) {
	// dump the first 3 x 16 bytes 
	dprintf("%s\n%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x \n", msg,
	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
	buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23],
	buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);

	dprintf("%2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",
	buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39],
	buf[40], buf[41], buf[42], buf[43], buf[44], buf[45], buf[46], buf[47]);
}


//////////////////////////////////////////////////////////////////////////
// Initialize the memory areas for rings

status_t init_ring(char *area_name, int size, area_id *area, uchar** base_address)
{
	area_id my_area; 
	uchar *base;
	
	base = area_malloc(area_name, size, &my_area);
	
	if (NULL == base) { 
		ERR("eepro100: error initializing %s in init_ring()!\n", area_name); 
		return B_ERROR; 
	}
	*area = my_area;
	*base_address = base; 

	return B_OK;
}	

status_t init_ring_areas(speedo_private *sp)
{
	// allocate contiguous areas for each set of buffers
	int size ;
	int i;
	
	uint32 default_status;
	
	DEBUG("eepro100: init_ring_areas()\n");

	VDEBUG("eepro100: init_ring_areas(): creating Tx area\n");
	// Create Tx area  
	size = sizeof(TxEntry) * TX_RING_SIZE;
	init_ring("eepro100 Tx area", size, &sp->tx_area, (uchar**)(&sp->tx_base));
	sp->pci_tx_base = (uchar*)virt_to_bus(sp->tx_base);

	
	VDEBUG("eepro100: init_ring_areas(): creating Rx area\n");
	// Create rx area
	size = sizeof(RxEntry) * RX_RING_SIZE;
	init_ring("eepro100 Rx area", size, &sp->rx_area, (uchar**)(&sp->rx_base));
	sp->pci_rx_base = (uchar*)virt_to_bus(sp->rx_base);
	


	/// Fill in default values for each block
	VDEBUG("eepro100: init_ring_areas(): filling in default values \n");

	// default status for a TxCB
	default_status = CmdSuspend | CmdNop; 
	
	VDEBUG("eepro100: init_ring_areas(): filling in default values for Tx data\n");
	// Fill in Tx data
	for (i = 0; i < TX_RING_SIZE; i++) {

		// Get descriptor
		TxCB *txcb = &(sp->tx_base[i].desc); 

		// set up links (make a circular list)
		txcb->link = virt_to_bus(&(sp->tx_base[(i + 1) % TX_RING_SIZE].desc));

		// point pci_desc to this descriptor
		sp->tx_base[i].flags.pci_desc = virt_to_bus(txcb);

		// we're using simplified mode
		txcb->tx_desc_addr = 0xffffffff;
			
		// set up status
		txcb->status = default_status;

		// Keep track of the PCI address of the packet buffer
		sp->tx_base[i].flags.pci_pkt = virt_to_bus(sp->tx_base[i].packet.buf);

	}

	// Initialize pointers into the ring 
	sp->dirty_tx = sp->cur_tx = sp->last_tx_cmd = 0;

	// debugging
//	if (debug_level > DEBUG_VDEBUG) {
//		VDEBUG("eepro100: Tx array summary:\n");
//		for (i = 0; i < TX_RING_SIZE; i++) {
//			VDEBUG("    tx_cbs[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->tx_cbs[i], virt_to_bus(sp->tx_cbs[i]));
//			INFO("tx_cbs[%d]: 0x%8.8x, tx_cbs[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x, diff: %d\n",
//				i, sp->tx_cbs[i], i - 1, sp->tx_cbs[(i-1)>0?(i-1):0], virt_to_bus(sp->tx_cbs[i]),
//				sp->tx_cbs[i] - sp->tx_cbs[(i-1)>0?(i-1):0]);
//			VDEBUG("tx_buffers[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->tx_buffers[i], virt_to_bus(sp->tx_buffers[i]));
//			dump_txcb(sp->tx_cbs[i]);
//		}
//	}

//	VDEBUG("eepro100: init_ring_areas(): filling in default values for Rx data\n");
	// Fill in Rx data
	for (i = 0; i < RX_RING_SIZE; i++) {
		RxFD *rxfd = &(sp->rx_base[i].desc); 

		// set buffer pointers
		rxfd->rx_buf_addr = sp->rx_base[i].flags.pci_pkt =
			virt_to_bus(sp->rx_base[i].packet.buf);

		// set links
		rxfd->link = virt_to_bus(&(sp->rx_base[(i + 1) & (RX_RING_SIZE - 1)].desc));
		
		// set pci_desc
		sp->rx_base[i].flags.pci_desc = virt_to_bus(rxfd);
		
		rxfd->status = 0; 
		rxfd->rx_buf_addr = 0xffffffff; 	// data follows immediately in simple mode
		rxfd->size = PACKET_BUF_SIZE;
		rxfd->count = 0;
	}

	// Mark the last entry as suspended.
	sp->rx_base[RX_RING_SIZE - 1].desc.status = CmdSuspend;

	sp->last_rx = RX_RING_SIZE - 1;
	sp->cur_rx = sp->card_owns_rx = 0;

/*	if (debug_level > DEBUG_VDEBUG) {*/
/*		VDEBUG("eepro100: Rx array summary:\n");*/
/*		for (i = 0; i < RX_RING_SIZE; i++) {*/
/*			VDEBUG("    rx_fds[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->rx_fds[i], virt_to_bus(sp->rx_fds[i]));*/
/*			VDEBUG("rx_buffers[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->rx_buffers[i], virt_to_bus(sp->rx_buffers[i]));*/
/*//			dump_rxfd(sp->rx_fds[i]);*/
/*		}*/
/*//	}*/

//	VDEBUG("eepro100: leaving init_ring_areas()\n");

	return B_OK;
}

status_t uninit_ring_areas(speedo_private *sp)
{
//	DEBUG("eeprol00: uninit_ring_areas\n");
	
	delete_area(sp->tx_area);
	delete_area(sp->rx_area);
	
	return B_OK;
}

//////////////////////////////////////////////////////////////////////////

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	INFO("eepro100: init_hardware()\n");
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	INFO("eepro100: init_driver()\n");
	
	if (get_module(pci_name, (module_info **) &pci))
		return B_ERROR; 

	return find_speedo(ANY_BUS, 0, 0); // build the cards array
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
	int i;

	INFO("eepro100: uninit_driver()\n");
	
	// free cards info
	for (i = 0; i < card_cnt; i++) {
		free(cards[i].name);
	}
/*
	// free published names
	for (i = 0; i < card_cnt; i++)
		free(devices[i]);
	free(devices);
*/
	put_module(pci_name); /* matches the call to get_module() in init_driver */
}

// Read the eeprom and fill in private data
status_t
setup_from_eeprom(speedo_private *sp)
{
	volatile uchar *ioaddr;
	int i, val, count;
	ether_address_t mac_addr;
	const char *connectors[] = {" RJ45", " BNC", " AUI", " MII"};
	uint16 ee[64]; // hold the eeprom data
	
	ioaddr = sp->io_base;
	DEBUG("eepro100: EEPROM contents:\n");
	// read_eeprom returns a 16-bit integer.
	val = 0;
	count = 0;
	for (i = 0; i < 0xd; i++) {
		val = read_eeprom(ioaddr, i, 6);
		ee[i] = val;
		DEBUG("[%2.2x]: %2.2x %2.2x  ", i, (uint8)(val >> 8), (uint8)val);
		if (! ((i + 1)%3)) DEBUG("\n");
	}
	DEBUG("\n");

	// get some other options from the eeprom
	sp->phy[0] = ee[6];
	sp->phy[1] = ee[7];
//	sp->rx_bug = (ee[3] & 0x03) == 3 ? 0 : 1;
	
	// find the station address
	for (i = 0; i < 6; i++) {
		mac_addr.ebyte[i] = (uint8)(ee[i / 2]);
		mac_addr.ebyte[++i] = ee[i / 2] >> 8;
	}
	memcpy(&sp->myaddr, &mac_addr, sizeof(ether_address_t));
	INFO("eepro100: MAC address: ");
	if (debug_level > DEBUG_INFO)
		print_address(&sp->myaddr);	
	
	if (ee[3] & 0x03)
		DEBUG("eepro100: Receiver lock-up bug exists\n");
		
	INFO("eepro100: Board assembly: %4.4x%2.2x-%3.3d, connectors:",
		ee[8], ee[9]>>8, ee[9] & 0xff);
	for (i = 0; i < 4; i++) {
		if (ee[5] & (1 << i)) {
			INFO(connectors[i]);
		}
	}
	INFO("\n");
	
	return B_NO_ERROR;
}


// self_test runs a self test on the card
status_t
self_test(speedo_private* sp)
{
	int32 results[6];
	int32 *results_ptr;
	void *where;
	physical_entry table[1]; // set up some memory for the card to put the results
	int boguscnt = 16000; 

	INFO("eepro100: Running a self test:\n");
	
	results_ptr = (int32*) ((((long) results) + 15) & ~0xf);
	results_ptr[0] = 0;
	results_ptr[1] = -1;
	
	lock_memory(results_ptr, sizeof(int32)*6 - (results_ptr - results), B_DMA_IO);
	get_memory_map(results_ptr, sizeof(int32)*6 - (results_ptr - results), table, 1);
	where = (*pci->ram_address)(table[0].address);
	writel(((unsigned long)where) | 1, (uchar*)(sp->io_base + SCBPort));
	
	do { spin(10); }
		while (results_ptr[1] == -1 && --boguscnt >=0);
		
	unlock_memory(results_ptr, sizeof(int32)*6 - (results_ptr - results), B_DMA_IO);
	
	// check the results
	if (boguscnt < 0) {
		ERR("eepro100: Self test timed out! status: %8.8x: \n", B_LENDIAN_TO_HOST_INT32(results_ptr[1]));
	}
	else {
		int32 res = B_LENDIAN_TO_HOST_INT32(results_ptr[1]);
		INFO("eepro100: General self test: %s\n", res & 0x1000 ? "failed" : "passed");
		INFO("eepro100: Serial subsystem self test: %s\n", res & 0x0020 ? "failed" : "passed");
		INFO("eepro100: Internal registers self test: %s\n", res & 0x0008 ? "failed" : "passed");
		INFO("eepro100: ROM checksum self test: %s (0x%8.8x)\n", res & 0x0004 ? "failed" : "passed", results_ptr[0]);
	}

	// if there was an error, return B_ERROR
	if (B_LENDIAN_TO_HOST_INT32(results_ptr[1]) & 0x102c) {
		ERR("eepro100: One or more self tests failed!\n");
		return B_ERROR;
	}

	// reset the card
	writel(0, (uchar*)(sp->io_base + SCBPort));

	return B_NO_ERROR;
}


/* ----------
	speedo_open - handle open() calls
----- */

static status_t
speedo_open (const char *name, uint32 openflags, void** cookie)
{
	area_id area;
	speedo_private *sp;
	pci_info p;
	int32 base, size;
	volatile uchar* ioaddr;
	uint32 pci_read_val;
	
	int i;

	char flag;

	int tx_entry;
	
	TxCB *cmd;
	uchar *buf;
	
	INFO("eepro100: open(/dev/%s): driver built on %s at %s\n", name, __DATE__, __TIME__);
	
	// locate the card and set up private data
	// find the correct card
	flag = 0;
	for (i = 0; i < card_cnt; i++) {
		if (!strcmp(cards[i].name, name)) {
			if (cards[i].open_flag == 1) {
				ERR("eepro100: card %s is already open!\n", name);
				return B_BUSY;
			}
			cards[i].open_flag = 1;
			p = cards[i].p;
			flag = 1;
			break;
		}	
	}
	if (!flag) {
		// card not found!
		ERR("eepro100: unable to locate %s\n", name);
		return EINVAL;
	}

	sp = (speedo_private *)area_malloc("eepro100 private data", sizeof(speedo_private), &area);
	if (!sp) {
		ERR("eepro100: out of memory!\n");
		return B_NO_MEMORY;
	}
	
	// assign the private data to the cookie
	*cookie = sp;
		
	sp->private_area = area;
	
	// initialize private data
	sp->dev_name = "Intel EtherExpressPro 10/100";
	sp->blocking_flag = 0;

	sp->mcast_count = 0;

	sp->bus = p.bus;
	sp->device = p.device;
	sp->function = p.function;
	sp->irq = p.u.h0.interrupt_line;
	INFO("eepro100: using PCI device %d with function 0x%2.2x on bus %d at IRQ %d\n",
		sp->device, sp->function, sp->bus, sp->irq);
		
	
	// base address and size of registers
	base = p.u.h0.base_registers[0];
	size = p.u.h0.base_register_sizes[0];

	// round the base address of the register down to the nearest
	// page boundary
	base = base & ~(B_PAGE_SIZE - 1);
	
	// adjust the size of our register space based on the rounding above
	size += p.u.h0.base_registers[0] - base;
	
	// round up to the neares page size boundary so that we occupy
	// only complete pages, and not any partial pages.
	size = (size + (B_PAGE_SIZE -1)) & ~(B_PAGE_SIZE - 1);


	// set things up for memory-mapped i/o.
	sp->io_area = map_physical_memory("speedo memory-mapped i/o",
		(void*)base,
		size,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA + B_WRITE_AREA,
		(void**)&sp->io_base);
	
	if (sp->io_area < 0)
		return (B_ERROR);

	// indicate that we are using memory-mapped i/o
	(*pci->write_pci_config)(sp->bus, sp->device, sp->function, PCI_command, 1, 0x16);

	// check latency timer
	pci_read_val = (*pci->read_pci_config)(sp->bus, sp->device, sp->function, PCI_latency, 1);
	DEBUG("eepro100: PCI_latency is %d. (before)\n", pci_read_val);
	if ((uchar)pci_read_val < 32) {
		INFO("eepro100: PCI_latency was %d, setting to 32.\n", pci_read_val);
		(*pci->write_pci_config)(sp->bus, sp->device, sp->function, PCI_latency, 1, 32);
	}
	DEBUG("eepro100: PCI_latency is %d. (after)\n", (*pci->read_pci_config)(sp->bus, sp->device, sp->function, PCI_latency, 1));
	
	sp->io_base += (base & (B_PAGE_SIZE - 1)) / sizeof(ulong);	

	ioaddr = sp->io_base;

	DEBUG("eepro100: memory-mapped i/o base address: 0x%x\n", ioaddr);


	// Initialize the buffers
	//VDEBUG("eepro100: initializing rings...\n");
	if (init_ring_areas(sp) < B_NO_ERROR) {
		ERR("eepro100: problem initializing send and receive buffers!\n");
		return B_ERROR;
	}
	VDEBUG("eepro100: completed initializing rings\n");


/*	if (debug_level > DEBUG_VDEBUG) {
		VDEBUG("eepro100: Tx ring summary: dump of TX_RING_SIZE=%d blocks\n", TX_RING_SIZE);
		for (i = 0; i < TX_RING_SIZE; i++) {
			TxCB *desc = sp->tx_cbs[i];
			VDEBUG("tx_cbs[%d]: ", i);
			dump_txcb(desc);
		}
	}
*/
	// Read in values from the eeprom and set up values in sp
	setup_from_eeprom(sp);
	
	// Run a self test
	self_test(sp);

	// Reset the card
	writel(0, (uchar*)(sp->io_base + SCBPort));

	sp->last_tx_time = sp->last_rx_time = system_time();

	// set up semaphores
	sp->rx_sem = create_sem(0, "eepro100 Rx semaphore");
	sp->tx_sem = create_sem(TX_RING_SIZE - TX_RING_WASTED, "eepro100 Tx semaphore");
	sp->mutex = create_sem(1, "eepro100 transmit command list access mutex");
	sp->hw_lock = create_sem(1, "eepro100 SCB access mutex");
	sp->mcast_lock = create_sem(1, "eepro100 multicast setup frame lock");

	// Start handling interrupts (we shouldn't be getting any yet, though!)
	install_io_interrupt_handler(sp->irq, speedo_isr, sp, 0);

	// write stats block address
	wait_for_cmd_done(sp);
	writel(virt_to_bus(&sp->card_stats), (uchar*)(ioaddr + SCBPointer));
	writew(INT_MASK | CUStatsAddr, (uchar*)(ioaddr + SCBCmd));
	sp->card_stats.done_marker = 0;


	// Trickery to make the start_cmd and finish_cmd functions work.
	// We want to start our real commands with the second element in the
	// list so that the first call to finish_cmd doesn't erase the
	// suspend bit in the last command block.
	acquire_sem(sp->tx_sem); // reduce number of available blocks
	sp->cur_tx++; // start the current pointer at 1
	sp->last_tx_cmd = 0; // the last command was 0
	sp->cmd_unit_started = 0; // we haven't started the command unit yet


	// Write the configure command to the command list
	start_cmd(sp, &tx_entry, &cmd, &buf);
	cmd->status = CmdConfigure | CmdIntr | CmdSuspend;

	VDEBUG("eepro100: cmd->status = 0x%8.8x    ", cmd->status);
	if (debug_level > DEBUG_VDEBUG) printbits32(cmd->status);
	VDEBUG("\n");

	// in this case, ignore buf. we want to use the built-in data area
	buf = (uchar*)&(cmd->tx_desc_addr);
	
	// Copy the defaults into the command
	for (i = 0; i < 23; i++) {
		buf[i] = i82558_config_cmd[i];
	}
	
	// configuration values
	buf[1] = (txfifo << 4) | rxfifo; // Tx/Rx FIFO limits
	buf[4] = 0;
	buf[5] = 0;
	//buf[13] = 10;
	//buf[14] = 213;
	//buf[15] |= 1; // turn promiscuous mode on

	// Complete the configuration command
	finish_cmd(sp, tx_entry);

	// Write the station address to the command list
	start_cmd(sp, &tx_entry, &cmd, &buf);

	// again, ignore buf. we want to use the built-in data area
	buf = (uchar*)&(cmd->tx_desc_addr);
	
	cmd->status = CmdSuspend | CmdIASetup | CmdIntr;

	for (i = 0; i < 6; i++) {
		buf[i] = sp->myaddr.ebyte[i];
	}

	// Complete the IASetup command
	finish_cmd(sp, tx_entry);

		// debugging
//			if (debug_level > DEBUG_DEBUG) {
//				int i;
//				DEBUG("eepro100: Tx ring summary: TX_RING_SIZE=%d blocks\n", TX_RING_SIZE);
//				for (i = 0; i < TX_RING_SIZE; i++) {
//					TxCB *desc = &(sp->tx_base[i].desc);
//					DEBUG("tx_entry[%d].desc: ", i);
//					dump_txcb(desc);
//				}
//			}
		// end debugging

//	if (debug_level > DEBUG_VDEBUG) dump_csr(ioaddr);

	// start command unit and unmask interrupts
	wait_for_cmd_done(sp);
	// clear out the base address (we will use absolute addressing)
	writel(0, (uchar*)(ioaddr + SCBPointer));
	writew(CUCmdBase, (uchar*)(ioaddr + SCBCmd));
	wait_for_cmd_done(sp);

	// write the location of the first element in the command list
	writel(sp->tx_base[0].flags.pci_desc, (uchar*)(ioaddr + SCBPointer));

	// Go! Go! Gadget ethernet card!
	DEBUG("eepro100: starting command (Tx) unit\n");
	writew(CUStart, (uchar*)(ioaddr + SCBCmd));
	sp->cmd_unit_started = 1;

/*	VDEBUG("eepro100: Tx ring summary: TX_RING_SIZE=%d blocks\n", TX_RING_SIZE);
	if (debug_level < DEBUG_VDEBUG) {
		for (i = 0; i < TX_RING_SIZE; i++) {
			TxCB *desc = sp->tx_cbs[i];
			VDEBUG("tx_cbs[%d]: ", i);
			dump_txcb(desc);
		}
	}
*/	
	// write Rx addresses and start receiving
	wait_for_cmd_done(sp);
	writel(0, (uchar*)(ioaddr + SCBPointer));
	writew(RUCmdBase, (uchar*)(ioaddr + SCBCmd));
	wait_for_cmd_done(sp);

	writel(sp->rx_base[0].flags.pci_desc, (uchar*)(ioaddr + SCBPointer));
	DEBUG("eepro100: starting Rx unit\n");
	//dump_csr(ioaddr);
	writew(RUStart, (uchar*)(ioaddr + SCBCmd));
	sp->rx_unit_running = 1;
	//dump_csr(ioaddr);

	INFO("eepro100: /dev/%s opened successfully\n", name);
	return B_OK;
}


void dump_tx_desc(speedo_private *sp) {
		// debugging
		int32 sem_count, i;
		
		get_sem_count(sp->tx_sem, &sem_count);
		DEBUG("eepro100: sp->dirty_tx: %d, sp->cur_tx: %d, sp->tx_sem = %d\n",
			sp->dirty_tx, sp->cur_tx, sem_count);
		for (i = 0; i < TX_RING_SIZE; i++) {
			TxCB *desc = &(sp->tx_base[i].desc);
			dprintf("tx[%d]=%8.8x ",i, sp->tx_base[i].desc.status);
		}
		dprintf("\n");
}

// TODO: change this to resume quitely and not cause a write() error
static status_t
tx_timeout(speedo_private *sp)
{
	uchar *ioaddr = (uchar*)sp->io_base;
	uint16 status;
	
	status = readw(ioaddr + SCBStatus);
	ERR("eepro100: TxTimeOut status=%4.4x\n", status);
	if ((status & 0x00C0) != 0x0080) {
	
		// debugging
		int32 sem_count;
		get_sem_count(sp->tx_sem, &sem_count);
		DEBUG("eepro100: TX timeout sp->dirty_tx: %d, sp->cur_tx: %d, sp->tx_sem = %d\n",
			sp->dirty_tx, sp->cur_tx, sem_count);
		//dump_tx_desc(sp);
		// end debugging
		
		acquire_sem(sp->hw_lock);
		wait_for_cmd_done(sp);
		writel(sp->tx_base[sp->dirty_tx].flags.pci_desc, ioaddr + SCBPointer);
		writew(CUStart, ioaddr + SCBCmd);
		release_sem(sp->hw_lock);

	}

	return B_NO_ERROR;
}


// The next two functions must be called in pairs.
// The first gets a command block. The second executes the command. 
// Nobody else can use the command list until the second function is called. 

// start_cmd gets the caller a free TxCB and a transmit buffer,
// as well as the entry number
status_t
start_cmd(speedo_private *sp, int *tx_entry, TxCB **cmd, uchar **buf)
{
	status_t err;

//	DEBUG("eepro100: start_cmd()\n");

	// acquire the mutex, which guarantees that nobody else is modifying the list
	err = acquire_sem(sp->mutex);
	if (err < B_NO_ERROR) {
		ERR("eepro100: error acquiring transmit command list mutex!\n");
		return B_ERROR;
	}

	// acquire the tx_sem, which guarantees a free spot on the list
	err = acquire_sem_etc(sp->tx_sem, 1, B_RELATIVE_TIMEOUT, TX_TIMEOUT);
	// If we timed out, restart the transmitter and reacquire the tx_sem
	if (err == B_TIMED_OUT) {
		tx_timeout(sp); 	
		release_sem(sp->mutex);	
		return B_ERROR;
	}

	// set the next free entry in the list
	*tx_entry = sp->cur_tx;
	// wrap the counter
	sp->cur_tx = (sp->cur_tx + 1) & TX_RING_MASK;

	// Set pointer to descriptor
	*cmd = &(sp->tx_base[*tx_entry].desc);
	// Set the suspend bit (just to be sure)
	(*cmd)->status &= CmdSuspend;

	// Set pointer to packet buffer
	*buf = sp->tx_base[*tx_entry].packet.buf;

//	VDEBUG("eepro100: leaving start_cmd()\n");

	return B_NO_ERROR;
}

// Clear the suspend bit on the previous command and issue a CUResume
status_t
finish_cmd(speedo_private *sp, int current_entry)
{
	status_t err;
	TxCB *cmd;
	int prev_entry;
	
//	VDEBUG("eepro100: finish_cmd()\n");

	// clear suspend on previous command
//	cmd = &(sp->tx_base[sp->last_tx_cmd].desc);
	prev_entry = (current_entry == 0) ? (TX_RING_MASK) : (current_entry - 1);
	cmd = &(sp->tx_base[prev_entry].desc);
//	cmd->status &= ~CmdSuspend;
	sp->last_tx_cmd = current_entry;


	// If the command unit is running (it should be after open is called),
	// get exclusive access to the card and resume command unit.
	if (sp->cmd_unit_started) {
//		acquire_sem(sp->hw_lock);
		// wait for previous command to be accepted
		wait_for_cmd_done(sp);
		// Clear the Suspend bit and *immediately* Tell the command unit to resume processing
		{
			cpu_status  	former;
			// Disable interrupts - this isn't really synchronization,
			// because we can't sync the NIC and the host CPU, so we
			// just limit the amount of code executed between susp clear & resume
			// by disabling interrupts
			former = disable_interrupts();
			cmd->status &= ~CmdSuspend;
			writew(CUResume, (uchar*)(sp->io_base + SCBCmd));
	    	restore_interrupts(former);
		}
//		release_sem(sp->hw_lock);
	}

	release_sem(sp->mutex);

//	VDEBUG("eepro100: leaving finish_cmd()\n");
	
	return B_NO_ERROR;
}


////// Interrupt Service Routine
static int32
speedo_isr (void* cookie)
{
	speedo_private *sp = (speedo_private*)cookie;
	volatile uchar* ioaddr = (uchar*)(sp->io_base);
	int32 handled;
	int i;
	TxCB *cmd;
	int workload = 32; // don't do more than 32 things (what's a good value?)
	

	// Is this our interrupt?
	unsigned short status = readw((uchar*)(ioaddr + SCBStatus));
	if ((status & 0xfc00) == 0) {
		// it's not
		return B_UNHANDLED_INTERRUPT;
	}
	
	handled = B_HANDLED_INTERRUPT;

	// loop until status is clear or we do too much work
	while (workload-- > 0) {
		int recycle_cnt = 0;
		uint32 dirty_tx = sp->dirty_tx & TX_RING_MASK;

		// capture and acknowledge interrupts
		status = readw((uchar*)(ioaddr + SCBStatus));
		writew(0xfc00 & status, (uchar*)(ioaddr + SCBStatus));
//		IDEBUG("speedo_isr: status = 0x%4.4x    ", status);
//		if (debug_level > DEBUG_IDEBUG) printbits16(status);
//		IDEBUG("\n");

		if ((status & 0xfc00) == 0) {
			break;
			// There are no more interrupts to process.
		}


		// Check transmit blocks
		if (status & 0xA400) {  // CX/TNO | CNA/CI | SWI
			uint32  entry, frame_status;
			int i; 
			int j = 0; // detect infinite loop
				
			//	IDEBUG("speedo_isr: Checking for completed Tx blocks:\n");
		
			// count the number of tx buffers the chip has acknoweldged transmitting, and
			// signal the write_hook via semaphore those buffers are available again.
			
			while ((sp->cur_tx & TX_RING_MASK) - dirty_tx ) {   // not an actual count - because of wrap -- its a boolean
						
				//	IDEBUG("speedo_isr: tx_cbs[%d] ", i);
				//	if (debug_level > DEBUG_IDEBUG) dump_txcb(sp->tx_cbs[i]);
				//	IDEBUG("speedo_isr: tx_cbs[%d] ", (i + 1) & (TX_RING_SIZE - 1));
				//	if (debug_level > DEBUG_IDEBUG) dump_txcb(sp->tx_cbs[(i + 1) & (TX_RING_SIZE - 1)]);
		
				// check the complete bit		
				frame_status = sp->tx_base[dirty_tx].desc.status;
				if ((frame_status & StatusComplete) == 0) {
					
					if ((((TX_RING_MASK - ((sp->cur_tx & TX_RING_MASK) - dirty_tx)) & TX_RING_MASK) > 1 ) &&   // actual count more than 2 queued? 
						((sp->tx_base[(dirty_tx + 1) & TX_RING_MASK].desc.status & StatusComplete) == StatusComplete) // next one ok?
					   ){
						;
	//					ERR("eepro100: EEPRO forgot to mark one!!! dirty=%d\n", dirty_tx);
					
					} else {
						break; // card owns, not host
					};

				};
				if (frame_status & 0x1000) { dprintf("TX UNDERRUN adjust threshold\n"); } ;
		
				// check for errors
				// this will print if the NIC forgets to mark the StatusComplete bit
				// todo: gather stats on errors...
#if 0
				if ((frame_status & StatusOK) != StatusOK) {
					// the comand was completed, but there was an error
					ERR("eepro100: interrupt: error completing command %d\n", i);
				}
#endif
				
				recycle_cnt++;
				if (recycle_cnt > TX_RING_SIZE - TX_RING_WASTED)
					ERR("eepro100:TX ISR indexing problem");
					
				// clear out the status of the command (erase complete bit)
				// setting CmdSuspend here may cause the NIC to stop when we don't want it to.
//				sp->tx_base[dirty_tx].desc.status = CmdNop | CmdSuspend;
				sp->tx_base[dirty_tx].desc.status = CmdNop;

				//IDEBUG("eepro100: interrupt: released tx block %d\n", sp->dirty_tx);

				dirty_tx = (dirty_tx + 1) & TX_RING_MASK;
			}

			if (recycle_cnt) {
				sp->dirty_tx = dirty_tx;
				release_sem_etc(sp->tx_sem, recycle_cnt, B_DO_NOT_RESCHEDULE);
				handled = B_HANDLED_INTERRUPT;   // just reschedule for rx frames, not for reclaimed tx buffers
				sp->last_tx_time = system_time();
			}
		}
		// check for received frames
		if (status & 0x5000) {
			int recycle_cnt = 0;

			IDEBUG("eepro100: interrupt: frame received!\n");

			// find free blocks and release rx_sem so that read() can free them
			while (sp->rx_base[sp->card_owns_rx].desc.status & RxComplete) {
				// erase complete bit
				sp->rx_base[sp->card_owns_rx].desc.status &= ~RxComplete;
				// increment card_owns pointer
				sp->card_owns_rx = (sp->card_owns_rx + 1) & (RX_RING_SIZE - 1);
				recycle_cnt++;
			}
			IDEBUG("eepro100: interrupt: frame received: found %d packets\n", recycle_cnt);
			release_sem_etc(sp->rx_sem, recycle_cnt, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
			sp->last_rx_time = system_time();
		}
		
		if (status & StatMaskRNR) {
//			ERR("eepro100: interrupt: no receive resources!\n");
			sp->rx_unit_running = 0;
		}
		if (status & StatMaskMDI) {
//			IDEBUG("eepro100: interrupt: MDI interrupt!\n");
		}
//		if (status & StatMaskSWI) {
//			IDEBUG("eepro100: interrupt: software-generated interrupt\n");
//		}
		if (status & StatMaskFCP) {
//			IDEBUG("eepro100: interrupt: flow control pause!\n");
		}
	}

	if (workload < 0)
		ERR("eepro100: ISR: too much work at interrupt!\n");

	return handled; 
}	


/* ----------
	speedo_read - handle read() calls
----- */

static status_t
speedo_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	speedo_private *sp = (speedo_private*)cookie;
	int entry, last;
	int count;
	status_t err = B_NO_ERROR;
	RxFD *rxfd;
	int i;

//	VDEBUG("eepro100: entering read()\n");

	if (*num_bytes == 0)
		return B_OK;

//	VDEBUG("eepro100: Rx array summary:\n");
//	for (i = 0; i < RX_RING_SIZE; i++) {
//		VDEBUG("    rx_fds[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->rx_fds[i], virt_to_bus(sp->rx_fds[i]));
//		VDEBUG("rx_buffers[%d]: 0x%8.8x, virt_to_bus: 0x%8.8x\n", i, sp->rx_buffers[i], virt_to_bus(sp->rx_buffers[i]));
//		dump_rxfd(sp->rx_fds[i]);
//	}

	err = acquire_sem_etc(sp->rx_sem, 1, B_CAN_INTERRUPT | sp->blocking_flag, 0);
	if (err != B_NO_ERROR) {
//		ERR("eepro100: read() failed to acquire rx_sem!\n");
		*num_bytes = 0;
		return err;
	}
	
	// get the first available entry
	entry = (RX_RING_SIZE - 1) & atomic_add((long *)&sp->cur_rx, (int)1);
	atomic_and((long *)(&sp->cur_rx), (int)(RX_RING_SIZE - 1));
	rxfd = &(sp->rx_base[entry].desc);
	
	
	// get the data from the packet
	count = 0x3fff & rxfd->count;
	memcpy(buf, sp->rx_base[entry].packet.buf, count);
//	VDEBUG("eepro100: read(): copied %d bytes to buffer\n", count);
	*num_bytes = count;
	// reset count 
	rxfd->size = PACKET_BUF_SIZE;

	// Kick the receive unit
	if (! sp->rx_unit_running) {

		rxfd->status = CmdSuspend;
		// remove Suspend bit from last available
		sp->rx_base[sp->last_rx].desc.status &= ~(CmdSuspend);
		sp->last_rx = entry;
		// count
		sp->card_stats.rx_good_frames++;

		// restart the receiver
		acquire_sem(sp->hw_lock);
		wait_for_cmd_done(sp);
		writew(RUResume, (uchar*)(sp->io_base + SCBCmd));
		sp->rx_unit_running = 1;
		release_sem(sp->hw_lock);
	}
	else {
		rxfd->status = CmdSuspend;
		// remove Suspend bit from last available
		sp->rx_base[sp->last_rx].desc.status &= ~(CmdSuspend);
		sp->last_rx = entry;
		// count
		sp->card_stats.rx_good_frames++;
	}

//	VDEBUG("eepro100: leaving read()\n");
	return B_OK;
}


/* ----------
	speedo_write - handle write() calls
----- */

static status_t
speedo_write (void* cookie, off_t position, const void* pktbuf, size_t* num_bytes)
{
//	DEBUG("eepro100: entering write()\n");

	// return if 0 bytes to write
	if (*num_bytes == 0) {
		return B_OK;
	}
    {
		speedo_private *sp = (speedo_private*)cookie;
		volatile uchar *ioaddr = (uchar*)(sp->io_base);
		int tx_entry;
		TxCB *cmd;
		uchar *buf;
		status_t err;
		int32 sem_count;

		// Get a free TxCB
		err = start_cmd(sp, &tx_entry, &cmd, &buf);
		if (err != B_NO_ERROR) {
			ERR("eepro100: write(): start_cmd() failed! returning %d.\n", err);
			*num_bytes = 0;
			return err;
		}
//		VDEBUG("eepro100: write(): using command block %d\n", tx_entry);
	
		// Set up simple transmit command
		cmd->status = CmdTx | CmdSuspend;
		// Only set interrupts bits if we need to (if less than half of the transmit blocks are available)
//		if (get_sem_count(sp->tx_sem, &sem_count) < (TX_RING_SIZE / 2))
//			cmd->status |= CmdIntr;
		cmd->tx_desc_addr = 0xffffffff;

		// Set the transmit threshold, EOF bit, and number of bytes
		cmd->count =
			(0x20 << 16) /* tx threshold */ |
			(1 << 15) /* EOF */ |
			*num_bytes /* number of bytes to be transmitted */;
	
		// copy packet to the buffer
		memcpy(buf, pktbuf, *num_bytes);


		// debugging
//			if (debug_level > DEBUG_DEBUG) {
//				int i;
//				DEBUG("eepro100: Tx ring summary: TX_RING_SIZE=%d blocks\n", TX_RING_SIZE);
//				for (i = 0; i < TX_RING_SIZE; i++) {
//					TxCB *desc = &(sp->tx_base[i].desc);
//					DEBUG("tx_entry[%d].desc: ", i);
//					dump_txcb(desc);
//				}
//			}
		// end debugging


		// send the packet
		finish_cmd(sp, tx_entry);


		// debugging
//			if (debug_level > DEBUG_DEBUG) {
//				int i;
//				DEBUG("eepro100: Tx ring summary: TX_RING_SIZE=%d blocks\n", TX_RING_SIZE);
//				for (i = 0; i < TX_RING_SIZE; i++) {
//					TxCB *desc = &(sp->tx_base[i].desc);
//					DEBUG("tx_entry[%d].desc: ", i);
//					dump_txcb(desc);
//				}
//			}
		// end debugging

	}
//	DEBUG("eepro100: leaving write()\n");
	return B_OK;
}


// add multicast address
static status_t
speedo_add_multi(speedo_private *sp, ether_address_t *addr)
{
	uchar *buf;
	TxCB *cmd;
	int tx_entry, mentry;
	int i;

	if ((mentry = atomic_add((vint32*)&sp->mcast_count, 1)) >= 3) {
		ERR("eepro100: too many multicast entries!\n");
		sp->mcast_count--;
		return B_ERROR;
	}
	
	{ status_t err = start_cmd(sp, &tx_entry, &cmd, &buf);
		if (err) return err;
	}
	// add the address to the tail of the list
	memcpy(&sp->mcast[mentry], addr, sizeof(ether_address_t));		
	
	// set up the multicast frame
	cmd->status = CmdMulticastList | CmdSuspend;

	// ignore buf. we want to use the built-in data area
	buf = (uchar*)&(cmd->tx_desc_addr);

	*((uint16*)buf) = sp->mcast_count * 6;

	for (i = 0; i < sp->mcast_count * 6; i++) {
		((uchar*)cmd)[i + 10] = sp->mcast[i / 6].ebyte[i % 6];
	}

	finish_cmd(sp, tx_entry);
	
	return B_NO_ERROR;
}


// remove multicast address
static status_t
speedo_rem_multi(speedo_private *sp, ether_address_t *addr)
{
	// find the address
	
	// overwrite it with the last address
	
	// get the cmd block
	
	// fill in addresses, set links properly
	
	// do it


	return B_NO_ERROR;
}


// set promiscuous mode
static status_t
set_promisc(speedo_private *sp, bool on)
{
	uchar *buf;
	TxCB *cmd;
	int tx_entry;
	int i;

	// Write the configure command to the command list
	start_cmd(sp, &tx_entry, &cmd, &buf);
	cmd->status = CmdConfigure | CmdIntr | CmdSuspend;

	VDEBUG("eepro100: cmd->status = 0x%8.8x    ", cmd->status);
	if (debug_level > DEBUG_VDEBUG) printbits32(cmd->status);
	VDEBUG("\n");

	// in this case, ignore buf. we want to use the built-in data area
	buf = (uchar*)&(cmd->tx_desc_addr);
	
	// Copy the defaults into the command
	//memcpy(buf, i82558_config_cmd, sizeof(i82558_config_cmd));
	for (i = 0; i < 23; i++) {
		buf[i] = i82558_config_cmd[i];
	}
	
	// magical configuration values
	buf[1] = (txfifo << 4) | rxfifo; // Tx/Rx FIFO limits
	buf[4] = 0;
	buf[5] = 0;

	if (on) {
		buf[15] |= 1; // turn promiscuous mode on
	}
	else {
		// use default
	}
	
	finish_cmd(sp, tx_entry);

	return B_NO_ERROR;
}

/* ----------
	speedo_control - handle ioctl calls
----- */

static status_t
speedo_control (void* cookie, uint32 op, void* arg, size_t len)
{
	speedo_private *sp = (speedo_private*)cookie;
	struct {
		char bus;
		char device;
		char function;
	} params;
	
	switch (op) {
	case ETHER_GETADDR:
		memcpy(arg, &sp->myaddr, sizeof(ether_address_t));
		return (B_NO_ERROR);
	case ETHER_INIT:
		params.bus = sp->bus;
		params.device = sp->device;
		params.function = sp->function;
		memcpy(&params, arg, sizeof(params));
		return (B_NO_ERROR);
	case ETHER_NONBLOCK:
		if (*((int32*)arg))
			sp->blocking_flag = B_TIMEOUT;
		else
			sp->blocking_flag = 0;
		return (B_NO_ERROR);
	case ETHER_ADDMULTI: {
		ERR("eepro100: ETHER_ADDMULTI\n");
		return speedo_add_multi(sp, (ether_address_t*)arg);	
	}
	case ETHER_REMMULTI:							/* rem multicast addr */
		ERR("eepro100: unimplemented ioctl(): ETHER_REMMULTI\n");	
		break;
	case ETHER_SETPROMISC:						/* set promiscuous */
		INFO("eepro100: ETHER_SETPROMISC %s\n", *((int32 *)arg) ? "ON" : "OFF");	
		if( *((int32 *)arg) )
			set_promisc(sp, true);
		else 
			set_promisc(sp, false);
		return B_OK;
	case ETHER_GETFRAMESIZE:						/* get frame size */
		// set frame size to 1514
		*((unsigned*)arg) = 1514;
		break;
	default:
		ERR("eepro100: unknown ioctl(): op=%d\n", op);	
	}

	return B_ERROR;
}


/* ----------
	speedo_close - handle close() calls
----- */

static status_t
speedo_close (void* cookie)
{
	speedo_private *sp = (speedo_private *)cookie;
	status_t rv;
	
	// shut off interrupts and receive unit
	wait_for_cmd_done(sp);
	writew(INT_MASK, (uchar*)(sp->io_base + SCBCmd));
	wait_for_cmd_done(sp);
	writew(INT_MASK | RUAbort, (uchar*)(sp->io_base + SCBCmd));
	wait_for_cmd_done(sp); // verify command completion

	rv = remove_io_interrupt_handler(sp->irq, speedo_isr, sp);
	if (rv < B_NO_ERROR)
		ERR("eepro100: close(): remove_io_interrupt_handler() returned %d\n", rv);

	delete_sem(sp->tx_sem);
	delete_sem(sp->rx_sem);
	delete_sem(sp->mutex);
	
	INFO("eepro100: close()\n");

	return B_OK;
}


/* -----
	speedo_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
speedo_free (void* cookie)
{
	speedo_private *sp = (speedo_private*)cookie;

	// do this last
	uninit_ring_areas(sp);
	delete_area(sp->io_area);
	delete_area(sp->private_area);

	INFO("eepro100: free()\n");
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *speedo_name[] = {
	"net/eepro100/0",
	"net/eepro100/1",
	"net/eepro100/2",
	"net/eepro100/3",
	"net/eepro100/4",
	"net/eepro100/5",
	"net/eepro100/6",
	"net/eepro100/7",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */
int32 api_version = B_CUR_DRIVER_API_VERSION;

device_hooks speedo_hooks = {
	speedo_open, 			/* -> open entry point */
	speedo_close, 			/* -> close entry point */
	speedo_free,			/* -> free cookie */
	speedo_control, 		/* -> control entry point */
	speedo_read,			/* -> read entry point */
	speedo_write			/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	int i;
	// devices is declared at the top 
	devices = (char**)malloc(sizeof(char*) * card_cnt + 1);
	
	for (i = 0; i < card_cnt; i++)
		devices[i] = strdup(speedo_name[i]);
	devices[i] = 0;

	return (const char**)devices;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

static int
lookup_device_name(const char *name)
{
	int i;
	for (i = 0; speedo_name[i]; i++) {
		if (!strcmp(speedo_name[i], name))
			return i;
	}
	return -1; // invalid device name
}


device_hooks*
find_device(const char* name)
{
//	if (lookup_device_name(name) >= 0)
		return &speedo_hooks;
//	return NULL;
}


