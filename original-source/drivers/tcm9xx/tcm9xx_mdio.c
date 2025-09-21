/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_mdio.c
 * Description:	MDIO Protocol Implementation
 * Changes:		Feb 21, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

/* MII transceiver control section.
 * Read and write the MII registers using software-generated serial
 * MDIO protocol.  See the MII specifications or DP83840A data sheet
 * for details. */

/* The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
 * met by back-to-back PCI I/O cycles, but we insert a delay to avoid
 * "overclocking" issues. */
#define mdio_delay() 		read32(mdio_addr)

#define MDIO_SHIFT_CLK		0x01
#define MDIO_DIR_WRITE		0x04
#define MDIO_DATA_WRITE0 	(0x00 | MDIO_DIR_WRITE)
#define MDIO_DATA_WRITE1 	(0x02 | MDIO_DIR_WRITE)
#define MDIO_DATA_READ		0x02
#define MDIO_ENB_IN			0x00

/*
 * mdio_sync()
 * Description: Generate the preamble required for initial synchronization and
 * 				a few older transceivers. 
 *
 */
void mdio_sync( device_info_t *device, int bits ) {
    int32 mdio_addr = device->base + Wn4_PhysicalMgmt;

    /* Establish sync by sending at least 32 logic ones. */
    while ( -- bits >= 0 ) {
        write16( mdio_addr, MDIO_DATA_WRITE1 );
        mdio_delay();
        write16( mdio_addr, MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK );
        mdio_delay();
    }
}

/*
 * mdio_read()
 * Description: Read from the transceiver
 *
 */
int mdio_read( device_info_t *device, int phy_id, int location ) {
    int i;
    int read_cmd = ( 0xf6 << 10 ) | ( phy_id << 5 ) | location;
    unsigned int retval = 0;
    int32 mdio_addr = device->base + Wn4_PhysicalMgmt;

    if ( mii_preamble_required )
        mdio_sync( device, 32 );

    /* Shift the read command bits out. */
    for ( i = 14; i >= 0; i-- ) {
        int dataval = ( read_cmd & ( 1 << i ) ) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
        write16( mdio_addr, dataval );
        mdio_delay();
        write16( mdio_addr, dataval | MDIO_SHIFT_CLK );
        mdio_delay();
    }
    /* Read the two transition, 16 data, and wire-idle bits. */
    for ( i = 19; i > 0; i-- ) {
        write16( mdio_addr, MDIO_ENB_IN );
        mdio_delay();
        retval = ( retval << 1 ) | ( ( read16( mdio_addr ) & MDIO_DATA_READ ) ? 1 : 0 );
        write16( mdio_addr, MDIO_ENB_IN | MDIO_SHIFT_CLK );
        mdio_delay();
    }
#if 0 
    return ( retval >> 1 ) & 0x1ffff;
#else 
return retval & 0x20000 ? 0xffff : retval >> 1 & 0xffff;
#endif
}

/*
 * mdio_write()
 * Description: write to the mdio transceiver
 *
 */
void mdio_write( device_info_t *device, int phy_id, int location, int value ) {
    int write_cmd = 0x50020000 | ( phy_id << 23 ) | ( location << 18 ) | value;
    int32 mdio_addr = device->base + Wn4_PhysicalMgmt;
    int i;

    if ( mii_preamble_required )
        mdio_sync( device, 32 );

    /* Shift the command bits out. */
    for ( i = 31; i >= 0; i-- ) {
        int dataval = ( write_cmd & ( 1 << i ) ) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
        write16( mdio_addr, dataval );
        mdio_delay();
        write16( mdio_addr, dataval | MDIO_SHIFT_CLK );
        mdio_delay();
    }
    /* Leave the interface idle. */
    for ( i = 1; i >= 0; i-- ) {
        write16( MDIO_ENB_IN, mdio_addr );
        mdio_delay();
        write16( mdio_addr, MDIO_ENB_IN | MDIO_SHIFT_CLK );
        mdio_delay();
    }

    return ;
}
