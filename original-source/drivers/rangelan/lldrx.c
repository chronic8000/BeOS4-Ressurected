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
/*	$Header:   J:\pvcs\archive\clld\lldrx.c_v   1.42   11 Aug 1998 15:58:24   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldrx.c_v  $											*/
/* 
/*    Rev 1.42   11 Aug 1998 15:58:24   BARBARA
/* LLDReceive fixes releated to the NetXray not working with RL2/WINNT/95 driver
/* 
/*    Rev 1.41   10 Jun 1998 15:25:52   BARBARA
/* Big Endian for Proxim packet again.
/* 
/*    Rev 1.40   09 Jun 1998 13:57:58   BARBARA
/* Station Network Management Data is now using only type 7.
/* 
/*    Rev 1.39   04 Jun 1998 12:08:08   BARBARA
/* 
/*    Rev 1.38   04 Jun 1998 11:32:54   BARBARA
/* Proxim protocol related changes added.
/* 
/*    Rev 1.37   07 May 1998 09:24:54   BARBARA
/* LLDReceive changed to handle the case of LLDReceive being called multiple 
/* times.
/* memcpy changed to CopyMemory. memset changed to SetMemory.
/* 
/*    Rev 1.36   24 Mar 1998 08:42:44   BARBARA
/* LLDPing added. Time out of the RxBuffers added.
/* 
/*    Rev 1.35   12 Sep 1997 16:50:08   BARBARA
/* LLDREPEATBIT related change.
/* 
/*    Rev 1.34   24 Jun 1997 10:16:12   BARBARA
/* The RepeatFlag in the NodeTable is set when LLDMSTARFlag==0 not as it was 
/* before LLDMSTAFlag != 0
/* 
/*    Rev 1.33   10 Apr 1997 17:53:26   BARBARA
/* Casting to (unsigned_16) added to keep the compliler quiet.
/* 
/*    Rev 1.32   10 Apr 1997 14:22:00   BARBARA
/* Casting added when accessing PRDLLDHeader4.
/* 
/*    Rev 1.31   15 Mar 1997 13:34:54   BARBARA
/* NodeExistsFlag added.
/* 
/*    Rev 1.30   15 Jan 1997 18:08:08   BARBARA
/* LLDDuplicateDetection, LLDRxFragments changed. CheckRoamNotify added.
/* 
/*    Rev 1.29   11 Oct 1996 11:07:42   BARBARA
/* Casting to unsigned_16 added to keep the compiler quiet.
/* 
/*    Rev 1.28   14 Jun 1996 16:11:52   JEAN
/* Comment changes.
/* 
/*    Rev 1.27   28 May 1996 16:10:18   TERRYL
/* Added debug information to print out the sequence number for received 
/* packets.
/* 
/*    Rev 1.26   16 Apr 1996 11:25:50   JEAN
/* Changed a constant to a define and fixed some spelling errors.
/* 
/*    Rev 1.25   01 Apr 1996 15:17:54   JEAN
/* Fixed a bug in LLDDuplicateDetection where the first 
/* fragment number is missing but not detected.
/* 
/*    Rev 1.24   22 Mar 1996 14:41:08   JEAN
/* Added a packet receive counter.
/* 
/*    Rev 1.23   13 Mar 1996 17:50:26   JEAN
/* Added some sanity checks and debug code.
/* 
/*    Rev 1.22   08 Mar 1996 19:00:36   JEAN
/* Removed some debug color prints.
/* 
/*    Rev 1.21   07 Feb 1996 15:38:34   JEAN
/* Made a change to LLDRepeat ().
/* 
/*    Rev 1.20   06 Feb 1996 14:19:08   JEAN
/* Fixed a problem in LLDRepeat() where we copy the reset of the
/* packet into the wrong offset in the look ahead buffer.
/* 
/*    Rev 1.19   31 Jan 1996 12:58:26   JEAN
/* Changed some constants to macros and changed the ordering
/* of some header include files.
/* 
/*    Rev 1.18   19 Dec 1995 18:05:30   JEAN
/* Removed a duplicate comment.
/* 
/*    Rev 1.17   14 Dec 1995 15:09:40   JEAN
/* Added a header include file.
/* 
/*    Rev 1.16   17 Nov 1995 16:34:10   JEAN
/* Check in for the RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.15   12 Oct 1995 11:34:48   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* Added #defines for bridge states.
/* 
/*    Rev 1.14   24 Jul 1995 18:36:18   truong
/* 
/*    Rev 1.13   20 Jun 1995 15:49:18   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.12   24 Apr 1995 15:47:14   tracie
/* LLDOutOfSyncCount should be cleared.
/* 
/*    Rev 1.11   13 Apr 1995 08:47:50   tracie
/* XROMTEST version
/* 
/*    Rev 1.10   16 Mar 1995 16:14:28   tracie
/* Added support for ODI
/* 
/*    Rev 1.9   09 Mar 1995 15:07:24   hilton
/* 
/*    Rev 1.8   22 Feb 1995 10:29:54   tracie
/* Add WBRP functions
/* 
/*    Rev 1.7   05 Jan 1995 09:53:32   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.6   27 Dec 1994 15:43:44   tracie
/* 
/*    Rev 1.4   14 Dec 1994 11:04:12   tracie
/* Modified to work with the Serial LLD (8250 UART working)
/* 
/*    Rev 1.3   29 Nov 1994 12:44:36   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:12   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:02   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:20   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#include "keywords.h"
#endif

#include <string.h>
#include "constant.h"
#include	"lld.h"
#include "pstruct.h"
#include "blld.h"
#include "bstruct.h"
#include "lldsend.h"
#include "lldrx.h"
#include "lldpack.h"
#ifndef SERIAL
#include "hardware.h"
#else
#include "slld.h"
#include "slldbuf.h"
#include "slldhw.h"
#endif
#include	"asm.h"
#ifdef WBRP
#include "rptable.h"
#include "wbrp.h"
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
#ifdef NDIS_MINIPORT_DRIVER
extern	PLAN_ADAPTER				LLSAdapter;
#endif
extern unsigned_8			LLDTransferMode;
extern unsigned_8			LLDBridgeFlag;
extern unsigned_8			LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_8			PacketizePktBuf [128];
extern unsigned_8			LLDTxMode;
extern unsigned_8			LLDMSTAFlag;
extern unsigned_8			LLDNoRepeatingFlag;
extern unsigned_8			LLDROMVersion [ROMVERLENGTH];
extern unsigned_8			LLDDomain;
extern unsigned_8			LLDRoamingFlag;
extern unsigned_8			LLDRoamConfiguration;
extern unsigned_8			LLDMSTASyncName[NAMELENGTH+1];
extern unsigned_8			LLDMSTAAddr[ADDRESSLENGTH];
extern unsigned_8			LLDTicksToSniff;
extern unsigned_8			NotesBuffer[NOTES_BUFFER_SIZE];
extern unsigned_8			NextMaster;
extern unsigned_8			StationName[STATION_NAME_SIZE];
extern unsigned_8			LLDPCMCIA;
extern unsigned_8			LLDMicroISA;
extern unsigned_8			LLDOEM;
extern unsigned_8			LLDOnePieceFlag;
extern unsigned_8			LLDPeerToPeerFlag;
extern unsigned_16		LLDInactivityTimeOut;
extern unsigned_16		LLDUnknownProximPkt;
extern unsigned_32		ResetStatisticsTime;

extern struct NodeEntry	NodeTable [TOTALENTRY];
extern unsigned_32		LLDRx;
extern unsigned_32		LLDRxLookAhead;
extern unsigned_32 		LLDBadCopyLen;
extern unsigned_32		FramesXmit;
extern unsigned_32		FramesXmitQFSK;
extern unsigned_32		FramesXmitBFSK;
extern unsigned_32		FramesXmitFrag;
extern unsigned_32		FramesACKError;
extern unsigned_32		FramesCTSError;
extern unsigned_32		FramesRx;
extern unsigned_32		FramesRxDuplicate;
extern unsigned_32		FramesRxFrag;
extern unsigned_32		TotalTxPackets;
extern unsigned_32		LLDErrResetCnt;

extern struct TXBStruct	TXBuffers [TOTALTXBUF];
extern struct TXBStruct	*FreeTXBuffer;
extern struct RoamHist	RoamHistory[MAXROAMHISTMSTRS];

unsigned_16			LookAheadLength;
unsigned_16			RawLookAheadLength;
unsigned_16			HardwareHeader;
unsigned_8 			*LLDLookAheadBufPtr;
unsigned_16			RcvPktLength;

unsigned_8			RxFlag;
unsigned_16			RxStartTime;

struct RXBStruct	RXBuffers [TOTALRXBUF];
struct RXBStruct	*FreeRXBuffer;
struct RXBStruct	*RXBuffersPtr;
#endif

const char DriverVersionString[DRIVERVERLENGTH];

/***************************************************************************/
/*																									*/
/*	HandleDataReceive (*DataPkt)															*/
/*																									*/
/*	INPUT: 	DataPkt	-	Pointer to the data packet.								*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure is called by LLDISR when a data packet		*/
/*		arrives.  RawLookAheadLength bytes of data have already been read		*/
/*    from the RFNC.  These bytes are store in the LLDLookAheadBuf, which	*/
/*    DataPkt points to.																	*/
/*																									*/
/***************************************************************************/

void HandleDataReceive (struct PRspData *DataPkt)

{	struct NodeEntry	*NodeEntryPtr;
	unsigned_16			Length;
	unsigned_8			TableEntry;
	unsigned_8			*BSourceAddr;
	unsigned_8			*SourceAddr;
	unsigned_8			*DataPtr;


	LLDRx++;

	/*-------------------------------------------------*/
	/* If this is the WBRP LLD, then call WBRP 			*/
	/* functions to handle the Receiving process.		*/
	/*																	*/
	/* LLDHeader1's Bit0 tells whether its a WBRP type */
	/* of packet.													*/
	/*																	*/
	/* LLDHeader2's Bit4 tells whether its a Repeating */
	/* data packet or a WBRP Reply packet.					*/
	/*-------------------------------------------------*/

#ifdef WBRP

	if ( LLDRepeatingWBRP )
	{
		if (DataPkt->PRDLLDHeader2 & LLDWBRPBIT)
		{
			ProcessWBRPPacket ( (struct WBRPPacket *) DataPkt );
			return;
		}
		else
		{
			if (ProcessDataPacket((struct LLDRepeatingPacket *) DataPkt) == DONOT_PASSITUP )
				return;
		}
	}
#endif

	/*-------------------------------------------------*/
	/* Check if need to repeat the packet.					*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((DataPkt->PRDLLDHeader2 & LLDREPEATBIT))
	{	
		OutChar ('R', BROWN);
		OutChar ('r', BROWN);

		if ((LLDMSTAFlag) && (LLDNoRepeatingFlag == 0))
		{	Length = (DataPkt->PRDLength [0] << 8) + DataPkt->PRDLength [1];
			/*----------------------------------------------------*/ 
			/* Get the packet length from the packetized command	*/
			/* header.  Subtract 4 to exclude the 4-byte LLD		*/
			/* header.  We are just interested in the packet		*/
			/* length.  The receive packetized command packet 		*/
			/* header is larger than the transmit packetized		*/
			/* command header.												*/
			/*----------------------------------------------------*/ 
			LLDRepeat ((unsigned_8 *)DataPkt, (unsigned_16)(Length - 4));
		}

		/*----------------------------------------------*/
		/* If this is a bridge packet that originated	*/
		/* from us, don't pass it up.							*/
		/*																*/
		/*----------------------------------------------*/

		if (DataPkt->PRDLLDHeader2 & LLDBRIDGEBIT)
		{	if (memcmp (((struct BPRspData *)DataPkt)->BPRDSrcAddr, LLDNodeAddress, ADDRESSLENGTH) == 0)
			{	return;
			}
		}
		
		/*----------------------------------------------*/
		/* Or if it's a regular packet from us, quit.	*/
		/*																*/
		/*----------------------------------------------*/

		else 
		{	if (memcmp (DataPkt->PRDSrcAddr, LLDNodeAddress, ADDRESSLENGTH) == 0)
			{	return;
			}
		}
	}

	/*-------------------------------------------------*/
	/* Get the entry into the node table.					*/
	/* If it is a new entry, duplicate detection not	*/
	/* needed, but need to update the sequence number.	*/
	/*																	*/
	/*-------------------------------------------------*/

	if (DataPkt->PRDLLDHeader2 & LLDBRIDGEBIT)
	{	
		LLDGetEntry (((struct BPRspData *)DataPkt)->BPRDSrcAddr, &TableEntry);
	}
	else
	{	
		LLDGetEntry (DataPkt->PRDSrcAddr, &TableEntry);
	}

	NodeEntryPtr = &NodeTable [TableEntry];

	/*----------------------------------------*/
	/*	We received a packet from this node.	*/
	/*	It must definitely exist. Set the		*/
	/*	flag so that a more agressive retry		*/
	/*	mechanism may be used for Tx.				*/
	/*----------------------------------------*/

	NodeEntryPtr->NodeExistsFlag = SET;

	/*----------------------------------------*/
	/* If this is the first receive packet,	*/
	/* then call duplicate detection but		*/
	/* ignore the return status so we keep		*/
	/* the packet.  The first packet could be	*/
	/* a fragmented packet or a roam notify	*/
	/* packet.											*/
	/*														*/
	/*----------------------------------------*/
	if (NodeEntryPtr->FirstRxFlag == 0)
	{
		NodeEntryPtr->FirstRxFlag = 1;
		NodeEntryPtr->LastRxFragNum = WAIT_FOR_FRAG_STATE;
		LLDDuplicateDetection (DataPkt, NodeEntryPtr);
	}
	else if (LLDDuplicateDetection (DataPkt, NodeEntryPtr))
	{
		return;
	}

	/*-------------------------------------------------*/
	/* Update the communication route using the source	*/
	/* address.														*/
	/*																	*/
	/* Set repeat flag for this node if the packet was	*/
	/* repeated to us.											*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((DataPkt->PRDLLDHeader2 & LLDREPEATBIT) && (LLDMSTAFlag == CLEAR))
	{	NodeEntryPtr->RepeatFlag = SET;
	}
	else
	{	NodeEntryPtr->RepeatFlag = CLEAR;
	}


	/*-------------------------------------------------*/
	/* If the packet is a bridge packet, go to bridge	*/
	/* state for this node.										*/
	/*																	*/
	/* We have to check if we are a bridge so that we	*/
	/* only update the route and the flag if the 		*/
	/* packet was from the station through a different	*/
	/* bridge.  Otherwise, if the route is updated all	*/
	/* the time, the bridge success flag will be set	*/
	/* and we won't delay the retry for wakeup.  This	*/
	/* will also prevent the bridge from sending			*/
	/* actual BPKT packets.										*/
	/*-------------------------------------------------*/

	if (DataPkt->PRDLLDHeader2 & LLDBRIDGEBIT)
	{	
		BSourceAddr = ((struct BPRspData *)DataPkt)->BPRDPreSrcAddr;
		SourceAddr = ((struct BPRspData *)DataPkt)->BPRDSrcAddr;
		if ((LLDBridgeFlag == 0) || ((LLDBridgeFlag) && (memcmp (BSourceAddr, SourceAddr, ADDRESSLENGTH))))
		{	
			NodeEntryPtr->BridgeStartTime = LLSGetCurrentTime ();
			NodeEntryPtr->BridgeState = BS_BRIDGED;
			NodeEntryPtr->BridgeSuccess = SET;

			CopyMemory (NodeEntryPtr->Route, BSourceAddr, ADDRESSLENGTH);
		}
		else
		{	
			NodeEntryPtr->BridgeState = BS_DIRECT;
			NodeEntryPtr->BridgeSuccess = CLEAR;
		}
	}
	else
	{	NodeEntryPtr->BridgeState = BS_DIRECT;
		NodeEntryPtr->BridgeSuccess = CLEAR;
	}

	
	/*-------------------------------------------------*/
	/* Get the length of the packet minus the 4 byte	*/
	/* LLD header.  Set the hardware header length.		*/
	/*																	*/
	/*-------------------------------------------------*/

	Length = (DataPkt->PRDLength [0] << 8) + DataPkt->PRDLength [1] - 4;

	/*-----------------------------------------------*/
	/* DataPtr is pointing to the look ahead buffer. */
	/*-----------------------------------------------*/
	HardwareHeader = 12;
	DataPtr = DataPkt->PRDDestAddr;


	if (DataPkt->PRDLLDHeader2 & LLDBRIDGEBIT)
	{	
		OutChar ('z', GREEN);
		OutChar ('0', GREEN);


		/*----------------------------------------------*/
		/* Filter out bridge discovery packets not for	*/
		/* us if we are not a bridge.							*/
		/*																*/
		/*----------------------------------------------*/

		if (((struct BPRspData *)DataPkt)->BPRDPreDestAddr [0] & 0x01)
		{
         if (!(((struct BPRspData *)DataPkt)->BPRDDestAddr [0] & 0x01))
         {
             if (LLDBridgeFlag == 0)
             { 
                 if (memcmp (((struct BPRspData *)DataPkt)->BPRDDestAddr, LLDNodeAddress, ADDRESSLENGTH))
                 { 
                     return;
                 }       
             }
         }
		}

		/*----------------------------------------------*/
		/* Adjust pointers and counters to account for	*/
		/* the 12 byte bridge header.							*/
		/*																*/
		/*----------------------------------------------*/

		DataPtr += 12;
		Length -= 12;
		HardwareHeader = 24;
		LookAheadLength -= 12;
	}

#ifdef WBRP
	if ( DataPkt->PRDLLDHeader1 & LLDWBRPONBIT )
	{
		/*----------------------------------------------*/
		/* Adjust pointers and counters to account for	*/
		/* the 14 byte Repeating Header.						*/
		/*	Repeating Dest (6) + 								*/
		/* Repeating Src  (6) +									*/
		/* Hops Left      (1) +									*/
		/* Reserved       (1) 									*/
		/*----------------------------------------------*/
		DataPtr += 14;
		Length  -= 14;
		HardwareHeader = 26;  /* 12 Pkt.header+14 */
		LookAheadLength -= 14;
	}
#endif
	RcvPktLength = Length;

	/*-------------------------------------------------*/
	/* Now decide whether to receive fragments or		*/
	/* regular packets.											*/
	/*																	*/
	/*-------------------------------------------------*/

	if (DataPkt->PRDLLDHeader4 & 0xF0)
	{	
		LLDRxFragments (DataPtr, Length, DataPkt, NodeEntryPtr);
	}
	else
	{	if (IsProximPacket (DataPtr))
		{	
			ProcessProximPkt(DataPtr,Length,DataPkt);
		}
		else					
		{	FramesRx++;
			LLSReceiveLookAhead (DataPtr, Length, SUCCESS, DataPkt->PRDAtoD);
		}
	}

}




/***************************************************************************/
/*																									*/
/*	LLDReceive (*PktDesc, Offset, Length)												*/
/*																									*/
/*	INPUT: 	PktDesc	-	Pointer to RX packet descriptor to put the data		*/
/*				Offset	-	Offset from start of packet to start reading			*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*																									*/
/*	OUTPUT:	Return	-	Number of bytes that were read in.						*/
/*																									*/
/*	DESCRIPTION:  This procedure is called by the upper layer after the		*/
/*		look ahead to read in the rest of the packet.								*/
/*																									*/
/***************************************************************************/

unsigned_16 LLDReceive (struct LLDRxPacketDescriptor FAR *PktDesc, 
								unsigned_16 Offset, 
								unsigned_16 Length )
{	
	unsigned_8	 FAR *BuffPtr;
	unsigned_16			BuffLength;
	unsigned_16			FragmentNum;
	unsigned_8	 	  *LookAheadPtr;
	unsigned_16			BytesToRead;
	unsigned_16			TotalRemaining;
	unsigned_16			TotalRead;
	unsigned_8			OddFlag;
	unsigned_8			OddBytes [2];
	unsigned_16			NumFromLookAhead;


	LLDRxLookAhead++;
	BuffPtr    = PktDesc->LLDRxFragList[0].FSDataPtr;
	BuffLength = PktDesc->LLDRxFragList[0].FSDataLen; 
	if (PktDesc->LLDRxFragCount == 1 && BuffLength < Length)
		OutChar ('c', GREEN);
	if (HardwareHeader)
	{	OutChar ('b', GREEN);
		OutChar ('2', GREEN);
	}

	TotalRemaining = Length;
	TotalRead = 0;
	FragmentNum = 0;

	/*-------------------------------------------------*/
	/* Set LookAheadLength to the number of bytes to	*/
	/* move from the look ahead buffer.						*/
	/*																	*/
	/*-------------------------------------------------*/

	NumFromLookAhead = LookAheadLength - Offset;
	LookAheadPtr = LLDLookAheadBufPtr + HardwareHeader + Offset;

	if (NumFromLookAhead > Length)
		NumFromLookAhead = Length;

	/*-------------------------------------------------*/
	/* Read in the look ahead bytes.							*/
	/*																	*/
	/*-------------------------------------------------*/
	while (NumFromLookAhead && FragmentNum < PktDesc->LLDRxFragCount)
	{	
		if (BuffLength >= NumFromLookAhead)
		{	
			/*--------------------------------------------*/
			/* The buffer passed from the user is BIGGER  */
			/* than the look ahead size, so copy number of*/
			/* bytes in the LookAheadLength variable only.*/
			/*--------------------------------------------*/

			/* Sanity check for the copy length. */
			if (NumFromLookAhead > MAX_PROX_PKT_SIZE)
			{
				LLDBadCopyLen++;
				OutChar ('B', RED);
				OutChar ('a', RED);
				OutChar ('d', RED);
				OutChar ('C', RED);
				OutChar ('p', RED);
				OutChar ('y', RED);
				OutChar ('L', RED);
				OutChar ('e', RED);
				OutChar ('n', RED);
				OutHex ((unsigned_8)(LookAheadLength >> 8), RED);
				OutHex ((unsigned_8)(LookAheadLength), RED);
#if INT_DEBUG
		__asm	int 3
#endif
				return (0);
			}

#ifdef ODI
			fmemcpy (BuffPtr, (unsigned_8 FAR *)LookAheadPtr, NumFromLookAhead);
#else
			CopyMemory (BuffPtr, LookAheadPtr, NumFromLookAhead);
#endif
			BuffPtr 			+= NumFromLookAhead;	/* Advance the buffer ptr 		 */
			BuffLength		-= NumFromLookAhead;	/* Decrement the Count	  		 */
			TotalRemaining -= NumFromLookAhead;	/* Decrement # of bytes to copy*/
			TotalRead 		+= NumFromLookAhead;	/* Increment total read count  */
			NumFromLookAhead = 0;						/* No more LookAhead count		 */
		}
		else
		{	
			/*--------------------------------------------*/
			/* The buffer passed from the user is SMALLER */
			/* than the look ahead size, so copy number of*/
			/* bytes in the BuffLength variable only.		 */
			/* NOTE:													 */
			/*		This situation will cause to loop again.*/
			/*--------------------------------------------*/

			/* Sanity check for the copy length. */
			if (BuffLength > MAX_PROX_PKT_SIZE)
			{
				LLDBadCopyLen++;
				OutChar ('B', RED);
				OutChar ('a', RED);
				OutChar ('d', RED);
				OutChar ('C', RED);
				OutChar ('p', RED);
				OutChar ('y', RED);
				OutChar ('L', RED);
				OutChar ('e', RED);
				OutChar ('n', RED);
				OutHex ((unsigned_8)(BuffLength >> 8), RED);
				OutHex ((unsigned_8)(BuffLength), RED);
#if INT_DEBUG
		__asm	int 3
#endif
				return (0);
			}

#ifdef ODI
			fmemcpy (BuffPtr, (unsigned_8 FAR *)LookAheadPtr, BuffLength);
#else
			CopyMemory (BuffPtr, LookAheadPtr, BuffLength);
#endif

			TotalRemaining	-= BuffLength;			/* Decrement # of bytes to read*/
			TotalRead 		+= BuffLength;			/* Increment total read count  */
			LookAheadPtr	+= BuffLength;			/* Advance the LookAhead Bufptr*/

			NumFromLookAhead -= BuffLength;			/* Decrement LookAhead count   */

			/*--------------------------------------------*/
			/* Get another Receiving Buffer from the Rx   */
			/* Packet Descriptor.								 */
			/*--------------------------------------------*/

			if ( ++FragmentNum < PktDesc->LLDRxFragCount)
			{
				
				BuffPtr    = PktDesc->LLDRxFragList[FragmentNum].FSDataPtr;
				BuffLength = PktDesc->LLDRxFragList[FragmentNum].FSDataLen; 
			}
		}
	}

	/*-------------------------------------------------*/
	/* Read in the rest of the bytes from the card.		*/
	/* Special care must be taken on odd byte bounds.	*/
	/*																	*/
	/*-------------------------------------------------*/

	OddFlag = 0;
	while (TotalRemaining)
	{	
		if (BuffLength == 0)
		{	
			
			if (++FragmentNum >= PktDesc->LLDRxFragCount)
			{
				if (OddFlag)
				{	LLDLookAheadBufPtr[HardwareHeader + LookAheadLength] = OddBytes [1];
					LookAheadLength++;
					OddFlag = CLEAR;
				}
				return(TotalRead);
			}

			BuffPtr    = PktDesc->LLDRxFragList[FragmentNum].FSDataPtr;
			BuffLength = PktDesc->LLDRxFragList[FragmentNum].FSDataLen; 

			if (OddFlag)
			{	*BuffPtr++ = OddBytes [1];
				LLDLookAheadBufPtr [HardwareHeader + LookAheadLength] = OddBytes [1];
				LookAheadLength++;
				BuffLength--;
				TotalRead++;
				TotalRemaining--;
				OddFlag = CLEAR;
			}
		}
		BytesToRead = TotalRemaining;
		if (BytesToRead > BuffLength)
			BytesToRead = BuffLength;

		/*-------------------------------------------*/
		/* All reads from the serial card are 8-bit	*/
		/* reads.  We don't need to worry about odd	*/
		/* packet lengths.									*/
		/*-------------------------------------------*/
#ifndef SERIAL
		if (BytesToRead & 0x01)
		{	OddFlag = SET;
			BytesToRead--;
		}
#endif

		GetRxData (BuffPtr, BytesToRead, LLDTransferMode);

		/*----------------------------------------------*/
		/*	Copy this to the end of the LookAhead buffer	*/
		/*	and increment the LookAheadSize.					*/
		/*----------------------------------------------*/

		CopyMemory (LLDLookAheadBufPtr + HardwareHeader + LookAheadLength, BuffPtr, BytesToRead);
		LookAheadLength += BytesToRead;

		TotalRemaining -= BytesToRead;
		TotalRead += BytesToRead;
		BuffPtr += BytesToRead;
		BuffLength -= BytesToRead;

		if (OddFlag)
		{	
			GetRxData (OddBytes, 2, LLDTransferMode);

			*BuffPtr++ = OddBytes [0];
			LLDLookAheadBufPtr [HardwareHeader + LookAheadLength] = OddBytes [0];
			LookAheadLength++;
			BuffLength--;
			TotalRead++;
			TotalRemaining--;

			if ((TotalRemaining) &&	(BuffLength))
			{	*BuffPtr++ = OddBytes [1];
				LLDLookAheadBufPtr [HardwareHeader + LookAheadLength] = OddBytes [1];
				LookAheadLength++;
				BuffLength--;
				TotalRead++;
				TotalRemaining--;
				OddFlag = CLEAR;
			}	
		}
		
	}
	/*---------------------------------------------------------------------*/
	/* The following case is provided to handle the protocol requesting	  */
	/* less data then there are on the card. We then need to save the last */
	/* byte for the next protocol.													  */
	/*---------------------------------------------------------------------*/

	if (OddFlag && RcvPktLength - TotalRead)
	{	LLDLookAheadBufPtr[HardwareHeader + LookAheadLength] = OddBytes[1];
		LookAheadLength++;
		OddFlag= CLEAR;
	}

	return (TotalRead);
}





/***************************************************************************/
/*																									*/
/*	LLDRawReceive (*PktDesc, Offset, Length)											*/
/*																									*/
/*	INPUT: 	PktDesc	-	Pointer to RX packet descriptor to put the data		*/
/*				Offset	-	Offset from start of packet to start reading			*/
/*				Length	-	Number of bytes to read into the buffer				*/
/*																									*/
/*	OUTPUT:	Return	-	Number of bytes that were read in.						*/
/*																									*/
/*	DESCRIPTION:  This procedure is called by the upper layer after the		*/
/*		raw look ahead to read in the rest of the packet.  It does 				*/
/*		everything that LLDReceive does except that it doesn't skip the		*/
/*		hardware header.																		*/
/*																									*/
/***************************************************************************/

unsigned_16 LLDRawReceive (struct LLDRxPacketDescriptor FAR *PktDesc, 
									unsigned_16 Offset, 
									unsigned_16 Length )

{
	HardwareHeader = 0;
	LookAheadLength = RawLookAheadLength;
	return (LLDReceive (PktDesc, Offset, Length));
}



/***************************************************************************/
/*																									*/
/*	LLDDuplicateDetection (LookAheadBuf, CurrEntry)									*/
/*																									*/
/*	INPUT: 	LookAheadBuf	-	Pointer to the look ahead buffer.				*/
/*				CurrEntry    	-	Pointer to node's entry in the node table.	*/
/*																									*/
/*				rl2TableSem already taken													*/
/*																									*/
/*	OUTPUT:	On Return	-	SUCCESS = Not a duplicate								*/
/*								-	FAILURE = Duplicate or bad fragment					*/
/*																									*/
/*	DESCRIPTION:  This procedure determines whether or not the 					*/
/*    non-fragmented packet just received is a duplicate packet.  It does	*/
/*    so by checking the sequence number of the new packet and comparing	*/
/*    it with the last sequence number received from this node.  If the		*/
/*		two sequence numbers matches, it is considered a duplicate packet.	*/
/*																									*/
/*		This procedure also checks for fragmentation.  If the packet is 		*/
/*		fragmented, it makes sure that the fragments are not only received	*/
/*		in order, but also makes sure none are missing.  When an incomplete	*/
/*		packet fragment is detected, it frees the buffer for receiving the	*/
/*		fragments (if one exists) and returns an error so that the packet		*/
/*		gets dropped.																			*/
/*																									*/
/***************************************************************************/

unsigned_16 LLDDuplicateDetection (struct PRspData *LookAheadBuf, 
											  struct NodeEntry *CurrEntry) 
{
	unsigned_8			FragNum;
	unsigned_8			TotalFrag;
	unsigned_8			SeqNum;

	/*-------------------------------------------------*/
	/*	Make sure we do not eliminate roam notify 		*/
	/*	messages.													*/
	/*-------------------------------------------------*/
	if (CheckRoamNotify (LookAheadBuf))
		return (SUCCESS);

	/*-------------------------------------------------*/
	/* Total Frag is the total number of fragments if	*/
	/* a packet has been fragmented.  For 					*/
	/* non-fragmented packets, this is always zero.		*/
	/* The SeqNum is the DataSequenceNum.					*/
	/*-------------------------------------------------*/
	SeqNum = LookAheadBuf->PRDLLDHeader3;
	TotalFrag = (unsigned_8)(LookAheadBuf->PRDLLDHeader4 & TOTAL_FRAGS_MASK);

	/*-------------------------------------------------*/
	/* Is it a fragmented packet?								*/
	/*																	*/
	/*	Check the fragment numbers to see if it is the	*/
	/* fragment we expected.  If the fragments are out	*/
	/* of order or some are missing, we drop the			*/
	/* the packet and free the RX buffer.					*/
	/*																	*/
	/* It is possible to get a new fragmented packet	*/
	/* that has the same sequence number as the 			*/
	/* previous fragmented packet.  This happens if		*/
	/* the previous packet we received from a node was	*/
	/* a fragmented packet that had a CTS or ACK error.*/
	/*-------------------------------------------------*/
	if (TotalFrag)
	{	
		
		/*-------------------------------------------------*/
		/* Is the remote station sending the same				*/
		/* fragmented packet again?  The LastSeqNum does	*/
		/* not get updated until we have successfully		*/
		/* received all of the fragments in	the packet.		*/
		/*-------------------------------------------------*/
		if (SeqNum == CurrEntry->LastSeqNum)
		{
			CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
			return (FAILURE);
		}

		/*-------------------------------------------------*/
		/* FragNum is the current fragment number.  			*/
		/* Fragment numbering starts at 0.						*/
		/*-------------------------------------------------*/
		FragNum = (unsigned_8)(LookAheadBuf->PRDLLDHeader4 & FRAG_NUM_MASK);

		/*-------------------------------------------------*/
		/* Is it a missing or out of order fragment OR a	*/
		/* new sequence number in the middle of receiving	*/
		/* a fragmented packet?										*/
		/*-------------------------------------------------*/
		if ( (FragNum != 0) && (((unsigned_8)(CurrEntry->LastRxFragNum + 1) != FragNum)
		    || (SeqNum != CurrEntry->LastFragSeqNum)) ) 
		{

			/*-----------------------------*/
			/* Free any existing Rx Buffer */
			/*-----------------------------*/
			if (CurrEntry->RxBuf != 0)
			{ 
				CurrEntry->RxBuf->NextBuffer = FreeRXBuffer;
				FreeRXBuffer = CurrEntry->RxBuf;
				CurrEntry->CurrentRxBufPtr = 0;
				CurrEntry->RxPktLength = 0;
				CurrEntry->RxBuf = 0;
			}

			/*----------------------------------------*/
			/* Set LastRxFragNum to the					*/
			/* WAIT_FOR_FRAG_STATE so we can start 	*/
			/* looking for a new fragmented packet.	*/
			/* Discard the out of order fragment.		*/
			/*----------------------------------------*/
			CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
			return (FAILURE);
		}

		/*----------------------------------------*/
		/* This is a new fragmented packet or a	*/
		/* fragment that was received in order.	*/
		/*----------------------------------------*/
		CurrEntry->LastFragSeqNum = SeqNum;
		CurrEntry->LastRxFragNum = FragNum;
		OutChar ('F', BRIGHT_CYAN);
		OutChar ((unsigned_8)(FragNum + '0'), BRIGHT_CYAN);

	}

