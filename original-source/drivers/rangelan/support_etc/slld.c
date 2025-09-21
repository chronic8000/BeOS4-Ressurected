/***************************************************************************/
/*                                                                         */
/*						  	PROXIM RANGELAN2 LOW LEVEL DRIVER							*/
/*                                                                         */
/*    PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the       */
/*    sole property of PROXIM, INC.  Reproduction or utilization of        */
/*    this source in whole or in part is forbidden without the written     */
/*    consent of PROXIM, INC.                                              */
/*                                                                         */
/*                                                                         */
/*                   (c) Copyright PROXIM, INC. 1994                       */
/*                         All Rights Reserved                             */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/*	$Header:   J:\pvcs\archive\clld\slld.c_v   1.29   29 Jul 1996 12:23:38   JEAN  $																						*/
/*																									*/
/* Edit History                                                            */
/*                                                                         */
/* $Log:   J:\pvcs\archive\clld\slld.c_v  $                                */
/* 
/*    Rev 1.29   29 Jul 1996 12:23:38   JEAN
/* Added code to enable RTS and restore processor flags.
/* 
/*    Rev 1.28   14 Jun 1996 16:23:12   JEAN
/* Comment changes.
/* 
/*    Rev 1.27   02 May 1996 11:20:24   JEAN
/* Changed the way isrTBE () handles the transmission of the
/* last character in the packet.
/* 
/*    Rev 1.26   16 Apr 1996 11:35:06   JEAN
/* Changed the function calls to SLLDGetABuf ().  Changed the
/* oldflags type from unsigned_16 to FLAGS.  In isrTBE (), added
/* a check to make sure that the transmit buffer is empty before
/* we try to fill it.
/* 
/*    Rev 1.25   29 Mar 1996 11:48:04   JEAN
/* Added code to protect against missing transmit buffer empty
/* interrupts.  Also added code in the modem status interrupt to
/* see if there is any data waiting to be transmitted when the
/* card wakes up.
/* 
/*    Rev 1.24   22 Mar 1996 15:42:00   JEAN
/* Turned off debug prints.
/* 
/*    Rev 1.23   22 Mar 1996 14:52:32   JEAN
/* Fixed a bug in isrTBE() where the transmit state was not being
/* initialized when we started transmitting a new packet.
/* Changed some debug color.
/* 
/*    Rev 1.22   14 Mar 1996 14:49:50   JEAN
/* Changed some debug color.
/* 
/*    Rev 1.21   13 Mar 1996 17:53:28   JEAN
/* Added some sanity checks and debug code.
/* 
/*    Rev 1.20   12 Mar 1996 14:04:32   JEAN
/* Fixed a bug in isrTBE where we would write more than 16
/* characters in the FIFO.
/* 
/*    Rev 1.19   12 Mar 1996 10:41:30   JEAN
/* Removed code to clear TxFlag when a character is received.
/* 
/*    Rev 1.18   08 Mar 1996 19:12:36   JEAN
/* Changed the parameters to SerialInit for soft baudrate support.
/* Added code to isrModemStatus to detect all signal changes.
/* Added a function to output a dummy character to the UART (needed
/* when sending the soft baudrate command).  Added a flag to indicate
/* that we are in the serial isr routine (debug only).  Added two new
/* statistics.
/* 
/*    Rev 1.17   06 Feb 1996 14:27:28   JEAN
/* Many changes to improve performance.
/* 
/*    Rev 1.16   31 Jan 1996 13:14:08   JEAN
/* Changed the ordering of some header include files, added some
/* statistics, modified SLLDInit() to disable interrupts before
/* calling SetupIOManagement(), reduced the delay between UART
/* register accesses, modified CheckError(), modified isrTBE() to
/* output the character instead of calling SLLDTxData() to output
/* the character, added a while loop inside of isrRxRDY() to read
/* in multiple characters, and removed an unused function,
/* LLSRoamNotify().
/* 
/*    Rev 1.15   08 Jan 1996 14:16:26   JEAN
/* Added a 2 microsecond delay between UART register accesses,
/* fixed a bug when computing the IRQ mask, deleted an unused
/* variable, RxFIFODepth, and added a new variable, IRQMaskRegister.
/* This is the address of either PIC 1 or PIC 2's IRQ mask
/* register.
/* 
/*    Rev 1.14   02 Jan 1996 18:28:44   JEAN
/* Changed the way we process receive interrupts.  Previous version
/* always read one character.  This new version reads in as many
/* characters as possible.
/* 
/*    Rev 1.13   19 Dec 1995 18:11:26   JEAN
/* Added a header include file.
/* 
/*    Rev 1.12   14 Dec 1995 15:13:12   JEAN
/* Changes made to support multiple serial cards.
/* 
/*    Rev 1.11   28 Nov 1995 10:37:50   JEAN
/* Documentation changes.
/* 
/*    Rev 1.10   17 Nov 1995 16:38:38   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   12 Oct 1995 11:43:14   JEAN
/* Added a FIFOtype variable.  Called DisableRTS() and EnableRTS()
/* instead of using in line code.  Fixed some compiler warnings.
/* 
/*    Rev 1.8   25 Jun 1995 09:15:18   tracie
/* Added more comments only.
/* 																								*/
/*    Rev 1.7   23 May 1995 09:43:32   tracie										*/
/* UART16550																					*/
/* 																								*/
/*    Rev 1.6   24 Apr 1995 15:50:26   tracie										*/
/* 																								*/
/*    Rev 1.5   13 Apr 1995 08:49:00   tracie										*/
/* XROMTEST version																			*/
/*                                                                         */
/* Initial revision     21 Nov 1994    Tracie                              */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#include <memory.h>
#include <stdio.h>
#include <conio.h>

#include "constant.h"
#include "lld.h"
#include "asm.h"
#include "lldinit.h"
#include "bstruct.h"
#include "slld.h"
#include "slldbuf.h"
#ifdef MULTI
#include "multi.h"
#endif
#include "slldport.h"
#include "slldhw.h"


/************************************************************************/
/*                                                                      */
/* External variables defined in other modules                          */
/*                                                                      */
/************************************************************************/

#ifdef MULTI
extern struct LLDVarsStruct   *LLDVars;

#else

#if INT_DEBUG
extern unsigned_8		LLDInactivityTimeOut;
extern unsigned_8 	LLDSniffTime;
#endif

extern unsigned_16 	uartBaseAddr;	/* UART's base address						*/
extern unsigned_16 	TxRcvHoldReg;	/* Transmit & Receive Holding Reg.- R0	*/
extern unsigned_16 	IntrEnableReg;	/* Interrupt Enable Register		 - R1	*/
extern unsigned_16 	IntrIdentReg;	/* Interrupt Identification Reg.	 - R2 */
extern unsigned_16	FIFOCtrlReg;   /* FIFO Control Register          - R2 */
extern unsigned_16 	LineCtrlReg;	/* Line Control/Data Format Reg.	 - R3	*/
extern unsigned_16 	ModemCtrlReg;	/* Modem Control/RS-232 Output Crl- R4 */
extern unsigned_16 	LineStatReg;	/* Line Status/Serialization Stat - R5 */
extern unsigned_16 	ModemStatReg;	/* Modem Status/RS-232 Input Stat - R6	*/
extern unsigned_8		IRQLevel;  		/* The interrupt level selected	 		*/										
extern unsigned_8		ModemCtrlVal;	/* Modem control value                 */
extern unsigned_8		IRQLevel;      /* The interrupt level selected        */                            
extern unsigned_8		IRQMask;       /* The interrupt controller mask bit.  */
extern unsigned_8		IRQMaskReg;    /* The interrupt mask register address.*/

extern struct rcvStruct  uartRx;    /* Serial CLLD's Receiving State       */
extern struct sndStruct  uartTx;    /* Serial CLLD's Transmitting State    */

extern unsigned_8 LenLow;				/* Used to calculate the checksum of   */
extern unsigned_8 LenHi;            /* the data length bytes.              */

extern unsigned_8		TxFIFODepth;	/* Size UART's Transmit FIFO           */
extern unsigned_8		FIFOType;		/* Is it an 8250/16450 or a 16550 UART?*/

extern unsigned_16   LLDIOAddress1;
extern unsigned_8    LLDIntLine1;	
extern unsigned_8		SLLDFIFOTrigger;
extern unsigned_8		SLLDBaudrateCode;
extern unsigned_8		InSerialISRFlag;
extern unsigned_32	SLLDRxHdrErr;
extern unsigned_32	SLLDRxChksumErr;
extern unsigned_32	SLLDRxLenErr;
extern unsigned_32	SLLDOverrunErr;
extern unsigned_32	SLLDRxErrors;


extern struct SLLDLinkedList  uFreeList;				
extern struct SLLDLinkedList  uTxRdyList;

#endif

extern int tbeCount;

#define RX_DEBUG	0
#define TX_DEBUG	0
#define ISR_DEBUG	0

/************************************************************************/
/*																								*/
/* Interrupt Identification Format													*/
/*																								*/
/*		Bit3	Bit2	Bit1	Bit0	Priority Interrupt ID 							*/
/*																								*/
/*		 0     0     0     1					No Pending Interrupts  				*/
/*		 0     1     1     0		Highest	Receiver Line Status 				*/
/*		 0     1     0     0		Second	Receiver Data Available				*/
/*     1     1     0     0		Second	Character Timeout       			*/
/*     0     0     1     0		Third		Transmitter Empty						*/
/*     0     0     0     0		Fourth	Modem Status							*/
/*																								*/
/* Fill in all values in uartISR just in case we get a bad read from		*/
/* the UART.																				*/
/*																								*/
/************************************************************************/

static void	(*uartISR[]) () =
					{							/*    B3B2B1	*/
						isrModemStatus,	/* 0 - 0 0 0	*/
						isrTBE,				/* 1 - 0 0 1	*/
						isrRxRDY,			/*	2 - 0 1 0	*/
						isrError,			/*	3 - 0 1 1	*/
						isrNIL,	 			/* 4 - 1 0 0	*/
						isrNIL,	 			/*	5 - 1 0 1	*/
						isrRxRDY,	 		/* 6 - 1 1 0	*/
						isrNIL	 			/*	7 - 1 1 1	*/
					};


/************************************************************************/
/*                                                                      */
/* SLLDInit ()                                                          */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/*                                                                      */
/* OUTPUT:			Success or Failure												*/
/*                                                                      */
/* DESCRIPTION:   This routine is called from LLDInit() to initialize	*/
/*                the global variables used by the Serial LLD and to		*/
/*                reset the RFNC and our UART (our interface to the		*/
/*                RFNC).															   */
/*                                                                      */
/************************************************************************/

int SLLDInit (void)

{
	DisableInterrupt ();
	SetupIOManagement ();	/* Initialize the UART IO addresses,	*/
									/* and the PIC interrupt mask.  			*/
	DetectUART ();				/* Which UART are we talking to?  The	*/
									/* 8250 or the 16550.						*/
	SerialInit (SLLDBaudrateCode);
									/* Initializes the UART's data format	*/
									/* and the baudrate.							*/
	EnableUARTModemCtrl ();	/* Enable the modem control lines.		*/
									/* These are RTS and GP02.					*/
	Clear_PendingUARTIntr ();	/* Clear any pending interrupts.		*/


	CheckBIOSMem ();				/* Checks to see if our COM port is	*/
										/* in BIOS memory.						*/

	if (LLSRegisterInterrupt (LLDIntLine1, SLLDIsr))
	{  
		EnableInterrupt ();
		return (FAILURE);
	}

	SLLDResetModem ();   	/* Reset's the modem, our RFNC, and		*/
									/* flushes the Rx and Tx FIFOs.			*/

	EnableUARTRxInt ();     /* Enable UART Rx interrupts.  			*/
	EnableRFNCInt ();       /* Enable PIC interrupts.				  	*/
	EnableInterrupt ();     /* Enable system interrupts.				*/

	return (SUCCESS);
}


/************************************************************************/
/*                                                                      */
/* SetupIOManagement ()                                                 */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION:   This routine initializes the UART addresses and			*/
/*                determines the UART interrupt mask and interrupt 		*/
/*                register mask address.  It also calls a function to	*/
/*                initialized the buffers used to receive and transmit	*/
/*                packets to and from the serial port.						*/
/*                                                                      */
/************************************************************************/

