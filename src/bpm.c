#include "../include/bpm.h"
#include "../include/disk_manager.h"
#include <fcntl.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BufferPoolManager *new_bpm(const size_t pool_size) {
    BpmPage pages[pool_size];
    bool free_list[pool_size];

    for (size_t i = 0; i < pool_size; i++) {
        free_list[i] = true;
    }

    BufferPoolManager *bpm = malloc(sizeof(BufferPoolManager));
    bpm->pool_size = pool_size;
    bpm->pages = pages;
    bpm->free_list = free_list;

    bpm->page_table = calloc(sizeof(struct hsearch_data), 1);
    hcreate_r(pool_size, bpm->page_table);

    return bpm;
}

BpmPage *new_bpm_page(BufferPoolManager *bpm, page_id_t page_id) {
    frame_id_t fid = 0;
    for (size_t i = 0; i < bpm->pool_size; i++) {
        if (bpm->pages[i].id == 0) {
            fid = bpm->pages[i].id + 1;
            break;
        }
    }

    int fd = db_file();

    BpmPage *page = malloc(sizeof(BpmPage));
    page->id = fid;
    page->is_dirty = false;
    page->pin_count = 1;
    memcpy(page->data, read_page(page_id, fd), PAGE_SIZE);

    return page;
}

bool unpin_page(page_id_t page_id, bool is_dirty, BufferPoolManager *bpm) {
    char *key = malloc(sizeof(char));
    sprintf(key, "%d", page_id);
    ENTRY e = {.key = key}, *res;

    uint8_t found = hsearch_r(e, FIND, &res, bpm->page_table);
    if (found == 0)
        return false;

    size_t frame_idx = (size_t)res->data;
    BpmPage *page = bpm->pages + frame_idx;
    page->pin_count = 0;
    page->is_dirty = is_dirty;

    free(key);
    return true;
}
