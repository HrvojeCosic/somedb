#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096

#define TUPLE_INDEX_TO_POINTER_OFFSET(idx) \
    sizeof(Header) + (idx * sizeof(TuplePtr));

// Header flags
#define COMPACTABLE (1 << 7)

typedef struct Header {
    uint32_t id;
    uint16_t free_start;
    uint16_t free_end;
    uint16_t free_total;
    uint8_t flags;
} Header;

typedef struct TuplePtr {
    uint16_t start_offset;
    uint16_t size;
} TuplePtr;

typedef struct TuplePtrList {
    TuplePtr* start;
    uint16_t length;
} TuplePtrList;

#define PAGE_HEADER(page) \
    (Header*)page

#define TUPLE_PTR(position) \
    (TuplePtr*)position


void* new_page(uint32_t id);
void* get_tuple(void* page, uint16_t tuple_index);
void add_tuple(void* page, void* tuple, uint16_t tuple_size);
void remove_tuple(void* page, uint16_t tuple_idx);
void defragment(void* page);
void write_page(void* page, uint32_t fd);
void* read_page(uint32_t page_id, uint32_t fd);
