// kernel/memory/stdlib.cpp

// 1. 使用尖括号，利用 CMake 配置的 include 路径
#include <types.h>
// 2. 引用同目录下的 heap 管理器
#include "heap.h"

// 告诉编译器：这些函数要按 C 语言规则导出符号
extern "C" {

/**
 * @brief 标准内存分配接口
 * @param size 需要分配的字节数
 * @return 成功返回指针，失败返回 nullptr
 */
void* malloc(size_t size) {
    // 在 32 位环境下 size_t 和 uint32_t 等价，直接转换
    return kmalloc(static_cast<uint32_t>(size));
}

/**
 * @brief 标准内存释放接口
 * @param ptr 待释放的内存指针
 */
void free(void* ptr) {
    // 增加空指针检查是好习惯，虽然 kfree 内部可能也做了
    if (ptr != nullptr) {
        kfree(ptr);
    }
}

} // extern "C" end