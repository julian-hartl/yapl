cmake_minimum_required(VERSION 3.24)
project(yapl-compiler C)

set(CMAKE_C_STANDARD 11)

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_executable(yaplc src/main.c)
