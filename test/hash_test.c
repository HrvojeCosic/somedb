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
static pthread_t t1, t2, t3;

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
    val1 = (page_id_t *)malloc(sizeof(page_id_t));
    val2 = (page_id_t *)malloc(sizeof(page_id_t));
    val3 = (page_id_t *)malloc(sizeof(page_id_t));
    key1 = (char *)malloc(sizeof(char) * 2);
    key2 = (char *)malloc(sizeof(char) * 2);
    key3 = (char *)malloc(sizeof(char) * 3);
    *val1 = 1;
    *val2 = 2;
    *val3 = 3;
    strncpy(key1, "1", 2);
    strncpy(key2, "2", 2);
    strncpy(key3, "11", 3); // collision
    HashInsertArgs in_args1 = {.key = key1, .data = val1, .ht = ht};
    HashInsertArgs in_args2 = {.key = key2, .data = val2, .ht = ht};
    HashInsertArgs in_args3 = {.key = key3, .data = val3, .ht = ht};
    pthread_create(&t1, NULL, hash_insert, &in_args1);
    pthread_create(&t2, NULL, hash_insert, &in_args2);
    pthread_create(&t3, NULL, hash_insert, &in_args3);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
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
    char *key2_overwrite = (char *)malloc(sizeof(char) * 2);
    strncpy(key2_overwrite, "2", 2);
    page_id_t *val2_overwrite = (page_id_t *)malloc(sizeof(page_id_t));
    *val2_overwrite = 33;
    HashInsertArgs in_args = {.key = key2_overwrite, .data = val2_overwrite, .ht = ht};
    hash_insert(&in_args);
    HashEl *overwritten = hash_find(key2, ht);
    ck_assert_ptr_null(overwritten->next); // still only key "2" in that bucket
    ck_assert_str_eq(overwritten->key, key2_overwrite);
    ck_assert_int_eq(*(page_id_t *)overwritten->data, *val2_overwrite);
}

START_TEST(remove_entry) {
    HashRemoveArgs rm_args = {.key = key2, .ht = ht, .success_out = NULL};
    pthread_create(&t1, NULL, hash_remove, &rm_args);
    pthread_create(&t2, NULL, hash_remove, &rm_args); // remove sole bucket el.
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    ck_assert_ptr_null(hash_find(key2, ht));
    HashInsertArgs in_args = {.key = key3, .data = val3, .ht = ht};
    hash_insert(&in_args);
    HashRemoveArgs rm_args2 = {.key = key3, .ht = ht, .success_out = NULL};
    hash_remove(&rm_args2); // Remove first element in a multiple-element bucket
    page_id_t *val3_second = (page_id_t *)malloc(sizeof(page_id_t));
    *val3_second = 32;
    char *key3_second = (char *)malloc(sizeof(char) * 3);
    HashInsertArgs in_args2 = {.key = key3_second, .data = val3_second, .ht = ht};
    hash_insert(&in_args2);
    HashRemoveArgs rm_args3 = {.key = key1, .ht = ht, .success_out = NULL};
    hash_remove(&rm_args3);
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
