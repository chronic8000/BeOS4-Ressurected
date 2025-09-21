	/*************************************************************************

	File Name: Memory.c

	File Description:

	*************************************************************************/
#include "typedefs.h"
#include "ostypedefs.h"
#include "osmemory_ex.h"

extern BOOL interrupts_enabled(void); 

void		*malloc( );
PVOID OsMemAlloc (UINT32 dwSize)
{
	UINT32 address;
	if (interrupts_enabled)
	{
		address = (UINT32) malloc(dwSize);
//dprintf("Malloc %08x bytes starting at %0x8\n", dwSize, address);
		memset((void *)address, 0, dwSize);
		return ((void *)address);
	}
	dprintf("ARGH! - OsMemAlloc() called while interrupts disabled!\n");
	return NULL;
}

 void  OsMemFree  (PVOID pBuf)
 {
 // a memory leak is better then a crash?
 	if (interrupts_enabled)
	{
//dprintf("freeing %08x\n", pBuf);
	 	free(pBuf);
	}
	else
		dprintf("ARGH! - OsMemFree() called while interrupts disabled!\n");
 }

#ifdef full_memory_manager
	BOOL OsPageCommit(UINT32 iPage, UINT32 nPages,UINT32	 PageData, UINT32 flags)
	{}
	BOOL OsPageDecommit(UINT32 iPage, UINT32 nPages) 
	{}
	PVOID OsPageReserve(UINT32 nPages)
	{}
	BOOL OsPageFree(PVOID	pMem)
	{}
	PVOID OsHeapAllocate(UINT32 Size, int flags)
	{}
	void OsHeapFree(PVOID pMem)
	{}
	UINT32	OsGetPageSize(void)
	{}
#endif

#ifdef WINDOWS
	/********************************************************************************
	
	Function Name: OsPageCommit.

	Function Description:The function commits a memory space.

	Arguments: iPages - the base address of the memory space.
			   nPages - number of pages to be commited.
	
	Return Value: BOOL.

	********************************************************************************/
	BOOL OsPageCommit(UINT32 iPage, UINT32 nPages,UINT32	 PageData, UINT32 flags)
	{
		return _PageCommit(iPage, nPages, PageData, 0, flags | PC_USER);
	}

	/********************************************************************************
	
	Function Name: OsPageDecommit.

	Function Description:The function decommits a given memory space.

	Arguments: iPages - the base address of the memory space.
			   nPages - number of pages to be commited.
	
	Return Value: BOOL.

	********************************************************************************/
	BOOL OsPageDecommit(UINT32 iPage, UINT32 nPages) 
	{
		return _PageDecommit(iPage, nPages, 0);
	}

	/********************************************************************************
	
	Function Name: OsPageReserve.

	Function Description:The function decommits a given memory space.

	Arguments: iPages - the base address of the memory space.
			   nPages - number of pages to be commited.
	

	Return Value: a pointer to the reserved memory space.

	********************************************************************************/
	PVOID OsPageReserve(UINT32 nPages)
	{
		return (PVOID)_PageReserve(PR_SYSTEM, nPages, PR_FIXED);
	}

	/********************************************************************************
	
	Function Name: OsPageFree.

	Function Description:The function decommits a given memory space.

	Arguments: iPages - the base address of the memory space.
			   nPages - number of pages to be commited.
	

	Return Value: BOOL

	********************************************************************************/
	BOOL OsPageFree(PVOID	pMem)
	{
		return _PageFree(pMem,0);
	}

	/********************************************************************************
	
	Function Name: OsHeapAllocate.

	Function Description:The function allocates a given memory space and initializes it to zero.

	Arguments: Size - the size of memory space (heap) to br allocated.
	
	Return Value: a pointer to the allocated memory..

	********************************************************************************/
	PVOID OsHeapAllocate(UINT32 Size, int flags)
	{
		return _HeapAllocate(Size,flags | HEAPZEROINIT);
	}

	/********************************************************************************
	
	Function Name: OsHeapFree.

	Function Description:The function frees a memory space.

	Arguments: pMem - a pointer to the memory to be frees.
	
	Return Value: None.

	********************************************************************************/
	void OsHeapFree(PVOID pMem)
	{
		_HeapFree(pMem,0);
	}

	/********************************************************************************
	
	Function Name: OsGetPageSize.

	Function Description:The function returns the size of a page in the specific OS.

	Arguments: None.
	
	Return Value: The size of a page in the specific OS.

	********************************************************************************/
	UINT32	OsGetPageSize(void)
	{
		return 1<<12;
	}
#endif	
	/********************************************************************************
	
	Function Name: OsMemSet.

	Function Description:The function initializes a buffer to a specified character.

	Arguments: Buffer - Pointer to buffer to contain copied characters.
			   c - Character to copy into the buffer.
			   Count - Number of times to copy the character into the buffer.
	
	Return Value: A pointer to Buffer.

	********************************************************************************/
	PVOID	OsMemSet(PVOID Buffer, UINT32 c, UINT32 Count)
	{
		return memset(Buffer, c, Count);
	}

	/********************************************************************************
	
	Function Name: OsMemCpy.

	Function Description:The function copies a specified number of characters from one 
						 buffer to another. This function may not work correctly if 
						 the buffers overlap. 

	Arguments: BuffDest - Pointer to buffer to contain copied characters.
			   BuffSrc - Pointer to buffer with the characters to copy.
			   Count - Number of characters to copy.
	
	Return Value: A pointer to BuffDest.

	********************************************************************************/
	PVOID	OsMemCpy(PVOID BuffDest, PVOID BuffSrc, UINT32 Count)
	{
		if(!BuffDest)
			dprintf("OsMemCpy() null Destination pointer\n");
		if(!BuffSrc)
			dprintf("OsMemCpy() null Source pointer\n");
		else
			return memcpy(BuffDest, BuffSrc, Count);
	}

	/********************************************************************************
	
	Function Name: OsMemCmp.

	Function Description:The function compares a specified number of characters from two buffers.

	Arguments: Buff1 - Pointer to first buffer to compare.
			   Buff2 - Pointer to second buffer to compare.
			   Count - Number of characters to compare.
	
	Return Value: Returns one of three values as follows:
														0 if buffers match.
														< 0 if buff1 is less than buff2.
														>0 if buff1 is greater than buff2.

	********************************************************************************/
	int		OsMemCmp(PVOID Buff1, PVOID Buff2, UINT32 Count)
	{
		return memcmp(Buff1, Buff2, Count);
	}

