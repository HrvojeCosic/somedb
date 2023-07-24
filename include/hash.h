#pragma once

#include "shared.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct HashEl {
    const char *key;
    void *data;
    struct HashEl *next;
} HashEl;

typedef struct {
    HashEl *arr;
    uint16_t size;
    RWLOCK latch;
} HashTable;

/*
 * Initializes and returns a hash table of the specified size
 */
HashTable *init_hash(uint32_t size);

/*
 * Inserts a HashEl of provided KEY and DATA into a hash table HT
 */
void hash_insert(void *hash_insert_args);

typedef struct {
    const char *key;
    void *data;
    HashTable *ht;
} HashInsertArgs;

/*
 * Returns a pointer to the element in the hash table HT of the provided KEY
 */
HashEl *hash_find(const char *key, HashTable *ht);

/*
 * Removes the element of KEY in the hash table HT
 */
void hash_remove(void *hash_remove_args);

typedef struct {
    const char *key;
    HashTable *ht;
    bool *success_out;
} HashRemoveArgs;

/*
 * Frees all memory allocated for a hash table HT
 */
void destroy_hash(HashTable **ht);
