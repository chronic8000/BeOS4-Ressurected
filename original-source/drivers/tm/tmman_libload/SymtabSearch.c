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
 *  Module name              : SymtabSearch.c    1.20
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    :   
 *
 *  Last update              : 17:18:40 - 97/04/02
 *
 *  Description              : 
 */

#include "Lib_Local.h"
#include "SymtabSearch.h"
//#include <stdlib.h>

void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size,
  	int  (*compar)(const void *, const void *));

typedef int (*CmpFunc)(const void *, const void *);

struct SymtabSearch {
	Int				nrof_addr_symbols;
	Int				nrof_name_symbols;
	SymtabSearch_Element		symrefs;
	SymtabSearch_Element		*addr_search;
	SymtabSearch_Element		*name_search;
} ;


static SymtabSearch
alloc_arrays( Int nelm )
{
	SymtabSearch	ss;
	
	ss     = (SymtabSearch) Lib_Mem_MALLOC(sizeof(struct SymtabSearch));
	if (ss == Null)
		return Null;
		
	ss->nrof_name_symbols = nelm;
	ss->nrof_addr_symbols = nelm;
	ss->addr_search  = (SymtabSearch_Element *)Lib_Mem_MALLOC(nelm * sizeof(SymtabSearch_Element));
	if ( ss->addr_search == Null ) {
		Lib_Mem_FREE(ss);
		return Null;
	}

	ss->name_search  = (SymtabSearch_Element *)Lib_Mem_MALLOC(nelm * sizeof(SymtabSearch_Element));
	if ( ss->name_search == Null ) {
		Lib_Mem_FREE(ss->addr_search);
		Lib_Mem_FREE(ss);
		return Null;
	}
	
	return ss;
}

static Int
addr_compar(SymtabSearch_Element *sym1, SymtabSearch_Element *sym2)
{
	if ( (UInt)((*sym1)->address) < (UInt)((*sym2)->address) )
		return -1;
	else if ( (UInt)((*sym1)->address) > (UInt)((*sym2)->address) )
		return 1;
	else 	return 0;
}


static Int
name_compar(SymtabSearch_Element *sym1, SymtabSearch_Element *sym2)
{
	return strcmp((*sym1)->name, (*sym2)->name);
}


/*
 * EXTERNAL
 * 
 * build a table
 */
extern SymtabSearch
SymtabSearch_create( Int nelm, SymtabSearch_Element symrefstore )
{
	Int		i, j;
	SymtabSearch	ss;
	
	ss = alloc_arrays(nelm);
	if (ss == Null)
		return Null;
	
	if ( nelm == 0 )
		return ss;

	ss->symrefs = symrefstore;
	for (i = 0; i < nelm; i++) {
		
		ss->addr_search[i] = &ss->symrefs[i];
		ss->name_search[i] = &ss->symrefs[i];
	}
	
	qsort(ss->addr_search, ss->nrof_addr_symbols, sizeof(SymtabSearch_Element), (CmpFunc)addr_compar);
	qsort(ss->name_search, ss->nrof_name_symbols, sizeof(SymtabSearch_Element), (CmpFunc)name_compar);
	
	
	/*
	 * Delete for multiple symbols to the same address all
	 * but the one with the shortest name
	 */
	j = 0;
	for ( i = 1; i < ss->nrof_addr_symbols; i++ ) {
		if ( ss->addr_search[i]->address != ss->addr_search[j]->address ) {
			ss->addr_search[++j] = ss->addr_search[i];
		}
		else if ( strlen(ss->addr_search[i]->name) <  strlen(ss->addr_search[j]->name) ) {
			ss->addr_search[j] = ss->addr_search[i];
		}
	}
	ss->nrof_addr_symbols = j + 1;
	
	return ss;
}



static Int
addr_search_compar(SymtabSearch_Element tobefound, SymtabSearch_Element *sym)
{
	SymtabSearch_Element	*next;
	
	next = sym + 1;
		/* 
		 * Do not access next outside of sym->address[sym->nrof_addr_symbols -1]
		 * This is guaranteed by the exception handling in SymtabSearch_get_from_address
		 * and the first test in the code below
		 */
	if ( (UInt)tobefound->address < (UInt)(*sym)->address )
		return -1;
	else if ( (UInt)tobefound->address >= (UInt)(*next)->address )
		return 1;
	else	return 0;
}

/*
 * EXTERNAL
 * 
 */
extern SymtabSearch_Element
SymtabSearch_get_from_address( SymtabSearch ss, Address addr )
{
	SymtabSearch_Element	   *rval;
	struct SymtabSearch_Element elm;
		/*
		 * exception handling 
		 */
	if ( ss->nrof_addr_symbols == 0 )
		return Null;
	if ( ss->nrof_addr_symbols == 1 )
		return ss->addr_search[0];
	if ( (UInt)addr < (UInt)ss->addr_search[0]->address )
		return Null;
	if ( (UInt)addr >= (UInt)ss->addr_search[ss->nrof_addr_symbols - 1]->address )
		return ss->addr_search[ss->nrof_addr_symbols - 1];

	elm.address = addr;
	rval = bsearch(	&elm, 
			ss->addr_search,
			ss->nrof_addr_symbols, 
			sizeof(SymtabSearch_Element),
			(CmpFunc)addr_search_compar);

	return (rval) ? *rval : Null;
}


static Int
name_search_compar(SymtabSearch_Element tobefound, SymtabSearch_Element (*sym))
{
	return strcmp(tobefound->name, (*sym)->name);
}



/*
 * EXTERNAL
 * 
 */
SymtabSearch_Element
SymtabSearch_get_from_name(SymtabSearch ss,  String name )
{
	SymtabSearch_Element	   *rval;
	struct SymtabSearch_Element elm;

	elm.name = name;
	rval = bsearch(	&elm, 
			ss->name_search,
			ss->nrof_name_symbols, 
			sizeof(SymtabSearch_Element),
			(CmpFunc)name_search_compar);

	return (rval) ? *rval : Null;
}


/*
 * EXTERNAL
 * 
 */
extern void
SymtabSearch_destroy(SymtabSearch ss)
{
	Lib_Mem_FREE(ss->addr_search);
	Lib_Mem_FREE(ss->name_search);
	Lib_Mem_FREE(ss);
}



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
    )
{
    Int i;
    /* to remove warning (might be used unitialized) NOT CHECKED */
    SymtabSearch_Element *symrefs = NULL;

    switch (order) {
    case TMDwnLdr_ByAddress : symrefs= ss->addr_search; break;
    case TMDwnLdr_ByName    : symrefs= ss->name_search; break;
    }

    for (i= 0; i<ss->nrof_name_symbols; i++) {
        SymtabSearch_Element cur_sym= symrefs[i];

        fun( 
          cur_sym->section,
          cur_sym->name, 
          cur_sym->address, 
          cur_sym->type, 
          cur_sym->scope, 
          data 
        );
    }
}


