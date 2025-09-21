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
/*	$Header:   J:\pvcs\archive\clld\lldwbrp.h_v   1.3   06 Feb 1996 14:41:30   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldwbrp.h_v  $																						*/
/* 
/*    Rev 1.3   06 Feb 1996 14:41:30   JEAN
/* Fixed some function prototypes.
/* 
/*    Rev 1.2   31 Jan 1996 13:53:56   Jean
/* Added duplicate header include detection.
/* 
/*    Rev 1.1   12 Oct 1995 13:42:34   JEAN
/* Added missing copyright and edit history.
/* 
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDWBRP_H
#define LLDWBRP_H

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "lldwbrp.c"					*/
/*																							*/
/*********************************************************************/

void LLDTxWBRPPacket (struct WBRPPacket *PacketPtr);
void LLDTxRepeatDataPacket (struct LLDRepeatingPacket *PacketPtr,unsigned_16 Length); 
unsigned_8 LLDSetMaxSyncDepth (unsigned_8 SyncDepth);
unsigned_8 LLDSetUARTBaudRate (unsigned_8 BaudRateCode);
unsigned_8 LLDSetHopStatistics (unsigned_8 OnOffFlag);
unsigned_8 LLDSetFreqTable (unsigned_8 *FreqTblPtr);
unsigned_8 LLDGetFrequencyTable (void);

#endif /* LLDWBRP_H */