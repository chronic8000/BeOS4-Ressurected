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
/*	$Header:   J:\pvcs\archive\clld\pcmcia.c_v   1.30   11 Oct 1996 11:11:28   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\pcmcia.c_v  $ 										*/
/* 
/*    Rev 1.30   11 Oct 1996 11:11:28   BARBARA
/* oldflag variable moved arround to keep the compiler quiet.
/* 
/*    Rev 1.29   02 Oct 1996 13:41:00   JEAN
/* In ConfigureSocket(), changed the ordering so that we now
/* power the 365 off before we check for proper card seating.
/* Also, the value we write to power off the card is 0 instead of 4.
/* 
/*    Rev 1.28   27 Sep 1996 09:05:42   JEAN
/* 
/*    Rev 1.27   28 Aug 1996 15:44:12   BARBARA
/* The number of requested IO ports for the PCMCIA card was cheged back to 8.
/* 
/*    Rev 1.26   15 Aug 1996 11:05:28   JEAN
/* Added a function, W_DummyReg(), to write to CIS location 0.
/* 
/*    Rev 1.25   29 Jul 1996 12:33:48   JEAN
/* Increased IOAddr0Stop to increase the memory window size.
/* 
/*    Rev 1.24   15 Jul 1996 14:37:28   TERRYL
/* Type cast the time difference (for LLSGetCurrentTime ) to 16-bit.
/* 
/*    Rev 1.23   28 Jun 1996 17:32:50   BARBARA
/* One piece PCMCIA code for NDIS3 has been added.
/* 
/*    Rev 1.22   14 Jun 1996 16:21:36   JEAN
/* Made changes to support a one piece PCMCIA card.
/* 
/*    Rev 1.21   16 Apr 1996 11:33:00   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.20   29 Mar 1996 11:43:38   JEAN
/* Moved some variable declarations to lld.c
/* 
/*    Rev 1.19   22 Mar 1996 14:50:06   JEAN
/* Added support for one-piece PCMCIA.
/* 
/*    Rev 1.18   08 Mar 1996 19:07:20   JEAN
/* Added compile time support for a one-pieced PCMCIA card.
/* 
/*    Rev 1.17   04 Mar 1996 15:06:32   truong
/* Conditional compile for WinNT/95 driver.
/* 
/*    Rev 1.16   06 Feb 1996 14:26:10   JEAN
/* Comment changes.
/* 
/*    Rev 1.15   31 Jan 1996 13:12:24   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.14   08 Jan 1996 14:15:20   JEAN
/* Increased the delay after power-up to 300 milliseconds.  This
/* is the PCMCIA spec.
/* 
/*    Rev 1.13   19 Dec 1995 18:09:04   JEAN
/* Added a header include file.
/* 
/*    Rev 1.12   14 Dec 1995 15:19:00   JEAN
/* Added a header include file.
/* 
/*    Rev 1.11   07 Dec 1995 12:11:22   JEAN
/* Changed the include directive for a C library function from
/* quotes to angle brackets.
/* 
/*    Rev 1.10   17 Nov 1995 16:37:36   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   16 Oct 1995 16:05:56   JEAN
/* Removed the redundant if (LLDPCMCIA) test.
/* 
/*    Rev 1.8   12 Oct 1995 13:55:38   JEAN
/* Added Header keyword.
/* 
/*    Rev 1.7   24 Jul 1995 18:37:28   truong
/* 
/*    Rev 1.6   16 Mar 1995 16:16:52   tracie
/* Added support for ODI
/* 
/*    Rev 1.5   22 Feb 1995 10:37:18   tracie
/* 
/*    Rev 1.4   05 Jan 1995 11:07:58   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.3   29 Nov 1994 12:45:30   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:22   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:18   hilton
/* 
/*    Rev 1.0   20 Sep 1994 11:00:32   hilton
/* Initial revision.
/* 																								*/
/*																									*/
/***************************************************************************/


//#include <conio.h>

#include <KernelExport.h>

#define outp(a,b) write_io_8(a,b)
#define inp(a) read_io_8(a)
#define _inp(a) read_io_8(a)
static inline uint8 _inpx( uint16 a ) { return read_io_8(a); }

#include "constant.h"
#include "lld.h"
#include "pcmcia.h"
#include "asm.h"
#include "blld.h"
#include "lldctrl.h"
#include	"bstruct.h"
#include "port.h"

#ifdef NDIS_MINIPORT_DRIVER

