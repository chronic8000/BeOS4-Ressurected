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
/*	$Header:   J:\pvcs\archive\clld\slldport.c_v   1.21   14 Jun 1996 16:24:56   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldport.c_v  $										*/
/* 
/*    Rev 1.21   14 Jun 1996 16:24:56   JEAN
/* Comment changes.
/* 
/*    Rev 1.20   16 Apr 1996 11:32:06   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.19   29 Mar 1996 11:49:56   JEAN
/* Removed a check in DisableRTS () to see if the card is asleep.
/* For power management, we need to toggle RTS to wake the card up.
/* 
/*    Rev 1.18   22 Mar 1996 14:53:56   JEAN
/* Added functions to remove/add the IO address from the 
/* BIOS table upon entry/exit.
/* 
/*    Rev 1.17   06 Feb 1996 14:30:06   JEAN
/* Many changes to improve performance.
/* 
/*    Rev 1.16   31 Jan 1996 13:10:08   JEAN
/* Changed the ordering of some header include files, reduced
/* the delay between UART register accesses (2 us to 1 us), removed
/* the SLLDTxData function.
/* 
/*    Rev 1.15   08 Jan 1996 14:20:32   JEAN
/* Added a 2 microsecond delay between UART register accesses,
/* fixed a bug in the enable/disable RFNC interrupt routines
/* where we would only write to PIC 1, and changed some defines
/* to all upper caps.
/* 
/*    Rev 1.14   19 Dec 1995 18:13:28   JEAN
/* Added a header include file.
/* 
/*    Rev 1.13   14 Dec 1995 15:17:38   JEAN
/* Added support for multiple serial cards.
/* 
/*    Rev 1.12   07 Dec 1995 12:16:14   JEAN
/* Changed DisableRTS() to check the state of DSR before disabling
/* RTS and changed LLDOutOfStandby() to call SetWakeupBit().
/* 
/*    Rev 1.11   27 Nov 1995 14:49:36   JEAN
/* Added a flag so DisableRTS() will know when we are running a
/* standby test.
/* 
/*    Rev 1.10   17 Nov 1995 16:39:30   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   12 Oct 1995 11:44:24   JEAN
/* Removed unused functions, added functions to enable/disable RTS,
/* and fixed some compiler warnings.
/* 
/*    Rev 1.8   25 Jun 1995 09:16:34   tracie
/* added more comments only.
/* 																								*/
/*    Rev 1.7   23 May 1995 09:47:36   tracie										*/
/* UART16550 support																			*/
/* 																								*/
/*    Rev 1.6   24 Apr 1995 15:51:22   tracie										*/
/* 																								*/
/*    Rev 1.5   13 Apr 1995 08:49:38   tracie										*/
/* XROMTEST version and NS16550 FIFO mode												*/
/* 																								*/
/*    Rev 1.5   16 Mar 1995 16:16:18   tracie										*/
/* Added support for ODI																	*/
/* 																								*/
/* Initial revision.																			*/
/*																									*/
/*																									*/
/***************************************************************************/

#include	<dos.h>
#include	<conio.h>

#include "constant.h"	
#include "lld.h"
#include "asm.h"
#include "bstruct.h"
#include "slld.h"
#include "slldbuf.h"

#ifdef MULTI
#include "multi.h"
#endif

#include "slldhw.h"
#include "slldport.h"


/***************************************************************************/
/*																									*/
/*										Global Variables										*/
/*																									*/
/***************************************************************************/
#ifdef MULTI

extern struct LLDVarsStruct	*LLDVars;

#else

extern unsigned_16 	uartBaseAddr;	/* UART's base address						*/
extern unsigned_16 	TxRcvHoldReg;	/* Transmit & Receive Holding Reg.- R0	*/
extern unsigned_16 	IntrEnableReg;	/* Interrupt Enable Register		 - R1	*/
extern unsigned_16 	IntrIdentReg;	/* Interrupt Identification Reg.	 - R2 */
extern unsigned_16 	LineCtrlReg;	/* Line Control/Data Format Reg.	 - R3	*/
extern unsigned_16 	ModemCtrlReg;	/* Modem Control/RS-232 Output Crl- R4 */
extern unsigned_16 	LineStatReg;	/* Line Status/Serialization Stat - R5 */
extern unsigned_16 	ModemStatReg;	/* Modem Status/RS-232 Input Stat - R6	*/
extern unsigned_8		IRQLevel;  		/* The interrupt level selected	 		*/										
extern unsigned_8		IRQMask;	  		/* The interrupt controller mask bit.	*/
extern unsigned_8		IRQMaskReg;    /* The interrupt mask register address.*/
extern unsigned_8		ModemCtrlVal;	/* Modem control value                 */

extern unsigned_8		IntrOutVal; 		/* Indicates if 8259 interrupts are	*/
													/* enabled or disabled.					*/
extern unsigned_8		IntStackPtr;		/* Current stack pointer				*/		
extern unsigned_8		IntStack [32];		/* To keep track of 8259 interrupts.*/

extern unsigned_8		serialInStandby;	/* Used when testing standby mode	*/
												 	/* with RL2DIAG.			 				*/	

extern unsigned_8				RestoreBIOSTableFlag;
extern unsigned_16 __far	*SaveBIOSAddr;

#endif


/***************************************************************************/
/*																									*/
/*	SetBit_ModemCtrl (Value )																*/
/*																									*/
/*	INPUT:			Value - The bits to "OR" to the Modem Control Register	*/
/*																									*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will "OR" the value passed in with the			*/
/*					 current value of the UART modem control register.				*/
/*																									*/
/***************************************************************************/

void SetBit_ModemCtrl (unsigned_8 Value)

{ 
	ModemCtrlVal = (unsigned_8) (((unsigned_8) _inp (ModemCtrlReg)) | Value);
	Delay_uS (1);
	_outp (ModemCtrlReg, ModemCtrlVal);
}


/***************************************************************************/
/*																									*/
/*	ClrBit_ModemCtrl (Value)																*/
/*																									*/
/*	INPUT:			Value - The bits to be clear in Modem Control Register	*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine will invert the value passed in and "AND"	*/
/*						it with the current value of the UART modem control		*/
/*                register.																*/
/*																									*/
/***************************************************************************/

void ClrBit_ModemCtrl (unsigned_8 Value)

{ 
	ModemCtrlVal = (unsigned_8) (((unsigned_8)_inp (ModemCtrlReg)) & ~Value);
	Delay_uS (1);
	_outp (ModemCtrlReg, ModemCtrlVal); 	
}


/***************************************************************************/
/*																									*/
/*	Clear_PendingUARTIntr () 														 		*/
/*																									*/
/*	INPUT:		  																				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine will clear any pending interrupts from the	*/
/*                UART by reading register R0 - R5/R6.	 						*/
/*																									*/
/***************************************************************************/

void Clear_PendingUARTIntr (void)		

{	
	unsigned_16 dummy;

	dummy = _inp (TxRcvHoldReg);
	Delay_uS (1);
	dummy = _inp (IntrEnableReg);
	Delay_uS (1);
	dummy = _inp (IntrIdentReg);
	Delay_uS (1);
	dummy = _inp (LineCtrlReg);
	Delay_uS (1);
	dummy = _inp (LineStatReg);
	Delay_uS (1);
	dummy = _inp (ModemStatReg);
	Delay_uS (1);
	ModemCtrlVal = (unsigned_8) _inp (ModemCtrlReg);
}


/***************************************************************************/
/*																									*/
/*	EnableUARTModemCtrl ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine enables UART modem control.  GP02 is a			*/
/*              general purpose output that allows the interrupt to be		*/
/*              seen.  RTS tells the RFNC that it can transmit characters. */
/*              This function should be called upon entry.						*/
/*																									*/
/***************************************************************************/

void EnableUARTModemCtrl (void)

{
	ModemCtrlVal = GPO2 | RTS;
	_outp (ModemCtrlReg, ModemCtrlVal);
}


/***************************************************************************/
/*																									*/
/*	DisableUARTModemCtrl ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine disables UART modem control.  GPO2 is set to	*/
/*              zero so we won't see any interrupts and RTS is clear so 	*/
/*              the RFNC won't send us any characters.  This function		*/
/*              should be called upon exit.											*/
/*																									*/
/***************************************************************************/

void DisableUARTModemCtrl (void)

{
	ModemCtrlVal = 0;
	_outp (ModemCtrlReg, ModemCtrlVal);
}



/***************************************************************************/
/*																									*/
/*	EnableUARTRxInt ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine enables UART receive, modem status, and line	*/
/*              status interrupts.  Call this routine to disable transmit	*/
/*              buffer empty interrupts while leaving the receive, modem	*/
/*              status, and line status interrupts enabled.						*/
/*																									*/
/***************************************************************************/

void EnableUARTRxInt (void)

{
	_outp (IntrEnableReg, RXRDY_EN | MODEMSTAT_EN | LINESTAT_EN);
}

/***************************************************************************/
/*																									*/
/*	EnableUARTRxTxInt ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine enables UART receive, transmit buffer empty,	*/
/*              modem status, and line status interrupts. 						*/
/*																									*/
/***************************************************************************/

