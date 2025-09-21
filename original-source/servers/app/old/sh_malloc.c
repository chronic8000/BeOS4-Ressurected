/*
	malloc.c
	Copyright (C) 1991 Be Labs, Inc.  All Rights Reserved
	A memory allocator.  Almost verbatim from  "The C Programming
	Language", Brian W. Kernighan and Dennis M. Ritchie, Prentice Hall, Inc.
	pages 175-6.

	Modification History (most recent first):
	25 jan 93	bgs	modified for server shared area
	11 jan 93	elr	added a realloc
	08 jan 93	elr	copied to belib for app use & reentrancy
	02 apr 92	rwh	added semaphore protection
	29 may 91	rwh	new today

+++++ */

#include <AppDefs.h>
#include <stdlib.h>
#include <OS.h>
#include <string.h>

#define _MALLOC_INTERNAL
#include <malloc.h>
#include <priv_syscalls.h>

#undef DEBUG
#define	DEBUG	0
#include <Debug.h>
/*------------------------------------------------------------*/

#define dprintf

/*------------------------------------------------------------*/

static char	   *shared_area = (char *)0;
static area_id  shared_aid;
static long	    shared_area_size = 0;

static char *
sh_sbrk(long size)
{
	char	*new_pointer;

	dprintf("sh_brk of %ld\n", size);
	if (shared_area == 0) {
		shared_area = (char *)0x70000000;
		
		shared_aid = create_area(	"shared heap",
						&shared_area,
						B_EXACT_ADDRESS,
						size,
						B_NO_LOCK,
						B_READ_AREA | B_WRITE_AREA);
		if (shared_aid < 0)
			return((char*)-1);
		shared_area_size = size;
		return(shared_area);
	}

	new_pointer = shared_area + shared_area_size;
	shared_area_size += size; 
	if (resize_area(shared_aid, shared_area_size) < B_NO_ERROR)
		return((char*)-1);
	return(new_pointer);
}


static void *
sh_morecore(long increment, malloc_state *ms)
{
	void *result = (void *) sh_sbrk((int) increment);

	if (result == (void *) -1)
		return NULL;

	return result;
}

static malloc_state sh_ms = { -1, };
static malloc_funcs sh_mf = { _kfree_memory_, sh_morecore, };

void
init_sh_malloc_g(void)
{
	sh_ms.malloc_sem  = create_sem(0, "sh_malloc_g");
	sh_ms.malloc_lock = 0;

	sh_mf.morecore    = sh_morecore;
}

void *
sh_malloc_g(unsigned nbytes)
{
	return _malloc(nbytes, &sh_ms, &sh_mf);
}


void
sh_free_g(void *ptr)
{
	_free(ptr, &sh_ms, &sh_mf);
}


void *
sh_realloc(void *p, long size)
{
	return _realloc(p, size, &sh_ms, &sh_mf);
}	
