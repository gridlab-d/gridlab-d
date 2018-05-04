cmake_minimum_required(VERSION 3.10)
project(optimize)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(optimize SHARED
        init.cpp
        main.cpp
        optimize.h
        particle_swarm_optimization.cpp
        particle_swarm_optimization.h
        simple.cpp
        simple.h
)

set_target_properties(optimize
        PROPERTIES PREFIX ""
        )

install(TARGETS optimize
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )