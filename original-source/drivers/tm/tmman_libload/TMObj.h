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
 *  Module name              : TMObj.h    1.83
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Object Module description
 *
 *  Last update              : 10:09:31 - 98/04/10
 *
 *  Description              :  
 *
 * 
 * This file contains the description of the Trimedia Object Module format.
 * An object module basically consists of an (unbounded) amount of named  
 * 'sections', which describe independent data areas with optional, arbitrary 
 * contents. Sections are expected to be loaded into memory at a certain   
 * 'position' during various stages of linking and during eventual object 
 * downloading. Changing the 'position' will cause the implicit value of 
 * markers (see further) to change. Propagating these changes is called 
 * 'relocation'.
 * 
 * For cross-referencing purposes between sections of the same or other
 * object modules, the following data types are supported:
 * 
 *          Markers          : A description of a position in a Section
 *                             (a section address, say). A marker has a 
 *                             value which is dependent on the position  
 *                             of the corresponding section.
 *          Symbols          : A symbolic name of a value, which can 
 *                             either be still unknown, or a constant, or  
 *                             (the value of) a marker.
 *          Reference	     : A Reference is a position in a section 
 *                             (identified by a marker) which is to contain
 *                             a yet unknown value (identified by a symbol 
 *                             or a marker).
 *          Scatters         : The various parts of TM1 instructions are 
 *                             stored in non-consecutive areas in memory. 
 *                             Particularly the different fields of 
 *                             (relocatable) constants are scattered around 
 *                             the location at which they must eventually be 
 *                             filled in. 
 *                             A scatter describes the details of this.
 *          Section Unit     : A section unit describes an area within a 
 *                             section whose contents must be kept together 
 *                             in the 'original' order.
 *                             This is to inform tools which want to reorder 
 *                             section contents or which want to delete parts 
 *                             of sections (e.g. dtree reordering for code 
 *                             compaction, identical code folding or unreferenced
 *                             code removal).
 * 
 * A byte oriented addressing mode is chosen for object modules. That is,
 * the values of (most) offsets, of alignment sizes etc are in byte units.
 * The only exception is the bit offset in scatter destination descriptions.
 * 
 * The type TMObj_Word has been identified as the one which is to be affected
 * when a transition is to be made to 64-bit machine words. The current object
 * file description assumes that section sizes (and hence offsets within 
 * sections) will always fit in 32-bit integers, and that hence only 
 * (relocatable) values and section positions will be affected in case of 
 * 64 bit words.
 *
 * Sections can contain arbitrary data. Users may define sections for executable
 * code, for different data segments, for symbol table information, for source
 * maps, etc. Note that hence not only *downloadable* data may be stored in 
 * sections. When needed, encryption information may be stored in a separate 
 * section.
 * 
 * The below type definitions describe the in- memory view of object modules.
 * An object module stored in a *file* (i.e. an object file) strongly resembles 
 * this format, but needs some conversion:
 * 
 *                1) Each object file may be stored in either little-
 *                   or big endian mode. When the stored endianness does
 *                   not match the 'current' endianness, conversion of all
 *                   words in the system sections is necessary. This conversion
 *                   is facilitated by the fact that all these sections contain
 *                   homogenous data.
 *                   By storing the object file in a specific endianness,
 *                   object file manipulations (including downloading overhead)
 *                   can be minimised.
 *                2) Items of all of the described pointer types below,
 *                   as well as the string table, are stored in special 
 *                   sections (special being that they have reserved names
 *                   and that their contents are known). All pointers to these
 *                   items must be offset with the position at which the
 *                   section in which they reside is loaded. 
 * 
 * As far as downloading is concerned, each section of an executable object has 
 * one of the following lifetime properties:
 * 
 *     1) It is not at all needed for downloading
 *     2) It is only needed during downloading
 *     3) It is needed after downloading, during runtime
 * 
 * Proper ordering of sections according to these properties allow
 * the minimisation of file accesses, loading multiple sections of same
 * lifetime in one access. Items as strings might be divided into
 * multiple sections, based on their life time.
 * 
 * Although during the process of linking all of the different types of 
 * object administration are needed, a fully linked executable only needs
 * a collection of reference descriptions based on anonymous markers for
 * relocation purposes. All other information such as symbols can be stripped.
 * For this reason, a number of reference- and marker cases have been identified
 * for space minimisation. 
 *
 */

#ifndef  TMObj_INCLUDED
#define  TMObj_INCLUDED

/*-------------------------------- Includes ----------------------------------*/

#include "tmtypes.h"


/*------------------------------ Basic Types ---------------------------------*/

typedef UInt32    TMObj_Word;


/*----------------------------- Pointer types --------------------------------*/

typedef struct TMObj_Module_Rec      	             *TMObj_Module;
typedef struct TMObj_Section_Rec     	             *TMObj_Section;

typedef struct TMObj_Scatter_Rec     	             *TMObj_Scatter;
typedef struct TMObj_Scatter_Source_Rec              *TMObj_Scatter_Source;
typedef struct TMObj_Scatter_Dest_Rec                *TMObj_Scatter_Dest;

typedef struct TMObj_Symbol_Rec      	             *TMObj_Symbol;
typedef struct TMObj_External_Module_Rec             *TMObj_External_Module;
typedef struct TMObj_SectionUnit_Rec                 *TMObj_SectionUnit;

typedef struct TMObj_Marker_Rec         	     *TMObj_Marker;
typedef struct TMObj_Default_Marker_Rec              *TMObj_Default_Marker;

typedef struct TMObj_Symbol_Reference_Rec            *TMObj_Symbol_Reference;
typedef struct TMObj_Marker_Reference_Rec            *TMObj_Marker_Reference;
typedef struct TMObj_JTab_Reference_Rec              *TMObj_JTab_Reference;
typedef struct TMObj_FromDefault_Reference_Rec       *TMObj_FromDefault_Reference;
typedef struct TMObj_DefaultToDefault_Reference_Rec  *TMObj_DefaultToDefault_Reference;

typedef struct TMObj_ModuleDescr_Rec                 *TMObj_ModuleDescr;
typedef struct TMObj_SectionInfo_Rec                 *TMObj_SectionInfo;
typedef struct TMObj_JtabEntry_Rec                   *TMObj_JtabEntry;

/*-------------------------- Module Header -----------------------------------*/

#define TMObj_Format_Version          219   /* v2.19 */
#define TMObj_Default_Stored_Endian   BigEndian
#define TMObj_Magic_Number            0x3c46f37a 
#define TMObj_ModDesc_Section_Name    "__mdesc__"
#define TMObj_ModDescs_Section_Name   "__mdescs__"
#define TMObj_Embedded_Modlist_Name   "__embl__"
#define TMObj_Debug_Section_Name      "debug"

/*
 * Module header, for Object Files, this is placed 
 * at the start to be able to interpret the rest 
 * of the file. Separate areas are reserved for
 * minor additions by users and TriMedia:
 */

    typedef enum {
	TMObj_ObjectSegment,
	TMObj_BootSegment,
	TMObj_DynBootSegment,
	TMObj_AppSegment,
	TMObj_DllSegment
    } TMObj_Module_Type;


