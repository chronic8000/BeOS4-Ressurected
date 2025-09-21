/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2001 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_basics.c
 * Description:	Implementation of the basic driver operation functions
 * Changes:		Feb 05, 2001	- Initial Revision [chrisl]
 *
 *****************************************************************************/

/******************************************************************************
 * IMPLEMENTATION NOTES:
 *
 * USE OF pci_dev_table:
 *   This structure contains a table of device names, vendor/device IDs, and
 *   device capabilities.  This "master list of devices" allows for easy 
 *   extendibility of the driver's compatibility.  Simply adding a new device
 *   to the list allows the driver to support the new card instantly with no
 *   modification of any further code (provided that no new features become
 *   available with added cards).
 *
 * ORGANIZATION OF gDev[].dev_other:
 *   [31-08] - Unused
 *   [07-00] - Index into global pci_dev_table list (i.e. Card type present)
 *
 *****************************************************************************/

#include <OS.h>						/* OS-Level defines */
#include <SupportDefs.h>			/* Support definitions (types, etc) */
#include <KernelExport.h>			/* Kernel exports */
#include <PCI.h>					/* PCI module use */
#include <Drivers.h>				/* useful driver functions */
#include <stdlib.h>					/* useful stuff */
#include <stdio.h>					/* more useful stuff... */
#include <string.h>					/* string manipulation */
#include <fc_lock.h>				/* fc_lock module */
#include "common/ether_driver.h"	/* ethernet driver types/defines */

/* tcm9xx Driver-Specific Includes */
#include "tcm9xx_defs.h"			/* Driver operation definitions */
#include "tcm9xx_regs.h"			/* Register Definitions */
#include "tcm9xx_proto.h"			/* Function Prototype Declarations */

#define kDevName 		"tcm9xx"
#define kDevDir			"net/" kDevName "/"

#define DEBUG 			true 		// if you want debug output
#define MAX_DEVICES		4			// instances of this driver, max
#define SINGLE_OPEN					// if only one open session per
									// device is allowed at a time
#define CARDBUS_SUPPORT				// CardBus support is enabled for 3C575 devices

struct device_info_t {
	// Required by driver_basics.h
	int32		devID;				// device identifier: 0-MAX_DEVICES
	pci_info	*pciInfo;			// info about this device
	uint32	debug;					// debugging level mask

	/*** tcm9xx-specific per-device data ***/

	/* CARD ADDRESS */
	ether_address_t	mac_address;						// MAC address of the NIC
	ether_address_t multi[ MAX_MULTI ];					// Multicast address list
	vuint8			multi_enabled;						// Multicast mode enabled

	/* MISCELLANEOUS */
	uint32	chip_idx;									// Chip index from the PCI table
	uint32	card_flags;									// Flags for the card's capabilities
	thread_id	media_poll_thread_id;					// Thread ID for media polling thread
	vuint32	media_poll_thread_running;					// is the polling thread running?
	vuint32	restart_tx;

	/* MEMORY ADDRESSES */
	uint32	base;										// Base memory address of the card
	area_id	reg_area;									// Memory-mapped I/O area
	area_id	tx_desc_area;								// Transmit descriptor storage area
	area_id	tx_buf_area;								// Transmit buffer storage area
	area_id	rx_desc_area;  								// Receive descriptor storage area
	area_id	rx_buf_area;  								// Receive buffer storage area
	area_id	cb_fns_area;								// CardBus function pointer area
	area_id reg_base_area;								// Area storing the register base
	uint8	*tx_buf[ TX_BUFFERS ]; 		 				// Transmit buffer
	uint8	*rx_buf[ RX_BUFFERS ]; 		 				// Receive buffer
	boom_tx_desc	*tx_desc[ TX_BUFFERS ];  			// Transmit descriptors
	boom_rx_desc	*rx_desc[ RX_BUFFERS ];  			// Receive descriptors
	uint32	paddr_table[ TX_BUFFERS + RX_BUFFERS ];		// Physical address table
	uint8	*vaddr_table[ TX_BUFFERS + RX_BUFFERS ];  	// Virtual address table
	uint8	*cb_fns_addr;								// Address to CardBus functions

	/* FLOW CONTROL LOCKS */
	sem_id	sem_rx_lock;  								// Receive flow control lock
	fc_lock	fc_tx_lock;  								// Transmit flow control lock
	sem_id	sem_media_poll;  							// Media polling semaphore
	spinlock	spin_window_lock;  						// Protect windowed registers
	vuint32	block_flag;  								// For nonblocking reads

	/* TRANSMIT FUNCTION POINTER */
	int32	( *card_tx ) ( struct device_info_t *device, const void *buf, size_t len );
	int32	( *card_rx ) ( struct device_info_t *device, const void *buf, size_t *len );

