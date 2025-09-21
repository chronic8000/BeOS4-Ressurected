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
 * Broadcom BCM42XX PCI iLine10(TM)
 * InsideLine 10Mbps Home Phoneline Networking Controller.
 *
 * $Id: bcm42xx.h,v 1.79 2000/05/01 23:49:13 nn Exp $
 * Copyright(c) 1999 Broadcom Corporation
 */


#ifndef _BCM42XX_H
#define	_BCM42XX_H

#include "typedefs.h"

/* known vendors */
#define	VENDOR_EPIGRAM		0xfeda
#define	VENDOR_BROADCOM		0x14e4
#define	VENDOR_3COM		0x10b7
#define	VENDOR_NETGEAR		0x1385
#define	VENDOR_DIAMOND		0x1092

#define	BCM4210_DEVICE_ID	0x1072		/* never used */
#define	BCM4211_DEVICE_ID	0x4211
#define	BCM4230_DEVICE_ID	0x1086		/* never used */
#define	BCM4231_DEVICE_ID	0x4231

#define	BCM4410_DEVICE_ID	0x4410		/* bcm44xx family pci iline */
#define	BCM4430_DEVICE_ID	0x4430		/* bcm44xx family cardbus iline */

#define	EPI41210_DEVICE_ID	0xa0fa		/* bcm4210 */
#define	EPI41230_DEVICE_ID	0xa10e		/* bcm4230 */

#define	BCM42XX_NFILTERS	64		/* # ethernet address filter entries */
#define	BCM42XX_MCHASHBASE	0x200		/* multicast hash filter base address */
#define	BCM42XX_MCHASHSIZE	256		/* multicast hash filter size in bytes */
#define	BCM42XX_MAX_DMA		4096		/* chip has 12 bits of DMA addressing */

/* power management wakeup pattern constants */
#define	BCM42XX_NPMP		4		/* chip supports 4 wakeup patterns */
#define	BCM42XX_PMPBASE		0x400		/* filter table pattern base address */
#define	BCM42XX_PMPSIZE		128		/* sizeof each wakeup pattern */
#define	BCM42XX_PMMBASE		0x600		/* filter table mask base address */
#define	BCM42XX_PMMSIZE		16		/* sizeof each wakeup mask */
#define BCM42XX_PMLIVE		0x1a		/* link integrity interval value */

/* known subids */
#define	EPIGRAM_ENIC		0x0013
#define	EPIGRAM_INIC		0x0101
#define	EPIGRAM_RNIC		0x0102
#define	NETGEAR_INIC		0x3000

/*
 * Sprom size on cbnics, current PCI NICs only have 64 words, the chips actually
 * supports up to 1024 words (16kilobit) sproms
 */
#define	SPROM_SPACE		256		/* 16bit words */

/* afe revisions */
#define	AFE_UNKNOWN		0
#define	AFE_A0			1
#define	AFE_A1			2
#define	AFE_B0			3

/* cpp contortions to concatenate w/arg prescan */
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)

/*
 * Host Interface Registers
 *
 * Registers and bits introduced after the 4210 are marked with "(chip#)" .
 */
typedef volatile struct {
	/* Device and Power Control */
	uint32	devcontrol;
	uint32	devstatus;
	uint32	compat;
	uint32	PAD;
	uint32	wakeuplength;
	uint32	wakeuplength1;			/* (4220) */
	uint32	PAD[2];
	
	/* Interrupt Control */
	uint32	intstatus;
	uint32	intmask;
	uint32	gptimer;
	uint32	PAD;

	/* Analog Front End Control */
	uint16	afestatus;
	uint16	PAD;
	uint16	afecontrol;
	uint16	PAD;
	uint32	PAD[2];

	/* Front End and Modem Control  (Start of slow registers) */
	uint16	frametrim;
	uint16	PAD;
	uint16	hpnaaddr;
	uint16	PAD;
	uint16	hpnadata;
	uint16	PAD;
	uint16	gapcontrol;
	uint16	PAD;
	uint16	trackerdebug;
	uint16	PAD;
	uint16	festatus;
	uint16	PAD;
	uint16	fecontrol;
	uint16	PAD;
	uint16	fecsenseon;
	uint16	PAD;
	uint16	fecsenseoff;
	uint16	PAD;
	uint16	fedbgaddr;
	uint16	PAD;
	uint16	fedbgdata;
	uint16	PAD;
	uint16	cspower;
	uint16	PAD;
	uint16	modcontrol;
	uint16	PAD;
	uint16	moddebug;
	uint16	PAD;
	uint16	cdcontrol;
	uint16	PAD;
	uint16	noisethreshold;
	uint16	PAD;			/* End of slow regs */

	/* Frame Processor */
	uint32	framestatus;
	uint32	framecontrol;
	uint32	enetaddrlower;
	uint32	enetaddrupper;
	uint32	enetftaddr;
	uint32	enetftdata;
	uint32	linksrclower;
	uint32	linksrcupper;

	uint32	maccontrol;
	uint32	pcom;
	uint32	PAD[2];

	/* GPIO Interface */
	uint32	gpiooutput;
	uint32	gpioouten;
	uint32	gpioinput;
	uint32	PAD;

	/* Direct FIFO Interface */
	uint16	xmtfifocontrol;
	uint16	xmtfifodata;
	uint16	rcvfifocontrol;
	uint16	rcvfifodata;
	uint16	msiintstatus;
	uint16	msiintmask;
	uint16	msictraddress;				/* 4210/4211 only */
	uint16	msictrdata;				/* 4210/4211 only */

	/* CardBus Function 0 registers */
	uint32	funcevnt;
	uint32	funcevntmask;
	uint32	funcstate;
	uint32	funcforce;

	uint32	PAD[8];

	/* Interrupt register */
	uint32	intrecvlazy;
	uint32	PAD[3];

	/* VOIP Timing (4220) */
	uint32	tsccontrol;
	uint32	tscsid;
	uint16	txtimestamplo;
	uint16	txtimestamphi;
	uint16	rxtimestamplo;
	uint16	rxtimestamphi;
	uint32	PAD[24];

	/* Shadow PCI config */
	uint16	pcicfg[64];

	/* Transmit DMA engine */
	uint32	xmtcontrol;
	uint32	xmtaddr;
	uint32	xmtptr;
	uint32	xmtstatus;

	/* Receive Dma engine */
	uint32	rcvcontrol;
	uint32	rcvaddr;
	uint32	rcvptr;
	uint32	rcvstatus;

	uint32	PAD[56];

	/* Miscellaneous counters */
	uint32	ctrcollisions;
	uint32	ctrcollframes;
	uint16	ctrshortframes;
	uint16	PAD;
	uint16	ctrlongframes;
	uint16	PAD;
	uint32	ctrfilteredframes;
	uint32	ctrmissedframes;
	uint32	ctrerrorframes;
	uint16	ctr1linkrcvd;
	uint16	PAD[3];
	uint16	ctrrcvtooshort;
	uint16	PAD;
	uint16	ctrxmtlinkintegrity;
	uint16	PAD;
	uint16	ctr1latecoll;
	uint16	PAD;
	uint16	ctrifgviolation;
	uint16	PAD;
	uint16	ctrsignalviolation;
	uint16	PAD;
	uint16	ctrstackbubble;
	uint16	PAD[3];

	/* QoS performance counters (4211) */
	uint32	qosusage[8];
	uint16	qoscoll[8];
	uint32	qosmacidle;

	uint32	PAD[291];

	/* Serial Prom */
	uint16	sprom[SPROM_SPACE];
} bcm42xxregs_t;

