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
/*	$Header:   J:\pvcs\archive\clld\lldsend.c_v   1.61   03 Jun 1998 14:27:18   BARBARA  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldsend.c_v  $										*/
/* 
/*    Rev 1.61   03 Jun 1998 14:27:18   BARBARA
/* TxPackets counters added.
/* 
/*    Rev 1.60   07 May 1998 09:20:42   BARBARA
/* memcpy became CopyMemory and memset-SetMemory.
/* 
/*    Rev 1.59   24 Mar 1998 08:29:52   BARBARA
/* Comment eliminated.
/* 
/*    Rev 1.58   24 Mar 1998 08:07:34   BARBARA
/* LLD Ping added. Inactivity sec < 5 sec added.
/* 
/*    Rev 1.57   12 Sep 1997 16:48:02   BARBARA
/* Commented out code has been eliminated.
/* 
/*    Rev 1.56   23 Jun 1997 17:48:06   BARBARA
/* LLDREPEATBIT is set if the LLDTxBridgePktsFlag is equal to 2
/* 
/*    Rev 1.55   16 Jun 1997 18:19:50   BARBARA
/* BUG FIX: TX_MACACK was not set when it should be in the case of Multicast 
/* destination address in the bridge packet.
/* 
/*    Rev 1.53   15 Mar 1997 13:30:58   BARBARA
/* NodeExistsFlag has been added.
/* 
/*    Rev 1.52   18 Feb 1997 15:21:30   BARBARA
/* Bug fix in TxRoamNotify which created a non bridge packet.
/* 
/*    Rev 1.51   24 Jan 1997 17:30:36   BARBARA
/* Resetting the card after 5 sec of NAKs was added.
/* 
/*    Rev 1.50   15 Jan 1997 18:10:56   BARBARA
/* The way PacketSeqNum is calculated changed. LLDTxFragments changed. Multipath
/* related changes were added.
/* 
/*    Rev 1.49   11 Oct 1996 18:19:04   BARBARA
/* W_DummyReg was taken out.
/* 
/*    Rev 1.48   11 Oct 1996 11:06:04   BARBARA
/* Casting to unsigned_16 added to keep the compiler quiet.
/* 
/*    Rev 1.47   02 Oct 1996 13:53:24   JEAN
/* Added a header include file.
/* 
/*    Rev 1.46   02 Oct 1996 13:40:02   JEAN
/* In LLDPacketizeSend(), for one-piece cards, added calls to
/* W_DummyReg.
/* 
/*    Rev 1.45   26 Sep 1996 10:52:26   JEAN
/* Fixed a problem in LLDPacketizeSend() where we were not 
/* evaluating the condition that decides to set the wakeup bit.
/* 
/*    Rev 1.44   29 Jul 1996 12:22:52   JEAN
/* Fixed an incorrect comment.
/* 
/*    Rev 1.43   15 Jul 1996 18:22:16   TERRYL
/* Initialized LLDKeepAliveTime to the current time instead of 0 in 
/* LLDSend() routine
/* 
/*    Rev 1.42   15 Jul 1996 14:18:18   TERRYL
/* Changed the way to handle out of sync condition during a send.
/* Type casted the time difference (from LLSGetCurrentTime()) to 16-bit.
/* 
/*    Rev 1.41   17 Jun 1996 10:05:02   JEAN
/* Changed the BFSKRcvCount to max out at 2 instead of 3.  Fixed
/* a bug in packetized send when sending a Go To Standby command.
/* the wakeup bit was set when it should have been cleared.
/* 
/*    Rev 1.40   28 May 1996 16:09:32   TERRYL
/* Removed ForceFragments variable
/* 
/*    Rev 1.39   14 May 1996 09:17:32   TERRYL
/* Changed the way the SetWakeupBit() and ClearWakeupBit() are called.
/* Added in LLDForceFrag flag.
/* Added in more color-coded debugging information
/* 
/*    Rev 1.38   16 Apr 1996 18:11:38   TERRYL
/* Fixed a bug on LLDTxFragments(). Compiler is confused by one of the line.
/* 
/*    Rev 1.37   16 Apr 1996 18:08:56   TERRYL
/* Added LLDNoRetries on HandleTransmitComplete().
/* 
/*    Rev 1.36   16 Apr 1996 14:52:54   JEAN
/* Fixed a bug in HandleTransmitComplete (), where we would
/* roam if we were the master (masters don't roam).
/* 
/*    Rev 1.35   16 Apr 1996 11:21:24   JEAN
/* Changed the return status checking from TxRequest () and
/* LLDSendFragments ().  Changed the oldflags type from unsigned_16
/* to FLAGS.  For serial, we don't want to loop on TxRequest () in
/* LLDSendFragments ().  In LLDTxRoamNotify (), added a test to see
/* if it is the first time the driver has been in sync, if so, we
/* don't need to send a roam notify.  
/* 
/*    Rev 1.34   29 Mar 1996 11:29:00   JEAN
/* Fixed a problem for serial when we would reset the card if
/* TxRequest () returns a failure.  Added a check to make sure
/* there is some immediate data before calling TxData ().  Added
/* a fix where we were accesses a local variable in LLDTxFragments ()
/* before it was initialized.
/* 
/*    Rev 1.33   22 Mar 1996 14:39:38   JEAN
/* Added a check for a NIL pointer in LLDPacketizeSend () when
/* removing an entry from the re-entrant queue.  Added a test for
/* the GoToStandby packet before calling SetWakeupBit ().
/* 
/*    Rev 1.32   15 Mar 1996 13:53:48   TERRYL
/* Modified LLDSend() and HandleTransmitComplete() to incorporate 
/* PeerToPeer feature.
/* 
/*    Rev 1.31   14 Mar 1996 14:41:58   JEAN
/* Changed some debug color prints.
/* 
/*    Rev 1.30   13 Mar 1996 17:49:32   JEAN
/* Fixed a problem where we did not process zero length transmit
/* packet fragments properly.  Also added some sanity checks and
/* debug code.
/* 
/*    Rev 1.29   12 Mar 1996 10:36:40   JEAN
/* Fixed a problem for SERIAL in LLDSend when we set TxFlag.  This flag
/* gets incremented in LLDPoll when a timeout is detected and cleared
/* when a packet is successfully received.
/* 
/*    Rev 1.28   08 Mar 1996 19:04:58   JEAN
/* Fixed a comment.
/* 
/*    Rev 1.27   08 Mar 1996 18:56:48   JEAN
/* For serial CLLD, changed the mechanism for detecting missing
/* transmit response packets.  Added a check for invalid packet
/* lengths.  Added code to save the packet sequence number in the
/* node table (for LLDFlushQ).
/* 
/*    Rev 1.26   04 Mar 1996 15:03:38   truong
/* Added second packetize buffer for RoamNotify.
/* Fixed roam condition for BFSK retry state.
/* 
/*    Rev 1.25   08 Feb 1996 14:36:16   JEAN
/* Added the function LLDRawSend () back in.
/* 
/*    Rev 1.24   06 Feb 1996 14:18:26   JEAN
/* Comment changes.
/* 
/*    Rev 1.23   31 Jan 1996 12:55:46   JEAN
/* Fixed a problem in LLDSend() after returning from TxRequest(),
/* added some statistics, changed some constants to macros, and
/* changed the ordering of some header include files.
/* 
/*    Rev 1.22   14 Dec 1995 15:08:34   JEAN
/* Added a header include file and an extern declaration.
/* 
/*    Rev 1.21   07 Dec 1995 12:06:46   JEAN
/* Added a missing EnableRTS().
/* 
/*    Rev 1.20   04 Dec 1995 11:07:32   JEAN
/* Fixed a bug when transmitting fragments where the wakeup bi
/* bit does not get set and cleared.
/* 
/*    Rev 1.19   27 Nov 1995 14:46:26   JEAN
/* Fix a bug in HandleTransmitComplete where we were checking the
/* roaming state incorrectly.
/* 
/*    Rev 1.18   17 Nov 1995 16:33:40   JEAN
/* Check in for the RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.17   16 Oct 1995 16:04:38   JEAN
/* Removed some commented out code.
/* 
/*    Rev 1.16   12 Oct 1995 11:30:48   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* For SERIAL, added function calls to disable/enable RTS when
/* RFNC interrupts are masked.  Added #defines for the bridge
/* states.  Added a call to LLSSendComplete() in LLDSend() when
/* TxRequest() returns neither an ACK nor an NAK.  Fixed an
/* incorrect comparison when testing for roaming conditions based
/* on BFSK - HandleTransmitComplete().
/* 
/*    Rev 1.15   24 Jul 1995 18:36:04   truong
/* 
/*    Rev 1.14   20 Jun 1995 15:48:36   hilton
/* Added support for protocol promiscuous
/* 
/*    Rev 1.13   23 May 1995 09:41:04   tracie
/* Bug fixes
/* 
/*    Rev 1.12   24 Apr 1995 15:43:34   tracie
/* TxStartTime should be cleared.
/* 
/*    Rev 1.11   13 Apr 1995 08:47:36   tracie
/* XROMTEST version
/* 
/*    Rev 1.10   16 Mar 1995 16:13:44   tracie
/* Added support for ODI
/* 
/*    Rev 1.9   09 Mar 1995 15:07:02   hilton
/* 
/*    Rev 1.8   22 Feb 1995 10:25:32   tracie
/* Added Re-Entrant queue and Bug fixes in LLDSend()
/* 																								*/
/*    Rev 1.7   05 Jan 1995 09:53:12   hilton										*/
/* Changes for multiple card version													*/
/* 																								*/
/*    Rev 1.6   27 Dec 1994 15:43:24   tracie										*/
/* 																								*/
/*    Rev 1.4   14 Dec 1994 10:56:40   tracie										*/
/* 						Modified to work with Serial LLD (8250 UART working)	*/
/* 																								*/
/*    Rev 1.3   29 Nov 1994 12:44:28   hilton										*/
/* 																								*/
/*    Rev 1.2   22 Sep 1994 10:56:10   hilton										*/
/* 																								*/
/*    Rev 1.1   20 Sep 1994 16:02:58   hilton										*/
/* 																								*/
/*    Rev 1.0   20 Sep 1994 11:00:16   hilton										*/
/* Initial revision.																			*/
/*																									*/
/*																									*/
/***************************************************************************/



#include <string.h>
#include	"constant.h"
#include "lld.h"
#include	"asm.h"
#include	"lldinit.h"
#include	"lldpack.h"
#include	"pstruct.h"
#include "bstruct.h"
#ifndef SERIAL
#include "port.h"
#include "hardware.h"
#else
#include "slld.h"
#include "slldbuf.h"
#include "slldhw.h"
#include "slldport.h"
#endif
#include "lldsend.h"
#include "blld.h"
#include "pcmcia.h"
#include "lldqueue.h"

#ifdef WBRP
#include "rptable.h"
#include "wbrp.h"
#endif

#ifdef MULTI
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_8			LLDTransferMode;
extern unsigned_8			LLDBridgeFlag;
extern unsigned_8			LLDNodeAddress [ADDRESSLENGTH];
extern unsigned_8			LLDSleepFlag;
extern unsigned_8			LLDNAKTime;
extern unsigned_16		LLDFragSize;

extern unsigned_8			LLDSyncState;
extern struct NodeEntry	NodeTable [TOTALENTRY];
extern unsigned_8			LLDMapTable [128];
extern unsigned_8			LLDRoamingState;
extern unsigned_16		RoamStartTime;
extern unsigned_8			LLDMSTAAddr [ADDRESSLENGTH];
extern unsigned_8			LLDOldMSTAAddr [ADDRESSLENGTH];
extern unsigned_8			LLDRepeatingWBRP;
extern unsigned_16		LLDInactivityTimeOut;
extern unsigned_32		LLDReSent;
extern unsigned_32		LLDSent;
extern unsigned_32		LLDSentCompleted;
extern unsigned_32		LLDOutSyncQDrp;
extern unsigned_32		LLDOutSyncDrp;
extern unsigned_32		LLDTimedQDrp;
extern unsigned_32		LLDFailureDrp;
extern unsigned_32		LLDNoReEntrantDrp;
extern unsigned_32		LLDSendDisabledDrp;
extern unsigned_32		LLDTooBigTxLen;
extern unsigned_32  		LLDBadCopyLen;
extern unsigned_8	 		LLDTxMode;
extern unsigned_8			LLDTxModeSave;

extern unsigned_8 		LLDPeerToPeerFlag;
extern unsigned_8 		LLDNoRetries;
extern unsigned_8			LLDForceFrag;

extern unsigned_8			ControlValue;
extern unsigned_8			StatusValue;
extern unsigned_8			LLDSyncAlarm;
extern unsigned_16		LLDLastRoam;
extern unsigned_8			LLDCTSRetries;

extern unsigned_8		LLDRoamRetryTmr;
extern unsigned_8		LLDRoamResponse;
extern unsigned_16	LLDRoamResponseTmr;
extern unsigned_8		LLDTxFragRetries;
extern unsigned_8		LLDRoamingFlag;
extern unsigned_8		LLDOEMCustomer;

extern unsigned_8		LLDTicksToSniff;
extern unsigned_8		LLDSniffCount;

extern unsigned_32		FramesXmit;
extern unsigned_32		FramesXmitQFSK;
extern unsigned_32		FramesXmitBFSK;
extern unsigned_32		FramesXmitFrag;
extern unsigned_32		FramesACKError;
extern unsigned_32		FramesCTSError;

unsigned_16		LLDLivedTime;
unsigned_16		LLDKeepAliveTmr;

unsigned_8		InLLDSend=CLEAR;
unsigned_8		DisableLLDSend;
unsigned_8		LLDOutOfSyncCount;
unsigned_8		TxFlag;
unsigned_8		TxFlagCleared;
#ifndef SERIAL
unsigned_16		TxStartTime;
#else
unsigned_16		TxStartTime[128];
unsigned_8		TxInProgress[128];
#endif
unsigned_16		WBRPStartTime;

unsigned_8		LLDTxBuffer [BUFFERLEN];
unsigned_8		PacketizePktBuf [128];
unsigned_8		SecPacketizePktBuf [128];
unsigned_8		PacketSeqNum;

unsigned_8		LLDNeedReset;

struct ReEntrantQueue	ReEntrantQBuff [TOTALREQUEUE];
struct LLDLinkedList 	FreeReEntrantQ;
struct LLDLinkedList 	ReEntrantQ;

#endif


/***************************************************************************/
/*																									*/
/*	LLDPacketizeSend (*PtrToPkt, PktLength)											*/
/*																									*/
/*	INPUT:	*PtrToPkt	-	Pointer to the packetize packet to be sent		*/
/*				PktLength	-	Length of the packetize packet to be sent			*/
/*																									*/
/*	OUTPUT:	Returns	 	-	Success / Error											*/
/*																									*/
/*	DESCRIPTION: This routine will send a packetize packet to the card		*/
/* 	and return a success or failure code.											*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDPacketizeSend (unsigned_8 *PtrToPkt, unsigned_16 PktLength)

{	struct PReqHdr	*Header;
	unsigned_8	FuncNum;
	unsigned_8	Status;
	unsigned_8	Color;
	struct ReEntrantQueue *Qptr;
	unsigned_8	SaveControlValue;
	unsigned_8	SaveStatusValue;

	OutChar ('P', GREEN);
	OutChar ('s', GREEN);

	/*---------------------------------------*/
	/* If already in the sending state, put  */
	/* the new packet in the ReEntrantQueue. */
	/*---------------------------------------*/
	if (InLLDSend == SET)
	{
		Qptr = RemoveReEntrantHead ( &FreeReEntrantQ );

		OutChar ('R', RED + BLINK);
		OutChar ('e', RED + BLINK);
		OutChar ('E', RED + BLINK);
		OutChar ('n', RED + BLINK);
		OutChar ('t', RED + BLINK);
		OutChar ('r', RED + BLINK);
		OutChar ('a', RED + BLINK);		
		OutChar ('n', RED + BLINK);
		OutChar ('t', RED + BLINK);

		if (Qptr == NIL)
		{
			OutChar ('N', RED + BLINK);		
			OutChar ('I', RED + BLINK);
			OutChar ('L', RED + BLINK);
			return (SUCCESS);
		}

		Qptr->PacketPtr  = PtrToPkt;		/* Pointer to packet to be sent 		*/

		/*-------------------------------------------------------*/
		/* A zero value means it is a regular data packet, and a	*/
		/* non-zero value means it is a packetized command.  The	*/
		/* non-zero value is the packet length.						*/		
		/*-------------------------------------------------------*/
		Qptr->PacketType = PktLength;
		AddToReEntrantList ( &ReEntrantQ, Qptr );

		return (SUCCESS);
	}

	InLLDSend = SET;

#ifdef SERIAL
	DisableRTS ();
#endif
	PushRFNCInt ();
	DisableRFNCInt ();

	Header = (struct PReqHdr *) PtrToPkt;
	FuncNum = Header->PHFnNo;

	if (FuncNum < 10)
	{	OutChar (Header->PHCmd, GREEN);
		OutChar ((unsigned_8)(FuncNum + '0'), GREEN);
	}

	else if (FuncNum < 20)
	{	if (Header->PHCmd == 'A')
		{	if ((FuncNum == SET_ROAMING_FN) || (FuncNum == ROAM_FN))
				Color = YELLOW;
			else
				Color = GREEN;
		}
		else
			Color = GREEN;

		OutChar (Header->PHCmd, Color);
		OutChar ('1', Color);
		OutChar ((unsigned_8)(FuncNum - 10 + '0'), Color);
	}

	else
	{	OutChar (Header->PHCmd, GREEN);
		OutChar ('2', GREEN);
		OutChar ((unsigned_8)(FuncNum - 20 + '0'), GREEN);
	}

	/*-------------------------------------------------------*/
	/* For all packetized commands EXCEPT the Go To Standby	*/
	/* command, we want to set the wakeup bit.  					*/
	/*-------------------------------------------------------*/
	if (! ((FuncNum == GOTO_STANDBY_FN) && (Header->PHCmd == INIT_COMMANDS_CMD)))
	{
#ifndef SERIAL
		ControlValue &= ~WAKEUPBIT;
#endif
		SaveControlValue = ControlValue;
		SaveStatusValue = StatusValue;
		SetWakeupBit ();
	}

	/*---------------------------------*/
	/* Make sure the length is not ODD */
	/*---------------------------------*/
#ifndef SERIAL
	if ( PktLength & 0x01 )
	{
		PktLength++;
	}
#endif

	Status = TxRequest (PktLength, LLDTransferMode);
	if ( Status == NAK )
	{
		OutChar ('N', RED+BLINK);
		OutChar ('A', RED+BLINK);
		OutChar ('K', RED+BLINK);

		ClearNAK ();
		if (LLDOEMCustomer)
			LLDNeedReset = SET;
	}

	else if ( Status == FAILURE )
	{
		OutChar ('F', RED+BLINK);
		OutChar ('A', RED+BLINK);
		OutChar ('I', RED+BLINK);
		OutChar ('L', RED+BLINK);
		OutChar ('U', RED+BLINK);
		OutChar ('R', RED+BLINK);
		OutChar ('E', RED+BLINK);

		/*----------------------------------------*/
		/* The serial TxRequest returns a failure	*/
		/* when an invalid packet length is			*/
		/* received.  We do not want to reset		*/
		/* when this happens.							*/
		/*----------------------------------------*/
#ifndef SERIAL
		LLDNeedReset = SET;
#endif

	}

	else
	{	
		TxData (PtrToPkt, PktLength, LLDTransferMode);
		Status = EndOfTx ();
	}

	if (TxFlag)
	{	SaveControlValue |= WAKEUPBIT;
		SaveStatusValue |= STAYAWAKE;
	}
	ClearWakeupBit (SaveControlValue, SaveStatusValue);
	PopRFNCInt ();
#ifdef SERIAL
	EnableRTS ();
#endif

	InLLDSend = CLEAR;

	OutChar ('E', GREEN);
	OutChar ('P', GREEN);
	OutChar ('s', GREEN);

	if ((Status == SUCCESS) &&
		((Qptr = RemoveReEntrantHead ( &ReEntrantQ )) != NIL) )

	{
 		/*-------------------------------------------------------*/
		/* Send the packet out according to it's type.				*/
		/*																			*/
 		/* A zero value means it is a regular data packet, and a	*/
		/* non-zero value means it is a packetized command.  The	*/
		/* non-zero value is the packet length.						*/		
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

	return (Status);
}




/***************************************************************************/
/*																									*/
/*	LLDSendProximPkt (*PktDesc)															*/
/*																									*/
/*	INPUT:	PktDesc	-	Pointer to the packet descriptor to send				*/
/*																									*/
/*	OUTPUT:	Returns	-	0	- Success													*/
/*								1	- Error														*/
/*																									*/
/*	DESCRIPTION: This routine will send a Proxim protocol packet using		*/
/*		LLDSend.  At this time, no checking is done and this is just a			*/
/*		procedure which acts exactly like LLDSend.									*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendProximPkt (struct LLDTxPacketDescriptor FAR *PktDesc)

{	unsigned_8	Status;


	Status = LLDSend (PktDesc);

	if (Status != 0xFF)
	{	return (0);
	}
	else
	{	return (1);
	}
}



/***************************************************************************/
/*																									*/
/*	LLDSend (*PktDesc)																		*/
/*																									*/
/*	INPUT:	PktDesc	-	Pointer to the packet descriptor to send				*/
/*																									*/
/*	OUTPUT:	Returns	-	0	- Success													*/
/*								1	- Send delay flag was set								*/
/*								FF	- Fatal, transmit timed out							*/
/*																									*/
/*	DESCRIPTION: This routine will send a data packet to the card.				*/
/*					 This routine is used when:											*/
/*																									*/
/*						1. Upper layer wants to send a packet using the Transmit */
/*							packet descriptor.												*/
/*						2. This routine will add the Bridge Header, Repeating		*/
/*							when necessary.													*/
/*						3. This routine will make decision on Fragmentation Send.*/
/*																									*/
/*              Regardless of the return status, the function the 			*/
/*              LLSSendComplete still gets called.									*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSend (struct LLDTxPacketDescriptor FAR *PktDesc)

{	
	struct NodeEntry 		  *NodeEntryPtr;
	unsigned_8 			 FAR *StartOfData;
	unsigned_8			 		TableEntry;
	unsigned_16					CurrTime;
	unsigned_8					LLDBridgeDest [ADDRESSLENGTH];
	unsigned_8					LLDEntryAddress [ADDRESSLENGTH];
	unsigned_8					LLDTxBridgePktsFlag;
	struct PacketizeTx	  *TxCmdPkt;
	unsigned_8					Status;
	unsigned_16					PacketPadding;
	unsigned_16					PacketSize;
	unsigned_8					LeftOverBytes [2];
	unsigned_8					LeftOverFlag;
	unsigned_16					i;
	struct ReEntrantQueue  *Qptr;
	FLAGS							oldflag;
	unsigned_8					SaveControlValue;
	unsigned_8					SaveStatusValue;
	short	int					ii;

	/*-----------------------------------------*/
	/* The following variable is used to store */
	/* the repeating destination address,		 */
	/* repeating address and hops left field.	 */
	/*-----------------------------------------*/
#ifdef WBRP
	unsigned_8			WBRPBuf [REPEATINGHDRLEN];	
#endif


	OutChar ('S', RED + WHITE_BACK);

#if 0
	OutHex ((unsigned_8)((unsigned_32)PktDesc >> 24), BROWN);
	OutHex ((unsigned_8)((unsigned_32)PktDesc >> 16), BROWN);
	OutHex ((unsigned_8)((unsigned_32)PktDesc >>  8), BROWN);
	OutHex ((unsigned_8)((unsigned_32)PktDesc >>  0), BROWN);
#endif

	/*-------------------------------------------------*/
	/* Make sure the packet length is a valid ethernet	*/
	/* packet length.  If the packet is too small, we	*/
	/* we will pad it to 60 bytes.  If it is greater	*/
	/* than 1514 bytes, drop it.								*/
	/*-------------------------------------------------*/
	if ( (PktDesc->LLDTxDataLength > MAX_PKT_SIZE) || 
		  (PktDesc->LLDTxDataLength == 0))
	{
		LLDTooBigTxLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(PktDesc->LLDTxDataLength >> 8), RED);
		OutHex ((unsigned_8)(PktDesc->LLDTxDataLength >> 0), RED);
#if INT_DEBUG
		OutHex ((unsigned_8)((unsigned_32)PktDesc >> 24), BROWN);
		OutHex ((unsigned_8)((unsigned_32)PktDesc >> 16), BROWN);
		OutHex ((unsigned_8)((unsigned_32)PktDesc >>  8), BROWN);
		OutHex ((unsigned_8)((unsigned_32)PktDesc >>  0), BROWN);
		__asm	int 3
#endif

		if (IsTxProximPacket (PktDesc))
		{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (PktDesc);
			}
			else
			{	LLSSendProximPktComplete (PktDesc, BAD_LENGTH);
			}
		}
		else
		{	LLSSendComplete (PktDesc);
		}
		return (0);
	}

#ifdef SERIAL
	DisableRTS ();
#endif
	oldflag = PreserveFlag ();
	DisableInterrupt ();

	/*-------------------------------------------------*/
	/* Check for re-entrancy and set flag to make sure	*/
	/* we do not re-enter here.								*/
	/*																	*/
	/*-------------------------------------------------*/

	/*---------------------------------------*/
	/* If already in the sending state, put  */
	/* the new packet in the ReEntrantQueue. */
	/*---------------------------------------*/
	if (InLLDSend == SET)
	{
		OutChar ('R', RED + BLINK);
		OutChar ('e', RED + BLINK);
		OutChar ('E', RED + BLINK);
		OutChar ('n', RED + BLINK);
		OutChar ('t', RED + BLINK);
		OutChar ('r', RED + BLINK);
		OutChar ('a', RED + BLINK);
		OutChar ('n', RED + BLINK);
		OutChar ('t', RED + BLINK);

		Qptr = RemoveReEntrantHead ( &FreeReEntrantQ );
		if ( Qptr == NIL )
		{
			OutChar ('N', RED + BLINK);
			OutChar ('I', RED + BLINK);
			OutChar ('L', RED + BLINK);

			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, QUEUE_FULL);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}

			LLDNoReEntrantDrp++;
			RestoreFlag (oldflag);
#ifdef SERIAL
			EnableRTS ();
#endif
			return (0);
		}

		Qptr->PacketPtr  = (unsigned_8 FAR *)PktDesc;

		/*-------------------------------------------------------*/
		/* A zero value means it is a regular data packet, and a	*/
		/* non-zero value means it is a packetized command.  The	*/
		/* non-zero value is the packet length.						*/		
		/*-------------------------------------------------------*/
		Qptr->PacketType = 0;
		AddToReEntrantList (&ReEntrantQ, Qptr);

		RestoreFlag (oldflag);
#ifdef SERIAL
		EnableRTS ();
#endif

		return (0);
	}
	InLLDSend = SET;


	if (DisableLLDSend)
	{	
		if (IsTxProximPacket (PktDesc))
		{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (PktDesc);	
			}
			else
			{	LLSSendProximPktComplete (PktDesc, SEND_DISABLED);
			}
		}
		else
		{	LLSSendComplete (PktDesc);
		}

		LLDSendDisabledDrp++;
		InLLDSend = CLEAR;	
		RestoreFlag (oldflag);
#ifdef SERIAL
		EnableRTS ();
#endif
		return (0);
	}

	EnableInterrupt ();	
#ifdef SERIAL
	EnableRTS ();
#endif

	LLDTxBridgePktsFlag = 0;


	/*-------------------------------------------------*/
	/* Check the immediate data section.  If none, set	*/
	/* the start to the first fragment.  Copy the		*/
	/* destination address to our local variable and	*/
	/* find or allocate a node table entry for it.		*/
	/*																	*/
	/*-------------------------------------------------*/

	if (PktDesc->LLDImmediateDataLength)
	{	
		StartOfData = PktDesc->LLDImmediateData;
	}
	else
	{	
		StartOfData = PktDesc->LLDTxFragmentListPtr->LLDTxFragList [0].FSDataPtr;
	}

#ifdef ODI
	fmemcpy ((unsigned_8 FAR *)LLDEntryAddress, StartOfData, ADDRESSLENGTH);
#else
	CopyMemory (LLDEntryAddress, StartOfData, ADDRESSLENGTH);
#endif

	LLDGetEntry (LLDEntryAddress, &TableEntry);
	OutHex (TableEntry, BLUE);
	NodeEntryPtr = &NodeTable [TableEntry];


	/*-------------------------------------------------*/
	/*																	*/
	/* If this is the Wireless Backbone Repeating LLD, */
	/* then automatically call the WBRP functions to   */
	/* handle it.													*/
	/*																	*/
	/*-------------------------------------------------*/
#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		/*-------------------------------------------------*/
		/* If its a Broadcast message (return code = 1),   */
		/* then continue with the LLDSend().					*/
		/*-------------------------------------------------*/
		if ( (i = WBRPGetRepeaterAddr (&WBRPBuf[0],LLDEntryAddress)) == 0 )
		{	
			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBufer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, DROPPED);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}

			InLLDSend = CLEAR;	
			RestoreFlag (oldflag);
			OutChar ('N', YELLOW+WHITE_BACK);
			OutChar ('o', YELLOW+WHITE_BACK);
			OutChar ('W', YELLOW+WHITE_BACK);
			OutChar ('B', YELLOW+WHITE_BACK);
			OutChar ('R', YELLOW+WHITE_BACK);
			OutChar ('P', YELLOW+WHITE_BACK);
			return (0);
		}
	}
#endif
		

	/*-------------------------------------------------*/
	/* Check if out of sync.  May need to send a keep	*/
	/* alive message to wakeup the card to get an in-	*/
	/* sync message.  Give the packet back to the		*/
	/* upper layer until there have been more than 10	*/
	/* messages passed up at which point the packet		*/
	/* will be put on the timed queue to send.			*/
	/*	LLDOutOfSyncCount will be cleared during InSync */
	/* message.                                        */
	/*-------------------------------------------------*/

	if (LLDSyncState == CLEAR)
	{	
		OutChar ('N', RED);
		OutChar ('o', RED);
		OutChar ('t', RED);
		OutChar ('S', RED);
		OutChar ('y', RED);
		OutChar ('n', RED);
		OutChar ('c', RED);

		if (LLDInactivityTimeOut)
		{
			/*-----------------------------------------*/
			/* Clear InLLDSend flag to avoid ReEntrant */
			/* checking, then reset it back.				 */
			/*-----------------------------------------*/
			InLLDSend = CLEAR;
			if (LLDSendKeepAlive ())
			{	
				OutChar ('T', RED);
				OutChar ('1', RED);
				LLDNeedReset = SET;
			}
			InLLDSend = SET;
		}

		LLDOutSyncDrp++;

		if (IsTxProximPacket (PktDesc))
		{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (PktDesc);	
			}
			else
			{	LLSSendProximPktComplete (PktDesc, OUT_OF_SYNC);
			}
		}
		else
		{	LLSSendComplete (PktDesc);
		}

		InLLDSend = CLEAR;	
		RestoreFlag (oldflag);
		return (0);
	}

	/*-------------------------------------------------*/
	/* Check if the node already has a packet in the	*/
	/* timed send queue.  If so, return the current		*/
	/* packet to the upper layer.								*/
	/*																	*/
	/*-------------------------------------------------*/

	if (NodeEntryPtr->PktInTimedQFlag)
	{	
		LLDAddToQueue (PktDesc, TableEntry);
		InLLDSend = CLEAR;	
		RestoreFlag (oldflag);
		return (0);
	}


	/*-------------------------------------------------*/
	/* Check to see if the node already has a packet	*/
	/* in progress.  If so, queue this packet to be		*/
	/* sent after the others are completed.				*/
	/*																	*/
	/*-------------------------------------------------*/
	
	if (NodeEntryPtr->PktInProgressFlag)
	{	
		LLDAddToQueue (PktDesc, TableEntry);
		InLLDSend = CLEAR;	
		RestoreFlag (oldflag);
		return (0);
	}


	/*-------------------------------------------------*/
	/* Done checking conditions.  Everything O.K. and	*/
	/* now check what type of packet should be sent		*/
	/* and prepare it.											*/
	/*																	*/
	/* PRESERVEFLAG will push the flag on the stack!!!	*/
	/* Any exit after this point must perform a			*/
	/*	RESTOREFLAG operation!!!!								*/
	/*																	*/
	/*-------------------------------------------------*/

#ifdef SERIAL
	DisableRTS ();
#endif
	DisableInterrupt ();
	NodeEntryPtr->PktInProgressFlag = SET;

			
	/*-------------------------------------------------*/
	/* Check the BridgeState.  If it's direct,			*/
	/*	and this code is running in a bridge, set			*/
	/*	the bridge destination variable to the Dest.		*/
	/*-------------------------------------------------*/

	if (NodeEntryPtr->BridgeState == BS_DIRECT)
	{	if (LLDBridgeFlag)
		{	CopyMemory (LLDBridgeDest, LLDEntryAddress, ADDRESSLENGTH);
			LLDTxBridgePktsFlag = 1;
		}
	}

	/*-------------------------------------------------*/
	/* If BridgeState is bridged, need to check the 	*/
	/* LLDPeerToPeerFlag. If LLDPeerToPeerFlag is 		*/
	/* Enabled, check to see if the state should be 	*/
   /* timed out.  If not timeout, set up to send 		*/
	/* through a bridge, by using the Route as the 		*/
	/* destination. If LLDPeerToPeerFlag is Disabled, 	*/
	/* always send through a bridge, by using the Route*/
   /* as the destination.										*/
	/*																	*/
	/*-------------------------------------------------*/

	else if (NodeEntryPtr->BridgeState == BS_BRIDGED)
	{	
		if (LLDPeerToPeerFlag)	/* If not in client server environment */
		{
			CurrTime = LLSGetCurrentTime ();
			if ((unsigned_16)(CurrTime - NodeEntryPtr->BridgeStartTime) > BRIDGETIMEOUT)
			{	NodeEntryPtr->BridgeState = BS_DIRECT;
				NodeEntryPtr->BridgeStartTime = 0;
			}
			else
			{	OutChar ('B', MAGENTA);
				OutChar ('P', MAGENTA);
				OutChar ('k', MAGENTA);
				OutChar ('t', MAGENTA);

				LLDTxBridgePktsFlag = 1;
				if (NodeEntryPtr->RepeatFlag)
				{	LLDTxBridgePktsFlag = 3;
				}
				CopyMemory (LLDBridgeDest, NodeEntryPtr->Route, ADDRESSLENGTH);
			}				
		}
		else	/* else in client server environment */
		{	OutChar ('B', MAGENTA);
			OutChar ('P', MAGENTA);
			OutChar ('k', MAGENTA);
			OutChar ('t', MAGENTA);

			OutChar ('P', MAGENTA);
			OutChar ('e', MAGENTA);
			OutChar ('e', MAGENTA);
			OutChar ('r', MAGENTA);

			LLDTxBridgePktsFlag = 1;
			if (NodeEntryPtr->RepeatFlag)
			{	LLDTxBridgePktsFlag = 3;
			}
			CopyMemory (LLDBridgeDest, NodeEntryPtr->Route, ADDRESSLENGTH);
		}
	}


	/*-------------------------------------------------*/
	/* If BridgeState is Discovery, check to see if		*/
	/* the state should be timed out.  If not, set the	*/
	/* destination as a broadcast.							*/
	/*																	*/
	/*-------------------------------------------------*/

	else if (NodeEntryPtr->BridgeState == BS_DISCOVERY)
	{	
		CurrTime = LLSGetCurrentTime ();
		if ((unsigned_16)(CurrTime - NodeEntryPtr->BridgeStartTime) > BRIDGETIMEOUT)
		{	NodeEntryPtr->BridgeState = BS_DIRECT;
			NodeEntryPtr->BridgeSuccess = 0;
			NodeEntryPtr->BridgeStartTime = 0;
		}
		else
		{	OutChar ('B', MAGENTA);
			OutChar ('D', MAGENTA);
			OutChar ('i', MAGENTA);
			OutChar ('s', MAGENTA);
			OutChar ('P', MAGENTA);
			OutChar ('k', MAGENTA);
			OutChar ('t', MAGENTA);

			LLDTxBridgePktsFlag = 2;
			memset (LLDBridgeDest, 0xFF, ADDRESSLENGTH);
		}
	}


	/*-------------------------------------------------*/
	/* Continuing the send.  All states execute.			*/
	/* Reset timers and check for fragmentation.			*/
	/*																	*/
	/*-------------------------------------------------*/

	LLDLivedTime = 0;
	LLDKeepAliveTmr = LLSGetCurrentTime();

	if (LLDForceFrag)
	{	NodeEntryPtr->FragState = 3;
		NodeEntryPtr->FragStartTime = LLSGetCurrentTime ();
	}

	if ((NodeEntryPtr->FragState == 3) && (PktDesc->LLDTxDataLength > LLDFragSize))
	{	
		CurrTime = LLSGetCurrentTime ();
		if ((unsigned_16)(CurrTime - NodeEntryPtr->FragStartTime) > FRAGTIMEOUT)
		{	
			NodeEntryPtr->FragState = 0;
			NodeEntryPtr->FragStartTime = 0;
		}
		else
		{	
			Status = LLDTxFragments (PktDesc, TableEntry, LLDTxBridgePktsFlag, LLDEntryAddress, LLDBridgeDest);
			InLLDSend = CLEAR;	
			RestoreFlag (oldflag);
#ifdef SERIAL
			EnableRTS ();
#endif
			return (Status);
		}
	}


	/*-------------------------------------------------*/
	/* Non-Fragmentation send.									*/
	/* Check if need to send in BFSK mode.					*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((NodeEntryPtr->BFSKFlag) && (++(NodeEntryPtr->BFSKPktCount) <= BFSKCOUNT))
	{	OutChar ('B', BRIGHT_WHITE);
		OutChar ('F', BRIGHT_WHITE);
		OutChar ('S', BRIGHT_WHITE);
		OutChar ('K', BRIGHT_WHITE);
	
		LLDTxMode = TX_BFSK;
	}


	/*-------------------------------------------------*/
	/* Not in BFSK mode, make sure variables are reset	*/
	/* in case we timed out of the BFSK state.  Must	*/
	/* check for broadcast and bridge discovery pkts	*/
	/* and decrement the BFSKRcvCount because we don't	*/
	/* want to count these to go into the BFSK state.	*/
	/*																	*/
	/*-------------------------------------------------*/

	else
	{	NodeEntryPtr->BFSKFlag = 0;
		NodeEntryPtr->BFSKPktCount = 0;
		LLDTxMode = LLDTxModeSave;

		if (NodeEntryPtr->BFSKRcvCount != 1)
		{	NodeEntryPtr->BFSKRcvCount = 0;
		}

		if (NodeEntryPtr->BridgeState == BS_DISCOVERY)
		{	NodeEntryPtr->BFSKRcvCount--;
		}
		else
		{	if (NodeEntryPtr->Address [0] & 0x01)
			{	NodeEntryPtr->BFSKRcvCount--;
			}
		}
	}


	/*-------------------------------------------------*/
	/* Done with the checks, continue with the send		*/
	/*																	*/
	/*-------------------------------------------------*/

	OutChar ('B', GREEN);
	OutChar ('0', GREEN);
	OutHex (PacketSeqNum, MAGENTA);

	/*-------------------------------------------------*/
	/* Make sure the Rx Status will not be set by the  */
	/* firmware during the Sending state.					*/
	/*-------------------------------------------------*/

	PushRFNCInt ();
	DisableRFNCInt ();
	
	SaveControlValue = ControlValue;
	SaveStatusValue = StatusValue;
	SetWakeupBit ();

	/*-------------------------------------------------*/
	/* Build the header for the transmit data command.	*/
	/*																	*/
	/*-------------------------------------------------*/

	TxCmdPkt = (struct PacketizeTx *) PacketizePktBuf;
	TxCmdPkt->PTHeader.PHCmd 	= DATA_COMMANDS_CMD;
	TxCmdPkt->PTHeader.PHFnNo 	= TRANSMIT_DATA_FN;
	TxCmdPkt->PTHeader.PHRsv 	= RESERVED;


	/*-------------------------------------------------*/
	/* Use the sequence number to find back the Node	*/
	/* entry when the transmit complete comes back.		*/
	/* Also, increment the Sequence number making sure	*/
	/* it doesn't match the one reserved for repeat		*/
	/* packets.														*/
	/*																	*/
	/*-------------------------------------------------*/
	ii = 0;
	while ((LLDMapTable [PacketSeqNum] != SET) && (ii++ < MAP_TABLE_SIZE))
	{	do
		{	PacketSeqNum = (unsigned_8)((PacketSeqNum + 1) & SEQNOMASK);
		} while (PacketSeqNum > MAX_DATA_SEQNO);
	}

	LLDMapTable [PacketSeqNum] = TableEntry;
	NodeEntryPtr->LastPacketSeqNum = PacketSeqNum;
	TxCmdPkt->PTHeader.PHSeqNo = PacketSeqNum;

	/*----------------------------------------------------------------------*/
	/*																								*/
	/* Choose a sequence number for the next packet to go through LLDSend.	*/
	/*																								*/
	/*----------------------------------------------------------------------*/
	do
	{	PacketSeqNum = (unsigned_8)((PacketSeqNum + 1) & SEQNOMASK);
	} while (PacketSeqNum > MAX_DATA_SEQNO);


	/*-------------------------------------------------*/
	/* Fill in the LLD header bytes.							*/
	/*																	*/
	/*-------------------------------------------------*/

	TxCmdPkt->PTLLDHeader1 = 0;
#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		TxCmdPkt->PTLLDHeader1 |= LLDWBRPONBIT;
	}
#endif

	if (LLDTxBridgePktsFlag)
	{	
		TxCmdPkt->PTLLDHeader2 = LLDBRIDGEBIT;
		if (LLDTxBridgePktsFlag == 3 || LLDTxBridgePktsFlag == 2)
		{	
			TxCmdPkt->PTLLDHeader2 |= LLDREPEATBIT;
		}
	}
	else
		TxCmdPkt->PTLLDHeader2 = 0;

#ifndef WBRP
	/*----------------------------------------*/
	/* Don't repeat it if its in WBRP mode		*/
	/*----------------------------------------*/
	if ((LLDTxBridgePktsFlag == 2) || (LLDEntryAddress [0] & 0x01))
	{	
		TxCmdPkt->PTLLDHeader2 |= LLDREPEATBIT;
	}
#endif

	TxCmdPkt->PTLLDHeader3 = NodeEntryPtr->DataSequenceNum;
	TxCmdPkt->PTLLDHeader4 = 0;


	/*-------------------------------------------------*/
	/* Set the Tx Power/Antenna byte.  Bit 7 of this	*/
	/* byte is set if the destination should wake up	*/
	/* if it is sleeping.  Do not wakeup up on a			*/
	/* discovery packet.											*/
	/*																	*/
	/*-------------------------------------------------*/

	if (LLDTxBridgePktsFlag == 2)
	{	TxCmdPkt->PTTxPowerAnt = 0x70;
	}
	else
	{	TxCmdPkt->PTTxPowerAnt = 0x70 | 0x80;
	}


	/*-------------------------------------------------*/
	/* Set the control byte.  Always set poll final,	*/
	/* check if need MAC Acknowledge service, and set	*/
	/* to transfer in BFSK, QFSK, or AUTO.  Unless		*/
	/* QFSK override, all broadcasts are in BFSK.		*/
	/*																	*/
	/*-------------------------------------------------*/

	/* No MAC ACK for bridge discovery packets.		*/
	
	if (LLDTxBridgePktsFlag == BS_DISCOVERY)
	{	if (LLDTxMode == TX_QFSK)
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_QFSK;
		}
		else
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_BFSK;
		}
	}

	/* No MAC ACK for bridged multicast packets.		*/

	else if ((LLDTxBridgePktsFlag == BS_BRIDGED) 
							&& (LLDBridgeDest[0] & 0x01))
	{	if (LLDTxMode == TX_QFSK)
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_QFSK;
		}
		else
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_BFSK;
		}
	}

	/* MAC ACK for directed multicast packets.		*/

	else if ((LLDTxBridgePktsFlag == BS_DIRECT) 
							&& (LLDEntryAddress [0] & 0x01))
	{	if (LLDTxMode == TX_QFSK)
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_QFSK;
		}
		else
		{	TxCmdPkt->PTControl = TX_POLLFINAL | TX_BFSK;
		}
	}

	/* MAC ACK for directed or bridged unicast packets.	*/

	else
	{	TxCmdPkt->PTControl = (unsigned_8)(TX_POLLFINAL | TX_MACACK | LLDTxMode);
	}

	/*-------------------------------------------------*/
	/* Calculate the length of the packet.  Check for	*/
	/* small packets which need to be padded.  Check	*/
	/* for bridge packets which have a 12 byte header.	*/
	/* Finally add the 4 byte LLD Header.					*/
	/*																	*/
	/* Remember to store the MSB first.						*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketSize = PktDesc->LLDTxDataLength;

	if (PacketSize < MIN_PKT_SIZE)
	{	
		/*------------- Changed by Serial LLD ----------------*/
		PacketPadding = (MIN_PKT_SIZE - PacketSize);
		PacketSize = MIN_PKT_SIZE;
	}
	else
	{	PacketPadding = 0;
	}

	if (LLDTxBridgePktsFlag)
	{	PacketSize += 12;
	}

	PacketSize += 4;

	/*-------------------------------------------------*/
	/* If its the Repeating LLD, then ADD extra fields */
	/* for the Repeating Information in front:			*/
	/* 																*/
	/* 	Destination (6) +										*/
	/*		Source		(6) +										*/
	/*		Hops Left	(1) +										*/
	/*		Reserve		(1) 										*/
	/* 																*/
	/* Total: 14 bytes											*/
	/*-------------------------------------------------*/
#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		PacketSize += RPLENGTH;
	}
#endif

	TxCmdPkt->PTPktLength [0] = (unsigned_8) (PacketSize >> 8);
	TxCmdPkt->PTPktLength [1] = (unsigned_8) (PacketSize & 0x00FF);


	/*-------------------------------------------------*/
	/* Now calculate the length of the entire packet	*/
	/* that will be sent to the RFNC.  This includes	*/
	/* the hardware header, media header, and the		*/
	/* length of the data packet.								*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketSize = PktDesc->LLDTxDataLength;

	if (PacketSize < MIN_PKT_SIZE)
	{	PacketSize = MIN_PKT_SIZE;
	}

	PacketSize += sizeof (struct PacketizeTx);

	if (LLDTxBridgePktsFlag)
	{	PacketSize += 12;
	}

	if (PacketSize & 0x01)
	{	
		PacketSize++;			/* avoid Odd length */
	}


	/*-------------------------------------------------*/
	/* If its the Repeating LLD, then ADD the repeating*/
	/* destination, source, and hops-left in front of	*/
	/* the data buffer.											*/
	/*-------------------------------------------------*/
#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		PacketSize += RPLENGTH;
	}
#endif

	/* This is not a fragmented packet. */
	NodeEntryPtr->NumberOfFragments = 0;

	/*-------------------------------------------------*/
	/* Now request to send the packet to the card and	*/
	/* handle any error conditions.							*/
	/*																	*/
	/*-------------------------------------------------*/

	Status = TxRequest (PacketSize, LLDTransferMode);
	if (Status != ACK)
	{
		/*-------------------------------------------*/
		/* The following two statements are added to */
		/* fix the NAK not cleared and causes reset  */
		/* bug.													*/
		/*-------------------------------------------*/
		TxFlag = CLEAR;
#ifndef SERIAL
		TxStartTime = LLSGetCurrentTime ();
#else
		TxStartTime [NodeEntryPtr->LastPacketSeqNum] = LLSGetCurrentTime ();
		TxInProgress [NodeEntryPtr->LastPacketSeqNum] = CLEAR;
#endif

		NodeEntryPtr->PktInProgressFlag = CLEAR;
		NodeEntryPtr->LastPacketSeqNum = SET;
		LLDMapTable [PacketSeqNum] = SET;

		if (TxFlag)
		{	SaveControlValue |= WAKEUPBIT;
			SaveStatusValue |= STAYAWAKE;
		}
		ClearWakeupBit (SaveControlValue, SaveStatusValue);
		PopRFNCInt ();
		InLLDSend = CLEAR;	

		if (Status == NAK)
		{	
			OutChar ('N', RED);
			OutChar ('a', RED);
			OutChar ('k', RED);

			ClearNAK ();
			CurrTime = LLSGetCurrentTime ();
			LLDAddToTimedQueue (PktDesc, TableEntry, CurrTime, MAXNAKWAIT,1);
			RestoreFlag (oldflag);
#ifdef SERIAL
			EnableRTS ();
#endif
			return (1);
		}

		/*----------------------------------------------------*/
		/* If we don't receive an ACK or a NAK, then there		*/
		/* is something wrong with the card.  We want to		*/
		/* return the packet to the upper layer and set			*/
		/* a flag to reset the card.									*/
		/*----------------------------------------------------*/
		else
		{	
			OutChar ('F', RED);
			OutChar ('a', RED);
			OutChar ('i', RED);
			OutChar ('l', RED);
			OutChar ('u', RED);
			OutChar ('r', RED);
			OutChar ('e', RED);

			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, HARDWARE);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}
			LLDFailureDrp++;
	
			/*----------------------------------------*/
			/* The serial TxRequest returns a failure	*/
			/* when an invalid packet length is			*/
			/* received.  We do not want to reset		*/
			/* when this happens.							*/
			/*----------------------------------------*/
			RestoreFlag (oldflag);
#ifndef SERIAL
			LLDNeedReset = SET;
#else
			EnableRTS ();
#endif
			return (0);
		}
	}


	/*-------------------------------------------------*/
	/* Now send the data to the card starting with		*/
	/* the packetize header.									*/
	/*																	*/
	/* Be careful, this only works because PacketizeTx	*/
	/* is even.  Otherwise might need to fetch the		*/
	/* next byte to send.										*/
	/*																	*/
	/*-------------------------------------------------*/

	TxData ((unsigned_8 *)TxCmdPkt, sizeof (struct PacketizeTx), LLDTransferMode);

	if (LLDTxBridgePktsFlag)
	{	TxData (LLDBridgeDest, ADDRESSLENGTH, LLDTransferMode);
		TxData (LLDNodeAddress, ADDRESSLENGTH, LLDTransferMode);
	}

	/*-------------------------------------------------*/
	/* If its the Repeating LLD, then send repeating	*/
	/* destination, source, and hops-left first.			*/
	/*-------------------------------------------------*/
#ifdef WBRP
	if (LLDRepeatingWBRP)
	{
		TxData (WBRPBuf, RPLENGTH,	LLDTransferMode);
	}
#endif

	/*-------------------------------------------------*/
	/* Now send the immediate data.  Care must be		*/
	/* taken to only send an even number in case			*/
	/* we're in the word mode.									*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketSize = PktDesc->LLDImmediateDataLength;

	if (PacketSize & 0x01)
	{	LeftOverFlag = SET;
		LeftOverBytes [0] = PktDesc->LLDImmediateData [--PacketSize];
	}
	else
	{	LeftOverFlag = CLEAR;
	}

	/* Is there any immediate data to transmit? */
	if (PacketSize)
	{	
		TxData (PktDesc->LLDImmediateData, PacketSize, LLDTransferMode);
	}


	/*-------------------------------------------------*/	
	/* Now send the fragments, again taking care of	*/
	/* the odd byte.												*/
	/*																	*/
	/*-------------------------------------------------*/

	for (i = 0; i < PktDesc->LLDTxFragmentListPtr->LLDTxFragmentCount; i++)
	{	
		StartOfData = PktDesc->LLDTxFragmentListPtr->LLDTxFragList [i].FSDataPtr;
		PacketSize = PktDesc->LLDTxFragmentListPtr->LLDTxFragList [i].FSDataLen;

		/*-------------------------------------------------------*/
		/* With ODI, any of the fragments can be zero lengths.	*/
		/* If we get one, skip it.											*/
		/*-------------------------------------------------------*/
		if (PacketSize == 0)
			continue;

		if (LeftOverFlag)
		{	
			LeftOverBytes [1] = *StartOfData++;
			PacketSize--;
			TxData (LeftOverBytes, 2, LLDTransferMode);
		}

		if (PacketSize & 0x01)
		{	
			LeftOverFlag = SET;
			LeftOverBytes [0] = *(StartOfData + (--PacketSize));
		}
		else
		{	
			LeftOverFlag = CLEAR;
		}

		/* If the packet size is zero, don't call TxData. */
		if (PacketSize)
			TxData (StartOfData, PacketSize, LLDTransferMode);
	}


	/*-------------------------------------------------*/
	/* Now take care of any stranded byte.					*/
	/*																	*/
	/*-------------------------------------------------*/

	if (LeftOverFlag)
	{	
		LeftOverBytes [1] = 0;
		TxData (LeftOverBytes, 2, LLDTransferMode);
		if (PacketPadding)
			PacketPadding--;
	}


	/*-------------------------------------------------*/
	/* Send out the pad bytes if any.  For neatness,	*/
	/* the pad bytes will be set to 0, but this can		*/
	/* be taken out.												*/
	/*																	*/
	/*	Wait for EOT from the card.							*/
	/*																	*/
	/*-------------------------------------------------*/

	if (PacketPadding)
	{	
		SetMemory (LLDTxBuffer, 0, PacketPadding);
		TxData (LLDTxBuffer, PacketPadding, LLDTransferMode);
	}

	/* If EndOfTx() fails, LLDNeedReset gets set. */
	if (EndOfTx () == SUCCESS)
	{	
		LLDSent++;
	}

	NodeEntryPtr->TxPkt = PktDesc;
#ifndef SERIAL
	TxStartTime = LLSGetCurrentTime ();
	TxFlag = SET;
#else
	TxStartTime [NodeEntryPtr->LastPacketSeqNum] = LLSGetCurrentTime ();
	TxInProgress [NodeEntryPtr->LastPacketSeqNum] = SET;
#endif

	if (TxFlag)
	{	SaveControlValue |= WAKEUPBIT;
		SaveStatusValue |= STAYAWAKE;
	}
	ClearWakeupBit (SaveControlValue, SaveStatusValue);
	PopRFNCInt ();
#ifdef SERIAL
	EnableRTS ();
#endif

	InLLDSend = CLEAR;	
	LLDSniffCount = LLDTicksToSniff;

	RestoreFlag (oldflag);

	OutChar ('E', RED + WHITE_BACK);
	OutChar ('S', RED + WHITE_BACK);

	if ( (Qptr = RemoveReEntrantHead ( &ReEntrantQ )) != NIL )

	{
 		/*-------------------------------------------------------*/
		/* Send the packet out according to it's type.				*/
		/*																			*/
 		/* A zero value means it is a regular data packet, and a	*/
		/* non-zero value means it is a packetized command.  The	*/
		/* non-zero value is the packet length.						*/		
		/*-------------------------------------------------------*/
		if ( Qptr->PacketType == 0 )
		{
			OutChar ('R', BROWN);
			OutChar ('Q', BROWN);
			OutChar ('S', BROWN);
			LLDSend ( (struct LLDTxPacketDescriptor FAR *) Qptr->PacketPtr );
		}
		else
		{
			OutChar ('R', BROWN);
			OutChar ('Q', BROWN);
			OutChar ('P', BROWN);
			OutChar ('S', BROWN);
			LLDPacketizeSend ( (unsigned_8 *)Qptr->PacketPtr, Qptr->PacketType );
		}
		AddToReEntrantList ( &FreeReEntrantQ, Qptr );
	}

	return (0);
}



/***************************************************************************/
/*																									*/
/*	LLDTxFragments (PktDesc, TableEntry, BridgeMode, LLDEntryAddress)			*/
/*																									*/
/*	INPUT: 	PktDesc			-	Pointer to the TX Packet Descriptor				*/
/*				TableEntry		-	Index into NodeTable of entry for this node	*/
/*				BridgeMode		-	Bridge mode byte (if bridge or disc pkt).		*/
/*				LLDEntryAddr	-	Node Address of destination sending to.		*/
/*				LLDBridgeDest	-	Node Address sending to in the bridge hdr.	*/
/*																									*/
/*	OUTPUT:	Returns			-	Success or Failure									*/
/*																									*/
/*	DESCRIPTION:  This procedure will send fragmented packets.					*/
/*																									*/
/*	AP2:	The semaphore was taken in LLDSend() and will be given back			*/
/*			there.  This procedure will need to allocate netbufs to put			*/
/*			the fragments in.																	*/
/*			Because of the way poll final is used, it is important that no		*/
/*			other routine may add a transmit packet to the queue in the			*/
/*			middle of this procedure.  All routines in the LLD must take		*/
/*			the rl2TableSemaphore.  Only the romtest interface has risk.		*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDTxFragments (	struct LLDTxPacketDescriptor FAR *PktDesc, 
										unsigned_8 TableEntry, 
										unsigned_8 BridgeMode,
										unsigned_8 LLDEntryAddress [], 
										unsigned_8 LLDBridgeDest [] )

{	struct NodeEntry		*NodeEntryPtr;
	unsigned_16				 CurrTime;
	unsigned_16			 	 i;
	unsigned_16			    BufferOffset;
	struct FragStruc FAR *Fragment;
	unsigned_16				 PacketLength;
	unsigned_8				 NumberOfFragments;
	struct PacketizeTx	*TxPacket;
	unsigned_8				 SequenceNum;
	unsigned_16				 SendSize;
	unsigned_8				 Status;
	unsigned_8				 SaveControlValue;
	unsigned_8				 SaveStatusValue;

	OutChar ('B', GREEN);
	OutChar ('1', GREEN);


	NodeEntryPtr = &NodeTable [TableEntry];

	/*-------------------------------------------------*/
	/* Check if in-sync.  If not, add it to the timed	*/
	/* queue.														*/
	/*																	*/
	/*-------------------------------------------------*/

	if (LLDSyncState != SET)
	{	
		CurrTime = LLSGetCurrentTime ();
		LLDAddToTimedQueue (PktDesc, TableEntry, CurrTime, MAXOUTOFSYNCTIME,1);
		NodeEntryPtr->PktInProgressFlag = CLEAR;
		return (FAILURE);
	}
	
	SaveControlValue = ControlValue;
	SaveStatusValue = StatusValue;
	SetWakeupBit ();

	/*-------------------------------------------------*/
	/* Copy the immediate data into our buffer.			*/
	/* Then copy the fragments in also.						*/
	/*																	*/
	/*-------------------------------------------------*/

	/* Sanity check for the packet length. */
	if (PktDesc->LLDImmediateDataLength > MAX_PROX_PKT_SIZE)
	{
		LLDBadCopyLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('C', RED);
		OutChar ('p', RED);
		OutChar ('y', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(PktDesc->LLDImmediateDataLength >> 8), RED);
		OutHex ((unsigned_8)(PktDesc->LLDImmediateDataLength), RED);
#if INT_DEBUG
		__asm	int 3
#endif
		if (IsTxProximPacket (PktDesc))
		{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (PktDesc);
			}
			else
			{	LLSSendProximPktComplete (PktDesc, BAD_LENGTH);
			}
		}
		else
		{	LLSSendComplete (PktDesc);
		}
		return (FAILURE);
	}

#ifdef ODI
	fmemcpy ((unsigned_8 FAR *)LLDTxBuffer, PktDesc->LLDImmediateData, PktDesc->LLDImmediateDataLength);
#else
	CopyMemory (LLDTxBuffer, PktDesc->LLDImmediateData, PktDesc->LLDImmediateDataLength);
#endif
	BufferOffset = PktDesc->LLDImmediateDataLength;

	for (i = 0; i < PktDesc->LLDTxFragmentListPtr->LLDTxFragmentCount; i++)	
	{	
		Fragment = &(PktDesc->LLDTxFragmentListPtr->LLDTxFragList [i]);
		/* Sanity check for the packet length. */
		if (Fragment->FSDataLen > MAX_PROX_PKT_SIZE)
		{
			LLDBadCopyLen++;
			OutChar ('B', RED);
			OutChar ('a', RED);
			OutChar ('d', RED);
			OutChar ('C', RED);
			OutChar ('p', RED);
			OutChar ('y', RED);
			OutChar ('L', RED);
			OutChar ('e', RED);
			OutChar ('n', RED);
			OutHex ((unsigned_8)(Fragment->FSDataLen >> 8), RED);
			OutHex ((unsigned_8)(Fragment->FSDataLen), RED);
#if INT_DEBUG
		__asm	int 3
#endif
			if (IsTxProximPacket (PktDesc))
			{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (PktDesc);
				}
				else
				{	LLSSendProximPktComplete (PktDesc, BAD_LENGTH);
				}
			}
			else
			{	LLSSendComplete (PktDesc);
			}
			return (FAILURE);
		}

#ifdef ODI
		fmemcpy ((unsigned_8 FAR *)&LLDTxBuffer [BufferOffset], Fragment->FSDataPtr, Fragment->FSDataLen);
#else
		CopyMemory (&LLDTxBuffer [BufferOffset], Fragment->FSDataPtr, Fragment->FSDataLen);
#endif
		BufferOffset += Fragment->FSDataLen; 
	}


	/*-------------------------------------------------*/
	/* Get the length, but take out the MAC header		*/
	/* (12 bytes) and use this to calculate the number	*/
	/* of fragments requiRED to send.						*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketLength = PktDesc->LLDTxDataLength - 12;

	NumberOfFragments = (unsigned_8)(PacketLength / LLDFragSize);
	if (PacketLength % LLDFragSize)
		NumberOfFragments++;
	NodeEntryPtr->NumberOfFragments = NumberOfFragments;


	/*-------------------------------------------------*/
	/* Initialize the sequence number and offset of 	*/
	/* the buffer and send out each fragment.				*/
	/*																	*/
	/*-------------------------------------------------*/

	SequenceNum= NodeEntryPtr->DataSequenceNum;
	BufferOffset = 12;

	for (i = 1; i <= NumberOfFragments; i++)
	{	
		TxPacket = (struct PacketizeTx *) PacketizePktBuf;

		TxPacket->PTHeader.PHSeqNo = PacketSeqNum;
		TxPacket->PTHeader.PHCmd = DATA_COMMANDS_CMD;
		TxPacket->PTHeader.PHFnNo = TRANSMIT_DATA_FN;
		TxPacket->PTHeader.PHRsv = RESERVED;


		/*----------------------------------------------*/
		/* Set the Power Antenna byte and control byte.	*/
		/*																*/
		/*----------------------------------------------*/

		if (BridgeMode == 2)
		{	TxPacket->PTTxPowerAnt = 0x70;
		}
		else
		{	TxPacket->PTTxPowerAnt = 0x70 | 0x80;		
		}


		/*-------------------------------------------------*/
		/* Set the control byte.  									*/
		/*																	*/
		/*-------------------------------------------------*/
	
		/* No MAC ACK for bridge discovery packets.		*/
		
		if (BridgeMode == BS_DISCOVERY)
		{	if (LLDTxMode == TX_QFSK)
			{	TxPacket->PTControl = TX_QFSK;
			}
			else
			{	TxPacket->PTControl = TX_BFSK;
			}
		}
	
		/* No MAC ACK for bridged multicast packets.		*/
	
		else if ((BridgeMode == BS_BRIDGED) 
								&& (LLDBridgeDest[0] & 0x01))
		{	if (LLDTxMode == TX_QFSK)
			{	TxPacket->PTControl = TX_QFSK;
			}
			else
			{	TxPacket->PTControl = TX_BFSK;
			}
		}
	
		/* MAC ACK for directed multicast packets.		*/
	
		else if ((BridgeMode == BS_DIRECT) 
								&& (LLDEntryAddress [0] & 0x01))
		{	if (LLDTxMode == TX_QFSK)
			{	TxPacket->PTControl = TX_QFSK;
			}
			else
			{	TxPacket->PTControl = TX_BFSK;
			}
		}
	
		/* MAC ACK for directed or bridged unicast packets.	*/
	
		else
		{	
			if (LLDTxMode == TX_AUTO)
				TxPacket->PTControl = TX_MACACK | TX_AUTO_FRAG;
			else
				TxPacket->PTControl = (unsigned_8)(TX_MACACK | LLDTxMode);
		}
	

		if (i == NumberOfFragments)
		{	TxPacket->PTControl |= TX_POLLFINAL;
		}


		/*----------------------------------------------*/
		/* Set the LLD header bytes							*/
		/*																*/
		/*----------------------------------------------*/

		TxPacket->PTLLDHeader3 = SequenceNum;
		TxPacket->PTLLDHeader4 = (unsigned_8)(((NumberOfFragments - 1) << 4) | (i - 1));


		/*----------------------------------------------*/
		/* Update the sequence number and enter it in 	*/
		/* the map table for the transmit complete.		*/
		/*																*/
		/*----------------------------------------------*/

		if (i == NumberOfFragments)
		{	LLDMapTable [PacketSeqNum] = TableEntry;
			NodeEntryPtr->LastPacketSeqNum = PacketSeqNum;
		}

		/*----------------------------------------------------------------------*/
		/* Choose a sequence number for the next packet to go through LLDSend.	*/
		/*																								*/
		/*----------------------------------------------------------------------*/

		do
		{	PacketSeqNum = (unsigned_8)((PacketSeqNum + 1) & SEQNOMASK);
		} while (PacketSeqNum > MAX_DATA_SEQNO);



		OutChar ((unsigned_8)(i - 1 + '0'), BRIGHT_RED);

		
		/*----------------------------------------------*/
		/* Calculate the size of the packet.				*/
		/* Bridge packets have an extra 12 byte header.	*/
		/*	At the end, add the LLD (4 bytes) and MAC		*/
		/* (12 bytes) header length.							*/
		/*																*/
		/*----------------------------------------------*/

		if (PacketLength < LLDFragSize)
		{	SendSize = PacketLength;
		}
		else
		{	SendSize = LLDFragSize;
		}
		PacketLength -= SendSize;

		TxPacket->PTLLDHeader2 = 0;
		if (SendSize & 0x01)
		{	SendSize++;
			TxPacket->PTLLDHeader2 |= (0x01 & LLDPADBITS_MASK);
		}

		if (BridgeMode)
		{	TxPacket->PTLLDHeader2 |= LLDBRIDGEBIT;
			SendSize += 12;
		}

		SendSize += 16;
		TxPacket->PTPktLength [0] = (unsigned_8)(SendSize >> 8);
		TxPacket->PTPktLength [1] = (unsigned_8)(SendSize & 0x00FF);

		/*----------------------------------------------*/
		/* Add the size of the Packetize header minus	*/
		/* the 4 LLD header bytes.								*/
		/*																*/
		/*----------------------------------------------*/

		SendSize += (sizeof (struct PacketizeTx) - 4);


		/*----------------------------------------------*/
		/* Send out the fragment								*/
		/*																*/
		/*----------------------------------------------*/

		Status = LLDSendFragments (LLDTxBuffer, BufferOffset, SendSize, BridgeMode, LLDBridgeDest);
		if (Status != SUCCESS)
		{	
#ifndef SERIAL
			TxFlag = CLEAR;
			TxStartTime = LLSGetCurrentTime ();
#else
			TxStartTime [NodeEntryPtr->LastPacketSeqNum] = LLSGetCurrentTime ();
			TxInProgress [NodeEntryPtr->LastPacketSeqNum] = CLEAR;
#endif
			if (TxFlag)
			{	SaveControlValue |= WAKEUPBIT;
				SaveStatusValue |= STAYAWAKE;
			}
			ClearWakeupBit (SaveControlValue, SaveStatusValue);
			NodeEntryPtr->LastPacketSeqNum = SET;
			LLDMapTable [PacketSeqNum] = SET;

			if (Status == NAK)
			{
				OutChar ('N', RED);
				OutChar ('a', RED);
				OutChar ('k', RED);

				CurrTime = LLSGetCurrentTime ();
				LLDAddToTimedQueue (PktDesc, TableEntry, CurrTime, MAXNAKWAIT,1);
				NodeEntryPtr->PktInProgressFlag = CLEAR;
				if (TxFlag)
				{	SaveControlValue |= WAKEUPBIT;
					SaveStatusValue |= STAYAWAKE;
				}
				ClearWakeupBit (SaveControlValue, SaveStatusValue);
				return (FAILURE);
			}
			else if (Status == FAILURE)
			{
				OutChar ('F', RED);
				OutChar ('a', RED);
				OutChar ('i', RED);
				OutChar ('l', RED);
				OutChar ('u', RED);
				OutChar ('r', RED);
				OutChar ('e', RED);
		
				if (IsTxProximPacket (PktDesc))
				{	if (*((unsigned_32 *)((unsigned_8 *)PktDesc - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
					{	LLDFreeInternalTXBuffer (PktDesc);	
					}
					else
					{	LLSSendProximPktComplete (PktDesc, HARDWARE);
					}
				}
				else
				{	LLSSendComplete (PktDesc);
				}
				LLDFailureDrp++;

				/*----------------------------------------*/
				/* The serial TxRequest returns a failure	*/
				/* when an invalid packet length is			*/
				/* received.  We do not want to reset		*/
				/* when this happens.							*/
				/*----------------------------------------*/
#ifndef SERIAL
				LLDNeedReset = SET;
#endif
				return (FAILURE);
			}
		}
		
		BufferOffset += LLDFragSize;
	}

	NodeEntryPtr->TxPkt = PktDesc;
	NodeEntryPtr->PktInProgressFlag = SET;

#ifndef SERIAL
	TxStartTime = LLSGetCurrentTime ();
	TxFlag = SET;
#else
	TxStartTime [NodeEntryPtr->LastPacketSeqNum] = LLSGetCurrentTime ();
	TxInProgress [NodeEntryPtr->LastPacketSeqNum] = SET;
#endif

	if (TxFlag)
	{	SaveControlValue |= WAKEUPBIT;
		SaveStatusValue |= STAYAWAKE;
	}
	ClearWakeupBit (SaveControlValue, SaveStatusValue);
	return (0);
}





/***************************************************************************/
/*																									*/
/*	LLDSendFragments (*Buffer, BufferOffset, Length, BridgeMode,				*/
/*							*LLDBridgeDest)													*/
/*																									*/
/*	INPUT: 	Buffer			-	Pointer to start of the fragment to send		*/
/*				BufferOffset	-	Offset in buffer of start of data to send		*/
/*				Length			-	Fragment length + headers							*/
/*				BridgeMode		-	Bridge mode byte (if bridge or disc pkt).		*/
/*				LLDBridgeDest	-	Node Address sending to in the bridge hdr.	*/
/*																									*/
/*	OUTPUT:	Returns			-	SUCCESS/NAK/FAILURE									*/
/*																									*/
/*	DESCRIPTION:  This procedure will send one fragment to the card.			*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSendFragments (unsigned_8 Buffer [], unsigned_16 BufferOffset, unsigned_16 Length, unsigned_8 BridgeMode,
											unsigned_8 LLDBridgeDest [])

{	unsigned_8	Status;
	unsigned_16	PacketSize;
	unsigned_16 StartTime;
#ifndef SERIAL
	FLAGS			oldflag;
#endif

	OutChar ('B', BRIGHT_RED);
	OutChar ('2', BRIGHT_RED);


	/*-------------------------------------------------*/
	/* Request to transfer data.  If a NAK is received,*/
	/* clear it, create a window for interrupts to 		*/
	/* come in, and try to request again.					*/
	/*																	*/
	/* For serial cards, a NAK means we are out of		*/
	/* buffers.  We don't want to wait for a buffer		*/
	/* to free up.  When we return a NAK, the packet	*/
	/* gets put on the timed queue.							*/
	/*-------------------------------------------------*/
#ifndef SERIAL
	StartTime = LLSGetCurrentTime();
	do 
	{	Status = TxRequest (Length, LLDTransferMode);
		if (Status == FAILURE)
		{ 
			LLDNAKTime = 100;
			return (FAILURE);
		}
		else if (Status == NAK)
		{	ClearNAK ();
			OutChar ('F', GREEN);
			OutChar ('N', GREEN);
			OutChar ('A', GREEN);
			OutChar ('K', GREEN);

			oldflag = PreserveFlag ();
			EnableInterrupt ();
			PushRFNCInt ();
			PopRFNCInt ();
			RestoreFlag (oldflag);
		}
	} while ((Status != ACK)	&&
		 ((unsigned_16)(LLSGetCurrentTime () - StartTime) <= MAXWAITTIME));

	if (Status == NAK)
	{
		if (LLDNAKTime == 100)
			LLDNAKTime--;
		return (NAK);
	}

#else
	Status = TxRequest (Length, LLDTransferMode);
	if (Status != ACK)
	{	return (Status);
	}
#endif

	
	/*-------------------------------------------------*/
	/* First send out the Packetize command header.		*/
	/* Remember this only works because the size is an	*/
	/* even number of bytes.									*/
	/*																	*/
	/*-------------------------------------------------*/
	LLDNAKTime = 100;

	TxData (PacketizePktBuf, sizeof (struct PacketizeTx), LLDTransferMode);


	/*-------------------------------------------------*/
	/* If it's a bridge packet, send the bridge			*/
	/* header.														*/
	/*																	*/
	/* Send the MAC header which is located at the 		*/
	/* start of the buffer.										*/
	/*																	*/
	/*-------------------------------------------------*/

	if (BridgeMode)
	{	TxData (LLDBridgeDest, ADDRESSLENGTH, LLDTransferMode);
		TxData (LLDNodeAddress, ADDRESSLENGTH, LLDTransferMode);
	}

	TxData (Buffer, 12, LLDTransferMode);


	/*-------------------------------------------------*/
	/* Calculate the length of the fragment to send		*/
	/* and send it out.  Wait for the EOT from the		*/
	/* card and return the status.							*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketSize = Length - (sizeof (struct PacketizeTx) + 12);
	if (BridgeMode)
		PacketSize -= 12;	
	
	TxData (&Buffer [BufferOffset], PacketSize, LLDTransferMode);

	Status = EndOfTx ();
	return (Status);
}


/***************************************************************************/
/*																									*/
/*	LLDRawSend (Packet, Length)															*/
/*																									*/
/*	INPUT:	Packet	-	Pointer to the packetize packet.							*/
/*				Length	-	Packet Length.													*/
/*																									*/
/*	OUTPUT:	Returns	-	0	- Success													*/
/*								1	- Got a NAK													*/
/*								FF	- Fatal, transmit timed out							*/
/*																									*/
/*	DESCRIPTION: This routine will send raw packets to the card without		*/
/*		adding any headers.																	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDRawSend (unsigned_8 Packet [], unsigned_16 Length)

{
	return (LLDPacketizeSend (Packet, Length) );
}


/***************************************************************************/
/*																									*/
/*	LLDTxRoamNotify (*CurrMSTAAddr, *OldMSTAAddr, NotifyMsg, Length)				*/
/*																									*/
/*	INPUT:	CurrMSTAAddr-	Current MSTA Address										*/
/*				OldMSTAAddr	-	Old MSTA address.											*/
/*				NotifyMsg	-	Pointer to the message to send.						*/
/*				Length		-	Length of the message.									*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will send a notification packet to the			*/
/*		old access point to say that it has roamed.									*/
/*																									*/
/***************************************************************************/

void LLDTxRoamNotify (unsigned_8 CurrMSTAAddr [], unsigned_8 OldMSTAAddr [], unsigned_8 NotifyMsg [], unsigned_16 Length)

{	
	struct RoamNotify	*PacketBuf;
	unsigned_16			RoamPktSize;

   /*-------------------------------------------------*/
	/* If this is the first time we have been in sync, */
	/* then there is not old master to notify.         */
	/*																	*/
	/*-------------------------------------------------*/
	if (memcmp (OldMSTAAddr, "\0\0\0\0\0\0", ADDRESSLENGTH) == 0)
	{
		return;
	}

	OutChar ('N', BROWN);
	OutChar ('o', BROWN);
	OutChar ('t', BROWN);
	OutChar ('i', BROWN);
	OutChar ('f', BROWN);
	OutChar ('y', BROWN);

	/*-------------------------------------------------------------*/
	/* Set a timer to make shure we don't miss the response packet */
	/*																	*/
	/*-------------------------------------------------------------*/

	LLDRoamResponse = SET;
	LLDRoamResponseTmr = LLSGetCurrentTime ();

	PacketBuf = (struct RoamNotify *) SecPacketizePktBuf;


	/*-------------------------------------------------*/
	/* Calculate the Roam Packet size starting from 	*/
	/* the Ethernet Address to the end of the packet.	*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((sizeof (struct RoamNotify) - sizeof (struct PacketizeTx) + Length) < MIN_PKT_SIZE)
	{	RoamPktSize = MIN_PKT_SIZE;
	}
	else
	{	RoamPktSize = sizeof (struct RoamNotify) - sizeof (struct PacketizeTx) + Length + 12;
	}


	/*-------------------------------------------------*/
	/* Fill in the packetize command header.				*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketBuf->RNHeader.PTHeader.PHCmd = DATA_COMMANDS_CMD;
	PacketBuf->RNHeader.PTHeader.PHFnNo = TRANSMIT_DATA_FN;
	PacketBuf->RNHeader.PTHeader.PHSeqNo = ROAM_NOTIFY_SEQNO;
	PacketBuf->RNHeader.PTHeader.PHRsv = RESERVED;
	
	PacketBuf->RNHeader.PTControl = (unsigned_8)(TX_POLLFINAL | TX_MACACK | LLDTxMode);
	PacketBuf->RNHeader.PTTxPowerAnt = 0x70 | 0x80;
	PacketBuf->RNHeader.PTPktLength [0] = (unsigned_8) ((RoamPktSize + 12 + 4) >> 8);
	PacketBuf->RNHeader.PTPktLength [1] = (unsigned_8) ((RoamPktSize + 12 + 4) & 0x00FF);

	PacketBuf->RNHeader.PTLLDHeader1 = 0;
	PacketBuf->RNHeader.PTLLDHeader2 = LLDBRIDGEBIT | LLDROAMNOTIFYBIT;
	PacketBuf->RNHeader.PTLLDHeader3 = ROAM_NOTIFY_SEQNO;
	PacketBuf->RNHeader.PTLLDHeader4 = 0;


	/*-------------------------------------------------*/
	/* Fill in the Bridge and Ethernet headers.			*/
	/*																	*/
	/*-------------------------------------------------*/

	CopyMemory (PacketBuf->RNBDest,   CurrMSTAAddr,    ADDRESSLENGTH);
	CopyMemory (PacketBuf->RNBSource, LLDNodeAddress, ADDRESSLENGTH);
	CopyMemory (PacketBuf->RNEDest,   OldMSTAAddr, ADDRESSLENGTH);
	CopyMemory (PacketBuf->RNESource, LLDNodeAddress, ADDRESSLENGTH);

	PacketBuf->RNProtocolID [0] = 0x81;
	PacketBuf->RNProtocolID [1] = 0x37;


	/*-------------------------------------------------*/
	/* Fill in the IPX header.									*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketBuf->RNIPXIdent = 0xFFFF;
	PacketBuf->RNIPXLength [0] = 0x00;
	PacketBuf->RNIPXLength [1] = 0x2C;

	SetMemory (PacketBuf->RNIPXRsv, 0, 6);
	CopyMemory (PacketBuf->RNIPXDest, OldMSTAAddr, ADDRESSLENGTH);
	PacketBuf->RNIPXSktID [0] = 0x86;
	PacketBuf->RNIPXSktID [1] = 0x4B;
	SetMemory (PacketBuf->RNIPXRsv1, 0, 4);
	CopyMemory (PacketBuf->RNIPXSource, LLDNodeAddress, ADDRESSLENGTH);
	PacketBuf->RNIPXSktID1 [0] = 0x86;
	PacketBuf->RNIPXSktID1 [1] = 0x4B;


	/*-------------------------------------------------*/
	/* Fill in the data and send the packet.				*/
	/*																	*/
	/*-------------------------------------------------*/

	/* Sanity check for the copy length. */
	if (Length > MAX_PROX_PKT_SIZE)
	{
		LLDBadCopyLen++;
		OutChar ('B', RED);
		OutChar ('a', RED);
		OutChar ('d', RED);
		OutChar ('C', RED);
		OutChar ('p', RED);
		OutChar ('y', RED);
		OutChar ('L', RED);
		OutChar ('e', RED);
		OutChar ('n', RED);
		OutHex ((unsigned_8)(Length >> 8), RED);
		OutHex ((unsigned_8)(Length), RED);
#if INT_DEBUG
		__asm	int 3
#endif
		return;
	}

	CopyMemory (PacketBuf->RNData, NotifyMsg, Length);

	LLDPacketizeSend ((unsigned_8 *) PacketBuf, (unsigned_16)(sizeof (struct PacketizeTx) + RoamPktSize + 12));

}




/***************************************************************************/
/*																									*/
/*	HandleTransmitComplete (*Response)													*/
/*																									*/
/*	INPUT:	Response  -	Pointer to the response packet							*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will handle transmit complete responses.		*/
/*																									*/
/***************************************************************************/

void HandleTransmitComplete (struct PacketizeTxComplete *Response)

{	struct NodeEntry	*NodeEntryPtr;
	unsigned_8			Status;
	unsigned_8			TableEntry;
	struct LLDTxPacketDescriptor FAR *TxPacket;
	FLAGS					oldflag;
#ifdef WBRP
	unsigned_16			WBRPTxStartTime = TxStartTime;
#endif
	unsigned_8			*respPtr;
	unsigned_8			fragStatError;
	unsigned_8			CTSErrors;
	short	int			ii;


	LLDSentCompleted++;

#ifdef SERIAL
	DisableRTS ();
#endif
	oldflag = PreserveFlag ();
	DisableInterrupt ();


	/*----------------------------------------------*/
	/* Vaidate the sequence number so we don't have	*/
	/* an out of bounds access into the Map Table.	*/
	/*																*/
	/*----------------------------------------------*/

	if (Response->PTCSeqNo >= MAP_TABLE_SIZE 
			|| LLDMapTable[Response->PTCSeqNo] >= TOTALENTRY) 
	{
		OutChar ('B', YELLOW);
		OutChar ('O', YELLOW);
		OutChar ('U', YELLOW);
		OutChar ('N', YELLOW);
		OutChar ('D', YELLOW);
		OutChar ('R', YELLOW);
		OutChar ('Y', YELLOW);

		RestoreFlag (oldflag);
#ifdef SERIAL
		EnableRTS ();
#endif
		return;
	}

	/*-------------------------------------------------------*/
	/* If there is no node entry for this packet, that means */
	/* that we have re-used the node entry.  In this case,	*/
	/* we have already returned the packet to the upper		*/
	/* layer.																*/
	/*-------------------------------------------------------*/
	TableEntry = LLDMapTable [Response->PTCSeqNo];
	if (TableEntry == SET)
	{
		OutChar ('N', YELLOW);
		OutChar ('o', YELLOW);
		OutChar ('N', YELLOW);
		OutChar ('o', YELLOW);
		OutChar ('d', YELLOW);
		OutChar ('e', YELLOW);
		OutChar ('#', YELLOW);

		RestoreFlag (oldflag);
#ifdef SERIAL
		EnableRTS ();
#endif
		return;
	}

#if INT_DEBUG
	if (LLDMapTable [Response->PTCSeqNo] == SET)
		__asm int 3
#endif

	LLDMapTable [Response->PTCSeqNo] = SET;


	/*-----------------------------------------------*/
	/* To avoid the problem in LLDPoll() when TxFlag */
	/* is checked, an TxComplete interrupt might come*/
	/* in and causes the Reset Flag to be set.       */
	/* Solution:  Don't set TxStartTime to 0.        */
	/*     														 */
	/*-----------------------------------------------*/

	TxFlag = CLEAR;
	TxFlagCleared = SET;
#ifndef SERIAL
	TxStartTime = LLSGetCurrentTime ();
#else
	TxStartTime [Response->PTCSeqNo] = LLSGetCurrentTime ();
	TxInProgress [Response->PTCSeqNo] = CLEAR;
#endif

	NodeEntryPtr = &NodeTable [TableEntry];
	TxPacket = NodeEntryPtr->TxPkt;
	NodeEntryPtr->TxPkt = CLEAR;
	NodeEntryPtr->PktInProgressFlag = CLEAR;

	if ( TxPacket == 0 )
	{
		OutChar ('T', RED);
		OutChar ('2', RED);
	
		LLDNeedReset = SET;

		RestoreFlag (oldflag);
#ifdef SERIAL
		EnableRTS ();
#endif
		return;
	}


	/*-------------------------------------------------*/
	/* The transmit complete status packet supports		*/
	/* up to 8 sequence numbers/statuses.  This is for	*/
	/* fragmented packets. 	The seq no/status are in	*/ 
	/* reverse order.  The last packet transmitted is	*/
	/* the first entry in the list.							*/
	/*																	*/
	/*-------------------------------------------------*/

	if (Response->PTCEntryNum > 1)
	{
		/*-------------------------------------------------------------*/
		/* Validate the number of entries.  When we receive an invalid	*/
		/* number of entries, resend the packet.								*/
		/*																					*/
		/*-------------------------------------------------------------*/

		if ((Response->PTCEntryNum > 8) || (Response->PTCEntryNum != NodeEntryPtr->NumberOfFragments))
		{

			OutChar ('E', RED);
			OutChar ('R', RED);
			OutChar ('R', RED);
			OutChar ('C', RED);
			OutChar ('F', RED);

			if (LLDNoRetries)
			{
				NodeEntryPtr->NumberOfFragments = 0;
				NodeEntryPtr->DataSequenceNum++;
				LLDPullFromQueue (TableEntry);

				if (IsTxProximPacket (TxPacket))
				{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
					{	LLDFreeInternalTXBuffer (TxPacket);
					}
					else
					{	LLSSendProximPktComplete (TxPacket, INVALID_RESPONSE);
					}
				}
				else				
				{	LLSSendComplete (TxPacket);
				}
			}
			else
			{
				NodeEntryPtr->NumberOfFragments = 0;
				NodeEntryPtr->PktInProgressFlag = CLEAR;
				LLDSend (TxPacket);
			}
			return;
		}




		/*-------------------------------------------------------*/
		/* respPtr gets incremented twice, once for the status	*/
		/* number and once for the sequence number.  We are		*/
		/* ignoring all but the first sequence number.				*/
		/*																			*/
		/*-------------------------------------------------------*/
		NodeEntryPtr->NumberOfFragments = 0;
		respPtr = (unsigned_8 *) &(Response->PTCStatus);
		for (ii = Response->PTCEntryNum, fragStatError = 0; ii > 0; ii--, respPtr++)
		{
			/*----------------------------------------------------*/
			/* Clear the retry bit.  We don't want to roam when	*/
			/* we are transmitting fragments.							*/
			/*																		*/
			/*----------------------------------------------------*/
			Status = (unsigned_8)(*respPtr++ & ~TX_RETRY_ST);
			if ((Status == TX_CTS_ST) || (Status == TX_ACK_ST))
				fragStatError = 1;
		} 

		/*----------------------------------------------------------*/
		/* If the transmissions were all QFSKs or BFSKs, then this	*/
		/* is a successful packet transmission.  Otherwise it was a	*/
		/* CTS or an ACK error. Treat both CTS and ACK errors the	*/
		/* same by resending the packet.  Ignore the retry bit.		*/
		/*																				*/
		/*----------------------------------------------------------*/

		if (!fragStatError)
		{
			NodeEntryPtr->DataSequenceNum++;
			LLDPullFromQueue (TableEntry);

#ifdef AP2_DEBUG
			NodeEntryPtr->TXFragments++;
#endif
			LLDTxFragRetries = 0;
 			FramesXmitFrag++;
			FramesXmit++;
			if (IsTxProximPacket (TxPacket))
			{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (TxPacket);
				}
				else
				{	LLSSendProximPktComplete (TxPacket, SUCCESS);
				}
			}
			else				
			{	LLSSendComplete (TxPacket);
			}
		}
		else
		{
			/*----------------------------------------------------*/
			/* If we have already transmitted the same fragmented	*/
			/* packet TX_FRAG_RETRIES times, give up and return	*/
			/* the packet to the upper layer.							*/
			/*																		*/
			/*----------------------------------------------------*/
			if (LLDTxFragRetries++ == TX_FRAG_RETRIES || LLDNoRetries)
			{
				LLDTxFragRetries = 0;
				NodeEntryPtr->DataSequenceNum++;
				LLDPullFromQueue (TableEntry);

				if (IsTxProximPacket (TxPacket))
				{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
					{	LLDFreeInternalTXBuffer (TxPacket);
					}
					else
					{	LLSSendProximPktComplete (TxPacket, ACK_ERROR);
					}
				}
				else				
				{	LLSSendComplete (TxPacket);
				}
			}
			else
			{
				NodeEntryPtr->PktInProgressFlag = CLEAR;
				LLDSend (TxPacket);
			}
		}
		return;
	}


	Status = Response->PTCStatus;

	/*-------------------------------------------------*/
	/* Check for successful transmission in QFSK mode.	*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((Status & TX_STATUS_OK) && (Status & TX_QFSK_MODE))
	{	/*----------------------------------------------*/
		/* Make sure transmit packet entered.				*/
		/* Clear Fragmentation Discovery State.			*/
		/* Clear CTS error and BFSK modes.					*/
		/* Clear bridge discovery state.						*/
		/*																*/
		/*----------------------------------------------*/

		if (NodeEntryPtr->FragState != 3)
		{ 	NodeEntryPtr->FragState = 0;
		}

		NodeEntryPtr->CTSErrorCount = 0;
		NodeEntryPtr->BFSKFlag = CLEAR;
					
		/*----------------------------------------------*/
		/* If bridge discovery, set back to direct for	*/
		/* next packet attempt.									*/
		/* If not, then either success through a			*/
		/* bridge or direct. In either case, set the		*/
		/* node exist flag for more aggressive retries	*/
		/* when sending.											*/
		/*----------------------------------------------*/

		if (NodeEntryPtr->BridgeState == BS_DISCOVERY)
		{	NodeEntryPtr->BridgeState = BS_DIRECT;
			NodeEntryPtr->BridgeSuccess = 0;
		}
		else
		{	NodeEntryPtr->NodeExistsFlag = SET;
		}

		NodeEntryPtr->PktInProgressFlag = CLEAR;
		NodeEntryPtr->DataSequenceNum++;

		LLDPullFromQueue (TableEntry);
		FramesXmitQFSK++;
		FramesXmit++;

		if (IsTxProximPacket (TxPacket))
		{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (TxPacket);
			}
			else
			{	LLSSendProximPktComplete (TxPacket, SUCCESS);
			}
		}
		else				
		{	LLSSendComplete (TxPacket);
		}
	}

	/*-------------------------------------------------*/
	/* Check for successful transmission in BFSK mode.	*/
	/*																	*/
	/*-------------------------------------------------*/

	else if ((Status & TX_STATUS_OK) && ((Status & TX_QFSK_MODE) == 0))
	{

		/*----------------------------------------------*/
		/* Make sure transmit packet entered.				*/
		/* Check for roam based on BFSK and too many		*/
		/* retries bit in the packet set.  Need to be	*/
		/* in BFSK mode, talking to the Master, and not	*/
		/* already roaming.										*/
		/*																*/
		/*----------------------------------------------*/

		if (Response->PTCStatus & 0x40)
		{

			/*----------------------------------------------*/
			/* If we are the master, we don't want to roam.	*/
			/*----------------------------------------------*/
         if ((LLDRoamingState == NOT_ROAMING) && (LLDRoamingFlag == ROAMING_ENABLE) 
				&& (memcmp (LLDMSTAAddr, LLDNodeAddress, ADDRESSLENGTH) != 0))
			{	LLDRoam ();
			 	LLDRoamingState = ROAMING_STATE;
			}
		}

		/*-------------------------------------------*/
		/* If three consecutive transfers in BFSK		*/
		/* mode while attempted in AUTO, attempt the	*/
		/* next packets in BFSK mode.					  	*/
		/*															*/
		/*-------------------------------------------*/

		if (++(NodeEntryPtr->BFSKRcvCount) == 2)
		{	NodeEntryPtr->BFSKFlag = SET;
			NodeEntryPtr->BFSKRcvCount = 0;
		}

		/*----------------------------------------------*/
		/* If bridge discovery, set back to direct for	*/
		/* next packet attempt.									*/
		/* If not, then either success through a			*/
		/* bridge or direct. In either case, set the		*/
		/* node exist flag for more aggressive retries	*/
		/* when sending.											*/
		/* Clear bridge discovery state.					*/
		/* Reset CTS error counts.							*/
		/*															*/
		/*-------------------------------------------*/

		if (NodeEntryPtr->BridgeState == BS_DISCOVERY)
		{	NodeEntryPtr->BridgeState = BS_DIRECT;
			NodeEntryPtr->BridgeSuccess = SET;
		}
		else
		{	NodeEntryPtr->NodeExistsFlag = SET;
		}

		NodeEntryPtr->CTSErrorCount = 0;

		/*-------------------------------------------*/
		/* Give the packet back to the upper layer	*/
		/* and check if there is another to send.		*/
		/*															*/
		/*-------------------------------------------*/

		NodeEntryPtr->PktInProgressFlag = CLEAR;
		NodeEntryPtr->DataSequenceNum++;

		LLDPullFromQueue (TableEntry);
		FramesXmitBFSK++;
		FramesXmit++;
		if (IsTxProximPacket (TxPacket))
		{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (TxPacket);
			}
			else
			{	LLSSendProximPktComplete (TxPacket, SUCCESS);
			}
		}
		else				
		{	LLSSendComplete (TxPacket);
		}
	}


	/*-------------------------------------------------*/
	/* Check for ACK error.										*/
	/*																	*/
	/*-------------------------------------------------*/

	else if (((Status & TX_STATUS_OK) == 0) && ((Status & 0x0F) == 02))
	{	OutChar ('A', RED);
		OutChar ('C', RED);
		OutChar ('K', RED);

		OutHex(NodeEntryPtr->FragState,RED);

		/*----------------------------------------------*/
		/* Make sure transmit packet entered.				*/
		/*																*/
		/*----------------------------------------------*/

		if (NodeEntryPtr->BFSKFlag)
		{ 	NodeEntryPtr->BFSKPktCount = CLEAR;
			NodeEntryPtr->BFSKRcvCount = CLEAR;
		}

		/*-------------------------------------------*/
		/* If fragment discovery, set fragment state	*/
		/* If not fragmenting, set frag discovery		*/
		/*															*/
		/*-------------------------------------------*/

		if (NodeEntryPtr->FragState != 3)
		{	
			NodeEntryPtr->FragState++;
			if (NodeEntryPtr->FragState == 3)
				NodeEntryPtr->FragStartTime = LLSGetCurrentTime ();
#ifdef WBRP
			/*----------------------------------------------*/
			/* For the Wireless Backbone Repeating Protocol,*/
			/* we need to re-calculate the TSF.					*/
			/*----------------------------------------------*/
			if (LLDRepeatingWBRP)
			{
				WBRPUpdateTSF ( TxPacket, WBRPTxStartTime );
				RestoreFlag (oldflag);
#ifdef SERIAL
				EnableRTS ();
#endif
				return;
			}
#endif
		}
		NodeEntryPtr->PktInProgressFlag = CLEAR;
		FramesACKError++;
		if (LLDNoRetries)
		{
			NodeEntryPtr->DataSequenceNum++;
			LLDPullFromQueue (TableEntry);

			if (IsTxProximPacket (TxPacket))
			{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
				{	LLDFreeInternalTXBuffer (TxPacket);	
				}
				else
				{	LLSSendProximPktComplete (TxPacket, ACK_ERROR);
				}
			}
			else				
			{	LLSSendComplete (TxPacket);
			}
		}
		else
		{

		/*-------------------------------------------*/
		/* Attempt to resend the packet.					*/
		/*															*/
		/*-------------------------------------------*/

			LLDReSent++;
			LLDSend (TxPacket);
		}
	}


	/*-------------------------------------------------*/
	/*  Must be CTS Error.										*/
	/*																	*/
	/*-------------------------------------------------*/

	else if (((Status & TX_STATUS_OK) == 0) && ((Status & 0x0F) == 01))
	{	
		OutChar ('C', YELLOW);
		OutChar ('T', YELLOW);
		OutChar ('S', YELLOW);

		OutHex(NodeEntryPtr->CTSErrorCount,YELLOW);

		if (NodeEntryPtr->BFSKFlag)
		{ 	NodeEntryPtr->BFSKPktCount = CLEAR;
			NodeEntryPtr->BFSKRcvCount = CLEAR;
		}

		if (NodeEntryPtr->FragState != 3)
		{ 	NodeEntryPtr->FragState = 0;
		}
		
		CTSErrors = ++NodeEntryPtr->CTSErrorCount;
		FramesCTSError++;

		/***************************************************/
		/* Retry up to configured number of CTS retries		*/
		/* However, if the node may not really exitst,		*/
		/* then only attempt twice before doing a bridge	*/
		/* discovery.													*/
		/***************************************************/

		if((CTSErrors <= LLDCTSRetries) && (!((CTSErrors == 2) &&
				(NodeEntryPtr->NodeExistsFlag == CLEAR))))	
		{
			if (LLDNoRetries)
			{
				NodeEntryPtr->PktInProgressFlag = CLEAR;
				NodeEntryPtr->DataSequenceNum++;
				LLDPullFromQueue (TableEntry);

				if (IsTxProximPacket (TxPacket))
				{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
					{	LLDFreeInternalTXBuffer (TxPacket);
					}
					else
					{	LLSSendProximPktComplete (TxPacket, CTS_ERROR);
					}
				}
				else				
				{	LLSSendComplete (TxPacket);
				}
			}
			else
			{
				LLDReSent++;
				if (NodeEntryPtr->CTSErrorCount == 1)
				{
					NodeEntryPtr->PktInProgressFlag = CLEAR;
					LLDSend (TxPacket);
				}
				else
				{
					/*--------------------------------------------*/
					/* The TransmitQueue might have to be cleared */
					/*--------------------------------------------*/
						LLDAddToTimedQueue (TxPacket, TableEntry, LLSGetCurrentTime (), 2,0);

					NodeEntryPtr->PktInProgressFlag = CLEAR;
				}

			}
		}
		else
		{
#ifdef WBRP
			/*----------------------------------------------*/
			/* For the Wireless Backbone Repeating Protocol,*/
			/* we don't want to do a bridge discovery.		*/
			/*----------------------------------------------*/
			if (LLDRepeatingWBRP)
			{
				WBRPUpdateTSF ( TxPacket, WBRPTxStartTime );
				RestoreFlag (oldflag);
#ifdef SERIAL
				EnableRTS ();
#endif
				return;
			}
#endif
			if (!LLDPeerToPeerFlag) 		/* In client server environment */
			{
				NodeEntryPtr->PktInProgressFlag = CLEAR;
				NodeEntryPtr->DataSequenceNum++;
				LLDPullFromQueue (TableEntry);

				if (IsTxProximPacket (TxPacket))
				{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
					{	LLDFreeInternalTXBuffer (TxPacket);
					}
					else
					{	LLSSendProximPktComplete (TxPacket, CTS_ERROR);
					}
				}
				else				
				{	LLSSendComplete (TxPacket);
				}
			}
			else
			{
			/*-------------------------------------------*/
			/* Set bridge discovery 							*/
			/*-------------------------------------------*/

            NodeEntryPtr->BridgeStartTime = LLSGetCurrentTime ();
				NodeEntryPtr->BridgeState = BS_DISCOVERY;
				if (LLDNoRetries)
				{
					NodeEntryPtr->PktInProgressFlag = CLEAR;
					NodeEntryPtr->DataSequenceNum++;
					LLDPullFromQueue (TableEntry);

					if (IsTxProximPacket (TxPacket))
					{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
						{	LLDFreeInternalTXBuffer (TxPacket);
						}
						else
						{	LLSSendProximPktComplete (TxPacket, CTS_ERROR);
						}
					}
					else				
					{	LLSSendComplete (TxPacket);
					}
				}
	 			else
				{
					NodeEntryPtr->PktInProgressFlag = CLEAR;
					LLDReSent++;
					LLDSend (TxPacket);
				}
			}
		
		}

	}
	else  /* Treat this as an OK status */
	{	
		OutChar ('U', RED);
		OutChar ('n', RED);
		OutChar ('k', RED);
		OutChar ('n', RED);
		OutChar ('o', RED);
		OutChar ('w', RED);
		OutChar ('n', RED);
		OutChar ('A', RED);
		OutChar ('c', RED);
		OutChar ('k', RED);
		OutHex (Status, RED);

		/*----------------------------------------------*/
		/* Make sure transmit packet entered.				*/
		/* Clear Fragmentation Discovery State.			*/
		/* Clear CTS error and BFSK modes.					*/
		/* Clear bridge discovery state.						*/
		/*																*/
		/*----------------------------------------------*/

		if (NodeEntryPtr->FragState != 3)
		{	
			NodeEntryPtr->FragState = 0;
		}

		NodeEntryPtr->CTSErrorCount = 0;
		NodeEntryPtr->BFSKFlag = CLEAR;
					
		if (NodeEntryPtr->BridgeState == BS_DISCOVERY)
		{	NodeEntryPtr->BridgeState = BS_DIRECT;
			NodeEntryPtr->BridgeSuccess = 0;
		}

		NodeEntryPtr->PktInProgressFlag = CLEAR;
		NodeEntryPtr->DataSequenceNum++;

		LLDPullFromQueue (TableEntry);

		if (IsTxProximPacket (TxPacket))
		{	if (*((unsigned_32 *)((unsigned_8 *)TxPacket - sizeof(unsigned_32))) == INTERNAL_PKT_SIGNATURE)
			{	LLDFreeInternalTXBuffer (TxPacket);
			}
			else
			{	LLSSendProximPktComplete (TxPacket, SUCCESS);
			}
		}
		else				
		{	LLSSendComplete (TxPacket);
		}
	}


	RestoreFlag (oldflag);
