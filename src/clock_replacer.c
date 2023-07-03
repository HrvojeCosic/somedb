#include "../include/clock_replacer.h"
#include "../include/shared.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ClockReplacer *clock_replacer_init(size_t capacity) {
    ClockReplacer *replacer = malloc(sizeof(ClockReplacer));
    RWLOCK latch;
    RWLOCK_INIT(&latch);

    replacer->hand = NULL;
    replacer->frames = circular_list_init(capacity);
    replacer->num_pages = capacity;
    replacer->frame_table = init_hash(capacity);
    replacer->latch = latch;

    return replacer;
}

static char *frame_to_str(frame_id_t frame_id) {
    char *str = malloc(sizeof(char) * 11);
    snprintf(str, 11, "%d", frame_id);
    return str;
}

frame_id_t victim(ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);
    if (replacer->frames->size == 0)
        return UINT32_MAX;
    if (replacer->hand == NULL)
        replacer->hand = replacer->frames->head;

    for (size_t i = 0; i < replacer->frames->size; i++) {
        char *curr_frame = replacer->hand->value;
        HashEl *frame_info = hash_find(curr_frame, replacer->frame_table);

        if (*(bool *)frame_info->data == true) {
            bool *ref_bit = malloc(sizeof(bool));
            *ref_bit = false;
            hash_insert(curr_frame, ref_bit, replacer->frame_table);
            replacer->hand =
                circular_list_next(replacer->hand, replacer->frames);
            continue;
        } else {
            frame_id_t frame_id = atoi(frame_info->key);
            hash_remove(curr_frame, replacer->frame_table);
            circular_list_remove(curr_frame, replacer->frames);
            RWLOCK_UNLOCK(&replacer->latch);
            return frame_id;
        }
    }

    RWLOCK_UNLOCK(&replacer->latch);
    return UINT32_MAX;
}

void unpin(frame_id_t *frame_id, ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);
    char *frame_str = frame_to_str(*frame_id);

    void *node = circular_list_insert(frame_str, replacer->frames);
    if (node == NULL) {
        RWLOCK_UNLOCK(&replacer->latch);
        return;
    }

    bool *unpinned = malloc(sizeof(bool));
    *unpinned = true;
    hash_insert(frame_str, unpinned, replacer->frame_table);
    RWLOCK_UNLOCK(&replacer->latch);
}

void pin(frame_id_t *frame_id, ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);

    char *frame_str = frame_to_str(*frame_id);
    if (hash_remove(frame_str, replacer->frame_table)) {
        circular_list_remove(frame_str, replacer->frames);
    }

    RWLOCK_UNLOCK(&replacer->latch);
}
