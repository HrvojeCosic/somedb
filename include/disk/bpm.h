#pragma once

#include "../utils/hash.h"
#include "../utils/shared.h"
#include "clock_replacer.h"
#include "disk_manager.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t data[PAGE_SIZE];
    page_id_t id;  // (p)id of page on disk (not frame)
    int pin_count; // number of threads using this bpm page
    bool is_dirty; // shows if the page has been modified after being read from
                   // disk
} BpmPage;

typedef struct {
    size_t pool_size;       // number of frames in the buffer pool
    BpmPage *pages;         // array of pages in the buffer pool
    HashTable *page_table;  // map RIDs (stringified) in the buffer pool to its frames
    bool *free_list;        // array of frame statuses (true=free/false=taken)
    ClockReplacer replacer; // finding unpinned frames to replace
    bool initialized;       // has a BufferPoolManager instance been initialized?
} BufferPoolManager;

/*
 * Initiates a new buffer pool manager and returns a pointer to it,
 * or returns a pointer to an already existing instance
 */
static BufferPoolManager bpm_ = {.pool_size = 0,
                                 .pages = NULL,
                                 .page_table = NULL,
                                 .free_list = NULL,
                                 .replacer = *clock_replacer_init(0),
                                 .initialized = false};
BufferPoolManager *bpm(size_t pool_size = 3);

/**
 * Unpins provided page from the buffer pool and returns true. If the
 * page does not exist or it's pin count is already 0, returns false. Sets
 * page's dirty bit to value provided in is_dirty
 */
bool unpin_page(PTKey ptkey, bool is_dirty);

/**
 * Flush provided page to disk, regardless of its dirty bit
 * Unsets the provided page's dirty bit afterwards
 * Returns false if the page could not be found in the page table, true
 * otherwise
 */
bool flush_page(PTKey ptkey);

/*
 * Allocates a new page of suitable TYPE on disk, places it in buffer pool BPM and returns a pointer to it.
 * Writes a possible replacement frame back to disk if it contains a dirty page.
 */
BpmPage *allocate_new_page(DiskManager *disk_manager, PageType type);

/*
 * Returns the requested page from the buffer pool, or returns a null pointer if
 * page needs to be fetched from disk but no frames are available or evictable.
 * Writes a possible replacement frame back to disk if it contains a dirty page
 */
BpmPage *fetch_bpm_page(PTKey pt);

/*
 * Writes provided data to a page contained in the provided frame id and marks it as dirty.
 * This function does NOT write anything to disk
 */
void write_to_frame(frame_id_t fid, u8 *data);
