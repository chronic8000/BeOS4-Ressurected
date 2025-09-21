/*---------------------------------------------------------------------------- 
COPYRIGHT (c) 1997 by Philips Semiconductors

THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED AND COPIED IN 
ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH A LICENSE AND WITH THE 
INCLUSION OF THE THIS COPY RIGHT NOTICE. THIS SOFTWARE OR ANY OTHER COPIES 
OF THIS SOFTWARE MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER
PERSON. THE OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED. 

THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT ANY PRIOR NOTICE
AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY Philips Semiconductor. 

PHILIPS ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF THIS SOFTWARE
ON PLATFORMS OTHER THAN THE ONE ON WHICH THIS SOFTWARE IS FURNISHED.
----------------------------------------------------------------------------*/
/*
	channel.c

	Provides multiple bidirectional packet transport channels across
	a shared memroy architecture system.

	HISTORY
	#define	TR	Tilakraj Roy
	960530	TR 	Created
	960905	TR	Pulled in from host channel sources
	960907	TR	Moved packet queuing to higher layer
	960924	TR	Pulled in mailbox sources from IPC 
	961019	TR	Enabled in the new interface stuff.
	970520	TR	Generic Host-Communication for Apple
	980303	TR	Changed Debug Statement to excude Level
    990511  DTO Ported to pSOS
*/

/*----------------------------------------------------------------------------
          SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
          DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmmanapi.h"
#include "tmmanlib.h"

/*	CONSTANTS */
#define		constChannelMailboxReady	1
#define		constChannelHandlerReady	2

#define		ChannelManagerFourCC		tmmanOBJECTID ( 'C', 'H', 'N', 'M' )
#define		ChannelFourCC				tmmanOBJECTID ( 'C', 'H', 'N', 'L' )

/* TYPDEFS */

/* This structure should be accessed only using the ACCESSXX() macros */
typedef struct	tagChannelPacket
{
	 UInt32	volatile Sequence;
	 UInt32	volatile ChannelID;
}	ChannelPacket;

typedef struct	tagChannelMailboxControl
{
	UInt32	volatile ReadIndex;
	UInt32	volatile WriteIndex;	
	UInt32	volatile Command;
}	ChannelMailboxControl;

typedef struct tagChannelObject
{
	GenericObject	Object;
	ChannelHandler	Handler;
	Pointer			Context;
	Pointer			ChannelManager;
	UInt32			ChannelNumber;
	UInt32			NameSpaceHandle;
}	ChannelObject;

typedef struct tagChannelManagerObject
{
	GenericObject			Object;
	ObjectList				List;
	UInt32					ChannelCount;
	UInt32					MailboxCount;
	UInt32					PacketSize;
	UInt8*					SharedData;
	UInt32					MailslotSize;
	UInt16					PacketSendSeq;
	UInt16					PacketRecvSeq;
	UInt32					FreeChannelCount;
	ChannelMailboxControl*	ToPeerMailboxControl;
	ChannelMailboxControl*	ToSelfMailboxControl;
	UInt8*					ToPeerMailbox;
	UInt8*					ToSelfMailbox;
	UInt32					HalHandle;
	UInt32					VIntrManagerHandle;
	UInt32					VIntrHandle;
	UInt32					NameSpaceManagerHandle;
	UInt32					CriticalSectionHandle;
}	ChannelManagerObject;


/* GLOBAL DATA STRUCTURES */
/* NONE - key to object oriented programming */


/* FUNCTIONS */
void	chnlVIntrHandler ( 
	Pointer Context );

UInt32	channelSharedDataSize ( 
	UInt32 MailboxCount, 
	UInt32 PacketSize )
{
	return  ( ( sizeof ( ChannelMailboxControl ) + 
		( PacketSize  + sizeof ( ChannelPacket ) ) * MailboxCount ) * 2 );
}

TMStatus	channelManagerReset ( 
	UInt32 ChannelManagerHandle )
{
	ChannelManagerObject*	ChannelManager = 
		( ChannelManagerObject* )ChannelManagerHandle;
			
	halAccessEnable( ChannelManager->HalHandle );
	ChannelManager->ToPeerMailboxControl->ReadIndex = halAccess32(ChannelManager->HalHandle, 0);
	ChannelManager->ToPeerMailboxControl->WriteIndex = halAccess32(ChannelManager->HalHandle, 0);
	ChannelManager->ToPeerMailboxControl->Command = halAccess32(ChannelManager->HalHandle, 0);

	ChannelManager->ToSelfMailboxControl->ReadIndex = halAccess32(ChannelManager->HalHandle, 0);
	ChannelManager->ToSelfMailboxControl->WriteIndex = halAccess32(ChannelManager->HalHandle, 0);
	ChannelManager->ToSelfMailboxControl->Command = halAccess32(ChannelManager->HalHandle, 0);
	halAccessDisable( ChannelManager->HalHandle );

	ChannelManager->PacketSendSeq = 0;
	ChannelManager->PacketRecvSeq = 0;

	return statusSuccess;
}

TMStatus	channelManagerCreate (
	channelManagerParameters*	Parameters,
	UInt32*	ChannelManagerPointer )
{
	ChannelManagerObject*	ChannelManager;	
	TMStatus	StatusCode;

	if ( ( ChannelManager = objectAllocate ( 
		sizeof ( ChannelManagerObject ),
		ChannelManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:channelManagerCreate:objectAllocate:FAIL\n" ));
		StatusCode = statusObjectAllocFail; 
		goto channelManagerCreateExit1;
	}

	ChannelManager->ChannelCount	= Parameters->ChannelCount;
	ChannelManager->MailboxCount	= Parameters->MailboxCount;
	ChannelManager->PacketSize		= Parameters->PacketSize;
	ChannelManager->SharedData		= Parameters->SharedData;
	ChannelManager->HalHandle		= Parameters->HalHandle;
	ChannelManager->VIntrManagerHandle	= Parameters->VIntrManagerHandle;
	ChannelManager->NameSpaceManagerHandle	= Parameters->NameSpaceManagerHandle;

	ChannelManager->MailslotSize	= 
		( ChannelManager->PacketSize  + sizeof ( ChannelPacket ) );


	if ( objectlistCreate ( &ChannelManager->List,  
		ChannelManager->ChannelCount ) != True )
	{
		DPF(0,("tmman:channelManagerCreate:objectlistCreate:FAIL\n" ));
		StatusCode = statusObjectListAllocFail;
		goto	channelManagerCreateExit2;
	}

	#ifdef TMMAN_HOST

	ChannelManager->ToPeerMailboxControl =  
		(ChannelMailboxControl*)ChannelManager->SharedData;
	ChannelManager->ToSelfMailboxControl = 
		(ChannelMailboxControl*) ( ChannelManager->SharedData + 
		( sizeof ( ChannelMailboxControl ) + 
		ChannelManager->MailslotSize * ChannelManager->MailboxCount ) );
	#else

	ChannelManager->ToSelfMailboxControl =
		(ChannelMailboxControl*)ChannelManager->SharedData;
	ChannelManager->ToPeerMailboxControl =  
		(ChannelMailboxControl*)( ChannelManager->SharedData + 
		( sizeof ( ChannelMailboxControl ) + 
		ChannelManager->MailslotSize * ChannelManager->MailboxCount ) );

	#endif

	ChannelManager->ToPeerMailbox = 
		( ( (UInt8*)ChannelManager->ToPeerMailboxControl )  + 
		sizeof ( ChannelMailboxControl ) );

	ChannelManager->ToSelfMailbox = 
		( ( (UInt8*)ChannelManager->ToSelfMailboxControl )  + 
		sizeof ( ChannelMailboxControl ) );


	if ( ( StatusCode = vintrCreate  (
		ChannelManager->VIntrManagerHandle,
		0,
		chnlVIntrHandler,
		(Pointer)ChannelManager,
		(UInt32*)&ChannelManager->VIntrHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:channelManagerCreate:vintrCreate:FAIL[%lx]\n",
			StatusCode ));
		goto channelManagerCreateExit3;
	}

	if ( critsectCreate ( &ChannelManager->CriticalSectionHandle ) == False )
	{
		StatusCode = statusCriticalSectionCreateFail;
		goto channelManagerCreateExit4;
	}

/*	channelManagerReset((UInt32)ChannelManager); */

	*ChannelManagerPointer = (UInt32)ChannelManager;
		
	return statusSuccess;

/*
channelManagerCreateExit5: 
	critsectDestroy ( ChannelManager->CriticalSectionHandle );
*/

channelManagerCreateExit4: 
	vintrDestroy ( ChannelManager->VIntrHandle );

channelManagerCreateExit3:
	objectlistDestroy ( &ChannelManager->List );

channelManagerCreateExit2:
	objectFree ( ChannelManager );

channelManagerCreateExit1:
	return StatusCode;
}

TMStatus	channelManagerDestroy ( UInt32 ChannelManagerHandle )
{
	ChannelManagerObject*	ChannelManager = 
		( ChannelManagerObject* )ChannelManagerHandle;


	if ( objectValidate ( ChannelManager, ChannelManagerFourCC ) != True )
	{
		DPF(0,("tmman:channelManagerDestroy:objectValidate:FAIL\n" ));
		return statusInvalidHandle;
	}

	critsectDestroy ( ChannelManager->CriticalSectionHandle );

	vintrDestroy ( ChannelManager->VIntrHandle );

	objectlistDestroy ( &ChannelManager->List );

	objectFree ( ChannelManager );

	return statusSuccess;
}

TMStatus	channelCreate(
	UInt32	ChannelManagerHandle,
	String	Name,
	Pointer	Handler, 
	Pointer	Context,
	UInt32*	ChannelHandlePointer )
{
	ChannelManagerObject*	ChannelManager = 
		( ChannelManagerObject* )ChannelManagerHandle;
	ChannelObject*	Channel;
	TMStatus	StatusCode;
	UInt32		NameSpaceHandle;
	UInt32		ChannelNumber;

	/* DPF(13,("tmman:channelCreate\n" )); */

	if ( objectValidate ( ChannelManager, ChannelManagerFourCC ) != True )
	{
		DPF(0,("tmman:channelCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( !Handler ) 
	{
		DPF(0,("tmman:channelCreate:(!Handler):FAIL\n" ));
		return statusInvalidHandle;
	}


	if ( ( StatusCode = namespaceCreate  (
		ChannelManager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectChannel,
		Name,
		&ChannelNumber,
		&NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:channelCreate:nameSpaceCreate:FAIL[%lx]\n",
			StatusCode ));
		goto channelCreateExit1;	
	}

	if ( ( Channel = objectAllocate (
		sizeof ( ChannelObject ), ChannelFourCC ) ) == Null )
	{
		DPF(0,("tmman:channelCreate:objectAllocate:FAIL\n" ));
		StatusCode = statusObjectAllocFail;
		goto channelCreateExit2;
	}

	if ( objectlistInsert ( 
		&ChannelManager->List, 
		Channel,
		ChannelNumber ) != True )
	{
		DPF(0,("tmman:channelCreate:objectListInsert:FAIL\n" ));
		StatusCode = statusObjectInsertFail;
		goto channelCreateExit3;	
	}

	/*
	DPF(13,("tmman:channelCreate:Number[%x]:&List->Object[%x]:Object[%x]\n",
		ChannelNumber, &ChannelManager->List.Objects[ChannelNumber],   Channel ));

	*/

	Channel->ChannelManager	= ChannelManager;
	Channel->ChannelNumber	= ChannelNumber;
	Channel->Handler		= Handler;
	Channel->Context		= Context;
	Channel->NameSpaceHandle = NameSpaceHandle;
	
	*ChannelHandlePointer = (UInt32)Channel;

	return statusSuccess;

/*
channelCreateExit4 :
	objectlistDelete ( 
		&ChannelManager->List, 
		Channel,
		ChannelNumber );
*/	

channelCreateExit3:
	objectFree ( Channel );

channelCreateExit2:
	namespaceDestroy ( NameSpaceHandle );

channelCreateExit1:
	return StatusCode;

}

TMStatus	channelDestroy ( 
	UInt32 ChannelHandle)
{
	ChannelManagerObject*	ChannelManager;
	ChannelObject*	Channel =
		(ChannelObject*) ChannelHandle;
	UInt32	NameSpaceHandle;
	
	/* DPF(13,("tmman:channelDestroy\n" )); */

	if ( objectValidate ( Channel, ChannelFourCC ) != True )
	{
		DPF(0,("tmman:channelDestroy:objectVaidate:FAIL\n"));
		return statusInvalidHandle;
	}

	ChannelManager = (ChannelManagerObject* )Channel->ChannelManager;
	NameSpaceHandle = Channel->NameSpaceHandle;

	objectlistDelete ( 
		&ChannelManager->List, 
		Channel,
		Channel->ChannelNumber );

	objectFree ( Channel );

	namespaceDestroy ( NameSpaceHandle );

	return	statusSuccess;
}


TMStatus	channelSend(
	UInt32 ChannelHandle, 
	Pointer Packet )
{
	ChannelPacket*	MailslotPointer;
	UInt32			WriteIndex, ReadIndex;
	TMStatus		StatusCode;
	ChannelObject*	Channel =
		(ChannelObject*) ChannelHandle;
	ChannelManagerObject*	ChannelManager;

	UInt32	NestedContext;
	UInt32	TempValue1, TempValue2;


	if ( objectValidate ( Channel, ChannelFourCC ) != True )
	{
		DPF(0,("tmman:channelSend:objectVaidate:FAIL\n"));
		return statusInvalidHandle;
	}
	
	ChannelManager = Channel->ChannelManager;


	/* ENTER CRITICAL SECITON */

	critsectEnter ( ChannelManager->CriticalSectionHandle, &NestedContext );

	/* sequence number inseration has to be atomic */
/*
	DPF(13,("channel:Send:ToPeer:MBOX&[%x]:WR&[%x]:RD&[%x]:\n",
		halAccess32( ChannelManager->HalHandle, (UInt32)ChannelManager->ToPeerMailbox ),
		halAccess32( ChannelManager->HalHandle, (UInt32)&ChannelManager->ToPeerMailboxControl->WriteIndex) ,
		halAccess32( ChannelManager->HalHandle, (UInt32)&ChannelManager->ToPeerMailboxControl->ReadIndex ) ));
*/
	halAccessEnable( ChannelManager->HalHandle );

	WriteIndex =  halAccess32 ( ChannelManager->HalHandle,
		ChannelManager->ToPeerMailboxControl->WriteIndex );
	ReadIndex = halAccess32( ChannelManager->HalHandle, 
		ChannelManager->ToPeerMailboxControl->ReadIndex );

	halAccessDisable( ChannelManager->HalHandle );

	WriteIndex = 
		( WriteIndex + 1 ) % ( ChannelManager->MailboxCount );

	if ( WriteIndex == ReadIndex )
	{
		/*
		unused variable
		UInt32 Idx;
		*/
		/* no point processing the other channels if mailbox has no room */

		DPF(0,("tmman:channelSend:FAIL\n"));

		/*
		DPF(13,("channel:Queue:Dump:"));
		
		for ( Idx = 0 ; Idx < ChannelManager->MailboxCount ; Idx ++ )
		{
			MailslotPointer = ( ((UInt8*)ChannelManager->ToPeerMailbox) + 
				( Idx * ChannelManager->MailslotSize )  );

			DPF(13,("[%x]", *(UInt32*)(MailslotPointer+1) ));
		}

		DPF(13,("\n"));
		*/

		StatusCode = statusChannelMailboxFullError;
		/* LEAVE CRITICAL SECITON */
		critsectLeave ( ChannelManager->CriticalSectionHandle, &NestedContext );
		goto chnlSendExit1;
	}

	halAccessEnable( ChannelManager->HalHandle );

	MailslotPointer = 
		(ChannelPacket*)( ((UInt8*)ChannelManager->ToPeerMailbox) + 
		( halAccess32 ( ChannelManager->HalHandle,
		ChannelManager->ToPeerMailboxControl->WriteIndex ) * 
		ChannelManager->MailslotSize) );

	MailslotPointer->ChannelID	= halAccess32 ( ChannelManager->HalHandle, 
		Channel->ChannelNumber );
	MailslotPointer->Sequence =	halAccess32 ( ChannelManager->HalHandle,
		TempValue1 = ChannelManager->PacketSendSeq );

	halAccessDisable( ChannelManager->HalHandle );

	ChannelManager->PacketSendSeq++;

	/* the user should take care of endianness of packet data */
	halAccessEnable(ChannelManager->HalHandle);
	memCopy ( 
		(Pointer)(MailslotPointer + 1), /* point to the packet struct in the mailslot */
		(Pointer)Packet, 
		ChannelManager->PacketSize );
	TempValue2 = *(UInt32*)(MailslotPointer+1);
	halAccessDisable(ChannelManager->HalHandle);



	DPF(13,("channel:Send:Packet:ID[%lx]:SQ[%lx]:DT[%lx]:WR[%lx]:RD[%lx]\n",
		Channel->ChannelNumber,
		TempValue1,
		TempValue2,
		WriteIndex,
		ReadIndex ));

	halAccessEnable( ChannelManager->HalHandle );

	ChannelManager->ToPeerMailboxControl->WriteIndex = 
		halAccess32 ( ChannelManager->HalHandle, WriteIndex );

	ChannelManager->ToPeerMailboxControl->Command = 
		halAccess32( ChannelManager->HalHandle, constChannelMailboxReady );

	halAccessDisable( ChannelManager->HalHandle );

	vintrGenerateInterrupt ( ChannelManager->VIntrHandle );

	/* LEAVE CRITICAL SECTION */
	critsectLeave ( ChannelManager->CriticalSectionHandle, &NestedContext );

	return	statusSuccess;

chnlSendExit1:
	return	StatusCode;
}



void	chnlVIntrHandler ( 
	Pointer Context )
{
	/* MailSlot is in Switched Endian Format */
	ChannelPacket	Mailslot;
	UInt8			Packet[constTMManPacketSize];
	ChannelPacket*	MailslotPointer;
	UInt32			IdxPacket;
	UInt32			ReadIndex, WriteIndex, Command;
	ChannelManagerObject* ChannelManager = 
		(ChannelManagerObject*)Context;
	ChannelObject*	Channel;
	UInt32			NestedContext;

	/*DPF(13,( "CH{")); 
	DPF(13,("tmman:chnlVIntrHandler:List1[%x]\n",
		ChannelManager->List.Objects[1] ));
	*/

	for ( IdxPacket = 0 ; ; IdxPacket++  )
	{
		/* ENTER CRITICAL SECTION */

		critsectEnter ( ChannelManager->CriticalSectionHandle, &NestedContext );

		halAccessEnable(ChannelManager->HalHandle);

		ReadIndex = halAccess32( ChannelManager->HalHandle, 
			ChannelManager->ToSelfMailboxControl->ReadIndex );

		WriteIndex = halAccess32( ChannelManager->HalHandle,
			ChannelManager->ToSelfMailboxControl->WriteIndex );

		halAccessDisable(ChannelManager->HalHandle);

		if ( ReadIndex == WriteIndex )
		{
			critsectLeave ( ChannelManager->CriticalSectionHandle, &NestedContext );
			break;
		}


		/* get the pointer to the mailslot data structure from the shared mailbox */
		MailslotPointer = (ChannelPacket*)( ((UInt8*)ChannelManager->ToSelfMailbox) + 
			(ReadIndex * ChannelManager->MailslotSize) );


		halAccessEnable(ChannelManager->HalHandle);
		
		Mailslot.ChannelID = 
			halAccess32( ChannelManager->HalHandle, MailslotPointer->ChannelID );

		Mailslot.Sequence = 
			halAccess32( ChannelManager->HalHandle, MailslotPointer->Sequence );

		halAccessDisable(ChannelManager->HalHandle);

		/* the user should take care of endianness of packet data */
		halAccessEnable(ChannelManager->HalHandle);
		memCopy ( 
			(Pointer)&Packet, 
			(Pointer)(MailslotPointer + 1), 
			ChannelManager->PacketSize );
		halAccessDisable(ChannelManager->HalHandle);

		DPF(13,("channel:Recv:Packet:ID[%lx]:SQ[%lx]:DT[%lx]:WR[%lx]:RD[%lx]\n",
			Mailslot.ChannelID,		
			Mailslot.Sequence,
			*(UInt32*)(Packet),
			WriteIndex,
			ReadIndex ));


		ReadIndex = ( ReadIndex + 1 ) % ( ChannelManager->MailboxCount );

		halAccessEnable(ChannelManager->HalHandle);

		ChannelManager->ToSelfMailboxControl->ReadIndex = 
			halAccess32( ChannelManager->HalHandle, ReadIndex );

		halAccessDisable(ChannelManager->HalHandle);

		/* LEAVE CRITICAL SECTION */
		critsectLeave ( ChannelManager->CriticalSectionHandle, &NestedContext );

		Channel = (ChannelObject* )objectlistGetObject ( 
			&ChannelManager->List, 
			Mailslot.ChannelID );
		if ( ! Channel )
		{
			DPF(0,("tmman:chnlVIntrHandler:Invalid Channel[%lx]:FAIL\n",
				Mailslot.ChannelID ));
			/*
			for ( Idx = 0 ; Idx < ChannelManager->List.Count ; Idx ++ )
			{
				DPF(0,("tmman:chnlVIntrHandler:ChannelList#[%x]:[%x]:FAIL\n",
					Idx, ChannelManager->List.Objects[Idx] ));
			} 

			while(1);

			*/

			/* 
			we have received a packet for a channel that has not 
			yet been opened 
			*/
			continue;
		}

		Channel->Handler (
			Mailslot.ChannelID,
			(Pointer)&Packet,
			Channel->Context );
	}


	/* ENTER CRITICAL SECION */

	critsectEnter ( ChannelManager->CriticalSectionHandle, &NestedContext );

	halAccessEnable(ChannelManager->HalHandle);

	Command = halAccess32( ChannelManager->HalHandle, 
		ChannelManager->ToSelfMailboxControl->Command );
	
	halAccessDisable(ChannelManager->HalHandle);


	switch ( Command )
	{
		case constChannelHandlerReady :
		
		halAccessEnable(ChannelManager->HalHandle);

		ReadIndex = halAccess32 ( ChannelManager->HalHandle,
			ChannelManager->ToPeerMailboxControl->ReadIndex );

		WriteIndex = halAccess32 ( ChannelManager->HalHandle,
			ChannelManager->ToPeerMailboxControl->WriteIndex  );

		halAccessDisable(ChannelManager->HalHandle);

		if ( ReadIndex != WriteIndex )
		{
			halAccessEnable(ChannelManager->HalHandle);
			ChannelManager->ToPeerMailboxControl->Command =
				halAccess32 ( ChannelManager->HalHandle,
				constChannelMailboxReady );
			halAccessDisable(ChannelManager->HalHandle);
			vintrGenerateInterrupt ( ChannelManager->VIntrHandle );
		}
		break;

		case constChannelMailboxReady :
		halAccessEnable(ChannelManager->HalHandle);

		if ( ReadIndex == WriteIndex )
		{
			ChannelManager->ToPeerMailboxControl->Command =
				halAccess32 ( ChannelManager->HalHandle,
				constChannelHandlerReady );
		}
		else
		{
			ChannelManager->ToPeerMailboxControl->Command =
				halAccess32 ( ChannelManager->HalHandle, 
				constChannelMailboxReady );
		}
		halAccessDisable(ChannelManager->HalHandle);

		vintrGenerateInterrupt ( ChannelManager->VIntrHandle );
		break;

		default :
		break;
	}

	/* LEAVE CRITICAL SECITON */
	critsectLeave ( ChannelManager->CriticalSectionHandle, &NestedContext );

/*	DPF(13,("}CH")); */
}


