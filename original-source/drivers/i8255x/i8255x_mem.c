/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_mem.c
 * Description:			8255x memory-related functions.
 *
 * Revision History:	March 29, 2001	- Initial Revision
 *****************************************************************************/

/*
 * i8255x_enable_addressing()
 * Description: Enable the card's address space
 */
void i8255x_enable_addressing( device_info_t *device ) {
	int	i;

	FUNCTION_ENTRY;

	set_bus_mastering( device->pciInfo );

	/*
	 * Enable the card's address space
	 */
	for ( i = 0; i < 5; i++ ) {
		/* Check for a mem-mapped I/O address to use */
		if ( !( device->pciInfo->u.h0.base_register_flags[ i ] & 0x01 ) ) {
			device->reg_base_area = map_pci_memory( device, i, ( void** )&device->base );
			break;
		}
		/* Just bail when we see a zero-address */
		if ( device->pciInfo->u.h0.base_registers[ i ] == 0 ) {
			break;
		}
	}
	if ( i == 5 ) {
		DEBUG_DEVICE( ERR, "Error mapping PCI memory.\n", device );
	}

	FUNCTION_EXIT;
}
