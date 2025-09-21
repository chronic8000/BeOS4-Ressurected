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
 * Common OS-independent driver header file for
 * Broadcom BCM42XX InsideLine(TM) Phoneline Networking Adapter.
 *
 * Copyright(c) 2000 Broadcom Corp.
 * $Id: ilc.h,v 13.130.4.2 2000/05/12 00:29:12 csw Exp $
 */

#ifndef _ilc_h_
#define _ilc_h_

#include <il_dbg.h>

/* ILCP implementation related tunables */
#define	MAXMULTILIST	32		/* max # multicast addresses */
#define CN_TIMEOUT	128		/* constel negot interval in seconds */
#define MAXBEBL		1024		/* maximum rate negot backoff */
#define FBBEBL		64		/* maximum fancy fallback rate negot backoff */
#define RXSEL_NPKTS_HIST 16		/* # frames PCRC err hist per path */

/*
 * Multicast rate negotiation flags.
 * Use 3 rc flags to make sure we always have a minimum of 2 un-expired
 * intervals, even immediately after rotating the flag bits.
 */
#define MC_HIST_SC0	1
#define MC_HIST_RD0	2
#define MC_HIST_RD1	4
#define MC_HIST_RC0	8
#define MC_HIST_RC1	16
#define MC_HIST_RC2	32
#define MC_HIST_MASK1	(MC_HIST_RD0 | MC_HIST_RD1 | MC_HIST_RC0 | MC_HIST_RC1)
#define MC_HIST_MASK2	(MC_HIST_RD0 | MC_HIST_RD1)
#define MC_HIST_MASK3	(MC_HIST_RD0 | MC_HIST_RD1 | MC_HIST_RC0 | MC_HIST_RC1 | MC_HIST_RC2)
#define MC_HIST_MASK4	~(MC_HIST_SC0 | MC_HIST_RD0 | MC_HIST_RC0)

/*
 * station control block - one per remote MAC address.
 */
struct scb {
	struct scb	*next;		/* pointer to next scb in chain */
	uint		hpnatype;	/* station type: 0=unknown, 1=v1, 2=v2 */
	struct ether_addr	ea;	/* key */
	struct framecontrol	fc;	/* frame control header */
	uint		used;		/* time of last use */
	bool		inuse;		/* inuse=true, free=false */

	uint8		rx_desc;	/* receive rate descriptor */
	uint16		bebi;		/* rate binary exponential backoff count */
	uint16		bebl;		/* rate binary exponential backoff limit */
	bool		ilineokay;	/* result of last TUT pregain hdr envelope / squelch test */
	uint		expire;		/* timestamp of when to try switching to mixed mode (sec) */
	uint		expirek;	/* exponent for recomputing next expire time */
	uint8		rxsel_pcrc_index; /* index into pld CRC err history */
	bool		rxsel_pcrc[RXSEL_NPKTS_HIST]; /* pld CRC err history, for TUT or ILINE frames */
	uint8		mc_hist[NSCB];	/* multicast rate negotiation stats */
	uint8		mc_sd0;		/* sent data to mcast addr, current interval */
#ifdef IL_STATS
	phy_stats_path_info_t *stats;	/* per-path stats */
#endif
};

#ifdef IL_BRIDGE
struct refid {
	struct ether_addr ea;		/* mac address */
	uint used;			/* last time addr was touched */
	struct scb *scb;		/* associated scb or NULL */
};
#endif

/*
 * chip performance and event counters.
 */
struct hwctrs {
	uint	ctrcollisions;		/* # tx total collisions */
	uint	ctrcollframes;		/* # tx frames encountering a collision */
	uint	ctrshortframes;		/* # rx frames discarded due to too short */
	uint	ctrlongframes;		/* # rx frames discarded due to too long */
	uint	ctrfilteredframes;	/* # rx frames discarded due to nomatch */
	uint	ctrmissedframes;	/* # rx frames discarded due to fifo overflow */
	uint	ctrerrorframes;		/* # rx frames discarded due to demod error */
	uint	ctrrcvtooshort;		/* # rx frames discarded due to <20byte len */
	uint	ctrxmtlinkintegrity;	/* # tx link integrity frames */
	uint	ctrifgviolation;	/* # carrier detected in Interframe_Gap */
	uint	ctrsignalviolation;	/* # carrier detected in Collision_Signal */
	uint	ctrstackbubble;		/* # dfpq winner did not transmit in slot */
	uint	qosusage[8];		/* rx mac time in us, by priority */
	uint	qoscoll[8];		/* # collisions by priority */
	uint	qosmacidle;		/* idle mac time, in us */
};

