#!/bin/bash

echo "🚀 FIXED BEOS TERMINAL BUILD & LINK SCRIPT 🚀"
echo "=============================================="

# Set up environment  
BEOS_ROOT="../../.."
INCLUDES="-I $BEOS_ROOT/headers -I $BEOS_ROOT/headers/app -I $BEOS_ROOT/headers/support -I $BEOS_ROOT/headers/posix -I $BEOS_ROOT/headers/kernel -I $BEOS_ROOT/headers/interface -I $BEOS_ROOT/headers/storage -I $BEOS_ROOT/headers/interface2 -I $BEOS_ROOT/headers/controls"

echo "🎯 COMPILING KNOWN WORKING COMPONENT..."
echo "Trying main_patched_v3.cpp (we know this works)..."

# Use our known working approach
if g++ -include beos_final.h $INCLUDES -c main_patched_v3.cpp -o main_patched_v3.o; then
    echo "✅ main_patched_v3.cpp compiled!"
    ls -la main_patched_v3.o
    
    # Try a simple test compilation of other components
    echo ""
    echo "🎯 TRYING SIMPLE COMPONENTS..."
    
    echo "Compiling original main.cpp..."
    if g++ -include beos_final.h $INCLUDES -c main.cpp -o main_simple.o 2>/dev/null; then
        echo "✅ main.cpp compiled!"
    else
        echo "❌ main.cpp needs more work"
    fi
    
    echo ""
    echo "🔗 ATTEMPTING SIMPLE LINK TEST..."
    echo "Creating minimal BeOS terminal with available objects..."
    
    # Collect available objects
    OBJECTS=$(ls *.o 2>/dev/null | tr '\n' ' ')
    echo "Available objects: $OBJECTS"
    
    if [[ -n "$OBJECTS" ]]; then
        echo "Attempting to link: $OBJECTS"
        if g++ $OBJECTS -o beos_terminal_minimal -lpthread 2>/dev/null; then
            echo "🎉🎉🎉 BEOS TERMINAL MINIMAL EXECUTABLE CREATED!!! 🎉🎉🎉"
            ls -la beos_terminal_minimal
            echo "File info: $(file beos_terminal_minimal)"
        else
            echo "Linking with detailed errors:"
            g++ $OBJECTS -o beos_terminal_minimal -lpthread 2>&1 | head -10
        fi
    fi
else
    echo "❌ Known working component failed"
fi

echo "🎉 Fixed build script completed!"
