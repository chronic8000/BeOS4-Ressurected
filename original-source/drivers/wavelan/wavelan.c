 /*
 * Lucent Wavelan IEEE 802.11 wireless PCMCIA Driver
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 * hank@sackett.net
 */

/* set this define nonzero if the BIOS is broken and doesn't route PCMCIA hardware interrutps */
#define INTERRUPTS_BROKEN 0


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
#include <driver_settings.h>


#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>

#include "hcf.h"

#define kDevName "wavelan"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64			

#define IW_ESSID_MAX_SIZE	32

#if INTERRUPTS_BROKEN
typedef struct {
	timer      t;
	void       *cookie;
} isr_timer_t;

isr_timer_t isr_timer;

static int32 isr_timer_hook(timer *t);
#endif

 
/* Ethernet ioctrl opcodes */
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_TIMESTAMP,						/* set timestamp option; type int */
	ETHER_IOVEC								/* report if iovecs are supported */
};


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
#define DEBUG_FLAGS ( ERR | INFO | WARN | INTERRUPT )



/*
 *PCMCIA, ISA, PCI bus related code
 */
/* driver does port io */
#define write8(address, value)			(*pci->write_io_8)((address), (value))
#define write16(address, value)			(*pci->write_io_16)((address), (value))
#define write32(address, value)			(*pci->write_io_32)((address), (value))
#define read8(address)					((*pci->read_io_8)(address))
#define read16(address)					((*pci->read_io_16)(address))
#define read32(address)					((*pci->read_io_32)(address))


static isa_module_info		*isa;
char						*pci_name = B_PCI_MODULE_NAME;
pci_module_info				*pci;


/* keep track of all the cards */
static char 				**names = NULL;			/* what's published */

#define MAX_CARDS			4
static dev_link_t 			*devs[MAX_CARDS];
static int              	ndevs = 0;

/* pcmcia private info */
typedef struct local_info_t {
    dev_node_t	node;
    int			stop;
	sem_id  	read_sem;	/* realease to unblock reader on card removal */
} local_info_t;

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


/* Hardware specific definitions - begin */

#define TX_BUFFERS        1
#define RX_BUFFERS        16
#define RX_BUFFER_MASK  (RX_BUFFERS-1)

#define BUFFER_SIZE       2048L              /* Enough to hold a 1536 Frame     */
#define MAX_FRAME_SIZE    1514               /* 1514                            */
#define MAX_MULTI         32                 /* Hardware Related, Do not change */

#define TRANSMIT_TIMEOUT  ((bigtime_t)10000) /* 1/100 Second                    */

// Ethernet-II snap header
static char snap_header[] = { 0x00, 0x00, 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

// Valid MTU values (HCF_MAX_MSG (2304) is the max hardware frame size)
#define WVLAN_MIN_MTU 256
#define WVLAN_MAX_MTU (HCF_MAX_MSG - sizeof(snap_header))

// Max number of multicast addresses that the filter can accept
#define WVLAN_MAX_MULTICAST GROUP_ADDR_SIZE/6

#if 0
// Frequency list (map channels to frequencies)
const long frequency_list[] = { 2412, 2417, 2422, 2427, 2432, 2437, 2442,
                                2447, 2452, 2457, 2462, 2467, 2472, 2484 };

// Bit-rate list in 1/2 Mb/s (first is dummy - not for original turbo)
const int rate_list[] = { 0, 2, 4, -255, 11, 22, -4, -11, 0, 0, 0, 0 };
#endif

// A few details needed for WEP (Wireless Equivalent Privacy)
#define MAX_KEY_SIZE 14                 // 128 (?) bits
#define MIN_KEY_SIZE  5                 // 40 bits RC4 - WEP
#define MAX_KEYS      4                 // 4 different keys

#define RX_IN_USE 	0x80000000
#define RX_LEN_MASK	0x000007ff 	/* buffer limited to 2047 */


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

/* fc_signal constrained to have a maximum count of 1 */
bool fc_signal_binary( struct fc_lock *fc, int32 sem_flags )
{
	cpu_status	cpu;
	bool		sig;
	
	// Lock while performing test and set
	cpu = disable_interrupts();
	acquire_spinlock( &fc->slock );
	
	fc->count = 1; // Increment count
	
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
typedef struct driver_instance
{
	uchar			*rx_buf[RX_BUFFERS];	/* RX Frame Descriptor                  */
	uint32			rx_desc[RX_BUFFERS];	/* rx descriptor low bits length,- high bit on=>in_use, high bit off => free*/
	area_id			rx_buf_area;			/* Receive Descriptor and Buffer Areas  */
	uint32			rx_received, rx_acked;	/* rx frame indexes: rx_received for ISR; rx_acked for read_hook */
	sem_id			ilock;					/* read semaphore */
	sem_id			write_lock;				/* re-entrant write protection */
	fc_lock			olock;					/* write semapbore */
	int32			readLock, writeLock;	/* reentrant reads/writes not allowed */

	area_id			cookieID;  				/* Cookie Area ID           */
	int32			devID;    				/* Device ID                */
	uint32			addr;      				/* Base Address of HostRegs */
	pci_info		*pciInfo;  				/* PCI Informations         */
	dev_link_t		*pcmcia_info;			/* for pcmcia preinit... */
	ushort			irq;					/* device interrupt line */
	uchar			mac_addr[6];   			/* Ethernet Address */

	uint8   		tx_thresh;
	uint8   		rx_thresh;
	uint16 			cmd;
	uint8   		interrupted;

	uint8		 	duplex_full;			/* Full-Duplex Operation Requested. */
	uint8			duplex_lock;

	uint8			cur_tx;
	uint8			cur_rx;
	int32			inrw;					/* In Read or Write Function          */
	int				blockFlag;
	uint32			debug;					/* global debug flag */
	
	IFB_STRCT 		ifb;					/* hcf interface */

	/* driver settings file configuration parameters						*/
	char  station_name[IW_ESSID_MAX_SIZE+1];	/* Name of station			*/
	char  network_name[IW_ESSID_MAX_SIZE+1];	/* Name of network			*/
	int32 port_type;							/* Port-type				*/
	int32 channel;								/*	Channel [3]				*/
	int32 ap_density;							/* AP density [1]			*/
	int32 medium_reservation;					/* RTS threshold			*/
	int32 frag_threshold;						/* Fragmentation threshold 	*/
	int32 transmit_rate;						/* Transmit rate			*/
	char  key[MAX_KEY_SIZE+2];					/* 40 bit security key + "0x"*/

} ether_dev_info_t;


typedef struct iw_statistics	iw_stats;
typedef struct iw_quality	iw_qual;
typedef struct iw_freq		iw_freq;
/*
 * FUNCTION PROTOTYPES
 */
int	wvlan_hw_msf_assert(IFBP ifbp, void * msf_assert);
int wvlan_hw_setmaxdatalen (IFBP ifbp, int maxlen);
int wavelan_hw_getmacaddr (IFBP ifbp, uint8 *mac);
int wvlan_hw_getchannellist (IFBP ifbp);
int wvlan_hw_setporttype (IFBP ifbp, int ptype);
int wvlan_hw_getporttype (IFBP ifbp);
int wvlan_hw_setstationname (IFBP ifbp, char *name);
int wvlan_hw_getstationname (IFBP ifbp, char *name, int len);
int wvlan_hw_setssid (IFBP ifbp, char *name, int port_type);
int wvlan_hw_getssid (IFBP ifbp, char *name, int len, int cur, int port_type);
int wvlan_hw_getbssid (IFBP ifbp, char *mac, int len);
int wvlan_hw_setchannel (IFBP ifbp, int channel);
int wvlan_hw_getchannel (IFBP ifbp);
int wvlan_hw_getcurrentchannel (IFBP ifbp);
int wvlan_hw_setthreshold (IFBP ifbp, int thrh, int cmd);
int wvlan_hw_getthreshold (IFBP ifbp, int cmd);
int wvlan_hw_setbitrate (IFBP ifbp, int brate);
int wvlan_hw_getbitrate (IFBP ifbp, int cur);
int wvlan_hw_getratelist (IFBP ifbp, char *brlist);
int wvlan_hw_getfrequencylist (IFBP ifbp, iw_freq *list, int max);
int wvlan_getbitratelist (IFBP ifbp, int32 *list, int max);
int wvlan_hw_setpower (IFBP ifbp, int enabled, int cmd);
int wvlan_hw_getpower (IFBP ifbp, int cmd);
int wvlan_hw_setpmsleep (IFBP ifbp, int duration);
int wvlan_hw_getpmsleep (IFBP ifbp);
int wvlan_hw_getprivacy (IFBP ifbp);
int wvlan_hw_setprivacy (IFBP ifbp, int mode, int transmit, KEY_STRCT *keys);
static int wvlan_hw_setpromisc (IFBP ifbp, int promisc);

int wvlan_hw_shutdown (ether_dev_info_t *dev);
int wvlan_hw_reset (ether_dev_info_t *dev);

static status_t init_ring_buffers(ether_dev_info_t *device);
static void		free_ring_buffers(ether_dev_info_t *device);
static void 	configure_device(ether_dev_info_t *device);
static void		load_wavelan_settings(ether_dev_info_t *device);

int wvlan_change_mtu (ether_dev_info_t *dev, int new_mtu);
static void wvlan_set_multicast_list (ether_dev_info_t *dev);




//#define ETHER_DEBUG(flags, run_time_flags, args...) if (flags & run_time_flags) dprintf(args)
#define ETHER_DEBUG(device, flags, args...) if (device->debug & flags) dprintf(args)

/* for serial debug command*/
#define DEBUGGER_COMMAND true
ether_dev_info_t * gdev;
static int 		wavelan(int argc, char **argv);


/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **_cookie);
static status_t close_hook(void *_device);
static status_t free_hook(void *_device);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *_device,off_t pos, void *buf,size_t *len);
static status_t write_hook(void *_device, off_t pos, const void *buf, size_t *len);
static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length );
static int32    interrupt_hook(void *_device);                	   /* interrupt handler */

