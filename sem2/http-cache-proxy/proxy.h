//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_PROXY_H
#define HTTP_CACHE_PROXY_PROXY_H

#include "proxy_config.h"
#include "io.h"
#include "cache.h"


typedef struct _req_content {
    char host[MAX_LINE_SIZE];
    char path[MAX_LINE_SIZE];
    int port;
} req_content;

void proxy_readln(io_buffer_t *iob, void *usrbuf);

void proxy_write(int fd, void *usrbuf, size_t n);

void parse_uri(char *uri, req_content *content);

void client_error(int fd, char *cause, char *errnum,
                  char *shortmsg, char *longmsg);

void handle_client(cache_t *cache, int fd);

#endif //HTTP_CACHE_PROXY_PROXY_H
