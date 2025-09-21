/*
 * Minimal BeOS Bootloader
 * Displays authentic BeOS boot sequence
 */

#include <stdio.h>

// Basic types for bootloader
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

// Function to display BeOS boot sequence
void beos_boot_sequence() {
    printf("BeOS 4.0 \"Dano\" - Bootloader\n");
    printf("==============================\n\n");
    
    printf("Loading BeOS...\n");
    printf("Detecting hardware...\n");
    printf("Intel i586 processor detected\n");
    printf("64MB RAM detected\n");
    
    printf("Loading kernel modules...\n");
    printf("Initializing file systems...\n");
    printf("BFS file system ready\n");
    
    printf("Loading device drivers...\n");
    printf("Starting application server...\n");
    
    printf("\nBeOS kernel loaded successfully!\n");
    printf("Starting BeOS desktop environment...\n");
    
    printf("\n*** BeOS 4.0 is ready! ***\n");
}

int main() {
    beos_boot_sequence();
    return 0;
}
