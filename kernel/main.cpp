// ==========================================
// XBTOS-LC 内核源码 (Kernel Core)
// 文件: kernel.cpp
// 描述: 32位保护模式下的简易操作系统内核
// 包含: VGA文本模式显示, 键盘中断处理(支持Shell历史与Shift), CPUID硬件探测
// ==========================================

// ==========================================
// 1. 硬件 I/O 操作与 VGA 显存管理
// ==========================================
static inline unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// VGA 显存映射与光标控制
volatile unsigned short* const vga_buffer = (volatile unsigned short*)0xB8000;
int cursor_x = 0;
int cursor_y = 0;
unsigned char color = 0x0E; // 属性字节: 0x0E = 黄字 (0x0E) 黑底 (0x00)

void update_cursor() {
    unsigned short pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void print_char(char c) {
    // 简单的换页逻辑
    if (cursor_y >= 25) {
        cursor_y = 0;
        cursor_x = 0;
    }

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * 80 + cursor_x] = (color << 8) | ' ';
        }
    } else {
        vga_buffer[cursor_y * 80 + cursor_x] = (color << 8) | c;
        cursor_x++;
    }
    update_cursor();
}

void print_string(const char* str) {
    if (!str) str = "(null)";
    for (int i = 0; str[i] != '\0'; ++i) {
        print_char(str[i]);
    }
}

// ==========================================
// 2. 硬件探测 (CPUID)
// ==========================================
void get_cpuid(unsigned int info_type, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(info_type)
    );
}

// ==========================================
// 3. 键盘映射表 (ScanCode -> ASCII)
// ==========================================
static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

// Shift 状态下的字符映射 (例如 !@#$)
static const char scancode_to_ascii_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0
};

// ==========================================
// 4. Shell 历史记录与命令行缓冲区
// ==========================================
#define BUFFER_SIZE 256
#define HISTORY_SIZE 10

char input_buffer[BUFFER_SIZE];
int buffer_index = 0;
bool shift_pressed = false;

// 历史命令存储
char history[HISTORY_SIZE][BUFFER_SIZE];
int history_count = 0;
int history_index = -1; // -1 表示当前不在浏览历史状态

void clear_input_buffer() {
    while (buffer_index > 0) {
        buffer_index--;
        print_char('\b');
    }
    input_buffer[0] = '\0';
}

// 命令执行器
void execute_command(const char* cmd) {
    // 1. help 命令
    if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0') {
        print_string("Available commands:\n");
        print_string(" help    - Show this help message\n");
        print_string(" clear   - Clear the screen\n");
        print_string(" about   - About XBTOS-LC\n");
        print_string(" sysinfo - whow CPU information\n");
        print_string("\nXBTOS-LC> ");
    }
    // 2. clear 命令
    else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' &&
             cmd[4] == 'r' && cmd[5] == '\0') {
        clear_screen();
        print_string("Welcome to XBTOS-LC Alpha\n");
        print_string("Type 'help' for available commands.\n");
        print_string("XBTOS-LC> ");
    }
    // 3. about 命令
    else if (cmd[0] == 'a' && cmd[1] == 'b' && cmd[2] == 'o' && cmd[3] == 'u' &&
             cmd[4] == 't' && cmd[5] == '\0') {
        print_string("XBTOS-LC Version Alpha 1000\n");
        print_string("Developed by XBT. Technology 2026\n");
        print_string("\nXBTOS-LC> ");
    }
    // 4. sysinfo 命令 (修复版)
    else if (cmd[0] == 's' && cmd[1] == 'y' && cmd[2] == 's' && cmd[3] == 'i' &&
             cmd[4] == 'n' && cmd[5] == 'f' && cmd[6] == 'o' && cmd[7] == '\0') {

        print_string("=== System Information ===\n");
        print_string("OS: XBTOS-LC Alpha 1000\n");
        print_string("Architecture: x86 (32-bit Protected Mode)\n");

        unsigned int eax, ebx, ecx, edx;
        get_cpuid(0, &eax, &ebx, &ecx, &edx);
        print_string("CPU Vendor: ");

        // 安全构造字符串: 手动提取 12 个字节
        char vendor[13];
        vendor[12] = '\0';

        // 填充 EBX (Bytes 0-3)
        *(unsigned int*)&vendor[0] = ebx;
        // 填充 EDX (Bytes 4-7)
        *(unsigned int*)&vendor[4] = edx;
        // 填充 ECX (Bytes 8-11)
        *(unsigned int*)&vendor[8] = ecx;

        print_string(vendor);
        print_string("\nXBTOS-LC> ");
    }
    // 5. 未知命令处理
    else {
        print_string("Fault: Unknown command: ");
        print_string(cmd);
        print_string("\nError: Command Unexpected/Undefined\n");
        print_string("Type 'help' for available commands.\n\nXBTOS-LC> ");
    }
}

// 回车键处理 (存储历史并执行)
void handle_enter() {
    print_string("\n");
    input_buffer[buffer_index] = '\0';

    if (buffer_index > 0) {
        // 存储到历史记录
        if (history_count < HISTORY_SIZE) {
            for (int i = 0; i <= buffer_index; i++) {
                history[history_count][i] = input_buffer[i];
            }
            history_count++;
        } else {
            // 覆盖最旧的记录
            for (int i = 0; i < HISTORY_SIZE - 1; i++) {
                for (int j = 0; j < BUFFER_SIZE; j++) {
                    history[i][j] = history[i+1][j];
                }
            }
            for (int i = 0; i <= buffer_index; i++) {
                history[HISTORY_SIZE-1][i] = input_buffer[i];
            }
        }
        history_index = -1; // 重置历史索引
        execute_command(input_buffer);
    }

    buffer_index = 0;
}

