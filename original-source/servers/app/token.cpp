//******************************************************************************
//
//      File:           token.cpp
//
//      Description:    token space class.
//                      Implements token managment in the window server.
//
//      Written by:     Benoit Schillings
//
//      Copyright 1992, Be Incorporated
//
//      Change History:
//	1/20/94		tdl	Support revNumber and portId data members for scripting usage.
//				Added new_token(long a_token, short type, void** data, long portId );
//				Added get_token(long a_token, void** data, long* portId );
//				Added set_token_port(long a_token, long portId );
//      5/22/92         bgs     new today
//
//******************************************************************************
 
#include <stdio.h>
#include <stdlib.h>
#include <Debug.h>

#ifndef _TOKEN_H
#include "./token.h"
#endif
#include "font_cache.h"
#include "as_debug.h"
#include "as_support.h"

TokenSpace *tokens = NULL;
TokenSpace bid;

extern "C" void xprintf(char *format, ...);
 
/*-------------------------------------------------------------*/
 
void    TokenSpace::dump_tokens()
{
        long            i;
        token_array     *tmp_ta;
 		
		return;
        printf("first free0 = %ld\n", first_free);
        for (i = 0; i < level_1_size; i++) {
                if (level_1[i]) {
                        tmp_ta = level_1[i];
                        printf("level 1 [%ld] = %lx, free = %ld, count = %ld\n", i, (uint32)tmp_ta, tmp_ta->first_free, tmp_ta->use_count);
                }
        }
}
 
/*-------------------------------------------------------------*/
extern "C" void zap_bitmap_hack(void *p);
extern "C" void zap_window_hack(void *p);
/*-------------------------------------------------------------*/

void	TokenSpace::get_token_by_type(int *cookie, short type, long owner, void **data)
{
	int             i, i0, j, j0;
	token_array     *tmp_ta;

	i0 = cookie[0];
	j0 = cookie[1];
	for (i = i0; i < level_1_size; i++) {
		if (level_1[i]) {
			tmp_ta = level_1[i];
			for (j = j0; j < level_2_size; j++)
				if ((tmp_ta->array[j].type == type) &&
					((tmp_ta->array[j].owner == owner) ||
					 ((tmp_ta->array[j].owner != NO_USER) && (owner == ANY_USER)))) {
					cookie[0] = i;
					cookie[1] = j+1;
					*data = tmp_ta->array[j].data;
					return;
				}
		}
		j0 = 0;
	}
	*data = 0L;
}

/*-------------------------------------------------------------*/

void	TokenSpace::iterate_tokens(int32 owner, short type, token_iterator tokiter, void *userData)
{
	int32 i,j,tok=0;
	uint32 r;
	token_array *tmp_ta;
 
	for (i = 0; i < level_1_size; i++) {
		if (level_1[i]) {
			tmp_ta = level_1[i];
			for (j = 0; j < level_2_size; j++) {
				if (((owner == ANY_USER) && (tmp_ta->array[j].owner != NO_USER)) ||
					(tmp_ta->array[j].owner == owner)) {
					if ((type == TT_ANY) || (tmp_ta->array[j].type == type)) {
						r = tokiter(tok,tmp_ta->array[j].owner,
									tmp_ta->array[j].type,
									tmp_ta->array[j].data,
									userData);
						if (r & TOK_ITER_REMOVE) {
							tmp_ta->array[j].owner = NO_USER;
							tmp_ta->array[j].type = 0;
						};
						if (r & TOK_ITER_STOP) return;
					};
				}
				tok++;
        	}
		}
	}
};

/*-------------------------------------------------------------*/

