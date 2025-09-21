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
 *  Module name              : ObjRepr.h    1.10
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Representations of objectfile related items
 *
 *  Last update              : 23:31:40 - 98/04/08
 *
 *  Description              :  
 *
 *                         This module defines a number of textual
 *                         representations, to be used by object file
 *                         dumpers and for getting the names of the
 *                         various system sections.
 */

#ifndef  ObjRepr_INCLUDED
#define  ObjRepr_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"


/*--------------------------- Functions --------------------------------------*/

/*
 * The next list defines representation arrays
 * for all enum types in TMObj.h. They can 
 * simply be indexed using the enum constants:
 */

extern const String ObjRepr_Endian            [];  
extern const String ObjRepr_Module_Type       [];  
extern const String ObjRepr_Binding_Time      [];  
extern const String ObjRepr_Symbol_Scope      [];  
extern const String ObjRepr_Symbol_Type       [];  
extern const String ObjRepr_Section_Lifetime  [];  
extern const String ObjRepr_System_Section_Id [];  
extern const String ObjRepr_Caching_Property  [];  


/*
 * Function         : Generate name of system section of
 *                    specified type and lifetime into
 *                    specified character buffer
 * Parameters       : buffer   (O)  buffer to construct name into
 *                                  (capacity > 50)
 *                    id       (I)  system section type
 *                    lifetime (I)  system section lifetime
 * Function Result  : -
 */
void ObjRepr_system_section_name(
             Char                    * buffer,
             TMObj_System_Section_Id   id,
             TMObj_Section_Lifetime    lifetime
     );



#endif /* ObjRepr_INCLUDED */
     
   
