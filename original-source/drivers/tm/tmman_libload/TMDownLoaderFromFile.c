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
 *  Module name              : TMDownLoaderFromFile.c    1.5
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : 
 *
 *  Author                   : Juul van der Spek 
 *
 *  Last update              : 15:44:53 - 98/04/20
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
#include "compr_shuffle.h"
#include "InternalHandle.h"

// #include <stdlib.h>

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
        driver= Lib_IOD_open_file_for_read(path);
    } 
    Lib_Excp_otherwise {
        driver = Null;
        status = TMDwnLdr_InsufficientMemory;
    }
    Lib_Excp_end_try;

    Lib_Memspace_set_default( old_space );

    if (driver == Null) {
        status = TMDwnLdr_InputFailed;
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



