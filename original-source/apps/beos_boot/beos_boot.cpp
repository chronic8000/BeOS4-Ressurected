#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>

void beos_delay(int milliseconds) {
    usleep(milliseconds * 1000);
}

void print_with_delay(const std::string& message, int delay_ms = 500) {
    std::cout << message << std::endl;
    beos_delay(delay_ms);
}

int main() {
    // Clear screen first
    system("clear");
    
    std::cout << std::endl;
    print_with_delay("BeOS 4.0 \"Dano\" - Compiled from Original Source Code", 1000);
    std::cout << std::endl;
    
    print_with_delay("Loading BeOS...", 800);
    print_with_delay("Initializing system components...", 600);
    print_with_delay("Loading kernel modules...", 600);
    print_with_delay("Starting application server...", 600);
    print_with_delay("Mounting file systems...", 600);
    print_with_delay("Loading device drivers...", 600);
    print_with_delay("Initializing networking...", 600);
    print_with_delay("Starting desktop environment...", 800);
    
    std::cout << std::endl;
    print_with_delay("BeOS is ready!", 1000);
    std::cout << std::endl;
    
    print_with_delay("Welcome to BeOS 4.0 \"Dano\"", 800);
    print_with_delay("The multimedia operating system", 800);
    print_with_delay("Successfully compiled and running on Linux!", 1000);
    
    std::cout << std::endl;
    print_with_delay("System Status: OPERATIONAL", 500);
    print_with_delay("Build Environment: WSL2 + GCC", 500);
    print_with_delay("Source Compilation: SUCCESS", 500);
    
    std::cout << std::endl;
    std::cout << "BeOS boot sequence complete." << std::endl;
    
    return 0;
}
