/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_basics.c
 * Description:			Implementation of the basic driver operation functions
 *
 * Revision History:	March 30, 2001	- Initial Revision [chrisl]
 *****************************************************************************/

/******************************************************************************
 * IMPLEMENTATION NOTES:
 *****************************************************************************/

#include <OS.h>						// OS-Level defines
#include <SupportDefs.h>			// Support definitions (types, etc)
#include <KernelExport.h>			// Kernel exports
#include <PCI.h>					// PCI module use
#include <Drivers.h>				// useful driver functions
#include <stdlib.h>					// useful stuff
#include <stdio.h>					// more useful stuff...
#include <string.h>					// string manipulation
#include <fc_lock.h>				// fc_lock module
#include <common/ether_driver.h>	// ethernet driver types/defines

/* tcm9xx Driver-Specific Includes */
#include "i8255x_regs.h"			// i8255x-specific registers
#include "i8255x_proto.h"			// i8255x-specific function prototypes
#include "i8255x_defs.h"			// i8255x-specific defines

#define kDevName 		"i8255x"
#define kDevDir			"net/" kDevName "/"

#define DEBUG 			true 		// if you want debug output
#define MAX_DEVICES		4			// instances of this driver, max
#define SINGLE_OPEN					// if only one open session per
									// device is allowed at a time

struct device_info_t {
	// Required by driver_basics.h
	int32		devID;		// device identifier: 0-MAX_DEVICES
	pci_info	*pciInfo;	// info about this device
	uint32		debug;		// debugging level mask

	/*** device-specific data ***/

	/* CARD ADDRESS */
	ether_address_t	mac_address;			// MAC address of the NIC
	ether_address_t multi[ MAX_MULTI ];		// Multicast address list
	vuint8			multi_enabled;			// Multicast mode enabled

	/* MEMORY ADDRESSES */
	uint32		base;			// Base memory address of the card
	area_id		reg_area;		// Memory-mapped I/O area
	area_id		tx_desc_area;	// Transmit descriptor storage area
	area_id		tx_buf_area;	// Transmit buffer storage area
	area_id		rx_desc_area;  	// Receive descriptor storage area
	area_id		rx_buf_area;  	// Receive buffer storage area
	area_id		cb_fns_area;	// CardBus function pointer area
	area_id 	reg_base_area;	// Area storing the register base

	/* RING BUFFERS */
	uint8			*tx_buf[ TX_BUFFERS ]; 		// Transmit buffer
	uint8			*rx_buf[ RX_BUFFERS ]; 		// Receive buffer
	//boom_tx_desc	*tx_desc[ TX_BUFFERS ];		// Transmit descriptors
	//boom_rx_desc	*rx_desc[ RX_BUFFERS ];		// Receive descriptors

	/* ADDRESS LOOKUP TABLE */
	uint32	paddr_table[ TX_BUFFERS + RX_BUFFERS ];		// Physical address table
	uint8	*vaddr_table[ TX_BUFFERS + RX_BUFFERS ];  	// Virtual address table

	/* FLOW CONTROL */
	uint32	block_flag;
};

#include <common/ether_basics.h>
#include <common/driver_basics.c>
#include <common/device_acpi.c>

/******************************************************************************
 * List of supported devices
 *****************************************************************************/
struct _pci_device_info {
	const char	*name;
	uint16	vendor_id, device_id, device_id_mask;
	uint32	flags;
};

struct _pci_device_info pci_dev_table[] = {
	{"Intel PCI EtherExpress Pro100",			0x8086, 0x1229, 0xFFFF, 0},
	{"Intel EtherExpress Pro/100+ i82559ER",	0x8086, 0x1209, 0xFFFF, 0},
	{"Intel EtherExpress Pro/100 type 1029",	0x8086, 0x1029, 0xFFFF, 0},
	{"Intel EtherExpress Pro/100 type 1030",	0x8086, 0x1030, 0xFFFF, 0},
	{"Intel Pro/100 V Network",					0x8086, 0x2449, 0xFFFF, 0},
	{0, 0, 0, 0, 0},		/* 0 terminated list. */
};

/******************************************************************************
 * Included driver sources
 *****************************************************************************/
#include <i8255x_eeprom.c>
#include <i8255x_mem.c>

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
				//gDev[ free_slot_index ].dev_other |= ( chip_idx & I8255X__CHIP_IDX_MSK );

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
	/*
	 * Wake up the card from ACPI sleep mode (i.e. set to D0 mode )
	 */
	pci_set_power_state( device->pciInfo, ACPI_D0 );

	/*
	 * Enable the address space of the card
	 */
	i8255x_enable_addressing( device );

	/*
	 * Read and decode the card's EEPROM
	 */
	i8255x_get_eeprom_settings( device );

	return B_OK;
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
			goto control_device_done;

		case ETHER_SETPROMISC:
			goto control_device_done;

		case ETHER_NONBLOCK:
			if ( *( ( int32 * ) buf ) )
				device->block_flag = B_TIMEOUT;
			else
				device->block_flag = 0;

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
	return B_OK;
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

	const char *usage = "usage: " kDevName " [<0-9>fnrtpio] (? for help)\n";

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
			break;
		default:
			kprintf( "%s", usage );
			return 0;
		}
	}

	return 0;
}
#endif