#ifdef IL_CERT
struct cert {
	/* data send control info */
	uint		enabled;	/* enable/disable flag */

	uint16		send_dseq;	/* current data pkt sequence number */
	uint		send_numend;	/* number of end pkts sent */
	uint32		send_dgen;	/* data generattor */
	uint32		send_dseed;	/* seed for data generator */
	uint8		send_dxsum;	/* use data checksums */
	uint32		send_tgen;	/* tag generattor */
	uint32		send_tseed;	/* seed for tag generattor */
	uint16		send_msec;	/* msec between bursts, 0=continuous */
	uint16		send_nburst;	/* number of data pkts/burst */
	uint32		send_npkts;	/* total number of data pkts to send */
	uint16		send_length;	/* data packet length */
	int8		send_txpe;	/* data packet payload encoding */
	int8		send_txpri;	/* data packet priority */
	uint8		send_numacks;	/* num. of acks to send at start/end */
	uint8		send_numaddr;	/* number of DAs (currently only 1 is
					   supported) */
	uint16		send_cseq;	/* sequence number of send request */

	struct ether_addr	send_sa;	/* SA for data pkts */
	struct ether_addr	send_da;	/* DA for data pkts */
	struct ether_addr	send_ca;	/* controller address */

	/* data stats */
	uint32		xmt_nframes;	/* total xmt frames w/o err */
	uint32		xmt_nbytes;	/* total xmt bytes w/o err */
	uint32		xmt_err;	/* transmit errors */
	uint32		xmt_etime;	/* elapsed time from xmt of first frame
						to xmt of first 'end' frame */

	uint32		rcv_seq_exp;	/* next expected sequence number */
	uint32		rcv_nframes;	/* total rcv frames w/o err */
	uint32		rcv_nbytes;	/* total rcv bytes w/o */
	uint32		rcv_seq_miss;	/* missed seq */
	uint32		rcv_seq_err;	/* rcv seq out of order */
	uint32		rcv_data_err;	/* rcv data compare error */
	uint32		rcv_err;	/* any other errors */
	uint32		rcv_etime;	/* elapsed time from recv of first frame
						to recv of first 'end' frame */
	uint32		rcv_xsum_err;	/* rcv checksum error */
};
#endif /* IL_CERT */

/*
 * Common OS-independent software data structure.
 * The OS-specific software data structure must start with one of these.
 *
 * There's a transmit descriptor ring and a receive descriptor ring
 * and the driver allocates one page of uncached shared memory for each.
 *
 * A window of the descriptors in each ring (CurrDscr thru (LastDscr-1))
 * is "posted" to the chip for DMA read at any given time.
 * Descriptors are read by the chip -- never written.
 * Descriptors are written by the device driver -- never read.
 *
 * There's a pointer ("regs") to the mapped chip registers.
 *
 * There are two vectors of "parallel" packet pointers, one for transmit
 * and the other for receive, with each entry pointing at the packet
 * referred to by the corresponding descriptor ring entry buffer address.
 * "Loading" a buffer into one of the descriptor rings involves
 * storing the packet pointer into the packet pointer array
 * then writing the descriptor with the address of the packet buffer memory.
 * Unloaded descriptors correspond to packet pointer array entries
 * having a value of NULL.
 *
 * There is a hash of SCBs - station control blocks - one per remote
 * MAC address.  The SCB caches the prepared framecontrol for the remote station,
 * along with constellation negotiation state.
 */
typedef struct ilc_info {
	struct ilc_info	*next;		/* debug linked list */
	uint		unit;		/* device instance number */
	bool		up;		/* interface up and running */
	bool		promisc;	/* promiscuous destination address */
	bool		promisctype;	/* promiscuous ether_type (send up ilcp) */
	bool		loopbk;		/* loopback override mode */
	bool		linkint;	/* enable link integrity */
	int		txbpb;		/* bpb override (<0 = default) */
	int		txpri;		/* priority override (<0 = default) */
	uint		txprimap[ILINE_NPRI];	/* tx priority map, use only with LARQ*/
	bool		enic2inic;	/* PR?? set enic txgain to 0x80 */

	uint		larqlevel;	/* 0=off, 1=rxonly, 2=on, 3=hdrsonly */
	void		*larq_handle;	/* larq handle */

	bool		piomode;	/* enable programmed io (!dma) */
	void		*pioactive;	/* points to pio packet being transmitted */

	uint16		vendorid;	/* PCI vendor id */
	uint16		deviceid;	/* device id of chip */
	uint		chiprev;	/* chip revision */
	uint		subvendor;	/* pci subsystem vendor id */
	uint		subid;		/* pci subsystem id */
	uint		rxhdrlen;	/* chip-specific rxhdr length in bytes */
	uchar		bdmajrev;	/* board major revision */
	uchar		bdminrev;	/* board minor revision */
	uchar		aferev;		/* afe revision */

	bcm42xxregs_t	*regs;		/* pointer to chip registers */
	uint		regsmaplen;	/* # bytes mapped chip registers */

	struct ether_addr cur_etheraddr; /* our local ethernet address */
	struct ether_addr perm_etheraddr; /* original sprom local ethernet address */

	struct ether_addr multicast[MAXMULTILIST];
	uint		nmulticast;
	bool		allmulti;	/* enable all multicasts */

#ifdef DMA
	bcm42xxdd_t	*txd;		/* point to tx descriptor ring */
	void		*txdpa;		/* physical address of descriptor ring */
	void		*txp[NTXD];	/* parallel vector of pointers to packets */
	uint		txin;		/* index of next descriptor to reclaim */
	uint		txout;		/* index of next descriptor to post */

	bcm42xxdd_t	*rxd;		/* point to rx descriptor ring */
	void		*rxdpa;		/* physical address of descriptor ring */
	void		*rxp[NRXD];	/* parallel vector of pointers to packets */
	uint		rxin;		/* index of next descriptor to reclaim */
	uint		rxout;		/* index of next descriptor to post */
#endif

	uint		macmode;	/* master current hpna operating mode */
	uint		configmode;	/* forced hpna operating mode */
	bool		localforce;	/* local configmode takes precedence */
	uint		v1_detected;	/* v1 w/pcom=0, 60sec countdown */
	uint		v1_signaled;	/* v1 w/pcom=2, 60sec countdown */
	uint		forcetut;	/* 60sec countdown to force tut before mixedmode */
	uint		pcom;		/* software pcom value */
	uint		ctr1linkrcvd;	/* most recently read chip hpna1.0 counter */
	bool		xmt_tut;	/* all outgoing frames must be tut */
	bool		xmt_gapped;	/* all outgoing frames must be gapped */

	char		debug[MAXDEBUGSTR]; /* register override string */

	uint		intmask;	/* current software interrupt mask */

	bool		dogtimerset;	/* true if dogtimer is running */

	bool		csa;		/* 0=off, 1=on */
	bool		csa_rtx;	/* csa retransmit requested */
	uint32		csa_configmode;	/* csa hpna forcing mode */
	uint32		csa_newtx, csa_prevtx, csa_oldtx; /* csa tx flag sets */
	uint32		csa_newrx, csa_prevrx; /* csa rx flag sets */

	uint		linkstate;	/* link integrity state */
	uint		li_force_ctr;	/* LI force send counter */
	struct ether_addr	srcid1;	/* src address of first link frame */
	bool		pm_modechange;	/* true if mode change is due to pm */

	uint16		cur_squelch;	/* latest squelch value received from tut frame */
	uint16		old_squelch;	/* squelch snapshot from prev watchdog */
	int		delta_squelch;	/* watchdog squelch sampled change over 1s */

	uint		cn_ctr;		/* constel negot interval counter */
	uint16		bandselmask;	/* bit flags for whether device can select each band */
	uint16		bandupdmask;	/* bit flags for whether device can update each band */

	struct scb	scb[NSCB];	/* station control block pool */
	struct scb	*scbhash[NSCB];	/* station control block hash */
#ifdef IL_BRIDGE
	struct refid    refid[NREFID];	/* fixed-size lru fully associative table */
#endif

	uint		now;		/* # elapsed seconds */

#ifdef IL_STATS
	stats_state_t	stats;		/* see il_stats.h */
	struct hwctrs	oldhwctr;	/* previous chip event counter values */
#endif

#ifdef IL_CERT
	struct cert	cert;
#endif

	uint		cheststate;	/* channel estimate sw state */
	uint		chestidx;	/* channel estimate array index */
	bool		bigendian;	/* endian type for platform */
} ilc_info_t;


#ifdef DMA
#define	PIOMODE(ilc)		((ilc)->piomode)
#else
#define	PIOMODE(ilc)		(1)
#endif

#ifdef IL_STATS
#define	COUNTER(ilc, index)	(ilc)->stats.counter_block->counters[index]
#else
static	int	dummycounter;
#define	COUNTER(ilc, index)	(dummycounter)
#endif

