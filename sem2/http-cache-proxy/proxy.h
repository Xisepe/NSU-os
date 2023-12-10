//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_PROXY_H
#define HTTP_CACHE_PROXY_PROXY_H

#include "proxy_config.h"
#include "io.h"

typedef struct _req_content {
    char host[MAX_LINE_SIZE];
    char path[MAX_LINE_SIZE];
    int port;
} req_content;

void proxy_readln(io_buffer_t *iob, void *usrbuf);

void parse_uri(char *uri, req_content *content);

#endif //HTTP_CACHE_PROXY_PROXY_H
