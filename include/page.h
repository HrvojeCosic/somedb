#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096

/*
 * Pointer to the page memory after it's header
 */
#define PAGE_NO_HEADER(page) page + sizeof(Header)

/*
 * Pointer to the start of tuple of index idx
 */
#define TUPLE_INDEX_TO_POINTER_OFFSET(idx)                                     \
    sizeof(Header) + (idx * sizeof(TuplePtr))

/*
 * Page header flags
 */
#define COMPACTABLE (1 << 7)

typedef struct Header {
    uint32_t id;
    uint16_t free_start; // offset to beginning of available page memory
    uint16_t free_end;   // offset to end of available page memory
    uint16_t free_total; // total available page memory
    uint8_t flags;
} Header;

typedef struct TuplePtr {
    uint16_t start_offset; // offset to the tuple start
    uint16_t size;         // size of the tuple
} TuplePtr;

typedef struct TuplePtrList {
    TuplePtr *start; // pointer to the beginning of tuple pointers in a page
    uint16_t length; // number of tuple pointers in a page
} TuplePtrList;

#define PAGE_HEADER(page) (Header *)page

#define TUPLE_PTR(position) (TuplePtr *)position

/*
 * Allocates memory for a new page of the given id and returns a pointer to it's
 * beginning
 */
void *new_page(uint32_t id);

/*
 * Returns a pointer to the beginning of the tuple with the given index at the
 * specified page, or a null pointer if the tuple does not exist in the given
 * page
 */
void *get_tuple(void *page, uint16_t tuple_index);

/*
 * Removes tuple of the specified page at the specified index
 */
void remove_tuple(void *page, uint16_t tuple_idx);

/*
 * Defragments the page by freeing up the memory of removed tuple components.
 * Unsets the COMPACTABLE flag in the header afterwards.
 * Does nothing if the page is not compactable
 */
void defragment(void *page);

/*
 * Writes contents of the given page to file with the specified file descriptor
 * fd
 */
void write_page(void *page, uint32_t fd);

/*
 * Reads contents of the page of specified page_id belonging to a file of
 * specified file descriptor fd into memory and returns a pointer to its
 * beginning
 */
void *read_page(uint32_t page_id, uint32_t fd);

/*
 * Adds given tuple to the given page and returns a pointer to beginning of
 * added tuple. If the page is full, immediately returns a null pointer
 */
TuplePtr *add_tuple(void *page, void *tuple, uint16_t tuple_size);

/*
 * Returns information about tuple pointers at the specified page contained in
 * TuplePtrList
 */
TuplePtrList get_tuple_ptr_list(void *page);
