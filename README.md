cat > README.md <<EOF
# Custom Memory Allocator (vmalloc)
🧠 vmalloc — A Custom Memory Allocator in C (with REPL Shell)

vmalloc is a fully custom memory allocator implemented in C.
It provides malloc, free, calloc, and realloc equivalents built from scratch using:

sbrk() for heap growth

A doubly linked list of block headers

Block splitting

Coalescing of free blocks

Thread safety via pthread_mutex

A full interactive REPL shell for educational use

This project is ideal for systems programming learners, OS developers, and anyone wanting to understand how real allocators work internally.

📚 Features
✔ Custom heap managed manually

The allocator uses a doubly linked list of block headers:

+------------+----------+-----------+
| header_t   | payload  | next block
+------------+----------+-----------+

✔ Supports:

my_malloc(size)

my_free(ptr)

my_calloc(num, size)

my_realloc(ptr, size)

mem_available()

print_heap()

✔ Coalescing of adjacent free blocks

Just like glibc, freelist blocks are merged to reduce fragmentation.

✔ Block splitting

A bigger free block can be split into:

[ allocated block ] [ free block ]

✔ Interactive REPL shell

You can type:

malloc 100
calloc 5 20
heap
realloc 0x1234 200
freebytes


Perfect for teaching or debugging allocator behavior.

✔ Library mode

You can link it into any C program as:

#include "vmalloc.h"
void *p = my_malloc(128);
my_free(p);

✔ pkg-config support

For clean integration into larger projects.

📁 Project Structure
memory-allocator/
│
├── vmalloc.c            # allocator implementation
├── vmalloc.h            # allocator API
├── vmalloc_shell.c      # interactive REPL shell
├── test.c               # example usage program
│
├── libvmalloc.a         # (generated) static library
├── libvmalloc.so        # (generated) shared library
│
├── Makefile             # build system
└── README.md            # this file

🛠 Build Instructions
1. Build the static library
gcc -c vmalloc.c -o vmalloc.o
ar rcs libvmalloc.a vmalloc.o


Produces:

libvmalloc.a

2. Build the shared library
gcc -shared -fPIC vmalloc.c -o libvmalloc.so

▶️ Build & Run the REPL Shell

This is an interactive UI for experimenting with the allocator.

gcc vmalloc_shell.c vmalloc.c -o vmalloc_shell
./vmalloc_shell

Available commands:
malloc <size>     
calloc <num> <size>
realloc <ptr> <size>
free <ptr>
heap
freebytes

Example REPL session:
malloc 256
Allocated 256 bytes at 0x616000401020

malloc 100
Allocated 100 bytes at 0x616000401130

heap
=== HEAP STATE ===
Block 0: header=0x616000401000, size=256, free=NO
Block 1: header=0x616000401110, size=100, free=NO
=================

freebytes
Free memory in allocator heap: 0 bytes

📘 Using vmalloc in Your C Programs
1. Include the header:
#include "vmalloc.h"

2. Example program:
#include "vmalloc.h"
#include <stdio.h>

int main() {
    void *a = my_malloc(128);
    void *b = my_calloc(5, 20);

    print_heap();

    my_free(a);
    my_free(b);

    print_heap();
    return 0;
}

3. Compile:

Option A — Link directly with source:

gcc test.c vmalloc.c -o testprog


Option B — Link with static library:

gcc test.c libvmalloc.a -o testprog


Run it:

./testprog

📦 pkg-config Support

Install the library and pkg-config file:

sudo make install


Then compile using:

gcc test.c $(pkg-config --cflags --libs vmalloc) -o testprog

🧩 Allocator Internals (Overview)
Each block contains:
typedef union header {
    struct {
        size_t size;
        unsigned is_free;
        header_t *next;
        header_t *prev;
    } s;
    ALIGN stub;  // 16-byte alignment
} header_t;

The heap is a doubly linked list:
head → [H][data] → [H][data] → … → tail

Allocation algorithm (first-fit)

Search for a free block large enough

If found:

split if necessary

mark allocated

Otherwise:

request memory via sbrk()

extend heap

Free algorithm

Mark block free

Coalesce with neighbors

If block is at heap end → return memory to OS with sbrk(-size)

📊 Example Heap State
=== HEAP STATE ===
Block 0: header=0x562b307dc000, size=200, free=NO
Block 1: header=0x562b307dc0e0, size=300, free=YES
Block 2: header=0x562b307dc220, size=100, free=NO
=================

🎯 Why This Project Matters

This allocator demonstrates:

Low-level memory manipulation

Manual heap management

Understanding fragmentation

Linked list architectures

Real-world malloc internals

Thread-safe design

It’s a perfect project to showcase on GitHub and Twitter/X for systems programming credibility.

🤝 Contributing

Pull requests are welcome!
If you'd like to contribute:

Add new fit strategies (best-fit, worst-fit)

Add color-coded heap visualization

Add valgrind-like diagnostics

Add fragmentation statistics

Add unit tests