/* device control */
#define	DC_RS		((uint32)1 << 0)	/* soft reset */
#define	DC_RT		((uint32)1 << 1)	/* receive test mode enable */
#define	DC_XT		((uint32)1 << 2)	/* transmit test mode enable */
#define	DC_ME		((uint32)1 << 3)	/* modem master enable */
#define	DC_XE		((uint32)1 << 5)	/* transmit enable */
#define	DC_RE		((uint32)1 << 6)	/* receive enable */
#define	DC_PM		((uint32)1 << 7)	/* pattern filtering enable */
#define	DC_DD		((uint32)1 << 8)	/* dsp clock disable */
#define	DC_FG		((uint32)1 << 9)	/* force gated clock on */
#define	DC_SW		((uint32)1 << 10)	/* sprom write (4220) */
#define	DC_BS		((uint32)1 << 11)	/* msi byte swap */

/* device status */
#define	DS_RI_MASK	0xf			/* revision id */
#define	DS_MM_MASK	0xf0			/* MSI mode (4220) */
#define	DS_MM_SHIFT	4
#define	DS_MM		((uint32)1 << 8)	/* msi selected (otherwise pci mode) */
#define	DS_SP		((uint32)1 << 9)	/* serial prom present */
#define	DS_SL		((uint32)1 << 10)	/* low-order 512 bits of sprom locked */
#define	DS_SS_MASK	0x1800			/* bcm4211: sprom size */
#define	DS_SS_SHIFT	11
#define	DS_SS_1K	0x0000			/*  1 kbit */
#define	DS_SS_4K	0x0800			/*  4 kbit */
#define	DS_SS_16K	0x1000			/* 16 kbit */
#define	DS_RR		((uint32)1 << 15)	/* register read ready (4220) */
#define	DS_BO_MASK	0xffff0000		/* bond option device id (4220) */
#define	DS_BO_SHIFT	16

/* compatibility mode */
#define	CM_CL		((uint32)1 << 0)	/* classify enable */
#define	CM_CE		((uint32)1 << 1)	/* mixed mode enable */
#define	CM_IL		((uint32)1 << 2)	/* hpna 2.0 enable */
#define	CM_MS		((uint32)1 << 3)	/* mac select: 0=DFPQ, 1=BEB */
#define	CM_TG		((uint32)1 << 4)	/* tx gap enable */

/* wakeup length */
#define	WL_P0_MASK	0x7f			/* pattern 0 */
#define	WL_D0		((uint32)1 << 7)
#define	WL_P1_MASK	0x7f00			/* pattern 1 */
#define	WL_P1_SHIFT	8
#define	WL_D1		((uint32)1 << 15)
#define	WL_P2_MASK	0x7f0000		/* pattern 2 */
#define	WL_P2_SHIFT	16
#define	WL_D2		((uint32)1 << 23)
#define	WL_P3_MASK	0x7f000000		/* pattern 3 */
#define	WL_P3_SHIFT	24
#define	WL_D3		((uint32)1 << 31)

/* intstatus and intmask */
#define	I_TO		((uint32)1 << 7)	/* general purpose timeout */
#define	I_EC		((uint32)1 << 8)	/* excessive collisions error */
#define	I_XH		((uint32)1 << 9)	/* transmit header error */
#define	I_PC		((uint32)1 << 10)	/* pci descriptor error */
#define	I_PD		((uint32)1 << 11)	/* pci data error */
#define	I_DE		((uint32)1 << 12)	/* descriptor protocol error */
#define	I_RU		((uint32)1 << 13)	/* receive descriptor underflow */
#define	I_RO		((uint32)1 << 14)	/* receive fifo overflow */
#define	I_XU		((uint32)1 << 15)	/* transmit fifo underflow */
#define	I_RI		((uint32)1 << 16)	/* receive interrupt */
#define	I_XI		((uint32)1 << 24)	/* transmit interrupt */

/* analog front end status */
#define	AFES_RV_MASK	0xff			/* revision id */

/* analog front end control */
#define	AFEC_PD		(1 << 1)		/* afe power down */
#define	AFEC_CL		(1 << 2)		/* calibrate */

/* frame trim */
#define	FT_OE_MASK	0xff			/* offset end of frame */
#define	FT_ED_MASK	0xff00			/* offset end */
#define	FT_ED_SHIFT	8

/* gapcontrol */
#define	GC_SL_MASK	0x3f			/* symbol length */
#define	GC_GP_MASK	0x3c0			/* gap payload */
#define	GC_GP_SHIFT	6
#define	GC_GT_MASK	0x7c00			/* gap to eof */
#define	GC_GT_SHIFT	10
#define	GC_GE		(1 << 15)		/* gap eof enable */

/* tracker debug */
#define	TD_HA_MASK	0x3			/* header kka shift */
#define	TD_HK_MASK	0xc			/* header k shift */
#define	TD_HK_SHIFT	2
#define	TD_PA_MASK	0x30			/* payload kka shift */
#define	TD_PA_SHIFT	4
#define	TD_PK_MASK	0xc0			/* payload k shift */
#define	TD_PK_SHIFT	6
#define	TD_ER_MASK	0xff00			/* test error */
#define	TD_ER_SHIFT	8

/* front end status */
#define	FES_JB		(1 << 0)		/* jabber detected */
#define	FES_MS_MASK	0x1e			/* mac state */
#define	FES_MS_SHIFT	1
#define	FES_SW		(1 << 7)		/* software led */
#define	FES_L0_MASK	0x300			/* led0 output select */
#define	FES_L0_SHIFT	8
#define	FES_L1_MASK	0xc00			/* led1 output select */
#define	FES_L1_SHIFT	10
#define	FES_L2_MASK	0x3000			/* led2 output select */
#define	FES_L2_SHIFT	12
#define	FES_L3_MASK	0xc000			/* led3 output select */
#define	FES_L3_SHIFT	14

/* front end control */
#define	FEC_PS		(1 << 0)		/* phase splitter enable */
#define	FEC_DC		(1 << 1)		/* dc blocking filter enable */
#define	FEC_XF		(1 << 2)		/* transmit filter enable */
#define	FEC_XB		(1 << 3)		/* transmit bandpass filter enable */
#define	FEC_RB		(1 << 4)		/* receive bandpass filter enable */
#define	FEC_RG		(1 << 5)		/* receive gain enable */
#define	FEC_MC		(1 << 6)		/* mac enable */
#define	FEC_CD		(1 << 7)		/* carrier detect enable */
#define	FEC_CE		(1 << 8)		/* channel estimator enable */
#define	FEC_RN		(1 << 9)		/* receive notch filter enable */
#define	FEC_LB		(1 << 10)		/* loopback enable */
#define	FEC_HX		(1 << 11)		/* hold transmit enable */
#define	FEC_UN		(1 << 12)		/* use notch (new in 4220) */
#define	FEC_CR		(1 << 13)		/* collision resolution enable */
#define	FEC_RF		(1 << 14)		/* rfi tracker enable */
#define	FEC_CL		(1 << 15)		/* collision detection enable */

/* front end carrier sense on threshold */
#define	FEON_T1_MASK	0xff			/* carrier sense on threshold 1 */
#define	FEON_T2_MASK	0xff00			/* carrier sense on threshold 2 */
#define	FEON_T2_SHIFT	8

/* front end carrier sense off threshold */
#define	FEOFF_OF_MASK	0xff			/* carrier sense off threshold */
#define	FEOFF_CT_MASK	0x700			/* cd off trials */
#define	FEOFF_CT_SHIFT	8
#define	FEOFF_GT_MASK	0x3800			/* gapped cd off trials */
#define	FEOFF_GT_SHIFT	11

/* front end debug address */
#define	FEDA_DO_MASK	0x1ff			/* debug address offset */
#define	FEDA_DS_MASK	0xfe00			/* debug address select */
#define	FEDA_DS_SHIFT	9
#define	FEDA_DS_MAC	(1 << 9)		/* mac */
#define	FEDA_DS_GAIN	(1 << 10)		/* gain estimator */
#define	FEDA_DS_DFE	(1 << 11)		/* dfe */
#define	FEDA_DS_FFE	(1 << 12)		/* ffe */
#define	FEDA_DS_PP	(1 << 13)		/* preamble processor */
#define	FEDA_DS_CHEST	(1 << 14)		/* channel estimator */
#define	FEDA_DS_CD	(1 << 15)		/* carrier detect */

/* carrier sense power */
#define	CP_PT_MASK	0xff			/* power threshold */

/* modem control */
#define	MC_SE		(1 << 0)		/* scrambler enable */
#define	MC_BM		(1 << 1)		/* bit mapper enable */
#define	MC_RS		(1 << 2)		/* resampler enable */
#define	MC_EU		(1 << 3)		/* equalizer update enable */
#define MC_SM		(1 << 4)		/* symbol mapper enable */
#define	MC_DE		(1 << 5)		/* descrambler enable */
#define	MC_TE		(1 << 6)		/* phase tracker enable */
#define	MC_DF		(1 << 7)		/* dfe enable */
#define	MC_FF		(1 << 8)		/* ffe enable */
#define	MC_DL		(1 << 9)		/* delay enable */
#define	MC_XN		(1 << 10)		/* tx notch enable */
#define	MC_MT		(1 << 11)		/* mac test enable */
#define	MC_CT		(1 << 12)		/* carrier test enable */
#define	MC_CM		(1 << 13)		/* hpna 1.0 compat mode carrier test enable */
#define	MC_CE		(1 << 14)		/* cs enable */
#define	MC_GU		(1 << 15)		/* gain update */

/* modem debug */
#define	MD_DT		(1 << 0)		/* debug trigger enable */
#define	MD_RF_MASK	0xe			/* receive format */
#define	MD_RF_SAMP	0x0			/* samples */
#define	MD_RF_SPLIT	0x2			/* splitter */
#define	MD_RF_BYTES	0x4			/* bytes */
#define	MD_FB		(1 << 4)		/* force bits per baud */
#define	MD_TB_MASK	0x1e0			/* test bits per baud */
#define	MD_TA_MASK	0xe00			/* transmit amplifier */
#define	MD_RA_MASK	0x7000			/* rfi alpha */
#define	MD_DV		(1 << 15)		/* debug valid */

/* carrier detect control */
#define	CDC_CD_MASK	0xff			/* cd offset */
#define	CDC_CT_MASK	0xff00			/* cd threshold */

/* noise threshold */
#define	NT_NL_MASK	0xff			/* noise low */
#define	NT_NH_MASK	0xff00			/* noise high */
#define	NT_NH_SHIFT	8

/* frame status */
#define	FS_LS		((uint32)1 << 0)	/* link integrity state */

/* frame control */
#define	FC_LA_MASK	0x3f			/* last valid mac address */
#define	FC_FE		((uint32)1 << 6)	/* address filtering enable */
#define	FC_HE		((uint32)1 << 7)	/* hash filter enable */
#define	FC_KS		((uint32)1 << 8)	/* keep short */
#define	FC_CE		((uint32)1 << 9)	/* keep header crc error frames */
#define	FC_FT		((uint32)1 << 10)	/* keep header error frames */
#define	FC_RE		((uint32)1 << 11)	/* keep header rate error frames */
#define	FC_CG8		((uint32)1 << 12)	/* header crc8 generation enable */
#define	FC_CG16		((uint32)1 << 13)	/* trailer crc16 generation enable */
#define	FC_CG32		((uint32)1 << 14)	/* trailer crc32 generation enable */
#define	FC_SI		((uint32)1 << 15)	/* random scrambler initialization enable */
#define	FC_PE		((uint32)1 << 16)	/* transmit pad enable */
#define	FC_LI		((uint32)1 << 17)	/* link integrity enable */
#define	FC_LC_MASK	0x7c0000		/* link counter select */
#define	FC_LC_SHIFT	18
#define	FC_LC_DEFAULT	((uint32)0x1a << FC_LC_SHIFT)
#define	FC_PP		((uint32)1 << 23)	/* pulse pme */
#define	FC_XB		((uint32)1 << 24)	/* transmit bypass enable */
#define	FC_RB		((uint32)1 << 25)	/* receive bypass enable */
#define	FC_LE		((uint32)1 << 26)	/* loopback enable */
#define	FC_WE		((uint32)1 << 27)	/* wakeup encap enable (4220) */
#define	FC_LP_MASK	0x70000000		/* link integ pri (4220) */

