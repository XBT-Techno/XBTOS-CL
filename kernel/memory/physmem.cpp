#include "physmem.h"

#define PAGE_SIZE 4096
static uint32_t* memory_bitmap = 0;
static uint32_t bitmap_size = 0;

void pm_init(uint32_t mem_size) {
    uint32_t num_pages = mem_size / PAGE_SIZE;
    bitmap_size = (num_pages + 31) / 32;
    // 将位图放在 1MB 处（假设内核小于 1MB）
    memory_bitmap = (uint32_t*)0x100000; 
    for (uint32_t i = 0; i < bitmap_size; i++) memory_bitmap[i] = 0;
    // 标记前 1MB 为已使用
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t index = i / 32;
        uint32_t offset = i % 32;
        memory_bitmap[index] |= (1 << offset);
    }
}

void* pm_alloc_page() {
    for (uint32_t i = 0; i < bitmap_size; i++) {
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(memory_bitmap[i] & (1 << j))) {
                    memory_bitmap[i] |= (1 << j);
                    return (void*)((i * 32 + j) * PAGE_SIZE);
                }
            }
        }
    }
    return nullptr;
}

void pm_free_page(void* addr) {
    uint32_t frame = (uint32_t)addr / PAGE_SIZE;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;
    memory_bitmap[index] &= ~(1 << offset);
}