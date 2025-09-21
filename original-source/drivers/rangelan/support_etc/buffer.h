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
/*	$Header:   J:\pvcs\archive\xromtest\buffer.h_v   1.4   31 Jan 1996 11:32:20   JEAN  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\xromtest\buffer.h_v  $	 								*/
/* 
/*    Rev 1.4   31 Jan 1996 11:32:20   JEAN
/* Added duplicate header include detection and removed some
/* unused code.
/* 
/*    Rev 1.3   17 Nov 1995 16:13:12   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG release.
/* 
/*    Rev 1.2   12 Oct 1995 14:38:24   JEAN
/* Added missing copyright and edit history section.
/* 
/* 
/*																									*/
/***************************************************************************/


/************************************************************************/
/*																								*/
/* BUFFER STRUCTURES																		*/
/*																								*/
/* DESCRIPTION: This module contains the buffer and linked list			*/
/*		structures used to maintain the linked list of receive buffers		*/
/*		from the LLD.																		*/
/*																								*/
/************************************************************************/

#ifndef BUFFER_H
#define BUFFER_H

#define		BUFFERSIZE	1664		/* Buffer size									*/
#define		NUMBUFFERS	8			/* Number of receive buffers				*/

struct CBlockStruct	
{	struct CBlockStruct	*Next;			
	unsigned int			CBlockLength;	
	unsigned int			CBlockRSS;
	unsigned long			CBlockTime;
	unsigned char			CBlockBuff [BUFFERSIZE];
};


struct LinkedList
{	struct CBlockStruct	*Head;
	struct CBlockStruct	*Tail;
};


/************************************************************************/
/*																								*/
/* PROTOTYPE DEFINITIONS																*/
/*																								*/
/************************************************************************/

void InitList (struct LinkedList *List);
void AddToList (struct LinkedList *List, struct CBlockStruct *CBlock);
struct CBlockStruct *RemoveHead (struct LinkedList *List);

#endif /* BUFFER_H */

