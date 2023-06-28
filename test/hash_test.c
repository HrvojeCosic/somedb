#include "../include/hash.h"
#include "../include/shared.h"
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

START_TEST(hash) {
    /*
     * INITIALIZE
     */
    uint8_t hsize = 5;
    HashTable *ht = init_hash(hsize);
    ck_assert_int_eq(ht->size, hsize);
    for (uint8_t i = 0; i < ht->size; i++) {
        ck_assert_ptr_null((ht->arr + i)->key);
    }

    /*
     * INSERT/FIND
     */
    page_id_t *val1 = malloc(sizeof(page_id_t));
    page_id_t *val2 = malloc(sizeof(page_id_t));
    page_id_t *val3 = malloc(sizeof(page_id_t));
    char *key1 = malloc(sizeof(char) * 2);
    char *key2 = malloc(sizeof(char) * 2);
    char *key3 = malloc(sizeof(char) * 3);
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

    // Test collision handling
    ck_assert_str_eq(found1->next->key, key3);
    ck_assert_ptr_null(found2->next);

    /*
     * REMOVE
     */
    hash_remove(key2, ht); // Remove sole element in a bucket
    ck_assert_ptr_null(hash_find(key2, ht));

    hash_insert(key3, val3, ht);
    hash_remove(key3, ht); // Remove first element in a multiple-element bucket

    page_id_t *val3_second = malloc(sizeof(page_id_t));
    *val3_second = 32;
    hash_insert(key3, val3_second, ht);
    hash_remove(key1, ht);

    destroy_hash(&ht);
}
END_TEST

Suite *page_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Hash");

    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, hash);
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
