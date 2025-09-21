/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
 *
 *  Module name              : Lib_Memspace.c    1.39
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : Memory Space Management
 *
 *  Last update              : 14:58:29 - 97/12/15
 *
 *  Description              : 
 *            
 *             This module provides for the creation of memory spaces, 
 *             allocation from specific spaces, and for the deletion of 
 *             a memory space as a whole. So no free for individually 
 *             allocated memory blocks is provided. 
 *            
 *             The result of a memory overflow is not very sophisticated:
 *             an exception (see module Lib_Exceptions) is raised, so
 *             better provide for an exception handler.
 */

/*--------------------------------- Includes: ------------------------------*/

//#include <prepc.h>

#ifndef UNDER_CE
  #include <stddef.h>
#endif
//#include <stdlib.h>
#include "Lib_Memspace.h"
#include "Lib_Local.h"
#include "Lib_Exceptions.h"
#include "Lib_StdFuncs.h"
#include "Lib_Messages.h"

/*---------------------------------- Types ---------------------------------*/
#ifdef UNDER_CE
typedef unsigned int size_t;
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif // UNDER_CE

typedef struct SuperBlock	*SuperBlock;
typedef struct TMMemoryBlock	*TMMemoryBlock;

/* Memory block structure. */
struct TMMemoryBlock {
	UInt  		size;		/* size 			*/
	UInt		offset;		/* offset to start of super block */
					/* (needed for address independent */
					/* address hash) 		*/
	TMMemoryBlock	prev;		/* previous free block (if free) */
	TMMemoryBlock	next;		/* next free block (if free)	*/
} ;

struct Lib_Memspace {
	Int			in_use;
	Int			increment;	/* default size to increment it with */
	TMMemoryBlock		free_list;	/* start of the freelist */
	SuperBlock		block_list;
};

struct SuperBlock {
	Int 			size;           /* of superblock */
	Lib_Memspace		memspace;
	SuperBlock		next;
} ;


/*---------------------------------- globals -------------------------------*/

Lib_Memspace		Lib_Memspace_default_space = Null;

static Lib_Memspace	mem_spaces;

#define NROF_MEM_SPACES	20	/* max over all the tools */

#define	DEF_KB_S		(8 * 1024)

#define SUPER_BLOCK_SIZE	(DEF_KB_S - 8)

static Lib_Memspace_MallocFunc	MallocFun = (Lib_Memspace_MallocFunc)malloc;
static Lib_Memspace_FreeFunc	FreeFun   = (Lib_Memspace_FreeFunc)free;

static Int total_allocated;


/*--------------------------------- Functions ------------------------------*/


static Pointer MallocFun_wrap( Int size )
{
    Int* result= MallocFun(size + MAX_ALIGNMENT);

    if (result) {
       *(result)= size;
        result = (Pointer) (((Int)result) + MAX_ALIGNMENT);

        memset( (Pointer) result, -1, size );
        return result;
    } else {
        return Null;
    }
}

static void FreeFun_wrap( Pointer p )
{
    Int *block= (Int*)(((Int)p)-MAX_ALIGNMENT);
    Int size= *block;

    memset( p, -1, size );

    FreeFun(block);
}



static void
init( void )
{
	Int	size;
	
	size = NROF_MEM_SPACES * sizeof(struct Lib_Memspace);
	mem_spaces = (Lib_Memspace) MallocFun_wrap( size );
        total_allocated= size;
	
        /* check for allocation failure */
	CHECK( mem_spaces != Null, (LibMsg_Memory_Overflow, size) ){}	

	memset((char *)mem_spaces, 0, size);
}

/*
this 'static' function is never used within this file
static void
free_block_list( SuperBlock block_list )
{
	SuperBlock	l, p;

	l = block_list;
	while (l != Null) {
                total_allocated-= l->size;
		
		p = l->next;
		FreeFun_wrap(l);
		l = p;
	}
}
*/
static Lib_Memspace
get_new_free_space( void )
{
	Int		i;
	
	if (mem_spaces == Null) {
		init();
	}

	for (i = 0; i < NROF_MEM_SPACES; i++ ) {
		if (!mem_spaces[i].in_use)
			return &mem_spaces[i];
	}

       /* out of mem spaces, bump NROF_MEM_SPACES */
	ASSERT(False,(LibMsg_Out_Of_Spaces));
}


