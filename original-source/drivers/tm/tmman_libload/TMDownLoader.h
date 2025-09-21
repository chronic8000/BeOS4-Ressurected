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
 *  Module name              : TMDownLoader.h    1.39
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : TM1 downloader interface
 *
 *  Last update              : 10:12:14 - 98/04/20
 *
 *  Description              :  
 *
 *               This interface provides the typical functions which 
 *               are needed by a TM-1 downloader. Downloading here is
 *               defined as the process of getting a bootable executable
 *               on a TM1 in reset state. This in contrast to 'dynamic 
 *               loading', in which case the TM1 itself loads an executable
 *               or library. 
 *
 *               Such downloading is expected to be performed in
 *               the following steps:
 *
 *               1) An executable object is loaded into the downloader's
 *                  memory, either from an object file, or from an image
 *                  of the object file which already resides in memory. 
 *                  This step results in a handle which is to be used in 
 *                  the next steps.
 *
 *               2) Optionally, the minimally required download size 
 *                  is retrieved (that is, the size that all downloaded
 *                  sections occupy, excluding heap- and stack).
 *                  This information can be used for allocating space.
 *
 *               3) Download symbols are resolved. Download
 *                  symbols are symbols which have been allowed to stay
 *                  undefined during final linking, to be given a definition
 *                  (a constant) during downloading. Special download symbols
 *                  are e.g. used for initial stack- and heap
 *                  pointer (implicitly handled in next step).
 *
 *               4) The loaded object is relocated, still in downloader's
 *                  memory, according to the following values specified by 
 *                  the client of this module:
 *                       - start of MMIO space
 *                       - processor frequency
 *                               (* Note: The above values are 
 *                                        used to define corresponding
 *                                        symbols in the TCS standard
 *                                        reset code with the following 
 *                                        names, when present:
 *                                             __MMIO_base_init
 *                                             __MMIO<i>_base_init
 *                                             __clock_freq_init
 *                                        The downloader also determines
 *                                        the stack and heap area, which
 *                                        will cause initialisation of the
 *                                        following names when present:
 *                                             __begin_stack_init
 *                                             __begin_heap_init
 *                                        All of these symbols should
 *                                        have been defined in C terms as:
 *                                           extern void *special_sym[];
 *                                *)
 *                       - The download area in terms of TM1's address
 *                         space.
 *                         
 *               5) The loaded object is written out, typically to
 *                  sdram, via the following values specified by
 *                  the client of this module:
 *                       - The download area in the current (TMDownLoader's)
 *                         address space.
 *
 *               6) The loaded object is unloaded from the downloader's
 *                  memory.
 *
 *
 *               Special provisions are defined for multiprocessor
 *               configurations, as follows: 
 *               
 *               a) A special version of the relocation function,
 *                  TMDwnLdr_multiproc_relocate, is defined. This function
 *                  takes two additional arguments, reflecting the number of
 *                  nodes in the system and the current node nr, and takes
 *                  an array of MMIO_base locations (one for each node) instead
 *                  of a single one.
 *                  By this, the downloader is able to let the downloaded code
 *                  know on which processor it runs, and how to access the 
 *                  MMIO spaces of the other nodes.
 *
 *               b) By using the option -sectionproperty <section>=shared of
 *                  tmld, sections can be declared as being shared between all
 *                  nodes. This means that, contrary to 'normal' sections,
 *                  space will be allocated only once, at relocation of the 
 *                  first object which contains such a section; relocation of
 *                  later objects will cause binding of shared sections with
 *                  the same name to the already loaded one. This binding will
 *                  be performed *regardless* of its contents, so it is the user
 *                  responsibility to make sure that the definitions match.
 *                  Addresses of previously loaded shared sections will be
 *                  passed by means of a SharedSectionTable. This table need
 *                  to be created by the user, passed to each call to the
 *                  load object function, and deleted when it is no longer used.
 *                  It is automatically updated and used by calls to the
 *                  appropriate functions.
 *                  
 *               This downloader interface is intended as a sensible 
 *               interface for the usual situations in which no
 *               sections exist which should be loaded at specific
 *               places in memory. The placement of the sections within
 *               the specified downloader range is determined by the
 *               downloader, based on e.g. section (caching) properties 
 *
 *               One of the assumptions underlying this interface is
 *               that relocation should not be performed in SDRAM due to
 *               the memory traffic involved (relocation, and bit shuffling); 
 *               this is the reason for the image preparation in downloader's 
 *               memory. 
 *
 *               NB: This library is *not* reentrant in a 
 *                   multithreading environment.
 */

#ifndef  TMDwnLdr_INCLUDED
#define  TMDwnLdr_INCLUDED


/*----------------------------includes---------------------------------------*/

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */


#ifdef __TCS__
#include <tmlib/tmtypes.h>
#include <tmlib/tmhost.h>
#include <tmlib/Lib_IODrivers.h>
#else
#include "tmtypes.h"
#include "tmhost.h"
#include "Lib_IODrivers.h"
#endif



/*----------------------------definitions-------------------------------------*/

/*
 * Result status values for the exported functions:
 */

typedef enum {
       TMDwnLdr_OK,
       TMDwnLdr_UnexpectedError,
       TMDwnLdr_InputFailed,
       TMDwnLdr_InsufficientMemory,
       TMDwnLdr_NotABootSegment,
       TMDwnLdr_InconsistentObject,
       TMDwnLdr_UnknownObjectVersion,
       TMDwnLdr_NotFound,
       TMDwnLdr_UnresolvedSymbols,
       TMDwnLdr_SymbolIsUndefined,
       TMDwnLdr_SymbolNotInInitialisedData,
       TMDwnLdr_SDRamTooSmall,
       TMDwnLdr_SDRamImproperAlignment,
       TMDwnLdr_SymbolNotADownloadParm,
       TMDwnLdr_NodeNumberTooLarge,
       TMDwnLdr_NumberOfNodesTooLarge,
       TMDwnLdr_HandleNotValid,
       TMDwnLdr_EndianMismatch,
       TMDwnLdr_ImagePackingRequired
} TMDwnLdr_Status;





typedef enum {
    TMDwnLdr_Cached,
    TMDwnLdr_Uncached,
    TMDwnLdr_CacheLocked
} TMDwnLdr_Caching;


typedef enum {
    TMDwnLdr_LocalScope,
    TMDwnLdr_GlobalScope,
    TMDwnLdr_DynamicScope 
} TMDwnLdr_Symbol_Scope;


typedef enum {
    TMDwnLdr_UnresolvedSymbol,
    TMDwnLdr_AbsoluteSymbol,
    TMDwnLdr_RelativeSymbol,
    TMDwnLdr_DynamicallyImportedSymbol 
} TMDwnLdr_Symbol_Type;


typedef enum {
    TMDwnLdr_ByAddress,
    TMDwnLdr_ByName
} TMDwnLdr_Symbol_Traversal_Order;



typedef struct TMDwnLdr_Section_Rec {
        String			name;             
	Address	        	bytes;           
	UInt          		size;             
	UInt          		alignment;        
	Bool                 	big_endian;       
	Bool          		has_data;         
	Bool                 	is_code;          
	Bool                 	is_read_only;     
	TMDwnLdr_Caching    	caching;         
	Address                 relocation;
} TMDwnLdr_Section_Rec;







/*
 * Identifier type for
 *    - loaded executable
 *    - loaded symboltable
 *    - loaded sectiontables
 */
typedef struct InternalHandle *TMDwnLdr_Object_Handle;
typedef struct InternalHandle *TMDwnLdr_Symbtab_Handle;
typedef struct InternalHandle *TMDwnLdr_SharedSectionTab_Handle;



/*
 * Function type for symboltable traversal:
 */
typedef void
         (*TMDwnLdr_Symbol_Fun)( 
                      String                 section, 
                      String                 symbol, 
                      Address                sym_address,
		      TMDwnLdr_Symbol_Type   type,
		      TMDwnLdr_Symbol_Scope  scope,
                      Pointer                data
                               );

/*
 * Function type for section traversal:
 */
typedef void
         (*TMDwnLdr_Section_Fun)( 
                      TMDwnLdr_Section_Rec *section, 
                      Pointer                   data
                               );

/*
 * Option for TMDwnLdr_relocate
 * (see there):
 */
