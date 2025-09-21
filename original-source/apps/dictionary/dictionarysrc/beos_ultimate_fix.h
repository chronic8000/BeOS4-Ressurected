#ifndef BEOS_ULTIMATE_FIX_H
#define BEOS_ULTIMATE_FIX_H

// Prevent redefinition warnings
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Define ssize_t BEFORE any BeOS headers
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

// Also make sure it's defined for C++
#ifndef __cplusplus_ssize_t
#define __cplusplus_ssize_t
typedef long ssize_t;
#endif

// System includes
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// BeOS compatibility
class _CEventPort_;
#define B_MAX_CPU_COUNT 8

#endif
