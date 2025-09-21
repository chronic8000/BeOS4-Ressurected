
//******************************************************************************
//
//	File:		support.h
//
//	Description:	Some useful classes/functions
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef AS_SUPPORT_H
#define AS_SUPPORT_H

#include <memory.h>
#include <malloc.h>
#include "as_debug.h"

#define MALLOC_LOW				1
#define MALLOC_MEDIUM			2
#define MALLOC_HIGH				3
#define MALLOC_CANNOT_FAIL		4

#ifdef INTERNAL_LEAKCHECKING

#define grMalloc(a,b,c)		grLeakyMalloc(a,b,c)
#define grFree(a)			grLeakyFree(a)
#define grCalloc(a,b,c,d)	grLeakyCalloc(a,b,c,d)
#define grRealloc(a,b,c,d)	grLeakyRealloc(a,b,c,d)
#define grStrdup(a,b,c)		grLeakyStrdup(a,b,c)

#ifdef	__cplusplus
extern "C" {
#endif

void *					grLeakyMalloc(int32 size, char *comment, int32 priority);
void					grLeakyFree(void *ptr);
void *					grLeakyCalloc(int32 size1, int32 size2, char *comment, int32 priority);
void *					grLeakyRealloc(void *ptr, int32 size, char *comment, int32 priority);
char *					grLeakyStrdup(const char *srcStr, char *comment, int32 priority);

#ifdef	__cplusplus
}
#endif

#else

#define grMalloc(a,b,c)		malloc(a)
#define grFree(a)			free(a)
#define grCalloc(a,b,c,d)	calloc(a,b)
#define grRealloc(a,b,c,d)	realloc(a,b)
#define grStrdup(a,b,c)		strdup(a)

#endif

#ifdef	__cplusplus
#include "BArray.h"
#endif

#endif