typedef enum {
    TMDwnLdr_CachesOff,
    TMDwnLdr_LeaveCachingToUser,
    TMDwnLdr_LeaveCachingToDownloader
} TMDwnLdr_CachingSupport;



/*----------------------- Shared Section Table Functions ---------------------*/


/*
 * Function         : Create an empty shared section_table, for use in 
 *                    multiprocessing downloading
 * Parameters       : result (O)  Returned handle, or undefined when
 *                                result unequal to TMDwnLdr_OK.
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_UnexpectedError 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc.
 */

TMDwnLdr_Status 
    TMDwnLdr_create_shared_section_table( 
               TMDwnLdr_SharedSectionTab_Handle   *result
    );




/*
 * Function         : Unload shared section table.
 * Parameters       : handle       (IO) handle of loaded table to unload
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Postcondition    : handle becomes invalid
 */

TMDwnLdr_Status 
    TMDwnLdr_unload_shared_section_table( 
               TMDwnLdr_SharedSectionTab_Handle    handle
    );


/*-------------------------- Object Loading Functions ------------------------*/



/*
 * Function         : Read an executable from file into memory, and
 *                    return a handle for subsequent operations.
 * Parameters       : path            (I)  Name of executable file to be loaded.
 *                    shared_sections (I)  Table remembering the addresses of 
 *                                         shared sections in case of multiple 
 *                                         downloads of executables in a 
 *                                         multiprocessor system.
 *                                         Null is allowed when this facilicity
 *                                         is not used, for instance in single
 *                                         processor downloads
 *                    result          (O)  Returned handle, or undefined when
 *                                         result unequal to TMDwnLdr_OK.
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_InputFailed
 *                    TMDwnLdr_NotABootSegment
 *                    TMDwnLdr_UnknownObjectVersion
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_UnexpectedError 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc.
 */

TMDwnLdr_Status 
    TMDwnLdr_load_object_from_file( 
               String                            path,
               TMDwnLdr_SharedSectionTab_Handle  shared_sections,
               TMDwnLdr_Object_Handle           *result
    );




/*
 * Function         : Read an executable from a memory area into memory, and
 *                    return a handle for subsequent operations.
 * Parameters       : mem             (I)  Start of memory image of executable.
 *                    length          (I)  Length of image
 *                    shared_sections (I)  Table remembering the addresses of 
 *                                         shared sections in case of multiple 
 *                                         downloads of executables in a 
 *                                         multiprocessor system.
 *                                         Null is allowed when this facilicity
 *                                         is not used, for instance in single
 *                                         processor downloads
 *                    result          (O)  Returned handle, or undefined when
 *                                         result unequal to TMDwnLdr_OK.
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_NotABootSegment
 *                    TMDwnLdr_UnknownObjectVersion
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_UnexpectedError 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc.
 */

TMDwnLdr_Status 
    TMDwnLdr_load_object_from_mem( 
               Address                           mem,
               Int                        	 length,
               TMDwnLdr_SharedSectionTab_Handle  shared_sections,
               TMDwnLdr_Object_Handle           *result
    );




/*
 * Function         : Read an executable from a precreated driver, and
 *                    return a handle for subsequent operations.
 * Parameters       : driver          (I)  Driver controlling object access.
 *                    shared_sections (I)  Table remembering the addresses of 
 *                                         shared sections in case of multiple 
 *                                         downloads of executables in a 
 *                                         multiprocessor system.
 *                                         Null is allowed when this facilicity
 *                                         is not used, for instance in single
 *                                         processor downloads
 *                    result          (O)  Returned handle, or undefined when
 *                                         result unequal to TMDwnLdr_OK.
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_NotABootSegment
 *                    TMDwnLdr_UnknownObjectVersion
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_UnexpectedError 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc.
 */

TMDwnLdr_Status 
    TMDwnLdr_load_object_from_driver( 
               Lib_IODriver                      driver,
               TMDwnLdr_SharedSectionTab_Handle  shared_sections,
               TMDwnLdr_Object_Handle           *result
    );




