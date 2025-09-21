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
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\ntport.c_v  $											*/
/* 
/*    Rev 1.12   03 Jun 1998 14:39:10   BARBARA
/* ifdef NDIS_MINIPORT_DRIVER added.
/* 
/*    Rev 1.11   05 Mar 1997 14:38:46   BARBARA
/* Delay in W_DataRegW commented out.
/* 
/*    Rev 1.10   05 Mar 1997 11:11:40   BARBARA
/* Delay was added in W_DataRegW.
/* 
/*    Rev 1.9   14 Oct 1996 10:33:24   BARBARA
/* R_CtrReg() function added.
/* 
/*    Rev 1.8   30 Sep 1996 16:30:12   BARBARA
/* in 0x61 read ushort which was changed to uchar.
/* 
/*    Rev 1.7   24 Sep 1996 16:48:04   BARBARA
/* FastIO for 1pc was substituted by the FastIO for the FastIO parameter.
/* 
/*    Rev 1.6   15 Aug 1996 09:50:40   JEAN
/* 
/*    Rev 1.5   29 Jul 1996 12:35:46   JEAN
/* 
/*    Rev 1.4   28 Jun 1996 18:12:14   BARBARA
/* One piece PCMCIA code has been added for NDIS3.
/* 
/*    Rev 1.3   14 Jun 1996 16:49:50   JEAN
/* Removed ^Z at the end of the file.
/* 
/*    Rev 1.2   12 May 1996 17:37:00   TERRYL
/* Added debug information for EnableRFNCInt()
/* 
/*    Rev 1.1   16 Apr 1996 18:14:20   TERRYL
/* Added NdisRawReadPortUshort() to delay the execution for some of
/* the write routines. This fix a bug in some of pentium machine.
/* Writing to the port successively too quickly may cause the driver
/* to reset.
/* 
/*    Rev 1.0   04 Mar 1996 11:07:12   truong
/* Initial revision.
/* 
/*    Rev 1.2   20 Oct 1995 18:39:12   BARBARA
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


#include	<dos.h>
#include	<conio.h>
#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#include "hw.h"
#endif
#include "constant.h"
#include "lld.h"
#include	"port.h"
#include	"asm.h"
#include	"bstruct.h"
#ifdef NDIS_MINIPORT_DRIVER
#include "sw.h"
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_32	MapStatusReg;
extern unsigned_8		StatusValue;
extern unsigned_32	MapControlReg;
extern unsigned_8		ControlValue;
extern unsigned_32	MapDataReg;
extern unsigned_32	MapEOIReg;
extern unsigned_32	MapIntSelReg;
extern unsigned_8		IntSelValue;
extern unsigned_8		LLDOnePieceFlag;
extern unsigned_8		LLDPCMCIA;

unsigned_8	IntStack [32];
unsigned_8	IntStackPtr;
		
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

{	NdisRawWritePortUchar (MapStatusReg, Value);

	/* Added the next line for delay */
	NdisRawReadPortUchar(0x61, &Value);
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
	NdisRawWritePortUchar (MapStatusReg, StatusValue);
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
	NdisRawWritePortUchar (MapStatusReg, StatusValue);
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
/* 	preserving the	tx status.															*/
/*																									*/
/***************************************************************************/

void W_RxStatusReg (unsigned_8 Value)

{
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/
	StatusValue = (unsigned_8)(Value | (StatusValue & TXSTATMASK));
	NdisRawWritePortUchar (MapStatusReg, StatusValue);

//	RestoreFlag (oldflag);
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
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/

	StatusValue = (unsigned_8)(Value | (StatusValue & RXSTATMASK));
	NdisRawWritePortUchar (MapStatusReg, StatusValue);
/*
	RestoreFlag (oldflag);
*/
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
	NdisRawWritePortUchar (MapControlReg, Value);
	ControlValue = Value;
}

/***************************************************************************/
/*																									*/
/*	R_CtrlReg ()																				*/
/*																									*/
/*	INPUT:	None																				*/
/*																									*/
/*	OUTPUT:	Value read in																	*/
/*																									*/
/*	DESCRIPTION: This routine will read the value from control register		*/
/*																									*/
/***************************************************************************/