/* general convenience */
#ifndef __BEOS__
#define	MIN(a, b)		(((a) <= (b))? (a): (b))
#define	MAX(a, b)		(((a) >= (b))? (a): (b))
#endif
#define	IS_ALIGNED(a, x)	(((uint)(a) & ((x)-1)) == 0)
#define ROUNDUP(x, y)		((((x)+((y)-1))/(y))*(y))

/* chip interrupt bit error summary */
#define	I_ERRORS	(I_EC | I_XH | I_PC | I_PD | I_DE | I_RU | I_RO | I_XU)
#define	MSII_ERRORS	(MSII_EC | MSII_XH | MSII_RO)

/* descriptor bumping macros */
#define	NEXTTXD(i)	((i + 1) & (NTXD - 1))
#define	NEXTRXD(i)	((i + 1) & (NRXD - 1))
#define	NTXDACTIVE(h, t)	((t - h) & (NTXD - 1))
#define	NRXDACTIVE(h, t)	((t - h) & (NRXD - 1))

/* macros to convert between byte offsets and indexes */
#define	B2I(bytes)	((bytes) / sizeof (bcm42xxdd_t))
#define	I2B(index)	((index) * sizeof (bcm42xxdd_t))

#define	SCBINDEX(il, scbx)	(int)((struct scb *)scbx - ((struct ilc_info*)(il))->scb)
#define	SCBHASHINDEX(id)	((id[0] ^ id[1] ^ id[2] ^ id[3] ^ id[4] ^ id[5]) & (NSCB - 1))

#define SPINWAIT(exp, us) { \
	uint countdown = (us) + 9; \
	while ((exp) && (countdown >= 10)) {\
		il_delay(10); \
		countdown -= 10; \
	} \
}

#define	EHINIT(eh, src, dst, type) \
	bcopy((uchar*)dst, (eh)->ether_dhost, ETHER_ADDR_LEN); \
	bcopy((uchar*)src, (eh)->ether_shost, ETHER_ADDR_LEN); \
	(eh)->ether_type = hton16(type);

/* link integrity states */
#define	LI_DOWN		0
#define	LI_UP_M1	1
#define	LI_UP_0		2
#define	LI_UP_1		3
#define	LI_UP_2		4

#define	RRCF_MIN_BACKOFF	1
#define	RRCF_MAX_BACKOFF	1024

/* scrambler init and priority macros */
#define	SI(x)		((x) & CTL_SI_MASK)
#define	SETSI(x, v)	(x) = (((x) & ~CTL_SI_MASK) | ((v) & CTL_SI_MASK))
#define	PRI(x)		(((x) & CTL_PRI_MASK) >> CTL_PRI_SHIFT)
#define	SETPRI(x, v)	(x) = (((x) & ~CTL_PRI_MASK) | (((v) << CTL_PRI_SHIFT) & CTL_PRI_MASK))

/* channel estimate state */
#define	CH_OFF		0	/* off */
#define	CH_ON		1	/* waiting for packet arrival */
#define	CH_TRIGGERED	2	/* triggered - need a il_init() to clean up */

/* for rxdesc selection */
#define TXDESC_TUT	8	/* logical rate value used to specify force to 1.0 mode */
#define IS_ILINE_PE(pe) \
	((pe >= (ILINE_MIN_BPB-1)) && (pe <= (ILINE_MAX_BPB-1)))

/* hpna type */
#define HPNAUNKNOWN	0
#define HPNAV1		1
#define HPNAV2		2

/* hpna mode values */
#define	MODE_AUTO	0	/* auto-select */
#define	MODE_TUT	1	/* hpna 1.x (tut) mode */
#define	MODE_ILINE	2	/* native hpna 2.x (iline10) mode */
#define	MODE_COMPAT	3	/* compatibility (mixed-mode) */

#define	AIDMASK		0xef	/* AID values 240-254 are reserved, 255 is used for MC */

#define	SENDLARQ(ilc)	(((ilc)->larqlevel > 0) && ((ilc)->macmode == MODE_ILINE))

#define IL_PMLARQBIT	(1 << 6) /* bit position of larq identifier */
#define IL_PMLARQBYTE	1	/* Byte position of larq identifier */

/* shortcuts for bug workarounds */
#define	IS4210(devid)	((devid == EPI41210_DEVICE_ID) || (devid == EPI41230_DEVICE_ID))
#define	IS4211(devid)	((devid == BCM4211_DEVICE_ID) || (devid == BCM4231_DEVICE_ID))
#define	IS4220(devid)	((devid == BCM4410_DEVICE_ID) || (devid == BCM4430_DEVICE_ID))

/* true if we're capable of measuring Band2 payload encodings */
#define BAND2UPDCAP(ilc) (!(IS4210((ilc)->deviceid)) && !(IS4211((ilc)->deviceid)))

/* true if we're capable of selecting Band2 payload encodings */
#define BAND2SELCAP(ilc) (BAND2UPDCAP(ilc) && ((ilc)->macmode == MODE_ILINE))

/* common ioctl definitions */
#define ILCTXDOWN	0
#define ILCLOOP		1
#define ILCDUMP		2
#define ILCDUMPSCB	3
#define ILCTXBPB	4
#define ILCTXPRI	5
#define ILCSETMSGLEVEL	6
#define ILCLINKINT	7
#define ILCHPNAMODE	8
#define ILCCSA		9
#define ILCCSAHPNAMODE	10
#define	ILCLARQDUMP	11
#define	ILCLARQONOFF	12
#define	ILCPROMISCTYPE	13
#define ILCDUMPPES	14
#define ILCPESSET	15

/* exported externs */
extern ilc_info_t *ilc_list;
extern uint ilc_stats_size;
extern int ilc_rxsel_hyst;
extern int ilc_rxsel_maxerr;
extern int ilc_rxsel_maxdelta;
extern int ilc_rxsel_kmax;
extern int ilc_rxsel_compatloss;
extern uint il_null_txpri[];

/* exported prototypes */
extern int ilc_attach(ilc_info_t *ilc);
extern void ilc_detach(ilc_info_t *ilc);
extern void ilc_reset(ilc_info_t *ilc);
extern void ilc_init(ilc_info_t *ilc);
extern void ilc_up(ilc_info_t *ilc);
extern uint ilc_down(ilc_info_t *ilc, int reset);
extern int ilc_ioctl(ilc_info_t *ilc, int cmd, void *arg);
extern void ilc_promisc(ilc_info_t *ilc, uint on);
extern void ilc_dump(ilc_info_t *ilc, uchar *buf, int size);
extern void ilc_dumphdrs(ilc_info_t *ilc, char *prefix,
	struct framecontrol *fc, uchar *pcomp, bcm42xxrxhdr_t *rxh, uint len);
extern char *ether_ntoa(struct ether_addr *ea, char *buf);
extern void ilc_chipreset(ilc_info_t *ilc);
extern void ilc_chipinit(ilc_info_t *ilc);
extern void ilc_etherfilter(ilc_info_t *ilc, uint index, struct ether_addr *ea);
extern void ilc_fill(ilc_info_t *ilc);
extern void ilc_intr(ilc_info_t *ilc, uint intstatus);
extern uint ilc_send(ilc_info_t *ilc, void *p, int txbpb, int txpri);
extern void ilc_recv(ilc_info_t *ilc, bcm42xxrxhdr_t *rxh, void *p);
extern void ilc_dispatch(ilc_info_t *ilc, void *p, uint pri, uint pcrc);
extern void *ilc_recvpio(ilc_info_t *ilc);
extern void ilc_sendpio(ilc_info_t *ilc, uchar *va, int len, uint datatype);
extern void ilc_xmtfifo(ilc_info_t *ilc, uchar *va, int len);
extern void ilc_trigger_channel_estimate(ilc_info_t *ilc);
extern void ilc_watchdog(ilc_info_t *ilc);
extern void ilc_csa_forcemode(ilc_info_t *ilc, uint forcemode);
extern struct scb *ilc_scblookup(ilc_info_t *ilc, uchar *ea);
extern void ilc_csaup(ilc_info_t *ilc);
extern void ilc_csadown(ilc_info_t *ilc);
#ifdef IL_STATS
extern uint ilc_stats_allocinit(ilc_info_t *ilc, char *driver_vers);
#endif
extern int ilc_set_debug(ilc_info_t *ilc, char *s, uint len);
extern void ilc_debug(ilc_info_t *ilc);
extern char *ilc_strchr(const char *p, int ch);
extern void ilc_spromread(ilc_info_t *ilc, uint16 *buf, uint byteoff, uint nbyte);
extern bool ilc_spromgood(ilc_info_t *ilc, uint byteoff, uint nbyte);
extern bool ilc_chipid(uint16 vendor, uint16 device);
extern void ilc_txint_on(ilc_info_t *ilc);
extern void ilc_txint_off(ilc_info_t *ilc);

#ifdef IL_CERT
extern void ilc_recvcert(ilc_info_t *ilc, struct ether_header *eh);
extern void ilc_intrcert(ilc_info_t *ilc);
#endif

#endif	/* _ilc_h_ */
