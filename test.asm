section .text16
bits 32
org 0x8000

SEG_TEMP_CS equ (9 << 3)
SEG_TEMP_DS equ (10 << 3)

align 16
; global _start

; ===================================
; 16-bit code to boot AP for SMP.
; ===================================
_start:
    bits 16
    cli

    ; -------------- 16 bits addressing mode -----------------
    mov ax, cs

    ; --> code segment: 0. 
    ;   : accesses: 0x0000 ~ 0xffff.
    mov ax, 0
    mov ds, ax
    
    ; --> load temporary 32-bit GDT.
    nop
	lgdt	[__args_gdtr16]

    ; --> set CR0.
    ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1.
    mov eax, 0x4000003b
    mov cr0, eax
    nop
    
    ; --> jump to code32.
    jmp dword SEG_TEMP_CS:_start32
    bits 32 ;_start

    nop
    nop

; ------------------
; section .text
bits 32

align 16
; extern main
_start32:
    mov ax, SEG_TEMP_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --> switch stack segment.
    mov ss, ax

    ; --> then, switch stack top.
    mov ebx, __stack32.top
    mov esp, ebx
    mov ebp, ebx

    ; --> switch to 32-bit segment.
    call switch_gdt32

    ; --> switch to long mode.
    mov eax, cr0
    and eax, ~(1 << 31) ; PG off.
    and eax, ~(1 << 16) ; WP off.
    mov cr0, eax

    mov eax, cr4
    and eax, ~(1 <<  7) ; PGE off.
    and eax, ~(1 <<  4) ; PSE off.
    mov cr4, eax

    ; set cr3 to PML4T address.
    mov ecx, __args_tlpd
    mov eax, [ecx]
    mov cr3, eax

    mov eax, cr4
    or eax,   (1 << 5)  ; PAE on.
    or eax,   (1 << 7)  ; PGE on.
    mov cr4, eax

    ; --> enable long mode.
    mov ecx, 0xc0000080 ; EFER.
    rdmsr

    or eax, 0x00000100  ; LME bit.
    wrmsr

    ; --> enable paging.
    mov eax, cr0
    or eax,   (1 << 31) ; PG on.
    mov cr0, eax

    ; --> jump to long mode.
    jmp (1 << 3):boot_long_mode
	.hang:
		jmp .hang

; global switch_gdt32
switch_gdt32:
    push ebp
    mov ebp, esp

    lgdt [__args_gdtr32]

    ; 32 bit data segment.
    mov ax, (10 << 3)
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    mov ss, ax

    ; 32 bit code segment.
    mov ax, (9 << 3)
    push ax
    push .done
    retf

    .done:
        mov esp, ebp
        pop ebp
        ret

align 16
bits 64
boot_long_mode:
    xor eax, eax
    mov ax, (3 << 3)    ; data segment.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; --> load entry point address.
    xor rax, rax
    mov eax, __args_entry64

    xor rbx, rbx
    mov rbx, qword [rax]

    ; --> switch stack to.
    mov rsp, __stack32.top
    mov rbp, rsp

    ; --> jump to entry point of 64bit kernel by `RET`.
    push rbx
    ret

; ------------------
; section .args
; global __args

align 16
__args:
    dd 0x20260218   ; MAGIC, dword.

__args_info:
    dw _start
    dw (__stack32.top - _start)
    dd 0

__args_tlpd:
    dd 0            ; TLPD pointer, must be 32-bit.

__args_entry64:
    dq 0            ; entry point.

__args_gdtr16:
    dd 0            ; GDTR: limit (16), addr (32)
    dd 0            ; extra 16bit: padding.

__args_gdtr32:
    dd 0            ; GDTR: limit (16), addr (32)
    dw 0            ; extra 16bit: padding.

__stack32:
    times 256 db 0
    .top: