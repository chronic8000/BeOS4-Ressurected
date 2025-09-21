#ifndef BEOS_COMPAT_H
#define BEOS_COMPAT_H

// Prevent redefinition warnings
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Fix ssize_t before any BeOS headers
#include <sys/types.h>
#include <unistd.h>
#ifndef ssize_t
typedef long ssize_t;
#endif

// Forward declarations for BeOS types
class _CEventPort_;
#define B_MAX_CPU_COUNT 8

// POSIX function fixes
extern "C" {
    ssize_t read(int fd, void *buf, size_t count);
    ssize_t write(int fd, const void *buf, size_t count);
}

#endif
