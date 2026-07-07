#  XBTOS-CL - 基于 x86 架构的 32位教育型操作系统

**XBTOS** 是一个完全从零开始构建的 32位教育型操作系统。本项目旨在深入探索计算机底层运行机制，使用现代 C++ 和 NASM 汇编语言，在裸机（Bare-metal）环境下实现了内存管理、保护模式切换以及基础中断处理等核心系统功能。

> **开发理念**：拒绝轮子，回归底层。通过手写每一行启动代码和内存分配器，真正理解计算机是如何从通电那一刻起，一步步运行起复杂软件的。

[![Language](https://img.shields.io/badge/Language-C++%20/%20NASM-blue)]()
[![Architecture](https://img.shields.io/badge/Architecture-x86_32-green)]()
[![License](https://img.shields.io/badge/License-MIT-blue)]()
[![OS Dev](https://img.shields.io/badge/OS%20Dev-Bare%20Metal-orange)]()

## 🌟 核心特性 (Features)

- **现代构建系统**：采用 `CMake` 进行跨平台项目配置，集成 `CLion` 提供极致的 IDE 开发体验。
- **保护模式与 GDT**：纯汇编实现全局描述符表（GDT）加载，成功完成从实模式到 32位保护模式的平滑切换。
- **物理与虚拟内存管理**：从零实现物理内存分配器（Physical Memory Manager）与内核堆管理器（Heap Manager），支持动态内存分配。
- **硬件中断处理**：实现 8259A PIC 中断控制器配置，支持键盘硬件中断（ISR）及 C++ 层面的事件分发。
- **标准 C 接口桥接**：提供 `extern "C"` 封装的 `malloc/free` 等标准接口，方便上层应用调用。
- **Multiboot 启动协议**：完美支持 GRUB / QEMU 的 Multiboot 1.0 规范，实现内核的标准化加载。

## 🛠️ 技术栈 (Tech Stack)

| 分类 | 技术选型 | 说明 |
| :--- | :--- | :--- |
| **核心语言** | C++ / NASM | 内核逻辑与底层硬件交互 |
| **交叉编译器** | x86_64-elf-gcc/g++ | 裸机环境专属工具链 |
| **构建工具** | CMake + Ninja | 自动化编译与链接管理 |
| **开发环境** | JetBrains CLion | 提供强大的代码提示与调试支持 |
| **虚拟机** | QEMU | 系统模拟与运行测试 |

## 🚀 快速开始 (Getting Started)

### 1. 环境准备
在开始之前，请确保你的系统已安装以下工具：
- **交叉编译器**：`x86_64-elf-tools` (Windows/Linux)
- **汇编器**：`NASM` (Netwide Assembler)
- **虚拟机**：`QEMU` (需包含 `qemu-system-x86_64`)
- **构建工具**：`CMake` (>= 3.20)

### 2. 编译与运行
```bash
>>>>>>> 5b65755355476be0188f3443cefb9dd9e96f7170
# 克隆项目
git clone https://github.com/你的用户名/XBTOS.git
cd XBTOS

# 创建构建目录并配置
mkdir build && cd build
cmake ..

# 编译内核
cmake --build . --target XBTOS.elf

# 使用 QEMU 运行内核
qemu-system-x86_64 -kernel XBTOS.elf
<<<<<<< HEAD
项目结构 (Project Structure)
text

编辑



Project_XBTOS-CL/
├── boot/
│   └── boot.asm            # Multiboot 头、GDT 初始化与保护模式切换
├── kernel/
│   ├── include/            # 全局头文件 (如 types.h)
│   ├── memory/             # 内存管理子系统 (physmem, heap, stdlib)
│   └── main.cpp            # C++ 内核主入口
├── CMakeLists.txt          # 核心构建配置文件
├── linker.ld               # 自定义链接脚本 (控制内存段布局)
└── README.md               # 项目说明文档
贡献指南 (Contributing)
XBTOS 是一个持续演进的教育项目。如果你对其中的某个子系统（如 VFS 文件系统、进程调度）感兴趣，欢迎提交 Issue 或 Pull Request！
=======
```
### 3.项目结构
Project_XBTOS-CL/

├── boot/

│   └── boot.asm 

├── kernel/

│   ├── include/            #自定义标准库

│   ├── memory/             # 内存管理子系统 (physmem, heap, stdlib)

│   └── main.cpp            # C++ 内核主入口

├── CMakeLists.txt          # 核心构建配置文件

├── linker.ld               # 自定义链接脚本 (控制内存段布局)

└── README.md               # 项目说明文档

## 🤝 贡献指南 (Contributing)
### XBTOS 是一个持续演进的教育项目。如果你对其中的某个子系统（如 VFS 文件系统、进程调度）感兴趣，欢迎提交 Issue 或 Pull Request！

本项目基于 MIT License 开源。欢迎学习、修改与分享！
Made with ️ by 小白科技（XBT. Technology Studio）
