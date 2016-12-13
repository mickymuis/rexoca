cmake_minimum_required (VERSION 2.8)

project (rexoca)

#set( CMAKE_VERBOSE_MAKEFILE ON )

list (APPEND CMAKE_C_FLAGS "-g -std=c99")

    add_executable(rexoca 
        src/main.c 
        src/rexoca.c )

target_link_libraries (rexoca "-lm" )
