#include "../include/disk_manager.h"
#include "../include/serialize.h"
#include <check.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int x;
    char y[30];
} TupleExample;

static void *page;
static page_id_t pid;
static pthread_t t1, t2;
static const char add_tab1[15] = "test_table1";
static const char add_tab2[15] = "test_table2";
static const char add_tab3[15] = "test_table3";
static const char add_page_tab[15] = "add_page_test";
static const char add_tuple_tab[25] = "add_tuple_to_page_test";
TuplePtr *t_ptr1, *t_ptr2;

void add_table_teardown(void) {
    remove_table(add_tab1);
    remove_table(add_tab2);
    remove_table(add_tab3);
}

void add_page_teardown(void) { remove_table(add_page_tab); }

void add_tuple_teardown(void) { 
    remove_table(add_tuple_tab); 
    free(t_ptr1);
    free(t_ptr2);
}

START_TEST(add_table) {
    char col_n[4] = "ads";
    Column cols[1] = {{.name_len = 3, .name = col_n, .type = STRING}};
    create_table(add_tab1, cols, 0);
    create_table(add_tab2, cols, 0);
    create_table(add_tab3, cols, 0);
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
    Column *cols = {0};
    create_table(add_page_tab, cols, 0);
    page_id_t pid1 = new_page(add_page_tab);
    page_id_t pid2 = new_page(add_page_tab);
    page_id_t pid3 = new_page(add_page_tab);

    // all three should get same (first) page id as it's not being occupied by tuples
    // TODO: make it thread safe (track a dirty bit or something)
    ck_assert_int_eq(pid1, START_USER_PAGE);
    ck_assert_int_eq(pid2, START_USER_PAGE);
    ck_assert_int_eq(pid3, START_USER_PAGE);

    uint8_t *page = read_page(pid1, add_page_tab);
    uint8_t *buf;
    // check correct header format
    ck_assert_uint_eq(decode_uint16(page), PAGE_HEADER_SIZE);
    ck_assert_uint_eq(decode_uint16(page + sizeof(uint16_t)), PAGE_SIZE - 1);
    ck_assert_uint_eq(*(uint8_t *)(page + (sizeof(uint16_t) * 2)), 0x00);
}

END_TEST

START_TEST(add_tuple_to_page) {
    char cname1[5] = "name";
    char cname2[5] = "age";
    Column cols[2] = {{.name_len = (uint8_t)strlen(cname1), .name = cname1, .type = STRING},
                      {.name_len = (uint8_t)strlen(cname2), .name = cname2, .type = UINT16}};
    create_table(add_tuple_tab, cols, (sizeof(cols) / sizeof(Column)));
    page_id_t pid = new_page(add_tuple_tab);

    const char *col_names[2] = {"name", "age"};
    const char *col_names2[2] = {"wrong_col", "another_wrong_col"};
    ColumnType col_types[2] = {STRING, UINT16};
    ColumnValue col_vals[2] = {{.string_value = "Pero"}, {.uint32_value = 21}};
    int name_len = strlen("Pero");

    t_ptr1 = (TuplePtr *)malloc(sizeof(TuplePtr));
    t_ptr2 = (TuplePtr *)malloc(sizeof(TuplePtr));

    AddTupleArgs t_args1 = {.table_name = add_tuple_tab,
                            .column_names = col_names,
                            .column_values = col_vals,
                            .column_types = col_types,
                            .num_columns = 2,
			    .tup_ptr_out = t_ptr1};
    AddTupleArgs t_args2 = {.table_name = add_tuple_tab,
                            .column_names = col_names2,
                            .column_values = col_vals,
                            .column_types = col_types,
                            .num_columns = 2,
			    .tup_ptr_out = t_ptr2};

    pthread_create(&t1, NULL, (void *(*)(void *))add_tuple, &t_args1);
    pthread_create(&t2, NULL, (void *(*)(void *))add_tuple, &t_args2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    uint8_t *page = read_page(pid, add_tuple_tab);
    Header header = extract_header(page, pid);

    // assert new tuple correctness
    uint8_t read_buf[t_ptr1->size];
    memcpy(read_buf, page, 2); // header free start
    ck_assert_uint_eq(decode_uint16(read_buf), PAGE_HEADER_SIZE + TUPLE_PTR_SIZE);
    memcpy(read_buf, page + 2, 2); // header free end
    ck_assert_uint_eq(decode_uint16(read_buf), PAGE_SIZE - 1 - t_ptr1->size);
    memcpy(read_buf, (page + header.free_start - TUPLE_PTR_SIZE),
           TUPLE_PTR_SIZE); // "tuple start offset" of new tuple pointer
    ck_assert_uint_eq(decode_uint16(read_buf), PAGE_SIZE - 1 - t_ptr1->size);
    memcpy(read_buf, page + t_ptr1->start_offset, t_ptr1->size); // new tuple's "name" string length
    ck_assert_uint_eq(decode_uint16(read_buf), name_len);
    memcpy(read_buf, page + t_ptr1->start_offset + sizeof(uint16_t), name_len); // new tuple's "name" value
    ck_assert_int_eq(strncmp((char*)read_buf, "Pero", name_len), 0);
    memcpy(read_buf, page + t_ptr1->start_offset + name_len + sizeof(uint16_t),
           sizeof(uint16_t)); // new tuple's "age" value
    ck_assert_uint_eq(decode_uint16(read_buf), 21);

    // assert correctness of tuple retreival from disk
    RID rid = {.pid = pid, .slot_num = 0};
    uint8_t *tuple_data = get_tuple(rid, add_tuple_tab);
    uint8_t tuple_data_buf[t_ptr1->size];
    memcpy(tuple_data_buf, tuple_data, sizeof(uint16_t)); // tuple's "name" string length
    ck_assert_uint_eq(decode_uint16(tuple_data_buf), name_len);
    memcpy(tuple_data_buf, tuple_data + sizeof(uint16_t), name_len); // tuple's "name" value
    ck_assert_int_eq(strncmp((char *)tuple_data_buf, "Pero", name_len), 0);
}

END_TEST

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
    tcase_add_test(tc_core, add_tuple_to_page);
    //     tcase_add_test(tc_core, remove_tuple_from_page);
    //     tcase_add_test(tc_core, defragment_and_disk_persistance);

    tcase_add_checked_fixture(tc_core, NULL, add_table_teardown);
    tcase_add_checked_fixture(tc_core, NULL, add_page_teardown);
    tcase_add_checked_fixture(tc_core, NULL, add_tuple_teardown);

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

    free(page);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
