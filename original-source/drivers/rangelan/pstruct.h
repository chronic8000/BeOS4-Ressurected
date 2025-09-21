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
/*																								 	*/
/*	$Header:   J:\pvcs\archive\clld\pstruct.h_v   1.18   24 Jan 1997 17:20:02   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\pstruct.h_v  $										*/
/* 
/*    Rev 1.18   24 Jan 1997 17:20:02   BARBARA
/* PReqHdr changed to PRspHdr in PRspAntenna.
/* 
/*    Rev 1.17   15 Jan 1997 17:44:38   BARBARA
/* RoamNotifyRx struct and PSRPMissedSyncEnabel and PSRPMissedSync fields added.
/* 
/*    Rev 1.16   15 Aug 1996 11:06:36   JEAN
/* Changed a comment.
/* 
/*    Rev 1.15   14 Jun 1996 16:26:26   JEAN
/* Fixed the PacketizeSearchCont structure definition.
/* 
/*    Rev 1.14   16 Apr 1996 11:39:08   JEAN
/* 
/*    Rev 1.13   12 Mar 1996 10:43:34   JEAN
/* Added defines for some new packetized commands.
/* 
/*    Rev 1.12   08 Mar 1996 19:21:56   JEAN
/* Added a structure and definitions for the serial soft baudrate
/* packetized command.  Changed some constants to defines.  Move
/* the ROMVERSIONLEN to constant.h.
/* 
/*    Rev 1.11   04 Mar 1996 15:06:56   truong
/* Changed default SyncAlarmCriteria.
/* 
/*    Rev 1.10   31 Jan 1996 13:33:36   JEAN
/* Added duplicate header include detection and moved NAMELENGTH
/* to constant.h
/* 
/*    Rev 1.9   05 Dec 1995 12:06:00   JEAN
/* Removed a #include.
/* 
/*    Rev 1.8   17 Nov 1995 16:41:18   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.7   19 Oct 1995 15:26:16   JEAN
/* Moved ADDRESSLENGTH to constant.h so it can be used by many
/* modules.
/* 
/*    Rev 1.6   12 Oct 1995 11:54:52   JEAN
/* Added #defines for the go to sniff mode function and the
/* set PX160 register function.
/* 
/*    Rev 1.5   10 Sep 1995 10:02:12   truong
/* Added PSRPSyncRSSI to mac configure structure to allow setting
/* of sync rssi level.
/* Changed syncalarm and retrythreshold defaults.
/* 
/*    Rev 1.4   20 Jun 1995 15:51:26   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.3   22 Feb 1995 10:38:28   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:26   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:22   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:36   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifndef PSTRUCT_H
#define PSTRUCT_H

/*********************************************************************/
/*																							*/
/* The following are the Header structures used in every packetized	*/
/* command and response packets.													*/
/*																							*/
/*********************************************************************/

struct PReqHdr
{	
	unsigned_8		PHCmd; 					/* Command byte		*/
	unsigned_8		PHSeqNo;					/* Sequence number	*/
	unsigned_8		PHFnNo;					/* Function number	*/
	unsigned_8		PHRsv;					/* Reserved				*/
};


struct PRspHdr
{	unsigned_8		PRCmd;					/* Command byte		*/
	unsigned_8		PRSeqNo;					/* Sequence number	*/
	unsigned_8		PRFnNo;					/* Function number	*/
};


/*-------------------------------------------------*/
/* Sequence number equates in the headers.			*/
/*																	*/
/*-------------------------------------------------*/

#define	SEQNOMASK			0x7F			/* Sequence number mask upper bit	*/
#define	RAWMASK 				0x80			/* Raw sequence bit						*/
#define	REPEAT_SEQ_NUMBER	0x7F      	/* Special sequence number	reserved	*/
#define	ROAM_NOTIFY_SEQNO	0x7D			/*	Special sequence number				*/
#define	MAX_DATA_SEQNO		0x7C			/* Last sequence number to use for	*/
													/* for repeated packets.				*/



/**************************************************************************/
/*																								  */
/* These are the command and response packet structures for the			  */
/* Configuration commands.																  */
/*																								  */
/**************************************************************************/

/*---------------------------------------*/
/* Configuration Group Equates			  */
/*													  */
/*---------------------------------------*/

#define	INIT_COMMANDS_CMD		0x41	 		/* 'A' All init commands			*/
#define	INIT_COMMANDS_RSP		0x61	 		/* 'a' Responses from init cmnds	*/


/*---------------------------------------*/
/* Configuration Group Function Numbers. */
/*													  */
/*---------------------------------------*/

#define	INITIALIZE_FN			0		 		/* Initialize cmnd function num	*/
#define	SEARCH_N_SYNC_FN		1		 		/* Search and Synchronize			*/
#define	SEARCH_N_CONT_FN		2		 		/* Search and Continue				*/
#define	ABORT_SEARCH_FN		3		 		/* Abort Search						*/
#define	IN_SYNC_FN				4		 		/* In Sync Msg (unsolicited msg)	*/
#define	OUT_OF_SYNC_FN			5		 		/* Out of Sync (unsolicited msg)	*/
#define	GOTO_STANDBY_FN		6		 		/* Goto Standby						*/
#define	SET_ITO_FN				7		 		/* Set Inactivity Timeout			*/
#define	KEEP_ALIVE_FN			8		 		/* Keep Alive Message				*/
#define	SET_MULTICAST_FN		9		 		/* Set Multicast						*/
#define	RESET_STATS_FN			0x0A	 		/* Reset RFNC Statistics			*/
#define	GET_STATS_FN			0x0B	 		/* Get RFNC Statistics				*/
#define	SET_SECURITY_ID_FN	0x0C	 		/* Set Security ID					*/
#define	GET_ROM_VER_FN			0x0D	 		/* Get ROM Version					*/
#define	GET_ADDRESS_FN			0x0E	 		/* Get Global Address				*/
#define	GET_GPIN0_FN			0x0F	 		/* Get Input pin status				*/
#define	CONFIG_MAC_FN			0x10	 		/* Configure MAC						*/
#define	SET_ROAMING_FN			0x11	 		/* Set Roaming Parameters			*/
#define	ROAMING_ALARM_FN		0x12	 		/* Roam Alarm (unsolicited msg)	*/
#define	ROAM_FN					0x13	 		/* Roam to better MSTA				*/
#define	ANTENNA_FN				0x14	 		/* Antenna Dis/Connect (unsol)	*/
#define	GET_COUNTRY_FN			0x15	 		/* Get Country Code					*/
#define  GOTO_SNIFF_MODE		0x16			/* Puts card in sniff mode			*/
#define	DISABLE_EEPROM_FN		0x17			/* Disable the card from writing	*/
														/* to EEPROM.							*/
#define	ENABLE_EEPROM_FN		0x18			/* Enables the card to write to	*/
														/* EEPROM.								*/

#define	GET_FREQUENCY_FN		0x20			/* Get Frequency Table 				*/
#define	SET_FREQUENCY_FN		0x21			/* Set Frequency Table 				*/
#define	SET_HOPSTAT_FN			0x22			/* Set Hop Statistics 				*/
#define	HOP_STATISTICS_FN		0x23			/* Hop Statistics						*/
#define	SET_UART_BAUDRATE_FN	0x24			/* Set UART Baud Rate				*/

#define	TX_CTS_ST				0x01			/* Bit set when CTS tx error		*/
#define	TX_ACK_ST				0x02			/* Bit set when ACK tx error		*/
#define	TX_RETRY_ST				0x40			/* Bit set if retry thresh met	*/


/*---------------------------------------*/
/*	Initialize Command and response		  */
/*													  */
/*---------------------------------------*/

struct PacketizeInitCmd																		
{	struct PReqHdr	PICHeader;				  	/* Command header						*/
	unsigned_8 		PICLocalAddress [ADDRESSLENGTH];	/* Local address			*/
	unsigned_8 		PICOpMode;					/* Operating mode						*/
	unsigned_8 		PICNodeType;	  			/* Node type							*/
	unsigned_8 		PICHopPeriod;	  			/* Hop period							*/
	unsigned_8 		PICBFreq;		  			/* Beacon frequency					*/
	unsigned_8 		PICSFreq;		  			/* Search frequency					*/
	unsigned_8 		PICChSub;		  			/* Radio channel/subchannel		*/
	unsigned_8 		PICMSTAName [NAMELENGTH]; 	/* Master station name			*/
	unsigned_8 		PICDomain;					/* Domain								*/
	unsigned_8		PICControl;					/* Control field						*/
	unsigned_8		PICReserved;				/* Reserved								*/
	unsigned_8 		PICListSize;				/* Number of elements in list		*/
	unsigned_8 		PICSyncChSub;				/* Channel/Subchannel to sync to	*/
	unsigned_8 		PICSyncName [NAMELENGTH]; 	/* Name of master to sync to	*/
};


