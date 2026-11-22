bits 32

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

section .multiboot
align 8

header_start:
    ; Multiboot2 header
    dd 0xE85250D6                ; Magic
    dd 0                         ; Architecture: i386
    dd header_end - header_start ; Header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

    ; Framebuffer request tag
    dw 5    ; type = framebuffer request
    dw 0    ; flags
    dd 20   ; size
    dd 640  ; width
    dd 480  ; height
    dd 32   ; bpp

    align 8 ; Pad to the next 8-byte boundary

    ; End tag
    dw 0
    dw 0
    dd 8

header_end:

section .text
global _start
global keyboard_handler
global read_port
global write_port
global load_idt

extern kmain
extern keyboard_handler_main
extern timer_handler_main
extern serial_init
extern serial_write_char
; All this external stuff is done in the C files somewhere

gdt_start:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

read_port:
    mov edx, [esp + 4]
    in al, dx
    ret

write_port:
    mov edx, [esp + 4]
    mov al, [esp + 4 + 4]
    out dx, al
    ret

load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret

global timer_handler

timer_handler:
    pusha

    call timer_handler_main

    mov al, 0x20
    out 0x20, al

    popa
    iretd

keyboard_handler:
    pusha

    call keyboard_handler_main

    mov al, 0x20
    out 0x20, al

    popa
    iretd

_start:
    cli

    ; Preserve the Multiboot2 handover registers IMMEDIATELY.
    ; EAX = Multiboot2 magic
    ; EBX = Multiboot2 information structure pointer
    ;
    ; Do this before touching AX or making any function calls.
    mov esi, eax
    mov edi, ebx

    ; Load our GDT and reload segment registers.
    lgdt [gdt_descriptor]
    jmp CODE_SEG:.reload_segments

.reload_segments:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    cld

    ; Establish the kernel stack.
    mov esp, stack_space
    and esp, 0xFFFFFFF0

    ; Initialise COM1 serial diagnostics.
    call serial_init

    ; Marker S: stack established.
    sub esp, 12
    push dword 'S'
    call serial_write_char
    add esp, 16

    ; Marker K: about to enter kmain.
    sub esp, 12
    push dword 'K'
    call serial_write_char
    add esp, 16

    ; Preserve 16-byte alignment before kmain().
    sub esp, 8

    ; Restore original Multiboot2 arguments.
    ; Push second argument first, then first argument.
    push edi
    push esi

    call kmain

halt:
    cli
    hlt
    jmp halt

section .bss
    align 16
    resb 8192 ; Reserve 8192 bytes of memory with no value

stack_space:
