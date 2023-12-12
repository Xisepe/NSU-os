#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include "proxy.h"

typedef struct {
    int fd;
    cache_t *cache;
    io_buffer_t *io_buffer;
} proxy_arg_t;

proxy_arg_t *create_proxy_arg(int fd, cache_t *cache) {
    proxy_arg_t *arg = malloc(sizeof(proxy_arg_t));
    assert(arg != NULL);
    arg->fd = fd;
    arg->cache = cache;
    arg->io_buffer = create_io_buffer(fd);
    return arg;
}

void free_proxy_arg(proxy_arg_t *arg) {
    arg->cache = NULL;
    free_io_buffer(arg->io_buffer);
    CHECK(close(arg->fd), "failed to close file descriptor");
    free(arg);
}

void *proxy_routine(void *args) {
    proxy_arg_t *arg = args;
    pthread_cleanup_push((void *) free_proxy_arg, arg)
            CHECK(pthread_detach(pthread_self()), "failed to detach thread");
            handle_client(arg->cache, arg->fd, arg->io_buffer);
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

void sig_handler(int sig) {
    printf("SIGPIPE signal trapped. Exiting thread.");
    pthread_exit(NULL);
}

void setup_signal_handler() {
    struct sigaction action, old_action;

    action.sa_handler = sig_handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    CHECK(sigaction(SIGPIPE, &action, &old_action), "failed to setup signal handler");
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

    bzero((char *) &server_addr, sizeof(server_addr));
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    setup_signal_handler();
    int port = atoi(argv[1]);
    int listen_fd = open_accept_listener(port);
    if (listen_fd <= 0) {
        fprintf(stderr, "failed to create server socket");
        exit(-1);
    }
    cache_t *cache = cache_init();
    pthread_cleanup_push((void*) free_cache, cache)

    while (1) {
        struct sockaddr_in addr;
        int len = sizeof(addr);
        int conn = accept(listen_fd, (struct sockaddr *) &addr, (socklen_t *) &len);
        if (conn == -1) {
            handle_error("failed to accept connection");
        }
        pthread_t tid;
        proxy_arg_t *arg = create_proxy_arg(conn, cache);
        CHECK(
                pthread_create(&tid, NULL, (void *) proxy_routine, (void *) arg),
                "cannot create thread for new connection"
        );
    }

    pthread_cleanup_pop(1);

    return 0;
}
