#pragma once

#include <pthread.h>
#include <stdint.h>

#define RWLOCK pthread_rwlock_t
#define RWLOCK_INIT(l) pthread_rwlock_init(l, NULL)
#define RWLOCK_RDLOCK(l) pthread_rwlock_rdlock(l)
#define RWLOCK_WRLOCK(l) pthread_rwlock_wrlock(l)
#define RWLOCK_UNLOCK(l) pthread_rwlock_unlock(l)

typedef uint32_t page_id_t;
typedef uint32_t frame_id_t;
