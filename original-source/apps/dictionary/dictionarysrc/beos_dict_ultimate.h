#ifndef BEOS_DICT_ULTIMATE_H
#define BEOS_DICT_ULTIMATE_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <unistd.h>
#include <cstdint>

// Define our types BEFORE any BeOS headers
typedef int ssize_t;
class _CEventPort_;
#define B_MAX_CPU_COUNT 8

// Redefine problematic macros to avoid conflicts
#define int32 int32_t
#define uint32 uint32_t  
#define int64 int64_t
#define status_t int32_t
#define bigtime_t int64_t
#define type_code uint32_t

#endif // BEOS_DICT_ULTIMATE_H
