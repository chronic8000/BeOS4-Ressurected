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
/*	$Header:   J:\pvcs\archive\clld\lld.h_v   1.23   11 Aug 1998 11:45:08   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\lld.h_v  $											*/
/* 
/*    Rev 1.23   11 Aug 1998 11:45:08   BARBARA
/* LLSGetCurrentTimeLONG declaration added.
/* 
/*    Rev 1.22   03 Jun 1998 15:01:50   BARBARA
/* Proxim protocol definitions added.
/* 
/*    Rev 1.21   24 Mar 1998 09:20:34   BARBARA
/* LLD Ping changes added.
/* 
/*    Rev 1.20   12 Sep 1997 17:04:40   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.19   15 Jan 1997 17:12:04   BARBARA
/* 
/* 
/*    Rev 1.18   27 Sep 1996 09:32:00   JEAN
/* 
/*    Rev 1.17   28 Jun 1996 17:36:44   BARBARA
/* One Piece PCMCIA code for NDIS3 has been added.
/* 
/*    Rev 1.16   14 Jun 1996 16:53:44   JEAN
/* Modified a function prototype and added the CSS label.
/* 
/*    Rev 1.15   16 Apr 1996 11:41:22   JEAN
/* Added the FLAGS typedef.
/* 
/*    Rev 1.14   08 Mar 1996 19:27:16   JEAN
/* Changed the typedef for int to short for NDIS.  Put the MAX_IMMED
/* define back to the size of a transmit buffer.
/* 
/*    Rev 1.13   04 Mar 1996 15:07:24   truong
/* Changed unsigned_16 type definition to unsigned short.
/* 
/*    Rev 1.12   06 Feb 1996 14:33:48   JEAN
/* Added a define.
/* 
/*    Rev 1.11   31 Jan 1996 13:40:50   JEAN
/* Reduced the size of MAX_IMMED from 1550 to 12, also added
/* duplicate header include detection.
/* 
/*    Rev 1.10   14 Dec 1995 15:39:40   JEAN
/* Moved NUMCARDS from lld.c to lld.h.
/* 
/*    Rev 1.9   17 Nov 1995 16:44:06   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.8   12 Oct 1995 12:06:04   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.7   20 Jun 1995 15:53:12   hilton
/* Added support for protcol promiscuous
/* 
/*    Rev 1.6   13 Apr 1995 08:51:30   tracie
/* xromtest version
/* 
/*    Rev 1.5   16 Mar 1995 16:20:20   tracie
/* Added support for ODI
/*																									*/
/***************************************************************************/

#ifndef LLD_H
#define LLD_H

#include <OS.h>
#include <KernelExport.h>

#ifdef NDIS_MINIPORT_DRIVER
#include <ndis.h>
#endif

/*--------------------------------------------*/
/* Exclude the PCMCIA code for Serial builds. */
/*--------------------------------------------*/
#ifndef SERIAL
#define CSS 1
#endif

#define	NUMCARDS		2


/*-------------------------------------------------------------------------*/
/* Important Note:																		   */
/*																									*/
/*		This file defines packet descriptor structures specific to the 	   */
/*    ODI driver only.																		*/
/*																									*/
/*-------------------------------------------------------------------------*/
#ifdef	ODI
#define	FAR				__far
#else
#define	FAR
#endif
#define	NIL				( (void *) 0 )

/***************************************************************************/
/*																								 	*/
/* These size declarations are made so that the code can be easily 		 	*/
/* ported across different compilers and processors.  These should be		*/
/* changed to match your configuration.												*/
/*																									*/
/***************************************************************************/

typedef	uint8 unsigned_8;

#ifndef MULTI
typedef	uint16 unsigned_16;
#else
typedef	uint16 unsigned_16;
#endif
typedef	uint32	unsigned_32;

typedef	int8				signed_8;
typedef	int16				signed_16;
typedef	int32				signed_32;

/*----------------------------------------------------------*/
/*																				*/
/* This typedef is used save and restore processor flags.	*/
/*																				*/
/*----------------------------------------------------------*/

typedef cpu_status FLAGS;



/***************************************************************************/
/*																									*/
/*  These constants are used to define the maximum sizes of the various		*/
/*  parts of the packet descriptors.  These can be changed to match your	*/
/*  needs.																						*/
/*																									*/
/***************************************************************************/

#define	MAX_IMMED		1520 	// This should maintain 8 byte alignment
#define	MAX_FRAGS	  	1 		// We never use more than one fragment



/***************************************************************************/
/*																								 	*/
/* Equates for the OpMode variable.													 	*/
/*																								 	*/
/***************************************************************************/

#define	OP_NORMAL		0
#define	OP_PROMISC		1
#define	OP_PROTOCOL		2


/***************************************************************************/
/*																								 	*/
/* Equates for roaming.																	 	*/
/*																								 	*/
/***************************************************************************/

#define	ROAMING_DISABLED	0
#define	ROAMING_ENABLE		0

#define	NOT_ROAMING			0
#define	ROAMING_STATE		1
#define	ROAMING_DELAY		2


/***************************************************************************/
/*																								 	*/
/* Equates for LLSSendProximPktComplete.											 	*/
/*																								 	*/
/***************************************************************************/

#define	SUCCESS				0
#define	CTS_ERROR			1
#define	ACK_ERROR			2
#define	BAD_LENGTH			3
#define	QUEUE_FULL			4
#define	SEND_DISABLED		5
#define	DROPPED				6
#define	OUT_OF_SYNC			7
#define	HARDWARE				8
#define	INVALID_RESPONSE	9
#define	TIMEOUT				10
#define	NO_CARD				11
#define	RESET_RFNC			12
#define	IN_PROGRESS			0xFF

