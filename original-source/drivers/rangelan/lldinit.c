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
/*	$Header:   J:\pvcs\archive\clld\lldinit.c_v   1.56   11 Aug 1998 11:34:50   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldinit.c_v  $										*/
/* 
/*    Rev 1.56   11 Aug 1998 11:34:50   BARBARA
/* ResetStatisticsTime gets time from LLSGetCurrentTimeLONG now.
/* 
/*    Rev 1.55   03 Jun 1998 14:28:44   BARBARA
/* ResetStatisticsTime added.
/* 
/*    Rev 1.54   07 May 1998 08:58:32   BARBARA
/* memcpy changed to CopyMemory and memset changed to SetMemory.
/* 
/*    Rev 1.53   24 Mar 1998 08:01:42   BARBARA
/* MemSize, TxFlagCleared added. Change related to inactivity sec < 5 sec added.
/* 
/*    Rev 1.52   12 Sep 1997 16:21:12   BARBARA
/* LLDChooseMaster, SendSearchContinue, SendAbortSearch added to implement 
/* random master search.
/* 
/*    Rev 1.51   10 Apr 1997 18:44:32   BARBARA
/* extern declarations for variables init in LLDInitIntenalVars added.
/* 
/*    Rev 1.50   10 Apr 1997 14:19:12   BARBARA
/* LLDInitInternalVars changed to initialize new variables.
/* 
/*    Rev 1.49   15 Jan 1997 17:08:10   BARBARA
/* LLDMaxRomaWaitTime was set for slow roaming to 15 sec.
/* 
/*    Rev 1.48   11 Oct 1996 10:58:32   BARBARA
/* #ifndef NDIS_MINIPORT_DRIVER added to keep the compiler quiet.
/* 
/*    Rev 1.47   02 Oct 1996 13:39:16   JEAN
/* Added code to deregister the interrupt if the initialization
/* fails.
/* 
/*    Rev 1.46   30 Sep 1996 16:16:06   BARBARA
/* PCMCIACleanUp added.
/* 
/*    Rev 1.44   26 Sep 1996 17:02:06   JEAN
/* 
/*    Rev 1.43   13 Sep 1996 14:34:30   JEAN
/* Added some debug prints.
/* 
/*    Rev 1.42   23 Aug 1996 08:24:52   JEAN
/* Fixed a bug where we were not checking for a one or two piece
/* PCMCIA card when Card and Socket Services was detected.
/* 
/*    Rev 1.41   15 Aug 1996 09:47:40   JEAN
/* 
/*    Rev 1.40   29 Jul 1996 12:19:28   JEAN
/* 
/*    Rev 1.39   15 Jul 1996 18:28:42   TERRYL
/* Initialize LLDKeepAliveTmr to the current time instead of 0 in the
/* LLDInitInternalVars() routine.
/* 
/*    Rev 1.38   12 Jul 1996 16:18:02   TERRYL
/* Type casted the result of LLSGetCurrentTime() subtraction to 16-bit integer.
/* 
/*    Rev 1.37   28 Jun 1996 17:29:48   BARBARA
/* One Piece PCMCIA code has been added.
/* 
/*    Rev 1.36   14 Jun 1996 16:10:44   JEAN
/* Removed some debug color prints.
/* 
/*    Rev 1.35   14 Jun 1996 16:08:50   JEAN
/* Made changes to support a one-piece PCMCIA card.
/* 
/*    Rev 1.34   23 Apr 1996 09:26:04   JEAN
/* Added the LLDNoRetries flag.
/* 
/*    Rev 1.33   16 Apr 1996 11:16:24   JEAN
/* Changed the oldflags type from unsigned_16 to FLAGS.  For serial,
/* changed the timeout value in the function WaitFlagSet.
/* 
/*    Rev 1.32   29 Mar 1996 11:27:58   JEAN
/* Moved some variable declarations from pcmcia.c and css.c to
/* this file.
/* 
/*    Rev 1.31   22 Mar 1996 14:37:34   JEAN
/* Added changes for one-piece PCMCIA.
/* 
/*    Rev 1.30   21 Mar 1996 13:15:08   JEAN
/* Added a RestoreBIOSTableFlag and some statistics.
/* 
/*    Rev 1.29   13 Mar 1996 17:49:12   JEAN
/* Added a new statistic.
/* 
/*    Rev 1.28   12 Mar 1996 10:35:28   JEAN
/* Added a call to disable EEPROM writes after initialization and
/* added two new functions for sending the Disable/Enable EEPROM
/* Write packetized commands.
/* 
/*    Rev 1.27   08 Mar 1996 18:55:12   JEAN
/* Added some new internal variables for both serial and non-serial
/* CLLD.
/* 
/*    Rev 1.26   04 Mar 1996 15:02:58   truong
/* Conditional compiled out CSS support when building WinNT/95 drivers.
/* 
/*    Rev 1.25   06 Feb 1996 14:17:32   JEAN
/* Removed an unused serial variable, used some defines instead
/* of hard-coded constants.
/* 
/*    Rev 1.24   31 Jan 1996 12:53:28   JEAN
/* Changes some variable declarations for ODI driver support and
/* changed some internal variables to configuration variables.
/* 
/*    Rev 1.23   08 Jan 1996 14:10:02   JEAN
/* Added a variable for accessing the IRQ mask register on PIC 2
/* and deleted an unused variable, RxFIFODepth.
/* 
/*    Rev 1.22   19 Dec 1995 18:04:38   JEAN
/* Added variable declarations for serial cards.
/* 
/*    Rev 1.21   14 Dec 1995 15:07:06   JEAN
/* Added multiple card support for Serial CLLD.
/* 
/*    Rev 1.20   07 Dec 1995 12:05:54   JEAN
/* Removed LLDTimerTick and some test code accidently left
/* in WaitFlagSet().
/* 
/*    Rev 1.19   05 Dec 1995 12:04:20   JEAN
/* Changed an #if to an #ifdef for SERIAL.
/* 
/*    Rev 1.18   17 Nov 1995 16:33:04   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.17   12 Oct 1995 11:29:46   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* 
/*    Rev 1.16   17 Sep 1995 16:57:00   truong
/* Fixed deferral and fairness bug.
/* Changed default sync_alarm.
/* 
/*    Rev 1.15   10 Sep 1995 09:46:14   truong
/* Added routines to support roam_config and mac_optimize keywords.
/* 
/*    Rev 1.14   24 Jul 1995 18:35:50   truong
/* 
/*    Rev 1.13   20 Jun 1995 15:47:42   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.12   23 May 1995 09:39:52   tracie
/* Global variable changed to extern
/* 
/*    Rev 1.11   13 Apr 1995 09:29:42   tracie
/* xromtest version 
/* 
/*    Rev 1.10   16 Mar 1995 16:12:50   tracie
/* Added support for ODI
/* 
/*    Rev 1.9   09 Mar 1995 15:06:44   hilton
/* 
/*    Rev 1.8   22 Feb 1995 10:24:52   tracie
/* Added WBRP includes and LLDTimerTick Variable
/* 
/*    Rev 1.7   05 Jan 1995 09:52:56   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.6   28 Dec 1994 09:45:12   hilton
/* 
/*    Rev 1.5   27 Dec 1994 15:43:10   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:10   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:02:56   hilton
/* 
/*    Rev 1.0   20 Sep 1994 11:00:14   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

// #define NDIS_MINIPORT_DRIVER // disable code for init of card & socket services and 365 chip

#include <string.h>
#include	"constant.h"
#include	"lld.h"
#include	"pstruct.h"
#ifndef SERIAL
#include "port.h"
#include "hardware.h"
#else
#include "slldbuf.h"
#include "slld.h"
#include "slldhw.h"
#include "slldport.h"
#endif
#include	"asm.h"
#include "lldpack.h"
#include	"lldinit.h"
#include	"lldisr.h"
#include "css.h"
#include "pcmcia.h"
#include "blld.h"
#include	"bstruct.h"
#include "lldqueue.h"

#ifdef WBRP
#include "rptable.h"
#include "wbrp.h"
#endif


#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else

unsigned_16		StatusReg;
unsigned_8		StatusValue;	
unsigned_16		ControlReg;
unsigned_8		ControlValue;
unsigned_16		DataReg;
unsigned_16		IntSelReg;
unsigned_8		IntSelValue;
unsigned_16		EOIReg;

unsigned_8		NodeOverrideFlag = CLEAR;
unsigned_8		LLDSyncName [NAMELENGTH+1] = {0,0,0,0,0,0,0,0,0,0,0,0};

unsigned_8		SendInitCmdFlag;
unsigned_8		SetSecurityIDFlag;
unsigned_8		GetGlobalAddrFlag;
unsigned_8		GetROMVersionFlag;
unsigned_8		SetITOFlag;
unsigned_8		DisableHopFlag;
unsigned_8		SetRoamingFlag;
unsigned_8		DisableEEPROMFlag;
unsigned_8		EnableEEPROMFlag;
unsigned_8		GetRFNCStatsFlag;
unsigned_8		SendSearchContinueFlag;
unsigned_8		SendAbortSearchFlag;
unsigned_8		LLDSyncRSSIThreshold = 0;
unsigned_8		InDriverISRFlag = CLEAR;
unsigned_8		InPollFlag = CLEAR;
unsigned_8		InResetFlag = CLEAR;

unsigned_16		CCReg = CCREG;
unsigned_32		CCRegPtr;
unsigned_16		SocketOffset;
unsigned_8		LLDFUJITSUFlag;
unsigned_16		CSSClientHandle;
unsigned_16		CORAddress;
unsigned_8		PCMCIACardInserted;
unsigned_16		CSSDetected;

unsigned_8		Inserted;
unsigned_8		FirstInsertion;
unsigned_8		FirstTimeIns = 1;
unsigned_16 	WindowHandle;
unsigned_32		WindowBase;
unsigned_16 	PCMCIASocket;
unsigned_16 	IOAddress;
unsigned_16 	IOAddress2;
unsigned_8		BusWidth;
unsigned_8		IRQLine;
unsigned_32		MemSize;

#ifdef SERIAL
unsigned_8			InSerialISRFlag = CLEAR;
unsigned_8			RestoreBIOSTableFlag;
unsigned_16 FAR	*SaveBIOSAddr;
#endif

#ifdef ODI
extern unsigned_8		LLDSecurityIDString [3];
extern unsigned_8		LLDRSSIThreshold;
#else
unsigned_8				LLDSecurityIDString [3];
unsigned_8				LLDRSSIThreshold = 15;
#endif

unsigned_8		IntStackPtr;		/* Current stack pointer.					*/		
unsigned_8		IntStack [32];		/* To keep track of 8259 interrupts.   */

#ifdef SERIAL
unsigned_16    uartBaseAddr;  /* UART's base address                    */
unsigned_16    TxRcvHoldReg;  /* Transmit & Receive Holding Reg.- R0    */
unsigned_16    IntrEnableReg; /* Interrupt Enable Register      - R1    */
unsigned_16    IntrIdentReg;  /* Interrupt Identification Reg.  - R2    */
unsigned_16    FIFOCtrlReg;   /* FIFO Control Register          - R2    */
unsigned_16    LineCtrlReg;   /* Line Control/Data Format Reg.  - R3    */
unsigned_16    ModemCtrlReg;  /* Modem Control/RS-232 Output Crl- R4    */
unsigned_16    LineStatReg;   /* Line Status/Serialization Stat - R5    */
unsigned_16    ModemStatReg;  /* Modem Status/RS-232 Input Stat - R6    */
unsigned_8     ModemCtrlVal;  /* Current Modem Control Register value   */
unsigned_8     IRQLevel;      /* The interrupt level selected           */                            
unsigned_8     IRQMask;       /* The interrupt controller mask bit.     */
unsigned_8     IRQMaskReg;    /* The interrupt mask register address.   */
unsigned_8		IntrOutVal; 	/* Indicates if 8259 interrupts are			*/
										/* enabled or disabled.							*/

