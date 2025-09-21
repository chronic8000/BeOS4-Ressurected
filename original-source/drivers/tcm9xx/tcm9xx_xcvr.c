/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2001 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_xcvr.c
 * Description:	Implementation of the transceiver functionality (rx/tx)
 * Changes:		March 12, 2001	- Initial Revision [chrisl]
 *
 *****************************************************************************/

/*
 * tcm9xx_activate_xcvr()
 * Description: Activate the transceiver on newer CardBus cards
 *
 */
void tcm9xx_activate_xcvr( device_info_t *device ) {
	int	reset_opts;

	SET_WINDOW(2);
	reset_opts = read16( device->base + Wn2_ResetOptions );
	if ( device->card_flags & INVERT_LED_PWR )
		reset_opts |= 0x0010;
	if ( device->card_flags & MII_XCVR_PWR )
		reset_opts |= 0x4000;
	write16( device->base + Wn2_ResetOptions, reset_opts );
}

/*
 * tcm9xx_vortex_rx()
 * Description: Vortex-specific receive routine 
 *
 */
status_t tcm9xx_vortex_rx( device_info_t *device, const void *buf, size_t *len ) {
	int16 rx_status;
	status_t status;
	bigtime_t trigger_time;

	( void ) len;

	DEBUG_DEVICE( FUNCTION, "tcm9xx_vortex_rx ENTRY\n", device );

	DEBUG_DEVICE( INFO, "In vortex_rx(), status %4.4x, rx_status %4.4x.\n", device,
				  read16( device-> base + IntStatus ), read16( device->base + RxStatus ) );

	if ( ( status = acquire_sem_etc( device->sem_rx_lock, 1, B_CAN_INTERRUPT | B_TIMEOUT, device->block_flag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT ) ) != B_OK ) {
		DEBUG_DEVICE( ERR, "fc_wait returned %s.  Bailing out of" __FUNCTION__ ".", device, strerror( status ) );
		*len = 0;
		return status;
	}
	trigger_time = system_time() - device->rx_triggered;
	if ( trigger_time > device->max_trig_time )
		device->max_trig_time = trigger_time;

	while ( ( rx_status = read16( device->base + RxStatus ) ) > 0 ) {
		if ( rx_status & 0x4000 ) {				  /* Error, update stats */
			unsigned char rx_error = read8( device->base + RxErrors );
			DEBUG_DEVICE( ERR, "Rx error: status %2.2x.\n", device, rx_error );
			INC_STATS( rx_errors );
			if ( rx_error & 0x01 ) INC_STATS( rx_over_errors );
			if ( rx_error & 0x02 ) INC_STATS( rx_length_errors );
			if ( rx_error & 0x04 ) INC_STATS( rx_frame_errors );
			if ( rx_error & 0x08 ) INC_STATS( rx_crc_errors );
			if ( rx_error & 0x10 ) INC_STATS( rx_length_errors );
		} else {
			/* The packet length: up to 4.5K!. */
			int pkt_len = rx_status & 0x1fff;

			DEBUG_DEVICE( INFO, "Receiving packet size %d status %4.4x.\n", device, pkt_len, rx_status );

			/* do a copy from the card's ring buffer to the passed in buffer */
			if ( ! ( read16( device->base + Wn7_MasterStatus ) & 0x8000 ) ) {
				write32( device->base + Wn7_MasterAddr, virt_to_bus( ( void * ) buf, ( ( pkt_len + 3 ) & ~3 ) ) );
				write16( device->base + Wn7_MasterLen, ( pkt_len + 3 ) & ~3 );
				write16( device->base + Command, StartDMAUp );
				while ( read16( device->base + Wn7_MasterStatus ) & 0x8000 )
					;
			} else {
				read32s( device->base + RX_FIFO, ( int32 * ) buf, ( pkt_len + 3 ) >> 2 );
			}

			write16( device->base + Command, RxDiscard );  	/* Pop top Rx packet. */
			/*netif_rx(skb);*/
			device->last_rx = system_time();
			INC_STATS( rx_packets );
			/* Wait a limited time to go to next packet. */
			tcm9xx_cmd_wait( device );
			continue;
		}
		INC_STATS( rx_dropped );
		write16( device->base + Command, RxDiscard );
		tcm9xx_cmd_wait( device );
	}

	return B_OK;
}


/* tcm9xx_boomerang_rx()
 * Description: boomerang-specific receive function */
status_t tcm9xx_boomerang_rx( device_info_t *device, const void *buf, size_t *len ) {
	uint32		entry = ( device->rx_current & RX_MASK );	// Current entry in the rx ring
	uint32		rx_status;	// Status of the current packet
	status_t	status;		// acquire_sem_etc return value
	bigtime_t	trigger_time;	// signalling delay from int handler
	bigtime_t	fn_start_time;	// start time of function trigger
	bigtime_t	fn_time;		// time to complete function

	DEBUG_DEVICE( FUNCTION, "tcm9xx_boomerang_rx ENTRY\n", device );

	while (1) {
		if ( ( status = acquire_sem_etc( device->sem_rx_lock, 1, B_CAN_INTERRUPT | B_TIMEOUT, device->block_flag & B_TIMEOUT ? 0 : B_INFINITE_TIMEOUT ) ) != B_OK ) {
			DEBUG_DEVICE( ERR, "acquire_sem_etc returned %s.  Bailing out of" __FUNCTION__ ".", device, strerror( status ) );
			*len = 0;
			return status;
		}

		/* Count the number of times the semaphore has been acquired */
		INC_STATS( rx_acquires );

		/* Profile the time it takes for the interrupt handler to signal us */
		if ( device->rx_triggered ) {
			trigger_time = system_time() - device->rx_triggered;
			if ( trigger_time > device->max_trig_time )
				device->max_trig_time = trigger_time;
			device->rx_triggered = 0;
		}

		/* Make sure we're *SUPPOSED* to be copying a buffer out - if not, we just wait again */
		if ( ( rx_status = B_LENDIAN_TO_HOST_INT32( device->rx_desc[ entry ]->status ) ) & RxDComplete ) {
			fn_start_time = system_time();
			break;
		}
	}

	DEBUG_DEVICE( INFO, "In boomerang_rx(), IntStatus %4.4x, rx_status %4.4x.\n", device,
			  read16( device->base + IntStatus ), read16( device->base + RxStatus ) );

	if ( rx_status & RxDError ) {	  /* Error, update stats */
		uint8	rx_error = rx_status >> 16;
		DEBUG_DEVICE( ERR, " Rx error: status %2.2x.\n", device, rx_error );
		INC_STATS( rx_errors );
		if ( rx_error & 0x01 ) INC_STATS( rx_over_errors );
		if ( rx_error & 0x02 ) INC_STATS( rx_length_errors );
		if ( rx_error & 0x04 ) INC_STATS( rx_frame_errors );
		if ( rx_error & 0x08 ) INC_STATS( rx_crc_errors );
		if ( rx_error & 0x10 ) INC_STATS( rx_length_errors );
	} else {
		/* The packet length: up to 4.5K!. */
		int pkt_len = rx_status & 0x1fff;

		DEBUG_DEVICE( INFO, "Receiving packet (index %d size %d status %4.4x) to buffer %x.\n", device, 
				( int ) entry, ( int ) pkt_len, ( int ) rx_status, ( int ) buf );

		/* Copy from the card's ring to the passed in buffer */
		memcpy( ( char * ) buf, device->rx_buf[ entry ], pkt_len );
		*len = pkt_len;

		device->last_rx = system_time();
		INC_STATS( rx_packets );
	}

	device->rx_desc[entry]->status = 0;				// Clear complete bit.
	device->rx_current++;							// Advance current rx pointer

	/*
	 * Jot down the time it took for this function to complete from trigger
	 * time for profiling reasons.
	 */
	fn_time = system_time() - fn_start_time;
	if ( fn_time > device->max_reader_time ) {
		device->max_reader_time = fn_time;
	}

	return B_OK;
}

/*
 * tcm9xx_vortex_tx_timeout()
 * Description: handle the transmitter timeout on Vortex cards 
 *
 */
void tcm9xx_vortex_tx_timeout( device_info_t *device ) {
	int j;

	DEBUG_DEVICE( FUNCTION, "tcm9xx_vortex_tx_timeout ENTRY (tx_status=%2.2x status=%4.4x)\n", device,
				  read8( device->base + TxStatus ), read16( device->base + IntStatus ) );

	/* Slight code bloat to be user friendly. */
	if ( ( read8( device->base + TxStatus ) & 0x88 ) == 0x88 )
		DEBUG_DEVICE( INFO, "Transmitter encountered 16 collisions -- network cable problem?\n", device );

	/* 
	 * This *really shouldn't happen* but if it does, display a message so at 
	 * least we can possibly solve it if it happens. 
	 */
	if ( read16( device->base + IntStatus ) & IntLatch ) {
		DEBUG_DEVICE( INFO, "Interrupt posted but not delivered -- IRQ blocked by another device?\n", device );
	}

	/*
	 * Dump the tx descriptors for debug purposes
	 */
	if ( device->debug & (WARN|TX) ) {
		if ( device->full_bus_master_tx ) {
			int i;
			DEBUG_DEVICE( INFO, "Flags; bus-master %d, full %d; dirty %d current %d.\n", device,
					( int ) device->full_bus_master_tx, ( int ) device->tx_full, 
					( int ) device->tx_dirty, ( int ) device->tx_current );
			DEBUG_DEVICE( INFO, "Transmit list %8.8x vs. %p.\n", device,
					( int ) read32( device->base + DownListPtr ), 
					&device->tx_desc[ device->tx_dirty % TX_BUFFERS ] );
			for ( i = 0; i < TX_BUFFERS; i++ ) {
				DEBUG_DEVICE( INFO, " %d: @%p length=%8.8x status=%8.8x\n", device, i,
						&device->tx_desc[ i ], ( int ) B_LENDIAN_TO_HOST_INT32( device->tx_desc[ i ] ->length ),
						( int ) B_LENDIAN_TO_HOST_INT32( ( int ) device->tx_desc[ i ] ->status ) );
			}
		}
	}

	/*
	 * Reset the transmitter
	 */
	write16( device->base + Command, TxReset );
	tcm9xx_cmd_wait( device );

	INC_STATS( tx_errors );

	/*
	 * Reset the bus master pointers
	 */
	if ( device->full_bus_master_tx ) {
		DEBUG_DEVICE( INFO, "Resetting the Tx ring pointer.\n", device );
		if ( device->tx_current - device->tx_dirty > 0 && read16( device->base + DownListPtr ) == 0 )
			write32( device->base + DownListPtr, ring_virt_to_bus( device, &device->tx_desc[ device->tx_dirty % TX_BUFFERS ] ) );

		if ( device->tx_full && ( device->tx_current - device->tx_dirty <= TX_BUFFERS - 1 ) ) {
			device->tx_full = 0;
			atomic_and( &device->tx_busy, ~0x01 );
		}
		write8( device->base + UpPollRate, 10 );
		if (device->card_flags & IS_BOOMERANG) {
			write8( device->base + TxFreeThreshold, BUFFER_SIZE >> 8 );
			write16( device->base + Command, DownUnstall );
		} else {
			write8( device->base + DownPollRate, 20 );
		}
	} else
		INC_STATS( tx_dropped );

	/* Issue Tx Enable */
	write16( device->base + Command, TxEnable );
	device->tx_start = system_time();
	device->restart_tx = 1;

	/* Switch to register set 7 for normal use. */
	SET_WINDOW( 7 );
}


/*
 * tcm9xx_vortex_start_xmit()
 * Description: vortex-specific transmit function 
 *
 */
