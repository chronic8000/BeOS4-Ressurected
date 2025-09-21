; Enhanced BeOS GUI with Real Elements
org 0x1000

start:
    ; Show startup message
    mov si, gui_start_msg
    call print_str
    call delay
    
    ; Initialize VGA graphics
    mov ah, 0x00
    mov al, 0x13        ; 320x200x256 VGA mode
    int 0x10
    
    ; Set up BeOS colors
    call setup_colors
    
    ; Draw enhanced BeOS desktop
    call draw_desktop
    
    ; Initialize mouse
    call init_mouse
    
    ; Enter enhanced event loop
    jmp enhanced_event_loop

setup_colors:
    ; BeOS Blue (color 1)
    mov dx, 0x3C8
    mov al, 1
    out dx, al
    inc dx
    mov al, 12
    out dx, al
    mov al, 25
    out dx, al
    mov al, 38
    out dx, al
    
    ; BeOS Gray (color 2) 
    mov dx, 0x3C8
    mov al, 2
    out dx, al
    inc dx
    mov al, 45
    out dx, al
    mov al, 45
    out dx, al
    mov al, 45
    out dx, al
    
    ; White (color 3)
    mov dx, 0x3C8
    mov al, 3
    out dx, al
    inc dx
    mov al, 63
    out dx, al
    mov al, 63
    out dx, al
    mov al, 63
    out dx, al
    
    ; BeOS Yellow (color 4)
    mov dx, 0x3C8
    mov al, 4
    out dx, al
    inc dx
    mov al, 63
    out dx, al
    mov al, 55
    out dx, al
    mov al, 0
    out dx, al
    
    ; Black (color 0)
    mov dx, 0x3C8
    mov al, 0
    out dx, al
    inc dx
    mov al, 0
    out dx, al
    mov al, 0
    out dx, al
    mov al, 0
    out dx, al
    
    ; Dark Gray (color 5)
    mov dx, 0x3C8
    mov al, 5
    out dx, al
    inc dx
    mov al, 20
    out dx, al
    mov al, 20
    out dx, al
    mov al, 20
    out dx, al
    ret

init_mouse:
    ; Initialize mouse driver
    mov ax, 0x0000      ; Reset mouse
    int 0x33
    cmp ax, 0xFFFF
    jne no_mouse
    
    ; Show mouse cursor
    mov ax, 0x0001      ; Show cursor
    int 0x33
    
    ; Set mouse range
    mov ax, 0x0007      ; Set horizontal range
    mov cx, 0           ; Min X
    mov dx, 319         ; Max X
    int 0x33
    
    mov ax, 0x0008      ; Set vertical range
    mov cx, 0           ; Min Y
    mov dx, 199         ; Max Y
    int 0x33
    
    mov byte [mouse_present], 1
    ret
    
no_mouse:
    mov byte [mouse_present], 0
    ret

draw_desktop:
    ; Fill screen with BeOS blue
    mov ax, 0xA000
    mov es, ax
    mov di, 0
    mov al, 1           ; BeOS blue
    mov cx, 64000
    rep stosb
    
    ; Draw enhanced deskbar
    call draw_enhanced_deskbar
    
    ; Draw multiple windows
    call draw_tracker_window
    call draw_terminal_window
    call draw_about_window
    ret

draw_enhanced_deskbar:
    ; Draw deskbar (bottom 25 lines for more space)
    mov ax, 0xA000
    mov es, ax
    mov di, 56000       ; Bottom 25 lines
    mov al, 2           ; Gray
    mov cx, 8000        ; 25 lines * 320 pixels
    rep stosb
    
    ; Draw Be menu button with better graphics
    mov di, 56320       ; In deskbar
    call draw_be_logo
    
    ; Draw application buttons
    mov di, 56400       ; After Be button
    call draw_app_buttons
    
    ; Draw system tray
    mov di, 58000       ; Right side
    call draw_system_tray
    ret

draw_be_logo:
    ; Draw detailed Be logo
    mov cx, 20          ; 20 lines
