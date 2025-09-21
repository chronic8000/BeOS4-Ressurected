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
/*	$Header:   J:\pvcs\archive\clld\lldpack.c_v   1.28   07 May 1998 09:28:20   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldpack.c_v  $										*/
/* 
/*    Rev 1.28   07 May 1998 09:28:20   BARBARA
/* memset changed to SetMemory. memcpy changed to CopyMemory.
/* 
/*    Rev 1.27   24 Mar 1998 08:45:22   BARBARA
/* LLDGotoSniffMode added. InSearchContinueMode added.
/* 
/*    Rev 1.26   12 Sep 1997 16:52:50   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.25   10 Apr 1997 14:23:44   BARBARA
/* (uchar) casting added to "LLDPMSync << 1" and "((PacketSeqNum+1)&SEQNOMASK))"
/* 
/*    Rev 1.24   15 Jan 1997 17:31:20   BARBARA
/* LLDPMSync, LLDMissedSync added. IncPacketSeqNum changed the way of calcul. 
/* seq. num. PICLocalAddress | with 01 instead off 80 for node override.
/* 
/*    Rev 1.23   27 Sep 1996 09:03:12   JEAN
/* 
/*    Rev 1.22   14 Jun 1996 16:12:24   JEAN
/* Comment changes.
/* 
/*    Rev 1.21   16 Apr 1996 11:26:34   JEAN
/* Added a function, LLDGetRFNCStats ().
/* 
/*    Rev 1.20   29 Mar 1996 11:34:00   JEAN
/* Added code in LLDSendInitializeCommand () when no local node
/* override is present, to initialize the local node address to
/* zeros instead of just initializing the first byte to zero
/* and the reset left as garbage.
/* 
/*    Rev 1.19   22 Mar 1996 14:42:04   JEAN
/* Fixed a problem in LLDSetPromiscuous () where we were always setting
/* the card in normal mode.  Also added code to save the current mode
/* in LLDOpMode.
/* 
/*    Rev 1.18   12 Mar 1996 10:39:58   JEAN
/* Added two new functions for sending the Disable/Enable
/* EEPROM Write packetized command.s
/* 
/*    Rev 1.17   08 Mar 1996 19:01:12   JEAN
/* Added a function to send the serial RFNC card a soft baud
/* rate packetized command.
/* 
/*    Rev 1.16   04 Mar 1996 15:04:28   truong
/* Fixed multicast enable routine.
/* 
/*    Rev 1.15   31 Jan 1996 12:58:58   JEAN
/* Changed some constants to macros and changed the ordering
/* of some header include files.
/* 
/*    Rev 1.14   19 Dec 1995 18:06:06   JEAN
/* Added a header include file.
/* 
/*    Rev 1.13   14 Dec 1995 15:10:28   JEAN
/* Added a header include file.
/* 
/*    Rev 1.12   30 Nov 1995 13:52:54   JEAN
/* Fixed a bug setting the sniff time to 4 times the inactivity time
/* out instead of just the inactivity time out.
/* 
/*    Rev 1.11   12 Oct 1995 11:35:26   JEAN
/* Added Header keyword.
/* 
/*    Rev 1.10   10 Sep 1995 10:00:02   truong
/* Added support for LLDSyncRSSIThreshold.
/* 
/*    Rev 1.9   24 Jul 1995 18:36:30   truong
/* 
/*    Rev 1.8   20 Jun 1995 15:49:40   hilton
/* Added support for protcol promiscuous
/* 
/*    Rev 1.7   16 Mar 1995 16:14:40   tracie
/* Added support for ODI
/* 
/*    Rev 1.6   22 Feb 1995 10:30:26   tracie
/* 
/*    Rev 1.5   05 Jan 1995 09:53:50   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.4   14 Dec 1994 11:04:42   tracie
/* Modified to work with the Serial LLD (8250 UART)
/* 
/*    Rev 1.3   29 Nov 1994 12:44:48   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:14   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:04   hilton
/* 
/*    Rev 1.0   20 Sep 1994 11:00:22   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/


#include <string.h>
#include	"constant.h"
#include "lld.h"
#include	"asm.h"
#include	"pstruct.h"
#include "lldpack.h"
#include "lldsend.h"
#include	"bstruct.h"
#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#include "slldhw.h"
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8		NodeOverrideFlag;
extern unsigned_8		LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_8		LLDChannel;
extern unsigned_8		LLDSubChannel;
extern unsigned_16  	LLDInactivityTimeOut;
extern unsigned_8		LLDNodeType;
extern unsigned_8		LLDHopPeriod;
extern unsigned_8		LLDBFreq;
extern unsigned_8		LLDSFreq;
extern unsigned_8		LLDDomain;
extern unsigned_8		LLDDisableHop;
extern unsigned_8		LLDSecurityID [3];
extern unsigned_8		LLDDeferralSlot;
extern unsigned_8		LLDFairnessSlot;
extern unsigned_8		LLDMSTAName [NAMELENGTH+1];
extern unsigned_8		LLDBDWakeup;
extern unsigned_8		LLDSyncName [NAMELENGTH+1];
extern unsigned_8		LLDTransferMode;
extern unsigned_8		LLDRSSIThreshold;
extern unsigned_8		LLDSyncRSSIThreshold;
extern unsigned_8		LLDSniffTime;
extern unsigned_8		LLDRoamingFlag;
extern unsigned_8		LLDPointToPoint;
extern unsigned_8		LLDOpMode;
extern unsigned_8		LLDSyncAlarm;
extern unsigned_8		LLDRetryThresh;
extern unsigned_8		LLDPMSync;
extern unsigned_8		LLDMissedSync;

extern unsigned_16  	RoamStartTime;

extern unsigned_8		PacketSeqNum;
extern unsigned_8		PacketizePktBuf [128];
extern unsigned_8		LLDNumMastersFound;
extern unsigned_8		InSearchContinueMode;

unsigned_8				MulticastOnFlag;

#endif



/***************************************************************************/
/*																									*/
/* It is unlikely anyone will want to change these.  If they do, they 		*/
/*	will probably want to change it for all cards.  Therefore, leave these	*/
/*	parameters outside of the multi-card structure.									*/
/*																									*/
/***************************************************************************/

unsigned_8	LLDRegularMACRetry = 7;
unsigned_8	LLDFragMACRetry = 10;
unsigned_8	LLDRegularMACQFSK = 2;
unsigned_8	LLDFragMACQFSK = 5;




/***************************************************************************/
/*																									*/
/*	LLDSendInitializeCommand ()															*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or Failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send an initialize packet to the card		*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendInitializeCommand (void)

{	struct PacketizeInitCmd *CmdPktPtr;

	CmdPktPtr = (struct PacketizeInitCmd *) PacketizePktBuf;

	CmdPktPtr->PICHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PICHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PICHeader.PHFnNo = INITIALIZE_FN;
	CmdPktPtr->PICHeader.PHRsv = RESERVED;

	if (NodeOverrideFlag)
	{	CopyMemory (CmdPktPtr->PICLocalAddress, &LLDNodeAddress, ADDRESSLENGTH);
 		CmdPktPtr->PICLocalAddress [0] |= 0x01;
	}
	else
	{	SetMemory (CmdPktPtr->PICLocalAddress, 0, ADDRESSLENGTH);
	}

	CmdPktPtr->PICOpMode = LLDOpMode;
	CmdPktPtr->PICNodeType = LLDNodeType;
	CmdPktPtr->PICHopPeriod = LLDHopPeriod;
	CmdPktPtr->PICBFreq = LLDBFreq;
	CmdPktPtr->PICSFreq = LLDSFreq;
	CmdPktPtr->PICChSub = (unsigned_8)((LLDChannel << 4) | LLDSubChannel);

	CopyMemory (CmdPktPtr->PICMSTAName, &LLDMSTAName, NAMELENGTH);

	CmdPktPtr->PICDomain = (unsigned_8)(LLDDomain << 4);

	if (LLDRoamingFlag)
	{	CmdPktPtr->PICControl = 0;
	}
	else
	{	CmdPktPtr->PICControl = 1;
	}

	if (LLDPointToPoint)
	{	CmdPktPtr->PICControl |= 0x80;
	}

	CmdPktPtr->PICReserved = 0;

	CmdPktPtr->PICListSize = 1;
	if (LLDNumMastersFound)
		CmdPktPtr->PICSyncChSub = (unsigned_8)((LLDChannel << 4) | LLDSubChannel);
	else
		CmdPktPtr->PICSyncChSub = 0;
	CopyMemory (CmdPktPtr->PICSyncName, &LLDSyncName, NAMELENGTH);
	LLDSyncName [NAMELENGTH] = 0;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeInitCmd)))
	{	return (FAILURE);
	}
	return (SUCCESS);

}

/***************************************************************************/
/*																									*/
/*	LLDSearchContinue ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a set inactivity timeout packet		*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSearchContinue (void)

{	struct PacketizeSearchCont *CmdPktPtr;

	InSearchContinueMode = 1;
	CmdPktPtr = (struct PacketizeSearchCont *) PacketizePktBuf;

	CmdPktPtr->PSCHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSCHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSCHeader.PHFnNo = SEARCH_N_CONT_FN;
	CmdPktPtr->PSCHeader.PHRsv = RESERVED;

	CmdPktPtr->PSCDomain = (unsigned_8) (LLDDomain << 4);
  	CmdPktPtr->PSCListSize = 1;
	CmdPktPtr->PSCSyncChSub = 0;
	SetMemory(CmdPktPtr->PSCSyncName, 0 , NAMELENGTH);

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSearchCont)))
	{	return (FAILURE);
	}
	return (SUCCESS);

}

/***************************************************************************/
/*																									*/
/*	LLDSetITO ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a set inactivity timeout packet		*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetITO (void)

{	struct PacketizeSetITO *CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetITO *) PacketizePktBuf;

	CmdPktPtr->PSITOHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSITOHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSITOHeader.PHFnNo = SET_ITO_FN;
	CmdPktPtr->PSITOHeader.PHRsv = RESERVED;

	CmdPktPtr->PSITOLength = 3;
	CmdPktPtr->PSITOSniffTime = (unsigned_8)LLDInactivityTimeOut;
	CmdPktPtr->PSITOControl = LLDBDWakeup;
	LLDPMSync = (unsigned_8)(LLDPMSync << 1);
	CmdPktPtr->PSITOControl |= LLDPMSync;

	if (LLDInactivityTimeOut)
		CmdPktPtr->PSITOStbyTime = LLDSniffTime;
	else
		CmdPktPtr->PSITOStbyTime = 0;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetITO)))
	{	return (FAILURE);
	}
	return (SUCCESS);

}



/***************************************************************************/
/*																									*/
/*	LLDSendGotoStandbyCmd ()																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a send goto standby packet				*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendGotoStandbyCmd (void)

{	struct PacketizeGoStandby *CmdPktPtr;

	CmdPktPtr = (struct PacketizeGoStandby *) PacketizePktBuf;

	CmdPktPtr->PGSBHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PGSBHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGSBHeader.PHFnNo = GOTO_STANDBY_FN;
	CmdPktPtr->PGSBHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGoStandby)))
	{	return(FAILURE);
	}
	return (SUCCESS);
}

/***************************************************************************/
/*																									*/
/*	LLDGotoSniffMode ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a send goto sniff mode packet			*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGotoSniffMode (void)

{	struct PacketizeGoStandby *CmdPktPtr;

	CmdPktPtr = (struct PacketizeGoStandby *) PacketizePktBuf;

	CmdPktPtr->PGSBHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PGSBHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGSBHeader.PHFnNo = GOTO_SNIFF_MODE;
	CmdPktPtr->PGSBHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGoStandby)))
	{	return(FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDDisableHopping ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a disable hopping command				*/
/* 	packet to the card																	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDDisableHopping (void)

{	struct PacketizeDHop *CmdPktPtr;

	CmdPktPtr = (struct PacketizeDHop *) PacketizePktBuf;

	CmdPktPtr->PDHHeader.PHCmd = DIAG_COMMANDS_CMD;
	CmdPktPtr->PDHHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PDHHeader.PHFnNo = DISABLE_HOP_FN;
	CmdPktPtr->PDHHeader.PHRsv = RESERVED;
	CmdPktPtr->PDHFrequency = LLDDisableHop;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeDHop)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDGetROMVersion ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a get rom version packet				*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGetROMVersion (void)

{	struct PacketizeGetROMVer *CmdPktPtr;

	CmdPktPtr = (struct PacketizeGetROMVer *) PacketizePktBuf;

	CmdPktPtr->PGRVHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PGRVHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGRVHeader.PHFnNo = GET_ROM_VER_FN;
	CmdPktPtr->PGRVHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGetROMVer)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDConfigMAC ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a configure MAC packet					*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDConfigMAC (void)

{	struct PacketizeConfigMAC *CmdPktPtr;

	CmdPktPtr = (struct PacketizeConfigMAC *) PacketizePktBuf;

	CmdPktPtr->PCMHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PCMHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PCMHeader.PHFnNo = CONFIG_MAC_FN;
	CmdPktPtr->PCMHeader.PHRsv = RESERVED;

	CmdPktPtr->PCMContention = (unsigned_8)(((LLDDeferralSlot << 3) | LLDFairnessSlot));
	CmdPktPtr->PCMRegularRetry = LLDRegularMACRetry;
	CmdPktPtr->PCMFragRetry = LLDFragMACRetry;
	CmdPktPtr->PCMRegularQFSK = LLDRegularMACQFSK;
	CmdPktPtr->PCMFragQFSK = LLDFragMACQFSK;
	CmdPktPtr->PCMsobto [0] = 0xFF;
	CmdPktPtr->PCMsobto [1] = 0xFF;
	CmdPktPtr->PCMsobto [2] = 0xFF;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeConfigMAC)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDSearchAndSync ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a search and synchronize				*/
/*		packet to the card																	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSearchAndSync (void)

{	unsigned_8	i;

	struct PacketizeSearchSync *CmdPktPtr;

	CmdPktPtr = (struct PacketizeSearchSync *) PacketizePktBuf;

	CmdPktPtr->PSSHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSSHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSSHeader.PHFnNo = SEARCH_N_SYNC_FN;
	CmdPktPtr->PSSHeader.PHRsv = RESERVED;

	for ( i = 0; i < 23; i++ )
	{	CmdPktPtr->PSSReserved [i] = 0;
	}

	CmdPktPtr->PSSDomain = (unsigned_8)(LLDDomain << 4);

	if (LLDRoamingFlag)
	{	CmdPktPtr->PSSControl = 0;
	}
	else
	{	CmdPktPtr->PSSControl = 1;
	}

	CmdPktPtr->PSSReserved1 = 0;
	CmdPktPtr->PSSListSize = 1;
	CmdPktPtr->PSSSyncChSub = 0;

	for ( i = 0; i < NAMELENGTH; i++ )
	{	CmdPktPtr->PSSSyncName [i] = 0;
	}

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSearchSync)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDAbortSearch ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send an abort search packet					*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDAbortSearch (void)

{	struct PacketizeAbortSearch *CmdPktPtr;

	InSearchContinueMode = 0;
	CmdPktPtr = (struct PacketizeAbortSearch *) PacketizePktBuf;

	CmdPktPtr->PASHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PASHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PASHeader.PHFnNo = ABORT_SEARCH_FN;
	CmdPktPtr->PASHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeAbortSearch)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDGetGlobalAddress ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a get global address packet			*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGetGlobalAddress (void)

{	struct PacketizeGetGlobal *CmdPktPtr;

	CmdPktPtr = (struct PacketizeGetGlobal *) PacketizePktBuf;

	CmdPktPtr->PGGHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PGGHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGGHeader.PHFnNo = GET_ADDRESS_FN;
	CmdPktPtr->PGGHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGetGlobal)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/* LLDSetRoaming ()																			*/
/*																									*/
/* INPUT:																						*/
/*																									*/
/* OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION:  This routine will send a Set Roaming Parameters packet		*/
/*		to the card.																			*/
/* 																								*/
/***************************************************************************/

unsigned_8 LLDSetRoaming (void)

{	struct PacketizeSetRoaming *CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetRoaming *) PacketizePktBuf;

	CmdPktPtr->PSRPHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSRPHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSRPHeader.PHFnNo = SET_ROAMING_FN;
	CmdPktPtr->PSRPHeader.PHRsv = RESERVED;

	CmdPktPtr->PSRPAlarm = LLDSyncAlarm;
	CmdPktPtr->PSRPRetry = LLDRetryThresh;
	CmdPktPtr->PSRPRSSI = LLDRSSIThreshold;
   CmdPktPtr->PSRPSyncRSSIEnable = 0x5a;
	CmdPktPtr->PSRPSyncRSSI = LLDSyncRSSIThreshold;
   CmdPktPtr->PSRPMissedSyncEnable = 0xa5;
	CmdPktPtr->PSRPMissedSync = LLDMissedSync;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetRoaming)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDSetMulticast (OnOrOff)																*/
/*																									*/
/*	INPUT:	OnOrOff	-	Non-zero activates multicast.								*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a set multicast packet					*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetMulticast (unsigned_8 OnOrOff)

{	struct PacketizeSetMult *CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetMult *) PacketizePktBuf;

	CmdPktPtr->PSMHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSMHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSMHeader.PHFnNo = SET_MULTICAST_FN;
	CmdPktPtr->PSMHeader.PHRsv = RESERVED;
	CmdPktPtr->PSMSwitch = OnOrOff;

	if (OnOrOff == 0)
	{
		MulticastOnFlag = CLEAR;
	}
	else
	{
		MulticastOnFlag = SET;
	}

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetMult)))
	{	return (FAILURE);
	}

	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDSetPromiscuous (Mode)																*/
/*																									*/
/*	INPUT:	Mode		-	Turn on/off promiscuous (0 = off, 1 = on)				*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send an initialize packet to the card		*/
/*																									*/
/***************************************************************************/

unsigned_8	LLDSetPromiscuous (unsigned_8 Mode)

{	struct PacketizeInitCmd *CmdPktPtr;

	CmdPktPtr = (struct PacketizeInitCmd *) PacketizePktBuf;

	CmdPktPtr->PICHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PICHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PICHeader.PHFnNo = INITIALIZE_FN;
	CmdPktPtr->PICHeader.PHRsv = RESERVED;

	CmdPktPtr->PICOpMode = Mode;

	if (NodeOverrideFlag)
	{	CopyMemory( CmdPktPtr->PICLocalAddress, LLDNodeAddress, ADDRESSLENGTH );
 		CmdPktPtr->PICLocalAddress [0] |= 0x01;
	}
	else
	{	CmdPktPtr->PICLocalAddress [0] = 0;
	}

	CmdPktPtr->PICNodeType = LLDNodeType;
	CmdPktPtr->PICHopPeriod = LLDHopPeriod;
	CmdPktPtr->PICBFreq = LLDBFreq;
	CmdPktPtr->PICSFreq = LLDSFreq;
	CmdPktPtr->PICChSub = (unsigned_8)((LLDChannel << 4) | LLDSubChannel);

	CopyMemory( CmdPktPtr->PICMSTAName, &LLDMSTAName, NAMELENGTH );

	CmdPktPtr->PICDomain = (unsigned_8)(LLDDomain << 4);

	if (LLDRoamingFlag)
	{	CmdPktPtr->PICControl = 0;
	}
	else
	{	CmdPktPtr->PICControl = 1;
	}

	CmdPktPtr->PICReserved = 0;

	CmdPktPtr->PICListSize = 1;
	if (LLDNumMastersFound)
		CmdPktPtr->PICSyncChSub = (unsigned_8)((LLDChannel << 4) | LLDSubChannel);
	else
		CmdPktPtr->PICSyncChSub = 0;
	CopyMemory( CmdPktPtr->PICSyncName, &LLDSyncName, NAMELENGTH );
	LLDSyncName [NAMELENGTH] = 0;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeInitCmd)))
	{	return (FAILURE);
	}
	LLDOpMode = Mode;
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDSendKeepAlive ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a keep alive packet to the card		*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendKeepAlive (void)

{
	struct PacketizeKeepAlive *CmdPktPtr;

	CmdPktPtr = (struct PacketizeKeepAlive *) PacketizePktBuf;

	CmdPktPtr->PKAHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PKAHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PKAHeader.PHFnNo = KEEP_ALIVE_FN;
	CmdPktPtr->PKAHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeKeepAlive)))
	{	return (FAILURE);
	}
	return (SUCCESS);

}



/***************************************************************************/
/*																									*/
/*	LLDSetSecurityID ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a set security ID packet				*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetSecurityID (void)

{	struct PacketizeSetSecurityID *CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetSecurityID *) PacketizePktBuf;

	CmdPktPtr->PSSIDHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PSSIDHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSSIDHeader.PHFnNo = SET_SECURITY_ID_FN;
	CmdPktPtr->PSSIDHeader.PHRsv = RESERVED;

	CmdPktPtr->PSSIDByte [0] = (unsigned_8)(LLDSecurityID [0] & 0x0F);
	CmdPktPtr->PSSIDByte [1] = LLDSecurityID [1];
	CmdPktPtr->PSSIDByte [2] = LLDSecurityID [2];

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof (struct PacketizeSetSecurityID)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}




/***************************************************************************/
/*																									*/
/*	LLDRoam ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a roam command packet					*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDRoam (void)

{	struct PacketizeRoam *CmdPktPtr;

	CmdPktPtr = (struct PacketizeRoam *) PacketizePktBuf;

	CmdPktPtr->PRHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PRHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PRHeader.PHFnNo = ROAM_FN;
	CmdPktPtr->PRHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof (struct PacketizeRoam)))
	{	return (FAILURE);
	}
	
	RoamStartTime = LLSGetCurrentTime ();
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDDisableEEPROMWrite ()																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a disable EEPROM write packet			*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDDisableEEPROMWrite (void)

{	struct PacketizeDisableEEPROMWrite *CmdPktPtr;

	CmdPktPtr = (struct PacketizeDisableEEPROMWrite *) PacketizePktBuf;

	CmdPktPtr->PDEWHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PDEWHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PDEWHeader.PHFnNo = DISABLE_EEPROM_FN;
	CmdPktPtr->PDEWHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeDisableEEPROMWrite)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDEnableEEPROMWrite ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send an enable EEPROM write packet			*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDEnableEEPROMWrite (void)

{	struct PacketizeEnableEEPROMWrite *CmdPktPtr;

	CmdPktPtr = (struct PacketizeEnableEEPROMWrite *) PacketizePktBuf;

	CmdPktPtr->PEEWHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PEEWHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PEEWHeader.PHFnNo = ENABLE_EEPROM_FN;
	CmdPktPtr->PEEWHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeEnableEEPROMWrite)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDGetRFNCStats ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a get statistics packet	to the card	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGetRFNCStats (void)

{	struct PacketizeGetStats	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeGetStats *) PacketizePktBuf;

	CmdPktPtr->PGSHeader.PHCmd = INIT_COMMANDS_CMD;
	CmdPktPtr->PGSHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGSHeader.PHFnNo = GET_STATS_FN;
	CmdPktPtr->PGSHeader.PHRsv = RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGetStats)))
	{	return (FAILURE);
	}
	return (SUCCESS);
}



#ifdef SERIAL

/***************************************************************************/
/*																									*/
/*	LLDSendSoftBaudCmd ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	Success or failure											*/
/*																									*/
/*	DESCRIPTION: This routine will send a send soft baudrate packet			*/
/* 	to the card																				*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendSoftBaudCmd (unsigned_8 baud)

{	struct PacketizeBaud *CmdPktPtr;

	CmdPktPtr = (struct PacketizeBaud *) PacketizePktBuf;

	CmdPktPtr->PSSBHeader.PHCmd = DIAG_COMMANDS_CMD;
	CmdPktPtr->PSSBHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSSBHeader.PHFnNo = SET_SOFT_BAUD_FN;
	CmdPktPtr->PSSBHeader.PHRsv = RESERVED;

	/* Valid baud rates are 0-7 */
	if (baud > RFNC_BAUD_115200)
	{	return (FAILURE);
	}
	CmdPktPtr->PSSBBaudrate = baud;
	OutChar ('S', GREEN);
	OutChar ('o', GREEN);
	OutChar ('f', GREEN);
	OutChar ('t', GREEN);
	OutChar ('B', GREEN);
	OutChar ('a', GREEN);
	OutChar ('u', GREEN);
	OutChar ('d', GREEN);
	OutHex (baud, GREEN);

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeBaud)))
	{	return (FAILURE);
	}

	/*-------------------------------------------------*/
	/* Delay for 1500 millisecond to give the FW time 	*/
	/* to change it's baud rate. 	 This delay value is */
	/* to accomodate baud rates down to 1200 baud.		*/
	/*-------------------------------------------------*/
	Delay (1500);

	/*-------------------------------------------------*/
	/* Reprogram our UART so we can talk to the RFNC. */
	/*-------------------------------------------------*/
	SerialInit (baud);
	outDummyChar ();

	return (SUCCESS);
}
#endif


/***************************************************************************/
/*																									*/
/*	IncPacketSeqNum ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT: 																						*/
/*																									*/
/*	DESCRIPTION: This routine will increment the variable "PacketSeqNum"		*/
/*		masking out the raw bit and making sure it doesn't equal the 			*/
/*		repeat sequence number.																*/
/*																									*/
/***************************************************************************/

void IncPacketSeqNum (void)

{	
	do
	{	PacketSeqNum = (unsigned_8)((PacketSeqNum + 1) & SEQNOMASK);
	}	while (PacketSeqNum > MAX_DATA_SEQNO);
/*
	if (++PacketSeqNum == REPEAT_SEQ_NUMBER)
 		PacketSeqNum++;
 	PacketSeqNum &= SEQNOMASK;
*/

}
