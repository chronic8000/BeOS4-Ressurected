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
/*	$Header:   J:\pvcs\archive\clld\lldwbrp.c_v   1.6   06 Feb 1996 14:30:46   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lldwbrp.c_v  $										*/
/* 
/*    Rev 1.6   06 Feb 1996 14:30:46   JEAN
/* Comment changes.
/* 
/*    Rev 1.5   31 Jan 1996 13:24:12   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.4   14 Dec 1995 15:36:52   JEAN
/* Added header include file.
/* 
/*    Rev 1.3   17 Nov 1995 16:39:54   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.2   12 Oct 1995 11:51:48   JEAN
/* Include SERIAL header files only when compiling for SERIAL.
/* 
/*    Rev 1.1   16 Mar 1995 16:17:56   tracie
/* 
/*    Rev 1.0   22 Feb 1995 09:51:28   tracie
/* Initial revision.
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
#include "slldhw.h"
#endif
#include "lldsend.h"
#include "blld.h"

#include "rptable.h"
#include "wbrp.h"

/***************************************************************************/
/*                                                                         */
/*                          Global Variables                               */
/*                                                                         */
/***************************************************************************/
#ifdef MULTI
#include "multi.h"
extern struct LLDVarsStruct   *LLDVars;

#else

extern unsigned_8		LLDTxMode;
extern unsigned_8		PacketizePktBuf [128];
#endif

extern struct WBRPStatistics	WBRPStatus;

/***************************************************************************/
/*                                                                      	*/
/*  LLDTxWBRPPacket (*PacketPtr)															*/
/*                                                                      	*/
/*  INPUT :			PacketPtr	- pointer to a WBRP reply packet					*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION :	This routine will send a WBRPReply packet to the 			*/
/*						destination address using packetizesend().					*/
/*                                                                      	*/
/***************************************************************************/

void LLDTxWBRPPacket (struct WBRPPacket *PacketPtr)

{	
	unsigned_16	PacketSize;
	unsigned_8	ret;


	if (LLDSyncState == CLEAR)
	{
		return;
	}

	OutChar ('W', BROWN);
	OutChar ('B', BROWN);
	OutChar ('R', BROWN);
	OutChar ('P', BROWN);

	/*-------------------------------------------------*/
	/* Calculate data length + LLD header (4) for the  */
	/* packetize header length field.						*/
	/*																	*/
	/*	   LLD length (4) + 										*/
	/*    Length of the data (WBRP Packet)					*/
	/*																	*/
	/*-------------------------------------------------*/
	PacketSize = PacketPtr->length + 4;

	/*-------------------------------------------------*/
	/* Fill in the packetize command header.				*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketPtr->LLDHeader.PTHeader.PHCmd		= DATA_COMMANDS_CMD;
	PacketPtr->LLDHeader.PTHeader.PHFnNo 	= TRANSMIT_DATA_FN;
	PacketPtr->LLDHeader.PTHeader.PHSeqNo	= REPEAT_SEQ_NUMBER;
	PacketPtr->LLDHeader.PTHeader.PHRsv 	= RESERVED;
	
	/*-------------------------------------------------*/
	/* IMPORTANT:													*/
	/*																	*/
	/*		If PHFnNo == TRANSMIT_DATA_FN then we need to*/
	/*		check whether the packet is a DIRECTED packet*/
	/*    which mean if the packet is not Broadcast nor*/
	/*		MultiCast, then the MACACK bit must be SET. 	*/
	/*																	*/
	/*-------------------------------------------------*/

	if ( PacketPtr->destination[0] & 0x01 )
	{
		if (LLDTxMode == TX_QFSK)
		{	
			PacketPtr->LLDHeader.PTControl = TX_POLLFINAL | TX_QFSK;
		}
		else
		{	
			PacketPtr->LLDHeader.PTControl = TX_POLLFINAL;
		}
	}
	else
	{
		PacketPtr->LLDHeader.PTControl = (unsigned_8)(TX_POLLFINAL | TX_MACACK | LLDTxMode);
	}

	PacketPtr->LLDHeader.PTTxPowerAnt 		= 0x70 | 0x80;
	PacketPtr->LLDHeader.PTPktLength [0] 	= (unsigned_8) (PacketSize >> 8);
	PacketPtr->LLDHeader.PTPktLength [1] 	= (unsigned_8) (PacketSize & 0x00FF);

	/********************************************************************/
	/*																						  */
	/*	LLD Header definition														  */
	/*																						  */
	/*	LLDHeader1					Bit 0    = Set if WBRP function is on	  */
	/*																						  */
	/*	LLDHeader2					Bit 0-3 	= # of pad bytes for even frags */
	/*									Bit 4 	= WBRPFlag							  */
	/*									Bit 5 	= Undefine							  */
	/*									Bit 6 	= Repeat flag						  */
	/*									Bit 7		= Bridge flag						  */
	/*																						  */
	/*	LLDHeader3					Packet Sequence number (0-255)			  */
	/*																						  */
	/*	LLDHeader4					Bit 0-3	= Fragment number					  */
	/*									Bit 4-7	= Total number of fragments	  */
	/*																						  */
	/********************************************************************/

	PacketPtr->LLDHeader.PTLLDHeader1 		= LLDWBRPONBIT;
	PacketPtr->LLDHeader.PTLLDHeader2 		= LLDWBRPBIT;	
	PacketPtr->LLDHeader.PTLLDHeader3 		= 0;
	PacketPtr->LLDHeader.PTLLDHeader4 		= 0;


	/*-------------------------------------------------*/
	/* Calculate the entire packet size:	  				*/
	/*																	*/
	/*		sizeof (struct PacketizeTx) + 					*/
	/*    Length of the packet.								*/
	/*																	*/
	/*-------------------------------------------------*/
	PacketSize = PacketPtr->length + sizeof (struct PacketizeTx);

	ret = LLDPacketizeSend ((unsigned_8 *) PacketPtr, PacketSize);

	if ( ret )
		WBRPStatus.TxWBRPFails++;
}



/***************************************************************************/
/*                                                                      	*/
/*  LLDTxRepeatDataPacket (*PacketPtr, length)										*/
/*                                                                      	*/
/*  INPUT :			PacketPtr	- pointer to a Repeating Data Packet 			*/
/*						length		- the size of the entire packet.					*/
/*																									*/
/*  OUTPUT:																						*/
/*                                        	                              */
/*  DESCRIPTION :	This routine will send a WBRPReply packet to the 			*/
/*						destination address using packetizesend().					*/
/*                                                                      	*/
/***************************************************************************/

void LLDTxRepeatDataPacket (struct LLDRepeatingPacket *PacketPtr,
									 unsigned_16 length) 
{	
	unsigned_16	PacketSize;
	unsigned_8	ret;

	/*-------------------------------------*/
	/* Are we out of sync. at this point?  */
	/* If we are, don't send any packetize */
	/* packets.										*/
	/*-------------------------------------*/
	if (LLDSyncState == CLEAR)
	{
		return;
	}

	OutChar ('D', YELLOW);
	OutChar ('a', YELLOW);
	OutChar ('t', YELLOW);
	OutChar ('a', YELLOW);

	/*-----------------------------------------------------*/
	/* Calculate the length to put in the packetize header */
	/* length field.													 */
	/* 		Entire packet length - Packetize Header length*/
	/*       + LLD header length (4)								 */
	/*-----------------------------------------------------*/

	PacketSize = length - sizeof (struct PacketizeTx) + 4;


	/*-------------------------------------------------*/
	/* Fill in the packetize command header.				*/
	/*																	*/
	/*-------------------------------------------------*/

	PacketPtr->RPHeader.LLDHeader.PTHeader.PHCmd		= DATA_COMMANDS_CMD;
	PacketPtr->RPHeader.LLDHeader.PTHeader.PHFnNo 	= TRANSMIT_DATA_FN;
	PacketPtr->RPHeader.LLDHeader.PTHeader.PHSeqNo	= REPEAT_SEQ_NUMBER;
	PacketPtr->RPHeader.LLDHeader.PTHeader.PHRsv 	= RESERVED;
					  
	/*-------------------------------------------------*/
	/* IMPORTANT:													*/
	/*																	*/
	/*		If PHFnNo == TRANSMIT_DATA_FN then we need to*/
	/*		check whether the packet is a DIRECTED packet*/
	/*    which mean if the packet is not Broadcast nor*/
	/*		MultiCast, then the MACACK bit must be SET. 	*/
	/*																	*/
	/*-------------------------------------------------*/
	if ( PacketPtr->RPHeader.RPDest[0] & 0x01 )
	{
		if (LLDTxMode == TX_QFSK)
		{	
			PacketPtr->RPHeader.LLDHeader.PTControl = TX_POLLFINAL | TX_QFSK;
		}
		else
		{	
			PacketPtr->RPHeader.LLDHeader.PTControl = TX_POLLFINAL;
		}
	}
	else
	{
		PacketPtr->RPHeader.LLDHeader.PTControl = (unsigned_8)(TX_POLLFINAL | TX_MACACK | LLDTxMode);
	}

	PacketPtr->RPHeader.LLDHeader.PTTxPowerAnt 		= 0x70 | 0x80;
	PacketPtr->RPHeader.LLDHeader.PTPktLength [0] 	= (unsigned_8) (PacketSize >> 8);
	PacketPtr->RPHeader.LLDHeader.PTPktLength [1] 	= (unsigned_8) (PacketSize & 0x00FF);

	/********************************************************************/
	/*																						  */
	/*	LLD Header definition														  */
	/*																						  */
	/*	LLDHeader1					Bit 0    = Set if WBRP function is on	  */
	/*																						  */
	/*	LLDHeader2					Bit 0-3 	= # of pad bytes for even frags */
	/*									Bit 4 	= WBRPFlag							  */
	/*									Bit 5 	= Undefine							  */
	/*									Bit 6 	= Repeat flag						  */
	/*									Bit 7		= Bridge flag						  */
	/*																						  */
	/*	LLDHeader3					Packet Sequence number (0-255)			  */
	/*																						  */
	/*	LLDHeader4					Bit 0-3	= Fragment number					  */
	/*									Bit 4-7	= Total number of fragments	  */
	/*																						  */
	/* Preserve Fragmentation information in LLDHeader2 and zero out all*/
	/* other bits.																		  */
	/*																						  */
	/********************************************************************/

	PacketPtr->RPHeader.LLDHeader.PTLLDHeader1 = LLDWBRPONBIT;
	PacketPtr->RPHeader.LLDHeader.PTLLDHeader2 &= (unsigned_8) (0x07);

	ret = LLDPacketizeSend ((unsigned_8 *) PacketPtr, length);

	if ( ret )
		WBRPStatus.TxDataFails++;
}





/***************************************************************************/
/*																									*/
/*               Packetized Commands for Armadillo Project						*/
/*																									*/
/***************************************************************************/


/***************************************************************************/
/*																									*/
/*	LLDGetFrequencyTable	()																	*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION: This routine will send a Get Frequency Table packet to the */
/*					 card.  It allows the LLD to retrieve the current table of  */
/* 				 frequency in use.														*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDGetFrequencyTable (void)

{	
	struct PacketizeGetFreqTbl	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeGetFreqTbl *) PacketizePktBuf;

	CmdPktPtr->PGFTHeader.PHCmd	= INIT_COMMANDS_CMD;
	CmdPktPtr->PGFTHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PGFTHeader.PHFnNo	= GET_FREQUENCY_FN;
	CmdPktPtr->PGFTHeader.PHRsv	= RESERVED;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeGetFreqTbl)))
	{	
		return (FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDSetFreqTable (unsigned_8 *FreqTblPtr)											*/
/*																									*/
/*	INPUT:		 FreqTblPtr - Pointer to the new frequency table  				*/
/*																									*/
/*	OUTPUT:		 Success or Failure														*/
/*																									*/
/*	DESCRIPTION: This routine will modify the table of frequencies.		   */
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetFreqTable (unsigned_8 *FreqTblPtr)

{	
	struct PacketizeSetFreqTbl	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetFreqTbl *) PacketizePktBuf;

	CmdPktPtr->PSFTHeader.PHCmd	= INIT_COMMANDS_CMD;
	CmdPktPtr->PSFTHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSFTHeader.PHFnNo	= SET_FREQUENCY_FN;
	CmdPktPtr->PSFTHeader.PHRsv	= RESERVED;

	CmdPktPtr->PSFTLength			= 0x35;
	memcpy (CmdPktPtr->PSFTFreqTbl, FreqTblPtr, 0x35);

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetFreqTbl)))
	{	
		return (FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDSetHopStatistics (OnOffFlag)  													*/
/*																									*/
/*	INPUT:		 OnOffFlag - 1 : Enable the Hop Statistics						*/
/*									 0 : Disable the Hop Statistics						*/
/*																									*/
/*	OUTPUT:		 Success or Failure														*/
/*																									*/
/*	DESCRIPTION: This routine will enable or disable the Hop Statistics.    */
/*					 When the Hop Statistics is enabled, the Hop Statistics     */
/*					 packet is returned to the driver at the end of each hop.	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetHopStatistics (unsigned_8 OnOffFlag)

{	
	struct PacketizeSetHopStat	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetHopStat *) PacketizePktBuf;

	CmdPktPtr->PSHSHeader.PHCmd	= INIT_COMMANDS_CMD;
	CmdPktPtr->PSHSHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSHSHeader.PHFnNo	= SET_HOPSTAT_FN;
	CmdPktPtr->PSHSHeader.PHRsv	= RESERVED;
	CmdPktPtr->PSHSHopStatOnOff	= OnOffFlag;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetHopStat)))
	{	
		return (FAILURE);
	}
	return (SUCCESS);
}


/***************************************************************************/
/*																									*/
/*	LLDSetUARTBaudRate (BaudRateCode)													*/
/*																									*/
/*	INPUT:		 BaudRateCode : 00h = 1200												*/
/*										 01h = 2400												*/
/*										 02h = 4800												*/
/*										 03h = 9600												*/
/*										 04h = 19200											*/
/*										 05h = 38400											*/
/*										 06h = 57600											*/
/*										 07h = 115200											*/
/*																									*/
/*	OUTPUT:		 Success or Failure														*/
/*																									*/
/*	DESCRIPTION: This routine will enable or disable the Hop Statistics.    */
/*					 When the Hop Statistics is enabled, the Hop Statistics     */
/*					 packet is returned to the driver at the end of each hop.	*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetUARTBaudRate (unsigned_8 BaudRateCode)

{	
	struct PacketizeSetBaudRate	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetBaudRate *) PacketizePktBuf;

	CmdPktPtr->PSBRHeader.PHCmd	= INIT_COMMANDS_CMD;
	CmdPktPtr->PSBRHeader.PHSeqNo = PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSBRHeader.PHFnNo	= SET_UART_BAUDRATE_FN;
	CmdPktPtr->PSBRHeader.PHRsv	= RESERVED;
	CmdPktPtr->PSBRBaudRate			= BaudRateCode;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetBaudRate)))
	{	
		return (FAILURE);
	}
	return (SUCCESS);
}



/***************************************************************************/
/*																									*/
/*	LLDSetMaxSyncDepth (SyncDepth)														*/
/*																									*/
/*	INPUT:		 SyncDepth - Max. Sync Depth to write to EEPROM					*/
/*																									*/
/*	OUTPUT:		 Success or Failure														*/
/*																									*/
/*	DESCRIPTION: This routine will set the maximum sync depth of the RFNC   */
/*					 assigned by Proxim manufacturing and is written to EEPROM  */
/*					 as a permanent value.													*/
/*																									*/
/***************************************************************************/

unsigned_8 LLDSetMaxSyncDepth (unsigned_8 SyncDepth)

{	
	struct PacketizeSetMaxSyncD	*CmdPktPtr;

	CmdPktPtr = (struct PacketizeSetMaxSyncD *) PacketizePktBuf;

	CmdPktPtr->PSMSDHeader.PHCmd		= DIAG_COMMANDS_CMD;
	CmdPktPtr->PSMSDHeader.PHSeqNo	= PacketSeqNum;
	IncPacketSeqNum ();
	CmdPktPtr->PSMSDHeader.PHFnNo		= SET_MAX_SYNCDEPTH_FN;
	CmdPktPtr->PSMSDHeader.PHRsv		= RESERVED;
	CmdPktPtr->PSMSDSyncDepth			= SyncDepth;

	if (LLDPacketizeSend ((unsigned_8 *)CmdPktPtr, sizeof(struct PacketizeSetMaxSyncD)))
	{	
		return (FAILURE);
	}
	return (SUCCESS);
}
