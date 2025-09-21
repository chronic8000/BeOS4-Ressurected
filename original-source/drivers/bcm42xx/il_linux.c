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
 * Linux device driver for Broadcom BCM42xx
 * InsideLine Home Phoneline Networking Adapter.
 *
 * Copyright(c) 2000 Broadcom Corporation.
 * All Rights Reserved.
 * $Id: il_linux.c,v 13.43 2000/05/02 01:44:33 nn Exp $
 */

#include <linux/config.h>
#include <linux/version.h>
#ifdef MODULE
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#else
#define	MOD_INC_USE_COUNT
#define	MOD_DEC_USE_COUNT
#endif

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/malloc.h>
#include <linux/string.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <epivers.h>
#include <bcmendian.h>
#include <typedefs.h>
#include <proto/ethernet.h>
#include <proto/ilcp.h>
#include <bcm42xx.h>
#include <il_stats.h>
#include <ilsockio.h>
#include <ilc.h>

typedef struct il_info {
	ilc_info_t	ilc;		/* common os-independent data */
	struct device	*dev;		/* backpoint to device */
	spinlock_t	lock;		/* per-device perimeter lock */
	struct sk_buff_head txq;	/* send queue */
	struct timer_list timer;	/* one second watchdog timer */
	struct timer_list larqtimer;	/* larq timeout */
	bool		larqtimerset;	/* true if larq timer is running */
	struct net_device_stats stats;	/* derived on-the-fly from il_stats */
} il_info_t;

/* defines */
#define	IL_INFO(dev)	(il_info_t*)((dev)->priv)
#define	DATAHIWAT	50		/* data msg txq hiwat mark */
#define	CTLHIWAT	(DATAHIWAT+256)	/* ctl msg txq hiwat mark (+max larq) */
#define	MILLISEC_PER_TICK	(1000/HZ)

/* overload skb->ip_summed field to tag skb with local txbpb value */
#define	SKBTXBPB(skb)	((int8)(skb)->ip_summed)

/*
 * MSI-mode chip register physical address and irq need filling in..
 */
#ifdef __arm /* .. really empeg */
#define	MSI_BASEADDR	0		/* chip register physical address */
#define	MSI_IRQ		2		/* chip interrupt irq */
#else
#define	MSI_BASEADDR	-1		/* chip register physical address */
#define	MSI_IRQ		-1		/* chip interrupt irq */
#endif

static struct ether_addr ether_default = {{ 0, 1, 2, 3, 4, 0 }};

#ifdef ILSIM
#define	FLUSH(a, n, f) sm_flush(a, n, f)
extern void sm_flush(uchar *a, uint n, uint f);
#else
#define	FLUSH(a, n, f)
#endif

/* prototypes called by ilc.c */
void il_init(il_info_t *il);
void il_reset(il_info_t *il);
void il_up(il_info_t *il);
void il_down(il_info_t *il, int reset);
uint il_sendctl(il_info_t *il, struct sk_buff *skb, int txpri, int txbpb);
void il_txintr(il_info_t *il);
void il_rxintr(il_info_t *il);
void il_sendup(il_info_t *il, struct sk_buff *skb, uint pri, uint pcrc);
void il_txreclaim(il_info_t *il, bool forceall);
void il_rxreclaim(il_info_t *il);
void *il_pget(il_info_t *il, bool send, uint len, uchar **va, uchar **pa);
void il_pfree(il_info_t *il, struct sk_buff *skb, bool send);
uchar* il_ppush(il_info_t *il, struct sk_buff *skb, uint bytes);
uchar* il_ppull(il_info_t *il, struct sk_buff *skb, uint bytes);
uint il_plen(struct sk_buff *skb);
void il_psetlen(struct sk_buff *skb, uint len);
void *il_pdup(il_info_t *il, struct sk_buff *skb);
void il_delay(uint us);
void *il_malloc(uint size);
void il_mfree(void *addr, uint size);
uint il_ms(void);
void il_link_up(il_info_t *il);
void il_link_down(il_info_t *il);
int il_dump(il_info_t *il, uchar *buf, uint len);

/* local prototypes */
#ifdef MODULE
int init_module(void);
void cleanup_module(void);
#else
int il_probe(struct device *dev);
#endif
static int il_scan(struct device *dev);
static int il_probe1(struct device *dev, ulong regpa, uint irq, uint unit);
static void il_free(il_info_t *il);
static int il_open(struct device *dev);
static int il_close(struct device *dev);
static int il_start(struct sk_buff *skb, struct device *dev);
static void il_sendnext(il_info_t *il);
static void il_send(il_info_t *il, struct sk_buff *skb, int txpri, int txbpb);
static struct net_device_stats *il_get_stats(struct device *dev);
static void il_set_multicast_list(struct device *dev);
static void il_watchdog(ulong data);
static int il_ioctl(struct device *dev, struct ifreq *ifr, int cmd);
static void il_intr(int irq, void *dev_id, struct pt_regs *ptregs);
static void il_recvpio(il_info_t *il);
#ifdef DMA
static void il_recvdma(il_info_t *il);
#endif
static void il_dumpil(il_info_t *il, uchar *buf, uint len);

/* externs */
extern ilc_info_t *ilc_list;

#ifdef MODULE

int
init_module(void)
{
	return (il_scan(0));
}

void
cleanup_module(void)
{
	il_info_t *il;
	uint flags;


	/* free everything */
	while ((il = (il_info_t*) ilc_list)) {
		spin_lock_irqsave(&il->lock, flags);
		il_down(il, 1);
		spin_unlock_irqrestore(&il->lock, flags);

		il_free(il);
	}
}

#else	/* !MODULE */

int
il_probe(struct device *dev)
{
	static int scanned = 0;
	if (scanned++)
		return (-ENODEV);
	return (il_scan(dev));
}
#endif	/* MODULE */

static int
il_scan(struct device *dev)
{
#ifdef CONFIG_PCI
	struct pci_dev *pdev;
#endif
	uint found;
	int error;

	found = 0;

#ifdef CONFIG_PCI
	/* if pci is config'd and present, scan for our chip */
	if (pci_present()) {
		pdev = NULL;
		while ((pdev = pci_find_class((PCI_CLASS_NETWORK_ETHERNET << 8), pdev))) {
			if (!ilc_chipid(pdev->vendor, pdev->device))
				continue;

			if ((error = il_probe1(dev, pdev->base_address[0], pdev->irq,
				found)))
				return (error);

			pci_set_master(pdev);

			found++;
		}
	}
	else {
#endif
		/*
		 * Assume this is an MSI (Microprocessor Slave Interface)
		 * hardware platform with the chip registers at MSI_BASEADDR.
		 */
		if ((MSI_BASEADDR == -1) || (MSI_IRQ == -1)) {
			IL_ERROR(("il_scan:  MSI_BASEADDR/MSI_IRQ not defined\n"));
			return (-ENODEV);
		}
		if ((error = il_probe1(dev, (ulong) MSI_BASEADDR, MSI_IRQ, found)))
			return (error);
		found++;

#ifdef CONFIG_PCI
	}
#endif

	return (found? 0: -ENODEV);
}

