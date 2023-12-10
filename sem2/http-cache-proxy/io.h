//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_IO_H
#define HTTP_CACHE_PROXY_IO_H

#include <unistd.h>

#define IO_BUFSIZE 8192
typedef struct {
    int fd;                /* descriptor for this internal buf */
    int cnt;               /* unread bytes in internal buf */
    char *bufptr;          /* next unread byte in internal buf */
    char buf[IO_BUFSIZE]; /* internal buffer */
} io_buffer_t;

io_buffer_t *create_io_buffer(int fd);
void free_io_buffer(io_buffer_t *buf);
ssize_t readln_b(io_buffer_t *iob, void *usrbuf, size_t maxlen);



#endif //HTTP_CACHE_PROXY_IO_H