#include <ndis.h>
#include "hw.h"
#include "sw.h"

#endif


unsigned_8		SocketTbl [4] = { 0x00, 0x40, 0x80, 0xC0 };
unsigned_8		IntTbl [16] = {0,0,0,1,1,1,0,1,0,0,1,1,1,0,0,1};


#ifdef MULTI

#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#endif

#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_16	CCReg;
extern unsigned_32	CCRegPtr;
extern unsigned_16	SocketOffset;
extern unsigned_8		LLDFUJITSUFlag;
extern unsigned_8		LLDOnePieceFlag;

extern unsigned_16	ControlReg;
extern unsigned_8		PCMCIACardInserted;
extern unsigned_8		LLDSyncState;

extern unsigned_8		LLDSocketNumber;
extern unsigned_8		LLDIntLine1;
extern unsigned_16	LLDIOAddress1;
extern unsigned_16	LLDMemoryAddress1;
extern unsigned_8		LLDInit365Flag;
extern unsigned_8		FirstTimeIns;
#endif





/***************************************************************************/
/*																									*/
/*	ConfigureMachine ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns	 -		0 = Success, 1 = Failure								*/
/*																									*/
/*	DESCRIPTION: This routine will configure the socket and check for			*/
/*		Fujitsu machine.																		*/
/*																									*/
/***************************************************************************/

unsigned_16 ConfigureMachine (void)

{	unsigned_16	Status;

	Status = ConfigureSocket ();
	if (Status)
		return (Status);

	SetPCMCIAForInterruptType ();
	return (0);
}




/***************************************************************************/
/*																									*/
/*	ConfigureSocket ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns	 -		0 = Success													*/
/*                         1 = Improper Card Seating								*/
/*                         2 = Power Improperly Supplied							*/
/*                         3 = No PCMCIA Bus Ready Signal						*/
/*																									*/
/*	DESCRIPTION: This routine will configure the PCMCIA socket.					*/
/*																									*/
/***************************************************************************/

unsigned_16 ConfigureSocket (void)

{	unsigned_16	Offset;
	unsigned_16	CurrTime;
	unsigned_8	value;
	FLAGS			oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	SocketOffset = SocketTbl [LLDSocketNumber];

	/*----------------------------------*/
	/* Turn off power to the 365 chip.	*/
	/*----------------------------------*/
	WritePCICReg (PwrCtlReg, 0);
	Delay (300);

	/*-------------------------------------------------*/
	/* Check for proper card seating.						*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((ReadPCICReg (IFStatusReg) & 0x0C) != 0x0C)
	{	RestoreFlag (oldflag);
		return (1);
	}


	/*-------------------------------------------------*/	
	/* Enable power and output to the socket.				*/
	/*	Check if power properly supplied to the socket.	*/
	/*																	*/
	/*-------------------------------------------------*/

	WritePCICReg (PwrCtlReg, 0x15);
	Delay (300);
	WritePCICReg (PwrCtlReg, 0xD5);

	if ((ReadPCICReg (IFStatusReg) & 0x40) == 0)
	{	RestoreFlag (oldflag);
		return (2);
	}


	/*-------------------------------------------------*/
	/* Configure the interrupt level and bring the		*/
	/* card out of reset.										*/
	/*																	*/
	/*-------------------------------------------------*/

	WritePCICReg (IOCtlReg, 0xBB);
	WritePCICReg (INTCtlReg, (unsigned_8)(LLDIntLine1 | 0x60));


	/*-------------------------------------------------*/
	/* Set up I/O and memory Window.							*/
	/*																	*/
	/*-------------------------------------------------*/

	WritePCICReg (IOAddr0Start, (unsigned_8)(LLDIOAddress1 & 0x00FF));
	WritePCICReg (IOAddr0Start + 1, (unsigned_8)(LLDIOAddress1 >> 8));
	WritePCICReg (IOAddr0Stop, (unsigned_8)((LLDIOAddress1 + 7) & 0x00FF));
	WritePCICReg (IOAddr0Stop + 1, (unsigned_8)((LLDIOAddress1 + 7) >> 8));

	WritePCICReg (Win0MemStart, (unsigned_8)(LLDMemoryAddress1 >> 8));
	WritePCICReg (Win0MemStart + 1, 0);
	WritePCICReg (Win0MemStop, (unsigned_8)(LLDMemoryAddress1 >> 8));
	WritePCICReg (Win0MemStop + 1, 0);

	Offset = (unsigned_16)(((0 - (LLDMemoryAddress1 >> 8)) & 0x3FFF) | 0x4000);
	WritePCICReg (Win0Offset, (unsigned_8)(Offset & 0x00FF));
	WritePCICReg (Win0Offset + 1, (unsigned_8)(Offset >> 8));


	/*-------------------------------------------------------------*/
	/* Wait for the ready signal on the PCMCIA bus to be asserted. */
	/* Enable interrupts so we can timeout.								*/
	/*-------------------------------------------------------------*/
	EnableInterrupt ();
	CurrTime = LLSGetCurrentTime ();
	while ((unsigned_16)(LLSGetCurrentTime() - CurrTime) <= MAXWAITTIME)	
	{
		value = ReadPCICReg (IFStatusReg);
		if (value & 0x20)
			break;
	}
	if (!(value & 0x20))
	{
		OutChar ('N', RED);
		OutChar ('o', RED);
		OutChar ('t', RED);
		OutChar ('R', RED);
		OutChar ('e', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('y', RED);
		OutHex (value, RED);
		RestoreFlag (oldflag);
		return (3);
	}

	DisableInterrupt ();

	/*-------------------------------------------------*/
	/* Enable memory and I/O windows.						*/
	/*																	*/
	/*-------------------------------------------------*/

	WritePCICReg (WinEnableReg, 0x41);

	RestoreFlag (oldflag);
	return (0);
}




/***************************************************************************/
/*																									*/
/*	SetPCMCIAForInterruptType ()															*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will set up the config register pointer.		*/
/*																									*/
/***************************************************************************/

void SetPCMCIAForInterruptType (void)

{

#ifdef NDIS_MINIPORT_DRIVER

	NdisWriteRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+CCReg,0x41);

#else

	CCRegPtr = (((unsigned_32)(LLDMemoryAddress1) << 16)) + CCReg;

	if (LLDFUJITSUFlag)
	{//	__asm		int	80H
	}

	*(unsigned_8 *)CCRegPtr = 0x41;

	if (LLDFUJITSUFlag)
	{//	__asm		int	81H
	}

#endif

}