	/* DRIVER SETTINGS INFORMATION */
	int32	full_duplex;  								// Is the card full duplex?
	int32	if_port;  									// Physical interface port
	int32	eep_info1;  								// EEPROM info storage
	int32	eep_info2;  								//	"	 "	 "
	int32	eep_capabilities; 				 			// EEPROM capabilities storage
	int32	available_media;  							// Available media types storage
	int32	default_media;  							// Default media selection
	int32	autoselect;  								// Enable autoselect
	int32	advertising;
	int32	bus_master;									// bus mastering for newer vortexes
	int32	full_bus_master_tx;  						// Enable full transmit bus master
	int32	full_bus_master_rx;  						// Enable full receive bus master
	unsigned char	phys[ 2 ];
	uint16	status_enable;
	uint16	intr_enable;
	uint16	mtu;  										// MTU (Maximum Transmission Unit) setting
	uint8	rx_flags;									// Receiver mode storage

	/* TIMING INFORMATION */
	bigtime_t	last_rx;  								// Time of last receive
	bigtime_t	tx_start;  								// Time of transmit start

	/* RING BUFFER INFORMATION */
	vuint32	rx_current;  								// Current rx ring index
	vuint32	rx_dirty;  									// Dirty rx ring index
	vuint32	tx_full;  									// Is tx ring full?
	vuint32	tx_current;  								// Current tx ring index
	vuint32	tx_dirty;  									// Dirty tx ring index

	/* ATOMIC FLAGS */
	vuint32	interrupt;  								// Avoid interrupt handler reentry
	vuint32	tx_busy;  									// Is tx ring busy?
	vuint32	start;										// Has the card been started?

	/* PROFILING TIMES */
	bigtime_t rx_triggered;
	bigtime_t max_trig_time;
	bigtime_t max_reader_time;
	bigtime_t rx_int_time;

	/* STATUS VARIABLES */
	status_t  rx_fc_status;

	/* STATISTICS */
	int32	stat_queued_packet;
	int32	stat_command_timeouts;

	int32	stat_rx_errors;
	int32	stat_rx_dropped;
	int32	stat_rx_over_errors;
	int32	stat_rx_length_errors;
	int32	stat_rx_frame_errors;
	int32	stat_rx_crc_errors;
	int32	stat_rx_packets;  							// Number of received packets
	int32	stat_rx_bytes;
	int32	stat_rx_fifo_errors;
	int32	stat_fixing_signal;
	int32	stat_rx_triggers;
	int32	stat_rx_acquires;

	int32	stat_tx_fifo_errors;
	int32	stat_tx_dropped;
	int32	stat_tx_errors;
	int32	stat_tx_aborted_errors;
	int32	stat_tx_packets;  							// Number of packets transmitted

	int32	stat_collisions;
	int32	stat_tx_carrier_errors;
	int32	stat_tx_heartbeat_errors;
	int32	stat_tx_window_errors;
	int32	stat_tx_bytes;
};

#ifdef CARDBUS_SUPPORT
#include <pcmcia/cb_enabler.h>
/* Fill this list with all your CardBus devices' vendor and device IDs. Also,
 * leave the 0xff's as they are. */
cb_device_descriptor cb_descrs[] = {
	{ 0x10B7, 0x5057, 0xff, 0xff, 0xff },   /* 3C575 Boomerang CardBus */
	{ 0x10B7, 0x5157, 0xff, 0xff, 0xff },   /* 3CCFE575 Cyclone CardBus */
	{ 0x10B7, 0x5257, 0xff, 0xff, 0xff },   /* 3CCFE575CT CardBus */
	//	{ 0x10B7, 0x5X57, 0xff, 0xff, 0xff }, /* Some other CardBus? Put another number in
	//										 * place of the 'X' if you find a new card */
};
#endif // CARDBUS_SUPPORT

#include <common/ether_basics.h>
#include <common/driver_basics.c>


/******************************************************************************
 * List of supported devices
 *****************************************************************************/
struct _pci_device_info {
	const char	*name;
	uint16	vendor_id, device_id, device_id_mask;
	uint32	flags, io_size;
};

enum _card_flags {
	IS_VORTEX = 0x01, IS_BOOMERANG = 0x02, IS_CYCLONE = 0x04, IS_TORNADO = 0x08,
	HAS_PWR_CTRL = 0x10, HAS_MII = 0x20, HAS_NWAY = 0x40, HAS_CB_FNS = 0x80,
	EEPROM_8BIT = 0x200, INVERT_LED_PWR = 0x400, 
	MII_XCVR_PWR = 0x4000,
};

