cmake_minimum_required(VERSION 3.10)
project(reliability)
set(CMAKE_CXX_STANDARD 11)

include_directories(../gldcore)

add_library(${PROJECT_NAME} SHARED
        reliability/eventgen.cpp
        reliability/eventgen.h
        reliability/init.cpp
        reliability/main.cpp
        reliability/metrics.cpp
        reliability/metrics.h
        reliability/reliability.h
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES PREFIX ""
        )

#install(TARGETS ${PROJECT_NAME}
#        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#        )