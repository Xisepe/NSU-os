//
// Created by Maxim on 10.12.2023.
//

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include "proxy.h"
#include "io.h"

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

static void proxy_readln(io_buffer_t *iob, void *buf);
static ssize_t proxy_read_n(int fd, void *buf, size_t n);
static int read_req_hdr(io_buffer_t *iob, char *data);

void proxy_readln(io_buffer_t *iob, void *buf) {
    ssize_t r = readln_b(iob, buf, MAX_LINE_SIZE);
    if (r < 0) {
        if (errno == ECONNRESET) {
            pthread_exit(NULL);
        } else {
            handle_error("failed to read data");
        }
    }
}

ssize_t proxy_read_n(int fd, void *buf, size_t n) {
    ssize_t r;
    if ((r = read_n(fd, buf, n)) < 0)
    {
        if(errno==ECONNRESET) {
            pthread_exit(NULL);
        } else {
            handle_error("failed to read data");
        }
    }
    return r;
}

void proxy_write(int fd, void *buf, size_t n) {
    if (write_n(fd, buf, n) != n) {
        if (errno == EPIPE) {
            pthread_exit(NULL);
        } else {
            handle_error("failed to write data");
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

void client_error(int fd, char *cause, char *err,
                  char *msg1, char *msg2) {
    char buf[MAX_LINE_SIZE], body[MAX_BUF_SIZE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, err, msg1);
    sprintf(body, "%s<p>%s: %s\r\n", body, msg2, cause);
    sprintf(body, "%s<hr><em>Proxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", err, msg1);
    proxy_write(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: text/html\r\n");
    proxy_write(fd, buf, strlen(buf));

    sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
    proxy_write(fd, buf, strlen(buf));

    proxy_write(fd, body, strlen(body));
}

void handle_client(cache_t *cache, int fd, io_buffer_t *io_buffer) {
    char buf[MAX_LINE_SIZE], method[MAX_LINE_SIZE], uri[MAX_LINE_SIZE], version[MAX_LINE_SIZE];
    char hdr_data[MAX_LINE_SIZE], new_request[MAX_BUF_SIZE], response[1 << 15];
    req_content content;
    int host_mentioned;
    int client_fd;
    int is_dynamic = 0; // Stores whether the content requested is dynamic

    update_seq(cache); //Refresh the clock with every request - for cache purposes

    proxy_readln(io_buffer, buf);

    sscanf(buf, "%s %s %s", method, uri, version);

    //This proxy currently only supports the GET call.
    if (strcasecmp(method, "GET") != 0) {
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
    if ((!is_dynamic) && ((hit_addr = check_cache_hit(cache, uri)) != NULL)) {
        read_cache_data(cache, hit_addr, response);
    } else { //Cache miss

        //Parse the request headers.
        host_mentioned = read_req_hdr(io_buffer, hdr_data);

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
        client_fd = open_connection(content.host, content.port);
        if (client_fd == -1) {
            handle_error("Cannot connect to destination");
        }

        //Write HTTP request to the server
        proxy_write(client_fd, new_request, sizeof(new_request));

        //Read response from the server
        ssize_t response_size = proxy_read_n(client_fd, response, sizeof(response));

        //Cache a copy of the response for future requests
        if (!is_dynamic) {
            write_to_cache(cache, uri, response, response_size);
        }

        CHECK(close(client_fd), "Cannot close client file descriptor");
    }

    //Forward the response to the client
    proxy_write(fd, response, sizeof(response));
}

int open_connection(char *host, int port) {
    int client_fd;
    struct addrinfo *info_list, *addr_info;
    char port_str[MAX_LINE_SIZE];

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    sprintf(port_str, "%d", port);
    if (getaddrinfo(host, port_str, NULL, &info_list) != 0) {
        return -1;
    }

    for (addr_info = info_list; addr_info; addr_info = addr_info->ai_next) {
        if (addr_info->ai_family == AF_INET) {
            if (connect(client_fd, addr_info->ai_addr, addr_info->ai_addrlen) == 0) {
                break;
            }
        }
    }

    freeaddrinfo(info_list);
    if (!addr_info) {
        close(client_fd);
        return -1;
    }
    else {
        return client_fd;
    }
}
