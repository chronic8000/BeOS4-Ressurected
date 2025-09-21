/*                                                                                                                                                                                                      */
/*                                                      PROXIM RANGELAN2 LOW LEVEL DRIVER                                                       */
/*                                                                                                                                                                                                      */
/*              PROXIM, INC. CONFIDENTIAL AND PROPRIETARY.  This source is the          */
/*              sole property of PROXIM, INC.  Reproduction or utilization of                   */
/*              this source in whole or in part is forbidden without the written                */
/*              consent of PROXIM, INC.                                                                                                                         */
/*                                                                                                                                                                                                      */
/*                                                                                                                                                                                                      */
/*                                                      (c) Copyright PROXIM, INC. 1994                                                         */
/*                                                                      All Rights Reserved                                                                             */
/*                                                                                                                                                                                                      */
/***************************************************************************/

/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      $Header:   J:\pvcs\archive\clld\lldisr.c_v   1.44   11 Aug 1998 11:32:12   BARBARA  $*/
/*                                                                                                                                                                                                      */
/* Edit History                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      $Log:   J:\pvcs\archive\clld\lldisr.c_v  $                                                                              */
/* 
/*    Rev 1.44   11 Aug 1998 11:32:12   BARBARA
/* LLSGetCurrentTime became LLSGetCurrentTimeLONG for RoamingHistory purpose.
/* 
/*    Rev 1.43   04 Jun 1998 11:10:22   BARBARA
/* Proxim protocol related changes added. The problem where the alternate master
/* was sending TxRoamNotify to itself after becoming a master was fixed.
/* 
/*    Rev 1.42   06 May 1998 13:58:48   BARBARA
/* memcpy became CopyMemory.
/* 
/*    Rev 1.41   24 Mar 1998 07:50:04   BARBARA
/* The fix to keep the card awake until packet transmitted added. Roam Notify
/* packets are not sent when in Search Continue mode.
/* 
/*    Rev 1.40   12 Sep 1997 16:18:38   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.39   06 Feb 1997 13:36:34   BARBARA
/* ReSending packet waiting on ReEntranceQ was added to the end of HandleInSync
/* 
/*    Rev 1.38   24 Jan 1997 17:35:16   BARBARA
/* Setting LLDNeedReset after antenna connect was added. Checking if API is 
/* running and not sending LLDTxRoamNotify in this case was added.
/* 
/*    Rev 1.37   15 Jan 1997 17:04:18   BARBARA
/* Handling completion of TX_ROAM_NOTIFY in ISR was added. Changes related to
/* Slow Roaming were added.
/* 
/*    Rev 1.36   11 Oct 1996 18:02:52   BARBARA
/* R_DummyReg was taken out.
/* 
/*    Rev 1.35   15 Aug 1996 09:46:34   JEAN
/* Changed the name of the function that writes to the
/* dummy CIS register.
/* 
/*    Rev 1.34   29 Jul 1996 12:17:50   JEAN
/* For the one-piece PCMCIA cards, added a function call to
/* write to a dummy register as the last register access.  This
/* prevents the standby current from drifting.
/* 
/*    Rev 1.33   14 Jun 1996 16:07:44   JEAN
/* 
/*    Rev 1.32   14 Jun 1996 16:05:40   JEAN
/* Added code to disable system interrupts before popping the
/* RFNC interrupt.
/* 
/*    Rev 1.31   12 May 1996 17:44:38   TERRYL
/* Modified the way for setting and clearing the WakeupBit. Before setting
/* the WakeupBit, save the Control and Status register values. When clearing
/* the WakeupBit, Restore these values. This is to fix a inactivity timeout
/* bug
/* 
/*    Rev 1.30   16 Apr 1996 11:14:56   JEAN
/* Added code to set the GetRFNCStatsFlag when the response for
/* a Get Stats packetized command is received.
/* 
/*    Rev 1.29   01 Apr 1996 09:25:02   JEAN
/* When we are the alternate master and we become the master, we
/* were using the incorrect master station name.
/* 
/*    Rev 1.28   29 Mar 1996 14:49:02   JEAN
/* Added master station name to roam notify print.
/* 
/*    Rev 1.27   29 Mar 1996 11:27:22   JEAN
/* Added a comment.
/* 
/*    Rev 1.26   15 Mar 1996 13:51:50   TERRYL
/* Added Peer to Peer in HandleInSync()
/* 
/*    Rev 1.25   14 Mar 1996 14:40:28   JEAN
/* Changed some debug color prints.
/* 
/*    Rev 1.24   13 Mar 1996 17:48:38   JEAN
/* Added some debug code and changed the sanity checks.
/* 
/*    Rev 1.23   12 Mar 1996 10:34:38   JEAN
/* Added support for two new packetized commands, Disable/Enable
/* EEPROM Write.
/* 
/*    Rev 1.22   08 Mar 1996 18:53:42   JEAN
/* Added a flag to indicate if we are in our ISR, added checks
/* to validate rx packet lengths.
/* 
/*    Rev 1.21   04 Mar 1996 15:02:30   truong
/* Added support for InDriverISRFlag.
/* 
/*    Rev 1.20   06 Feb 1996 14:16:18   JEAN
/* Merged the serial and non-serial LLDIsr () code.
/* 
/*    Rev 1.19   31 Jan 1996 12:51:28   JEAN
/* For serial, changed a function call to LLSRoamNotfity() to
/* LLDTxRoamNotifiy(), fixed a debug color print for 'C' functions,
/* changed some constants to macros, and changed the order of
/* some header include files.
/* 
/*    Rev 1.18   19 Dec 1995 18:03:32   JEAN
/* Removed a type casting from LLDLookAheadBuf.
/* 
/*    Rev 1.17   14 Dec 1995 15:06:24   JEAN
/* Added some comments.
/* 
/*    Rev 1.16   17 Nov 1995 16:32:34   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.15   19 Oct 1995 15:27:14   JEAN
/* Added a #ifdef around an extern that is only used for WBPR.
/* 
/*    Rev 1.14   12 Oct 1995 11:28:52   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* 
/*    Rev 1.13   24 Jul 1995 18:14:04   truong
/* 
/*    Rev 1.12   25 Jun 1995 09:13:52   tracie
/* If serial driver, ignore the Attena response packet
/* 
/*    Rev 1.11   20 Jun 1995 15:47:18   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.10   13 Apr 1995 08:47:00   tracie
/* XROMTEST version
/* 
/*    Rev 1.9   16 Mar 1995 16:10:24   tracie
/* Added support for ODI
/* 
/*    Rev 1.8   09 Mar 1995 15:06:30   hilton
/* 
/*    Rev 1.7   22 Feb 1995 10:28:32   tracie
/* Added WBRP functions
/* 
/*    Rev 1.6   05 Jan 1995 09:52:38   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.5   27 Dec 1994 15:42:52   tracie
/*                                                                                                                                                                                              */
/*    Rev 1.2   22 Sep 1994 10:56:08   hilton                                                                           */
/*                                                                                                                                                                                              */
/*    Rev 1.1   20 Sep 1994 16:02:54   hilton                                                                           */
/*                                                                                                                                                                                              */
/*    Rev 1.0   20 Sep 1994 10:59:52   hilton                                                                           */
/* Initial revision.                                                                                                                                                    */
/*                                                                                                                                                                                                      */
/*                                                                                                                                                                                                      */
/***************************************************************************/

#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#endif
#include <string.h>
#include        "constant.h"
#include "lld.h"
#include "blld.h"
#include "bstruct.h"
#include	"pstruct.h"
#include	"asm.h"
#include "lldisr.h"
#include "lldpack.h"
#include "pcmcia.h"
#include "port.h"
#ifndef SERIAL
#include "port.h"
#include "hardware.h"
#else
#include "slldbuf.h"
#include "slld.h"
#include "slldhw.h"
#endif
#include "lldrx.h"
#include "lldsend.h"
#include "lldqueue.h"
#ifdef WBRP
extern void LLSUpdateSyncAddr (void);
#endif

#ifdef MULTI
#include        "multi.h"

extern struct LLDVarsStruct     *LLDVars;

#else
unsigned_8			LLDLookAheadBuf [BUFFERLEN];
unsigned_16			LLDLookAheadAddOn;
unsigned_8			InDriverISRFlag;
unsigned_8			LLDRoamingState;
unsigned_16			RoamStartTime;
unsigned_16			LLDLastRoam;
unsigned_8			LLDMSTAFlag;
unsigned_8			LLDSleepFlag;

#ifdef NDIS_MINIPORT_DRIVER
extern unsigned_8               API_FlagLLD;
#endif

extern unsigned_8               LLDSyncState;

extern unsigned_16		LLDLookAheadSize;
extern unsigned_8			LLDTransferMode;
extern unsigned_8			LLDMSTAName [NAMELENGTH+1];
extern unsigned_8			LLDMSTAAddr [ADDRESSLENGTH];
extern unsigned_8			LLDOldMSTAAddr [ADDRESSLENGTH];
extern unsigned_8			LLDROMVersion [ROMVERLENGTH];
extern unsigned_8			LLDIntLine1;
extern unsigned_8			LLDNodeType;
extern unsigned_8			LLDRoamingFlag;
extern unsigned_8			LLDMSTASyncName [NAMELENGTH+1];
extern unsigned_8			LLDMSTASyncSubChannel;
extern unsigned_8			LLDMSTASyncChannel;
extern unsigned_8               LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_8               LLDPhysAddress [ADDRESSLENGTH];
extern unsigned_8               LLDPCMCIA;
extern unsigned_8               TxFlagCleared;
extern unsigned_8               TxFlag;

extern unsigned_8               SendInitCmdFlag;
extern unsigned_8               SetSecurityIDFlag;
extern unsigned_8               GetGlobalAddrFlag;
extern unsigned_8               GetROMVersionFlag;
extern unsigned_8               SetITOFlag;
extern unsigned_8               DisableHopFlag;
extern unsigned_8               ConfigureMACFlag;
extern unsigned_8               SetRoamingFlag;
extern unsigned_8               DisableEEPROMFlag;
extern unsigned_8               EnableEEPROMFlag;
extern unsigned_8               GetRFNCStatsFlag;
extern unsigned_8               SetMulticastFlag;
extern unsigned_8               LLDNumMastersFound;
extern unsigned_8               LLDSearchTime;
extern unsigned_8               SendSearchContinueFlag;
extern unsigned_8               SendAbortSearchFlag;
extern unsigned_8               NextMaster;
extern struct MastersList LLDMastersFound[MAXROAMHISTMSTRS];
							
extern unsigned_16      RawLookAheadLength;
extern unsigned_16      LookAheadLength;
extern unsigned_8               *LLDLookAheadBufPtr;

extern unsigned_8               LLDOutOfSyncCount;
extern unsigned_32      LLDZeroRxLen;
extern unsigned_32      LLDTooBigRxLen;

extern unsigned_8               LLDPeerToPeerFlag;

extern unsigned_8               ControlValue;
extern unsigned_8               StatusValue;
extern unsigned_8               LLDSyncAlarm;
extern unsigned_8               LLDRoamRetryTmr;
extern unsigned_8               LLDRoamResponse;
extern unsigned_8               LLDNeedReset;
extern unsigned_16      LLDRoamResponseTmr;
extern unsigned_8               PostponeSniff;
extern unsigned_8               InSearchContinueMode;

extern struct LLDLinkedList     FreeReEntrantQ;
extern struct LLDLinkedList     ReEntrantQ;

#endif



/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      LLDIsr ()                                                                                                                                                                       */
/*                                                                                                                                                                                                      */
/*      INPUT:                                                                                                                                                                          */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION: Interrupt service routine, will receive packets from                       */
/* the RFNC and process them according to packet type.                                                  */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void LLDIsr (void)

{       
	unsigned_16             ReadinLength;
	unsigned_16             RawLength;
	unsigned_8              FuncNum;
	unsigned_8              PRCmd;
	unsigned_8              Color;
#ifndef SERIAL
	unsigned_8              SaveControlValue;
	unsigned_8              SaveStatusValue;
#endif

	OutChar ('I', RED + WHITE_BACK);

	if (InDriverISRFlag)
	{
		OutChar ('I', YELLOW);
		OutChar ('s', YELLOW);
		OutChar ('r', YELLOW);
		OutChar ('R', YELLOW);
		OutChar ('e', YELLOW);
		OutChar ('E', YELLOW);
		OutChar ('n', YELLOW);
		OutChar ('t', YELLOW);
		return;
	}

	InDriverISRFlag = SET;

	/*-------------------------------------------*/
	/*                                              SERIAL                                                  */
	/*                                                                                                                      */
	/* We get called by SLLDIsr() after each                */
	/* packet is received and the checksum is       */
	/* validated.  There is only one receive                */
	/* buffer for serial packets, so we must                */
	/* empty out this buffer before returning               */
	/* from the interrupt.  We will copy the                */
	/* data from the uartRx buffer into the         */
	/* LLDLookAheadBuf.                                                                     */
	/*                                                                                                                      */
	/*              ISA/PCMCIA/OEM                                  */
	/*															*/
	/* We get called as a new packet arrives.		*/
	/* We have not yet read in any bytes from		*/
	/* the RFNC.  We read in the bytes as they	*/
	/* are needed into the LLDLookAheadBuf.		*/
	/*															*/
	/*-------------------------------------------*/


	/*-------------------------------------------*/
	/* SLLDIsr has already disabled interrupts	*/
	/* and EOIed the PIC.								*/
	/*-------------------------------------------*/
#ifndef SERIAL
	PushRFNCInt ();
	DisableRFNCInt ();

#ifdef CSS
	if (LLDPCMCIA)
		ClearPCMCIAInt ();
#endif

	EOI (LLDIntLine1);

	/*----------------------------------------*/
	/* Make sure the card is awake so we can        */
	/* reliably read from it's registers.           */
	/*----------------------------------------*/
	SaveControlValue = ControlValue;
	SaveStatusValue = StatusValue;
	SetWakeupBit ();

#endif

	/*--------------------------------------------*/
	/* Call RxRequest to get the packet length.   */
	/* RawLength is the length of the entire      */
	/* packet, including the packetize header.       */
	/*--------------------------------------------*/
	RawLength = RxRequest (LLDTransferMode);
	if ((RawLength > MAX_PROX_PKT_SIZE) || (RawLength < MIN_PROX_PKT_SIZE))
	{       
		if (RawLength < MIN_PROX_PKT_SIZE)
			LLDZeroRxLen++;
		else
			LLDTooBigRxLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(RawLength >> 8), RED);
		OutHex ((unsigned_8)(RawLength), RED);
		EndOfRx ();
		InDriverISRFlag = CLEAR;
		DisableInterrupt ();
#ifndef SERIAL
		PopRFNCInt ();
		if (TxFlag)
		{       SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
#if INT_DEBUG
		__asm   int 3
#endif
		return;
	}

	/*--------------------------------------------*/
	/* Reset the look ahead buffer pointer for    */
	/* each new packet.  This pointer is used by  */
	/* both the LLDReceive routine and the        */
	/* LLDRxFragments routine.                   */
	/*--------------------------------------------*/
	LLDLookAheadBufPtr = LLDLookAheadBuf;

#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		ReadinLength = RawLength;
		RawLookAheadLength = RawLength;
		LookAheadLength = RawLength - sizeof (struct PacketizeTx);
	}
#else
	/*-------------------------------------------------------*/
	/* LLDLookAheadSize + LLDLookAheadAddOn = 18 + 52 = 70  */
	/* if (entire packet length < 18+52)                                                    */
	/* then                                                                                                                                 */
	/*          only read in the entire packet length (RawLength) */
	/* 18 : LLD LookAhead size set by upper layer.                             */
	/* 52 : LLD LookAhead Add On. (includes packetize header */
	/*                                                                                      and Ethernet stuff.)            */
	/*-------------------------------------------------------*/
	if (RawLength <= (LLDLookAheadSize + LLDLookAheadAddOn))
	{       ReadinLength = RawLength;
		RawLookAheadLength = RawLength;
		LookAheadLength = RawLength - sizeof (struct PacketizeTx);
	}
	else
	{       /*-------------------------------------------------*/
		/* if the entire packet length > 18+52 then                     */
		/* only read in (18+52=70) bytes.                                               */
		/*-------------------------------------------------*/
		ReadinLength = LLDLookAheadSize + LLDLookAheadAddOn;
		RawLookAheadLength = ReadinLength;
		LookAheadLength = ReadinLength - sizeof (struct PacketizeTx);
	}
#endif


	if (RxData (LLDLookAheadBuf, ReadinLength, LLDTransferMode))
	{
      EndOfRx ();
		InDriverISRFlag = CLEAR;
		DisableInterrupt ();
#ifndef SERIAL
		PopRFNCInt ();
		if (TxFlag)
		{       SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
		return;
	}

	if (((struct PRspHdr *)LLDLookAheadBuf)->PRSeqNo & RAWMASK)
	{
		FuncNum = ((struct PRspHdr *)LLDLookAheadBuf)->PRFnNo;
		PRCmd = ((struct PRspHdr *)LLDLookAheadBuf)->PRCmd;
		if (FuncNum < 10)
		{       OutChar (((struct PRspHdr *)LLDLookAheadBuf)->PRCmd, GREEN);
			OutChar ((unsigned_8)(FuncNum + '0'), GREEN);
		}

		else if (FuncNum < 20)
		{       if (PRCmd == 'a')
			{       if ((FuncNum == SET_ROAMING_FN) || (FuncNum == ROAM_FN))
					Color = YELLOW;
				else
					Color = GREEN;
			}
			else
				Color = GREEN;

			OutChar (PRCmd, Color);
			OutChar ('1', Color);
			OutChar ((unsigned_8)(FuncNum - 10 + '0'), Color);
		}

		else
		{       OutChar (PRCmd, GREEN);
			OutChar ('2', GREEN);
			OutChar ((unsigned_8)(FuncNum - 20 + '0'), GREEN);
		}

		LLSRawReceiveLookAhead (LLDLookAheadBuf, RawLength);
		EndOfRx ();
		InDriverISRFlag = CLEAR;
		DisableInterrupt ();
#ifndef SERIAL
		PopRFNCInt ();
		if (TxFlag)
		{       SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
		return;
	}

	/*------------------------------------------------------------*/
	/* For packets that have the repeat sequence number, we don't */
	/* want to pass it up to the caller.  This kind of packet may */
	/* be called from the LLD itself, such as the WBRP packets.   */
	/*------------------------------------------------------------*/
	else if (((struct PRspHdr *)LLDLookAheadBuf)->PRSeqNo == REPEAT_SEQ_NUMBER)
	{
		OutChar ( 'R', GREEN );
		OutChar ( 'D', GREEN );
		OutChar ( 'O', GREEN );
		OutChar ( 'N', GREEN );
		OutChar ( 'E', GREEN );

		EndOfRx ();
		InDriverISRFlag = CLEAR;
		DisableInterrupt ();
#ifndef SERIAL
		PopRFNCInt ();
		if (TxFlag)
		{       SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
		return;
	}

	else if (((struct PRspHdr *)LLDLookAheadBuf)->PRSeqNo == ROAM_NOTIFY_SEQNO)
	{
	/*-------------------------------------------------------*/
	/* We received the response, so clear the flag used to  */
	/* detect missing roam notify response messages.                        */
	/*                                                                                                                                                      */
	/*-------------------------------------------------------*/

		LLDRoamResponse = CLEAR;
		LLDRoamResponseTmr = 0;

		/*-------------------------------------------------------------*/
		/* When the TX_STATUS_OK bit is set, the packet successfully    */
		/* transmitted in either QFSK or BFSK mode.  Was there an ACK   */
		/* or CTS error on the Roam Notify response?                                                    */
		/*                                                                                                                                                                      */
		/*-------------------------------------------------------------*/
		if ( ! (((struct PacketizeTxComplete *)LLDLookAheadBuf)->PTCStatus
					& TX_STATUS_OK) )
		{
				/*----------------------------------------------------------*/
				/* We set a timer so that LLDPoll can call LLDTxRoamNotify.     */
				/* We don't want to call it from here in case we are in a       */
				/* multipath environment.  We want to wait 1/2 of a hop         */
				/* sequence.                                                                                                                            */
				/*                                                                                                                                                              */
				/*----------------------------------------------------------*/
				LLDRoamRetryTmr = ROAMNOTIFYWAIT;
		}
		else
		{
			LLDRoamRetryTmr = 0;
		}

		EndOfRx ();

		InDriverISRFlag = CLEAR;
		DisableInterrupt ();
#ifndef SERIAL
		PopRFNCInt ();
		if (TxFlag)
		{       SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
		return;

	}


	switch (((struct PRspHdr *)LLDLookAheadBuf)->PRCmd)
	{       
		case INIT_COMMANDS_RSP:
			HandleInitCmdRsp (LLDLookAheadBuf);
			break;

		case DATA_COMMANDS_RSP:
			HandleDataCmdRsp (LLDLookAheadBuf, RawLength);
			break;

		case DIAG_COMMANDS_RSP:
			HandleDiagCmdRsp (LLDLookAheadBuf);
			break;
	}

	EndOfRx ();
	InDriverISRFlag = CLEAR;
	DisableInterrupt ();
#ifndef SERIAL
	PopRFNCInt ();
	if (TxFlag)
	{       SaveControlValue |= WAKEUPBIT;
		SaveStatusValue |= STAYAWAKE;
		OutChar ('T', WHITE);
	}
	/* The code below is executed when this ISR is send complete. */
	else if (TxFlagCleared)
	{       
	
		OutChar ('X', WHITE);
		TxFlagCleared = CLEAR;
		SaveControlValue &= ~WAKEUPBIT;
		SaveStatusValue &= ~STAYAWAKE;
	}
	ClearWakeupBit (SaveControlValue, SaveStatusValue);
#endif
}




/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      HandleInitCmdRsp (RspPktPtr)                                                                                                                    */
/*                                                                                                                                                                                                      */
/*      INPUT:  RspPktPtr               -       Pointer to the response packet                                          */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION: This routine will handle responses for commands                            */
/*      from the INIT commands group.                                                                                                           */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void HandleInitCmdRsp (unsigned_8 *RspPktPtr)

{       unsigned_8      CmdResp;
	unsigned_8      Color;
	unsigned_8      ii;
	unsigned_8      zeroaddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	CmdResp = ((struct PRspHdr *)RspPktPtr)->PRFnNo;

	if (CmdResp < 10)
	{       OutChar ('a', GREEN);
		OutChar ((unsigned_8)(CmdResp + '0'), GREEN);
	}

	else if (CmdResp < 20)
	{       if ((CmdResp == SET_ROAMING_FN) || ((CmdResp == ROAM_FN) || (CmdResp == ROAMING_ALARM_FN)))
			Color = RED;
		else
			Color = GREEN;

		OutChar ('a', Color);
		OutChar ('1', Color);
		OutChar ((unsigned_8)(CmdResp - 10 + '0'), Color);
	}

	else
	{       if (CmdResp == ANTENNA_FN)
			Color = RED + BLUE_BACK;
		else
			Color = GREEN;

		OutChar ('a', Color);
		OutChar ('2', Color);
		OutChar ((unsigned_8)(CmdResp - 20 + '0'), Color);
	}

	switch (CmdResp)
	{
		case INITIALIZE_FN:
			if (((struct PRspInitCmd *)RspPktPtr)->PRICStatus == 0)
			{       SendInitCmdFlag = SET;
			}
			break;

		case SEARCH_N_SYNC_FN:
			break;

		case SEARCH_N_CONT_FN:
			SendSearchContinueFlag = SET;
			break;

		case ABORT_SEARCH_FN:
			SendAbortSearchFlag = SET;
			break;

		case IN_SYNC_FN:
			if(LLDSearchTime)
				HandleInSyncMSTASearch((struct PacketizeInSync *) RspPktPtr);
			else
				HandleInSync ((struct PacketizeInSync *) RspPktPtr);
			break;

		case OUT_OF_SYNC_FN:
			LLDSyncState = CLEAR;
			if ((NextMaster == 0) ||
				 (memcmp (RoamHistory[NextMaster-1].MasterAddress, zeroaddress, ADDRESSLENGTH)))
			{
				if (NextMaster >= MAXROAMHISTMSTRS)
				{       /*-------------------------------------*/
					/* roaming history master list if full  */
					/* so remove the oldest record and              */
					/* advance the records in the list              */
					/*-------------------------------------*/
					NextMaster--;
					for (ii = 0; ii < NextMaster; ii++)
						RoamHistory[ii] = RoamHistory[ii + 1];
				}
				/*-------------------------------------*/
				/* get the current system time as the   */
				/* time the node get out of sync			*/
				/*-------------------------------------*/
				RoamHistory[NextMaster].Time = LLSGetCurrentTimeLONG();
				/*----------------------------------------*/
				/* set the master name and address in the       */
				/* roaming history structure to all 0's */
				/*----------------------------------------*/
				SetMemory (RoamHistory[NextMaster].MasterName, 0, NAMELENGTH);
				SetMemory (RoamHistory[NextMaster].MasterAddress, 0, ADDRESSLENGTH);
				NextMaster++;
			}
#ifdef WBRP
			if (LLDRepeatingWBRP)
			{
				SetMemory (LLDSyncToNodeAddr, 0, ADDRESSLENGTH);
				LLDSyncDepth = (unsigned_8) 0;
				LLSUpdateSyncAddr ();
			}
#endif

			break;

		case    GOTO_STANDBY_FN:
			break;

		case SET_ITO_FN:
			SetITOFlag = SET;
			break;

		case KEEP_ALIVE_FN:
			PostponeSniff = CLEAR;
			break;

		case SET_MULTICAST_FN:
			SetMulticastFlag = SET;
			break;

		case RESET_STATS_FN:
			break;

		case GET_STATS_FN:
			GetRFNCStatsFlag = SET;
			break;

		case SET_SECURITY_ID_FN:
			SetSecurityIDFlag = SET;
			break;

		case GET_ROM_VER_FN:
			CopyMemory (LLDROMVersion, ((struct PRspGetROMVer *)RspPktPtr)->PRGRVVersion, ADDRESSLENGTH);
			/* Null terminate the version length string. */
			LLDROMVersion [ROMVERLENGTH-1] = 0;
			GetROMVersionFlag = SET;
			break;

		case GET_ADDRESS_FN:
			GetGlobalAddrFlag = SET;
			CopyMemory (LLDPhysAddress, ((struct PRspGetAddr *)RspPktPtr)->PRGAAddress, ADDRESSLENGTH);
			break;

		case GET_GPIN0_FN:
			break;

		case CONFIG_MAC_FN:
			ConfigureMACFlag = SET;
			break;

		case SET_ROAMING_FN:
			SetRoamingFlag = SET;
			break;

		case ROAMING_ALARM_FN:
			if ((LLDRoamingState == NOT_ROAMING) && (LLDRoamingFlag == ROAMING_ENABLE))
			{       
				LLDRoam ();
				LLDRoamingState = ROAMING_STATE;
			}
			break;

		case ROAM_FN:
			break;

		case ANTENNA_FN:
#ifndef SERIAL
			if (((struct PRspAntenna *) RspPktPtr)->PAStatus)
			{       Beep (1500);
				Delay (84);
				Beep (1500);
				Delay (42);
				Beep (1500);
				LLDNeedReset = 1;
			}
			else
			{       Beep (1200);
			}
#endif
			break;

		case GET_COUNTRY_FN:
			break;

		/*----------------------------------------------*/
		/* The following commands are used by Armadillo */
		/* project only.                                                                                        */
		/*----------------------------------------------*/
		case GET_FREQUENCY_FN:
/*                      CopyMemory (LLDFreqTable, 
						((struct PRspGetFreqTable *)RspPktPtr)->PGFTFreqTbl,
																	0x35);
*/
			break;

		case SET_FREQUENCY_FN:
			break;

		case SET_HOPSTAT_FN:
			break;

		case HOP_STATISTICS_FN:
			HandleHopStatistics ((struct PacketizeHopStat *)RspPktPtr);
			break;

		case SET_UART_BAUDRATE_FN:
			break;

		case DISABLE_EEPROM_FN:
			DisableEEPROMFlag = SET;
			break;

		case ENABLE_EEPROM_FN:
			EnableEEPROMFlag = SET;
			break;

	}
}



/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      HandleDataCmdRsp (RspPktPtr, RawLength)                                                                                 */
/*                                                                                                                                                                                                      */
/*      INPUT:  RspPktPtr       -       Pointer to the response packet                                          */
/*                              RawLength       -       Length of the packet including headers                          */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION: This routine will handle responses for commands from                       */
/*      the DATA commands group.                                                                                                                        */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void HandleDataCmdRsp (unsigned_8 *RspPktPtr, unsigned_16 RawLength)

{       unsigned_16             CmdResp;

	OutChar ('b', GREEN);
	CmdResp = ((struct PRspHdr *)RspPktPtr)->PRFnNo;
	OutChar ((unsigned_8)(CmdResp + '0'), GREEN);

	switch (CmdResp)
	{       case TRANSMIT_DATA_FN:
			if (((struct PRspHdr *)RspPktPtr)->PRSeqNo != REPEAT_SEQ_NUMBER)
			{       
				OutHex (((struct PRspHdr *)RspPktPtr)->PRSeqNo, MAGENTA);
				HandleTransmitComplete ((struct PacketizeTxComplete *) RspPktPtr);
			}
			break;

		case RECEIVE_DATA_FN:
			HandleDataReceive ((struct PRspData *) RspPktPtr);
			break;

		case PING_DATA_FN:
			OutHex (((struct PRspHdr *)RspPktPtr)->PRSeqNo, MAGENTA);
			break;

		case PING_RESPONSE_FN:
			OutHex (((struct PRspHdr *)RspPktPtr)->PRSeqNo, MAGENTA);
			LLSRawReceiveLookAhead (LLDLookAheadBuf, RawLength);
			break;

#ifdef PROTOCOL
		case RECEIVE_PROT_FN:
			HandleProtReceive ((struct PRspProt *) RspPktPtr);
			break;;
#endif

		default:
			break;
	}
}



/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      HandleDiagCmdRsp (RspPktPtr)                                                                                                                    */
/*                                                                                                                                                                                                      */
/*      INPUT:  RspPktPtr       -       Pointer to the response packet                                          */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION:  This routine will handle responses for commands from              */
/*              the DIAG commands group.                                                                                                                        */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void HandleDiagCmdRsp (unsigned_8 *RspPktPtr)

{       unsigned_8      CmdResp;

	OutChar ('c', GREEN);
	CmdResp = ((struct PRspHdr *)RspPktPtr)->PRFnNo;
	if (CmdResp < 10)
		OutChar ((unsigned_8)(CmdResp + '0'), GREEN);
	else
	{
		OutChar ('1', GREEN);
		OutChar ((unsigned_8)(CmdResp - 10 + '0'), GREEN);
	}


	switch (CmdResp)
	{
		case SET_GLOBAL_FN:
			break;

		case PEEK_EEPROM_FN:
			break;

		case POKE_EEPROM_FN:
			break;

		case PEEK_MEMORY_FN:
			break;

		case POKE_MEMORY_FN:
			break;

		case HB_TIMER_FN:
			break;

		case ROM_CHECK_FN:
			break;

		case RADIO_MODE_FN:
			break;

		case ECHO_PACKET_FN:
			break;

		case DISABLE_HOP_FN:
			DisableHopFlag = SET;
			break;

		case GET_RSSI_FN:
			break;

		case SET_COUNTRY_FN:
			break;

		case SET_MAX_SYNCDEPTH_FN:
			break;
	}
}

/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      HandleInSyncMSTASearch (InSyncMsg)                                                                                                                              */
/*                                                                                                                                                                                                      */
/*      INPUT:  InSyncMsg       -       Pointer to the In sync message.                                         */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION:  This procedure will handle the in sync message, saving            */
/*              the current master address, name, and channel/subchannel in the array*/
/*              sorted in the increasing order by the channel/subchannel.                               */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void HandleInSyncMSTASearch (struct PacketizeInSync *InSyncMsg)

{
	unsigned_8      ii;
	unsigned_8      iii = 0;

	OutHex ((unsigned_8)(InSyncMsg->PISChSub), BROWN);


	/*-------------------------------------------------*/
	/*      Check if the information about the master                       */
	/*      had have been already entered into the array.   */
	/*                                                                                                                                      */
	/*-------------------------------------------------*/

	for (ii = 0; ii < LLDNumMastersFound; ii++)
	{       if (LLDMastersFound [ii].ChSubChannel == InSyncMsg->PISChSub)
			break;
	}


	/*----------------------------------------------------------------*/
	/*      Check if the information about the master       is already in the               */
	/*      array. Exit if yes or if the array is full.  Watch out for the  */
	/*      case when it is the first aray element and the PISChSub is 0.   */
	/*                                                                                                                                                                              */
	/*----------------------------------------------------------------*/

	if (((LLDMastersFound [ii].ChSubChannel == InSyncMsg->PISChSub) 
						&& LLDNumMastersFound != 0) || (ii == (MAXMASTERS - 1) ))
	{
		return;
	}


	/*----------------------------------------------------------------*/
	/*      Enter the information about the master, in the increassing              */
	/*      order by the channel/subchannel, into the array.                                        */
	/*                                                                                                                                                                              */
	/*----------------------------------------------------------------*/

	ii = 0;
	while (ii < LLDNumMastersFound && ii < MAXMASTERS && iii != SET)
	{       
		if (LLDMastersFound [ii].ChSubChannel > InSyncMsg->PISChSub)
		{
			for(iii = LLDNumMastersFound; iii > ii; iii-- )
			{       LLDMastersFound[iii].ChSubChannel = LLDMastersFound[iii-1].ChSubChannel;
				CopyMemory (LLDMastersFound [iii].Name, LLDMastersFound[iii-1].Name, 11);
				CopyMemory (LLDMastersFound [iii].Address, LLDMastersFound[iii-1].Address, 6);
			}
			iii = SET;
			ii--;
		}
		ii++;
	}

	LLDMastersFound [ii].ChSubChannel = InSyncMsg->PISChSub;
	CopyMemory (LLDMastersFound [ii].Name, InSyncMsg->PISSyncName, 11);
	CopyMemory (LLDMastersFound [ii].Address, InSyncMsg->PISSyncAddr, 6);
	LLDNumMastersFound++;

}

/*
	if ((LLDMastersFound [ii].ChSubChannel != InSyncMsg->PISChSub) && (ii < (MAXMASTERS - 1)))
	{       LLDMastersFound [LLDNumMastersFound].ChSubChannel = InSyncMsg->PISChSub;
		CopyMemory (LLDMastersFound [LLDNumMastersFound].Name, InSyncMsg->PISSyncName, 11);
		CopyMemory (LLDMastersFound [LLDNumMastersFound].Address, InSyncMsg->PISSyncAddr, 6);
		LLDNumMastersFound++;
	}
*/

/***************************************************************************/
/*                                                                                                                                                                                                      */
/*      HandleInSync (InSyncMsg)                                                                                                                                */
/*                                                                                                                                                                                                      */
/*      INPUT:  InSyncMsg       -       Pointer to the In sync message.                                         */
/*                                                                                                                                                                                                      */
/*      OUTPUT:                                                                                                                                                                         */
/*                                                                                                                                                                                                      */
/*      DESCRIPTION:  This procedure will handle the in sync message, updating  */
/*              the current master address, name, and channel/subchannel.  It will      */
/*              also check for a roam.                                                                                                                          */
/*                                                                                                                                                                                                      */
/***************************************************************************/

void HandleInSync (struct PacketizeInSync *InSyncMsg)

{
	unsigned_8      ii;
	struct ReEntrantQueue *Qptr;
	unsigned_8      Status;

	/*-------------------------------------------------*/
	/* Now that we are in sync, we need to reset the        */
	/* roaming state.                                                                                               */
	/*                                                                                                                                      */
	/* If we are in a multipath environment and slow        */
	/* roaming has been selected, set the state to          */
	/* ROAMING_DELAY and the roaming time to the                    */
	/* current time.  By doing this, all roaming                    */
	/* alarms and retry bits are ignored until                      */
	/* MAXSLOWROAMWAITTMIE seconds has elapsed.                     */
	/*                                                                                                                                      */
	/* For normal roaming, if the driver receives a         */
	/* roaming alarm immediately after an IN_SYNC, we       */
	/* want to roam right away.                                                             */
	/*                                                                                                                                      */
	/*-------------------------------------------------*/

	if (LLDSyncAlarm == 0x23 )
	{
		RoamStartTime = LLSGetCurrentTime ();
		LLDRoamingState = ROAMING_DELAY;
	}
	else
	{
		RoamStartTime = 0;
		LLDRoamingState = NOT_ROAMING;
	}

	/*-------------------------------------------------*/
	/* Check if roamed.  Masters will not roam.                     */
	/*                                                                                                                                      */
	/*-------------------------------------------------*/

	if (LLDNodeType != MASTER)
	{       
  		if ((NextMaster == 0) ||
			 (memcmp (RoamHistory[NextMaster-1].MasterAddress,
							InSyncMsg->PISSyncAddr, ADDRESSLENGTH) != 0))
		{
			if (NextMaster >= MAXROAMHISTMSTRS)
			{	/*-------------------------------------*/
				/* roaming history master list is full	*/
				/* as remove the oldest record and		*/
				/* advance the records in the list		*/
				/*-------------------------------------*/
				NextMaster--;
				for (ii = 0; ii < NextMaster; ii++)
					RoamHistory[ii] = RoamHistory[ii + 1];
			}
			/*-------------------------------------------*/
			/* get the current system time as the time      */
			/* the node get in-sync with the new master     */
			/*-------------------------------------------*/
			RoamHistory[NextMaster].Time = LLSGetCurrentTimeLONG ();
			/*----------------------------------------*/
			/* store the new master name and address        */
			/* into the roaming history list                                */
			/*----------------------------------------*/
			CopyMemory (RoamHistory[NextMaster].MasterName,
							InSyncMsg->PISSyncName, NAMELENGTH);
			CopyMemory (RoamHistory[NextMaster].MasterAddress,
							InSyncMsg->PISSyncAddr, ADDRESSLENGTH);
			NextMaster++;
		}
	
		if (memcmp (InSyncMsg->PISSyncAddr, LLDMSTAAddr, ADDRESSLENGTH))
		{  OutChar ('R', YELLOW);
			OutChar ('o', YELLOW);
			OutChar ('a', YELLOW);
			OutChar ('m', YELLOW);
			OutChar ('e', YELLOW);
			OutChar ('d', YELLOW);

			ii = 0;
			while (InSyncMsg->PISSyncName [ii] && (ii < NAMELENGTH+1))
			{       OutChar (InSyncMsg->PISSyncName [ii++], YELLOW);
			}


			/*---------------------------------*/
			/* Roaming Flag is reversed logic  */
			/*                                                                                        */
			/*---------------------------------*/

			LLDRoamRetryTmr = 0;
#ifdef NDIS_MINIPORT_DRIVER
			if (!(API_FlagLLD))
#endif          
			/*------------------------------------------------*/
			/* Do not send RoamNotity if you are searching    */
			/* or if you are the Master now.                  */
			/*                                                */
			/*------------------------------------------------*/

				if (!InSearchContinueMode)
					if (memcmp (InSyncMsg->PISSyncAddr, LLDNodeAddress, ADDRESSLENGTH) != 0)
						LLDTxRoamNotify (InSyncMsg->PISSyncAddr, LLDMSTAAddr, (unsigned_8 *)"PROXIM ROAMING", 14);

			if (LLDPeerToPeerFlag)                  /* if in client server environment */
				LLDSetupAutoBridging();
		}
	}


	/*-------------------------------------------------*/
	/* Update the name, address, channel, subchannel.       */
	/* Did we sync to ourselves?  If so, set the                    */
	/* master sync name to our master name.  If we are      */
	/* an alternate master and we lose sync from the        */
	/* true master, the insync message gives us the         */
	/* incorrect master name.  Instead of giving us         */
	/* our name, it gives us the name of the old                    */
	/* master.                                                                                                              */
	/*                                                                                                                                      */
	/*-------------------------------------------------*/

	CopyMemory (LLDOldMSTAAddr, LLDMSTAAddr, ADDRESSLENGTH);
	if (memcmp (InSyncMsg->PISSyncAddr, LLDNodeAddress, ADDRESSLENGTH) == 0)
		CopyMemory (LLDMSTASyncName, LLDMSTAName, NAMELENGTH);
	else
		CopyMemory (LLDMSTASyncName, InSyncMsg->PISSyncName, NAMELENGTH);

	CopyMemory (LLDMSTAAddr, InSyncMsg->PISSyncAddr, ADDRESSLENGTH);

	LLDMSTASyncSubChannel = (unsigned_8)(InSyncMsg->PISChSub & 0x0F);
	LLDMSTASyncChannel = (unsigned_8)(InSyncMsg->PISChSub >> 4);

	LLDSyncState = SET;
	LLDOutOfSyncCount = 0;

	/*-------------------------------------------------*/
	/* Set flag to indicate if Master or not.                               */
	/*                                                                                                                                      */
	/*-------------------------------------------------*/

	if (LLDNodeType == MASTER)
	{       LLDMSTAFlag = SET;
	}
	else
	{       if (memcmp (LLDNodeAddress, LLDMSTAAddr, ADDRESSLENGTH))
		{       LLDMSTAFlag = CLEAR;
		}
		else
		{       LLDMSTAFlag = SET;
		}
	}

	/*-------------------------------------------------*/
	/* For Wireless Backbone Repeating Protocol, The   */
	/* In Sync packet has two more fields:                                  */
	/*                                                                                                                                      */
	/*              Node ID [6] : RFNC is synchronized to.                  */
	/*    Sync Depth  : Sync. depth of node that    RFNC    */
	/*                                                is synchronized to.                           */
	/*-------------------------------------------------*/

#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		CopyMemory (LLDSyncToNodeAddr, InSyncMsg->PISSyncToAddr,ADDRESSLENGTH);
		LLDSyncDepth = (unsigned_8) InSyncMsg->PISSyncDepth;
		LLSUpdateSyncAddr ();
	}
#endif
	
	/*-------------------------------------------------*/
	/* If PeerToPeerFlag is Disabled, need to setup the*/
	/* BridgeState and the Route to the master address      */
	/* for all the nodes                                                                                    */
	/*-------------------------------------------------*/
	
	if (!LLDPeerToPeerFlag)                 /* if in client server environment */
		LLDSetupAutoBridging();

	if ((Qptr = RemoveReEntrantHead ( &ReEntrantQ )) != NIL)

	{
		/*-------------------------------------------------------*/
		/* Send the packet out according to it's type.                          */
		/*                                                                                                                                                      */
		/* A zero value means it is a regular data packet, and a        */
		/* non-zero value means it is a packetized command.  The        */
		/* non-zero value is the packet length.                                         */              
		/*-------------------------------------------------------*/
		if ( Qptr->PacketType == 0 )
		{
			OutChar ('R', CYAN);
			OutChar ('Q', CYAN);
			OutChar ('S', CYAN);
			Status = LLDSend ( (struct LLDTxPacketDescriptor FAR *)Qptr->PacketPtr );
		}
		else
		{
			OutChar ('R', CYAN);
			OutChar ('Q', CYAN);
			OutChar ('P', CYAN);
			OutChar ('S', CYAN);
			Status = LLDPacketizeSend ( (unsigned_8 *)Qptr->PacketPtr, (unsigned_16)Qptr->PacketType );
		}

		AddToReEntrantList ( &FreeReEntrantQ, Qptr );
	}

}


/******************************************************************/
/*                                                                                                                                                                              */
/*      HandleHopStatistics (*HopStatPtr)                                                                               */
/*                                                                                                                                                                              */
/*      INPUT:  HopStatPtr      -       Pointer to the Hop statistics.                  */
/*                                                                                                                                                                              */
/*      OUTPUT:                                                                                                                                                 */
/*                                                                                                                                                                              */
/*      DESCRIPTION:  Hop Statistics command is an unsolicited message  */
/*               to the driver containing the statistics                                */
/*               collected during the previous hop period.                      */
/******************************************************************/

void HandleHopStatistics (struct PacketizeHopStat *HopStatPtr)
{
	unsigned_8      LLDLogicalFreqNum;
	unsigned_8      LLDPhysFreqNum;
	unsigned_8      LLDFreqBoolean;


	LLDLogicalFreqNum = HopStatPtr->PHSLogicalFreq;
	LLDPhysFreqNum    = HopStatPtr->PHSPhysicalFreq;
	LLDFreqBoolean          = HopStatPtr->PHSBoolean1;

}
