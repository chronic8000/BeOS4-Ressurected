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
/*	$Header:   J:\pvcs\archive\clld\constant.h_v   1.30   03 Jun 1998 14:41:54   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\constant.h_v  $										*/
/* 
/*    Rev 1.30   03 Jun 1998 14:41:54   BARBARA
/* Proxim protocol constants added.
/* 
/*    Rev 1.29   24 Mar 1998 09:12:44   BARBARA
/* RXBUFFTIMEOUT and INTERNAL_PKT_SIGNATURE added.
/* 
/*    Rev 1.28   12 Sep 1997 16:56:02   BARBARA
/* Random master search implemented.
/* 
/*    Rev 1.27   10 Apr 1997 14:26:46   BARBARA
/* Commented out line taken out.
/* 
/*    Rev 1.26   24 Jan 1997 17:17:56   BARBARA
/* MAXSLOWROAMTIME changed from 15 to 16. MAX_CTS changed from 7 to 6.
/* 
/*    Rev 1.25   15 Jan 1997 18:00:06   BARBARA
/* 
/*    Rev 1.24   27 Sep 1996 09:11:52   JEAN
/* 
/*    Rev 1.23   15 Aug 1996 11:07:08   JEAN
/* Added some new defines for the TxMode and for the
/* LLDHeader2 byte.
/* 
/*    Rev 1.22   14 Jun 1996 16:33:48   JEAN
/* Added some station type definitions.
/* 
/*    Rev 1.21   12 May 1996 17:49:10   TERRYL
/* 
/*    Rev 1.20   16 Apr 1996 11:39:42   JEAN
/* For the serial ODI driver, reduced the size of the node table.
/* 
/*    Rev 1.19   14 Mar 1996 14:50:42   JEAN
/* Moved the serial MAXTIMEOUTTIME to lldctrl.c.
/* 
/*    Rev 1.18   13 Mar 1996 17:55:04   JEAN
/* Increased the MAXTIMEOUTTIME for serial to allow for slower
/* baud rates.  Added a define for the smallest proxim packet.
/* 
/*    Rev 1.17   08 Mar 1996 19:25:06   JEAN
/* Added the ROMVERSIONLEN define.  Added some defines for calculating
/* the buffer sizes needed for receiving and transmitting data.
/* Reduced the node table size for the ODI driver and reduced the
/* number of fragment receive buffers for serial CLLD.
/* 
/*    Rev 1.16   06 Feb 1996 14:32:26   JEAN
/* Added some new defines and moved the PIC controller defines
/* from port.h to this file.
/* 
/*    Rev 1.15   31 Jan 1996 13:34:44   JEAN
/* Added duplicate header include detection, moved some defines
/* into this file, and reduced the RESETTIMEOUT delay.
/* 
/*    Rev 1.14   14 Dec 1995 15:38:34   JEAN
/* Increased the maximum frequency from 82 to 100.
/* 
/*    Rev 1.13   05 Dec 1995 12:10:16   JEAN
/* Fixed some spelling errors.
/* 
/*    Rev 1.12   17 Nov 1995 16:41:42   JEAN
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.11   19 Oct 1995 15:25:50   JEAN
/* Added ADDRESSLENGTH so it can be used by many modules.
/* 
/*    Rev 1.10   12 Oct 1995 13:57:50   JEAN
/* Added Header PVCS keyword.
/* 
/*    Rev 1.9   20 Jun 1995 16:10:36   tracie
/* 
/*    Rev 1.8   13 Apr 1995 08:50:14   tracie
/* xromtest version
/* 
/*    Rev 1.7   16 Mar 1995 16:19:08   tracie
/* Added support for ODI
/* 
/*    Rev 1.6   22 Feb 1995 10:38:36   tracie
/* 
/*    Rev 1.5   27 Dec 1994 15:45:40   tracie
/* 
/*    Rev 1.2   22 Sep 1994 10:56:28   hilton
/* No change.
/* 
/*    Rev 1.1   20 Sep 1994 16:03:24   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:38   hilton
/* Initial revision.
/*																									*/
/*																									*/
/***************************************************************************/