// ==========================================
// 5. IDT结构体
// ==========================================
struct IDTEntry {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_high;
} __attribute__((packed));

extern "C" {
    IDTEntry idt[256];
    void keyboard_handler_asm(); // 汇编编写的中断入口桩
    void kernel_main();
}

// ==========================================
// 6. 中断处理程序
// ==========================================
extern "C" void keyboard_handler() {
    unsigned char scancode = inb(0x60);

    // --- 处理扩展键 (前缀 0xE0) ---
    if (scancode == 0xE0) {
        unsigned char ext_code = inb(0x60);

        // 右 Shift 按下 (0xE0, 0xB6)
        if (ext_code == 0xB6) {
            shift_pressed = true;
            outb(0x20, 0x20);
            return;
        }
        // 右 Shift 松开 (0xE0, 0xF0, 0xB6) 或 (0xE0, 0xD6)
        // 简化处理: 只要收到 D6 或者 F0(我们忽略F0的具体配对，直接视为释放信号)，就设为 false
        else if (ext_code == 0xD6 || ext_code == 0xF0) {
            shift_pressed = false;
            outb(0x20, 0x20);
            return;
        }

        // --- 命令行历史记录控制 (上下箭头) ---
        // 上箭头: 0xE0 0x48
        if (ext_code == 0x48 && history_count > 0) {
            if (history_index == -1) history_index = history_count - 1;
            else if (history_index > 0) history_index--;

            clear_input_buffer();

            // 回显选中的历史命令
            for (int i = 0; history[history_index][i] != '\0'; i++) {
                input_buffer[buffer_index++] = history[history_index][i];
                print_char(history[history_index][i]);
            }
        }
        // 下箭头: 0xE0 0x50
        else if (ext_code == 0x50 && history_count > 0) {
            if (history_index != -1) {
                history_index++;
                clear_input_buffer();
                if (history_index < history_count) {
                    for (int i = 0; history[history_index][i] != '\0'; i++) {
                        input_buffer[buffer_index++] = history[history_index][i];
                        print_char(history[history_index][i]);
                    }
                } else {
                    history_index = -1; // 回到输入模式
                }
            }
        }

        outb(0x20, 0x20);
        return;
    }

    // --- 处理普通扫描码 (左 Shift 等) ---

    // 左 Shift 按下 (0x2A)
    if (scancode == 0x2A) {
        shift_pressed = true;
        outb(0x20, 0x20);
        return;
    }
    // 左 Shift 松开 (0xAA)
    else if (scancode == 0xAA) {
        shift_pressed = false;
        outb(0x20, 0x20);
        return;
    }

    // --- 字符输入处理 ---
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = 0;

        if (shift_pressed) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        if (c == '\n') {
            handle_enter();
        }
        else if (c == '\b') {
            if (buffer_index > 0) {
                buffer_index--;
                print_char('\b');
                history_index = -1;
            }
        }
        else if (c != 0 && buffer_index < BUFFER_SIZE - 1) {
            input_buffer[buffer_index++] = c;
            print_char(c);
            history_index = -1; // 输入新字符时退出历史浏览模式
        }
    }

    outb(0x20, 0x20); // 发送 EOI 到主 PIC
}

// ==========================================
// 7. 内核主函数 (入口点)
// ==========================================
#include "memory/physmem.h" // 声明 pm_init 等
#include "memory/heap.h"    // 声明 heap_init, kmalloc 等
extern "C" void kernel_main() {
    // 1. 初始化 8259A PIC (可编程中断控制器)
    // ICW1 (边沿触发, 级联, 需要 ICW4)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    // ICW2 (中断向量偏移)
    outb(0x21, 0x20); // IRQ0-7 映射到 0x20-0x27
    outb(0xA1, 0x28); // IRQ8-15 映射到 0x28-0x2F
    // ICW3 (级联主从)
    outb(0x21, 0x04); // 主片 IR2 接从片
    outb(0xA1, 0x02);
    // ICW4 (32位保护模式)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    // OCW1 (屏蔽除键盘外的所有中断)
    outb(0x21, 0xFD); // 允许 IRQ1 (键盘)
    outb(0xA1, 0xFF); // 禁用从片所有中断

    // 2. 初始化 IDT
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    // 加载 IDT
    struct { unsigned short limit; unsigned int base; } __attribute__((packed)) idt_ptr;
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (unsigned int)&idt[0];
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // 3. 注册键盘中断 (IRQ1 -> 向量 0x21)
    unsigned long handler_addr = (unsigned long)keyboard_handler_asm;
    idt[0x21].offset_low = (unsigned short)(handler_addr & 0xFFFF);
    idt[0x21].selector = 0x08; // 代码段选择子
    idt[0x21].zero = 0;
    idt[0x21].type_attr = 0x8E; // 中断门 (Present, DPL=0, 32-bit)
    idt[0x21].offset_high = (unsigned short)((handler_addr >> 16) & 0xFFFF);

    // 4. 开启全局中断
    __asm__ volatile("sti");

    pm_init(128 * 1024 * 1024);
    heap_init(); // 初始化内核堆

    // ... 之后的 Shell 启动代码 ...
    clear_screen();
    for (int i = 0; i < 2; i++){
        print_string("128/128KB --- SD OK \n --Malloc OK\n");
    }

    print_string("\n---------XBTOS Memory Manager Initialized!----------\n");
    print_string("Welcome to XBTOS-LC Alpha\n");
    print_string("Version Pack: Alpha - 1020\n");
    print_string("Type 'help' to get a list of available commands\n");
    print_string("XBTOS-LC> ");

    // 现在你可以在 Shell 命令里测试 malloc 了！
    while(true) {
        __asm__ volatile("hlt");
    }
}