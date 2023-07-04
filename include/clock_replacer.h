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
frame_id_t evict(ClockReplacer *replacer);

/**
 * Pins a frame of specified id, meaning it should not be evictable until
 * unpinned
 */
void clock_replacer_pin(frame_id_t *frame_id, ClockReplacer *replacer);

/**
 * Unpins a frame of specified id, which makes it evictable
 */
void clock_replacer_unpin(frame_id_t *frame_id, ClockReplacer *replacer);

/**
 * Returns the number of frames currently in the ClockReplacer
 */
size_t size();
