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
/*	$Header:   J:\pvcs\archive\clld\lld.c_v   1.52   15 Sep 1998 13:49:22   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lld.c_v  $											*/
/* 
/*    Rev 1.52   15 Sep 1998 13:49:22   BARBARA
/* LLDReadPortOff added.
/* 
/*    Rev 1.51   11 Aug 1998 11:42:06   BARBARA
/* ResetStatisticsTime changed form unsigned_16 to unsingend_32.
/* 
/*    Rev 1.50   03 Jun 1998 14:32:32   BARBARA
/* Proxim protocol variables added.
/* 
/*    Rev 1.49   07 May 1998 09:31:38   BARBARA
/* memset became SetMemory and memcpy became CopyMemory.
/* 
/*    Rev 1.48   25 Mar 1998 07:26:24   BARBARA
/* InSearchContinueMode added as well as LLD Ping and Inactivity Sec < 5 sec 
/* variables added.
/* 
/*    Rev 1.47   12 Sep 1997 16:53:54   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.46   11 Apr 1997 16:17:12   BARBARA
/* CLLDVersion changed to 1.4.
/* 
/*    Rev 1.45   10 Apr 1997 17:20:38   BARBARA
/* LLDuISA changed to LLDMicroISA.
/* 
/*    Rev 1.44   10 Apr 1997 17:07:18   BARBARA
/* LLDuISA added.
/* 
/*    Rev 1.43   10 Apr 1997 14:35:26   BARBARA
/* uISA variable added.
/* 
/*    Rev 1.42   24 Jan 1997 17:25:32   BARBARA
/* LLDNAKTime added.
/* 
/*    Rev 1.41   15 Jan 1997 18:03:56   BARBARA
/* Slow Roaming and TxRoamNotify related variables added.
/* 
/*    Rev 1.40   26 Sep 1996 11:19:18   JEAN
/* Updated the version number.
/* 
/*    Rev 1.39   24 Sep 1996 16:45:20   BARBARA
/* LLDFastIO flag has been added.
/* 
/*    Rev 1.38   13 Sep 1996 11:55:32   JEAN
/* Updated the version number string for rl2setup 3.18.
/* 
/*    Rev 1.37   09 Sep 1996 16:27:50   JEAN
/* Updated the version number string.
/* 
/*    Rev 1.36   29 Jul 1996 12:23:24   JEAN
/* Updated the version number string.
/* 
/*    Rev 1.35   14 Jun 1996 16:14:52   JEAN
/* Made changes for a one piece PCMCIA card.
/* 
/*    Rev 1.34   14 May 1996 09:35:08   TERRYL
/* Added more debug information
/* 
/*    Rev 1.33   02 May 1996 14:01:28   JEAN
/* 
/*    Rev 1.32   23 Apr 1996 09:27:00   JEAN
/* Added some configuration variables.
/* 
/*    Rev 1.31   18 Apr 1996 11:46:58   TERRYL
/* 
/*    Rev 1.29   16 Apr 1996 13:45:46   JEAN
/* Updated the version number.
/* 
/*    Rev 1.28   16 Apr 1996 11:29:06   JEAN
/* Updated the version number and fixed a spelling error.
/* 
/*    Rev 1.27   01 Apr 1996 09:34:12   JEAN
/* Updated the version number.
/* 
/*    Rev 1.26   29 Mar 1996 11:40:52   JEAN
/* Moved 3 variable declarations because they are now defined
/* by the ODI driver.
/* 
/*    Rev 1.25   22 Mar 1996 14:46:50   JEAN
/* Added support for LLDOnePiece and LLDPeerToPeerFlag and some
/* new statistics.
/* 
/*    Rev 1.24   15 Mar 1996 13:59:16   TERRYL
/* Added LLDPeerToPeerFlag
/* 
/*    Rev 1.23   13 Mar 1996 17:51:34   JEAN
/* Added a statistic and updated the version number.
/* 
/*    Rev 1.22   12 Mar 1996 10:40:42   JEAN
/* Changed the CLLD version string.
/* 
/*    Rev 1.21   08 Mar 1996 19:05:52   JEAN
/* Added some new internal variables.
/* 
/*    Rev 1.20   06 Feb 1996 14:22:54   JEAN
/* Used defines instead of hard coded constants.
/* 
/*    Rev 1.19   31 Jan 1996 13:05:00   JEAN
/* Changed some variable declarations for ODI driver support and
/* changed some internal variables to configuration variables.  Also
/* changed the ordering of some header include files.
/* 
/*    Rev 1.18   08 Jan 1996 14:14:08   JEAN
/* Added a CLLD version string.
/* 
/*    Rev 1.17   19 Dec 1995 18:07:20   JEAN
/* Added a header include file.
/* 
/*    Rev 1.16   14 Dec 1995 15:12:48   JEAN
/* Added a header include file.
/* 
/*    Rev 1.15   05 Dec 1995 12:05:26   JEAN
/* Changed an #if to an #ifdef for SERIAL.
/* 
/*    Rev 1.14   04 Dec 1995 14:31:54   JEAN
/* Added variables for SERIAL baud rate and fifo trigger level.
/* Also initialized driver statistics to zero.
/* 
/*    Rev 1.13   17 Nov 1995 16:35:08   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.12   12 Oct 1995 11:38:44   JEAN
/* 
/*    Rev 1.11   10 Sep 1995 10:00:58   truong
/* Added LLDSyncRSSIThreshold.
/* Changed defaults for LLDRSSIThreshold from 8 to 15.
/* 
/*    Rev 1.10   24 Jul 1995 18:36:46   truong
/* 
/*    Rev 1.9   20 Jun 1995 15:50:12   hilton
/* Added support for protcol promiscuous
/* 
/*    Rev 1.8   23 May 1995 09:42:12   tracie
/* Global and external variables swapped
/* 
/*    Rev 1.1   24 Apr 1995 16:01:34   tracie
/* Combined version
/* 
/*    Rev 1.7   13 Apr 1995 08:53:46   tracie
/* xromtest version
/* 
/*    Rev 1.0   04 Apr 1995 16:12:16   tracie
/* Initial revision.
/* 
/*    Rev 1.6   05 Jan 1995 09:51:28   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.5   27 Dec 1994 15:42:06   tracie
/* 
/* 																								*/
/*    Rev 1.4   14 Dec 1994 10:54:32   tracie										*/
/* 				 Changes made to work with the Serial LLD 						*/
/*					 (8250 UART working version)											*/
/* 																								*/
/*    Rev 1.3   29 Nov 1994 12:44:14   hilton										*/
/* 																								*/
/*    Rev 1.2   22 Sep 1994 10:56:04   hilton										*/
/* 																								*/
/*    Rev 1.1   20 Sep 1994 16:02:50   hilton										*/
/* 																								*/
/*    Rev 1.0   20 Sep 1994 10:58:24   hilton										*/
/* Initial revision.																			*/
/*																									*/
/*																									*/
/***************************************************************************/


#include <string.h>
#include "constant.h"
#include "lld.h"
#include "asm.h"
#include	"bstruct.h"
#include "pstruct.h"
#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#endif

#ifdef MULTI
#include	"multi.h"

struct LLDVarsStruct		LLDVarsArray [NUMCARDS];
struct LLDVarsStruct		*LLDVars;

#else


/*----------------------------------------------------*/
/* The ODI driver defines some of the LLD variables	*/
/* in it's DriverConfigTable.									*/
/*----------------------------------------------------*/
#ifndef ODI

#ifndef SERIAL											
unsigned_16		LLDIOAddress1 = 0x270;
unsigned_8		LLDIntLine1 = 15;

#else														/*---------------------------*/
unsigned_16		LLDIOAddress1 = 0x2F8;			/* COM port 2 is the default */
unsigned_8		LLDIntLine1   = 3;				/*---------------------------*/
#endif


unsigned_8		LLDChannel = 1;
unsigned_8		LLDSubChannel = 1;
unsigned_8		LLDHopPeriod = 1;
unsigned_8		LLDBFreq = 2;
unsigned_8		LLDSFreq = 7;
unsigned_8		LLDMSTAName [NAMELENGTH+1] = "PROXIM_MSTA";
unsigned_8		LLDOldMSTAAddr [ADDRESSLENGTH];
unsigned_8		LLDDeferralSlot = 0xFF;
unsigned_8		LLDFairnessSlot = 0xFF;
unsigned_8		LLDBridgeFlag = 0;
unsigned_8		LLDSecurityIDOverride = 0;
unsigned_8		LLDSecurityID [3] = {3, 2, 1};
unsigned_8		NextMaster = 0;

unsigned_8		LLDNodeAddress [ADDRESSLENGTH] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned_16		LLDInactivityTimeOut = 0;
unsigned_8		LLDDomain = 0;
unsigned_8		LLDNodeType = ALTERNATE;
unsigned_8		LLDDisableHop = 0xFF;
unsigned_8		LLDBDWakeup = 0;
unsigned_8		LLDSniffTime = 0;

unsigned_16		LLDIORange1 = 6;
unsigned_8		LLDTransferMode = AUTODETECT;
unsigned_8		LLDSocketNumber = 0;
unsigned_8		LLDSlot = 0;
unsigned_32		LLDMemoryAddress1 = 0xD000;
unsigned_16		LLDMemorySize1 = 0;
unsigned_8		LLDDMALine1 = 0;
					
unsigned_8		LLDMaxPowerLevel = 0;
unsigned_8 		LLDReserved = 0;
					
unsigned_16		LLDMaxDataSize = MAX_PKT_SIZE;
unsigned_16		LLDLookAheadSize = LOOKAHEADSIZE;
unsigned_16		LLDFragSize = 310;
unsigned_8		LLDRoamingFlag = 0;
unsigned_16		LLDMaxRoamWaitTime = MAXROAMWAITTIME;
unsigned_8		LLDTxMode = TX_AUTO;
unsigned_8		LLDNumMastersFound = 0;
unsigned_8		LLDSearchTime = 0;

