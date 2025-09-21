/***************************************************************************/
/*																									*/
/*						    PROXIM RANGELAN2 TEST PROGRAM 								*/
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
/*	$Header:   J:\pvcs\archive\xromtest\buffer.c_v   1.3   31 Jan 1996 11:25:22   JEAN  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\xromtest\buffer.c_v  $			 						*/
/* 
/*    Rev 1.3   31 Jan 1996 11:25:22   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.2   17 Nov 1995 16:11:46   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG release.
/* 
/*    Rev 1.1   12 Oct 1995 14:32:40   JEAN
/* Added missing edit history section.
/* 
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
/*		AddToList (List, CBlock)		;Add element to the list.					*/
/*																									*/
/***************************************************************************/


#include "constant.h"
#include "lld.h"
#include	"buffer.h"



/************************************************************************/
/*																								*/
/* INITLIST																					*/
/*																								*/
/* ABSTRACT: InitList sets the data linked list to be empty.				*/
/*																								*/
/* INPUTS:	List	-	List to initialize to empty.								*/
/*																								*/
/* OUTPUTS:	None.																			*/
/*																								*/
/************************************************************************/

void InitList (struct LinkedList *List)


{	
	List->Head = NIL;
	List->Tail = NIL;
}
	 	



/************************************************************************/
/*																								*/
/* ADDTOLIST																				*/
/*																								*/
/*	ABSTRACT: AddToList adds the CBlock pointer to the tail of the			*/
/*		given list.																			*/
/*																								*/
/* INPUTS:	List		-	Linked List to add CBlock to.							*/
/*				CBlock	-	CBlock to add to the list.								*/
/*																								*/
/* OUTPUTS:	None.																			*/
/*																								*/
/************************************************************************/

void AddToList (struct LinkedList *List, struct CBlockStruct *CBlock)


{
	if (List->Head == NIL)			/* Check for empty list						*/
	{	List->Head = CBlock;			/* If not, New element is head & tail	*/
		List->Tail = CBlock;
		CBlock->Next = NIL; 			/*	Element is last, so point to NIL		*/
	}
	else
	{	List->Tail->Next = CBlock;	/* Link Old tail to new element			*/
		List->Tail = CBlock;			/* New element is the tail					*/
		CBlock->Next = NIL;			/* Element is last, so point to NIL		*/
	}
}		





/************************************************************************/
/*																								*/
/* REMOVEHEAD																				*/
/*																								*/
/* ABSTRACT: RemoveHead removes the head CBlock from the list and			*/
/*		returns the address of the element removed.								*/
/*																								*/
/* INPUTS:	List	-	Linked List to get head element from.					*/
/*																								*/
/* OUTPUTS:	Returns the address of the CBlock removed.						*/
/*																								*/
/************************************************************************/

struct CBlockStruct *RemoveHead (struct LinkedList *List)


{	struct CBlockStruct	*Temp;

	if (List->Head == NIL)
	{	return (NIL);
	}
	else
	{	Temp = List->Head;
		List->Head = List->Head->Next;
		return (Temp);
	}
}
