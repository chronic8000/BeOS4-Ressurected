/***************************************************************************/
/*                                                                         */
/*						  	PROXIM RANGELAN2 LOW LEVEL DRIVER							*/
/*                                                                         */
/*    PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the       */
/*    sole property of PROXIM, INC.  Reproduction or utilization of        */
/*    this source in whole or in part is forbidden without the written     */
/*    consent of PROXIM, INC.                                              */
/*                                                                         */
/*                                                                         */
/*                   (c) Copyright PROXIM, INC. 1995                       */
/*                         All Rights Reserved                             */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/*	$Header:   J:\pvcs\archive\clld\wbrp.c_v   1.6   16 Apr 1996 11:37:48   JEAN  $																						*/
/*																									*/
/* Edit History                                                            */
/*                                                                         */
/* $Log:   J:\pvcs\archive\clld\wbrp.c_v  $                                */
/* 
/*    Rev 1.6   16 Apr 1996 11:37:48   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.
/* 
/*    Rev 1.5   06 Feb 1996 14:31:10   JEAN
/* Comment changes.
/* 
/*    Rev 1.4   31 Jan 1996 13:24:36   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.3   14 Dec 1995 15:37:30   JEAN
/* Added header include file.
/* 
/*    Rev 1.2   12 Oct 1995 11:52:36   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* When compiling for SERIAL, added code to enable/disable RTS
/* when RFNC interrupts are masked.
/* 
/*    Rev 1.1   16 Mar 1995 16:18:04   tracie
/* 
/*    Rev 1.0   22 Feb 1995 09:51:42   tracie
/* Initial revision.
/***************************************************************************/

#include <assert.h>
#include <memory.h>

#include "constant.h"
#include "lld.h"
#include "bstruct.h"
#include "pstruct.h"
#include "asm.h"

#ifdef WBRP
#include "rptable.h"
#include "wbrp.h"
#include "lldwbrp.h"
#endif

#ifdef SERIAL
#include "slld.h"
#include "slldport.h"
#endif


/***************************************************************************/
/*                                                                         */
/*                      Global Variables used by LLD                       */
/*                                                                         */
/***************************************************************************/
#ifdef MULTI
#include "multi.h"

extern struct LLDVarsStruct *LLDVars;

#else

extern unsigned_8			   LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_16		   WBRPStartTime;

#endif


/***************************************************************************/
/*                                                                         */
/*                 Global Variables used by the WBRP Only                  */
/*                                                                         */
/***************************************************************************/

unsigned_8  					BroadcastAddress[] = { 	0xff, 0xff, 0xff, 
																	0xff, 0xff, 0xff 
																};
unsigned_32                TicksSinceLastBroadcast;
struct WBRPPacketBuf 		WBRPBuffers[WBRPBUFFERCNT];
struct LLDRepeatingPacket  RepeatingPacket;	/* Repeating Data Packet	*/
struct WBRPPacket          WBRPPacketBuf;		/* WBRP Reply/Request Pkt	*/
struct WBRPStatistics		WBRPStatus;			/* WBRP Statistics			*/


/***************************************************************************/
/*                                                                         */
/*  RepeatingInit ()					                                          */
/*                                                                         */
/*  INPUT:	                                                               */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION: This procedure will perform the Repeater Initialization   */
/*                                                                         */
/***************************************************************************/

void WBRPRepeaterInit (void)

{
	OutChar ('W', CYAN);
	OutChar ('B', CYAN);
	OutChar ('R', CYAN);
	OutChar ('P', CYAN);
	OutChar ('I', CYAN);
	OutChar ('n', CYAN);
	OutChar ('i', CYAN);
	OutChar ('t', CYAN);

	/*--------------------------------------------*/
	/* Initialize its Repeating Information Table */
	/* with itself as the only entry.             */
	/*--------------------------------------------*/
	memset ( &WBRPStatus, 0, sizeof (struct WBRPStatistics) );

	InitWBRPBuffers ();
	RepeatingTableInit();
	AddRepeatingTableEntry(LLDNodeAddress, 0, 100, LLDNodeAddress);

	/*--------------------------------------------*/
	/* The repeater broadcasts a WBRP REPLY with a*/
	/* single repeater entry.  Sends out a        */
	/* broadcast WBRP REQUEST asking for routing  */
	/* information from its repeating neighbors.  */
	/*--------------------------------------------*/

	SendCompleteWBRPReply (BroadcastAddress); 
	SendWBRPRequest();                       

	TicksSinceLastBroadcast = 0L;
	WBRPStartTime = LLSGetCurrentTime ();
}


/***************************************************************************/
/*                                                                         */
/*  SendCompleteWBRPReply (*destination)                                   */
/*                                                                         */
/*  INPUT:       destination - The destination address                     */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will prepare a WBRP reply packet and send */
/*                WBRP Reply to all repeaters in the Repeater Table that   */
/*                is aware of.                                             */
/*                                                                         */
/***************************************************************************/

void SendCompleteWBRPReply (unsigned_8 *destination)
{
	struct WBRPPacket *packet;
	int i;

	if ( (packet = AllocateWBRPReply()) != NIL ) 
	{
		/*-------------------------------------*/
		/* Send a WBRP Reply for each entry in */
		/* the Repeating Table                 */
		/*-------------------------------------*/

		for (i=0; i<MAX_REPEATERS; i++) 
		{
			if ((RepeatingTable[i].state == REPEATING_ENTRY_USED) &&
				 (RepeatingTable[i].primaryRoute.hopsToRepeater != NO_ROUTE))
			{
				OutChar('C', RED+WHITE_BACK);

				AddEntryToWBRPReply(packet, &RepeatingTable[i]);
		   }
		}
		SendWBRPReply(packet, destination);
	}
	return;
}


/***************************************************************************/
/*                                                                         */
/*  AllocateWBRPReply ()                                                   */
/*                                                                         */
/*  INPUT:                                                                 */
/*                                                                         */
/*  OUTPUT:       Pointer to WBRP packet structure.                        */
/*                                                                         */
/*  DESCRIPTION:  This procedure will get a WBRP packet buffer from a WBRP */
/*						packet buffer poll and initialize its state to 				*/
/*						WBRP_REPLY.  The length field is initialized to the size */
/*						of the packet without the Repeating Information Entries. */   
/*                                                                         */
/***************************************************************************/

struct WBRPPacket *AllocateWBRPReply (void)
{
	struct WBRPPacket *pkt;

	if ( (pkt=(struct WBRPPacket *) GetAWBRPBuffer ()) == NIL )
	{
		return NIL;
	}
	pkt->length = E8023HEADERLEN + sizeof(pkt->operation) + 
							sizeof (pkt->numofentries);

	pkt->operation = WBRP_REPLY;
	pkt->numofentries = 0;
	return (pkt);
}


