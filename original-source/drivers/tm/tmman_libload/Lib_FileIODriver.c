/*
 *  COPYRIGHT (c) 1997 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THIS COPY RIGHT NOTICE.     |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : Lib_FileIODriver.c    1.4
 *
 *  Last update              : 16:21:20 - 97/05/14
 *
 *  Description              :  
 *
 */

/*---------------------------- Includes --------------------------------------*/

#include "Lib_IODrivers.h"
#include "Lib_Local.h"

#include "../be_files.h"
//DL
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/

//#ifdef UNDER_CE
    #define FILE_POINTER (FILE*)
//#else
//    #define FILE_POINTER  
//#endif


/*--------------------------- Functions --------------------------------------*/

/*
this 'static' function is never used within this file
    static Int32 file_seek  ( Int32 fd, Int32 offset, Lib_IOD_SeekMode mode)
    {

        
    switch (mode) {
	case Lib_IOD_SEEK_SET : return lseek(FILE_POINTER fd,offset,SEEK_SET);
	case Lib_IOD_SEEK_CUR : return lseek(FILE_POINTER fd,offset,SEEK_CUR);
	case Lib_IOD_SEEK_END : return lseek(FILE_POINTER fd,offset,SEEK_END);
        }
    }
*/


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
        )
{
#if defined (UNDER_CE)
    FILE* fd = fopen(path, "rb");
    if (fd == NULL) {

#elif defined (__DCC__)
    FILE* fd = fopen(path, "rb");
    if (fd == NULL) {

#elif defined (__BEOS__)
    FILE* fd = fopen(path, "rb");
    if (fd == NULL) {

#else
    Int32 fd= open(path, O_BINARY | O_RDONLY);
    if (fd == -1) {
#endif // UNDER_CE
        printf("Lib_IOD_open_file_for_read: Failure: %s\n", path);
        return Null;
    } 
    else 
    {
        printf("Lib_IOD_open_file_for_read: Success: %s pointer=%p\n", path, fd);
        return 
            Lib_IOD_create_driver(
#ifdef _MW_IDE
	        (Lib_IOD_CloseFun) tm_close,
	        (Lib_IOD_ReadFun ) tm_read,
	        (Lib_IOD_WriteFun) tm_write,
	        (Lib_IOD_SeekFun ) tm_lseek,
#elif defined (__BEOS__)
	        (Lib_IOD_CloseFun) be_close,
	        (Lib_IOD_ReadFun ) be_read,
	        (Lib_IOD_WriteFun) be_write,
	        (Lib_IOD_SeekFun ) be_seek,
#else
	        (Lib_IOD_CloseFun) close,
	        (Lib_IOD_ReadFun ) read,
	        (Lib_IOD_WriteFun) write,
	        (Lib_IOD_SeekFun ) lseek,
#endif
	        (Pointer         ) fd
            );
    }
}