struct _pci_device_info pci_dev_table[] = {
	{"3c590 Vortex 10Mbps",			0x10B7, 0x5900, 0xffff, IS_VORTEX, 32},
	{"3c595 Vortex 100baseTx",		0x10B7, 0x5950, 0xffff, IS_VORTEX, 32},
	{"3c595 Vortex 100baseT4",		0x10B7, 0x5951, 0xffff, IS_VORTEX, 32},
	{"3c595 Vortex 100base-MII",	0x10B7, 0x5952, 0xffff, IS_VORTEX, 32},
	{"3Com Vortex",					0x10B7, 0x5900, 0xff00, IS_BOOMERANG, 64},
	{"3c900 Boomerang 10baseT",		0x10B7, 0x9000, 0xffff, IS_BOOMERANG, 64},
	{"3c900 Boomerang 10Mbps Combo",0x10B7, 0x9001, 0xffff, IS_BOOMERANG, 64},
	{"3c900B Cyclone 10baseT",		0x10B7, 0x9004, 0xffff, IS_CYCLONE, 128},
	{"3c900 Cyclone 10Mbps Combo",	0x10B7, 0x9005, 0xffff, IS_CYCLONE, 128},
	{"3c900B-FL Cyclone 10base-FL",	0x10B7, 0x900A, 0xffff, IS_CYCLONE, 128},
	{"3c905 Boomerang 100baseTx",	0x10B7, 0x9050, 0xffff, IS_BOOMERANG | HAS_MII, 64},
	{"3c905 Boomerang 100baseT4",	0x10B7, 0x9051, 0xffff, IS_BOOMERANG | HAS_MII, 64},
	{"3c905B Cyclone 100baseTx",	0x10B7, 0x9055, 0xffff, IS_CYCLONE | HAS_NWAY, 128},
	{"3c905B Cyclone 10/100 BNC",	0x10B7, 0x9058, 0xffff, IS_CYCLONE | HAS_NWAY, 128},
	{"3c905B-FX Cyclone 100baseFx",	0x10B7, 0x905A, 0xffff, IS_CYCLONE, 128},
	{"3c905C-TX Tornado 100baseTx",	0x10B7, 0x9200, 0xffff, IS_TORNADO | HAS_NWAY, 128},
	{"3c980 Cyclone",				0x10B7, 0x9800, 0xfff0, IS_CYCLONE, 128},
	{"3cSOHO100-TX Hurricane", 		0x10B7, 0x7646, 0xffff, IS_CYCLONE, 128},
	{"3c555 Laptop Hurricane", 		0x10B7, 0x5055, 0xffff, IS_CYCLONE, 128},
	{"3c556 Laptop Hurricane",		0x10B7, 0x6055, 0xffff, IS_TORNADO | HAS_NWAY, 128},
	{"3c575 Boomerang CardBus",		0x10B7, 0x5057, 0xffff, IS_BOOMERANG | HAS_MII | EEPROM_8BIT, 64},
	{"3CCFE575BT Cyclone CardBus",	0x10B7, 0x5157, 0xffff, IS_CYCLONE | HAS_NWAY | HAS_CB_FNS | EEPROM_8BIT | INVERT_LED_PWR, 128},
	{"3C3FE575CT Cyclone CardBus",	0x10B7, 0x5257, 0xffff, IS_TORNADO | HAS_NWAY | EEPROM_8BIT | HAS_CB_FNS | MII_XCVR_PWR, 128},
	{"3CCFE656 Cyclone CardBus",	0x10B7, 0x6560, 0xffff, IS_CYCLONE | HAS_NWAY | HAS_CB_FNS | INVERT_LED_PWR | MII_XCVR_PWR, 128},
	{"3CCFE656B Cyclone+WinModem",	0x10B7, 0x6562, 0xffff, IS_CYCLONE | HAS_NWAY | HAS_CB_FNS | INVERT_LED_PWR | MII_XCVR_PWR, 128},
	{"3CCFE656C Tornado+WinModem",	0x10B7, 0x6564, 0xffff, IS_TORNADO | HAS_NWAY | HAS_CB_FNS | EEPROM_8BIT | MII_XCVR_PWR, 128},
	{"3c450 HomePNA Tornado",		0x10B7, 0x4500, 0xffff, IS_TORNADO | HAS_NWAY, 128},
	{"3c575 series CardBus (?)",	0x10B7, 0x5057, 0xf0ff, IS_BOOMERANG | HAS_MII, 64},
	{"3Com Boomerang (?)",			0x10B7, 0x9000, 0xff00, IS_BOOMERANG, 64},
	{0, 0, 0, 0, 0, 0},				/* 0 terminated list. */
};

/* The action to take with a media selection timer tick.
 * Note that we deviate from the 3Com order by checking 10base2 before AUI.
 */
