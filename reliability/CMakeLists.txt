cmake_minimum_required(VERSION 3.10)
project(reliability)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(reliability SHARED
        eventgen.cpp
        eventgen.h
        init.cpp
        main.cpp
        metrics.cpp
        metrics.h
        reliability.h
)

set_target_properties(reliability
        PROPERTIES PREFIX ""
        )

install(TARGETS reliability
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )