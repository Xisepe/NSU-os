cmake_minimum_required(VERSION 3.22)
project(http_cache_proxy C)

set(CMAKE_C_STANDARD 11)

add_executable(http_cache_proxy main.c cache.c cache.h proxy_config.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(http_cache_proxy PRIVATE Threads::Threads)