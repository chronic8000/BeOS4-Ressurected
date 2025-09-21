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
 *  Module name              : ModuleBuild.h    1.45
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 23:31:42 - 98/04/08
 *
 *  Description              :  
 *
 * This module defines the facilities for manipulating Trimedia Objest files.
 * The idea is that the client of this module creates an empty Object, and 
 * fills it with the basic data elements of which an Object consists while
 * treating these elements as in-memory data structures which refer to each 
 * other via pointers. Once an Object has beed defined, a memory- or file 
 * image can be constructed by means of functions defined by this object.
 * These functions have all the responsibility of packing the data into
 * a consecutive memory region, pointer conversion and endianness conversion
 * as defined in the module header. Moreover, making use of optimised 
 * representations (e.g. DefaultToDefault_References, or partitioning
 * of the different system sections according to lifetime, is taken care
 * of by the image constructor: no facilities are present for letting 
 * the client tamper with these. 
 * 
 * Conversely, a read function/memory image unpacker is defined which
 * takes care of unpacking an object, taking care of conversions necessary
 * according to the stored endiandess and of restoring the pointers so
 * that the object can again be treated as an in-memory data structure.
 * 
 * The basic elements which are to be added to a module are:
 * 
 *      Symbol_Reference   
 *      Marker_Reference   
 *      Section           
 *      Symbol
 *      SectionUnit       (A data fragment of a section)
 * 
 * MEMORY MANAGEMENT:
 * 
 *      Since the environments in which this module is expected
 *      to be used are linker, assembler and other compiler tools,
 *      no explicit precautions are taken against memory overflow.
 *      This is expected to be solved on a general level by the 
 *      memory manager against which this is linked. Also, this module
 *      expects that all memory which is added to object modules is 
 *      further 'owned' by this module, and hence never deallocated
 *      or reused by the client. However, 'specific' properties may
 *      be altered by the client (information on a per-item basis 
 *      available on request).
 */

#ifndef  ModuleBuild_INCLUDED
#define  ModuleBuild_INCLUDED


/*------------------------------- Includes -----------------------------------*/

#include "TMObj.h"
#include "Lib_List.h"
#include "Lib_Set.h"


/*----------------------------- Type Definitions -----------------------------*/

/*
 * Opaque type for  modules 
 * under construction. Although the
 * internal representation is not public,
 * it has been defined in a separate header
 * file to allow the external ModBld interfacd
 * to be implemented by several internal C 
 * modules.
 */
typedef struct ModBuildRepr_Module_Rec  *ModBld_Module;


/*
 * Traversal function types:
 */
typedef void (*ModBld_Symbol_ReferenceFun)    ( ModBld_Module, TMObj_Symbol_Reference,    Pointer );
typedef void (*ModBld_Marker_ReferenceFun)    ( ModBld_Module, TMObj_Marker_Reference,    Pointer );
typedef void (*ModBld_JTab_ReferenceFun  )    ( ModBld_Module, TMObj_JTab_Reference,      Pointer );
typedef void (*ModBld_SectionFun         )    ( ModBld_Module, TMObj_Section,             Pointer );
typedef void (*ModBld_SymbolFun          )    ( ModBld_Module, TMObj_Symbol,              Pointer );
typedef void (*ModBld_SectionUnitFun     )    ( ModBld_Module, TMObj_SectionUnit,         Pointer );
typedef void (*ModBld_External_ModuleFun )    ( ModBld_Module, TMObj_External_Module,     Pointer );



/*-----------------------Basic Module Handling Functions ---------------------*/



/*
 * Function         : Create new module
 * Parameters       : type          (I)   Type of returned module
 *                    stored_endian (I)   Required stored endianness of returned module
 *                    code_endian   (I)   Intended endianness of the code which might
 *                                        be stored in the object. 
 * Function Result  : A new, empty module containing only an initialised header
 */
ModBld_Module 
    ModBld_create_module( 
            TMObj_Module_Type      type,
            Endian                 stored_endian,
            Endian                 code_endian
    );



/*
 * Function         : Read module from file, giving a result
 *                    with all sections loaded, and all offsets
 *                    and addresses in data structures resolved
 *                    according to the actual locations where
 *                    the sections were loaded. 
 *                    The section offsets in section markers 
 *                    have been resolved to the actual address
 *                    in memory.
 * Parameters       : path                   (I)  name of file to read from
 *                    keep_system_sections   (I)  if True, the system sections
 *                                                will be kept in the resulting
 *                                                module; otherwise they will be 
 *                                                deleted. These sections
 *                                                should normally *not* be kept,
 *                                                but this option is available
 *                                                for e.g. tmdump who want to
 *                                                inspect them.
 *                    unshuffle_dynamic_scatters
 *                                   (I)   unshuffle scatters of dynamic loading
 *                                         life. these scatteres are normally 
 *                                         shuffled, because they are used *after*
 *                                         the object has beed loaded and bit-shuffled.
 *                                         this in contrast to normal scatters, which
 *                                         are used during loading, *before* the object
 *                                         image has been bit-shuffled.
 * Function Result  : An in-memory representation of the module.
 * Errors           : If the object could not be read,
 *                    an error is reported to the error handler,
 *                    and Null is returned.
 */
ModBld_Module
    ModBld_read_module_from_file( 
                   String      path, 
                   Bool        keep_system_sections,
                   Bool        unshuffle_dynamic_scatters
    );



/*
 * Function         : Write module to file. Being in building
 *                    mode, the contents of sections (i.e. the
 *                    section units) are not expected to be
 *                    allocated consecutively in memory, and
 *                    all section offsets in markers are actual
 *                    addresses. This routine will take care of
 *                    all issues related to determining which 
 *                    system sections need be created, how section
 *                    contents should be made consecutive, and
 *                    how addresses and offsets of data structures
 *                    should be properly adjusted.
 * Parameters       : module           (I)  module to write
 *                    path             (I)  name of file to write to
 *                    lifetime_filter  (I)  output filter: only data
 *                                          up to the specified lifetime
 *                                          is written. Allowed values are:
 *                               Unused:           write all data to
 *                                                 file, regardless of
 *                                                 lifetime.
 *                               Administrative:   strip all unused data
 *                                                 (e.g. symbols which
 *                                                 are nowhere used)
 *                               Loading:          Strip all data which
 *                                                 is not strictly used
 *                                                 for downloading (e.g.
 *                                                 debugger info).
 *                    preserved_symbols (I) set of names of symbols
 *                                          which should be kept with
 *                                          the object, regardless of the
 *                                          lifetime filter; symbols
 *                                          must be external
 *                    minimize_debug    (I) when TRUE, the debug section
 *                                          will be stripped from non-minimal
 *                                          stabs, and all symbol names
 *                                          except those in preserved_symbols
 *                                          will be replaced by random ones.
 *                    symbol_prefix     (I) prefix to use for randomizes symbols
 *                               
 * Function Result  : -
 * Errors           : If the object could not be written,
 *                    an error is reported to the error handler.
 */
void
    ModBld_write_module( 
             ModBld_Module           module, 
             String                  path,
             TMObj_Section_Lifetime  lifetime_filter,
             Lib_Set /* String */    preserved_symbols,
             Bool                    minimize_debug,
             String                  symbol_prefix
    );


/*
 * Function         : Unpack a memory image containing an object.
 * Parameters       : mem                    (I)  start address of object image
 *                    length                 (I)  length of object image
 *                    keep_system_sections   (I)  if True, the system sections
 *                                                will be kept in the resulting
 *                                                module; otherwise they will be 
 *                                                deleted. These sections
 *                                                should normally *not* be kept,
 *                                                but this option is available
 *                                                for e.g. tmdump who want to
 *                                                inspect them.
 *                    unshuffle_dynamic_scatters
 *                                   (I)   unshuffle scatters of dynamic loading
 *                                         life. these scatteres are normally 
 *                                         shuffled, because they are used *after*
 *                                         the object has beed loaded and bit-shuffled.
 *                                         this in contrast to normal scatters, which
 *                                         are used during loading, *before* the object
 *                                         image has been bit-shuffled.
 * Function Result  : An in-memory representation of the module.
 * Errors           : If the object could not be unpacked,
 *                    an error is reported to the error handler,
 *                    and Null is returned.
 */
ModBld_Module
    ModBld_read_module_from_mem( 
                   Address     mem, 
                   Int         length, 
                   Bool        keep_system_sections,
                   Bool        unshuffle_dynamic_scatters
    );




/*
 * Function         : Deallocate the module's private data structures.
 *                    This function removes all data structures in
 *                    the module which are only accessible via the
 *                    module descriptor. Data which could have been
 *                    extracted earlier, or which could have been
 *                    stored inte the module and remembered for other
 *                    use (sections, symbols, markers, sectionunits e.d.)
 *                    are *not* deallocated. For this reason, this routine
 *                    is not a complete module deallocator, but it is
 *                    in line with the linker tools usages of modules
 *                    which is cannibalising modules to use their data
 *                    in a new result.
 * Parameters       : module   (I)  module to clean out
 * Function Result  : -
 */
void
    ModBld_cleanout_module( 
                   ModBld_Module module
    );




/*------------------------------- Header Retrieval ---------------------------*/

/*
 * Function         : Get (address of) a module's header. Apart from the fields
 *                    nrof_sections and szof_section_strtab, its contents are
 *                    free to be adjusted.
 * Parameters       : module  (I)  module to get header from
 * Function Result  : (Address of) the module header
 */
TMObj_Module
    ModBld_get_header( ModBld_Module module );


/*------------------------------- Symbol Handling ----------------------------*/


/*
 * Function         : Get a global symbol from name
 * Parameters       : module  (I)  module to get descriptor from
 *                    name    (I)  name of symbol to get
 * Function Result  : requested symbol, or Null
 */
TMObj_Symbol
    ModBld_get_global_symbol( 
             ModBld_Module   module, 
             String          name
    );



/*
 * Function         : Add the symbol to the specified module; 
 *                    in case the symbol is global and a global
 *                    symbol with the same name already existed in the
 *                    module, it is replaced in the module's 
 *                    global symbol index (see return status).
 * Parameters       : module  (I)  module to add symbol to
 *                    symbol  (I)  symbol to add
 * Function Result  : True iff. the symbol could be successfully added
 *                    (i.e. no global symbol clash)
 */
Bool
    ModBld_add_symbol( 
             ModBld_Module   module, 
             TMObj_Symbol    symbol
    );



/*------------------------------ Section Handling ----------------------------*/


/*
 * Function         : Get section from name
 * Parameters       : module  (I)  module to get section from
 *                    system  (I)  if True: lookup in system namespace
 *                                 otherwise in user namespace.
 *                    name    (I)  name of section to get
 * Function Result  : requested section, or Null
 */
TMObj_Section
    ModBld_get_section( 
             ModBld_Module  module, 
             Bool        system,
             String         name 
    );


/*
 * Function         : Add the section to the specified module; 
 *                    in case the section with the same 
 *                    name already exists in the module,
 *                    it is replaced in the module's 
 *                    section index (see return status).
 * Parameters       : module    (I)  module to add section to
 *                    section   (I)  section to add
 * Function Result  : True iff. the section could be successfully added
 *                    (i.e. no section clash)
 */
Bool
    ModBld_add_section( 
             ModBld_Module  module, 
             TMObj_Section  section 
    );


/*
 * Function         : Rename the section in the specified module
 *                    to the specified new name; 
 *                    in case a section with the new name 
 *                    already exists in the module,
 *                    it is replaced in the module's 
 *                    section index (see return status).
 * Parameters       : module    (I)  module to add section to
 *                    section   (I)  section to add
 *                    new_name  (I)  new section name
 * Function Result  : True iff. the section could be successfully added
 *                    (i.e. no section clash)
 */
Bool
    ModBld_rename_section( 
             ModBld_Module  module, 
             TMObj_Section  section,
             String         new_name
    );




/*---------------------------- Section Unit Handling -------------------------*/



/*
 * Function         : Add a new sectionunit to the module, having the meaning
 *                    of adding data at the end of the corresponding section.
 *                    A section unit sonsists of a section marker, an alignment,
 *                    a length and a pointer to bytes.
 *                    The offset in the marker is irrelevant, since it will be
 *                    constructed in this function from the current section 
 *                    length while taking the specified alignment into account.
 *                    The section length will be updated afterwards.
 *                    In case the section does have_bytes, the section image
 *                    will be constructed at file write from the bytes specified
 *                    with the section units.
 * Parameters       : module   (I)  module to add new section data to
 *                    unit     (I)  unit of data to add to section
 * Function Result  : True iff. section unit could be added
 *                    (otherwise, there was 'some' inconsistency in the input)
 */
Bool
    ModBld_add_sectionunit( 
             ModBld_Module      module, 
             TMObj_SectionUnit  unit
    );



/*----------------------------- Reference Handling ---------------------------*/


/*
 * Function         : Add symbolic reference to data
 *                    NB: this function implicitly adds 
 *                        scatter info to the module
 * Parameters       : module   (I)  module to add new reference to
 *                    ref      (I)  reference to be added
 * Function Result  : True iff. reference could be added
 *                    (otherwise, there was 'some' inconsistency in the input)
 */
Bool
    ModBld_add_symbol_reference( 
             ModBld_Module            module, 
             TMObj_Symbol_Reference   ref 
    );


/*
 * Function         : Add marker reference to data
 *                    NB: this function implicitly adds 
 *                        scatter info to the module
 * Parameters       : module   (I)  module to add new reference to
 *                    ref      (I)  reference to be added
 * Function Result  : True iff. reference could be added
 *                    (otherwise, there was 'some' inconsistency in the input)
 */
Bool
    ModBld_add_marker_reference( 
             ModBld_Module            module, 
             TMObj_Marker_Reference   ref 
    );


/*
 * Function         : Add a jumptable marker reference to data
 *                    NB: this function implicitly adds 
 *                        scatter info to the module
 * Parameters       : module   (I)  module to add new reference to
 *                    ref      (I)  reference to be added
 * Function Result  : True iff. reference could be added
 *                    (otherwise, there was 'some' inconsistency in the input)
 */
Bool
    ModBld_add_jtab_reference( 
             ModBld_Module               module, 
             TMObj_JTab_Reference        ref 
    );


/*-------------------------- Module Object Traversal -------------------------*/




/*
 * Function         : Apply the specified function to all sections 
 *                    in the specified module, with specified additional argument.
 *                    NB: both user and system sections are traversed.
 * Parameters       : module           (I)  module to traverse
 *                    section_fun      (I)  void (section, data), function to apply
 *                    data             (I)  additional argument to section_fun
 * Function Result  : -
 */
void
    ModBld_traverse_sections( 
             ModBld_Module           module, 
             ModBld_SectionFun       section_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all sections 
 *                    in the specified module, with specified additional argument.
 *                    Traversal is done in the order in which the sections
 *                    will be/have been stored in the object file.
 *                    NB: both user and system sections are traversed.
 * Parameters       : module           (I)  module to traverse
 *                    section_fun      (I)  void (section, data), function to apply
 *                    data             (I)  additional argument to section_fun
 * Function Result  : -
 */
void
    ModBld_traverse_sorted_sections( 
             ModBld_Module           module, 
             ModBld_SectionFun       section_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all external modules 
 *                    referred to in the specified module, with specified 
 *                    additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    module_fun       (I)  void (section, data), function to apply
 *                    data             (I)  additional argument to section_fun
 * Function Result  : -
 */
void
    ModBld_traverse_external_modules( 
             ModBld_Module                module, 
             ModBld_External_ModuleFun    module_fun,
             Pointer                        data 
    );



/*
 * Function         : Apply the specified function to all symbolic references 
 *                    in the module, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 */
void
    ModBld_traverse_symbol_references( 
             ModBld_Module               module, 
             ModBld_Symbol_ReferenceFun  reference_fun,
             Pointer                       data 
    );



/*
 * Function         : Apply the specified function to all symbol references in the specified
 *                    module which fill in data into the specified section unit, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit      (I)  section unit from which the references must be traversed
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 * NB               : The references are traversed in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
void
    ModBld_traverse_symbol_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit,
             ModBld_Symbol_ReferenceFun  reference_fun,
             Pointer                       data 
    );



/*
 * Function         : Return a list of all symbol references in the specified
 *                    module which fill in data into the specified section unit.
 * Parameters       : module           (I)  module to inspect
 *                    sectionunit      (I)  section unit from which the references must be returned
 * Function Result  : see function
 * NB               : The returned references are in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
Lib_List
    ModBld_get_symbol_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit
    );



/*
 * Function         : Return a list of all symbol references in the specified
 *                    module which fill in data into the specified section unit.
 * Parameters       : module           (I)  module to inspect
 *                    sectionunit      (I)  section unit from which the references must be returned
 * Function Result  : see function
 * NB               : The returned references are in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
Lib_List
    ModBld_get_jtab_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit
    );



/*
 * Function         : Return a list of all symbol references in the specified
 *                    module which fill in data into the specified section unit.
 * Parameters       : module           (I)  module to inspect
 *                    sectionunit      (I)  section unit from which the references must be returned
 * Function Result  : see function
 * NB               : The returned references are in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
Lib_List
    ModBld_get_marker_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit
    );



/*
 * Function         : Apply the specified function to all marker references 
 *                    in the module, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 */
void
    ModBld_traverse_marker_references( 
             ModBld_Module               module, 
             ModBld_Marker_ReferenceFun  reference_fun,
             Pointer                       data 
    );



/*
 * Function         : Apply the specified function to all marker references in the specified
 *                    module which fill in data into the specified section, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    section          (I)  section from which the references must be traversed
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 */
void
    ModBld_traverse_marker_references_in_section( 
             ModBld_Module               module, 
             TMObj_Section               section,
             ModBld_Marker_ReferenceFun  reference_fun,
             Pointer                       data 
    );



/*
 * Function         : Apply the specified function to all marker references in the specified
 *                    module which fill in data into the specified section unit, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit      (I)  section unit from which the references must be traversed
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 * NB               : The references are traversed in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
void
    ModBld_traverse_marker_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit,
             ModBld_Marker_ReferenceFun  reference_fun,
             Pointer                       data 
    );



/*
 * Function         : Apply the specified function to all marker references in the specified
 *                    module which point *to* the specified section unit, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit      (I)  section unit to which the references must be traversed
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 */
void
    ModBld_traverse_marker_references_to_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit,
             ModBld_Marker_ReferenceFun  reference_fun,
             Pointer                     data 
    );



/*
 * Function         : Apply the specified function to all jumptable references 
 *                    in the module, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 */
void
    ModBld_traverse_jtab_references( 
             ModBld_Module                  module, 
             ModBld_JTab_ReferenceFun       reference_fun,
             Pointer                        data 
    );



/*
 * Function         : Apply the specified function to all jtab references in the specified
 *                    module which fill in data into the specified section unit, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit      (I)  section unit from which the references must be traversed
 *                    reference_fun    (I)  void (reference, data), function to apply
 *                    data             (I)  additional argument to reference_fun
 * Function Result  : -
 * NB               : The references are traversed in the order of 'offset' in
 *                    the dest fields, i.e in order of referencing within the unit
 */
void
    ModBld_traverse_jtab_references_in_sectionunit( 
             ModBld_Module               module, 
             TMObj_SectionUnit           sectionunit,
             ModBld_JTab_ReferenceFun    reference_fun,
             Pointer                     data 
    );



/*
 * Function         : Apply the specified function to all symbols in the specified
 *                    module with the specified type, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    symbol_fun       (I)  void (symbol, data), function to apply
 *                    data             (I)  additional argument to symbol_fun
 * Function Result  : -
 */
void
    ModBld_traverse_symbols( 
             ModBld_Module           module, 
             ModBld_SymbolFun        symbol_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all symbols in the specified
 *                    module, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    type             (I)  type of symbols to traverse
 *                    symbol_fun       (I)  void (symbol, data), function to apply
 *                    data             (I)  additional argument to symbol_fun
 * Function Result  : -
 */
void
    ModBld_traverse_symbols_by_type( 
             ModBld_Module           module, 
             TMObj_Symbol_Type       type,
             ModBld_SymbolFun        symbol_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all symbols in the specified
 *                    module which fill in data into the specified section, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    section          (I)  section from which the symbols must be traversed
 *                    symbol_fun       (I)  void (symbol, data), function to apply
 *                    data             (I)  additional argument to symbol_fun
 * Function Result  : -
 */
void
    ModBld_traverse_symbols_in_section( 
             ModBld_Module           module, 
             TMObj_Section           section,
             ModBld_SymbolFun        symbol_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all symbols in the specified
 *                    module which refer (in)to the specified sectionunit, 
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit      (I)  section from which the symbols must be traversed
 *                    symbol_fun       (I)  void (symbol, data), function to apply
 *                    data             (I)  additional argument to symbol_fun
 * Function Result  : -
 */
void
    ModBld_traverse_symbols_in_sectionunit( 
             ModBld_Module           module, 
             TMObj_SectionUnit       sectionunit,
             ModBld_SymbolFun        symbol_fun,
             Pointer                   data 
    );



/*
 * Function         : Apply the specified function to all section units in the specified
 *                    module, with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    sectionunit_fun  (I)  void (module, data), function to apply
 *                    data             (I)  additional argument to sectionunit_fun
 * Function Result  : -
 */
void
    ModBld_traverse_sectionunits( 
             ModBld_Module           module, 
             ModBld_SectionUnitFun   sectionunit_fun,
             Pointer                   data 
    );


/*
 * Function         : Apply the specified function to all section units in the specified
 *                    module which fill in data into the specified section,
 *                    with specified additional argument.
 * Parameters       : module           (I)  module to traverse
 *                    section          (I)  section from which the units must be traversed
 *                    sectionunit_fun  (I)  void (module, data), function to apply
 *                    data             (I)  additional argument to sectionunit_fun
 * Function Result  : -
 */
void
    ModBld_traverse_sectionunits_in_section( 
             ModBld_Module           module, 
             TMObj_Section           section,
             ModBld_SectionUnitFun   sectionunit_fun,
             Pointer                   data 
    );


/*
 * Function         : Return secionunit which contains the 
 *                    specified marker in the specified module.
 * Parameters       : module           (I)  module to inspect
 *                    marker           (I)  (address of) marker to locate
 * Function Result  : sectionunit which contains marker
 */
TMObj_SectionUnit
    ModBld_get_sectionunit_of_marker( 
             ModBld_Module           module, 
             TMObj_Marker            marker 
    );





/*
 * Function         : Strip all sectionunit information from the 
 *                    module. Note: this freezes the order of all section
 *                    units in their sections.
 * Parameters       : module           (I)  module to strip
 * Function Result  : -
 */
void
    ModBld_strip_sectionunit_info( 
             ModBld_Module  module 
    );





void
    ModBld_setup_traversal( 
             ModBld_Module           module
    );


void
    ModBld_setdown_traversal( 
             ModBld_Module           module
    );



#endif /* ModuleBuild_INCLUDED */
     
   
