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
/*	$Header:   J:\pvcs\archive\clld\slldhw.c_v   1.29   14 Jun 1996 16:23:48   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldhw.c_v  $										*/
/* 
/*    Rev 1.29   14 Jun 1996 16:23:48   JEAN
/* Changed the parameters to ClearWakeupBit ().
/* 
/*    Rev 1.28   23 Apr 1996 09:43:24   JEAN
/* In EndOfTx (), added code to disable system interrupts before
/* updating the transmit state or putting a buffer on the transmit
/* ready buffer.
/* 
/*    Rev 1.27   16 Apr 1996 11:36:48   JEAN
/* Changed the function calls to SLLDGetABuf ().  Also changed
/* the TxRequest () return status.  When we are out of buffers,
/* we now return NAK instead of FAILURE.
/* 
/*    Rev 1.26   01 Apr 1996 15:17:30   JEAN
/* Added a test in ResetRFNC to detect a firmware fatal
/* error.
/* 
/*    Rev 1.25   29 Mar 1996 11:49:26   JEAN
/* Added some comments.
/* 
/*    Rev 1.24   22 Mar 1996 14:59:14   JEAN
/* Removed some debug prints.
/* 
/*    Rev 1.23   14 Mar 1996 14:57:44   JEAN
/* Changed a debug color print.
/* 
/*    Rev 1.22   14 Mar 1996 14:50:20   JEAN
/* Changed some debug color.
/* 
/*    Rev 1.21   13 Mar 1996 17:54:28   JEAN
/* Added some sanity checks and debug code.
/* 
/*    Rev 1.20   12 Mar 1996 14:10:46   JEAN
/* Added an extern declaration for the LineStatusReg.
/* 
/*    Rev 1.19   12 Mar 1996 14:05:20   JEAN
/* Added a check in EndOfTx to make sure that the FIFO is empty
/* before starting to transmit a new packet.
/* 
/*    Rev 1.18   12 Mar 1996 10:42:18   JEAN
/* Added in a debug color print.
/* 
/*    Rev 1.17   08 Mar 1996 19:18:14   JEAN
/* Added checks packet length checks in RxData and GetRxData.  Also
/* added some debug color prints for failure conditions.  Added a
/* parameter to SerialInit.
/* 
/*    Rev 1.16   06 Feb 1996 14:28:46   JEAN
/* Many changes to improve performance.
/* 
/*    Rev 1.15   31 Jan 1996 13:22:40   JEAN
/* Modified ResetModem() to flush the Rx and Tx FIFOs, reduced
/* the delay between UART register accesses, changed the ordering
/* of some header include files.
/* 
/*    Rev 1.14   08 Jan 1996 14:18:34   JEAN
/* Added a 2 microsecond delay between UART register accesses.
/* 
/*    Rev 1.13   19 Dec 1995 18:12:30   JEAN
/* Moved some extern declarations under the ifndef MULTI.
/* 
/*    Rev 1.12   14 Dec 1995 15:30:52   JEAN
/* Added support for multiple serial cards and fixed a bug
/* in ResetRFNC where we were enabling RFNC interrupts instead
/* of UART interrupts.
/* 
/*    Rev 1.11   07 Dec 1995 12:14:56   JEAN
/* Changed SetWakeupBit() to toggle RTS only when the card
/* is really asleep.
/* 
/*    Rev 1.10   17 Nov 1995 16:39:00   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   12 Oct 1995 11:43:58   JEAN
/* Fixed some compiler warnings and spelling errors.
/* 
/*    Rev 1.8   25 Jun 1995 09:15:50   tracie
/* added more comments only.
/* 																								*/
/*    Rev 1.7   23 May 1995 09:46:40   tracie										*/
/* UART16550																					*/
/* 																								*/
/*    Rev 1.6   24 Apr 1995 15:50:38   tracie										*/
/* 																								*/
/*    Rev 1.5   13 Apr 1995 08:49:16   tracie										*/
/* XROMTEST version and NS16550 FIFO mode												*/
/* 																								*/
/*    Rev 1.4   16 Mar 1995 16:17:30   tracie										*/
/* Added support for ODI																	*/
/* 																								*/
/*    Rev 1.3   22 Feb 1995 10:37:50   tracie										*/
/* 																								*/
/*    Rev 1.2   27 Dec 1994 15:44:52   tracie										*/
/* 																								*/
/*    Rev 1.1   14 Dec 1994 10:46:14   tracie										*/
/* Minor changes made for this 8250 UART working version							*/
/* 																								*/
/* Initial revision		21 Nov 1994		Tracie										*/
/*																									*/
/*																									*/
/***************************************************************************/
#include <stdio.h>
#include <memory.h>
#include <conio.h>

