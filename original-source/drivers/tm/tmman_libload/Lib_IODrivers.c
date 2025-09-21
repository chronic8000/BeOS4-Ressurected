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
 *  Module name              : Lib_IODrivers.c    1.11
 *
 *  Last update              : 10:37:39 - 97/08/06
 *
 *  Description              :  
 *
 */

/*---------------------------- Includes --------------------------------------*/

#include "Lib_IODrivers.h"
#include "Lib_Local.h"

/*-------------------------- Definitions -------------------------------------*/

struct Lib_IODriver_t {
    Lib_IOD_CloseFun    close;
    Lib_IOD_ReadFun     read;
    Lib_IOD_WriteFun    write;
    Lib_IOD_SeekFun     seek;
    Pointer             data;
};

/*--------------------------- Functions --------------------------------------*/


    typedef struct RomFD {
        Address    buffer;
        UInt32     size;
        UInt32     pos;
    } *RomFD;


    static Int32 mem_close ( RomFD fd )
    { 
        Lib_Mem_FREE(fd);
        return 0;
    }
    
    
    static Int32 mem_read  ( RomFD fd, Address buffer, Int32 size)
    { 
        size = LMIN((unsigned) size, fd->size - fd->pos );
        size = LMAX( size, 0                  );

        memcpy( buffer, &fd->buffer[fd->pos], size );
        fd->pos += size;

        return size;
    }
    
    
    static Int32 mem_seek  ( RomFD fd, Int32 offset, Lib_IOD_SeekMode mode)
    { 
        UInt32 new_pos;

	switch (mode) {

	case Lib_IOD_SEEK_SET :
                 new_pos= offset; 

                 if (new_pos > fd->size) {
                     return -1;
                 } else {
                     fd->pos= new_pos;
                     return new_pos;
                 }

	case Lib_IOD_SEEK_CUR :
                 new_pos= fd->pos + offset; 

                 if (new_pos > fd->size) {
                     return -1;
                 } else {
                     fd->pos= new_pos;
                     return new_pos;
                 }

	case Lib_IOD_SEEK_END :
                 new_pos= fd->size;
                 fd->pos= new_pos;          
                 return new_pos;
	}
    }


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
        )
{
    RomFD fd;

    Lib_Mem_NEW(fd);

    fd->buffer= memory;
    fd->size  = size;
    fd->pos   = 0;

    return 
	Lib_IOD_create_driver(
	    (Lib_IOD_CloseFun) mem_close,
	    (Lib_IOD_ReadFun ) mem_read,
	    (Lib_IOD_WriteFun) Null,
	    (Lib_IOD_SeekFun ) mem_seek,
	    (Pointer         ) fd
	);
}


/*--------------------------- Functions --------------------------------------*/


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
        )
{
    Lib_IODriver result;

    Lib_Mem_NEW(result);

    result->close = close;
    result->read  = read;
    result->write = write;
    result->seek  = seek;
    result->data  = data;

    return result;
}



/* 
 * Function: Wrappers for calling the member function;
 *           NB: Close invalidates its argument.
 */

Int32 Lib_IOD_close ( Lib_IODriver fd )
{
#if defined(UNDER_CE) || defined(__DCC__)
    Int32 result;
    result= fclose((FILE*)fd->data);
#else
    Int32 result= fd->close(fd->data);
#endif
    Lib_Mem_FREE(fd);
    return result;
}


Int32 Lib_IOD_read  ( Lib_IODriver fd, Pointer buffer, Int32 size)
{
#if defined(UNDER_CE) || defined(__DCC__)
    return fread(buffer,sizeof(char),size,fd->data);
#else
    return fd->read(fd->data,buffer,size);
#endif
}


Int32 Lib_IOD_write ( Lib_IODriver fd, Pointer buffer, Int32 size)
{
#if defined(UNDER_CE) || defined(__DCC__)
    return fwrite(buffer,sizeof(char),size,fd->data);
#else
    return fd->write(fd->data,buffer,size);
#endif
}


Int32 Lib_IOD_seek  ( Lib_IODriver fd, Int32 offset, Lib_IOD_SeekMode mode)
{
#if defined(UNDER_CE) || defined(__DCC__)
    return fseek(fd->data,offset,mode);
#else
    return fd->seek(fd->data,offset,mode);
#endif
}



