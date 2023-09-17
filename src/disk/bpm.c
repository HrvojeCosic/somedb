#include "../../include/disk/bpm.h"
#include "../../include/disk/disk_manager.h"
#include "../../include/disk/heapfile.h"
#include "../../include/utils/hash.h"
#include "../../include/utils/serialize.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

BufferPoolManager *bpm(const size_t pool_size) {
    if (bpm_.initialized == true)
        return &bpm_;

    BpmPage *pages = (BpmPage *)calloc(sizeof(BpmPage), pool_size);
    bool *free_list = (bool *)malloc(sizeof(bool) * pool_size);

    for (size_t i = 0; i < pool_size; i++) {
        free_list[i] = true;
    }

    bpm_.pool_size = pool_size;
    bpm_.pages = pages;
    bpm_.free_list = free_list;
    bpm_.page_table = init_hash(pool_size);
    bpm_.replacer = *clock_replacer_init(pool_size);
    bpm_.initialized = true;

    return &bpm_;
}

static void add_to_pagetable(PTKey key, frame_id_t *val) {
    char *key_str = ptkey_to_str(key);

    HashInsertArgs in_args = {.key = key_str, .data = val, .ht = bpm()->page_table};
    hash_insert(&in_args);
}

/*
 * Helper function for creating a page in the buffer pool.
 * If no frame is available or evictable, returns a null pointer.
 */
static BpmPage *new_bpm_page(PTKey ptkey) {
    // Return early if already exists
    HashEl *found = hash_find(ptkey_to_str(ptkey), bpm()->page_table);
    if (found) {
        frame_id_t fidx = (*(frame_id_t *)found->data);
        return bpm()->pages + fidx;
    }

    // Find free frame id
    frame_id_t *fid = (frame_id_t *)malloc(sizeof(frame_id_t));
    *fid = UINT32_MAX;
    for (size_t i = 0; i < bpm()->pool_size; i++) {
        if (bpm()->free_list[i] == true) {
            *fid = i;
            bpm()->free_list[i] = false;
            break;
        }
    }
    if (*fid == UINT32_MAX) {
        *fid = evict(&bpm()->replacer);
        if (*fid == UINT32_MAX)
            return NULL;
        else if (bpm()->pages[*fid].is_dirty) {
            ptkey.pid = bpm()->pages[*fid].id;
            flush_page(ptkey);
        }
    }

    BpmPage page;
    page.id = ptkey.pid;
    page.pin_count = 1;
    page.is_dirty = false;
    memset(page.data, 0, PAGE_SIZE);
    bpm()->pages[*fid] = page;

    add_to_pagetable(ptkey, fid);
    clock_replacer_pin(fid, &bpm()->replacer);

    return bpm()->pages + *fid;
}

BpmPage *allocate_new_page(DiskManager *disk_manager, PageType type) {
    page_id_t pid;
    BpmPage *bpm_page = NULL;

    // Write page out to disk in appropriate format without populating the buffer pool for now
    switch (type) {
    case HEAP_PAGE:
        pid = new_heap_page(disk_manager);
        break;
    case BTREE_INDEX_PAGE:
        pid = new_btree_index_page(disk_manager, false);
        break;
    case INVALID:
        assert(type != INVALID); // "throw" error
        break;
    }

    while (bpm_page == NULL)
        bpm_page = new_bpm_page(new_ptkey(disk_manager->table_name, pid));

    return bpm_page;
}

bool unpin_page(PTKey pt, bool is_dirty) {
    HashEl *el = hash_find(ptkey_to_str(pt), bpm()->page_table);

    if (el == NULL)
        return false;

    frame_id_t frame_idx = *(frame_id_t *)(el->data);
    BpmPage *page = bpm()->pages + frame_idx;

    if (page->pin_count == 0)
        return false;

    page->is_dirty = is_dirty;
    page->pin_count--;
    if (page->pin_count == 0)
        clock_replacer_unpin((frame_id_t *)el->data, &bpm()->replacer);

    return true;
}

void write_to_frame(frame_id_t fid, u8 *data) {
    memcpy(bpm()->pages[fid].data, data, PAGE_SIZE);
    bpm()->pages[fid].is_dirty = true;
}

bool flush_page(PTKey ptkey) {
    char *pt_str = ptkey_to_str(ptkey);
    HashEl *entry = hash_find(pt_str, bpm()->page_table);
    if (!entry)
        return false;
    frame_id_t *fid = (frame_id_t *)entry->data;

    write_page(ptkey, bpm()->pages[*fid].data);

    HashRemoveArgs rm_args = {.key = pt_str, .ht = bpm()->page_table, .success_out = NULL};
    hash_remove(&rm_args);

    bpm()->free_list[*fid] = true;
    clock_replacer_unpin(fid, &bpm()->replacer);

    return true;
}

BpmPage *fetch_bpm_page(PTKey pt) {
    char pid_str[11];
    sprintf(pid_str, "%d", pt.pid);
    HashEl *entry = hash_find(pid_str, bpm()->page_table);
    frame_id_t fid = 0;

    if (entry) {
        fid = *(frame_id_t *)entry->data;
        bpm()->pages[fid].pin_count++;
        return bpm()->pages + fid;
    } else {
        BpmPage *newp = new_bpm_page(pt); // TODO: handle NULL return (aka no space)
        fid = *(frame_id_t *)hash_find(pid_str, bpm()->page_table)->data;
        bpm()->pages[fid].pin_count++;
        memcpy(newp->data, read_page(pt), PAGE_SIZE);
        return newp;
    }
}
