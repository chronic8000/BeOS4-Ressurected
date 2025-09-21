/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2001 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_mem.c
 * Description:	Memory mapping routines
 * Changes:		Feb 14, 2001	- Initial Revision [chrisl]
 *
 *****************************************************************************/

/*
 * virt_to_bus()
 * Description: map a virtual address to a bus address
 *
 */
long virt_to_bus( void *address, uint32 size ) {
    physical_entry entry;
    get_memory_map( ( uint8* ) address, size, &entry, 1 );
    return B_HOST_TO_LENDIAN_INT32( ( long ) gPCIModInfo->ram_address( entry.address ) );
}


/*
 * ring_init_addr_table()
 * Description: initialize the address table
 *
 */
void ring_init_addr_table( device_info_t *device ) {
    int i;

    for ( i = 0; i < ( TX_BUFFERS + RX_BUFFERS ); i++ ) {
        device->paddr_table[ i ] = 0;
        device->vaddr_table[ i ] = NULL;
    }
}


/*
 * ring_map_n_store_addr()
 * Description: Map and store the specified address
 *
 */
int32 ring_map_n_store_addr( device_info_t *device, uint8 *v_addr, int32 v_size ) {
    physical_entry entry;
    int i;

    for ( i = 0; i < ( TX_BUFFERS + RX_BUFFERS ); i++ ) {
        if ( !device->vaddr_table[ i ] ) {			  // get 1st free
            device->vaddr_table[ i ] = v_addr;

            /* get the physical entry */
            get_memory_map( v_addr, v_size, &entry, 1 );

            /* store the bus address */
            device->paddr_table[ i ] = B_HOST_TO_LENDIAN_INT32( gPCIModInfo->ram_address( entry.address ) );

            DEBUG_DEVICE( MEM, "mapping %#8.8x(virt) to %#8.8x(phys) with passed in size of %#8.8x\n",
                          device, ( int ) device->vaddr_table[ i ], ( int ) device->paddr_table[ i ], ( int ) v_size );

            return TRUE;
        }
    }

    return FALSE;
}


/*
 * ring_bus_to_virt()
 * Description: Map a bus address to its virtual address
 *
 */
void* ring_bus_to_virt( device_info_t *device, uint32 ph_addr ) {
    int i;

    DEBUG_DEVICE( MEM, __FUNCTION__ " ENTRY (looking up %x)\n", device, ( int ) ph_addr );

    for ( i = 0; i < ( TX_BUFFERS + RX_BUFFERS ); i++ ) {
        if ( device->paddr_table[ i ] == ph_addr ) {
            return ( ( void* ) device->vaddr_table[ i ] );
        }
    }

    /* Not found */
    return ( NULL );
}


/*
 * ring_virt_to_bus()
 * Description: Map a virtual address to a bus address
 *
 */
uint32 ring_virt_to_bus( device_info_t *device, void* v_addr ) {
    int i;

    DEBUG_DEVICE( MEM, __FUNCTION__ " ENTRY (looking up %p)\n", device, v_addr );

    for ( i = 0; i < ( TX_BUFFERS + RX_BUFFERS ); i++ ) {
        if ( device->vaddr_table[ i ] == v_addr ) {
            return ( device->paddr_table[ i ] );
        }
    }

    /* Not found */
    return 0;
}
