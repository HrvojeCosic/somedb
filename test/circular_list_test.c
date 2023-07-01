#include "../include/circular_list.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

START_TEST(circular_list) {
    /*
     * INITIALIZE
     */
    CircularList *cl = circular_list_init(10);
    ck_assert_int_eq(cl->capacity, 10);
    ck_assert_ptr_null(cl->head);

    /*
     * INSERT
     */
    int *val1 = malloc(sizeof(int));
    *val1 = 1;
    int *val2 = malloc(sizeof(int));
    *val2 = 2;
    int *val3 = malloc(sizeof(int));
    *val3 = 3;
    CircularListNode *node1 = circular_list_insert(val1, cl);
    circular_list_insert(val2, cl);
    CircularListNode *node3 = circular_list_insert(val3, cl);
    ck_assert_int_eq(*(int *)(cl->head->value), *val3);
    ck_assert_int_eq(*(int *)(cl->head->next->value), *val2);
    ck_assert_int_eq(*(int *)(cl->head->next->next->value), *val1);
    ck_assert_int_eq(cl->size, 3);

    /*
     * GET NEXT
     */
    CircularListNode *next_to_3 = circular_list_next(node3, cl);
    ck_assert_ptr_eq(next_to_3, cl->head->next);

    CircularListNode *next_to_1 = circular_list_next(node1, cl);
    ck_assert_ptr_eq(next_to_1, cl->head);

    /*
     * REMOVE
     */
    circular_list_remove(val2, cl);
    ck_assert_ptr_eq(node3->next, node1);
    ck_assert_int_eq(cl->size, 2);

    /*
     * DESTROY
     */
    circular_list_destroy(&cl);
    ck_assert_int_eq(cl->size, 0);
}

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("CircularList");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, circular_list);
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
