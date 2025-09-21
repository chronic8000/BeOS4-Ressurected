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
/*	$Header:   J:\pvcs\archive\clld\lldqueue.h_v   1.5   27 Sep 1996 09:21:02   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldqueue.h_v  $										*/
/* 
/*    Rev 1.5   27 Sep 1996 09:21:02   JEAN
/* 
/*    Rev 1.4   14 Jun 1996 16:45:22   JEAN
/* Comment changes.
/* 
/*    Rev 1.3   06 Feb 1996 14:38:40   JEAN
/* Fixed some function prototypes.
/* 
/*    Rev 1.2   31 Jan 1996 13:49:56   Jean
/* Added duplicate header include detection.
/* 
/*    Rev 1.1   12 Oct 1995 13:40:22   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.0   22 Feb 1995 10:16:28   tracie
/* Initial revision.
/* 																								*/
/***************************************************************************/

#ifndef LLDQUEUE_H
#define LLDQUEUE_H


/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "lldqueue.c"					*/
/*																							*/
/*********************************************************************/


void initReEntrantQueues (void);
void InitReEntrantList (struct LLDLinkedList *List);
void AddToReEntrantList (struct LLDLinkedList *List, struct ReEntrantQueue *node);
struct ReEntrantQueue *RemoveReEntrantHead (struct LLDLinkedList *List);

#endif /* LLDQUEUE_H */