enum xcvr_types {
	XCVR_10baseT	= 0,
	XCVR_AUI, XCVR_10baseTOnly, XCVR_10base2, XCVR_100baseTx, XCVR_100baseFx,
	XCVR_MII	= 6,
	XCVR_NWAY	= 8,
	XCVR_ExtMII	= 9,
	XCVR_Default	= 10,
};

struct media_table {
	char *name;
	uint32 media_bits: 16,	/* Bits to set in Wn4_Media register. */
		mask: 8,			/* The transceiver-present bit in Wn3_Config.*/
		next: 8; 			/* The media type to try next. */
	int wait;				/* Time before we check media status. */
}
media_tbl[] = {
	{ "10baseT",		Media_10TP,	0x08, XCVR_10base2,		( 14 * HZ ) / 10},
	{ "10Mbs AUI",		Media_SQE,	0x20, XCVR_Default,		( 1 * HZ ) / 10},
	{ "undefined",		0,			0x80, XCVR_10baseT,		10000},
	{ "10base2",		0,			0x10, XCVR_AUI,			( 1 * HZ ) / 10},
	{ "100baseTX",		Media_Lnk,	0x02, XCVR_100baseFx,	( 14 * HZ ) / 10},
	{ "100baseFX",		Media_Lnk,	0x04, XCVR_MII,			( 14 * HZ ) / 10},
	{ "MII",			0,			0x41, XCVR_10baseT,		3*HZ },
	{ "undefined",		0,			0x01, XCVR_10baseT,		10000},
	{ "Autonegotiate",	0,			0x41, XCVR_10baseT,		3*HZ},
	{ "MII-External",	0,			0x41, XCVR_10baseT,		3*HZ },
	{ "Default",		0,			0xFF, XCVR_10baseT,		10000},
};

#include "tcm9xx_io.c"
#include "tcm9xx_mem.c"
#include "tcm9xx_acpi.c"
#include "tcm9xx_mdio.c"
#include "tcm9xx_init.c"
#include "tcm9xx_media.c"
#include "tcm9xx_xcvr.c"
#include "tcm9xx_ints.c"

/*
 * check_device
 *
 * Return B_OK iff dev is supported by this driver.
 *
 */
static status_t check_device( const pci_info* dev, uint32* devIDptr ) {
	uint16 free_slot_index;
	uint16	chip_idx;
	pci_info	*new_item;

	DEBUG_MSG( FUNCTION, "check_device ENTRY\n" );

	/* 
	 * Find the card in our list of supported cards above 
	 */
	for ( chip_idx = 0; pci_dev_table[ chip_idx ].vendor_id; chip_idx++ ) {
		if ( dev->vendor_id == pci_dev_table[ chip_idx ].vendor_id &&
				( dev->device_id & pci_dev_table[ chip_idx ].device_id_mask ) == pci_dev_table[ chip_idx ].device_id ) {
			break;
		}
	}

	if ( pci_dev_table[ chip_idx ].vendor_id != 0 ) {
		/* 
		 * check if the device really has an IRQ 
		 */
		if ( ( dev->u.h0.interrupt_line == 0 ) || ( dev->u.h0.interrupt_line == 0xFF ) ) {
			DEBUG_MSG( FUNCTION, "check_device EXIT (found with invalid IRQ)" );
			return B_ERROR;
		}

		/* 
		 * This message is *very* helpful when diagnosing card-specific problems...make
		 * sure it's still there in the final release (for your own sanity's sake...). 
		 */
		DEBUG_MSG( ERR, "%s found at IRQ %d \n", pci_dev_table[ chip_idx ].name, dev->u.h0.interrupt_line );

		for ( free_slot_index = 0; free_slot_index < MAX_DEVICES; free_slot_index++ ) {
			if ( gDev[ free_slot_index ].dev_pci_info == NULL ) {
				new_item = ( pci_info * ) malloc( sizeof( pci_info ) );
				if ( new_item == NULL ) {
					return B_NO_MEMORY;
				}
				memcpy( new_item, dev, sizeof( pci_info ) );
				gDev[ free_slot_index ].dev_pci_info = new_item;

				if ( devIDptr != NULL ) {
					*devIDptr = free_slot_index;
					/* Set the bus type to CardBus since we're being called by cs_attach() */
					gDev[ free_slot_index ].dev_flags |= DevBusCardBus;
				} else {
					/* Set the bus type to PCI since we're being called by get_pci_list() */
					gDev[ free_slot_index ].dev_flags |= DevBusPCI;
				}

				/* Indicate that there is now a piece of hardware present in our free slot */
				gDev[ free_slot_index ].dev_flags |= DevHardwarePresent;
				gDev[ free_slot_index ].dev_other |= ( chip_idx & TCM9XX__CHIP_IDX_MSK );

				DEBUG_MSG( FUNCTION, "check_device EXIT (Found Device) \n" );

				return B_OK;
			}
		}
	}

	DEBUG_MSG( FUNCTION, "check_device EXIT (not found)\n" );
	return B_ERROR;
}


