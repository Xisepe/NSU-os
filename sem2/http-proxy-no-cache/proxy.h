//
// Created by Maxim on 28.12.2023.
//

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#ifndef HTTP_PROXY_PROXY_H
#define HTTP_PROXY_PROXY_H

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define CHECK(x, msg) do { \
  int retval = (x); \
  if (retval != 0) { \
    handle_error_en(retval, msg); \
  } else { break; } \
} while (0)

#define HTTP_PORT 80
#define PORT 5555

#define ERR_MSG_SIZE 512
#define MAX_BUFFER_SIZE 1 << 15
#define MAX_METHOD_SIZE 32
#define MAX_HOST_SIZE 256
#define MAX_PATH_SIZE 1 << 14 //16kb
#define MAX_URL_SIZE (MAX_HOST_SIZE + (MAX_PATH_SIZE))
#define MAX_VERSION_SIZE 16
#define MAX_HEADERS_SIZE 1 << 15 //32kb
#define MAX_HEADER_SIZE 1 << 14 //16kb
#define CRLF "\r\n"

static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

typedef struct _url_t {
    char host[MAX_HOST_SIZE];
    char path[MAX_PATH_SIZE];
    int port;
} url_t;

typedef struct _request_t {
    char method[MAX_METHOD_SIZE];
    char version[MAX_VERSION_SIZE];
    url_t url;
    char headers[MAX_HEADERS_SIZE];
} request_t;

void init_request(request_t *req);
void parse_request(char *data, size_t data_size, request_t *request);
void parse_request_url(char *data, request_t *request);
void parse_uri(char *uri, url_t *content);
void build_byte_request(request_t *req, char *buf);

ssize_t readline_header(char *data, char *buf);

int open_accept_listener(int port);
int open_connection(char *url);
void *handle_request(void *arg);

void sig_handler(int sig);
void setup_signal_handler();

#endif //HTTP_PROXY_PROXY_H
