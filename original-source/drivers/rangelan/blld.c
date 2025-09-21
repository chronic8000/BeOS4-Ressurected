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
/*	$Header:   J:\pvcs\archive\clld\blld.c_v   1.36   25 Sep 1998 09:03:18   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\blld.c_v  $											*/
/* 
/*    Rev 1.36   25 Sep 1998 09:03:18   BARBARA
/* Some parentheses eliminated.
/* 
/*    Rev 1.35   03 Jun 1998 14:29:58   BARBARA
/* TotalTxPackets added.
/* 
/*    Rev 1.34   07 May 1998 09:30:08   BARBARA
/* memcpy became CopyMemory and memset SetMemory.
/* 
/*    Rev 1.33   24 Mar 1998 08:49:14   BARBARA
/* LLD Ping added. LLDSetupAutoBridging changed.
/* 
/*    Rev 1.32   15 Jan 1997 17:26:14   BARBARA
/* Flushing the queue in AddToTimeQueue became configurable.
/* 
/*    Rev 1.31   14 Jun 1996 16:13:08   JEAN
/* Removed code to reset the RFNC when we run out of TCBQs.
/* 
/*    Rev 1.30   14 May 1996 09:34:42   TERRYL
/* Added LLDForceFrag flag.
/* 
/*    Rev 1.29   16 Apr 1996 11:27:20   JEAN
/* Fixed a bug in LLDGetEntry () when a node was reused and the
/* BridgeSuccess flag was not initialized.  Changed the oldflags
/* type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.28   01 Apr 1996 09:26:38   JEAN
/* Fixed a bug in LLDFlushQ where TxInProgress was being SET
/* instead of being CLEARed.
/* 
/*    Rev 1.27   29 Mar 1996 11:35:54   JEAN
/* Removed some PCMCIA code for SERIAL builds.
/* 
/*    Rev 1.26   22 Mar 1996 14:43:56   JEAN
/* Fixed a problem in LLDInitTable where the last entry's BridgeSuccess
/* did not get initialized.  Also fixed a problem in LLDSetupAutoBridging
/* where we were not initializing the last node entry.
/* 
/*    Rev 1.25   15 Mar 1996 13:57:40   TERRYL
/* Modified LLDGetEntry() to incorporate Peer_to_Peer feature.
/* Added LLDSetupAutoBridging() for Peer_to_Peer
/* 
/*    Rev 1.24   14 Mar 1996 14:42:38   JEAN
/* Added a debug color print.
/* 
/*    Rev 1.23   08 Mar 1996 19:01:44   JEAN
/* Fixed an infinite loop problem in LLDGetEntry where the LRU entry
/* is also the last entry in a hash bin.  Also added code to return
/* any queued packets or packets in progress when we reuse a node
/* in the node table.  Added code in LLDFlushQ to clear the Map
/* Table when cleaning out a node entry.
/* 
/*    Rev 1.22   04 Mar 1996 15:04:56   truong
/* Initialized node entry to bridge success state to reduce initial delay.
/* 
/*    Rev 1.21   06 Feb 1996 14:22:10   JEAN
/* Removed port.h for serial builds.
/* 
/*    Rev 1.20   31 Jan 1996 13:00:46   JEAN
/* For serial, removed a call to SLLDTxRxInit() when initializing
/* the node entry table (shouldn't be done here).  Changed the
/* ordering of some header include files and added some comments
/* to LLDGetEntry().
/* 
/*    Rev 1.19   19 Dec 1995 18:06:38   JEAN
/* Added a header include file.
/* 
/*    Rev 1.18   14 Dec 1995 15:11:08   JEAN
/* Bug fix in LLDGetEntry() and removed saving the links when
/* clearing out the LRU.
/* 
/*    Rev 1.17   07 Dec 1995 12:10:00   JEAN
/* Added a missing EnableRTS().
/* 
/*    Rev 1.16   05 Dec 1995 12:04:46   JEAN
/* Changed an #if to an #ifdef for SERIAL.
/* 
/*    Rev 1.15   27 Nov 1995 18:08:50   JEAN
/* Removed some debug code.
/* 
/*    Rev 1.14   17 Nov 1995 16:34:42   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.13   12 Oct 1995 11:37:34   JEAN
/* Added a function to flush the timed queue when we do a reset.
/* For SERIAL, added function calls to enable/disable RTS when
/* masking RFNC interrupts.
/* 
/*    Rev 1.12   23 May 1995 09:41:58   tracie
/* Bug fixes
/* 
/*    Rev 1.11   24 Apr 1995 15:48:30   tracie
/* 
/*    Rev 1.10   13 Apr 1995 08:48:04   tracie
/* XROMTEST version
/* 
/*    Rev 1.9   16 Mar 1995 16:15:00   tracie
/* Added support for ODI
/* 
/*    Rev 1.8   09 Mar 1995 15:07:44   hilton
/* 
/*    Rev 1.7   22 Feb 1995 10:29:20   tracie
/* Added Re-Entrant Queue include
/* 
/*    Rev 1.6   05 Jan 1995 09:54:08   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.5   27 Dec 1994 15:44:02   tracie
/* 
/*    Rev 1.3   29 Nov 1994 12:44:56   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:16   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:06   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:24   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/


#include	<memory.h>
#include <string.h>
#include "constant.h"
#include	"lld.h"
#include "asm.h"
#include "pstruct.h"
#include "bstruct.h"
#include "lldpack.h"
#include "blld.h"
#include "lldqueue.h"
#include	"lldsend.h"

#ifdef SERIAL
#include "slld.h"
#include "slldport.h"
#include "slldbuf.h"
#else
#include "port.h"
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8			LLDTransferMode;
extern unsigned_8			LLDNodeAddress [6];
extern unsigned_8			LLDPCMCIA;
extern unsigned_8			PCMCIACardInserted;

extern unsigned_8			PacketizePktBuf [128];
extern unsigned_8			LLDTxMode;

extern unsigned_8			DisableLLDSend;
extern unsigned_8			LLDNeedReset;
extern unsigned_8			LLDMSTAAddr [ADDRESSLENGTH];
extern unsigned_8			LLDPeerToPeerFlag;	/* Peer to Peer */
extern unsigned_32		TotalTxPackets;
#ifdef SERIAL
extern unsigned_8			TxInProgress[128];
#endif

