//
// Created by Maxim on 10.12.2023.
//

#include <errno.h>
#include "greedy_io.h"

ssize_t read_n(int fd, void *buf, size_t n) {
    size_t left = n;
    ssize_t r;
    char *bufp = buf;

    while (left > 0) {
        if ((r = read(fd, bufp, left)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
            { r = 0; }     /* and call read() again */
            else { return -1; }     /* errno set by read() */
        } else if (r == 0) { break; }         /* EOF */
        left -= r;
        bufp += r;
    }
    return (n - left);         /* return >= 0 */
}

ssize_t write_n(int fd, void *buf, size_t n) {
    size_t left = n;
    ssize_t w;
    char *bufp = buf;

    while (left > 0) {
        if ((w = write(fd, bufp, left)) <= 0) {
            if (errno == EINTR)  /* interrupted by sig handler return */
            { w = 0; }    /* and call write() again */
            else { return -1; }       /* errorno set by write() */
        }
        left -= w;
        bufp += w;
    }
    return n;
}