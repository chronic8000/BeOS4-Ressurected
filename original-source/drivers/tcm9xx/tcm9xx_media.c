/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2001 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_media.c
 * Description:	tcm9xx media polling thread
 * Changes:		March 12, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

/*
 * tcm9xx_media_poll_thread()
 * Description:	Polls the hardware to discover the currently active media type
 * Note:		This function is set to run pretty much every 5 seconds.  The
 * 				acquire_sem() call has a timeout which will pretty much always
 * 				run out so that the thread can poll as normal.  This is better
 * 				than using a timer interrupt as we don't want to waste time
 * 				with interrupts held off 
 *
 */
status_t tcm9xx_media_poll_thread( void *_device ) {
	device_info_t * device = ( device_info_t * ) _device;
	int ok = 0;
	int media_status, mii_status, old_window;
	cpu_status	cpu;
	status_t	status;
	TIME_SPINLOCK_DEFS;

	while ( device->media_poll_thread_running ) {
		status = acquire_sem_etc( device->sem_media_poll, 1, B_RELATIVE_TIMEOUT, media_tbl[ device->if_port ].wait );

		if ( status == B_INTERRUPTED ) {
			DEBUG_DEVICE( INFO, "tcm9xx_media_poll_thread EXIT (interrupted %x)\n", device, ( int ) status );
			return B_OK;
		}

		DEBUG_DEVICE( INFO, "tcm9xx_media_poll_thread ENTRY (%s)\n", device, media_tbl[ device->if_port ].name );

		TIME_SPINLOCK_START;

		cpu = disable_interrupts();
		acquire_spinlock( &device->spin_window_lock );

		old_window = read16( device->base + Command ) >> 13;

		SET_WINDOW( 4 );

		media_status = read16( device->base + Wn4_Media );

		switch ( device->if_port ) {
		case XCVR_10baseT:
		case XCVR_100baseTx:
		case XCVR_100baseFx:
			if ( media_status & Media_LnkBeat ) {
				ok = 1;
				DEBUG_DEVICE( INFO, "Media %s has link beat, %x.\n", device,
							  media_tbl[ device->if_port ].name, media_status );
			} else {
				DEBUG_DEVICE( ERR, "Media %s is has no link beat, %x.\n",
							  device, media_tbl[ device->if_port ].name, media_status );
			}
			break;
		case XCVR_MII:
		case XCVR_NWAY:
			mii_status = mdio_read( device, device->phys[ 0 ], 1 );
			ok = 1;
			DEBUG_DEVICE( INFO, "MII transceiver has status %4.4x.\n", device, mii_status );

			if ( mii_status & 0x0004 ) {
				int mii_reg5 = mdio_read( device, device->phys[ 0 ], 5 );

				if ( mii_reg5 != 0xffff ) {
					int duplex = ( mii_reg5 & 0x0100 ) || ( mii_reg5 & 0x01C0 ) == 0x0040;

					if ( device->full_duplex != duplex ) {
						device->full_duplex = duplex;
						DEBUG_DEVICE( INFO, "Setting %s-duplex based on MII #%d link partner capability of %4.4x.\n",
									  device, device->full_duplex ? "full" : "half", device->phys[ 0 ], mii_reg5 );

						/* Set the full-duplex bit. */
						write8( device->base + Wn3_MAC_Ctrl, ( device->full_duplex ? 0x20 : 0 ) | ( device->mtu > 1500 ? 0x40 : 0 ) );
					}
				}
			}
			break;
		default:  	/* Other media types handled by Tx timeouts. */
			DEBUG_DEVICE( ERR, "Media %s is has no indication, %x.\n", device, media_tbl[ device->if_port ].name, media_status );
			ok = 1;
		}

		if ( !ok ) {
			union wn3_config config;

			do {
				device->if_port = media_tbl[ device->if_port ].next;
			} while ( !( device->available_media & media_tbl[ device->if_port ].mask ) );

			if ( device->if_port == XCVR_Default ) { /* Go back to default. */
				device->if_port = device->default_media;
				DEBUG_DEVICE( ERR, "Media selection failing, using default %s port.\n", device, media_tbl[ device->if_port ].name );
			} else {
				DEBUG_DEVICE( ERR, "Media selection failed, now trying %s port.\n", device, media_tbl[ device->if_port ].name );
			}
			write16( device->base + Wn4_Media, ( media_status & ~( Media_10TP | Media_SQE ) ) | media_tbl[ device->if_port ].media_bits );

			SET_WINDOW( 3 );
			config.i = read32( device->base + Wn3_Config );
			config.u.xcvr = device->if_port;
			write32( device->base + Wn3_Config, config.i );

			write16( device->base + Command, ( ( device->if_port ) == XCVR_10base2 ? StartCoax : StopCoax ) );
		}

		SET_WINDOW( old_window );

		release_spinlock( &device->spin_window_lock );
		restore_interrupts( cpu );

		TIME_SPINLOCK_STOP;

		DEBUG_DEVICE( FUNCTION, "Media selection timer finished, %s.\n", device, media_tbl[ device->if_port ].name );

	}

	return B_OK;
}
