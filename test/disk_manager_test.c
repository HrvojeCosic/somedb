#include "../include/disk_manager.h"
#include "../include/serialize.h"
#include <check.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int x;
    char y[30];
} TupleExample;

static void *page;
static page_id_t pid;
static Header *header;
static pthread_t t1, t2;
static TupleExample *tup_get;
static char add_tab1[15] = "test_table1";
static char add_tab2[15] = "test_table2";
static char add_tab3[15] = "test_table3";
static char add_page_tab[15] = "add_page_test";

void add_table_teardown(void) {
    remove_table(add_tab1);
    remove_table(add_tab2);
    remove_table(add_tab3);
}

void add_page_teardown(void) { remove_table(add_page_tab); }

START_TEST(add_table) {
    table_file(add_tab1);
    table_file(add_tab2);
    table_file(add_tab3);
    DIR *directory;
    struct dirent *entry;
    directory = opendir(DBFILES_DIR);

    int dir_file_count = 0;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type != DT_REG)
            continue;

        // assert correct names
        char add_tab1[15] = "test_table1.db";
        char add_tab2[15] = "test_table2.db";
        char add_tab3[15] = "test_table3.db";
        ck_assert(strcmp(add_tab1, entry->d_name) == 0 || strcmp(add_tab2, entry->d_name) == 0 ||
                  strcmp(add_tab3, entry->d_name) == 0);

        // assert correct file headers
        char path[50] = {0};
        sprintf(path, "%s/%s", DBFILES_DIR, entry->d_name);
        int fd = open(path, O_RDONLY);
        uint8_t header_buf[PAGE_SIZE];
        read(fd, header_buf, PAGE_SIZE);
        for (uint16_t i = 0; i < MAX_PAGES; i++) {
            uint16_t pid = decode_uint16(header_buf + (i * 4));
            uint16_t free_space = decode_uint16(header_buf + (i * 4) + 2);
            ck_assert_int_eq(pid, i);
            ck_assert_int_eq(free_space, PAGE_SIZE);
        }

        dir_file_count++;
    }
    ck_assert_int_eq(dir_file_count, 3);
}

END_TEST

START_TEST(add_page) {
    table_file(add_page_tab);
    page_id_t pid = new_page(add_page_tab);
    ck_assert_int_eq(pid, 1);
}

END_TEST

//
// START_TEST(add_tuple_to_page) {
//    TupleExample t_ex1 = {.x = 1, .y = "str1"};
//    TupleExample t_ex2 = {.x = 2, .y = "str2"};
//    AddTupleArgs t_args1 = {
//        .page_id = pid, .tuple = &t_ex1, .tuple_size = sizeof(TupleExample), .disk_manager = disk_manager};
//    AddTupleArgs t_args2 = {
//        .page_id = pid, .tuple = &t_ex2, .tuple_size = sizeof(TupleExample), .disk_manager = disk_manager};
//    add_tuple(&t_args1);
//    add_tuple(&t_args2);
//    pthread_create(&t1, NULL, (void *(*)(void *))add_tuple, &t_args1);
//    pthread_create(&t2, NULL, (void *(*)(void *))add_tuple, &t_args2);
//    pthread_join(t1, NULL);
//    pthread_join(t2, NULL);
//    tup_get = (TupleExample *)get_tuple(pid, 0, disk_manager);
//    ck_assert_int_eq(tup_get->x, 1);
//    ck_assert_str_eq(tup_get->y, "str2");
//}
//
// END_TEST
//
// START_TEST(remove_tuple_from_page) {
//    uint32_t tid = 1;
//    remove_tuple(page, tid);
//    ck_assert_ptr_null(get_tuple(pid, tid, disk_manager));
//    ck_assert_int_eq(header->flags, COMPACTABLE);
//    ck_assert_int_eq(get_tuple_ptr_list(page).length, 2);
//}
//
// END_TEST
//
// START_TEST(defragment_and_disk_persistance) {
//    uint16_t total_before = header->free_total;
//    defragment(page, disk_manager);
//    TupleExample *removed = (TupleExample *)get_tuple(pid, 1, disk_manager);
//    ck_assert_ptr_null(removed);
//    ck_assert_int_eq(header->free_total, total_before + sizeof(TupleExample) + sizeof(TuplePtr));
//    ck_assert_int_eq(get_tuple_ptr_list(page).length, 1);
//
//    write_page(pid, page, disk_manager);
//    void *r_page = read_page(pid, disk_manager);
//    TupleExample *r_tuple = (TupleExample *)get_tuple(pid, 0, disk_manager);
//    ck_assert_str_eq(r_tuple->y, "str1");
//    ck_assert_int_eq(r_tuple->x, 1);
//}
//
// END_TEST
//
Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("DiskManager");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, add_table);
    tcase_add_test(tc_core, add_page);
    // tcase_add_test(tc_core, add_tuple_to_page);
    //     tcase_add_test(tc_core, remove_tuple_from_page);
    //     tcase_add_test(tc_core, defragment_and_disk_persistance);

    tcase_add_checked_fixture(tc_core, NULL, add_table_teardown);
    tcase_add_checked_fixture(tc_core, NULL, add_page_teardown);

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
