/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_defs.h
 * Description:			8255x driver defines
 *
 * Revision History:	April 4, 2001	- Initial Revision
 *****************************************************************************/

#ifndef _I8255X_DEFS_H
#define _I8255X_DEFS_H

/******************************************************************************
 * Driver operation defines
 *****************************************************************************/
#define MAX_MULTI		16	// Maximum multicast addresses

#define TX_BUFFERS		16	// Must be a power of two, for efficiency
#define RX_BUFFERS		32	// Must be a power of two, for efficiency

/******************************************************************************
 * I/O Defines
 *****************************************************************************/
#define read8(address)   				(*((volatile uint8*)(address)))
#define read16(address)  				(*((volatile uint16*)(address)))
#define read32(address) 				(*((volatile uint32*)(address)))

#define write8(address, data)  			(*((volatile uint8*)(address)) = (data))
#define write16(address, data) 			(*((volatile uint16*)(address)) = (data))
#define write32(address, data) 			(*((volatile uint32*)(address)) = (data))

/******************************************************************************
 * Debugging/Profiling Defines
 *****************************************************************************/
#ifdef DEBUG
#define FUNCTION_ENTRY	DEBUG_DEVICE( FUNCTION, __FUNCTION__ " ENTRY\n", device );
#define FUNCTION_EXIT	DEBUG_DEVICE( FUNCTION, __FUNCTION__ " EXIT\n", device );
#else
#define FUNCTION_ENTRY
#define FUNCTION_EXIT
#endif // DEBUG

#endif // _I8255X_DEFS_H
