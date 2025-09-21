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
 *  Module name              : ObjCreate.h    1.30
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Object Data Creation Library
 *
 *  Last update              : 23:31:45 - 98/04/08
 *
 *  Description              :  
 *
 *         This module exports a list of trivial constructor functions,
 *         for creating object elements with contents as defined in
 *         TMObj.h. Created elements do not (yet) have any relation
 *         with modules, and are non-unique. That is, each call returns
 *         a new object.
 */

#ifndef  ObjCreate_INCLUDED
#define  ObjCreate_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"


/*--------------------------- Functions --------------------------------------*/


TMObj_Symbol
    ObjCreate_unresolved_symbol( 
         String                   name,
         TMObj_Symbol_Scope       scope,
         Bool                     is_download_parameter
    );

TMObj_Symbol
    ObjCreate_absolute_symbol( 
         String                   name,
         TMObj_Symbol_Scope       scope,
	 TMObj_Word               value
    );

TMObj_Symbol
    ObjCreate_common_symbol( 
         String                   name,
         TMObj_Symbol_Scope       scope,
	 UInt                     length,
	 UInt                     alignment
    );

TMObj_Symbol
    ObjCreate_relative_symbol( 
         String                   name,
         TMObj_Symbol_Scope       scope,
         TMObj_Marker_Rec         marker
    );
 
TMObj_Symbol
    ObjCreate_external_symbol( 
         String                   name,
         TMObj_Symbol_Scope       scope,
         TMObj_External_Module    module,
         Int                      jtab_index
    );
 
TMObj_Section
    ObjCreate_section( 
         String                   name,
	 UInt                     alignment, 
         Endian                   endian,  
         Bool                     has_data,        
         TMObj_Section_Lifetime   lifetime,
         Bool                     is_system,
         Bool                     is_code,
         TMObj_Caching_Property   caching_property,
         Bool                     is_read_only
    );
 
TMObj_External_Module
    ObjCreate_external_module( 
         String                   name,
         TMObj_Binding_Time       binding_time,
         Int       		  checksum,     
         Int                      nrof_exported_symbols 
    );

TMObj_SectionUnit
    ObjCreate_sectionunit( 
         TMObj_Marker_Rec      	  marker,
	 UInt              	  length,
         UInt                     alignment,
         Bool                     is_root  
    );

TMObj_Symbol_Reference
    ObjCreate_symbol_reference( 
         TMObj_Scatter            scatter,
         TMObj_Marker_Rec   	  dest,
         TMObj_Symbol    	  value,
         Int32  		  offset 
    );

TMObj_Marker_Reference
    ObjCreate_marker_reference( 
         TMObj_Scatter   	  scatter,
         TMObj_Marker_Rec   	  dest,
         TMObj_Marker_Rec   	  value
    );

TMObj_JTab_Reference
    ObjCreate_jtab_reference( 
         TMObj_Scatter   	  scatter,
         TMObj_Marker_Rec   	  dest,
         TMObj_External_Module    module,
         Int32   	          jtab_offset,
	 Int32       	          offset 
    );

TMObj_Marker_Rec
    ObjCreate_marker( 
         TMObj_Section            section,
	 UInt32       	          offset 
    );


#define ObjCreate_convert_fromdefault_reference(defsect,ref) \
                ObjCreate_marker_reference(			\
                        (ref)->scatter,				\
                        ObjCreate_marker(			\
                            defsect,				\
                            (ref)->dest.offset			\
                        ), 					\
		        (ref)->value				\
      		) 						\

#define ObjCreate_convert_defaulttodefault_reference(defsect,ref) \
                ObjCreate_marker_reference(			\
                        (ref)->scatter,				\
                        ObjCreate_marker(			\
                            defsect,				\
                            (ref)->dest.offset			\
                        ),					\
                        ObjCreate_marker(			\
                            defsect,				\
                            (ref)->value.offset			\
                )       ) 					\

#endif /* ObjCreate_INCLUDED */
     
   
