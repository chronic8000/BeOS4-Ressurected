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
	sgbuffer.c

	Pvides a mechanism for the operating system running on one processor
	to signal the operating system running on another processor.

	Author : T.Roy (Tilakraj.Roychoudhury@sv.sc.philips.com)

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
/*	CONSTANTS */

#define		SGBufferManagerFourCC		tmmanOBJECTID ( 'S', 'G', 'B', 'M' )
#define		SGBufferFourCC				tmmanOBJECTID ( 'S', 'G', 'B', 'O' )

/* TYPDEFS */

typedef struct tagSGBufferObject
{
	GenericObject	Object;
	UInt32			SGBufferManager;
	UInt32			MemoryHandle;
	UInt32			NameSpaceHandle;
	UInt32			ClientHandle;
	UInt32			PageTableHandle;

	UInt32			SGBufferNumber;
	UInt32			BufferSize;
	UInt32			EntryCount;
	Pointer*		PageTableBuffer; /* pointer to the scratch buffer for pagetableCreate */

	PageTableEntry*	Control;		/* pointer to the base of shared memory */
	PageTableEntry*	Entries;		/* pointer to the page table entry list in the shared memory */
	

	/* use only on the target */
	UInt32	CurrentEntry;
	UInt32	CurrentOffset;

	/* used only on the host */
	PageTableEntry*	ControlPageTable; /* pointer to the local page table list */

}	SGBufferObject;

typedef struct tagSGBufferManagerObject
{
	GenericObject			Object;
	ObjectList				List;
	UInt32					Count;
	UInt32					HalHandle;
	UInt32					MemoryManagerHandle;
	UInt32					NameSpaceManagerHandle;
	UInt32					CriticalSectionHandle;
}	SGBufferManagerObject;


/* GLOBAL DATA STRUCTURES */
/* NONE - key to object oriented programming */


/* FUNCTIONS */

TMStatus	sgbufferManagerCreate ( 
	sgbufferManagerParameters* Parameters,
	UInt32* SGBufferManagerHandle )
{
	TMStatus		StatusCode;

	SGBufferManagerObject*	Manager;

	if ( ( Manager = objectAllocate ( 
		sizeof ( SGBufferManagerObject ),
		SGBufferManagerFourCC ) ) == Null )
	{
		DPF(0,("tmman:sgbufferManagerCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail; 
		goto sgbufferManagerCreateExit1;
	}

	Manager->Count					= Parameters->SGBufferCount;
	Manager->HalHandle				= Parameters->HalHandle;
	Manager->MemoryManagerHandle	= Parameters->MemoryManagerHandle;
	Manager->NameSpaceManagerHandle	= Parameters->NameSpaceManagerHandle;

	if ( objectlistCreate ( &Manager->List,  
		Manager->Count ) != True )
	{
		DPF(0,("tmman:sgbufferManagerCreate:objectlistCreate:FAIL\n"));
		StatusCode = statusObjectListAllocFail;
		goto	sgbufferManagerCreateExit2;
	}

	if ( critsectCreate ( &Manager->CriticalSectionHandle ) == False )
	{
		DPF(0,("tmman:sgbufferManagerCreate:critsectCreate:FAIL\n"));
		StatusCode = statusCriticalSectionCreateFail;
		goto sgbufferManagerCreateExit3;
	}

	*SGBufferManagerHandle  = (UInt32)Manager;

	return statusSuccess;

/*
sgbufferManagerCreateExit3:
	critsectDestroy ( Manager->CriticalSectionHandle );
*/

sgbufferManagerCreateExit3:
	objectlistDestroy ( &Manager->List );

sgbufferManagerCreateExit2:
	objectFree ( Manager );

sgbufferManagerCreateExit1:
	return StatusCode;
}

TMStatus	sgbufferManagerReset ( 
	UInt32 SGBufferManagerHandle )
{
	SGBufferManagerObject* Manager = 
		( SGBufferManagerObject* )SGBufferManagerHandle;

	SGBufferObject* Object;

	UInt32	Idx;

	if ( objectValidate ( Manager, SGBufferManagerFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferManagerReset:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	for ( Idx = 0 ; Idx < Manager->Count ; Idx ++ )
	{
		UInt32	EntryIdx;

		if ( ( Object = objectlistGetObject (
			&Manager->List, 
			Idx ) ) != Null )
		{
			halAccessEnable( Manager->HalHandle );

			Object->Control[0].PhysicalAddress = 
				halAccess32 ( Manager->HalHandle, Object->EntryCount );

			Object->Control[0].RunLength = 
				halAccess32 ( Manager->HalHandle, Object->BufferSize );

			halAccessDisable( Manager->HalHandle );

			for ( EntryIdx = 0 ; EntryIdx < Object->EntryCount ; EntryIdx ++ )
			{
				halAccessEnable( Manager->HalHandle );

				Object->Control[EntryIdx+1].PhysicalAddress = 
					halAccess32 ( Manager->HalHandle, 
					Object->ControlPageTable[EntryIdx].PhysicalAddress );

				Object->Control[EntryIdx+1].RunLength = 
					halAccess32 ( Manager->HalHandle, 
					Object->ControlPageTable[EntryIdx].RunLength );

				halAccessDisable( Manager->HalHandle );
			}
		
		}
	
	}

	return statusSuccess;
}

TMStatus	sgbufferManagerDestroy ( 
	UInt32 SGBufferManagerHandle )
{
	SGBufferManagerObject* Manager = 
		( SGBufferManagerObject* )SGBufferManagerHandle;

	if ( objectValidate ( Manager, SGBufferManagerFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferManagerDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	critsectDestroy ( Manager->CriticalSectionHandle );

	objectlistDestroy ( &Manager->List );

	objectFree ( Manager );

	return statusSuccess;
}

TMStatus  sgbufferManagerDestroySGBufferByClient (
	UInt32	SGBufferManagerHandle, 
	UInt32 ClientHandle )
{
	SGBufferManagerObject* Manager = 
		( SGBufferManagerObject* )SGBufferManagerHandle;
	SGBufferObject*	Object;
	UInt32	Idx;

	if ( objectValidate ( Manager, SGBufferManagerFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferManagerDestroySGBufferByClient:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}


	for ( Idx = 0 ; Idx < Manager->Count ; Idx++ )
	{
		Object = objectlistGetObject (
			&Manager->List, 
			Idx );
		if ( ( Object ) && Object->ClientHandle == ClientHandle )
		{

#ifdef TMMAN_HOST
			sgbufferDestroy ( (UInt32) Object );
#else
			
#endif
		}
	}

	return statusSuccess;
}
/*
	This is the general algorithm of how scatter gather locking is going to 
	work.
	The shraed PTE has 1 extra entry in the begining that contains the 
	Size and the number of actual PTE entries. If it is not done this way then
	we would be wasting another shared memory block where sgbuffer has to store its 
	private data structures.

	platform.h has a structure PageLockBufferDescription which is transparent to
		this file
	tmif calls sgbufferCreate with pointer PageLockBufferDescription
	sgbufferCreate
		ContextPointerLength = pagetableGetSize ( BufferDescription );

		memAllocate ( ContextPointerLength );

		ContextPointer = pagetableCreate ( 
			BufferDescription,
			ContextPointer,
			&PTEPointer,
			&EntryCount,
			&Size,
			&Address,
			PageTableHandle );
		
		SharedPTE = memoryCreate ( sizeof ( PageTableEntry ) * ( EntryCount + 1) );

		memCopy ( PTEPoitner, ShredPTE, sizeof ( PageTableEntry ) * EntryCount );

		memFree ( contextPointer );




*/
#ifdef TMMAN_HOST
TMStatus	sgbufferCreate (
	UInt32	SGBufferManagerHandle,
	Pointer	ListHead,
	String	Name,
	UInt32	BufferAddress,
	UInt32	BufferSize,
	UInt32	Flags,
	UInt32*	SGBufferHandlePointer )
{
	SGBufferManagerObject* Manager = 
		( SGBufferManagerObject* )SGBufferManagerHandle;
	SGBufferObject*	SGBuffer;

	TMStatus		StatusCode;
	UInt32			Idx;
	UInt32			EstimatedScatterTableSize;
	UInt8			MemoryName[constTMManNameSpaceNameLength];

	if ( objectValidate ( Manager, SGBufferManagerFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferCreate:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( ( SGBuffer = objectAllocate (
		sizeof ( SGBufferObject ), SGBufferFourCC ) ) == Null )
	{
		DPF(0,("tmman:sgbufferCreate:objectAllocate:FAIL\n"));
		StatusCode = statusObjectAllocFail;
		goto sgbufferCreateExit1;	
	}

	SGBuffer->BufferSize  = BufferSize;
	SGBuffer->SGBufferManager = (UInt32)Manager;
	SGBuffer->ClientHandle = (UInt32)ListHead;


	if ( ( StatusCode = namespaceCreate  (
		Manager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectSGBuffer,
		Name,
		&SGBuffer->SGBufferNumber,
		&SGBuffer->NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:sgbufferCreate:namespaceCreate:FAIL[%x]\n", 
			StatusCode ));
		goto sgbufferCreateExit2;	
	}


	if ( objectlistInsert ( 
		&Manager->List, 
		SGBuffer,
		SGBuffer->SGBufferNumber ) != True )
	{
		DPF(0,("tmman:sgbufferCreate:objectlistInsert:FAIL\n"));
		StatusCode = statusObjectInsertFail;
		goto sgbufferCreateExit3;	
	}


	/* 
		make the name unique by putting the calling object type
		before the name.
	*/


	if ( ! ( EstimatedScatterTableSize = 
		pagetableGetTempBufferSize ( 
			BufferAddress, SGBuffer->BufferSize ) ) )
	{
		DPF(0,("tmman:sgbufferCreate:pagetableGetSize:FAIL\n"));
		StatusCode = statusSGBufferPageTableSizeFail;
		goto sgbufferCreateExit4;	
	}

	/*
	 allocate a local buffer for retrieving the page table.
	 pagetableCreate does not allocate this buffer
	 as we don't need this buffer after this call
	 and it would not get deallocated until pagetableDestroy
	 is called
	*/
	if ( ( SGBuffer->PageTableBuffer =  memAllocate ( 
		EstimatedScatterTableSize ) ) == Null )
	{
		DPF(0,("tmman:sgbufferCreate:memAlloc:FAIL\n"));
		StatusCode = statusSGBufferTempPageAllocFail;
		goto sgbufferCreateExit4;	
	}

	/* page lock the memory and receive the page table in the buffer provided 
		we need not pass the EstimatedScatterTableSize as it can be derived
		from the BufferDescritpion

	*/
	
	if ( pagetableCreate ( 
		BufferAddress,
		SGBuffer->BufferSize,
		SGBuffer->PageTableBuffer,
		Manager->HalHandle,
		&SGBuffer->ControlPageTable,
		&SGBuffer->EntryCount,
		&SGBuffer->PageTableHandle ) != True )
	{
		DPF(0,("tmman:sgbufferCreate:pagetableCreate:FAIL\n"));
		StatusCode = statusSGBufferPageLockingFail;
		goto sgbufferCreateExit5;	
	}
	
	sprintf ( MemoryName, "%d\\%s", 
		constTMManNameSpaceObjectSGBuffer, Name );

	if ( ( StatusCode = memoryCreate (
		Manager->MemoryManagerHandle,
		ListHead,
		MemoryName, 
		(SGBuffer->EntryCount + 1) * sizeof(PageTableEntry), /* one extra entry for table size & count */
		&SGBuffer->Control,
		&SGBuffer->MemoryHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:sgbufferCreate:memoryCreate:FAIL[%x]\n", StatusCode ));
		goto sgbufferCreateExit6;
	}

	/* 
		the first entry contains the buffer size & count
		PhysicalAddress = number of entries
		BufferSize = Total amount of memory locked
	*/

	SGBuffer->Entries = SGBuffer->Control + 1;

	halAccessEnable( Manager->HalHandle );

	SGBuffer->Control->PhysicalAddress = halAccess32( 
			Manager->HalHandle, SGBuffer->EntryCount );

	SGBuffer->Control->RunLength = halAccess32( 
			Manager->HalHandle, SGBuffer->BufferSize );

	halAccessDisable( Manager->HalHandle );

	DPF(0,("tmman:sgbufferCreate:EntryCount[%x]:BufferSize[%x]\n", SGBuffer->EntryCount, SGBuffer->BufferSize ));

	for ( Idx = 0 ; Idx < SGBuffer->EntryCount ; Idx++ )
	{
		halAccessEnable( Manager->HalHandle );

		SGBuffer->Entries[Idx].PhysicalAddress  = halAccess32( 
			Manager->HalHandle,
			SGBuffer->ControlPageTable[Idx].PhysicalAddress );

		SGBuffer->Entries[Idx].RunLength = halAccess32( 
			Manager->HalHandle,
			SGBuffer->ControlPageTable[Idx].RunLength );

		halAccessDisable( Manager->HalHandle );

		DPF(0,("tmman:sgbufferCreate:Entry[%x]:PhysicalAddress[%x]:RunLength[%x]\n", 
			Idx, SGBuffer->Entries[Idx].PhysicalAddress, SGBuffer->Entries[Idx].RunLength ));

	}
	
	*SGBufferHandlePointer = (UInt32)SGBuffer;

	return statusSuccess;

/*
sgbufferCreateExit7 :
	memoryDestroy ( SGBuffer->MemoryHandle );
*/

sgbufferCreateExit6 :	
	pagetableDestroy ( SGBuffer->PageTableHandle );

sgbufferCreateExit5 :
	memFree ( SGBuffer->PageTableBuffer );

sgbufferCreateExit4 :
	objectlistDelete ( 
		&Manager->List, 
		SGBuffer,
		SGBuffer->SGBufferNumber );


sgbufferCreateExit3 :
	namespaceDestroy ( SGBuffer->NameSpaceHandle );

sgbufferCreateExit2 :
	objectFree ( SGBuffer );

sgbufferCreateExit1 :
	return StatusCode;
}

TMStatus	sgbufferDestroy ( 
	UInt32 SGBufferHandle )
{
	SGBufferManagerObject*	SGBufferManager;
	SGBufferObject*	SGBuffer =
		(SGBufferObject*) SGBufferHandle;
	UInt32	NameSpaceHandle;

	if ( objectValidate ( SGBuffer, SGBufferFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferDestroy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	SGBufferManager = (SGBufferManagerObject* )SGBuffer->SGBufferManager;
	NameSpaceHandle = SGBuffer->NameSpaceHandle;


	memoryDestroy ( SGBuffer->MemoryHandle );

	pagetableDestroy ( SGBuffer->PageTableHandle );

	memFree ( SGBuffer->PageTableBuffer );

	objectlistDelete ( 
		&SGBufferManager->List, 
		SGBuffer,
		SGBuffer->SGBufferNumber );

	namespaceDestroy ( NameSpaceHandle );

	objectFree ( SGBuffer );

	return statusSuccess;
}

	
#else /* TARGET */

TMStatus	sgbufferOpen ( 
	UInt32	SGBufferManagerHandle, 
	Pointer	ListHead,
	String	Name,
	UInt32*	EntryCountPointer,
	UInt32* SizePointer,
	UInt32*	SGBufferHandlePointer )
{
	SGBufferManagerObject*	Manager = (SGBufferManagerObject*)SGBufferManagerHandle;
	SGBufferObject*			Object;
	Int8			MemoryName[constTMManNameSpaceNameLength];
	UInt32			Dummy, Idx;
	TMStatus		StatusCode;

	if ( objectValidate ( Manager, SGBufferManagerFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferOpen:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( ( Object = objectAllocate (
		sizeof ( SGBufferObject ), SGBufferFourCC ) ) == Null )
	{
		DPF(0,("tmman:sgbufferOpen:objectAllocate:FAIL\n" ));
		StatusCode = statusObjectAllocFail;
		goto sgbufferOpenExit2;	
	}

	if ( ( StatusCode = namespaceCreate  (
		Manager->NameSpaceManagerHandle,
		constTMManNameSpaceObjectSGBuffer,
		Name,
		&Object->SGBufferNumber,
		&Object->NameSpaceHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:sgbufferOpen:namespaceCreate:FAIL[%x]\n", 
			StatusCode ));
		goto sgbufferOpenExit1;	
	}

	if ( objectlistInsert ( 
		&Manager->List, 
		Object,
		Object->SGBufferNumber ) != True )
	{
		DPF(0,("tmman:sgbufferOpen:objectlistInsert:FAIL\n" ));
		StatusCode = statusObjectInsertFail;
		goto sgbufferOpenExit3;	
	}

	sprintf ( MemoryName, "%d\\%s", 
		constTMManNameSpaceObjectSGBuffer, Name );

	if ( ( StatusCode = memoryOpen (
		Manager->MemoryManagerHandle,
		ListHead,
		MemoryName, 
		&Dummy,
		(Pointer *)&Object->Control,
		&Object->MemoryHandle ) ) != statusSuccess )
	{
		DPF(0,("tmman:sgbufferOpen:memoryOpen:FAIL[%x]\n",StatusCode ));
		goto sgbufferOpenExit4;
	}

	/* 
		the first entry contains the length of the entire buffer 
		and the number of PTEs 
	*/

	halAccessEnable( Manager->HalHandle );

	Object->BufferSize = halAccess32 ( Manager->HalHandle, 
		Object->Control->RunLength );

	Object->EntryCount = halAccess32 ( Manager->HalHandle,
		Object->Control->PhysicalAddress );

	halAccessDisable( Manager->HalHandle );

	Object->CurrentEntry = 0;
	Object->CurrentOffset = 0;
	Object->SGBufferManager = Manager;
	Object->Entries = Object->Control + 1;

	DPF(0,("tmman:sgbufferOpen:EntryCount[%x]:BufferSize[%x]\n", Object->EntryCount, Object->BufferSize ));

	for ( Idx = 0 ; Idx < Object->EntryCount ; Idx++ )
	{
		halAccessEnable( Manager->HalHandle );

		DPF(0,("tmman:sgbufferOpen:Entry[%x]:PhysicalAddress[%x]:RunLength[%x]\n", 
			Idx, Object->Entries[Idx].PhysicalAddress, Object->Entries[Idx].RunLength ));

		halAccessDisable( Manager->HalHandle );


	}


	*EntryCountPointer		= Object->EntryCount;
	*SizePointer			= Object->BufferSize;
	*SGBufferHandlePointer = (UInt32)Object;

	return statusSuccess;

/*
sgbufferOpenExit5 :
	memoryClose ( Object->MemoryHandle );
*/

sgbufferOpenExit4 :
	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->SGBufferNumber );

sgbufferOpenExit3 :
	namespaceDestroy ( Object->NameSpaceHandle );

sgbufferOpenExit2 :
	objectFree ( Object );

sgbufferOpenExit1 :
	return StatusCode;
}

TMStatus	sgbufferClose (
	UInt32	SGBufferHandle )
{
	SGBufferObject*	Object =	(SGBufferObject*)SGBufferHandle;
	SGBufferManagerObject *Manager;

	if ( objectValidate ( Object, SGBufferFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferClose:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	Manager = (SGBufferManagerObject* )Object->SGBufferManager;

	memoryClose ( Object->MemoryHandle );

	objectlistDelete ( 
		&Manager->List, 
		Object,
		Object->SGBufferNumber );

	namespaceDestroy ( Object->NameSpaceHandle );

	objectFree ( Object );


	return statusSuccess;
}


TMStatus	sgbufferFirstBlock ( 
	UInt32	SGBufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer )
{
	/* remote object - this object exists on the host */
	SGBufferObject*	Object = (SGBufferObject*)SGBufferHandle;

	if ( objectValidate ( Object, SGBufferFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferFirstBlock:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	Object->CurrentEntry = 0;
	Object->CurrentOffset = 0;

	return	sgbufferNextBlock ( 
		SGBufferHandle,
		OffsetPointer, 
		AddressPointer, 
		SizePointer );
}


TMStatus	sgbufferNextBlock ( 
	UInt32	SGBufferHandle,
	UInt32* OffsetPointer, 
	UInt32* AddressPointer, 
	UInt32* SizePointer )
{
	/* remote object - this object exists on the host */
	SGBufferObject*	Object = (SGBufferObject*)SGBufferHandle;
	SGBufferManagerObject*	Manager = (SGBufferManagerObject*)Object->SGBufferManager;

	if ( objectValidate ( Object, SGBufferFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferNextBlock:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( Object->EntryCount == 0 )
	{
		DPF(0,("tmman:sgbufferNextBlock:ZERO EntryCount:FAIL\n"));
		return statusSGBufferInvalidPageTable;
	}

	if ( Object->CurrentEntry >= Object->EntryCount )
	{
		DPF(0,("tmman:sgbufferNextBlock:CurrentEntry[%x] INVALID:FAIL\n",
			Object->CurrentEntry ));
		return statusSGBufferNoMoreEntries;
	}

	*OffsetPointer = Object->CurrentOffset;
	
	halAccessEnable( Manager->HalHandle );

	*AddressPointer = halAccess32( Manager->HalHandle, 
		Object->Entries[Object->CurrentEntry].PhysicalAddress );
	*SizePointer = halAccess32 ( Manager->HalHandle,
		Object->Entries[Object->CurrentEntry].RunLength );
	
	halAccessDisable( Manager->HalHandle );

	/* prepare for the call to sgbufferNextBlock */
	Object->CurrentOffset += Object->Entries[Object->CurrentEntry].RunLength;
	Object->CurrentEntry++;

	return statusSuccess;

}

TMStatus	sgbufferCopy ( 
	UInt32	SGBufferHandle,
	UInt32	Offset,
	UInt32	Address, 
	UInt32	Size, 
	UInt32	Direction )
{

	UInt32	IdxPTE;
	UInt32	BlockSize;		/* size for the current memcopy */
	UInt32	BlockStart;		/* offset from the beginning of the buffer */
	UInt32	BytesCopied;	/* bytes memcopied so far */
	SGBufferObject*	Object = (SGBufferObject*)SGBufferHandle;
	SGBufferManagerObject*	Manager = (SGBufferManagerObject*)Object->SGBufferManager;

	if ( objectValidate ( Object, SGBufferFourCC ) != True )
	{
		DPF(0,("tmman:sgbufferCopy:objectValidate:FAIL\n"));
		return statusInvalidHandle;
	}

	if ( Object->EntryCount == 0 )
	{
		DPF(0,("tmman:sgbufferCopy:ZERO EntryCount:FAIL\n"));
		return statusSGBufferInvalidPageTable;
	}

	if ( Offset > Object->BufferSize )
	{
		DPF(0,("tmman:sgbufferCopy:Offset[%x] OUT OF RANGE:FAIL\n",
			Offset ));
		return statusSGBufferOffsetOutOfRange;
	}


	/* validate the size requested */
	if ( ( Offset + Size ) >  Object->BufferSize )
	{
		DPF(0,("tmman:sgbufferCopy:Size[%x] OUT OF RANGE:FAIL\n",
			Size ));
		return statusSGBufferSizeOutOfRange;
	}

	/* base linear address of the buffer */
	BlockStart = 0;
	BytesCopied = 0;

	for ( IdxPTE = 0 ; IdxPTE < Object->EntryCount ; IdxPTE ++)
	{
		UInt32		PhysicalAddress, RunLength;
		
		halAccessEnable( Manager->HalHandle );

		RunLength = halAccess32 ( Manager->HalHandle, 
			Object->Entries[IdxPTE].RunLength );

		PhysicalAddress = halAccess32 ( Manager->HalHandle, 
			Object->Entries[IdxPTE].PhysicalAddress );

		halAccessDisable( Manager->HalHandle );

		/* offset is from the begining of the entire buffer */
		if ( ( Offset >=  BlockStart  )  && 
			( Offset <= ( BlockStart + RunLength ) ) )
		{
			/* we found the first PTE encompassing the offset */
			/* Offset points to the middle of the current block */
			BlockSize = RunLength - ( Offset - BlockStart );

			halAccessEnable( Manager->HalHandle );

			/* this is the first copy so bytes copied is assumed to be 0 */
			if ( Direction ) /* host to target */
			{
				memCopy ( 
					Address, 
					PhysicalAddress + RunLength - BlockSize,
					BlockSize );
			}
			else
			{
				memCopy ( 
					PhysicalAddress + RunLength - BlockSize,
					Address,
					BlockSize );

			}

			halAccessDisable( Manager->HalHandle );

			BlockStart += RunLength;

			BytesCopied = BlockSize;

			break;
		}

		/* 
			if Offset does not fall in this range increment BlockStart by the 
			length of the current block. 
		*/
		BlockStart += RunLength; 
	}

	for ( IdxPTE++ ; IdxPTE < Object->EntryCount ; IdxPTE ++)
	{
		UInt32		PhysicalAddress, RunLength;

		halAccessEnable( Manager->HalHandle );

		RunLength = halAccess32 ( Manager->HalHandle, 
			Object->Entries[IdxPTE].RunLength );

		PhysicalAddress = halAccess32 ( Manager->HalHandle, 
			Object->Entries[IdxPTE].PhysicalAddress );

		halAccessDisable( Manager->HalHandle );

		/* 
			check if this is the last block we have to deal with,
			the last block may be partial
		*/
		if ( ( ( Offset + Size ) >=  BlockStart  )  && 
			( ( Offset + Size ) <= ( BlockStart + RunLength ) ) )
		{
			/* we found the last PTE encompassing the ( offset + size ) */
			/* ( Offset + Size ) points to the middle of the current block */
			BlockSize = ( Offset + Size ) - BlockStart ;

			halAccessEnable( Manager->HalHandle );

			if ( Direction )
			{
				memCopy ( 
					(Pointer)( Address + BytesCopied), 
					(Pointer)PhysicalAddress,
					BlockSize );
			}
			else
			{
				memCopy ( 
					(Pointer)PhysicalAddress,
					(Pointer)( Address + BytesCopied), 
					BlockSize );

			}

			halAccessDisable( Manager->HalHandle );

			BytesCopied += BlockSize;

			break;
		}
		else
		{
			/* 
				copy this block entirely , as it does not encompass the begining 
				or end of the buffer
			*/
			halAccessEnable( Manager->HalHandle );

			if ( Direction )
			{

				memCopy ( 
					(Pointer)( Address + BytesCopied ), 
					(Pointer)PhysicalAddress,
					RunLength );
			}
			else
			{
				memCopy (
					(Pointer)PhysicalAddress,
					(Pointer)( Address + BytesCopied ), 
					RunLength );
			}

			halAccessDisable( Manager->HalHandle );

			
			BytesCopied += RunLength;
		}

		BlockStart += RunLength; 
	}

	return statusSuccess;
}

#endif
