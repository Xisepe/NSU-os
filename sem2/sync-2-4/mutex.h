//
// Created by Maxim on 09.12.2023.
//

#ifndef SYNC_2_4_MUTEX_H
#define SYNC_2_4_MUTEX_H

#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>


typedef struct Mutex_t {
    int lock;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#define futex(uaddr, futex_op, val, timeout, uaddr2, val3) syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3)

#endif //SYNC_2_4_MUTEX_H
