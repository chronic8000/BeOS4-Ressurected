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
 *  Module name              : Lib_Mapping.c    1.12
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : generic mapping ADT
 *
 *  Author                   : Juul van der Spek 
 *                           & Rogier Wester
 *
 *  Last update              : 17:16:45 - 97/05/16
 *
 *  Reviewed                 : 
 *
 *  Revision history         : 
 *
 *  Description              : generic hashing- based mapping data type 
 *
 *
 * This unit models the concept of (mathematical) function, which is a
 * (partial) mapping of a Domain type to a Range type. Such mappings can
 * be considered as a collection of pairs
 *
 *              { (d1,r1), .., (dn,rn) }   di is elt. Domain, ri is elt. Range
 *
 * with unique di's.
 *
 * This unit provides a hashing-based implementation of Mappings with
 * yet unspecified Domain and Range types. No assumptions or restrictions
 * are made on these types, except for their representation sizes: these must
 * be equal to the representation size of pointers.
 * 
 * This unit provides a function to create mappings, procedures for adding
 * / deleting pairs to/from mappings, a function to retrieve the Range
 * value currently associated to a Domain value, and a traversal function
 * that enumerates all pairs of a mapping while applying a specified traversal
 * function to each of them.
 * Due to the generality of the 'formal' types Domain and Range, the client
 * of this unit has to provide a function to decide for equality of two
 * Domain elements, and a function that hashes a Domain element on the
 * Int range. Standard equalities and hashings for often used Domain types
 * have been provided in unit StdOps.
 *
 * An often used Domain type is Int. This type is commonly considered
 * to be compatible with pointer types in "C", although this has not been
 * specified in the language definition.
 * This unit allows Int to occur as Domain, but performs a check on
 * pointer compatibility in the initialisation. In case this check fails, this
 * unit has to be recoded to allow Int as Domain.
 *
 */


/*--------------------------------includes------------------------------------*/

#include "Lib_Local.h"
#include "Lib_Mapping.h"
#include "Lib_StdFuncs.h"

/*--------------------------------definitions---------------------------------*/

/*
 * definition of the pairlist link block,
 * NB: the link field 'tail' is at the beginning,
 * to make use of the list allocation possibilities
 * of the memory manager:
 */

struct PairList_s
{
        struct PairList_s   *tail;
	struct PairList_s   *insert_order_prev;
        struct PairList_s   *insert_order_next;
        Pointer  domain;
        Pointer  range;

} ;

/*--------------------------------functions-----------------------------------*/

#define MAP_TABLE_SIZE(map)	(1<<(map)->log_table_size)
#define HASH_MASK(map)		(MAP_TABLE_SIZE(map) -1)
#define HASH(map, d)		((Int)map->hash(d))
 

/*
 * Function         : create a new mapping pair identified by 'value'
 *                    domain
 * Parameters       : (I) domain   - a domain value
 *                    (I) range    - a pointer to a Range pair
 *                    (I) next     - new mapping pair will be 
 *                                   inserted before next
 * Function Result  : insert new mapping pair with the tail 
 *                    field pointing to the 'next' mapping pair
 * Precondition     : dynamic memory has to be available
 * Postcondition    : a pointer to the new mapping pair
 * Sideeffects      : dynamic memeory is allocated
 */



static Int 
log_2( UInt size )
{
    Int     result = 0;
    UInt	c;


    c = size;
    while (c > 0)
    {
	c = c >> 1;
	result++;
    }
    
    if (LIS_POW_2(size))
    {
        result--;
    }
    
    return result;
}


/*
 * Function         : create an empty mapping with the specified 
 *                    characteristics
 * Parameters       : (I) equal - function to test for equality between 
 *                                map-list elements
 *                    (I) hash  - function to hash a user defined domain 
 *                                   element
 *                    (I) size  - number of hash-buckets of the mapping,
 *                                will be rounded up to next power of two
 * Function Result  : a new mapping is created using the parameters
 * Sideeffects      : size is rounded up to next power of 2.
 */

Lib_Mapping Lib_Mapping_create( 
                  Lib_Local_BoolResFunc    equal, 
                  Lib_Local_EltResFunc     hash, 
                  Int                  size)
{
    Int log_table_size, nsize;

    Lib_Mapping result;

    Lib_Mem_NEW(result);
    
    size= LMAX(size,1);    
    
    log_table_size = log_2(size);
    nsize          = 1 << log_table_size;

    result->pairs           = (PairList*) Lib_Mem_MALLOC(nsize*sizeof(Pointer));
    result->equal           = equal;
    result->hash            = hash;
    result->log_table_size  = log_table_size;
    result->nrof_elements   = 0;
    result->insert_order_head   = NULL;
    result->insert_order_tail   = NULL;

    memset((char *)result->pairs, 0, nsize * sizeof(PairList));
    
    return result;
}






static void
mapping_add(
      Pointer    dom, 
      Pointer     ran, 
      Lib_Mapping   result
   )
{
    Lib_Mapping_define(result, dom, ran);
}


/*
 * Function         : Change the number of buckets in specified mapping,
 *                    and reorganise representation.
 * Parameters       : mapping  (I/O)  mapping to be enlarged/shrunk
 *                    factor   (I)    factor to get new nr of buckets
 * Function Result  : -
 * Sideeffects      : when factor > 0, the new nr of mapping buckets becomes
 *                    the old number multiplied by 'factor', otherwise
 *                    it becomes the old number divided by 'factor'
 */

void 
Lib_Mapping_rehash( Lib_Mapping mapping, Int factor)
{
    Int      old_size = MAP_TABLE_SIZE(mapping);
    Int      new_size = ( factor>=0 
                            ? old_size *  factor
                            : old_size / -factor
                            );

    Lib_Mapping  new_mapping 
                              = Lib_Mapping_create(
                                   mapping->equal,
                                   mapping->hash,
                                   new_size
                                );

    Lib_Mapping_traverse(mapping, mapping_add, new_mapping);

    LSWAP( *mapping, *new_mapping, struct Lib_Mapping );

    Lib_Mapping_delete( new_mapping );
}






/*
this 'static' function is never used within this file
    static Int pairlist_size( PairList list )
    {
	Int result= 0;
    
	while (list != Null)
	{
	    result++;
	    list = list->tail;
	}
    
	return result;
    }
*/    
/*
this 'static' function is never used within this file
    static PairList pairlist_copy( PairList list )
    {
	PairList result, l;
    
        Lib_Mem_LIST_NEW(result, pairlist_size(list));
	
	l= result;
    
	while (list != Null)
	{
	    l->domain= list->domain;
	    l->range = list->range;
    
	    list = list->tail;
	    l    = l   ->tail;
	}
	return result;
    }
*/


/*
 * Function         : copy an existing mapping or return Null if non existing
 * Parameters       : (I) map  - mapping to copy 
 * Function Result  : a mapping with the same contents as
 *                    'map' (at the moment of copying
 */

Lib_Mapping Lib_Mapping_copy( Lib_Mapping map )
{
    Lib_Mapping result = Null;
    PairList    l;
    Int		i;

    if (map == Null)
        return result;

    result = Lib_Mapping_create(
                    map->equal,
                    map->hash,
                    MAP_TABLE_SIZE(map));

    for(i = 0; i < MAP_TABLE_SIZE(map); i++) 
	result->pairs[i] = NULL;

    for(l = map->insert_order_head; l != NULL; l = l->insert_order_next)
        Lib_Mapping_define(result, l->domain, l->range);

    result->nrof_elements = map->nrof_elements;
    return result;
}





/*
 * Function         : find the a mapping pair identified by 'domain'
 * Parameters       : (I) map   - a mapping
 *                    (I) domain - find the mapping pair identified by 
 *                    'domain'
 * Function Result  : function_result = map( domain) OR 
 *                    function_result = Null if 'domain' is not in the
 *                    mapping domain
 */

Pointer Lib_Mapping_apply ( Lib_Mapping map, Pointer  domain)
{
    PairList pairs;
    /*
    unused variable
    Int  search_size = 0;
	*/
	
    ASSERT( map != Null, (LibMsg_Null_Mapping) );
    pairs = map->pairs[ HASH(map, domain) & HASH_MASK(map) ];

    if (map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_address_equal 
     || map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_int_equal)
    {
        while (pairs != (PairList)Null) 
        {
            if ( pairs->domain == domain ) 
            {
                return pairs->range;
            }
            pairs = pairs->tail;
        }
    }
    else
    {
        while (pairs != (PairList)Null) 
        {
            if ( map->equal(pairs->domain, domain) ) 
            {
                return pairs->range;
            }
            pairs = pairs->tail;
        }
    }

    return (Pointer)Null;
}