void SetupIOManagement (void)
{

	/* Initialize the receive and transmit control structures. */
	SLLDTxRxInit ();			

	uartBaseAddr   = LLDIOAddress1;  
	TxRcvHoldReg   = LLDIOAddress1 + DATAHOLDING;   
	IntrEnableReg  = LLDIOAddress1 + INTRENABLE;
	IntrIdentReg   = LLDIOAddress1 + INTRIDENTIFY;
	FIFOCtrlReg    = LLDIOAddress1 + FIFOCONTROL;
	LineCtrlReg    = LLDIOAddress1 + LINECONTROL;
	ModemCtrlReg   = LLDIOAddress1 + MODEMCONTROL;
	LineStatReg    = LLDIOAddress1 + LINESTATUS;
	ModemStatReg   = LLDIOAddress1 + MODEMSTATUS;

	/*-------------------------------------------------------*/
   /* Create the interrupt mask by shifting the interrupt	*/
   /* number.  For example, interrupt 3 would need bit 3		*/
   /* set, which is hex 0x08.											*/
   /*																			*/
   /* Interrupts 0-7 use PIC 1 and interrupts 8-15 use		*/
   /* PIC 2.  The interrupt mask register on each PIC is		*/
   /* an 8-bit register.  AND the interrupt number with		*/
   /* 7 so that the interrupt mask for the 2nd PIC				*/
   /* sets bits 0-7 instead of bits 8-15.							*/
	/*-------------------------------------------------------*/

	IRQMask = (unsigned_8)((unsigned_8) 1 << (LLDIntLine1 & 7));
	if (LLDIntLine1 <= 7)
		IRQMaskReg = I8259IRQMASK;
	else
		IRQMaskReg = I8259IRQMASK2;

}


/************************************************************************/
/*                                                                      */
/* DetectUART ()																			*/
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION:   Determine which UART chip we are using by turning on	*/
/*						FIFO mode.  If the FIFO mode flag is not set, then    */
/*						the UART chip is 8250/16450.  If the FIFO mode flag   */
/*                is set, then the UART chip is a 16550.                */
/*                                                                      */
/************************************************************************/

void DetectUART (void)
{
	unsigned_8	IIRValue;


	/*----------------------------------------------------*/
	/* Determine the FIFO type by setting the FIFO enable	*/
	/* bits in the FIFO control register.  If we are 		*/
	/* using the 16550 UART, this causes bits 6 & 7 in		*/
	/* the interrupt identification register to be set.	*/
	/* If these bits are clear, then it must be the 8250	*/
	/* UART.																*/
	/*----------------------------------------------------*/
	_outp (FIFOCtrlReg, FIFOCTRL_ENABLE);
	OutChar ('U', BROWN);
	OutChar ('A', BROWN);
	OutChar ('R', BROWN);
	OutChar ('T', BROWN);
	IIRValue = (unsigned_8) _inp (IntrIdentReg);

	if (IIRValue & FIFOMODE)	
	{
		FIFOType = FIFO_16550;
		OutChar ('1', BROWN);
		OutChar ('6', BROWN);
		OutChar ('5', BROWN);
		OutChar ('5', BROWN);
		OutChar ('0', BROWN);

		TxFIFODepth		= 16;
		switch (SLLDFIFOTrigger)
		{
			case 4:	_outp (FIFOCtrlReg, TRIGGER_4);
						break;

			case 8:	_outp (FIFOCtrlReg, TRIGGER_8);
						break;

			case 14:	_outp (FIFOCtrlReg, TRIGGER_14);
						break;

			default:	_outp (FIFOCtrlReg, TRIGGER_1);
						break;
		}
	}
	else
	{
		FIFOType = FIFO_8250;
		OutChar ('8', BROWN);
		OutChar ('2', BROWN);
		OutChar ('5', BROWN);
		OutChar ('0', BROWN);

		TxFIFODepth			= 1;
		SLLDFIFOTrigger	= 1;
	}
}



/************************************************************************/
/*                                                                      */
/* SLLDTxRxInit ()                                                      */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION:   This routine initializes the serial state machine     */
/*                used to receive and transmit packets from the RFNC.   */
/*                This consists of one receive control structure        */
/*                (uartRx), one transmit control structure (uartTx),    */
/*                and a pool of buffers for receiving/transmitting      */
/*                data.                                                 */
/*                                                                      */
/************************************************************************/

void SLLDTxRxInit (void)

{
	/*-------------------------------------------------*/
	/* Reserve one buffer for processing an incoming	*/
	/* packet by getting it from the linked list			*/
	/* of free buffers.  Leave the rest of the buffers	*/
	/* for transmitting packets.								*/
	/*-------------------------------------------------*/
	SLLDInitBufs ();
	uartRx.rQPtr = SLLDGetABuf (&uFreeList, BUFFERLEN);

	/* Initialize the receive and transmit control structures. */
	SLLDResetReceiver ();
	uartRx.rReserved	= 0;	 /* Not used, clear it once.			*/
	memset (&uartTx, 0, sizeof (uartTx));
}

	
/***************************************************************************/
/*                                                                         */
/* isrModemStatus ()                                                       */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION: This routine gets called when one of the UART's status     */
/*              lines changes states.  These lines are the input lines		*/
/*              CTS, DTR, DSR, and DCD.  We just process the changes in		*/
/*              the CTS line.  The serial module uses this line to flow		*/
/*              control us. 																*/
/*                                                                         */
/*              CTS High - Driver Flow Control ON  - Don't send the RFNC	*/
/*                                                   any more characters.	*/
/*                                                                         */
/*              CTS Low  - Driver Flow Control OFF - OK to send the RFNC	*/
/*                                                   more characters.		*/
/*                                                                         */
/***************************************************************************/

void isrModemStatus (void)