unsigned_8		LLDROMVersion [ROMVERLENGTH] = {0, 0, 0, 0, 0, 0, 0};
unsigned_8		LLDPhysAddress [ADDRESSLENGTH];
unsigned_8		LLDSyncState = 0;
unsigned_8		LLDMSTAAddr [ADDRESSLENGTH] = {0, 0, 0, 0, 0, 0};
unsigned_8		LLDMSTASyncName [NAMELENGTH+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned_8		LLDMSTASyncChannel = 0;
unsigned_8		LLDMSTASyncSubChannel = 0;
					
unsigned_8		LLDPCMCIA = 0;
unsigned_8		LLDMicroISA = 0;
unsigned_8		LLDInit365Flag = 0;
unsigned_8		LLDPointToPoint = 0;
unsigned_8		LLDPeerToPeerFlag=1; /* Peer_to_Peer (Enabled) */
unsigned_8		LLDRoamConfiguration=1; 
unsigned_8		LLDSyncAlarm=SYNCALARMCRITERIA;
unsigned_8		LLDRetryThresh=RETRYTHRESHOLD;
unsigned_8		ConfigureMACFlag=0;
unsigned_8		InSearchContinueMode=0;

unsigned_8		DriverNoResetFlag = 0;
unsigned_8		LLDNoRepeatingFlag = 0;
unsigned_8		LLDDebugColor=0; 		/* Debug color (Disabled) */
unsigned_8		LLDRoamRetryTmr=0;
unsigned_8		LLDRoamResponse=0;

unsigned_8		LLDTxFragmentedPackets=0;
unsigned_8		LLDTxFragRetries=0;
unsigned_8		LLDNAKTime = 100;
unsigned_8		NotesBuffer[NOTES_BUFFER_SIZE];
unsigned_8		StationName[STATION_NAME_SIZE];
unsigned_16		LLDRoamResponseTmr=0;
unsigned_16		LLDUnknownProximPkt=0;
unsigned_8		LLDReadPortOff=0;

unsigned_32		LLDSent=0;			  	/* # packets we sent to RFNC 				*/
unsigned_32		LLDReSent=0; 		  	/* # packets we resent to RFNC 			*/
unsigned_32		LLDSentCompleted=0;	/* # transmit completes Rxed from RFNC */
unsigned_32		LLDRx=0;				  	/* # data packets ISR read			 		*/
unsigned_32		LLDRxLookAhead=0;  	/* # data packets upper layer read 		*/
unsigned_32		LLDErrResetCnt=0;  	/* # time reset due to error condition */
unsigned_32		LLDNeedResetCnt=0; 	/* # time reset due to error condition */
unsigned_32		LLDRxTimeOuts=0;	 	/* # packet Rx timeouts						*/
unsigned_32		LLDTxTimeOuts=0; 		/* # packet Tx timeouts						*/
unsigned_32		LLDOutSyncQDrp=0;		/* Counters for number of packets      */
unsigned_32		LLDOutSyncDrp=0;		/* dropped in LLDSend().					*/
unsigned_32		LLDTimedQDrp=0;
unsigned_32		LLDFailureDrp=0;
unsigned_32		LLDNoReEntrantDrp=0;
unsigned_32		LLDSendDisabledDrp=0;
unsigned_32		LLDZeroRxLen=0;
unsigned_32		LLDTooBigRxLen=0;
unsigned_32		LLDTooBigTxLen=0;
unsigned_32		LLDBadCopyLen=0;
unsigned_32		FramesXmit=0;
unsigned_32		FramesXmitQFSK=0;
unsigned_32		FramesXmitBFSK=0;
unsigned_32		FramesXmitFrag=0;
unsigned_32		FramesACKError=0;
unsigned_32		FramesCTSError=0;
unsigned_32		FramesRx=0;
unsigned_32		FramesRxDuplicate=0;
unsigned_32		FramesRxFrag=0;
unsigned_32		TotalTxPackets=0;
unsigned_32		ResetStatisticsTime;
unsigned_32		LLDConfToolResetCnt=0;
struct TXBStruct	TXBuffers [TOTALTXBUF];
struct TXBStruct	*FreeTXBuffer;


#ifdef SERIAL
unsigned_32		SLLDRxHdrErr=0;		/* # Header Checksum errors 				*/
unsigned_32		SLLDRxChksumErr=0; 	/* # Packet Checksum errors 				*/
unsigned_32		SLLDRxLenErr=0;		/* # Invalid Rx packet length errors 	*/
unsigned_32		SLLDOverrunErr=0;		/* Overrun errors								*/
unsigned_32		SLLDRxErrors=0;		/* All receive errors						*/
#endif


/*
 * Variables defined in the ODI driver.
 */
#else

extern unsigned_16	LLDIOAddress1;
extern unsigned_8		LLDIntLine1;
extern unsigned_8		LLDChannel;
extern unsigned_8		LLDSubChannel;
extern unsigned_8		LLDHopPeriod;
extern unsigned_8		LLDBFreq;
extern unsigned_8		LLDSFreq;
extern unsigned_8		LLDMSTAName [NAMELENGTH+1];
extern unsigned_8		LLDDeferralSlot;
extern unsigned_8		LLDFairnessSlot;
extern unsigned_8		LLDBridgeFlag;
extern unsigned_8		LLDSecurityIDOverride;
extern unsigned_8		LLDSecurityID [3];

extern unsigned_8		LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_16	LLDInactivityTimeOut;
extern unsigned_8		LLDDomain;
extern unsigned_8		LLDNodeType;
extern unsigned_8		LLDDisableHop;
extern unsigned_8		LLDBDWakeup;
extern unsigned_8		LLDSniffTime;

extern unsigned_16	LLDIORange1;
extern unsigned_8		LLDTransferMode;
extern unsigned_8		LLDSocketNumber;
extern unsigned_8		LLDSlot;
extern unsigned_32	LLDMemoryAddress1;
extern unsigned_16	LLDMemorySize1;
extern unsigned_8		LLDDMALine1;
					
extern unsigned_8		LLDMaxPowerLevel;
extern unsigned_8 	LLDReserved;
					
extern unsigned_16	LLDMaxDataSize;
extern unsigned_16	LLDLookAheadSize;
extern unsigned_16	LLDFragSize;
extern unsigned_8		LLDRoamingFlag;
extern unsigned_8		LLDTxMode;
					
extern unsigned_8		LLDROMVersion [ROMVERLENGTH];
extern unsigned_8		LLDPhysAddress [ADDRESSLENGTH];
extern unsigned_8		LLDSyncState;
extern unsigned_8		LLDMSTAAddr [ADDRESSLENGTH];
extern unsigned_8		LLDMSTASyncName [NAMELENGTH+1];
extern unsigned_8		LLDMSTASyncChannel;
extern unsigned_8		LLDMSTASyncSubChannel;
					
extern unsigned_8		LLDPCMCIA;
extern unsigned_8		LLDInit365Flag;
extern unsigned_8		LLDPointToPoint;
extern unsigned_8		LLDPeerToPeerFlag;
extern unsigned_8		LLDSyncAlarm;
extern unsigned_8		LLDRetryThresh;
extern unsigned_8		ConfigureMACFlag;
extern unsigned_8		LLDRSSIThreshold;

extern unsigned_8		DriverNoResetFlag;
extern unsigned_8		LLDNoRepeatingFlag;
extern unsigned_8		LLDDebugColor; 		/* Debug color (Disabled) */

extern unsigned_32	LLDSent;			  	/* # packets we sent to RFNC 				*/
extern unsigned_32	LLDReSent; 		  	/* # packets we resent to RFNC 			*/
extern unsigned_32	LLDSentCompleted;	/* # transmit completes Rxed from RFNC */
extern unsigned_32	LLDRx;			  	/* # data packets ISR read 			   */
extern unsigned_32	LLDRxLookAhead;  	/* # data packets upper layer read 		*/
extern unsigned_32	LLDErrResetCnt;  	/* # time reset due to error condition */
extern unsigned_32	LLDNeedResetCnt; 	/* # time reset due to error condition */
extern unsigned_32	LLDRxTimeOuts;	 	/* # packet Rx timeouts						*/
extern unsigned_32	LLDTxTimeOuts; 	/* # packet Tx timeouts						*/
extern unsigned_32	LLDOutSyncQDrp;
extern unsigned_32	LLDOutSyncDrp;
extern unsigned_32	LLDTimedQDrp;
extern unsigned_32	LLDFailureDrp;
extern unsigned_32	LLDNoReEntrantDrp;
extern unsigned_32	LLDSendDisabledDrp;

#ifdef SERIAL
extern unsigned_8		SLLDBaudrateCode;	/* Represents the current setting. 		*/
extern unsigned_32	SLLDRxHdrErr;		/* # Header Checksum errors 				*/
extern unsigned_32	SLLDRxChksumErr; 	/* # Packet Checksum errors 				*/
extern unsigned_32	SLLDRxLenErr;		/* # Invalid Rx packet length errors 	*/
extern unsigned_32	SLLDOverrunErr;	/* # Overrun errors							*/
extern unsigned_32	SLLDRxErrors;		/* # Receive errors							*/
#endif

#endif

/* Variables not declared in the ODI driver. */
unsigned_8		LLDOEM = 0;
unsigned_8		LLDOpMode = OP_NORMAL;
unsigned_8		LLDTxModeSave = TX_AUTO;
unsigned_8		LLDOnePieceFlag = 0;
unsigned_8		LLDForceFrag=0;
unsigned_8		LLDNoRetries=0;
unsigned_8		LLDFastIO=0;
unsigned_8		LLDPMSync=0;
unsigned_8		LLDMissedSync=4;
unsigned_8		LLDCTSRetries=7;
unsigned_8		LLDOEMCustomer=0;

unsigned_8		LLDSniffModeFlag=0;
unsigned_8		LLDSniffCount=0;
unsigned_8		LLDTicksToSniff=0;
unsigned_8		PostponeSniff=0;

#ifdef SERIAL
unsigned_8		SLLDFIFOTrigger = 4;   /* Interrupt every 4 characters.	*/
#endif

extern unsigned_8		LLDSyncRSSIThreshold;
extern unsigned_8		LLDSyncName [NAMELENGTH+1];

#endif


/* CLLD Version String */

#if SERIAL
unsigned char CLLDVersion[25] = "Version 1.5s";
#else
unsigned char CLLDVersion[25] = "Version 1.5";
#endif



#ifdef MULTI

/***************************************************************************/
/*																									*/
/*	InitLLDVars (Card) 																		*/
/*																									*/
/*	INPUT:			Card	-	Card Number to initialize (1 - 2)					*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine will initialize the user settable LLD			*/
/*		variables.																				*/
/*																									*/
/***************************************************************************/

void InitLLDVars (unsigned_8 Card)

{	struct LLDConfigVars		*LLDConfig;

	LLDConfig = &LLDVarsArray [Card - 1].Config;
	
#ifndef SERIAL											
	LLDConfig->MLLDIOAddress1 = 0x270;
	LLDConfig->MLLDIntLine1 = 15;

#else														/*--------------------------*/
	LLDConfig->MLLDIOAddress1 = 0x3F8;	  		/* Serial UART Base Address */
	LLDConfig->MLLDIntLine1 = 4;		  			/*--------------------------*/
#endif

	LLDConfig->MLLDChannel = 1;
	LLDConfig->MLLDSubChannel = 1;
	LLDConfig->MLLDHopPeriod = 1;
	LLDConfig->MLLDBFreq = 2;
	LLDConfig->MLLDSFreq = 7;
	CopyMemory (&LLDConfig->MLLDMSTAName, "PROXIM_MSTA\0", ADDRESSLENGTH);
	LLDConfig->MLLDDeferralSlot = 0xFF;
	LLDConfig->MLLDFairnessSlot = 0xFF;
	LLDConfig->MLLDBridgeFlag = 0;
	LLDConfig->MLLDSecurityIDOverride = 0;
	SetMemory (&LLDConfig->MLLDSecurityID, 0, 3); 
					
	memset (&LLDConfig->MLLDNodeAddress, 0xFF, ADDRESSLENGTH);
	LLDConfig->MLLDInactivityTimeOut = 0;
	LLDConfig->MLLDDomain = 0;
	LLDConfig->MLLDNodeType = ALTERNATE;
	LLDConfig->MLLDDisableHop = 0xFF;
	LLDConfig->MLLDRSSIThreshold = 15;
	LLDConfig->MLLDSyncRSSIThreshold = 0;
	LLDConfig->MLLDBDWakeup = 0;
	LLDConfig->MLLDSniffTime = 0;

	LLDConfig->MLLDIORange1 = 6;
	LLDConfig->MLLDTransferMode = AUTODETECT;
	LLDConfig->MLLDSocketNumber = 0;
	LLDConfig->MLLDSlot = 0;
	LLDConfig->MLLDMemoryAddress1 = 0xD000;
	LLDConfig->MLLDMemorySize1 = 0;
	LLDConfig->MLLDDMALine1 = 0;
					
	LLDConfig->MLLDMaxPowerLevel = 0;
	LLDConfig->MLLDReserved = 0;
					
	LLDConfig->MLLDMaxDataSize = MAX_PKT_SIZE;
	LLDConfig->MLLDLookAheadSize = LOOKAHEADSIZE;
	LLDConfig->MLLDFragSize = 310;
	LLDConfig->MLLDRoamingFlag = 0;
					
	SetMemory (&LLDConfig->MLLDROMVersion, 0, ROMVERLENGTH); 
	LLDConfig->MLLDSyncState = CLEAR;
	SetMemory (&LLDConfig->MLLDMSTAAddr, 0, ADDRESSLENGTH);
	SetMemory (&LLDConfig->MLLDMSTASyncName, 0, NAMELENGTH+1);
	LLDConfig->MLLDMSTASyncChannel = 0;
	LLDConfig->MLLDMSTASyncSubChannel = 0;
					
	LLDConfig->MLLDOEM = 0;
	LLDConfig->MLLDPCMCIA = 0;
	LLDConfig->MLLDInit365Flag = 0;
	LLDConfig->MLLDOnePieceFlag = 0;
	LLDConfig->MLLDPointToPoint = 0;					
	LLDConfig->MLLDPeerToPeerFlag = 1;
	LLDConfig->MLLDOpMode = OP_NORMAL;
	LLDConfig->MDriverNoResetFlag = 0;
	LLDConfig->MLLDNoRepeatingFlag = 0;
	LLDConfig->MLLDDebugColor=0; 		/* Debug color (Disabled) */
	LLDConfig->MLLDNoRetries=0;
	LLDConfig->MLLDForceFrag=0;
	LLDConfig->MLLDSyncAlarm = SYNCALARMCRITERIA;
	LLDConfig->MLLDRetryThresh = RETRYTHRESHOLD;
	LLDConfig->MConfigureMACFlag = 0;
	LLDConfig->MLLDTxMode = TX_AUTO;

	LLDConfig->MLLDPMSync = 0;
	LLDConfig->MLLDMissedSync = 4;
	LLDConfig->MLLDOEMCustomer = 0;
	LLDConfig->MLLDCTSRetries = 7;

 	LLDConfig->MLLDTicksToSniff = 0;
	LLDConfig->MLLDRoamConfiguration = 1;
	LLDConfig->MLLDReadPortOff = 0;
	SetMemory (&LLDConfig->MStationName, 0, STATION_NAME_SIZE);
	SetMemory (&LLDConfig->MNotesBuffer, 0, NOTES_BUFFER_SIZE);
	SetMemory (&LLDConfig->MDriverVersionString, 0, DRIVERVERLENGTH);

#ifdef SERIAL
	LLDConfig->MSLLDFIFOTrigger = 4;   /* Interrupt every 4 characters.	*/
	LLDConfig->MSLLDBaudrateCode = RFNC_BAUD_19200; 
#endif

}

#endif
