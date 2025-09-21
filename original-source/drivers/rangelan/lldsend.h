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
/*	$Header:   J:\pvcs\archive\clld\lldsend.h_v   1.11   24 Mar 1998 08:30:10   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldsend.h_v  $										*/
/* 
/*    Rev 1.11   24 Mar 1998 08:30:10   BARBARA
/* Comment eliminated.
/* 
/*    Rev 1.10   24 Mar 1998 08:08:28   BARBARA
/* LLD Ping added.
/* 
/*    Rev 1.9   27 Sep 1996 09:14:50   JEAN
/* 
/*    Rev 1.8   14 Jun 1996 16:37:00   JEAN
/* Comment changes.
/* 
/*    Rev 1.7   08 Feb 1996 14:36:42   JEAN
/* Added a function prototype.
/* 
/*    Rev 1.6   31 Jan 1996 13:37:12   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   17 Nov 1995 16:42:30   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 11:57:54   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.3   16 Mar 1995 16:19:34   tracie
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

#ifndef LLDSEND_H
#define LLDSEND_H

/**************************************************************************/
/*																								  */
/* These are the prototype definitions for "lldsend.c"						  */
/*																								  */
/**************************************************************************/

unsigned_8 	LLDSendProximPkt (struct LLDTxPacketDescriptor FAR *PktDesc);
unsigned_8	LLDSend (struct LLDTxPacketDescriptor FAR *PktDesc);
unsigned_8	LLDTxFragments (struct LLDTxPacketDescriptor FAR *PktDesc, unsigned_8 TableEntry, unsigned_8 BridgeMode,
									 unsigned_8 LLDEntryAddress [], unsigned_8 LLDBridgeDest []);
unsigned_8	LLDSendFragments (unsigned_8 Buffer [], unsigned_16 BufferOffset, unsigned_16 Length, unsigned_8 BridgeMode,
										unsigned_8 LLDBridgeDest []);
unsigned_8	LLDRawSend (unsigned_8 Packet [], unsigned_16 Length);
void			LLDTxRoamNotify (unsigned_8 LLDMSTAAddr [], unsigned_8 OldMSTAAddr [], unsigned_8 NotifyMsg [], unsigned_16 Length);
void			HandleTransmitComplete (struct PacketizeTxComplete *Response);
unsigned_8	IsTxProximPacket (struct LLDTxPacketDescriptor FAR *TxPacket);
unsigned_8 	IsProximPacket (unsigned_8 *Buffer);

#endif /* LLDSEND_H */