static status_t domulti(ether_dev_info_t *device, uint8 *addr);		   /* add multicast address to hardware filter list */
static void     reset_device(ether_dev_info_t *device);                  /* reset the lan controller (NIC) hardware */
static void 	dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
static void 	dump_rx_desc(ether_dev_info_t *device);				   /* dump hardware receive descriptor structures */
static void 	dump_tx_desc(ether_dev_info_t *device);				   /* dump hardware transmit descriptor structures */
static void 	dump_window(ether_dev_info_t *device, uchar window);	   /* dump hardware registers */
static void		clear_statistics(ether_dev_info_t *device);			   /* clear statistics counters */
static status_t allocate_sems(ether_dev_info_t *device);     	   /* allocate semaphores & spinlocks */
static void     delete_sems(ether_dev_info_t *device);                /* deallocate semaphores & spinlocks */

static int32 	get_isa_list(int32 maxEntries );
static uint16 	read_eeprom(ether_dev_info_t *device, int32 index);


status_t pc_init_driver(void);
void pc_uninit_driver(void);
const char **pc_publish_devices(void);


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



status_t init_driver()
{
	status_t 		status;
	int32			entries;
	char			devName[64];
	int32			i;
	

	status = get_module(pci_name, (module_info **)&pci);
	if(status != B_NO_ERROR)
		goto err1;

	if ((status = pc_init_driver()) != B_OK) {
		goto err2;
	};

	gdev = NULL;
	add_debugger_command (kDevName, wavelan, "Wavelan 802.11 Wireless");

	return B_NO_ERROR;
	
err2:
	pc_uninit_driver();
	put_module(pci_name);
err1:
	dprintf("pc_init_card failed %x\n", status);
	return status;
}

void uninit_driver(void)
{
	int32 	i;
	void 	*item;

	remove_debugger_command (kDevName, wavelan);

	pc_uninit_driver();

}


device_hooks *find_device(const char *name)
{
	int32 	i;
	char 	*item;

	return &gDeviceHooks;
 
}

void rc_error(uint16 err, uint16 line) {
	dprintf(kDevName ": RC err %d line %d\n",err, line);
}

static status_t open_hook(const char *name, uint32 flags, void **cookie) {

	int 				nd;
	ether_dev_info_t 	*device;
	IFBP				ifbp;
	hcf_16	i, j, rc;
	status_t err;
		
	/* allocate memory for each piece of hardware */
	if ((device = (ether_dev_info_t *)memalign( 8, sizeof(ether_dev_info_t) )) == NULL) {
		dprintf (kDevName ": memalign(%d) out of memory\n", sizeof(ether_dev_info_t));
		goto err0;
	}

	for (nd = 0; nd < MAX_CARDS; nd++)
		if ((devs[nd] != NULL) && (strcmp(devs[nd]->dev->dev_name, name) == 0))
        	break;
 
	if (nd == MAX_CARDS) {
		dprintf(kDevName ": %d device limit exceeded\n", MAX_CARDS);
		goto err0;
	}

	if((devs[nd]->state & DEV_PRESENT) == 0) {
		dprintf("%s: tried to open device that is gone\n", devs[nd]->dev->dev_name);
		goto err0;
	}
 
 	*cookie = device;
	memset(device, 0, sizeof(ether_dev_info_t));
	device->pcmcia_info = devs[nd];
	device->debug = DEBUG_FLAGS;
	ifbp = &device->ifb;
	dprintf("%s opened: irq %d   iobase 0x%04x \n",
		devs[nd]->dev->dev_name,devs[nd]->irq.AssignedIRQ, devs[nd]->io.BasePort1);
		
	if ((devs[nd]->irq.AssignedIRQ == 0) ||	(devs[nd]->irq.AssignedIRQ == 0xff)) {
		dprintf(kDevName ": IRQ assignment ERROR - check BIOS and device IRQ assignements\n");
		goto err1;
	}
	
	device->irq = devs[nd]->irq.AssignedIRQ;	
		
	if (allocate_sems(device) != B_OK) goto err1;	
		
	gdev = device; 

	if (init_ring_buffers(device) != B_OK) goto err2;	/* initialize ring buffer */	
		
#if INTERRUPTS_BROKEN

	isr_timer.cookie = device;
	#undef add_timer	/* ugh! <pcmcia/k_compat.h> is nasty! */
	err = add_timer(&isr_timer.t, isr_timer_hook, 1000, B_PERIODIC_TIMER);
	if(err != B_NO_ERROR)
		goto err3;
#else
	install_io_interrupt_handler( devs[nd]->irq.AssignedIRQ, interrupt_hook, *cookie, 0 );
#endif

	dprintf(kDevName ":open_hook: hcf_connect ifbp=%x, base=%x ", &device->ifb, devs[nd]->io.BasePort1);
	hcf_connect( ifbp, devs[nd]->io.BasePort1);
	dprintf("hcf_connect: magic %04x HCF_version %02x %02x\n",
			 ifbp->IFB_Magic, ifbp->IFB_HCFVersionMajor, ifbp->IFB_HCFVersionMinor );
	
	/* set msf_assert function */
	wvlan_hw_msf_assert(ifbp, msf_assert);

	configure_device(device);

	/* enable the hardware */
	if (rc = hcf_enable( ifbp, 0 )) rc_error(rc, __LINE__);

	if (rc= wavelan_hw_getmacaddr ( ifbp, device->mac_addr)) rc_error(rc, __LINE__);

#if 0  /* off so debugging is easier */
	if (rc = hcf_action( ifbp, HCF_ACT_TICK_ON )) rc_error(rc, __LINE__);
#endif

  	return B_OK;           
	
	err3:
		free_ring_buffers(device);
	err2:
		delete_sems(device);
	err1:
		free(device);	
	err0:
		return B_ERROR;
}


