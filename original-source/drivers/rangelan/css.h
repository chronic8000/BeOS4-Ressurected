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
/*	$Header:   J:\pvcs\archive\clld\css.h_v   1.13   30 Sep 1996 16:59:30   JEAN  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\css.h_v  $											*/
/* 
/*    Rev 1.13   30 Sep 1996 16:59:30   JEAN
/* 
/*    Rev 1.12   30 Sep 1996 16:50:58   JEAN
/* 
/*    Rev 1.11   30 Sep 1996 16:15:08   BARBARA
/* AddResources added.
/* 
/*    Rev 1.8   24 Sep 1996 18:25:52   BARBARA
/* Request specific resources from CSS added.
/* 
/*    Rev 1.7   09 Sep 1996 16:30:30   JEAN
/* Modified some function prototypes.
/* 
/*    Rev 1.6   14 Jun 1996 16:43:10   JEAN
/* Comment changes.
/* 
/*    Rev 1.5   06 Feb 1996 14:35:16   JEAN
/* Comment changes.
/* 
/*    Rev 1.4   31 Jan 1996 13:43:34   JEAN
/* Added some duplicate header include detection.
/* 
/*    Rev 1.3   29 Nov 1995 10:22:26   JEAN
/* Added the PVCS Header keyword.
/* 
/*    Rev 1.2   22 Sep 1994 10:56:34   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:36   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:46   hilton
/* Initial revision.
/* 																								*/
/*																									*/
/***************************************************************************/

#ifndef CSS_H
#define CSS_H

#define	CARD_SERVICES			0xAF



/******************************************************/
/*																		*/
/* DeRegister Client Function (0x02)						*/
/*																		*/
/******************************************************/

#define	DEREG_CLIENT_FN		0x02



/******************************************************/
/*																		*/
/* GetFirstTuple Function (0x07)								*/
/*																		*/
/******************************************************/

#define	GET_1ST_TUPLE_FN		0x07


struct GetFirstTupleArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_8		DesiredTuple;
	unsigned_8		Reserved;
	unsigned_16		Flags;
	unsigned_32		LinkOffset;
	unsigned_32		CISOffset;
	unsigned_8		TupleCode;
	unsigned_8		TupleLink;
};




/******************************************************/
/*																		*/
/* Get Card Services Info Function (0x0B)					*/
/*																		*/
/******************************************************/

#define	GET_CARD_SVC_FN		0x0B


struct GetCardSvcArgs
{	unsigned_16		InfoLen;
	unsigned_8		Signature [2];
	unsigned_16		Count;
	unsigned_16		Revision;
	unsigned_16		CSLevel;
	unsigned_16		VStrOff;
	unsigned_16		VStrLen;
	unsigned_8		VendorStr [50];
};




/******************************************************/
/*																		*/
/* GetTupleData Function (0x0D)								*/
/*																		*/
/******************************************************/

#define	GET_TUPLE_DATA_FN		0x0D


struct GetTupleDataArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_8		DesiredTuple;
	unsigned_8		TupleOffset;
	unsigned_16		Flags;
	unsigned_32		LinkOffset;
	unsigned_32		CISOffset;
	unsigned_16		TupleDataMax;
	unsigned_16		TupleDataLen;
	unsigned_8		TupleData [50];
};




/******************************************************/
/*																		*/
/* Register Client Function (0x10)							*/
/*																		*/
/******************************************************/

#define	REG_CLIENT_FN			0x10


struct RegClientArgs
{	unsigned_16		Attributes;
	unsigned_16		EventMask;
	unsigned_16		ClientData;
	unsigned_16		ClientSegment;
	unsigned_16		ClientOffset;
	unsigned_16		Reserved;
	unsigned_16		Version;
};




/******************************************************/
/*																		*/
/* MapMemPage Function (0x14)									*/
/*																		*/
/******************************************************/

#define	MAPMEMPAGE_FN			0x14


struct MapMemPageArgs
{	unsigned_32		CardOffset;
	unsigned_8		Page;
};
	


/******************************************************/
/*																		*/
/* Release IO Function (0x1B)									*/
/*																		*/
/******************************************************/

#define	RELEASE_IO_FN			0x1B


struct ReleaseIOArgs
{	unsigned_16		Socket;
	unsigned_16		BasePort1;
	unsigned_8		NumPorts1;
	unsigned_8		Attributes1;
	unsigned_16		BasePort2;
	unsigned_8		NumPorts2;
	unsigned_8		Attributes2;
	unsigned_8		IOAddrLines;
};



/******************************************************/
/*																		*/
/* Release IRQ Function (0x1C)								*/
/*																		*/
/******************************************************/

#define	RELEASE_IRQ_FN			0x1C


struct ReleaseIRQArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_8		AssignedIRQ;
};




/******************************************************/
/*																		*/
/* Release Window Function (0x1D)							*/
/*																		*/
/******************************************************/

#define	RELEASE_WIN_FN			0x1D



/******************************************************/
/*																		*/
/* Release Configuration Function (0x1E)					*/
/*																		*/
/******************************************************/

#define	RELEASE_CFG_FN			0x1E


struct ReleaseCfgArgs
{	unsigned_16		Socket;
};



/******************************************************/
/*																		*/
/* Request IO Function (0x1F)									*/
/*																		*/
/******************************************************/

#define	REQUEST_IO_FN			0x1F


struct RequestIOArgs
{	unsigned_16		Socket;
	unsigned_16		BasePort1;
	unsigned_8		NumPorts1;
	unsigned_8		Attributes1;
	unsigned_16		BasePort2;
	unsigned_8		NumPorts2;
	unsigned_8		Attributes2;
	unsigned_8		IOAddrLines;
};