/*
 * open_device
 *
 * Fill in dev, given flags, and return B_OK if everything went
 * as planned.
 *
 */
static status_t open_device( device_info_t *device, uint32 flags ) {
	int i;
	cpu_status cpu;
	TIME_SPINLOCK_DEFS;

	( void ) flags;

	/* 
	 * Set the mtu variable in the device structure to the default max frame
	 * size.  The 3Com cards allow for large frame transmits and we can later
	 * add adjustable mtu sizes through ioctl calls. 
	 */
	device->mtu = 1500;
	device->chip_idx = gDev[ device->devID ].dev_other & TCM9XX__CHIP_IDX_MSK;
	device->card_flags = pci_dev_table[ device->chip_idx ].flags;

	if ( tcm9xx_initialize_locks( device ) != B_OK ) {
		goto od_err1;
	}

	tcm9xx_ACPI_wake( device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function );

	if ( tcm9xx_enable_addressing( device ) != B_OK ) {
		goto od_err2;
	}

	tcm9xx_activate_xcvr( device );

	if ( tcm9xx_read_eeprom_data( device ) != B_OK ) {
		goto od_err2;
	}
	if ( tcm9xx_select_media_port( device ) != B_OK ) {
		goto od_err2;
	}

	/* Allow status bits to be seen. */
	DEBUG_DEVICE( INFO, "Enabling %s and %s interrupts for Tx and Rx\n", device,
			( device->full_bus_master_tx ? "DownComplete" : "TxAvailable" ),
			( device->full_bus_master_rx ? "UpComplete" : "RxComplete" ) );

	device->status_enable = SetStatusEnb | HostError | IntReq | StatsFull | TxComplete |
							( device->full_bus_master_tx ? DownComplete : TxAvailable ) |
							( device->full_bus_master_rx ? UpComplete : RxComplete ) |
							( device->bus_master ? DMADone : 0 );
	device->intr_enable = SetIntrEnb | IntLatch | RxComplete | StatsFull | TxAvailable |
		HostError | TxComplete  | IntReq | UpComplete | DownComplete;

	tcm9xx_set_media_type( device );

	if ( tcm9xx_reset_device( device ) != B_OK ) {
		goto od_err2;
	}

	/* Install the interrupt handler */
	DEBUG_DEVICE( INFO, "Installing interrupt handler on IRQ %d\n", device, device->pciInfo->u.h0.interrupt_line );
	if ( install_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, tcm9xx_interrupt_handler, device, 0 ) != B_NO_ERROR )
		dprintf( "tcm9xx/%d: VERY VERY BAD!!!! install_io_interrupt_handler FAILED!", (int)device->devID );

	TIME_SPINLOCK_START;
	cpu = disable_interrupts();
	acquire_spinlock( &device->spin_window_lock );

	/* Switch to the stats window, and clear all stats by reading. */
	write16( device->base + Command, StatsDisable );
	SET_WINDOW( 6 );
	for ( i = 0; i < 10; i++ )
		read8( device->base + i );
	read16( device->base + 10 );
	read16( device->base + 12 );
	/* On the Vortex we must also clear the BadSSD counter. */
	SET_WINDOW( 4 );
	read8( device->base + 12 );
	/* ..and on the Boomerang we enable the extra statistics bits. */
	write16( device->base + Wn4_NetDiag, 0x0040 );

	/* Switch to register set 7 for normal use. */
	SET_WINDOW( 7 );

	release_spinlock( &device->spin_window_lock );
	restore_interrupts( cpu );
	TIME_SPINLOCK_STOP;

	/* allocate and initialize frame buffer rings & descriptors */
	if ( device->full_bus_master_rx && device->full_bus_master_tx ) {
		uint32	dma_control;

		device->tx_current = device->tx_dirty = 0;
		device->rx_current = device->rx_dirty = 0;

		write16( device->base + Command, SetRxThreshold | ( MAX_FRAME_SIZE >> 2 ) );
		write32( device->base + PktStatus, 0x0020 );
		if ( tcm9xx_init_ring_buffers( device ) != B_OK ) {
			goto od_err2;
		}

		device->card_tx = &tcm9xx_boomerang_start_xmit;		// boomerang-specific transmit function
		device->card_rx = &tcm9xx_boomerang_rx;  			//	 "		 "	receive	  "

		write8( device->base + UpPollRate, 10 );
		if (device->card_flags & IS_BOOMERANG) {
			write8( device->base + TxFreeThreshold, ( MAX_FRAME_SIZE >> 8 ) );  	/* Room for a packet. */
		} else {
			write8( device->base + DownPollRate, 20 );
		}

		// Install ring buffers on the card.  TX DPD buffers are set up on transmit.
		write32( device->base + UpListPtr, ring_virt_to_bus( device, ( void* ) device->rx_desc[ 0 ] ) );
		write32( device->base + DownListPtr, 0 );

		dma_control = read32( device->base + PktStatus );
		DEBUG_DEVICE( ERR, "after writing up/down pointers, dmacontrol=%8.8x\n", device, ( int ) dma_control );

	} else {
		device->card_tx = &tcm9xx_vortex_start_xmit;
		device->card_rx = &tcm9xx_vortex_rx;
	}

	tcm9xx_set_rx_mode( device );

	device->tx_busy = 0;
	device->interrupt = 0;
	device->start = 1;

	tcm9xx_start_operation( device );

	/* Install the kernel thread which will poll the active media port... */
	device->media_poll_thread_id = spawn_kernel_thread( tcm9xx_media_poll_thread,
			"tcm9xx_media_poll_thread", B_NORMAL_PRIORITY, ( void* ) device );
	device->media_poll_thread_running = 1;
	resume_thread( device->media_poll_thread_id );

	return B_OK;

	/* Error handling */
