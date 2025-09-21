/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_io.c
 * Description:	Basic (and useful) I/O routines
 * Changes:		March 12, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

/* Multiple read/write functions */
void read8s( uint32 address, uint8 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { data[ i ] = read32( address ); }
}
void read16s( uint32 address, uint16 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { data[ i ] = read32( address ); }
}
void read32s( uint32 address, uint32 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { data[ i ] = read32( address ); }
}
void write8s( uint32 address, uint8 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { write8( address, data[ i ] ); }
}
void write16s( uint32 address, uint16 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { write16( address, data[ i ] ); }
}
void write32s( uint32 address, uint32 *data, size_t length ) {
    uint32 i;
    for ( i = 0; i < length; i++ ) { write32( address, data[ i ] ); }
}