/******************************************************/
/*																		*/
/* RequestIRQ Function (0x20)									*/
/*																		*/
/******************************************************/

#define	REQUEST_IRQ_FN		0x20


struct RequestIRQArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_8		AssignedIRQ;
	unsigned_8		IRQInfo1;
	unsigned_16		IRQInfo2;
};



/******************************************************/
/*																		*/
/* Request Window Function (0x21)							*/
/*																		*/
/******************************************************/

#define	REQUEST_WINDOW_FN		0x21

/*------------------------------------------*/
/* Access Speed field equates					  */
/*														  */
/*------------------------------------------*/

#define	SPEED_250nS				1
#define	SPEED_200nS				2
#define	SPEED_150nS				3
#define	SPEED_100nS				4


struct ReqWindowArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_32		Base;
	unsigned_32		Size;
	unsigned_8		Speed;
};
	


/******************************************************/
/*																		*/
/* Request Configuration Function (0x30)					*/
/*																		*/
/******************************************************/

#define	REQUEST_CFG_FN		0x30


struct RequestCfgArgs
{	unsigned_16		Socket;
	unsigned_16		Attributes;
	unsigned_8		Vcc;
	unsigned_8		Vpp1;
	unsigned_8		Vpp2;
	unsigned_8		IntType;
	unsigned_32		ConfigBase;
	unsigned_8		Status;
	unsigned_8		Pin;
	unsigned_8		Copy;
	unsigned_8		ConfigIndex;
	unsigned_8		Present;
};



/******************************************************/
/*																		*/
/* Adjust Resource Info											*/
/*																		*/
/******************************************************/

#define	ADJUST_RESOURCE_FN	0x35


/*------------------------------------------*/
/*	Action field equates							  */
/*														  */
/*------------------------------------------*/

#define	REMOVE_RESOURCE		0
#define	ADD_RESOURCE			1
#define	GET_FIRST_RESOURCE	2
#define	GET_NEXT_RESOURCE		3


/*------------------------------------------*/
/*	Resource field equates						  */
/*														  */
/*------------------------------------------*/

#define	MEMORY_RANGE			0
#define	IO_RANGE					1
#define	IRQ						2


/*------------------------------------------*/
/*	Memory range attributes						  */
/*														  */
/*------------------------------------------*/

#define	SHARED_MEMORY			0x0020
#define	ALLOCATED_MEMORY		0x0080


/*------------------------------------------*/
/*	I/O range attributes							  */
/*														  */
/*------------------------------------------*/

#define	SHARED_IO				0x01
#define	ALLOCATED_IO			0x80


struct AdjMemArgs
{	unsigned_8		Action;
	unsigned_8		Resource;
	unsigned_16		Attributes;
	unsigned_32		Base;
	unsigned_32		WindowSize;
};


struct AdjIOArgs
{	unsigned_8		Action;
	unsigned_8		Resource;
	unsigned_16		BasePort;
	unsigned_8		NumPorts;
	unsigned_8		Attributes;
	unsigned_8		IOAddrLines;
};


struct AdjIRQArgs
{	unsigned_8	Action;
	unsigned_8	Resource;
	unsigned_8	Attributes;
	unsigned_8	IRQNum;
};



/*********************************************************************/
/*																							*/
/* These are the prototype definitions for "css.c"							*/
/*																							*/
/*********************************************************************/

void				CallBack (void);
unsigned_16		HandleCallBack (unsigned_8 CSSFunction, unsigned_16 CSSSocket);
unsigned_16		GetCardServicesInfo (void);
unsigned_16		CSSUnload (void);
unsigned_16		RegisterClient (void);
unsigned_16		DeRegisterClient (void);
unsigned_16		AddResources(void);
unsigned_16		RequestConfiguration (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
												 unsigned_32 CORAddress);
unsigned_16		ReleaseConfiguration (unsigned_16 CSSSocket, unsigned_16 ClientHandle);
unsigned_16		RequestWindow (unsigned_16 CSSSocket, 
										unsigned_16 *CurrWindowHandle, unsigned_32 *WindowBase);
unsigned_16		ReleaseWindow (unsigned_16 WinHandle);
unsigned_16		MapMemPage (unsigned_16 WindowHandle);
unsigned_16		AddMemoryResource (unsigned_32 WindowBase);
unsigned_16		RemoveMemoryResource (unsigned_32 WindowBase);
unsigned_16		RequestIO (unsigned_16 CSSSocket,
								  unsigned_16 ClientHandle, unsigned_16 *IOAddress,
								  unsigned_16 *IOAddress2, unsigned_8 *BusWidth);
unsigned_16		ReleaseIO (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
								  unsigned_16 IOAddress, unsigned_16 IOAddress2,
								  unsigned_8 BusWidth);
unsigned_16 	AddIOResource (unsigned_16 IOAddress);
unsigned_16 	RemoveIOResource (unsigned_16 IOAddress);
unsigned_16 	RequestIRQ (unsigned_16 CSSSocket,
									unsigned_16 ClientHandle, unsigned_8 *IRQLine);
unsigned_16		ReleaseIRQ (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
									unsigned_8 IRQLine);
unsigned_16		AddIRQResource (unsigned_8 IRQLine);
unsigned_16		RemoveIRQResource (unsigned_8 IRQLine);
unsigned_16		GetFirstTuple (struct GetFirstTupleArgs *TupleArgs, unsigned_8 TupleCode,
										unsigned_16 CSSSocket);
unsigned_16		GetTupleData (struct GetTupleDataArgs *TupleArgs);
unsigned_16 	CardServices (unsigned_8 Function, unsigned_16 *Handle,
									  unsigned_32 ClientEntry, unsigned_16 ArgLength,
									  unsigned_32 ArgPointer);
#endif /* CSS_H */

