//
// Created by Maxim on 10.12.2023.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "cache.h"


cache_t *cache_init() {
    cache_t *cache = malloc(sizeof(cache_t));
    assert(cache != NULL);
    cache->head = NULL;
    cache->size = 0;
    cache->timestamp_seq = 0;
    CHECK(pthread_rwlock_init(&cache->lock, NULL), "cache_init(): failed to init lock");
    return cache;
}

cache_node_t *create_node(char *key, char *data) {
    cache_node_t *node = malloc(sizeof(cache_node_t));
    assert(node != NULL);
    strcpy(node->key, key);
    strcpy(node->data, data);
    CHECK(pthread_rwlock_init(&node->lock, NULL), "create_node(): failed to init lock");
    node->next = NULL;
    node->timestamp = 0;
    return node;
}

void free_node(cache_node_t *node) {
    CHECK(pthread_rwlock_destroy(&node->lock), "free_node(): failed to destroy lock");
    free(node->key);
    free(node->data);
    node->next = NULL;
    free(node);
}

void free_cache(cache_t *cache) {
    CHECK(pthread_rwlock_destroy(&cache->lock), "free_cache(): failed to destroy lock");
    cache_node_t *tmp = cache->head;
    cache_node_t *next = tmp;
    while (next != NULL) {
        next = tmp->next;
        free_node(tmp);
        tmp = next;
    }
    free(cache);
}

cache_node_t *check_cache_hit(cache_t *cache, char *key) {

    CHECK(pthread_rwlock_rdlock(&cache->lock), "check_cache_hit(): failed to read-lock cache");

    cache_node_t *cur = cache->head;

    while (cur != NULL) {
        if (pthread_rwlock_trywrlock(&cur->lock) == 0) {
            if (!strcmp(cur->key, key)) {
                return cur;
            }
            CHECK(pthread_rwlock_unlock(&cur->lock), "check_cache_hit(): failed to read-unlock cache");
        }
        cur = cur->next;
    }

    CHECK(pthread_rwlock_unlock(&cache->lock), "check_cache_hit(): failed to read-unlock cache");

    return NULL;
}

void write_to_cache(cache_t *cache, char *key, char *data, int size) {
    if (size > CACHE_DATA_SIZE) return;

    CHECK(pthread_rwlock_rdlock(&cache->lock), "write_to_cache(): failed to read-lock cache");
    int32_t cache_size = cache->size;
    CHECK(pthread_rwlock_unlock(&cache->lock), "write_to_cache(): failed to read-unlock cache");

    if (cache_size + size <= MAX_CACHE_SIZE) {

        CHECK(pthread_rwlock_wrlock(&cache->lock), "write_to_cache(): failed to write-lock cache");

        cache_node_t *node = create_node(key, data);
        node->timestamp = cache->timestamp_seq;
        node->next = cache->head;
        cache->head = node;
        cache->size += size;

        CHECK(pthread_rwlock_unlock(&cache->lock), "write_to_cache(): failed to read-unlock cache");
    } else {
        cache_node_t *victim, *cur;

        CHECK(pthread_rwlock_rdlock(&cache->lock), "write_to_cache(): failed to read-lock cache");
        cur = cache->head;
        int64_t lru = cache->timestamp_seq;
        CHECK(pthread_rwlock_unlock(&cache->lock), "write_to_cache(): failed to read-unlock cache");

        while (cur != NULL) {
            if (pthread_rwlock_trywrlock(&cur->lock) == 0) {
                if (cur->timestamp < lru) {
                    CHECK(pthread_rwlock_unlock(&victim->lock), "write_to_cache(): failed to write-unlock cache node");
                    lru = cur->timestamp;
                    victim = cur;
                } else{
                    CHECK(pthread_rwlock_unlock(&cur->lock), "write_to_cache(): failed to write-unlock cache node");
                }
            }
            cur = cur->next;
        }
        //Modify victim
        strcpy(victim->data, data);
        strcpy(victim->key, key);

        CHECK(pthread_rwlock_rdlock(&cache->lock), "write_to_cache(): failed to read-lock cache");
        victim->timestamp = cache->timestamp_seq;
        CHECK(pthread_rwlock_unlock(&cache->lock), "write_to_cache(): failed to read-unlock cache");

        CHECK(pthread_rwlock_unlock(&victim->lock), "write_to_cache(): failed to write-unlock cache node");
    }
}