/*
 * Function         : get arbitrary element from the mapping's domain
 * Parameters       : (I) map      - mapping to get from 
 * Function Result  : an element from the mapping's domain, 
 *                    or Null when set empty
 */

Pointer 
Lib_Mapping_any_domain( Lib_Mapping map )
{
	return map->insert_order_head->domain;
}





/*
 * Function         : add mapping pair to a mapping
 * Parameters       : (O) map - is a mapping
 *                    (I) domain - domain element of the new mapping pair
 *                    (I) range  - range element of the new mapping pair
 * Function Result  : If the mapping already contained a pair 
 *                    <domain,old_range>, old_range is returned; 
 *                    Null otherwise
 * Sideeffects      : if the domain value was already used, then the
 *                    old range value is overwritten
 */

Pointer
        Lib_Mapping_define ( Lib_Mapping map, 
                             Pointer domain, 
                             Pointer range)
{
    PairList *pairs;
    PairList result;

    ASSERT( map!= Null, (LibMsg_Null_Mapping) );
    pairs = &( map->pairs[ HASH(map, domain) & HASH_MASK(map) ]);

    if (map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_address_equal
     || map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_int_equal)
    {
	while (*pairs != (PairList) Null)
	{
	    if ( (*pairs)->domain == domain)
	    {
		Pointer old_range = (*pairs)->range;

		(*pairs)->domain = domain;
		(*pairs)->range  = range;
		return old_range;
	    }
	    pairs = &((*pairs)->tail);
	}
    }
    else
    {
	while (*pairs != (PairList) Null)
	{
	    if (map->equal((*pairs)->domain, domain))
	    {
		Pointer old_range = (*pairs)->range;

		(*pairs)->domain = domain;
		(*pairs)->range  = range;
		return old_range;
	    }
	    pairs = &((*pairs)->tail);
	}
    }
    

    Lib_Mem_NEW(result);

    result->domain = domain;
    result->range  = range;
    result->tail   = Null;

   *pairs=  result;

    result->insert_order_next = NULL;   /* append to insert order list */
    result->insert_order_prev = map->insert_order_tail;

    if(map->insert_order_head == NULL)
        map->insert_order_head = result;
    else
        map->insert_order_tail->insert_order_next = result;

    map->insert_order_tail = result;

    map->nrof_elements++;

    return Null;
}





/*
 * Function         : remove a mapping element identified by domain
 * Parameters       : (O) map - a mapping
 *                    (I) domain - a domain element
 * Function Result  : If the mapping contained a pair 
 *                    <domain,old_range>, old_range is returned; 
 *                    Null otherwise
 */

Pointer
        Lib_Mapping_undefine ( Lib_Mapping map, 
                               Pointer  domain)
{
    PairList *pairs, remove;

    ASSERT( map!= Null, (LibMsg_Null_Mapping) );
    pairs = &( map->pairs[ HASH(map, domain) & HASH_MASK(map) ]);

    if (map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_address_equal
     || map->equal == (Lib_Local_BoolResFunc)Lib_StdFuncs_int_equal)
    {
	while ((*pairs) != (PairList) Null)
	{
	    if ((*pairs)->domain == domain)
	    {
		Pointer old_range = (*pairs)->range;

		remove = *pairs;

		/* unlink remove from insert_order list */
                if(remove->insert_order_prev == NULL)
                    map->insert_order_head = remove->insert_order_next;
                else
                    remove->insert_order_prev->insert_order_next = 
                                                remove->insert_order_next;

                if(remove->insert_order_next == NULL)
                    map->insert_order_tail = remove->insert_order_prev;
                else
                    remove->insert_order_next->insert_order_prev = 
                                                    remove->insert_order_prev;  

		*pairs = (*pairs)->tail;
		map->nrof_elements--;
		Lib_Mem_FAST_FREE(remove);
		return old_range;
	    }
	    pairs = &((*pairs)->tail);
	}
    }
    else
    {
	while ((*pairs) != (PairList) Null)
	{
	    if (map->equal((*pairs)->domain, domain))
	    {
		Pointer old_range = (*pairs)->range;

		remove = *pairs;

		/* unlink remove from insert_order list */
                if(remove->insert_order_prev == NULL)
                    map->insert_order_head = remove->insert_order_next;
                else
                    remove->insert_order_prev->insert_order_next = 
                                                remove->insert_order_next;

                if(remove->insert_order_next == NULL)
                    map->insert_order_tail = remove->insert_order_prev;
                else
                    remove->insert_order_next->insert_order_prev = 
                                                    remove->insert_order_prev;  

		*pairs = (*pairs)->tail;
		map->nrof_elements--;
		Lib_Mem_FAST_FREE(remove);
		return old_range;
	    }
	    pairs = &((*pairs)->tail);
	}
    }
    
    return Null;
}




/*
 * Function         : perform function 'f' on all elements of the mapping,
 *                    with additional data argument.
 * Parameters       : (I) map  - a mapping
 *                    (I) f    - function for processing mapping pairs 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each mapping pair, 
 *                    in unspecified but reproducible order.
 */

void _Lib_Mapping_traverse( Lib_Mapping           map, 
                            Lib_Local_TripleFunc  f,
                            Pointer               data )
{
	PairList l, l_next;

	for(l = map->insert_order_tail; l != NULL; l = l_next)
    	{
		l_next = l->insert_order_prev;

         	f(l->domain, l->range, data);
    	}
}



/*
 * Function         : perform function 'f' on all domain elements of the 
 *                    mapping, with additional data argument.
 * Parameters       : (I) map  - a mapping
 *                    (I) f    - function for processing domain elements 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each domain element, 
 *                    in unspecified but reproducible order.
 */

void _Lib_Mapping_domain_traverse( Lib_Mapping         map, 
                                   Lib_Local_PairFunc  f,
                                   Pointer               data )
{
	PairList l, l_next;

        for(l = map->insert_order_tail; l != NULL; l = l_next)
        {
		l_next = l->insert_order_prev;

                f(l->domain, data);
        }
}





/*
 * Function         : perform function 'f' on all range elements of the 
 *                    mapping, with additional data argument.
 * Parameters       : (I) map  - a mapping
 *                    (I) f    - function for processing range elements 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each range element, 
 *                    in unspecified but reproducible order.
 */

void _Lib_Mapping_range_traverse(  Lib_Mapping         map, 
                                   Lib_Local_PairFunc  f,
                                   Pointer               data )
{
	PairList l, l_next;

	for(l = map->insert_order_head; l != NULL; l = l_next)
        {
                l_next = l->insert_order_next;

                f(l->range, data);
        }
}





/*
 * Function         : remove all pairs from the mapping,
 *                    but do not delete it
 * Parameters       : (O) map is a mapping
 * Function Result  : none
 */

void Lib_Mapping_empty(Lib_Mapping map)
{
    Int  i;
    /*
    unused variables
    PairList l, p;
	*/
    for (i = 0; i < MAP_TABLE_SIZE(map); i++) 
    {
	if (map->pairs[i] != Null) {
	    Lib_Mem_LIST_FAST_FREE(map->pairs[i]);
	    map->pairs[i] = Null;
	}
    }

    map->nrof_elements = 0;
    map->insert_order_head   = NULL;
    map->insert_order_tail   = NULL;
}








/*
 * Function         : delete a mapping, free the occupied memory
 * Parameters       : (O) map is a mapping
 * Function Result  : none
 */

void Lib_Mapping_delete( Lib_Mapping map)
{
    if (map == Null)
        return;

    Lib_Mapping_empty( map );

    Lib_Mem_FREE_SIZE( 
               map->pairs, 
               MAP_TABLE_SIZE(map)*sizeof(Pointer)
    );

    Lib_Mem_FAST_FREE( map );
}




/*
 * Function         : get size of mapping
 * Parameters       : map    (I) mapping to obtain size from
 * Function Result  : nr of mapping pairs contained by the mapping
 */

Int Lib_Mapping_size( Lib_Mapping map)
{
    return map->nrof_elements;
}