struct PRspInitCmd
{	struct PRspHdr	PRICHeader;					/* Command header						*/
	unsigned_8 		PRICStatus;					/* Status of the command			*/
	unsigned_8 		PRICErrOffset;				/* Parameter offset of error		*/
};


/*---------------------------------------*/
/* Initialize Command equates				  */
/*													  */
/*---------------------------------------*/

#define	RESERVED					0

#define	NORMAL_OP_MODE			0		 		/* Normal operating mode		*/
#define	PROMIS_OP_MODE			1		 		/* Promiscuous mode				*/
#define	PPROMIS_OP_MODE		2		 		/* Protocol promiscuous mode	*/

#define	SYNC_LIST_SIZE			1		 		/* Synchronize list size		*/



/*---------------------------------------*/
/*	Search and Synchronize					  */
/*													  */
/*---------------------------------------*/

struct PacketizeSearchSync
{	struct PReqHdr	PSSHeader;				  	/* Command header						*/
	unsigned_8		PSSReserved [23];		  	/* Reserve padding to match init	*/
	unsigned_8		PSSDomain;		 		  	/* Domain								*/
	unsigned_8		PSSControl;					/* Control byte						*/
	unsigned_8		PSSReserved1;				/* Reserved								*/
	unsigned_8		PSSListSize;	  		  	/* # of elements in sync list		*/
	unsigned_8		PSSSyncChSub;	  		  	/* Channel/Subchannel to sync to	*/
	unsigned_8		PSSSyncName [NAMELENGTH]; 	/* Name of master to sync to	*/
};



/*---------------------------------------*/
/*	Search and Continue						  */
/*													  */
/*---------------------------------------*/

struct PacketizeSearchCont
{	struct PReqHdr	PSCHeader;					/* Command header						*/
	unsigned_8		PSCReserved [23];		  	/* Reserve padding to match init	*/
	unsigned_8		PSCDomain;		 		  	/* Domain								*/
	unsigned_8		PSCReserved1;				/* Reserved								*/
	unsigned_8		PSCReserved2;				/* Reserved								*/
	unsigned_8		PSCListSize;	  		  	/* # of elements in sync list		*/
	unsigned_8		PSCSyncChSub;	  		  	/* Channel/Subchannel to sync to	*/
	unsigned_8		PSCSyncName [NAMELENGTH]; 	/* Name of master to sync to	*/
};



/*---------------------------------------*/
/*	Abort Search								  */
/*													  */
/*---------------------------------------*/

struct PacketizeAbortSearch
{	struct PReqHdr	PASHeader;					/* Command header	*/
};



/*---------------------------------------*/
/*	In Sync Message (Unsolicited Message) */
/*													  */
/*---------------------------------------*/

struct PacketizeInSync
{	struct PRspHdr	PISHeader;							/* Command header						*/
	unsigned_8		PISReserve;							/* Reserve field						*/
	unsigned_8		PISChSub;							/* Channel/Subchannel synced to	*/
	unsigned_8		PISSyncName [NAMELENGTH];		/* Name of master synced to		*/
	unsigned_8		PISSyncAddr	[ADDRESSLENGTH];	/* Address of master synced to	*/
#ifdef WBRP
	unsigned_8		PISSyncToAddr[ADDRESSLENGTH];	/* Node ID RFNC is synced to		*/
	unsigned_8		PISSyncDepth;						/* Sync. Depth of node				*/
#endif
};


/*---------------------------------------*/
/* Out of Sync (Unsolicited Message		  */
/*													  */
/*---------------------------------------*/

struct PacketizeOutSync
{	struct PRspHdr	POSHeader;					/* Command header	*/
};


/*---------------------------------------*/
/*	Go to Standby								  */
/*													  */
/*---------------------------------------*/

struct PacketizeGoStandby
{	struct PReqHdr	PGSBHeader;					/* Command header	*/
};


/*---------------------------------------*/
/*	Set Inactivity Time Out					  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetITO
{	struct PReqHdr	PSITOHeader;				/* Command header						*/
	unsigned_8		PSITOLength;				/* Length of the time-out field	*/
	unsigned_8		PSITOSniffTime;			/* Inactivity T.O. (5 sec resol)	*/
	unsigned_8		PSITOControl;				/* Inactivity T.O. control byte	*/
	unsigned_8		PSITOStbyTime;				/* Standby T.O. (5 sec resol)		*/
};



/*---------------------------------------*/
/*	Keep Alive									  */
/*													  */
/*---------------------------------------*/

struct PacketizeKeepAlive
{	struct PReqHdr	PKAHeader;					/* Command header	*/
};



/*---------------------------------------*/
/*	Set Multicast								  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetMult
{	struct PReqHdr	PSMHeader;					/* Command header					*/
	unsigned_8		PSMSwitch;					/* Turning multicast On/Off	*/
};


/*---------------------------------------*/
/* Reset RFNC Stats							  */
/*													  */
/*---------------------------------------*/

struct PacketizeResetStats
{	struct PReqHdr	PRSHeader;					/* Command header	*/
};


/*---------------------------------------*/
/* Get RFNC Stats command and response	  */
/*													  */
/*---------------------------------------*/

struct PacketizeGetStats
{	struct PReqHdr	PGSHeader;					/* Command header	*/
};


struct PRspGetStats
{	struct PRspHdr	PRGSHeader;					/* Command header						*/
	unsigned_16  	PRGSNumHops;				/* Total number of hops				*/
	unsigned_16  	PRGSNumSearches;			/* Searches for other Masters		*/
	unsigned_16		PRGSMaster;					/* Times been a Master				*/
	unsigned_16		PRGSResync;					/* Times synced to new Master		*/
	unsigned_16		PRGSMissedSyncs;			/* Number of sync msgs missed		*/
	unsigned_8		PRGSReserved1 [20]; 		/* Reserved bytes						*/
	unsigned_16		PRGSRxPackets;				/* Number of Data pkts received	*/
	unsigned_16		PRGSTxPackets;				/* Number of Data pkts xmitted	*/
	unsigned_16		PRGSLostCTS;				/* Times sent RTS without CTS		*/
	unsigned_16		PRGSCTSErrors;				/* Number of CTS errors				*/
	unsigned_16		PRGSACKErrors;				/* Number of ACK errors				*/
	unsigned_16		PRGSSIDMismatch;			/* # of security ID mismatches	*/
	unsigned_16		PRGSMissedSOBs;			/* # of missed start of B msgs	*/
	unsigned_16		PRGSCorrupt;				/* # of corrupt data packets		*/
	unsigned_8		Reserved2 [18];			/* Reserved bytes						*/
	unsigned_8		Reserved3 [20];			/* Reserved bytes						*/
};
	

/*---------------------------------------*/
/*	Set Security ID							  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetSecurityID
{	struct PReqHdr	PSSIDHeader;				/* Command header				*/
	unsigned_8		PSSIDByte [3];				/* First security ID byte	*/
};



/*---------------------------------------*/
/*	Get ROM Version command and response  */
/*													  */
/*---------------------------------------*/

struct PacketizeGetROMVer
{	struct PReqHdr	PGRVHeader;					/* Command header	*/
};


struct PRspGetROMVer
{	struct PRspHdr	PRGRVHeader;				/* Command header				*/
	unsigned_8		PRGRVStatus;				/* Status byte					*/
	unsigned_8		PRGRVLength;				/* Length of version field	*/
	unsigned_8		PRGRVVersion [ROMVERLENGTH];	/* ROM version string*/
};



/*---------------------------------------*/
/*	Get Global Address						  */
/*													  */
/*---------------------------------------*/

struct PacketizeGetGlobal
{	struct PReqHdr	PGGHeader;					/* Command header	*/
};


struct PRspGetAddr
{	struct PRspHdr	PRGAHeader;					/* Command header						*/
	unsigned_8		PRGAStatus;					/* Status of the command			*/
	unsigned_8		PRGALength;					/* Offset detected with an error	*/
	unsigned_8		PRGAAddress	[ADDRESSLENGTH]; 	/* Global address			 	*/
};



/*---------------------------------------*/
/* Get GPIn0 Value command and response  */
/*													  */
/*---------------------------------------*/

struct PacketizeGPIn0
{	struct PReqHdr	PGPInHeader;				/* Command header	*/
};


struct PRspGPIn0
{	struct PRspHdr	PRGPInHeader;				/* Command header			*/
	unsigned_8		PRGPInValue;				/* Value of the GPIn0	*/
};



/*---------------------------------------*/
/*	Configure MAC command and response	  */
/*													  */
/*---------------------------------------*/

struct PacketizeConfigMAC
{	struct PReqHdr	PCMHeader;					/* Command header						*/
	unsigned_8		PCMContention;				/* Contention period					*/
	unsigned_8		PCMRegularRetry;			/* Regular # of ack retries		*/
	unsigned_8		PCMFragRetry;				/* Frag # of ack retries			*/
	unsigned_8		PCMRegularQFSK;			/* Regular # of QFSK in auto		*/
	unsigned_8		PCMFragQFSK;				/* Frag # of QFSK in auto mode	*/
	unsigned_8		PCMsobto	[3];				/* Start of B time out				*/
	unsigned_8		PCMOutBound;				/* # of outbound msgs allowed		*/
};


struct PRspConfigMAC
{	struct PRspHdr	PRCMHeader;					/* Command header	*/
	unsigned_8		PRCMStatus;					/* Return status	*/
};


#define	SYNCALARMCRITERIA	0x27				/* 2 of 7 sync msgs missed		*/
														/* for alarm						*/
#define	RETRYTHRESHOLD		6					/* 6 RF TX attempts to set		*/
														/* retry bit						*/



/*---------------------------------------*/
/* Set Roaming Parameters					  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetRoaming
{	struct PReqHdr	PSRPHeader;					/* Command header		  		*/
	unsigned_8		PSRPAlarm;					/* Sync Alarm Threshold		*/
	unsigned_8		PSRPRetry;					/* Retry Threshold			*/
	unsigned_8		PSRPRSSI;					/* RSSI Threshold				*/
	unsigned_8		PSRPSyncRSSIEnable;		/* RSSI Threshold	Enable	*/
	unsigned_8		PSRPSyncRSSI;				/* Sync RSSI Threshold	 	*/
	unsigned_8		PSRPMissedSyncEnable;	/* RSSI Threshold	Enable	*/
	unsigned_8		PSRPMissedSync;			/* Sync RSSI Threshold	 	*/
};



/*---------------------------------------*/
/* Roaming Alarm (Unsolicited Message)	  */
/*													  */
/*---------------------------------------*/

struct PacketizeRoamAlarm
{	struct PRspHdr	PRAHeader;					/* Command header	*/
};



/*---------------------------------------*/
/* Roam Command								  */
/*													  */
/*---------------------------------------*/

struct PacketizeRoam
{	struct PReqHdr	PRHeader;					/* Command header	*/
};



/*---------------------------------------*/
/* Antenna Connect/Disconnect Message	  */
/*													  */
/*---------------------------------------*/

struct PRspAntenna
{	struct PRspHdr	PAHeader;					/* Command header				*/
	unsigned_8		PAStatus;					/* Status of the antenna	*/
};


/*---------------------------------------*/
/* Get Country Code							  */
/*													  */
/*---------------------------------------*/

struct PacketizeCountryCode
{	
	struct PReqHdr	PCCHeader;					/* Command header	*/
};



/*********************************************************************/
/*																					 	   */
/* These are the configuration command used by the Armadillo project */
/*																						   */
/*********************************************************************/

/*---------------------------------------*/
/* Get Frequency Table						  */
/*													  */
/*---------------------------------------*/

struct PacketizeGetFreqTbl
{
	struct PReqHdr	PGFTHeader;			/* Command header	*/
};
	

struct PRspGetFreqTable
{	
	struct PRspHdr	PGFTHeader;	  
	unsigned_8		PGFTStatus;			/* 00:Success, 01:Error reading EEPROM */
	unsigned_8		PGFTLength;			/* Length = 53 (35h)							*/
	unsigned_8		PGFTFreqTbl[60];	/* Contents of the Frequency Table		*/
};


/*---------------------------------------*/
/* Set Frequency Table						  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetFreqTbl
{
	struct PReqHdr	PSFTHeader;			/* Command header	*/
	unsigned_8		PSFTLength;			/* Length = 53 (35h)							*/
	unsigned_8		PSFTFreqTbl[60];	/* Contents of the Frequency Table		*/
};
	

struct PRspSetFreqTable
{	
	struct PRspHdr	PSFTHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PGFTStatus;			/* 00:Success, 01:Error writing EEPROM */
};


/*---------------------------------------*/
/* Set Hop Statistics						  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetHopStat
{
	struct PReqHdr	PSHSHeader;			/* Command header								*/
	unsigned_8		PSHSHopStatOnOff;	/* b0:1-on, 0-off 							*/
};
	

struct PRspSetHopStat
{	
	struct PRspHdr	PSHSHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PSHSStatus;			/* 00:Success									*/
};



/*---------------------------------------*/
/* Hop Statistics (unsolicited message)  */
/*													  */
/*---------------------------------------*/

struct PacketizeHopStat
{	
	struct PRspHdr	PHSHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PHSLogicalFreq; 	/* Logical Frequency #						*/
	unsigned_8		PHSPhysicalFreq;	/* Physical Frequency #						*/
	unsigned_8		PHSBoolean1;		/* bytes defines in Packetized doc.		*/

};


/*---------------------------------------*/
/* Set UART Baud Rate						  */
/*													  */
/*---------------------------------------*/

struct PacketizeSetBaudRate
{
	struct PReqHdr	PSBRHeader;			/* Command header								*/
	unsigned_8		PSBRBaudRate;		/* BaudRate = 19200 (default)				*/
};
	

struct PRspSetBaudRate
{	
	struct PRspHdr	PSBRHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PSBRStatus;			/* 00:Success									*/
};



/*---------------------------------------*/
/* Disable EEPROM Write						  */
/*													  */
/*---------------------------------------*/

struct PacketizeDisableEEPROMWrite
{
	struct PReqHdr	PDEWHeader;			/* Command header								*/
};
	

struct PRspDisableEEPROMWrite
{	
	struct PRspHdr	PDEWHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PDEWStatus;			/* 00:Success									*/
};


/*---------------------------------------*/
/* Enable EEPROM Write						  */
/*													  */
/*---------------------------------------*/

struct PacketizeEnableEEPROMWrite
{
	struct PReqHdr	PEEWHeader;			/* Command header								*/
};
	

struct PRspEnableEEPROMWrite
{	
	struct PRspHdr	PEEWHeader;			/* Command Byte, Seq. #, Function #		*/	  		
	unsigned_8		PEEWStatus;			/* 00:Success									*/
};





/******************************************************************/
/*																					 	*/
/* These are the data command group packet definitions.				*/
/*																						*/
/******************************************************************/

/*---------------------------------------*/
/* Data Group Equates						  */
/*													  */
/*---------------------------------------*/

#define	DATA_COMMANDS_CMD		0x42			/* 'B' All data transfer cmnds	*/
#define	DATA_COMMANDS_RSP		0x62			/* 'b' Responses from data cmds	*/


/*---------------------------------------*/
/* Data Group Function Numbers			  */
/*													  */
/*---------------------------------------*/

#define	TRANSMIT_DATA_FN		0				/* Transmit Data Packet	*/
#define	RECEIVE_DATA_FN		1				/* Receive Data Packet	*/
#define	PING_DATA_FN			2				/* Transmit Ping Packet	*/
#define	PING_RESPONSE_FN		3				/* Ping Response Packet	*/
#define	RECEIVE_PROT_FN		4				/* Receive Protocol Packet */


/*---------------------------------------*/
/*	Transmit Data Command and Response	  */
/*													  */
/*---------------------------------------*/

struct PacketizeTx
{	struct PReqHdr	PTHeader;					/* Command header						*/
	unsigned_8		PTControl;					/* Poll final, service type, ... */
	unsigned_8		PTTxPowerAnt;				/* Xmit power/antenna selection	*/
	unsigned_8		PTPktLength [2];			/* Packet length						*/
	unsigned_8		PTLLDHeader1;				/* LLD header							*/
	unsigned_8		PTLLDHeader2;				/* LLD header							*/
	unsigned_8		PTLLDHeader3;				/* LLD header							*/
	unsigned_8		PTLLDHeader4;				/* LLD header							*/
};


struct PacketizeTxComplete
{	struct PReqHdr	PTCHeader;					/* Command header				*/
	unsigned_8		PTCEntryNum;				/* Number of entries			*/
	unsigned_8		PTCSeqNo;					/* First sequence number	*/
	unsigned_8		PTCStatus;					/* First status				*/
};


#define	TX_STATUS_OK			0x80			/* Bit set if xmit successful		*/
#define	TX_QFSK_MODE			0x01			/* Bit set if xmit in QFSK mode	*/



/*---------------------------------------*/
/*	Receive data packet structure			  */
/*													  */
/*---------------------------------------*/

struct PRspData
{	struct PRspHdr	PRDHeader;					/* Command header						*/
	unsigned_8		PRDStatus;					/* Status of the command			*/
	unsigned_8		PRDAtoD;						/* A to D measurement of RSSI		*/
	unsigned_8		PRDPower;					/* Transmit power used by source	*/
	unsigned_8		PRDLength [2];				/* Receive packet length			*/
	unsigned_8		PRDLLDHeader1;	 			/* LLD header							*/
	unsigned_8		PRDLLDHeader2;	 			/* LLD header							*/
	unsigned_8		PRDLLDHeader3;	 			/* LLD header							*/
	unsigned_8		PRDLLDHeader4;	 			/* LLD header							*/
	unsigned_8		PRDDestAddr [6];			/* Destination address				*/
	unsigned_8		PRDSrcAddr [6];			/* Source address						*/
	unsigned_16		PRDProtID;		 			/* Protocol ID							*/
	unsigned_8		PRDData;			 			/* Start of the data					*/
};



/*---------------------------------------*/
/*	Receive protocol packet structure	  */
/*													  */
/*---------------------------------------*/

struct PRspProt
{	struct PRspHdr	PRPHeader;					/* Command header						*/
	unsigned_8		PRPStatus;					/* Always 0								*/
	unsigned_8		PRPLength [2];				/* Protocol packet length			*/
	unsigned_8		PRPTimeStamp [4];			/* Time stamp of reception			*/
	unsigned_8		PRPData;						/* Start of the protocol packet	*/
};



/**************************************************************************/
/*																								  */
/* These are the test group commands.  This is not a complete set			  */
/* because most of these commands are not used by the driver.  They		  */
/* are only used in test beds.														  */
/*																								  */
/**************************************************************************/


	/*---------------------------------------*/
	/* Test Group Equates 'C'					  */
	/*													  */
	/*---------------------------------------*/

#define	DIAG_COMMANDS_CMD		0x43	 		/* 'C' All diag commands			*/
#define	DIAG_COMMANDS_RSP		0x63	 		/* 'c' Responses from diag cmds	*/


	/*---------------------------------------*/
	/* Test Group Function Numbers			  */
	/*													  */
	/*---------------------------------------*/

#define	SET_GLOBAL_FN			0				/* Set Global Address	 */
#define	PEEK_EEPROM_FN			1			 	/* Peek EEPROM				 */
#define	POKE_EEPROM_FN			2				/* Poke EEPROM				 */
#define	PEEK_MEMORY_FN			3				/* Peek Memory				 */
#define	POKE_MEMORY_FN			4				/* Poke Memory				 */
#define	HB_TIMER_FN				5				/* Heartbeat Timer Test	 */
#define	ROM_CHECK_FN			6				/* Get ROM Checksum		 */
#define	RADIO_MODE_FN			7				/* Set Radio Mode			 */
#define	ECHO_PACKET_FN			8				/* Echo Packet				 */
#define	DISABLE_HOP_FN			9				/* Disabling Hopping		 */
#define	GET_RSSI_FN				10				/* Get RSSI Value			 */
#define	SET_COUNTRY_FN			11				/* Set Country Code		 */
#define	SET_CALLING_CODE		12				/* Set Calling Code		 */
#define	SET_PX160_REG			13				/* Set the PX160 chip	 */
#define	SET_RANDOM_BUFF_FN	14				/* Set Random Buffer     */
#define	SEND_RAND_BUFF_FN		15				/* Send Random Buffer    */
#define	ACTIVATE_CALL_FN		16				/* Activate Calling Code */
#define	SET_PTOP_ENABLE_FN	17				/* Set PtoP Enable       */
#define	SET_TXRX_FN				18				/* Set TX/RX             */
#define	SET_SOFT_BAUD_FN		19				/* Set Soft Baudrate     */
#define	SET_MAX_SYNCDEPTH_FN	32				/* Set Maximum Sync Depth*/


	/*---------------------------------------*/
	/*	Disable Hopping Command and Response  */
	/*													  */
	/*---------------------------------------*/

struct PacketizeDHop
{	
	struct PReqHdr	PDHHeader;					/* Command header			 	*/
	unsigned_8		PDHFrequency;				/* Frequency to remain on	*/
};


struct PRspDHop
{	
	struct PRspHdr	PRDHHeader;					/* Command header				*/
	unsigned_8		PRDHStatus;					/* Status of the command	*/
};


	/*---------------------------------------*/
	/*	Set Maximum Sync Depth					  */
	/*													  */
	/*---------------------------------------*/

struct PacketizeSetMaxSyncD
{	
	struct PReqHdr	PSMSDHeader;				/* Command header			 	*/
	unsigned_8		PSMSDSyncDepth;			/* Set Maximum Sync Depth	*/
};


struct PRspSyncDepth
{	
	struct PRspHdr	PSMSDHeader; 				/* Command header				*/
	unsigned_8		PSMSDStatus; 				/* Status of the command	*/
};


#ifdef SERIAL
	/*----------------------------------------*/
	/*		Set Soft Baudrate Command				*/
	/*														*/
	/*	There is no response packet for this	*/
	/* command.											*/
	/*----------------------------------------*/

struct PacketizeBaud
{	
	struct PReqHdr	PSSBHeader;					/* Command header			 	*/
	unsigned_8		PSSBBaudrate;				/* Baud rate to change to	*/
};
#endif


/**************************************************************************/
/*																								  */
/* The Roam Notify packet is really a Transmit Data packetize command.	  */
/* It is defined here to give a structure of the IPX header appended.	  */
/*																								  */
/**************************************************************************/

struct RoamNotify
{	struct PacketizeTx	RNHeader;			/* Transmit Data header				*/
	unsigned_8				RNBDest [6];		/* Bridge header dest address		*/
	unsigned_8				RNBSource [6];		/* Bridge header source address	*/
	unsigned_8				RNEDest [6];		/* Ethernet destination address	*/
	unsigned_8				RNESource [6];		/* Ethernet source address			*/
	unsigned_8				RNProtocolID [2];	/* Ethernet protocol ID bytes		*/
	unsigned_16				RNIPXIdent;			/* Identifies that its IPX packet*/
	unsigned_8				RNIPXLength [2];	/* Length of the packet				*/
	unsigned_8				RNIPXRsv [6];		/* Reserved bytes						*/
	unsigned_8				RNIPXDest [6];		/* IPX Destination address			*/
	unsigned_8				RNIPXSktID [2];	/* IPX Socket ID bytes				*/
	unsigned_8				RNIPXRsv1 [4];		/* IPX Reserved bytes				*/
	unsigned_8				RNIPXSource [6];	/* IPX Source address				*/
	unsigned_8				RNIPXSktID1 [2];	/* IPX socket ID bytes				*/
	unsigned_8				RNData [100];		/* Data portion of the packet		*/
};


struct RoamNotifyRx
{
	unsigned_8				RNBDest [6];		/* Bridge header dest address		*/
	unsigned_8				RNBSource [6];		/* Bridge header source address	*/
	unsigned_8				RNEDest [6];		/* Ethernet destination address	*/
	unsigned_8				RNESource [6];		/* Ethernet source address			*/
	unsigned_8				RNProtocolID [2];	/* Ethernet protocol ID bytes		*/
	unsigned_16				RNIPXIdent;			/* Idenifies that its IPX packet	*/
	unsigned_8				RNIPXLength [2];	/* Length of the packet				*/
	unsigned_8				RNIPXRsv [6];		/* Reserved bytes						*/
	unsigned_8				RNIPXDest [6];		/* IPX Destination address			*/
	unsigned_8				RNIPXSktID [2];	/* IPX Socket ID bytes				*/
	unsigned_8				RNIPXRsv1 [4];		/* IPX Reserved bytes				*/
	unsigned_8				RNIPXSource [6];	/* IPX Source address				*/
	unsigned_8				RNIPXSktID1 [2];	/* IPX socket ID bytes				*/
	unsigned_8				RNData [100];		/* Data portion of the packet		*/
};


/********************************************************************/
/*																						  */
/* The Bridge packet format is really a Receive Data packet, but	  */
/* the LLD adds a 12 byte bridge header which is defined below.	  */
/*																						  */
/********************************************************************/

struct BPRspData
{	struct PRspHdr	BPRDHeader;				/* Command header							*/
	unsigned_8 		BPRDStatus;				/* Status of the command				*/
	unsigned_8 		BPRDAtoD;				/* A/D of RSSI on current packet		*/
	unsigned_8 		BPRDPower;				/* Transmit power used by source		*/
	unsigned_16  	BPRDLength;				/* Receive packet length				*/
	unsigned_8 		BPRDLLDHeader1;		/* LLD header								*/
	unsigned_8 		BPRDLLDHeader2;		/* LLD header								*/
	unsigned_8 		BPRDLLDHeader3;		/* LLD header								*/
	unsigned_8 		BPRDLLDHeader4;		/* LLD header								*/
	unsigned_8 		BPRDPreDestAddr [6];	/* Bridge header dest addr				*/
	unsigned_8 		BPRDPreSrcAddr [6];	/* Bridge header src addr				*/
	unsigned_8 		BPRDDestAddr [6];		/* Destination address					*/
	unsigned_8 		BPRDSrcAddr [6];		/* Source address							*/
	unsigned_16  	BRDProtID;				/* Protocol ID								*/
	unsigned_8 		BPRDData;				/* Start of the data						*/
};

#endif /* PSTRUCT_H */

