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
/*	$Header:   J:\pvcs\archive\clld\lldctrl.c_v   1.50   25 Sep 1998 08:50:18   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldctrl.c_v  $										*/
/* 
/*    Rev 1.50   25 Sep 1998 08:50:18   BARBARA
/* Update in the SERIAL part of the code.
/* 
/*    Rev 1.49   15 Sep 1998 13:48:12   BARBARA
/* Read_Port_Off added.
/* 
/*    Rev 1.48   06 May 1998 13:55:32   BARBARA
/* Comments modified.
/* 
/*    Rev 1.47   24 Mar 1998 09:41:56   BARBARA
/* LLDGotoSniffMode added. LLD Ping added.
/* 
/*    Rev 1.46   06 Feb 1997 13:29:58   BARBARA
/* If TxFlag and OutOfSync are set then ResetQ is needed instead off LLDReset.
/* 
/*    Rev 1.45   24 Jan 1997 17:42:32   BARBARA
/* Resetting after 5 sec. of NAKs added.
/* 
/*    Rev 1.44   15 Jan 1997 16:59:50   BARBARA
/* LLDTxRoamNotify is done in LLDPoll now.
/* 
/*    Rev 1.43   04 Dec 1996 16:41:06   JEAN
/* Placed a one-piece PCMCIA function call under the CSS macro.
/* 
/*    Rev 1.42   14 Oct 1996 10:29:54   BARBARA
/* R_CtrlReg was added to LLDPoll function to fix 1pc PCMCIA power draw problem.
/* 
/*    Rev 1.41   02 Oct 1996 13:37:38   JEAN
/* In LLDStop(), we now call PCMCIACleanUp() to unload
/* C&SS and degregister the client.  If C&SS was not loaded, then
/* this function will also power off the 365 chip.
/* 
/*    Rev 1.40   26 Sep 1996 10:51:24   JEAN
/* 
/*    Rev 1.39   24 Sep 1996 16:59:56   BARBARA
/* Do not wait for in sync when API is running.
/* 
/*    Rev 1.38   29 Jul 1996 12:17:02   JEAN
/* For the serial driver, fixed a problem where we were not
/* detecting missed transmit buffer interrupts correctly.
/* 
/*    Rev 1.37   15 Jul 1996 19:15:32   TERRYL
/* Checked InactivityTimeOut before sending out keep alive packets.
/* 
/*    Rev 1.36   15 Jul 1996 17:35:16   TERRYL
/* Type-casted the time difference (from LLSGetCurrentTime() routine) to 
/* 16-bit.
/* 
/*    Rev 1.35   14 Jun 1996 16:08:12   JEAN
/* 
/*    Rev 1.34   14 Jun 1996 16:03:30   JEAN
/* Changed some conditional compilation lables.  Also changed the
/* way we determine if a keep alive packet needs to be send.
/* 
/*    Rev 1.33   28 May 1996 16:08:42   TERRYL
/* Added WriteStatusLine for Win95
/* 
/*    Rev 1.32   14 May 1996 09:38:36   TERRYL
/* Added more debug information
/* 
/*    Rev 1.31   16 Apr 1996 11:13:26   JEAN
/* Fixed a couple of problems.  Added a return status check 
/* after calling LLDSendKeepAlive, cleared the entry in LLDMapTable
/* when a serial transmission times out, and fixed a bug when
/* testing the CTS and DSR bits for serial.
/* 
/*    Rev 1.30   01 Apr 1996 11:14:26   JEAN
/* Added an include file for serial builds.
/* 
/*    Rev 1.29   01 Apr 1996 09:24:14   JEAN
/* When a serial transmit packet times out, added a test to
/* validate the table entry number.
/* 
/*    Rev 1.28   29 Mar 1996 10:42:02   JEAN
/* Fixed a bug in LLDMulticast () where we were calling
/* LLDSetPromiscuous () instead of LLDSetMulticast ().  For serial,
/* added code to check to see if the card has woken up when there
/* is data waiting to be transmitted.
/* 
/*    Rev 1.27   21 Mar 1996 13:11:54   JEAN
/* Added a call to RestoreBIOSMem() when LLDStop is called.
/* 
/*    Rev 1.26   15 Mar 1996 13:49:08   TERRYL
/* Added SetPCMCIAForInterruptType() in LLDReset()
/* 
/*    Rev 1.25   14 Mar 1996 15:03:44   JEAN
/* Added an ifdef around an extern declaration.
/* 
/*    Rev 1.24   14 Mar 1996 14:39:36   JEAN
/* Added a timeout value for each baudrate for SERIAL CLLD.
/* 
/*    Rev 1.23   13 Mar 1996 17:48:08   JEAN
/* Added some debug code.
/* 
/*    Rev 1.22   12 Mar 1996 10:33:28   JEAN
/* Added a call to SendDisableEEPROMWrite () in LLDReset (),
/* 
/*    Rev 1.21   08 Mar 1996 19:39:44   JEAN
/* Changed the way we handle transmit packet timeouts for serial
/* cards.  Added a flag to prevent LLDReset form being re-entered.
/* Added a flag to prevent LLDPoll from being re-entered.  Added
/* a check in LLDPoll for in LLDPoll and in LLDReset.
/* Fixed a hard coded constant for TPS in LLDPoll.
/* 
/*    Rev 1.20   04 Mar 1996 14:59:42   truong
/* Added InDriverISRFlag keep poll from entering LLDISR.
/* 
/*    Rev 1.19   06 Feb 1996 14:15:32   JEAN
/* Added code to LLDStop() to disable the UART for serial cards.
/* 
/*    Rev 1.18   31 Jan 1996 12:49:46   JEAN
/* Modified LLDPoll() to just reset the RFNC once, added some
/* statistics, and changed the ordering of some header include
/* files.
/* 
/*    Rev 1.17   14 Dec 1995 15:04:02   JEAN
/* Added a header include file.
/* 
/*    Rev 1.16   07 Dec 1995 12:04:46   JEAN
/* Removed some header includes and LLDTimerTick.  Time is
/* now kept in an LLS routine.
/* 
/*    Rev 1.15   05 Dec 1995 12:03:22   JEAN
/* Changed an #if to an #ifdef for SERIAL.
/* 
/*    Rev 1.14   17 Nov 1995 16:32:04   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.13   12 Oct 1995 11:26:18   JEAN
/* Added changes for SERIAL.  When resetting the RFNC, we need to
/* clear any pending 8259 interrupts.  Also, only include SERIAL
/* header files when compiling for SERIAL.
/* 
/*    Rev 1.12   20 Jun 1995 15:46:50   hilton
/* Change
/* 
/* 
/*    Rev 1.11   13 Apr 1995 08:46:44   tracie
/* XROMTEST version
/* 
/*    Rev 1.10   16 Mar 1995 16:07:20   tracie
/* Added support for ODI
/* 
/*    Rev 1.9   09 Mar 1995 15:06:04   hilton
/* 
/* 
/*    Rev 1.8   22 Feb 1995 10:31:32   tracie
/* Add WBRP functions
/* 
/*    Rev 1.7   05 Jan 1995 09:52:16   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.6   28 Dec 1994 09:44:58   hilton
/* 
/*    Rev 1.5   27 Dec 1994 15:42:38   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:08   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:02:52   hilton
/* 
/*    Rev 1.0   20 Sep 1994 10:58:46   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/

#ifdef SERIAL
#include <conio.h>
#endif
#include "constant.h"
#include "lld.h"
#include "lldpack.h"
#include "blld.h"
#include "pstruct.h"
#include "lldinit.h"
#include "lldctrl.h"
#include "pcmcia.h"
#include "asm.h"
#include "css.h"
#ifndef SERIAL
#include "port.h"
#include "hardware.h"
#include "lldsend.h"
#else
#include "slld.h"
#include "slldbuf.h"
#include "slldhw.h"
#include "slldport.h"
#endif
#include	"bstruct.h"

#ifdef WBRP
#include "rptable.h"
#include "wbrp.h"
#endif


/*-------------------------------------------------------*/
/* For serial cards, if we send three packets in a row	*/
/* to the RFNC without getting back a response, we can	*/
/* assume the card is dead.  When this occurs, we need	*/
/* to reset the card.												*/
/*-------------------------------------------------------*/

