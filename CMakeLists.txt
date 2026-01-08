cmake_minimum_required(VERSION 3.30)
project(pos_semestralka C)

set(CMAKE_C_STANDARD 11)

add_executable(pos_semestralka main.c
        game.c
        game.h)
