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
	message.c

	Pvides a mechanism for the operating system running on one processor
	to signal the operating system running on another processor.

	HISTORY
	#define	TR	Tilakraj Roy
	960530	TR 	Created
	960905	TR	Pulled in from host channel sources
	960907	TR	Moved packet queuing to higher layer
	960924	TR	Pulled in mailbox sources from IPC 
	961019	TR	Enabled in the new interface stuff.
	970520	TR	Generic Host-Communication for Apple
	970707	Tilakraj Roy	Pulled in form the Channel and VIntr sources.
    990511  DTO Ported to pSOS
*/

/*----------------------------------------------------------------------------
          SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
          DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmmanlib.h"
#include "tmmanapi.h"
#include <stdio.h>
/*	CONSTANTS */

#define		MessageManagerFourCC		tmmanOBJECTID ( 'M', 'S', 'G', 'M' )
#define		MessageFourCC				tmmanOBJECTID ( 'M', 'S', 'G', 'O' )

/* TYPDEFS */

typedef struct tagMessageObject
{
	GenericObject	Object;
	UInt32			SynchObject;
	Pointer			MessageManager;
	UInt32			MessageNumber;
	UInt32			ChannelHandle;
	UInt32			QueueHandle;
	UInt32			NameSpaceHandle;
	UInt32			ClientHandle;
}	MessageObject;

typedef struct tagMessageManagerObject
{
	GenericObject			Object;
	ObjectList				List;
	UInt32					Count;
	UInt32					PacketSize;
	UInt32					HalHandle;
	UInt32					ChannelManagerHandle;
	UInt32					NameSpaceManagerHandle;
	UInt32					CriticalSectionHandle;
}	MessageManagerObject;


/* GLOBAL DATA STRUCTURES */
/* NONE - key to object oriented programming */


/* FUNCTIONS */
void	messageChannelCallback ( 
	UInt32	ChannelHandle,
	Pointer Packet,		
	Pointer Context );

