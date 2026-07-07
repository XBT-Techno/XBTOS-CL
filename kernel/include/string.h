#ifndef STRING_H
#define STRING_H

#include "types.h" 
inline void* kmemset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

// 2. 内存拷贝 (类似 memcpy)
inline void* kmemcpy(void* destination, const void* source, size_t num) {
    unsigned char* dest = (unsigned char*)destination;
    const unsigned char* src = (const unsigned char*)source;
    while (num--) {
        *dest++ = *src++;
    }
    return destination;
}

// 3. 字符串比较 (类似 strcmp)
inline int kstrcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// 4. 字符串长度 (类似 strlen)
inline size_t kstrlen(const char* str) {
    size_t len = 0;
    while (*str++) len++;
    return len;
}

#endif