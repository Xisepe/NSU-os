//
// Created by Maxim on 09.12.2023.
//

#include "mutex.h"
#include <stdatomic.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


void mutex_init(mutex_t *mutex) {
    mutex->lock = 0;
}

void mutex_lock(mutex_t *mutex) {
    int err;

    while (1) {
        int unlocked = 0;
        if (atomic_compare_exchange_strong(&mutex->lock, &unlocked, 1))
            break;

        err = futex(&mutex->lock, FUTEX_WAIT, 1, NULL, NULL, 0);
        if (err = -1 && errno != EAGAIN) {
            perror("futex wait failed");
            abort();
        }
    }
}

void mutex_unlock(mutex_t *mutex) {
    int locked = 1;
    if (atomic_compare_exchange_strong(&mutex->lock, &locked, 0)) {
        int err;

        err = futex(&mutex->lock, FUTEX_WAKE, 0, NULL, NULL, 0);
        if (err = -1) {
            perror("futex wake failed");
            abort();
        }
    }
}