od_err2:
	tcm9xx_uninitialize_locks( device );
od_err1:
	return B_ERROR;
}

/*
 * stop_device
 *
 * The device's hardware has been removed and the driver has not
 * yet been closed properly with 'close_device'.  Shut down any
 * unnecessary waiting/blocking (e.g. in the write_device hook)
 *
 */
static status_t stop_device( device_info_t *device ) {
	/* Delete the transmit lock so that transmits don't block */
	gFC->delete_fc_lock( &device->fc_tx_lock );

	/* Delete the media polling lock so that we don't get any errors from
	 * touching nonexistent hardware */
	delete_sem( device->sem_media_poll );

	return B_OK;
}

/*
 * close_device
 *
 * Tell all pending operations to abort and unblock all blocked ops.
 * DO NOT deallocate anything, as some ops could still be pending.
 *
 */
static status_t close_device( device_info_t *device ) {
	status_t	status;

	/* Uninitialize all our locks */
	tcm9xx_uninitialize_locks( device );

	/* Shut down our media polling thread */
	device->media_poll_thread_running = 0;
	wait_for_thread( device->media_poll_thread_id, &status );

	/* Power down the card and leave it in Wake-On-Lan mode */
	if ( device->eep_capabilities & CapPwrMgmt ) {
		tcm9xx_ACPI_set_WOL( device );
	}

	return B_OK;
}

/*
 * free_device
 *
 * All operations have completed, no more will be called.  De-
 * allocate anything you allocated (in open() or elsewhere).
 *
 */
static status_t free_device( device_info_t *device ) {

	/* Remove the interrupt handler */
	remove_io_interrupt_handler( device->pciInfo->u.h0.interrupt_line, tcm9xx_interrupt_handler, device );

	/* Free all our ring buffers */
	tcm9xx_free_ring_buffers( device );

	return B_OK;
}

/*
 * control_device
 *
 * Handle ioctl().
 *
 */
static status_t
control_device( device_info_t *device, uint32 msg, void *buf, size_t len ) {
	DEBUG_DEVICE( FUNCTION, "control_device ENTRY\n", device );

	( void ) len;

	switch ( msg ) {
		case ETHER_GETADDR:
			{
				uint8 i;
				for ( i = 0; i < 6; i++ )
					( ( uint8 * ) buf ) [ i ] = device->mac_address.ebyte[ i ];
				goto control_device_done;
			}
		case ETHER_INIT:
			goto control_device_done;

		case ETHER_GETFRAMESIZE:
			*( uint32 * ) buf = MAX_FRAME_SIZE;
			goto control_device_done;

		case ETHER_ADDMULTI:
			//tcm9xx_add_multicast(device, (char *) buf);

			goto control_device_done;

		case ETHER_SETPROMISC:
			//tcm9xx_set_promiscuous(device, *(uint32 *)buf );


			goto control_device_done;

		case ETHER_NONBLOCK:
			if ( *( ( int32 * ) buf ) )
				device->block_flag = B_TIMEOUT;
			else
				device->block_flag = 0;

			/* TODO: remove this if ETHER_NONBLOCKs are common */
			DEBUG_DEVICE( FUNCTION, "ETHER_NONBLOCK called!! -- WHY?!\n", device );

			goto control_device_done;

		default:
			DEBUG_DEVICE( FUNCTION, "control_hook EXIT (Error - Unknown IOCTL %x)\n", device, ( int ) msg );
			return B_ERROR;
	}
control_device_done:
	DEBUG_DEVICE( FUNCTION, "control_device EXIT\n", device );
	return B_OK;
}

/*
 * read_device
 *
 * Handle read().
 *
 */
static status_t
read_device( device_info_t *device, off_t pos, void *buf, size_t *len ) {
	status_t	status;

	( void ) pos;

	DEBUG_DEVICE( FUNCTION, "read_device ENTRY\n", device );

	status = ( *device->card_rx ) ( device, buf, len );


	DEBUG_DEVICE( FUNCTION, "read_device EXIT (buf=0x%8x len=0x%4x(%d)\n", device, ( int ) buf, ( int ) * len, ( int ) * len );

	return status;
}

/* dump ethernet packets */
void dump_packet( const char * msg, unsigned char * buf, uint16 size ) {
	uint16 j;

	dprintf( "%s dumping %p size %d", msg, buf, size );
	for ( j = 0; j < size; j++ ) {
		if ( ( j & 0xF ) == 0 ) dprintf( "\n" );
		dprintf( "%2.2x ", buf[ j ] );
	}
	dprintf( "\n" );
}

/*
 * write_device
 *
 * Handle write().
 *
 */
static status_t
write_device( device_info_t *device, off_t pos, const void *buf, size_t *len ) {
	status_t	status;

	( void ) pos;

	/* TODO: check for valid write sizes */

	if ( ( status = gFC->fc_wait( &device->fc_tx_lock, B_INFINITE_TIMEOUT ) ) != B_NO_ERROR ) {
		*len = 0;
		return status;
	}

	status = ( *device->card_tx ) ( device, buf, *len );

	if ( device->debug & TX ) {
		dump_packet( "rx: ", ( uint8* ) buf, 16 );
	}

	return status;
}

/*
 * tcm9xx: Serial Debugger command
 * Connect a terminal emulator to the serial port at 19.2 8-1-None
 * hit the Alt + SysRq + D keys, and type "tcm9xx <CR>" 
 *
 */
