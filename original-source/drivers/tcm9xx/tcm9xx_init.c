/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_init.c
 * Description:	tcm9xx-specific initialization functions
 * Changes:		Feb 05, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

/*
 * tcm9xx_initialize_locks()
 * Description:	Initialize all the fc_locks and semaphores 
 *
 */
status_t tcm9xx_initialize_locks( device_info_t *device ) {
	status_t result;

	DEBUG_DEVICE( FUNCTION, "allocate_resources ENTRY\n", device );

	/* 
	 * Set up receive fc_lock
	 */
	if ( ( device->sem_rx_lock = create_sem( 0, kDevName " rx" ) ) < 0 ) {
		DEBUG_MSG( ERR, "create sem_rx_lock failed %lx\n", result );
		return ( device->sem_rx_lock );
	}

	/* 
	 * intialize tx semaphore with the number of free tx buffers
	 */
	if ( ( result = gFC->create_fc_lock( &device->fc_tx_lock, TX_BUFFERS, kDevName " tx" ) ) < 0 ) {
		delete_sem( device->sem_rx_lock );
		DEBUG_MSG( ERR, "create fc_tx_lock failed %lx\n", result );
		return ( result );
	}

	/*
	 * Set up the media polling semaphore
	 */
	if ( ( device->sem_media_poll = create_sem( 0, kDevName " media poll" ) ) < B_NO_ERROR ) {
		delete_sem( device->sem_rx_lock );
		gFC->delete_fc_lock( &device->fc_tx_lock );
		DEBUG_MSG( ERR, "create media poll semaphore failed %lx\n", result );
		return ( result );
	}

	device->spin_window_lock = 0;

	DEBUG_DEVICE( FUNCTION, "allocate_resources EXIT\n", device );

	return B_OK;
}

/*
 * tcm9xx_uninitialize_locks()
 * Description:	Uninitialize all the fc_locks and semaphores 
 *
 */
status_t tcm9xx_uninitialize_locks( device_info_t *device ) {
	DEBUG_MSG( FUNCTION, "free_resources ENTRY\n" );
	delete_sem( device->sem_rx_lock );
	gFC->delete_fc_lock( &device->fc_tx_lock );
	delete_sem( device->sem_media_poll );
	DEBUG_MSG( FUNCTION, "free_resources EXIT\n" );
	return B_OK;
}

/*
 * tcm9xx_enable_addressing()
 * Description:	Set up access to the card's registers via PIO or mem-mapped I/O
 * Note:		There are pretty much two types of 3Com cards: those that
 * 				support BOTH mem-mapped I/O and PIO mode, and those that 
 * 				support ONLY PIO mode.  We will have 'swappable' I/O functions
 * 				in our device_info_t structure to compensate for this. 
 *
 */
status_t tcm9xx_enable_addressing( device_info_t *device ) {

	DEBUG_MSG( FUNCTION, "tcm9xx_enable_addressing ENTRY\n" );

	set_bus_mastering( device->pciInfo );

	/* 
	 * Notes on mem-mapped/PIO modes:
	 * What we have here is some cards which support ONLY PIO mode and some which
	 * support both mem-mapped and PIO modes.  In order to support both types of
	 * card I/O routines, we're going to have to have some function pointers in
	 * the device_info_t structure.  We're also going to have to test whether or
	 * not using mem-mapped I/O and function pointers is any faster than using
	 * PIO-mode on its own. [liscio] 
	 */

	/* 
	 * TODO: Change this over to support both I/O modes later on.  In the meantime
	 * we're going to work with mem-mapped mode until we find a card that refuses
	 * to work. 
	 */
	device->reg_base_area = map_pci_memory( device, 1, (void**)&device->base );

	DEBUG_MSG( FUNCTION, "tcm9xx_enable_addressing EXIT\n" );
	return B_OK;
}

