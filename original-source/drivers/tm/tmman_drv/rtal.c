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
FILE    :   RTAL.c

	HISTORY
	960510	Tilakraj Roy 	   Created
	980604  Volker Schildwach  Ported to Windows CE
	981021	Tilakraj Roy	   Changes for integrating into common source base
	990511  DTO Ported to pSOS
	
	 ABSTRACT
	 Contains support functions that are OS / Platform dependent.
	 This file should be in the platform specific directories,
	 However most platforms can use this file exactly as it is,
	 so this file is kept in the common area.
	 Platforms that require these functions to be overridden can
	 ignore this file and make a local platform specific copy in 
	 the platform specific directory.
	 
*/

/*----------------------------------------------------------------------------
SYSTEM INCLUDE FILES
----------------------------------------------------------------------------*/
/* tm1 specific includes */



/*----------------------------------------------------------------------------
DRIVER SPECIFIC INCLUDE FILES
----------------------------------------------------------------------------*/

#include <stdio.h>

#include "tmtypes.h"
#include "tmmanlib.h"
#include "tm_platform.h"




//-----------------------------------------------------------------------------
// Types and defines:
//

typedef struct tagtmmanSharedMem
{
    UInt32       PhysAddr;
    Pointer     *pVirtAddr;
    UInt32       Size;
}tmmanSharedMem;



#define MAX_MEM_AREAS           32

/*
seems to be unused
static tmmanSharedMem   gMemArray[MAX_MEM_AREAS] = {0};
*/

#define TMMAN_REGION_SIZE   (16 * 1024) // 16k reserved for TMMAN
#define TMMAN_UNIT_SIZE     (32)       // 32 bytes per allocation unit

#define SHARED_MEM_UNIT_SIZE  (0x400)
#define SHARED_MEM_SIZE       (0x18000)

#define KSEG0_MASK            (0x80000000)  // Bit indicates a kseg0 address
#define KSEG1_MASK            (0xa0000000)  // Uncached memory

//-----------------------------------------------------------------------------
// Global data and external function/data references:
//
/*
seems to be unused
static UInt32  gTMMANRegion = 0;
static UInt32  gSharedRegion = 0;
static UInt32  *pgLockedMem;

static UInt32  gFixedSize = 0;
*/


static Pointer  LocalAlloc ( UInt32 size );
static Pointer  LocalFree (Pointer ptr);



//-----------------------------------------------------------------------------
// Exported functions:
//


Pointer memAllocate( UInt32 Size )
{
	Pointer Memory;
	Memory = (Pointer)LocalAlloc (Size );
	if (Memory != Null)
    {
        memset ( Memory, 0, Size);
    }
	return Memory;
}

void	memFree( Pointer Memory )
{
	LocalFree ( Memory );
}


void	memCopy( Pointer Destination, Pointer Source, UInt32 Size )
{
	memcpy ( Destination, Source, Size );
}

void    memSet ( Pointer Memory, UInt8  Value, UInt32 Size )
{
	
    memset ( Memory, Value, Size);
}

// functions for dealing with ANSI strings.
UInt32    strCmp ( Pointer String1, Pointer String2 )
{
    UInt8  *Str1 = (UInt8 *)String1;
    UInt8  *Str2 = (UInt8 *)String2;
	
    while ( *Str1 )
    {
        if ( *Str1 == *Str2 )
		{
			Str1++;
			Str2++;
			continue;
		}
		
		return ( *Str1 - *Str2 );
    }
	return 0;
}


// DTO: This function copies the given string into memory byte-swapped.
//      This is to counteract the bye-swapping taking place over the PCI
// 
Pointer    strCopy ( Pointer Destination, Pointer Source )
{
    UInt8  *Dest = (UInt8 *)Destination;
    UInt8  *Src  = (UInt8 *)Source; 
    
    /*
    UInt32  Offset;
    UInt32  roundUp;
    UInt32  idx;
	*/
	
    // Assuming storage space is dword aligned!
    //DL??
	
    /*    roundUp = ((strLen(Source) + 3)/ 4) * 4;
    for (idx = 0; idx < roundUp; idx++)
    {
	Offset = (4*(idx/4)) + 3 - (idx % 4);
	Dest[idx] = Src[Offset];
    }
    */
	
	
    while ( (*Dest++ = *Src++) );
	
    *Dest++ = *Src++;
	
	
	
	
    DPF(0, ("strCopy(): [%s] to [%s]\n", 
        (UInt8 *)Source,
        (UInt8 *)Destination));
	
    return Dest;
}

UInt32    strLen ( Pointer String )
{
    UInt8  *Str = (UInt8 *)String;
    UInt32    Length = 0;
	
    while ( *Str++ ) Length++;
	
    return ( Length + 1 );
	
}







