#include "../../include/disk/disk_manager.h"
#include "../../include/disk/bpm.h"
#include "../../include/utils/serialize.h"
#include "../../include/utils/shared.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int table_file(const char *table_name) {
    opendir(DBFILES_DIR);
    if (ENOENT == errno) {
        mkdir(DBFILES_DIR, 0700);
        opendir(DBFILES_DIR);
    }

    char path[40];
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    return fd;
}

void write_page(PTKey pt, void *data) {
    char pid_str[11];
    sprintf(pid_str, "%d", pt.pid);

    int fd = table_file(pt.table_name);
    int l = lseek(fd, pt.pid * PAGE_SIZE, SEEK_SET);
    int w = write(fd, data, PAGE_SIZE);
    int f = fsync(fd);

    if (l == -1 || w == -1 || f == -1) {
        printf("I/O error while writing page\n");
        return;
    }
}

uint8_t *read_page(PTKey pt) {
    uint8_t *page = (uint8_t *)malloc(PAGE_SIZE);

    int fd = table_file(pt.table_name);

    int offset = pt.pid * PAGE_SIZE;
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

void close_table_file(DiskManager *disk_manager) { close(table_file(disk_manager->table_name)); }

// for test only
void remove_table(const char *table_name) {
    char path[40] = {0};
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    remove(path);
}

// Warning we're just using the fixed PT_KEY_TABLENAME_LEN for the table_name length
// which could lead to buffer overflows, but we'll just make sure it's never above that
PTKey new_ptkey(char *table_name, page_id_t pid) {
    PTKey key{};
    key.pid = pid;
    key.table_name = (char *)malloc(PT_KEY_TABLENAME_LEN);
    memcpy(key.table_name, table_name, PT_KEY_TABLENAME_LEN);
    return key;
};

char *ptkey_to_str(PTKey ptkey) {
    char *str = (char *)malloc(PT_KEY_TABLENAME_LEN + sizeof(page_id_t));
    sprintf(str, "%s__%d", ptkey.table_name, ptkey.pid);
    return str;
};

PTKey ptkey_from_str(char *ptkey_str) {
    PTKey ptkey = PTKey{};
    sscanf(ptkey_str, "%s__%d", ptkey.table_name, &ptkey.pid);
    return ptkey;
};

// B+tree disk handling methods used by buffer pool, which is in C
// (so I don't have to deal with linking complications that would arise in test files from declaring these in index.cpp)
// probably solve this properly if there are more issues like this in the future, but this will do for now
//-------------------------------------------------------------------------------------------------
page_id_t new_btree_index_page(DiskManager *disk_manager, bool is_leaf) {
    u8 *metadata_page = read_page(new_ptkey(disk_manager->table_name, 0));
    assert(decode_uint32(metadata_page) == BTREE_INDEX);

    u16 curr_node_num = decode_uint16(metadata_page + NODE_COUNT_OFFSET);
    u16 new_node_num = curr_node_num + 1;
    encode_uint16(new_node_num, metadata_page + NODE_COUNT_OFFSET);
    write_page(new_ptkey(disk_manager->table_name, 0), metadata_page);

    u8 *page = (u8 *)calloc(PAGE_SIZE, 1);
    memcpy(page + IS_LEAF_OFFSET, &is_leaf, IS_LEAF_SIZE);
    write_page(new_ptkey(disk_manager->table_name, new_node_num), page);

    return new_node_num;
}

DiskManager *create_btree_index(const char *idx_name, const u8 max_keys) {
    DiskManager *disk_mgr = (DiskManager *)malloc(sizeof(DiskManager));
    disk_mgr->table_name = (char *)malloc(sizeof(char) * 25);
    strcpy(disk_mgr->table_name, idx_name);
    RWLOCK l;
    RWLOCK_INIT(&l);
    disk_mgr->latch = l;

    int fd = table_file(disk_mgr->table_name);

    off_t offset = lseek(fd, 0, SEEK_END);
    if (offset != 0) {
        printf("Index with that name already exists");
        close(fd);
        return NULL;
    }

    u8 index_buf[PAGE_SIZE] = {0};
    encode_uint32(BTREE_INDEX, index_buf);
    encode_uint16(0, index_buf + sizeof(u32));
    memcpy(index_buf + sizeof(u32) + sizeof(u16), &max_keys, 1);

    write(fd, index_buf, PAGE_SIZE);
    return disk_mgr;
}

//-------------------------------------------------------------------------------------------------
