cmake_minimum_required(VERSION 3.9)
project(OSex4 C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(OSex4 main.c threadPool.c threadPool.h osqueue.c osqueue.h)