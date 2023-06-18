#include "../include/page.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void *new_page(uint32_t id) {
    void *page = malloc(PAGE_SIZE);

    Header *header = PAGE_HEADER(page);
    header->id = id;
    header->free_end = PAGE_SIZE - 1;
    header->free_start = sizeof(Header);
    header->free_total = header->free_end - header->free_start;

    return page;
}

TuplePtr *add_tuple(void *page, void *tuple, uint16_t tuple_size) {
    Header *header = PAGE_HEADER(page);
    if (header->free_total < tuple_size + sizeof(TuplePtr)) {
	printf("Page is full, tuple insertion failed.\n");
	return NULL;
    }

    TuplePtr *tuple_ptr = page + header->free_start;
    tuple_ptr->start_offset = header->free_end - tuple_size;
    tuple_ptr->size = tuple_size;

    void *tuple_start = page + tuple_ptr->start_offset;
    memcpy(tuple_start, tuple, tuple_size);

    header->free_start += sizeof(TuplePtr);
    header->free_end -= tuple_ptr->size;
    header->free_total = header->free_end - header->free_start;

    return tuple_ptr;
}

void remove_tuple(void *page, uint16_t tuple_idx) {
    TuplePtr *tuple_ptr = page + TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx);
    Header *page_header = PAGE_HEADER(page);
    page_header->flags |= COMPACTABLE;

    tuple_ptr->start_offset = 0;
}

void *get_tuple(void *page, uint16_t tuple_idx) {
    TuplePtr *tuple_ptr = page + TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx);

    if (tuple_ptr->start_offset == 0) {
	return NULL;
    }

    return page + tuple_ptr->start_offset;
}

TuplePtrList get_tuple_ptr_list(void *page) {
    Header *header = PAGE_HEADER(page);
    TuplePtrList list;
    list.start = page + sizeof(Header);
    list.length = (header->free_start - sizeof(Header)) / sizeof(TuplePtr);
    return list;
}

void defragment(void *page) {
    Header *page_header = PAGE_HEADER(page);

    if ((page_header->flags & COMPACTABLE) == 0) {
	return;
    }

    Header *temp = PAGE_HEADER(new_page(1));
    TuplePtrList tuple_ptr_list = get_tuple_ptr_list(page);


    TuplePtr *curr_tuple_ptr;
    for (int i = 0; i < tuple_ptr_list.length; i++) {
	curr_tuple_ptr = tuple_ptr_list.start + i;

	// Removed tuples' offsets are 0
	if (curr_tuple_ptr->start_offset != 0) {
	    add_tuple(temp, page + curr_tuple_ptr->start_offset, curr_tuple_ptr->size);
	}
    }

    page_header->free_start = temp->free_start;
    page_header->free_end = temp->free_end;
    page_header->free_total = temp->free_total;
    page_header->flags &= ~COMPACTABLE;

    memcpy(page + sizeof(Header), temp + sizeof(Header), PAGE_SIZE - sizeof(Header));
    free(temp);
}

void write_page(void *page, uint32_t fd) {
    Header *page_header = PAGE_HEADER(page);

    int s = lseek(fd, page_header->id * PAGE_SIZE, SEEK_SET);
    int w = write(fd, page, PAGE_SIZE);
    int f = fsync(fd);

    if (s == -1 || w == -1 || f == -1) {
	printf("I/O error while writing page\n");
    }
}

void *read_page(uint32_t page_id, uint32_t fd) {
    void *page = malloc(PAGE_SIZE);

    int offset = page_id * PAGE_SIZE;
    if (offset > PAGE_SIZE) {
	printf("I/O error, reading past EOF\n");
	exit(1);
    }

    int s = lseek(fd, offset, SEEK_SET);
    int r = read(fd, page, PAGE_SIZE);

    if (s == -1 || r == -1) {
	printf("I/O error while reading specified page\n");
	exit(1);
    }

    if (r < PAGE_SIZE) {
	printf("Read less than a page size\n");
    }

    return page;
}
