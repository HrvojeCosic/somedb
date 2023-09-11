#include "../include/disk/bpm.h"
#include "../include/disk/heapfile.h"
#include <check.h>
#include <fcntl.h>
#include <search.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BufferPoolManager *bpm;
static page_id_t pid;
static DiskManager *disk_manager;
static const char table_name[15] = "bpm_test";

void teardown(void) { remove_table(table_name); }

START_TEST(initialize) {
    char col_n[4] = "bpm";
    Column cols[1] = {{.name_len = 3, .name = col_n, .type = VARCHAR}};
    disk_manager = create_table("bpm_test", cols, 1);

    const size_t pool_size = 3;

    bpm = new_bpm(pool_size, disk_manager);
    ck_assert_int_eq(bpm->pool_size, pool_size);
    for (size_t i = 0; i < bpm->pool_size; i++) {
        ck_assert_int_eq(bpm->free_list[i], true);
    }
}

END_TEST

START_TEST(pin) {
    BpmPage *bpm_page = allocate_new_page(bpm, HEAP_PAGE);
    pid = bpm_page->id;

    ck_assert_int_eq(bpm_page->is_dirty, false);
    ck_assert_int_eq(bpm_page->pin_count, 1);

    Header *page1_h = PAGE_HEADER(bpm_page->data);
    ck_assert_int_eq(page1_h->id, 0);
}

END_TEST

START_TEST(unpin) {
    bool ok1 = unpin_page(pid, false, bpm);
    ck_assert_int_eq(ok1, true);

    unpin_page(pid, false, bpm);
    bool ok2 = unpin_page(pid, false, bpm); // once for newpage once for fetchpage
    ck_assert_int_eq(ok2, false);

    char pid_str[11];
    sprintf(pid_str, "%d", pid);
    HashEl *el = hash_find(pid_str, bpm->page_table);
    ck_assert_int_eq(*(bool *)el->data, false); // dirty bit unset
}

END_TEST

START_TEST(flush_page_test) {
    char pid_str[11];
    sprintf(pid_str, "%d", pid);
    HashEl *entry_before_flush = hash_find(pid_str, bpm->page_table);
    frame_id_t fid = *(frame_id_t *)entry_before_flush->data;
    sprintf(pid_str, "%d", fid);
    ck_assert_int_eq(bpm->free_list[fid], false);

    bool ok1 = flush_page(pid, bpm);
    ck_assert_int_eq(ok1, true);

    HashEl *entry_after_flush = hash_find(pid_str, bpm->page_table);
    ck_assert_ptr_null(entry_after_flush);
    ck_assert_int_eq(bpm->free_list[fid], true);

    bool ok2 = flush_page(pid, bpm);
    ck_assert_int_eq(ok2, false);
}

END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("BufferPoolManager");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, initialize);
    tcase_add_test(tc_core, pin);
    tcase_add_test(tc_core, unpin);
    tcase_add_test(tc_core, flush_page_test);

    tcase_add_checked_fixture(tc_core, NULL, teardown);

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
