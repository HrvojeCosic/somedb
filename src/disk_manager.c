#include "../include/disk_manager.h"
#include "../include/serialize.h"
#include "../include/shared.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static PageDirectory *pdm_instance_ = NULL;

void close_table_file(char *table_name) { close(table_file(table_name)); }

// TODO: make it thread safe
static PageDirectory *page_directory_manager_instance() {
    // if it already exists, return instance, otherwise initialize page directory manager
    if (pdm_instance_ != NULL)
        return pdm_instance_;

    DIR *directory;
    struct dirent *entry;
    directory = opendir(DBFILES_DIR);

    PageDirectory *head = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type != DT_REG)
            continue;

        char path[50] = {0};
        sprintf(path, "%s/%s", DBFILES_DIR, entry->d_name);
        char *table_name = strtok(entry->d_name, ".");

        int fd = open(path, O_RDONLY);
        void *buf = malloc(PAGE_SIZE);
        read(fd, buf, PAGE_SIZE);

        PageDirectory *table_page_dir = (PageDirectory *)malloc(sizeof(PageDirectory));
        table_page_dir->next = head;
        table_page_dir->page_directory = init_hash(MAX_PAGES);
        strcpy(table_page_dir->table_name, table_name);

        uint8_t *page_dir = read_page(0, table_name);
        for (uint16_t i = START_USER_PAGE; i < MAX_PAGES; i++) {
            char *pid_key = (char *)malloc(sizeof(char) * 11);
            sprintf(pid_key, "%u", i);

            uint16_t *free_space = (uint16_t *)malloc(sizeof(size_t));
            *free_space = decode_uint16(page_dir + (i * 4) + 2);

	    HashInsertArgs in_args = {.key = pid_key, .data = free_space, .ht = table_page_dir->page_directory };
	    hash_insert(&in_args);
        }

        head = table_page_dir;
        close(fd);
        free(buf);
    }
    pdm_instance_ = head;
    closedir(directory);
    return pdm_instance_;
}

Header extract_header(uint8_t *page, page_id_t page_id) {
    uint16_t free_start = decode_uint16(page);
    uint16_t free_end = decode_uint16(page + sizeof(uint16_t));
    uint16_t free_total = free_end - free_start;
    Header header = {.id = page_id,
                     .free_start = free_start,
                     .free_end = free_end,
                     .free_total = free_total,
                     .flags = *(page + (sizeof(uint16_t) * 2))};

    return header;
}

TuplePtr extract_tuple_ptr(uint8_t *page, uint32_t slot_num) {
    TuplePtr data;

    uint8_t tuple_ptr_buf[TUPLE_PTR_SIZE];
    memcpy(tuple_ptr_buf, page + TUPLE_INDEX_TO_TUPLE_POINTER_OFFSET(slot_num), sizeof(uint16_t));
    data.start_offset = decode_uint16(tuple_ptr_buf);
    data.size = decode_uint16(tuple_ptr_buf + sizeof(uint16_t));

    return data;
}

static void construct_page_header_buf(uint8_t *page, Header header) {
    encode_uint16(header.free_start, page);
    encode_uint16(header.free_end, page + sizeof(uint16_t));
    page[sizeof(uint16_t) * 2] = header.flags;
}

int table_file(const char *table_name) {
    DIR *dbfiles_dir = opendir(DBFILES_DIR);
    if (ENOENT == errno) {
        mkdir(DBFILES_DIR, 0700);
        dbfiles_dir = opendir(DBFILES_DIR);
    }

    char path[40];
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    return fd;
}

