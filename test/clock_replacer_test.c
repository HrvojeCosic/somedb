#include "../include/clock_replacer.h"
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static char *frame_to_str(frame_id_t frame_id) {
    char *str = (char *)malloc(sizeof(char) * 11);
    snprintf(str, 11, "%d", frame_id);
    return str;
}

static ClockReplacer *replacer;

START_TEST(initialize) {
    size_t size = 10;
    replacer = clock_replacer_init(size);
    ck_assert_ptr_null(replacer->hand);
    ck_assert_int_eq(replacer->num_pages, size);
    ck_assert_ptr_nonnull(replacer->frame_table);
    ck_assert_ptr_nonnull(replacer->frames);
}

START_TEST(pin_unpin) {
    for (frame_id_t fid = 0; fid < 10; fid++) {
        /**
         * UNPIN
         */
        frame_id_t *fid_ptr = (frame_id_t *)malloc(sizeof(frame_id_t));
        *fid_ptr = fid;
        clock_replacer_unpin(fid_ptr, replacer);

        char *fid_str = (char *)malloc(sizeof(char) * 11);
        snprintf(fid_str, 11, "%d", fid);
        HashEl *found = hash_find(fid_str, replacer->frame_table);
        ck_assert_ptr_nonnull(found);
        ck_assert_str_eq(found->key, fid_str);
        ck_assert_int_eq(*(bool *)found->data, 1);

        /**
         * PIN
         */
        clock_replacer_pin(fid_ptr, replacer);
        HashEl *found_fid = hash_find(frame_to_str(fid), replacer->frame_table);
        ck_assert_ptr_null(found_fid);
    }
}

START_TEST(victim) {
    for (frame_id_t fid = 0; fid < 3; fid++) {
        frame_id_t *fid_ptr = (frame_id_t *)malloc(sizeof(frame_id_t));
        *fid_ptr = fid;
        clock_replacer_unpin(fid_ptr, replacer);
    }
    frame_id_t vic_full_fid = evict(replacer);
    ck_assert_uint_eq(vic_full_fid, UINT32_MAX);

    /**
     * Unpin frames 0 and 1. Leave frame 2 pinned
     */
    for (frame_id_t fid = 0; fid < 2; fid++) {
        frame_id_t *fid_ptr = (frame_id_t *)malloc(sizeof(frame_id_t));
        *fid_ptr = fid;
        clock_replacer_unpin(fid_ptr, replacer);
    }
    frame_id_t *fid_ptr = (frame_id_t *)malloc(sizeof(frame_id_t));
    *fid_ptr = 2;
    frame_id_t vic_nonfull_fid = evict(replacer);
    ck_assert_uint_eq(vic_nonfull_fid, *fid_ptr);
}

END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("ClockReplacer");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, initialize);
    tcase_add_test(tc_core, pin_unpin);
    tcase_add_test(tc_core, victim);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = page_suite();
    sr = srunner_create(s);

    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
