#!/bin/bash

# BeOS 4.0 Resurrection - Complete Build Script
# Builds all components and creates bootable image

echo "🎉 BeOS 4.0 Resurrection - Complete Builder 🎉"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check dependencies
echo "🔍 Checking dependencies..."

if ! command_exists nasm; then
    echo "❌ NASM not found! Please install: sudo apt install nasm"
    exit 1
fi

if ! command_exists gcc; then
    echo "❌ GCC not found! Please install: sudo apt install gcc"
    exit 1
fi

echo "✅ All dependencies found!"
echo ""

# Build bootloader
echo "🔧 Building bootloader..."
cd ../bootloaders
nasm -f bin corrected_bootloader.asm -o bootloader.bin
if [ $? -eq 0 ]; then
    echo "✅ Bootloader built successfully"
else
    echo "❌ Bootloader build failed!"
    exit 1
fi

echo ""

# Build shell
echo "🔧 Building shell..."
cd ../shells
nasm -f bin working_shell.asm -o shell.bin
if [ $? -eq 0 ]; then
    echo "✅ Shell built successfully"
else
    echo "❌ Shell build failed!"
    exit 1
fi

echo ""

# Build GUI
echo "🔧 Building GUI..."
cd ../gui
nasm -f bin beos_gui_system.asm -o gui.bin
if [ $? -eq 0 ]; then
    echo "✅ GUI built successfully"
else
    echo "❌ GUI build failed!"
    exit 1
fi

echo ""

# Create bootable image
echo "🔧 Creating bootable image..."
cd ../build
./create_image.sh

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 COMPLETE SUCCESS! ��"
    echo ""
    echo "📁 Files created:"
    echo "  └── beos_resurrection.img (64MB bootable image)"
    echo ""
    echo "🎮 Ready to boot BeOS 4.0!"
    echo ""
    echo "🏆 Achievement Unlocked: Resurrected BeOS from source code!"
else
    echo "❌ Image creation failed!"
    exit 1
fi
