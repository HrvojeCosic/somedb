#pragma once

#include "hash.h"
#include "shared.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 500     // max number of pages in a file
#define START_USER_PAGE 1 // first user page_id (preceding pages are reserved by the system)
#define DBFILES_DIR "db_files"

#define PAGE_NO_HEADER(page) page + sizeof(Header) // Pointer to the page memory after it's header
#define TUPLE_INDEX_TO_POINTER_OFFSET(idx) sizeof(Header) + (idx * sizeof(TuplePtr))
#define PAGE_HEADER(page) (Header *)page // Pointer to the start of page header

// Page header flags
#define COMPACTABLE (1 << 7)

/*
 * NOTE: first page in any database file (page_id = 0) is reserved for a file header including metadata about contents
 * of the database file
 */

/**
 * For a good explanation of this struct, see (under "Page Directory Implementation"):
 * https://cs186berkeley.net/notes/note3/ Current idea is that there is only ever one TablePageDirectory linked list
 * stored in memory and getting updated as needed
 */
typedef struct TablePageDirectory {
    char table_name[30]; // page directory is being tracked for each table (file) in the database
    HashTable *page_directory;
    TablePageDirectory *next; // pointer to the page directory of the next table
} PageDirectory;

typedef struct {
    uint16_t keys_start;   // offset to beginning of page_id array
    uint16_t values_start; // offset to beginning of array of file offsets corresponding to each key
    uint16_t
        entry_num; // current number of entries in the page directory. Useful for knowing the length of keys/values arrs
} PageDirectoryHeader;

typedef struct {
    page_id_t id;
    uint16_t free_start; // offset to beginning of available page memory
    uint16_t free_end;   // offset to end of available page memory
    uint16_t free_total; // total available page memory
    uint8_t flags;
    RWLOCK latch;
} Header;

typedef struct {
    uint16_t start_offset; // offset to the tuple start
    uint16_t size;         // size of the tuple
} TuplePtr;

typedef struct {
    TuplePtr *start; // pointer to the beginning of tuple pointers in a page
    uint16_t length; // number of tuple pointers in a page
} TuplePtrList;

/*
 * Type of argument for add_tuple
 */
typedef struct {
    page_id_t page_id;
    void *tuple;
    uint16_t tuple_size;
} AddTupleArgs;

/*
 * Creates a new disk manager, responsible to manage the database file of given FILENAME
 */
// DiskManager *new_disk_manager(char *filename);

/*
 * Free disk_manager memory allocations
 */
// void destroy_disk_manager(DiskManager *disk_manager);

/*
 * Creates a database file for a TABLE_NAME if it doesn't already exist.
 * Returns a file descriptor of a (new) database file
 */
int table_file(char *table_name);

void close_table_file(char *table_name);

/*
 * Allocates memory for a new page for the provided table and returns its page_id.
 * Returns 0 if page can not be allocated. Returning 0 is fine because its not a valid page id for the user stored data
 */
page_id_t new_page(char *table_name);

/*
 * Returns a pointer to the beginning of the tuple with the given index at the
 * specified page offset, or a null pointer if the tuple does not exist in the given
 * page
 */
void *get_tuple(page_id_t pid, uint16_t tuple_idx);

/*
 * Removes tuple of the specified page at the specified index
 */
void remove_tuple(void *page, uint16_t tuple_idx);

/*
 * Defragments the page by freeing up the memory of removed tuple components.
 * Unsets the COMPACTABLE flag in the header afterwards.
 * Does nothing if the page is not compactable
 */
void defragment(void *page);

/*
 * Writes raw DATA to the offset of PAGE_ID to a database table file of TABLE_NAME
 */
void write_page(page_id_t page_id, char *table_name, void *data);

/*
 * Reads serialized contents of the page of specified page_id into memory and returns a pointer to its beginning.
 * Serialization policy can be found in seriailze.h
 */

uint8_t *read_page(page_id_t page_id, char *table_name);

/*
 * Adds given tuple to the given page and returns a pointer to beginning of
 * added tuple. If the page is full, immediately returns a null pointer
 */
TuplePtr *add_tuple(void *data);

/*
 * Returns information about tuple pointers at the specified page contained in
 * TuplePtrList
 */
TuplePtrList get_tuple_ptr_list(void *page);

/*
 * Currently only used for testing. It just removes the database file of the particular table
 */
void remove_table(char *table_name);
