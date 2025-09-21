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
/*	$Header:   J:\pvcs\archive\clld\hardware.c_v   1.39   10 Apr 1997 17:21:06   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\hardware.c_v  $										*/
/* 
/*    Rev 1.39   10 Apr 1997 17:21:06   BARBARA
/* LLDuISA changed to LLDMicroISA.
/* 
/*    Rev 1.38   10 Apr 1997 17:06:52   BARBARA
/* Micro ISA implemented.
/* 
/*    Rev 1.37   10 Apr 1997 14:36:00   BARBARA
/* Support for Micro ISA added.
/* 
/*    Rev 1.36   05 Mar 1997 14:37:14   BARBARA
/* Unused variable deleted from TxData.
/* 
/*    Rev 1.35   05 Mar 1997 11:08:06   BARBARA
/* Code for NDIS_MINIPORT_DRIVER in TxData was substituted with W_DataRegW & B.
/* 
/*    Rev 1.34   24 Jan 1997 17:24:04   BARBARA
/* Issuing Reset after 5 sec of NAKs added.
/* 
/*    Rev 1.33   15 Jan 1997 17:34:30   BARBARA
/* Delay has been added for Intermec Power Management.
/* 
/*    Rev 1.32   27 Sep 1996 09:04:12   JEAN
/* 
/*    Rev 1.31   26 Sep 1996 17:02:38   JEAN
/* 
/*    Rev 1.30   24 Sep 1996 17:04:04   BARBARA
/* FastIO is set by the parameter not a hardware (1pc).
/* 
/*    Rev 1.29   28 Aug 1996 15:35:28   BARBARA
/* A loop has been added to set up StayAwake several times in a row.
/* 
/*    Rev 1.27   15 Jul 1996 14:32:42   TERRYL
/* Type-casted the time difference (from LLSGetCurrentTime) to 16-bit.
/* Modified TxData() for Ndis drivers. Instead of calling W_DataRegB()
/* and W_DataRegW() in the for loop, we incorporate the two routines in
/* the TxData().
/* 
/*    Rev 1.26   28 Jun 1996 17:31:24   BARBARA
/* One Piece PCMCIA code has been added.
/* 
/*    Rev 1.25   14 Jun 1996 16:18:34   JEAN
/* Made changes to support a one piece PCMCIA card.
/* 
/*    Rev 1.24   14 May 1996 09:21:04   TERRYL
/* Modified SetWakeupBit() and ClearWakeupBit()
/* Added in more debugging information
/* 
/*    Rev 1.23   16 Apr 1996 11:30:36   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* Added code to make sure interrupts are enabled whenever
/* we sit in a timeout loop.
/* 
/*    Rev 1.22   22 Mar 1996 14:48:14   JEAN
/* Changed some debug color.
/* 
/*    Rev 1.21   08 Mar 1996 19:06:58   JEAN
/* Removed some test code.
/* 
/*    Rev 1.20   07 Feb 1996 15:38:52   JEAN
/* Fixed a timeout problem in EndOfTx().
/* 
/*    Rev 1.19   06 Feb 1996 14:25:28   JEAN
/* Comment changes.
/* 
/*    Rev 1.18   31 Jan 1996 13:08:10   JEAN
/* Modified ResetRFNC() to reduce the timeout and the number of
/* reset attempts.  Also added a flag to solve the recursive
/* interrupt problem and changed the ordering of some header
/* include files.
/* 
/*    Rev 1.17   07 Dec 1995 12:10:38   JEAN
/* Removed a duplicate header include.
/* 
/*    Rev 1.16   17 Nov 1995 16:36:02   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.15   16 Oct 1995 16:05:08   JEAN
/* Removed some commented out code.
/* 
/*    Rev 1.14   12 Oct 1995 11:50:54   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.13   24 Jul 1995 18:37:00   truong
/* 
/*    Rev 1.12   20 Jun 1995 16:10:20   tracie
/* 
/*    Rev 1.11   23 May 1995 09:42:58   tracie
/* 
/*    Rev 1.10   13 Apr 1995 08:48:38   tracie
/* XROMTEST version
/* 
/*    Rev 1.9   16 Mar 1995 16:16:04   tracie
/* Added support for ODI
/* 
/*    Rev 1.8   09 Mar 1995 15:11:00   hilton
/* 
/*    Rev 1.7   22 Feb 1995 10:27:16   tracie
/* 
/*    Rev 1.6   05 Jan 1995 09:54:28   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.5   27 Dec 1994 15:44:22   tracie
/* 
/*    Rev 1.3   29 Nov 1994 12:45:08   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:18   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:10   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:26   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/


#include	"constant.h"
#include "lld.h"
#include "asm.h"
#include	"hardware.h"
#include "port.h"
#include "pcmcia.h"
#include	"bstruct.h"

#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8		LLDPCMCIA;
extern unsigned_8		LLDMicroISA;
extern unsigned_8		LLDInit365Flag;
extern unsigned_8		LLDNeedReset;
extern unsigned_8	   LLDOEM;
extern unsigned_8		LLDFastIO;
extern unsigned_8		LLDOnePieceFlag;
extern unsigned_8		LLDInactivityTimeOut;
extern unsigned_8		LLDOEMCustomer;
extern unsigned_8		LLDNAKTime;

#endif

#ifdef NDIS_MINIPORT_DRIVER
extern unsigned_32 	MapDataReg;
#endif


/***************************************************************************/
/*																									*/
/*	ResetRFNC (*TxMode, IntNum)															*/
/*																									*/
/*	INPUT:	TxMode 	-	Pointer to the Bus transfer mode to use				*/
/*                      8 or 16 bit														*/
/*				IntNum 	-	Interrupt Number to use.									*/
/*																									*/
/*	OUTPUT:	Returns	-	0 = Success, 0xFF = Failure								*/
/*																									*/
/*	DESCRIPTION: This routine resets the RFNC once, checks/sets/ the 8/16	*/
/*              bit operating mode and writes the interrupt select			*/
/*              register with the passed interrupt number.		 				*/
/*																									*/
/***************************************************************************/

