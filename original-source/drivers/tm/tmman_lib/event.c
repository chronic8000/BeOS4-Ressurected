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
	event.c

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
	970707	TR	Pulled in form the Channel and VIntr sources.
    990511  DTO Ported to pSOS
*/

/*----------------------------------------------------------------------------
          SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
          DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmmanlib.h"

/*	CONSTANTS */

#define		EventManagerFourCC		tmmanOBJECTID ( 'E', 'V', 'N', 'M' )
#define		EventFourCC				tmmanOBJECTID ( 'E', 'V', 'N', 'T' )

/* TYPDEFS */


typedef struct EventControl
{
	UInt32	volatile Signal;
	UInt32	volatile Reset;	
}	EventControl;

typedef struct tagEventObject
{
	GenericObject	Object;
	UInt32			SynchObject;
	Pointer			EventManager;
	UInt32			EventNumber;
	UInt32			NameSpaceHandle;
	UInt32			ClientHandle;
}	EventObject;

typedef struct tagEventManagerObject
{
	GenericObject			Object;
	ObjectList				List;
	UInt32					EventCount;
	UInt8*					SharedData;
	EventControl*			ToPeer;
	EventControl*			ToSelf;
	UInt32					HalHandle;
	UInt32					VIntrManagerHandle;
	UInt32					VIntrHandle;
	UInt32					NameSpaceManagerHandle;
	UInt32					CriticalSectionHandle;
}	EventManagerObject;


/* GLOBAL DATA STRUCTURES */
/* NONE - key to object oriented programming */


/* FUNCTIONS */
void	eventVIntrHandler ( 
	Pointer Context );

UInt32	eventSharedDataSize ( 
	UInt32 EventCount )
{
	return  ( ( sizeof ( EventControl ) * EventCount ) * 2 );
}

