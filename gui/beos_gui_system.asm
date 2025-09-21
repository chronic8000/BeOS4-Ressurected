; BeOS GUI System - Boot to Graphics
org 0x1000

start:
    ; Show authentic BeOS boot sequence
    call show_beos_boot
    
    ; Initialize graphics system
    call init_graphics_system
    
    ; Start app_server
    call start_app_server_gui
    
    ; Enter GUI mode
    jmp gui_main_loop

show_beos_boot:
    ; Clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    ; Show BeOS boot messages
    mov si, welcome_beos
    call print_str
    call delay
    
    mov si, copyright_msg
    call print_str
    call delay
    
    mov si, kernel_loading
    call print_str
    call delay
    
    mov si, graphics_init
    call print_str
    call delay
    
    mov si, app_server_init
    call print_str
    call delay
    
    mov si, desktop_loading
    call print_str
    call delay
    
    mov si, gui_ready
    call print_str
    call delay
    ret

init_graphics_system:
    ; Switch to VGA graphics mode
    mov ah, 0x00
    mov al, 0x13        ; 320x200x256 VGA
    int 0x10
    
    ; Set BeOS color palette
    call setup_beos_colors
    ret

setup_beos_colors:
    ; Set up BeOS-style color palette
    
    ; Color 0: Black
    mov dx, 0x3C8
    mov al, 0
    out dx, al
    inc dx
    xor al, al
    out dx, al
    out dx, al
    out dx, al
    
    ; Color 1: BeOS Blue
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
    
    ; Color 2: BeOS Gray
    mov dx, 0x3C8
    mov al, 2
    out dx, al
    inc dx
    mov al, 54
    out dx, al
    mov al, 54
    out dx, al
    mov al, 54
    out dx, al
    
    ; Color 3: White
    mov dx, 0x3C8
    mov al, 3
    out dx, al
    inc dx
    mov al, 63
    out dx, al
    out dx, al
    out dx, al
    
    ; Color 4: BeOS Yellow
    mov dx, 0x3C8
    mov al, 4
    out dx, al
    inc dx
    mov al, 63
    out dx, al
    mov al, 50
    out dx, al
    mov al, 1
    out dx, al
    
    ; Color 5: Dark Gray
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

start_app_server_gui:
    ; Draw BeOS desktop
    call draw_beos_desktop
    
    ; Draw Deskbar
    call draw_deskbar
    
    ; Draw sample windows
    call draw_windows
    
    ; Show cursor
    call draw_cursor
    ret

draw_beos_desktop:
    ; Fill desktop with BeOS blue
    mov ax, 0xA000
    mov es, ax
    mov di, 0
    mov al, 1           ; BeOS blue
    mov cx, 64000       ; 320x200
    rep stosb
    
    ; Add subtle desktop pattern
    call add_desktop_texture
    ret

add_desktop_texture:
    ; Add diagonal texture to desktop
    mov ax, 0xA000
    mov es, ax
    mov di, 0
    mov cx, 200         ; 200 lines
texture_line:
    push cx
    mov cx, 320
    mov bx, di
texture_pixel:
    test bx, 0x20       ; Every 32 pixels
    jnz skip_texture
    mov byte [es:di], 2 ; Slightly lighter
skip_texture:
    inc di
    inc bx
    loop texture_pixel
    pop cx
    loop texture_line
    ret

draw_deskbar:
    ; Draw Deskbar at bottom of screen
    mov ax, 0xA000
    mov es, ax
    
    ; Deskbar background (20 lines high)
    mov di, 57600       ; Line 180
    mov cx, 20
deskbar_bg:
    push cx
    push di
    mov cx, 320
    mov al, 2           ; Gray
    rep stosb
    pop di
    add di, 320
    pop cx
    loop deskbar_bg
    
    ; Be menu button
    mov di, 57610       ; Left side
    call draw_be_button
    
    ; Application area
    mov di, 57680       ; Center
    call draw_app_buttons
    
    ; System tray
    mov di, 57900       ; Right side
    call draw_system_tray
    ret

draw_be_button:
    ; Draw the iconic Be menu button
    mov cx, 15          ; 15 lines high
be_button_loop:
    push cx
    push di
    mov cx, 50          ; 50 pixels wide
    mov al, 4           ; Yellow
    rep stosb
    pop di
    add di, 320
    pop cx
    loop be_button_loop
    
    ; Add "Be" text effect (simple)
    mov di, 58250       ; Inside button
    mov al, 0           ; Black text
    mov [es:di], al
    mov [es:di+2], al
    mov [es:di+320], al
    mov [es:di+322], al
    ret