/***************************************************************************/
/*																									*/
/* ClearPCMCIAInt ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will clear the PCMCIA interrupt by writing		*/
/*		the address pointed to by CCReg.													*/
/*																									*/
/***************************************************************************/

void ClearPCMCIAInt (void)

{	unsigned_8	Temp;

#ifdef NDIS_MINIPORT_DRIVER

	/*----------------------------------------------*/
	/* Read register and write back to clear			*/
	/* PCMCIA interrupt.										*/
	/*																*/
	/*----------------------------------------------*/
	if (LLDOnePieceFlag)
	{
		R_EOIReg();
	}
	else
	{
		NdisReadRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+CCReg, &Temp);
		NdisWriteRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+CCReg,Temp);
	}


#else
	FLAGS	 		oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	if (LLDFUJITSUFlag)
	{//	__asm		int	80H
	}

	/*----------------------------------------------*/
	/* Read register and write back to clear			*/
	/* PCMCIA interrupt.										*/
	/*																*/
	/*----------------------------------------------*/

	if (LLDOnePieceFlag)
	{
		_inp (ControlReg+1);
		IO_DELAY
	}
	else
	{
		Temp = *(unsigned_8 *)CCRegPtr;
		*(unsigned_8 *)CCRegPtr = Temp;
	}


	if (LLDFUJITSUFlag)
	{//	__asm		int	81H
	}

	RestoreFlag (oldflag);

#endif

}		




/***************************************************************************/
/*																									*/
/*	TurnPowerOff ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will turn off the power to the PCMCIA slot.	*/
/*																									*/
/***************************************************************************/

void TurnPowerOff (void)

{
	FLAGS	oldflag;

	oldflag = PreserveFlag ();
   DisableInterrupt ();

	WritePCICReg (PwrCtlReg, 0);
	Delay (7);

	RestoreFlag (oldflag);
}





/***************************************************************************/
/*																									*/
/*	InsertedPCMCIACard (IRQNum, WinBase, IOAddr, Socket)							*/
/*																									*/
/*	INPUT:	IRQNum		-	IRQ number from card and socket services.			*/
/*				WinBase		-	Linear Address of Memory resource allocated.		*/
/*				IOAddr		-	I/O Port address resource allocated.				*/
/*				Socket		-	Socket number of the card.								*/
/*																									*/
/*	OUTPUT:	Returns		-	0 = Configuration successful.							*/
/*									Non-zero if error.										*/
/*																									*/
/*	DESCRIPTION: This routine will initialize the PCMCIA card.					*/
/*																									*/
/***************************************************************************/

