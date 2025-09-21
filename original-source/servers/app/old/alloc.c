//******************************************************************************
//
//	File:		alloc.c
//
//	Description:	fast memory allocator implementation
//	
//	Written by:	Original code by Steve Hill. Mods by Eric Knight
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	8/27/92		ehk	new today
//	8/27/92		ehk	replace root global with passed in parameters
//				so we can multiprocess
//	8/28/92		ehk	let caller define block size for his pool
//
//******************************************************************************/

#include <stdio.h>

#ifndef ALLOC_H
#include "alloc.h"
#endif


/*------------------------------------------------------------------*/

/* AllocHdr()
 *
 * Private routine to allocate a header and memory block.
 */

static
alloc_hdr_t *
AllocHdr(block_size)
long	block_size;
{
        alloc_hdr_t        *hdr;
        char               *block;

        block = (char *) gr_malloc(block_size);
        hdr   = (alloc_hdr_t *) gr_malloc(sizeof(alloc_hdr_t));

        if (hdr == NULL || block == NULL) {
                printf("Out of memory\n");
                exit(1);
        }
        
        hdr->block = block;
        hdr->free  = block;
        hdr->next  = NULL;
        hdr->end   = block + block_size;

        return(hdr);
}

/* AllocInit()
 *
 * Create a new memory pool with one block of the passed block size.
 * Returns pointer to the new pool.
 */

alloc_root_t*
AllocInit(block_size)
long	block_size;
{
        alloc_root_t*		root;

        root = (alloc_root_t *) gr_malloc(sizeof(alloc_root_t));
        root->first = AllocHdr(block_size);
        root->current = root->first;
        root->block_size = block_size;
        return(root);
}

/* Alloc()
 *
 * Use as a direct replacement for malloc().  Allocates memory
 * from the passed pool.
 */

char *
Alloc(size, root)
long        	size;
alloc_root_t*	root;
{
        alloc_hdr_t        *hdr = root->current;
        char               *ptr;

        /* Align to 4 byte boundary */
        // lose this as long as we always alloc 4 bytes multiples
        // size = (size + 3) & 0xfffffffc;

        ptr = hdr->free;
        hdr->free += size;

        /* Check if the current block is exhausted */

        if (hdr->free >= hdr->end) {
                /* Is the next block already allocated? */

                if (hdr->next != NULL) {
                        /* re-use block */
                        hdr->next->free = hdr->next->block;
                        root->current = hdr->next;
                }
                else {
                        /* extend the pool with a new block */
                        root->current = hdr->next = AllocHdr(root->block_size);
                }

                /* set ptr to the first location in the next block */
                ptr = root->current->free;
                root->current->free += size;
        }
        /* Return pointer to allocated memory */
        return(ptr);
}

/* AllocReset()
 *
 * Reset the passed pool for re-use.  No memory is freed, so
 * this is very fast.
 */

void
AllocReset(root)
alloc_root_t*	root;
{
        root->current = root->first;
        root->current->free = root->current->block;
}

/* AllocFreePool()
 *
 * Free the memory used by the current pool.
 * Don't use where AllocReset() could be used.
 */

void
AllocFreePool(alloc_root_t *root)
{
		alloc_hdr_t *tmpHdr;
        alloc_hdr_t *hdr = root->first;

        while (hdr != NULL) {
                gr_free((char *) hdr->block);
        		tmpHdr = hdr;
                hdr = hdr->next;
               	gr_free((char *) tmpHdr);
        }
        gr_free((char *) root);
}