static int
il_probe1(struct device *dev, ulong regpa, uint irq, uint unit)
{
	il_info_t *il;
	ilc_info_t *ilc;

	IL_TRACE(("il_probe1: 0x%lx, %d, %d\n", regpa, irq, unit));

	if ((dev = init_etherdev(dev, 0)) == NULL) {
		IL_ERROR(("il%d: il_probe1: init_etherdev() failed\n", unit));
		return (-ENOMEM);
	}

	/* allocate private info */
	if ((il = (il_info_t*) kmalloc(sizeof (il_info_t), GFP_KERNEL)) == NULL) {
		IL_ERROR(("il%d: il_probe1: kmalloc failed\n", unit));
		unregister_netdev(dev);
		return (-ENOMEM);
	}
	bzero(il, sizeof (il_info_t));
	dev->priv = (void*) il;

	il->dev = dev;

	ilc = &il->ilc;
	ilc->unit = unit;

	printf("%s: BCM42xx iLine 10Mbps Home Phoneline Ethernet\n", dev->name);

	/* map chip registers */
#ifdef __arm
	ilc->regs = (bcm42xxregs_t *) regpa;

	/* setup io timings */
	MECR = (MECR & 0xffff0000) | 0x0008;

	/* setup irq on falling edge of gpio2 */
	GRER &= ~(GPIO_GPIO2);
	GFER |= ~(GPIO_GPIO2);
	GEDR = GPIO_GPIO2;
#else
	if ((ilc->regs = (bcm42xxregs_t *) ioremap(regpa, 4096)) == NULL) {
		IL_ERROR(("il%d: ioremap failed\n", ilc->unit));
		goto fail;
	}
#endif

	if (!ilc_chipid(R_REG(&ilc->regs->pcicfg[0]), R_REG(&ilc->regs->pcicfg[1]))) {
		IL_ERROR(("il%d: cannot access bcm42xx chip\n", ilc->unit));
		goto fail;
	}

	ilc->chiprev = R_REG(&ilc->regs->devstatus) & DS_RI_MASK;

	ilc_chipreset(ilc);

	spin_lock_init(&il->lock);
	skb_queue_head_init(&il->txq);

	/* init 1 second watchdog timer */
	init_timer(&il->timer);
	il->timer.data = (ulong)dev;
	il->timer.function = il_watchdog;


	/* register our interrupt handler */
	if (request_irq(irq, il_intr, SA_SHIRQ, dev->name, dev)) {
		IL_ERROR(("il%d: request_irq failed\n", ilc->unit));
		goto fail;
	}
	dev->irq = irq;

	/* are we in DMA or PIO mode? */
#ifdef DMA
	if (R_REG(&ilc->regs->devstatus) & DS_MM)
		ilc->piomode = TRUE;
#else
	ilc->piomode = TRUE;
#endif

#ifdef DMA
	/* allocate dma resources */
	if (!PIOMODE(ilc)) {
		/* allocate transmit descriptor ring */
		if ((ilc->txd = (bcm42xxdd_t*) get_free_page(GFP_ATOMIC)) == NULL)
			goto fail;
		ASSERT(IS_ALIGNED(ilc->txd, 4096));
		ilc->txdpa = (void*) virt_to_bus(ilc->txd);

		/* allocate receive descriptor ring */
		if ((ilc->rxd = (bcm42xxdd_t*) get_free_page(GFP_ATOMIC)) == NULL)
			goto fail;
		ASSERT(IS_ALIGNED(ilc->rxd, 4096));
		ilc->rxdpa = (void*) virt_to_bus(ilc->rxd);
	}
#endif

	/* set default software intmask - no tx interrupts in dma mode */
	ilc->intmask = PIOMODE(ilc)? (MSII_XI | MSII_RI | MSII_ERRORS): (I_RI | I_ERRORS);

	/* common load-time initialization */
	if (ilc_attach(ilc) < 0) {
		IL_ERROR(("il%d: ilc_attach failed\n", ilc->unit));
		goto fail;
	}

	/* init stats infrastructure */
	if (ilc_stats_allocinit(ilc, "bcm42xx")) {
		IL_ERROR(("il%d: il_attach: stats_allocinit failed\n", ilc->unit));
		goto fail;
	}

	/* if no sprom, set the local ether address */
	if (!(R_REG(&ilc->regs->devstatus) & DS_SP)) {
		bcopy(&ether_default, &ilc->perm_etheraddr, ETHER_ADDR_LEN);
		bcopy(&ilc->perm_etheraddr, &ilc->cur_etheraddr, ETHER_ADDR_LEN);
	}

	bcopy(&ilc->cur_etheraddr, dev->dev_addr, ETHER_ADDR_LEN);

	/* lastly, enable our entry points */
	dev->open = il_open;
	dev->stop = il_close;
	dev->hard_start_xmit = il_start;
	dev->get_stats = il_get_stats;
	dev->set_multicast_list = il_set_multicast_list;
	dev->do_ioctl = il_ioctl;

	return (0);

fail:
	il_free(il);
	return (-ENODEV);
}

static void
il_free(il_info_t *il)
{
	if (il == NULL)
		return;

	IL_TRACE(("il%d: il_free\n", il->ilc.unit));

#ifdef DMA
	if (il->ilc.txd)
		free_page((ulong)il->ilc.txd);
	if (il->ilc.rxd)
		free_page((ulong)il->ilc.rxd);
#endif

#ifndef __arm
	if (il->ilc.regs)
		iounmap((void*)il->ilc.regs);
#endif

	if (il->dev && il->dev->irq)
		free_irq(il->dev->irq, il->dev);

	if (il->dev) {
		unregister_netdev(il->dev);
		kfree(il->dev);
		il->dev = NULL;
	}

	/* can't deallocate common resources until after unregister_netdev().. */
	ilc_detach(&il->ilc);

	kfree(il);
}

static int
il_open(struct device *dev)
{
	il_info_t *il;
	uint flags;

	il = IL_INFO(dev);

	IL_TRACE(("il%d: il_open\n", il->ilc.unit));

	dev->interrupt = 0;

	il->ilc.promisc = (dev->flags & IFF_PROMISC)? TRUE: FALSE;

	spin_lock_irqsave(&il->lock, flags);
	il_up(il);
	spin_unlock_irqrestore(&il->lock, flags);

	MOD_INC_USE_COUNT;

	return (0);
}

static int
il_close(struct device *dev)
{
	il_info_t *il;
	uint flags;

	il = IL_INFO(dev);

	IL_TRACE(("il%d: il_close\n", il->ilc.unit));

	il->ilc.promisc = FALSE;

	spin_lock_irqsave(&il->lock, flags);
	il_down(il, 1);
	spin_unlock_irqrestore(&il->lock, flags);

	MOD_DEC_USE_COUNT;

	return (0);
}

/*
 * Yeah, queueing the packets on a tx queue instead of throwing them
 * directly into the descriptor ring in the case of dma is kinda lame,
 * but this results in a unified transmit path for both dma and pio
 * and localizes/simplifies the dev->tbusy semantics, too.
 */
static int
il_start(struct sk_buff *skb, struct device *dev)
{
	il_info_t *il;
	ulong flags;

	il = IL_INFO(dev);

	IL_TRACE(("il%d: il_start: len %d\n", il->ilc.unit, skb->len));

	/* normalize priority */
	if (skb->priority > ILINE_MAX_PRI)
		skb->priority = ILINE_MAX_PRI;

	/* use negotiated txbpb */
	SKBTXBPB(skb) = -1;

	spin_lock_irqsave(&il->lock, flags);

	/* put it on the tx queue and call sendnext */
	skb_queue_tail(&il->txq, skb);
	il_sendnext(il);

	spin_unlock_irqrestore(&il->lock, flags);

	return (0);
}

static void
il_sendnext(il_info_t *il)
{
	ilc_info_t *ilc;
	struct sk_buff *skb;

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_sendnext\n", ilc->unit));

	/* dequeue and send each packet */
	while (ilc->pioactive == NULL) {
#ifdef DMA
		/* if tx ring is full */
		if (!PIOMODE(ilc) && (NEXTTXD(ilc->txout) == (ilc->txin)))
			break;
#endif
		if ((skb = skb_dequeue(&il->txq)) == NULL)
			break;

		il_send(il, skb, skb->priority, SKBTXBPB(skb));
	}

#ifdef DMA
	if (!PIOMODE(ilc)) {
		/* reclaim any completed tx ring entries */
		il_txreclaim(il, FALSE);

		/* enable tx interrupts whenever txq is non-empty */
		if ((skb_queue_len(&il->txq) > 0) && ((ilc->intmask & I_XI) == 0))
			ilc_txint_on(ilc);
		else if ((ilc->intmask & I_XI) && (skb_queue_len(&il->txq) == 0))
			ilc_txint_off(ilc);
	}
#endif

	/* set tbusy whenever txq fills */
	if ((skb_queue_len(&il->txq) > DATAHIWAT) && !il->dev->tbusy)
		il->dev->tbusy = 1;
	else if (il->dev->tbusy && (skb_queue_len(&il->txq) < (DATAHIWAT/2))) {
		il->dev->tbusy = 0;
		mark_bh(NET_BH);
	}
}

/* hardware send routine */
static void
il_send(il_info_t *il, struct sk_buff *skb, int txpri, int txbpb)
{
	ilc_info_t *ilc;
	bcm42xxregs_t *regs;
	struct sk_buff *n;
	uint datatype;
#ifdef DMA
	uint32 ctrl;
	uint txout;
#endif

	ilc = &il->ilc;
	regs = ilc->regs;

	ASSERT(ilc->pioactive == NULL);

	IL_TRACE(("il%d: il_send: len %d txpri %d txbpb %d\n",
		ilc->unit, skb->len, txpri, txbpb));

	/* ensure sufficient headroom for larq and iline/tut headers */
	if (skb_headroom(skb) < TXOFF) {
		n = skb_realloc_headroom(skb, TXOFF);
		dev_kfree_skb(skb);
		if (n == NULL) {
			IL_ERROR(("il%d: il_send: skb_realloc_headroom failed\n",
				ilc->unit));
			COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
			return;
		}
		skb = n;
	}

	/* set pioactive before calling ilc_send() to avoid sendctl-recursion */
	if (PIOMODE(ilc))
		ilc->pioactive = (void*) skb;

	/* common tx code */
	datatype = ilc_send(ilc, (void*)skb, txpri, txbpb);

	if (PIOMODE(ilc)) {
		ASSERT(ilc->pioactive == (void*) skb);
		ilc_sendpio(ilc, skb->data, skb->len, datatype);
	}
#ifdef DMA
	else {
		FLUSH(skb->data, skb->len, 0);

		ASSERT(NEXTTXD(ilc->txout) != ilc->txin);

		txout = ilc->txout;

		/* build the descriptor control value */
		ctrl = CTRL_SOF | CTRL_EOF | CTRL_IOC | (skb->len & CTRL_BC_MASK);
		if (txout == (NTXD - 1))
			ctrl |= CTRL_EOT;
		if (datatype == DT_TUT)
			ctrl |= CTRL_COMPAT;

		/* init the tx descriptor */
		W_SM(&ilc->txd[txout].ctrl, ctrl);
		W_SM(&ilc->txd[txout].addr, (uint32)virt_to_bus(skb->data));

		/* save the skb */
		ASSERT(ilc->txp[txout] == NULL);
		ilc->txp[txout] = skb;

		/* bump the tx descriptor index */
		txout = NEXTTXD(txout);
		ilc->txout = txout;

		/* kick the chip */
		W_REG(&regs->xmtptr, I2B(txout));
	}
#endif
}

uint
il_sendctl(il_info_t *il, struct sk_buff *skb, int txpri, int txbpb)
{
	IL_TRACE(("il%d: il_sendctl\n", il->ilc.unit));

	/* discard if txq is too full */
	if (skb_queue_len(&il->txq) > CTLHIWAT) {
		IL_ERROR(("il%d: il_sendctl: txq overflow\n", il->ilc.unit));
		dev_kfree_skb(skb);
		COUNTER(&il->ilc, IL_STATS_XMT_ERR_COUNT)++;
		return(1);
	}

	skb->priority = txpri;
	SKBTXBPB(skb) = txbpb;

	skb_queue_tail(&il->txq, skb);

	/* don't call il_sendnext() here - it would cause larq recursion */
	return(0);
}

void
il_init(il_info_t *il)
{
	IL_TRACE(("il%d: il_init\n", il->ilc.unit));

	COUNTER(&il->ilc, IL_STATS_RESET_COUNT)++;

	il_reset(il);

	ilc_init(&il->ilc);
}


void
il_reset(il_info_t *il)
{
	IL_TRACE(("il%d: il_reset\n", il->ilc.unit));

	ilc_reset(&il->ilc);
}

void
il_up(il_info_t *il)
{
	ilc_info_t *ilc;

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_up\n", ilc->unit));

	ilc_up(ilc);

	/* schedule one second watchdog timer */
	il->timer.expires = jiffies + HZ;
	add_timer(&il->timer);

	il->dev->start = 1;
	il->dev->tbusy = 0;
}

void
il_down(il_info_t *il, int reset)
{
	ilc_info_t *ilc;
	struct sk_buff *skb;

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_down\n", ilc->unit));

	il->dev->start = 0;
	il->dev->tbusy = 1;

	/* stop watchdog timer */
	del_timer(&il->timer);

	ilc_down(ilc, reset);

	/* flush the txq */
	while ((skb = skb_dequeue(&il->txq)))
		dev_kfree_skb(skb);
}

static void
il_watchdog(ulong data)
{
	il_info_t *il;
	ulong flags;

	il = IL_INFO((struct device*)data);

	spin_lock_irqsave(&il->lock, flags);

	ilc_watchdog(&il->ilc);

	il_txreclaim(il, FALSE);

	/* run the tx queue */
	if (il->txq.next)
		il_sendnext(il);

	/* reschedule one second watchdog timer */
	il->timer.expires = jiffies + HZ;
	add_timer(&il->timer);

	spin_unlock_irqrestore(&il->lock, flags);
}

static int
il_ioctl(struct device *dev, struct ifreq *ifr, int cmd)
{
	il_info_t *il;
	ilc_info_t *ilc;
	int flags;
	int error;

	il = IL_INFO(dev);
	ilc = &il->ilc;

	IL_TRACE(("il%d: il_ioctl: cmd 0x%x\n", il->ilc.unit, cmd));

	spin_lock_irqsave(&il->lock, flags);
	error = ilc_ioctl(ilc, cmd - SIOCSILTXDOWN, ifr->ifr_data);
	if (error)
		error = -EINVAL;
	spin_unlock_irqrestore(&il->lock, flags);

	return (error);
}

