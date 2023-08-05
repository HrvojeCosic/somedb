#pragma once

#include <pthread.h>
#include <stdint.h>

#define RWLOCK pthread_rwlock_t
#define RWLOCK_INIT(l) pthread_rwlock_init(l, NULL)
#define RWLOCK_RDLOCK(l) pthread_rwlock_rdlock(l)
#define RWLOCK_WRLOCK(l) pthread_rwlock_wrlock(l)
#define RWLOCK_UNLOCK(l) pthread_rwlock_unlock(l)

#define FRAME(f) *(frame_id_t *)f

typedef uint32_t page_id_t;
typedef uint32_t frame_id_t;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

enum PageType { HEAP_PAGE = 1, BTREE_INDEX_PAGE = 2 };

/*
 * Record id (RID) is a unique identifier for a record with information necessary to
 * find the corresponding page and the offset in that page using the slot number
 */
#define RID_SIZE 8

typedef struct {
    page_id_t pid;
    uint32_t slot_num;
} RID;
