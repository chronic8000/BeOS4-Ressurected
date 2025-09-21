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
/*	$Header:   J:\pvcs\archive\clld\hardware.h_v   1.12   27 Sep 1996 09:19:42   JEAN  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\hardware.h_v  $										*/
/* 
/*    Rev 1.12   27 Sep 1996 09:19:42   JEAN
/* 
/*    Rev 1.11   14 Jun 1996 16:42:02   JEAN
/* Comment changes.
/* 
/*    Rev 1.10   12 May 1996 17:52:36   TERRYL
/* Modified ClearWakeupBit() declaration.
/* 
/*    Rev 1.9   31 Jan 1996 13:41:48   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.8   17 Nov 1995 16:44:28   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.7   12 Oct 1995 12:06:26   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.6   23 May 1995 09:48:32   tracie
/* 
/*    Rev 1.5   16 Mar 1995 16:20:32   tracie
/* Added support for ODI
/* 
/*    Rev 1.4   27 Dec 1994 15:46:16   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:32   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:32   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:44   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "hardware.c"					*/
/*																							*/
/*********************************************************************/

#ifndef HARDWARE_H
#define HARDWARE_H

unsigned_8		ResetRFNC (unsigned_8 *Txmode, unsigned_8 IntNum);
void				SetWakeupBit (void);
void				ClearWakeupBit (unsigned_8 OldControlValue, unsigned_8 OldStatusValue);
unsigned_8		TxRequest (unsigned_16 PktLength, unsigned_8 Txmode);
void				TxData (unsigned_8 FAR *DataPtr, unsigned_16 DataLength, unsigned_8 Txmode);
unsigned_8		EndOfTx (void);
unsigned_16		RxRequest (unsigned_8 Txmode);
unsigned_8		RxData (unsigned_8 *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode);
void				GetRxData (unsigned_8 FAR *RxPktPtr, unsigned_16 Length, unsigned_8 Txmode);
void				EndOfRx (void);
void				ClearNAK (void);

#endif /* HARDWARE_H */

