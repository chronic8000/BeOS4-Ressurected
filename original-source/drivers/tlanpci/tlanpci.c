#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <stdarg.h>
#include <module.h>
#include <malloc.h>
#include <ByteOrder.h>

#ifdef __cplusplus
extern "C" {
#endif

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;

// ***
// Constants and Enums
// ***

// Ethernet ioctrl opcodes
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_TIMESTAMP,						/* set timestamp option; type int */
	ETHER_IOVEC,							/* report if iovecs are supported */
};

#if B_HOST_IS_LENDIAN
// NetSio Bits
enum
{
	MINTEN = 0x8000,
	ECLOK = 0x4000,
	ETXEN = 0x2000,
	EDATA = 0x1000,
	NMRST = 0x0800,
	MCLK = 0x0400,
	MTXEN = 0x0200,
	MDATA = 0x0100,
};

// HostCMD Bits
enum
{
	HCMD_GO = 0x80000000,
	HCMD_STOP = 0x40000000,
	HCMD_ACK = 0x20000000,
	HCMD_CH_SEL = 0x00200000,
	HCMD_EOC = 0x00100000,
	HCMD_RT = 0x00080000,
	HCMD_NES = 0x00040000,
	HCMD_AD_RST = 0x00008000,
	HCMD_LD_TMR = 0x00004000,
	HCMD_LD_THR = 0x00002000,
	HCMD_REQ_INT = 0x00001000,
	HCMD_INTS_OFF = 0x00000800,
	HCMD_INTS_ON = 0x00000400,
};

// NetConfig Bits
enum
{
	NCFG_RCLK_TEST = 0x8000,
	NCFG_TCLK_TEST = 0x4000,
	NCFG_BITRATE = 0x2000,
	NCFG_RX_CRC = 0x1000,
	NCFG_PEF = 0x0800,
	NCFG_ONE_FRAGMENT = 0x0400,
	NCFG_ONE_CHN = 0x0200,
	NCFG_MTEST = 0x0100,
	NCFG_PHY_EN = 0x0080,
};

#else // Big Endian
// NetSio Bits
enum
{
	MINTEN = 0x0080,
	ECLOK = 0x0040,
	ETXEN = 0x0020,
	EDATA = 0x0010,
	NMRST = 0x0008,
	MCLK = 0x0004,
	MTXEN = 0x0002,
	MDATA = 0x0001,
};

// HostCMD Bits
enum
{
	HCMD_GO = 0x00000080,
	HCMD_STOP = 0x00000040,
	HCMD_ACK = 0x00000020,
	HCMD_CH_SEL = 0x00002000,
	HCMD_EOC = 0x00001000,
	HCMD_RT = 0x00000800,
	HCMD_NES = 0x00000400,
	HCMD_AD_RST = 0x00800000,
	HCMD_LD_TMR = 0x00400000,
	HCMD_LD_THR = 0x00200000,
	HCMD_REQ_INT = 0x00100000,
	HCMD_INTS_OFF = 0x00080000,
	HCMD_INTS_ON = 0x00040000,
};

// NetConfig Bits
enum
{
	NCFG_RCLK_TEST = 0x0080,
	NCFG_TCLK_TEST = 0x0040,
	NCFG_BITRATE = 0x0020,
	NCFG_RX_CRC = 0x0010,
	NCFG_PEF = 0x0008,
	NCFG_ONE_FRAGMENT = 0x0004,
	NCFG_ONE_CHN = 0x0002,
	NCFG_MTEST = 0x0001,
	NCFG_PHY_EN = 0x8000,
};

#endif

// NetCmd
enum
{
	NCMD_NRESET = 0x80,
	NCMD_NWRAP = 0x40,
	NCMD_CSF = 0x20,
	NCMD_CAF = 0x10,
	NCMD_NOBRX = 0x08,
	NCMD_DUPLEX = 0x04,
	NCMD_TRFRAM = 0x02,
	NCMD_TXPACE = 0x01,
};

typedef enum
{
	MII_READ,
	MII_WRITE
}mii_action;

typedef enum 
{
	EEPROM_READ = 1,
	EEPROM_WRITE = 0
}eeprom_access;

#define TLAN_COMPAQ_ID              0x0E11
#define TLAN_NETELLIGENT_ID         0xAE32
#define TLAN_NETELLIGENT_DUAL_ID    0xAE40
#define TLAN_COMPAQ_ON_BOARD		0xAE35

#define MAX_TLAN_CARDS				4
#define DEVNAME_LENGTH				64

#define TX_LISTS					16 // 11
#define RX_LISTS					64 // 32
#define TX_LIST_SIZE				88L
#define RX_LIST_SIZE				16L
#define BUFFER_SIZE					1520L

static const char *kTLanName = "net/tlan/";

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

// ***
// TLan Types
// ***

// Transmit and Receive List Sturctures
typedef struct
{
	volatile uint32		data_count;
	void				*data_address;
}list_buffer;

typedef struct
{
	volatile void	*forward_ptr;
	volatile uint16	cstat;
	volatile uint16	frame_size;
	list_buffer		buffers[10];
}tlan_list;

typedef struct
{
	void			*next;
	void			*previous;
	tlan_list		*list;
	void			*listPhy;
	volatile void	*forward_ptr;
	void			*buffer;
	bigtime_t		timestamp;
}ring_element;

// Info struct used for each open device
typedef struct
{
	area_id			infoArea;
	int32			devID; // device identifier: 0-n
	pci_info		*pciInfo;
	fc_lock			ilock, olock;
	area_id			reg_area; // Area used for MMIO of hostRegs
	void			*reg_base; // Base address of hostRegs
	spinlock		netSioLock;
	spinlock		cmdLock;
	spinlock		tx_eoc_lock;
	volatile int32	blockFlag;
			
	// Host Registers
	volatile uint32	*host_cmd;
	volatile uint32	*ch_parm;
	volatile uint16	*host_int;
	volatile uint16	*dio_adr;
	volatile uint32	*dio_data32;
	volatile uint16	*dio_data16;
	volatile uint8 	*dio_data8;
	
	// MII PHY Stuff
	uint32			phyAdrMask;
	uint8			phyAdr;
	
	// MAC Address
	uint8			macAdr[6];
	
	// Lists & Buffers
	area_id			list_area;
	ring_element	*rx_ring;
	ring_element	*tx_ring;
	
	volatile ring_element *tx_now; // element to use when restarting tx/rx at eoc interupt time
	volatile ring_element *rx_now;
	
	// Control Flags
	volatile int32	rx_eoc;
	volatile int32	tx_eoc;
	volatile int32	readLock;
	volatile int32	writeLock;
	volatile int32	tx_pending;
	volatile int32	fullDuplex;
	
	// MII Cache
	uint16			ourAbility;
	uint16			partnerAbility;
	uint16			linkStatus;
	uint16			pad0;
	
	// Stats
	int32			s_txeof, s_txeoc, s_txframes, s_tx_pending_high, s_tx_start, s_eoc_start;
	int32			s_rxeoc, s_rxeof, s_rx_start;
	bigtime_t		rxt;
	int32			rxw;
}dev_info;

typedef int32 (*interrupt_vector)( dev_info *device, uint32 *ackCount );

// ***
// Driver Interface Prototypes
// ***

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

// Driver Entry Points
_EXPORT status_t init_hardware( void );
_EXPORT status_t init_driver( void );
_EXPORT void uninit_driver( void );
_EXPORT const char** publish_devices( void );
_EXPORT device_hooks *find_device( const char *name );

// Driver Hook Functions
static status_t open_hook( const char *name, uint32 flags, void **cookie );
static status_t close_hook( void *cookie );
static status_t free_hook( void *cookie );
static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length );
static status_t read_hook( void *cookie, off_t position, void *data, size_t *length );
static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length );
static status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes );

// ***
// TLan Interface Prototypes
// ***

int32 get_pci_list( pci_info *info[], int32 maxEntries ); // Get pci_info for each TLan device
status_t free_pci_list( pci_info *info[] ); // Free storage used by pci_info list
status_t setup_memory_io( dev_info *device ); // Setup memory mapped io for device

// Interrupt Handlers
int32 device_interrupt_handler( void *cookie ); // main handler: jump table dispatch
int32 invalid_interrupt( dev_info *device, uint32 *ackCount );
int32 tx_eof_interrupt( dev_info *device, uint32 *ackCount );
int32 statistics_interrupt( dev_info *device, uint32 *ackCount );
int32 rx_eof_interrupt( dev_info *device, uint32 *ackCount );
int32 dummy_interrupt( dev_info *device, uint32 *ackCount );
int32 tx_eoc_interrupt( dev_info *device, uint32 *ackCount );
int32 status_interrupt( dev_info *device, uint32 *ackCount );
int32 rx_eoc_interrupt( dev_info *device, uint32 *ackCount );

// MII Kernel Daemon
void mii_poll( void *arg, int i );

status_t reset_device( dev_info *device, bool wait );
void set_multicast( dev_info *device, uint8 *adr, bool state );

// Device Access Functions
status_t read_eeprom( dev_info *device, uint8 address, uint8 *data );
status_t access_mii( dev_info *device, mii_action action, uint8 mii_dev, uint8 address, uint16 *data );

// List Management Stuff
status_t allocate_lists( dev_info *device );
void init_list( volatile ring_element *e, bool end );

// ***
// Globals
// ***

// Device Hooks
static device_hooks gDeviceHooks = {
	open_hook, 			/* -> open entry point */
	close_hook, 		/* -> close entry point */
	free_hook,			/* -> free cookie */
	control_hook, 		/* -> control entry point */
	read_hook,			/* -> read entry point */
	write_hook,			/* -> write entry point */
	NULL,               /* -> select */
	NULL,               /* -> deselect */
	NULL,               /* -> readv */
	writev_hook         /* -> writev */
};

// Interrupt Jump Table
static interrupt_vector gIntJumpTable[8] = {
	invalid_interrupt,
	tx_eof_interrupt,
	statistics_interrupt,
	rx_eof_interrupt,
	dummy_interrupt,
	tx_eoc_interrupt,
	status_interrupt,
	rx_eoc_interrupt
};
static pci_module_info	*gPCIModInfo;
static char 			*gDevNameList[MAX_TLAN_CARDS+1];
static pci_info 		*gDevList[MAX_TLAN_CARDS+1];
static int32 			gOpenMask = 0;

// ***
// Driver Entry Points
// ***

status_t init_hardware( void )
{
	status_t		status;
	
	// Check for hardware with init_driver()
	if( (status = init_driver()) == B_OK )
		uninit_driver();
	return status;
}

status_t init_driver( void )
{
	status_t 		status;
	int32			entries;
	char			devName[DEVNAME_LENGTH];
	int32			i;
	
	dprintf( "TLan:  init_driver()\n" );
	// Setup pci_module_info
	if( (status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK )
	{
		dprintf( "TLan: Get module failed! %s\n", strerror( status ) );
		return status;
	}
	
	// Find TLan cards
	if( (entries = get_pci_list( gDevList, MAX_TLAN_CARDS )) == 0 )
	{
		dprintf( "TLan: TLan not found\n" );
		free_pci_list( gDevList );
		put_module( B_PCI_MODULE_NAME );
		return B_ERROR;
	}
	
	// Create device name list
	for( i=0; i<entries; i++ )
	{
		sprintf( devName, "%s%ld", kTLanName, i );
		gDevNameList[i] = (char *)malloc( strlen(devName)+1 );
		strcpy( gDevNameList[i], devName );
	}
	gDevNameList[i] = NULL;
	
	//dprintf( "TLan: Init Ok. Found %ld devices.\n", i );
	return B_OK;
}

void uninit_driver( void )
{
	int32 	i;
	void 	*item;
	
	dprintf( "TLan: uninit_driver()\n" );
	// Free device name list
	for( i=0; (item=gDevNameList[i]); i++ )
		free( item );
	
	// Free device list
	free_pci_list( gDevList );
	put_module( B_PCI_MODULE_NAME );
}

const char** publish_devices( void )
{
	//dprintf( "TLan: publish_devices()\n" );
	return (const char **)gDevNameList;
}

device_hooks *find_device( const char *name )
{
	int32 	i;
	char 	*item;
	
	//dprintf( "TLan: find_device() %s\n", name );
	// Find device name
	for( i=0; (item=gDevNameList[i]); i++ )
	{
		if( strcmp( name, item ) == 0 )
		{
			dprintf( "TLan: Found Device!\n" );
			return &gDeviceHooks;
		}
	}
	return NULL; // Device not found
}

// ***
// Driver Hook Functions
// ***

static status_t open_hook( const char *name, uint32 flags, void **cookie )
{
	int32				devID;
	int32				mask;
	status_t			status;
	char 				*devName;
	dev_info			*device;
	area_id				infoArea;
	
	//dprintf( "TLan: open_hook()\n" );
	// Find device name
	for( devID=0; (devName=gDevNameList[devID]); devID++ )
	{
		if( strcmp( name, devName ) == 0 )
			break;
	}
	if( !devName )
		return EINVAL;
	
	// Check if the device is busy and set in-use flag if not
	mask = 1 << devID;
	if( atomic_or( &gOpenMask, mask ) &mask )
		return B_BUSY;
	
	// Allocate storage for the cookie
	
	if( (*cookie = device = (dev_info *)memalign( 8, sizeof(dev_info) )) == NULL )
	{
		status = B_NO_MEMORY;
		goto err0;
	}
	
	// Setup the cookie
	device->pciInfo = gDevList[devID];
	device->devID = devID;
	
	// Setup Semaphores
	if( (status = create_fc_lock( &device->ilock, 0, "tlan_receive" )) < 0 )
		goto err1;
	
	if( (status = create_fc_lock( &device->olock, TX_LISTS, "tlan_transmit" )) < 0 )
		goto err2;
	

	device->netSioLock = 0;
	device->cmdLock = 0;
	device->readLock = 0;
	device->writeLock = 0;
	device->tx_eoc_lock = 0;
	device->rxt = 0;
	device->rxw = 0;
	
	// Init blocking IO
	device->blockFlag = 0;
	
	// Setup Memory Mapped IO
	if( (status = setup_memory_io( device )) != B_OK )
		goto err3;
	
	// Allocate Lists and Buffers
	if( allocate_lists( device ) != B_OK )
		goto err4;
	
	// Init Device
	reset_device( device, true );
	
	// Setup interrupts
	dprintf( "TLan on IRQ %d\n", device->pciInfo->u.h0.interrupt_line );
	install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, device_interrupt_handler, *cookie, 0 );
	
	// Start mii_poll deamon
	register_kernel_daemon( mii_poll, device, 50 );
	
	//dprintf( "TLan: open ok\n" );
	return B_OK; // We are ready to go!
	
	err4:
		delete_area( device->reg_area );
		delete_area( device->list_area );
		
	err3:
		delete_fc_lock( &device->olock );
	err2:
		delete_fc_lock( &device->ilock );
	err1:
		free( device );
		
	err0:
		atomic_and( &gOpenMask, ~mask );
		dprintf( "TLan: open failed!\n" );
		return status;
}

static status_t close_hook( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	
	dprintf( "TLan: close_hook()\n" );
	// Unblock IO
	// delete Semaphores
	delete_fc_lock( &device->ilock );
	delete_fc_lock( &device->olock );
	
	return B_OK;
}

static status_t free_hook( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	uint32				statistic, i;
	// ring_element		*ring;
	
	// Stop Transmit and Receive Channels; Disable adapter interrupts
	*device->host_cmd = HCMD_STOP;
	*device->host_cmd = HCMD_STOP | HCMD_RT;
	*device->host_cmd = HCMD_INTS_OFF;
	
	// Clear LED status
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0044); __eieio();
	*device->dio_data8 &= 0xFF; __eieio();
	
	// Display Statistics
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0030 | 0x8000); __eieio(); // Set to base address of stats and set address auto increment flag to true
	
	for( i=0; i<5; i++ ) // Read all stats
	{
		statistic = B_LENDIAN_TO_HOST_INT32(*device->dio_data32); __eieio();
		dprintf( "Statistic %ld: 0x%.8lX\n", i, statistic );
	}
	
	dprintf( "txframes: %ld txeof: %ld txeoc: %ld txpen_high: %ld tx_start: %ld \n tx_pending: %ld eoc_start: %ld rxeoc: %ld rxeof: %ld rx_restart: %ld\n", 
		device->s_txframes, device->s_txeof, device->s_txeoc, device->s_tx_pending_high, device->s_tx_start, device->tx_pending,
		device->s_eoc_start, device->s_rxeoc, device->s_rxeof, device->s_rx_start );
	
	// Remove MII Poll daemon
	unregister_kernel_daemon( mii_poll, device );
	
	// Remove Interrupt Handler
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, device_interrupt_handler, cookie );
	
	// Device is now available again
	atomic_and( &gOpenMask, ~(1 << device->devID) );
	
	// Free Areas
	delete_area( device->reg_area );
	delete_area( device->list_area );
	free( device );
	
	return B_OK;
}
	
static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length )
{
	dev_info			*device = (dev_info *)cookie;
	int32				i;
	
	switch( opcode )
	{
		case ETHER_GETADDR:
			//dprintf( "TLan: control_hook(): ETHER_GETADDR\n" );
			for( i=0; i<6; i++ )
				((uint8 *)data)[i] = device->macAdr[i];
			return B_OK;
		
		case ETHER_INIT:
			//dprintf( "TLan: control_hook(): ETHER_INIT\n" );
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			//dprintf( "TLan: control_hook(): ETHER_GETFRAMESIZE\n" );
			*(uint32 *)data = 1514;
			return B_OK;
			
		case ETHER_ADDMULTI:
			// dprintf( "TLan: control_hook(): ETHER_ADDMULTI\n" );
			set_multicast( device, (uint8 *)data, true );
			return B_OK;
			
		case ETHER_REMMULTI:
			// dprintf( "TLan: control_hook(): ETHER_REMMULTI\n" );
			set_multicast( device, (uint8 *)data, false );
			return B_OK;
			
		case ETHER_SETPROMISC:
			//dprintf( "TLan: control_hook(): ETHER_SETPROMISC\n" );
			*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
			if( *((int32 *)data) ) {
				*device->dio_data8 |= NCMD_CAF; __eieio(); }
			else {
				*device->dio_data8 &= ~NCMD_CAF; __eieio(); }
			return B_OK;
		
		case ETHER_NONBLOCK:
			// dprintf( "TLan: control_hook(): ETHER_NONBLOCK\n" );
			if( *((int32 *)data) )
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_OK;
		
		case ETHER_IOVEC:
			return B_OK;
		
		case ETHER_TIMESTAMP:
			
			return B_OK;
		default:
			//dprintf( "TLan: control_hook(): code=%ld\n", opcode );
			return B_ERROR;
	}
	
}

static status_t read_hook( void *cookie, off_t position, void *data, size_t *length )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	size_t				frame_size;
	int32				i;
	bool				localBroadcast = true;
	uint8				*frame;
	cpu_status			former;
	
	// dprintf( "Read %ld bytes requested\n", *length );
	
	do
	{
		// Block until data is available
		if( (status = fc_wait( &device->ilock, device->blockFlag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT )) != B_NO_ERROR )
		{
			*length = 0;
			return status;
		}
		
		// Restart the receiver if it has stopped
		if( (device->rx_eoc == true)&&(device->ilock.count == 0) )
		{
			ring_element *element = device->rx_ring;
			
			device->rx_eoc = false;
			
			// Find last element
			for( ; element->list->forward_ptr != NULL; element = (ring_element *)(element->next) ) {  }
			
			// The first element is the one following the last element
			element = (ring_element *)(element->next);
			
			// Sync the rx_ring
			device->rx_ring = element;
			device->rx_now = element;
			
			// Send rx go command
			former = disable_interrupts();
			acquire_spinlock( &device->cmdLock );
			*device->ch_parm = (uint32)element->listPhy;
			*device->host_cmd = HCMD_GO | HCMD_RT;
			release_spinlock( &device->cmdLock );
			device->s_rx_start++;
			restore_interrupts(former);
			continue;
		}
		
		// Check if it is our own broadcast
		frame = ((uint8 *)device->rx_ring->buffer) + 6;
		for( i=5; i>0; i-- )
		{
			if( frame[i] != device->macAdr[i] )
			{
				localBroadcast = false;
				break;
			}
		}
		
		if( !localBroadcast )
		{
			// Get frame size
			frame_size = B_LENDIAN_TO_HOST_INT16(device->rx_ring->list->frame_size);
			
			// dprintf( "%ld byte frame read\n", frame_size );
			
			if( frame_size > *length )
				frame_size = *length;
			
			memcpy( data, device->rx_ring->buffer, frame_size );
			
			if( *length >= frame_size + sizeof(bigtime_t) )
				*((bigtime_t *)(((uint8 *)data) + frame_size)) = device->rx_ring->timestamp;
			*length = frame_size;
		}
		
		// Reset this list and mark it as the end
		init_list( device->rx_ring, true );
		
		// Update the forward pointer of previous element to point to this one
		((ring_element *)device->rx_ring->previous)->list->forward_ptr = device->rx_ring->listPhy;
		
		// Advance to next element
		device->rx_ring = (ring_element *)device->rx_ring->next;
	}while( localBroadcast );
	// LED Ack Clear
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0044); __eieio();
	*device->dio_data8 &= ~0x10; __eieio();
	
	return B_OK;
}

static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	size_t				frame_size;
	cpu_status			former;
	
	// Is link ready to use?
	/*if( !(device->linkStatus & 0x0004) )
	{
		snooze( 10000 );
		*length = 0;
		return B_ERROR;
	}*/
	
	if( *length > BUFFER_SIZE )
		*length = BUFFER_SIZE;
	frame_size = *length;
	
	if( (status = fc_wait( &device->olock, B_INFINITE_TIMEOUT )) != B_NO_ERROR )
	{
		*length = 0;
		return status;
	}
	
	device->s_txframes++;
	
	// Protect againsts reentrant write
	/*if( atomic_or( &device->writeLock, 1 ) )
	{
		release_sem_etc( device->olock, 1, 0 );
		*length = 0;
		return B_ERROR;
	}*/
		
	// Copy data to buffer
	memcpy( device->tx_ring->buffer, data, frame_size );
	
	// Reset this list and mark it as the end
	device->tx_ring->list->forward_ptr = NULL;
	device->tx_ring->list->cstat = B_HOST_TO_LENDIAN_INT16(0x3000);
	
	// Set the frame size
	if( frame_size < 60 )
		frame_size = 60;
	
	// dprintf( "write %ld byte frame\n", frame_size );
	device->tx_ring->list->frame_size = B_HOST_TO_LENDIAN_INT16(frame_size); 
	device->tx_ring->list->buffers[0].data_count = B_HOST_TO_LENDIAN_INT32(frame_size | 0x80000000); // 0x80000000 is last fragment flag
	
	// Protect againsts tx_eoc interrupt
	former = disable_interrupts();
	acquire_spinlock( &device->tx_eoc_lock );
	
	device->tx_pending++;
	if( device->tx_pending > device->s_tx_pending_high )
		device->s_tx_pending_high = device->tx_pending;
	
	// Update the forward pointer of previous element to point to this one
	((ring_element *)device->tx_ring->previous)->list->forward_ptr = device->tx_ring->listPhy;
	
	// Start Transmission
	if( atomic_and( &device->tx_eoc, 0 ) )
	{
		device->s_tx_start++;
		device->tx_now = device->tx_ring;
		acquire_spinlock( &device->cmdLock );
		*device->ch_parm = (uint32)device->tx_ring->listPhy;
		*device->host_cmd = HCMD_GO;
		release_spinlock( &device->cmdLock );
	}
		
	// Advance to next element
	device->tx_ring = (ring_element *)device->tx_ring->next;
	
	// tx_eoc ok now
	release_spinlock( &device->tx_eoc_lock );
	restore_interrupts(former);
	
	// Another write may now take place
	device->writeLock = 0;
	
	return B_OK;
}

static status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *length )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	size_t				frame_size;
	cpu_status			former;
	off_t 				offset;
	const iovec			*endVec;
	
	// Is link ready to use?
	/*
	if( !(device->linkStatus & 0x0004) )
	{
		snooze( 10000 );
		*length = 0;
		return B_ERROR;
	}
	*/
	
	if( (status = fc_wait( &device->olock, B_INFINITE_TIMEOUT )) != B_NO_ERROR )
	{
		*length = 0;
		return status;
	}
	
	device->s_txframes++;
	
	// Protect againsts reentrant write
	/*if( atomic_or( &device->writeLock, 1 ) )
	{
		release_sem_etc( device->olock, 1, 0 );
		*length = 0;
		return B_ERROR;
	}*/
		
	// Copy data to buffer
	for( endVec = vec + count, offset=0; vec != endVec; offset += vec->iov_len, vec++ )
		memcpy( ((uint8 *)(device->tx_ring->buffer))+offset, vec->iov_base, vec->iov_len );
	
	*length = offset;
	if( *length > BUFFER_SIZE )
		*length = BUFFER_SIZE;
	frame_size = *length;
	
	// Reset this list and mark it as the end
	device->tx_ring->list->forward_ptr = NULL;
	device->tx_ring->list->cstat = B_HOST_TO_LENDIAN_INT16(0x3000);
	
	// Set the frame size
	if( frame_size < 60 )
		frame_size = 60;
	
	// dprintf( "write %ld byte frame\n", frame_size );
	device->tx_ring->list->frame_size = B_HOST_TO_LENDIAN_INT16(frame_size); 
	device->tx_ring->list->buffers[0].data_count = B_HOST_TO_LENDIAN_INT32(frame_size | 0x80000000); // 0x80000000 is last fragment flag
	
	// Protect againsts tx_eoc interrupt
	former = disable_interrupts();
	acquire_spinlock( &device->tx_eoc_lock );
	
	device->tx_pending++;
	if( device->tx_pending > device->s_tx_pending_high )
		device->s_tx_pending_high = device->tx_pending;
	
	// Update the forward pointer of previous element to point to this one
	((ring_element *)device->tx_ring->previous)->list->forward_ptr = device->tx_ring->listPhy;
	
	// Start Transmission
	
	if( device->tx_eoc )
	{
		device->tx_eoc = 0;
		device->s_tx_start++;
		device->tx_now = device->tx_ring;
		//acquire_spinlock( &device->cmdLock );
		*device->ch_parm = (uint32)device->tx_ring->listPhy;
		*device->host_cmd = HCMD_GO;
		//release_spinlock( &device->cmdLock );
	}
	// Advance to next element
	device->tx_ring = (ring_element *)device->tx_ring->next;
	
	// tx_eoc ok now
	release_spinlock( &device->tx_eoc_lock );
	restore_interrupts(former);
	
	// Another write may now take place
	device->writeLock = 0;
	
	return B_OK;
}

// ***
// Thunder Lan Functions
// ***

int32 get_pci_list( pci_info *info[], int32 maxEntries )
{
	status_t status;
	int32 i, entries;
	pci_info	*item;
	
	item = (pci_info *)malloc( sizeof( pci_info ) );
	
	for( i=0, entries=0; entries<maxEntries; i++ )
	{
		if( (status = gPCIModInfo->get_nth_pci_info( i, item )) != B_OK )
			break;
		if( (item->vendor_id == TLAN_COMPAQ_ID)&& 
		((item->device_id == TLAN_NETELLIGENT_ID)||(item->device_id == TLAN_NETELLIGENT_DUAL_ID)||
		(item->device_id == TLAN_COMPAQ_ON_BOARD)))
		{
			info[entries++] = item;
			item = (pci_info *)malloc( sizeof( pci_info ) );
		}
	}
	info[entries] = NULL;
	free( item );
	return entries;
}

status_t free_pci_list( pci_info *info[] )
{
	int32 		i;
	pci_info	*item;
	
	for( i=0; (item=info[i]); i++ )
		free( item );
	return B_OK;
}

// ***
// Interrupt Handlers
// ***

int32 device_interrupt_handler( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	uint32				intType = (uint32)B_LENDIAN_TO_HOST_INT16(*device->host_int) & 0x0000001F;
	//static uint32		last_int = 0;
	//static bigtime_t 	last = 0;
	//static int32		i = 0;
	uint32				ackCount = 0;
	int32				status;
	
	// Dispatch interrupt and ack if it was handled
	if( (status = gIntJumpTable[intType >> 2]( device, &ackCount )) != B_UNHANDLED_INTERRUPT )
	{
		*device->host_cmd = B_HOST_TO_LENDIAN_INT32(intType << 16) | HCMD_ACK | B_HOST_TO_LENDIAN_INT32((uint32)ackCount);
		//last_int = intType >> 2;
		//last = system_time();
		//i++;
	}
	else
	{
		//dprintf( "bogus interrupt: now = %ld, last = %ld, delta = %Ld us, i = %ld\n", (intType = (uint32)B_LENDIAN_TO_HOST_INT16(*device->host_int) & 0x0000001F) >> 2, last_int, system_time() - last, i );
		//i = 0;
		return B_HANDLED_INTERRUPT;
	}
	
	return status;
}

//B_HANDLED_INTERRUPT
//B_INVOKE_SCHEDULER
//B_UNHANDLED_INTERRUPT

int32 invalid_interrupt( dev_info *device, uint32 *ackCount )
{
	//dprintf( "Tlan: invalid_interrupt\n" );
	return B_UNHANDLED_INTERRUPT;
}

int32 tx_eof_interrupt( dev_info *device, uint32 *ackCount )
{
	int32		i=0;
	
	acquire_spinlock( &device->tx_eoc_lock );
	// dprintf( "Tlan: tx_eof_interrupt\n" );
	while( B_LENDIAN_TO_HOST_INT16(device->tx_now->list->cstat) & 0x4000 )
	{
		device->tx_now->list->cstat = 0;
		device->tx_now = device->tx_now->next;
		i++;
	}
	device->s_txeof++;
	device->tx_pending -= i;
	release_spinlock( &device->tx_eoc_lock );
	
	*ackCount = i;
	if( fc_signal( &device->olock, i, B_DO_NOT_RESCHEDULE ) )
		return B_INVOKE_SCHEDULER;
	else
		return B_HANDLED_INTERRUPT;
}

int32 statistics_interrupt( dev_info *device, uint32 *ackCount )
{
	uint32		i, statistic;
	//dprintf( "Tlan: statistics_interrupt\n" );
	// Clear Statistics
	// Reading the value of a statistic will clear it
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0030 | 0x8000); __eieio(); // Set to base address of stats and set address auto increment flag to true
	
	for( i=0; i<5; i++ ) // Read all stats
	{
		statistic = B_LENDIAN_TO_HOST_INT32(*device->dio_data32); __eieio();
		// dprintf( "Statistic %ld: 0x%.8lX\n", i, statistic );
	}
	
	return B_HANDLED_INTERRUPT;
}

int32 rx_eof_interrupt( dev_info *device, uint32 *ackCount )
{
	bigtime_t	now;
	int32		dt;
	//dprintf( "Tlan: rx_eof_interrupt\n" );
	
	// Set LED Ack
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0044); __eieio();
	*device->dio_data8 |= 0x10; __eieio();
	
	device->s_rxeof++;
	*ackCount = 1;
	
	now = real_time_clock_usecs();
	device->rx_now->timestamp = now;
	device->rx_now = device->rx_now->next;
	
	dt = now - device->rxt;
	device->rxt = now;
	
	if( (device->rxw |= fc_signal( &device->ilock, 1, B_DO_NOT_RESCHEDULE )) != 0 )
	{
		
		if( device->ilock.count >= 4 || dt > 240 )
		{
			device->rxw = 0;
			return B_INVOKE_SCHEDULER;
		}
		else
			return B_HANDLED_INTERRUPT;
	}
	else
		return B_HANDLED_INTERRUPT; // B_HANDLED_INTERRUPT
}

int32 dummy_interrupt( dev_info *device, uint32 *ackCount )
{
	dprintf( "Tlan: dummy_interrupt\n" );
	return B_HANDLED_INTERRUPT;
}

int32 tx_eoc_interrupt( dev_info *device, uint32 *ackCount )
{
	// dprintf( "Tlan: tx_eoc_interrupt\n" );
	
	device->s_txeoc++;
	acquire_spinlock( &device->tx_eoc_lock );
	
	// Has the last frame been transmitted?
	if( device->tx_pending )
	{
		device->s_eoc_start++;
		*device->ch_parm = (uint32)device->tx_now->listPhy;
		*ackCount = B_HOST_TO_LENDIAN_INT32(HCMD_GO);
	}
	else
		device->tx_eoc = true;
	release_spinlock( &device->tx_eoc_lock );
	return B_HANDLED_INTERRUPT;
}

int32 status_interrupt( dev_info *device, uint32 *ackCount )
{
	uint16				netSts;
	volatile uint16		*dio_high_data16 = (uint16 *)((uint8 *)(device->reg_base)+14);
	uint16				control, status, tl_status;
	int32				thread_count;
	int32				i, statistic;
	
	if( *device->host_int & B_HOST_TO_LENDIAN_INT16(0x1FE0) )
	{
		dprintf( "Tlan: adapter_check_interrupt: 0x%.8lX\n", B_LENDIAN_TO_HOST_INT32(*device->ch_parm) );
		// Display Statistics
		*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0030 | 0x8000); __eieio(); // Set to base address of stats and set address auto increment flag to true
		for( i=0; i<5; i++ ) // Read all stats
		{
			statistic = B_LENDIAN_TO_HOST_INT32(*device->dio_data32); __eieio();
			dprintf( "Statistic %ld: 0x%.8lX\n", i, statistic );
		}
		dprintf( "txframes: %ld txeof: %ld eoc: %ld txpen_high: %ld\n", device->s_txframes, device->s_txeof, device->s_txeoc, device->s_tx_pending_high );
		reset_device( device, false );
		
		// Reset semaphores
		device->ilock.count = 0;
		device->olock.count = TX_LISTS;
	}
	else
	{
		// Clear interrupt
		*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
		netSts = *dio_high_data16; __eieio();
		*dio_high_data16 &= 0x80FF; __eieio();
		
		//dprintf( "Tlan: status_interrupt\n" );
		
		if( netSts & 0x0080 ) // Was it a MII interrupt?
		{
			// Clear MII interrupt by reading generic and tlan specific status registers
			access_mii( device, MII_READ, device->phyAdr, 0x00, &control );
			access_mii( device, MII_READ, device->phyAdr, 0x01, &status );
			access_mii( device, MII_READ, device->phyAdr, 0x12, &tl_status );
			
			dprintf( "Tlan: MII Interrupt: 0x%.4X:0x%.4X:0x%.4X\n", control, status, tl_status );
			
			// If link is down, start auto-negotiation
			if( (status & 0x0024) == 0x0020 )
			{
				// Enable auto-negotiation
				access_mii( device, MII_READ, device->phyAdr, 0x00, &control );
				control |= 0x1200;
				access_mii( device, MII_WRITE, device->phyAdr, 0x00, &control );
			}
		}
	}
	return B_HANDLED_INTERRUPT;
}

int32 rx_eoc_interrupt( dev_info *device, uint32 *ackCount )
{
	//dprintf( "Tlan: rx_eoc_interrupt\n" );
	device->s_rxeoc++;
	device->rx_eoc = true;
	fc_signal( &device->ilock, 1, B_DO_NOT_RESCHEDULE );
	return B_HANDLED_INTERRUPT;
}

void mii_poll( void *cookie, int i )
{
	dev_info		*device = (dev_info *)cookie;
	uint16			partner, status, data;
	
	access_mii( device, MII_READ, device->phyAdr, 0x01, &status );
	/*dprintf( "MII Status: 0x%.4x\n", status );
	
	access_mii( device, MII_READ, device->phyAdr, 0x12, &data );
	dprintf( "MII 0x12: 0x%.4x\n", data );
	
	access_mii( device, MII_READ, device->phyAdr, 0x13, &data );
	dprintf( "MII 0x13: 0x%.4x\n", data );
	
	access_mii( device, MII_READ, device->phyAdr, 0x15, &data );
	dprintf( "MII 0x15: 0x%.4x\n", data );
	
	access_mii( device, MII_READ, device->phyAdr, 0x17, &data );
	dprintf( "MII 0x17: 0x%.4x\n", data );
	
	access_mii( device, MII_READ, device->phyAdr, 0x18, &data );
	dprintf( "MII 0x18: 0x%.4x\n", data );
	
	access_mii( device, MII_READ, device->phyAdr, 0x19, &data );
	dprintf( "MII 0x19: 0x%.4x\n", data );*/
	
	// If link status has changed since the last time we checked
	if( (device->linkStatus & 0x0004) != (status & 0x0004) )
	{
		// If link is up and autonegotiation is complete
		if( status & 0x0004 )
		{
			dprintf( "TLan: Link is up and ready to go...\n" );
			access_mii( device, MII_READ, device->phyAdr, 0x05, &partner );
			if( partner != device->partnerAbility )
			{
				device->partnerAbility = partner;
				
				if( partner && !device->fullDuplex && (((device->ourAbility & 0x4000)&&(partner & 0x0100))||((device->ourAbility & 0x1000)&&(partner & 0x0040)) ) )
				{
					// Enable full duplex if partner info and we support it
					*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
					*device->dio_data16 |= B_HOST_TO_LENDIAN_INT16(0x0004); __eieio();
					dprintf( "Partner info: Full duplex enabled\n" );
					device->fullDuplex = true;
				}
				else if( device->fullDuplex )
				{
					// Clear full duplex if no partner info
					*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
					*device->dio_data16 &= B_HOST_TO_LENDIAN_INT16(~0x0004); __eieio();
					dprintf( "No partner info: Full duplex cleared\n" );
					device->fullDuplex = false;
				}
			}
		}
		else
			dprintf( "TLan: Link is down\n" );
		device->linkStatus = status;
	}
}


