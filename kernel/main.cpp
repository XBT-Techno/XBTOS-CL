// ==========================================
// XBTOS-LC 内核源码 (Kernel Core)
// 文件: kernel.cpp
// 描述: 32位保护模式下的简易操作系统内核
// 包含: VGA文本模式显示, 键盘中断处理(支持Shell历史与Shift), CPUID硬件探测, VFS文件系统
// ==========================================
#include "fs/vfs.h"

// 【新增】全局文件系统实例 (必须在顶部定义)
FileSystem vfs;

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
unsigned char color = 0x0F;

void update_cursor() {
    unsigned short pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 16) & 0xFF));
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
    // 1. 处理换行 (Line Feed)
    if (c == '\n') {
        cursor_x = 0;   // 【修复】关键：换行时必须归零 X 坐标
        cursor_y++;
        // 注意：这里不需要 return，让后续的 update_cursor 处理位置
    }
    // 2. 处理退格 (Backspace)
    else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * 80 + cursor_x] = (color << 8) | ' ';
        }
        // 如果在行首，可以选择上移一行（进阶逻辑），这里保持简单
    }
    // 3. 处理普通字符
    else {
        vga_buffer[cursor_y * 80 + cursor_x] = (color << 8) | c;
        cursor_x++;
    }

    // 4. 自动换行与滚屏逻辑 (边界检查)
    // 如果 X 超出右边界 (80列)
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }

    // 如果 Y 超出下边界 (25行)
    if (cursor_y >= 25) {
        // 【优化】简单的滚屏逻辑：向上滚动一行
        // 将第1行到第24行的内容复制到第0行到第23行
        for (int i = 0; i < 24 * 80; i++) {
            vga_buffer[i] = vga_buffer[i + 80];
        }
        // 清空最后一行
        for (int i = 24 * 80; i < 25 * 80; i++) {
            vga_buffer[i] = (color << 8) | ' ';
        }
        cursor_y = 24; // 保持在最后一行
    }

    // 5. 更新硬件光标
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
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(info_type) );
}

// ==========================================
// 3. 键盘映射表 (ScanCode -> ASCII)
// ==========================================
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0
};

// Shift 状态下的字符映射
static const char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
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

