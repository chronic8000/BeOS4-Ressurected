#ifndef BEOS_COMPAT_FULL_H
#define BEOS_COMPAT_FULL_H

// Prevent redefinition warnings
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// System includes first
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Fix ssize_t
#ifndef ssize_t
typedef long ssize_t;
#endif

// Forward declarations for BeOS types
class _CEventPort_;
#define B_MAX_CPU_COUNT 8

// POSIX function stubs for missing functions
extern "C" {
    int ioctl(int fd, unsigned long request, ...);
}

#endif