void	TokenSpace::cleanup_dead(long target_owner)
{
	long            i;
	long			j;
	token_array     *tmp_ta;
	void *			data;
 
	for (i = 0; i < level_1_size; i++) {
		if (level_1[i]) {
			for (j = 0; (tmp_ta = level_1[i]) && (j < level_2_size); j++) {
				if (tmp_ta->array[j].owner == target_owner) {
					data = tmp_ta->array[j].data;
					if (tmp_ta->array[j].type == (TT_WINDOW|TT_OFFSCREEN|TT_ATOM)) {
						remove_token((i*level_2_size + j) | tmp_ta->array[j].revNumber);
						zap_window_hack(data);
					} else if (tmp_ta->array[j].type == TT_FONT_CACHE) {
						remove_token((i*level_2_size + j) | tmp_ta->array[j].revNumber);
						fc_release_app_settings(data);
					} else if (tmp_ta->array[j].type & TT_ATOM) {
						remove_token((i*level_2_size + j) | tmp_ta->array[j].revNumber);
						((SAtom*)data)->ClientUnref();
					}
				}
        	}
		}
	}
}

/*-------------------------------------------------------------*/
 
TokenSpace::TokenSpace() : atomLock("atomLock")
{
	int32 i;

	first_free = -1;

	for (i = 0; i < level_1_size; i++)
		level_1[i] = (token_array *)0;
}
 
/*-------------------------------------------------------------*/
 
TokenSpace::~TokenSpace()
{
}
 
/*-------------------------------------------------------------*/
 
void    TokenSpace::new_token_array(long level_1_index)
{
        token_array     *tmp_ta;
        long            i;
 
        tmp_ta = (token_array *)grMalloc(sizeof(token_array),"taken array",MALLOC_CANNOT_FAIL);
 
        for (i = 0; i < level_2_size; i++) {
                tmp_ta->array[i].owner = NO_USER;
        }
 
        tmp_ta->first_free = 0;
        tmp_ta->use_count  = 0;
 
        if ((first_free < 0) || (first_free > level_1_index))
                first_free = level_1_index;
 
        level_1[level_1_index] = tmp_ta;
}
 
/*-------------------------------------------------------------*/

long    TokenSpace::new_token(long owner, short type, void *pointer)
{
        token_array     *tmp_ta;
        long            error;
        long            level_1_index;
        long            level_2_index;
        token_type	*tmp_token_ptr;
 
		tokenLock.Lock();
 
        error = find_free_entry(&level_1_index, &level_2_index);
 
        if (error < 0) {
				tokenLock.Unlock();
                printf("the token space is full. I will crash now\n");
                debugger (NULL);
        }
 
        tmp_ta = level_1[level_1_index];
        tmp_ta->use_count++;
 
        tmp_token_ptr = &(tmp_ta->array[level_2_index]);
 
        tmp_token_ptr->owner = owner;
        tmp_token_ptr->type  = type;
        tmp_token_ptr->data  = pointer;
        tmp_token_ptr->portId = NO_PORT;
        tmp_token_ptr->revNumber = (tmp_token_ptr->revNumber + REV_NUMBER_INC) &
        	REV_NUMBER_MASK;
 
		tokenLock.Unlock();
        return((level_1_index * level_2_size) + level_2_index + tmp_token_ptr->revNumber);
}

/*-------------------------------------------------------------*/
 
long
TokenSpace::get_token(long a_token, void **data)
{
	long			tokenRev = a_token & REV_NUMBER_MASK;
    token_array     *tmp_ta;
    token_type      *tmp_token_ptr;
 
 	if (a_token == NO_TOKEN)
		return -1;

 	a_token &= ~REV_NUMBER_MASK;
        if (a_token > MAX_TOKEN_SPACE)
                return(-1);
		if (a_token < 0)
				return(-1);
 
// Note that the /, *, and % are replaced by shift and & by the compiler
// due to the choice of level_2_size
 
        tmp_ta = level_1[a_token / level_2_size];
        if (tmp_ta == 0)
		return -1;
 
        tmp_token_ptr = &(tmp_ta->array[a_token % level_2_size]);
        
 	if (tmp_token_ptr->revNumber != tokenRev ||
 	    tmp_token_ptr->owner == NO_USER)
		return -1;
 
        if (data)
                *data = tmp_token_ptr->data;
        return(0);
}
 
/*-------------------------------------------------------------*/
 
