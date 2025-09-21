//******************************************************************************
//
//	File:		alloc.h
//
//	Description:	fast memory allocator interfaces
//	
//	Written by:	Original code by Steve Hill. Mods by Eric Knight
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	8/27/92		ehk	new today. see .c for mods
//
//******************************************************************************/

#ifndef ALLOC_H
#define ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/
//
// AllocInit()    - create an alloc pool, returns the new pool handle.
// Alloc()        - allocate memory from the passed pool.
// AllocReset()   - reset the passed pool.
// AllocFree()    - free the memory used by the passed pool.
//
/*------------------------------------------------------------------*/


// alloc_hdr_t - Header for each block of memory
typedef
struct alloc_hdr_s
{
        struct alloc_hdr_s *next;   /* Next Block          */
        char               *block,  /* Start of block      */
                           *free,   /* Next free in block  */
                           *end;    /* block + block size  */
}
alloc_hdr_t;

// alloc_root_t - Header for the whole pool
typedef
struct alloc_root_s
{
        alloc_hdr_t *first,   	/* First header in pool  */
                    *current; 	/* Current header        */
        long	    block_size;	/* This pool's block size */
}
alloc_root_t;

/*------------------------------------------------------------------*/

alloc_root_t* 	AllocInit(long);
char*		Alloc(long, alloc_root_t*);
void		AllocReset(alloc_root_t*);
void		AllocFreePool(alloc_root_t*);

/*------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
