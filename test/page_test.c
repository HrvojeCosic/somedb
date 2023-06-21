#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../include/page.h"

typedef struct Customer {
    char name[30];
    int age;
} Customer;

START_TEST(page)
{
    /*
     * ADD PAGE
     */
    const uint32_t id = 0;
    void* page = new_page(id);
    Header* header = PAGE_HEADER(page);

    /*
     * ADD TUPLE
     */
    Customer c1 = {.name="Bob", .age=27};
    Customer c2 = {.name="Alice", .age=32};

    TuplePtr* c1_ptr = add_tuple(page, &c1, sizeof(Customer));
    add_tuple(page, &c2, sizeof(Customer));

    Customer* c1_add = page + c1_ptr->start_offset;
    ck_assert_str_eq(c1_add->name, "Bob");
    ck_assert_int_eq(c1_add->age, 27);

    /*
     * GET TUPLE
     */
    Customer* c2_get = get_tuple(page, 1);
    ck_assert_str_eq(c2_get->name, "Alice");
    ck_assert_int_eq(c2_get->age, 32);

    /*
     * REMOVE TUPLE
     */
    uint32_t tid = 0;
    remove_tuple(page, tid);
    ck_assert_ptr_null(get_tuple(page, tid));
    ck_assert_int_eq(c1_ptr->start_offset, 0);
    ck_assert_int_eq(header->flags, COMPACTABLE);
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 2);

    /*
     * DEFRAGMENT
     * After defragmenting, the removed tuple memory should be freed
     */
    uint16_t total_before = header->free_total;
    defragment(page);
    ck_assert_int_eq(c2_get->age, 0);
    ck_assert_int_eq(*c2_get->name, 0); 
    ck_assert_int_eq(header->free_total, total_before + sizeof(Customer) + sizeof(TuplePtr));
    ck_assert_int_eq(get_tuple_ptr_list(page).length, 1);

    /*
     * DISK PERSISTENCE
     */
    char filename[20] = "test_page.txt";
    int fd = open(filename, O_RDWR | O_CREAT);
    write_page(page, fd);
    void* rp = read_page(id, fd);
    remove(filename); // remove now in case assert fails
    Customer* rc = get_tuple(rp, 0);
    ck_assert_str_eq(rc->name, "Alice");
    ck_assert_int_eq(rc->age, 32);

    free(page);
}
END_TEST

Suite* page_suite(void)
{
    Suite* s;
    TCase* tc_core;

    s = suite_create("Page");
    
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, page);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) 
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = page_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