long
TokenSpace::get_token(long a_token, short *type, void **data)
{
	long			tokenRev = a_token & REV_NUMBER_MASK;
    token_array     *tmp_ta;
    token_type      *tmp_token_ptr;
 
 	if (a_token == NO_TOKEN)
		 return -1;

 	a_token &= ~REV_NUMBER_MASK;
    if (a_token > MAX_TOKEN_SPACE)
                return(-1);
	if (a_token < 0)
				return(-1);
 
// Note that the /, *, and % are replaced by shift and & by the compiler
// due to the choice of level_2_size
 
        tmp_ta = level_1[a_token / level_2_size];
        if (tmp_ta == 0) return -1;
 
        tmp_token_ptr = &(tmp_ta->array[a_token % level_2_size]);

 	if (tmp_token_ptr->revNumber != tokenRev ||
 	    tmp_token_ptr->owner == NO_USER)
		return -1;
 
        if (type)
                *type  = tmp_token_ptr->type;
        if (data)
                *data  = tmp_token_ptr->data;
        return(0);
}

/*-------------------------------------------------------------*/
 
long
TokenSpace::set_token_type(long a_token, short type)
{
	long		tokenRev = a_token & REV_NUMBER_MASK;
	token_array	*tmp_ta;
	token_type	*tmp_token_ptr;
 
 	if (a_token == NO_TOKEN)
		return -1;

 	a_token &= ~REV_NUMBER_MASK;
	if (a_token > MAX_TOKEN_SPACE)
		return(-1);
 
// Note that the /, *, and % are replaced by shift and & by the compiler
// due to the choice of level_2_size
 
	tmp_ta = level_1[a_token / level_2_size];
	if (tmp_ta == 0)
		return -1;
 
	tmp_token_ptr = &(tmp_ta->array[a_token % level_2_size]);
        
	if (tmp_token_ptr->revNumber != tokenRev ||
	tmp_token_ptr->owner == NO_USER )
		return -1;

	tmp_token_ptr->type = type;
	return(0);
}

#if 0
/*-------------------------------------------------------------*/
 
long
TokenSpace::set_token_port(long a_token, long newPortId)
{
	long		tokenRev = a_token & REV_NUMBER_MASK;
        token_array     *tmp_ta;
        token_type      *tmp_token_ptr;
 
 	if (a_token == NO_TOKEN)
		return -1;

 	a_token &= ~REV_NUMBER_MASK;
        if (a_token > MAX_TOKEN_SPACE)
                return(-1);
 
// Note that the /, *, and % are replaced by shift and & by the compiler
// due to the choice of level_2_size
 
        tmp_ta = level_1[a_token / level_2_size];
        if (tmp_ta == 0)
		return -1;
 
        tmp_token_ptr = &(tmp_ta->array[a_token % level_2_size]);
        
 	if (tmp_token_ptr->revNumber != tokenRev ||
 	    tmp_token_ptr->owner == NO_USER ) return -1;
 
        tmp_token_ptr->portId = newPortId;
        return(0);
}
#endif

/*-------------------------------------------------------------*/
 
long    TokenSpace::find_free_entry(long *level_1_index, long *level_2_index)
{
        long            level_1_scan;
        token_array     *tmp_ta;
 
// If first free is = -1 then there is no token array with a free entry
// In this case, we need to allocate a new token array if possible.
 
        if (first_free == -1) {
                level_1_scan = -1;
 
                do {
                        level_1_scan++;
 
// case of all the level_1 full -> token space is full
 
                        if (level_1_scan == level_1_size)
                                return(-1);
 
                } while (level_1[level_1_scan] != 0);
 
                new_token_array(level_1_scan);
                first_free = level_1_scan;
        }
 
// now we are certain that first_free is valid
// Also note that there is a assumption that first_free is pointing
// toward an token_array that still has some free space.
// This can be done thanks to the call to adjust_free done at the
// end of this routine.
 
 
        tmp_ta = level_1[first_free];
 
// This test is for safety during testing.
// It should be removed when things are ok.
 
        if (tmp_ta->first_free < 0) {
                printf("this should never append !!!!\n");
                debugger (NULL);
        }
 
        *level_1_index = first_free;
        *level_2_index = tmp_ta->first_free;
 
        adjust_free(first_free, tmp_ta->first_free);
        return(0);
}
 