void EnableUARTRxTxInt (void)

{
	_outp (IntrEnableReg, TBE_EN | RXRDY_EN | MODEMSTAT_EN | LINESTAT_EN);
}



/***************************************************************************/
/*																									*/
/*	DisableUARTInt ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine disables all UART interrupts.  These are the	*/
/*					 receive interrupt, transmit buffer empty interrupt, the 	*/
/*					 modem status interrupt, and the line status interrupt.		*/
/*																									*/
/***************************************************************************/

void DisableUARTInt (void)

{	
	_outp (IntrEnableReg, 0);
}


/***************************************************************************/
/*																									*/
/* SetBaudRate (Divisor) 																	*/
/*																									*/
/* INPUT:		Divisor - The value written to the UART to set it to a 		*/
/*								 particular baudrate.										*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: The baud rate is changed by writing lowbaud and hibaud to	*/
/*					 UART's LSB and MSB Baud registers.  These registers can be	*/
/*					 addressed only if bit 7 in the Data Format/Line Control		*/
/*					 register is TRUE.  After writing to these two registers, 	*/
/*					 the original contents of the Data Format register are 		*/
/*					 restored.																	*/
/***************************************************************************/

void SetBaudRate (unsigned_16 Divisor)

{
	_outp (LineCtrlReg, DLAB);
	Delay_uS (1);
	_outp (TxRcvHoldReg,(unsigned_8) Divisor);
	Delay_uS (1);
	_outp (IntrEnableReg, ((unsigned_8) Divisor >> 8));
	Delay_uS (1);
	_outp (LineCtrlReg, ~DLAB);
}



/***************************************************************************/
/*																									*/
/* DisableRTS ()																				*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION:  This routine prevents the RFNC card from sending us 		*/
/*               characters.  If interrupts are disabled for a period of	*/
/*               time and we do not disable RTS, we will get FIFO overrun	*/
/*               errors.																	*/
/*																									*/
/*               This routine also enables us to take the card out of		*/
/*               standby.  The card can be taken out of standby by			*/
/*               toggling RTS.  When CTS and DSR are both low, the card is	*/
/*               either in sniff mode or in standby.								*/
/*																									*/
/*               RFNC Flow Control ON - don't send the driver characters.	*/
/*																									*/
/***************************************************************************/

void DisableRTS (void)
{
	/*-------------------------------------------*/
	/* If we are in a standby test for RL2DIAG,	*/
	/* we don't want to toggle RTS.					*/
	/*-------------------------------------------*/
	if (!serialInStandby)
	{
		_outp (ModemCtrlReg, ModemCtrlVal &= ~RTS);  
	}
}



/***************************************************************************/
/*																									*/
/* EnableRTS ()																				*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION:  This routine allows the RFNC card to send us characters.	*/
/*																									*/
/*               RFNC Flow Control OFF - send the driver characters.			*/
/*																									*/
/***************************************************************************/

void EnableRTS (void)
{
	_outp (ModemCtrlReg, ModemCtrlVal |= RTS);
}



/***************************************************************************/
/*																						  			*/
/*	EOI (irq)																		  			*/
/*																						  			*/
/*	INPUT:	irq -	The interrupt that the card is using (0 - 15). 				*/
/*																					 	 			*/
/*	OUTPUT:																		  				*/
/*																					  				*/
/*	DESCRIPTION: This routine will issue an EOI command to the interrupt		*/
/*              controller.  If we are clearing an interrupt on the second	*/
/*              controller, we need to write to both PICs to clear the		*/
/*              interrupt.													  	  			*/
/*																					 	 			*/
/***************************************************************************/

void EOI (unsigned_8 irq)

{
	if (irq > 7)
	{
		_outp (I8259CMD2, I8259EOI);
	}
	_outp (I8259CMD, I8259EOI);
}


/***************************************************************************/
/*																									*/
/*	LLDOutOfStandby ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will bring the serial card out of standby by	*/
/*              calling SetWakeupBit(), which toggles RTS.  	 				*/
/*																									*/
/***************************************************************************/

void LLDOutOfStandby (void)

{
	SetWakeupBit ();
}


/*-------------------------------------------------------------*/
/* RFNCInt functions:  For ISA/OEM/PCMCIA cards, there is an 	*/
/*                     interrupt register on the card.  The		*/
/*                     serial card does not have an interrupt	*/
/*                     register, therefore, we must enable/		*/
/*                     disable interrupt through the PIC			*/
/*                     controllers.										*/
/*-------------------------------------------------------------*/


