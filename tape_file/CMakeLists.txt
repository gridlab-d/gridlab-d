cmake_minimum_required(VERSION 3.10)
project(tape_file)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(tape_file SHARED
        tape_file.cpp
        tape_file.h
)

set_target_properties(tape_file
        PROPERTIES PREFIX ""
        )

install(TARGETS tape_file
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )