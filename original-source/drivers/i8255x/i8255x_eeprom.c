/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_eeprom.c
 * Description:			EEPROM Reading routines.
 *
 * Revision History:	March 29, 2001	- Initial Revision
 *****************************************************************************/

/* Serial EEPROM defines */
#define EE_SHIFT_CLK		0x01				// EEPROM shift clock.
#define EE_CS				0x02				// EEPROM chip select.
#define EE_DATA_WRITE		0x04				// EEPROM chip data in.
#define EE_DATA_READ		0x08				// EEPROM chip data out.
#define EE_ENB				(0x4800 | EE_CS)
#define EE_WRITE_0			0x4802
#define EE_WRITE_1			0x4806
#define EE_READ_CMD			(6)					// The only opcode we use
#define EEPROM_CHECKSUM		0xBABA				// Intel-specified checksum

#define eeprom_delay()		read16( device->base + CSR_EEP_CTRL )


/*
 * i8255x_get_eeprom_settings()
 * Description: Read (and decode) the chip's settings out of the EEPROM
 */
void i8255x_get_eeprom_settings( device_info_t *device ) {
	uint8	mac_count;
	uint16	eeprom_size;
	uint16	eeprom_count;
	uint16	eeprom_data[ 0x100 ];
	uint16	eeprom_checksum;
	uint32	read_cmd;

	FUNCTION_ENTRY;

	/*
	 * Determine the EEPROM size
	 */
	if ( ( i8255x_do_eeprom_cmd( device, EE_READ_CMD << 24, 27 ) & 0xFFE0000 ) == 0xFFE0000 ) {
		eeprom_size = 0x100;
		read_cmd = EE_READ_CMD << 24;
	} else {
		eeprom_size = 0x40;
		read_cmd = EE_READ_CMD << 22;
	}

	/*
	 * Read the checksum data out one word at a time, summing each word
	 * for checksum calculation and grabbing the mac address.
	 */
	for( eeprom_count = 0, mac_count = 0; eeprom_count < eeprom_size; eeprom_count++ ) {
		uint16 value = i8255x_do_eeprom_cmd( device, read_cmd | ( eeprom_count << 16 ), 27 );
		eeprom_data[ eeprom_count ] = value;
		eeprom_checksum += value;
		if ( eeprom_count < 3 ) {
			device->mac_address.ebyte[ mac_count++ ] = value;
			device->mac_address.ebyte[ mac_count++ ] = value >> 8;
		}
	}

	/* 
	 * Dump the EEPROM data for debugging purposes 
	 */
#if DEBUG
	if ( device->debug & INFO ) {
		dprintf( kDevName "/%d EEPROM Data:\n", (int)device->devID );
		for ( eeprom_count = 0; eeprom_count < eeprom_size; eeprom_count++ ) {
			dprintf( "%4.4x ", eeprom_data[ eeprom_count ] );
			if ( ( eeprom_count % 8 ) == 0 ) {	
				dprintf( "\n" );	// We want 8 words per line here
			}
		}
	}
#endif // DEBUG

	/* 
	 * Check the checksum 
	 */
	if ( eeprom_checksum != EEPROM_CHECKSUM ) {
		DEBUG_DEVICE( ERR, "INVALID CHECKSUM %4.4x\n", device, (int)eeprom_checksum );
	}

	/* 
	 * Dump the MAC address for debug purposes
	 */
#ifdef DEBUG
	{
		int	i;
		dprintf( kDevName "/%d MAC Address:", (int)device->devID );
		for ( i = 0; i < 6; i++ ) {
			dprintf( "%2.2x ", device->mac_address.ebyte[ i ] );
		}
		dprintf( "\n" );
	}
#endif // DEBUG

	FUNCTION_EXIT;
}

/*
 * i8255x_do_eeprom_cmd()
 * Description: Perform an eeprom command
 */
uint32 i8255x_do_eeprom_cmd( device_info_t *device, int cmd, int cmd_len ) {
	uint32	ee_addr = device->base + CSR_EEP_CTRL;
	uint32	retval = 0;

	FUNCTION_ENTRY;

	/*
	 * Enable the eeprom chip
	 */
	write16( ee_addr, EE_ENB | EE_SHIFT_CLK );

	/* 
	 * Shift the command bits out. 
	 */
	do {
		uint16	dataval = ( cmd & ( 1 << cmd_len ) ) ? EE_WRITE_1 : EE_WRITE_0;

		write16( ee_addr, dataval );
		eeprom_delay();
		write16( ee_addr, dataval | EE_SHIFT_CLK );
		eeprom_delay();
		retval = ( retval << 1 ) | ( ( read16( ee_addr ) & EE_DATA_READ) ? 1 : 0);
	} while ( --cmd_len >= 0 );

	write16( ee_addr, EE_ENB );

	/* 
	 * Terminate the EEPROM access. 
	 */
	write16( ee_addr, EE_ENB & ~EE_CS );

	FUNCTION_EXIT;

	return retval;
}

