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
/*	$Header:   J:\pvcs\archive\clld\css.c_v   1.24   24 Mar 1998 08:51:48   BARBARA  $*/
/*																									*/
/* Edit History																				*/
/*																									*/
/*	$Log:   J:\pvcs\archive\clld\css.c_v  $											*/
/* 
/*    Rev 1.24   24 Mar 1998 08:51:48   BARBARA
/* Option of requesting 16K memory window added.
/* 
/*    Rev 1.23   30 Sep 1996 16:50:36   JEAN
/* 
/*    Rev 1.22   30 Sep 1996 16:15:34   BARBARA
/* AddResources added.
/* 
/*    Rev 1.20   27 Sep 1996 09:05:14   JEAN
/* 
/*    Rev 1.19   26 Sep 1996 10:54:48   JEAN
/* Added some extern declarations.
/* 
/*    Rev 1.18   09 Sep 1996 16:29:14   JEAN
/* Made changes to request a specific IO address, IRQ number, and
/* memory window address.
/* 
/*    Rev 1.17   14 Jun 1996 16:20:40   JEAN
/* Made changes to support a one piece PCMCIA card.
/* 
/*    Rev 1.16   29 Mar 1996 11:43:12   JEAN
/* Moved some variable declarations to lld.c
/* 
/*    Rev 1.15   04 Mar 1996 15:05:48   truong
/* Conditional compiled out CSS code when building WinNT/95 drivers.
/* 
/*    Rev 1.14   06 Feb 1996 14:25:50   JEAN
/* Comment changes.
/* 
/*    Rev 1.13   31 Jan 1996 13:12:02   JEAN
/* Changed the ordering of some header include files.
/* 
/*    Rev 1.12   19 Dec 1995 18:08:30   JEAN
/* Added a header include file.
/* 
/*    Rev 1.11   14 Dec 1995 16:18:12   JEAN
/* Added a header include file.
/* 
/*    Rev 1.10   17 Nov 1995 16:36:58   JEAN
/* 
/* Check in for the 2.0 RL2DIAG/SRL2DIAG version.
/* 
/*    Rev 1.9   12 Oct 1995 11:51:18   JEAN
/* Added the Header PVCS keyword.
/* 
/*    Rev 1.8   17 Sep 1995 16:57:56   truong
/* Added local stack to fix stack overflow problem with rl2setup.
/* Remove RemoveMemoryResource call from unload.
/* 
/*    Rev 1.7   24 Jul 1995 18:37:16   truong
/* 
/*    Rev 1.6   16 Mar 1995 16:16:34   tracie
/* Added support for ODI
/* 
/*    Rev 1.5   22 Feb 1995 10:37:08   tracie
/* 
/*    Rev 1.4   05 Jan 1995 11:07:28   hilton
/* Changes for multiple card version
/* 
/*    Rev 1.3   29 Nov 1994 12:45:20   hilton
/* 
/*    Rev 1.2   22 Sep 1994 10:56:20   hilton
/* 
/*    Rev 1.1   20 Sep 1994 16:03:16   hilton
/* No change.
/* 
/*    Rev 1.0   20 Sep 1994 11:00:30   hilton
/* Initial revision.
/* 																								*/
/*																									*/
/***************************************************************************/


#pragma pack(1)


#include <string.h>
#include	"constant.h"
#include "lld.h"
#include "css.h"
#include "pcmcia.h"
#include	"bstruct.h"


#define CARD_INSERTION		0x40
#define CARD_REMOVAL			0x05


#ifdef MULTI
#ifdef SERIAL
#include "slld.h"
#include "slldbuf.h"
#endif
#include	"multi.h"

extern struct LLDVarsStruct	*LLDVars;

#else
extern unsigned_16	CSSClientHandle;
extern unsigned_16	CORAddress;
extern unsigned_16	CCReg;
extern unsigned_8		PCMCIACardInserted;
extern unsigned_16	CSSDetected;
extern unsigned_8		LLDOnePieceFlag;

extern unsigned_8		Inserted;
extern unsigned_8		FirstInsertion;
extern unsigned_8		FirstTimeIns;
extern unsigned_16 	WindowHandle;
extern unsigned_32	WindowBase;
extern unsigned_16 	PCMCIASocket;
extern unsigned_16 	IOAddress;
extern unsigned_16 	IOAddress2;
extern unsigned_8		BusWidth;
extern unsigned_8		IRQLine;
extern unsigned_16	LLDIOAddress1;
extern unsigned_8		LLDIntLine1;
extern unsigned_32	LLDMemoryAddress1;
extern unsigned_32 	MemSize;

#endif


#ifndef NDIS_MINIPORT_DRIVER

/***************************************************************************/
/*																									*/
/*	CallBack ()																					*/
/*																									*/
/*	INPUT:	AL = Function number.														*/
/*				CX = Socket number.															*/
/*				ES:BX = Buffer.																*/
/*																									*/
/*	OUTPUT:																						*/
/*																									*/
/*	DESCRIPTION:	This routine will be called by the card and socket			*/
/*		services to notify the driver of certain events.  This is basically	*/
/*		an assembly language link to call the real callback handler.			*/
/*		It can not really pass back a status value because it is called		*/
/*		like an interrupt handler and the flags and AX needs to be saved.		*/
/*																									*/
/***************************************************************************/


#define CALLBACK_STACK_SIZE     255

unsigned_32 OldCallBackStack;
unsigned_16 NewCallBackStack[CALLBACK_STACK_SIZE+1];

void CallBack (void)

{
/* unsigned_8	CSSFunction; */
/*	unsigned_16	CSSSocket; */

#if 0
   /* switch stack since we will use a lot of it in the ISR */
   __asm    mov   word ptr OldCallBackStack+2,ss
   __asm    mov   word ptr OldCallBackStack,sp

   __asm    push  ds
   __asm    pop   ss
   __asm    mov   sp, offset NewCallBackStack+(CALLBACK_STACK_SIZE*2)

	/*-------------------------------------------------*/
	/*	Save registers and move registers to local		*/
	/* variables for "C" to operate on.						*/
	/*																	*/
	/*-------------------------------------------------*/

	__asm		pushf
	__asm		push	ax
	__asm		push	bx
	__asm		push	cx
	__asm		push	dx
	__asm		push	si
	__asm		push	di
	__asm		push	ds
	__asm		push	es

	__asm		push	ds
	__asm		pop	es

/*	__asm		mov	CSSFunction, al */
/*	__asm		mov	CSSSocket, cx */
/* HandleCallBack (CSSFunction, CSSSocket); */

   __asm    push  cx
   __asm    push  ax
   __asm    call  HandleCallBack
   __asm    add   sp,4

	__asm		pop	es
	__asm		pop	ds
	__asm		pop	di	
	__asm		pop	si	
	__asm		pop	dx
	__asm		pop	cx
	__asm		pop	bx
	__asm		pop	ax
	__asm		popf

   __asm    mov   ss,word ptr OldCallBackStack+2
   __asm    mov   sp,word ptr OldCallBackStack
#endif
}




/***************************************************************************/
/*																									*/
/*	HandleCallBack (CSSFunction, CSSSocket)											*/
/*																									*/
/*	INPUT:	CSSFunction		-	Function number.										*/
/*				CSSSocket		-	Socket number.											*/
/*																									*/
/*	OUTPUT:	0					-	Return code.											*/
/*				2					-																*/
/*																									*/
/*	DESCRIPTION:	This routine is the actual call back handler.  It is		*/
/*		implemented in this way so that error handling can be done more		*/
/*		effectively, by returning immediately without worrying about			*/
/*		popping saved registers off the stack.  Only card insertion and		*/
/*		removal events are handled.														*/
/*																									*/
/***************************************************************************/

unsigned_16 HandleCallBack (unsigned_8 CSSFunction, unsigned_16 CSSSocket)

{	struct GetTupleDataArgs	TupleCommand;

	unsigned_16	CurrWindowHandle;
	unsigned_8	CIS_Rev;


	/*-------------------------------------------------*/
	/*	Check for card insertion event and card not		*/
	/* already inserted.											*/
	/*																	*/
	/*-------------------------------------------------*/

	if ((CSSFunction == CARD_INSERTION) && (Inserted == 0))
	{	PCMCIASocket = CSSSocket;

		if (FirstInsertion == 0)
		{	if (AddMemoryResource (WindowBase))
				return (2);
		}

		if (RequestWindow (CSSSocket, &CurrWindowHandle, &WindowBase))
			return (2);

		if (MapMemPage (CurrWindowHandle))
			return (2);


		/*------------------------------------------------------------------*/
		/* Get the product info tuple and check for the Proxim string.		  */
		/* For IBM card services compatibility, offsets 4 and 6 are			  */
		/* checked also.  If the string isn't found, the memory window		  */
		/* is returned.																	  */
		/*																						  */
		/*------------------------------------------------------------------*/

		if (GetFirstTuple ((struct GetFirstTupleArgs *)&TupleCommand, 0x15, CSSSocket))
		{	ReleaseWindow (CurrWindowHandle);
			RemoveMemoryResource (WindowBase);
			return (2);
		}

		if (GetTupleData (&TupleCommand))
			return (2);

		if (memcmp (&TupleCommand.TupleData [2], "PROXIM", 6))
		{	if (memcmp (&TupleCommand.TupleData [4], "PROXIM", 6))
			{	if (memcmp (&TupleCommand.TupleData [6], "PROXIM", 6))
				{	ReleaseWindow (CurrWindowHandle);
					RemoveMemoryResource (WindowBase);
					return (0);
				}
			}
		}
					
		Inserted = 1;
		WindowHandle = CurrWindowHandle;

		CIS_Rev = TupleCommand.TupleData [27];

		if (GetFirstTuple ((struct GetFirstTupleArgs *)&TupleCommand, 0x1A, CSSSocket))
		{	return (2);
		}

		if (GetTupleData (&TupleCommand))
		{	return (2);
		}

		CORAddress = *(unsigned_16 *)(&TupleCommand.TupleData [2]);
		CCReg = CORAddress;
		if ((unsigned_16)CCReg == CCREG_1PIECE)
		{
		 	LLDOnePieceFlag = 1;
		}

		/*----------------------------------------------*/
		/*	For each resource, check if this is the 1st	*/
		/* insertion before requesting it (in case the	*/
		/* resource needs to be added back first.			*/
		/*																*/
		/*----------------------------------------------*/

		if (FirstInsertion == 0)
		{	if (AddIOResource (IOAddress))
				return (2);
		}
		if (RequestIO (CSSSocket, CSSClientHandle, &IOAddress, &IOAddress2, &BusWidth))
			return (2);

		if (FirstInsertion == 0)
		{	if (AddIRQResource (IRQLine))
				return (2);
		}
		if (RequestIRQ (CSSSocket, CSSClientHandle, &IRQLine))
			return (2);

		if (RequestConfiguration (CSSSocket, CSSClientHandle, CORAddress))
			return (2);

		FirstInsertion = 0;

		if (InsertedPCMCIACard (IRQLine, WindowBase, IOAddress, CSSSocket))
			return (2);

		return (0);
	}


	/*-------------------------------------------------*/
	/*	Check for card removal event and release the		*/
	/* resources.													*/
	/*																	*/
	/*-------------------------------------------------*/

	else if ((CSSFunction == CARD_REMOVAL) && (CSSSocket == PCMCIASocket))
	{	if (ReleaseConfiguration (CSSSocket, CSSClientHandle))
			return (2);

		if (ReleaseIO (CSSSocket, CSSClientHandle, IOAddress, IOAddress2, BusWidth))
			return (2);		

		if (RemoveIOResource (IOAddress))
			return (2);

		if (ReleaseIRQ (CSSSocket, CSSClientHandle, IRQLine))
			return (2);

		if (RemoveIRQResource (IRQLine))
			return (2);

		if (ReleaseWindow (WindowHandle))
			return (2);

		if	(RemoveMemoryResource (WindowBase))
			return (2);

		Inserted = 0;
		RemovedPCMCIACard ();

		return (0);
	}
	return (0);
}





/***************************************************************************/
/*																									*/
/*	CSSUnload ()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns 	-	0 = Success, otherwise error								*/
/*																									*/
/*	DESCRIPTION:	This procedure will release the resources from the card	*/
/*		and socket services.																	*/
/*																									*/
/***************************************************************************/

unsigned_16 CSSUnload (void)

{
	if (ReleaseConfiguration (PCMCIASocket, CSSClientHandle))
		return (2);

	if (ReleaseIO (PCMCIASocket, CSSClientHandle, IOAddress, IOAddress2, BusWidth))
		return (2);		

	if (ReleaseIRQ (PCMCIASocket, CSSClientHandle, IRQLine))
		return (2);

	if (ReleaseWindow (WindowHandle))
		return (2);

/*	if	(RemoveMemoryResource (WindowBase))
		return (2);
*/

	Inserted = 0;
	return (0);
}

/***************************************************************************/
/*																									*/
/*	AddResources()																				*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns 	-	0 = Success, otherwise error								*/
/*																									*/
/*	DESCRIPTION:	This procedure will return the resources to the card   	*/
/*		and socket services.																	*/
/*																									*/
/***************************************************************************/

unsigned_16 AddResources (void)

{
	if (AddMemoryResource (WindowBase))
		return (2);

	if (AddIOResource (IOAddress))
		return (2);		

	if (AddIRQResource (IRQLine))
		return (2);
	DeRegisterClient();
	return (0);
}

/***************************************************************************/
/*																									*/
/*	GetCardServicesInfo ()															 		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns 	-	0 indicates presence of card and socket services.	*/
/*		IROLine, WindowBase, IOAddress are set up.			  						*/
/*	DESCRIPTION:	This procedure will check for the presence of C&SS.		*/
/*																									*/
/***************************************************************************/

unsigned_16	GetCardServicesInfo (void)