typedef struct TMObj_Module_Rec {
        Int8       		type;                  /* TMObj_Module_Type                          */
        Int8       		stored_endian;         /* TMObj_Endian, stored object endianness     */
        Int8       		code_endian;           /* TMObj_Endian, endianness of contained code */
        Int8       	       _padd;          

        TMObj_Symbol    	start;                 /* Optional start symbol, or Null             */

        Int32      		version;               /* For                                        */
        Int32       		machine;               /*      Consistency                   	     */
        Int32       		checksum;              /*                   Checking         	     */
        Int32       		magic;        

        Int32         	        reserved1;  
   
        TMObj_Section   	default_section;       /* Optional default section, or Null  	     */
        TMObj_Section   	mod_desc_section;      /* Module description sections, filled in     */
                                                       /* for apps, dlls and dynboots, or else NULL  */
        TMObj_Symbol            embedded_modlist;      /* List of module descriptors for statically  */
                                                       /* linked modules                             */
        Int32       		nrof_sections;         /* Number of sections                 	     */
        Int32       		szof_section_strtab;   /* Size of stringtable for strings    	     */
                                                       /* in section table                   	     */

       /* 
        * The following contain information on the layout of the
        * sections which play a role in downloading. It may, but
        * need not, be used by downloaders.
        *
        * It defines the sizes of the following blocks of data
        *
        *    1) downloadable 'normal' section images
        *       (starting with all text sections)
        *    2) sections containing dynamic loader data
        *    3) sections containing data used during downloading
        *        (excluding the streamloading sections)
        *    4) uninitialised sections
        *
        *    The sizes include all necessary padding, and groups 
        *    1,2 and 3 are stored adjacent in the object file. 
        *    This particularly means that both loaders on host and 
        *    target are supported: target loaders can easily combine
        *    dynamic loader data with the 'normal' image, and read it
        *    using a single file access into preallocated memory,
        *    while host loaders can only transfer the 'normal' image
        *    and keep the (temporary) download data separate from the
        *    (permanent) dynamic loader data.
        *    The following additional information is provided:
        *
        *    5) alignments of uninitialised and initialised 'normal'
        *       section groups. The alignment of all (dynamic)loader 
        *       sections is specified as 4.
        *    6) the combined size of all text sections. This is needed
        *       for doing instruction shuffling at the end of downloading
        */

        Int32                  initialised_size;
        Int32                  uninitialised_size;
        Int32                  dynamicloading_size;
        Int32                  downloading_size;

        Int16                  initialised_alignment;
        Int16                  uninitialised_alignment;

        Int32                  text_size;
        Int16                  nrof_exported_symbols;

        Int16       	      _padd1;     
        Int32       	       internal_stream_size;     
        Int16       	       nrof_internal_stream_sections;     
        Int16       	       first_internal_stream_section;     

       /* 
        * Reserved space:
        */

        Int32       	       trimedia_reserved [ 7];
        Int32       	       user_reserved     [10];

} TMObj_Module_Rec;




/*
 * Descriptions of referred other modules
 * needed for dynamic linking. 
 */

    typedef enum {
	TMObj_Immediate  = 0,
	TMObj_Deferred   = 1
    } TMObj_Binding_Time;


typedef struct TMObj_External_Module_Rec {
        String			name;                  
        Int32       		checksum;              /* for guaranteeing consistency
                                                        * at actual loading
                                                        */
        Int8       		binding_time;          /* TMObj_Binding_Time */
        Int8                   _padd;          

        Int16                   nrof_exported_symbols;
     
        Int8                    name_hash;
        Int8     	       _padd1, _padd2, _padd3;    
        TMObj_ModuleDescr       loaded_module;         /* placeholder for dynamic loader
                                                        * to fill in the corresponding 
                                                        * module descriptor, when loaded
                                                        */                      
        TMObj_ModuleDescr       loading_module;        /* placeholder for dynamic loader
                                                        * to hold in the corresponding 
                                                        * module descriptor, when loading
                                                        */                      
} TMObj_External_Module_Rec;



/*----------------------- Section Descriptor ---------------------------------*/

/*
 * Sections, either in 'current' module, 
 * or in external module in case of 
 * dynamic linking.
 */

    typedef enum {
	TMObj_Null_Life			=  0,
	TMObj_Unused_Life		=  1,
	TMObj_Administrative_Life	=  2,
	TMObj_ObjectConstruction_Life	=  3,
	TMObj_Object_Life		=  4,
	TMObj_StreamLoading_Life	=  5,
	TMObj_Loading_Life		=  6,
	TMObj_DynamicLoading_Life	=  7,
	TMObj_Permanent_Life		=  8,
        TMObj_System_Life		=  9,
        TMObj_LAST_LIFE			= 10
    } TMObj_Section_Lifetime;

    typedef enum {
        TMObj_Cached,
	TMObj_Uncached,
	TMObj_CacheLocked
    } TMObj_Caching_Property;

    typedef enum {
	TMObj_Scatter_Section,
	TMObj_Scatter_Source_Section,
	TMObj_Symbol_Section,
	TMObj_Section_Section,
	TMObj_External_Module_Section,
	TMObj_String_Section,
	TMObj_SectionUnit_Section,
	TMObj_Scatter_Dest_Section,
	TMObj_Symbol_Reference_Section,
	TMObj_Marker_Reference_Section,
	TMObj_JTab_Reference_Section,
	TMObj_FromDefault_Reference_Section,
	TMObj_DefaultToDefault_Reference_Section,
	TMObj_LAST_SECTION_ID
    } TMObj_System_Section_Id;



typedef struct TMObj_Section_Rec {
        String		name;
        Int32       	reserved;   
        Pointer	        bytes;             /* pointer to section contents, or NULL     */
        Int32           size;              /* section size in bytes                    */
	Int16           alignment;         /* entire section alignment                 */
        Int8          	endian;            /* of data in section                       */
        Int8          	has_data;          /* TRUE iff section has data in the object  */
        Int8            lifetime;          /* TMObj_Section_Lifetime                   */
	Int8            is_system;         /* TRUE iff. is created by linker/loader    */
	Int8            is_code;           /* TRUE iff. contains executable code       */
	Int8            caching_property;  /*  - sic -                                 */
        Int8            order;             /* to guide section order in object file    */
        Int8            is_not_shared;     /* false iff section is to be shared on MP systems */
        Int8            is_read_only;      /* true iff section contains readonly data  */
        Int8            system_section_id; /* ico system section: System_Section_Id    */
        Int32           relocation_offset;
} TMObj_Section_Rec;



/*------------------------ Section Markers -----------------------------------*/

/*
 * A section marker defines a byte 
 * position in a specified section.
 * Each module has an optional default 
 * section (usually the one which
 * contains the executable code); 
 * markers in this default 
 * section may be abbreviated:
 */

typedef struct TMObj_Marker_Rec {
        TMObj_Section   	section;
	Int32       		offset;
} TMObj_Marker_Rec;



typedef struct TMObj_Default_Marker_Rec {
	Int32       		offset;
} TMObj_Default_Marker_Rec;


/*------------------------ Symbol Descriptor ---------------------------------*/


    typedef enum {
	TMObj_LocalScope,
	TMObj_GlobalScope,
	TMObj_DynamicScope 
    } TMObj_Symbol_Scope;


    typedef enum {
	TMObj_UnresolvedSymbol,
	TMObj_CommonSymbol,
	TMObj_AbsoluteSymbol,
	TMObj_RelativeSymbol,
	TMObj_ExternalSymbol 
    } TMObj_Symbol_Type;



typedef struct TMObj_Symbol_Rec {
        String				name;
        Int8       			scope;      /* TMObj_Symbol_Scope       */
        Int8       			type;       /* TMObj_Symbol_Type        */
        Int16       	       	        jtab_index; /* for dynamic symbols: 
                                                     * index in jtab, or -1
                                                     */
        union {
           
            TMObj_Word     		value;      /* TMObj_AbsoluteType       */
            TMObj_Marker_Rec  		marker;     /* TMObj_RelativeType       */

            struct {
                Int8     	        is_download_parameter;    
                Int8     	       _padd1, _padd2, _padd3;    
            } unresolved;                           /* TMObj_UnresolvedSymbol   */

            struct {
                 Int16     		alignment;
                 Int8     	       _padd1, _padd2; 
                 Int32     		length;   
            } memspec;                              /* TMObj_CommonSymbol       */

            struct {
                 TMObj_External_Module  module;
            } external;                              /* TMObj_ExternalSymbol     */
        } attr;
} TMObj_Symbol_Rec;



/*---------------------- Section Unit Descriptor -----------------------------*/


typedef struct TMObj_SectionUnit_Rec {
        TMObj_Marker_Rec      	marker;
	Int32             	length;
        Int16                   alignment;      /* must be divisor of section's alignment */
        Int8                    reserved; 
        Int8                    is_root;
} TMObj_SectionUnit_Rec;


/*---------------------- Scatter Descriptors ---------------------------------*/

/* 
 * A scatter consists of two arrys with one-one element mapping.
 * Each source element contains a bit mask specifying a number 
 * of (not necessarily consecutive) bits of the scattered Word; 
 * the corresponding destination element describes a relative byte
 * offset and a bitshift which must be applied to the masked bits
 * in order to achieve the proper bit position.
 * A scatter has an alignment (in byte unit). It is guaranteed that
 * shifting the mask according to the specifiec amount will let the
 * entire mask fall within the representation capacity of the number
 * of bytes specified by alignment.
 */