status_t tcm9xx_vortex_start_xmit( device_info_t *device, const void *buf, size_t len ) {
	DEBUG_DEVICE( FUNCTION, "tcm9xx_vortex_start_xmit ENTRY\n", device );

	if ( atomic_or( &device->tx_busy, 0x01 ) ) {
		if ( system_time() - device->tx_start >= TX_TIMEOUT )
			tcm9xx_vortex_tx_timeout( device );
		return B_ERROR;
	}

	/* Write the header out to the card... */
	write32( device->base + TX_FIFO, len );

	if ( device->bus_master ) {
		/* 
		 * Copy the passed in buffer into the first transmit buffer 
		 */
		memcpy( device->tx_buf[ 0 ], buf, len );

		/* 
		 * Set the bus-master controller to transfer the packet. 
		 */
		write32( device->base + Wn7_MasterAddr, virt_to_bus( device->tx_desc[ 0 ], ( len + 3 ) & ~3 ) );
		write16( device->base + Wn7_MasterLen, ( len + 3 ) & ~3 );
		write16( device->base + Command, StartDMADown );
		/* device->tx_busy will be cleared at the DMADone interrupt. */
	} else {
		/*
		 * write the packet to the card word-by-word
		 */
		write32s( device->base + TX_FIFO, ( uint32 * ) buf, ( len + 3 ) >> 2 );
		if ( read16( device->base + TxFree ) > 1536 ) {
			atomic_and( &device->tx_busy, ~0x01 );
		} else {
			/* 
			 * Interrupt us when the FIFO has room for max-sized packet. 
			 */
			write16( device->base + Command, SetTxThreshold + ( 1536 >> 2 ) );
		}
	}

	device->tx_start = system_time(); 

	{
		/* Clear the Tx status stack. */
		int tx_status;
		int i = 32;

		while ( --i > 0 && ( tx_status = read8( device->base + TxStatus ) ) > 0 ) {
			if ( tx_status & 0x3C ) {				  /* A Tx-disabling error occurred.  */
				DEBUG_DEVICE( ERR, "Tx error, status %2.2x.\n", device, tx_status );
				if ( tx_status & 0x04 ) INC_STATS( tx_fifo_errors );
				if ( tx_status & 0x38 ) INC_STATS( tx_aborted_errors );
				if ( tx_status & 0x30 ) {
					write16( device->base + Command, TxReset );
					tcm9xx_cmd_wait( device );
				}
				write16( device->base + Command, TxEnable );
			}
			write8( device->base + TxStatus, 0x00 );  /* Pop the status stack. */
		}
	}

	DEBUG_DEVICE( FUNCTION, "tcm9xx_vortex_start_xmit EXIT\n", device );

	return B_OK;
}


/*
 * tcm9xx_boomerang_start_xmit()
 * Description: boomerang-specific transmit function 
 * 
 */