unsigned_8 ResetRFNC (unsigned_8 *Txmode, unsigned_8 IntNum)

{	unsigned_16			StartTime;
	unsigned_8			resetValue = 0;
	FLAGS					oldflag;
	static unsigned_8	ResetInProgress = FALSE;

#ifdef CSS
	if ((LLDPCMCIA) && (LLDInit365Flag))
	{	if (ReadPCICReg (PwrCtlReg) != 0xD5)
		{	if (ConfigureMachine ())
			{
				return (FAILURE);
			}
		}
	}
#endif

	oldflag = PreserveFlag ();
	DisableInterrupt ();
	
	dprintf( "rl: ResetRFNC: reset it progress\n" );
	if (ResetInProgress == TRUE)
	{	
		RestoreFlag (oldflag);
		return FAILURE;
	}
	ResetInProgress = TRUE;


	OutChar ('R', GREEN);

	if (LLDOnePieceFlag || LLDMicroISA)
		resetValue = RESETRFNCBIT;

	/*----------------------------------------------*/
	/* Make sure card is not in standby.				*/
	/*																*/
	/*----------------------------------------------*/

	if ((R_Status () & STANDBYBIT))
	{	W_CtrlReg (resetValue);
		W_CtrlReg ((unsigned_8)(resetValue | WAKEUPBIT));
		Delay (7);
		W_CtrlReg (resetValue);
		Delay (7);
	}

	/*----------------------------------------------*/
	/* Reset card and clear the status register.		*/
	/*																*/
	/*----------------------------------------------*/

	W_CtrlReg (resetValue);
	W_CtrlReg (resetValue);
	W_CtrlReg ((unsigned_8)(resetValue | NMIRFNCBIT));
	Delay (67);
	W_StatusReg (0);

	if (!LLDOnePieceFlag && !LLDMicroISA)
		W_CtrlReg (RESETRFNCBIT | NMIRFNCBIT | BUSMODEBIT);
	else
		W_CtrlReg (NMIRFNCBIT | BUSMODEBIT | WAKEUPBIT);

	Delay (67);
	W_DataRegW (SIGNATUREWORD);
	W_StatusReg (RESETSTATUS);

	/* Reenable interrupts so that Time can be updated */
	EnableInterrupt ();
	StartTime = LLSGetCurrentTime ();
	while ((R_Status () != RESETSTATUS) &&	((unsigned_16)(LLSGetCurrentTime () - StartTime) < RESETTIMEOUT))
		;
	DisableInterrupt ();

	if (R_Status () != RESETSTATUS)
	{
		dprintf( "rl: ResetRFNC: reset failed!\n" );
		ResetInProgress = FALSE;
		RestoreFlag (oldflag);
		return (FAILURE);
	}

	if (*Txmode == BYTEMODE)
	{	
		if (!LLDOnePieceFlag && !LLDMicroISA)
			W_CtrlReg (RESETRFNCBIT | NMIRFNCBIT);
		else
			W_CtrlReg (NMIRFNCBIT);
		W_DataRegB (BUSMODEBIT);
	}
	else if (*Txmode == WORDMODE )
	{	W_DataRegW (0);
	}
	else
	{	if (R_DataRegW () == SIGNATUREWORDCMP)
		{ 	W_DataRegW (0);
			*Txmode = WORDMODE;
		}
		else
		{	
			if (!LLDOnePieceFlag && !LLDMicroISA)
				W_CtrlReg (RESETRFNCBIT | NMIRFNCBIT);
			else
				W_CtrlReg (NMIRFNCBIT);
			W_DataRegB (BUSMODEBIT);
			*Txmode = BYTEMODE;
		}
	}

#ifdef CSS
	if (LLDPCMCIA)
		ClearPCMCIAInt ();
#endif

	W_StatusReg (READYSTATUS);

	if (LLDPCMCIA || LLDOEM)
	{	W_IntSelReg (15);
	}
	
	else
	{	W_IntSelReg (IntNum);
	}

	ResetInProgress = FALSE;
	RestoreFlag (oldflag);
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/* SetWakeupBit ()																			*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION:  This procedure will set the wakeup bit, but will check		*/
/*		to see if the card was in standby previously to see if it needs to 	*/
/*		allow the card time to regain power.  The stayawake bit is also set	*/
/*		in the status register to tell the controller that a command is in	*/
/*		progress and to not sleep.															*/
/*																									*/
/***************************************************************************/

void SetWakeupBit (void)

{	unsigned_8	Status;
	short	i;

	SetBit_StatusReg (STAYAWAKE);
	Delay_uS(100);
	if (LLDInactivityTimeOut && LLDOEMCustomer)
		Delay(167);
	Status = R_Status ();
	SetBit_CtrlReg (WAKEUPBIT);
	if (Status & STANDBYBIT)
	{	
		for (i=0; i<=8; i++)
		{	Delay (2);
			SetBit_StatusReg (STAYAWAKE);
		}
	}
}



/***************************************************************************/
/*																									*/
/* ClearWakeupBit (OldControlValue, OldStatusValue)																			*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/* DESCRIPTION:  This procedure will clear the wakeup bit in the control	*/
/*		register as well as the stayawake bit in the status register.			*/
/*																									*/
/***************************************************************************/

void ClearWakeupBit (unsigned_8 OldControlValue, unsigned_8 OldStatusValue)

{ 
	if (NOT(OldStatusValue & STAYAWAKE))
		ClearBit_StatusReg (STAYAWAKE); 
	
	if (NOT(OldControlValue & WAKEUPBIT))
		ClearBit_CtrlReg (WAKEUPBIT);
}



/***************************************************************************/
/*																									*/
/*	TxRequest (PktLength, Txmode)															*/
/*																									*/
/*	INPUT:	PktLength	-	Length of packet to transmit to the card			*/
/*				TxMode		-	Bus mode for transmission, 8/16 bit					*/
/*																									*/
/*	OUTPUT:	Returns		-	ACK / NAK or Failure										*/
/*																									*/
/*	DESCRIPTION: This routine will request permission from the card to		*/
/* 	transmit a packet	of given length to it by writing the length to		*/
/*		the data register, setting the TX status to REQ a transfer, and		*/
/*		waiting for and ACK from the controller.  In byte mode, the length	*/
/*		is written LSB first and requires an additional REQ/ACK handshake.	*/
/*																									*/
/***************************************************************************/

unsigned_8 TxRequest (unsigned_16 PktLength, unsigned_8 Txmode)

{	unsigned_16	StartTime;
	unsigned_8	status;
	FLAGS			oldflag;

	if (Txmode == WORDMODE)
	{	
		W_TxStatusReg (STATTX_REQ);
		W_DataRegW (PktLength);
		IntRFNCTx ();

		/*-------------------------------------------------------------------*/
		/* Make sure interrupts are enabled so that the time can be updated.	*/
		/*																							*/
		/*-------------------------------------------------------------------*/
		oldflag = PreserveFlag ();
		EnableInterrupt ();

		StartTime = LLSGetCurrentTime();
		while (((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME))
		{	
			status = R_TxStatus ();
			if (status == STATTX_ACK)
			{ 
				RestoreFlag (oldflag);
				LLDNAKTime = 100;
				return (ACK);
			}
			else if (status == STATTX_NAK)
			{	
				RestoreFlag (oldflag);
				if (LLDNAKTime == 100)
					LLDNAKTime--;
				return (NAK);
			}
		}
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('T', RED);
		OutChar ('x', RED);
		OutChar ('S', RED);
		OutChar ('t', RED);
		OutChar ('a', RED);
		OutChar ('t', RED);
		OutHex (status, RED);
		RestoreFlag (oldflag);
		LLDNAKTime = 100;
	
		return (FAILURE);
	}

	else if (Txmode == BYTEMODE)
	{	W_TxStatusReg (STATTX_REQ1);
		W_DataRegB ((unsigned_8)(PktLength & 0xFF));
		IntRFNCTx ();
	  
		/*-------------------------------------------------------------------*/
		/* Make sure interrupts are enabled so that the time can be updated.	*/
		/*																							*/
		/*-------------------------------------------------------------------*/
		oldflag = PreserveFlag ();
		EnableInterrupt ();

		StartTime = LLSGetCurrentTime();
		while (((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME))
		{	status = R_TxStatus ();
			if (status == STATTX_ACK1)
				break;				
		}

		if (status == STATTX_ACK1)
		{	W_DataRegB ((unsigned_8) (PktLength >> 8));
			W_TxStatusReg (STATTX_REQ);

			StartTime = LLSGetCurrentTime();
			while (((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME))
			{	status = R_TxStatus ();
				if (status == STATTX_ACK)
				{	
					RestoreFlag (oldflag);
					LLDNAKTime = 100;
					return (ACK);
				}
				else if (status == STATTX_NAK)
				{	
					RestoreFlag (oldflag);
					if (LLDNAKTime == 100)
						LLDNAKTime--;
					return (NAK);
				}
			}
		}
		RestoreFlag (oldflag);
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('T', RED);
		OutChar ('x', RED);
		OutChar ('S', RED);
		OutChar ('t', RED);
		OutChar ('a', RED);
		OutChar ('t', RED);
		OutHex (status, RED);
		LLDNAKTime = 100;
		return( FAILURE );
	}
	else
	{
		OutChar ('I', RED);
		OutChar ('n', RED);
		OutChar ('v', RED);
		OutChar ('a', RED);
		OutChar ('l', RED);
		OutChar ('i', RED);
		OutChar ('d', RED);
		OutChar ('T', RED);
		OutChar ('x', RED);
		OutChar ('M', RED);
		OutChar ('o', RED);
		OutChar ('d', RED);
		OutChar ('e', RED);
		
	}
	return( FAILURE );
}




/***************************************************************************/
/*																									*/
/*	TxData (DataPtr, DataLength, Txmode)												*/
/*																									*/
/*	INPUT:	*DataPtr		-	Pointer to the data to write to the card			*/
/*				DataLength	-	Number of data bytes to write to the card			*/
/*				TxMode		-	Transfer mode, 8 or 16 bits							*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will inform the card that data is coming		*/
/*		by setting the TX status to the DATA state.  Then it will transfer	*/
/*		the data pointed to by DataPtr to the card.									*/
/*																									*/
/***************************************************************************/

void TxData (	unsigned_8 FAR *DataPtr, unsigned_16 DataLength, unsigned_8 Txmode )

{	
	unsigned_16			i;
	unsigned_16	 FAR *WordDataPtr;

	W_TxStatusReg (STATTX_DAT);

	if (Txmode == WORDMODE)
	{	
		WordDataPtr	= (unsigned_16 FAR *) DataPtr;
		if (LLDFastIO)
		{	for ( i = 0; i < DataLength; i += 2 ) 
			{	W_FastDataRegW (*WordDataPtr++);
			}
		}
		else
		{	for ( i = 0; i < DataLength; i += 2 ) 
			{	
				W_DataRegW (*WordDataPtr++);
			}
		}
	}
	else
	{	
		if (LLDFastIO)
		{	for ( i = 0; i < DataLength; i++ )
			{	W_FastDataRegB (*DataPtr++);
			}
		}
		else
		{	for ( i = 0; i < DataLength; i++ )
			{	
				W_DataRegB (*DataPtr++);
			}
		}
	}

}



/***************************************************************************/
/*																									*/
/*	EndOfTx ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns	-	Success or Failure											*/
/*																									*/
/*	DESCRIPTION: This routine will wait for an end of transmit status			*/
/* 	and will return failure if one is not received within 20 				*/
/* 	milliseconds.																			*/
/*																									*/
/***************************************************************************/

unsigned_8 EndOfTx (void)

{	unsigned_16	StartTime;
	unsigned_8	status;
	FLAGS			oldflag;

	/*-------------------------------------------------------------------*/
	/* Make sure interrupts are enabled so that the time can be updated.	*/
	/*																							*/
	/*-------------------------------------------------------------------*/
	oldflag = PreserveFlag ();
	EnableInterrupt ();

	StartTime = LLSGetCurrentTime ();

  	while (((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME))
  	{	
		status = R_TxStatus ();
		if (status == STATTX_EOT)
  		{	
			RestoreFlag (oldflag);
			return (SUCCESS);
  		}
  	}
	
	OutChar ('F', RED);
	OutChar ('a', RED);
	OutChar ('i', RED);
	OutChar ('l', RED);
 
	OutChar ('T', RED);
	OutChar ('6', RED);
	OutHex (status, RED);

	LLDNeedReset = SET;
	RestoreFlag (oldflag);
  	return (FAILURE);
}



/***************************************************************************/
/*																									*/
/*	RxRequest (Txmode)																		*/
/*																									*/
/*	INPUT:	Txmode	-	Transmit mode 8/16 bits mode								*/
/*																									*/
/*	OUTPUT:	Returns	-	Length of the packet the card is going to send		*/
/*								(0 if no packet).												*/
/*																									*/
/*	DESCRIPTION: This routine will read the RX status register and check		*/
/*		for a REQ status.  If it is a REQ, it reads the data register to		*/
/*		get the length of the packet the RFNC is going to send.					*/
/*																									*/
/***************************************************************************/

unsigned_16 RxRequest (unsigned_8 Txmode)

{	unsigned_16	length;
	unsigned_16	StartTime;
	unsigned_8	status;
	FLAGS			oldflag;

	status = R_RxStatus ();

	if (Txmode == WORDMODE)
	{	
		if (status == STATRX_REQ )
		{	return (R_DataRegW ());
		}
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('R', RED);
		OutChar ('x', RED);
		OutChar ('S', RED);
		OutChar ('t', RED);
		OutChar ('a', RED);
		OutChar ('t', RED);
		OutHex (status, RED);
	}

	if (Txmode == BYTEMODE)
	{	if (status == STATRX_REQ1)
		{	length = R_DataRegB ();
			W_RxStatusReg (STATRX_ACK1);
			IntRFNCRx ();
		}
		else
		{
			OutChar ('B', RED);
			OutChar ('a', RED);
			OutChar ('d', RED);
			OutChar ('R', RED);
			OutChar ('x', RED);
			OutChar ('S', RED);
			OutChar ('t', RED);
			OutChar ('a', RED);
			OutChar ('t', RED);
			OutHex (status, RED);
		}

		/*-------------------------------------------------------------------*/
		/* Make sure interrupts are enabled so that the time can be updated.	*/
		/*																							*/
		/*-------------------------------------------------------------------*/
		oldflag = PreserveFlag ();
		EnableInterrupt ();

		StartTime = LLSGetCurrentTime ();
  		while (((unsigned_16)(LLSGetCurrentTime() - StartTime) <= MAXWAITTIME))
	  	{	
			status = R_RxStatus ();
			if (status == STATRX_REQ)
  			{	length += R_DataRegB () << 8;	
				RestoreFlag (oldflag);
				return (length);
	  		}
	  	}
		OutChar ('R', RED);
		OutChar ('x', RED);
		OutChar ('R', RED);
		OutChar ('e', RED);
		OutChar ('q', RED);
		OutChar ('T', RED);
		OutChar ('i', RED);
		OutChar ('m', RED);
		OutChar ('e', RED);
		OutChar ('O', RED);
		OutChar ('u', RED);
		OutChar ('t', RED);
		RestoreFlag (oldflag);
	}
	return (0);
}



/***************************************************************************/
/*																									*/
/*	RxData (RxPktPtr, Length, Txmode)													*/
/*																									*/
/*	INPUT:	RxPktPtr	-	Pointer to the buffer to read the packet into		*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*				Txmode	-	Transmit mode 8/16 bit mode								*/
/*																									*/
/*	OUTPUT: 	Returns	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send an ACK to the card, wait for			*/
/*		the controller to change the status to the DATA state, and will		*/
/*		read the data into the buffer.													*/
/*																									*/
/***************************************************************************/

unsigned_8 RxData (unsigned_8 *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode)

{	unsigned_16		StartTime;
	unsigned_16		*WordPktPtr;
	unsigned_16		i;
	FLAGS				oldflag;

	W_RxStatusReg (STATRX_ACK);
	IntRFNCRx ();

	/*-------------------------------------------------------------------*/
	/* Make sure interrupts are enabled so that the time can be updated.	*/
	/*																							*/
	/*-------------------------------------------------------------------*/
	oldflag = PreserveFlag ();
	EnableInterrupt ();

	StartTime = LLSGetCurrentTime ();
  	while (((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME))
  	{	if (R_RxStatus () == STATRX_DAT)
  		{	if (Txmode == WORDMODE)
			{	WordPktPtr = (unsigned_16 *) RxPktPtr;
				if (LLDFastIO)
				{	for ( i = 0; i < Length; i += 2 )
					{	*WordPktPtr++ = R_FastDataRegW ();
					}
				}
				else
				{	for ( i = 0; i < Length; i += 2 )
					{	*WordPktPtr++ = R_DataRegW ();
					}
				}
			}
			else
			{	
				if (LLDFastIO)
				{	for ( i = 0; i < Length; i++ )
					{	*RxPktPtr++ = R_FastDataRegB ();
					}
				}
				else
				{	for ( i = 0; i < Length; i++ )
					{	*RxPktPtr++ = R_DataRegB ();
					}
				}
			}
			RestoreFlag (oldflag);
			return (SUCCESS);
		}
  	}
	RestoreFlag (oldflag);
	return (FAILURE);
}



/***************************************************************************/
/*																									*/
/*	GetRxData (RxPktPtr, Length, Txmode)												*/
/*																									*/
/*	INPUT:	RxPktPtr	-	Pointer to the buffer to read the packet into		*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*				Txmode	-	Transmit mode 8/16 bit mode								*/
/*																									*/
/*	OUTPUT: 																						*/
/*																									*/
/*	DESCRIPTION: This routine will read	the data into the buffer.  It			*/
/*		differs from RxData because it does not issue an ACK to the RFNC.		*/
/*		This procedure is used when only the lookahead portion of the			*/
/*		buffer is read in.  To receive the rest of the data, this procedure	*/
/*		should be called.																		*/
/*																									*/
/***************************************************************************/

void GetRxData (unsigned_8 FAR *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode)

{	unsigned_16	FAR *WordPktPtr;
	unsigned_16		  i;

	if (Txmode == WORDMODE)
	{	WordPktPtr = (unsigned_16 FAR *) RxPktPtr;

		for ( i = 0; i < Length; i += 2 )
		{	*WordPktPtr++ = R_DataRegW ();
		}
	}
	else
	{	for ( i = 0; i < Length; i++ )
		{	*RxPktPtr++ = R_DataRegB ();
		}
	}
}



/***************************************************************************/
/*																									*/
/*	EndOfRx ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will send an end of receive command				*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

void EndOfRx (void)

{
	W_RxStatusReg (STATRX_EOT);
	IntRFNCRx ();
}




/***************************************************************************/
/*																									*/
/*	ClearNAK ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will send a clear NAK status to the card		*/
/*		and wait for the NAK status to clear.											*/
/*																									*/
/***************************************************************************/

void ClearNAK (void)

{
	W_TxStatusReg (STATTX_CNAK);
	IntRFNCTx ();

}
