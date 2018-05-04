cmake_minimum_required(VERSION 3.10)
project(tape_plot)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(tape_plot SHARED
        tape_plot.cpp 
        tape_plot.h
        )

set_target_properties(tape_plot
        PROPERTIES PREFIX ""
        )

install(TARGETS tape_plot
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )