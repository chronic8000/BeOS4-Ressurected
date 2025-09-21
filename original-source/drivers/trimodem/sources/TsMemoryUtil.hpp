/*======================================================================== 
   File:    TsMemoryUtil.hpp 
   Purpose: New/Delete definitions used throughout the Generic Modem Core.

   Author:  Bryan Pong, July 2000 
   Modifications:
                    Copyright (C) 2000 by Trisignal communications Inc.
                                   All Rights Reserved 
=========================================================================*/ 
#ifndef TSMEMORYUTIL_HPP
#define TSMEMORYUTIL_HPP

//BeOS doesn't support new/delete functions

extern "C" { 
#include "malloc.h"
}

inline void *operator new(size_t nSize) 
{  
    void *p; 

    p = malloc(nSize); 
    return p; 
} 
 

inline void *operator new[](size_t nSize) 
{  
    void *p;
     
    p = malloc(nSize); 
    return p; 
} 
 
inline void operator delete(void* p) 
{  
    if (p) 
    { 
        free(p);
    } 
} 

inline void operator delete[](void* p) 
{  
    if (p) 
    { 
        free(p);
    } 
} 

#endif  // TSMEMORYUTIL_HPP
