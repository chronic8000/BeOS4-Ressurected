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
HISTORY
960510	TR 	Created
961019	TR	Moved the CTC 1.1 bug fix in IRQGen from IRQAck.
970521	TR	Rewrote for Generic Target TMMan
980604  VS      Ported to Windows CE
981021	Tilakraj Roy	Changes for integrating into common source base
990511  DTO     Ported to pSOS
001109          Ported to linux

*/

//-----------------------------------------------------------------------------
// Standard include files:
//DL: #include <psos.h>



#include <errno.h>



//-----------------------------------------------------------------------------
// Project include files:
//


#include "tmmanapi.h"
#include "tmmanlib.h"
#include "tm_platform.h"


//-----------------------------------------------------------------------------
// Types and defines:
//

/* Synchronization Object Abstraction Functions */
/*
typedef	struct	tagSynchronizationObject
{
UInt32	UserModeHandle;
UInt32	KernelModeHandle;	
UInt32	SynchronizationFlags;
}	SynchronizationObject;
*/

typedef	struct	tagPageTableObject
{
    void   *pBufferAddress;
    UInt32  BufferSize;
	
} tPageTableObject;



//-----------------------------------------------------------------------------
// Exported functions:
//

//-----------------------------------------------------------------------------
// FUNCTION:     syncobjCreate:
//
// DESCRIPTION:  This function returns a handle to a created Synchronization
//               object. The Semaphore should already be created by the 
//               Application so this routine simply returns the given 
//               Semaphore Id
//               
//
// RETURN:       Bool, always returns True
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

Bool	syncobjCreate ( 
					   UInt32 SynchronizationFlags,
					   UInt32 OSSynchronizationHandle,
					   UInt32 *SynchronizationHandlePointer,
					   String Name
					   )
{
	/*
	struct semaphore *sem = Null;
	sem = (struct semaphore *)kmalloc(sizeof(sem),GFP_KERNEL);
	if (!sem)
    {
		DPF(0,("osal.c: syncobjCreate: kmalloc failed.\n"));
		return False;
    } 
	*sem = MUTEX_LOCKED;
	*SynchronizationHandlePointer = (UInt32)sem;
	*/
	
	/* unused parameters */
	(void)SynchronizationFlags;
	(void)OSSynchronizationHandle;
	
	/* create semaphore */
	*SynchronizationHandlePointer = (UInt32)create_sem(0, Name);
		
	/* always return True */
	return True;
}


//-----------------------------------------------------------------------------
// FUNCTION:     syncobjSignal:
//
// DESCRIPTION:  The function is used to perform a Syncronization object signal.
//               This is achieved by releasing the given Semaphore.
//               
//               
// RETURN:       Bool:  True on success, False Otherwise
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

Bool	syncobjSignal (
					   UInt32 SynchronizationHandle )
{
	Bool return_value;
	
	DPF(0,("osal.c: syncobjSignal: up will now be called.\n"));
	
	/*
	up((struct semaphore *)SynchronizationHandle);
	*/
	
	if (B_NO_ERROR == release_sem ((sem_id) SynchronizationHandle))
	{
		return_value = True;
	}
	else
	{
		return_value = False;
	}
	
	DPF(0,("osal.c: syncobjSignal: up has returned.\n"));

	return return_value;
}

// The following function is added because it is not possible to use these 
// semaphores from user space.

Bool	syncobjWait (
					 UInt32 SynchronizationHandle )
{ 
	DPF(8,("osal.c: syncobjWait: down_interruptible will now be called.\n"));

	PRINTF("sema_id = d:%ld\n", (sem_id)SynchronizationHandle);
	PRINTF("sema_id = x:%lx\n", (sem_id)SynchronizationHandle);
		
	/*
	if( down_interruptible((struct semaphore*)SynchronizationHandle) )
	{
		DPF(0,("osal.c: syncobjWait: down_interruptible has been interrupted by a signal.\n"));
	}
	else
	{
		DPF(8,("osal.c: syncobjWait: down_interruptible has normally returned.\n"));  
	}
	*/
	
	if
		(
	   	B_INTERRUPTED
	   	==
	   	acquire_sem_etc
	   		(
	   		(sem_id)SynchronizationHandle,
	   		1,
	   		B_CAN_INTERRUPT,
	   		0
	   		)
	   	)
	{
		DPF(0,("osal.c: syncobjWait: down_interruptible has been interrupted by a signal.\n"));
	}
	else
	{
		DPF(8,("osal.c: syncobjWait: down_interruptible has normally returned.\n"));  
	}
	
	return True;
	
}




//-----------------------------------------------------------------------------
// FUNCTION:     syncobjDestroy:
//
// DESCRIPTION:  This function destroys the Synchronization object. As the 
//               Semaphore will be deleted by the Application there is nothing
//               to do here.
//
// RETURN:       Bool, always True
//
// NOTES:        -
//-----------------------------------------------------------------------------
//
Bool	syncobjDestroy ( 
						UInt32 SynchronizationHandle )
{
	/* delete the semaphore */
	delete_sem((sem_id)SynchronizationHandle);
	
	/* return always True */
	return True;	
}

//-----------------------------------------------------------------------------
// FUNCTION:     critsectCreate:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//

Bool
critsectCreate (
				UInt32* pCriticalSectionObject )
{
	/* unused arguments (linux driver has dummy implementation of this function) */
	(void)pCriticalSectionObject;
	
	/*
	struct semaphore *sem = Null;
	sem = (struct semaphore *)kmalloc(sizeof(sem),GFP_KERNEL);
	if (!sem)
    {
	DPF(0,("osal.c: critsectCreate: kmalloc failed.\n"));
	return False;
    } 
	*sem = MUTEX;
	*pCriticalSectionObject = (UInt32)sem;
    
	*/
	
	return True;
}




//-----------------------------------------------------------------------------
// FUNCTION:     critsectDestroy:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//

Bool	critsectDestroy (
						 UInt32 CriticalSectionObject )
{   
	/* unused arguments (linux driver has dummy implementation of this function) */
	(void)CriticalSectionObject;
	
	/*  kfree((struct semaphore *)CriticalSectionObject ); */
	return True;
}  



//-----------------------------------------------------------------------------
// FUNCTION:     critsectEnter:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
/* Note that the caller has to allocate storage 
sizeof (UInt32) for nested context and pass the address
of that parameter to this function.
*/


Bool
critsectEnter (
			   UInt32  CriticalSectionObject,
			   Pointer NestedContext )
{
	/* unused arguments (linux driver has dummy implementation of this function) */
	(void)CriticalSectionObject;
	(void)NestedContext;
	
/*   DPF(0,("osal.c: critsectEnter: down will now be called.\n"));
down((struct semaphore*)CriticalSectionObject);
DPF(0,("osal.c: syncobjWait: down has returned.\n"));
	*/
	return True;
    
}


//-----------------------------------------------------------------------------
// FUNCTION:     critsectLeave:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//

Bool
critsectLeave (
			   UInt32  CriticalSectionObject,
			   Pointer NestedContext )
{  
	/* unused arguments (linux driver has dummy implementation of this function) */
	(void)CriticalSectionObject;
	(void)NestedContext;

/* DPF(0,("osal.c: critsectLeave: up will now be called.\n"));
up((struct semaphore *)CriticalSectionObject);
DPF(0,("osal.c: critsectLeave: up has returned.\n"));
	*/
	return True;
}




// Page Table Routines

//-----------------------------------------------------------------------------
// FUNCTION:     pagetableGetTempBufferSize:
//
// DESCRIPTION:  This routine returns the size that a Page Table should be in 
//               order to contain an entry for all pages associated with the 
//               given buffer address & size. 
//               Under pSOS there is no concept of pages. So a page size will  
//               be taken as the size of the associated buffer. Therefore the
//               Page Table need only have one entry.
//               
//
// RETURN:       UInt32:  size in bytes of the Page Table
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

/* DL:
UInt32	pagetableGetTempBufferSize ( 
UInt32 BufferAddress, UInt32 BufferSize )
{

	return (sizeof(PageTableEntry));
	}
	
*/

//-----------------------------------------------------------------------------
// FUNCTION:     pagetableCreate:
//
// DESCRIPTION:  This function creates a page table for the given buffer 
//               address and size. Essentially this is a list of the 
//               contiguous memory regions that are mapped to the given
//               'virtual' buffer. Under pSOS there are no pages as such
//               so there is only one contiguous memory region which is
//               the buffer itself.
//               
//               
// RETURN:       Bool:  True if no error occurs
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

/* DL:

 Bool
 pagetableCreate ( 
 UInt32          pBufferAddress,// [in]: address of pre-allocated memory
 UInt32          BufferSize,    // [in]: size of buffer in bytes
 Pointer         TempBuffer,    // [out]: pre-allocated buffer for page table 
 //        entries
 UInt32	        HalHandle,     // [in]: hal handle (not used)
 PageTableEntry	**pPageTable,  // [out]: array of the page table struct
 UInt32          *pPTEntryCount,// [out]: number of contiguous regions (= 1)
 UInt32          *pPTHandle     // [out]: identifier for this page table
 )
 {
 tPageTableObject*  pPageTableObj;
 
  // Store Physical address
  ((PageTableEntry*)TempBuffer)[0].PhysicalAddress = 
  MmGetPhysicalAddress((Pointer)pBufferAddress);
  
   // Size is set to Buffer Size
   ((PageTableEntry*)TempBuffer)[0].RunLength = BufferSize;
   
    DPF(8,("Osal:pagetableCreate: Entri[0]=  %x\n", pBufferAddress));
	
	 // allocate page table object handle
	 if ((pPageTableObj = (tPageTableObject*)memAllocate(
	 sizeof(tPageTableObject))) == Null)
	 {
	 return False;
	 }
	 
	  pPageTableObj->pBufferAddress = (void *)pBufferAddress;
	  pPageTableObj->BufferSize    = BufferSize;
	  
	   // set return values:
	   *pPageTable = TempBuffer;
	   
		// Only one entry 
		*pPTEntryCount = 1;
		
		 *pPTHandle = (UInt32)pPageTableObj;
		 
		  return True;
		  }
		  */
		  
		  //-----------------------------------------------------------------------------
		  // FUNCTION:     pagetableDestroy:
		  //
// DESCRIPTION:  This function free the memory used by the Page Table Object
//
// RETURN:       Bool:  True
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

/*
Bool
pagetableDestroy ( 
UInt32  PageTableHandle // [in]: Handle of the Page Table to be freed
)
{
tPageTableObject *pPageTableObj = (tPageTableObject*)PageTableHandle;

 
  memFree (pPageTableObj);
  
   return True;
   }
   
*/


