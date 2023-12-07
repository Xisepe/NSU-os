cmake_minimum_required(VERSION 3.25)
project(sync_task2_3 C)

set(CMAKE_C_STANDARD 17)
add_executable(sync_task2_3 main.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(sync_task2_3 PRIVATE Threads::Threads)

