/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_ints.c
 * Description:	tcm9xx interrupt handling
 * Changes:		March 12, 2000	- Initial Revision [chrisl]
 *****************************************************************************/


/*
 * tcm9xx_interrupt_handler()
 * Description: Interrupt handler routine 
 * 
 */
static status_t tcm9xx_interrupt_handler( void *_device ) {
	device_info_t	* device = ( device_info_t * ) _device;
	int32 status = 0, latency = 0;
	int32 handled	= B_UNHANDLED_INTERRUPT;
	int32 work_done = 20;
	int32	invoke_scheduler = 0;

	DEBUG_DEVICE( FUNCTION, "tcm9xx_interrupt_handler ENTRY\n", device );

	if ( atomic_or( &device->interrupt, 0x01 ) ) {
		/* this should *never* happen */
		DEBUG_DEVICE( ERR, "reentry into interrupt hook\n", device );
		return B_UNHANDLED_INTERRUPT;
	}

	latency = read8( device->base + Timer );
	status = read16( device->base + IntStatus );
	DEBUG_DEVICE( INTERRUPT, "ISR=%4.4x, latency=%d ticks.\n", device, ( int ) status, ( int ) latency );

	if ( status == 0 ) {		/* shared interrupts go to next devices ISR */
		handled = B_UNHANDLED_INTERRUPT;
	} else {
		handled = B_HANDLED_INTERRUPT;

		do {
			DEBUG_DEVICE( INTERRUPT, "In interrupt loop, status %4.4x.\n", device, ( int ) status );

			if ( status & RxComplete ) {	/* Only gets called on non bus-master devices */
				DEBUG_DEVICE( INTERRUPT, "RxComplete Interrupt\n", device );
				release_sem_etc( device->sem_rx_lock, 1, B_DO_NOT_RESCHEDULE );
				invoke_scheduler = 1;
			}
			if ( status & UpComplete ) {	/* Called on bus-master devices */
				uint32		entry = ( device->rx_current & RX_MASK );
				bigtime_t	now;

				DEBUG_DEVICE( INTERRUPT, "UpComplete Interrupt\n", device );
				write16( device->base + Command, AckIntr | UpComplete );
				now = system_time();
				device->rx_triggered = now;
				device->rx_int_time = now;
				/*
				 * Beginning from the packet AFTER the one we just read in the
				 * read_hook, signal for every packet which can now be read.
				 */
				while ( B_LENDIAN_TO_HOST_INT32( device->rx_desc[ entry ]->status ) & RxDComplete ) {
					release_sem_etc( device->sem_rx_lock, 1, B_DO_NOT_RESCHEDULE );
					INC_STATS( rx_triggers );
					entry = ( entry + 1 ) & RX_MASK;
				}
				invoke_scheduler = 1;
			}
#if 0
			if ( status & TxAvailable ) {
				DEBUG_DEVICE( INTERRUPT, "TxAvailable Interrupt\n", device );
				DEBUG_DEVICE( INFO, "TX room bit was handled.\n", device );
				/* There's room in the FIFO for a full-sized packet. */
				write16( device->base + Command, AckIntr | TxAvailable );
				atomic_and( &device->tx_busy, ~0x01 );
				invoke_scheduler = 1;
			}
#endif
			/* With CardBus cards, this signal goes high despite the fact that
			 * the 3Com 90x manual shows this bit as always-0.  All we can hope
			 * to do now is acknowledge this bit and move on.  */
			if ( status & TxAvailable ) {
				write16( device->base + Command, AckIntr | TxAvailable );
			}
			if ( status & DownComplete ) {
				uint32	tx_dirty = device->tx_dirty;
				uint32	dirty_count = 0;
				int		entry;

				DEBUG_DEVICE( INTERRUPT, "DownComplete Interrupt\n", device );

				write16( device->base + Command, AckIntr | DownComplete );

				while ( ( device->tx_current - tx_dirty ) > 0 ) {
 					entry = (tx_dirty & TX_MASK);

					//if ( read32( device->base + DownListPtr ) == ring_virt_to_bus( device, device->tx_desc[ entry ] ) )
						//break;  			/* It still hasn't been processed. */

					/* Zero-out the buffer completely */
					device->tx_desc[ entry ]->addr = 0;
					device->tx_desc[ entry ]->next = 0;
					device->tx_desc[ entry ]->status = 0;
					device->tx_desc[ entry ]->length = 0;

					dirty_count++;
					tx_dirty++;
				}

				/* Signal for all 'completed' buffers */
				gFC->fc_signal( &device->fc_tx_lock, dirty_count, B_DO_NOT_RESCHEDULE );
				invoke_scheduler = 1;

				device->tx_dirty = tx_dirty;
				if ( device->tx_full && ( device->tx_current - tx_dirty <= TX_BUFFERS - 1 ) ) {
					device->tx_full = 0;
					atomic_and( &device->tx_busy, ~0x01 );
				}
			}
#if 0
			if ( status & LinkEvent ) {
				/* Just ack these and return. */
				handled = B_HANDLED_INTERRUPT;
				DEBUG_DEVICE( INTERRUPT, "handling LinkEvent\n", device );
				write16( device->base + Command, AckIntr | LinkEvent );
			}
#endif

			if ( status & DMADone ) {
				DEBUG_DEVICE( INTERRUPT, "DMADone Interrupt\n", device );
				invoke_scheduler = 1;
				if ( read16( device->base + Wn7_MasterStatus ) & 0x1000 ) {
					write16( device->base + Wn7_MasterStatus, 0x1000 );   /* Ack the event. */
					// TODO: free the transmitted buffer here?
					if ( read16( device->base + TxFree ) > 1536 ) {
						atomic_and( &device->tx_busy, ~0x01 );
					} else { /* Interrupt when FIFO has room for max-sized packet. */
						write16( device->base + Command, SetTxThreshold + ( 1536 >> 2 ) );
					}
				}
			}
			/* Check for all uncommon interrupts at once. */
			if ( status & ( HostError | RxEarly | StatsFull | TxComplete | IntReq ) ) {
				DEBUG_DEVICE( INTERRUPT, "ERROR (Host, RxEarly, StatsFull, TxComplete, IntReq) Interrupt\n", device );
				handled = B_HANDLED_INTERRUPT;
				if ( status == 0xffff )
					break;
				tcm9xx_vortex_error( device, status );
			}

			if ( --work_done < 0 ) {
				if ( ( status & ( 0x7fe - ( UpComplete | DownComplete ) ) ) == 0 ) {
					/* Just ack these and return. */
					handled = B_HANDLED_INTERRUPT;
					write16( device->base + Command, AckIntr | UpComplete | DownComplete );
				} else {
					DEBUG_DEVICE( WARN, "Too much work in interrupt, status "
								  "%4.4x.  Temporarily disabling functions (%4.4x).\n", device,
								  ( int ) status, ( int ) ( SetStatusEnb | ( ( ~status ) & 0x7FE ) ) );
					/* Disable all pending interrupts. */
					write16( device->base + Command, SetStatusEnb | ( ( ~status ) & 0x7FE ) );
					write16( device->base + Command, AckIntr | 0x7FF );
					/* The media polling thread should reenable interrupts. */
					break;
				}
			}
			/* Acknowledge the IRQ. */
			write16( device->base + Command, AckIntr | IntReq | IntLatch );

			if ( device->cb_fns_addr )
				write32( device->cb_fns_addr + 4, 0x8000 );

		} while ( ( status = read16( device->base + IntStatus ) ) & ( IntLatch | RxComplete ) );

	}

	DEBUG_DEVICE( FUNCTION, "tcm9xx_interrupt_handler EXIT (status=%4.4x).\n", device, ( int ) status );

	atomic_and( &device->interrupt, ~0x01 );

	if ( invoke_scheduler ) {
		return B_INVOKE_SCHEDULER;
	} else {
		return ( handled );
	}
}

