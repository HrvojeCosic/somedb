#pragma once

#include "./circular_list.h"
#include "hash.h"
#include "shared.h"

typedef struct {
    size_t num_pages; // max number of pages ClockReplacer is required to store
    CircularList *frames;   // frames tracked by ClockReplacer
    HashTable *frame_table; // to map each frame to its reference bit
    CircularListNode *hand; // current position of the clock hand
    RWLOCK latch;
} ClockReplacer;

/**
 * Initializes and returns a clock replacer of the specified capacity
 */
ClockReplacer *clock_replacer_init(size_t capacity);

/**
 * Removes and returns the frame(id) closest to the clock hand that is both in
 * the ClockReplacer and with its ref flag set to false. If its ref flag is set
 * to true, sets it to false without returning. Returns UINT32_MAX if there is
 * no evictable frame
 */
frame_id_t victim(ClockReplacer *replacer);

/**
 * Removes provided frame containing the pinned page from the ClockReplacer
 * Should be called when the page is pinned to a frame in the BufferPoolManager.
 */
void pin(frame_id_t *frame_id, ClockReplacer *replacer);

/**
 * Adds provided frame containing the unpinned page to the ClockReplacer
 * Should be called when page's pin count becomes 0
 */
void unpin(frame_id_t *frame_id, ClockReplacer *replacer);

/**
 * Returns the number of frames currently in the ClockReplacer
 */
size_t size();