	/*----------------------------------------------*/
	/* This packet is not a fragmented packet or a	*/
	/* duplicate packet.										*/
	/*----------------------------------------------*/
	else if (SeqNum != CurrEntry->LastSeqNum)
	{
		CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
		CurrEntry->LastSeqNum = SeqNum;

		/*-------------------------------------------------*/
		/* If we were previously receiving fragments and	*/
		/* we never received the last fragments in the 		*/
		/* packets, return the Rx buffer to the free list.	*/
		/*-------------------------------------------------*/
		if (CurrEntry->RxBuf != 0)
		{ 
			CurrEntry->RxBuf->NextBuffer = FreeRXBuffer;
			FreeRXBuffer = CurrEntry->RxBuf;
			CurrEntry->CurrentRxBufPtr = 0;
			CurrEntry->RxPktLength = 0;
			CurrEntry->RxBuf = 0;
		}
	}

	/*-------------------------------------------------*/
	/* The last two packets received from a node are	*/
	/* identical and the packet is not a fragmented		*/
	/* packet.  Therefore, it is a duplicate packet		*/
	/* that needs dropping.										*/
	/*-------------------------------------------------*/
	else
	{	
		FramesRxDuplicate++;
		CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
		OutChar ('D',YELLOW);
		OutChar ('U',YELLOW);
		OutChar ('P',YELLOW);
		return (FAILURE);
	}

	return (SUCCESS);
}

	


/***************************************************************************/
/*																									*/
/*	LLDRxFragments (Data, Length, Packet, CurrEntry)								*/
/*																									*/
/*	INPUT: 	Data				-	Pointer to start of the data (after header)	*/
/*				Length			-	Length of the packet									*/
/*				Packet			-	Pointer to the receive buffer.					*/
/*				CurrEntry    	-	Pointer to node's entry in the node table.	*/
/*																									*/
/*	OUTPUT:	On Return	-	SUCCESS = Able to receive the packet				*/
/*								-	FAILURE = No RX buffers available					*/
/*																									*/
/*	DESCRIPTION:  This procedure allocates a receive buffer for storing the	*/
/*		fragments when the first fragment in a packet is received.  It then 	*/
/*    reads the data into the buffer and updates the current buffer 			*/
/*    pointer in the NodeTable.  Care is taken to strip out bridge and		*/
/*    packet headers, leaving them in only on the first fragment.  When an	*/
/*    entire packet is built (all fragments received), it is passed to the	*/
/*    upper	layer by calling LLSReceiveLookAhead.									*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDRxFragments (unsigned_8 Data [], unsigned_16 Length, 
									struct PRspData *Packet, struct NodeEntry	*CurrEntry)
{
	unsigned_16			PackLength;
	unsigned_8			FragNum;

	FragNum = (unsigned_8)(Packet->PRDLLDHeader4 & FRAG_NUM_MASK);

	/*-------------------------------------------------*/
	/* If the fragment number is not 0, then we are		*/
	/* currently receiving fragments.  If not, then we	*/
	/* need to allocate a RX buffer, if available.		*/
	/*-------------------------------------------------*/

	if (FragNum == 0)
	{
		/*-------------------------------------------------*/
		/* Make sure there isn't 't already an Rx buffer.	*/
		/* If there is an Rx buffer, go ahead and use it.	*/
		/*-------------------------------------------------*/
		if (CurrEntry->RxBuf != 0)
		{
			CurrEntry->CurrentRxBufPtr = CurrEntry->RxBuf->RxData;
			CurrEntry->RxPktLength = 0;
		}

		/*----------------------------------------------*/
		/* Try to allocate a free Rx buffer.  If one		*/
		/* isn't available, then we need to drop the		*/
		/* fragment.												*/
		/*----------------------------------------------*/
		else if (FreeRXBuffer)
		{	CurrEntry->RxBuf = FreeRXBuffer;
			CurrEntry->CurrentRxBufPtr = FreeRXBuffer->RxData;
			CurrEntry->RxPktLength = 0;
			FreeRXBuffer = FreeRXBuffer->NextBuffer;
		}
		/*----------------------------------------------*/
		/* Check if any of the RXBuffers timed out.		*/
		/* If not, then we need to drop the	fragment.	*/
		/*       													*/
		/*----------------------------------------------*/
		else if (CheckRxBuffTimeOut())
		{	CurrEntry->RxBuf = FreeRXBuffer;
			CurrEntry->CurrentRxBufPtr = FreeRXBuffer->RxData;
			CurrEntry->RxPktLength = 0;
			FreeRXBuffer = FreeRXBuffer->NextBuffer;
		}
		else
		{
			CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
			return (FAILURE);
		}

		/*-------------------------------------------------*/
		/* This is the first fragment, copy the source		*/
		/* and destination addresses along with the rest	*/
		/*	of the data from the LookAheadBuf into the Rx	*/
		/* buffer.														*/
		/*-------------------------------------------------*/
		CopyMemory (CurrEntry->CurrentRxBufPtr, Data, LookAheadLength);
		CurrEntry->CurrentRxBufPtr += LookAheadLength;
	}

	/*-------------------------------------------------*/
	/* This is not the first fragment, so skip the		*/
	/* source and destination addresses, and copy		*/
	/* the data from the LookAheadBuf into the Rx		*/
	/* buffer.														*/
	/*-------------------------------------------------*/
	else
	{
		/*----------------------------------------------*/
		/* Make sure there is an Rx buffer.  If there	*/
		/* isn't an Rx buffer, drop the fragment and		*/
		/* restart the fragmentation state.					*/
		/*----------------------------------------------*/
		if (CurrEntry->RxBuf == 0)
		{
			CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
			return (FAILURE);
		}

		CopyMemory (CurrEntry->CurrentRxBufPtr, &Data [12], LookAheadLength - 12);
		CurrEntry->CurrentRxBufPtr += LookAheadLength - 12;
	}


	/*-------------------------------------------------*/
	/* Read in any remaining data.							*/
	/*-------------------------------------------------*/

	if (Length > LookAheadLength)
	{	
		GetRxData (CurrEntry->CurrentRxBufPtr, (unsigned_16)(Length - LookAheadLength), LLDTransferMode);
		CurrEntry->CurrentRxBufPtr += Length - LookAheadLength;
	}


	/*-------------------------------------------------*/
	/* Get the length of the packet, but subtract the	*/
	/* ethernet address and the 4 byte LLD header.		*/
	/* Subtract the bridge header if it exists.			*/
	/*-------------------------------------------------*/

	PackLength = ((Packet->PRDLength [0] << 8) + Packet->PRDLength [1]) - 16;
	if (Packet->PRDLLDHeader2 & LLDBRIDGEBIT)
	{	HardwareHeader -= 12;
		PackLength -=12;
	}
	CurrEntry->RxPktLength += PackLength;
	CurrEntry->RxBuf->Time = LLSGetCurrentTime();
	CurrEntry->RxBuf->NodePointer = CurrEntry;


	/*-------------------------------------------------*/
	/* If this is the last fragment, give it to the		*/
	/* upper layer.												*/
	/*-------------------------------------------------*/

	if (((Packet->PRDLLDHeader4 & TOTAL_FRAGS_MASK) >> 4)	== FragNum)
	{
		LLDLookAheadBufPtr = CurrEntry->RxBuf->RxData;
		LookAheadLength = CurrEntry->RxPktLength - 
								(unsigned_16)(Packet->PRDLLDHeader2 & LLDPADBITS_MASK) + 12;
		HardwareHeader -= 12;		

#ifdef AP2_DEBUG
		CurrEntry->LastRxFragBuf = CurrEntry->RxBuf->RxData;
		CurrEntry->RXFragments++;
#endif
		FramesRxFrag++;
		FramesRx++;
		RcvPktLength = LookAheadLength;
		LLSReceiveLookAhead ((unsigned_8 *) LLDLookAheadBufPtr, LookAheadLength, 0, 
									Packet->PRDAtoD);

		/* Free the Rx buffer */
		CurrEntry->RxBuf->NextBuffer = FreeRXBuffer;
		FreeRXBuffer = CurrEntry->RxBuf;
		CurrEntry->RxBuf = 0;
		CurrEntry->CurrentRxBufPtr = 0;
		CurrEntry->RxPktLength = 0;
		CurrEntry->LastSeqNum = Packet->PRDLLDHeader3;
		CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
	}

	return (SUCCESS);
}




