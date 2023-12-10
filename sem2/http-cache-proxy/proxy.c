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

void proxy_write(int fd, void *usrbuf, size_t n) {
    if (write_n(fd, usrbuf, n) != n) {
        if (errno == EPIPE)
            pthread_exit(NULL);
        else
            handle_error("Rio_writen error");
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

void client_error(int fd, char *cause, char *errnum,
                  char *shortmsg, char *longmsg) {
    char buf[MAX_LINE_SIZE], body[MAX_BUF_SIZE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Proxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    proxy_write(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: text/html\r\n");
    proxy_write(fd, buf, strlen(buf));

    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    proxy_write(fd, buf, strlen(buf));

    proxy_write(fd, body, strlen(body));
}

void handle_client(cache_t *cache, int fd) {
    char buf[MAX_LINE_SIZE], method[MAX_LINE_SIZE], uri[MAX_LINE_SIZE], version[MAX_LINE_SIZE];
    char hdr_data[MAX_LINE_SIZE], new_request[MAX_BUF_SIZE], response[1 << 15];

    io_buffer_t *io_buffer = create_io_buffer(fd);
    req_content content;

    int host_mentioned;
    int client_fd;
    int is_dynamic = 0; // Stores whether the content requested is dynamic

    update_seq(cache); //Refresh the clock with every request - for cache purposes

    proxy_readln(io_buffer, buf);

    sscanf(buf, "%s %s %s", method, uri, version);

    //This proxy currently only supports the GET call.
    if (strcasecmp(method, "GET")) {
        client_error(fd, method, "501", "Not implemented",
                     "This proxy does not implement this method");
        return;
    }

    /* if the resource requested is in the "cgi-bin" directory then the content
     * is dynamic
     */
    if (strstr(uri, "cgi-bin")) is_dynamic = 1;

    //Parse the HTTP request to extract the hostname, path and port.
    parse_uri(uri, &content);

    cache_node_t *hit_addr;

    //Check if the requested content is not dynamic and is already in cache.
    if ((!is_dynamic) && ((hit_addr = check_cache_hit(uri)) != NULL))//Cache hit
        read_cache_data(hit_addr, response);

    else { //Cache miss

        //Parse the request headers.
        host_mentioned = read_requesthdrs(&io_buffer, hdr_data);

        //Generate a new modified HTTP request to forward to the server
        sprintf(new_request, "GET %s HTTP/1.0\r\n", content.path);

        if (!host_mentioned)
            sprintf(new_request, "%sHost: %s\r\n", new_request, content.host);

        strcat(new_request, hdr_data);
        strcat(new_request, user_agent_hdr);
        strcat(new_request, accept_hdr);
        strcat(new_request, accept_encoding_hdr);
        strcat(new_request, connection_hdr);
        strcat(new_request, proxy_conn_hdr);
        strcat(new_request, "\r\n");

        //Create new connection with server
        client_fd = Open_clientfd_r(content.host, content.port);

        //Write HTTP request to the server
        Rio_writen(client_fd, new_request, sizeof(new_request));

        //Read response from the server
        int response_size = Rio_readn(client_fd, response, sizeof(response));

        //Cache a copy of the response for future requests
        if (!is_dynamic)
            write_to_cache(uri, response, response_size);

        Close(client_fd);
    }

    //Forward the response to the client
    Rio_writen(fd, response, sizeof(response));
}

