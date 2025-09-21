#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>
#include <driver_settings.h>
#include <Drivers.h>
#include <stdarg.h>
#include <module.h>
#include <malloc.h>
#include <ByteOrder.h>
#include <PCI.h>

#include "lld.h"
#include "constant.h"
#include "net_data.h"

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
	ETHER_DEVICE_END = ETHER_GETADDR + 1000
};

// Rangelan ioctrl opcodes
enum {
	ETHER_SET_SECURITY_ID = ETHER_DEVICE_END
};

static const char *kDevices[] = {
	"net/rangelan/0",
	NULL
};

static const char *kMasterName = "foobar";

// ***
// Structs
// ***

#define DATA_SIZE 	1520
typedef void (*CBFunc)( void );


typedef struct rx_descriptor
{
	struct LLDRxPacketDescriptor	rxd;
	uint8							data[DATA_SIZE];
	struct rx_descriptor			*next;
} rx_descriptor;

typedef struct tx_descriptor
{
	struct LLDTxPacketDescriptor	txd;
	struct LLDTxFragmentList		list;
} tx_descriptor;

// Info struct used for each open device
typedef struct dev_info
{
	char					masterName[32];
	int32					devID; 			// device identifier: 0-n
	sem_id					isr_sem;		// Used to signal the isr_thread
	volatile uint32			isr_running;	// should the isr thread continue running?
	thread_id				isr_thread;		// Thread used to service interupts
	CBFunc 					CallBackFunc;	// Interrupt callback function
	spinlock				listLock;
	
	sem_id					rxlock, txlock;	// Send and receive sems
	
	struct rx_descriptor	*rxHead;		// Head of the receive queue
	struct rx_descriptor	*rxTail;		// Tail of the receive queue
	int32					rxCount;		// Used to prevent the reception of more than 16 buffers
	
	int32					multicastCount;
	volatile int32			blockFlag;
	bool					isGo;
	bool					regInterrupt;	// Has the device registered the isr
	
	sem_id					ctrlSem;
	uint8					ctrlCmd;
	uint8					ctrlResult;
}dev_info;

// Driver Entry Points
__declspec(dllexport) status_t init_hardware( void );
__declspec(dllexport) status_t init_driver( void );
__declspec(dllexport) void uninit_driver( void );
__declspec(dllexport) const char** publish_devices( void );
__declspec(dllexport) device_hooks *find_device( const char *name );

// Driver Hook Functions
static status_t open_hook( const char *name, uint32 flags, void **cookie );
static status_t close_hook( void *cookie );
static status_t free_hook( void *cookie );
static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length );
static status_t read_hook( void *cookie, off_t position, void *data, size_t *length );
static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length );
static status_t select_hook( void *cookie, uint8 event, uint32 ref, selectsync *sync );
static status_t deselect_hook( void *cookie, uint8 event, selectsync *sync );
static status_t readv_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes );
static status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes );

static int32 device_interrupt_handler( void *cookie );
status_t isr_thread_loop( void *cookie );

// Debug print calls
uint8 *print_bytes( uint8 *data, size_t length );
uint8 *print_mac_addr( uint8 *address );

// ***
// Globals
// ***

static device_hooks gDeviceHooks = {
	open_hook,			// -> open entry point
	close_hook,			// -> close entry point
	free_hook,			// -> free cookie
	control_hook,		// -> control entry point
	read_hook,			// -> read entry point
	write_hook,			// -> write entry point
	select_hook,		// -> select
	deselect_hook,		// -> deselect
	readv_hook,			// -> readv
	writev_hook			// -> writev
};

static int32 			gOpenMask = 0;
static dev_info			*gDevice;

// ***
// LLD Globals
// ***

