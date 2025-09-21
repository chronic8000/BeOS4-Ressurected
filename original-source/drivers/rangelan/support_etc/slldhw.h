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
/*	$Header:   J:\pvcs\archive\clld\slldhw.h_v   1.9   27 Sep 1996 09:21:28   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldhw.h_v  $										*/
/* 
/*    Rev 1.9   27 Sep 1996 09:21:28   JEAN
/* 
/*    Rev 1.8   14 Jun 1996 16:47:14   JEAN
/* Modified a function prototype.
/* 
/*    Rev 1.7   08 Mar 1996 19:37:22   JEAN
/* Changed a function prototype.
/* 
/*    Rev 1.6   06 Feb 1996 14:40:00   JEAN
/* Cleaned up the file.
/* 
/*    Rev 1.5   31 Jan 1996 13:53:08   Jean
/* Added duplicate header include detection.
/* 
/*    Rev 1.4   12 Oct 1995 13:41:58   JEAN
/* Fixed compiler warnings and added the Header PVCS keyword.
/* 
/*    Rev 1.3   13 Apr 1995 08:52:38   tracie
/* xromtest version and NS16550 FIFO mode
/* 
/*    Rev 1.2   16 Mar 1995 16:21:26   tracie
/* Added support for ODI
/* 
/*    Rev 1.1   14 Dec 1994 10:49:26   tracie
/* 																								*/
/* Initial revision		21 Nov 1994		Tracie										*/
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef SLLDHW_H
#define SLLDHW_H

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "slldhw.c"						*/
/*																							*/
/*********************************************************************/

unsigned_8 	ResetRFNC (unsigned_8 *TxMode, unsigned_8 IntNum);
unsigned_8	TxRequest (unsigned_16 PktLength, unsigned_8 Txmode);
unsigned_8 	EndOfTx (void);
unsigned_16 RxRequest (unsigned_8 Txmode);
void 			EndOfRx (void);
void 			SetWakeupBit (void);
void			ClearWakeupBit (unsigned_8 OldControlValue, unsigned_8 OldStatusValue);
void 			ClearNAK (void);
void			SLLDResetModem (void);
void 			SerialInit (unsigned_8 baud);
void 			TxData (unsigned_8 FAR *DataPtr, unsigned_16 DataLength, unsigned_8 Txmode);
unsigned_8 	RxData (unsigned_8 *RxPktPtr, unsigned_16 Length, unsigned_8 TxMode);
void			GetRxData (unsigned_8 FAR *RxPktPtr, unsigned_16 Length, unsigned_8 TxMode);

#endif /* SLLDHW_H */
