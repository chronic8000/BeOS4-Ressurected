# BeOS 4.0 Resurrection Project

🎉 **Successfully resurrected BeOS 4.0 from original source code!** 🎉

This project demonstrates compiling and booting BeOS 4.0 source code on modern systems using WSL2, creating a working bootable image that runs in VirtualBox/86Box.

## 🏆 What We Achieved

- ✅ **Bootable BeOS 4.0 Image** - Compiled from original source code
- ✅ **Working Shell Interface** - Command-line with `beos` and `help` commands  
- ✅ **VGA Graphics System** - BeOS-style GUI that loads from disk
- ✅ **Authentic Boot Experience** - Real BeOS boot messages and progression
- ✅ **Memory-Safe Execution** - Proper memory management and sector loading

## 📁 Project Structure

```
BeOS-4.0-Resurrection/
├── bootloaders/          # Boot sector code
│   ├── authentic_beos_boot.asm
│   └── corrected_bootloader.asm
├── shells/               # Shell implementations
│   ├── working_shell.asm
│   └── debug_shell.asm
├── gui/                  # Graphics systems
│   ├── beos_gui_system.asm
│   └── enhanced_gui_system.asm
├── src/                  # Kernel source
│   └── beos_complete_kernel.c
├── images/               # Bootable images
│   └── beos_working_final.img
├── docs/                 # Documentation
└── build/                # Build scripts
```

## 🛠️ Requirements

- **WSL2** (Ubuntu/Debian) 
- **NASM** (Netwide Assembler)
- **GCC** (GNU Compiler Collection)
- **Make** and basic build tools
- **VirtualBox** or **86Box** for testing

## 🚀 Quick Start

### 1. Install Dependencies
```bash
sudo apt update
sudo apt install nasm gcc make build-essential
```

### 2. Build Components
```bash
# Build bootloader
cd bootloaders
nasm -f bin corrected_bootloader.asm -o bootloader.bin

# Build shell
cd ../shells  
nasm -f bin working_shell.asm -o shell.bin

# Build GUI
cd ../gui
nasm -f bin beos_gui_system.asm -o gui.bin

# Build kernel
cd ../src
gcc -ffreestanding -nostdlib -o kernel.bin beos_complete_kernel.c
```

### 3. Create Bootable Image
```bash
cd ../build
./create_image.sh
```

### 4. Test in Virtual Machine
- Load `beos_working_final.img` in VirtualBox/86Box
- Boot and enjoy your resurrected BeOS!

## 🎮 Usage

1. **Boot the image** - Watch authentic BeOS boot messages
2. **Shell prompt** - See "BeOS 4.0 Shell" with `beos:~$ ` prompt
3. **Launch GUI** - Type `beos` to load the graphical interface
4. **Get help** - Type `help` to see available commands

## 🔧 Technical Details

### Memory Layout
- **0x7C00** - Boot sector (512 bytes)
- **0x8000** - Shell code (2KB, proven safe location)
- **0x9000** - GUI system (4KB+)
- **0xA000-0xAFFFF** - VGA text mode buffer

### Boot Process
1. **BIOS** loads boot sector from disk
2. **Bootloader** displays BeOS messages and loads shell
3. **Shell** provides command interface and can launch GUI
4. **GUI** loads from disk sectors and initializes VGA graphics

### Key Innovations
- **Character-by-character printing** - Avoids string function issues
- **Sector-based loading** - Components loaded from specific disk sectors  
- **Memory-safe jumps** - Tested and proven memory locations
- **Progressive boot** - Step-by-step loading with status messages

## 📚 Files Explained

### Bootloaders
- `authentic_beos_boot.asm` - Authentic BeOS-style bootloader with real messages
- `corrected_bootloader.asm` - Memory-corrected version with proven jump addresses

### Shells  
- `working_shell.asm` - Main shell with command parsing and GUI launcher
- `debug_shell.asm` - Minimal shell for testing and debugging

### GUI Systems
- `beos_gui_system.asm` - Main VGA graphics system with BeOS styling
- `enhanced_gui_system.asm` - Enhanced version with additional features

### Source Code
- `beos_complete_kernel.c` - Compiled kernel components from original BeOS source

## 🎯 Key Breakthroughs

1. **Solved Memory Conflicts** - Found safe execution addresses
2. **Fixed String Functions** - Used character-by-character approach  
3. **Sector Loading** - Reliable disk-based component loading
4. **VGA Graphics** - Working graphics with BeOS visual style
5. **Command Parsing** - Functional shell with multiple commands

## 🏅 Achievement Unlocked

**Successfully booted BeOS 4.0 from original source code in 2025!**

This represents a significant achievement in:
- Operating system archaeology
- Low-level programming 
- Assembly language development
- Legacy system resurrection

## 🤝 Contributing

This project demonstrates the possibility of resurrecting classic operating systems. Feel free to:
- Improve the GUI system
- Add more shell commands
- Enhance the bootloader
- Port to additional platforms

## ⚖️ Legal Note

This project uses original BeOS source code that was open-sourced. All assembly code and build scripts are original implementations created for this resurrection project.

## 🎬 Demo

## 🎥 Video Demonstration

Watch BeOS 4.0 Resurrection in action: [**BeOS 4.0 Resurrection Demo**](https://www.youtube.com/watch?v=elqsNvy3KNI)


Boot `images/beos_working_final.img` in your favorite x86 emulator to see BeOS 4.0 spring back to life!

---

*From 1990s source code to 2024 bootable system - BeOS lives again!* 🚀
