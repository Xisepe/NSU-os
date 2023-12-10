//
// Created by Maxim on 10.12.2023.
//

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "proxy.h"
#include "io.h"

void proxy_readln(io_buffer_t *iob, void *usrbuf) {
    ssize_t r = readln_b(iob, usrbuf, MAX_LINE_SIZE);
    if (r < 0) {
        if (errno == ECONNRESET) {
            pthread_exit(NULL);
        } else {
            handle_error("failed to read data");
        }
    }
}


void parse_uri(char *uri, req_content *content) {
    char temp[MAX_LINE_SIZE];

    if (strstr(uri, "http://") != NULL) {
        sscanf(uri, "http://%[^/]%s", temp, content->path);
    } else {
        sscanf(uri, "%[^/]%s", temp, content->path);
    }

    if (strstr(temp, ":") != NULL) {
        sscanf(temp, "%[^:]:%d", content->host, &content->port);
    } else {
        strcpy(content->host, temp);
        content->port = 80;
    }

    if (!content->path[0]) {
        strcpy(content->path, "./");
    }

}

int read_req_hdr(io_buffer_t *iob, char *data) {
    char buf[MAX_LINE_SIZE];
    int ret = 0;

    do {
        proxy_readln(iob, buf);

        if (!strcmp(buf, "\r\n"))
            continue;

        if (strstr(buf, "User-Agent:"))
            continue;

        if (strstr(buf, "Accept:"))
            continue;

        if (strstr(buf, "Accept-Encoding:"))
            continue;

        if (strstr(buf, "Connection:"))
            continue;

        if (strstr(buf, "Proxy-Connection:"))
            continue;

        if (strstr(buf, "Host:")) {
            sprintf(data, "%s%s", data, buf);
            ret = 1;
            continue;
        }

        sprintf(data, "%s%s", data, buf);

    } while (strcmp(buf, "\r\n"));
    return ret;
}

