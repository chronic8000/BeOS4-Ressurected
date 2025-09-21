#include "common/kbuf.h"

/*
 * il_pget
 *
 * allocates a buffer with len bytes of data and (at least) TXOFF
 * bytes of headroom.  Returns the virtual and physical addrs of
 * this data by reference in va and pa.
 */
void*
il_pget(void *_il, bool send, uint len, uchar **va, uchar **pa)
{
	kbuf_t *b;
	il_info_t *il = (il_info_t*)_il;
	kbq_t *q = send? &il->tx_reserve:&il->rx_reserve;
	
	if (send)
	{
		il_release(il);
		errno = acquire_sem_etc(il->wlock, 1, B_RELATIVE_TIMEOUT, il->noblock? 0:B_INFINITE_TIMEOUT);
		il_acquire(il);
		if (errno) return NULL;
	}
	
	b = kbq_pop_front(q);
	
	if (b==NULL)
	{
		DEBUG_DEVICE(ERR, "il_pget(): no %s buffers available (or interrupted)\n",
			il, send? "tx":"rx");
		return NULL;
	}
	DEBUG_DEVICE(INFO, "il_pget(): got %cxb %p\n", il, (send? 't':'r'), b);
	
	kb_clear(b);
	if (kb_reserve_front(b,TXOFF)!=B_OK || kb_alloc_back(b,len)==NULL)
	{
		DEBUG_DEVICE(ERR, "il_pget(): buffer too small (%ld vs %d)\n",
			il, b->space, len+TXOFF);
		
		kbq_push_back(q,b);
		if (send) release_sem_etc(il->wlock, 1, B_DO_NOT_RESCHEDULE);
		DEBUG_DEVICE(INFO, "freed %cxb %p\n", il, (send?'t':'r'), b);
		return NULL;
	}
	
	if (va) *va = kb_data(b);
	if (pa) *pa = kb_data_pa(b);
	
	return (void*)b;
}

// recycle the buffer
void
il_pfree(void *_il, void *_b, bool send)
{
	il_info_t *il = (il_info_t*)_il;
	kbuf_t *b = (kbuf_t*)_b;
	kbq_t *q = send? &il->tx_reserve:&il->rx_reserve;
	
	kbq_push_front(q, b);
	if (send) release_sem_etc(il->wlock, 1, B_DO_NOT_RESCHEDULE);
	DEBUG_DEVICE(INFO, "freed %cxb %p\n", il, (send?'t':'r'), b);
}

/*
 * push, pull, etc
 *
 * see kbuf.h for details
 */
uchar*
il_ppush(void *_il, void *b, uint bytes)
{
	return (kb_alloc_front((kbuf_t*)b, bytes));
}

uchar*
il_ppull(void *_il, void *b, uint bytes)
{
	return (kb_free_front((kbuf_t*)b, bytes));
}


uint
il_plen(void *b)
{
	return ((kbuf_t*)b)->len;
}

void
il_psetlen(void *_b, uint len)
{
	kbuf_t *b = (kbuf_t*)_b;
	
	ASSERT(len <= b->space);
	if (len<b->len) kb_free_back(b,b->len-len);
	else if (len!=b->len) kb_alloc_back(b,len-b->len);
}

// duplicate a buffer
void*
il_pdup(void *_il, void *_b)
{
	il_info_t *il = (il_info_t*)_il;
	kbuf_t *b = (kbuf_t*)_b, *n;
	
	n = il_pget(il, TRUE, b->len, NULL, NULL);
	if (n == NULL) return NULL;
	
	memcpy(n->data, b->data, b->len);
	return n;
}

void
il_txreclaim(void *_il, bool forceall)
{
	il_info_t *il = (il_info_t*)_il;
	ilc_info_t *ilc;
	uint start, end;
	uint i, freect=0;
	
	ilc = &il->ilc;
	
	// XXX int-safe?
	DEBUG_DEVICE(FUNCTION, "il_txreclaim()\n", il);
	
	/* if pio and transmit is pending AND (forceall or txdone) */
	if (PIOMODE(ilc))
	{
		if (ilc->pioactive
		&& (forceall || (R_REG(&ilc->regs->xmtfifocontrol) & XFC_FR)))
		{
			il_pfree(il, (kbuf_t*)ilc->pioactive, 1);
			ilc->pioactive = NULL;
		}
		return;
	}
	
	/* unload and free all/completed skb in the tx descriptor ring */
	if (forceall)
	{
		for (i = 0; i < NTXD; i++)
		{
			if (ilc->txp[i])
			{
				W_SM(&ilc->txd[i].addr, 0);
				kbq_push_front(&il->tx_reserve,(kbuf_t*)ilc->txp[i]);
				DEBUG_DEVICE(INFO, "reclaimed txb %p\n", il, (kbuf_t*)ilc->txp[i]);
				// we don't use il_pfree here so we don't signal
				// the wlock over and over again
				ilc->txp[i] = NULL;
				
				++freect;
			}
		}
	}
	else
	{
		start = ilc->txin;
		end = B2I(R_REG(&ilc->regs->xmtstatus) & XS_CD_MASK);

		ASSERT((start < NTXD) && (end < NTXD));

		for (i = start; i != end; i = NEXTTXD(i))
		{
			if (ilc->txp[i])
			{
				W_SM(&ilc->txd[i].addr, 0);
				kbq_push_front(&il->tx_reserve,(kbuf_t*)ilc->txp[i]);
				DEBUG_DEVICE(INFO, "reclaimed txb %p\n", il, (kbuf_t*)ilc->txp[i]);
				// we don't use il_pfree here so we don't signal
				// the wlock over and over again
				ilc->txp[i] = NULL;
			
				++freect;
			}
		}
		ilc->txin = end;
	}
	
	if (freect)
	{
		DEBUG_DEVICE(INFO, "interrupt(): freeing %ld tx buffers\n", il, freect);
		release_sem_etc(il->wlock, freect, B_DO_NOT_RESCHEDULE);
	}
}

void
il_rxreclaim(void *_il)
{
	il_info_t *il = (il_info_t*)_il;
	ilc_info_t *ilc;
	uint i;
	
	ilc = &il->ilc;
	DEBUG_DEVICE(FUNCTION, "il_rxreclaim()\n", il);
	
	if (PIOMODE(ilc)) return;
	
	/* unload and free rx kbufs from the rx descriptor ring */
	for (i = 0; i != NRXD; i++)
	{
		if (ilc->rxp[i])
		{
			W_SM(&ilc->rxd[i].addr, 0);
			il_pfree(il, (kbuf_t*)ilc->rxp[i], 0);
			ilc->rxp[i] = NULL;
		}
	}
}
