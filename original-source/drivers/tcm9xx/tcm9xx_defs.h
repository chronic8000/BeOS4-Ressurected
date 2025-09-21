/******************************************************************************
 * 3Com 3c9xx Series Driver
 * Copyright (c)2000 Be Inc., All Rights Reserved.
 * Written by Chris Liscio
 * NOTE: Some portions may be taken from Donald Becker's purchased codebase.
 *
 * File:		tcm9xx_defs.h
 * Description:	General driver operation defines
 * Changes:		Feb 14, 2000	- Initial Revision [chrisl]
 *****************************************************************************/

#ifndef _TCM9XX_DEFS_H_
#define _TCM9XX_DEFS_H_

/******************************************************************************
 * Compile defines for different testing 'features'
 *****************************************************************************/
#define TIME_SPINLOCKS			true
#define DISABLE_905B_FUNC		true
#define STATS_COUNTING			true				// For extra stat disabling

/******************************************************************************
 * Driver operation performance settings
 *****************************************************************************/
/* Ring buffer sizes - tune for performance */
#define TX_BUFFERS				16L					// must be a power of 2
#define RX_BUFFERS				32L					// must be a power of 2
#define TX_MASK					(TX_BUFFERS - 1)
#define RX_MASK					(RX_BUFFERS - 1)

#define TCM9XX__MAX_CMD_WAIT	2000				// Maximum wait time for
													// command completion

/* Time to wait before concluding transmitter is hung */
#define TX_TIMEOUT				40000				// 40ms should suffice

/* Maximum number of multicast addresses */
#define MAX_MULTI				32

/******************************************************************************
 * Driver-specific globals
 *****************************************************************************/
/* Set iff a MII transceiver on any interface requires mdio preamble.
 * This only set with the original DP83840 on older 3c905 boards, so the extra
 * code size of a per-interface flag is not worthwhile. */
char mii_preamble_required = 0;

/******************************************************************************
 * Masks for gDev[].dev_other use
 *****************************************************************************/
#define TCM9XX__CHIP_IDX_MSK	0xFF

/******************************************************************************
 * Receiver modes
 *****************************************************************************/
#define TCM9XX__PROMISCUOUS_MODE	0x01
#define TCM9XX__ALL_MULTI_MODE		0x02

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
 * Spinlock Utilit(y/ies)
 *****************************************************************************/
#ifdef TIME_SPINLOCKS
#define TIME_SPINLOCK_DEFS		bigtime_t timer_start, timer_stop
#define TIME_SPINLOCK_START		timer_start = system_time()
#define TIME_SPINLOCK_STOP		timer_stop = system_time(); \
								DEBUG_DEVICE(TIMER, __FUNCTION__ \
										" - Interrupts held off for %lluus\n", \
										device, (timer_stop - timer_start))
#else
#define TIME_SPINLOCK_DEFS
#define TIME_SPINLOCK_START
#define TIME_SPINLOCK_STOP
#endif // TIME_SPINLOCKS

/******************************************************************************
 * Special Statistics Macros
 *****************************************************************************/
#ifdef STATS_COUNTING
#define INC_STATS( stat )		((device->stat_##stat) += 1)
#define PRINT_STAT( thestat )	kprintf( #thestat " = %d, ", (int)device->stat_##thestat )
#define PRINT_NEW_LINE			kprintf( "\n" )
#else
#define INC_STATS( stat )
#define PRINT_STAT( thestat )
#define PRINT_NEW_LINE
#endif // STATS_COUNTING

/******************************************************************************
 * Debugging toys
 *****************************************************************************/
#ifdef DEBUG
#define FUNCTION_ENTRY				DEBUG_DEVICE( FUNCTION, "##__FUNCTION__ ENTRY\n", device );
#define FUNCTION_EXIT				DEBUG_DEVICE( FUNCTION, "##__FUNCTION__ EXIT\n", device );
#else
#define FUNCTION_ENTRY
#define FUNCTION_EXIT
#endif // DEBUG

/******************************************************************************
 * Miscellaneous Utilities
 *****************************************************************************/
#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

#endif // _TCM9XX_DEFS_H_
