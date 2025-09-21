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
/*	$Header:   J:\pvcs\archive\clld\lldinit.h_v   1.14   12 Sep 1997 17:00:54   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldinit.h_v  $										*/
/* 
/*    Rev 1.14   12 Sep 1997 17:00:54   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.13   02 Oct 1996 14:32:42   JEAN
/* Added a function prototype.
/* 
/*    Rev 1.12   27 Sep 1996 09:12:18   JEAN
/* 
/*    Rev 1.11   14 Jun 1996 16:34:46   JEAN
/* Added some new LLDInit return codes.
/* 
/*    Rev 1.10   15 Mar 1996 14:10:20   TERRYL
/* Added LLDMacOptimize() and LLDRoamConfig()
/* 
/*    Rev 1.9   12 Mar 1996 10:44:14   JEAN
/* Added two function prototypes.
/* 
/*    Rev 1.8   31 Jan 1996 13:36:14   JEAN
/* Added duplicate header include detection.
/* 
/*    Rev 1.7   17 Nov 1995 16:42:06   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.6   12 Oct 1995 11:57:10   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.5   13 Apr 1995 08:51:08   tracie
/* xromtest version
/* 																								*/
/*    Rev 1.4   16 Mar 1995 16:19:20   tracie										*/
/* Added support for ODI																	*/
/* 																								*/
/*    Rev 1.3   05 Jan 1995 11:09:46   hilton										*/
/* Changes for multiple card version													*/
/* 																								*/
/*    Rev 1.2   22 Sep 1994 10:56:28   hilton										*/
/* 																								*/
/*    Rev 1.1   20 Sep 1994 16:03:26   hilton										*/
/* No change.																					*/
/* 																								*/
/*    Rev 1.0   20 Sep 1994 11:00:38   hilton										*/
/* Initial revision.																			*/
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef LLDINIT_H
#define LLDINIT_H

/*********************************************************************/
/*																							*/
/* LLDInit () return error codes.												*/
/*																							*/
/*********************************************************************/

#define	RESET_ERR	 			1
#define	REGISTER_INT_ERR		2
#define	INITIALIZE_ERR			3
#define	GET_ROM_VERSION_ERR	4
#define	GET_ADDRESS_ERR		5
#define	SET_ADDRESS_ERR		6
#define	INACTIVITY_ERR			7
#define	DISABLE_HOP_ERR		8
#define	SET_SID_ERR				9
#define	CONFIG_MAC_ERR			10
#define	SET_ROAMING_ERR		11
#define	MULTICAST_ERR			12
#define	CSS_ERR					13
#define	SOCKET_ERR 				14		/* Improper Card Seating		*/
#define	SOCKET_PWR_ERR			15		/* Power Improperly Supplied	*/
#define	SOCKET_RDY_ERR			16		/* No PCMCIA Bus Ready Signal	*/
#define	WINDOW_ERR				17
#define	PCM_DETECT_ERR			18
#define	SET_SC_ERR				19

/*********************************************************************/
/*																							*/
/* Prototype definitions for file "lldinit.c"								*/
/*																							*/
/*********************************************************************/

void 			LLDInitInternalVars (void);
void			PCMCIACleanUp(void);
unsigned_16	LLDInit (void);
unsigned_8	SendInitialize (void);
unsigned_8	SendSetSecurityID (void);
unsigned_8	SendGetGlobalAddr (void);
unsigned_8	SendGetROMVersion (void);
unsigned_8	SendSetITO (void);
unsigned_8	SendConfigMAC (void);
unsigned_8	SendDisableHop (void);
unsigned_8	SendSetRoaming (void);
unsigned_8	SendDisableEEPROMWrite (void);
unsigned_8	SendEnableEEPROMWrite (void);
unsigned_8	WaitFlagSet (unsigned_8 *Flag);
int 			LLDMacOptimize (int MACOptimize);
int 			LLDRoamConfig(int RoamConfig);
unsigned_8	SendSearchContinue(void);
unsigned_8	SendAbortSearch(void);
void			LLDChooseMaster(void);
#endif /* LLDINIT_H */