extern struct RXBStruct	RXBuffers [TOTALRXBUF];
extern struct RXBStruct	*FreeRXBuffer;
extern struct RXBStruct	*RXBuffersPtr;
extern struct LLDLinkedList 	ReEntrantQ;

extern struct TXBStruct	TXBuffers [TOTALTXBUF];
extern struct TXBStruct	*FreeTXBuffer;

struct TXQEntry	TXQMemory [LLDMAXSEND];
struct TXQStruct	TXQueue;

struct TCBQEntry	TCBQMemory [LLDMAXSEND];
struct TCBQEntry	*FreeTCBQEntry;

struct NodeEntry	NodeTable [TOTALENTRY];

unsigned_8			LLDMapTable [128];
unsigned_8			HashTable [256];
unsigned_8			NextFreeNodeEntry;

unsigned_8			MRUNodeEntry;
unsigned_8			LRUNodeEntry;

#endif





/***************************************************************************/
/*																									*/
/*	LLDInitTable ()																			*/
/*																									*/
/*	INPUT: 																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will initialize all lookup tables,			*/
/*		buffers, linked lists, and queues.	  											*/
/*																									*/
/***************************************************************************/

void LLDInitTable (void)

{	unsigned_8	i;

	OutChar ('I', YELLOW+BLINK);	

	/*-------------------------------------------------*/
	/* Set the hash and map table entries to 0xFF. 		*/
	/* This is used instead of 0 as a NIL entry			*/
	/* because these are indexes into the NodeTable		*/
	/* which starts with 0.  The maximum number of		*/
	/* entries is currently 128.  If this were 256, FF	*/
	/* could not be used.										*/
	/*																	*/
	/*-------------------------------------------------*/

	memset (HashTable, 0xFF, 256);
	memset (LLDMapTable, 0xFF, 128);


	/*-------------------------------------------------*/
	/* Link the TCBQ elements together and set them		*/
	/* all in the free list.									*/
	/*																	*/
	/*-------------------------------------------------*/

	SetMemory (TCBQMemory, 0, sizeof (TCBQMemory));
	for (i = 0; i < (LLDMAXSEND - 1); i++)
	{	TCBQMemory [i].NextTCBLink = &TCBQMemory [i + 1];
	}
	TCBQMemory [LLDMAXSEND - 1].NextTCBLink = 0;
	FreeTCBQEntry = &TCBQMemory [0];


	/*-------------------------------------------------*/
	/* Link the timed queue elements together.  Set 	*/
	/* the head and tail pointers.							*/
	/*																	*/
	/*-------------------------------------------------*/

	SetMemory (TXQMemory, 0, sizeof (TXQMemory));
	for (i = 0; i < (LLDMAXSEND - 1); i++)
	{	TXQMemory [i].NextPktLink = &TXQMemory [i + 1];
	}
	TXQMemory [LLDMAXSEND - 1].NextPktLink = 0;
	TXQueue.Head = &TXQMemory [0];
	TXQueue.Tail = &TXQMemory [LLDMAXSEND - 1];


	/*-------------------------------------------------*/
	/* Link all of the Node table entries together and	*/
	/* place them on the free list.  Set the MRU and 	*/
	/* LRU node entry to NIL.									*/
	/*																	*/
	/*-------------------------------------------------*/
		
	SetMemory (NodeTable, 0, sizeof(NodeTable));
	for (i = 0; i < TOTALENTRY-1; i++)
	{	NodeTable [i].Link = (unsigned_8)(i + 1);
		NodeTable [i].BridgeSuccess = TRUE;
	}
	NodeTable [TOTALENTRY-1].Link = 0xFF;
	NodeTable [i].BridgeSuccess = TRUE;
	NextFreeNodeEntry = 0;

	MRUNodeEntry = 0xFF;
	LRUNodeEntry = 0xFF;

	/*-------------------------------------------------*/
	/* Link all of the receive buffers together and		*/
	/* put them in the free list.								*/
	/*																	*/
	/*-------------------------------------------------*/

	for (i = 0; i < (TOTALRXBUF - 1); i++)
	{	RXBuffers [i].NextBuffer = &RXBuffers [i + 1];
	}
	RXBuffers [TOTALRXBUF - 1].NextBuffer = 0;
	FreeRXBuffer = &RXBuffers [0];
	RXBuffersPtr = FreeRXBuffer;


	/*-------------------------------------------------*/
	/* Link all of the transmit buffers together and	*/
	/* put them in the free list.								*/
	/*																	*/
	/*-------------------------------------------------*/

	for (i = 0; i < (TOTALTXBUF - 1); i++)
	{	TXBuffers [i].NextBuffer = &TXBuffers [i + 1];
		TXBuffers [i].LLDInternalFlag = INTERNAL_PKT_SIGNATURE;
	}
	TXBuffers [TOTALTXBUF - 1].NextBuffer = 0;
	TXBuffers [TOTALTXBUF - 1].LLDInternalFlag = INTERNAL_PKT_SIGNATURE;
	FreeTXBuffer = &TXBuffers [0];

	initReEntrantQueues ();
}



/***************************************************************************/
/*																									*/
/*	LLDGetEntry (NodeAddress, *TableEntry)												*/
/*																									*/
/*	INPUT: 	NodeAddress	-	Pointer to 6 byte node address.						*/
/*				TableEntry	-	Location to store the index into the NodeTable	*/
/*									for this node.												*/
/*																									*/
/*	OUTPUT:	On Return	-	0 = Found an existing entry							*/
/*									1 = New Entry Created									*/
/*				TableEntry  -  Contains the index number for this node			*/
/*																									*/
/*	DESCRIPTION:  This procedure checks the Node Table for the entry.  It	*/
/*		starts the search by hashing the last byte of the node address to		*/
/* 	give an index into the NodeTable.  The Node Address is compared to	*/
/*		the entry in the table and if it matches, this entry is returned.		*/
/*		If not, the Link field of the entry is used as the next index since	*/
/*		this is the hash collision link.  If all links are checked and the	*/
/*		address is not found, a new entry is created.								*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGetEntry (unsigned_8 NodeAddress [], unsigned_8 *TableEntry)

{	unsigned_8		CurrEntry;

	/*-------------------------------------------------*/
	/* Start with the index given by hashing on the		*/
	/* last byte of the node address.						*/
	/*																	*/
	/*-------------------------------------------------*/

	CurrEntry = HashTable [NodeAddress [ADDRESSLENGTH - 1]];


	/*-------------------------------------------------*/
	/* Search until the Address is found in the hash	*/
	/* table or we are at the end of the list.  If	 	*/
	/* found, the most recently used node list is 		*/
	/*	updated.														*/
	/*																	*/
	/*-------------------------------------------------*/

	while (CurrEntry != 0xFF)
	{	if (memcmp (NodeAddress, NodeTable [CurrEntry].Address, ADDRESSLENGTH) == 0)
		{	
			/*-------------------------------------------*/
			/* We found the node already in the table.	*/
			/*-------------------------------------------*/
			*TableEntry = CurrEntry;


			/*-------------------------------------------*/
			/* Is the node already the MRU?  If so, then	*/
			/* we don't have anything to do.					*/
			/*															*/
			/*-------------------------------------------*/

			if (MRUNodeEntry != CurrEntry)
			{	

				/*-------------------------------------------*/
				/* Unlink the node from the MRU list and		*/
				/* place it at the head of the list.			*/
				/*															*/
				/*-------------------------------------------*/

				NodeTable [NodeTable [CurrEntry].TimeBackLink].TimeLink = NodeTable [CurrEntry].TimeLink;

				if (LRUNodeEntry != CurrEntry)
				{	NodeTable [NodeTable [CurrEntry].TimeLink].TimeBackLink = NodeTable [CurrEntry].TimeBackLink;
				}
				else
				{	LRUNodeEntry = NodeTable [CurrEntry].TimeBackLink;
				}

				NodeTable [CurrEntry].TimeLink = MRUNodeEntry;
				NodeTable [CurrEntry].TimeBackLink = 0xFF;

				if (MRUNodeEntry != 0xFF)
				{	NodeTable [MRUNodeEntry].TimeBackLink = CurrEntry;
				}
				else
				{	LRUNodeEntry = CurrEntry;
				}

				MRUNodeEntry = CurrEntry;
			}
			return (0);
		}
		else
		{	CurrEntry = NodeTable [CurrEntry].Link;
		}
	}

	/*-------------------------------------------------*/
	/* If there are no free node elements, then use		*/
	/* the LRU node element.  If there are any packets	*/
	/* in progress, we will flush the LRU's queue. 		*/
	/*																	*/
	/*-------------------------------------------------*/

	if (NextFreeNodeEntry == 0xFF)
	{	
		OutChar ('N', YELLOW);
		OutChar ('o', YELLOW);
		OutChar ('d', YELLOW);
		OutChar ('e', YELLOW);
		OutChar ('R', YELLOW);
		OutChar ('e', YELLOW);
		OutChar ('U', YELLOW);
		OutChar ('s', YELLOW);
		OutChar ('e', YELLOW);
		OutChar ('d', YELLOW);
		*TableEntry = LRUNodeEntry;
		NodeTable [NodeTable [LRUNodeEntry].TimeBackLink].TimeLink = 0xFF;
		LRUNodeEntry = NodeTable [LRUNodeEntry].TimeBackLink;

		/*----------------------------------------------*/
		/* Search from the hash table to fix any links	*/
		/* affected by removing this node.					*/
		/*																*/
		/*----------------------------------------------*/

		CurrEntry = HashTable [NodeTable [*TableEntry].Address [ADDRESSLENGTH - 1]];

		if (CurrEntry == *TableEntry)
		{	HashTable [NodeTable [*TableEntry].Address [ADDRESSLENGTH - 1]] = NodeTable [*TableEntry].Link;
		}
		else
		{	while (NodeTable [CurrEntry].Link != *TableEntry)
			{	CurrEntry = NodeTable [CurrEntry].Link;
			}
			NodeTable [CurrEntry].Link = NodeTable [*TableEntry].Link;
		}


		/*----------------------------------------------------*/
		/* If the node has any packets in progress, we need	*/
		/* to return them to the upper layer.  Also, we want	*/
		/* to clear out the entry in the node table before		*/
		/* reusing it.														*/
		/*----------------------------------------------------*/
		if (NodeTable[*TableEntry].PktInProgressFlag)
		{
			LLDFlushQ (*TableEntry);
		}

		SetMemory (&NodeTable [*TableEntry], 0, sizeof(struct NodeEntry) );

		NodeTable [*TableEntry].Link = NextFreeNodeEntry;
		NodeTable [*TableEntry].BridgeSuccess = TRUE;
		NextFreeNodeEntry = *TableEntry;

	}

			
			
	/*-------------------------------------------------*/
	/* The Address was not found in the table.  Get a	*/
	/* node element off the free list and link it to	*/
	/* the hash table or the end of the collision		*/
	/* chain.														*/
	/*																	*/
	/*-------------------------------------------------*/

	*TableEntry = NextFreeNodeEntry;
	NextFreeNodeEntry = NodeTable [NextFreeNodeEntry].Link;

	/*-------------------------------------------------*/
	/* Set DataSequence randomly.								*/
	/*-------------------------------------------------*/
	NodeTable [*TableEntry].DataSequenceNum = (unsigned_8) LLSGetCurrentTime ();


	/*------------------------------------------------------*/
	/* Make the new node the first element in the hash bin. */
	/*------------------------------------------------------*/
	NodeTable [*TableEntry].Link = HashTable [NodeAddress [ADDRESSLENGTH - 1]];
	HashTable [NodeAddress [ADDRESSLENGTH - 1]] = *TableEntry;

	CopyMemory (NodeTable [*TableEntry].Address, NodeAddress, ADDRESSLENGTH);

	
	/*-------------------------------------------------*/
	/* If Peer to Peer is Disabled, set up the Route   */
	/*	and the BridgeState										*/
	/*-------------------------------------------------*/

	if (!LLDPeerToPeerFlag)			/* In client server environment */
	{
		NodeTable[*TableEntry].BridgeState = BS_BRIDGED;
		CopyMemory (NodeTable[*TableEntry].Route, LLDMSTAAddr, ADDRESSLENGTH);
	}
 
	/*-------------------------------------------------*/
	/* Make this node the most recently used.				*/
	/*																	*/
	/*-------------------------------------------------*/

	if (MRUNodeEntry != 0xFF)
	{	NodeTable [MRUNodeEntry].TimeBackLink = *TableEntry;
	}
	else
	{	LRUNodeEntry = *TableEntry;
	}

	NodeTable [*TableEntry].TimeLink = MRUNodeEntry;
	NodeTable [*TableEntry].TimeBackLink = 0xFF;

	MRUNodeEntry = *TableEntry;

	
	return (1);
}




/***************************************************************************/
/*																									*/
/*	LLDAddToQueue (PktDesc, TableEntry)													*/
/*																									*/
/*	INPUT: 	PktDesc		-	Ptr to Tx Packet descriptor to add.					*/
/*				TableEntry	-	Index of the node entry.								*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will add the TX packet descriptor to the	*/
/*		queue in the table.																	*/
/*																									*/
/***************************************************************************/

void LLDAddToQueue (	struct LLDTxPacketDescriptor FAR *PktDesc, 
							unsigned_8 TableEntry )

{	struct TCBQEntry	*CurrentTCB;
	FLAGS					oldflag;

	OutChar ('+', BLACK+WHITE_BACK);
	OutChar ('Q', BLACK+WHITE_BACK);
	OutHex (TableEntry, BLUE);

	oldflag = PreserveFlag ();
	DisableInterrupt ();


	/*-------------------------------------------------*/
	/* First check if any free nodes are available to	*/
	/* add to the list.  If not, just return the 		*/
	/* packet to the upper layer.								*/
	/*																	*/
	/*-------------------------------------------------*/

	if (FreeTCBQEntry == 0)
	{	
#ifdef SERIAL
		DisableRTS();
#endif
		OutChar ('Q', RED);
		OutChar ('F', RED);
		OutChar ('U', RED);
		OutChar ('L', RED);
		OutChar ('L', RED);
		
		OutChar ('T', RED);
		OutChar ('5', RED);

		if (IsTxProximPacket (PktDesc))
		{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (PktDesc);
			}
			else
			{	LLSSendProximPktComplete (PktDesc, QUEUE_FULL);
			}
		}
		else
		{	LLSSendComplete (PktDesc);
		}
#ifdef SERIAL
		EnableRTS();
#endif
	}


	/*-------------------------------------------------*/
	/* If an entry is already in the queue, find the	*/
	/* last entry and link it to the end.					*/
	/*																	*/
	/*-------------------------------------------------*/

	else if (NodeTable [TableEntry].QPointer)
	{	CurrentTCB = NodeTable [TableEntry].QPointer;

		while (CurrentTCB->NextTCBLink)		
		{	CurrentTCB = CurrentTCB->NextTCBLink;
		}

		CurrentTCB->NextTCBLink = FreeTCBQEntry;
		FreeTCBQEntry = FreeTCBQEntry->NextTCBLink;
		CurrentTCB->NextTCBLink->NextTCBLink = 0;
		CurrentTCB->NextTCBLink->TCBPtr = PktDesc;
		CurrentTCB->NextTCBLink->ENode = TableEntry;
	}


	/*-------------------------------------------------*/
	/* No entries are in the queue.  Put this as the	*/
	/* first in the list.										*/
	/*																	*/
	/*-------------------------------------------------*/

	else
	{	NodeTable [TableEntry].QPointer = FreeTCBQEntry;
		CurrentTCB = FreeTCBQEntry;
		FreeTCBQEntry = FreeTCBQEntry->NextTCBLink;
		CurrentTCB->NextTCBLink = 0;
		CurrentTCB->TCBPtr = PktDesc;
		CurrentTCB->ENode = TableEntry;
	}

	RestoreFlag (oldflag);
}




/***************************************************************************/
/*																									*/
/*	LLDPullFromQueue (TableEntry)															*/
/*																									*/
/*	INPUT: 	TableEntry	-	Index of node entry with packet on TX queue.		*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will pull a packet from the queue and		*/
/*		send it.																					*/
/*																									*/
/***************************************************************************/

void LLDPullFromQueue (unsigned_8 TableEntry)

{	struct TCBQEntry	*CurrTCB;
	FLAGS					oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	if (NodeTable [TableEntry].QPointer)
	{	
#ifdef SERIAL
		DisableRTS();
#endif
		OutChar ('-', BLACK+WHITE_BACK);
		OutChar ('Q', BLACK+WHITE_BACK);
		OutHex (TableEntry, BLUE);

		CurrTCB = NodeTable [TableEntry].QPointer;
		NodeTable [TableEntry].QPointer = CurrTCB->NextTCBLink;

		CurrTCB->NextTCBLink = FreeTCBQEntry;
		FreeTCBQEntry = CurrTCB;

		NodeTable [TableEntry].PktInProgressFlag = CLEAR;

		LLDSend ( (struct LLDTxPacketDescriptor FAR *) CurrTCB->TCBPtr);
#ifdef SERIAL
		EnableRTS();
#endif
	}

	RestoreFlag (oldflag);
}





/***************************************************************************/
/*																									*/
/*	LLDAddToTimedQueue (PktDesc, TableEntry, StartTime, MaxWait)				*/
/*																									*/
/*	INPUT: 	PktDesc		-	TX Packet descriptor of packet to send.			*/
/*				TableEntry	-	Index of node entry with packet on TX queue.		*/
/*				StartTime	-	Time in ticks - start time.							*/
/*				MaxWait		-	Time in ticks to wait until time out				*/
/*				FlushQFlag 	-	Set when the SendQueue needs to be flushed		*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will add a packet to the timed queue.		*/
/*																									*/
/***************************************************************************/