static Bool	called_once = False;

extern Bool
Lib_Memspace_init( Lib_Memspace_MallocFunc malloc_fun, Lib_Memspace_FreeFunc free_fun )
{
	/*
	unused variable
	Int		i;
	*/
	
	if ( called_once ) {
		if ( malloc_fun != MallocFun || free_fun != FreeFun )
			return False;
		else	return True;
	}	
	else called_once = True;
	
	if ( MallocFun != Null )
		MallocFun = malloc_fun;
	if ( FreeFun != Null )
		FreeFun   = free_fun;
	return True;
}


extern Lib_Memspace
Lib_Memspace_new(UInt increment_size)
{
	Lib_Memspace space;

	space = get_new_free_space();
	
	space->in_use     = True;
	space->increment  = increment_size;
	space->free_list  = Null;
	space->block_list = Null;
	
	return space;
}



/*
 * Function         : Delete a previously allocated memory space,
 *                    and give all the related memory back to the
 *                    underlying memory manager.   
 * Parameters       : space  (IO)  memory space to be deleted.
 * Function result  : -        
 */

extern void
Lib_Memspace_delete( Lib_Memspace space )
{
	SuperBlock	block_list;

	block_list = space->block_list;
	while ( block_list != Null ) {
	
		SuperBlock 	h;

		h = block_list->next;
		FreeFun_wrap(block_list);
		block_list = h;
	}

	space->in_use     = False;
	space->increment  = 0;
	space->block_list = Null;
}


/*
 * set the new default meory space
 */
extern Lib_Memspace
Lib_Memspace_set_default( Lib_Memspace mem_space )
{
	Lib_Memspace	old_default;
	
	old_default                = Lib_Memspace_default_space;
	Lib_Memspace_default_space = mem_space;
	return	old_default;
}


/*--------------------------------- Functions ------------------------------*/



/* Manifest constants. */
#define	MIN_BLOCK	(sizeof(struct TMMemoryBlock))		/* minimum block size		*/
#define	OVERHEAD	(offsetof(struct TMMemoryBlock, prev))	/* block overhead (if used)	*/

#define	roundup(n, m)		(((n) + (m) - 1) & ~((m) - 1))
#define	check_alignment(n, m)	ASSERT(((Int)(n) & ((m) - 1)) == 0,(LibMsg_Alignment_Error))

#define get_memory_block(addr)	((TMMemoryBlock)((Char*)(addr) - OVERHEAD))
#define get_super_block(addr)	((SuperBlock)((Char*)(addr) - get_memory_block(addr)->offset))

static TMMemoryBlock
grow_free( Lib_Memspace space, Int size )
{
	SuperBlock	sb;
	TMMemoryBlock	free;
	Int		n;
	
	n = size + sizeof(struct SuperBlock);
	n = LMAX3(n, SUPER_BLOCK_SIZE, space->increment);
	n = roundup(n, MAX_ALIGNMENT);
	
		/* 
		 * here we assume/check malloc returns 4 byte aligned pointers 
		 */
	sb = (SuperBlock)MallocFun_wrap(n);

	CHECK( sb != Null, (LibMsg_Memory_Overflow, total_allocated) ){}	
	
        sb->size         = n;	
        total_allocated += n;

	check_alignment(sb, MAX_ALIGNMENT);
	
		/* 
		 * attach it to this mem space' free_list 
		 */
	free         = (TMMemoryBlock)((Char *)sb + sizeof(struct SuperBlock));
	free->size   = n - sizeof(struct SuperBlock);
	free->offset = sizeof(struct SuperBlock) + OVERHEAD;
	free->prev   = Null;
	free->next   = space->free_list;
	
	if (space->free_list != Null)
		space->free_list->prev = free;

	space->free_list = free;
	
		/*
		 * Attach the block to the memory space
		 */
	sb->next          = space->block_list;
	sb->memspace      = space;
	space->block_list = sb;

	return space->free_list;
}


/*
this 'static' function is never used within this file
static void check_free_lists( void ) 
{
	Lib_Memspace 	space;
	TMMemoryBlock	p;
	Int		i;
	
	for (i = 0; i < NROF_MEM_SPACES; i++ ) {
	
		space = &mem_spaces[i];
		if (!space->in_use)
			continue;
		
		p = space->free_list;
		while (p != Null) {
			ASSERT( p->size >= MIN_BLOCK, (LibMsg_Freelist_Corruption) );
			p = p->next;
		} 
	}
}
*/

