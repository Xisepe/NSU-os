//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_PROXY_H
#define HTTP_CACHE_PROXY_PROXY_H

#include "proxy_config.h"

typedef struct _req_content {
    char host[MAX_LINE_SIZE];
    char path[MAX_LINE_SIZE];
    int port;
} req_content;

#endif //HTTP_CACHE_PROXY_PROXY_H