TMStatus	messageManagerCreate ( 
	messageManagerParameters* Parameters,
	UInt32* MessageManagerHandle )
{
	TMStatus		StatusCode;

	MessageManagerObject*	Manager;

	if ( ( Manager = objectAllocate ( 
		sizeof ( MessageManagerObject ),
		MessageManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:messageManagerCreate:objectAllcoate:FAIL\n"));
		StatusCode = statusObjectAllocFail; 
		goto messageManagerCreateExit1;
	}

	Manager->Count					= Parameters->MessageCount;
	Manager->PacketSize				= Parameters->PacketSize;
	Manager->HalHandle				= Parameters->HalHandle;
	Manager->ChannelManagerHandle	= Parameters->ChannelManagerHandle;
	Manager->NameSpaceManagerHandle	= Parameters->NameSpaceManagerHandle;

	if ( objectlistCreate ( &Manager->List,  
		Manager->Count ) != True )
	{
		DPF(0,("tmman:messageManagerCreate:objectlistCreate:FAIL\n"));
		StatusCode = statusObjectListAllocFail;
		goto	messageManagerCreateExit2;
	}

	*MessageManagerHandle  = (UInt32)Manager;

	return statusSuccess;

/*

messageManagerCreateExit3:
	objectlistDestroy ( &MessageManager->List );
*/

messageManagerCreateExit2:
	objectFree ( Manager );

messageManagerCreateExit1:
	return StatusCode;
}

TMStatus	messageManagerDestroy ( 
	UInt32 MessageManagerHandle )
{
	MessageManagerObject* Manager = 
		( MessageManagerObject* )MessageManagerHandle;


	if ( objectValidate ( Manager, MessageManagerFourCC ) != True )
	{
		DPF(0,("tmman:messageManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	objectlistDestroy ( &Manager->List );

	objectFree ( Manager );

	return statusSuccess;
}

TMStatus  messageManagerDestroyMessageByClient (
	UInt32	MessageManagerHandle, 
	UInt32 ClientHandle )
{
	MessageManagerObject*	Manager = 
		( MessageManagerObject* )MessageManagerHandle;
	MessageObject*	Object;
	UInt32	Idx;

	for ( Idx = 0 ; Idx < Manager->Count ; Idx++ )
	{
		Object = objectlistGetObject (
			&Manager->List, 
			Idx );
		if ( ( Object ) && Object->ClientHandle == ClientHandle )
		{

			messageDestroy ( (UInt32) Object );
		}
	}

	return statusSuccess;
}

TMStatus	messageCreate (
	UInt32	MessageManagerHandle,
	Pointer	ListHead,
	String	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* MessageHandlePointer )
{
	MessageManagerObject* Manager = 
		( MessageManagerObject* )MessageManagerHandle;
	MessageObject*	Message;
	TMStatus	StatusCode;
	UInt32		NameSpaceHandle;
	UInt32		MessageNumber;
	UInt8		ChannelName[constTMManNameSpaceNameLength];


	if ( objectValidate ( Manager, MessageManagerFourCC ) != True )
	{
		DPF(0,("tmman:messageCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( ( StatusCode = namespaceCreate  (
		Manager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectMessage,
		Name,
		&MessageNumber,
		&NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:messageCreate:namaspaceCreate:FAIL[%lx]\n",
			StatusCode));
		goto messageCreateExit1;	
	}

	if ( ( Message = objectAllocate (
		sizeof ( MessageObject ), MessageFourCC ) ) == Null )
	{
		DPF(0,("tmman:messageCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto messageCreateExit2;	
	}

	if ( objectlistInsert ( 
		&Manager->List, 
		Message,
		MessageNumber ) != True )
	{
		DPF(0,("tmman:messageCreate:objectlstInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto messageCreateExit3;	
	}

	if ( syncobjCreate ( 
		SynchronizationFlags,
		SynchronizationHandle,
		&Message->SynchObject,
		Name ) != True )
	{
		DPF(0,("tmman:messageCreate:syncobjCreate:FAIL\n"));
		StatusCode = statusSynchronizationObjectCreateFail;
		goto messageCreateExit4;
	}

	/* 
		make the name unique by putting the calling object type
		before the name.
	*/

	sprintf ( ChannelName, "%d\\%s", 
		constTMManNameSpaceObjectMessage, Name );

	if ( ( StatusCode = channelCreate (
		Manager->ChannelManagerHandle,
		ChannelName, 
		messageChannelCallback, 
		Message,
		&Message->ChannelHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:messageCreate:channelCreate:FAIL[%lx]\n",
			StatusCode));
		goto messageCreateExit5;
	}


	if ( queueCreate (
		Manager->PacketSize,
		Manager->HalHandle,
		&Message->QueueHandle ) != True )
	{
		DPF(0,("tmman:messageCreate:queueCreate:FAIL\n"));
		StatusCode = statusQueueObjectCreateFail;
		goto messageCreateExit6;
	}

	Message->MessageManager = Manager;
	Message->MessageNumber = MessageNumber;
	Message->NameSpaceHandle = NameSpaceHandle;
	Message->ClientHandle = (UInt32)ListHead;

	*MessageHandlePointer = (UInt32)Message;

	return statusSuccess;

/*
messageCreateExit7 :
	queueDestroy ( Message->QueueHandle );
*/

messageCreateExit6 :
	channelDestroy ( Message->ChannelHandle );


messageCreateExit5 :
	syncobjDestroy ( 
		Message->SynchObject );


messageCreateExit4 :
	objectlistDelete ( 
		&Manager->List, 
		Message,
		MessageNumber );


messageCreateExit3 :
	objectFree ( Message );

messageCreateExit2 :
	namespaceDestroy ( NameSpaceHandle );

messageCreateExit1 :
	return StatusCode;
}

TMStatus	messageDestroy ( 
	UInt32 MessageHandle )
{
	MessageManagerObject*	MessageManager;
	MessageObject*	Message =
		(MessageObject*) MessageHandle;
	UInt32	NameSpaceHandle;


	if ( objectValidate ( Message, MessageFourCC ) != True )
	{
		DPF(0,("tmman:messageDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	/* signal the obejct so that a waiting thread can unblock */
	syncobjSignal ( Message->SynchObject );

	MessageManager = (MessageManagerObject* )Message->MessageManager;
	NameSpaceHandle = Message->NameSpaceHandle;

	queueDestroy ( Message->QueueHandle );

	channelDestroy ( Message->ChannelHandle );

	syncobjDestroy ( 
		Message->SynchObject );

	objectlistDelete ( 
		&MessageManager->List, 
		Message,
		Message->MessageNumber );

	objectFree ( Message );

	namespaceDestroy ( NameSpaceHandle );

	return statusSuccess;
}

TMStatus	messageReceive ( 
	UInt32 MessageHandle,
	Pointer Data )
{
	MessageObject*	Message =
		(MessageObject*) MessageHandle;


	if ( objectValidate ( Message, MessageFourCC ) != True )
	{
		DPF(0,("tmman:messageReceive:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	if ( queueDelete ( Message->QueueHandle, Data ) != True )
	{
		/* DPF(0,("tmman:messageReceive:queueDelete:FAIL\n")); */
		return statusMessageQueueEmptyError;
	}
	return statusSuccess;
}

TMStatus	messageSend( 
	UInt32 MessageHandle, 
	Pointer Data )
{
	MessageObject*	Message =
		(MessageObject*) MessageHandle;
	TMStatus	StatusCode;


	if ( objectValidate ( Message, MessageFourCC ) != True )
	{
		DPF(0,("tmman:messageSend:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	if ( ( StatusCode = channelSend(
		Message->ChannelHandle, 
		Data ) ) != statusSuccess )
	{
		DPF(0,("tmman:messageSend:channelSend:FAIL[%lx]\n",
			StatusCode));
		return StatusCode;
	}

	return statusSuccess;
}

//DL Added This function is normally not part of tmman
TMStatus	messageWait( 
	UInt32 MessageHandle)
{
	MessageObject*	Message =
		(MessageObject*) MessageHandle;
	/*
	unused variable
	TMStatus	StatusCode;
        Bool            error;
	*/

	if ( objectValidate ( Message, MessageFourCC ) != True )
	{
		DPF(0,("tmman:messageSend:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	syncobjWait(Message->SynchObject);

	return statusSuccess;
}











void	messageChannelCallback ( 
	UInt32	ChannelHandle,
	Pointer Packet,		
	Pointer Context )	
{
	MessageObject*	Message =
		(MessageObject*) Context;

	/* unused parameter */
	(void)ChannelHandle;

	if ( objectValidate ( Message, MessageFourCC ) != True )
	{
		DPF(0,("tmman:messageChannelCallback:objectValidate:FAIL\n"));
		return;
	}

	if ( queueInsert ( Message->QueueHandle, Packet ) != True )
	{
		/* message queue full */
		DPF(0,("tmman:messageChannelCallback:queueInsert:FAIL\n"));
		return;
	}
	/*
		in spite of an error we do this here for the following reason
		if the message queue is full maybe the application has missed
		the event. So we make sure we trigger the event hoping the 
		applicaiton will wake up and have its breakfast out of the queue
	*/
	syncobjSignal ( Message->SynchObject );
}


