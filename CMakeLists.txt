cmake_minimum_required(VERSION 3.6)
project(lizp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c11 -ledit")

include_directories(/usr/include/readline)
set(SOURCE_FILES
    lizp.c
    lizp.h
    mpc.c
    mpc.h)

add_executable(lizp ${SOURCE_FILES})
target_link_libraries(lizp readline)
