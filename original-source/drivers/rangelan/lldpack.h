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
/*	$Header:   J:\pvcs\archive\clld\lldpack.h_v   1.12   24 Mar 1998 09:13:58   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldpack.h_v  $										*/
/* 
/*    Rev 1.12   24 Mar 1998 09:13:58   BARBARA
/* LLDGotoSniffMode() added.
/* 
/*    Rev 1.11   12 Sep 1997 17:02:06   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.10   27 Sep 1996 09:14:24   JEAN
/* 
/*    Rev 1.9   14 Jun 1996 16:35:26   JEAN
/* Comment changes.
/* 
/*    Rev 1.8   16 Apr 1996 11:40:12   JEAN
/* Added a function prototype.
/* 
/*    Rev 1.7   12 Mar 1996 10:44:40   JEAN
/* Added two new function prototypes.
/* 
/*    Rev 1.6   08 Mar 1996 19:26:48   JEAN
/* Added a function prototype.
/* 
/*    Rev 1.5   31 Jan 1996 13:36:42   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.4   12 Oct 1995 11:57:32   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.3   29 Nov 1994 12:45:54   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:30   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:26   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:38   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDPACK_H
#define LLDPACK_H

/*********************************************************************/
/*																							*/
/* Prototype definitions for file "lldpack.c"								*/
/*																							*/
/*********************************************************************/

unsigned_8	LLDSendInitializeCommand (void);
unsigned_8	LLDSetITO (void);
unsigned_8	LLDSendGotoStandbyCmd (void);
unsigned_8	LLDGotoSniffMode (void);
unsigned_8	LLDDisableHopping (void);
unsigned_8	LLDGetROMVersion (void);
unsigned_8	LLDConfigMAC (void);
unsigned_8	LLDSearchAndSync (void);
unsigned_8	LLDAbortSearch (void);
unsigned_8	LLDGetGlobalAddress (void);
unsigned_8	LLDSetRoaming (void);
unsigned_8	LLDSetMulticast (unsigned_8 OnOrOff);
unsigned_8	LLDSetPromiscuous (unsigned_8 Mode);
unsigned_8	LLDSendKeepAlive (void);
unsigned_8	LLDSetSecurityID (void);
unsigned_8	LLDRoam (void);
unsigned_8	LLDDisableEEPROMWrite (void);
unsigned_8	LLDEnableEEPROMWrite (void);
unsigned_8	LLDGetRFNCStats (void);
unsigned_8	LLDSearchContinue (void);
#ifdef SERIAL
unsigned_8  LLDSendSoftBaudCmd (unsigned_8 baud);
#endif
void 			IncPacketSeqNum (void);

#endif /* LLDPACK_H */
