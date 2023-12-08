//
// Created by Maxim on 09.12.2023.
//

#include "spinlock.h"
#include <stdatomic.h>

void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}

void spinlock_lock(spinlock_t *lock) {
    int unlocked = 0;
    while (!atomic_compare_exchange_strong(&lock->lock, &unlocked, 1));
}

void spinlock_unlock(spinlock_t *lock) {
    int locked = 1;
    atomic_compare_exchange_strong(&lock->lock, &locked, 0);
}