#ifdef SERIAL
	EnableRTS ();
#endif
}



/***************************************************************************/
/*																									*/
/*	IsTxProximPacket (TxPacket)															*/
/*																									*/
/*	INPUT: 	TxPacket		-	Pointer to the TX Packet Descriptor to check.	*/
/*																									*/
/*	OUTPUT:	Returns 		-	Non-Zero value if it is a Proxim Packet.			*/
/*																									*/
/*	DESCRIPTION:  This procedure will check the transmit packet to see if	*/
/*		it is a Proxim Protocol packet.													*/
/*																									*/
/***************************************************************************/

unsigned_8 IsTxProximPacket (struct LLDTxPacketDescriptor FAR *TxPacket)

{	unsigned_8	Buffer [sizeof (struct ProximProtocolHeader)];
	unsigned_16	BytesLeft;
	unsigned_16	BytesToCopy;
	unsigned_16	BufferIndex;
	unsigned_8	FragmentNumber;

	if (TxPacket->LLDTxDataLength < sizeof (struct ProximProtocolHeader))
	{	return (0);
	}

	BytesLeft = sizeof (struct ProximProtocolHeader);

	if (TxPacket->LLDImmediateDataLength >= BytesLeft)
	{	BytesToCopy = BytesLeft;
		BytesLeft = 0;
	}
	else
	{	BytesToCopy = TxPacket->LLDImmediateDataLength;
		BytesLeft -= BytesToCopy;
	}

	CopyMemory (Buffer, TxPacket->LLDImmediateData, BytesToCopy);
	BufferIndex = BytesToCopy;

	FragmentNumber = 0;
	while (BytesLeft)
	{	if (TxPacket->LLDTxFragmentListPtr->LLDTxFragList [FragmentNumber].FSDataLen >= BytesLeft)
		{	BytesToCopy = BytesLeft;
			BytesLeft = 0;
		}
		else
		{	BytesToCopy = TxPacket->LLDTxFragmentListPtr->LLDTxFragList [FragmentNumber].FSDataLen;
			BytesLeft -= BytesToCopy;
		}

		CopyMemory (&Buffer [BufferIndex], TxPacket->LLDTxFragmentListPtr->LLDTxFragList [FragmentNumber].FSDataPtr, BytesToCopy);

		BufferIndex += BytesToCopy;
		FragmentNumber++;
	}

	return (IsProximPacket (Buffer));
}
		


/***************************************************************************/
/*																									*/
/*	IsProximPacket (Buffer)																	*/
/*																									*/
/*	INPUT: 	TxPacket		-	Pointer to the buffer to check.						*/
/*																									*/
/*	OUTPUT:	Returns 		-	Non-Zero value if it is a Proxim Packet.			*/
/*																									*/
/*	DESCRIPTION:  This procedure will check the buffer to see if it is a 	*/
/*		Proxim Protocol packet.																*/
/*																									*/
/***************************************************************************/

unsigned_8	HeaderBytes1 [8] = {0xAA, 0xAA, 0x03, 0x00, 0x20, 0xA6, 0x00, 0x01};
unsigned_8  HeaderBytes2 [4] = "prox";

unsigned_8 IsProximPacket (unsigned_8 *Buffer)
	
{	struct ProximProtocolHeader	*Header;

	Header = (struct ProximProtocolHeader *) Buffer;

	if (memcmp (HeaderBytes1, &Header->DSAP, 8))
	{	return (0);
	}

	if (memcmp (HeaderBytes2, Header->Identifier, 4))
	{	return (0);
	}

	return (1);
}