draw_app_buttons:
    ; Draw application buttons in Deskbar
    mov cx, 3           ; 3 app buttons
app_button_loop:
    push cx
    push di
    
    ; Button background
    mov cx, 60
    mov al, 5           ; Dark gray
    rep stosb
    
    ; Button separator
    add di, 60
    mov cx, 5
    mov al, 2           ; Light gray
    rep stosb
    
    pop di
    add di, 65          ; Next button position
    pop cx
    loop app_button_loop
    ret

draw_system_tray:
    ; Draw system tray area
    mov cx, 15          ; 15 lines high
tray_loop:
    push cx
    push di
    mov cx, 80          ; 80 pixels wide
    mov al, 5           ; Dark gray
    rep stosb
    pop di
    add di, 320
    pop cx
    loop tray_loop
    
    ; Clock display (simple)
    mov di, 58220       ; Inside tray
    mov al, 3           ; White
    mov [es:di], al
    mov [es:di+2], al
    mov [es:di+4], al
    ret

draw_windows:
    ; Draw sample BeOS windows
    call draw_terminal_window
    call draw_finder_window
    call draw_about_window
    ret

draw_terminal_window:
    ; Terminal window (top-left)
    mov di, 6400        ; Line 20
    mov cx, 80          ; Height
    mov dx, 150         ; Width
    mov al, 0           ; Black background
    call draw_window_frame
    
    ; Terminal content
    mov di, 7040        ; Inside window
    call draw_terminal_content
    ret

draw_finder_window:
    ; Finder/Tracker window (center)
    mov di, 9600        ; Line 30, offset
    mov cx, 100         ; Height
    mov dx, 180         ; Width
    mov al, 3           ; White background
    call draw_window_frame
    
    ; File list content
    mov di, 10240       ; Inside window
    call draw_file_content
    ret

draw_about_window:
    ; About BeOS window (smaller)
    mov di, 16000       ; Line 50
    mov cx, 60          ; Height
    mov dx, 120         ; Width
    mov al, 3           ; White background
    call draw_window_frame
    
    ; About content
    mov di, 16640       ; Inside window
    call draw_about_content
    ret

draw_window_frame:
    ; Generic window frame drawer
    ; DI = start position, CX = height, DX = width, AL = bg color
    push ax             ; Save background color
    
    ; Draw window content
frame_loop:
    push cx
    push di
    
    ; Window border
    mov al, 5           ; Dark gray border
    mov [es:di], al
    
    ; Content area
    inc di
    mov cx, dx
    sub cx, 2           ; Account for borders
    pop ax              ; Get background color
    push ax
    rep stosb
    
    ; Right border
    mov al, 5
    mov [es:di], al
    
    pop di
    add di, 320         ; Next line
    pop cx
    loop frame_loop
    
    ; Title bar
    sub di, cx          ; Back to start
    mov cx, dx
    mov al, 2           ; Gray title bar
    rep stosb
    
    pop ax              ; Restore stack
    ret

draw_terminal_content:
    ; Draw terminal text simulation
    mov cx, 10          ; 10 lines of text
term_content_loop:
    push cx
    push di
    
    ; Prompt
    mov al, 4           ; Yellow
    mov [es:di], al
    mov [es:di+1], al
    
    ; Command text
    add di, 5
    mov cx, 20
    mov al, 3           ; White
    rep stosb
    
    pop di
    add di, 640         ; Next line (skip one)
    pop cx
    loop term_content_loop
    ret

draw_file_content:
    ; Draw file browser content
    mov cx, 12          ; 12 file entries
file_content_loop:
    push cx
    push di
    
    ; File icon
    mov al, 4           ; Yellow icon
    mov [es:di], al
    mov [es:di+1], al
    
    ; File name
    add di, 5
    mov cx, 25
    mov al, 0           ; Black text
    rep stosb
    
    pop di
    add di, 960         ; Next file line
    pop cx
    loop file_content_loop
    ret

draw_about_content:
    ; Draw About BeOS content
    mov cx, 8           ; 8 lines
about_content_loop:
    push cx
    push di
    
    mov cx, 30
    mov al, 0           ; Black text
    rep stosb
    
    pop di
    add di, 640
    pop cx
    loop about_content_loop
    ret

