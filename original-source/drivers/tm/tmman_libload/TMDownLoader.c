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
 *  Module name              : TMDownLoader.c    1.114
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : 
 *
 *  Author                   : Juul van der Spek 
 *
 *  Last update              : 10:59:46 - 98/05/19
 *
 *  Reviewed                 : 
 *
 *  Revision history         : 
 *
 *  Description              :  
 *
 */

/*---------------------------- Includes --------------------------------------*/

#include "tmtypes.h"
#include "TMDownLoader.h"
#include "TMRelocate.h"
#include "TMScatter.h"
#include "FlatObject.h"
#include "LoaderMacros.h"
#include "Lib_Local.h"
#include "Lib_Set.h"
#include "Lib_Mapping.h"
#include "Lib_Memspace.h"
#include "Lib_Messages.h"
#include "Lib_Exceptions.h"
#include "SymtabSearch.h"
#include "Lib_IODrivers.h"
#include "InternalHandle.h"
#include "compr_shuffle.h"
//#include <stdlib.h>
#include <memory.h>

/*---------------------------- Definitions -----------------------------------*/


#define MEMSPACE_INCREMENT  1000   /*  1K increments in memspaces      */
#define STREAM_BUFF_SIZE   10000   /* 10K stream buffer for references */


#define TM1_ICACHE_BLOCK_SIZE   			64
#define TM1_DCACHE_BLOCK_SIZE   			64

#define TM1_DCACHE_LOCK_BASE_GRANULARITY   	(1<<14)
#define TM1_ICACHE_LOCK_BASE_GRANULARITY   	(1<<15)
#define TM1_DCACHEABLE_LIMIT_GRANULARITY   	(64 * 1024)

#define NULL_ENDIAN 100


/*
 * sets consisting of all known handles.
 */

static Lib_Set  object_handles;
static Lib_Set  symbtab_handles;
static Lib_Set  shared_section_handles;

void *LongWordCopy( void *dest, const void *src, size_t count);

static TMDwnLdr_MemCpyFunc	MyMemCpy = (TMDwnLdr_MemCpyFunc)memcpy;
//static TMDwnLdr_MemCpyFunc	MyMemCpy = (TMDwnLdr_MemCpyFunc)LongWordCopy;

static TMDwnLdr_MemSetFunc	MyMemSet = (TMDwnLdr_MemSetFunc)memset;



/*----------------------------- Auxiliary Functions --------------------------*/
void *LongWordCopy( void *dest, const void *src, size_t count)
{
    unsigned long  *pDest;
    unsigned long  *pSrc;
    unsigned long   Idx;

    pDest = (unsigned long *)dest;
    pSrc = (unsigned long *)src;
    printf("pSrc = %p, pDest = %p\n", pDest, pSrc);
//    printf("Swapping %d: 32 bit values\n", ((count + 3) / 4));
    for (Idx = 0; Idx < ((count + 3) / 4); Idx++)
    {
       *pDest = LIENDIAN_SWAP4(*pSrc);
        pDest++;
        pSrc++;
    }
}


static TMDwnLdr_Status map_error( Lib_Msg_Message message )
{
    switch (message) {
    case TMDLdMsg_UnresolvedSymbols :
       return TMDwnLdr_UnresolvedSymbols;    

    case LdLibMsg_Open_Failed    :
    case TMDLdMsg_Input_Failed   : 
       return TMDwnLdr_InputFailed;

    case LibMsg_Memory_Overflow  :
       return TMDwnLdr_InsufficientMemory;

    case LdLibMsg_InvalidModule :
    case LdLibMsg_WrongMagic :
    case LdLibMsg_Inconsistent_Lifetimes :
       return TMDwnLdr_InconsistentObject;

    case LdLibMsg_AncientFormat :
    case LdLibMsg_Unknown_Format_Version :
       return TMDwnLdr_UnknownObjectVersion;

    default:
       return TMDwnLdr_UnexpectedError;
    }
}



static Lib_Set create_handle_set()
{
    Lib_Set       result;
    Lib_Memspace  space, old_space;

   /* ------------------------------------------------- */
    space= Lib_Memspace_new( MEMSPACE_INCREMENT );
    CHECK( space != Null, (LibMsg_Memory_Overflow,MEMSPACE_INCREMENT) ){}

   /* ------------------------------------------------- */
    old_space= Lib_Memspace_set_default( space );

    Lib_Excp_try {
        result= 
            Lib_Set_create( 			
               (Lib_Local_BoolResFunc)Lib_StdFuncs_address_equal, 
               (Lib_Local_EltResFunc )Lib_StdFuncs_address_hash, 
               4 /* just a small amount of buckets */	
            );
    } 
    Lib_Excp_otherwise {
        result= Null;
    }
    Lib_Excp_end_try;

    Lib_Memspace_set_default( old_space );
   /* ------------------------------------------------- */

    if (result == Null) { 
        Lib_Memspace_delete(space);
        CHECK( False, (LibMsg_Memory_Overflow,0) ){}
    }
   /* ------------------------------------------------- */

    return result;
}



/*----------------------- Shared Section Table Functions ---------------------*/


/*
 * Function         : Create an empty shared section_table, for use in 
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
    )
{
    TMDwnLdr_Status   status;
    Lib_Memspace      space, old_space;

    Lib_Msg_clear_last_fatal_message();

   /*
    * Create a new memory space, and install it as default
    * space for the duration of this function call. Be careful
    * herafter: protect the rest of this with exception handlers
    * so that the memspace, with all allocated memory, can be deleted
    * when anything goes wrong. All Lib_Mem operations operate on
    * the default memspace.
    */
    space= Lib_Memspace_new( MEMSPACE_INCREMENT );

    if (space == Null) { return TMDwnLdr_InsufficientMemory; }
    old_space= Lib_Memspace_set_default( space );

   /* ------------------------------------------------- */
    Lib_Excp_try {
        InternalHandle  handle;
        Lib_Mapping     mapping;

        Lib_Mem_NEW(handle);

        mapping= Lib_Mapping_create( 			
		   (Lib_Local_BoolResFunc)Lib_StdFuncs_string_equal, 
		   (Lib_Local_EltResFunc )Lib_StdFuncs_string_hash, 
		   10	
		 );

        handle->space    = space;
        handle->object   = mapping;
        handle->refcount = 1;
        handle->endian   = NULL_ENDIAN;

        if (shared_section_handles == Null) { shared_section_handles= create_handle_set(); }
        Lib_Set_insert(shared_section_handles, handle);

       /*
        * decide success:
        */
       *result= handle;
        status= TMDwnLdr_OK;
    } 
   /* ------------------------------------------------- */
    Lib_Excp_otherwise {
        status= map_error(___Lib_Excp_raised_excep___);
    }
    Lib_Excp_end_try;

   /* ------------------------------------------------- */

    Lib_Memspace_set_default( old_space );

    if (status != TMDwnLdr_OK) {
        Lib_Memspace_delete(space);
    }

   /* ------------------------------------------------- */

    return status;
}




/*
 * Function         : Unload shared section table. These tables are stored
 *                    within loaded objects, and are reference counted to
 *                    allow a call to this unload function before the last
 *                    'sharing' object is unloaded.
 * Parameters       : handle       (IO) handle of loaded table to unload
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_HandleNotValid
 * Postcondition    : handle becomes invalid
 */

	static TMDwnLdr_Status 
	    unload_shared_section_table( 
		       TMDwnLdr_SharedSectionTab_Handle    handle
	    )
	{
	    if ( shared_section_handles != Null
	       && Lib_Set_element(shared_section_handles, handle)
	       ) {
		if (--(handle->refcount) == 0) {
		     Lib_Set_remove(shared_section_handles, handle);
		     Lib_Memspace_delete(handle->space);
		}
		return TMDwnLdr_OK;
	    } else {
		return TMDwnLdr_HandleNotValid;
	    }
	}
	

TMDwnLdr_Status 
    TMDwnLdr_unload_shared_section_table( 
               TMDwnLdr_SharedSectionTab_Handle    handle
    )
{
    Lib_Msg_clear_last_fatal_message();

    return unload_shared_section_table(handle);
}


/*-------------------------- Object Loading Functions ------------------------*/





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
    )
{
    TMDwnLdr_Status   status;
    Lib_Memspace      driver_space, old_space;
    Lib_IODriver      driver;

    Lib_Msg_clear_last_fatal_message();

   /*
    * Create a new memory space for 
    * allocating the driver only, 
    * and allocate the driver:
    */
    driver_space= Lib_Memspace_new(1);

    if (driver_space == Null) { return TMDwnLdr_InsufficientMemory; }
    old_space= Lib_Memspace_set_default( driver_space );

   /* ------------------------------------------------- */

    Lib_Excp_try {
        driver= Lib_IOD_open_mem_for_read(mem,length);
    } 
    Lib_Excp_otherwise {
        driver = Null;
        status = TMDwnLdr_InsufficientMemory;
    }
    Lib_Excp_end_try;

    Lib_Memspace_set_default( old_space );

    if (driver == Null) {
        status= TMDwnLdr_InsufficientMemory;
    } else {
        status= TMDwnLdr_load_object_from_driver(driver,shared_sections,result);
    }

    if (status != TMDwnLdr_OK) {
        if (driver != Null) {
            Lib_IOD_close(driver);
        }
        Lib_Memspace_delete(driver_space);
    } else {
        (*result)->driver_space= driver_space;
    }

   /* ------------------------------------------------- */

    return status;
}


static void 
	section_relocate(
               FlatObject_Module       module, 
               TMObj_Section           section,
               TMDwnLdr_Object_Handle  handle
        );


/*
 * Function         : Read an executable from a precreated driver, and
 *                    return a handle for subsequent operations.
 * Parameters       : mem             (I)  Start of memory image of executable.
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
    )
{
    TMDwnLdr_Status   status;
    Lib_Memspace      space, old_space;

    Lib_Msg_clear_last_fatal_message();

    if (  shared_sections        != Null
       && shared_section_handles != Null
       && !Lib_Set_element(shared_section_handles, shared_sections)
       ) {
        return TMDwnLdr_HandleNotValid;
    }

    if (shared_sections != Null) {
       shared_sections->refcount++;        
    } else {
       if (TMDwnLdr_create_shared_section_table(&shared_sections) != TMDwnLdr_OK) {
           return TMDwnLdr_InsufficientMemory;
       }
    }

   /*
    * Create a new memory space, and install it as default
    * space for the duration of this function call. Be careful
    * herafter: protect the rest of this with exception handlers
    * so that the memspace, with all allocated memory, can be deleted
    * when anything goes wrong. All Lib_Mem operations operate on
    * the default memspace.
    */
    space= Lib_Memspace_new( MEMSPACE_INCREMENT );

    if (space == Null) { 
        unload_shared_section_table(shared_sections);
        return TMDwnLdr_InsufficientMemory; 
    }

    old_space= Lib_Memspace_set_default( space );

   /* ------------------------------------------------- */
    Lib_Excp_try {
        InternalHandle      handle;
        FlatObject_Module   module;

        Lib_Mem_NEW(handle);

        module= FlatObject_open_object( 
                        driver, Null, False, False,
                       (FlatObject_SectionLoaded_Fun)section_relocate, 
                       handle,
                       STREAM_BUFF_SIZE 
                 );

	    CHECK( module != Null, (TMDLdMsg_Input_Failed,Lib_Msg_get_last_fatal_message()) ){}

        handle->space                                  = space;
        handle->object                                 = module;
        handle->driver                                 = driver;
        handle->driver_space                           = Null;
        handle->sstab                                  = shared_sections;
        handle->unresolved_symbols[0]                  = 0;
        handle->nrof_unresolved_symbols                = 0;
        handle->shuffle_image                          = True;
        handle->endian                                 = FlatObject_get_header(module)->code_endian;
        handle->unpack_image                           = False;

        sprintf(handle->unresolved_symbols+UNRESOLVED_SPACE, "...");

        if (object_handles == Null) { object_handles= create_handle_set(); }
        Lib_Set_insert(object_handles, handle);

        if (shared_sections->endian == NULL_ENDIAN) {
            shared_sections->endian= handle->endian;
        }

       /*
        * decide success:
        */
       *result= handle;


        if ( FlatObject_get_header(module)->type != TMObj_BootSegment
          && FlatObject_get_header(module)->type != TMObj_DynBootSegment
           ) {
            status= TMDwnLdr_NotABootSegment;
        } else 

        if (shared_sections->endian != handle->endian) {
            status= TMDwnLdr_EndianMismatch;
        } else {

            status= TMDwnLdr_OK;
        }
    } 
   /* ------------------------------------------------- */
    Lib_Excp_otherwise {
        status= map_error(___Lib_Excp_raised_excep___);
    }
    Lib_Excp_end_try;

   /* ------------------------------------------------- */

    Lib_Memspace_set_default( old_space );

    if (status != TMDwnLdr_OK) {
        unload_shared_section_table(shared_sections);
        Lib_Memspace_delete(space);
    }

   /* ------------------------------------------------- */


    return status;
}




	static Int*
		    select_sectiongroup(
			    TMObj_Section         section,
			    SectionGroupInfo     *info
		    ) 
	{
	    switch (section->caching_property) {
            case TMObj_Cached: 
                      if (section->is_code) { 
                          return &(info->cached_text);
                      } else {
			  if (section->has_data) {       
			     return &(info->cached_data_i);
			  } else {
			     return &(info->cached_data_u);
			  }
                      }
            case TMObj_Uncached:     
                      if (section->is_code) { 
                          return &(info->cached_text);
                      } else {
                          if (section->has_data) {       
                              return &(info->uncached_data_i);
                          } else {
                              return &(info->uncached_data_u);
                          }
                      }
            case TMObj_CacheLocked:   
                      if (section->is_code) { 
                          return &(info->locked_text);
                      } else {
                          if (section->has_data) {       
                              return &(info->locked_data_i);
                          } else {
                              return &(info->locked_data_u);
                          }
                      }
	    }
	}





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
	static void 
	    count_sectiongroup_sizes( 
		    TMObj_Section       section,
		    CountTraversalRec  *rec
			       )
	{
            if ( !section->is_not_shared
               && Lib_Mapping_apply(rec->sstab->object,section->name) != Null
               ) {
               /*
                * Nothing, shared sections which already 
                * have been allocated do not eat up space:
                */

            } else {
		Int *p_size      = select_sectiongroup(section,&(rec->size));
		Int *p_alignment = select_sectiongroup(section,&(rec->align));
    
		Int    size      = *p_size;
		Int    alignment = *p_alignment;
    
		size= LROUND_UP(size, section->alignment);
    
		(*p_size     )= size + section->size;
		(*p_alignment)= LCM(alignment, section->alignment);
            }
	}


TMDwnLdr_Status 
    TMDwnLdr_get_image_size( 
               TMDwnLdr_Object_Handle    handle,
               Int                      *minimal_image_size, 
               Int                      *alignment 
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status    result    = TMDwnLdr_OK;  
        Lib_Memspace       old_space = Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
        Lib_Excp_try {

            CountTraversalRec  rec;
    
	   /* initialise sizes */
	    rec.size.cached_text     = 0; 
	    rec.size.cached_data_i   = 0; 
	    rec.size.cached_data_u   = 0; 
	    rec.size.locked_text     = 0; 
	    rec.size.locked_data_i   = 0; 
	    rec.size.locked_data_u   = 0; 
	    rec.size.uncached_data_i = 0; 
	    rec.size.uncached_data_u = 0; 
    
	   /* initialise alignments */
	    rec.align.cached_text     = 1; 
	    rec.align.cached_data_i   = 1; 
	    rec.align.cached_data_u   = 1; 
	    rec.align.locked_text     = 1; 
	    rec.align.locked_data_i   = 1; 
	    rec.align.locked_data_u   = 1; 
	    rec.align.uncached_data_i = 1; 
	    rec.align.uncached_data_u = 1; 
    
            rec.sstab= handle->sstab;    

	    FlatObject_traverse_sections( 
		handle->object,
		(FlatObject_Section_Fun)count_sectiongroup_sizes,
		TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
		&rec
	    );
            
            if (!handle->unpack_image) {
		if (TMDwnLdr_resolve_symbol( handle, "___Rdo_unpack", True ) == TMDwnLdr_OK) {
                    handle->unpack_image= True;
                }
            }

	   *alignment= rec.align.cached_text;


            if (handle->unpack_image) {
                Int size = LROUND_UP(rec.size.cached_text,rec.align.cached_data_i)
			           + rec.size.cached_data_i
			           + rec.size.locked_data_i
		                   + rec.size.locked_text 
			           + rec.size.uncached_data_i;

	       *minimal_image_size = LROUND_UP(size,sizeof(int));

            } else 

            if ( rec.size.uncached_data_i > 0
              || rec.size.uncached_data_u > 0
               ) {
               /*
                * Uncached data is mapped at the
                * end of SDRAM: generating a consecutive
                * image is not feasible without image
                * packing.
                */
                result= TMDwnLdr_ImagePackingRequired;
            } else {

	       Int size = rec.size.cached_text;

               size = LROUND_UP( size, rec.align.cached_data_i     ) + rec.size.cached_data_i     ;
               size = LROUND_UP( size, rec.align.cached_data_u     ) + rec.size.cached_data_u     ;

	       if (rec.size.locked_text) {
		   size = LROUND_UP( size, TM1_ICACHE_LOCK_BASE_GRANULARITY );
               }
	       size = LROUND_UP( size, rec.align.locked_text  ) + rec.size.locked_text  ;
	       if (rec.size.locked_text) {
		   size = LROUND_UP( size, TM1_ICACHE_BLOCK_SIZE );
	       }

	       if (rec.size.locked_data_i | rec.size.locked_data_u) {
		   size = LROUND_UP( size, TM1_DCACHE_LOCK_BASE_GRANULARITY );
	       }
	       size = LROUND_UP( size, rec.align.locked_data_i) + rec.size.locked_data_i;
     	       size = LROUND_UP( size, rec.align.locked_data_u) + rec.size.locked_data_u;
	       if (rec.size.locked_data_i | rec.size.locked_data_u) {
		   size = LROUND_UP( size, TM1_DCACHE_BLOCK_SIZE );
	       }

	       *minimal_image_size = LROUND_UP(size,sizeof(Int));
            }
        } 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );
        return result;
    }
}

/***********************************************************************************************/

/*
 * Relocate all pointers in the downloaded system data
 * (lifetime DynamicLoading) into the target address space.
 * Such downloaded system data is object description data
 * which is used at runtime by the dynamic loader, and hence
 * should be in target's terms (i.e. endian-ness, and memory
 * addresses).
 *
 * Note that this dynamic loading data may also be used 
 * during relocation (for instance a shared scatter descriptor).
 * For this reason, shifting should be performed after 
 * relocation, just before actual image copying.
 *
 * Such pointers are always to within other such downloaded
 * system sections, except for the 'sections' field in 
 * markers: these point via the module's section table, 
 * which is *not* downloaded.
 * Since in the latter case the full pointer information is
 * encoded in the 'offset' field, these internal section pointers
 * have been simply set to Null after relocation.
 *
 * Relocation itself shifts addresses from downloader space to
 * TM1 space, and performs endian conversion when there is
 * an endian difference between downloader and TM. Address 
 * space shifting of a pointer to an object involves
 * adding the address delta of the section in which
 * the object resides. This address delta is the difference
 * between the 'bytes' field (host address of section start) 
 * and the 'relocation offset' (TM address of section start).
 * Apart from shifting the 'offset' pointers in marker
 * objects, all pointers are pointers to system sections.
 * This depends on these sections to be consecutively loaded/allocated
 * in both address spaces, so their address deltas are the same
 * (though recomputed each time in function relocate_system_data);
 * Consecutivity should be (and is) guaranteed by the FlatObject 
 * interface, and the way of section copying into the target
 * memory. 
 */




    static void shift_address( Pointer a, Int delta, Bool swap )
    {
        Pointer *aa= a;
       *aa = (Pointer) (((Int)*aa)+delta);
	if (swap) XAENDIAN_SWAP4(*aa);
    }
    
    static void shift_int32( Int32 *i, Bool swap )
    {
	if (swap) XIENDIAN_SWAP4(*i);
    }
    
    static void shift_int16( Int16 *i, Bool swap )
    {
	if (swap) XIENDIAN_SWAP2(*i);
    }




static void 
	SHIFT_SCATTER(
		TMObj_Scatter	object,
                Int             delta,
                Bool            swap,
                Pointer         dummy
        ) 
{
/* unused parameter */
(void)dummy;
					
    shift_address( &(object->dest)  , delta,swap ); 	
    shift_address( &(object->source), delta,swap ); 			
}

static void 
	SHIFT_SCATTER_SOURCE(
		TMObj_Scatter_Source	object,
                Int             	delta,
                Bool            	swap,
                Pointer         	dummy
        ) 
{ 									
/* unused parameter */
(void)delta;
(void)dummy;

    shift_int32( (Pointer)&(object->mask), swap );			
}

static void 
	SHIFT_EXTERNAL_MODULE(
		TMObj_External_Module	object,
                Int             	delta,
                Bool            	swap,
                Pointer         	dummy
        ) 
{									
/* unused parameter */
(void)dummy;
					
    shift_address( &(object->name)  ,   delta,swap );			
    shift_int32  ( &(object->checksum), swap );				
    shift_int16  ( &(object->nrof_exported_symbols), swap );				
}

static void 
	SHIFT_MARKER(
		TMObj_Marker	object,
                Int             delta,
                Bool            swap,
                Pointer         dummy
        ) 
{
    Int section_delta;

/* unused parameter */
(void)delta;
(void)dummy;


    section_delta= object->section->relocation_offset
                   - (Int)object->section->bytes;
					 				
    shift_address( &(object->offset), section_delta, swap ); 			
    object->section= Null;						
}			

static void 
	SHIFT_JTAB_REF(
		TMObj_JTab_Reference	object,
                Int             	delta,
                Bool            	swap,
                Pointer         	dummy
        ) 
{ 									
    shift_address( &(object->module),  delta, swap); 			
    shift_address( &(object->scatter), delta, swap); 			
    shift_int32  ( &(object->offset),         swap);				
    shift_int32  ( &(object->jtab_offset),    swap);			
    SHIFT_MARKER ( &object->dest,      delta, swap,dummy); 			
} 



    static void
	relocate_system_data( 
		  TMObj_Section      section,
		  Bool               endian_swap 
	) 
    {
	if ( section->is_system ) {
            Int address_delta;

            address_delta= section->relocation_offset
                         - (Int)section->bytes
                         ;

	    switch (section->system_section_id) {
            case TMObj_Scatter_Section:
	            TRAVERSE_SYSTEM_SECTION(
			      Scatter, section,
			      SHIFT_SCATTER,address_delta,endian_swap,0
	            ); break;
	
            case TMObj_Scatter_Source_Section:
	            TRAVERSE_SYSTEM_SECTION(
			      Scatter_Source, section,
			      SHIFT_SCATTER_SOURCE,address_delta,endian_swap,0
	            ); break;
	
            case TMObj_Scatter_Dest_Section:
	            break;
	
            case TMObj_External_Module_Section:
        	    TRAVERSE_SYSTEM_SECTION(	
			      External_Module, section,
			      SHIFT_EXTERNAL_MODULE,address_delta,endian_swap,0
		    ); break;
	
            case TMObj_JTab_Reference_Section:
	            TRAVERSE_SYSTEM_SECTION(	
			      JTab_Reference, section,
			      SHIFT_JTAB_REF,address_delta,endian_swap,0
	            ); break;
	
            default: 
                /* How do we report errors here? */;
	    }
        }
    }




/***********************************************************************************************/


#define RELOCATE_SYMBOL_REF(ref,handle,d2,d3) \
             if (!TMRelocate_symbol_ref(ref)) {									\
                 Int buflen= strlen(handle->unresolved_symbols);						\
														\
                 if (handle->nrof_unresolved_symbols++) {							\
                     memcpy(handle->unresolved_symbols + buflen, ", ", UNRESOLVED_SPACE-buflen );		\
                     buflen += 2;										\
                 }												\
														\
                 memcpy(handle->unresolved_symbols + buflen, ref->value->name, UNRESOLVED_SPACE-buflen );	\
             } 

#define RELOCATE_MARKER_REF(ref,d1,d2,d3) \
             TMRelocate_marker_ref(ref);

#define RELOCATE_FROMDEFAULT_REF(ref,defsect,d2,d3) \
 	     TMRelocate_fromdefault_marker_ref(ref,defsect);

#define RELOCATE_DEFAULTTODEFAULT_REF(ref,defsect,d2,d3) \
 	     TMRelocate_defaulttodefault_marker_ref(ref,defsect);



    static void
	section_relocate( 
                  FlatObject_Module       module, 
		  TMObj_Section           section,
                  TMDwnLdr_Object_Handle  handle
	) 
    {
        TMObj_Section default_section= FlatObject_get_header(module)->default_section;

	if (section->is_system) {
	    switch (section->system_section_id) {
            case TMObj_Symbol_Reference_Section:
	            TRAVERSE_SYSTEM_SECTION(
			      Symbol_Reference, section,
			      RELOCATE_SYMBOL_REF,handle,dummy1,dummy2
	            ); break;
	
            case TMObj_Marker_Reference_Section:
	            TRAVERSE_SYSTEM_SECTION(	
			      Marker_Reference, section,
			      RELOCATE_MARKER_REF,dummy1,dummy2,dummy3
	            ); break;
	
            case TMObj_FromDefault_Reference_Section:
        	    TRAVERSE_SYSTEM_SECTION(	
			      FromDefault_Reference, section,
			      RELOCATE_FROMDEFAULT_REF,default_section,dummy1,dummy2
		    ); break;
	
            case TMObj_DefaultToDefault_Reference_Section:
	            TRAVERSE_SYSTEM_SECTION(	
			      DefaultToDefault_Reference, section,
			      RELOCATE_DEFAULTTODEFAULT_REF,default_section,dummy1,dummy2
	            ); break;
	    }
        }
    }




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
    ) {
    Address MMIO_bases[1];
    UInt    node_number     = 0;
    UInt    number_of_nodes = 1;

    MMIO_bases[0]= MMIO_base;

    return 
        TMDwnLdr_multiproc_relocate( 
                 handle, host_type, MMIO_bases, node_number, number_of_nodes,
                 TM1_frequency, sdram_base, sdram_length, caching_support );
}




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
 *                    host_type
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

	static void 
	    set_relocation_addresses( 
		    TMObj_Section          section,
		    RelocateTraversalRec  *rec
			       )
	{
            Address shared_address= Lib_Mapping_apply(rec->sstab->object,section->name);

            if ( !section->is_not_shared
               && shared_address != Null
               ) {
               /*
                * Note that shared sections located at address
                * zero will cause problems here. However, because
                * a processor usually executes instructions, and
                * because the SDRAM address range is not expected 
                * to wrap around, this is not expected to give problems:
                */
		section->relocation_offset= (Int)shared_address;

            } else {
		Int *p_address      = select_sectiongroup(section,&(rec->addr));
		Int    address      = *p_address;
    
		address     = LROUND_UP(address, section->alignment);
		(*p_address)= address + section->size;
    
		section->relocation_offset= address;

	       /*
		* If this section is shared, then remember its
		* address for later connection from subsequent nodes:
		*/
		if (!section->is_not_shared) {
		    Lib_Memspace old_space;

		    old_space= Lib_Memspace_set_default( rec->sstab->space );

		    Lib_Mapping_define(
			rec->sstab->object,
			COPY_STRING(section->name),
			(Pointer)address
		    );

		    Lib_Memspace_set_default(old_space);
		}
            }
	}


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
    )
{
    SectionGroupInfo       start_classes;
    FlatObject_Module      module;
    Int                    segment_list;
    Int                    cacheable_limit;
    Int                    begin_stack_init, begin_heap_init;
    CountTraversalRec      crec;  
    Int                    do_section_lock;
    Int                    sdram_top;
    Int                    locked_data_size;
    Int                    locked_text_size;
    TMDwnLdr_Status        mmio_base_stat;
    RelocateTraversalRec   rrec;
    Int                    node;
    TMDwnLdr_Status        result;
    Lib_Memspace           old_space;
    Int                    tmp;    

    Lib_Msg_clear_last_fatal_message();

    result    =   TMDwnLdr_OK;
    sdram_top = ((Int)sdram_base) + sdram_length;
    module    =   handle->object;

   /*
    * Check handle validity:
    */
    if (node_number >= number_of_nodes) {
        return TMDwnLdr_NodeNumberTooLarge;
    } else

    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {

        old_space= Lib_Memspace_set_default( handle->space );

       /*---------------------------------------------------------------*/
	Lib_Excp_try {

           /* decide whether image unpacking is necessary: */
            if (!handle->unpack_image) {
		if (TMDwnLdr_resolve_symbol( handle, "___Rdo_unpack", True ) == TMDwnLdr_OK) {
                    handle->unpack_image= True;
                }
            }

           /* initialise sizes */
	    crec.size.cached_text     = 0; 
	    crec.size.cached_data_i   = 0; 
	    crec.size.cached_data_u   = 0; 
	    crec.size.locked_text     = 0; 
	    crec.size.locked_data_i   = 0; 
	    crec.size.locked_data_u   = 0; 
	    crec.size.uncached_data_i = 0; 
	    crec.size.uncached_data_u = 0; 

           /* initialise alignments */
	    crec.align.cached_text     = 1; 
	    crec.align.cached_data_i   = 1; 
	    crec.align.cached_data_u   = 1; 
	    crec.align.locked_text     = 1; 
	    crec.align.locked_data_i   = 1; 
	    crec.align.locked_data_u   = 1; 
	    crec.align.uncached_data_i = 1; 
	    crec.align.uncached_data_u = 1; 

	    crec.sstab= handle->sstab;    

	    FlatObject_traverse_sections( 
	        module,
	        (FlatObject_Section_Fun)count_sectiongroup_sizes,
	        TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
	        &crec
	    );

	    rrec.addr.cached_text     = (Int)sdram_base;
	    rrec.addr.cached_data_i   = LROUND_UP( rrec.addr.cached_text        + crec.size.cached_text,        crec.align.cached_data_i );
	    rrec.addr.cached_data_u   = LROUND_UP( rrec.addr.cached_data_i      + crec.size.cached_data_i,      crec.align.cached_data_u );
	    rrec.addr.locked_text     = LROUND_UP( rrec.addr.cached_data_u      + crec.size.cached_data_u,      crec.align.locked_text   );
              if (crec.size.locked_text) {
                  rrec.addr.locked_text= LROUND_UP(rrec.addr.locked_text,TM1_ICACHE_LOCK_BASE_GRANULARITY);
              }

	    rrec.addr.locked_data_i   = LROUND_UP( rrec.addr.locked_text   + crec.size.locked_text,   crec.align.locked_data_i );
              if (crec.size.locked_text) {
                  rrec.addr.locked_data_i= LROUND_UP(rrec.addr.locked_data_i,TM1_ICACHE_BLOCK_SIZE);
              }
              if (crec.size.locked_data_i | crec.size.locked_data_u) {
                  rrec.addr.locked_data_i= LROUND_UP(rrec.addr.locked_data_i,TM1_DCACHE_LOCK_BASE_GRANULARITY);
              }

	    rrec.addr.locked_data_u   = LROUND_UP( rrec.addr.locked_data_i + crec.size.locked_data_i, crec.align.locked_data_u );
	    begin_heap_init           = LROUND_UP( rrec.addr.locked_data_u + crec.size.locked_data_u, sizeof(Int)              );
              if (crec.size.locked_data_i | crec.size.locked_data_u) {
	          begin_heap_init      = LROUND_UP( begin_heap_init, TM1_DCACHE_BLOCK_SIZE);
              }

	    rrec.addr.uncached_data_u = LROUND_DOWN( sdram_top                 - crec.size.uncached_data_u, crec.align.uncached_data_u );
	    rrec.addr.uncached_data_i = LROUND_DOWN( rrec.addr.uncached_data_u - crec.size.uncached_data_i, crec.align.uncached_data_i );
	    begin_stack_init          = LROUND_DOWN( rrec.addr.uncached_data_i,                             TM1_DCACHEABLE_LIMIT_GRANULARITY);

            locked_text_size          = LROUND_UP( crec.size.locked_text,                           TM1_ICACHE_BLOCK_SIZE );
            locked_data_size          = LROUND_UP( begin_heap_init - rrec.addr.locked_data_i,       TM1_DCACHE_BLOCK_SIZE );


           /* save for memory image extraction: */
            handle->sectiongroup_sizes      = crec.size;    
            handle->sectiongroup_alignments = crec.align;    

            handle->sectiongroup_positions.cached_text     = rrec.addr.cached_text     - (Int)sdram_base;    
            handle->sectiongroup_positions.cached_data_i   = rrec.addr.cached_data_i   - (Int)sdram_base;    
            handle->sectiongroup_positions.cached_data_u   = rrec.addr.cached_data_u   - (Int)sdram_base;    
            handle->sectiongroup_positions.locked_text     = rrec.addr.locked_text     - (Int)sdram_base;    
            handle->sectiongroup_positions.locked_data_i   = rrec.addr.locked_data_i   - (Int)sdram_base;    
            handle->sectiongroup_positions.locked_data_u   = rrec.addr.locked_data_u   - (Int)sdram_base;    
            handle->sectiongroup_positions.uncached_data_u = rrec.addr.uncached_data_u - (Int)sdram_base;    
            handle->sectiongroup_positions.uncached_data_i = rrec.addr.uncached_data_i - (Int)sdram_base;    
           /* --------------------------------- */

	    if (((Int)sdram_base) % crec.align.cached_text != 0) { 
                result= TMDwnLdr_SDRamImproperAlignment; 
            } else 

	    if ( begin_stack_init <= begin_heap_init ) { 
                result= TMDwnLdr_SDRamTooSmall; 
            } else {

                rrec.sstab    = handle->sstab;    
		start_classes = rrec.addr;
	    

		FlatObject_traverse_sections( 
		    module,
		    (FlatObject_Section_Fun)set_relocation_addresses,
		    TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
		    &rrec
		);
	
		if (FlatObject_get_header(module)->mod_desc_section == Null) {
		    segment_list= Null;
		} else {
		    segment_list= (Int)FlatObject_get_header(module)->mod_desc_section->relocation_offset;
		}
	
		switch (caching_support) {
		case TMDwnLdr_CachesOff:
			   do_section_lock  = True;
			   cacheable_limit  = (Int)sdram_base;
			   locked_data_size = 0;
			   locked_text_size = 0;
			   break;
	
		case TMDwnLdr_LeaveCachingToUser:
			   do_section_lock= False;
			   break;
	
		case TMDwnLdr_LeaveCachingToDownloader:
			   cacheable_limit= begin_stack_init;
			   do_section_lock= True;
			   break;
		}
	
		TMDwnLdr_resolve_symbol( handle, "__begin_stack_init",      begin_stack_init             );
		TMDwnLdr_resolve_symbol( handle, "__begin_heap_init",       begin_heap_init              );
	
		TMDwnLdr_resolve_symbol( handle, "__MMIO_base_init",        (Int)MMIO_bases[node_number] );
		TMDwnLdr_resolve_symbol( handle, "__host_type_init",        host_type                    );
		TMDwnLdr_resolve_symbol( handle, "__clock_freq_init",       TM1_frequency                );
		TMDwnLdr_resolve_symbol( handle, "__segment_list_init",     segment_list                 );
		TMDwnLdr_resolve_symbol( handle, "__node_number_init",      node_number                  );
		TMDwnLdr_resolve_symbol( handle, "__number_of_nodes_init",  number_of_nodes              );
		
		TMDwnLdr_resolve_symbol( handle, "__do_section_lock",       do_section_lock              );
		TMDwnLdr_resolve_symbol( handle, "__locked_data_size",      locked_data_size             );
		TMDwnLdr_resolve_symbol( handle, "__locked_text_size",      locked_text_size             );
		TMDwnLdr_resolve_symbol( handle, "__cacheable_limit",       cacheable_limit              );

		TMDwnLdr_resolve_symbol( handle, "__begin_mem",             (Int)sdram_base              );
		TMDwnLdr_resolve_symbol( handle, "__end_mem",               (Int)sdram_top               );

		TMDwnLdr_resolve_symbol( handle, "__locked_data_addr",      start_classes.locked_data_i  );
		TMDwnLdr_resolve_symbol( handle, "__locked_text_addr",      start_classes.locked_text    );

		TMDwnLdr_resolve_symbol( handle, "___Rzud_len"   , crec.size.uncached_data_u );
		TMDwnLdr_resolve_symbol( handle, "___Rud_len"    , crec.size.uncached_data_i );
		TMDwnLdr_resolve_symbol( handle, "___Rzld_len"   , crec.size.locked_data_u   );
		TMDwnLdr_resolve_symbol( handle, "___Rld_len"    , crec.size.locked_data_i   );
		TMDwnLdr_resolve_symbol( handle, "___Rlt_len"    , crec.size.locked_text     );
		TMDwnLdr_resolve_symbol( handle, "___Rzc_len"    , crec.size.cached_data_u        );

		TMDwnLdr_resolve_symbol( handle, "___Rzud_mbase" , start_classes.uncached_data_u );
		TMDwnLdr_resolve_symbol( handle, "___Rud_mbase"  , start_classes.uncached_data_i );
		TMDwnLdr_resolve_symbol( handle, "___Rzld_mbase" , start_classes.locked_data_u   );
		TMDwnLdr_resolve_symbol( handle, "___Rld_mbase"  , start_classes.locked_data_i   );
		TMDwnLdr_resolve_symbol( handle, "___Rlt_mbase"  , start_classes.locked_text     );
		TMDwnLdr_resolve_symbol( handle, "___Rzc_mbase"  , start_classes.cached_data_u   );

                tmp= ((Int)sdram_base) + LROUND_UP(crec.size.cached_text,crec.align.cached_data_i) + crec.size.cached_data_i;

		TMDwnLdr_resolve_symbol( handle, "___Rlt_ibase"  , tmp );
		TMDwnLdr_resolve_symbol( handle, "___Rld_ibase"  , tmp + crec.size.locked_text );
		TMDwnLdr_resolve_symbol( handle, "___Rud_ibase"  , tmp + crec.size.locked_text + crec.size.locked_data_i );

                if (handle->unpack_image) {
                    handle->locked_text_image_base= tmp;
                } else {
                    handle->locked_text_image_base= handle->sectiongroup_positions.locked_text;
                }

               /*
                * If still no errors, resolve all symbols __MMIO_base<i>_init,  
                * which should be defined for a certain range 0<=i<N,
                * where N is the maximum amount of nodes for
                * which the loaded object is prepared 
                * (hence N should be >= number_of_nodes).
                * All unused symbols (number_of_nodes<=i<N)
                * should be resolved to a dummy value.
                */
		node= 0;
		do {
		    Char name[30];

		    sprintf( name, "__MMIO_base%d_init", node);

		    if ((unsigned)node < number_of_nodes) {

			mmio_base_stat= TMDwnLdr_resolve_symbol(handle, name, (Int)MMIO_bases[node]);

			if ( (mmio_base_stat != TMDwnLdr_OK) 
			   && node > 0 /* don't report error if no multinode 
					* mmio table is defined at all
					*/
			   ) { 
			    result= TMDwnLdr_NumberOfNodesTooLarge; 
			}
		    } else {
			mmio_base_stat= TMDwnLdr_resolve_symbol(handle, name, 0);
		    }

		    node++;
		} while (mmio_base_stat == TMDwnLdr_OK);

               /*
                * If still no errors, stream the  
                * references to do the actual relocation:
                */
                if (result == TMDwnLdr_OK) {	
		    FlatObject_traverse_sections( 
			module,
			(FlatObject_Section_Fun)Lib_StdFuncs_null_func,
			TMObj_StreamLoading_Life, TMObj_StreamLoading_Life,
			Null
		    );
    
		    CHECK (
                       handle->nrof_unresolved_symbols == 0,
		       (TMDLdMsg_UnresolvedSymbols, 
                        handle->nrof_unresolved_symbols == 1 ? "" : "s", 
                        handle->unresolved_symbols)
		    ){}
		}
            }
	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/

        Lib_Memspace_set_default( old_space );

        return result;
    }
}



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
	static void 
	    copy_section_image( 
		    TMObj_Section        section,
                    ExtractTraversalRec *rec
			       )
	{
            if ( !section->is_not_shared
               && Lib_Mapping_apply(rec->sstab->object,section->name) != Null
               ) {
               /*
                * Nothing: forget this section, and just connect to
                * the earlier allocated one (on a previous node):
                */

            } else {
		Int *p_pbase        = select_sectiongroup(section,&(rec->physical_base));
		Int    pbase        = *p_pbase;

		Int *p_vbase        = select_sectiongroup(section,&(rec->virtual_base));
		Int    vbase        = *p_vbase;
    
                if (pbase != -1) {
			vbase     = LROUND_UP(vbase, section->alignment);
			(*p_vbase)= vbase + section->size;
	
			if (section->is_code && rec->shuffle_image) {
			   /* This is why TMDwnLdr_get_memory_image 
			    * is destructive; since shuffling is by far
			    * the most expensive operation, and we do not
			    * want to do it over a possible PCI bus, and
			    * one generally does exactly one download anyway,
			    * and a (non)destructive flag on the interface
			    * is a bother with little value, we keep all this
			    * simply destructive. Note that the instruction padder
			    * in tmld should take care of the section being a multiple
			    * of '32' bytes, which is a precondition to SHFL_shuffle.
			    */
			    SHFL_shuffle(
				section->bytes,
				section->bytes,
				section->size
			    );
                printf("(is_code)MyMemCpy(): to 0x%lx, size: 0x%lx\n", 
                    (unsigned long)(rec->copy_base + pbase + vbase),
                    section->size);
                
			    MyMemCpy(
			       (Pointer) (rec->copy_base + pbase + vbase),
			       section->bytes,
			       section->size
			    );
			}
			
			else if (section->has_data) {
                
                printf("(has_data)MyMemCpy(): to 0x%lx, size: 0x%lx\n", 
                    (unsigned long)(rec->copy_base + pbase + vbase),
                    section->size);
                
			    MyMemCpy(
			       (Pointer) (rec->copy_base + pbase + vbase),
			       section->bytes,
			       section->size
			    );


			} else {
                
                printf("(0)MyMemSet(): to 0x%lx, size: 0x%lx\n", 
                    (unsigned long)(rec->copy_base + pbase + vbase),
                    section->size);
                
			    MyMemSet(
			       (Pointer) (rec->copy_base + pbase + vbase),
			       0,
			       section->size
			    );
			}
                }
            }
	}

TMDwnLdr_Status 
    TMDwnLdr_get_memory_image( 
               TMDwnLdr_Object_Handle    handle,
               Address                   base
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status    result;
        Lib_Memspace       old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {
            ExtractTraversalRec   rec;
            SectionGroupInfo     *sg_size = &handle->sectiongroup_sizes;
            SectionGroupInfo     *sg_align= &handle->sectiongroup_alignments;
	    FlatObject_Module     module = handle->object;
    
	    FlatObject_traverse_sections( 
		module,
		(FlatObject_Section_Fun)relocate_system_data,
		TMObj_DynamicLoading_Life, TMObj_DynamicLoading_Life,
		(Pointer)(FlatObject_get_header(module)->code_endian != LCURRENT_ENDIAN)
	    );
	
            memset((Pointer)&rec.virtual_base,0,sizeof(rec.virtual_base));

            if (handle->unpack_image) {
                Int tmp= LROUND_UP(sg_size->cached_text, sg_align->cached_data_i);

		rec.physical_base.cached_text     =  0;
		rec.physical_base.cached_data_i   =  tmp;
		rec.physical_base.cached_data_u   = -1;
		rec.physical_base.locked_text     =  tmp + sg_size->cached_data_i;
		rec.physical_base.locked_data_i   =  tmp + sg_size->cached_data_i + sg_size->locked_text;
		rec.physical_base.locked_data_u   = -1;
		rec.physical_base.uncached_data_i =  tmp + sg_size->cached_data_i + sg_size->locked_text + sg_size->locked_data_i;
		rec.physical_base.uncached_data_u = -1;
            } else {
		rec.physical_base = handle->sectiongroup_positions;
            }

            rec.sstab                  = handle->sstab;    
            rec.copy_base              = base;    
            rec.shuffle_image          = handle->shuffle_image;    

	    FlatObject_traverse_sections( 
		module,
		(FlatObject_Section_Fun)copy_section_image,
		TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
		&rec
	    );
    
	    result= TMDwnLdr_OK;

	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}









/*
 * Just do a linear search for the
 * symbol. There are not that much 
 * dynamic symbols, and there are not
 * that much symbols to be patched or
 * to be resolved:
 */
    static TMObj_Symbol
        get_dynamic_symbol( FlatObject_Module module, String name )
    {
        TMObj_Section_Lifetime life;

       /*
        * restrict search to sections 
        * with 'dynamic' lifetime:
        */
        for (life= TMObj_Permanent_Life; life >= TMObj_Unused_Life; life--){
            TMObj_Section s;

            s= FlatObject_get_system_section(
                                   module,
                                   TMObj_Symbol_Section,
                                   life
               );

            if (s != Null) {
                Address       base    = s->bytes;
                TMObj_Symbol  symbol  = (TMObj_Symbol) base;
                TMObj_Symbol  ceiling = (TMObj_Symbol) (base + s->size);

                while (symbol < ceiling) {
                    if (symbol->scope != TMObj_LocalScope) {
                        if ( strcmp(symbol->name,name)==0 ) return symbol;
                    }
                    symbol++;
                }
            }
        }

        return Null;
    }


    static TMObj_Symbol
        get_download_symbol( FlatObject_Module module, String name )
    {
        TMObj_Section_Lifetime life;

       /*
        * restrict search to sections 
        * with 'dynamic' lifetime:
        */
        for (life= TMObj_Permanent_Life; life >= TMObj_Object_Life; life--){
            TMObj_Section s;

            s= FlatObject_get_system_section(
                                   module,
                                   TMObj_Symbol_Section,
                                   life
               );

            if (s != Null) {
                Address       base    = s->bytes;
                TMObj_Symbol  symbol  = (TMObj_Symbol) base;
                TMObj_Symbol  ceiling = (TMObj_Symbol) (base + s->size);

                while (symbol < ceiling) {
                    if (symbol->type == TMObj_UnresolvedSymbol) {
                        if ( strcmp(symbol->name,name)==0 ) return symbol;
                    }
                    symbol++;
                }
            }
        }

        return Null;
    }




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
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status  result;
        Lib_Memspace     old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {

	    TMObj_Symbol     s;
    
	    s= get_dynamic_symbol(handle->object,symbol);
    
	    if (s==Null) {
		  result= TMDwnLdr_SymbolIsUndefined;
	    } else 
    
	    if (s->type != TMObj_RelativeSymbol) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (s->attr.marker.section->is_code) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (!s->attr.marker.section->has_data) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else {
    
		  TMScatter_put_word( 
		      (Pointer)s->attr.marker.offset,
		      Null,
		      s->attr.marker.section->endian,
		      (TMObj_Word)value
		  );
    
		  result= TMDwnLdr_OK;
	    }

	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}




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
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status  result;
        Lib_Memspace     old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {

	    TMObj_Symbol     s;
    
	    s= get_dynamic_symbol(handle->object,symbol);
    
	    if (s==Null) {
		  result= TMDwnLdr_SymbolIsUndefined;
	    } else 
    
	    if (s->type != TMObj_RelativeSymbol) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (s->attr.marker.section->is_code) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (!s->attr.marker.section->has_data) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else {
    
		  *value=
		      TMScatter_get_word( 
			  (Pointer)s->attr.marker.offset,
			  Null,
			  s->attr.marker.section->endian
		      );
    
		  result= TMDwnLdr_OK;
	    }

	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}




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
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status  result;
        Lib_Memspace     old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {
	    TMObj_Symbol     s;
    
	    s= get_download_symbol(handle->object,symbol);
    
	    if (s==Null) {
		  result= TMDwnLdr_SymbolIsUndefined;
	    } else 
    
	    if (s->type != TMObj_UnresolvedSymbol) {
		  result= TMDwnLdr_SymbolNotADownloadParm;
	    } else 
    
	    if (!s->attr.unresolved.is_download_parameter) {
		  result= TMDwnLdr_SymbolNotADownloadParm;
	    } else {
    
		s->type       = TMObj_AbsoluteSymbol;
		s->attr.value = value;
		result        = TMDwnLdr_OK;
	    }
	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}




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
               UInt32                   *resultp
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status  result;
        Lib_Memspace     old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {
	    TMObj_Symbol     s;
    
	    s= get_dynamic_symbol(handle->object,symbol);
    
	    if (s==Null) {
		  result= TMDwnLdr_SymbolIsUndefined;
	    } else 
    
	    if (s->type != TMObj_RelativeSymbol) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (s->attr.marker.section->is_code) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else 
    
	    if (!s->attr.marker.section->has_data) {
		  result= TMDwnLdr_SymbolNotInInitialisedData;
	    } else {
    
	       *resultp=
		      TMScatter_get_word( 
			  (Pointer)s->attr.marker.offset,
			  Null,
			  s->attr.marker.section->endian
		      );
	
		result= TMDwnLdr_OK;
	    }
	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}



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
    )
{
    Lib_Msg_clear_last_fatal_message();

    if ( object_handles != Null
        && Lib_Set_remove(object_handles, handle)
       ) {
        FlatObject_close_object(handle->object);
        Lib_IOD_close(handle->driver);

        if (handle->sstab) {
            TMDwnLdr_unload_shared_section_table(handle->sstab);
        }
        if (handle->driver_space) {
            Lib_Memspace_delete(handle->driver_space);
        }

        Lib_Memspace_delete(handle->space); /* this deallocates the handle itself,
                                             * as well as the loaded module.
                                             * So do it as very last.
                                             */
        return TMDwnLdr_OK;
    } else {
        return TMDwnLdr_HandleNotValid;
    }
}





        void copy_section( TMDwnLdr_Section_Rec  *s1, TMObj_Section s2 )
        {
	    s1->name         =           s2->name;
	    s1->bytes        =           s2->bytes;
	    s1->size         =           s2->size;
	    s1->alignment    =           s2->alignment;
	    s1->big_endian   =           s2->endian == BigEndian;
	    s1->has_data     =           s2->has_data;
	    s1->is_code      =           s2->is_code;
	    s1->is_read_only =           s2->is_read_only;
	    s1->caching      =           s2->caching_property;
	    s1->relocation   = (Address) s2->relocation_offset;
        }


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
    )
{
    Lib_Msg_clear_last_fatal_message();

    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status result;
        Lib_Memspace    old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {

	    TMObj_Section s= FlatObject_get_section(
				     (FlatObject_Module)(handle->object),
				     name
			     );
    
	    if (s == Null) {
		result= TMDwnLdr_NotFound;
	    } else {
		copy_section(section,s);
		result= TMDwnLdr_OK;
	    }
	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}



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

   /* The 'select_sectiongroup' mechanism has grown a little 
    * bit out of hand for this: it is used here as
    * an identifier in which section group we are currently
    * interested: cached, locked text, locked data, uncached data.
    * We do a traversal for each of these groups separately because
    * the sections do not necessarily appear in this grouping order
    * in the object file.
    */

    void connect_fun( TMObj_Section section, Address *data )
    {
        if ( data[2] == (Address)select_sectiongroup(section, 0) ) {
            TMDwnLdr_Section_Rec s;
            copy_section(&s, section);
            ((TMDwnLdr_Section_Fun)(data[0]))(&s,data[1]);
        }
    }



TMDwnLdr_Status
    TMDwnLdr_traverse_sections(
               TMDwnLdr_Object_Handle    handle,
               TMDwnLdr_Section_Fun      fun,
               Pointer                   data
    )
{
    Lib_Msg_clear_last_fatal_message();

    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        TMDwnLdr_Status    result;
        Lib_Memspace       old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {

	    Address dd[3];
	    
	    dd[0]= (Address)fun;
	    dd[1]= (Address)data;
    
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, cached_text);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, cached_data_i);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, cached_data_u);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, locked_text);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, locked_data_i);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, locked_data_u);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, uncached_data_i);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
    
	       /* --- . --- */
    
	    dd[2]= (Address)LOFFSET_OF(SectionGroupInfo, uncached_data_u);
    
	    FlatObject_traverse_sections(
			 (FlatObject_Module)(handle->object),
			 (FlatObject_Section_Fun)connect_fun, 
			 TMObj_DynamicLoading_Life, TMObj_LAST_LIFE,
			 dd
		      );
    
	    result= TMDwnLdr_OK;

	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
    }
}




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
    )
{
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
       *endian= handle->endian;
        return TMDwnLdr_OK;
    }
}




