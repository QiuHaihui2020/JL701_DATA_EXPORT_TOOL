#ifndef SYS_SPINLOCK_H
#define SYS_SPINLOCK_H

#include "typedef.h"
#include "cpu.h"
#include "irq.h"


struct __spinlock {
    volatile u8 rwlock;
    volatile u8 lock_cnt[CPU_CORE_NUM];
};

typedef struct __spinlock spinlock_t;


#define DEFINE_SPINLOCK(x) \
	spinlock_t x = { .rwlock = 0 }


void spin_lock_init(spinlock_t *lock);

void spin_lock(spinlock_t *lock);

void spin_unlock(spinlock_t *lock);


#endif

