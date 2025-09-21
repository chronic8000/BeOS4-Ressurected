/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.

*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
    

*/
/*
 * Common [OS-independent] portion of
 * Broadcom BCM42XX InsideLine Home Phoneline Networking Adapter
 * Device Driver.
 *
 * Copyright(c) 2000 Broadcom Corp.
 * All Rights Reserved.
 *
 * $Id: ilc.c,v 13.262.4.1 2000/05/11 23:43:13 csw Exp $
 */

#include <bcmendian.h>
#include <proto/ethernet.h>
#include <proto/iline.h>
#include <proto/ilcp.h>
#include <bcm42xx.h>

#ifdef IL_STATS
#define	STATS_NAMES	1
#include <il_stats.h>
#endif

#include <ilc.h>
#include <il_export.h>
#if defined(IL_FIXES)|| !defined(NOSPROM)
#include <rts/crc.h>
#endif

#ifdef IL_BRIDGE
extern int bridge_peek(void *h, uint8 *ea);
#endif

ilc_info_t *ilc_list = NULL;

uint ilc_stats_size = 0;

/* debug/trace */
#ifdef DBG
int il_msg_level = 1;
#else
int il_msg_level = 0;
#endif

uint il_null_txpri[] = {
	2, 0, 1, 3, 4, 5, 7, 6
};

static uint il_null_rxpri[] = {
	1, 2, 0, 3, 4, 5, 7, 6
};


/* led0 on/off festatus register writeonly values */
#define	FES_LEDON	(FES_SW | (3 << FES_L0_SHIFT))
#define	FES_LEDOFF	(3 << FES_L0_SHIFT)

#ifdef _WIN32
extern uint __cdecl strlen(const char *string);
#else
extern uint strlen(const char *string);
#endif

/* local prototypes */
static void ilc_chipinitfec(ilc_info_t *ilc);
static struct scb* ilc_scbreclaim(ilc_info_t *ilc);
void validate_chip_access(ilc_info_t *ilc);
#ifdef IL_BRIDGE
void ilc_refidpurge(ilc_info_t *ilc, struct scb *scb);
void ilc_refidset(ilc_info_t *ilc, struct scb *scb, uchar *ea);
#endif
static uint ilc_check_hdr(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, uint datatype,
	uint pcrc, uint len);
static void ilc_wtutreg(ilc_info_t *ilc, uint16 addr, uint16 v);
static uint16 ilc_rtutreg(ilc_info_t *ilc, uint16 addr);
static void ilc_loopback(ilc_info_t *ilc, int on);
static void ilc_sendlink(ilc_info_t *ilc);
static void ilc_dumpscb(ilc_info_t *ilc, uchar *buf, int size);
static void ilc_linkupd(ilc_info_t *ilc, struct ether_header *eh);
static uint ilc_txbpb(ilc_info_t *ilc, int txbpb);
static uint ilc_txpri(ilc_info_t *ilc, int txpri);
static void ilc_rateupd(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, struct ether_header *eh,
	uint tx_desc, struct scb *scb, uint pcrc);
static void ilc_modepurge(ilc_info_t *ilc, uint newmacmode);
static uint ilc_macmode(ilc_info_t *ilc);
static void ilc_ratewd(ilc_info_t *ilc, bool sawtut);
static void ilc_linkwd(ilc_info_t *ilc, bool sawtut);
static void ilc_csawd(ilc_info_t *ilc);


#ifdef BCMDBG
static void ilc_dumpilc(ilc_info_t *ilc, uchar *buf);
static void ilc_dumpregs(ilc_info_t *ilc, uchar *buf);
#endif

static uint ilc_capture_chest(ilc_info_t *ilc, uint32 timestamp);
#ifdef IL_STATS
static void ilc_chipctrupd(ilc_info_t *ilc);
#endif

int
ilc_attach(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uint32 w;
	uint sprom;

	/* some code depends on packed structures */
	ASSERT(sizeof (struct ether_addr) == ETHER_ADDR_LEN);
	ASSERT(sizeof (struct ether_header) == ETHER_HDR_LEN);
	ASSERT(sizeof (ilcp_t8hdr_t) == 2);
	ASSERT(sizeof (rate_ctrl_t) == 16);
	ASSERT(sizeof (csa_t) == 34);
	ASSERT(sizeof (ilcp_v0_cds_t) == 8);
	ASSERT(sizeof (ilcp_v0_cdcf_t) == 14 + sizeof (ilcp_v0_cds_t));

	regs = ilc->regs;

	/* add to the global linked list */
	ilc->next = ilc_list;
	ilc_list = ilc;

	/* set pci vendor and device ids */
	ilc->vendorid = R_REG(&regs->pcicfg[0]);
	ilc->deviceid = R_REG(&regs->pcicfg[1]);

	validate_chip_access(ilc);

	sprom = R_REG(&regs->devstatus) & DS_SP;

#ifdef NOSPROM
	ASSERT(sprom == 0);
#else
	/* read afe revision */
	ilc->aferev = R_REG(&regs->afestatus) & AFES_RV_MASK;

	/* validate sprom hw and sys regions (0x0-0x3f) contents */
	if (sprom && !ilc_spromgood(ilc, 0, 0x40)) {
		IL_ERROR(("il%d: ilc_attach: bad sys sprom crc8\n", ilc->unit));
		return (-1);
	}

	/* if sprom present, set perm/cur_etheraddr */
	if (sprom) {
		w = R_REG(&regs->enetaddrupper);
		ilc->perm_etheraddr.octet[0] = (w >> 8) & 0xff;
		ilc->perm_etheraddr.octet[1] = w & 0xff;
		w = R_REG(&regs->enetaddrlower);
		ilc->perm_etheraddr.octet[2] = (w >> 24) & 0xff;
		ilc->perm_etheraddr.octet[3] = (w >> 16) & 0xff;
		ilc->perm_etheraddr.octet[4] = (w >> 8) & 0xff;
		ilc->perm_etheraddr.octet[5] = w & 0xff;
		bcopy(&ilc->perm_etheraddr, &ilc->cur_etheraddr, ETHER_ADDR_LEN);

		/* validate ethernet address */
		if (ETHER_ISNULLADDR(&ilc->perm_etheraddr)
			|| ETHER_ISBCAST(&ilc->perm_etheraddr)) {
			IL_ERROR(("il%d: ilc_attach: bad sprom etheraddr\n", ilc->unit));
			return (-1);
		}

		/* set pci subvendor and subid */
		ilc->subvendor = R_REG(&regs->pcicfg[0x16]);
		ilc->subid = R_REG(&regs->pcicfg[0x17]);
	}

	/* subvendor and subid should be set by now */
	ASSERT(ilc->subvendor);
	ASSERT(ilc->subid);

	/* all non-Epigram/non-Broadcom cards are RNICs */
	if ((ilc->subvendor != VENDOR_EPIGRAM)
		&& (ilc->subvendor != VENDOR_BROADCOM))
		ilc->subid = EPIGRAM_RNIC;

	/* if sprom present, read 2byte board major.minor revision */
	if (sprom) {
		w = R_REG(&regs->sprom[0x18]);
		ilc->bdmajrev = (w >> 8) & 0xff;
		ilc->bdminrev = w & 0xff;
	}
#endif	/* NOSPROM */

	/* set default afe revision */
	if (ilc->aferev == 0)
		ilc->aferev = AFE_B0;

	/* tx bpb override is off by default */
	ilc->txbpb = -1;

	/* tx priority override is off by default */
	ilc->txpri = -1;

	/* link integrity is on by default */
	ilc->linkint = TRUE;


	ilc->li_force_ctr = (ilc->perm_etheraddr.octet[5] & LI_FORCE_MASK) + LI_FORCE_MIN;
	ilc->linkstate = LI_DOWN;

	/* set starting hpna driver mode */
	ilc->pcom = PCOM_V2;

	ilc->macmode = ilc_macmode(ilc);
	/* clear forcing state */
	ilc->xmt_tut = FALSE;
	ilc->xmt_gapped = FALSE;
	ilc->forcetut = 0;


	/* rx data header length is chip-specific */
	ilc->rxhdrlen = (IS4210(ilc->deviceid) || IS4211(ilc->deviceid))?
		RXHDR_2MBAUD_LEN: RXHDR_4MBAUD_LEN;


#if BYTE_ORDER== BIG_ENDIAN
	ilc->bigendian = TRUE;
#else
	ilc->bigendian = FALSE;
#endif

#ifdef IL_CERT
	/* enable gptimer interrupts if certification test suite is compiled in */
	ilc->intmask |= (PIOMODE(ilc)? MSII_TO: I_TO);
#endif

	return (0);
}

void
ilc_detach(ilc_info_t *ilc)
{
	ilc_info_t **prevp;

	/* remove ilc from global linked list */
	for (prevp = &ilc_list; *prevp; prevp = &(*prevp)->next)
		if (*prevp == ilc) {
			*prevp = ilc->next;
			break;
		}

	/* free stats structure */
	if (ilc->stats.phy_if) {
		il_mfree(ilc->stats.phy_if, STATS_MEM_SIZE(&ilc->stats.sizes));
		ilc->stats.phy_if = NULL;
	}

}

void
ilc_reset(ilc_info_t *ilc)
{
	IL_TRACE(("il%d: ilc_reset\n", ilc->unit));

#ifdef IL_STATS
	/* read chip counters */
	ilc_chipctrupd(ilc);

	/* and clear the software copy of the chip counters */
	bzero(&ilc->oldhwctr, sizeof (struct hwctrs));
#endif

	/* zero our software copy of the ctr1linkrcvd register */
	ilc->ctr1linkrcvd = 0;

	/* reset the chip */
	ilc_chipreset(ilc);

	/* free any posted tx packets */
	il_txreclaim((void*)ilc, TRUE);

	/* free any posted rx packets */
	il_rxreclaim((void*)ilc);
}

void
ilc_init(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uint newmacmode;

	IL_TRACE(("il%d: ilc_init\n", ilc->unit));

	regs = ilc->regs;

	ASSERT(ilc->pioactive == NULL);

	/* select operating mac mode */
	newmacmode = ilc_macmode(ilc);
	if (newmacmode != ilc->macmode)
		ilc_modepurge(ilc, newmacmode);
	ilc->macmode = newmacmode;


	/* init the chip */
	ilc_chipinit(ilc);

	if (PIOMODE(ilc))
		OR_REG(&regs->rcvcontrol, RC_FM);
#ifdef DMA
	else {
		/* clear tx descriptor ring */
		BZERO_SM((void*)ilc->txd, NTXD * sizeof (bcm42xxdd_t));
		ilc->txout = ilc->txin = 0;

		/* clear rx descriptor ring */
		BZERO_SM((void*)ilc->rxd, NRXD * sizeof (bcm42xxdd_t));
		ilc->rxout = ilc->rxin = 0;

		/* init the tx dma engine */
		OR_REG(&regs->xmtcontrol, XC_XE);
		W_REG(&regs->xmtaddr, (uint32) ilc->txdpa);
		ASSERT(R_REG(&regs->xmtptr) == 0);

		/* init the rx dma engine */
		W_REG(&regs->rcvcontrol,
			((HWRXOFF << RC_RO_SHIFT) | RC_RE));
		W_REG(&regs->rcvaddr, (uint32) ilc->rxdpa);
		ASSERT(R_REG(&regs->rcvptr) == 0);

		/* post receive buffers */
		ilc_fill(ilc);
	}
#endif

	/* set MSI byte swap bit - has no effect in dma mode */
	if (ilc->bigendian)
		OR_REG(&regs->devcontrol, DC_BS);

	/* enable tx and rx */
	OR_REG(&regs->devcontrol, (DC_XE | DC_RE));

	/* apply debug overrides */
	ilc_debug(ilc);
}

/* mark interface up */
void
ilc_up(ilc_info_t *ilc)
{
	ilc->up = TRUE;

	/* intmask should have been set by per-port code */
	ASSERT(ilc->intmask);

	il_init((void*)ilc);

	if (ilc->csa)
		ilc_csaup(ilc);
}

/* mark interface down */
uint
ilc_down(ilc_info_t *ilc, int reset)
{
	uint callback;

	callback = 0;

	ilc->up = FALSE;
	if (reset)
		il_reset((void*)ilc);


	/* clear compatibility mode timers */
	ilc->v1_detected = ilc->v1_signaled = 0;

	/* clear forcing state */
	ilc->xmt_tut = FALSE;
	ilc->xmt_gapped = FALSE;
	ilc->forcetut = 0;

	/* suppress link state changes during power management mode changes */
	if (!ilc->pm_modechange && (ilc->linkstate != LI_DOWN)) {
		ilc->linkstate = LI_DOWN;
		il_link_down((void*)ilc);
		W_REG(&ilc->regs->festatus, FES_LEDOFF);
	}

	return (callback);
}