unsigned_16 InsertedPCMCIACard (unsigned_8 IRQNum, unsigned_32 WinBase,
										  unsigned_16 IOAddr,	unsigned_16 Socket)

{	
	PCMCIACardInserted = SET;

	if (FirstTimeIns == 0)
	{	return (LLDReset ());
	}

	LLDIntLine1 = IRQNum;
	LLDMemoryAddress1 = (unsigned_16)(WinBase >> 4);
	LLDIOAddress1 = IOAddr;
	LLDSocketNumber = (unsigned_8)(Socket + 9);

	FirstTimeIns = 0;

	return (0);
}





/***************************************************************************/
/*																									*/
/*	RemovedPCMCIACard ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns		-	Always returns success.									*/
/*																									*/
/*	DESCRIPTION: This routine will reset some LLD variables.						*/
/*																									*/
/***************************************************************************/

unsigned_16 RemovedPCMCIACard (void)

{
	PCMCIACardInserted = CLEAR;
	LLDResetQ ();
	LLDSyncState = CLEAR;

	return (0);
}




/***************************************************************************/
/*																									*/
/*	ParameterRangeCheck ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns	-	0 = Check O.K.													*/
/*								1 = Address Error												*/
/*								2 = Int line error.											*/
/*																									*/
/*	DESCRIPTION: This routine will check LLDMemoryAddress and IntLine1		*/
/*		to make sure they are valid.														*/
/*																									*/
/***************************************************************************/

unsigned_16 ParameterRangeCheck (void)

{
	if (((LLDMemoryAddress1 < 0xA000) || (LLDMemoryAddress1 > 0xFF00)) || (LLDMemoryAddress1 & 0x00FF))
		return (1);

	if (LLDIntLine1 > 15)
	{	return (2);
	}
	else
	{	if (IntTbl [LLDIntLine1] == 0)
		{	return (2);
		}
	}

	return (0);
}	




/***************************************************************************/
/*																									*/
/*	WritePCICReg (Index, Value)															*/
/*																									*/
/*	INPUT:	Index		-	Index to write.												*/
/*				Value		-	Value to write.												*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write the given value to the specified	*/
/*		indexed register.																		*/
/*																									*/
/***************************************************************************/

void WritePCICReg (unsigned_8 Index, unsigned_8 Value)

{	outp (PCICIndexReg, Index + SocketOffset);
	outp (PCICDataReg, Value);
}




/***************************************************************************/
/*																									*/
/*	ReadPCICReg (Index)																		*/
/*																									*/
/*	INPUT:	Index		-	Index to write.												*/
/*																									*/
/*	OUTPUT:	Value read from the given register.										*/
/*																									*/
/*	DESCRIPTION: This routine will read the given indexed register.			*/
/*																									*/
/***************************************************************************/

unsigned_8 ReadPCICReg (unsigned_8 Index)

{	unsigned_8	Value;

	outp (PCICIndexReg, Index + SocketOffset);
	Value = (unsigned_8)inp (PCICDataReg);
	return (Value);
}

/***************************************************************************/
/*																									*/
/*	CheckForOnePiece ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:		 0  = SUCCESS																*/
/*              FF = FAILURE																*/
/*																									*/
/*	DESCRIPTION: This routine determines if the PCMCIA card is a one pieced	*/
/*              PCMCIA card or a two pieced PCMCIA card.  We don't need		*/
/*              to do anything for a two pieced card, but if it is a one	*/
/*              pieced card, we need to initialize the CCReg to a				*/
/*              address and set the LLDOnePiece flag.								*/
/*																									*/
/***************************************************************************/

int CheckForOnePiece (void)

