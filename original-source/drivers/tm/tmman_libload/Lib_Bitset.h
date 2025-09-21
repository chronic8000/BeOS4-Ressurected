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
 *  Module name              : Lib_Bitset.h    1.6
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Dynamic Bitsets
 *
 *  Last update              : 08:42:35 - 97/03/19
 *
 *  Description              :  integer set ADT with bitset implementation
 *
 * This unit models the concept of Integer Set, and actually is a 
 * specialisation of the more general unit Set. This means that this module
 * provides the same operations such that a Set(Integer) is interchangeable
 * with a Bitset.
 *
 * NOTE that a Bitset only is a more efficient candidate if its elements 
 * are 'near', since for each two elements of a bitset, all intermediate
 * values have bits reserved as well.
 *
 * NOTE !!!! negative integers in sets are *NOT* supported by this unit.
 */

#ifndef Lib_Bitset_INCLUDED
#define Lib_Bitset_INCLUDED

/*--------------------------------- Includes: ------------------------------*/

#include "Lib_Local.h"
#include "Lib_StdFuncs.h"



/*--------------------------------definitions---------------------------------*/

/*
 * definition of a bit set 
 */
typedef struct {
   Int              size;        /* Number of elements                 */
   Int              hash;        /* Incremental hash value             */
   Int             *bitset;      /* *the* bitset                       */
   Int              low,high;    /* range of bitset                    */
} *Lib_Bitset;


/*--------------------------------functions-----------------------------------*/


/*
 * Function         : create an empty set 
 * Parameters       : -
 * Function Result  : a new, empty bit set
 */

Lib_Bitset Lib_Bitset_create();





/*
 * Function         : create a new one- element bitset
 * Parameters       : i  (I) value to appear in result
 * Function Result  : a new bitset containing (only) i
 */

Lib_Bitset Lib_Bitset_singleton( Int i );





/*
 * Function         : compare 2 sets for inclusion
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return True iff all of l's elements are also in r
 */

Bool Lib_Bitset_subset( Lib_Bitset l, Lib_Bitset r);





/*
 * Function         : compare 2 sets for equality
 * Parameters       : (I) l and (I) r are sets to be compared
 * Function Result  : return True iff l and r have the same elements
 */

Bool Lib_Bitset_equal( Lib_Bitset l, Lib_Bitset r);





/*
 * Function         : hash set
 * Parameters       : (I) set - set to be hashed
 * Function Result  : hash-value for this set
 */

Int Lib_Bitset_hash( Lib_Bitset set);



/*
 * Function         : copy an existing or Null set
 * Parameters       : (I) set  - set to copy 
 * Function Result  : a set with ths same contents as
 *                    'set' (at the moment of copying)
 * Sideeffects      : set is purged prior to copying
 */

Lib_Bitset Lib_Bitset_copy( Lib_Bitset set );





/*
 * Function         : minimise representation of set
 * Parameters       : (IO) set  - set to purge 
 * Function Result  : -
 */

void Lib_Bitset_purge( Lib_Bitset set );





/*
 * Function         : add element to a set (when not already there)
 * Parameters       : (IO) set      - set to insert to
 *                    (I)  element  - element to insert
 * Function Result  : True iff element was already in the set
 */

Bool Lib_Bitset_insert ( Lib_Bitset      set, 
                            Int         element
                          );





/*
 * Function         : remove a set element (when existent)
 * Parameters       : (O) set      - set to delete from
 *                    (I) element  - element to remove
 * Function Result  : True iff element was in the set
 */

Bool Lib_Bitset_remove  ( Lib_Bitset      set, 
                             Int         element);





/*
 * Function         : test for existence of element in  set
 * Parameters       : (I) set      - set to check 
 *                    (I) element  - element to check for
 * Function Result  : True iff. the element is in the set
 */

Bool Lib_Bitset_element  ( Lib_Bitset      set, 
                              Int         element);





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

void _Lib_Bitset_traverse( Lib_Bitset             set, 
                           Lib_Local_PairResFunc  f,
                           Pointer                data );

#define Lib_Bitset_traverse(set,f,data) \
               _Lib_Bitset_traverse(                   \
                        (set),                         \
                        (Lib_Local_PairResFunc)(f),    \
                        (Pointer)(data)                \
                )





/*
 * Function         : delete a set, free the occupied memory
 * Parameters       : (O) set to delete
 * Function Result  : -
 */

void Lib_Bitset_delete( Lib_Bitset set);





/*
 * Function         : remove all elements from the set,
 *                    but do not delete it
 * Parameters       : (O) set to empty
 * Function Result  : -
 */

void Lib_Bitset_empty( Lib_Bitset set);





/*
 * Function         : get size of set
 * Parameters       : set    (I) set to obtain size from
 * Function Result  : nr of bits contained by the set
 */

Int Lib_Bitset_size( Lib_Bitset set);





/*
 * Function         : get the current capacity of the set,
 *                    i.e. the number of bit positions currently
 *                    allocated. Note that the Lib_Bitsets are
 *                    dynamic, and that the capacity may change 
 *                    due to insertions and purge operations.
 *                    This function can be used to get a measure
 *                    of efficiency of the bitset, as follows:
 *
 *                       Lib_Bitset_size / Lib_Bitset_capacity
 *
 * Parameters       : set    (I) set to obtain capacity from
 * Function Result  : nr of bits allocated by the set
 */

Int Lib_Bitset_capacity( Lib_Bitset set);





/*
 * Function         : makes a new set that is the intersection of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : intersection
 */

Lib_Bitset Lib_Bitset_intersection( Lib_Bitset set1, Lib_Bitset set2);





/*
 * Function         : check for overlap of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : ~ EMPTY[ intersection( set1, set2 ) ]
 */

Bool Lib_Bitset_overlaps( Lib_Bitset set1, Lib_Bitset set2);





/*
 * Function         : makes a new set with all values in set2 removed from set1
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : set1 \ set2
 */

Lib_Bitset Lib_Bitset_difference( Lib_Bitset set1, Lib_Bitset set2);





/*
 * Function         : makes a new set that is the union of input sets
 * Parameters       : set    (I) set 1
 *                  : set    (I) set 2
 * Function Result  : union
 */
Lib_Bitset Lib_Bitset_union( Lib_Bitset set1, Lib_Bitset set2);





/*
 * Function         : intersection of two bitset, modifying the first
 *                    argument into the result
 * Parameters       : into    (I) set 1
 *                  : with    (I) set 2
 * Function Result  : -
 */

void Lib_Bitset_in_place_intersection( Lib_Bitset into, Lib_Bitset with);





/*
 * Function         : difference of two bitset, modifying the first
 *                    argument into the result
 * Parameters       : into    (I) set 1
 *                  : with    (I) set 2
 * Function Result  : -
 */

void Lib_Bitset_in_place_difference( Lib_Bitset into, Lib_Bitset with);





/*
 * Function         : union of two bitset, modifying the first
 *                    argument into the result
 * Parameters       : into    (I) set 1
 *                  : with    (I) set 2
 * Function Result  : -
 */
void Lib_Bitset_in_place_union(Lib_Bitset into, Lib_Bitset with);



#endif /* Lib_Bitset_INCLUDED */


     
   