status_t tcm9xx_boomerang_start_xmit( device_info_t *device, const void *buf, size_t len ) {
	DEBUG_DEVICE( FUNCTION, "tcm9xx_boomerang_start_xmit ENTRY\n", device );

	if ( atomic_or( &device->tx_busy, 0x01 ) ) {
		if ( system_time() - device->tx_start >= TX_TIMEOUT )
			tcm9xx_vortex_tx_timeout( device );
		return B_ERROR;
	} else {
		int				entry = (device->tx_current & TX_MASK);	// Next TX descriptor entry
		boom_tx_desc	*prev_entry = device->tx_desc[ ( device->tx_current - 1 ) & TX_MASK ];	// Previous TX descriptor
		int				i;

		DEBUG_DEVICE( INFO, "Trying to send a packet, Tx index %d.\n", device, ( int )( device->tx_current & TX_MASK ) );

		if ( device->tx_full ) {
			DEBUG_DEVICE( ERR, "Tx Ring full, refusing to send buffer. (BAD!)\n", device );
			return B_ERROR;
		}

		memcpy( device->tx_buf[ entry ], buf, len );	// copy buffer into the ring buffer */
		device->tx_desc[ entry ]->next = 0;
		device->tx_desc[ entry ]->addr = 
			B_HOST_TO_LENDIAN_INT32( virt_to_bus( ( void* ) device->tx_buf[ entry ], BUFFER_SIZE ) );
		device->tx_desc[ entry ]->length = B_HOST_TO_LENDIAN_INT32( len | LAST_FRAG );

		/* Try generating an interrupt on every tx frame */
		device->tx_desc[ entry ]->status = B_HOST_TO_LENDIAN_INT32( len | TxIntrUploaded );

		/* 
		 * TODO: For 100mbit operation, we may want to use this left-out code
		 * below instead of the line above because it could improve our 
		 * wire speed performance a whole lot. 
		 */
#if 0
		/*
		 * Only generate an interrupt for the *LAST* packet in the list.  That
		 * way, we save on the number of interrupts that are 'unnecessarily'
		 * generated.
		 */
		if ( entry < (TX_BUFFERS - 1) ) {
			device->tx_desc[ entry ]->status = B_HOST_TO_LENDIAN_INT32( len );
		} else {
			DEBUG_DEVICE( INFO, "HOOKING LAST PACKET (%d) for interrupt\n", device, entry );
			device->tx_desc[ entry ]->status = B_HOST_TO_LENDIAN_INT32( len | TxIntrUploaded );
		}
#endif

		if ( device->card_flags & IS_BOOMERANG ) {
			cpu_status	cpu;
			TIME_SPINLOCK_DEFS;

			TIME_SPINLOCK_START;
			cpu = disable_interrupts();

			/*
			 * Stall the Download engine so we can mess with the ring
			 */
			write16( device->base + Command, DownStall );
			tcm9xx_cmd_wait( device );

			prev_entry->next = B_HOST_TO_LENDIAN_INT32( ring_virt_to_bus( device, (void*)device->tx_desc[ entry ] ) );

			/*
			 * If, for some reason, the download engine appears to be stalled,
			 * write the downlist pointer to the card in order to get it
			 * running again.
			 */
			if ( read32( device->base + DownListPtr ) == 0 ) {
				write32( device->base + DownListPtr, 
						B_HOST_TO_LENDIAN_INT32( ring_virt_to_bus( device, (void*)device->tx_desc[ entry ] ) ) );
				INC_STATS( queued_packet );
			}

			write16( device->base + Command, DownUnstall );

			restore_interrupts( cpu );
			TIME_SPINLOCK_STOP;

		} else {	// IS_CYCLONE
			prev_entry->next = ring_virt_to_bus( device, (void*)device->tx_desc[ entry ] );

			DEBUG_ASSERT( prev_entry->next != 0, "ring_virt_to_bus return address of 0 in " __FUNCTION__ " line: %d\n", device, (int)__LINE__ );

			/*
			 * If the transmitter was recently restarted, we should jump-start
			 * the ring buffer pointer with the current entry
			 */
			if ( device->restart_tx ) {
				write32( device->base + DownListPtr, B_HOST_TO_LENDIAN_INT32( prev_entry->next ) );
				device->restart_tx = 0;
				INC_STATS( queued_packet );
			}
		}

		/*
		 * Advance the current transmitting packet index
		 */
		device->tx_current += 1;

		/* 
		 * Check if the tx ring is full 
		 */
		if ( ( device->tx_current - device->tx_dirty ) > TX_BUFFERS - 1 ) {
			device->tx_full = 1;
		} else {
			/*
			 * We clear the previous packet's interrupt generating code so that
			 * there are no unnecessary interrupts being generated upon every
			 * single packet going out.
			 */
			prev_entry->status &= B_HOST_TO_LENDIAN_INT32(~TxIntrUploaded);
			atomic_and( &device->tx_busy, ~0x01 );
		}

		/*
		 * Record the transmit start time so we can determine whether or not
		 * the transmitter has timed out later on
		 */
		device->tx_start = system_time();

		DEBUG_DEVICE( FUNCTION, "tcm9xx_boomerang_start_xmit EXIT\n", device );
		return B_OK;
	}
}