/*-------------------------------------------------------------*/
 
void    TokenSpace::adjust_free(long old_level_1_free, long old_level_2_free)
{
        long            level_2_scan;
        long            level_1_scan;
        token_array     *tmp_ta;
 
// first check if there is another entry free in the current level_2
 
        if (old_level_1_free >= 0) {
                level_2_scan = old_level_2_free + 1;
 
                tmp_ta = level_1[old_level_1_free];
 
                while(level_2_scan < level_2_size) {
                        if (tmp_ta->array[level_2_scan].owner == NO_USER) {
                                tmp_ta->first_free = level_2_scan;
                                return;
                        }
                        level_2_scan++;
                }
 
// mark the first free of this token array as no free space avail.
 
                tmp_ta->first_free = -1;
        }
 
// seems clear that this token_array is full, so we need to check for
// the next one etc...
// If the end of the level_1 is reached, then the first free of the
// level 1 is set to -1.
 
        level_1_scan = old_level_1_free + 1;
 
        while(level_1_scan < level_1_size) {
                tmp_ta = level_1[level_1_scan];
                if (tmp_ta) {
                        if (tmp_ta->first_free != -1) {
                                first_free = level_1_scan;
                                return;
                        }
                }
                level_1_scan++;
        }
 
        first_free = -1;
}
 
/*-------------------------------------------------------------*/
// Note that remove_token is not desallocating the ptr but
// only updating the token space.
 
void    TokenSpace::remove_token(long a_token)
{
	long			tokenRev = a_token & REV_NUMBER_MASK;
	long			level_1_index;
	long			level_2_index;
	token_array *	tmp_ta;

 	if (a_token == NO_TOKEN)
 		return;

 	a_token &= ~REV_NUMBER_MASK;
 	a_token &= 0x7FFFFFFF; // Don't use the sign bit
	if (a_token > MAX_TOKEN_SPACE)
		return;
 
        level_1_index = a_token / level_2_size;
        level_2_index = a_token % level_2_size;

		tokenLock.Lock(); 
        tmp_ta = level_1[level_1_index];
 
        if (tmp_ta == 0) {
//			This is actually fine...
//			printf("Bad token remove %ld\n", a_token);
//			debugger(NULL);
			tokenLock.Unlock();
			return;
        }
 
        if (tmp_ta->array[level_2_index].owner == NO_USER ||
            tmp_ta->array[level_2_index].revNumber != tokenRev) {
	        tokenLock.Unlock();
            return;
        }
 
        tmp_ta->array[level_2_index].owner = NO_USER;
 
        if ((tmp_ta->first_free == -1) || (tmp_ta->first_free > level_2_index)) {
                tmp_ta->first_free = level_2_index;
        }
 
        tmp_ta->use_count--;
 
        if (tmp_ta->use_count == 0) {
                grFree((char *)tmp_ta);
                level_1[level_1_index] = 0;
                full_search_adjust();
        }
        else {
                if (level_1_index < first_free) {
                        first_free = level_1_index;
                }
        }

	tokenLock.Unlock();
}
 
/*-------------------------------------------------------------*/

int32 TokenSpace::grab_atom(int32 token, SAtom **atom)
{
	int32 r=-1;
	int16 type=0;
	atomLock.Lock();

	if (get_token(token,&type,(void **)atom) == 0) {
		if (atom) {
			if (type & TT_ATOM) {
				(*atom)->ServerLock();
				r = 0;
			} else r = 1;
		};
	};

	atomLock.Unlock();
	return r;
};

int32 TokenSpace::delete_atom(int32 token, SAtom *atom)
{
	atomLock.Lock();

	if (atom->ClientUnlock() == 1) remove_token(token);
	atom->ServerUnlock();

	atomLock.Unlock();
	return 0;
};

/*-------------------------------------------------------------*/
 
void    TokenSpace::full_search_adjust()
{
        adjust_free(-1, -1);
}
