static void il_intr(il_info_t *il)
{
	uint32 intstatus = il->intstatus;
	ilc_info_t *ilc = &il->ilc;
	
	if (intstatus == 0) return;
	/* detect cardbus removed */
	else if (intstatus == 0xffffffff)
	{
		DEBUG_DEVICE(ERR, "il_intr(): dead chip (card removed?)\n", il);
		il_down(il, 1);
	}
	else
	{
		/* common interrupt code */
		ilc_intr(ilc, intstatus);
		
		/* run the tx queue */
		if (kbq_notempty(&il->txq)) il_sendnext(il);
	}
	atomic_and(&il->intstatus, ~intstatus);
}

void
il_txintr(void *_device)
{
	il_info_t *il;
	
	il = (il_info_t*)_device;
	DEBUG_DEVICE(FUNCTION, "intr(): txintr\n", il);
	il_txreclaim(il, FALSE);
}

void
il_rxintr(void *_device)
{
	il_info_t *il = (il_info_t*)_device;
	
	if (PIOMODE(&il->ilc)) il_recvpio(il);
	else il_recvdma(il);
}

static void
il_recvpio(il_info_t *il)
{
	ilc_info_t *ilc;
	kbuf_t *b;
	bcm42xxrxhdr_t *rxh;

	DEBUG_DEVICE(FUNCTION, "il_recvpio\n", il);
	ilc = &il->ilc;
	
	/* process all received packets */
	while (R_REG(&ilc->regs->rcvfifocontrol) & RFC_FR)
	{
		/* alloc new skb and copy rx frame into it */
		if ((b = (kbuf_t*) ilc_recvpio(ilc)) == NULL)
			continue;
		
		/* skb starts with rxhdr */
		rxh = (bcm42xxrxhdr_t*) b->data;
		
		/* strip off rxhdr */
		il_ppull(il, b, HWRXOFF);
		
		/* call common receive and dispatch */
		ilc_recv(ilc, rxh, (void*)b);
	}
}

static void
il_recvdma(il_info_t *il)
{
	ilc_info_t *ilc;
	kbuf_t *b;
	bcm42xxrxhdr_t *rxh;
	uint rxin, curr;
	uint skiplen;
	uint len;
	
	DEBUG_DEVICE(FUNCTION, "il_recvdma\n", il);
	ilc = &il->ilc;
	skiplen = 0;
	
	/* process all received packets */
	curr = B2I(R_REG(&ilc->regs->rcvstatus) & RS_CD_MASK);
	for (rxin = ilc->rxin; rxin!=curr; rxin=NEXTRXD(rxin))
	{
		/* get the packet that corresponds to the rx descriptor */
		b = (kbuf_t*) ilc->rxp[rxin];
		ilc->rxp[rxin] = NULL;
		W_SM(&ilc->rxd[rxin].addr, 0);
		
		// paranoia
		ASSERT(b);
		
		/* skip giant packets which span multiple rx descriptors */
		if (skiplen > 0)
		{
			skiplen -= RXBUFSZ;
			if (skiplen < 0)
				skiplen = 0;
			il_pfree(il,b,false);
			continue;
		}
		
		FLUSH(b->data, RXBUFSZ, 1);
		rxh = (bcm42xxrxhdr_t*) b->data;
		len = ltoh16(FRAMELEN(rxh));
		
		/* giant frame check */
		if (len > (RXBUFSZ - HWRXOFF))
		{
			DEBUG_DEVICE(INFO, "read(): skipping giant packet (%db)\n", il, len);
			COUNTER(ilc, IL_STATS_RCV_GIANT_COUNT)++;
			COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
			skiplen = len - (RXBUFSZ - HWRXOFF);
			il_pfree(il,b,false);
			continue;
		}
		
		/* strip off rxhdr */
		il_ppull(il, b, HWRXOFF);
		
		/* set actual length */
		il_psetlen(b, len);
		
		/* call common receive and dispatch */
		ilc_recv(ilc, rxh, (void*)b);
	}
	
	ilc->rxin = rxin;
	
	/* post more rx buffers */
	ilc_fill(&il->ilc);
}


void
il_sendup(void *_il, void *_b, uint pri, uint pcrc)
{
	il_info_t *il = (il_info_t*)_il;
	kbuf_t *b = (kbuf_t*)_b;
	DEBUG_DEVICE(INFO, "il_sendup(): %ld bytes\n", il, b->len);
	
	if (pcrc) {
		il_pfree(il,b,FALSE);
		return;
	}
	
	// append b to the tail of the rxq
	kbq_push_back(&il->rxq, b);
	
	// wake up any readers
	fc_signal(&il->rlock, 1, B_DO_NOT_RESCHEDULE);
}

