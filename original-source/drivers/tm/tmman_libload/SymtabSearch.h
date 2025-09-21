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
 *  Module name              : SymtabSearch.h    1.10
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    :   
 *
 *  Last update              : 13:00:01 - 97/04/02
 *
 *  Description              : 
 */

#ifndef SymtabSearch_INCLUDED
#define SymtabSearch_INCLUDED

#include "tmtypes.h"
#include "TMDownLoader.h"

typedef struct SymtabSearch_Element {
	Address		       address;
	String		       section;
	String		       name;
        TMDwnLdr_Symbol_Type   type;
        TMDwnLdr_Symbol_Scope  scope;
} *SymtabSearch_Element;

typedef struct SymtabSearch	*SymtabSearch;





/*
 * Function         : create a new symtab search
 * Parameters       : nelm        (I) nrof elements
 *                    symrefstore (I) store of symrefs.
 * Function Result  : A handle to a new symtab search object.
 * Side Effects     : The symrefstore is exposed in this
 *                    object so it will use the data for all
 *                    subsequent calls to this interface with
 *                    handle return by this creation as input.
 */

extern SymtabSearch 
SymtabSearch_create( Int nelm, SymtabSearch_Element symrefstore );


/*
 * Function         : Search in a SymtabSearch object for the
 *                    address smaller than the given address
 * Parameters       : ss   (I) SymtabSearch object to search in
 *                    addr (I) address wanted.
 * Function Result  : Address smaller or equal to addr. NULL when
 *                    addr is smaller than the smallest or the
 *                    store is empty.
 */

extern SymtabSearch_Element
SymtabSearch_get_from_address(SymtabSearch ss, Address addr );

/*
 * Function         : Search in a SymtabSearch object for a
 *                    symbol
 * Parameters       : ss   (I) SymtabSearch object to search in
 *                    name (I) name of the symbol wanted.
 * Function Result  : The SymtabSearch_Element for which elm->name
 *                    equals name. NULL if not found.
 */

extern SymtabSearch_Element
SymtabSearch_get_from_name( SymtabSearch ss, String name );


/*
 * Function         : destropy the internal administration for a 
 *                    SymtabSearch object
 * Parameters       : ss   (I) SymtabSearch object to destroy
 * Function Result  : -
 */

extern void
SymtabSearch_destroy(SymtabSearch ss);




/*
 * Function         : apply function 'fun' to all symbols in the
 *                    specified SymtabSearch object, with additional 
 *                    data argument.
 * Parameters       : ss     (I)  SymtabSearch object to traverse
 *                    order  (I)  parameter to guide order of traversal
 *                    fun    (I)  function to apply
 *                    data   (I)  additional data argument
 * Function Result  : -
 * Sideeffects      : fun has been applied to each symbol, 
 *                    in the order specified by 'order'.
 */

extern void
    SymtabSearch_traverse_symbols(
         SymtabSearch                     ss,
         TMDwnLdr_Symbol_Traversal_Order  order,
         TMDwnLdr_Symbol_Fun              fun,
         Pointer                          data
    );






#endif
