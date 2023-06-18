#pragma once

#include <cstddef>
#include <cstdint>

typedef struct ClockReplacer {
    size_t num_pages; // max number of pages ClockReplacer is required to store
    void  *frames; // frames tracked by ClockReplacer
    void *frame_table; // to map each frame to its reference bit
    void *hand; // current position of the clock h
} ClockReplacer;

/**
 * Finds the frame closest to the clock hand that is both in the ClockReplacer and with its ref flag set to false. If its ref flag is set to true, sets it to false without returning.
 */
uint32_t *victim();

/**
 * Removes provided frame containing the pinned page from the ClockReplacer
 * Should be called when the page is pinned to a frame in the BufferPoolManager.
 */
void pin(uint32_t frame_id);

/**
 * Adds provided frame containing the unpinned page to the ClockReplacer
 * Should be called when page's pin count becomes 0
 */
void unpint(uint32_t frame_id);

/**
 * Returns the number of frames currently in the ClockReplacer
 */
size_t size();
