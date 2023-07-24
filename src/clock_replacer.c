#include "../include/clock_replacer.h"
#include "../include/shared.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ClockReplacer *clock_replacer_init(size_t capacity) {
    ClockReplacer *replacer = (ClockReplacer *)malloc(sizeof(ClockReplacer));
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
    char *str = (char *)malloc(sizeof(char) * 11);
    snprintf(str, 11, "%d", frame_id);
    return str;
}

frame_id_t evict(ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);
    if (replacer->frames->size == 0)
        return UINT32_MAX;
    if (replacer->hand == NULL)
        replacer->hand = replacer->frames->head;

    for (size_t i = 0; i < replacer->frames->size; i++) {
        char *curr_frame = (char *)replacer->hand->value;
        HashEl *frame_info = hash_find(curr_frame, replacer->frame_table);

        if (*(bool *)frame_info->data == true) {
            bool *ref_bit = (bool *)malloc(sizeof(bool));
            *ref_bit = false;
	    HashInsertArgs in_args = {.key = curr_frame, .data = ref_bit, .ht = replacer->frame_table };
	    hash_insert(&in_args);
            replacer->hand = circular_list_next(replacer->hand, replacer->frames);
            continue;
        } else {
            frame_id_t frame_id = atoi(frame_info->key);
	    HashRemoveArgs rm_args = {.key = curr_frame, .ht = replacer->frame_table};
	    hash_remove(&rm_args);
            circular_list_remove(curr_frame, replacer->frames);
            RWLOCK_UNLOCK(&replacer->latch);
            return frame_id;
        }
    }

    RWLOCK_UNLOCK(&replacer->latch);
    return UINT32_MAX;
}

void clock_replacer_unpin(frame_id_t *frame_id, ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);
    char *frame_str = frame_to_str(*frame_id);

    void *node = circular_list_insert(frame_str, replacer->frames);
    if (node == NULL) {
        RWLOCK_UNLOCK(&replacer->latch);
        return;
    }

    bool *unpinned = (bool *)malloc(sizeof(bool));
    *unpinned = true;
    HashInsertArgs in_args = {.key = frame_str, .data = unpinned, .ht = replacer->frame_table };
    hash_insert(&in_args);
    RWLOCK_UNLOCK(&replacer->latch);
}

void clock_replacer_pin(frame_id_t *frame_id, ClockReplacer *replacer) {
    RWLOCK_WRLOCK(&replacer->latch);

    char *frame_str = frame_to_str(*frame_id);
    bool *ok = (bool*)malloc(sizeof(bool));
    HashRemoveArgs rm_args = {.key = frame_str, .ht = replacer->frame_table, .success_out = ok};
    hash_remove(&rm_args);
    if (ok) {
        circular_list_remove(frame_str, replacer->frames);
	free(ok);
    }

    RWLOCK_UNLOCK(&replacer->latch);
}
