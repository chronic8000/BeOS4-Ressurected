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
 *  Module name              : Scatter_Shuffle.h    1.3
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 00:05:33 - 98/04/17
 *
 *  Description              :  
 *
 */

#ifndef  Scatter_Shuffle_INCLUDED
#define  Scatter_Shuffle_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"
#include "Lib_Mapping.h"


/*--------------------------- Functions --------------------------------------*/

/*
 * Function         : Convert a scatter which identifies
 *                    a word in *unshuffled* TM1 code, into an
 *                    equivalent scatter for the same code
 *                    after shuffling.
 * Parameters       : scatter   (I) input scatter
 *                    offset    (I) 'location' of the scattered
 *                                  word in the (unshuffled) code
 *                    memo      (IO) pointer to mapping for memoising
 *                                   shuffle results
 * Function Result  : shuffled scatter.
 *                    NB: this function will return earlier results
 *                        when called again with the same inputs
 */

TMObj_Scatter
    ScatShuf_shuffle( 
            TMObj_Scatter  scatter, 
            TMObj_Word     offset,
            Lib_Mapping   *memo 
    );



/*
 * Function         : Convert a scatter which identifies
 *                    a word in *shuffled* TM1 code, into an
 *                    equivalent scatter for the same code
 *                    after unshuffling.
 * Parameters       : scatter   (I) input scatter
 *                    offset    (I) 'location' of the scattered
 *                                  word in the (unshuffled) code
 *                    memo      (IO) pointer to mapping for memoising
 *                                   unshuffle results
 * Function Result  : shuffled scatter.
 *                    NB: this function will return earlier results
 *                        when called again with the same inputs
 */

TMObj_Scatter
    ScatShuf_unshuffle( 
            TMObj_Scatter  scatter, 
            TMObj_Word     offset,
            Lib_Mapping   *memo 
    );



#endif /* Scatter_Shuffle_INCLUDED */
     
   