/*
 * Function         : Get the minimal size which is needed for 
 *                    downloading the memory image contained 
 *                    in the specified loaded executable.
 *                    This size includes all inter-section 
 *                    padding, but does not include space for
 *                    heap and stack. All additional space specified
 *                    with the next functions will be added to the
 *                    stack/heap region. 
 * Parameters       : handle              (I) handle of loaded exec  
 *                                            to be queried
 *                    minimal_image_size  (O) minimal size of image       
 *                    alignment           (O) the required alignment
 *                                            of the download area
 *                                            in terms of TM1's address
 *                                            space
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status 
    TMDwnLdr_get_image_size( 
               TMDwnLdr_Object_Handle    handle,
               Int                      *minimal_image_size, 
               Int                      *alignment 
    );




/*
 * Function         : Relocate the loaded executable into a specified
 *                    TM1 address range, with specified values for
 *                    MMIO_base and TM1_frequency. The specified TM1
 *                    address base must be aligned according to the
 *                    value returned by TMDwnLdr_get_image_sizes. Also
 *                    the length of this range must be larger than the
 *                    minimal length returned by TMDwnLdr_get_image_sizes.
 * Parameters       : handle         (IO) handle of loaded exec to be relocated
 *
 *                    host_type,
 *                    MMIO_base,
 *                    TM1_frequency   (I) values of which the object might 
 *                                        want to know.
 *                    sdram_base      (I) base of download area in TM's
 *                                        address space
 *                    sdram_len       (I) length of download area
 *                    caching_support (I) specification of responsibility
 *                                        of setting the cacheable limit
 *                                        and the cachelocked regions:
 *                             TMDwnLdr_LeaveCachingToUser:
 *                                   cacheable limit and cachelocked
 *                                   regions are entirely under control of  
 *                                   the user, the downloader/boot code will 
 *                                   not touch it.
 *                             TMDwnLdr_LeaveCachingToDownloader:
 *                                   cachelocked regions and cacheble 
 *                                   limit are entirely under control
 *                                   of the downloader, which will use
 *                                   this control to intelligently map
 *                                   the different cached/uncached/
 *                                   cachelocked sections within the specified
 *                                   sdram, partitioned in different caching
 *                                   property regions, and let the downloaded
 *                                   program set cacheable limit and cachelocked
 *                                   regions accordingly. 
 *                             TMDwnLdr_CachesOff:
 *                                   cachelocked regions and cacheble 
 *                                   limit are entirely under control
 *                                   of the downloader, which will let
 *                                   the downloaded program run with
 *                                   'cache off'. 
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_UnresolvedSymbols
 *                    TMDwnLdr_SDRamTooSmall,
 *                    TMDwnLdr_SDRamImproperAlignment,
 *                    TMDwnLdr_HandleNotValid
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_UnexpectedError 
 */

TMDwnLdr_Status 
    TMDwnLdr_relocate( 
               TMDwnLdr_Object_Handle    handle,
               tmHostType_t              host_type,
               Address                   MMIO_base,
               UInt                      TM1_frequency,
               Address                   sdram_base,
               UInt                      sdram_length,
               TMDwnLdr_CachingSupport   caching_support
    );




