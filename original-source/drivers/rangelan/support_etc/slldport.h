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
/*	$Header:   J:\pvcs\archive\clld\slldport.h_v   1.10   14 Jun 1996 16:45:46   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldport.h_v  $									  	*/
/* 
/*    Rev 1.10   14 Jun 1996 16:45:46   JEAN
/* Comment changes.
/* 
/*    Rev 1.9   22 Mar 1996 14:57:08   JEAN
/* Added defines for the BIOS table.
/* 
/*    Rev 1.8   06 Feb 1996 14:34:42   JEAN
/* Cleaned up the file.
/* 
/*    Rev 1.7   31 Jan 1996 13:42:32   JEAN
/* Added duplicate header include detection removed/added some
/* defines, and removed some unused function prototypes.
/* 
/*    Rev 1.6   08 Jan 1996 14:23:54   JEAN
/* Moved a PIC define to port.h
/* 
/*    Rev 1.5   17 Nov 1995 16:46:26   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 13:50:02   JEAN
/* Added function prototypes.
/* 
/*    Rev 1.3   13 Apr 1995 08:52:54   tracie
/* xromtest version and NS16550 FIFO mode
/* 
/*    Rev 1.2   22 Feb 1995 10:40:02   tracie
/* 
/*    Rev 1.1   14 Dec 1994 10:49:46   tracie
/* minor changes made for this 8250 UART working version
/*																									*/
/*																									*/
/* Initial revision		23 Nov 1994		Tracie										*/
/*																									*/
/***************************************************************************/

#ifndef SLLDPORT_H
#define SLLDPORT_H


/*-------------------------------------------------------*/
/* Interrupt Enable Register 										*/
/*-------------------------------------------------------*/

#define	RXRDY_EN			1			/* Received Data Available	Enable			*/
#define	TBE_EN			2			/* Transmitter Buffer Empty Enable			*/
#define	LINESTAT_EN		4			/* Receiver Line Status Enable				*/
#define	MODEMSTAT_EN	8			/* Modem Status Enable							*/

/*-------------------------------------------------------*/
/* Data Format/Line Control Register							*/
/*-------------------------------------------------------*/

#define	DATABIT			3			/* Num of Data Bits: B0=1, B1=1				*/
#define	STOPBIT			4			/* B2 of Data Format/Line Ctrl Register	*/
#define	DATA8STOP1		(DATABIT & ~STOPBIT)	/* DATA=8, STOP=1					*/
#define	DLAB				0x80		/* Divisor Latch Access Bit					*/


/*-------------------------------------------------------*/
/* Modem Control Register											*/
/*-------------------------------------------------------*/
#define	DTR 				1			/* Writing a 1 asserts the DTR line			*/
#define	RTS				2			/* Writing a 1 asserts the RTS line			*/
#define 	GPO1				4			/* General purpose output # 1					*/
#define	GPO2				8			/* General purpose output # 2					*/
			
/*-------------------------------------------------------*/
/* Line Status Register												*/																	
/*-------------------------------------------------------*/
#define	RXRDY				0x01 		/* 1-incoming byte has been received		*/
#define	OVERRUN			0x02 		/* Overrun error									*/
#define	PARITY			0x04	  	/* Parity error									*/
#define	FRAMING			0x08	  	/* Framing error									*/
#define	BREAKDETECT		0x10		/* Break detect									*/
#define	THRE		 		0x20		/* Transmitter buffer empty					*/
#define	TXEMPTY			0x40		/* Transmitter empty								*/
#define	RXFIFOERR		0x80		/* Error in receive FIFO - either an		*/
											/* overrun, parity, framing, or break		*/
											/* error occurred.	16550 FIFO only.		*/

#define	RXERROR	(OVERRUN | PARITY | FRAMING | BREAKDETECT)
#define	RXINT (RXRDY | OVERRUN | PARITY | FRAMING | BREAKDETECT)
#define	TXINT (THRE | TXEMPTY)


/*-------------------------------------------------------*/
/* Modem Status Register			  								*/
/*-------------------------------------------------------*/
#define	DELTA_CTS 		0x01		/* 1 means a change in state of CTS line.	*/
#define	DELTA_DSR 		0x02		/* 1 means a change in state of DSR	line.	*/
#define	DELTA_ERI		0x04		/* 1 means a change in state of ERI line.	*/
#define	DELTA_DCD		0x08		/* 1 means a change in state of DCD	line. */
#define	CTS_BIT			0x10		/* The state of CTS.								*/
#define	DSR_BIT			0x20		/* The state of DSR.								*/
#define	RI_BIT			0x40		/* The state of RI (ring indicator).		*/
#define	DCD_BIT			0x80		/* The state of DCD.								*/



/*----------------------------------*/
/* BIOS COM Port Address Locations	*/
/*----------------------------------*/

#define	BIOS_COM_BASE_ADDR	0x400
#define	COM_PORT_CNT_ADDR		0x410
#define	NUM_BIOS_COM_PORT		4 
#define	COM_PORT_CNT_BIT		9
#define	COM_PORT_CNT_MASK		0x0600


/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "slldport.c"	  				*/
/*																							*/
/*********************************************************************/

void 	SetBit_ModemCtrl (unsigned_8 Value);
void 	ClrBit_ModemCtrl (unsigned_8 Value);
void 	Clear_PendingUARTIntr (void);
void	EnableUARTModemCtrl (void);
void	DisableUARTModemCtrl (void);
void	EnableUARTRxInt (void);
void	EnableUARTRxTxInt (void);
void 	SetBaudRate (unsigned_16 Divisor);
void 	DisableUARTInt (void);
void 	EnableRTS(void);
void	DisableRTS(void);
void	PushRFNCInt (void);
void	PopRFNCInt (void);
void	DisableRFNCInt (void);
void	EnableRFNCInt (void);
void	EOI (unsigned_8 irq);
void	CheckBIOSMem (void);
void	RestoreBIOSMem (void);

#endif /* SLLDPORT_H */
