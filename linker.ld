ENTRY(_start)

SECTIONS {
    . = 1M; /* 内核加载到 1MB 处 */

    /* 1. 必须把 multiboot 头放在最前面！ */
    .multiboot BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
    }

    /* 2. 然后是代码段 */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.text)
    }

    /* 3. 只读数据段 */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* 4. 读写数据段 */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* 5. BSS 段 (未初始化变量) */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}