/***************************************************************************/
/*                                                                         */
/*                   PROXIM RANGELAN2 LOW LEVEL DRIVER                     */
/*                                                                         */
/*    PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the       */
/*    sole property of PROXIM, INC.  Reproduction or utilization of        */
/*    this source in whole or in part is forbidden without the written     */
/*    consent of PROXIM, INC.                                              */
/*                                                                         */
/*                                                                         */
/*                   (c) Copyright PROXIM, INC. 1994                       */
/*                         All Rights Reserved                             */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/*      $Header:   J:\pvcs\archive\clld\bstruct.h_v   1.18   11 Aug 1998 13:03:10   BARBARA  $*/
/*                                                                                                                                                                                                      */
/* Edit History                                                            */
/*                                                                         */
/* $Log:   J:\pvcs\archive\clld\bstruct.h_v  $                             */
/* 
/*    Rev 1.18   11 Aug 1998 13:03:10   BARBARA
/* Time field of RoamHistory changed from unsigned_16 to ULONGLONG.
/* 
/*    Rev 1.17   04 Jun 1998 11:15:30   BARBARA
/* Proxim protocol releated changes added.
/* 
/*    Rev 1.16   24 Mar 1998 08:53:42   BARBARA
/* LLD Ping struct added. RxBuffers struct modified.
/* 
/*    Rev 1.15   12 Sep 1997 16:55:02   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.14   15 Mar 1997 13:36:20   BARBARA
/* NodeExistsFlag added into the NodeTable.
/* 
/*    Rev 1.13   15 Jan 1997 17:42:22   BARBARA
/* NumberOfFragments and LastFragSeqNum added.
/* 
/*    Rev 1.12   14 Jun 1996 16:25:24   JEAN
/* Comment changes.
/* 
/*    Rev 1.11   16 Apr 1996 11:38:42   JEAN
/* 
/*    Rev 1.10   08 Mar 1996 19:19:18   JEAN
/* Reorganized the node table entries to get rid of 2 padding bytes
/* for each entry and to make reading dumped memory easier.  Change
/* a pad byte in the node table to save the last packet sequence
/* number used.
/* 
/*    Rev 1.9   31 Jan 1996 13:32:34   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.8   17 Nov 1995 16:40:36   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.7   12 Oct 1995 11:53:30   JEAN
/* Added #defines for bridge states.
/* 
/*    Rev 1.6   23 May 1995 09:48:00   tracie
/* 
/*    Rev 1.5   16 Mar 1995 16:18:26   tracie
/* Added support for ODI
/* 
/*    Rev 1.4   22 Feb 1995 10:38:16   tracie
/* 
/*    Rev 1.3   27 Dec 1994 16:09:44   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:24   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:20   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:34   hilton
/* Initial revision.
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#ifndef BSTRUCT_H
#define BSTRUCT_H


/*
 * Bridge State Constants
 */

#define  BS_DIRECT      0
#define  BS_BRIDGED     1
#define  BS_DISCOVERY   2

#define  FRAG_NUM_MASK                  0x0F
#define  TOTAL_FRAGS_MASK               0xF0
#define WAIT_FOR_FRAG_STATE     0xFF
#define TX_FRAG_RETRIES         10

/*********************************************************************/
/*                                                                   */
/* The TCB Queue entry is a node containing a transmit packet        */
/* descriptor which needs to be sent.  If a packet is already being  */
/* sent to the same destination node, the new packet to be           */
/* transmitted must be queued.  The NodeTable entry QPointer will    */
/* point to the next packet to send.                                 */
/*                                                                   */
/*********************************************************************/

struct TCBQEntry
{  struct TCBQEntry     *NextTCBLink;
	unsigned_16           ENode;
	struct LLDTxPacketDescriptor FAR *TCBPtr;
};



/*********************************************************************/
/*                                                                   */
/* The timed transmit queue is for packets which need to be sent at  */
/* a later time (mostly for waiting for possible sleeping stations   */
/* to wake up.  The head of the queue is checked in LLDPoll to see   */
/* if a packet needs to be fired off if the expired time exceeds     */
/* the max time field.                                               */
/*                                                                   */
/*********************************************************************/

struct TXQEntry
{  struct TXQEntry                  *NextPktLink;
	unsigned_8                       QOccupy;
	unsigned_8                       QNode;
	struct LLDTxPacketDescriptor FAR *QPkt;
	unsigned_16                      QStartTime;
	unsigned_16                      QMaxTime;
};


struct TXQStruct
{  struct TXQEntry   *Head;
	struct TXQEntry   *Tail;
};



/*********************************************************************/
/*                                                                   */
/* The receive buffers are linked together to form a free buffer     */
/* pool from which buffers may be allocated.                         */
/*                                                                   */
/*********************************************************************/

struct RXBStruct
{  struct RXBStruct  *NextBuffer;
	unsigned_8        RxData [BUFFERLEN];
	unsigned_16                     Time;
	struct NodeEntry        *NodePointer;
};



