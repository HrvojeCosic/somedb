#include "../include/hash.h"
#include "../include/shared.h"
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HashTable *ht;
static char *key1, *key2, *key3;
static page_id_t *val1, *val2, *val3;

START_TEST(initialize) {
    uint8_t hsize = 5;
    ht = init_hash(hsize);
    ck_assert_int_eq(ht->size, hsize);
    for (uint8_t i = 0; i < ht->size; i++) {
        ck_assert_ptr_null((ht->arr + i)->key);
    }
}
END_TEST

START_TEST(insert_find) {
    val1 = malloc(sizeof(page_id_t));
    val2 = malloc(sizeof(page_id_t));
    val3 = malloc(sizeof(page_id_t));
    key1 = malloc(sizeof(char) * 2);
    key2 = malloc(sizeof(char) * 2);
    key3 = malloc(sizeof(char) * 3);
    *val1 = 1;
    *val2 = 2;
    *val3 = 3;
    strncpy(key1, "1", 2);
    strncpy(key2, "2", 2);
    strncpy(key3, "11", 3); // collision
    hash_insert(key1, val1, ht);
    hash_insert(key2, val2, ht);
    hash_insert(key3, val3, ht);
    HashEl *found1 = hash_find(key1, ht);
    HashEl *found2 = hash_find(key2, ht);
    HashEl *found3 = hash_find(key3, ht);
    ck_assert_str_eq(found1->key, key1);
    ck_assert_str_eq(found2->key, key2);
    ck_assert_str_eq(found3->key, key3);
    ck_assert_int_eq(*(page_id_t *)found1->data, *val1);

    /**
     *Test collision handling
     */
    ck_assert_str_eq(found1->next->key, key3);
    ck_assert_ptr_null(found2->next);

    /**
     *Test overwriting duplicate keys
     */
    char *key2_overwrite = malloc(sizeof(char) * 2);
    strncpy(key2_overwrite, "2", 2);
    page_id_t *val2_overwrite = malloc(sizeof(page_id_t));
    *val2_overwrite = 33;
    hash_insert(key2_overwrite, val2_overwrite, ht);
    HashEl *overwritten = hash_find(key2, ht);
    ck_assert_ptr_null(overwritten->next); // still only key "2" in that bucket
    ck_assert_str_eq(overwritten->key, key2_overwrite);
    ck_assert_int_eq(*(page_id_t *)overwritten->data, *val2_overwrite);
}

START_TEST(remove_entry) {
    hash_remove(key2, ht); // Remove sole element in a bucket
    ck_assert_ptr_null(hash_find(key2, ht));
    hash_insert(key3, val3, ht);
    hash_remove(key3, ht); // Remove first element in a multiple-element bucket
    page_id_t *val3_second = malloc(sizeof(page_id_t));
    *val3_second = 32;
    char *key3_second = malloc(sizeof(char) * 3);
    hash_insert(key3_second, val3_second, ht);
    hash_remove(key1, ht);
}

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Hash");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, initialize);
    tcase_add_test(tc_core, insert_find);
    tcase_add_test(tc_core, remove_entry);
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

    destroy_hash(&ht);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
