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
 *  Module name              : TMTrgLoader.h    1.24
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 10:53:12 - 98/04/16
 *
 *  Description              :  
 *
 */

#ifndef  TMTrgLoader_INCLUDED
#define  TMTrgLoader_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "TMObj.h"

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

/*--------------------------- Definitions ------------------------------------*/

/*
 * Result status values for the exported functions;
 * !!! keep consistent with DynamicLoader !!!
 */

typedef enum {
       TMTrgLoader_OK                           =  0,

       TMTrgLoader_FileNotFound                 =  1,
       TMTrgLoader_InsufficientMemory           =  2,
       TMTrgLoader_InconsistentObject           =  3,
       TMTrgLoader_UnknownObjectVersion         =  4,
       TMTrgLoader_WrongEndian                  =  5,
       TMTrgLoader_WrongChecksum                =  6,
       TMTrgLoader_NotUnloadable                =  7,
       TMTrgLoader_UnresolvedSymbol             =  8,
       TMTrgLoader_NotADll                      =  9,
       TMTrgLoader_NotAnApp                     = 10,
       TMTrgLoader_NotPresent                   = 11,
       TMTrgLoader_StillReferenced              = 12,
       TMTrgLoader_StackOverflow
} TMTrgLoader_Status;


typedef Pointer         (*TMTrgLoader_MallocFun)(UInt);
typedef void            (*TMTrgLoader_FreeFun)(Pointer);
typedef void  (*TMTrgLoader_ErrorFun)(TMTrgLoader_Status,String);


/*--------------------------- Functions --------------------------------------*/



/*
 * Function         : Read an application segment from file into memory, and
 *                    return a handle for subsequent operations.
 *                    Contrary to dlls, location of application object
 *                    files is not subject to any lookup mechanism;
 *                    for this reason a path must be used for specifying
 *                    the application file. The `path' here is the text string
 *                    which could be used in calls to 'open` to open the 
 *                    application object file.
 *                    Also contrary to dlls, duplicate copies of apps are 
 *                    allowed, and therefore subsequent load calls
 *                    with the same path value result in different,
 *                    independent loaded applications.
 *                    The transfer address of a loaded application 
 *                    can be found in the `start' field of the returned
 *                    module descriptor. Loaded applications can be unloaded
 *                    by means of a call to TMTrgLoader_unload_segment.
 * Parameters       : path   (I)  Name of executable file to be loaded.
 *                    result (O)  Returned handle, or undefined when
 *                                result unequal to TMTrgLoader_OK.
 * Function Result  : TMTrgLoader_OK
 *                    TMTrgLoader_FileNotFound 
 *                    TMTrgLoader_InsufficientMemory 
 *                    TMTrgLoader_InconsistentObject 
 *                    TMTrgLoader_UnknownObjectVersion 
 *                    TMTrgLoader_WrongEndian
 *                    TMTrgLoader_WrongChecksum 
 *                    TMTrgLoader_UnresolvedSymbol 
 *                    TMTrgLoader_NotAnApp 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc, or via
 *                    the user specified memory manager
 *                    (see function TMTrgLoader_swap_mm).
 */

TMTrgLoader_Status 
    TMTrgLoader_load_application( 
               String                path,
               TMObj_ModuleDescr    *result
    );




/*
 * Function         : Unload specified application from memory.
 *                    This function will fail if the segment does not
 *                    correspond with an application segment, or if
 *                    the application's code is still in use 
 *                    (e.g. when it contains a still installed 
 *                    interrupt handler, or when other tasks are still
 *                    executing its code, or when it has been bound
 *                    by a call to AppModel_bind_codeseg.
 * Parameters       : segment (I)  descriptor of application to unload.
 * Function Result  : TMTrgLoader_OK
 *                    TMTrgLoader_NotAnApp 
 *                    TMTrgLoader_StillReferenced
 */

TMTrgLoader_Status 
    TMTrgLoader_unload_application( TMObj_ModuleDescr segment );
 



/*
 * Function         : Locate specified dll, load it into memory
 *                    when not already loaded, and return a handle for 
 *                    subsequent operations. The dll is marked as
 *                    being in use, preventing it from being unloaded,
 *                    until a matching call to TMTrgLoader_unbind_dll.
 *
 *                    Contrary to applications, which must be loaded
 *                    with complete path specification, dlls are subject
 *                    to a lookup mechanism; for this reason, no path
 *                    specification is allowed. Instead, just the base
 *                    file name need be given.
 *
 * Parameters       : name   (I)  Name of dll to be loaded. No path
 *                                specification is allowed.
 *                    result (O)  Returned handle, or undefined when
 *                                result unequal to TMTrgLoader_OK.
 * Function Result  : TMTrgLoader_OK
 *                    TMTrgLoader_FileNotFound 
 *                    TMTrgLoader_InsufficientMemory 
 *                    TMTrgLoader_InconsistentObject 
 *                    TMTrgLoader_UnknownObjectVersion 
 *                    TMTrgLoader_WrongEndian
 *                    TMTrgLoader_WrongChecksum 
 *                    TMTrgLoader_UnresolvedSymbol 
 *                    TMTrgLoader_NotADll 
 * Sideeffects      : Memory to hold the result  
 *                    is allocated via malloc, or via
 *                    the user specified memory manager
 *                    (see function TMTrgLoader_swap_mm).
 */

TMTrgLoader_Status 
    TMTrgLoader_bind_dll( 
               String                name,
               TMObj_ModuleDescr    *result
    );



/*
 * Function         : Remove usage mark from dll.
 * Parameters       : name (I)  name of dll to unbind
 * Function Result  : TMTrgLoader_OK
 *                    TMTrgLoader_NotPresent
 */

TMTrgLoader_Status 
    TMTrgLoader_unbind_dll( String  name );
 


/*
 * Function         : Unload specified dll from memory, 
 *                    together with all other dlls that make
 *                    'immediate' use of it. 
 *                    Unloading will fail if any of these dlls
 *                    contain code that is still in use 
 *                    (e.g. when it contains a still installed 
 *                    interrupt handler, or when other tasks are still
 *                    executing its code, or when it has been bound
 *                    by a call to AppModel_bind_codeseg or 
 *                    TMTrgLoader_bind_dll).
 * Parameters       : name (I)  name of dll to unload
 * Function Result  : TMTrgLoader_OK
 *                    TMTrgLoader_NotPresent
 *                    TMTrgLoader_StillReferenced
 */

TMTrgLoader_Status 
    TMTrgLoader_unload_dll( String name );
 

/*
 * Function         : Unload all currently unused dlls.
 * Parameters       : name (I)  name of dll to unload
 * Function Result  : TMTrgLoader_OK
 */

TMTrgLoader_Status 
    TMTrgLoader_unload_all( );




/*
 * Function         : Swap the permanent- and temporary
 *                    storage manager currently in use
 *                    by the dynamic loader.
 *                    Note: this function is for system's
 *                    purposes only, and should be called
 *                    before the dynamic loader has been active.
 * Parameters       : perm_malloc (IO)  malloc + free functions for
 *                    perm_free   (IO)    allocating storage for loaded 
 *                                         code segments
 *                    temp_malloc (IO)  malloc + free functions for
 *                    temp_free   (IO)    allocating storage for working 
 *                                         memory during dynamic loading
 */

void 
    TMTrgLoader_swap_mm( 
         TMTrgLoader_MallocFun *perm_malloc,
         TMTrgLoader_FreeFun   *perm_free,
         TMTrgLoader_MallocFun *temp_malloc,
         TMTrgLoader_FreeFun   *temp_free
    );
 


/*
 * Function         : Swap the (global) error handler that is to be
 *                    called upon failure in implicit dll loading
 *                    from function stubs.
 * Parameters       : stub_error_handler (IO)  function stub error handler
 */

void 
    TMTrgLoader_swap_stub_error_handler( 
         TMTrgLoader_ErrorFun *stub_error_handler
    );



/*
 * Function         : Mark the code segment that contains
 *                    the specified code address as used.
 *                    TMTrgLoader_bind_codeseg maintains a reference count. 
 * Parameters       : code (I)  code address (e.g. function ptr)
 * Function Result  : TMTrgLoader_OK
 */

TMObj_ModuleDescr
    TMTrgLoader_bind_codeseg( Address code );


/*
 * Function         : Mark the code segment that contains
 *                    the specified code address as no longer used.
 *                    TMTrgLoader_bind_codeseg maintains a reference count. 
 * Parameters       : code (I)  code address (e.g. function ptr)
 * Function Result  : TMTrgLoader_OK
 */

TMObj_ModuleDescr
    TMTrgLoader_unbind_codeseg( Address code );



#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif /* TMTrgLoader_INCLUDED */
