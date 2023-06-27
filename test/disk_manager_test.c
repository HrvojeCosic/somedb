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

START_TEST(disk_manager) {
    pthread_t t1, t2;

    /*
     * ADD PAGE
     */
    const uint32_t id = 0;
    void *page = new_page(id);
    Header *header = PAGE_HEADER(page);

    /*
     * ADD TUPLE
     */
    TupleExample t_ex1 = {.y = "str1", .x = 1};
    TupleExample t_ex2 = {.y = "str2", .x = 2};
    AddTupleArgs t_args1 = {
        .page = page, .tuple = &t_ex1, .tuple_size = sizeof(TupleExample)};
    AddTupleArgs t_args2 = {
        .page = page, .tuple = &t_ex2, .tuple_size = sizeof(TupleExample)};
    pthread_create(&t1, NULL, (void *)add_tuple, &t_args1);
    pthread_create(&t2, NULL, (void *)add_tuple, &t_args2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    /*
     * GET TUPLE
     */
    TupleExample *t2_get = get_tuple(page, 1);
    ck_assert_str_eq(t2_get->y, "str2");
    ck_assert_int_eq(t2_get->x, 2);

    /*
     * REMOVE TUPLE
     */
    uint32_t tid = 0;
    remove_tuple(page, tid);
    ck_assert_ptr_null(get_tuple(page, tid));
    ck_assert_int_eq(header->flags, COMPACTABLE);
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 2);

    /*
     * DEFRAGMENT
     * After defragmenting, the removed tuple memory should be freed
     */
    uint16_t total_before = header->free_total;
    defragment(page);
    ck_assert_int_eq(t2_get->x, 0);
    ck_assert_int_eq(*t2_get->y, 0);
    ck_assert_int_eq(header->free_total,
                     total_before + sizeof(TupleExample) + sizeof(TuplePtr));
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 1);

    /*
     * DISK PERSISTENCE
     */
    write_page(page);
    void *rp = read_page(id);
    TupleExample *rc = get_tuple(rp, 0);
    ck_assert_str_eq(rc->y, "str2");
    ck_assert_int_eq(rc->x, 2);

    free(page);
}
END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("DiskManager");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, disk_manager);
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