#ifdef DEBUG
static int
device_debugcmd( int argc, char **argv ) {
	uint16 i, j;
	int	id;
	device_info_t *device = gDev[ gDevID ].dev_info;

	const char *usage = "usage: tcm9xx [<0-9>fnrtpio] (? for help)\n";

	if ( argc < 2 ) {
		kprintf( "%s", usage );
		return 0;
	}

	for ( i = argc, j = 1; i > 1; i--, j++ ) {
		switch ( *argv[ j ] ) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				char * temp_str = "";
				char tempchar = *argv[ j ];

				id = atoi( &tempchar );
				if ( ( id < MAX_DEVICES ) && ( gDev[ id ].dev_info != NULL ) ) {
					gDevID = id;
					device = gDev[ gDevID ].dev_info;

					if ( gDev[ gDevID ].dev_flags & DevBusPCI ) strcat( temp_str, "PCI" );
					if ( gDev[ gDevID ].dev_flags & DevBusCardBus ) strcat( temp_str, "CardBus" );
					if ( gDev[ gDevID ].dev_flags & DevBusISA ) strcat ( temp_str, "ISA" );
					if ( gDev[ gDevID ].dev_flags & DevBusPCMCIA ) strcat ( temp_str, "/PCMCIA" );

					kprintf( "Current card set to %d (%s)\n", id, temp_str );
					kprintf( "Driver flags are: " );
					if ( gDev[ gDevID ].dev_info->card_flags & IS_VORTEX ) strcat( temp_str, "| IS_VORTEX " );
					if ( gDev[ gDevID ].dev_info->card_flags & IS_BOOMERANG ) strcat( temp_str, "| IS_BOOMERANG " );
					if ( gDev[ gDevID ].dev_info->card_flags & IS_CYCLONE ) strcat( temp_str, "| IS_CYCLONE " );
					if ( gDev[ gDevID ].dev_info->card_flags & IS_TORNADO ) strcat( temp_str, "| IS_TORNADO " );
					if ( gDev[ gDevID ].dev_info->card_flags & HAS_PWR_CTRL ) strcat( temp_str, "| HAS_PWR_CTRL " );
					if ( gDev[ gDevID ].dev_info->card_flags & HAS_MII ) strcat( temp_str, "| HAS_MII " );
					if ( gDev[ gDevID ].dev_info->card_flags & HAS_NWAY ) strcat( temp_str, "| HAS_NWAY " );
					if ( gDev[ gDevID ].dev_info->card_flags & HAS_CB_FNS ) strcat( temp_str, "| HAS_CB_FNS " );
					if ( gDev[ gDevID ].dev_info->card_flags & EEPROM_8BIT ) strcat( temp_str, "| EEPROM_8BIT " );
					if ( gDev[ gDevID ].dev_info->card_flags & INVERT_LED_PWR ) strcat( temp_str, "| INVERT_LED_PWR " );
					if ( gDev[ gDevID ].dev_info->card_flags & MII_XCVR_PWR ) strcat( temp_str, "| MII_XCVR_PWR " );
					kprintf( "%s\n", temp_str );
				} else {
					kprintf( "Card %d doesn't exist!\n", id );
				}
				break;
			}
		case '?':
			kprintf( kDevName " command help:\n" );
			kprintf( "%s", usage );
			kprintf( "? - this help display\n" );
			kprintf( "f - function trace display\n" );
			kprintf( "n - rx ping sequence number display\n" );
			kprintf( "r - receive packet trace\n" );
			kprintf( "t - transmit packet trace\n" );
			kprintf( "p - PCI I/O trace\n" );
			kprintf( "i - interrupt trace\n" );
			kprintf( "o - info trace\n" );
			kprintf( "\n" );
		case 'f':
			gDev[ gDevID ].dev_info->debug ^= FUNCTION;
			if ( gDev[ gDevID ].dev_info->debug & FUNCTION )
				kprintf( "Function trace Enabled\n" );
			else
				kprintf( "Function trace Disabled\n" );
			break;
		case 'o':
			gDev[ gDevID ].dev_info->debug ^= INFO;
			if ( gDev[ gDevID ].dev_info->debug & INFO )
				kprintf( "info trace Enabled\n" );
			else
				kprintf( "info trace Disabled\n" );
			break;
		case 'i':
			gDev[ gDevID ].dev_info->debug ^= INTERRUPT;
			if ( gDev[ gDevID ].dev_info->debug & INTERRUPT )
				kprintf( "interrupt trace Enabled\n" );
			else
				kprintf( "interrupt trace Disabled\n" );
			break;
		case 'n':
			gDev[ gDevID ].dev_info->debug ^= SEQ;
			if ( gDev[ gDevID ].dev_info->debug & SEQ )
				kprintf( "rx ping sequence Numbers Enabled\n" );
			else
				kprintf( "rx ping sequence Numbers Disabled\n" );
			break;
		case 'r':
			gDev[ gDevID ].dev_info->debug ^= RX;
			if ( gDev[ gDevID ].dev_info->debug & RX )
				kprintf( "Recieve packet trace Enabled\n" );
			else
				kprintf( "Receive packet trace Disabled\n" );
			break;
		case 't':
			gDev[ gDevID ].dev_info->debug ^= TX;
			if ( gDev[ gDevID ].dev_info->debug & TX )
				kprintf( "Transmit packet trace Enabled\n" );
			else
				kprintf( "Transmit packet trace Disabled\n" );
			break;
		case 's':
			kprintf( "tcm9xx STATISTICS\n" );
			kprintf( "=================\n" );
			PRINT_STAT( queued_packet );
			kprintf( "RX STATS:\n" );
			kprintf( "---------\n" );
			PRINT_STAT( rx_errors ); PRINT_STAT( rx_dropped ); PRINT_STAT( rx_over_errors );
			PRINT_NEW_LINE;
			PRINT_STAT( rx_length_errors ); PRINT_STAT( rx_frame_errors ); PRINT_STAT( rx_crc_errors );
			PRINT_NEW_LINE;
			PRINT_STAT( rx_fifo_errors ); PRINT_STAT( fixing_signal );
			PRINT_NEW_LINE;
			PRINT_STAT( rx_triggers ); PRINT_STAT( rx_acquires );
			PRINT_NEW_LINE;
			PRINT_STAT( rx_packets ); PRINT_STAT( rx_bytes ); 
			PRINT_NEW_LINE;
			PRINT_NEW_LINE;
			kprintf( "TX STATS:\n" );
			kprintf( "---------\n" );
			PRINT_STAT( tx_fifo_errors ); PRINT_STAT( tx_dropped ); PRINT_STAT( tx_errors );
			PRINT_NEW_LINE;
			PRINT_STAT( tx_aborted_errors ); PRINT_STAT( tx_carrier_errors ); PRINT_STAT( tx_heartbeat_errors );
			PRINT_NEW_LINE;
			PRINT_STAT( tx_window_errors );
			PRINT_NEW_LINE;
			PRINT_STAT( tx_packets ); PRINT_STAT( tx_window_errors ); 
			PRINT_NEW_LINE;
			PRINT_NEW_LINE;
			kprintf( "OTHER STATS:\n" );
			kprintf( "------------\n" );
			PRINT_STAT( command_timeouts );
			PRINT_NEW_LINE;
			kprintf( "max_trigger_time %Ld, max_reader_time %Ld\n", device->max_trig_time, device->max_reader_time  );
			kprintf( "fc_tx_lock.count %d\n", (int)device->fc_tx_lock.count  );
			kprintf( "=================\n" );
			break;
		default:
			kprintf( "%s", usage );
			return 0;
		}
	}

	return 0;
}
#endif
