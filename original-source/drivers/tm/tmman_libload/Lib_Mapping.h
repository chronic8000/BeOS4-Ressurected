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
 *  Module name              : Lib_Mapping.h    1.9
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : generic mapping ADT
 *
 *  Last update              : 15:47:53 - 97/04/10
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
 * Integer range. Standard equalities and hashings for often used Domain types
 * have been provided in unit StdOps.
 *
 * An often used Domain type is Integer and Address.
 * This unit allows Integer to occur as Domain, but performs a check on
 * pointer compatibility in the initialisation. In case this check fails, this
 * unit has to be recoded to allow Integer as Domain.
 *
 */

#ifndef Lib_Mapping_INCLUDED
#define Lib_Mapping_INCLUDED

/*--------------------------------includes------------------------------------*/

#include "Lib_Local.h"
#include "Lib_StdFuncs.h"



/*--------------------------------definitions---------------------------------*/

/*
 * definition of a mapping pair structure
 */

typedef struct PairList_s	*PairList;


/*
 * definition of a mapping administration structure
 */
typedef struct Lib_Mapping {
    Lib_Local_BoolResFunc       equal;
    Lib_Local_EltResFunc        hash;
    Int				log_table_size;
    Int				nrof_elements;
    PairList *			pairs;
    PairList  			insert_order_head;
    PairList  			insert_order_tail;
} *Lib_Mapping;




/*--------------------------------functions-----------------------------------*/


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
                  Int			   size);





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
Lib_Mapping_rehash( Lib_Mapping mapping, Int factor);




/*
 * Function         : copy an existing mapping or return Null if non existing
 * Parameters       : (I) map  - mapping to copy 
 * Function Result  : a mapping with the same contents as
 *                    'map' (at the moment of copying
 */

Lib_Mapping Lib_Mapping_copy( Lib_Mapping map );





/*
 * Function         : find the a mapping pair identified by 'domain'
 * Parameters       : (I) map   - a mapping
 *                    (I) domain - find the mapping pair identified by 
 *                    'domain'
 * Function Result  : function_result = map( domain) OR 
 *                    function_result = Null if 'domain' is not in the
 *                    mapping domain
 */

Pointer Lib_Mapping_apply ( Lib_Mapping map, Pointer  domain);





/*
 * Function         : get arbitrary element from the mapping's domain
 * Parameters       : (I) map      - mapping to get from 
 * Function Result  : an element from the mapping's domain, 
 *                    or Null when set empty
 */

Pointer 
Lib_Mapping_any_domain( Lib_Mapping map );





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
                             Pointer range);





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
                               Pointer  domain);





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
                            Pointer               data );

#define Lib_Mapping_traverse(map,f,data) \
               _Lib_Mapping_traverse(                         \
                        (map),                                \
                        (Lib_Local_TripleFunc)(f),            \
                        (Pointer)(data)                       \
                )





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
                                   Pointer               data );

#define Lib_Mapping_domain_traverse(map,f,data) \
               _Lib_Mapping_domain_traverse(                \
                        (map),                              \
                        (Lib_Local_PairFunc)(f),            \
                        (Pointer)(data)                       \
                )





/*
 * Function         : perform function 'f' on all range elements of the 
 *                    mapping, with additional data argument.
 * Parameters       : (I) map  - a mapping
 *                    (I) f    - function for processing range elemants 
 *                    (I) data - additional argument for 'f'
 * Function Result  : -
 * Sideeffects      : f has been applied to each range element, 
 *                    in unspecified but reproducible order.
 */

void _Lib_Mapping_range_traverse( Lib_Mapping         map, 
                                   Lib_Local_PairFunc  f,
                                   Pointer               data );

#define Lib_Mapping_range_traverse(map,f,data) \
               _Lib_Mapping_range_traverse(                 \
                        (map),                              \
                        (Lib_Local_PairFunc)(f),            \
                        (Pointer)(data)                       \
                )





/*
 * Function         : remove all pairs from the mapping,
 *                    but do not delete it
 * Parameters       : (O) map is a mapping
 * Function Result  : none
 */

void Lib_Mapping_empty(Lib_Mapping map);



/*
 * Function         : delete a mapping, free the occupied memory
 * Parameters       : (O) map is a mapping
 * Function Result  : none
 */

void Lib_Mapping_delete( Lib_Mapping map);



/*
 * Function         : get size of mapping
 * Parameters       : map    (I) mapping to obtain size from
 * Function Result  : nr of mapping pairs contained by the mapping
 */

Int Lib_Mapping_size( Lib_Mapping map);



#endif /* Lib_Mapping_INCLUDED */
     
   
