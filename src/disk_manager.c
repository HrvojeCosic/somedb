#include "../include/disk_manager.h"
#include "../include/serialize.h"
#include "../include/shared.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

void write_page(page_id_t page_id, DiskManager *disk_manager, void *data) {
    char pid_str[11];
    sprintf(pid_str, "%d", page_id);

    int fd = table_file(disk_manager->table_name);
    int l = lseek(fd, page_id * PAGE_SIZE, SEEK_SET);
    int w = write(fd, data, PAGE_SIZE);
    int f = fsync(fd);

    if (l == -1 || w == -1 || f == -1) {
        printf("I/O error while writing page\n");
        return;
    }
}

uint8_t *read_page(page_id_t page_id, DiskManager *disk_manager) {
    uint8_t *page = (uint8_t *)malloc(PAGE_SIZE);

    int fd = table_file(disk_manager->table_name);

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

void close_table_file(DiskManager *disk_manager) { close(table_file(disk_manager->table_name)); }

// for test only
void remove_table(const char *table_name) {
    char path[40] = {0};
    sprintf(path, "%s/%s.db", DBFILES_DIR, table_name);
    remove(path);
}
