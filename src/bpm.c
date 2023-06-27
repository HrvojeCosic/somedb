#include "../include/bpm.h"
#include "../include/disk_manager.h"
#include "../include/hash.h"
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BufferPoolManager *new_bpm(const size_t pool_size) {
    BpmPage *pages = calloc(sizeof(BpmPage), pool_size);
    bool *free_list = malloc(sizeof(bool) * pool_size);

    for (size_t i = 0; i < pool_size; i++) {
        free_list[i] = true;
    }

    BufferPoolManager *bpm = malloc(sizeof(BufferPoolManager));
    bpm->pool_size = pool_size;
    bpm->pages = pages;
    bpm->free_list = free_list;
    bpm->page_table = init_hash(pool_size);

    return bpm;
}

static void add_to_pagetable(frame_id_t key, page_id_t val,
                             BufferPoolManager *bpm) {
    char *key_str = malloc(sizeof(char) * 5);
    char *val_str = malloc(sizeof(char) * 5);
    sprintf(key_str, "%d", key);
    sprintf(val_str, "%d", val);

    hash_insert(key_str, val_str, bpm->page_table);
}

BpmPage *new_bpm_page(BufferPoolManager *bpm, page_id_t pid) {
    /*
     * TODO: REPLACEMENT POLICY INTEGRATION
     */

    // TODO: RETURN EARLY IF EXISTS ALREADY
    //    char pid_str[15];
    //    sprintf(pid_str, "%d", pid);
    //    HashEl *found = hash_find(pid_str, bpm->page_table);
    //    if (strcmp(pid_str, found->key) == 0) {
    //	BpmPage page = {0};
    //	return page;
    //    }

    frame_id_t fid = 0;
    for (size_t i = 0; i < bpm->pool_size; i++) {
        if (bpm->pages[i].id == 0) {
            fid = bpm->pages[i].id + 1;
            break;
        }
    }
    if (fid == 0)
        return NULL;

    BpmPage *page = malloc(sizeof(BpmPage));
    page->id = fid;
    page->is_dirty = false;
    page->pin_count = 1;
    memcpy(page->data, read_page(pid), PAGE_SIZE);

    add_to_pagetable(fid, pid, bpm);

    return page;
}

bool unpin_page(page_id_t page_id, bool is_dirty, BufferPoolManager *bpm) {
    char pid_str[15];
    sprintf(pid_str, "%d", page_id);

    HashEl *el = hash_find(pid_str, bpm->page_table);

    if (el == NULL)
        return false;

    size_t frame_idx = *(size_t *)(el->data);
    BpmPage *page = bpm->pages + frame_idx;
    page->pin_count = 0;
    page->is_dirty = is_dirty;

    return true;
}
