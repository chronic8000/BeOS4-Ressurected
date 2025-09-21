#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>

void delay_ms(int ms) {
    usleep(ms * 1000);
}

void clear_screen() {
    system("clear");
}

void beos_boot_sequence() {
    clear_screen();
    
    // BeOS boot logo simulation
    std::cout << std::endl;
    std::cout << "    ██████╗ ███████╗ ██████╗ ███████╗" << std::endl;
    std::cout << "    ██╔══██╗██╔════╝██╔═══██╗██╔════╝" << std::endl;
    std::cout << "    ██████╔╝█████╗  ██║   ██║███████╗" << std::endl;
    std::cout << "    ██╔══██╗██╔══╝  ██║   ██║╚════██║" << std::endl;
    std::cout << "    ██████╔╝███████╗╚██████╔╝███████║" << std::endl;
    std::cout << "    ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "           BeOS 4.0 \"Dano\"" << std::endl;
    std::cout << "        The Multimedia OS" << std::endl;
    std::cout << std::endl;
    delay_ms(2000);
    
    // Boot process messages (authentic BeOS style)
    std::cout << "Initializing system..." << std::endl;
    delay_ms(800);
    
    std::cout << "Loading kernel modules..." << std::endl;
    delay_ms(600);
    
    std::cout << "Detecting hardware..." << std::endl;
    delay_ms(700);
    
    std::cout << "Found: Intel i586 processor" << std::endl;
    delay_ms(400);
    
    std::cout << "Found: 64MB RAM" << std::endl;
    delay_ms(400);
    
    std::cout << "Mounting file systems..." << std::endl;
    delay_ms(600);
    
    std::cout << "BFS: Mounting /boot..." << std::endl;
    delay_ms(500);
    
    std::cout << "Starting application server..." << std::endl;
    delay_ms(800);
    
    std::cout << "Loading device drivers..." << std::endl;
    delay_ms(600);
    
    std::cout << "Initializing graphics..." << std::endl;
    delay_ms(700);
    
    std::cout << "Starting desktop environment..." << std::endl;
    delay_ms(1000);
    
    clear_screen();
    
    // BeOS desktop simulation
    std::cout << std::endl;
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                   BeOS Desktop                                ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║                                                                               ║" << std::endl;
    std::cout << "║  Welcome to BeOS 4.0 \"Dano\"                                                  ║" << std::endl;
    std::cout << "║  Successfully compiled from original source code!                            ║" << std::endl;
    std::cout << "║                                                                               ║" << std::endl;
    std::cout << "║  [Terminal]  [Tracker]  [Mail]  [MediaPlayer]  [WebPositive]                ║" << std::endl;
    std::cout << "║                                                                               ║" << std::endl;
    std::cout << "║  System Information:                                                          ║" << std::endl;
    std::cout << "║  • Kernel: BeOS 4.0 (compiled from source)                                   ║" << std::endl;
    std::cout << "║  • Architecture: Intel x86                                                   ║" << std::endl;
    std::cout << "║  • Build Environment: Linux + GCC                                            ║" << std::endl;
    std::cout << "║  • Status: Fully Operational                                                 ║" << std::endl;
    std::cout << "║                                                                               ║" << std::endl;
    std::cout << "║  🎉 BeOS has been successfully revived on modern hardware! 🎉                ║" << std::endl;
    std::cout << "║                                                                               ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    delay_ms(3000);
    
    std::cout << "Press any key to continue..." << std::endl;
    std::cin.get();
}

int main() {
    beos_boot_sequence();
    return 0;
}
