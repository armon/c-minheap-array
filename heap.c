/**
 * This file defines the methods declared in heap.h
 * These are used to create and manipulate a heap
 * data structure.
 */

#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <strings.h>
#include "heap.h"

// Helpful Macro's
#define LEFT_CHILD(i)   ((i<<1)+1)
#define RIGHT_CHILD(i)  ((i<<1)+2)
#define PARENT_ENTRY(i) ((i-1)>>1)
#define SWAP_ENTRIES(parent,child)  { \
                                      void* temp = parent->key; \
                                      parent->key = child->key;          \
                                      child->key = temp;                 \
                                      temp = parent->value;              \
                                      parent->value = child->value;      \
                                      child->value = temp;               \
                                    }

#define GET_ENTRY(index,map_table) (((heap_entry*)*(map_table+index/ENTRIES_PER_PAGE))+(index % ENTRIES_PER_PAGE))




/**
 * Stores the number of heap_entry structures
 * we can fit into a single page of memory.
 *
 * This is determined by the page size, so we
 * need to determine this at run time.
 */
static int ENTRIES_PER_PAGE = 0;

/**
 * Stores the number of bytes in a single
 * page of memory.
 */
static int PAGE_SIZE = 0;

// Helper function to map a number of pages into memory
// Returns NULL on error, otherwise returns a pointer to the
// first page.
static void* map_in_pages(int page_count) {
    // Check everything
    assert(page_count > 0);

    // Call mmmap to get the pages
    void* addr = mmap(NULL, page_count*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);

    if (addr == MAP_FAILED)
        return NULL;
    else {
        // Clear the memory
        bzero(addr,page_count*PAGE_SIZE);
        
        // Return the address 
        return addr;
    }
}


// Helper function to map a number of pages out of memory
static void map_out_pages(void* addr, int page_count) {
    // Check everything
    assert(addr != NULL);
    assert(page_count > 0);

    // Call munmap to get rid of the pages
    int result = munmap(addr, page_count*PAGE_SIZE);

    // The result should be 0
    assert(result == 0);
}


// This is a comparison function that treats keys as signed ints
int compare_int_keys(register void* key1, register void* key2) {
    // Cast them as int* and read them in
    register int key1_v = *((int*)key1);
    register int key2_v = *((int*)key2);

    // Perform the comparison
    if (key1_v < key2_v)
        return -1;
    else if (key1_v == key2_v)
        return 0;
    else
        return 1;
}


// Creates a new heap
void heap_create(heap* h, int initial_size, int (*comp_func)(void*,void*)) {
    // Check if we need to setup our globals
    if (PAGE_SIZE == 0) {
        // Get the page size
        PAGE_SIZE = getpagesize();

        // Calculate the max entries
        ENTRIES_PER_PAGE = PAGE_SIZE / sizeof(heap_entry);
    }
    
    // Check that initial size is greater than 0, else set it to ENTRIES_PER_PAGE
    if (initial_size <= 0)
        initial_size = ENTRIES_PER_PAGE;

    // If the comp_func is null, treat the keys as signed ints
    if (comp_func == NULL)
        comp_func = compare_int_keys;


    // Store the compare function
    h->compare_func = comp_func;

    // Set active entries to 0
    h->active_entries = 0;


    // Determine how many pages of entries we need
    h->minimum_pages = initial_size / ENTRIES_PER_PAGE + ((initial_size % ENTRIES_PER_PAGE > 0) ? 1 : 0);

    // Determine how big the map table should be
    h->map_pages = sizeof(void*) * h->minimum_pages / PAGE_SIZE + 1;

    // Allocate the map table
    h->mapping_table = (void**)map_in_pages(h->map_pages);
    assert(h->mapping_table != NULL);


    // Allocate the entry pages
    void* addr = map_in_pages(h->minimum_pages);
    assert(addr != NULL);

    // Add these to the map table
    for (int i=0;i<h->minimum_pages;i++) {
        *(h->mapping_table+i) = addr+(i*PAGE_SIZE);
    }

    // Set the allocated pages
    h->allocated_pages = h->minimum_pages;
}


// Cleanup a heap
void heap_destroy(heap* h) {
    // Check that h is not null
    assert(h != NULL);

    // Un-map all the entry pages
    void** map_table = h->mapping_table;
    assert(map_table != NULL);

    for (int i=0; i < h->allocated_pages; i++) {
        map_out_pages(*(map_table+i),1);
    }

    // Map out the map table
    map_out_pages(map_table, h->map_pages);

    // Clear everything
    h->active_entries = 0;
    h->allocated_pages = 0;
    h->map_pages = 0;
    h->mapping_table = NULL;
}


// Gets the size of the heap
int heap_size(heap* h) {
    // Return the active entries
    return h->active_entries;
}


/* In-line version is much faster
// Fetches the address of a heap_entry
static heap_entry* heap_get_entry(int index, void** map_table) {
    // Determine which page that index falls in
    int entry_page = index / ENTRIES_PER_PAGE;

    // Determine the offset into the page
    int page_offset = index % ENTRIES_PER_PAGE;

    // Get the address of the page
    heap_entry* page_address = (heap_entry*)*(map_table+entry_page);

    // Get the corrent entry
    return page_address+page_offset;
}
*/


// Gets the minimum element
int heap_min(heap* h, void** key, void** value) {
    // Check the number of elements, abort if 0
    if (h->active_entries == 0)
        return 0;

    // Get the 0th element
    heap_entry* root = GET_ENTRY(0, h->mapping_table);

    // Set the key and value
    *key = root->key;
    *value = root->value;

    // Success
    return 1;
}


// Insert a new element
void heap_insert(heap *h, void* key, void* value) {
    // Check if this heap is not destoyed
    assert(h->mapping_table != NULL);

    // Check if we have room
    int max_entries = h->allocated_pages * ENTRIES_PER_PAGE;
    if (h->active_entries + 1 > max_entries) {
        // Get the number of map pages
        int map_pages = h->map_pages;

        // We need a new page, do we have room?
        int mapable_pages = map_pages * PAGE_SIZE / sizeof(void*);
    
        // Check if we need to grow the map table
        if (h->allocated_pages + 1 > mapable_pages) {
            // Allocate a new table, slightly bigger
            void *new_table = map_in_pages(map_pages + 1);

            // Get the old table
            void *old_table = (void*)h->mapping_table;

            // Copy the old entries to the new table
            memcpy(new_table, old_table, map_pages * PAGE_SIZE);

            // Delete the old table
            map_out_pages(old_table, map_pages);

            // Swap to the new table
            h->mapping_table = (void**)new_table;

            // Update the number of map pages
            h->map_pages = map_pages + 1;
        }

        // Allocate a new page
        void* addr = map_in_pages(1);

        // Add this to the map
        *(h->mapping_table+h->allocated_pages) = addr;

        // Update the number of allocated pages
        h->allocated_pages++;
    }

    // Store the comparison function
    int (*cmp_func)(void*,void*) = h->compare_func;

    // Store the map table address
    void** map_table = h->mapping_table;

    // Get the current index
    int current_index = h->active_entries;
    heap_entry* current = GET_ENTRY(current_index, map_table);

    // Loop variables
    int parent_index;
    heap_entry *parent;

    // While we can, keep swapping with our parent
    while (current_index > 0) {
        // Get the parent index
        parent_index = PARENT_ENTRY(current_index);

        // Get the parent entry
        parent = GET_ENTRY(parent_index, map_table);
       
        // Compare the keys, and swap if we need to 
        if (cmp_func(key, parent->key) < 0) {
            // Move the parent down
            current->key = parent->key;
            current->value = parent->value;

            // Move our reference
            current_index = parent_index;
            current = parent;

        // We are done swapping
        }   else
            break;
    }

    // Insert at the current index
    current->key = key;
    current->value = value; 

    // Increase the number of active entries
    h->active_entries++;
}


// Deletes the minimum entry in the heap
int heap_delmin(heap* h, void** key, void** value) {
    // Check there is a minimum
    if (h->active_entries == 0)
        return 0;

    // Load in the map table
    void** map_table = h->mapping_table;

    // Get the root element
    int current_index = 0;
    heap_entry* current = GET_ENTRY(current_index, map_table);

    // Store the outputs
    *key = current->key;
    *value = current->value;

    // Reduce the number of active entries
    h->active_entries--;

    // Get the active entries
    int entries = h->active_entries;
   
    // If there are any other nodes, we may need to move them up
    if (h->active_entries > 0) {
        // Move the last element to the root
        heap_entry* last = GET_ENTRY(entries,map_table);
        current->key = last->key;
        current->value = last->value;

        // Loop variables
        heap_entry* left_child;
        heap_entry* right_child;

        // Load the comparison function
        int (*cmp_func)(void*,void*) = h->compare_func;

        // Store the left index
        int left_child_index;

        while (left_child_index = LEFT_CHILD(current_index), left_child_index < entries) {
            // Load the left child
            left_child = GET_ENTRY(left_child_index, map_table);

            // We have a left + right child
            if (left_child_index+1 < entries) {
                // Load the right child
                right_child = GET_ENTRY((left_child_index+1), map_table);

                // Find the smaller child
                if (cmp_func(left_child->key, right_child->key) <= 0) {

                    // Swap with the left if it is smaller
                    if (cmp_func(current->key, left_child->key) == 1) {
                        SWAP_ENTRIES(current,left_child);
                        current_index = left_child_index;
                        current = left_child;

                    // Otherwise, the current is smaller
                    } else
                        break;

                // Right child is smaller
                } else {

                    // Swap with the right if it is smaller
                    if (cmp_func(current->key, right_child->key) == 1) {
                        SWAP_ENTRIES(current,right_child);
                        current_index = left_child_index+1;
                        current = right_child;

                    // Current is smaller
                    } else
                        break;

                }


            // We only have a left child, only do something if the left is smaller
            } else if (cmp_func(current->key, left_child->key) == 1) {
                SWAP_ENTRIES(current,left_child);
                current_index = left_child_index;
                current = left_child;

            // Done otherwise
            }  else
                break;

        }
    } 

    // Check if we should release a page of memory
    int used_pages = entries / ENTRIES_PER_PAGE + ((entries % ENTRIES_PER_PAGE > 0) ? 1 : 0);

    // Allow one empty page, but not two
    if (h->allocated_pages > used_pages + 1 && h->allocated_pages > h->minimum_pages) {
        // Get the address of the page to delete
        void* addr = *(map_table+h->allocated_pages-1);

        // Map out
        map_out_pages(addr, 1);

        // Decrement the allocated count
        h->allocated_pages--;
    }

    // Success
    return 1;
}


// Allows a user to iterate over all entries, e.g. to free() the memory
void heap_foreach(heap* h, void (*func)(void*,void*)) {
    // Store the current index and max index
    int index = 0;
    int entries = h->active_entries;

    heap_entry* entry;
    void** map_table = h->mapping_table;

    for (;index<entries;index++) {
        // Get the entry
        entry = GET_ENTRY(index,map_table);

        // Call the user function
        func(entry->key, entry->value);
    }
}


