/*
	cqueue

	#define	TR	Tilakraj	Roy

	960807	TR	Created	from scratch for vtmman

	Circular fixed length queue manipulation routines.
	FIFO queuesof fixed length
*/


/*----------------------------------------------------------------------------
          SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
          FOLLOWING SECTION  REMAINS UNCHANGED
----------------------------------------------------------------------------*/

#include "tmmanlib.h"

/*
	cqueueCreate
	ItemSize		Size of each item in the queue. This size in bytes
					is used to copy queue Items at the Insert, Delete and
					Retrieve calls.
	Object			Address of teh location where the pointer to the 
					newly allocated object will be stored.
*/

#define		constQueueSize		0x40

typedef	struct tagQueueObject
{
	UInt32	ReadIndex;
	UInt32	WriteIndex;
	UInt32	ItemCount;
	UInt32	ItemSize;
	UInt8*	ItemBuffer;
	UInt32 	Context;
	UInt32	HalHandle;
	UInt32	CriticalSectionHandle;
}	QueueObject;

Bool	queueCreate ( 
	UInt32	ItemSize, 
	UInt32	HalHandle,
	UInt32* QueueHandlePointer )
{
	QueueObject*	Object;

	if ( ( Object = memAllocate ( sizeof ( QueueObject ) ) ) == Null )
	{
		goto queueCreateExit1;
	}

	Object->ItemCount = constQueueSize;
	Object->ItemSize = ItemSize;
	Object->ReadIndex = 0;
	Object->WriteIndex = 0;
	Object->HalHandle = HalHandle;


	if ( ( Object->ItemBuffer = 
		memAllocate ( Object->ItemCount * Object->ItemSize ) ) == Null )
	{
		goto queueCreateExit2;
	}

	if ( critsectCreate ( &Object->CriticalSectionHandle ) == False )
	{
		goto queueCreateExit3;
	}

	*QueueHandlePointer = (UInt32)Object;
	return True;
/*
queueCreateExit4:
	critsectDestroy ( Object->CriticalSectionHandle );
*/
queueCreateExit3:
	memFree ( Object->ItemBuffer );

queueCreateExit2:
	memFree ( Object );

queueCreateExit1:
	return False;
}

/*
	queueDestroy
	Destroys the queue object, after this all references to Object are invalid.
	TRUE	Successful destruction of queue object
	FALSE	Invalid object pointer
*/
Bool	queueDestroy ( 
	UInt32 QueueHandle )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;

	critsectDestroy ( Object->CriticalSectionHandle );
	memFree ( Object->ItemBuffer );
	memFree ( Object );

	return True;
}


/*

Bool	queueIsEmpty ( 
	UInt32 QueueHandle )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;
	Bool			Result;
	UInt32			CriticalSection;

	halDisableInterrupts ( Object->HalHandle, &CriticalSection );

	if ( Object->ReadIndex == Object->WriteIndex )
	{
		Result =  True;
	}
	else
	{
		Result = False;
	}	
	halRestoreInterrupts ( Object->HalHandle, &CriticalSection );
	return Result;

}

Bool	queueIsFull ( 
	UInt32 QueueHandle )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;
	UInt32	WriteIndex;
	Bool	Result;
	UInt32	CriticalSection;

	halDisableInterrupts ( Object->HalHandle, &CriticalSection );

	WriteIndex = ( Object->WriteIndex + 1 ) % Object->ItemCount;	
	if ( Object->ReadIndex == WriteIndex )
	{
		Result =  True;
	}
	else
	{
		Result = False;
	}	
	halRestoreInterrupts ( Object->HalHandle, &CriticalSection );
	return Result;
}
*/
/*
	queueRetrieve
	Retrieves the first item from the queue without deleting it
*/
Bool	queueRetrieve ( 
	UInt32 QueueHandle, 
	Pointer Item )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;

	UInt32	ReadIndex;
	UInt8*	ItemQueue;
	Bool	Result;
	UInt32	NestedContext;

	critsectEnter ( Object->CriticalSectionHandle, &NestedContext );
	
	if ( Object->ReadIndex == Object->WriteIndex )
	{
		Result = False;
		goto cqueueRetrieve_exit;
	}
		

	ReadIndex = Object->ReadIndex;

	ItemQueue = ( Object->ItemBuffer + ( ReadIndex * Object->ItemSize ) );

	memCopy  ( Item, ItemQueue, Object->ItemSize );

	Result = True;

cqueueRetrieve_exit :
	critsectLeave ( Object->CriticalSectionHandle, &NestedContext );
	return Result;
}


/*
	cqueueInsert
	Inserts Item at the end of the  queue
	TRUE	Successful Insertion
	FALSE	No room in the queue
			Invalid Object Pointer
*/
Bool	queueInsert ( 
	UInt32 QueueHandle, 
	Pointer Item )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;
	UInt32	WriteIndex;
	UInt8*	ItemQueue;
	Bool	Result;
	UInt32	NestedContext;

	critsectEnter ( Object->CriticalSectionHandle, &NestedContext );
	

	WriteIndex = ( Object->WriteIndex + 1 ) % Object->ItemCount;	

	if ( Object->ReadIndex == WriteIndex )
	{
		Result = False;
		goto cqueueInsert_exit;
	}

	ItemQueue = ( Object->ItemBuffer + ( Object->WriteIndex * Object->ItemSize ) );
	memCopy  ( ItemQueue, Item, Object->ItemSize );

	Object->WriteIndex = WriteIndex;

	Result = True;

cqueueInsert_exit :
	critsectLeave ( Object->CriticalSectionHandle, &NestedContext );
	return Result;

}


/*
	cqueueDelete
	deletes the first item in the queue, and retrieves it in Item
*/
Bool	queueDelete ( 
	UInt32 QueueHandle, 
	Pointer Item )
{
	QueueObject*	Object = (QueueObject*)QueueHandle;
	UInt32	ReadIndex;
	UInt8*	ItemQueue;
	Bool	Result;
	UInt32	NestedContext;

	critsectEnter ( Object->CriticalSectionHandle, &NestedContext );

	if ( Object->ReadIndex == Object->WriteIndex )
	{
		Result  =  False;
		goto cqueueDelete_exit;
	}
	ReadIndex = Object->ReadIndex;

	ItemQueue = ( Object->ItemBuffer + ( ReadIndex * Object->ItemSize ) );
	memCopy  ( Item, ItemQueue, Object->ItemSize );

	Object->ReadIndex = ( ReadIndex + 1 ) % Object->ItemCount;	

	Result = True;

cqueueDelete_exit :
	critsectLeave ( Object->CriticalSectionHandle, &NestedContext );
	return Result;

}