#define FAR  


/***************************************************************************/
/*																								 	*/
/* These are the structures that must be defined to link to the LLD.		 	*/
/*																								 	*/
/***************************************************************************/

struct FragStruc
{	
	unsigned_8	 FAR *FSDataPtr;		  	 /* Ptr to the fragment data	*/
	unsigned_16			FSDataLen;			 /* Length of the data			*/
};


/*-------------------------------------------------------------------------*/
/*																									*/
/* ODI Transmit Packet descriptor structure definition  							*/
/*																									*/
/*-------------------------------------------------------------------------*/

struct LLDTxPacketDescriptor
{	
	unsigned_8							LLDTxdriverWS[6];
	unsigned_16							LLDTxDataLength;					/* Length of total TX data		*/
	struct LLDTxFragmentList FAR	*LLDTxFragmentListPtr;			/* Ptr to list of fragments	*/
	unsigned_16							LLDImmediateDataLength;			/* Length of immediate data	*/
	unsigned_8							LLDImmediateData [MAX_IMMED];	/* Immediate data					*/
};


struct LLDTxFragmentList
{	
	unsigned_16				LLDTxFragmentCount;			/* Num of fragment descriptors	*/
	struct FragStruc		LLDTxFragList [MAX_FRAGS];	/* List of fragments					*/
};


/*-------------------------------------------------------------------------*/
/*																									*/
/* ODI Receive Packet descriptor structure definition  							*/
/*																									*/
/*-------------------------------------------------------------------------*/

struct LLDRxPacketDescriptor
{	
	unsigned_8				LLDRxDriverWS [8];
	unsigned_8				LLDRxReserved [36];
	unsigned_16				LLDRxFragCount;  				/* Num of fragment descriptors	*/
	struct FragStruc		LLDRxFragList [MAX_FRAGS];	/* List of fragments					*/
};



/*-------------------------------------------------------------------------*/
/*																									*/
/* Proxim Protocol Packet structure Definition										*/
/*																									*/
/*-------------------------------------------------------------------------*/

struct ProximProtocolHeader
{
	unsigned_8				DestAddr [6];
	unsigned_8				SourceAddr [6];
	unsigned_8				Length [2];
	unsigned_8				DSAP;
	unsigned_8				SSAP;
	unsigned_8				Control;
	unsigned_8				ProtocolID [5];
	unsigned_8				Identifier [4];
	unsigned_8				PacketType;
	unsigned_8				Version;
	unsigned_8				SubType[2];
};




/**************************************************************************/
/*																								  */
/* Prototype definitions of LLD procedures called by the LLS.				  */
/*																								  */
/**************************************************************************/

unsigned_16		LLDInit (void);
void 				LLDIsr (void);
unsigned_16		LLDReceive (struct LLDRxPacketDescriptor FAR *PktDesc, unsigned_16 Offset, unsigned_16 Length);
unsigned_16		LLDRawReceive (struct LLDRxPacketDescriptor FAR *PktDesc, unsigned_16 Offset, unsigned_16 Length);
unsigned_8		LLDSend (struct LLDTxPacketDescriptor FAR *PktDesc);
unsigned_8		LLDSendProximPkt (struct LLDTxPacketDescriptor FAR *PktDesc);
unsigned_8 	LLDRawSend (unsigned_8 Packet [], unsigned_16 Length);
unsigned_8		LLDReset (void);
void 				LLDPoll (void);
unsigned_8		LLDMulticast (unsigned_8 OnOrOff);
unsigned_8		LLDPromiscuous (unsigned_8 OnOrOff);
unsigned_8		LLDStop (void);
void				LLDOutOfStandby (void);


/**************************************************************************/
/*																								  */
/* Prototype definitions of LLS procedures called by the LLD.				  */
/*																								  */
/**************************************************************************/

void				LLSReceiveLookAhead (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 Status, unsigned_16 RSSI);
void 				LLSPingReceiveLookAhead (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 Status, unsigned_16 RSSI);
void				LLSRawReceiveLookAhead (unsigned_8 Buffer[], unsigned int Length);
unsigned_8		LLDPacketizeSend (unsigned_8 *PtrToPkt, unsigned_16 PktLength);
unsigned_16		LLSSendComplete (struct LLDTxPacketDescriptor FAR *PktDesc);
unsigned_16		LLSSendProximPktComplete (struct LLDTxPacketDescriptor FAR *PktDesc, unsigned_8 Status);
unsigned_16		LLSRawSendComplete (unsigned_8 *Buffer);
unsigned_8		LLSRegisterInterrupt (unsigned_8 IntNum, void (*CallBackFunc) ());
unsigned_8		LLSDeRegisterInterrupt (unsigned_8 IntNum);
unsigned_16		LLSGetCurrentTime (void);
void 				SendLLDPingResponse (unsigned_8 *Buffer, unsigned_16 Length, unsigned_16 SubType);
#ifdef  NDIS_MINIPORT_DRIVER
ULONGLONG		LLSGetCurrentTimeLONG(void);
unsigned_32		LLSGetTimeInMicroSec (void);
#else
unsigned_32		LLSGetCurrentTimeLONG(void);
#endif
void				fmemcpy (unsigned_8 FAR *, unsigned_8 FAR *, unsigned_16);
void 				InitLLDVars (unsigned_8 Card);

#ifdef PROTOCOL
void 				LLSReceiveProt (unsigned char *Buffer, unsigned int Length, unsigned long TimeStamp);
#endif

#endif /* LLD_H */
