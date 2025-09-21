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
    FILE    :    misc.c

	HISTORY
	970703	Tilakraj Roy 	Created

    ABSTRACT
    Contains OS independent support functions that don't fit anywhere else.
	
*/

/*----------------------------------------------------------------------------
          SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/
#include "tmmanlib.h"

/* OBJECT MANIPULATION FUNCTIONS */
Pointer	objectAllocate ( UInt32 Size, UInt32 FourCCID )
{
	GenericObject*	Object;
	if ( ( Object = memAllocate ( Size ) ) == Null )
	{
		DPF(0,("tmman:objectAllocate:memAllocate:Size[%lx]:FAIL\n", (uint32)Size ));
		return Null;
	}

	Object->FourCCID = FourCCID;
	Object->SelfRef	= Object;
	return Object;
}

UInt32	objectValidate ( Pointer Object, UInt32 FourCCID )
{
	if ( ! Object )
	{
		DPF(0,("tmman:objectValidate:NULL Object:FAIL\n"));
		goto objectValidateFail;
	}

	if ( ((GenericObject*)Object)->FourCCID != FourCCID )
	{
		DPF(0,("tmman:objectValidate:FourCCID MISMATCH:FAIL\n" ));
		goto objectValidateFail;
	}

	if ( ((GenericObject*)Object)->SelfRef != Object )
	{
		DPF(0,("tmman:objectValidate:INVALID SelfRef:FAIL\n" ));
		goto objectValidateFail;
	}
	return True;

objectValidateFail :
        DPF(0,( "tmman:objectValidate:object: FourCC[%c%c%c%c]:FAIL\n",
		((UInt8*) (&( ((GenericObject*)Object)->FourCCID )) )[0], 
		((UInt8*) (&( ((GenericObject*)Object)->FourCCID )) )[1], 
                ((UInt8*) (&( ((GenericObject*)Object)->FourCCID )) )[2], 
                ((UInt8*) (&( ((GenericObject*)Object)->FourCCID )) )[3] 
               ));
	
	DPF(0,("tmman:objectValidate:FourCC[%c%c%c%c]:FAIL\n",
		((UInt8*)&FourCCID)[0], 
		((UInt8*)&FourCCID)[1], 
		((UInt8*)&FourCCID)[2], 
		((UInt8*)&FourCCID)[3] ));
	return False;
}

void	objectFree ( Pointer Object )
{
	((GenericObject*)Object)->FourCCID = 0;
	((GenericObject*)Object)->SelfRef = Null;
	memFree ( Object );
}


/* OBJECT LIST MANIPUTATION FUNCTIONS */
UInt32 objectlistCreate ( 
	Pointer ObjectListPointer, 
	UInt32 Count )
{
	ObjectList*	List = (ObjectList*)ObjectListPointer;
	UInt32 Idx;

	List->Count = Count;
	List->FreeCount = Count;
	if ( ( List->Objects = 
		memAllocate ( 
		sizeof ( Pointer ) * List->Count ) ) == Null )
	{
		DPF(0,("tmman:objectlistCreate:memAllocate:Size[%lx]:FAIL\n",
			sizeof ( Pointer ) * List->Count ));
		return False;
	}
	for ( Idx = 0 ; Idx < List->Count ; Idx ++ )
	{
		List->Objects[Idx] = Null;
	} 

	return True;
}

void objectlistDestroy ( 
	Pointer ObjectListPointer )
{
	ObjectList*	List = (ObjectList*)ObjectListPointer;
	memFree ( List->Objects );
}

Pointer objectlistGetObject (
	Pointer ObjectListPointer, 
	UInt32	Number )
{
	ObjectList*	List = (ObjectList*)ObjectListPointer;
	return List->Objects[Number] ;
}

UInt32 objectlistInsert ( 
	Pointer ObjectListPointer, 
	Pointer Object,
	UInt32	Number )
{
	ObjectList*	List = (ObjectList*)ObjectListPointer;

	if ( Number >= List->Count ) 
	{
		DPF(0,("tmman:objectlistInsert:Number[%lx]:OUT OF RANGE:FAIL\n",
			(uint32)Number ));
		return False;
	}

	if ( List->FreeCount == 0 )
	{
		DPF(0,("tmman:objectlistInsert:Number[%lx]:NO MORE FREE SLOTS:FAIL\n",
			(uint32)Number ));
		return False;
	}

	if ( List->Objects[Number] ) 
	{
		DPF(0,("tmman:objectlistInsert:Number[%lx]:INVALID:FAIL\n",
			(uint32)Number ));
		return False;
	}

	List->FreeCount--;
	List->Objects[Number] = Object;

	return True;
}


void objectlistDelete ( 
	Pointer ObjectListPointer, 
	Pointer Object,
	UInt32 Number )
{
	ObjectList*	List = (ObjectList*)ObjectListPointer;
	
	/* unused parameter */
	(void)Object;
	
	List->FreeCount++;
	List->Objects[Number] = Null;
}

/* LIST MANIPULATION ROUTINES */

void listInitializeHead( 
	ListStruct* ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

Bool listIsEmpty (
	ListStruct* ListHead )
{
    return ListHead->Flink == ListHead;
}


ListStruct* listRemoveHead (
	ListStruct* ListHead )
{
    ListStruct* Temp;
	Temp = ListHead->Flink;
    listRemoveEntry(ListHead->Flink);
	return Temp;
}

ListStruct* listRemoveTail (
	ListStruct* ListHead )
{
	ListStruct* Temp;

    Temp = ListHead->Blink;
    listRemoveEntry(ListHead->Blink);

	return Temp;
}


void listRemoveEntry(
	ListStruct* Entry)
{
    ListStruct* _EX_Blink;
    ListStruct* _EX_Flink;
    _EX_Flink = (Entry)->Flink;
    _EX_Blink = (Entry)->Blink;
    _EX_Blink->Flink = _EX_Flink;
    _EX_Flink->Blink = _EX_Blink;
}

void listInsertTail (
	ListStruct* ListHead,
	ListStruct* Entry ) 
{
    ListStruct* _EX_Blink;
    ListStruct* _EX_ListHead;

    _EX_ListHead = (ListHead);
    _EX_Blink = _EX_ListHead->Blink;
    (Entry)->Flink = _EX_ListHead;
    (Entry)->Blink = _EX_Blink;
    _EX_Blink->Flink = (Entry);
    _EX_ListHead->Blink = (Entry);
}


void listInsertHead (
	ListStruct* ListHead,
	ListStruct* Entry ) 
{
    ListStruct* _EX_Flink;
    ListStruct* _EX_ListHead;

    _EX_ListHead = (ListHead);
    _EX_Flink = _EX_ListHead->Flink;
    (Entry)->Flink = _EX_Flink;
    (Entry)->Blink = _EX_ListHead;
    _EX_Flink->Blink = (Entry);
    _EX_ListHead->Flink = (Entry);
}

