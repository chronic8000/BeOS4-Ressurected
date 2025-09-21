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
/*	$Header:   J:\pvcs\archive\clld\multi.h_v   1.32   24 Mar 1998 09:22:46   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\multi.h_v  $																							*/
/* 
/*    Rev 1.32   24 Mar 1998 09:22:46   BARBARA
/* LLD Ping changes added.
/* 
/*    Rev 1.31   10 Apr 1997 17:21:34   BARBARA
/* LLDuISA changed to LLDMicroISA.
/* 
/*    Rev 1.30   10 Apr 1997 17:08:16   BARBARA
/* LLDuISA added.
/* 
/*    Rev 1.29   10 Apr 1997 14:27:14   BARBARA
/* New variables added to the LLDVars->Config and Internal structure.
/* 
/*    Rev 1.28   26 Sep 1996 10:57:02   JEAN
/* Added a new configuration parameter, LLDFastIO.
/* 
/*    Rev 1.27   15 Aug 1996 10:10:12   JEAN
/* 
/*    Rev 1.26   29 Jul 1996 12:36:52   JEAN
/* Added a new IO register, EOIReg.
/* 
/*    Rev 1.25   14 Jun 1996 16:44:38   JEAN
/* Made the LLDOnePieceFlag a configuration variable instead of
/* a configuration variable.
/* 
/*    Rev 1.24   14 May 1996 09:31:12   TERRYL
/* Added LLDForceFrag flag.
/* 
/*    Rev 1.23   19 Apr 1996 15:45:52   TERRYL
/* Corrected declaration errors on LLDNoRetries and LLDDebugColor.
/* 
/*    Rev 1.22   18 Apr 1996 11:44:22   TERRYL
/* Added LLDNoRestries and LLDDebugColor.
/* 
/*    Rev 1.21   16 Apr 1996 11:41:58   JEAN
/* Added GetRFNCStatsFlag, changed the sizes of uBufPool and
/* uBufTable, and added a uSmallBufSize variable.
/* 
/*    Rev 1.20   22 Mar 1996 14:55:40   JEAN
/* Added some new internal variables, LLDPeerToPeerFlag, CCReg,
/* SaveBIOSAddr, RestoreBIOSTableFlag, LLDRx, and LLDRxLenErr.
/* 
/*    Rev 1.19   13 Mar 1996 17:56:06   JEAN
/* Added a new statistic for counting the number of bad
/* packet lengths.
/* 
/*    Rev 1.18   12 Mar 1996 10:45:16   JEAN
/* Added two internal variables for two new packetized commands.
/* 
/*    Rev 1.17   08 Mar 1996 19:36:28   JEAN
/* Added some internal variables and statistics.
/* 
/*    Rev 1.16   06 Feb 1996 14:37:24   JEAN
/* Removed an used serial variable.
/* 
/*    Rev 1.15   31 Jan 1996 13:48:42   Jean
/* Added duplicate header include detection, used some macros
/* instead of constants, moved some variables from the internal
/* vars structure to the configuration vars structure.
/* 
/*    Rev 1.14   08 Jan 1996 14:23:38   JEAN
/* Added and deleted some variables.
/* 
/*    Rev 1.13   19 Dec 1995 18:14:04   JEAN
/* Added support for multiple receive and transmit buffers
/* for serial cards.
/* 
/*    Rev 1.12   14 Dec 1995 15:40:30   JEAN
/* Added support for multiple serial cards.
/* 
/*    Rev 1.11   07 Dec 1995 12:17:18   JEAN
/* Removed LLDTimerTick.
/* 
/*    Rev 1.10   05 Dec 1995 12:06:30   JEAN
/* Changed an #if to an #ifdef for SERIAL.
/* 
/*    Rev 1.9   17 Nov 1995 16:45:12   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.8   12 Oct 1995 12:11:04   JEAN
/* Added $Log$ PVCS keyword.
/*
/*																									*/
/***************************************************************************/

#ifndef MULTI_H
#define MULTI_H

/***************************************************************************/
/*																									*/
/* These are the structures used to create a driver which will operate		*/
/*	multiple RangeLAN2 cards.  The idea is to allocate a set of variables	*/
/*	for each card and pass a pointer to the structure containing the LLD		*/
/* variables at each entry point into the LLD.										*/
/*																									*/
/***************************************************************************/

struct LLDConfigVars
{	unsigned_16			MLLDIOAddress1;
	unsigned_8			MLLDIntLine1;
	unsigned_8			MLLDChannel;
	unsigned_8			MLLDSubChannel;
	unsigned_8			MLLDHopPeriod;
	unsigned_8			MLLDBFreq;
	unsigned_8			MLLDSFreq;
	unsigned_8			MLLDMSTAName [NAMELENGTH+1];	/* One byte for the NULL char */
	unsigned_8			MLLDDeferralSlot;
	unsigned_8			MLLDFairnessSlot;
	unsigned_8			MLLDBridgeFlag;
	unsigned_8			MLLDSecurityIDOverride;
	unsigned_8			MLLDSecurityID [3];
					
	unsigned_8			MLLDNodeAddress [ADDRESSLENGTH];
	unsigned_16			MLLDInactivityTimeOut;
	unsigned_8			MLLDDomain;
	unsigned_8			MLLDNodeType;
	unsigned_8			MLLDDisableHop;
	unsigned_8			MLLDRSSIThreshold;
	unsigned_8			MLLDSyncRSSIThreshold;
	unsigned_8			MLLDBDWakeup;
	unsigned_8			MLLDSniffTime;

	unsigned_16			MLLDIORange1;
	unsigned_8			MLLDTransferMode;
	unsigned_8			MLLDSocketNumber;
	unsigned_8			MLLDSlot;
	unsigned_32			MLLDMemoryAddress1;
	unsigned_16			MLLDMemorySize1;
	unsigned_8			MLLDDMALine1;
					
	unsigned_8			MLLDMaxPowerLevel;
	unsigned_8 			MLLDReserved;
							
	unsigned_16			MLLDMaxDataSize;
	unsigned_16			MLLDLookAheadSize;
	unsigned_16			MLLDFragSize;
	unsigned_8			MLLDRoamingFlag;
					
	unsigned_8			MLLDROMVersion [ROMVERLENGTH];
	unsigned_8			MLLDPhysAddress [ADDRESSLENGTH];
	unsigned_8			MLLDSyncState;
	unsigned_8			MLLDMSTAAddr [ADDRESSLENGTH];
	unsigned_8			MLLDSyncName [NAMELENGTH+1];
	unsigned_8			MLLDMSTASyncName [NAMELENGTH+1];
	unsigned_8			MLLDMSTASyncChannel;
	unsigned_8			MLLDMSTASyncSubChannel;
					
	unsigned_8			MLLDPCMCIA;
	unsigned_8			MLLDOnePieceFlag;
	unsigned_8			MLLDMicroISA;
	unsigned_8			MLLDInit365Flag;
	unsigned_8			MLLDPointToPoint;					
	unsigned_8			MLLDPeerToPeerFlag;

	unsigned_8			MLLDRepeatingWBRP;	
	unsigned_8			MLLDOEM;
	unsigned_8			MLLDDebugFlag;
	unsigned_8			MLLDOpMode;
	unsigned_8			MLLDFastIO;

	unsigned_8			MDriverNoResetFlag;
	unsigned_8			MLLDNoRepeatingFlag;
	unsigned_8			MLLDDebugColor; 	  
	unsigned_8			MLLDNoRetries;
	unsigned_8			MLLDForceFrag;
	unsigned_8			MLLDSyncAlarm;
	unsigned_8			MLLDRetryThresh;
	unsigned_8			MConfigureMACFlag;
	unsigned_8			MLLDTxMode;

	unsigned_8			MLLDPMSync;
	unsigned_8			MLLDMissedSync;
	unsigned_8			MLLDOEMCustomer;
	unsigned_8			MLLDCTSRetries;
	unsigned_8			MLLDTicksToSniff;
	unsigned_8			MLLDRoamConfiguration;
	unsigned_8			MLLDReadPortOff;
	unsigned_8			*MStationName[STATION_NAME_SIZE];
	unsigned_8			*MNotesBuffer[NOTES_BUFFER_SIZE];
	unsigned_8			*MDriverVersionString[DRIVERVERLENGTH];
#ifdef SERIAL
	unsigned_8			MSLLDFIFOTrigger;
	unsigned_8			MSLLDBaudrateCode;
#endif
};


struct LLDInternalVars
{	unsigned_8			MLLDLookAheadBuf [BUFFERLEN];
	unsigned_16			MLLDLookAheadAddOn;
	unsigned_8			MInDriverISRFlag;
	unsigned_8			MInPollFlag;
	unsigned_8			MInResetFlag;
	unsigned_8			MLLDRoamingState;
	unsigned_16			MRoamStartTime;
	unsigned_8			MLLDMSTAFlag;
	unsigned_8			MLLDSleepFlag;

	unsigned_16			MLLDLivedTime;
	unsigned_16			MLLDKeepAliveTmr;
	unsigned_8			MInLLDSend;
	unsigned_8			MDisableLLDSend;
	unsigned_8			MLLDOutOfSyncCount;
	unsigned_8			MLLDTxModeSave;
	unsigned_8			MTxFlag;
	unsigned_8			MTxFlagCleared;
#ifndef SERIAL
	unsigned_16			MTxStartTime;
#else
	unsigned_16			MTxStartTime[128];
	unsigned_8			MTxInProgress[128];
#endif
	unsigned_8			MLLDTxBuffer [BUFFERLEN];
	unsigned_8			MPacketizePktBuf [128];
	unsigned_8			MSecPacketizePktBuf [128];
	unsigned_8			MPacketSeqNum;
	unsigned_8			MLLDNeedReset;

	unsigned_32			MLLDSent;
	unsigned_32			MLLDReSent;
	unsigned_32			MLLDSentCompleted;
	unsigned_32			MLLDRx;
	unsigned_32			MLLDRxLookAhead;
	unsigned_32			MLLDErrResetCnt;
	unsigned_32			MLLDNeedResetCnt;
	unsigned_32			MLLDRxTimeOuts;
	unsigned_32			MLLDTxTimeOuts;
	unsigned_32			MLLDOutSyncQDrp;
	unsigned_32			MLLDOutSyncDrp;
	unsigned_32			MLLDTimedQDrp;
	unsigned_32			MLLDFailureDrp;
	unsigned_32			MLLDNoReEntrantDrp;
	unsigned_32			MLLDSendDisabledDrp;
	unsigned_32			MLLDZeroRxLen;
	unsigned_32			MLLDTooBigRxLen;
	unsigned_32			MLLDTooBigTxLen;
	unsigned_32			MLLDBadCopyLen;
#ifdef SERIAL
	unsigned_8			MInSerialISRFlag;
	unsigned_8			MRestoreBIOSTableFlag;
	unsigned_16	FAR	*MSaveBIOSAddr;
	unsigned_32			MSLLDRxHdrErr;
	unsigned_32			MSLLDRxChksumErr;
	unsigned_32			MSLLDRxLenErr;
	unsigned_32			MSLLDOverrunErr;
	unsigned_32			MSLLDRxErrors;
#endif

	struct TXQEntry	MTXQMemory [LLDMAXSEND];
	struct TXQStruct	MTXQueue;
	struct TCBQEntry	MTCBQMemory [LLDMAXSEND];
	struct TCBQEntry	*MFreeTCBQEntry;
	struct NodeEntry	MNodeTable [TOTALENTRY];
	unsigned_8			MLLDMapTable [128];			/* Indexed by sequence number */
	unsigned_8			MHashTable [256];				/* Indexed by last byte in MAC address */
	unsigned_8			MNextFreeNodeEntry;
	unsigned_8			MMRUNodeEntry;
	unsigned_8			MLRUNodeEntry;

	unsigned_8			MMulticastOnFlag;

	unsigned_8			MIntStack [32];
	unsigned_8			MIntStackPtr;

	unsigned_8			MSetMulticastFlag;
	unsigned_16			MLookAheadLength;
	unsigned_16			MRawLookAheadLength;
	unsigned_16			MHardwareHeader;
	unsigned_8 			*MLLDLookAheadBufPtr;
	unsigned_8			MRxFlag;
	unsigned_16			MRxStartTime;
	struct RXBStruct	MRXBuffers [TOTALRXBUF];
	struct RXBStruct	*MFreeRXBuffer;
	struct TXBStruct	MTXBuffers [TOTALTXBUF];
	struct TXBStruct	*MFreeTXBuffer;

	unsigned_16			MStatusReg;
	unsigned_8			MStatusValue;	
	unsigned_16			MControlReg;
	unsigned_16			MEOIReg;
	unsigned_8			MControlValue;
	unsigned_16			MDataReg;
	unsigned_16			MIntSelReg;
	unsigned_8			MIntSelValue;
	unsigned_8			MNodeOverrideFlag;
	unsigned_8			MLLDSecurityIDString [3];
	unsigned_8			MSendInitCmdFlag;
	unsigned_8			MSetSecurityIDFlag;
	unsigned_8			MGetGlobalAddrFlag;
	unsigned_8			MGetROMVersionFlag;
	unsigned_8			MSetITOFlag;
	unsigned_8			MDisableHopFlag;
	unsigned_8			MSetRoamingFlag;
	unsigned_8			MDisableEEPROMFlag;
	unsigned_8			MEnableEEPROMFlag;
	unsigned_8			MGetRFNCStatsFlag;

	unsigned_16			MCSSClientHandle;
	unsigned_16			MCORAddress;
	unsigned_8			MPCMCIACardInserted;
	unsigned_16			MCSSDetected;
	unsigned_8			MInserted;
	unsigned_8			MFirstInsertion;
	unsigned_8			MFirstTimeIns;
	unsigned_16 		MWindowHandle;
	unsigned_32			MWindowBase;
	unsigned_16 		MPCMCIASocket;
	unsigned_16 		MIOAddress;
	unsigned_16 		MIOAddress2;
	unsigned_8			MBusWidth;
	unsigned_8			MIRQLine;
	unsigned_32			MMemSize;

	unsigned_16			MCCReg;
	unsigned_32			MCCRegPtr;
	unsigned_16			MSocketOffset;
	unsigned_8			MLLDFUJITSUFlag;

	unsigned_16			MWBRPStartTime;
	unsigned_8			MLLDSyncToNodeAddr[6];
	unsigned_8			MLLDSyncDepth;

	unsigned_16			MLLDMaxRoamWaitTime;
	unsigned_16			MLLDRoamResponseTmr;
	unsigned_16			MLLDRoamRetryTmr;
	unsigned_8			MLLDRoamResponse;
	unsigned_8			MLLDOldMSTAAddr [ADDRESSLENGTH];

	unsigned_8			MLLDNAKTime;
	unsigned_8			MLLDTxFragRetries;

	unsigned_8			MLLDSearchTime;
	unsigned_8			MLLDNumMastersFound;
	unsigned_8			MSendSearchContinueFlag;
	unsigned_8			MSendAbortSearchFlag;

	unsigned_8			MLLDSniffModeFlag;
	unsigned_8			MInSearchContinueMode;
	unsigned_8			MPostponeSniff;
	unsigned_8			MNextMaster;
	unsigned_8			MLLDSniffCount;
	unsigned_16			MRcvPktLength;
	unsigned_16			MLLDUnknownProximPkt;
	unsigned_32			MResetStatisticsTime;
	unsigned_32			MFramesXmitFrag;
	unsigned_32			MFramesXmit;
	unsigned_32			MFramesXmitBFSK;
	unsigned_32			MFramesXmitQFSK;
	unsigned_32			MFramesACKError;
	unsigned_32			MFramesCTSError;
	unsigned_32			MTotalTxPackets;
	unsigned_32			MFramesRx;
	unsigned_32			MFramesRxDuplicate;
	unsigned_32			MFramesRxFrag;


	struct RXBStruct			*MRXBuffersPtr;
	struct ReEntrantQueue	MReEntrantQBuff [TOTALREQUEUE];
	struct LLDLinkedList 	MReEntrantQ;
	struct LLDLinkedList		MFreeReEntrantQ;

#ifdef SERIAL

	unsigned_16    MuartBaseAddr;  /* UART's base address                    */

	unsigned_16    MTxRcvHoldReg;  /* Transmit & Receive Holding Reg.- R0    */
	unsigned_16    MIntrEnableReg; /* Interrupt Enable Register      - R1    */
	unsigned_16    MIntrIdentReg;  /* Interrupt Identification Reg.  - R2    */
	unsigned_16    MFIFOCtrlReg;   /* FIFO Control Register          - R2    */
	unsigned_16    MLineCtrlReg;   /* Line Control/Data Format Reg.  - R3    */
	unsigned_16    MModemCtrlReg;  /* Modem Control/RS-232 Output Crl- R4    */
	unsigned_16    MLineStatReg;   /* Line Status/Serialization Stat - R5    */
	unsigned_16    MModemStatReg;  /* Modem Status/RS-232 Input Stat - R6    */
	unsigned_8     MModemCtrlVal;  /* Current Modem Control Register value   */
	unsigned_8     MIRQLevel;      /* The interrupt level selected           */                            
	unsigned_8     MIRQMask;       /* The interrupt controller mask bit.     */
   unsigned_8     MIRQMaskReg;    /* The interrupt mask register address.   */
   unsigned_8		MIntrOutVal;    /* Indicates if 8259 interrupts are	    */
						 					 /* enabled or disabled.					    */

	struct rcvStruct  MuartRx;     /* Serial LLD's Receiving State        */
	struct sndStruct  MuartTx;     /* Serial LLD's Transmitting State     */

	unsigned_8		MLenLow;			 /* Used to calculate the Length Checksum  */
	unsigned_8		MLenHi;			 /* when receiving a packet.					*/
	unsigned_8		MTxFIFODepth;	 /* Size UART's Transmit FIFO			*/
	unsigned_8		MFIFOType;		 /* Is it an 8250/16450 or a 16550?	*/
	unsigned_8		MserialInStandby;	/* Used when testing standby mode with	*/
												/* RL2DIAG.										*/	

	struct TxRcvQueue	*MQLoad;			/* Returned from uartGetABuf()			*/
	unsigned_8			*MQDBptr;		/* Points to Data buffer being loaded	*/

	unsigned_8						MuBufPool[BUF_POOL_SIZE];
	struct TxRcvQueue				MuBufTable[MAX_BUF_TABLES];

	struct SLLDLinkedList		MuFreeList;
	struct SLLDLinkedList		MuTxRdyList;
	unsigned_16						MuSmallBufSize;

#endif

};



struct LLDVarsStruct
{	unsigned char				Card;
	struct LLDConfigVars		Config;
	struct LLDInternalVars	Internal; 
};



/***************************************************************************/
/*																									*/
/* These are the macro definitions which will replace each reference to		*/
/*	the LLD variable with the pointed to replacement.								*/
/*																									*/
/***************************************************************************/

#define	LLDIOAddress1				(LLDVars->Config.MLLDIOAddress1)
#define	LLDIntLine1					(LLDVars->Config.MLLDIntLine1)
#define	LLDChannel					(LLDVars->Config.MLLDChannel)
#define	LLDSubChannel				(LLDVars->Config.MLLDSubChannel)
#define	LLDHopPeriod				(LLDVars->Config.MLLDHopPeriod)
#define	LLDBFreq						(LLDVars->Config.MLLDBFreq)
#define	LLDSFreq						(LLDVars->Config.MLLDSFreq)
#define	LLDMSTAName					(LLDVars->Config.MLLDMSTAName)
#define	LLDDeferralSlot			(LLDVars->Config.MLLDDeferralSlot)
#define	LLDFairnessSlot			(LLDVars->Config.MLLDFairnessSlot)
#define	LLDBridgeFlag				(LLDVars->Config.MLLDBridgeFlag)
#define	LLDSecurityIDOverride	(LLDVars->Config.MLLDSecurityIDOverride)
#define	LLDSecurityID				(LLDVars->Config.MLLDSecurityID)
#define	LLDNodeAddress				(LLDVars->Config.MLLDNodeAddress)
#define	LLDInactivityTimeOut		(LLDVars->Config.MLLDInactivityTimeOut)
#define	LLDDomain					(LLDVars->Config.MLLDDomain)
#define	LLDNodeType					(LLDVars->Config.MLLDNodeType)
#define	LLDDisableHop				(LLDVars->Config.MLLDDisableHop)
#define	LLDRSSIThreshold			(LLDVars->Config.MLLDRSSIThreshold)
#define	LLDSyncRSSIThreshold		(LLDVars->Config.MLLDSyncRSSIThreshold)
#define	LLDBDWakeup					(LLDVars->Config.MLLDBDWakeup)
#define	LLDSniffTime				(LLDVars->Config.MLLDSniffTime)
#define	LLDIORange1					(LLDVars->Config.MLLDIORange1)
#define	LLDTransferMode			(LLDVars->Config.MLLDTransferMode)
#define	LLDSocketNumber			(LLDVars->Config.MLLDSocketNumber)
#define	LLDSlot						(LLDVars->Config.MLLDSlot)
#define	LLDMemoryAddress1			(LLDVars->Config.MLLDMemoryAddress1)
#define	LLDMemorySize1				(LLDVars->Config.MLLDMemorySize1)
#define	LLDDMALine1					(LLDVars->Config.MLLDDMALine1)
#define	LLDMaxPowerLevel			(LLDVars->Config.MLLDMaxPowerLevel)
#define	LLDReserved					(LLDVars->Config.MLLDReserved)
#define	LLDMaxDataSize				(LLDVars->Config.MLLDMaxDataSize)
#define	LLDLookAheadSize			(LLDVars->Config.MLLDLookAheadSize)
#define	LLDFragSize					(LLDVars->Config.MLLDFragSize)
#define	LLDRoamingFlag				(LLDVars->Config.MLLDRoamingFlag)
#define	LLDROMVersion				(LLDVars->Config.MLLDROMVersion)
#define	LLDPhysAddress				(LLDVars->Config.MLLDPhysAddress)
#define	LLDSyncState				(LLDVars->Config.MLLDSyncState)
#define	LLDMSTAAddr					(LLDVars->Config.MLLDMSTAAddr)
#define	LLDSyncName					(LLDVars->Config.MLLDSyncName)
#define	LLDMSTASyncName			(LLDVars->Config.MLLDMSTASyncName)
#define	LLDMSTASyncChannel		(LLDVars->Config.MLLDMSTASyncChannel)
#define	LLDMSTASyncSubChannel	(LLDVars->Config.MLLDMSTASyncSubChannel)
#define	LLDPCMCIA					(LLDVars->Config.MLLDPCMCIA)
#define	LLDOnePieceFlag 			(LLDVars->Config.MLLDOnePieceFlag)
#define	LLDMicroISA					(LLDVars->Config.MLLDMicroISA)
#define	LLDOEM						(LLDVars->Config.MLLDOEM)
#define	LLDInit365Flag				(LLDVars->Config.MLLDInit365Flag)
#define	LLDPointToPoint			(LLDVars->Config.MLLDPointToPoint)
#define	LLDPeerToPeerFlag			(LLDVars->Config.MLLDPeerToPeerFlag)
#define	LLDRepeatingWBRP			(LLDVars->Config.MLLDRepeatingWBRP)
#define	LLDDebugFlag				(LLDVars->Config.MLLDDebugFlag)
#define	LLDOpMode					(LLDVars->Config.MLLDOpMode)
#define	LLDFastIO					(LLDVars->Config.MLLDFastIO)
#define	DriverNoResetFlag			(LLDVars->Config.MDriverNoResetFlag)
#define	LLDNoRepeatingFlag		(LLDVars->Config.MLLDNoRepeatingFlag)
#define 	LLDDebugColor 	  			(LLDVars->Config.MLLDDebugColor)
#define	LLDNoRetries				(LLDVars->Config.MLLDNoRetries)
#define  LLDForceFrag				(LLDVars->Config.MLLDForceFrag)
#define	LLDSyncAlarm				(LLDVars->Config.MLLDSyncAlarm)
#define	LLDRetryThresh				(LLDVars->Config.MLLDRetryThresh)
#define	ConfigureMACFlag			(LLDVars->Config.MConfigureMACFlag)
#define	LLDTxMode					(LLDVars->Config.MLLDTxMode)
#define	LLDPMSync					(LLDVars->Config.MLLDPMSync)
#define	LLDMissedSync				(LLDVars->Config.MLLDMissedSync)
#define	LLDOEMCustomer				(LLDVars->Config.MLLDOEMCustomer)
#define	LLDCTSRetries				(LLDVars->Config.MLLDCTSRetries)
#define	LLDTicksToSniff			(LLDVars->Config.MLLDTicksToSniff)
#define	LLDRoamConfiguration		(LLDVars->Config.MLLDRoamConfiguration)
#define	LLDReadPortOff				(LLDVars->Config.MLLDReadPortOff)
#define	StationName					(LLDVars->Config.MStationName)
#define	NotesBuffer					(LLDVars->Config.MNotesBuffer)
#define	DriverVersionString		(LLDVars->Config.MDriverVersionString)


#ifdef SERIAL
#define	SLLDFIFOTrigger			(LLDVars->Config.MSLLDFIFOTrigger)
#define	SLLDBaudrateCode			(LLDVars->Config.MSLLDBaudrateCode)
#endif
	

#define	LLDLookAheadBuf			(LLDVars->Internal.MLLDLookAheadBuf)
#define	LLDLookAheadAddOn			(LLDVars->Internal.MLLDLookAheadAddOn)
#define	InDriverISRFlag			(LLDVars->Internal.MInDriverISRFlag)
#define	InPollFlag					(LLDVars->Internal.MInPollFlag)
#define	InResetFlag					(LLDVars->Internal.MInResetFlag)
#define	LLDRoamingState			(LLDVars->Internal.MLLDRoamingState)
#define	RoamStartTime				(LLDVars->Internal.MRoamStartTime)
#define	LLDMSTAFlag					(LLDVars->Internal.MLLDMSTAFlag)
#define	LLDSleepFlag				(LLDVars->Internal.MLLDSleepFlag)

#define	LLDLivedTime				(LLDVars->Internal.MLLDLivedTime)
#define	LLDKeepAliveTmr			(LLDVars->Internal.MLLDKeepAliveTmr)
#define	InLLDSend					(LLDVars->Internal.MInLLDSend)
#define	DisableLLDSend				(LLDVars->Internal.MDisableLLDSend)
#define	LLDOutOfSyncCount			(LLDVars->Internal.MLLDOutOfSyncCount)
#define	LLDTxModeSave				(LLDVars->Internal.MLLDTxModeSave)
#define	TxStartTime					(LLDVars->Internal.MTxStartTime)
#define	TxErrCount					(LLDVars->Internal.MTxErrCount)
#define	TxFlag						(LLDVars->Internal.MTxFlag)
#define	TxFlagCleared				(LLDVars->Internal.MTxFlagCleared)
#define	TxInProgress				(LLDVars->Internal.MTxInProgress)
#define	LLDTxBuffer					(LLDVars->Internal.MLLDTxBuffer)
#define	PacketizePktBuf			(LLDVars->Internal.MPacketizePktBuf)
#define	SecPacketizePktBuf		(LLDVars->Internal.MSecPacketizePktBuf)
#define	PacketSeqNum				(LLDVars->Internal.MPacketSeqNum)

#define	LLDSent						(LLDVars->Internal.MLLDSent)
#define	LLDReSent				  	(LLDVars->Internal.MLLDReSent)
#define	LLDSentCompleted			(LLDVars->Internal.MLLDSentCompleted)
#define	LLDRx                   (LLDVars->Internal.MLLDRx)
#define	LLDRxLookAhead          (LLDVars->Internal.MLLDRxLookAhead)
#define	LLDErrResetCnt          (LLDVars->Internal.MLLDErrResetCnt)
#define	LLDNeedResetCnt         (LLDVars->Internal.MLLDNeedResetCnt)
#define	LLDRxTimeOuts           (LLDVars->Internal.MLLDRxTimeOuts)
#define	LLDTxTimeOuts           (LLDVars->Internal.MLLDTxTimeOuts)
#define	LLDOutSyncQDrp          (LLDVars->Internal.MLLDOutSyncQDrp)
#define	LLDOutSyncDrp           (LLDVars->Internal.MLLDOutSyncDrp)
#define	LLDTimedQDrp            (LLDVars->Internal.MLLDTimedQDrp)
#define	LLDFailureDrp           (LLDVars->Internal.MLLDFailureDrp)
#define	LLDNoReEntrantDrp       (LLDVars->Internal.MLLDNoReEntrantDrp)
#define	LLDSendDisabledDrp      (LLDVars->Internal.MLLDSendDisabledDrp)
#define	LLDZeroRxLen				(LLDVars->Internal.MLLDZeroRxLen)
#define	LLDTooBigRxLen	  			(LLDVars->Internal.MLLDTooBigRxLen)
#define	LLDTooBigTxLen	  			(LLDVars->Internal.MLLDTooBigTxLen)
#define	LLDBadCopyLen	  			(LLDVars->Internal.MLLDBadCopyLen)

#ifdef SERIAL
#define	InSerialISRFlag			(LLDVars->Internal.MInSerialISRFlag)
#define	RestoreBIOSTableFlag		(LLDVars->Internal.MRestoreBIOSTableFlag)
#define	SaveBIOSAddr				(LLDVars->Internal.MSaveBIOSAddr)
#define	SLLDRxHdrErr            (LLDVars->Internal.MSLLDRxHdrErr)
#define	SLLDRxChksumErr         (LLDVars->Internal.MSLLDRxChksumErr)
#define	SLLDRxLenErr            (LLDVars->Internal.MSLLDRxLenErr)
#define	SLLDOverrunErr          (LLDVars->Internal.MSLLDOverrunErr)
#define	SLLDRxErrors	         (LLDVars->Internal.MSLLDRxErrors)
#endif

#define	LLDNeedReset				(LLDVars->Internal.MLLDNeedReset)
#define	TXQMemory					(LLDVars->Internal.MTXQMemory)
#define	TXQueue						(LLDVars->Internal.MTXQueue)
#define	TCBQMemory					(LLDVars->Internal.MTCBQMemory)
#define	FreeTCBQEntry				(LLDVars->Internal.MFreeTCBQEntry)
#define	NodeTable					(LLDVars->Internal.MNodeTable)
#define	LLDMapTable					(LLDVars->Internal.MLLDMapTable)
#define	HashTable 					(LLDVars->Internal.MHashTable)
#define	NextFreeNodeEntry			(LLDVars->Internal.MNextFreeNodeEntry)
#define	MRUNodeEntry				(LLDVars->Internal.MMRUNodeEntry)
#define	LRUNodeEntry				(LLDVars->Internal.MLRUNodeEntry)

#define	MulticastOnFlag			(LLDVars->Internal.MMulticastOnFlag)

#define	IntStack						(LLDVars->Internal.MIntStack)
#define	IntStackPtr					(LLDVars->Internal.MIntStackPtr)

#define	SetMulticastFlag			(LLDVars->Internal.MSetMulticastFlag)
#define	LookAheadLength			(LLDVars->Internal.MLookAheadLength)
#define	RawLookAheadLength		(LLDVars->Internal.MRawLookAheadLength)
#define	HardwareHeader				(LLDVars->Internal.MHardwareHeader)
#define	LLDLookAheadBufPtr		(LLDVars->Internal.MLLDLookAheadBufPtr)
#define	RxFlag						(LLDVars->Internal.MRxFlag)
#define	RxStartTime					(LLDVars->Internal.MRxStartTime)
#define	RXBuffers					(LLDVars->Internal.MRXBuffers)
#define	FreeRXBuffer				(LLDVars->Internal.MFreeRXBuffer)
#define	TXBuffers					(LLDVars->Internal.MTXBuffers)
#define	FreeTXBuffer				(LLDVars->Internal.MFreeTXBuffer)

#define	StatusReg					(LLDVars->Internal.MStatusReg)
#define	StatusValue					(LLDVars->Internal.MStatusValue)
#define	ControlReg					(LLDVars->Internal.MControlReg)
#define	EOIReg						(LLDVars->Internal.MEOIReg)
#define	ControlValue				(LLDVars->Internal.MControlValue)
#define	DataReg						(LLDVars->Internal.MDataReg)
#define	IntSelReg					(LLDVars->Internal.MIntSelReg)
#define	IntSelValue					(LLDVars->Internal.MIntSelValue)
#define	NodeOverrideFlag			(LLDVars->Internal.MNodeOverrideFlag)
#define	LLDSecurityIDString		(LLDVars->Internal.MLLDSecurityIDString)
#define	SendInitCmdFlag			(LLDVars->Internal.MSendInitCmdFlag)
#define	SetSecurityIDFlag			(LLDVars->Internal.MSetSecurityIDFlag)
#define	GetGlobalAddrFlag			(LLDVars->Internal.MGetGlobalAddrFlag)
#define	GetROMVersionFlag			(LLDVars->Internal.MGetROMVersionFlag)
#define	SetITOFlag					(LLDVars->Internal.MSetITOFlag)
#define	DisableHopFlag				(LLDVars->Internal.MDisableHopFlag)
#define	SetRoamingFlag				(LLDVars->Internal.MSetRoamingFlag)
#define	DisableEEPROMFlag			(LLDVars->Internal.MDisableEEPROMFlag)
#define	EnableEEPROMFlag			(LLDVars->Internal.MEnableEEPROMFlag)
#define	GetRFNCStatsFlag			(LLDVars->Internal.MGetRFNCStatsFlag)

#define	CSSClientHandle			(LLDVars->Internal.MCSSClientHandle)
#define	CORAddress					(LLDVars->Internal.MCORAddress)
#define	PCMCIACardInserted		(LLDVars->Internal.MPCMCIACardInserted)
#define	CSSDetected					(LLDVars->Internal.MCSSDetected)
#define	Inserted						(LLDVars->Internal.MInserted)
#define	FirstInsertion				(LLDVars->Internal.MFirstInsertion)
#define	FirstTimeIns				(LLDVars->Internal.MFirstTimeIns)
#define	WindowHandle				(LLDVars->Internal.MWindowHandle)
#define	WindowBase					(LLDVars->Internal.MWindowBase)
#define	PCMCIASocket				(LLDVars->Internal.MPCMCIASocket)
#define	IOAddress					(LLDVars->Internal.MIOAddress)
#define	IOAddress2					(LLDVars->Internal.MIOAddress2)
#define	BusWidth						(LLDVars->Internal.MBusWidth)
#define	IRQLine						(LLDVars->Internal.MIRQLine)
#define	MemSize						(LLDVars->Internal.MMemSize)

#define	CCReg							(LLDVars->Internal.MCCReg)
#define	CCRegPtr						(LLDVars->Internal.MCCRegPtr)
#define	SocketOffset				(LLDVars->Internal.MSocketOffset)
#define	LLDFUJITSUFlag				(LLDVars->Internal.MLLDFUJITSUFlag)

#define	WBRPStartTime				(LLDVars->Internal.MWBRPStartTime)
#define	LLDSyncToNodeAddr			(LLDVars->Internal.MLLDSyncToNodeAddr)
#define	LLDSyncDepth				(LLDVars->Internal.MLLDSyncDepth)

#define	LLDMaxRoamWaitTime		(LLDVars->Internal.MLLDMaxRoamWaitTime)
#define	LLDRoamResponse			(LLDVars->Internal.MLLDRoamResponse)
#define	LLDRoamResponseTmr		(LLDVars->Internal.MLLDRoamResponseTmr)
#define	LLDRoamRetryTmr			(LLDVars->Internal.MLLDRoamRetryTmr)
#define	LLDOldMSTAAddr				(LLDVars->Internal.MLLDOldMSTAAddr)

#define	LLDNAKTime					(LLDVars->Internal.MLLDNAKTime)
#define	LLDTxFragRetries			(LLDVars->Internal.MLLDTxFragRetries)

#define	LLDSearchTime				(LLDVars->Internal.MLLDSearchTime)
#define	LLDNumMastersFound		(LLDVars->Internal.MLLDNumMastersFound)
#define	SendSearchContinueFlag	(LLDVars->Internal.MSendSearchContinueFlag)
#define	SendAbortSearchFlag		(LLDVars->Internal.MSendAbortSearchFlag)

#define	ResetStatisticsTime		(LLDVars->Internal.MResetStatisticsTime)
#define	LLDSniffModeFlag 			(LLDVars->Internal.MLLDSniffModeFlag)
#define	InSearchContinueMode		(LLDVars->Internal.MInSearchContinueMode)
#define	PostponeSniff				(LLDVars->Internal.MPostponeSniff)
#define	NextMaster					(LLDVars->Internal.MNextMaster)
#define	LLDSniffCount				(LLDVars->Internal.MLLDSniffCount)
#define	FramesXmitFrag				(LLDVars->Internal.MFramesXmitFrag)
#define	FramesXmit					(LLDVars->Internal.MFramesXmit)
#define	FramesXmitBFSK				(LLDVars->Internal.MFramesXmitBFSK)
#define	FramesXmitQFSK				(LLDVars->Internal.MFramesXmitQFSK)
#define	FramesACKError				(LLDVars->Internal.MFramesACKError)
#define	FramesCTSError				(LLDVars->Internal.MFramesCTSError)
#define	TotalTxPackets				(LLDVars->Internal.MTotalTxPackets)
#define	RcvPktLength				(LLDVars->Internal.MRcvPktLength)
#define	FramesRx						(LLDVars->Internal.MFramesRx)				
#define	FramesRxDuplicate			(LLDVars->Internal.MFramesRxDuplicate)
#define	FramesRxFrag				(LLDVars->Internal.MFramesRxFrag)		
#define	LLDUnknownProximPkt		(LLDVars->Internal.MLLDUnknownProximPkt)


#define	RXBuffersPtr				(LLDVars->Internal.MRXBuffersPtr)
#define	ReEntrantQBuff				(LLDVars->Internal.MReEntrantQBuff)
#define	ReEntrantQ					(LLDVars->Internal.MReEntrantQ)
#define	FreeReEntrantQ				(LLDVars->Internal.MFreeReEntrantQ)

#ifdef SERIAL
#define	uartBaseAddr				(LLDVars->Internal.MuartBaseAddr)
#define	TxRcvHoldReg				(LLDVars->Internal.MTxRcvHoldReg)
#define	IntrEnableReg				(LLDVars->Internal.MIntrEnableReg)
#define	IntrIdentReg				(LLDVars->Internal.MIntrIdentReg)
#define	FIFOCtrlReg					(LLDVars->Internal.MFIFOCtrlReg)
#define	LineCtrlReg					(LLDVars->Internal.MLineCtrlReg)
#define	ModemCtrlReg				(LLDVars->Internal.MModemCtrlReg)
#define	LineStatReg					(LLDVars->Internal.MLineStatReg)
#define	ModemStatReg				(LLDVars->Internal.MModemStatReg)
#define  ModemCtrlVal				(LLDVars->Internal.MModemCtrlVal)
#define  IRQLevel						(LLDVars->Internal.MIRQLevel)
#define  IRQMask						(LLDVars->Internal.MIRQMask)
#define  IRQMaskReg					(LLDVars->Internal.MIRQMaskReg)
#define	IntrOutVal					(LLDVars->Internal.MIntrOutVal)

#define  uartRx						(LLDVars->Internal.MuartRx)
#define  uartTx						(LLDVars->Internal.MuartTx)

#define  LenLow						(LLDVars->Internal.MLenLow)
#define  LenHi							(LLDVars->Internal.MLenHi)
	
#define  TxFIFODepth					(LLDVars->Internal.MTxFIFODepth)
#define  FIFOType						(LLDVars->Internal.MFIFOType)

#define  serialInStandby			(LLDVars->Internal.MserialInStandby)

#define  QLoad							(LLDVars->Internal.MQLoad)
#define  QDBptr						(LLDVars->Internal.MQDBptr)

#define	uBufPool						(LLDVars->Internal.MuBufPool)
#define	uBufTable			 		(LLDVars->Internal.MuBufTable)
#define	uFreeList					(LLDVars->Internal.MuFreeList)
#define	uTxRdyList					(LLDVars->Internal.MuTxRdyList)
#define	uSmallBufSize				(LLDVars->Internal.MuSmallBufSize)

#endif

#endif /* MULTI_H */
