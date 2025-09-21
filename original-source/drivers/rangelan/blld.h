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
/*	$Header:   J:\pvcs\archive\clld\blld.h_v   1.11   24 Mar 1998 09:16:42   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\blld.h_v  $											*/
/* 
/*    Rev 1.11   24 Mar 1998 09:16:42   BARBARA
/* LLDFreeInternalTXBuffer() added.
/* 
/*    Rev 1.10   15 Jan 1997 17:10:26   BARBARA
/* ROAMING_DISABLE _ENABLE _STATE _DELAY, NOT_ROAMING were added.
/* 
/*    Rev 1.9   27 Sep 1996 09:16:56   JEAN
/* 
/*    Rev 1.8   14 Jun 1996 16:40:56   JEAN
/* Comment changes.
/* 
/*    Rev 1.7   15 Mar 1996 14:03:18   TERRYL
/* Added LLDSetupAutoBridging(void)
/* 
/*    Rev 1.6   31 Jan 1996 13:38:24   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   17 Nov 1995 16:43:18   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 12:04:08   JEAN
/* Added a function prototype and the Header PVCS keyword.
/* 
/*    Rev 1.3   16 Mar 1995 16:19:56   tracie
/* Added support for ODI
/* 
/*    Rev 1.2   22 Sep 1994 10:56:32   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:28   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:40   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef BLLD_H
#define BLLD_H


/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "blld.c"						*/
/*																							*/
/*********************************************************************/

void				LLDInitTable (void);
unsigned_8		LLDGetEntry (unsigned_8 NodeAddress [], unsigned_8 *TableEntry);
void				LLDAddToQueue (struct LLDTxPacketDescriptor FAR *PktDesc, unsigned_8 TableEntry);
void				LLDPullFromQueue (unsigned_8 TableEntry);
void				LLDAddToTimedQueue	(struct LLDTxPacketDescriptor FAR *PktDesc, unsigned_8 TableEntry,
												 unsigned_16 StartTime, unsigned_16 MaxWait, unsigned_8 FlushQFlag);
void				LLDPullFromTimedQueue (void);
void				LLDFlushQ (unsigned_8 TableEntry);
void				LLDResetQ (void);
void				LLDFlushTimedQueue (void);
void 				LLDSetupAutoBridging(void);
void 				LLDFreeInternalTXBuffer (struct LLDTxPacketDescriptor *TXBuffer);

#endif /* BLLD_H */
