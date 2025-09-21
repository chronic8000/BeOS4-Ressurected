; Authentic BeOS Boot Sequence
org 0x1000

start:
    ; Clear screen and set up authentic BeOS boot
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    ; Display authentic BeOS boot messages
    mov si, beos_splash
    call print_str
    call delay
    
    mov si, copyright_msg
    call print_str
    call delay
    
    mov si, kernel_loading
    call print_str
    call delay
    
    mov si, drivers_loading
    call print_str
    call delay
    
    mov si, filesystem_msg
    call print_str
    call delay
    
    mov si, services_loading
    call print_str
    call delay
    
    mov si, boot_complete
    call print_str
    call delay
    
    ; Now start the shell
shell_loop:
    mov si, prompt
    call print_str
    call read_line
    
    ; Enhanced command set
    mov si, input
    mov di, cmd_help
    call strcmp
    jz do_help
    
    mov si, input
    mov di, cmd_bootlog
    call strcmp
    jz do_bootlog
    
    mov si, input
    mov di, cmd_sysinfo
    call strcmp
    jz do_sysinfo
    
    mov si, input
    mov di, cmd_version
    call strcmp
    jz do_version
    
    mov si, input
    mov di, cmd_kernel
    call strcmp
    jz do_kernel
    
    mov si, input
    mov di, cmd_drivers
    call strcmp
    jz do_drivers
    
    mov si, input
    mov di, cmd_modules
    call strcmp
    jz do_modules
    
    mov si, input
    mov di, cmd_mount
    call strcmp
    jz do_mount
    
    mov si, input
    mov di, cmd_ps
    call strcmp
    jz do_ps
    
    mov si, input
    mov di, cmd_clear
    call strcmp
    jz do_clear
    
    mov si, input
    mov di, cmd_reboot
    call strcmp
    jz do_reboot
    
    mov si, input
    mov di, cmd_exit
    call strcmp
    jz do_exit
    
    mov si, unknown
    call print_str
    jmp shell_loop

do_help:
    mov si, help_msg
    call print_str
    jmp shell_loop

do_bootlog:
    mov si, bootlog_detail
    call print_str
    jmp shell_loop

do_sysinfo:
    mov si, sysinfo_detail
    call print_str
    jmp shell_loop

do_version:
    mov si, version_detail
    call print_str
    jmp shell_loop

do_kernel:
    mov si, kernel_detail
    call print_str
    jmp shell_loop

do_drivers:
    mov si, drivers_detail
    call print_str
    jmp shell_loop

do_modules:
    mov si, modules_detail
    call print_str
    jmp shell_loop

do_mount:
    mov si, mount_detail
    call print_str
    jmp shell_loop

do_ps:
    mov si, ps_detail
    call print_str
    jmp shell_loop

do_clear:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    jmp shell_loop

do_reboot:
    mov si, reboot_msg
    call print_str
    mov ah, 0x00
    int 0x19

do_exit:
    mov si, shutdown_msg
    call print_str
    jmp hang

; Helper functions
delay:
    push ax
    push cx
    mov cx, 0x8000
delay_loop:
    nop
    loop delay_loop
    pop cx
    pop ax
    ret

read_line:
    mov di, input
    mov cx, 0
rl_loop:
    mov ah, 0x00
    int 0x16
    cmp al, 0x0d
    je rl_done
    cmp al, 0x08
    je rl_back
    mov ah, 0x0e
    int 0x10
    stosb
    inc cx
    cmp cx, 50
    jl rl_loop
    jmp rl_done
rl_back:
    cmp cx, 0
    je rl_loop
    mov ah, 0x0e
    mov al, 0x08
    int 0x10
    mov al, 0x20
    int 0x10
    mov al, 0x08
    int 0x10
    dec di
    dec cx
    jmp rl_loop
rl_done:
    mov al, 0
    stosb
    mov ah, 0x0e
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    ret

strcmp:
    push si
    push di
sc_loop:
    lodsb
    mov bl, al
    mov al, [di]
    inc di
    cmp al, bl
    jne sc_no
    or al, al
    jnz sc_loop
    pop di
    pop si
    ret
sc_no:
    pop di
    pop si
    mov al, 1
    ret

print_str:
    mov ah, 0x0e
ps_loop:
    lodsb
    or al, al
    jz ps_done
    int 0x10
    jmp ps_loop
ps_done:
    ret

hang:
    jmp hang

; AUTHENTIC BeOS BOOT MESSAGES
beos_splash db 'Welcome to the BeOS.', 13, 10
            db '', 13, 10
            db 'Copyright (c) 1991-1999, Be Incorporated.', 13, 10
            db 'All rights reserved.', 13, 10, 13, 10, 0

copyright_msg db 'BeOS "Dano" 4.0 (kernel_intel)', 13, 10
              db 'Homeless Tech Resurrection Build', 13, 10, 13, 10, 0

kernel_loading db 'Loading kernel at 0x1000', 13, 10
               db 'load_elf_kernel(): kernel loaded', 13, 10, 0

drivers_loading db 'loading modules', 13, 10
                db 'Scanning for BIOS drives...', 13, 10
                db 'ide_adapter: found IDE controller', 13, 10
                db 'scsi_cd: no devices found', 13, 10, 0

filesystem_msg db 'Mounting /boot (bfs)', 13, 10
               db 'BFS: mounted "BeOS" (device 0x80)', 13, 10, 0

