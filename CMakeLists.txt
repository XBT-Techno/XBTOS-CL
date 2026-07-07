cmake_minimum_required(VERSION 3.20)

# ==========================================
# 1. 项目基础配置 (裸机环境)
# ==========================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(XBTOS C CXX ASM_NASM)

# ==========================================
# 2. 指定交叉编译器路径
# ==========================================
set(CMAKE_C_COMPILER "E:/x86_64-elf-tools-windows/bin/x86_64-elf-gcc.exe")
set(CMAKE_CXX_COMPILER "E:/x86_64-elf-tools-windows/bin/x86_64-elf-g++.exe")
set(CMAKE_ASM_NASM_COMPILER "C:/Program Files/NASM/nasm.exe")

# ==========================================
# 3. 启用 NASM 并强制指定 32 位 ELF 编译规则
# ==========================================
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_COMPILE_OBJECT
        "<CMAKE_ASM_NASM_COMPILER> -f elf32 -o <OBJECT> <SOURCE>"
)

# ==========================================
# 4. 定义内核可执行文件
# ==========================================
add_executable(XBTOS.elf
        boot/boot.asm
        kernel/main.cpp
        kernel/include/types.h
        kernel/memory/physmem.cpp
        kernel/memory/physmem.h
        kernel/memory/heap.cpp
        kernel/memory/heap.h
        kernel/memory/stdlib.cpp
        kernel/fs/vfs.cpp
        kernel/fs/vfs.h
        kernel/include/string.h
)

target_include_directories(XBTOS.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/kernel/include)

# ==========================================
# 5. C++ 专属编译选项 (使用生成器表达式，避免污染 NASM)
# ==========================================
target_compile_options(XBTOS.elf PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-m32>
        $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding>
        $<$<COMPILE_LANGUAGE:CXX>:-nostdlib>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-pie>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-stack-protector>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        -g -O0 -Wall
)

# ==========================================
# 6. 【核心修复】使用 LINKER: 前缀传递链接器参数
# ==========================================
# 1. 明确指定 linker.ld 的绝对路径，防止 CMake 解析 Windows 路径出错
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

# 2. 使用 LINKER: 前缀，这是 CMake 传递链接器参数的最佳实践
target_link_options(XBTOS.elf PRIVATE
        -m32
        -nostdlib
        -static
        -Wl,-m,elf_i386
        LINKER:-T,${LINKER_SCRIPT}
)