cmake_minimum_required(VERSION 3.10)
project(optimize)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        optimize/init.cpp
        optimize/main.cpp
        optimize/optimize.h
        optimize/particle_swarm_optimization.cpp
        optimize/particle_swarm_optimization.h
        optimize/simple.cpp
        optimize/simple.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_PATH}
#        )