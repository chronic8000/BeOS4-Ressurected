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
/*	$Header:   J:\pvcs\archive\clld\slldbuf.h_v   1.11   14 Jun 1996 16:46:16   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\slldbuf.h_v  $																							*/
/* 
/*    Rev 1.11   14 Jun 1996 16:46:16   JEAN
/* Comment changes.
/* 
/*    Rev 1.10   16 Apr 1996 11:43:24   JEAN
/* Changed and added some defines for the new buffer structures.
/* 
/*    Rev 1.9   06 Feb 1996 14:39:06   JEAN
/* Cleaned up the file.
/* 
/*    Rev 1.8   31 Jan 1996 13:50:50   Jean
/* Added duplicate header include detection and deleted some
/* unused function prototypes.
/* 
/*    Rev 1.7   19 Dec 1995 18:15:12   JEAN
/* Since the buffers for multiple cards moved to the internal
/* vars structure, we no longer need to increase the number of
/* buffers (each card has it's own array of buffers).
/* 
/*    Rev 1.6   14 Dec 1995 15:55:34   JEAN
/* Increased the number of buffers for multiple serial cards.
/* 
/*    Rev 1.5   17 Nov 1995 16:45:38   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 13:40:56   JEAN
/* Added missing copyright and edit history.
/*
/*																									*/
/***************************************************************************/

#ifndef SLLDBUF_H
#define SLLDBUF_H

/************************************************************************/
/*																								*/
/* BUFFER STRUCTURES																		*/
/*																								*/
/* DESCRIPTION: This module contains the buffer and linked list			*/
/*		structures used to maintain the linked list of Free, Transmit and	*/
/*		Receive buffers from the Serial LLD. 										*/
/*																								*/
/************************************************************************/

#define	TOTBUFFERS 5				/* The number of buffers in the buffer		*/
											/* pool.  One is used for receiving			*/
											/* buffers and four are used for				*/
											/* transmitting buffers.						*/

#define	BUF_POOL_SIZE (TOTBUFFERS*BUFFERLEN)
											/* This is the number of bytes in the		*/
											/* buffer pool.  1550 bytes for each of	*/
											/* the 5 buffers.									*/

#define TOT_RX_BUFFERS	1			/* The number of receive buffers.			*/

#define TOT_LARGE_BUFFERS 2		/* There are four transmit buffers.  Two	*/
											/* are large enough to accomodate the		*/
											/* largest packet size and the other two	*/
											/* are split up into smaller buffers		*/
											/* based on the LLDFragSize.					*/

#define MIN_FRAG_SIZE	190		/* The smallest size a larger packet gets	*/
											/* divided into when we are in the			*/
											/* FragState.										*/
                                
#define TOT_SMALL_BUFFERS	2		/* These are split into smaller buffer		*/
											/* used to transmit small packets and		*/
											/* large fragmented packets.					*/

#define MAX_FRAG_BUFFERS	((BUFFERLEN*TOT_SMALL_BUFFERS)/MIN_FRAG_SIZE)
											/* This is the maximum number of buffers	*/
											/* the two larger transmit buffers can		*/
											/* be split into.  The size of each is		*/
											/* dependent on LLDFragSize.					*/

#define MAX_BUF_TABLES	(TOT_RX_BUFFERS+TOT_LARGE_BUFFERS+MAX_FRAG_BUFFERS)

struct TxRcvQueue
{
	struct TxRcvQueue	*next;		 	/* Points to next buffer in 	*/
												/* buffer list.					*/
	unsigned_16			qSize;	 		/* The size of the buffer.		*/
	unsigned_16			qDataLen;	 	/* Length of the data in		*/
												/* buffer.  This is used for	*/
												/* transmit packets.  The		*/
												/* receive packets use the		*/
												/* length in the rvcStruct.	*/
	unsigned_8		  	*qDataPtr;		/* Pointer to the beginning	*/
												/* of the data buffer.			*/
};



/*-------------------------------------------------*/
/* Use these defines to distinguish between the		*/
/* free list and the ready list.  When the caller	*/
/* requests a buffer, if they want a buffer from	*/
/* the free list, we need to check the buffer		*/
/* size.  If the caller requests a buffer from		*/
/* the ready list, we just return the first 			*/
/* buffer at the head of the list.						*/
/*-------------------------------------------------*/

#define READY_LIST	0
#define	FREE_LIST	1

struct SLLDLinkedList
{	unsigned_16 		 type;
	struct TxRcvQueue	*head;
	struct TxRcvQueue	*tail;
	struct TxRcvQueue	*smhead;
	struct TxRcvQueue	*smtail;
};




/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "slldbuf.c"					*/
/*																							*/
/*********************************************************************/

void 					SLLDInitBufs (void);
void					SLLDPutABuf (struct SLLDLinkedList *list, struct TxRcvQueue * bptr);
struct TxRcvQueue *SLLDGetABuf (struct SLLDLinkedList *list, unsigned_16 size);
void					SLLDInitList (struct SLLDLinkedList *list, unsigned_16 type);

#endif /* SLLDBUF_H */
