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
/*	$Header:   J:\pvcs\archive\clld\lldqueue.c_v   1.7   14 Jun 1996 16:22:12   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldqueue.c_v  $																						*/
/* 
/*    Rev 1.7   14 Jun 1996 16:22:12   JEAN
/* Comment changes.
/* 
/*    Rev 1.6   16 Apr 1996 11:33:28   JEAN
/* 
/*    Rev 1.5   06 Feb 1996 14:26:32   JEAN
/* Comment changes.
/* 
/*    Rev 1.4   31 Jan 1996 13:12:48   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.3   19 Dec 1995 18:09:52   JEAN
/* Added a header include file.
/* 
/*    Rev 1.2   14 Dec 1995 15:19:36   JEAN
/* Added a header include file.
/* 
/*    Rev 1.1   12 Oct 1995 11:42:22   JEAN
/* Added missing edit history section.
/* 
/*																									*/
/***************************************************************************/

/***************************************************************************/
/*																									*/
/* BUFFER MANAGEMENT																			*/
/*																									*/
/* DESCRIPTION: This module contains the buffer management procedures		*/
/*		to maintain the linked list of buffers from the LLD.  All of			*/
/*		the procedures, are list independent so they can be used to				*/
/*		operate on new lists later.														*/
/*																									*/
/* FUNCTIONS:																					*/
/*																									*/
/*		InitList (List) 					;Initialize linked list to empty.		*/
/*		RemoveHead (List)					;Remove the head of the list.				*/
/*		AddToList (List, void *)		;Add element to the list.					*/
/*																									*/
/***************************************************************************/

#include "constant.h"
#include "lld.h"
#include	"bstruct.h"
#include "lldqueue.h"

#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#endif

#ifdef MULTI
#include "multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else

extern struct ReEntrantQueue	ReEntrantQBuff [TOTALREQUEUE];
extern struct LLDLinkedList 	FreeReEntrantQ;
extern struct LLDLinkedList 	ReEntrantQ;

#endif



/***************************************************************************/
/*																									*/
/*	initReEntrantQueues ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will build a linked list of free ReEntrant    */
/*					 buffers from a global buffer poll and initialize the       */
/*					 FreeReEntrantQ and ReEntrantQ pointers.							*/ 
/*																									*/
/***************************************************************************/

void initReEntrantQueues (void)

{
	unsigned_8	i;


	InitReEntrantList ( &FreeReEntrantQ );
	InitReEntrantList ( &ReEntrantQ );

	for (i=0; i < TOTALREQUEUE; i++)
	{
		ReEntrantQBuff [i].PacketPtr	= NIL;
		ReEntrantQBuff [i].PacketType = 0;
		AddToReEntrantList ( &FreeReEntrantQ, &ReEntrantQBuff[i] );
	}
}



/************************************************************************/
/*																								*/
/* InitReEntrantList (*List)															*/
/*																								*/
/* INPUT:		List	-	List to initialize to empty.							*/
/*																								*/
/* OUTPUT:																					*/
/*																								*/
/* DESCRIPTION: Initialize the head and tail pointers to NIL.				*/
/*																								*/
/************************************************************************/

void InitReEntrantList (struct LLDLinkedList *List)

{	
	List->Head = NIL;
	List->Tail = NIL;
}
	 	

/************************************************************************/
/*																								*/
/* AddToReEntrantList (*List, *node)												*/
/*																								*/
/* INPUT:	List		-	Linked List to add CBlock to.							*/
/*				node		-  pointer to ReEntrantQueue node to be added.		*/
/*																								*/
/* OUTPUT:																					*/
/*																								*/
/*	DESCRIPTION:	Adds a ReEntrantQueue Buffer to the tail of given     */
/*						linked list.														*/
/*																								*/
/************************************************************************/

void AddToReEntrantList (struct LLDLinkedList *List,
								 struct ReEntrantQueue *node)

{
	if (List->Head == NIL)			/* Check for empty list						*/
	{	
		List->Head = node;			/* If not, New element is head & tail	*/
		List->Tail = node;
		node->Next = NIL; 			/*	Element is last, so point to NIL		*/
	}
	else
	{	
		List->Tail->Next = node;	/* Link Old tail to new element			*/
		List->Tail = node;			/* New element is the tail					*/
		node->Next = NIL;				/* Element is last, so point to NIL		*/
	}
}		


/************************************************************************/
/*																								*/
/* RemoveReEntrantHead (*List)														*/
/*																								*/
/* INPUT:		List	-	Linked List to get head element from.				*/
/*																								*/
/* OUTPUT:		Returns the address of the ReEntrantQueue buffer removed.*/
/*																								*/
/*	DESCRIPTION:This routine will remove an element from the head of the */
/*					linked list.															*/
/*																								*/
/************************************************************************/

struct ReEntrantQueue *RemoveReEntrantHead (struct LLDLinkedList *List)

{	struct ReEntrantQueue *Temp;

	if (List->Head == NIL)
	{	
		return (NIL);
	}
	else
	{	
		Temp = List->Head;
		List->Head = List->Head->Next;
		return (Temp);
	}
}

