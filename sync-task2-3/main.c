#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>

#define MUTEX 1
//#define SPINLOCK 1
//#define RWLOCK 1

#if defined(MUTEX)
#define LOCK_TYPE pthread_mutex_t
#define LOCK_INIT(LOCK) pthread_mutex_init(LOCK, NULL)
#define LOCK(L) pthread_mutex_lock(L)
#define UNLOCK(L) pthread_mutex_unlock(L)

#elif defined(SPINLOCK)
#define LOCK_TYPE pthread_spinlock_t
#define LOCK_INIT(LOCK) pthread_spin_init(LOCK, PTHREAD_PROCESS_PRIVATE)
#define LOCK(L) pthread_spin_lock(L)
#define UNLOCK(L) pthread_spin_unlock(L)

#elif defined(RWLOCK)
#define LOCK_TYPE pthread_rwlock_t
#define LOCK_INIT(LOCK) pthread_rwlock_init(LOCK, NULL)
#define RLOCK(L) pthread_rwlock_rdlock(L)
#define WLOCK(L) pthread_rwlock_wrlock(L)
#define UNLOCK(L) pthread_rwlock_unlock(L)

#endif

#define NODE_VALUE_SIZE 128
#define STORAGE_SIZE 1000
#define TOTAL_THREADS 7
#define MIN_LIST_SIZE 3

typedef struct _Node {
    char value[NODE_VALUE_SIZE];
    struct _Node *next;
    LOCK_TYPE sync;
} Node;

typedef struct _Storage {
    Node *first;
} Storage;

typedef struct _FindArg {
    Storage *storage;
    int find_op;
} FindArg;

typedef struct _SwapArg {
    Storage *storage;
    int id;
} SwapArg;

enum num_thread {
    INCREASING,
    DECREASING,
    EQUALS,
    RANDOM_SWAP_1,
    RANDOM_SWAP_2,
    RANDOM_SWAP_3,
    MONITOR
};

enum op {
    LS,
    GR,
    EQ
};

int find_ind(int op) {
    switch (op) {
        case LS:
            return INCREASING;
        case GR:
            return DECREASING;
        case EQ:
            return EQUALS;
        default:
            exit(-1);
    }
}

_Atomic int count_increment[TOTAL_THREADS] = {0};

int rnd(int min, int max) {
    int index;
    do {
        unsigned int seed = (unsigned int) time(NULL);
        index = rand_r(&seed) % (max - 2);
    } while (index < min);
    return index;
}

Node *build_node(const char *value) {
    Node *nd = malloc(sizeof(Node));
    assert(nd != NULL);
    strcpy(nd->value, value);
    nd->next = NULL;
    LOCK_INIT(&nd->sync);
    return nd;
}

void push(Storage *list, Node *nd) {
    assert(list != NULL);
    if (list->first == NULL) {
        list->first = nd;
        return;
    }
    Node *tmp = list->first;
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = nd;
}

Storage *build_storage() {
    Storage *list = malloc(sizeof(Storage));
    assert(list != NULL);
    list->first = NULL;
    return list;
}

void prepare_dataset(Storage *list, int len) {
    assert(list != NULL);
    char buf[NODE_VALUE_SIZE];
    for (int i = 0; i < len; ++i) {
        memset(buf, 0, NODE_VALUE_SIZE);
        memset(buf, 'f', i % NODE_VALUE_SIZE);
        push(list, build_node(buf));
    }
}

void print_storage(Storage *list) {
    Node *node = list->first;
    while (node != NULL) {
        printf("%s \n", node->value);
        node = node->next;
    }
}

void *find(void *args) {
    FindArg *arg = args;
    Storage *storage = arg->storage;
    while (1) {
        int counter = 0;
        Node *nd = storage->first, *next, *tmp;
        if (nd == NULL || nd->next == NULL) {
            fprintf(stderr, "warn: storage is empty\n");
            continue;
        }
        while (1) {
#ifdef RWLOCK
            RLOCK(&nd->sync);
#else
            LOCK(&nd->sync);
#endif

            printf("lock current nd %d, %p\n", find_ind(arg->find_op), &(nd->sync));
            tmp = nd;

            next = nd->next;
            if (next == NULL) {
                UNLOCK(&nd->sync);
                break;
            }
#ifdef RWLOCK
                RLOCK(&next->sync);
#else
            LOCK(&next->sync);
#endif

            printf("lock future nd %d, %p\n", find_ind(arg->find_op), &(next->sync));

            if (arg->find_op == LS) {
                if (strlen(nd->value) < strlen(next->value)) ++counter;
            } else if (arg->find_op == GR) {
                if (strlen(nd->value) > strlen(next->value)) ++counter;
            } else if (arg->find_op == EQ) {
                if (strlen(nd->value) == strlen(next->value)) ++counter;
            }


            nd = nd->next;

            printf("Unlock next nd %d, %p\n", find_ind(arg->find_op), &(next->sync));
            UNLOCK(&next->sync);

            printf("Unlock cur nd %d, %p\n", find_ind(arg->find_op), &(tmp->sync));
            UNLOCK(&tmp->sync);
        }
        ++count_increment[find_ind(arg->find_op)];
    }
}

