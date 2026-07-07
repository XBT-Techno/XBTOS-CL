// kernel/include/fs/vfs.h
#ifndef VFS_H
#define VFS_H

// 【修复】直接使用全局搜索路径，去掉 "../"
#include "types.h"
#include <string.h>  // 同样建议去掉相对路径，如果你的 string.h 也在 include 目录下

// 2. 定义文件系统常量
#define MAX_FILES        64
#define FILE_NAME_SIZE   32
#define FILE_DATA_SIZE   1024

struct File {
    char name[FILE_NAME_SIZE]; // 现在编译器知道 FILE_NAME_SIZE 是 32
    char data[FILE_DATA_SIZE];
    uint32_t size;             // 现在编译器知道 uint32_t 是 unsigned int
    bool is_used;
};
// 文件系统管理类
class FileSystem {
private:
    File files[MAX_FILES];    // 文件表
    uint32_t file_count;      // 当前文件数量

public:
    FileSystem();

    // 核心功能
    void init();              // 初始化
    int create(const char* name); // 创建文件
    int write(const char* name, const char* content); // 写入内容
    const char* read(const char* name); // 读取内容
    void list();              // 列出所有文件
    File* find(const char* name); // 查找文件
    int remove(const char* filename);
};
// 4. 字符串拷贝 (类似 strcpy)
inline char* kstrcpy(char* dest, const char* src) {
    char* temp = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0'; // 别忘了加上字符串结束符！
    return temp;
}

#endif