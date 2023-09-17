#pragma once

#include "../utils/hash.h"
#include "../utils/shared.h"
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 500 // max number of pages in a file
#define DBFILES_DIR "db_files"

// Page-Table key. Page identifier including the page id and filename it belongs to
#define PT_KEY_TABLENAME_LEN 50

typedef struct {
    char *table_name;
    page_id_t pid;
} PTKey;

typedef struct {
    HashTable *page_directory; // table's page directory in memory representation, for saving some file seek expenses
    PageType page_type;        // (usually optional) type of page present in a file handled by disk manager instance.
    char *table_name;
    RWLOCK latch;
} DiskManager;

/*
 * Returns a file descriptor of a database file of given TABLE_NAME
 */
int table_file(const char *table_name);

/*
 * Writes raw data to the offset of pt.pid to a database table file of pt.table_name
 */
void write_page(PTKey pt, void *data);

/*
 * Reads serialized contents of the specified page into
 * memory and returns a pointer to its beginning.
 * Serialization policy can be found in seriailze.h
 */
uint8_t *read_page(PTKey pt);

/*
 * Currently only used for testing. It just removes the database file of the particular table
 */
void remove_table(const char *table_name);

void close_table_file(char *table_name);

PTKey new_ptkey(char table_name[PT_KEY_TABLENAME_LEN], page_id_t pid);

/*
 * Stringify/unstringity a given PTKey in a certain format
 */
char *ptkey_to_str(PTKey ptkey);
PTKey ptkey_from_str(char *ptkey_str);

/* See comment in source file about these btree methods */
page_id_t new_btree_index_page(DiskManager *disk_manager, bool is_leaf);
DiskManager *create_btree_index(const char *idx_name, const u8 max_keys);