/*
 * tcm9xx_read_eeprom_data()
 * Description:	Read the card's settings/capabilities out of the EEPROM 
 * Note:		The CardBus and PCI cards both have different commands for 
 * 				reading the EEPROM out of the card.  Go figure. :P 
 *
 */
status_t tcm9xx_read_eeprom_data( device_info_t *device ) {
	int i;
	unsigned int eeprom[ 0x40 ], checksum = 0;

	FUNCTION_ENTRY;

	/* 
	 * Read the EEPROM off the card. 
	 */
	DEBUG_DEVICE( INFO, "reading EEPROM using %s method\n", device, ( (device->card_flags & EEPROM_8BIT) ? "8bit" : "standard" ) );
	SET_WINDOW( 0 );
	for ( i = 0; i < 0x40; i++ ) {
		int timer;
		write16( device->base + Wn0EepromCmd, (device->card_flags & EEPROM_8BIT ? 0x230 : EEPROM_Read) + i );
		/* Pause for at least 162 us. for the read to take place. */
		for ( timer = 10; timer >= 0; timer-- ) {
			snooze( 162 );
			if ( ( read16( device->base + Wn0EepromCmd ) & 0x8000 ) == 0 ) 
				break;
		}
		eeprom[ i ] = read16( device->base + Wn0EepromData );
	}

	/* 
	 * Print the EEPROM contents (for debug purposes only) 
	 */
	if ( device->debug & INFO ) {
		dprintf( "tcm9xx: EEPROM Contents:\n" );
		for ( i = 0; i < 0x40; i++ ) {
			dprintf( "%4.4x%s", eeprom[ i ], ( ( ( ( i & 0xf ) == 0 ) && ( i != 0 ) ) ? "\n" : " " ) );
		}
		dprintf( "\n" );
	}

	/* 
	 * Calculate the checksum of the EEPROM 
	 */
	for ( i = 0; i < 0x18; i++ )
		checksum ^= eeprom[ i ];
	checksum = ( checksum ^ ( checksum >> 8 ) ) & 0xff;
	if ( checksum != 0x00 ) {
		while ( i < 0x21 )
			checksum ^= eeprom[ i++ ];
		checksum = ( checksum ^ ( checksum >> 8 ) ) & 0xff;
	}
	if ( ( checksum != 0x00 ) && !(device->card_flags & IS_TORNADO) ) {
		DEBUG_DEVICE( ERR, "***INVALID CHECKSUM %4.4x***\n", device, checksum );
	}

	/* 
	 * Read the station address from the EEPROM to the device structure. 
	 */
	for ( i = 0; i < 3; i++ ) {
		int addr_word;
		addr_word = htons( eeprom[ i + 10 ] );
		device->mac_address.ebyte[ ( 2 * i ) ] = ( uint8 ) ( addr_word & 0xff );
		device->mac_address.ebyte[ ( 2 * i ) + 1 ] = ( uint8 ) ( addr_word >> 8 );
	}
	if ( device->debug & INFO ) {
		/* Display the MAC address */
		dprintf( "tcm9xx: Mac Address: " );
		for ( i = 0; i < 6; i++ ) {
			dprintf( "%c%2.2x", i ? ':' : ' ', device->mac_address.ebyte[ i ] );
		}
		dprintf( "\n" );
	}

	/* 
	 * Write the MAC address out to the card
	 */
	SET_WINDOW( 2 );
	for ( i = 0; i < 6; i++ ) {
		write8( device->base + i, device->mac_address.ebyte[ i ] );
	}

	/* 
	 * Map the cardbus function status space (if necessary)
	 */
	if ( pci_dev_table[ gDev[ device->devID ].dev_other & TCM9XX__CHIP_IDX_MSK ].flags & HAS_CB_FNS ) {
		uint32 fn_status_addr = device->pciInfo->u.h0.base_registers[2];
		if ( fn_status_addr ) {
			/* This particular address resides at register 2 */
			device->cb_fns_area = map_pci_memory( device, 2, (void**)(&(device->cb_fns_addr)) );
		}
		SET_WINDOW( 2 );
		write16( device->base + Wn2_ResetOptions, 0x10 | read16( device->base + Wn2_ResetOptions ) );
	}

	/* 
	 * Extract (and store) some raw EEPROM data. 
	 */
	device->eep_info1 = eeprom[ 13 ];
	device->eep_info2 = eeprom[ 15 ];
	device->eep_capabilities = eeprom[ 16 ];
	if ( device->eep_info1 & 0x8000 ) {
		device->full_duplex = 1;
	}

	/* 
	 * Turn on the transceiver
	 */
	tcm9xx_activate_xcvr( device );

	/*
	 * Read the InternalConfig register and extract the pertinent info
	 */
	{
		char	*ram_split[] = {"5:3", "3:1", "1:1", "3:5"};
		union	wn3_config config;

		SET_WINDOW( 3 );
		device->available_media = read16( device->base + Wn3_Options );
		if ( ( device->available_media & 0xff ) == 0 )  	/* Broken 3c916 */
			device->available_media = 0x40;
		config.i = read32( device->base + Wn3_Config );
		DEBUG_DEVICE( ERR, "Internal config register is %8.8x, transceivers %#x.\n",
				device, config.i, read16( device->base + Wn3_Options ) );
		DEBUG_DEVICE( ERR, "%dK %s-wide RAM %s Rx:Tx split, %s%s interface.\n", device,
				8 << config.u.ram_size, config.u.ram_width ? "word" : "byte",
				ram_split[ config.u.ram_split ], config.u.autoselect ? "autoselect/" : "",
				config.u.xcvr > XCVR_ExtMII ? "<invalid transceiver>" : media_tbl[ config.u.xcvr ].name );

		device->default_media = config.u.xcvr;
		device->if_port = device->default_media;
		device->autoselect = config.u.autoselect;
	}


	/*
	 * Find the MII (or NWAY) transceiver
	 */
	if ( device->if_port == XCVR_MII || device->if_port == XCVR_NWAY ) {
		uint32	phy, phy_idx = 0;

		SET_WINDOW( 4 );
		mii_preamble_required++;
		mii_preamble_required++;	// why 2?
		mdio_read( device, 24, 1 );
		for ( phy = 1; phy <= 32 && phy_idx < sizeof( device->phys ); phy++ ) {
			int mii_status, phyx = phy & 0x1f;
			mii_status = mdio_read( device, phyx, 1 );
			if ( mii_status && mii_status != 0xffff ) {
				device->phys[ phy_idx++ ] = phyx;
				DEBUG_DEVICE( INFO, "MII transceiver found at address %d, status %4x.\n", device, phyx, mii_status );
				if ( ( mii_status & 0x0040 ) == 0 )
					mii_preamble_required++;
			}
		}
		mii_preamble_required--;
		if ( phy_idx == 0 ) {
			DEBUG_DEVICE( WARN, " ***WARNING*** No MII transceivers found!\n", device );
			device->phys[ 0 ] = 24;
		} else {
			device->advertising = mdio_read( device, device->phys[ 0 ], 4 );
			DEBUG_DEVICE( INFO, "MII Transceiver found (advertising=%lx)\n", device, device->advertising );
			if ( device->full_duplex ) {
				/* Only advertise the FD media types. */
				device->advertising &= ~0x02A0;
				mdio_write( device, device->phys[ 0 ], 4, device->advertising );
			}
		}
	}

	/*
	 * Set the device's bus mastering bits
	 */
	if ( device->eep_capabilities & CapBusMaster ) {
		device->full_bus_master_tx = 1;
		device->full_bus_master_rx = ( device->eep_info2 & 1 ) ? 1 : 2;
		DEBUG_DEVICE( INFO, "Enabling bus-master transmits and %s receives (master_rx=%d).\n", device,
				( device->eep_info2 & 1 ) ? "early" : "whole-frame", (int)device->full_bus_master_rx );
	}

	FUNCTION_EXIT;

	return B_OK;
}

