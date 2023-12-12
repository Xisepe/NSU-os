//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_CACHE_H
#define HTTP_CACHE_PROXY_CACHE_H

#include <pthread.h>
#include <stdint-gcc.h>
#include "proxy_config.h"

#define CACHE_KEY_SIZE MAX_LINE_SIZE
#define CACHE_DATA_SIZE MAX_OBJECT_SIZE
#define MAX_CACHE_SIZE 1049000

/*
 * LRU cache
*/

typedef struct _cache_node {
    char key[CACHE_KEY_SIZE];
    char data[CACHE_DATA_SIZE];
    int64_t timestamp;
    pthread_rwlock_t lock;
    struct _cache_node *next;
} cache_node_t;

typedef struct _cache {
    cache_node_t *head;
    int32_t size;
    int64_t timestamp_seq;
    pthread_rwlock_t lock;
} cache_t;

void update_seq(cache_t *cache);

cache_t *cache_init();

void free_cache(cache_t *cache);

cache_node_t *create_node(char *key, char *data);

void free_node(cache_node_t *node);

cache_node_t *check_cache_hit(cache_t *cache, char *key);

void read_cache_data(cache_t *cache, cache_node_t *hit_addr, char *response);

void write_to_cache(cache_t *cache, char *key, char *data, int size);


#endif //HTTP_CACHE_PROXY_CACHE_H