/***************************************************************************/
/*																		  							*/
/*	PushRFNCInt ()													  							*/
/*																		  							*/
/*	INPUT:															  							*/
/*																		  							*/
/*	OUTPUT:															  							*/
/*																									*/
/*	DESCRIPTION: This routine preserves the current interrupt state. by		*/
/*              pushing it onto our "stack".  The state is either enabled	*/
/*              (SET) or disabled (CLEAR).									  		*/
/*																									*/
/***************************************************************************/

void PushRFNCInt (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	IntStack [IntStackPtr++] = IntrOutVal;

	RestoreFlag (oldflag);
}


/***************************************************************************/
/*																							 		*/
/*	PopRFNCInt ()																		 		*/
/*																								 	*/
/*	INPUT:																					 	*/
/*																							 		*/
/*	OUTPUT:																					 	*/
/*																								 	*/
/*	DESCRIPTION: This routine restores the state of our PIC interrupt	 		*/
/*	             by popping it from our "stack" and then masking or			*/
/*              unmasking the interrupt on the PIC controller.  The state 	*/
/*              is either enabled or disabled.  When our state is enabled,	*/
/*              we want to unmask the interrupt.  When our state is			*/
/*              disabled, we want to mask the interrupt.		  					*/
/*																									*/
/***************************************************************************/

void PopRFNCInt (void)

{	unsigned_8	val;
	FLAGS			oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*-------------------------------------------------*/
	/* Read in the current IRQ mask settings.  Other	*/
	/* applications may change this registers, so		*/
	/* we need to preserve all bits but the one we		*/
	/* are using.													*/
	/*-------------------------------------------------*/
	val = (unsigned_8) _inp (IRQMaskReg);
	IntrOutVal = IntStack [--IntStackPtr];

	/*-------------------------------------------------*/
	/* If IntrOutVal is zero, then we want our 8259		*/
	/* interrupt disabled.  To mask an interrupt on		*/
	/* the 8259, set the bit.  Note that IRQMaskReg		*/
	/* contains the address of either PIC 1 or PIC 2.	*/
	/*-------------------------------------------------*/
   if (IntrOutVal == CLEAR)
		_outp (IRQMaskReg, val | IRQMask);
	else
		_outp (IRQMaskReg, val & ~IRQMask);

	RestoreFlag (oldflag);
}


/***************************************************************************/
/*																									*/
/*	DisableRFNCInt ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine disables our 8259 PIC interrupt. 					*/
/*																				  					*/
/***************************************************************************/

void DisableRFNCInt (void)

{	unsigned_8	val;
	FLAGS			oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*-------------------------------------------*/
	/* Preserve all bits in the interrupt mask	*/
	/* register but our bit.  Clear the global	*/
	/* IntrOutVal, to indicate that the current	*/
	/* state is disabled.								*/
	/*-------------------------------------------*/
	val = (unsigned_8) _inp (IRQMaskReg);
	IntrOutVal = CLEAR;
	_outp (IRQMaskReg, val | IRQMask);

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																					 				*/
/*	EnableRFNCInt ()															 				*/
/*																					 				*/
/*	INPUT:																			 			*/
/*																						 			*/
/*	OUTPUT:																		 				*/
/*																					 				*/
/*	DESCRIPTION: This routine enables our 8259 PIC interrupt.	 				*/
/*																						 			*/
/***************************************************************************/

void EnableRFNCInt (void)

{	unsigned val;
	FLAGS		oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*-------------------------------------------*/
	/* Preserve all bits in the interrupt mask	*/
	/* register but our bit.  Set the global		*/
	/* IntrOutVal to indicate that our current	*/
	/* state is enabled.									*/
	/*-------------------------------------------*/
	val = (unsigned_8) _inp (IRQMaskReg);
	IntrOutVal = SET;
	_outp (IRQMaskReg, val & ~IRQMask);

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																					 				*/
/*	CheckBIOSMem ()															 				*/
/*																					 				*/
/*	INPUT:																			 			*/
/*																						 			*/
/*	OUTPUT:																		 				*/
/*																					 				*/
/*	DESCRIPTION: This function searches the BIOS Com port table to				*/
/*              determine if we need to remove our IO address.  If Windows	*/
/*              is started, it will initialize the Com port.  We don't		*/
/*              want this to happen.  By removing our IO address and			*/
/*              decrementing the number of serial ports, Windows won't		*/
/*              access our Com port.													*/
/*																						 			*/
/***************************************************************************/

void CheckBIOSMem (void)
{
	unsigned_16	orig;
	unsigned_16	serialPortCnt;
	int			ii;

	/*----------------------------------------------*/
	/* Search for our IO address in the BIOS table. */
	/*----------------------------------------------*/
	for (ii = 0, SaveBIOSAddr = _MK_FP(0, BIOS_COM_BASE_ADDR); 
		  ii < NUM_BIOS_COM_PORT; ii++, SaveBIOSAddr++)
	{
		/*--------------------*/
		/* Found our address. */
		/*--------------------*/
		if (*SaveBIOSAddr == uartBaseAddr)
		{
			break;
		}
	}

	/*----------------------------------------------*/
	/* We did not find our IO address in the table. */
	/*----------------------------------------------*/
	if (ii == NUM_BIOS_COM_PORT)
	{
		return;
	}

	/*-------------------------------------------------------*/
	/* We found our address in the table.  Save the location	*/
	/* of where we found our address.  We want to restore		*/
	/* the table to it's original contents when the driver	*/
	/* is unloaded.  Clear our address from the table.  We 	*/
	/* don't want anyone but us to write to our Com port.		*/
	/*-------------------------------------------------------*/
	RestoreBIOSTableFlag = SET;
	*SaveBIOSAddr = 0;

	OutChar ('B', BROWN);
	OutChar ('I', BROWN);
	OutChar ('O', BROWN);
	OutChar ('S', BROWN);
	OutChar ('T', BROWN);
	OutChar ('b', BROWN);
	OutChar ('l', BROWN);

	/*-------------------------------------------------------*/
	/* Now we need to decrement the number of serial ports.	*/
	/* This value is stored in the BIOS equipment-list word.	*/
	/*-------------------------------------------------------*/
	orig = *(unsigned_16 __far *)(_MK_FP(0, COM_PORT_CNT_ADDR));

	/*-------------------------------------------------------*/
	/* Extract the 3-bit count from the 16-bit value.  Then	*/
	/* clear the count in the 16-bit value.						*/
	/*-------------------------------------------------------*/
	serialPortCnt = (orig & COM_PORT_CNT_MASK) >> COM_PORT_CNT_BIT;
	orig &= ~COM_PORT_CNT_MASK;

	/*-------------------------------------------------------*/
	/* Make sure the count isn't zero before we decrement it	*/
	/* and then put it back into the 16-bit value.				*/
	/*-------------------------------------------------------*/
	if (serialPortCnt)
		serialPortCnt--;
	orig |= serialPortCnt << COM_PORT_CNT_BIT;

	/*-------------------------------------------------------*/
	/* Save the new equipment list with the updated serial	*/
	/* port count.															*/
	/*-------------------------------------------------------*/
	*(unsigned_16 __far *)_MK_FP(0, COM_PORT_CNT_ADDR) = orig;
}


/***************************************************************************/
/*																					 				*/
/*	RestoreBIOSMem ()															 				*/
/*																					 				*/
/*	INPUT:																			 			*/
/*																						 			*/
/*	OUTPUT:																		 				*/
/*																					 				*/
/*	DESCRIPTION: This function should only be called if we had to remove		*/
/*              our Com port IO address from the BIOS table.  It write		*/
/*              our IO address back into the table and increments the		*/
/*              number of serial ports.												*/
/*																						 			*/
/***************************************************************************/

void RestoreBIOSMem (void)
{
	unsigned_16	orig;
	unsigned_16	serialPortCnt;

	/*-------------------------------------------------------*/
	/* Write the address of our Com port back into the BIOS	*/
	/* table.																*/
	/*-------------------------------------------------------*/
	*SaveBIOSAddr = uartBaseAddr;

	/*-------------------------------------------------------*/
	/* Recalculate the serial port count.  Another driver		*/
	/* may have modified this value.							  		*/
	/*-------------------------------------------------------*/
	orig = *(unsigned_16 __far *)(_MK_FP(0, COM_PORT_CNT_ADDR));
	OutChar ('B', BROWN);
	OutChar ('I', BROWN);
	OutChar ('O', BROWN);
	OutChar ('S', BROWN);
	OutChar ('T', BROWN);
	OutChar ('b', BROWN);
	OutChar ('l', BROWN);
	serialPortCnt = (orig & COM_PORT_CNT_MASK) >> COM_PORT_CNT_BIT;
	orig &= ~COM_PORT_CNT_MASK;

	/*-------------------------------------------------------*/
	/* Increment the serial port count and write it back to	*/
	/* the BIOS equipment-list word.									*/
	/*-------------------------------------------------------*/
	if (serialPortCnt < 7)
		serialPortCnt++;
	orig |= serialPortCnt << COM_PORT_CNT_BIT;

	*(unsigned_16 __far *)(_MK_FP(0, COM_PORT_CNT_ADDR)) = orig;
}

