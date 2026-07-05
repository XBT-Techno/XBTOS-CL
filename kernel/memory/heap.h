#ifndef HEAP_H
#define HEAP_H
#include "../include/types.h"

void heap_init();
void* kmalloc(uint32_t size);
void kfree(void* ptr);
#endif