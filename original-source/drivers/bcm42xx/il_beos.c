/*
 * BroadCom bcm42xx PCI 11mbit HPNA phone line driver
 * Copyright (c) 2000 Be Inc., All Rights Reserved.
 * written by Peter Folk, from the BroadCom linux driver
 */


/*
 * il_...: functions used by ilc.c
 *
 * Here lie the callbacks of BeOS.  May they rest in peace
 * disturbed not by bug-fixes, only feature additions.
 */

#include "beos/il_basics.c"
#include "beos/il_util.c"
#include "beos/il_pbuf.c"
#include "beos/il_send.c"
#include "beos/il_interrupt.c"

/* time stuff */
uint il_ms() {	return (uint)(system_time()/1000);	}
void il_delay(uint us) {	spin(us);	}

/* No-ops under BeOS */
void il_link_up(void *il) {}
void il_link_down(void *il) {}

void
il_reset(void *_il)
{
	il_info_t *il = (il_info_t*)_il;
	DEBUG_DEVICE(FUNCTION, "il_reset()\n", il);
	ilc_reset(&il->ilc);
}


int il_up(void *_il)
{
	il_info_t *il = (il_info_t*)_il;
	DEBUG_DEVICE(FUNCTION, "il_up()\n", il);
	ilc_up(&il->ilc);
	
	/* register interrupt handler */
	install_io_interrupt_handler(
		il->pciInfo->u.h0.interrupt_line,
		interrupt_device, (void*)il, 0
	);
	
	return 0;
}

int il_down(void *_il, int reset)
{
	size_t xfer_ct;
	il_info_t *il = (il_info_t*)_il;
	DEBUG_DEVICE(FUNCTION, "il_down()\n", il);

	/* unregister interrupt handler */
	remove_io_interrupt_handler(
		il->pciInfo->u.h0.interrupt_line,
		interrupt_device, (void*)il
	);
	
	ilc_down(&il->ilc, reset);
	
	/* flush the txq */
	xfer_ct = kbq_pushall_front(&il->txq, &il->tx_reserve);
	fc_signal(&il->wlock, xfer_ct, B_DO_NOT_RESCHEDULE);
	
	return 0;
}

void
il_init(void *_il)
{
	il_info_t *il = (il_info_t*)_il;
	DEBUG_DEVICE(FUNCTION, "il_init()\n", il);
	COUNTER(&il->ilc, IL_STATS_RESET_COUNT)++;
	il_reset(il);
	ilc_init(&il->ilc);
}

