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
 *  Module name              : InternalHandle.h    1.8
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Programming interface to dynamic loader
 *
 *  Last update              : 10:42:29 - 98/05/19
 *
 *  Description              :  
 *              
 *        non-exported definition of handle record for
 *        TMDownLoaded objects and loaded symboltables.
 *        Shared between TMDownLoader.c and TMDownLoaderFromFile.c
 */

#ifndef  InternalHandle_INCLUDED
#define  InternalHandle_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "tmtypes.h"
#include "Lib_Memspace.h"
#include "Lib_Exceptions.h"
#include "Lib_IODrivers.h"

/*--------------------------- Definitions ------------------------------------*/

/*
 * Data structures to traverse sections
 * in the different object manimplation 
 * routines; _i = initialised; _u = uninitialised
 */

typedef struct {
	Int   cached_text;
	Int   cached_data_i;
	Int   cached_data_u;
	Int   locked_text;
	Int   locked_data_i;
	Int   locked_data_u;
	Int   uncached_data_i;
	Int   uncached_data_u;
} SectionGroupInfo;




typedef struct {
        SectionGroupInfo  size;
        SectionGroupInfo  align;

        TMDwnLdr_SharedSectionTab_Handle  sstab;
} CountTraversalRec;




typedef struct {
        SectionGroupInfo  addr;

        TMDwnLdr_SharedSectionTab_Handle  sstab;
} RelocateTraversalRec;



typedef struct {
        SectionGroupInfo  virtual_base;
        SectionGroupInfo  physical_base;
        Address           copy_base;

        Bool                              shuffle_image;
        TMDwnLdr_SharedSectionTab_Handle  sstab;
} ExtractTraversalRec;


#define UNRESOLVED_SPACE  100

typedef struct InternalHandle {
    Lib_Memspace                        space;
    Pointer                             object;
    Endian                              endian;

    TMDwnLdr_SharedSectionTab_Handle    sstab;
    Lib_Memspace                        driver_space;
    Lib_IODriver                        driver;

    Char                                unresolved_symbols[UNRESOLVED_SPACE+10];
    Int                                 nrof_unresolved_symbols;
    Bool                                unpack_image;
    Bool                                shuffle_image;
    Int                                 refcount;
    
    SectionGroupInfo                    sectiongroup_sizes;
    SectionGroupInfo                    sectiongroup_alignments;
    SectionGroupInfo                    sectiongroup_positions;

    Int                                 locked_text_image_base;
} *InternalHandle;


#endif /* InternalHandle_INCLUDED */
     
   
