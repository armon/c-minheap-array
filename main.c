
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "heap.h"


int index_counter = 0;

void verify_order(void* key, void* value) {
    // Get the key
    int val = *(int*)key;

    printf("%d - Key: %d\n",index_counter,val);
    index_counter++;
}

int main(int argc, char** argv) {

    // Create the heap
    heap h;
    heap_create(&h,0,NULL);

    // Maximum
    int count = 100000000;

    // Allocate a key and value
    int* key = (int*)malloc(count*sizeof(int));
    char* value = "The meaning of life.";

    // Initialize the first key
    unsigned int val = 123;
    srand( val);
    printf("Seed %d\n",val);

    // Store that as the minimum
    int min = INT_MAX;

    // Use a pseudo-random generator for the other keys
    for (int i=0;i<count;i++) {
        *(key+i) = rand();

        // Check for a new min
        if (*(key+i) < min)
            min = *(key+i);

        // Insert into the heap
        heap_insert(&h, key+i, value);
    }



    // Get the minimum
    int* min_key;
    char* min_val;
    int *prev_key = NULL;

    // Show the real minimum 
    printf("Real min: %d\n", min);

    // Try to get the minimum
    while (heap_delmin(&h, &min_key, &min_val)) {
        // Dump
        //index_counter = 0;
        //heap_foreach(&h, verify_order);

        // printf("Key: %d Val: %s\n",*min_key,min_val);

        if (prev_key != NULL && *prev_key > *min_key) {
            printf("Previous key is greater than current key!\n");
        }
        prev_key = min_key;
    }

    // Clean up the heap
    heap_destroy(&h);
}

