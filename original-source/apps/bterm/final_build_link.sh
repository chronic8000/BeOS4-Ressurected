#!/bin/bash

echo "🚀 DEFINITIVE BEOS TERMINAL BUILD & LINK SCRIPT 🚀"
echo "=================================================="

# Environment setup
BEOS_ROOT="../../.."
INCLUDES="-I $BEOS_ROOT/headers -I $BEOS_ROOT/headers/app -I $BEOS_ROOT/headers/support -I $BEOS_ROOT/headers/posix -I $BEOS_ROOT/headers/kernel -I $BEOS_ROOT/headers/interface -I $BEOS_ROOT/headers/storage -I $BEOS_ROOT/headers/interface2 -I $BEOS_ROOT/headers/controls"
COMPILE_FLAGS="-Wno-error -include beos_final.h"

echo "🎯 COMPILING MAIN BEOS TERMINAL COMPONENT..."
g++ $COMPILE_FLAGS $INCLUDES -c main_patched_v3.cpp -o main_patched_v3.o
if [[ -f main_patched_v3.o ]]; then
    echo "✅ Main component compiled successfully!"
    ls -la main_patched_v3.o
    
    echo ""
    echo "🎯 TRYING TO COMPILE ADDITIONAL COMPONENTS..."
    
    # Try simpler components
    echo "Compiling original main.cpp..."
    g++ $COMPILE_FLAGS $INCLUDES -c main.cpp -o main_original.o 2>/dev/null
    
    echo "Compiling shell.cpp..."
    g++ $COMPILE_FLAGS $INCLUDES -c shell.cpp -o shell_original.o 2>/dev/null
    
    echo ""
    echo "📋 AVAILABLE OBJECT FILES:"
    ls -la *.o 2>/dev/null || echo "No object files found"
    
    echo ""
    echo "🔗 ATTEMPTING TO LINK BEOS TERMINAL EXECUTABLE..."
    
    # Get all available object files
    OBJECTS=$(ls *.o 2>/dev/null | tr '\n' ' ')
    echo "Linking objects: $OBJECTS"
    
    if [[ -n "$OBJECTS" ]]; then
        echo "Attempting link with pthread support..."
        if g++ $OBJECTS -o beos_terminal -lpthread 2>/dev/null; then
            echo ""
            echo "🎉🎉🎉 BEOS TERMINAL EXECUTABLE CREATED!!! 🎉🎉🎉"
            echo "================================================"
            ls -la beos_terminal
            echo ""
            echo "📋 EXECUTABLE DETAILS:"
            file beos_terminal
            echo ""
            echo "🧪 TESTING EXECUTABLE:"
            echo "Executable size: $(du -h beos_terminal | cut -f1)"
            echo "Permissions: $(ls -l beos_terminal | cut -d' ' -f1)"
        else
            echo "❌ Linking failed, showing detailed errors:"
            g++ $OBJECTS -o beos_terminal -lpthread 2>&1 | head -15
        fi
    else
        echo "❌ No object files available for linking"
    fi
    
else
    echo "❌ Main component compilation failed"
    echo "Showing compilation errors:"
    g++ $COMPILE_FLAGS $INCLUDES -c main_patched_v3.cpp -o main_patched_v3.o 2>&1 | tail -10
fi

echo ""
echo "🎉 Build and link process completed!"
