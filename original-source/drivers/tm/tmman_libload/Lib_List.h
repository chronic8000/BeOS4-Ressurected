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
 *  Module name              : Lib_List.h    1.9
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : generic list functions library
 *
 *  Last update              : 13:43:56 - 97/03/19
 *
 *  Description              : List functions library 
 *
 *
 * This unit defines a generic List implementation. Contrary to the more high
 * level abstractions of sets, bags, mappings, this list definition is *not*
 * an abstract data type; its representation is completely open, and the client
 * of this unit may freely 'link around' list blocks, and also is responsible 
 * for aliasing- and circularity problems at deletion, etc.
 *
 * This unit provides an implementation of Lists with
 * yet unspecified Element types. No assumptions or restrictions
 * are made on these types, except for their representation sizes: these must
 * be equal to the representation size of pointers. 
 *
 * This unit provides concatenation functions, a generic sort function, 
 * and a traversal function that enumerates all elements of a List while  
 * applying a specified traversal function to each of them.
 *
 * An often used Element type is Int. This type is commonly considered
 * to be compatible with pointer types in "C", although this has not been
 * specified in the language definition.
 * This unit allows Int to occur as Element, but performs a check on
 * pointer compatibility in the initialisation. In case this check fails, 
 * this unit has to be recoded to allow Int as Element.
 *
 */

#ifndef  Lib_List_INCLUDED
#define  Lib_List_INCLUDED


/*----------------------------includes---------------------------------------*/

#include "Lib_Local.h"


/*----------------------------definitions-------------------------------------*/


/*
 * definition of the list element structure,
 * NB: the link field 'tail' is at the beginning,
 * to make use of the list allocation possibilities
 * of the memory manager:
 */
typedef struct Lib_List *Lib_List;

struct Lib_List {
    Lib_List  tail;
    Pointer     head;
};




/*----------------------------functions---------------------------------------*/

/*
 * Function         : retrieve i'th element from list
 * Parameters       : list  (I)  list to get from
 *                    index (I)  nr of element to retrieve 
 *                                (first element has index 1);
 * Function Result  : index <= size(list)   ==> list@index 
 *                    index >  size(list)   ==> Null
 */

Lib_List Lib_List_index( Lib_List list, Int index );





/*
 * Function         : copy list (NB: but NOT it's elements)
 * Parameters       : list (I)  list to copy
 * Function Result  : copied list
 */

Lib_List Lib_List_copy( Lib_List list );





/*
 * Function         : prepend an element in front of a list
 * Parameters       : element (I)  - element to be prepended
 *                    list    (I)  - list to prepend to
 * Function Result  : function_result->head == element AND 
 *                    function_result->tail == list
 */

Lib_List Lib_List_cons( Pointer element, Lib_List list );





/*
 * Function         : concatenate list r at the end of list l
 * Parameters       : l (IO) address a list
 *                    r (I)  the list to concatenate 
 * Function Result  : none
 * Precondition     : the lists should not have a common tail; this avoids
 *                    list cycles
 * Postcondition    : list r is concatenated at the end of list l 
 * Sideeffects      : list l is modified
 */

void Lib_List_concat( Lib_List *l, Lib_List r);




/*
 * Function         : free a single list block; same as Mem_free
 * Parameters       : l (IO) block to delete
 * Function Result  : none
 */

void Lib_List_free( Lib_List l );




/*
 * Function         : sort list l using Element ordering defined by less_eq
 * Parameters       : (IO) l         - 
 *  		      (I)  less_eq - "<=" total ordering on the list's elements 
 * Function Result  : -
 */

void Lib_List_sort( Lib_List *l, Lib_Local_BoolResFunc less_eq);



/*
 * Function         : apply function 'f' to all elements of the list,
 *                    in the order of occurrence in the list
 * Parameters       : (I) l    - a list
 *                    (I) f    - function to be applied to the list's elements 
 *                    (I) data - additional data to be supplied to 'f'
 * Function Result  : -
 */

void _Lib_List_traverse( Lib_List              l, 
                         Lib_Local_PairFunc    f,
                         Pointer                 data);

#define Lib_List_traverse(l,f,data) \
               _Lib_List_traverse(                    \
                        (l),                          \
                        (Lib_Local_PairFunc)(f),      \
                        (Pointer)(data)                 \
                )



/*
 * Function         : delete all blocks of a list, and apply a
 *                    procedure (e.g. an element deletion routine) to
 *                    all of its elements, in order of occurrence in
 *                    the list.
 * Parameters       : (IO) l      - a list
 *                    (I ) delete - function for e.g. deleting list elements
 *                    (I ) data   - additional data to be supplied to 'delete'
 * Function Result  : none
 * Sideeffects      : delete has been applied to each list element
 *                    in the order of occurrence in the list, and the list
 *                    itself has been deallocated.
 */

void _Lib_List_delete( Lib_List              l, 
                       Lib_Local_PairFunc    delete,
                       Pointer                 data );

#define Lib_List_delete(l,delete,data) \
               _Lib_List_delete(                               \
                        (l),                                   \
                        (Lib_Local_PairFunc)(delete),          \
                        (Pointer)(data)                          \
                )



/*
 * Function         : return length of list
 * Parameters       : (I) l - a list
 * Function Result  : number of elements in list
 */

Int Lib_List_size( Lib_List l );



#endif /* Lib_List_INCLUDED */
     
   
