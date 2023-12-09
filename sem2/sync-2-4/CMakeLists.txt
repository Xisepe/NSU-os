cmake_minimum_required(VERSION 3.22.1)
project(sync_2_4 C)

set(CMAKE_C_STANDARD 17)

add_executable(sync_2_4 main.c spinlock.c spinlock.h mutex.c mutex.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(sync_2_4 PRIVATE Threads::Threads)