/*
 * tcm9xx_select_media_port()
 * Description:	Select the active media port 
 * 
 */
status_t tcm9xx_select_media_port( device_info_t *device ) {
	union wn3_config	config;

	FUNCTION_ENTRY;

	/*
	 * Determine the initial media type
	 */
	SET_WINDOW( 3 );
	config.i = read32( device->base + Wn3_Config );
	if ( device->autoselect && ( pci_dev_table[ ( gDev[ device->devID ].dev_other & TCM9XX__CHIP_IDX_MSK ) ].flags & HAS_NWAY ) ) {
		device->if_port = XCVR_NWAY;
	} else if ( device->autoselect ) {
		/* Find first available media type, starting with 100baseTx. */
		device->if_port = XCVR_100baseTx;
		while ( !( device->available_media & media_tbl[ device->if_port ].mask ) ) {
			device->if_port = media_tbl[ device->if_port ].next;
		}
	} else {
		device->if_port = device->default_media;
	}
	DEBUG_DEVICE( INFO, "Initial media type %s (%s-duplex).\n", device, 
			media_tbl[ device->if_port ].name, device->full_duplex ? "full" : "half");

	FUNCTION_EXIT;
	return B_OK;
}

/*
 * tcm9xx_set_media_type()
 * Description: Set the current media type
 *
 */
