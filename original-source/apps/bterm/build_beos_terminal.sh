#!/bin/bash

echo "🚀 BEOS TERMINAL AUTOMATED BUILD SCRIPT 🚀"
echo "==========================================="

# Set up environment
BEOS_ROOT="../../.."
INCLUDES="-I $BEOS_ROOT/headers -I $BEOS_ROOT/headers/app -I $BEOS_ROOT/headers/support -I $BEOS_ROOT/headers/posix -I $BEOS_ROOT/headers/kernel -I $BEOS_ROOT/headers/interface -I $BEOS_ROOT/headers/storage -I $BEOS_ROOT/headers/interface2 -I $BEOS_ROOT/headers/controls"

# Create comprehensive fix header
echo "🔧 Creating comprehensive BeOS compatibility header..."
cat > beos_compat.h << 'HEADER_EOF'
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
HEADER_EOF

echo "✅ Compatibility header created!"

# Try to compile main component we know works
echo "🎯 Compiling main_patched_v3.cpp (known working)..."
if g++ -include beos_compat.h $INCLUDES -c main_patched_v3.cpp -o main_patched_v3.o 2>/dev/null; then
    echo "✅ Main component compiled successfully!"
    ls -la main_patched_v3.o
else
    echo "❌ Main component failed, checking errors..."
    g++ -include beos_compat.h $INCLUDES -c main_patched_v3.cpp -o main_patched_v3.o 2>&1 | tail -5
fi

echo "🎉 Build script completed!"