void swap_nodes(Node *prev, Node *cur, Node *next) {
    printf("swap nodes\n");
    assert(prev != NULL && cur != NULL && next != NULL);
    prev->next = next;
    Node *tmp = next->next;
    next->next = cur;
    cur->next = tmp;
}

void *swap(void *args) {
    SwapArg *arg = args;
    Storage *storage = arg->storage;
    pid_t id = arg->id;
    printf("number thread %d\n", id);
    assert(storage != NULL);

    while (1) {
        Node *prev = storage->first;
        if (prev == NULL || prev->next == NULL) {
            fprintf(stderr, "warn: there are less, than two items in the value\n");
            continue;
        }

        int index = rnd(MIN_LIST_SIZE, STORAGE_SIZE - 2);

        Node *current = prev->next, *next = prev->next->next, *node;

        for (int i = 0; i < index; ++i) {
#ifdef RWLOCK
            WLOCK(&prev->sync);
#else
            LOCK(&prev->sync);
#endif
            printf("lock previous node %d %p\n", id, &(prev->sync));

            if (prev->next == NULL) {
                printf("NULL prev->next\n");
                UNLOCK(&prev->sync);
                break;
            }
#ifdef RWLOCK
                WLOCK(&(prev->next->sync));
#else
            LOCK(&(prev->next->sync));
#endif
            printf("lock current node %d %p\n", id, &(prev->next->sync));

            if (prev->next->next == NULL) {
                printf("NULL prev->next->next\n");
                UNLOCK(&(prev->next->sync));
                UNLOCK(&(prev->sync));
                break;
            }
#ifdef RWLOCK
                WLOCK(&(prev->next->next->sync));
#else
            LOCK(&(prev->next->next->sync));
#endif
            printf("lock next node %d %p\n", id, &(prev->next->sync));
            node = prev;
            if (i == index - 1) swap_nodes(prev, current, next);
            else {
                prev = prev->next;
                current = current->next;
                next = next->next;
            }

            printf("Unlock next node %d %p\n", id, &(node->next->next->sync));
            UNLOCK(&(node->next->next->sync));

            printf("Unlock current node %d %p\n", id, &(node->next->sync));
            UNLOCK(&(node->next->sync));

            printf("Unlock previous node %d %p\n", id, &(node->sync));
            UNLOCK(&(node->sync));
        }

        ++count_increment[id];
    }
}

void *monitor(void *arg) {
    int fd = open("log.txt", O_CREAT | O_WRONLY);
    while (1) {
        dprintf(fd, "\n\ncount INCREASING: %d,"
                    " DECREASING %d, EQUALS %d,"
                    " RANDOM_SWAP_1 %d,"
                    " RANDOM_SWAP_2 %d,"
                    " RANDOM_SWAP_3 %d\n\n", count_increment[INCREASING],
                count_increment[DECREASING],
                count_increment[EQUALS],
                count_increment[RANDOM_SWAP_1],
                count_increment[RANDOM_SWAP_2],
                count_increment[RANDOM_SWAP_3]);
        sleep(1);
    }
    close(fd);
}

int main() {
    Storage *list = build_storage();
    prepare_dataset(list, STORAGE_SIZE);
//    print_storage(list);

    pthread_t tids[TOTAL_THREADS];

    FindArg ls = {list, LS};
    FindArg gr = {list, GR};
    FindArg eq = {list, EQ};

    pthread_create(&tids[INCREASING], NULL, find, (void *) &ls);
    pthread_create(&tids[DECREASING], NULL, find, (void *) &gr);
    pthread_create(&tids[EQUALS], NULL, find, (void *) &eq);

    SwapArg s1 = {list, RANDOM_SWAP_1};
    SwapArg s2 = {list, RANDOM_SWAP_2};
    SwapArg s3 = {list, RANDOM_SWAP_3};

    pthread_create(&tids[RANDOM_SWAP_1], NULL, swap, (void *) &s1);
    pthread_create(&tids[RANDOM_SWAP_2], NULL, swap, (void *) &s2);
    pthread_create(&tids[RANDOM_SWAP_3], NULL, swap, (void *) &s3);

    pthread_create(&tids[MONITOR], NULL, monitor, NULL);

    pthread_join(tids[MONITOR], NULL);
    return 0;
}