static status_t close_hook(void *_device) {
	ether_dev_info_t *device = (ether_dev_info_t *) _device;
	uint16 rc;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ": close_hook\n");
	
	if (rc = hcf_disable(&device->ifb, 0)) rc_error(rc, __LINE__);
	if (rc = hcf_action(&device->ifb, HCF_ACT_INT_OFF)) rc_error(rc, __LINE__);
	if (rc = hcf_action(&device->ifb, HCF_ACT_CARD_OUT)) rc_error(rc, __LINE__);

	dprintf(kDevName ": config_device CARD_OUT\n");

	hcf_disconnect(&device->ifb);

	delete_sems(device);
	
	return (B_NO_ERROR);
}


static status_t free_hook(void * cookie) {
	ether_dev_info_t *device = (ether_dev_info_t *) cookie;
	
	dprintf(kDevName ": free_hook %x\n", device);

	ETHER_DEBUG(device, FUNCTION, kDevName": free %x\n",device);

#if INTERRUPTS_BROKEN
	cancel_timer(&isr_timer.t);
#else
	remove_io_interrupt_handler( device->irq, interrupt_hook, cookie );
#endif

	
	free_ring_buffers(device);

	gdev = NULL;
		
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
/*			ETHER_DEBUG(device, INFO, kDevName ": control %x ether_getaddr\n",msg); */
			for (i=0; i<6; i++) {
				((uint8 *)buf)[i] = device->mac_addr[i];
			}
			return B_NO_ERROR;
		}
		case ETHER_INIT:
			ETHER_DEBUG(device, INFO, kDevName ": control %x init\n",msg);
			return B_NO_ERROR;
			
		case ETHER_GETFRAMESIZE:
			ETHER_DEBUG(device, INFO, kDevName ": control get_framesize %d\n", MAX_FRAME_SIZE);
			*(uint32 *)buf = MAX_FRAME_SIZE;
			return B_NO_ERROR;
			
		case ETHER_ADDMULTI:
			ETHER_DEBUG(device, INFO, kDevName ": control %x add multi\n",msg);
			return (domulti(device, (unsigned char *)buf));
		
		case ETHER_SETPROMISC:
			ETHER_DEBUG(device, INFO, kDevName ": control %x set promiscuous\n",msg);
			if ( wvlan_hw_setpromisc(&device->ifb, *(uint32 *)buf) == 0)
				return B_NO_ERROR;
			else
				return B_ERROR;
		
		case ETHER_NONBLOCK:
//			ETHER_DEBUG(device, INFO, kDevName ": control %x blocking\n",msg);

			if (*((int32 *)buf))
				device->blockFlag = B_TIMEOUT;
			else
				device->blockFlag = 0;
			return B_NO_ERROR;

		case ETHER_TIMESTAMP:
			ETHER_DEBUG(device, INFO, kDevName ": control %x ether time stamp\n",msg);
			return B_ERROR;
	
		case ETHER_IOVEC:
			ETHER_DEBUG(device, INFO, kDevName ": control %x iovec\n",msg);
			return B_ERROR;

		default:
			ETHER_DEBUG(device, ERR, kDevName ": unknown iocontrol %x\n",msg);
			return B_ERROR;
	}

}


