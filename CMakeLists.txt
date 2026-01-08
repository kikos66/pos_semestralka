cmake_minimum_required(VERSION 3.25.1)
project(pos_semestralka C)

set(CMAKE_C_STANDARD 11)

add_executable(pos_semestralka main.c
        server.c
        server.h
        client.c
        client.h)
