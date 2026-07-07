// kernel/include/types.h
#ifndef XBTOS_TYPES_H
#define XBTOS_TYPES_H

// --- 基础整数类型 ---
// 确保在不同编译器下大小一致
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

// --- 常用别名 ---
typedef uint32_t            size_t;
//typedef uint8_t             bool;

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif //XBTOS_TYPES_H