/*
 * Function         : Attempt allocation of memory
 * Parameters       : space    (I)   space to allocate from
 *                    size     (I)   size in bytes of requested
 *                                   memory block
 * Function result  : Address of requested block 
 * Sideeffects      : In case no memory could be allocated, an exception
 *                    is generated, and this function does not return.
 */

extern Pointer
Lib_Memspace_malloc( Lib_Memspace space, Int size )
{
	TMMemoryBlock	p, q;
	Int		m, n;
	Pointer		result;

	if (space == Null) {
		CHECK(Lib_Memspace_default_space == Null, (LibMsg_No_Memspace)){} 
		Lib_Memspace_default_space = Lib_Memspace_new(DEF_KB_S);
		space = Lib_Memspace_default_space;
	}

	if ( size <= 0 )
		return Null;

       /* size computation; keep consistent with the one in realloc */
	n = LMAX(size + OVERHEAD, MIN_BLOCK);
	n = roundup(n, MAX_ALIGNMENT);

	p = space->free_list;
	while (p != Null) {
		if (p->size >= (unsigned)n)
			break;		/* block p is large enough */
		p = p->next;
	} 

	if ( p == Null )
		p = grow_free(space, n);
	
	m = p->size;
	
	if ((unsigned) m - n >= MIN_BLOCK ) {
		
		q = p;
		p = (TMMemoryBlock)((Char *)q + (m - n));
		p->size     = n;
		p->offset   = q->offset + (m - n);
		q->size    -= n;
	}
	else {
		if ( p == space->free_list )
			space->free_list = space->free_list->next;
		if ( p->prev )
			p->prev->next = p->next;
		if ( p->next )
			p->next->prev = p->prev;
	}
	
	result = (Pointer)((Char *)p + OVERHEAD);

        memset(result,-1,size);

	return result;
}



/*
 * release memory
 */

extern void
Lib_Memspace_free( Pointer addr )
{
	TMMemoryBlock	p, free_list;
	SuperBlock	q;

	if (addr == Null)
		return;
		
	p = get_memory_block(addr);
	q = get_super_block(addr);
	
        memset(addr, -1, p->size-OVERHEAD);

	if (p->size < 20) {
		/* PATCH Do not grow the freelist too much */
		p->size = 0xffffffffUL;
		p->offset = 0xffffffffUL;
		return;
	}
	ASSERT( q->memspace->in_use == True, (LibMsg_Memspace_In_Use) );
	free_list = q->memspace->free_list;
	
	if ( free_list )
		free_list->prev = p;
	p->prev = Null;
	p->next = free_list;
	q->memspace->free_list = p;
}


extern Pointer
Lib_Memspace_realloc( Pointer address, Int size )
{
	TMMemoryBlock	p;
	SuperBlock	q;
	Pointer		result;
	Int		old_size, new_size;
	
	p = get_memory_block(address);
	q = get_super_block(address);

	old_size = p->size;
	
       /* size computation; keep consistent with the one in malloc */
	new_size = LMAX(size + OVERHEAD, MIN_BLOCK);
	new_size = roundup(new_size, MAX_ALIGNMENT);

	if ( new_size <= old_size )
		return address;

	result = Lib_Memspace_malloc( q->memspace, size );

	if ( result == Null )
		return result;
	
	memcpy( (Char *)result, (Char *)address, old_size - OVERHEAD );
	Lib_Memspace_free( address );	
	return result;
}


extern Pointer
Lib_Memspace_calloc( Lib_Memspace space, UInt nelms, UInt size )
{
	Pointer result;

	result = Lib_Memspace_malloc(space, nelms * size);
	if ( result == NULL )
		return NULL;
	
	(void)memset((Char*)result, 0, nelms * size);
	return result;
}


/*
 * address hash
 */

extern Int
Lib_Memspace_address_hash( Pointer addr )
{
        if (addr == Null) {
            return 0;
        } else {
	    Int	a = get_memory_block(addr)->offset;
	    return _Lib_StdFuncs_int_hash(a);
        }
}
