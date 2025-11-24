# 🧠 vmalloc — A Simple Custom Memory Allocator (C)

`vmalloc` is a lightweight, educational memory allocator implemented in C.  
It uses `sbrk()` to grow the heap, maintains a doubly linked list of blocks,  
supports block splitting, coalescing, and exposes the classic API:

```c
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t num, size_t size);
void *my_realloc(void *ptr, size_t size);
