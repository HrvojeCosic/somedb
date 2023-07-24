#include "../include/bpm.h"
#include <check.h>
#include <fcntl.h>
#include <search.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// static BufferPoolManager *bpm;
// static const page_id_t pid1 = 1, pid2 = 2;
// static DiskManager *disk_manager;
//
// START_TEST(initialize) {
//     char *filename = (char *)malloc(sizeof(char) * 11);
//     strcpy(filename, "dbfile.bin");
//     disk_manager = new_disk_manager(filename);
//     const size_t pool_size = 3;
//
//     bpm = new_bpm(pool_size, disk_manager);
//     ck_assert_int_eq(bpm->pool_size, pool_size);
//     for (size_t i = 0; i < bpm->pool_size; i++) {
//         ck_assert_int_eq(bpm->free_list[i], true);
//     }
// }
//
// END_TEST
//
// START_TEST(pin) {
//     page_id_t pid1 = new_page(disk_manager);
//     page_id_t pid2 = new_page(disk_manager);
//     // write_page(read_page(pid1, disk_manager), disk_manager);
//     // write_page(read_page(pid2, disk_manager), disk_manager);
//
//     BpmPage *bpm_page2 = new_bpm_page(bpm, pid2);
//     BpmPage *bpm_page1 = new_bpm_page(bpm, pid1);
//     ck_assert_int_eq(bpm_page1->is_dirty, false);
//     ck_assert_int_eq(bpm_page1->pin_count, 1);
//
//     Header *page1_h = PAGE_HEADER(bpm_page1->data);
//     Header *page2_h = PAGE_HEADER(bpm_page2->data);
//     ck_assert_int_eq(page1_h->id, pid1);
//     ck_assert_int_eq(page2_h->id, pid2);
//
//     BpmPage *dup_page1 = new_bpm_page(bpm, pid1);
//     ck_assert_ptr_eq(dup_page1, bpm_page1);
// }
//
// END_TEST
//
// START_TEST(unpin) {
//     bool ok2 = unpin_page(pid1, false, bpm);
//     ck_assert_int_eq(ok2, true);
// }
//
// END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("BufferPoolManager");

    tc_core = tcase_create("Core");

    // tcase_add_test(tc_core, initialize);
    // tcase_add_test(tc_core, pin);
    // tcase_add_test(tc_core, unpin);
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
