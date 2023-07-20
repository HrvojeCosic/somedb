#pragma once

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
} HashTable;

/*
 * Initializes and returns a hash table of the specified size
 */
HashTable *init_hash(uint32_t size);

/*
 * Inserts a HashEl of provided KEY and DATA into a hash table HT
 */
void hash_insert(const char *key, void *data, HashTable *ht);

/*
 * Returns a pointer to the element in the hash table HT of the provided KEY
 */
HashEl *hash_find(const char *key, HashTable *ht);

/*
 * Removes the element of KEY in the hash table HT
 */
bool hash_remove(const char *key, HashTable *ht);

/*
 * Frees all memory allocated for a hash table HT
 */
void destroy_hash(HashTable **ht);