/*--------------------------- Symbol Table Functions -------------------------*/



static String
    unique_string(
        String         string,
        Lib_Memspace   stringmapspace,
        Lib_Mapping    stringmap
    )
{
    Lib_Memspace  old_space;
    String        result;

    if (string == Null) return Null;

    result= Lib_Mapping_apply( stringmap, string );

    if (result != Null) return result;
    
    result= COPY_STRING(string);

    old_space = Lib_Memspace_set_default( stringmapspace );
    Lib_Mapping_define( stringmap, result, result );
    Lib_Memspace_set_default( old_space );

    return result;
}


static TMDwnLdr_Symbol_Type
    get_type( TMObj_Symbol symbol )
{
    switch (symbol->type) {

    case TMObj_UnresolvedSymbol:
            return TMDwnLdr_UnresolvedSymbol;

    case TMObj_AbsoluteSymbol:
            return TMDwnLdr_AbsoluteSymbol;

    case TMObj_RelativeSymbol:
            return TMDwnLdr_RelativeSymbol;

    case TMObj_CommonSymbol:
            assert(False);
    }
}


static TMDwnLdr_Symbol_Scope
    get_scope( TMObj_Symbol symbol )
{
    switch (symbol->scope) {

    case TMObj_LocalScope:
            return TMDwnLdr_LocalScope;

    case TMObj_GlobalScope:
            return TMDwnLdr_GlobalScope;

    case TMObj_DynamicScope:
            return TMDwnLdr_DynamicScope;
    }
}




static SymtabSearch 
    create_symbtab( FlatObject_Module module )
{
        Int                      next_sym_index;
        TMObj_Section_Lifetime   life;
        Int                      nrof_symbols;
        SymtabSearch_Element     symtable;

        Lib_Memspace             stringmapspace;
        Lib_Memspace             old_space;
        Lib_Mapping              stringmap;

       /*
        * count nr of symbols, and 
        * allocate SymtabSearch array:
        */
        nrof_symbols= 0;

        for (life= TMObj_Unused_Life; life <= TMObj_Permanent_Life; life++) {
            TMObj_Section s;

            s= FlatObject_get_system_section(
                                   module,
                                   TMObj_Symbol_Section,
                                   life
               );

            if (s != Null) {
                Address       base      = s->bytes;
                TMObj_Symbol  symbol    = (TMObj_Symbol) base;
                TMObj_Symbol  ceiling   = (TMObj_Symbol) (base + s->size);
                nrof_symbols += (ceiling - symbol);
            }
        }

        symtable= Lib_Mem_MALLOC( nrof_symbols * sizeof(struct SymtabSearch_Element) );


       /*
        * Fill SymtabSearch array,
        * but leave out symbols which have been
        * resolved to external sections.
        */
        next_sym_index= 0;

        for (life= TMObj_Unused_Life; life <= TMObj_Permanent_Life; life++) {
            TMObj_Section s;

            s= FlatObject_get_system_section(
                                   module,
                                   TMObj_Symbol_Section,
                                   life
               );

            if (s != Null) {
                Address       base      = s->bytes;
                TMObj_Symbol  symbol = (TMObj_Symbol) base;
                TMObj_Symbol  ceiling   = (TMObj_Symbol) (base + s->size);

                while (symbol < ceiling) {
                    symtable[next_sym_index].address = (Address)TMRelocate_symbol_value(symbol);
                    symtable[next_sym_index].name    = symbol->name;
                    symtable[next_sym_index].type    = get_type  ( symbol );
                    symtable[next_sym_index].scope   = get_scope ( symbol );
                   
                    switch (symbol->type) {

                    case TMObj_RelativeSymbol:
                        symtable[next_sym_index].section= symbol->attr.marker.section->name;
                        break;

                    case TMObj_AbsoluteSymbol:
                        symtable[next_sym_index].section= Null;
                        break;

                    case TMObj_UnresolvedSymbol:
                        symtable[next_sym_index].section= Null;
                        break;
                    }

                    next_sym_index++;
                    symbol++;
                }
            }
        }
        assert(next_sym_index == nrof_symbols);


       /*
        * Copy strings over:
        */

        stringmapspace = Lib_Memspace_new( MEMSPACE_INCREMENT );
        CHECK( stringmapspace != Null, (LibMsg_Memory_Overflow,0) ){}

        Lib_Excp_try {
            old_space = Lib_Memspace_set_default( stringmapspace );

	    stringmap= 
		Lib_Mapping_create( 			
		   (Lib_Local_BoolResFunc)Lib_StdFuncs_string_equal, 
		   (Lib_Local_EltResFunc )Lib_StdFuncs_string_hash, 
		   1000	
		);

            Lib_Memspace_set_default( old_space );


            while ( --next_sym_index >= 0) {
                symtable[next_sym_index].name= 
                              unique_string(
                                    symtable[next_sym_index].name,
                                    stringmapspace,
                                    stringmap
                              );
                symtable[next_sym_index].section=
                              unique_string(
                                    symtable[next_sym_index].section,
                                    stringmapspace,
                                    stringmap
                              );
            }
        } 
        Lib_Excp_otherwise {
            if (stringmapspace!=Null) Lib_Memspace_delete(stringmapspace);
            Lib_Memspace_set_default( old_space );
            CHECK( False, (LibMsg_Memory_Overflow,0) ){}
        }
        Lib_Excp_end_try;

       /*
        * Cleanup and return result
        */
        Lib_Memspace_delete(stringmapspace);

        return SymtabSearch_create(nrof_symbols, symtable);
}



static TMDwnLdr_Status preload_symbol_sections( TMDwnLdr_Object_Handle handle )
{
        TMObj_Section_Lifetime   life;
        TMDwnLdr_Status         result;

        Lib_Memspace  old_space= Lib_Memspace_set_default( handle->space );
       /*---------------------------------------------------------------*/
	Lib_Excp_try {
	    for (life= TMObj_Unused_Life; life <= TMObj_Permanent_Life; life++) {
	       FlatObject_get_system_section( handle->object, TMObj_Symbol_Section, life );
	    }
    
	    result= TMDwnLdr_OK;
	} 
       /* ------------------------------------------------- */
	Lib_Excp_otherwise {
	    result= map_error(___Lib_Excp_raised_excep___);
	}
	Lib_Excp_end_try;
       /*---------------------------------------------------------------*/
        Lib_Memspace_set_default( old_space );

        return result;
}





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
    )
{
    TMDwnLdr_Status   status;
    Lib_Memspace      space, old_space;

    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( object_handles == Null
       || !Lib_Set_element(object_handles, object)
       ) {
        return TMDwnLdr_HandleNotValid;
    }

   /*
    * preload all used sections, making
    * sure that they end up on the object's
    * memory space:
    */
    status= preload_symbol_sections(object);

    if (status != TMDwnLdr_OK) return status;

   /*
    * Create a new memory space, and install it as default
    * space for the duration of this function call. Be careful
    * herafter: protect the rest of this with exception handlers
    * so that the memspace, with all allocated memory, can be deleted
    * when anything goes wrong. All Lib_Mem operations operate on
    * the default memspace.
    */
    space= Lib_Memspace_new( MEMSPACE_INCREMENT );

    if (space == Null) { return TMDwnLdr_InsufficientMemory; }
    old_space= Lib_Memspace_set_default( space );

   /* ------------------------------------------------- */
    Lib_Excp_try {
        InternalHandle  handle;

       /*
        * create- and define new handle:
        */
        Lib_Mem_NEW(handle);
        handle->space  = space;
        handle->object = create_symbtab( (FlatObject_Module)object->object );
        
        if (symbtab_handles == Null) { symbtab_handles= create_handle_set(); }
        Lib_Set_insert(symbtab_handles, handle);

       /*
        * decide success:
        */
       *result= handle;
        status= TMDwnLdr_OK;
    } 
   /* ------------------------------------------------- */
    Lib_Excp_otherwise {
        status= map_error(___Lib_Excp_raised_excep___);
    }
    Lib_Excp_end_try;

   /* ------------------------------------------------- */

    Lib_Memspace_set_default( old_space );

    if (status != TMDwnLdr_OK) {
        Lib_Memspace_delete(space);
    }

   /* ------------------------------------------------- */

    return status;
}





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
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( symbtab_handles == Null
       || !Lib_Set_element(symbtab_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
       SymtabSearch_Element result;

       result= SymtabSearch_get_from_name(
                   (SymtabSearch)handle->object,
                   symbol
               );

       if (result==Null) {
           return TMDwnLdr_SymbolIsUndefined;
       } else {
           *section  = result->section;
           *address  = result->address;

           return TMDwnLdr_OK;
       }
    }
}




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
    )
{
    Lib_Msg_clear_last_fatal_message();

   /*
    * Check handle validity:
    */
    if ( symbtab_handles == Null
       || !Lib_Set_element(symbtab_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
       SymtabSearch_Element result;

       result= SymtabSearch_get_from_address(
                   (SymtabSearch)handle->object,
                   address
               );

       if (result==Null) {
           return TMDwnLdr_SymbolIsUndefined;
       } else {
           *section        = result->section;
           *symbol         = result->name;
           *symbol_address = result->address;

           return TMDwnLdr_OK;
       }
    }
}




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
    )
{
    Lib_Msg_clear_last_fatal_message();

    if ( symbtab_handles == Null
       || !Lib_Set_element(symbtab_handles, handle)
       ) {
        return TMDwnLdr_HandleNotValid;
    } else {
        SymtabSearch_traverse_symbols( 
               (SymtabSearch)handle->object,
               order,
               fun,
               data
        );

        return TMDwnLdr_OK;
    }
}




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
    )
{
    Lib_Msg_clear_last_fatal_message();

    if ( symbtab_handles != Null
       && Lib_Set_remove(symbtab_handles, handle)
       ) {
        Lib_Memspace_delete(handle->space);
        return TMDwnLdr_OK;
    } else {
        return TMDwnLdr_HandleNotValid;
    }
}