/*
 * Function         : Relocate the loaded executable into a specified
 *                    TM1 address range, with specified values for
 *                    MMIO_base and TM1_frequency. The specified TM1
 *                    address base must be aligned according to the
 *                    value returned by TMDwnLdr_get_image_sizes. Also
 *                    the length of this range must be larger than the
 *                    minimal length returned by TMDwnLdr_get_image_sizes.
 *
 *                    This relocation function is intended for use in 
 *                    multiprocessor TM1 environments; the model which this
 *                    function assumes is that in such an enviroment
 *                    all processors are numbered from 0 .. number_of_nodes-1,
 *                    and their SDRAMs and MMIO spaces are cross accessible.
 *
 * Parameters       : handle         (IO) handle of loaded exec to be relocated
 *
 *                    host_type,
 *                    MMIO_bases      (I) Array specifying for each node 
 *                                        in the range 0 .. number_of_nodes-1 
 *                                        its mmio base address
 *                    node_number     (I) 'current' node number, i.e. the processor
 *                                        number on which the relocated code is to run;
 *                                        its value is required to
 *                                        be in the range 0 .. number_of_nodes
 *                    number_of_nodes (I) number of TM1s available
 *                    TM1_frequency   (I) Processor frequency in MHz.
 *                    sdram_base      (I) base of download area in TM's
 *                                        address space
 *                    sdram_len       (I) length of download area
 *                    caching_support (I) specification of responsibility
 *                                        of setting the cacheable limit
 *                                        and the cachelocked regions:
 *                             TMDwnLdr_LeaveCachingToUser:
 *                                   cacheable limit and cachelocked
 *                                   regions are entirely under control of  
 *                                   the user, the downloader/boot code won't
 *                                   touch it.
 *                             TMDwnLdr_LeaveCachingToDownloader:
 *                                   cachelocked regions and cacheble 
 *                                   limit are entirely under control
 *                                   of the downloader, which will use
 *                                   this control to intelligently map
 *                                   the different cached/uncached/
 *                                   cachelocked sections within the specified
 *                                   sdram, partitioned in different caching
 *                                   property regions, and let the downloaded
 *                                   program set cacheable limit and cachelocked
 *                                   regions accordingly. 
 *                             TMDwnLdr_CachesOff:
 *                                   cachelocked regions and cacheble 
 *                                   limit are entirely under control
 *                                   of the downloader, which will let
 *                                   the downloaded program run with
 *                                   'cache off'. 
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_UnresolvedSymbols
 *                    TMDwnLdr_SDRamTooSmall,
 *                    TMDwnLdr_SDRamImproperAlignment,
 *                    TMDwnLdr_HandleNotValid
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_NodeNumberTooLarge
 *                    TMDwnLdr_NumberOfNodesTooLarge
 *                    TMDwnLdr_UnexpectedError 
 */

TMDwnLdr_Status 
    TMDwnLdr_multiproc_relocate( 
               TMDwnLdr_Object_Handle    handle,
               tmHostType_t              host_type,
               Address                  *MMIO_bases,
               UInt                      node_number,
               UInt                      number_of_nodes,
               UInt                      TM1_frequency,
               Address                   sdram_base,
               UInt                      sdram_length,
               TMDwnLdr_CachingSupport   caching_support
    );






/*
 * Function         : Copy the section's memory images of the specified 
 *                    loaded executable into a buffer in the current
 *                    (i.e. the downloader's) address space. The 
 *                    buffer must be as large as the size specified
 *                    at the call to TMDwnLdr_relocate.
 *                    
 *                    NB: This function is destructive on the loaded
 *                        object. It cannnot further be used and must
 *             	          be deallocated after this call.
 *
 * Parameters       : handle        (I) handle of loaded exec to be queried
 *                    base          (I) base of download area in the 
 *                                      address space of the caller of
 *                                      this function
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status 
    TMDwnLdr_get_memory_image( 
               TMDwnLdr_Object_Handle    handle,
               Address                   base
    );




/*
 * Function         : Assign a 32-bit value to a 
 *                    symbol with specified name in an 
 *                    initialised data section. 
 *                   *NOTE*: the symbol must have dynamic scope.
 * Parameters       : handle   (I) handle of loaded exec to be patched
 *                    symbol   (I) name of symbol in null-terminated string
 *                    value    (I) value to assign
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_SymbolNotInInitialisedData
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_patch_value(
               TMDwnLdr_Object_Handle    handle,
               String                    symbol,
               UInt32                    value
    );




/*
 * Function         : Get a 32-bit contents of a 
 *                    symbol with specified name from an 
 *                    initialised data section. 
 *                   *NOTE*: the symbol must have dynamic scope.
 * Parameters       : handle   (I) handle of loaded exec to be patched
 *                    symbol   (I) name of symbol in null-terminated string
 *                    value    (I) value to assign
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_SymbolNotInInitialisedData
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_get_contents(
               TMDwnLdr_Object_Handle    handle,
               String                    symbol,
               UInt32                   *value
    );




/*
 * Function         : Define a 32-bit absolute value for the still 
 *                    unresolved symbol with specified name (this must
 *                    then be an unresolved symbol of type 'download_parm').
 *                    This function must be used to resolve all download
 *                    parameters before any call to TMDwnLdr_relocate.
 *                   *NOTE*: the symbol must have dynamic scope, but
 *                           this is automatically the case when it has 
 *                           been created by the Trimedia object library,
 *                           or by tmld
 * Parameters       : handle   (I) handle of loaded exec to be patched
 *                    symbol   (I) name of symbol in null-terminated string
 *                    value    (I) value to define with
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_HandleNotValid
 *                    TMDwnLdr_SymbolNotADownloadParm
 */

