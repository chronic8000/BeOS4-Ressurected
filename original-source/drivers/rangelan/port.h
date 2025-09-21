/***************************************************************************/
/*																									*/
/*						  	PROXIM RANGELAN2 LOW LEVEL DRIVER							*/
/*																									*/
/*		PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the 		*/
/*		sole property of PROXIM, INC.  Reproduction or utilization of			*/
/*		this source in whole or in part is forbidden without the written		*/
/*		consent of PROXIM, INC.																*/
/*																									*/
/*																									*/
/*							(c) Copyright PROXIM, INC. 1994								*/
/*									All Rights Reserved										*/
/*																									*/
/***************************************************************************/

/***************************************************************************/
/*																									*/
/*	$Header:   J:\pvcs\archive\clld\port.h_v   1.16   14 Oct 1996 10:34:38   BARBARA  $	*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\port.h_v  $											*/
/* 
/*    Rev 1.16   14 Oct 1996 10:34:38   BARBARA
/* R_CtrlReg() declaration added.
/* 
/*    Rev 1.15   27 Sep 1996 09:20:30   JEAN
/* 
/*    Rev 1.14   24 Sep 1996 16:54:30   BARBARA
/* Changes related to the FastIO which does not apply to 1pc anymore.
/* 
/*    Rev 1.13   15 Aug 1996 10:09:36   JEAN
/* 
/*    Rev 1.12   29 Jul 1996 12:36:26   JEAN
/* 
/*    Rev 1.11   28 Jun 1996 18:09:04   BARBARA
/* One piece PCMCIA code has been added.
/* 
/*    Rev 1.10   14 Jun 1996 16:42:28   JEAN
/* Added a delay macro.
/* 
/*    Rev 1.9   06 Feb 1996 14:34:28   JEAN
/* Moved the PIC defines to constant.h.
/* 
/*    Rev 1.8   31 Jan 1996 13:42:10   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.7   08 Jan 1996 14:22:58   JEAN
/* Changed the PIC defines.
/* 
/*    Rev 1.6   12 Oct 1995 12:07:08   JEAN
/* Changed two constants to fix a  problem where the stayawake bit was being masked out 
/* by W_TxStatusReg and W_RxStatusReg.
/* 
/*    Rev 1.5   16 Mar 1995 16:20:40   tracie
/* Added support for ODI
/* 
/*    Rev 1.4   27 Dec 1994 15:46:36   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:34   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:32   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:44   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef PORT_H
#define PORT_H

/***************************************/
/* Add this macro to introduce a			*/
/* delay between register accesses.		*/
/***************************************/

#define IO_DELAY	_inpx(0x61);


/*********************************************************************/
/*																							*/
/* Equates for the Status Port.  TX and RX share the same status		*/
/* register (the lower 4 bits are for the TX and the upper 3 bits		*/
/* are for RX.  The most significant bit is for indicating if the		*/
/* card is in standby.																*/
/*																							*/
/*********************************************************************/

#define	TXSTATMASK		0x8F				/* Tx state machine status mask		*/
#define	RXSTATMASK		0xF0				/* Rx state machine status mask		*/
#define	STANDBYBIT		0x80				/* The standby bit						*/
#define	STAYAWAKE		0x80				/* Set card to not sleep				*/

#define	STATTX_RDY		0					/* Rdy issued after error recovery	*/
#define	STATTX_REQ		1					/* Final request to RFNC				*/
#define	STATTX_ACK		2					/* Final ACK:  RFNC ready to RX		*/
#define	STATTX_EOT		3					/* Transmission complete				*/
#define	STATTX_DAT		4					/* In the send data state				*/
#define	STATTX_NAK		5					/* No buffers available					*/
#define	STATTX_REQ1		6					/* 1st request (byte mode xfer)		*/
#define	STATTX_ACK1		7					/* 1st ACK (byte mode xfer)			*/
#define	STATTX_CNAK		8					/* Clear NAK command						*/

#define	STATRX_RDY		STATTX_RDY*16	/* Rdy issued after error recovery	*/
#define	STATRX_REQ		STATTX_REQ*16	/* Final request from RFNC				*/
#define	STATRX_ACK		STATTX_ACK*16	/* Final ACK:  PC ready to receive	*/
#define	STATRX_EOT		STATTX_EOT*16	/* Transmission complete				*/
#define	STATRX_DAT		STATTX_DAT*16	/* In the send data state				*/
#define	STATRX_NAK		STATTX_NAK*16	/* No buffers available					*/
#define	STATRX_REQ1		STATTX_REQ1*16	/* 1st req from RFNC (byte mode)		*/
#define	STATRX_ACK1		STATTX_ACK1*16	/* 1st ACK to RFNC (byte mode)		*/



/*********************************************************************/
/*																							*/
/*	Control Register Bit Definitions:											*/
/*		b6-7	Reserved																	*/
/*		b5	-	Transfer mode bit 1=16 bit mode, 0=8 bit mode				*/
/*		b4	-	Out of standby bit													*/
/*		b3	-	NMI RFNC bit															*/
/*		b2	-	Reset RFNC bit															*/
/*		b1	-	RX interrupt to RFNC													*/
/*		b0	-	TX interrupt to RFNC													*/
/*																							*/
/*********************************************************************/

#define	INT_RFNCBITTX	1					/* Interrupt for RFNC's Tx state		*/
#define	INT_RFNCBITRX	2					/* Interrupt for RFNC's Rx state		*/
#define	RESETRFNCBIT	4					/* Reset RFNC bit							*/
#define	NMIRFNCBIT		8					/* NMI RFNC bit							*/
#define	WAKEUPBIT		0x10				/* Out of standby bit					*/
#define	BUSMODEBIT		0x20				/* 8/16 (1=16) bit xfer mode bit		*/



/*********************************************************************/
/*																							*/
/*	Interrupt Enable Register Bit Definitions.								*/
/*		b4		Interrupt enable bit													*/
/*		b0-b3	Interrupt select bits												*/
/*																							*/
/*********************************************************************/

#define	INT_ENABLEBIT	0x10



/*********************************************************************/
/*																							*/
/* Equates used when resetting the RFNC.										*/
/*																							*/
/*********************************************************************/

#define	SIGNATUREWORD		0xAA55		/* For 16/8 bit test					*/
#define	SIGNATUREWORDCMP	0x55AA		/* For 16/8 bit test read back	*/
#define	RESETSTATUS			0x5A			/* RFNC reset status					*/
#define	READYSTATUS			0				/* RFNC ready status					*/


/*********************************************************************/
/*																							*/
/* Prototype definitions for file "port.c"									*/
/*																							*/
/*********************************************************************/

void				W_StatusReg (unsigned_8 Value);
void				SetBit_StatusReg (unsigned_8 Value);
void				ClearBit_StatusReg (unsigned_8 Value);
void				W_RxStatusReg (unsigned_8 Value);
void				W_TxStatusReg (unsigned_8 Value);
void				W_CtrlReg (unsigned_8 Value);
unsigned_8		R_CtrlReg (void);
void				SetBit_CtrlReg (unsigned_8 Value);
void				ClearBit_CtrlReg (unsigned_8 Value);
void				W_DataRegB (unsigned_8 Value);
void				W_DataRegW (unsigned_16 Value);
void				W_FastDataRegB (unsigned_8 Value);
void				W_FastDataRegW (unsigned_16 Value);
void				W_IntSelReg (unsigned_8 Value);
void				LLDOutOfStandby (void);
unsigned_8		R_Status (void);
unsigned_8		R_RxStatus (void);
unsigned_8		R_TxStatus (void);
unsigned_8		R_DataRegB (void);
unsigned_16		R_DataRegW (void);
unsigned_8		R_FastDataRegB (void);
unsigned_16		R_FastDataRegW (void);
void				R_EOIReg (void);
void				IntRFNCTx (void);
void				IntRFNCRx (void);
void				PushRFNCInt (void);
void				PopRFNCInt (void);
void				DisableRFNCInt (void);
void				EnableRFNCInt (void);
void				EOI (unsigned_8 irq);

#endif /* PORT_H */