#ifndef VMALLOC_H
#define VMALLOC_H

#include <stddef.h>

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t num, size_t size);
void *my_realloc(void *ptr, size_t size);

size_t mem_available(void);
void print_heap(void);

#endif

