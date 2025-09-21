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
/*	$Header:   J:\pvcs\archive\clld\lldisr.h_v   1.10   12 Sep 1997 17:03:26   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldisr.h_v  $										*/
/* 
/*    Rev 1.10   12 Sep 1997 17:03:26   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.9   27 Sep 1996 09:15:40   JEAN
/* 
/*    Rev 1.8   14 Jun 1996 16:40:30   JEAN
/* Comment changes.
/* 
/*    Rev 1.7   06 Feb 1996 14:33:00   JEAN
/* Fixed a function prototype.
/* 
/*    Rev 1.6   31 Jan 1996 13:38:02   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   17 Nov 1995 16:42:56   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 13:59:58   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.3   22 Feb 1995 10:38:52   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:30   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:28   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:40   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDISR_H
#define LLDISR_H

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "lldisr.c"						*/
/*																							*/
/*********************************************************************/

void	LLDIsr (void);
void	HandleInitCmdRsp (unsigned_8 *RspPktPtr);
void	HandleDataCmdRsp (unsigned_8 *RspPktPtr, unsigned_16 RawLength);
void	HandleDiagCmdRsp (unsigned_8 *RspPktPtr);
void	HandleTransmitComplete (struct PacketizeTxComplete *Response);
void	HandleInSync (struct PacketizeInSync *InSyncMsg);
void	HandleInSyncMSTASearch (struct PacketizeInSync *InSyncMsg);
void  HandleHopStatistics ( struct PacketizeHopStat *HopStatPtr);

#endif /* LLDISR_H */
