#ifndef CHECKMALLOC_H
#define CHECKMALLOC_H

#ifdef CHECKMALLOC

#include <sys/types.h>

extern void my_free(void *myp);
extern void *my_malloc(size_t size);
extern void my_memory_used();

#define free my_free
#define malloc my_malloc

#endif

#endif