/* maccontrol */
#define	MAC_CL_MASK	0xff			/* collision limit */

/* gpio output/enable/input */
#define	GPIO_MASK	0x7ff			/* output/enable/input */

/* transmit fifo control */
#define	XFC_BV_MASK	0x3			/* bytes valid */
#define	XFC_LO		(1 << 0)		/* low byte valid */
#define	XFC_HI		(1 << 1)		/* high byte valid */
#define	XFC_BOTH	(XFC_HI | XFC_LO)	/* both bytes valid */
#define	XFC_EF		(1 << 2)		/* end of frame */
#define	XFC_FR		(1 << 3)		/* frame ready */
#define	XFC_H1		(1 << 4)		/* hpna 1.0 compatibility */
#define	XFC_FL		(1 << 5)		/* flush request */
#define	XFC_FP		(1 << 6)		/* flush pending */

/* receive fifo control */
#define	RFC_FR		(1 << 0)		/* frame ready */
#define	RFC_DR		(1 << 1)		/* data ready */

/* msiintstatus and msiintmask */
#define	MSII_EC		(1 << 0)		/* excessive collisions error */
#define	MSII_XH		(1 << 1)		/* transmit header error */
#define	MSII_RO		(1 << 2)		/* receive fifo overflow */
#define	MSII_RI		(1 << 3)		/* receive notification interrupt */
#define	MSII_XI		(1 << 4)		/* transmit notification interrupt */
#define	MSII_TO		(1 << 5)		/* timeout */

/* msi counter address (4210/4211 only - discontinued in 4220) */
#define	MSICA_CL	0x0			/* collisions low */
#define	MSICA_CH	0x2			/* collisions high */
#define	MSICA_CFL	0x4			/* collision frames low */
#define	MSICA_CFH	0x6			/* collision frames high */
#define	MSICA_SF	0x8			/* short frame */
#define	MSICA_LF	0xc			/* long frame */
#define	MSICA_FFL	0x10			/* filtered frames low */
#define	MSICA_FFH	0x12			/* filtered frames high */
#define	MSICA_MFL	0x14			/* missed frames low */
#define	MSICA_MFH	0x16			/* missed frames high */
#define	MSICA_EFL	0x18			/* error frames low */
#define	MSICA_EFH	0x1a			/* error frames high */
#define	MSICA_H1	0x1c			/* hpna 1.0 link received */
#define	MSICA_RTS	0x24			/* receive too short */
#define	MSICA_TLI	0x28			/* transmit link integrity */
#define	MSICA_H1LC	0x2c			/* hpna 1.0 late collision */
#define	MSICA_IV	0x30			/* ifg violation */
#define	MSICA_SV	0x34			/* signal violation */
#define	MSICA_SB	0x38			/* stack bubble */

/* funcevnt, funcevntmask, funcstate, and funcforce */
#define	F_RD		((uint32)1 << 1)	/* card function ready */
#define	F_GW		((uint32)1 << 4)	/* general wakeup */
#define	F_WU		((uint32)1 << 14)	/* wakeup mask */
#define	F_IR		((uint32)1 << 15)	/* interrupt request pending */

/* interrupt receive lazy */
#define	IRL_TO_MASK	0x00ffffff		/* timeout */
#define	IRL_FC_MASK	0xff000000		/* frame count */
#define	IRL_FC_SHIFT	24			/* frame count */

/* transmit channel control */
#define	XC_XE		((uint32)1 << 0)	/* transmit enable */
#define	XC_SE		((uint32)1 << 1)	/* transmit suspend request */
#define	XC_LE		((uint32)1 << 2)	/* dma loopback enable */
#define	XC_FL		((uint32)1 << 4)	/* flush request */
#define	XC_MS		((uint32)1 << 15)	/* most-significant bit */

/* transmit descriptor table pointer */
#define	XP_LD_MASK	0xfff			/* last valid descriptor */

/* transmit channel status */
#define	XS_CD_MASK	0x0fff			/* current descriptor pointer */
#define	XS_XS_MASK	0xf000			/* transmit state */
#define	XS_XS_SHIFT	12
#define	XS_XS_DISABLED	0x0000			/* disabled */
#define	XS_XS_ACTIVE	0x1000			/* active */
#define	XS_XS_IDLE	0x2000			/* idle wait */
#define	XS_XS_STOPPED	0x3000			/* stopped */
#define	XS_XS_SUSP	0x4000			/* suspended */
#define	XS_XE_MASK	0xf0000			/* transmit errors */
#define	XS_XE_SHIFT	16
#define	XS_XE_NOERR	0x00000			/* no error */
#define	XS_XE_DPE	0x10000			/* descriptor protocol error */
#define	XS_XE_DFU	0x20000			/* data fifo underrun */
#define	XS_XE_PCIBR	0x30000			/* pci bus error on buffer read */
#define	XS_XE_PCIDA	0x40000			/* pci bus error on descriptor access */
#define	XS_FL		((uint32)1 << 20)	/* flushed */

/* receive channel control */
#define	RC_RE		((uint32)1 << 0)	/* receive enable */
#define	RC_RO_MASK	0x7e			/* receive frame offset */
#define	RC_RO_SHIFT	1
#define	RC_FM		((uint32)1 << 8)	/* direct fifo receive mode */
#define	RC_MS		((uint32)1 << 15)	/* most-significant bit */

/* receive descriptor table pointer */
#define	RP_LD_MASK	0xfff			/* last valid descriptor */