/*********************************************************************/
/*																							*/
/* Size of a MAC address															*/
/*																							*/
/*********************************************************************/

#ifndef CONSTANT_H
#define CONSTANT_H

#define	TRUE	  	1
#define 	FALSE	  	0
#define  NOT		!

#define	ADDRESSLENGTH		6			/* # of bytes in node address.	*/
#define	NAMELENGTH	  		11			/* Length of station name.  When	*/
												/* allocation storage for the 	*/
												/* name, use NAMELENGTH+1 for a	*/
												/* null termination character.	*/
#define	ROMVERLENGTH		7		 	/* Length of ROM version string	*/
#define	MAXMASTERS			7			/* Max # masters to keep track	*/
#define	DRIVERVERLENGTH	10			/* Length of Driver version string*/
#define	MAXROAMHISTMSTRS	10			/* Max # masters to keep track	*/
												/*	in the roaming history			*/

/*********************************************************************/
/*																							*/
/* Constants used by the Wireless Backbone Repeating Protocol			*/
/*																							*/
/*********************************************************************/
#define	REPEATINGHDRLEN	13		/* 6 * 2 + 1 (dest+src+hopsleft)	*/
#define	TOTALREQUEUE		5		/* total number re-entrant queues*/
#define	REGULAR_SEND		2		/* Regular send packet				*/
#define	PACKETIZED_SEND	1		/* Packetized send packet			*/
#define	FREE					0		/* Buffer is free						*/



/*********************************************************************/
/*																							*/
/*	Data packets control byte definitions										*/
/*		b0-1	00=BFSK, 01=QFSK, 11=AUTO											*/
/*		b2-3	Encryption method														*/
/*		b4		Encryption switch 1=on, 0=off										*/
/*		b5-6	Service type 0=type 1, 1=type 3									*/
/*		b7		Poll final bit															*/
/*																							*/
/*********************************************************************/

#define	TX_BFSK			0					/* BFSK transfer mode			*/
#define	TX_QFSK			1					/* QFSK transfer mode			*/
#define	TX_AUTO_FRAG	2					/* Auto mode for fragments		*/
#define	TX_AUTO			3					/* Auto mode QFSK or BFSK		*/
#define	TX_ENCRYPMETH1	4					/* Encryption method 1			*/
#define	TX_ENCRYPMETH2	8					/* Encryption method 2			*/
#define	TX_ENCRYPMETH3	12					/* Encryption method 3			*/
#define	TX_ENCRYPTION	0x10				/* Encryption						*/
#define	TX_MACACK		0x20				/* MAC acknowledge				*/
#define	TX_SRVCTYPE		0x40				/* Service type					*/
#define	TX_POLLFINAL	0x80				/* Poll final bit					*/


/*********************************************************************/
/*																							*/
/* Time equates in ticks (for use with GetTime function)					*/
/*																							*/
/*********************************************************************/

#define	TPS						30				/*	30 ticks per second		*/

#define	MAXNAKWAIT				TPS / 2		/* Ticks = 1/2 second		*/
#define	SLEEPWAITTIME			TPS / 2		/* Ticks = 1/2 second		*/
#define	BEFORERESENDTIME		TPS / 10		/* Ticks = 100 millisec		*/
#define	MAXOUTOFSYNCTIME		TPS			/* Ticks = 1 second			*/
#define	FRAGTIMEOUT				TPS * 2		/* Ticks = 2 seconds			*/
#define	MAXTIMEOUTTIME			TPS * 20		/* Ticks = 20 seconds		*/
#define	RESETTIMEOUT			TPS * 2		/* Ticks = 2 seconds			*/
#define	REPEATTIMEOUT			TPS * 4		/* Ticks = 4 seconds			*/
#define	MAXROAMWAITTIME		TPS * 5		/* Ticks = 5 seconds			*/
#define	MAXSLOWROAMTIME		TPS * 16		/* Ticks = 15 seconds 		*/
#define	ROAMNOTIFYWAIT			TPS / 10		/* Ticks = 100 milliseconds*/
#define	MAXWAITTIME				TPS * 6		/* Ticks = 6 seconds			*/
#define	MAXOSTIME				TPS * 8		/* Ticks = 8 seconds			*/
#define	MAXROAMOSTIME			TPS * 10		/* Ticks = 10 seconds		*/
#define	BRIDGETIMEOUT			TPS * 24		/* Ticks = 24 seconds		*/
#define	RXBUFFTIMEOUT			TPS * 480	/* Ticks = 8 min				*/
#define	SENDLLDPINGTIME		TPS * 300	/* Ticks = 5 min				*/

#define	BFSKCOUNT			20 			/* Maximum pkts to send in BFSK		*/
#define	FLAGSET				1				/* State flags have been set			*/
#define	LLDMAXSEND			8				/* # pkts RFNC can handle at once	*/
#define	LOOKAHEADADDON		52				/* Hardware header +						*/
#define	LOOKAHEADSIZE		18				/* Set by the upper layer.				*/
#define	EVENIZE				0xFE			/* Evenize an odd length				*/
#define	MIN_PKT_SIZE		60				/* Minimum ethernet packet	size.		*/
#define	MAX_PKT_SIZE		1514			/* Maximum ethernet packet size.		*/
#define	MAX_CTS				6				/* Max # of CTS errors before BDisc */
													/* 7 Failed roaming test */
#define	LLDPKT_HDR_LEN		12
#define	LLDBRIDGE_HDR_LEN	(ADDRESSLENGTH*2)

#ifdef SERIAL
#define	LLDSERIAL_HDR_LEN	7				/* Also called TOTAL_HDRS in slld.h	*/
#define	SLUSH					7
#else
#define	LLDSERIAL_HDR_LEN	0				/* Also called TOTAL_HDRS in slld.h	*/
#define	SLUSH					14
#endif

#define	MIN_PROX_PKT_SIZE 4				/* Our smallest packetized command.	*/

#define	MAX_PROX_PKT_SIZE	(MAX_PKT_SIZE+LLDPKT_HDR_LEN+LLDBRIDGE_HDR_LEN+LLDSERIAL_HDR_LEN)
#define	MAP_TABLE_SIZE		128			/* Total number of entries in the	*/
													/* Use this to verify the packet		*/
													/* lengths received from the RFNC.	*/
													/* Maximum packet length that we		*/
													/* transmit/receive.  This is the	*/	
													/* 7-byte header for serial CLLD,	*/
													/* 12-byte LLD header, the 12-byte	*/
													/* bridge header, and the maximum	*/
													/* ethernet packet.  We also add		*/
													/* 14 bytes as a precaution and to	*/
													/* align the buffer on a 16-byte		*/
													/* boundary.				  				*/

#define BUFFERLEN				(MAX_PROX_PKT_SIZE+SLUSH)
													/* BUFFERLEN = 1550 	  					*/
													/* Use this to declare buffers		*/
													/* for receiving/transmitting			*/
													/* packets.									*/

#define	MAXFREQUENCY		100			/* Maximum number of frequency		*/

#ifdef SERIAL
#define	TOTALRXBUF			2				/* Total number of receive buffers	*/
#else
#define	TOTALRXBUF			3				/* Total number of receive buffers	*/
#endif

#define	TOTALTXBUF			3				/* Total number of transmit buffers	*/


/*********************************************************************/
/* Do not make TOTALENTRY greater than 128 entries.  We use an 8-bit	*/
/* value as an index into the NodeTable array.  Instead of using		*/
/* 0 as a NULL pointer, we use 0xFF.											*/
/*********************************************************************/

#ifdef ODI
#define	TOTALENTRY	8		/* Total number of entries in the node table. */
#else
#define	TOTALENTRY	128	/* Total number of entries in the node table. */
#endif


/*********************************************************************/
/*																							*/
/*	IBM on board 8259 Interrupt Controller Equates  						*/
/*																							*/
/*********************************************************************/

/* Value written to either PIC to clear an interrupt. */
#define	I8259EOI							0x20

/* Addresses of the first PIC */
#define	I8259CMD							0x20
#define	I8259IRQMASK					0x21

