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
 *  Module name              : Lib_Memspace.h    1.16
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Memory Space Management
 *
 *  Last update              : 11:17:39 - 98/03/26
 *
 *  Description              : 
 *            
 *             This module provides for the creation of memory spaces, 
 *             allocation from specific spaces, and for the deletion of 
 *             a memory space as a whole.  
 */


#ifndef Lib_Memspace_INCLUDED
#define Lib_Memspace_INCLUDED



/*--------------------------------- Includes: ------------------------------*/

#include "tmtypes.h"

typedef Pointer   (*Lib_Memspace_MallocFunc)	(Int size);
typedef void  (*Lib_Memspace_FreeFunc)		(Pointer);


/*---------------------------------- Types ---------------------------------*/

/*
 * Handle to a read only memory space created by Lib_Memspace_new
 */

typedef struct Lib_Memspace *Lib_Memspace;

/*
 * Default memory space. Can be changed to another 
 * on the fly created memory space.
 * The default memory space needs to be created by the 
 * user of the library and installed with Lib_Memspace_set_default
 */

extern Lib_Memspace	Lib_Memspace_default_space;

/*--------------------------------- Functions ------------------------------*/


/*
 * Function         : install your own memory maneger, must be done
 *                    before any other call to Lib_Memspace.
 * Parameters       : malloc_fun (I)	malloc routine
 *                    free_fun   (I)    free_fun
 * Function result  : True is initialization is OK, False
 *                    when another call was already made to Lib_Memspace      
 */

extern Bool  
Lib_Memspace_init( Lib_Memspace_MallocFunc malloc_fun, Lib_Memspace_FreeFunc free_fun );


/*
 * Function         : Create new memory space.
 * Parameters       : increment_size (I) size to increment large chunks
 * Function result  : Returned new space handle      
 */

extern Lib_Memspace  
Lib_Memspace_new( UInt increment_size );


/*
 * Function         : Delete a proviously allocated memory space,
 *                    and give all the related memory back to the
 *                    underlying memory manager.   
 * Parameters       : space  (IO)  memory space to be deleted.
 * Function result  : -        
 */

extern void
Lib_Memspace_delete( Lib_Memspace space );


/*
 * Function         : Attempt allocation of memory
 * Parameters       : space    (I) space to allocate from
 *                    size     (I) size in bytes of requested
 *                                 memory block
 * Function result  : Address of requested block 
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : In case no memory could be allocated, an exception
 *                    is generated, and this function does not return.
 */

extern Pointer
Lib_Memspace_malloc( Lib_Memspace space, Int size );


/*
 * Function         : free previously allocated memory
 * Parameters       : addr (I) address
 * Function result  : - 
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -
 */

extern void
Lib_Memspace_free( Pointer address );



/*
 * Function         : change the size of a memory block, return pointer
 *                    to the new space
 * Parameters       : addr (I) address
 * Function result  : - 
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -
 */

extern Pointer
Lib_Memspace_realloc( Pointer address, Int size );



/*
 * Function         : allocate space for an array and zero
 * Parameters       : space    (I) space to allocate from
 *                    nelms (I) nrof elements in the array
 *                    size  (I) size of one element
 * Function result  : - 
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -
 */

extern Pointer
Lib_Memspace_calloc( Lib_Memspace space, UInt nelms, UInt size );




/*
 * Function         : change the default memory space
 * Parameters       : mem_space (I) the new memory space 
 *                                  to allocate from
 * Function result  : returns the old default space
 * Precondition     : -
 * Postcondition    : -      
 * Sideeffects      : -
 */

extern Lib_Memspace
Lib_Memspace_set_default( Lib_Memspace mem_space );



#endif /* Lib_Memspace_INCLUDED */
     
   
