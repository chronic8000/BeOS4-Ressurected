/******************************************************************************\
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_proto.h
 * Description:	Function prototypes file
 * Changes:		Feb 14, 2000	- Initial Revision [chrisl]
\******************************************************************************/

#ifndef _TCM9XX_PROTO_H
#define _TCM9XX_PROTO_H

/* Forward Declaration of the device_info_t struct */
typedef struct device_info_t device_info_t;

/******************************************************************************\
 * tcm9xx_mem.c
\******************************************************************************/
long virt_to_bus( void *address, uint32 size );
void ring_init_addr_table( device_info_t *device );
int32 ring_map_n_store_addr( device_info_t *device, uint8 *v_addr, int32 v_size );
void* ring_bus_to_virt( device_info_t *device, uint32 ph_addr );
uint32 ring_virt_to_bus( device_info_t *device, void* v_addr );

/******************************************************************************\
 * tcm9xx_acpi.c
\******************************************************************************/
void tcm9xx_ACPI_wake( int bus, int device, int devfn );
void tcm9xx_ACPI_set_WOL( device_info_t *device );

/******************************************************************************\
 * tcm9xx_mdio.c
\******************************************************************************/
void mdio_sync( device_info_t *device, int bits );
int mdio_read( device_info_t *device, int phy_id, int location );
void mdio_write( device_info_t *device, int phy_id, int location, int value );

/******************************************************************************\
 * tcm9xx_init.c
\******************************************************************************/
status_t tcm9xx_initialize_locks( device_info_t *device );
status_t tcm9xx_uninitialize_locks( device_info_t *device );
status_t tcm9xx_enable_addressing( device_info_t *device );
status_t tcm9xx_read_eeprom_data( device_info_t *device );
status_t tcm9xx_select_media_port( device_info_t *device );
status_t tcm9xx_reset_device( device_info_t *device );
status_t tcm9xx_init_ring_buffers( device_info_t *device );
void tcm9xx_free_ring_buffers( device_info_t *device );
void tcm9xx_cmd_wait( device_info_t *device );

/******************************************************************************\
 * tcm9xx_media.c
\******************************************************************************/
status_t tcm9xx_media_poll_thread( void *_device );

/******************************************************************************\
 * tcm9xx_ints.c
\******************************************************************************/
static status_t tcm9xx_interrupt_handler( void *_device );
void tcm9xx_vortex_error( device_info_t *device, int status );
void tcm9xx_update_stats( device_info_t *device );

/******************************************************************************\
 * tcm9xx_xcvr.c
\******************************************************************************/
void tcm9xx_activate_xcvr( device_info_t *device );
status_t tcm9xx_vortex_rx( device_info_t *device, const void *buf, size_t *len );
status_t tcm9xx_boomerang_rx( device_info_t *device, const void *buf, size_t *len );
void tcm9xx_vortex_tx_timeout( device_info_t *device );
status_t tcm9xx_vortex_start_xmit( device_info_t *device, const void *buf, size_t len );
status_t tcm9xx_boomerang_start_xmit( device_info_t *device, const void *buf, size_t len );
void tcm9xx_signal_dirty_packets( device_info_t *device );

#endif // _TCM9XX_PROTO_H