void tcm9xx_set_media_type( device_info_t *device ) {
	union wn3_config config;

	FUNCTION_ENTRY;

	/* 
	 * Read the InternalConfig register and set up our preffered transceiver
	 * port.
	 */
	SET_WINDOW(3);
	config.i = read32( device->base + Wn3_Config );
	config.u.xcvr = device->if_port;
	if ( ! (device->card_flags & HAS_NWAY) )
		write32( config.i, device->base + Wn3_Config );

	/*
	 * Determine whether or not we have full duplex on this card
	 */
	if ( device->if_port == XCVR_MII || device->if_port == XCVR_NWAY ) {
		int mii_reg1, mii_reg5;
		SET_WINDOW(4);
		/* Read BMSR (reg1) only to clear old status. */
		mii_reg1 = mdio_read( device, device->phys[0], 1 );
		mii_reg5 = mdio_read( device, device->phys[0], 5 );
		if ( mii_reg5 == 0xffff || mii_reg5 == 0x0000 )
			; // No MII device or no link partner report
		else if ( ( mii_reg5 & 0x00C0 ) == 0x0040 ) // 10T-FD, but not 100-HD
			device->full_duplex = 1;
		DEBUG_DEVICE( INFO, "MII #%d status %4.4x, link partner capability %4.4x,"
				" setting %s-duplex.\n", device, device->phys[0], mii_reg1,
				mii_reg5, device->full_duplex ? "full" : "half" );
		SET_WINDOW(3);
	}

	/* 
	 * Manually start up the 'legacy' ports
	 */
	if ( device->if_port == XCVR_10base2 ) {
		write16( device->base + Command, StartCoax );
	}
	if ( device->if_port != XCVR_NWAY ) {
		SET_WINDOW( 4 );
		write16( device->base + Wn4_Media, ( read16( device->base + Wn4_Media ) & ~( Media_10TP | Media_SQE ) ) | media_tbl[ device->if_port ].media_bits );
	}

	/* 
	 * Set the full-duplex bit in hardware along with the large-size packet bit
	 */
	SET_WINDOW(3);
	write8( device->base + Wn3_MAC_Ctrl, ( ( device->eep_info1 & 0x8000 ) || device->full_duplex ? 0x20 : 0 ) | ( device->mtu > 1500 ? 0x40 : 0) );

	FUNCTION_EXIT;
}

/*
 * tcm9xx_reset_device()
 * Description: Reset the tcm9xx device into a known state
 *
 */