/***************************************************************************/
/*																									*/
/*	LLDRepeat (*LookAheadBuf, Length)		 											*/
/*																									*/
/*	INPUT: 	LookAheadBuf	-	Pointer to the look ahead buffer					*/
/*				Length			-	Length of the packet less hardware header		*/
/*																									*/
/*	OUTPUT:	On Return		-	0 if successful										*/
/*																									*/
/*	DESCRIPTION:  This procedure will repeat the packet that it receives.	*/
/*																									*/
/***************************************************************************/

void LLDRepeat (unsigned_8 LookAheadBuf [], unsigned_16 Length)

{	struct PacketizeTx	*PacketPtr;
	struct BPRspData		*BridgePkt;
	unsigned_8				RpDestAddr [ADDRESSLENGTH];
	unsigned_8				RpSrcAddr [ADDRESSLENGTH];
	unsigned_16				PacketSize;

	OutChar ('R', BROWN);
	OutChar ('e', BROWN);
	OutChar ('p', BROWN);
	OutChar ('e', BROWN);
	OutChar ('a', BROWN);
	OutChar ('t', BROWN);


	/*-------------------------------------------------*/
	/* Read in the rest of the data into the look		*/
	/* ahead buffer.  Start the buffer after the			*/
	/* look ahead data bytes.	The amount of data left */
	/* to read is the total packet length minus the		*/
	/* the amount we already read (LookAheadLength+12).*/
	/* The size of our hardware header was subtracted	*/
	/* from LookAheadLength in LLDIsr ().					*/
	/*																	*/
	/*-------------------------------------------------*/
	GetRxData (&LookAheadBuf [LookAheadLength + sizeof (struct PacketizeTx)],
					(unsigned_16) (Length - LookAheadLength), LLDTransferMode);
	LookAheadLength = Length;


	/*-------------------------------------------------*/
	/* Now form the packet header to transmit data.		*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketPtr = (struct PacketizeTx *) LookAheadBuf;
	
	PacketPtr->PTHeader.PHCmd = DATA_COMMANDS_CMD;
	PacketPtr->PTHeader.PHSeqNo = REPEAT_SEQ_NUMBER;
	PacketPtr->PTHeader.PHFnNo = TRANSMIT_DATA_FN;
	PacketPtr->PTHeader.PHRsv = RESERVED;

	PacketPtr->PTControl = TX_POLLFINAL;
	if (LLDTxMode == TX_QFSK)
	{	PacketPtr->PTControl |= TX_QFSK;
	}

	PacketPtr->PTTxPowerAnt = 0x70;
	/* Add the 4-byte LLD header back into the packet length. */
	PacketPtr->PTPktLength [0] = (unsigned_8) ((Length + 4) >> 8);
	PacketPtr->PTPktLength [1] = (unsigned_8) ((Length + 4) & 0x00FF);

	PacketSize = LookAheadLength;
	if (((struct PRspData *)PacketPtr)->PRDLLDHeader2 & LLDBRIDGEBIT)
	{	PacketSize += 12;
	}


	/*-------------------------------------------------*/
	/* Add 12 byte hardware header to the PacketSize	*/
	/* to give to LLDPacketizeSend.							*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketSize += sizeof (struct PacketizeTx);


	/*-------------------------------------------------*/
	/* Check for broadcast packet.  Repeat it, but		*/
	/* don't change the original bridge node address.	*/
	/*																	*/
	/*-------------------------------------------------*/

	BridgePkt = (struct BPRspData *) PacketPtr;

	if (BridgePkt->BPRDPreDestAddr [0] & 0x01)
	{	LLDPacketizeSend ((unsigned_8 *) PacketPtr, PacketSize);
	}


	/*-------------------------------------------------*/
	/* Save the original bridge source and destination	*/
	/* addresses.  Put our bridge address as the source*/
	/* address and send out the packet.  Then restore	*/
	/* the addresses.												*/
	/*																	*/
	/* Need to add the hardware header to the size.		*/
	/*																	*/
	/*-------------------------------------------------*/

	else
	{	CopyMemory (RpDestAddr, BridgePkt->BPRDPreDestAddr, ADDRESSLENGTH);
		CopyMemory (RpSrcAddr, BridgePkt->BPRDPreSrcAddr, ADDRESSLENGTH);

		CopyMemory (BridgePkt->BPRDPreDestAddr, BridgePkt->BPRDDestAddr, ADDRESSLENGTH);
		if ((BridgePkt->BPRDPreDestAddr [0] & 0x01) == 0)
		{	PacketPtr->PTControl |= TX_MACACK;
		}

	 	PacketPtr->PTLLDHeader2 |= LLDBRIDGEBIT;
		CopyMemory (BridgePkt->BPRDPreSrcAddr, LLDNodeAddress, ADDRESSLENGTH);

		LLDPacketizeSend ((unsigned_8 *) PacketPtr, PacketSize);

		CopyMemory (BridgePkt->BPRDPreDestAddr, RpDestAddr, ADDRESSLENGTH);
		CopyMemory (BridgePkt->BPRDPreSrcAddr, RpSrcAddr, ADDRESSLENGTH);
	}
}



/***************************************************************************/
/*																									*/
/*	SendLLDPingResponse (Buffer, Length, SubType)												*/
/*																									*/
/*	INPUT:	Buffer	-	Pointer to the Ping Request packet.						*/
/*				Length	-	Length of the Ping Request packet.						*/
/*				SubType	-	SubType of the Proxim Pkt.									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This procedure will send the ping response packet by		*/
/*		allocating an internal TX Buffer, copying the request data into it	*/
/*		and sending the packet back.														*/
/*																									*/
/***************************************************************************/

void SendLLDPingResponse (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 SubType)

{	struct ProximProtocolHeader	*Header;
	struct TXBStruct					*TxBuffer;

	OutChar ('P', WHITE);
	OutChar ('R', WHITE);
	OutChar ('e', WHITE);
	OutChar ('s', WHITE);
	OutChar ('p', WHITE);

	if (FreeTXBuffer)
	{	TxBuffer = FreeTXBuffer;
		FreeTXBuffer = TxBuffer->NextBuffer;

		CopyMemory (TxBuffer->TxPkt.LLDImmediateData, Buffer, Length);
		TxBuffer->TxPkt.LLDImmediateDataLength = Length;
		TxBuffer->TxPkt.LLDTxDataLength = Length;
		TxBuffer->TxPkt.LLDTxFragmentListPtr = &TxBuffer->TxFragList;
		TxBuffer->TxFragList.LLDTxFragmentCount = 0;

		Header = (struct ProximProtocolHeader *)TxBuffer->TxPkt.LLDImmediateData;
		CopyMemory (Header->DestAddr, Header->SourceAddr, ADDRESSLENGTH);
		CopyMemory (Header->SourceAddr, LLDNodeAddress, ADDRESSLENGTH);

		Header->SubType[0] = (unsigned_8)((SubType >> 8) & 0xFF);
		Header->SubType[1] = (unsigned_8)(SubType & 0xFF);
	
		LLDSend (&TxBuffer->TxPkt);
	}
}
		


#ifdef PROTOCOL

/***************************************************************************/
/*																									*/
/*	HandleProtReceive (*DataPkt)															*/
/*																									*/
/*	INPUT: 	DataPkt	-	Pointer to the data packet.								*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure is called by LLDISR when a protocol			*/
/*		packet arrives.																		*/
/*																									*/
/***************************************************************************/

void HandleProtReceive (struct PRspProt *DataPkt)

{
	unsigned_16			Length;
	unsigned_8			*DataPtr;
	unsigned_32			TimeStamp;
	unsigned_8			i;

	
	/*-------------------------------------------------*/
	/* Get the length of the packet.							*/
	/* Set the hardware header length.						*/
	/*																	*/
	/*-------------------------------------------------*/

	Length = (DataPkt->PRPLength [0] << 8) + DataPkt->PRPLength [1];
	HardwareHeader = 10;
	DataPtr = &DataPkt->PRPData;

	TimeStamp = 0;
	for (i = 0; i < 4; i++)
	{	TimeStamp = (unsigned_32)((TimeStamp << 8) + (unsigned_32)(DataPkt->PRPTimeStamp [i]));
	}

	LLSReceiveProt (DataPtr, Length, TimeStamp);
}

#endif

/***************************************************************************/
/*																									*/
/*	CheckRoamNotify (DataPkt)																*/
/*																									*/
/*	INPUT: 	DataPkt	-	Pointer to the data packet.								*/
/*																									*/
/*	OUTPUT:	Returns	-	0 if not a roam notify packet								*/
/*							-	1 if it is a roam notify packet							*/
/*																									*/
/*	DESCRIPTION:  This procedure is called to check if the packet received	*/
/*		is a roam notify message.															*/
/*																									*/
/***************************************************************************/

int CheckRoamNotify (struct PRspData *DataPkt)

{	struct RoamNotifyRx	*Packet;
	int						roamPkt = FALSE;

	Packet = (struct RoamNotifyRx *)DataPkt->PRDDestAddr;

	/*-------------------------------------------------*/
	/*	Make sure we do not eliminate roam notify 		*/
	/*	messages.  Newer versions of the driver set the	*/
	/* roam notify bit in the LLD header.					*/
	/*-------------------------------------------------*/
	if (DataPkt->PRDLLDHeader2 & LLDROAMNOTIFYBIT)
		roamPkt = TRUE;

	else if (((((Packet->RNProtocolID [0] == 0x81) && (Packet->RNProtocolID [1] == 0x37))
				&& (Packet->RNIPXIdent == 0xFFFF)) &&
			 (Packet->RNIPXSktID [0] == 0x86)) && (Packet->RNIPXSktID [1] == 0x4B))
	{
		roamPkt = TRUE;
	}

	if (roamPkt)
	{
		return (1);
	}
	else
	{	return (0);
	}
}


/***************************************************************************/
/*																									*/
/*	CheckRxBuffTimeOut ()		 															*/
/*																									*/
/*	INPUT: 	None.																				*/
/*																									*/
/*	OUTPUT:	Returns	-	0 if no timeout												*/
/*							-	Other if there is a timing out buffer					*/
/*																									*/
/*	DESCRIPTION:  This procedure is called to check if any of the RXBuffers	*/
/*		timed out.																				*/
/*																									*/
/***************************************************************************/

struct RXBStruct *CheckRxBuffTimeOut (void)

{	
	unsigned_8			i;
	struct RXBStruct	*TmpBuffer;
	struct NodeEntry	*CurrEntry;
	
	
	TmpBuffer = RXBuffersPtr;

	/*----------------------------------------------------*/
	/*	Check each RXBuffer if maybe it is waiting for		*/
	/* data for too long (8 min). Release it in this case	*/
	/*----------------------------------------------------*/

	for (i = 0; i < TOTALRXBUF; i++)
	{	if ((LLSGetCurrentTime() - TmpBuffer[i].Time) > RXBUFFTIMEOUT)
		{
		 	CurrEntry = TmpBuffer[i].NodePointer;
			if (CurrEntry->RxBuf != 0)
			{
				CurrEntry->RxBuf->NextBuffer = FreeRXBuffer;
				FreeRXBuffer = CurrEntry->RxBuf;
				CurrEntry->RxBuf = 0;
				CurrEntry->CurrentRxBufPtr = 0;
				CurrEntry->RxPktLength = 0;
				CurrEntry->LastRxFragNum = WAIT_FOR_FRAG_STATE;
			}
			else
			{
				/*----------------------------------------------------*/
				/*	Buffer Time Out error in the case if there is		*/
				/* RxBuf associated with the CurrEntry						*/
				/*----------------------------------------------------*/

				OutChar ('B', WHITE);
				OutChar ('T', WHITE);
				OutChar ('E', WHITE);
				OutChar ('R', WHITE);
				OutChar ('R', WHITE);
			}
		}
	}

	return (FreeRXBuffer);
}

/***************************************************************************/
/*																									*/
/*	ProcessProximPkt (DataPtr, Length, DataPkt)										*/
/*																									*/
/*	INPUT:	DataPtr 	- Pointer to the Request packet.								*/
/*				Length	- Length of the Request packet.								*/
/*				DataPkt	- Pointer to the Packetize Header of Request Packet.	*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This procedure will prepare the response packet 			*/
/*		and sending the packet back.														*/
/*																									*/
/***************************************************************************/

void ProcessProximPkt(unsigned_8 *dataPtr, unsigned_16 length, struct PRspData *dataPkt)
{
	unsigned_8		*data;
	unsigned_16		i;
	unsigned_32		temp;
	struct ProximProtocolHeader	*header;

	header = (struct ProximProtocolHeader *)dataPtr;
	if (!(header->PacketType == 4 && *(unsigned_16 *)header->SubType == 0x0100))
		if (length > LookAheadLength)
		{	GetRxData ((unsigned_8 *)(dataPtr + LookAheadLength), 
						(unsigned_16) (length - LookAheadLength), LLDTransferMode);
			LookAheadLength = length;
		}
	

	data = dataPtr + sizeof (struct ProximProtocolHeader);
	switch(header->PacketType)
	{
		case	4:

			if (*(unsigned_16 *)header->SubType == 0) 
			{
				SendLLDPingResponse (dataPtr, length, 1);
			}
			else if (*(unsigned_16 *)header->SubType == 0x0100)
			{	RcvPktLength = length;
				LLSPingReceiveLookAhead (dataPtr, length, SUCCESS, dataPkt->PRDAtoD);
			}
			break;

		case	7:
			switch(*(unsigned_16 *)header->SubType)
			{
				case 0x0000:
					SetMemory(data, 0, ROMVERLENGTH+DRIVERVERLENGTH);
					CopyMemory(data, LLDROMVersion, ROMVERLENGTH);
#ifdef NDIS_MINIPORT_DRIVER
					CopyMemory(data + ROMVERLENGTH, DRIVER_VERSION_STRING, 
							sizeof (DRIVER_VERSION_STRING));
					SendManagementDataPkt(ROMVERLENGTH + sizeof(DRIVER_VERSION_STRING), 0x8000, dataPtr);
#else
					CopyMemory(data + ROMVERLENGTH, DriverVersionString, 
							sizeof (DriverVersionString));
					SendManagementDataPkt(ROMVERLENGTH + sizeof(DriverVersionString), 0x8000, dataPtr);
#endif
				break;

				case 0x0100:
					*data = LLDDomain;
					*(data + 1) = LLDPeerToPeerFlag;
					if (LLDOEM)
						*(data + 2) = CARD_TYPE_OEM;
					else	if (LLDMicroISA)
						*(data + 2) = 2;
					else	if (LLDPCMCIA)
					{
						if (LLDOnePieceFlag)
							*(data + 2) = 3;
						else
							*(data + 2) = 4;
					}
					else	
						*(data + 2) = CARD_TYPE_ISA;
					SendManagementDataPkt(3, 0x8001, dataPtr);
				break;

				case 0x0200:
					/* LLDRoamingFlag is set to 1 when roaming is disabled */
					if (LLDRoamingFlag)
					{
						*data = 0;
						SendManagementDataPkt(1, 0x8002, dataPtr);
					}
					else
					{	*data = 1;
						data[1] = LLDRoamConfiguration;
						SendManagementDataPkt(2 * sizeof(unsigned_8), 0x8002, dataPtr);
					}
				break;

				case 0x0300:
					temp = (unsigned_32) (LLDInactivityTimeOut * 5 + LLDTicksToSniff / TPS);
					ConvertLongIntoBigEndian (data, temp) ;
					SendManagementDataPkt(sizeof(unsigned_32), 0x8003, dataPtr);
				break;

				case 0x0400:
					SetMemory(data, 0, NAMELENGTH + 1 + ADDRESSLENGTH);
					CopyMemory(data, LLDMSTASyncName, sizeof (LLDMSTASyncName));
					CopyMemory((data + sizeof (LLDMSTASyncName)), LLDMSTAAddr, ADDRESSLENGTH);
					SendManagementDataPkt(NAMELENGTH+ADDRESSLENGTH+1, 0x8004, dataPtr);
				break;

				case 0x0500:
					i = SetupMasterHistory(RoamHistory, NextMaster, data);
					SendManagementDataPkt(i, 0x8005, dataPtr);
				break;

				case 0x0600:
					ConvertLongIntoBigEndian(data, (unsigned_32)(LLSGetCurrentTimeLONG() - ResetStatisticsTime));
					CopyMemory (data + sizeof (unsigned_32), StationName, STATION_NAME_SIZE);
					SendManagementDataPkt(STATION_NAME_SIZE + sizeof(unsigned_32), 0x8006, dataPtr);
				break;

				case 0x0700:
					CopyMemory(data, NotesBuffer, NOTES_BUFFER_SIZE);
					SendManagementDataPkt(NOTES_BUFFER_SIZE, 0x8007, dataPtr);
				break;

				case 0x0800:
					ConvertLongIntoBigEndian(data, TotalTxPackets);
					ConvertLongIntoBigEndian(data + sizeof(unsigned_32), FramesXmit);
					ConvertLongIntoBigEndian(data + 2 * sizeof(unsigned_32), FramesCTSError);
					ConvertLongIntoBigEndian(data + 3 * sizeof(unsigned_32), FramesACKError);
					ConvertLongIntoBigEndian(data + 4 * sizeof(unsigned_32), FramesXmitBFSK);
					ConvertLongIntoBigEndian(data + 5 * sizeof(unsigned_32), FramesXmitQFSK);
					ConvertLongIntoBigEndian(data + 6 * sizeof(unsigned_32), FramesXmitFrag);
					SendManagementDataPkt(7 * sizeof(unsigned_32), 0x8008, dataPtr);
				break;

				case 0x0900:
					ConvertLongIntoBigEndian(data, FramesRx);
					ConvertLongIntoBigEndian(data + sizeof(unsigned_32), FramesRxDuplicate);
					ConvertLongIntoBigEndian(data + 2 * sizeof(unsigned_32), FramesRxFrag);
					SendManagementDataPkt(3 * sizeof(unsigned_32), 0x8009, dataPtr);
				break;

				case 0x0A00:
					ConvertLongIntoBigEndian(data, LLDErrResetCnt);
#ifdef NDIS_MINIPORT_DRIVER
					ConvertLongIntoBigEndian(data + sizeof(unsigned_32), LLSAdapter->ConfToolResetCnt);
#endif
					SendManagementDataPkt(2 * sizeof(unsigned_32), 0x800A, dataPtr);
				break;

				case 0x0B00:
					TotalTxPackets		= 0;
					FramesXmit 	  		= 0;
					FramesCTSError		= 0;
					FramesACKError		= 0;
					FramesXmitBFSK		= 0;
					FramesXmitQFSK		= 0;
					FramesXmitFrag		= 0;
					FramesRx				= 0;
					FramesRxDuplicate	= 0;
					FramesRxFrag	  	= 0;
					ResetStatisticsTime	= (unsigned_32)LLSGetCurrentTimeLONG();

					*data = 0;
					SendManagementDataPkt(sizeof (unsigned_16), 0x800B, dataPtr);
				break;
/*

				case 0x0C00:
					SetMemory(NotesBuffer, 0, NOTES_BUFFER_SIZE);
					for (i = 0; i < (NOTES_BUFFER_SIZE - 1) && (dataPtr[i] != '\0'); i++)
						NotesBuffer[i]	= dataPtr[i];
				//Update the Registry at this point and if success send the response
					*dataPtr = 0;
					SendManagementDataPkt(1, 0x800C, dataPtr);
				break;
*/
			}

		break;

		default:
		{	RcvPktLength = length;
			LLSReceiveLookAhead (dataPtr, length, SUCCESS, dataPkt->PRDAtoD);
			LLDUnknownProximPkt++;
		}
	}
}

