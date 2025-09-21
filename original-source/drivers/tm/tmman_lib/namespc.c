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
	namespace.c

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
#include "tmmanapi.h"
#include "tmmanlib.h"


/*	CONSTANTS */

#define		NameSpaceManagerFourCC		tmmanOBJECTID ( 'N', 'M', 'S', 'M' )
#define		NameSpaceFourCC				tmmanOBJECTID ( 'N', 'M', 'S', 'O' )

/* TYPDEFS */


typedef struct tagNameSpaceControl
{
	UInt32	volatile	ObjectType;
	UInt32	volatile	ObjectIndex;
	UInt8	volatile	Name[constTMManNameSpaceNameLength];
}	NameSpaceControl;

typedef struct tagNameSpaceObject
{
	GenericObject		Object;
	Pointer				NameSpaceManager;
	UInt32				NameSpaceNumber;
	UInt32				ObjectId;
	UInt32				ControlObjectType;
	UInt32				ControlObjectIndex;
	UInt8				ControlName[constTMManNameSpaceNameLength];
}	NameSpaceObject;

typedef struct tagNameSpaceManagerObject
{
	GenericObject			Object;
	ObjectList				List;
	UInt32					NameSpaceCount;
	UInt8*					SharedData;
	NameSpaceControl*		Control;
	UInt32					HalHandle;
	UInt32					CriticalSectionHandle;
}	NameSpaceManagerObject;


/* GLOBAL DATA STRUCTURES */
/* NONE - key to object oriented programming */


/* FUNCTIONS */
void namespaceDump ( UInt32 NameSpaceHandle );

UInt32	namespaceSharedDataSize ( 
	UInt32 NameSpaceCount )
{
	return  ( ( sizeof ( NameSpaceControl ) * NameSpaceCount ) );
}