#ifdef NDIS_MINIPORT_DRIVER
extern unsigned_8		API_FlagLLD;
#endif
extern unsigned_8		LLDOnePieceFlag;

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8		LLDInactivityTimeOut;
extern unsigned_8		LLDTransferMode;
extern unsigned_8		LLDIntLine1;
extern unsigned_16	LLDKeepAliveTmr;
extern unsigned_8		LLDDeferralSlot;
extern unsigned_8		LLDFairnessSlot;
extern unsigned_8		LLDDisableHop;
extern unsigned_8 	LLDRoamingFlag;
extern unsigned_8		LLDPCMCIA;
extern unsigned_8		LLDInit365Flag;
extern unsigned_8		InPollFlag;
extern struct			NodeEntry NodeTable [TOTALENTRY];
extern unsigned_8		LLDMapTable [128];

extern unsigned_16	CSSDetected;
extern unsigned_8		PCMCIACardInserted;

extern unsigned_8		LLDSyncState;
extern unsigned_8		MulticastOnFlag;

extern unsigned_8		LLDNeedReset;
extern unsigned_16	RxStartTime;
extern unsigned_8		RxFlag;
extern unsigned_8		TxFlag;
extern unsigned_8		TxFlagCleared;
extern unsigned_8		LLDSyncAlarm;
#ifndef SERIAL
extern unsigned_16	TxStartTime;
#else
extern unsigned_16	TxStartTime[128];
extern unsigned_8		TxInProgress[128];
extern unsigned_16 	LineStatReg;
extern unsigned_16 	ModemStatReg;
extern struct			sndStruct  uartTx;
#endif
extern unsigned_8		LLDRoamingState;
extern unsigned_16	RoamStartTime;
extern unsigned_8 	LLDSniffTime;
extern unsigned_8		InDriverISRFlag;

extern unsigned_8		SendInitCmdFlag;
extern unsigned_8		InDriverISRFlag;
extern unsigned_8		InLLDSend;
extern unsigned_8		InResetFlag;
		
extern unsigned_8 	LLDPCMCIA;
extern unsigned_16	WBRPStartTime;
extern unsigned_32	LLDErrResetCnt;
extern unsigned_32	LLDNeedResetCnt;
extern unsigned_32 	LLDRxTimeOuts;
extern unsigned_32	LLDTxTimeOuts;

extern unsigned_8		LLDDebugColor;
extern unsigned_8		LLDRoamRetryTmr;
extern unsigned_8		LLDRoamResponse;
extern unsigned_8		LLDNAKTime;
extern unsigned_16	LLDRoamResponseTmr;
extern unsigned_16	LLDMaxRoamWaitTime;
extern unsigned_8		LLDMSTAAddr [ADDRESSLENGTH];
extern unsigned_8		LLDOldMSTAAddr [ADDRESSLENGTH];

extern unsigned_8		LLDSniffModeFlag;
extern unsigned_8		LLDSniffCount;
extern unsigned_8		LLDTicksToSniff;
extern unsigned_8		PostponeSniff;
extern unsigned_8		LLDMSTAFlag;

extern unsigned_8		LLDReadPortOff;
unsigned_8	SetMulticastFlag;

#ifdef ODI
extern unsigned_8	DriverNoResetFlag;
#else
unsigned_8	DriverNoResetFlag;
#endif

#endif

#ifdef SERIAL
#define MAX_TX_ALLOWED 2

#ifndef MULTI
extern unsigned_8	SLLDBaudrateCode;
extern unsigned_8	RestoreBIOSTableFlag;
#endif

int tbeCount = 0;
int missedTBE = 0;

unsigned_16 serialTimeouts [MAX_BAUDRATES] = 
{	(TPS*60*7),		/*   1200 -  7 minutes	*/
	(TPS*60*5),		/*   2400 -  5 minutes	*/
	(TPS*60*2),		/*   4800 -  2 minutes	*/
	(TPS*60*1),		/*   9600 -  1 minute	*/
	(TPS*50),		/*  19200 - 50 seconds	*/
	(TPS*40),		/*  38400 - 40 seconds	*/
	(TPS*30),		/*  57600 - 30 seconds	*/
	(TPS*20),		/* 115200 - 20 seconds	*/
};
#endif


/***************************************************************************/
/*																									*/
/*	LLDReset ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return Codes:																	*/
/*					0	-	Successful Initialization										*/
/*					1	-	Error resetting the RFNC										*/
/*					3	-	Error sending the initialize command						*/
/*					7	-	Error setting the inactivity time out						*/
/*					8	-	Error disabling hopping											*/
/*					10 -	Error sending configure MAC									*/
/*					11	-	Error sending set roaming parameters command				*/
/*					12	-	Error sending set multicast command							*/
/*																									*/
/*	DESCRIPTION:	This routine will initialize the hardware by resetting	*/
/* 	the RFNC and re-initializing the card.											*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDReset (void)

{	
	/*-------------------------------------------------*/
	/* Problems:													*/
	/*		Sometimes during Data transmitting and Out   */
	/*    of Sync occurs, the interrupt is disable and */
	/*    the RESET won't work.  Therefore, we must    */
	/*		make sure all the interrupts are turned on   */
	/*    before the reset process.							*/
	/*-------------------------------------------------*/

	if (InResetFlag)
		return (INITIALIZE_ERR);

	InResetFlag = SET;

	LLDErrResetCnt++;
	PushRFNCInt ();
	DisableRFNCInt ();

	LLDSyncState = CLEAR;	
	InLLDSend = CLEAR;

	LLDRoamRetryTmr = 0;
	LLDRoamResponse = CLEAR;
	LLDRoamResponseTmr = 0;
	LLDRoamingState = NOT_ROAMING;


#ifdef CSS
	if (LLDPCMCIA && PCMCIACardInserted)
	{
		if (!CSSDetected)
			CheckForOnePiece ();
		SetPCMCIAForInterruptType();	
	}
#endif

	if (ResetRFNC (&LLDTransferMode, LLDIntLine1))
	{ 
		PopRFNCInt ();
		InResetFlag = CLEAR;
		return (RESET_ERR);
	}

	PopRFNCInt ();
	EnableRFNCInt ();
	
	/*
	 * Issue an EOI to the interrupt controller to clear
	 * an pending interrupts.  If we don't the system
	 * could hang.
	 */
#ifdef SERIAL
	EOI (LLDIntLine1);
#endif

	EnableInterrupt ();

	if (SendInitialize ())
	{ 
		InResetFlag = CLEAR;
		return (INITIALIZE_ERR);
	}

	/*-------------------------------------------------------------*/
	/* Don't allow any writes to EEPROM.  This mode gets cleared	*/
	/* by resetting the card or sending an enable EEPROM write		*/
	/* packetized command.														*/
	/*-------------------------------------------------------------*/
	SendDisableEEPROMWrite ();

	if (SendGetGlobalAddr ())
	{	
		InResetFlag = CLEAR;
		return (GET_ADDRESS_ERR);
	}

	if (MulticastOnFlag)
	{	if (LLDMulticast (1))
		{	
			InResetFlag = CLEAR;
			return (MULTICAST_ERR);
		}
	}

	if (SendSetITO ())
	{	
		InResetFlag = CLEAR;
		return (INACTIVITY_ERR);
	}

	if (LLDDisableHop < MAXFREQUENCY)
	{	if (SendDisableHop ())
		{	
			InResetFlag = CLEAR;
			return (DISABLE_HOP_ERR);
		}
	}

	if ((LLDDeferralSlot != 0xFF) || (LLDFairnessSlot != 0xFF))
	{	if (SendConfigMAC ())
		{	
			InResetFlag = CLEAR;
			return (CONFIG_MAC_ERR);
		}
	}		

	if (LLDRoamingFlag == 0)
	{	if (SendSetRoaming ())
		{	
			InResetFlag = CLEAR;
			return (SET_ROAMING_ERR);
		}
	}

	/*----------------------------------------------*/
	/* Give the card a chance to sync up with a		*/
	/* master before returning from initialization.	*/
	/*----------------------------------------------*/
#ifdef NDIS_MINIPORT_DRIVER
	if (!(API_FlagLLD))
#endif
   WaitFlagSet(&LLDSyncState);
	InResetFlag = CLEAR;

	return (SUCCESS);
}





/***************************************************************************/
/*																									*/
/*	LLDPoll ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will check for timeout and takes care of		*/
/* 	sleep mode stuff.  Also, will send packets if send delay flag			*/
/* 	is set from driver send.															*/
/*																									*/
/***************************************************************************/

void LLDPoll (void)

{
#ifdef WBRP
	unsigned_16	timerticks;
#endif
#ifdef SERIAL
	unsigned_8 TableEntry;
	unsigned_8 status;
	int ii;
#endif

	/*-------------------------------------------------------*/
	/* If we are in the middle of doing something critical,	*/
	/* we want skip this routine.  We'll catch it the next	*/
	/* clock tick.															*/
	/*-------------------------------------------------------*/
	if (InLLDSend || InDriverISRFlag || InPollFlag || InResetFlag)
		return;

	InPollFlag = SET;

	if (LLDNAKTime != 100)
	{
		if (! (--LLDNAKTime)) 
		{
			LLDNeedReset = 1;
			LLDNAKTime = 100;
		}
	}


#ifdef CSS
	if (LLDOnePieceFlag)
		if (!LLDReadPortOff)
			R_CtrlReg ();
#endif

	/*-------------------------------------*/
	/* Update status line for WIN95 and NT	*/
	/*												   */
	/*-------------------------------------*/
#ifdef NDIS_MINIPORT_DRIVER
	if(LLDDebugColor)
	{
		WriteStatusLine();	
	}
#endif
	
	/*----------------------------------*/
	/* Check if card needs to be reset.	*/
	/*												*/
	/*----------------------------------*/

	if (LLDNeedReset)
	{	
		LLDNeedResetCnt++;
		RxStartTime = 0;
		LLDNeedReset = CLEAR;
		if (TxFlag)
			TxFlagCleared = SET;
		TxFlag = CLEAR;
#ifndef SERIAL
		TxStartTime = 0;
#else
		for (ii = 0; ii < 128; ii++)
		{
			TxStartTime [ii] = 0;
			TxInProgress [ii] = CLEAR;
		}
#endif
		RxFlag = CLEAR;

		if (DriverNoResetFlag == CLEAR)
		{	
#ifdef CSS
			if (LLDPCMCIA)
			{	if (PCMCIACardInserted)
					LLDReset ();
			}
			else
			{	
				LLDReset ();
			}
#else
			LLDReset ();
#endif
		}

		LLDResetQ ();
	}

	else
	{	

	/*-------------------------------------------------*/
	/*	The code below puts the card to Sniff Mode when */
	/*	inactivity sec is set to less than 5 sec.			*/
	/*	The card should not go into the sniff mode if it*/
	/*	is master or it needs to respond to KeepAlive	*/
	/*-------------------------------------------------*/


		if (LLDSniffModeFlag && LLDSniffCount > 0 && !LLDMSTAFlag)
		{
			if(LLDSniffCount == FLAGSET)
			{
				if (!TxFlag && !PostponeSniff)
				{
					LLDGotoSniffMode();
					LLDSniffCount--;
				}
			}
			else
				LLDSniffCount--;

		}

	/*-------------------------------------------------*/
	/* Check the inactivity time out and see if a keep	*/
	/* alive message needs to be sent.  We send this	*/
	/* packet to prevent the card from going into		*/
	/* standby.														*/
	/*																	*/
	/* The naming of LLDSniffTime and						*/
	/* LLDInactivityTimeOut is confusing.					*/
	/*																	*/
	/* LLDSniffTime refers to the Standby Timeout		*/
	/* byte in the Set Inactivity Time-out packetized	*/
	/* command.														*/
	/*																	*/
	/* LLDInactivityTimeOut refers to the Sniff			*/
	/* Timeout byte in the Set Inactivity Time-out		*/
	/* packetized command.										*/
	/*																	*/
	/*-------------------------------------------------*/

	
		if (LLDSniffTime && LLDInactivityTimeOut)
		/*-------------------------------------------------*/
		/* Send a keep alive packet every 1/2 of the sniff	*/
		/* time interval.  Multiply LLDSniffTime by 5		*/
		/* because the sniff time is in units of 5 seconds.*/
		/*-------------------------------------------------*/
		{	
			if ((unsigned_16)(LLSGetCurrentTime () - LLDKeepAliveTmr) >= (unsigned_16)((LLDSniffTime * 5 * TPS) / 2))
			{	
				/* Send Keep Alive for all cases but PCM card not inserted. */
				if (! (LLDPCMCIA && !PCMCIACardInserted))
				{
					/*----------------------------------------------*/
					/* Don't clear the keep alive timer flag unless	*/
					/* we successfully sent the packet.  If we were	*/
					/* not successful at sending the keep alive		*/
					/* packet, we'll try again the next time we are	*/
					/* in LLDPoll ().											*/
					/*----------------------------------------------*/
					if (LLDSendKeepAlive () == SUCCESS)
					{
						LLDKeepAliveTmr = LLSGetCurrentTime ();
						LLDSniffCount = LLDTicksToSniff;
						PostponeSniff = FLAGSET;
					}
				}
				else
				{
				}
			}
		}

			/*-------------------------------------------------*/
			/* If we haven't received an IN_SYNC message when	*/
			/* the ROAMING_STATE times out, send another roam	*/
			/* command to the driver.  If we have already sent	*/
			/* 2 roam commands (one from here and the other		*/
			/* from either ROAMING_ALARM or the retry bit in	*/
			/* handle transmit complete) and we still haven't	*/
			/* received an IN_SYNC message, clear the roaming	*/
			/* state so the next roam alarm or retry bit will	*/
			/* restart the process.										*/
			/*																	*/
			/* The reason we use the timeouts and states is		*/
			/* to prevent sending a roam command for each		*/
			/* roam alarm or retry bit we receive from the FW.	*/
			/*																	*/
			/*-------------------------------------------------*/

			if (LLDRoamingState == ROAMING_STATE)
			{
				if ((unsigned_16)(LLSGetCurrentTime ()-RoamStartTime) > LLDMaxRoamWaitTime)
				{
					LLDRoam ();
					LLDRoamingState = ROAMING_DELAY;
				}
			}
			else if (LLDRoamingState == ROAMING_DELAY)
			{
				if ((unsigned_16)(LLSGetCurrentTime ()-RoamStartTime) > LLDMaxRoamWaitTime)
					LLDRoamingState = NOT_ROAMING;
			}

			/*----------------------------------------------------------------*/
			/* If we had sent a roam notify packet to the card, make sure we	*/
			/* received a response packet.  If it is missing, then something	*/
			/* is wrong with the card.														*/
			/*																						*/
			/*----------------------------------------------------------------*/
		if (LLDRoamResponse)
		{
			if ((unsigned_16)(LLSGetCurrentTime() - LLDRoamResponseTmr) >= MAXTIMEOUTTIME)
			{
				LLDNeedReset = SET;
			}
		}

	/*-------------------------------------------------*/
	/* Check to see if we have a sent a Roam Notify		*/
	/* message that has not been transmitted				*/
	/* successfully yet.  If the Roam Notify message	*/
	/* fails with a CTS or ACK error, we don't want to	*/
	/* retransmit it right away.  We want to delay for	*/
	/* 100ms to avoid the multipath problems (txing		*/
	/* on the same frequency as another master.			*/
	/*																	*/
	/*-------------------------------------------------*/
		if (LLDRoamRetryTmr && (--LLDRoamRetryTmr == 0) )
		{
			LLDTxRoamNotify (LLDMSTAAddr, LLDOldMSTAAddr, (unsigned_8 *)"PROXIM ROAMING", 14);
		}


	/*-------------------------------------------------*/
	/* Check the timed queue to see if there's any-		*/
	/* thing to flush out.										*/
	/*																	*/
	/*-------------------------------------------------*/

		LLDPullFromTimedQueue ();	


	/*-------------------------------------------------*/
	/* Check for timeout in the receive loops.			*/
	/*																	*/
	/*-------------------------------------------------*/

		if (RxFlag)
		{	
#ifndef SERIAL
			if ( (unsigned_16)(LLSGetCurrentTime () - RxStartTime) >= MAXTIMEOUTTIME)
#else
			if ( (LLSGetCurrentTime () - RxStartTime) >= serialTimeouts [SLLDBaudrateCode])
#endif
			{
				LLDRxTimeOuts++;	
				OutChar ('R', RED);
				OutChar ('x', RED);

				OutChar ('T', RED);
				OutChar ('3', RED);
				LLDNeedReset = SET;
#if INT_DEBUG
				__asm	int 3
#endif
			}
		}


	/*-------------------------------------------------*/
	/* Check for timeout in the transmit loops.			*/
	/*																	*/
	/*-------------------------------------------------*/

#ifndef SERIAL
		if (TxFlag)
		{	
			if ( (unsigned_16)(LLSGetCurrentTime () - TxStartTime) >= MAXTIMEOUTTIME)
			{	
				LLDTxTimeOuts++;
				OutChar ('T', RED);
				OutChar ('x', RED);

				OutChar ('T', RED);
				OutChar ('4', RED);

/* Do not call LLDReset if Tx failed  because of out of sync */				

				if (LLDSyncState)
					LLDNeedReset = SET;
				else
					LLDResetQ;
#if INT_DEBUG
				__asm	int 3
#endif
			}
		}
#else
		for (ii = 0; ii < 128; ii++)
		{
			if (TxInProgress [ii])
			{	
				if ( (LLSGetCurrentTime () - TxStartTime [ii]) >= 
						serialTimeouts [SLLDBaudrateCode])
				{	
					LLDTxTimeOuts++;
					OutChar ('T', YELLOW + BLINK);
					OutChar ('x', YELLOW + BLINK);
					TableEntry = LLDMapTable [ii];

					/*----------------------------------------------*/
					/* This shouldn't happen, but just in case...	*/	
					/*----------------------------------------------*/
					if (TableEntry > TOTALENTRY)
					{
						OutChar ('I', RED);
						OutChar ('n', RED);
						OutChar ('v', RED);
						OutChar ('a', RED);
						OutChar ('l', RED);
						OutChar ('i', RED);
						OutChar ('d', RED);
						OutChar ('T', RED);
						OutChar ('E', RED);
						OutHex (TableEntry, RED);
						LLDMapTable [ii] = SET;
						TxInProgress [ii] = CLEAR;
						TxStartTime [ii] = LLSGetCurrentTime ();
#if INT_DEBUG
						__asm	int 3
#endif
						continue;
					}
					OutHex (NodeTable [TableEntry].LastPacketSeqNum, MAGENTA);

					LLDMapTable [ii] = SET;
					TxInProgress [ii] = CLEAR;
					TxStartTime [ii] = LLSGetCurrentTime ();

					/*----------------------------------------------*/
					/* Reset the card if several packets in a row	*/
					/* have not received responses.  We clear the	*/
					/* flag each time we receive a transmit			*/
					/* response from the card.								*/
					/*----------------------------------------------*/
					if (++TxFlag == MAX_TX_ALLOWED)
					{
						OutChar ('T', RED);
						OutChar ('x', RED);
						LLDNeedReset = SET;
					}

					/*----------------------------------------------*/
					/* If we did not get a response back from the	*/
					/* RFNC, it probably dropped the packet due to	*/
					/* a receive error.  Clean up the node and map	*/
					/* tables.													*/
					/*----------------------------------------------*/
					else
					{
						/* Return the packet to the upper layer. */
						if (NodeTable [TableEntry].TxPkt)
						{	
							OutChar ('s', CYAN);
							OutChar ('c', CYAN);

							if (IsTxProximPacket (NodeTable [TableEntry].TxPkt))
							{	if (*((unsigned_32 *)((unsigned_8 *)NodeTable [TableEntry].TxPkt - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
								{	LLDFreeInternalTXBuffer (NodeTable [TableEntry].TxPkt);
								}
								else
								{	LLSSendProximPktComplete (NodeTable [TableEntry].TxPkt, TIMEOUT);
								}
							}
							else
							{	LLSSendComplete (NodeTable [TableEntry].TxPkt);
							}
						}
						NodeTable [TableEntry].PktInProgressFlag = CLEAR;
						NodeTable [TableEntry].TxPkt = 0;
					}
				}
			}
		}
#endif

#ifdef SERIAL

		/*-------------------------------------------------------*/
		/* Is there data waiting to be transmitted?  There are	*/
		/* two cases where we have data to transmit but we			*/
		/* cannot send data to the card.  The first is when we	*/
		/* are flow controlled.  The second is when we are			*/
		/* waiting for the card to wake up from sniff or standby	*/
		/* mode.																	*/
		/*-------------------------------------------------------*/

		if ( ((unsigned_8)_inp (LineStatReg) & THRE) && 
	  		  (uartTx.sState == TxING_DATA))
		{
			/*-------------------------------------------*/
			/* When we start a transmission, we toggle	*/
			/* RTS to wake up the RF card.  It can take	*/
			/* the card 1/2 a second to come out of		*/
			/* sniff mode or standby.  If the card is		*/
			/* awake, go ahead and start transmitting		*/
			/* the data.											*/
			/*-------------------------------------------*/

		 	status = (unsigned_8)_inp(ModemStatReg);
			if ( (status & CTS_BIT) && (status & DSR_BIT))
			{
				if (++missedTBE > TPS)
				{
					OutChar ('t', YELLOW);
					OutChar ('b', YELLOW);
					OutChar ('e', YELLOW);
					isrTBE ();
					missedTBE = 0;
				}
			}

			/*-------------------------------------------*/
			/* If we have given the card one second to	*/
			/* wake up and it is still asleep, something	*/
			/* is wrong.  Go ahead and reset the card.	*/
			/* tbeCount gets cleared each time isrTBE ()	*/
			/* is called.											*/
			/*-------------------------------------------*/
			else if (++tbeCount > TPS)
			{	
				OutChar ('N', RED);
				OutChar ('o', RED);
				OutChar ('W', RED);
				OutChar ('a', RED);
				OutChar ('k', RED);
				OutChar ('e', RED);
				OutChar ('U', RED);
				OutChar ('p', RED);
#if INT_DEBUG
				__asm	int 3
#endif
				LLDNeedReset = SET;
			}
			else
			{
			 	missedTBE = 0;
			}
		}

#endif

	}


#ifdef WBRP
	if ( LLDRepeatingWBRP )
	{
		timerticks = LLSGetCurrentTime () - WBRPStartTime;
		if ( timerticks >= WBRP_AGING_PERIOD_TICKS )
		{
			WBRPTimerTick (timerticks);
			WBRPStartTime = LLSGetCurrentTime ();
		}
	}
#endif

	InPollFlag = CLEAR;
}





/***************************************************************************/
/*																									*/
/*	LLDMulticast (OnOrOff)																	*/
/*																									*/
/*	INPUT:	OnOrOff	-	Byte to set multicast mode (0 = off, 1 = on)			*/
/*																									*/
/*	OUTPUT:	Return	-	SUCCESS or FAILURE											*/
/*																									*/
/*	DESCRIPTION:  This routine will send the Set Multicast command and		*/
/* 	wait for a timeout or a successful return.									*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDMulticast (unsigned_8 OnOrOff)

{
	SetMulticastFlag = CLEAR;
	if (LLDSetMulticast (OnOrOff))
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SetMulticastFlag));
}




/***************************************************************************/
/*																									*/
/*	LLDPromiscuous (OnOrOff)																*/
/*																									*/
/*	INPUT:	OnOrOff	-	Byte to set promiscuous mode (0 = off, 1 = on)		*/
/*																									*/
/*	OUTPUT:	Return	-	SUCCESS or FAILURE											*/
/*																									*/
/*	DESCRIPTION:  This routine will send an initialize command and				*/
/* 	wait for a timeout or a successful return.									*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDPromiscuous (unsigned_8 OnOrOff)

{
	SendInitCmdFlag = CLEAR;
	if (LLDSetPromiscuous (OnOrOff))
	{	return (FAILURE);
	}

	return (WaitFlagSet (&SendInitCmdFlag));
}




/***************************************************************************/
/*																									*/
/*	LLDStop ()																					*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Return	-	SUCCESS or FAILURE											*/
/*																									*/
/*	DESCRIPTION:  This routine will send a Goto Standby command to shut		*/
/*		the card off and will disable the card interrupt.							*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDStop (void)

{
#ifdef WBRP
	if ( LLDRepeatingWBRP )
	{
		RepeaterShutdown ();
	}
#endif

	if (LLDSendGotoStandbyCmd ())
	{	
		return (FAILURE);
	}

#if defined(CSS) && !defined(NDIS_MINIPORT_DRIVER)
	PCMCIACleanUp ();
#endif

#ifdef SERIAL
	DisableUARTInt ();
	DisableUARTModemCtrl ();

	if (RestoreBIOSTableFlag)
	{
		RestoreBIOSMem ();
	}

#endif	
	DisableRFNCInt ();

	return (SUCCESS);	
}