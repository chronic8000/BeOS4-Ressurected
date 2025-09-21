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
 *  Module name              : TMScatter.c    1.13
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : Scatter operations
 *
 *  Last update              : 08:32:29 - 97/03/19
 *
 *  Description              : This module defines operations 
 *                             on scatter descriptions.
 *
 */

/*----------------------------includes----------------------------------------*/

#include "TMScatter.h"
#include "Lib_Local.h"


/*----------------------------functions---------------------------------------*/


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
    )
{
    return TMScatter_source_length(scatter->source);
}




/*
 * Function         : Get the number of elements in the 
 *                    scatter source array
 * Parameters       : scatter_source (I)  scatter source description to inspect
 * Function Result  : number of scatter units
 */

Int
    TMScatter_source_length( 
               TMObj_Scatter_Source    scatter_source
    )
{
    TMObj_Word           done   = 0;
    Int              result = 0;

    while (done != 0xffffffff) {
        done   |= scatter_source->mask;
        result++;
        scatter_source++;
    }

    return result;
}





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
    )
{
    if (scatter == Null) {
        if (endian == BigEndian) {
            return ( bytes[0] << 24)
                 | ( bytes[1] << 16)
                 | ( bytes[2] <<  8)
                 | ( bytes[3]      )
                 ; 
        } else {
            return ( bytes[3] << 24)
                 | ( bytes[2] << 16)
                 | ( bytes[1] <<  8)
                 | ( bytes[0]      )
                 ; 
        }
    } else {
        TMObj_Word bits_done= 0;
        TMObj_Word result   = 0;
	Int i;

	for (i=0; bits_done != 0xffffffff; i++) {
	    UInt8*     ptr   = (UInt8*)(((UInt)bytes) + scatter->dest[i].offset); 
	    Int        shift = scatter->dest[i].shift;
	    TMObj_Word mask  = scatter->source[i].mask; 
    
	    bits_done  |= mask;
    
	    if (shift < 0) {
		result |= ((*ptr) << -shift) & mask;
	    } else {
		result |= ((*ptr) >>  shift) & mask; 
	    }
	}	

        return result;
    }
}




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
    )
{
    if (scatter == Null) {
        if (endian == BigEndian) {
            bytes[0]= word >> 24;
            bytes[1]= word >> 16;
            bytes[2]= word >>  8;
            bytes[3]= word;
        } else {
            bytes[3]= word >> 24;
            bytes[2]= word >> 16;
            bytes[1]= word >>  8;
            bytes[0]= word;
        }
    } else {
	Int i;
	TMObj_Word bits_done;

        bits_done= 0;

	for (i=0; bits_done != 0xffffffff; i++) {
	    UInt8*     ptr   = (UInt8*)(((UInt)bytes) + scatter->dest[i].offset);
	    Int        shift = scatter->dest[i].shift;
	    TMObj_Word mask  = scatter->source[i].mask;

	    bits_done  |= mask;

	    if (shift < 0) {
		(*ptr) = ((word&mask) >> -shift) | ((*ptr) & ~(mask >> -shift) ); 
	    } else {
		(*ptr) = ((word&mask) <<  shift) | ((*ptr) & ~(mask <<  shift) ); 
	    }
	}
    }
}







     
   
