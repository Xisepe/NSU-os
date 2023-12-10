//
// Created by Maxim on 10.12.2023.
//

#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "io.h"

io_buffer_t *create_io_buffer(int fd) {
    io_buffer_t *buf = malloc(sizeof(io_buffer_t));
    assert(buf != NULL);
    buf->fd = fd;
    buf->cnt = 0;
    buf->bufptr = buf->buf;
    return buf;
}

void free_io_buffer(io_buffer_t *buf) {
    buf->bufptr = NULL;
    free(buf);
}


static ssize_t _io_read(io_buffer_t *iob, char *usrbuf, size_t n) {
    int cnt;

    while (iob->cnt <= 0) {  /* refill if buf is empty */
        iob->cnt = read(iob->fd, iob->buf,
                        sizeof(iob->buf));
        if (iob->cnt < 0) {
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        } else if (iob->cnt == 0)  /* EOF */
            return 0;
        else
            iob->bufptr = iob->buf; /* reset buffer ptr */
    }

    /* Copy min(n, iob->cnt) bytes from internal buf to user buf */
    cnt = n;
    if (iob->cnt < n)
        cnt = iob->cnt;
    memcpy(usrbuf, iob->bufptr, cnt);
    iob->bufptr += cnt;
    iob->cnt -= cnt;
    return cnt;
}

ssize_t readln_b(io_buffer_t *iob, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = _io_read(iob, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break;    /* EOF, some data was read */
        } else
            return -1;    /* error */
    }
    *bufp = 0;
    return n;
}