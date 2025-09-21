[BITS 16]
[ORG 0x8000]

start:
    ; First, just print a single character to confirm we got here
    mov ah, 0x0E
    mov al, 'X'
    int 0x10
    
    ; If we see X, the jump works
    ; Now try to print a simple message
    mov al, ':'
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
    mov al, ' '
    int 0x10
    mov al, 'O'
    int 0x10
    mov al, 'K'
    int 0x10
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    
    ; Simple prompt
    mov al, '>'
    int 0x10
    mov al, ' '
    int 0x10
    
hang:
    ; Simple input loop
    mov ah, 0x00
    int 0x16
    
    ; Echo character
    mov ah, 0x0E
    int 0x10
    
    jmp hang

times 2048-($-$$) db 0