TMDwnLdr_Status
    TMDwnLdr_resolve_symbol(
               TMDwnLdr_Object_Handle    handle,
               String                    symbol,
               UInt32                    value
    );




/*
 * Function         : Retrieve a 32-bit value from a 
 *                    symbol with specified name in an 
 *                    initialised data section.
 * Parameters       : handle   (I)  handle of loaded exec
 *                    symbol   (I)  name of symbol in null-terminated string
 *                    result   (O)  pointer to result location
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_SymbolNotInInitialisedData
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_get_value(
               TMDwnLdr_Object_Handle    handle,
               String                    symbol,
               UInt32                   *result
    );



/*
 * Function         : Unload loaded executable; all resources 
 *                    allocated for the executable will be freed,
 *                    but extracted section group images and extracted
 *                    symboltables will be unaffected.
 * Parameters       : handle       (IO) handle of loaded exec to unload
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Postcondition    : handle becomes invalid
 */

TMDwnLdr_Status 
    TMDwnLdr_unload_object( 
               TMDwnLdr_Object_Handle    handle
    );



/*
 * Function         : Lookup a user section by name
 * Parameters       : handle  (I)  handle of loaded exec to get section from
 *                    name    (I)  name of requested section
 *                    section (IO) pointer to buffer which will be set
 *                                 as a result of this function
 *                                 
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_NotFound
 *                    TMDwnLdr_HandleNotValid
 */
TMDwnLdr_Status
    TMDwnLdr_get_section( 
               TMDwnLdr_Object_Handle    handle,
               String                    name,
               TMDwnLdr_Section_Rec     *section
    );



/*
 * Function         : apply function 'fun' to all sections in the
 *                    specified loaded object, in download order
 *                    and with additional data argument.
 * Parameters       : handle     (I)  handle of loaded executable to traverse
 *                    fun        (I)  function to apply
 *                    data       (I)  additional data argument
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Sideeffects      : fun has been applied to all sections, 
 *                    in the order in which they will be downloaded.
 *                    NB: note that the TMDownLoader will place all
 *                        the cached sections at the beginning of
 *                        SDRam and the uncached (data) sections 
 *                        at the end; so although function will traverse
 *                        in download order, there might be a hole
 *                        in between
 *                    NB: the section buffers which will be used in
 *                        the calls to 'fun' will not survive this
 *                        function call. 
 */

TMDwnLdr_Status
    TMDwnLdr_traverse_sections(
               TMDwnLdr_Object_Handle    handle,
               TMDwnLdr_Section_Fun      fun,
               Pointer                   data
    );




/*
 * Function         : Get the endianness of the specified loaded object.
 * Parameters       : handle     (I)  handle of loaded executable
 *                    endian     (O)  pointer to result location
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_get_endian(
               TMDwnLdr_Object_Handle    handle,
               Endian                   *endian
    );




/*--------------------------- Symbol Table Functions -------------------------*/



/*
 * Function         : Construct a symboltable from a previously
 *                    loaded object, and return a handle for 
 *                    subsequent operations.
 *                    NOTE: the information in this symboltable becomes
 *                          senseless upon subsequent relocation of
 *                          the object, but remains valid when the
 *                          object is unloaded.
 * Parameters       : object (I)  Handle of object to extract from.
 *                    result (O)  Returned handle, or undefined when
 *                                result unequal to TMDwnLdr_OK.
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_InsufficientMemory
 *                    TMDwnLdr_HandleNotValid
 *                    TMDwnLdr_InconsistentObject
 *                    TMDwnLdr_UnexpectedError 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc.
 */

TMDwnLdr_Status 
    TMDwnLdr_load_symbtab_from_object( 
               TMDwnLdr_Object_Handle    object,
               TMDwnLdr_Symbtab_Handle  *result
    );




