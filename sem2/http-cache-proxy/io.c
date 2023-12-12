//
// Created by Maxim on 10.12.2023.
//

#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "io.h"

static int relative_copy(io_buffer_t *iob, char *buf, size_t n);
static ssize_t relative_read(io_buffer_t *iob, char *buf, size_t n);

io_buffer_t *create_io_buffer(int fd) {
    io_buffer_t *buf = malloc(sizeof(io_buffer_t));
    assert(buf != NULL);
    buf->fd = fd;
    buf->len = 0;
    buf->bufptr = buf->buf;
    return buf;
}

void free_io_buffer(io_buffer_t *buf) {
    buf->bufptr = NULL;
    free(buf);
}

ssize_t read_n(int fd, void *buf, size_t n) {
    size_t left = n;
    ssize_t r;
    char *tmp = buf;

    while (left > 0) {
        if ((r = read(fd, tmp, left)) < 0) {
            if (errno == EINTR) {
                r = 0;
            } else {
                return -1;
            }
        } else if (r == 0) {
            break;
        }
        left -= r;
        tmp += r;
    }
    return n - left;         /* return >= 0 */
}

static ssize_t relative_read(io_buffer_t *iob, char *buf, size_t n) {
    while (iob->len <= 0) {  /* refill if buf is empty */
        iob->len = read(iob->fd, iob->buf,
                        sizeof(iob->buf));
        if (iob->len < 0) {
            if (errno != EINTR) { /* interrupted by sig handler return */
                return -1;
            }
        } else if (iob->len == 0) { /* EOF */
            return 0;
        } else {
            iob->bufptr = iob->buf; /* reset buffer ptr */
        }
    }

    /* Copy min(n, iob->len) bytes from internal buf to user buf */
    int len = relative_copy(iob, buf, n);
    return len;
}

static int relative_copy(io_buffer_t *iob, char *buf, size_t n) {
    int len = n;
    if (iob->len < n) {
        len = iob->len;
    }
    memcpy(buf, iob->bufptr, len);
    iob->bufptr += len;
    iob->len -= len;
    return len;
}

ssize_t readln_b(io_buffer_t *iob, void *buf, size_t len) {
    size_t n, rc;
    char c, *tmp = buf;
    for (n = 1; n < len; n++) {
        if ((rc = relative_read(iob, &c, 1)) == 1) {
            *tmp++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0; /* EOF, no data read */
            } else {
                break;    /* EOF, some data was read */
            }
        } else {
            return -1;    /* error */
        }
    }
    *tmp = 0;
    return n;
}

ssize_t write_n(int fd, void *buf, size_t n) {
    size_t left = n;
    ssize_t w;
    char *tmp = buf;

    while (left > 0) {
        if ((w = write(fd, tmp, left)) <= 0) {
            if (errno == EINTR) { /* interrupted by sig handler return */
                w = 0;    /* and call write() again */
            } else {
                return -1;       /* errorno set by write() */
            }
        }
        left -= w;
        tmp += w;
    }
    return n;
}