#include "constant.h"	
#include "lld.h"
#include "asm.h"
#include "bstruct.h"
#include	"slld.h"
#include "slldbuf.h"
#ifdef MULTI
#include "multi.h"
#endif
#include "slldport.h"
#include "slldhw.h"

#ifdef MULTI
extern struct LLDVarsStruct	*LLDVars;

#else

extern struct rcvStruct	  	uartRx;
extern struct sndStruct	  	uartTx;
extern unsigned_16		  	IntrEnableReg;
extern unsigned_16		  	LineCtrlReg;
extern unsigned_16 			LineStatReg;
extern unsigned_16		  	ModemStatReg;
extern unsigned_16		  	ModemCtrlReg;
extern unsigned_8			  	ModemCtrlVal;
extern unsigned_16		  	FIFOCtrlReg;
extern unsigned_8			  	SLLDFIFOTrigger;
extern unsigned_8				SLLDBaudrateCode;
extern unsigned_8				FIFOType;
extern unsigned_32			LLDBadCopyLen;

extern struct TxRcvQueue *QLoad;
extern unsigned_8 		 *QDBptr;

extern struct SLLDLinkedList	uFreeList;
extern struct SLLDLinkedList	uTxRdyList;

#endif

/*----------------------------------------------------------------*/
/* The RF network card's firmware does not support all possible	*/
/* baud rates, so remove the unsupported baud rates.					*/
/*----------------------------------------------------------------*/
#if 1
static unsigned_16	DivisorTable[] = { 
											96, 		/* [0]  : 1200		*/
											48, 		/* [1]  : 2400		*/
											24, 		/* [2]  : 4800		*/
											12, 		/* [3]  : 9600		*/
											6, 		/* [4] : 19200		*/
											3, 		/* [5] : 38400		*/
											2, 		/* [6] : 56000		*/
											1 			/* [7] : 115200	*/
						 				};


#else
/* These are all of the baud rates the UART supports. */
static unsigned_16	DivisorTable[] = { 
											384,		/* [0]  : 300		*/ 
											192, 		/* [1]  : 600		*/
											96, 		/* [2]  : 1200		*/
											64,		/* [3]  : 1800		*/
											58, 		/* [4]  : 2000		*/
											48, 		/* [5]  : 2400		*/
											32, 		/* [6]  : 3600		*/
											24, 		/* [7]  : 4800		*/
											16, 		/* [8]  : 7200		*/
											12, 		/* [9]  : 9600		*/
											6, 		/* [10] : 19200	*/
											3, 		/* [11] : 38400	*/
											2, 		/* [12] : 56000	*/
											1 			/* [13] : 115200	*/
						 				};
#endif



/***************************************************************************/
/*																									*/
/*	ResetRFNC (*TxMode, IntNum)	  														*/
/*																									*/
/*	INPUT:	TxMode 	-	Not used, left in to be compatible with non-serial */
/*                       cards.															*/
/*				IntNum 	-	Not used.														*/
/*																									*/
/*	OUTPUT:	Returns	-	0 = Success, 0xFF = Failure								*/
/*																									*/
/*	DESCRIPTION: This routine resets the serial RFNC and the data				*/
/*              structures used to manage it.  It is assumed that PIC		*/
/*              interrupts are disabled when this function is called.		*/
/*																									*/
/***************************************************************************/

unsigned_8 ResetRFNC (unsigned_8 *Txmode, unsigned_8 IntNum)

