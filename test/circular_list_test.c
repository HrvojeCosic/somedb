#include "../include/circular_list.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

CircularList *cl;
static int *val1, *val2, *val3;
static CircularListNode *node1, *node2, *node3;

START_TEST(initialize) {
    cl = circular_list_init(10);
    ck_assert_int_eq(cl->capacity, 10);
    ck_assert_ptr_null(cl->head);

    val1 = malloc(sizeof(int));
    val2 = malloc(sizeof(int));
    val3 = malloc(sizeof(int));
    *val1 = 1;
    *val2 = 2;
    *val3 = 3;
}
END_TEST

START_TEST(insert_el) {
    node1 = circular_list_insert(val1, cl);
    node2 = circular_list_insert(val2, cl);
    node3 = circular_list_insert(val3, cl);
    ck_assert_int_eq(*(int *)(cl->head->value), *val3);
    ck_assert_int_eq(*(int *)(cl->head->next->value), *val2);
    ck_assert_int_eq(*(int *)(cl->head->next->next->value), *val1);
    ck_assert_int_eq(cl->size, 3);
}

START_TEST(get_next_el) {
    CircularListNode *next_to_3 = circular_list_next(node3, cl);
    ck_assert_ptr_eq(next_to_3, cl->head->next);

    CircularListNode *next_to_1 = circular_list_next(node1, cl);
    ck_assert_ptr_eq(next_to_1, cl->head);
}
END_TEST

START_TEST(remove_el) {
    circular_list_remove(val2, cl);
    ck_assert_ptr_eq(node3->next, node1);
    ck_assert_int_eq(cl->size, 2);
}
END_TEST

START_TEST(destroy) {
    circular_list_destroy(&cl);
    ck_assert_int_eq(cl->size, 0);
}
END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("CircularList");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, initialize);
    tcase_add_test(tc_core, insert_el);
    tcase_add_test(tc_core, get_next_el);
    tcase_add_test(tc_core, remove_el);
    tcase_add_test(tc_core, destroy);
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
