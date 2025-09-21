#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <stdarg.h>
#include <malloc.h>
#include <ByteOrder.h>

#include "net_data.h"

// Number of data buffers in each block
#define BUF_PER_BLOCK	32	// Each block is a uint32

// Number of blocks for each pool
#define LARGE_BLOCKS	8	// 8 * 32 = 256 ( 512 KB total )

// The size of the buffers expressed in powers of two
#define LARGE_SIZE		11 	// 2^11 = 2048

// The size of the buffers expressed in bytes
#define LARGE_BSIZE		2048


struct mem_pool
{
	area_id dataAreaID;
	void *areaBase;
	void *lBase, *lEnd;
	
	uint32 lBits[LARGE_BLOCKS]; 			// Use bits for buffers
};

static struct mem_pool mp;

static void *mp_allocate( void *base, uint32 *useBits, int32 bitBlocks, size_t size );
static void mp_free( void *base, void *data, uint32 *useBits, size_t size );

status_t init_mempool( void )
{
	size_t size;
	
	size = (LARGE_BLOCKS * LARGE_BSIZE * BUF_PER_BLOCK);
	if( (mp.dataAreaID = create_area( "net_mem_pool", &mp.areaBase, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA )) < 0 )
		return mp.dataAreaID;
	mp.lBase = mp.areaBase;
	mp.lEnd = mp.lBase + (LARGE_BLOCKS * LARGE_BSIZE * BUF_PER_BLOCK);
	
	// All the bits are contiguous, so we should be able to get away with this
	memset( mp.lBits, 0, (LARGE_BLOCKS)*4 ); // Clear all usage bits
	return B_OK;
}

void uninit_mempool( void )
{
	delete_area( mp.dataAreaID );
}

void *new_buf( size_t size )
{
	if( size <= LARGE_BSIZE )
		return mp_allocate( mp.lBase, mp.lBits, LARGE_BLOCKS, LARGE_SIZE );
	else
		return NULL;
}

void delete_buf( void *buffer )
{
	if( (buffer >= mp.lBase)&&(buffer < mp.lEnd) )
		mp_free( mp.lBase, buffer, mp.lBits, LARGE_SIZE );
}

static void *mp_allocate( void *base, uint32 *useBits, int32 bitBlocks, size_t size )
{
	int32 		i, j;
	uint32		bitMask;
	void 		*data;
	
	// Look for free bits using 32 bit compare
	for( i=0; i<bitBlocks; i++ )
	{
		if( useBits[i] != 0xFFFFFFFF )
		{
			// Find a free bit in the 32 bit word
			for( j=0; j<32; j++ )
			{
				bitMask = (1L << j);
				if( !(useBits[i] & bitMask) ) // Speculative check for free bit
				{
					if( atomic_or( useBits + i, bitMask ) & bitMask ) // Try to get this block
						continue; // Someone managed to get it before us!
					data = (void *)(((char *)base) + (((i<<5)+j) << size)); // ((i*8*4)+j)*(2^size)
					return data;
				}
			}
		}
	}
	return NULL;
}

static void mp_free( void *base, void *data, uint32 *useBits, size_t size )
{
	int32 hash = (data - base) >> size;
	
	//if( !(useBits[hash>>5] & (1<<(hash&0x1F))) )
	//	kernel_debugger( "ss: Attempt to free unallocated block!\n" );
	useBits[hash>>5] &= ~(1<<(hash&0x1F)); // Clear use bit
}