static struct net_device_stats*
il_get_stats(struct device *dev)
{
	il_info_t *il;
	ilc_info_t *ilc;
	struct net_device_stats *stats;

	il = IL_INFO(dev);
	ilc = &il->ilc;

	IL_TRACE(("il%d: il_get_stats\n", il->ilc.unit));

	ASSERT(ilc->stats.phy_if);

	stats = &il->stats;
	bzero(stats, sizeof (struct net_device_stats));

	/* SWAG */
	stats->rx_packets = COUNTER(ilc, IL_STATS_RCV_FRAME_COUNT);
	stats->tx_packets = COUNTER(ilc, IL_STATS_XMT_FRAME_COUNT);
	stats->rx_bytes = COUNTER(ilc, IL_STATS_RCV_BYTE_COUNT);
	stats->tx_bytes = COUNTER(ilc, IL_STATS_XMT_BYTE_COUNT);
	stats->rx_errors = COUNTER(ilc, IL_STATS_RCV_ERR_COUNT);
	stats->tx_errors = COUNTER(ilc, IL_STATS_XMT_ERR_COUNT);
	stats->collisions = COUNTER(ilc, IL_STATS_COLL_COUNT);

	stats->rx_length_errors = COUNTER(ilc, IL_STATS_RCV_RUNT_COUNT);
	stats->rx_over_errors = COUNTER(ilc, IL_STATS_RD_UFLO_COUNT);
	stats->rx_crc_errors = COUNTER(ilc, IL_STATS_PAY_CRC_ERR_COUNT);
	stats->rx_frame_errors = COUNTER(ilc, IL_STATS_XMT_HDR_ERR_COUNT);
	stats->rx_fifo_errors = COUNTER(ilc, IL_STATS_RF_OFLO_COUNT);
	stats->rx_missed_errors = COUNTER(ilc, IL_STATS_RCV_MISSED_COUNT);

	stats->tx_fifo_errors = COUNTER(ilc, IL_STATS_XF_UFLO_COUNT);

	return (stats);
}

static void
il_set_multicast_list(struct device *dev)
{
	il_info_t *il;
	ilc_info_t *ilc;
	struct dev_mc_list *mclist;
	int i;

	il = IL_INFO(dev);
	ilc = &il->ilc;

	IL_TRACE(("il%d: il_set_multicast_list\n", ilc->unit));

	ilc->promisc = (dev->flags & IFF_PROMISC)? TRUE: FALSE;
	ilc->allmulti = (dev->flags & IFF_ALLMULTI)? TRUE: FALSE;

	/* copy the list of multicasts into our private table */
	for (i = 0, mclist = dev->mc_list; mclist && (i < dev->mc_count);
		i++, mclist = mclist->next) {
		if (i >= MAXMULTILIST) {
			ilc->allmulti = TRUE;
			i = 0;
			break;
		}
		ilc->multicast[i] = *((struct ether_addr*) mclist->dmi_addr);
	}
	ilc->nmulticast = i;

	il_init(il);
}

static void
il_intr(int irq, void *dev_id, struct pt_regs *ptregs)
{
	struct device *dev;
	il_info_t *il;
	ilc_info_t *ilc;
	uint32 intstatus;
	ulong flags;

	dev = (struct device*) dev_id;
	il = IL_INFO(dev);
	ilc = &il->ilc;

	dev->interrupt = 1;

	spin_lock_irqsave(&il->lock, flags);

	if (PIOMODE(ilc))
		intstatus = R_REG(&ilc->regs->msiintstatus);
#ifdef DMA
	else
		intstatus = R_REG(&ilc->regs->intstatus);
#endif

	/* detect cardbus removed */
	if (intstatus == 0xffffffff) {
		IL_ERROR(("il%d: il_intr: dead chip\n", ilc->unit));
	        il_down(il, 1);
		intstatus = 0;
	}

	intstatus &= ilc->intmask;

	if (intstatus == 0)
		goto done;

	IL_TRACE(("il%d: il_intr: intstatus 0x%x\n", ilc->unit, intstatus));

	/* clear the interrupt */
	if (PIOMODE(ilc))
		W_REG(&ilc->regs->msiintstatus, intstatus);
#ifdef DMA
	else
		W_REG(&ilc->regs->intstatus, intstatus);
#endif

	/* common interrupt code */
	ilc_intr(ilc, intstatus);

	/* run the tx queue */
	if (il->txq.next)
		il_sendnext(il);

done:
	spin_unlock_irqrestore(&il->lock, flags);
	dev->interrupt = 0;
}

void
il_txintr(il_info_t *il)
{
	IL_TRACE(("il%d: il_txintr\n", il->ilc.unit));

	il_txreclaim(il, FALSE);
}

void
il_rxintr(il_info_t *il)
{
	IL_TRACE(("il%d: il_rxintr\n", il->ilc.unit));

	if (PIOMODE(&il->ilc))
		il_recvpio(il);
#ifdef DMA
	else
		il_recvdma(il);
#endif
}

static void
il_recvpio(il_info_t *il)
{
	ilc_info_t *ilc;
	struct sk_buff *skb;
	bcm42xxrxhdr_t *rxh;

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_recvpio\n", ilc->unit));

	/* process all received packets */
	while (R_REG(&ilc->regs->rcvfifocontrol) & RFC_FR) {
		/* alloc new skb and copy rx frame into it */
		if ((skb = (struct sk_buff*) ilc_recvpio(ilc)) == NULL)
			continue;

		/* skb starts with rxhdr */
		rxh = (bcm42xxrxhdr_t*) skb->data;

		/* strip off rxhdr */
		skb_pull(skb, HWRXOFF);

		/* call common receive and dispatch */
		ilc_recv(ilc, rxh, (void*)skb);
	}
}

#ifdef DMA
static void
il_recvdma(il_info_t *il)
{
	ilc_info_t *ilc;
	struct sk_buff *skb;
	bcm42xxrxhdr_t *rxh;
	uint rxin, curr;
	uint len;
	uint skiplen;

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_recvdma\n", ilc->unit));

	rxin = ilc->rxin;
	curr = B2I(R_REG(&ilc->regs->rcvstatus) & RS_CD_MASK);
	skiplen = 0;

	/* process all received packets */
	for (rxin = ilc->rxin; rxin != curr; rxin = NEXTRXD(rxin)) {

		/* get the skb that corresponds to the rx descriptor */
		skb = (struct sk_buff*) ilc->rxp[rxin];
		ilc->rxp[rxin] = NULL;
		W_SM(&ilc->rxd[rxin].addr, 0);
		ASSERT(skb);

		/* skip giant packets which span multiple rx descriptors */
		if (skiplen > 0) {
			skiplen -= RXBUFSZ;
			if (skiplen < 0)
				skiplen = 0;
			dev_kfree_skb(skb);
			continue;
		}

		FLUSH(skb->data, RXBUFSZ, 1);

		rxh = (bcm42xxrxhdr_t*) skb->data;
		len = ltoh16(FRAMELEN(rxh));

		/* giant frame check */
		if (len > (RXBUFSZ - HWRXOFF)) {
			IL_ERROR(("il%d: giant packet\n", ilc->unit));
			COUNTER(ilc, IL_STATS_RCV_GIANT_COUNT)++;
			COUNTER(ilc, IL_STATS_RCV_ERR_COUNT)++;
			skiplen = len - (RXBUFSZ - HWRXOFF);
			dev_kfree_skb(skb);
			continue;
		}

		/* strip off rxhdr */
		skb_pull(skb, HWRXOFF);

		/* set actual length */
		skb_trim(skb, len);

		/* call common receive and dispatch */
		ilc_recv(ilc, rxh, (void*)skb);
	}

	ilc->rxin = rxin;

	/* post more rx buffers */
	ilc_fill(&il->ilc);
}
#endif /* DMA */

void
il_sendup(il_info_t *il, struct sk_buff *skb, uint pri, uint pcrc)
{
	IL_TRACE(("il%d: il_sendup: %d bytes\n", il->ilc.unit, skb->len));

	if (pcrc) {
		dev_kfree_skb(skb);
		return;
	}

	/* set packet priority */
	skb->priority = pri;

	/* send it up */
	skb->dev = il->dev;
	skb->protocol = eth_type_trans(skb, il->dev);
	netif_rx(skb);
}

void
il_txreclaim(il_info_t *il, bool forceall)
{
	ilc_info_t *ilc;
#ifdef DMA
	uint start, end;
	uint i;
#endif

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_txreclaim\n", ilc->unit));

	/* if pio and transmit is pending AND (forceall or txdone) */
	if (PIOMODE(ilc)) {
		if (ilc->pioactive
			&& (forceall || (R_REG(&ilc->regs->xmtfifocontrol) & XFC_FR))) {
			dev_kfree_skb((struct sk_buff*) ilc->pioactive);
			ilc->pioactive = NULL;
		}
		return;
	}

#ifdef DMA
	/* unload and free all/completed skb in the tx descriptor ring */
	if (forceall) {
		for (i = 0; i < NTXD; i++)
			if (ilc->txp[i]) {
				W_SM(&ilc->txd[i].addr, 0);
				dev_kfree_skb((struct sk_buff*) ilc->txp[i]);
				ilc->txp[i] = NULL;
			}
	}
	else {
		start = ilc->txin;
		end = B2I(R_REG(&ilc->regs->xmtstatus) & XS_CD_MASK);

		ASSERT((start < NTXD) && (end < NTXD));

		for (i = start; i != end; i = NEXTTXD(i))
			if (ilc->txp[i]) {
				W_SM(&ilc->txd[i].addr, 0);
				dev_kfree_skb((struct sk_buff*) ilc->txp[i]);
				ilc->txp[i] = NULL;
			}

		ilc->txin = end;
	}
#endif
}

void
il_rxreclaim(il_info_t *il)
{
	ilc_info_t *ilc;
#ifdef DMA
	uint i;
#endif

	ilc = &il->ilc;

	IL_TRACE(("il%d: il_rxreclaim\n", ilc->unit));

	if (PIOMODE(ilc))
		return;

#ifdef DMA
	/* unload and free rx sk_buffs from the rx descriptor ring */
	for (i = 0; i != NRXD; i++)
		if (ilc->rxp[i]) {
			W_SM(&ilc->rxd[i].addr, 0);
			dev_kfree_skb((struct sk_buff*) ilc->rxp[i]);
			ilc->rxp[i] = NULL;
		}
#endif
}

void*
il_pget(il_info_t *il, bool send, uint len, uchar **va, uchar **pa)
{
	struct sk_buff *skb;

	if ((skb = dev_alloc_skb(len)) == NULL) {
		IL_ERROR(("il%d: il_pget: dev_alloc_skb failed\n", il->ilc.unit));
		return (NULL);
	}
	skb_put(skb, len);

	/* there must be at least TXOFF headroom */
	ASSERT(skb_headroom(skb) >= TXOFF);

	/* return a 0-mod-4 aligned buffer */
	ASSERT(IS_ALIGNED(skb->data, sizeof (int)));

	if (va)
		*va = skb->data;

	if (pa)
#ifdef __arm
		*pa = skb->data;
#else
		*pa = (uchar*) virt_to_bus(skb->data);
#endif

	return ((void*) skb);
}

void
il_pfree(il_info_t *il, struct sk_buff *skb, bool send)
{
	dev_kfree_skb(skb);
}

uchar*
il_ppush(il_info_t *il, struct sk_buff *skb, uint bytes)
{
	return (skb_push(skb, bytes));
}

uchar*
il_ppull(il_info_t *il, struct sk_buff *skb, uint bytes)
{
	return (skb_pull(skb, bytes));
}

uint
il_plen(struct sk_buff *skb)
{
	return (skb->len);
}

void
il_psetlen(struct sk_buff *skb, uint len)
{
	ASSERT(len <= BUFSZ);
	ASSERT((skb->data + len) <= skb->end);

	skb->len = len;
	skb->tail = skb->data + len;
}

void*
il_pdup(il_info_t *il, struct sk_buff *skb)
{
	struct sk_buff *n;

	if ((n = il_pget(il, TRUE, skb->len, NULL, NULL)) == NULL)
		return (NULL);

	bcopy(skb->data, n->data, skb->len);

	return (n);
}

int
il_dump(il_info_t *il, uchar *buf, uint len)
{
	/* big enough? */
	if (len < 4096)
		return (4096);

	sprintf(buf, "il%d: %s %s version %s\n", il->ilc.unit,
		__DATE__, __TIME__, EPI_VERSION_STR);

#ifdef BCMDBG
	il_dumpil(il, buf, len);
	ilc_dump(&il->ilc, &buf[strlen(buf)], len - strlen(buf));
#endif

	return (strlen(buf));
}

#ifdef BCMDBG
static void
il_dumpil(il_info_t *il, uchar *buf, uint len)
{
	sprintf(&buf[strlen(buf)], "il %p dev %p name %s tbusy %ld txq.qlen %u\n",
		il, il->dev, il->dev->name, il->dev->tbusy, il->txq.qlen);
}
#endif


uint
il_ms(void)
{
	return (jiffies * MILLISEC_PER_TICK);
}

void
il_delay(uint us)
{
	udelay(us);
}

void
il_link_up(il_info_t *il)
{
	IL_ERROR(("il%d: link up\n", il->ilc.unit));
}

void
il_link_down(il_info_t *il)
{
	IL_ERROR(("il%d: link down\n", il->ilc.unit));
}

void
il_assert(char *exp, char *file, uint line)
{
	char tempbuf[255];

	sprintf(tempbuf, "assertion \"%s\" failed: file \"%s\", line %d\n", exp, file, line);
	panic(tempbuf);
}

void*
il_malloc(uint size)
{
	return (kmalloc(size, GFP_ATOMIC));
}

void
il_mfree(void *addr, uint size)
{
	kfree(addr);
}
