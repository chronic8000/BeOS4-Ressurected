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
 *  Module name              : Lib_IODrivers.h    1.9
 *
 *  Last update              : 10:37:40 - 97/08/06
 *
 *  Description              :  
 *
 */

#ifndef  Lib_IOD_INCLUDED
#define  Lib_IOD_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "tmtypes.h"

/*-------------------------- Definitions -------------------------------------*/

typedef struct Lib_IODriver_t *Lib_IODriver;

typedef enum {
    Lib_IOD_SEEK_SET,
    Lib_IOD_SEEK_CUR,
    Lib_IOD_SEEK_END,
} Lib_IOD_SeekMode;

typedef Int32 (*Lib_IOD_CloseFun) ( Pointer fd );
typedef Int32 (*Lib_IOD_ReadFun ) ( Pointer fd, Pointer buffer, Int32 size);
typedef Int32 (*Lib_IOD_WriteFun) ( Pointer fd, Pointer buffer, Int32 size);
typedef Int32 (*Lib_IOD_SeekFun ) ( Pointer fd, Int32 offset,   Lib_IOD_SeekMode mode);


/*--------------------------- Functions --------------------------------------*/


/*
 * Function         : Create an IO driver by opening the specified
 *                    file, for reading.
 * Parameters       : path   (I) name of file to open
 * Function Result  : A new driver attached to the file, 
 *                    or Null when opening the file 
 *                    for reading failed.
 */

Lib_IODriver 
	Lib_IOD_open_file_for_read  ( 
	    String    path 
        );


/*
 * Function         : Create an IO driver attached to the 
 *                    specified memory buffer, for reading.
 *                    NB: closing the driver will *not*
 *                        deallocate this memory buffer.
 * Parameters       : memory  (I) start of memory buffer
 *                    size    (I) size of contents which
 *                                can be read
 * Function Result  : 
 */

Lib_IODriver 
	Lib_IOD_open_mem_for_read   ( 
            Pointer   memory, 
            Int32     size    
        );


/*
 * Function         : Constructor for a driver.
 * Parameters       : close  (I) function for whatever cleanup is
 *                               needed when closing the driver.
 *                    read   (I) its read function 
 *                    write  (I) its write function
 *                    seek   (I) its seek function
 *                    data   (I) Value for keeping instance specific data
 * Function Result  : A new driver from the specified data
 */

Lib_IODriver 
	Lib_IOD_create_driver ( 
	    Lib_IOD_CloseFun    close,
	    Lib_IOD_ReadFun     read,
	    Lib_IOD_WriteFun    write,
	    Lib_IOD_SeekFun     seek,
	    Pointer             data
        );



/* 
 * Function: Driver access functions
 *           NB: Close invalidates its argument.
 */

Int32 Lib_IOD_close ( Lib_IODriver fd );
Int32 Lib_IOD_read  ( Lib_IODriver fd, Pointer buffer, Int32 size);
Int32 Lib_IOD_write ( Lib_IODriver fd, Pointer buffer, Int32 size);
Int32 Lib_IOD_seek  ( Lib_IODriver fd, Int32 offset,   Lib_IOD_SeekMode mode);


#endif /* Lib_IOD_INCLUDED */
     
   