/*
 * Function         : Install your own memory manager and memcpy routine. 
 *                    Should be called only once.
 *                    For memory management, only the memory interface
 *                    is used.
 *                    For copying the sections to SDRAM, the specified 
 *                    memcpy function is used.
 * Parameters       : memcpy_fun (I) memcpy function with default interface
 *                    malloc_fun (I) malloc function with default interface
 *                    free_fun   (I) free   function with default interface
 * Function Result  : TMDwnLdr_OK
 *                    TMDwnLdr_UnexpectedError, when it is called after
 *                    other calls to the downloader library are made.
 * Postcondition    : 
 */


TMDwnLdr_Status 
    TMDwnLdr_install_kernel_functions( 
		TMDwnLdr_MemCpyFunc memcpy_fun, 
		TMDwnLdr_MemSetFunc memset_fun, 
		TMDwnLdr_MallocFunc malloc_fun, 
		TMDwnLdr_FreeFunc   free_fun
    )
{
	Lib_Msg_clear_last_fatal_message();

	if ( memcpy_fun != Null ) { MyMemCpy = memcpy_fun; }
	if ( memset_fun != Null ) { MyMemSet = memset_fun; }

	if ( malloc_fun == Null 
             || Lib_Memspace_init(malloc_fun, free_fun)
            ) {
		return TMDwnLdr_OK;
        } else {
	    	return TMDwnLdr_UnexpectedError;
        }
}



/*
 * Function         : return a string that contains the last error message 
 * Parameters       : error code (I) last returned error code
 * Function Result  : pointer to a string that contains an error description
 * Postcondition    : String is owned by the library.
 */

String 
    TMDwnLdr_get_last_error( TMDwnLdr_Status status )
{
	String s;

	s = Lib_Msg_get_last_fatal_message();

	if ( s[0] != '\0' ) {
		return s;
	}

	switch (status) {
	case TMDwnLdr_OK			: return "no error";
	case TMDwnLdr_UnexpectedError		: return "unexpected error";
	case TMDwnLdr_InputFailed		: return "reading object failed";
	case TMDwnLdr_InsufficientMemory	: return "insufficient memory";
	case TMDwnLdr_NotABootSegment		: return "not a boot segment";
	case TMDwnLdr_InconsistentObject	: return "inconsistent object";
	case TMDwnLdr_UnknownObjectVersion	: return "unknown object version";
	case TMDwnLdr_NotFound			: return "not found";
	case TMDwnLdr_UnresolvedSymbols 	: return "unresolved symbols";
	case TMDwnLdr_SymbolIsUndefined 	: return "symbol is undefined";
	case TMDwnLdr_SymbolNotInInitialisedData: return "symbol is not in initialized data";
	case TMDwnLdr_SDRamTooSmall		: return "SDRAM too small";
	case TMDwnLdr_SDRamImproperAlignment	: return "improper alignment";
	case TMDwnLdr_SymbolNotADownloadParm	: return "symbol not a download parameter"; 
	case TMDwnLdr_HandleNotValid		: return "invalid handle";
	case TMDwnLdr_EndianMismatch		: return "objects do not all have the same endianness";
	case TMDwnLdr_ImagePackingRequired	: return "uncached data requires image packing";
	default					: return "Unknown error";
	}
	
}




void TMDwnLdr_set_unshuffled( TMDwnLdr_Object_Handle    handle )
{
    handle->shuffle_image= False;
}


void TMDwnLdr_get_text_positions( TMDwnLdr_Object_Handle handle, 
                                  Int *text_len, Int *locked_text_len, Int *locked_text_base)
{
    *text_len         = handle->sectiongroup_sizes.cached_text;
    *locked_text_len  = handle->sectiongroup_sizes.locked_text;
    *locked_text_base = handle->locked_text_image_base;
}