be_logo_loop:
    push cx
    push di
    mov cx, 60          ; 60 pixels wide
    mov al, 4           ; Yellow
    rep stosb
    pop di
    add di, 320
    pop cx
    loop be_logo_loop
    
    ; Add "Be" text pattern
    mov di, 56960       ; Inside logo
    mov al, 0           ; Black text
    ; Draw "B"
    mov [es:di], al
    mov [es:di+1], al
    mov [es:di+2], al
    add di, 320
    mov [es:di], al
    mov [es:di+3], al
    add di, 320
    mov [es:di], al
    mov [es:di+1], al
    mov [es:di+2], al
    add di, 320
    mov [es:di], al
    mov [es:di+3], al
    add di, 320
    mov [es:di], al
    mov [es:di+1], al
    mov [es:di+2], al
    
    ; Draw "e"
    add di, -1280       ; Back to top
    add di, 10          ; Offset for "e"
    mov [es:di+1], al
    mov [es:di+2], al
    add di, 320
    mov [es:di], al
    mov [es:di+3], al
    add di, 320
    mov [es:di+1], al
    mov [es:di+2], al
    add di, 320
    mov [es:di], al
    add di, 320
    mov [es:di+1], al
    mov [es:di+2], al
    ret

draw_app_buttons:
    ; Draw application buttons with labels
    mov cx, 3           ; 3 app buttons
app_button_loop:
    push cx
    push di
    
    ; Button background
    mov cx, 80
    mov al, 5           ; Dark gray
    rep stosb
    
    ; Button highlight
    sub di, 80
    mov cx, 2
    mov al, 3           ; White highlight
    rep stosb
    
    add di, 78
    mov cx, 2
    mov al, 0           ; Black shadow
    rep stosb
    
    pop di
    add di, 85          ; Next button
    pop cx
    loop app_button_loop
    ret

draw_system_tray:
    ; Draw system tray with clock
    mov cx, 20
tray_loop:
    push cx
    push di
    mov cx, 100
    mov al, 5           ; Dark gray
    rep stosb
    pop di
    add di, 320
    pop cx
    loop tray_loop
    
    ; Draw clock digits
    mov di, 58080       ; Inside tray
    call draw_clock
    ret

draw_clock:
    ; Draw simple digital clock "12:34"
    mov al, 3           ; White
    ; Draw "1"
    mov [es:di], al
    add di, 320
    mov [es:di], al
    add di, 320
    mov [es:di], al
    
    ; Draw "2" 
    sub di, 640
    add di, 5
    mov [es:di], al
    mov [es:di+1], al
    add di, 320
    mov [es:di+1], al
    add di, 320
    mov [es:di], al
    mov [es:di+1], al
    
    ; Draw ":"
    add di, -640
    add di, 10
    mov [es:di], al
    add di, 640
    mov [es:di], al
    ret

draw_tracker_window:
    ; Draw Tracker window (file manager)
    mov di, 3200        ; Top area
    mov cx, 100         ; Height
    mov dx, 250         ; Width
    mov al, 3           ; White background
    call draw_window_with_title
    
    ; Draw file list
    mov di, 4160        ; Inside window
    call draw_file_list
    ret

draw_terminal_window:
    ; Draw Terminal window
    mov di, 6400        ; Middle left
    mov cx, 80          ; Height
    mov dx, 200         ; Width
    mov al, 0           ; Black background
    call draw_window_with_title
    
    ; Draw terminal content
    mov di, 7040        ; Inside window
    call draw_terminal_content
    ret

draw_about_window:
    ; Draw About BeOS window
    mov di, 12800       ; Center
    mov cx, 60          ; Height
    mov dx, 180         ; Width
    mov al, 3           ; White background
    call draw_window_with_title
    
    ; Draw about content
    mov di, 13440       ; Inside window
    call draw_about_content
    ret

draw_window_with_title:
    ; Generic window drawer with title bar
    ; DI = position, CX = height, DX = width, AL = bg color
    push ax
    
    ; Draw title bar
    push cx
    push di
    mov cx, dx
    mov al, 2           ; Gray title bar
    rep stosb
    pop di
    pop cx
    
    ; Draw window border and content
    sub cx, 1           ; Reserve space for title bar
window_loop:
    push cx
    push di
    add di, 320         ; Skip title bar
    
    ; Left border
    mov al, 5           ; Dark border
    mov [es:di], al
    
    ; Content
    inc di
    mov cx, dx
    sub cx, 2
    pop ax              ; Get background color
    push ax
    rep stosb
    
    ; Right border
    mov al, 5
    mov [es:di], al
    
    pop di
    add di, 320
    pop cx
    loop window_loop
    
    pop ax
    ret

draw_file_list:
    ; Draw file list entries
    mov cx, 12          ; 12 files
file_loop:
    push cx
    push di
    
    ; File icon (folder or file)
    mov al, 4           ; Yellow folder
    mov [es:di], al
    mov [es:di+1], al
    add di, 320
    mov [es:di], al
    mov [es:di+1], al
    sub di, 320
    
    ; File name
    add di, 5
    mov cx, 30
    mov al, 0           ; Black text
    rep stosb
    
    pop di
    add di, 640         ; Next file entry
    pop cx
    loop file_loop
    ret