/*
 * tcm9xx_vortex_error()
 * Description: Handle uncommon interrupt sources.  This is a separate routine 
 * to minimize the cache impact. 
 * 
 */
void tcm9xx_vortex_error( device_info_t *device, int status ) {
	int do_tx_reset = 0;
	int i;

	if ( status & TxComplete )   			/* Really "TxError" for us. */
	{
		unsigned char tx_status = read8( device->base + TxStatus );
		/* Presumably a tx-timeout. We must merely re-enable. */
		if ( tx_status != 0x88 )
			DEBUG_DEVICE( INTERRUPT, "Transmit error, Tx status register=%2.2x.\n", device, tx_status );
		if ( tx_status & 0x14 ) device->stat_tx_fifo_errors++;
		if ( tx_status & 0x38 ) device->stat_tx_aborted_errors++;
		write8( device->base + TxStatus, 0 );
		if ( tx_status & 0x30 )
			do_tx_reset = 1;
		else					/* Merely re-enable the transmitter. */
			write16( device->base + Command, TxEnable );
	}
	if ( status & RxEarly )   				/* Rx early is unused. */
	{
		release_sem_etc( device->sem_rx_lock, 1, B_DO_NOT_RESCHEDULE );
		write16( device->base + Command, AckIntr | RxEarly );
	}
	if ( status & StatsFull )   			/* Empty statistics. */
	{
		static int DoneDidThat = 0;
		DEBUG_DEVICE( INFO, "Updating statistics\n", device );
		tcm9xx_update_stats(device);
		/* HACK: Disable statistics as an interrupt source. */
		/* This occurs when we have the wrong media type! */
		if ( DoneDidThat == 0 && read16( device->base + IntStatus ) & StatsFull ) {
			DEBUG_DEVICE( ERR, "Updating stats failed, disabling stats as an int source.\n", device );
			SET_WINDOW( 5 );
			write16( device->base + Command, SetIntrEnb | ( read16( device->base + 10 ) & ~StatsFull ) );
			SET_WINDOW( 7 );
			DoneDidThat++;
		}
	}
	if ( status & IntReq )   		/* Restore all interrupt sources.  */
	{
		write16( device->base + Command, device->status_enable );
		write16( device->base + Command, device->intr_enable );
	}

	if ( status & HostError ) {
		uint16 fifo_diag;
		SET_WINDOW( 4 );
		fifo_diag = read16( device->base + Wn4_FIFODiag );
		DEBUG_DEVICE( ERR, "Host error, FIFO diagnostic register %4.4x.\n", device, fifo_diag );
		/* Adapter failure requires Tx/Rx reset and reinit. */
		if ( device->full_bus_master_tx ) {
			write16( device->base + Command, TotalReset | 0xff );
			for ( i = 2000; i >= 0 ; i-- )
				if ( !( read16( device->base + IntStatus ) & CmdInProgress ) )
					break;
			/* Re-enable the receiver. */
			write16( device->base + Command, RxEnable );
			write16( device->base + Command, TxEnable );
		} else if ( fifo_diag & 0x0400 )
			do_tx_reset = 1;
		if ( fifo_diag & 0x3000 ) {
			write16( device->base + Command, RxReset );
			for ( i = 2000; i >= 0 ; i-- )
				if ( !( read16( device->base + IntStatus ) & CmdInProgress ) )
					break;
			/* Set the Rx filter to the current state. */
			write16( device->base + Command, SetRxFilter | RxStation | RxBroadcast );
			write16( device->base + Command, RxEnable );   /* Re-enable the receiver. */
			write16( device->base + Command, AckIntr | HostError );
		}
	}
	if ( do_tx_reset ) {
		int j;
		write16( device->base + Command, TxReset );
		for ( j = 200; j >= 0 ; j-- )
			if ( !( read16( device->base + IntStatus ) & CmdInProgress ) )
				break;
		write16( device->base + Command, TxEnable );
	}
}