{	
#ifdef NDIS_MINIPORT_DRIVER	
	unsigned_16	TupleOffset;
	unsigned_16	TupleEnd;
	unsigned_8	Temp;
#else
	unsigned_8	FAR *TuplePtr;	
	unsigned_8	FAR *EndTuple;
#endif
	unsigned_16	TupleLen;

	/*----------------------------------*/
	/* If the flag is already set, just	*/
	/* initialize the CCReg address.		*/
	/* This flag gets set from RL2DIAG	*/
	/* with the /PCM1 command line		*/
	/* parameter.								*/
	/*----------------------------------*/
	if (LLDOnePieceFlag)
	{
		CCReg = CCREG_1PIECE;
		OutChar ('1', BROWN);
		OutChar ('P', BROWN);
		OutChar ('c', BROWN);
		return(SUCCESS);
	}

	/*-------------------------------------------*/
	/* Clear the register, if we successfully		*/
	/* read from the memory window, we'll find	*/
	/* the CCReg address in one of the Tupples	*/
	/*-------------------------------------------*/
	CCReg = 0;

#ifdef NDIS_MINIPORT_DRIVER

	TupleOffset = 0;
	TupleEnd = 0x0000AFFF;

	while (TupleOffset != TupleEnd)
	{

		NdisReadRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+TupleOffset, &Temp);

		/* Did we find the CC Reg Entry?	*/
		if (Temp == 0x1A)
		{
			TupleOffset += 0x0A;
			NdisReadRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+TupleOffset, &Temp);
			CCReg  = Temp << 8;
			TupleOffset -= 2;
			NdisReadRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+TupleOffset, &Temp);
			CCReg |=	Temp;
			break;
		}
		/* Are we at the end of the Tuple list?	*/
		else if (Temp == 0xFF)
		{
			break;
		}
		/* Get the next Tuple */
		else
		{
			NdisReadRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr+TupleOffset+2, &Temp);
			TupleLen = (unsigned_16)Temp;
			TupleLen *= 2;
			TupleLen += 4;
			TupleOffset = TupleLen + TupleOffset;
		}
	}

#else

	TuplePtr = (unsigned_8 FAR *)((unsigned_32)LLDMemoryAddress1 << 16);
	EndTuple = (unsigned_8 FAR *)((unsigned_32)TuplePtr + 0x0000AFFF);

	while (TuplePtr != EndTuple)
	{
		/* Did we find the CC Reg Entry?	*/
		if (*TuplePtr == 0x1A)
		{
			TuplePtr += 0x0A;
			CCReg  = *TuplePtr << 8;
			TuplePtr -= 2;
			CCReg |=	*TuplePtr;
			break;
		}
		/* Are we at the end of the Tuple list?	*/
		else if (*TuplePtr == 0xFF)
		{
			break;
		}
		/* Get the next Tuple */
		else
		{
			TupleLen = (unsigned_16) (*(TuplePtr + 2));
			TupleLen *= 2;
			TupleLen  += 4;
			TuplePtr = (unsigned_8 FAR *)((unsigned_32)TupleLen + (unsigned_32)TuplePtr);
		}
	}	

#endif

	/*-------------------------------------------*/
	/* If the CC register is 0x100, then it is a	*/
	/* one piece PCMCIA card.  For a two piece	*/
	/* PCMCIA register, it is 0x800.					*/ 
	/*-------------------------------------------*/

	if ((unsigned_16)CCReg == CCREG_1PIECE)
	{
	 	LLDOnePieceFlag = 1;
		OutChar ('1', BROWN);
	}
	else if ((unsigned_16)CCReg == CCREG)
	{
		OutChar ('2', BROWN);
	}
	else
	{
		OutChar ('M', RED);
		OutChar ('e', RED);
		OutChar ('m', RED);
		OutChar ('W', RED);
		OutChar ('i', RED);
		OutChar ('n', RED);
		OutChar ('F', RED);
		OutChar ('a', RED);
		OutChar ('i', RED);
		OutChar ('l', RED);
		return (FAILURE);
	}

	OutChar ('P', BROWN);
	OutChar ('i', BROWN);
	OutChar ('e', BROWN);
	OutChar ('c', BROWN);
	OutChar ('e', BROWN);
	OutChar ('P', BROWN);
	OutChar ('C', BROWN);
	OutChar ('M', BROWN);
	return (SUCCESS);

}


/***************************************************************************/
/*																									*/
/*	W_DummyReg (Value)																		*/
/*																									*/
/*	INPUT:	Value	-	The word value to write to the dummy register 			*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will write one byte to CIS address 0.			*/
/*																									*/
/***************************************************************************/

void W_DummyReg (unsigned_8 Value)

{
#ifndef NDIS_MINIPORT_DRIVER
	unsigned_8 FAR *dummyReg;

   dummyReg = (unsigned_8 FAR *)((unsigned_32)LLDMemoryAddress1 << 16);
	*dummyReg = Value;

#else
	NdisWriteRegisterUchar((PUCHAR)LLSAdapter->MmMappedBaseAddr, Value);
#endif

}