status_t tcm9xx_reset_device( device_info_t *device ) {
	int	i;

	FUNCTION_ENTRY;

	/* 
	 * Reset the transmitter and receiver, and ensure the issued reset
	 * commands complete properly.
	 */
	write16( device->base + Command, TxReset );
	for ( i = 2000; i >= 0 ; i-- ) {
		if ( !( read16( device->base + IntStatus ) & CmdInProgress ) )
			break;
	}
	write16( device->base + Command, RxReset );
	for ( i = 2000; i >= 0 ; i-- ) {
		if ( !( read16( device->base + IntStatus ) & CmdInProgress ) )
			break;
	}
	write16( device->base + Command, SetStatusEnb | 0x00 );

	/* 
	 * Reset the station address and mask 
	 */
	SET_WINDOW( 2 );
	for ( i = 0; i < 6; i++ ) {
		write8( device->base + i, device->mac_address.ebyte[ i ] );
	}
	for ( ; i < 12; i += 2 ) {
		write16( device->base + i, 0 );
	}

	/*
	 * On the Tornado variants, we can take advantage of the DownPollRate
	 * register and have the card polling while we upload new packets to
	 * the card.
	 */
	write8( device->base + UpPollRate, 10 );
	if (device->card_flags & IS_BOOMERANG) {
		write8( device->base + TxFreeThreshold, ( MAX_FRAME_SIZE >> 8 ) );
	} else if ( device->card_flags & IS_TORNADO ) {
		write8( device->base + DownPollRate, 20 );
		device->restart_tx = 1;
	}

	FUNCTION_EXIT;
	return B_NO_ERROR;
}

/*
 * tcm9xx_init_ring_buffers()
 * Description: Initialize areas which are to contain the tx and rx ring buffers 
 *
 */