/***************************************************************************/
/*                                                                         */
/*  AllocateWBRPRequest ()                                                 */
/*                                                                         */
/*  INPUT:                                                                 */
/*                                                                         */
/*  OUTPUT:       Pointer to WBRP packet structure.                        */
/*                                                                         */
/*  DESCRIPTION:  This procedure will get a WBRP packet buffer from a WBRP */
/*						packet buffer poll and initialize its state to 				*/
/*						WBRP_REQUEST.  The length field is initialized to the 	*/
/*						size of the packet without the Repeating Information 		*/
/*						Entries. 																*/   
/*                                                                         */
/***************************************************************************/

struct WBRPPacket * AllocateWBRPRequest (void)
{
	struct WBRPPacket *pkt;

	if ( (pkt = (struct WBRPPacket *) GetAWBRPBuffer ()) == NIL )
		return NIL;

	pkt->length = E8023HEADERLEN + 
							sizeof(pkt->operation) + sizeof (pkt->numofentries);

	pkt->operation = WBRP_REQUEST;
	pkt->numofentries = 0;
	return (pkt);
}


/***************************************************************************/
/*                                                                         */
/*  AddEntryToWBRPReply (*packet, *entry)                                  */
/*                                                                         */
/*  INPUT:        packet - pointer to a WBRP reply packet                  */
/*                entry  - pointer to the Repeating Table Entry            */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will add an entry, which was retrived 		*/
/*                from the repeating table, to the WBRP REPLY packet.		*/
/*                                                                         */
/***************************************************************************/

void AddEntryToWBRPReply ( struct WBRPPacket *packet,
									struct RepeatingTableEntry *entry)
{
	struct RepeaterEntry *pktEntry;
	unsigned_16           numEntries;
	FLAGS						 oldflag;


#ifdef SERIAL
	DisableRTS();
#endif
	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*----------------------------------------*/
	/* WBRP Reply Packet header Length =      */
	/*														*/
	/*    Destination address	(6) 	+	      */
	/*    Source address			(6)	+	      */
	/*    Length					(2)   +      	*/
	/*    Operation				(2)	+			*/
	/*		Numofentries			(1)	+			*/
	/*    1st RepeaterEntry		(14)  +	      */
	/*    2nd RepeaterEntry		(14)	+			*/
	/*    ...                                 */
	/*                               +	      */
	/*    Last RepeaterEntry	(14)				*/
	/*----------------------------------------*/

	numEntries = 0;

	/*----------------------------------------------*/
	/* Ensure we don't send conflicting information */
	/* by going through all packet entries that were*/
	/* already in the WBRP Reply packet.				*/
	/*----------------------------------------------*/

	for (pktEntry=&packet->entry[0];numEntries<packet->numofentries;
															numEntries++,pktEntry++) 
	{
		if (!memcmp(entry->repeaterAddress,
									pktEntry->repeaterAddress,ADDRESSLENGTH))
			break;      /* found an matching entry? */
	}

	if (pktEntry == &packet->entry[numEntries]) 
	{
		/*--------------------------------------*/
		/* Its a new entry, add to the repeater */
		/* table.                               */
		/*--------------------------------------*/
		packet->length += sizeof(struct RepeaterEntry);
		packet->numofentries++;
	}

	memcpy(pktEntry->repeaterAddress, entry->repeaterAddress, ADDRESSLENGTH);
	pktEntry->numberOfHops = entry->primaryRoute.hopsToRepeater;
	pktEntry->routingMetric = entry->primaryRoute.routingMetric;
	memcpy(pktEntry->nextHopRepeater, entry->primaryRoute.nextHopRepeater, 
																				ADDRESSLENGTH);
	RestoreFlag (oldflag);
#ifdef SERIAL
	EnableRST();
#endif

	return;
}



/***************************************************************************/
/*                                                                         */
/*  SendWBRPReply (*packet, *destination)                                  */
/*                                                                         */
/*  INPUT:        packet      - pointer to a WBRP reply packet             */
/*                destination - address of destination node                */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will send a WBRP Reply packet using 		*/
/*						PacketizeSend() routine to the destination address only  */
/*						if there is at least one repeating entry in the packet.	*/
/*						This is the routine that passes Repeating Information		*/
/*						between the Wireless Backbone APs.								*/						
/*                                                                         */
/***************************************************************************/

void SendWBRPReply (struct WBRPPacket *packet, unsigned_8 *destination)
{
	if ( packet->numofentries && 
						memcmp(destination, LLDNodeAddress, ADDRESSLENGTH) != 0 )
	{
		memcpy(packet->source, LLDNodeAddress, ADDRESSLENGTH);
		memcpy(packet->destination, destination, ADDRESSLENGTH);

		LLDTxWBRPPacket (packet);

		if ( destination[0] & 0x01 )
			WBRPStatus.numOfWBRPBcast++;
		else
			WBRPStatus.numOfWBRPTx++;
	}

	FreeAWBRPBuffer (packet);
}



/***************************************************************************/
/*                                                                         */
/*  SendWBRPRequest ()                                                     */
/*                                                                         */
/*  INPUT:                                                                 */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will prepare a WBRP request packet and    */
/*                to all repeaters in the Repeater Table that is aware of. */
/*                                                                         */
/***************************************************************************/

void SendWBRPRequest (void)
{
	struct WBRPPacket *packet;

	
	if ( (packet = AllocateWBRPRequest()) != NIL ) 
	{
		memcpy(packet->source, LLDNodeAddress, ADDRESSLENGTH);
		memcpy(packet->destination, BroadcastAddress, ADDRESSLENGTH);
		LLDTxWBRPPacket (packet);
		FreeAWBRPBuffer (packet);

		WBRPStatus.numOfWBRPBcast++;
	}

}


/***************************************************************************/
/*                                                                         */
/*  KillPrimaryRoute (*entry, *packet)                                     */
/*                                                                         */
/*  INPUT:        entry - entry of Repeating Table                         */
/*                packet- WBRP packet pointer                              */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will be called during LLD STOP call       */
/*                                                                         */
/***************************************************************************/

void KillPrimaryRoute (struct RepeatingTableEntry *entry, 
							  struct WBRPPacket *packet)
{
	unsigned_8 tmpHops;

	tmpHops = entry->primaryRoute.hopsToRepeater;
	entry->primaryRoute.hopsToRepeater = MAX_HOP_COUNT;
	if (packet)
	{
		OutChar('K', RED+WHITE_BACK);
		AddEntryToWBRPReply(packet, entry);
	}
	entry->primaryRoute.hopsToRepeater = tmpHops;
}



/***************************************************************************/
/*                                                                         */
/*  RepeaterShutdown ()                                                    */
/*                                                                         */
/*  INPUT:                                                                 */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  This procedure will be called during LLD STOP call       */
/*                                                                         */
/***************************************************************************/

void RepeaterShutdown (void)
{
	struct RepeatingTableEntry *entry;

	for (entry=&RepeatingTable[0];entry<&RepeatingTable[MAX_REPEATERS];entry++) 
	{
		if ((entry->state == REPEATING_ENTRY_USED) &&
				 (entry->primaryRoute.hopsToRepeater != NO_ROUTE))

			entry->primaryRoute.hopsToRepeater = MAX_HOP_COUNT;
	}

	SendCompleteWBRPReply (BroadcastAddress);
	WBRPStatus.numOfWBRPBcast++;
}


/*-------------------------------------------------------------------------*/
/*                                                                         */
/*                              Receiving State                            */
/*                                                                         */
/*-------------------------------------------------------------------------*/


/***************************************************************************/
/*                                                                         */
/*  ProcessWBRPPacket ()                                                   */
/*                                                                         */
/*  INPUT:        packet - Pointer to the receiving packet.                */                                                       
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  Upon receiving a WBRP packet, process it according to    */
/*                WBRP Reply or WBRP Request operation.                    */
/*                                                                         */
/***************************************************************************/

void ProcessWBRPPacket (struct WBRPPacket *packet)
{
	WBRPStatus.numOfWBRPRx++;

	switch(packet->operation) 
	{
		case WBRP_REQUEST:
				/* its a request */
				OutChar ('p', CYAN);
				OutChar ('q', CYAN);
				SendCompleteWBRPReply (packet->source);
				break;

		case WBRP_REPLY:
				ProcessWBRPReply (packet->source,
										&packet->entry[0],
										packet->numofentries);
				break;
		default:
				assert(0);
	}
}


/***************************************************************************/
/*                                                                         */
/*  ProcessWBRPReply (*source, *entries, numEntries)								*/
/*                                                                         */
/*  INPUT:        source - source address                                  */
/*                entries - pointer to 1st entries of Repeating Table      */
/*                numentries - number of entries in repeating table        */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  Upon receiving a WBRP packet, process it according to    */
/*                WBRP Reply or WBRP Request operation.                    */
/*                                                                         */
/***************************************************************************/

void ProcessWBRPReply (unsigned_8           *source, 
							  struct RepeaterEntry *entries,
							  int numEntries)
{
	struct RepeatingTableEntry *tableEntry;
	struct RepeatingTableEntry *sourceEntry;
	unsigned_8                 metric;
	unsigned_8                 newTSF;
	unsigned_8                 hopChange;
	struct RepeatingInfo       repeatingInfo;
	int                        i;
	struct WBRPPacket          *packet = NIL;


	OutChar ('p', CYAN);
	OutChar ('r', CYAN);

	/*---------------------------------------------*/
	/* Increase the TSF associated with the source */
	/* address by setting newTSF.  If the source   */
	/* address is not in the Neighbor Table,       */
	/* initialize it to INITIAL_TSF.               */
	/*---------------------------------------------*/

	if ( (sourceEntry = FindNeighborTableEntry (source)) == NIL ) 
		sourceEntry = AddNeighborTableEntry(source, INITIAL_TSF);
	else 
	{
		/* Increase the tsf */
		newTSF = CalculateNewTSF(sourceEntry->tsf, 100);
		if (newTSF != sourceEntry->tsf) 
		{
			UpdateRoutingMetrics(sourceEntry->tsf, newTSF,
												sourceEntry->repeaterAddress, &packet);
			sourceEntry->tsf = sourceEntry->newTSF = newTSF;
		}
	}
	if (!sourceEntry)
	{
		return;
	}

	sourceEntry->neighborAge = 0;

	if (!packet && (packet = AllocateWBRPReply()) == NIL )
	{
			return;
	}

	/*------------------------------------*/
	/*       Process each entry           */
	/*------------------------------------*/

	for (i=0; i<numEntries; i++) 
	{
		hopChange = FALSE;

		/*----------------------------------------------*/
		/* Ignore entry if nextHopRepeater or           */
		/* repeaterAddress matches my address           */
		/*----------------------------------------------*/

		if (memcmp(entries[i].nextHopRepeater,LLDNodeAddress,ADDRESSLENGTH) &&
			 memcmp(entries[i].repeaterAddress,LLDNodeAddress,ADDRESSLENGTH)) 
		{
			metric = CalculateMetric (sourceEntry->tsf,entries[i].routingMetric);
			tableEntry = FindRepeatingTableEntry(entries[i].repeaterAddress);

			if (entries[i].numberOfHops == MAX_HOP_COUNT) 
			{
				/*------------------------------*/
				/* This is a route kill message */
				/*------------------------------*/
				if (tableEntry) 
				{
					if ( !memcmp(source,tableEntry->primaryRoute.nextHopRepeater,
														ADDRESSLENGTH) )
					{
						/* Propagate the route kill */
						KillPrimaryRoute(tableEntry, packet);

						/* Is there a secondary route? */
						if (tableEntry->secondaryRoute.hopsToRepeater != NO_ROUTE) 
						{
							/* Promote secondary route to primary */
							PromoteSecondaryRoute(tableEntry);

							/*--------------------------------------------------*/
							/* Schedule next broadcast, so that secondary route */
							/* is advertised.                                   */
							/*--------------------------------------------------*/

							TicksSinceLastBroadcast = WBRP_TICKS_BETWEEN_BROADCASTS 
																- WBRP_SECONDARY_ROUTE_DELAY;
						}
						else 
						{
							RemoveRepeatingTableEntry(entries[i].repeaterAddress);
						}
					}
					else if (!memcmp(source,
									tableEntry->secondaryRoute.nextHopRepeater, 
																			ADDRESSLENGTH) ) 
					{
						/* Remove the invalidated secondary route */
						tableEntry->secondaryRoute.hopsToRepeater = NO_ROUTE;
					}
				}/*if*/
			}
			else if ( tableEntry == NIL ) 
			{
				AddRepeatingTableEntry(entries[i].repeaterAddress,
											  (unsigned_8)(entries[i].numberOfHops + 1), 
											  metric, source);

				if ( (tableEntry=FindRepeatingTableEntry(entries[i].repeaterAddress)) != NIL)

					if (tableEntry->primaryRoute.hopsToRepeater < MAX_HOP_COUNT)
					{
						AddEntryToWBRPReply(packet, tableEntry);
					}
			}
			else 
			{	/*--------------------------------------------------*/
				/* Is source our current primary next-hop repeater? */
				/*--------------------------------------------------*/
				if (!memcmp(source, tableEntry->primaryRoute.nextHopRepeater, 
																			ADDRESSLENGTH)) 
				{	/*------------------------*/
					/* Update routing metric  */
					/*------------------------*/
					tableEntry->primaryRoute.agingTimer = 0;
					tableEntry->primaryRoute.routingMetric = metric;
						
					/*--------------------------------------*/
					/* Update number of hops if it changes  */
					/*--------------------------------------*/
					if (tableEntry->primaryRoute.hopsToRepeater != (entries[i].numberOfHops + 1)) 
					{
						hopChange = TRUE;
						tableEntry->primaryRoute.hopsToRepeater =(unsigned_8) (entries[i].numberOfHops + 1);
					}
					/*----------------------------------------------*/
					/* Do we want to switch to the secondary route? */
					/*----------------------------------------------*/
					if (RouteSwitch(&tableEntry->primaryRoute, &tableEntry->secondaryRoute)) 
					{
						hopChange = FALSE;
						KillPrimaryRoute(tableEntry, packet);

						/*----------------------------------------*/
						/* Swap the primary and secondary routes  */
						/*----------------------------------------*/
						SwapRoutes(tableEntry);	

						/*-------------------------*/
						/* Schedule next broadcast */
						/*-------------------------*/
						TicksSinceLastBroadcast =
							WBRP_TICKS_BETWEEN_BROADCASTS - WBRP_SECONDARY_ROUTE_DELAY;
					}
					if (hopChange)
					{
						OutChar('H', RED+WHITE_BACK);
						AddEntryToWBRPReply(packet, tableEntry);
					}
				}

				/*----------------------------------------------------*/
				/* Is source our current secondary next-hop repeater? */
				/*----------------------------------------------------*/
				else if (!memcmp(source,tableEntry->secondaryRoute.nextHopRepeater, ADDRESSLENGTH)) 
				{	
					/* Update routing metrics */
					tableEntry->secondaryRoute.agingTimer = 0;
					tableEntry->secondaryRoute.routingMetric = metric;
					tableEntry->secondaryRoute.hopsToRepeater = (unsigned_8)(entries[i].numberOfHops + 1);

					/* Do we want to switch to the secondary route? */
					if (RouteSwitch (&tableEntry->primaryRoute, &tableEntry->secondaryRoute) ) 
					{
						/* Kill the primary route */
						KillPrimaryRoute(tableEntry, packet);
						SwapRoutes(tableEntry);
						TicksSinceLastBroadcast =
							WBRP_TICKS_BETWEEN_BROADCASTS - WBRP_SECONDARY_ROUTE_DELAY;
					}
				}
				else 
				{  /*-------------------------------------------------------*/
					/* At this point, we know that the source is not the     */
					/* next-hop repeater for either the primary or secondary */
					/* route.  We also know it is not a route kill           */
					/* (i.e., numberOfHops == MAX_HOP_COUNT).                */
					/* Put the new info into a temporary entry.              */
					/*-------------------------------------------------------*/

					repeatingInfo.hopsToRepeater = (unsigned_8)(entries[i].numberOfHops + 1);
					repeatingInfo.routingMetric = metric;
					memcpy(&repeatingInfo.nextHopRepeater, source, ADDRESSLENGTH);
					repeatingInfo.agingTimer = 0;

					/*--------------------------------------*/
					/* Should we replace the primary route? */
					/*--------------------------------------*/
					if ( RouteSwitch (&tableEntry->primaryRoute, &repeatingInfo) ) 
					{
						KillPrimaryRoute(tableEntry, packet);
						DemotePrimaryRoute(tableEntry);

						/* Copy in the new primary route */
						memcpy(&tableEntry->primaryRoute,
									  &repeatingInfo, sizeof(struct RepeatingInfo));

						/* Schedule broadcast of new route. */
						TicksSinceLastBroadcast = WBRP_TICKS_BETWEEN_BROADCASTS 
																- WBRP_SECONDARY_ROUTE_DELAY;
					}
					/*----------------------------------------*/
					/* Should we replace the secondary route? */
					/*----------------------------------------*/
					else if (RouteSwitch(&tableEntry->secondaryRoute, &repeatingInfo)) 
					{
						/*-------------------------------------------------*/
						/* No route kills are necessary, since we aren't   */
						/* advertising the secondary route.                */
						/*-------------------------------------------------*/

						memcpy(&tableEntry->secondaryRoute,&repeatingInfo, 
											sizeof(struct RepeatingInfo));
					}
				}
			}
		}
	}

	SendWBRPReply(packet, BroadcastAddress);

}


