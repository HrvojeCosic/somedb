#pragma once

#include "hash.h"
#include "shared.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 500 // max number of pages in a file
#define DBFILES_DIR "db_files"

typedef struct {
    HashTable *page_directory; // table's page directory in memory representation, for saving some file seek expenses
    char *table_name;
    RWLOCK latch;
} DiskManager;

/*
 * Returns a file descriptor of a database file of given TABLE_NAME
 */
int table_file(const char *table_name);

/*
 * Writes raw DATA to the offset of PAGE_ID to a database table file of DISK_MANAGER's table
 */
void write_page(page_id_t page_id, DiskManager *disk_manager, void *data);

/*
 * Reads serialized contents of the page of specified page_id of a table inside disk_manager into
 * memory and returns a pointer to its beginning.
 * Serialization policy can be found in seriailze.h
 */
uint8_t *read_page(page_id_t page_id, DiskManager *disk_manager);

/*
 * Currently only used for testing. It just removes the database file of the particular table
 */
void remove_table(const char *table_name);

void close_table_file(char *table_name);

/* See comment in source file about these btree methods */
page_id_t new_btree_index_page(DiskManager *disk_manager);
DiskManager *create_btree_index(const char *idx_name, const u8 max_keys);
