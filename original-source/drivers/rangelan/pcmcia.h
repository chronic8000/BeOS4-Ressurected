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
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\pcmcia.h_v  $										*/
/* 
/*    Rev 1.8   15 Aug 1996 11:07:40   JEAN
/* Added a function prototype.
/* 
/*    Rev 1.7   14 Jun 1996 16:43:40   JEAN
/* Modified a function prototype.
/* 
/*    Rev 1.6   22 Mar 1996 14:54:56   JEAN
/* Added support for one-piece PCMCIA.
/* 
/*    Rev 1.5   08 Mar 1996 19:29:24   JEAN
/* Changed the CCReg address for a one-pieced PCMCIA card.
/* 
/*    Rev 1.4   06 Feb 1996 14:37:02   JEAN
/* Comment changes.
/* 
/*    Rev 1.3   31 Jan 1996 13:47:42   Jean
/* Added duplicate header include detection.
/* 
/*    Rev 1.2   22 Sep 1994 10:56:34   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:36   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:48   hilton
/* Initial revision.
/* 																								*/
/*																									*/
/***************************************************************************/

#ifndef PCMCIA_H
#define PCMCIA_H

/**************************************************************************/
/*																								  */
/* Equates for PCIC registers															  */
/*																								  */
/**************************************************************************/

#define CCREG_1PIECE		0x100
#define PCICINTREG		0x276
#define PCICIndexReg		0x3E0
#define PCICDataReg		0x3E1
#define CCREG				0x800

#define IFStatusReg		0x01
#define PwrCtlReg			0x02
#define INTCtlReg			0x03
#define WinEnableReg		0x06
#define IOCtlReg			0x07
#define IOAddr0Start		0x08
#define IOAddr0Stop		0x0A
#define Win0MemStart		0x10
#define Win0MemStop		0x12
#define Win0Offset		0x14



/**************************************************************************/
/*																								  */
/* These are the prototype definitions for "pcmcia.c"							  */
/*																								  */
/**************************************************************************/

unsigned_16		ConfigureMachine (void);
unsigned_16		ConfigureSocket (void);
void				SetPCMCIAForInterruptType (void);
void				ClearPCMCIAInt (void);
void				TurnPowerOff (void);
unsigned_16		InsertedPCMCIACard (unsigned_8 IRQLine, unsigned_32 WindowBase,
											  unsigned_16 IOAddress, unsigned_16 Socket);
unsigned_16		RemovedPCMCIACard (void);
unsigned_16		ParameterRangeCheck (void);
void				WritePCICReg (unsigned_8 Index, unsigned_8 Value);
unsigned_8		ReadPCICReg (unsigned_8 Index);
int				CheckForOnePiece (void);
void				W_DummyReg (unsigned_8 Value);

#endif /* PCMCIA_H */
