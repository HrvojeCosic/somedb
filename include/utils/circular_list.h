#include <stdbool.h>
#include <stddef.h>

typedef struct CircularListNode {
    void *value;
    struct CircularListNode *next;
} CircularListNode;

typedef struct {
    size_t capacity; // max list element number
    size_t size;     // current list element number
    CircularListNode *head;

} CircularList;

/**
 * Initializes a new circular list and returns a pointer to it
 */
CircularList *circular_list_init(size_t capacity);

/**
 * Given the current CL node, returns the next one. Loops around to the
 * beginning if necessary
 */
CircularListNode *circular_list_next(CircularListNode *curr, CircularList *cl);

/**
 * Creates a new node in the circular list with the specified value.
 * Returns a pointer to it, or a null pointer if node cannot be added
 */
CircularListNode *circular_list_insert(void *value, CircularList *cl);

/**
 * Removes a node of given value from the CL.
 * Returns false if it couldn't find the node with such value, true otherwise
 */
bool circular_list_remove(void *node_val, CircularList *cl);

/*
 * Frees all memory allocated for circular list CL
 */
void circular_list_destroy(CircularList **cl);