status_t reset_device( dev_info *device, bool wait )
{
	int32		i;
	int32		statistic;
	uint8		data, adr;
	uint16		phyData;
	bigtime_t	t;
	
	// Turn on I/O port decode, Memory Address Decode, and Bus Mastering
	gPCIModInfo->write_pci_config( device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, 0x04, 2, 0x0001 | 0x0002 | 0x0004 );
	// Set PCI Bus Latency Timer to 0xFF
	gPCIModInfo->write_pci_config( device->pciInfo->bus, device->pciInfo->device, 
		device->pciInfo->function, 0x0D, 1, 0xFF );
	
	// **** #1 ****
	// Clear Statistics
	// Reading the value of a statistic will clear it
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0030 | 0x8000); __eieio();// Set to base address of stats and set address auto increment flag to true
	
	for( i=0; i<5; i++ ) // Read all stats
		statistic = B_LENDIAN_TO_HOST_INT32(*device->dio_data32); __eieio();
	
	// **** #2 ****
	// Issue reset command
	*device->host_cmd = HCMD_AD_RST; __eieio();
	//*device->host_cmd = HCMD_AD_RST | HCMD_INTS_ON | HCMD_INTS_OFF; __eieio();
	__eieio();
		
	// **** #3 ****
	// Disable interrupts
	*device->host_cmd = HCMD_INTS_OFF; __eieio();
	__eieio();
	
	// **** #4 ****
	// Load Mac Address from EEPROM
	dprintf( "MAC Adr: 0x" );
	for( i=0; i<6; i++ )
	{
		read_eeprom( device, 0x83+i, device->macAdr+i );
		dprintf( "%.2X", (int16)device->macAdr[i] );
	}
	dprintf( "\n" );
	
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0010 | 0x8000); __eieio(); // Auto address increment enabled
	// Zero address registers and the HASH registers
	for( i=0; i<8; i++ ) {
		*device->dio_data32 = 0; __eieio(); }
	
	// **** #5 ****
	// Set NetConfig Reg
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0004); __eieio();
	*device->dio_data16 = NCFG_PHY_EN | NCFG_ONE_CHN | NCFG_ONE_FRAGMENT; __eieio();
	
	// **** #6 ****
	// Set burst size in the BSIZE reg
	read_eeprom( device, 0x71, &data );
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0044); __eieio();
	*device->dio_data8 = data; __eieio();
	
	// **** #7 ****
	// Load the Tx commit level in the Acommit register
	//read_eeprom( device, 0x70, &data );
	data = 0;
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0040); __eieio();
	*device->dio_data32 = (*device->dio_data32 & B_HOST_TO_LENDIAN_INT32(0x00FFFFFF)) | B_HOST_TO_LENDIAN_INT32((uint32)data << 24); __eieio();
	
	// **** #8 ****
	// Load interrupt paceing timer in Ld_Tmr in HOST_CMD
	read_eeprom( device, 0x73, &data );
	*device->host_cmd = HCMD_LD_TMR | B_HOST_TO_LENDIAN_INT32((uint32)data);
	// *device->host_cmd = HCMD_LD_TMR | B_HOST_TO_LENDIAN_INT32(1);
	
	// **** #9 ****
	// Load the appropriate Tx Threshold value in Ld_Thr in HOST_CMD
	*device->host_cmd = HCMD_LD_THR | B_HOST_TO_LENDIAN_INT32((uint32)4); // Threshold = 1
	// *device->host_cmd = HCMD_LD_THR; // Threshold = 0
	
	// **** #10 ****
	// Unreset the MII by asserting NMRST in NetSio
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
	*device->dio_data16 |= NMRST; __eieio();
	
	if( wait )
		snooze( 50000 );
	
	// **** #11 ****
	// Init the PHY Layer
	// Look for MII PHY Interfaces
	device->phyAdrMask = 0;
	device->phyAdr = 0xFF;
	for( adr = 0x00; adr < 32; adr++ )
	{
		if( access_mii( device, MII_READ, adr, 0x00, &phyData ) == B_OK )
		{
			dprintf( "PHY At: 0x%.2X\n", (int16)adr );
			device->phyAdrMask |= 1 << adr;
		}
	}
	
	// Reset the first device and disable other devices
	for( i=0; i<32; i++ )
	{
		if( device->phyAdrMask & (1L<<i) )
		{
			// Is it the first device
			if( device->phyAdr == 0xFF )
			{
				// Reset Device
				dprintf( "Reset PHY: 0x%.2X\n", (int16)i );
				device->phyAdr = i;
				phyData = 0x8000;
				access_mii( device, MII_WRITE, device->phyAdr, 0x00, &phyData );
				
				t = system_time();
				do{
					access_mii( device, MII_READ, device->phyAdr, 0x00, &phyData );
				}while( (phyData & 0x8000)&&((system_time()-t)<50000) ); // Wait for reset to complete... but not too long
				
				// Enable auto-negotiation
				phyData |= 0x1200;
				access_mii( device, MII_WRITE, device->phyAdr, 0x00, &phyData );
				
				// Display Basic Control Register
				access_mii( device, MII_READ, device->phyAdr, 0x00, &phyData );
				dprintf( "PHY 0x00: 0x%.4X\n", phyData );
				
				// Get Initial Value of Basic Status Register
				access_mii( device, MII_READ, device->phyAdr, 0x01, &device->ourAbility );
				dprintf( "PHY 0x01: 0x%.4X\n", device->ourAbility );
				device->linkStatus = 0;
				device->partnerAbility = 0;
				
				// Force Carrier Integrity Monitor Disable
				access_mii( device, MII_READ, device->phyAdr, 0x17, &phyData );
				dprintf( "MII 0x17: 0x%.4x\n", phyData );
				phyData |= 0x0020;
				access_mii( device, MII_WRITE, device->phyAdr, 0x17, &phyData );
				
			}
			else
			{
				// Powerdown and isolate device
				dprintf( "Disable PHY: 0x%.2X\n", (int16)i );
				phyData = 0x0C00;
				access_mii( device, MII_WRITE, i, 0x00, &phyData );
			}
		}
	}
	
	// Setup Network Command Register
	// Bring out of reset, disable wrap, set half-duplex, enable transmit pacing
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
	*device->dio_data8 = NCMD_NRESET | NCMD_NWRAP | NCMD_TXPACE; __eieio();
	device->fullDuplex = false;
	
	// Setup Maximum Rx Size
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0044);
	*(device->dio_data16+1) = B_HOST_TO_LENDIAN_INT16(1518);
	//dprintf( "MTU Set to : %hd\n", *(device->dio_data16+1) );
	
	// Enable Receive all frames
	// *device->dio_data8 |= 0x10;
	
	// **** #12 ****
	// Setup NetMask Reg
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
	*device->dio_data32 &= B_HOST_TO_LENDIAN_INT32(0x00FFFFFF);
	//*device->dio_data32 |= B_HOST_TO_LENDIAN_INT32(0x80000000); // MII Interrupts
	
	// Enable MII Interrupts
	//*device->dio_adr = 0x0000; __eieio();
	//*device->dio_data16 &= ~MDATA;  __eieio();
	//*device->dio_data16 |= MINTEN; __eieio();
	
	// **** #13 ****
	// Enable interrupts
	*device->host_cmd = HCMD_INTS_ON;
	__eieio();
	
	// Clear Host Command
	*device->host_cmd = 0;
	
	// Test for Vendor ID ( Debug )
	/**device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0008); __eieio();
	dprintf( "Device ID: 0x%.8lx\n", *device->dio_data32 );
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x000c); __eieio();
	dprintf( "Vendor ID: 0x%.8lx\n", *device->dio_data32 );*/
	
	// Create Dummy Test Interrupt
	//*device->host_cmd = HCMD_REQ_INT;
	
	// Set Mac Address in general address registers
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0010 | 0x8000); __eieio(); // Auto address increment enabled
	#if B_HOST_IS_LENDIAN
	*device->dio_data32 = (((uint32)device->macAdr[3]) << 24) | (((uint32)device->macAdr[2]) << 16) | (((uint32)device->macAdr[1]) << 8) | (uint32)device->macAdr[0]; __eieio();
	*device->dio_data16 = (((uint32)device->macAdr[5]) << 8) | (uint32)device->macAdr[4]; __eieio();
	#else
	*device->dio_data32 = (((uint32)device->macAdr[0]) << 24) | (((uint32)device->macAdr[1]) << 16) | (((uint32)device->macAdr[2]) << 8) | (uint32)device->macAdr[3]; __eieio();
	*device->dio_data16 = (((uint32)device->macAdr[4]) << 8) | (uint32)device->macAdr[5]; __eieio();
	#endif
	
	// Start Receive
	device->rx_eoc = false;
	device->tx_eoc = true;
	device->s_tx_start = device->s_eoc_start = device->tx_pending = 0;
	device->s_txeof = device->s_txeoc = device->s_tx_pending_high = device->s_txframes = 0;
	device->s_rxeoc = device->s_rxeof = device->s_rx_start = 0;
	device->rx_now = device->rx_ring;
	*device->ch_parm = (uint32)device->rx_ring->listPhy;
	*device->host_cmd = HCMD_GO | HCMD_RT;
	
	return B_OK;
}

// ***
// Thunder Lan Setup Functions
// ***

