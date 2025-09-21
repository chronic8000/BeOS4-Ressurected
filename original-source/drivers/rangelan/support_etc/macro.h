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
/*	$Header:   J:\pvcs\archive\clld\macro.h_v   1.5   31 Jan 1996 13:38:54   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\macro.h_v  $											*/
/* 
/*    Rev 1.5   31 Jan 1996 13:38:54   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.4   12 Oct 1995 12:04:38   JEAN
/* Added the $Header:   J:\pvcs\archive\clld\macro.h_v   1.5   31 Jan 1996 13:38:54   JEAN  $ PVCS keyword.
/* 
/*    Rev 1.3   27 Dec 1994 16:10:54   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:32   hilton
/* No change.
/* 
/*    Rev 1.1   20 Sep 1994 16:03:30   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:42   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/


#ifndef MACRO_H
#define MACRO_H

#define	PRESERVEFLAG	_asm	pushf
#define	DISABLEINT		_asm	cli
#define	ENABLEINT		_asm	sti
#define	RESTOREFLAG		_asm	popf
#define	INT3				_asm	int 3

#endif /* MACRO_H */