void create_table(const char *table_name, Column *columns, uint8_t n_columns) {
    int fd = table_file(table_name);

    off_t offset = lseek(fd, 0, SEEK_END);
    if (offset != 0) {
        printf("Table with that name already exists");
        return;
    }

    // Setup a database file header
    uint8_t pid_buf[2], free_space_buf[2], total_buf[4];
    for (page_id_t pid = 0; pid < MAX_PAGES; pid++) {
        encode_uint16(pid, pid_buf);
        encode_uint16(PAGE_SIZE, free_space_buf);
        memcpy(total_buf, pid_buf, 2);
        memcpy(total_buf + 2, free_space_buf, 2);
        write(fd, total_buf, 4);
    }

    // Add padding to the header page up to PAGE_SIZE (necessary for reading page into memory)
    const uint16_t remainder = PAGE_SIZE - (MAX_PAGES * 4);
    uint8_t fill_buf[remainder];
    for (int i = 0; i < remainder; i++)
        fill_buf[i] = 0;
    write(fd, fill_buf, remainder);

    // Add table columns metadata page
    uint8_t col_buf[PAGE_SIZE] = {n_columns};
    size_t col_size = 0;
    for (uint8_t j = 0; j < n_columns; j++) {
        uint8_t *buf_offset = col_buf + START_COLUMNS_INFO + (j * col_size);
        memcpy(buf_offset, &columns[j].name_len, 1);
        memcpy(buf_offset + 1, columns[j].name, columns[j].name_len);
        memcpy(buf_offset + 1 + columns[j].name_len, &columns[j].type, 1);
        col_size = SCHEMA_COLUMN_SIZE(columns[j].name_len);
    }
    write(fd, col_buf, PAGE_SIZE);

    // Add table to page directory manager
    PageDirectory *new_page_dir = (PageDirectory *)malloc(sizeof(PageDirectory));
    strcpy(new_page_dir->table_name, table_name);
    new_page_dir->page_directory = init_hash(MAX_PAGES);

    for (uint16_t i = START_USER_PAGE; i < MAX_PAGES; i++) {
        char *pid_key = (char *)malloc(sizeof(char) * 11);
        sprintf(pid_key, "%u", i);

        uint16_t *free_space = (uint16_t *)malloc(sizeof(size_t));
        *free_space = PAGE_SIZE;
	HashInsertArgs in_args = {.key = pid_key, .data = free_space, .ht = new_page_dir->page_directory };
        hash_insert(&in_args);
    }

    new_page_dir->next = page_directory_manager_instance();
    pdm_instance_ = new_page_dir;
    lseek(fd, 0, SEEK_SET);
}

// for test only
void remove_table(const char *table_name) {
    char path[40];
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    int r = remove(path);
}

/**
 * Finds page in the table file that has enough space to store the provided size.
 * Returns true if found, false otherwise.
 * Found page_id is stored in the provided out param
 */
static bool find_spacious_page(uint16_t size_needed, PageDirectory *table_page_dir, page_id_t *page_id) {
    for (page_id_t i = START_USER_PAGE; i < MAX_PAGES; i++) {
        char pid_key[11];
        sprintf(pid_key, "%d", i);
        HashEl *found = hash_find(pid_key, table_page_dir->page_directory);
        if (found && (size_t)found->data >= size_needed) {
            *page_id = i;
            return true;
        }
    }
    return false;
}

// returns table's page directory or NULL if couldn't find it
static PageDirectory *find_page_dir(const char *table_name) {
    PageDirectory *curr = page_directory_manager_instance();
    while (strcmp(curr->table_name, table_name) != 0) {
        curr = curr->next;
        if (!curr)
            return NULL;
    }
    return curr;
}

page_id_t new_page(const char *table_name) {
    uint8_t *page = (uint8_t *)malloc(PAGE_SIZE);
    page_id_t pid = 0;
    if (!find_spacious_page(PAGE_SIZE, find_page_dir(table_name), &pid))
        return 0;

    // Construct page header
    Header header = {.free_start = PAGE_HEADER_SIZE, .free_end = PAGE_SIZE - 1, .flags = 0x00};
    header.free_total = header.free_end - header.free_start;
    construct_page_header_buf(page, header);

    write_page(pid, table_name, page);
    free(page);
    return pid;
}

// Updates the free_space associated with the provided page id in a table's page directory (in memory and on disk)
static void update_page_dir(const char *table_name, page_id_t pid, uint16_t tuple_size, enum TupleAction act) {
    char *pid_key = (char *)malloc(sizeof(char) * 11);
    sprintf(pid_key, "%u", pid);
    PageDirectory *page_dir = find_page_dir(table_name);
    if (page_dir == NULL) return;

    // TODO: make find->remove->insert a single procedure
    uint16_t curr_free_space = *(uint16_t *)hash_find(pid_key, page_dir->page_directory)->data;
    HashRemoveArgs rm_args = {.key = pid_key, .ht = page_dir->page_directory};
    hash_remove(&rm_args);
    uint16_t *free_space = (uint16_t *)malloc(sizeof(uint16_t));
    *free_space = act == TUPLE_ADD ? curr_free_space - tuple_size : curr_free_space + tuple_size;
    HashInsertArgs in_args = {.key = pid_key, .data = free_space, .ht = page_dir->page_directory };
    hash_insert(&in_args);

    int fd = table_file(table_name);
    uint8_t pid_buf[4], free_space_buf[4], total_buf[4];
    encode_uint16(pid, pid_buf);
    encode_uint16(*free_space, free_space_buf);
    memcpy(total_buf, pid_buf, 2);
    memcpy(total_buf + 2, free_space_buf, 2);
    pwrite(fd, total_buf, 4, PID_TO_PAGE_DIRECTORY_OFFSET(pid));
}

void add_tuple(void *data_args) {
    AddTupleArgs *data = (AddTupleArgs *)data_args;

    // TODO: get tuple length and create tuple buffer in one pass
    uint16_t tuple_size = 0;
    for (uint8_t i = 0; i < data->num_columns; i++) {
        switch (data->column_types[i]) {
        case STRING: {
            const char *str = data->column_values[i].string_value;
            uint16_t size = strlen(str);
            tuple_size += (sizeof(uint16_t) + size);
            break;
        }
        case UINT16:
            tuple_size += sizeof(uint16_t);
            break;
        case UINT32:
            tuple_size += sizeof(uint32_t);
            break;
        }
    }

    uint8_t tuple_buf[tuple_size];
    uint16_t tuple_buf_offset = 0;
    for (uint8_t i = 0; i < data->num_columns; i++) {
        switch (data->column_types[i]) {
        case STRING: {
            const char *str = data->column_values[i].string_value;
            uint16_t size = strlen(str);
            encode_uint16(size, tuple_buf + tuple_buf_offset);
            memcpy(tuple_buf + tuple_buf_offset + sizeof(uint16_t), str, size);
            tuple_buf_offset += (sizeof(uint16_t) + size);
            break;
        }
        case UINT16: {
            encode_uint16(data->column_values[i].uint16_value, tuple_buf + tuple_buf_offset);
            tuple_buf_offset += sizeof(uint16_t);
            break;
        }
        case UINT32:
            encode_uint32(data->column_values[i].uint32_value, tuple_buf + tuple_buf_offset);
            tuple_buf_offset += sizeof(uint32_t);
            break;
        }
    }

    page_id_t *pid = (page_id_t *)malloc(sizeof(page_id_t));
    find_spacious_page(tuple_size, find_page_dir(data->table_name), pid);
    if (!pid) {
        printf("Couldn't find available page"); // this can be solved with overflow pages
        return;
    }
    uint8_t *page = read_page(*pid, data->table_name);
    Header header = extract_header(page, *pid);

    // Extract schema info
    uint8_t *schema = read_page(1, data->table_name);
    uint8_t cols_num = *schema + 0;
    schema = schema + START_COLUMNS_INFO;

    uint8_t col_name_len_buf[1];
    uint8_t data_type_buf[1];
    int schema_curr_offset = 0;

    for (uint8_t i = 0; i < cols_num; i++) {
        memcpy(col_name_len_buf, schema + schema_curr_offset, 1);
        schema_curr_offset = i * (1 + *col_name_len_buf + 1);

        uint8_t col_name_len = schema[schema_curr_offset];
        char col_name_buf[col_name_len];
        char *col_name = (char *)schema + schema_curr_offset + 1;
        memcpy(col_name_buf, col_name, col_name_len);

        uint8_t *col_type = schema + schema_curr_offset + col_name_len + 1;
        memcpy(data_type_buf, col_type, 1);

        // Validate columns
        if (data->num_columns != cols_num) {
            fprintf(stderr, "Provided number of columns does not match the schema\n");
            return;
        }
        if (strncmp(col_name_buf, data->column_names[i], col_name_len) != 0) {
            fprintf(stderr, "Provided column name '%s' does not match its schema counterpart '%s'\n",
                    data->column_names[i], col_name_buf);
            return;
        }
        if (data->column_types[i] != *data_type_buf) {
            fprintf(stderr, "Provided type '%d' does not match its schema counterpart '%d'", data->column_types[i],
                    *data_type_buf);
            return;
        }
    }

    // Write the tuple out
    uint16_t start_off = header.free_end - tuple_size;
    TuplePtr tuple_ptr = {.start_offset = start_off, .size = tuple_size};
    uint8_t tuple_ptr_buf[TUPLE_PTR_SIZE];
    encode_uint16(tuple_ptr.start_offset, tuple_ptr_buf);
    encode_uint16(tuple_ptr.size, tuple_ptr_buf + sizeof(uint16_t));
    memcpy(page + header.free_start, tuple_ptr_buf, TUPLE_PTR_SIZE);

    memcpy(page + header.free_end - tuple_size, tuple_buf, tuple_size);

    header.free_start += TUPLE_PTR_SIZE;
    header.free_end -= tuple_size;
    header.free_total = header.free_end - header.free_start;
    construct_page_header_buf(page, header);

    write_page(*pid, data->table_name, page);
    update_page_dir(data->table_name, *pid, tuple_size, TUPLE_ADD);

    data->tup_ptr_out->size = tuple_ptr.size;
    data->tup_ptr_out->start_offset = tuple_ptr.start_offset;
    free(pid);
}