/*
 * tcm9xx_update_stats()
 * Description: Update Statistics
 * Note: Unlike with the EtherLinkIII, we need not worry about interrupts
 * changing the window settings from underneath us, but we still guard
 * against a race condition with a StatsUpdate interrupt updating the
 * table.  This is done by checking that the ASM (!) code generated
 * uses atomic updates with '+='.
 * 
 */
void tcm9xx_update_stats( device_info_t *device ) {
	int old_window = read16(device->base + Command);

	if (old_window == 0xffff)	/* Chip suspended or ejected. */
		return;

	/* Unlike the 3c5x9 we need not turn off stats updates while reading. */
	/* Switch to the stats window, and read everything. */
	SET_WINDOW(6);
	device->stat_tx_carrier_errors		+= read8(device->base + 0);
	device->stat_tx_heartbeat_errors	+= read8(device->base + 1);
	/* Multiple collisions. */			read8(device->base + 2);
	device->stat_collisions				+= read8(device->base + 3);
	device->stat_tx_window_errors		+= read8(device->base + 4);
	device->stat_rx_fifo_errors			+= read8(device->base + 5);
	device->stat_tx_packets				+= read8(device->base + 6);
	device->stat_tx_packets				+= (read8(device->base + 9)&0x30) << 4;
	/* Rx packets	*/					read8(device->base + 7);   /* Must read to clear */
	/* Tx deferrals */					read8(device->base + 8);
	/* Don't bother with register 9, an extension of registers 6&7.
	   If we do use the 6&7 values the atomic update assumption above
	   is invalid. */
	device->stat_rx_bytes += read16(device->base + 10);
	device->stat_tx_bytes += read16(device->base + 12);

	/* New: On the Vortex we must also clear the BadSSD counter. */
	SET_WINDOW(4);
	read8(device->base + 12);

	/* We change back to window 7 (not 1) with the Vortex. */
	SET_WINDOW(old_window >> 13);
	return;
}
