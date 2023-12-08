//
// Created by Maxim on 09.12.2023.
//

#ifndef SYNC_2_4_SPINLOCK_H
#define SYNC_2_4_SPINLOCK_H
typedef struct {
    int lock;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

#endif //SYNC_2_4_SPINLOCK_H
