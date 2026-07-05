#ifndef PHYSMEM_H
#define PHYSMEM_H
#include "../include/types.h"

void pm_init(uint32_t mem_size);
void* pm_alloc_page();
void pm_free_page(void* addr);
#endif