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
/*	$Header:   J:\pvcs\archive\clld\lldrx.h_v   1.13   11 Aug 1998 11:57:42   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldrx.h_v  $											*/
/* 
/*    Rev 1.13   11 Aug 1998 11:57:42   BARBARA
/* LLDReceive declaration eliminated.
/* 
/*    Rev 1.12   09 Jun 1998 14:01:24   BARBARA
/* ConvertLongIntoBigEndian added.
/* 
/*    Rev 1.11   03 Jun 1998 14:43:50   BARBARA
/* Proxim protocol definitions added.
/* 
/*    Rev 1.10   24 Mar 1998 09:15:20   BARBARA
/* SendLLDPingResponse() and CheckRxBuffTimeOut() added.
/* 
/*    Rev 1.9   15 Jan 1997 17:57:56   BARBARA
/* LLDDuplicateDetection, LLDRxFragments declar. changed. CheckRoamNotity added.
/* 
/*    Rev 1.8   27 Sep 1996 09:15:14   JEAN
/* 
/*    Rev 1.7   14 Jun 1996 16:39:38   JEAN
/* Comment changes.
/* 
/*    Rev 1.6   31 Jan 1996 13:37:42   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   12 Oct 1995 11:58:20   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.4   20 Jun 1995 15:52:44   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.3   16 Mar 1995 16:19:46   tracie
/* Added support for ODI
/* 
/*    Rev 1.2   22 Sep 1994 10:56:30   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:26   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:40   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDRX_H
#define LLDRX_H
#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#include "hw.h"
#include "sw.h"
#endif

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "lldrx.c"						*/
/*																							*/
/*********************************************************************/

void				HandleDataReceive (struct PRspData *DataPkt);
unsigned_16		LLDRawReceive (struct LLDRxPacketDescriptor FAR *PktDesc, unsigned_16 Offset, unsigned_16 Length);
unsigned_16		LLDDuplicateDetection (struct PRspData *LookAheadBuf, struct NodeEntry	*CurrEntry);
unsigned_8		LLDRxFragments (unsigned_8 Data [], unsigned_16 Length, struct PRspData *Packet, struct NodeEntry	*CurrEntry);
void 				LLDRepeat (unsigned_8 LookAheadBuf [], unsigned_16 Length);
int				CheckRoamNotify (struct PRspData *DataPkt);
void 				ProcessProximPkt (unsigned_8 *DataPtr, unsigned_16 Length, struct PRspData *DataPkt);
void				SendManagementDataPkt(unsigned_16 Length, unsigned_16 SubType, unsigned_8 *Data);
struct RXBStruct *CheckRxBuffTimeOut (void);
unsigned_16 	SetupMasterHistory(struct RoamHist *RoamHistory, 
							unsigned_8 Length, unsigned_8 *Data);
void ConvertLongIntoBigEndian (unsigned_8 *buffer, unsigned_32 longVariable);

#ifdef PROTOCOL
void				HandleProtReceive (struct PRspProt *DataPkt);
#endif

#endif /* LLDRX_H */