void LLDAddToTimedQueue	(	struct LLDTxPacketDescriptor FAR *PktDesc, 
									unsigned_8 TableEntry,
									unsigned_16 StartTime, 
									unsigned_16 MaxWait,
									unsigned_8	FlushQFlag )

{	struct TXQEntry	*CurrEntry;
	FLAGS					oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	OutChar ('+', BLACK+WHITE_BACK);
	OutChar ('t', BLACK+WHITE_BACK);
	OutChar ('Q', BLACK+WHITE_BACK);
	OutHex (TableEntry, BLUE);

	NodeTable [TableEntry].PktInTimedQFlag = SET;
	

	/*-------------------------------------------------*/
	/* Check if anything at the head of the queue.  If	*/
	/* so, search down to find an available entry.		*/
	/*																	*/
	/*-------------------------------------------------*/

	CurrEntry = TXQueue.Head;

	if (TXQueue.Head->QOccupy)
	{	
#ifdef SERIAL
		DisableRTS();
#endif
		CurrEntry = TXQueue.Head;

		while ((CurrEntry != 0) && (CurrEntry->QOccupy))
		{	CurrEntry = CurrEntry->NextPktLink;
		}

		if (CurrEntry == 0)
		{	OutChar ('h', YELLOW);
			NodeTable [TableEntry].PktInTimedQFlag = CLEAR;
			
			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, QUEUE_FULL);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}
			
			RestoreFlag (oldflag);
#ifdef SERIAL
			EnableRTS();
#endif
			return;
		}
#ifdef SERIAL
		EnableRTS();
#endif
	}


	/*-------------------------------------------------*/
	/* Put the packet at this location in the queue.	*/
	/*																	*/
	/*-------------------------------------------------*/

	CurrEntry->QOccupy = SET;
	CurrEntry->QMaxTime = MaxWait;
	CurrEntry->QStartTime = StartTime;
	CurrEntry->QNode = TableEntry;
	CurrEntry->QPkt = PktDesc;


	/*-------------------------------------------------*/
	/* The queue must be flushed because it took time	*/
	/* to figure out that we should be on the timed		*/
	/* queue.														*/
	/*																	*/
	/*-------------------------------------------------*/
	if (FlushQFlag)
		LLDFlushQ (TableEntry);

	RestoreFlag (oldflag);
}




/***************************************************************************/
/*																									*/
/*	LLDPullFromTimedQueue ()																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will pull a packet from the queue and		*/
/*		send it if the time to wait has expired.										*/
/*																									*/
/***************************************************************************/

void LLDPullFromTimedQueue (void)

{	unsigned_16									CurrTime;
	struct TXQEntry						  *CurrEntry;
	unsigned_8									TableEntry;
	struct LLDTxPacketDescriptor FAR   *PktDesc;
	FLAGS											oldflag;


	oldflag = PreserveFlag ();
	DisableInterrupt ();

	CurrTime = LLSGetCurrentTime ();
	CurrEntry = TXQueue.Head;


	/*-------------------------------------------------*/
	/* Check if the head of the queue time has expired.*/
	/* Yes, then pull it from the queue and send it.	*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((CurrEntry->QOccupy) && ((CurrTime - CurrEntry->QStartTime) >= CurrEntry->QMaxTime))
	{	
#ifdef SERIAL
		DisableRTS();
#endif
		OutChar ('-', BLACK+WHITE_BACK);
		OutChar ('t', BLACK+WHITE_BACK);
		OutChar ('Q', BLACK+WHITE_BACK);
		OutHex (CurrEntry->QNode, BLUE);
		OutHex ((unsigned_8)(NodeTable [CurrEntry->QNode].Address [0]), BLUE);

		/*----------------------------------------------*/
		/* Now pull from the queue and put the element	*/
		/* to the tail.											*/
		/*																*/
		/*----------------------------------------------*/

		TXQueue.Head = CurrEntry->NextPktLink;
		TXQueue.Tail->NextPktLink = CurrEntry;
		TXQueue.Tail = CurrEntry;
		CurrEntry->NextPktLink = 0;

		/*----------------------------------------------*/
		/* Now clear the variables in the element.		*/
		/* Send the packet out.									*/
		/*																*/
		/*----------------------------------------------*/

		TableEntry = CurrEntry->QNode;
		PktDesc = CurrEntry->QPkt;

		CurrEntry->QOccupy = CLEAR;
		CurrEntry->QStartTime = 0;
		CurrEntry->QMaxTime = 0;
		CurrEntry->QPkt = 0;
		CurrEntry->QNode = 0;

		NodeTable [TableEntry].PktInTimedQFlag = CLEAR;

#ifdef CSS
		if ((LLDPCMCIA) && (PCMCIACardInserted == 0))
		{	
			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, NO_CARD);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}
		}
		else
		{	
			LLDSend (PktDesc);
		}
