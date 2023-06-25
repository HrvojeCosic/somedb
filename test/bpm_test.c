#include "../include/bpm.h"
#include <check.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>

START_TEST(buffer_pool_manager) {

    /*
     * INITIALIZE
     */
    size_t pool_size = 10;
    BufferPoolManager *bpm = new_bpm(pool_size);
    ck_assert_int_eq(bpm->pool_size, pool_size);
    for (size_t i = 0; i < bpm->pool_size; i++) {
        ck_assert_int_eq(bpm->free_list[i], true);
    }

    /*
     * UNPIN PAGE
     */
    bool unpinned = unpin_page(-1, false, bpm);
    ck_assert_int_eq(unpinned, false);
}
END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Page");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, buffer_pool_manager);
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
