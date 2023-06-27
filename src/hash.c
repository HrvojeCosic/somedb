#include "../include/hash.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashTable *init_hash(uint8_t size) {
    HashTable *ht = malloc(sizeof(HashTable));
    ht->size = size;
    ht->arr = calloc(sizeof(HashEl), size);
    return ht;
}

void destroy_hash(HashTable **ht) {
    if (ht == NULL || *ht == NULL)
        return;

    for (uint8_t i = 0; i < (*ht)->size; i++) {
        HashEl *curr = (*ht)->arr + i;
        HashEl *temp = NULL;

        while (curr != NULL && curr->key != NULL) {
            temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
    free((*ht)->arr);
    (*ht)->arr = NULL;
    free(*ht);
    *ht = NULL;
}

/**
 * hashing function
 * To be thought about
 */
static uint8_t hash(const char *key, HashTable *ht) {
    return atoi(key) % ht->size;
}

void hash_insert(const char *key, void *data, HashTable *ht) {
    if (key == NULL || data == NULL)
        return;
    uint8_t idx = hash(key, ht);

    HashEl *el = malloc(sizeof(HashEl));
    el->key = key;
    el->data = data;
    el->next = NULL;

    if (ht->arr[idx].key == NULL) {
        ht->arr[idx] = *el;
    } else {
        HashEl *temp = &ht->arr[idx];
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = el;
    }
}

void hash_remove(const char *key, HashTable *ht) {
    uint8_t idx = hash(key, ht);
    HashEl *temp = ht->arr + idx;
    HashEl *prev = NULL;

    while (temp != NULL && strcmp(temp->key, key) != 0) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
        return;

    if (prev == NULL) {           // if its first el. in LL
        if (temp->next == NULL) { // if its the only el. in LL
            ht->arr[idx] = (HashEl){0};
            free(temp);
        } else {
            HashEl *next = temp->next;
            free(temp);
            ht->arr[idx] = *next;
            free(next);
        }
    } else {
        prev->next = temp->next;
        free(temp);
    }
}

HashEl *hash_find(const char *key, HashTable *ht) {
    uint8_t idx = hash(key, ht);
    HashEl *temp = ht->arr + idx;

    if (!temp->key)
        return NULL;

    while (temp != NULL && strcmp(temp->key, key) != 0) {
        temp = temp->next;
    }
    return temp;
}