{	unsigned_8 delta;
	unsigned_8 status;

	delta = (unsigned_8)_inp(ModemStatReg);
	OutChar ('M', MAGENTA+WHITE_BACK);
	OutChar ('S', MAGENTA+WHITE_BACK);
	OutChar ('t', MAGENTA+WHITE_BACK);
	status = (unsigned_8)_inp(ModemStatReg);

#if INT_DEBUG
	if ((delta & (DELTA_CTS | DELTA_ERI | DELTA_DCD | DELTA_DSR)) == 0)
		__asm int 3
#endif

	/* Did the CTS signal change state? */
	if (delta & DELTA_CTS)
	{
		OutChar ('C', MAGENTA+WHITE_BACK);
		OutChar ('T', MAGENTA+WHITE_BACK);
		OutChar ('S', MAGENTA+WHITE_BACK);

		/*-------------------------------------------------------------*/
		/* When CTS goes high, it means that the RFNC can now accept	*/
		/* characters.  If we have data to transmit, re-enable the 		*/
		/* transmit buffer empty interrupt.										*/
		/*-------------------------------------------------------------*/
		if (status & CTS_BIT)
		{
			OutChar ('H', MAGENTA+WHITE_BACK);
			if (uartTx.sState == TxING_DATA)
			{	EnableUARTRxTxInt ();
				isrTBE ();
			}
		}
	
		/*----------------------------------------------------------------*/
		/* When CTS goes low, it means that the RFNC can NOT accept any	*/
		/* more characters.  We need to disable the transmit buffer empty	*/
		/* interrupt so we don't send the RFNC any more characters.  The	*/
		/* RFNC sets CTS low when he runs out of buffers and cannot 		*/
		/* accomodate any new packets.												*/
		/*----------------------------------------------------------------*/
		else
		{
	
			OutChar ('L', MAGENTA+WHITE_BACK);
			EnableUARTRxInt ();
		}
		OutChar (' ', MAGENTA+WHITE_BACK);
	}

	/* Did the DCD signal change state? */
	if (delta & DELTA_DCD)
	{
		OutChar ('D', MAGENTA+WHITE_BACK);
		OutChar ('C', MAGENTA+WHITE_BACK);
		OutChar ('D', MAGENTA+WHITE_BACK);
		if (status & DCD_BIT)
			OutChar ('H', MAGENTA+WHITE_BACK);
		else
			OutChar ('L', MAGENTA+WHITE_BACK);
		OutChar (' ', MAGENTA+WHITE_BACK);
	}

	/* Did the DSR signal change state? */
	if (delta & DELTA_DSR)
	{
		OutChar ('D', MAGENTA+WHITE_BACK);
		OutChar ('S', MAGENTA+WHITE_BACK);
		OutChar ('R', MAGENTA+WHITE_BACK);
		if (status & DSR_BIT)
			OutChar ('H', MAGENTA+WHITE_BACK);
		else
			OutChar ('L', MAGENTA+WHITE_BACK);
		OutChar (' ', MAGENTA+WHITE_BACK);

#ifdef INT_DEBUG
	
		if ( (LLDInactivityTimeOut == 0) && (LLDSniffTime == 0) &&
			  ((status & DSR_BIT) == 0) && ((status & CTS_BIT) == 0))
			__asm int 3
#endif
	}

	/* Did the ERI signal change state? */
	if (delta & DELTA_ERI)
	{
		OutChar ('E', MAGENTA+WHITE_BACK);
		OutChar ('R', MAGENTA+WHITE_BACK);
		OutChar ('I', MAGENTA+WHITE_BACK);
		if (status & RI_BIT)
			OutChar ('H', MAGENTA+WHITE_BACK);
		else
			OutChar ('L', MAGENTA+WHITE_BACK);
		OutChar (' ', MAGENTA+WHITE_BACK);
	}

	return;
}


/***************************************************************************/
/*                                                                         */
/* isrNIL () 		                                                         */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION: This routine should never be called unless the UART is     */
/*              generating bogus interrupts, or reads from the interrupt	*/
/*              identification register are bad.									*/
/*                                                                         */
/***************************************************************************/

void isrNIL (void)
{
	OutChar ('I', MAGENTA+BLINK+WHITE_BACK);
	OutChar ('S', MAGENTA+BLINK+WHITE_BACK);
	OutChar ('R', MAGENTA+BLINK+WHITE_BACK);
	OutChar ('N', MAGENTA+BLINK+WHITE_BACK);
	OutChar ('I', MAGENTA+BLINK+WHITE_BACK);
	OutChar ('L', MAGENTA+BLINK+WHITE_BACK);
	Clear_PendingUARTIntr ();
	return;
}


/***************************************************************************/
/*                                                                         */
/* isrError () 	                                                         */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION: This routine is called when the UART detects an error		*/
/*              condition (overrun error, parity error, framing error, or	*/
/*              break interrupt).														*/
/*                                                                         */
/***************************************************************************/

void isrError (void)
{
	unsigned_8	linestatus;

	linestatus = (unsigned_8)_inp (LineStatReg);
	CheckError (linestatus);
	return;
}


/***************************************************************************/
/*                                                                         */
/* outDummyChar ()                                                         */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION: This routine is called to write one character to the UART.	*/
/*              It is called by the function that sends the soft baud rate	*/
/*              command LLDSendSoftBaudCmd().										*/
/*                                                                         */
/***************************************************************************/

void outDummyChar (void)

{
 	_outp (TxRcvHoldReg, 0xAA);
}


/************************************************************************/
/*                                                                      */
/* isrTBE ()                                                            */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION: This routine processes the transmit buffer empty        */
/*              interrupt.  As long as we are not flow controlled, we	*/
/*              write as many characters as possible to the UART.  If	*/
/*              we finish transmitting the data for one packet and		*/
/*              there is another packet waiting for transmission, we		*/
/*              will start transmitting the new packet.  This routine	*/
/*              also checks for flow control before transmitting any		*/
/*              characters.  If CTS is low, we don't send any				*/
/*              characters and we disable the transmit buffer empty		*/
/*              interrupt.																*/
/*                                                                      */
/************************************************************************/

void isrTBE (void)