extern unsigned_16		LLDIOAddress1;
extern unsigned_8		LLDIntLine1;
extern unsigned_8		LLDChannel;
extern unsigned_8		LLDSubChannel;
extern unsigned_8		LLDTransferMode;
extern unsigned_8		*LLDMSTAName;
extern unsigned_8		LLDDomain;
extern unsigned_8		LLDNodeType;
extern unsigned_8		LLDPhysAddress[6];
extern unsigned_16		LLDLookAheadSize;
extern unsigned_8		LLDPCMCIA;
extern unsigned_8		LLDInit365Flag;
extern unsigned_8		LLDOEM;
extern unsigned_8		LLDMicroISA;
extern unsigned_8		LLDInactivityTimeOut;
extern unsigned_8		LLDSniffTime;
extern unsigned_8		LLDBDWakeup;
extern unsigned_8		LLDRoamingFlag;
extern unsigned_8		LLDBridgeFlag;

status_t init_hardware( void )
{
	
	return B_OK;
}

status_t init_driver( void )
{
	dprintf( "rl: init_driver\n" );
	if( init_mempool() != B_OK )
		return B_ERROR;
	return B_OK;
}

void uninit_driver( void )
{
	//dprintf( "rl: uninit_driver\n" );
	uninit_mempool();
}

const char** publish_devices( void )
{
	return kDevices;
}

device_hooks *find_device( const char *name )
{
	return &gDeviceHooks;
}

static status_t open_hook( const char *name, uint32 flags, void **cookie )
{
	int32				devID;
	int32				mask;
	status_t			status;
	const char 			*devName;
	dev_info			*device;
	void				*handle;
	
	//dprintf( "rl: open_hook\n" );
	// Find device name
	for( devID=0; (devName=kDevices[devID]); devID++ )
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
	if( (*cookie = gDevice = device = (dev_info *)memalign( 8, sizeof(dev_info) )) == NULL )
	{
		status = B_NO_MEMORY;
		goto err0;
	}
	
	// Setup the cookie
	device->devID = devID;
	device->regInterrupt = 0;
	device->blockFlag = 0;
	device->multicastCount = 0;
	device->listLock = 0;
	device->ctrlSem = -1;
	device->ctrlCmd = 0;
	
	
	
	// Hard Coded for now!
	LLDTransferMode = 0;	// 16 Bit transfers
	LLDInit365Flag = 0;		// Don't assume 365 PCMCIA controller
	LLDLookAheadSize = 20;	// 20 Bytes on look-ahead data
	LLDSniffTime = 12;		// 60 seconds until shutdown after computer goes into standy mode
	LLDRoamingFlag = 0;		// We can roam from one station to another
	
	if( (handle = load_driver_settings( "rangelan.conf" )) == NULL )
	{
		dprintf( "rl: no driver setting found: using defaults.\n" );
		LLDIOAddress1 = 0x0270;
		LLDIntLine1 = 15;
		LLDPCMCIA = 0;
		LLDOEM = 1;
		LLDMicroISA = 1;
		LLDDomain = 0;
		LLDInactivityTimeOut = 3;
		LLDBDWakeup = 0;
		LLDNodeType = STATION;
	}
	else
	{
		dprintf( "rl: Loading driver settings\n" );
		LLDIOAddress1 = strtol( get_driver_parameter( handle, "LLDIOAddress1", "0x270", "0x270" ), NULL, 16 );
		LLDIntLine1 = strtol( get_driver_parameter( handle, "LLDIntLine1", "15", "15" ), NULL, 10 );
		LLDPCMCIA = get_driver_boolean_parameter( handle, "LLDIOAddress1", 0, 0 );
		LLDOEM = LLDMicroISA = get_driver_boolean_parameter( handle, "LLDMicroISA", (bool)1, (bool)1 );
		LLDDomain = strtol( get_driver_parameter( handle, "LLDDomain", "0", "0" ), NULL, 10 );
		LLDInactivityTimeOut = strtol( get_driver_parameter( handle, "LLDInactivityTimeOut", "3", "3" ), NULL, 10 );
		LLDBDWakeup = get_driver_boolean_parameter( handle, "LLDBDWakeup", (bool)0, (bool)0 );
		LLDNodeType = strtol( get_driver_parameter( handle, "LLDNodeType", "0", "0" ), NULL, 10 );
		LLDChannel = strtol( get_driver_parameter( handle, "LLDChannel", "1", "1" ), NULL, 10 );
		LLDSubChannel = strtol( get_driver_parameter( handle, "LLDSubChannel", "1", "1" ), NULL, 10 );
		LLDMSTAName = device->masterName;
		strcpy( LLDMSTAName, get_driver_parameter( handle, "LLDMSTAName", "BeOS", "BeOS" ) );
		LLDBridgeFlag = get_driver_boolean_parameter( handle, "LLDBridgeFlag", 0, 0 );
		unload_driver_settings( handle );
	}
	dprintf( "rl: io base=0x%.4X irq=%ld\n", (int32)LLDIOAddress1, (int32)LLDIntLine1 );
	// Init Receive Queue
	device->rxCount = 0;
	device->rxHead = NULL;
	device->rxTail = NULL;
	
	// Setup Semaphores
	device->rxlock = -1;
	device->txlock = -1;
	device->isr_sem = -1;
	
	if( (device->rxlock = create_sem( 0, "rangelan_receive" )) < 0 )
	{
		status = device->rxlock;
		goto err1;
	}
	set_sem_owner( device->rxlock, B_SYSTEM_TEAM );
	
	if( (device->txlock = create_sem( 8, "rangelan_transmit" )) < 0 )
	{
		status = device->txlock;
		goto err1;
	}
	set_sem_owner( device->txlock, B_SYSTEM_TEAM );
	
	if( (device->isr_sem = create_sem( 0, "rangelan_isr_sem" )) < 0 )
	{
		status = device->isr_sem;
		goto err1;
	}
	set_sem_owner( device->isr_sem, B_SYSTEM_TEAM );
	
	// Start ISR Thread
	if( (device->isr_thread = spawn_kernel_thread( &isr_thread_loop, "rangelan_isr", 	B_DISPLAY_PRIORITY, device )) < 0 )
	{
		status = device->isr_thread;
		goto err1;
	}
	device->isr_running = true;
	device->isGo = false;
	resume_thread( device->isr_thread );
	
	// Start device
	if( (status = LLDInit()) != 0 )
	{
		dprintf( "rl: LLDInit() failure code = %ld\n", status );
		goto err2;
	}
	device->isGo = true;
	return B_OK;
	
	err2:
	device->isr_running = false;
	release_sem( device->isr_sem );
	wait_for_thread( device->isr_thread, &status );
	
	err1:
	delete_sem( device->txlock );
	delete_sem( device->rxlock );
	delete_sem( device->isr_sem );
	free( device );
	err0:
	gOpenMask &= ~mask;
	return B_ERROR;
}

