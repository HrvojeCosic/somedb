#pragma once

#include "hash.h"
#include "shared.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 500 // max number of pages in a file
#define DBFILES_DIR "db_files"
/*
 * START_USER_PAGE is the first user page_id. Preceding pages are reserved by the system:
 * page 0 - page directory
 * page 1 - table schema
 */
#define START_USER_PAGE 2
/*
 * START_COLUMNS_INFO is the first byte that contains information about the first column.
 * Preceeding bytes are reserved for table metadata:
 * byte 0 - number of table columns
 */
#define START_COLUMNS_INFO 1

#define PAGE_NO_HEADER(page) page + sizeof(Header) // Pointer to the page memory after it's header
#define TUPLE_INDEX_TO_TUPLE_POINTER_OFFSET(idx) PAGE_HEADER_SIZE + (idx * TUPLE_PTR_SIZE)
#define PAGE_HEADER(page) (Header *)page // Pointer to the start of page header
#define PID_TO_PAGE_DIRECTORY_OFFSET(pid) pid * 2
#define SCHEMA_COLUMN_SIZE(col_name_len)                                                                               \
    (1 + col_name_len + 1) // column_name_length + column_name_string + column_data_type

// Page header flags
#define COMPACTABLE (1 << 7)

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

enum TupleAction { TUPLE_ADD = 1, TUPLE_REMOVE = 2 };

enum ColumnType { UINT32 = 0x01, UINT16 = 0x02, STRING = 0x03 };

typedef union {
    uint32_t uint32_value;
    uint16_t uint16_value;
    const char *string_value;
} ColumnValue;

typedef struct {
    uint8_t name_len; // length of column name
    char *name;       // should not include null character
    ColumnType type;
} Column;

typedef struct {
    const char *table_name;
    const char **column_names;
    ColumnValue *column_values;
    ColumnType *column_types;
    uint8_t num_columns;
} AddTupleArgs;

/*
 * Creates a database table with provided COLUMNS if it doesn't already exist under the same TABLE_NAME.
 *
 * Table database file header consists of multiple metadata pages:
 *  1."page directory" page of the following layout:
 *   -32 bit (2x16) pairs consisting of page id and free space of the page
 *  2.table columns metadata page of the following layout:
 *   -first byte in the paae represents the number of columns in the table
 *   -triplets of data (column_name_length (8bit uint), column_name_string (variable string), column_data_type (8bit
 * uint)) All data is serialized as described in serialize.h
 */
void create_table(const char *table_name, Column *columns, uint8_t n_columns);

/*
 * Returns a file descriptor of a database file of given TABLE_NAME
 */
int table_file(const char *table_name);

/*
 * Allocates memory for a new page for the provided table and returns its page_id.
 * Returns 0 if page can not be allocated. Returning 0 is fine because its not a valid page id for the user stored data
 *
 * Each page is layed out on disk in the "slotted page" format:
 * 1.Page header:
 *  -16 bit unsigned integer containing page offset to the beginning of available space
 *  -16 bit unsigned integer containing page offset to the end of available space
 *  -8 bit unsigned integer containing special page header flags
 * 2.Tuple pointer list consisting of 32 bit pairs (2x16) of the following layout:
 *  -16 bit unsigned integer representing the page offset to the corresponding tuple
 *  -16 bit unsigned integer representing the tuple length
 * 3.Tuple list containing stored data
 * All data is serialized as described in serialize.h
 */
#define PAGE_HEADER_SIZE 5
#define TUPLE_PTR_SIZE 4
page_id_t new_page(const char *table_name);

/*
 * Takes in page bytes and returns a constructed header of the page
 */
Header extract_header(uint8_t *page, page_id_t page_id);

/*
 * Returns a pointer to the beginning of raw tuple data with the given RECORD_ID
 * or a null pointer if the tuple does not exist in the page id provided in RID
 */
uint8_t *get_tuple(RID rid, const char *table_name);

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
void write_page(page_id_t page_id, const char *table_name, void *data);

/*
 * Reads serialized contents of the page of specified page_id into memory and returns a pointer to its beginning.
 * Serialization policy can be found in seriailze.h
 */

uint8_t *read_page(page_id_t page_id, const char *table_name);

/*
 * Adds tuple to a page in the provided table and returns a pointer to beginning of
 * added tuple. If the page is full, immediately returns a null pointer
 */
TuplePtr *add_tuple(void *data_args);

/*
 * Returns information about tuple pointers at the specified page contained in
 * TuplePtrList
 */
TuplePtrList get_tuple_ptr_list(void *page);

/*
 * Currently only used for testing. It just removes the database file of the particular table
 */
void remove_table(const char *table_name);

void close_table_file(char *table_name);
