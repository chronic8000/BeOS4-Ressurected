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
/*							(c) Copyright PROXIM, INC. 1995								*/
/*									All Rights Reserved										*/
/*																									*/
/***************************************************************************/

/***************************************************************************/
/*																									*/
/*	$Header:   J:\pvcs\archive\clld\rptable.c_v   1.5   16 Apr 1996 11:38:28   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\rptable.c_v  $										*/
/* 
/*    Rev 1.5   16 Apr 1996 11:38:28   JEAN
/* 
/*    Rev 1.4   06 Feb 1996 14:31:46   JEAN
/* Comment changes.
/* 
/*    Rev 1.3   31 Jan 1996 13:25:02   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.2   12 Oct 1995 13:57:00   JEAN
/* Added $Header:   J:\pvcs\archive\clld\rptable.c_v   1.5   16 Apr 1996 11:38:28   JEAN  $ PVCS keyword and fixed a compiler warning.
/* 
/*    Rev 1.1   16 Mar 1995 16:18:14   tracie
/* 
/*    Rev 1.0   22 Feb 1995 09:52:12   tracie
/* Initial revision.
/***************************************************************************/

#include <assert.h>
#include <memory.h>
#include <stdio.h>

/***************************************************************************/
/*																									*/
/*							Include files used by the LLD									*/
/*																									*/
/***************************************************************************/
#include "constant.h"
#include "lld.h"
#include "pstruct.h"
#include "asm.h"

#include "rptable.h"
#include "wbrp.h"


/***************************************************************************/
/*																									*/
/*                          Global Variables											*/
/*																									*/
/***************************************************************************/
unsigned_16 					WBRPHashTable[HASH_TABLE_SIZE];
struct RepeatingTableEntry RepeatingTable[MAX_REPEATERS];
int 								RepeatingFreeIndex;



/***************************************************************************/
/*                                                                      	*/
/*  RepeatingTableInit ()																	*/
/*                                                                      	*/
/*  INPUT:																						*/
/*                                                                      	*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This procedure will initialize the Wireless Backbone's	*/
/*						Repeating Information Table.										*/
/*                                                                      	*/
/***************************************************************************/

void RepeatingTableInit (void)
{
	int i;
	struct RepeatingTableEntry *entry;

	SetMemory (RepeatingTable,0,sizeof(RepeatingTable));

	entry = RepeatingTable;
	for (i=0; i < MAX_REPEATERS; i++,entry++) 
	{
		entry->state = REPEATING_ENTRY_FREE;
		entry->link = i+1;
	}
	RepeatingFreeIndex = 0;
	RepeatingTable[MAX_REPEATERS-1].link = NO_LINK;

   /* Initialize hash table */
   for (i=0 ; i < HASH_TABLE_SIZE ; i++)
      WBRPHashTable[i] = NO_LINK;

}


/***************************************************************************/
/*                                                                      	*/
/*  AddRepeatingTableEntry	(*repeaterAddress, hops, metric, nexHopRepeater)*/
/*                                                                      	*/
/*  INPUT:			repeaterAddress - Address of the repeater.					*/
/*						hops				 - number of hops to repeater.				*/
/*					   metric			 - link metric (1-255).							*/
/*						nextHopRepeater - the address of the next-hop repeater.  */
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This procedure will add an entry in the Repeating Table  */
/*						with the given information.										*/
/*																									*/
/***************************************************************************/

void AddRepeatingTableEntry (unsigned_8 *repeaterAddress, 
									  unsigned_8 hops, 
									  unsigned_8 metric,
									  unsigned_8 *nextHopRepeater)
{
	unsigned_16						*index;
	unsigned_16	 					newIndex;
	struct RepeatingTableEntry *entry;


	index = &WBRPHashTable [Hash(repeaterAddress)];

	while (*index != NO_LINK) 
	{
		entry = &RepeatingTable[*index];

		if (!memcmp (repeaterAddress, entry->repeaterAddress, ADDRESSLENGTH)) 
		{ 
			/*---------------------------------------*/
			/*	Entry must in for neighbor table only */
			/*---------------------------------------*/
			assert(!ENTRY_FOR_REPEATING(entry));
			assert(ENTRY_FOR_NEIGHBOR(entry));

			/*	Add repeating information */
			SetPrimaryRoute (entry, hops, metric, nextHopRepeater);
			entry->secondaryRoute.hopsToRepeater = NO_ROUTE;
			return;
		}
		index = &RepeatingTable[*index].link;
	}

	if ( (newIndex = GetRepeatingEntry()) != NO_LINK ) 
	{
		entry = &RepeatingTable[newIndex];

		CopyMemory(entry->repeaterAddress, repeaterAddress, ADDRESSLENGTH);
		SetPrimaryRoute(entry, hops, metric, nextHopRepeater);
		entry->secondaryRoute.hopsToRepeater = NO_ROUTE;
		entry->neighbor = FALSE;
		entry->tsf = 0;
		entry->newTSF = 0;

		/*	Add entry to Hash Table */
		*index = newIndex;
	}
	return;
}


/***************************************************************************/
/*                                                                      	*/
/*  Hash	(NodeID)																				*/
/*                                                                      	*/
/*  INPUT:			NodeID - Node Address												*/
/*																									*/
/*  OUTPUT:			Location of the node in the hash table							*/																			
/*                                        	                              */
/*  DESCRIPTION:	This routine will take a given node address and run		*/
/*						through a HASH algorithm to return the location of that	*/
/*						node address in the HASH table.									*/
/*																									*/
/***************************************************************************/

unsigned_16	Hash (unsigned_8 *NodeID)
{
   return ( ((*(unsigned_16 *)(NodeID+0)) ^ (*(unsigned_16 *)(NodeID+2)) ^
          (*(unsigned_16 *)(NodeID+4))) & HASH_TABLE_MASK);
	/*-------------------------------------------------------------------*/
	/*        			                   	^^^^^^^^^^^^^^^^^             */
	/* equivalent to a mod function but only for table size of  			*/
	/*	power of 2   																		*/
	/*-------------------------------------------------------------------*/
}


/***************************************************************************/
/*                                                                      	*/
/*  FindNeighborTableEntry	(*node)														*/
/*                                                                      	*/
/*  INPUT:			node - Node Address													*/
/*																									*/
/*  OUTPUT:			entry of the neighbor in the RepeatingTable					*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will look into the repeating table of the   */
/*						given node and returns its neighbor's entry address.		*/
/*																									*/
/***************************************************************************/

struct RepeatingTableEntry * FindNeighborTableEntry (unsigned_8 *node)
{
	unsigned_16						 index;
	struct RepeatingTableEntry *entry;

	index = WBRPHashTable[Hash(node)];

	while (index != NO_LINK) 
	{
		entry = &RepeatingTable[index];
		if (!memcmp(node, entry->repeaterAddress, ADDRESSLENGTH)) 
		{
			if (entry->neighbor)
				return(&RepeatingTable[index]);
			else
				return((struct RepeatingTableEntry *) NIL);
		}
		index = entry->link;
	}

	return((struct RepeatingTableEntry *) NIL); 
}



/***************************************************************************/
/*                                                                      	*/
/*  FindRepeatingTableEntry (*node)														*/
/*                                                                      	*/
/*  INPUT:			node - Node Address													*/
/*																									*/
/*  OUTPUT:			Location of the node in the hash table							*/																			
/*                                        	                              */
/*  DESCRIPTION:	This routine will take a given node address and returns	*/
/*						its entry address within the Hash table.						*/ 
/*																									*/
/***************************************************************************/

struct RepeatingTableEntry *FindRepeatingTableEntry (unsigned_8 *node)
{
	unsigned_16						 index;
	struct RepeatingTableEntry *entry;

	index = WBRPHashTable[Hash(node)];

	while (index != NO_LINK) 
	{
		entry = &RepeatingTable[index];
		if (!memcmp(node, entry->repeaterAddress, ADDRESSLENGTH)) 
		{
			if (ENTRY_FOR_REPEATING(entry))
				return(&RepeatingTable[index]);
			else
				/*	Entry must only be a Neighbor entry */
				return((struct RepeatingTableEntry *) NIL);
		}
		index = entry->link;
	}

	/*	Entry not found */
	return((struct RepeatingTableEntry *) NIL);
}


/***************************************************************************/
/*                                                                      	*/
/*  AddNeighborTableEntry (*repeaterAddress, tsf)									*/
/*                                                                      	*/
/*  INPUT:			repeaterAddress - address of the repeater						*/
/*						tsf				 - Transmit Success Factor						*/
/*																									*/
/*  OUTPUT:			Entry address															*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will add a neighbor entry into the repeater */
/*						information table.													*/
/*																									*/
/***************************************************************************/

struct RepeatingTableEntry *AddNeighborTableEntry (unsigned_8 *repeaterAddress, 
																  unsigned_8 tsf)
{
	unsigned_16 					*index;
	unsigned_16 					 newIndex;
	struct RepeatingTableEntry *entry = NIL;

	OutChar ('N', MAGENTA);
	OutChar ('e', MAGENTA);
	OutChar ('i', MAGENTA);
	OutChar ('g', MAGENTA);
	OutChar ('h', MAGENTA);
	OutChar ('b', MAGENTA);
	OutChar ('o', MAGENTA);
	OutChar ('r', MAGENTA);

	index = &WBRPHashTable[Hash(repeaterAddress)];

	while (*index != NO_LINK) 
	{
		entry = &RepeatingTable[*index];

		if (!memcmp(repeaterAddress, entry->repeaterAddress, ADDRESSLENGTH)) 
		{
			/*	Entry must in for repeating table only */
			assert(ENTRY_FOR_REPEATING(entry));
			assert(!ENTRY_FOR_NEIGHBOR(entry));

			/*	Add neighbor information */
			entry->neighbor = TRUE;
			entry->tsf = tsf;
			entry->newTSF = tsf;
			entry->neighborAge = 0;
			return entry;
		}
		index = &RepeatingTable[*index].link;
	}

	if (((newIndex = GetRepeatingEntry()) != NO_LINK)) 
	{
		entry = &RepeatingTable[newIndex];

		CopyMemory(entry->repeaterAddress, repeaterAddress, ADDRESSLENGTH);
		entry->primaryRoute.hopsToRepeater = NO_ROUTE;
		entry->secondaryRoute.hopsToRepeater = NO_ROUTE;
		entry->neighbor = TRUE;
		entry->tsf = tsf;
		entry->newTSF = tsf;
		entry->neighborAge = 0;
		/*	Add entry to Hash Table */
		*index = newIndex;
	}
	return entry;
}


/***************************************************************************/
/*                                                                      	*/
/*  SetPrimaryRoute (*entry, hops, metric, *nextHopRepeater)					*/
/*                                                                      	*/
/*  INPUT:			entry  			 - Repeating Table Entry 						*/
/*						hops   			 - number of hops	to the repeater			*/
/*						metric 			 - link metric (1-255)							*/
/*						nextHopRepeater - Address of the next-hop repeater			*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will take a given node address and run		*/
/*						through a HASH algorithm to return the location of that	*/
/*						node address in the HASH table.									*/
/*																									*/
/***************************************************************************/

void SetPrimaryRoute (struct RepeatingTableEntry *entry, 
							 unsigned_8 hops,
							 unsigned_8 metric, 
							 unsigned_8 *nextHopRepeater)
{
	entry->primaryRoute.hopsToRepeater 	= hops;
	entry->primaryRoute.routingMetric 	= metric;
	CopyMemory(entry->primaryRoute.nextHopRepeater, nextHopRepeater, ADDRESSLENGTH);
	entry->primaryRoute.agingTimer 		= 0;
}


/***************************************************************************/
/*                                                                      	*/
/*  PromotoSecondaryRoutine (*entry)													*/
/*                                                                      	*/
/*  INPUT:			entry - repeating table entry										*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will promote a secondary route when the 		*/
/*						primary route disappears.											*/
/*																									*/
/***************************************************************************/

void PromoteSecondaryRoute (struct RepeatingTableEntry *entry)
{
	OutChar ('P', MAGENTA);
	OutChar ('r', MAGENTA);
	OutChar ('o', MAGENTA);
	OutChar ('M', MAGENTA);
	OutChar ('o', MAGENTA);
	OutChar ('t', MAGENTA);
	OutChar ('e', MAGENTA);
	CopyMemory (&entry->primaryRoute, &entry->secondaryRoute,
														sizeof(struct RepeatingInfo));
	entry->secondaryRoute.hopsToRepeater = NO_ROUTE;
}


/***************************************************************************/
/*                                                                      	*/
/*  DemotePrimaryRoute (*entry)															*/
/*                                                                      	*/
/*  INPUT:			entry - entry in Repeating Table									*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will demote a primary route						*/
/*																									*/
/***************************************************************************/

void DemotePrimaryRoute (struct RepeatingTableEntry *entry)
{
	OutChar ('D', MAGENTA);
	OutChar ('e', MAGENTA);
	OutChar ('M', MAGENTA);
	OutChar ('o', MAGENTA);
	OutChar ('t', MAGENTA);
	OutChar ('e', MAGENTA);
	CopyMemory (&entry->secondaryRoute, &entry->primaryRoute,
													sizeof(struct RepeatingInfo));
	entry->primaryRoute.hopsToRepeater = NO_ROUTE;
}


/***************************************************************************/
/*                                                                      	*/
/*  SwapRoutes (*entry)																		*/
/*                                                                      	*/
/*  INPUT:			entry - entry address of Repeating Table						*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine swaps the primary and secondary routes.		*/
/*																									*/
/***************************************************************************/

void SwapRoutes (struct RepeatingTableEntry *entry)
{
	struct RepeatingInfo tmpInfo;

	OutChar ('S', MAGENTA);
	OutChar ('W', MAGENTA);
	OutChar ('A', MAGENTA);
	OutChar ('P', MAGENTA);
	CopyMemory (&tmpInfo, &entry->primaryRoute, sizeof(struct RepeatingInfo));
	CopyMemory (&entry->primaryRoute, &entry->secondaryRoute,
														sizeof(struct RepeatingInfo));
	CopyMemory (&entry->secondaryRoute, &tmpInfo,	sizeof(struct RepeatingInfo));
}


/***************************************************************************/
/*                                                                      	*/
/*  RemoveTableEntry (*node, entry)														*/
/*                                                                      	*/
/*  INPUT:			node  - Node address													*/
/*						entry - entry address in the Repeating Table					*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will remove an entry from the Repeating		*/
/*						table.																	*/
/*																									*/
/***************************************************************************/

void RemoveTableEntry (unsigned_8 *node, 
							  struct RepeatingTableEntry *entry)
{
	unsigned_16	hashIndex;
	unsigned_16	repeatingIndex;
	unsigned_16 nodeIndex;
	unsigned_8	entryFound = FALSE;


	OutChar ('R', CYAN);
	OutChar ('E', CYAN);
	OutChar ('M', CYAN);
	OutChar ('O', CYAN);
	OutChar ('V', CYAN);
	OutChar ('E', CYAN);

	/*------------------------------*/
	/*	Remove entry from Hash Table */
	/*------------------------------*/

	hashIndex = Hash(node);
	repeatingIndex = WBRPHashTable[hashIndex];

	/*------------------------------*/
	/*	There must be something at   */
	/* this hash location			  */
	/*------------------------------*/
	assert(repeatingIndex != NO_LINK);

	/*------------------------------*/
	/*	Calculate index of node in   */
	/* Hash Table						  */
	/*------------------------------*/
	nodeIndex = entry - &RepeatingTable[0];

	if (repeatingIndex == nodeIndex)
		WBRPHashTable[hashIndex] = RepeatingTable[nodeIndex].link;
	else 
	{
		while (RepeatingTable[repeatingIndex].link != NO_LINK) 
		{
			if (RepeatingTable[repeatingIndex].link == nodeIndex) 
			{
				RepeatingTable[repeatingIndex].link = RepeatingTable[nodeIndex].link;
				entryFound = TRUE;
				break;
			}
			repeatingIndex = RepeatingTable[repeatingIndex].link;
		}
		assert(entryFound);
	}

	/*----------------------------------*/
	/* Remove entry from RepeatingTable */
	/*----------------------------------*/

	assert(entry->state == REPEATING_ENTRY_USED);
	entry->state = REPEATING_ENTRY_FREE;
	entry->link = RepeatingFreeIndex;
	RepeatingFreeIndex = nodeIndex;

	return;
}


/***************************************************************************/
/*                                                                      	*/
/*  RemoveNeighborTableEntry (*node)													*/
/*                                                                      	*/
/*  INPUT:			node - Node address													*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will a neighbor of the node from the 			*/
/*						repeating table														*/
/*																									*/
/***************************************************************************/

void RemoveNeighborTableEntry (unsigned_8 *node)
{
	struct RepeatingTableEntry *entry;

	assert(entry = FindNeighborTableEntry(node));

	if ( ENTRY_FOR_REPEATING (entry) ) 
	{
		/*	Invalidate neighbor entry */
		entry->neighbor = FALSE;
		entry->tsf = 0;
		entry->newTSF = 0;
	}
	else 
	{
		RemoveTableEntry(node, entry);
	}
}


/***************************************************************************/
/*                                                                      	*/
/*  RemoveRepeatingTableEntry (*node)													*/
/*                                                                      	*/
/*  INPUT:			node - Node address													*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION:	This routine will remove a node from the repeating table */
/*																									*/
/***************************************************************************/

void RemoveRepeatingTableEntry (unsigned_8 *node)
{
	struct RepeatingTableEntry *entry;

	assert(entry = FindRepeatingTableEntry(node));

	if ( ENTRY_FOR_NEIGHBOR (entry) ) 
	{
		/*	Invalidate repeating entries */
		entry->primaryRoute.hopsToRepeater = NO_ROUTE;
		entry->primaryRoute.hopsToRepeater = NO_ROUTE;
	}
	else 
	{
		RemoveTableEntry(node, entry);
	}
}


/***************************************************************************/
/*                                                                      	*/
/*  GetRepeatingEntry ()																	*/
/*                                                                      	*/
/*  INPUT:																						*/
/*																									*/
/*  OUTPUT:			Index of the repeating entry.										*/																						
/*                                        	                              */
/*  DESCRIPTION:	This routine will 													*/
/*																									*/
/***************************************************************************/

unsigned_16 GetRepeatingEntry (void)
{
   unsigned_16 index = RepeatingFreeIndex;

   if (index != NO_LINK)
   {
      RepeatingFreeIndex = RepeatingTable[RepeatingFreeIndex].link;

		/*----------------------------------------*/
      /* repeating entry must be currently free */
		/*----------------------------------------*/

      assert(RepeatingTable[index].state == REPEATING_ENTRY_FREE);
      RepeatingTable[index].state = REPEATING_ENTRY_USED;
      RepeatingTable[index].link = NO_LINK;
   }

   return(index);
}
