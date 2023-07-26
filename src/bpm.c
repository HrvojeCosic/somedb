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

BufferPoolManager *new_bpm(const size_t pool_size, DiskManager *disk_manager) {
    BpmPage *pages = (BpmPage *)calloc(sizeof(BpmPage), pool_size);
    bool *free_list = (bool *)malloc(sizeof(bool) * pool_size);

    for (size_t i = 0; i < pool_size; i++) {
        free_list[i] = true;
    }

    BufferPoolManager *bpm = (BufferPoolManager *)malloc(sizeof(BufferPoolManager));
    bpm->pool_size = pool_size;
    bpm->pages = pages;
    bpm->free_list = free_list;
    bpm->page_table = init_hash(pool_size);
    bpm->replacer = *clock_replacer_init(pool_size);
    bpm->disk_manager = disk_manager;

    return bpm;
}

static void add_to_pagetable(page_id_t key, frame_id_t *val, BufferPoolManager *bpm) {
    char *key_str = (char *)malloc(sizeof(char) * 5);
    sprintf(key_str, "%u", key);

    HashInsertArgs in_args = {.key = key_str, .data = val, .ht = bpm->page_table};
    hash_insert(&in_args);
}

BpmPage *new_bpm_page(BufferPoolManager *bpm, page_id_t pid) {
    // Return early if already exists
    char pid_str[11];
    sprintf(pid_str, "%d", pid);
    HashEl *found = hash_find(pid_str, bpm->page_table);
    if (found) {
        frame_id_t fidx = (*(frame_id_t *)found->data);
        return bpm->pages + fidx;
    }

    // Find free frame id
    frame_id_t *fid = (frame_id_t *)malloc(sizeof(frame_id_t));
    *fid = UINT32_MAX;
    for (size_t i = 0; i < bpm->pool_size; i++) {
        if (bpm->free_list[i] == true) {
            *fid = i;
            bpm->free_list[i] = false;
            break;
        }
    }
    if (*fid == UINT32_MAX)
        *fid = evict(&bpm->replacer);
    if (*fid == UINT32_MAX)
        return NULL;
    else if (bpm->pages[*fid].is_dirty)
        flush_page(*fid, bpm);

    BpmPage page = {.id = *fid, .data = NULL, .pin_count = 1, .is_dirty = false};
    bpm->pages[*fid] = page;

    add_to_pagetable(pid, fid, bpm);
    clock_replacer_pin(fid, &bpm->replacer);

    return bpm->pages + *fid;
}

bool unpin_page(page_id_t page_id, bool is_dirty, BufferPoolManager *bpm) {
    char pid_str[15];
    sprintf(pid_str, "%d", page_id);

    HashEl *el = hash_find(pid_str, bpm->page_table);

    if (el == NULL)
        return false;

    frame_id_t frame_idx = *(frame_id_t *)(el->data);
    BpmPage *page = bpm->pages + frame_idx;

    if (page->pin_count == 0)
        return false;

    page->is_dirty = is_dirty;
    page->pin_count--;
    if (page->pin_count == 0)
        clock_replacer_unpin((frame_id_t *)el->data, &bpm->replacer);

    return true;
}

bool flush_page(page_id_t page_id, BufferPoolManager *bpm) {
    char pid_str[11];
    sprintf(pid_str, "%d", page_id);
    HashEl *entry = hash_find(pid_str, bpm->page_table);
    if (!entry)
        return false;
    frame_id_t *fid = (frame_id_t *)entry->data;

    write_page(page_id, bpm->disk_manager, bpm->pages[*fid].data);

    HashRemoveArgs rm_args = {.key = pid_str, .ht = bpm->page_table, .success_out = NULL};
    hash_remove(&rm_args);

    bpm->free_list[*fid] = true;
    clock_replacer_unpin(fid, &bpm->replacer);

    return true;
}

BpmPage *fetch_bpm_page(page_id_t page_id, BufferPoolManager *bpm) {
    char pid_str[11];
    sprintf(pid_str, "%d", page_id);
    HashEl *entry = hash_find(pid_str, bpm->page_table);
    frame_id_t fid = 0;

    if (entry) {
        fid = *(frame_id_t *)entry->data;
        bpm->pages[fid].pin_count++;
        return bpm->pages + fid;
    }

    if (BpmPage *bpm_page = new_bpm_page(bpm, page_id)) {
        bpm->pages[fid].pin_count++;
        return bpm_page;
    } else {
        return NULL;
    }
}
