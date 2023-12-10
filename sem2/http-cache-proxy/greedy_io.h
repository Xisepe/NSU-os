//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_GREEDY_IO_H
#define HTTP_CACHE_PROXY_GREEDY_IO_H

#include <unistd.h>

ssize_t read_n(int fd, void *buf, size_t n);
ssize_t write_n(int fd, void *buf, size_t n);



#endif //HTTP_CACHE_PROXY_GREEDY_IO_H