static status_t close_hook( void *cookie )
{
	return B_OK;
}

static status_t free_hook( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	
	//dprintf( "rl: free_hook\n" );
	LLDStop();
	device->isr_running = false;
	release_sem( device->isr_sem );
	wait_for_thread( device->isr_thread, &status );
	
	if( device->regInterrupt )
	{
		//dprintf( "rl: free_hook: auto remove interrupt\n" );
		device->regInterrupt = false;
		remove_io_interrupt_handler( LLDIntLine1, device_interrupt_handler, device );
	}
	
	delete_sem( device->txlock );
	delete_sem( device->rxlock );
	delete_sem( device->isr_sem );
	free( device );
	gOpenMask = 0;
	return B_OK;
}

static status_t control_hook( void *cookie, uint32 opcode, void *data, size_t length )
{
	dev_info			*device = (dev_info *)cookie;
	int32				i;
	
	switch( opcode )
	{
		case ETHER_GETADDR:
			for( i=0; i<6; i++ )
				((uint8 *)data)[i] = LLDPhysAddress[i];
			return B_OK;
		
		case ETHER_INIT:
			
			return B_OK;
			
		case ETHER_GETFRAMESIZE:
			*(uint32 *)data = 1514;
			return B_OK;
			
		case ETHER_ADDMULTI:
			if( device->multicastCount++ == 0 )
				LLDMulticast( true );
			return B_OK;
			
		case ETHER_REMMULTI:
			if( --device->multicastCount == 0 )
				LLDMulticast( false );
			return B_OK;
			
		case ETHER_SETPROMISC:
			if( *((int32 *)data) ) {
				LLDPromiscuous(true); }
			else {
				LLDPromiscuous(false); }
			return B_OK;
		
		case ETHER_NONBLOCK:
			if( *((int32 *)data) )
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_OK;
		case ETHER_SET_SECURITY_ID:
			{
				char		command[8];
				char		*code = (char *)data;
				status_t 	status = B_ERROR;
				uint8		result;
				
				if( (device->ctrlSem = create_sem( 0, "rl_ctrl" )) <= 0 )
					return device->ctrlSem;
				
				// Enable EEPROM Write
				command[0] = 'A';
				command[1] = 0x80;
				command[2] = 0x18;
				command[3] = 0;
				device->ctrlCmd = 0x18;
				
				if( (result = LLDRawSend( command, 4 )) == 0 &&
					acquire_sem_etc( device->ctrlSem, 1, B_TIMEOUT | B_CAN_INTERRUPT, 10000000 ) == B_OK )
				{
					if( device->ctrlResult == 0 )
					{
						// Set Security ID
						command[0] = 'A';
						command[1] = 0x80;
						command[2] = 0x0C;
						command[3] = 0;
						command[4] = code[0];
						command[5] = code[1];
						command[6] = code[2];
						device->ctrlCmd = 0x0C;
						
						if( (result = LLDRawSend( command, 7 )) == 0 &&
							(status = acquire_sem_etc( device->ctrlSem, 1, B_TIMEOUT | B_CAN_INTERRUPT, 10000000 )) == B_OK )
						{
							if( device->ctrlResult != 0 )
								status = B_ERROR;
						}
						
						// Disable EEPROM write
						command[0] = 'A';
						command[1] = 0x80;
						command[2] = 0x17;
						command[3] = 0;
						LLDRawSend( command, 4 );
					}
				}
				delete_sem( device->ctrlSem );
				device->ctrlSem = -1;
				return status;
			}
		default:
			
			return B_ERROR;
	}
}