status_t tcm9xx_init_ring_buffers( device_info_t *device ) {
	uint32	size;  			// Temporary size storage
#ifdef DISABLE_905B_FUNC
	uint32	dma_control;  	// Temporary dma_control register storage
#endif

	FUNCTION_ENTRY;

	/* 
	 * create transmit buffer area
	 */
	size = RNDUP( BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE );
	if ( ( device->tx_buf_area = create_area( kDevName " tx buffers", ( void ** ) device->tx_buf,
					B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA ) ) < 0 ) {
		DEBUG_DEVICE( ERR, " create tx buffer area failed %x \n", device, ( int ) device->tx_buf_area );
		return device->tx_buf_area;
	}

	/* 
	 * create tx descriptor area 
	 */
	size = RNDUP( sizeof( boom_tx_desc ) * TX_BUFFERS, B_PAGE_SIZE );
	if ( ( device->tx_desc_area = create_area( kDevName " tx descriptors", ( void ** ) device->tx_desc,
					B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA ) ) < 0 ) {
		DEBUG_DEVICE( ERR, kDevName " create tx descriptor area failed %x \n", device, ( int ) device->tx_desc_area );
		delete_area( device->tx_buf_area );
		return device->tx_desc_area;
	}

	/* 
	 * Initialize our lookup table of rx_ and tx_desc physical addresses
	 */
	DEBUG_DEVICE( INFO, "init addr table\n", device );
	ring_init_addr_table( device );

	/* 
	 * Initialize the tx buffer descriptors 
	 */
	DEBUG_DEVICE( INFO, "init tx buffer descriptors\n", device );
	{
		uint16 i;

		/* 
		 * Init the descriptors and buffers with their virtual addresses 
		 */
		for ( i = 0; i < TX_BUFFERS; i++ ) {
			device->tx_buf[ i ] = device->tx_buf[ 0 ] + ( i * BUFFER_SIZE );
			device->tx_desc[ i ] = ( boom_tx_desc * ) ( ( ( uint32 ) device->tx_desc[ 0 ] ) + ( i * sizeof( boom_tx_desc ) ) );
			ring_map_n_store_addr( device, ( uint8* ) device->tx_desc[ i ], sizeof( boom_tx_desc ) );
		}

		/* 
		 * Initialize the TX descriptors
		 */
		for ( i = 0; i < TX_BUFFERS; i++ ) {
			/* all fields are set up upon transmits.. */
			device->tx_desc[ i ]->addr = 0;
			device->tx_desc[ i ]->next = 0;
			device->tx_desc[ i ]->status = 0;
			device->tx_desc[ i ]->length = 0;
		}

		if ( device->debug & MEM ) {
			for ( i = 0; i < TX_BUFFERS; i++ ) {
				dprintf( "tx_desc[%3.3d]=%8.8x next=%8.8x addr=%8.8x length=%8.8x \n", i, ( int ) device->tx_desc[ i ],
						( int ) device->tx_desc[ i ] ->next, ( int ) device->tx_desc[ i ] ->addr, ( int ) device->tx_desc[ i ] ->length );
			}
		}
	}

	/* 
	 * create rx buffer area 
	 */
	DEBUG_DEVICE( INFO, "create rx buffer area\n", device );
	size = RNDUP( BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE );
	if ( ( device->rx_buf_area = create_area( kDevName " rx buffers", ( void ** ) device->rx_buf,
					B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA ) ) < 0 ) {
		DEBUG_DEVICE( ERR, " create rx buffer area failed %x \n", device, ( int ) device->rx_buf_area );
		delete_area( device->tx_buf_area );
		delete_area( device->tx_desc_area );
		return device->rx_buf_area;
	}

	/* 
	 * create rx descriptor area 
	 */
	DEBUG_DEVICE( INFO, "create rx desc area\n", device );
	size = RNDUP( sizeof( boom_rx_desc ) * RX_BUFFERS, B_PAGE_SIZE );
	if ( ( device->rx_desc_area = create_area( kDevName " rx descriptors", ( void ** ) device->rx_desc,
					B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA ) ) < 0 ) {
		DEBUG_DEVICE( ERR, " create rx descriptor area failed %x \n", device, ( int ) device->rx_desc_area );
		delete_area( device->tx_buf_area );
		delete_area( device->tx_desc_area );
		delete_area( device->rx_buf_area );
		return device->rx_desc_area;
	}

	/* 
	 * init rx buffer descriptors 
	 */
	DEBUG_DEVICE( ERR, "init rx desc\n", device );
	{
		uint16	i;
		/*
		 * Set the rx buffers and rx descriptors to their virtual addresses
		 */
		for ( i = 0; i < RX_BUFFERS; i++ ) {
			device->rx_buf[ i ] = device->rx_buf[ 0 ] + ( i * BUFFER_SIZE );
			device->rx_desc[ i ] = ( boom_rx_desc * ) ( ( ( uint32 ) device->rx_desc[ 0 ] ) + ( i * sizeof( boom_rx_desc ) ) );
			ring_map_n_store_addr( device, ( uint8* ) device->rx_desc[ i ], sizeof( boom_rx_desc ) );
		}

		/*
		 * Initialize the RX descriptors with their physical addresses and sizes
		 */
		for ( i = 0; i < RX_BUFFERS; i++ ) {
			device->rx_desc[ i ]->addr = virt_to_bus( ( void* ) device->rx_buf[ i ], BUFFER_SIZE );
			device->rx_desc[ i ]->next = ring_virt_to_bus( device, ( void* ) device->rx_desc[ ( i + 1 ) & RX_MASK ] );
			device->rx_desc[ i ]->status = 0;
			device->rx_desc[ i ]->length = B_HOST_TO_LENDIAN_INT32( BUFFER_SIZE | LAST_FRAG );
		}

		if ( device->debug & MEM ) {
			for ( i = 0; i < RX_BUFFERS; i++ ) {
				dprintf( "rx_desc[%2.2d]=%8.8x(v) next=%8.8x buf_addr=%8.8x length=%8.8x \n", i, ( int ) device->rx_desc[ i ],
						 ( int ) device->rx_desc[ i ] ->next, ( int ) device->rx_desc[ i ] ->addr, ( int ) device->rx_desc[ i ] ->length );
			}
		}
	}

	/*
	 * To retain compatibility with earlier non-B or non-C variations, we must
	 * disable a few B- and C-only functions.
	 */
#if DISABLE_905B_FUNC
	dma_control = read32( device->base + PktStatus );
	DEBUG_DEVICE( INFO, "Before writing, dmacontrol=%8.8x\n", device, ( int ) dma_control );
	dma_control |= ( defeatMWI | defeatMRL | upAltSeqDisable );
	DEBUG_DEVICE( INFO, "Now dmacontrol=%8.8x\n", device, ( int ) dma_control );
	write32( device->base + PktStatus, dma_control );
#endif

	FUNCTION_EXIT;
	return B_OK;
}