unsigned_8 R_CtrlReg (void)
{ 	unsigned_8	PortInput;

	NdisRawReadPortUchar (MapControlReg, &PortInput);
	return (PortInput);
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
	NdisRawWritePortUchar (MapControlReg, ControlValue);
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
	NdisRawWritePortUchar (MapControlReg, ControlValue);
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
	NdisRawWritePortUchar (MapDataReg, Value);

	/* Added the next line for delay */
	NdisRawReadPortUchar(0x61, &Value);
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
	unsigned_8	TmpValue;

	NdisRawWritePortUshort (MapDataReg, Value);

	/* Added the next line for delay */
	NdisRawReadPortUchar(0x61, &TmpValue);
/*
	if (LLDPCMCIA && (!LLDOnePieceFlag))
	{
		NdisRawReadPortUchar(0x61, &TmpValue);
		NdisRawReadPortUchar(0x61, &TmpValue);
		NdisRawReadPortUchar(0x61, &TmpValue);
	}
*/
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
	NdisRawWritePortUchar (MapDataReg, Value);
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
	NdisRawWritePortUshort (MapDataReg, Value);
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

{	unsigned_8	PortInput;

	NdisRawReadPortUchar (MapStatusReg, &PortInput);
	return (PortInput);
}


/***************************************************************************/
/*																									*/
/*	R_EOIReg ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																	  					*/
/*																									*/
/*	DESCRIPTION: This routine will read the EOI register and return			*/
/*		the value																				*/
/*																									*/
/***************************************************************************/

void R_EOIReg (void)

{	unsigned_8	PortInput;

	NdisRawReadPortUchar (MapEOIReg, &PortInput);
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

{ 	unsigned_8	PortInput;

	NdisRawReadPortUchar (MapStatusReg, &PortInput);
	return (PortInput & RXSTATMASK);
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

{	unsigned_8 PortInput;

	NdisRawReadPortUchar (MapStatusReg, &PortInput);
	return (PortInput & TXSTATMASK);
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

{	unsigned_8	PortInput;
	unsigned_8	Value;

	NdisRawReadPortUchar (MapDataReg, &PortInput);
	NdisRawReadPortUchar(0x61, &Value);
	return (PortInput);
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

{	unsigned_16 PortInput;
	unsigned_8	Value;

	NdisRawReadPortUshort (MapDataReg, &PortInput);
	NdisRawReadPortUchar(0x61, &Value);
	return (PortInput); 
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

{	unsigned_8	PortInput;

	NdisRawReadPortUchar (MapDataReg, &PortInput);
	return (PortInput);
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

{	unsigned_16 PortInput;

	NdisRawReadPortUshort (MapDataReg, &PortInput);
	return (PortInput); 
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
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/

	NdisRawWritePortUchar (MapControlReg, ControlValue & (~INT_RFNCBITRX));
	NdisRawWritePortUchar (MapControlReg, ControlValue | INT_RFNCBITRX);
	ControlValue |= INT_RFNCBITRX;

//	RestoreFlag (oldflag);
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
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/

	NdisRawWritePortUchar (MapControlReg, ControlValue & (~INT_RFNCBITTX));
	NdisRawWritePortUchar (MapControlReg, ControlValue | INT_RFNCBITTX);
	ControlValue = (unsigned_8)(ControlValue | INT_RFNCBITTX);

//	RestoreFlag (oldflag);
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
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/

	DisableSystemInterrupt ();

	IntStack [IntStackPtr++] = IntSelValue;

	EnableSystemInterrupt ();

//	RestoreFlag (oldflag);
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
/*
	unsigned_16	oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();
*/

	DisableSystemInterrupt ();

	IntSelValue = IntStack [--IntStackPtr];

	EnableSystemInterrupt ();

	NdisRawWritePortUchar (MapIntSelReg, IntSelValue);


//	RestoreFlag (oldflag);
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
	IntSelValue &= ~INT_ENABLEBIT;
	NdisRawWritePortUchar (MapIntSelReg, IntSelValue);

	OutChar ('D', GREEN + BLUE_BACK);
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
	IntSelValue |= INT_ENABLEBIT;
	NdisRawWritePortUchar (MapIntSelReg, IntSelValue);

	OutChar ('E', GREEN + BLUE_BACK);

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
	NdisRawWritePortUchar (MapIntSelReg, IntSelValue);
}



/***************************************************************************/
/*																									*/
/*	EOI (irq)																					*/
/*																									*/
/*	INPUT:	irq -	The interrupt that the card is using							*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will give the EOI command to the					*/
/* 	interrupt controller.																*/
/*																									*/
/***************************************************************************/

void EOI (unsigned_8 irq)

{
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
	Delay (200);
	W_CtrlReg (ControlValue);
}
