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
/*	$Header:   J:\pvcs\archive\clld\port.c_v   1.18   14 Oct 1996 10:32:58   BARBARA  $	*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\port.c_v  $											*/
/* 
/*    Rev 1.18   14 Oct 1996 10:32:58   BARBARA
/* R_CtrReg() function added.
/* 
/*    Rev 1.17   27 Sep 1996 09:04:38   JEAN
/* 
/*    Rev 1.16   24 Sep 1996 17:05:18   BARBARA
/* FastIO is set for the parameter not a hardware type (1pc).
/* 
/*    Rev 1.15   15 Aug 1996 09:50:28   JEAN
/* 
/*    Rev 1.14   29 Jul 1996 12:31:42   JEAN
/* 
/*    Rev 1.13   28 Jun 1996 17:32:02   BARBARA
/* One Piece PCMCIA code has been added.
/* 
/*    Rev 1.12   14 Jun 1996 16:19:22   JEAN
/* Added a delay between RFNC register accesses.
/* 
/*    Rev 1.11   16 Apr 1996 11:31:50   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.10   31 Jan 1996 13:09:56   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.9   08 Jan 1996 14:14:54   JEAN
/* Changed a define to all caps.
/* 
/*    Rev 1.8   14 Dec 1995 15:16:58   JEAN
/* Changed a variable declaration to an extern declaration.
/* 
/*    Rev 1.7   17 Nov 1995 16:36:28   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.6   12 Oct 1995 11:41:26   JEAN
/* Corrected some spelling errors.
/* 
/*    Rev 1.5   16 Mar 1995 16:16:18   tracie
/* Added support for ODI
/* 
/*    Rev 1.4   22 Feb 1995 10:36:48   tracie
/* 
/*    Rev 1.3   05 Jan 1995 09:54:52   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.2   22 Sep 1994 10:56:20   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:12   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:28   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/


//#include	<dos.h>
//#include	<conio.h>

#include <KernelExport.h>

//static inline void _outp( uint16 a, uint8 b ) { dprintf("w:%.4hx:%.2hx", a, (uint16)b ); write_io_8(a,b); }
//static inline uint8 _inp( uint16 a ) { uint8 b = read_io_8(a); dprintf("r:%.4hx:%.2hx", a, (uint16)b ); return b; }
static inline uint8 _inpx( uint16 a ) { return read_io_8(a); }
#define _outp(a,b)  write_io_8(a,b)
#define _inp(a) read_io_8(a)
#define _outpw(a,b) write_io_16(a,b)
#define _inpw(a) read_io_16(a)

#include "constant.h"
#include "lld.h"
#include	"port.h"
#include	"asm.h"
#include	"bstruct.h"

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_16	StatusReg;
extern unsigned_8		StatusValue;
extern unsigned_16	ControlReg;
extern unsigned_8		ControlValue;
extern unsigned_16	DataReg;
extern unsigned_16	IntSelReg;
extern unsigned_8		IntSelValue;
extern unsigned_8		IntStack [32];
extern unsigned_8		IntStackPtr;
		
#endif


/***************************************************************************/
/*																									*/
/*	W_StatusReg (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The value to output to the status register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine writes both the Tx and Rx portions of the		*/
/* 	register.  This routine should not be used except in initialization	*/
/* 	routine since the	same status register is used for both Tx and Rx.	*/
/*																									*/
/***************************************************************************/

void W_StatusReg (unsigned_8 Value)

