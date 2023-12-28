cmake_minimum_required(VERSION 3.25)
project(http_proxy C)

set(CMAKE_C_STANDARD 11)
add_executable(http_proxy main.c
        proxy.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(http_proxy PRIVATE Threads::Threads)