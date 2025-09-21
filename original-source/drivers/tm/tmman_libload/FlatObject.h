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
 *  Module name              : FlatObject.h    1.18
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 14:42:37 - 98/03/25
 *
 *  Description              :  
 *
 */

#ifndef  FlatObject_INCLUDED
#define  FlatObject_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"
#include "Lib_IODrivers.h"


/*--------------------------- Definitions ------------------------------------*/


typedef struct FlatObject_Module *FlatObject_Module;

typedef void (*FlatObject_Section_Fun) (TMObj_Section, Pointer);

typedef void (*FlatObject_SectionLoaded_Fun) (FlatObject_Module, TMObj_Section, Pointer);



/*--------------------------- Functions --------------------------------------*/


/*
 * Function         : Create a data structure for opening an object.
 * Parameters       : object         (IO)  IO driver by which the object's
 *                                         contents can be accessed.
 *                    dataless_base  (I)   artificial bytes address for
 *                                         data-less sections. When having
 *                                         multiple opened objects around,
 *                                         this separates the memory spaces
 *                                         of dataless sections.
 *                    encode_sysrefs (I)   when True, all markers
 *                                         which point into system
 *                                         sections will be encoded
 *                                         using the SSID encoding
 *                    unshuffle_dynamic_scatters
 *                                   (I)   unshuffle scatters of dynamic loading
 *                                         life. these scatteres are normally 
 *                                         shuffled, because they are used *after*
 *                                         the object has beed loaded and bit-shuffled.
 *                                         this in contrast to normal scatters, which
 *                                         are used during loading, *before* the object
 *                                         image has been bit-shuffled.
 *                    section_loaded (I)   Function which gets called foreach
 *                                         section which has been loaded.
 *                    data           (I)   Additional argument for section_loaded
 *                    streamsize     (I)   Size of buffer reserved for 
 *                                         streaming the references during
 *                                         relocation. When equal to zero,
 *                                         no streaming will be performed, and
 *                                         the StreamLoad sections will be 
 *                                         loaded and left in memory like the
 *                                         other sections.
 * Function Result  : Iff. the memory image contained
 *                    a valid object module, and hence opening
 *                    succeeded: a handle to be used in subsequent 
 *                    operations; Null otherwise
 */
FlatObject_Module
    FlatObject_open_object( 
            Lib_IODriver                     object,
            Address                          dataless_base,
            Bool                             encode_sysrefs,
            Bool                             unshuffle_dynamic_scatters,
            FlatObject_SectionLoaded_Fun     section_loaded,
            Pointer                          data,
            Int                              streamsize
    );



/*
 * Function         : Close an earlier opened object.
 *                    Closing only invalidates the FlatObject
 *                    administration; all object data which has
 *                    been read from file, and which has been 
 *                    unpacked, stays valid. This data may have
 *                    been used in the construction of another
 *                    object, and has to be deleted by other means.
 * Parameters       : object (I)  object to close
 * Function Result  : -
 */
void
    FlatObject_close_object( 
            FlatObject_Module   module
    );



/*
 * Function         : Get (address of) a module's header. 
 * Parameters       : module  (I)  module to get header from
 * Function Result  : (Address of) the module header
 */
TMObj_Module
    FlatObject_get_header( FlatObject_Module module );



/*
 * Function         : Lookup a user section by name
 *                    in an (opened) module.
 * Parameters       : module  (I)  module to look in
 *                    name    (I)  name of requested section
 * Function Result  : Requested section, or NULL when not found.
 */
TMObj_Section
    FlatObject_get_section( 
            FlatObject_Module   module,
            String              name 
    );


/*
 * Function         : Lookup a global or dynamic symbol by name
 *                    in an (opened) module.
 * Parameters       : module  (I)  module to look in
 *                    name    (I)  name of requested symbol
 * Function Result  : Requested symbol, or NULL when not found.
 * Note             : Search is linear.
 */
TMObj_Symbol
    FlatObject_get_global_symbol( 
            FlatObject_Module  module,
            String             name 
    );


/*
 * Function         : Lookup a system section by its
 *                    attributes in an (opened) module.
 * Parameters       : module   (I)  module to look in
 *                    id       (I)  type of system section
 *                    lifetime (I)  lifetime of system section
 * Function Result  : Requested section, or NULL when not found.
 */
TMObj_Section
    FlatObject_get_system_section( 
            FlatObject_Module         module,
            TMObj_System_Section_Id   id,
            TMObj_Section_Lifetime    life
    );



/*
 * Function         : Apply the specified function to all internal sections 
 *                    in the specified module, in the specified lifetime
 *                    range, and with specified additional argument.
 *                    NB: both user and system sections are traversed.
 * Parameters       : module           (I)  module to traverse
 *                    section_fun      (I)  function to apply
 *                    start            (I)  start of lifetime range to traverse
 *                    end              (I)  end of lifetime range to traverse
 *                    data             (I)  additional argument to section_fun
 * Function Result  : -
 */
void
    FlatObject_traverse_sections (
            FlatObject_Module         module,
            FlatObject_Section_Fun    section_fun,
            TMObj_Section_Lifetime    start,
            TMObj_Section_Lifetime    end,
            Pointer                   data
    );



/*
 * Function         : Same as FlatObject_traverse_sections; but
 *                    traverses section table only;  
 *                    does ->not<- load the section contents
 */
void
    FlatObject_traverse_section_info (
            FlatObject_Module         module,
            FlatObject_Section_Fun    section_fun,
            TMObj_Section_Lifetime    start,
            TMObj_Section_Lifetime    end,
            Pointer                   data
    );



#endif /* FlatObject_INCLUDED */
     
   
