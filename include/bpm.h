#pragma once

#include "clock_replacer.h"
#include "disk_manager.h"
#include "hash.h"
#include "shared.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    page_id_t id;
    uint8_t data[PAGE_SIZE];
    int pin_count; // number of threads using this bpm page
    bool is_dirty; // shows if the page has been modified after being read from
                   // disk
} BpmPage;

typedef struct {
    size_t pool_size;       // number of frames in the buffer pool
    BpmPage *pages;         // array of pages in the buffer pool
    HashTable *page_table;  // map pages in the buffer pool to its frames
    bool *free_list;        // array of frame statuses (true=free/false=taken)
    ClockReplacer replacer; // finding unpinned frames to replace
    DiskManager *disk_manager;
} BufferPoolManager;

/*
 * Initiates a new buffer pool manager for a specified disk manager and returns a pointer to it
 */
BufferPoolManager *new_bpm(size_t pool_size, DiskManager *disk_manager);

/**
 * Unpins page of provided id from the buffer pool and returns true. If the
 * page does not exist or it's pin count is already 0, returns false. Sets
 * page's dirty bit to value provided in is_dirty
 */
bool unpin_page(page_id_t id, bool is_dirty, BufferPoolManager *bpm);

/**
 * Flush provided page to disk, regardless of its dirty bit
 * Unsets the provided page's dirty bit afterwards
 * Returns false if the page could not be found in the page table, true
 * otherwise
 */
bool flush_page(page_id_t id, BufferPoolManager *bpm);

/*
 * Allocates a new page of suitable TYPE on disk, places it in buffer pool BPM and returns a pointer to it.
 * Writes a possible replacement frame back to disk if it contains a dirty page.
 */
BpmPage *allocate_new_page(BufferPoolManager *bpm, PageType type);

/*
 * Returns the requested page from the buffer pool, or returns a null pointer if
 * page needs to be fetched from disk but no frames are available or evictable.
 * Writes a possible replacement frame back to disk if it contains a dirty page
 */
BpmPage *fetch_bpm_page(page_id_t page_id, BufferPoolManager *bpm);
