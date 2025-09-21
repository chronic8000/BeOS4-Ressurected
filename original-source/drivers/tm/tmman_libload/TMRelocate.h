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
 *  Module name              : TMRelocate.h    1.5
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 08:32:30 - 97/03/19
 *
 *  Description              :  
 *
 */

#ifndef  TMRelocate_INCLUDED
#define  TMRelocate_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"


/*--------------------------- Functions --------------------------------------*/

/*
 * Function         : 
 * Parameters       : 
 * Function Result  : 
 */
TMObj_Word 
    TMRelocate_symbol_value( TMObj_Symbol sym );




/*
 * Function         : 
 * Parameters       : 
 * Function Result  : 
 */
Bool 
    TMRelocate_symbol_ref( TMObj_Symbol_Reference ref );




/*
 * Function         : 
 * Parameters       : 
 * Function Result  : 
 */
void 
    TMRelocate_marker_ref( TMObj_Marker_Reference ref );




/*
 * Function         : 
 * Parameters       : 
 * Function Result  : 
 */
void 
    TMRelocate_fromdefault_marker_ref(
			TMObj_FromDefault_Reference  ref,
			TMObj_Section                def
       );



/*
 * Function         : 
 * Parameters       : 
 * Function Result  : 
 */
void 
    TMRelocate_defaulttodefault_marker_ref(
			TMObj_DefaultToDefault_Reference ref,
			TMObj_Section                    def
      );




#endif /* TMRelocate_INCLUDED */
     
   