TMStatus	namespaceManagerCreate ( 
	namespaceManagerParameters* Parameters,
	UInt32* NameSpaceManagerHandle )
{
	TMStatus		StatusCode;

	NameSpaceManagerObject*	NameSpaceManager;

	if ( ( NameSpaceManager = objectAllocate ( 
		sizeof ( NameSpaceManagerObject ),
		NameSpaceManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:namespaceManagerCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail; 
		goto namespaceManagerCreateExit1;
	}
	
	NameSpaceManager->NameSpaceCount			= Parameters->NameSpaceCount;
	NameSpaceManager->SharedData			= (UInt8*)Parameters->SharedData;
	NameSpaceManager->HalHandle				= Parameters->HalHandle;
	


	NameSpaceManager->Control = (NameSpaceControl*)NameSpaceManager->SharedData; 
	
	if ( objectlistCreate ( &NameSpaceManager->List,  
		NameSpaceManager->NameSpaceCount ) != True )
	{
		DPF(0,("tmman:namespaceManagerCreate:objectlistCreate:FAIL\n"));
 
		StatusCode = statusObjectListAllocFail;
		goto	namespaceManagerCreateExit2;
	}

	
	if ( critsectCreate ( &NameSpaceManager->CriticalSectionHandle ) == False )
	{
		DPF(0,("tmman:namespaceManagerCreate:critsectCreate:FAIL\n"));
		StatusCode = statusCriticalSectionCreateFail;
		goto namespaceManagerCreateExit3;
	}

#ifdef TMMAN_HOST
/*	namespaceManagerReset ( (UInt32)NameSpaceManager ); */
#endif


	*NameSpaceManagerHandle  = (UInt32)NameSpaceManager;

	return statusSuccess;
/*
namespaceManagerCreateExit4:
	critsectDestroy ( Manager->CriticalSectionHandle );
*/

namespaceManagerCreateExit3:
	objectlistDestroy ( &NameSpaceManager->List );

namespaceManagerCreateExit2:
	objectFree ( NameSpaceManager );

namespaceManagerCreateExit1:
	return StatusCode;
}

TMStatus	namespaceManagerDestroy ( 
	UInt32 NameSpaceManagerHandle )
{
	NameSpaceManagerObject* NameSpaceManager = 
		( NameSpaceManagerObject* )NameSpaceManagerHandle;

	if ( objectValidate ( NameSpaceManager, NameSpaceManagerFourCC ) != True )
	{
		DPF(0,("tmman:namespaceManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	objectlistDestroy ( &NameSpaceManager->List );

	objectFree ( NameSpaceManager );

	return statusSuccess;
}

TMStatus	namespaceManagerReset ( 
	UInt32 NameSpaceManagerHandle  )
{
	NameSpaceManagerObject	*Manager = 
		( NameSpaceManagerObject* )NameSpaceManagerHandle;
	NameSpaceObject *Object;

	UInt32 Idx;
	/*
	unused variable
	UInt32	NestedContext;
	*/

	if ( objectValidate ( Manager, NameSpaceManagerFourCC ) != True )
	{
		DPF(0,("tmman:namespaceManagerReset:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < Manager->NameSpaceCount ; Idx ++ )
	{
		halAccessEnable( Manager->HalHandle );

        Manager->Control[Idx].ObjectType =
            halAccess32 ( Manager->HalHandle, constTMManNameSpaceObjectInvalid );
        Manager->Control[Idx].ObjectIndex =
            halAccess32 ( Manager->HalHandle, 0 );

		halAccessDisable( Manager->HalHandle );
	}

	for ( Idx = 0 ; Idx < Manager->NameSpaceCount ; Idx ++ )
	{
		if ( ( Object = objectlistGetObject (
			&Manager->List, 
			Idx ) ) != Null )
		{
			halAccessEnable(Manager->HalHandle);

			Manager->Control[Idx].ObjectType = 
				halAccess32 ( Manager->HalHandle, Object->ControlObjectType );

			Manager->Control[Idx].ObjectIndex = 
				halAccess32 ( Manager->HalHandle, Object->ControlObjectIndex );
			

            // DTO 4/28/99: Work around for PCI Swap problem
			//strCopy( (Pointer)Manager->Control[Idx].Name, 
			//	(Pointer)Object->ControlName );

			halAccessDisable(Manager->HalHandle);
		}
	}
	
	return statusSuccess;
}




TMStatus	namespaceCreate (
	UInt32	NameSpaceManagerHandle,						
	UInt32	NameSpaceObjectType,
	String	NameSpaceName,
	UInt32*	ObjectIdPointer,
	UInt32* NameSpaceHandlePointer )
{
	NameSpaceManagerObject* Manager = 
		( NameSpaceManagerObject* )NameSpaceManagerHandle;
	NameSpaceObject*	NameSpace;
	TMStatus			Status;
	UInt32				SlotIdx, ObjectIdx, Idx;
	UInt32				NestedContext;


	DPF(9,("tmman:namespaceCreate:Type[%lx]:Name[%s]\n",
			NameSpaceObjectType,
			NameSpaceName ));

	if ( objectValidate ( Manager, NameSpaceManagerFourCC ) != True )
	{
		DPF(0,("tmman:namespaceCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}
        //DL
          DPF(0,("tmman:namespaceCreate:objectValidate:SUCCESS\n"));
	/*
	for ( Idx = 0 ; Idx < Manager->NameSpaceCount ; Idx ++ )
	{
		NameSpaceObject *Object;

		if ( ( Object = objectlistGetObject (
			&Manager->List, 
			Idx ) ) != Null )
		{
			namespaceDump(Object);

		}
	}
	*/

#ifdef TMMAN_HOST

	/* ENTER CRITICAL SECTION */
        DPF(0,("tmman:namespaceCreate:critsectEnter is about to be called.\n"));
	critsectEnter ( Manager->CriticalSectionHandle, &NestedContext );
        DPF(0,("tmman:namespaceCreate:critsectEnter has returned.\n"));
	/* 
		find a slot (empty local name space object) that we can use 
		this should be the same as the shared slot index
	*/	
	for ( SlotIdx = 0 ; SlotIdx < Manager->NameSpaceCount ; SlotIdx ++ )
	{

		if ( objectlistGetObject (
			&Manager->List, 
			SlotIdx ) == Null )
			break;
	}

	/* we couldn't find empty slots so bail out */
	if ( SlotIdx >= Manager->NameSpaceCount )
	{
		DPF(0,("tmman:namespaceCreate:SlotIdx[%lx]:OUT OF EMPTY SLOTS:FAIL\n",
			SlotIdx ));
		Status = statusNameSpaceNoMoreSlots;
		/* LEAVE CRITICAL SECTION */
		critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );
		goto	namespaceCreateExit1;
	}

	/* find the ID relative to the type of object we are looking for */
	for ( ObjectIdx = 0 ; ; ObjectIdx++ )
	{
		Bool	IDFound = False;

		for ( Idx = 0 ; Idx < Manager->NameSpaceCount ; Idx ++ )
		{
			NameSpaceObject*	Object;

			if ( ( Object = objectlistGetObject (
				&Manager->List, 
				Idx ) ) != Null )
			{


				/* a different type of object */
				if ( NameSpaceObjectType != Object->ControlObjectType )
					continue;

				
				/* same type of object with a different ID */
				if ( ObjectIdx != Object->ControlObjectIndex )
					continue;

				/* name space conflict */
				if ( strCmp ( NameSpaceName, 
					(Pointer)Object->ControlName ) == 0 )
				{
					DPF(0,("tmman:namespaceCreate:NameSpace[%s]:CONFLICT:FAIL\n",
						NameSpaceName ));

					/* this name already exists for this object so stop */
					Status = statusNameSpaceNameConflict;
					/* LEAVE CRITICAL SECTION */
					critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );
					goto	namespaceCreateExit1;
				}

				/* Id exists - so increment ObjectIdx and start search again */
				break;
			}
		}

		/* check if have reached the end without colliding with the ID */
		if ( Idx >= Manager->NameSpaceCount ) 
		{
			/* we have found an ObjectIdx that is not present */
			IDFound = True;
			break;
		}

		/* otherwise increment and try the search again */
	}
	
	/* this loop always exists with some Object Idx that is not already present */

	/* copy the shared control data */
        DPF(0,("tmman:namespaceCreate:halAccessEnable is about to be called.\n"));
	halAccessEnable( Manager->HalHandle );
              
        DPF(0,("tmman:namespaceCreate:halAccess32 is about to be called.\n"));
	Manager->Control[SlotIdx].ObjectType = 
		halAccess32 ( Manager->HalHandle, NameSpaceObjectType );
	Manager->Control[SlotIdx].ObjectIndex = 
		halAccess32 ( Manager->HalHandle, ObjectIdx );

	halAccessDisable( Manager->HalHandle );


	halAccessEnable( Manager->HalHandle );
	strCopy( (Pointer)Manager->Control[SlotIdx].Name, NameSpaceName );
	halAccessDisable( Manager->HalHandle );


	
#else	/* TMMAN_TARGET */


	for ( SlotIdx = 0 ; SlotIdx < Manager->NameSpaceCount ; SlotIdx ++ )
	{
		UInt32	StrCmpResult, ObjectType, ObjectIndex;
		UInt8	ObjectName[constTMManNameSpaceNameLength];


		halAccessEnable( Manager->HalHandle );

		ObjectType = halAccess32 ( Manager->HalHandle, 
			Manager->Control[SlotIdx].ObjectType );
	
		ObjectIndex = halAccess32 ( Manager->HalHandle, 
			Manager->Control[SlotIdx].ObjectIndex );

		strCopy( (Pointer)ObjectName, (Pointer) Manager->Control[SlotIdx].Name );

		halAccessDisable( Manager->HalHandle );

		if ( ObjectType != 	constTMManNameSpaceObjectInvalid )
		{
			DPF(0,("tmman:TARGET:namespaceCreate:Slot#[%x]:Type[%x]:Idx[%x]:Name[%s]\n",
				SlotIdx, 
				ObjectType,
				ObjectIndex,
				ObjectName ));
		}
		else
		{
			continue;
		}


		if ( ObjectType  != NameSpaceObjectType )
			continue;
		
		/* BUGCHECK : Strcmp performs bytes accesses over the PCI bus */
		halAccessEnable( Manager->HalHandle );

        StrCmpResult = strCmp ((Pointer)Manager->Control[SlotIdx].Name, 
			NameSpaceName );

		halAccessDisable( Manager->HalHandle );

		if ( StrCmpResult == 0 )
		{
			break;
		}
	}


	if ( SlotIdx >= Manager->NameSpaceCount )
	{
		DPF(0,("tmman:namespaceCreate:Name[%s]:NON EXISTENT:FAIL\n",
			NameSpaceName ));
		Status = statusNameSpaceNameNonExistent;
		goto	namespaceCreateExit1;
	}

#endif

	if ( ( NameSpace = objectAllocate (
		sizeof ( NameSpaceObject ), NameSpaceFourCC ) ) == Null )
	{
		DPF(0,("tmman:namespaceCreate:objectAllocate:FAIL\n"));
		Status = statusObjectAllocFail;
		goto namespaceCreateExit2;	
	}

	if ( objectlistInsert ( 
		&Manager->List, 
		NameSpace,
		SlotIdx ) != True )
	{
		DPF(0,("tmman:namespaceCreate:objectlistInsert:FAIL\n"));
		Status = statusObjectInsertFail;
		goto namespaceCreateExit3;	
	}

	NameSpace->NameSpaceManager = Manager;
	NameSpace->NameSpaceNumber = SlotIdx;

#ifdef TMMAN_HOST

	/* LEAVE CRITICAL SECTION */
        DPF(0,("tmman:namespaceCreate:critsectLeave is about to be called.\n"));
	critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );

	NameSpace->ObjectId = ObjectIdx;
	/*
	save the shared (control) data in our local object  for 
	using it at the time of reset, when endianess is known
	*/
	NameSpace->ControlObjectType = NameSpaceObjectType;
	NameSpace->ControlObjectIndex = ObjectIdx;
	strCopy( (Pointer)NameSpace->ControlName, NameSpaceName );

#else

	halAccessEnable( Manager->HalHandle );

	NameSpace->ObjectId = 
		halAccess32 ( Manager->HalHandle, 
		Manager->Control[SlotIdx].ObjectIndex );

	halAccessDisable( Manager->HalHandle );

#endif

	DPF(9,("tmman:namespaceCreate:Slot#[%lx]:Type[%lx]:Idx[%lx]:Name[%s]\n",
		SlotIdx, 
		NameSpaceObjectType,
		NameSpace->ObjectId,
		NameSpaceName ));

	*NameSpaceHandlePointer = (UInt32)NameSpace;
	*ObjectIdPointer = NameSpace->ObjectId;

	return statusSuccess;
/*
namespaceCreateExit4 :
	objectlistDelete ( 
		&Manager->List, 
		NameSpace,
		SlotIdx );
*/

namespaceCreateExit3 :
	objectFree ( NameSpace );

namespaceCreateExit2 :

#ifdef TMMAN_HOST
	halAccessEnable( Manager->HalHandle );

	Manager->Control[SlotIdx].ObjectType = 
		halAccess32 ( Manager->HalHandle, constTMManNameSpaceObjectInvalid );
	Manager->Control[SlotIdx].ObjectIndex = 
		halAccess32 ( Manager->HalHandle, 0 );

	halAccessDisable( Manager->HalHandle );

#endif

namespaceCreateExit1 :
	return Status;
}

TMStatus	namespaceDestroy ( 
	UInt32 NameSpaceHandle )
{
	NameSpaceManagerObject*	NameSpaceManager;
	NameSpaceObject*	NameSpace =
		(NameSpaceObject*) NameSpaceHandle;
	UInt32				SlotIdx;
	UInt32				NestedContext;


	if ( objectValidate ( NameSpace, NameSpaceFourCC ) != True )
	{
		DPF(0,("tmman:namespaceDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	DPF(9,("tmman:namespaceDestroy:Slot#[%lx]:Type[%lx]:Idx[%lx]:Name[%s]\n",
		NameSpace->NameSpaceNumber,
		NameSpace->ControlObjectType,
		NameSpace->ControlObjectIndex,
		NameSpace->ControlName ));

	NameSpaceManager = (NameSpaceManagerObject* )NameSpace->NameSpaceManager;

	SlotIdx = NameSpace->NameSpaceNumber;

	objectlistDelete ( 
		&NameSpaceManager->List, 
		NameSpace,
		NameSpace->NameSpaceNumber );

	objectFree ( NameSpace );

#ifdef TMMAN_HOST
	
	/* ENTER CRITICAL SECTION */
	critsectEnter ( NameSpaceManager->CriticalSectionHandle, &NestedContext );
	
	halAccessEnable( NameSpaceManager->HalHandle );
				
	NameSpaceManager->Control[SlotIdx].ObjectType = 
		halAccess32 ( NameSpaceManager->HalHandle, constTMManNameSpaceObjectInvalid );
	NameSpaceManager->Control[SlotIdx].ObjectIndex = 
		halAccess32 ( NameSpaceManager->HalHandle, 0 );

	halAccessDisable( NameSpaceManager->HalHandle );
			
	/* LEAVE CRITICAL SECTION */
	critsectLeave ( NameSpaceManager->CriticalSectionHandle, &NestedContext );

#endif

	return statusSuccess;
}



void namespaceDump ( UInt32 NameSpaceHandle )
{
	NameSpaceObject*	Object =
		(NameSpaceObject*) NameSpaceHandle;

	DPF(9,("namespaceDump:MAN[%p]:NUM[%lx]:OID[%lx]:COT[%lx]:COI[%lx]\n",
		Object->NameSpaceManager,
		Object->NameSpaceNumber,
		Object->ObjectId,
		Object->ControlObjectType,
		Object->ControlObjectIndex ));
}