draw_cursor:
    ; Draw mouse cursor
    mov ax, 0xA000
    mov es, ax
    mov di, 32000       ; Center position
    
    ; Simple arrow cursor
    mov al, 3           ; White
    mov [es:di], al
    mov [es:di+320], al
    mov [es:di+640], al
    mov [es:di+1], al
    mov [es:di+321], al
    ret

gui_main_loop:
    ; Main GUI event loop
    call handle_gui_input
    call update_gui_elements
    call animate_cursor
    jmp gui_main_loop

handle_gui_input:
    ; Check for keyboard input
    mov ah, 0x01
    int 0x16
    jz no_gui_input
    
    ; Get key
    mov ah, 0x00
    int 0x16
    
    ; ESC = exit to text mode
    cmp al, 27
    je exit_gui
    
    ; Space = show system info
    cmp al, 32
    je show_gui_info
    
    ; Enter = activate Be menu
    cmp al, 13
    je activate_be_menu
    
no_gui_input:
    ret

show_gui_info:
    ; Show GUI system information
    call draw_info_popup
    ret

activate_be_menu:
    ; Show Be menu
    call draw_be_menu
    ret

draw_info_popup:
    ; Draw information popup window
    mov ax, 0xA000
    mov es, ax
    mov di, 20000       ; Center screen
    
    ; Popup background
    mov cx, 40
info_popup_loop:
    push cx
    push di
    mov cx, 120
    mov al, 2           ; Gray
    rep stosb
    pop di
    add di, 320
    pop cx
    loop info_popup_loop
    ret

draw_be_menu:
    ; Draw Be menu popup
    mov ax, 0xA000
    mov es, ax
    mov di, 51200       ; Above Deskbar
    
    ; Menu background
    mov cx, 60
be_menu_loop:
    push cx
    push di
    mov cx, 100
    mov al, 2           ; Gray
    rep stosb
    pop di
    sub di, 320         ; Go up
    pop cx
    loop be_menu_loop
    ret

update_gui_elements:
    ; Update dynamic GUI elements
    call blink_cursor_in_terminal
    ret

animate_cursor:
    ; Simple cursor animation (placeholder)
    ret

blink_cursor_in_terminal:
    ; Blink terminal cursor
    mov ax, 0xA000
    mov es, ax
    mov di, 7200        ; Terminal cursor position
    
    ; Toggle cursor visibility
    mov al, [cursor_blink_state]
    xor al, 1
    mov [cursor_blink_state], al
    
    cmp al, 0
    je cursor_off
    mov al, 3           ; White cursor
    jmp set_cursor
cursor_off:
    mov al, 0           ; Black (invisible)
set_cursor:
    mov [es:di], al
    ret

exit_gui:
    ; Return to text mode
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    ; Show exit message
    mov si, gui_exit_msg
    call print_str
    jmp text_mode_shell

text_mode_shell:
    ; Simple shell after GUI exit
    mov si, shell_prompt
    call print_str
    
    ; Get input
    mov ah, 0x00
    int 0x16
    
    ; 'g' = restart GUI
    cmp al, 'g'
    je start
    
    ; 'q' = quit
    cmp al, 'q'
    je final_exit
    
    jmp text_mode_shell

final_exit:
    mov si, goodbye_msg
    call print_str
    jmp hang

; Helper functions
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

; Messages
welcome_beos db 'Welcome to the BeOS.', 13, 10, 13, 10, 0
copyright_msg db 'Copyright (c) 1991-1999, Be Incorporated.', 13, 10
              db 'Homeless Tech GUI Resurrection', 13, 10, 13, 10, 0
kernel_loading db 'Loading kernel_intel...', 13, 10, 0
graphics_init db 'Initializing graphics system...', 13, 10, 0
app_server_init db 'Starting app_server...', 13, 10, 0
desktop_loading db 'Creating desktop workspace...', 13, 10, 0
gui_ready db 'BeOS GUI system ready!', 13, 10, 13, 10
          db 'Controls: ESC=exit, SPACE=info, ENTER=Be menu', 13, 10, 13, 10, 0

gui_exit_msg db 'BeOS GUI system shutdown.', 13, 10
             db 'Press g for GUI, q to quit', 13, 10, 0
shell_prompt db 'beos-gui> ', 0
goodbye_msg db 'BeOS GUI system halted.', 13, 10, 0

; Variables
cursor_blink_state db 0

times 8192-($-$$) db 0