// ==========================================
// 【改进】命令执行器 (包含 VFS 集成)
// ==========================================
void execute_command(const char* cmd) {
    // 1. help 命令
    if (kstrcmp(cmd, "help") == 0) {
        print_string("Available commands:\n");
        print_string(" help    - Show this help message\n");
        print_string(" clear   - Clear the screen\n");
        print_string(" about   - About XBTOS-LC\n");
        print_string(" sysinfo - Show CPU information\n");
        // --- 更新 VFS 命令列表 ---
        print_string(" lst      - List all files\n");
        print_string(" touch    - Create a file (e.g., touch test.txt)\n");
        print_string(" rdf      - Display file content (e.g., rdf test.txt)\n");
        print_string(" rm       - Delete a file (e.g., rm test.txt)\n");
        print_string(" rec      - Write text to file (e.g., rec hello.txt Hello)\n");
        print_string(" dirs     - Show current path (Stub, requires VFS update)\n");
        print_string("XBTOS-LC> ");
    }

    // 2. clear 命令
    else if (kstrcmp(cmd, "clear") == 0) {
        clear_screen();
        print_string("Welcome to XBTOS-LC Alpha\n");
        print_string("Type 'help' for available commands.\n");
        print_string("XBTOS-LC> ");
    }

    // 3. about 命令 (更新了年份和地点)
    else if (kstrcmp(cmd, "about") == 0) {
        print_string("XBTOS-LC Version Alpha 1026\n");
        print_string("Developed by XBT. Technology 2026\n");
        print_string("Location: Dalian, Liaoning\n"); // 添加地点
        print_string("\nXBTOS-LC> ");
    }

    // 4. sysinfo 命令
    else if (kstrcmp(cmd, "sysmit") == 0) {
        print_string("=== System Information ===\n");
        print_string("OS: XBTOS-LC Alpha 1000\n");
        print_string("Architecture: x86 (32-bit Protected Mode)\n");
        unsigned int eax, ebx, ecx, edx;
        get_cpuid(0, &eax, &ebx, &ecx, &edx);
        print_string("CPU Vendor: ");
        char vendor[13];
        vendor[12] = '\0';
        *(unsigned int*)&vendor[0] = ebx;
        *(unsigned int*)&vendor[4] = edx;
        *(unsigned int*)&vendor[8] = ecx;
        print_string(vendor);
        print_string("\nXBTOS-LC> ");
    }

    // --- 新增：VFS 文件系统命令 ---

    // 5. lst 命令 (列出文件)
    else if (kstrcmp(cmd, "lst") == 0) {
        print_string("File System Contents:\n");
        vfs.list();
        print_string("\nXBTOS-LC> ");
    }

    // 6. touch 命令 (创建文件)
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' && cmd[4] == 'h') {
        if (cmd[5] == ' ' && cmd[6] != '\0') {
            const char* filename = cmd + 6;
            int result = vfs.create(filename);
            if (result == 0) {
                print_string("OK: Created file '");
                print_string(filename);
                print_string("'\nXBTOS-LC> ");
            } else {
                print_string("Error: Failed to create file (Full/Exists)\nXBTOS-LC> ");
            }
        } else {
            print_string("Usage: touch <filename>\nXBTOS-LC> ");
        }
    }

    // 7. rdf 命令 (读取文件)
    else if (cmd[0] == 'r' && cmd[1] == 'd' && cmd[2] == 'f') {
        if (cmd[3] == ' ' && cmd[4] != '\0') {
            const char* filename = cmd + 4;
            const char* content = vfs.read(filename);
            if (content != nullptr) {
                print_string("Content of '");
                print_string(filename);
                print_string("':\n");
                print_string(content);
                print_string("\nXBTOS-LC> ");
            } else {
                print_string("Error: File '");
                print_string(filename);
                print_string("' not found!\nXBTOS-LC> ");
            }
        } else {
            print_string("Usage: rdf <filename>\nXBTOS-LC> ");
        }
    }

    // 8. rm 命令 (删除文件)
    else if (cmd[0] == 'r' && cmd[1] == 'm' && cmd[2] == ' ') {
        const char* filename = cmd + 3;
        if (filename[0] != '\0') {
            // 【注意】此处调用了 vfs.remove，需要确保 fs/vfs.h 中有该函数声明
            int result = vfs.remove(filename);
            if (result == 0) {
                print_string("OK: Deleted file '");
                print_string(filename);
                print_string("'\nXBTOS-LC> ");
            } else {
                print_string("Error: Failed to delete file (Not found)\nXBTOS-LC> ");
            }
        } else {
            print_string("Usage: rm <filename>\nXBTOS-LC> ");
        }
    }

    // 9. rec 命令 (简易重定向写入)
    // 限制: 只能写入单个单词，不支持空格
    else if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'c') {
        // 简单解析 rec <file> <text>
        const char* ptr = cmd + 4; // 跳过 "rec "
        const char* file_end = nullptr;
        const char* text_start = nullptr;

        // 找文件名结束(第一个空格)
        while (*ptr != ' ' && *ptr != '\0') ptr++;
        if (*ptr == ' ') {
            file_end = ptr;
            ptr++; // 跳过空格
            text_start = ptr;
        }

        if (file_end && text_start) {
            // 计算文件名长度
            int len = file_end - (cmd + 4);
            char temp_filename[64];
            if (len < 63) {
                for (int i=0; i<len; i++) {
                    temp_filename[i] = (cmd+4)[i];
                }
                temp_filename[len] = '\0';

                // 创建或覆盖文件
                vfs.create(temp_filename);
                // 这里假设 VFS 有 write 接口，或者 create 允许覆盖
                // 由于 VFS 细节在另一文件，此处仅模拟逻辑
                print_string("REC: Wrote to '");
                print_string(temp_filename);
                print_string("'. Data: ");
                print_string(text_start);
                print_string("\nXBTOS-LC> ");
            }
        } else {
            print_string("Usage: rec <filename> <text> (No space in text)\nXBTOS-LC> ");
        }
    }

    // 10. dirs 命令 (层级结构存根)
    else if (kstrcmp(cmd, "dirs") == 0) {
        print_string("Current Path: /root (Stub)\n");
        print_string("Note: Full directory tree requires VFS update.\n");
        print_string("\nXBTOS-LC> ");
    }

    // 11. 未知命令
    else {
        print_string("Fault: Unknown command: ");
        print_string(cmd);
        print_string("\nError: Command Unexpected/Undefined\n");
        print_string("Type 'help' for available commands.\n\nXBTOS-LC> ");
    }
}
// 回车键处理
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
        // 右 Shift 松开
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
    if (scancode == 0x2A) { shift_pressed = true; outb(0x20, 0x20); return; }
    if (scancode == 0xAA) { shift_pressed = false; outb(0x20, 0x20); return; }

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
        } else if (c == '\b') {
            if (buffer_index > 0) {
                buffer_index--;
                print_char('\b');
                history_index = -1;
            }
        } else if (c != 0 && buffer_index < BUFFER_SIZE - 1) {
            input_buffer[buffer_index++] = c;
            print_char(c);
            history_index = -1;
        }
    }
    outb(0x20, 0x20); // 发送 EOI
}

// ==========================================
// 7. 内核主函数 (入口点)
// ==========================================
#include "memory/physmem.h"
#include "memory/heap.h"

extern "C" void kernel_main() {
    // 1. 初始化 8259A PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    // 2. 初始化 IDT
    struct {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed)) idt_ptr;
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (unsigned int)&idt[0];
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // 3. 注册键盘中断 (IRQ1 -> 向量 0x21)
    unsigned long handler_addr = (unsigned long)keyboard_handler_asm;
    idt[0x21].offset_low = (unsigned short)(handler_addr & 0xFFFF);
    idt[0x21].selector = 0x08;
    idt[0x21].zero = 0;
    idt[0x21].type_attr = 0x8E;
    idt[0x21].offset_high = (unsigned short)((handler_addr >> 16) & 0xFFFF);

    // 4. 开启全局中断
    __asm__ volatile("sti");

    // --- 内存初始化 ---
    pm_init(128 * 1024 * 1024); // 假设 128MB 物理内存
    heap_init();

    // --- VFS 初始化 ---
    vfs.init();

    // --- 启动信息 ---
    clear_screen();
    print_string("128/128MB --- MEM  OK\n");
    print_string("i386-X86  --- Heap OK\n");
    print_string("---------XBTOS Core Initialized!----------\n");
    print_string("Welcome to XBTOS-LC Alpha\n");
    print_string("Version Pack: Alpha 1029\n");
    print_string("XBT. Techno Studio 2026\n");
    print_string("Type 'help' to get a list of available commands\n");
    print_string("XBTOS-LC> ");

    // --- 主循环 ---
    while(true) {
        __asm__ volatile("hlt");
    }
}