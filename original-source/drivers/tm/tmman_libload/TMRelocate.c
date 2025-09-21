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
 *  Module name              : TMRelocate.c    1.5
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : 
 *
 *  Last update              : 20:08:56 - 97/02/24
 *
 *  Description              :  
 *
 */

/*----------------------------includes----------------------------------------*/

#include "TMRelocate.h"
#include "TMScatter.h"
#include "Lib_Local.h"


/*----------------------------functions---------------------------------------*/

    



#define CURRENT_MARKER_VALUE(section,offset) \
		((Pointer)(offset) )

#define TARGET_MARKER_VALUE(section,offset) \
		((Pointer)( ((Int)section->relocation_offset) \
                        - ((Int)section->bytes) 		    \
                        + (offset) 					    \
                )       )		




TMObj_Word TMRelocate_symbol_value( TMObj_Symbol sym )
{
    switch (sym->type) {
    case TMObj_CommonSymbol:
    case TMObj_UnresolvedSymbol:
              return Null;

    case TMObj_AbsoluteSymbol:
              return (TMObj_Word)sym->attr.value;

    case TMObj_RelativeSymbol:
		  return
		      (TMObj_Word)
			 TARGET_MARKER_VALUE(
				sym->attr.marker.section,
				sym->attr.marker.offset
			 );
    }
}


Bool TMRelocate_symbol_ref( TMObj_Symbol_Reference ref )
{
    switch (ref->value->type) {

    case TMObj_AbsoluteSymbol:
    case TMObj_RelativeSymbol:
        TMScatter_put_word( 
	      CURRENT_MARKER_VALUE(ref->dest.section,ref->dest.offset), 
              ref->scatter,
              ref->dest.section->endian, 
              TMRelocate_symbol_value(ref->value)+ref->offset
        );  
        return True;

    case TMObj_CommonSymbol:
    case TMObj_UnresolvedSymbol:
    default:
	return False;
    } 
          
}


void TMRelocate_marker_ref( TMObj_Marker_Reference ref )
{
        TMScatter_put_word( 
	      CURRENT_MARKER_VALUE(ref->dest.section,ref->dest.offset), 
              ref->scatter,
              ref->dest.section->endian, 
              (TMObj_Word)TARGET_MARKER_VALUE(ref->value.section,ref->value.offset)
        );    
}

void TMRelocate_fromdefault_marker_ref(
			TMObj_FromDefault_Reference  ref,
			TMObj_Section                def
       )
{
        TMScatter_put_word( 
	      CURRENT_MARKER_VALUE(def,ref->dest.offset), 
              ref->scatter,
              def->endian, 
              (TMObj_Word)TARGET_MARKER_VALUE(ref->value.section,ref->value.offset)
        );    
}


void TMRelocate_defaulttodefault_marker_ref(
			TMObj_DefaultToDefault_Reference ref,
			TMObj_Section                    def
      )
{
        TMScatter_put_word( 
	      CURRENT_MARKER_VALUE(def,ref->dest.offset), 
              ref->scatter,
              def->endian, 
              (TMObj_Word)TARGET_MARKER_VALUE(def,ref->value.offset)
        );    
}






     
   
