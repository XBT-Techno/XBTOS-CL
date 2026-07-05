[BITS 32]
DEFAULT REL

; ==========================================
; 1. Multiboot 1.0 标识头 (QEMU/GRUB 启动必需)
; ==========================================
MBOOT_MAGIC     equ  0x1BADB002         ; Multiboot 魔数
MBOOT_MEM_INFO  equ  1 << 0             ; 要求 bootloader 提供内存信息
MBOOT_FLAGS     equ  MBOOT_MEM_INFO     ; 标志位
MBOOT_CHECKSUM  equ  -(MBOOT_MAGIC + MBOOT_FLAGS) ; 校验和 (保证三者相加为0)

section .multiboot
align 4
    dd MBOOT_MAGIC      ; 魔数
    dd MBOOT_FLAGS      ; 标志
    dd MBOOT_CHECKSUM   ; 校验和

; ==========================================
; 2. GDT (全局描述符表) 定义
; ==========================================
section .data
align 8
gdt_start:
    dq 0x0000000000000000 ; Null 描述符
    dq 0x00CF9A000000FFFF ; Code Segment (0x08)
    dq 0x00CF92000000FFFF ; Data Segment (0x10)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT 大小
    dd gdt_start               ; GDT 起始地址

CODE_SEG equ 0x08
DATA_SEG equ 0x10

; ==========================================
; 3. 外部声明与全局导出
; ==========================================
extern kernel_main        ; 声明 C++ 内核主函数
extern keyboard_handler   ; 声明 C++ 键盘中断处理函数

global _start             ; 导出程序入口
global keyboard_handler_asm ; 导出汇编中断外壳

; ==========================================
; 4. 代码段与入口点
; ==========================================
section .text

_start:
    cli                   ; 1. 关中断
    lgdt [gdt_descriptor] ; 2. 加载 GDT

    ; 3. 开启保护模式 (设置 CR0 的 PE 位)
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 4. 远跳转，刷新 CS 寄存器进入 32 位保护模式
    jmp CODE_SEG:.flush_cs
.flush_cs:
    ; 5. 更新所有数据段寄存器
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 6. 初始化栈指针 (放在 1MB 以下的空闲区，比如 0x90000)
    mov esp, 0x90000

    ; 7. 跳转到 C++ 内核！
    call kernel_main

    ; 8. 如果 kernel_main 意外返回，死循环挂起
.hang:
    hlt
    jmp .hang

; ==========================================
; 5. 键盘中断汇编外壳 (ISR Wrapper)
; ==========================================
keyboard_handler_asm:
    pusha                 ; 1. 保护所有通用寄存器现场

    call keyboard_handler ; 2. 调用 C++ 写的实际处理函数

    ; 3. 发送 EOI (End of Interrupt) 给主 PIC (8259A)
    ; 告诉中断控制器：中断处理完毕，可以接收下一个中断了
    mov al, 0x20
    out 0x20, al

    popa                  ; 4. 恢复寄存器现场
    iret                  ; 5. 中断返回 (注意：必须用 iret，不能用 ret！)