{  
	unsigned_16	TxCnt;	/* # bytes to XMIT during this interrupt	*/
	FLAGS			oldflag;

#if TX_DEBUG
	OutChar ('T', BROWN);
#endif

	/*----------------------------------------*/
	/* Make sure the FIFO is empty.  Data		*/
	/* transmission is driven by two				*/
	/* different interrupts, UART interrupts	*/
	/* and LLDPoll interrupts.  The UART		*/
	/* interrupts are FIFO empty and modem		*/
	/* status interrupts.  The side effect		*/
	/* from these routines being called at		*/
	/* about the same time, is the FIFO data	*/
	/* being overwritten.  By added a check	*/
	/* to make sure the FIFO is empty, we can	*/
	/* avoid this side effect.						*/
	/*----------------------------------------*/
	DisableRTS ();
	oldflag = PreserveFlag ();
	DisableInterrupt ();
	if ( !((unsigned_8)_inp (LineStatReg) & THRE)) 
	{
		OutChar ('N', YELLOW);
		OutChar ('o', YELLOW);
		OutChar ('T', YELLOW);
		OutChar ('B', YELLOW);
		OutChar ('E', YELLOW);
		RestoreFlag (oldflag);
		EnableRTS ();
		return;
	}

	/*-------------------------------------*/
	/* Protection against missing transmit	*/
	/* buffer empty interrupts.				*/
	/*-------------------------------------*/
	tbeCount = 0;

	/*----------------------------------------------------*/
	/* Are there any packets waiting to be transmitted?	*/
	/*----------------------------------------------------*/
	if (uartTx.sState != TxING_DATA)
	{
		if ((uartTx.sQPtr = SLLDGetABuf (&uTxRdyList, 0)) != NIL)
		{
			OutChar ('-', BROWN);
			OutChar ('q', BROWN);
			uartTx.sTxCnt		= uartTx.sQPtr->qDataLen;
			uartTx.sDBPtr		= uartTx.sQPtr->qDataPtr;
			uartTx.sState = TxING_DATA;

#if TX_DEBUG
			OutChar ('S', BROWN);
			OutChar ('O', BROWN);
			OutChar ('H', BROWN);
#endif
		}
		else
		{
			/*----------------------------------------------*/
			/* We have nothing to transmit, so disable the	*/
			/* transmit buffer empty interrupt and return.	*/
			/*----------------------------------------------*/
			EnableUARTRxInt ();
#if TX_DEBUG
			OutChar ('t', BROWN);
#endif
			RestoreFlag (oldflag);
			EnableRTS ();
			return;
		}
	}

	TxCnt = TxFIFODepth;

	/*-------------------------------------------*/
	/* If there are more bytes to be transferred	*/
	/* than FIFO can hold, then transmit ONLY 1	*/
	/* or 16 bytes depending on the UART chip.	*/
	/*-------------------------------------------*/
	if (uartTx.sTxCnt > TxCnt)
	{
		while (TxCnt--)
		{	
			/*----------------------------------------------------*/
			/* When CTS is low, we are flowed controlled.  This	*/
			/* means the RFNC cannot accept any more characters.	*/
			/* Disable the transmit buffer empty interrupt.			*/
			/*----------------------------------------------------*/
			if ( !(((unsigned_8)_inp(ModemStatReg)) & CTS_BIT))
			{
				OutChar ('T', BLACK+WHITE_BACK);
				EnableUARTRxInt ();
#if TX_DEBUG
				OutChar ('t', BROWN);
#endif
				RestoreFlag (oldflag);
				EnableRTS ();
				return;
			}

#if TX_DEBUG
			OutHex(*uartTx.sDBPtr, BLUE+WHITE_BACK);
#endif
			_outp (TxRcvHoldReg, *uartTx.sDBPtr++);
			uartTx.sTxCnt--;

		}
#if TX_DEBUG
		OutChar ('t', BROWN);
#endif
		RestoreFlag (oldflag);
		EnableRTS ();
		return;
	}

	/*----------------------------------------------*/
	/* We will be able to finish transmitting			*/
	/* this packet.  Transmit the remaining bytes,	*/
	/* and if the FIFO is not full, see if there is	*/
	/* another packet waiting to be transmitted.	  	*/
	/*----------------------------------------------*/

	while (TxCnt--)
	{
		/*----------------------------------------------------*/
		/* When CTS is low, we are flowed controlled.  This	*/
		/* means the RFNC cannot accept any more characters.	*/
		/* Disable the transmit buffer empty interrupt.			*/
		/*----------------------------------------------------*/
		if ( !(((unsigned_8)_inp(ModemStatReg)) & CTS_BIT))
		{
			OutChar ('T', BLACK+WHITE_BACK);
			EnableUARTRxInt ();
#if TX_DEBUG
			OutChar ('t', BROWN);
#endif
			RestoreFlag (oldflag);
			EnableRTS ();
			return;
		}

		/*-------------------------------------*/
		/*	Write one character into the FIFO.	*/
		/*-------------------------------------*/
#if TX_DEBUG
		OutHex(*uartTx.sDBPtr, BLUE+WHITE_BACK);
#endif
		_outp (TxRcvHoldReg, *uartTx.sDBPtr++);
		uartTx.sTxCnt--;

		/*---------------------------------------------------*/
		/* Any bytes left in the current packet to transmit? */
		/*---------------------------------------------------*/
		if (uartTx.sTxCnt == 0)
		{

			OutChar ('e', BLACK + GREEN_BACK);
#if TX_DEBUG
			OutHex (*(uartTx.sDBPtr-3), GREEN+BROWN_BACK);
			OutHex (*(uartTx.sDBPtr-2), GREEN+BROWN_BACK);
#endif

			/* Return the buffer to the buffer free list. */
			SLLDPutABuf (&uFreeList, uartTx.sQPtr);

			/*-------------------------------------------------*/
			/* There are no more bytes left in this				*/
			/* in this buffer, so see if there are					*/
			/* any other packets waiting to be transmitted.		*/
			/*-------------------------------------------------*/
			if ((uartTx.sQPtr = SLLDGetABuf (&uTxRdyList, 0)) != NIL)
			{
				OutChar ('-', BROWN);
				OutChar ('q', BROWN);

				uartTx.sTxCnt		= uartTx.sQPtr->qDataLen;
				uartTx.sDBPtr		= uartTx.sQPtr->qDataPtr;

#if TX_DEBUG
				OutChar ('S', BROWN);
				OutChar ('O', BROWN);
				OutChar ('H', BROWN);
#endif
			}
			else
			{
				/*----------------------------------------*/
				/* No more buffers to transmit, so reset	*/
				/* the transmit control structure.			*/
				/*----------------------------------------*/
				memset (&uartTx, 0, sizeof (uartTx));
#if TX_DEBUG
				OutChar ('t', BROWN);
#endif
				RestoreFlag (oldflag);
				EnableRTS ();
				return;
			}
		}

	}

#if TX_DEBUG
			OutChar ('t', BROWN);
#endif

	RestoreFlag (oldflag);
	EnableRTS ();
}


/************************************************************************/
/*                                                                      */
/* isrRxRDY ()                                                          */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION:  This routine processes multiple receive characters and	*/
/*               possibly some receive errors.  Once we have read in		*/
/*               an entire packet, we call LLDIsr () to process the		*/
/*               packet.  When receiving a packet, we don't copy the		*/
/*               packet header or trailer into the receive buffer, we	*/
/*               just copy the data.												*/
/*                                                                      */
/************************************************************************/

void isrRxRDY (void)

{  unsigned_8  linestatus;
	unsigned_8 IncomingByte;
	int		  tries = 30;		/* Incase of a hardware failure. */

	/*-------------------------------------------------------------------*/
	/* Make it a loop so we can empty the FIFO and/or catch any incoming	*/
	/* characters.  When the number of characters in the FIFO drop			*/
	/* below the trigger level, a character receive interrupt will no		*/
	/* longer be pending.  However, if there are still some characters	*/
	/* in the FIFO, the data ready bit in the line status register be		*/
	/* set.  In this case, we want to read in characters as long as		*/
	/* there are characters in the FIFO and not just a receive 				*/
	/* interrupt pending.																*/
	/*																							*/
	/* We must also process any receiver errors because these get			*/
	/* cleared when we read the line status registers.							*/
	/*-------------------------------------------------------------------*/
	
	while (((linestatus = (unsigned_8)_inp (LineStatReg)) & RXINT) && --tries)
	{

		/*-------------------------------------------------------*/
		/* Process any receive character errors.  This routine	*/
		/* resets the receive state and if necessary, reads in	*/
		/* the bad character.												*/
		/*-------------------------------------------------------*/
		if (linestatus & RXERROR)
		{
			CheckError (linestatus);
			continue;
		}

		IncomingByte = (unsigned_8) _inp (TxRcvHoldReg);

		switch (uartRx.rState)
		{
			case WAIT_SOH:
				/*-------------------------------------------*/
				/* We are starting to receiving a new packet.	*/
				/* There is just one receive buffer,			*/
				/* assume that it is free.  We emptied			*/
				/* the buffer when we processed the last		*/
				/* receive packet.									*/
				/*-------------------------------------------*/

				/* If it isn't the start-of-header character, ignore it. */
				if (IncomingByte != SOH)
				{
#if RX_DEBUG
					OutHex (IncomingByte, GREEN);
#endif
					return;
				}

#if 1
				OutChar ('R', BLACK+WHITE_BACK);
				OutChar ('x', BLACK+WHITE_BACK);
#endif

				/*------------------------------------------*/
				/* Reset the data pointer to the beginning  */
				/* of the receive buffer.		 				  */
				/*------------------------------------------*/
				uartRx.rDBPtr     = uartRx.rQPtr->qDataPtr;
				uartRx.rState     = RxING_HDR;

				/*----------------------------------------*/
				/* Set rPosition to 1 to count the number	*/
				/* of header length bytes we are going to */
				/* receive.  The length is 2-bytes.			*/
				/*----------------------------------------*/
				uartRx.rPosition  = 1;

				/*----------------------------------------*/
				/* Clear rCheckAcc.  We use it while		*/
				/* receiving data.  It contains the data	*/
				/* checksum.										*/
				/*----------------------------------------*/
				uartRx.rCheckAcc  = 0;
				break;

			case RxING_HDR:
				/*---------------------------------*/
				/* Receive the 2-byte data length. */
				/*---------------------------------*/
#if RX_DEBUG
				OutHex (IncomingByte, CYAN);
#endif

				/* Is this the 1st length byte? */
				if (uartRx.rPosition < HDRSIZE)
				{
					uartRx.rDataLen = (unsigned_16) 
						((unsigned_8)IncomingByte << 8) & 0xff00;
					++uartRx.rPosition;

					/*-------------------------------------------------*/
					/* Save the LenHi and LenLow bytes for computing a	*/
					/* header checksum on the length bytes.				*/
					/*-------------------------------------------------*/
					LenHi = IncomingByte;
				}
				else
				{
					LenLow = IncomingByte;
					uartRx.rDataLen |= (((unsigned_16) IncomingByte) & 0x00ff);
				
					/*----------------------------------------*/
					/* Make sure the length is reasonable and	*/
					/* the packet will fit into the receive	*/
					/* buffer.											*/
					/*----------------------------------------*/
					if ((uartRx.rDataLen > MAX_PROX_PKT_SIZE) ||
					    (uartRx.rDataLen < MIN_PROX_PKT_SIZE))
					{
						OutChar ('B', RED);
						OutChar ('a', RED);
						OutChar ('d', RED);
						OutChar ('R', RED);
						OutChar ('x', RED);
						OutChar ('L', RED);
						OutChar ('e', RED);
						OutChar ('n', RED);
						OutHex ((unsigned_8)(uartRx.rDataLen >> 8), RED);
						OutHex ((unsigned_8)(uartRx.rDataLen), RED);
						SLLDRxLenErr++;
						SLLDRxErrors++;
#if INT_DEBUG
						if (SLLDOverrunErr == 0)
							__asm	int 3
#endif
						SLLDResetReceiver ();
						continue;
					}

					/*-------------------------------------------*/
					/* Reset rPosition again to start counting	*/
					/* the data bytes we are about to receive.	*/
					/* The rDataLen that we just received is		*/
					/* the length of the DATA section only.		*/
					/* It does not include the checksum.			*/
					/*-------------------------------------------*/
					uartRx.rState = RxING_HDRCHK;
					uartRx.rPosition = 1;
				}
				break;

			case RxING_HDRCHK:
				/*----------------------------------------*/
				/* Read in the header checksum.  This is	*/
				/* a check sum of the data length bytes.	*/
				/*														*/
				/* Checksum := ~(LENlow) + ~(LENhi)			*/
				/*----------------------------------------*/

#if RX_DEBUG
				OutHex (IncomingByte, BROWN+WHITE_BACK);
#endif

				if (IncomingByte != (unsigned_8) (~LenLow + ~LenHi))
				{
					OutChar ('H', RED);
					OutChar ('D', RED);
					OutChar ('R', RED);
					OutChar ('C', RED);
					OutChar ('s', RED);
					OutChar ('u', RED);
					OutChar ('m', RED);
					OutChar ('E', RED);
					OutChar ('r', RED);
					OutChar ('r', RED);
					SLLDRxHdrErr++;
					SLLDRxErrors++;
#if INT_DEBUG
						if (SLLDOverrunErr == 0)
							__asm	int 3
#endif
					SLLDResetReceiver ();
					continue;
				}           

				/*-------------------------------------------*/
				/* The header checksum was good, so switch	*/
				/* to the receiving data state.		 			*/
				/*-------------------------------------------*/
				uartRx.rState     = RxING_DATA;
				break;

			case RxING_DATA:
				/*-------------------------*/
				/* Read in the data bytes.	*/
				/*-------------------------*/

#if RX_DEBUG
				OutHex (IncomingByte, WHITE);
#endif

				uartRx.rCheckAcc += IncomingByte;
				*uartRx.rDBPtr++ = IncomingByte;

				/*----------------------------------------*/
				/* Have we read in all of the bytes that	*/
				/* the packet header length said we were	*/
				/* going to receive?								*/
				/*----------------------------------------*/
				if (uartRx.rPosition < uartRx.rDataLen)
				{
					uartRx.rPosition++;
				}
				else 
				{
					/*-------------------------------------------*/
					/* End of data receiving state.  Prepare		*/
					/* to receive the checksum.  Reset rPosition	*/
					/* to count the number of checksum bytes.		*/
					/*-------------------------------------------*/
					uartRx.rState    = RxING_CHKSUM;
					uartRx.rPosition = 1;      
				}
				break;

			case RxING_CHKSUM:
				/*----------------------------------*/
				/* Read in the final data checksum.	*/
				/* This is a 2-byte sum of all the	*/
				/* data bytes.								*/
				/*----------------------------------*/

#if RX_DEBUG
				OutHex (IncomingByte, YELLOW);
#endif

				/* Is this the 1st checksum byte? */
				if (uartRx.rPosition < CHKSUMSIZE)
				{
					uartRx.rChecksum = (unsigned_16)((unsigned_8)IncomingByte << 8);
					++uartRx.rPosition;
				}
				else
				{
					uartRx.rChecksum |= (unsigned_16) IncomingByte;
					if (uartRx.rChecksum != uartRx.rCheckAcc)
					{
						OutChar ('C', RED);
						OutChar ('s', RED);
						OutChar ('u', RED);
						OutChar ('m', RED);
						OutChar ('E', RED);
						OutChar ('r', RED);
						OutChar ('r', RED);
	
						SLLDRxChksumErr++;
						SLLDRxErrors++;
#if INT_DEBUG
						if (SLLDOverrunErr == 0)
							__asm	int 3
#endif
						SLLDResetReceiver ();
						continue;
					}

					/*----------------------------------------------------*/
					/* Tell the RFNC card to stop sending us characters	*/
					/* while we process the newly received packet.			*/
					/*----------------------------------------------------*/
					DisableRTS();

					/*-------------------------------------------*/
					/* The following variables are going to be	*/
					/* used by the RxData() and GetRxData()		*/
					/* during LLDIsr() call.							*/
					/*															*/
					/* rDBPtr points to beginning of DATA.			*/
					/* rDataLen already contains the length of	*/
					/* the data portion.									*/
					/*-------------------------------------------*/
					uartRx.rDBPtr = uartRx.rQPtr->qDataPtr;

					LLDIsr ();
					EnableRTS();
				}
				break;

			default:
				/*-------------------------------------*/
				/* This is an invalid receive state.	*/
				/*-------------------------------------*/
				OutChar ('I', RED+BLINK);
				OutChar ('n', RED+BLINK);
				OutChar ('V', RED+BLINK);
				OutChar ('a', RED+BLINK);
				OutChar ('l', RED+BLINK);
				OutChar ('S', RED+BLINK);
				OutChar ('t', RED+BLINK);
				OutHex (uartRx.rState, RED+BLINK);
#if INT_DEBUG
		__asm	int 3
#endif
				SLLDResetReceiver ();
				break;


		}
	}
}