/* Addresses of the second PIC */
#define	I8259CMD2						0xA0
#define	I8259IRQMASK2					0xA1



/********************************************************************/
/*																						  */
/*	LLD Header definition														  */
/*																						  */
/*	LLDHeader1					Undefined										  */
/*																						  */
/*	LLDHeader2					Bit 0-3 	= # of pad bytes for even frags */
/*									Bit 4 	= Master Flag/WBRPFlag 			  */
/*									Bit 5 	= Roam notify flag				  */
/*									Bit 6 	= Repeat flag						  */
/*									Bit 7		= Bridge flag						  */
/*																						  */
/*	LLDHeader3					Packet Sequence number (0-255)			  */
/*																						  */
/*	LLDHeader4					Bit 0-3	= Fragment number					  */
/*									Bit 4-7	= Total number of fragments	  */
/*																						  */
/********************************************************************/

#define	LLDBRIDGEBIT		0x80			/* Set for bridge pkts					*/
#define	LLDREPEATBIT		0x40			/* Set for repeat pkts					*/
#define  LLDROAMNOTIFYBIT	0x20			/* Can be set if the packet is a		*/
#define	LLDPADBITS_MASK	0x0F			/* Mask for # of pad bytes				*/
													/* Roam Notify packet.  The CLLD 	*/
													/* does not currently use this bit.	*/
#define  LLDMASTERBIT		0x10			/* Set if the packet came from a 	*/
													/* master.									*/
#ifdef WBRP
#define	LLDWBRPBIT			0x10			/* Set for WBRP Reply/Request Pkts 	*/
#endif

/********************************************************************/
/*																						  */
/*	Transfer mode equates														  */
/*																						  */
/********************************************************************/

#define	AUTODETECT				2			/* Automatically determines txmode	*/
#define	BYTEMODE					1			/* 8 bit mode								*/
#define	WORDMODE					0			/* 16 bit mode								*/

/********************************************************************/
/*																						  */
/*	Node Types																		  */
/*																						  */
/********************************************************************/

#define	STATION					0
#define	ALTERNATE				1
#define	MASTER					2


/********************************************************************/
/*																						  */
/*	Driver Types																	  */
/*																						  */
/********************************************************************/

#define	ISA_DRIVER				0			/* LLD is build for ISA					*/
#define	PCMCIA_DRIVER			1			/* LLD is build for PCMCIA 			*/
#define	OEM_DRIVER				2			/* LLD is build for OEM					*/




/********************************************************************/
/*																						  */
/*	Return value equates.														  */
/*																						  */
/********************************************************************/

#define	SUCCESS	0
#define	FAILURE	0xFF

#define	ACK		1
#define	NAK		2

#define	NO			0
#define	YES		0xFF

#define	CLEAR		0
#define	SET		0xFF



/********************************************************************/
/*																						  */
/* Debug color equates (Character Attribute Codes)						  */
/*																						  */
/********************************************************************/

#define	BLACK				0
#define	BLUE				1
#define	GREEN				2
#define	CYAN				3
#define	RED				4
#define	MAGENTA			5
#define	BROWN				6
#define	WHITE				7
#define	GRAY				8
#define	BRIGHT_BLUE		9
#define	BRIGHT_GREEN	10
#define	BRIGHT_CYAN		11
#define	BRIGHT_RED		12
#define	BRIGHT_MAGENTA 13
#define	YELLOW			14
#define	BRIGHT_WHITE	15

#define	BLACK_BACK		0
#define	BLUE_BACK		0x10
#define	GREEN_BACK		0x20
#define	CYAN_BACK		0x30
#define	RED_BACK			0x40
#define	MAGENTA_BACK	0x50
#define	BROWN_BACK		0x60
#define	WHITE_BACK		0x70

#define	BLINK				0x80

#define	NOTES_BUFFER_SIZE	251
#define	STATION_NAME_SIZE	20
#define	INTERNAL_PKT_SIGNATURE	0x45674567

#define	CARD_TYPE_ISA	0
#define	CARD_TYPE_OEM	1

#endif /* CONSTANT_H */
