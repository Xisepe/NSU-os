//
// Created by Maxim on 10.12.2023.
//

#include <string.h>
#include <stdio.h>
#include "proxy.h"

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