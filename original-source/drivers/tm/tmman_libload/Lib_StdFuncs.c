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
 *  Module name              : Lib_StdFuncs.c    1.7
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : Standard traversal function definitions
 *
 *  Last update              : 08:42:56 - 97/03/19
 *
 *  Description              :  This module defines several standard
 *                              functions, mostly having to do with
 *                              hashing and comparisons for the standard
 *                              types.
 *
 */

/*--------------------------------includes------------------------------------*/

#include "Lib_StdFuncs.h"

/*--------------------------------functions-----------------------------------*/

    
/*
 * Function         : hash functions for Integer, String and Pointer
 * Parameters       : val (I) value to be hashed
 * Function Result  : hashed value in the Integer range
 */

Int Lib_StdFuncs_int_hash( Int val )
{ 
    return _Lib_StdFuncs_int_hash(val);
}

Int Lib_StdFuncs_address_hash( Pointer val )
{ 
    return _Lib_StdFuncs_address_hash(val);
}

Int Lib_StdFuncs_string_hash( String str)
{
    Int i, g;
    Int result = 0;

    for (i = 0; str[i] != '\0'; i++) {
        result = (result << 4) + str[i];
        if ( (g = result&0xf0000000) ) {
            result = result ^ (g >> 24);
            result = result ^ g;
        }
    }

    return result;
}


/*
 * Function         : equality functions for Int, String and Pointer
 * Parameters       : l,r (I) values to be compared
 * Function Result  : True iff the contents of l and r are equal
 * Precondition     : none
 * Postcondition    : none 
 * Sideeffects      : none
 */

Bool Lib_StdFuncs_int_equal( Int l, Int r)
{ 
    return( l == r ); 
}

Bool Lib_StdFuncs_address_equal( Pointer l, Pointer r)
{ 
    return( l == r ); 
}

Bool Lib_StdFuncs_string_equal( String l, String r)
{ 
    return strcmp( l, r) == 0; 
}


/*
 * Function         : Less_Eq functions for Int, String and Pointer
 * Parameters       : l,r (I) values to be compared
 * Function Result  : True iff. l is less than or equal to r.
 */

Bool Lib_StdFuncs_int_less_eq( Int l, Int r)
{
    return l <= r;
}

Bool Lib_StdFuncs_address_less_eq( Pointer l, Pointer r)
{
    return l <= r;
}

Bool Lib_StdFuncs_string_less_eq( String l, String r)
{ 
    return( strcmp( l, r) <= 0); 
}



/*
 * Function         : empty function; does nothing
 * Parameters       : -
 * Function Result  : -
 */

void Lib_StdFuncs_null_func ( ){}