status_t setup_memory_io( dev_info *device )
{
	int32		base, size, offset, i;
	
	base = device->pciInfo->u.h0.base_registers[1];
	size = device->pciInfo->u.h0.base_register_sizes[1];
	
	// Round down to nearest page boundary
	base = base & ~(B_PAGE_SIZE-1);
	
	// Adjust the size
	offset = device->pciInfo->u.h0.base_registers[1] - base;
	size += offset;
	size = (size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	
	if( (device->reg_area = map_physical_memory( "TLan Regs", (void *)base, size, B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &device->reg_base )) < 0 )
		return B_ERROR;
	device->reg_base = (uint8 *)(device->reg_base)+offset;
	
	device->host_cmd = (volatile uint32 *)((uint8 *)(device->reg_base)+0);
	device->ch_parm = (volatile uint32 *)((uint8 *)(device->reg_base)+4);
	device->dio_adr = (volatile uint16 *)((uint8 *)(device->reg_base)+8);
	device->host_int = (volatile uint16 *)((uint8 *)(device->reg_base)+10);
	device->dio_data32 = (volatile uint32 *)((uint8 *)(device->reg_base)+12);
	device->dio_data16 = (volatile uint16 *)((uint8 *)(device->reg_base)+12);
	device->dio_data8 = (volatile uint8 *)((uint8 *)(device->reg_base)+12);
	
	return B_OK;
}

status_t allocate_lists( dev_info *device )
{
	uint32				size;
	int32				i, j;
	void				*list_base, *tx_list_base, *rx_list_base, *phys_rx_list_base, *phys_tx_list_base;
	void				*base, *end;
	ring_element		*ring;
	tlan_list			*list;
	physical_entry		entry;
	
	size = ((BUFFER_SIZE+sizeof(ring_element)) * (TX_LISTS+RX_LISTS))+(TX_LIST_SIZE*TX_LISTS)+(RX_LIST_SIZE*RX_LISTS);
	// Round size up to nearest page boundry
	size = (size+(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	
	// Create area for lists and buffers
	if( (device->list_area = create_area( "tlan lists & buffers", &list_base, B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA )) < 0 )
		return device->list_area;
	
	// Setup list base pointers
	tx_list_base = (tlan_list *)list_base;
	rx_list_base = (tlan_list *)((uint8 *)tx_list_base + TX_LISTS*TX_LIST_SIZE);
	device->tx_ring = (ring_element *)((uint8 *)rx_list_base + RX_LISTS*RX_LIST_SIZE);
	device->rx_ring = (ring_element *)(device->tx_ring + TX_LISTS);
	
	// Setup Tx Lists
	for( base=device->rx_ring+RX_LISTS, end=(uint8 *)base+(TX_LISTS*BUFFER_SIZE), list=(tlan_list *)tx_list_base, 
		ring=device->tx_ring, i=0; base<end; (uint8 *)base+=BUFFER_SIZE, list=(tlan_list *)((uint8 *)list + TX_LIST_SIZE), ring++, i++ )
	{
		get_memory_map( (uint8 *)list + TX_LIST_SIZE, 4, &entry, 1 );
		list->forward_ptr = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		list->cstat = B_HOST_TO_LENDIAN_INT16(0x3000);
		list->frame_size = B_HOST_TO_LENDIAN_INT16(BUFFER_SIZE);
		get_memory_map( base, BUFFER_SIZE, &entry, 1 );
		list->buffers[0].data_count = B_HOST_TO_LENDIAN_INT32(BUFFER_SIZE);
		list->buffers[0].data_address = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		for( j=1; j<10; j++ )
		{
			list->buffers[j].data_count = 0;
			list->buffers[j].data_address = NULL;
		}
		
		ring->next = (void *)(ring+1);
		ring->previous = (void *)(ring-1);
		ring->list = list;
		ring->forward_ptr = list->forward_ptr;
		ring->buffer = base;
		
		get_memory_map( list, 4, &entry, 1 );
		ring->listPhy = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
	}
	
	ring--;
	ring->list->forward_ptr = NULL;
	ring->next = (void *)device->tx_ring;
	ring->forward_ptr = device->tx_ring->listPhy;
	device->tx_ring->previous = (void *)ring;
	
	// Setup Rx buffers
	for( base=end, end=(uint8 *)base+(RX_LISTS*BUFFER_SIZE), list=(tlan_list *)rx_list_base, 
		ring=device->rx_ring, i=0; base<end; (uint8 *)base+=BUFFER_SIZE, list=(tlan_list *)((uint8 *)list + RX_LIST_SIZE), ring++, i++ )
	{
		get_memory_map( (uint8 *)list + RX_LIST_SIZE, 4, &entry, 1 );
		list->forward_ptr = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		list->cstat = B_HOST_TO_LENDIAN_INT16(0x3000);
		list->frame_size = B_HOST_TO_LENDIAN_INT16(BUFFER_SIZE);
		
		get_memory_map( base, BUFFER_SIZE, &entry, 1 );
		list->buffers[0].data_count = B_HOST_TO_LENDIAN_INT32(BUFFER_SIZE);
		list->buffers[0].data_address = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
		
		ring->next = (void *)(ring+1);
		ring->previous = (void *)(ring-1);
		ring->list = list;
		ring->forward_ptr = list->forward_ptr;
		ring->buffer = base;
		
		get_memory_map( list, 4, &entry, 1 );
		ring->listPhy = (void *)B_HOST_TO_LENDIAN_INT32((long)gPCIModInfo->ram_address( entry.address ));
	}
	
	ring--;
	ring->list->forward_ptr = NULL;
	ring->next = (void *)device->rx_ring;
	ring->forward_ptr = device->rx_ring->listPhy;
	device->rx_ring->previous = (void *)ring;
	
	// Debug Ring
	/*ring = device->tx_ring;
	i = 0;
	do
	{
		dprintf( "%ld: listAdr: 0x%.8lX\n", i, (int32)ring->listPhy );
		dprintf( "%ld: forward_ptr: 0x%.8lX\n", i, (int32)ring->list->forward_ptr );
		dprintf( "%ld: this: 0x%.8lX\n", i, (int32)ring );
		dprintf( "%ld: next: 0x%.8lX\n", i, (int32)ring->next );
		dprintf( "%ld: ring fwd_ptr: 0x%.8lX\n", i, (int32)ring->forward_ptr );
		ring = (ring_element *)ring->next;
		i++;
	}while( ring != device->tx_ring );*/
	
	return B_OK;
}

void init_list( volatile ring_element *e, bool end )
{
	e->list->cstat = B_HOST_TO_LENDIAN_INT16(0x3000);
	e->list->frame_size = B_HOST_TO_LENDIAN_INT16(BUFFER_SIZE);
	e->list->buffers[0].data_count = B_HOST_TO_LENDIAN_INT32(BUFFER_SIZE);
	if( end )
		e->list->forward_ptr = NULL;
	else
		e->list->forward_ptr = e->forward_ptr;
}

// ***
// Begin EEPROM support functions
// ***
inline void e_clr( volatile uint16 *netSio, uint16 mask ) { *netSio &= ~mask; __eieio(); spin( 10 ); };
inline void e_set( volatile uint16 *netSio, uint16 mask ) { *netSio |= mask; __eieio(); spin( 10 ); };
inline void e_send_0( volatile uint16 *netSio ) { e_clr(netSio,EDATA); e_set(netSio,ECLOK); e_clr(netSio,ECLOK); };
inline void e_send_1( volatile uint16 *netSio ) { e_set(netSio,EDATA); e_set(netSio,ECLOK); e_clr(netSio,ECLOK); };
// high-to-low transistion on data line while clock is high
inline void e_send_start( volatile uint16 *netSio ) { e_set(netSio,ECLOK); e_set(netSio,EDATA|ETXEN); e_clr(netSio,EDATA); e_clr(netSio,ECLOK); };
// low-to-high transistion on data line while clock is high
inline void e_send_stop( volatile uint16 *netSio ) { e_clr(netSio,EDATA); e_set(netSio,ECLOK); e_set(netSio,EDATA); e_clr(netSio,ETXEN); };
inline bool e_ack( volatile uint16 *netSio ) {
	e_clr(netSio,EDATA); e_clr( netSio, ETXEN ); e_set( netSio, ECLOK );
	
	if( *netSio & EDATA )
		return false;
	e_clr(netSio, ECLOK);
	return true;
};
inline void e_sel( volatile uint16 *netSio, eeprom_access access ) {
	// Send Device Type Identifier: 1010
	e_send_1( netSio ); e_send_0( netSio ); e_send_1( netSio ); e_send_0( netSio );
	// Send Device Number: first device on bus: 000
	e_send_0( netSio ); e_send_0( netSio ); e_send_0( netSio );
	// Send access mode: 1=read, 0=write
	if( access )
		e_send_1( netSio );
	else
		e_send_0( netSio );
};

// ***
// End EEPROM support functions
// ***

status_t read_eeprom( dev_info *device, uint8 address, uint8 *data )
{
	cpu_status  	former;
	uint8       	i;
	status_t		status = B_OK; 
	volatile uint16	*netSio = device->dio_data16;
	
	// Disable interrupts and lock
	former = disable_interrupts();
	acquire_spinlock( &device->netSioLock );
	
	// Set address of NetSio
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
	
	// Send EEPROM start sequence
	e_send_start( netSio );
	
	// put EEPROM device id, address, and write command on bus
	e_sel( netSio, EEPROM_WRITE );
	
	// Ack
	if( !e_ack( netSio ) )
	{
		kprintf( "1st" );
		status = B_ERROR;
		goto bailout;
	}
	
	// Set Write
	e_set( netSio, ETXEN );
	
	// Send Address
	for( i=0x80; i; i >>=1 )
	{
		if( i & address )
			e_send_1( netSio );
		else
			e_send_0( netSio );
	}
	
	// Ack for address received
	if( !e_ack( netSio ) )
	{
		kprintf( "2nd" );
		status = B_ERROR;
		goto bailout;
	}
	
	// Set Write
	e_set( netSio, ETXEN );
	
	// Send EEPROM start sequence
	e_send_start( netSio );
		
	// put EEPROM device id, address, read command on bus
	e_sel( netSio, EEPROM_READ );
	
	// Ack
	if( !e_ack( netSio ) )
	{
		kprintf( "3rd" );
		status = B_ERROR;
		goto bailout;
	}
	
	*data = 0;
	// Read Data
	for( i=0x80; i; i>>=1 )
	{
		e_set( netSio, ECLOK );
		if( *netSio & EDATA )
			*data |= i;
		e_clr( netSio, ECLOK );
	}
	
	e_set( netSio, ECLOK );
	e_clr( netSio, ECLOK );
	// e_set( netSio, ETXEN );
	
	// Send stop access sequence
	e_send_stop( netSio );
	
	// Restore interrupts and unlock
	bailout:
	release_spinlock( &device->netSioLock );
    restore_interrupts(former);
    return status;
}

// ***
// Begin MII PHY support functions
// ***
inline void m_clr( volatile uint16 *netSio, uint16 mask ) { *netSio &= ~mask; __eieio(); spin( 1 ); };
inline void m_set( volatile uint16 *netSio, uint16 mask ) { *netSio |= mask; __eieio(); spin( 1 ); };
inline void m_send_0( volatile uint16 *netSio ) { m_clr(netSio,MCLK); m_clr(netSio,MDATA); m_set(netSio,MCLK); };
inline void m_send_1( volatile uint16 *netSio ) { m_clr(netSio,MCLK); m_set(netSio,MDATA); m_set(netSio,MCLK); };

void mii_send( volatile uint16 *netSio, uint16 data, uint8 numBits )
{
	uint16		mask = 1;
	
	// data = B_HOST_TO_LENDIAN_INT16(data);
	for( mask<<=(numBits-1); mask; mask>>=1 )
	{
		if( mask & data )
			m_send_1( netSio );
		else
			m_send_0( netSio );
	}
}

void mii_receive( volatile uint16 *netSio, uint16 *data, uint8 numBits )
{
	uint16		mask = 1;
	
	for( mask<<=(numBits-1); mask; mask>>=1 )
	{
		m_clr( netSio, MCLK );
		m_set( netSio, MCLK );
		if( *netSio & MDATA )
			*data |= mask;
	}
	
	// *data = B_HOST_TO_LENDIAN_INT16( *data );
}

status_t access_mii( dev_info *device, mii_action action, uint8 mii_dev, uint8 address, uint16 *data )
{
	cpu_status  	former;
	status_t		status = B_OK;
	uint8			i;
	uint16			minten;
	volatile uint16	*netSio = device->dio_data16;
	
	// Disable interrupts and lock
	former = disable_interrupts();
	acquire_spinlock( &device->netSioLock );
	
	// Set address of NetSio
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0000); __eieio();
	
	// Preamble: Sync PHY
	// Write 32, 1-bits to the mii
	m_set( netSio, MDATA ); // Set data to 1
	m_set( netSio, MTXEN ); // Set to write
	
	// Toggle the clock 32 times
	for( i=0; i<32; i++ )
	{
		m_clr(netSio,MCLK);
		m_set(netSio,MCLK);
	}
	
	if( (minten=*netSio & MINTEN) )
		m_clr( netSio, MINTEN ); // Disable MII interrupts
	m_clr( netSio, MDATA ); // Clear data
	m_set( netSio, MCLK ); // Assert clock
	m_set( netSio, MTXEN ); // Set to write
	
	// send 2-bit start of frame delimiter (01)
	m_send_0( netSio );
	m_send_1( netSio );
	
	// Send 2-bit op code: Read=10, Write=01
	if( action == MII_READ ) {
		m_send_1( netSio ); m_send_0( netSio ); }
	else {
		m_send_0( netSio ); m_send_1( netSio ); }
	
	// Send PHY address
	mii_send( netSio, mii_dev, 5 );
	
	// Send reg address
	mii_send( netSio, address, 5 );
	
	if( action == MII_READ ) // Read
	{
		m_set( netSio, MDATA ); // Set data high to detect PHY drive to zero
		m_clr( netSio, MTXEN ); // Set to read
		
		// Idle clock for turnaround time
		m_clr(netSio,MCLK);
		m_set(netSio,MCLK);
		m_clr(netSio,MCLK);
		
		if( !(*netSio & MDATA) ) // check for ack=0
		{
			*data = 0;
			mii_receive( netSio, data, 16 );
		}
		else // No ack
		{
			for( i=0; i<17; i++ )
			{
				m_clr(netSio,MCLK); 
				m_set(netSio,MCLK);
			}
			status = B_ERROR;
		}
	}
	else // Write
	{
		// Send ack
		m_send_1( netSio ); m_send_0( netSio );
		mii_send( netSio, *data, 16 );
	}
	
	// Idle Cycle
	m_clr(netSio,MCLK); 
	m_set(netSio,MCLK);
	
	// Enable PHY interupts
	if( minten )
		m_set(netSio,MINTEN);
		
	// Restore interrupts and unlock
	release_spinlock( &device->netSioLock );
    restore_interrupts(former);
	return status;
}

void set_multicast( dev_info *device, uint8 *adr, bool state )
{
	// Use an exact address
	/*
	*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x001C | 0x8000); __eieio(); // Auto address increment enabled
	#if B_HOST_IS_LENDIAN
	*device->dio_data32 = (((uint32)adr[3]) << 24) | (((uint32)adr[2]) << 16) | (((uint32)adr[1]) << 8) | (uint32)adr[0]; __eieio();
	*device->dio_data16 = (((uint32)adr[5]) << 8) | (uint32)adr[4]; __eieio();
	#else
	*device->dio_data32 = (((uint32)adr[0]) << 24) | (((uint32)adr[1]) << 16) | (((uint32)adr[2]) << 8) | (uint32)adr[3]; __eieio();
	*device->dio_data16 = (((uint32)adr[4]) << 8) | (uint32)adr[5]; __eieio();
	#endif
	
	*/
	uint8	hash_offset;
	uint32	mask;
	
	// Hash_fun(0) = DA(0) xor DA(6) xor DA(12) xor DA(18) xor DA(24) xor DA(30) xor DA(36) xor DA(42)
	hash_offset =  (((adr[5]&0x01)>>0)^((adr[5]&0x40)>>6)^((adr[4]&0x10)>>4)^((adr[3]&0x04)>>2)^((adr[2]&0x01)>>0)^((adr[2]&0x40)>>6)^((adr[1]&0x10)>>4)^((adr[0]&0x04)>>2))<<0;
	// Hash_fun(1) = DA(1) xor DA(7) xor DA(13) xor DA(19) xor DA(25) xor DA(31) xor DA(37) xor DA(43)
	hash_offset |= (((adr[5]&0x02)>>1)^((adr[5]&0x80)>>7)^((adr[4]&0x20)>>5)^((adr[3]&0x08)>>3)^((adr[2]&0x02)>>1)^((adr[2]&0x80)>>7)^((adr[1]&0x20)>>5)^((adr[0]&0x08)>>3))<<1;
	// Hash_fun(2) = DA(2) xor DA(8) xor DA(14) xor DA(20) xor DA(26) xor DA(32) xor DA(38) xor DA(44)
	hash_offset |= (((adr[5]&0x04)>>2)^((adr[4]&0x01)>>0)^((adr[4]&0x40)>>6)^((adr[3]&0x10)>>4)^((adr[2]&0x04)>>2)^((adr[1]&0x01)>>0)^((adr[1]&0x40)>>6)^((adr[0]&0x10)>>4))<<2;
	// Hash_fun(3) = DA(3) xor DA(9) xor DA(15) xor DA(21) xor DA(27) xor DA(33) xor DA(39) xor DA(45)
	hash_offset |= (((adr[5]&0x08)>>3)^((adr[4]&0x02)>>1)^((adr[4]&0x80)>>7)^((adr[3]&0x20)>>5)^((adr[2]&0x08)>>3)^((adr[1]&0x02)>>1)^((adr[1]&0x80)>>7)^((adr[0]&0x20)>>5))<<3;
	// Hash_fun(4) = DA(4) xor DA(10) xor DA(16) xor DA(22) xor DA(28) xor DA(34) xor DA(40) xor DA(46)
	hash_offset |= (((adr[5]&0x10)>>4)^((adr[4]&0x04)>>2)^((adr[3]&0x01)>>0)^((adr[3]&0x40)>>6)^((adr[2]&0x10)>>4)^((adr[1]&0x04)>>2)^((adr[0]&0x01)>>0)^((adr[0]&0x40)>>6))<<4;
	// Hash_fun(5) = DA(5) xor DA(11) xor DA(17) xor DA(23) xor DA(29) xor DA(35) xor DA(41) xor DA(47)
	hash_offset |= (((adr[5]&0x20)>>5)^((adr[4]&0x08)>>3)^((adr[3]&0x02)>>1)^((adr[3]&0x80)>>7)^((adr[2]&0x20)>>5)^((adr[1]&0x08)>>3)^((adr[0]&0x02)>>1)^((adr[0]&0x80)>>7))<<5;
	
	// Select the hash address
	if( hash_offset >= 32 )
	{
		// Select HASH2
		*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x002C); __eieio();
		hash_offset -= 32;
	}
	else
	{
		// Select HASH1
		*device->dio_adr = B_HOST_TO_LENDIAN_INT16(0x0028); __eieio();
	}
	
	// Set or clear the bit
	if( state )
	{
		mask = 1 << hash_offset;
		*device->dio_data32 |= B_HOST_TO_LENDIAN_INT32(mask); __eieio();
	}
	else
	{
		mask = ~(1 << hash_offset);
		*device->dio_data32 &= B_HOST_TO_LENDIAN_INT32(mask); __eieio();
	}
}

#ifdef __cplusplus
}
#endif