/* common ioctl handler.  return: 0=ok, -1=error */
int
ilc_ioctl(ilc_info_t *ilc, int cmd, void *arg)
{
	int error;
	int val;

	error = 0;

	val = arg? *(int*)arg: 0;

	IL_TRACE(("il%d: ilc_ioctl: cmd 0x%x\n", ilc->unit, cmd));

	switch (cmd) {
	case ILCTXDOWN:
		il_down(ilc, 0);
		break;

	case ILCLOOP:
		ilc_loopback(ilc, val);
		break;

	case ILCDUMP:
		il_dump(ilc, (uchar*) arg, 4096);
		break;

#ifdef BCMDBG
	case ILCDUMPSCB:
		ilc_dumpscb(ilc, (uchar*)arg, 4096);
		break;
#endif

	case ILCTXBPB:
		if (ilc_txbpb(ilc, val))
			error = -1;
		break;

	case ILCTXPRI:
		if (ilc_txpri(ilc, val))
			error = -1;
		break;
#ifdef DBG
	case ILCSETMSGLEVEL:
		il_msg_level = val;
		break;
#endif

	case ILCPROMISCTYPE:
		ilc->promisctype = (bool) val;
		break;

	case ILCLINKINT:
		if (val != ilc->linkint) {
			ilc->linkint = (bool) val;
			if (ilc->linkint == FALSE) {
				ilc->linkstate = LI_DOWN;
				il_link_down((void*)ilc);
				W_REG(&ilc->regs->festatus, FES_LEDOFF);
			}
		}
		break;

	case ILCHPNAMODE:
		/* only set state variables here - chipinit happens in watchdog */
		ilc->pcom = PCOM_V2;
		ilc->xmt_gapped = FALSE;
		ilc->v1_detected = ilc->v1_signaled = 0;
		ilc->localforce = (val == MODE_AUTO)? FALSE: TRUE;

		if ((val >= MODE_AUTO) && (val <= MODE_COMPAT)) {
			ilc->configmode = val;
		} else if (val == 4) {
			ilc->configmode = MODE_TUT;
			ilc->pcom = PCOM_V1;
		} else if (val == 5) {
			ilc->configmode = MODE_COMPAT;
			ilc->xmt_gapped = TRUE;
		} else {
			ilc->configmode = MODE_AUTO;
			ilc->localforce = FALSE;
		}

		/*
		 * if no macmode transition actually results from the ioctl,
		 * need to force the state init to happen anyway.
		 * this gives testers a set of known initial conditions.
		 */
		if (ilc_macmode(ilc) == ilc->macmode)
			ilc_modepurge(ilc, ilc->macmode);

		break;


	default:
		error = -1;
	}

	return (error);
}

static void
ilc_loopback(ilc_info_t *ilc, int on)
{
	IL_TRACE(("il%d: ilc_loopback: %d\n", ilc->unit, on));

	ilc->loopbk = (bool) on;
	il_init((void*)ilc);
}


void
ilc_promisc(ilc_info_t *ilc, uint on)
{
	IL_TRACE(("il%d: ilc_promisc: %d\n", ilc->unit, on));

	ilc->promisc = (bool) on;
	il_init((void*)ilc);
}

#ifdef BCMDBG
void
ilc_dump(ilc_info_t *ilc, uchar *buf, int size)
{
	/* big enough */
	if (size < 3700)
		return;

	ilc_dumpilc(ilc, buf);
	//ilc_dumpregs(ilc, buf + strlen(buf));
}

static void
ilc_dumpscb(ilc_info_t *ilc, uchar *buf, int size)
{
	uint k, j;
	struct scb *scb;

	/* big enough */
	if (size < 4000)
		return;

	buf += sprintf(buf, "idx ether_addr           hpna txd  rxd  bebi bebl  iok  exp  expk\n");

	for (k=0; k<NSCB; k++) {
		scb = &(ilc->scb[k]);
		if (scb->inuse && !ETHER_ISMULTI(&(scb->ea))) {

			buf += sprintf(buf, "%02d  %02x:%02x:%02x:%02x:%02x:%02x ", k,
				scb->ea.octet[0],
				scb->ea.octet[1],
				scb->ea.octet[2],
				scb->ea.octet[3],
				scb->ea.octet[4],
				scb->ea.octet[5]);
			buf += sprintf(buf, "%4d %4d %4d %4d %4d %4d %4d %4d",
				scb->hpnatype,
				scb->fc.txdesc,
				scb->rx_desc,
				scb->bebi,
				scb->bebl,
				scb->ilineokay,
				scb->expire,
				scb->expirek);

			buf += sprintf(buf, "\n");
		}
	}

	buf += sprintf(buf, "idx ether_addr           sd mc_hist\n");

	for (k=0; k<NSCB; k++) {
		scb = &(ilc->scb[k]);
		if (scb->inuse && ETHER_ISMULTI(&(scb->ea))) {

			buf += sprintf(buf, "%02d  %02x:%02x:%02x:%02x:%02x:%02x    ", k,
				scb->ea.octet[0],
				scb->ea.octet[1],
				scb->ea.octet[2],
				scb->ea.octet[3],
				scb->ea.octet[4],
				scb->ea.octet[5]);
			buf += sprintf(buf, "%x  ", scb->mc_sd0);
			for (j=0; j<NSCB; j++) {
				buf += sprintf(buf, "%x", scb->mc_hist[j]);
				if (!((j+1)%8)) buf += sprintf(buf, " ");
			}
			buf += sprintf(buf, "\n");
		}
	}
}

static void
ilc_dumpilc(ilc_info_t *ilc, uchar *buf)
{
	char perm[32], cur[32];
	uint *primap;

	uint32 *counters = NULL;

#ifdef IL_STATS
	if (ilc->stats.counter_block)
		counters = &COUNTER(ilc, 0);
#endif

	if (SENDLARQ(ilc))
		primap = ilc->txprimap;
	else
		primap = il_null_txpri;

	buf += sprintf(buf, "ilc 0x%lx regs 0x%lx msglevel %d debugstr \"%s\"\n",
		(ulong)ilc, (ulong)ilc->regs, il_msg_level, ilc->debug);
	buf += sprintf(buf, "up %d promisc %d promisctype %d loopbk %d linkint %d\n",
		ilc->up, ilc->promisc, ilc->promisctype, ilc->loopbk, ilc->linkint);
	buf += sprintf(buf, "txbpb %d txpri %d enic2inic %d piomode %d pioactive 0x%lx\n",
		ilc->txbpb, ilc->txpri, ilc->enic2inic, ilc->piomode, (ulong)ilc->pioactive);
	buf += sprintf(buf, "larqlevel %d larq_handle 0x%lx nmulticast %d allmulti %d\n",
		ilc->larqlevel, (ulong)ilc->larq_handle, ilc->nmulticast, ilc->allmulti);
	buf += sprintf(buf, "vendor 0x%x device 0x%x chiprev %d\n",
		ilc->vendorid, ilc->deviceid, ilc->chiprev);
	buf += sprintf(buf, "subvendor 0x%x subid 0x%x boardrev %d.%d afe %d\n",
		ilc->subvendor, ilc->subid,
		ilc->bdmajrev, ilc->bdminrev, ilc->aferev);

	buf += sprintf(buf, "perm_etheraddr %s cur_etheraddr %s\n",
		ether_ntoa(&ilc->perm_etheraddr, perm),
		ether_ntoa(&ilc->cur_etheraddr, cur));

#ifdef DMA
	buf += sprintf(buf, "txd 0x%lx txdpa 0x%lx txp 0x%lx txin %d txout %d\n",
		(ulong)ilc->txd, (ulong)ilc->txdpa, (ulong)ilc->txp, ilc->txin, ilc->txout);

	buf += sprintf(buf, "rxd 0x%lx rxdpa 0x%lx rxp 0x%lx rxin %d rxout %d\n",
		(ulong)ilc->rxd, (ulong)ilc->rxdpa, (ulong)ilc->rxp, ilc->rxin, ilc->rxout);
#endif

	buf += sprintf(buf, "macmode %d v1_detected %d v1_signaled %d"
		" configmode %d localforce %d forcetut %d\n",
		ilc->macmode, ilc->v1_detected, ilc->v1_signaled,
		ilc->configmode, ilc->localforce, ilc->forcetut);
	buf += sprintf(buf, "pcom %d ctr1linkrcvd %d xmt_tut %d xmt_gapped %d\n",
		ilc->pcom, ilc->ctr1linkrcvd, ilc->xmt_tut, ilc->xmt_gapped);
	buf += sprintf(buf, "intmask 0x%x dogtimerset %d linkstate %d\n",
		ilc->intmask, ilc->dogtimerset, ilc->linkstate);
	buf += sprintf(buf, "csa %d csa_rtx %d csa_configmode 0x%x\n",
		ilc->csa, ilc->csa_rtx, ilc->csa_configmode);
	buf += sprintf(buf, "csa_newtx 0x%08x csa_prevtx 0x%08x csa_oldtx 0x%08x\n",
		ilc->csa_newtx, ilc->csa_prevtx, ilc->csa_oldtx);
	buf += sprintf(buf, "csa_newrx 0x%08x csa_prevrx 0x%08x\n",
		ilc->csa_newrx, ilc->csa_prevrx);

	buf += sprintf(buf, "\n");

	if (counters) {
		buf += sprintf(buf, "xmtframe %d xmtbyte %d xmterr %d rcvframe %d rcvbyte %d rcverr %d\n",
			counters[IL_STATS_XMT_FRAME_COUNT],
			counters[IL_STATS_XMT_BYTE_COUNT],
			counters[IL_STATS_XMT_ERR_COUNT],
			counters[IL_STATS_RCV_FRAME_COUNT],
			counters[IL_STATS_RCV_BYTE_COUNT],
			counters[IL_STATS_RCV_ERR_COUNT]);
		buf += sprintf(buf, "xmtctrl %d rcvctrl %d xmthdrerr %d rcvhdrerr %d\n",
			counters[IL_STATS_XMT_CTRL_COUNT],
			counters[IL_STATS_RCV_CTRL_COUNT],
			counters[IL_STATS_XMT_HDR_ERR_COUNT],
			counters[IL_STATS_RCV_HDR_ERR_COUNT]);
		buf += sprintf(buf, "xmtbpberr %d rcvbpberr %d hdrcrcerr %d paycrcerr %d\n",
			counters[IL_STATS_XMT_BPB_ERR_COUNT],
			counters[IL_STATS_RCV_BPB_ERR_COUNT],
			counters[IL_STATS_HDR_CRC_ERR_COUNT],
			counters[IL_STATS_PAY_CRC_ERR_COUNT]);
		buf += sprintf(buf, "hdrcverr %d hdrmaxerr %d\n",
			counters[IL_STATS_HDR_CV_ERR_COUNT],
			counters[IL_STATS_HDR_MAX_ERR_COUNT]);
		buf += sprintf(buf, "csgap %d csover %d sat %d trk %d\n",
			counters[IL_STATS_CS_GAP_COUNT],
			counters[IL_STATS_CS_OVER_COUNT],
			counters[IL_STATS_SAT_COUNT],
			counters[IL_STATS_TRK_COUNT]);
		buf += sprintf(buf, "xmtexcol %d rcvrunt %d rcvgiant %d\n",
			counters[IL_STATS_XMT_EXCOL_COUNT],
			counters[IL_STATS_RCV_RUNT_COUNT],
			counters[IL_STATS_RCV_GIANT_COUNT]);
		buf += sprintf(buf, "pcide %d pcida %d dpe %d rduflo %d xfuflo %d rfoflo %d\n",
			counters[IL_STATS_PCI_DE_COUNT],
			counters[IL_STATS_PCI_DA_COUNT],
			counters[IL_STATS_DPE_COUNT],
			counters[IL_STATS_RD_UFLO_COUNT],
			counters[IL_STATS_XF_UFLO_COUNT],
			counters[IL_STATS_RF_OFLO_COUNT]);

		buf += sprintf(buf, "coll %d collframes %d filtered %d missed %d\n",
			counters[IL_STATS_COLL_COUNT],
			counters[IL_STATS_COLLFRAMES_COUNT],
			counters[IL_STATS_FILTERED_COUNT],
			counters[IL_STATS_RCV_MISSED_COUNT]);
		buf += sprintf(buf, "demoderr %d xmt_tooshort %d rcv_tooshort %d linkint %d reset %d\n",
			counters[IL_STATS_DEMODERR_COUNT],
			counters[IL_STATS_XMT_TOOSHORT_COUNT],
			counters[IL_STATS_RCV_TOOSHORT_COUNT],
			counters[IL_STATS_LINKINT_COUNT],
			counters[IL_STATS_RESET_COUNT]);
		buf += sprintf(buf, "ifgviolations %d signalviolations %d stackbubbles %d cls %d\n",
			counters[IL_STATS_IFGV_COUNT],
			counters[IL_STATS_SIGV_COUNT],
			counters[IL_STATS_STACKBUB_COUNT],
			counters[IL_STATS_CLS_COUNT]);

		/* qos counters introduced in bcm4211 */
		if (!IS4210(ilc->deviceid)) {
			buf += sprintf(buf, "collisions_pri:\t%07u %07u %07u %07u %07u %07u %07u %07u\n",
				counters[IL_STATS_QOS_PHY_COLLS],
				counters[IL_STATS_QOS_PHY_COLLS+1],
				counters[IL_STATS_QOS_PHY_COLLS+2],
				counters[IL_STATS_QOS_PHY_COLLS+3],
				counters[IL_STATS_QOS_PHY_COLLS+4],
				counters[IL_STATS_QOS_PHY_COLLS+5],
				counters[IL_STATS_QOS_PHY_COLLS+6],
				counters[IL_STATS_QOS_PHY_COLLS+7]);
			buf += sprintf(buf, "qos use rx 0-3: %010u %010u %010u %010u\n"
				"qos use rx 4-7: %010u %010u %010u %010u\n",
				counters[IL_STATS_QOS_PHY_USAGE],
				counters[IL_STATS_QOS_PHY_USAGE+1],
				counters[IL_STATS_QOS_PHY_USAGE+2],
				counters[IL_STATS_QOS_PHY_USAGE+3],
				counters[IL_STATS_QOS_PHY_USAGE+4],
				counters[IL_STATS_QOS_PHY_USAGE+5],
				counters[IL_STATS_QOS_PHY_USAGE+6],
				counters[IL_STATS_QOS_PHY_USAGE+7]);
			buf += sprintf(buf, "qos use idle:   %010u\n",
				counters[IL_STATS_MAC_IDLE_USAGE]);
		}
		buf += sprintf(buf, "\n");
		buf += sprintf(buf, "txpri map:\t%d %d %d %d %d %d %d %d\n",
			primap[0], primap[1], primap[2], primap[3],
			primap[4], primap[5], primap[6], primap[7]);
		buf += sprintf(buf, "xmt_ll_pri:\t%07u %07u %07u %07u %07u %07u %07u %07u\n",
			counters[IL_STATS_XMT_LL_PRI],
			counters[IL_STATS_XMT_LL_PRI+1],
			counters[IL_STATS_XMT_LL_PRI+2],
			counters[IL_STATS_XMT_LL_PRI+3],
			counters[IL_STATS_XMT_LL_PRI+4],
			counters[IL_STATS_XMT_LL_PRI+5],
			counters[IL_STATS_XMT_LL_PRI+6],
			counters[IL_STATS_XMT_LL_PRI+7]);
		buf += sprintf(buf, "xmt_dfpq_pri:\t%07u %07u %07u %07u %07u %07u %07u %07u\n",
			counters[IL_STATS_XMT_DFPQ_PRI],
			counters[IL_STATS_XMT_DFPQ_PRI+1],
			counters[IL_STATS_XMT_DFPQ_PRI+2],
			counters[IL_STATS_XMT_DFPQ_PRI+3],
			counters[IL_STATS_XMT_DFPQ_PRI+4],
			counters[IL_STATS_XMT_DFPQ_PRI+5],
			counters[IL_STATS_XMT_DFPQ_PRI+6],
			counters[IL_STATS_XMT_DFPQ_PRI+7]);
		buf += sprintf(buf, "rcv_ll_pri:\t%07u %07u %07u %07u %07u %07u %07u %07u\n",
			counters[IL_STATS_RCV_LL_PRI],
			counters[IL_STATS_RCV_LL_PRI+1],
			counters[IL_STATS_RCV_LL_PRI+2],
			counters[IL_STATS_RCV_LL_PRI+3],
			counters[IL_STATS_RCV_LL_PRI+4],
			counters[IL_STATS_RCV_LL_PRI+5],
			counters[IL_STATS_RCV_LL_PRI+6],
			counters[IL_STATS_RCV_LL_PRI+7]);
		buf += sprintf(buf, "rcv_dfpq_pri:\t%07u %07u %07u %07u %07u %07u %07u %07u\n",
			counters[IL_STATS_RCV_DFPQ_PRI],
			counters[IL_STATS_RCV_DFPQ_PRI+1],
			counters[IL_STATS_RCV_DFPQ_PRI+2],
			counters[IL_STATS_RCV_DFPQ_PRI+3],
			counters[IL_STATS_RCV_DFPQ_PRI+4],
			counters[IL_STATS_RCV_DFPQ_PRI+5],
			counters[IL_STATS_RCV_DFPQ_PRI+6],
			counters[IL_STATS_RCV_DFPQ_PRI+7]);
	}

	buf += sprintf(buf, "\n");
}

#define	PRREG(name)	buf += sprintf(buf, #name " 0x%x ", R_REG(&regs->name))

static void
ilc_dumpregs(ilc_info_t *ilc, uchar *buf)
{
	bcm42xxregs_t *regs;

	regs = ilc->regs;

	if (regs == 0)
		return;

	PRREG(devcontrol); PRREG(devstatus);
	PRREG(compat);
	PRREG(wakeuplength);
	if (!IS4210(ilc->deviceid) && !IS4211(ilc->deviceid))
		PRREG(wakeuplength1);
	buf += sprintf(buf, "\n");
	PRREG(intstatus); PRREG(intmask);
	buf += sprintf(buf, "\n");
	PRREG(afestatus); PRREG(afecontrol);
	buf += sprintf(buf, "\n");
	PRREG(frametrim); PRREG(gapcontrol);
	buf += sprintf(buf, "\n");
	PRREG(trackerdebug);
	buf += sprintf(buf, "\n");
	PRREG(festatus); PRREG(fecontrol); PRREG(fecsenseon); PRREG(fecsenseoff);
	buf += sprintf(buf, "\n");
	PRREG(cspower);
	buf += sprintf(buf, "\n");
	PRREG(modcontrol); PRREG(moddebug); PRREG(cdcontrol); PRREG(noisethreshold);
	buf += sprintf(buf, "\n");
	PRREG(framestatus); PRREG(framecontrol);
	buf += sprintf(buf, "\n");
	PRREG(enetaddrlower); PRREG(enetaddrupper);
	buf += sprintf(buf, "\n");
	PRREG(maccontrol); PRREG(pcom);
	buf += sprintf(buf, "\n");
	PRREG(gpiooutput); PRREG(gpioouten); PRREG(gpioinput);
	buf += sprintf(buf, "\n");
	PRREG(msiintstatus); PRREG(msiintmask);
	buf += sprintf(buf, "\n");
	PRREG(funcevnt); PRREG(funcevntmask); PRREG(funcstate);
	buf += sprintf(buf, "\n");
	PRREG(intrecvlazy);
	buf += sprintf(buf, "\n");
	if (PIOMODE(ilc)) {
		PRREG(xmtfifocontrol);
		PRREG(rcvfifocontrol);
		PRREG(msiintstatus);
		buf += sprintf(buf, "\n");
	}
#ifdef DMA
	else {
		PRREG(xmtcontrol); PRREG(xmtaddr);
		PRREG(xmtptr); PRREG(xmtstatus);
		buf += sprintf(buf, "\n");
		PRREG(rcvcontrol); PRREG(rcvaddr);
		PRREG(rcvptr); PRREG(rcvstatus);
		buf += sprintf(buf, "\n");
	}
#endif
	PRREG(ctrcollisions); PRREG(ctrcollframes);
	PRREG(ctrshortframes); PRREG(ctrlongframes);
	PRREG(ctrfilteredframes);
	buf += sprintf(buf, "\n");
	PRREG(ctrmissedframes);
	buf += sprintf(buf, "\n");
	PRREG(ctrerrorframes);
	PRREG(ctr1linkrcvd);
	PRREG(ctrrcvtooshort);
	PRREG(ctrxmtlinkintegrity);
	buf += sprintf(buf, "\n");
	PRREG(ctr1latecoll); PRREG(ctrifgviolation); PRREG(ctrsignalviolation);
	PRREG(ctrstackbubble);
	buf += sprintf(buf, "\n");

	/* qos counters introduced in bcm4211 */
	if (!IS4210(ilc->deviceid)) {
		PRREG(qosusage[0]); PRREG(qosusage[1]);
		PRREG(qosusage[2]); PRREG(qosusage[3]);
		buf += sprintf(buf, "\n");
		PRREG(qosusage[4]); PRREG(qosusage[5]);
		PRREG(qosusage[6]); PRREG(qosusage[7]);
		buf += sprintf(buf, "\n");
		PRREG(qosmacidle);
		buf += sprintf(buf, "\n");
		PRREG(qoscoll[0]); PRREG(qoscoll[1]);
		PRREG(qoscoll[2]); PRREG(qoscoll[3]);
		buf += sprintf(buf, "\n");
		PRREG(qoscoll[4]); PRREG(qoscoll[5]);
		PRREG(qoscoll[6]); PRREG(qoscoll[7]);
		buf += sprintf(buf, "\n");
	}

	/* print tut registers */
	buf += sprintf(buf, "\n");
	buf += sprintf(buf, "tutctl 0x%x tutaidcw 0x%x tutaids 0x%x tutdt 0x%x\n",
		ilc_rtutreg(ilc, TUTCTL), ilc_rtutreg(ilc, TUTAIDCW),
		ilc_rtutreg(ilc, TUTAIDS), ilc_rtutreg(ilc, TUTDT));
	buf += sprintf(buf, "tutmc 0x%x tuttdcst 0x%x tuttaidcst 0x%x tutb 0x%x\n",
		ilc_rtutreg(ilc, TUTMC), ilc_rtutreg(ilc, TUTTDCST),
		ilc_rtutreg(ilc, TUTTAIDCST), ilc_rtutreg(ilc, TUTB));
	buf += sprintf(buf, "tutds 0x%x tutsid 0x%x tutsctl 0x%x tutnctl 0x%x\n",
		ilc_rtutreg(ilc, TUTDS), ilc_rtutreg(ilc, TUTSID),
		ilc_rtutreg(ilc, TUTSCTL), ilc_rtutreg(ilc, TUTNCTL));
	buf += sprintf(buf, "tutifsc1 0x%x tutifsc2 0x%x tuts1c 0x%x tutsc 0x%x\n",
		ilc_rtutreg(ilc, TUTIFSC1), ilc_rtutreg(ilc, TUTIFSC2),
		ilc_rtutreg(ilc, TUTS1C), ilc_rtutreg(ilc, TUTSC));
	buf += sprintf(buf, "tutdcst 0x%x tutaidcst 0x%x tuttgdcst 0x%x tuttgaidcst 0x%x\n",
		ilc_rtutreg(ilc, TUTDCST), ilc_rtutreg(ilc, TUTAIDCST),
		ilc_rtutreg(ilc, TUTTGDCST), ilc_rtutreg(ilc, TUTTGAIDCST));
	buf += sprintf(buf, "tutgaids 0x%x tutgms 0x%x",
		ilc_rtutreg(ilc, TUTGAIDS), ilc_rtutreg(ilc, TUTGMS));
	if (IS4210(ilc->deviceid))
		buf += sprintf(buf, "\n");
	else
		buf += sprintf(buf, " tutss 0x%x\n", ilc_rtutreg(ilc, TUTSS));
}
#endif	/* BCMDBG */

#ifdef DBG
void
ilc_dumphdrs(ilc_info_t *ilc, char *prefix, struct framecontrol *fc,
	uchar *pcomp, bcm42xxrxhdr_t *rxh, uint len)
{
	char da[32], sa[32];
	int snrval;
	uint tmp;
	uint freqest_hi, freqest_lo, freqacc_hi, freqacc_lo;
	struct ether_header *eh;
	uint32 pcom;

	eh = NULL;

	printf("il%d: %s: ", ilc->unit, prefix);

	if (fc) {
		eh = (struct ether_header*) &fc[1];
		printf("dst %s src %s type 0x%x txc %d pri %d len %d ",
			ether_ntoa((struct ether_addr*)eh->ether_dhost, da),
			ether_ntoa((struct ether_addr*)eh->ether_shost, sa),
			ntoh16(eh->ether_type),
			fc->txdesc,
			PRI(fc->ctl), len);
	} else if (pcomp) {
		eh = (struct ether_header*) &pcomp[TUT_HDR_LEN];
		bcopy(pcomp, (uchar*) &pcom, sizeof (uint32));
		pcom = ntoh32(pcom);
		printf("pcom 0x%x dst %s src %s type 0x%x len %d ",
			pcom,
			ether_ntoa((struct ether_addr*)eh->ether_dhost, da),
			ether_ntoa((struct ether_addr*)eh->ether_shost, sa),
			ntoh16(eh->ether_type),
			len);
	}
	if (eh)
		ilc_prhex("data", (uchar*) &eh[1], 8);

	if (rxh == NULL)
		return;

	/* dump appropriate header */
	if (DATATYPE(rxh) == DT_TUT) {
		bcm42xxv1hdr_t *v1rxh = (bcm42xxv1hdr_t *)rxh;

		printf("rxh: framelen %d datatype %d\n", FRAMELEN(v1rxh), DATATYPE(v1rxh));
		printf("\taid 0x%x cw %d pcrc %d satdet %d csover %d rxgain %d\n",
			AID(v1rxh), CW(v1rxh),
			PCRC(v1rxh), SATDET(v1rxh), CSOVER(v1rxh), RXGAIN(v1rxh));
		printf("\tsquelch %d datasquelch %d aidsquelch %d\n",
			v1rxh->squelch, v1rxh->datasquelch, v1rxh->aidsquelch);
	} else if (DATATYPE(rxh) == DT_ILINE) {
		/* part of snr calculation */
		snrval = 277 - HDRCV(rxh);

		/* compute freqest and freqacc printables */
		if (rxh->freqest < 0)
			tmp = -rxh->freqest;
		else
			tmp = rxh->freqest;
		freqest_hi = tmp >> 5;
		freqest_lo = tmp & 31;

		if (rxh->freqacc < 0)
			tmp = -rxh->freqacc;
		else
			tmp = rxh->freqacc;
		freqacc_hi = tmp >> 5;
		freqacc_lo = tmp & 31;

		printf("rxh: framelen %d datatype %d\n", FRAMELEN(rxh), DATATYPE(rxh));
		printf("\tfterr %d hcrc %d pcrc %d rterr %d satdet %d "
			"csover %d compat %d rxpri %d\n",
			FTERR(rxh), HCRC(rxh), PCRC(rxh), RTERR(rxh),
			SATDET(rxh), CSOVER(rxh), COMPAT(rxh),
			RXPRI(rxh));
		printf("\tsymcount %d freqest %s%d(%d) freqacc %s%d(%d) hdrmaxerr %d paymaxerr %d\n",
			rxh->symcount,
			(rxh->freqest < 0)? "-": "", freqest_hi, freqest_lo,
			(rxh->freqacc < 0)? "-": "", freqacc_hi, freqacc_lo,
			rxh->hdrmaxerr, rxh->paymaxerr);
	} else {
		printf("rxh: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			((uint32 *)rxh)[0], ((uint32 *)rxh)[1], ((uint32 *)rxh)[2],
			((uint32 *)rxh)[3], ((uint32 *)rxh)[4]);
	}
}