/************************************************************************/
/*                                                                      */
/* SLLDIsr (void)                                                       */
/*                                                                      */
/* INPUT:                                                               */
/*                                                                      */
/* OUTPUT:                                                              */
/*                                                                      */
/* DESCRIPTION: This is serial LLD's Interrupt Service Routine.         */
/*              This routine acts like an interrupt administrator.  		*/
/*              When called, it reads the Interrupt Identification		*/
/*              register to identify the event that requires service.	*/
/*              Then it calls the correct interrupt service routine to  */
/*              process the interrupt.  We do not exit this routine		*/
/*              until we have clear all of the UART interrupts.			*/
/*                                                                      */
/************************************************************************/

void SLLDIsr (void)

{  
	unsigned_8  IntrIndex;
	FLAGS			oldflag;

	InSerialISRFlag = SET;

	/*-------------------------------------------------*/
	/* The following steps are recommended by the ODI  */
	/* driver spec:                                    */
	/*-------------------------------------------------*/
	PushRFNCInt ();            /* Save the current interrupt state.      */
	DisableRFNCInt ();         /* Disable PIC interrupts. 				   */
	oldflag = PreserveFlag (); /* Save the current processor flags.      */
	EOI (LLDIntLine1);         /* Issued an EOI to clear the PIC         */
										/* interrupt.                             */
	EnableInterrupt ();        /* Enable system interrupts (STI)         */


#if ISR_DEBUG
	OutChar ('S', BROWN);
#endif

	for (;;)
	{

		IntrIndex = (unsigned_8) (_inp (IntrIdentReg) & INT_BITS_MASK);
		if (IntrIndex & NO_INT_PENDING)
		{
			break;
		}
		else
		{
#if ISR_DEBUG
	OutChar('i', BROWN);
#endif

			/*-------------------------------------------------------*/
			/* A 0 in Bit 0 of Interrupt Identification register     */
			/* means that an interrupt is pending; bits 1-3          */
			/* identify the actual interrupt.  Each interrupt has    */
			/* a priority: while an interrupt is pending,            */
			/* interrupt with an equal or lower priority are not     */
			/* reported.                                             */
			/*                                                       */
			/*    Bit-2    Bit-1    Bit-0    Interrupt ID            */
			/*      0        0        1      None                    */
			/*                                                       */
			/* (hi) 1        1        0      Serialization Err/BREAK */
			/*      1        0        0      Received Data           */
			/*      0        1        0      Transmitter Buffer Empty*/
			/*      0        0        0      Modem Status            */
			/*																			*/
			/*-------------------------------------------------------*/
			IntrIndex >>= 1;  
			(*uartISR [ (IntrIndex & 7) ]) ();
		}
	}

	/* Make sure we don't leave a pending transmit buffer empty. */
	if ( (_inp(LineStatReg) & THRE) && (uartTx.sTxCnt != 0) )
	{
		Delay_uS (1);

		/*----------------------------------------------------------*/
		/* If we are not flow controlled, transmit more characters,	*/
		/* otherwise disable the transmit buffer empty interrupt.	*/
		/*----------------------------------------------------------*/
		if (_inp(ModemStatReg) & CTS_BIT)
			isrTBE();
		else
			EnableUARTRxInt ();
	}

	RestoreFlag (oldflag);
	PopRFNCInt ();

#if ISR_DEBUG
	OutChar ('s', BROWN);
#endif
	InSerialISRFlag = CLEAR;

}




