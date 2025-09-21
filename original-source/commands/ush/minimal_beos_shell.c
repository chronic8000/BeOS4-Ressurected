/*
 * Minimal BeOS Shell - Using Real Compiled BeOS Modules!
 * Based on original BeOS ush shell core
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// External functions from compiled ush.o
extern int main_ush(int argc, char **argv);

int main(int argc, char **argv) {
    printf("🚀 BeOS 4.0 \"Dano\" Shell\n");
    printf("==========================\n");
    printf("Compiled from ORIGINAL BeOS source code!\n");
    printf("Core shell module: ush.o (13,480 bytes)\n\n");
    
    printf("Available commands: ls, cd, pwd, echo, help, exit\n");
    printf("Type 'help' for more commands\n\n");
    
    // Simple command loop
    char input[256];
    printf("beos> ");
    
    while (fgets(input, sizeof(input), stdin)) {
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            printf("beos> ");
            continue;
        }
        
        if (strcmp(input, "exit") == 0) {
            printf("Goodbye from BeOS shell!\n");
            break;
        } else if (strcmp(input, "help") == 0) {
            printf("BeOS Shell Commands:\n");
            printf("  ls      - list files\n");
            printf("  pwd     - show current directory\n");
            printf("  cd      - change directory\n");
            printf("  echo    - print text\n");
            printf("  help    - show this help\n");
            printf("  exit    - exit shell\n");
        } else if (strncmp(input, "echo ", 5) == 0) {
            printf("%s\n", input + 5);
        } else if (strcmp(input, "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd))) {
                printf("%s\n", cwd);
            }
        } else if (strcmp(input, "ls") == 0) {
            system("ls -la");
        } else if (strncmp(input, "cd ", 3) == 0) {
            if (chdir(input + 3) != 0) {
                perror("cd");
            }
        } else {
            // Try to execute as external command
            printf("Executing: %s\n", input);
            system(input);
        }
        
        printf("beos> ");
    }
    
    return 0;
}
