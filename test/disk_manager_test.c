#include "../include/disk_manager.h"
#include <check.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x;
    char y[30];
} TupleExample;

static void *page;
static uint32_t pid;
static Header *header;
static pthread_t t1, t2;
static TupleExample *tup_get;

START_TEST(add_page) {
    pid = 0;
    page = new_page(pid);
    header = PAGE_HEADER(page);
}
END_TEST

START_TEST(add_tuple_to_page) {
    TupleExample t_ex1 = {.x = 1, .y = "str1"};
    TupleExample t_ex2 = {.x = 2, .y = "str2"};
    AddTupleArgs t_args1 = {
        .page = page, .tuple = &t_ex1, .tuple_size = sizeof(TupleExample)};
    AddTupleArgs t_args2 = {
        .page = page, .tuple = &t_ex2, .tuple_size = sizeof(TupleExample)};
    pthread_create(&t1, NULL, (void *(*)(void *))add_tuple, &t_args1);
    pthread_create(&t2, NULL, (void *(*)(void *))add_tuple, &t_args2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}
END_TEST

START_TEST(get_tuple_from_page) {
    tup_get = (TupleExample *)get_tuple(page, 1);
    ck_assert_str_eq(tup_get->y, "str2");
    ck_assert_int_eq(tup_get->x, 2);
}
END_TEST

START_TEST(remove_tuple_from_page) {
    uint32_t tid = 1;
    remove_tuple(page, tid);
    ck_assert_ptr_null(get_tuple(page, tid));
    ck_assert_int_eq(header->flags, COMPACTABLE);
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 2);
}
END_TEST

START_TEST(defragment_and_disk_persistance) {
    uint16_t total_before = header->free_total;
    defragment(page);
    TupleExample *removed = (TupleExample *)get_tuple(page, 1);
    ck_assert_ptr_null(removed);
    ck_assert_int_eq(header->free_total,
                     total_before + sizeof(TupleExample) + sizeof(TuplePtr));
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 1);

    write_page(page);
    void *r_page = read_page(pid);
    TupleExample *r_tuple = (TupleExample *)get_tuple(r_page, 0);
    ck_assert_str_eq(r_tuple->y, "str1");
    ck_assert_int_eq(r_tuple->x, 1);
}
END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("DiskManager");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, add_page);
    tcase_add_test(tc_core, add_tuple_to_page);
    tcase_add_test(tc_core, get_tuple_from_page);
    tcase_add_test(tc_core, remove_tuple_from_page);
    tcase_add_test(tc_core, defragment_and_disk_persistance);
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

    free(header);
    free(page);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
