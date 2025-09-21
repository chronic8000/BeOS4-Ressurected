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
 *  Module name              : Lib_Set.h    1.9
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : generic set ADT
 *
 *  Last update              : 15:47:55 - 97/04/10
 *
 *  Description              : generic set ADT 
 *
 *
 * This unit models the concept of Set.
 *
 * Conceptually, Sets are mappings from the element type of the Sets to Boolean.
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

#ifndef Lib_Set_INCLUDED
#define Lib_Set_INCLUDED

/*--------------------------------includes------------------------------------*/

#include "Lib_Local.h"
#include "Lib_StdFuncs.h"


/*--------------------------------definitions---------------------------------*/

typedef struct ElementList	*ElementList;

/*
 * definition of a set 
 */
typedef struct Lib_Set {
    Lib_Local_BoolResFunc      	equal;
    Lib_Local_EltResFunc        hash;
    Int                   	log_table_size;
    Int                   	nrof_elements;	/* Cached number of elements     */
    Int              	set_hash;       /* Cached, incremental hash value*/
    ElementList  		*elements;      /* elements of the set           */
    ElementList                 insert_order_head;
    ElementList                 insert_order_tail;
} *Lib_Set;


/*
 * definition of the set element link block,
 * NB: the link field 'tail' is at the beginning,
 * to make use of the list allocation possibilities
 * of the memory manager:
 */
struct ElementList
{
        struct ElementList     *tail;
        struct ElementList     *insert_order_prev;
        struct ElementList     *insert_order_next;
        Pointer            	element;
} ;




/*--------------------------------functions-----------------------------------*/


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
                Int                       size);



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
Lib_Set_rehash( Lib_Set set, Int factor);




/*
 * Function         : compare 2 sets for inclusion
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return TRUE iff all of l's elements are also in r
 */

Bool 
Lib_Set_subset( Lib_Set l, Lib_Set r);




/*
 * Function         : compare 2 sets for equality
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return TRUE iff l and r have the same elements
 */

Bool 
Lib_Set_equal( Lib_Set l, Lib_Set r);




/*
 * Function         : copy an existing or Null set
 * Parameters       : (I) set  - set to copy 
 * Function Result  : a set with ths same contents as
 *                    'set' (at the moment of copying)
 */

Lib_Set 
Lib_Set_copy( Lib_Set set );




/*
 * Function         : add element to a set (when not already there)
 * Parameters       : (IO) set      - set to insert to
 *                    (I)  element  - element to insert
 * Function Result  : -
 */

void 
Lib_Set_insert( Lib_Set set, Pointer element);




/*
 * Function         : remove a set element (when existent)
 * Parameters       : (O) set      - set to delete from
 *                    (I) element  - element to remove
 * Function Result  : -
 */

Pointer 
Lib_Set_remove( Lib_Set set, Pointer element);





/*
 * Function         : test for existence of element in  set
 * Parameters       : (I) set      - set to check 
 *                    (I) element  - element to check for
 * Function Result  : TRUE iff the element is in the set
 */

Bool 
Lib_Set_element( Lib_Set set,  Pointer  element);





/*
 * Function         : get arbitrary element from set
 * Parameters       : (I) set      - set to get from 
 * Function Result  : an element from the set, of Null when set empty
 */

Pointer 
Lib_Set_any_element( Lib_Set set );





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
                   Pointer               data );

#define Lib_Set_traverse(set,f,data) \
               _Lib_Set_traverse(                    \
                        (set),                       \
                        (Lib_Local_PairFunc)(f),     \
                        (Pointer)(data))



/*
 * Function         : delete a set, free the occupied memory
 * Parameters       : (O) set to delete
 * Function Result  : -
 */

void 
Lib_Set_delete( Lib_Set set);


/*
 * Function         : remove all elements from the set,
 *                    but do not delete it
 * Parameters       : (O) set to empty
 * Function Result  : -
 */

void 
Lib_Set_empty( Lib_Set set);



/*
 * Function         : get size of set
 * Parameters       : set    (I) set to obtain size from
 * Function Result  : nr of set pairs contained by the set
 */

Int Lib_Set_size( Lib_Set set);




/*
 * Function         : hash set
 * Parameters       : (I) set - set to be hashed
 * Function Result  : hash-value for this set
 */

Int Lib_Set_hash( Lib_Set set);




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
Lib_Set_intersection( Lib_Set set1, Lib_Set set2);



/*
 * Function         : makes a new set with all values in set2 removed from set1
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : set1 \ set2
 * Precondition     : equal, hash of both sets should be the same
 * Sideeffects      : number of hash buckets is that of set1
 */

Lib_Set 
Lib_Set_difference( Lib_Set set1, Lib_Set set2);



/*
 * Function         : makes a new set that is the union of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : disjunction
 * Precondition     : equal, hash of both sets should be the same
 * Sideeffects      : number of hash buckets is that of the largest input set
 */
Lib_Set 
Lib_Set_union( Lib_Set set1, Lib_Set set2);


#endif /* Lib_Set_INCLUDED */
     
   
