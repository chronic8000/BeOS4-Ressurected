static void
il_sendnext(il_info_t *il)
{
	ilc_info_t *ilc;
	kbuf_t *b;
	
	DEBUG_DEVICE(FUNCTION, "il_sendnext()\n", il);
	ilc = &il->ilc;
	
	/* dequeue and send each packet */
	while (ilc->pioactive == NULL)
	{
		/* if tx ring is full */
		if (!PIOMODE(ilc) && (NEXTTXD(ilc->txout) == (ilc->txin))) break;
		if ((b = kbq_pop_front(&il->txq)) == NULL) break;
		il_send(il, b, ILINE_MAX_PRI, -1);
		// max priority, negotiated "bpb"
		// XXX: WTF's a BPB?
	}
	
	if (!PIOMODE(ilc))
	{
		/* reclaim any completed tx ring entries */
		il_txreclaim(il, FALSE);
		
		/* enable tx interrupts whenever txq is non-empty */
		if ((ilc->intmask & I_XI)==0   && kbq_notempty(&il->txq))
			ilc_txint_on(ilc);
		else if ((ilc->intmask & I_XI) && kbq_isempty(&il->txq))
			ilc_txint_off(ilc);
	}
	
	/* XXX: linux sets il->dev->tbusy whenever txq has more than
	 * DATAHIWAT packets in it (and unsets it when not), should we
	 * do something like that? */
}


/*
 * hardware send routine
 *
 * Given a device structure, packet buffer, txpri? and txbpb?
 */
static void
il_send(il_info_t *il, kbuf_t *b, int txpri, int txbpb)
{
	ilc_info_t *ilc;
	bcm42xxregs_t *regs;
	uint datatype;
	uint32 ctrl;
	uint txout;

	ilc = &il->ilc;
	regs = ilc->regs;

	ASSERT(ilc->pioactive == NULL);
	
	DEBUG_DEVICE(INFO,
		"il_send(): len %ld txpri %d txbpb %d\n",
		il, b->len, txpri, txbpb
	);
	
	/* ensure sufficient headroom for larq and iline/tut headers */
	if (kb_headroom(b) < TXOFF)
	{
		DEBUG_DEVICE(ERR,"il_send(): no room in buffer for iline/tut headers\n",il);
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		return;
	}
	
	/* set pioactive before calling ilc_send() to avoid sendctl-recursion */
	if (PIOMODE(ilc))
		ilc->pioactive = (void*)b;

	/* common tx code */
	datatype = ilc_send(ilc, (void*)b, txpri, txbpb);
	
	if (PIOMODE(ilc))
	{
		ASSERT(ilc->pioactive == (void*)b);
		ilc_sendpio(ilc, kb_data(b), b->len, datatype);
	}
	else
	{
		ASSERT(NEXTTXD(ilc->txout) != ilc->txin);
		
		txout = ilc->txout;
		
		/* build the descriptor control value */
		ctrl = CTRL_SOF | CTRL_EOF | CTRL_IOC | (b->len & CTRL_BC_MASK);
		if (txout == (NTXD - 1)) ctrl |= CTRL_EOT;
		if (datatype == DT_TUT)  ctrl |= CTRL_COMPAT;

		/* init the tx descriptor */
		W_SM(&ilc->txd[txout].ctrl, ctrl);
		W_SM(&ilc->txd[txout].addr, (uint32)kb_data_pa(b));
		
		/* save the skb */
		ASSERT(ilc->txp[txout] == NULL);
		ilc->txp[txout] = b;
		
		/* bump the tx descriptor index */
		txout = NEXTTXD(txout);
		ilc->txout = txout;
		
		/* kick the chip */
		W_REG((&regs->xmtptr), I2B(txout));
	}
}


uint
il_sendctl(void *_il, void *_b, int txpri, int txbpb)
{
	il_info_t *il = (il_info_t*)_il;
	DEBUG_DEVICE(FUNCTION, "il_sendctl()\n", il);
	
	// queue b on the end of the txq
	kbq_push_back(&il->txq,(kbuf_t*)_b);
	
	/* don't call il_sendnext() here - it would cause larq recursion */
	return(0);
}
