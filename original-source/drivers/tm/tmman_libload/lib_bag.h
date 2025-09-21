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
 *  Module name              : Lib_Bag.h    1.7
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : generic bag ADT
 *
 *  Last update              : 08:42:32 - 97/03/19
 *
 *  Description              : generic bag (or multiset) data type 
 *
 *
 * This unit models the concept of Bag, which is an abstraction similar
 * to a Set, with the exception that elements may occur 'more than once'.
 *
 * Conceptually, and this is also the way in which it is implemented by
 * this unit, Bags are mappings from the element type of the Bag to Integer.
 *
 * In this way, this unit provides a hashing-based implementation of Bags with
 * yet unspecified Element types. No assumptions or restrictions
 * are made on these types, except for their representation sizes: these must
 * be equal to the representation size of pointers.
 * 
 * This unit provides a function to create Bags, procedures for adding
 * / deleting elements to/from bags, and a traversal function
 * that enumerates all pairs (element,amount) of a Bag while applying 
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
 * allow Integer as Domain.
 *
 */

#ifndef Lib_Bag_INCLUDED
#define Lib_Bag_INCLUDED

/*--------------------------------includes------------------------------------*/

#include "Lib_Local.h"
#include "Lib_StdFuncs.h"
#include "Lib_Mapping.h"



/*--------------------------------definitions---------------------------------*/

/*
 * definition of a bag based on mapping 
 */
typedef struct Lib_Bag {
   Lib_Mapping          mapping;      /* Mapping on which the bag is based  */
   Int              hash;         /* Cached, and incremental hash value */
   Int              domain_size;  /* Number of *different* elements     */
   Int              total_size;   /* Total number of elements, i.e.     */
                                      /*  sum of the amounts.               */
} *Lib_Bag;




/*--------------------------------functions-----------------------------------*/


/*
 * Function         : create an empty bag with the specified 
 *                    characteristics
 * Parameters       : (I) equal - function to test for equality between 
 *                                bag elements
 *                    (I) hash  - function to hash the bag's elements
 *                    (I) size  - number of hash-buckets of the bag
 * Function Result  : a new, empty bag
 */

Lib_Bag Lib_Bag_create( 
                  Lib_Local_BoolResFunc         equal, 
                  Lib_Local_EltResFunc          hash, 
                  Int                       size
                          );





/*
 * Function         : Change the number of buckets in specified bag,
 *                    and reorganise representation.
 * Parameters       : bag      (I/O)  bag to be enlarged/shrunk
 *                    factor   (I)    factor to get new nr of buckets
 * Function Result  : -
 * Sideeffects      : when factor > 0, the new nr of bag buckets becomes
 *                    the old number multiplied by 'factor', otherwise
 *                    it becomes the old number divided by 'factor'
 */

void Lib_Bag_rehash( Lib_Bag bag, Int factor);




/*
 * Function         : compare 2 bags for inclusion
 * Parameters       : (I) l and (I) r are bags to be compared
 * Function Result  : return TRUE iff all of l's elements are also in r
 */

Bool Lib_Bag_subbag( Lib_Bag l, Lib_Bag r);




/*
 * Function         : compare 2 bags for equality
 * Parameters       : (I) l and (I) r are bags to be compared
 * Function Result  : return TRUE iff l and r have the same elements
 */

Bool Lib_Bag_equal( Lib_Bag l, Lib_Bag r);




/*
 * Function         : hash bag
 * Parameters       : (I) bag - bag to be hashed
 * Function Result  : hash-value for this bag
 */

Int Lib_Bag_hash( Lib_Bag bag);



/*
 * Function         : copy an existing or Null bag
 * Parameters       : (I) bag  - bag to copy 
 * Function Result  : a bag with ths same contents as
 *                    'bag' (at the moment of copying)
 */

Lib_Bag Lib_Bag_copy( Lib_Bag bag );




/*
 * Function         : add element to a bag 
 * Parameters       : (IO) bag      - bag to insert to
 *                    (I)  element  - element to insert
 * Function Result  : none
 */

void Lib_Bag_insert ( Lib_Bag   bag, 
                      Pointer   element,
                      Int   amount
                    );




/*
 * Function         : remove a bag element (when existent)
 * Parameters       : (O) bag      - bag to delete from
 *                    (I) element  - element to remove
 *                    (I) amount   - nrof elements to be removed
 * Function Result  : none
 */

void Lib_Bag_remove  ( Lib_Bag  bag, 
                       Pointer  element,
                       Int  amount
                      );





/*
 * Function         : test for occurrence of element in  bag
 * Parameters       : (I) bag      - bag to check 
 *                    (I) element  - element to check for
 * Function Result  : results the number of elements of this element in the bag
 */

Int Lib_Bag_element  ( Lib_Bag  bag, 
                           Pointer  element);





/*
 * Function         : get arbitrary element from bag
 * Parameters       : (I) bag      - bag to get from 
 * Function Result  : an element from the bag, of Null when bag empty
 */

Pointer Lib_Bag_any_element( Lib_Bag bag );





/*
 * Function         : apply function 'f' to all elements of the bag,
 *                    with additional data argument.
 * Parameters       : (I) bag  - bag to traverse
 *                    (I) f    - function to apply to the bag's elements 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each bag pair, 
 *                    in unspecified but reproducible order.
 */

void _Lib_Bag_traverse( Lib_Bag                  bag, 
                        Lib_Local_TripleFunc     f,
                        Pointer                  data );

#define Lib_Bag_traverse(bag,f,data) \
               _Lib_Bag_traverse(                         \
                        (bag),                            \
                        (Lib_Local_TripleFunc)(f),        \
                        (Pointer)(data)                   \
                )





/*
 * Function         : delete a bag, free the occupied memory
 * Parameters       : (O) bag to delete
 * Function Result  : none
 */

void Lib_Bag_delete( Lib_Bag bag );



/*
 * Function         : remove all elements from the bag,
 *                    but do not delete it
 * Parameters       : (O) bag to empty
 * Function Result  : none
 */

void Lib_Bag_empty( Lib_Bag bag );



/*
 * Function         : get the total size of bag, taking multiple 
 *                    occurrences into acount.
 * Parameters       : bag    (0) bag to obtain size from 
 *                               two equal elements are counted twice
 * Function Result  : nr of bag pairs contained by the bag
 */

Int Lib_Bag_size( Lib_Bag bag);
Int Lib_Bag_set_size( Lib_Bag bag);



/*
 * Function         : makes a new bag that is the intersection of input bags
 * Parameters       : bag    (I) bag 1
 *                  : bag    (I) bag 2
 * Function Result  : intersection when there is a common element
 *                    or Null when there is no common element.
 * Precondition     : equal, size, hash of both bags should be the same
 * Sideeffects      : size is the minimum of the bag1 and bag2
 */

Lib_Bag Lib_Bag_intersection( Lib_Bag bag1, Lib_Bag bag2);




/*
 * Function         : makes a new bag that is a special intersection of 
 *                    input bags: an element with amount is in this kind
 *                    of intersection iff the element is present with 
 *                    exactly that same amount in the input bags.
 * Parameters       : bag    (I) bag 1
 *                  : bag    (I) bag 2
 * Function Result  : intersection when there is a common element 
 *                    with common amount or Null when there is no common 
 *                    element.
 * Precondition     : equal, size, hash of both bags should be the sam
 * Sideeffects      : size is the minimum of the bag1 and bag2
 */

Lib_Bag Lib_Bag_amounts_intersection( Lib_Bag bag1, Lib_Bag bag2);




/*
 * Function         : makes a new bag with all values in bag2 removed from bag1
 * Parameters       : bag    (I) bag 1
 *                  : bag    (I) bag 2
 * Function Result  : bag1 \ bag2
 * Precondition     : equal, hash of both bags should be the same
 * Sideeffects      : number of hash buckets is that of bag1
 */

Lib_Bag Lib_Bag_difference( Lib_Bag bag1, Lib_Bag bag2);




/*
 * Function         : all values in bag2 removed from bag1
 * Parameters       : bag    (I) bag 1
 *                  : bag    (I) bag 2
 * Function Result  : bag1 \ bag2
 * Precondition     : equal, hash of both bags should be the same
 * Sideeffects      : number of hash buckets is that of bag1
 */

void Lib_Bag_in_place_difference( Lib_Bag bag1, Lib_Bag bag2);




/*
 * Function         : makes a new bag that is the union of input bags
 * Parameters       : bag    (I) bag 1
 *                  : bag    (I) bag 2
 * Function Result  : disjunction
 * Precondition     : equal, size, hash of both bags shoudl be the same
 * Sideeffects      : size is the minimum of the bag1 and bag
 */
Lib_Bag Lib_Bag_union( Lib_Bag bag1, Lib_Bag bag2);


#endif /* Lib_Bag_INCLUDED */
     
   