static status_t read_hook( void *cookie, off_t position, void *data, size_t *length )
{
	dev_info						*device = (dev_info *)cookie;
	status_t						status;
	rx_descriptor					*rx;
	size_t							size;
	cpu_status						previous;
	
	//dprintf( "rl: read_hook\n" );
	// Block until data is available
	if( (status = acquire_sem_etc( device->rxlock, 1, B_CAN_INTERRUPT | device->blockFlag, 0 )) != B_NO_ERROR )
	{
		*length = 0;
		return status;
	}
	
	// Pull the data off the front of the rx queue
	
	if( device->rxHead == NULL )
	{
		// We should not be here!
		dprintf( "rangelan: Received NULL packet!?\n" );
		return B_ERROR;
	}
	
	previous = disable_interrupts();
	acquire_spinlock( &device->listLock );
	
	rx = device->rxHead;
	device->rxHead = rx->next;
	
	release_spinlock( &device->listLock );
	restore_interrupts( previous );
	
	// Get the receive size	
	size = rx->rxd.LLDRxFragList[0].FSDataLen;
	if( size > *length )
		size = *length;
	else
		*length = size;
	// Copy the data out of the buffer and free the buffer
	memcpy( data, rx->data, size );
	delete_buf( rx );
	device->rxCount--;
	
	return B_OK;
}

