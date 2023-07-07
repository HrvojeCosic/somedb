#include "../include/disk_manager.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DBFILE "dbfile.txt" // name of database file

int db_file() {
    int fd = open(DBFILE, O_CREAT | O_RDWR, 0644);
    return fd;
}

void shut_down_db() { close(db_file()); }

void *new_page(page_id_t id) {
    void *page = malloc(PAGE_SIZE);
    RWLOCK latch;
    RWLOCK_INIT(&latch);

    Header *header = PAGE_HEADER(page);
    header->id = id;
    header->free_end = PAGE_SIZE - 1;
    header->free_start = sizeof(Header);
    header->free_total = header->free_end - header->free_start;
    header->latch = latch;

    return page;
}

TuplePtr *add_tuple(void *data) {
    AddTupleArgs *data_args = (AddTupleArgs *)data;
    void *page = data_args->page;
    uint16_t tuple_size = data_args->tuple_size;
    void *tuple = data_args->tuple;

    Header *header = PAGE_HEADER(page);
    RWLOCK_WRLOCK(&header->latch);
    if (header->free_total < tuple_size + sizeof(TuplePtr)) {
        printf("Page is full, tuple insertion failed.\n");
        return NULL;
    }

    TuplePtr *tuple_ptr = (TuplePtr *)((uintptr_t *)page + header->free_start);
    tuple_ptr->start_offset = header->free_end - tuple_size;
    tuple_ptr->size = tuple_size;

    void *tuple_start = (uintptr_t *)page + tuple_ptr->start_offset;
    memcpy(tuple_start, tuple, tuple_size);

    header->free_start += sizeof(TuplePtr);
    header->free_end -= tuple_ptr->size;
    header->free_total = header->free_end - header->free_start;

    RWLOCK_UNLOCK(&header->latch);
    return tuple_ptr;
}

void remove_tuple(void *page, uint16_t tuple_idx) {
    TuplePtr *tuple_ptr =
        (TuplePtr *)((uintptr_t *)page +
                     TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx));
    Header *page_header = PAGE_HEADER(page);
    page_header->flags |= COMPACTABLE;

    tuple_ptr->start_offset = 0;
}

void *get_tuple(void *page, uint16_t tuple_idx) {
    TuplePtr *tuple_ptr =
        (TuplePtr *)((uintptr_t *)page +
                     TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx));

    if (tuple_ptr->start_offset == 0) {
        return NULL;
    }

    return (uintptr_t *)page + tuple_ptr->start_offset;
}

TuplePtrList get_tuple_ptr_list(void *page) {
    Header *header = PAGE_HEADER(page);
    TuplePtrList list;
    list.start = (TuplePtr *)((uintptr_t *)page + sizeof(Header));
    list.length = (header->free_start - sizeof(Header)) / sizeof(TuplePtr);
    return list;
}

void defragment(void *page) {
    Header *page_header = PAGE_HEADER(page);

    if ((page_header->flags & COMPACTABLE) == 0) {
        return;
    }

    void *temp = new_page(1);
    Header *temp_header = PAGE_HEADER(temp);

    TuplePtrList tuple_ptr_list = get_tuple_ptr_list(page);

    TuplePtr *curr_tuple_ptr;
    for (int i = 0; i < tuple_ptr_list.length; i++) {
        curr_tuple_ptr = tuple_ptr_list.start + i;

        // Removed tuples' offsets are 0
        if (curr_tuple_ptr->start_offset != 0) {
            AddTupleArgs t_args = {.page = temp,
                                   .tuple = (uintptr_t *)page +
                                            curr_tuple_ptr->start_offset,
                                   .tuple_size = curr_tuple_ptr->size};
            add_tuple(&t_args);
        } else {
        }
    }
    page_header->free_start = temp_header->free_start;
    page_header->free_end = temp_header->free_end;
    page_header->free_total = temp_header->free_total;
    page_header->flags &= ~COMPACTABLE;

    memcpy((uintptr_t *)PAGE_NO_HEADER(page), (uintptr_t *)PAGE_NO_HEADER(temp),
           PAGE_SIZE - sizeof(Header));
    free(temp);
}

void write_page(void *page) {
    Header *page_header = PAGE_HEADER(page);

    int fd = db_file();
    int s = lseek(fd, page_header->id * PAGE_SIZE, SEEK_SET);
    int w = write(fd, page, PAGE_SIZE);
    int f = fsync(fd);

    if (s == -1 || w == -1 || f == -1) {
        printf("I/O error while writing page\n");
    }
}

void *read_page(page_id_t page_id) {
    void *page = malloc(PAGE_SIZE);

    int fd = db_file();

    int offset = page_id * PAGE_SIZE;
    off_t fsize = lseek(fd, 0, SEEK_END);
    if (offset >= fsize) {
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
