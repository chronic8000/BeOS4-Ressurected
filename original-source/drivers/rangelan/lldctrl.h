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
/*	$Header:   J:\pvcs\archive\clld\lldctrl.h_v   1.8   27 Sep 1996 09:06:12   JEAN  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldctrl.h_v  $										*/
/* 
/*    Rev 1.8   27 Sep 1996 09:06:12   JEAN
/* 
/*    Rev 1.7   14 Jun 1996 16:25:56   JEAN
/* Comment changes.
/* 
/*    Rev 1.6   31 Jan 1996 13:33:02   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.5   17 Nov 1995 16:40:58   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.4   12 Oct 1995 11:54:28   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.3   16 Mar 1995 16:18:54   tracie
/* Added support for ODI
/* 
/*    Rev 1.2   22 Sep 1994 10:56:24   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:20   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:34   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDCTRL_H
#define LLDCTRL_H

/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "lldctrl.c"					*/
/*																							*/
/*********************************************************************/

unsigned_8	LLDReset (void);
void 		 	LLDPoll (void);
unsigned_8	LLDMulticast (unsigned_8 OnOrOff);
unsigned_8	LLDPromiscuous (unsigned_8 OnOrOff);
unsigned_8	LLDStop (void);
void OutChar (unsigned_8 Character, unsigned_8 ColorCode);

#endif /* LLDCTRL_H */