draw_terminal_content:
    ; Draw terminal prompt and text
    mov cx, 10
term_loop:
    push cx
    push di
    
    ; Prompt "beos> "
    mov al, 4           ; Yellow prompt
    mov [es:di], al
    mov [es:di+1], al
    mov [es:di+2], al
    mov [es:di+3], al
    
    ; Command text
    add di, 8
    mov cx, 25
    mov al, 3           ; White text
    rep stosb
    
    pop di
    add di, 640
    pop cx
    loop term_loop
    ret

draw_about_content:
    ; Draw About BeOS content
    mov cx, 8
about_loop:
    push cx
    push di
    
    mov cx, 40
    mov al, 0           ; Black text
    rep stosb
    
    pop di
    add di, 640
    pop cx
    loop about_loop
    ret

enhanced_event_loop:
    ; Enhanced event loop with mouse support
    call handle_mouse
    call handle_keyboard
    call animate_elements
    jmp enhanced_event_loop

handle_mouse:
    ; Check mouse status
    cmp byte [mouse_present], 0
    je no_mouse_update
    
    mov ax, 0x0003      ; Get mouse position and buttons
    int 0x33
    
    ; Store mouse position
    mov [mouse_x], cx
    mov [mouse_y], dx
    mov [mouse_buttons], bx
    
    ; Check for clicks
    test bx, 1          ; Left button
    jnz handle_click
    
no_mouse_update:
    ret

handle_click:
    ; Handle mouse clicks
    ; Check if click is on Be button
    cmp word [mouse_x], 60
    jg not_be_button
    cmp word [mouse_y], 175
    jl not_be_button
    
    ; Clicked on Be button - show menu
    call show_be_menu
    
not_be_button:
    ret

show_be_menu:
    ; Show Be menu popup
    mov ax, 0xA000
    mov es, ax
    mov di, 48000       ; Above deskbar
    
    ; Menu background
    mov cx, 40
menu_loop:
    push cx
    push di
    mov cx, 120
    mov al, 2           ; Gray menu
    rep stosb
    pop di
    sub di, 320         ; Go up
    pop cx
    loop menu_loop
    
    ; Wait for release
wait_release:
    mov ax, 0x0003
    int 0x33
    test bx, 1
    jnz wait_release
    
    ; Clear menu
    call draw_desktop
    ret

handle_keyboard:
    ; Handle keyboard input
    mov ah, 0x01
    int 0x16
    jz no_key
    
    mov ah, 0x00
    int 0x16
    
    ; ESC = exit
    cmp al, 27
    je exit_gui
    
    ; Space = system info
    cmp al, 32
    je show_system_info
    
no_key:
    ret

show_system_info:
    ; Show system information overlay
    mov ax, 0xA000
    mov es, ax
    mov di, 20000
    
    ; Info background
    mov cx, 60
info_loop:
    push cx
    push di
    mov cx, 200
    mov al, 2           ; Gray
    rep stosb
    pop di
    add di, 320
    pop cx
    loop info_loop
    
    ; Wait for key
    mov ah, 0x00
    int 0x16
    
    ; Redraw desktop
    call draw_desktop
    ret

animate_elements:
    ; Animate cursor blink in terminal
    inc byte [animation_counter]
    test byte [animation_counter], 0x1F  ; Every 32 frames
    jnz no_animate
    
    ; Toggle terminal cursor
    mov ax, 0xA000
    mov es, ax
    mov di, 7200        ; Terminal cursor position
    
    mov al, [cursor_state]
    xor al, 1
    mov [cursor_state], al
    
    cmp al, 0
    je cursor_off
    mov al, 3           ; White cursor
    jmp set_cursor
cursor_off:
    mov al, 0           ; Black
set_cursor:
    mov [es:di], al
    
no_animate:
    ret

exit_gui:
    ; Return to text mode
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    mov si, exit_msg
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
    mov cx, 0x8000
delay_loop:
    nop
    loop delay_loop
    pop cx
    ret

hang:
    jmp hang

; Data
gui_start_msg db 'Starting Enhanced BeOS GUI System...', 13, 10, 0
exit_msg db 'Enhanced BeOS GUI System Exited', 13, 10, 0

; Variables
mouse_present db 0
mouse_x dw 160
mouse_y dw 100
mouse_buttons dw 0
cursor_state db 0
animation_counter db 0

times 8192-($-$$) db 0
