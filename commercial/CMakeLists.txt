cmake_minimum_required(VERSION 3.10)
project(commercial)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(commercial SHARED
        hvac.cpp
        hvac.h
        init.cpp
        main.cpp
        multizone.cpp
        multizone.h
        office.cpp
        office.h
        solvers.cpp
        solvers.h
        )

set_target_properties(commercial
        PROPERTIES PREFIX ""
        )

install(TARGETS commercial
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )