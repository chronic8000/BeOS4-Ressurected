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
/*							(c) Copyright PROXIM, INC. 1995								*/
/*									All Rights Reserved										*/
/*																									*/
/***************************************************************************/

/***************************************************************************/
/*																									*/
/*	$Header:   J:\pvcs\archive\clld\wbrp.h_v   1.3   06 Feb 1996 14:42:06   JEAN  $																						*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\wbrp.h_v  $											*/
/* 
/*    Rev 1.3   06 Feb 1996 14:42:06   JEAN
/* Fixed some function prototypes.
/* 
/*    Rev 1.2   31 Jan 1996 13:55:00   Jean
/* Added duplicate header include detection.
/* 
/*    Rev 1.1   12 Oct 1995 13:44:48   JEAN
/* Added missing copyright and edit history.
/* 
/*    Rev 1.0   22 Feb 1995 09:52:24   tracie
/* Initial revision.
/***************************************************************************/

#ifndef 	WBRP_H
#define	WBRP_H

/*-------------------------------*/
/* Bit 0 of LLDHeader1			   */
/*-------------------------------*/
#define	LLDWBRPONBIT		0x01	

/*-------------------------------*/
/* Bit 4 of LLDHeader2			   */
/*-------------------------------*/
#define	LLDWBRPBIT			0x10

#define	MAXWBRPWAITTIME	( 18.2 * 30 )

/***************************************************************************/
/*                                                                         */
/*                              CONSTANTS                                  */
/*                                                                         */
/***************************************************************************/
#define	PASSITUP									1
#define	DONOT_PASSITUP							0

#define	WBRP_REQUEST							1
#define	WBRP_REPLY								2

#define	WBRP_SECONDS_BETWEEN_BROADCASTS	30
#define	WBRP_TICKS_BETWEEN_BROADCASTS		(TPS * WBRP_SECONDS_BETWEEN_BROADCASTS)
#define	WBRP_AGING_PERIOD_TICKS	 			(TPS * 2)

#define	WBRP_HALF_BROADCAST_PERIOD			(WBRP_TICKS_BETWEEN_BROADCASTS / 2)
#define	WBRP_SECONDARY_ROUTE_DELAY			((unsigned_32) (18.2 * 5))

#define	MAX_ENTRY_AGE_SECONDS				120
#define	MAX_ENTRY_AGE_TICKS					((unsigned_32) (18.2 * MAX_ENTRY_AGE_SECONDS))

#define	MAX_NEIGHBOR_AGE_SECONDS			120
#define	MAX_NEIGHBOR_AGE_TICKS				((unsigned_32) (18.2 * MAX_NEIGHBOR_AGE_SECONDS))

#define	INITIAL_TSF								20
#define	TSF_WEIGHT								25

/*---------------------------------------------------------*/
/* Constants for Transmit Success Factor (TSF) Calculation */
/*---------------------------------------------------------*/
#define	MAX_ACCEPTABLE_DELAY					TPS * 4
#define	MAX_TXDELAY								TPS * 4

#define	SWITCHING_THRESHOLD					25
#define	MINIMUM_ROUTING_METRIC				70


/*----------------------------------------*/
/* E8023HEADERLEN : includes the length of*/
/* the Destination Address, length of the */
/* Source Address and the size of the     */
/* packet length field.							*/
/*      6 + 6 + 2 = 14							*/
/*----------------------------------------*/
#define	E8023HEADERLEN							14
#define	WBRPBUFFERCNT							20



/*----------------------------------------*/
/* Repeater Information Table	Format		*/
/*----------------------------------------*/
struct RepeaterEntry 
{
	unsigned_8 	repeaterAddress[ADDRESSLENGTH];
	unsigned_8 	numberOfHops;
	unsigned_8 	routingMetric;
	unsigned_8 	nextHopRepeater[ADDRESSLENGTH];
};



/*----------------------------------------*/
/* WBRP Packet Format							*/
/*----------------------------------------*/
struct WBRPPacket 
{
	struct PacketizeTx	LLDHeader;						/* Transmit Data Header	*/
	unsigned_8				destination[ADDRESSLENGTH];/* Destination address	*/
	unsigned_8				source[ADDRESSLENGTH];		/* Source address			*/
	unsigned_16				length;							/* starts from dest.addr*/
	unsigned_16				operation;						/* 2-Request, 1-Reply	*/
	unsigned_16				numofentries;					/* number of entries		*/
	struct RepeaterEntry entry[MAX_REPEATERS];		/* Repeater Info Table	*/
};

