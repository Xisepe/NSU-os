#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "spinlock.h"

int glob;

void *incrementAndPrint(void *args) {
    spinlock_t *lock = args;
    while (1) {
        spinlock_lock(lock);
        glob++;
        printf("incr glob %d\n", glob);
        sleep(3);
        spinlock_unlock(lock);
    }
}

int main() {
    pthread_t tid;
    spinlock_t lock;
    spinlock_init(&lock);

    pthread_create(&tid, NULL, incrementAndPrint, &lock);
    sleep(1);

    while (1) {
        spinlock_lock(&lock);
        printf("lock in main\n");
        sleep(1);
        spinlock_unlock(&lock);
    }

    return 0;
}
