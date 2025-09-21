/******************************************************************************
 * Intel i8255x (82557, 58, 59) Ethernet Controller Driver
 * Copyright (c) 2001 Be Incorporated, All Rights Reserved.
 *
 * Written by: Chris Liscio <liscio@be.com>
 *
 * File:				i8255x_proto.h
 * Description:			8255x function prototypes
 *
 * Revision History:	March 30, 2001	- Initial Revision
 *****************************************************************************/

#ifndef _I8255X_PROTO_H
#define _I8255X_PROTO_H

/* Forward Declaration of the device_info_t struct */
typedef struct device_info_t device_info_t;

/******************************************************************************
 * i8255x_eeprom.c
 *****************************************************************************/
void i8255x_get_eeprom_settings( device_info_t *device );
uint32 i8255x_do_eeprom_cmd( device_info_t *device, int cmd, int cmd_len );

/******************************************************************************
 * i8255x_mem.c
 *****************************************************************************/
void i8255x_enable_addressing( device_info_t *device );

#endif // _I8255X_PROTO_H
