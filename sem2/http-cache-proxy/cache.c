//
// Created by Maxim on 10.12.2023.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
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