struct rcvStruct  uartRx;     /* Serial LLD's Receiving State           */
struct sndStruct  uartTx;     /* Serial LLD's Transmitting State        */

unsigned_8 LenLow;				/* Used to calculate the length checksum  */
unsigned_8 LenHi;					/* for a serial packet.  This is the 4th	*/
										/* byte in the packet (SOH, 2-byte length,*/
										/* and 1-byte checksum.							*/
unsigned_8 TxFIFODepth;	 		/* Size UART's Transmit FIFO					*/
unsigned_8 FIFOType;		 		/* Is it an 8250/16450 or a 16550 UART?	*/
unsigned_8 serialInStandby;	/* Used when testing standby mode with		*/
										/* RL2DIAG.											*/	

struct TxRcvQueue	*QLoad; 		/* Returned from uartGetABuf()			*/
unsigned_8			*QDBptr;		/* Points to Data buffer being loaded	*/

#endif   /* SERIAL */

extern unsigned_8		LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_8		LLDPhysAddress [ADDRESSLENGTH];
extern unsigned_16	LLDIOAddress1;
extern unsigned_8		LLDIntLine1;
extern unsigned_8		LLDDeferralSlot;
extern unsigned_8		LLDFairnessSlot;
extern unsigned_8		LLDSecurityID [3];
extern unsigned_8		LLDDisableHop;
extern unsigned_8		LLDTransferMode;
extern unsigned_8 	LLDRoamingFlag;
extern unsigned_8 	LLDBridgeFlag;
extern unsigned_8 	LLDSecurityIDOverride;
extern unsigned_8		LLDSyncState;

extern unsigned_8		ConfigureMACFlag;
extern unsigned_8		LLDTxMode;
extern unsigned_8		LLDTxModeSave;
extern unsigned_8		LLDNoRetries;
extern unsigned_8 	LLDPCMCIA;

extern unsigned_8		PCMCIACardInserted;
extern unsigned_8 	LLDInit365Flag;

extern unsigned_16	LLDLookAheadAddOn;
extern unsigned_8		LLDRoamingState;
extern unsigned_16	RoamStartTime;
extern unsigned_8		LLDMSTAFlag;
extern unsigned_8		LLDSleepFlag;

extern unsigned_16	LLDLivedTime;
extern unsigned_16	LLDKeepAliveTmr;
extern unsigned_8		InLLDSend;
extern unsigned_8		DisableLLDSend;
extern unsigned_8		LLDOutOfSyncCount;
extern unsigned_8		TxFlag;
extern unsigned_8		TxFlagCleared;
#ifndef SERIAL
extern unsigned_16	TxStartTime;
#else
extern unsigned_16	TxStartTime[128];
extern unsigned_8		TxInProgress[128];
extern unsigned_8		SLLDBaudrateCode;
#endif
extern unsigned_8		RxFlag;
extern unsigned_8		PacketSeqNum;
extern unsigned_8		LLDNeedReset;
extern unsigned_8		LLDSearchTime;

extern unsigned_32	LLDSent;
extern unsigned_32	LLDReSent;
extern unsigned_32	LLDSentCompleted;
extern unsigned_32	LLDRx;
extern unsigned_32	LLDRxLookAhead;
extern unsigned_32	LLDErrResetCnt;
extern unsigned_32	LLDNeedResetCnt;
extern unsigned_32	LLDRxTimeOuts;
extern unsigned_32	LLDTxTimeOuts;
extern unsigned_32	LLDOutSyncQDrp;
extern unsigned_32	LLDOutSyncDrp;
extern unsigned_32	LLDTimedQDrp;
extern unsigned_32	LLDFailureDrp;
extern unsigned_32	LLDNoReEntrantDrp;
extern unsigned_32	LLDSendDisabledDrp;
extern unsigned_32	LLDZeroRxLen;
extern unsigned_32	LLDTooBigRxLen;
extern unsigned_32	LLDTooBigTxLen;
extern unsigned_32	LLDBadCopyLen;
extern unsigned_32	ResetStatisticsTime;

#ifdef SERIAL
extern unsigned_32 	SLLDRxHdrErr;
extern unsigned_32 	SLLDRxChksumErr;
extern unsigned_32 	SLLDRxLenErr;
extern unsigned_32	SLLDOverrunErr;
extern unsigned_32	SLLDRxErrors;
#endif

extern unsigned_8		MulticastOnFlag;
extern unsigned_8		SetMulticastFlag;

extern unsigned_8		LLDRepeatingWBRP;
extern unsigned_8    LLDSyncAlarm;
extern unsigned_8    LLDRetryThresh;
extern unsigned_8    LLDPCMEnabled;
extern unsigned_16    LLDMaxRoamWaitTime;

extern unsigned_8		LLDRoamResponse;
extern unsigned_16	LLDRoamResponseTmr;
extern unsigned_8		LLDRoamRetryTmr;
extern unsigned_8		LLDNAKTime;
extern unsigned_8		LLDTxFragRetries;
extern unsigned_8		LLDChannel;
extern unsigned_8		LLDSubChannel;
extern unsigned_8		LLDNumMastersFound;
extern unsigned_8		LLDNodeType;
extern unsigned_8		LLDTicksToSniff;
extern unsigned_8		LLDSniffModeFlag;
extern unsigned_16	LLDInactivityTimeOut;

extern unsigned_8		LLDSniffModeFlag;
extern unsigned_8		InSearchContinueMode;
extern unsigned_8		PostponeSniff;
extern unsigned_8		NextMaster;
extern unsigned_8		LLDSniffCount;
extern unsigned_32	FramesXmitFrag;
extern unsigned_32	FramesXmit;
extern unsigned_32	FramesXmitBFSK;
extern unsigned_32	FramesXmitQFSK;
extern unsigned_32	FramesACKError;
extern unsigned_32	FramesCTSError;
extern unsigned_32	TotalTxPackets;
extern unsigned_32	FramesRx;
extern unsigned_32	FramesRxDuplicate;
extern unsigned_32	FramesRxFrag;
extern unsigned_16	RcvPktLength;
extern unsigned_16	LLDUnknownProximPkt;

#endif

#ifdef SERIAL
extern unsigned_16	serialTimeouts [MAX_BAUDRATES];
#endif

/***************************************************************************/
/*	PCMCIACleanUp																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*	DESCRIPTION:	This routine will return the PCMCIA system to the 			*/
/* 	pre-initialize state. 																*/
/*																									*/
/***************************************************************************/

void PCMCIACleanUp(void)
{
#ifndef NDIS_MINIPORT_DRIVER
#ifdef CSS
 	if (LLDInit365Flag)
 		TurnPowerOff();
 	else if (CSSDetected)
 	{	
 		if (PCMCIACardInserted)
 		{
 			CSSUnload();
 			DeRegisterClient();
 		}
 		else
 			AddResources();
 	}
#endif
#endif
}

/***************************************************************************/
/*																									*/
/*	LLDInit ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return Codes:																	*/
/*					0	-	Successful Initialization										*/
/*					1	-	Error resetting the RFNC										*/
/*					2	-	Error setting the interrupt vector							*/
/*					3	-	Error sending the initialize command						*/
/*					4	-	Error getting the ROM version									*/
/*					5	-	Error getting the global address								*/
/*					6	-	Error setting the local address								*/
/*					7	-	Error setting the inactivity time out						*/
/*					8	-	Error disabling hopping											*/
/*					9	-	Error setting security ID										*/
/*					10 -	Error sending configure MAC									*/
/*					11	-	Error sending set roaming parameters command				*/
/*					12	-	Error sending multicast command								*/
/*					13	-	Error in card and socket services							*/
/*					14	-	Error configuring socket - improper card seating		*/
/*					15	-	Error configuring socket - power imporperly supplied	*/
/*					16	-	Error configuring socket - no PCMCIA bus ready signal	*/
/*					17	-	Error in window or interrupt number							*/
/*					18	-	Error detecting one or two piece PCMCIA card				*/
/*																									*/
/*	DESCRIPTION:	This routine will initialize the hardware by resetting	*/
/* 	the RFNC, setup the interrupt vector, send initialize command, get	*/
/* 	global address, get rom version, and set the inactivity timeout.		*/
/*																									*/
/***************************************************************************/

unsigned_16 LLDInit (void)

{
	unsigned_16 SearchTime;

#ifndef NDIS_MINIPORT_DRIVER
#ifdef CSS
	unsigned_16 status;
#endif
#endif
	ResetStatisticsTime = (unsigned_32)LLSGetCurrentTimeLONG();
	LLDInitInternalVars ();
	LLDSyncState = CLEAR;
	initReEntrantQueues ();
	if (LLDSyncAlarm == 0x23)
		LLDMaxRoamWaitTime = MAXSLOWROAMTIME;
	LLDInitTable();

#ifdef SERIAL

	/*-------------------------------------------------*/
	/* Serial LLD's Initialization routines				*/
	/*-------------------------------------------------*/
	if ( SLLDInit () )
		return (REGISTER_INT_ERR);

#else
	/*-------------------------------------------------*/
	/* Check for PCMCIA card.  Check for card and		*/
	/* socket services or 365 chip.							*/
	/*																	*/
	/*-------------------------------------------------*/

#ifdef CSS
	if (LLDPCMCIA)
	{
#ifdef NDIS_MINIPORT_DRIVER
      PCMCIACardInserted = SET;
		if (CheckForOnePiece ())
		{	return( PCM_DETECT_ERR);
		}
#else

      if (GetCardServicesInfo () == 0)
	   {	PCMCIACardInserted = CLEAR;
		   CSSDetected = SET;
		   LLDInit365Flag = CLEAR;
			OutString ("CSSDet", YELLOW);

		   if (RegisterClient ())
		   {	return (CSS_ERR);
		   }
	
			OutString ("RegClnt", CYAN);
		   if (WaitFlagSet (&PCMCIACardInserted))
		   {	
				DeRegisterClient ();
			   return (CSS_ERR);
		   }
	   }
	   else
	   {	
			if (LLDInit365Flag)
		   {	if (ParameterRangeCheck ())
				   return (WINDOW_ERR);
			   status = ConfigureSocket ();
				if (status)
				   return (status + (SOCKET_ERR-1));
		   }
			PCMCIACardInserted = SET;
	   }

		/*-------------------------------------------*/
		/* See which PCMCIA card we are talking to.	*/
		/* The one and two piece PCMCIA cards have	*/
		/* different CCReg addresses.						*/
		/*-------------------------------------------*/
		if (CheckForOnePiece ())
		{	
			PCMCIACleanUp();
			return( PCM_DETECT_ERR);

		}

	   SetPCMCIAForInterruptType ();
#endif
	}
#endif

	if (LLDTicksToSniff != 0)
		if (!LLDInactivityTimeOut)
		{
			LLDSniffModeFlag = SET;
			LLDInactivityTimeOut = 1;
		}

	/*-------------------------------------------------*/
	/* Set up the card's I/O port addresses.				*/
	/*																	*/
	/*-------------------------------------------------*/

	DataReg = LLDIOAddress1;
	StatusReg = LLDIOAddress1 + 2;
	ControlReg = LLDIOAddress1 + 4;
	EOIReg = LLDIOAddress1 + 5;
	IntSelReg = LLDIOAddress1 + 6;
	
	if (ResetRFNC (&LLDTransferMode, LLDIntLine1))
	{	
		PCMCIACleanUp();
		return (RESET_ERR);
	}

 	if (LLSRegisterInterrupt (LLDIntLine1, LLDIsr))
	{	
		PCMCIACleanUp();
		return (REGISTER_INT_ERR);
	}				
	EnableRFNCInt ();

#endif	

	if (LLDNodeAddress [0] != 0xFF)
	{	NodeOverrideFlag = SET;
	}
	else
	{	NodeOverrideFlag = CLEAR;
	}

	if (SendInitialize ())
	{	
		LLSDeRegisterInterrupt (LLDIntLine1);
		PCMCIACleanUp();
		return (INITIALIZE_ERR);
	}

	
	/*----------------------------------------------------------------*/
	/*	If the LLDSearchTime has been set, search for the masters will	*/
	/*	be performed and a master to sync to will be chosen randomly	*/
	/*																						*/
	/*----------------------------------------------------------------*/

	if(LLDSearchTime && LLDNodeType == STATION)
	{
		if (SendSearchContinue ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (SET_SC_ERR);
		}
		
		SearchTime = (unsigned_16)LLSGetCurrentTime();
		while (((unsigned_16)LLSGetCurrentTime()) - SearchTime <= LLDSearchTime)
			;

		if (ResetRFNC (&LLDTransferMode, LLDIntLine1))
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (RESET_ERR);
		}

		LLDSearchTime = CLEAR;
		EnableRFNCInt ();

		if (LLDNumMastersFound)
			LLDChooseMaster();

		if (SendInitialize ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (INITIALIZE_ERR);
		}

	}	

	/*-------------------------------------------------*/
	/*																	*/
	/*	If we are running in a bridge, we must check to */
	/* see if we need	to override the security ID.		*/
	/*																	*/
	/*-------------------------------------------------*/

	if (LLDBridgeFlag && LLDSecurityIDOverride)
	{	LLDSecurityID [0] = LLDSecurityIDString[0];
		LLDSecurityID [1] = LLDSecurityIDString[1];
		LLDSecurityID [2] = LLDSecurityIDString[2];

		if (SendSetSecurityID ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (SET_SID_ERR);
		}

		DisableRFNCInt();
		if (ResetRFNC (&LLDTransferMode, LLDIntLine1))
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (RESET_ERR);
		}
		EnableRFNCInt();

		if (SendInitialize ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (INITIALIZE_ERR);
		}

	}

	/*-------------------------------------------------------------*/
	/* Don't allow any writes to EEPROM.  This mode gets cleared	*/
	/* by resetting the card or sending an enable EEPROM write		*/
	/* packetized command.														*/
	/*-------------------------------------------------------------*/
	SendDisableEEPROMWrite ();

	/*-------------------------------------------------*/
	/*																	*/
	/*	Perform the rest of the initialization.			*/
	/*																	*/
	/*-------------------------------------------------*/

	if (SendGetGlobalAddr ())
	{	
		LLSDeRegisterInterrupt (LLDIntLine1);
		PCMCIACleanUp();
		return (GET_ADDRESS_ERR);
	}

	if (NodeOverrideFlag != SET)
	{	CopyMemory (&LLDNodeAddress, &LLDPhysAddress, ADDRESSLENGTH);
	}

	if (SendGetROMVersion ())
	{	
		LLSDeRegisterInterrupt (LLDIntLine1);
		PCMCIACleanUp();
		return (GET_ROM_VERSION_ERR);
	}

	if (SendSetITO ())
	{	
		LLSDeRegisterInterrupt (LLDIntLine1);
		PCMCIACleanUp();
		return (INACTIVITY_ERR);
	}

	if (LLDDisableHop < MAXFREQUENCY)
	{	if (SendDisableHop ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (DISABLE_HOP_ERR);
		}
	}

	if ((LLDDeferralSlot != 0xFF) || (LLDFairnessSlot != 0xFF))
	{	
		if (SendConfigMAC ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (CONFIG_MAC_ERR);
		}
	}

	if (LLDRoamingFlag == 0)
	{	if (SendSetRoaming ())
		{	
			LLSDeRegisterInterrupt (LLDIntLine1);
			PCMCIACleanUp();
			return (SET_ROAMING_ERR);
		}
	}

	LLDTxModeSave = LLDTxMode;

#ifdef WBRP

	if (LLDRepeatingWBRP)
	{
		WBRPRepeaterInit ();
	}
#endif

	/*----------------------------------------------*/
	/* Give the card a chance to sync up with a		*/
	/* master before returning from initialization.	*/
	/*----------------------------------------------*/
#ifdef NDIS_MINIPORT_DRIVER
	WaitFlagSet (&LLDSyncState);
#endif
	return (SUCCESS);

}


/***************************************************************************/
/*																									*/
/*	LLDChooseMaster ()																		*/
/*																									*/
/*	INPUT: nothing																				*/
/*																									*/
/*	OUTPUT: nothing																			*/
/*																									*/
/*	DESCRIPTION:	Randomly chooses a master from the masters list.			*/
/*																									*/
/***************************************************************************/

void LLDChooseMaster (void)
{
	unsigned_16	Seed;

#ifdef NDIS_MINIPORT_DRIVER
	Seed = (unsigned_16)LLSGetTimeInMicroSec();
#else
	Seed = (unsigned_16)LLSGetCurrentTime();
#endif
	Seed = Seed % LLDNumMastersFound;
	LLDChannel = (unsigned_8)(LLDMastersFound [Seed].ChSubChannel >> 4);
	LLDSubChannel = LLDMastersFound [Seed].ChSubChannel;
	LLDSubChannel &= 0x0F;

}

/***************************************************************************/
/*																									*/
/*	LLDInitInternalVars ()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine will initialize the internal variables 		*/
/*		used by the LLD.  Because the LLD can handle multiple cards, the		*/
/*		variables need to be initialized here and not when they are defined.	*/
/*																									*/
/***************************************************************************/

void LLDInitInternalVars (void)

{
	LLDLookAheadAddOn 	= LOOKAHEADADDON;
	InDriverISRFlag 		= 0;
	InPollFlag		 		= 0;
	InResetFlag		 		= 0;
	LLDRoamingState 		= 0;
	RoamStartTime 			= 0;
	LLDMSTAFlag 			= 0;
	LLDSleepFlag 			= 0;

	LLDLivedTime 			= 0;
	LLDKeepAliveTmr 		= LLSGetCurrentTime();
	InLLDSend 				= 0;
	DisableLLDSend 		= 0;
	LLDOutOfSyncCount 	= 0;
	LLDTxModeSave 			= TX_AUTO;
	LLDNoRetries			= 0;
	TxFlag 					= 0;
#ifndef SERIAL
	TxStartTime 			= 0;
#else
	SetMemory (TxStartTime, 0, 128*sizeof(unsigned_16));
	SetMemory (TxInProgress, 0, 128);
#endif
	RxFlag 					= 0;
	PacketSeqNum 			= 0;
	LLDNeedReset 			= 0;

	LLDSent 					= 0;
	LLDReSent				= 0;
	LLDSentCompleted	 	= 0;
	LLDRx					 	= 0;
	LLDRxLookAhead		 	= 0;
	LLDErrResetCnt		 	= 0;
	LLDNeedResetCnt	 	= 0;
	LLDRxTimeOuts		 	= 0;
	LLDTxTimeOuts		 	= 0;
	LLDOutSyncQDrp			= 0;
	LLDOutSyncDrp			= 0;
	LLDTimedQDrp			= 0;
	LLDFailureDrp			= 0;
	LLDNoReEntrantDrp		= 0;
	LLDSendDisabledDrp	= 0;
	LLDZeroRxLen			= 0;
	LLDTooBigRxLen			= 0;
	LLDTooBigTxLen			= 0;
	LLDBadCopyLen			= 0;

#ifdef SERIAL
	InSerialISRFlag 		= 0;
	RestoreBIOSTableFlag	= 0;
	SaveBIOSAddr			= 0;
	SLLDRxHdrErr			= 0;
	SLLDRxChksumErr		= 0;
	SLLDRxLenErr			= 0;
	SLLDOverrunErr			= 0;
	SLLDRxErrors			= 0;
#endif

	MulticastOnFlag 		= 0;
	IntStackPtr 			= 0;

	SetMulticastFlag 		= 0;

	CORAddress 				= 0;
	PCMCIACardInserted 	= 0;
	CSSDetected 			= 0;
	Inserted 				= 0;
	FirstInsertion 		= 1;
	FirstTimeIns			= 1;
 	WindowHandle 			= 0;
	WindowBase 				= 0;
 	PCMCIASocket 			= 0;
 	IOAddress 				= 0;
 	IOAddress2 				= 0;
	BusWidth 				= 16;
	IRQLine 					= 0;
	MemSize					= 0x00002000;
	
	LLDFUJITSUFlag 		= 0;
	CCReg                = CCREG;

	LLDMaxRoamWaitTime	= MAXROAMWAITTIME;
	LLDRoamResponse		= 0;
	LLDRoamResponseTmr	= 0;
	LLDRoamRetryTmr		= 0;

	LLDNAKTime				= 100;
	LLDTxFragRetries		= 0;

	LLDSearchTime			= 0;
	LLDNumMastersFound	= 0;
	SendSearchContinueFlag = 0;
	SendAbortSearchFlag	= 0;

	LLDSniffModeFlag		= 0;
	InSearchContinueMode	= 0;
	PostponeSniff			= 0;
	NextMaster				= 0;
	LLDSniffCount			= 0;
	FramesXmitFrag			= 0;
	FramesXmit				= 0;
	FramesXmitBFSK			= 0;
	FramesXmitQFSK			= 0;
	FramesACKError			= 0;
	FramesCTSError			= 0;
	TotalTxPackets			= 0;
	FramesRx					= 0;
	FramesRxDuplicate		= 0;
	FramesRxFrag	  		= 0;
	RcvPktLength			= 0;
	LLDUnknownProximPkt	= 0;

#ifdef SERIAL
	TxFIFODepth				= 16;
	serialInStandby		= 0;
	IntrOutVal				= 0;
#endif

}


/***************************************************************************/
/*																									*/
/*	LLDMacOptimize (MACOptimize)													      */
/*																									*/
/*	INPUT:         MACOptimize = 0 - Light                                  */
/*                              1 - Normal                                 */
/*																									*/
/*	OUTPUT:			Success or Failure													*/
/*																									*/
/*	DESCRIPTION:	This routine initializes the mac configuration           */
/*    parameters to optimize for the specified network size.               */
/*																									*/
/***************************************************************************/

int LLDMacOptimize (int MACOptimize)
{
   ConfigureMACFlag = SET;

   switch (MACOptimize)
   {
      case 0:
         LLDDeferralSlot = 1-1;
         LLDFairnessSlot = 2-1;
         break;
      case 1:
         LLDDeferralSlot = 4-1;
         LLDFairnessSlot = 4-1;
         break;
      default:
         return(FAILURE);
   }
   return(SUCCESS);
}

/***************************************************************************/
/*																									*/
/*	LLDRoamConfig (RoamConfig) 	 											         */
/*																									*/
/*	INPUT:         RoamConfig = 0 - Slow                                    */
/*                             1 - Normal                                  */
/*                             2 - Fast                                    */
/*																									*/
/*	OUTPUT:			Success or Failure													*/
/*																									*/
/*	DESCRIPTION:   This routine initializes the roaming parameters to       */
/*    optimize roaming to the specified AP Coverage.                       */
/*																									*/
/***************************************************************************/

int LLDRoamConfig (int RoamConfig)
{

   switch (RoamConfig)
   {
      case 2:
         LLDRSSIThreshold = 5;
         LLDRetryThresh = 4;
         LLDSyncAlarm = 0x17;
         break;
      case 1:
         LLDRSSIThreshold = 15;
         LLDRetryThresh = 6;
         LLDSyncAlarm = 0x27;
         break;
      case 0:
         LLDRSSIThreshold = 5;
         LLDRetryThresh = 6;
         LLDSyncAlarm = 0x23;
         break;
      default:
         return(FAILURE);
   }
   return(SUCCESS);
}

/***************************************************************************/
/*																									*/
/*	SendInitialize ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the initialize command and wait		*/
/*		for a timeout or a successful return.											*/
/*																									*/
/***************************************************************************/

unsigned_8 SendInitialize (void)

{
	SendInitCmdFlag = CLEAR;
	if (LLDSendInitializeCommand ())
	{	return (FAILURE);
	}
	return (WaitFlagSet (&SendInitCmdFlag));

}


/***************************************************************************/
/*																									*/
/*	SendSearchContinue ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Search Continue			 			*/
/* 	command and wait for a timeout or a successful return.					*/
/*																									*/
/***************************************************************************/

unsigned_8 SendSearchContinue (void)

{
	SendSearchContinueFlag = CLEAR;
	if (LLDSearchContinue ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SendSearchContinueFlag));
}


/***************************************************************************/
/*																									*/
/*	SendAbortSearch ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Abort Search 			 			*/
/* 	command and wait for a timeout or a successful return.					*/
/*																									*/
/***************************************************************************/

unsigned_8 SendAbortSearch (void)

{
	SendAbortSearchFlag = CLEAR;
	if (LLDAbortSearch ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SendAbortSearchFlag));
}



/***************************************************************************/
/*																									*/
/*	SendSetSecurityID ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Set Security ID command			*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendSetSecurityID (void)

{
	SetSecurityIDFlag = CLEAR;
	if (LLDSetSecurityID ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SetSecurityIDFlag));
}




/***************************************************************************/
/*																									*/
/*	SendGetGlobalAddr ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the GetGlobalAddress command			*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendGetGlobalAddr (void)

{
	GetGlobalAddrFlag = CLEAR;
	if (LLDGetGlobalAddress ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&GetGlobalAddrFlag));
}




/***************************************************************************/
/*																									*/
/*	SendGetROMVersion ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the GetROMVersion command				*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendGetROMVersion (void)

{
	GetROMVersionFlag = CLEAR;
	if (LLDGetROMVersion ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&GetROMVersionFlag));
}




/***************************************************************************/
/*																									*/
/*	SendSetITO ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the SetInactivity Timeout command	*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendSetITO (void)

{
	SetITOFlag = CLEAR;
	if (LLDSetITO ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SetITOFlag));
}




/***************************************************************************/
/*																									*/
/*	SendDisableHop ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Disable Hopping command			*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendDisableHop (void)

{
	DisableHopFlag = CLEAR;
	if (LLDDisableHopping ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&DisableHopFlag));
}




/***************************************************************************/
/*																									*/
/*	SendConfigMAC ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Configure MAC command				*/
/*		and wait for a timeout or a successful return.								*/
/*																									*/
/***************************************************************************/

unsigned_8 SendConfigMAC (void)

{
	ConfigureMACFlag = CLEAR;
	if (LLDConfigMAC ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&ConfigureMACFlag));
}




/***************************************************************************/
/*																									*/
/*	SendSetRoaming ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Set Roaming Parameters			*/
/* 	command and wait for a timeout or a successful return.					*/
/*																									*/
/***************************************************************************/

unsigned_8 SendSetRoaming (void)

{
	SetRoamingFlag = CLEAR;
	if (LLDSetRoaming ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SetRoamingFlag));
}


/***************************************************************************/
/*																									*/
/*	SendDisableEEPROMWrite () 																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Send Disable EEPROM Write		*/
/*		packetize command and waits for a timeout or a successful return.		*/
/*																									*/
/***************************************************************************/

unsigned_8 SendDisableEEPROMWrite (void)

{
	DisableEEPROMFlag = CLEAR;
	if (LLDDisableEEPROMWrite ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&DisableEEPROMFlag));
}


/***************************************************************************/
/*																									*/
/*	SendEnableEEPROMWrite () 																*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Send Enable EEPROM Write			*/
/*		packetize command and waits for a timeout or a successful return.		*/
/*																									*/
/***************************************************************************/

unsigned_8 SendEnableEEPROMWrite (void)

{
	EnableEEPROMFlag = CLEAR;
	if (LLDEnableEEPROMWrite ())
	{	return (FAILURE);
	}

	return (WaitFlagSet (&EnableEEPROMFlag));
}





/***************************************************************************/
/*																									*/
/*	WaitFlagSet (*Flag)																		*/
/*																									*/
/*	INPUT:	Address of flag to wait on.												*/
/*																									*/
/*	OUTPUT:	ReturnCode will return SUCCESS or FAILURE								*/
/*																									*/
/*	DESCRIPTION:  This routine will wait for a flag to be set or a time		*/
/*		out.																						*/
/*																									*/
/***************************************************************************/

unsigned_8 WaitFlagSet (unsigned_8 *Flag)

{	unsigned_16	StartTime;
	FLAGS			oldflag;

	/*-------------------------------------------------------------------*/
	/* Make sure interrupts are enabled so that the time can be updated.	*/
	/*																							*/
	/*-------------------------------------------------------------------*/
	oldflag = PreserveFlag ();
	EnableInterrupt ();
	
	StartTime = LLSGetCurrentTime ();

#ifndef SERIAL
	while (((unsigned_16)(LLSGetCurrentTime() - StartTime) <= MAXWAITTIME ) && (*Flag != SET))
		;
#else
	while ((((unsigned_16)LLSGetCurrentTime() - StartTime) <=  serialTimeouts [SLLDBaudrateCode]) &&
			 (*Flag != SET))
		;
#endif

	RestoreFlag (oldflag);
	if (*Flag != SET)
	{	return (FAILURE);
	}
	else
	{	return (SUCCESS);
	}
}