void
ilc_prhex(char *msg, uchar *buf, uint nbytes)
{
	char line[256];
	uint i, li;

	printf("%s: ", msg);

	li = 0;
	for (i = 0; i < nbytes; i++) {
		sprintf(&line[li], "%02x ", buf[i]);
		li += 3;
		if (li >= 48) {
			printf("%s\n", line);
			li = 0;
		}
	}
	if (li > 0)
		printf("%s\n", line);
}
#endif /* DBG */

char*
ether_ntoa(struct ether_addr *e, char *buf)
{
	sprintf(buf,"%x:%x:%x:%x:%x:%x",
		e->octet[0], e->octet[1], e->octet[2],
		e->octet[3], e->octet[4], e->octet[5]);
	return (buf);
}

void
ilc_chipreset(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;

	IL_TRACE(("il%d: ilc_chipreset\n", ilc->unit));

	regs = ilc->regs;

	/* bang you're dead */
	W_REG(&regs->devcontrol, DC_FG | DC_RS);
	il_delay(10);
	W_REG(&regs->devcontrol, DC_FG);

	/* calibrate afe */
	AND_REG(&regs->afecontrol, ~AFEC_PD);
	il_delay(150);
	OR_REG(&regs->afecontrol, AFEC_CL);
	il_delay(64);
	AND_REG(&regs->afecontrol, ~AFEC_CL);

	/*  - software LED0 select */
	W_REG(&regs->festatus, (uint16)((ilc->linkstate != LI_DOWN)? FES_LEDON : FES_LEDOFF));

	/* set LED after a chipreset to reflect the current state */
	if (ilc->linkint && (ilc->linkstate > LI_DOWN))
		W_REG(&regs->festatus, FES_LEDON);
	else
		W_REG(&regs->festatus, FES_LEDOFF);
}


/*
 * Initialize all the chip registers.  If dma mode, init tx and rx dma engines
 * but leave the devcontrol tx and rx (fifos) disabled.
 */
void
ilc_chipinit(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uint32 fcbits, w;
	uint aid;
	uint idx;
	uint i;

	regs = ilc->regs;
	fcbits = 0;

	IL_TRACE(("il%d: ilc_chipinit\n", ilc->unit));

	/*
	 * The chip uses the contents of the enetaddrupper/lower registers
	 * as the seed for the MAC random number generator.
	 * IFF no sprom, write cur_etheraddr back into the chip registers
	 * so they aren't left with the value zero.
	 */
	if (!(R_REG(&regs->devstatus) & DS_SP)) {
		w = ((uint)ilc->cur_etheraddr.octet[0] << 8) | ilc->cur_etheraddr.octet[1];
		W_REG(&regs->enetaddrupper, w);
		w = ((uint)ilc->cur_etheraddr.octet[2] << 24)
			| ((uint)ilc->cur_etheraddr.octet[3] << 16)
			| ((uint)ilc->cur_etheraddr.octet[4] << 8)
			| ilc->cur_etheraddr.octet[5];
		W_REG(&regs->enetaddrlower, w);
	}

	/* chip uses the enetaddr regs for its random number seed */
	ASSERT(R_REG(&ilc->regs->enetaddrlower) != 0);

	/* set rxgain2 */
	W_REG(&regs->fedbgaddr, 0x287);
	W_REG(&regs->fedbgdata, 0x0);

	/*
	 * Set txgain:
	 * For native iline, set txgain off to zero.
	 * For tut/compat, set txgain off to the hpna gain on value.
	 */
	W_REG(&regs->fedbgaddr, 0x288);
	if (ilc->subid == EPIGRAM_ENIC)
		W_REG(&regs->fedbgdata, 0x2828);
	else
		W_REG(&regs->fedbgdata, 0x7000);

	/* set HPNA gain */
	W_REG(&regs->fedbgaddr, 0x289);
	if (ilc->subid == EPIGRAM_ENIC)
		W_REG(&regs->fedbgdata, 0x287c);
	else
		W_REG(&regs->fedbgdata, 0x707c);

	/* set tx slot count */
	W_REG(&regs->fedbgaddr, 0x200);
	W_REG(&regs->fedbgdata, 0xa8);

	/* set signal slot count */
	W_REG(&regs->fedbgaddr, 0x201);
	W_REG(&regs->fedbgdata, 0x100);

	/* frametrim register format change introduced in the 4211 */
	if (ilc->macmode == MODE_COMPAT) {
		if (IS4210(ilc->deviceid))
			W_REG(&regs->frametrim, 0xec5d);
	}

	/*  - incorrect FrametrimMM hardware default */
	if (IS4211(ilc->deviceid)) {
		W_REG(&regs->fedbgaddr, 0x2c0);
		W_REG(&regs->fedbgdata, 0x2458);
	}

	/* set collision limit to 255 (DFPQ) or 15 (BEB) */
	if (ilc->macmode == MODE_ILINE)
		W_REG(&regs->maccontrol, 255);
	else
		W_REG(&regs->maccontrol, 15);

	/* for tut/mixed mode, increase cs threshold by 0.5 dB */
	if ((ilc->macmode == MODE_COMPAT) || (ilc->macmode == MODE_TUT))
		W_REG(&regs->fecsenseon, 0x0e24);

	/* set fecsenseoff, cspower, cdcontrol, and noisethreshold */
	W_REG(&regs->fecsenseoff, 0x2326);
	W_REG(&regs->cspower, 0x30);
	W_REG(&regs->cdcontrol, 0x2406);
	W_REG(&regs->noisethreshold, 0xff03);

	/* 4210 fecontrol workaround */
	if (IS4210(ilc->deviceid))
		ilc_chipinitfec(ilc);

	W_REG(&regs->wakeuplength, 0x80808080);

	/*  - 4210-specific tut/compat mode gain table */
	if (IS4210(ilc->deviceid)
		&& ((ilc->macmode == MODE_COMPAT) || (ilc->macmode == MODE_TUT))) {
		W_REG(&regs->fedbgaddr, 0x280);
		W_REG(&regs->fedbgdata, 0x1004);
		W_REG(&regs->fedbgaddr, 0x281);
		W_REG(&regs->fedbgdata, 0x3020);
		W_REG(&regs->fedbgaddr, 0x282);
		W_REG(&regs->fedbgdata, 0x5040);
		W_REG(&regs->fedbgaddr, 0x283);
		W_REG(&regs->fedbgdata, 0x1f1f);
		W_REG(&regs->fedbgaddr, 0x284);
		W_REG(&regs->fedbgdata, 0x0);
		W_REG(&regs->fedbgaddr, 0x285);
		W_REG(&regs->fedbgdata, 0x0);
	}

	/* set overrunctr to handle longer max-length mixed-mode frames */
	W_REG(&regs->fedbgaddr, 0x206);
	W_REG(&regs->fedbgdata, 0x8800);

	/* paranoia - set fedbgaddr to zero when all snoop init accesses are complete */
	W_REG(&regs->fedbgaddr, 0);

	/* enable modem - snoop registers cannot be accessed when modem is on */
	W_REG(&regs->devcontrol, DC_ME | DC_PM);

	/* if channel estimate mode is on, set trigger */
	if (ilc->cheststate == CH_ON) {
		OR_REG(&regs->moddebug, MD_DT);
		OR_REG(&regs->fecontrol, FEC_CE);
	}

	/* enable one rx interrupt per received frame */
	W_REG(&regs->intrecvlazy, (1 << IRL_FC_SHIFT));

	/* initialize frameproc address filtering */
	if (!ilc->promisc) {
		idx = 0;
		ilc_etherfilter(ilc, idx++, &ilc->cur_etheraddr);
		fcbits |= FC_FE;
		ilc_etherfilter(ilc, idx++, (struct ether_addr*)&ether_bcast);
		if (ilc->allmulti) {
			/* set multicast hash table to all ones */
			for (i = 0; i < BCM42XX_MCHASHSIZE; i += sizeof (int)) {
				W_REG(&regs->enetftaddr, BCM42XX_MCHASHBASE + i);
				W_REG(&regs->enetftdata, 0xffffffff);
			}
			fcbits |= FC_HE;
		} else if (ilc->nmulticast) {
			for (i = 0; i < ilc->nmulticast; i++)
				ilc_etherfilter(ilc, idx++, &ilc->multicast[i]);
		}
		fcbits |= (idx - 1) & FC_LA_MASK;
	}

	/* enable frameproc normal mode */
	/*  - disable hardware random scrambler stuffing and do it in software */
	/*  - disable hardware link integrity - do it in software */
	fcbits |= FC_LC_DEFAULT | FC_PE | FC_CG32 | FC_CG16 | FC_CG8;

	/*  - minimum sized Tut frames are rejected if keepshort is off */
	if (IS4210(ilc->deviceid))
		fcbits |= FC_KS;

	/* for loopback mode, enable frameproc loopback */
	if (ilc->loopbk)
		fcbits |= FC_LE;

	/* randSI is broken in the 4210 and 4211 */
	if (!IS4210(ilc->deviceid) && !IS4211(ilc->deviceid))
		fcbits |= FC_SI;

	/* now write the framecontrol register */
	W_REG(&regs->framecontrol, fcbits);

	/* the pcom register is used only by hw link integrity */
	W_REG(&regs->pcom, ilc->pcom);

	/* compute a semi-unique AID */
	aid = ilc->perm_etheraddr.octet[5] & AIDMASK;

	/* initialize tut registers (make these symbolic once they settle down) */
	ilc_wtutreg(ilc, TUTAIDCW, (uint16) ((0x44 << 8) | aid));
	ilc_wtutreg(ilc, TUTGAIDS, 0xf03);
	W_REG(&regs->gapcontrol, 0xa9d2);
	ilc_wtutreg(ilc, TUTCTL, 0x2140);
	ilc_wtutreg(ilc, TUTB, 0x23f);
	ilc_wtutreg(ilc, TUTSCTL, 0x1df);
	ilc_wtutreg(ilc, TUTAIDS, 0x3);
	ilc_wtutreg(ilc, TUTNCTL, 0x0e03);
	if (IS4210(ilc->deviceid))
		ilc_wtutreg(ilc, TUTDT, 0x2873);
	else
		ilc_wtutreg(ilc, TUTDT, 0x29f4);

	/* set compat register to enable native or compatibility mode */
	if (ilc->macmode == MODE_ILINE)
		W_REG(&regs->compat, CM_IL | CM_CE | CM_CL);
	else
		W_REG(&regs->compat, CM_TG | CM_MS | CM_IL | CM_CE | CM_CL);

#ifndef NOINTS
	/* write interrupt mask */
	if (PIOMODE(ilc))
		W_REG(&regs->msiintmask, ilc->intmask);
	else
		W_REG(&regs->intmask, ilc->intmask);
#endif
}

static void
ilc_chipinitfec(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;

	regs = ilc->regs;

	ASSERT(IS4210(ilc->deviceid));

	/*
	 *  bcm4210 rxgain bug
	 * When the fecontrol.rxgain is enabled during chip initialization,
	 * there is a bug in the 4210 chiprev0 and chiprev1 where the first few
	 * samples are never subtracted from the running rxgain delay window.
	 * If these first few samples have high values, this will cause
	 * the computed rxgain value for attenuated networks to be too low.
	 *
	 * The workaround is to turn off rxgain enable and carrier sense,
	 * turn on dc blocking, power down the AFE, wait for the AFE
	 * to settle and the dc blocking filter to settle to "zero",
	 * turn on rxgain enable and carrier sense, wait for the first
	 * few samples to have been read, then power on and recalibrate the AFE.
	 */
	W_REG(&regs->fecontrol, 0x235f);
	OR_REG(&regs->afecontrol, AFEC_PD);
	il_delay(2);
	W_REG(&regs->fecontrol, 0x23ff);
	il_delay(1);
	AND_REG(&regs->afecontrol, ~AFEC_PD);
	il_delay(150);
	OR_REG(&regs->afecontrol, AFEC_CL);
	il_delay(64);
	AND_REG(&regs->afecontrol, ~AFEC_CL);
}

/* program an ethernet filter table address entry */
void
ilc_etherfilter(ilc_info_t *ilc, uint index, struct ether_addr *ea)
{
	bcm42xxregs_t *regs;
	uint ftaddr;

	regs = ilc->regs;
	ftaddr = index * 8;

	/* low word */
	W_REG(&regs->enetftaddr, ftaddr);
	W_REG(&regs->enetftdata,
		((uint32)ea->octet[2] << 24) | ((uint32)ea->octet[3] << 16)
		| ((uint32)ea->octet[4] << 8) | (uint32)ea->octet[5]);
	/* high word */
	W_REG(&regs->enetftaddr, ftaddr+4);
	W_REG(&regs->enetftdata, ((uint32)ea->octet[0] << 8) | (uint32)ea->octet[1]);
}

