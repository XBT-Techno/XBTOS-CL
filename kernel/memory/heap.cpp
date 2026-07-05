#include "heap.h"

#define HEAP_START 0x200000  // 堆起始地址 (2MB处)
#define HEAP_SIZE  0x100000  // 堆大小 1MB

struct Header {
    uint32_t size;
    uint8_t free;
    Header* next;
};

static Header* free_list = 0;

void heap_init() {
    free_list = (Header*)HEAP_START;
    free_list->size = HEAP_SIZE;
    free_list->free = 1;
    free_list->next = 0;
}

void* kmalloc(uint32_t size) {
    if (size == 0) return 0;
    uint32_t total_size = size + sizeof(Header);
    Header* curr = free_list;
    Header* prev = 0;

    while (curr) {
        if (curr->free && curr->size >= total_size) {
            if (curr->size >= total_size + sizeof(Header) + 32) {
                Header* new_block = (Header*)((uint8_t*)curr + total_size);
                new_block->size = curr->size - total_size;
                new_block->free = 1;
                new_block->next = curr->next;
                curr->size = total_size;
                curr->free = 0;
                curr->next = new_block;
                if (prev) prev->next = curr;
                else free_list = curr;
            } else {
                curr->free = 0;
                if (prev) prev->next = curr->next;
                else free_list = curr->next;
            }
            return (void*)((uint8_t*)curr + sizeof(Header));
        }
        prev = curr;
        curr = curr->next;
    }
    return nullptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    Header* header = (Header*)((uint8_t*)ptr - sizeof(Header));
    header->free = 1;
    header->next = free_list;
    free_list = header;
}