/* receive channel status */
#define	RS_CD_MASK	0x0fff			/* current descriptor pointer */
#define	RS_RS_MASK	0xf000			/* receive state */
#define	RS_RS_SHIFT	12
#define	RS_RS_DISABLED	0x0000			/* disabled */
#define	RS_RS_ACTIVE	0x1000			/* active */
#define	RS_RS_IDLE	0x2000			/* idle wait */
#define	RS_RS_STOPPED	0x3000			/* reserved */
#define	RS_RE_MASK	0xf0000			/* receive errors */
#define	RS_RE_SHIFT	16
#define	RS_RE_NOERR	0x00000			/* no error */
#define	RS_RE_DPE	0x10000			/* descriptor protocol error */
#define	RS_RE_DFO	0x20000			/* data fifo overflow */
#define	RS_RE_PCIBW	0x30000			/* pci bus error on buffer write */
#define	RS_RE_PCIDA	0x40000			/* pci bus error on descriptor access */

/*
 * Tut Mac Register Indirect Addresses (via hpnaaddr/hpnadata)
 */
#define	TUTCTL		0x400			/* control */
#define	TUTAIDCW	0x401			/* access identifier/control word */
#define	TUTAIDS		0x402			/* access identifier squelch */
#define	TUTDT		0x403			/* demod threshold */
#define	TUTMC		0x404			/* master command */
#define	TUTTDCST	0x405			/* tx data carrier sense timeout */
#define	TUTTAIDCST	0x406			/* tx access id carrier sense timeout */
#define	TUTB		0x407			/* bypass */
#define	TUTDS		0x408			/* data squelch */
#define	TUTSID		0x409			/* squelch increment/decrement */
#define	TUTSCTL		0x40a			/* squelch control */
#define	TUTNCTL		0x40b			/* noise control */
#define	TUTIFSC1	0x40c			/* interframe space count 1 */
#define	TUTIFSC2	0x40d			/* interframe space count 2 */
#define	TUTS1C		0x40e			/* slot 1 count */
#define	TUTSC		0x40f			/* slot count */
#define	TUTDCST		0x410			/* data carrier sense timeout */
#define	TUTAIDCST	0x411			/* access id carrier sense timeout */
#define	TUTTGDCST	0x412			/* tx gapped data carrier sense timeout */
#define	TUTTGAIDCST	0x413			/* tx gapped access id carrier sense timeout */
#define	TUTGAIDS	0x414			/* gapped access id stop */
#define	TUTGMS		0x415			/* gapped mapper start */
#define	TUTSS		0x416			/* squelch status (new in 4211) */

/* tut control */
#define	TC_SR		(1 << 0)		/* force slow rx */
#define	TC_FR		(1 << 1)		/* force fast rx */
#define	TC_LB		(1 << 2)		/* loopback */
#define	TC_DR		(1 << 3)		/* force data rx */
#define	TC_SW		(1 << 4)		/* use software header */
#define	TC_AP_MASK	0xe0			/* rx aid pll control */
#define	TC_AP_SHIFT	5
#define	TC_DP_MASK	0x700			/* rx data pll control */
#define	TC_DP_SHIFT	8
#define	TC_SM		(1 << 11)		/* slicer mode */
#define	TC_SC_MASK	0x3000			/* data slice control */
#define	TC_SC_SHIFT	12

/* tut aid/cw */
#define	TACW_AI_MASK	0xff			/* aid */
#define	TACW_CW_MASK	0xf00			/* control word for tut tx */
#define	TACW_CW_SHIFT	8
#define	TACW_GC_MASK	0xf000			/* gapped control word */
#define	TACW_GC_SHIFT	12

/* tut aid squelch */
#define	TA_AS_MASK	0x3ff			/* aid squelch */

/* tut demod threshold */
#define	TDT_GT_MASK	0x3ff			/* gain threshold */
#define	TDT_CT_MASK	0xfc00			/* collision threshold */
#define	TDT_CT_SHIFT	10

/* tut master command */
#define	TMC_MC_MASK	0xf			/* master command */
#define	TMC_SM		(1 << 15)		/* send master command */

/* tut transmit data carrier sense timeout */
#define	TDCST_TO_MASK	0x3ff			/* timeout */

/* tut transmit aid carrier sense timeout */
#define	TACST_TO_MASK	0x3ff			/* timeout */

/* tut bypass */
#define	TB_CE		(1 << 0)		/* collision enable */
#define	TB_RP		(1 << 1)		/* random phase enable */
#define	TB_CS		(1 << 2)		/* carrier sense defer enable */
#define	TB_FG		(1 << 3)		/* fine gain enable */
#define	TB_SA		(1 << 4)		/* squelch adjust enable */
#define	TB_SE		(1 << 5)		/* slicer enable */
#define	TB_EB		(1 << 6)		/* envelope detector bypass */
#define	TB_RB_MASK	0x180			/* rx bypass */
#define	TB_RB_SHIFT	7
#define	TB_FC		(1 << 9)		/* fast carrier sense enable */

/* tut data squelch */
#define	TDS_DS_MASK	0x3ff			/* data squelch */

/* tut squelch inc/dec */
#define	TSID_SI_MASK	0xf			/* aid squelch increment */
#define	TSID_SD_MASK	0x30			/* aid squelch level delta */
#define	TSID_SD_SHIFT	4

/* tut squelch control */
#define	TSCTL_SI_MASK	0x3			/* squelch interval control */
#define	TSCTL_SW_MASK	0xc			/* squelch watchdog interval control */
#define	TSCTL_SW_SHIFT	2
#define	TSCTL_MN_MASK	0x30			/* max noise events */
#define	TSCTL_MN_SHIFT	4
#define	TSCTL_LP_MASK	0xc0			/* long packet control */
#define	TSCTL_LP_SHIFT	6
#define	TSCTL_SE	(1 << 8)		/* squelch enable */
#define	TSCTL_SO	(1 << 9)		/* squelch override */
#define	TSCTL_WE	(1 << 10)		/* squelch watchdog enable */

/* tut noise control */
#define	TNCTL_NF_MASK	0xff			/* noise floor */
#define	TNCTL_NC_MASK	0xff00			/* noise ceiling */
#define	TNCTL_NC_SHIFT	8

/* tut ifs count1 */
#define	TIFS1_IC_MASK	0xfff			/* count in clk32 of first 2/3 of ifg */

/* tut ifs count2 */
#define	TIFS2_IC_MASK	0xfff			/* count in clk32 of second 1/3 of ifg */

/* tut data carrier sense timeout */
#define	TDCST_TO_MASK	0x3ff			/* timeout */

