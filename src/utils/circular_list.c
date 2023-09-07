#include "../../include/utils/circular_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CircularList *circular_list_init(size_t capacity) {
    CircularList *cl = (CircularList *)malloc(sizeof(CircularList));
    cl->head = NULL;
    cl->size = 0;
    cl->capacity = capacity;

    return cl;
}

CircularListNode *circular_list_next(CircularListNode *curr, CircularList *cl) {
    if (cl->size == 0)
        return NULL;

    if (curr->next == NULL)
        return cl->head;

    return curr->next;
}

CircularListNode *circular_list_insert(void *value, CircularList *cl) {
    if (cl->size == cl->capacity || value == NULL)
        return NULL;

    CircularListNode *node = (CircularListNode *)malloc(sizeof(CircularListNode));
    node->value = value;
    node->next = cl->head;

    cl->head = node;
    cl->size++;
    return node;
}

static void free_cl_node(CircularListNode **node) {
    free((*node)->value);
    (*node)->value = NULL;
    free(*node);
    *node = NULL;
}

static bool are_equal(void *val1, void *val2) { return (val1 == val2 || strcmp((char *)val1, (char *)val2) == 0); }

void circular_list_destroy(CircularList **cl) {
    CircularListNode *curr = (*cl)->head;
    while (curr != NULL) {
        CircularListNode *temp = curr;
        curr = curr->next;
        circular_list_remove(temp->value, *cl);
    }
}

bool circular_list_remove(void *node_val, CircularList *cl) {
    if (cl->head == NULL)
        return false;

    if (are_equal(cl->head->value, node_val)) {
        CircularListNode *temp = cl->head;
        cl->head = cl->head->next;
        free_cl_node(&temp);
        cl->size--;
        return true;
    }

    CircularListNode *curr = cl->head->next;
    CircularListNode *prev = cl->head;

    while (curr != NULL && are_equal(cl->head->value, node_val)) {
        prev = curr;
        curr = curr->next;
    }
    prev->next = curr->next;
    free_cl_node(&curr);
    cl->size--;
    return true;
}
