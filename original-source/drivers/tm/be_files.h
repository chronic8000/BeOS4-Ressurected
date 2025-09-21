/* ========================================================================== */
#ifndef __INCLUDE_be_files_h
#define __INCLUDE_be_files_h
/* ========================================================================== */
#include "tmman_libload/Lib_IODrivers.h"
/* ========================================================================== */
Int32 be_close ( Lib_IODriver fd );
Int32 be_read  ( Lib_IODriver fd, Pointer buffer, Int32 size);
Int32 be_write ( Lib_IODriver fd, Pointer buffer, Int32 size);
Int32 be_seek  ( Lib_IODriver fd, Int32 offset,   Lib_IOD_SeekMode mode);
/* ========================================================================== */
#endif
/* ========================================================================== */