/*
 * tcm9xx_free_ring_buffers()
 * Description: Free the areas associated with our rx and tx ring buffers 
 *
 */
void tcm9xx_free_ring_buffers( device_info_t *device ) {
	FUNCTION_ENTRY;
	delete_area( device->tx_buf_area );
	delete_area( device->tx_desc_area );
	delete_area( device->rx_buf_area );
	delete_area( device->rx_desc_area );
	delete_area( device->cb_fns_area );
	delete_area( device->reg_base_area );
	FUNCTION_EXIT
}

/*
 * tcm9xx_set_rx_mode()
 * Description: Set receiver mode
 *
 */
void tcm9xx_set_rx_mode( device_info_t *device ) {
	int32	new_mode;

	FUNCTION_ENTRY;

	if ( device->rx_flags & TCM9XX__PROMISCUOUS_MODE ) {
		DEBUG_DEVICE( INFO, "Setting promiscuous mode.\n", device );
		new_mode = SetRxFilter | RxStation | RxMulticast | RxBroadcast | RxProm;
	} else if ( ( device->multi_enabled ) || ( device->rx_flags & TCM9XX__ALL_MULTI_MODE ) ) {
		new_mode = SetRxFilter | RxStation | RxMulticast | RxBroadcast;
	} else
		new_mode = SetRxFilter | RxStation | RxBroadcast;
	write16( device->base + Command, new_mode );

	FUNCTION_EXIT;
}

/*
 * tcm9xx_start_operation()
 * Description: Start the card up...
 *
 */
void tcm9xx_start_operation( device_info_t *device ) {
	FUNCTION_ENTRY;

	if ( device->full_bus_master_rx ) { // Post-Vortex bus master
		/* Initialize the RxEarly register as recommended. */
		write16( device->base + Command, SetRxThreshold | (1536>>2) );
		write32( device->base + PktStatus, 0x0020 );
		write32( device->base + UpListPtr, 
				ring_virt_to_bus( device, (void*)device->rx_desc[ device->tx_current ] ) );
	}

	write16( device->base + Command, StatsEnable );
	write16( device->base + Command, RxEnable );
	write16( device->base + Command, TxEnable );

	write16( device->base + Command, device->status_enable );
	write16( device->base + Command, AckIntr | IntLatch | TxAvailable | RxEarly | IntReq );
	write16( device->base + Command, device->intr_enable );
	if ( device->cb_fns_addr )
		write32( device->cb_fns_addr + 4, 0x8000 );

	device->tx_busy = 0;

	FUNCTION_EXIT;
}

/*
 * tcm9xx_cmd_wait()
 * Description: Wait for the last issued command to complete
 *
 */
void tcm9xx_cmd_wait( device_info_t *device ) {
	register int	i;

	for ( i = 0; i < TCM9XX__MAX_CMD_WAIT; i++ ) {
		if ( ! ( read16( device->base + IntStatus ) & CmdInProgress ) )
			break;
	}

	if ( i == TCM9XX__MAX_CMD_WAIT )
		INC_STATS( command_timeouts );
}

