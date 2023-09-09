#pragma once

#include <pthread.h>
#include <stdint.h>

#define RWLOCK pthread_rwlock_t
#define RWLOCK_INIT(l) pthread_rwlock_init(l, NULL)
#define RWLOCK_RDLOCK(l) pthread_rwlock_rdlock(l)
#define RWLOCK_WRLOCK(l) pthread_rwlock_wrlock(l)
#define RWLOCK_UNLOCK(l) pthread_rwlock_unlock(l)

#define FRAME(f) *(frame_id_t *)f

#define BTREE_INDEX 0x951F6C01 // magic number for the index metadata page

typedef uint32_t page_id_t;
typedef uint32_t frame_id_t;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t i32;
typedef int64_t i64;

enum PageType { HEAP_PAGE = 1, BTREE_INDEX_PAGE = 2, INVALID = 0 };

/*
 * Record id (RID) is a unique identifier for a record with information necessary to
 * find the corresponding page and the offset in that page using the slot number
 */
#define RID_SIZE sizeof(u32) * 2

typedef struct RID {
    page_id_t pid;
    uint32_t slot_num;

    bool operator==(const RID &other) const { return (pid == other.pid) && (slot_num == other.slot_num); }
} RID;
