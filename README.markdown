# README

This is a very simple library for implementing a Min-Heap or Priority Queue
in plain C. It should be very understandable, and is a useful reference for
people who are learning C or want to understand the binary heap data structure.

This implementation makes use of a plain array to store the table. It supports
resizing (growing and shrinking) and does this by simply doubling or halving the
array size when appropriate. This was make as a counter part to a different project,
[c-minheap-indirect](https://github.com/armon/c-minheap-indirect) which utilizes
an indirection table to increase memory efficiency. This is an important distinction
when the number of elements is very large, as this implementation will have large amounts
of wasted space, and relies on expensive doubling operations, while the indirect
implementation has very cheap resizing operations and wastes less than a single
page worth of memory.

# Using the library

To use the library you only need to include the heap.h header, and
link against the heap.c file. There is nothing fancy required.

# Testing the library

Included in the project is a file main.c which serves as a simple test file.
We randomly generate a large number of keys (variable, default to 10M), and
insert them into our heap. We then extract the keys and verify they are ordered.
To test this, just run `make` and then run the test program.


