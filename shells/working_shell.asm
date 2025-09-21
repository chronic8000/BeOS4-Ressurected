[BITS 16]
[ORG 0x8000]

start:
    ; Print BeOS welcome using proven method
    mov ah, 0x0E
    mov al, 'B'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'O'
    int 0x10
    mov al, 'S'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, '4'
    int 0x10
    mov al, '.'
    int 0x10
    mov al, '0'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'S'
    int 0x10
    mov al, 'h'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'l'
    int 0x10
    mov al, 'l'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    
    ; Print instructions
    mov al, 'T'
    int 0x10
    mov al, 'y'
    int 0x10
    mov al, 'p'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, ':'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'b'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 's'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'f'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 'r'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'G'
    int 0x10
    mov al, 'U'
    int 0x10
    mov al, 'I'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10

shell_loop:
    ; Print prompt
    mov al, 'b'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 's'
    int 0x10
    mov al, ':'
    int 0x10
    mov al, '~'
    int 0x10
    mov al, '$'
    int 0x10
    mov al, ' '
    int 0x10
    
    ; Read command
    call read_command
    
    jmp shell_loop

read_command:
    mov cx, 0
    mov di, cmd_buffer
    
read_loop:
    mov ah, 0x00
    int 0x16
    
    cmp al, 13
    je process_command
    
    cmp al, 8
    je handle_backspace
    
    cmp cx, 10
    jge read_loop
    
    mov ah, 0x0E
    int 0x10
    
    stosb
    inc cx
    jmp read_loop

handle_backspace:
    cmp cx, 0
    je read_loop
    
    mov ah, 0x0E
    mov al, 8
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    
    dec di
    dec cx
    jmp read_loop

process_command:
    mov ah, 0x0E
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    
    ; Check for "beos" command
    mov si, cmd_buffer
    lodsb
    cmp al, 'b'
    jne check_help
    lodsb
    cmp al, 'e'
    jne check_help
    lodsb
    cmp al, 'o'
    jne check_help
    lodsb
    cmp al, 's'
    jne check_help
    
    call launch_gui
    ret

check_help:
    mov si, cmd_buffer
    lodsb
    cmp al, 'h'
    jne unknown_command
    
    call show_help
    ret

unknown_command:
    mov ah, 0x0E
    mov al, 'U'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'k'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 'w'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'c'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 'm'
    int 0x10
    mov al, 'm'
    int 0x10
    mov al, 'a'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'd'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    ret

launch_gui:
    mov ah, 0x0E
    mov al, 'L'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 'a'
    int 0x10
    mov al, 'd'
    int 0x10
    mov al, 'i'
    int 0x10
    mov al, 'n'
    int 0x10
    mov al, 'g'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'B'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'O'
    int 0x10
    mov al, 'S'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'G'
    int 0x10
    mov al, 'U'
    int 0x10
    mov al, 'I'
    int 0x10
    mov al, '.'
    int 0x10
    mov al, '.'
    int 0x10
    mov al, '.'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    
    ; Load and run GUI
    mov ah, 0x02
    mov al, 0x04
    mov ch, 0x00
    mov cl, 0x3F
    mov dh, 0x00
    mov dl, 0x80
    mov bx, 0x9000
    int 0x13
    
    jc gui_error
    jmp 0x0000:0x9000

gui_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    mov al, 'r'
    int 0x10
    mov al, 'r'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 'r'
    int 0x10
    ret

show_help:
    mov ah, 0x0E
    mov al, 'C'
    int 0x10
    mov al, 'm'
    int 0x10
    mov al, 'd'
    int 0x10
    mov al, 's'
    int 0x10
    mov al, ':'
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'b'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'o'
    int 0x10
    mov al, 's'
    int 0x10
    mov al, ','
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 'h'
    int 0x10
    mov al, 'e'
    int 0x10
    mov al, 'l'
    int 0x10
    mov al, 'p'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    ret

cmd_buffer: times 12 db 0

times 2048-($-$$) db 0
