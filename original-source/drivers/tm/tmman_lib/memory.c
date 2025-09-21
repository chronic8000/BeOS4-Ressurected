/*---------------------------------------------------------------------------- 
COPYRIGHT (c) 1995 by Philips Semiconductors

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
/*----------------------------------------------------------------------------

	#define TR Tilakraj Roy
	940424	TR	Created
	940827	TR	Documented new algorithms
	960925	TR	Pulled in sources for tmman
	960927	TR	Created interfaces for tmman
    990511  DTO Ported to pSOS

	ABOUT
	This file contains the dynamic memory allocation
	routines required by the application to allocate
	and free memory as and when required. With the
	current implementation the kernel data structures
	are statically allocated. This is done to maintain
	alogorithm simplicity. Since we are manipulating
	linked lists. These functions have to acquire the
	lock to the memory manager data structure.This is
	not implemented currently.  
	 		  
		MEM_BLOCK
		+----------+ <--------MEM_MGR_OBJECT.pHead
+------>| pPrev    |---> NULL
|		| pNext    |---+
|	 +--| pData    |   |
|	 |  | Length   |   |
|	 |  +----------+   |
|	 +->|          |   |
|		|          |   |
|		| Allocated|   |
|		| Block    |   |
|		|          |   |
|		|          |   |
|		+----------+   |
+-------| pPrev    |<--+
 		| pNext    |---> NULL
 	 +--| pData    |
 	 |  | Length   |
 	 |  +----------+
 	 +->|          |
 		|          |
 		| Free     |
		| Block    |
		|          |
		|          |
		+----------+


The above diagram shows the me memory layout after the calls to memCreate()
and memAlloc().
MEM_MGR_OBJECT points to the first SHMEM_BLOCK after the call to 
memCreate(). SHMEM_BLOCK points to the entire free memory. 
After the first call to memAllocate() a new memory block is created that
points to the remaining free space and the original SHMEM_BLOCK points
to the memory that was allocated as a result of this call to memAlloc().
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
          DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmmanlib.h"

#define		MemoryManagerFourCC		tmmanOBJECTID ( 'M', 'E', 'M', 'M' )
#define		MemoryFourCC			tmmanOBJECTID ( 'M', 'E', 'M', 'O' )

#define		COLLAPSE(x)	((x)>>2)
#define		EXPAND(x)	((x)<<2)

typedef struct	tagMemoryBlock
{
	struct tagMemoryBlock*	Prev;
	struct tagMemoryBlock*	Next;
	Pointer					Data;
	struct
	{
		UInt32	Length		:30;
		UInt32	Allocated	:1;
		UInt32	Contig		:1;
	}	Flags;
}	MemoryBlock;

typedef struct	tagMemoryControl
{
	UInt32	volatile Offset;
	UInt32	volatile Size;
}	MemoryControl;

typedef struct tagMemoryObject
{
	GenericObject	Object;
	Pointer			MemoryManager;
	UInt32			MemoryNumber;
	UInt32			Size;
	Pointer			Address;
	UInt32			NameSpaceHandle;
	UInt32			ClientHandle;
	UInt32			ControlOffset;
	UInt32			ControlSize;

}	MemoryObject;

typedef struct tagMemoryManagerObject
{
	GenericObject		Object;
	ObjectList			List;
	UInt32				MemoryCount;
	UInt32				MemorySize;
	Pointer				MemoryBlock;
	Pointer				SharedData; 
	UInt32				HalHandle;
	MemoryControl*		Control;

	/* required ony on the host */
	UInt32				MemoryFree;
	Pointer				Head;
	UInt32				NameSpaceManagerHandle;
	UInt32				CriticalSectionHandle;
}	MemoryManagerObject;


UInt32	memorySharedDataSize ( 
	UInt32 MemoryCount )
{
	return  ( sizeof ( MemoryControl ) * MemoryCount );
}

