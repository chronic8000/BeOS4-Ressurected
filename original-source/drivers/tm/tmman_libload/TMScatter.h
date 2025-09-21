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
 *  Module name              : TMScatter.h    1.9
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Scatter operations
 *
 *  Last update              : 08:32:28 - 97/03/19
 *
 *  Description              : This module defines operations 
 *                             on scatter descriptions.
 *
 */

#ifndef  TMScatter_INCLUDED
#define  TMScatter_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"


/*--------------------------- Functions --------------------------------------*/




/*
 * Function         : Get number of scatter units in scatter,  
 *                    i.e. the number of elements in its source
 *                    and dest arrays.
 * Parameters       : scatter (I)  scatter description to inspect
 * Function Result  : number of scatter units
 */

Int
    TMScatter_length( 
               TMObj_Scatter    scatter
    );




/*
 * Function         : Get the number of elements in the 
 *                    scatter source array
 * Parameters       : scatter_source (I)  scatter source description to inspect
 * Function Result  : number of scatter units
 */

Int
    TMScatter_source_length( 
               TMObj_Scatter_Source    scatter_source
    );




/*
 * Function         : Get scattered word from byte sequence
 * Parameters       : bytes   (I)  address into byte string which
 *                                 is the origin of all the offsets
 *                                 in the scatter (offsets in scatter
 *                                 might be less than zero)
 *                    scatter (I)  scatter description of word to retrieve,
 *                                 or Null. In case this parameter is
 *                                 equal to Null, the word is read
 *                                 according to the next parameter
 *                    endian  (I)  specification of endianness according
 *                                 to which the word must be read in case
 *                                 the scatter descriptor is Null
 * Function Result  : Retrieved word
 */

TMObj_Word
    TMScatter_get_word( 
               Address          bytes, 
               TMObj_Scatter    scatter,
               Endian           endian
    );




/*
 * Function         : Put scattered word into byte sequence
 * Parameters       : bytes   (I)  address into byte string which
 *                                 is the origin of all the offsets
 *                                 in the scatter (offsets in scatter
 *                                 might be less than zero)
 *                    scatter (I)  scatter description of word to be put,
 *                                 or Null. In case this parameter is
 *                                 equal to Null, the word is put
 *                                 according to the next parameter
 *                    endian  (I)  specification of endianness according
 *                                 to which the word must be put in case
 *                                 the scatter descriptor is Null
 *                    word    (I)  word to put
 * Function Result  : -
 */

void
    TMScatter_put_word( 
	       Address 		bytes, 
               TMObj_Scatter 	scatter,
               Endian           endian, 
               TMObj_Word      	word
    );




#define TMScatter_resolve_marker_reference( ref ) \
                TMScatter_put_word(				\
                    (Address)ref->dest.offset,			\
                    ref->scatter,				\
                    ref->dest.section->attr.internal.endian,	\
		    (Address)ref->value.offset			\
                ) 


#endif /* TMScatter_INCLUDED */
     
   