{	_outp (StatusReg, Value);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	SetBit_StatusReg (Value)																*/
/*																									*/
/*	INPUT:	Value	-	The bits to "OR" to the status register.					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will "OR" the value passed with the current	*/
/*		value of the status register.														*/
/*																									*/
/***************************************************************************/

void SetBit_StatusReg (unsigned_8 Value)

{
	StatusValue |= Value;
	_outp (StatusReg, StatusValue);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	ClearBit_StatusReg (Value)																*/
/*																									*/
/*	INPUT:	Value	-	The bits to clear in the status register.					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will invert the value passed and "AND" it		*/
/*		with the current value of the status register.								*/
/*																									*/
/***************************************************************************/

void ClearBit_StatusReg (unsigned_8 Value)

{
	StatusValue &= ~Value;
	_outp (StatusReg, StatusValue);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	W_RxStatusReg (Value)																	*/
/*																									*/
/*	INPUT:	Value	-	The value to output to Rx status register					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine writes the Rx half of the status register		*/
/* 	preserving the	Tx status.															*/
/*																									*/
/***************************************************************************/

void W_RxStatusReg (unsigned_8 Value)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	StatusValue = (unsigned_8)(Value | (StatusValue & TXSTATMASK));
	_outp (StatusReg, StatusValue);
	IO_DELAY

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	W_TxStatusReg (Value)																	*/
/*																									*/
/*	INPUT:	Value	-	The value to output to Tx status register					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine writes the Tx half of the status register		*/
/* 	preserving the	Rx status.															*/
/*																									*/
/***************************************************************************/

void W_TxStatusReg (unsigned_8 Value)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	StatusValue = (unsigned_8)(Value | (StatusValue & RXSTATMASK));
	_outp (StatusReg, StatusValue);
	IO_DELAY

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	W_CtrlReg (Value)																			*/
/*																									*/
/*	INPUT:	Value	-	The value to write to the control register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the value passed to the control		*/
/*		register then update the global control value.								*/
/*																									*/
/***************************************************************************/

void W_CtrlReg (unsigned_8 Value)

{
	_outp (ControlReg, Value);
	ControlValue = Value;
	IO_DELAY
}

/***************************************************************************/
/*																									*/
/*	R_CtrlReg (void)																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Read value																		*/
/*																									*/
/*	DESCRIPTION: This routine will read the value from the control register	*/
/*																									*/
/***************************************************************************/

unsigned_8 R_CtrlReg (void)

{
	unsigned_8 status;

	status = (unsigned_8)_inp (ControlReg);
 	return (status);
}



/***************************************************************************/
/*																									*/
/*	SetBit_CtrlReg (Value)																	*/
/*																									*/
/*	INPUT:	Value	-	The bits to "OR" to the control register.					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will "OR" the value passed with the current	*/
/*		value of the control register.													*/
/*																									*/
/***************************************************************************/

void SetBit_CtrlReg (unsigned_8 Value)

{
	ControlValue |= Value;
	_outp (ControlReg, ControlValue);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	ClearBit_CtrlReg (Value)																*/
/*																									*/
/*	INPUT:	Value	-	The bits to clear in the control register.				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will invert the value passed and "AND" it		*/
/*		with the current value of the control register.								*/
/*																									*/
/***************************************************************************/

void ClearBit_CtrlReg (unsigned_8 Value)

{
	ControlValue &= ~Value;
	_outp (ControlReg, ControlValue);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	W_DataRegB (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The byte value to write to the data register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the data register one byte at		*/
/*		a time for 8 bit mode																*/
/*																									*/
/***************************************************************************/

void W_DataRegB (unsigned_8 Value)

{
	_outp (DataReg, Value);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	W_DataRegW (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The word value to write to the data register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the data register one word			*/
/* 	at a time for 16 bit mode															*/
/*																									*/
/***************************************************************************/

void W_DataRegW (unsigned_16 Value)

{
	_outpw (DataReg, Value);
	IO_DELAY
}


/***************************************************************************/
/*																									*/
/*	W_FastDataRegB (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The byte value to write to the data register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the data register one byte at		*/
/*		a time for 8 bit mode																*/
/*																									*/
/***************************************************************************/

void W_FastDataRegB (unsigned_8 Value)

{
	_outp (DataReg, Value);
}



/***************************************************************************/
/*																									*/
/*	W_FastDataRegW (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The word value to write to the data register				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the data register one word			*/
/* 	at a time for 16 bit mode															*/
/*																									*/
/***************************************************************************/

void W_FastDataRegW (unsigned_16 Value)

{
	_outpw (DataReg, Value);
}


/***************************************************************************/
/*																									*/
/*	R_Status ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The byte value read from the status port					*/
/*																									*/
/*	DESCRIPTION: This routine will read the status register and return		*/
/*		the value																				*/
/*																									*/
/***************************************************************************/

unsigned_8 R_Status (void)

{
	unsigned_8 status;

	status = (unsigned_8)_inp (StatusReg);
	IO_DELAY
	return (status);
}



/***************************************************************************/
/*																									*/
/*	R_RxStatus ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The Rx status read from the port								*/
/*																									*/
/*	DESCRIPTION: This routine will read the status register, mask out			*/
/* 	the Tx status and	return only the Rx status.									*/
/*																									*/
/***************************************************************************/

unsigned_8 R_RxStatus (void)

{
	unsigned_8 status;

	status = (unsigned_8)(_inp (StatusReg) & RXSTATMASK);
	IO_DELAY
 	return (status);
}



/***************************************************************************/
/*																									*/
/*	R_TxStatus ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The Tx status read from the port								*/
/*																									*/
/*	DESCRIPTION: This routine will read the status register, mask out			*/
/* 	the Rx status and return only the Tx status.									*/
/*																									*/
/***************************************************************************/

unsigned_8 R_TxStatus (void)

{
	unsigned_8 status;

	status = (unsigned_8)(_inp (StatusReg) & TXSTATMASK);
	IO_DELAY
	return (status);
}



/***************************************************************************/
/*																									*/
/*	R_DataRegB ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The byte value read from the port							*/
/*																									*/
/*	DESCRIPTION: This routine will read a byte from the data register			*/
/*																									*/
/***************************************************************************/

unsigned_8 R_DataRegB (void)

{
	unsigned_8 status;

	status = (unsigned_8)(_inp (DataReg));
	IO_DELAY
	return (status);
}



/***************************************************************************/
/*																									*/
/*	R_DataRegW ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The word value read from the port							*/
/*																									*/
/*	DESCRIPTION: This routine will read a word from the data register			*/
/*																									*/
/***************************************************************************/

unsigned_16 R_DataRegW (void)

{
	unsigned_16 data;

	data = _inpw (DataReg);
	IO_DELAY
	return (data);
}

/***************************************************************************/
/*																									*/
/*	R_FastDataRegB ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The byte value read from the port							*/
/*																									*/
/*	DESCRIPTION: This routine will read a byte from the data register			*/
/*																									*/
/***************************************************************************/

unsigned_8 R_FastDataRegB (void)

{
	unsigned_8 status;

	status = (unsigned_8)(_inp (DataReg));
	return (status);
}



/***************************************************************************/
/*																									*/
/*	R_FastDataRegW ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Value	-	The word value read from the port							*/
/*																									*/
/*	DESCRIPTION: This routine will read a word from the data register			*/
/*																									*/
/***************************************************************************/

unsigned_16 R_FastDataRegW (void)

{
	unsigned_16 data;

	data = _inpw (DataReg);
	return (data);
}



/***************************************************************************/
/*																									*/
/*	IntRFNCTx ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine generates an interrupt to the RFNC as			*/
/* 	related to the PC	sending data to the RFNC.									*/
/*																									*/
/***************************************************************************/

void IntRFNCTx (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	_outp (ControlReg, ControlValue & (~INT_RFNCBITRX));
	IO_DELAY
	_outp (ControlReg, ControlValue | INT_RFNCBITRX);
	IO_DELAY
	ControlValue |= INT_RFNCBITRX;

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	IntRFNCRx ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine generates an interrupt to the RFNC as			*/
/* 	related to the PC	receiving data from the RFNC.								*/
/*																									*/
/***************************************************************************/

void IntRFNCRx (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	_outp (ControlReg, ControlValue & (~INT_RFNCBITTX));
	IO_DELAY
	_outp (ControlReg, ControlValue | INT_RFNCBITTX);
	IO_DELAY
	ControlValue = (unsigned_8)(ControlValue | INT_RFNCBITTX);

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	PushRFNCInt ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine preserves the current state of the RFNC			*/
/*		interrupt by pushing it onto our "stack".										*/
/*																									*/
/***************************************************************************/

void PushRFNCInt (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	IntStack [IntStackPtr++] = IntSelValue;

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	PopRFNCInt ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine restores the state of the RFNC interrupt		*/
/*		by popping it from our "stack".													*/
/*																									*/
/***************************************************************************/

void PopRFNCInt (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	IntSelValue = IntStack [--IntStackPtr];
	_outp (IntSelReg, IntSelValue);
	IO_DELAY

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
/*	DESCRIPTION: This routine disables the board's interrupt.  This			*/
/* 	will disable the board from interrupting the PC.							*/
/*																									*/
/***************************************************************************/

void DisableRFNCInt (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	IntSelValue &= ~INT_ENABLEBIT;
	_outp (IntSelReg, IntSelValue);
	IO_DELAY

	OutChar ('D', GREEN + BLUE_BACK);
	
	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	EnableRFNCInt ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine enables the board's interrupt.  This				*/
/* will allow the board to interrupt the PC.											*/
/*																									*/
/***************************************************************************/

void EnableRFNCInt (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	IntSelValue |= INT_ENABLEBIT;
	_outp (IntSelReg, IntSelValue);
	IO_DELAY

	RestoreFlag (oldflag);
}



/***************************************************************************/
/*																									*/
/*	W_IntSelReg (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The interrupt number to write to the int select reg.	*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the interrupt number to the			*/
/* 	interrupt select register on the board to set up the board to			*/
/* 	use that interrupt.																	*/
/*																									*/
/***************************************************************************/

void	W_IntSelReg (unsigned_8 Value)

{
	IntSelValue |= Value;
	_outp (IntSelReg, IntSelValue);
	IO_DELAY
}



/***************************************************************************/
/*																									*/
/*	EOI (irq)																					*/
/*																									*/
/*	INPUT:	irq -	The interrupt that the card is using (0 - 15).				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will give the EOI command to the					*/
/* 	interrupt controller.  If we are clearing an interrupt on the  		*/
/*    second controller, we need to write to both PICs to clear the        */
/*    interrupt.                                                           */
/*																									*/
/***************************************************************************/

void EOI (unsigned_8 irq)

{
	/*
	if (irq > 7)
	{
		_outp (I8259CMD2, I8259EOI);
		IO_DELAY
	}
	_outp (I8259CMD, I8259EOI);
	IO_DELAY
	*/
}



/***************************************************************************/
/*																									*/
/*	LLDOutOfStandby ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will bring the card out of standby by			*/
/* 	setting the wakeup bit, hold it high for approx 2ms then clearing it.*/
/*																									*/
/***************************************************************************/

void LLDOutOfStandby (void)

{	unsigned_8	Value;
	
	Value = (unsigned_8)(ControlValue | WAKEUPBIT);
	W_CtrlReg (Value);
	Delay (3);
	W_CtrlReg (ControlValue);
}