{
	unsigned_8 status;

	/* To avoid compiler warnings. */
	if (Txmode);
	if (IntNum);

	/*-------------------------------------------------*/
	/* When CTS is high and DSR is low, this indicates	*/
	/* the firmware had a fatal error.  This condition	*/
	/* happens when it runs out of buffers.				*/
	/*-------------------------------------------------*/
	status = (unsigned_8)_inp(ModemStatReg);
	if ( (status & CTS_BIT) && !(status & DSR_BIT))
	{
		OutChar ('F', RED+BLINK);
		OutChar ('W', RED+BLINK);
		OutChar ('H', RED+BLINK);
		OutChar ('a', RED+BLINK);
		OutChar ('l', RED+BLINK);
		OutChar ('t', RED+BLINK);
		OutChar ('e', RED+BLINK);
		OutChar ('d', RED+BLINK);
		OutChar ('D', RED+BLINK);
		OutChar ('S', RED+BLINK);
		OutChar ('R', RED+BLINK);
		OutChar ('L', RED+BLINK);
		OutChar ('o', RED+BLINK);
		OutChar ('w', RED+BLINK);
	}

	DisableUARTInt ();					/* Disable all UART interrupts.			*/
	Clear_PendingUARTIntr ();			/* Clear any pending UART interrupts.	*/
	SLLDResetModem ();					/* Reset the serial RFNC.					*/
	SLLDTxRxInit ();						/* Initialize Rx and Tx states.			*/
	EnableUARTRxInt (); 			      /* Enable UART receive interrupts.   	*/

	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	TxRequest (PktLength, Txmode)															*/
/*																									*/
/*	INPUT:		PktLength	-	Length of packet to transmit to the card 		*/
/*					TxMode		-	Bus mode for transmission, 8/16 bit		  		*/
/*                            Not used for serial.									*/
/*																									*/
/*	OUTPUT:		Returns		-	ACK / NAK / Failure							  		*/
/*																									*/
/*	DESCRIPTION: This routine will request a FREE buffer from the buffer 	*/
/*					 pool and sets up variables to get ready to load the packet	*/
/*					 into the buffer.	It will copy the packet header into			*/
/*              the buffer, but not the data portion of the packet.  This	*/
/*              routine does not transmit any characters.						*/
/*																									*/
/***************************************************************************/

unsigned_8 TxRequest (unsigned_16 PktLength, unsigned_8 Txmode)
{	

#if 1
	OutChar ('T', BLACK + GREEN_BACK);
	OutChar ('x', BLACK + GREEN_BACK);
#endif

	if ((PktLength > (BUFFERLEN - TOTAL_HDRS)) || (PktLength == 0))
	{
		LLDBadCopyLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('T', RED);
		OutChar ('x', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(PktLength >> 8), RED);
		OutHex ((unsigned_8)(PktLength), RED);
#if INT_DEBUG
		__asm	int 3
#endif
		return (FAILURE);
	}

	if (Txmode);

	/*----------------------------------------------------------*/
	/* If we are out of buffers, return a NAK.  The packet		*/
	/* will get put on the timed queue for later transmission.	*/
	/* Added the 7 byte serial packet header/footer to the		*/
	/* packet length so that we get a buffer large enough.		*/
	/*----------------------------------------------------------*/
	if ((QLoad = SLLDGetABuf (&uFreeList, PktLength+TOTAL_HDRS)) == NIL)
	{
		OutChar ('B', BLACK + CYAN_BACK);
		return (NAK);
	}

	/*-------------------------------------------------------*/
	/*	Packet format:														*/
	/*																			*/
	/* BYTE	SOH:				Start of header (0x01)				*/
	/* BYTE  LENhigh:			High byte of data length			*/
	/* BYTE	LENlow:			Low byte of data length				*/
	/*	BYTE	HEADER CHECK:	~(LENhigh) + ~(LENlow)				*/
	/*																			*/
	/* BYTES	DATA:															*/
	/*																			*/
	/* BYTE	CHECKSUMhigh:	16 bit check sum of data			*/
	/* BYTE	CHECKSUMlow:												*/
	/* BYTE	NULL:				Null byte 								*/
	/*																			*/
	/* Copy the start of header character, the data length,	*/
	/* and the data length header checksum into the	transmit	*/
	/* buffer.  Set qData length to zero, this is the number	*/
	/* of data bytes that we have copied into the buffer.		*/
	/*-------------------------------------------------------*/
	QDBptr = QLoad->qDataPtr;
	*QDBptr++ = SOH;
	*QDBptr++ = (unsigned_8) (PktLength >> 8);
	*QDBptr++ = (unsigned_8) PktLength;
	*QDBptr++ = (unsigned_8)(~((unsigned_8) (PktLength >> 8)) + 
							~((unsigned_8) PktLength));
	QLoad->qDataLen = 0;

	return (ACK);
}


/***************************************************************************/
/*																									*/
/*	TxData (*DataPtr, DataLength, Txmode)												*/
/*																									*/
/*	INPUT:		DataPtr		-	Pointer to the data to write to the card		*/
/*					DataLength	-	Number of data bytes to write to the card		*/
/*					TxMode		-	Transfer mode, 8 or 16 bits				  		*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine copies data into the transmit buffer.  It		*/
/*              does not send any characters to the RFNC.  This routine		*/
/*              may be called several times for one packet.						*/
/*																									*/
/***************************************************************************/

void TxData (unsigned_8 FAR *DataPtr, unsigned_16 DataLength, unsigned_8 Txmode)
{
	if (Txmode);

#if 0
	OutChar ('T', BLACK + GREEN_BACK);
	OutChar ('x', BLACK + GREEN_BACK);
	OutChar ('D', BLACK + GREEN_BACK);
	OutChar ('a', BLACK + GREEN_BACK);
#endif

	if ((DataLength > (BUFFERLEN - TOTAL_HDRS)) || (DataLength == 0))
	{
		LLDBadCopyLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('T', RED);
		OutChar ('x', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(DataLength >> 8), RED);
		OutHex ((unsigned_8)(DataLength), RED);
#if INT_DEBUG
		__asm	int 3
#endif
		return;
	}

#ifdef ODI
	fmemcpy ((unsigned_8 FAR *)QDBptr, DataPtr, DataLength);
#else
	memcpy (QDBptr, DataPtr, DataLength);
#endif

	/* Update the data length and data pointer */
	QLoad->qDataLen += DataLength;
	QDBptr += DataLength;						
}


/***************************************************************************/
/*																									*/
/*	EndOfTx ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:		Returns	-	SUCCESS														*/
/*																									*/
/*	DESCRIPTION:  This routine computes the data checksum and copies the		*/
/*               trailer bytes (checksum + NULL) into the transmit buffer.	*/
/*               Next checks to see if a packet transmission is already		*/
/*               in progress.  If not, it starts transmitting the current	*/
/*               packet.  If a transmission is already in progress, it		*/
/*               queues the current packet on the transmit ready list.		*/
/*																									*/
/***************************************************************************/

unsigned_8 EndOfTx ()

{	
	unsigned_8	*ptr;
	unsigned_16	chksum=0;
	FLAGS			oldflag;
	

	/*-------------------------------------------*/
	/* First compute the data checksum.  Skip		*/
	/* over the packet header (SOH, length and	*/
	/* header checksum).									*/
	/*-------------------------------------------*/
	ptr = QLoad->qDataPtr + HDR_LEN;
	for (; ptr < QDBptr; chksum += *ptr++);

	/* Copy the checksum into the buffer, high byte first. */
	*QDBptr++ = (unsigned_8) (chksum >> 8);
	*QDBptr++ = (unsigned_8) chksum;

	/*-------------------------------------------------------------*/
	/* Add a zero character to the end of each packet.  The UART	*/
	/* on the firmware side needs this extra character to make		*/
	/* sure he gets an interrupt when the last byte is received.	*/
	/*-------------------------------------------------------------*/
	*QDBptr = (unsigned_8) 0;	

	/*-------------------------------------------------------------*/
	/* Increment the packet length to include the packet headers	*/
	/* and trailers.																*/
	/*-------------------------------------------------------------*/
	QLoad->qDataLen += TOTAL_HDRS;


	/*-------------------------------------------------*/
	/* Disable interrupts before changing the uartTx	*/
	/* and transmit buffer queue structures.				*/
	/*-------------------------------------------------*/
	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*-----------------------------------------*/
	/* Are we in the middle of a transmission? */
	/*-----------------------------------------*/
	if ( ((unsigned_8)_inp (LineStatReg) & THRE) && 
		  (uartTx.sState == WAIT_SEND_REQ))
	{	

		/*-------------------------------------------------*/
		/* Transmitter is idle, so start the transmission. */
		/*-------------------------------------------------*/
		uartTx.sTxCnt		= QLoad->qDataLen;
		uartTx.sQPtr	  	= QLoad;
		uartTx.sDBPtr		= QLoad->qDataPtr;
		uartTx.sState		= TxING_DATA;

#if 0
		OutChar ('S', BROWN);
		OutChar ('O', BROWN);
		OutChar ('H', BROWN);
#endif

		EnableUARTRxTxInt ();
		isrTBE ();
	}
	else
	{	/*----------------------------------------*/
		/* We are already transmitting a packet,	*/
		/* so queue this buffer for transmission	*/
		/* at a later time.								*/
		/*----------------------------------------*/
		SLLDPutABuf (&uTxRdyList, QLoad);
		OutChar ('+', BROWN);
		OutChar ('q', BROWN);
	}
	RestoreFlag (oldflag);

  	return (SUCCESS);

}


/***************************************************************************/
/*																									*/
/*	RxRequest (Txmode)																		*/
/*																									*/
/*	INPUT:	Txmode	-	Transmit mode 8/16 bits mode								*/
/*																									*/
/*	OUTPUT:	Returns	-	Length of the packet the card is going to receive.	*/
/*																									*/
/*	DESCRIPTION: This routine is called from the LLDIsr() to request the		*/
/*					 length of the receive buffer. 	 									*/
/*																									*/
/***************************************************************************/

unsigned_16 RxRequest (unsigned_8 Txmode)

{
	if (Txmode);

#if 0
	OutChar ('R', BROWN);
	OutChar ('x', BROWN);
	OutChar ('R', BROWN);
	OutChar ('e', BROWN);
	OutChar ('q', BROWN);
	OutHex ((unsigned_8)(uartRx.rDataLen >> 8), BROWN);
	OutHex ((unsigned_8)(uartRx.rDataLen >> 0), BROWN);
#endif

	return (uartRx.rDataLen);
}



/***************************************************************************/
/*																									*/
/*	RxData (*RxPktPtr, Length, Txmode)													*/
/*																									*/
/*	INPUT:	RxPktPtr	-	Pointer to the buffer to read the packet into		*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*				Txmode	-	Transmit mode 8/16 bit mode								*/
/*																									*/
/*	OUTPUT: 	Returns	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION:  This routine copies Length bytes from the SLLD's				*/
/*               receiving buffer into RxPktPtr.  We do not necessarily		*/
/*               copy the entire receive buffer at one time.  Multiple		*/
/*               calls to this routine or to GetRxData may be made before	*/
/*               the entire buffer has been copied.								*/
/*																									*/
/***************************************************************************/

unsigned_8 RxData (unsigned_8 *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode)
{	
	/* To avoid compiler warnings */
	if (Txmode);

	/*----------------------------------------------------------------*/
	/* rDataLen contains the number of available bytes in the buffer.	*/
	/* An available bytes is a byte that has not been copied yet.		*/
	/*----------------------------------------------------------------*/
	if ((Length > uartRx.rDataLen) || (Length == 0))
	{	
		LLDBadCopyLen++;
		OutChar ('R', CYAN);
		OutChar ('x', CYAN);
		OutChar ('D', CYAN);
		OutChar ('a', CYAN);
		OutChar ('t', CYAN);
		OutChar ('a', CYAN);
		OutChar ('L', CYAN);
		OutChar ('e', CYAN);
		OutChar ('n', CYAN);
		OutChar ('E', CYAN);
		OutChar ('r', CYAN);
		OutChar ('r', CYAN);
		OutHex ((unsigned_8)(Length >> 8), CYAN);
		OutHex ((unsigned_8)(Length), CYAN);
#if INT_DEBUG
		__asm	int 3
#endif
		return (FAILURE);
	}

	memcpy (RxPktPtr, uartRx.rDBPtr, Length);

	/*----------------------------------------------*/
	/* Since multiple copies are required, update	*/
	/* the data pointer and length fields.				*/
	/*----------------------------------------------*/
	uartRx.rDBPtr += Length;
	uartRx.rDataLen -= Length;
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	GetRxData (*RxPktPtr, Length, Txmode)												*/
/*																									*/
/*	INPUT:	RxPktPtr	-	Pointer to the buffer to read the packet into		*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*				Txmode	-	Transmit mode 8/16 bit mode								*/
/*																									*/
/*	OUTPUT: 																						*/
/*																									*/
/*	DESCRIPTION: This routine is the same as RxData except the destination	*/
/*              address is a FAR pointer and it does not have a return		*/
/*              status.																		*/
/*																									*/
/***************************************************************************/

void GetRxData (unsigned_8 FAR *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode)

{	
	/* To avoid compiler warnings */
	if (Txmode);

	/*----------------------------------------------------------------*/
	/* rDataLen contains the number of available bytes in the buffer.	*/
	/* An available bytes is a byte that has not been copied yet.		*/
	/*----------------------------------------------------------------*/
	if ((Length > uartRx.rDataLen) || (Length == 0))
	{	
		LLDBadCopyLen++;
		OutChar ('G', CYAN);
		OutChar ('R', CYAN);
		OutChar ('x', CYAN);
		OutChar ('D', CYAN);
		OutChar ('a', CYAN);
		OutChar ('t', CYAN);
		OutChar ('a', CYAN);
		OutChar ('L', CYAN);
		OutChar ('e', CYAN);
		OutChar ('n', CYAN);
		OutChar ('E', CYAN);
		OutChar ('r', CYAN);
		OutChar ('r', CYAN);
		OutHex ((unsigned_8)(Length >> 8), CYAN);
		OutHex ((unsigned_8)(Length >> 0), CYAN);
#if INT_DEBUG
		__asm	int 3
#endif

		/* If the user asks for more data than we have,	*/
		/* just give him what is available.  				*/
		if (Length == 0)
			return;
		else
			Length = uartRx.rDataLen;
	}

#ifdef ODI
	fmemcpy (RxPktPtr, (unsigned_8 FAR *)uartRx.rDBPtr, Length);
#else
	memcpy (RxPktPtr, uartRx.rDBPtr, Length);
#endif

	/*----------------------------------------------*/
	/* Since multiple copies are required, update	*/
	/* the data pointer and length fields.				*/
	/*----------------------------------------------*/
	uartRx.rDBPtr += Length;
	uartRx.rDataLen -= Length;
}


/***************************************************************************/
/*																									*/
/*	EndOfRx ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine initializes the receive buffer control			*/
/*              structure so that we are ready to receive another packet.	*/
/*																									*/
/***************************************************************************/

void EndOfRx (void)

{
#if 1
	OutChar ('E', BLACK+WHITE_BACK);
	OutChar ('R', BLACK+WHITE_BACK);
#endif
	SLLDResetReceiver();				
}


/***************************************************************************/
/*																									*/
/* SetWakeupBit ()																			*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION: For the serial module, there is no wakeup bit.  To wake		*/
/*              the card up, RTS needs to be deasserted.	If the card is		*/
/*              not asleep, then we don't need to bother waking up the		*/
/*              card.  The card is asleep when DSR and CTS are 0.	 		 	*/
/*																									*/
/***************************************************************************/

void SetWakeupBit (void)

{	
	if ((((unsigned_8)_inp(ModemStatReg)) & DSR_BIT) == 0)
	{
		OutChar ('W', BLACK+WHITE_BACK);
		OutChar ('K', BLACK+WHITE_BACK);
		Delay_uS (1);
		_outp (ModemCtrlReg, ModemCtrlVal &= ~RTS);  
		Delay_uS (60);
		_outp (ModemCtrlReg, ModemCtrlVal |= RTS);
	}
}



/***************************************************************************/
/*																									*/
/* ClearWakeupBit ()																			*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION:  Since there is no wakeup bit, there is nothing to do!		*/
/*																									*/
/***************************************************************************/

void ClearWakeupBit (unsigned_8 OldControlValue, unsigned_8 OldStatusValue)

{ 
	if (OldControlValue);
	if (OldStatusValue);

	return;
}


/***************************************************************************/
/*																									*/
/*	ClearNAK ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: Nothing to do for the serial RFNC.									*/
/*																									*/
/***************************************************************************/

void ClearNAK (void)

{
	return;
}

/***************************************************************************/
/*																									*/
/*	SLLDResetModem ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine resets the UART modem (the RFNC) by			*/
/*						asserting DTR for 6 milliseconds, resets our UART's		*/
/*                FIFOs, and delays 800 milliseconds to give the RFNC time	*/
/*                to initialize.															*/
/*																									*/
/***************************************************************************/

void	SLLDResetModem ()

{
	OutChar ('R',RED + WHITE_BACK);
	OutChar ('e',RED + WHITE_BACK);
	OutChar ('s',RED + WHITE_BACK);
	OutChar ('e',RED + WHITE_BACK);
	OutChar ('t',RED + WHITE_BACK);
	OutChar ('R',RED + WHITE_BACK);
	OutChar ('F',RED + WHITE_BACK);
	OutChar ('N',RED + WHITE_BACK);
	OutChar ('C',RED + WHITE_BACK);

	SetBit_ModemCtrl (DTR);				 	/* Assert DTR to reset the modem */
	Delay (6);								 	/* Delay approx. 6 ms.				*/
	ClrBit_ModemCtrl (DTR);					/* DeAssert DTR						*/

	/*----------------------------------------*/
	/* Clear out any bytes in the receive and	*/
	/* transmit FIFOs and reprogram the FIFO	*/
	/* trigger levels.								*/
	/*----------------------------------------*/
	if (FIFOType == FIFO_16550)
	{

		_outp (FIFOCtrlReg, FIFOCLTR_RESET);
		Delay (1);
		switch (SLLDFIFOTrigger)
		{		case 4:	_outp (FIFOCtrlReg, TRIGGER_4);
							break;
	
				case 8:	_outp (FIFOCtrlReg, TRIGGER_8);
							break;
	
				case 14:	_outp (FIFOCtrlReg, TRIGGER_14);
							break;
	
				default:	_outp (FIFOCtrlReg, TRIGGER_1);
							break;
		}
	}

	Delay (800);		 					/* Delay approx. 800 ms to give	*/
												/* RF card time to reset.			*/

	/*----------------------------------------------------*/
	/* Clear the modem status interrupt that is generated	*/
	/* when the RFNC card is reset.								*/
	/*----------------------------------------------------*/
	Clear_PendingUARTIntr ();

}


/***************************************************************************/
/*																									*/
/*	SerialInit ()				 																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine sets the UART to the correct baud rates,		*/
/*                number of start/stop bits, and correct parity.				*/
/*																									*/
/***************************************************************************/

void SerialInit (unsigned_8 baud)

{
	unsigned_16	FreqDivisor;

	OutChar ('B', GREEN);
	OutChar ('a', GREEN);
	OutChar ('u', GREEN);
	OutChar ('d', GREEN);
	OutChar ('R', GREEN);
	OutChar ('a', GREEN);
	OutChar ('t', GREEN);
	OutChar ('e', GREEN);
	OutHex (baud, GREEN);

	FreqDivisor = DivisorTable [baud];

	SetBaudRate (FreqDivisor);
	_outp (LineCtrlReg, DATA8STOP1);		/* DataFormat:8-bytes,1-stop,0-parity*/

}