/* The read() system call - upper layer interface to the ethernet driver */
static status_t  read_hook(void *_device,off_t pos, void *buf,size_t *len)
{
	ether_dev_info_t  *device = (ether_dev_info_t *) _device;
	IFBP ifbp = &device->ifb;
	status_t    status;
	uint32 data_size, buflen;
	
	buflen = *len;
	*len = 0;
		
	/* Block until data is available */
	if ((status = acquire_sem_etc(device->ilock, 1, B_CAN_INTERRUPT | device->blockFlag, 0)) != B_NO_ERROR) {
/*		dprintf(kDevName ": read_hook %x on acquire ilock\n",status); */
		if (status == B_WOULD_BLOCK)
			return B_OK;
		else
			return status;
	}
	/* Prevent reentrant read */
	if (atomic_or(&device->readLock, 1 )) {
		release_sem_etc( device->ilock, 1, 0 );
		dprintf(kDevName ": read_hook, reentrant read!! Yike\n");
		return B_NO_ERROR;
	}
		
	if ((ifbp->IFB_RxStat == 0x2000) || (ifbp->IFB_RxStat == 0x4000))
	{

/* this hits a when running in INTERRUPTS_BROKEN mode */
//		dprintf(kDevName " RX:frame decap\n");
#if 0
		hcf_get_data(ifbp, 0, p, 12);
		hcf_get_data(ifb[, 12+sizeof(snap_header), p+12, len-12-sizeof(snap_header));
		skb_trim(skb, len-sizeof(snap_header));
#endif
	}	*len = device->rx_desc[device->rx_acked] & RX_LEN_MASK;
		memcpy(buf, device->rx_buf[device->rx_acked], *len);
		device->rx_acked = (device->rx_acked + 1) & RX_BUFFER_MASK;
	
	if ( device->debug & RX) {
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
	IFBP ifbp = &device->ifb;
	int16        frame_size;
	status_t      status;
	int16         j, rc;
	
	ETHER_DEBUG(device, FUNCTION, kDevName ":write buf %x len %d (decimal)\n",buf,*len);
 
	if( *len > MAX_FRAME_SIZE ) {
		ETHER_DEBUG(device, ERR, kDevName ": write %d > MAX_FRAME_SIZE tooo long\n",*len);
		*len = MAX_FRAME_SIZE;
	}
	frame_size = *len;

	if( (status = acquire_sem_etc( device->write_lock, 1, B_CAN_INTERRUPT, 0 )) != B_NO_ERROR ) {
		return status;	
	}
retry:
	if( (status = fc_wait( &device->olock, B_INFINITE_TIMEOUT )) != B_NO_ERROR ) {
		*len = 0;
		goto err;
	}

	/* Prevent reentrant write */
	if (atomic_or( &device->writeLock, 1 )) {
		ETHER_DEBUG(device, ERR, "write_hook: reentrant write\n"); /* this should never happen */
		fc_signal_binary(&device->olock, B_DO_NOT_RESCHEDULE);
		*len = 0;
		status = B_ERROR;
		goto err;
	}

	/* is PC card still present? */
	if(device->pcmcia_info && (device->pcmcia_info->state & DEV_PRESENT) == 0) {
		dprintf(kDevName ": write_hook card not present\n");
		atomic_add(&device->writeLock, -1);
		*len = 0;
		status = B_INTERRUPTED;
		goto err;
	}
	
	/* device specific write code goes here */
	hcf_action( ifbp, HCF_ACT_INT_OFF );

	if( ifbp->IFB_PIFRscInd == 0 ) {
		hcf_action( ifbp, HCF_ACT_INT_ON );
		dprintf(kDevName ": write_hook: PIFRscInd resource not available!\n");
		device->writeLock = 0;
		goto retry;
	}
	
	hcf_put_data( ifbp, (hcf_8 *)buf, frame_size, 0 );

	if (rc = hcf_send( ifbp, 0 )) rc_error(rc, __LINE__);

	hcf_action( ifbp, HCF_ACT_INT_ON );
	
	if (device->debug & TX) {
		dump_packet("tx", (unsigned char *) buf, frame_size);
	}

	/* Another write may now take place */
	device->writeLock = 0;

	status = B_OK;
err:
	release_sem(device->write_lock);

	return status;
}

static status_t writev_hook( void *_device, off_t position, const iovec *vec, size_t count, size_t *length ) {

	return B_ERROR;
};

static long
sem_count(sem_id sem)
{
	long count;

	get_sem_count(sem, &count);
	return (count);
}

#if INTERRUPTS_BROKEN
static int32
isr_timer_hook(timer *t)
{
	isr_timer_t *isr = (isr_timer_t *)t;
	interrupt_hook(isr->cookie);
	return B_HANDLED_INTERRUPT;
}
#endif

/* service interrupts generated by the Lan controller (card) hardware */
static int32
interrupt_hook(void *_device)
{
	ether_dev_info_t *device = (ether_dev_info_t *) _device;
	IFBP ifbp = &device->ifb;
	int32 handled = B_UNHANDLED_INTERRUPT;
	uint16      rc, event, rcs_ind;
	
	int32 work_limit = 10;


	/* disable card interrupts - odd but necessary for receivew to work */
	hcf_action(ifbp, HCF_ACT_INT_OFF); /* wavelan card interrupts off */
	
//	while (work_limit--) {

		if ((event = hcf_service_nic( ifbp )) == 0) {
//		 	dprintf(kDevName ":ISR zero event\n");	
			;
			//break;
			//return handled;
		}
		handled = B_HANDLED_INTERRUPT;

		if ((event & HREG_EV_RX) ||  (event == 0)) {  /* rx frame copy-move to high priority thread!!! */
			uint32 len = ifbp->IFB_RxLen;
			if (len == 0) {
				//dprintf(kDevName ": ISR rx zero length!\n");
				;
			} else  {
				if (len > MAX_FRAME_SIZE) {
					dprintf(kDevName ": ISR rx %d too big!\n", len);
					len = MAX_FRAME_SIZE;
				}					
				if (len > 1514) dprintf("Warning: frame %d will crash NetServer\n",len);
				/* receive frame from card */
//				dprintf(kDevName ": ISR rx %d bytes\n", len);
				hcf_get_data( ifbp, 0, device->rx_buf[device->rx_received], len );
				if (((device->rx_received + 1) & RX_BUFFER_MASK) == device->rx_acked) {
					int32 sem_count;
					if (get_sem_count(device->ilock, &sem_count) < 0) {
						dprintf(kDevName ": ISR rx frame dropped get_sem_count failed\n");
					} else {
						dprintf(kDevName ": ISR rx frame dropped sem count = %d\n",sem_count);
					}
				} else {
					device->rx_desc[device->rx_received] = RX_IN_USE | len;
					device->rx_received = (device->rx_received + 1) & RX_BUFFER_MASK;
					release_sem_etc(device->ilock, 1, B_DO_NOT_RESCHEDULE);
					handled = B_INVOKE_SCHEDULER;
				}
			}
		}
		if (ifbp->IFB_PIFRscInd) {
//			dprintf(kDevName ": ISR sig wr %d\n", ifbp->IFB_PIFRscInd);
			fc_signal_binary(&device->olock, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
		
//	}	 /* while (work_limit)		

	hcf_action( ifbp, HCF_ACT_INT_ON );
	return handled;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))



static status_t domulti(ether_dev_info_t *device, uint8 *addr) {
	
	ETHER_DEBUG(device, FUNCTION,kDevName ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
					addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	
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

void msf_assert ( wci_bufp file_namep, hcf_16 line_number, hcf_16 trace, int qual ) {
	dprintf("assert in %s at line %d, trace info: %04X, qualifier %04X\n",
			 file_namep, line_number, trace, qual);
} // msf_assert


/* Serial Debugger command
   Connect a terminal emulator to the serial port at 19.2 8-1-None
   Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
   At the kdebug> prompt enter "wavelan arg...",
   for example "AcmeEthernet R" to enable a received packet trace.
*/
static int
wavelan(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: wavelan { Function_calls | Interrupts | Number_frames | BUS_IO | Stats | Rx_trace | Tx_trace }\n";	
	

	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	if (gdev == NULL) {
		kprintf(kDevName ": not open\n");
		return 0;
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

static void load_wavelan_settings(ether_dev_info_t *device) {

	void *handle;
	const driver_settings *settings;
	uint8 found = 0;
	int entries = 0;
	int i = 0;
	int j = 0;
	
	handle = load_driver_settings(kDevName);
	settings = get_driver_settings(handle);
	if (settings) {
		entries = settings->parameter_count;
		/* Basically, if we exceed the max. number of cards, we don't care
		   about any of the other entries in the settings file */
		for (i=0; i<entries; i++) {
			driver_parameter *parameter = settings->parameters + i;

			dprintf(kDevName ": loading kernel settings for device (%d)\n",  i);

			for (j=0; j < parameter->parameter_count; j++) {
				driver_parameter *temp_param = parameter->parameters + j;

				if (strcmp(temp_param->name, "key") == 0) 	/* WEP security key */
				{
					if (strlen(temp_param->values[0]) > MAX_KEY_SIZE) {
						dprintf("key toooo long %s\n",temp_param->values[0]);
					} else {
						strcpy( device->key, temp_param->values[0]);
					}
				} 
				if (strcmp(temp_param->name, "network_name") == 0) /* network name */
				{
					if (strlen(temp_param->values[0]) > IW_ESSID_MAX_SIZE) {
						dprintf("network name toooo long %s\n", temp_param->values[0]);
					} else {
						strcpy( device->network_name, temp_param->values[0]);
					}
				} 
				if (strcmp(temp_param->name, "channel") == 0) 
				{
					device->channel = strtoll(temp_param->values[0], NULL, 10);
					dprintf(kDevName ": channel %d\n", device->channel);
				} 
				if (strcmp(temp_param->name, "port_type") == 0) 
				{
					device->port_type = strtoll(temp_param->values[0], NULL, 10);
					dprintf(kDevName ": port_type=%d\n", device->port_type);
				} 
				if (strcmp(temp_param->name, "mode") == 0) 
				{
					device->channel = strtoll(temp_param->values[0], NULL, 10);
					dprintf(kDevName ": channel \n", device->channel);
				} 
			}
		}
	}
	unload_driver_settings(handle);
}


static void configure_device(ether_dev_info_t *device) {

	IFBP	ifbp = &device->ifb;
	uint16 rc;
	
	/* defualt values */	
	*device->station_name = 0;						/* Name of station			*/
	*device->network_name = 0;						/* Name of network			*/
	device->port_type = 1;							/* Port-type				*/
	device->channel = 3;							/*	Channel [3]				*/
	device->ap_density = 1;							/* AP density [1]			*/
	device->medium_reservation = 2347;				/* RTS threshold			*/
	device->frag_threshold = 2346;					/* Fragmentation threshold 	*/
	device->transmit_rate = 3;						/* Transmit rate			*/
	*device->key = 0;								/* no security by default 	*/


#if 1
	if (rc = hcf_action( ifbp, HCF_ACT_CARD_IN )) rc_error(rc, __LINE__);
	dprintf(kDevName ": CARD_IN: %04x %04x %04x\n", rc, ifbp->IFB_PRICfg, ifbp->IFB_STACfg );
	if (rc = hcf_action( ifbp, HCF_ACT_INT_ON )) rc_error(rc, __LINE__);
	
	if (rc = wvlan_hw_setmaxdatalen (ifbp, 0x900)) rc_error(rc, __LINE__);
	if (rc = wvlan_hw_setporttype (ifbp, 1)) rc_error(rc, __LINE__);

	if (rc = wvlan_hw_setthreshold (ifbp, 1, CFG_CNF_SYSTEM_SCALE)) rc_error(rc, __LINE__);
	if (rc = wvlan_hw_setthreshold (ifbp, 0x92b, CFG_RTS_THRH)) rc_error(rc, __LINE__);
	if (rc = wvlan_hw_setthreshold (ifbp, 0x92a, CFG_FRAGMENTATION_THRH)) rc_error(rc, __LINE__);
	if (rc = wvlan_hw_setthreshold (ifbp, 3, CFG_TX_RATE_CONTROL)) rc_error(rc, __LINE__);
//#else
#endif

	load_wavelan_settings(device);

	dprintf(kDevName ": config: network %s key %s channel %d\n",
		device->network_name,device->key, device->channel);


	if (device->network_name[0]) {
		dprintf(kDevName ": setting network name %s len %d\n", device->network_name, strlen(device->network_name));
		wvlan_hw_setstationname (ifbp, device->network_name);
	}

	if (device->port_type != 1 ) {	/* not default */
		dprintf(kDevName ": setting port_type %d\n", device->port_type);
		if (rc = wvlan_hw_setporttype (ifbp, device->port_type)) rc_error(rc, __LINE__);
		if (rc = wvlan_hw_setchannel (ifbp, device->channel)) rc_error(rc, __LINE__);
		if (rc = wvlan_hw_setssid (ifbp, device->network_name, device->port_type)) rc_error(rc, __LINE__);
	}

	rc = wvlan_hw_getstationname (ifbp, device->network_name, 32);
	wvlan_hw_reset(device);
	
	/* set Wired Equivalent Privacy key if loaded from /boot/home/config/settings/kernel/drivers/wavelan file */
	{
		KEY_STRCT keys[MAX_KEYS];
		char *	p;
		char	temp[3];
		int  	len, j, k=0;
		
		memset(keys, 0, sizeof(KEY_STRCT) * MAX_KEYS);
		memset(temp, 0, sizeof(temp));
		if (*device->key) {  		/* to do: generalize for all 4 default keys */
			len = min(32, strlen(device->key));	/* maximum 128 bit encryption */
			p = device->key;
			/* convert ascii key to hex  */
			for (j=strlen(device->key); j>0; j=j-2) {
				temp[0] = *p++; temp[1]=*p++;
				keys[0].key[k++] = strtol(temp, NULL, 16);
			}
			keys[0].len = len;
			wvlan_hw_setprivacy ( ifbp, 1, 0, keys);
			wvlan_hw_reset(device);
		}
	}
}


/*
 * Allocate and initialize semaphores and spinlocks.
 */
static status_t allocate_sems(ether_dev_info_t *device) {
	status_t result;

	/* Setup Semaphores */
	if ((device->ilock = create_sem(0, kDevName " rx")) < 0) {
		dprintf(kDevName " create rx sem failed %x \n", device->ilock);
		return (device->ilock);
	}

	if ((device->write_lock = create_sem(1, kDevName " write_lock")) < 0) {
		dprintf(kDevName " create write_lock sem failed %x \n", device->ilock);
		delete_sem(device->ilock);
		return (device->write_lock);
	}


	((local_info_t *)device->pcmcia_info->priv)->read_sem = device->ilock;
	
	/* Intialize tx fc_lock to zero.
	   The semaphore is used to block the write_hook until free space
	   in on the card is available.
	 */
	if((result = create_fc_lock( &device->olock, 1, kDevName " tx" )) < 0 ) {
		dprintf(kDevName " create tx fc_lock failed %x \n", result);
		return (result);
	}
	
	device->readLock = device->writeLock = 0;

	device->blockFlag = 0;
		
	return (B_OK);
}

static void delete_sems(ether_dev_info_t *device) {
		delete_sem(device->ilock);
		delete_sem(device->write_lock);
		delete_fc_lock(&device->olock);
}


int wvlan_hw_reset (ether_dev_info_t *device)
{
	IFBP	ifbp = &device->ifb;
	unsigned long flags;
	int rc;

	// Disable hardware
	if (rc = hcf_disable( ifbp, 0 )) rc_error(rc, __LINE__);
	if (rc = hcf_action( ifbp, HCF_ACT_INT_OFF )) rc_error(rc, __LINE__);

	// Re-Enable hardware
	if (rc = hcf_action( ifbp, HCF_ACT_INT_ON )) rc_error(rc, __LINE__);
	if (rc = hcf_enable( ifbp, 0 )) rc_error(rc, __LINE__);

	return rc;
}

#define WVLAN_CURRENT	1
#define WVLAN_DESIRED	0


int	wvlan_hw_msf_assert(IFBP ifbp, void * msf_assert)
{
		CFG_REG_ASSERT_RTNP_STRCT	ltv;

		ltv.len = 4;
		ltv.typ = CFG_REG_ASSERT_RTNP;
		ltv.lvl = 0xFFFF;
		ltv.rtnp = ( MSF_ASSERT_RTNP ) msf_assert;
		return hcf_put_info( ifbp, (LTVP) &ltv );	
}	


int wvlan_hw_setmaxdatalen (IFBP ifbp, int maxlen)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_MAX_DATA_LEN;
	ltv.id[0] = maxlen;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc)	dprintf(kDevName ": hcf_put_info(CFG_CNF_MAX_DATA_LEN:0x%x) returned 0x%x\n", maxlen, rc);
	return rc;
}

int wavelan_hw_getmacaddr (IFBP ifbp, uint8 *mac)
{
	CFG_MAC_ADDR_STRCT ltv;
	int rc, l;

	ltv.len = 4;
	ltv.typ = CFG_CNF_OWN_MAC_ADDR;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	if (rc) {
		dprintf(kDevName ": hcf_get_info(CFG_CNF_OWN_MAC_ADDR) returned 0x%x\n", rc);
		return rc;
	}
	l = min(6, ltv.len*2);
	memcpy(mac, (char *)ltv.mac_addr, l);
	dprintf(kDevName ": mac addr %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return 0;
}

int wvlan_hw_getchannellist (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, chlist;

	ltv.len = 2;
	ltv.typ = CFG_CHANNEL_LIST;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	chlist = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(CFG_CHANNEL_LIST):0x%x returned 0x%x\n", chlist, rc);
	return rc ? 0 : chlist;
}

int wvlan_hw_setporttype (IFBP ifbp, int ptype)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_PORT_TYPE;
	ltv.id[0] = ptype;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_CNF_PORT_TYPE:0x%x) returned 0x%x\n", ptype, rc);
	return rc;
}

int wvlan_hw_getporttype (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, ptype;

	ltv.len = 2;
	ltv.typ = CFG_CNF_PORT_TYPE;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	ptype = ltv.id[0];
	if (rc) dprintf(kDevName ": hcf_get_info(CFG_CNF_PORT_TYPE):0x%x returned 0x%x\n", ptype, rc);
	return rc ? 0 : ptype;
}

int wvlan_hw_setstationname (IFBP ifbp, char *name)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	ltv.typ = CFG_CNF_OWN_NAME;
	l = min(strlen(name), ltv.len*2);
	ltv.id[0] = l;
	memcpy((char *) &ltv.id[1], name, l);
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_CNF_OWN_NAME:'%s') returned 0x%x\n", name, rc);
	return rc;
}

int wvlan_hw_getstationname (IFBP ifbp, char *name, int len)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	ltv.typ = CFG_CNF_OWN_NAME;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	dprintf(kDevName ": hcf_get_info(CFG_CNF_OWN_NAME) returned 0x%x\n", rc);
	if (rc)
		return rc;
	if (ltv.id[0])
		l = min(len, ltv.id[0]);
	else
		l = min(len, ltv.len*2);	/* It's a feature */
	memcpy(name, (char *) &ltv.id[1], l);
	name[l] = 0;
	dprintf(kDevName ": hcf_get_info(CFG_CNF_OWN_NAME):'%s'\n", name);
	return 0;
}

int wvlan_hw_setssid (IFBP ifbp, char *name, int port_type)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;

	if (port_type == 3)
		ltv.typ = CFG_CNF_OWN_SSID;
	else
		ltv.typ = CFG_CNF_DESIRED_SSID;
	l = min(strlen(name), ltv.len*2);
	ltv.id[0] = l;
	memcpy((char *) &ltv.id[1], name, l);
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_CNF_OWN/DESIRED_SSID:'%s') returned 0x%x\n", name, rc);
	return rc;
}

