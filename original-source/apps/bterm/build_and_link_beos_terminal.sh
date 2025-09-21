#!/bin/bash

echo "🚀 COMPREHENSIVE BEOS TERMINAL BUILD & LINK SCRIPT 🚀"
echo "====================================================="

# Set up environment
BEOS_ROOT="../../.."
INCLUDES="-I $BEOS_ROOT/headers -I $BEOS_ROOT/headers/app -I $BEOS_ROOT/headers/support -I $BEOS_ROOT/headers/posix -I $BEOS_ROOT/headers/kernel -I $BEOS_ROOT/headers/interface -I $BEOS_ROOT/headers/storage -I $BEOS_ROOT/headers/interface2 -I $BEOS_ROOT/headers/controls"

# Create comprehensive fix header
echo "🔧 Creating comprehensive BeOS compatibility header..."
cat > beos_compat_full.h << 'HEADER_EOF'
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
HEADER_EOF

echo "✅ Enhanced compatibility header created!"

# List available source files
echo "📁 Available source files:"
ls -la *.cpp

echo ""
echo "🎯 COMPILING ALL BEOS TERMINAL COMPONENTS..."

# Track compilation results
COMPILED_OBJECTS=""
FAILED_COMPONENTS=""

# Try to compile each component
for cpp_file in *.cpp; do
    if [[ "$cpp_file" == *"patched"* ]]; then
        echo "⚡ Compiling $cpp_file..."
        obj_file="${cpp_file%.cpp}.o"
        if g++ -include beos_compat_full.h $INCLUDES -c "$cpp_file" -o "$obj_file" 2>/dev/null; then
            echo "✅ $cpp_file compiled successfully!"
            COMPILED_OBJECTS="$COMPILED_OBJECTS $obj_file"
        else
            echo "❌ $cpp_file failed compilation"
            FAILED_COMPONENTS="$FAILED_COMPONENTS $cpp_file"
            # Show last few errors
            echo "  Last few errors:"
            g++ -include beos_compat_full.h $INCLUDES -c "$cpp_file" -o "$obj_file" 2>&1 | tail -3
        fi
    fi
done

echo ""
echo "📊 COMPILATION SUMMARY:"
echo "✅ Compiled objects: $COMPILED_OBJECTS"
echo "❌ Failed components: $FAILED_COMPONENTS"

# List all object files
echo ""
echo "🎯 CHECKING COMPILED OBJECTS:"
ls -la *.o 2>/dev/null || echo "No object files found"

# Try linking if we have objects
if [[ -n "$COMPILED_OBJECTS" ]]; then
    echo ""
    echo "🔗 ATTEMPTING TO LINK BEOS TERMINAL EXECUTABLE..."
    echo "Linking objects: $COMPILED_OBJECTS"
    
    if g++ $COMPILED_OBJECTS -o beos_terminal_experimental 2>/dev/null; then
        echo "🎉🎉🎉 BEOS TERMINAL EXECUTABLE CREATED!!! 🎉🎉🎉"
        ls -la beos_terminal_experimental
        echo ""
        echo "🧪 Testing executable..."
        echo "File type: $(file beos_terminal_experimental)"
    else
        echo "❌ Linking failed, checking errors..."
        g++ $COMPILED_OBJECTS -o beos_terminal_experimental 2>&1 | head -10
    fi
else
    echo "❌ No object files to link"
fi

echo "🎉 Build and link script completed!"