// void remove_tuple(void *page, uint16_t tuple_idx) {
//     TuplePtr *tuple_ptr = (TuplePtr *)((uintptr_t *)page + TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx));
//     Header *page_header = PAGE_HEADER(page);
//     page_header->flags |= COMPACTABLE;
//
//     tuple_ptr->start_offset = 0;
// }

uint8_t *get_tuple(RID rid, const char *table_name) {
    uint8_t *page = read_page(rid.pid, table_name);
    TuplePtr tuple_ptr = extract_tuple_ptr(page, rid.slot_num);

    if (tuple_ptr.start_offset == 0) {
        return NULL;
    }

    return page + tuple_ptr.start_offset;
}

//
// void defragment(void *page, DiskManager *disk_manager) {
//     Header *page_header = PAGE_HEADER(page);
//
//     if ((page_header->flags & COMPACTABLE) == 0) {
//         return;
//     }
//
//     page_id_t pid = new_page(disk_manager);
//     void *temp = read_page(pid, disk_manager);
//     Header *temp_header = PAGE_HEADER(temp);
//
//     TuplePtrList tuple_ptr_list = get_tuple_ptr_list(page);
//
//     TuplePtr *curr_tuple_ptr;
//     for (int i = 0; i < tuple_ptr_list.length; i++) {
//         curr_tuple_ptr = tuple_ptr_list.start + i;
//
//         // Removed tuples' offsets are 0
//         if (curr_tuple_ptr->start_offset != 0) {
//             AddTupleArgs t_args = {.page_id = pid,
//                                    .tuple = (uintptr_t *)page + curr_tuple_ptr->start_offset,
//                                    .tuple_size = curr_tuple_ptr->size};
//             add_tuple(&t_args);
//         }
//     }
//     page_header->free_start = temp_header->free_start;
//     page_header->free_end = temp_header->free_end;
//     page_header->free_total = temp_header->free_total;
//     page_header->flags &= ~COMPACTABLE;
//
//     memcpy((uintptr_t *)PAGE_NO_HEADER(page), (uintptr_t *)PAGE_NO_HEADER(temp), PAGE_SIZE - sizeof(Header));
//     free(temp);
// }
//
void write_page(page_id_t page_id, const char *table_name, void *data) {
    char pid_str[11];
    sprintf(pid_str, "%d", page_id);

    Header *page_header = PAGE_HEADER(data);

    int fd = table_file(table_name);
    int l = lseek(fd, page_id * PAGE_SIZE, SEEK_SET);
    int w = write(fd, data, PAGE_SIZE);
    int f = fsync(fd);

    if (l == -1 || w == -1 || f == -1) {
        printf("I/O error while writing page\n");
        return;
    }
}

uint8_t *read_page(page_id_t page_id, const char *table_name) {
    uint8_t *page = (uint8_t *)malloc(PAGE_SIZE);

    int fd = table_file(table_name);

    int offset = page_id * PAGE_SIZE;
    off_t fsize = lseek(fd, 0, SEEK_END);
    if (offset >= fsize) {
        fprintf(stderr, "I/O error, reading past EOF\n");
        exit(1);
    }

    int s = lseek(fd, offset, SEEK_SET);
    int r = read(fd, page, PAGE_SIZE);

    if (s == -1 || r == -1) {
        fprintf(stderr, "I/O error while reading specified page\n");
        exit(1);
    }

    if (r < PAGE_SIZE)
        fprintf(stdout, "Read less than a page size\n");

    return page;
}
