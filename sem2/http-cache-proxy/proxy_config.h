//
// Created by Maxim on 10.12.2023.
//

#ifndef HTTP_CACHE_PROXY_PROXY_CONFIG_H
#define HTTP_CACHE_PROXY_PROXY_CONFIG_H

#include <errno.h>
#include <stdlib.h>

#define handle_error(msg) \
               do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define CHECK(x, msg) do { \
  int retval = (x); \
  if (retval != 0) { \
    handle_error_en(retval, msg); \
  } else { break; } \
} while (0)

#define	MAX_LINE_SIZE 8192
#define MAX_OBJECT_SIZE 102400
#define MAX_BUF_SIZE 8192

#endif //HTTP_CACHE_PROXY_PROXY_CONFIG_H
