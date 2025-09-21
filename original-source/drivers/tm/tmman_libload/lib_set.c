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
 *  Module name              : Lib_Set.c    1.13
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : generic set ADT
 *
 *  Last update              : 17:17:39 - 97/05/16
 *
 *  Description              : generic set data type 
 *
 *
 * This unit models the concept of Set.
 *
 * Conceptually, Sets are mappings from the element type of the Sets to Bool.
 *
 * In this way, this unit provides a hashing-based implementation of Sets with
 * yet unspecified Element types. No assumptions or restrictions
 * are made on these types, except for their representation sizes: these must
 * be equal to the representation size of pointers.
 * 
 * This unit provides a function to create Sets, procedures for adding
 * / deleting elements to/from Sets, and a traversal function
 * that enumerates all elements of a Set while applying 
 * a specified traversal function to each of them.
 *
 * Due to the generality of the 'formal' type Element, the client
 * of this unit has to provide a function to decide for equality of two
 * Elements, and a function that hashes an Element on the
 * Integer range. Standard equalities and hashings for often used Element
 * types have been provided in unit StdOps.
 *
 * An often used Element type is Integer. This type is commonly considered
 * to be compatible with pointer types in "C", although this has not been
 * specified in the language definition.
 * This unit allows Integer to occur as Element, but performs a check on
 * pointer compatibility in the initialisation. In case this check fails, 
 * the unit Mapping, on which this unit is based, has to be recoded to 
 * allow Integer as Domain. Or when sets always contain 'near' elements
 * the unit Lib_Bitset can be used.
 *
 */

/*--------------------------------includes------------------------------------*/

#include "Lib_Set.h"
#include "Lib_Exceptions.h"


/*--------------------------------macros-------------------------------------*/


#define SET_TABLE_SIZE(set)	(1<<(set)->log_table_size)
#define HASH_MASK(set)		(SET_TABLE_SIZE(set) -1)
#define HASH(set, d)	        ((Int)set->hash(d))
 
 


/*--------------------------------functions-----------------------------------*/

/*
 * Function         : create a new set element identified by 'value'
 *                    element
 * Parameters       : (I) element   - a element value
 *                    (I) tail     - new set element will be 
 *                                   inserted before tail
 * Function Result  : insert new set element with the tail 
 *                    field pointing to the 'tail' set element
 * Precondition     : dynamic memory has to be available
 * Postcondition    : a pointer to the new set element
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
    
    ASSERT(result >= 0 && result < 31, (LibMsg_Unclassified) );
    
    return result;
}



/*
 * Function         : create an empty set with the specified 
 *                    characteristics
 * Parameters       : (I) equal - function to test for equality between 
 *                                set elements
 *                    (I) hash  - function to hash the set's elements
 *                    (I) size  - size of the hash table of the set. Size
 *                                will be rounded up to the next power of 2.
 * Function Result  : a new, empty set
 */

Lib_Set 
Lib_Set_create( Lib_Local_BoolResFunc         equal, 
                Lib_Local_EltResFunc          hash, 
                Int                       size)
{
    Int log_table_size, nsize;
    
    Lib_Set result;

    Lib_Mem_NEW(result);

    size= LMAX(size,1);    

    log_table_size = log_2(size);
    nsize          = 1 << log_table_size;

    result->elements		=  (ElementList*)
                                      Lib_Mem_MALLOC(
                                           nsize*sizeof(Pointer)
                                      );
    result->equal		= equal;
    result->hash		= hash;
    result->log_table_size	= log_table_size;
    result->nrof_elements	= 0;
    result->set_hash		= 0;
    result->insert_order_head   = NULL;
    result->insert_order_tail   = NULL;

    memset((char*)result->elements, 0, nsize*sizeof(Pointer));
    return result;
}





static void
set_add(Pointer e, Lib_Set result)
{
    Lib_Set_insert(result, e);
}


/*
 * Function         : Change the number of buckets in specified set,
 *                    and reorganise representation.
 * Parameters       : set    (I/O)  set to be enlarged/shrunk
 *                    factor (I)    factor to get new nr of buckets
 * Function Result  : -
 * Precondition     : -
 * Postcondition    : - 
 * Sideeffects      : when factor > 0, the new nr of set buckets becomes
 *                    the old number multiplied by 'factor', otherwise
 *                    it becomes the old number divided by 'factor'
 */

void 
Lib_Set_rehash( Lib_Set set, Int factor)
{
    Int      old_size = SET_TABLE_SIZE(set);
    Int      new_size = ( factor>=0 
                            ? old_size *  factor
                            : old_size / -factor
                            );

    Lib_Set  new_set  = Lib_Set_create(
                                   set->equal,
                                   set->hash,
                                   new_size
                            );
    Lib_Set_traverse(set, set_add, new_set);

    LSWAP( *set, *new_set, struct Lib_Set );

    Lib_Set_delete( new_set );
}



/*
these 'static' functions are never used within this file

    static Int elementlist_size( ElementList list )
    {
	Int result= 0;
    
	while (list != Null)
	{
	    result++;
	    list = list->tail;
	}
    
	return result;
    }
    

    static ElementList elementlist_copy( ElementList list )
    {
	ElementList result, l;
    
        Lib_Mem_LIST_NEW( result, elementlist_size(list) );
	
	l= result;
    
	while (list != Null)
	{
	    l->element= list->element;
    
	    list = list->tail;
	    l    = l   ->tail;
	}
	return result;
    }
*/


/*
 * Function         : copy an existing or Null set
 * Parameters       : (I) set  - set to copy 
 * Function Result  : a set with ths same contents as
 *                    'set' (at the moment of copying)
 */

Lib_Set 
Lib_Set_copy( Lib_Set set )
{
    Lib_Set result= Null;
    ElementList l;
    Int  	i;
    /*
    unused variable
    , hash;
    */

    if (set == Null)
        return result;

    result = Lib_Set_create(set->equal,
                            set->hash,
                            SET_TABLE_SIZE(set));

    for(i = 0; i < SET_TABLE_SIZE(set); i++)
        result->elements[i] = NULL;

    for(l = set->insert_order_head; l != NULL; l = l->insert_order_next)
	Lib_Set_insert(result, l);

    result->nrof_elements = set->nrof_elements;
    result->set_hash      = set->set_hash;
    return result;
}





/*
 * Function         : test for existence of element in  set
 * Parameters       : (I) set      - set to check 
 *                    (I) element  - element to check for
 * Function Result  : True iff the element is in the set
 */

Bool 
Lib_Set_element( Lib_Set set, 
                 Pointer  element)
{
    ElementList elements;

    ASSERT( set!= Null, (LibMsg_Null_Set) );

    elements = set->elements[ HASH(set, element) & HASH_MASK(set) ];

    while ( elements != (ElementList)Null ) 
    {
        if ( set->equal(elements->element, element) ) 
            return True;
                
        elements = elements->tail;
    }
    return False;
}



/*
 * Function         : add element to a set (when not already there)
 * Parameters       : (IO) set      - set to insert to
 *                    (I)  element  - element to insert
 * Function Result  : -
 */

void 
Lib_Set_insert( Lib_Set          set, 
                Pointer  element)
{
    ElementList *elements;
    ElementList result;
    Int	hash;

    ASSERT( set != Null, (LibMsg_Null_Set) );
    
    hash = HASH(set, element);
    elements = &( set->elements[ hash & HASH_MASK(set)]);

    while (*elements != (ElementList) Null)
    {
	if (set->equal((*elements)->element, element))
	{
	    (*elements)->element = element;
	    return ;
	}
	elements = &((*elements)->tail);
    }
    

    Lib_Mem_NEW(result);

    result->element =  element;
    result->tail    = *elements;
   *elements        =  result;
 
    result->insert_order_next = NULL;	/* append to insert order list */
    result->insert_order_prev = set->insert_order_tail;

    if(set->insert_order_head == NULL)
        set->insert_order_head = result;
    else
        set->insert_order_tail->insert_order_next = result;

    set->insert_order_tail = result;
   
    set->nrof_elements++;
    set->set_hash ^= hash;
}






/*
 * Function         : remove a set element (when existent)
 * Parameters       : (O) set      - set to delete from
 *                    (I) element  - element to remove
 * Function Result  : -
 */

Pointer 
Lib_Set_remove( Lib_Set set, 
                Pointer element)
{
    ElementList *elements, remove;
    Int	hash;

    ASSERT( set != Null, (LibMsg_Null_Set) );
    
    hash = HASH(set, element);
    elements = &( set->elements[ hash & HASH_MASK(set) ]);

    while ((*elements) != (ElementList) Null)
    {
        if (set->equal((*elements)->element, element))
        {
            Pointer result;

	    remove    = *elements;

	    /* unlink remove from insert_order list */
	    if(remove->insert_order_prev == NULL)
                set->insert_order_head = remove->insert_order_next;
            else
                remove->insert_order_prev->insert_order_next = 
						remove->insert_order_next;

            if(remove->insert_order_next == NULL)
                set->insert_order_tail = remove->insert_order_prev;
            else
                remove->insert_order_next->insert_order_prev = 
						remove->insert_order_prev;     

	    *elements = (*elements)->tail;
	    set->nrof_elements--;
            set->set_hash ^= hash;
            result= remove->element;
            Lib_Mem_FAST_FREE(remove);

            return result;
       }
       elements = &((*elements)->tail);
    }

    return Null;
}



                 
/*
 * Function         : get arbitrary element from set
 * Parameters       : (I) set      - set to get from 
 * Function Result  : an element from the set, of Null when set empty
 */

Pointer 
Lib_Set_any_element( Lib_Set set )
{
    return set->insert_order_head->element;
}



    
/*
 * Function         : apply function 'f' to all elements of the set,
 *                    with additional data argument 
 * Parameters       : (I) set  - set to traverse
 *                    (I) f    - function to apply to the set's elements 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each set element, 
 *                    in unspecified but reproducible order.
 */

void 
_Lib_Set_traverse( Lib_Set             set, 
                   Lib_Local_PairFunc  f,
                   Pointer             data )
{
    ElementList l, l_next;

    for(l = set->insert_order_head; l != NULL; l = l_next)
    {
         l_next = l->insert_order_next;

	 f(l->element, data);
    }
}







/*
 * Function         : remove all elements from the set,
 *                    but do not delete it
 * Parameters       : (O) set to empty
 * Function Result  : -
 */

void 
Lib_Set_empty( Lib_Set set)
{
    Int 	i;
    /*
    unused variables
    ElementList	l, p;
	*/
	
    for (i = 0; i < SET_TABLE_SIZE(set); i++) 
    {
	if (set->elements[i] != Null) {
	    Lib_Mem_LIST_FAST_FREE( 
		 set->elements[i]
	    );
	    set->elements[i] = Null;
	}
    }

    set->nrof_elements       = 0;
    set->set_hash            = 0;
    set->insert_order_head   = NULL;
    set->insert_order_tail   = NULL;

}




/*
 * Function         : get size of set
 * Parameters       : set    (I) set to obtain size from
 * Function Result  : nr of set pairs contained by the set
 */

Int Lib_Set_size( Lib_Set set)
{
    return set->nrof_elements;
}




/*
 * Function         : hash set
 * Parameters       : (I) set - set to be hashed
 * Function Result  : hash-value for this set
 */

Int Lib_Set_hash( Lib_Set set)
{
    return set->set_hash;
}




/*
 * Function         : delete a set, free the occupied memory
 * Parameters       : (O) set to delete
 * Function Result  : -
 */

void 
Lib_Set_delete( Lib_Set set)
{
    if (set == Null)
        return;

    Lib_Set_empty( set );

    Lib_Mem_FREE_SIZE ( 
               set->elements,
               SET_TABLE_SIZE(set)*sizeof(Pointer)
    );

    Lib_Mem_FAST_FREE ( set );
}