int wvlan_hw_getssid (IFBP ifbp, char *name, int len, int cur, int port_type)
{
	CFG_ID_STRCT ltv;
	int rc, l;

	ltv.len = 18;
	if (cur)
		ltv.typ = CFG_CURRENT_SSID;
	else
		if (port_type == 3)
			ltv.typ = CFG_CNF_OWN_SSID;
		else
			ltv.typ = CFG_CNF_DESIRED_SSID;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	dprintf(kDevName ": Check PORT!!! hcf_get_info(CFG_CNF_OWN/DESIRED/CURRENT_SSID) returned 0x%x\n", rc);
	if (rc)
		return rc;
	if (ltv.id[0])
	{
		l = min(len, ltv.id[0]);
		memcpy(name, (char *) &ltv.id[1], l);
	}
	else
		l = 0;
	name[l] = '\0';
	dprintf(kDevName ": hcf_get_info(CFG_CNF_OWN/DESIRED/CURRENT_SSID):'%s'\n", name);
	return 0;
}

int wvlan_hw_getbssid (IFBP ifbp, char *mac, int len)
{
	CFG_MAC_ADDR_STRCT ltv;
	int rc, l;

	ltv.len = 4;
	ltv.typ = CFG_CURRENT_BSSID;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	dprintf(kDevName ": hcf_get_info(CFG_CURRENT_BSSID) returned 0x%x\n", rc);
	if (rc)
		return rc;
	l = min(len, ltv.len*2);
	memcpy(mac, (char *)ltv.mac_addr, l);
	return 0;
}