typedef struct TMObj_Scatter_Source_Rec {
	TMObj_Word			mask;
} TMObj_Scatter_Source_Rec;


typedef struct TMObj_Scatter_Dest_Rec {
	Int8 				offset;  
	Int8 				shift;  
} TMObj_Scatter_Dest_Rec;


typedef struct TMObj_Scatter_Rec {
        TMObj_Scatter_Source /*[]*/   	source;
        TMObj_Scatter_Dest   /*[]*/   	dest;
} TMObj_Scatter_Rec;


/*----------------------- Reference Descriptors ------------------------------*/


/*
 * References, which have five different representations:
 *   1) A general form, from an arbitrary section to a position in
 *      another section which is represented by means of a marker.
 *   2) A general form, from an arbitrary section to a position in
 *      another section which is represented by means of a symbol
 *      with offset.
 *   3) From within the default section to a position in
 *      (possibly) another section (compact representation).
 *   4) From within the default section to a position in that
 *      same default section (even more compact representation).
 *   5) Reference to an item in another code segment. Such references
 *      always refer to such an external item by means of its byte offset 
 *      into the code segment's external address table ('jump' table), in which
 *      the item's actual address is to be found.
 *      This jump table is part of the code segment's module header,
 *      which is stored in the 'mdesc' section of its object file.
 */

typedef struct TMObj_Marker_Reference_Rec {
        TMObj_Scatter   		scatter;
        TMObj_Marker_Rec   		dest;
        TMObj_Marker_Rec   		value;
} TMObj_Marker_Reference_Rec;



typedef struct TMObj_Symbol_Reference_Rec {
        TMObj_Scatter    		scatter;
        TMObj_Marker_Rec   		dest;
        TMObj_Symbol    		value;
        Int32  			        offset;
} TMObj_Symbol_Reference_Rec;



typedef struct TMObj_FromDefault_Reference_Rec {
        TMObj_Scatter  			scatter;
	TMObj_Default_Marker_Rec        dest;
        TMObj_Marker_Rec 	        value;
} TMObj_FromDefault_Reference_Rec;



typedef struct TMObj_DefaultToDefault_Reference_Rec {
        TMObj_Scatter   		scatter;
	TMObj_Default_Marker_Rec        dest;
        TMObj_Default_Marker_Rec 	value;
} TMObj_DefaultToDefault_Reference_Rec;



typedef struct TMObj_JTab_Reference_Rec {
        TMObj_Scatter   		scatter;
        TMObj_Marker_Rec   		dest;
        TMObj_External_Module           module;
        Int32   	                jtab_offset;
        Int32  			        offset;
} TMObj_JTab_Reference_Rec;



/*------------------ Runtime Descriptor of Loaded Modules --------------------*/



    typedef struct TMObj_SectionInfo_Rec {
	TMObj_Word   base;
	UInt         size;
    } TMObj_SectionInfo_Rec;

    typedef struct TMObj_JtabEntry_Rec {
	TMObj_Word   address;
	UInt         partial_checksum;
    } TMObj_JtabEntry_Rec;



typedef struct TMObj_ModuleDescr_Rec {
    String                          name;
    TMObj_Word                      start;
    Int8                            type;
    Int8     	                    name_hash;    
    Int8     	                   _padd;    
    Int8     	                    marked;    
    Int                             nrof_sections;
    TMObj_SectionInfo               section_table /*[]*/;
    TMObj_ModuleDescr               next;

    Int                             reference_count;

    Int                             nrof_external_modules;
    TMObj_External_Module           external_modules;

    Int                             nrof_jtab_references;
    TMObj_JTab_Reference            jtab_references;

    Int8                            embedded;
    Int8                            is_dirty;
    Int16                           visited;

    TMObj_JtabEntry                 jumptable;
    Int                             nrof_exported_symbols;
    Int                             text_size;

    TMObj_Word                      image_memory;
    Pointer                         loading_state;
} TMObj_ModuleDescr_Rec;




#endif /* TMObj_INCLUDED */
     
   
