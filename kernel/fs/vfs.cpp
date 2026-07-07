// kernel/fs/vfs.cpp
#include "vfs.h"

// 【重要】假设你的 print_string 函数定义在 main.cpp 中
// 这里通过 extern "C" 或普通 extern 声明它，以便在 VFS 中调用
extern void print_string(const char* str);

// 构造函数：初始化为空
FileSystem::FileSystem() : file_count(0) {
    kmemset(files, 0, sizeof(files));
}

// 初始化函数
void FileSystem::init() {
    print_string("VFS Initialized.\n");
}

// 查找文件辅助函数 (私有)
File* FileSystem::find(const char* name) {
    for (uint32_t i = 0; i < file_count; i++) {
        // 只有 is_used 为 true 的文件才进行比较
        if (files[i].is_used && kstrcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return nullptr;
}

// 创建文件
int FileSystem::create(const char* name) {
    if (file_count >= MAX_FILES) return -1; // 空间已满
    if (find(name) != nullptr) return -2;   // 文件已存在

    File* new_file = &files[file_count++];
    kstrcpy(new_file->name, name);
    new_file->size = 0;
    new_file->is_used = true;
    kmemset(new_file->data, 0, FILE_DATA_SIZE);
    return 0; // 成功
}

// 写入文件
int FileSystem::write(const char* name, const char* content) {
    File* f = find(name);
    if (!f) return -1; // 文件不存在

    uint32_t len = kstrlen(content);
    // 防止溢出，最多写入 FILE_DATA_SIZE - 1 个字符，留一个给 '\0'
    if (len >= FILE_DATA_SIZE) {
        len = FILE_DATA_SIZE - 1;
    }

    kmemcpy(f->data, content, len);
    f->data[len] = '\0';
    f->size = len;
    return 0; // 成功
}

// 读取文件
const char* FileSystem::read(const char* name) {
    File* f = find(name);
    if (!f) return nullptr; // 文件不存在
    return f->data;         // 返回文件内容的指针
}

// 列出所有文件
void FileSystem::list() {
    bool found = false;
    for (uint32_t i = 0; i < file_count; i++) {
        if (files[i].is_used) {
            found = true;
            print_string(" - ");
            print_string(files[i].name);
            print_string("\n");
        }
    }

    if (!found) {
        print_string(" (No files found)\n");
    }
}

// 删除文件
int FileSystem::remove(const char* filename) {
    for (uint32_t i = 0; i < file_count; i++) {
        if (files[i].is_used && kstrcmp(files[i].name, filename) == 0) {
            // 找到文件，将其标记为未使用
            files[i].is_used = false;
            files[i].size = 0;
            kmemset(files[i].name, 0, FILE_NAME_SIZE);
            kmemset(files[i].data, 0, FILE_DATA_SIZE);

            // 【数组压缩】将最后一个文件移动到当前被删除的位置
            // 这样 file_count 减 1 后，数组依然是紧凑的
            if (i != file_count - 1) {
                files[i] = files[file_count - 1];
            }
            file_count--;
            return 0; // 成功删除
        }
    }
    return -1; // 未找到该文件
}