static status_t write_hook( void *cookie, off_t position, const void *data, size_t *length )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	tx_descriptor		*tx;
	
	//dprintf( "rl: write_hook\n" );
	if( (status = acquire_sem_etc( device->txlock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR )
	{
		*length = 0;
		return status;
	}
	
	// Allocate buffer
	if( (tx = new_buf( sizeof(struct tx_descriptor) )) == NULL )
	{
		// This should never happen!
		release_sem_etc( gDevice->txlock, 1, B_DO_NOT_RESCHEDULE );
		*length = 0;
		return B_NO_MEMORY;
	}
	
	// Copy data to descriptor
	
	tx->txd.LLDTxDataLength = *length;
	memcpy( tx->txd.LLDImmediateData, data, *length );
	tx->txd.LLDImmediateDataLength = *length;
	tx->txd.LLDTxFragmentListPtr = &tx->list;
	tx->list.LLDTxFragmentCount = 0;
	
	// Enqueue the data to be sent
	if( LLDSend( (struct LLDTxPacketDescriptor *)tx ) > 1 )
	{
		delete_buf( tx );
		release_sem_etc( gDevice->txlock, 1, B_DO_NOT_RESCHEDULE );
		*length = 0;
		return B_ERROR;
	}
	
	return B_OK;
}

status_t select_hook( void *cookie, uint8 event, uint32 ref, selectsync *sync )
{
	return B_ERROR;
}

status_t deselect_hook( void *cookie, uint8 event, selectsync *sync )
{
	return B_ERROR;
}

status_t readv_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes )
{
	return B_ERROR;
}

status_t writev_hook( void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes )
{
	return B_ERROR;
}

static int32 device_interrupt_handler( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	
	//dprintf( "rl: device_interrupt_handler\n" );
	release_sem_etc( device->isr_sem, 1, B_DO_NOT_RESCHEDULE );
	return B_INVOKE_SCHEDULER;
	
	// B_INVOKE_SCHEDULER, B_HANDLED_INTERRUPT, B_UNHANDLED_INTERRUPT
}

status_t isr_thread_loop( void *cookie )
{
	dev_info			*device = (dev_info *)cookie;
	status_t			status;
	
	//dprintf( "rl: isr_thread_loop\n" );
	while( device->isr_running )
	{
		if( (status = acquire_sem_etc( device->isr_sem, 1, B_CAN_INTERRUPT | B_TIMEOUT, 100000 )) != B_OK )
		{
			if( device->isGo )
				LLDPoll();
			continue;
		}
		if( !device->isr_running )
			break;
		//dprintf( "rl: isr_thread_loop: callback\n" );
		device->CallBackFunc();
	}
	//dprintf( "rl: isr_thread_loop: exit\n" );
	return B_OK;
}


/**************************************************************************/
/*																								  */
/* Prototype definitions of LLS procedures called by the LLD.				  */
/*																								  */
/**************************************************************************/

// */\*
void LLSReceiveLookAhead (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 Status, unsigned_16 RSSI)
{
	bool	localBroadcast = true;
	uint8 	*frame = ((uint8 *)Buffer) + 6;
	int32	i;
	
	//dprintf( "rl: LLSReceiveLookAhead\n" );
	if( Length >= 12 )
	{
		// Check if we sent it
		for( i=5; i>0; i-- )
		{
			if( frame[i] != LLDPhysAddress[i] )
			{
				localBroadcast = false;
				break;
			}
		}
	}
	else
		localBroadcast = false;
		
	// Do we have room for the data in the receive queue?
	if( (Length > 0 ) && !localBroadcast && (gDevice->rxCount < 23) )
	{
		rx_descriptor 	*rx;
		cpu_status 		previous;
		
		if( (rx = (rx_descriptor *)new_buf( sizeof(struct rx_descriptor) )) != NULL )
		{
			rx->rxd.LLDRxFragCount = 1;
			rx->rxd.LLDRxFragList[0].FSDataPtr = rx->data;
			rx->rxd.LLDRxFragList[0].FSDataLen = Length;
			rx->next = NULL;
			
			// Receive the data
			LLDReceive( (struct LLDRxPacketDescriptor *)rx, 0, Length );
			rx->rxd.LLDRxFragList[0].FSDataLen = Length;
			
			previous = disable_interrupts();
			acquire_spinlock( &gDevice->listLock );
	
			// Append to recieve queue and signal receive
			if( gDevice->rxHead == NULL )
				gDevice->rxHead = gDevice->rxTail = rx;
			else
				gDevice->rxTail = gDevice->rxTail->next = rx;
			gDevice->rxCount++;
			
			release_spinlock( &gDevice->listLock );
			restore_interrupts( previous );
			
			//dprintf( "rl: LLSReceiveLookAhead: enqueue packet\n" );
			release_sem_etc( gDevice->rxlock, 1, 0 ); //B_DO_NOT_RESCHEDULE
		}
	}
}

void LLSPingReceiveLookAhead (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 Status, unsigned_16 RSSI)
{

}

void LLSRawReceiveLookAhead (unsigned_8 Buffer[], unsigned int Length)
{
	if( gDevice->ctrlSem >= 0 && Buffer[2] == gDevice->ctrlCmd )
	{
		dprintf( "rl: LLSRawReceiveLookAhead: Signaling control %.2X\n", (int32)Buffer[3] );
		gDevice->ctrlResult = Buffer[3];
		release_sem( gDevice->ctrlSem );
	}
}

/*unsigned_8 LLDPacketizeSend (unsigned_8 *PtrToPkt, unsigned_16 PktLength)
{

}*/

// */\*
unsigned_16 LLSSendComplete (struct LLDTxPacketDescriptor FAR *PktDesc)
{
	//dprintf( "rl: LLSSendComplete\n" );
	delete_buf( PktDesc );
	release_sem_etc( gDevice->txlock, 1, B_DO_NOT_RESCHEDULE );
	return 0;
}

unsigned_16 LLSSendProximPktComplete (struct LLDTxPacketDescriptor FAR *PktDesc, unsigned_8 Status)
{
	return 0;
}

unsigned_16 LLSRawSendComplete (unsigned_8 *Buffer)
{
	//dprintf( "rl: LLSRawSendComplete\n" );
	return 0;
}

unsigned_8 LLSRegisterInterrupt (unsigned_8 IntNum, void (*CallBackFunc) ())
{
	//dprintf( "rl: LLSRegisterInterrupt\n" );
	gDevice->regInterrupt = true;
	gDevice->CallBackFunc = CallBackFunc;
	install_io_interrupt_handler( IntNum, device_interrupt_handler, gDevice, 0 );
	return 0;
}

unsigned_8 LLSDeRegisterInterrupt (unsigned_8 IntNum)
{
	//dprintf( "rl: LLSDeRegisterInterrupt\n" );
	if( gDevice->regInterrupt )
	{
		gDevice->regInterrupt = false;
		remove_io_interrupt_handler( IntNum, device_interrupt_handler, gDevice );
	}
	return 0;
}

unsigned_16	LLSGetCurrentTime (void)
{
	return ((system_time() >> 15) & 0xFFFF);
}

/*void SendLLDPingResponse (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 SubType)
{

}*/

#ifdef  NDIS_MINIPORT_DRIVER
ULONGLONG LLSGetCurrentTimeLONG(void)
{
	return system_time();
}

unsigned_32 LLSGetTimeInMicroSec (void)
{
	return system_time() & 0xFFFFFFFF;
}

#else
unsigned_32	LLSGetCurrentTimeLONG(void)
{
	return system_time() & 0xFFFFFFFF;
}

#endif
/*void fmemcpy (unsigned_8 FAR *, unsigned_8 FAR *, unsigned_16)
{

}*/

void InitLLDVars (unsigned_8 Card)
{

}


#ifdef PROTOCOL
void LLSReceiveProt (unsigned char *Buffer, unsigned int Length, unsigned long TimeStamp)
{

}

#endif

// Debug Support Calls
uint8 *print_bytes( uint8 *data, size_t length )
{
	int32 i;
	for( i=0; i<length; i++ )
	{
		dprintf( "%.2x ", data[i] );	
	}
	return data+length;
}

uint8 *print_mac_addr( uint8 *address )
{
	int32 i;
	
	for( i=0; i<6; i++ )
	{
		if( i )
			dprintf( ":" );
		dprintf( "%.2x", address[i] );	
	}
	return address+6;
}

#ifdef __cplusplus
}
#endif