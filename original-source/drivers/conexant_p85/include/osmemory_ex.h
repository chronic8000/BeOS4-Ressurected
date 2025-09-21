/****************************************************************************************
*		                     Version Control Information
*
*	$Header:   P:/d942/octopus/archives/Include/OsMemory_ex.h_v   1.5   20 Apr 2000 14:19:30   woodrw  $
*
*****************************************************************************************/


/****************************************************************************************

File Name:			Memory.h	

File Description:	Operting specific memory functions

*****************************************************************************************/


/****************************************************************************************
*****************************************************************************************
***                                                                                   ***
***                                 Copyright (c) 2000                                ***
***                                                                                   ***
***                                Conexant Systems, Inc.                             ***
***                             Personal Computing Division                           ***
***                                                                                   ***
***                                 All Rights Reserved                               ***
***                                                                                   ***
***                                    CONFIDENTIAL                                   ***
***                                                                                   ***
***               NO DISSEMINATION OR USE WITHOUT PRIOR WRITTEN PERMISSION            ***
***                                                                                   ***
*****************************************************************************************
*****************************************************************************************/


#ifndef __MEMORY_H__
#define __MEMORY_H__

/* Define Vtoolsd constants */
#define OS_HEAPZEROINIT		0x00000001    
#define OS_HEAPZEROREINIT	0x00000002  

#define OS_HEAPSWAP			0x00000200 

#define OS_NON_PAGED		(UINT32)0
#define OS_FIXED			0x00000008

#define OS_ZEROINIT			0x00000001	
#define OS_FIXEDZERO		0x00000003		

/********************************************************************************

Function Name: OsPageCommit.

Function Description:The function commits a memory space.

Arguments: iPage - the base address of the memory space.
		   nPages - number of pages to be commited.

Return Value: BOOL.

********************************************************************************/
BOOL OsPageCommit(UINT32 iPage, UINT32 nPages, UINT32 PageData, UINT32 flags);


/********************************************************************************

Function Name: OsPageDecommit.

Function Description:The function decommits a given memory space.

Arguments: iPage - the base address of the memory space.
		   nPages - number of pages to be commited.

Return Value: BOOL.

********************************************************************************/
BOOL OsPageDecommit(UINT32 iPage, UINT32 nPages) ;


/********************************************************************************

Function Name: OsPageReserve.

Function Description:The function reserves a given memory space.

Arguments: Size - the size of memory space to br reserved.

Return Value: BOOL.

********************************************************************************/
PVOID OsPageReserve(UINT32 Size);


/********************************************************************************

Function Name: OsPageFree.

Function Description:The function decommits a given memory space.

Arguments: iPages - the base address of the memory space.
		   nPages - number of pages to be commited.


Return Value: BOOL.
********************************************************************************/
BOOL OsPageFree(PVOID	pMem);


/********************************************************************************

Function Name: OsHeapAllocate.

Function Description:The function allocates a given memory space and initializes it to zero.

Arguments: Size - the size of memory space (heap) to br allocated.

Return Value: PVOID - a pointer to the allocated memory.

********************************************************************************/
PVOID OsHeapAllocate(UINT32 Size, int flags);


/********************************************************************************

Function Name: OsHeapFree.

Function Description:The function frees a memory space.

Arguments: pMem - a pointer to the memory to be frees.

Return Value: None.

********************************************************************************/
void OsHeapFree(PVOID pMem);


PVOID OsMemAlloc (UINT32 dwSize);
void  OsMemFree  (PVOID pBuf);
BOOL  OsMemCheck (HANDLE hObject);


/********************************************************************************

Function Name: OsGetPageSize.

Function Description:The function returns the size of a page in the specific OS.

Arguments: None.

Return Value: The size of a page in the specific OS.

********************************************************************************/
UINT32	OsGetPageSize(void);


/********************************************************************************

Function Name: OsMemSet.

Function Description:The function initializes a buffer to a specified character.

Arguments: Buffer - Pointer to buffer to contain copied characters.
		   c - Character to copy into the buffer.
		   Count - Number of times to copy the character into the buffer.

Return Value: A pointer to Buffer.

********************************************************************************/
PVOID	OsMemSet(PVOID Buffer, UINT32 c, UINT32 Count);


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
PVOID	OsMemCpy(PVOID BuffDest, PVOID BuffSrc, UINT32 Count);


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
int		OsMemCmp(PVOID Buff1, PVOID Buff2, UINT32 Count);


#endif /*__MEMORY_H__*/
