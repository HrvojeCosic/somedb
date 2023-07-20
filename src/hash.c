#include "../include/hash.h"
#include "../include/shared.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashTable *init_hash(uint32_t size) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    ht->size = size;
    ht->arr = (HashEl *)calloc(sizeof(HashEl), size);
    return ht;
}

static void free_hash_el(HashEl **el) {
    free((void *)(*el)->key);
    free((*el)->data);
    (*el)->key = NULL;
    (*el)->data = NULL;
    free(*el);
    *el = NULL;
}

void destroy_hash(HashTable **ht) {
    if (ht == NULL || *ht == NULL)
        return;

    for (uint16_t i = 0; i < (*ht)->size; i++) {
        HashEl *curr = (*ht)->arr + i;
        HashEl *temp = NULL;

        while (curr && curr->key && curr->next) { // don't attempt to free an "ht->arr" element
            temp = curr->next;
            curr = curr->next->next;
            free_hash_el(&temp);
        }
    }
    free((*ht)->arr);
    (*ht)->arr = NULL;
    free(*ht);
    *ht = NULL;
}

/**
 * Hashing function
 * To be thought about
 */
static uint16_t hash(const char *key, HashTable *ht) { return atoi(key) % ht->size; }

void hash_insert(const char *key, void *data, HashTable *ht) {
    if (key == NULL || data == NULL)
        return;

    uint16_t idx = hash(key, ht);

    HashEl *el = (HashEl *)malloc(sizeof(HashEl));
    el->key = key;
    el->data = data;
    el->next = NULL;

    if (ht->arr[idx].key == NULL) {
        ht->arr[idx] = *el;
    } else {
        HashEl *temp = &ht->arr[idx];
        HashEl *prev = NULL;
        bool overwrite = false;
        while (temp != NULL) {
            if (strcmp(temp->key, key) == 0) {
                el->next = temp->next;
                *temp = *el;
                overwrite = true;
                break;
            }
            prev = temp;
            temp = temp->next;
        }
        if (!overwrite)
            prev->next = el;
    }
}

bool hash_remove(const char *key, HashTable *ht) {
    uint16_t idx = hash(key, ht);
    HashEl *temp = ht->arr + idx;
    HashEl *prev = NULL;

    while (temp != NULL && temp->key != NULL && strcmp(temp->key, key) != 0) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
        return false;

    if (prev == NULL) {           // if its first el. in LL
        if (temp->next == NULL) { // if its the only el. in LL
            ht->arr[idx] = (HashEl){.key = NULL, .data = NULL, .next = NULL};
        } else {
            HashEl *next = temp->next;
            ht->arr[idx] = *next;
        }
        free((void *)ht->arr[idx].key);
        free(ht->arr[idx].data);
    } else {
        prev->next = temp->next;
        free_hash_el(&temp);
    }
    return true;
}

HashEl *hash_find(const char *key, HashTable *ht) {
    uint16_t idx = hash(key, ht);
    HashEl *temp = ht->arr + idx;

    if (!temp->key)
        return NULL;

    while (temp != NULL && strcmp(temp->key, key) != 0) {
        temp = temp->next;
    }
    return temp;
}
