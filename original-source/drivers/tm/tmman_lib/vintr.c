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
	vintr.c

	Virtual Interrupt Layer.

	This layer provides virtual bidirectional interrupts over a 
	single physical interrupt line.

	HISTORY
	#define TR	Tilakraj Roy
	960426	TR	Created
	960428	TR	Added IPC layer to this file 
	960502	TR	Seperated common routines for standalone and RTOS compiling.
	960531	TR	Put in the Msg interface for other programs to call this module
	960618	TR	Added the posting diabled flag checking 
	960923	TR	Added calls for AppMem* for OS scheduling manipulation
	960924	TR	Added Multiplexed interrupt routing  removed mailbox funcs.
	961016	TR	Renamed AppMem* AppModel*, added version info
	970521	TR	Ported to generic interface
*/

#include "tmmanlib.h"

#define		VIntrManagerFourCC		tmmanOBJECTID ( 'V', 'I', 'N', 'M' )
#define		VIntrFourCC				tmmanOBJECTID ( 'V', 'I', 'N', 'T' )

typedef struct tagVIntrControl
{
	UInt32	volatile Req;
	UInt32	volatile Ack;	
}	VIntrControl;

typedef struct tagVIntrObject
{
	GenericObject		Object;
	VInterruptHandler	Handler;
	Pointer				Context;
	Pointer				VIntrManager;
	UInt32				VIntrNumber;
}	VIntrObject;

typedef struct tagVIntrManagerObject
{
	GenericObject	Object;
	ObjectList		List;
	UInt32			VIntrCount;
	VIntrControl*	ToPeer;
	VIntrControl*	ToSelf;
	UInt8*			SharedData;
	UInt32			HalHandle;
	UInt32			CriticalSectionHandle;
}	VIntrManagerObject;



void vintrHandler( Pointer Context );

	
UInt32	vintrSharedDataSize ( 
	UInt32 InterruptCount )
{
	return  ( ( sizeof ( VIntrControl ) * InterruptCount ) * 2 );
}

TMStatus	vintrManagerCreate ( 
	vintrManagerParameters* Parameters,
	UInt32* VIntrManagerHandle )
{
	/*
	initialization only to avoid warning at compilation time
	variable is always modified before being used
	*/
	TMStatus		StatusCode = statusSuccess;

	VIntrManagerObject*	VIntrManager;

	if ( ( VIntrManager = objectAllocate ( 
		sizeof ( VIntrManagerObject ),
		VIntrManagerFourCC ) ) == Null )
	{
		StatusCode = statusObjectAllocFail; 
		goto vintrManagerCreateExit1;
	}

	VIntrManager->VIntrCount			= Parameters->VIntrCount;
	VIntrManager->SharedData			= (UInt8*)Parameters->SharedData;
	VIntrManager->HalHandle				= Parameters->HalHandle;

	#ifdef TMMAN_HOST

	VIntrManager->ToPeer = (VIntrControl*)VIntrManager->SharedData; 
	VIntrManager->ToSelf = (VIntrControl*)( VIntrManager->SharedData + 
		sizeof ( VIntrControl ) * VIntrManager->VIntrCount ); 

	#else
	
	VIntrManager->ToSelf = (VIntrControl*)VIntrManager->SharedData; 
	VIntrManager->ToPeer = (VIntrControl*)( VIntrManager->SharedData + 
		sizeof ( VIntrControl ) * VIntrManager->VIntrCount ); 

	#endif

	if ( objectlistCreate ( &VIntrManager->List,  
		VIntrManager->VIntrCount ) != True )
	{
		StatusCode = statusObjectListAllocFail;
		goto	vintrManagerCreateExit2;
	}

	if ( critsectCreate ( &VIntrManager->CriticalSectionHandle ) == False )
	{
		goto vintrManagerCreateExit3;
	}

	halInstallHandler ( VIntrManager->HalHandle, 
		vintrHandler, VIntrManager );

	/*vintrManagerReset ( (UInt32)VIntrManager ); */

	*VIntrManagerHandle  = (UInt32)VIntrManager;

	return statusSuccess;

/*
vintrManagerCreateExit5:	
	halRemoveHandler ( VIntrManager->HalHandle );

vintrManagerCreateExit4:	
	critsectDestroy ( VIntrManager->CriticalSectionHandle );
*/

vintrManagerCreateExit3:
	objectlistDestroy ( &VIntrManager->List );

vintrManagerCreateExit2:
	objectFree ( VIntrManager );

vintrManagerCreateExit1:
	return StatusCode;
}