int wvlan_hw_setchannel (IFBP ifbp, int channel)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_OWN_CHANNEL;
	ltv.id[0] = channel;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_CNF_OWN_CHANNEL:0x%x) returned 0x%x\n", channel, rc);
	return rc;
}

int wvlan_hw_getchannel (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, channel;

	ltv.len = 2;
	ltv.typ = CFG_CNF_OWN_CHANNEL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	channel = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(CFG_CNF_OWN_CHANNEL):0x%x returned 0x%x\n", channel, rc);
	return rc ? 0 : channel;
}

int wvlan_hw_getcurrentchannel (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, channel;

	ltv.len = 2;
	ltv.typ = CFG_CURRENT_CHANNEL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	channel = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(CFG_CURRENT_CHANNEL):0x%x returned 0x%x\n", channel, rc);
	return rc ? 0 : channel;
}

int wvlan_hw_setthreshold (IFBP ifbp, int thrh, int cmd)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = cmd;
	ltv.id[0] = thrh;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(0x%x:0x%x) returned 0x%x\n", cmd, thrh, rc);
	return rc;
}

int wvlan_hw_getthreshold (IFBP ifbp, int cmd)
{
	CFG_ID_STRCT ltv;
	int rc, thrh;

	ltv.len = 2;
	ltv.typ = cmd;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	thrh = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(0x%x):0x%x returned 0x%x\n", cmd, thrh, rc);
	return rc ? 0 : thrh;
}

int wvlan_hw_setbitrate (IFBP ifbp, int brate)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_TX_RATE_CONTROL;
	ltv.id[0] = brate;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_TX_RATE_CONTROL:0x%x) returned 0x%x\n", brate, rc);
	return rc;
}

int wvlan_hw_getbitrate (IFBP ifbp, int cur)
{
	CFG_ID_STRCT ltv;
	int rc, brate;

	ltv.len = 2;
	ltv.typ = cur ? CFG_CURRENT_TX_RATE : CFG_TX_RATE_CONTROL;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	brate = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(CFG_TX_RATE_CONTROL):0x%x returned 0x%x\n", brate, rc);
	return rc ? 0 : brate;
}

int wvlan_hw_getratelist(IFBP ifbp, char *brlist)
{
	CFG_ID_STRCT ltv;
	int rc, brnum;

	ltv.len = 10;
	ltv.typ = CFG_SUPPORTED_DATA_RATES;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	brnum = ltv.id[0];
	memcpy(brlist, (char *) &ltv.id[1], brnum);
	dprintf(kDevName "%s: hcf_get_info(CFG_CHANNEL_LIST):0x%x returned 0x%x\n", brnum, rc);
	return rc ? 0 : brnum;
}

#if 0
int wvlan_hw_getfrequencylist(IFBP ifbp, iw_freq *list, int max)
{
	int chlist = wvlan_hw_getchannellist(ifbp);
	int i, k = 0;

	/* Compute maximum number of freq to scan */
	if(max > 15)
		max = 15;

	/* Check availability */
	for(i = 0; i < max; i++)
		if((1 << i) & chlist)
		{
			list[k].i = i + 1;	/* Set the list index */
			list[k].m = frequency_list[i] * 100000;
			list[k++].e = 1;	/* Values in table in MHz -> * 10^5 * 10 */
		}

	return k;
}
#endif

int wvlan_getbitratelist(IFBP ifbp, int32 *list, int max)
{
	char brlist[9];
	int brnum = wvlan_hw_getratelist(ifbp, brlist);
	int i;

	/* Compute maximum number of freq to scan */
	if(brnum > max)
		brnum = max;

	/* Convert to Mb/s */
	for(i = 0; i < max; i++)
		list[i] = (brlist[i] & 0x7F) * 500000;

	return brnum;
}

int wvlan_hw_setpower (IFBP ifbp, int enabled, int cmd)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = cmd;
	ltv.id[0] = enabled;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(0x%x:0x%x) returned 0x%x\n", cmd, enabled, rc);
	return rc;
}

int wvlan_hw_getpower (IFBP ifbp, int cmd)
{
	CFG_ID_STRCT ltv;
	int rc, enabled;

	ltv.len = 2;
	ltv.typ = cmd;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	enabled = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(0x%x):0x%x returned 0x%x\n", cmd, enabled, rc);
	return rc ? 0 : enabled;
}

int wvlan_hw_setpmsleep (IFBP ifbp, int duration)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_CNF_MAX_SLEEP_DURATION;
	ltv.id[0] = duration;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CNF_MAX_SLEEP_DURATION:0x%x) returned 0x%x\n", duration, rc);
	return rc;
}

int wvlan_hw_getpmsleep (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, duration;

	ltv.len = 2;
	ltv.typ = CFG_CNF_MAX_SLEEP_DURATION;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	duration = ltv.id[0];
	dprintf(kDevName ": hcf_get_info(CNF_MAX_SLEEP_DURATION):0x%x returned 0x%x\n", duration, rc);
	return rc ? 0 : duration;
}

int wvlan_hw_getprivacy (IFBP ifbp)
{
	CFG_ID_STRCT ltv;
	int rc, privacy;

	// This function allow to distiguish bronze cards from other
	// types, to know if WEP exist...
	// This is stupid, we have no way to distinguish the silver
	// and gold cards, because the call below return 1 in all
	// cases. Yuk...
	ltv.len = 2;
	ltv.typ = CFG_PRIVACY_OPTION_IMPLEMENTED;
	rc = hcf_get_info(ifbp, (LTVP) &ltv);
	privacy = ltv.id[0];
	dprintf(kDevName ": hcf_get_info: privacy=%x returned %x\n", privacy, rc);
	return rc ? 0 : privacy;
}

int wvlan_hw_setprivacy (IFBP ifbp, int mode, int transmit, KEY_STRCT *keys)
{
	CFG_ID_STRCT ltv;
	CFG_CNF_DEFAULT_KEYS_STRCT ltv_key;
	int rc;
	
	if (mode)
	{
		// Set the index of the key used for transmission
		ltv.len = 2;
		ltv.typ = CFG_CNF_TX_KEY_ID;
		ltv.id[0] = transmit;
		rc = hcf_put_info(ifbp, (LTVP) &ltv);
		if (rc) {
			dprintf(kDevName ": hcf_put_info(CFG_CNF_TX_KEY_ID:0x%x) returned 0x%x\n", mode, rc);
			return rc;
		}
		// Set the keys themselves (all in one go !)
		ltv_key.len = sizeof(KEY_STRCT)*MAX_KEYS/2 + 1;
		ltv_key.typ = CFG_CNF_DEFAULT_KEYS;
		
		memcpy((char *) &ltv_key.key, (char *) keys, sizeof(KEY_STRCT)*MAX_KEYS);
		rc = hcf_put_info(ifbp, (LTVP) &ltv_key);
		if (rc) {
			dprintf(kDevName ": hcf_put_info(CFG_CNF_TX_KEY_ID:0x%x) returned 0x%x\n", mode, rc);
			return rc;
		}
	}
	// enable/disable encryption
	ltv.len = 2;
	ltv.typ = CFG_CNF_ENCRYPTION;
	ltv.id[0] = mode;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_CNF_ENCRYPTION:0x%x) returned 0x%x\n", mode, rc);
	return rc;
}



static int wvlan_hw_setpromisc (IFBP ifbp, int promisc)
{
	CFG_ID_STRCT ltv;
	int rc;

	ltv.len = 2;
	ltv.typ = CFG_PROMISCUOUS_MODE;
	ltv.id[0] = promisc;
	rc = hcf_put_info(ifbp, (LTVP) &ltv);
	if (rc) dprintf(kDevName ": hcf_put_info(CFG_PROMISCUOUS_MODE:0x%x) returned 0x%x\n", promisc, rc);
	return rc;
}



/*** PCMCIA STUFF ***/

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
   insertion and ejection events.  They are invoked from the wavelan
   event handler. 
*/

static void wavelan_config(dev_link_t *link);
static void pcmcia_release(u_long arg);
static int pcmcia_event(event_t event, int priority,
		       event_callback_args_t *args);

/*
   The attach() and detach() entry points are used to create and destroy
   "instances" of the driver, where each instance represents everything
   needed to manage one actual PCMCIA card.
*/

static dev_link_t *pcmcia_attach(void);
static void pcmcia_detach(dev_link_t *);

/*
   The dev_info variable is the "key" that is used to match up this
   device driver with appropriate cards, through the card configuration
   database.
*/
static dev_info_t dev_info = kDevName;

/*
   An array of "instances" of the wavelan device.  Each actual PCMCIA
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

    pcmcia_attach() creates an "instance" of the driver, allocating
    local data structures for one device.  The device is registered
    with Card Services.

    The dev_link structure is initialized, but we don't actually
    configure the card at this point -- we wait until we receive a
    card insertion event.
    
======================================================================*/

