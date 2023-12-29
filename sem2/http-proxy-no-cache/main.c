#include "proxy.h"

void init_request(request_t *req) {
    bzero(req->headers, sizeof(req->headers));
    bzero(req->version, sizeof(req->version));
    bzero(req->method, sizeof(req->method));
    bzero(req->url.path, sizeof(req->url.path));
    bzero(req->url.host, sizeof(req->url.host));
}

void finish_thread(const char *msg) {
    fprintf(stderr, "Error message: %s\nExiting thread: %d\n", msg, gettid());
    pthread_exit(PTHREAD_CANCELED);
}

void parse_uri(char *uri, url_t *content) {
    char temp[512];

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

int open_connection(char *url) {
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(HTTP_PORT);

    struct hostent *host = gethostbyname(url);
    if (host == NULL) {
        finish_thread("Cannot resolve server name\n");
    }
    memcpy(&target_addr.sin_addr, host->h_addr_list[0], host->h_length);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        finish_thread("Cannot create socket for target connection\n");
    }

    if (connect(socket_fd, (struct sockaddr *) &target_addr, sizeof(target_addr)) == -1) {
        char err[ERR_MSG_SIZE];
        sprintf(err, "Cannot establish connection with server: %s\n", url);
        close(socket_fd);
        finish_thread(err);
    }
    return socket_fd;
}

ssize_t readline_header(char *data, char *buf) {
    if (data == NULL || buf == NULL) {
        return -1;
    }
    char *cur = data;
    char *end = strstr(cur, CRLF);
    if (end == NULL) {
        return -1;
    }
    ssize_t n = end - cur;
    if (n == 0) {
        return 0;
    }
    strncpy(buf, cur, n);
    buf[n] = '\0';
    return n;
}

void parse_request_url(char *data, request_t *request) {
    char tmp[MAX_URL_SIZE];
    sscanf(data, "%s %s %s", request->method, tmp, request->version);
    parse_uri(tmp, &request->url);
}

int parse_request(char *data, size_t data_size, request_t *req) {
    char buf[MAX_BUFFER_SIZE];
    bzero(buf, MAX_BUFFER_SIZE);
    char *cur = data;

    ssize_t read = readline_header(cur, buf);
    if (read == -1) {
        return -1;
    }

    cur += read + 2;
    parse_request_url(buf, req);

    int hostFlag = 0;

    while (cur - data < data_size && (read = readline_header(cur, buf))) {
        cur += read + 2;
        if (strstr(buf, "Connection:")) {
            continue;
        }
        if (strstr(buf, "Proxy-Connection:")) {
            continue;
        }
        if (strstr(buf, "Host:")) {
            hostFlag = 1;
        }
        strncat(req->headers, buf, read);
        strcat(req->headers, "\r\n");
    }
    if (!hostFlag) {
        char tmp[MAX_HEADER_SIZE];
        bzero(tmp, sizeof(tmp));
        sprintf(tmp, "Host: %s:%d\r\n", req->url.host, req->url.port);
        strcat(req->headers, tmp);
    }
    strcat(req->headers, connection_hdr);
    strcat(req->headers, proxy_conn_hdr);
    strcat(req->headers, "\r\n");
    return 0;
}

void build_byte_request(request_t *req, char *buf) {
    sprintf(buf, "%s %s %s\r\n%s", req->method, req->url.path, "HTTP/1.0", req->headers);
}

void close_routine(void *arg) {
    int fd = *((int*)arg);
    close(fd);
}

void *handle_request(void *arg) {
    int socket_fd = *((int*)arg);
    free(arg);

    pthread_cleanup_push(close_routine, &socket_fd)

    if (pthread_detach(pthread_self())) {
        finish_thread("Cannot detach request handler thread. Exiting thread\n");
    }

    char buffer[MAX_BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));

    ssize_t r = read(socket_fd, buffer, MAX_BUFFER_SIZE);
    if (r == -1) {
        finish_thread("Cannot read request from client\n");
    }

    request_t request;
    init_request(&request);
    if (parse_request(buffer, r, &request) == -1) {
        finish_thread("Failed to parse request\n");
    }

    bzero(buffer, sizeof(buffer));
    build_byte_request(&request, buffer);
    printf("%s", buffer);

    int server_fd = open_connection(request.url.host);
            pthread_cleanup_push(close_routine, &server_fd)
    ssize_t w = write(server_fd, buffer, strlen(buffer));
    if (w == -1) {
        char err[ERR_MSG_SIZE];
        sprintf(err, "Cannot send request to server: %s\n", request.url.host);
        finish_thread(err);
    }

    bzero(buffer, sizeof(buffer));
    while ((r = read(server_fd, buffer, sizeof(buffer))) > 0) {
        w = send(socket_fd, buffer, r, MSG_NOSIGNAL);
        if (w == -1) {
            finish_thread("Client closed connection\n");
        }
    }
            pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

int open_accept_listener(int port) {
    int listen_fd, optval = 1;
    struct sockaddr_in server_addr;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *) &optval, sizeof(int)) < 0) {
        return -1;
    }

    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short) port);
    if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }

    if (listen(listen_fd, 1024) < 0) {
        return -1;
    }
    return listen_fd;
}

int main() {
    int listen_fd = open_accept_listener(PORT);
    if (listen_fd == -1) {
        finish_thread("Cannot create socket to accept connections\n");
    }
    int err;
    while (1) {
        struct sockaddr_in addr;
        int len = sizeof(addr);
        int conn = accept(listen_fd, (struct sockaddr *) &addr, (socklen_t *) &len);
        if (conn == -1) {
            fprintf(stderr, "Failed to accept connection\n");
            continue;
        }

        int *arg = malloc(sizeof(int));
        *arg = conn;

        pthread_t tid;
        err = pthread_create(&tid, NULL, (void *) handle_request, (void *) arg);
        if (err) {
            fprintf(stderr,"Cannot create request handler thread\n");
            close(conn);
        }
    }

    return 0;
}