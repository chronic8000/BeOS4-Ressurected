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
/*																								 	*/
/*	$Header:   J:\pvcs\archive\clld\slld.h_v   1.13   14 Jun 1996 16:46:44   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slld.h_v  $											*/
/* 
/*    Rev 1.13   14 Jun 1996 16:46:44   JEAN
/* Comment changes.
/* 
/*    Rev 1.12   16 Apr 1996 11:44:00   JEAN
/* 
/*    Rev 1.11   14 Mar 1996 14:51:08   JEAN
/* Added a define for the maximum number of baudrates.
/* 
/*    Rev 1.10   08 Mar 1996 19:28:34   JEAN
/* Added defines for the different baudrates and added a function
/* prototype.
/* 
/*    Rev 1.9   06 Feb 1996 14:39:26   JEAN
/* Cleaned up the file.
/* 
/*    Rev 1.8   31 Jan 1996 13:52:00   Jean
/* Added duplicate header include detection and removed some
/* unused defines and function prototypes.
/* 
/*    Rev 1.7   17 Nov 1995 16:45:58   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.6   12 Oct 1995 13:41:26   JEAN
/* Added macros and function types.
/* 
/*    Rev 1.5   13 Apr 1995 08:52:20   tracie
/* xromtest version and NS16550 FIFO
/* 
/*    Rev 1.4   16 Mar 1995 16:21:16   tracie
/* Added support for ODI
/* 
/*    Rev 1.3   22 Feb 1995 10:39:54   tracie
/* 
/*    Rev 1.2   27 Dec 1994 15:46:52   tracie
/* 
/*    Rev 1.1   14 Dec 1994 10:48:28   tracie
/* Minor changes made for this 8250 UART working version
/*																									*/
/*																									*/
/* 																								*/
/* Initial revision	21 Nov 1994		Tracie											*/
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef SLLD_H
#define SLLD_H



#define	DATAHOLDING		0			/* UART's Base - Tx/Rcv Holding 			R0	*/
#define	INTRENABLE		1			/* offset of Interrupt enable register	R1	*/
#define	INTRIDENTIFY	2			/* offset of Interrupt ID Register		R2	*/
#define	FIFOCONTROL		2			/* FIFO Control Register (Write Only)  R2	*/
#define	LINECONTROL		3			/* offset of Line Control/Data Format  R3	*/
#define	MODEMCONTROL	4			/* offset of Modem Control/RS-232 Out	R4	*/
#define	LINESTATUS		5			/* offset of Line/Serialization Status	R5 */
#define	MODEMSTATUS		6			/* offset of Modem stat/RS-232 Input	R6	*/

#define	FIFO_16550		1			/* Indicates the UART is a 16550				*/
#define	FIFO_8250		2			/* Indicates the UART is an 8250/16450		*/


/************************************************************************/
/*																								*/
/*	These constants are used when the user requests to change the			*/
/*	baud rate on the serial interface. 												*/
/*																								*/
/************************************************************************/

/*-------------------------------------------------------------*/
/* This is the 8-bit location on the RFNC where the FW stores 	*/
/* the baud rate.  The firmware stores the last baud rate		*/
/* setting in EEPROM.  To change this setting, send a 			*/
/* PokeEEPROM packetized command to the RFNC.  Write one byte	*/
/* to location 0x2c.															*/
/*-------------------------------------------------------------*/
#define RFNC_BAUDRATE_ADDR  0x2c


/* These are the values written to RFNC_BAUDRATEADDR. */
#define RFNC_BAUD_1200		0
#define RFNC_BAUD_2400		1
#define RFNC_BAUD_4800		2
#define RFNC_BAUD_9600		3
#define RFNC_BAUD_19200		4
#define RFNC_BAUD_38400		5
#define RFNC_BAUD_57600		6
#define RFNC_BAUD_115200	7
#define RFNC_BAUD_UNINIT	0xFF

#define MAX_BAUDRATES		8

/*-------------------------------------------------*/
/* Every packet received through the serial port	*/
/* arrives in the following format.  There are 		*/
/* header bytes and some trailer bytes.				*/
/*																	*/
/*   SOH character											*/
/*   High byte of data length								*/
/*   Low byte of the data length							*/
/*   Header Checksum											*/
/*   <Data - multiple bytes>								*/
/*   High byte of the data checksum						*/
/*   Low byte of the data checksum						*/
/*   NULL byte													*/
/*																	*/
/*-------------------------------------------------*/