/*********************************************************************/
/*                                                                   */
/* The transmit buffers are linked together to form a free buffer    */
/* pool from which buffers may be allocated.                         */
/*                                                                   */
/* DO NOT put anything before TxPkt in the structure.  The code                 */
/*      will assume that the returned packet descriptor is the starting */
/*      address of this linked list buffer structure.                                                   */
/*                                                                                                                                                                                      */
/*********************************************************************/

struct TXBStruct
{       
	unsigned_32                                                      LLDInternalFlag;
	struct LLDTxPacketDescriptor     TxPkt;
	struct TXBStruct                                        *NextBuffer;
	struct LLDTxFragmentList                 TxFragList;
};



/*********************************************************************/
/*                                                                   */
/* The node table will be use to store information about the node    */
/* that station is talking with.  A hash table is used to index      */
/* to the entry in the NodeTable.                                    */
/*                                                                   */
/*********************************************************************/

struct NodeEntry
{  
	/* Arrange the structure into 4-byte groups.  Makes reading a dump easier.      */

	unsigned_8        Link;             /* Index to next entry                 */
	unsigned_8        TimeLink;         /* Index for MRU -> LRU linked list    */
	unsigned_8        TimeBackLink;     /* Back index for MRU -> LRU list      */
	unsigned_8                      LastPacketSeqNum; /* This is the last sequence number           */
													/* used in the packetized command               */
													/* header.                                                                              */

	unsigned_8        Address[6];       /* Address of node communicating with  */
	unsigned_8        Route[6];         /* Address of middle man, if any       */

	unsigned_8        RepeatFlag;       /* Set if going through a repeater     */
	unsigned_8        BridgeState;      /* Bridge state (direct, bridge, disc) */
	unsigned_8        FragState;        /* Frag state (none, frag, discovery)  */
	unsigned_8        BFSKFlag;         /* Set if send in BFSK mode            */

	unsigned_8        CTSErrorCount;    /* # of times we got CTS error         */
	unsigned_8        BridgeSuccess;    /* Successfully gone through a bridge  */
	unsigned_16       FragStartTime;    /* Starting time of the fragment state */

	unsigned_16       BridgeStartTime;  /* Starting time of the bridge state   */
	unsigned_8        BFSKPktCount;     /* Total # of pkts transmitted in BFSK */
	unsigned_8        BFSKRcvCount;     /* # of BFSK responses from firmware   */

	unsigned_8                      NumberOfFragments; /* Number of Tx Fragments for 1 pkt  */
	unsigned_16       RepeatStartTime;  /* Starting time of the repeat state   */
	unsigned_8        LastRxFragNum;    /* Last fragment number received       */
	unsigned_8        LastFragSeqNum;    /* Last fragment's sequence number         */
	unsigned_8        FirstRxFlag;      /* 0 - if waiting for first receive    */

	unsigned_8        LastSeqNum;       /* Last sequence number received from  */
													/* from remote node.                                                    */
	unsigned_8        DataSequenceNum;  /* Current sequence # for LLDHeader3   */
	unsigned_16       RxPktLength;      /* Data portion length of RX packet    */

	struct RXBStruct  *RxBuf;           /* Ptr to start of the current RX buf  */
	unsigned_8        *CurrentRxBufPtr; /* Ptr to pending receive packet buf   */

	unsigned_8        PktInTimedQFlag;  /* Set if packet in the timed queue    */
	unsigned_8        PktInProgressFlag;/* Set after putting pkt in the card   */
	struct TCBQEntry  *QPointer;        /* Ptr to transmit queue               */

	struct LLDTxPacketDescriptor FAR *TxPkt;        /* Ptr to pkt descriptor of xmit pkt   */
	unsigned_8                      NodeExistsFlag;                 /* Set if Tx or RX successfully         */
};



/*********************************************************************/
/*                                                                   */
/* The ReEntrant Queue are used to store packets that were unable to */
/* send because there is another packet in the middle of sending.               */
/* When the sending state becomes idle, this ReEntrant Queue will be */
/*      polled and packets waiting in this queue will be send out.                      */
/*                                                                   */
/*********************************************************************/
struct ReEntrantQueue
{
	struct ReEntrantQueue   *Next;          /* Pointer to next buffer in queue      */
	unsigned_8                        FAR   *PacketPtr;     /* Pointer to packet to be sent                 */
	unsigned_16                                     PacketType;     /* 0-Reg.Send, else=Pktiz. Send len.*/
};


struct LLDLinkedList
{       
	struct ReEntrantQueue *Head;
	struct ReEntrantQueue *Tail;
};

struct MastersList
{
	unsigned_8      ChSubChannel;
	unsigned_8      Name[11];
	unsigned_8      Address[6];
};

struct MastersList LLDMastersFound[MAXROAMHISTMSTRS];


struct RoamHist
{
	unsigned_8		MasterName[NAMELENGTH];
	unsigned_8		MasterAddress[ADDRESSLENGTH];
#ifdef NDIS_MINIPORT_DRIVER
	ULONGLONG		Time;
#else
	unsigned_32		Time;
#endif
};

struct RoamHist	RoamHistory[MAXROAMHISTMSTRS];

#endif /* BSTRUCT_H */
