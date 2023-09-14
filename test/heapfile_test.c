#include "../include/disk/disk_manager.h"
#include "../include/disk/heapfile.h"
#include "../include/utils/serialize.h"
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

static pthread_t t1, t2;
static const char add_tab1[15] = "test_table1";
static const char add_tab2[15] = "test_table2";
static const char add_tab3[15] = "test_table3";
static const char add_page_tab[15] = "add_page_test";
static const char add_tuple_tab[25] = "add_tuple_to_page_test";
static TuplePtr *t_ptr1, *t_ptr2, *t_ptr3;

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
        char path[265] = {0};
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
    DiskManager *disk_mgr = create_table(add_page_tab, cols, 0);
    page_id_t pid1 = new_heap_page(disk_mgr);
    page_id_t pid2 = new_heap_page(disk_mgr);
    page_id_t pid3 = new_heap_page(disk_mgr);

    // all three should get same (first) page id as it's not being occupied by tuples
    // TODO: make it thread safe (track a dirty bit or something)
    ck_assert_int_eq(pid1, START_USER_PAGE);
    ck_assert_int_eq(pid2, START_USER_PAGE);
    ck_assert_int_eq(pid3, START_USER_PAGE);

    uint8_t *page = read_page(pid1, disk_mgr);
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
                      {.name_len = (uint8_t)strlen(cname2), .name = cname2, .type = INTEGER}};
    const char add_tuple_tab[25] = "add_tuple_to_page_test";
    DiskManager *disk_mgr = create_table(add_tuple_tab, cols, (sizeof(cols) / sizeof(Column)));
    page_id_t pid = new_heap_page(disk_mgr);

    const char *col_names[2] = {"name", "age"};
    const char *col_names2[2] = {"wrong_col", "another_wrong_col"};
    ColumnType col_types[2] = {STRING, INTEGER};
    ColumnValue col_vals[2] = {{.string = "Pero"}, {.integer = 21}};
    int name_len = strlen("Pero");

    t_ptr1 = (TuplePtr *)malloc(sizeof(TuplePtr));
    t_ptr2 = (TuplePtr *)malloc(sizeof(TuplePtr));

    AddTupleArgs t_args1 = {.disk_manager = disk_mgr,
                            .column_names = col_names,
                            .column_values = col_vals,
                            .column_types = col_types,
                            .num_columns = 2,
                            .tup_ptr_out = t_ptr1};
    AddTupleArgs t_args2 = {.disk_manager = disk_mgr,
                            .column_names = col_names2,
                            .column_values = col_vals,
                            .column_types = col_types,
                            .num_columns = 2,
                            .tup_ptr_out = t_ptr2};

    pthread_create(&t1, NULL, add_tuple, &t_args1);
    pthread_create(&t2, NULL, add_tuple, &t_args2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    uint8_t *page = read_page(pid, disk_mgr);
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
    memcpy(read_buf, (page + header.free_start - TUPLE_PTR_SIZE + sizeof(uint16_t)),
           TUPLE_PTR_SIZE); // "tuple size" of new tuple pointer
    ck_assert_uint_eq(decode_uint16(read_buf), t_ptr1->size);
    ck_assert_uint_ne(decode_uint16(read_buf), 0);
    memcpy(read_buf, page + t_ptr1->start_offset, t_ptr1->size); // new tuple's "name" string length
    ck_assert_uint_eq(decode_uint16(read_buf), name_len);
    memcpy(read_buf, page + t_ptr1->start_offset + sizeof(uint16_t), name_len); // new tuple's "name" value
    ck_assert_int_eq(strncmp((char *)read_buf, "Pero", name_len), 0);
    memcpy(read_buf, page + t_ptr1->start_offset + name_len + sizeof(uint16_t),
           sizeof(int32_t)); // new tuple's "age" value
    ck_assert_uint_eq(decode_int32(read_buf), 21);

    // assert correctness of tuple retreival from disk
    RID rid = {.pid = pid, .slot_num = 0};
    uint8_t *tuple_data = get_tuple(rid, disk_mgr);
    uint8_t tuple_data_buf[t_ptr1->size];
    memcpy(tuple_data_buf, tuple_data, sizeof(uint16_t)); // tuple's "name" string length
    ck_assert_uint_eq(decode_uint16(tuple_data_buf), name_len);
    memcpy(tuple_data_buf, tuple_data + sizeof(uint16_t), name_len); // tuple's "name" value
    ck_assert_int_eq(strncmp((char *)tuple_data_buf, "Pero", name_len), 0);
}

END_TEST

