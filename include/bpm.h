#pragma once

#define _GNU_SOURCE
#include "page.h"
#include <search.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct BufferPoolManagerPage {
    uint32_t id;
    void *data[PAGE_SIZE];
    int pin_count;
    bool is_dirty;
} BpmPage;

typedef struct BufferPoolManager {
    size_t pool_size; // number of frames in the buffer pool
    BpmPage *pages;   // array of pages in the buffer pool
    struct hsearch_data
        *page_table; // map pages in the buffer pool to its frames
    bool *free_list; // list of available frames
} BufferPoolManager;

/*
 * Initiates a new buffer pool manager and returns a pointer to it
 */
BufferPoolManager *new_bpm(size_t pool_size);

/**
 * Unpin page of page_id from the buffer pool and returns true. If the provided
 * page does not exist, return false. Sets page's dirty bit to value provided in
 * is_dirty
 */
bool unpin_page(uint32_t page_id, bool is_dirty, BufferPoolManager *bpm);

/**
 * Flush provided page to disk, regardless of its dirty bit
 * Unsets the provided page's dirty bit afterwards
 * Returns false if the page could not be found in the page table, true
 * otherwise
 */
bool flush_page(uint32_t page_id);

/**
 * Flush all pages in the buffer pool to disk
 */
void flush_all_pages();

/*
 * Removes a page from the buffer pool and returns true.
 * If the page with provided page_id is pinned, returns false.
 */
bool delete_bpm_page(uint32_t page_id);

/*
 * Creates a new page of the id PAGE_ID in the buffer pool BPM and returns a
 * pointer to it. If no frame is available or evictable, returns a null pointer.
 * Writes a possible in replacement frame back to disk if it contains a dirty
 * page
 */
BpmPage *new_bpm_page(BufferPoolManager *bpm, uint32_t page_id);

/*
 * Returns the requested page from the buffer pool, or returns a null pointer if
 * page needs to be fetched from disk but no frames are available or evictable.
 * Writes a possible replacement frame back to disk if it contains a dirty page
 */
BpmPage *fetch_bpm_page(uint32_t page_id);
