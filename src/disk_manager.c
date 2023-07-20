#include "../include/disk_manager.h"
#include "../include/serialize.h"
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

/**
 * File header includes the page directory which includes:
 * -serialized 32 bit pairs consisting of page id and free space of the page
 */
int table_file(char *table_name) {
    DIR *dbfiles_dir = opendir(DBFILES_DIR);
    if (ENOENT == errno) {
        mkdir(DBFILES_DIR, 0700);
        dbfiles_dir = opendir(DBFILES_DIR);
    }

    char path[40];
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    int fd = open(path, O_CREAT | O_RDWR, 0644);

    // Setup a database file header when creating a new file
    off_t offset = lseek(fd, 0, SEEK_END);
    if (offset == 0) {
        uint8_t pid_buf[2], free_space_buf[2], total_buf[4];
        for (page_id_t pid = 0; pid < MAX_PAGES; pid++) {
            encode_uint16(pid, pid_buf);
            encode_uint16(PAGE_SIZE, free_space_buf);
            memcpy(total_buf, pid_buf, 2);
            memcpy(total_buf + 2, free_space_buf, 2);
            write(fd, total_buf, 4);
        }
    }
    return fd;
}

// test only
void remove_table(char *table_name) {
    char path[40];
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    int r = remove(path);
}

void close_table_file(char *table_name) { close(table_file(table_name)); }

static TablePageDirectory *pdm_instance_ = NULL;

static TablePageDirectory *page_directory_manager_instance() {
    // if it already exists, return instance, otherwise initialize page directory manager
    if (pdm_instance_ != NULL)
        return pdm_instance_;

    DIR *directory;
    struct dirent *entry;
    directory = opendir(DBFILES_DIR);

    TablePageDirectory *head = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type != DT_REG)
            continue;

        char path[50] = {0};
        sprintf(path, "%s/%s", DBFILES_DIR, entry->d_name);
        char *table_name = strtok(entry->d_name, ".");

        int fd = open(path, O_RDONLY);
        void *buf = malloc(PAGE_SIZE);
        read(fd, buf, PAGE_SIZE);

        TablePageDirectory *table_page_dir = (TablePageDirectory *)malloc(sizeof(TablePageDirectory));
        table_page_dir->next = head;
        table_page_dir->page_directory = init_hash(MAX_PAGES);
        strcpy(table_page_dir->table_name, table_name);

        uint8_t *page_dir = read_page(0, table_name);
        for (uint16_t i = START_USER_PAGE; i < MAX_PAGES; i++) {
            char *pid_key = (char *)malloc(sizeof(char) * 11);
            sprintf(pid_key, "%u", i);

            uint16_t *free_space = (uint16_t *)malloc(sizeof(size_t));
            *free_space = decode_uint16(page_dir + (i * 4) + 2);

            hash_insert(pid_key, free_space, table_page_dir->page_directory);
        }

        head = table_page_dir;
        close(fd);
        free(buf);
    }
    pdm_instance_ = head;
    closedir(directory);
    return pdm_instance_;
}

/**
 * Finds page in the table file that has enough space to store the provided size.
 * Returns true if found, false otherwise.
 * Found page_id is stored in the provided out param
 */
static bool find_spacious_page(size_t size_needed, TablePageDirectory *table_page_dir, page_id_t *page_id) {
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

page_id_t new_page(char *table_name) {
    void *page = malloc(PAGE_SIZE);
    RWLOCK latch;
    RWLOCK_INIT(&latch);

    TablePageDirectory *curr = page_directory_manager_instance();
    while (strcmp(curr->table_name, table_name) != 0) {
        if (curr->next == NULL)
            return 0;
        curr = curr->next;
    }

    Header *header = PAGE_HEADER(page);
    if (!find_spacious_page(PAGE_SIZE, curr, &header->id))
        return 0;
    header->free_end = PAGE_SIZE - 1;
    header->free_start = sizeof(Header);
    header->free_total = header->free_end - header->free_start;
    header->latch = latch;
    write_page(header->id, table_name, page);

    return header->id;
}

// TuplePtr *add_tuple(void *data) {
//     AddTupleArgs *data_args = (AddTupleArgs *)data;
//     page_id_t pid = data_args->page_id;
//     uint16_t tuple_size = data_args->tuple_size;
//     void *tuple = data_args->tuple;
//     DiskManager *disk_manager = data_args->disk_manager;
//
//     void *page = read_page(pid, disk_manager);
//     Header *header = PAGE_HEADER(page);
//     //RWLOCK_WRLOCK(&header->latch);
//     if (header->free_total < tuple_size + sizeof(TuplePtr)) {
//         printf("Page is full, tuple insertion failed.\n");
//         return NULL;
//     }
//
//     TuplePtr *tuple_ptr = (TuplePtr *)((uintptr_t *)page + header->free_start);
//     tuple_ptr->start_offset = header->free_end - tuple_size;
//     tuple_ptr->size = tuple_size;
//
//     void *tuple_start = (uintptr_t *)page + tuple_ptr->start_offset;
//     memcpy(tuple_start, tuple, tuple_size);
//
//     header->free_start += sizeof(TuplePtr);
//     header->free_end -= tuple_ptr->size;
//     header->free_total = header->free_end - header->free_start;
//
//     write_page(pid, page, disk_manager);
//     //RWLOCK_UNLOCK(&header->latch);
//     return tuple_ptr;
// }
//
// void remove_tuple(void *page, uint16_t tuple_idx) {
//     TuplePtr *tuple_ptr = (TuplePtr *)((uintptr_t *)page + TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx));
//     Header *page_header = PAGE_HEADER(page);
//     page_header->flags |= COMPACTABLE;
//
//     tuple_ptr->start_offset = 0;
// }
//
// void *get_tuple(page_id_t pid, uint16_t tuple_idx, DiskManager *disk_manager) {
//     void *page = read_page(pid, disk_manager);
//     TuplePtr *tuple_ptr = (TuplePtr *)((uintptr_t *)page + TUPLE_INDEX_TO_POINTER_OFFSET(tuple_idx));
//
//     if (tuple_ptr->start_offset == 0) {
//         return NULL;
//     }
//
//     return (uintptr_t *)page + tuple_ptr->start_offset;
// }
//
// TuplePtrList get_tuple_ptr_list(void *page) {
//     Header *header = PAGE_HEADER(page);
//     TuplePtrList list;
//     list.start = (TuplePtr *)((uintptr_t *)page + sizeof(Header));
//     list.length = (header->free_start - sizeof(Header)) / sizeof(TuplePtr);
//     return list;
// }
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
void write_page(page_id_t page_id, char *table_name, void *data) {
    char pid_str[11];
    sprintf(pid_str, "%d", page_id);

    Header *page_header = PAGE_HEADER(data);

    int fd = table_file(table_name);
    int s = lseek(fd, page_header->id * PAGE_SIZE, SEEK_SET);
    int w = write(fd, data, PAGE_SIZE);
    int f = fsync(fd);

    if (s == -1 || w == -1 || f == -1) {
        printf("I/O error while writing page\n");
        return;
    }
}

uint8_t *read_page(page_id_t page_id, char *table_name) {
    uint8_t *page = (uint8_t *)malloc(PAGE_SIZE);

    int fd = table_file(table_name);

    int offset = page_id * PAGE_SIZE;
    off_t fsize = lseek(fd, 0, SEEK_END);
    if (offset >= fsize) {
        printf("I/O error, reading past EOF\n");
        exit(1);
    }

    int s = lseek(fd, offset, SEEK_SET);
    int r = read(fd, page, PAGE_SIZE);

    if (s == -1 || r == -1) {
        printf("I/O error while reading specified page\n");
        exit(1);
    }

    if (r < PAGE_SIZE) {
        printf("Read less than a page size\n");
    }

    return page;
}
