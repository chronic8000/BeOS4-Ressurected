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
/*	$Header:   J:\pvcs\archive\clld\rptable.h_v   1.3   06 Feb 1996 14:42:58   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\rptable.h_v  $	  									*/
/* 
/*    Rev 1.3   06 Feb 1996 14:42:58   JEAN
/* Fixed some function prototypes.
/* 
/*    Rev 1.2   31 Jan 1996 13:55:54   Jean
/* Added duplicate header include detection and moved defines for
/* TRUE and FALSE to constant.h.
/* 
/*    Rev 1.1   12 Oct 1995 13:45:12   JEAN
/* Added missing copyright and edit history.
/* 
/*    Rev 1.0   22 Feb 1995 09:52:52   tracie
/* Initial revision.
/***************************************************************************/

#ifndef RPTABLE_H
#define RPTABLE_H

#define 	ADDR_lEN	 				6

#define 	MAX_HOP_COUNT			10
#define	NO_ROUTE					0xff

#define	ENTRY_FOR_REPEATING(entry)	\
										(entry->primaryRoute.hopsToRepeater != NO_ROUTE)
#define	ENTRY_FOR_NEIGHBOR(entry)	(entry->neighbor == TRUE)

#define	MAX_REPEATERS			10
#define	MAX_NEIGHBORS			(MAX_REPEATERS)

#define  REPEATING_ENTRY_FREE	((unsigned_8) 0x00)
#define  REPEATING_ENTRY_USED	((unsigned_8) 0xff)
#define	NO_LINK					0xffff

#define	HASH_TABLE_SIZE		32
#define  HASH_TABLE_MASK      ((HASH_TABLE_SIZE)-1)


/*************************************************************************/
/*                                                                       */
/*                           STRUCTURES											 */                                                                        
/*                                                                       */
/*************************************************************************/

struct RepeatingInfo 
{
	unsigned_8		hopsToRepeater;
	unsigned_8		routingMetric;
	unsigned_8		nextHopRepeater[ADDRESSLENGTH];
	unsigned_16		agingTimer;
};

struct RepeatingTableEntry 
{
	unsigned_8				state;
	unsigned_8				repeaterAddress[ADDRESSLENGTH];
	struct RepeatingInfo	primaryRoute;
	struct RepeatingInfo	secondaryRoute;
	unsigned_8				neighbor;
	unsigned_8				tsf; 			 /* Only relevant for neighbors 	*/
	unsigned_8				newTSF;		 /* newly calculated TSF			*/
	unsigned_16				neighborAge; /* Only relevant for neighbors 	*/
	unsigned_16		 		link; 		 /* Link to next node with same 	*/
												 /* repeater address hash			*/
};


/*********************************************************************/
/*																							*/
/* Prototype definitions for file "rptable.c" 								*/
/*																							*/
/*********************************************************************/

void			RepeatingTableInit (void);
void			AddRepeatingTableEntry (unsigned_8 *repeaterAddress, unsigned_8 hops,
										unsigned_8 metric, unsigned_8 *nextHopRepeater);
struct		RepeatingTableEntry *FindNeighborTableEntry(unsigned_8 *node);
struct		RepeatingTableEntry *FindRepeatingTableEntry(unsigned_8 *node);
struct		RepeatingTableEntry *AddNeighborTableEntry(unsigned_8 *repeaterAddress,
										unsigned_8 tsf);
void			SetPrimaryRoute (struct RepeatingTableEntry *entry, unsigned_8 hops,
									 unsigned_8 metric, unsigned_8 *nextHopRepeater);
void			PromoteSecondaryRoute(struct RepeatingTableEntry *entry);
void			DemotePrimaryRoute (struct RepeatingTableEntry *entry);
void			SwapRoutes (struct RepeatingTableEntry *entry);
void			RemoveTableEntry (unsigned_8 *node, struct RepeatingTableEntry *entry);
void			RemoveNeighborTableEntry (unsigned_8 *node);
void			RemoveRepeatingTableEntry(unsigned_8 *node);
unsigned_16	GetRepeatingEntry (void);


/*************************************************************************/
/*                                                                       */
/*                              VARIABLES											 */
/*                                                                       */
/*************************************************************************/
extern struct RepeatingTableEntry RepeatingTable[MAX_REPEATERS];

#endif /* RPTABLE_H */