/***************************************************************************/
/*																									*/
/*	SendManagementDataPkt (Length, SubType, Buffer)									*/
/*																									*/
/*	INPUT:	Length	- Length of the data of the response packet(no header)*/
/*				SubType	- Subtype of the Response Packet.							*/
/*				Buffer	- Pointer to the Request packet.								*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This procedure will send the response packet by				*/
/*		allocating an internal TX Buffer, copying the request data header		*/
/*		and sending the packet back.														*/
/*																									*/
/***************************************************************************/

void SendManagementDataPkt (unsigned_16 Length, unsigned_16 SubType,	unsigned_8 *Buffer)

{	struct ProximProtocolHeader	*Header;
	struct TXBStruct					*TxBuffer;
	unsigned_16							SNAP_SAP_Length;

	OutChar ('M', WHITE);
	OutChar ('D', WHITE);
	OutChar ('P', WHITE);

	if (FreeTXBuffer)
	{	TxBuffer = FreeTXBuffer;
		FreeTXBuffer = TxBuffer->NextBuffer;

		CopyMemory (TxBuffer->TxPkt.LLDImmediateData, Buffer, (sizeof (struct ProximProtocolHeader) + Length));
		TxBuffer->TxPkt.LLDImmediateDataLength = (unsigned_16)(Length + sizeof (struct ProximProtocolHeader));
		TxBuffer->TxPkt.LLDTxDataLength = (unsigned_16)(Length + sizeof (struct ProximProtocolHeader));

		Header = (struct ProximProtocolHeader *)TxBuffer->TxPkt.LLDImmediateData;

		SNAP_SAP_Length = (unsigned_16) (Length + sizeof (struct ProximProtocolHeader) - ((2 * ADDRESSLENGTH) + 2));
		Header->Length [0] = (unsigned_8)((SNAP_SAP_Length >> 8) & 0xFF);
		Header->Length [1] = (unsigned_8)(SNAP_SAP_Length & 0xFF);

		TxBuffer->TxPkt.LLDTxFragmentListPtr = &TxBuffer->TxFragList;
		TxBuffer->TxFragList.LLDTxFragmentCount = 0;

		CopyMemory (Header->DestAddr, Header->SourceAddr, ADDRESSLENGTH);
		CopyMemory (Header->SourceAddr, LLDNodeAddress, ADDRESSLENGTH);

		Header->SubType[0] = (unsigned_8)((SubType >> 8) & 0xFF);
		Header->SubType[1] = (unsigned_8)(SubType & 0xFF);
	
		LLDSend (&TxBuffer->TxPkt);
	}
}

/***************************************************************************/
/*																									*/
/*	SetupMasterHistory (*MasterHistory, MHLength, *Data)							*/
/*																									*/
/*	INPUT:	MasterHistory - pointer to buffer with inf. about MSTR history */
/*				MHLenght - offset of the position.  									*/
/*				Data - pointer to buffer where the inf. will be copied to.		*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This procedure will copy master inf. into the buffer.		*/
/*																									*/
/***************************************************************************/
		
unsigned_16 SetupMasterHistory(struct RoamHist *MasterHistory, unsigned_8 MHLength,	unsigned_8 *Data)
{
 	unsigned_16 MasterHistLength;
	unsigned_8	i, ii;
	unsigned_8	*TmpPtr;
#ifdef NDIS_MINIPORT_DRIVER
	ULONGLONG	currenttime, timediff;
#else
	unsigned_32 currenttime, timediff;
#endif

	MasterHistLength = (NAMELENGTH + 1 + ADDRESSLENGTH + 4) * MHLength + 1;
	if (MHLength != 0)
	{
		TmpPtr = Data;
		SetMemory (Data, 0, MasterHistLength);

		/*-------------------------------*/
		/* Get the current system time	*/
		/*-------------------------------*/
		currenttime = LLSGetCurrentTimeLONG ();

		/*----------------------------------------------------------*/
		/* add the list of masters roamed to the packet one by one	*/
		/* the data packet stores Master name, Master address,		*/
		/* how many 100-nanoseconds ago the node sync to it			*/
		/*----------------------------------------------------------*/
		for (i = 0; i < MHLength; i++)
		{
			ii = (unsigned_8)(MHLength - 1 - i);
			CopyMemory (TmpPtr, MasterHistory[ii].MasterName, NAMELENGTH);
			TmpPtr = TmpPtr + NAMELENGTH + 1;
			CopyMemory (TmpPtr, MasterHistory[ii].MasterAddress, ADDRESSLENGTH);
			TmpPtr = TmpPtr + ADDRESSLENGTH;
			/*----------------------------------------------------*/
			/* calculate how long ago the node sync to the master	*/
			/* send the difference in number of ticks					*/
			/*----------------------------------------------------*/
			timediff = currenttime - MasterHistory[ii].Time;
			/*----------------------------------*/
			/* append the time to the packet		*/
			/*----------------------------------*/
			ConvertLongIntoBigEndian(TmpPtr,(unsigned_32)timediff);
			TmpPtr = TmpPtr + 4;
		}
	}

	return	(MasterHistLength);
}

/***************************************************************************/
/*																									*/
/*	ConvertLongIntoBigEndian (buffer, longVariable)														*/
/*																									*/
/*	INPUT:	buffer	- pointer to the buffer where the data will be copy to*/
/*				longVariable - long that will be copy.									*/
/*																									*/
/* OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This procedure will copy a long in the big indian mode.	*/
/*																									*/
/***************************************************************************/

void ConvertLongIntoBigEndian (unsigned_8 *buffer, unsigned_32 longVariable)
{
	buffer[0] = (unsigned_8)((longVariable >> 24) & 0xFF);
	buffer[1] = (unsigned_8)((longVariable >> 16) & 0xFF);
	buffer[2] = (unsigned_8)((longVariable >> 8) & 0xFF);
	buffer[3] = (unsigned_8)(longVariable & 0xFF);
}
