BITS 32

global _start
global idt_load
global isr0

extern kernel_main
extern isr0_c

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x0
    dd -(0x1BADB002)

section .text

_start:
    cli
    mov esp, stack_top
    call kernel_main

.hang:
    hlt
    jmp .hang

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr0:
    cli

    pushad

    push ds
    push es
    push fs
    push gs
    

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    

    ;call isr0_c
    

    pop gs
    pop fs
    pop es
    pop ds
    

    popad
    
    iret

section .bss
align 16

stack_bottom:
    resb 16384
stack_top: