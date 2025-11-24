#include "vmalloc.h"
#include <stdio.h>

int main() {
    void *a = my_malloc(100);
    void *b = my_calloc(5, 10);

    printf("Allocated a=%p, b=%p\n", a, b);
    print_heap();

    my_free(a);
    my_free(b);

    return 0;
}