/* tut aid carrier sense timeout */
#define	TACST_TO_MASK	0x3ff			/* timeout */

/* tut transmit gapped data carrier sense timeout */
#define	TTGDCST_TO_MASK	0x3ff			/* timeout */

/* tut transmit gapped aid carrier sense timeout */
#define	TTGACST_TO_MASK	0x3ff			/* timeout */

/* tut gapped aid stop */
#define	TGAIDS_GS_MASK	0x3ff			/* timeout */

/* tut gapped mapper start */
#define	TGMS_DL_MASK	0xfff			/* delay */

/*
 * Hardware mandated srom definitions
 *
 * Offsets and sizes are all in words.
 */

/* Hardware area starts at word 0 */
#define	SPROM_HW	0

/* Hardware control is word 0 */
#define	SPROM_HWCTL	0

/* Bits in the Hardware control word: */
#define	HWC_LOCKHW	0x0001			/* Lock the hardware part of the sprom */
#define	HWC_LOCKSYS	0x0002			/* Lock the System part of the sprom */
#define	HWC_LOCK	0x0004			/* Lock the rest of the sprom */
#define	HWC_LOCKMAC	0x0100			/* Lock the MAC address */

/* Power management and expansion ROM */
#define	SPROM_PMX	1

/* Power management and expansion ROM bits: */
#define	PMX_XSZ_MASK	0x000f			/* Expansion ROM size: 2 ^(9 + x) */
#define	PMX_XROM	0x0010			/* Expansion ROM present */
#define	PMX_XXL		0x0020			/* Expansion ROM eXternal Latch */
#define	PMX_CST_MASK	0x01c0			/* Expansion ROM CS timing */
#define	PMX_CST_SHIFT	6
#define	PMX_D3PMEAUTO	0x0200			/* D3cold PME-disabled Auto Off */
#define	PMX_REPVAUX	0x0400			/* Report availability of Vaux on D3cold */
#define	PMX_REP_MASK	0xf800			/* Report bits for each PME state */
#define	PMX_REP_SHIFT	11

/* Subsystem Id */
#define	SPROM_SSID	2

/* Subsystem vendor Id */
#define	SPROM_SSVID	3

/* AFE Revision Id / Supported BPB */
#define	SPROM_AFEBPB	4

/* Fileds in the AFE Revision Id / Supported BPB word */
#define	AB_AFE_MASK	0x00ff			/* AFE Revision Id */
#define	AB_BPB_MASK	0xff00			/* Supported BPB bits */
#define	AB_BPB_SHIFT	8
#define	AB_BPB_DEF	0xfe00			/* Default supported BPB: 8-2 */

/* MAC address low/mid/hi */
#define	SPROM_MACLO	5
#define	SPROM_MACMID	6
#define	SPROM_MACHI	7

/* 4211 Device ID and Enable & Power Management bits */
#define SPROM_4211_DID      12
#define SPROM_4211_ENPWR    13

/* Number of words the HW lock bit covers */
#define	SPROM_HW_SZ	16

/* Start of System Configuration Area: */
/*  First word is board revision */
#define	SPROM_SYS	24
#define	SPROM_SYS_SZ	8

/* High byte is crc for first 32 words */
#define	SPROM_CRC1	31

/* Size of the Hardware + System Areas */
#define	SPROM_HWSYS_SZ	32
#define	SPROM_HWSYS_BYTES (SPROM_HWSYS_SZ * 2)
#define	SPROM_AREA_1_OFFSET	0
#define	SPROM_AREA_1_SIZE	SPROM_HWSYS_BYTES

/* Start of OEM Data Area */
#define	SPROM_OEM	32
#define	SPROM_OEM_SZ	32
#define	SPROM_OEM_BYTES	(SPROM_OEM_SZ * 2)
#define	SPROM_AREA_2_OFFSET	(SPROM_OEM * 2)
#define	SPROM_AREA_2_SIZE	SPROM_OEM_BYTES

/* High byte is crc for words 32-64 */
#define	SPROM_CRC2	63

/* Size of the 93c46 on all PCI nics.
 * XXX: cbnic's will have 93c66 which are
 * 4 times as big!
 */
#define	SPROM_SIZE	64
#define	SPROM_BYTES	(SPROM_SIZE * 2)


/*
 * Transmit/Receive Ring Descriptor
 */
typedef volatile struct {
	uint32	ctrl;		/* misc control bits & bufcount */
	uint32	addr;		/* data buffer address */
} bcm42xxdd_t;

#define	CTRL_BC_MASK	0x1fff			/* buffer byte count */
#define	CTRL_TSL	((uint32)1 << 25)	/* timestamp latch (4220) */
#define	CTRL_COMPAT	((uint32)1 << 26)	/* hpna s1 compatibility data */
#define	CTRL_SAMPDATA	((uint32)1 << 27)	/* sample data */
#define	CTRL_EOT	((uint32)1 << 28)	/* end of descriptor table */
#define	CTRL_IOC	((uint32)1 << 29)	/* interrupt on completion */
#define	CTRL_EOF	((uint32)1 << 30)	/* end of frame */
#define	CTRL_SOF	((uint32)1 << 31)	/* start of frame */

/*
 * Receive Frame Data Header for DT_ILINE frames
 */
typedef struct {
	uint16	ctl1;			/* framelen low */
	uint16	ctl2;			/* framelen high & datatype */
	uint16	ctl3;			/* hdrcv & paycv low */
	uint16	ctl4;			/* paycv high thru pcrc */
	uint16	ctl5;			/* rxgain thru rxpri */
	uint16	symcount;		/* payload symbol count */
	int16	freqest;		/* initial timing frequency estimate */
	int16	freqacc;		/* tracker timing frequency accumulator */
	uint16	hdrmaxerr;		/* header max squared error */
	uint16	paymaxerr;		/* payload max squared error */

	/* added in 4220 */
	uint8	isipwr;			/* isi 2mbaud units = db below signal power */
	uint8	isipwr4;		/* isi 4mbaud units = db below signal power */
	uint8	ffepwr;			/* sum of squared 2mbaud ffe coeffs (in db) */
	uint8	ffepwr4;		/* sum of squared 4mbaud ffe coeffs (in db) */
	uint32	timestamp;		/* receive timestamp */
} bcm42xxrxhdr_t;

/*
 * 4210 and 4211 have a 20byte iline rxhdr.
 * 4220 and followons support 4Mbaud and have a 28byte iline rxhdr.
 */
#define	RXHDR_2MBAUD_LEN	20	/* 20byte 2mbaud sizeof bcm42xxrxhdr_t */
#define	RXHDR_4MBAUD_LEN	28	/* 28byte 4mbaud sizeof bcm42xxrxhdr_t (4220) */

/* frame length */
/* XXX: If and when we have chips able to receive more than 64KBytes
 * in a single frame, we can start paying attention to the high
 * 2 bits as in:
 *	#define	RX_FRAMELEN_HI_MASK	0x3
 *	#define	FRAMELEN(h)		((uint32)((h)->ctl1)
 *			| (((uint32)((h)->ctl2) & RX_FRAMELEN_HI_MASK) << 16))
 */
#define RX_FRAMELEN_HI_MASK  0x3
#define RX_FRAMELEN_HI_SHIFT 16

#define	FRAMELEN(h)		((h)->ctl1)
#define SET_FRAMELEN(h, framelen) \
    ((h)->ctl2) &= ~RX_FRAMELEN_HI_MASK; \
    ((h)->ctl2) |= (((framelen)>>RX_FRAMELEN_HI_SHIFT) & RX_FRAMELEN_HI_MASK); \
    ((h)->ctl1) = (framelen); \

/* resv1 */
#define RX_RESV1_MASK   0x0ffc
#define RX_RESV1_SHIFT  2 
#define RESV1(h)        ((((h)->ctl2) & RX_RESV1_MASK) >> RX_RESV1_SHIFT)
#define SET_RESV1(h, resv1) \
    ((h)->ctl2) &= ~RX_RESV1_MASK; \
    ((h)->ctl2) |= (((resv1) << RX_RESV1_SHIFT) & RX_RESV1_MASK)

/* datatype */
#define	RX_DT_MASK		0xf000
#define	RX_DT_SHIFT		12
#define	DATATYPE(h)		((((h)->ctl2) & RX_DT_MASK) >> RX_DT_SHIFT)
#define SET_DATATYPE(h, datatype) \
    ((h)->ctl2) &= ~RX_DT_MASK; \
    ((h)->ctl2) |= (((datatype) << RX_DT_SHIFT) & RX_DT_MASK)

/* hdrcv */
#define	RX_HDRCV_MASK		0x1ff
#define	HDRCV(h)		(((h)->ctl3) & RX_HDRCV_MASK)
#define SET_HDRCV(h, hdrcv) \
    ((h)->ctl3) &= ~RX_HDRCV_MASK; \
    ((h)->ctl3) |= ((hdrcv) & RX_HDRCV_MASK)

/* paycv */
#define	RX_PAYCV_LO_MASK	0xfe00
#define	RX_PAYCV_LO_SHIFT	9
#define	RX_PAYCV_HI_MASK	0x3
#define	RX_PAYCV_HI_SHIFT	7
#define	PAYCV(h)		(((((h)->ctl4) & RX_PAYCV_HI_MASK) << RX_PAYCV_HI_SHIFT) \
				| ((((h)->ctl3) & RX_PAYCV_LO_MASK) >> RX_PAYCV_LO_SHIFT))
#define SET_PAYCV(h, paycv) \
    ((h)->ctl4) &= ~RX_PAYCV_HI_MASK; \
    ((h)->ctl4) |= (((paycv) >> RX_PAYCV_HI_SHIFT) & RX_PAYCV_HI_MASK); \
    ((h)->ctl3) &= ~RX_PAYCV_LO_MASK; \
    ((h)->ctl3) |= (((paycv) << RX_PAYCV_LO_SHIFT) & RX_PAYCV_LO_MASK)

/* noise */
#define	RX_NOISE_MASK		0x7fc
#define	RX_NOISE_SHIFT		2
#define	NOISE(h)		((((h)->ctl4) & RX_NOISE_MASK) >> RX_NOISE_SHIFT)
#define SET_NOISE(h, noise) \
    ((h)->ctl4) &= ~RX_NOISE_MASK; \
    ((h)->ctl4) |= (((noise) << RX_NOISE_SHIFT) & RX_NOISE_MASK)

/* resv2 */
#define RX_RESV2_MASK       0x1800
#define RX_RESV2_SHIFT      11
#define RESV2(h)        ((((h)->ctl4) & RX_RESV2_MASK) >> RX_RESV2_SHIFT)
#define SET_RESV2(h, resv2) \
    ((h)->ctl4) &= ~RX_RESV2_MASK; \
    ((h)->ctl4) |= (((resv2) << RX_RESV2_SHIFT) & RX_RESV2_MASK)

/* fterr */
#define	RX_FTERR_MASK		0x2000
#define	RX_FTERR_SHIFT		13
#define	FTERR(h)		((((h)->ctl4) & RX_FTERR_MASK) >> RX_FTERR_SHIFT)
#define SET_FTERR(h, fterr) \
    ((h)->ctl4) &= ~RX_FTERR_MASK; \
    ((h)->ctl4) |= (((fterr) << RX_FTERR_SHIFT) & RX_FTERR_MASK)

/* hcrc */
#define	RX_HCRC_MASK		0x4000
#define	RX_HCRC_SHIFT		14
#define	HCRC(h)			((((h)->ctl4) & RX_HCRC_MASK) >> RX_HCRC_SHIFT)
#define SET_HCRC(h, hcrc) \
    ((h)->ctl4) &= ~RX_HCRC_MASK; \
    ((h)->ctl4) |= (((hcrc) << RX_HCRC_SHIFT) & RX_HCRC_MASK)

/* pcrc */
#define	RX_PCRC_MASK		0x8000
#define	RX_PCRC_SHIFT		15
#define	PCRC(h)			((((h)->ctl4) & RX_PCRC_MASK) >> RX_PCRC_SHIFT)
#define SET_PCRC(h, pcrc) \
    ((h)->ctl4) &= ~RX_PCRC_MASK; \
    ((h)->ctl4) |= (((pcrc) << RX_PCRC_SHIFT) & RX_PCRC_MASK)

/* rxgain */
#define	RX_RXGAIN_MASK		0xff
#define	RXGAIN(h)		(((h)->ctl5) & RX_RXGAIN_MASK)
#define SET_RXGAIN(h, rxgain) \
    ((h)->ctl5) &= ~RX_RXGAIN_MASK; \
    ((h)->ctl5) |= ((rxgain) & RX_RXGAIN_MASK)