{	struct GetCardSvcArgs  	GetCardSvcCmd;
	struct GetCardSvcArgs  	*ArgPointer;
	unsigned_16			  		ArgLength;
	unsigned_16			  		Status;
	unsigned_16			  		DummyHandle;

	GetCardSvcCmd.Signature [0] = 0;
	GetCardSvcCmd.Signature [1] = 0;

	ArgLength = sizeof (struct GetCardSvcArgs);
	ArgPointer = &GetCardSvcCmd;


	Status = CardServices (	(unsigned_8) GET_CARD_SVC_FN, 
									(unsigned_16 *)&DummyHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	if (Status)
		return (Status);

	if (memcmp (&GetCardSvcCmd.Signature, "CS", 2) == 0)
	{	
		IRQLine = LLDIntLine1;
		WindowBase = LLDMemoryAddress1; 
		WindowBase = WindowBase << 4;
		IOAddress = LLDIOAddress1;
		return (0);
	}
	else
	{	return (1);
	}
}




/***************************************************************************/
/*																									*/
/*	RegisterClient ()																			*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns 	-	Status from card services.									*/
/*																									*/
/*	DESCRIPTION:	This procedure will register a client with card services.*/
/*																									*/
/***************************************************************************/

unsigned_16 RegisterClient (void)

{	struct RegClientArgs		RegClientCmd;
	struct RegClientArgs 	*ArgPointer;
	unsigned_16					ArgLength;
	unsigned_16					Status;
	unsigned_32					ClientEntry;
	unsigned_32					DataAddress;

	DataAddress = (unsigned_32)&CSSDetected;

	RegClientCmd.Attributes = 0x001C;
	RegClientCmd.EventMask = 0x0080;
	RegClientCmd.ClientData = 0;
	RegClientCmd.ClientSegment = (unsigned_16)(DataAddress >> 16);
	RegClientCmd.ClientOffset = (unsigned_16)(DataAddress & 0x00FF);
	RegClientCmd.Reserved = 0;
	RegClientCmd.Version = 0x0201;

	ArgLength = sizeof (struct RegClientArgs);
	ArgPointer = &RegClientCmd;
	ClientEntry = (unsigned_32)CallBack;
	CSSClientHandle = 0;

	Status = CardServices ( (unsigned_8) REG_CLIENT_FN, 
									(unsigned_16 *) &CSSClientHandle, 
									(unsigned_32) ClientEntry, 
									(unsigned_16) ArgLength, 
									(unsigned_32) ArgPointer );
	return (Status);
}





/***************************************************************************/
/*																									*/
/*	DeRegisterClient ()																		*/
/*																									*/
/*	INPUT:																						*/
/*																									*/
/*	OUTPUT:	Returns 	-	Status from card services.									*/
/*																									*/
/*	DESCRIPTION:	This procedure will deregister a client from card 			*/
/*						services.																*/
/*																									*/
/***************************************************************************/

unsigned_16 DeRegisterClient (void)

{	unsigned_16	Status;

	Status = CardServices ( (unsigned_8) DEREG_CLIENT_FN, 
									(unsigned_16 *) &CSSClientHandle, 
									(unsigned_32) 0, 
									(unsigned_16) 0, 
									(unsigned_32) 0);
	FirstInsertion = 0;
	FirstTimeIns = 1;
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	RequestConfiguration (CSSSocket, ClientHandle, CfgAddress)					*/
/*																									*/
/*	INPUT:	CSSSocket	 	-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*				CfgAddress		-	Config register base address						*/
/*																									*/
/*	OUTPUT:	Returns		 	-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will configure the card and socket.			*/
/*
/***************************************************************************/

unsigned_16 RequestConfiguration (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
											 unsigned_32 CfgAddress)

{	struct RequestCfgArgs 	RequestCfgCmd;
	struct RequestCfgArgs 	*ArgPointer;
	unsigned_16			 		ArgLength;
	unsigned_16			 		Status;

	RequestCfgCmd.Socket = CSSSocket;
	RequestCfgCmd.Attributes = 2;
	RequestCfgCmd.Vcc = 50;
	RequestCfgCmd.Vpp1 = 50;
	RequestCfgCmd.Vpp2 = 50;
	RequestCfgCmd.IntType = 2;
	RequestCfgCmd.ConfigBase = (unsigned_32)CfgAddress & 0x0000FFFF;
	RequestCfgCmd.Status = 0;
	RequestCfgCmd.Pin = 0;
	RequestCfgCmd.Copy = 0;
	RequestCfgCmd.ConfigIndex = 0x41;
	RequestCfgCmd.Present = 0x01;

	ArgLength = sizeof (struct RequestCfgArgs);
	ArgPointer = &RequestCfgCmd;

	Status = CardServices (	(unsigned_8) REQUEST_CFG_FN, 
									(unsigned_16 *) &ClientHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}


	

/***************************************************************************/
/*																									*/
/*	ReleaseConfiguration (CSSSocket, ClientHandle)									*/
/*																									*/
/*	INPUT:	CSSSocket	 	-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*																									*/
/*	OUTPUT:	Returns		 	-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will return the card and socket.				*/
/*																									*/
/***************************************************************************/

unsigned_16 ReleaseConfiguration (unsigned_16 CSSSocket, unsigned_16 ClientHandle)

{	struct ReleaseCfgArgs 	ReleaseCfgCmd;
	struct ReleaseCfgArgs 	*ArgPointer;
	unsigned_16			 		ArgLength;
	unsigned_16			 		Status;

	ReleaseCfgCmd.Socket = CSSSocket;

	ArgLength = sizeof (struct ReleaseCfgArgs);
	ArgPointer = &ReleaseCfgCmd;

	Status = CardServices (	(unsigned_8) RELEASE_CFG_FN, 
									(unsigned_16 *)&ClientHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}

 


/***************************************************************************/
/*																									*/
/*	RequestWindow (CSSSocket, CurrWindowHandle, WinBase)							*/
/*																									*/
/*	INPUT:	CSSSocket			-	Socket number of card.							*/
/*				CurrWindowHandle	-	Pointer to store returned window handle.	*/
/*				WinBase				-	Pointer to window base variable.				*/
/*																									*/
/*	OUTPUT:	Returns				-	Return code from card services.				*/
/*																									*/
/*	DESCRIPTION:	This procedure will request an 8K memory block.  If this	*/
/*		is not the first insertion, the original memory block will be asked	*/
/*		for again.																				*/
/*																									*/
/***************************************************************************/

unsigned_16 RequestWindow (unsigned_16 CSSSocket, 
									unsigned_16 *CurrWindowHandle, unsigned_32 *WinBase)

{	struct ReqWindowArgs		ReqWindowCommand;
	struct ReqWindowArgs 	*ArgPointer;
	unsigned_16					ArgLength;
	unsigned_16					Status;
	unsigned_16					TmpWindowHandle;

	ReqWindowCommand.Socket = CSSSocket;
	ReqWindowCommand.Attributes = 0x0006;
	ReqWindowCommand.Base = *WinBase;
	ReqWindowCommand.Size = MemSize;
	ReqWindowCommand.Speed = SPEED_150nS;

	ArgLength = sizeof (struct ReqWindowArgs);
	ArgPointer = &ReqWindowCommand;
	TmpWindowHandle = CSSClientHandle;

	Status = CardServices ( (unsigned_8) REQUEST_WINDOW_FN, 
									(unsigned_16 *) &TmpWindowHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	if (Status)
	{
		ReqWindowCommand.Attributes = 0x0006;
		ReqWindowCommand.Base = 0;
		TmpWindowHandle = CSSClientHandle;
		ArgPointer = &ReqWindowCommand;
		Status = CardServices ( (unsigned_8) REQUEST_WINDOW_FN, 
									(unsigned_16 *) &TmpWindowHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	}

	if (Status)
	{
		MemSize = 0x00004000;
		ReqWindowCommand.Attributes = 0x0006;
		ReqWindowCommand.Base = *WinBase;
		ReqWindowCommand.Size = MemSize;
		ArgPointer = &ReqWindowCommand;
		TmpWindowHandle = CSSClientHandle;

		Status = CardServices ( (unsigned_8) REQUEST_WINDOW_FN, 
									(unsigned_16 *) &TmpWindowHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
		if (Status)
		{
			ReqWindowCommand.Attributes = 0x0006;
			ReqWindowCommand.Base = 0;
			TmpWindowHandle = CSSClientHandle;
			ArgPointer = &ReqWindowCommand;
			Status = CardServices ( (unsigned_8) REQUEST_WINDOW_FN, 
									(unsigned_16 *) &TmpWindowHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
		}

	}

	*CurrWindowHandle = TmpWindowHandle;	
	*WinBase = ReqWindowCommand.Base;
	return (Status);
}
	



/***************************************************************************/
/*																									*/
/*	ReleaseWindow (WinHandle)																*/
/*																									*/
/*	INPUT: 	WinHandle	-	Window handle to release								*/
/*																									*/
/*	OUTPUT:	Returns		-	Return code from card services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will release the passed window by calling	*/
/*		the ReleaseWindow function of card services.									*/
/*																									*/
/***************************************************************************/

unsigned_16 ReleaseWindow (unsigned_16 WinHandle)

{	unsigned_16	Status;

	Status = CardServices (	(unsigned_8) RELEASE_WIN_FN, 
									(unsigned_16 *) &WinHandle, 
									(unsigned_32) 0, 
									(unsigned_16) 0, 
									(unsigned_32) 0	);
	return (Status);
}
	



/***************************************************************************/
/*																									*/
/*	MapMemPage (CurrWindowHandle)	  														*/
/*																									*/
/*	INPUT:	CurrWindowHandle	-	The memory window handle to use.				*/
/*																									*/
/*	OUTPUT:	Returns				-	Status code from Card Services.				*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the MapMemPage routine				*/
/*		of the card services to map the memory window allocated.					*/
/*																									*/
/***************************************************************************/

unsigned_16 MapMemPage (unsigned_16 CurrWindowHandle)

{	struct MapMemPageArgs	MapMemPageCommand;
	struct MapMemPageArgs 	*ArgPointer;
	unsigned_16				ArgLength;
	unsigned_16				Status;

	MapMemPageCommand.CardOffset = 0;
	MapMemPageCommand.Page = 0;

	ArgLength = sizeof (struct MapMemPageArgs);
	ArgPointer = &MapMemPageCommand;

	Status = CardServices (	(unsigned_8) MAPMEMPAGE_FN, 
									(unsigned_16 *) &CurrWindowHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	AddMemoryResource (WinBase)															*/
/*																									*/
/*	INPUT:	WinBase				-	Pointer to window base variable.				*/
/*																									*/
/*	OUTPUT:	Returns status code from Card Services.								*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to add the memory resource.  This is called to	*/
/*		ADD BACK the memory window allocated.											*/
/*																									*/
/***************************************************************************/

unsigned_16 AddMemoryResource (unsigned_32 WinBase)

{	struct AdjMemArgs		AdjustCommand;
	struct AdjMemArgs 	*ArgPointer;
	unsigned_16				ArgLength;
	unsigned_16				OwnersHandle;
	unsigned_16				Status;

	AdjustCommand.Action = ADD_RESOURCE;
	AdjustCommand.Resource = MEMORY_RANGE;
	AdjustCommand.Attributes = 0;
	AdjustCommand.Base = WinBase;
	AdjustCommand.WindowSize = MemSize;

	ArgLength = sizeof (struct AdjMemArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (	(unsigned_8) ADJUST_RESOURCE_FN, 
									(unsigned_16 *) &OwnersHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}

	


/***************************************************************************/
/*																									*/
/*	RemoveMemoryResource (WinBase)														*/
/*																									*/
/*	INPUT:	WinBase				-	Pointer to window base variable.				*/
/*																									*/
/*	OUTPUT:	Returns	-	Status code from Card Services.							*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to remove the memory resource.  This is called	*/
/*		to remove the resource from the database.										*/
/*																									*/
/***************************************************************************/

unsigned_16 RemoveMemoryResource (unsigned_32 WinBase)

{	struct AdjMemArgs	 	AdjustCommand;
	struct AdjMemArgs 	*ArgPointer;
	unsigned_16		 		ArgLength;
	unsigned_16		 		OwnersHandle;
	unsigned_16		 		Status;

	AdjustCommand.Action = REMOVE_RESOURCE;
	AdjustCommand.Resource = MEMORY_RANGE;
	AdjustCommand.Attributes = 0;
	AdjustCommand.Base = WinBase;
	AdjustCommand.WindowSize = MemSize;

	ArgLength = sizeof (struct AdjMemArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (	(unsigned_8) ADJUST_RESOURCE_FN, 
									(unsigned_16 *) &OwnersHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	RequestIO (CSSSocket, ClientHandle, IOAddr,							 			*/
/*				  IOAddr2, BusWidthVar)														*/
/*																									*/
/*	INPUT:	CSSSocket		-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*				IOAddr			-	Pointer to IO Port Address variable.			*/
/*				IOAddr2			-	Pointer to IO Port Address (not used).			*/
/*				BusWidthVar		-	Pointer to bus width variable (8 or 16)		*/
/*																									*/
/*	OUTPUT:	Returns			-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will request a contiguous set of 8			*/
/*		registers.  It will request for 16 bit ports, but if an error			*/
/*		occurs on the first insertion, it will try 8 bit ports.					*/
/*																									*/
/***************************************************************************/

unsigned_16 RequestIO (unsigned_16 CSSSocket,
							  unsigned_16 ClientHandle, unsigned_16 *IOAddr,
							  unsigned_16 *IOAddr2, unsigned_8 *BusWidthVar)

{	struct RequestIOArgs		RequestIOCmd;
	struct RequestIOArgs 	*ArgPointer;
	unsigned_16					ArgLength;
	unsigned_16					Status;

	RequestIOCmd.Socket = CSSSocket;

	do
	{	
		RequestIOCmd.BasePort1 = *IOAddr;

		RequestIOCmd.NumPorts1 = 8;

		if (*BusWidthVar == 16)
		{	RequestIOCmd.Attributes1 = 8;
		}
		else
		{	RequestIOCmd.Attributes1 = 0;
		}

		RequestIOCmd.BasePort2 = 0;
		RequestIOCmd.NumPorts2 = 0;
		RequestIOCmd.Attributes2 = 0;
		RequestIOCmd.IOAddrLines = 3;

		ArgLength = sizeof (struct RequestIOArgs);
		ArgPointer = &RequestIOCmd;

		Status = CardServices (	(unsigned_8) REQUEST_IO_FN, 
					(unsigned_16 *) &ClientHandle, 
					(unsigned_32) 0, 
					(unsigned_16) ArgLength, 
					(unsigned_32)ArgPointer );
		if (Status)
		{	*BusWidthVar -= 8;
			if (*BusWidthVar == 0 && (*IOAddr))
			{
				*BusWidthVar = 16;
				*IOAddr = 0;
			}
		}
	} while ((Status) && (*BusWidthVar));
 
	*IOAddr = RequestIOCmd.BasePort1;
	*IOAddr2 = RequestIOCmd.BasePort2;
	return (Status);
}



/***************************************************************************/
/*																									*/
/*	ReleaseIO (CSSSocket, ClientHandle, IOAddr, IOAddr2, BusWidthVar)			*/
/*																									*/
/*	INPUT:  	CSSSocket		-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*				IOAddr			-	IO Port Address to release.						*/
/*				IOAddr2			-	IO Port Address to release.						*/
/*				BusWidthVar		-	Bus width (8 or 16).									*/
/*																									*/
/*	OUTPUT:	Returns			-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will release the requested I/O addresses.	*/
/*																									*/
/***************************************************************************/

unsigned_16 ReleaseIO (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
							  unsigned_16 IOAddr, unsigned_16 IOAddr2,
							  unsigned_8 BusWidthVar)

{	struct ReleaseIOArgs		ReleaseIOCmd;
	struct ReleaseIOArgs 	*ArgPointer;
	unsigned_16					ArgLength;
	unsigned_16					Status;

	ReleaseIOCmd.Socket = CSSSocket;
	ReleaseIOCmd.BasePort1 = IOAddr;
	ReleaseIOCmd.NumPorts1 = 8;

	if (BusWidthVar == 16)
	{	ReleaseIOCmd.Attributes1 = 8;
	}
	else
	{	ReleaseIOCmd.Attributes1 = 0;
	}

	ReleaseIOCmd.BasePort2 = IOAddr2;
	ReleaseIOCmd.NumPorts2 = 0;
	ReleaseIOCmd.Attributes2 = 0;
	ReleaseIOCmd.IOAddrLines = 3;

	ArgLength = sizeof (struct ReleaseIOArgs);
	ArgPointer = &ReleaseIOCmd;

	Status = CardServices (	(unsigned_8) RELEASE_IO_FN, 
									(unsigned_16 *) &ClientHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	AddIOResource (IOAddr)																	*/
/*																									*/
/*	INPUT:	IOAddr		-	I/O Address to add back to resources.				*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to add back the IO resource.							*/
/*																									*/
/***************************************************************************/

unsigned_16 AddIOResource (unsigned_16 IOAddr)

{	struct AdjIOArgs 	AdjustCommand;
	struct AdjIOArgs 	*ArgPointer;
	unsigned_16	  		ArgLength;
	unsigned_16	  		OwnersHandle;
	unsigned_16	  		Status;

	AdjustCommand.Action = ADD_RESOURCE;
	AdjustCommand.Resource = IO_RANGE;
	AdjustCommand.BasePort = IOAddr;
	AdjustCommand.NumPorts = 8;
	AdjustCommand.Attributes = 0;
	AdjustCommand.IOAddrLines = 3;

	ArgLength = sizeof (struct AdjIOArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (	(unsigned_8)ADJUST_RESOURCE_FN, 
									(unsigned_16 *) &OwnersHandle, 
									(unsigned_32) 0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	RemoveIOResource (IOAddr)																*/
/*																									*/
/*	INPUT:	IOAddr		-	I/O Address remove from resources.					*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to remove the IO resource.								*/
/*																									*/
/***************************************************************************/

unsigned_16 RemoveIOResource (unsigned_16 IOAddr)

{	struct AdjIOArgs 	AdjustCommand;
	struct AdjIOArgs 	*ArgPointer;
	unsigned_16			ArgLength;
	unsigned_16			OwnersHandle;
	unsigned_16			Status;

	AdjustCommand.Action = REMOVE_RESOURCE;
	AdjustCommand.Resource = IO_RANGE;
	AdjustCommand.BasePort = IOAddr;
	AdjustCommand.NumPorts = 8;
	AdjustCommand.Attributes = 0;
	AdjustCommand.IOAddrLines = 3;

	ArgLength = sizeof (struct AdjIOArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (	(unsigned_8) ADJUST_RESOURCE_FN, 
									(unsigned_16 *) &OwnersHandle, 
									(unsigned_32)0, 
									(unsigned_16) ArgLength, 
									(unsigned_32)ArgPointer );
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	RequestIRQ (CSSSocket, ClientHandle, IRQNum) 									*/
/*																									*/
/*	INPUT:	CSSSocket		-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*				IRQNum			-	Pointer to IO Port Address variable.			*/
/*																									*/
/*	OUTPUT:	Returns			-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will request an interrupt line.				*/
/*																									*/
/***************************************************************************/

unsigned_16 RequestIRQ (unsigned_16 CSSSocket,
								unsigned_16 ClientHandle, unsigned_8 *IRQNum)

{	struct RequestIRQArgs 	RequestIRQCmd;
	struct RequestIRQArgs 	*ArgPointer;
	unsigned_16			 		ArgLength;
	unsigned_16			 		Status;

	RequestIRQCmd.Socket = CSSSocket;
	RequestIRQCmd.Attributes = 0;
	RequestIRQCmd.IRQInfo1 = 0x30;
	RequestIRQCmd.IRQInfo2 = (0x01 << *IRQNum);

	ArgLength = sizeof (struct RequestIRQArgs);
	ArgPointer = &RequestIRQCmd;

	Status = CardServices (	(unsigned_8) REQUEST_IRQ_FN, 
						(unsigned_16 *) &ClientHandle, 
						(unsigned_32) 0, 
						(unsigned_16) ArgLength, 
						(unsigned_32)ArgPointer	);
	if (Status)
	{
		RequestIRQCmd.Attributes = 0;
		RequestIRQCmd.IRQInfo2 = 0xFFFF;

		ArgLength = sizeof (struct RequestIRQArgs);
		ArgPointer = &RequestIRQCmd;

		Status = CardServices (	(unsigned_8) REQUEST_IRQ_FN, 
						(unsigned_16 *) &ClientHandle, 
						(unsigned_32) 0, 
						(unsigned_16) ArgLength, 
						(unsigned_32)ArgPointer	);
	}




	*IRQNum = RequestIRQCmd.AssignedIRQ;
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	ReleaseIRQ (CSSSocket, ClientHandle, IRQNum)			 							*/
/*																									*/
/*	INPUT: 	CSSSocket		-	Socket number of card.								*/
/*				ClientHandle	-	Handle number for our card.						*/
/*				IRQNum			-	Pointer to IO Port Address variable.			*/
/*																									*/
/*	OUTPUT:	Returns			-	Return code from card services.					*/
/*																									*/
/*	DESCRIPTION:	This procedure will release the allocated interrupt		*/
/*						line.																		*/
/*																									*/
/***************************************************************************/

unsigned_16 ReleaseIRQ (unsigned_16 CSSSocket, unsigned_16 ClientHandle,
								unsigned_8 IRQNum)

{	struct ReleaseIRQArgs 	ReleaseIRQCmd;
	struct ReleaseIRQArgs 	*ArgPointer;
	unsigned_16			 		ArgLength;
	unsigned_16			 		Status;

	ReleaseIRQCmd.Socket = CSSSocket;
	ReleaseIRQCmd.Attributes = 0;
	ReleaseIRQCmd.AssignedIRQ = IRQNum;

	ArgLength = sizeof (struct ReleaseIRQArgs);
	ArgPointer = &ReleaseIRQCmd;

	Status = CardServices (RELEASE_IRQ_FN, &ClientHandle, 0, ArgLength, (unsigned_32)ArgPointer);
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	AddIRQResource (IRQNum)			  														*/
/*																									*/
/*	INPUT:	IRQNum 		-	IRQ assigned to add back to resources.				*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to add back the IRQ resource.							*/
/*																									*/
/***************************************************************************/

unsigned_16 AddIRQResource (unsigned_8 IRQNum)

{	struct AdjIRQArgs		AdjustCommand;
	struct AdjIRQArgs 	*ArgPointer;
	unsigned_16				ArgLength;
	unsigned_16				OwnersHandle;
	unsigned_16				Status;

	AdjustCommand.Action = ADD_RESOURCE;
	AdjustCommand.Resource = IRQNum;
	AdjustCommand.Attributes = 0;
	AdjustCommand.IRQNum = IRQNum;

	ArgLength = sizeof (struct AdjIRQArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (ADJUST_RESOURCE_FN, &OwnersHandle, 0, ArgLength, (unsigned_32)ArgPointer);
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	RemoveIRQResource (IRQNum)				  												*/
/*																									*/
/*	INPUT:	IRQNum 		-	IRQ assigned to remove from resources.				*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the AdjustResourceInfo routine	*/
/*		of the card services to remove the IRQ resource.							*/
/*																									*/
/***************************************************************************/

unsigned_16 RemoveIRQResource (unsigned_8 IRQNum)

{	struct AdjIRQArgs		AdjustCommand;
	struct AdjIRQArgs 	*ArgPointer;
	unsigned_16				ArgLength;
	unsigned_16				OwnersHandle;
	unsigned_16				Status;

	AdjustCommand.Action = REMOVE_RESOURCE;
	AdjustCommand.Resource = IRQNum;
	AdjustCommand.Attributes = 0;
	AdjustCommand.IRQNum = IRQNum;

	ArgLength = sizeof (struct AdjIRQArgs);
	ArgPointer = &AdjustCommand;
	OwnersHandle = 0;

	Status = CardServices (ADJUST_RESOURCE_FN, &OwnersHandle, 0, ArgLength, (unsigned_32)ArgPointer);
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	GetFirstTuple (TupleArgs, TupleCode)												*/
/*																									*/
/*	INPUT:	TupleArgs	-	Pointer to GetFirstTuple argument structure		*/
/*				TupleCode	-	Tuple code to search for.								*/
/*																									*/
/*	OUTPUT:	Returns 		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the GetFirstTuple routine			*/
/*		of the card services to fill in the tuple argument structure which	*/
/*		will be used later to call GetTupleData.										*/
/*																									*/
/***************************************************************************/

unsigned_16 GetFirstTuple (struct GetFirstTupleArgs *ArgPointer, unsigned_8 TupleCode,
									unsigned_16 CSSSocket)

{	unsigned_16	Status;
	unsigned_16	ArgLength;
	unsigned_16	DummyClient;

	ArgPointer->Socket = CSSSocket;
	ArgPointer->Attributes = 0x0001;
	ArgPointer->DesiredTuple = TupleCode;
	ArgPointer->Reserved = 0;

	ArgLength = sizeof (struct GetFirstTupleArgs);

	Status = CardServices (GET_1ST_TUPLE_FN, &DummyClient, 0, ArgLength, (unsigned_32)ArgPointer);
	return (Status);
}




/***************************************************************************/
/*																									*/
/*	GetTupleData (TupleArgs)																*/
/*																									*/
/*	INPUT:	TupleArgs	-	Pointer to GetTupleData argument structure		*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will call the GetTupleData routine			*/
/*		of the card services to fill in the tuple argument structure which	*/
/*		was used earlier to call GetFirstTuple.										*/
/*																									*/
/***************************************************************************/

unsigned_16 GetTupleData (struct GetTupleDataArgs *TupleArgs)

{	unsigned_16		Status;
	unsigned_16		ArgLength;
	unsigned_16		DummyClient;

	TupleArgs->TupleOffset = 0;
	TupleArgs->TupleDataMax = 50;

	ArgLength = sizeof (struct GetTupleDataArgs);

	Status = CardServices (GET_TUPLE_DATA_FN, &DummyClient, 0, ArgLength, (unsigned_32)TupleArgs);
	return (Status);
}



/***************************************************************************/
/*																									*/
/*	CardServices (Function, Handle, ClientEntry, ArgLength, ArgPointer)		*/
/*																									*/
/*	INPUT:	Function		-	Function of Card Services to perform.				*/
/*				Handle		-	Pointer to Client Handle, Window Handle, etc.	*/
/*				ClientEntry	-	Pointer to callback routine for RegisterClient.	*/
/*				ArgLength	-	Length of arguments.										*/
/*				ArgPointer	-	Pointer to arguments.									*/
/*																									*/
/*	OUTPUT:	Returns		-	Status code from Card Services.						*/
/*																									*/
/*	DESCRIPTION:	This procedure will set up the registers and invoke the	*/
/*		interrupt to call card services.  Care must be taken with the Handle	*/
/*		parameter.  On functions that return a handle different from the		*/
/*		one passed, the destination pointer should be given, but should		*/
/*		first be initialized to the value of the input handle. 					*/
/*																									*/
/*		For example, in the RequestWindow function, WindowHandle should be	*/
/*		initialized to ClientHandle, and &WindowHandle should be passed to	*/
/*		CardServices ().																		*/
/*																									*/
/***************************************************************************/

unsigned_16 CardServices (unsigned_8 Function, unsigned_16 *Handle,
								  unsigned_32 ClientEntry, unsigned_16 ArgLength,
								  unsigned_32 ArgPointer)

{	unsigned_16		NewHandle;
	unsigned_16		OldHandle;
	unsigned_16		Status;

	OldHandle = *Handle;

//	__asm		push	bx
//	__asm		push	cx
//	__asm		push	dx
//	__asm		push	si
//	__asm		push	di
//	__asm		push	es
//	
//	__asm		les	si, ClientEntry
//	__asm		push	es
//	__asm		pop	di
//
//	__asm		mov	ah, CARD_SERVICES
//	__asm		mov	al, Function
//	__asm		les	bx, ArgPointer
//	__asm		mov	cx, ArgLength
//	__asm		mov	dx, OldHandle
//
//	__asm		int	1AH
// 
//	__asm		mov	NewHandle, dx
//	__asm		mov	Status, ax
//
//	__asm		pop	es
//	__asm		pop	di
//	__asm		pop	si
//	__asm		pop	dx
//	__asm		pop	cx
//	__asm		pop	bx

	*Handle = NewHandle;
	return (Status);
}

#endif /* NDIS_MINI_PORT */
