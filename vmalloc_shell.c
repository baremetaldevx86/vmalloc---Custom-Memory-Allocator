#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef char ALIGN[16];

union header {
    struct {
        size_t size;              // Block size
        unsigned is_free;         // Free/allocated flag
        union header *next;       // Next block in the list
        union header *prev;       // Previous block in the list
    } s;
    ALIGN stub;                   // Force 16-byte alignment
};

typedef union header header_t;

header_t *head = NULL, *tail = NULL;// Doubly linked list of all blocks
pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size) {
    header_t *curr = head;
    while (curr) {
        if (curr->s.is_free && curr->s.size >= size) {
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}

// Check if two blocks are physically adjacent in memory
int blocks_adjacent(header_t *first, header_t *second) {
    return ((char*)first + sizeof(header_t) + first->s.size) == (char*)second;
}

// Coalesce adjacent free blocks
void coalesce(header_t *block) {
    // First, coalesce with all following adjacent free blocks
    while (block->s.next && block->s.next->s.is_free && blocks_adjacent(block, block->s.next)) {
        header_t *next = block->s.next;
        block->s.size += sizeof(header_t) + next->s.size;
        block->s.next = next->s.next;
        if (next->s.next) {
            next->s.next->s.prev = block;
        } else {
            tail = block;  // next was the tail
        }
    }
    
    // Then coalesce with previous block if it's free and adjacent
    if (block->s.prev && block->s.prev->s.is_free && blocks_adjacent(block->s.prev, block)) {
        header_t *prev = block->s.prev;
        prev->s.size += sizeof(header_t) + block->s.size;
        prev->s.next = block->s.next;
        if (block->s.next) {
            block->s.next->s.prev = prev;
        } else {
            tail = prev;  // block was the tail
        }
    }
}

void *my_malloc(size_t size) {
    if (!size) return NULL;
    
    pthread_mutex_lock(&global_malloc_lock);
    
    // Try to find a free block
    header_t *header = get_free_block(size);
    if (header) {
        // Check if block is large enough to split
        // Only split if remainder can hold header + at least 16 bytes of data
        size_t remaining = header->s.size - size;
        if (remaining > sizeof(header_t) + 16) { // Minimum viable block size
            // Split the block
            header_t *new_block = (header_t*)((char*)header + sizeof(header_t) + size);
            new_block->s.size = remaining - sizeof(header_t);
            new_block->s.is_free = 1;
            new_block->s.next = header->s.next;
            new_block->s.prev = header;
            
            if (header->s.next) {
                header->s.next->s.prev = new_block;
            } else {
                tail = new_block; // new_block is now the tail
            }
            
            header->s.next = new_block;
            header->s.size = size; // Shrink original block to requested size
        }
        
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    
    // No free block found, request memory from OS
    size_t total_size = sizeof(header_t) + size;
    void *block = sbrk(total_size);
    if (block == (void*)-1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    
    // Setup the new block header
    header = (header_t*)block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    header->s.prev = tail;
    
    if (!head) {
        head = tail = header;
    } else {
        tail->s.next = header;
        tail = header;
    }
    
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}

void my_free(void *block) {
    if (!block) return;
    
    pthread_mutex_lock(&global_malloc_lock);
    
    header_t *header = (header_t*)block - 1;
    
    // Basic validation - prevent double free
    if (header->s.is_free) {
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    
    void *programbreak = sbrk(0);
    
    // Check if block is at the end of the heap (fixed boundary check)
    if ((char*)header + sizeof(header_t) + header->s.size == programbreak) {
        // Remove from list
        if (header->s.prev) {
            header->s.prev->s.next = NULL;
            tail = header->s.prev;
        } else {
            head = tail = NULL;
        }
        
        // Return memory to OS
        sbrk(0 - sizeof(header_t) - header->s.size);
    } else {
        // Mark block as free and coalesce with adjacent free blocks
        header->s.is_free = 1;
        coalesce(header);
    }
    
    pthread_mutex_unlock(&global_malloc_lock);
}

void *my_calloc(size_t num, size_t nsize) {
    if (!num || !nsize) return NULL;

    size_t size = num * nsize;
    if (nsize > size / num) return NULL;

    void *block = my_malloc(size);
    if (!block) return NULL;

    memset(block, 0, size);
    return block;
}

void *my_realloc(void *block, size_t size) {
    if (!block) return my_malloc(size);
    if (!size) {
        my_free(block);
        return NULL;
    }

    header_t *header = (header_t*)block - 1;
    if (header->s.size >= size) {
        return block; // shrinking in place
    }

    void *ret = my_malloc(size);
    if (ret) {
        memcpy(ret, block, header->s.size);
        my_free(block);
    }
    return ret;
}

size_t mem_available(void) {
    pthread_mutex_lock(&global_malloc_lock);

    if (head == NULL) {
        pthread_mutex_unlock(&global_malloc_lock);
        return 0;  // but caller should interpret as “no heap yet”
    }

    size_t total = 0;
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free)
            total += curr->s.size;
        curr = curr->s.next;
    }

    pthread_mutex_unlock(&global_malloc_lock);
    return total;
}




// Debug function to print heap state
void print_heap() {
    printf("\n=== HEAP STATE ===\n");
    header_t *curr = head;
    int block_num = 0;
    while (curr) {
        void *user_ptr = (void*)(curr + 1);
        printf("Block %d: addr=%p, size=%zu, free=%s\n", 
               block_num++, curr, curr->s.size, curr->s.is_free ? "YES" : "NO");
        curr = curr->s.next;
    }
    printf("=================\n\n");
}


char cmd[32];
size_t size;
void *ptr;

int main() {
    pthread_mutex_init(&global_malloc_lock, NULL);

    printf("======Custom Allocator======\n");
    printf("Commands:\n");
    printf("  malloc <size>\n");
    printf("  calloc <num> <size>\n");
    printf("  realloc <ptr> <size>\n");
    printf("  free <ptr>\n");
    printf("  heap\n");
    printf("  freebytes\n\n");

    while (scanf("%s", cmd) != EOF) {

        if (strcmp(cmd, "malloc") == 0) {
            scanf("%zu", &size);
            ptr = my_malloc(size);
            printf("Allocated %zu bytes at %p\n", size, ptr);
        }

        else if (strcmp(cmd, "free") == 0) {
            void *tptr;
            scanf("%p", &tptr);
            my_free(tptr);
            printf("Freed memory at %p\n", tptr);
        }

        else if (strcmp(cmd, "realloc") == 0) {
            void *oldptr;
            scanf("%p %zu", &oldptr, &size);
            void *newptr = my_realloc(oldptr, size);
            printf("Reallocated %p → %p\n", oldptr, newptr);
        }

        else if (strcmp(cmd, "calloc") == 0) {
        size_t num, nsize;
        scanf("%zu %zu", &num, &nsize);

        void *newptr = my_calloc(num, nsize);
        printf("Calloc'd %zu elements of %zu bytes each at %p\n",num, nsize, newptr);
        }


        else if (strcmp(cmd, "heap") == 0) {
            print_heap();
        }
        else if (strcmp(cmd, "freebytes") == 0) {
            size_t free = mem_available();
            if(!head) {
                printf("Heap not initialized yet (no memory allocated)\n");
            }        
            else {
                printf("Free memory in allocator heap: %zu bytes\n", mem_available());
            }
        }

        else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    pthread_mutex_destroy(&global_malloc_lock);
    return 0;
}