/***************************************************************************/
/*                                                                         */
/*  ProcessDataPacket (*packet)                                            */
/*                                                                         */
/*  INPUT:        packet - pointer to the LLDRepeating Packet              */
/*                                                                         */
/*  OUTPUT:       0 - don't pass it up.												*/
/*						1 - Pass it up.														*/                                                         
/*                                                                         */
/*  ALGORITHMS:   Data packets are send through the wireless backbone by   */
/*                setting both of the LLDREPEATBIT and LLDWBRPBIT in LLD   */
/*                Header2.  A repeating destination and source address     */
/*                will be included in the packet.  This pair of addresses  */
/*                is used by the firmware to reliably deliver across a     */
/*                single hop.  Each time a packet is repeated, the repeater*/
/*                sets those two bits and  decrements the Hops Left field, */
/*                and fills in the repeating destination and repeating     */
/*                source addresses.  All fields from the destination       */
/*                address to the end of the LLD Packet are passed through  */
/*                the repeaters transparently.                             */                
/*                                                                         */
/*  STRUCTURES:	Packet structures -													*/
/*                                                                         */
/*								LLDHeader														*/
/*								RPDest	- Repeating Destination (the node that    */
/*											  will repeate the packet).					*/
/*								RPSource - LLDNodeAddress									*/
/*								. . .																*/
/*                      destination - Original(Final) destination          */
/*								source		- Original source	(not the repeater)	*/
/*                      . . .                                              */
/*                                                                         */
/*  DESCRIPTION:  This routine handles a Repeated Data Packet during       */
/*                receiving process.                                       */
/*                                                                         */
/***************************************************************************/

unsigned_8 ProcessDataPacket (struct LLDRepeatingPacket *packet)
{
	struct RepeatingTableEntry *entry;
	struct LLDRepeatingPacket  *newPacket;
	unsigned_16                 length;

	OutChar ('p', CYAN);
	OutChar ('d', CYAN);

	WBRPStatus.numOfDataRx++;

	/*-------------------------------------------*/
	/* Case 1:                                   */
	/*    The Destination Address is the address */
	/*    of the Repeater which received the     */
	/*    packet.  The packet is FORWARDED UP   */
	/*    the stack and no further action is     */
	/*    taken.                                 */
	/*-------------------------------------------*/

	if (!memcmp(packet->RPHeader.RPDest,packet->destination,ADDRESSLENGTH) &&
		  memcmp(packet->destination, BroadcastAddress, ADDRESSLENGTH)) 
	{
		return PASSITUP;
	}

	/*-------------------------------------------*/
	/* Case 2:                                   */
	/*    The Destination Address is a broadcast */
	/*    or multicast address and the Repeater  */
	/*    is on my route to the original source. */
	/*		then repeat the broadcast, or else     */
	/*    just pass it up.								*/
	/*-------------------------------------------*/

	else if ( !memcmp(packet->destination, BroadcastAddress, ADDRESSLENGTH)) 
	{
		if ( (entry = FindRepeatingTableEntry ( packet->source )) != NIL ) 
		{	
			/*-----------------------------------------------*/
			/* Is there a pass from the Original SOURCE to   */
			/* this node? 												 */
			/* 			    source <-- destinatin  	  		 */
			/*																 */
			/* If the Repeating Source Address matches the   */
			/* Next-Hop Repeater in this entry, the packet is*/
			/* forwarded up the stack.                       */
			/*		RPSource == source->NextHopRepeater			 */
			/*																 */
			/*-----------------------------------------------*/
			if ( !memcmp(packet->RPHeader.RPSource,
								entry->primaryRoute.nextHopRepeater, ADDRESSLENGTH) )  
			{
				if (packet->RPHeader.hopsLeft) 
				{
					for (entry=&RepeatingTable[0]; entry<&RepeatingTable[MAX_REPEATERS]; entry++) 
					{
						/*-------------------------------------------*/
						/* If there is at least one entry in the     */
						/* Neighbor table with a Neighboring Repeater*/
						/* Address other than the Source Repeating   */
						/* Address, decrement hopsleft and puts its  */
						/* own address in the Source Repeating addr. */
						/* field, and RE_BROADCASTS the packet.      */
						/*-------------------------------------------*/
						if (entry->neighbor && memcmp(packet->RPHeader.RPSource,
												entry->repeaterAddress, ADDRESSLENGTH)) 
						{
							/*-------------------------------------------------*/
							/* packet->RPHeader.LLDHeader.PTPktLength[0] (MSB)	*/
							/*													 [1] (LSB)  */
							/* 		LLD Header1 (1) 									*/
							/*		 +	LLD Header2 (1) 									*/
							/*		 +	LLD Header3 (1) 									*/
							/*		 +	LLD Header4 (1) 									*/
							/*		 + Repeating Destination Address (6)			*/
							/*		 + Repeating Source Address (6)					*/
							/*		 + HopsLeft (1)										*/
							/*		 + Reserved (1)										*/
							/*		 +	Destination Address (6)				 			*/
							/*		 +	Source Address (6)					 			*/
							/*		 +	Immediate Data Length (???)		 			*/
							/*																   */
							/* length = entire packet length with the 			*/
							/* 			repeating information, include 			*/
							/*				packetize header.								*/
							/*-------------------------------------------------*/
							
							length = (unsigned_16) (packet->RPHeader.LLDHeader.PTPktLength[0] << 8);
							length += packet->RPHeader.LLDHeader.PTPktLength[1];
							length -= 4;							/* LLD Header Length */							
							length += sizeof (struct PacketizeTx);

							newPacket = (struct LLDRepeatingPacket *) &RepeatingPacket;

							memcpy(newPacket, packet, length); 
							--newPacket->RPHeader.hopsLeft;
							memcpy(newPacket->RPHeader.RPSource, LLDNodeAddress, ADDRESSLENGTH);
							memcpy(newPacket->RPHeader.RPDest,BroadcastAddress,ADDRESSLENGTH);

							LLDTxRepeatDataPacket (newPacket, length);
							WBRPStatus.numOfRepeatPkt++;

							return PASSITUP;
						}
					}/* for */
				}/*if HopsLeft > 0*/
			}/*if*/

			/*--------------------------------------------*/
			/* Received a Broadcast packet from someone   */
			/* that is not on my route to the original    */
			/* source, then don't pass it up.				 */
			/*--------------------------------------------*/
			else 
				return DONOT_PASSITUP;
		}/*if*/

		return PASSITUP;

	}/*else if*/

	/*----------------------------------------------------*/
	/* Case 3:                                            */
	/*    The Destination Address is not the address of   */
	/*    the Repeater and is not broadcast or multicast. */
	/*																		*/
	/*	Action:															*/
	/*    Repeat it but don't pass it up.						*/
	/*----------------------------------------------------*/

	else 
	{
		if ( (packet->RPHeader.hopsLeft) != 0 ) 
		{
			if ( (entry = FindRepeatingTableEntry(packet->destination)) != NIL ) 
			{
				/*-------------------------------------------------*/
				/* packet->RPHeader.LLDHeader.PTPktLength[0] (MSB)	*/
				/*													 [1] (LSB)  */
				/* 		LLD Header1 (1) 									*/
				/*		 +	LLD Header2 (1) 									*/
				/*		 +	LLD Header3 (1) 									*/
				/*		 +	LLD Header4 (1) 									*/
				/*		 + Repeating Destination Address (6)			*/
				/*		 + Repeating Source Address (6)					*/
				/*		 + HopsLeft (1)										*/
				/*		 + Reserved (1)										*/
				/*		 +	Destination Address (6)				 			*/
				/*		 +	Source Address (6)					 			*/
				/*		 +	Immediate Data Length (???)		 			*/
				/*																   */
				/* length = entire packet length with the 			*/
				/* 			repeating information, include 			*/
				/*				packetize header.								*/
				/*-------------------------------------------------*/
				
				length = (unsigned_16) (packet->RPHeader.LLDHeader.PTPktLength[0] << 8);
				length += packet->RPHeader.LLDHeader.PTPktLength[1];
				length -= 4;							/* LLD Header Length */							
				length += sizeof (struct PacketizeTx);

				newPacket = (struct LLDRepeatingPacket *) &RepeatingPacket;

				memcpy(newPacket, packet, length);
				--newPacket->RPHeader.hopsLeft;

				memcpy(newPacket->RPHeader.RPSource, LLDNodeAddress, ADDRESSLENGTH);
				memcpy(newPacket->RPHeader.RPDest,
										entry->primaryRoute.nextHopRepeater, ADDRESSLENGTH);

				LLDTxRepeatDataPacket (newPacket, length);

				WBRPStatus.numOfRepeatPkt++;

				return DONOT_PASSITUP;
			}
		}
		return DONOT_PASSITUP;
	}

}



/***************************************************************************/
/*                                                                         */
/*  WBRPGetRepeaterAddr (*bufptr, *dest)                                   */
/*                                                                         */
/*  INPUT:        bufptr - pointer to a buffer for storing repeating dest. */
/*									and repeating src address.								*/
/*						dest	 - pointer to the destination address.					*/
/*                                                                         */
/*  OUTPUT:       0 - didn't find an entry for the destination address.		*/
/*						1 - found it and store the addresses in bufptr.				*/
/*						2 - its a broadcast													*/
/*                                                                         */
/*  DESCRIPTION:  This routine will look into the Repeating Table to get   */
/*                the Repeating Destination address and Repeating Source   */
/*                Address and hops left for the source.                    */
/*                                                                         */
/***************************************************************************/

unsigned_8 WBRPGetRepeaterAddr ( unsigned_8 *bufptr, unsigned_8 *dest )

{
	struct RepeatingTableEntry *entry;


	if ( !memcmp (dest, BroadcastAddress, ADDRESSLENGTH) )
	{
		memcpy (bufptr, BroadcastAddress, ADDRESSLENGTH);
		bufptr += ADDRESSLENGTH;
		memcpy (bufptr, LLDNodeAddress, ADDRESSLENGTH);
		bufptr += ADDRESSLENGTH;
		*bufptr++ = MAX_HOP_COUNT;		/* Hops left */
		*bufptr++ = 0;						/* reserved  */

		return (1);
	}

	else	if ( (entry = FindRepeatingTableEntry (dest)) != NIL )
	{
		/*----------------------------------------------*/
		/* use the nextHopRepeater of the primary route */
		/* as the Repeating Destination Address.        */
		/*----------------------------------------------*/
		memcpy (bufptr, entry->primaryRoute.nextHopRepeater, ADDRESSLENGTH);
		bufptr += ADDRESSLENGTH;
		memcpy (bufptr, LLDNodeAddress, ADDRESSLENGTH);
		bufptr += ADDRESSLENGTH;
		*bufptr++ = entry->primaryRoute.hopsToRepeater;
		*bufptr++ = 0;						/* reserved */

		return (1);
	}

	else
		return (0);

}



/***************************************************************************/
/*                                                                         */
/*  CalculateMetric (tsf, metric)                                          */
/*                                                                         */
/*  INPUT:        tsf - Transmit Success Factor                            */
/*                metric - link metric                                     */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:  A new metric is calculated usiing the Metric received in */
/*						the packet from the previous repeater and the TSF 			*/
/*						information for that repeater in the Neighbor Table.		*/
/*                                                                         */
/***************************************************************************/

unsigned_8 CalculateMetric (unsigned_8 tsf, unsigned_8 metric)
{
	int result;

	if ( (result = (int) tsf * metric / 100) == 0 )
		result = 1;
	return ( (unsigned_8) (result & 0xff));
}


/***************************************************************************/
/*                                                                         */
/*  CalculateNewTSF (averageTsf, newTsf)                                   */
/*                                                                         */
/*  INPUT:        averageTsf - average TSF                                 */
/*                newTsf     - new TSF                                     */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:	The TSF is recalculated every time a message is sent to  */
/*						a neighboring node.													*/ 
/*                                                                         */
/***************************************************************************/

unsigned_8 CalculateNewTSF (unsigned_8 averageTsf, unsigned_8 newTsf)
{
	int result;

	result = (int) newTsf * TSF_WEIGHT + (int) averageTsf * (100-TSF_WEIGHT);
	return ( (unsigned_8) (result/100 & 0xff) );
}


/***************************************************************************/
/*                                                                         */
/*  CalculateTSF (PacketSize, Status, TxDelay)                             */
/*                                                                         */
/*  INPUT :       PacketSize - 															*/
/*						Status -																	*/
/*						TxDelay -																*/
/*                                                                         */
/*  OUTPUT:       tsf                                                      */
/*                                                                         */
/*  DESCRIPTION :	The TSF is recalculated every time a message is sent to  */
/*						a neighboring node according to the following algorithm: */
/*																									*/ 
/*						- MaxDelay = MAX_ACCEPTABLE_DELAY * (packetsize/100 + 1)	*/
/*                                                                         */
/*						- If the message results in CTS or ACK error, then   		*/
/*								tsf = 0;															*/
/*                                                                         */
/*					  	  else if (TxDelay > MaxDelay ) then 							*/
/*								tsf = 0															*/
/*								TxDelay = the delay in PC ticks between the Send   */
/*									       and the Transmit Complete						*/
/*						  else																	*/
/*								tsf = 100 - (TxDelay * 100/MaxDelay)					*/
/*                                                                         */
/***************************************************************************/

unsigned_8 CalculateTSF (	unsigned_16 PacketSize, 
									unsigned_8 Status,
								  	unsigned_16 TxDelay)
{
	unsigned_16	MaxDelay;
	unsigned_8  tsf;

	
	MaxDelay = MAX_ACCEPTABLE_DELAY * (PacketSize / 100 + 1);

	if ( Status || (TxDelay > MaxDelay) )
	{
		tsf = 0;
	}
	else
	{
		tsf = (unsigned_8) ( 100 - (TxDelay * 100 / MaxDelay) );
	}
		
	return (tsf);
}
		

/***************************************************************************/
/*                                                                         */
/*  WBRPUpdateTSF (*TxPacket, starttime) 												*/
/*                                                                         */
/*  INPUT:       TxPacket  - Transmitted packet descriptor						*/
/*               starttime -																*/
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:	The TSF is recalculated every time a message is sent to  */
/*						a neighboring node.													*/ 
/*                                                                         */
/***************************************************************************/

void WBRPUpdateTSF ( struct LLDRepeatingPacket *TxPacket,
							unsigned_16						starttime)

{
	struct RepeatingTableEntry *entry;
	unsigned_16						 PacketSize;
	unsigned_8						 tsf;


	/*----------------------------------------------------*/
	/* Get the entry of the Node who has a neighbor that  */
	/* is this destination 
	/* repeating table.												*/
	/*----------------------------------------------------*/
	OutChar ('U', YELLOW);
	OutChar ('p', YELLOW);
	OutChar ('d', YELLOW);
	OutChar ('T', YELLOW);
	OutChar ('S', YELLOW);
	OutChar ('F', YELLOW);

	for (entry = &RepeatingTable[0]; entry < &RepeatingTable[MAX_REPEATERS]; 
																							entry++) 
	{
		if ( (entry->state == REPEATING_ENTRY_USED) && (entry->neighbor) )
		{
			if ( !memcmp (TxPacket->RPHeader.RPDest, entry->repeaterAddress,
																		ADDRESSLENGTH) )
			{
				/*----------------------------------------*/
				/* Found it, update this repeater's TSF	*/
				/*----------------------------------------*/

				PacketSize = (unsigned_16) (TxPacket->RPHeader.LLDHeader.PTPktLength[0] << 8);
				PacketSize += (unsigned_16) (TxPacket->RPHeader.LLDHeader.PTPktLength[1]);

				tsf = CalculateTSF (	PacketSize, 1,
													LLSGetCurrentTime() - starttime);
				entry->newTSF = CalculateNewTSF (entry->tsf, tsf);
			}
		}
	}
			
}



/***************************************************************************/
/*                                                                         */
/*  UpdateRoutingMetrics (oldTSF, newTSF, *nextHopRepeater, **packet)      */
/*                                                                         */
/*  INPUT:	       oldTSF - old TSF                                         */
/*                newTsf - new TSF                                         */
/*                nextHopRepeater -                                        */
/*                packet - Pointer to pointer of WBRP packet buffer        */
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION: 																				*/
/*                                                                         */
/***************************************************************************/

void UpdateRoutingMetrics (unsigned_8 oldTSF, 
									unsigned_8 newTSF, 
									unsigned_8 *nextHopRepeater,
									struct WBRPPacket **packet)
{
	struct RepeatingTableEntry *entry;

	for (entry=&RepeatingTable[0]; entry<&RepeatingTable[MAX_REPEATERS]; entry++) 
	{
		if (entry->state == REPEATING_ENTRY_USED) 
		{
			if (entry->primaryRoute.routingMetric != NO_ROUTE) 
			{
				if ( !memcmp(entry->primaryRoute.nextHopRepeater, nextHopRepeater, ADDRESSLENGTH)) 
				{
					entry->primaryRoute.routingMetric = (unsigned_8)
					(((unsigned_16) entry->primaryRoute.routingMetric * newTSF / oldTSF) & 0xff);
				}
				else if (entry->secondaryRoute.routingMetric != NO_ROUTE) 
				{
					if (!memcmp(entry->secondaryRoute.nextHopRepeater, nextHopRepeater, ADDRESSLENGTH)) 
					{
						entry->secondaryRoute.routingMetric = (unsigned_8)
							(((unsigned_16) entry->secondaryRoute.routingMetric * newTSF / oldTSF) & 0xff);
					}
				}

				/*------------------------------------------------*/
				/* Should we switch primary and secondary routes? */
				/*------------------------------------------------*/

				if (RouteSwitch(&entry->primaryRoute, &entry->secondaryRoute)) 
				{
					if (*packet == NIL)
						*packet = AllocateWBRPReply();

					/* Kill the primary route */
					KillPrimaryRoute(entry, *packet);
					SwapRoutes(entry);
				}
			}/*if*/
		}/*if*/
	}/*for*/
}


/***************************************************************************/
/*                                                                         */
/*  RouteSwitch (*primaryRoute, *secondaryRoute)                           */
/*                                                                         */
/*  INPUT:        primaryRoute -                                           */
/*                secondaryRoute -                                         */
/*                                                                         */
/*  OUTPUT:       TRUE / FALSE                                             */                                                       
/*                                                                         */
/*  ALGORITHM:    A secondary route is considered significantly better if  */
/*						it has fewer hops than the primary route and has a metric*/
/*						value greater than MINIMUM_ROUTING_METRIC. It will also  */
/*						be considered significantly better if it has the same    */
/*                number of hops, but has a metric more than METRIC_BASED_ */
/*						SWITCHING_THRESHOLD greater than the primary route.		*/
/*                                                                         */
/*  DESCRIPTION:  Decides if we should swith the primary route with the    */
/*						secondary route, if the secondary route seems better.		*/
/*																									*/
/***************************************************************************/

unsigned_8 RouteSwitch (struct RepeatingInfo *primaryRoute,
								struct RepeatingInfo *secondaryRoute)
{
	if (secondaryRoute->hopsToRepeater == NO_ROUTE)
		return (FALSE);
	else 
	{
		if ((primaryRoute->routingMetric < MINIMUM_ROUTING_METRIC) &&
			 (secondaryRoute->routingMetric >= MINIMUM_ROUTING_METRIC))
				return (TRUE);
		if (primaryRoute->hopsToRepeater == NO_ROUTE)
			return (TRUE);
		else if ((secondaryRoute->hopsToRepeater < primaryRoute->hopsToRepeater)
			&& (secondaryRoute->routingMetric >= MINIMUM_ROUTING_METRIC))
			return (TRUE);
		else if (secondaryRoute->hopsToRepeater == primaryRoute->hopsToRepeater)
			if ((int) secondaryRoute->routingMetric -
				 (int) primaryRoute->routingMetric > SWITCHING_THRESHOLD)
					return(TRUE);
	}
	return (FALSE);
}


/***************************************************************************/
/*                                                                         */
/*  WBRPTimerTick (timertickintrval)                                       */
/*                                                                         */
/*  INPUT:        timertickintrval - number of ticks since last time this  */
/*												 was called.									*/
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  ALGORITHMS:   Repeater Information Entry Aging:                        */
/*                When the Aging Timer reaches MAX_ENTRY_AGE seconds for   */
/*                any entry in the Repeating Information Table, the entry  */
/*                is deleted from the table.  A WBRP REPLY is broadcast    */
/*                immediately which contains the Repeater Address which is */
/*                aged out, MAX_HOP_COUNT as the number of hops, 0 as      */
/*                Metric, and Next-Hop Repeater from the deleted table     */
/*                entry.                                                   */
/*                                                                         */
/*  DESCRIPTION:  This routine will be called from the LLDPoll() routine.	*/
/*						The current time will be compared with the WBRPStartTime */
/*						to see if the timer interval is reached.  The Repeating  */
/*						Table will be updated by sending a WBRPReply to everyone.*/
/*                                                                         */
/***************************************************************************/