TMStatus	memoryManagerReset ( 
	UInt32 MemoryManagerHandle )
{
	MemoryManagerObject*	Manager = 
		( MemoryManagerObject* )MemoryManagerHandle;
	MemoryObject*			Object;
	UInt32					Idx;


	if ( objectValidate ( Manager, MemoryManagerFourCC ) != True )
	{
		DPF(0,("tmman:memoryManagerReset:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	for ( Idx = 0 ; Idx < Manager->MemoryCount ; Idx ++ )
	{
		halAccessEnable( Manager->HalHandle );
		Manager->Control[Idx].Offset = 
			halAccess32 ( Manager->HalHandle, 0 );
		
		Manager->Control[Idx].Size	= 
			halAccess32 ( Manager->HalHandle, 0 );
		halAccessDisable( Manager->HalHandle );
	}

	

	for ( Idx = 0 ; Idx < Manager->MemoryCount ; Idx ++ )
	{

		if ( ( Object = objectlistGetObject (
			&Manager->List, 
			Idx ) ) != Null )
		{
			halAccessEnable( Manager->HalHandle );
			Manager->Control[Object->MemoryNumber].Offset = 
				halAccess32 ( Manager->HalHandle, 
				Object->ControlOffset );
			
			Manager->Control[Object->MemoryNumber].Size	= 
				halAccess32 ( Manager->HalHandle, 
				Object->ControlSize );
			halAccessDisable( Manager->HalHandle );
		}
	}

	

	return statusSuccess;
}

TMStatus	memoryManagerCreate (
	memoryManagerParameters*	Parameters,
	UInt32*	MemoryManagerHandlePointer )
{
	MemoryManagerObject*	Manager;	
	MemoryBlock*			Block;
	TMStatus				StatusCode;

	if ( ( Manager = objectAllocate ( 
		sizeof ( MemoryManagerObject ),
		MemoryManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:memoryManagerCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail; 
		goto memoryManagerCreateExit1;
	}
	

	Manager->MemoryCount	= Parameters->MemoryCount;
	Manager->SharedData		= Parameters->SharedData;
	Manager->HalHandle		= Parameters->HalHandle;
	Manager->MemorySize		= Parameters->MemorySize;
	Manager->MemoryBlock	= Parameters->MemoryBlock;
	Manager->Control		= Parameters->SharedData;
	Manager->MemoryFree		= Manager->MemorySize - sizeof(MemoryBlock);
	Manager->NameSpaceManagerHandle		= Parameters->NameSpaceManagerHandle;

	if ( objectlistCreate ( &Manager->List,  
		Manager->MemoryCount ) != True )
	{
		DPF(0,("tmman:memoryManagerCreate:objectlistCreate:FAIL\n"));
		StatusCode = statusObjectListAllocFail;
		goto	memoryManagerCreateExit2;
	}

#ifdef TMMAN_HOST	
	/* initialize the free memory block pointer */
	Block =  (MemoryBlock* )Manager->MemoryBlock;
	Block->Prev = Null;
	Block->Next = Null;
	Block->Flags.Allocated = False;
	Block->Flags.Length = COLLAPSE(Manager->MemoryFree);
	Block->Data = Block + 1 ;

	Manager->Head  = Block;
#endif

	if ( critsectCreate ( &Manager->CriticalSectionHandle ) == False )
	{
		DPF(0,("tmman:memoryManagerCreate:critSectCreate:FAIL\n"));
		StatusCode = statusCriticalSectionCreateFail;
		goto memoryManagerCreateExit3;
	}

/*	memoryManagerReset((UInt32)Manager); */

	*MemoryManagerHandlePointer = (UInt32)Manager;
		
	return statusSuccess;

/*
memoryManagerCreateExit4:
	critsectDestroy ( Manager->CriticalSectionHandle );
*/

memoryManagerCreateExit3:
	objectlistDestroy ( &Manager->List );

memoryManagerCreateExit2:
	objectFree ( Manager );

memoryManagerCreateExit1:
	return StatusCode;
}


TMStatus	memoryManagerDestroy ( 
	UInt32 MemoryManagerHandle )
{
	MemoryManagerObject*	Manager = 
		( MemoryManagerObject* )MemoryManagerHandle;

	if ( objectValidate ( Manager, MemoryManagerFourCC ) != True )
	{
		DPF(0,("tmman:memoryManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	critsectDestroy ( Manager->CriticalSectionHandle );

	objectlistDestroy ( &Manager->List );

	objectFree ( Manager );

	return statusSuccess;
}

TMStatus  memoryManagerDestroyMemoryByClient (
	UInt32	MemoryManagerHandle, 
	UInt32 ClientHandle )
{
	MemoryManagerObject*	Manager = 
		( MemoryManagerObject* )MemoryManagerHandle;
	MemoryObject*	Object;
	UInt32	Idx;

	if ( objectValidate ( Manager, MemoryManagerFourCC ) != True )
	{
		DPF(0,("tmman:memoryManagerDestroyMemoryByClient:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < Manager->MemoryCount ; Idx++ )
	{
		Object = objectlistGetObject (
			&Manager->List, 
			Idx );
		if ( ( Object ) && Object->ClientHandle == ClientHandle )
		{
#ifdef TMMAN_HOST
			memoryDestroy ( (UInt32) Object );
#else
			memoryClose ( (UInt32) Object );
#endif
		}
	}

	return statusSuccess;
}

TMStatus	memoryGetAddress ( 
	UInt32 MemoryHandle,
	UInt32* AddressPointer )
{
	MemoryObject*	Object =
		(MemoryObject*) MemoryHandle;
	MemoryManagerObject*	Manager;
	Manager = (MemoryManagerObject* )Object->MemoryManager;

	if ( objectValidate ( Object, MemoryFourCC ) != True )
	{
		DPF(0,("tmman:memoryGetAddress:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}
	
	*AddressPointer = (UInt32) Object->Address;

	return	statusSuccess;
}


TMStatus	memoryGetHalHandle ( 
	UInt32 MemoryHandle,
	UInt32* HalHandle )
{
	MemoryObject*			Object = (MemoryObject*) MemoryHandle;
	MemoryManagerObject*	Manager;

	if ( objectValidate ( Object, MemoryFourCC ) != True )
	{
		DPF(0,("tmman:memoryGetAddress:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	Manager = (MemoryManagerObject* )Object->MemoryManager;
		
	*HalHandle = Manager->HalHandle;

	return	statusSuccess;
}

#ifdef TMMAN_HOST
TMStatus	memoryCreate(
	UInt32	MemoryManagerHandle,
	Pointer	ListHead,
	String	Name, 
	UInt32	Length, 
	Pointer* AddressPointer,
	UInt32*	MemoryHandlePointer )
{
	MemoryManagerObject* Manager = 
		( MemoryManagerObject* )MemoryManagerHandle;
	MemoryObject*	Object;

	TMStatus		StatusCode;
	MemoryBlock*	Block;
	MemoryBlock*	FreeBlock;
	UInt32			AlignedLength;
	UInt32			NestedContext;

	if ( objectValidate ( Manager, MemoryManagerFourCC ) != True )
	{
		DPF(0,("tmman:memoryCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	AlignedLength = ( Length + 0x3 ) & 0xfffffffc;

	DPF(8,("tmman:memoryCreate:ReqLen[%lx]:AlignedSize[%lx]:AvailLen[%lx]\n",
		Length,AlignedLength, Manager->MemoryFree ));

	/* decide if we have enough left for this block */

	if ((AlignedLength + sizeof( MemoryBlock)) > Manager->MemoryFree)
	{
		DPF(0,("tmman:memoryCreate:Length[%lx] OUT OF MEMORY:FAIL\n",
			AlignedLength ));
		return statusMemoryUnavailable;
	}


	if ( ( Object = objectAllocate (
		sizeof ( MemoryObject ), MemoryFourCC ) ) == Null )
	{
		DPF(0,("tmman:memoryCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto memoryCreateExit1;	
	}

	if ( ( StatusCode = namespaceCreate  (
		Manager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectMemory,
		Name,
		&Object->MemoryNumber,
		&Object->NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:memoryCreate:namespaceCreate:FAIL[%lx]\n",
			StatusCode ));
		goto memoryCreateExit2;	
	}

	Object->MemoryManager = Manager;
	Object->ClientHandle = (UInt32)ListHead;

	if ( objectlistInsert ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber ) != True )
	{
		DPF(0,("tmman:memoryCreate:objectlistInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto memoryCreateExit3;	
	}
	for (Block = Manager->Head;	Block ; Block = Block->Next)
	{
	
		/* ENTER CRITICAL SECTION */
		critsectEnter ( Manager->CriticalSectionHandle, &NestedContext );
		
		/*
		   if the current control blcok is allocated or
		   it is too small to satisfy this request, then
		   go tho the next block

		*/
		if( (Block->Flags.Allocated) ||
		    (EXPAND(Block->Flags.Length) < (signed) AlignedLength))
		{
			/* LEAVE CRITICAL SECTION */
			critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );
			continue;
		}
		/*
		   Since we are storing the data & the control block
		   in the same area, there is no point in
		   alloacting a control block for a buffer size <
		   sizeof ( control block ). This is what I check
		   below and if the above is true I allocate the
		   entire memory block to the application.
		*/
		if( EXPAND(Block->Flags.Length) > (signed)(AlignedLength + 2 * sizeof(MemoryBlock)) )
		{
			/* allocate a new SHMEM_BLOCK in the free space */
			FreeBlock = 
				(MemoryBlock* )(((UInt8*)Block->Data) + AlignedLength);

			/* get the doubly link list pointers set */
			FreeBlock->Prev = Block;
			FreeBlock->Next = Block->Next;
			if( Block->Next)
				Block->Next->Prev = FreeBlock;

			Block->Next = FreeBlock;
			

			/*
				the rest of free memory minus the new block header 
				we are using to tag the free memory
			*/
			FreeBlock->Flags.Length = 
				 COLLAPSE( EXPAND(Block->Flags.Length) - ( AlignedLength +  sizeof(MemoryBlock) ) );

			/* the free block of memory is split into two parts */
			Block->Flags.Length		= COLLAPSE(AlignedLength);

			/* pMemory points to the newly allcoated block */
			Block->Flags.Allocated	= True;

			

			/* pNew Memory points to the remaining free space */
			FreeBlock->Flags.Allocated	= False;
			FreeBlock->Data				= FreeBlock + 1;

			halAccessEnable( Manager->HalHandle );
			Manager->Control[Object->MemoryNumber].Offset	= 
				halAccess32 ( Manager->HalHandle, 
				((UInt8*)Block->Data) - ((UInt8*)Manager->MemoryBlock) );

			Manager->Control[Object->MemoryNumber].Size	= 
				halAccess32 ( Manager->HalHandle, 
				AlignedLength );
			halAccessDisable( Manager->HalHandle );

			//{TRC
			Object->ControlOffset	= 
				((UInt8*)Block->Data) - ((UInt8*)Manager->MemoryBlock);
			Object->ControlSize		= AlignedLength;
			//}TRC

			Object->Address = ((UInt8*)Block->Data); 
			
			halAccessEnable( Manager->HalHandle );
			Object->Size = 
				halAccess32 ( Manager->HalHandle, 
				Manager->Control[Object->MemoryNumber].Size );
			halAccessDisable( Manager->HalHandle );

			Manager->MemoryFree -= ( AlignedLength + sizeof(MemoryBlock) );
		}
		else  /* memory too small to allocate another control block */
		{
			/* return the entire free block */
			Block->Flags.Allocated = True;

			halAccessEnable( Manager->HalHandle );
			Manager->Control[Object->MemoryNumber].Offset	= 
				halAccess32 ( Manager->HalHandle, 
				((UInt8*)Block->Data) - ((UInt8*)Manager->MemoryBlock) );
			Manager->Control[Object->MemoryNumber].Size	= 
				halAccess32 ( Manager->HalHandle, 
				AlignedLength );
			halAccessDisable( Manager->HalHandle );

			//{TRC
			Object->ControlOffset	= 
				((UInt8*)Block->Data) - ((UInt8*)Manager->MemoryBlock);
			Object->ControlSize		= AlignedLength;
			//}TRC

			Object->Address = ((UInt8*)Block->Data); 
			Object->Size = AlignedLength; 

			Manager->MemoryFree -= EXPAND(Block->Flags.Length);
		}

		/* LEAVE CRITICAL SECTION */
		critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );

		DPF(8,
			("tmman:memoryCreate:Block:P[%p]:B[%p]:N[%p]:L[%x]:A[%lx]\n",
			Block->Prev, 
			Block, 
			Block->Next, 
			EXPAND( Block->Flags.Length), 
			Manager->MemoryFree ));
		DPF(8,
			("tmman:memoryCreate:Object:Addr[%p]:Size[%lx]:Num[%lx]:Offs[%lx]:Manager:BlockBase[%p]\n",
			Object->Address,
			Object->Size, 
			Object->MemoryNumber,
			Manager->Control[Object->MemoryNumber].Offset,
			Manager->MemoryBlock ));
		*AddressPointer = Object->Address;
		*MemoryHandlePointer = (UInt32)Object;

		return	statusSuccess;
	}
	return statusMemoryUnavailable;

/*
memoryCreateExit3:
	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber );
*/
memoryCreateExit3:
	namespaceDestroy ( Object->NameSpaceHandle );

memoryCreateExit2:
	objectFree ( Object );

memoryCreateExit1:
	return StatusCode;

}


TMStatus	memoryDestroy ( 
	UInt32 MemoryHandle)
{
	MemoryManagerObject*	Manager;
	MemoryObject*	Object =
		(MemoryObject*) MemoryHandle;
	MemoryBlock*		Block;
	UInt32				NestedContext;

	if ( objectValidate ( Object, MemoryFourCC ) != True )
	{
		DPF(0,("tmman:memoryDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	Manager = (MemoryManagerObject* )Object->MemoryManager;

	Block = Object->Address;

	Block--; /* memory now points to MemoryBlock */

	if ( Block->Data != Object->Address )
	{
		DPF(0,("tmman:memoryDestroy:Block[%p] INVALID BLOCK:FAIL\n",
			Object->Address ));
		return statusInvalidHandle;
	}

	if ( ! Block->Flags.Allocated )
	{
		DPF(0,("tmman:memroyDestroy:Block[%p] NOT ALLOCATED:FAIL\n",
			Object->Address ));
		return statusInvalidHandle;
	}

	DPF(8,
		("tmman:memoryDestroy:P[%p]:B[%p]:N[%p]:L[%x]:A[%lx]\n",
		Block->Prev, 
		Block, 
		Block->Next, 
		EXPAND( Block->Flags.Length), 
		Manager->MemoryFree ));

	Block->Flags.Allocated = False;

	/* ENTER CRITICAL SECTION */

	critsectEnter ( Manager->CriticalSectionHandle, &NestedContext );

	Manager->MemoryFree += EXPAND ( Block->Flags.Length );
	
	
	/* check if we are dealing with the first control block */
	if( Block->Prev != Null )
	{
		/*
			if the previous block also free then we combine
			the two blocks in to a single one to prevent
			fragmentation.
		*/
		if (Block->Prev->Flags.Allocated == False )
		{
			/* remove this block from the doubly link list. */
			/* adjust the next pointer of the previous block */
			Block->Prev->Next = Block->Next;

			/* 
				adjust the previous pointer of the next block 
				if the block we are freeing is not the last 
				block in the list
			*/
			if ( Block->Next )
				Block->Next->Prev = Block->Prev;

			/* 
				add the length of the current block to the previous 
				block, also we are going to get rid of the current
				MEMORY_OBJECT so add the size of that to the 
				previous block and to the memory manager free space
			*/
			Block->Prev->Flags.Length =  COLLAPSE ( EXPAND ( Block->Prev->Flags.Length ) +
				EXPAND( Block->Flags.Length)   + sizeof ( MemoryBlock ) );
			Manager->MemoryFree  += sizeof ( MemoryBlock );

			/* make the pMemory point to the previous node. */
			Block = Block->Prev;
		}
	}
	/* 
		So far we have combined the length of the block we are trying to free
		with the previous block.
		At this point if the block previous to the one we are tyring to free was free
		then Block is currently pointing to that block.

		Not we try to do the same with the next block 
	*/


	/* NOTE : now pMemory points to the previous SHMEM_BLOCK */
	/* check if we are dealing with the last control block */
	if ( Block->Next != Null )
	{
		/* 
			if the next node is free, we have to combine that
			too into the previous block. we always combine
			with the previous block.
		*/
		if ( Block->Next->Flags.Allocated == False )
		{
			/* pMemory always points to the block that has to be removed */
			Block = Block->Next;

			/* remove this block from the doubly link list. */
			Block->Prev->Next = Block->Next;

			/* check if there is another block after this block */
			if ( Block->Next ) 
				Block->Next->Prev = Block->Prev;

			/* adjust the length */
			Block->Prev->Flags.Length = 
				COLLAPSE ( EXPAND( Block->Prev->Flags.Length ) +
				EXPAND (Block->Flags.Length)  + sizeof ( MemoryBlock ) );
			
			

			Manager->MemoryFree  += sizeof ( MemoryBlock );

		}
	}

	/* LEAVE CRITICAL SECTION */
	critsectLeave ( Manager->CriticalSectionHandle, &NestedContext );
/*
	DPF(8,("tmman:memoryDestroy:MemorySize[%x]:AvailLen[%x]\n",
		Manager->MemorySize, Manager->MemoryFree ));
*/

	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber );

	namespaceDestroy ( Object->NameSpaceHandle );

	objectFree ( Object );



	return	statusSuccess;

}


#else

TMStatus  memoryOpen ( 
	UInt32 MemoryManagerHandle, 
	Pointer	ListHead,
	String	Name,
	UInt32* LengthPtr,
	Pointer* AddressPointer,
	UInt32*	MemoryHandlePointer )
{
	MemoryManagerObject* Manager = 
		( MemoryManagerObject* )MemoryManagerHandle;
	MemoryObject*	Object;
	TMStatus		StatusCode;

	if ( objectValidate ( Manager, MemoryManagerFourCC ) != True )
	{
		DPF(0,("tmman:memoryOpen:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( ( Object = objectAllocate (
		sizeof ( MemoryObject ), MemoryFourCC ) ) == Null )
	{
		DPF(0,("tmman:memoryOpen:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto memoryOpenExit1;	
	}

	if ( ( StatusCode = namespaceCreate  (
		Manager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectMemory,
		Name,
		&Object->MemoryNumber,
		&Object->NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:memoryOpen:namespceCreate:FAIL[%x]\n", StatusCode ));
		goto memoryOpenExit2;	
	}

	if ( objectlistInsert ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber ) != True )
	{
		DPF(0,("tmman:memoryOpen:objectlistInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto memoryOpenExit3;	
	}

	Object->MemoryManager	= Manager;
	
	halAccessEnable( Manager->HalHandle );
	Object->Address			= 
		((UInt8*)Manager->MemoryBlock) + halAccess32 ( Manager->HalHandle, 
		Manager->Control[Object->MemoryNumber].Offset );
	Object->Size			= halAccess32 ( Manager->HalHandle, 
		Manager->Control[Object->MemoryNumber].Size );
	halAccessDisable( Manager->HalHandle );

	Object->ClientHandle	= (UInt32)ListHead;
	*AddressPointer			= Object->Address;
	*LengthPtr = Object->Size;

	*MemoryHandlePointer = (UInt32)Object;

	DPF(8,
		("tmman:memoryOpen:Object:Addr[%x]:Size[%x]:Num[%x]:Manager:BlockBase[%x]\n",
		Object->Address,
		Object->Size, 
		Object->MemoryNumber,
		Manager->MemoryBlock ));

	return statusSuccess;
/*
memoryOpenExit4:
	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber );
*/
memoryOpenExit3:
	namespaceDestroy ( Object->NameSpaceHandle );

memoryOpenExit2:
	objectFree ( Object );

memoryOpenExit1:
	return StatusCode;

}

TMStatus  memoryClose (
	UInt32	MemoryHandle )
{
	MemoryManagerObject*	Manager;
	MemoryObject*	Object =
		(MemoryObject*) MemoryHandle;

	if ( objectValidate ( Object, MemoryFourCC ) != True )
	{
		DPF(0,("tmman:memoryClose:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	Manager = (MemoryManagerObject*)Object->MemoryManager;

	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->MemoryNumber );

	namespaceDestroy ( Object->NameSpaceHandle );

	objectFree ( Object );

	return	statusSuccess;
}


#endif

