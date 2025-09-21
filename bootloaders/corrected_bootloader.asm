; Corrected BeOS Bootloader - Fixed Jump
org 0x7c00

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    
    ; Print boot message
    mov si, boot_msg
    call print_str
    
    ; Load BeOS system from sector 62 (not 63!)
    mov si, loading_msg
    call print_str
    
    ; Load starting from sector 62 (0x3E)
    mov ah, 0x02        ; Read sectors
    mov al, 0x08        ; Read 8 sectors (4KB)
    mov ch, 0x00        ; Cylinder 0
    mov cl, 0x3E        ; Sector 62 (0x3E) - where we actually put it
    mov dh, 0x00        ; Head 0
    mov dl, 0x80        ; Hard drive
    mov bx, 0x1000      ; Load to 0x1000
    int 0x13
    
    jc disk_error
    
    mov si, success_msg
    call print_str
    
    ; Small delay and clear screen before jump
    call delay
    
    ; Jump to GUI system at 0x1000
    jmp 0x0000:0x1000

disk_error:
    mov si, error_msg
    call print_str
    jmp hang

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

delay:
    push cx
    mov cx, 0x4000
delay_loop:
    nop
    loop delay_loop
    pop cx
    ret

hang:
    jmp hang

boot_msg    db 'BeOS 4.0 Hard Drive Boot', 13, 10, 0
loading_msg db 'Loading BeOS from HDD...', 13, 10, 0
success_msg db 'BeOS system loaded!', 13, 10, 'Jumping to GUI...', 13, 10, 0
error_msg   db 'HDD load error!', 13, 10, 0

; Pad to 446 bytes
times 446-($-$$) db 0

; Partition table
db 0x80, 0x01, 0x3E, 0x00, 0xEB, 0x0F, 0x3F, 0x07
dd 0x3D, 0x1F000
times 48 db 0

; Boot signature
dw 0xaa55