void WBRPTimerTick (unsigned_16 timertickintrval)
{
	struct WBRPPacket          *packet = NIL;
	struct RepeatingTableEntry *entry;
	unsigned_16                neighborAge;
	unsigned_16						divResult;
	unsigned_16						modResult;
	unsigned_8                 newtsf;


	/*-------------------------------------------*/
	/* Check for aged out repeating entries      */
	/*-------------------------------------------*/

	for (entry=&RepeatingTable[0];entry<&RepeatingTable[MAX_REPEATERS];entry++) 
	{
		if (entry->state == REPEATING_ENTRY_USED) 
		{
			if ( entry->primaryRoute.hopsToRepeater > 0 && 
						entry->primaryRoute.hopsToRepeater <= MAX_HOP_COUNT)
			{
				/*-----------------------------*/
				/* Debugging code only If this */
				/* assert kicks in, we missed a*/
				/* route switch                */
				/*-----------------------------*/

				assert(!RouteSwitch(&entry->primaryRoute,&entry->secondaryRoute));

				/*-----------------------------*/
				/* Age primary route           */
				/*-----------------------------*/

				entry->primaryRoute.agingTimer += timertickintrval;

				/*-----------------------------*/
				/* Age secondary route and     */
				/* remove it if necessary      */
				/*-----------------------------*/

				if (entry->secondaryRoute.hopsToRepeater != NO_ROUTE) 
				{
					entry->secondaryRoute.agingTimer += timertickintrval;
					if (entry->secondaryRoute.agingTimer >= MAX_ENTRY_AGE_TICKS) 
					{
						entry->secondaryRoute.hopsToRepeater = NO_ROUTE;
					}
				}
				if (entry->primaryRoute.agingTimer >= MAX_ENTRY_AGE_TICKS) 
				{
					if (!packet)
						packet = AllocateWBRPReply();

					/*-------------------------------*/
					/* Kill route                    */
					/*-------------------------------*/

					KillPrimaryRoute(entry, packet);

					/*-------------------------------*/
					/* If there is a secondary route,*/
					/* use it.                       */
					/*-------------------------------*/

					if (entry->secondaryRoute.hopsToRepeater != NO_ROUTE) 
					{
						PromoteSecondaryRoute(entry);
						TicksSinceLastBroadcast = WBRP_TICKS_BETWEEN_BROADCASTS - 
																	WBRP_SECONDARY_ROUTE_DELAY;
					}
					else 
					{
						RemoveRepeatingTableEntry(entry->repeaterAddress);
					}
				}
			}
		}/*if REPEATING_ENTRY_USED*/
	}/*for every entry */

	/*----------------------------------------------*/
	/* Check for aged out neighbor entries and TSF  */
	/* updates.                                     */
	/*----------------------------------------------*/

	for (entry=&RepeatingTable[0];entry<&RepeatingTable[MAX_REPEATERS];entry++) 
	{
		if (entry->state == REPEATING_ENTRY_USED) 
		{
			if (entry->neighbor) 
			{
				entry->neighborAge += timertickintrval;

				neighborAge = entry->neighborAge;

				if (neighborAge >= MAX_NEIGHBOR_AGE_TICKS) 
				{
					/* Age out entry */
					RemoveNeighborTableEntry(entry->repeaterAddress);
				}

				else
				{	
					/*----------------------------------------*/
					/* If the newtsf value is different than  */
					/* the average TSF value, then there must */
					/* have been a CTS or ACK error during 	*/
					/* transmittion, so we want to update the */
					/* routing metric based on the new TSF 	*/
					/* value.											*/
					/*----------------------------------------*/

					if ( entry->tsf != entry->newTSF )
					{
						OutChar ('T', CYAN);
						OutChar ('S', CYAN);
						OutChar ('F', CYAN);
						OutChar ('T', CYAN);
						OutChar ('x', CYAN);
						UpdateRoutingMetrics ( entry->tsf,newtsf,
													  entry->repeaterAddress,&packet);
						entry->newTSF = entry->tsf = newtsf;
					}

					/*----------------------------------------------------*/
					/*	For each missed broadcast, we want to ajust the    */
					/* TSF value. If the age is more than 1-1/2 Broadcast */
					/* interval, then we missed a broadcast.					*/
					/*																		*/
					/* 			|---|---|---|---|---|---|---|---|---|---|	*/
					/* divResult: 0   1   2   3   4   5   6   7   8   9 	*/
					/*																		*/
					/*	divResult%2==1	(odd)	  ^		 ^			^		  ^	*/
					/*																		*/
					/*	modResult: If divResult do land at the ODD interval*/
					/*				  we want to make sure only the first time*/
					/*            at that interval is important, so			*/
					/*				  (modResult < WBRP_AGING_PERIOD_TICKS)   */
					/*				  is checked.										*/
					/*																		*/
					/*----------------------------------------------------*/
					divResult = neighborAge / WBRP_HALF_BROADCAST_PERIOD;
					if (divResult > 2)
					{
						if ( (divResult % 2) == 1)
						{	
							/* if its odd number, then 1-1/2 
							 * broadcast period passed.
							 */
							modResult = neighborAge % WBRP_HALF_BROADCAST_PERIOD;
							if ( modResult < WBRP_AGING_PERIOD_TICKS )
							{
								/*------------------------------------*/
								/* Lower the TSF for each missed WBRP */
								/* broadcast.								  */
								/*------------------------------------*/
	 							newtsf = CalculateNewTSF(entry->tsf, (unsigned_8) 0);

 								if (newtsf != entry->tsf) 
 								{
									OutChar ('T', YELLOW);
									OutChar ('S', YELLOW);
									OutChar ('F', YELLOW);
									OutChar ('M', YELLOW);
									OutChar ('i', YELLOW);
									OutChar ('s', YELLOW);
									OutChar ('s', YELLOW);

	 								UpdateRoutingMetrics ( entry->tsf,
																  newtsf,
 															     entry->repeaterAddress,
																  &packet );
									entry->newTSF = entry->tsf = newtsf;
								}
 							}
						}/*if its odd*/
					}
				}/*else*/

			}/*if neighbor*/
		}/*if entry is used*/
	}/*for*/

	/*-----------------------------------------*/
	/* See if any repeaters have been aged out */
	/*-----------------------------------------*/
	if (packet)
		SendWBRPReply(packet, BroadcastAddress);


	/*-------------------------------------------*/
	/* Is it time for another periodic broadcast?*/
	/*-------------------------------------------*/

	TicksSinceLastBroadcast += timertickintrval;

	if (TicksSinceLastBroadcast >= WBRP_TICKS_BETWEEN_BROADCASTS) 
	{
		OutChar ('B', CYAN);
		OutChar ('C', CYAN);
		OutChar ('a', CYAN);
		OutChar ('s', CYAN);
		OutChar ('t', CYAN);

		SendCompleteWBRPReply ( BroadcastAddress );
		TicksSinceLastBroadcast = 0L;
	}
}


/****************************************************************************/
/*                                                                          */
/*             Buffer Management for handling the WBRP Packets              */
/*                                                                          */
/****************************************************************************/


/***************************************************************************/
/*                                                                         */
/*  InitWBRPBuffers ()																		*/
/*                                                                         */
/*  INPUT:	       																			*/
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:	Initialize buffer status.											*/
/*                                                                         */
/***************************************************************************/

void InitWBRPBuffers (void)
{
	unsigned_8  i;

	for (i = 0; i < WBRPBUFFERCNT; i++)
	{
		memset ( &(WBRPBuffers[i].packet), 0, sizeof (struct WBRPPacket) );
		WBRPBuffers[i].bufstatus = 0;
	}
}


/***************************************************************************/
/*                                                                         */
/*  GetAWBRPBuffer ()																		*/
/*                                                                         */
/*  INPUT: 	      																			*/
/*                                                                         */
/*  OUTPUT:       Pointer to a free WBRP buffer.									*/
/*                                                                         */
/*  DESCRIPTION:	Go through the buffer poll and return the pointer of a   */
/*						free buffer.															*/
/*                                                                         */
/***************************************************************************/

struct WBRPPacket *GetAWBRPBuffer (void)
{
	unsigned_8 i;

	for (i = 0; i < WBRPBUFFERCNT; i++)
	{
		if ( WBRPBuffers[i].bufstatus == 0 )
		{
			WBRPBuffers[i].bufstatus = 1;
			return &(WBRPBuffers[i].packet);
		}
	}
	return NIL;
}


/***************************************************************************/
/*                                                                         */
/*  FreeAWBRPBuffer (*packet)																*/
/*                                                                         */
/*  INPUT:        packet - Pointer to the WBRP buffer to be freed.			*/																  
/*                                                                         */
/*  OUTPUT:                                                                */
/*                                                                         */
/*  DESCRIPTION:	Finds the given buffer in the buffer poll and sets its 	*/
/*						status to FREE.														*/
/*                                                                         */
/***************************************************************************/

void FreeAWBRPBuffer (struct WBRPPacket *packet)
{
	unsigned_8 i;

	for (i=0; i < WBRPBUFFERCNT; i++)
	{
		if (WBRPBuffers[i].bufstatus && (packet == &(WBRPBuffers[i].packet)) )
		{
			WBRPBuffers[i].bufstatus = 0;
			return;
		}
	}
	return;
}
