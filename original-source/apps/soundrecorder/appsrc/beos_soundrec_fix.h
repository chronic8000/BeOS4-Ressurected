#ifndef BEOS_SOUNDREC_FIX_H
#define BEOS_SOUNDREC_FIX_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
typedef int ssize_t;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef uint32_t type_code;

class _CEventPort_;
#define B_MAX_CPU_COUNT 8

#endif // BEOS_SOUNDREC_FIX_H
