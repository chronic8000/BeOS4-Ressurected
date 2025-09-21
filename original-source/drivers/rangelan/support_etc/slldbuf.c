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
/*	$Header:   J:\pvcs\archive\clld\slldbuf.c_v   1.11   14 Jun 1996 16:22:42   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldbuf.c_v  $																							*/
/* 
/*    Rev 1.11   14 Jun 1996 16:22:42   JEAN
/* Comment changes.
/* 
/*    Rev 1.10   16 Apr 1996 11:33:42   JEAN
/* Changed the buffering scheme.  Instead of 3 large transmit
/* buffers, there are now 2 large buffers and upto 15 small
/* buffers.  This change was needed to support fragmentation.
/* 
/* Initial revision		30 Nov 1994		Tracie										*/
/*																									*/
/***************************************************************************/


/***************************************************************************/
/*																									*/
/* BUFFER MANAGEMENT																			*/
/*																									*/
/* DESCRIPTION: This module contains the buffer management procedures		*/
/*					 to maintain the buffer pool used for serial data 				*/
/*					 communications.															*/
/*																									*/
/***************************************************************************/

#include "constant.h"
#include "lld.h"
#include "slldbuf.h"
#include "slld.h"
#include "pstruct.h"

#ifdef MULTI
#include "bstruct.h"
#include "multi.h"
extern struct LLDVarsStruct   *LLDVars;
#else

/************************************************************************/
/*																								*/
/* There is a buffer pool for each cards in the system.  One buffer		*/
/* in the pool is used for receiving packets and the rest of the			*/
/* buffers are used for transmitting packets.				  					*/
/*																								*/
/************************************************************************/

unsigned_8						uBufPool[BUF_POOL_SIZE]; 	

struct TxRcvQueue				uBufTable [MAX_BUF_TABLES];
									/* One receive, two large transmit, and two	*/
									/* small transmit buffers.  The two small 	*/
									/* transmit buffers are split into multiple	*/
									/* buffers.												*/

struct SLLDLinkedList		uFreeList;
struct SLLDLinkedList		uTxRdyList;
unsigned_16						uSmallBufSize;
extern unsigned_16			LLDFragSize;

#endif


/************************************************************************/
/*																								*/
/* SLLDInitBufs ()																		*/
/*																								*/
/* INPUT:			 																		*/
/*																								*/
/* OUTPUT:																					*/
/*																								*/
/* DESCRIPTION: This routine initializes the buffer table, the free	 	*/
/*              list, and the transmit ready list.  All of the buffers	*/
/*              are placed onto the free list.									*/
/*																								*/
/************************************************************************/


void SLLDInitBufs (void)

{	
	unsigned_8	ii;
	unsigned_16 bytesUsed = 0;
	struct TxRcvQueue	*qptr;

	SLLDInitList (&uFreeList, FREE_LIST);
	SLLDInitList (&uTxRdyList, READY_LIST);
	
	/*-------------------------------------*/
	/* Build a free list	of large buffers.	*/
	/* Each of these buffers is large		*/
	/* enough to accomodate the maximum		*/
	/* ethernet packet plus our headers.	*/
	/*-------------------------------------*/
	for (ii = 0; ii < TOT_LARGE_BUFFERS+TOT_RX_BUFFERS; ii++)
	{
		qptr				= &uBufTable[ii];
		qptr->qSize 	= BUFFERLEN;
		qptr->qDataLen = 0;
		qptr->qDataPtr = &uBufPool[bytesUsed];
		bytesUsed	  += BUFFERLEN;
		SLLDPutABuf (&uFreeList, qptr);
	}

	/*-------------------------------------------*/
	/* Calculate the size of each small buffer.	*/
	/* LLDFragSize is for data only.  We need to	*/
	/* allow for our headers.  These are:  the	*/
	/* 7 byte serial header, the 12 byte proxim	*/
	/* header, a possible 12 byte bridge header,	*/
	/* and the 12 bytes for the source and 		*/
	/* destination MAC addresses.  Finally, we	*/
	/* align the buffers on an 8 byte boundary. 	*/
	/*-------------------------------------------*/
	uSmallBufSize = ((LLDFragSize+TOTAL_HDRS + sizeof (struct PacketizeTx) +
							LLDBRIDGE_HDR_LEN + (ADDRESSLENGTH*2)) + 7) & 0xFFF8;


	/*-------------------------------------*/
	/* Build a free list	of small buffers. */
	/* SLLDPutABuf puts the buffers on the	*/
	/* smhead/smtail list.						*/
	/*-------------------------------------*/
	while ( ((bytesUsed+uSmallBufSize) < BUF_POOL_SIZE) &&
			 (ii < MAX_BUF_TABLES))
	{
		qptr				= &uBufTable[ii++];
		qptr->qSize 	= uSmallBufSize;
		qptr->qDataLen = 0;
		qptr->qDataPtr = &uBufPool[bytesUsed];
		bytesUsed     += uSmallBufSize;
		SLLDPutABuf (&uFreeList, qptr);
	}

}


/************************************************************************/
/*																								*/
/* SLLDInitList (*list)																	*/
/*																								*/
/* INPUT:		list - Points to the list to initialize.						*/
/*																								*/
/* OUTPUT:																					*/
/*																								*/
/* DESCRIPTION: This routine initializes either the free or the			*/
/*              transmit ready list.												*/
/*																								*/
/************************************************************************/

void SLLDInitList (struct SLLDLinkedList *list, unsigned_16 type)

{
	list->type = type;
	list->head = NIL;
	list->tail = NIL;
	list->smhead = NIL;
	list->smtail = NIL;
}


/************************************************************************/
/*																								*/
/* SLLDGetABuf (*list, size) 															*/
/*																								*/
/* INPUT:			list - List to remove a buffer from.						*/
/*                size - Size of buffer needed.									*/
/*																								*/
/* OUTPUT:			Pointer to the buffer.	 							  			*/
/*																								*/
/*	DESCRIPTION:	This routine removes a buffer from the head of either	*/
/*						the free or the transmit ready list.						*/
/*																								*/
/************************************************************************/

struct TxRcvQueue *SLLDGetABuf (struct SLLDLinkedList *list, unsigned_16 size)

{ 	
	struct TxRcvQueue *tmp;

	/*-------------------------------------------------*/
	/* When we are getting a packet from the transmit	*/
	/* ready list, just get the first buffer on the		*/
	/* head/tail list.  The smhead/smtail lists are		*/
	/* not used.													*/
	/*																	*/
	/* If the caller request a buffer larger than the	*/
	/* small buffer size, give them the first buffer	*/
	/* on the large buffer list.  Like the transmit		*/
	/* ready list, the large buffers	are kept on the	*/
	/* head/tail list.											*/
	/*-------------------------------------------------*/
	if ( (list->type == READY_LIST) || (size > uSmallBufSize) )
	{
		if (list->head == NIL)
		{
			/*----------------------*/
			/* The list is empty.	*/
			/*----------------------*/
			return NIL;
		}
		else
		{

			/*----------------------------------------*/
			/* Removed the first buffer in the list.	*/
			/* Buffers are added to end of the list,	*/
			/* so we need to pull them from the			*/
			/* beginning of the list.  If it is the	*/
			/* transmit ready list, we want to make	*/
			/* sure we don't transmit the packets		*/
			/* out of order.									*/
			/*----------------------------------------*/
			tmp = list->head;
			list->head = list->head->next;
			return (tmp);
		}
	}


	/*-------------------------------------------*/
	/* Get a buffer from the free list.  Try to	*/
	/* get a small buffer first.  If the small	*/
	/* buffer list is empty, try to give the		*/
	/* caller a	large buffer.			 				*/
	/*-------------------------------------------*/
	else
	{

		if (list->smhead == NIL)
		{
			if (list->head == NIL)
			{
				/*-------------------------*/
				/* Both lists are empty.	*/
				/*-------------------------*/
				return NIL;
			}
			else
			{

				/*----------------------------------------*/
				/* Since there are no more small buffers	*/
				/* left, give the caller the first buffer	*/
				/* from the large buffer list.	  			*/
				/*----------------------------------------*/
				tmp = list->head;
				list->head = list->head->next;
				return (tmp);
			}
		}

		else
		{

			/*----------------------------------*/
			/* Give the caller the small buffer	*/
			/* at the head of the list.			*/
			/*----------------------------------*/
			tmp = list->smhead;
			list->smhead = list->smhead->next;
			return (tmp);
		}
	}
}


/***************************************************************************/
/*					    																			*/
/* SLLDPutABuf (*list, *bptr) 															*/
/*							   																	*/
/*	INPUT:		list   - The linked list that buffer goes to						*/
/*					bufptr - Pointer to buffer to put into the list.				*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine adds a buffer to the end of either the free	*/
/*              or the transmit ready list.	  										*/
/*																									*/
/***************************************************************************/

void SLLDPutABuf (struct SLLDLinkedList *list, struct TxRcvQueue *bptr)

{

	/*----------------------------------*/
	/* Make the buffer the last buffer	*/
	/* in the list.							*/
	/*----------------------------------*/
	bptr->next = NIL;

	/*-------------------------------------------------------*/
	/* All transmit ready buffers go on the head/tail list.	*/
	/* For the free list, the large buffers go on the			*/
	/* head/tail list and the small buffers go on the			*/
	/* smhead/smtail list.												*/
	/*-------------------------------------------------------*/
	if ((list->type == READY_LIST) || (bptr->qSize == BUFFERLEN))
	{
		if (list->head == NIL)
		{
			list->head = bptr;			/* The list is empty, so assign head 	*/
			list->tail = bptr;			/* and tail to the buffer.				 	*/
		}
		else
		{
			list->tail->next = bptr;	/* Add the buffer to the end of the		*/
			list->tail = bptr;			/* list and advance the tail pointer.	*/
		}

	}


	/*----------------------------*/
	/* It is a small free buffer.	*/
	/*----------------------------*/
	else
	{
		if (list->smhead == NIL)
		{
			list->smhead = bptr;			/* The list is empty, so assign head 	*/
			list->smtail = bptr;			/* and tail to the buffer.				 	*/
		}
		else
		{
			list->smtail->next = bptr;	/* Add the buffer to the end of the		*/
			list->smtail = bptr;			/* list and advance the tail pointer.	*/
		}
	}
}