/*
 * Function         : Return the address of the symbol in terms
 *                    of the loaded object's memory space
 *                    given its current relocation state.
 * Parameters       : handle   (I)  handle of executable's symboltable
 *                    symbol   (I)  name of symbol in null-terminated string
 *                    section  (O)  returned name of symbol's section
 *                    address  (O)  returned symbol address
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_get_address(
               TMDwnLdr_Symbtab_Handle   handle,
               String                    symbol,
               String                   *section,
               Address                  *address
    );




/*
 * Function         : Return info on the symbol with the highest
 *                    address less than or equal to the specified
 *                    address. All addresses in terms
 *                    of the loaded object's memory space
 *                    given its current relocation state.
 * Parameters       : handle         (I)  handle of executable's symboltable
 *                    address        (I)  input address
 *                    section        (O)  section name of matching symbol
 *                    symbol         (O)  name of matching symbol
 *                    symbol_address (O)  address of matching symbol
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_SymbolIsUndefined
 *                    TMDwnLdr_HandleNotValid
 */

TMDwnLdr_Status
    TMDwnLdr_get_enclosing_symbol(
               TMDwnLdr_Symbtab_Handle   handle,
               Address                   address,
               String                   *section,
               String                   *symbol,
               Address                  *symbol_address
    );




/*
 * Function         : apply function 'fun' to all symbols in the
 *                    specified symbol table, with additional 
 *                    data argument.
 * Parameters       : handle     (I)  handle of symboltable to traverse
 *                    order      (I)  parameter to guide order of traversal
 *                    fun        (I)  function to apply
 *                    data       (I)  additional data argument
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Sideeffects      : fun has been applied to each symbol, 
 *                    in the order specified by 'order'.
 */

TMDwnLdr_Status
    TMDwnLdr_traverse_symbols(
               TMDwnLdr_Symbtab_Handle          handle,
               TMDwnLdr_Symbol_Traversal_Order  order,
               TMDwnLdr_Symbol_Fun              fun,
               Pointer                          data
    );




/*
 * Function         : Unload loaded symboltable; all resources 
 *                    allocated for the symboltable will be freed.
 * Parameters       : handle       (IO) handle of loaded symbtab to unload
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Postcondition    : handle becomes invalid
 */

TMDwnLdr_Status 
    TMDwnLdr_unload_symboltable( 
               TMDwnLdr_Symbtab_Handle   handle
    );



/*
 * Function         : Install your own memory manager, memset and memcpy 
 *                    routines. 
 *                    Should be called only once.
 *                    For memory management, only the memory interface
 *                    is used.
 *                    For copying the sections to SDRAM, the specified 
 *                    memcpy function is used. For clearing bss,
 *                    the memset function is used.
 * Parameters       : memcpy_fun (I) memcpy function with default interface
 *                    memset_fun (I) memset function with default interface
 *                    malloc_fun (I) malloc function with default interface
 *                    free_fun   (I) free   function with default interface
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_UnexpectedError, when it is called after
 *                    other calls to the downloader library are made.
 * Postcondition    : 
 */

typedef Pointer   (*TMDwnLdr_MallocFunc)(Int size);
typedef void      (*TMDwnLdr_FreeFunc)	(Pointer);
typedef Pointer   (*TMDwnLdr_MemCpyFunc)(Pointer, const Pointer, UInt);
typedef Pointer   (*TMDwnLdr_MemSetFunc)(Pointer, Int, UInt);

TMDwnLdr_Status 
    TMDwnLdr_install_kernel_functions( 
		TMDwnLdr_MemCpyFunc memcpy_fun, 
		TMDwnLdr_MemSetFunc memset_fun, 
		TMDwnLdr_MallocFunc malloc_fun, 
		TMDwnLdr_FreeFunc   free_fun
    );



/*
 * Function         : return a string that contains the last error message 
 * Parameters       : error code (I) last returned error code
 * Function Result  : pointer to a string that contains an error description
 * Postcondition    : String is owned by the library.
 */

String 
    TMDwnLdr_get_last_error( TMDwnLdr_Status status );

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif /* TMDwnLdr_INCLUDED */