struct WBRPPacketBuf
{
	struct WBRPPacket		packet;
	unsigned_8				bufstatus;
};


/*----------------------------------------*/
/* 		WB Repeating Packet Header			*/
/*----------------------------------------*/
struct RepeatingHdr
{
	struct PacketizeTx	LLDHeader;						/* Packetized Cmd Header */
	unsigned_8				RPDest  [ADDRESSLENGTH];	/* Repeating Destination */
	unsigned_8				RPSource[ADDRESSLENGTH];	/* Repeating Source		 */
	unsigned_8				hopsLeft;						/* # of Hops Left			 */
	unsigned_8				reserved;			  			/* To make it even 		 */
};																


/*----------------------------------------*/
/* DATA Packets:  LLD Packet Format			*/
/*----------------------------------------*/
struct LLDRepeatingPacket 
{
	struct RepeatingHdr	RPHeader;
	unsigned_8				destination[ADDRESSLENGTH];/* Ultimate Destination  */
	unsigned_8				source[ADDRESSLENGTH];		/* Original Source Addr. */
	unsigned_8			  	ImmediateData [1550];		/* Data 						 */
};


/*----------------------------------------*/
/*   Statistics of the LLD Data Transfer	*/
/*----------------------------------------*/
struct WBRPStatistics 
{
	unsigned_16				numOfWBRPBcast;
	unsigned_16				numOfWBRPTx;
	unsigned_16				numOfWBRPRx;
	unsigned_16				numOfDataRx;
	unsigned_16				numOfRepeatPkt;
	unsigned_16				TxDataFails;
	unsigned_16				TxWBRPFails;
};


#define RPLENGTH	( sizeof(struct RepeatingHdr) - sizeof(struct PacketizeTx) )


/*********************************************************************/
/*																							*/
/* Prototype definitions for file "wbrp.c" 									*/
/*																							*/
/*********************************************************************/

void			WBRPRepeaterInit(void);
void			SendCompleteWBRPReply (unsigned_8 *destination);
void			AddEntryToWBRPReply (struct WBRPPacket *packet, struct RepeatingTableEntry *entry);
struct		WBRPPacket	*AllocateWBRPReply (void);
void			SendWBRPReply(struct WBRPPacket *packet, unsigned_8 *destination);
void			SendWBRPRequest(void);
struct		WBRPPacket	*AllocateWBRPRequest(void);
void			KillPrimaryRoute(struct RepeatingTableEntry *entry, struct WBRPPacket *packet);
void			RepeaterShutdown(void);
void			ProcessWBRPPacket (struct WBRPPacket *packet);
void			ProcessWBRPReply (unsigned_8 *source, struct RepeaterEntry *entries,
								  int numEntries);
unsigned_8	CalculateMetric(unsigned_8 tsf, unsigned_8 metric);
unsigned_8	CalculateNewTSF (unsigned_8 averageTsf, unsigned_8 newTsf);
void			UpdateRoutingMetrics (unsigned_8 oldTSF, unsigned_8 newTSF, 
									unsigned_8 *nextHopRepeater, struct WBRPPacket **packet);
unsigned_8	RouteSwitch (struct RepeatingInfo *primaryRoute,
									struct RepeatingInfo *secondaryRoute);
unsigned_8	ProcessDataPacket(struct LLDRepeatingPacket *packet);
void			WBRPTimerTick (unsigned_16 timertickintrval);
void			FreeAWBRPBuffer (struct WBRPPacket *packet);
struct		WBRPPacket *GetAWBRPBuffer (void);
void			InitWBRPBuffers (void);
unsigned_8	WBRPGetRepeaterAddr (unsigned_8 *bufptr, unsigned_8 *dest);
unsigned_8	CalculateTSF (unsigned_16 PacketSize, unsigned_8 Status,
								  	unsigned_16 TxDelay);
void			WBRPUpdateTSF (struct LLDRepeatingPacket *TxPacket, unsigned_16 starttime);

#endif /* WBRP_H */