#else
		LLDSend (PktDesc);
#endif


#ifdef SERIAL
		EnableRTS();
#endif
	}

	RestoreFlag (oldflag);
}
		




/***************************************************************************/
/*																									*/
/*	LLDFlushQ (TableEntry)																	*/
/*																									*/
/*	INPUT:	TableEntry	-	Node table entry whose TX queue will be flushed.*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will pull all packets from the regular		*/
/*		transmit queue and throw them away.												*/
/*																									*/
/***************************************************************************/

void LLDFlushQ (unsigned_8 TableEntry)

{	
	struct TCBQEntry	*CurrEntry;
	struct TCBQEntry	*Temp;
	FLAGS					oldflag;

	oldflag = PreserveFlag ();
	DisableInterrupt ();

	OutChar ('F', YELLOW);
	OutChar ('l', YELLOW);
	OutChar ('u', YELLOW);
	OutChar ('s', YELLOW);
	OutChar ('h', YELLOW);
	OutChar ('Q', YELLOW);
	OutHex (TableEntry, YELLOW);

	CurrEntry = NodeTable [TableEntry].QPointer;
	while (CurrEntry != 0)
	{	
#ifdef SERIAL
		DisableRTS();
#endif
		OutChar ('-', WHITE+BLUE_BACK);
		OutChar ('Q', WHITE+BLUE_BACK);

		if (IsTxProximPacket (CurrEntry->TCBPtr))
		{	if (*((unsigned_32 *)((unsigned_8 *)CurrEntry->TCBPtr - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (CurrEntry->TCBPtr);
			}
			else
			{	LLSSendProximPktComplete (CurrEntry->TCBPtr, RESET_RFNC);
			}
		}
		else
		{	LLSSendComplete (CurrEntry->TCBPtr);
		}

		/*--------------------------------------------*/
		/* Is there an outstanding transmit complete? */
		/*--------------------------------------------*/
		if (TableEntry == LLDMapTable [NodeTable[TableEntry].LastPacketSeqNum])
		{
			LLDMapTable [NodeTable[TableEntry].LastPacketSeqNum] = SET;
#ifdef SERIAL
			TxInProgress [NodeTable[TableEntry].LastPacketSeqNum] = CLEAR;
			OutHex (NodeTable[TableEntry].LastPacketSeqNum, MAGENTA);
#endif
		}

		Temp = CurrEntry->NextTCBLink;

		CurrEntry->TCBPtr = 0;
		CurrEntry->NextTCBLink = FreeTCBQEntry;
		FreeTCBQEntry = CurrEntry;

		CurrEntry = Temp;
#ifdef SERIAL
		EnableRTS();
#endif
	}
	NodeTable [TableEntry].QPointer = 0;

	RestoreFlag (oldflag);
}




/***************************************************************************/
/*																									*/
/*	LLDResetQ ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure will pull all packets from the regular		*/
/*		transmit queue and throw them away.  This will get all packets from	*/
/*		all nodes in the table.																*/
/*																									*/
/***************************************************************************/

void LLDResetQ (void)

{	
	struct ReEntrantQueue	*Qptr;
	unsigned_8					i;
	FLAGS							oldflag;

	OutChar ('R', BROWN);
	OutChar ('E', BROWN);
	OutChar ('S', BROWN);
	OutChar ('E', BROWN);
	OutChar ('T', BROWN);
	OutChar ('Q', BROWN);

#ifdef SERIAL
	DisableRTS();
#endif
	oldflag = PreserveFlag ();
	DisableInterrupt ();

	DisableLLDSend = SET;

	for (i = 0; i < TOTALENTRY; i++)
	{	
		if ((NodeTable [i].PktInProgressFlag) && (NodeTable [i].TxPkt))
		{	
			if (IsTxProximPacket (NodeTable [i].TxPkt))
			{	if (*((unsigned_32 *)((unsigned_8 *)NodeTable[i].TxPkt - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (NodeTable [i].TxPkt);
				}
				else
				{	LLSSendProximPktComplete (NodeTable [i].TxPkt, RESET_RFNC);
				}
			}
			else
			{	LLSSendComplete (NodeTable [i].TxPkt);
			}
		}

		NodeTable [i].PktInProgressFlag = CLEAR;

		if (NodeTable [i].QPointer)
		{	
			LLDFlushQ (i);
		}
	}

	/*------------------------------------------*/
	/*      Clean up the Re-Entrant Queue       */
	/*------------------------------------------*/
	while ((Qptr = RemoveReEntrantHead (&ReEntrantQ)) != NIL)
	{
		OutChar ('R', CYAN);
		OutChar ('E', CYAN);
		OutChar ('Q', CYAN);
		if ( Qptr->PacketType == 0 )
		{
			if (IsTxProximPacket ((struct LLDTxPacketDescriptor FAR *) Qptr->PacketPtr))
			{	if (*((unsigned_32 *)((unsigned_8 *)Qptr->PacketPtr - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer ((struct LLDTxPacketDescriptor FAR *) Qptr->PacketPtr);
				}
				else
				{	LLSSendProximPktComplete ((struct LLDTxPacketDescriptor FAR *) Qptr->PacketPtr, RESET_RFNC);
				}
			}
			else
			{	LLSSendComplete ((struct LLDTxPacketDescriptor FAR *) Qptr->PacketPtr );
			}
		}
	}

	LLDFlushTimedQueue ();
	LLDInitTable ();

	DisableLLDSend = CLEAR;
	RestoreFlag (oldflag);
#ifdef SERIAL
	EnableRTS();
#endif
}

		
/***************************************************************************/
/*																									*/
/*	LLDFlushTimedQueue ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This procedure pulls all of the packets from the timed		*/
/*		transmit queue and returns them to the upper layer.  It is assumed	*/
/*    that interrupts are disabled when this routine is called.				*/
/*																									*/
/***************************************************************************/

void LLDFlushTimedQueue (void)
{	struct TXQEntry *CurrEntry;

	OutChar ('R', GREEN);
	OutChar ('e', GREEN);
	OutChar ('s', GREEN);
	OutChar ('e', GREEN);
	OutChar ('t', GREEN);
	OutChar ('T', GREEN);
	OutChar ('X', GREEN);
	OutChar ('Q', GREEN);

	/*-------------------------------------------------------*/
	/* The timed wait queue is a single singly-linked list.	*/
	/* There is not a separate list for free elements.  The	*/
	/* head of the list contains the elements that are			*/
	/* occupied, and the end of the list contains free 		*/
	/* elements.  The oldest entry is at the head of the		*/
	/* list and the youngest entry is somewhere in the			*/
	/* middle of the list followed by the free entries.		*/
	/*-------------------------------------------------------*/
	CurrEntry = TXQueue.Head;

	/*
	 * Search the list for all occupied entries.
	 * When we find one, we need to call LLSSendComplete
	 * to return the packet to the upper layer and free
	 * up the entry.
	 */
	while ((CurrEntry != NULL) && (CurrEntry->QOccupy == SET))
	{
		if (IsTxProximPacket (CurrEntry->QPkt))
		{	if (*((unsigned_32 *)((unsigned_8 *)CurrEntry->QPkt - sizeof (unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (CurrEntry->QPkt);
			}
			else
			{	LLSSendProximPktComplete (CurrEntry->QPkt, RESET_RFNC);
			}
		}
		else		
		{	LLSSendComplete (CurrEntry->QPkt);
		}

		NodeTable [CurrEntry->QNode].PktInTimedQFlag = CLEAR;

		CurrEntry->QOccupy = CLEAR;
		CurrEntry->QStartTime = 0;
		CurrEntry->QMaxTime = 0;
		CurrEntry->QPkt = 0;
		CurrEntry->QNode = 0;

		CurrEntry = CurrEntry->NextPktLink;
	}
}




/***************************************************************************/
/*																									*/
/*	LLDSetupAutoBridging ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This routine will setup the Router to the master address  */
/*					and set the BridgeState for client/server environment			*/
/*																									*/
/***************************************************************************/

void LLDSetupAutoBridging (void)
{
   int i;
 
	OutChar ('P', MAGENTA);
	OutChar ('e', MAGENTA);
	OutChar ('e', MAGENTA);
	OutChar ('r', MAGENTA);
	
	for (i = 0; i < TOTALENTRY; i++)
	{	
		if (!LLDPeerToPeerFlag)
		{
			CopyMemory (NodeTable[i].Route, LLDMSTAAddr, ADDRESSLENGTH);
			NodeTable[i].BridgeState = BS_BRIDGED;
		}
		else
		{
			SetMemory (NodeTable[i].Route, 0, ADDRESSLENGTH);
			NodeTable[i].BridgeState = BS_DIRECT;
		}

	}
}




/***************************************************************************/
/*																									*/
/*	LLDFreeInternalTXBuffer ()																*/
/*																									*/
/*	INPUT:	TXBuffer	-	Pointer to TX Packet Descriptor to free.				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:  This routine will put the TX Buffer back on the free		*/
/*		list.  Note that the packet descriptor must be the starting address	*/
/*		of the TXBStruct.																		*/
/*																									*/
/***************************************************************************/

void LLDFreeInternalTXBuffer (struct LLDTxPacketDescriptor *TXBuffer)

{	struct TXBStruct	*Buffer;

	TotalTxPackets++;
	Buffer = (struct TXBStruct *)((unsigned_8 *)TXBuffer - sizeof(unsigned_32));

	Buffer->NextBuffer = FreeTXBuffer;
	FreeTXBuffer = Buffer;
}