static void
ilc_chiperrors(ilc_info_t *ilc, uint intstatus)
{
	bool fatal;
	uint instance;

	/* ignore any non-error bits */
	intstatus &= PIOMODE(ilc) ? MSII_ERRORS : I_ERRORS;

	instance = ilc->unit;
	fatal = FALSE;

	IL_TRACE(("il%d: ilc_chiperrors: intstatus 0x%x\n", instance, intstatus));

	if (intstatus & (PIOMODE(ilc) ? MSII_EC : I_EC)) {
		IL_ERROR(("il%d: excessive collisions\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_EXCOL_COUNT)++;
	}

	if (intstatus & (PIOMODE(ilc) ? MSII_XH : I_XH)) {
		IL_ERROR(("il%d: transmit header error\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_HDR_ERR_COUNT)++;
	}

	if (intstatus & (PIOMODE(ilc) ? MSII_RO : I_RO)) {
		IL_ERROR(("il%d: receive fifo overflow\n", instance));
		COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_RF_OFLO_COUNT)++;
		fatal = TRUE;
	}

#ifdef DMA
	if (!PIOMODE(ilc) && (intstatus & I_PC)) {
		IL_ERROR(("il%d: pci descriptor error\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_PCI_DE_COUNT)++;
		fatal = TRUE;
	}

	if (!PIOMODE(ilc) && (intstatus & I_PD)) {
		IL_ERROR(("il%d: pci data error\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_PCI_DA_COUNT)++;
		fatal = TRUE;
	}

	if (!PIOMODE(ilc) && (intstatus & I_DE)) {
		IL_ERROR(("il%d: descriptor protocol error\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_DPE_COUNT)++;
		fatal = TRUE;
	}

	if (!PIOMODE(ilc) && (intstatus & I_RU)) {
		IL_ERROR(("il%d: receive descriptor underflow\n", instance));
		COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_RD_UFLO_COUNT)++;
		fatal = TRUE;
	}

	if (!PIOMODE(ilc) && (intstatus & I_XU)) {
		IL_ERROR(("il%d: transmit fifo underflow\n", instance));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XF_UFLO_COUNT)++;
		fatal = TRUE;
	}
#endif	/* DMA */

	if (fatal) {
		/* big hammer */
		il_init((void*)ilc);
	}
}

#ifdef IL_STATS
/*
 * Read the chip performance and event counters,
 * compare against the last read value, and, if it differs,
 * increment the software aggregate counter by the delta.
 */
#define	CTRUPD(name, max, index) { \
	v = R_REG(&regs->name); \
	if (v != ilc->oldhwctr.name) { \
		if (v > ilc->oldhwctr.name) \
			delta = v - ilc->oldhwctr.name; \
		else \
			delta = (((max) - ilc->oldhwctr.name) + v); \
		COUNTER(ilc, index) += delta; \
		ilc->oldhwctr.name = v; \
	} \
}

static void
ilc_chipctrupd(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uint v;
	uint delta;
	int i;

	regs = ilc->regs;

	CTRUPD(ctrcollisions, 0xffffffff, IL_STATS_COLL_COUNT);
	CTRUPD(ctrcollframes, 0xffffffff, IL_STATS_COLLFRAMES_COUNT);
	CTRUPD(ctrshortframes, 0xffffffff, IL_STATS_RCV_TOOSHORT_COUNT);
	CTRUPD(ctrlongframes, 0xffffffff, IL_STATS_RCV_GIANT_COUNT);
	CTRUPD(ctrfilteredframes, 0xffffffff, IL_STATS_FILTERED_COUNT);
	CTRUPD(ctrmissedframes, 0xffffffff, IL_STATS_RCV_MISSED_COUNT);
	CTRUPD(ctrerrorframes, 0xffffffff, IL_STATS_DEMODERR_COUNT);
	CTRUPD(ctrrcvtooshort, 0xffff, IL_STATS_RCV_TOOSHORT_COUNT);
	CTRUPD(ctrxmtlinkintegrity, 0xffff, IL_STATS_LINKINT_COUNT);
	CTRUPD(ctrifgviolation, 0xffff, IL_STATS_IFGV_COUNT);
	CTRUPD(ctrsignalviolation, 0xffff, IL_STATS_SIGV_COUNT);
	CTRUPD(ctrstackbubble, 0xffff, IL_STATS_STACKBUB_COUNT);

	if (!IS4210(ilc->deviceid)) {
		for (i = 0; i < 8; i++)
			CTRUPD(qosusage[i], 0xffffffff, IL_STATS_QOS_PHY_USAGE+i);
		for (i = 0; i < 8; i++)
			CTRUPD(qoscoll[i], 0xffff, IL_STATS_QOS_PHY_COLLS+i);
		CTRUPD(qosmacidle, 0xffffffff, IL_STATS_MAC_IDLE_USAGE);
	}
}
#endif	/* IL_STATS */

#ifdef DMA
/* allocate and post receive buffers */
void
ilc_fill(ilc_info_t *ilc)
{
	void *p;
	uint rxin, rxout;
	uint ctrl;
	uint n;
	uint i;
	uchar *pa;

	/*
	 * Determine how many receive buffers we're lacking
	 * from the full complement, allocate, initialize,
	 * and post them, then update the chip rx LastDscr.
	 */

	rxin = ilc->rxin;
	rxout = ilc->rxout;

	n = NRXBUFPOST - NRXDACTIVE(rxin, rxout);

	IL_TRACE(("il%d: ilc_fill %d\n", ilc->unit, n));

	for (i = 0; i < n; i++) {
		if ((p = il_pget((void*)ilc, FALSE, RXBUFSZ, NULL, &pa)) == NULL) {
			IL_ERROR(("il%d: ilc_fill: out of rxbufs\n", ilc->unit));
			COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
			break;
		}

		/* save the free packet pointer */
		ASSERT(ilc->rxp[rxout] == NULL);
		ilc->rxp[rxout] = p;

		/* paranoia */
		ASSERT(R_SM(&ilc->rxd[rxout].addr) == 0);

		/* prep the descriptor control value */
		ctrl = RXBUFSZ;
		if (rxout == (NRXD - 1))
			ctrl |= CTRL_EOT;

		/* init the rx descriptor */
		W_SM(&ilc->rxd[rxout].ctrl, ctrl);
		W_SM(&ilc->rxd[rxout].addr, (uint32) pa);

		rxout = NEXTRXD(rxout);
	}

	ilc->rxout = rxout;

	/* update the chip LastDscr pointer */
	W_REG(&ilc->regs->rcvptr, I2B(ilc->rxout));
}
#endif	/* DMA */


#ifndef NOINTS
/* common interrupt code */
void
ilc_intr(ilc_info_t *ilc, uint intstatus)
{
	if (PIOMODE(ilc)) {
		/* rx */
		if (intstatus & MSII_RI)
			il_rxintr((void*)ilc);

		/* tx */
		if (intstatus & MSII_XI)
			il_txintr((void*)ilc);

		/* chip errors */
		if (intstatus & MSII_ERRORS)
			ilc_chiperrors(ilc, intstatus);

#ifdef IL_CERT
		if (intstatus & MSII_TO)
			ilc_intrcert(ilc);
#endif
	}
#ifdef DMA
	else {
		/* rx */
		if (intstatus & I_RI)
			il_rxintr((void*)ilc);

		/* tx */
		if (intstatus & I_XI)
			il_txintr((void*)ilc);

		/* chip errors */
		if (intstatus & I_ERRORS)
			ilc_chiperrors(ilc, intstatus);

#ifdef IL_CERT
		if (intstatus & I_TO)
			ilc_intrcert(ilc);
#endif
	}
#endif	/* DMA */

	/* channel estimate cleanup */
	if (ilc->cheststate == CH_TRIGGERED) {
		ilc->cheststate = CH_OFF;
		ilc->promisc = FALSE;
		il_init((void*)ilc);
	}
}
#endif	/* NOINTS */

static uint
ilc_datatype(ilc_info_t *ilc, struct scb *scb, int txbpb)
{
	if (ilc->macmode == MODE_ILINE)
		return (DT_ILINE);

	if (ilc->macmode == MODE_TUT)
		return (DT_TUT);

	/* macmode is MODE_COMPAT */

	/* hpnamode 5 override */
	if (ilc->xmt_gapped)
		return (DT_ILINE);

	/* 1.0 or unknown nodes */
	if (scb->hpnatype != HPNAV2)
		return (DT_TUT);

	/* min rate for certain control frames, (RRCFs) */
	if (txbpb == -2)
		return (DT_TUT);

	/* 60 seconds following macmode transition to MODE_COMPAT */
	if (ilc->forcetut)
		return (DT_TUT);

	/* any active 2.0 node has advertised 1.0 */
	if (ilc->xmt_tut)
		return (DT_TUT);

	return (DT_ILINE);
}

/* Common send payload encoding selection. */
static uint8
ilc_pe(ilc_info_t *ilc, int txbpb, uint fc_txdesc)
{
	/* check if there is a global override */
	if (ilc->txbpb >= ILINE_MIN_BPB)
		return ((uint8) (ilc->txbpb - 1));

	/* check if this specific call requires an override */
	if (txbpb >= ILINE_MIN_BPB)
		return ((uint8) (txbpb - 1));

	/* use the min rate for certain control frames, (RRCFs) */
	if (txbpb == -2)
		return (1);

	/* sanity check PE */
	if (IS_ILINE_PE(fc_txdesc))
		return ((uint8) fc_txdesc);

	IL_ERROR(("il%d: ilc_pe: invalid PE: %2d\n", ilc->unit, fc_txdesc));
	return (1);
}

/*
 * Common send preparation.
 * 'p' data must start with ether_header
 * 'p' must allow at least TXOFF bytes of headers to be "pushed" onto the packet
 * 'txpri' is the desired logical (pre-priority-mapping) priority.
 * 'txbpb' is a bpb override: <0=use negotiated value, else use this value.
 */
uint
ilc_send(ilc_info_t *ilc, void *p, int txpri, int txbpb)
{
	struct framecontrol *fc;
	uint32 pcom;
	struct scb *scb;
	uint datatype;
	struct ether_header *eh;
	uchar *h;
	uint len;

	fc = NULL;
	eh = (struct ether_header*) il_ppull((void*)ilc, p, 0);
	len = il_plen(p);

	ASSERT((txpri >= ILINE_MIN_PRI) && (txpri <= ILINE_MAX_PRI));
	ASSERT((txbpb < 0) || ((txbpb >= ILINE_MIN_BPB) && (txbpb <= ILINE_MAX_BPB)));
	ASSERT(len >= ETHER_HDR_LEN);


	COUNTER(ilc, IL_STATS_XMT_LL_PRI + txpri)++;


	/*
	 * Priority mapping:
	 * If global override, use that,
	 * else if pri is out of range, use zero,
	 * else if sending larq, use larq_on mapping,
	 * else (not sending larq) use "1-to-1" mapping.
	 */
	if (ilc->txpri >= 0)
		txpri = ilc->txpri;
	else if ((txpri < 0) || (txpri > ILINE_MAX_PRI))
		txpri = 0;
	else if (SENDLARQ(ilc))
		txpri = ilc->txprimap[txpri];
	else
		txpri = il_null_txpri[txpri];

	COUNTER(ilc, IL_STATS_XMT_DFPQ_PRI + txpri)++;

	/* lookup the remote station control block */
	scb = ilc_scblookup(ilc, eh->ether_dhost);
	ASSERT(scb);

	/*
	 * The outgoing frame format depends upon the current mode,
	 * the type of the remote station, and various tx overrides.
	 */
	datatype = ilc_datatype(ilc, scb, txbpb);

	/* add header */
	h = il_ppush((void*)ilc, p, ILINE_HDR_LEN);
	ASSERT(h);
	if (datatype == DT_ILINE) {
		fc = (struct framecontrol*) h;
		SETSI(scb->fc.ctl, SI(scb->fc.ctl)+1);
		bcopy((uchar*) &scb->fc, (uchar*) fc, sizeof (struct framecontrol));
		/* override the outgoing PE if needed */
		fc->txdesc = ilc_pe(ilc, txbpb, fc->txdesc);
		SETPRI(fc->ctl, txpri);
	} else {
		pcom = hton32(ilc->pcom);
		bcopy((uchar*)&pcom, h, sizeof (uint32));

		/* set v1_signaled if we're the node signaling */
		if (ilc->pcom == PCOM_V2D)
			ilc->v1_signaled = 60;
	}

	/* record that we have sent a non-link multicast this interval */
	if (ETHER_ISMULTI(eh->ether_dhost) && (datatype == DT_ILINE) && !ISLICF(eh))
		scb->mc_sd0 = 1;

#ifdef IL_STATS
	/* update the stat counters and log */
	if (datatype == DT_TUT)
		il_stats_update_xmt(&ilc->stats, scb->stats, il_ms(),
			eh->ether_shost, eh->ether_dhost, 0, 0,
			len, ntoh16(eh->ether_type),
			((ilcp_t8hdr_t *)(eh + 1))->subtype8);
	else
		il_stats_update_xmt(&ilc->stats, scb->stats, il_ms(),
			eh->ether_shost, eh->ether_dhost,
			(uint) fc->txdesc+1, (uint) txpri,
			len, ntoh16(eh->ether_type),
			((ilcp_t8hdr_t *)(eh + 1))->subtype8);
#endif	/* IL_STATS */

	IL_PRHDRS(ilc, "tx", fc, h, NULL, il_plen(p));
	IL_PRPKT("txpkt", h, il_plen(p));

	return (datatype);
}

void
ilc_recv(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, void *p)
{
	struct ether_header *eh;
	uchar *origva;
	struct scb *scb;
	uint datatype;
	uint hpnatype;
	uint chestidx;
	uint timestamp;
	uint txc, pri;
	uint herr, pcrc;
	uint origlen;
	struct framecontrol *fc;
	void *tmp;
	uchar *tmpva;
	uint len;
#ifdef DBG
	char sa[32];
#endif

	origva = il_ppull(ilc, p, 0);
	origlen = il_plen(p);

	/* fixup rx header endianness */
	ltoh16_buf((void*)rxh, RXHDR_4MBAUD_LEN);

	IL_PRPKT("rxpkt", origva, origlen);

	/* have to special case tiny frames */
	if (origlen < ETHER_HDR_LEN) {
		COUNTER(ilc, IL_STATS_RCV_RUNT_COUNT)++;
		goto toss;
	}

	datatype = DATATYPE(rxh);
	chestidx = IL_STATS_INVALID_CH_EST_INDEX;
	hpnatype = HPNAV2;
	pcrc = 0;
	scb = NULL;

	if (datatype == DT_ILINE)
	{
		/* pick out the pri and txc values */
		fc = (struct framecontrol*) origva;
		pri = PRI(fc->ctl);
		txc = fc->txdesc;

		IL_PRHDRS(ilc, "rx", fc, NULL, (il_msg_level & 16)? rxh: NULL, origlen);

		/* strip off the iline header */
		eh = (struct ether_header*) il_ppull((void*)ilc, p, ILINE_HDR_LEN);
		len = origlen - ILINE_HDR_LEN;

		pcrc = PCRC(rxh);

		/* strip off crc32 and crc16 */
		len -= (ETHER_CRC_LEN + ILINE_CRC16_LEN);
		il_psetlen(p, len);
	}
	else
	{
		IL_ERROR(("il%d: unsupported datatype %d\n", ilc->unit, datatype));
#ifdef DBG
		if (il_msg_level & 1)
			ilc_dumphdrs(ilc, "unsupported datatype", NULL, NULL, rxh, 0);
#endif /* DBG */
		goto toss;
	}

	/* check for corrupted iline/tut headers */
	herr = ilc_check_hdr(ilc, rxh, datatype, pcrc, origlen);

#ifdef DBG
	/* pretty print the rxh if any of the error bits are set */
	if (((pcrc || herr) && (origlen >= ETHER_MIN_LEN))
		|| RTERR(rxh)
		|| ((datatype == DT_ILINE) && HCRC(rxh)))
		if (il_msg_level & 1)
			ilc_dumphdrs(ilc, "recv error", NULL, NULL, rxh, 0);
#endif

	timestamp = il_ms();

	/* one-shot capture channel estimate must preceed any error checking */
	if (ilc->cheststate == CH_ON)
		chestidx = ilc_capture_chest(ilc, timestamp);

	/* lookup the station control block */
	if (!herr) {
		scb = ilc_scblookup(ilc, eh->ether_shost);
#ifdef IL_BRIDGE
		if (!ETHER_ISMULTI(eh->ether_dhost))
			ilc_refidset(ilc, scb, eh->ether_dhost);
#endif
	}

#ifdef IL_STATS
	il_stats_update_rcv(&ilc->stats, scb? scb->stats : NULL,
		timestamp, eh->ether_shost, eh->ether_dhost, txc+1,
		pri, origlen, ntoh16(eh->ether_type),
		((ilcp_t8hdr_t *)(eh + 1))->subtype8, rxh, chestidx);
#endif

	/* map priority */
	COUNTER(ilc, IL_STATS_RCV_DFPQ_PRI + pri)++;
	pri = il_null_rxpri[pri];
	if (ilc->larqlevel == 0)
		COUNTER(ilc, IL_STATS_RCV_LL_PRI + pri)++;

	/* if header error, discard and return */
	if (herr)
		goto toss;

	ASSERT(scb);

	/* remember the remote station type */
	scb->hpnatype = hpnatype;

	/* short frames with payload errors are considered collision fragments */

	/* print warning on frames with payload crc errors */
	if (pcrc && (origlen >= ETHER_MIN_LEN)) {
		IL_ERROR(("il%d: bad crc from %s\n", ilc->unit,
			ether_ntoa((struct ether_addr*)eh->ether_shost, sa)));
	}

	/* per-frame constel-select part of rate negotiation */
	ilc_rateupd(ilc, rxh, eh, txc, scb, pcrc);

	/* do software link integrity */
	if (ilc->linkint)
		ilc_linkupd(ilc, eh);


	/* toss (frames with payload crc errors) except larq frames */
	if (pcrc && !ISLARQ(eh))
		goto toss;

	/* if we are in "promiscuous type" mode, send up all ilcp frames */
	if (ilc->promisctype && (ntoh16(eh->ether_type) == ETHERTYPE_ILCP) && !pcrc) {
		if ((tmp = il_pget((void*)ilc, FALSE, len, &tmpva, NULL))) {
			bcopy((uchar*)eh, tmpva, len);
			il_sendup((void*)ilc, tmp, pri, 0);
		}
	}

	ilc_dispatch(ilc, p, pri, pcrc);
	return;

toss:
	il_pfree((void*)ilc, p, FALSE);
}

/* called by ilc_recv() and larq */
void
ilc_dispatch(ilc_info_t *ilc, void *p, uint pri, uint pcrc)
{
	struct ether_header *eh, *neh;
	uint hlen;
	uint subtype;
	ilcp_t8hdr_t *ilcp;
	uint buf[256/sizeof(uint)];	/* word-aligned buffer */

	IL_TRACE(("il%d: ilc_dispatch: len %d\n", ilc->unit, il_plen(p)));

	eh = (struct ether_header*) il_ppull((void*)ilc, p, 0);

	/* loop stripping and processing ILCP headers */
	while (ntoh16(eh->ether_type) == ETHERTYPE_ILCP) {

		ilcp = (ilcp_t8hdr_t*) &eh[1];

		/* extract subtype and ilcp header length */
		if (ilcp->subtype8 >= 128) {
			subtype = ntoh16(((ilcp_t16hdr_t*)ilcp)->subtype16);
			hlen = ntoh16(((ilcp_t16hdr_t*)ilcp)->length) +
				sizeof (ilcp_t16hdr_t);
		} else {
			subtype = ilcp->subtype8;
			hlen = ilcp->length + sizeof (ilcp_t8hdr_t);
		}

		/* toss frames with invalid hlen */
		if (hlen > (il_plen(p) - ETHER_HDR_LEN))
			goto badhlen;

#ifdef IL_CERT /* XXX kludge for large cert header */
		if ((subtype == ILCP_SUBTYPE_CERT) && !pcrc) {
			ilc_recvcert(ilc, eh);
			il_pfree((void*)ilc, p, FALSE);
			return;
		}
#endif

		/* toss frames if hlen too big or odd value */
		if ((hlen > sizeof (buf)) || (hlen & 1))
			goto badhlen;

		/* copy ilcp header to local variable before clobbering */
		bcopy((char*)&eh[1], (uchar*)buf, hlen);

		/* remove ilcp header */
		neh = (struct ether_header*) il_ppull((void*)ilc, p, hlen);
		bcopy(eh->ether_shost, neh->ether_shost, ETHER_ADDR_LEN);
		bcopy(eh->ether_dhost, neh->ether_dhost, ETHER_ADDR_LEN);
		eh = neh;

	}

	/* discard frames with null terminating ethertype */
	if (eh->ether_type == 0) {
		il_pfree((void*)ilc, p, FALSE);
		return;
	}

	/* send up */
	il_sendup((void*)ilc, p, pri, pcrc);
	return;

badhlen:
	IL_ERROR(("il%d: ilc_dispatch: bad ilcp hlen %d\n", ilc->unit, hlen));
	COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
	il_pfree((void*)ilc, p, FALSE);
}

/*
 * Allocate a packet buffer, read the rxhdr and frame data into it, and return it.
 */
void*
ilc_recvpio(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uchar *va;
	void *p;
	uint16 *va16;
	uint16 w;
	int i, len;

	regs = ilc->regs;

	ASSERT(R_REG(&regs->rcvfifocontrol) & RFC_FR);

	/* clear frame ready */
	W_REG(&regs->rcvfifocontrol, RFC_FR);

	/* : 4210 may take several writes of FR */
	i = 16;
	while ((R_REG(&regs->rcvfifocontrol) & RFC_FR) && i--) {
		W_REG(&regs->rcvfifocontrol, RFC_FR);
		il_delay(1);
	}

	if (R_REG(&regs->rcvfifocontrol) & RFC_FR) {
		IL_ERROR(("il%d: ilc_recvpio: frame ready never cleared\n", ilc->unit));
		COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
		return (0);
	}

	/* wait for data valid */
	SPINWAIT(((R_REG(&regs->rcvfifocontrol) & RFC_DR) == 0), 100);
	if ((R_REG(&regs->rcvfifocontrol) & RFC_DR) == 0) {
		IL_ERROR(("il%d: ilc_recvpio: data ready never set\n", ilc->unit));
		COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
		return (0);
	}

	/* read the first 16bits of the rxhdr which gives the length */
	w = R_REG(&regs->rcvfifodata);
	len = ltoh16(w);

	/* flush giant frames */
	if (len > (RXBUFSZ - HWRXOFF)) {
		IL_ERROR(("il%d: giant packet\n", ilc->unit));
		COUNTER(ilc, IL_STATS_RCV_GIANT_COUNT)++;
		goto flush;
	}

	/* alloc a new packet buffer */
	if ((p = il_pget(ilc, FALSE, (HWRXOFF + len), &va, NULL)) == NULL) {
		IL_ERROR(("il%d: ilc_recvpio: out of rxbufs\n", ilc->unit));
		COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
		goto flush;
	}

	/* read the rxheader from the fifo first */
	va16 = (uint16*) va;
	*va16++ = w;
	i = (ilc->rxhdrlen / 2) - 1;
	while (i--) {
		*va16++ = R_REG(&regs->rcvfifodata);
	}

	/* read the frame */
	va16 = (uint16*) &va[HWRXOFF];
	i = len / 2;
	while (i--) {
		*va16++ = R_REG(&regs->rcvfifodata);
	}

	/* read last (odd) byte */
	if (len & 1) {
		*va16 = R_REG(&regs->rcvfifodata) & 0xff;
	}

	/*  - workaround for 4211 and subsequent chips */
	if (!IS4210(ilc->deviceid))
		W_REG(&regs->rcvfifocontrol, RFC_DR);

	/* silently discard runt frames */
	if (len < (ILINE_HDR_LEN + ETHER_HDR_LEN)) {
		COUNTER(ilc, IL_STATS_RCV_RUNT_COUNT)++;
		il_pfree(ilc, p, FALSE);
		p = NULL;
	}

	return (p);

flush:
	/* flush rx frame */
	/*  - don't set DR to flush on 4210, bang rxfifo instead */
	if (IS4210(ilc->deviceid)) {
		AND_REG(&regs->devcontrol, ~DC_RE);
		OR_REG(&regs->devcontrol, DC_RE);
	}
	else
		W_REG(&regs->rcvfifocontrol, RFC_DR);

	return (NULL);
}

/* common pio tx routine */
void
ilc_sendpio(ilc_info_t *ilc, uchar *va, int len, uint datatype)
{
	uint16 ctrl;

	/* frameready should be set */
	ASSERT(R_REG(&ilc->regs->xmtfifocontrol) & XFC_FR);

	/* clear frameready */
	W_REG(&ilc->regs->xmtfifocontrol, XFC_FR);

	/* poke the bytes into the xmtfifo */
	ilc_xmtfifo(ilc, va, len);

	/* set end-of-frame */
	ctrl = XFC_EF;
	if (datatype == DT_TUT)
		ctrl |= XFC_H1;

	/* start transmit */
	W_REG(&ilc->regs->xmtfifocontrol, ctrl);
}

/* low-level routine to stuff bytes into the xmtfifo */
void
ilc_xmtfifo(ilc_info_t *ilc, uchar *va, int len)
{
	bcm42xxregs_t *regs;
	uint16 *va16;
	int i;

	regs = ilc->regs;

	/* write data bytes two-by-two */
	W_REG(&regs->xmtfifocontrol, XFC_BOTH);
	va16 = (uint16 *)va;
	i = len / 2;
	while (i--) {
		W_REG(&regs->xmtfifodata, *va16++);
	}

	/* write last odd byte */
	if (len & 1) {
		/* Put byte in correct data lane, this depends on the MSI byte swap bit */
		if (ilc->bigendian)
			W_REG(&regs->xmtfifocontrol, XFC_HI);
		else
			W_REG(&regs->xmtfifocontrol, XFC_LO);
		W_REG(&regs->xmtfifodata, *(uint8*)va16);
	}
}

#ifdef IL_STATS
static uint
ilc_capture_chest(ilc_info_t *ilc, uint32 timestamp)
{
	uint i;
	channel_est_item_t *chi;
	uint32 chestidx;
	bcm42xxregs_t *regs;

	regs = ilc->regs;

	chestidx = ilc->chestidx;
	chi = CHANNEL_EST_PTR(ilc->stats.ch_est, chestidx);
	ilc->chestidx = (chestidx + 1) % IL_STATS_NUM_CH_EST;

	/* timestamp must match that of packet */
	chi->timestamp = timestamp;

	/* data's ready...read channel estimate */
	for (i = 0; i < 64; i++) {
		W_REG(&regs->fedbgaddr, (uint16)(FEDA_DS_CHEST + (2*i)));
		chi->vals[i].i = (uint16) R_REG(&regs->fedbgdata);
		W_REG(&regs->fedbgaddr, (uint16)(FEDA_DS_CHEST + (2*i) + 1));
		chi->vals[i].q = (uint16) R_REG(&regs->fedbgdata);
	}

	/* done - request re-initialization */
	ilc->cheststate = CH_TRIGGERED;

	return (chestidx);
}

void
ilc_trigger_channel_estimate(ilc_info_t *ilc)
{
	ilc->cheststate = CH_ON;
	ilc->promisc = TRUE;
	il_init((void*)ilc);
}
#endif /* IL_STATS */


/* common watchdog code */
void
ilc_watchdog(ilc_info_t *ilc)
{
	uint new;
	bcm42xxregs_t *regs;
	bool sawtut;
	uint newmacmode;

	if (!ilc->up)
		return;

	regs = ilc->regs;
	sawtut = FALSE;

#ifdef IL_STATS
	/* read chip counters */
	ilc_chipctrupd(ilc);
#endif

	ilc->now++;

	/* have any tut frames been seen? */
	if ((new = R_REG(&regs->ctr1linkrcvd)) != ilc->ctr1linkrcvd) {
		ilc->ctr1linkrcvd = new;
		sawtut = TRUE;
	}

	/* protocol periodic code */
	ilc_linkwd(ilc, sawtut);
	ilc_ratewd(ilc, sawtut);
	ilc_csawd(ilc);

	/* the one place where we decide to change the macmode */
	newmacmode = ilc_macmode(ilc);
	if (newmacmode != ilc->macmode)
		il_init((void*)ilc);
}

/*
 * Lookup station control block corresponding to the remote id.
 * If found, increment scrambler initialization field before returning.
 * If not found, create a new entry.
 */
struct scb*
ilc_scblookup(ilc_info_t *ilc, uchar *ea)
{
	int index;
	int i;
	struct scb *scb;
#ifdef IL_STATS
	phy_stats_path_info_t *savestats;
#endif

	index = SCBHASHINDEX(ea);

	/* search for the scb which corresponds to the remote station ea */
	for (scb = ilc->scbhash[index]; scb; scb = scb->next)
		if (bcmp(ea, &scb->ea, ETHER_ADDR_LEN) == 0)
			break;
	if (scb) {
		scb->used = ilc->now;
		goto done;
	}

	/* allocate a station control block */
	for (i = 0; i < NSCB; i++)
		if (ilc->scb[i].inuse == FALSE)
			break;
	if (i < NSCB) {
#ifdef IL_STATS
		/* update per-path stats count */
		ilc->stats.path_block->num = i + 1;
#endif
		scb = &ilc->scb[i];
	} else
		scb = ilc_scbreclaim(ilc);

	/* initialize it */
#ifdef IL_STATS
	savestats = scb->stats;
#endif
	bzero(scb, sizeof (struct scb));
	scb->hpnatype = HPNAUNKNOWN;
	scb->ea = *((struct ether_addr*)ea);
	if ((ilc->macmode == MODE_COMPAT) && (!ilc->xmt_gapped)) {
		scb->fc.txdesc = TXDESC_TUT;	/* 1M8 */
		scb->rx_desc = TXDESC_TUT;	/* 1M8 */
	} else {
		scb->fc.txdesc = 1;	/* 2bpb */
		scb->rx_desc = 1;	/* 2bpb */
	}
	scb->used = ilc->now;
	scb->inuse = TRUE;
	scb->bebl = 1;
	scb->expirek = 1;
#ifdef IL_STATS
	scb->stats = savestats;
	bzero(scb->stats, sizeof (phy_stats_path_info_t));
	bcopy(ea, scb->stats->id.octet, ETHER_ADDR_LEN);
#endif


	/* install it in the cache */
	scb->next = ilc->scbhash[index];
	ilc->scbhash[index] = scb;

done:
	return (scb);
}

static struct scb*
ilc_scbreclaim(ilc_info_t *ilc)
{
	int i;
	uint oldest;
	uint now;
	uint age;
	uint oldindex;
	int index;
	struct scb *scb;
	struct scb **prevp;

	/* ASSERT(lockheld) */

	now = ilc->now;
	oldest = 0;
	oldindex = 0;

	/* find the oldest inuse entry */
	for (i = 0; i < NSCB; i++)
		if (ilc->scb[i].inuse)
			if ((age = (now - ilc->scb[i].used)) > oldest) {
				oldest = age;
				oldindex = i;
			}

	index = SCBHASHINDEX(ilc->scb[oldindex].ea.octet);

	/* delete it from the hash */
	prevp = &ilc->scbhash[index];
	for (scb = ilc->scbhash[index]; scb; scb = scb->next) {
		if (scb == &ilc->scb[oldindex])
			break;
		prevp = &(*prevp)->next;
	}
	ASSERT(scb != NULL);
	*prevp = scb->next;

	/* invalidate mcast info in other scb's that refer to this scb */
	if (!ETHER_ISMULTI(&(scb->ea))) {
		for (i = 0; i < NSCB; i++) {
			ilc->scb[i].mc_hist[oldindex] = 0;
		}
	}

#ifdef IL_BRIDGE
	ilc_refidpurge(ilc, scb);
#endif

	return (scb);
}

#ifdef IL_BRIDGE
/* Free up mac addresses associated with an SCB */
void
ilc_refidpurge(ilc_info_t *ilc, struct scb *scb)
{
	int i;

	for (i = 0; i < NREFID; i++) {
		if (ilc->refid[i].scb == scb)
			ilc->refid[i].scb = NULL;
	}
}

/* Add a mac address for a given scb */
void
ilc_refidset(ilc_info_t *ilc, struct scb *scb, uchar *ea)
{
	int i;
	struct refid *r, *roldest;

	roldest = r = NULL;

	/* Find a MAC address structure */
	for (i = 0; i < NREFID; i++) {

		/* If we found it, update the time and get out */
		if ((ilc->refid[i].scb == scb) &&
			((bcmp(ea, &ilc->refid[i].ea, ETHER_ADDR_LEN)) == 0)) {
			ilc->refid[i].used = ilc->now;
			return;
		}

		/* Keep entries at the beginning */
		if (r != NULL)
			continue;

		/* If there is a free mac address slot, grab it */
		if (ilc->refid[i].scb == NULL) {
			r = &ilc->refid[i];
		} else if ((roldest == NULL) ||
				(roldest->used > ilc->refid[i].used)) {
			roldest = &ilc->refid[i];
		}
	}

	if (r == NULL)
		r = roldest;

	ASSERT(r != NULL);
	r->ea = *((struct ether_addr*)ea);
	r->scb = scb;
	r->used = ilc->now;

	/* Clear binary back off so rate can be sent for the ref just added */
	scb->bebi = 0;
	scb->bebl = 1;
	return;
}
#endif /* IL_BRIDGE */

/* send a link integrity control frame */
static void
ilc_sendlink(ilc_info_t *ilc)
{
	void *p;
	struct ether_header *eh;
	link_integ_t *link;
	uint size;

	IL_TRACE(("il%d: ilc_sendlink\n", ilc->unit));

	size = sizeof (struct ether_header) + sizeof (link_integ_t);

	if ((p = il_pget((void*)ilc, TRUE, size, (uchar**) &eh, NULL)) == NULL) {
		IL_ERROR(("il%d: ilc_sendlink: out of packets\n", ilc->unit));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
		return;
	}

	/* fill in ether_header */
	EHINIT(eh, &ilc->cur_etheraddr, &ether_bcast, ETHERTYPE_ILCP);

	/* fill in link integrity frame */
	link = (link_integ_t *) &eh[1];
	/* make sure all pad and reserved bits are zero */
	bzero(link, sizeof(link_integ_t));
	link->h.subtype8 = ILCP_SUBTYPE_LINK;
	link->h.length = LI_MIN_LENGTH;
	link->li_version = LI_VERSION;
	link->li_pad = 0;
	link->next_ethertype = 0;

	COUNTER(ilc, IL_STATS_LINKINT_COUNT)++;

	/* send LI frame with pri=7, negotiated bpb */
	il_sendctl((void*)ilc, p, 7, -1);
}

static void
ilc_linkupd(ilc_info_t *ilc, struct ether_header *eh)
{
	if (ilc->linkstate == LI_DOWN) {
		il_link_up((void*)ilc);
		W_REG(&ilc->regs->festatus, FES_LEDON);
	}

	/*
	 * If the current state is UP1,
	 * the mode is native iline,
	 * the received frame is a broadcast LICF,
	 * the received SA is different from the stored one,
	 * and the stored SA is not "invalid" (set to broadcast), then go to UP2.
	 */
	if (((ilc->linkstate == LI_UP_1) && (ilc->macmode == MODE_ILINE))
		&& (ETHER_ISBCAST(eh->ether_dhost) && ISLICF(eh))
		&& (bcmp(eh->ether_shost, (uchar*) &ilc->srcid1, ETHER_ADDR_LEN) != 0)
		&& !(ETHER_ISBCAST(&ilc->srcid1)))
		ilc->linkstate = LI_UP_2;

	/* if the state is UP2, just stay there, otherwise go to UP1 */
	else if (ilc->linkstate != LI_UP_2) {
		ilc->linkstate = LI_UP_1;

		/* if frame was a broadcast LICF, save the source address */
		if (ETHER_ISBCAST(eh->ether_dhost) && ISLICF(eh))
			bcopy(eh->ether_shost, (uchar*) &ilc->srcid1, ETHER_ADDR_LEN);
	}
}

static void
ilc_linkwd(ilc_info_t *ilc, bool sawtut)
{
	uint new;

	/*  link integrity non-functional in the 4210 and 4211 */
	if (ilc->linkint) {
		/* invalidate/reset srcid1 */
		ilc->srcid1 = ether_bcast;

		/* send link integrity packet */
		if ((--ilc->li_force_ctr == 0) || (ilc->linkstate != LI_UP_2)) {
			ilc_sendlink(ilc);
			ilc->li_force_ctr = (ilc->perm_etheraddr.octet[5] & LI_FORCE_MASK)
				+ LI_FORCE_MIN;
		}

		/* decrement our link integrity state */
		if (ilc->linkstate == LI_UP_2)
			new = LI_UP_0;
		else
			new = MAX(0, ((int)ilc->linkstate - 1));

		if ((new == LI_DOWN) && (ilc->linkstate != LI_DOWN)) {
			il_link_down((void*)ilc);
			W_REG(&ilc->regs->festatus, FES_LEDOFF);
		}
		ilc->linkstate = new;

		/* workaround for AMD runt tut link integrity frames */
		if (sawtut) {
			if (ilc->linkstate == LI_DOWN) {
				il_link_up((void*)ilc);
				W_REG(&ilc->regs->festatus, FES_LEDON);
			}
			ilc->linkstate = LI_UP_1;
		}
	}
}

static uint
ilc_macmode(ilc_info_t *ilc)
{
	/* configmode always takes precedence in setting macmode */
	if ((ilc->configmode >= MODE_TUT) && (ilc->configmode <= MODE_COMPAT))
		return (ilc->configmode);

	if (ilc->v1_detected || ilc->v1_signaled)
		return (MODE_COMPAT);

	return (MODE_ILINE);
}

static uint
ilc_txbpb(ilc_info_t *ilc, int txbpb)
{
	IL_TRACE(("il%d: ilc_txbpb: %d\n", ilc->unit, txbpb));

	if (((txbpb >= 0) && (txbpb < ILINE_MIN_BPB)) || (txbpb > ILINE_MAX_BPB))
		return (1);

	ilc->txbpb = txbpb;

	return (0);
}

static uint
ilc_txpri(ilc_info_t *ilc, int txpri)
{
	IL_TRACE(("il%d: ilc_txpri: %d\n", ilc->unit, txpri));

	if (((txpri >= 0) && (txpri < ILINE_MIN_PRI)) || (txpri > ILINE_MAX_PRI))
		return (1);

	ilc->txpri = txpri;

	return (0);
}

/* stubs */
void ilc_csaup(ilc_info_t *ilc) {}
void ilc_csadown(ilc_info_t *ilc) {}
static void ilc_rateupd(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, struct ether_header *eh,
	uint tx_desc, struct scb *scb, uint pcrc) {}
static void ilc_modepurge(ilc_info_t *ilc, uint newmacmode) {}
static void ilc_ratewd(ilc_info_t *ilc, bool sawtut) {}
static void ilc_csawd(ilc_info_t *ilc) {}

#ifdef IL_STATS
uint
ilc_stats_allocinit(ilc_info_t *ilc, char *driver_vers)
{
	stats_sizes_t *sizes;
	int i;

	/* set stats sizes */
	sizes = &ilc->stats.sizes;
	sizes->num_counters = IL_STATS_NCOUNTERS;
	sizes->num_xmt_hist = 64;
	sizes->num_rcv_hist = 64;
	sizes->num_ch_est = IL_STATS_NUM_CH_EST;
	sizes->num_ch_est_val = 64;
	sizes->num_paths = NSCB;
	sizes->num_path_counters = 1;
	sizes->len_err_queue = 0;
	sizes->len_counter_names = sizeof (il_counter_names);
	sizes->len_path_counter_names = sizeof (il_path_counter_names);

	/* allocate the stat memory */
	ilc_stats_size = STATS_MEM_SIZE(sizes);
	if ((ilc->stats.phy_if = (phy_if_t*) il_malloc(ilc_stats_size)) == NULL) {
		IL_ERROR(("il%d: ilc_stats_allocinit: memory allocation error\n", ilc->unit));
		return (1);
	}

	/* init all the pointers and offsets */
	il_stats_init(&ilc->stats, ilc->stats.phy_if, ilc_stats_size,
		il_counter_names, il_path_counter_names);

	/* point each station control block at a path_info structure */
	for (i = 0; i < NSCB; i++)
		ilc->scb[i].stats = &(ilc->stats.path_block->path_info[i]);

	/* fill in driver and board versions */
	bcopy(driver_vers, ilc->stats.phy_if->driver_vers, 32);
	sprintf(ilc->stats.phy_if->board_vers, "%d.%d", ilc->bdmajrev, ilc->bdminrev);
	sprintf(ilc->stats.phy_if->subsystem_vendor, "%d", ilc->subvendor);
	sprintf(ilc->stats.phy_if->subsystem_id, "%d", ilc->subid);

	return (0);
}
#endif	/* IL_STATS */

/*
 * Reset the debug register overwrite string.
 * It will be used in the next ilc_up().
 */
int
ilc_set_debug(ilc_info_t *ilc, char *s, uint len)
{
	if (len < MAXDEBUGSTR) {
		bcopy(s, ilc->debug, len);
		return (0);
	}
	return (1);
}

/*
 * Parse and apply registry debug register overwrite string.
 * Format:  "<addr>=<val>[,<addr>=<val>]*" where <addr> and <val>
 * are strings of hex digits [0-9a-f] that do not include a "0x" prefix.
 * If <addr> begins with the string "dbg", then the address and value
 * are taken to refer to the "snoop bus" -- fedbgaddr/fedbgdata .
 */
void
ilc_debug(ilc_info_t *ilc)
{
	uint addr, val;
	char *p1, *p2;
	uint dbg;
	uint tut;
	uint nibbles;
	bcm42xxregs_t *regs;

	regs = ilc->regs;

	p1 = ilc->debug;

	while (p1 && *p1) {

		addr = 0;
		val = 0;
		dbg = 0;
		tut = 0;
		nibbles = 0;

		/* snoop bus addresses have a leading "dbg" string */
		if (bcmp(p1, "dbg", 3) == 0) {
			dbg = 1;
			p1 += 3;
		}

		/* tut register access have a leading "tut" string */
		if (bcmp(p1, "tut", 3) == 0) {
			tut = 1;
			p1 += 3;
		}

		/* extract addr string */
		p2 = ilc_strchr(p1, '=');
		if (p2 == NULL) {
			IL_ERROR(("ilc_debug: no =\n"));
			goto cleanup;
		}
		for ( ; p1 < p2; p1++) {
			if ((*p1 >= '0') && (*p1 <= '9'))
				addr = (addr << 4) + (*p1 - '0');
			else if ((*p1 >= 'a') && (*p1 <= 'f'))
				addr = (addr << 4) + (*p1 - 'a' + 10);
			else {
				IL_ERROR(("ilc_debug: non hex char %c\n", *p1));
				goto cleanup;
			}
		}
		p1++;

		/* extract val string */
		p2 = ilc_strchr(p1, ',');
		if (p2 == NULL)
			p2 = p1 + 256;
		for ( ; *p1 && (p1 < p2); p1++) {
			nibbles++;
			if ((*p1 >= '0') && (*p1 <= '9'))
				val = (val << 4) + (*p1 - '0');
			else if ((*p1 >= 'a') && (*p1 <= 'f'))
				val = (val << 4) + (*p1 - 'a' + 10);
			else {
				IL_ERROR(("ilc_debug: non hex char %c\n", *p1));
				goto cleanup;
			}
		}

		if (!dbg && !tut && (addr > 0x32c)) {
			IL_ERROR(("ilc_debug: bogus addr 0x%x\n", addr));
			goto cleanup;
		}

		if (dbg) {	/* write to snoop bus */
			W_REG(&regs->fedbgaddr, (uint16) addr);
			W_REG(&regs->fedbgdata, (uint16) val);
		}
		else if (tut) {	/* write a tut register */
			W_REG(&regs->hpnaaddr, (uint16) addr);
			W_REG(&regs->hpnadata, (uint16) val);
		}
		else {
			if (nibbles == 8)
				W_REG((uint32*)((uint)regs + addr), val);
			else if (nibbles == 4)
				W_REG((uint16*)((uint)regs + addr),
					(uint16)(val & 0xffff));
			else
				IL_ERROR(("ilc_debug: val 0x%x:  %d nibbles:  ignored\n",
					val, nibbles));
		}

cleanup:
		p1 = ilc_strchr(p1, ',');
		if (p1)
			p1++;
	}
}

static uint
ilc_check_hdr(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, uint datatype, uint pcrc, uint len)
{
	uint herr;

	herr = 0;

	ASSERT((datatype == DT_TUT) || (datatype == DT_ILINE));

	switch (datatype) {
	case DT_TUT:
		/* check for runt frame */
		if (len < TUT_MIN_LEN) {
			COUNTER(ilc, IL_STATS_RCV_RUNT_COUNT)++;
			herr++;
		}
		if (pcrc)
			herr++;
		break;

	case DT_ILINE:
		if (FTERR(rxh)) {
			IL_ERROR(("il%d: receive frame type error\n", ilc->unit));
			herr++;
		}
		if (RTERR(rxh)) {
			IL_ERROR(("il%d: receive payload rate error\n", ilc->unit));
			herr++;
		}
		if (HCRC(rxh)) {
			IL_ERROR(("il%d: receive header crc8 error\n", ilc->unit));
			herr++;
		}

		/* check for runt frame */
		/*  - special case hardware link integrity frames */
		if ((len < ILINE_MIN_LEN) && !((len == 48) && !pcrc)) {
			COUNTER(ilc, IL_STATS_RCV_RUNT_COUNT)++;
			herr++;
		}
		break;
	}

	return (herr);
}


char*
ilc_strchr(const char *p, int ch)
{
	for (;; ++p) {
		if (*p == ch)
			return((char *)p);
		if (!*p)
			return((char *)NULL);
	}
}

static void
ilc_wtutreg(ilc_info_t *ilc, uint16 addr, uint16 v)
{
	W_REG(&ilc->regs->hpnaaddr, addr);
	W_REG(&ilc->regs->hpnadata, v);
}

static uint16
ilc_rtutreg(ilc_info_t *ilc, uint16 addr)
{
	W_REG(&ilc->regs->hpnaaddr, addr);
	return (R_REG(&ilc->regs->hpnadata));
}

#ifndef NOSPROM
void
ilc_spromread(ilc_info_t *ilc, uint16 *buf, uint byteoff, uint nbyte)
{
	int off, nw;
	int i;

	off = byteoff / 2;
	nw = ROUNDUP(nbyte, 2) / 2;

	for (i = 0; i < nw; i++)
		buf[i] = R_REG(&ilc->regs->sprom[off + i]);
}

/* return true if sprom present and has valid crc8, otherwise false */
bool
ilc_spromgood(ilc_info_t *ilc, uint byteoff, uint nbyte)
{
	uint16 sprom[64];
	uint8 chk8;
	int i;

	ASSERT(nbyte <= 128);

	if ((R_REG(&ilc->regs->devstatus) & DS_SP) == 0)
		return (FALSE);

	ilc_spromread(ilc, sprom, byteoff, nbyte);
	chk8 = crc8((uchar*)sprom, nbyte, CRC8_INIT_VALUE);

	if (chk8 == CRC8_GOOD_VALUE)
		return (TRUE);

	/* compatibility kludge for old sprom format */
	if ((chk8 != CRC8_GOOD_VALUE) && (byteoff == 0))
		for (i = 0x11; i < 0x20; i++)
			if (sprom[i] != 0)
				return (FALSE);

	return (TRUE);
}
#endif	/* !NOSPROM */

bool
ilc_chipid(uint16 vendor, uint16 device)
{
	if ((vendor == VENDOR_EPIGRAM) && IS4210(device))
		return (TRUE);
	if ((vendor == VENDOR_BROADCOM) && IS4211(device))
		return (TRUE);
	if ((vendor == VENDOR_BROADCOM) && IS4220(device))
		return (TRUE);
	return (FALSE);
}

#ifdef DMA
/* handy functions to enable and disable tx interrupts in DMA mode */
void
ilc_txint_on(ilc_info_t *ilc)
{
	if (PIOMODE(ilc))
		return;

	ilc->intmask |= I_XI;
	W_REG(&ilc->regs->intmask, ilc->intmask);
}

void
ilc_txint_off(ilc_info_t *ilc)
{
	if (PIOMODE(ilc))
		return;

	ilc->intmask &= ~I_XI;
	W_REG(&ilc->regs->intmask, ilc->intmask);
}
#endif

void
validate_chip_access(ilc_info_t *ilc)
{
	bcm42xxregs_t *regs;
	uint32 w;

	regs = ilc->regs;

	/* Validate dchip register access */

	/* Do vendorid and deviceid (i.e. 16bit regs) make sense? */
	ASSERT((ilc->vendorid == VENDOR_EPIGRAM) || (ilc->vendorid == VENDOR_BROADCOM));
	ASSERT(IS4210(ilc->deviceid) || IS4211(ilc->deviceid) || IS4220(ilc->deviceid));

	/* Can we write and read back a 32bit register? */
	w = R_REG(&regs->wakeuplength);

	W_REG(&regs->wakeuplength, (uint32)0xaa5555aa);
	ASSERT(R_REG(&regs->wakeuplength) == (uint32)0xaa5555aa);
	W_REG(&regs->wakeuplength, (uint32)0x55aaaa55);
	ASSERT(R_REG(&regs->wakeuplength) == (uint32)0x55aaaa55);

	W_REG(&regs->wakeuplength, w);
}