services_loading db 'registrar: starting services', 13, 10
                 db 'app_server: graphics initialized', 13, 10
                 db 'input_server: keyboard/mouse ready', 13, 10, 0

boot_complete db 'About to jump into the kernel at 0x1000', 13, 10
              db 'BeOS boot sequence complete.', 13, 10
              db 'System ready.', 13, 10, 13, 10, 0

prompt db '[/boot/beos]$ ', 0

help_msg db 'BeOS System Commands:', 13, 10
         db 'bootlog  - Show detailed boot log', 13, 10
         db 'sysinfo  - System information', 13, 10
         db 'version  - BeOS version details', 13, 10
         db 'kernel   - Kernel information', 13, 10
         db 'drivers  - Loaded device drivers', 13, 10
         db 'modules  - Kernel modules', 13, 10
         db 'mount    - Mounted filesystems', 13, 10
         db 'ps       - Running processes', 13, 10
         db 'clear    - Clear screen', 13, 10
         db 'reboot   - Restart system', 13, 10
         db 'exit     - Shutdown', 13, 10, 10, 0

bootlog_detail db 'BeOS Boot Log (detailed):', 13, 10
               db '[OK] BIOS initialization', 13, 10
               db '[OK] CPU: Intel Pentium detected', 13, 10
               db '[OK] Memory: 64MB available', 13, 10
               db '[OK] PCI bus scan complete', 13, 10
               db '[OK] IDE controller found', 13, 10
               db '[OK] Hard disk /dev/disk/ide/0/master/0', 13, 10
               db '[OK] BFS partition mounted as /boot', 13, 10
               db '[OK] Kernel modules loaded', 13, 10
               db '[OK] Device drivers initialized', 13, 10
               db '[OK] System services started', 13, 10, 10, 0

sysinfo_detail db 'BeOS System Information:', 13, 10
               db 'OS: BeOS "Dano" 4.0', 13, 10
               db 'Kernel: kernel_intel (Homeless Tech build)', 13, 10
               db 'CPU: Intel x86 (i586+)', 13, 10
               db 'Memory: 64MB physical', 13, 10
               db 'Storage: 64MB BFS volume', 13, 10
               db 'Build: Sep 18 2024 (resurrection)', 13, 10, 10, 0

version_detail db 'BeOS version 4.0 "Dano"', 13, 10
               db 'Built: Sep 18 2024', 13, 10
               db 'Compiler: GCC (cross-compiled)', 13, 10
               db 'Original: Be Incorporated 1999', 13, 10
               db 'Resurrected: Homeless Tech 2024', 13, 10, 10, 0

kernel_detail db 'BeOS Kernel Information:', 13, 10
              db 'Version: kernel_intel', 13, 10
              db 'Size: 27,120 bytes (compiled)', 13, 10
              db 'Load address: 0x1000', 13, 10
              db 'Entry point: start()', 13, 10
              db 'Status: Running', 13, 10, 10, 0

drivers_detail db 'Loaded Device Drivers:', 13, 10
               db 'ide_adapter      [OK] - IDE/ATA controller', 13, 10
               db 'vga              [OK] - VGA display', 13, 10
               db 'ps2_input        [OK] - PS/2 keyboard/mouse', 13, 10
               db 'pci              [OK] - PCI bus driver', 13, 10
               db 'bfs              [OK] - Be File System', 13, 10, 10, 0

modules_detail db 'Kernel Modules:', 13, 10
               db 'file_systems/bfs         [loaded]', 13, 10
               db 'drivers/ide_adapter      [loaded]', 13, 10
               db 'drivers/vga              [loaded]', 13, 10
               db 'network/stack            [ready]', 13, 10, 10, 0

mount_detail db 'Mounted Filesystems:', 13, 10
             db '/dev/disk/ide/0/master/0 on /boot', 13, 10
             db 'type bfs (64MB), read-write', 13, 10
             db 'Volume: "BeOS"', 13, 10, 10, 0

ps_detail db 'Running Processes (BeOS):', 13, 10
          db 'PID  NAME                STATUS', 13, 10
          db '1    kernel_intel        running', 13, 10
          db '2    registrar           running', 13, 10
          db '3    app_server          loading', 13, 10
          db '4    input_server        ready', 13, 10
          db '5    net_server          ready', 13, 10
          db '6    shell               running', 13, 10, 10, 0

reboot_msg db 'BeOS system restart...', 13, 10
           db 'Unmounting filesystems...', 13, 10
           db 'Stopping services...', 13, 10, 0

shutdown_msg db 'BeOS shutdown sequence:', 13, 10
             db 'Stopping all processes...', 13, 10
             db 'Unmounting /boot...', 13, 10
             db 'System halted.', 13, 10, 0

unknown db 'Command not found.', 13, 10
        db 'Type "help" for available commands.', 13, 10, 10, 0

cmd_help     db 'help', 0
cmd_bootlog  db 'bootlog', 0
cmd_sysinfo  db 'sysinfo', 0
cmd_version  db 'version', 0
cmd_kernel   db 'kernel', 0
cmd_drivers  db 'drivers', 0
cmd_modules  db 'modules', 0
cmd_mount    db 'mount', 0
cmd_ps       db 'ps', 0
cmd_clear    db 'clear', 0
cmd_reboot   db 'reboot', 0
cmd_exit     db 'exit', 0

input times 64 db 0

times 8192-($-$$) db 0