/* rterr */
#define	RX_RTERR_MASK		0x100
#define	RX_RTERR_SHIFT		8
#define	RTERR(h)		((((h)->ctl5) & RX_RTERR_MASK) >> RX_RTERR_SHIFT)
#define SET_RTERR(h, rterr) \
    ((h)->ctl5) &= ~RX_RTERR_MASK; \
    ((h)->ctl5) |= (((rterr) << RX_RTERR_SHIFT) & RX_RTERR_MASK)

/* trkerr */
#define	RX_TRKERR_MASK		0x200
#define	RX_TRKERR_SHIFT		9
#define	TRKERR(h)		((((h)->ctl5) & RX_TRKERR_MASK) >> RX_TRKERR_SHIFT)
#define SET_TRKERR(h, trkerr) \
    ((h)->ctl5) &= ~RX_TRKERR_MASK; \
    ((h)->ctl5) |= (((trkerr) << RX_TRKERR_SHIFT) & RX_TRKERR_MASK)

/* satdet */
#define	RX_SATDET_MASK		0x400
#define	RX_SATDET_SHIFT		10
#define	SATDET(h)		((((h)->ctl5) & RX_SATDET_MASK) >> RX_SATDET_SHIFT)
#define SET_SATDET(h, satdet) \
    ((h)->ctl5) &= ~RX_SATDET_MASK; \
    ((h)->ctl5) |= (((satdet) << RX_SATDET_SHIFT) & RX_SATDET_MASK)

/* csover */
#define	RX_CSOVER_MASK		0x800
#define	RX_CSOVER_SHIFT		11
#define	CSOVER(h)		((((h)->ctl5) & RX_CSOVER_MASK) >> RX_CSOVER_SHIFT)
#define SET_CSOVER(h, csover) \
    ((h)->ctl5) &= ~RX_CSOVER_MASK; \
    ((h)->ctl5) |= (((csover) << RX_CSOVER_SHIFT) & RX_CSOVER_MASK)

/* compat */
#define	RX_COMPAT_MASK		0x1000
#define	RX_COMPAT_SHIFT		12
#define	COMPAT(h)		((((h)->ctl5) & RX_COMPAT_MASK) >> RX_COMPAT_SHIFT)
#define SET_COMPAT(h, compat) \
    ((h)->ctl5) &= ~RX_COMPAT_MASK; \
    ((h)->ctl5) |= (((compat) << RX_COMPAT_SHIFT) & RX_COMPAT_MASK);

/* rxpri */
#define	RX_RXPRI_MASK		0xe000
#define	RX_RXPRI_SHIFT		13
#define	RXPRI(h)		((((h)->ctl5) & RX_RXPRI_MASK) >> RX_RXPRI_SHIFT)
#define SET_RXPRI(h, rxpri) \
    ((h)->ctl5) &= ~RX_RXPRI_MASK; \
    ((h)->ctl5) |= (((rxpri) << RX_RXPRI_SHIFT) & RX_RXPRI_MASK)

/*
 * Receive Frame Data Header for DT_TUT frames
 */
typedef struct {
	uint16	ctl1;			/* framelen low */
	uint16	ctl2;			/* framelen high & datatype */
	uint16	ctl3;			/* aid */
	uint16	ctl4;			/* pcrc */
	uint16	ctl5;			/* rxgain, satdet, csover and cw */
	uint16	resv1;			/* 0 */
	uint16	squelch;		/* pre-gain aid squelch level */
	uint16	datasquelch;		/* data squelch level */
	uint16	aidsquelch;		/* post-gain aid squelch level */
	uint16	resv2;			/* 0 */
} bcm42xxv1hdr_t;

/* aid */
#define	RX_AID_MASK		0xff
#define	AID(h)			(((h)->ctl3) & RX_AID_MASK)
#define SET_AID(h, aid) \
    ((h)->ctl3) &= ~RX_AID_MASK; \
    ((h)->ctl3) |= ((aid) & RX_AID_MASK)

/* cw */

#define	RX_CW_MASK		0xf000
#define	RX_CW_SHIFT		12
#define	CW(h)			((((h)->ctl5) & RX_CW_MASK) >> RX_CW_SHIFT)
#define SET_CW(h, cw) \
    ((h)->ctl5) &= ~RX_CW_MASK; \
    ((h)->ctl5) |= (((cw) << RX_CW_SHIFT) & RX_CW_MASK)

/* datatype */
#define	DT_SAMP		0		/* raw samples */
#define	DT_ILINE	1		/* iLine10 demodulated frame */
#define	DT_TUT		2		/* HPNA 1.0 demodulated frame */


/*
 * ISB DMA Registers
 */
typedef volatile struct {
	uint32  config;			/* channel configuration */
	uint32	maxburst;		/* DMA burst length (32-bit word count) */
	uint32	buffaddr;		/* buffer addr or descriptor ring base addr */
	uint32	bcntrlen;		/* byte count or ring length */
	uint32	buffstat;		/* non-chaining status word */

	/* Interrupt Control */
	uint32	intstat;
	uint32	intmask;
	
	/* Ethernet Flow Control */
	uint32	fcthr;			/* flow control desciptor threshold */
	uint32	buffalloc;		/* flow control buffer allocation */
	uint32	PAD[5];
} isbdma_regs_t;

/* ISB intstatus and intmask */
#define I_BDONE		((uint32)1 << 0)	/* DMA finished a buffer */
#define I_PDONE		((uint32)1 << 1)	/* DMA finished last buffer of a packet */
#define	I_NOTVLD	((uint32)1 << 2)	/* DMA read a desc without OB set */

/* ISB ethernet flow control */
#define ISB_FCTHR_MASK		0xffff
#define ISB_BUFFALLOC_MASK	0xffff

/*
 * ISB Transmit/Receive Ring Descriptor
 */
typedef volatile struct {
	uint16	status;				/* flag/status info bits */
	uint16	len;				/* data buffer byte length */
	uint32	addr;				/* data buffer address */
} isbdd_t;

#define ISB_STATUS_OB	((uint16)1 << 15)	/* Ownership Bit */
#define ISB_STATUS_L	((uint16)1 << 11)	/* Last buffer */
#define ISB_STATUS_F	((uint16)1 << 10)	/* First buffer */
#define ISB_STATUS_W	((uint16)1 <<  9)	/* Wrap (last descriptor) */
#define ISB_LEN_MASK	0xfff

#endif	/* _BCM42XX_H */