TMStatus	eventManagerCreate ( 
	eventManagerParameters* Parameters,
	UInt32* EventManagerHandle )
{
	TMStatus		StatusCode;

	EventManagerObject*	EventManager;

	if ( ( EventManager = objectAllocate ( 
		sizeof ( EventManagerObject ),
		EventManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:eventManagerCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail; 
		goto eventManagerCreateExit1;
	}
	EventManager->VIntrManagerHandle	= Parameters->VIntrManagerHandle;
	EventManager->EventCount			= Parameters->EventCount;
	EventManager->SharedData			= (UInt8*)Parameters->SharedData;
	EventManager->HalHandle				= Parameters->HalHandle;
	EventManager->NameSpaceManagerHandle = Parameters->NameSpaceManagerHandle;

	#ifdef TMMAN_HOST

	EventManager->ToPeer = (EventControl*)EventManager->SharedData; 
	EventManager->ToSelf = (EventControl*)( EventManager->SharedData + 
		sizeof ( EventControl ) * EventManager->EventCount ); 

	#else
	
	EventManager->ToSelf = (EventControl*)EventManager->SharedData; 
	EventManager->ToPeer = (EventControl*)( EventManager->SharedData + 
		sizeof ( EventControl ) * EventManager->EventCount ); 

	#endif

	if ( objectlistCreate ( &EventManager->List,  
		EventManager->EventCount ) != True )
	{
		DPF(0,("tmman:eventManagerCreate:objectlistCreate:FAIL\n"));
		StatusCode = statusObjectListAllocFail;
		goto	eventManagerCreateExit2;
	}

	if ( ( StatusCode = vintrCreate  (
		EventManager->VIntrManagerHandle,
		1,
		eventVIntrHandler,
		(Pointer)EventManager,
		(UInt32*)&EventManager->VIntrHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:eventManagerCreate:vintrCreate:FAIL[%lx]\n",
			StatusCode));
		StatusCode = statusEventInterruptRegistrationFail;
		goto eventManagerCreateExit3;
	}

	if ( critsectCreate ( &EventManager->CriticalSectionHandle ) == False )
	{
		DPF(0,("tmman:eventManagerCreate:critsectCreate:FAIL\n"));
		StatusCode = statusCriticalSectionCreateFail;
		goto eventManagerCreateExit4;
	}


/*	eventManagerReset ( (UInt32)EventManager ); */

	*EventManagerHandle  = (UInt32)EventManager;

	return statusSuccess;
/*
eventManagerCreateExit5:	
	critsectDestroy ( EventManager->CriticalSectionHandle );
*/

eventManagerCreateExit4:	
	halRemoveHandler ( EventManager->HalHandle );

eventManagerCreateExit3:
	objectlistDestroy ( &EventManager->List );

eventManagerCreateExit2:
	objectFree ( EventManager );

eventManagerCreateExit1:
	return StatusCode;
}

TMStatus	eventManagerDestroy ( 
	UInt32 EventManagerHandle )
{
	EventManagerObject* EventManager = 
		( EventManagerObject* )EventManagerHandle;


	if ( objectValidate ( EventManager, EventManagerFourCC ) != True )
	{
		DPF(0,("tmman:eventManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	critsectDestroy ( EventManager->CriticalSectionHandle );

	vintrDestroy ( EventManager->VIntrHandle );

	objectlistDestroy ( &EventManager->List );

	objectFree ( EventManager );

	return statusSuccess;
}

TMStatus  eventManagerDestroyEventByClient (
	UInt32	EventManagerHandle, 
	UInt32 ClientHandle )
{
	EventManagerObject*	Manager = 
		( EventManagerObject* )EventManagerHandle;
	EventObject*	Object;
	UInt32	Idx;

	if ( objectValidate ( Manager, EventManagerFourCC ) != True )
	{
		DPF(0,("tmman:eventManagerDestroyEventByClient:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < Manager->EventCount ; Idx++ )
	{
		Object = objectlistGetObject (
			&Manager->List, 
			Idx );
		if ( ( Object ) && Object->ClientHandle == ClientHandle )
		{
			eventDestroy ( (UInt32) Object );
		}
	}

	return statusSuccess;
}

TMStatus	eventManagerReset ( UInt32 EventManagerHandle  )
{
	EventManagerObject	*EventManager = 
		( EventManagerObject* )EventManagerHandle;

	UInt32 Idx;


	if ( objectValidate ( EventManager, EventManagerFourCC ) != True )
	{
		DPF(0,("tmman:eventManagerReset:objectValidate:FAIL\n")); 
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < EventManager->EventCount ; Idx ++ )
	{
		halAccessEnable( EventManager->HalHandle );
		EventManager->ToPeer[Idx].Signal = 
			halAccess32 ( EventManager->HalHandle, 0 );
		EventManager->ToPeer[Idx].Reset = 
			halAccess32 ( EventManager->HalHandle, 0 );
		EventManager->ToSelf[Idx].Signal = 
			halAccess32 ( EventManager->HalHandle, 0 );
		EventManager->ToSelf[Idx].Reset = 
			halAccess32 ( EventManager->HalHandle, 0 );
		halAccessDisable( EventManager->HalHandle );

	}
	return statusSuccess;
}

TMStatus	eventCreate (
	UInt32	EventManagerHandle,						
	Pointer	ListHead,
	String	Name,
	UInt32	SynchronizationHandle,
	UInt32	SynchronizationFlags,
	UInt32* EventHandlePointer )
{
	EventManagerObject* EventManager = 
		( EventManagerObject* )EventManagerHandle;
	EventObject*	Event;
	TMStatus	StatusCode;
	UInt32		NameSpaceHandle;
	UInt32		EventNumber;

	if ( objectValidate ( EventManager, EventManagerFourCC ) != True )
	{
		DPF(0,("tmman:eventCreate:objectValidate:FAIL\n")); 
		return statusInvalidHandle;
	}

	if ( ( StatusCode = namespaceCreate  (
		EventManager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectEvent,
		Name,
		&EventNumber,
		&NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:eventCreate:namespaceCreate:FAIL[%lx]\n",
			StatusCode ));
		goto eventCreateExit1;	
	}


	if ( ( Event = objectAllocate (
		sizeof ( EventObject ), EventFourCC ) ) == Null )
	{
		DPF(0,("tmman:eventCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto eventCreateExit2;	
	}

	if ( objectlistInsert ( 
		&EventManager->List, 
		Event,
		EventNumber ) != True )
	{
		DPF(0,("tmman:eventCreate:objectlistInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto eventCreateExit3;	
	}

	if ( syncobjCreate ( 
		SynchronizationFlags,
		SynchronizationHandle,
		&Event->SynchObject,
		Name ) != True )
	{
		DPF(0,("tmman:eventCreate:synchobjCreate:FAIL\n"));
		StatusCode = statusSynchronizationObjectCreateFail;
		goto eventCreateExit4;
	}

	Event->EventManager    = EventManager;
	Event->EventNumber     = EventNumber;
	Event->NameSpaceHandle = NameSpaceHandle;
	Event->ClientHandle    = (UInt32)ListHead;
	

	*EventHandlePointer = (UInt32)Event;

	return statusSuccess;
/*
eventCreateExit5 :
	syncobjDestroy ( 
		Event->SynchObject );
*/

eventCreateExit4 :
	objectlistDelete ( 
		&EventManager->List, 
		Event,
		EventNumber );


eventCreateExit3 :
	objectFree ( Event );

eventCreateExit2 :
	namespaceDestroy ( NameSpaceHandle );

eventCreateExit1 :
	return StatusCode;
}

TMStatus	eventDestroy ( 
	UInt32 EventHandle )
{
	EventManagerObject*	EventManager;
	EventObject*	Event =
		(EventObject*) EventHandle;
	UInt32	NameSpaceHandle;


	if ( objectValidate ( Event, EventFourCC ) != True )
	{
		DPF(0,("tmman:eventDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	/* signal the obejct so that a waiting thread can unblock */
	syncobjSignal ( Event->SynchObject );

	EventManager = (EventManagerObject* )Event->EventManager;
	NameSpaceHandle = Event->NameSpaceHandle;

	syncobjDestroy ( 
		Event->SynchObject );

	objectlistDelete ( 
		&EventManager->List, 
		Event,
		Event->EventNumber );

	objectFree ( Event );
	
	namespaceDestroy ( NameSpaceHandle );

	return statusSuccess;
}

TMStatus	eventSignal (
	UInt32 EventHandle )
{
	EventManagerObject*	EventManager;
	EventObject*	Event =
		(EventObject*) EventHandle;


	if ( objectValidate ( Event, EventFourCC ) != True )
	{
		DPF(0,("tmman:eventSignal:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	EventManager = (EventManagerObject* )Event->EventManager;
	
	halAccessEnable( EventManager->HalHandle );

	EventManager->ToPeer[Event->EventNumber].Signal  = 
		halAccess32( EventManager->HalHandle, 
		halAccess32 ( EventManager->HalHandle, 
		EventManager->ToPeer[Event->EventNumber].Signal ) + 1 );

	halAccessDisable( EventManager->HalHandle );
				
	vintrGenerateInterrupt( EventManager->VIntrHandle );

	return statusSuccess;
}

//DL function added
TMStatus eventWait(UInt32 EventHandle)
{
     EventObject* Event = (EventObject*) EventHandle;
	
     if ( objectValidate ( Event, EventFourCC ) != True )
     {
       DPF(0,("tmman:eventWait-------:objectValidate:FAIL\n"));
       return statusInvalidHandle;
     }
     syncobjWait(Event->SynchObject);
     DPF(0,("event.c: eventWait: syncobjWait has returned.\n")); 
     return statusSuccess; 
}


void	eventVIntrHandler ( 
	Pointer Context )
{
	EventManagerObject*	EventManager = (EventManagerObject*)Context;

	UInt32	IdxInt;

	if ( objectValidate ( EventManager, EventManagerFourCC ) != True )
	{
		DPF(0,("tmman:eventVIntrHandler:objectValidate:FAIL\n"));
		return;
	}


	for ( IdxInt = 0 ; IdxInt < EventManager->EventCount ; IdxInt ++ )
	{
		UInt32	IntCount;
		EventObject*	Event = 
			(EventObject* )objectlistGetObject( &EventManager->List, IdxInt );
		
		if ( ! Event )
		{
			continue;
		}

		/* ENTER CRITICAL SECTION */
		halAccessEnable( EventManager->HalHandle );
		for (  IntCount = 0; 
			halAccess32 ( EventManager->HalHandle, 
			EventManager->ToSelf[IdxInt].Signal ) != 
			halAccess32 ( EventManager->HalHandle, 
			EventManager->ToSelf[IdxInt].Reset );
			/* overflow - reset to 0 after 0xffffffff */
			EventManager->ToSelf[IdxInt].Reset  = 
			halAccess32( EventManager->HalHandle, 
			halAccess32 ( EventManager->HalHandle, 
			EventManager->ToSelf[IdxInt].Reset ) + 1 ), 
			IntCount++ )
		{
			/* INCREMENT THE COUNT */
		}
		halAccessDisable( EventManager->HalHandle );

		/* LEAVE CRITICAL SECTION */

		if ( IntCount == 0 )
		{
			continue;
		}

		syncobjSignal ( Event->SynchObject );
	}
}