//-----------------------------------------------------------------------------
// FUNCTION:     MmAllocateContiguousMemory:
//
// DESCRIPTION:  This function allocates a contiguous block of memory. On the
//               first call a segment of 93k is allocated from Region 0. 
//               Subsequent requests for memory are taken out of this block.
//               The address returned points to an area in kseg1.
//               
// 
// RETURN:       Pointer:  pointer to the allocated memory
//
// NOTES:        -
//-----------------------------------------------------------------------------
//

Pointer MmAllocateContiguousMemory (
									UInt32 Size                // IN:  Size of memory requested
									)
{
	
	// return kmalloc(Size, GFP_KERNEL);
	// DL: we are going to use __get_dma_page because we want
	// the required memory to be page aligned because we want
	// to remap it also to user space
	/*
	unsigned long order;
	unsigned long k_buf;
	for (order = 0; Size > (0x1 << (order + PAGE_SHIFT)) ; order++)
    { ;
    }
	if(order > 5)
    {
		DPF(0,("rtal.c:MmAllocateContiguousMemory: maximum 128 KBytes can be allocated\n"));
		return Null;
    }
	k_buf = kmalloc(Size, GFP_KERNEL);
	*/
	
	void *addr = NULL;
	
	/* make sure size is a multiple of 4096 */
	Size += 4095;
	Size /= 4096;
	Size *= 4096;
	
	/* allocate it */
	if (
		create_area
			(
				"tm_cont_mem",				//name
				&addr,						//result
				B_ANY_ADDRESS,				//address_spec
				Size,						//size
				B_CONTIGUOUS,				//lock
				B_READ_AREA|B_WRITE_AREA	//protection
			)
		<
		B_OK
		)
	{
		DPF(0,("rtal.c:MmAllocateContiguousMemory: %ld cannot be allocated\n", Size));
		return NULL;
	}
	
	/* zeroes it */
	memset(addr, 0, Size);
	
	return addr;
}


//-----------------------------------------------------------------------------
// FUNCTION:     MmFreeContiguousMemory:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//

//DL
Bool
MmFreeContiguousMemory(
					   Pointer pBaseAddress)
{
	if (B_OK == delete_area(area_for(pBaseAddress)))
	{
		return True;
	}
	else
	{
		return False;
	}
}


//-----------------------------------------------------------------------------
// FUNCTION:     MmGetPhysicalAddress:
//
// DESCRIPTION:  This function converts the given MIPS kseg1 Virtual address 
//               into a physical address. This is done by subtracting
//               0xa0000000 from the Virtual address
//
// RETURN:       UInt32: physical address
//
// NOTES:        -
//     DL: I think bus address is a better term in this case and I also see
//     no reason why this function should be restricted to kseg1 addresses
//-----------------------------------------------------------------------------
//
/*DL UInt32
MmGetPhysicalAddress (
Pointer pBaseAddress)
{
return ((UInt32)pBaseAddress & (~KSEG1_MASK));
}
*/

UInt32
MmGetPhysicalAddress (
					  Pointer pBaseAddress)
{
	physical_entry	pe;
	
	get_memory_map(pBaseAddress, 1,&pe,1);
	
	return (UInt32)pe.address;
}

/*
Pointer PsGetCurrentProcess()
{
return (Pointer)GetCurrentProcess();
}
*/

/*
VOID KeQuerySystemTime ( PLARGE_INTEGER Time )
{
SYSTEMTIME      WinCeSystemTime;
GetSystemTime ( &WinCeSystemTime );
SystemTimeToFileTime (&WinCeSystemTime, (FILETIME*)Time);
}
*/

//-----------------------------------------------------------------------------
// FUNCTION:     LocalAlloc:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        Debug subsystem may not be initialised at this point.
//               Cannot use DPF()
//-----------------------------------------------------------------------------
//


Pointer  LocalAlloc ( UInt32 size )
{
	//return kmalloc(size, GFP_KERNEL);
	return MmAllocateContiguousMemory(size);
}

//-----------------------------------------------------------------------------
// FUNCTION:     LocalFree:
//
// DESCRIPTION:  <Function description/purpose>
//
// RETURN:       <Return data type/description>
//
// NOTES:        <Function notes>
//-----------------------------------------------------------------------------
//
/* DL: Pointer LocalFree (Pointer ptr)
{
if (rn_retseg (gTMMANRegion, ptr) == 0)
{
return NULL;
}
else
{
return ptr;
}
}
*/
Pointer LocalFree (Pointer ptr)
{
	//kfree(ptr);
	MmFreeContiguousMemory(ptr);
	return NULL;
}