/*
 * Function         : compare 2 sets for equality
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return True iff l and r have the same elements
 */

Bool 
Lib_Set_equal( Lib_Set l, Lib_Set r)
{
	/*
	unused variable
    Int  	i;
	*/
	
    ASSERT( l->equal  == r->equal &&
            l->hash   == r->hash,  
            (LibMsg_Incompatible_Sets));

    if ( l == r )
        return True;
        
    if ( l->nrof_elements != r->nrof_elements
      || l->set_hash      != r->set_hash )
    {
        return False;
    }
    
    return Lib_Set_subset(l, r);
}







#define MAGIC 0x9a3fb299

static void
subset(Pointer e, Lib_Set superset)
{
    if (!Lib_Set_element(superset, e))
    	{
    	Lib_Excp_raise(MAGIC);
    	}
}

/*
 * Function         : compare 2 sets for inclusion
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return True iff all of l's elements are also in r
 */

Bool 
Lib_Set_subset( Lib_Set l, Lib_Set r)
{
    Bool result;

    ASSERT( l->equal  == r->equal &&
            l->hash   == r->hash,
            (LibMsg_Incompatible_Sets));

    if (l->nrof_elements > r->nrof_elements) return False;

    Lib_Excp_try {
           Lib_Set_traverse( l, subset, r );
           result= True;
    }  Lib_Excp_otherwise {
           result= False;
    }  Lib_Excp_end_try;

    return result;
}





typedef struct
{
    Lib_Set set2, result;
}  IntersectData;


static void
intersect(Pointer e, IntersectData *data)
{
    if (Lib_Set_element(data->set2, e))
	Lib_Set_insert(data->result, e);
}


/*
 * Function         : makes a new set that is the intersection of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : intersection
 * Precondition     : equal, hash of both sets should be the same
 * Sideeffects      : number of hash buckets is the minimum of that of the
 *                  : input sets
 */

Lib_Set 
Lib_Set_intersection( Lib_Set set1, Lib_Set set2)
{
    IntersectData  	data;
    Int		size1, size2;

    ASSERT( set1->equal  == set2->equal &&
            set1->hash   == set2->hash,
            (LibMsg_Incompatible_Sets));


    size1 = SET_TABLE_SIZE(set1);
    size2 = SET_TABLE_SIZE(set2);
    
    data.set2   = set2;
    data.result = Lib_Set_create( set1->equal, 
                                  set1->hash, 
                                  LMIN( size1, size2 ));

    Lib_Set_traverse( set1, intersect, &data );

    return data.result;
}







static void
difference(Pointer e, Lib_Set new_set)
{
    Lib_Set_remove(new_set, e);
}


/*
 * Function         : makes a new set with all values in set2 removed from set1
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : set1 \ set2
 * Precondition     : equal, hash of both sets should be the same
 * Sideeffects      : number of hash buckets is that of set1
 */

Lib_Set 
Lib_Set_difference( Lib_Set set1, Lib_Set set2)
{
    Lib_Set result = Lib_Set_copy(set1);

    ASSERT( set1->equal  == set2->equal &&
            set1->hash   == set2->hash,
            (LibMsg_Incompatible_Sets));

    Lib_Set_traverse( set2, difference, result );

    return result;
}







static void
set_union(Pointer e, Lib_Set result)
{
    Lib_Set_insert(result, e);
}


/*
 * Function         : makes a new set that is the union of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : disjunction
 * Precondition     : equal, hash of both sets should be the same
 * Sideeffects      : number of hash buckets is that of the largest input set
 */
Lib_Set 
Lib_Set_union( Lib_Set set1, Lib_Set set2)
{
    Lib_Set result = Lib_Set_copy(set1);

    ASSERT( set1->equal  == set2->equal &&
            set1->hash   == set2->hash,
            (LibMsg_Incompatible_Sets));

    Lib_Set_traverse( set2, set_union, result );

    return result;
}