/***************************************************************************/
/*                                                                         */
/* CheckError (linestatus)                                                 */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION:   This routine processes receive errors and resets the		*/
/*                receiver state machine.  For the parity error, framing	*/
/*                error, and break error, we also read and discard the		*/
/*                errored character.													*/
/*                                                                         */
/***************************************************************************/

unsigned_32 SLLDParityErr = 0;
unsigned_32 SLLDFramingErr = 0;
unsigned_32 SLLDBreakErr = 0;

void CheckError (unsigned_8 linestatus)

{
	unsigned_8 IncomingByte;

	/*----------------------------------------------*/
	/* Since at least one byte in the packet has		*/
	/* been lost, reset the receive control block	*/
	/* to drop the current packet.						*/
	/*----------------------------------------------*/
	SLLDResetReceiver ();
	SLLDRxErrors++;

	if (linestatus & OVERRUN)
	{
		OutChar ('O', RED+BLINK+WHITE_BACK);
		OutChar ('v', RED+BLINK+WHITE_BACK);
		OutChar ('e', RED+BLINK+WHITE_BACK);
		OutChar ('r', RED+BLINK+WHITE_BACK);
		OutChar ('r', RED+BLINK+WHITE_BACK);
		OutChar ('u', RED+BLINK+WHITE_BACK);
		OutChar ('n', RED+BLINK+WHITE_BACK);
		SLLDOverrunErr++;
	}

	if (linestatus & PARITY)
	{
		IncomingByte = (unsigned_8) _inp (TxRcvHoldReg);
		OutChar ('P', RED+BLINK+WHITE_BACK);
		OutChar ('a', RED+BLINK+WHITE_BACK);
		OutChar ('r', RED+BLINK+WHITE_BACK);
		OutChar ('i', RED+BLINK+WHITE_BACK);
		OutChar ('t', RED+BLINK+WHITE_BACK);
		OutChar ('y', RED+BLINK+WHITE_BACK);
		SLLDParityErr++;
	}

	if (linestatus & FRAMING)
	{
		IncomingByte = (unsigned_8) _inp (TxRcvHoldReg);
		OutChar ('F', RED+BLINK+WHITE_BACK);
		OutChar ('r', RED+BLINK+WHITE_BACK);
		OutChar ('a', RED+BLINK+WHITE_BACK);
		OutChar ('m', RED+BLINK+WHITE_BACK);
		OutChar ('i', RED+BLINK+WHITE_BACK);
		OutChar ('n', RED+BLINK+WHITE_BACK);
		OutChar ('g', RED+BLINK+WHITE_BACK);
		SLLDFramingErr++;
	}

	if (linestatus & BREAKDETECT)
	{
		IncomingByte = (unsigned_8) _inp (TxRcvHoldReg);
		OutChar ('B', RED+BLINK+WHITE_BACK);
		OutChar ('r', RED+BLINK+WHITE_BACK);
		OutChar ('e', RED+BLINK+WHITE_BACK);
		OutChar ('a', RED+BLINK+WHITE_BACK);
		OutChar ('k', RED+BLINK+WHITE_BACK);
		SLLDBreakErr++;
	}

}



/***************************************************************************/
/*                                                                         */
/* SLLDResetReceiver	()																	   */
/*                                                                         */
/* INPUT:                                                                  */
/*                                                                         */
/* OUTPUT:                                                                 */
/*                                                                         */
/* DESCRIPTION:   This routine resets the structure used to read in			*/
/*                packets from the serial port.										*/
/*                                                                         */
/***************************************************************************/


void SLLDResetReceiver ()

{
	uartRx.rState     = WAIT_SOH;    /* Initialize the receiving state.  */
	uartRx.rPosition  = 0;           /* Position for counting receive    */
												/* characters.  When position       */
												/* equals rDataLen, we have         */
												/* received all of the data.        */
	uartRx.rDataLen   = 0;           /* The data length, in bytes.       */
	uartRx.rChecksum  = 0;           /* Data checksum                    */
	uartRx.rCheckAcc  = 0;           /* Data checksum accumulator        */
	uartRx.rDBPtr     = uartRx.rQPtr->qDataPtr;  /* Data buffer pointer. */
}