static dev_link_t *pcmcia_attach(void)
{
    client_reg_t client_reg;
    dev_link_t *link;
    local_info_t *local;
	ether_dev_info_t *dev;
    int ret, i;
	    
   	dprintf(kDevName ": pcmcia_attach\n");

    /* Find a free slot in the device table */
    for (i = 0; i < MAX_CARDS; i++) {
		if (devs[i] == NULL) break;
	    if (i == MAX_CARDS) {
			printk(kDevName ": device limit %d exceede\n", MAX_CARDS);
			return NULL;
   		}
    }
    /* Initialize the dev_link_t structure */
    link = malloc(sizeof(struct dev_link_t));
  	if (link == NULL) return NULL;
	memset(link, 0, sizeof(struct dev_link_t));
    devs[i] = link;
    ndevs++;
    link->release.function = &pcmcia_release;
    link->release.data = (u_long)link;


    /* Interrupt setup */
    link->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    link->irq.IRQInfo1 = IRQ_INFO2_VALID|IRQ_LEVEL_ID;
	link->irq.IRQInfo2 = 0xdeb8;
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
    if (local == NULL) goto err1;
	memset(local, 0, sizeof(local_info_t));
    link->priv = local;

#if 0
    /* Allocate driver instance (per card) globals */
    dev = malloc(sizeof(ether_dev_info_t));
    if (dev == NULL) goto err2;
	memset(dev, 0, sizeof(ether_dev_info_t));
    local->dev = dev;
#endif

    /* Register with Card Services */
    client_reg.dev_info = &dev_info;
    client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
    client_reg.EventMask =
	CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
	CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
	CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
    client_reg.event_handler = &pcmcia_event;
    client_reg.Version = 0x0210;
    client_reg.event_callback_args.client_data = link;
    ret = CardServices(RegisterClient, &link->handle, &client_reg);
    if (ret != 0) {
		cs_error(link->handle, RegisterClient, ret);
		pcmcia_detach(link);
		goto err3;
    }

    return link;

	err3:
		free(dev);
	err2:
		free(local);
	err1:
		free(link);
		return NULL;

} /* pcmcia_attach */

/*======================================================================

    This deletes a driver "instance".  The device is de-registered
    with Card Services.  If it has been released, all local data
    structures are freed.  Otherwise, the structures will be freed
    when the device is released.

======================================================================*/

static void pcmcia_detach(dev_link_t *link)
{
    int i;
    
    dprintf(kDevName ": pcmcia_detach(0x%p)\n", link);
    
	if (link == NULL) {
		dprintf("pcmcia_detach called with NULL argumant\n");
		return;
	}

    /* Locate device structure */
    for (i = 0; i < MAX_CARDS; i++)
		if (devs[i] == link) break;
    if (i == MAX_CARDS)
		return;

 dprintf(kDevName ": pcmcia_detach link %x\n", link); 
	 /*
       If the device is currently configured and active, we won't
       actually delete it yet.  Instead, it is marked so that when
       the release() function is called, that will trigger a proper
       detach().
    */
    if (link->state & DEV_CONFIG) {
		dprintf( kDevName ": detach postponed, '%s' " "still locked\n", link->dev->dev_name);
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
} /* pcmcia_detach */

/*======================================================================

    wavelan_config() is scheduled to run after a CARD_INSERTION event
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

static void wavelan_config(dev_link_t *link)
{
	client_handle_t handle = link->handle;
	tuple_t tuple;
	cisparse_t parse;
	local_info_t *dev = link->priv;
	int last_fn, last_ret, nd;
	u_char buf[64];
	win_req_t req;
	memreq_t map;
	
	
	dprintf("wavelan_config(0x%p)\n", link);
	
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
		
		if (cfg->index == 0) goto next_entry;
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
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
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
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
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
	for (nd = 0; nd < MAX_CARDS; nd++)
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
	pcmcia_release((u_long)link);
} /* wavelan_config */

/*======================================================================

    After a card is removed, pcmcia_release() will unregister the
    device, and release the PCMCIA configuration.  If the device is
    still open, this will be postponed until it is closed.
    
======================================================================*/

static void pcmcia_release(u_long arg)
{
    dev_link_t *link = (dev_link_t *)arg;

   	dprintf(kDevName ": pcmcia_release(0x%p)\n", link);

    /*
       If the device is currently in use, we won't release until it
       is actually closed, because until then, we can't be sure that
       no one will try to access the device or its data structures.
    */
    if (link->open) {
	dprintf(kDevName ": wavelan_cs: release postponed, '%s' still open\n",
		  link->dev->dev_name);
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
	pcmcia_detach(link);
    
} /* pcmcia_release */

/*======================================================================

    The card status event handler.  Mostly, this schedules other
    stuff to run after an event is received.

    When a CARD_REMOVAL event is received, we immediately set a
    private flag to block future accesses to this device.  All the
    functions that actually access the device should check this flag
    to make sure the card is still present.
    
======================================================================*/

static int pcmcia_event(event_t event, int priority,
		       event_callback_args_t *args)
{
    dev_link_t *link = args->client_data;

    dprintf(kDevName ": pcmcia_event(0x%06x) link%x\n", event,link);
    
    switch (event) {
    case CS_EVENT_CARD_REMOVAL:
		link->state &= ~DEV_PRESENT;
		release_sem_etc(((local_info_t *)link->priv)->read_sem, 1, B_DO_NOT_RESCHEDULE);
		if (link->state & DEV_CONFIG) {
		    ((local_info_t *)link->priv)->stop = 1;
			link->release.expires = RUN_AT(HZ/20);
	   		cs->_add_timer(&link->release);
		}
		break;
    case CS_EVENT_CARD_INSERTION:
		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		wavelan_config(link);
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
} /* pcmcia_event */


status_t pc_init_driver(void)
{
    client_reg_t client_reg;
    servinfo_t serv;
    bind_req_t bind;
    int i, ret;
    
    for(i=0;i<MAX_CARDS;i++) devs[i] = NULL;
    
    if (get_module(CS_CLIENT_MODULE_NAME, (struct module_info **)&cs) != B_OK) goto err1;
    if (get_module(DS_MODULE_NAME, (struct module_info **)&ds) != B_OK) goto err2;
	
    CardServices(GetCardServicesInfo, &serv);
    if (serv.Revision != CS_RELEASE_CODE) goto err3;

    /* When this is called, Driver Services will "attach" this driver
       to any already-present cards that are bound appropriately. */
    register_pccard_driver(&dev_info, &pcmcia_attach, &pcmcia_detach);
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
        for (i = 0; i < MAX_CARDS; i++)
        if (devs[i]) {
            if (devs[i]->state & DEV_CONFIG)
            pcmcia_release((u_long)devs[i]);
            pcmcia_detach(devs[i]);
        }
		if (names != NULL) free(names);
		if (ds) put_module(DS_MODULE_NAME);
		if (cs) put_module(CS_CLIENT_MODULE_NAME);
    }
}


const char **publish_devices(void)
{
    int i, nd;

	if (names != NULL) free(names);
	names = malloc((ndevs+1) * sizeof(char *));
	for (nd = i = 0; nd < MAX_CARDS; nd++) {
		if (devs[nd] == NULL) continue;
		names[i++] = devs[nd]->dev->dev_name;
	}
	names[i] = NULL;
	return (const char **)names;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;