START_TEST(remove_tuple_and_defragment) {
    // -----------------------------------------------------------------------------------------------------
    // REMOVE
    // -----------------------------------------------------------------------------------------------------
    char cname1[5] = "name";
    char cname2[5] = "age";
    Column cols[2] = {{.name_len = (uint8_t)strlen(cname1), .name = cname1, .type = STRING},
                      {.name_len = (uint8_t)strlen(cname2), .name = cname2, .type = INTEGER}};
    DiskManager *disk_mgr = create_table(add_tuple_tab, cols, (sizeof(cols) / sizeof(Column)));
    page_id_t pid = new_heap_page(disk_mgr);

    const char *col_names[2] = {"name", "age"};
    ColumnType col_types[2] = {STRING, INTEGER};
    ColumnValue col_vals1[2] = {{.string = "Marica"}, {.integer = 21}};
    ColumnValue col_vals2[2] = {{.string = "Perica"}, {.integer = 31}};
    ColumnValue col_vals3[2] = {{.string = "Nikolina"}, {.integer = 41}};
    // Perica and Marica are of same length for defragment testing convenience (that while loop)
    uint16_t name_len1 = strlen("Marica");
    uint16_t name_len2 = strlen("Perica");
    t_ptr1 = (TuplePtr *)malloc(sizeof(TuplePtr));
    t_ptr2 = (TuplePtr *)malloc(sizeof(TuplePtr));
    t_ptr3 = (TuplePtr *)malloc(sizeof(TuplePtr));
    AddTupleArgs t_args1 = {.disk_manager = disk_mgr,
                            .column_names = col_names,
                            .column_values = col_vals1,
                            .column_types = col_types,
                            .num_columns = 2,
                            .tup_ptr_out = t_ptr1};
    AddTupleArgs t_args2 = {.disk_manager = disk_mgr,
                            .column_names = col_names,
                            .column_values = col_vals2,
                            .column_types = col_types,
                            .num_columns = 2,
                            .tup_ptr_out = t_ptr2};
    AddTupleArgs t_args3 = {.disk_manager = disk_mgr,
                            .column_names = col_names,
                            .column_values = col_vals3,
                            .column_types = col_types,
                            .num_columns = 2,
                            .tup_ptr_out = t_ptr3};
    add_tuple(&t_args1);
    add_tuple(&t_args2);
    add_tuple(&t_args3);
    // Slot numbers are just initialized in a way that tuple pointers are currently being inserted in a page on disk,
    // but in the future there should probably be some function that gives this information instead
    RID rid3 = {.pid = pid, .slot_num = 2};

    uint8_t *page_before = read_page(pid, disk_mgr);
    Header header_before = extract_header(page_before, pid);
    uint8_t read_buf[t_ptr1->size + t_ptr2->size]; // a bit of extra space
    memcpy(read_buf, (page_before + header_before.free_start - TUPLE_PTR_SIZE),
           TUPLE_PTR_SIZE); // last added tuple's "start offset"
    ck_assert_uint_eq(decode_uint16(read_buf),
                      PAGE_SIZE - 1 - t_ptr3->size - t_ptr2->size - t_ptr1->size); // is not 0 before
    memcpy(read_buf, page_before + t_ptr1->start_offset + name_len1 + sizeof(uint16_t),
           sizeof(uint32_t)); // "age" value
    ck_assert_uint_eq(decode_int32(read_buf), 21);

    remove_tuple(disk_mgr, rid3);

    uint8_t *page_after_remove = read_page(pid, disk_mgr);
    Header header_after_remove = extract_header(page_after_remove, pid);
    memcpy(read_buf, (page_after_remove + header_after_remove.free_start - TUPLE_PTR_SIZE),
           TUPLE_PTR_SIZE);                        // last added tuple's "start offset"
    ck_assert_uint_eq(decode_uint16(read_buf), 0); // should be 0, which means marked as removed
    memcpy(read_buf, page_before + t_ptr1->start_offset + name_len1 + sizeof(uint16_t),
           sizeof(uint32_t));                       // "age" value
    ck_assert_uint_eq(decode_uint32(read_buf), 21); // should remain same as before
    ck_assert_uint_eq(header_after_remove.flags, header_before.flags | COMPACTABLE);

    // -----------------------------------------------------------------------------------------------------
    // DEFRAGMENT
    // -----------------------------------------------------------------------------------------------------
    defragment(pid, disk_mgr);
    uint8_t *page_after_defrag = read_page(pid, disk_mgr);
    Header header_after_defrag = extract_header(page_after_defrag, pid);

    uint16_t curr_offset = PAGE_HEADER_SIZE;
    while (curr_offset < header_after_defrag.free_start) {
        memcpy(read_buf, (page_after_defrag + curr_offset), TUPLE_PTR_SIZE); // tuple's ptr
        uint16_t tup_off = decode_uint16(read_buf);
        uint16_t tup_size = decode_uint16(read_buf + sizeof(uint16_t));
        memcpy(read_buf, page_after_defrag + decode_uint16(read_buf), tup_size); // tuple content

        // assert tuple is of any content of added tuples other than that of the removed tuple's
        ck_assert(decode_uint16(read_buf) == name_len1 || decode_uint16(read_buf) == name_len2);
        memcpy(read_buf, page_after_defrag + tup_off + sizeof(uint16_t), name_len1); // "name" value
        ck_assert(strncmp((char *)read_buf, "Marica", name_len1) == 0 ||
                  strncmp((char *)read_buf, "Perica", name_len1) == 0);
        memcpy(read_buf, page_after_defrag + tup_off + sizeof(uint16_t) + name_len1, sizeof(int32_t)); // "age" value
        ck_assert(decode_uint32(read_buf) == 21 || decode_uint32(read_buf) == 31);

        curr_offset += TUPLE_PTR_SIZE;
    }
    ck_assert_uint_eq(header_after_defrag.flags, header_after_remove.flags & ~COMPACTABLE);
    ck_assert_uint_ne(decode_uint32(read_buf), 0); // should not be 0, which means tuple's not marked as removed
}

END_TEST

// for some reason there is a segfault in srunner_run_all() when running with tsan if this doesnt exist??
START_TEST(x) { return; }

END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Heapfile");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, x);
    tcase_add_test(tc_core, add_table);
    tcase_add_test(tc_core, add_page);
    tcase_add_test(tc_core, add_tuple_to_page);
    tcase_add_test(tc_core, remove_tuple_and_defragment);

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

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