TMStatus	vintrManagerDestroy ( 
	UInt32 VIntrManagerHandle )
{
	VIntrManagerObject* VIntrManager = 
		( VIntrManagerObject* )VIntrManagerHandle;


	if ( objectValidate ( VIntrManager, VIntrManagerFourCC ) != True )
	{
		DPF(0,("tmman:vintrManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	halRemoveHandler ( VIntrManager->HalHandle );

	objectlistDestroy ( &VIntrManager->List );

	objectFree ( VIntrManager );

	return statusSuccess;
}

TMStatus	vintrManagerReset ( UInt32 VIntrManagerHandle  )
{
	VIntrManagerObject	*VIntrManager = 
		( VIntrManagerObject* )VIntrManagerHandle;

	UInt32 Idx;


	if ( objectValidate ( VIntrManager, VIntrManagerFourCC ) != True )
	{
		DPF(0,("tmman:vintrManagerReset:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < VIntrManager->VIntrCount ; Idx ++ )
	{
		halAccessEnable( VIntrManager->HalHandle );

		VIntrManager->ToPeer[Idx].Req = 
			halAccess32 ( VIntrManager->HalHandle, 0 );
		VIntrManager->ToPeer[Idx].Ack = 
			halAccess32 ( VIntrManager->HalHandle, 0 );
		VIntrManager->ToSelf[Idx].Req = 
			halAccess32 ( VIntrManager->HalHandle, 0 );
		VIntrManager->ToSelf[Idx].Ack = 
			halAccess32 ( VIntrManager->HalHandle, 0 );

		halAccessDisable( VIntrManager->HalHandle );

	}
	return statusSuccess;
}

TMStatus	vintrCreate ( 
	UInt32 VIntrManagerHandle,
	UInt32 VIntrNumber,
	VInterruptHandler Handler, 
	Pointer Context,
	UInt32 *VIntrHandlePointer )
{
	VIntrManagerObject* VIntrManager = 
		( VIntrManagerObject* )VIntrManagerHandle;
	VIntrObject*	VIntr;
	TMStatus	StatusCode;


	if ( objectValidate ( VIntrManager, VIntrManagerFourCC ) != True )
	{
		DPF(0,("tmman:vintrCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( !Handler ) 
	{
		DPF(0,("tmman:vintrCreate:NULL Handler:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( ( VIntr = objectAllocate (
		sizeof ( VIntrObject ), VIntrFourCC ) ) == Null )
	{
		DPF(0,("tmman:vintrCreate:objectAllcoate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto vintrCreateExit1;	
	}

	if ( objectlistInsert ( 
		&VIntrManager->List, 
		VIntr,
		VIntrNumber ) != True )
	{
		DPF(0,("tmman:vintrCreate:objectlistInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto vintrCreateExit2;	
	}

	VIntr->VIntrManager		= VIntrManager;
	VIntr->VIntrNumber		= VIntrNumber;
	VIntr->Handler			= Handler;
	VIntr->Context			= Context;

	*VIntrHandlePointer = (UInt32)VIntr;

	return statusSuccess;

/*
vintrCreateExit3 :
	objectlistDelete ( 
		&VIntrManager->List, 
		VIntr,
		VIntrNumber );
*/	

vintrCreateExit2 :
	objectFree ( VIntr );

vintrCreateExit1 :
	return StatusCode;
}

TMStatus	vintrDestroy ( 
	UInt32 VIntrHandle )
{
	VIntrManagerObject*	VIntrManager;
	VIntrObject*	VIntr =
		(VIntrObject*) VIntrHandle;


	if ( objectValidate ( VIntr, VIntrFourCC ) != True )
	{
		DPF(0,("tmman:vintrDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	VIntrManager = (VIntrManagerObject* )VIntr->VIntrManager;

	objectlistDelete ( 
		&VIntrManager->List, 
		VIntr,
		VIntr->VIntrNumber );

	objectFree ( VIntr );

	return statusSuccess;
}

TMStatus	vintrGenerateInterrupt (  
	UInt32 VIntrHandle )
{
	VIntrManagerObject*	VIntrManager;
	VIntrObject*	VIntr =
		(VIntrObject*) VIntrHandle;


	if ( objectValidate ( VIntr, VIntrFourCC ) != True )
	{
		DPF(0,("tmman:vintrGenerateInterrupt:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	VIntrManager = (VIntrManagerObject* )VIntr->VIntrManager;
	
	halAccessEnable( VIntrManager->HalHandle );

	VIntrManager->ToPeer[VIntr->VIntrNumber].Req  = 
		halAccess32( VIntrManager->HalHandle, 
		halAccess32 ( VIntrManager->HalHandle, 
		VIntrManager->ToPeer[VIntr->VIntrNumber].Req ) + 1 );

	halAccessDisable( VIntrManager->HalHandle );
				
	halGenerateInterrupt( VIntrManager->HalHandle );

	return statusSuccess;
}

void	vintrHandler( Pointer Context )
{
	VIntrManagerObject*	VIntrManager = (VIntrManagerObject*)Context;

	UInt32	IdxInt;

/*	DPF(0,("VI{")); */

	for ( IdxInt = 0 ; IdxInt < VIntrManager->VIntrCount ; IdxInt ++ )
	{
		UInt32	IntCount;
		VIntrObject*	VIntr = 
			(VIntrObject* )objectlistGetObject( &VIntrManager->List, IdxInt );
		
		if ( ! VIntr )
		{
			continue;
		}

		halAccessEnable( VIntrManager->HalHandle );

		for (  IntCount = 0; 
			halAccess32 ( VIntrManager->HalHandle, 
			VIntrManager->ToSelf[IdxInt].Req ) != 
			halAccess32 ( VIntrManager->HalHandle, 
			VIntrManager->ToSelf[IdxInt].Ack );
			/* overflow - reset to 0 after 0xffffffff */
			VIntrManager->ToSelf[IdxInt].Ack  = 
			halAccess32( VIntrManager->HalHandle, 
			halAccess32 ( VIntrManager->HalHandle, 
			VIntrManager->ToSelf[IdxInt].Ack ) + 1 ), 
			IntCount++ )
		{
			/* INCREMENT THE COUNT */
		}

		halAccessDisable( VIntrManager->HalHandle );

		if ( IntCount == 0 )
		{
			continue;
		}

		if ( VIntr->Handler )
		{
			( VIntr->Handler) ( VIntr->Context );
		}
		else
		{
			/* ERROR : No handler installed for object */
			DPF(0,("tmman:vintrHandler:NO Handler:FAIL\n"));
		}
	}
/*	DPF(0,("}VI")); */
}