#define	SOH 				0x01		/* Start of header character.		 		*/
#define	HDRSIZE			2			/* Size of the data length header.		*/
#define	CHKSUMSIZE		2			/* Size of the data checksum.				*/

#define	HDR_LEN			(1+HDRSIZE+1)
											/* This is the SOH character, the two	*/
										 	/* byte packet header length, and the	*/
										 	/* one byte header checksum.				*/

#define	TOTAL_HDRS		(HDR_LEN+CHKSUMSIZE+1)
											/* This is the length of all overhead	*/
											/* bytes (headers and trailers) 			*/
											/* required to transmit one packet.		*/


/*----------------------------------*/
/* Interrupt Identification Defines */
/*----------------------------------*/
#define	NO_INT_PENDING		0x01		/* This bit is set when there are no	*/
												/* interrupts pending.						*/
#define	INT_BITS_MASK		0x0F		/* These bits indicate which, if any,	*/
												/* interrupts are pending.					*/
#define	FIFOMODE				0xC0		/* The chip is in FIFO mode		  		*/


/*----------------------------------------*
 * Serial Receive and Transmit States 		*
 *----------------------------------------*/

#define	WAIT_SEND_REQ	0			/* Initial state for the send state machin*/
#define	TxING_DATA		1			/* Transmitting the data buffer				*/

#define	WAIT_SOH			0			/* Initial state for receiver state machin*/
#define	RxING_HDR		1			/* Receiving the header							*/
#define	RxING_DATA		2			/* Receiving the data part of packet		*/
#define	RxING_CHKSUM	3			/* Receiving the checksum						*/
#define	RxING_HDRCHK	4			/* Arithmetic sum	of the length				*/



/*-------------------------------------------------*/
/* FIFO Control Register Values							*/
/*															  		*/
/* This register is a write-only register.  Bit 0	*/
/* must always be set or the other bits will	not	*/
/* be written.													*/
/*															  		*/
/* Receiver's FIFO Trigger Depth			  				*/
/* FIFO Control Register's Bit6 & Bit7 & Bit0 		*/
/*-------------------------------------------------*/

#define FIFOCTRL_ENABLE	0x1
#define FIFOCLTR_RESET  0
#define TRIGGER_1			(0x00 | FIFOCTRL_ENABLE)
#define TRIGGER_4			(0x40 | FIFOCTRL_ENABLE)
#define TRIGGER_8			(0x80 | FIFOCTRL_ENABLE)
#define TRIGGER_14		(0xC0 | FIFOCTRL_ENABLE)

/*-------------------------------------------*/
/*	Data structure used for SENDING		  		*/
/*													  		*/
/*-------------------------------------------*/

struct sndStruct
{
	unsigned_8				sState;	 	/* Current send state				*/
	unsigned_8				sReserved;	/* Pad for alignment					*/
	unsigned_16				sTxCnt;		/* Total number of bytes to xmit.*/
	unsigned_8			  *sDBPtr;		/* Pointer to Data being Tx.		*/
	struct TxRcvQueue	  *sQPtr;		/* Pointer to current queue		*/
};



/*-------------------------------------------*/
/* Data structure used for RECEIVING 		 	*/
/*												 			*/
/*-------------------------------------------*/
struct rcvStruct
{
	unsigned_8		  	rState;	 		/* Current receive state			*/
	unsigned_8			rReserved;		/* Pad for alignment	  				*/
	unsigned_16		  	rPosition;		/* Used to count the number of	*/
												/* received.							*/
	unsigned_16		  	rDataLen; 		/* The data length.					*/
	unsigned_16		  	rChecksum;		/* Data checksum received from	*/
												/* the RFNC.  The Hi byte is 		*/
												/* received first.					*/
	unsigned_16		  	rCheckAcc;		/* Contains the data checksum		*/
												/* that we accumulate as we read	*/
												/* in bytes.							*/
	unsigned_8		   *rDBPtr;			/* Pointer to data being Rxed.	*/
	struct TxRcvQueue *rQPtr;			/* Pointer to current Receive Q.	*/
};



/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "slld.c"						*/
/*																							*/
/*********************************************************************/

int		SLLDInit (void);
void 		SLLDTxRxInit (void);
void 		SLLDSetupIO();
void 		SLLDIsr (void);
void 		isrModemStatus (void);
void 		isrTBE (void);
void 		isrRxRDY (void);
void 		isrError (void);
void		isrNIL (void);
void  	CheckError (unsigned_8  linestatus);
void		outDummyChar (void);
void 		SLLDResetReceiver(void);
void 		SetupIOManagement (void);
void 		DetectUART (void);

#endif /* SLLD_H */
