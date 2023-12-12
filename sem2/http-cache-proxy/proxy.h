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

void parse_uri(char *uri, req_content *content);

void client_error(int fd, char *cause, char *err,
                  char *msg1, char *msg2);

void handle_client(cache_t *cache, int fd, io_buffer_t *io_buffer);

int open_connection(char *host, int port);

#endif //HTTP_CACHE_PROXY_PROXY_H
